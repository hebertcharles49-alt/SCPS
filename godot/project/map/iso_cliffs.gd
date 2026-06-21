extends Node2D
## Couche FALAISES — escarpements (sprites) dessinés SANS le shader splat. Raison : un shader canvas
## avec un `sampler2DArray` (le splat) ne lie PAS la TEXTURE intégrée pour `draw_texture_rect` en GL
## Compatibility → les sprites de falaise étaient INVISIBLES. Ici, aucun material → la texture se lie
## normalement. Enfant d'IsoGround (dessiné par-dessus le sol, sous l'overlay des villes).
##
## Logique : à chaque rupture de relief (forte DESCENTE vers un tuile-voisin), on pose un escarpement
## ORIENTÉ vers le bas (miroir selon la direction écran) et ÉLARGI → recouvre ses voisins = mur continu.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const TILE_K := 8
const CLIFF_GRAD := 46          ## descente (/255) vers un voisin = vraie MARCHE (rupture franche)
const CLIFF_CONVEX := 10        ## convexité min (/255) : tuile au-dessus de la MOYENNE des voisins →
                                ## la LÈVRE HAUTE d'une marche, pas un flanc de pente lisse (anti-blob)
const CLIFF_TILES := 2.2        ## HAUTEUR de l'escarpement EN TUILES (~2 — l'objectif de design)
const CLIFF_WIDEN := 1.18       ## léger élargissement horizontal → les escarpements se touchent = mur

var _cliffs := {}
var _cliffs_loaded := false

func _ready() -> void:
	name = "IsoCliffs"
	texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	# OCCLUSION : la roche passe PAR-DESSUS routes & bâtiments (l'Overlay, z=0) → ce qui est sur une
	# tuile-falaise DISPARAÎT sous l'escarpement (la passabilité reste un sujet moteur, pas ici).
	z_index = 50
	z_as_relative = false
	Sim.generated.connect(queue_redraw)
	Sim.ticked.connect(_on_tick)

func _on_tick(_y: int) -> void:
	var w = Sim.world
	if w != null and int((w.endgame_info() as Dictionary).get("fin", 0)) > 0:
		queue_redraw()

func _mapview() -> Node2D:
	var p := get_parent()
	return p.get_parent() as Node2D if p != null else null   # IsoGround → MapView

func _draw() -> void:
	if not UIKit.has_iso_tiles():
		return
	var w = Sim.world
	if w == null:
		return
	var mv := _mapview()
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
				continue
			var cf := _cliff_info(hgt, cx, cy, W, H)   # -1 / 0 (face droite) / 1 (face gauche, miroir)
			if cf >= 0:
				var ip: Vector2 = mv.iso_pos(float(cx), float(cy))
				_draw_cliff(b, ip, (col * 73856093) ^ (row * 19349663), cf)

## plus forte DESCENTE vers un tuile-voisin (la falaise regarde le BAS). -1 si trop douce, 0 si le
## bas est à l'écran-DROITE (+x/-y), 1 si à l'écran-GAUCHE (-x/+y → sprite MIROIR).
func _cliff_info(hgt: Image, x: int, y: int, W: int, H: int) -> int:
	var h0 := hgt.get_pixel(x, y).r
	var hpx := hgt.get_pixel(mini(x + TILE_K, W - 1), y).r
	var hny := hgt.get_pixel(x, maxi(y - TILE_K, 0)).r
	var hnx := hgt.get_pixel(maxi(x - TILE_K, 0), y).r
	var hpy := hgt.get_pixel(x, mini(y + TILE_K, H - 1)).r
	var drops := [h0 - hpx, h0 - hny, h0 - hnx, h0 - hpy]    # +x(SE)/-y(NE) droite · -x(NW)/+y(SW) gauche
	var bi := 0
	for i in range(1, 4):
		if drops[i] > drops[bi]:
			bi = i
	if drops[bi] * 255.0 < float(CLIFF_GRAD):
		return -1
	# CONVEXITÉ : on ne pose la falaise que sur la LÈVRE HAUTE (tuile nettement au-dessus de la
	# moyenne de ses voisins). Un flanc de pente LISSE a h0 ≈ moyenne → écarté → plus de blob.
	if (h0 - 0.25 * (hpx + hnx + hpy + hny)) * 255.0 < float(CLIFF_CONVEX):
		return -1
	return 0 if bi < 2 else 1

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

func _draw_cliff(biome: int, ip: Vector2, vh: int, flip: int) -> void:
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
	var ctex := UIKit.cliff_tex(c["rel"])   # texture lifted ; AUCUN material ici → se lie normalement
	if ctex == null:
		return
	var sz: Vector2 = c["size"]
	# 1 TUILE iso = TILE_K de HAUT (un pas diagonal complet = +TILE_K en y). On dimensionne par la
	# HAUTEUR (objectif ~CLIFF_TILES tuiles) → la largeur SUIT l'aspect, ×WIDEN pour la continuité.
	var csy := CLIFF_TILES * float(TILE_K) / maxf(sz.y, 1.0)
	var dw := sz.x * csy * CLIFF_WIDEN
	var dh := sz.y * csy
	# base posée SUR la tuile (centre horizontal = ip.x), le ROC monte vers le haut d'écran ; on
	# enfonce la base d'½-tuile pour qu'il couvre bien sa cellule (pas suspendu au-dessus du vide).
	var top := ip.y - dh + float(TILE_K) * 0.5
	if flip == 1:                            # MIROIR horizontal autour de ip.x (face écran-gauche)
		draw_set_transform(Vector2(ip.x, 0.0), 0.0, Vector2(-1.0, 1.0))
		draw_texture_rect(ctex, Rect2(Vector2(-dw * 0.5, top), Vector2(dw, dh)), false)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	else:
		draw_texture_rect(ctex, Rect2(Vector2(ip.x - dw * 0.5, top), Vector2(dw, dh)), false)
