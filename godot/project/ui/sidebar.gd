extends Control
## Sidebar — le RAIL gauche PLEINE HAUTEUR (cadre d'écran), entre le bandeau haut et
## le bandeau bas : 8 onglets menu_* (économie · démographie · stocks · marché ·
## armée · filtres · diplomatie · conseil). Fond Panel navy + liseré cuivre (capte
## ses clics). Le TIROIR (panneaux éco/démo/…) sort à droite du rail. Suit la hauteur.

const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")
const IconButton = preload("res://ui/icon_button.gd")
const BTN := 38.0

const TABS := [
	["menu_economy",   "Économie"],
	["menu_demography","Démographie"],
	["menu_stocks",    "Stocks"],
	["menu_market",    "Marché"],
	["menu_army",      "Armée"],
	["menu_filters",   "Filtres"],
	["menu_diplomacy", "Diplomatie"],
	["menu_council",   "Conseil"],
]

signal tab_selected(index: int)   ## -1 = aucun (replié)
signal charts_requested           ## le tiroir Économie demande les courbes (sous-menu)

var _btns := []
var _sel := -1
var _drawer
var _map
var _rail: Panel
var _vb: VBoxContainer

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)

	# rail de fond PLEINE HAUTEUR : Panel navy + liseré cuivre à droite (capte ses clics)
	_rail = Panel.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = VKit.COL_PANEL
	sb.border_color = VKit.COL_COPPER
	sb.set_border_width(SIDE_RIGHT, 2)
	_rail.add_theme_stylebox_override("panel", sb)
	add_child(_rail)

	# colonne d'onglets, centrée sur le rail
	_vb = VBoxContainer.new()
	_vb.add_theme_constant_override("separation", 4)
	add_child(_vb)
	for i in range(TABS.size()):
		var b = IconButton.new()
		_vb.add_child(b)
		b.setup_icon(String(TABS[i][0]), BTN)
		b.pad_frac = 0.16
		b.pressed.connect(_on_tab.bind(i))
		b.tooltip_text = String(TABS[i][1])
		_btns.append(b)

	_drawer = load("res://ui/sidebar_drawer.gd").new()
	_drawer.name = "SidebarDrawer"
	add_child(_drawer)
	_drawer.charts_requested.connect(func(): charts_requested.emit())
	if _map != null:
		_drawer.setup(_map)

	get_viewport().size_changed.connect(_resize)
	_resize.call_deferred()

func _resize() -> void:
	var vp := get_viewport_rect().size
	_rail.position = Vector2(0, Frame.TOPBAR_H)
	_rail.size = Vector2(Frame.SIDEBAR_W, maxf(40.0, vp.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H))
	_vb.position = Vector2((Frame.SIDEBAR_W - BTN) * 0.5, Frame.TOPBAR_H + 8.0)

## la carte (pour le tiroir Filtres → set_mode)
func setup(map) -> void:
	_map = map
	if _drawer != null:
		_drawer.setup(map)

func _on_tab(i: int) -> void:
	_sel = -1 if _sel == i else i      # re-cliquer un onglet ouvert le replie
	for k in range(_btns.size()):
		_btns[k].selected = (k == _sel)
		_btns[k].queue_redraw()
	_drawer.show_tab(_sel)
	tab_selected.emit(_sel)

## referme le tiroir (p.ex. quand on sélectionne une province)
func close() -> void:
	if _sel != -1:
		_on_tab(_sel)   # re-cliquer l'onglet courant → repli
