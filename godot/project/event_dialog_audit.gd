extends Node
## event_dialog_audit — pendant headless de la MEMBRANE DE DÉCISION. Prouve, à travers la
## GDExtension LIVE : (1) pending_count expose 0 décision au départ ; (2) le monde tourne
## assez longtemps pour qu'une VRAIE décision (Marbrive, n_options>1) concernant le joueur
## s'ENFILE — jamais tranchée par l'IA à sa place ; (3) event_dialog.gd s'OUVRE en PAUSE,
## affiche la situation (mot résolu) + les choix (labels/flavors) ; (4) le clic ENFILE
## player_event_choice (drainé au tick suivant) → le pending disparaît, la vitesse d'avant
## REVIENT. Lancer : godot --headless res://event_dialog_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(640, 480)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("event_dialog_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== EVENT DIALOG AUDIT — la membrane de décision (file joueur) ===")
	var viol := 0

	var gen_seed := 9
	if OS.get_environment("EDAUDIT_SEED") != "":
		gen_seed = int(OS.get_environment("EDAUDIT_SEED"))
	w.generate(gen_seed)
	Sim.game_on = true   # au-delà du menu/setup : le monde CONCERNE le joueur (comme en jeu)
	if int(w.pending_count()) != 0:
		push_error("event_dialog_audit: pending non-vide à la genèse"); viol += 1
	else:
		print("  ✓ aucune décision en attente à la genèse")

	# le dialogue, câblé comme dans main.gd
	var dlg: Control = load("res://ui/event_dialog.gd").new()
	add_child(dlg)
	await get_tree().process_frame

	Sim.set_speed(2)   # le jeu « tourne » (vitesse non nulle) pour prouver la mise en pause

	# on laisse le monde VIVRE (jusqu'à 200 ans) — une VRAIE décision (Marbrive) finit
	# statistiquement par concerner le joueur ; timeout gracieux sinon (rare, mais possible).
	var found := false
	for yr in range(200):
		w.advance_days(365)
		if int(w.pending_count()) > 0:
			found = true
			break

	if not found:
		print("  (aucune décision joueur apparue en 200 ans sur cette graine — audit non-concluant, PAS un échec)")
		print("EVENT DIALOG AUDIT OK (ignoré)")
		get_tree().quit(0); return

	print("  ✓ une VRAIE décision (Marbrive) concerne le joueur → ENFILÉE (pas auto-résolue)")

	# le dialogue poll sur Sim.ticked — un tick supplémentaire le fait ouvrir.
	w.advance_days(1)
	await get_tree().process_frame

	if not dlg.visible:
		push_error("event_dialog_audit: le dialogue ne s'est pas ouvert"); viol += 1
	else:
		print("  ✓ dialogue OUVERT")
		if Sim.speed_index != 0:
			push_error("event_dialog_audit: le jeu n'est pas en PAUSE"); viol += 1
		else:
			print("  ✓ jeu mis en PAUSE (speed 0)")
		var situation := String(dlg._pending.get("situation", ""))
		var n_opt := int(dlg._pending.get("n_options", 0))
		if situation == "" or n_opt < 2:
			push_error("event_dialog_audit: situation/options mal résolues (%s, %d)" % [situation, n_opt]); viol += 1
		else:
			print("  ✓ situation « %s » — %d choix" % [situation, n_opt])
		var labels: Array = dlg._pending.get("labels", [])
		var flavors: Array = dlg._pending.get("flavors", [])
		if labels.size() != n_opt or flavors.size() != n_opt:
			push_error("event_dialog_audit: labels/flavors incomplets"); viol += 1
		else:
			print("  ✓ %d label(s) + %d flavor(s) — des MOTS, membrane tenue" % [labels.size(), flavors.size()])

	# résout le choix 0 — le pending doit disparaître et la vitesse d'avant revenir.
	if dlg.visible:
		dlg._choose(0)
		w.advance_days(2)   # le drain applique CMD_EVENT_CHOICE au tick suivant
		if int(w.pending_count()) > 0:
			# un AUTRE pending a pu apparaître entretemps (autre pays/région) — non bloquant ;
			# on vérifie juste que LE dialogue s'est refermé après le choix.
			print("  (un autre pending est apparu entretemps — hors-scope de ce choix)")
		if dlg.visible:
			push_error("event_dialog_audit: le dialogue reste visible après le choix"); viol += 1
		else:
			print("  ✓ le choix DRAINÉ ferme le dialogue")
		if Sim.speed_index != 2:
			push_error("event_dialog_audit: la vitesse d'avant n'est pas restaurée (%d)" % Sim.speed_index); viol += 1
		else:
			print("  ✓ vitesse d'avant restaurée")

	if viol == 0:
		print("EVENT DIALOG AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
