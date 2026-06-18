extends PanelContainer
## Topbar — année · pop · pays · vitesse. Un PanelContainer auto-dimensionné
## (il prend la taille de son HBox). Lit Sim, n'écrit RIEN dans la sim (sauf le
## verbe vitesse, qui ne touche que la cadence d'affichage). UI bâtie en code.

var _info: Label
var _speed: Button

func _ready() -> void:
	position = Vector2(8, 8)

	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 14)
	add_child(hb)

	_speed = Button.new()
	_speed.focus_mode = Control.FOCUS_NONE
	_speed.pressed.connect(_on_speed)
	hb.add_child(_speed)

	_info = Label.new()
	hb.add_child(_info)

	Sim.generated.connect(_refresh)
	Sim.ticked.connect(_on_ticked)
	_refresh()

func _on_ticked(_year: int) -> void:
	_refresh()

func _on_speed() -> void:
	Sim.cycle_speed()
	_refresh()

func _refresh() -> void:
	if Sim.world == null:
		_info.text = "(libscps absente — voir README)"
		_speed.text = "—"
		return
	# DLL antérieure à la Phase 2 (sans les readouts) → le clic ne peut pas marcher.
	if not Sim.world.has_method("province_at"):
		_info.text = "⚠ libscps OBSOLÈTE (Phase 1) — rebâtir : cd godot && scons platform=windows use_mingw=yes"
		_info.add_theme_color_override("font_color", Color(0.95, 0.55, 0.25))
		_speed.text = "—"
		return
	_info.remove_theme_color_override("font_color")
	_info.text = "An %d · pop %d · pays %d" % [
		Sim.world.year(), Sim.world.world_pop(), Sim.world.country_count()]
	_speed.text = Sim.speed_label()
