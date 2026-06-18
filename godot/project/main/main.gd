extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène.

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
