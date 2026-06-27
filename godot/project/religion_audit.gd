extends Node
## religion_audit — pendant headless du UI religion (binding P5). Prouve, à travers la
## GDExtension LIVE, que la membrane religion traverse : listes (16 pôles / 3 crédos),
## validation un-par-axe, fondation (pays + régions héritent), schisme (enfant + fracture),
## recrutement de lettré, lecture du nom. Lancer : godot --headless res://religion_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("religion_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== RELIGION AUDIT — UI religion (binding live) ===")
	var viol := 0

	# INVARIANT 0 : le panneau compile
	if load("res://ui/religion_panel.gd") == null:
		push_error("religion_audit: religion_panel.gd ne compile pas"); viol += 1

	# INVARIANT 1 : binding présent
	for m in ["religion_pole_list", "credo_list", "religion_found", "religion_eligible",
			  "religion_schism", "religion_of_country", "religion_of_region",
			  "religion_recruit_scholar", "religion_name", "religion_picks_valid",
			  "religion_founding_ready"]:
		if not w.has_method(m):
			push_error("religion_audit: méthode absente : " + m); viol += 1
	if viol > 0:
		print("RELIGION AUDIT : ", viol, " VIOLATION(S)"); get_tree().quit(1); return

	# INVARIANT 1b : le monde est ATHÉE au départ — aucune foi, créateur PAS déclenché
	var atheist := true
	for c in range(w.country_count()):
		if int(w.religion_of_country(c)) >= 0: atheist = false
	if not atheist: viol += 1; push_error("monde non athée au départ")
	if int(w.religion_founding_ready(w.player())) != 0:
		viol += 1; push_error("créateur prêt sans édifice religieux")
	print("  monde athée au départ : ", atheist, " · créateur prêt : ", w.religion_founding_ready(w.player()))

	# INVARIANT 2 : listes peuplées
	var poles: Array = w.religion_pole_list()
	var credos: Array = w.credo_list()
	print("  pôles=", poles.size(), " crédos=", credos.size())
	if poles.size() != 16 or String(poles[0]["nom"]) == "" or String(poles[0]["tip"]) == "":
		viol += 1; push_error("pôles incomplets")
	if credos.size() != 3 or String(credos[0]["nom"]) == "":
		viol += 1; push_error("crédos incomplets")

	# INVARIANT 3 : un-par-axe — 3 pôles d'axes distincts valides ; même axe invalide
	# (pôles 0 et 1 partagent l'axe SANG ; 0,4,10 sont 3 axes distincts)
	if not w.religion_picks_valid(0, 4, 10): viol += 1; push_error("axes distincts refusés")
	if w.religion_picks_valid(0, 1, 10): viol += 1; push_error("même axe accepté (devrait être refusé)")

	# INVARIANT 4 : fondation → pays + régions héritent ; nom non vide
	var me: int = w.player()
	var rid: int = w.religion_found(me, 0, 0, 4, 10)   # crédo pluraliste · Fécondité/Accueil/Gnose
	var inh := 0
	for r in range(w.region_count()):
		if w.religion_of_region(r) == rid: inh += 1
	print("  fondation : rid=", rid, " · régions héritées=", inh, " · nom=« ", w.religion_name(me), " »")
	if rid < 0 or w.religion_of_country(me) != rid: viol += 1; push_error("fondation échouée")
	if inh <= 0: viol += 1; push_error("régions n'héritent pas")
	if String(w.religion_name(me)) == "": viol += 1; push_error("nom de religion vide")
	# une fois la foi fondée, le créateur n'est plus « prêt » (one-shot)
	if int(w.religion_founding_ready(me)) != 0: viol += 1; push_error("créateur encore prêt après fondation")

	# INVARIANT 5 : schisme interne → enfant + fracture bornée
	var sch: Dictionary = w.religion_schism(me, 1, 5, 2, 11, 2)   # repick MUR/Orthodoxie, crédo purificateur
	var child := int(sch["child"]); var flipped := int(sch["flipped"])
	print("  schisme : enfant=", child, " · basculées=", flipped)
	if child <= rid: viol += 1; push_error("schisme n'a pas créé d'enfant")
	if flipped < 0 or flipped > inh: viol += 1; push_error("fracture hors-bornes")

	# INVARIANT 6 : recrutement de lettré (rôle dérivé du crédo)
	var role := int(w.religion_recruit_scholar(me, 0))
	print("  lettré recruté : rôle=", role)
	if role < 0: viol += 1; push_error("recrutement de lettré échoué")

	print("")
	print("RELIGION AUDIT OK" if viol == 0 else ("RELIGION AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)
