extends Node
## colon_shot — REGARDER le monde au lieu de deviner (« tu mesures sur quelle graine ?
## l'as-tu seulement regardée ? »). Monde ENTIER à l'an 250 : vue POLITIQUE (qui tient
## quoi — les vides sautent aux yeux) puis vue NATURE (le terrain sous les vides : mer,
## glacier, ou terre vivable abandonnée). Fenêtré, muet, unfocusable.
##   SCPS_MUTE=1 Godot --audio-driver Dummy --path . res://colon_shot.tscn -- seed=1518

var _main: Node = null
var _dir := "res://shots_colon/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().unfocusable = true
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(8):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(ProjectSettings.globalize_path(_dir + nom + ".png"))
	print("SHOT ", nom)

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "1518")))
	for i in range(30):
		await get_tree().process_frame
	if Sim.world == null:
		push_error("no world"); get_tree().quit(1); return
	Sim.world.advance_days(360 * 250)          # AN 250 — l'horizon des mesures
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sim.game_on = true
	Sim.speed_index = 0
	var map: Node = _main.get_node_or_null("MapView")
	var w = Sim.world
	var ov = map.get_node_or_null("Overlay")
	if ov != null:
		ov.fog_off = true                      # voir TOUT le monde, pas seulement le découvert
		ov.queue_redraw()
	for i in range(45):
		await get_tree().process_frame
	# MONDE ENTIER : zoom au plus large, centré sur la carte
	map._camera.zoom = Vector2(0.9, 0.9)
	map._camera.position = map.iso_pos(float(w.map_w()) * 0.5, float(w.map_h()) * 0.5)
	map._nav_redraw()
	await _shot("01_politique_an250")
	if not map.is_nature():
		map.toggle_nature()
	map._nav_redraw()
	await _shot("02_nature_an250")
	print("COLON SHOTS OK — ", _dir)
	get_tree().quit()
