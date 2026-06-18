extends Control
## MapControls — barres habillées au-dessus de la carte : sélecteur de MODE
## (terrain · politique · régions · pays) en bas-gauche + ZOOM (in/out/fit) en
## bas-droite. Câblé à MapView (set_mode + zoom). Boutons IconButton (chrome+icône).

const IconButton = preload("res://ui/icon_button.gd")

# mode render_map → icône du pack
const MODES := [
	[0, "layer_terrain"],      # Terrain
	[1, "politics_law"],       # Politique
	[2, "settlement_cluster"], # Régions
	[3, "politics_crown"],     # Pays
]

var _map
var _mode := 0
var _mode_btns := []
var _mb: HBoxContainer
var _zb: HBoxContainer

func setup(map) -> void:
	_map = map

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # ne capte qu'au-dessus des boutons
	set_anchors_preset(Control.PRESET_FULL_RECT)

	_mb = HBoxContainer.new()
	_mb.add_theme_constant_override("separation", 4)
	add_child(_mb)
	for m in MODES:
		var b = IconButton.new()
		_mb.add_child(b)
		b.setup_icon(String(m[1]), 38)
		b.selected = (int(m[0]) == _mode)
		b.pressed.connect(_on_mode.bind(int(m[0])))
		_mode_btns.append(b)

	_zb = HBoxContainer.new()
	_zb.add_theme_constant_override("separation", 4)
	add_child(_zb)
	for c in [["map_zoom_in", "_zin"], ["map_zoom_out", "_zout"], ["map_fit_view", "_zfit"]]:
		var b = IconButton.new()
		_zb.add_child(b)
		b.setup_chrome(String(c[0]), 38)
		b.pressed.connect(Callable(self, String(c[1])))

	get_viewport().size_changed.connect(_reposition)
	_reposition.call_deferred()

func _reposition() -> void:
	var vp := get_viewport_rect().size
	_mb.position = Vector2(12, vp.y - _mb.size.y - 12)
	_zb.position = Vector2(vp.x - _zb.size.x - 12, vp.y - _zb.size.y - 12)

func _on_mode(m: int) -> void:
	_mode = m
	if _map != null:
		_map.set_mode(m)
	for i in range(_mode_btns.size()):
		_mode_btns[i].selected = (int(MODES[i][0]) == m)
		_mode_btns[i].queue_redraw()

func _zin() -> void:  if _map != null: _map.zoom_in()
func _zout() -> void: if _map != null: _map.zoom_out()
func _zfit() -> void: if _map != null: _map.fit()
