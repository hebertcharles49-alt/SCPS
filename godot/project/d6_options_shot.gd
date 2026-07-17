extends Node
## d6_options_shot — preuve visuelle des 4 curseurs de volume (mission UI-DOCTRINE D6).
## Boote le VRAI shell (Main.tscn, motif uipolish_shot.gd) et affiche l'écran Options
## DEPUIS le menu principal — écran de menu, accessible avant toute partie (pas besoin
## de Sim.regenerate). FENÊTRÉE seulement (--headless = hang connu, cf. TROUVAILLES).
##   Godot --path godot/project res://d6_options_shot.tscn -- seed=9
var _main: Node = null
var _dir := "res://shots_uidoctrine_d6/"

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	if FileAccess.file_exists("user://session_running.flag"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path("user://session_running.flag"))
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(_dir + nom + ".png")
	print("SHOT ", nom)

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return

	var menu: Control = _main._menu
	if menu == null:
		push_error("no menu")
		get_tree().quit(1)
		return

	# écran-titre du menu (Jouer/Charger/Options/Quitter)
	await _shot("01_menu_principal")

	# écran Options — bouton "Options" du menu (menu_root._options)
	var opt: Control = menu._options
	menu._show(opt)
	await _shot("02_options_son")

	print("D6 OPTIONS SHOTS OK — ", _dir)
	get_tree().quit(0)
