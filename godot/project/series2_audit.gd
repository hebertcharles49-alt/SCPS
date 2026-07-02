extends Node
## series2_audit — probe du CHROME SÉRIE 2 : topbar (médaillons fins) · panneau
## province (bande de biome) · tiroir Conseil (bustes) · sidebar (blasons de
## faction · bateau de flotte) · popup OYEZ (tampon + filigrane) · arbre tech
## (médaillons). Fenêtré : Godot --path godot/project res://series2_audit.tscn
func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(9)
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	for i in range(30):
		Sim.world.advance_days(360)
	Sim.game_on = true
	var w = Sim.world
	var me: int = w.player()

	var ui := CanvasLayer.new()
	add_child(ui)
	var th = load("res://ui/ui_theme.gd").build()

	# — topbar (sablier + pièce/épi/plume fins) —
	var tb = load("res://ui/topbar.gd").new()
	tb.theme = th
	ui.add_child(tb)

	# — panneau province : une province du joueur (bande de biome) —
	var pp = load("res://ui/province_panel.gd").new()
	pp.theme = th
	ui.add_child(pp)
	var pid := -1
	for p in range(w.province_count()):
		var inf: Dictionary = w.province_info(p)
		if bool(inf.get("valide", false)) and int(inf.get("owner", -2)) == me:
			pid = p
			break
	if pid >= 0:
		var inf2: Dictionary = w.province_info(pid)
		print("PROV ", pid, " climat=", inf2.get("climat"), " relief=", inf2.get("relief"))
		pp.show_province(pid)

	# — tiroir : onglet CONSEIL (bustes planche 13) —
	var dr = load("res://ui/sidebar_drawer.gd").new()
	dr.theme = th
	ui.add_child(dr)
	if dr.has_method("show_tab"):
		dr.show_tab(7)

	# — sidebar d'empire (blasons de faction + bateau de flotte) —
	var sb = load("res://ui/empire_sidebar.gd").new()
	sb.theme = th
	ui.add_child(sb)

	# — popup OYEZ : une révolte forgée (tampon flamme + filigrane) —
	var ep = load("res://ui/event_popup.gd").new()
	ep.theme = th
	ui.add_child(ep)
	ep.enqueue({"title": "RÉVOLTE !", "kind": 6,
		"body": "Un soulèvement éclate — la garnison réclame des ordres. Le tampon de flamme et le filigrane de rosace doivent marquer ce parchemin.",
		"buttons": [{"label": "Vu", "act": "close"}]})

	Sim.ticked.emit(w.year())
	for i in range(8):
		tb.queue_redraw(); pp.queue_redraw(); sb.queue_redraw(); dr.queue_redraw()
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://series2_a.png")
	print("SAVED A (topbar+province+conseil+sidebar+popup)")

	# — 2e cadre : l'ARBRE TECH seul (médaillons par nom/quartier/tier) —
	ep.visible = false
	pp.visible = false
	dr.visible = false
	var tp = load("res://ui/tech_panel.gd").new()
	tp.theme = th
	ui.add_child(tp)
	tp.show()   # visibility_changed → _build()
	for i in range(8):
		tp.queue_redraw()
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://series2_b.png")
	print("SAVED B (tech)")
	get_tree().quit(0)
