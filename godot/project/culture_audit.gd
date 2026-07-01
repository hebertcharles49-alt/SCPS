extends Node
## culture_audit — le pendant headless du CRÉATEUR DE CULTURE (binding + window).
##
## Prouve, à travers la GDExtension LIVE (libscps), que la membrane du créateur
## traverse : les listes (héritages/éthos/traditions), la validation 1maj/1min/1déf,
## l'aperçu des leviers (mots+signe), le nom de culture, et — le round-trip — que
## set_player_culture + regenerate GRAVE le choix (l'épithète d'éthos paraît au nom
## du pays joueur). Sort ≠ 0 si un invariant casse.
##
## Lancer : godot --headless res://culture_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("culture_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	var w = Sim.world
	print("=== CULTURE AUDIT — créateur de culture (binding live) ===")
	var viol := 0

	# INVARIANT 0 : la fenêtre du créateur compile (load échoue si le script est cassé).
	if load("res://ui/culture_creator.gd") == null:
		push_error("culture_audit: culture_creator.gd ne compile pas")
		viol += 1

	# INVARIANT 1 : les méthodes de binding EXISTENT dans le dll bâti.
	for m in ["heritage_list", "ethos_list", "tradition_list", "culture_validate",
			  "culture_preview", "culture_name", "set_player_culture", "clear_player_culture"]:
		if not w.has_method(m):
			push_error("culture_audit: méthode absente du binding : " + m)
			viol += 1
	if viol > 0:
		print("CULTURE AUDIT : ", viol, " VIOLATION(S) (binding incomplet)")
		get_tree().quit(1)
		return

	# INVARIANT 2 : les listes sont peuplées (6 / 6 / 36) avec mots + survols.
	var her: Array = w.heritage_list()
	var eth: Array = w.ethos_list()
	var trs: Array = w.tradition_list()
	print("  héritages=", her.size(), " éthos=", eth.size(), " traditions=", trs.size())
	if her.size() != 6 or String(her[0]["nom"]) == "" or String(her[0]["exemple"]) == "":
		viol += 1; push_error("héritages incomplets")
	if eth.size() != 6 or String(eth[0]["epithete"]) == "":
		viol += 1; push_error("éthos incomplets")
	if trs.size() != 36 or String(trs[0]["hover"]) == "":
		viol += 1; push_error("traditions incomplètes")

	# INVARIANT 3 : compo VALIDE piochée dans la liste (maj Phys / min Soc / déf Int).
	var maj := -1
	var mn := -1
	var df := -1
	for t in trs:
		var ax := int(t["axe"])
		var rg := int(t["rang"])
		if maj < 0 and ax == 0 and rg >= 2: maj = int(t["id"])
		if mn  < 0 and ax == 1 and rg == 1: mn  = int(t["id"])
		if df  < 0 and ax == 2 and rg <  0: df  = int(t["id"])
	if maj < 0 or mn < 0 or df < 0:
		viol += 1; push_error("pioche compo échouée")
	else:
		if not w.culture_validate(maj, mn, df):
			viol += 1; push_error("compo valide REFUSÉE")
		var prev: Array = w.culture_preview(maj, mn, df)
		if prev.size() == 0 or (int(prev[0]["signe"]) != 1 and int(prev[0]["signe"]) != -1):
			viol += 1; push_error("aperçu leviers vide/incohérent")

	# INVARIANT 4 : nom de culture (ethnonyme) non vide.
	var cn := String(w.culture_name(0, 7))
	print("  nom de culture (ESOTERIQUE,7) = ", cn)
	if cn == "":
		viol += 1; push_error("culture_name vide")

	# INVARIANT 5 : ROUND-TRIP — composer (éthos PACIFISTE=5 → épithète « Havre ») + régénérer
	# → le pays joueur PORTE l'épithète. Prouve que le choix grave le monde via la genèse.
	if maj >= 0 and mn >= 0 and df >= 0:
		var ok: bool = w.set_player_culture(0, 5, maj, mn, df)
		if not ok:
			viol += 1; push_error("set_player_culture a refusé une compo valide")
		Sim.regenerate(9)
		var info: Dictionary = w.country_info(w.player())
		var nom := String(info.get("nom", ""))
		print("  empire joueur composé = « ", nom, " »")
		if not nom.begins_with("Havre"):
			viol += 1; push_error("éthos non gravé (nom sans épithète « Havre ») : " + nom)
		w.clear_player_culture()

	print("")
	print("CULTURE AUDIT OK" if viol == 0 else ("CULTURE AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)
