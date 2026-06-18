extends Node2D
## MapView — affiche la carte du moteur et la pilote en caméra. DISPLAY-ONLY :
## lit Sim.world (octets de carte), n'écrit jamais dans la sim.
##
## Construit ses nodes EN CODE (Terrain + Camera2D) — pas de .tscn à maintenir.

const LAYER_SEA := 1            ## scps_map_layer : 1 = SEA (pour le shader d'eau)
const WATER_SHADER := "res://shaders/water.gdshader"

## ViewMode (cf. scps_render.h) : 0 terrain · 1 politique · 2 régions · 3 pays
var mode := 0

var _terrain: Sprite2D
var _camera: Camera2D
var _sea_tex: ImageTexture

func _ready() -> void:
	_terrain = Sprite2D.new()
	_terrain.centered = false
	var mat := ShaderMaterial.new()
	var sh := load(WATER_SHADER)
	if sh != null:
		mat.shader = sh
		_terrain.material = mat
	add_child(_terrain)

	_camera = Camera2D.new()
	add_child(_camera)
	_camera.make_current()

	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_ticked)
	if Sim.world != null:
		_on_generated()

func _on_generated() -> void:
	# la couche SEA est figée par worldgen → on la pousse une fois au shader.
	_sea_tex = ImageTexture.create_from_image(Sim.world.layer_image(LAYER_SEA))
	var mat := _terrain.material as ShaderMaterial
	if mat != null:
		mat.set_shader_parameter("sea_tex", _sea_tex)
	_refresh_terrain()
	_fit_camera()

func _on_ticked(_year: int) -> void:
	_refresh_terrain()             # le monde évolue (couleurs régions/pop)

func _refresh_terrain() -> void:
	if Sim.world == null:
		return
	_terrain.texture = ImageTexture.create_from_image(Sim.world.map_image(mode))

func set_mode(m: int) -> void:
	mode = m
	_refresh_terrain()

func _fit_camera() -> void:
	var w := float(Sim.world.map_w())
	var h := float(Sim.world.map_h())
	_camera.position = Vector2(w, h) * 0.5
	var vp := get_viewport_rect().size
	var z: float = min(vp.x / w, vp.y / h)   # Camera2D : zoom = pixels/unité-monde
	_camera.zoom = Vector2(z, z)

# ── navigation : pan (clic-droit glissé) · zoom (molette) ──────────────────
func _unhandled_input(event: InputEvent) -> void:
	if _camera == null:
		return
	if event is InputEventMouseMotion and (event.button_mask & MOUSE_BUTTON_MASK_RIGHT):
		_camera.position -= event.relative / _camera.zoom
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_zoom(1.1)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_zoom(1.0 / 1.1)

func _zoom(factor: float) -> void:
	var z: float = clampf(_camera.zoom.x * factor, 0.2, 16.0)
	_camera.zoom = Vector2(z, z)
