extends Node
## verbs_audit — pendant headless du WIRING COMPLET (§3). Prouve, à travers la GDExtension
## LIVE : (1) les 13 verbes fraîchement bindés EXISTENT ; (2) chacun s'enfile puis se
## draine SANS crash (le drain revalide — un refus silencieux est un succès de plomberie) ;
## (3) le verbe COLONISER mute vraiment (+1 province, la seule assertion d'état forte ici —
## les autres mutations dépendent de l'état du monde, couvertes par scps_api_demo côté C).
## Lancer : godot --headless res://verbs_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("verbs_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== VERBS AUDIT — wiring complet du binding (§3) ===")
	var viol := 0

	# INVARIANT 0 : les 13 méthodes existent au binding
	for m in ["player_repress", "player_assimilate", "player_purge",
			"player_council_hire", "player_council_dismiss",
			"player_route", "player_market_buy", "player_market_sell",
			"player_campaign", "player_posture", "player_refill",
			"player_navy_build", "player_disband", "player_set_levy"]:
		if not w.has_method(m):
			push_error("verbs_audit: méthode absente : " + m); viol += 1
	if viol > 0:
		get_tree().quit(1); return
	print("  ✓ 14 méthodes présentes au binding")

	w.generate(9)
	var me: int = w.player()
	var capr: int = w.province_region(w.country_capital_province(me))
	if capr < 0:
		push_error("verbs_audit: pas de région-capitale"); get_tree().quit(1); return

	# INVARIANT 1 : chaque verbe s'ENFILE puis se DRAINE sans crash
	w.player_repress(capr)
	w.player_assimilate(capr, false)
	w.player_purge(capr)
	w.player_council_hire(0, 0)
	w.player_council_dismiss(1)
	w.player_route(capr, (capr + 1) % w.region_count(), false)
	w.player_market_buy(capr, 1, 10, 0)
	w.player_market_sell(capr, 1, 5, 0)
	w.player_campaign(capr, capr)
	w.player_posture(2)
	w.player_refill()
	w.player_navy_build(1)
	w.player_set_levy(2)
	w.player_disband()
	w.advance_days(3)
	print("  ✓ 14 verbes enfilés + drainés sans crash (an %d)" % w.year())

	# INVARIANT 2 : COLONISER mute (+1 province au joueur)
	var before: int = w.country_province_count(me)
	var tgt := -1
	for pp in range(w.province_count()):
		if w.can_colonize(pp):
			tgt = pp; break
	if tgt < 0:
		push_error("verbs_audit: aucune cible colonisable"); viol += 1
	else:
		w.player_colonize(tgt)
		w.advance_days(2)
		var after: int = w.country_province_count(me)
		print("  colonisation : %d → %d provinces" % [before, after])
		if after != before + 1:
			push_error("verbs_audit: coloniser n'a pas mordu"); viol += 1

	if viol == 0:
		print("VERBS AUDIT OK")
		get_tree().quit(0)
	else:
		get_tree().quit(1)
