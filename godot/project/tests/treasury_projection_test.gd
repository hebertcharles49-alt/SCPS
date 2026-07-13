extends Node

const Topbar = preload("res://ui/topbar.gd")

func _ready() -> void:
	var topbar = Topbar.new()
	var me := int(Sim.world.player())
	var budget: Dictionary = Sim.world.budget_summary(me)
	_check(budget.has("monthly_net") and budget.has("projected_year_end") and budget.has("runway_months"),
		"projection absente du binding")
	var card: Dictionary = topbar._treasury_card(Sim.world, me, float(budget.get("gold", 0.0)))
	var labels := []
	for line in card.get("lines", []):
		labels.append(String(line.get("label", "")))
	_check(labels.has("Fin d'année (projection)"), "projection de fin d'année absente du hover")
	_check(labels.has("Autonomie trésor + crédit"), "autonomie absente du hover")
	topbar.free()
	print("treasury_projection_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("treasury_projection_test: " + message)
	get_tree().quit(1)

