extends Node2D
## MapView — DEUX rendus du même monde, reliés par un zoom CONTINU (DISPLAY-ONLY : lit
## Sim.world, n'écrit jamais dans la sim) :
##
##   · GLOBE (vue d'ensemble, tout en haut du zoom) — la carte 1024×512 équirectangulaire
##     projetée sur une SPHÈRE 3D (SubViewport + Camera3D), reliefée par l'altitude, éclairée.
##     Sert à SE REPÉRER : l'overlay n'y montre que FRONTIÈRES + NOMS d'empire.
##   · ISO (surface de JEU) — le sol est ISOIFIÉ : une GRILLE de TUILES iso (losange canonique
##     128×64), un par bloc de TILE_K cellules, couleur PLATE par tuile (les côtes ÉPOUSENT la
##     grille au lieu de la lisser) + texture/dithering/ombrage iso par shader. Les centres-villes
##     & bâtiments s'ancrent au SOMMET BAS du losange ; les routes ENTRENT par ce même sommet.
##     Piloté par une Camera2D. C'est là que se LIT le jeu : routes & assets (bourg).
##
## Zoom DEHORS au maximum sur l'ISO ⇒ on bascule au GLOBE (au point regardé). Zoom DEDANS sur
## le GLOBE ⇒ on bascule à l'ISO (au point regardé). Un seul geste de molette traverse les deux.

const Iso = preload("res://map/iso.gd")
const LAYER_HEIGHT := 0
const LAYER_SEA := 1
const WATER_SHADER := "res://shaders/water.gdshader"
const CLICK_SLOP := 5.0

# ── GLOBE ──
const SEG_LON := 192
const SEG_LAT := 96
const RELIEF := 0.05
const GLOBE_FAR := 6.0          ## le plus loin (Terre entière, petite)
const GLOBE_NEAR := 2.6         ## le plus près du globe AVANT de basculer en ISO
# ── ISO ── le sol parle ISO : GRILLE de tuiles losange (empreinte canonique 128×64), une par
# bloc de TILE_K cellules. Couleur PLATE par tuile (côtes EN ESCALIER sur la grille, pas lissées)
# + shader de texture (dithering/herbe/roches/variation) & ombrage iso. Sommet BAS = ancre des
# bâtiments ET entrée des routes (l'orientation officielle, calée sur les centres-villes).
const TILE_K := 5               ## cellules monde par tuile iso (la maille ; bandes côtières en escalier)
const ISO_FAR := 4.0            ## zoom Camera2D à l'ENTRÉE en ISO (≈ l'échelle du globe au seuil) ;
                               ## dézoomer dessous rebascule au GLOBE. Les assets sont DÉJÀ lisibles ici.
const ISO_NEAR := 16.0          ## le plus zoomé (on plonge dans un bourg)

enum { VIEW_GLOBE = 0, VIEW_ISO = 1 }

signal province_picked(province: int, region: int, owner: int)

var mode := 0                   ## ViewMode de carte (0 terrain · 1 politique · 2 régions · 3 pays)
var view_mode := VIEW_GLOBE     ## GLOBE (overview) ou ISO (jeu) — lu par l'overlay

var _selected_prov := -1
var _press_pos := Vector2.ZERO
var _dragged := false
var _himg: Image                ## couche HEIGHT (figée) — relief globe + hillshade iso par tuile

# globe
var _sv: SubViewport
var _disp: Sprite2D
var _cam3d: Camera3D
var _globe: MeshInstance3D
var _albedo: ImageTexture
var _yaw := 0.0
var _pitch := 0.35
var _cam_dist := GLOBE_FAR

# iso
var _terrain: MeshInstance2D
var _ground: Node2D               ## sol en TUILES iso (si assets présents ; sinon repli procédural)
var _camera: Camera2D
var _height_tex: ImageTexture

func _ready() -> void:
	# ---- GLOBE : SubViewport 3D + sprite d'affichage ----
	_sv = SubViewport.new()
	_sv.transparent_bg = false
	_sv.msaa_3d = Viewport.MSAA_4X
	_sv.size = Vector2i(get_viewport_rect().size)
	add_child(_sv)
	var world := World3D.new()
	_sv.world_3d = world
	_cam3d = Camera3D.new()
	_cam3d.fov = 38.0
	_sv.add_child(_cam3d)
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(deg_to_rad(-55.0), deg_to_rad(-40.0), 0.0)
	sun.light_energy = 1.25
	_sv.add_child(sun)
	var amb := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.03, 0.04, 0.07)
	env.ambient_light_color = Color(0.5, 0.55, 0.62)
	env.ambient_light_energy = 0.45
	amb.environment = env
	_sv.add_child(amb)
	_globe = MeshInstance3D.new()
	_sv.add_child(_globe)
	_disp = Sprite2D.new()
	_disp.centered = false
	_disp.texture = _sv.get_texture()
	add_child(_disp)

	# ---- ISO : maillage LISSE à volumétrie + Camera2D ----
	_terrain = MeshInstance2D.new()
	# filtre LINÉAIRE = blend à fond (couleur lissée, pas de « pixel » au zoom).
	_terrain.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	# shader : albédo × ombrage de sommet + houle d'eau (le hillshade reste dans COLOR de sommet).
	var matw := ShaderMaterial.new()
	var sh := load(WATER_SHADER)
	if sh != null:
		matw.shader = sh
		_terrain.material = matw
	_terrain.visible = false
	add_child(_terrain)
	# SOL EN TUILES ISO (par-dessus le sol procédural ; exclusif via has_iso_tiles). Display-only.
	_ground = Node2D.new()
	_ground.set_script(load("res://map/iso_ground.gd"))
	_ground.name = "IsoGround"
	_ground.visible = false
	add_child(_ground)
	# anti-crénelage 2D (silhouette du relief + bords d'assets nets au zoom).
	get_viewport().msaa_2d = Viewport.MSAA_4X
	_camera = Camera2D.new()
	_camera.enabled = false           # le GLOBE est la vue de départ → pas de caméra 2D
	add_child(_camera)

	# overlay des ACTEURS — projeté selon le view_mode actif
	var ov := Node2D.new()
	ov.set_script(load("res://map/overlay.gd"))
	ov.name = "Overlay"
	add_child(ov)

	get_viewport().size_changed.connect(_on_resize)
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_ticked)
	if Sim.world != null:
		_on_generated()

func _on_resize() -> void:
	if _sv != null:
		_sv.size = Vector2i(get_viewport_rect().size)

# ════════════════════════ GLOBE ════════════════════════
func _build_globe_mesh() -> void:
	var himg := _himg
	var hw := himg.get_width()
	var hh := himg.get_height()
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for j in range(SEG_LAT + 1):
		for i in range(SEG_LON + 1):
			var u := float(i) / float(SEG_LON)
			var v := float(j) / float(SEG_LAT)
			var lon := u * TAU
			var lat := (v - 0.5) * PI
			var cl := cos(lat)
			var dir := Vector3(cl * cos(lon), sin(lat), cl * sin(lon))
			var hx := clampi(int(u * hw), 0, hw - 1)
			var hy := clampi(int(v * hh), 0, hh - 1)
			var h := himg.get_pixel(hx, hy).r
			st.set_uv(Vector2(u, v))
			st.add_vertex(dir * (1.0 + h * RELIEF))
	for j in range(SEG_LAT):
		for i in range(SEG_LON):
			var a := j * (SEG_LON + 1) + i
			var c := a + (SEG_LON + 1)
			st.add_index(a); st.add_index(c); st.add_index(a + 1)
			st.add_index(a + 1); st.add_index(c); st.add_index(c + 1)
	st.generate_normals()
	_globe.mesh = st.commit()
	var matm := StandardMaterial3D.new()
	matm.roughness = 0.92
	matm.metallic = 0.0
	matm.cull_mode = BaseMaterial3D.CULL_BACK
	_globe.material_override = matm

func _place_camera() -> void:
	if _globe != null:
		_globe.rotation = Vector3(0, _yaw, 0)
	_cam3d.position = Vector3(0, sin(_pitch), cos(_pitch)) * _cam_dist
	_cam3d.look_at(Vector3.ZERO, Vector3.UP)

func _world_to_globe(wx: float, wy: float, lift: float = 0.0) -> Vector3:
	var W := float(Sim.world.map_w())
	var H := float(Sim.world.map_h())
	var lon := (wx / W) * TAU
	var lat := ((wy / H) - 0.5) * PI
	var cl := cos(lat)
	return Vector3(cl * cos(lon), sin(lat), cl * sin(lon)) * (1.0 + RELIEF * 0.5 + lift)

## monde → ÉCRAN (globe + caméra). {pos, vis} ; court-circuite l'unproject pour la face cachée.
func globe_to_screen(wx: float, wy: float, lift: float = 0.0) -> Dictionary:
	var p3 := _world_to_globe(wx, wy, lift).rotated(Vector3.UP, _yaw)
	if p3.dot(_cam3d.position - p3) <= 0.0 or _cam3d.is_position_behind(p3):
		return {"pos": Vector2.ZERO, "vis": false}
	return {"pos": _cam3d.unproject_position(p3), "vis": true}

func _center_world() -> Vector2:
	if Sim.world == null:
		return Vector2.ZERO
	var d := _cam3d.position.normalized().rotated(Vector3.UP, -_yaw)
	var lat := asin(clampf(d.y, -1.0, 1.0))
	var lon := atan2(d.z, d.x)
	if lon < 0.0:
		lon += TAU
	return Vector2((lon / TAU) * Sim.world.map_w(), (lat / PI + 0.5) * Sim.world.map_h())

func globe_unit_px() -> float:
	var c := _center_world()
	var a := globe_to_screen(c.x, c.y)
	var b := globe_to_screen(c.x + 2.0, c.y)
	if not a["vis"] or not b["vis"]:
		return 1.0
	return (a["pos"] as Vector2).distance_to(b["pos"]) * 0.5

## oriente le globe pour amener le point MONDE au centre (pitch = lat · yaw = lon − π/2).
func center_on(wx: float, wy: float) -> void:
	if Sim.world == null:
		return
	var lon := (wx / float(Sim.world.map_w())) * TAU
	var lat := ((wy / float(Sim.world.map_h())) - 0.5) * PI
	_pitch = clampf(lat, -1.3, 1.3)
	_yaw = lon - PI * 0.5
	_place_camera()

# ════════════════════════ ISO ════════════════════════
## hauteur (0..1) à la cellule monde — volumétrie iso (× VOLUME × _relief) + gradient de hillshade.
func height_at(wx: float, wy: float) -> float:
	if _himg == null:
		return 0.0
	var hw := _himg.get_width()
	var hh := _himg.get_height()
	return _himg.get_pixel(clampi(int(wx), 0, hw - 1), clampi(int(wy), 0, hh - 1)).r

## monde → position ISO (sol PLAT : les côtes épousent la grille de tuiles, pas de relief lissé).
## Le maillage ET l'overlay y passent → assets et tuiles partagent EXACTEMENT la même projection.
func iso_pos(wx: float, wy: float) -> Vector2:
	return Iso.proj(wx, wy, 0.0)

## SOMMET BAS (sud) du losange contenant la cellule (wx,wy) — coin monde de (col+1,row+1)×TILE_K.
## L'ANCRE OFFICIELLE : pied des centres-villes/bâtiments ET point d'ENTRÉE des routes (un seul
## sommet, partagé) → les routes raccordent exactement là où les centres-villes branchent la leur.
func tile_anchor_world(wx: float, wy: float) -> Vector2:
	var col: int = int(wx) / TILE_K
	var row: int = int(wy) / TILE_K
	return Vector2(float((col + 1) * TILE_K), float((row + 1) * TILE_K))

## position ÉCRAN du sommet bas de la tuile (ancre des assets / entrée des routes).
func tile_anchor(wx: float, wy: float) -> Vector2:
	var a := tile_anchor_world(wx, wy)
	return iso_pos(a.x, a.y)

## bâtit la GRILLE de tuiles iso : un LOSANGE par bloc de TILE_K cellules. La tuile fixe la
## GÉOMÉTRIE iso (projection + ancres) ; TOUTE la couleur (terre lissée, côte, profondeur d'eau,
## ombrage) est calculée PAR PIXEL dans le shader depuis l'altitude → rendu doux, pas de damier.
func _build_tile_mesh() -> void:
	var w = Sim.world
	var W := float(w.map_w())
	var H := float(w.map_h())
	var nx := int(W) / TILE_K
	var ny := int(H) / TILE_K
	var nt := nx * ny
	var verts := PackedVector3Array(); verts.resize(nt * 4)
	var uvs := PackedVector2Array(); uvs.resize(nt * 4)
	var idx := PackedInt32Array(); idx.resize(nt * 6)
	var vi := 0
	var ii := 0
	for ty in range(ny):
		for tx in range(nx):
			var x0 := float(tx * TILE_K)
			var x1 := x0 + float(TILE_K)
			var y0 := float(ty * TILE_K)
			var y1 := y0 + float(TILE_K)
			var cxw := x0 + float(TILE_K) * 0.5
			var cyw := y0 + float(TILE_K) * 0.5
			var pT := iso_pos(x0, y0)   # NORD (sommet haut)
			var pR := iso_pos(x1, y0)   # EST
			var pB := iso_pos(x1, y1)   # SUD (sommet BAS = ancre)
			var pL := iso_pos(x0, y1)   # OUEST
			var uvc := Vector2(cxw / W, cyw / H)             # centre (réservé) ; tout est lissé au shader
			verts[vi] = Vector3(pT.x, pT.y, 0.0);     uvs[vi] = uvc
			verts[vi + 1] = Vector3(pR.x, pR.y, 0.0); uvs[vi + 1] = uvc
			verts[vi + 2] = Vector3(pB.x, pB.y, 0.0); uvs[vi + 2] = uvc
			verts[vi + 3] = Vector3(pL.x, pL.y, 0.0); uvs[vi + 3] = uvc
			idx[ii] = vi; idx[ii + 1] = vi + 1; idx[ii + 2] = vi + 2
			idx[ii + 3] = vi; idx[ii + 4] = vi + 2; idx[ii + 5] = vi + 3
			vi += 4
			ii += 6
	var arr := []
	arr.resize(Mesh.ARRAY_MAX)
	arr[Mesh.ARRAY_VERTEX] = verts
	arr[Mesh.ARRAY_TEX_UV] = uvs
	arr[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arr)
	_terrain.mesh = mesh

# ════════════════════════ commun ════════════════════════
func _on_generated() -> void:
	_himg = Sim.world.layer_image(LAYER_HEIGHT)
	_build_globe_mesh()
	_build_tile_mesh()
	_height_tex = ImageTexture.create_from_image(_himg)
	var matw := _terrain.material as ShaderMaterial
	if matw != null:
		var W := float(Sim.world.map_w())
		var H := float(Sim.world.map_h())
		matw.set_shader_parameter("height_tex", _height_tex)
		matw.set_shader_parameter("map_size", Vector2(W, H))
		matw.set_shader_parameter("tile_count", Vector2(W / float(TILE_K), H / float(TILE_K)))
	_refresh_terrain()
	_place_camera()

func _on_ticked(_year: int) -> void:
	_refresh_terrain()

func _refresh_terrain() -> void:
	if Sim.world == null:
		return
	_albedo = ImageTexture.create_from_image(Sim.world.map_image(mode, _selected_prov))
	if _globe != null:
		var m := _globe.material_override as StandardMaterial3D
		if m != null:
			m.albedo_texture = _albedo
	if _terrain != null:
		_terrain.texture = _albedo

func set_mode(m: int) -> void:
	mode = m
	_refresh_terrain()

# ── bascule GLOBE ↔ ISO en préservant le point regardé ──
func _enter_iso(at_world: Vector2) -> void:
	view_mode = VIEW_ISO
	_disp.visible = false
	# tuiles iso PAR-DESSUS le blend procédural (TOUJOURS visible = filet anti-trou : un coin de
	# losange non couvert montre le terrain, jamais du noir).
	_terrain.visible = true
	if _ground != null:
		_ground.visible = _ground.has_method("is_active") and bool(_ground.is_active())
	_camera.enabled = true
	_camera.make_current()
	_camera.zoom = Vector2(ISO_FAR, ISO_FAR)
	_camera.position = iso_pos(at_world.x, at_world.y)
	queue_redraw()

func _enter_globe(at_world: Vector2) -> void:
	view_mode = VIEW_GLOBE
	_camera.enabled = false
	_terrain.visible = false
	if _ground != null:
		_ground.visible = false
	_disp.visible = true
	_cam_dist = GLOBE_NEAR
	center_on(at_world.x, at_world.y)
	queue_redraw()

## centre monde actuellement regardé, quel que soit le mode.
func _looking_at() -> Vector2:
	if view_mode == VIEW_ISO:
		return Iso.unproj(_camera.position.x, _camera.position.y)
	return _center_world()

# ── navigation ───────────────────────────────────────────
func _input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and (event.button_mask & (MOUSE_BUTTON_MASK_RIGHT | MOUSE_BUTTON_MASK_LEFT)):
		if event.position.distance_to(_press_pos) > CLICK_SLOP:
			_dragged = true
		if view_mode == VIEW_GLOBE:
			_yaw -= event.relative.x * 0.006
			_pitch = clampf(_pitch + event.relative.y * 0.006, -1.3, 1.3)
			_place_camera()
		else:
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

## zoom CONTINU : facteur < 1 = on s'approche. Traverse la frontière GLOBE↔ISO.
func _zoom(factor: float) -> void:
	if view_mode == VIEW_GLOBE:
		var nd := _cam_dist * factor
		if nd < GLOBE_NEAR and factor < 1.0:
			_enter_iso(_center_world())          # zoom DEDANS sous le seuil → ISO
			return
		_cam_dist = clampf(nd, GLOBE_NEAR, GLOBE_FAR)
		_place_camera()
	else:
		var nz := _camera.zoom.x / factor        # zoom Camera2D : ÷facteur (approcher = ×)
		if nz < ISO_FAR and factor > 1.0:
			_enter_globe(_looking_at())           # zoom DEHORS sous le seuil → GLOBE
			return
		nz = clampf(nz, ISO_FAR, ISO_NEAR)
		_camera.zoom = Vector2(nz, nz)
	queue_redraw()

func _pick_at_mouse() -> void:
	if Sim.world == null or not Sim.world.has_method("province_at"):
		return
	var cell := Vector2i(-1, -1)
	if view_mode == VIEW_GLOBE:
		cell = _globe_pick()
	else:
		var wp := Iso.unproj(get_global_mouse_position().x, get_global_mouse_position().y)
		var cx := int(floor(wp.x))
		var cy := int(floor(wp.y))
		if cx >= 0 and cy >= 0 and cx < Sim.world.map_w() and cy < Sim.world.map_h():
			cell = Vector2i(cx, cy)
	if cell.x < 0:
		return
	var prov: int = Sim.world.province_at(cell.x, cell.y)
	_selected_prov = prov
	_refresh_terrain()
	var region := -1
	var owner := -1
	if prov >= 0:
		region = Sim.world.province_region(prov)
		if region >= 0:
			owner = Sim.world.region_owner(region)
	province_picked.emit(prov, region, owner)

## clic globe → rayon caméra → intersection sphère unité → lon/lat → cellule monde.
func _globe_pick() -> Vector2i:
	var mp := get_viewport().get_mouse_position()
	var o := _cam3d.project_ray_origin(mp)
	var dir := _cam3d.project_ray_normal(mp)
	var b := o.dot(dir)
	var c := o.dot(o) - 1.0
	var disc := b * b - c
	if disc < 0.0:
		return Vector2i(-1, -1)
	var t := -b - sqrt(disc)
	if t < 0.0:
		return Vector2i(-1, -1)
	var hit := (o + dir * t).rotated(Vector3.UP, -_yaw)
	var lat := asin(clampf(hit.y, -1.0, 1.0))
	var lon := atan2(hit.z, hit.x)
	if lon < 0.0:
		lon += TAU
	var cx := clampi(int((lon / TAU) * Sim.world.map_w()), 0, Sim.world.map_w() - 1)
	var cy := clampi(int((lat / PI + 0.5) * Sim.world.map_h()), 0, Sim.world.map_h() - 1)
	return Vector2i(cx, cy)

# ── verbes publics (boutons de carte) ──
func zoom_in() -> void:  _zoom(0.8)
func zoom_out() -> void: _zoom(1.0 / 0.8)
func fit() -> void:
	# « fit » = la vue d'ensemble = le GLOBE au plus loin
	if view_mode == VIEW_ISO:
		_enter_globe(_looking_at())
	_cam_dist = GLOBE_FAR
	_yaw = 0.0
	_pitch = 0.35
	_place_camera()
	queue_redraw()
