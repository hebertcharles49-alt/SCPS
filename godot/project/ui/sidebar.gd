extends Control
## Sidebar — le RAIL d'onglets de viewer.c, habillé : 8 onglets menu_* (économie ·
## démographie · stocks · marché · armée · filtres · diplomatie · conseil) en bande
## gauche, sélectionnables. Le TIROIR (les vrais panneaux éco/démo/… de viewer.c)
## viendra avec leur port ; ici on pose et on habille le rail.

const IconButton = preload("res://ui/icon_button.gd")
const RAIL_X := 4.0
const RAIL_Y := 102.0
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

var _btns := []
var _sel := -1
var _drawer

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 3)
	vb.position = Vector2(RAIL_X, RAIL_Y)
	add_child(vb)
	for i in range(TABS.size()):
		var b = IconButton.new()
		vb.add_child(b)
		b.setup_icon(String(TABS[i][0]), BTN)
		b.pad_frac = 0.16
		b.pressed.connect(_on_tab.bind(i))
		b.tooltip_text = String(TABS[i][1])
		_btns.append(b)
	_drawer = load("res://ui/sidebar_drawer.gd").new()
	_drawer.name = "SidebarDrawer"
	add_child(_drawer)

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
