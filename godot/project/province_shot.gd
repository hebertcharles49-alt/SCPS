extends Node
## province_shot — capture du PILOTE fiche province (province_panel_v2). Miroir de
## budget_shot : probe de rendu hors make, FENÊTRÉE (--headless = noir). Génère un monde,
## avance ~30 ans, choisit une province possédée par le joueur, instancie
## province_panel_v2, refresh, sauve un PNG 1600×900 centré.
##   Godot --path godot/project res://province_shot.tscn -- seed=9 years=30
const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"
const TABS := [
	[0, "province_infra.png"],
	[1, "province_region.png"],
	[2, "province_militaire.png"],
]
const CONSTRUCT_TABS := [
	[0, "construction_edifices.png"],
	[1, "construction_manufactures.png"],
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
	for i in range(int(_arg("years=", "30"))):
		Sim.world.advance_days(360)
	Sim.generated.emit()

	# une province possédée par le joueur, la PLUS DIVERSE (max de groupes) pour que
	# les frises culture/foi montrent plusieurs segments ; repli : 1re valide.
	var me: int = int(Sim.world.player()) if Sim.world.has_method("player") else 0
	var pid := -1
	var best_score := -1
	var n: int = int(Sim.world.province_count()) if Sim.world.has_method("province_count") else 0
	for p in range(n):
		var info: Dictionary = Sim.world.province_info(p)
		if not bool(info.get("valide", false)) or int(info.get("owner", -1)) != me:
			continue
		# la PLUS développée (manuf + édifices) pour voir les chips + [−][+], départage par diversité
		var nb: int = Sim.world.province_buildings(p).size() if Sim.world.has_method("province_buildings") else 0
		var ne: int = Sim.world.province_edifices(p).size() if Sim.world.has_method("province_edifices") else 0
		var ng: int = Sim.world.province_groups(p).size() if Sim.world.has_method("province_groups") else 0
		var score := (nb + ne) * 10 + ng
		if score > best_score:
			best_score = score
			pid = p
	if pid < 0:
		for p in range(n):
			if bool(Sim.world.province_info(p).get("valide", false)):
				pid = p
				break
	print("PROVINCE pid=", pid, " owner=me?", me, " score=", best_score)

	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)

	var lay := CanvasLayer.new()
	add_child(lay)
	var panel: Control = load("res://ui/province_panel_v2.gd").new()
	lay.add_child(panel)

	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.emit(Sim.world.year())
	for i in range(12):
		await get_tree().process_frame
	if panel.has_method("show_province"):
		panel.show_province(pid)
	for i in range(8):
		await get_tree().process_frame

	var ok_all := true
	for entry in TABS:
		var idx: int = int(entry[0])
		var fname: String = String(entry[1])
		if panel.has_method("select_tab"):
			panel.select_tab(idx)
		for i in range(6):
			await get_tree().process_frame
		panel.reset_size()
		panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()
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
		if idx == 0:
			# chantier 5 — le HOVER biome (image + détail) n'apparaît qu'au survol natif
			# (TooltipServer, un minuteur réel) : on le force manuellement pour PROUVER
			# que `_make_custom_tooltip` construit bien la carte, sans dépendre du timing.
			var terrain_row: Node = panel._body.get_child(0) if panel._body.get_child_count() > 0 else null
			if terrain_row != null and terrain_row.has_method("_make_custom_tooltip"):
				var tip: Control = terrain_row._make_custom_tooltip("")
				if tip != null:
					lay.add_child(tip)
					tip.position = Vector2(700, 200)
					for i in range(4):
						await get_tree().process_frame
					await RenderingServer.frame_post_draw
					await RenderingServer.frame_post_draw
					var himg := get_viewport().get_texture().get_image()
					var hpath := OUTDIR + "province_biome_hover.png"
					var herr := himg.save_png(hpath)
					if herr == OK:
						print("SAVED ", hpath, " (", himg.get_width(), "x", himg.get_height(), ")")
					else:
						push_error("save_png failed err=%d for province_biome_hover.png" % herr)
						ok_all = false
					tip.queue_free()
				else:
					push_error("_make_custom_tooltip a renvoyé null")
					ok_all = false

	# le MENU CONSTRUCTION (« Construire… ») — ouvert sur la province choisie, les
	# deux onglets (édifices + manufactures), pour vérifier la carte-par-bâtiment.
	var construct: Control = load("res://ui/construction_panel.gd").new()
	lay.add_child(construct)
	construct.target_pid = pid
	for entry in CONSTRUCT_TABS:
		var cidx: int = int(entry[0])
		var cfname: String = String(entry[1])
		if construct.has_method("open_on"):
			construct.open_on(cidx)
		for i in range(6):
			await get_tree().process_frame
		construct.position = Vector2(200, 60)
		for i in range(4):
			await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var cimg := get_viewport().get_texture().get_image()
		var cpath := OUTDIR + cfname
		var cerr := cimg.save_png(cpath)
		if cerr == OK:
			print("SAVED ", cpath, " (", cimg.get_width(), "x", cimg.get_height(), ")")
		else:
			push_error("save_png failed err=%d for %s" % [cerr, cfname])
			ok_all = false
	get_tree().quit(0 if ok_all else 1)
