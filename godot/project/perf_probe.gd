extends Node
## PERF PROBE — mesure le « paquet de jours » (retour joueur 2026-07-10 : « les jours
## s'écoulent par paquets de 8-9 ? »). Boote le vrai shell, vitesse NORMALE (8.1 j/s),
## 12 s de course : histogramme des jours avancés PAR FRAME + fps + coût moteur pur
## (60 jours chronométrés hors frame). Fenêtré (--headless = noir mais on ne capture
## pas — les timings restent valides pour le CPU-side).
##   Godot --path . res://perf_probe.tscn

var _main: Node = null
var _frames := 0
var _t := 0.0
var _last_days := 0
var _maxnd := 0
var _hist := {}
var _running := false

func _ready() -> void:
	get_window().size = Vector2i(1920, 1080)
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_start.call_deferred()

func _start() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	Sim.regenerate(9)
	for i in range(10):
		Sim.world.advance_days(360)   # an 10 : un monde qui vit (routes, armées)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(2)                  # ▶▶ normal (8.1 j/s)
	_last_days = Sim.day_count
	_running = true

func _process(_delta: float) -> void:
	if not _running:
		return
	_frames += 1
	_t += _delta
	var nd := Sim.day_count - _last_days
	_last_days = Sim.day_count
	_maxnd = maxi(_maxnd, nd)
	_hist[nd] = int(_hist.get(nd, 0)) + 1
	if _t >= 12.0:
		_running = false
		Sim.set_speed(0)
		var t0 := Time.get_ticks_usec()
		Sim.world.advance_days(60)
		var eng60 := (Time.get_ticks_usec() - t0) / 1000.0
		var days_total := 0
		for k in _hist.keys():
			days_total += int(k) * int(_hist[k])
		print("PERF frames=%d t=%.1fs fps=%.1f jours=%d max_nd=%d hist=%s moteur60j=%.1fms" % [
			_frames, _t, _frames / _t, days_total, _maxnd, str(_hist), eng60])
		get_tree().quit(0)
