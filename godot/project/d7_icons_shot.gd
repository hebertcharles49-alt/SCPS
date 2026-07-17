extends Node
## d7_icons_shot — preuve visuelle APRÈS pour la mission UI-DOCTRINE D7 (tailles
## d'icônes : rail gauche, topbar, tiroir). Boote le VRAI shell (Main.tscn, motif
## uipolish_shot.gd) — piège connu : une probe isolée MENT par contexte (thème de
## fenêtre, sidebar, empire_sidebar…). FENÊTRÉE seulement (--headless = hang connu).
## L'AVANT reste reconstituable via `git show pre-uidoctrine:godot/project/ui/topbar.gd`
## (tag posé avant tout changement de la mission UI-DOCTRINE) — cf. brief D7.
##   Godot --path godot/project res://d7_icons_shot.tscn -- seed=9 years=25
var _main: Node = null
var _map: Node2D = null
var _dir := "res://shots_uidoctrine_d7/"

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

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(_dir + nom + ".png")
	print("SHOT ", nom)

func _reset() -> void:
	while _main._close_topmost():
		pass
	var sb: Control = _main._sidebar
	if sb != null and sb.has_method("close"):
		sb.close()

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

	_map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		_map._camera.zoom = Vector2(3.0, 3.0)
		_map._camera.position = _map.iso_pos(cc.x, cc.y)
		_map.queue_redraw()

	# ── 1. RAIL GAUCHE + TOPBAR seuls (aucun onglet ouvert) — vue de référence ──
	await _shot("01_rail_topbar_seuls")

	# ── 2. TIROIR ÉCONOMIE (F1) — chip menu_economy 13→16, gold_coin/trade inchangés ──
	_main._sidebar.open_tab(0)
	await _shot("02_tiroir_economie")

	# ── 3. TIROIR ARMÉE (F5) — menu_army 18→22, harbor_anchor 16→18 ──
	_main._sidebar.open_tab(4)
	await _shot("03_tiroir_armee")

	# ── 4. TIROIR CONSEIL (F8) — menu_council 16→20 (repli siège vacant) ──
	_main._sidebar.open_tab(7)
	await _shot("04_tiroir_conseil")

	# ── 5. TIROIR DÉMOGRAPHIE (F2) — population_group 14→16 par classe ──
	_main._sidebar.open_tab(1)
	await _shot("05_tiroir_demographie")
	_reset()

	# ── 6. ZOOM topbar (cellules 26→32 + couronne 18→26) — capture pleine fenêtre,
	#     la topbar occupe la bande du haut, lisible en zoomant sur le PNG ensuite. ──
	await _shot("06_topbar_plein")

	print("D7 ICONS SHOTS OK — ", _dir)
	get_tree().quit(0)
