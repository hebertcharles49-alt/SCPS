extends Node
## Sim — le SINGLETON (autoload) qui détient le moteur (ScpsWorld) et cadence le
## temps. Tout le front lit le monde À TRAVERS lui. AUCUNE logique de simulation
## ici : il APPELLE le moteur C (déterministe) et RELAIE des signaux.
##
## Enregistré comme autoload "Sim" dans project.godot.

signal generated()              ## le monde vient d'être (re)généré
signal ticked(year: int)        ## un pas de simulation vient d'avancer

const SEED_DEFAULT := 9

## CADENCE en jours simulés / SECONDE (index 0 = pause) — la baseline demandée :
## un AN (365 j) ≈ 100 s en v1 · 45 s en v2 · 20 s en v3. Accumulateur fractionnaire
## (3.6 j/s ne tombe pas rond par frame) ; sur les grosses sims un pas peut durer
## plus longtemps — la baseline est un PLANCHER de lenteur, pas une promesse.
## (Display-only : aucun impact sur le déterminisme — advance_days(N) roule la
## MÊME suite de sim_day, juste regroupée autrement.)
const SPEED_RATE := [0.0, 3.65, 8.1, 18.25]   ## pause · v1 · v2 · v3 (jours/seconde)
const SPEED_LABELS := ["❙❙", "▶ lent", "▶▶ normal", "▶▶▶ rapide"]
const CATCHUP_MAX := 30         ## jours max rattrapés d'un coup (lag spike → pas de rafale)

# `world` est UNTYPED à dessein : référencer le type `ScpsWorld` à la compilation
# ferait échouer l'OUVERTURE du projet si la GDExtension n'est pas encore bâtie.
# On l'instancie par NOM (ClassDB.instantiate) → le projet ouvre sans libscps, et
# le garde-fou ci-dessous donne un message clair au lieu d'un crash de parse.
var world = null                ## le handle moteur (GDExtension) ; null si libscps absente
var speed_index := 2            ## « normal » par défaut
var _last_speed := 2            ## dernière vitesse non-pause (pour la bascule Espace)
var _accum := 0.0
var day_count := 0              ## jours simulés CETTE SESSION (display-only : croissance des
                                ## routes & co à grain fin — seuls les DELTAS comptent)
var game_on := false            ## la PARTIE a commencé (Lancer/Charger) — avant : le monde de
                                ## fond tourne en vitrine, alertes & popups restent muets
var current_seed := SEED_DEFAULT ## graine du monde courant (contexte pour le rapport de bug)

func _ready() -> void:
	if not ClassDB.class_exists("ScpsWorld"):
		push_error("GDExtension `libscps` absente — bâtir d'abord : `cd godot && scons`. Voir godot/README.md.")
		return
	world = ClassDB.instantiate("ScpsWorld")
	regenerate(SEED_DEFAULT)

func regenerate(seed_value: int) -> void:
	if world == null:
		return
	current_seed = seed_value
	world.generate(seed_value)
	generated.emit()

## SAUVEGARDE (menu Charger). save_game → bool ; load_game → 0 ok/1 absent/2 ère antérieure
## (émet generated sur succès : la carte se redessine) ; save_slots → infos des 3 emplacements.
func save_game(slot: int) -> bool:
	return world.save_game(slot) if world != null else false

func load_game(slot: int) -> int:
	if world == null:
		return 1
	var rc: int = world.load_game(slot)
	if rc == 0:
		generated.emit()
	return rc

func save_slots() -> Array:
	return world.save_slots() if world != null else []

func _process(delta: float) -> void:
	if world == null:
		return
	var rate: float = SPEED_RATE[speed_index]
	if rate <= 0.0:
		return
	_accum += delta * rate
	var nd := int(_accum)
	if nd <= 0:
		return
	if nd > CATCHUP_MAX:            # gel/lag : on rattrape borné, le reste est abandonné
		nd = CATCHUP_MAX
		_accum = 0.0
	else:
		_accum -= float(nd)
	world.advance_days(nd)
	day_count += nd
	ticked.emit(world.year())

func set_speed(i: int) -> void:
	speed_index = clampi(i, 0, SPEED_RATE.size() - 1)
	if speed_index > 0:
		_last_speed = speed_index

func cycle_speed() -> void:
	set_speed((speed_index + 1) % SPEED_RATE.size())

## Espace : bascule pause ↔ dernière vitesse (parité viewer.c SDLK_SPACE)
func toggle_pause() -> void:
	set_speed(0 if speed_index > 0 else _last_speed)

## +/- : monter / descendre d'un cran (sans repasser par la pause en montant)
func faster() -> void:
	set_speed(mini(speed_index + 1, SPEED_RATE.size() - 1))

func slower() -> void:
	set_speed(maxi(speed_index - 1, 0))

func speed_label() -> String:
	return SPEED_LABELS[speed_index]
