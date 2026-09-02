extends Node
## doctrine_shot — capture du panneau DOCTRINES (P2) : slots · catalogue · détail.
## Probe de rendu hors make, FENÊTRÉE (--headless = noir). Génère un monde, avance
## quelques années (l'influence s'accumule), pilote le panneau par son état interne
## (_show_view/_cat_slot/_detail_id — probe assumée fragile aux renommages) et sauve
## 4 PNG dans shots_doctrines/.
##   Godot --audio-driver Dummy --path godot/project res://doctrine_shot.tscn -- seed=9
const OUT_DIR := "C:/Users/Charl/Desktop/SCPS-main/godot/project/shots_doctrines"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_run.call_deferred()

func _snap(path: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(path)
	print("[doctrine_shot] ", path)

func _run() -> void:
	DirAccess.make_dir_recursive_absolute(OUT_DIR)
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	for i in range(15):
		Sim.world.advance_days(360)
	Sim.generated.emit()

	var bg := ColorRect.new()
	bg.color = Color("2a2622")
	bg.anchor_right = 1.0
	bg.anchor_bottom = 1.0
	add_child(bg)
	var lay := CanvasLayer.new()
	add_child(lay)
	var panel: Control = load("res://ui/doctrine_panel.gd").new()
	lay.add_child(panel)
	if panel.has_method("open"):
		panel.open()
	panel.reset_size()
	panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()

	# 1) SLOTS, vierges
	await _snap(OUT_DIR + "/01_slots_vides.png")

	# adoption réelle par le verbe (enfilé → drainé au fil des jours)
	var me: int = Sim.world.player()
	Sim.world.doctrine_adopt(0, 10)          # Technologie au slot 0
	Sim.world.advance_days(40)
	Sim.world.doctrine_buy_idea(10)          # la 1re idée
	Sim.world.advance_days(40)
	panel.refresh()
	panel.reset_size()
	panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()

	# 2) SLOTS, un slot occupé (pastilles)
	await _snap(OUT_DIR + "/02_slots_occupe.png")

	# 3) CATALOGUE (17 cartes) pour le slot 1
	panel._cat_slot = 1
	panel._show_view(1)
	panel.reset_size()
	panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()
	await _snap(OUT_DIR + "/03_catalogue.png")

	# 4) DÉTAIL de la doctrine adoptée (fond + colonne d'idées)
	panel._detail_id = 10
	panel._detail_slot = 0
	panel._detail_name = "Technologie"
	panel._show_view(2)
	panel.reset_size()
	panel.position = ((Vector2(1600, 900) - panel.size) * 0.5).floor()
	await _snap(OUT_DIR + "/04_detail.png")

	print("[doctrine_shot] influence_info = ", Sim.world.influence_info(me))
	get_tree().quit(0)
