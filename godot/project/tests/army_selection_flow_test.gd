extends Node

const MapView = preload("res://map/map_view.gd")

class MockWorld extends RefCounted:
	var moved_id := -1
	var moved_region := -1

	func corps_info(id: int) -> Dictionary:
		return {"active": id == 7}

	func player_move_corps(id: int, region: int) -> bool:
		moved_id = id
		moved_region = region
		return true

	func province_info(_province: int) -> Dictionary:
		return {"nom": "Mont-Roux"}

	func corps_move_preview(id: int, region: int) -> Dictionary:
		return {
			"valid": id == 7, "corps_id": id, "target_region": region,
			"target_name": "Mont-Roux", "travel_days": 17.4,
			"reason": "Route praticable", "arrival": "Marche vers un siège",
			"units_start": 1400, "attrition_loss": 100, "units_arrival": 1300,
			"worst_daily_pct10": 10,
			"path": [2, 5, region],
		}

func _ready() -> void:
	var previous = Sim.world
	var world := MockWorld.new()
	Sim.world = world
	var map = MapView.new()
	var feedback: Array = []
	map.army_order_feedback.connect(func(message, good): feedback.append([message, good]))
	map._set_selected_corps([7])
	var issued: int = map._issue_selected_move(8, 12)
	_check(issued == 1 and world.moved_id == 7 and world.moved_region == 8, "ordre non transmis")
	_check(map._selected_corps == [7] and map._army_selected, "sélection perdue après l'ordre")
	_check(feedback.size() == 1 and bool(feedback[0][1]), "accusé positif absent")
	_check(String(feedback[0][0]).contains("Mont-Roux"), "destination non nommée")
	var preview: Dictionary = map._aggregate_move_preview(8)
	_check(bool(preview.get("valid", false)), "aperçu agrégé refusé")
	_check(int(preview.get("corps_count", 0)) == 1, "nombre de corps d'aperçu faux")
	_check(is_equal_approx(float(preview.get("travel_days", 0.0)), 17.4), "durée d'aperçu fausse")
	_check(int(preview.get("attrition_loss", 0)) == 100, "attrition d'aperçu perdue")
	_check(int(preview.get("units_arrival", 0)) == 1300, "effectif d'arrivée perdu")
	_check(preview.get("path", []) == [2, 5, 8], "route d'aperçu perdue")
	map.free()
	Sim.world = previous
	print("army_selection_flow_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("army_selection_flow_test: " + message)
	get_tree().quit(1)
