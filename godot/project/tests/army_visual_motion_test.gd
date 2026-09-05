extends Node

const MapView = preload("res://map/map_view.gd")
const Overlay = preload("res://map/overlay.gd")
var _checks := 0
var _failures := 0

class MotionWorld extends RefCounted:
	func region_centroid(region: int) -> Vector2:
		return Vector2(float(region * 10), float(region * 4))

class RouteWorld extends RefCounted:
	var only_route := true

	func player() -> int:
		return 0

	func corps_ids(_country: int) -> Array:
		return [7, 8]

	func corps_info(id: int) -> Dictionary:
		return {"active": id == 7}

	func corps_route(id: int) -> Array:
		return [2, 5, 8] if id == 7 and only_route else []

func _ready() -> void:
	var motion_world := MotionWorld.new()
	var overlay := Overlay.new()
	overlay._region_seat = {2: Vector2(20, 8), 5: Vector2(50, 20)}
	var at_start := overlay._army_world_position(motion_world, {
		"region": 2, "next": 5, "phase_id": 1, "progress_pct": 0,
	})
	var at_half := overlay._army_world_position(motion_world, {
		"region": 2, "next": 5, "phase_id": 1, "progress_pct": 50,
	})
	var at_end := overlay._army_world_position(motion_world, {
		"region": 2, "next": 5, "phase_id": 1, "progress_pct": 100,
	})
	_check(at_start == Vector2(20, 8), "marche 0% hors de l'ancre courante")
	_check(at_half == Vector2(35, 14), "marche 50% non interpolée")
	_check(at_end == Vector2(50, 20), "marche 100% hors de l'étape suivante")
	var ratio_fallback := overlay._army_world_position(motion_world, {
		"region": 2, "next": 5, "phase_id": 1, "progress_pct": -1,
		"leg_days": 20.0, "days_left": 15.0,
	})
	_check(ratio_fallback == Vector2(27.5, 11.0), "repli leg_days/days_left incorrect")
	var at_siege := overlay._army_world_position(motion_world, {
		"region": 2, "next": 5, "phase_id": 2, "progress_pct": 90,
	})
	_check(at_siege == Vector2(20, 8), "siège déplacé par la progression")

	var previous = Sim.world
	var route_world := RouteWorld.new()
	Sim.world = route_world
	var map := MapView.new()
	map._overlay = overlay
	map._refresh_engaged_routes()
	_check(map._engaged_routes == {7: [2, 5]}, "segment engagé non borné à loc→next")
	_check(overlay.engaged_routes == {7: [2, 5]}, "segment non publié à l'overlay")
	route_world.only_route = false
	map._refresh_engaged_routes()
	_check(map._engaged_routes.is_empty() and overlay.engaged_routes.is_empty(), "route périmée non purgée")

	var soldier_center := Vector2(100, 100)
	var soldier_rect: Rect2 = overlay._army_soldier_rect(soldier_center, 50.0)
	var soldier_entry := {"pos": soldier_center, "radius": 35.0, "rect": soldier_rect}
	overlay._pa_positions = {7: soldier_entry}
	var head := Vector2(soldier_rect.get_center().x, soldier_rect.position.y + soldier_rect.size.y * 0.18)
	var bust := Vector2(soldier_rect.get_center().x, soldier_rect.position.y + soldier_rect.size.y * 0.50)
	_check(overlay.point_hits_player_army(head) == 7, "tête hors hit-test")
	_check(overlay.point_hits_player_army(bust) == 7, "buste hors hit-test")
	_check(overlay.point_hits_player_army(soldier_center) == 7, "pieds hors hit-test")
	_check(overlay.point_hits_player_army(soldier_rect.position - Vector2(1, 1)) == -1, "extérieur sélectionné")
	overlay._pa_positions = {"g0": {"pos": soldier_center, "radius": 100.0, "rect": soldier_rect}}
	_check(overlay.point_hits_player_army(head) == -1, "garnison sélectionnable")
	overlay._pa_positions = {9: soldier_entry, 3: soldier_entry}
	_check(overlay.point_hits_player_army(bust) == 3, "chevauchement non déterministe")
	overlay._pa_positions = {7: soldier_entry, "g0": {"pos": soldier_center, "radius": 100.0, "rect": soldier_rect}}
	var crossing := Rect2(Vector2(soldier_rect.position.x - 2.0, soldier_rect.position.y + 10.0), Vector2(4.0, 6.0))
	_check(overlay.player_corps_in_rect(crossing).has(7), "croisement du rectangle non détecté")
	map.free()
	overlay.free()
	Sim.world = previous
	if _failures > 0:
		get_tree().quit(1)
		return
	print("army_visual_motion_test: %d/%d OK" % [_checks, _checks])
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	_checks += 1
	if ok:
		return
	_failures += 1
	push_error("army_visual_motion_test: " + message)
