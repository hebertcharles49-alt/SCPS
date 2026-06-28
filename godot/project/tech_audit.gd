extends Node
## tech_audit — le pendant headless de la membrane ARBRE DE TECH + MÉTABOLISATION (Medusa).
##
## Prouve, via le binding LIVE, que : (0) tech_panel.gd compile ; (1) tech_info porte
## metab_pct (le « +X% recherche » du hover) ; (2) heritage_access() renvoie les 6 héritages,
## chacun avec un tier 0-3 et une part digérée 0-100, EXACTEMENT un natif (tier 3) ; (3)
## tech_nodes() couvre l'arbre ÉTOFFÉ + les COMBOS tier-4 (≥ 46 nœuds, une fusion nommée
## présente). Sort ≠ 0 si un invariant casse.
##
## Lancer : godot --headless res://tech_audit.tscn -- seed=9,11,42 years=120

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
	return 120

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("tech_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== TECH AUDIT — arbre Medusa + barre de métabolisation ===")
	var total := 0
	# INVARIANT 0 : le panneau qui CONSOMME la membrane compile (load échoue si cassé).
	if load("res://ui/tech_panel.gd") == null:
		push_error("tech_audit: tech_panel.gd ne compile pas")
		total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("TECH AUDIT OK" if total == 0 else ("TECH AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	var viol := 0
	var flags := ""

	# INVARIANT 1 : tech_info — points ≥ 0, metab_pct présent & ≥ 0 (le hover « +X% recherche »).
	var info: Dictionary = w.tech_info()
	if not info.has("metab_pct"):
		viol += 1; flags += " ✗no-metab_pct"
	var mp := int(info.get("metab_pct", -1))
	if mp < 0 or mp > 500:
		viol += 1; flags += " ✗metab(" + str(mp) + ")"

	# INVARIANT 2 : heritage_access — 6 héritages, tier 0-3, digéré 0-100, EXACTEMENT 1 natif (tier 3).
	var acc: Array = w.heritage_access()
	if acc.size() != 6:
		viol += 1; flags += " ✗heritage_count(" + str(acc.size()) + ")"
	var n_native := 0
	for h in acc:
		var t: int = int(h["tier"])
		var dp: int = int(h["digested_pct"])
		if t < 0 or t > 3:
			viol += 1; flags += " ✗tier(" + str(t) + ")"
		if dp < 0 or dp > 100:
			viol += 1; flags += " ✗digéré(" + str(dp) + ")"
		if bool(h["native"]):
			n_native += 1
			if t != 3:
				viol += 1; flags += " ✗natif-pas-tier3"
	if n_native != 1:
		viol += 1; flags += " ✗n_native(" + str(n_native) + ")"

	# INVARIANT 3 : tech_nodes — l'arbre ÉTOFFÉ + les COMBOS (≥46 nœuds, une fusion nommée présente).
	var nodes: Array = w.tech_nodes()
	if nodes.size() < 46:
		viol += 1; flags += " ✗n_nodes(" + str(nodes.size()) + ")"
	var has_combo := false
	for nd in nodes:
		var nm := String(nd.get("name", ""))
		if nm == "Arquebuserie de précision" or nm == "Engins de siège" or nm == "Académie cosmopolite":
			has_combo = true
			break
	if not has_combo:
		viol += 1; flags += " ✗no-combo-node"

	print("seed ", sd, " an ", w.year(), " | nœuds ", nodes.size(), " | métab +", mp, "% | natifs ",
		n_native, (flags if flags != "" else " | OK"))
	return viol
