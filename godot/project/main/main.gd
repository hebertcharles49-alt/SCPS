extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène et
## relier la sélection de carte aux panneaux de lecture (la membrane → UI).

var _prov_panel: PanelContainer
var _country_panel: PanelContainer

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

	# la carte SÉLECTIONNE → on remplit les panneaux (lecture seule de la membrane)
	map.province_picked.connect(_on_province_picked)

func _on_province_picked(province: int, _region: int, owner: int) -> void:
	if Sim.world == null:
		return
	if province < 0:
		_prov_panel.show_province({})        # clic en mer → on referme
		_country_panel.show_country({})
		return
	_prov_panel.show_province(Sim.world.province_info(province))
	if owner >= 0:
		_country_panel.show_country(Sim.world.country_info(owner))
	else:
		_country_panel.show_country({})      # terre libre → pas de pays
