extends Node
## Sim — le SINGLETON (autoload) qui détient le moteur (ScpsWorld) et cadence le
## temps. Tout le front lit le monde À TRAVERS lui. AUCUNE logique de simulation
## ici : il APPELLE le moteur C (déterministe) et RELAIE des signaux.
##
## Enregistré comme autoload "Sim" dans project.godot.

signal generated()              ## le monde vient d'être (re)généré
signal ticked(year: int)        ## un pas de simulation vient d'avancer

const SEED_DEFAULT := 9

## jours simulés par PAS, selon la vitesse (index 0 = pause). Ticks ALLONGÉS pour
## un déroulé OBSERVABLE : à « normal » l'an 100 prend ~8 min (et non ~20 s) ; on
## peut accélérer. (Display-only : aucun impact sur le déterminisme — advance_days(N)
## roule la MÊME suite de sim_day, juste regroupée autrement.)
const SPEED_DAYS := [0, 7, 20, 50]   ## pause · lent · normal · rapide (jours/pas)
const SPEED_LABELS := ["❙❙", "▶ lent", "▶▶ normal", "▶▶▶ rapide"]
const STEP_PERIOD := 0.25       ## secondes entre deux pas

# `world` est UNTYPED à dessein : référencer le type `ScpsWorld` à la compilation
# ferait échouer l'OUVERTURE du projet si la GDExtension n'est pas encore bâtie.
# On l'instancie par NOM (ClassDB.instantiate) → le projet ouvre sans libscps, et
# le garde-fou ci-dessous donne un message clair au lieu d'un crash de parse.
var world = null                ## le handle moteur (GDExtension) ; null si libscps absente
var speed_index := 2            ## « normal » par défaut
var _last_speed := 2            ## dernière vitesse non-pause (pour la bascule Espace)
var _accum := 0.0

func _ready() -> void:
	if not ClassDB.class_exists("ScpsWorld"):
		push_error("GDExtension `libscps` absente — bâtir d'abord : `cd godot && scons`. Voir godot/README.md.")
		return
	world = ClassDB.instantiate("ScpsWorld")
	regenerate(SEED_DEFAULT)

func regenerate(seed_value: int) -> void:
	if world == null:
		return
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
	var step: int = SPEED_DAYS[speed_index]
	if step <= 0:
		return
	_accum += delta
	if _accum < STEP_PERIOD:
		return
	_accum = 0.0
	world.advance_days(step)
	ticked.emit(world.year())

func set_speed(i: int) -> void:
	speed_index = clampi(i, 0, SPEED_DAYS.size() - 1)
	if speed_index > 0:
		_last_speed = speed_index

func cycle_speed() -> void:
	set_speed((speed_index + 1) % SPEED_DAYS.size())

## Espace : bascule pause ↔ dernière vitesse (parité viewer.c SDLK_SPACE)
func toggle_pause() -> void:
	set_speed(0 if speed_index > 0 else _last_speed)

## +/- : monter / descendre d'un cran (sans repasser par la pause en montant)
func faster() -> void:
	set_speed(mini(speed_index + 1, SPEED_DAYS.size() - 1))

func slower() -> void:
	set_speed(maxi(speed_index - 1, 0))

func speed_label() -> String:
	return SPEED_LABELS[speed_index]
