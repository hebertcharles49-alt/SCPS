extends Node2D
## MapView — affiche la carte du moteur et la pilote en caméra. DISPLAY-ONLY :
## lit Sim.world (octets de carte), n'écrit jamais dans la sim. Le clic SÉLECTIONNE
## une province (picking → province_at) ; la sélection est surlignée par render_map
## et émise en signal pour les panneaux.
##
## Construit ses nodes EN CODE (Terrain + Camera2D) — pas de .tscn à maintenir.

const LAYER_HEIGHT := 0         ## scps_map_layer : 0 = HEIGHT (pour le relief volumétrique)
const LAYER_SEA := 1            ## scps_map_layer : 1 = SEA (pour le shader d'eau)
const WATER_SHADER := "res://shaders/water.gdshader"
const CLICK_SLOP := 5.0         ## px : au-delà, le geste est un glissé (pas un clic)

## clic gauche → province sélectionnée (-1 = mer/hors-monde) + sa région et son pays
signal province_picked(province: int, region: int, owner: int)

## ViewMode (cf. scps_render.h) : 0 terrain · 1 politique · 2 régions · 3 pays
var mode := 0

var _terrain: Sprite2D
var _camera: Camera2D
var _sea_tex: ImageTexture
var _height_tex: ImageTexture
var _selected_prov := -1
var _press_pos := Vector2.ZERO
var _dragged := false

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

	# overlay des ACTEURS (villes + armées), en espace monde, AU-DESSUS du terrain
	var ov := Node2D.new()
	ov.set_script(load("res://map/overlay.gd"))
	ov.name = "Overlay"
	add_child(ov)

	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_ticked)
	if Sim.world != null:
		_on_generated()

func _on_generated() -> void:
	# couches SEA + HEIGHT figées par worldgen → poussées une fois au shader (eau + relief).
	_sea_tex = ImageTexture.create_from_image(Sim.world.layer_image(LAYER_SEA))
	_height_tex = ImageTexture.create_from_image(Sim.world.layer_image(LAYER_HEIGHT))
	var mat := _terrain.material as ShaderMaterial
	if mat != null:
		mat.set_shader_parameter("sea_tex", _sea_tex)
		mat.set_shader_parameter("height_tex", _height_tex)
		mat.set_shader_parameter("tex_px", Vector2(1.0 / Sim.world.map_w(), 1.0 / Sim.world.map_h()))
	_refresh_terrain()
	_fit_camera()

func _on_ticked(_year: int) -> void:
	_refresh_terrain()             # le monde évolue (couleurs régions/pop)

func _refresh_terrain() -> void:
	if Sim.world == null:
		return
	_terrain.texture = ImageTexture.create_from_image(Sim.world.map_image(mode, _selected_prov))

func set_mode(m: int) -> void:
	mode = m
	_refresh_terrain()

## convertit la souris (espace écran) en cellule MONDE, ou (-1,-1) hors-carte.
func _mouse_cell() -> Vector2i:
	var wpos := get_global_mouse_position()
	var cx := int(floor(wpos.x))
	var cy := int(floor(wpos.y))
	if cx < 0 or cy < 0 or cx >= Sim.world.map_w() or cy >= Sim.world.map_h():
		return Vector2i(-1, -1)
	return Vector2i(cx, cy)

func _pick_at_mouse() -> void:
	if Sim.world == null:
		push_warning("[SCPS] clic ignoré : Sim.world absente (libscps non chargée).")
		return
	if not Sim.world.has_method("province_at"):
		push_error("[SCPS] libscps OBSOLÈTE : méthode 'province_at' absente. " +
			"Rebâtir la GDExtension : cd godot && scons platform=windows use_mingw=yes")
		return
	var cell := _mouse_cell()
	var prov := -1
	if cell.x >= 0:
		prov = Sim.world.province_at(cell.x, cell.y)
	print("[SCPS] clic cellule=", cell, " → province=", prov)   # diag : visible dans l'Output
	_selected_prov = prov
	_refresh_terrain()
	var region := -1
	var owner := -1
	if prov >= 0:
		region = Sim.world.province_region(prov)
		if region >= 0:
			owner = Sim.world.region_owner(region)
	province_picked.emit(prov, region, owner)

func _fit_camera() -> void:
	var w := float(Sim.world.map_w())
	var h := float(Sim.world.map_h())
	_camera.position = Vector2(w, h) * 0.5
	var vp := get_viewport_rect().size
	var z: float = min(vp.x / w, vp.y / h)   # Camera2D : zoom = pixels/unité-monde
	_camera.zoom = Vector2(z, z)

# ── navigation : pan (clic-droit glissé) · zoom (molette) · sélection (clic gauche) ──
# On écoute dans _input (livraison GARANTIE) plutôt que _unhandled_input (qui peut
# être court-circuité selon le routage GUI) ; on ne picke pas si le curseur est sur
# un Control opaque (la topbar) via gui_get_hovered_control.
func _input(event: InputEvent) -> void:
	if _camera == null:
		return
	if event is InputEventMouseMotion and (event.button_mask & MOUSE_BUTTON_MASK_RIGHT):
		_camera.position -= event.relative / _camera.zoom
	elif event is InputEventMouseMotion and (event.button_mask & MOUSE_BUTTON_MASK_LEFT):
		if event.position.distance_to(_press_pos) > CLICK_SLOP:
			_dragged = true
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			_zoom(1.1)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			_zoom(1.0 / 1.1)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			if event.pressed:
				_press_pos = event.position
				_dragged = false
			elif not _dragged and get_viewport().gui_get_hovered_control() == null:
				_pick_at_mouse()        # relâché sans glisser, hors UI → c'est un clic carte

func _zoom(factor: float) -> void:
	var z: float = clampf(_camera.zoom.x * factor, 0.2, 16.0)
	_camera.zoom = Vector2(z, z)

# ── verbes publics pour les contrôles de carte (boutons habillés) ──────────
func zoom_in() -> void:  _zoom(1.25)
func zoom_out() -> void: _zoom(1.0 / 1.25)
func fit() -> void:      _fit_camera()
