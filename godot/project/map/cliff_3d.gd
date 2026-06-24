extends Node2D
## Cliff3D — FALAISES en MICRO-MESH 3D. Le relief des highlands (la FACE de roche sous le plateau) est
## rendu par de petits polyèdres 3D EMPILÉS dans un SubViewport ORTHOGRAPHIQUE calé EXACTEMENT à la
## projection iso 2:1 du jeu, composité par-dessus le sol 2D. Display-only (lit Sim.world, n'écrit rien).
##
## CALAGE EXACT (vérifié au pixel) : la projection iso est screen=(wx-wy, (wx+wy)/2 - h·90) — la hauteur
## est EXAGÉRÉE ×90 vs le sol, donc une caméra 3D tournée UNIFORME ne peut pas la reproduire telle quelle.
## On PRÉ-ÉTIRE la hauteur des mesh par KH = 90/√1.5 ≈ 73.48 : avec ce facteur, la base (right=(1,0,-1),
## up=(-½,90/KH,-½)) devient ORTHONORMÉE (|right|=|up|=√2) → une seule taille ortho suffit. Un sommet
## (wx,wy,h) se place en 3D (wx, h·KH, wy) ; la caméra projette alors au pixel près comme Iso.proj.
##
## La caméra SUIT la Camera2D (pan/zoom) → on ne rend que la région visible à pleine résolution (net au
## zoom). Le composite est tracé dans l'ESPACE MONDE iso (rect couvrant l'écran) → la Camera2D le replace
## 1:1 ; il s'insère ainsi DANS l'ordre de dessin (sous le dressing de l'overlay, sur le sol).

const Iso = preload("res://map/iso.gd")
const UIKit = preload("res://ui/uikit.gd")
const TILE_K := 8               ## DOIT matcher iso_ground.TILE_K (la grille du masque highland)
const KH := 73.484692           ## pré-étirement de hauteur = 90/sqrt(1.5) → base ortho-normée
const TOP_H := 0.11111          ## hauteur MONDE du plateau = highland_lift(10) / 90 (miroir du shader)
const CAM_BACK := 1600.0        ## recul caméra le long de l'axe de vue (ortho : n'affecte que near/far)
# base ORTHONORMÉE de la caméra iso (colonnes X=right, Y=up, Z=back) — cf. en-tête.
const BX := Vector3(0.70710678, 0.0, -0.70710678)
const BY := Vector3(-0.35355339, 0.86602540, -0.35355339)
const BZ := Vector3(0.61237244, 0.5, 0.61237244)
const SY := TOP_H * KH          ## hauteur 3D du mur (du sol au plateau)
const MAX_SEG := 12000          ## garde-fou : nb max de segments de mur traités

# biomes highland (face de roche) ; topbio « ouvert » → couleur de la coiffe (herbe/neige/sable)
const HL_GLACIER := 19
const HL_VOLCANO := 23

var _cam2d: Camera2D = null     ## la Camera2D du jeu (injectée par map_view) — on la MIROITE
var _csv: SubViewport = null
var _ccam: Camera3D = null
var _mm_slab: MultiMeshInstance3D = null    ## strates de roche (le mur)
var _mm_cap: MultiMeshInstance3D = null     ## coiffe herbeuse/neigeuse du rebord
var _mm_bould: MultiMeshInstance3D = null   ## gros blocs (éboulis de pied)
var _mm_rock: MultiMeshInstance3D = null    ## petits cailloux (scree)
var _mesh_slab: ArrayMesh = null
var _mesh_cap: ArrayMesh = null
var _mesh_bould: ArrayMesh = null
var _mesh_rock: ArrayMesh = null
var _rock_mat: ShaderMaterial = null    ## roche PEINTE (strates + mottle, par-dessus le toon)
var _cap_mat: StandardMaterial3D = null  ## coiffe herbe/neige (toon simple)
var _built := false

func setup(cam2d: Camera2D) -> void:
	_cam2d = cam2d

func _ready() -> void:
	_csv = SubViewport.new()
	_csv.transparent_bg = true
	_csv.size = Vector2i(get_viewport_rect().size)
	_csv.own_world_3d = true
	_csv.world_3d = World3D.new()
	_csv.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	_csv.msaa_3d = Viewport.MSAA_4X
	add_child(_csv)

	var we := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0, 0, 0, 0)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.70, 0.71, 0.74)
	env.ambient_light_energy = 1.05            # ombres CLAIRES (peinte, pas charbon) → moins CG
	we.environment = env
	_csv.add_child(we)
	# soleil cohérent avec le terrain (haut-gauche) → faces sud/est et sommets ÉCLAIRÉS, recoins à l'ombre.
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(deg_to_rad(-48.0), deg_to_rad(38.0), 0.0)
	sun.light_color = Color(1.0, 0.97, 0.90)   # lumière chaude
	sun.light_energy = 1.05
	_csv.add_child(sun)
	# FILL froid de l'autre côté → décolle les faces d'ombre du noir (rendu plus PEINT, moins facetté CG).
	var fill := DirectionalLight3D.new()
	fill.rotation = Vector3(deg_to_rad(-26.0), deg_to_rad(-140.0), 0.0)
	fill.light_color = Color(0.80, 0.85, 0.95)
	fill.light_energy = 0.45
	_csv.add_child(fill)

	_ccam = Camera3D.new()
	_ccam.projection = Camera3D.PROJECTION_ORTHOGONAL
	_ccam.near = 1.0
	_ccam.far = 3400.0
	_csv.add_child(_ccam)

	_mesh_slab = _make_slab()
	_mesh_cap = _make_cap()
	_mesh_bould = _make_boulder(1.0)
	_mesh_rock = _make_boulder(0.42)
	# roche PEINTE (strates + mottle de bruit) → sort du « CG » low-poly, raccorde au terrain peint
	_rock_mat = ShaderMaterial.new()
	_rock_mat.shader = load("res://map/cliff_rock.gdshader")
	var noise := UIKit.blend_noise()
	if noise != null:
		_rock_mat.set_shader_parameter("noise_tex", noise)
	_cap_mat = _toon_mat()

	get_viewport().size_changed.connect(_on_resize)
	Sim.generated.connect(_on_generated)

func _on_resize() -> void:
	if _csv != null:
		_csv.size = Vector2i(get_viewport_rect().size)

func _on_generated() -> void:
	_built = false
	queue_redraw()

# ─────────────────────────── MESHS (SurfaceTool, low-poly FACETTÉ pour le toon) ───────────────────────────

## matériau TOON partagé : la lumière + l'ombrage cel donnent les facettes ; la couleur d'INSTANCE
## (MultiMesh) teinte par biome. Unshaded NON (on veut l'ombrage) → DIFFUSE_TOON.
func _toon_mat() -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.vertex_color_use_as_albedo = true
	m.diffuse_mode = BaseMaterial3D.DIFFUSE_LAMBERT_WRAP   # doux (pas toon) → coiffe peinte, moins CG
	m.specular_mode = BaseMaterial3D.SPECULAR_DISABLED
	m.roughness = 1.0
	m.metallic = 0.0
	return m

## ajoute une face PLATE (triangle-fan sur les sommets donnés, ≥3) avec une NORMALE unique → facette nette.
func _face(st: SurfaceTool, vs: Array) -> void:
	var n := (Vector3(vs[1]) - Vector3(vs[0])).cross(Vector3(vs[2]) - Vector3(vs[0])).normalized()
	for i in range(1, vs.size() - 1):
		st.set_normal(n); st.add_vertex(vs[0])
		st.set_normal(n); st.add_vertex(vs[i])
		st.set_normal(n); st.add_vertex(vs[i + 1])

## SLAB — strate de roche TAPERÉE (base large, sommet plus étroit & décalé) ; faces planes → facettes
## toon. Unité y∈[0,1], x,z∈[-0.5,0.5] (mise à l'échelle par instance). Pas de face du dessous (cachée).
func _make_slab() -> ArrayMesh:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	# base (y=0) un peu plus large que le sommet (y=1), sommet incliné (front plus bas = surplomb léger)
	var b := 0.5
	var t := 0.38
	var ox := 0.05    # décalage du sommet vers l'arrière (-z) → la face avant penche
	var b00 := Vector3(-b, 0, -b); var b10 := Vector3(b, 0, -b)
	var b11 := Vector3(b, 0, b);   var b01 := Vector3(-b, 0, b)
	var t00 := Vector3(-t, 1, -t - ox); var t10 := Vector3(t, 1, -t - ox)
	var t11 := Vector3(t, 1, b - 0.06 - ox); var t01 := Vector3(-t, 1, b - 0.06 - ox)
	_face(st, [b01, b11, t11, t01])   # AVANT (+z) — la face de falaise vue
	_face(st, [b10, b00, t00, t10])   # ARRIÈRE (-z)
	_face(st, [b00, b01, t01, t00])   # GAUCHE (-x)
	_face(st, [b11, b10, t10, t11])   # DROITE (+x)
	_face(st, [t00, t01, t11, t10])   # DESSUS
	return st.commit()

## GRASS_CAP — coiffe basse du rebord (monticule de terre/herbe). Frustum très plat, légèrement bombé,
## débordant vers l'avant. Teinté herbe/neige par instance. Unité y∈[0,1] très écrasé à l'usage.
func _make_cap() -> ArrayMesh:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var b := 0.5
	var t := 0.47    # quasi-vertical → PLAQUE plate (pas un nugget facetté qui fait des « X » au soleil)
	var b00 := Vector3(-b, 0, -b); var b10 := Vector3(b, 0, -b)
	var b11 := Vector3(b, 0, b);   var b01 := Vector3(-b, 0, b)
	var t00 := Vector3(-t, 1, -t); var t10 := Vector3(t, 1, -t)
	var t11 := Vector3(t, 1, t);   var t01 := Vector3(-t, 1, t)
	_face(st, [b01, b11, t11, t01])
	_face(st, [b10, b00, t00, t10])
	_face(st, [b00, b01, t01, t00])
	_face(st, [b11, b10, t10, t11])
	_face(st, [t00, t01, t11, t10])
	return st.commit()

## BOULDER/ROCK — bloc anguleux (octaèdre irrégulier), faces planes facettées. `s` met à l'échelle
## (1 = gros bloc, 0.42 = caillou). 6 sommets → 8 faces triangulaires.
func _make_boulder(s: float) -> ArrayMesh:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var px := Vector3(0.55, 0.12, 0.05) * s; var nxv := Vector3(-0.5, 0.05, -0.08) * s
	var py := Vector3(0.06, 0.62, 0.0) * s;  var nyv := Vector3(-0.04, -0.4, 0.05) * s
	var pz := Vector3(0.0, 0.1, 0.56) * s;   var nzv := Vector3(0.08, 0.08, -0.5) * s
	_face(st, [py, px, pz]); _face(st, [py, pz, nxv]); _face(st, [py, nxv, nzv]); _face(st, [py, nzv, px])
	_face(st, [nyv, pz, px]); _face(st, [nyv, nxv, pz]); _face(st, [nyv, nzv, nxv]); _face(st, [nyv, px, nzv])
	return st.commit()

# ─────────────────────────── BUILD : détection de bord + empilement déterministe ───────────────────────────

func _hash(cx: int, cy: int, k: int) -> int:
	return absi((cx * 73856093) ^ (cy * 19349663) ^ (k * 83492791))

func _rock_tint(b: int, jit: float, lay: float) -> Color:
	var base := Color(0.56, 0.50, 0.45)            # roche brun-gris CLAIRE (peinte, pas charbon)
	var warm := Color(0.62, 0.52, 0.40)            # veine chaude (ocre)
	if b == HL_GLACIER:
		base = Color(0.68, 0.71, 0.75)             # roche glaciaire (gris froid clair)
		warm = Color(0.72, 0.70, 0.68)
	elif b == HL_VOLCANO:
		base = Color(0.34, 0.30, 0.30)             # basalte sombre
		warm = Color(0.40, 0.31, 0.28)
	base = base.lerp(warm, 0.45 * jit)             # variation chaude/froide par strate → peinte, pas uni
	var f := (0.86 + 0.12 * jit) * lay             # `lay` ÉCLAIRCIT les strates hautes (ensoleillées)
	return Color(base.r * f, base.g * f, base.b * f)

func _cap_tint(topb: int, jit: float) -> Color:
	var base := Color(0.40, 0.52, 0.26)            # herbe
	if topb == 20 or topb == 19:
		base = Color(0.70, 0.73, 0.77)             # neige/glacier (cassée, pas blanc pur flottant)
	elif topb >= 9 and topb <= 11:
		base = Color(0.74, 0.66, 0.44)             # steppe/désert
	elif topb == 5 or topb == 3:
		base = Color(0.80, 0.74, 0.52)             # sable/plage
	var f := 0.90 + 0.20 * jit
	return Color(base.r * f, base.g * f, base.b * f)

## bcell = biome PAR CELLULE (face de roche) ; topb = biome du sommet ouvert (coiffe). hl = masque FINAL.
func build(hl: PackedByteArray, nx: int, ny: int, bcell: PackedByteArray, topb: PackedByteArray) -> void:
	_built = true
	for mm in [_mm_slab, _mm_cap, _mm_bould, _mm_rock]:
		if mm != null:
			mm.queue_free()
	_mm_slab = null; _mm_cap = null; _mm_bould = null; _mm_rock = null

	var slab_x: Array[Transform3D] = []; var slab_c: Array[Color] = []
	var cap_x: Array[Transform3D] = [];  var cap_c: Array[Color] = []
	var bld_x: Array[Transform3D] = [];  var bld_c: Array[Color] = []
	var rk_x: Array[Transform3D] = [];   var rk_c: Array[Color] = []
	var seg := 0
	for cy in range(ny):
		for cx in range(nx):
			if hl[cy * nx + cx] == 0:
				continue
			var south: bool = (cy + 1 >= ny) or hl[(cy + 1) * nx + cx] == 0
			var east: bool = (cx + 1 >= nx) or hl[cy * nx + cx + 1] == 0
			if not south and not east:
				continue
			var bi := int(bcell[cy * nx + cx])
			var ti := int(topb[cy * nx + cx])
			if south:
				_emit_wall(cx, cy, false, bi, ti, slab_x, slab_c, cap_x, cap_c, bld_x, bld_c, rk_x, rk_c)
				seg += 1
			if east:
				_emit_wall(cx, cy, true, bi, ti, slab_x, slab_c, cap_x, cap_c, bld_x, bld_c, rk_x, rk_c)
				seg += 1
			if seg >= MAX_SEG:
				break
		if seg >= MAX_SEG:
			break
	_mm_slab = _make_mmi(_mesh_slab, slab_x, slab_c, _rock_mat)
	_mm_cap = _make_mmi(_mesh_cap, cap_x, cap_c, _cap_mat)
	_mm_bould = _make_mmi(_mesh_bould, bld_x, bld_c, _rock_mat)
	_mm_rock = _make_mmi(_mesh_rock, rk_x, rk_c, _rock_mat)

## empile un mur sur un bord highland. `is_east` : bord EST (cx+1, face +x, mur le long de Z) sinon SUD
## (cy+1, face +z, mur le long de X). Strates SLAB empilées (jitter déterministe) + coiffe + éboulis.
func _emit_wall(cx: int, cy: int, is_east: bool, bi: int, ti: int,
		slab_x: Array, slab_c: Array, cap_x: Array, cap_c: Array,
		bld_x: Array, bld_c: Array, rk_x: Array, rk_c: Array) -> void:
	# centre MONDE du bord + axes (le long du bord = `along`, vers le vide = `out`)
	var ex: float; var ez: float; var along: Vector3; var outv: Vector3
	if is_east:
		ex = float(cx + 1) * TILE_K
		ez = (float(cy) + 0.5) * TILE_K
		along = Vector3(0, 0, 1)
		outv = Vector3(1, 0, 0)
	else:
		ex = (float(cx) + 0.5) * TILE_K
		ez = float(cy + 1) * TILE_K
		along = Vector3(1, 0, 0)
		outv = Vector3(0, 0, 1)
	var width := float(TILE_K) * 1.14        # un peu > la cellule → murs voisins se CHEVAUCHENT (continu)
	var depth := 2.4
	var h0 := _hash(cx, cy, is_east as int)
	var nslab := 3 + (h0 % 3)                 # 3..5 strates (varié par mur) → bancs irréguliers
	for i in range(nslab):
		var hh := _hash(cx, cy, i + (10 if is_east else 0))
		var j := float(hh % 1000) / 1000.0          # jitter principal [0,1)
		var j2 := float((hh / 1000) % 1000) / 1000.0  # 2e jitter décorrélé
		var sh := SY / float(nslab)
		var yc := (float(i) + 0.5) * sh + (j2 - 0.5) * sh * 0.35   # strates pas parfaitement alignées
		# RECUL IRRÉGULIER : chaque banc avance/recule un peu différemment (paroi rugueuse, pas un plan),
		# décalage le long du bord + rotation → silhouette cassée. Spread avant net SUPPRIMÉ (anti-ballon).
		var recede := (float(i) / float(nslab)) * 0.7 + (j - 0.5) * 1.0
		var off := along * ((j - 0.5) * 2.1) - outv * recede
		var pos := Vector3(ex, yc, ez) + off
		var b := Basis().rotated(Vector3.UP, deg_to_rad((j - 0.5) * 22.0))
		var sx := width * (0.80 + 0.40 * j)         # largeur très variée (0.80..1.20) → bancs inégaux
		var sd := depth * (0.85 + 0.6 * j2)         # profondeur variée
		b = b.scaled((Vector3(sd, sh * 1.08, sx) if is_east else Vector3(sx, sh * 1.08, sd)))
		slab_x.append(Transform3D(b, pos))
		var lay := 0.78 + 0.18 * float(i) / maxf(1.0, float(nslab - 1))   # bas sombre → haut ensoleillé
		slab_c.append(_rock_tint(bi, j, lay))
	# COIFFE : un LISERÉ herbeux/neigeux PLAT sur le rebord (pas un monticule) — débordant un peu vers le vide.
	var hc := _hash(cx, cy, 7 + (is_east as int))
	var jc := float(hc % 1000) / 1000.0
	var capw := width * 0.96
	var cb := Basis().rotated(Vector3.UP, deg_to_rad((jc - 0.5) * 10.0))
	cb = cb.scaled((Vector3(depth * 1.5, SY * 0.10, capw) if is_east else Vector3(capw, SY * 0.10, depth * 1.5)))
	cap_x.append(Transform3D(cb, Vector3(ex, SY * 0.97, ez) + outv * (depth * 0.12)))
	cap_c.append(_cap_tint(ti, jc))
	# ÉBOULIS de pied : 1 à 3 blocs/cailloux ÉPARS le long du pied → cachent l'arête basse et RACCORDENT
	# la paroi au sol (le « raccord de pied »). Le 1er est un gros bloc, les suivants des cailloux. Déterministe.
	var nsc := 1 + (h0 % 3)
	for s in range(nsc):
		var hb := _hash(cx, cy, 21 + s * 13 + (is_east as int))
		var jb := float(hb % 1000) / 1000.0
		var jb2 := float((hb / 1000) % 1000) / 1000.0
		var aoff := along * ((jb2 - 0.5) * width * 1.0)
		if s == 0:
			var bs := 2.2 + 1.6 * jb
			var bb := Basis().rotated(Vector3.UP, deg_to_rad(jb * 360.0)).scaled(Vector3(bs, bs * 0.78, bs))
			bld_x.append(Transform3D(bb, Vector3(ex, bs * 0.28, ez) + outv * (1.2 + 1.0 * jb) + aoff))
			bld_c.append(_rock_tint(bi, jb * 0.7, 0.84))
		else:
			var rs := 1.1 + 1.0 * jb
			var rb := Basis().rotated(Vector3.UP, deg_to_rad(jb * 360.0)).scaled(Vector3(rs, rs, rs))
			rk_x.append(Transform3D(rb, Vector3(ex, rs * 0.24, ez) + outv * (1.8 + 1.6 * jb) + aoff))
			rk_c.append(_rock_tint(bi, 0.3 + 0.5 * jb, 0.9))

func _make_mmi(mesh: ArrayMesh, xforms: Array, colors: Array, mat: Material) -> MultiMeshInstance3D:
	if xforms.is_empty():
		return null
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = mesh
	mm.instance_count = xforms.size()
	for i in range(xforms.size()):
		mm.set_instance_transform(i, xforms[i])
		mm.set_instance_color(i, colors[i])
	var mmi := MultiMeshInstance3D.new()
	mmi.multimesh = mm
	mmi.material_override = mat
	_csv.add_child(mmi)
	return mmi

# ─────────────────────────── CAMÉRA (miroir de la Camera2D) + composite ───────────────────────────

func _sync_camera() -> void:
	if _cam2d == null or _ccam == null:
		return
	var z: float = _cam2d.zoom.y
	if z <= 0.0:
		return
	var vp := Vector2i(get_viewport_rect().size)
	if _csv.size != vp:
		_csv.size = vp
	var vh := float(_csv.size.y)
	_ccam.size = 0.70710678 * vh / z
	var fw := Iso.unproj(_cam2d.position.x, _cam2d.position.y)   # iso → monde (centre regardé)
	var pc := Vector3(fw.x, 0.0, fw.y)
	_ccam.transform = Transform3D(Basis(BX, BY, BZ), pc + BZ * CAM_BACK)

func _process(_dt: float) -> void:
	if _cam2d == null:
		return
	var par := get_parent()
	if _cam2d.enabled and par != null and par.view_mode == 1:
		visible = true
		_sync_camera()
		queue_redraw()
	else:
		visible = false

func _draw() -> void:
	if _csv == null or _cam2d == null or not _cam2d.enabled:
		return
	# composite l'image du SubViewport (déjà en pixels écran) au rect MONDE qui couvre l'écran → la
	# Camera2D la replace 1:1 (et l'insère dans l'ordre de dessin iso, sous le dressing).
	var inv := get_viewport_transform().affine_inverse()
	var vp := get_viewport_rect().size
	var tl := inv * Vector2.ZERO
	var br := inv * vp
	draw_texture_rect(_csv.get_texture(), Rect2(tl, br - tl), false)
