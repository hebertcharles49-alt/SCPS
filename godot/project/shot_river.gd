extends Node
## shot_river — capture ISO grand format centrée sur un FLEUVE MAJEUR (rivières + falaises +
## ombres portées + ombres d'assets). Probe de rendu, hors `make`. Seed 9 par défaut.
var _map: Node2D = null
func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map); _run.call_deferred()
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(120): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(6): await get_tree().process_frame
	var w = Sim.world
	var c := Vector2(512, 256)
	var foc := _focus_arg()
	if foc == "town":
		# centre = la plus grosse ville (vitrine des OMBRES d'assets)
		var best := -1; var bestpop := -1
		for r in range(w.region_count()):
			if w.region_tier(r) < 0: continue
			var p: int = w.region_pop(r)
			if p > bestpop: bestpop = p; best = r
		if best >= 0: c = w.region_centroid(best)
		print("focus town region=", best, " pop=", bestpop, " at=", c)
	elif foc == "mountain":
		# centre = la MAILLE 24×24 la plus DENSE en cellules FALAISE (18/19/23) → vrai massif (pas un barycentre noyé)
		var bio: Image = w.layer_image(2)
		var best := Vector2(512, 256); var bestn := -1
		if bio != null:
			var gw := bio.get_width(); var gh := bio.get_height()
			var cell := 24
			for gy in range(0, gh, cell):
				for gx in range(0, gw, cell):
					var n := 0
					for yy in range(gy, min(gy + cell, gh), 2):
						for xx in range(gx, min(gx + cell, gw), 2):
							var b := int(bio.get_pixel(xx, yy).r * 255.0 + 0.5)
							if b == 18 or b == 19 or b == 23: n += 1
					if n > bestn: bestn = n; best = Vector2(gx + cell * 0.5, gy + cell * 0.5)
		c = best
		print("focus mountain densest cell n=", bestn, " at=", c)
	elif foc == "flow":
		# centre = milieu du fleuve le plus FORT (le plus probable « circlé » par l'utilisateur)
		var fp: Array = w.river_paths()
		if not fp.is_empty():
			fp.sort_custom(func(a, b): return float(a["flow"]) > float(b["flow"]))
			var pts: PackedVector2Array = fp[0]["points"]
			c = pts[pts.size() / 2]
			print("focus FLOW river flow=", fp[0]["flow"], " mid=", c)
	else:
		# centre = milieu du fleuve dont la SOURCE est la plus HAUTE (la rivière qui vient de la MONTAGNE)
		var paths: Array = w.river_paths()
		var hi: Image = w.layer_image(0)   # HEIGHT
		if not paths.is_empty():
			var best := -1; var best_h := -1.0
			for i in range(paths.size()):
				var pp: PackedVector2Array = paths[i]["points"]
				if pp.size() < 20: continue
				var src: Vector2 = pp[0]
				var h := 0.0
				if hi != null: h = hi.get_pixel(clampi(int(src.x), 0, hi.get_width() - 1), clampi(int(src.y), 0, hi.get_height() - 1)).r
				if h > best_h: best_h = h; best = i
			if best < 0: best = 0
			var pts: PackedVector2Array = paths[best]["points"]
			c = pts[pts.size() / 2]
			print("focus MOUNTAIN river src_h=", best_h, " flow=", paths[best]["flow"], " mid=", c, " nrivers=", paths.size())
	var zoom: float = float(_zoom_arg())
	_map._enter_iso(c)
	_map._camera.zoom = Vector2(zoom, zoom)
	_map._camera.position = _map.iso_pos(c.x, c.y)
	for i in range(8): await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	print("view_mode=", _map.view_mode, " zoom=", _map._camera.zoom)
	var out: String = "res://" + _out_arg()
	_map.get_viewport().get_texture().get_image().save_png(out)
	print("SAVED ", out)
	get_tree().quit(0)
func _zoom_arg() -> float:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("zoom="): return float(a.substr(5))
	return 5.5
func _out_arg() -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("out="): return a.substr(4)
	return "river_shot.png"
func _focus_arg() -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("focus="): return a.substr(6)
	return "river"
