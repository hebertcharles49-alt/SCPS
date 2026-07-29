extends Node
## pause_zoom_shot — REPRO du bug « en pause, l'UI ne s'actualise pas au zoom/dézoom » :
## boote le vrai shell (Main.tscn), lance la partie, met la SIM en PAUSE, puis capture
## la carte au fit / après zoom profond / après re-dézoom. Si un calque reste figé à
## l'échelle d'avant, il apparaît faux sur les PNG.
##   Godot --audio-driver Dummy --path godot/project res://pause_zoom_shot.tscn -- seed=9

var _main: Node = null
var _dir := "res://shots_pausezoom/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1280, 720)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(ProjectSettings.globalize_path(_dir + nom + ".png"))
	print("SHOT ", nom)

func _run() -> void:
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	for i in range(30):
		await get_tree().process_frame
	if Sim.world == null:
		push_error("no world"); get_tree().quit(1); return
	Sim.world.advance_days(360 * 25)   # an 25 : villes/routes/armées existent
	Sim.generated.emit()
	var menu: Control = _main._menu    # même geste que « Lancer » (motif shot_ui) :
	if menu != null:                   # regen + game_on + menu caché
		menu.hide()
	Sim.game_on = true
	var map: Node = _main.get_node_or_null("MapView")
	if map == null:
		push_error("no MapView"); get_tree().quit(1); return
	Sim.speed_index = 0                 # PAUSE SIM — le cœur du repro
	map.fit()
	await _shot("01_pause_fit")
	for i in range(10):                 # zoom profond (ISO : routes/bourgs doivent surgir)
		map.zoom_in()
		await get_tree().process_frame
	await _shot("02_pause_zoomin")
	for i in range(10):
		map.zoom_out()
		await get_tree().process_frame
	await _shot("03_pause_dezoom")
	# ISOLATION : caméra déplacée SANS _nav_redraw (l'état entre deux inputs) — si des
	# calques sont cuits en ÉCRAN au draw, ils restent plantés pendant que le monde bouge.
	Sim.speed_index = 0
	map.focus_player()
	await _shot("06_pause_focus")
	map._camera.position += Vector2(60, 0)   # « scroll » brut, aucun redraw demandé
	for i in range(6):
		await get_tree().process_frame
	await _shot("07_pause_campan_noredraw")
	map._nav_redraw()                         # l'« input » qui réactualise
	await _shot("08_apres_redraw")
	print("PAUSE ZOOM SHOTS OK — ", _dir)
	get_tree().quit()
