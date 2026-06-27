extends Node
## menu_audit — pendant headless du menu/Nouvelle-partie. Prouve, à travers la GDExtension
## LIVE, que (1) les scripts du shell compilent, (2) le binding des sliders + culture-par-slot
## existe et MORD (un grand monde a plus de régions ; une IA reçoit sa culture composée).
## Lancer : godot --headless res://menu_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("menu_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== MENU AUDIT — shell + nouvelle partie (binding live) ===")
	var viol := 0

	# INVARIANT 0 : les scripts du shell compilent (load() = null si parse cassé)
	for s in ["res://ui/menu_root.gd", "res://ui/new_game_panel.gd", "res://ui/culture_creator.gd"]:
		if load(s) == null:
			push_error("menu_audit: ne compile pas : " + s); viol += 1

	# INVARIANT 1 : le binding « nouvelle partie » existe
	for m in ["worldparams_default", "worldgen_set", "worldgen_clear", "set_empire_culture", "country_role"]:
		if not w.has_method(m):
			push_error("menu_audit: méthode absente du binding : " + m); viol += 1
	if viol > 0:
		print("MENU AUDIT : ", viol, " VIOLATION(S)"); get_tree().quit(1); return

	# INVARIANT 2 : worldparams_default peuplé
	var d: Dictionary = w.worldparams_default(9)
	print("  défauts : empires=", d.get("n_empires"), " continents=", d.get("n_continents"), " âge=", d.get("world_age"))
	if int(d.get("n_empires", 0)) <= 0: viol += 1; push_error("worldparams_default vide")

	# INVARIANT 3 : la TAILLE mord (grand monde ⇒ plus de régions)
	var small := d.duplicate(); small["n_empires"] = 2; small["n_city_states"] = 4
	w.worldgen_set(small); w.generate(9); var ra: int = w.region_count()
	var big := d.duplicate(); big["n_empires"] = 10; big["n_city_states"] = 20
	w.worldgen_set(big); w.generate(9); var rb: int = w.region_count()
	print("  régions : petit=", ra, " · grand=", rb)
	if rb <= ra: viol += 1; push_error("la taille ne mord pas")
	w.worldgen_clear()

	# INVARIANT 4 : culture par SLOT — joueur (slot 0) + IA (slot 1)
	w.clear_player_culture()
	# compo valide piochée
	var maj := -1; var mn := -1; var df := -1
	for t in w.tradition_list():
		var ax := int(t["axe"]); var rg := int(t["rang"])
		if maj < 0 and ax == 0 and rg >= 2: maj = int(t["id"])
		if mn  < 0 and ax == 1 and rg == 1: mn  = int(t["id"])
		if df  < 0 and ax == 2 and rg <  0: df  = int(t["id"])
	w.set_empire_culture(0, 0, 5, maj, mn, df)   # ESOTERIQUE / PACIFISTE → Havre
	w.set_empire_culture(1, 5, 0, maj, mn, df)   # CLANIQUE / DOMINATEUR → Horde
	w.generate(9)
	var me: int = w.player()
	var pname := String(w.country_info(me).get("nom", ""))
	var ai := -1
	for c in range(w.country_count()):
		if w.country_role(c) == 1: ai = c; break
	var aname := String(w.country_info(ai).get("nom", "")) if ai >= 0 else ""
	print("  slot 0 joueur=« ", pname, " » · slot 1 IA(cid ", ai, ")=« ", aname, " »")
	if not pname.begins_with("Havre"): viol += 1; push_error("joueur slot 0 non gravé : " + pname)
	if ai < 0 or not aname.begins_with("Horde"): viol += 1; push_error("IA slot 1 non gravée : " + aname)
	w.clear_player_culture()

	print("")
	print("MENU AUDIT OK" if viol == 0 else ("MENU AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)
