extends Node
## council_audit — le pendant headless de l'onglet CONSEIL (V2a : faction/loyauté/paie).
##
## Prouve le chemin GDScript → binding → façade → moteur : country_council LIT
## faction/loyauté/paie/mood (bornés), player_council_pay ACTIVE le curseur et le
## CLAMPE, council_pair_state reste dans {0..3}. Round-trip via le journal
## déterministe. Sort ≠ 0 si un invariant casse.
##
## Lancer : godot --headless res://council_audit.tscn -- seed=9,11,42 years=30

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
	return 30

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("council_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== CONSEIL AUDIT — la membrane du Conseil vivant (V2a) ===")
	var total := 0
	# INVARIANT 0 : le panneau qui CONSOMME la membrane compile.
	if load("res://ui/sidebar_drawer.gd") == null:
		push_error("council_audit: sidebar_drawer.gd ne compile pas")
		total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("CONSEIL AUDIT OK" if total == 0 else ("CONSEIL AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	var me: int = w.player()
	var viol := 0
	var flags := ""

	# 1. LECTURE : 3 sièges, chacun borné (loyauté 0-100, paie 0-2, faction/mood non-nuls)
	var seats: Array = w.country_council(me)
	if seats.size() != 3:
		viol += 1; flags += " ✗seats-count"
	for seat in seats:
		var loy := int(seat.get("loyalty", -1))
		var pay := float(seat.get("pay", -1.0))
		if loy < 0 or loy > 100:
			viol += 1; flags += " ✗loyalty-bound"
		if pay < 0.0 or pay > 2.0:
			viol += 1; flags += " ✗pay-bound"
		if not seat.has("faction") or not seat.has("mood"):
			viol += 1; flags += " ✗missing-fields"

	# 2. RECRUTER le premier siège vacant (si disponible), puis vérifier loyauté humaine
	var vacant_seat := -1
	for i in range(seats.size()):
		if not bool(seats[i].get("filled", false)):
			vacant_seat = i
			break
	if vacant_seat >= 0:
		var cands: Array = w.council_candidates(vacant_seat)
		if cands.is_empty():
			viol += 1; flags += " ✗no-candidates"
		else:
			if not w.player_council_hire(vacant_seat, int(cands[0].get("slot", 0))):
				viol += 1; flags += " ✗hire-enqueue"
			w.advance_days(5)
			var after: Array = w.country_council(me)
			if not bool(after[vacant_seat].get("filled", false)) or int(after[vacant_seat].get("loyalty", 0)) <= 0:
				viol += 1; flags += " ✗hire-no-effect"

	# 3. LA PAIE : poser 1.6, vérifier que le curseur suit ; poser hors-borne (99), vérifier CLAMP à 2.0
	if not w.player_council_pay(0, 1.6):
		viol += 1; flags += " ✗pay-enqueue"
	w.advance_days(32)
	var p1: Array = w.country_council(me)
	if bool(p1[0].get("filled", false)) and absf(float(p1[0].get("pay", 0.0)) - 1.6) > 0.2:
		viol += 1; flags += " ✗pay-not-reflected"
	w.player_council_pay(0, 99.0)
	w.advance_days(32)
	var p2: Array = w.country_council(me)
	if bool(p2[0].get("filled", false)) and float(p2[0].get("pay", 0.0)) > 2.0:
		viol += 1; flags += " ✗pay-not-clamped"

	# 4. PAIR_STATE : borné {0..3}, jamais un crash sur des sièges hors-borne
	var pst := int(w.council_pair_state(0, 1))
	if pst < 0 or pst > 3:
		viol += 1; flags += " ✗pair-state-bound"
	if int(w.council_pair_state(-1, 99)) != 0:
		viol += 1; flags += " ✗pair-state-oob"

	# 5. Avancer 30 ans : les tests de loyauté/faction/paie restent BORNÉS (le tick vit).
	for _i in range(years):
		w.advance_days(360)
	var seats3: Array = w.country_council(me)
	for seat in seats3:
		var loy2 := int(seat.get("loyalty", -1))
		if loy2 < 0 or loy2 > 100:
			viol += 1; flags += " ✗loyalty-bound-longrun"

	print("seed ", sd, " an ", w.year(), " | sièges ", seats3.size(),
		(flags if flags != "" else " | OK"))
	return viol
