extends Node
## army_panel_shot — capture du PANNEAU ARMÉE (army_panel.gd) : composition/actions
## hors combat, PUIS provoque une vraie guerre (déclaration + marche d'un corps sur
## une région ennemie) pour capturer la SECTION COMBAT en direct (siège/bataille),
## puis la conclusion (résultat + pertes). Motif province_shot : FENÊTRÉ (--headless
## = noir), PNG 1600×900 dans build/.
##   Godot --path godot/project res://army_panel_shot.tscn -- seed=42 years=90
const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _shot(panel: Control, fname: String) -> bool:
	panel.reset_size()
	for i in range(4):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var path := OUTDIR + fname
	var err := img.save_png(path)
	if err == OK:
		print("SAVED ", path, " (", img.get_width(), "x", img.get_height(), ")")
		return true
	push_error("save_png failed err=%d for %s" % [err, fname])
	return false

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "42")))
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	var w = Sim.world
	for i in range(int(_arg("years=", "90"))):
		w.advance_days(360)
	Sim.generated.emit()
	print("AN ", w.year())

	var ok_all := true
	var me: int = int(w.player())
	var capr: int = int(w.country_capital_region(me))
	print("me=", me, " capital_region=", capr)

	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)
	var lay := CanvasLayer.new()
	add_child(lay)
	var panel: Control = load("res://ui/army_panel.gd").new()
	# le VRAI chrome (comme main.gd) : posé sur le Control lui-même — get_window().theme=…
	# ne propage PAS de façon fiable quand la scène tourne seule en racine (pas de retard de
	# frame qui tienne ; en jeu réel, army_panel est un enfant PROFOND de Main.tscn, déjà
	# posé APRÈS plusieurs frames de démarrage — ce que ce probe reproduit en visant le
	# Control directement plutôt que la fenêtre).
	panel.theme = load("res://ui/ui_theme.gd").build()
	lay.add_child(panel)
	for i in range(6):
		await get_tree().process_frame

	# ── un corps du joueur : celui qui existe déjà, sinon on en lève un ──────
	var ids: Array = w.corps_ids(me)
	if ids.is_empty():
		var reserve: int = int(w.country_army(me).get("regiments", 0))
		print("reserve initiale=", reserve)
		if reserve <= 0:
			w.player_set_levy(3)
			for u in range(6):
				w.player_recruit(u)
			for i in range(6):
				w.player_refill()
				w.advance_days(180)
				print("  refill#%d reserve=%d" % [i + 1, int(w.country_army(me).get("regiments", 0))])
			reserve = int(w.country_army(me).get("regiments", 0))
		if reserve > 0:
			var packets: int = maxi(1, int(reserve / 2))
			var raised: bool = w.player_raise_corps(packets, capr)
			print("raise packets=", packets, " ok=", raised)
			w.advance_days(30)
			ids = w.corps_ids(me)
		if ids.is_empty():
			# repli prouvé (army_audit) : mobiliser une CAMPAGNE depuis la capitale.
			var tgt := -1
			for r in range(w.region_count()):
				if int(w.region_owner(r)) == me and r != capr:
					tgt = r; break
			if tgt < 0: tgt = capr
			var camp: bool = w.player_campaign(capr, tgt)
			print("campaign cap→", tgt, " ok=", camp)
			w.advance_days(60)
			ids = w.corps_ids(me)
	print("corps_ids=", ids)

	# ── état HORS COMBAT : composition, actions, section combat VIDE ────────
	panel.set_army(ids)
	for i in range(6):
		await get_tree().process_frame
	panel.position = Vector2(20, 20)
	ok_all = await _shot(panel, "army_panel_no_combat.png") and ok_all

	if ids.is_empty():
		push_error("aucun corps levé — impossible de mesurer la guerre")
		get_tree().quit(1 if not ok_all else 2)
		return
	var cid: int = int(ids[0])

	# ── provoque une guerre RÉELLE : déclare sur un voisin possédant une région,
	# puis marche le corps dessus (la section combat s'allume au siège) ──────
	var target := -1
	var target_region := -1
	for r in range(w.region_count()):
		var o := int(w.region_owner(r))
		if o >= 0 and o != me:
			var opt: Dictionary = w.diplo_options(o)
			if bool(opt.get("can_declare_war", false)):
				target = o; target_region = r; break
	if target < 0:
		# repli : n'importe quel pays étranger vivant, guerre déjà possible ou pas —
		# on tente quand même (can_declare_war peut être faux pour trêve ; on log et on continue).
		for r in range(w.region_count()):
			var o2 := int(w.region_owner(r))
			if o2 >= 0 and o2 != me:
				target = o2; target_region = r; break
	print("target=", target, " target_region=", target_region)

	var battle_shot_live := false
	var battle_shot_done := false
	if target >= 0 and target_region >= 0:
		var decl: bool = w.player_declare_war(target)
		print("declare_war ok=", decl)
		w.advance_days(10)
		var mv: bool = w.player_move_corps(cid, target_region)
		print("move_corps ok=", mv)

		# avance jusqu'à ce que la section combat s'allume (siège/bataille), plafonné.
		for step in range(60):
			w.advance_days(30)
			panel.set_army(ids)
			for i in range(2):
				await get_tree().process_frame
			var bi: Dictionary = w.battle_info(target_region)
			if bool(bi.get("valid", false)):
				battle_shot_live = true
				print("COMBAT VIVANT capté au pas ", step, " · phase=", bi.get("phase", "?"), " in_battle=", bi.get("in_battle", false))
				break
		if battle_shot_live:
			for i in range(6):
				await get_tree().process_frame
			panel.position = Vector2(20, 20)
			ok_all = await _shot(panel, "army_panel_combat_live.png") and ok_all

			# avance jusqu'à la CONCLUSION (battle_info retombe invalide), plafonné.
			for step in range(120):
				w.advance_days(30)
				panel.set_army(ids)
				for i in range(2):
					await get_tree().process_frame
				var bi2: Dictionary = w.battle_info(target_region)
				if not bool(bi2.get("valid", false)):
					battle_shot_done = true
					print("COMBAT CONCLU capté au pas ", step)
					break
			if battle_shot_done:
				for i in range(6):
					await get_tree().process_frame
				panel.position = Vector2(20, 20)
				ok_all = await _shot(panel, "army_panel_combat_result.png") and ok_all

	if not battle_shot_live:
		push_error("RESTES : aucun combat vivant n'a pu être provoqué dans la fenêtre de pas — voir TROUVAILLES")
	if battle_shot_live and not battle_shot_done:
		push_error("RESTES : combat resté ouvert au-delà de la fenêtre de pas — pas de capture 'résultat'")

	print("ARMY PANEL SHOT DONE — live=", battle_shot_live, " done=", battle_shot_done)
	get_tree().quit(0 if ok_all else 1)
