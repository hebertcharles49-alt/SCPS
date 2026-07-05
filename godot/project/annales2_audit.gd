extends Node
## annales2_audit — pendant headless des ANNALES-2 (récap d'âge · épilogue · épithètes).
## Prouve : (1) l'épithète est DÉTERMINISTE (mêmes annales → même épithète) et suit ses
## seuils (comptage de kinds) ; (2) slice_since filtre bien par année ; (3) l'écran de
## RÉCAP D'ÂGE s'instancie et son builder remplit titre/bilan/tranche depuis la façade ;
## (4) l'ÉPILOGUE s'instancie et open(fin) compose la phrase du règne (épithète + fin).
## Lancer : godot --headless res://annales2_audit.tscn

const Epithet = preload("res://ui/epithet.gd")

func _ready() -> void:
	get_window().size = Vector2i(640, 480)
	_run.call_deferred()

func _mk(kind: int, year: int) -> Dictionary:
	return {"year": year, "kind": kind, "ligne": "fait de l'an %d" % year, "region": -1}

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("annales2_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== ANNALES-2 AUDIT — récap d'âge · épilogue · épithètes ===")
	var viol := 0

	# ── (1) ÉPITHÈTES : déterminisme + seuils ────────────────────────────────
	var sanglant := []
	for i in range(5): sanglant.append(_mk(Epithet.K_GUERRE_GAGNEE, 10 + i))
	for i in range(3): sanglant.append(_mk(Epithet.K_CICATRICE, 20 + i))
	var e1: String = Epithet.derive(sanglant)
	var e2: String = Epithet.derive(sanglant.duplicate(true))
	print("  épithète (5 guerres + 3 cicatrices) : « %s » / re-dérivée « %s »" % [e1, e2])
	if e1 != e2:
		push_error("annales2_audit: épithète NON déterministe"); viol += 1
	if e1 != "le Sanglant":
		push_error("annales2_audit: seuil Sanglant non atteint (obtenu « %s »)" % e1); viol += 1
	if Epithet.derive([]) != "le Discret":
		push_error("annales2_audit: annales vides ≠ le Discret"); viol += 1
	var conq := []
	for i in range(4): conq.append(_mk(Epithet.K_GUERRE_GAGNEE, i))
	if Epithet.derive(conq) != "le Conquérant":
		push_error("annales2_audit: 4 guerres gagnées ≠ le Conquérant (obtenu « %s »)" % Epithet.derive(conq)); viol += 1
	var bat := []
	for i in range(3): bat.append(_mk(Epithet.K_MONUMENT, i))
	if Epithet.derive(bat) != "le Bâtisseur":
		push_error("annales2_audit: 3 monuments ≠ le Bâtisseur (obtenu « %s »)" % Epithet.derive(bat)); viol += 1
	# fin du monde : domine tout
	var fin_arr := sanglant.duplicate(true)
	fin_arr.append(_mk(Epithet.K_FIN, 200))
	if Epithet.derive(fin_arr) != "l'Apocalyptique":
		push_error("annales2_audit: FIN ne domine pas (obtenu « %s »)" % Epithet.derive(fin_arr)); viol += 1
	print("  ✓ épithètes : déterministes, seuils tenus (Sanglant/Conquérant/Bâtisseur/Discret/Apocalyptique)")

	# ── (2) slice_since : filtre STRICTEMENT postérieur ─────────────────────
	var sl: Array = Epithet.slice_since(sanglant, 20)   # garde 21,22 (cicatrices an 21-22)
	if sl.size() != 2:
		push_error("annales2_audit: slice_since(20) attend 2, obtenu %d" % sl.size()); viol += 1
	if Epithet.slice_since(sanglant, -1000000).size() != sanglant.size():
		push_error("annales2_audit: slice_since(-inf) devrait tout garder"); viol += 1
	print("  ✓ slice_since : tranche par année correcte")

	# ── (3) RÉCAP D'ÂGE : constructible + builder rempli depuis la façade ───
	w.generate(9)
	var recap: Control = load("res://ui/age_recap.gd").new()
	add_child(recap)
	await get_tree().process_frame
	w.advance_days(400)   # un an : quelques faits possibles, country_info vivant
	recap._build(w, "l'Essor")
	if String(recap._title.text) == "":
		push_error("annales2_audit: récap — titre vide"); viol += 1
	if not String(recap._bilan.text).begins_with("Bilan"):
		push_error("annales2_audit: récap — bilan absent (« %s »)" % recap._bilan.text); viol += 1
	if String(recap._epithet_line.text).find("dit ") < 0:
		push_error("annales2_audit: récap — l'épithète manque à l'appel"); viol += 1
	print("  ✓ récap d'âge : « %s » / %s" % [recap._title.text, recap._bilan.text])

	# ── (4) ÉPILOGUE : constructible + phrase composée ──────────────────────
	var epi: Control = load("res://ui/epilogue.gd").new()
	add_child(epi)
	await get_tree().process_frame
	var speed_before: int = Sim.speed_index
	epi.open(3)   # fin RONCES
	var phrase := String(epi._phrase.text)
	print("  épilogue : « %s »" % phrase)
	if phrase.find("Ronces") < 0:
		push_error("annales2_audit: épilogue — la nature de la fin manque"); viol += 1
	if phrase.find("dit ") < 0:
		push_error("annales2_audit: épilogue — l'épithète manque"); viol += 1
	if phrase.find("régna") < 0:
		push_error("annales2_audit: épilogue — la durée du règne manque"); viol += 1
	if not epi.visible:
		push_error("annales2_audit: épilogue — pas visible après open()"); viol += 1
	if Sim.speed_index != 0:
		push_error("annales2_audit: épilogue — le monde n'est pas en pause"); viol += 1
	epi.visible = false   # « Contempler » / Échap : la vitesse d'avant est restaurée
	await get_tree().process_frame
	if Sim.speed_index != speed_before:
		push_error("annales2_audit: épilogue — vitesse non restaurée à la fermeture"); viol += 1
	print("  ✓ épilogue : phrase complète, pause à l'ouverture, vitesse restaurée")

	if viol == 0:
		print("ANNALES-2 AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
