extends Node

const ArmyPanel = preload("res://ui/army_panel.gd")

func _ready() -> void:
	var panel = ArmyPanel.new()
	var march := panel._corps_status_text({
		"id": 7, "region": 2, "location": "Valdor", "phase": "En marche", "units": 1400,
		"dest": 8, "destination": "Mont-Roux", "progress_pct": 40, "days_left": 9.0,
		"broken_days": 0, "rally_days": 0.0,
	})
	_check(march.contains("Valdor") and march.contains("Mont-Roux"), "trajet illisible")
	_check(march.contains("40%") and march.contains("9 j restants"), "progression de marche absente")
	var routed := panel._corps_status_text({
		"id": 9, "region": 4, "location": "Sore", "phase": "À l'arrêt", "units": 600,
		"dest": -1, "progress_pct": -1, "broken_days": 18,
		"rally_days": 12.0, "rally_units": 400,
	})
	_check(routed.contains("BRISÉ 18 j"), "déroute absente")
	_check(routed.contains("ralliement 12 j (400 hommes)"), "ralliement absent")
	_check(not routed.contains("→"), "destination fantôme sur corps immobile")
	var preview := panel._move_preview_text({
		"valid": true, "corps_count": 2, "target_name": "Mont-Roux",
		"travel_days": 17.4, "arrival": "Marche vers un siège",
		"units_start": 4300, "attrition_loss": 300, "units_arrival": 4000,
		"attrition_pct": 7, "worst_daily_pct10": 30,
	})
	_check(preview.contains("2 corps") and preview.contains("Mont-Roux"), "destination d'aperçu illisible")
	_check(preview.contains("~17 j") and preview.contains("siège"), "durée ou issue d'aperçu absente")
	_check(preview.contains("4 300 → 4 000 hommes"), "effectif d'arrivée projeté absent")
	_check(preview.contains("−300 en marche (7%)"), "attrition projetée absente")
	_check(preview.contains("3.0%/j"), "pire terrain d'attrition absent")
	var refused := panel._move_preview_text({
		"valid": false, "corps_count": 2, "invalid_count": 1,
		"target_name": "Littoral", "reason": "Aucune route terrestre",
	})
	_check(refused.contains("Impossible") and refused.contains("Aucune route terrestre"), "refus d'aperçu absent")
	_check(refused.contains("1/2 corps bloqués"), "portée du refus absente")
	var stack := panel._stack_summary_text([
		{"id": 7, "phase_id": 0}, {"id": 9, "phase_id": 1},
	], [4], 4300)
	_check(stack.contains("2 corps") and stack.contains("4 300 hommes"), "synthèse de stack absente")
	_check(stack.contains("corps #7"), "résultat de fusion non annoncé")
	var dispersed := panel._stack_summary_text([
		{"id": 7, "phase_id": 0}, {"id": 9, "phase_id": 0},
	], [4, 8], 4300)
	_check(dispersed.contains("2 régions") and dispersed.contains("impossible"), "stack dispersé non expliqué")
	_check(panel._split_packets(4300) == 21, "conversion hommes vers paquets de scission fausse")
	var refill := panel._refill_summary_text([
		{"valid": true, "allowed": true, "requested_humans": 200,
			"population_ready_humans": 200, "guaranteed_humans": 100,
			"weapons_needed": 200, "weapons_owned": 100,
			"needs": [{"resource": 20, "name": "Armes légères", "needed": 200, "owned": 100}]},
	])
	_check(refill.contains("+200 hommes") and refill.contains("100 garantis"), "volume de renfort illisible")
	_check(refill.contains("200 armes (100 nationales)"), "coût d'armement absent")
	_check(refill.contains("marché sollicité"), "complément marchand non annoncé")
	var blocked := panel._refill_summary_text([
		{"valid": true, "allowed": false, "reason": "Ravitaillement possible uniquement sur une région nationale"},
	])
	_check(blocked.contains("indisponible") and blocked.contains("région nationale"), "refus de renfort illisible")
	panel.free()
	print("army_panel_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("army_panel_test: " + message)
	get_tree().quit(1)
