extends Node
## army_audit — MESURE le mécanisme de recrutement via la GDExtension LIVE (retour joueur
## « l'armée ne se recrute pas / n'est pas visible »). Enchaîne : lire la réserve → monter
## la levée → recompléter → laisser le temps drainer → relire. Imprime les nombres à chaque
## étape pour trancher : BUG mécanique (rien ne monte) vs UX (ça marche, mal visible).
## Lancer : godot --headless --path godot/project res://army_audit.tscn

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _dump(w, me: int, tag: String) -> void:
	var a: Dictionary = w.country_army(me)
	var ci: Dictionary = w.country_info(me)
	var ar: Dictionary = w.army_info(me)
	print("  [%s] régiments=%d · levée=%d(%s) · flotte=%d · or=%d · campagne=%s (units=%d)" % [
		tag, int(a.get("regiments", 0)), int(a.get("levy", 0)), String(a.get("levy_name", "?")),
		int(a.get("fleet", 0)), int(ci.get("or", 0)),
		str(bool(ar.get("active", false))), int(ar.get("units", 0))])

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("army_audit: pas de monde (libscps absente ?)"); get_tree().quit(2); return
	var w = Sim.world
	print("=== ARMY AUDIT — le recrutement mord-il ? ===")
	w.generate(9)
	# on laisse le monde mûrir un peu (économie/pop) pour avoir de quoi lever
	w.advance_days(3650)   # ~10 ans
	var me: int = w.player()
	var capr: int = w.province_region(w.country_capital_province(me))
	_dump(w, me, "an%d départ" % w.year())

	# 1) MONTER LA LEVÉE au max
	w.player_set_levy(3)
	w.advance_days(60)
	_dump(w, me, "levée→3 +60j")

	# 2) RECOMPLÉTER (paye or + matière) — plusieurs fois, en laissant le temps
	for i in range(4):
		w.player_refill()
		w.advance_days(180)
		_dump(w, me, "refill#%d +180j" % (i + 1))

	# 3) RECRUTER des unités par type (0..5) puis laisser bâtir
	for u in range(6):
		w.player_recruit(u)
	w.advance_days(365)
	_dump(w, me, "recruit 0-5 +365j")

	# 4) MOBILISER une campagne depuis la capitale vers une voisine → armée VISIBLE ?
	var tgt := -1
	for r in range(w.region_count()):
		if int(w.region_owner(r)) == me and r != capr:
			tgt = r; break
	if tgt < 0:
		tgt = capr
	w.player_campaign(capr, tgt)
	w.advance_days(120)
	_dump(w, me, "campagne cap→%d +120j" % tgt)

	print("ARMY AUDIT DONE")
	get_tree().quit(0)
