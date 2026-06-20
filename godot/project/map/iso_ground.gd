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
const LAND_ALPHA := 1.0        ## le masque de fondu (baké dans l'alpha) adoucit déjà les bords
const CLIFF_GRAD := 40         ## gradient de hauteur (/255) = falaise (monde bimodal, vérifié)

var _mv: Node2D = null
var _active := false

func _ready() -> void:
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_tick)
	_active = UIKit.has_iso_tiles()
	queue_redraw()

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
			if b <= 2:
				continue                       # mer (deep/ocean/shallow) : le blend procédural la fait
			var tex := UIKit.biome_tile(b)     # tuile AoE2 (texture × masque de fondu) ; coast=sable
			if tex == null:
				continue
			var tint := Color(1, 1, 1, LAND_ALPHA)
			if _is_cliff(hgt, cx, cy, W, H):   # falaise = barrière inhabitable → assombrie
				tint = Color(0.34, 0.27, 0.24, 0.92)
			var ip: Vector2 = mv.iso_pos(float(cx), float(cy))
			draw_set_transform(ip, 0.0, Vector2(sc, sc))
			draw_texture(tex, -half, tint)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

## rupture de relief : gradient de hauteur vers un 4-voisin (à TILE_K) au-dessus du seuil.
func _is_cliff(hgt: Image, x: int, y: int, W: int, H: int) -> bool:
	var h0 := hgt.get_pixel(x, y).r
	var g := absf(h0 - hgt.get_pixel(mini(x + TILE_K, W - 1), y).r)
	g = maxf(g, absf(h0 - hgt.get_pixel(maxi(x - TILE_K, 0), y).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, mini(y + TILE_K, H - 1)).r))
	g = maxf(g, absf(h0 - hgt.get_pixel(x, maxi(y - TILE_K, 0)).r))
	return g * 255.0 >= float(CLIFF_GRAD)
