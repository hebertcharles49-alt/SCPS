extends Node
## shot_zoom — bascule en ISO sur la plus grosse ville et capture le bourg/routes (vue de JEU).
var _map: Node2D = null
func _ready() -> void:
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map); _run.call_deferred()
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(120): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(6): await get_tree().process_frame
	var w = Sim.world
	var best := -1
	var bestpop := -1
	for r in range(w.region_count()):
		if w.region_tier(r) < 0: continue
		var p: int = w.region_pop(r)
		if p > bestpop: bestpop = p; best = r
	var c := Vector2(512, 256)
	if best >= 0:
		c = w.region_centroid(best)
		print("focus region ", best, " pop ", bestpop, " at ", c)
	# bascule en ISO au point regardé, puis plonge (zoom 8)
	_map._enter_iso(c)
	_map._camera.zoom = Vector2(8.0, 8.0)
	_map._camera.position = _map.iso_pos(c.x, c.y)
	for i in range(8): await get_tree().process_frame
	await RenderingServer.frame_post_draw
	print("view_mode=", _map.view_mode, " zoom=", _map._camera.zoom)
	_map.get_viewport().get_texture().get_image().save_png("res://iso_play.png"); print("SAVED")
	get_tree().quit(0)
