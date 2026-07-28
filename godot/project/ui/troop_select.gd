extends PanelContainer
## TroopSelect — le panneau satellite de SÉLECTION DE TROUPES (clic sur le nom d'un corps,
## army_panel) : une ligne par type PRÉSENT (glyphe + dispo) avec un compteur en RÉGIMENTS
## (paquets de 100), puis « Scinder la sélection » → player_split_comp (composition EXACTE,
## le moteur refuse net si un type dépasse — jamais de clamp). Un corps à la fois.
## ParchTheme natif, widgets à état (SpinBox) persistants par ouverture — display-only.

const ParchTheme = preload("res://ui/parch_theme.gd")
const VKit = preload("res://ui/vkit.gd")

signal split_ordered(msg: String, good: bool)

const TYPES := [["■", "inf", "Infanterie", 0], ["▲", "arch", "Tirailleurs/archers", 1],
	["●", "cav", "Cavalerie", 3], ["▬", "mages", "Mages", 5]]

var _cid := -1
var _spins := {}          # clé type → SpinBox (état utilisateur : jamais rebâti pendant l'édition)
var _avail := {}          # clé type → régiments disponibles
var _total_lbl: Label = null
var _split_btn: Button = null

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	theme = ParchTheme.build()
	custom_minimum_size = Vector2(210, 0)
	visible = false

func open_for(cid: int) -> void:
	_cid = cid
	_build()
	visible = true

func _build() -> void:
	for c in get_children():
		c.queue_free()
	_spins.clear()
	_avail.clear()
	var w = Sim.world
	if w == null or not w.has_method("corps_info"):
		return
	var a: Dictionary = w.corps_info(_cid)
	if not bool(a.get("active", false)):
		visible = false
		return
	var scale := 1 if bool(a.get("units_are_humans", false)) else 100
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 4)
	add_child(root)

	var head := HBoxContainer.new()
	root.add_child(head)
	var title := Label.new()
	title.theme_type_variation = "Title"
	title.text = "Corps #%d — détacher" % _cid
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	head.add_child(title)
	var close := Button.new()
	close.text = "✕"
	close.flat = true
	close.focus_mode = Control.FOCUS_NONE
	close.pressed.connect(func(): visible = false)
	head.add_child(close)

	# une ligne par type présent : glyphe teinté · dispo · compteur en régiments
	for t in TYPES:
		var key := String(t[1])
		var regs := (int(a.get(key, 0)) * scale) / 100
		if regs <= 0: continue
		_avail[key] = regs
		var row := HBoxContainer.new()
		row.add_theme_constant_override("separation", 6)
		root.add_child(row)
		var g := Label.new()
		g.theme_type_variation = "RowLabel"
		g.text = "%s %d" % [t[0], regs]
		g.tooltip_text = "%s · %d régiment(s)" % [String(t[2]), regs]
		g.mouse_filter = Control.MOUSE_FILTER_STOP
		g.add_theme_color_override("font_color", VKit.SLICE_PAL[int(t[3])])
		g.custom_minimum_size = Vector2(64, 0)
		row.add_child(g)
		var sp := SpinBox.new()
		sp.min_value = 0
		sp.max_value = regs
		sp.step = 1
		sp.value = 0
		sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		sp.value_changed.connect(func(_v): _sync())
		row.add_child(sp)
		_spins[key] = sp

	_total_lbl = Label.new()
	_total_lbl.theme_type_variation = "RowDim"
	root.add_child(_total_lbl)

	_split_btn = Button.new()
	_split_btn.text = "Scinder la sélection"
	_split_btn.focus_mode = Control.FOCUS_NONE
	_split_btn.pressed.connect(_do_split)
	root.add_child(_split_btn)
	_sync()

## sélection courante en régiments par type
func _sel() -> Dictionary:
	var s := {}
	for key in _spins:
		s[key] = int((_spins[key] as SpinBox).value)
	return s

## total sélectionné + gate : ≥1 régiment ET il en reste ≥1 au corps d'origine
func _sync() -> void:
	if _total_lbl == null:
		return
	var s := _sel()
	var tot := 0
	var avail_tot := 0
	for key in _avail:
		tot += int(s.get(key, 0))
		avail_tot += int(_avail[key])
	_total_lbl.text = "Sélection : %d régiment(s) (%s hommes)" % [tot, str(tot * 100)]
	_split_btn.disabled = tot < 1 or tot >= avail_tot

func _do_split() -> void:
	if Sim.world == null or not Sim.world.has_method("player_split_comp"):
		split_ordered.emit("Scission indisponible (DLL antérieure).", false)
		return
	var s := _sel()
	var ok: bool = Sim.world.player_split_comp(_cid,
		int(s.get("inf", 0)), int(s.get("arch", 0)), int(s.get("cav", 0)), int(s.get("mages", 0)))
	split_ordered.emit("Détachement ordonné (au prochain tick)." if ok else "Scission refusée.", ok)
	if ok:
		visible = false
		Sim.notify_action()

func _unhandled_input(e: InputEvent) -> void:
	if visible and e is InputEventKey and e.pressed and e.keycode == KEY_ESCAPE:
		visible = false
		get_viewport().set_input_as_handled()
