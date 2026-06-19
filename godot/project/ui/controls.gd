extends Control
## MapControls — le BANDEAU BAS, pleine largeur (cadre d'écran). Porte le sélecteur
## de MODE (terrain · politique · régions · pays) à gauche et le ZOOM (in/out/fit) à
## droite. La barre capte ses clics (la carte dessous n'est pas sélectionnée). Câblé
## à MapView. Suit la largeur de la fenêtre (size_changed).

const VKit = preload("res://ui/vkit.gd")
const IconButton = preload("res://ui/icon_button.gd")
const Frame = preload("res://ui/frame.gd")
const H := Frame.BOTTOMBAR_H
const BTN := 38.0

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
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics

	_mb = HBoxContainer.new()
	_mb.add_theme_constant_override("separation", 4)
	add_child(_mb)
	for m in MODES:
		var b = IconButton.new()
		_mb.add_child(b)
		b.setup_icon(String(m[1]), BTN)
		b.selected = (int(m[0]) == _mode)
		b.pressed.connect(_on_mode.bind(int(m[0])))
		_mode_btns.append(b)

	_zb = HBoxContainer.new()
	_zb.add_theme_constant_override("separation", 4)
	add_child(_zb)
	for c in [["map_zoom_in", "_zin"], ["map_zoom_out", "_zout"], ["map_fit_view", "_zfit"]]:
		var b = IconButton.new()
		_zb.add_child(b)
		b.setup_chrome(String(c[0]), BTN)
		b.pressed.connect(Callable(self, String(c[1])))

	get_viewport().size_changed.connect(_resize)
	_resize.call_deferred()

func _resize() -> void:
	var vp := get_viewport_rect().size
	position = Vector2(0, vp.y - H)
	size = Vector2(vp.x, H)
	var by := (H - BTN) * 0.5                    # centrage vertical des boutons
	_mb.position = Vector2(Frame.SIDEBAR_W + 10, by)   # à droite du rail
	_zb.position = Vector2(vp.x - _zb.size.x - 12, by)
	queue_redraw()

func _draw() -> void:
	# barre PLEINE LARGEUR : navy + liseré cuivre en haut
	VKit.fill(self, Rect2(0, 0, size.x, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, 0, size.x, 2), VKit.COL_COPPER)

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
