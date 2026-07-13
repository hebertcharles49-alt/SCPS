extends Node
## province_shot — capture du PILOTE fiche province (province_panel_v2). Miroir de
## budget_shot : probe de rendu hors make, FENÊTRÉE (--headless = noir). Génère un monde,
## avance ~30 ans, choisit une province possédée par le joueur, instancie
## province_panel_v2, refresh, sauve un PNG 1600×900 centré.
##   Godot --path godot/project res://province_shot.tscn -- seed=9 years=30
const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"
const TABS := [
	[0, "province_infra.png"],
	[1, "province_militaire.png"],
	[2, "province_demo.png"],
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
	var best_groups := -1
	var n: int = int(Sim.world.province_count()) if Sim.world.has_method("province_count") else 0
	for p in range(n):
		var info: Dictionary = Sim.world.province_info(p)
		if not bool(info.get("valide", false)) or int(info.get("owner", -1)) != me:
			continue
		var ng: int = Sim.world.province_groups(p).size() if Sim.world.has_method("province_groups") else 0
		if ng > best_groups:
			best_groups = ng
			pid = p
	if pid < 0:
		for p in range(n):
			if bool(Sim.world.province_info(p).get("valide", false)):
				pid = p
				break
	print("PROVINCE pid=", pid, " owner=me?", me, " groups=", best_groups)

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
	get_tree().quit(0 if ok_all else 1)
