extends Node
## perf_shot — CHASSE AU STUTTER (« la sim vitesse max stutter tous les 15 jours ») :
## boote le vrai shell, avance 60 ans, passe à VITESSE MAX et laisse tourner N secondes
## réelles avec SCPS_PERF=1 — les chronos (sim.gd advance / overlay road_network /
## build_dressing) impriment chaque dépassement avec le JOUR SIM : la périodicité du
## coupable se lit dans le log. Fenêtré (--headless = hang connu), unfocusable.
##   SCPS_PERF=1 Godot --audio-driver Dummy --path . res://perf_shot.tscn -- seed=9 secs=60

var _main: Node = null

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1280, 720)
	get_window().unfocusable = true   # ne VOLE PAS le focus (décision joueur)
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "9")))
	for i in range(30):
		await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	Sim.world.advance_days(360 * 60)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sim.game_on = true
	Sim.set_speed(Sim.SPEED_RATE.size() - 1)      # VITESSE MAX
	var secs := float(_arg("secs=", "60"))
	print("[PERF] départ jour=%d vitesse=%d — %d s de mesure" % [Sim.day_count, Sim.speed_index, int(secs)])
	await get_tree().create_timer(secs).timeout
	print("[PERF] fin jour=%d — RUN OK" % Sim.day_count)
	get_tree().quit()
