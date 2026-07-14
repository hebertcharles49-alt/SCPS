extends Node
## tech_shot — capture de l'arbre de technologie Civ 6 (tech_panel.gd) + du popup de
## découverte (tech_popup.gd). Probe de rendu hors make, FENÊTRÉ (--headless = noir).
## Sauve trois PNG 1600×900 : l'arbre seul, l'arbre avec un survol simulé (tooltip),
## et le popup de découverte.
##   Godot --path godot/project res://tech_shot.tscn -- seed=42 years=60

const OUTDIR := "C:/Users/Charl/Desktop/SCPS-main/build/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

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
	for i in range(int(_arg("years=", "60"))):
		Sim.world.advance_days(360)
	Sim.generated.emit()

	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)

	var lay := CanvasLayer.new()
	add_child(lay)
	var tp = load("res://ui/tech_panel.gd").new()
	# le VRAI chrome (comme main.gd), posé sur le Control directement — get_window().theme=…
	# ne propage pas de façon fiable quand cette scène tourne seule en racine (cf. army_panel_shot).
	tp.theme = load("res://ui/ui_theme.gd").build()
	lay.add_child(tp)
	for i in range(4):
		await get_tree().process_frame
	tp.visible = true
	for i in range(10):
		await get_tree().process_frame

	var ok_all := true
	ok_all = (await _save(OUTDIR + "tech_tree.png")) and ok_all
	print("nœuds : ", tp._nodes.size(), " | cartes posées : ", tp._cards.size(),
		" | largeur contenu : ", tp._content_w)

	# ── survol simulé : une carte recherchable (ou la première dispo) ─────────
	var target_idx := -1
	for i in tp._nodes.size():
		if int(tp._nodes[i].get("state", 0)) == 1:
			target_idx = i
			break
	if target_idx < 0 and tp._nodes.size() > 0:
		target_idx = 0
	if target_idx >= 0 and tp._cards[target_idx] != null:
		var card = tp._cards[target_idx]
		var center: Vector2 = card.global_position + card.size * 0.5
		print("survol simulé sur : ", String(tp._nodes[target_idx].get("name", "?")), " @ ", center)
		var ev := InputEventMouseMotion.new()
		ev.position = center
		ev.global_position = center
		Input.parse_input_event(ev)
		await get_tree().create_timer(0.65).timeout
		for i in range(4):
			await get_tree().process_frame
	ok_all = (await _save(OUTDIR + "tech_tree_hover.png")) and ok_all

	# ── dossier persistant (footer) : clic sur une carte, SANS flavor ─────────
	if target_idx >= 0:
		tp._on_card_activated(target_idx)
		for i in range(3):
			await get_tree().process_frame
	ok_all = (await _save(OUTDIR + "tech_footer.png")) and ok_all

	# ── popup de découverte (déclenché directement pour la capture, motif
	# déterministe — la détection réelle dépend de research_status() en jeu) ──
	if tp._nodes.size() > 0:
		tp._queue_discovery(target_idx if target_idx >= 0 else 0)   # motif réel (move_child + show_tech)
		for i in range(4):
			await get_tree().process_frame
	ok_all = (await _save(OUTDIR + "tech_popup.png")) and ok_all

	get_tree().quit(0 if ok_all else 1)

func _save(path: String) -> bool:
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(path)
	if err == OK:
		print("SAVED ", path, " (", img.get_width(), "x", img.get_height(), ")")
		return true
	push_error("save_png failed err=%d for %s" % [err, path])
	return false
