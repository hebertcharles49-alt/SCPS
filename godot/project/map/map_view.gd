extends Node2D
## MapView — la carte PARCHEMIN, rendu UNIQUE (DISPLAY-ONLY : lit Sim.world, n'écrit jamais
## dans la sim). Le sol est une carte de cartographe rendue TOTALEMENT par shader
## (iso_antique.gdshader) à partir des couches moteur (biome + rivières) : lavis sépia, côtes à
## l'encre, marais, rivières à la plume, relief en lavis, rose des vents, bords brûlés. Aucun
## sprite d'asset n'est posé sur la carte — les acteurs (villes, armées, frontières, routes) sont
## tracés en ENCRE vectorielle par l'overlay.
##
## Projection TOP-DOWN (le monde = l'écran, à un facteur d'inclinaison TILT_Y près) ; une Camera2D
## cadre/zoome. Il n'y a plus de vue GLOBE 3D ni de splat iso 3D : un seul rendu, à tous les zooms.

const Iso = preload("res://map/iso.gd")
const LAYER_HEIGHT := 0
const CLICK_SLOP := 5.0

# ── projection PARCHEMIN ──
const TILE_K := 5               ## cellules monde par tuile (ancres d'acteurs / entrée des routes)
const TILT_Y := 0.80            ## inclinaison : 1.0 = top-down pur · 0.80 = carte LÉGÈREMENT inclinée (Y comprimé)
const ZOOM_MIN := 0.55          ## le plus DÉZOOMÉ (carte entière, marge de papier visible)
const ZOOM_MAX := 16.0          ## le plus ZOOMÉ (on plonge dans un bourg)

# view_mode : conservé pour compat overlay (toujours ISO désormais ; le globe a été retiré).
enum { VIEW_GLOBE = 0, VIEW_ISO = 1 }

signal province_picked(province: int, region: int, owner: int)
signal mode_changed(m: int)     ## le mode render a changé (légende, sélecteurs)

var mode := 0                   ## ViewMode de carte (0 terrain · 1 politique · 2 régions · 3 pays)
var view_mode := VIEW_ISO       ## TOUJOURS ISO (compat overlay : le globe n'existe plus)

var _selected_prov := -1
var _press_pos := Vector2.ZERO
var _dragged := false
var _himg: Image                ## couche HEIGHT (figée) — lue par l'overlay (relief/ombres)

var _ground: Node2D             ## sol PARCHEMIN (iso_ground.gd, shader cartographique)
var _camera: Camera2D
var _overlay: Node2D            ## overlay des acteurs (frontières/villes/dressing) — toggle mode NATURE

func _ready() -> void:
	# SOL PARCHEMIN — le shader cartographique peint tout (display-only).
	_ground = Node2D.new()
	_ground.set_script(load("res://map/iso_ground.gd"))
	_ground.name = "IsoGround"
	_ground.scale = Vector2(1.0, TILT_Y)   # inclinaison : le sol est comprimé en Y comme l'overlay (iso_pos)
	add_child(_ground)

	# anti-crénelage 2D MATÉRIEL (côtes/encre nettes au zoom) — UNIQUEMENT si le renderer le supporte.
	# Le MSAA 2D n'existe PAS sous GL Compatibility (GLES3) : un RenderingDevice non-nul ⇒ Forward+/Mobile
	# (Vulkan/D3D12/Metal) qui, lui, le gère. Sous GL Compatibility on saute (sinon warning), et la netteté
	# vient du lissage géométrique des frontières + de l'antialiasing par-trait (draw_* `antialiased`).
	if RenderingServer.get_rendering_device() != null:
		get_viewport().msaa_2d = Viewport.MSAA_4X
	_camera = Camera2D.new()
	add_child(_camera)
	_camera.make_current()

	# overlay des ACTEURS (encre vectorielle), projeté via iso_pos
	var ov := Node2D.new()
	ov.set_script(load("res://map/overlay.gd"))
	ov.name = "Overlay"
	add_child(ov)
	_overlay = ov

	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_ticked)
	if Sim.world != null:
		_on_generated()
		fit()

# ════════════════════════ projection ════════════════════════
## hauteur (0..1) à la cellule monde — lue par l'overlay (ombres/relief éventuels).
func height_at(wx: float, wy: float) -> float:
	if _himg == null:
		return 0.0
	var hw := _himg.get_width()
	var hh := _himg.get_height()
	return _himg.get_pixel(clampi(int(wx), 0, hw - 1), clampi(int(wy), 0, hh - 1)).r

## monde → position ÉCRAN (TOP-DOWN : monde = écran, Y comprimé par TILT_Y pour l'inclinaison).
## Le sol ET l'overlay y passent → tout partage EXACTEMENT la même projection.
func iso_pos(wx: float, wy: float) -> Vector2:
	return Vector2(wx, wy * TILT_Y)

## écran → monde (inverse de iso_pos), pour le picking.
func unproj(sx: float, sy: float) -> Vector2:
	return Vector2(sx, sy / TILT_Y)

## SOMMET de la tuile contenant (wx,wy) — ancre des acteurs / entrée des routes (un seul point partagé).
func tile_anchor_world(wx: float, wy: float) -> Vector2:
	var col: int = int(wx) / TILE_K
	var row: int = int(wy) / TILE_K
	return Vector2(float((col + 1) * TILE_K), float((row + 1) * TILE_K))

func tile_anchor(wx: float, wy: float) -> Vector2:
	var a := tile_anchor_world(wx, wy)
	return iso_pos(a.x, a.y)

## COMPAT overlay (le globe a été retiré) : jamais visible → l'overlay ignore son chemin globe.
func globe_to_screen(_wx: float, _wy: float, _lift: float = 0.0) -> Dictionary:
	return {"pos": Vector2.ZERO, "vis": false}

# ════════════════════════ commun ════════════════════════
func _on_generated() -> void:
	_himg = Sim.world.layer_image(LAYER_HEIGHT)
	focus_player()   # nouveau monde → cadrer d'emblée sur la capitale du JOUEUR (« je joue qui / où »)

func _on_ticked(_year: int) -> void:
	pass

func set_mode(m: int) -> void:
	mode = m
	mode_changed.emit(m)

# ── navigation ───────────────────────────────────────────
func _input(event: InputEvent) -> void:
	# (touche N retirée — le toggle est dans la barre de modes controls.gd)
	if event is InputEventMouseMotion and (event.button_mask & (MOUSE_BUTTON_MASK_RIGHT | MOUSE_BUTTON_MASK_LEFT)):
		if event.position.distance_to(_press_pos) > CLICK_SLOP:
			_dragged = true
		_camera.position -= event.relative / _camera.zoom.x
		queue_redraw()
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			_zoom(0.84)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			_zoom(1.0 / 0.84)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			if event.pressed:
				_press_pos = event.position
				_dragged = false
			elif not _dragged and get_viewport().gui_get_hovered_control() == null:
				_pick_at_mouse()

## zoom CONTINU : facteur < 1 = on s'approche.
func _zoom(factor: float) -> void:
	var nz := _camera.zoom.x / factor
	nz = clampf(nz, ZOOM_MIN, ZOOM_MAX)
	_camera.zoom = Vector2(nz, nz)
	queue_redraw()

func _pick_at_mouse() -> void:
	if Sim.world == null or not Sim.world.has_method("province_at"):
		return
	var wp := unproj(get_global_mouse_position().x, get_global_mouse_position().y)
	var cx := int(floor(wp.x))
	var cy := int(floor(wp.y))
	if cx < 0 or cy < 0 or cx >= Sim.world.map_w() or cy >= Sim.world.map_h():
		return
	var prov: int = Sim.world.province_at(cx, cy)
	_selected_prov = prov
	if _ground != null:
		_ground.queue_redraw()
	var region := -1
	var owner := -1
	if prov >= 0:
		region = Sim.world.province_region(prov)
		if region >= 0:
			owner = Sim.world.region_owner(region)
	province_picked.emit(prov, region, owner)

# ── verbes publics (boutons de carte) ──
func toggle_nature() -> void:
	if _overlay != null:
		_overlay.nature_mode = not _overlay.nature_mode
		_overlay.queue_redraw()

func is_nature() -> bool:
	return _overlay != null and _overlay.nature_mode

func zoom_in() -> void:  _zoom(0.8)
func zoom_out() -> void: _zoom(1.0 / 0.8)

## « fit » = la carte ENTIÈRE cadrée dans la fenêtre (remplace l'ancienne vue d'ensemble GLOBE).
func fit() -> void:
	if Sim.world == null:
		return
	var W := float(Sim.world.map_w())
	var H := float(Sim.world.map_h()) * TILT_Y
	var vp := get_viewport_rect().size
	var z := minf(vp.x / maxf(W, 1.0), vp.y / maxf(H, 1.0)) * 0.96
	z = clampf(z, ZOOM_MIN, ZOOM_MAX)
	_camera.zoom = Vector2(z, z)
	_camera.position = iso_pos(W * 0.5, Sim.world.map_h() * 0.5)
	queue_redraw()

## FOCUS DÉPART : cadre la caméra sur la CAPITALE du joueur, à un zoom de lecture
## confortable (le joueur voit tout de suite QUI il joue et OÙ il commence — au lieu
## d'un plan large anonyme sur le monde entier).
const ZOOM_START := 4.0        ## ni plan large, ni bourg : on voit la capitale + son voisinage
func focus_player() -> void:
	if Sim.world == null or _camera == null:
		return
	var me: int = Sim.world.player()
	var cap: int = Sim.world.country_capital_region(me)
	if cap < 0:
		fit()
		return
	var c: Vector2 = Sim.world.region_centroid(cap)   # cellule monde
	var z := clampf(ZOOM_START, ZOOM_MIN, ZOOM_MAX)
	_camera.zoom = Vector2(z, z)
	_camera.position = iso_pos(c.x, c.y)
	queue_redraw()
