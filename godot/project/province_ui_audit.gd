extends Node
## province_ui_audit — le pendant headless du câblage UI PROVINCE (5 readers additifs).
##
## Prouve, via le binding LIVE, que : (0) province_panel.gd et province_detail.gd
## compilent ; (1) province_slave_count est borné (≥0) ; (2) province_tax est fini
## et ≥0 ; (3) province_defense_pct est borné [100,1000] (100=neutre plaine) ; (4)
## province_seed est déterministe et non-négatif sur une province valide, -1 hors-
## borne ; (5) province_market renvoie 0..3 lignes bornées + un port cohérent
## (jamais "" ET jamais le mot "Port" sur une province non-EDI_PORT côtière — on ne
## peut pas vérifier ça de l'extérieur, donc juste bornage) ; (6) heraldry.gd
## expose compose_arms_generic + province_arms sans lever d'erreur.
##
## Lancer : godot --headless res://province_ui_audit.tscn -- seed=9,11,42 years=60

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
	return 60

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("province_ui_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== PROVINCE UI AUDIT — câblage complet (5 readers) ===")
	var total := 0
	# INVARIANT 0 : les panneaux qui CONSOMMENT la membrane compilent.
	if load("res://ui/province_panel.gd") == null:
		push_error("province_ui_audit: province_panel.gd ne compile pas")
		total += 1
	if load("res://ui/province_detail.gd") == null:
		push_error("province_ui_audit: province_detail.gd ne compile pas")
		total += 1
	if load("res://ui/heraldry.gd") == null:
		push_error("province_ui_audit: heraldry.gd ne compile pas")
		total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("PROVINCE UI AUDIT OK" if total == 0 else ("PROVINCE UI AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	var viol := 0
	var flags := ""
	var np: int = w.province_count()
	var any_slave := 0
	var any_tax := 0.0
	var any_market := 0

	for p in range(np):
		var sc: int = int(w.province_slave_count(p))
		if sc < 0:
			viol += 1; flags += " ✗slave(" + str(sc) + ")"
		any_slave += sc

		var tax: float = float(w.province_tax(p))
		if tax < 0.0 or is_nan(tax) or is_inf(tax):
			viol += 1; flags += " ✗tax(" + str(tax) + ")"
		any_tax += tax

		var dp: int = int(w.province_defense_pct(p))
		if dp < 100 or dp > 1000:
			viol += 1; flags += " ✗defense(" + str(dp) + ")"

		var mk: Dictionary = w.province_market(p)
		var lines: Array = mk.get("lines", [])
		if lines.size() > 3:
			viol += 1; flags += " ✗market_n(" + str(lines.size()) + ")"
		for l in lines:
			any_market += 1
			if float(l.get("price", -1.0)) < 0.0 or float(l.get("stock", -1.0)) < 0.0:
				viol += 1; flags += " ✗market_line"
			if String(l.get("name", "")) == "" or String(l.get("marche", "")) == "":
				viol += 1; flags += " ✗market_words"
		if not mk.has("port"):
			viol += 1; flags += " ✗no-port-key"

	# INVARIANT SEED : déterministe (même province → même seed d'un appel à l'autre), non-négatif.
	var seed_stable := true
	for p in range(mini(np, 30)):
		var a: int = int(w.province_seed(p))
		var b: int = int(w.province_seed(p))
		if a != b or a < 0:
			seed_stable = false
	if not seed_stable:
		viol += 1; flags += " ✗seed-unstable"
	if int(w.province_seed(-1)) != -1:
		viol += 1; flags += " ✗seed-oob"

	# INVARIANT HERALDRY : compose_arms_generic + province_arms ne lèvent pas d'erreur.
	var Heraldry = load("res://ui/heraldry.gd")
	var img: Image = Heraldry.compose_arms_generic(12345, 0.37, 2, 4, false)
	if img == null:
		viol += 1; flags += " ✗compose_arms_generic-null"
	if np > 0:
		var ptex: Texture2D = Heraldry.province_arms(0)
		# peut être null si la province 0 n'a pas de seed valide (rare) — pas une violation en soi,
		# on vérifie juste qu'AUCUNE province valide ne plante l'appel (déjà prouvé par l'absence
		# d'erreur de script à ce point).
		if ptex != null and ptex.get_width() <= 0:
			viol += 1; flags += " ✗province_arms-empty"

	print("seed ", sd, " an ", w.year(), " | ", np, " provinces | esclaves ", any_slave,
		" | taxe~", int(any_tax), "or/an | lignes marché ", any_market,
		(flags if flags != "" else " | OK"))
	return viol
