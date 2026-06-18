extends Node
## Sim — le SINGLETON (autoload) qui détient le moteur (ScpsWorld) et cadence le
## temps. Tout le front lit le monde À TRAVERS lui. AUCUNE logique de simulation
## ici : il APPELLE le moteur C (déterministe) et RELAIE des signaux.
##
## Enregistré comme autoload "Sim" dans project.godot.

signal generated()              ## le monde vient d'être (re)généré
signal ticked(year: int)        ## un pas de simulation vient d'avancer

const SEED_DEFAULT := 9

## jours simulés par PAS, selon la vitesse (index 0 = pause)
const SPEED_DAYS := [0, 30, 90, 180]
const SPEED_LABELS := ["❙❙", "▶ ×1", "▶▶ ×2", "▶▶▶ ×3"]
const STEP_PERIOD := 0.20       ## secondes entre deux pas

# `world` est UNTYPED à dessein : référencer le type `ScpsWorld` à la compilation
# ferait échouer l'OUVERTURE du projet si la GDExtension n'est pas encore bâtie.
# On l'instancie par NOM (ClassDB.instantiate) → le projet ouvre sans libscps, et
# le garde-fou ci-dessous donne un message clair au lieu d'un crash de parse.
var world = null                ## le handle moteur (GDExtension) ; null si libscps absente
var speed_index := 1
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

func cycle_speed() -> void:
	set_speed((speed_index + 1) % SPEED_DAYS.size())

func speed_label() -> String:
	return SPEED_LABELS[speed_index]
