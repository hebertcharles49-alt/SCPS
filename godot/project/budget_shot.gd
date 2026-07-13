extends Node
## budget_shot — capture du PILOTE « grand livre parchemin » (budget_panel_v2).
## Probe de rendu hors make, FENÊTRÉE (--headless = noir). Génère un monde, avance
## quelques années, instancie budget_panel_v2 dans un CanvasLayer, attend quelques
## frames et sauve un PNG 1600×900.
##   Godot --path godot/project res://budget_shot.tscn -- seed=9 years=30
const OUT := "C:/Users/Charl/Desktop/SCPS-main/build/budget_v2.png"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	# avance le temps → un vrai budget (trésor, flux, enveloppes réalisées)
	for i in range(int(_arg("years=", "30"))):
		Sim.world.advance_days(360)
	Sim.generated.emit()

	# fond neutre pour lire le parchemin (le shell n'est PAS booté — pilote isolé)
	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)

	var lay := CanvasLayer.new()
	add_child(lay)
	var panel: Control = load("res://ui/budget_panel_v2.gd").new()
	lay.add_child(panel)

	# laisse le mois passer une fois pour peupler les valeurs
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.emit(Sim.world.year())
	for i in range(12):
		await get_tree().process_frame
	if panel.has_method("refresh"):
		panel.refresh()
	for i in range(6):
		await get_tree().process_frame
	# hug content (aucune taille forcée) puis centrer sur le canevas 1600×900
	panel.reset_size()
	panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()
	for i in range(4):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw

	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(OUT)
	if err == OK:
		print("SAVED ", OUT, " (", img.get_width(), "x", img.get_height(), ")")
	else:
		push_error("save_png failed err=%d" % err)
	get_tree().quit(0 if err == OK else 1)
