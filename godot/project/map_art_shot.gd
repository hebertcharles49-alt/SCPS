extends Node
## map_art_shot — GROS PLANS de la vague carte : l'ESTUAIRE du plus grand fleuve (la
## connexion à la mer se juge là, pas sur une vignette), un champ de CHEVRONS (montagnes),
## et un plan moyen pour les ENSEMBLES NOMMÉS. Cadrage par les données réelles
## (river_paths → embouchure ; couche biome → montagnes).
##   Godot --audio-driver Dummy --path godot/project res://map_art_shot.tscn -- seed=9

var _main: Node = null
var _dir := "res://shots_mapart/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1280, 720)
	get_window().unfocusable = true   # ne VOLE PAS le focus (« 25 alt-tab par minute », décision joueur) — le rendu continue, l'utilisateur garde sa fenêtre
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(ProjectSettings.globalize_path(_dir + nom + ".png"))
	print("SHOT ", nom)

func _cam(map: Node, wx: float, wy: float, z: float) -> void:
	map._camera.zoom = Vector2(z, z)
	map._camera.position = map.iso_pos(wx, wy)
	map._nav_redraw()

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
	Sim.world.advance_days(360 * 60)   # 60 ans : le réseau ROUTIER a besoin de décennies pour exister
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sim.game_on = true
	Sim.speed_index = 0
	var map: Node = _main.get_node_or_null("MapView")
	var w = Sim.world
	# 0) LES ROUTES D'ABORD, en mode NORMAL (l'aller-retour nature->normal rend la carte NOIRE
	#    — sol jamais reconstruit, piège mesuré) : cadré capitale du joueur (hors fog), an 25.
	#    du plus gros empire — les routes rayonnent des villes (an 25 : réseau posé).
	# ⚠ province_seed(pid) est un HASH rng, PAS des coords (premier essai : caméra hors
	# carte, deux shots noir uni) — le centroïde de la région-capitale est la vraie ancre.
	var cap_reg := int(w.country_capital_region(int(w.player())))
	if cap_reg >= 0:
		var ctr: Vector2 = w.region_centroid(cap_reg)
		for i in range(45):   # le sol/fog se construisent sur les premières frames
			await get_tree().process_frame
		_cam(map, ctr.x, ctr.y, 4.5)
		await _shot("06_routes_z45")
		_cam(map, ctr.x, ctr.y, 2.8)
		await _shot("07_routes_z28")
		_cam(map, ctr.x, ctr.y, 8.0)
		await _shot("08_routes_z80")
	# Cadrage ARBITRAIRE optionnel (mode NORMAL — les routes se voient ici, pas en nature) : -- at=x,y (ex. le Nil-mètre SCPS_RIVDIAG) → 00_at_*.png
	var at := _arg("at=", "")
	if at != "":
		var xy := at.split(",")
		if xy.size() == 2:
			var ov0 = map.get_node_or_null("Overlay")
			if ov0 != null:
				ov0.fog_off = true      # photographier SOUS le voile (motif shot_parch fog=0)
				ov0.queue_redraw()
			_cam(map, float(xy[0]), float(xy[1]), 7.0)
			await _shot("00_at_z7")
			_cam(map, float(xy[0]), float(xy[1]), 3.5)
			await _shot("00_at_z35")
			if ov0 != null:
				ov0.fog_off = false
				ov0.queue_redraw()
	# MODE NATURE : terrain nu sans brouillard ni politique — l'embouchure du grand fleuve
	# vit loin du joueur, sous le fog (premier essai : deux shots NOIRS)
	if not map.is_nature():
		map.toggle_nature()

	# 1) L'ESTUAIRE : embouchure du plus LONG fleuve (dernier point du tracé)
	var rivers: Array = w.river_paths()
	rivers.sort_custom(func(a, b): return (a["points"] as PackedVector2Array).size() > (b["points"] as PackedVector2Array).size())
	print("RIVSTATS total=", rivers.size())
	for k in range(mini(8, rivers.size())):
		print("RIVSTATS #", k, " len=", (rivers[k]["points"] as PackedVector2Array).size(),
			" flow=", rivers[k]["flow"])
	if not rivers.is_empty():
		var pts: PackedVector2Array = rivers[0]["points"]
		var mouth := pts[pts.size() - 1]
		print("embouchure fleuve #1 : ", mouth)
		_cam(map, mouth.x, mouth.y, 7.0)
		await _shot("01_estuaire_z7")
		_cam(map, mouth.x, mouth.y, 3.5)
		await _shot("02_estuaire_z35")
		var mid := pts[pts.size() / 2]
		_cam(map, mid.x, mid.y, 5.0)
		await _shot("03_cours_median")
	# 2) LES CHEVRONS : première cellule de biome montagne (18/19) trouvée
	var bio: Image = w.layer_image(2)
	var found := Vector2(-1, -1)
	for y in range(0, bio.get_height(), 3):
		for x in range(0, bio.get_width(), 3):
			var b := int(bio.get_pixel(x, y).r * 255.0 + 0.5)
			if b == 18 or b == 19:
				found = Vector2(x, y)
				break
		if found.x >= 0: break
	if found.x >= 0:
		_cam(map, found.x, found.y, 4.0)
		await _shot("04_chevrons")
		_cam(map, found.x, found.y, 8.0)
		await _shot("04b_chevrons_z80")   # zoom PROCHE : l'échelle/la casse des symboles se juge ici
	# 3) LES NOMS : plan moyen sur le centre de la carte
	_cam(map, float(w.map_w()) * 0.5, float(w.map_h()) * 0.5, 2.2)
	await _shot("05_noms_plan_moyen")
	print("MAP ART SHOTS OK — ", _dir)
	get_tree().quit()
