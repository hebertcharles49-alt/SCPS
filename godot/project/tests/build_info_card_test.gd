extends Node

const ConstructionPanel = preload("res://ui/construction_panel.gd")

func _ready() -> void:
	_check(Sim.world != null, "monde absent")
	var panel = ConstructionPanel.new()
	var me: int = Sim.world.player()
	var found := false
	for building in Sim.world.building_roster(me):
		if not bool(building.get("debloque", false)):
			continue
		var legal: Dictionary = Sim.world.build_legal(-1, int(building.get("type", -1)))
		var card: Dictionary = panel._build_info_card(building, legal)
		_check(String(card.get("title", "")) != "", "titre absent")
		_check(String(card.get("state", "")) != "", "état absent")
		_check(String(card.get("trend", "")).contains("jours"), "durée absente")
		_check((card.get("lines", []) as Array).size() >= 1, "coûts absents")
		found = true
		break
	panel.free()
	_check(found, "aucun édifice débloqué pour le fixture")
	print("build_info_card_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("build_info_card_test: " + message)
	get_tree().quit(1)

