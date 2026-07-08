extends Node
## shot_parch — capture de la carte PARCHEMIN (nouveau rendu unique). Probe de rendu, hors make.
## Lancer (fenêtré ; --headless donne du noir) :
##   Godot --path godot/project res://shot_parch.tscn -- seed=9 years=0 zoom=0 out=parch.png
##   zoom=0 → fit (carte entière) ; zoom>0 → cadrage à (cx,cy) monde.
var _map: Node2D = null
func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map)
	_run.call_deferred()
func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p): return a.substr(p.length())
	return d
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(int(_arg("years=", "0"))): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(8): await get_tree().process_frame
	# mode NATURE (nature=1) : carte vierge — terrain + dressing seuls, sans frontières/villes.
	if _arg("nature=", "0") == "1":
		var ov := _map.get_node_or_null("Overlay")
		if ov != null:
			ov.nature_mode = true
			ov.queue_redraw()
	var zoom := float(_arg("zoom=", "0"))
	# cap=1 : centre sur la CAPITALE DU JOUEUR (évite de deviner des coordonnées monde)
	var def_cx: String = str(Sim.world.map_w() * 0.5)
	var def_cy: String = str(Sim.world.map_h() * 0.5)
	if _arg("cap=", "0") == "1":
		var me: int = Sim.world.player()
		var capr: int = Sim.world.province_region(Sim.world.country_capital_province(me))
		if capr >= 0:
			var cc: Vector2 = Sim.world.region_centroid(capr)
			def_cx = str(cc.x)
			def_cy = str(cc.y)
	if _arg("cs=", "0") == "1":   # centre sur la 1re CITÉ-ÉTAT (enceinte, marché)
		for c in range(Sim.world.country_count()):
			if int(Sim.world.country_role(c)) == 2:
				var csr: int = Sim.world.province_region(Sim.world.country_capital_province(c))
				if csr >= 0:
					var c2: Vector2 = Sim.world.region_centroid(csr)
					def_cx = str(c2.x)
					def_cy = str(c2.y)
					break
	if _arg("wild=", "0") == "1":   # centre sur le 1er HAMEAU LIBRE (rôle 4 — vignette bourg_wild)
		for c in range(Sim.world.country_count()):
			if int(Sim.world.country_role(c)) == 4:
				var wr := -1
				for r in range(Sim.world.region_count()):
					if int(Sim.world.region_owner(r)) == c:
						wr = r
						break
				if wr >= 0:
					var c3: Vector2 = Sim.world.region_centroid(wr)
					def_cx = str(c3.x)
					def_cy = str(c3.y)
					break
	var cx := float(_arg("cx=", def_cx))
	var cy := float(_arg("cy=", def_cy))
	if zoom > 0.0:
		_map._camera.zoom = Vector2(zoom, zoom)
		_map._camera.position = _map.iso_pos(cx, cy)
	else:
		_map.fit()
	# sel=N force la PROVINCE SÉLECTIONNÉE ; sel=auto la résout au centre (cx,cy) —
	# vérifie le CONTOUR DORÉ de sélection (province_border_segments) sans souris.
	var sarg := _arg("sel=", "")
	if sarg != "":
		var selp: int = Sim.world.province_at(int(cx), int(cy)) if sarg == "auto" else int(sarg)
		if selp >= 0:
			_map._selected_prov = selp
			var ov2 := _map.get_node_or_null("Overlay")
			if ov2 != null:
				ov2.queue_redraw()
	# le monde est en PAUSE (aucun tick ne requeue le redraw) : on RE-POSE la caméra et on
	# FORCE le redraw À CHAQUE frame d'attente — sinon un draw RETENU (fait avant que la
	# caméra n'applique son transform, sous les seuils dressing/villes) persiste et le
	# shot sort « nu » (flake observé, dépendant de l'ordre caméra/draw dans la frame).
	var ov3 := _map.get_node_or_null("Overlay")
	var want_zoom := zoom
	for i in range(8):
		if want_zoom > 0.0:
			_map._camera.zoom = Vector2(want_zoom, want_zoom)
			_map._camera.position = _map.iso_pos(cx, cy)
		if ov3 != null:
			ov3.queue_redraw()
		_map.queue_redraw()
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var out: String = "res://" + _arg("out=", "parch.png")
	_map.get_viewport().get_texture().get_image().save_png(out)
	print("SAVED ", out)
	get_tree().quit(0)
