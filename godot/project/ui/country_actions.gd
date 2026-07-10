extends Control
## COUNTRY ACTIONS — la fenêtre DIPLOMATIQUE d'un pays cible : le résumé exhaustif de la
## relation (statut · opinion ±100 · composantes · mémoire d'actes) + LES VERBES (guerre /
## paix / alliance / pacte / embargo), grisés par la légalité (diplo_options) ET par le
## DIPLOMATE (un émissaire, 1 acte / 2 mois — scps_diplo_cd). Ouverte par la liste diplo
## de la sidebar OU par le CLIC DROIT sur la carte. Zéro logique sim : lit la façade,
## enfile des verbes journalisés.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const DrawerK = preload("res://ui/sidebar_drawer.gd")   # DACT_LABEL partagé (mémoire datée)

const PW := 380.0

var _cid := -1
var _btns := {}          ## verbe → Button
var _panel: PanelContainer
var _head: Label
var _arms_rect: TextureRect   ## les ARMES du pays cible (héraldique dérivée)
var _status: Label
var _opinion_bar: Rect2
var _sum_lbl: Label
var _cd_lbl: Label
var _cb_lbl: Label   ## W-GUERRE-3 : état de l'intrigue fabriquée (en cours / prête / coût)
var _flash: Label

# UI-4 (retour joueur 2026-07-10) : hiérarchie d'actions — Guerre est DESTRUCTIF (rouge
# sombre + confirmation 2 clics, motif _servile_manumit_armed/province_panel _purge_armed) ;
# Paix/Allier/Pacte/Migration/Embargo restent SECONDAIRES (thème neutre inchangé).
const BTN_LABELS := {"war": "Guerre", "peace": "Paix", "ally": "Allier", "pact": "Pacte",
	"migration": "Migration", "embargo": "Embargo"}
var _war_armed := false
var _war_armed_ms := -100000
var _war_sb_idle: StyleBoxFlat
var _war_sb_hover: StyleBoxFlat
var _war_sb_press: StyleBoxFlat
var _war_sb_armed: StyleBoxFlat

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: _refresh())

## la fenêtre de confirmation « Guerre » (4 s) retombe même en pause (Sim.ticked ne
## tourne pas si le jeu est arrêté ; ce Control, si — miroir province_panel._process).
func _process(_dt: float) -> void:
	if _war_armed and Time.get_ticks_msec() - _war_armed_ms > 4000:
		_war_armed = false
		if visible:
			_refresh()

func _build() -> void:
	_panel = PanelContainer.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.10, 0.08, 0.055, 0.98)
	sb.border_color = Color(0.79, 0.64, 0.29)
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(14)
	_panel.add_theme_stylebox_override("panel", sb)
	add_child(_panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	_panel.add_child(col)

	var hrow := HBoxContainer.new()
	col.add_child(hrow)
	_arms_rect = TextureRect.new()
	_arms_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_arms_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_arms_rect.custom_minimum_size = Vector2(34, 34)
	hrow.add_child(_arms_rect)
	_head = Label.new()
	_head.add_theme_font_size_override("font_size", 18)
	_head.add_theme_color_override("font_color", Color(0.86, 0.70, 0.42))
	hrow.add_child(_head)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hrow.add_child(sp)
	var closeb := Button.new()
	closeb.text = "✕"
	closeb.pressed.connect(func():
		visible = false
		Sound.play("ui_parchment_close"))
	hrow.add_child(closeb)

	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
	col.add_child(_status)

	_sum_lbl = Label.new()
	_sum_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_sum_lbl.custom_minimum_size = Vector2(PW - 40.0, 0)
	_sum_lbl.add_theme_font_size_override("font_size", 12)
	_sum_lbl.add_theme_color_override("font_color", Color(0.72, 0.70, 0.66))
	col.add_child(_sum_lbl)

	_cd_lbl = Label.new()
	_cd_lbl.add_theme_color_override("font_color", Color(0.85, 0.65, 0.30))
	col.add_child(_cd_lbl)

	_cb_lbl = Label.new()
	_cb_lbl.add_theme_color_override("font_color", Color(0.70, 0.55, 0.75))
	col.add_child(_cb_lbl)

	# UI-4 : les styleboxes DESTRUCTIFS de « Guerre » (rouge sombre — distinct du chrome
	# cuir/or par défaut des verbes secondaires) — précalculés une fois.
	_war_sb_idle = _mkbox(Color(0.22, 0.06, 0.05), Color(0.58, 0.18, 0.13), 2)
	_war_sb_hover = _mkbox(Color(0.30, 0.09, 0.07), Color(0.80, 0.26, 0.18), 2)
	_war_sb_press = _mkbox(Color(0.14, 0.04, 0.03), Color(0.46, 0.13, 0.10), 2, true)
	_war_sb_armed = _mkbox(Color(0.48, 0.12, 0.09), Color(0.95, 0.36, 0.25), 2)

	# 6 verbes en grille 3×2 à largeur ÉGALE (le long libellé « Fabriquer… » déformait
	# la grille : Guerre géant, Paix minuscule — capture 2026-07-09) ; « Fabriquer une
	# revendication » vit sur SA ligne, pleine largeur.
	var grid := GridContainer.new()
	grid.columns = 3
	grid.add_theme_constant_override("h_separation", 6)
	grid.add_theme_constant_override("v_separation", 6)
	col.add_child(grid)
	for v in [["war", "Guerre"], ["peace", "Paix"], ["ally", "Allier"], ["pact", "Pacte"], ["migration", "Migration"], ["embargo", "Embargo"]]:
		var b := Button.new()
		b.text = v[1]
		b.custom_minimum_size = Vector2(104, 0)
		b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		var verb: String = v[0]
		if verb == "war":
			# DESTRUCTIF : rouge sombre + CONFIRMATION 2 clics (motif _servile_manumit_armed/
			# province_panel._purge_armed) — jamais exécuté directement au 1er clic.
			b.add_theme_stylebox_override("normal", _war_sb_idle)
			b.add_theme_stylebox_override("hover", _war_sb_hover)
			b.add_theme_stylebox_override("pressed", _war_sb_press)
			b.add_theme_color_override("font_color", Color(0.94, 0.82, 0.78))
			b.add_theme_color_override("font_hover_color", Color(1.0, 0.90, 0.86))
			b.pressed.connect(func(): _war_press())
		else:
			b.pressed.connect(func(): _act(verb))
		grid.add_child(b)
		_btns[verb] = b
	var fb := Button.new()
	fb.text = "Fabriquer une revendication"
	fb.pressed.connect(func(): _act("fabricate"))
	col.add_child(fb)
	_btns["fabricate"] = fb

	_flash = Label.new()
	_flash.add_theme_color_override("font_color", Color(0.46, 0.74, 0.42))
	col.add_child(_flash)
	_layout()

## petit StyleBoxFlat cuir/bordure (miroir ui_theme._box, dupliqué ici : country_actions
## n'a pas licence d'éditer ui_theme.gd, et une couleur DESTRUCTIVE n'a pas sa place dans
## le thème global neutre).
static func _mkbox(bg: Color, border: Color, bw: int = 2, shift_down := false) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = bg
	sb.border_color = border
	sb.set_border_width_all(bw)
	sb.set_corner_radius_all(3)
	sb.content_margin_left = 10.0
	sb.content_margin_right = 10.0
	sb.content_margin_top = 5.0 + (2.0 if shift_down else 0.0)
	sb.content_margin_bottom = 5.0 - (2.0 if shift_down else 0.0)
	return sb

## UI-4 : « Guerre » exige 2 clics — le 1er ARME la confirmation (rien n'est déclaré), le
## 2e (dans les 4 s, cf. _process) déclare pour de vrai. Jamais de popup modal.
func _war_press() -> void:
	if not _war_armed:
		_war_armed = true
		_war_armed_ms = Time.get_ticks_msec()
		_refresh()
		return
	_war_armed = false
	_act("war")

func _layout() -> void:
	var vp := get_viewport_rect().size
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_panel.custom_minimum_size = Vector2(clampf(vp.x * 0.24, PW, 520.0), 0)
	_panel.position = Vector2((vp.x - _panel.custom_minimum_size.x) * 0.5, vp.y * 0.20)

func open_country(cid: int) -> void:
	if Sim.world == null or cid < 0 or cid == int(Sim.world.player()):
		return
	# BROUILLARD : un pays jamais découvert ne se laisse pas approcher (retour joueur)
	if Sim.world.has_method("country_known") and int(Sim.world.country_known(cid)) == 0:
		return
	_cid = cid
	visible = true
	Sound.play("ui_parchment_open")
	_flash.text = ""
	if _arms_rect != null:
		_arms_rect.texture = load("res://ui/heraldry.gd").arms(cid)
	_refresh()
	_layout()

func _refresh() -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	_head.text = String(w.country_info(_cid).get("nom", "?"))
	# la relation vue du joueur : statut + opinion (ce que LUI pense de NOUS)
	var rel := {}
	for rl in w.country_relations(w.player()):
		if int(rl.get("country", -1)) == _cid:
			rel = rl
			break
	var op := int(rel.get("opinion", 0))
	_status.text = "Statut : %s · Opinion : %+d" % [String(rel.get("status", "—")), op]
	# le RÉSUMÉ : composantes d'opinion + les 3 derniers actes (la mémoire)
	var parts := PackedStringArray()
	if w.has_method("opinion_summary"):
		var ps: Dictionary = w.opinion_summary(_cid)
		for pk in [["Alliance", "ally"], ["Guerre", "war"], ["Vassalité", "vassal"],
			["Pacte", "pact"], ["Embargo", "embargo"], ["Rancune", "rancor"], ["Mémoire", "memory"]]:
			var v := int(ps.get(pk[1], 0))
			if v != 0:
				parts.append("%s %+d" % [pk[0], v])
	var mem := PackedStringArray()
	if w.has_method("diplo_journal"):
		var me2: int = int(w.player())
		var nj := 0
		for a in w.diplo_journal(_cid):
			if nj >= 3:
				break
			if not DrawerK.DACT_LABEL.has(int(a.get("act", -1))):
				continue
			var lab: Array = DrawerK.DACT_LABEL[int(a.get("act", -1))]
			var by_us: bool = int(a.get("a", -1)) == me2
			mem.append("an %d · %s" % [int(a.get("year", 0)), String(lab[1] if by_us else lab[0])])
			nj += 1
	_sum_lbl.text = ("Pourquoi : " + ", ".join(parts) + "\n" if parts.size() > 0 else "") \
		+ ("Mémoire : " + " — ".join(mem) if mem.size() > 0 else "Mémoire : aucun acte notable")
	# le DIPLOMATE : cooldown → tous les verbes grisés + la raison affichée
	var cd := int(w.diplo_cd()) if w.has_method("diplo_cd") else 0
	_cd_lbl.text = ("Émissaire en tournée — de retour dans %d j" % cd) if cd > 0 else "Émissaire disponible"
	var op2: Dictionary = w.diplo_options(_cid) if w.has_method("diplo_options") else {}
	var can := {
		"war": bool(op2.get("can_declare_war", false)),
		"peace": bool(op2.get("can_make_peace", false)),
		"ally": bool(op2.get("can_offer_alliance", false)),
		"pact": bool(op2.get("can_offer_pact", false)),
		"migration": bool(op2.get("can_offer_migration", false)),
		"embargo": bool(op2.get("can_embargo", false)) or bool(op2.get("can_lift_embargo", false)),
		"fabricate": bool(op2.get("can_fabricate", false)),
	}
	var would := {
		"ally": bool(op2.get("would_accept_alliance", true)),
		"pact": bool(op2.get("would_accept_pact", true)),
		"migration": bool(op2.get("would_accept_migration", true)),
		"peace": bool(op2.get("would_accept_peace", true)),
	}
	# état de relation (langue-indépendant, via opinion_summary) pour NOMMER la raison d'un verbe grisé
	var psr: Dictionary = w.opinion_summary(_cid) if w.has_method("opinion_summary") else {}
	var at_war: bool = int(psr.get("war", 0)) != 0
	var allied: bool = int(psr.get("ally", 0)) != 0
	var has_pact: bool = int(psr.get("pact", 0)) != 0
	for verb in _btns:
		if verb == "fabricate":
			continue   # géré à part plus bas (texte/état dynamiques)
		var b: Button = _btns[verb]
		b.disabled = cd > 0 or not bool(can.get(verb, false))
		if verb == "war" and b.disabled:
			_war_armed = false          # plus légal ⇒ la confirmation en attente retombe
		# AMBRE : permis mais l'offre serait REFUSÉE (l'opinion #26 prévisualisée)
		var amber: bool = (not b.disabled) and would.has(verb) and not bool(would[verb])
		# UI-5 (retour joueur : « la couleur seule ne suffit pas ») : l'ambre « il
		# refusera » ne se voyait qu'à la teinte du bouton (invisible avant le survol) —
		# un « ⚠ » sur le LIBELLÉ double le canal, visible sans survoler.
		var base_label: String = String(BTN_LABELS.get(verb, verb))
		if verb == "war":
			# DESTRUCTIF : le libellé PORTE la confirmation (« Confirmer la guerre ? »),
			# le fond bascule à un rouge plus vif tant que l'armement tient (4 s).
			b.text = "Confirmer la guerre ?" if _war_armed else "⚔ %s" % base_label
			b.add_theme_stylebox_override("normal", _war_sb_armed if _war_armed else _war_sb_idle)
			b.modulate = Color(1, 1, 1)
		else:
			b.text = ("%s ⚠" % base_label) if amber else base_label
			b.modulate = Color(1.0, 0.82, 0.5) if amber else Color(1, 1, 1)
		# RETOUR JOUEUR : chaque verbe GRISÉ nomme sa raison au survol (« pourquoi je peux pas ? »)
		if b.disabled:
			b.tooltip_text = _why_disabled(verb, cd, at_war, allied, has_pact)
		elif verb == "war" and _war_armed:
			b.tooltip_text = "irréversible — cliquez de nouveau pour confirmer (4 s)"
		elif amber:
			b.tooltip_text = "il refusera (opinion trop basse)"
		else:
			b.tooltip_text = ""
	# W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ : « Guerre » reste grisé sans motif gratuit NI
	# intrigue mûre (can_declare_war le dit déjà côté moteur) ; « Fabriquer » porte l'état
	# de l'intrigue en cours/mûre/coût — un bouton de CORRUPTION, distinct de la déclaration.
	var fabricating: bool = bool(op2.get("fabricating", false))
	var cb_ready: bool = bool(op2.get("cb_ready", false))
	var cost := float(op2.get("fabricate_cost", 0.0))
	var fab_btn: Button = _btns.get("fabricate")
	if fab_btn != null:
		if fabricating:
			var dleft := int(ceili(float(op2.get("fabricating_days_left", 0.0))))
			fab_btn.text = "Intrigue en cours (%d j)" % dleft
			fab_btn.disabled = true
		elif cb_ready:
			var yleft := float(op2.get("cb_ready_years_left", 0.0))
			fab_btn.text = "Revendication prête (expire dans %.1f an)" % yleft
			fab_btn.disabled = true   # rien à refaire tant qu'elle est valide — déclarez la guerre
		else:
			fab_btn.text = "Fabriquer une revendication (%d or)" % int(round(cost))
			fab_btn.disabled = cd > 0 or not bool(can.get("fabricate", false))
	if fabricating:
		_cb_lbl.text = "Une intrigue mûrit contre ce pays."
	elif cb_ready:
		_cb_lbl.text = "Une revendication est prête : déclarez la guerre avant qu'elle ne s'évente."
	else:
		_cb_lbl.text = ""

func _act(verb: String) -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var ok := false
	match verb:
		"war": ok = bool(w.player_declare_war(_cid))
		"peace": ok = bool(w.player_make_peace(_cid))
		"ally": ok = bool(w.player_offer_alliance(_cid))
		"pact": ok = bool(w.player_offer_pact(_cid))
		"migration": ok = bool(w.player_offer_migration(_cid))
		"embargo": ok = bool(w.player_embargo(_cid, 1))
		"fabricate": ok = bool(w.player_fabricate_cb(_cid))
	_flash.text = "Ordre émis — l'émissaire part (verdict au drain)." if ok else "Ordre refusé."
	if ok and verb == "war":
		Sound.play("moment_war_horn")
	elif not ok:
		Sound.play("ui_click")
	_refresh()

## la RAISON explicite d'un verbe GRISÉ (retour joueur : « chaque chose grisée doit être nommée »).
## Dérivée des flags diplo_options + de l'état de relation (opinion_summary), langue-indépendante.
func _why_disabled(verb: String, cd: int, at_war: bool, allied: bool, has_pact: bool) -> String:
	if cd > 0:
		return "émissaire en tournée — de retour dans %d j" % cd
	match verb:
		"war":
			return "déjà en guerre avec ce pays" if at_war else "trêve en cours (paix trop récente)"
		"peace":
			return "vous n'êtes pas en guerre avec ce pays"
		"ally":
			if at_war: return "impossible : vous êtes en guerre"
			if allied: return "vous êtes déjà alliés"
			return "aucun créneau d'alliance libre (de part ou d'autre)"
		"pact":
			if at_war: return "impossible : vous êtes en guerre"
			if has_pact: return "un pacte commercial existe déjà"
			return "pacte impossible pour l'instant"
		"migration":
			if at_war: return "impossible : vous êtes en guerre"
			return "un pacte migratoire existe déjà"
		"embargo":
			return "embargo indisponible pour l'instant"
	return "indisponible pour l'instant"
