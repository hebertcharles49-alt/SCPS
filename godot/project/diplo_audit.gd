extends Node
## diplo_audit — le pendant headless de la membrane DIPLOMATIE (le binding §3).
##
## viewer_audit prouve le PLACEMENT ; ce probe prouve les VERBES DU JOUEUR : que
## l'opinion #26 traverse bornée, que scps_diplo_options est cohérent (jamais guerre
## ET paix offrables ensemble), et — le round-trip — qu'un verbe (déclarer la guerre)
## MUTE vraiment le monde via le journal déterministe. Sort ≠ 0 si un invariant casse.
##
## Lancer : godot --headless res://diplo_audit.tscn -- seed=9,11,42 years=80

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
	return 80

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("diplo_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== DIPLO AUDIT — membrane des verbes joueur (§3) ===")
	var total := 0
	# INVARIANT 0 : le panneau (qui CONSOMME la membrane) compile — un headless ne dessine
	# pas, mais load() échoue (null + Parse Error) si le script est cassé.
	if load("res://ui/sidebar_drawer.gd") == null:
		push_error("diplo_audit: sidebar_drawer.gd ne compile pas")
		total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("DIPLO AUDIT OK" if total == 0 else ("DIPLO AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	var me: int = w.player()
	var nc: int = w.country_count()
	var viol := 0
	var flags := ""

	var rels: Array = w.country_relations(me)
	var seen := {}
	# INVARIANT 1 : opinion bornée [-100,100] · index pays valide & unique · jamais soi.
	for rel in rels:
		var op: int = int(rel["opinion"])
		var tgt: int = int(rel["country"])
		if op < -100 or op > 100:
			viol += 1; flags += " ✗opinion(" + str(op) + ")"
		if tgt < 0 or tgt >= nc or tgt == me or seen.has(tgt):
			viol += 1; flags += " ✗index(" + str(tgt) + ")"
		seen[tgt] = true

	# INVARIANT 2 : options COHÉRENTES — jamais guerre ET paix offrables ; en guerre ⇒
	# paix offrable et déclaration grisée ; embargo poser XOR lever ; consentements ∈ {bool}.
	var war_target := -1
	for rel in rels:
		var tgt: int = int(rel["country"])
		var o: Dictionary = w.diplo_options(tgt)
		if not bool(o["valid"]):
			viol += 1; flags += " ✗opt-invalid(" + str(tgt) + ")"
			continue
		var cw: bool = bool(o["can_declare_war"])
		var cp: bool = bool(o["can_make_peace"])
		if cw and cp:
			viol += 1; flags += " ✗guerre+paix(" + str(tgt) + ")"
		if bool(rel["at_war"]):
			if cw or not cp:
				viol += 1; flags += " ✗enguerre(" + str(tgt) + ")"
		if bool(o["can_embargo"]) and bool(o["can_lift_embargo"]):
			viol += 1; flags += " ✗embargo±(" + str(tgt) + ")"
		# cible candidate pour le round-trip : un voisin PAS en guerre, déclaration permise
		if war_target < 0 and cw and not bool(rel["at_war"]):
			war_target = tgt

	# INVARIANT 3 : ROUND-TRIP — déclarer la guerre MUTE le monde (le verbe traverse le
	# journal déterministe et le drain l'applique ; on re-lit l'état au tick suivant).
	if war_target >= 0:
		var ok: bool = w.player_declare_war(war_target)
		if not ok:
			viol += 1; flags += " ✗war-enqueue"
		w.advance_days(2)   # franchit le drain (sim_cmd_drain dans sim_day) → applique l'ordre
		var now_war := false
		for rel in w.country_relations(me):
			if int(rel["country"]) == war_target and bool(rel["at_war"]):
				now_war = true
				break
		if not now_war:
			viol += 1; flags += " ✗war-no-effect(" + str(war_target) + ")"

	print("seed ", sd, " an ", w.year(), " | pays ", nc, " | relations ", rels.size(),
		" | cible guerre ", war_target, (flags if flags != "" else " | OK"))
	return viol
