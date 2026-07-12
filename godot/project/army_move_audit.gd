extends Node
## army_move_audit — vérifie le verbe MOUVEMENT LIBRE (clic-armée → destination) via la
## façade LIVE. Déploie/redirige l'armée du joueur vers une région À SOI et confirme
## qu'elle s'ACTIVE (campagne) sans crash. Lancer : --path godot/project res://army_move_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame ; await get_tree().process_frame
	if Sim.world == null:
		push_error("army_move_audit: pas de monde") ; get_tree().quit(2) ; return
	var w = Sim.world
	print("=== ARMY MOVE AUDIT ===")
	w.generate(9)
	var me: int = w.player()
	w.advance_days(300)
	for i in range(6):
		w.player_recruit(0)   # lève une réserve (unité 0)
	w.advance_days(400)   # le recrutement aboutit
	# deux régions À SOI : capitale + une autre (destination de marche)
	var cap := int(w.country_capital_region(me))
	var dest := -1
	for r in range(w.region_count()):
		if int(w.region_owner(r)) == me and r != cap:
			dest = r ; break
	print("  joueur=%d · capitale_région=%d · dest(à soi)=%d" % [me, cap, dest])
	var a0: Dictionary = w.army_info(me)
	print("  AVANT : active=%s · région=%d" % [str(a0.get("active", false)), int(a0.get("region", -1))])
	var ok: bool = w.player_move_army(dest if dest >= 0 else cap)
	print("  player_move_army émis=%s" % str(ok))
	w.advance_days(120)
	var a1: Dictionary = w.army_info(me)
	print("  APRÈS : active=%s · région=%d · phase=%s" % [
		str(a1.get("active", false)), int(a1.get("region", -1)), String(a1.get("phase", "?"))])
	print("ARMY MOVE AUDIT DONE")
	get_tree().quit(0)
