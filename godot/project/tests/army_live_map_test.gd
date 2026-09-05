extends Node

var failures := 0
var main: Node
var map: Node

func check(ok: bool, message: String) -> void:
	print("CHECK ", message, ": ", ok)
	if not ok:
		failures += 1

func day() -> void:
	var event: Dictionary = Sim.world.pending_event(0)
	if bool(event.get("valid", false)):
		Sim.world.player_event_choice(0, 0)
	Sim.world.advance_days(1)
	Sim.ticked.emit(Sim.world.year())

func shot(label: String) -> void:
	map._nav_redraw()
	for i in range(8):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	check(get_viewport().get_texture().get_image().save_png("user://army_live_" + label + ".png") == OK, "capture " + label)

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	get_window().content_scale_size = Vector2i(1600, 900)
	get_window().unfocusable = true
	run.call_deferred()

func run() -> void:
	main = load("res://main/Main.tscn").instantiate()
	add_child(main)
	await get_tree().process_frame
	Sim.regenerate(9)
	Sim.game_on = true
	Sim.speed_index = 0
	if main._menu != null:
		main._menu.hide()
	map = main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap: int = w.country_capital_region(me)
	w.player_recruit(0)
	day()
	w.player_raise_corps(1, cap)
	day()
	var ids: Array = w.corps_ids(me)
	check(not ids.is_empty(), "corps leve par commandes")
	if ids.is_empty():
		get_tree().quit(1)
		return
	var id: int = ids[0]
	var target := -1
	for region in range(w.region_count()):
		var owner: int = w.region_owner(region)
		if owner < 0 or not w.country_known(owner):
			continue
		var preview: Dictionary = w.corps_move_preview(id, region)
		if bool(preview.get("valid", false)) and float(preview.get("travel_days", 0)) >= 8 and float(preview.get("travel_days", 0)) < 60:
			target = region
			break
	check(target >= 0, "destination accessible")
	if target < 0:
		get_tree().quit(1)
		return
	map._set_selected_corps([id])
	map._set_move_preview(map._aggregate_move_preview(target))
	for i in range(40):
		await get_tree().process_frame
	var ov = map._overlay
	var pos: Vector2 = ov._army_world_position(w, w.corps_info(id))
	map._camera.zoom = Vector2(6, 6)
	map._camera.position = map.iso_pos(pos.x, pos.y)
	await shot("preview")
	check(map._issue_selected_move(target) == 1, "ordre transmis via carte")
	day()
	await shot("start")
	var first: Vector2 = ov._pa_positions[id]["pos"]
	for i in range(3):
		day()
	await shot("moving")
	var last: Vector2 = ov._pa_positions[id]["pos"]
	check(first.distance_to(last) > 0.01, "position rendue avance sur vraie carte")
	check(ov.point_hits_player_army(last) == id, "selection aux pieds en mouvement")
	var hit: Rect2 = ov._pa_positions[id]["rect"]
	check(ov.point_hits_player_army(hit.get_center()) == id, "selection sur silhouette en mouvement")
	var elapsed := 0
	while int(w.corps_info(id).get("phase_id", -1)) == 1 and elapsed < 90:
		day()
		elapsed += 1
	check(int(w.corps_info(id).get("region", -1)) == target, "arrivee destination par moteur")
	check(int(w.corps_info(id).get("phase_id", -1)) == 0, "arrivee en paix sans siege")
	await shot("arrival")
	print("army_live_map_test failures=", failures)
	main.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	get_tree().quit(0 if failures == 0 else 1)
