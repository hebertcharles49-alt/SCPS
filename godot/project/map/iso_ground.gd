extends Node2D
## IsoGround — COUVRE LE MONDE en tuiles iso (cf. godot/ASSETS_ISO.md). Display-only.
##
## Dessiné en espace ISO (la Camera2D du MapView fait pan/zoom) ; rendu UNE FOIS au générate — les
## biomes sont quasi statiques post-worldgen (seul l'endgame §27 les mute → re-dessin alors). Tri
## YSort par profondeur (col+row), arrière→avant. La tuile dominante d'une case est échantillonnée
## au centre ; pour l'instant on pose la tuile PLEINE (masque 15) — l'autotiling dual-grid (les 15
## autres masques, empilés par priorité de biome) se branche dès que les atlas de transition sont là.
##
## REPLI : si aucune tuile iso n'est présente (UIKit.has_iso_tiles) → on ne dessine RIEN ; le sol
## PROCÉDURAL du MapView (terre lissée + côte par pixel) reste. Bascule automatique à la pose des assets.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_BIOME := 2
const TILE_K := 8       ## cellules monde par tuile iso (granularité du sol)

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
	if bio == null:
		return
	var W := int(w.map_w())
	var H := int(w.map_h())
	var nx := W / TILE_K
	var ny := H / TILE_K
	var sc := 2.0 * float(TILE_K) / float(UIKit.ISO_TILE_W)        # 256 px → 2·TILE_K unités iso
	var half := Vector2(UIKit.ISO_TILE_W, UIKit.ISO_TILE_H) * 0.5
	# YSort : profondeur d = col+row croissante → arrière (nord) vers avant (sud)
	for d in range(nx + ny - 1):
		var c0 := maxi(0, d - (ny - 1))
		var c1 := mini(nx - 1, d)
		for col in range(c0, c1 + 1):
			var row := d - col
			var cxw := col * TILE_K + TILE_K / 2
			var cyw := row * TILE_K + TILE_K / 2
			var b := int(bio.get_pixel(mini(cxw, W - 1), mini(cyw, H - 1)).r * 255.0 + 0.5)
			var tex := UIKit.iso_tile(b, 15)                      # tuile PLEINE (base)
			if tex == null:
				continue
			var ip: Vector2 = mv.iso_pos(float(cxw), float(cyw))
			draw_set_transform(ip, 0.0, Vector2(sc, sc))
			draw_texture(tex, -half)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
