extends Node
## road_dump_probe — DIAGNOSTIC TEMPORAIRE (mission ANTISPAG, pas un livrable) : sérialise le
## réseau de routes (ov._roads, post `_augment_roads`) en JSON pour analyse hors-Godot (Python,
## sans dépendance) — comprendre la GÉOMÉTRIE réelle du « spaghetti » avant de choisir un seuil.
## Lancer (fenêtré) : Godot --path godot/project res://road_dump_probe.tscn -- seed=9 years=120 out=roads.json
var _map: Node2D = null
func _ready() -> void:
	get_window().size = Vector2i(640, 480)
	_map = load("res://map/map_view.gd").new(); _map.name = "MapView"; add_child(_map)
	_run.call_deferred()
func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p): return a.substr(p.length())
	return d
func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	if Sim.world == null: push_error("no world"); get_tree().quit(1); return
	for i in range(int(_arg("years=", "120"))): Sim.world.advance_days(360)
	Sim.generated.emit()
	for i in range(8): await get_tree().process_frame
	var ov = _map.get_node_or_null("Overlay")
	if ov == null: push_error("no overlay"); get_tree().quit(1); return
	ov._ensure_roads()
	var out := []
	for rd in ov._roads:
		var pts: PackedVector2Array = rd["points"]
		var arr := []
		for p in pts:
			arr.append([p.x, p.y])
		out.append({"level": rd.get("level", 1), "key": rd.get("key", -1), "pts": arr})
	var f := FileAccess.open("res://" + _arg("out=", "roads.json"), FileAccess.WRITE)
	f.store_string(JSON.stringify(out))
	f.close()
	print("SAVED ", out.size(), " routes")
	# MARITIME N4 : le même dump pour les LANES (ov._lanes, post _augment_lanes) + la
	# visibilité fog des deux bouts — diagnostic du spaghetti MARIN et du cadrage probe.
	ov._ensure_lanes()
	var lout := []
	for ln in ov._lanes:
		var lpts: PackedVector2Array = ln["points"]
		var larr := []
		for p in lpts:
			larr.append([p.x, p.y])
		lout.append({"open": int(ln.get("open", 0)), "choke": int(ln.get("choke", -1)),
			"ra": int(ln.get("ra", -1)), "rb": int(ln.get("rb", -1)),
			"fog_ra": ov._fog_visible_region(int(ln.get("ra", -1))),
			"fog_rb": ov._fog_visible_region(int(ln.get("rb", -1))),
			"pts": larr})
	var lf := FileAccess.open("res://" + _arg("lanes=", "lanes.json"), FileAccess.WRITE)
	lf.store_string(JSON.stringify(lout))
	lf.close()
	print("SAVED ", lout.size(), " lanes")
	get_tree().quit(0)
