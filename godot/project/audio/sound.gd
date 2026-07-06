extends Node
## Sound — l'AUTOLOAD SONORE de SCPS. Display-only, HORLOGE MUR : le déterminisme
## n'entend rien (aucune lecture ici ne mute le moteur ; on LIT la façade — endgame,
## âge — et on JOUE des WAV générés par tools/gen_sounds.py, régénérables au seed).
##
## Architecture :
##   · 3 bus (default_bus_layout.tres) : Master → UI / Ambiance / Moments.
##     `_ensure_buses()` recrée les bus en code si le layout n'a pas chargé (headless).
##   · `play(nom)` : one-shot sur le bus déduit du préfixe (moment_* → Moments,
##     le reste → UI), pool de players réutilisés.
##   · Ambiances : DEUX players en crossfade (~1.5 s, Tween horloge mur) ; le choix
##     de l'ambiance de base vient d'indices de SCÈNE (mer sous la caméra, foule si
##     panneau de province agitée) poussés par main.gd — lecture display du viewport,
##     jamais un état sim. L'ENTROPIE est une COUCHE séparée dont le volume suit
##     entropie_pct (endgame_info) ; une FIN (fin>0) remplace tout par son drone.
##   · Ducking : quand un Moment joue, l'Ambiance baisse (~-9 dB) puis remonte.
##   · Le TICK (Sim.ticked) est RATE-LIMITÉ (≥ 500 ms entre deux tocks — le tick
##     rythme, il ne mitraille pas) ; l'an nouveau joue la variante cloche.
##
## Robustesse headless : driver audio Dummy → les players jouent en silence sans
## crash ; toute ressource absente (WAV non importé) → no-op silencieux.

const DIR := "res://audio/"
const BUS_UI := "UI"
const BUS_AMB := "Ambiance"
const BUS_MOM := "Moments"

const TICK_MIN_MS := 500          ## rate-limit du tock (max ~2/s aux vitesses rapides)
const XFADE_S := 1.5              ## crossfade d'ambiance
const DUCK_DB := -9.0             ## atténuation de l'Ambiance sous un Moment
const ENTROPY_FROM := 25          ## % d'entropie où la couche d'inquiétude s'éveille

## fin (§27 endgame_info) → drone de fin. 1 EAU · 2 FROID · 3 RONCES · 4 ascension
## (le drone "sang" habille la 4e fin faute d'un drone dédié — le MOMENT ascension
## sonne la transition, le drone n'est que le lit de fond).
const FIN_DRONE := {1: "drone_eau", 2: "drone_hiver", 3: "drone_ronces", 4: "drone_sang"}

var _streams := {}                ## nom → AudioStream (chargés paresseusement)
var _ui_pool: Array = []          ## AudioStreamPlayer (bus UI)
var _mom_pool: Array = []         ## AudioStreamPlayer (bus Moments)
var _amb_a: AudioStreamPlayer
var _amb_b: AudioStreamPlayer
var _amb_on_a := true             ## quel player d'ambiance est « au premier plan »
var _amb_cur := ""                ## nom de l'ambiance de base active
var _amb_tw: Tween = null
var _entropy: AudioStreamPlayer   ## couche entropie (volume ∝ entropie_pct)
var _duck_db := 0.0               ## atténuation courante (ducking)
var _duck_tw: Tween = null

var _last_tick_ms := 0
var _last_year := -1
var _last_age := -1
var _last_fin := 0
var _hint_sea := false            ## poussé par main.gd (lecture display du viewport)
var _hint_crowd := false

const CFG_PATH := "user://audio.cfg"


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_ensure_buses()
	for i in 4:
		_ui_pool.append(_make_player(BUS_UI))
	for i in 2:
		_mom_pool.append(_make_player(BUS_MOM))
	_amb_a = _make_player(BUS_AMB)
	_amb_b = _make_player(BUS_AMB)
	_entropy = _make_player(BUS_AMB)
	_load_volumes()
	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(_on_generated)
	# l'ambiance par défaut : le vent sur la table du cartographe
	set_ambient("amb_wind")


func _make_player(bus: String) -> AudioStreamPlayer:
	var p := AudioStreamPlayer.new()
	p.bus = bus
	add_child(p)
	return p


## Les bus doivent exister même si default_bus_layout.tres n'a pas chargé (probe
## headless minimal) — on les recrée alors en code, routés vers Master.
func _ensure_buses() -> void:
	for bus in [BUS_UI, BUS_AMB, BUS_MOM]:
		if AudioServer.get_bus_index(bus) < 0:
			var idx := AudioServer.bus_count
			AudioServer.add_bus(idx)
			AudioServer.set_bus_name(idx, bus)
			AudioServer.set_bus_send(idx, "Master")


## charge (et met en cache) un stream ; les ambiances/drones sont mis en BOUCLE
## (le fichier a déjà sa couture invisible — crossfade au générateur).
func _stream(nom: String) -> AudioStream:
	if _streams.has(nom):
		return _streams[nom]
	var path := DIR + nom + ".wav"
	if not ResourceLoader.exists(path):
		_streams[nom] = null
		return null
	var s: AudioStream = load(path)
	if s is AudioStreamWAV and (nom.begins_with("amb_") or nom.begins_with("drone_")):
		var w: AudioStreamWAV = s
		w.loop_mode = AudioStreamWAV.LOOP_FORWARD
		w.loop_begin = 0
		var bytes_per_frame := 2 * (2 if w.stereo else 1)   # 16-bit PCM
		w.loop_end = w.data.size() / bytes_per_frame
	_streams[nom] = s
	return s


## ── ONE-SHOTS ────────────────────────────────────────────────────────────────
## joue `nom` (sans .wav) sur le bus déduit du préfixe. Silencieux si absent.
func play(nom: String) -> void:
	var s := _stream(nom)
	if s == null:
		return
	var pool := _mom_pool if nom.begins_with("moment_") else _ui_pool
	var player: AudioStreamPlayer = null
	for p in pool:
		if not p.playing:
			player = p
			break
	if player == null:
		player = pool[0]   # tout occupé : on vole le plus ancien
	player.stream = s
	player.play()
	if nom.begins_with("moment_"):
		_duck_for(s.get_length())


## ── DUCKING : l'Ambiance s'incline sous un Moment, puis remonte ─────────────
func _duck_for(seconds: float) -> void:
	if _duck_tw != null and _duck_tw.is_valid():
		_duck_tw.kill()
	_duck_db = DUCK_DB
	_apply_amb_volumes()
	_duck_tw = create_tween()
	_duck_tw.tween_interval(maxf(0.2, seconds * 0.6))
	_duck_tw.tween_method(func(v: float):
		_duck_db = v
		_apply_amb_volumes(), DUCK_DB, 0.0, 1.2)


## ── AMBIANCES ────────────────────────────────────────────────────────────────
## indices de scène (main.gd, 1 Hz, lecture display du viewport/panneaux).
func set_scene_hint(sea: bool, crowd: bool) -> void:
	_hint_sea = sea
	_hint_crowd = crowd
	_refresh_ambient()


func _desired_ambient() -> String:
	if _last_fin > 0:
		return FIN_DRONE.get(_last_fin, "drone_sang")
	if _hint_crowd:
		return "amb_crowd"
	if _hint_sea:
		return "amb_sea"
	return "amb_wind"


func _refresh_ambient() -> void:
	set_ambient(_desired_ambient())


## bascule l'ambiance de base en crossfade equal-ish (Tween horloge mur).
func set_ambient(nom: String) -> void:
	if nom == _amb_cur:
		return
	var s := _stream(nom)
	if s == null:
		return
	_amb_cur = nom
	var incoming := _amb_b if _amb_on_a else _amb_a
	var outgoing := _amb_a if _amb_on_a else _amb_b
	_amb_on_a = not _amb_on_a
	incoming.stream = s
	incoming.volume_db = -60.0
	incoming.play()
	if _amb_tw != null and _amb_tw.is_valid():
		_amb_tw.kill()
	_amb_tw = create_tween().set_parallel(true)
	_amb_tw.tween_method(func(t: float):
		incoming.volume_db = linear_to_db(clampf(t, 0.0001, 1.0)) + _duck_db,
		0.0, 1.0, XFADE_S)
	_amb_tw.tween_method(func(t: float):
		outgoing.volume_db = linear_to_db(clampf(1.0 - t, 0.0001, 1.0)) + _duck_db,
		0.0, 1.0, XFADE_S)
	_amb_tw.chain().tween_callback(func(): outgoing.stop())


## ré-applique duck + volume d'entropie (appelé par le ducking et le tick).
func _apply_amb_volumes() -> void:
	var front := _amb_a if _amb_on_a else _amb_b
	if front.playing and (_amb_tw == null or not _amb_tw.is_valid()):
		front.volume_db = _duck_db
	if _entropy.playing:
		_entropy.volume_db = _entropy_target_db() + _duck_db


var _entropy_pct := 0

func _entropy_target_db() -> float:
	# silencieux sous ENTROPY_FROM, monte vers 0 dB à 100 % (le VOLUME suit l'entropie)
	var t := clampf((float(_entropy_pct) - ENTROPY_FROM) / (100.0 - ENTROPY_FROM), 0.0, 1.0)
	return linear_to_db(clampf(t, 0.0001, 1.0))


## ── LE TICK & LES LECTURES DE FAÇADE (âge, endgame) ─────────────────────────
func _on_generated() -> void:
	_last_year = -1
	_last_age = -1
	_last_fin = 0
	_entropy_pct = 0
	if _entropy.playing:
		_entropy.stop()
	_refresh_ambient()


func _on_tick(year: int) -> void:
	if not Sim.game_on:
		_last_year = year
		return   # le monde-vitrine du menu ne sonne pas
	# — LE TOCK (rate-limité ; l'an nouveau → la variante cloche) —
	var now := Time.get_ticks_msec()
	if now - _last_tick_ms >= TICK_MIN_MS:
		_last_tick_ms = now
		if year != _last_year and _last_year >= 0:
			play("ui_tick_year")
		else:
			play("ui_tick")
	_last_year = year
	var w = Sim.world
	if w == null:
		return
	# — L'ÂGE : l'avènement sonne la cloche (une fois par âge advenu) —
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		var age := int(ag.get("age", -1))
		if age > _last_age:
			if _last_age >= 0:
				play("moment_age_bell")
			_last_age = age
	# — L'ENDGAME : couche entropie + drone de fin + ascension —
	if w.has_method("endgame_info"):
		var e: Dictionary = w.endgame_info()
		var fin := int(e.get("fin", 0))
		_entropy_pct = int(e.get("entropie_pct", 0))
		if fin != _last_fin:
			_last_fin = fin
			if fin == 4:
				play("moment_ascension")
			_refresh_ambient()          # drone_<fin> remplace tout
			if _entropy.playing:
				_entropy.stop()          # la fin a éclos : la couche d'augure se tait
		elif fin == 0:
			if _entropy_pct >= ENTROPY_FROM:
				if not _entropy.playing:
					var s := _stream("amb_entropy")
					if s != null:
						_entropy.stream = s
						_entropy.volume_db = -60.0
						_entropy.play()
				_entropy.volume_db = _entropy_target_db() + _duck_db
			elif _entropy.playing:
				_entropy.stop()


## ── VOLUMES (options) ────────────────────────────────────────────────────────
func get_vol(bus: String) -> float:
	var i := AudioServer.get_bus_index(bus)
	return db_to_linear(AudioServer.get_bus_volume_db(i)) if i >= 0 else 1.0


func set_vol(bus: String, linear: float) -> void:
	var i := AudioServer.get_bus_index(bus)
	if i >= 0:
		AudioServer.set_bus_volume_db(i, linear_to_db(clampf(linear, 0.0001, 1.0)))
	_save_volumes()


func _save_volumes() -> void:
	var cfg := ConfigFile.new()
	for bus in ["Master", BUS_UI, BUS_AMB, BUS_MOM]:
		cfg.set_value("volume", bus, get_vol(bus))
	cfg.save(CFG_PATH)


func _load_volumes() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(CFG_PATH) != OK:
		return
	for bus in ["Master", BUS_UI, BUS_AMB, BUS_MOM]:
		var v = cfg.get_value("volume", bus, null)
		if v != null:
			var i := AudioServer.get_bus_index(bus)
			if i >= 0:
				AudioServer.set_bus_volume_db(i, linear_to_db(clampf(float(v), 0.0001, 1.0)))
