extends Node2D
## IsoGround — peint le SOL en SPLAT PAR PIXEL : UN quad couvre la carte iso ; le shader iso_blend
## lit le biome par pixel (carte des biomes) et échantillonne la texture PLATE tuilable de ce biome
## (texture-array) en ESPACE MONDE continu — frontières ONDULÉES par un bruit + fondu inter-biome
## → terrain CONTINU, zéro grille de losanges. Les cellules HIGHLAND (relief/roche) reçoivent une
## FALAISE PLATE : un autotile « blob » 47 états (scree auto-connecté d'une cellule à l'autre),
## intégré DANS le splat (sol top-down, pas de sprite iso) → bord de plateau net, zéro empilement.
##
## Display-only. Espace ISO (la Camera2D fait pan/zoom) ; rendu UNE FOIS au générate (biomes
## ~statiques post-worldgen ; re-dessin sur cataclysme §27). REPLI : pas de tuiles ⇒ ne dessine rien.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const TILE_K := 8              ## cellules monde par tuile iso (granularité du sol)
## Biomes « HIGHLAND » → reçoivent la falaise plate (autotile). TRI SERRÉ : seulement le relief
## RUGUEUX (MOUNTAINS, PEAK, VOLCANO) ; HIGHLANDS (upland DOUX, très répandu) et HILLS en sont EXCLUS
## (sinon « tout devient falaise »). La gentle upland reste son biome de sol (herbe/roche habillable).
const HIGHLAND_BIOMES := [18, 19, 23]
const CLIFF_AT_DIR := "res://assets/scps/pack/iso_tiles/cliff_autotile/"
const DIST_MAX := 24.0         ## plafond du champ de distance highland (cellules) → terrasses + pente
const CITY_CARVE := 3          ## rayon (cellules) dégagé de falaise autour d'une ville → assise plate
## PRIORITÉ de fondu par biome (index = enum Biome) : le PLUS HAUT domine le plus bas. HIÉRARCHIE :
## l'HERBE est le SOCLE BAS ; au-dessus forêt < aride/désert < sable/plage < relief < neige <
## volcan/ronces. L'eau tout en bas (la terre domine la mer = rivage depuis la terre).
const PRIO := [0, 0, 1, 16, 4, 5, 4, 6, 6, 12, 14, 13, 8, 8, 9, 7,
	17, 10, 19, 22, 24, 7, 8, 26, 28]
## VAR variantes de texture PLATE par biome (var/, palette B) → variété par RÉGION (le shader
## choisit/fond une variante selon un bruit basse-fréquence). Index = enum Biome.
const VAR := 4
const BIOME_VAR := [
	["uwtr", "wt5", "wt4", "wt3"], ["wtr", "wt2", "wt3", "wt4"], ["sha", "sh2", "sh3", "sha"],
	["snd", "ds3", "des", "ds2"], ["grs", "gr3", "gr2", "sr1"], ["pal", "pal1", "pal", "pal1"],
	["gr3", "grs", "gr6", "gr2"], ["gr5", "gr4", "sr1", "sr2"], ["gr6", "gr5", "sr2", "gr4"],
	["ds2", "ds3", "qs", "des"], ["des", "ds2", "ds4", "snd"], ["ds3", "snd", "des", "qs"],
	["for", "fo2", "fc1", "fc2"], ["fo2", "for", "fc3", "fc1"], ["fc1", "fc2", "fo2", "for"],
	["m05", "m10", "m15", "m20"], ["rc1", "rc2", "rd1", "rm1"], ["gr2", "sr1", "gr4", "rc1"],
	["sr1", "des", "gr3", "grs"], ["sno", "snf", "sng", "ice"], ["ice", "sno", "snf", "sng"],
	["m20", "m25", "rm1", "m15"], ["m25", "rm2", "m15", "m20"], ["bc2", "bc3", "bc4", "bla"],
	["for", "bc4", "fo2", "fc3"],
]

var _active := false
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté
var _terr_arr: Texture2DArray = null   ## texture-array des sols PLATS, indexée par BIOME
var _bmap: ImageTexture = null         ## couche biome → texture (R = biome/255) pour le splat shader
var _cliff_arr: Texture2DArray = null  ## 94 tuiles d'autotile falaise (ordre des entrées JSON)
var _cliff_mask := {}                  ## mask blob (canon) → [layerA, layerB] (couches du _cliff_arr)
var _cliff_idx: ImageTexture = null    ## carte PAR CELLULE : R = layer+1 (0 = pas de falaise)
var _cliff_h: ImageTexture = null      ## champ de DISTANCE au bord (cellules/DIST_MAX) → rampes d'accès
var _cliff_topbio: ImageTexture = null ## biome du SOMMET (terrain alentour) par cellule highland

## texture-array des tuiles PLATES tuilables : VAR couches par biome (couche = biome*VAR + variante),
## chargées de var/ (palette B). Bâtie une fois. Le shader fond les variantes par région.
const VAR_DIR := "res://assets/scps/pack/iso_tiles/var/"
func _terr_array() -> Texture2DArray:
	if _terr_arr != null:
		return _terr_arr
	var imgs: Array[Image] = []
	for b in range(25):
		var vs: Array = BIOME_VAR[b] if b < BIOME_VAR.size() else []
		for v in range(VAR):
			var img: Image = null
			if not vs.is_empty():
				var p := VAR_DIR + String(vs[v % vs.size()]) + ".png"
				if FileAccess.file_exists(p):
					img = Image.load_from_file(p)
			if img == null:
				img = Image.create(256, 256, false, Image.FORMAT_RGBA8)
				img.fill(Color(1, 0, 1, 1))
			if img.get_format() != Image.FORMAT_RGBA8:
				img.convert(Image.FORMAT_RGBA8)
			if img.get_width() != 256 or img.get_height() != 256:
				img.resize(256, 256)
			img.generate_mipmaps()
			imgs.append(img)
	_terr_arr = Texture2DArray.new()
	_terr_arr.create_from_images(imgs)
	return _terr_arr

## texture-array de l'AUTOTILE falaise (94 tuiles plates 256²) + table mask→couches, lus de index.json.
## Bâti une fois. Le sol highland échantillonne LA couche choisie par le masque blob de la cellule.
func _cliff_autotile() -> Texture2DArray:
	if _cliff_arr != null:
		return _cliff_arr
	var imgs: Array[Image] = []
	_cliff_mask.clear()
	var path := CLIFF_AT_DIR + "index.json"
	if FileAccess.file_exists(path):
		var d = JSON.parse_string(FileAccess.open(path, FileAccess.READ).get_as_text())
		if typeof(d) == TYPE_DICTIONARY:
			var ents: Array = d.get("entries", [])
			for i in range(ents.size()):
				var e: Dictionary = ents[i]
				var img: Image = null
				var p := CLIFF_AT_DIR + String(e["output"]).get_file()
				if FileAccess.file_exists(p):
					img = Image.load_from_file(p)
				if img == null:
					img = Image.create(256, 256, false, Image.FORMAT_RGBA8)
					img.fill(Color(1, 0, 1, 1))
				if img.get_format() != Image.FORMAT_RGBA8:
					img.convert(Image.FORMAT_RGBA8)
				if img.get_width() != 256 or img.get_height() != 256:
					img.resize(256, 256)
				img.generate_mipmaps()
				imgs.append(img)
				var m := int(e["mask"])
				if not _cliff_mask.has(m):
					_cliff_mask[m] = []
				_cliff_mask[m].append(i)
	if imgs.is_empty():        # repli : une couche magenta pour ne pas planter create_from_images
		var mg := Image.create(256, 256, false, Image.FORMAT_RGBA8)
		mg.fill(Color(1, 0, 1, 1)); mg.generate_mipmaps(); imgs.append(mg)
	_cliff_arr = Texture2DArray.new()
	_cliff_arr.create_from_images(imgs)
	return _cliff_arr

func _is_highland(hl: PackedByteArray, nx: int, ny: int, x: int, y: int) -> bool:
	if x < 0 or y < 0 or x >= nx or y >= ny:
		return false
	return hl[y * nx + x] == 1

## nombre de voisins (8) highland (hors-carte = 0).
func _count8(src: PackedByteArray, nx: int, ny: int, x: int, y: int) -> int:
	var c := 0
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			if dx == 0 and dy == 0:
				continue
			var xx := x + dx
			var yy := y + dy
			if xx >= 0 and yy >= 0 and xx < nx and yy < ny and src[yy * nx + xx] == 1:
				c += 1
	return c

## MORPHO du masque highland : COMBLE trous & concavités (≥ FILL voisins → devient highland) puis
## RETIRE les cellules isolées / pointes (≤ SPECK voisins → retiré). 2 passes → massifs PLEINS (zéro
## trou), aucune falaise orpheline, coins saillants rabotés (amorce le « moins carré »).
func _morph_highland(hl: PackedByteArray, nx: int, ny: int) -> void:
	for _pass in range(2):
		var src := hl.duplicate()
		for cy in range(ny):
			for cx in range(nx):
				var i := cy * nx + cx
				var c := _count8(src, nx, ny, cx, cy)
				if src[i] == 0 and c >= 5:
					hl[i] = 1
				elif src[i] == 1 and c <= 2:
					hl[i] = 0       # retire slivers/pointes (≤2 voisins) → plus de petites faces noires éparses

## masque blob 8-voisins (bits n=1 e=2 s=4 w=8 ; diagonales SEULEMENT si les deux cardinaux adjacents).
func _blob_mask(hl: PackedByteArray, nx: int, ny: int, x: int, y: int) -> int:
	var n := _is_highland(hl, nx, ny, x, y - 1)
	var e := _is_highland(hl, nx, ny, x + 1, y)
	var s := _is_highland(hl, nx, ny, x, y + 1)
	var w := _is_highland(hl, nx, ny, x - 1, y)
	var m := 0
	if n: m |= 1
	if e: m |= 2
	if s: m |= 4
	if w: m |= 8
	if n and e and _is_highland(hl, nx, ny, x + 1, y - 1): m |= 16
	if s and e and _is_highland(hl, nx, ny, x + 1, y + 1): m |= 32
	if s and w and _is_highland(hl, nx, ny, x - 1, y + 1): m |= 64
	if n and w and _is_highland(hl, nx, ny, x - 1, y - 1): m |= 128
	return m

## carte PAR CELLULE (nx×ny, R8) : 0 = pas de falaise ; sinon layer+1 (couche d'autotile choisie par
## le masque blob, variante par hash). Le shader lit cette carte au pas de cellule (filter_nearest).
func _build_cliff_idx(w, bio: Image, W: int, H: int) -> void:
	_cliff_autotile()       # garantit _cliff_mask peuplée
	var nx := W / TILE_K
	var ny := H / TILE_K
	var k2 := TILE_K / 2
	var hl := PackedByteArray()
	hl.resize(nx * ny)
	var bcell := PackedByteArray()
	bcell.resize(nx * ny)
	for cy in range(ny):
		for cx in range(nx):
			var bx := mini(cx * TILE_K + k2, W - 1)
			var by := mini(cy * TILE_K + k2, H - 1)
			var b := int(bio.get_pixel(bx, by).r * 255.0 + 0.5)
			bcell[cy * nx + cx] = b
			hl[cy * nx + cx] = 1 if HIGHLAND_BIOMES.has(b) else 0
	_morph_highland(hl, nx, ny)     # comble les trous, retire les falaises isolées, rabote les coins
	# CARVE : aucune falaise SOUS une ville (le bourg s'embourbait dans la paroi). On dégage un disque
	# autour de chaque centroïde colonisé → assise PLATE pour le village.
	if w != null:
		for r in range(w.region_count()):
			if not w.region_colonized(r):
				continue
			var ctr: Vector2 = w.region_centroid(r)
			var ccx := int(ctr.x) / TILE_K
			var ccy := int(ctr.y) / TILE_K
			for dy in range(-CITY_CARVE, CITY_CARVE + 1):
				for dx in range(-CITY_CARVE, CITY_CARVE + 1):
					if dx * dx + dy * dy > CITY_CARVE * CITY_CARVE:
						continue
					var xx := ccx + dx
					var yy := ccy + dy
					if xx >= 0 and yy >= 0 and xx < nx and yy < ny:
						hl[yy * nx + xx] = 0
	var img := Image.create(nx, ny, false, Image.FORMAT_R8)
	for cy in range(ny):
		for cx in range(nx):
			if hl[cy * nx + cx] == 0:
				continue
			var mask := _blob_mask(hl, nx, ny, cx, cy)
			var layers: Array = _cliff_mask.get(mask, [])
			if layers.is_empty():
				layers = _cliff_mask.get(mask & 15, [])   # repli : cardinaux seuls
			if layers.is_empty():
				continue
			var h := (cx * 73856093) ^ (cy * 19349663)
			var layer: int = layers[absi(h) % layers.size()]
			img.set_pixel(cx, cy, Color(float(layer + 1) / 255.0, 0.0, 0.0))
	_cliff_idx = ImageTexture.create_from_image(img)
	# CHAMP DE DISTANCE au bord (en CELLULES) : 0 hors highland, CROÎT vers le cœur (BFS multi-source
	# depuis les cellules NON-highland). Quantifié, il donne les TERRASSES (marches concentriques) ;
	# son GRADIENT donne la direction de PENTE (flanc 30-45°) ; ses 1ères cellules, le fondu de bord.
	var n := nx * ny
	var dist := PackedFloat32Array()
	dist.resize(n)
	var q := PackedInt32Array()
	for i in range(n):
		if hl[i] == 1:
			dist[i] = 1.0e9
		else:
			dist[i] = 0.0
			q.append(i)
	var head := 0
	while head < q.size():
		var idx := q[head]
		head += 1
		var cx := idx % nx
		var cy := idx / nx
		var nd := dist[idx] + 1.0
		for ni in [idx - 1 if cx > 0 else -1, idx + 1 if cx < nx - 1 else -1,
				idx - nx if cy > 0 else -1, idx + nx if cy < ny - 1 else -1]:
			if ni >= 0 and dist[ni] > nd:
				dist[ni] = nd
				q.append(ni)
	var df := Image.create(nx, ny, false, Image.FORMAT_R8)
	for cy in range(ny):
		for cx in range(nx):
			var dv: float = dist[cy * nx + cx]
			df.set_pixel(cx, cy, Color(minf(dv, DIST_MAX) / DIST_MAX, 0.0, 0.0))
	_cliff_h = ImageTexture.create_from_image(df)
	# BIOME du SOMMET : le biome de TERRAIN OUVERT le plus proche (herbe/plaine/steppe/désert/NEIGE),
	# propagé À TRAVERS la terre par BFS. On EXCLUT les biomes SOMBRES (forêt/jungle/marais) et rocheux
	# des sources → le plateau garde un sol CLAIR & lisible (jamais de forêt ni de roche sur le dessus ;
	# la roche reste sur la FACE). Ouvert = 3..11 (côte→désert côtier) + 20 (glacier/neige).
	var topb := bcell.duplicate()
	var bdist := PackedInt32Array()
	bdist.resize(n)
	var q2 := PackedInt32Array()
	for i in range(n):
		var bb := int(bcell[i])
		if (bb >= 3 and bb <= 11) or bb == 20:
			bdist[i] = 0
			topb[i] = bcell[i]
			q2.append(i)
		else:
			bdist[i] = 1 << 28
	var h2 := 0
	while h2 < q2.size():
		var idx := q2[h2]
		h2 += 1
		var cx := idx % nx
		var cy := idx / nx
		var nd := bdist[idx] + 1
		for ni in [idx - 1 if cx > 0 else -1, idx + 1 if cx < nx - 1 else -1,
				idx - nx if cy > 0 else -1, idx + nx if cy < ny - 1 else -1]:
			if ni >= 0 and bcell[ni] >= 3 and bdist[ni] > nd:   # propage à travers la TERRE (pas la mer)
				bdist[ni] = nd
				topb[ni] = topb[idx]
				q2.append(ni)
	var tb := Image.create(nx, ny, false, Image.FORMAT_R8)
	for cy in range(ny):
		for cx in range(nx):
			tb.set_pixel(cx, cy, Color(float(topb[cy * nx + cx]) / 255.0, 0.0, 0.0))
	_cliff_topbio = ImageTexture.create_from_image(tb)

func _ready() -> void:
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_tick)
	_active = UIKit.has_iso_tiles()
	_setup_blend()
	queue_redraw()

## monte le ShaderMaterial de fondu DIRECTIONNEL (rivage). Sans lui → tuiles brutes à bords francs.
func _setup_blend() -> void:
	var sh := load("res://map/iso_blend.gdshader")
	if sh == null:
		return
	var mat := ShaderMaterial.new()
	mat.shader = sh
	var noise := UIKit.blend_noise()        # stamp fbm seamless → bord organique cohérent
	if noise != null:
		mat.set_shader_parameter("noise_tex", noise)
	material = mat
	texture_repeat = CanvasItem.TEXTURE_REPEAT_ENABLED   # la texture PLATE COULE (UV monde > 1 → wrap)
	# Filtre du NŒUD = LINEAR (pour la TEXTURE intégrée = sprites de falaise) ; le splat anti-moiré via
	# le hint `filter_linear_mipmap` de l'uniforme `terrains` (indépendant). MIPMAP ici rendait les
	# falaises INVISIBLES (la TEXTURE intégrée échantillonnée en mipmap sans mips → vide).
	texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	_shaded = true

func is_active() -> bool:
	return _active

func _on_generated() -> void:
	_active = UIKit.has_iso_tiles()
	_bmap = null            # nouveau monde → recharge la carte des biomes pour le splat
	_cliff_idx = null       # … et recalcule l'autotile falaise (+ distance + biome de sommet)
	_cliff_h = null
	_cliff_topbio = null
	queue_redraw()

func _on_tick(_y: int) -> void:
	# les biomes ne bougent qu'en FIN §27 (cataclysme) → re-dessin seulement alors
	var w = Sim.world
	if w != null and int((w.endgame_info() as Dictionary).get("fin", 0)) > 0:
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
	# uniformes du splat : carte des biomes (R = biome/255) + texture-array des sols plats + autotile falaise
	var mat := material as ShaderMaterial
	if mat != null:
		if _bmap == null:
			_bmap = ImageTexture.create_from_image(bio)
		if _cliff_idx == null:
			_build_cliff_idx(w, bio, W, H)
		mat.set_shader_parameter("biome_map", _bmap)
		mat.set_shader_parameter("map_size", Vector2(W, H))
		mat.set_shader_parameter("terrains", _terr_array())
		mat.set_shader_parameter("cliff_atlas", _cliff_autotile())
		mat.set_shader_parameter("cliff_idx", _cliff_idx)
		mat.set_shader_parameter("cliff_dist", _cliff_h)
		mat.set_shader_parameter("cliff_top_biome", _cliff_topbio)
		mat.set_shader_parameter("dist_max", DIST_MAX)
		mat.set_shader_parameter("cliff_grid", Vector2(W / TILE_K, H / TILE_K))
		mat.set_shader_parameter("tile_k", float(TILE_K))
	# SOL = UN seul QUAD couvrant la carte iso (x∈[-H,W], y∈[0,(W+H)/2]) → splat PAR PIXEL dans le shader
	draw_rect(Rect2(-float(H), 0.0, float(W + H), float(W + H) * 0.5), Color(0.0, 0.0, 0.0, 1.0))

