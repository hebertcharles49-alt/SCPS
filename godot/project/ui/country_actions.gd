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
var _status: Label
var _opinion_bar: Rect2
var _sum_lbl: Label
var _cd_lbl: Label
var _flash: Label

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: _refresh())

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
	_head = Label.new()
	_head.add_theme_font_size_override("font_size", 18)
	_head.add_theme_color_override("font_color", Color(0.86, 0.70, 0.42))
	hrow.add_child(_head)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hrow.add_child(sp)
	var closeb := Button.new()
	closeb.text = "✕"
	closeb.pressed.connect(func(): visible = false)
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

	var grid := GridContainer.new()
	grid.columns = 3
	grid.add_theme_constant_override("h_separation", 6)
	grid.add_theme_constant_override("v_separation", 6)
	col.add_child(grid)
	for v in [["war", "Guerre"], ["peace", "Paix"], ["ally", "Allier"], ["pact", "Pacte"], ["embargo", "Embargo"]]:
		var b := Button.new()
		b.text = v[1]
		var verb: String = v[0]
		b.pressed.connect(func(): _act(verb))
		grid.add_child(b)
		_btns[verb] = b

	_flash = Label.new()
	_flash.add_theme_color_override("font_color", Color(0.46, 0.74, 0.42))
	col.add_child(_flash)
	_layout()

func _layout() -> void:
	var vp := get_viewport_rect().size
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_panel.custom_minimum_size = Vector2(clampf(vp.x * 0.24, PW, 520.0), 0)
	_panel.position = Vector2((vp.x - _panel.custom_minimum_size.x) * 0.5, vp.y * 0.20)

func open_country(cid: int) -> void:
	if Sim.world == null or cid < 0 or cid == int(Sim.world.player()):
		return
	_cid = cid
	visible = true
	_flash.text = ""
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
		"embargo": bool(op2.get("can_embargo", false)) or bool(op2.get("can_lift_embargo", false)),
	}
	var would := {
		"ally": bool(op2.get("would_accept_alliance", true)),
		"pact": bool(op2.get("would_accept_pact", true)),
		"peace": bool(op2.get("would_accept_peace", true)),
	}
	for verb in _btns:
		var b: Button = _btns[verb]
		b.disabled = cd > 0 or not bool(can.get(verb, false))
		# AMBRE : permis mais l'offre serait REFUSÉE (l'opinion #26 prévisualisée)
		var amber: bool = (not b.disabled) and would.has(verb) and not bool(would[verb])
		b.modulate = Color(1.0, 0.82, 0.5) if amber else Color(1, 1, 1)
		b.tooltip_text = "il refusera (opinion trop basse)" if amber else ""

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
		"embargo": ok = bool(w.player_embargo(_cid, 1))
	_flash.text = "Ordre émis — l'émissaire part (verdict au drain)." if ok else "Ordre refusé."
	_refresh()
