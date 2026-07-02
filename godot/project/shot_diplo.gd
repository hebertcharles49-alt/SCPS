extends Node
## shot_diplo — capture du TIROIR DIPLOMATIE (opinion + résumé + journal d'actes).
## Fenêtré (--headless donne du noir). Monte la scène main, ferme le menu, déclare
## une guerre + 60 j (le journal se peuple), ouvre l'onglet 6, capture.
## Lancer : Godot --path godot/project res://shot_diplo.tscn -- out=diplo.png

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
	var main: Node = load("res://main/Main.tscn").instantiate()
	add_child(main)
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("shot_diplo: pas de monde"); get_tree().quit(1); return
	var w = Sim.world
	# menu refermé (la partie « commence »)
	if main._menu != null:
		main._menu.visible = false
	# une GUERRE + du temps : le journal d'actes a de la matière (amorce 35 j d'abord)
	w.advance_days(35)
	var me: int = w.player()
	for c in range(w.country_count()):
		if c != me and int(w.country_province_count(c)) > 0:
			w.player_declare_war(c)   # enfilé ; le drain revalide (certains refusent)
	w.advance_days(60)
	# l'onglet DIPLOMATIE (6) s'ouvre
	main._sidebar.open_tab(6)
	for i in range(10):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var out: String = "res://" + _arg("out=", "diplo_panel.png")
	get_viewport().get_texture().get_image().save_png(out)
	print("SAVED ", out)
	get_tree().quit(0)
