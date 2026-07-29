extends Node
## toponym_shot — probe DÉDIÉE à la TOPONYMIE DES VILLES (docs/DESIGN_TOPONYMIE_VILLES.md).
## Charge la scène RÉELLE (Main.tscn, motif map_art_shot.gd) pour vérifier l'intégration
## complète : bandeau de ville sur la carte (overlay.gd::_draw_banner), sidebar « VILLES »
## (empire_sidebar.gd) et l'onglet « Région » de la fiche province (province_panel_v2.gd).
## Génère un monde, avance assez d'années pour que le balayage annuel toponym_world_tick
## ait nommé les bourgs colonisés, puis :
##   1) N gros plans « zoom bourg » (zoom>=4.0, banner pleinement éclos) sur des villes
##      VARIÉES (héritages différents), un fichier par ville — pour lire le nom CADRÉ.
##   2) un plan large montrant PLUSIEURS bannières ensemble (cohérence de novlang par
##      culture, lisible dans une même scène).
##   3) la sidebar empire (section VILLES, droite de l'écran).
##   4) la fiche province, onglet « Région » (section VILLE ajoutée par cette mission).
##   Godot --audio-driver Dummy --path godot/project res://toponym_shot.tscn -- seed=9 years=30

const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _shot(nom: String) -> bool:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var path := OUTDIR + nom + ".png"
	var err := img.save_png(path)
	if err == OK:
		print("SAVED ", path, " (", img.get_width(), "x", img.get_height(), ")")
		return true
	push_error("save_png failed err=%d for %s" % [err, nom])
	return false

func _cam(map: Node, wx: float, wy: float, z: float) -> void:
	map._camera.zoom = Vector2(z, z)
	map._camera.position = map.iso_pos(wx, wy)
	map._nav_redraw()

func _run() -> void:
	var main: Node = load("res://main/Main.tscn").instantiate()
	add_child(main)
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	for i in range(30):
		await get_tree().process_frame
	if Sim.world == null:
		push_error("no world"); get_tree().quit(1); return
	var w = Sim.world
	for i in range(int(_arg("years=", "30"))):
		w.advance_days(360)
	Sim.generated.emit()
	var menu: Control = main._menu
	if menu != null:
		menu.hide()
	Sim.game_on = true
	Sim.speed_index = 0
	var map: Node = main.get_node_or_null("MapView")

	# mode POLITIQUE (pas nature) : on veut le lavis + les bannières de ville, pas le terrain nu.
	if map.has_method("is_nature") and map.is_nature():
		map.toggle_nature()

	var ok_all := true

	# ── recense les VILLES du monde (grain région, motif overlay.gd::_refresh_setts) :
	#    colonisée + ≥150 âmes (ou cité-état/peuple libre) + un nom de ville assigné. ──
	var me: int = int(w.player()) if w.has_method("player") else -1
	var fog: PackedByteArray = w.fog_region_mask() if w.has_method("fog_region_mask") else PackedByteArray()
	var villes := []   # [pop, region, heritage, nom, owner, visible]
	var n_reg: int = int(w.region_count())
	for r in range(n_reg):
		var tier: int = int(w.region_tier(r))
		var owner: int = int(w.region_owner(r))
		var role: int = int(w.country_role(owner)) if owner >= 0 else -1
		var pop: int = int(w.region_pop(r))
		if (tier < 0 or owner < 0 or pop < 150) and role != 2 and role != 4:
			continue
		var nm := String(w.region_city_name(r)) if w.has_method("region_city_name") else ""
		if nm == "":
			continue   # pas encore nommée cette année-là (repli région ailleurs, hors capture ici)
		var her := int(w.country_heritage(owner)) if w.has_method("country_heritage") else -1
		# VISIBLE au joueur : la sienne, ou DÉCOUVERTE (brouillard levé) — sinon la carte
		# reste noire sous ce siège (fog gate, overlay.gd::_fog_visible_region) et la
		# capture serait vide malgré un vrai nom.
		var visible := (owner == me) or (r < fog.size() and fog[r] != 0)
		villes.append([pop, r, her, nm, owner, visible])
	villes.sort_custom(func(a, b): return int(a[0]) > int(b[0]))
	print("TOPONYM villes nommées trouvées : ", villes.size(), " / ", n_reg, " régions (me=", me, ")")
	for v in villes:
		print("  région=", v[1], " pop=", v[0], " heritage=", v[2], " owner=", v[4],
			" visible=", v[5], " nom=", v[3])

	if villes.is_empty():
		push_error("aucune ville nommée trouvée — toponymie non intégrée ou balayage annuel pas encore passé")
		get_tree().quit(1)
		return

	# ── 1) GROS PLANS « zoom bourg » : jusqu'à 6 villes VISIBLES (jamais sous le
	#    brouillard — la capture serait noire) et VARIÉES (héritage différent en
	#    priorité) pour juger la cohérence de novlang par culture d'un coup d'œil. ──
	var vis: Array = villes.filter(func(v): return bool(v[5]))
	if vis.is_empty():
		push_error("aucune ville VISIBLE (toutes sous le brouillard) — avancer plus d'années ou élargir")
		vis = villes   # dernier repli : capture quand même (noir attendu), pour ne pas bloquer la probe
	var picked := []
	var seen_her := {}
	for v in vis:
		if picked.size() >= 6:
			break
		var her: int = int(v[2])
		if seen_her.has(her) and picked.size() < vis.size():
			continue
		seen_her[her] = true
		picked.append(v)
	if picked.size() < 6:
		for v in vis:
			if picked.size() >= 6:
				break
			if not picked.has(v):
				picked.append(v)

	var idx := 0
	for v in picked:
		var r: int = int(v[1])
		var seat: Vector2 = w.region_seat(r) if w.has_method("region_seat") else w.region_centroid(r)
		if seat.x < 0:
			continue
		_cam(map, seat.x, seat.y, 6.5)
		var fname := "toponym_%02d_%s" % [idx, String(v[3]).to_lower().replace(" ", "_")]
		ok_all = (await _shot(fname)) and ok_all
		idx += 1

	# ── 2) PLAN LARGE : plusieurs bannières visibles ensemble (zoom bourg minimal). ──
	if not vis.is_empty():
		var r0: int = int(vis[0][1])
		var seat0: Vector2 = w.region_seat(r0) if w.has_method("region_seat") else w.region_centroid(r0)
		_cam(map, seat0.x, seat0.y, 4.2)
		ok_all = (await _shot("toponym_ensemble_z42")) and ok_all

	# ── 3) SIDEBAR EMPIRE : section VILLES (empire_sidebar.gd, ajoutée par main.gd). ──
	_cam(map, float(w.map_w()) * 0.5, float(w.map_h()) * 0.5, 2.0)
	for i in range(4):
		await get_tree().process_frame
	ok_all = (await _shot("toponym_sidebar_villes")) and ok_all

	# ── 4) FICHE PROVINCE — onglet « Région » : la section VILLE ajoutée par cette
	#    mission (province_panel_v2.gd::_build_region), sur une province de la RÉGION
	#    la plus peuplée nommée. ──
	var r_top: int = int(vis[0][1])
	var seat_top: Vector2 = w.region_seat(r_top) if w.has_method("region_seat") else w.region_centroid(r_top)
	var pid := -1
	if seat_top.x >= 0 and w.has_method("province_at"):
		pid = int(w.province_at(int(seat_top.x), int(seat_top.y)))
	print("TOPONYM fiche province pid=", pid, " region=", r_top, " nom_attendu=", vis[0][3])
	if pid >= 0:
		var lay := CanvasLayer.new()
		add_child(lay)
		var panel: Control = load("res://ui/province_panel_v2.gd").new()
		lay.add_child(panel)
		for i in range(6):
			await get_tree().process_frame
		if panel.has_method("show_province"):
			panel.show_province(pid)
		for i in range(6):
			await get_tree().process_frame
		if panel.has_method("select_tab"):
			panel.select_tab(1)   # onglet « Région »
		for i in range(6):
			await get_tree().process_frame
		panel.reset_size()
		panel.position = Vector2(60, 60)
		ok_all = (await _shot("toponym_fiche_region")) and ok_all

	get_tree().quit(0 if ok_all else 1)
