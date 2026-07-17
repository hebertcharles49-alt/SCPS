extends Node
## d5_resources_shot — preuve visuelle AVANT/APRÈS pour la mission UI-DOCTRINE D5
## (« les ressources prises exactes », Menu Construction). Boote le VRAI shell
## (Main.tscn, motif uipolish_shot.gd) — piège connu : une probe isolée MENT par
## contexte (thème de fenêtre, sidebar…). FENÊTRÉE seulement (--headless = hang connu,
## cf. TROUVAILLES).
##   Godot --path godot/project res://d5_resources_shot.tscn -- seed=9 years=25
var _main: Node = null
var _dir := "res://shots_uidoctrine_d5/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	if FileAccess.file_exists("user://session_running.flag"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path("user://session_running.flag"))
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_run.call_deferred()

## ferme le dialogue « Fermeture anormale détectée » (ui/feedback.gd) s'il traîne d'une
## session précédente tuée par un timeout de probe — sinon il reste PAR-DESSUS le Menu
## Construction et masque les puces de ressources sur la capture.
func _dismiss_crash_dialog() -> void:
	var fb: Node = _main.get_node_or_null("Feedback")
	if fb == null:
		return
	for c in fb.get_children():
		if c is ConfirmationDialog:
			c.hide()
			c.queue_free()

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

	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	var years := int(_arg("years=", "25"))
	for i in range(years):
		Sim.world.advance_days(360)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)
	_dismiss_crash_dialog()
	await get_tree().process_frame

	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)

	# AUDIT D5 — combien de régions possède le joueur (pour vérifier visuellement le
	# facteur d'étendue 1+0.15·n appliqué aux puces de ressources) : imprimé dans les
	# logs, comparable à la quantité affichée sur les cartes.
	var ci: Dictionary = w.country_info(me)
	print("D5 country regions=", ci.get("regions", 0), " gold=", ci.get("or", 0))

	_main._on_province_picked(cap_prov, cap_reg, me)
	_main._construct.visible = true
	_main._construct.target_pid = cap_prov
	if _main._construct.has_method("open_on"):
		_main._construct.open_on(0)
	_main._construct.queue_redraw()
	await _shot("01_construction_edifices")

	if _main._construct.has_method("open_on"):
		_main._construct.open_on(1)
		_main._construct.queue_redraw()
	await _shot("02_construction_manufactures")

	print("D5 SHOTS OK — ", _dir)
	get_tree().quit(0)
