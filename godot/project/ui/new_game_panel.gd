extends Control
## NewGamePanel — l'écran « Nouvelle partie » : sliders MONDE (taille tiny→huge, âge,
## continents, relief, climat) + liste des EMPIRES (chacun composable façon Stellaris :
## slot 0 = Vous, 1..N = IA) + graine + « Lancer la partie ».
##
## RÈGLE D'OR : zéro logique de sim. On LIT/ÉCRIT la façade C (Sim.world.*) : worldgen_set
## (les sliders), set_empire_culture (les compos), puis Sim.regenerate. Le monde naît en PAUSE.

signal launched   ## la partie est lancée (le monde régénéré, en pause an 0)
signal back       ## retour au menu principal

const Creator = preload("res://ui/culture_creator.gd")

# palette
const C_BG    := Color(0.04, 0.03, 0.02, 0.92)
const C_PANEL := Color(0.10, 0.08, 0.055, 0.98)   # cuir sombre
const C_EDGE  := Color(0.79, 0.64, 0.29)          # or vieilli
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.62, 0.60, 0.58)
const C_TITLE := Color(0.86, 0.70, 0.42)

# presets de TAILLE : clé tr() (i18n/ui.csv) → [n_empires, n_city_states]
const SIZES := [
	["T_SIZE_0", 2, 4],
	["T_SIZE_1", 4, 8],
	["T_SIZE_2", 6, 12],
	["T_SIZE_3", 8, 16],
	["T_SIZE_4", 10, 20],
	["T_SIZE_5", 12, 24],
]
# clés tr() — les noms rendus suivent la langue choisie aux Options
const HERITAGE_KEYS := ["T_HERITAGE_0", "T_HERITAGE_1", "T_HERITAGE_2", "T_HERITAGE_3", "T_HERITAGE_4", "T_HERITAGE_5"]
const ETHOS_KEYS := ["T_ETHOS_0", "T_ETHOS_1", "T_ETHOS_2", "T_ETHOS_3", "T_ETHOS_4", "T_ETHOS_5"]

var _size_opt: OptionButton
var _seed_spin: SpinBox
var _sliders := {}          # clé → HSlider
var _slider_vals := {}      # clé → Label (valeur)
var _empire_box: VBoxContainer
var _empire_rows := []      # [{summary:Label, btn:Button}]
var _compos := {}           # slot:int → {heritage,ethos,t0,t1,t2}
var _creator: Control = null


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()
	_rebuild_empire_list()

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(880, 600)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(2)
	sb.set_corner_radius_all(6); sb.set_content_margin_all(18)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	panel.add_child(root)

	var title := Label.new()
	title.text = tr("T_NG_TITLE")
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", C_TITLE)
	root.add_child(title)

	# deux colonnes : MONDE | EMPIRES
	var cols := HBoxContainer.new()
	cols.add_theme_constant_override("separation", 24)
	cols.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(cols)

	# ── colonne MONDE ──
	var world := VBoxContainer.new()
	world.add_theme_constant_override("separation", 6)
	world.custom_minimum_size = Vector2(400, 0)
	cols.add_child(world)
	world.add_child(_section(tr("T_NG_WORLD")))

	var size_row := HBoxContainer.new()
	var size_lab := Label.new(); size_lab.text = tr("T_NG_SIZE"); size_lab.custom_minimum_size = Vector2(150, 0)
	size_lab.add_theme_color_override("font_color", C_TEXT)
	size_row.add_child(size_lab)
	_size_opt = OptionButton.new()
	for s in SIZES:
		_size_opt.add_item(tr(String(s[0])))
	_size_opt.selected = 2   # Normal
	_size_opt.item_selected.connect(func(_i): _rebuild_empire_list())
	_size_opt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_row.add_child(_size_opt)
	world.add_child(size_row)

	# sliders 0..1 (clé worldgen, clé tr())
	var SL := [
		["world_age", "T_NG_AGE"],
		["land_amount", "T_NG_LAND"],
		["mountains", "T_NG_MOUNTAINS"],
		["erosion", "T_NG_EROSION"],
		["temperature", "T_NG_TEMP"],
		["humidity", "T_NG_HUMID"],
	]
	for e in SL:
		world.add_child(_make_slider(String(e[0]), tr(String(e[1])), 0.0, 1.0, 0.5, 0.05))
	# continents 1..8 (entier)
	world.add_child(_make_slider("n_continents", tr("T_NG_CONTINENTS"), 1.0, 8.0, 6.0, 1.0))

	var seed_row := HBoxContainer.new()
	var seed_lab := Label.new(); seed_lab.text = tr("T_NG_SEED"); seed_lab.custom_minimum_size = Vector2(150, 0)
	seed_lab.add_theme_color_override("font_color", C_TEXT)
	seed_row.add_child(seed_lab)
	_seed_spin = SpinBox.new(); _seed_spin.min_value = 0; _seed_spin.max_value = 999999; _seed_spin.value = 9
	_seed_spin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	seed_row.add_child(_seed_spin)
	world.add_child(seed_row)

	# pré-remplir les sliders avec les défauts moteur (worldparams_default)
	_load_defaults()

	# ── colonne EMPIRES ──
	var emp := VBoxContainer.new()
	emp.add_theme_constant_override("separation", 6)
	emp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cols.add_child(emp)
	emp.add_child(_section(tr("T_NG_EMPIRES")))
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.custom_minimum_size = Vector2(380, 380)
	emp.add_child(scroll)
	_empire_box = VBoxContainer.new()
	_empire_box.add_theme_constant_override("separation", 4)
	_empire_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(_empire_box)

	# ── pied : actions ──
	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	root.add_child(foot)
	var back_btn := Button.new(); back_btn.text = tr("T_BACK")
	back_btn.pressed.connect(func(): back.emit())
	foot.add_child(back_btn)
	var spacer := Control.new(); spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(spacer)
	var go := Button.new(); go.text = tr("T_NG_LAUNCH")
	go.add_theme_font_size_override("font_size", 16)
	go.pressed.connect(_on_lancer)
	foot.add_child(go)


func _section(txt: String) -> Label:
	var l := Label.new(); l.text = txt
	l.add_theme_color_override("font_color", C_EDGE)
	return l

func _make_slider(key: String, label: String, lo: float, hi: float, val: float, step: float) -> Control:
	var row := HBoxContainer.new()
	var lab := Label.new(); lab.text = label; lab.custom_minimum_size = Vector2(150, 0)
	lab.add_theme_color_override("font_color", C_TEXT)
	row.add_child(lab)
	var sld := HSlider.new()
	sld.min_value = lo; sld.max_value = hi; sld.step = step; sld.value = val
	sld.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sld.custom_minimum_size = Vector2(160, 0)
	row.add_child(sld)
	var vlab := Label.new(); vlab.custom_minimum_size = Vector2(46, 0)
	vlab.add_theme_color_override("font_color", C_DIM)
	row.add_child(vlab)
	var is_int := step >= 1.0
	var upd := func(v):
		vlab.text = (str(int(v)) if is_int else ("%.2f" % v))
	sld.value_changed.connect(upd)
	upd.call(val)
	_sliders[key] = sld
	_slider_vals[key] = vlab
	return row


func _load_defaults() -> void:
	if Sim.world == null or not Sim.world.has_method("worldparams_default"):
		return
	var d: Dictionary = Sim.world.worldparams_default(int(_seed_spin.value))
	for k in ["world_age", "land_amount", "mountains", "erosion", "temperature", "humidity"]:
		if _sliders.has(k):
			_sliders[k].value = float(d.get(k, 0.5))
	if _sliders.has("n_continents"):
		_sliders["n_continents"].value = float(int(d.get("n_continents", 6)))


func _cur_size() -> Array:
	var i: int = clampi(_size_opt.selected, 0, SIZES.size() - 1)
	return SIZES[i]

func _rebuild_empire_list() -> void:
	for c in _empire_box.get_children():
		c.queue_free()
	_empire_rows.clear()
	var n_emp: int = int(_cur_size()[1])
	for slot in range(n_emp):
		var row := PanelContainer.new()
		var rb := StyleBoxFlat.new()
		rb.bg_color = Color(0.16, 0.13, 0.09, 0.9); rb.set_corner_radius_all(4); rb.set_content_margin_all(6)
		row.add_theme_stylebox_override("panel", rb)
		var hb := HBoxContainer.new()
		row.add_child(hb)
		var name_lab := Label.new()
		name_lab.text = (tr("T_NG_EMPIRE_YOU") % (slot + 1)) if slot == 0 else (tr("T_NG_EMPIRE_AI") % (slot + 1))
		name_lab.add_theme_color_override("font_color", C_TITLE if slot == 0 else C_TEXT)
		name_lab.custom_minimum_size = Vector2(120, 0)
		hb.add_child(name_lab)
		var summary := Label.new()
		summary.add_theme_color_override("font_color", C_DIM)
		summary.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		hb.add_child(summary)
		var btn := Button.new(); btn.text = tr("T_NG_COMPOSE")
		var s := slot
		btn.pressed.connect(func(): _on_compose(s))
		hb.add_child(btn)
		_empire_box.add_child(row)
		_empire_rows.append({"summary": summary, "btn": btn})
		_refresh_row(slot)

func _refresh_row(slot: int) -> void:
	if slot < 0 or slot >= _empire_rows.size():
		return
	var summary: Label = _empire_rows[slot]["summary"]
	if _compos.has(slot):
		var c = _compos[slot]
		var h: int = int(c["heritage"]); var e: int = int(c["ethos"])
		summary.text = "%s · %s" % [tr(HERITAGE_KEYS[h]) if h < 6 else "?", tr(ETHOS_KEYS[e]) if e < 6 else "?"]
		summary.add_theme_color_override("font_color", C_TEXT)
	else:
		summary.text = tr("T_NG_RANDOM")
		summary.add_theme_color_override("font_color", C_DIM)


func _ensure_creator() -> void:
	if _creator != null:
		return
	_creator = Creator.new()
	_creator.name = "SlotCreator"
	add_child(_creator)
	_creator.hide()
	_creator.composed.connect(_on_composed)

func _on_compose(slot: int) -> void:
	if Sim.world == null:
		return
	_ensure_creator()
	_creator.open_for_slot(slot)

func _on_composed(slot: int, heritage: int, ethos: int, t0: int, t1: int, t2: int) -> void:
	_compos[slot] = {"heritage": heritage, "ethos": ethos, "t0": t0, "t1": t1, "t2": t2}
	_refresh_row(slot)


func _gather_params() -> Dictionary:
	var sz := _cur_size()
	var d := {
		"n_empires": int(sz[1]),
		"n_city_states": int(sz[2]),
		"n_continents": int(_sliders["n_continents"].value),
		"world_age": _sliders["world_age"].value,
		"land_amount": _sliders["land_amount"].value,
		"mountains": _sliders["mountains"].value,
		"erosion": _sliders["erosion"].value,
		"temperature": _sliders["temperature"].value,
		"humidity": _sliders["humidity"].value,
	}
	return d

func _on_lancer() -> void:
	if Sim.world == null:
		return
	Sim.world.clear_player_culture()              # repart d'une ardoise propre (tous slots)
	Sim.world.worldgen_set(_gather_params())      # les sliders pilotent la genèse
	for slot in _compos.keys():
		var c = _compos[slot]
		Sim.world.set_empire_culture(int(slot), int(c["heritage"]), int(c["ethos"]),
			int(c["t0"]), int(c["t1"]), int(c["t2"]))
	Sim.regenerate(int(_seed_spin.value))         # == chargement == → monde neuf
	Sim.set_speed(0)                              # Pause — année 0
	hide()
	launched.emit()
