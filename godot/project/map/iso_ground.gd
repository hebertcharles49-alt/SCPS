extends Node2D
## IsoGround — peint le SOL en SPLAT PAR PIXEL : UN quad couvre la carte iso ; le shader iso_blend
## lit le biome par pixel (carte des biomes) et échantillonne la texture PLATE tuilable de ce biome
## (texture-array) en ESPACE MONDE continu — frontières ONDULÉES par un bruit + fondu inter-biome
## → terrain CONTINU, zéro grille de losanges. Les cellules de FALAISE (rupture de relief) reçoivent
## un ESCARPEMENT (sprite, passe dédiée par-dessus).
##
## Display-only. Espace ISO (la Camera2D fait pan/zoom) ; rendu UNE FOIS au générate (biomes
## ~statiques post-worldgen ; re-dessin sur cataclysme §27). REPLI : pas de tuiles ⇒ ne dessine rien.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const TILE_K := 8              ## cellules monde par tuile iso (granularité du sol)
const CLIFF_GRAD := 44         ## gradient de hauteur (/255) = falaise : HAUT → seulement les vraies
                               ## ruptures (pas les pentes douces) ; là, l'escarpement large fait mur.
const CLIFF_W := 1.7           ## largeur de l'escarpement en tuiles (> 1 → recouvre les voisins = mur)
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
	["rck", "rc3", "rd2", "rd1"], ["sno", "snf", "sng", "ice"], ["ice", "sno", "snf", "sng"],
	["m20", "m25", "rm1", "m15"], ["m25", "rm2", "m15", "m20"], ["bc2", "bc3", "bc4", "bla"],
	["for", "bc4", "fo2", "fc3"],
]

var _mv: Node2D = null
var _active := false
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté
var _cliffs := {}           ## style → Array de {rel, hot:Vector2, size:Vector2} (index.json)
var _cliffs_loaded := false
var _terr_arr: Texture2DArray = null   ## texture-array des sols PLATS, indexée par BIOME
var _bmap: ImageTexture = null         ## couche biome → texture (R = biome/255) pour le splat shader

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
	texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS   # anti-moiré au dézoom
	_shaded = true

## biome au CENTRE de la tuile voisine (col,row) ; HORS-grille = océan (le monde est ceint de mer).
func _tile_biome(bio: Image, col: int, row: int, nx: int, ny: int, W: int, H: int) -> int:
	if col < 0 or row < 0 or col >= nx or row >= ny:
		return 0
	var x := mini(col * TILE_K + TILE_K / 2, W - 1)
	var y := mini(row * TILE_K + TILE_K / 2, H - 1)
	return int(bio.get_pixel(x, y).r * 255.0 + 0.5)

func _mv_ref() -> Node2D:
	if _mv == null:
		_mv = get_parent() as Node2D
	return _mv

func is_active() -> bool:
	return _active

func _on_generated() -> void:
	_active = UIKit.has_iso_tiles()
	_bmap = null            # nouveau monde → recharge la carte des biomes pour le splat
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
	var mv := _mv_ref()
	if mv == null or not mv.has_method("iso_pos"):
		return
	var bio: Image = w.layer_image(LAYER_BIOME)
	var hgt: Image = w.layer_image(LAYER_HEIGHT)
	if bio == null or hgt == null:
		return
	var W := int(w.map_w())
	var H := int(w.map_h())
	# uniformes du splat : carte des biomes (R = biome/255) + texture-array des sols plats
	var mat := material as ShaderMaterial
	if mat != null:
		if _bmap == null:
			_bmap = ImageTexture.create_from_image(bio)
		mat.set_shader_parameter("biome_map", _bmap)
		mat.set_shader_parameter("map_size", Vector2(W, H))
		mat.set_shader_parameter("terrains", _terr_array())
	# SOL = UN seul QUAD couvrant la carte iso (x∈[-H,W], y∈[0,(W+H)/2]) → splat PAR PIXEL dans le shader
	draw_rect(Rect2(-float(H), 0.0, float(W + H), float(W + H) * 0.5), Color(0.0, 0.0, 0.0, 1.0))
	# FALAISES : passe SPRITE par-dessus, en profondeur (YSort par col+row)
	var nx := W / TILE_K
	var ny := H / TILE_K
	var k2 := TILE_K / 2
	var sc := 2.0 * float(TILE_K) / float(UIKit.ISO_TILE_W)
	for d in range(nx + ny - 1):
		var c0 := maxi(0, d - (ny - 1))
		var c1 := mini(nx - 1, d)
		for col in range(c0, c1 + 1):
			var row := d - col
			var cx := mini(col * TILE_K + k2, W - 1)
			var cy := mini(row * TILE_K + k2, H - 1)
			var b := int(bio.get_pixel(cx, cy).r * 255.0 + 0.5)
			if b <= 2:
				continue
			if _is_cliff(hgt, cx, cy, W, H):
				var ip: Vector2 = mv.iso_pos(float(cx), float(cy))
				_draw_cliff(b, ip, sc, (col * 73856093) ^ (row * 19349663))

## rupture de relief : gradient de hauteur vers un 4-voisin (à TILE_K) au-dessus du seuil.
func _is_cliff(hgt: Image, x: int, y: int, W: int, H: int) -> bool:
	var h0 := hgt.get_pixel(x, y).r
	var g := absf(h0 - hgt.get_pixel(mini(x + TILE_K, W - 1), y).r)
	g = maxf(g, absf(h0 - hgt.get_pixel(maxi(x - TILE_K, 0), y).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, mini(y + TILE_K, H - 1)).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, maxi(y - TILE_K, 0)).r))
	return g * 255.0 >= float(CLIFF_GRAD)

## charge l'index des escarpements (une fois) → listes par style avec hotspot (origine monde) + taille.
func _load_cliffs() -> void:
	_cliffs_loaded = true
	for s in ["temperate", "frost", "volcanic"]:
		_cliffs[s] = []
	var path := "res://assets/scps/pack/iso_tiles/cliffs/index.json"
	if not FileAccess.file_exists(path):
		return
	var d = JSON.parse_string(FileAccess.open(path, FileAccess.READ).get_as_text())
	if typeof(d) != TYPE_DICTIONARY:
		return
	for e in d.get("entries", []):
		var st: String = e.get("style", "temperate")
		if not _cliffs.has(st):
			_cliffs[st] = []
		_cliffs[st].append({
			"rel": String(e["output"]),
			"hot": Vector2(e["hotspot"][0], e["hotspot"][1]),
			"size": Vector2(e["size"][0], e["size"][1])})

## escarpement isométrique à la cellule de falaise : sprite ancré par hotspot, largeur ≈ 1.4 tuile,
## style choisi par biome (neige→frost, volcan→volcanic, sinon temperate), variante par hash.
func _draw_cliff(biome: int, ip: Vector2, sc: float, vh: int) -> void:
	if not _cliffs_loaded:
		_load_cliffs()
	var st := "temperate"
	if biome == 19 or biome == 20:
		st = "frost"
	elif biome == 23:
		st = "volcanic"
	var lst: Array = _cliffs.get(st, [])
	if lst.is_empty():
		return
	var c: Dictionary = lst[abs(vh) % lst.size()]
	var ctex := UIKit.cliff_tex(c["rel"])
	if ctex == null:
		return
	var sz: Vector2 = c["size"]
	var cs := CLIFF_W * float(UIKit.ISO_TILE_W) * sc / maxf(sz.x, 1.0)   # largeur normalisée (recouvre = mur)
	# COLOR.b=0.5 → passe SPRITE du shader (UV local, hors splat → le sprite n'est PAS tuilé)
	draw_texture_rect(ctex, Rect2(ip - (c["hot"] as Vector2) * cs, sz * cs), false, Color(0.0, 1.0, 0.5, 1.0))
