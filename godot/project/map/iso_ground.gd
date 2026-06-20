extends Node2D
## IsoGround — peint le SOL en tuiles canevas « super_biomes » PAR-DESSUS le blend procédural du
## MapView (cf. godot/ASSETS_ISO.md §3). Pas de « système climatique » : les tuiles sont des
## VARIATIONS de terrain, fusionnées (alpha) sur le blend biome-couleur — le blend derrière dissout
## le clash entre tuiles voisines (sinon : bruit). L'EAU reste au procédural (le blend FAIT la mer
## + les côtes). Les cellules de FALAISE (rupture de relief) sont assombries = barrière inhabitable
## (façon Age of Empires).
##
## Display-only. Dessiné en espace ISO (la Camera2D fait pan/zoom) ; rendu UNE FOIS au générate
## (biomes ~statiques post-worldgen ; re-dessin sur cataclysme §27). YSort par profondeur (col+row).
## REPLI : pas d'atlas de sol (UIKit.has_iso_tiles) ⇒ ne dessine RIEN → le blend procédural seul.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const TILE_K := 8              ## cellules monde par tuile iso (granularité du sol)
const CLIFF_GRAD := 40         ## gradient de hauteur (/255) = falaise (monde bimodal, vérifié)
## PRIORITÉ de fondu par biome (index = enum Biome) : le PLUS HAUT déborde sur le plus bas.
## Eau basse · sable · herbe · aride · forêt · relief · neige · roche nue. Réglage maison.
const PRIO := [0, 0, 0, 5, 8, 9, 8, 6, 6, 11, 12, 11, 16, 16, 17, 10,
	17, 15, 20, 22, 24, 13, 13, 25, 26]

var _mv: Node2D = null
var _active := false
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté

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
	var nx := W / TILE_K
	var ny := H / TILE_K
	var sc := 2.0 * float(TILE_K) / float(UIKit.ISO_TILE_W)
	var half := Vector2(UIKit.ISO_TILE_W, UIKit.ISO_TILE_H) * 0.5
	var k2 := TILE_K / 2
	for d in range(nx + ny - 1):
		var c0 := maxi(0, d - (ny - 1))
		var c1 := mini(nx - 1, d)
		for col in range(c0, c1 + 1):
			var row := d - col
			var cx := mini(col * TILE_K + k2, W - 1)
			var cy := mini(row * TILE_K + k2, H - 1)
			var b := int(bio.get_pixel(cx, cy).r * 255.0 + 0.5)
			var vh := (col * 73856093) ^ (row * 19349663)   # hash → variante (variété par cellule)
			var tex := UIKit.biome_variant(b, vh)
			if tex == null:
				continue
			# placement ABSOLU (pas de draw_set_transform) → VERTEX = iso continu pour le bruit du shader
			var ip: Vector2 = mv.iso_pos(float(cx), float(cy))
			var rect := Rect2(ip - half * sc, half * (2.0 * sc))
			var nb := [                        # 4 voisins iso : NE · SE · SW · NW
				_tile_biome(bio, col, row - 1, nx, ny, W, H),
				_tile_biome(bio, col + 1, row, nx, ny, W, H),
				_tile_biome(bio, col, row + 1, nx, ny, W, H),
				_tile_biome(bio, col - 1, row, nx, ny, W, H)]
			# BASE pleine (EAU comprise) ; tout le bord vient du DÉBORDEMENT des voisins prioritaires
			var darken := 0.42 if (b > 2 and _is_cliff(hgt, cx, cy, W, H)) else 1.0   # falaise = barrière
			draw_texture_rect(tex, rect, false, Color(0.0, darken, 0.0, 1.0))
			# DÉBORDEMENT : voisin plus PRIORITAIRE mord sur la tuile (la TERRE déborde sur l'EAU = rivage)
			var pb: int = PRIO[b] if b < PRIO.size() else 0
			var spill := {}
			for k in range(4):
				var n: int = nb[k]
				if n >= 0 and n < PRIO.size() and PRIO[n] > pb:
					spill[n] = int(spill.get(n, 0)) | (1 << k)
			for n in spill:
				var ntex := UIKit.biome_variant(n, vh + n * 7)
				if ntex != null:
					draw_texture_rect(ntex, rect, false, Color(float(spill[n]) / 15.0, 1.0, 1.0, 1.0))

## rupture de relief : gradient de hauteur vers un 4-voisin (à TILE_K) au-dessus du seuil.
func _is_cliff(hgt: Image, x: int, y: int, W: int, H: int) -> bool:
	var h0 := hgt.get_pixel(x, y).r
	var g := absf(h0 - hgt.get_pixel(mini(x + TILE_K, W - 1), y).r)
	g = maxf(g, absf(h0 - hgt.get_pixel(maxi(x - TILE_K, 0), y).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, mini(y + TILE_K, H - 1)).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, maxi(y - TILE_K, 0)).r))
	return g * 255.0 >= float(CLIFF_GRAD)
