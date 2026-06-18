extends Node2D
#
# main.gd — le SPIKE : le moteur C (GDExtension `ScpsWorld`) pilote l'affichage.
# Le moteur CALCULE (déterministe), Godot AFFICHE et SAISIT. C'est la membrane,
# version Godot : pas une ligne de simulation ici.
#

var sim: ScpsWorld

@onready var map: Sprite2D = $Map
@onready var hud: Label = $HUD

const SEED_VALUE := 9
const DAYS_PER_STEP := 30        # un mois simulé par tick d'affichage
const STEP_PERIOD := 0.20        # secondes entre deux pas
var _accum := 0.0

func _ready() -> void:
	sim = ScpsWorld.new()
	sim.generate(SEED_VALUE)

	# terrain (render_map) → texture
	map.texture = ImageTexture.create_from_image(sim.map_image(0))   # 0 = VIEW_TERRAIN
	map.centered = false

	# couche SEA → le shader d'eau (continuité eau↔asset)
	var sea_tex := ImageTexture.create_from_image(sim.layer_image(1))  # 1 = SEA
	(map.material as ShaderMaterial).set_shader_parameter("sea_tex", sea_tex)

	# cadrer la carte plein écran
	var vp := get_viewport_rect().size
	map.scale = Vector2(vp.x / float(sim.map_w()), vp.y / float(sim.map_h()))

	_refresh_hud()
	print("[scps] pays=%d régions=%d pop=%d joueur=%d" % [
		sim.country_count(), sim.region_count(), sim.world_pop(), sim.player()])

func _process(delta: float) -> void:
	_accum += delta
	if _accum < STEP_PERIOD:
		return
	_accum = 0.0
	sim.advance_days(DAYS_PER_STEP)               # le monde VIT
	map.texture = ImageTexture.create_from_image(sim.map_image(0))
	_refresh_hud()

func _refresh_hud() -> void:
	hud.text = "An %d · pop %d" % [sim.year(), sim.world_pop()]
