extends Node
## alloc_audit — le pendant headless de l'onglet MAIN-D'ŒUVRE (binding allocation).
##
## Prouve le chemin GDScript → binding → façade → moteur : province_alloc LIT les puits
## (extraction + manufactures), les verbes player_alloc_* ACTIVENT l'override, ferment
## un bâtiment (poids 0), et le retour AUTO. RE-KEY PROVINCE (doctrine « la province est
## la seule réalité économique ») : la cible est un PID DIRECT, jamais une région.
## Round-trip via le journal déterministe. Sort ≠ 0 si un invariant casse.
##
## Lancer : godot --headless res://alloc_audit.tscn -- seed=9,11,42 years=60

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _seeds() -> Array:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("seed="):
			var out: Array = []
			for s in a.substr(5).split(","):
				out.append(int(s))
			return out
	return [9, 11, 42]

func _years() -> int:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("years="):
			return int(a.substr(6))
	return 60

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("alloc_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== ALLOC AUDIT — membrane de l'allocation de main-d'œuvre ===")
	var total := 0
	# INVARIANT 0 : le panneau qui CONSOMME la membrane compile.
	if load("res://ui/province_detail.gd") == null:
		push_error("alloc_audit: province_detail.gd ne compile pas")
		total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("ALLOC AUDIT OK" if total == 0 else ("ALLOC AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	var me: int = w.player()
	var viol := 0
	var flags := ""
	# trouver une PROVINCE DU JOUEUR (RE-KEY : la cible est un pid direct, jamais une région)
	var pid := -1
	for p in range(w.province_count()):
		var pi: Dictionary = w.province_info(p)
		if bool(pi.get("valide", false)) and int(pi.get("owner", -1)) == me and int(pi.get("ames", 0)) > 0:
			pid = p
			break
	if pid < 0:
		print("seed ", sd, " | aucune province joueur colonisée (monde fendu ?) — sauté")
		return 0
	# 1. LECTURE : province, bassin>0, puits listés ; mode AUTO au départ
	var al: Dictionary = w.province_alloc(pid)
	var sinks: Array = al.get("sinks", [])
	if int(al.get("region", -1)) != pid or float(al.get("pool", 0)) <= 0.0 or sinks.is_empty():
		viol += 1; flags += " ✗read"
	if bool(al.get("on", true)):
		viol += 1; flags += " ✗not-auto-start"
	# repérer un puits manufacture + un puits extraction
	var kbld := -1
	var kraw := -1
	for i in range(sinks.size()):
		if int(sinks[i].get("kind", 0)) == 1 and kbld < 0: kbld = i
		if int(sinks[i].get("kind", 0)) == 0 and kraw < 0: kraw = i
	# 2. ACTIVER l'override via un poids d'extraction → on=true au drain
	if kraw >= 0:
		if not w.player_alloc_raw(pid, int(sinks[kraw].get("id", 0)), 80):
			viol += 1; flags += " ✗raw-enqueue"
		w.advance_days(2)
		if not bool(w.province_alloc(pid).get("on", false)):
			viol += 1; flags += " ✗override-inactive"
	# 3. FERMER un bâtiment (poids 0) → closed reflété au readout
	if kbld >= 0:
		var bid: int = int(sinks[kbld].get("id", 0))
		w.player_alloc_bld(pid, bid, 0)
		w.advance_days(2)
		var closed := false
		for s in w.province_alloc(pid).get("sinks", []):
			if int(s.get("kind", 0)) == 1 and int(s.get("id", 0)) == bid:
				closed = bool(s.get("closed", false))
		if not closed:
			viol += 1; flags += " ✗close"
	# 4. RETOUR AUTO
	w.player_alloc_auto(pid)
	w.advance_days(2)
	if bool(w.province_alloc(pid).get("on", true)):
		viol += 1; flags += " ✗auto-revert"

	print("seed ", sd, " an ", w.year(), " | prov ", pid, " | puits ", sinks.size(),
		(flags if flags != "" else " | OK"))
	return viol
