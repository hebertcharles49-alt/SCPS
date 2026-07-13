extends Node

const Alerts = preload("res://ui/alerts.gd")

func _ready() -> void:
	var packed := 12 | (34 << 16)
	var text := Alerts._battle_losses_text(packed)
	if not text.contains("nous 1 200") or not text.contains("ennemi 3 400"):
		push_error("alerts_battle_test: décodage des pertes faux")
		get_tree().quit(1)
		return
	var action := Alerts._feed_event_action(8, 41)
	if String(action.get("act", "")) != "goto" or int(action.get("region", -1)) != 41:
		push_error("alerts_battle_test: bataille non localisable")
		get_tree().quit(1)
		return
	if not Alerts._feed_event_action(2, 41).is_empty():
		push_error("alerts_battle_test: paix routée vers une région arbitraire")
		get_tree().quit(1)
		return
	print("alerts_battle_test: OK")
	get_tree().quit(0)
