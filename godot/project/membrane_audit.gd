extends Node
## membrane_audit — lot M « membrane honnête » : prouve, à travers la GDExtension LIVE,
## les readers neufs : build_legal (Dictionary {legal,reason} — le miroir du drain CMD_BUILD),
## manuf_cost (le prix que le drain débite), le spread servile (price_buy/price_sell),
## le LETTRÉ (expected + noms résolus + recrutement), et fin == fin_raw (RFIN_SANG
## traverse la membrane). Lancer : godot --headless res://membrane_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("membrane_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== MEMBRANE AUDIT — lot M (membrane honnête) ===")
	var viol := 0

	# INVARIANT 0 : les readers neufs existent au binding
	for m in ["build_legal", "manuf_cost", "religion_scholar_expected",
			"scholar_role_name", "scholar_role_ability", "slave_market"]:
		if not w.has_method(m):
			push_error("membrane_audit: méthode absente : " + m); viol += 1
	if viol > 0:
		print("MEMBRANE AUDIT : ", viol, " VIOLATION(S)"); get_tree().quit(1); return

	# INVARIANT 1 : build_legal — Dictionary borné + cohérence legal⇔reason sur TOUT le roster
	var me: int = w.player()
	var roster: Array = w.building_roster(me)
	var n_legal := 0
	for b in roster:
		var bl: Dictionary = w.build_legal(-1, int(b.get("type", -1)))
		var legal: bool = bool(bl.get("legal", false))
		var reason := int(bl.get("reason", -9))
		if reason < 0 or reason > 3:
			viol += 1; push_error("build_legal: reason hors-bornes (%d)" % reason)
		if legal and reason != 0:
			viol += 1; push_error("build_legal: legal=true mais reason=%d" % reason)
		if not legal and reason == 0:
			viol += 1; push_error("build_legal: legal=false mais reason=0")
		if legal: n_legal += 1
	print("  build_legal : roster=", roster.size(), " · légaux MAINTENANT=", n_legal)
	# hors-bornes : illégal, structurel
	var blx: Dictionary = w.build_legal(-1, 9999)
	if bool(blx.get("legal", true)):
		viol += 1; push_error("build_legal: édifice hors-bornes accepté")

	# INVARIANT 2 : manuf_cost — le prix affiché est un nombre tangible > 0
	var mc := int(w.manuf_cost())
	print("  manuf_cost : ", mc, " or")
	if mc <= 0: viol += 1; push_error("manuf_cost <= 0")

	# INVARIANT 3 : le SPREAD servile — achat (×2) ≥ vente (×1) > 0
	var mk: Dictionary = w.slave_market()
	var pb := int(mk.get("price_buy", -1))
	var ps := int(mk.get("price_sell", -1))
	print("  marché servile : achat=", pb, " or/âme · vente=", ps, " or/âme")
	if pb <= 0 or ps <= 0: viol += 1; push_error("prix serviles absents/nuls")
	if pb < ps: viol += 1; push_error("spread inversé (achat < vente)")

	# INVARIANT 4 : le LETTRÉ — sans foi expected=-1 ; après fondation, expected résolu,
	# nom/aptitude non vides, et le recrutement DONNE ce rôle-là (rôle actif = attendu).
	if int(w.religion_scholar_expected(me)) != -1:
		viol += 1; push_error("scholar_expected != -1 sans foi")
	var rid: int = w.religion_found(me, 0, 0, 4, 10)   # crédo pluraliste → Gourou (RESIST)
	if rid < 0:
		viol += 1; push_error("fondation échouée (préalable lettré)")
	else:
		var expected := int(w.religion_scholar_expected(me))
		var nm := String(w.scholar_role_name(expected))
		var ab := String(w.scholar_role_ability(expected))
		print("  lettré attendu : rôle=", expected, " · ", nm, " (", ab, ")")
		if expected < 0: viol += 1; push_error("expected < 0 avec une foi")
		if nm == "" or nm == "?": viol += 1; push_error("nom de rôle non résolu")
		if ab == "" or ab == "?": viol += 1; push_error("aptitude non résolue")
		if int(w.religion_scholar_role(me)) != -1:
			viol += 1; push_error("lettré actif avant recrutement")
		var reg := int(w.country_capital_region(me))
		var got := int(w.religion_recruit_scholar(me, reg))
		if got != expected: viol += 1; push_error("rôle recruté != attendu")
		if int(w.religion_scholar_role(me)) != expected:
			viol += 1; push_error("rôle actif != attendu après recrutement")
		print("  lettré recruté à la capitale (rég ", reg, ") : rôle actif=", w.religion_scholar_role(me))

	# INVARIANT 5 : fin == fin_raw (le miroir RFIN porte SANG — même échelle 0..5)
	var e: Dictionary = w.endgame_info()
	var fin := int(e.get("fin", -1))
	var fraw := int(e.get("fin_raw", -1))
	print("  endgame : fin=", fin, " · fin_raw=", fraw)
	if fin != fraw: viol += 1; push_error("fin != fin_raw (le miroir RFIN perd une fin)")
	if fin < 0 or fin > 5: viol += 1; push_error("fin hors-bornes 0..5")

	# INVARIANT 6 : après 2 mois (hub map bâtie, économie tickée), le miroir RESPIRE —
	# histogramme des raisons + cohérence re-vérifiée ; on n'asserte pas un compte
	# (dépendant du monde), seulement que chaque refus reste MOTIVÉ.
	w.advance_days(60)
	var hist := {0: 0, 1: 0, 2: 0, 3: 0}
	for b in roster:
		var bl2: Dictionary = w.build_legal(-1, int(b.get("type", -1)))
		var r2 := int(bl2.get("reason", -9))
		if r2 < 0 or r2 > 3:
			viol += 1; push_error("build_legal (j+60): reason hors-bornes"); continue
		if bool(bl2.get("legal", false)) != (r2 == 0):
			viol += 1; push_error("build_legal (j+60): legal incohérent avec reason")
		hist[r2] += 1
	print("  build_legal (j+60) : OK=", hist[0], " · structurel=", hist[1],
		" · or=", hist[2], " · matière=", hist[3])

	print("")
	print("MEMBRANE AUDIT OK" if viol == 0 else ("MEMBRANE AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)
