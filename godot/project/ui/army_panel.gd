extends PanelContainer
## ArmyPanel — la barre de COMMANDEMENT du pion sélectionné, PORTÉE au squelette unifié
## (PanelContainer racine + ParchTheme.build() + HeaderStrip + LedTabStrip + corps VBox
## rebâti au refresh, CONTENEURS NATIFS, ZÉRO `_draw` — le même patron que
## province_panel_v2.gd / empire_window.gd). Deux onglets : COMPOSITION (barre d'unités,
## campagne, actions) et COMBAT (vide hors engagement, temps réel en direct, résultat figé
## à la conclusion). Zéro logique sim : lit corps_info/army_info/battle_info/region_war_state,
## enfile des verbes déjà câblés (player_raise_corps, player_refill_corps, player_split_corps,
## player_split_comp, player_merge_corps, player_disband_corps) — port de PRÉSENTATION, aucun
## verbe changé côté logique (le split composé OUVRE troop_select.gd, panneau satellite).
## Montré/caché par map_view.army_selection_changed (main le câble, cf. main.gd:239-245).
##
## RETOUR JOUEUR 2026-07-28 (icônes/chiffres, pas de barres ; État silencieux si RAS ;
## pillage en case à cocher ; renfort = le déficit seul, détail au hover ; nom du corps
## cliquable → sélection de troupes) : cf. les commentaires de section ci-dessous.

const ParchTheme = preload("res://ui/parch_theme.gd")
const PopBar = preload("res://ui/pop_bar.gd")     # encore utilisé par _side_block (combat)
const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")
const Concepts = preload("res://ui/concepts.gd")   # D4 — glossaire hover
const BattleAnim = preload("res://ui/battle_anim.gd")
const TroopSelect = preload("res://ui/troop_select.gd")   # panneau satellite : split composé
const UIKit = preload("res://ui/uikit.gd")   # Chantier F : glyphes d'unités (lot5_troupes)

signal raid_requested   ## case « Pillage » cochée → main arme le sous-mode raid de la carte
signal raid_disarmed    ## décochée → désarme
signal selection_replaced(ids: Array) ## fusion : le corps survivant devient l'unique sélection

const PW := 420.0

## icônes de PHASE (lot11_systeme pha_*, campagne 2, 2026-08-26) — le mot de
## campaign_phase_name() (scps_campaign.c) → l'icône, quand un des 7 livrés
## (lever/marche/pillage/renfort/repli/siège/bataille) correspond. Au repos/Embarque/
## En mer/Débarque/« N phases » n'ont pas d'équivalent — pas d'icône (cf. TROUVAILLES
## 2026-08-26 Restes) plutôt qu'un mauvais choix forcé.
const PHASE_ICON := {
	"En marche": "pha_marche", "En siège": "pha_siege", "En mêlée": "pha_bataille",
}

var _selected_ids: Array[int] = []
var _move_preview: Dictionary = {}
var _refill_previews: Array[Dictionary] = []
var _disband_armed := false
var _disband_ms := -100000
var _flash := ""
var _flash_good := true
var _flash_ms := -100000

# STRUCTURE (décision joueur 2026-07-25) : plus d'onglets — UNE colonne : troupes ·
# composition · raccourcis · stats · (combat s'il y en a un) · et EN BAS, la formation
# (battle_anim : parade au repos, le choc animé en bataille). Le détail tactique complet
# reste dans battle_panel (clic sur le jeton).
var _body: VBoxContainer = null     # corps rebâti à chaque refresh
var _title_lbl: Label = null
var _sub_lbl: Label = null
var _phase_lbl: Label = null
var _phase_icon: TextureRect = null
var _anim: Control = null           # la FORMATION (persistante, hors du rebuild du corps)
var _anim_battle_region := -1       # bataille armée dans l'anim (-1 = parade/aucune)
var _parade_sig := []               # signature de compo de la parade (évite le re-setup au tick)
var _raid_on := false               # état de la case Pillage (remis à zéro à la re-sélection)
var _troops: Control = null         # troop_select (satellite), instancié à la demande

# ── SECTION COMBAT — vide si aucun combat, EN TEMPS RÉEL (Sim.ticked, chaque jour)
# sinon, et persiste le RÉSULTAT (victoire/défaite + pertes) jusqu'à re-sélection ou
# nouveau combat. Lit scps_battle_info/region_war_state ; zéro logique de sim.
var _battle_region := -1
var _battle_live: Dictionary = {}     # dernier battle_info valide vu pour _battle_region
var _battle_result: Dictionary = {}   # { bi, ws } — figé quand le combat vient de se conclure

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, 0)   # largeur plancher ; hauteur AU CONTENU
	theme = ParchTheme.build()
	_apply_chrome_bg()
	_build_shell()
	visible = false
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: _refresh())

func _process(_dt: float) -> void:
	if _disband_armed and Time.get_ticks_msec() - _disband_ms > 4000:
		_disband_armed = false
		if visible:
			_refresh()
	if _flash != "" and Time.get_ticks_msec() - _flash_ms > 3000:
		_flash = ""
		if visible:
			_refresh()

## appelé par main sur map_view.army_selection_changed(on)
func set_army(ids: Array) -> void:
	var new_ids: Array[int] = []
	for id in ids: new_ids.append(int(id))
	if new_ids != _selected_ids:
		# re-sélection : on oublie le combat/résultat suivi (le nouveau corps n'est pas
		# forcément celui qui combattait) et la parade se ré-arme.
		_battle_region = -1
		_battle_live = {}
		_battle_result = {}
		_anim_battle_region = -1
		_parade_sig = []
		_raid_on = false
		if _troops != null:
			_troops.visible = false
	_selected_ids = new_ids
	visible = not _selected_ids.is_empty()
	if visible:
		_disband_armed = false
		_refresh()

## CHROME 2026-08-26 : la plaque livrée `chrome_panel_armee_bg` remplace le fond
## parchemin PAR DÉFAUT de ce panneau (grain_sb PANEL_BG, `ParchTheme.build()` —
## PARTAGÉ par province_panel_v2/empire_window/budget_panel_v2 : on ne touche PAS le
## thème commun, on pose un override D'INSTANCE sur CE panneau seul). 9-slice (cap
## 64 px, source 2×) : la largeur est fixe (PW) mais la hauteur varie au contenu —
## les coins/médaillons de la plaque restent nets, seul le centre s'étire. No-op (le
## fond parchemin d'origine reste) si l'asset manque encore (.import pas généré, etc).
func _apply_chrome_bg() -> void:
	var tex := UIKit.chrome_panel_armee_bg()
	if tex == null:
		return
	var sb := StyleBoxTexture.new()
	sb.texture = tex
	sb.texture_margin_left = 64.0
	sb.texture_margin_right = 64.0
	sb.texture_margin_top = 64.0
	sb.texture_margin_bottom = 64.0
	add_theme_stylebox_override("panel", sb)

# ── LE SQUELETTE (header + onglets + corps) ───────────────────────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER : nom/numéro du corps + effectif à gauche · phase courante à droite
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	var lcol := VBoxContainer.new()
	lcol.add_theme_constant_override("separation", 0)
	lcol.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(lcol)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "⚔ Votre armée"
	_title_lbl.clip_text = true
	lcol.add_child(_title_lbl)
	_sub_lbl = Label.new()
	_sub_lbl.theme_type_variation = "RowDim"
	_sub_lbl.text = "—"
	lcol.add_child(_sub_lbl)
	var rcol := VBoxContainer.new()
	rcol.add_theme_constant_override("separation", 0)
	rcol.size_flags_horizontal = Control.SIZE_SHRINK_END
	hb.add_child(rcol)
	var phase_row := HBoxContainer.new()
	phase_row.add_theme_constant_override("separation", 4)
	rcol.add_child(phase_row)
	_phase_icon = TextureRect.new()
	_phase_icon.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_phase_icon.stretch_mode = TextureRect.STRETCH_SCALE
	_phase_icon.custom_minimum_size = Vector2(18, 18)
	_phase_icon.visible = false
	phase_row.add_child(_phase_icon)
	_phase_lbl = Label.new()
	_phase_lbl.theme_type_variation = "RowLabel"
	_phase_lbl.text = "Réserve"
	_phase_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	phase_row.add_child(_phase_lbl)

	# CORPS (fond transparent — laisse voir le parchemin)
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	root.add_child(bodypanel)
	_body = VBoxContainer.new()
	_body.add_theme_constant_override("separation", 4)
	bodypanel.add_child(_body)

	# EN BAS, LA FORMATION — widget PERSISTANT (état d'anim : jamais dans le rebuild)
	var animpanel := PanelContainer.new()
	animpanel.theme_type_variation = "Body"
	root.add_child(animpanel)
	var center := CenterContainer.new()
	animpanel.add_child(center)
	_anim = BattleAnim.new()
	center.add_child(_anim)

## compat sondes (l'ex-onglet Combat n'existe plus — colonne unique)
func select_tab(_idx: int) -> void:
	_refresh()

## un mouvement survolé sur la carte (avant clic) : juste le texte d'aperçu bouge,
## pas de re-tirage complet des corps/renfort — cette entrée peut arriver à chaque
## changement de région survolée pendant un glissé.
func set_move_preview(preview: Dictionary) -> void:
	_move_preview = preview.duplicate(true)
	if visible:
		_refresh()

func show_feedback(message: String, good: bool) -> void:
	_flash_msg(message, good)

# ── REFRESH : colonne unique (troupes · composition · raccourcis · stats · combat) ─
func _refresh() -> void:
	var w = Sim.world
	if w == null or _body == null:
		return
	var me := int(w.player())
	var data := _gather_corps(w, me)
	_update_header(data)
	_refresh_combat_state(w, data.regions)
	for c in _body.get_children():
		c.queue_free()
	_build_composition_tab(w, me, data)
	if not _battle_live.is_empty():
		_body.add_child(_phase_title(String(_battle_live.get("phase", "?"))))
		_build_combat_live(_battle_live)
	elif not _battle_result.is_empty():
		_body.add_child(_line("COMBAT — TERMINÉ", "Section"))
		_build_combat_result(_battle_result)
	_feed_anim(data)
	_layout.call_deferred()

## EN BAS, LA FORMATION : le choc animé quand une bataille est vive, la PARADE sinon
## (le biome du lieu n'a de reader qu'en combat — la parade pose le fond par défaut).
func _feed_anim(data: Dictionary) -> void:
	if _anim == null:
		return
	if not _battle_live.is_empty() and bool(_battle_live.get("in_battle", false)):
		if _anim_battle_region != _battle_region:
			_anim.setup(_battle_live)
			_anim_battle_region = _battle_region
		_anim.on_tick(_battle_live)
		_anim.visible = true
		return
	_anim_battle_region = -1
	if int(data.active) <= 0:
		_anim.visible = false
		_parade_sig = []
		return
	var sig := [int(data.inf), int(data.arch), int(data.cav), int(data.mages)]
	if sig != _parade_sig:
		_parade_sig = sig
		_anim.setup_parade({"inf": data.inf, "arch": data.arch, "cav": data.cav,
			"mages": data.mages, "region": (data.regions as Array)[0] if not (data.regions as Array).is_empty() else 0})
	_anim.visible = true

## rassemble les corps SÉLECTIONNÉS valides : agrégats (effectif, composition) +
## la liste brute (pour les lignes détaillées et les gates d'action).
func _gather_corps(w, me: int) -> Dictionary:
	var total := 0; var inf := 0; var arch := 0; var cav := 0; var mages := 0; var active := 0
	var phase := "Réserve"
	var phases: Array[String] = []
	var regions: Array[int] = []
	var corps_data: Array[Dictionary] = []
	for id in _selected_ids:
		var a: Dictionary = w.corps_info(id) if w.has_method("corps_info") else w.army_info(me)
		if not bool(a.get("active", false)): continue
		if not bool(a.get("units_are_humans", false)):
			# Compatibilité avec la DLL debug précédente : elle exposait encore des paquets.
			for key in ["units", "inf", "arch", "cav", "mages"]:
				a[key] = int(a.get(key, 0)) * 100
		corps_data.append(a)
		active += 1
		total += int(a.get("units", 0)); inf += int(a.get("inf", 0)); arch += int(a.get("arch", 0))
		cav += int(a.get("cav", 0)); mages += int(a.get("mages", 0))
		phase = String(a.get("phase", "?"))
		if phase not in phases: phases.append(phase)
		var region := int(a.get("region", -1))
		if region not in regions: regions.append(region)
	var reserve := 0
	if active == 0 and w.has_method("country_army"):
		reserve = int(w.country_army(me).get("regiments", 0))
	return {"active": active, "total": total, "inf": inf, "arch": arch, "cav": cav, "mages": mages,
		"phase": phase, "phases": phases, "regions": regions, "corps_data": corps_data, "reserve": reserve}

## pose (ou cache) l'icône de phase 18 px devant `_phase_lbl` — cf. PHASE_ICON.
func _set_phase_icon(word: String) -> void:
	if _phase_icon == null:
		return
	var name: String = PHASE_ICON.get(word, "")
	var t: Texture2D = UIKit.icon2(name) if name != "" else null
	_phase_icon.texture = t
	_phase_icon.visible = t != null

func _update_header(data: Dictionary) -> void:
	var active := int(data.active)
	var phases: Array = data.phases
	if active > 0:
		var corps_data: Array = data.corps_data
		_title_lbl.text = "⚔ %d corps" % active if active > 1 else "⚔ Corps #%d" % int(corps_data[0].get("id", -1))
		_sub_lbl.text = "%s hommes · %s inf · %s dist · %s cav · %s mages" % [
			_grp(int(data.total)), _grp(int(data.inf)), _grp(int(data.arch)), _grp(int(data.cav)), _grp(int(data.mages))]
		_phase_lbl.text = String(data.phase) if phases.size() == 1 else "%d phases" % phases.size()
		_set_phase_icon(_phase_lbl.text)
	else:
		_title_lbl.text = "⚔ Votre armée"
		_sub_lbl.text = "réserve : %d régiment(s)" % int(data.reserve)
		_phase_lbl.text = "Réserve"
		_set_phase_icon("")

# ── LA COLONNE : troupes · composition · raccourcis · stats (ordre joueur 2026-07-25) ──
func _build_composition_tab(w, me: int, data: Dictionary) -> void:
	var active := int(data.active)
	var corps_data: Array[Dictionary] = data.corps_data
	var regions: Array[int] = data.regions
	# TROUPES — les corps (compact : 3 + résumé de pile)
	if active > 0:
		for i in range(mini(corps_data.size(), 3)):
			_body.add_child(_corps_block(corps_data[i]))
		if corps_data.size() > 3:
			_dim_line("… et %d autres corps" % (corps_data.size() - 3))
		if corps_data.size() >= 2:
			var gold := _line(_stack_summary_text(corps_data, regions, int(data.total)), "RowLabel")
			gold.add_theme_color_override("font_color", ParchTheme.HEADER_INK)
			_body.add_child(gold)
	else:
		_dim_line("Cliquez une province : à vous → repositionner · ennemie → attaquer (siège, assaut, occupation & butin).")

	# COMPOSITION — glyphes+chiffres (le même langage que la formation en bas), pas de barre
	if int(data.total) > 0:
		_body.add_child(_line("COMPOSITION", "Section"))
		_body.add_child(_compo_glyphs(int(data.inf), int(data.arch), int(data.cav), int(data.mages)))

	# RACCOURCIS — lever · renforcer (déficit) · pillage (case) · scinder · fusionner · dissoudre
	_refresh_refill_data(w)
	if not _move_preview.is_empty():
		_body.add_child(_tone_line(_move_preview_text(_move_preview), _move_preview_tone()))
	_body.add_child(_action_row(w, me, data))
	if _flash != "":
		_body.add_child(_tone_line(_flash, 0.80 if _flash_good else 0.20))

	# ÉTAT — seulement s'il dit quelque chose (BRISÉ/ralliement · cohésion en bataille)
	if active > 0:
		var st := _stats_block(data)
		if st != null:
			_body.add_child(st)

## l'état ne parle que s'il y a quelque chose à dire — null sinon (pas de section vide)
func _stats_block(data: Dictionary) -> Control:
	var broken := 0; var rally := 0
	for a in (data.corps_data as Array):
		broken = maxi(broken, int(a.get("broken_days", 0)))
		rally += int(a.get("rally_units", 0))
	var in_battle := not _battle_live.is_empty() and bool(_battle_live.get("in_battle", false))
	if broken <= 0 and rally <= 0 and not in_battle:
		return null
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 2)
	if broken > 0:
		box.add_child(_stat_line("État", "BRISÉ — %d j" % broken, 0.12, "pha_repli"))
	elif rally > 0:
		box.add_child(_stat_line("État", "ralliement · %s hommes en route" % _grp(rally), 0.45, "pha_repli"))
	if in_battle:
		var me := int(Sim.world.player())
		var pfx := "atk_" if int(_battle_live.get("attacker", -1)) == me else "def_"
		var mp := clampi(int(_battle_live.get(pfx + "morale_pct", 0)), 0, 100)
		box.add_child(_stat_line("Cohésion", "%d%%" % mp, float(mp) / 100.0))
		box.add_child(_mini_bar(mp))
	return box

## une carte de corps : NOM CLIQUABLE (→ sélection de troupes, split composé) + glyphes
func _corps_block(a: Dictionary) -> Control:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 1)
	var name_btn := Button.new()
	name_btn.text = _corps_status_text(a)
	name_btn.flat = true
	name_btn.focus_mode = Control.FOCUS_NONE
	name_btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	name_btn.add_theme_color_override("font_color", ParchTheme.INK)
	name_btn.add_theme_color_override("font_hover_color", ParchTheme.HEADER_INK)
	name_btn.tooltip_text = "Choisir les troupes à détacher (scission sur mesure)"
	var cid := int(a.get("id", -1))
	name_btn.pressed.connect(func(): _open_troops(cid))
	box.add_child(name_btn)
	var inf := int(a.get("inf", 0)); var arch := int(a.get("arch", 0))
	var cav := int(a.get("cav", 0)); var mages := int(a.get("mages", 0))
	if inf + arch + cav + mages > 0:
		box.add_child(_compo_glyphs(inf, arch, cav, mages))
	return box

## GLYPHE d'unité (registre UIKit.unit_icon, Chantier F) + chiffre, teinté par catégorie —
## un représentant par catégorie AGRÉGÉE (inf/arch/cav/mages : le grain le plus fin que
## corps_info() expose côté moteur à ce niveau — pas de rupture par UnitType individuel
## sans nouveau reader façade, hors périmètre de cette passe). Repli texte (■▲●▬) si
## l'asset manque — jamais un carré magenta. Nom de catégorie au hover.
const _COMPO_GLYPH_FALLBACK := ["■", "▲", "●", "▬"]
func _compo_glyphs(inf: int, arch: int, cav: int, mages: int) -> Control:
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 10)
	# [UnitType représentant, effectif, nom catégorie, couleur]
	var defs := [[0, inf, "Infanterie", VKit.SLICE_PAL[0]], [3, arch, "Tirailleurs/archers", VKit.SLICE_PAL[1]],
		[5, cav, "Cavalerie", VKit.SLICE_PAL[3]], [7, mages, "Mages", VKit.SLICE_PAL[5]]]
	for i in range(defs.size()):
		var d: Array = defs[i]
		if int(d[1]) <= 0: continue
		var cell := HBoxContainer.new()
		cell.add_theme_constant_override("separation", 3)
		cell.mouse_filter = Control.MOUSE_FILTER_STOP
		cell.tooltip_text = String(d[2])
		var tex: Texture2D = UIKit.unit_icon(int(d[0]))
		if tex != null:
			var tr := TextureRect.new()
			tr.texture = tex
			# EXPAND_IGNORE_SIZE : sinon le Control adopte la taille NATIVE de la texture
			# (128²) et ignore custom_minimum_size (piège Godot 4, pas Godot 3 `expand`).
			tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			tr.stretch_mode = TextureRect.STRETCH_SCALE
			tr.custom_minimum_size = Vector2(16, 16)
			tr.modulate = d[3]
			tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
			cell.add_child(tr)
		else:
			var glyph := Label.new()
			glyph.theme_type_variation = "RowLabel"
			glyph.text = _COMPO_GLYPH_FALLBACK[i]
			glyph.add_theme_color_override("font_color", d[3])
			cell.add_child(glyph)
		var l := Label.new()
		l.theme_type_variation = "RowLabel"
		l.text = _grp(int(d[1]))
		l.add_theme_color_override("font_color", d[3])
		cell.add_child(l)
		row.add_child(cell)
	return row

## ouvre/rafraîchit le panneau satellite de sélection de troupes pour le corps `cid`
func _open_troops(cid: int) -> void:
	if _troops == null:
		_troops = TroopSelect.new()
		_troops.position = Vector2(PW + 6.0, 0.0)
		_troops.split_ordered.connect(func(msg: String, good: bool):
			_flash_msg(msg, good))
		add_child(_troops)
	_troops.open_for(cid)

## membres pour PopBar.proportion_bar (nom/valeur/couleur/pct) — même langage de
## couleur que battle_panel._compo_bar (SLICE_PAL 0/1/3/5).
func _compo_members(inf: int, arch: int, cav: int, mages: int) -> Array:
	var tot: int = maxi(1, inf + arch + cav + mages)
	var raw := [["Infanterie", inf, VKit.SLICE_PAL[0]], ["Tirailleurs/archers", arch, VKit.SLICE_PAL[1]],
		["Cavalerie", cav, VKit.SLICE_PAL[3]], ["Mages", mages, VKit.SLICE_PAL[5]]]
	var out := []
	for e in raw:
		var v: int = e[1]
		if v <= 0: continue
		out.append({"name": e[0], "value": float(v), "pct": roundi(100.0 * float(v) / float(tot)), "color": e[2]})
	return out

## la rangée d'ACTIONS : Lever · Renforcer · Piller la côte · Scinder · Fusionner ·
## Dissoudre — vrais Buttons ParchTheme, désactivés + motif de refus au hover.
func _action_row(w, me: int, data: Dictionary) -> HBoxContainer:
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 4)

	var raise_btn := _action_btn("Lever un corps",
		"Détache la moitié de la réserve nationale à la capitale.", "pha_lever")
	raise_btn.pressed.connect(_do_raise)
	row.add_child(raise_btn)

	# RENFORCER = le DÉFICIT seul (requested_humans = manque vs force nominale ; 0 ⇒ grisé)
	var totals := _refill_totals(_refill_previews)
	var deficit := int(totals.requested)
	var refill_ok := int(totals.allowed) > 0 and deficit > 0
	var refill_btn := _action_btn(
		("Renforcer (+%s)" % _grp(deficit)) if refill_ok else "Renforcer",
		_refill_tooltip(_refill_previews) if refill_ok else "Corps à pleine force.", "pha_renfort")
	refill_btn.disabled = not refill_ok
	refill_btn.pressed.connect(_do_refill)
	row.add_child(refill_btn)

	# PILLAGE — une CASE (le verbe est un mode de l'armée, pas un bouton-phrase). Icône
	# posée à CÔTÉ (TextureRect séparé, motif _compo_glyphs, plutôt que CheckButton.icon)
	# — PAS pour contourner un bug d'icône (aucun : Button.icon direct fonctionne très
	# bien ailleurs dans ce même fichier, cf. raise_btn/refill_btn juste au-dessus) mais
	# parce que le texte « Pillage » de CE CheckButton est INVISIBLE au probe, un bug
	# PRÉEXISTANT (confirmé sur le HEAD d'avant cette mission, `font_color` = ParchTheme.INK
	# sur le fond sombre du panneau, sans stylebox clair derrière comme les boutons
	# voisins) — AUCUN rapport avec l'icône. Non corrigé ici (hors mandat de cette
	# mission) ; signalé en TROUVAILLES Restes + tâche de fond.
	var raid_wrap := HBoxContainer.new()
	raid_wrap.add_theme_constant_override("separation", 3)
	var raid_icon := UIKit.icon2("pha_pillage")
	if raid_icon != null:
		var raid_ic := TextureRect.new()
		raid_ic.texture = raid_icon
		raid_ic.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		raid_ic.stretch_mode = TextureRect.STRETCH_SCALE
		raid_ic.custom_minimum_size = Vector2(18, 18)
		raid_wrap.add_child(raid_ic)
	var raid_ck := CheckButton.new()
	raid_ck.text = "Pillage"
	raid_ck.focus_mode = Control.FOCUS_NONE
	raid_ck.button_pressed = _raid_on
	raid_ck.tooltip_text = "Cochée : le prochain clic sur une province côtière étrangère la pille (coque pirate requise)."
	raid_ck.add_theme_font_size_override("font_size", 12)
	raid_ck.add_theme_color_override("font_color", ParchTheme.INK)
	raid_ck.add_theme_color_override("font_hover_color", ParchTheme.INK)
	raid_ck.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	raid_ck.add_theme_color_override("font_hover_pressed_color", ParchTheme.INK)
	raid_ck.toggled.connect(func(on: bool):
		_raid_on = on
		if on: raid_requested.emit()
		else: raid_disarmed.emit())
	raid_wrap.add_child(raid_ck)
	row.add_child(raid_wrap)

	var corps_data: Array = data.corps_data
	var split_ok := int(data.active) > 0
	for c in corps_data:
		var spid := int(c.get("phase_id", 0))
		if int(c.get("units", 0)) < 200 or spid == 3 or spid >= 4: split_ok = false
	var split_btn := _action_btn("Scinder", "Sépare chaque corps sélectionné en deux détachements égaux.")
	split_btn.disabled = not split_ok
	if not split_ok:
		split_btn.tooltip_text = "Scission impossible : il faut ≥200 hommes par corps, hors bataille et hors mer."
	split_btn.pressed.connect(_do_split)
	row.add_child(split_btn)

	var merge_ok := int(data.active) >= 2 and int((data.regions as Array).size()) == 1
	for c in corps_data:
		var mpid := int(c.get("phase_id", 0))
		if mpid == 3 or mpid >= 4: merge_ok = false
	var merge_btn := _action_btn("Fusionner",
		"Fusion possible : tous les corps sont au même endroit." if merge_ok else
		"Fusion impossible : sélectionnez au moins deux corps co-localisés, hors bataille et hors mer.")
	merge_btn.disabled = not merge_ok
	merge_btn.pressed.connect(_do_merge)
	row.add_child(merge_btn)

	var disband_btn := _action_btn("Confirmer ?" if _disband_armed else "Dissoudre",
		"Dissout définitivement le(s) corps sélectionné(s)." if not _disband_armed else "Un second clic dissout pour de bon.")
	disband_btn.add_theme_color_override("font_color", VKit.sense(0.10) if _disband_armed else ParchTheme.INK)
	disband_btn.pressed.connect(_do_disband)
	row.add_child(disband_btn)

	return row

func _action_btn(txt: String, tip: String, phase_icon: String = "") -> Button:
	var b := Button.new()
	b.text = txt
	b.tooltip_text = tip
	b.focus_mode = Control.FOCUS_NONE
	b.custom_minimum_size = Vector2(0, 32)
	b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	b.add_theme_font_size_override("font_size", 12)
	if phase_icon != "":
		var t := UIKit.icon2(phase_icon)
		if t != null:
			b.icon = t
			b.icon_alignment = HORIZONTAL_ALIGNMENT_LEFT
			b.expand_icon = false
			b.add_theme_constant_override("icon_max_width", 18)
	b.add_theme_stylebox_override("normal", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 3, 5, 5, 3, 3))
	b.add_theme_stylebox_override("hover", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.TAB_UNDERLINE, 1, 3, 5, 5, 3, 3))
	b.add_theme_stylebox_override("pressed", ParchTheme.sb(ParchTheme.DIVIDER, ParchTheme.TAB_UNDERLINE, 1, 3, 5, 5, 3, 3))
	b.add_theme_stylebox_override("disabled", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.DIVIDER, 1, 3, 5, 5, 3, 3))
	b.add_theme_color_override("font_color", ParchTheme.INK)
	b.add_theme_color_override("font_hover_color", ParchTheme.INK)
	b.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	b.add_theme_color_override("font_disabled_color", ParchTheme.DIM_INK)
	return b

## rafraîchit l'état de combat suivi (_battle_region/_battle_live/_battle_result) depuis
## les régions des corps sélectionnés — plus d'onglet : la section combat + la formation
## animée apparaissent d'elles-mêmes dans la colonne quand un engagement s'allume.
func _refresh_combat_state(w, regions: Array) -> void:
	if w == null or not w.has_method("battle_info") or not w.has_method("region_war_state") or regions.is_empty():
		return
	var bi := {}
	var region := -1
	for r in regions:
		var cand: Dictionary = w.battle_info(int(r))
		if bool(cand.get("valid", false)):
			bi = cand; region = int(r); break
	if region >= 0:
		_battle_region = region
		_battle_live = bi.duplicate(true)
		_battle_result = {}
		return
	if not _battle_live.is_empty() and _battle_region >= 0:
		var ws: Dictionary = w.region_war_state(_battle_region)
		_battle_result = {"bi": _battle_live.duplicate(true), "ws": ws.duplicate(true)}
		_battle_live = {}

func _country_name(cid: int) -> String:
	if cid < 0 or Sim.world == null:
		return "—"
	var info: Dictionary = Sim.world.country_info(cid)
	return String(info.get("nom", "—"))

func _phase_title(txt: String) -> Control:
	var icon_name: String = PHASE_ICON.get(txt, "")
	var l := Label.new()
	l.theme_type_variation = "RowLabel"
	l.text = txt
	l.add_theme_font_size_override("font_size", 16)
	l.add_theme_color_override("font_color", ParchTheme.HEADER_INK)
	if icon_name == "":
		return l
	var t := UIKit.icon2(icon_name)
	if t == null:
		return l
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 6)
	var ic := TextureRect.new()
	ic.texture = t
	ic.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	ic.stretch_mode = TextureRect.STRETCH_SCALE
	ic.custom_minimum_size = Vector2(18, 18)
	row.add_child(ic)
	row.add_child(l)
	return row

func _stat_line(label: String, value: String, tone: float = -1.0, phase_icon: String = "") -> Control:
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 6)
	if phase_icon != "":
		var t := UIKit.icon2(phase_icon)
		if t != null:
			var ic := TextureRect.new()
			ic.texture = t
			ic.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			ic.stretch_mode = TextureRect.STRETCH_SCALE
			ic.custom_minimum_size = Vector2(18, 18)
			h.add_child(ic)
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = label
	l.custom_minimum_size = Vector2(140, 0)
	h.add_child(l)
	var v := Label.new()
	v.theme_type_variation = "RowLabel"
	v.text = value
	if tone >= 0.0:
		v.add_theme_color_override("font_color", VKit.sense(tone))
	h.add_child(v)
	return h

func _side_block(bi: Dictionary, is_atk: bool, cid: int, me: int) -> Control:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 2)
	var prefix := "atk" if is_atk else "def"
	var role := "Attaquant" if is_atk else "Défenseur"
	var name_lbl := Label.new()
	name_lbl.theme_type_variation = "RowLabel"
	name_lbl.text = "%s — %s%s" % [role, _country_name(cid), "  (VOUS)" if cid == me else ""]
	name_lbl.add_theme_color_override("font_color", ParchTheme.HEADER_INK if cid == me else ParchTheme.INK)
	box.add_child(name_lbl)
	var unit_scale := 1 if bool(bi.get("units_are_humans", false)) else 100
	var units := int(bi.get(prefix + "_units", 0)) * unit_scale
	if units > 0:
		box.add_child(_line("%s hommes · %d corps" % [_grp(units), int(bi.get(prefix + "_corps", 0))], "RowDim"))
		box.add_child(_compo_glyphs(
			int(bi.get(prefix + "_inf", 0)) * unit_scale, int(bi.get(prefix + "_arch", 0)) * unit_scale,
			int(bi.get(prefix + "_cav", 0)) * unit_scale, int(bi.get(prefix + "_mages", 0)) * unit_scale))
	else:
		box.add_child(_stat_line("Force", "place forte" if not is_atk else "—"))
	if bool(bi.get("in_battle", false)) and bi.has(prefix + "_morale_pct"):
		var mp := clampi(int(bi.get(prefix + "_morale_pct", 0)), 0, 100)
		box.add_child(_stat_line("Cohésion", "%d%%" % mp, float(mp) / 100.0))
		box.add_child(_mini_bar(mp))
	var helper := int(bi.get(prefix + "_helper", -1))
	if helper >= 0:
		box.add_child(_stat_line("Renfort", _country_name(helper)))
	return box

func _mini_bar(v: int) -> ProgressBar:
	var pb := ProgressBar.new()
	pb.min_value = 0; pb.max_value = 100; pb.value = v; pb.show_percentage = false
	pb.custom_minimum_size = Vector2(0, 8)
	pb.add_theme_stylebox_override("background", ParchTheme.sb(Color("caa768"), ParchTheme.BORDER, 1, 2, 0, 0, 0, 0))
	pb.add_theme_stylebox_override("fill", ParchTheme.sb(VKit.sense(float(v) / 100.0), Color(0, 0, 0, 0), 0, 2, 0, 0, 0, 0))
	return pb

func _tactic_block(bi: Dictionary, atk: int, df: int) -> Control:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 2)
	box.add_child(_line("LECTURE TACTIQUE — %s" % String(bi.get("stage", "—")), "Section"))
	var holder := int(bi.get("terrain_holder", -1))
	var terr := "neutre"
	if holder == atk: terr = "avantage attaquant %+d%%" % (int(bi.get("atk_terrain_pct", 100)) - 100)
	elif holder == df: terr = "avantage défenseur %+d%%" % (int(bi.get("def_terrain_pct", 100)) - 100)
	box.add_child(_stat_line("Terrain", terr))
	if bool(bi.get("river", false)):
		box.add_child(_stat_line("Rivière", "pontée" if bool(bi.get("bridged", false)) else "non pontée"))
	box.add_child(_stat_line("Contres", "attaquant %+d%% · défenseur %+d%%" % [
		int(bi.get("atk_counter_pct", 100)) - 100, int(bi.get("def_counter_pct", 100)) - 100]))
	var bal := int(bi.get("balance_atk_pct", 50))
	box.add_child(_stat_line("Rapport pré-aléa", "%d / %d (±15%% au choc)" % [bal, 100 - bal]))
	box.add_child(_stat_line("Rupture", "sous %d%% de cohésion" % int(bi.get("rupture_pct", 0))))
	box.add_child(_stat_line("Pertes attaquant", "%s hommes" % _grp(int(float(bi.get("loss_atk", 0.0)) * 100.0)), 0.15))
	box.add_child(_stat_line("Pertes défenseur", "%s hommes" % _grp(int(float(bi.get("loss_def", 0.0)) * 100.0)), 0.15))
	return box

func _siege_block(bi: Dictionary) -> Control:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 2)
	var siege_head := _line("LECTURE DU SIÈGE", "Section")
	# D4 — glossaire hover : « Cohésion » plus bas dans ce même onglet est le MORAL de
	# bataille (moteur), pas la Cohésion nationale de concepts.gd — collision volontairement
	# NON câblée (mauvaise définition sinon) ; « Siège » n'a, lui, qu'un seul sens ici.
	var sdef := Concepts.def_of("Siège")
	if sdef != "":
		siege_head.tooltip_text = sdef
		siege_head.mouse_filter = Control.MOUSE_FILTER_STOP
	box.add_child(siege_head)
	var sp := clampi(int(bi.get("siege_progress_pct", 0)), 0, 100)
	box.add_child(_stat_line("Progression estimée", "%d%%" % sp))
	box.add_child(_mini_bar(sp))
	box.add_child(_stat_line("Échéance", "%.0f j restants / %.0f j de résistance" % [
		float(bi.get("siege_days_left", 0.0)), float(bi.get("siege_full_days", 0.0))]))
	box.add_child(_stat_line("Ouvrages", "défense %.1f" % float(bi.get("siege_defense", 0.0))))
	box.add_child(_stat_line("Vivres", "%.1f mois" % float(bi.get("siege_food_months", 0.0))))
	var tp := int(bi.get("siege_terrain_pct", 100))
	box.add_child(_stat_line("Terrain", "tenue neutre" if tp == 100 else "%+d%% de tenue" % (tp - 100)))
	box.add_child(_stat_line("À la chute", "libération de la région" if int(bi.get("siege_outcome", 0)) == 1 else "occupation de la région"))
	return box

func _build_combat_live(bi: Dictionary) -> void:
	var me := int(Sim.world.player())
	var atk := int(bi.get("attacker", -1))
	var df := int(bi.get("defender", -1))
	var sub := ""
	if bool(bi.get("in_battle", false)):
		sub = "Jour %d · %d choc(s) livré(s)" % [int(bi.get("days", 0)), int(bi.get("chocs", 0))]
	else:
		sub = "%.0f j restants · %d%% estimés" % [float(bi.get("siege_days_left", 0.0)), int(bi.get("siege_progress_pct", 0))]
	_body.add_child(_line(sub, "RowDim"))
	_body.add_child(_side_block(bi, true, atk, me))
	_body.add_child(_side_block(bi, false, df, me))
	if bool(bi.get("in_battle", false)):
		_body.add_child(_tactic_block(bi, atk, df))
	else:
		_body.add_child(_siege_block(bi))
	var ws := float(bi.get("war_score", 0.0))
	_body.add_child(_stat_line("Score de guerre", "%+.0f (point de vue attaquant)" % ws, 0.5 + ws / 200.0))

## le VERDICT de bataille du FIL (kinds 8/9/11 — le moteur l'a déjà tranché, côté
## joueur) pour la région suivie ; {} si aucun (siège pur, ou combat IA-vs-IA).
func _feed_battle_verdict(region: int) -> Dictionary:
	var w = Sim.world
	if w == null or not w.has_method("feed_poll") or region < 0:
		return {}
	var found := {}
	for ev in w.feed_poll(0):   # lecture PURE (curseur 0 = tout le ring borné)
		var kind := int(ev.get("kind", -1))
		if (kind == 8 or kind == 9 or kind == 11) and int(ev.get("region", -1)) == region:
			found = ev   # on garde le DERNIER (le ring est chronologique)
	return found

func _build_combat_result(result: Dictionary) -> void:
	var bi: Dictionary = result.get("bi", {})
	var ws: Dictionary = result.get("ws", {})
	var w = Sim.world
	var me := int(w.player())
	var atk := int(bi.get("attacker", -1))
	var df := int(bi.get("defender", -1))
	var was_battle := bool(bi.get("in_battle", false))
	var res_region := int(bi.get("region", _battle_region))
	# le siège abouti se lit de DEUX façons : région OCCUPÉE par l'attaquant (state 2,
	# la paix n'a pas encore transféré) OU propriété DÉJÀ basculée (region_owner == atk).
	var attacker_took := (int(ws.get("state", 0)) == 2 and int(ws.get("belligerent", -1)) == atk)
	if not attacker_took and res_region >= 0 and atk >= 0 and atk != df \
			and w.has_method("region_owner"):
		attacker_took = int(w.region_owner(res_region)) == atk
	var atk_name := _country_name(atk)
	var def_name := _country_name(df)
	var feed := _feed_battle_verdict(res_region) if was_battle and (me == atk or me == df) else {}
	var head_txt := ""
	var head_tone := 0.5
	if not feed.is_empty():
		# le fil porte le verdict TRANCHÉ par le moteur (8 gagnée · 9 perdue · 11 indécise)
		var fk := int(feed.get("kind", 11))
		head_txt = "Bataille gagnée" if fk == 8 else ("Bataille perdue" if fk == 9 else "Bataille indécise")
		head_tone = 0.85 if fk == 8 else (0.10 if fk == 9 else 0.5)
	elif was_battle:
		head_txt = "Bataille conclue"
	else:
		# siège pur : le verdict honnête est la DISPOSITION de la place, pas un mot de gloire
		if me == atk:
			head_txt = "Siège conclu — région prise" if attacker_took else "Siège conclu — sans la place"
			head_tone = 0.85 if attacker_took else 0.20
		elif me == df:
			head_txt = "Siège conclu — place perdue" if attacker_took else "Siège conclu — place tenue"
			head_tone = 0.15 if attacker_took else 0.85
		else:
			head_txt = "Siège conclu"
	var head := _line(head_txt, "RowLabel")
	head.add_theme_font_size_override("font_size", 15)
	head.add_theme_color_override("font_color", VKit.sense(head_tone))
	_body.add_child(head)
	_body.add_child(_line("%s vs %s · %s" % [atk_name, def_name,
		("%s tient la région." % atk_name) if attacker_took else ("la région reste à %s." % def_name)], "RowDim"))
	if not feed.is_empty():
		# pertes CONFIRMÉES par le fil (v = nôtres | ennemies<<16, en paquets de 100)
		var packed := int(feed.get("v", 0))
		var ours := (packed & 0xffff) * 100
		var theirs := ((packed >> 16) & 0xffff) * 100
		_body.add_child(_stat_line("Nos pertes", "%s hommes" % _grp(ours), 0.15))
		_body.add_child(_stat_line("Pertes ennemies", "%s hommes" % _grp(theirs), 0.15))
	else:
		var loss_atk := int(float(bi.get("loss_atk", 0.0)) * 100.0)
		var loss_def := int(float(bi.get("loss_def", 0.0)) * 100.0)
		if loss_atk > 0 or loss_def > 0:
			_body.add_child(_stat_line("Pertes %s" % atk_name, "%s hommes" % _grp(loss_atk), 0.15))
			_body.add_child(_stat_line("Pertes %s" % def_name, "%s hommes" % _grp(loss_def), 0.15))
		else:
			_body.add_child(_stat_line("Pertes", "aucun choc confirmé (siège seul)"))

# ── VERBES (logique CONSERVÉE à l'identique — port de présentation) ────────────
func _flash_msg(msg: String, good: bool) -> void:
	_flash = msg
	_flash_good = good
	_flash_ms = Time.get_ticks_msec()
	_refresh()

func _do_refill() -> void:
	if Sim.world == null or not Sim.world.has_method("player_refill_corps"): return
	var ok := false
	for i in range(_selected_ids.size()):
		var allowed := true
		if i < _refill_previews.size(): allowed = bool(_refill_previews[i].get("allowed", false))
		if not allowed: continue
		ok = Sim.world.player_refill_corps(_selected_ids[i]) or ok
	if ok and _refill_previews.is_empty():
		_flash_msg("Recomplètement ordonné.", true) # ancienne DLL debug
	else:
		var totals := _refill_totals(_refill_previews)
		_flash_msg("Vague ordonnée · jusqu'à +%s hommes (%s garantis avant imports)." % [
			_grp(int(totals.requested)), _grp(int(totals.guaranteed))] if ok else "Rien à renforcer.", ok)

func _do_raise() -> void:
	if Sim.world == null or not Sim.world.has_method("player_raise_corps"): return
	var me := int(Sim.world.player())
	var reserve := int(Sim.world.country_army(me).get("regiments", 0))
	var capital := int(Sim.world.country_capital_region(me)) if Sim.world.has_method("country_capital_region") else -1
	var packets := maxi(1, int(reserve / 2))
	var ok: bool = reserve > 0 and capital >= 0 and Sim.world.player_raise_corps(packets, capital)
	_flash_msg("Nouveau corps levé à la capitale." if ok else "Réserve insuffisante.", ok)

func _do_disband() -> void:
	if not _disband_armed:
		_disband_armed = true
		_disband_ms = Time.get_ticks_msec()
		_refresh()
		return
	_disband_armed = false
	if Sim.world != null and Sim.world.has_method("player_disband_corps"):
		var ok := false
		for id in _selected_ids: ok = Sim.world.player_disband_corps(id) or ok
		_flash_msg("Armée dissoute." if ok else "Aucune armée à dissoudre.", ok)
	else:
		_refresh()

func _do_split() -> void:
	if Sim.world == null or not Sim.world.has_method("player_split_corps"): return
	var ok := false
	for id in _selected_ids:
		var a: Dictionary = Sim.world.corps_info(id)
		# La membrane expose des HOMMES ; la commande moteur attend des paquets de 100.
		var humans := int(a.get("units", 0))
		if not bool(a.get("units_are_humans", false)): humans *= 100
		var half := _split_packets(humans)
		if half > 0: ok = Sim.world.player_split_corps(id, half) or ok
	_flash_msg("Scission ordonnée." if ok else "Scission impossible.", ok)

func _do_merge() -> void:
	if Sim.world == null or not Sim.world.has_method("player_merge_corps") or _selected_ids.size() < 2:
		_flash_msg("Sélectionnez au moins deux corps au même endroit.", false); return
	var dst := _selected_ids[0]
	var ok := false
	for i in range(1, _selected_ids.size()): ok = Sim.world.player_merge_corps(dst, _selected_ids[i]) or ok
	_flash_msg("Fusion ordonnée · le corps #%d conserve son identité." % dst if ok else "Les corps doivent être dans la même région.", ok)
	if ok:
		selection_replaced.emit([dst])

# ── APERÇUS (données pures — logique CONSERVÉE à l'identique) ──────────────────
func _refresh_refill_data(w) -> void:
	_refill_previews.clear()
	if w == null or not w.has_method("corps_refill_preview"):
		return
	for id in _selected_ids:
		_refill_previews.append(w.corps_refill_preview(id))

func _refill_totals(previews: Array) -> Dictionary:
	var out := {"valid": 0, "allowed": 0, "requested": 0, "population": 0,
		"guaranteed_raw": 0, "guaranteed": 0, "weapons_needed": 0,
		"weapons_owned": 0, "reason": "Aucun corps sélectionné", "needs": {}}
	for raw in previews:
		var p: Dictionary = raw
		if not bool(p.get("valid", false)):
			continue
		out.valid += 1
		if bool(p.get("allowed", false)):
			out.allowed += 1
		else:
			if out.reason == "Aucun corps sélectionné": out.reason = String(p.get("reason", "Renfort indisponible"))
			continue
		out.requested += int(p.get("requested_humans", 0))
		out.population += int(p.get("population_ready_humans", 0))
		out.guaranteed_raw += int(p.get("guaranteed_humans", 0))
		out.weapons_needed += int(p.get("weapons_needed", 0))
		for raw_need in p.get("needs", []):
			var need: Dictionary = raw_need
			var key := str(int(need.get("resource", -1)))
			if not out.needs.has(key):
				out.needs[key] = {"name": String(need.get("name", "Armes")), "needed": 0, "owned": 0}
			var agg: Dictionary = out.needs[key]
			agg.needed += int(need.get("needed", 0))
			agg.owned = maxi(int(agg.owned), int(need.get("owned", 0)))
			out.needs[key] = agg
	var weapon_cover := 0
	for key in out.needs:
		var agg: Dictionary = out.needs[key]
		weapon_cover += mini(int(agg.needed), int(agg.owned))
	out.weapons_owned = weapon_cover
	var fortune_humans := maxi(0, int(out.requested) - int(out.weapons_needed))
	out.guaranteed = mini(int(out.guaranteed_raw), mini(int(out.population), fortune_humans + weapon_cover))
	if int(out.allowed) > 0: out.reason = ""
	return out

func _refill_summary_text(previews: Array) -> String:
	var t := _refill_totals(previews)
	if int(t.allowed) <= 0:
		return "Renfort indisponible · %s" % String(t.reason)
	var text := "Renfort · jusqu'à +%s hommes · %s garantis par vos stocks" % [
		_grp(int(t.requested)), _grp(int(t.guaranteed))]
	text += "\nCoût de la vague : %s hommes mobilisables · %s armes (%s nationales)" % [
		_grp(int(t.population)), _grp(int(t.weapons_needed)), _grp(int(t.weapons_owned))]
	if int(t.guaranteed) < int(t.population):
		text += " · marché sollicité pour les armes manquantes"
	if int(t.population) < int(t.requested):
		text += " · certaines classes sont épuisées"
	return text

func _refill_tooltip(previews: Array) -> String:
	var t := _refill_totals(previews)
	if int(t.allowed) <= 0:
		return String(t.reason)
	var lines: Array[String] = ["Une vague ajoute au plus 100 hommes par type d'unité présent."]
	for key in t.needs:
		var need: Dictionary = t.needs[key]
		lines.append("%s : %s en arsenal / %s requis" % [String(need.name), _grp(int(need.owned)), _grp(int(need.needed))])
	if int(t.guaranteed) < int(t.population):
		lines.append("Le manque peut être acheté au marché au prix et au trésor du prochain drain.")
	return "\n".join(lines)

func _corps_status_text(a: Dictionary) -> String:
	var loc := String(a.get("location", ""))
	if loc == "": loc = "région %d" % int(a.get("region", -1))
	var phase := String(a.get("phase", "Inconnu"))
	var text := "Corps #%d · %s · %s · %s hommes" % [int(a.get("id", -1)), loc, phase, _grp(int(a.get("units", 0)))]
	var dest := String(a.get("destination", ""))
	if int(a.get("dest", -1)) >= 0:
		if dest == "": dest = "région %d" % int(a.get("dest", -1))
		text += " → %s" % dest
	var progress := int(a.get("progress_pct", -1))
	if progress >= 0:
		text += " · %d%% · %.0f j restants" % [progress, float(a.get("days_left", 0.0))]
	elif float(a.get("days_left", 0.0)) > 0.5:
		text += " · %.0f j" % float(a.get("days_left", 0.0))
	if int(a.get("broken_days", 0)) > 0:
		text += " · BRISÉ %d j" % int(a.get("broken_days", 0))
	if float(a.get("rally_days", 0.0)) > 0.5:
		text += " · ralliement %.0f j (%s hommes)" % [float(a.get("rally_days", 0.0)), _grp(int(a.get("rally_units", 0)))]
	return text

func _stack_summary_text(corps_data: Array[Dictionary], regions: Array[int], total: int) -> String:
	if corps_data.size() < 2:
		return ""
	if regions.size() != 1:
		return "Stack dispersé · %d corps dans %d régions · fusion impossible" % [corps_data.size(), regions.size()]
	for corps in corps_data:
		var phase_id := int(corps.get("phase_id", 0))
		if phase_id == 3 or phase_id >= 4:
			return "Stack · %d corps · %s hommes · fusion bloquée pendant bataille/mer" % [corps_data.size(), _grp(total)]
	return "Stack · %d corps · %s hommes · fusion → corps #%d (%s hommes)" % [
		corps_data.size(), _grp(total), int(corps_data[0].get("id", -1)), _grp(total)]

func _move_preview_text(preview: Dictionary) -> String:
	if preview.is_empty():
		return ""
	var count := int(preview.get("corps_count", 0))
	var target := String(preview.get("target_name", ""))
	if target == "": target = "région %d" % int(preview.get("target_region", -1))
	if not bool(preview.get("valid", false)):
		return "Impossible vers %s · %s (%d/%d corps bloqués)" % [target,
			String(preview.get("reason", "route refusée")), int(preview.get("invalid_count", count)), count]
	var text := "Aperçu · %d corps → %s · ~%.0f j · %s" % [count, target,
		float(preview.get("travel_days", 0.0)), String(preview.get("arrival", "Déplacement"))]
	var start := int(preview.get("units_start", 0))
	var loss := int(preview.get("attrition_loss", 0))
	if start > 0:
		var arrival := int(preview.get("units_arrival", maxi(0, start - loss)))
		var pct := int(preview.get("attrition_pct", round(100.0 * float(loss) / float(start))))
		text += "\nArrivée projetée · %s → %s hommes" % [_grp(start), _grp(arrival)]
		if loss > 0:
			text += " · −%s en marche (%d%%)" % [_grp(loss), pct]
		else:
			text += " · aucune perte de marche projetée"
		var worst10 := int(preview.get("worst_daily_pct10", 0))
		if worst10 > 0:
			text += " · pire terrain %.1f%%/j" % (float(worst10) / 10.0)
	return text

func _move_preview_tone() -> float:
	if not bool(_move_preview.get("valid", false)):
		return 0.15
	if int(_move_preview.get("attrition_pct", 0)) >= 10:
		return 0.20
	if int(_move_preview.get("attrition_pct", 0)) >= 3:
		return 0.48
	return 0.75

func _split_packets(humans: int) -> int:
	return maxi(0, humans / 200)

# ── LAYOUT — s'ancre en marge gauche, au-dessus de la barre basse ──────────────
func _layout() -> void:
	reset_size()
	var vp := get_viewport_rect().size
	var h: float = size.y
	position = Vector2(Frame.SIDEBAR_W + 14.0,
		maxf(Frame.TOPBAR_H + 12.0, vp.y - h - Frame.BOTTOMBAR_H - 12.0))

# ── PRIMITIVES (mêmes variations de theme que province_panel_v2/empire_window) ──
func _line(txt: String, variation: String) -> Label:
	var l := Label.new()
	l.theme_type_variation = variation
	l.text = txt
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.custom_minimum_size = Vector2(PW - 24.0, 0)
	return l

func _dim_line(txt: String) -> void:
	_body.add_child(_line(txt, "RowDim"))

func _tone_line(txt: String, tone: float) -> Label:
	var l := _line(txt, "RowLabel")
	l.add_theme_color_override("font_color", VKit.sense(tone))
	return l

func _grp(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var count := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		count += 1
		if count % 3 == 0 and i > 0: out = " " + out
	return ("-" if n < 0 else "") + out
