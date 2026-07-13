extends Node
## empire_shot — capture de la FENÊTRE EMPIRE à onglets (empire_window). Probe de rendu
## hors make, FENÊTRÉE (--headless = noir). Génère un monde, avance quelques années,
## instancie empire_window dans un CanvasLayer, puis sauve QUATRE PNG 1600×900 — un par
## onglet (Économie · Population · Diplomatie · Conseil).
##   Godot --path godot/project res://empire_shot.tscn -- seed=9 years=40

const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"
const TABS := [
	[0, "empire_economie.png"],
	[1, "empire_population.png"],
	[2, "empire_diplomatie.png"],
	[3, "empire_conseil.png"],
]

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
	# avance le temps → un vrai empire (relations, factions, conseil, budget peuplés)
	for i in range(int(_arg("years=", "40"))):
		Sim.world.advance_days(360)
	Sim.generated.emit()

	# fond neutre pour lire le parchemin (le shell n'est PAS booté — fenêtre isolée)
	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)

	var lay := CanvasLayer.new()
	add_child(lay)
	var win: Control = load("res://ui/empire_window.gd").new()
	lay.add_child(win)

	# laisse le mois passer une fois pour peupler les valeurs vivantes
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.emit(Sim.world.year())
	if win.has_method("open"):
		win.open()
	for i in range(12):
		await get_tree().process_frame

	var ok_all := true
	for entry in TABS:
		var idx: int = int(entry[0])
		var fname: String = String(entry[1])
		if win.has_method("select_tab"):
			win.select_tab(idx)
		for i in range(6):
			await get_tree().process_frame
		win.reset_size()
		win.position = ((Vector2(1600, 900) - win.size) * 0.5).floor()
		for i in range(4):
			await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var img := get_viewport().get_texture().get_image()
		var path := OUTDIR + fname
		var err := img.save_png(path)
		if err == OK:
			print("SAVED ", path, " (", img.get_width(), "x", img.get_height(), ")")
		else:
			push_error("save_png failed err=%d for %s" % [err, path])
			ok_all = false
	get_tree().quit(0 if ok_all else 1)
