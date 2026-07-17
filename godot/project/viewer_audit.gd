extends Node
## viewer_audit — « VIEWER CHRONICLE » : le pendant headless de `chronicle`, mais pour le FRONT-END.
##
## Le moteur a `chronicle` (télémétrie = preuve d'équilibre) et `make fx-proof` (compositing FX headless).
## Le FRONT-END (placement : décor, bourg, routes, ponts) n'avait RIEN — on le vérifiait à l'œil, sur des
## rendus lents. Les bugs (pavé dans le fleuve, décor dans l'eau, pont hors fleuve) ne sortaient QUE par
## hasard. Ce probe REND la donnée de placement et ASSERTE les invariants de membrane, sans regarder un
## pixel. Sort ≠ 0 si un invariant casse → gate de non-régression. Déterministe (seed → scène figée).
##
## Lancer : godot --headless res://viewer_audit.tscn -- seed=9,11,42 years=120
var _map: Node2D = null

func _ready() -> void:
	get_window().size = Vector2i(640, 480)
	_map = load("res://map/map_view.gd").new()
	_map.name = "MapView"
	add_child(_map)
	_run.call_deferred()

func _seeds() -> Array:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("seed="):
			var out: Array = []
			for s in a.substr(5).split(","):
				out.append(int(s))
			return out
	return [9]

func _years() -> int:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("years="):
			return int(a.substr(6))
	return 120

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("viewer_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== VIEWER AUDIT — invariants de placement du front-end ===")
	var total := 0
	for sd in _seeds():
		total += await _audit_seed(sd, _years())
	print("")
	print("VIEWER AUDIT OK" if total == 0 else ("VIEWER AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	Sim.generated.emit()                 # le front-end rebâtit décor/structures/routes sur le monde âgé
	for _f in range(8):
		await get_tree().process_frame
	# ANCIEN _enter_iso (globe→iso) DISPARU depuis l'unification ISO unique — on pose la caméra
	# directement (même geste que shot_parch.gd) à un zoom de lecture (routes/décor/bourgs actifs).
	_map._camera.position = _map.iso_pos(w.map_w() * 0.5, w.map_h() * 0.5)
	_map._camera.zoom = Vector2(4.0, 4.0)
	_map.queue_redraw()
	for _f2 in range(3):
		await get_tree().process_frame
	var ov = _map.get_node_or_null("Overlay")
	if ov == null:
		push_error("viewer_audit: pas d'Overlay")
		return 99
	var sea: Image = w.layer_image(4)    # SCPS_LAYER_WATER (mer/lac)
	var rf: Image = ov._carved_river_field()

	# INVARIANT 1-2 : aucun ASSET (décor, bâti) avec sa BASE dans l'eau (mer/lac OU fleuve carvé).
	var decor_water := 0
	for d in ov._decor:
		var p: Vector2 = d["pos"]
		if ov._is_sea_cell(sea, int(p.x), int(p.y)) or ov._in_river_water(rf, int(p.x), int(p.y)):
			decor_water += 1
	var struct_water := 0
	for s in ov._structures:
		var q: Vector2 = s["pos"]
		if ov._is_sea_cell(sea, int(q.x), int(q.y)) or ov._in_river_water(rf, int(q.x), int(q.y)):
			struct_water += 1

	# INVARIANT 3 : la ROUTE ne court pas sur la MER (l'A* moteur la contourne) ; sur FLEUVE = franchissement.
	var road_cells := {}
	for rd in ov._roads:
		for pt in (rd["points"] as PackedVector2Array):
			road_cells[Vector2i(int(pt.x), int(pt.y))] = true
	var road_sea := 0
	var road_river := 0
	for ck in road_cells.keys():
		var c: Vector2i = ck
		if ov._is_sea_cell(sea, c.x, c.y):
			road_sea += 1
		elif ov._in_river_water(rf, c.x, c.y):
			road_river += 1

	# Télémétrie PONTS : nombre de franchissements détectés (ov._ink_bridges, peuplé par
	# _ensure_roads()/_augment_roads() DÉJÀ appelé via le signal Sim.generated ci-dessus —
	# ancien _build_bridges/_bridges disparus avec la réécriture en ponts d'ENCRE vectoriels).
	var bridges: int = ov._ink_bridges.size()

	# TÉLÉMÉTRIE SPAGHETTI (mission ANTISPAG A1/A3) : segments de tracé qui ont encore un voisin
	# proche-et-parallèle appartenant à une AUTRE route sans être fusionnés — la mesure du retour
	# joueur « faisceaux de routes parallèles/redondantes ». Voir overlay.gd::_count_spaghetti_segments.
	var spag: int = ov._count_spaghetti_segments()

	var viol := 0
	var flags := ""
	# Invariants DURS = assets RENDUS dans l'eau (la membrane : on ne pose rien sur l'eau).
	if decor_water > 0:
		viol += 1
		flags += " ✗decor-eau(" + str(decor_water) + ")"
	if struct_water > 0:
		viol += 1
		flags += " ✗struct-eau(" + str(struct_water) + ")"
	# Info (pas un échec viewer) : cellules de TRACÉ sur la mer — le shader ne DESSINE pas la route
	# sur l'eau (gate !water_here) ; c'est un signal sur le PATHING moteur, pas un rendu fautif.
	if road_sea > 0:
		flags += " ⚠route-mer-path(" + str(road_sea) + ")"
	# Info (pas un échec viewer — pas de seuil « zéro » assumé) : la métrique ANTISPAG, comparée
	# AVANT/APRÈS entre deux runs de cette même sonde, pas un invariant dur en soi.
	if spag > 0:
		flags += " ⚠spaghetti(" + str(spag) + ")"
	print("seed ", sd, " an ", w.year(),
		" | decor ", ov._decor.size(), " | struct ", ov._structures.size(),
		" | route ", road_cells.size(), "c (fleuve ", road_river, ")",
		" | ponts ", bridges,
		(flags if flags != "" else " | OK"))
	return viol
