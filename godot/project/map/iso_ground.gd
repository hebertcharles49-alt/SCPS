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

var _active := false
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté
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
	add_child(preload("res://map/iso_cliffs.gd").new())   # couche FALAISES (sprites, hors shader splat)
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
	# FALAISES : dessinées par la couche enfant `iso_cliffs` (sprites, hors shader → la TEXTURE se lie)

