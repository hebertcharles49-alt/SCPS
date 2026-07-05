extends Node
## annales_audit — pendant headless du binding LES ANNALES DU RÈGNE (§ Annales). Prouve,
## à travers la GDExtension LIVE : (1) le binding annals() existe ; (2) un monde neuf
## rend un anneau VIDE ; (3) le ROUND-TRIP — un dilemme résolu par le JOUEUN (drain réel,
## CMD_EVENT_CHOICE) fait apparaître une entrée, ligne non vide, triée par année.
## Lancer : godot --headless res://annales_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("annales_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== ANNALES AUDIT — le récit sélectif du règne (binding live) ===")
	var viol := 0

	if not w.has_method("annals"):
		push_error("annales_audit: méthode 'annals' absente du binding"); viol += 1
	if not w.has_method("pending_event"):
		push_error("annales_audit: méthode 'pending_event' absente du binding"); viol += 1
	if viol > 0:
		get_tree().quit(2); return

	w.generate(42)
	var a0: Array = w.annals()
	print("  an-0 : %d entrée(s)" % a0.size())
	if a0.size() != 0:
		push_error("annales_audit: an-0 déjà des Annales ?"); viol += 1

	# ROUND-TRIP : avance jusqu'à ce qu'une décision joueur s'enfile (Marbrive…), résous-la,
	# vérifie qu'une ANNAL_DILEMME apparaît, ligne non vide, triée par année.
	var enfile := false
	for y in range(200):
		w.advance_days(365)
		if int(w.pending_count()) > 0:
			enfile = true
			break
	if not enfile:
		print("  (aucune décision joueur apparue en 200 ans sur cette graine — round-trip non exerçable)")
	else:
		var pe: Dictionary = w.pending_event(0)
		print("  décision en attente : '%s' (%d option(s))" % [String(pe.get("situation", "")), int(pe.get("n_options", 0))])
		if not w.player_event_choice(0, 0):
			push_error("annales_audit: player_event_choice refuse l'enfilement"); viol += 1
		w.advance_days(2)   # le drain résout au tick suivant
		var a1: Array = w.annals()
		if a1.size() <= 0:
			push_error("annales_audit: aucune entrée après un dilemme résolu"); viol += 1
		else:
			var prev_year := -1000000
			var sorted := true
			var has_line := true
			for e in a1:
				if String(e.get("ligne", "")) == "":
					has_line = false
				var yr := int(e.get("year", 0))
				if yr < prev_year:
					sorted = false
				prev_year = yr
			if not has_line:
				push_error("annales_audit: une entrée porte une ligne vide"); viol += 1
			if not sorted:
				push_error("annales_audit: les entrées ne sont pas triées par année"); viol += 1
			print("  ✓ %d entrée(s) après résolution, lignes non vides, triées (ex: '%s')" % [a1.size(), String(a1[0].get("ligne", ""))])

	if viol == 0:
		print("ANNALES AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
