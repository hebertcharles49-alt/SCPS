extends Node2D
## IsoGround — peint le SOL en carte PARCHEMIN : UN quad couvre le monde, et le shader iso_antique
## (100 % PROCÉDURAL) lit le BIOME par pixel + le DÉBIT des rivières (couches moteur) + un bruit
## GÉNÉRÉ → lavis sépia, côtes à l'encre, marais, rivières à la plume, relief en lavis, rose des
## vents, bords brûlés. Aucun atlas de sprites. Le champ de débit des rivières est carvé CPU
## (méandre Chaikin + sinusoïde jittée) et passé au shader (le moteur trace déjà le réseau).
##
## Display-only. La Camera2D fait pan/zoom ; rendu UNE FOIS au générate (biomes ~statiques
## post-worldgen ; re-dessin sur cataclysme §27 quand les biomes mutent).

const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const ANTIQUE_MARGIN := 48.0           ## marge de PAPIER (unités monde) autour de la carte

var _active := false
var _bmap: ImageTexture = null         ## couche biome → texture (R = biome/255) pour le shader
var _river_map: ImageTexture = null    ## couche DÉBIT (rivières carvées) → texture pour le shader
var _river_field: Image = null         ## la MÊME couche débit en Image L8 (lue aussi par l'overlay)

func _ready() -> void:
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_tick)
	_active = true                  # le parchemin est PROCÉDURAL (couches moteur) → pas besoin d'iso_tiles
	_setup_blend()
	queue_redraw()

## monte le ShaderMaterial PARCHEMIN (iso_antique) — rendu cartographique 100 % procédural depuis les
## couches moteur (biome + rivières) + un bruit. Plus de splat iso 3D, plus d'atlas de sprites.
func _setup_blend() -> void:
	var sh := load("res://map/iso_antique.gdshader")
	if sh == null:
		return
	var mat := ShaderMaterial.new()
	mat.shader = sh
	mat.set_shader_parameter("noise_tex", _make_noise())   # bruit PROCÉDURAL seamless (plus d'asset PNG)
	material = mat
	texture_repeat = CanvasItem.TEXTURE_REPEAT_ENABLED     # le bruit COULE (UV monde > 1 → wrap)

## bruit fbm SEAMLESS généré (plus de PNG) — grain de papier, warp de frontières, écume/encre du shader.
func _make_noise() -> NoiseTexture2D:
	var fnl := FastNoiseLite.new()
	fnl.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	fnl.frequency = 0.012
	fnl.fractal_type = FastNoiseLite.FRACTAL_FBM
	fnl.fractal_octaves = 4
	fnl.seed = 1337
	var nt := NoiseTexture2D.new()
	nt.width = 512
	nt.height = 512
	nt.seamless = true
	nt.noise = fnl
	return nt

func is_active() -> bool:
	return _active

func _on_generated() -> void:
	_active = true
	_bmap = null            # nouveau monde → recharge biome + débit
	_river_map = null
	_river_field = null
	queue_redraw()

func _on_tick(_y: int) -> void:
	var w = Sim.world
	if w == null:
		return
	# les biomes ne bougent qu'en FIN §27 (cataclysme) → recharge + re-dessin alors
	if int((w.endgame_info() as Dictionary).get("fin", 0)) > 0:
		_bmap = null
		queue_redraw()

func _draw() -> void:
	if not _active:
		return
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(LAYER_BIOME)
	if bio == null:
		return
	var W := int(w.map_w())
	var H := int(w.map_h())
	# PARCHEMIN : le shader peint tout depuis les SEULES couches moteur — carte des biomes (R = biome/255)
	# + champ de débit des rivières (carvé CPU) + bruit (grain/warp). Aucun atlas de sprites.
	var mat := material as ShaderMaterial
	if mat != null:
		if _bmap == null:
			_bmap = ImageTexture.create_from_image(bio)
		if _river_map == null:
			if _river_field == null:
				_river_field = _build_river_field(w, W, H)
			_river_map = ImageTexture.create_from_image(_river_field)
		mat.set_shader_parameter("biome_map", _bmap)
		if _river_map != null:
			mat.set_shader_parameter("river_map", _river_map)
		mat.set_shader_parameter("map_size", Vector2(W, H))
		# flat_map = 1.0 : mapping cellule TOP-DOWN (biome lu en monde direct). L'INCLINAISON visuelle
		# est portée par l'échelle Y du nœud IsoGround (map_view.TILT_Y) → sol & overlay restent alignés.
		mat.set_shader_parameter("flat_map", 1.0)
	# SOL PARCHEMIN : un seul QUAD TOP-DOWN couvrant le monde (+ marge de PAPIER), peint par le shader.
	draw_rect(Rect2(-ANTIQUE_MARGIN, -ANTIQUE_MARGIN, float(W) + 2.0 * ANTIQUE_MARGIN,
		float(H) + 2.0 * ANTIQUE_MARGIN), Color(0.0, 0.0, 0.0, 1.0))

## tuile pavé seamless (1×) — centre de la chaussée, échantillonnée tuilée sur le plan du sol.
func river_field(w) -> Image:
	if _river_field == null and w != null and w.has_method("map_w"):
		_river_field = _build_river_field(w, int(w.map_w()), int(w.map_h()))
	return _river_field

## bâtit le CHAMP DÉBIT à graver : la worldgen sème des fleuves partout → on ne garde QUE les majeurs
## (cluster par embouchure, tronc le plus long par système), gravés en CONTINU (pinceau ∝ débit) dans
## un L8. Le shader y lit l'eau : cœur propre au fort, berges fondues (champ lissé par filtre linéaire).
func _build_river_field(w, W: int, H: int) -> Image:
	var img := Image.create(W, H, false, Image.FORMAT_L8)
	if w == null or not w.has_method("river_paths"):
		return img
	var raw: Array = w.river_paths()
	if raw.is_empty():
		return img
	var hgt: Image = w.layer_image(LAYER_HEIGHT)   # pente locale → module l'amplitude du méandre
	var sea: Image = w.layer_image(4)              # mer/lac → on n'envoie pas le méandre dans l'eau
	var bio: Image = w.layer_image(LAYER_BIOME)    # biome → forêt ≈ droit, plaine ouverte = serpente
	# flow = NIVEAU : fleuve 1.0 · rivière 0.62 · affluent 0.34. Le MOTEUR donne une LIGNE MÉDIANE propre ;
	# ICI on l'ARRONDIT (Chaikin → fin de l'escalier D8) puis on FORCE un MÉANDRE SINUSOÏDAL JITTÉ — amplitude
	# ∝ platitude × BIOME (serpente en terre plate ouverte, quasi droit en forêt, nul en montagne), bruit
	# cohérent pour casser l'axe NSEO (zéro angle droit). Largeur qui CROÎT vers l'aval (affluents collectés).
	for rv in raw:
		var pts: PackedVector2Array = rv["points"]
		if pts.size() < 6:
			continue
		var fl := float(rv["flow"])
		var v := clampf(0.58 + 0.42 * fl, 0.0, 1.0)
		var base_w := 1 if fl > 0.8 else 0                       # fleuve = trait fin · rivière/affluent = FIL
		var grow := 1 if fl > 0.8 else (1 if fl > 0.5 else 0)     # enfle un PEU vers l'aval (fleuve 1→2 · rivière 0→1)
		var mp := _meander(pts, hgt, sea, bio, W, H)
		var n := mp.size()
		for k in range(n - 1):
			var frac := float(k) / float(maxi(1, n - 1))         # 0 = source → 1 = embouchure
			var wd := base_w + int(round(frac * float(grow)))    # PLUS LARGE en aval (affluents accumulés)
			_carve_seg(img, mp[k], mp[k + 1], v, wd, W, H)
	return img

## amplitude de MÉANDRE par BIOME (index = enum Biome) : terre plate OUVERTE serpente (≈1), FORÊT quasi
## droite (les arbres = obstacles, ≈0.25), relief droit (≈0), MARAIS serpente fort. Module l'amplitude.
const BIOME_MEANDER := [
	0.0, 0.0, 0.0, 0.7,        # 0 deep_ocean · 1 ocean · 2 shallow · 3 coast
	1.0, 1.0, 1.0, 1.0, 1.0,   # 4 plains · 5 farmland · 6 grassland · 7 steppe · 8 savanna → SERPENTE
	0.85, 0.85, 0.8,           # 9 drylands · 10 desert · 11 coastal_desert → ouvert
	0.25, 0.25, 0.30,          # 12 forest · 13 woods · 14 jungle → QUASI DROIT
	1.1,                       # 15 marsh → serpente FORT
	0.12, 0.15, 0.05, 0.05,    # 16 highlands · 17 hills · 18 mountains · 19 peak → droit
	0.18,                      # 20 glacier
	0.8, 1.0, 0.05, 0.2,       # 21 mangrove · 22 bog · 23 volcano · 24 thorns
]
var _riv_noise: FastNoiseLite = null

## FORCE un MÉANDRE SINUSOÏDAL JITTÉ sur la ligne médiane. (1) Chaikin ARRONDIT d'abord l'escalier D8 (plus
## d'angle droit de base). (2) Déplacement PERPENDICULAIRE d'amplitude ∝ PLATITUDE × BIOME (plaine ouverte
## serpente, forêt quasi droite, montagne nulle). (3) Sinusoïde JITTÉE : deux sinus incommensurables + bruit
## COHÉRENT (FastNoiseLite) → ni régulière ni verrouillée sur l'axe NSEO (zéro angle droit). Bouts ancrés ;
## jamais poussé dans la mer. Longueur d'onde ~ `wave` cellules ; abscisse curviligne `s` = phase constante.
func _meander(pts: PackedVector2Array, hgt: Image, sea: Image, bio: Image, W: int, H: int) -> PackedVector2Array:
	var base := _chaikin(pts, 2)                          # arrondit l'escalier D8 AVANT de méandrer
	var n := base.size()
	if n < 4:
		return base
	var out := PackedVector2Array(); out.resize(n)
	out[0] = base[0]; out[n - 1] = base[n - 1]
	var nz := _meander_noise()
	var wave := 26.0
	var amp := 7.0
	var phase := float((int(base[0].x) * 131 + int(base[0].y) * 57) % 628) / 100.0   # déphasage par rivière
	var s := 0.0
	for i in range(1, n - 1):
		s += base[i].distance_to(base[i - 1])             # abscisse curviligne (longueur d'onde en CELLULES)
		var p: Vector2 = base[i]
		var tang: Vector2 = (base[i + 1] - base[i - 1]).normalized()
		var perp := Vector2(-tang.y, tang.x)
		var fac := _flatness(hgt, p, W, H) * _biome_meander(bio, p, W, H)   # pente × biome
		# sinusoïde JITTÉE : 2 sinus incommensurables (normalisés) + bruit cohérent → cassée, hors-axe
		var wig := (sin(s / wave + phase) + 0.4 * sin(s / wave * 2.37 + phase * 1.7)) / 1.4
		wig += 0.4 * nz.get_noise_2d(p.x * 1.4, p.y * 1.4)
		var disp := perp * (amp * fac * wig)
		# micro-jitter COHÉRENT, décorrélé x/y → casse tout verrou cardinal résiduel (déplacement OFF-AXIS)
		disp += Vector2(nz.get_noise_2d(p.y * 2.3 + 40.0, p.x * 2.3),
				nz.get_noise_2d(p.x * 2.3, p.y * 2.3 + 70.0)) * (amp * 0.30 * fac)
		var q := p + disp
		if sea != null:                                   # ne pas méandrer DANS la mer (réduit jusqu'à rester sur terre)
			var tries := 0
			while tries < 5 and _is_sea(sea, q, W, H):
				disp *= 0.5; q = p + disp; tries += 1
			if _is_sea(sea, q, W, H):
				q = p
		out[i] = q
	return out

## Chaikin (corner-cutting) : remplace chaque segment par ses points ¼ et ¾ → coins ARRONDIS (l'escalier
## D8 devient courbe). Bouts conservés (source/embouchure ancrés). `iters` passes (≈ ×2 points/passe).
func _chaikin(pts: PackedVector2Array, iters: int) -> PackedVector2Array:
	var cur := pts
	for _it in range(iters):
		if cur.size() < 3:
			break
		var nxt := PackedVector2Array()
		nxt.append(cur[0])
		for i in range(cur.size() - 1):
			var a: Vector2 = cur[i]
			var b: Vector2 = cur[i + 1]
			nxt.append(a.lerp(b, 0.25))
			nxt.append(a.lerp(b, 0.75))
		nxt.append(cur[cur.size() - 1])
		cur = nxt
	return cur

## amplitude de méandre du BIOME sous (p) — 1 = serpente (plaine ouverte), 0.25 = forêt, ~0 = relief.
func _biome_meander(bio: Image, p: Vector2, W: int, H: int) -> float:
	if bio == null:
		return 1.0
	var b := int(bio.get_pixel(clampi(int(p.x), 0, W - 1), clampi(int(p.y), 0, H - 1)).r * 255.0 + 0.5)
	return BIOME_MEANDER[b] if b >= 0 and b < BIOME_MEANDER.size() else 0.6

## bruit COHÉRENT partagé (Simplex lissé) — le jitter du méandre. Spatialement lisse → wander organique,
## jamais une dent de scie. Display-only (n'entre pas dans le moteur/déterminisme).
func _meander_noise() -> FastNoiseLite:
	if _riv_noise == null:
		_riv_noise = FastNoiseLite.new()
		_riv_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
		_riv_noise.frequency = 0.05
		_riv_noise.seed = 1337
	return _riv_noise

## PLATITUDE 0..1 dérivée de la pente locale (gradient central du champ de hauteur). Plaine → ~1 (grand
## méandre) ; montagne → ~0 (droit).
func _flatness(hgt: Image, p: Vector2, W: int, H: int) -> float:
	if hgt == null:
		return 1.0
	var x := clampi(int(p.x), 1, W - 2)
	var y := clampi(int(p.y), 1, H - 2)
	var sx := absf(hgt.get_pixel(x + 1, y).r - hgt.get_pixel(x - 1, y).r)
	var sy := absf(hgt.get_pixel(x, y + 1).r - hgt.get_pixel(x, y - 1).r)
	return clampf(1.0 - (sx + sy) * 12.0, 0.0, 1.0)

## VRAI si (p) tombe sur une cellule de mer/lac (ou hors carte).
func _is_sea(sea: Image, p: Vector2, W: int, H: int) -> bool:
	var x := int(round(p.x))
	var y := int(round(p.y))
	if x < 0 or y < 0 or x >= W or y >= H:
		return true
	return sea.get_pixel(x, y).r > 0.5

## lissage moving-average d'une polyligne (fenêtre `win`) → courbes douces, plus d'angles droits.
func _carve_seg(img: Image, a: Vector2, b: Vector2, v: float, wd: int, W: int, H: int) -> void:
	var steps := int(maxf(absf(b.x - a.x), absf(b.y - a.y))) + 1
	for t in range(steps + 1):
		var p := a.lerp(b, float(t) / float(steps))
		_carve_dot(img, p.x, p.y, v, wd, W, H)

## DISQUE DOUX sous-pixel : cœur PLEIN (dist ≤ wd) puis halo LARGE fondu en quadratique jusqu'à 0. La
## distance est mesurée au point SOUS-PIXEL (fx,fy) → bord vraiment anti-crénelé, jamais en marches.
func _carve_dot(img: Image, fx: float, fy: float, v: float, wd: int, W: int, H: int) -> void:
	var core := float(wd)
	var halo := 1.3                                  # halo FIN (rivière fluette, pas un Mississippi) — bord fondu ~1.5 cellule
	var rad := core + halo
	var ri := int(ceil(rad)) + 1
	var cx := int(round(fx))
	var cy := int(round(fy))
	for dy in range(-ri, ri + 1):
		for dx in range(-ri, ri + 1):
			var x := cx + dx
			var y := cy + dy
			if x < 0 or y < 0 or x >= W or y >= H:
				continue
			var dist := sqrt((float(x) - fx) * (float(x) - fx) + (float(y) - fy) * (float(y) - fy))
			if dist > rad:
				continue
			var fall := 1.0 if dist <= core else clampf((rad - dist) / halo, 0.0, 1.0)
			fall = fall * fall                       # falloff quadratique → transition plus douce
			_setmax(img, x, y, v * fall, W, H)

func _setmax(img: Image, x: int, y: int, v: float, W: int, H: int) -> void:
	if x < 0 or y < 0 or x >= W or y >= H:
		return
	if img.get_pixel(x, y).r < v:
		img.set_pixel(x, y, Color(v, 0.0, 0.0))

## TRONC du système = le brin le plus LONG (le cours complet, qui remonte le plus loin vers la
## MONTAGNE) ; à longueur ~égale (copies braidées), on préfère la SOURCE la plus HAUTE. Tout le reste
## CONFLUE vers ce tronc → la rivière vient bien de la montagne, jamais de bras isolé.