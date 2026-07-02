extends Node
## sidebar_audit — probe de l'EMPIRE SIDEBAR (résumé + factions + mission + log).
## Fenêtré (le rendu compte) : Godot --path godot/project res://sidebar_audit.tscn
func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(9)
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	for i in range(30):
		Sim.world.advance_days(360)   # 30 ans : factions/mission/pop en place
	Sim.game_on = true
	var me: int = Sim.world.player()
	var fx: Dictionary = Sim.world.country_factions(me)
	print("FACTIONS n=", (fx.get("list", []) as Array).size(),
		" coup=", fx.get("coup", -1), " corruption=", fx.get("corruption", -1))
	for fe in fx.get("list", []):
		print("  ", fe.get("name"), " part=", fe.get("part"), " grief=", fe.get("grief"),
			" dom=", fe.get("dominant"))
	print("MISSION ", Sim.world.mission_info(me))
	print("FOOD ", Sim.world.country_food(me), " DIPLO_CD ", Sim.world.diplo_cd())
	var ui := CanvasLayer.new()
	add_child(ui)
	var sb = load("res://ui/empire_sidebar.gd").new()
	ui.add_child(sb)
	Sim.ticked.emit(Sim.world.year())   # déclenche le poll du fil
	for i in range(6):
		sb.queue_redraw()
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://sidebar1.png")
	print("SAVED")
	get_tree().quit(0)
