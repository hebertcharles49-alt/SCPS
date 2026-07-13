extends Node

const BattlePanel = preload("res://ui/battle_panel.gd")

func _ready() -> void:
	var panel = BattlePanel.new()
	var bi := {
		"terrain_holder": 4, "atk_terrain_pct": 91, "def_terrain_pct": 110,
		"atk_counter_pct": 106, "def_counter_pct": 94,
	}
	_check(panel._terrain_text(bi, 2, 4).contains("défenseur +10%"), "avantage terrain illisible")
	var counters := panel._counter_text(bi)
	_check(counters.contains("attaquant +6%") and counters.contains("défenseur -6%"), "contres illisibles")
	_check(panel._signed_pct(100) == "+0%", "multiplicateur neutre faux")
	var siege := {
		"siege_days_left": 90.0, "siege_full_days": 360.0,
		"siege_defense": 3.5, "siege_food_months": 4.0,
		"siege_terrain_pct": 125, "siege_outcome": 1,
	}
	_check(panel._siege_strength_text(siege).contains("défense 3.5"), "défense de siège illisible")
	_check(panel._siege_duration_text(siege).contains("90 j restants / 360 j"), "échéance de siège illisible")
	_check(panel._siege_terrain_text(125) == "+25% de tenue", "terrain de siège illisible")
	_check(panel._siege_outcome_text(siege).contains("libération"), "issue de libération absente")
	_check(panel._siege_strength_text({}).contains("ouvrages et vivres"), "fallback ancienne DLL absent")
	panel.free()
	print("battle_panel_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("battle_panel_test: " + message)
	get_tree().quit(1)
