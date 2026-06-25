extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène et
## relier la sélection de carte aux panneaux de lecture (la membrane → UI).

const Frame = preload("res://ui/frame.gd")

var _prov_panel: Control
var _country_panel: Control
var _sidebar: Control
var _construct: Control
var _tech: Control
var _econ: Control
var _prov_detail: Control
var _sel_prov := -1

func _ready() -> void:
	# la carte (Node2D, caméra dedans)
	var map_script := load("res://map/map_view.gd")
	var map: Node2D = map_script.new()
	map.name = "MapView"
	add_child(map)

	# l'UI, sur une couche écran (au-dessus de la carte, indépendante de la caméra)
	var ui := CanvasLayer.new()
	ui.name = "UI"
	add_child(ui)

	var topbar_script := load("res://ui/topbar.gd")
	var topbar: Control = topbar_script.new()
	topbar.name = "Topbar"
	ui.add_child(topbar)

	_prov_panel = load("res://ui/province_panel.gd").new()
	_prov_panel.name = "ProvincePanel"
	ui.add_child(_prov_panel)

	_country_panel = load("res://ui/country_panel.gd").new()
	_country_panel.name = "CountryPanel"
	ui.add_child(_country_panel)

	# le DESTIN du monde (§27) : barre d'entropie + bandeau de fin, haut-centre
	var endgame = load("res://ui/endgame_banner.gd").new()
	endgame.name = "EndgameBanner"
	ui.add_child(endgame)

	# le RAIL de sidebar (onglets menu_*) + tiroir — gauche
	_sidebar = load("res://ui/sidebar.gd").new()
	_sidebar.name = "Sidebar"
	ui.add_child(_sidebar)
	_sidebar.setup(map)                       # le tiroir Filtres pilote la carte
	# tiroir ouvert ⇒ on cache le panneau de province (même bande, exclusifs)
	_sidebar.tab_selected.connect(func(i): if i >= 0: _prov_panel.show_province(-1))

	# barres de carte : sélecteur de mode (bas-gauche) + zoom (bas-droite)
	var controls = load("res://ui/controls.gd").new()
	controls.name = "MapControls"
	controls.setup(map)
	ui.add_child(controls)

	# CONSTRUCTION : les boutons de levée & de bâti (touche B) — lit la façade
	# (roster 22 unités + édifices, prix réels). Caché par défaut, bascule au clavier.
	_construct = load("res://ui/construction_panel.gd").new()
	_construct.name = "ConstructionPanel"
	_construct.position = Vector2(Frame.SIDEBAR_W + 8, Frame.TOPBAR_H + 8)
	_construct.visible = false
	ui.add_child(_construct)

	# ARBRE DE TECH (touche T) : l'arbre du joueur, rendu en GRAPHE (medusa) sur
	# une trame radiale (chart 2D). Lit tech_info/tech_nodes. Caché par défaut.
	_tech = load("res://ui/tech_panel.gd").new()
	_tech.name = "TechPanel"
	_tech.visible = false
	ui.add_child(_tech)

	# ÉCONOMIE DANS LE TEMPS (touche G) : graphes Easy Charts (pop · trésor ·
	# prospérité), historique accumulé an par an. Caché par défaut, read-only.
	_econ = load("res://ui/economy_panel.gd").new()
	_econ.name = "EconomyPanel"
	_econ.visible = false
	ui.add_child(_econ)
	# les COURBES sont DERRIÈRE le sous-menu Économie (sidebar) — pas affichées d'office
	_sidebar.charts_requested.connect(func():
		_econ.visible = not _econ.visible
		_econ.queue_redraw())

	# PROVINCE — DÉTAIL (touche V) : graphes Easy Charts des flux + camemberts +
	# classes de la province SÉLECTIONNÉE. Caché par défaut, read-only.
	_prov_detail = load("res://ui/province_detail.gd").new()
	_prov_detail.name = "ProvinceDetail"
	_prov_detail.visible = false
	ui.add_child(_prov_detail)

	# la carte SÉLECTIONNE → on remplit les panneaux (lecture seule de la membrane)
	map.province_picked.connect(_on_province_picked)

func _unhandled_input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo):
		return
	match e.keycode:
		KEY_B:
			if _construct != null:
				_construct.visible = not _construct.visible
				_construct.queue_redraw()
		KEY_T:
			if _tech != null:
				_tech.visible = not _tech.visible
				_tech.queue_redraw()
		KEY_G:
			if _econ != null:
				_econ.visible = not _econ.visible
				_econ.queue_redraw()
		KEY_V:
			if _prov_detail != null:
				_prov_detail.show_province(_sel_prov)
				_prov_detail.visible = not _prov_detail.visible
				_prov_detail.queue_redraw()
		KEY_SPACE:                       # pause ↔ reprise (parité viewer.c)
			Sim.toggle_pause()
		KEY_EQUAL, KEY_PLUS, KEY_KP_ADD:        # « + » : accélérer
			Sim.faster()
		KEY_MINUS, KEY_KP_SUBTRACT:             # « - » : ralentir
			Sim.slower()

func _on_province_picked(province: int, _region: int, owner: int) -> void:
	if Sim.world == null:
		return
	if province < 0:
		_prov_panel.show_province(-1)         # clic en mer → on referme
		_country_panel.show_country(-1)
		return
	if _sidebar != null:
		_sidebar.close()                      # un clic province referme le tiroir (exclusifs)
	_sel_prov = province                      # mémorisé pour le détail (touche V)
	_prov_panel.show_province(province)
	_country_panel.show_country(owner)        # -1 (terre libre) → panneau caché
	if _prov_detail != null and _prov_detail.visible:
		_prov_detail.show_province(province)  # détail ouvert → suit la sélection
