extends Node
var _map: Node2D = null
func _ready() -> void:
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map); _run.call_deferred()
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(40): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(10): await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://globe.png"); print("SAVED")
	get_tree().quit(0)
