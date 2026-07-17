extends Node
## ROUTES PERF PROBE — mesure le coût de `w.road_paths()` (A* moteur, mission ROUTES R2) :
## COLD (1re construction, cache invalide) vs WARM (signature inchangée → cache C, mais la
## copie GDScript/Dictionary reste à payer) vs REBUILD (signature changée par la colonisation
## qui avance → nouvelle construction complète). Fenêtré (--headless donne du noir, cf. piège
## documenté) : Godot --path godot/project res://routes_perf_probe.tscn -- seed=9 years=250

func _ready() -> void:
	get_window().size = Vector2i(800, 600)
	_run.call_deferred()

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p): return a.substr(p.length())
	return d

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	var seed := int(_arg("seed=", "9"))
	var years := int(_arg("years=", "250"))
	Sim.regenerate(seed)
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world"); get_tree().quit(1); return
	for i in range(years):
		Sim.world.advance_days(360)
	print("MONDE seed=%d years=%d" % [seed, years])

	var t0 := Time.get_ticks_usec()
	var roads: Array = Sim.world.road_paths()
	var cold_ms := (Time.get_ticks_usec() - t0) / 1000.0
	var npts := 0
	for rd in roads:
		npts += (rd["points"] as PackedVector2Array).size()
	print("COLD  routes=%d points=%d temps=%.2fms" % [roads.size(), npts, cold_ms])

	var t1 := Time.get_ticks_usec()
	var roads2: Array = Sim.world.road_paths()
	var warm_ms := (Time.get_ticks_usec() - t1) / 1000.0
	print("WARM  routes=%d temps=%.2fms (signature inchangée — cache C hit)" % [roads2.size(), warm_ms])

	# REBUILD : fait avancer le monde (colonisation/pertes bougent la signature) → cache invalidé.
	for i in range(20):
		Sim.world.advance_days(360)
	var t2 := Time.get_ticks_usec()
	var roads3: Array = Sim.world.road_paths()
	var rebuild_ms := (Time.get_ticks_usec() - t2) / 1000.0
	print("REBUILD routes=%d temps=%.2fms (20 ans plus tard — signature changée)" % [roads3.size(), rebuild_ms])

	get_tree().quit(0)
