extends Node
## title_shot — capture l'ÉCRAN TITRE (menu_root.gd) tel qu'affiché au lancement, AVANT
## toute partie : chrome 2026-08-26 (title_screen.png, menu décalé au tiers gauche) +
## curseur flèche par défaut. Motif map_art_shot : FENÊTRÉ (--headless = noir).
##   Godot --audio-driver Dummy --path godot/project res://title_shot.tscn

var _main: Node = null
var _dir := "res://shots_title/"

func _ready() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().unfocusable = true
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
	# ~8 s au rythme du _process (pas de sleep : on attend des FRAMES, cf. brief) —
	# laisse le shell menu_root construire son fond + son menu avant la capture.
	for i in range(480):
		await get_tree().process_frame
	await _shot("00_title_screen")
	print("TITLE SHOT OK — ", _dir)
	get_tree().quit()
