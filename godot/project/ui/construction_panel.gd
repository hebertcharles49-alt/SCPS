extends PanelContainer
## ConstructionPanel — le menu de bâti en DEUX ONGLETS (Édifices | Manufactures), bâti
## avec des CONTENEURS Godot NATIFS (PanelContainer/VBox/HBox/Grid) + le THEME parchemin
## PARTAGÉ (parch_theme.gd), MÊME squelette que province_panel_v2/empire_window
## (HeaderStrip + LedTabStrip + corps ScrollContainer). ZÉRO `_draw`.
##
## LA VÉRITÉ ABSOLUE (retour joueur 2026-07-14) : une CARTE par bâtiment — icône + TITRE +
## prix·durée à droite / ligne d'effet / rangée d'icônes de ressources ×qty / « Prochain
## palier » (edifice_succ, affiché MÊME verrouillé — un bâtiment tech-verrouillé n'est
## JAMAIS listé comme posable, seulement en tag sur la carte de son palier courant) /
## raison de verrou en rouge. Le CONTENU et les gates sont INCHANGÉS : ce fichier est un
## port de présentation du pilote `_draw` précédent, pas une refonte de logique.

const ParchTheme = preload("res://ui/parch_theme.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

signal build_requested(kind: String, type: int)

const PW := 440.0

var target_pid := -1       ## la PROVINCE visée (posée par main à l'ouverture) — les manufactures y vivent
var _builds := []
var _bytype := {}          # type(int) → b(Dictionary) — pour résoudre le « Prochain palier » (edifice_succ)
var _blegal := {}          # type → {legal, reason} — miroir read-only du drain CMD_BUILD (lot M)
var _tab := 0              # 0 = Édifices · 1 = Manufactures

## LES DEUX ONGLETS, nommés UNE fois : la fiche province réutilise ces mots pour
## distinguer ses deux boutons « Construire… » (W2-7, rapport joueur F10 — ils étaient
## identiques à 60 px l'un de l'autre). Un seul vocabulaire pour une seule surface.
const TAB_NAMES := ["Édifices", "Manufactures"]
var _flash := ""           # retour de la dernière action (chantier mis / refus)
var _flash_ok := true

var _title_lbl: Label = null
var _tab_group: ButtonGroup = null
var _tab_btns: Array = []
var _scroll: ScrollContainer = null
var _body: VBoxContainer = null
var _flash_lbl: Label = null
var _fit_gen := 0          # jeton anti-course : seule la DERNIÈRE mesure différée s'applique

## carte cliquable qui porte SON dossier (get_info_card, lu par le TooltipServer natif
## au survol) — motif nested class de province_panel_v2 (BiomeTip/TerrainRow).
class InfoCard:
	extends PanelContainer
	var card_data: Dictionary = {}
	func get_info_card(_at_position: Vector2) -> Dictionary:
		return card_data.duplicate(true)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, 0)
	theme = ParchTheme.build()
	_build_shell()
	Sim.generated.connect(_refresh)
	Sim.month_ticked.connect(func(_y): _refresh())   # ressources dispo : cadence mensuelle
	get_viewport().size_changed.connect(_fit_scroll)
	if Sim.world != null:
		_refresh()

# ── LE SQUELETTE (header + onglets + corps déroulant) ─────────────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER : « Construction — <province visée> » + bouton fermer
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "Construction"
	_title_lbl.clip_text = true
	_title_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(_title_lbl)
	hb.add_child(_close_btn())

	# BARRE D'ONGLETS : Édifices | Manufactures
	var tabpanel := PanelContainer.new()
	tabpanel.theme_type_variation = "LedTabStrip"
	root.add_child(tabpanel)
	var tabs := HBoxContainer.new()
	tabs.add_theme_constant_override("separation", 2)
	tabpanel.add_child(tabs)
	_tab_group = ButtonGroup.new()
	_tab_btns.clear()
	var names := TAB_NAMES
	for i in range(names.size()):
		var b := Button.new()
		b.theme_type_variation = "Tab"
		b.toggle_mode = true
		b.button_group = _tab_group
		b.text = names[i]
		b.focus_mode = Control.FOCUS_NONE
		if i == _tab:
			b.button_pressed = true
		var idx := i
		b.pressed.connect(func(): _tab = idx; _refresh())
		tabs.add_child(b)
		_tab_btns.append(b)

	# CORPS (fond transparent, laisse voir le parchemin) : la liste des cartes DÉFILE
	# (molette native) sous une hauteur bornée au viewport — jamais une fenêtre qui
	# déborde de l'écran.
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	root.add_child(bodypanel)
	_scroll = ScrollContainer.new()
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	bodypanel.add_child(_scroll)
	_body = VBoxContainer.new()
	_body.add_theme_constant_override("separation", 8)
	_body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.add_child(_body)

	# retour transitoire (persistant jusqu'à la prochaine action) — TOUJOURS visible,
	# hors du défilement.
	_flash_lbl = Label.new()
	_flash_lbl.theme_type_variation = "Income"
	_flash_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_flash_lbl.visible = false
	root.add_child(_flash_lbl)

## bouton fermer, au thème parchemin (motif _sq_btn de province_panel_v2).
func _close_btn() -> Button:
	var b := Button.new()
	b.text = "✕"
	b.focus_mode = Control.FOCUS_NONE
	b.custom_minimum_size = Vector2(24, 24)
	b.add_theme_font_size_override("font_size", 13)
	b.add_theme_stylebox_override("normal", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("hover", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("pressed", ParchTheme.sb(ParchTheme.DIVIDER, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_color_override("font_color", ParchTheme.INK)
	b.add_theme_color_override("font_hover_color", ParchTheme.INK)
	b.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	b.pressed.connect(func():
		visible = false
		Sound.play("ui_parchment_close"))
	return b

## la fenêtre déroulante HUGGE le contenu, bornée au viewport (moins les barres pleine
## largeur) — le surplus défile, la fenêtre ne déborde jamais de l'écran. La hauteur
## mini du contenu n'est fiable qu'après un passage de layout (les cartes viennent
## d'être ajoutées) : deux frames de grâce, jeton anti-course pour ignorer une mesure
## périmée si `_refresh()` est rappelé entre-temps (changement d'onglet rapide).
func _fit_scroll() -> void:
	if _scroll == null or _body == null:
		return
	_fit_gen += 1
	var gen := _fit_gen
	await get_tree().process_frame
	await get_tree().process_frame
	if gen != _fit_gen or _scroll == null or not is_instance_valid(_scroll):
		return
	var vp := get_viewport_rect().size
	var hmax := clampf(vp.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 40.0, 200.0, 760.0)
	var want := clampf(_body.get_combined_minimum_size().y, 0.0, hmax)
	_scroll.custom_minimum_size = Vector2(0, want)
	reset_size.call_deferred()

## ouvre le panneau directement sur un onglet (0 Édifices · 1 Manufactures) — appelé
## depuis la fiche province (bouton « Construire… »).
func open_on(tab: int) -> void:
	_tab = clampi(tab, 0, 1)
	if _tab < _tab_btns.size():
		_tab_btns[_tab].button_pressed = true
	visible = true
	_refresh()

# ── RAFRAÎCHISSEMENT ────────────────────────────────────────────────────────────
func _refresh() -> void:
	if Sim.world == null or _body == null:
		return
	var me: int = Sim.world.player()
	_builds = Sim.world.building_roster(me)
	_bytype.clear()
	for b in _builds:
		_bytype[int(b.get("type", -1))] = b
	# lot M — la LÉGALITÉ réelle (or/matière/palier, miroir du drain qui refusait en
	# silence) : rafraîchie au tick, consommée pour griser/router le clic honnêtement.
	_blegal.clear()
	if Sim.world.has_method("build_legal"):
		for b in _builds:
			if bool(b.get("debloque", false)):
				var t := int(b.get("type", -1))
				_blegal[t] = Sim.world.build_legal(-1, t)
	_update_header()
	for c in _body.get_children():
		c.queue_free()
	match _tab:
		1: _build_manufactures()
		_: _build_edifices()
	_update_flash()
	_fit_scroll()

func _update_header() -> void:
	if target_pid >= 0 and Sim.world.has_method("province_info"):
		var info: Dictionary = Sim.world.province_info(target_pid)
		if bool(info.get("valide", false)):
			_title_lbl.text = "Construction — %s" % String(info.get("nom", "?"))
			return
	_title_lbl.text = "Construction"

func _update_flash() -> void:
	if _flash_lbl == null:
		return
	_flash_lbl.visible = _flash != ""
	_flash_lbl.text = _flash
	_flash_lbl.theme_type_variation = "Income" if _flash_ok else "Expense"

## la raison du refus, en mot (reason de build_legal : 2 couronnes · 3 matière · 4 tech de palier · 1 structurel)
func _reason_word(reason: int) -> String:
	match reason:
		2: return "Nécessite : plus d'or"
		3: return "Nécessite : matière"
		4: return "Nécessite : tech de palier"
		_: return "indisponible ici (palier/déjà bâti)"

## style Civ/AoE (2026-07-25) : toujours _reason_word — le reason_label C (phrase) est ignoré.
func _reason_label(result: Dictionary) -> String:
	return _reason_word(int(result.get("reason", 1)))

## dossier complet d'un édifice — consommé par le TooltipServer (get_info_card) ET par
## le test unitaire build_info_card_test (signature conservée telle quelle).
func _build_info_card(b: Dictionary, legal: Dictionary) -> Dictionary:
	var allowed := bool(legal.get("allowed", legal.get("legal", true)))
	var me: int = Sim.world.player()
	var ci: Dictionary = Sim.world.country_info(me)
	var gold_have := int(floor(float(ci.get("or", 0.0))))
	var gold_need := int(b.get("gold", 0))
	var lines := [{
		"label": "Or",
		"value": "coût %d · trésor %d" % [gold_need, gold_have],
	}]
	var stocks := {}
	if Sim.world.has_method("country_stocks"):
		for st in Sim.world.country_stocks(me):
			stocks[String(st.get("name", ""))] = int(st.get("stock", 0))
	for cost in b.get("cost", []):
		var name := String(cost.get("res", "Matière"))
		var need := _cost_qty_real(int(cost.get("qty", 0)))
		var have := int(stocks.get(name, 0))
		lines.append({
			"label": name,
			"value": "recette %d · stock national %d" % [need, have],
		})
	var effect := String(b.get("effet", ""))
	if effect != "":
		lines.append({"label": "Effet", "value": effect})
	# … et ce que CHACUN de ces mots veut dire : les mots de l'effet + les deux libellés
	# que la carte porte toujours (Or, Entretien). Rien de plus : une définition qui ne
	# répond à AUCUN mot affiché serait du bruit.
	_append_gloss(lines, [effect, "Or", "Entretien" if int(b.get("entretien", 0)) > 0 else ""])
	return {
		"title": String(b.get("nom", "Construction")),
		"state": "Constructible" if allowed else "Bloqué — %s" % _reason_label(legal),
		"trend": "%d jours" % int(b.get("days", 0)),
		"lines": lines,
		"body": "Cliquez la carte pour ordonner le chantier." if allowed else
			"Premier verrou opposé par le moteur : %s." % _reason_label(legal),
	}

## ── UI-1 (retour joueur 2026-09-04) : LES VERBES DE CONSTRUCTION SONT CODEXIFIÉS ──────
## « on voit +1 prospérité sur le port sans savoir ce que prospérité veut dire ; répéter
## le process pour chaque building. » Le hover d'une carte DIT désormais, en une phrase,
## ce que chaque mot d'effet signifie EN JEU. Les définitions viennent du GLOSSAIRE DU
## MOTEUR (scps_lang.c::G_GLOSSARY → façade `glossary()`), le même registre qui porte
## déjà Stabilité/Prospérité/Savoir : la ligne d'effet et sa définition sortent du MÊME
## STR_GLOSS_* (api_edifice_effet), donc un terme affiché ne peut pas manquer sa phrase.
## Aucun texte ne naît ici : ni le mot, ni la définition.
var _gloss := {}          ## mot MINUSCULE → {mot: le mot tel qu'écrit, def: la phrase}
var _gloss_read := false

func _gloss_table() -> Dictionary:
	if _gloss_read:
		return _gloss
	_gloss_read = true
	if Sim.world != null and Sim.world.has_method("glossary"):
		for g in Sim.world.glossary():
			var m := String(g.get("mot", ""))
			if m != "":
				_gloss[m.to_lower()] = {"mot": m, "def": String(g.get("def", ""))}
	return _gloss

## ajoute UNE ligne « mot → sa phrase » par terme trouvé dans `textes` (sans doublon,
## dans l'ordre du glossaire). `to_lower()` replie correctement les accents français —
## contrairement au flag (?i) des RegEx de Godot (piège vérifié, cf. concepts.gd::D4).
func _append_gloss(lines: Array, textes: Array) -> void:
	var hay := " " + " ".join(PackedStringArray(textes)).to_lower() + " "
	# la ponctuation devient de l'espace : on cherche des MOTS ENTIERS, jamais une
	# tranche (sans ça « Port » livrerait aussi la définition de « or »).
	for ch in ["·", ",", ";", ":", ".", "(", ")", "…", "—", "/", "%", "+", "×", "\n"]:
		hay = hay.replace(ch, " ")
	for key in _gloss_table().keys():
		if not hay.contains(" " + String(key) + " "):
			continue
		var e: Dictionary = _gloss[key]
		lines.append({"label": String(e["mot"]), "value": String(e["def"]), "tone": "dim"})

## la RECETTE réelle d'une manufacture, en mots : « Laine ×1.5 (ou Coton) → Étoffe ×2.8 ».
func _recipe_text(rec: Dictionary) -> String:
	var in1 := String(rec.get("in1", ""))
	if in1 == "":
		return "hors-sol (aucun intrant de tuile)"
	var s := "%s ×%s" % [in1, _fmt1(rec.get("q1", 0.0))]
	var in2 := String(rec.get("in2", ""))
	if in2 != "":
		s += " + %s ×%s" % [in2, _fmt1(rec.get("q2", 0.0))]
	var alt1 := String(rec.get("alt1", ""))
	if alt1 != "" and alt1 != in1:
		s += " (ou %s)" % alt1
	var out := String(rec.get("out", ""))
	if out != "":
		s += " → %s ×%s" % [out, _fmt1(rec.get("qout", 0.0))]
	return s

## un nombre à 1 décimale, sans zéro inutile (1.0 → "1", 2.8 → "2.8").
func _fmt1(v) -> String:
	var f := float(v)
	return ("%d" % int(round(f))) if absf(f - round(f)) < 0.05 else ("%.1f" % f)

## AUDIT D5 (2026-07-18) — le multiplicateur d'ÉTENDUE : `scps_building_roster`
## (scps_api.c) rapporte la recette NUE (d->cost.qty[k], sans le facteur), mais le drain
## réel (agency_build_acct, scps_agency.c:411-416 : `mult = agency_extent_mult(...)`,
## consomme `c->qty[k]*mult`) et le gate de légalité (scps_build_legal_ex, scps_api.c:
## 2454-2461 : `ext = 1.f + 0.15f*nreg`) appliquent TOUS DEUX ce facteur — seul le
## nombre affiché sur la puce l'omettait (l'or affiché, lui, l'inclut déjà via
## agency_build_gold). Rejoué ICI depuis `country_info().regions` (même compte que
## agency_extent_mult : régions possédées par le joueur) plutôt que d'ajouter un lecteur
## C, pour rester dans le seul fichier de ce chantier (D5).
func _extent_mult() -> float:
	var me: int = Sim.world.player()
	var nreg := int(Sim.world.country_info(me).get("regions", 0))
	return 1.0 + 0.15 * float(nreg)

## la quantité RÉELLEMENT débitée d'une ligne de recette (miroir de `c->qty[k]*mult`
## consommé par agency_build_acct) — arrondie comme le moteur (+0.5).
func _cost_qty_real(qty_base: int) -> int:
	return int(round(float(qty_base) * _extent_mult()))

# ── ONGLET ÉDIFICES : une CARTE par bâtiment ──────────────────────────────────
## Un édifice verrouillé par la tech N'EST JAMAIS listé comme posable : il n'apparaît
## qu'en tag « Prochain palier » sur la carte de son palier COURANT.
func _build_edifices() -> void:
	var w = Sim.world
	if w == null:
		return
	# VÉTUSTÉ (2026-07-25) : le bâti usé (≤95 %) propose sa RÉNOVATION en tête de liste.
	if target_pid >= 0 and w.has_method("renover_state"):
		var rs: Dictionary = w.renover_state(target_pid)
		if int(rs.get("wear_pct", 100)) <= 95:
			_body.add_child(_renover_card(rs))
	var any := false
	for b in _builds:
		if int(b.get("prev", -1)) >= 0 and not bool(b.get("prev_built", false)):
			continue   # palier hors de portée : son précédent n'existe pas encore chez nous
		if not bool(b.get("debloque", false)):
			continue   # verrouillé par la tech : surfacé en tag sur son prédécesseur, pas ici
		any = true
		_body.add_child(_edifice_card(w, b))
	if not any:
		_dim_line("aucun édifice constructible pour l'instant")

## carte : icône + titre / prix·durée · effet · ressources (icônes ×qty) · prochain
## palier · raison de verrou. Le CLIC (carte entière, si affordable) ordonne le chantier.
## VÉTUSTÉ — la carte « Rénover le bâti » : re-paye le bâti usé (50 % du coût de
## reconstruction, 180 j) → le moteur re-pose le delta plein (CMD_RENOVER, revalidé drain).
## Même gabarit InfoCard que les édifices ; grisée avec « Nécessite : plus d'or » si refus.
func _renover_card(rs: Dictionary) -> Control:
	var allowed := bool(rs.get("allowed", false))
	var gold := int(rs.get("gold", 0))
	var wear := int(rs.get("wear_pct", 100))
	var card := InfoCard.new()
	card.mouse_filter = Control.MOUSE_FILTER_STOP
	var bg := ParchTheme.HEADER_BG if allowed else ParchTheme.PANEL_BG
	var bd := ParchTheme.BORDER if allowed else ParchTheme.DIVIDER
	card.add_theme_stylebox_override("panel", ParchTheme.sb(bg, bd, 1, 4, 10, 10, 8, 8))
	var rlines := [{"label": "Bâti", "value": "%d %%" % wear}, {"label": "Or", "value": str(gold)}]
	_append_gloss(rlines, ["Or"])   # UI-1 : la rénovation aussi dit ses mots
	card.card_data = {
		"title": "Rénover le bâti",
		"state": "Disponible" if allowed else "Bloqué",
		"trend": "180 jours",
		"lines": rlines,
		"body": "Re-pose l'effet plein de chaque édifice de la province.",
	}
	if allowed:
		card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
		card.gui_input.connect(func(e):
			if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
				var ok: bool = Sim.world.player_renover(target_pid)
				_flash_ok = ok
				_flash = "⚒ Rénovation — ordre émis" if ok else "✗ Rénovation — file pleine"
				build_requested.emit("renover", 0)
				_refresh()
				Sim.notify_action())
	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 4)
	card.add_child(vb)
	var row0 := HBoxContainer.new()
	row0.add_theme_constant_override("separation", 8)
	row0.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vb.add_child(row0)
	_icon(row0, UIKit.building_sprite(0), 34)
	var title := Label.new()
	title.theme_type_variation = "RowLabel"
	title.text = "Rénover le bâti — %d %%" % wear
	title.add_theme_color_override("font_color", ParchTheme.INK if allowed else ParchTheme.DIM_INK)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row0.add_child(title)
	var price := Label.new()
	price.theme_type_variation = "RowDim"
	price.text = "%d · 180 j" % gold
	price.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	row0.add_child(price)
	if not allowed and int(rs.get("reason", 0)) == 2:
		var rl := Label.new()
		rl.theme_type_variation = "Expense"
		rl.text = "✗ Nécessite : plus d'or"
		rl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(rl)
	return card

func _edifice_card(w, b: Dictionary) -> Control:
	var btype := int(b.get("type", -1))
	var leg: Dictionary = _blegal.get(btype, {})
	var affordable: bool = bool(leg.get("legal", true))
	var nom := String(b.get("nom", ""))

	var card := InfoCard.new()
	card.mouse_filter = Control.MOUSE_FILTER_STOP
	var bg := ParchTheme.HEADER_BG if affordable else ParchTheme.PANEL_BG
	var bd := ParchTheme.BORDER if affordable else ParchTheme.DIVIDER
	card.add_theme_stylebox_override("panel", ParchTheme.sb(bg, bd, 1, 4, 10, 10, 8, 8))
	card.card_data = _build_info_card(b, leg)
	if affordable:
		card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
		card.gui_input.connect(func(e):
			if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
				_act("build", btype, nom))

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 4)
	card.add_child(vb)

	# L1 — icône · titre (gauche) · prix + durée (droite)
	var row0 := HBoxContainer.new()
	row0.add_theme_constant_override("separation", 8)
	row0.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vb.add_child(row0)
	_icon(row0, UIKit.building_sprite(btype), 34)
	var title := Label.new()
	title.theme_type_variation = "RowLabel"
	title.text = nom
	title.add_theme_color_override("font_color", ParchTheme.INK if affordable else ParchTheme.DIM_INK)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.clip_text = true
	row0.add_child(title)
	var price := Label.new()
	price.theme_type_variation = "RowDim"
	price.text = "%d · %d j" % [int(b.get("gold", 0)), int(b.get("days", 0))]
	price.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	row0.add_child(price)

	# L2 — RENDEMENT (l'effet RÉEL, delta ProvBuild — la membrane, pas une promesse)
	var eff := String(b.get("effet", ""))
	if eff != "":
		var effl := Label.new()
		effl.theme_type_variation = "RowDim"
		effl.text = eff
		effl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		effl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(effl)

	# L2bis — ENTRETIEN (retour joueur 2026-07-14 : le moteur le prélève chaque mois, la
	# carte doit le dire — miroir EXACT E1bis.10, valeur réelle, jamais le calcul).
	# UI-POLISH #7 : le « ~ » était un artefact GDScript pur — la valeur elle-même est
	# DÉJÀ un miroir exact (arrondi au gold près, scps_edifice_upkeep_month) ; le joueur
	# veut le prix réel, pas un signe d'approximation qui n'a jamais existé côté moteur.
	var upk := int(b.get("entretien", 0))
	if upk > 0:
		var upkl := Label.new()
		upkl.theme_type_variation = "RowDim"
		upkl.text = "Entretien : %d/mois" % upk
		upkl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(upkl)

	# L3 — RESSOURCES (la recette, en icônes)
	var cost: Array = b.get("cost", [])
	if cost.is_empty():
		var sl := Label.new()
		sl.theme_type_variation = "RowDim"
		sl.text = "structurel"
		sl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(sl)
	else:
		var flow := HFlowContainer.new()
		flow.add_theme_constant_override("h_separation", 12)
		flow.add_theme_constant_override("v_separation", 3)
		flow.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(flow)
		for c in cost:
			_cost_chip(flow, String(c.get("res", "")), _cost_qty_real(int(c.get("qty", 0))))

	# L4 — PROCHAIN PALIER (edifice_succ), affiché MÊME s'il est verrouillé par la tech
	var succ := int(w.edifice_succ(btype)) if w.has_method("edifice_succ") else -1
	var succ_b: Dictionary = _bytype.get(succ, {})
	if not succ_b.is_empty():
		var slocked := not bool(succ_b.get("debloque", false))
		var nl := Label.new()
		nl.theme_type_variation = "RowDim"
		nl.text = "Déverrouille : %s%s" % [String(succ_b.get("nom", "")), " (verrou tech)" if slocked else ""]
		if not slocked:
			nl.add_theme_color_override("font_color", ParchTheme.GREEN)
		nl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(nl)

	# L5 — raison de verrou, en rouge
	if not affordable:
		var rl := Label.new()
		rl.theme_type_variation = "Expense"
		rl.text = "✗ %s" % _reason_label(leg)
		rl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(rl)

	return card

## une puce ressource : icône (si dispo) + « Nom ×qty ».
func _cost_chip(into: Container, name: String, qty: int) -> void:
	var chip := HBoxContainer.new()
	chip.add_theme_constant_override("separation", 3)
	chip.mouse_filter = Control.MOUSE_FILTER_IGNORE
	into.add_child(chip)
	_icon(chip, UIKit.resource_icon(name), 18)
	var lb := Label.new()
	lb.theme_type_variation = "RowLabel"
	lb.text = "%s ×%d" % [name, qty]
	chip.add_child(lb)

# ── ONGLET MANUFACTURES — sur la province visée (target_pid, RE-KEY : pid direct) ──
## CARTE : icône + titre + prix (L1) · la RECETTE réelle intrants → produit (L2, chantier
## « vérité absolue » — matcher manuf_recipe(bld), plus une phrase d'ambiance).
func _build_manufactures() -> void:
	var w = Sim.world
	if w == null:
		return
	if target_pid < 0:
		_dim_line("sélectionnez une de vos provinces")
		return
	if not w.has_method("manuf_legal"):
		return
	var mcost: int = int(w.manuf_cost()) if w.has_method("manuf_cost") else 0
	var any := false
	for bld in range(24):   # BLD_TYPE_COUNT (miroir display-only, motif province_detail)
		if int(w.manuf_legal(target_pid, bld)) != 1:
			continue
		any = true
		var mnom := String(w.manuf_name(bld))
		var rec: Dictionary = w.manuf_recipe(bld) if w.has_method("manuf_recipe") else {}
		var upk := int(w.manuf_upkeep_month(target_pid, bld)) if w.has_method("manuf_upkeep_month") else 0
		_body.add_child(_manuf_card(bld, mnom, _recipe_text(rec), mcost, upk))
	if not any:
		_dim_line("aucune manufacture posable ici (intrants/tech)")

func _manuf_card(bld: int, nom: String, recipe_txt: String, mcost: int, upkeep: int) -> Control:
	var card := InfoCard.new()
	card.mouse_filter = Control.MOUSE_FILTER_STOP
	card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
	card.add_theme_stylebox_override("panel", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 4, 10, 10, 8, 8))
	var info_lines := [{"label": "Recette", "value": recipe_txt}]
	if upkeep > 0:
		info_lines.append({"label": "Entretien", "value": "%d couronnes/mois" % upkeep})
	# UI-1 : « répéter le process pour chaque building » — les manufactures aussi.
	_append_gloss(info_lines, ["Recette", "Entretien" if upkeep > 0 else "", "Or"])
	card.card_data = {
		"title": nom,
		"state": "Constructible",
		"trend": ("%d couronnes (chantier)" % mcost) if mcost > 0 else "coût au drain",
		"lines": info_lines,
		"body": "Cliquez la carte pour ordonner le chantier.",
	}
	card.gui_input.connect(func(e):
		if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
			_act("manuf", bld, nom))

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 4)
	card.add_child(vb)

	var row0 := HBoxContainer.new()
	row0.add_theme_constant_override("separation", 8)
	row0.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vb.add_child(row0)
	_icon(row0, UIKit.manuf_sprite(nom), 32)
	var title := Label.new()
	title.theme_type_variation = "RowLabel"
	title.text = nom
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.clip_text = true
	row0.add_child(title)
	if mcost > 0:
		var price := Label.new()
		price.theme_type_variation = "RowDim"
		price.text = "%d" % mcost
		price.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		row0.add_child(price)

	var rl := Label.new()
	rl.theme_type_variation = "RowDim"
	rl.text = recipe_txt
	rl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	rl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vb.add_child(rl)

	# ENTRETIEN (retour joueur 2026-07-14 : le moteur le prélève chaque mois, la carte
	# doit le dire — miroir EXACT E1bis.10, au niveau de naissance ici, pas encore bâtie).
	if upkeep > 0:
		var upkl := Label.new()
		upkl.theme_type_variation = "RowDim"
		upkl.text = "Entretien : %d/mois" % upkeep
		upkl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(upkl)
	return card

# ── LE CLIC agit : on appelle l'actionneur joueur (façade) et on affiche le retour ──
func _act(kind: String, type: int, nom: String) -> void:
	if Sim.world == null:
		return
	# Les ordres sont ENFILÉS (journal déterministe) : ils s'appliquent au prochain
	# tick (après agency_advance). En pause, l'ordre attend la reprise. Le retour
	# n'est donc que « mis en file », pas le verdict d'application (qui tombe au tick).
	# lot M — le drain refuse en SILENCE (or/matière) : on ne dit « ordre émis » que
	# si build_legal passe AU MOMENT DU CLIC ; sinon on nomme le refus.
	if kind == "build":
		if Sim.world.has_method("build_legal"):
			var bl: Dictionary = Sim.world.build_legal(-1, type)
			if not bool(bl.get("legal", true)):
				_flash_ok = false
				_flash = "✗ %s — %s" % [nom, _reason_label(bl)]
				Sound.play("ui_click")
				_refresh()
				return
		var ok: bool = Sim.world.player_build(type, -1)
		_flash_ok = ok
		_flash = ("⚒ %s — ordre émis" % nom) if ok else ("✗ %s — file pleine" % nom)
	else:
		var okm: bool = target_pid >= 0 and bool(Sim.world.player_build_manuf(target_pid, type))
		_flash_ok = okm
		_flash = ("⚒ %s — chantier ordonné" % nom) if okm else ("✗ %s — refusé" % nom)
	if not _flash_ok:
		Sound.play("ui_click")
	build_requested.emit(kind, type)
	_refresh()
	Sim.notify_action()   # verbe joueur (bâtir) → refresh des chiffres au drain (live)

# ── PRIMITIVES DE LAYOUT ──────────────────────────────────────────────────────
func _dim_line(txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = txt
	_body.add_child(l)

## une petite icône du pack (cadrée au slot ; rien si absente) — motif province_panel_v2.
func _icon(into: Container, tex: Texture2D, sz := 18) -> void:
	if tex == null:
		return
	var tr := TextureRect.new()
	tr.texture = tex
	tr.custom_minimum_size = Vector2(sz, sz)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	into.add_child(tr)
