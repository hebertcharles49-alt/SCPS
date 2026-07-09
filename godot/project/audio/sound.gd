extends Node
## Sound — l'AUTOLOAD SONORE, MINIMAL (façon Paradox : un clic = un son, jamais une nappe
## continue). HORLOGE MUR : le déterminisme n'entend rien. Tous les sons sont de VRAIS
## enregistrements (zéro synthé). `play(nom)` tire une variante `nom_1…` au hasard + un léger
## jitter pitch/gain par lecture (l'organique). Deux stings d'ÉVÈNEMENT seulement — le COR de
## guerre et la notif de RECHERCHE — ; tout le reste du feedback est un son de CLIC. Ressource
## absente (WAV/MP3 non importé) → no-op silencieux. Robuste headless (driver Dummy → silence).
##
## Bus (default_bus_layout.tres) : Master → UI / Moments (+ Ambiance conservé pour le curseur
## d'options, aucun player n'y route). `_ensure_buses()` les recrée en code si le layout n'a
## pas chargé (probe headless).

const DIR := "res://audio/"
const BUS_UI := "UI"
const BUS_MOM := "Moments"
const BUS_AMB := "Ambiance"       ## conservé pour l'option de volume (plus d'ambiance jouée)

const TICK_MIN_MS := 500          ## rate-limit du tock (max ~2/s aux vitesses rapides)

## ── ORGANIQUE : round-robin de variantes + jitter par lecture ──
const VARIANT_MAX := 6            ## on sonde nom_1 … nom_VARIANT_MAX au 1er usage
const PITCH_JITTER := 0.04        ## ±4 % de hauteur par lecture (sons à variantes)
const GAIN_JITTER_DB := 1.5       ## ±1.5 dB de gain par lecture

var _streams := {}                ## nom → AudioStream (chargés paresseusement)
var _variants := {}               ## nom de base → Array[String] de variantes (cache)
var _ui_pool: Array = []          ## AudioStreamPlayer (bus UI)
var _mom_pool: Array = []         ## AudioStreamPlayer (bus Moments)
var _music: AudioStreamPlayer = null   ## la SEULE nappe continue : le thème de menu (bus Ambiance)
var _music_name := ""             ## thème en cours (anti-redémarrage)

var _last_tick_ms := 0
var _last_year := -1
var _last_age := -1
var _last_research_target := -1   ## suivi de la cible de recherche (détecte la complétion)
var _last_research_prog := 0.0

const CFG_PATH := "user://audio.cfg"


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_ensure_buses()
	for i in 4:
		_ui_pool.append(_make_player(BUS_UI))
	for i in 2:
		_mom_pool.append(_make_player(BUS_MOM))
	_music = _make_player(BUS_AMB)   # la musique de menu route sur Ambiance (le bus de fond)
	_load_volumes()
	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(_on_generated)


func _make_player(bus: String) -> AudioStreamPlayer:
	var p := AudioStreamPlayer.new()
	p.bus = bus
	add_child(p)
	return p


## Les bus doivent exister même si default_bus_layout.tres n'a pas chargé (probe headless).
func _ensure_buses() -> void:
	for bus in [BUS_UI, BUS_AMB, BUS_MOM]:
		if AudioServer.get_bus_index(bus) < 0:
			var idx := AudioServer.bus_count
			AudioServer.add_bus(idx)
			AudioServer.set_bus_name(idx, bus)
			AudioServer.set_bus_send(idx, "Master")


## charge (et met en cache) un stream ; WAV d'abord, repli MP3 (un one-shot livré en mp3).
func _stream(nom: String) -> AudioStream:
	if _streams.has(nom):
		return _streams[nom]
	var path := DIR + nom + ".wav"
	if not ResourceLoader.exists(path):
		path = DIR + nom + ".mp3"
	if not ResourceLoader.exists(path):
		_streams[nom] = null
		return null
	_streams[nom] = load(path)
	return _streams[nom]


## découvre (et met en cache) les variantes d'un son : nom_1, nom_2, … tant que le fichier
## existe (jusqu'à VARIANT_MAX). [] si aucune → le fichier unique `nom` est joué tel quel.
func _variant_list(nom: String) -> Array:
	if _variants.has(nom):
		return _variants[nom]
	var found: Array = []
	for k in range(1, VARIANT_MAX + 1):
		if ResourceLoader.exists(DIR + nom + "_" + str(k) + ".wav"):
			found.append(nom + "_" + str(k))
		else:
			break
	_variants[nom] = found
	return found


## joue `nom` (sans extension) sur le bus déduit du préfixe (moment_* → Moments, sinon UI).
## Variantes (nom_1…nom_N) → une au hasard + jitter pitch/gain par lecture (l'organique) ;
## un nom sans variante joue son fichier unique, sans jitter destructeur. Silencieux si absent.
func play(nom: String) -> void:
	# Le son de FERMETURE de fenêtre est volontairement MUET (demande joueur) : ouvrir une
	# fenêtre sonne (ui_parchment_open), la refermer non. Point de contrôle unique.
	if nom == "ui_parchment_close":
		return
	var has_var := true
	var vs := _variant_list(nom)
	var pick := nom
	if vs.is_empty():
		has_var = false
	else:
		pick = vs[randi() % vs.size()]
	var s := _stream(pick)
	if s == null:
		if has_var:
			s = _stream(nom)   # repli : le fichier de base si aucune variante n'a chargé
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
	if has_var:
		player.pitch_scale = 1.0 + randf_range(-PITCH_JITTER, PITCH_JITTER)
		player.volume_db = randf_range(-GAIN_JITTER_DB, GAIN_JITTER_DB)
	else:
		player.pitch_scale = 1.0
		player.volume_db = 0.0
	player.play()


## ── MUSIQUE DE FOND (menu) ────────────────────────────────────────────────────
## La SEULE nappe continue du jeu (le reste = clics façon Paradox) : le thème de menu,
## en boucle, sur le bus Ambiance (déjà exposé au volume). OGG chargé paresseusement
## (importé par Godot) ; loop forcé en code. Absent / non importé → no-op silencieux.
func play_music(nom: String) -> void:
	if _music == null:
		return
	if _music_name == nom and _music.playing:
		return                        # déjà en train de jouer ce thème
	var path := DIR + nom + ".ogg"
	if not ResourceLoader.exists(path):
		return
	var s: AudioStream = load(path)
	if s == null:
		return
	if s is AudioStreamOggVorbis:
		s.loop = true                 # boucle garantie quel que soit le réglage d'import
	_music.stream = s
	_music.pitch_scale = 1.0
	_music.volume_db = 0.0
	_music.play()
	_music_name = nom


func stop_music() -> void:
	if _music != null:
		_music.stop()
	_music_name = ""


## ── LE TICK & LES LECTURES DE FAÇADE (âge, recherche) ───────────────────────
func _on_generated() -> void:
	_last_year = -1
	_last_age = -1
	_last_research_target = -1
	_last_research_prog = 0.0


func _on_tick(year: int) -> void:
	if not Sim.game_on:
		_last_year = year
		return   # le monde-vitrine du menu ne sonne pas
	# — LE TOCK (rate-limité) : le month-tick à CHAQUE tock (pas de year-tick distinct) —
	var now := Time.get_ticks_msec()
	if now - _last_tick_ms >= TICK_MIN_MS:
		_last_tick_ms = now
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
	# — RECHERCHE TERMINÉE → notif : la cible passe de PRESQUE PLEINE (≥85 %) à autre/-1
	#   (une tech vient d'être débloquée). Annuler/switcher à bas % ne sonne pas.
	if w.has_method("research_status"):
		var rs: Dictionary = w.research_status()
		var rt := int(rs.get("target", -1))
		if _last_research_target >= 0 and rt != _last_research_target and _last_research_prog >= 0.85:
			play("tech_notif")
		_last_research_target = rt
		_last_research_prog = float(rs.get("progress", 0.0))


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
