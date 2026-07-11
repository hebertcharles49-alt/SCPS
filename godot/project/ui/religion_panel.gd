extends Control
## ReligionPanel — le CRÉATEUR DE FOI : s'ouvre quand le joueur bâtit son PREMIER édifice
## religieux (sanctuaire/temple…) — avant, le monde est ATHÉE. Le joueur compose un CRÉDO +
## 3 traditions (pôles) sur 3 axes DISTINCTS ; si le centre de sa foi est conquis (RUPTURE),
## un bouton SCHISME apparaît. Aussi rouvrable à la touche R. Une section LETTRÉ (P6)
## recrute le Missionnaire/Gourou/Moine du crédo (lot M). RÈGLE D'OR : zéro logique de
## sim — on LIT la façade (Sim.world.*) et on émet des verbes (religion_found / religion_schism
## / religion_recruit_scholar).

signal closed   ## le panneau se ferme → le jeu reprend (main.gd)

const VKit = preload("res://ui/vkit.gd")
const C_BG    := Color(0.02, 0.025, 0.025, 0.76)
const C_PANEL := VKit.COL_PANEL
const C_EDGE  := Color(0.55, 0.42, 0.78)        # liseré violet (religion)
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.62, 0.60, 0.58)
const C_GOOD  := Color(0.46, 0.74, 0.42)
const C_BAD   := Color(0.82, 0.40, 0.34)
const C_TITLE := Color(0.74, 0.62, 0.90)

var _poles: Array = []      # [{id,nom,axe,axe_nom,tip}]
var _credos: Array = []     # [{id,nom}]
var _credo_opt: OptionButton
var _trad_opt := [null, null, null]
var _trad_tip := [null, null, null]
var _valid_lbl: Label
var _state_lbl: Label
var _title_lbl: Label
var _found_btn: Button
var _schism_btn: Button
var _scholar_lbl: Label     # lot M — le LETTRÉ (P6) : état courant (rôle résolu)
var _scholar_btn: Button    # « Recruter » / « Renouveler », gaté par la foi + le crédo


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()
	_load_data()
	_refresh()

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(600, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(1)
	sb.set_border_width(SIDE_TOP, 3); sb.set_corner_radius_all(1); sb.set_content_margin_all(18)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	panel.add_child(col)

	_title_lbl = Label.new(); _title_lbl.text = "Créateur de foi"
	_title_lbl.add_theme_font_size_override("font_size", 24)
	_title_lbl.add_theme_color_override("font_color", C_TITLE)
	col.add_child(_title_lbl)

	_state_lbl = Label.new()
	_state_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_state_lbl.custom_minimum_size = Vector2(560, 0)
	_state_lbl.add_theme_color_override("font_color", C_DIM)
	col.add_child(_state_lbl)

	col.add_child(HSeparator.new())
	var cl := Label.new(); cl.text = "Crédo"; cl.add_theme_color_override("font_color", C_EDGE)
	col.add_child(cl)
	_credo_opt = OptionButton.new()
	_credo_opt.item_selected.connect(func(_i): _refresh())
	col.add_child(_credo_opt)

	var tl := Label.new(); tl.text = "Trois traditions (axes distincts)"
	tl.add_theme_color_override("font_color", C_EDGE)
	col.add_child(tl)
	for i in range(3):
		var opt := OptionButton.new()
		opt.item_selected.connect(func(_x): _refresh())
		col.add_child(opt)
		_trad_opt[i] = opt
		var tip := Label.new()
		tip.add_theme_color_override("font_color", C_DIM)
		tip.add_theme_font_size_override("font_size", 12)
		col.add_child(tip)
		_trad_tip[i] = tip

	_valid_lbl = Label.new()
	col.add_child(_valid_lbl)

	# lot M — le LETTRÉ (P6, Missionnaire/Gourou/Moine selon crédo) : état + recrutement.
	# Il agit à la CAPITALE (CONVERT répand · STABILIZE exempte du malus · RESIST bloque).
	col.add_child(HSeparator.new())
	var sl := Label.new(); sl.text = "Lettré"; sl.add_theme_color_override("font_color", C_EDGE)
	col.add_child(sl)
	var srow := HBoxContainer.new()
	srow.add_theme_constant_override("separation", 10)
	col.add_child(srow)
	_scholar_lbl = Label.new()
	_scholar_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scholar_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_scholar_lbl.add_theme_color_override("font_color", C_DIM)
	srow.add_child(_scholar_lbl)
	_scholar_btn = Button.new(); _scholar_btn.text = "Recruter"
	_scholar_btn.pressed.connect(_on_recruit_scholar)
	srow.add_child(_scholar_btn)

	col.add_child(HSeparator.new())
	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	col.add_child(foot)
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(func(): hide(); Sound.play("ui_parchment_close"); closed.emit())
	foot.add_child(close)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	_schism_btn = Button.new(); _schism_btn.text = "Schisme"
	_schism_btn.pressed.connect(_on_schism)
	foot.add_child(_schism_btn)
	_found_btn = Button.new(); _found_btn.text = "Fonder"
	_found_btn.pressed.connect(_on_found)
	foot.add_child(_found_btn)


func _load_data() -> void:
	if Sim.world == null or not Sim.world.has_method("religion_pole_list"):
		_valid_lbl.text = "Moteur religion absent (recompiler libscps)."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
		return
	_credos = Sim.world.credo_list()
	for c in _credos:
		_credo_opt.add_item(String(c["nom"]))
		_credo_opt.set_item_metadata(_credo_opt.item_count - 1, int(c["id"]))
	_credo_opt.select(0)
	_poles = Sim.world.religion_pole_list()
	for i in range(3):
		var opt: OptionButton = _trad_opt[i]
		for p in _poles:
			opt.add_item("%s  [%s]" % [String(p["nom"]), String(p["axe_nom"])])
			opt.set_item_metadata(opt.item_count - 1, int(p["id"]))
	# défaut : 3 axes distincts (pôles 0, 4, 10)
	var defaults := [0, 4, 10]
	for i in range(3):
		var opt: OptionButton = _trad_opt[i]
		for k in range(opt.item_count):
			if int(opt.get_item_metadata(k)) == defaults[i]:
				opt.select(k); break


func _cur_credo() -> int:
	return int(_credo_opt.get_item_metadata(_credo_opt.selected)) if _credo_opt.selected >= 0 else 0
func _cur_pole(i: int) -> int:
	var opt: OptionButton = _trad_opt[i]
	return int(opt.get_item_metadata(opt.selected)) if opt.selected >= 0 else -1
func _pole_tip(id: int) -> String:
	for p in _poles:
		if int(p["id"]) == id: return String(p["tip"])
	return ""


func _refresh() -> void:
	if Sim.world == null or _credo_opt.item_count == 0:
		return
	var me: int = Sim.world.player()
	var has: bool = (int(Sim.world.religion_of_country(me)) >= 0)
	if _title_lbl != null:
		_title_lbl.text = "Religion" if has else "Créateur de foi"
	# état courant
	if has:
		var elig := int(Sim.world.religion_eligible(me))
		var emot := "" if elig == 0 else (" — SCHISME possible (RUPTURE)" if elig == 1 else " — SCHISME possible (DÉRIVE)")
		_state_lbl.text = "Foi d'État : %s%s" % [String(Sim.world.religion_name(me)), emot]
		_schism_btn.disabled = (elig == 0)
		_found_btn.disabled = true
	else:
		var can := true
		if Sim.world.has_method("religion_can_found"):
			can = int(Sim.world.religion_can_found()) == 1
		if can:
			_state_lbl.text = "Aucune foi fondée. Composez un crédo + 3 traditions."
			_found_btn.text = "Fonder"
		else:
			_state_lbl.text = "Le monde a atteint son nombre de religions (⌈empires/3⌉) — vous RALLIEZ une foi existante."
			_found_btn.text = "Rallier une foi"
		_schism_btn.disabled = true
	for i in range(3):
		_trad_tip[i].text = _pole_tip(_cur_pole(i))
	# validité (axes distincts)
	var t0 := _cur_pole(0); var t1 := _cur_pole(1); var t2 := _cur_pole(2)
	var ok: bool = Sim.world.religion_picks_valid(t0, t1, t2)
	if not has:
		var can_create := true
		if Sim.world.has_method("religion_can_found"):
			can_create = int(Sim.world.religion_can_found()) == 1
		_found_btn.disabled = (not ok) if can_create else false   # rallier ne dépend pas des picks
	if ok:
		_valid_lbl.text = "✓ Trois axes distincts."
		_valid_lbl.add_theme_color_override("font_color", C_GOOD)
	else:
		_valid_lbl.text = "✗ Choisissez trois pôles sur trois axes DIFFÉRENTS."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
	_refresh_scholar(me, has)

## lot M — le LETTRÉ : rôle ACTIF (religion_scholar_role) vs rôle qu'un recrutement
## DONNERAIT (religion_scholar_expected, résolu du crédo). Sans foi ⇒ section grisée.
func _refresh_scholar(me: int, has: bool) -> void:
	if _scholar_lbl == null or _scholar_btn == null:
		return
	if not Sim.world.has_method("religion_scholar_expected"):
		_scholar_lbl.text = "(recompiler libscps — lecteur lettré absent)"
		_scholar_btn.disabled = true
		return
	if not has:
		_scholar_lbl.text = "Une foi d'État d'abord — le lettré la sert."
		_scholar_btn.disabled = true
		_scholar_btn.text = "Recruter"
		return
	var active := int(Sim.world.religion_scholar_role(me))
	var expected := int(Sim.world.religion_scholar_expected(me))
	if active >= 0:
		_scholar_lbl.text = "%s en mission (%s, ~5 ans), à la capitale." % [
			String(Sim.world.scholar_role_name(active)), String(Sim.world.scholar_role_ability(active))]
		_scholar_btn.text = "Renouveler"
		_scholar_btn.disabled = (expected < 0)
	elif expected >= 0:
		_scholar_lbl.text = "Aucun lettré en mission — votre crédo forme un %s (%s)." % [
			String(Sim.world.scholar_role_name(expected)), String(Sim.world.scholar_role_ability(expected))]
		_scholar_btn.text = "Recruter"
		_scholar_btn.disabled = false
	else:
		_scholar_lbl.text = "Votre crédo ne forme pas de lettré."
		_scholar_btn.disabled = true

## recrute/renouvelle le lettré À LA CAPITALE (la région où sa face agit).
func _on_recruit_scholar() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	var region: int = int(Sim.world.country_capital_region(me))
	if region < 0:
		return
	var role: int = int(Sim.world.religion_recruit_scholar(me, region))
	Sound.play("ui_click")
	if role >= 0:
		_refresh()


func _on_found() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	var rid: int = Sim.world.religion_found(me, _cur_credo(), _cur_pole(0), _cur_pole(1), _cur_pole(2))
	if rid >= 0:
		_refresh()

func _on_schism() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	# repick : on rejoue les 2 derniers slots avec les pôles choisis (un schisme « dérivé »)
	var res: Dictionary = Sim.world.religion_schism(me, 1, _cur_pole(1), 2, _cur_pole(2), _cur_credo())
	if int(res.get("child", -1)) >= 0:
		_refresh()

## ouvre le panneau (touche R en jeu).
func open() -> void:
	show()
	Sound.play("ui_parchment_open")
	_refresh()
	queue_redraw()
