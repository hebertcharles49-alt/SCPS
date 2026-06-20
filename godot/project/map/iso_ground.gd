extends Node2D
## IsoGround — peint le SOL en tuiles canevas « super_biomes » PAR-DESSUS le blend procédural du
## MapView (cf. godot/ASSETS_ISO.md §3). Pas de « système climatique » : les tuiles sont des
## VARIATIONS de terrain, fusionnées (alpha) sur le blend biome-couleur — le blend derrière dissout
## le clash entre tuiles voisines (sinon : bruit). L'EAU reste au procédural (le blend FAIT la mer
## + les côtes). Les cellules de FALAISE (rupture de relief) reçoivent un ESCARPEMENT (sprite iso
## ancré par hotspot, style par biome) = barrière de relief (façon Age of Empires).
##
## Display-only. Dessiné en espace ISO (la Camera2D fait pan/zoom) ; rendu UNE FOIS au générate
## (biomes ~statiques post-worldgen ; re-dessin sur cataclysme §27). YSort par profondeur (col+row).
## REPLI : pas d'atlas de sol (UIKit.has_iso_tiles) ⇒ ne dessine RIEN → le blend procédural seul.

const UIKit = preload("res://ui/uikit.gd")
const LAYER_HEIGHT := 0
const LAYER_BIOME := 2
const TILE_K := 8              ## cellules monde par tuile iso (granularité du sol)
const CLIFF_GRAD := 40         ## gradient de hauteur (/255) = falaise (monde bimodal, vérifié)
## PRIORITÉ de fondu par biome (index = enum Biome) : le PLUS HAUT déborde sur le plus bas (façon
## Age of Empires). HIÉRARCHIE : l'HERBE est le SOCLE BAS sur lequel tout déborde ; au-dessus
## viennent forêt < aride/désert < sable/plage < relief < neige < volcan/ronces. L'eau tout en bas
## (la terre déborde sur la mer = rivage depuis la terre).
const PRIO := [0, 0, 1, 16, 4, 5, 4, 6, 6, 12, 14, 13, 8, 8, 9, 7,
	17, 10, 19, 22, 24, 7, 8, 26, 28]

var _mv: Node2D = null
var _active := false
var _shaded := false        ## le shader de fondu directionnel (rivage) est monté
var _cliffs := {}           ## style → Array de {rel, hot:Vector2, size:Vector2} (index.json)
var _cliffs_loaded := false

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
			var tex := UIKit.biome_variant(b, 0)
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
			var is_cliff := b > 2 and _is_cliff(hgt, cx, cy, W, H)   # rupture de relief → escarpement posé
			draw_texture_rect(tex, rect, false, Color(0.0, 1.0, 0.0, 1.0))   # luminance déjà NORMALISÉE (cuite)
			# BLEND : « dominance par biome » = la BASE (UNE texture par biome → même biome SANS COUTURE,
			# la grille disparaît) ; le blend de priorité se colle PAR-DESSUS sur les arêtes inter-biomes
			# (le voisin ≥ prioritaire mord ici, façon AoE). La variété vient d'un bruit CONTINU (shader),
			# pas de variantes par cellule (c'était ça, la grille).
			var pb: int = PRIO[b] if b < PRIO.size() else 0
			# blend MUTUEL : toute arête vers un AUTRE biome se fond (les deux côtés) → les couples de
			# biomes entrelacés ne laissent plus de losanges francs. La PRIORITÉ joue par l'ORDRE de
			# tracé : on pose d'ABORD les voisins dominés, le dominant PAR-DESSUS (il garde le dessus).
			var order := []
			for k in range(4):
				var n: int = nb[k]
				if n < 0 or n == b:
					continue
				var sb: int = n                       # biome qui DÉBORDE sur cette arête
				if (b <= 2) != (n <= 2):              # frontière TERRE↔MER → SHORELINE
					sb = 3                            # terre > SABLE > mer : le sable des DEUX côtés du bord
				order.append([PRIO[sb] if sb < PRIO.size() else 0, k, sb])
			order.sort_custom(func(a, c): return a[0] < c[0])   # priorité croissante → dominant en dernier
			for e in order:
				var ntex := UIKit.biome_variant(int(e[2]), 0)
				if ntex != null:
					draw_texture_rect(ntex, rect, false, Color(float(1 << int(e[1])) / 15.0, 1.0, 1.0, 1.0))
			if is_cliff:
				_draw_cliff(b, ip, sc, vh)   # escarpement par-dessus (profondeur OK : on est dans la passe YSort)

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
	var cs := 1.4 * float(UIKit.ISO_TILE_W) * sc / maxf(sz.x, 1.0)   # normalise la largeur (~1.4 tuile)
	# COLOR=(0,1,0,1) → passe BASE du shader (cfg=0 ⇒ pas de fondu, alpha du sprite intact)
	draw_texture_rect(ctex, Rect2(ip - (c["hot"] as Vector2) * cs, sz * cs), false, Color(0.0, 1.0, 0.0, 1.0))
