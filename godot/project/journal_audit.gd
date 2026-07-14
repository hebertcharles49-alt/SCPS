extends Node
## journal_audit — probe du JOURNAL (section persistante de la bande droite,
## empire_sidebar.gd, alimentée par alerts.gd::journal_rows). Fenêtré (le rendu
## compte) : Godot --path godot/project res://journal_audit.tscn
func _ready() -> void:
	get_window().size = Vector2i(560, 1000)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(42)
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	for i in range(9):
		Sim.world.advance_days(3650)   # ~90 ans : de quoi peupler guerres/batailles/révoltes/évènements
	Sim.game_on = true
	var ui := CanvasLayer.new()
	add_child(ui)
	var alerts = load("res://ui/alerts.gd").new()
	alerts.name = "Alerts"
	ui.add_child(alerts)
	alerts.set_ledger_mode(true)   # comme main.gd : tout le rendu passe par la bande droite
	var sb = load("res://ui/empire_sidebar.gd").new()
	sb.name = "EmpireSidebar"
	ui.add_child(sb)
	sb.set_alert_source(alerts)
	await get_tree().process_frame
	await get_tree().process_frame

	var jrows: Array = alerts.journal_rows() if alerts.has_method("journal_rows") else []
	print("JOURNAL n=", jrows.size())
	var act_count := {}
	for r in jrows:
		var a := String(r.get("act", "(vide)"))
		act_count[a] = int(act_count.get(a, 0)) + 1
	print("PAR ACT : ", act_count)
	for r in jrows.slice(0, 12):
		print("  an ", r.get("year"), " · ", r.get("tip"), "  [act=", r.get("act"), " col=", r.get("col"), "]")
	# preuve d'edge-detection : au moins une entrée « condition » (act de _collect,
	# jamais « goto »/« tech_metab ») doit exister — pas seulement le fil moteur.
	var cond_acts := ["council", "army", "market", "tech", "construct", "religion", "age"]
	var has_cond := false
	for r in jrows:
		if cond_acts.has(String(r.get("act", ""))):
			has_cond = true
			print("CONDITION EXEMPLE : an ", r.get("year"), " · ", r.get("tip"))
			break
	print("A UNE CONDITION AU JOURNAL=", has_cond)

	sb.queue_redraw()
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	# scroller vers le bas : le JOURNAL est la DERNIÈRE section du panneau — sans ça,
	# la capture ne montrerait que le résumé d'empire (armées/villes/mission).
	sb._scrolloff = sb._maxscroll
	sb.queue_redraw()
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://journal_audit.png")
	print("SAVED journal_audit.png maxscroll=", sb._maxscroll)
	get_tree().quit(0 if jrows.size() > 0 else 1)
