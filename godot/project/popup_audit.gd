extends Node
## popup_audit — pendant headless du POPUP OYEZ OYEZ. Prouve, à travers la GDExtension
## LIVE : (1) une guerre déclarée par le joueur ARRIVE au fil (kind 1) ; (2) alerts.gd la
## ROUTE vers popup_requested (POPUP_KINDS) au lieu d'un chip ; (3) le popup s'OUVRE, met
## le jeu en PAUSE (speed 0) et porte des boutons adaptatifs ; (4) « Vu » referme et la
## vitesse d'avant REVIENT. Lancer : godot --headless res://popup_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(640, 480)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("popup_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== POPUP AUDIT — OYEZ OYEZ (fil → routage → pause → boutons) ===")
	var viol := 0

	w.generate(9)
	var me: int = w.player()

	# la paire alerts + popup, câblée comme dans main.gd
	var alerts: Control = load("res://ui/alerts.gd").new()
	add_child(alerts)
	var popup: Control = load("res://ui/event_popup.gd").new()
	add_child(popup)
	alerts.popup_requested.connect(popup.enqueue)
	await get_tree().process_frame

	# le jeu « tourne » (vitesse non nulle) pour prouver la mise en pause
	Sim.set_speed(2)

	# AMORCE : franchir la 1re frontière de mois AVANT de déclarer — le fil s'amorce
	# en silence (anti-spam an-0) ; une guerre déclarée avant serait avalée en baseline.
	w.advance_days(35)

	# GUERRE : déclarer sur toute cible valide (comme le banc C), franchir une frontière de mois
	var declared := false
	for c in range(w.country_count()):
		if c != me and int(w.country_province_count(c)) > 0 and bool(w.player_declare_war(c)):
			declared = true
	if not declared:
		push_error("popup_audit: aucune guerre déclarable"); get_tree().quit(1); return
	w.advance_days(40)
	var at_war := 0
	for rel in w.country_relations(me):
		if String(rel.get("status", "")).to_lower().contains("guerre"):
			at_war += 1
	print("  [debug] pays en GUERRE au drain : %d" % at_war)
	var raw: Array = w.feed_poll(0)
	var kinds := []
	for ev in raw:
		kinds.append(int(ev["kind"]))
	print("  [debug] fil brut : %d entrée(s), kinds %s" % [raw.size(), str(kinds)])
	alerts._refresh()          # poll explicite (le probe avance hors Sim.ticked)
	await get_tree().process_frame
	print("  [debug] popup.visible=%s queue=%d" % [str(popup.visible), popup._queue.size()])

	# (1)+(2)+(3) : le popup est OUVERT, en PAUSE, titre guerre, boutons adaptatifs
	if not popup.visible:
		push_error("popup_audit: le popup ne s'est pas ouvert"); viol += 1
	else:
		print("  ✓ popup OUVERT (titre « %s »)" % String(popup._cur.get("title", "")))
		if Sim.speed_index != 0:
			push_error("popup_audit: le jeu n'est pas en PAUSE"); viol += 1
		else:
			print("  ✓ jeu mis en PAUSE (speed 0)")
		var btns: Array = popup._cur.get("buttons", [])
		if btns.size() < 2:
			push_error("popup_audit: boutons adaptatifs manquants (%d)" % btns.size()); viol += 1
		else:
			print("  ✓ %d boutons adaptatifs" % btns.size())

	# (4) : fermer TOUTE la file (« Vu ») → la vitesse d'avant revient
	var guard := 0
	while popup.visible and guard < 20:
		popup._fire("close", -1)
		guard += 1
	if popup.visible:
		push_error("popup_audit: la file ne se vide pas"); viol += 1
	elif Sim.speed_index != 2:
		push_error("popup_audit: la vitesse d'avant n'est pas restaurée (%d)" % Sim.speed_index); viol += 1
	else:
		print("  ✓ file vidée (%d évènement(s)) + vitesse restaurée" % guard)

	if viol == 0:
		print("POPUP AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
