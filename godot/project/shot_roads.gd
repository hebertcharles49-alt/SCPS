extends Node
## shot_roads — force le réseau de routes COMPLET (le test avance le temps puis émet generated,
## ce qui redate les chantiers à « maintenant » → frac 0 ; ici on antidate pour TOUT bâtir) afin
## de valider le rendu « fil conducteur » + l'habillage de bord de route.
var _map: Node2D = null
func _ready() -> void:
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map); _run.call_deferred()
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(150): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(6): await get_tree().process_frame
	var w = Sim.world
	var ov = _map.get_node("Overlay")
	# ANTIDATE tous les chantiers → routes 100 % bâties + habillage autorisé
	var yr: int = w.year()
	for rd in ov._roads:
		ov._road_start[rd["key"]] = yr - 1000
	print("roads=", ov._roads.size(), " dress=", ov._road_dress.size(), " year=", yr)
	# centre sur la région la plus peuplée (carrefour probable)
	var best := -1; var bestpop := -1
	for r in range(w.region_count()):
		if w.region_tier(r) < 0: continue
		var p: int = w.region_pop(r)
		if p > bestpop: bestpop = p; best = r
	var c := Vector2(512, 256)
	if best >= 0: c = w.region_centroid(best)
	_map._enter_iso(c)
	_map._camera.zoom = Vector2(6.0, 6.0)
	_map._camera.position = _map.iso_pos(c.x, c.y)
	for i in range(8): await get_tree().process_frame
	await RenderingServer.frame_post_draw
	_map.get_viewport().get_texture().get_image().save_png("res://roads_test.png"); print("SAVED")
	get_tree().quit(0)
