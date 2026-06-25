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
## CARVE STRATÉGIQUE : la worldgen sème des fleuves PARTOUT → on ne grave QUE les majeurs. On
## CLUSTER par embouchure (une baie = un système), on garde le TRONC le plus long, on le grave en
## CONTINU (pinceau ∝ débit) dans le champ. Le shader y lit l'eau (cœur propre, berges fondues).
const RIVER_MERGE := 64.0   ## embouchures à ≤ ça = UN système (le tronc le plus long le représente)
const RIVER_MAX_SYS := 5    ## systèmes (les plus forts) gravés
const RIVER_MIN_PTS := 12   ## tronc plus court = stub → système ignoré
const RIVER_SECOND_MIN := 8 ## affluent dont la course AMONT (source→confluence) est plus courte = bruit → ignoré
const RIVER_SECOND_MAX := 3 ## affluents gravés au plus par système (tronc + 3) → réseau lisible, pas d'inondation
const RIVER_FLOW_FLOOR := 0.16  ## fraction du plus fort débit en-dessous de laquelle on s'arrête
const ESTUARY_CORE := 4      ## rayon du cœur d'eau à l'EMBOUCHURE (le fleuve s'ÉVASE dans la mer → accès LARGE, pas bouché par le sable)
const TILE_K := 8              ## cellules monde par tuile iso (granularité du sol)
## Biomes « HIGHLAND » → reçoivent la falaise plate (autotile). TRI SERRÉ : seulement le relief
## RUGUEUX (MOUNTAINS, PEAK, VOLCANO) ; HIGHLANDS (upland DOUX, très répandu) et HILLS en sont EXCLUS
## (sinon « tout devient falaise »). La gentle upland reste son biome de sol (herbe/roche habillable).
const HIGHLAND_BIOMES := [18, 19, 23]
const CLIFF_AT_DIR := "res://assets/scps/pack/iso_tiles/cliff_autotile/"
const ROAD_SURF := "road_cobble"   ## surface routière du pack (mud/gravel/cobble)
const ROAD_DIR := "res://assets/scps/pack/iso_tiles/road_cobble/"
## tuiles cobble TRANSPARENTES (RGBA épars) — de vraies TUILES échantillonnées sur le plan du sol par le
## shader. Couche 0 = flat (rangées monde-X, routes "/") · couche 1 = flat PIVOTÉE 90° (rangées monde-Y,
## routes "\") · couche 2 = diagonale (jonctions). La verticale est le 1er fichier dupliqué tourné de 90°.
const COBBLE_DIR := "res://assets/scps/pack/iso_tiles/road_detail/"
const COBBLE_TILES := ["scps_cobbles_sparse_flat_01", "scps_cobbles_sparse_flat_vertical_01",
	"scps_cobbles_sparse_flat_diagonal_01"]
var _cobble_arr: Texture2DArray = null
const DIST_MAX := 24.0         ## plafond du champ de distance highland (cellules) → terrasses + pente
const CITY_CARVE := 3          ## rayon (cellules) dégagé de falaise autour d'une ville → assise plate
const MIN_CLUSTER := 9         ## taille mini d'un massif (cellules) → pas de petites falaises éparses
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
var _antique := false       ## mode « carte ancienne » : shader cartographique TOP-DOWN (quad plat)
const ANTIQUE_MARGIN := 48.0   ## marge de PAPIER (unités monde) autour de la carte en mode top-down
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté
var _terr_arr: Texture2DArray = null   ## texture-array des sols PLATS, indexée par BIOME
var _bmap: ImageTexture = null         ## couche biome → texture (R = biome/255) pour le splat shader
var _river_map: ImageTexture = null    ## couche DÉBIT (c->river/255) → le shader carve la rivière comme l'eau
var _river_field: Image = null         ## la MÊME couche débit en Image L8 (échantillonnée par l'overlay : pas d'asset BASE dans l'eau de rivière)
var _cliff_arr: Texture2DArray = null  ## 94 tuiles d'autotile falaise (ordre des entrées JSON)
var _cliff_mask := {}                  ## mask blob (canon) → [layerA, layerB] (couches du _cliff_arr)
var _cliff_idx: ImageTexture = null    ## carte PAR CELLULE : R = layer+1 (0 = pas de falaise)
var _cliff_h: ImageTexture = null      ## champ de DISTANCE au bord (cellules/DIST_MAX) → rampes d'accès
var _cliff_topbio: ImageTexture = null ## biome du SOMMET (terrain alentour) par cellule highland
var _road_arr: Texture2DArray = null   ## tuiles ROUTE (autotile cardinal) — couches = entrées chargées
var _road_mask := {}                   ## masque cardinal 1-15 → [couches] (variantes)
var _road_idx: ImageTexture = null     ## carte PAR CELLULE : R = couche+1 (0 = pas de route)
var _road_cov: ImageTexture = null     ## couverture LISSE (1 route, 0 sinon, filtre linéaire) → fondu de bord
var _road_pave: Texture2D = null       ## tuile pavé seamless (centre de la chaussée)
const ROAD_PAVE := "res://assets/scps/pack/iso_tiles/road_detail/cobblestone-path-seamless-512.png"
const ROAD_CLEAR_R := 8                 ## rayon (cellules) effacé de route autour d'un centre → la dalle n'est pas traversée
var _city_wear: ImageTexture = null     ## SOL URBAIN (terre battue sous les bourgs) — champ lisse comme road_cov
const CITY_WEAR_BANDS := [150, 400, 900, 1800, 3500, 7000, 14000]   ## seuils de pop → band (miroir overlay)
var _road_sig := -1                    ## signature du réseau (nb cellules) → ne reposer que si ça change

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

## retire les PETITS amas highland (composantes connexes < MIN_CLUSTER cellules) → fini les petites
## falaises éparses qui rendaient des "stries" noires ; seuls les VRAIS massifs gardent une falaise.
func _remove_small_clusters(hl: PackedByteArray, nx: int, ny: int) -> void:
	var seen := PackedByteArray()
	seen.resize(nx * ny)
	for start in range(nx * ny):
		if hl[start] == 0 or seen[start] == 1:
			continue
		var comp := PackedInt32Array()
		comp.append(start)
		seen[start] = 1
		var h := 0
		while h < comp.size():
			var idx := comp[h]
			h += 1
			var cx := idx % nx
			var cy := idx / nx
			for ni in [idx - 1 if cx > 0 else -1, idx + 1 if cx < nx - 1 else -1,
					idx - nx if cy > 0 else -1, idx + nx if cy < ny - 1 else -1]:
				if ni >= 0 and hl[ni] == 1 and seen[ni] == 0:
					seen[ni] = 1
					comp.append(ni)
		if comp.size() < MIN_CLUSTER:
			for idx2 in comp:
				hl[idx2] = 0

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
	_remove_small_clusters(hl, nx, ny)   # retire les petits amas → fini les "stries" noires éparses
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
	# FALAISES 3D : on passe le masque highland FINAL + le biome par cellule (face) + le biome de sommet
	# (coiffe) au système de micro-mesh (le frère Cliff3D) — la MÊME donnée qui nourrit le lift du shader
	# → les faces 3D épousent exactement le rebord du plateau. (No-op si Cliff3D absent.)
	var _par := get_parent()
	var _c3d: Node = _par.get_node_or_null("Cliff3D") if _par != null else null
	if _c3d != null and _c3d.has_method("build"):
		_c3d.call("build", hl, nx, ny, bcell, topb)

## texture-array des tuiles ROUTE (cardinal 15 états × 2 variantes) + table masque→couches. Bâti 1×.
func _road_atlas() -> Texture2DArray:
	if _road_arr != null:
		return _road_arr
	var imgs: Array[Image] = []
	_road_mask.clear()
	for m in range(1, 16):
		for v in [1, 2]:
			var p := ROAD_DIR + "scps_" + ROAD_SURF + "_%02d_%02d.png" % [m, v]
			if FileAccess.file_exists(p):
				var img := Image.load_from_file(p)
				if img != null:
					if img.get_format() != Image.FORMAT_RGBA8:
						img.convert(Image.FORMAT_RGBA8)
					if img.get_width() != 256 or img.get_height() != 256:
						img.resize(256, 256)
					img.generate_mipmaps()
					if not _road_mask.has(m):
						_road_mask[m] = []
					_road_mask[m].append(imgs.size())
					imgs.append(img)
	if imgs.is_empty():
		var mg := Image.create(256, 256, false, Image.FORMAT_RGBA8)
		mg.fill(Color(1, 0, 1, 1)); mg.generate_mipmaps(); imgs.append(mg)
	_road_arr = Texture2DArray.new()
	_road_arr.create_from_images(imgs)
	return _road_arr

## texture-array des 3 tuiles cobble (0 flat · 1 flat pivotée 90° · 2 diagonale) → plan du sol (shader).
func _cobble_atlas() -> Texture2DArray:
	if _cobble_arr != null:
		return _cobble_arr
	var imgs: Array[Image] = []
	for nm in COBBLE_TILES:
		var p: String = COBBLE_DIR + str(nm) + ".png"
		if FileAccess.file_exists(p):
			var img := Image.load_from_file(p)
			if img != null:
				if img.get_format() != Image.FORMAT_RGBA8:
					img.convert(Image.FORMAT_RGBA8)
				if img.get_width() != 256 or img.get_height() != 256:
					img.resize(256, 256)
				imgs.append(img)
	if imgs.size() < 3:
		return null
	_cobble_arr = Texture2DArray.new()
	_cobble_arr.create_from_images(imgs)
	return _cobble_arr

## carte PAR CELLULE (nx×ny) de la route : R = couche+1 (0 = pas de route) + couverture lisse. Le réseau
## (façade `road_paths`) est quantifié sur la grille losange, les pas DIAGONAUX sont COMBLÉS (4-connexité),
## puis chaque cellule prend le masque CARDINAL de ses 4 voisins → la couche d'autotile. Le shader pose la
## tuile comme SOL et la fond au bord via la couverture (filtre linéaire).
func _build_road_idx(w, W: int, H: int) -> void:
	_road_atlas()
	var nx := W / TILE_K
	var ny := H / TILE_K
	var grid := {}
	var npts := 0
	for rd in w.road_paths():
		var pts: PackedVector2Array = rd["points"]
		npts += pts.size()
		for p in pts:
			var cx := int(p.x) / TILE_K
			var cy := int(p.y) / TILE_K
			if cx >= 0 and cy >= 0 and cx < nx and cy < ny:
				grid[Vector2i(cx, cy)] = true
	# RACCORD SUD aux TOURS : la tour est posée au pied SUD (ancre) de sa tuile ; le réseau, lui, vise le
	# CENTROÏDE. On ajoute un court stub depuis l'ancre vers le sud (iso-sud = +x+y) sur terre → la route
	# rejoint le pied de la tour (« snap sud »), comme l'ancienne rue principale.
	var sea: Image = w.layer_image(4)        # SCPS_LAYER_WATER (mer/lac) → le stub s'arrête à l'eau
	for r in range(w.region_count()):
		if not w.region_colonized(r):
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var col := int(ctr.x) / TILE_K
		var row := int(ctr.y) / TILE_K
		for i in range(0, 3):                # ancre (col+1,row+1) puis 2 cellules plus au sud
			var sx := col + 1 + i
			var sy := row + 1 + i
			if sx < 0 or sy < 0 or sx >= nx or sy >= ny:
				break
			var wx := mini(sx * TILE_K + TILE_K / 2, W - 1)
			var wy := mini(sy * TILE_K + TILE_K / 2, H - 1)
			if sea != null and int(sea.get_pixel(wx, wy).r * 255.0 + 0.5) >= 1:
				break                        # eau devant → on arrête le stub
			grid[Vector2i(sx, sy)] = true
	# 4-CONNEXITÉ : tout lien seulement diagonal reçoit une cellule cardinale de comblement
	var fill := {}
	for ck in grid.keys():
		var c: Vector2i = ck
		for d in [Vector2i(1, 1), Vector2i(1, -1), Vector2i(-1, 1), Vector2i(-1, -1)]:
			if grid.has(c + d):
				var aa := Vector2i(c.x + d.x, c.y)
				var bb := Vector2i(c.x, c.y + d.y)
				if not grid.has(aa) and not grid.has(bb) and not fill.has(aa) and not fill.has(bb):
					fill[aa] = true
	for fk in fill.keys():
		grid[fk] = true
	var img := Image.create(nx, ny, false, Image.FORMAT_RGB8)   # R=couche+1 · G=masque cardinal (forme route)
	for ck2 in grid.keys():
		var cell: Vector2i = ck2
		var m := 0
		if grid.has(Vector2i(cell.x, cell.y - 1)): m |= 1   # n
		if grid.has(Vector2i(cell.x + 1, cell.y)): m |= 2   # e
		if grid.has(Vector2i(cell.x, cell.y + 1)): m |= 4   # s
		if grid.has(Vector2i(cell.x - 1, cell.y)): m |= 8   # w
		if m == 0:
			m = 15
		var layers: Array = _road_mask.get(m, [])
		if layers.is_empty():
			layers = _road_mask.get(15, [])
		if layers.is_empty():
			continue
		var h := (cell.x * 73856093) ^ (cell.y * 19349663)
		var layer: int = layers[absi(h) % layers.size()]
		img.set_pixel(cell.x, cell.y, Color(float(layer + 1) / 255.0, float(m) / 255.0, 0.0))
	_road_idx = ImageTexture.create_from_image(img)
	_road_sig = npts

func _ready() -> void:
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_tick)
	_active = UIKit.has_iso_tiles()
	_setup_blend()
	queue_redraw()

## monte le ShaderMaterial de fondu DIRECTIONNEL (rivage). Sans lui → tuiles brutes à bords francs.
func _setup_blend() -> void:
	# OPT-IN « carte ancienne » (essai) : un shader cartographique remplace le splat réaliste. Display-only,
	# déclenché par l'argument `antique=on` (ou SCPS_ANTIQUE) → ne touche pas le rendu par défaut.
	_antique = OS.has_environment("SCPS_ANTIQUE")
	for a in OS.get_cmdline_user_args():
		if a == "antique=on":
			_antique = true
	var sh := load("res://map/iso_antique.gdshader") if _antique else load("res://map/iso_blend.gdshader")
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
	_river_map = null       # … et la couche débit (rivières carvées)
	_river_field = null
	_cliff_idx = null       # … et recalcule l'autotile falaise (+ distance + biome de sommet)
	_cliff_h = null
	_cliff_topbio = null
	_road_idx = null        # … et repose les tuiles de route (réseau du monde neuf)
	_road_cov = null
	_city_wear = null       # … et le sol urbain (bourgs du monde neuf)
	_road_sig = -1
	queue_redraw()

func _on_tick(_y: int) -> void:
	var w = Sim.world
	if w == null:
		return
	_city_wear = null       # la pop a bougé → le sol urbain (blob ∝ band) se recale
	# le RÉSEAU de routes grandit/change (conquête) → reposer les tuiles si le nb de cellules a bougé
	if _road_sig >= 0:
		var n := 0
		for rd in w.road_paths():
			n += (rd["points"] as PackedVector2Array).size()
		if n != _road_sig:
			_road_idx = null
			_road_cov = null
			queue_redraw()
	# les biomes ne bougent qu'en FIN §27 (cataclysme) → re-dessin aussi alors
	if int((w.endgame_info() as Dictionary).get("fin", 0)) > 0:
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
		if _river_map == null:
			if _river_field == null:
				_river_field = _build_river_field(w, W, H)
			_river_map = ImageTexture.create_from_image(_river_field)
		if _cliff_idx == null:
			_build_cliff_idx(w, bio, W, H)
		mat.set_shader_parameter("biome_map", _bmap)
		if _river_map != null:
			mat.set_shader_parameter("river_map", _river_map)
		mat.set_shader_parameter("map_size", Vector2(W, H))
		mat.set_shader_parameter("terrains", _terr_array())
		mat.set_shader_parameter("cliff_atlas", _cliff_autotile())
		mat.set_shader_parameter("cliff_idx", _cliff_idx)
		mat.set_shader_parameter("cliff_dist", _cliff_h)
		mat.set_shader_parameter("cliff_top_biome", _cliff_topbio)
		mat.set_shader_parameter("dist_max", DIST_MAX)
		mat.set_shader_parameter("cliff_grid", Vector2(W / TILE_K, H / TILE_K))
		mat.set_shader_parameter("tile_k", float(TILE_K))
		# ROUTES = CHAMP LISSE (comme la rivière) : on rasterise les polylignes en couverture douce
		# (`road_cov`, filtre linéaire), pas un pâté cardinal en losange. Le shader lit le champ → bord
		# terreux (road_worn) + centre PAVÉ (tuile cobblestone seamless), avec warp de bruit sur le bord.
		if _road_cov == null:
			_road_cov = ImageTexture.create_from_image(_build_road_cov(w, W, H))
			var nn := 0
			for rd in w.road_paths():
				nn += (rd["points"] as PackedVector2Array).size()
			_road_sig = nn
		mat.set_shader_parameter("road_cov", _road_cov)
		mat.set_shader_parameter("map_size", Vector2(W, H))
		var pave := _road_pave_tex()
		if pave != null:
			mat.set_shader_parameter("road_pave", pave)
		# SOL URBAIN : terre battue graduelle sous les bourgs (remplace les dalles « pâté » de l'overlay)
		if _city_wear == null:
			_city_wear = ImageTexture.create_from_image(_build_city_wear(w, W, H))
		mat.set_shader_parameter("city_wear", _city_wear)
		mat.set_shader_parameter("road_on", 1.0)
		mat.set_shader_parameter("flat_map", 1.0 if _antique else 0.0)
	# SOL = UN seul QUAD couvrant la carte iso (x∈[-H,W], y∈[0,(W+H)/2]) → splat PAR PIXEL dans le shader
	if _antique:
		# CARTE ANCIENNE : quad TOP-DOWN (monde direct) + marge de PAPIER → vraie carte a plat.
		draw_rect(Rect2(-ANTIQUE_MARGIN, -ANTIQUE_MARGIN, float(W) + 2.0 * ANTIQUE_MARGIN,
			float(H) + 2.0 * ANTIQUE_MARGIN), Color(0.0, 0.0, 0.0, 1.0))
	else:
		draw_rect(Rect2(-float(H), 0.0, float(W + H), float(W + H) * 0.5), Color(0.0, 0.0, 0.0, 1.0))

## tuile pavé seamless (1×) — centre de la chaussée, échantillonnée tuilée sur le plan du sol.
func _road_pave_tex() -> Texture2D:
	if _road_pave != null:
		return _road_pave
	if FileAccess.file_exists(ROAD_PAVE):
		var img := Image.load_from_file(ROAD_PAVE)
		if img != null:
			if img.get_format() != Image.FORMAT_RGBA8:
				img.convert(Image.FORMAT_RGBA8)
			img.generate_mipmaps()
			_road_pave = ImageTexture.create_from_image(img)
	return _road_pave

## CHAMP de COUVERTURE des routes (L8) — le pendant ROUTE de `river_map` : on rasterise les polylignes
## en pinceau DOUX (réutilise `_carve_brush`), continu le long du fil. LARGEUR VARIABLE : plus étroit en
## FORÊT (2), plus large près des BOURGS (extrémités, +1). Le shader lit ce champ (filtre linéaire) → bord
## terreux + centre pavé, sans masque cardinal. Pas de route sur l'eau (le pont franchit).
func _build_road_cov(w, W: int, H: int) -> Image:
	var img := Image.create(W, H, false, Image.FORMAT_L8)
	if w == null or not w.has_method("road_paths"):
		return img
	var bio: Image = w.layer_image(LAYER_BIOME)
	var sea: Image = w.layer_image(4)
	for rd in w.road_paths():
		var pts: PackedVector2Array = rd["points"]
		var n := pts.size()
		if n < 2:
			continue
		for i in range(n - 1):
			var a: Vector2 = pts[i]
			var b: Vector2 = pts[i + 1]
			var steps := maxi(1, int(ceil(a.distance_to(b))))
			for s in range(steps + 1):
				var p: Vector2 = a.lerp(b, float(s) / float(steps))
				var ix := int(p.x)
				var iy := int(p.y)
				if ix < 0 or iy < 0 or ix >= W or iy >= H:
					continue
				if sea != null and int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1:
					continue                              # eau → le pont franchit, pas de chaussée
				var soft := 2                             # SENTE médiévale ÉTROITE (ex-3 : la route faisait ~BLD_SIZE)
				if bio != null:
					var bb := int(bio.get_pixel(ix, iy).r * 255.0 + 0.5)
					if bb == 12 or bb == 13 or bb == 14:
						soft = 2                          # FORÊT : sente étroite
				if mini(i, n - 2 - i) <= 2:
					soft += 1                             # évasement près du bourg (extrémités)
				_carve_brush(img, ix, iy, 1.0, 1, soft, W, H)
	# CLEARANCE : la route MÈNE au bourg sans le TRAVERSER. On EFFACE la couverture dans un rayon autour
	# de chaque centre (l'emprise de la dalle de fondation) → sinon la route passe « par-dessus » la
	# fondation (elle perçait sous ses bords semi-transparents). Le tracé s'arrête net au bord de la dalle.
	for r in range(w.region_count()):
		if not w.region_colonized(r):
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var cx := int(ctr.x)
		var cy := int(ctr.y)
		for dy in range(-ROAD_CLEAR_R, ROAD_CLEAR_R + 1):
			for dx in range(-ROAD_CLEAR_R, ROAD_CLEAR_R + 1):
				if dx * dx + dy * dy > ROAD_CLEAR_R * ROAD_CLEAR_R:
					continue
				var x := cx + dx
				var y := cy + dy
				if x >= 0 and y >= 0 and x < W and y < H:
					img.set_pixel(x, y, Color(0.0, 0.0, 0.0))
	# A1.5 / B3 / C2 — RUES PAVÉES DU BOURG : par-dessus la clairance, on grave le squelette de rues (overlay).
	# Le SOL URBAIN (city_wear) tasse déjà tout le bourg en TERRE BATTUE → une sente de terre s'y NOIE. Les
	# rues doivent donc RESSORTIR : on les PAVE (peak > road_pave_edge) → chaussée claire sur le brun du bourg,
	# exactement la lecture « ville médiévale : terre tassée + rues pavées ». RAYONNENT du centre vers les
	# routes ; la clairance ne les efface pas (gravées APRÈS). Le point de CONVERGENCE (origine commune des
	# rues principales) reçoit une PLACE pavée (§C2 marché ; §B3 assise des monuments civiques au cœur).
	var par := get_parent()
	var ov: Node = par.get_node_or_null("Overlay") if par != null else null
	if ov != null and ov.has_method("town_streets"):
		var plazas := {}
		for st in ov.call("town_streets"):
			var a: Vector2 = st["a"]
			var b: Vector2 = st["b"]
			var is_main: bool = bool(st.get("main", true))
			if is_main:
				plazas[Vector2i(int(a.x), int(a.y))] = a   # origine = cœur du bourg → place du marché
			var pk := 0.80 if is_main else 0.66            # grande rue PAVÉE nette · ruelle plus fine
			var sf := 2 if is_main else 1
			var steps := maxi(1, int(ceil(a.distance_to(b))))
			for s in range(steps + 1):
				var p: Vector2 = a.lerp(b, float(s) / float(steps))
				var ix := int(p.x)
				var iy := int(p.y)
				if ix < 0 or iy < 0 or ix >= W or iy >= H:
					continue
				if sea != null and int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1:
					continue
				_carve_brush(img, ix, iy, pk, 1, sf, W, H)
		# PLACE DU MARCHÉ : disque pavé au point de convergence (§C2) — l'assise des monuments civiques (§B3).
		for key in plazas:
			var c: Vector2 = plazas[key]
			_carve_brush(img, int(c.x), int(c.y), 0.88, 3, 5, W, H)
	return img

## CHAMP de SOL URBAIN (RF) : un BLOB ORGANIQUE par bourg (centré sur le centroïde, rayon ∝ band, contour
## LOBÉ par harmoniques d'angle → tache d'huile, pas un timbre rond). 1 au cœur, 0 au bord (smoothstep).
## Le shader le rend en terre battue graduelle. Jamais sur l'eau. (Les rues l'épaississent — phase A1.)
func _build_city_wear(w, W: int, H: int) -> Image:
	var img := Image.create(W, H, false, Image.FORMAT_RF)
	if w == null:
		return img
	var sea: Image = w.layer_image(4)
	for r in range(w.region_count()):
		if w.region_tier(r) < 0:
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var pop: int = w.region_pop(r)
		var band := 1
		for thr in CITY_WEAR_BANDS:
			if pop >= thr:
				band += 1
		var r_in := 6.0 + float(band) * 2.0
		var r_out := r_in + 8.0
		var seed := float(r) * 1.7
		var cx := int(ctr.x)
		var cy := int(ctr.y)
		var rr := int(ceil(r_out)) + 2
		for dy in range(-rr, rr + 1):
			for dx in range(-rr, rr + 1):
				var x := cx + dx
				var y := cy + dy
				if x < 0 or y < 0 or x >= W or y >= H:
					continue
				if sea != null and int(sea.get_pixel(x, y).r * 255.0 + 0.5) >= 1:
					continue                                  # pas de terre battue sur l'eau
				var dist := sqrt(float(dx * dx + dy * dy))
				var ang := atan2(float(dy), float(dx))
				var rmod := 1.0 + 0.25 * sin(ang * 3.0 + seed) + 0.14 * sin(ang * 5.0 - seed)   # contour LOBÉ
				var eff := dist / maxf(0.3, rmod)
				var v := 1.0 - smoothstep(r_in, r_out, eff)   # 1 au cœur, 0 au bord
				if v <= 0.0:
					continue
				if img.get_pixel(x, y).r < v:
					img.set_pixel(x, y, Color(v, 0.0, 0.0))
	return img

## le CHAMP DÉBIT carvé (L8), bâti à la DEMANDE et mis en cache — la MÊME donnée que le shader rend en
## eau. L'overlay l'échantillonne pour interdire la BASE d'un asset dans l'eau de rivière (≥ river_water).
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
	var water: Image = w.layer_image(4)   # SCPS_LAYER_WATER : mer OU lac (255) → prolonger le fleuve jusque dedans
	var height: Image = w.layer_image(0)  # SCPS_LAYER_HEIGHT : choisir le TRONC qui vient de la MONTAGNE (source haute)
	# 1) CLUSTER par embouchure (distance) → un système = une baie ; on garde TOUS ses brins (tronc +
	#    affluents). Les copies braidées se RECOUVRENT dans le champ (fusionnées au max) ; les VRAIS
	#    affluents ajoutent des branches secondaires.
	var systems := []
	for rv in raw:
		var pts: PackedVector2Array = rv["points"]
		if pts.size() < 2:
			continue
		var mouth: Vector2 = pts[pts.size() - 1]
		var f := float(rv["flow"])
		var placed := false
		for s in systems:
			if mouth.distance_to(s["mouth"]) < RIVER_MERGE:
				(s["strands"] as Array).append(rv)
				s["flow"] = maxf(float(s["flow"]), f)
				if pts.size() > s["trunk_len"]:
					s["trunk_len"] = pts.size()
				placed = true
				break
		if not placed:
			systems.append({"mouth": mouth, "flow": f, "strands": [rv], "trunk_len": pts.size()})
	if systems.is_empty():
		return img
	# 2) graver les RIVER_MAX_SYS systèmes les plus forts ; pour chacun, TOUS ses brins (secondaires
	#    inclus, plus fins) — le champ fusionne les recouvrements, les affluents restent distincts.
	systems.sort_custom(func(a, b): return float(a["flow"]) > float(b["flow"]))
	var fmax := float(systems[0]["flow"])
	if fmax <= 0.0:
		fmax = 1.0
	var n := 0
	for s in systems:
		if n >= RIVER_MAX_SYS:
			break
		var f := float(s["flow"])
		if f < RIVER_FLOW_FLOOR * fmax:
			break
		if int(s["trunk_len"]) < RIVER_MIN_PTS:
			continue                                  # même le tronc est un stub → système écarté
		var strands: Array = s["strands"]
		# TRONC = le brin venant de la MONTAGNE (source la plus HAUTE) parmi les longs → une VRAIE
		# rivière source→mer, jamais un bras isolé.
		var trunk: Dictionary = _pick_trunk(strands, height, W, H)
		if trunk.is_empty():
			continue
		var tpts: PackedVector2Array = trunk["points"]
		var trel := float(trunk["flow"]) / fmax
		_carve_path(img, tpts, trel, W, H)            # tronc source→embouchure = le réseau de référence
		# AFFLUENTS : SEULEMENT ceux qui CONFLUENT avec le réseau (clipés à la jonction) → JAMAIS de bras
		# isolé. Du plus long au plus court, plafonnés. (Le tronc & les copies braidées confluent dès leur
		# source — déjà gravée — donc jct≈0 → sautés ; un cours sans confluence amont est un bras isolé → sauté.)
		strands.sort_custom(func(a, b): return (a["points"] as PackedVector2Array).size() > (b["points"] as PackedVector2Array).size())
		var drew := 1
		for rv in strands:
			if drew > RIVER_SECOND_MAX:
				break
			var pts: PackedVector2Array = rv["points"]
			if pts.size() < RIVER_SECOND_MIN:
				continue
			var jct := -1
			for k in range(pts.size()):              # 1re cellule (depuis la SOURCE) déjà gravée = la confluence
				if img.get_pixel(clampi(int(pts[k].x), 0, W - 1), clampi(int(pts[k].y), 0, H - 1)).r > 0.30:
					jct = k
					break
			if jct < RIVER_SECOND_MIN:
				continue                              # pas de confluence en AMONT (ou trop court) → bras isolé : SKIP
			_carve_path(img, pts.slice(0, jct + 1), float(rv["flow"]) / fmax, W, H)
			drew += 1
		_carve_to_sea(img, tpts[tpts.size() - 1], water, height, W, H)   # le TRONC se jette dans la mer
		n += 1
	return img

## TRONC du système = le brin le plus LONG (le cours complet, qui remonte le plus loin vers la
## MONTAGNE) ; à longueur ~égale (copies braidées), on préfère la SOURCE la plus HAUTE. Tout le reste
## CONFLUE vers ce tronc → la rivière vient bien de la montagne, jamais de bras isolé.
func _pick_trunk(strands: Array, height: Image, W: int, H: int) -> Dictionary:
	var best := {}
	var best_score := -1.0
	for rv in strands:
		var pts: PackedVector2Array = rv["points"]
		if pts.size() < RIVER_MIN_PTS:
			continue
		var score := float(pts.size())               # longueur = critère principal (le cours complet)
		if height != null:                            # +tiebreak : la source la plus HAUTE (vers la montagne)
			var src: Vector2 = pts[0]
			score += height.get_pixel(clampi(int(src.x), 0, W - 1), clampi(int(src.y), 0, H - 1)).r * 3.0
		if score > best_score:
			best_score = score
			best = rv
	return best

## largeur du PINCEAU pour un débit relatif : [peak, core, soft]. FIN : cœur d'1 cellule au plus fort
## seulement, 0 sinon ; halo dégradé de +2 = la berge fondue (le « cut » latéral). Affluent = tout fondu.
func _brush_for(rel: float) -> Array:
	var peak := clampf(0.52 + 0.55 * rel, 0.0, 1.07)   # le DÉBIT module la largeur via le seuil du shader
	return [peak, 0, 3]                                # cœur d'1 cellule (core 0) + halo de 3 → place pour la BERGE de sable

## grave un brin dans le champ : à chaque point, un PINCEAU DOUX. Le RECOUVREMENT le long du fil garde
## le cœur CONTINU ; le halo dégradé FOND la berge latéralement. Le tiers AVAL s'ÉVASE vers la mer
## (cœur qui grossit) → le neck d'embouchure n'est plus étranglé par le sable (vraie largeur d'estuaire).
func _carve_path(img: Image, pts: PackedVector2Array, rel: float, W: int, H: int) -> void:
	var b := _brush_for(rel)
	var n := pts.size()
	for i in range(n):
		var core: int = b[1]
		var soft: int = b[2]
		var frac := float(i) / float(maxi(1, n - 1))      # 0 = source, 1 = embouchure
		if frac > 0.55:                                   # tiers AVAL → évasement progressif vers la mer
			var add := int(round((frac - 0.55) / 0.45 * 2.0))   # +0 … +2 cellules de cœur
			core += add
			soft = core + 2
		_carve_brush(img, int(pts[i].x), int(pts[i].y), b[0], core, soft, W, H)

## un coup de pinceau en (cx,cy) : cœur PLEIN (dist ≤ core) puis dégradé LINÉAIRE jusqu'à 0 au halo.
func _carve_brush(img: Image, cx: int, cy: int, peak: float, core: int, soft: int, W: int, H: int) -> void:
	for dy in range(-soft, soft + 1):
		for dx in range(-soft, soft + 1):
			var d2 := dx * dx + dy * dy
			if d2 > soft * soft:
				continue
			var x := cx + dx
			var y := cy + dy
			if x < 0 or y < 0 or x >= W or y >= H:
				continue
			var dist := sqrt(float(d2))
			var fall := 1.0 if dist <= float(core) else clampf((float(soft) - dist) / float(maxi(1, soft - core)), 0.0, 1.0)
			var v := peak * fall
			if img.get_pixel(x, y).r < v:              # garde le MAX (troncs/recouvrements priment)
				img.set_pixel(x, y, Color(v, 0.0, 0.0))

## PROLONGE l'aval du tronc jusqu'à TOUCHER la mer en SUIVANT LA PENTE (l'eau coule vers le bas) :
## la worldgen arrête souvent le fil dans le bas-pays côtier, à 15-25 cellules de l'eau ouverte — on
## descend donc cellule par cellule (voisin le plus BAS) en gravant un pont d'eau SOLIDE jusqu'à
## jouxter l'eau. Repli LIGNE DROITE vers la mer la plus proche si on tombe dans une cuvette plate.
func _carve_to_sea(img: Image, mouth: Vector2, water: Image, height: Image, W: int, H: int) -> void:
	if water == null:
		return
	var px := int(mouth.x)
	var py := int(mouth.y)
	for _step in range(90):
		_carve_brush(img, px, py, 1.07, 3, 5, W, H)        # pont d'eau SOLIDE LARGE → rejoint franchement la mer
		if _touches_sea(water, px, py, W, H):
			_carve_brush(img, px, py, 1.10, ESTUARY_CORE, ESTUARY_CORE + 2, W, H)   # ESTUAIRE : large embouchure
			return                                          # jouxte l'eau → REJOINT la mer (accès NON bouché par le sable)
		# descendre vers le voisin le PLUS BAS (le sens du courant)
		var bx := px
		var by := py
		var bh := 2.0
		if height != null:
			bh = height.get_pixel(px, py).r
		for dy in range(-1, 2):
			for dx in range(-1, 2):
				if dx == 0 and dy == 0:
					continue
				var nx := px + dx
				var ny := py + dy
				if nx < 0 or ny < 0 or nx >= W or ny >= H:
					continue
				var h := height.get_pixel(nx, ny).r if height != null else 1.0
				if h < bh:
					bh = h
					bx = nx
					by = ny
		if bx == px and by == py:
			# CUVETTE plate : on vise la mer la plus proche — mais en MÉANDRE (jamais une droite stricte)
			var s := _nearest_sea(water, px, py, 55, W, H)
			if s.x < 0:
				return
			_carve_meander(img, Vector2(px, py), s, W, H)
			return
		px = bx
		py = by

## grave une LIGNE MÉANDREUSE de a→b (JAMAIS une droite stricte) : décalage perpendiculaire
## sinusoïdal d'amplitude/fréquence/phase VARIABLES (dérivées de a → déterministe mais non répétitif) ;
## enveloppe sin(πt) → 0 aux deux bouts (raccord net au fil et à la mer) ; deux harmoniques
## incommensurables → aucun « pattern » de sinus visible. C'est l'embouchure qui serpente vers la mer.
func _carve_meander(img: Image, a: Vector2, b: Vector2, W: int, H: int) -> void:
	var dist := a.distance_to(b)
	var dir := (b - a) / maxf(dist, 0.001)
	var perp := Vector2(-dir.y, dir.x)
	var sd := (int(a.x) * 73856093) ^ (int(a.y) * 19349663)
	var amp := dist * (0.07 + 0.09 * float(sd & 0xff) / 255.0)        # 7-16 % de la longueur (variable)
	var freq := 1.4 + float((sd >> 8) & 0x7) * 0.5                    # ~1.4 … 4.9 ondulations (variable)
	var phase := float((sd >> 12) & 0xff) / 255.0 * TAU              # déphasage (variable)
	var steps := maxi(2, int(dist))
	for i in range(steps + 1):
		var t := float(i) / float(steps)
		var env := sin(t * PI)                                       # 0 aux extrémités
		var wig := sin(t * freq * TAU + phase) + 0.35 * sin(t * freq * 2.7 * TAU + phase * 1.7)
		var q := a.lerp(b, t) + perp * (amp * env * wig)
		_carve_brush(img, int(q.x), int(q.y), 1.07, 3, 5, W, H)
	_carve_brush(img, int(b.x), int(b.y), 1.10, ESTUARY_CORE, ESTUARY_CORE + 2, W, H)   # ESTUAIRE au débouché

## VRAI si une cellule d'eau (mer/lac) JOUXTE (px,py) — la rivière est alors connectée.
func _touches_sea(water: Image, px: int, py: int, W: int, H: int) -> bool:
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			var nx := px + dx
			var ny := py + dy
			if nx >= 0 and ny >= 0 and nx < W and ny < H and water.get_pixel(nx, ny).r > 0.5:
				return true
	return false

## cellule d'eau la plus proche (anneaux croissants ≤ maxr) ; (-1,-1) si aucune.
func _nearest_sea(water: Image, px: int, py: int, maxr: int, W: int, H: int) -> Vector2:
	for R in range(1, maxr):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if absi(dx) != R and absi(dy) != R:
					continue
				var nx := px + dx
				var ny := py + dy
				if nx >= 0 and ny >= 0 and nx < W and ny < H and water.get_pixel(nx, ny).r > 0.5:
					return Vector2(nx, ny)
	return Vector2(-1, -1)

