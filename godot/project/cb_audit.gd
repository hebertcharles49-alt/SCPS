extends Node
## cb_audit — vérifie la CHAÎNE du casus belli de bout en bout via la façade LIVE
## (retour joueur « le verbe du CB ne donne qu'une trêve ? »). Fabrique → mûrit → déclare
## → est-ce vraiment la GUERRE (pas une trêve) ? Imprime chaque étape.
## Lancer : godot --headless --path godot/project res://cb_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _status(w, me: int, t: int) -> String:
	for rl in w.country_relations(me):
		if int(rl.get("country", -1)) == t:
			return String(rl.get("status", "?"))
	return "(inconnu)"

func _run() -> void:
	await get_tree().process_frame; await get_tree().process_frame
	if Sim.world == null:
		push_error("cb_audit: pas de monde"); get_tree().quit(2); return
	var w = Sim.world
	print("=== CB AUDIT — fabriquer → mûrir → déclarer la guerre ===")
	w.generate(9)
	var me: int = w.player()
	w.advance_days(200)   # tôt : les hameaux libres voisins pas encore ralliés
	var t := -1
	for cid in range(64):
		if cid == me: continue
		if not w.has_method("country_known") or int(w.country_known(cid)) != 1: continue
		var ci: Dictionary = w.country_info(cid)
		if bool(ci.get("valide", false)) and int(ci.get("regions", 0)) > 0:   # VIVANT (a des régions)
			t = cid; break
	if t < 0:
		print("  (aucun pays étranger connu à l'an %d)" % w.year()); get_tree().quit(0); return
	print("  (an %d) cible = %s · rôle = %d" % [w.year(), String(w.country_info(t).get("nom","?")),
		int(w.country_role(t)) if w.has_method("country_role") else -1])
	var opX: Dictionary = w.diplo_options(t)
	print("  DIAG live-WILD : regions(info)=%d · can_declare_war=%s · can_fabricate=%s · can_offer_pact=%s · can_embargo=%s" % [
		int(w.country_info(t).get("regions",-1)),
		str(opX.get("can_declare_war",false)), str(opX.get("can_fabricate",false)),
		str(opX.get("can_offer_pact",false)), str(opX.get("can_embargo",false))])
	print("  cible = %s (cid %d) · statut initial = %s · or joueur = %d" % [
		String(w.country_info(t).get("nom","?")), t, _status(w, me, t), int(w.country_info(me).get("or",0))])

	var op0: Dictionary = w.diplo_options(t)
	print("  options : can_fabricate=%s · can_declare_war=%s · truce_days=%.0f · cb_ready=%s" % [
		str(op0.get("can_fabricate",false)), str(op0.get("can_declare_war",false)),
		float(op0.get("truce_days",0.0)), str(op0.get("cb_ready",false))])

	# 1) FABRIQUER
	var okf: bool = w.player_fabricate_cb(t)
	w.advance_days(30)
	var op1: Dictionary = w.diplo_options(t)
	print("  après fabrication (émis=%s) : fabricating=%s (reste %.0f j)" % [
		str(okf), str(op1.get("fabricating",false)), float(op1.get("fabricating_days_left",0.0))])

	# 2) MÛRIR (~1 an + marge)
	w.advance_days(400)
	var op2: Dictionary = w.diplo_options(t)
	print("  après ~400 j : cb_ready=%s · can_declare_war=%s · truce_days=%.0f" % [
		str(op2.get("cb_ready",false)), str(op2.get("can_declare_war",false)), float(op2.get("truce_days",0.0))])

	# 3) DÉCLARER LA GUERRE
	var okw: bool = w.player_declare_war(t)
	w.advance_days(30)
	print("  après déclaration (émis=%s) : STATUT = %s" % [str(okw), _status(w, me, t)])
	print("CB AUDIT DONE")
	get_tree().quit(0)
