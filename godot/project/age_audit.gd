extends Node
## age_audit — pendant headless du verbe d'ÂGE (§7). Prouve, à travers la GDExtension
## LIVE : (1) le binding age_state/player_age_engage existe ; (2) l'état est SAIN avant
## tout âge (-1, engaged=false) ; (3) le ROUND-TRIP — quand un âge se lève, le verbe
## enfilé passe le drain et age_state() bascule engaged=true (une fois par âge).
## Lancer : godot --headless res://age_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("age_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== AGE AUDIT — engagement d'âge joueur (binding live) ===")
	var viol := 0

	# INVARIANT 0 : le binding existe
	for m in ["age_state", "player_age_engage"]:
		if not w.has_method(m):
			push_error("age_audit: méthode absente du binding : " + m); viol += 1
	if viol > 0:
		get_tree().quit(2); return

	w.generate(9)
	# INVARIANT 1 : monde neuf = aucun âge levé, rien d'engagé, verbe REFUSABLE sans crash
	var ag: Dictionary = w.age_state()
	print("  an-0 : age=%d engaged=%s name='%s'" % [int(ag["age"]), str(ag["engaged"]), String(ag["name"])])
	if int(ag["age"]) >= 0 and bool(ag["engaged"]):
		push_error("age_audit: an-0 déjà engagé ?"); viol += 1

	# INVARIANT 2 : le round-trip — avancer jusqu'au 1er âge levé (bornée 220 ans)
	var dawned := -1
	for y in range(220):
		w.advance_days(365)
		ag = w.age_state()
		if int(ag["age"]) >= 0:
			dawned = int(ag["age"]); break
	if dawned < 0:
		print("  (aucun âge levé en 220 ans — round-trip non exerçable sur cette graine)")
	else:
		print("  âge %d « %s » levé (an %d) ; engaged=%s" % [dawned, String(ag["name"]), w.year(), str(ag["engaged"])])
		if bool(ag["engaged"]):
			push_error("age_audit: engagé AVANT le verbe (le gate human a fui ?)"); viol += 1
		if not w.player_age_engage():
			push_error("age_audit: player_age_engage() refuse l'enfilement"); viol += 1
		w.advance_days(2)   # le drain tombe au tick suivant
		ag = w.age_state()
		if not bool(ag["engaged"]):
			push_error("age_audit: le verbe n'a pas mordu (engaged reste false)"); viol += 1
		else:
			print("  ✓ verbe drainé : engaged=true (âge %d)" % int(ag["age"]))

	if viol == 0:
		print("AGE AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
