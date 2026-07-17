extends Node
## d1_after_shot — capture APRÈS (D1-UNIFICATION) de la fiche province au clic,
## comparaison avec d1_before_shot.gd (worktree pre-uidoctrine). Motif uipolish_shot.gd.
## FENÊTRÉ seulement.
##   Godot --path godot/project res://d1_after_shot.tscn -- seed=9 years=25
var _main: Node = null
var _dir := "res://shots_uidoctrine_d1/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
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

	var map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		map._camera.zoom = Vector2(3.0, 3.0)
		map._camera.position = map.iso_pos(cc.x, cc.y)
		map.queue_redraw()

	# APRÈS (D1-UNIFICATION) : le clic ouvre LA SEULE fiche (province_panel_v2.gd),
	# nomenclature canonique Journaliers/Bourgeois/Élites, pied d'actions (Réprimer/
	# Assimiler/Purger/Détail — ma province), onglet Infrastructure par défaut.
	_main._on_province_picked(cap_prov, cap_reg, me)
	map._selected_prov = cap_prov
	var ov = map.get_node_or_null("Overlay")
	if ov != null: ov.queue_redraw()
	await _shot("d1_apres_01_clic_province_unifiee_infra")

	# onglet Région (agrégat nommé, conforme doctrine)
	if _main._prov_panel_v2 != null and _main._prov_panel_v2.has_method("select_tab"):
		_main._prov_panel_v2.select_tab(1)
		await _shot("d1_apres_02_onglet_region")
		_main._prov_panel_v2.select_tab(0)

	# une province ÉTRANGÈRE en paix (pour montrer le pied Route terre/mer) et une
	# province VIERGE (pour montrer Coloniser) si on en trouve une facilement
	var n := int(w.province_count())
	var foreign_pid := -1
	var wild_pid := -1
	for p in range(n):
		var info: Dictionary = w.province_info(p)
		if not bool(info.get("valide", false)):
			continue
		var o := int(info.get("owner", -2))
		if o >= 0 and o != me and foreign_pid < 0:
			foreign_pid = p
		elif o < 0 and wild_pid < 0:
			wild_pid = p
		if foreign_pid >= 0 and wild_pid >= 0:
			break
	if foreign_pid >= 0:
		var freg: int = w.province_region(foreign_pid)
		var finfo: Dictionary = w.province_info(foreign_pid)
		_main._on_province_picked(foreign_pid, freg, int(finfo.get("owner", -1)))
		map._selected_prov = foreign_pid
		if ov != null: ov.queue_redraw()
		await _shot("d1_apres_03_province_etrangere_pied_diplo")
	if wild_pid >= 0:
		var wreg: int = w.province_region(wild_pid)
		_main._on_province_picked(wild_pid, wreg, -1)
		map._selected_prov = wild_pid
		if ov != null: ov.queue_redraw()
		await _shot("d1_apres_04_province_vierge_pied_coloniser")

	# le ✕ ferme la fiche en UN Échap (ex-2 Échap avant le fix major_open/_close_topmost)
	_main._on_province_picked(cap_prov, cap_reg, me)
	await get_tree().process_frame
	print("AVANT Echap: prov_panel_v2.visible=", _main._prov_panel_v2.visible, " sel_prov=", _main._sel_prov)
	_main._close_topmost()
	await get_tree().process_frame
	print("APRES 1 Echap: prov_panel_v2.visible=", _main._prov_panel_v2.visible, " sel_prov=", _main._sel_prov,
		" (attendu: false, -1 — UN SEUL Echap suffit désormais)")

	print("D1 AFTER SHOTS OK — ", _dir)
	get_tree().quit(0)
