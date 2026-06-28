extends Node
## devpanel_audit — le pendant headless de la membrane MODTOOLS (panneau dev, F10).
##
## Prouve, via le binding LIVE, que : (0) devpanel.gd compile ; (1) tunables() renvoie le
## registre complet (>100 entrées), chacune avec nom/value/def/overridden ; (2) tune_set
## SURCHARGE en direct (la valeur change ET le drapeau « overridden » bascule). Sort ≠ 0
## si un invariant casse.
##
## Lancer : godot --headless res://devpanel_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("devpanel_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== DEVPANEL AUDIT — registre des tunables (live) ===")
	var viol := 0

	# INVARIANT 0 : le panneau qui CONSOMME la membrane compile.
	if load("res://ui/devpanel.gd") == null:
		push_error("devpanel_audit: devpanel.gd ne compile pas")
		viol += 1

	var w = Sim.world

	# INVARIANT 1 : tunables() — registre complet, chaque entrée bien formée.
	var tn: Array = w.tunables()
	if tn.size() < 100:
		viol += 1; print("  ✗ registre trop court : ", tn.size())
	var first_name := ""
	for t in tn:
		if not (t.has("nom") and t.has("value") and t.has("def") and t.has("overridden")):
			viol += 1; print("  ✗ entrée mal formée : ", t)
			break
		if first_name == "":
			first_name = String(t["nom"])
	print("  registre : ", tn.size(), " tunable(s) — premier : ", first_name)

	# INVARIANT 2 : tune_set surcharge EN DIRECT (valeur change + overridden bascule).
	if first_name != "":
		var before := 0.0
		var was_over := false
		for t in tn:
			if String(t["nom"]) == first_name:
				before = float(t["value"]); was_over = bool(t["overridden"]); break
		var target := before + 1.0
		w.tune_set(first_name, target)
		var found := false
		for t in w.tunables():
			if String(t["nom"]) == first_name:
				found = true
				if abs(float(t["value"]) - target) > 0.0001:
					viol += 1; print("  ✗ valeur non appliquée : ", t["value"], " ≠ ", target)
				if not bool(t["overridden"]):
					viol += 1; print("  ✗ drapeau overridden non basculé")
				break
		if not found:
			viol += 1; print("  ✗ tunable introuvable après tune_set")
		print("  tune_set(", first_name, ") : ", before, " → ", target,
			"  (overridden ", was_over, " → vrai)")

	print("")
	print("DEVPANEL AUDIT OK" if viol == 0 else ("DEVPANEL AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)
