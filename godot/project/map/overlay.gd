extends Node2D
## Overlay — les ACTEURS sur la carte, en espace MONDE (enfant de MapView → suit
## la caméra zoom/pan automatiquement). DISPLAY-ONLY : lit la façade (region_tier,
## army_info, centroïdes), ne calcule rien. Redessine au tick (les données bougent).
##
## Villes : un disque au centroïde, dimensionné au tier (0-5), teinté au pays.
## Armées : un losange au centroïde de leur région + une ligne vers leur but
## (marche), un anneau coloré par phase (marche/siège/bataille).

const UIKit = preload("res://ui/uikit.gd")
const VKit = preload("res://ui/vkit.gd")
const PHASE_MARCH := 1
const PHASE_SIEGE := 2
const PHASE_BATTLE := 3
const LAYER_WATER := 4       ## SCPS_LAYER_WATER : masque mer OU LAC (≥1 = eau) — l'assise des
                             ## bourgs tient l'EAU COMPLÈTE (les lacs intérieurs, ignorés par SEA seul)
## Seuils de zoom ISO (px/unité-monde de la Camera2D). L'ISO est la surface de JEU : on y montre
## ROUTES & ASSETS (bourg). L'entrée en ISO est à zoom ≈ ISO_FAR (4.0) → assets déjà lisibles.
const CITY_ZOOM_MIN := 3.5   ## villes + bourg
const DECOR_ZOOM_MIN := 3.0  ## forêts/arbres
const BLD_SIZE := 9.0        ## taille MONDE UNIFORME d'un bâti de bourg (égalisée — variété par le sprite, pas l'échelle)
const CITY_CORE_SIZE := 18.0 ## taille MONDE FIXE du centre-ville (la ville ne GRANDIT pas ; l'importance = le variant T1-T7)

## OMBRE PORTÉE des assets : un disque APLATI au pied, décalé dans la direction ANTI-LUMIÈRE,
## la MÊME lumière globale que le shader de terrain (light_world ≈ (-0.95,-0.32)). L'ombre tombe
## donc à l'OPPOSÉ (monde (0.95,0.32)) → projeté iso ≈ (0.70,0.71) = bas-droite (soleil haut-gauche).
## Subtile (le « trop violent » d'avant) + tracée en 2 PASSES (toutes les ombres SOUS tous les sprites).
const SHADOW_COL := Color(0.0, 0.0, 0.0, 0.17)
const SHADOW_DIR := Vector2(0.704, 0.710)   ## direction ÉCRAN de l'ombre (anti-lumière projetée en iso)

## COHÉRENCE DE LUMIÈRE (assets & routes calés sur le terrain) — fini le « posé là » : chaque asset
## est modulé par (a) l'OMBRAGE du relief sous le MÊME soleil que iso_blend.gdshader, et (b) la
## LUMINOSITÉ/teinte du SOL sous lui. Calculé une fois (au build), display-only.
const LIGHT_WORLD := Vector2(-0.95, -0.32)   ## même direction-source que le shader de terrain
const SHADE_K := 0.22        ## amplitude de l'ombrage relief (± sur la clarté de l'asset)
const GROUND_TINT_BLD := 0.11 ## fraction de teinte du sol mêlée à un BÂTIMENT (lumière rebondie / brume)
const GROUND_TINT_DEC := 0.07 ## idem, plus discret, pour le DRESSING (arbres : ne pas mudder les verts)


var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre
var _decor := []          ## [{name, pos}] — arbres/forêts (dressing nature), bâti au générate
var _structures := []     ## [{name, pos}] — bâti de terrain parsemé autour des villes
var _bio_img: Image = null ## couche biome (cache) → interdit le PIED d'un asset sur une tuile falaise
var _region_variant := {} ## région colonisée → nom de variante de ville TERRAIN (petits bourgs)
var _region_centre := {}  ## région colonisée → TERRAIN du centre-ville (plaine/foret/montagne/estuaire/portuaire/lacustre)
var _region_anchor := {}  ## région colonisée → assise de ville CALÉE SUR TERRE (centroïde snappé + rabat côtier)
var _region_citymax := {} ## région colonisée → plus grande taille de sprite de ville TENANT au sec (anti-débord mer)
var _bk := {}             ## noms de structures triés en bancs (civic/craft/dwell/field), calculé 1×
var _clear_set := {}      ## cellules DÉBOISÉES autour des villes (le bourg respire, pas noyé sous la forêt)
var _country_names := []  ## nom de chaque pays (figé au générate) — pour les étiquettes d'empire
var _borders := {}        ## niveau (1=région · 2=pays) → PackedVector2Array de segments (façade)
var _borders_dirty := true ## la souveraineté a bougé (conquête/colonisation) → refaire les frontières
var _owner_sig := -1      ## signature de la photo des propriétaires → détecte le changement de souveraineté
var _roads := []          ## [{points, level, nprov, key}] — réseau de routes (façade + méta locale)
var _road_dress := []     ## [{name, pos, road}] — mobilier de BORDURE (apparaît à la FIN du chantier)
var _road_cells := {}     ## cellules occupées par une route (+ marge) → le bourg en SPIRALE les évite
var _main_streets := []   ## [{a, s, r}] — RUE PRINCIPALE (vers le sud/avant) de chaque bourg (façade western)
var _road_start := {}     ## clé de route → ANNÉE de début de chantier (croissance 1 an/province)
var _roads_dirty := true  ## le réseau commercial a pu bouger → recharger les routes
var _struct_dirty := false ## le bourg dépend de pop+bâtiments (évolue) → reconstruit à la demande
var _road_placed := 0     ## logements/ateliers réellement posés LE LONG des routes (le reste comble en anneau)
var _rivers := []         ## [Vector3(x, y, ang)] — nuage de points (façade) gardé pour l'anti-bâti SUR le fil
var _mv: Node2D = null    ## le MapView parent (porte la projection GLOBE monde→écran)
var _himg_l: Image = null ## couche HEIGHT (cache local) → ombrage cohérent des assets/routes
var _alb_l: Image = null  ## terrain albedo (cache) → couleur/luminosité du SOL sous l'asset

# RIVIÈRES : CARVÉES DANS LE TERRAIN (worldgen), pas un asset par-dessus. Le shader iso_blend lit
# un champ de débit (bâti par iso_ground._build_river_field depuis `river_paths`, fusionné par baie)
# et rend l'eau DANS le relief — cœur propre, berges fondues, continu jusqu'à la mer. L'overlay n'en
# garde QUE le nuage de points (`_rivers`) pour interdire de BÂTIR sur le fil.
const ROAD_ZOOM_MIN := 2.5    ## routes (zoom ISO)
const ROAD_CASING := Color(0.227, 0.165, 0.110)  ## bord sombre (viewer 58,42,28)
const ROAD_FILL   := Color(0.86, 0.74, 0.52)     ## surface parchemin CLAIRE — la route = le fil conducteur landmark↔bourg
const ROAD_SOFT   := Color(0.227, 0.165, 0.110, 0.13)  ## halo doux SOUS le casing → blend feutré (anti-alias) TIGHT
# MOBILIER de bord de route (habillage) — bornes/murets/buissons/rochers/bottes (pack dressing)
const ROADSIDE := [
	"DRESS_BUSH_LOW", "DRESS_BUSH_DENSE_GREEN", "DRESS_BUSH_DRY", "DRESS_BUSH_YELLOW",
	"DRESS_BUSH_LOW", "DRESS_BUSH_DENSE_GREEN", "DRESS_GRASS_GOLD", "DRESS_BUSH_THORNY",
	"DRESS_ROCK_GRAY_A", "DRESS_ROCK_LIGHT", "DRESS_STONE_PILE", "DRESS_ROCK_GRAY_B",
]   # surtout des BUISSONS (ils CACHENT/adoucissent le bord), quelques cailloux

# Traitement FRONT-END du tracé (l'A* moteur reste la vérité ; on en lisse la SORTIE, hors tick) :
#  · SNAP : raccord d'extrémité PROPRE (trim des points qui tanglent près de l'ancre de ville) ;
#  · PATHFINDING (rendu) : ré-échantillonnage à PAS CONSTANT + Chaikin GARDÉ-EAU (courbe nette qui
#    épouse la côte sans la couper) ;
#  · ASSETS : mobilier semé à l'ARC (espacement RÉGULIER, indépendant de la densité de points).
const ROAD_RESAMPLE := 2.0       ## pas d'échantillonnage du tracé (cellules) → points réguliers
const ROAD_SNAP_TRIM := 4.5      ## rayon de nettoyage des points près de l'ancre de ville (cellules)
const ROAD_DRESS_OFF := 0.95     ## décalage SUD de base du mobilier (marge)
const MAIN_ST_LEN := 9.0         ## longueur de la RUE PRINCIPALE (vers le sud/avant) — la façade western

# biome (couche, valeurs Biome) → NOMS de sprites dressing. FOREST=12 · WOODS=13 · JUNGLE=14
const FOREST_TREES := {
	12: ["DRESS_TREE_OAK_SUMMER", "DRESS_TREE_OAK_GOLD", "DRESS_GROVE_BROADLEAF", "DRESS_TREE_POPLAR"],  # FOREST : feuillus
	13: ["DRESS_GROVE_MIXED", "DRESS_TREE_PINE", "DRESS_GROVE_PINE_A", "DRESS_GROVE_PINE_B"],            # WOODS : mixte/conifères
	14: ["DRESS_GROVE_BROADLEAF", "DRESS_TREE_GNARLED", "DRESS_GROVE_MIXED"],                            # JUNGLE : dense
}
# ── DRESSING du monde par MILIEU (cailloux, buissons, arbres sporadiques, roseaux…) — habille la
#    carte hors des forêts. Pools tirés au hasard, semés SPORADIQUEMENT (cf. _dress_rule). ──
const DRESS_OPEN := ["DRESS_TREE_OAK_SUMMER", "DRESS_TREE_OAK_GOLD", "DRESS_TREE_OAK_AUTUMN", "DRESS_TREE_POPLAR",
	"DRESS_GROVE_BROADLEAF", "DRESS_GROVE_MIXED", "DRESS_TREE_PINE", "DRESS_BUSH_LOW", "DRESS_BUSH_DENSE_GREEN"]  # surtout des ARBRES, peu de buissons
const DRESS_DRY := ["DRESS_BUSH_DRY", "DRESS_BUSH_YELLOW", "DRESS_BUSH_THORNY", "DRESS_TREE_GNARLED", "DRESS_ROCK_LIGHT", "DRESS_GRASS_GOLD", "DRESS_DIRT_MOUND"]
# MARAIS : roselière DENSE — roseaux (verts/secs/en eau), massettes, herbe mouillée, fougères, mousse,
# quelques snags morts/tordus, peu de cailloux. Couvre la vasière brune.
const DRESS_MARSH := ["DRESS_REEDS_GREEN", "DRESS_REEDS_DRY", "DRESS_REEDS_WATER", "DRESS_GRASS_CATTAIL",
	"DRESS_GRASS_WET", "DRESS_FERNS", "DRESS_REEDS_GREEN", "DRESS_MUSHROOMS", "DRESS_MOSS_PATCH",
	"DRESS_TREE_DEAD", "DRESS_TREE_GNARLED", "DRESS_ROOT_CLUSTER", "DRESS_BUSH_LOW"]
# MANGROVE (côte tropicale ennoyée) : THICKET de palétuviers — arbres tordus + amas de racines en eau,
# roseaux d'eau, fougères, herbe mouillée. Dense.
const DRESS_MANGROVE := ["DRESS_TREE_GNARLED", "DRESS_ROOT_CLUSTER", "DRESS_REEDS_WATER", "DRESS_REEDS_GREEN",
	"DRESS_TREE_GNARLED", "DRESS_FERNS", "DRESS_GRASS_WET", "DRESS_ROOT_CLUSTER", "DRESS_MOSS_PATCH", "DRESS_TREE_DEAD"]
const DRESS_HILL := ["DRESS_ROCK_GRAY_A", "DRESS_ROCK_GRAY_B", "DRESS_ROCK_LIGHT", "DRESS_ROCK_MOSSY", "DRESS_BUSH_LOW", "DRESS_BUSH_DRY", "DRESS_TREE_PINE", "DRESS_STONE_PILE"]
const DRESS_CLIFF := ["DRESS_ROCK_GRAY_A", "DRESS_ROCK_GRAY_B", "DRESS_ROCK_DARK", "DRESS_ROCK_SPIRES", "DRESS_STONE_PILE", "DRESS_STONE_SLABS", "DRESS_BUSH_DRY", "DRESS_BUSH_THORNY", "DRESS_BUSH_LOW"]
const DRESS_SNOW := ["DRESS_TREE_PINE_SNOW", "DRESS_ROCK_SNOW", "DRESS_BRANCH_SNOW", "DRESS_ROCK_GRAY_B"]
const DRESS_RIVER := ["DRESS_ROCK_GRAY_A", "DRESS_ROCK_GRAY_B", "DRESS_ROCK_LIGHT", "DRESS_ROCK_MOSSY", "DRESS_BUSH_LOW", "DRESS_BUSH_DENSE_GREEN", "DRESS_REEDS_GREEN", "DRESS_PEBBLE_PATH", "DRESS_GRASS_CATTAIL"]
# STEPPE/SAVANE (brun, sec) : couvrir de TOUFFES d'herbe dorée (ground cover, pas des cailloux),
# quelques buissons secs, rare arbre tordu — densité haute mais BAS (l'herbe ne fait pas de clutter).
const DRESS_STEPPE := ["DRESS_GRASS_GOLD", "DRESS_GRASS_GOLD", "DRESS_GRASS_GOLD", "DRESS_GRASS_GOLD",
	"DRESS_GRASS_CATTAIL", "DRESS_GRASS_GOLD", "DRESS_BUSH_DRY", "DRESS_BUSH_YELLOW", "DRESS_BUSH_LOW", "DRESS_TREE_GNARLED"]
# TOURBIÈRE/LANDE (brun humide) : mousse, roseaux, herbe mouillée, snags morts — dense (terre couverte), peu de cailloux
const DRESS_BOG := ["DRESS_MOSS_PATCH", "DRESS_GRASS_WET", "DRESS_REEDS_DRY", "DRESS_BUSH_LOW",
	"DRESS_TREE_DEAD", "DRESS_REEDS_GREEN", "DRESS_MUSHROOMS", "DRESS_TREE_GNARLED", "DRESS_ROOT_CLUSTER",
	"DRESS_FERNS", "DRESS_GRASS_CATTAIL", "DRESS_MOSS_PATCH"]

# biome → variante de ville TERRAIN (CITY_BIOME_*). SEULEMENT les terrains
# DISTINCTIFS (côte/forêt/marais/montagne/neige…) ; les plaines génériques (4-10)
# gardent le sprite par BANDE DE POP (qui, lui, gradue la taille avec la pop).
const BIOME_CITY := {
	3: "CITY_BIOME_COAST_FISHING", 11: "CITY_BIOME_COAST_FISHING",         # côtes
	12: "CITY_BIOME_FOREST_HAMLET", 13: "CITY_BIOME_FOREST_HAMLET", 14: "CITY_BIOME_FOREST_HAMLET",  # forêts
	15: "CITY_BIOME_MARSH_STILTS", 21: "CITY_BIOME_MARSH_STILTS", 22: "CITY_BIOME_MARSH_STILTS",     # humides
	16: "CITY_BIOME_PINE_HIGHLAND", 17: "CITY_BIOME_DRY_UPLAND",           # hauteurs/collines
	18: "CITY_BIOME_MOUNTAIN_TERRACE", 19: "CITY_BIOME_CLIFFSIDE",         # montagnes
	20: "CITY_BIOME_SNOW_HAMLET",                                          # glacier
}

func _ready() -> void:
	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(_on_generated)
	if Sim.world != null:
		_set_rivers()
		_build_names()
		_build_anchors()
		_ensure_roads(Sim.world.year() > 0)   # monde mûr (save chargée) ⇒ routes déjà bâties
		_build_decor()
		_build_structures()
		_build_city_skins()
	queue_redraw()

func _build_names() -> void:
	_country_names.clear()
	var w = Sim.world
	if w == null:
		return
	for c in range(w.country_count()):
		var info: Dictionary = w.country_info(c)
		_country_names.append(String(info.get("nom", "")))

func _on_generated() -> void:
	_set_rivers()
	_himg_l = null              # monde neuf → recharger les caches de lumière (relief + albedo)
	_alb_l = null
	_borders_dirty = true       # monde neuf → frontières ET routes à refaire
	_roads_dirty = true
	_road_start.clear()         # chantiers remis à zéro (le monde neuf rebâtit ses routes)
	_owner_sig = -1
	_build_names()
	_build_anchors()
	_ensure_roads(Sim.world.year() > 0)   # an 0 (monde neuf) ⇒ croît ; an N (save/monde mûr) ⇒ déjà bâtie
	_build_decor()
	_build_structures()         # le bourg en spirale ÉVITE les routes
	_build_city_skins()
	queue_redraw()

## lit le nuage de points (anti-bâti) PUIS sélectionne les fleuves MAJEURS (tracé en ruban).
## Calculé 1× au générate, comme le reste du fil.
func _set_rivers() -> void:
	_rivers = Sim.world.river_points()    # gardé : _build_structures évite de bâtir SUR le fil

## pré-calcule la variante de ville TERRAIN de chaque région colonisée (échantillon
## du biome au centroïde ; l'hydro via le groupe de settlement) — pour les petits bourgs.
func _build_city_skins() -> void:
	_region_variant.clear()
	_region_centre.clear()
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(2)   # SCPS_LAYER_BIOME
	var sea: Image = w.layer_image(LAYER_WATER)   # mer OU lac
	for r in range(w.region_count()):
		if w.region_tier(r) < 0:
			continue
		var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))   # assise CALÉE SUR TERRE
		if ctr.x < 0:
			continue
		var nm := ""
		var sg: int = w.region_settle_group(r)
		var b := -1
		if bio != null and ctr.x < bio.get_width() and ctr.y < bio.get_height():
			b = int(bio.get_pixel(int(ctr.x), int(ctr.y)).r * 255.0 + 0.5)
		if sg == 2:                       # estuaire
			nm = "CITY_BIOME_ESTUARY_STILTS"
		elif sg == 1:                     # rivière
			nm = "CITY_BIOME_RIVERBANK_QUAY"
		elif b >= 0:
			nm = BIOME_CITY.get(b, "")
		var coastal := not _footprint_clear(sea, {}, ctr, 4.5, 9.0)
		# si l'assise jouxte l'eau et qu'aucune variante AQUATIQUE n'a été choisie,
		# bascule sur le village de PÊCHE (sprite à quais) → une ville au bord de l'eau
		# lit toujours comme un PORT voulu, jamais comme un bâti générique débordant.
		if nm == "" and coastal:
			nm = "CITY_BIOME_COAST_FISHING"
		if nm != "":
			_region_variant[r] = nm
		# TERRAIN du CENTRE-VILLE (pack centres) : hydro d'abord, puis biome, puis côte.
		_region_centre[r] = _centre_kind(sg, b, coastal)

## terrain du centre-ville (pack centres/) : 6 familles, dérivées de l'hydro/biome/côte.
func _centre_kind(sg: int, biome: int, coastal: bool) -> String:
	if sg == 2 or sg == 1:                                  # estuaire / rivière
		return "estuaire"
	if biome == 15 or biome == 21 or biome == 22:           # marais / humides → lacustre
		return "lacustre"
	if biome == 16 or biome == 17 or biome == 18 or biome == 19:  # collines / montagnes
		return "montagne"
	if biome == 12 or biome == 13 or biome == 14:           # forêts
		return "foret"
	if coastal or biome == 3 or biome == 11:                # côte → portuaire
		return "portuaire"
	return "plaine"

## cale l'assise de chaque ville colonisée SUR LA TERRE (le centroïde brut d'une
## région côtière/insulaire peut tomber dans l'eau → snap vers la terre la plus
## proche, puis léger RABAT vers l'intérieur — comme settle_land_spot/rabat de
## viewer.c) → le gros sprite de ville n'a plus sa base dans la mer.
func _build_anchors() -> void:
	_region_anchor.clear()
	_region_citymax.clear()
	_clear_set.clear()
	var w = Sim.world
	if w == null:
		return
	var sea: Image = w.layer_image(LAYER_WATER)   # mer OU lac : 0 = terre, ≥ 1 = eau
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		if t < 0:
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var land := _snap_to_land(sea, ctr)
		var want := 16.0 + t * 6.0                         # taille de sprite VOULUE (∝ tier)
		var best := land
		var best_sz := _max_dry_size(sea, land)
		# si le gros sprite ne tient pas (côte), POUSSE vers l'intérieur (à l'opposé de
		# la mer) jusqu'à trouver une assise qui le porte au sec — une cité s'asseoit en
		# RETRAIT de son rivage (naturel), plutôt que de rapetisser en pastille.
		var sdir := _nearest_sea_dir(sea, land, 8)
		if sdir != Vector2.ZERO and best_sz < want:
			for push in [2.0, 4.0, 6.0, 9.0, 12.0]:
				var cand := _snap_to_land(sea, land - sdir * push)
				var sz := _max_dry_size(sea, cand)
				if sz > best_sz:
					best_sz = sz
					best = cand
				if best_sz >= want:
					break
		_region_anchor[r] = best
		_region_citymax[r] = best_sz
		# DÉBOISE un disque autour de l'assise (∝ tier) → le bourg respire dans une
		# clairière au lieu d'être noyé sous la canopée (comme le masque de viewer.c).
		# Couvre TOUTE l'emprise du bourg (champs au large à r≈12) + une marge franche.
		var clr := int(18.0 + t * 2.2)
		var bcx := int(best.x)
		var bcy := int(best.y)
		for dy in range(-clr, clr + 1):
			for dx in range(-clr, clr + 1):
				if dx * dx + dy * dy <= clr * clr:
					_clear_set[Vector2i(bcx + dx, bcy + dy)] = true

## rend la cellule de TERRE (sea < 1) la plus proche de `c` (anneaux croissants,
## comme settle_land_spot). Renvoie `c` tel quel si aucune terre à portée.
func _snap_to_land(sea: Image, c: Vector2) -> Vector2:
	if sea == null:
		return c
	var sw := sea.get_width()
	var sh := sea.get_height()
	var cx := int(c.x)
	var cy := int(c.y)
	for R in range(0, 15):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if R > 0 and absi(dx) != R and absi(dy) != R:
					continue                       # bord d'anneau seulement
				var nx := cx + dx
				var ny := cy + dy
				if nx < 0 or ny < 0 or nx >= sw or ny >= sh:
					continue
				if int(sea.get_pixel(nx, ny).r * 255.0 + 0.5) < 1:
					return Vector2(nx, ny)
	return c

## direction NORMALISÉE vers la mer la plus proche (≤ maxrad), Vector2.ZERO si aucune.
func _nearest_sea_dir(sea: Image, c: Vector2, maxrad: int) -> Vector2:
	if sea == null:
		return Vector2.ZERO
	var sw := sea.get_width()
	var sh := sea.get_height()
	var cx := int(c.x)
	var cy := int(c.y)
	for R in range(1, maxrad + 1):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if absi(dx) != R and absi(dy) != R:
					continue
				var nx := cx + dx
				var ny := cy + dy
				if nx < 0 or ny < 0 or nx >= sw or ny >= sh:
					continue
				if int(sea.get_pixel(nx, ny).r * 255.0 + 0.5) >= 1:
					return Vector2(dx, dy).normalized()
	return Vector2.ZERO

## VRAI ssi le RECTANGLE du sprite (ancré au PIED en `base`, large de 2·halfw, montant
## de `up` vers le nord) est ENTIÈREMENT au sec — balayage DENSE de chaque cellule (pas
## un échantillon clairsemé : un seul pixel d'eau sous le sprite suffit à le refuser).
func _sea_clear_rect(sea: Image, base: Vector2, halfw: float, up: float) -> bool:
	if sea == null:
		return true
	var sw := sea.get_width()
	var sh := sea.get_height()
	var x0 := int(floor(base.x - halfw))
	var x1 := int(ceil(base.x + halfw))
	var y0 := int(floor(base.y - up))
	var y1 := int(ceil(base.y))
	var y := y0
	while y <= y1:
		if y >= 0 and y < sh:
			var x := x0
			while x <= x1:
				if x >= 0 and x < sw:
					if int(sea.get_pixel(x, y).r * 255.0 + 0.5) >= 1:
						return false
				x += 1
		y += 1
	return true

## plus grande taille de sprite carré (sz large, sz haut, ancré au pied) TENANT au sec
## à `base` — on essaie des tailles CROISSANTES et on s'arrête au premier débord.
func _max_dry_size(sea: Image, base: Vector2) -> float:
	var best := 0.0
	for sz in [6.0, 8.0, 12.0, 16.0, 22.0, 28.0, 34.0, 40.0, 46.0]:
		if _sea_clear_rect(sea, base, sz * 0.5, sz):
			best = sz
		else:
			break
	return best

## empreinte d'un BÂTI parsemé : rect (±4 × 9) au sec ET aucune cellule de rivière.
func _footprint_clear(sea: Image, rset: Dictionary, p: Vector2, halfw: float, up: float) -> bool:
	if not _sea_clear_rect(sea, p, halfw, up):
		return false
	if rset.is_empty():
		return true
	var bx := int(p.x)
	var by := int(p.y)
	var hw := int(ceil(halfw))
	var uu := int(ceil(up))
	for dy in range(0, -uu - 1, -1):
		for dx in range(-hw, hw + 1):
			if rset.has(Vector2i(bx + dx, by + dy)):
				return false
	return true

## VRAI si le nom contient l'un des fragments donnés.
func _has_any(s: String, subs: Array) -> bool:
	for sub in subs:
		if s.contains(sub):
			return true
	return false

## trie (1×, mis en cache) les 96 sprites de structures en BANCS thématiques, pour
## composer un bourg COHÉRENT : civique au cœur, ateliers, logements, champs au large.
func _buckets(names: Array) -> Dictionary:
	if not _bk.is_empty():
		return _bk
	var civic := []
	var craft := []
	var dwell := []
	var field := []
	for nm in names:
		var s: String = nm
		if _has_any(s, ["FIELD", "FARM", "BARN", "ORCHARD", "VINE", "WHEAT", "HAY", "VEGETABLE", "IRRIGATION", "SCARECROW", "REED", "NET_HUT", "FISHER"]):
			field.append(s)
		elif _has_any(s, ["TRADE", "FORGE", "SMITH", "LUMBER", "QUARRY", "POTTER", "WORKSHOP", "STONE_YARD", "WATERMILL", "WINDMILL", "MASON", "WEAVER", "TANNER", "KILN", "PLANK", "TIMBER", "MILLER", "MINER"]):
			craft.append(s)
		elif _has_any(s, ["CIVIC", "UTILITY", "GUARD", "WATCH", "SIGNAL", "SHRINE", "COURT", "TOWN_HALL"]):
			civic.append(s)
		else:
			dwell.append(s)
	# garde-fou : aucun banc vide (repli sur l'ensemble)
	if civic.is_empty(): civic = names
	if craft.is_empty(): craft = names
	if dwell.is_empty(): dwell = names
	if field.is_empty(): field = names
	_bk = {"civic": civic, "craft": craft, "dwell": dwell, "field": field}
	return _bk

## nombre de BÂTIMENTS (manufactures) réellement posés dans la région — lu par la
## membrane (province_income · ligne « manufactured ») = ce que montre l'UI provinciale.
func _region_craft_count(w, ctr: Vector2) -> int:
	var pid: int = w.province_at(int(ctr.x), int(ctr.y))
	if pid < 0:
		return 0
	var inc: Array = w.province_income(pid)
	var n := 0
	for line in inc:
		if bool(line.get("manufactured", false)):
			n += 1
	return n

## pose `count` sprites d'un BANC en spirale phyllotaxique (angle d'or) autour de `ctr`,
## dans la bande de rayon [rbase, rbase+rspan] — un anneau cohérent, pas un nuage. Chaque
## bâtiment tire SA taille (autour de `base_sz`) et un MIROIR optionnel → variété ; son
## empreinte (à SA taille) doit être au sec, sinon SAUTÉ. `idx` court d'un anneau à l'autre.
## VRAI si la BASE (le pied) de l'asset tombe sur une tuile FALAISE (biome highland 18/19/23). En
## iso, seule la BASE compte (profondeur) : le HAUT d'un asset PEUT surplomber la falaise — on ne
## teste donc QUE le point-pied, pas une empreinte.
func _is_cliff_base(p: Vector2) -> bool:
	if _bio_img == null:
		return false
	var x := clampi(int(p.x), 0, _bio_img.get_width() - 1)
	var y := clampi(int(p.y), 0, _bio_img.get_height() - 1)
	var b := int(_bio_img.get_pixel(x, y).r * 255.0 + 0.5)
	return b == 18 or b == 19 or b == 23

func _place_zone(pool: Array, count: int, ctr: Vector2, rbase: float, rspan: float,
		base_sz: float, flipok: bool, idx: int, jit: float, sea: Image, rset: Dictionary, r: int) -> int:
	if pool.is_empty():
		return idx
	for j in range(count):
		var hh := ((r * 374761393) ^ (idx * 2246822519 + 668265263)) & 0x7fffffff
		var ang := float(hh % 6283) * 0.001 + jit        # angle ALÉATOIRE → scatter ORGANIQUE (pas la spirale)
		var rad := rbase + float((hh >> 13) % 100) / 100.0 * rspan
		var p := ctr + Vector2(cos(ang), sin(ang)) * rad
		var sz := base_sz                                 # taille UNIFORME : la variété vient du SPRITE (+miroir), pas de l'échelle
		idx += 1
		if _road_cells.has(Vector2i(int(p.x), int(p.y))):
			continue                                      # sur la chaussée → on saute (le bâti longe, ne couvre pas)
		if not _footprint_clear(sea, rset, p, sz * 0.5, sz):
			continue                                      # déborde l'eau → on saute (ville sur terre)
		if _is_cliff_base(p):
			continue                                      # PIED sur une tuile falaise → interdit (iso : seule la base compte, le haut peut surplomber)
		var nm: String = pool[(hh ^ (hh >> 5)) % pool.size()]   # pick mieux brassé
		_structures.append({"name": nm, "pos": p, "sz": sz, "flip": flipok and (((hh >> 17) & 1) == 1)})
		if _structures.size() >= 4800:
			break
	return idx

## organise le BÂTI le long des ROUTES de la ville (la rue-village) : on longe chaque route depuis le
## bourg et on pose des LOGEMENTS en QUINCONCE de part et d'autre de la chaussée, en laissant LIBRE le
## pied des marches (≥ `clear_foot` : le raccord route↔asset reste visible) et la chaussée (décalé perp,
## jamais sur une cellule de route). `budget` = total de logements à aligner ; le reste ira en anneau.
func _place_road_houses(pool: Array, r: int, base_sz: float, idx: int, sea: Image, rset: Dictionary,
		reach: float, clear_foot: float, budget: int) -> int:
	_road_placed = 0
	if pool.is_empty() or budget <= 0:
		return idx
	for rd in _roads:
		if _road_placed >= budget:
			break
		var pts: PackedVector2Array = rd["points"]
		if pts.size() < 3:
			continue
		var te := -1                                          # le bout côté CETTE ville
		if int(rd.get("ra", -1)) == r:
			te = 0
		elif int(rd.get("rb", -1)) == r:
			te = pts.size() - 1
		if te < 0:
			continue
		var stepd := 1 if te == 0 else -1
		var i := te
		var walked := 0.0
		var next_place := clear_foot                          # 1er logement APRÈS le pied des marches
		var sidx := 0
		while _road_placed < budget and walked < reach:
			var ni := i + stepd
			if ni < 0 or ni >= pts.size():
				break
			var a: Vector2 = pts[i]
			var b: Vector2 = pts[ni]
			var seg := a.distance_to(b)
			i = ni
			if seg < 0.001:
				continue
			walked += seg
			if walked < next_place:
				continue
			var dir := (b - a) / seg
			var perp := Vector2(-dir.y, dir.x)
			var hh := ((r * 374761393) ^ (idx * 2246822519 + sidx * 668265263)) & 0x7fffffff
			var side := 1.0 if (sidx % 2 == 0) else -1.0       # QUINCONCE : alterne les rives
			var off := 1.7 + float(hh % 100) / 100.0 * 1.4     # 1.7..3.1 : À CÔTÉ de la chaussée
			var jl := (float((hh >> 7) % 100) / 100.0 - 0.5) * 1.2
			var p: Vector2 = a + perp * (side * off) + dir * jl
			sidx += 1
			idx += 1
			next_place += 2.6 + float((hh >> 14) % 100) / 100.0 * 1.6   # pas le long ~2.6..4.2
			if _road_cells.has(Vector2i(int(p.x), int(p.y))):
				continue
			if not _footprint_clear(sea, rset, p, base_sz * 0.5, base_sz):
				continue
			if _is_cliff_base(p):
				continue
			_structures.append({"name": pool[(hh ^ (hh >> 5)) % pool.size()], "pos": p, "sz": base_sz,
				"flip": ((hh >> 17) & 1) == 1})
			_road_placed += 1
			if _structures.size() >= 4800:
				return idx
	return idx

## bâti le long de la RUE PRINCIPALE (a → s, vers le sud) — la route est tracée à largeur NORMALE
## (elle se fond dans le réseau), mais on la BORDE sur TOUTE sa longueur en QUINCONCE serré (les deux
## rives alternées) → l'axe visuel côté joueur est COUVERT d'un bout à l'autre par un lot de bâtiments.
func _place_western(r: int, a: Vector2, s: Vector2, pool: Array, base_sz: float, idx: int,
		sea: Image, rset: Dictionary) -> int:
	if pool.is_empty():
		return idx
	var d := s - a
	var lng := d.length()
	if lng < 3.0:
		return idx
	var dir := d / lng
	var perp := Vector2(-dir.y, dir.x)
	var t := 2.0                                          # démarre tôt → l'axe est garni dès le pied
	var sidx := 0
	while t <= lng + 0.5:
		var hh := ((r * 2654435761) ^ (sidx * 40503 + 668265263)) & 0x7fffffff
		var side := 1.0 if (sidx % 2 == 0) else -1.0      # QUINCONCE : les deux rives alternées
		var off := 1.6 + float(hh % 90) / 100.0           # 1.6..2.5 : SERRÉ contre la chaussée
		var p: Vector2 = a + dir * t + perp * (side * off)
		sidx += 1
		idx += 1
		t += 2.0 + float((hh >> 14) % 70) / 100.0         # pas ~2.0..2.7 : COUVRE tout l'axe
		if _road_cells.has(Vector2i(int(p.x), int(p.y))):
			continue
		if not _footprint_clear(sea, rset, p, base_sz * 0.5, base_sz):
			continue
		if _is_cliff_base(p):
			continue
		_structures.append({"name": pool[(hh ^ (hh >> 5)) % pool.size()], "pos": p, "sz": base_sz,
			"flip": side < 0.0})
	return idx

## bâtit, pour chaque ville, une AGGLOMÉRATION cohérente AUTOUR du centre urbain :
## un cœur CIVIQUE, une couronne d'ATELIERS (∝ bâtiments posés), une ceinture de
## LOGEMENTS (∝ population), et au large quelques CHAMPS épars (le disparate, ponctuel).
func _build_structures() -> void:
	_structures.clear()
	var w = Sim.world
	if w == null:
		return
	var names := UIKit.structure_names()
	if names.is_empty():
		return
	var bk := _buckets(names)
	_ensure_terrain(w)                            # caches relief + albedo → teinte cohérente des assets
	var sea: Image = w.layer_image(LAYER_WATER)   # mer OU lac : 0 = terre, ≥ 1 = eau (toute classe)
	var rf: Image = _carved_river_field()         # rivière carvée → l'avant de la rue principale l'évite
	var mvr := _mv_ref()
	_bio_img = w.layer_image(2)                   # biome → interdit le PIED d'asset sur tuile falaise
	# ensemble des cellules de RIVIÈRE (on ne bâtit pas dessus non plus) ──────────
	var rset := {}
	for rp in _rivers:
		rset[Vector2i(int(rp.x), int(rp.y))] = true
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		if t < 0:
			continue
		var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))   # assise CALÉE SUR TERRE
		if ctr.x < 0:
			continue
		# Le bourg CROÎT avec le TIER (band, comme le centre T1-T7) ET avec les BÂTIMENTS
		# posés : T1 ≈ 4-5 assets épars ; T7 « full upgrade » ≈ la tuile presque entièrement
		# occupée (métropole ~10k). Scatter ORGANIQUE (Londres médiévale, pas une grille).
		# Croissance QUADRATIQUE des logements → l'écart hameau↔métropole est NET.
		var band := _city_band(w.region_pop(r))          # 1-8 (le tier visible)
		var nb := _region_craft_count(w, ctr)            # bâtiments posés (UI provinciale)
		var civic_n := clampi(band / 2, 1, 5)
		var craft_n := clampi(nb, 0, band)               # ateliers ∝ bâtiments, plafonnés au tier
		var dwell_n := clampi(band * band - 1, 2, 60)    # logements : T1=2 … T7≈48 … T8≈60 (la tuile pleine)
		var field_n := clampi(band - 3, 0, 5)
		var jit := float((r * 2654435761) & 0xffff) / 65536.0 * TAU
		var idx := 0
		# zone : rayon · empan · taille · miroir. TAILLE UNIFORME (`BLD_SIZE`). Les rayons
		# ENTOURENT le centre (≈ demi-largeur 9) : le bourg RING le cœur (visible autour),
		# il ne se cache pas dessous. Scatter organique (Londres), pas une grille.
		# le bourg S'ORGANISE AUTOUR DE LA RUE PRINCIPALE : un petit cœur CIVIQUE serré, la FAÇADE WESTERN
		# (deux rangées qui se font face le long de la rue sud), puis l'agglomération le long des AUTRES
		# routes ; l'anneau ne fait que combler le surplus (routes saturées / absentes).
		idx = _place_zone(bk["civic"], civic_n, ctr, 3.0, 2.0, BLD_SIZE, false, idx, jit, sea, rset, r)
		var along_pool: Array = bk["dwell"] + bk["craft"]
		if mvr != null and mvr.has_method("tile_anchor_world"):
			var aW: Vector2 = mvr.tile_anchor_world(ctr.x, ctr.y)
			var sW: Vector2 = _main_street_end(aW, sea, rf)
			idx = _place_western(r, aW, sW, along_pool, BLD_SIZE, idx, sea, rset)
		var along_n := dwell_n + craft_n
		idx = _place_road_houses(along_pool, r, BLD_SIZE, idx, sea, rset, 40.0, 4.0, along_n)
		# le surplus comble en anneau autour du bourg
		idx = _place_zone(bk["dwell"], maxi(0, along_n - _road_placed), ctr, 6.0, 7.0, BLD_SIZE, true, idx, jit, sea, rset, r)
		idx = _place_zone(bk["field"], field_n, ctr, 13.0, 5.0, BLD_SIZE, true, idx, jit, sea, rset, r)
		if _structures.size() >= 4800:
			break
	# teinte de chaque bâti CALÉE sur le terrain (relief + sol) → fini le « posé là »
	for s in _structures:
		var p: Vector2 = s["pos"]
		s["tint"] = _asset_tint(STRUCT_MUTE, p.x, p.y, GROUND_TINT_BLD)
	# tri arrière→avant (par y) → l'empilement du bourg se lit correctement
	_structures.sort_custom(func(a, b): return a["pos"].y < b["pos"].y)

## DRESSING du monde : scan de la couche biome → arbres/bosquets/buissons/cailloux/roseaux semés
## SPORADIQUEMENT selon le MILIEU (forêt dense ; plaine & aride sporadiques ; marais ; collines ;
## falaise = cailloux/buissons ; neige), PLUS un liseré de cailloux/buissons le long des rivières.
const DRAW_DECOR := true
## règle de semis par biome → [pool, probabilité (au pas), taille] ; [] = rien. La PROBABILITÉ tient
## la densité (forêt dense, plaine très clairsemée), le SPORADIQUE casse tout motif.
func _dress_rule(b: int) -> Array:
	if FOREST_TREES.has(b):
		return [FOREST_TREES[b], 0.55, 12.0]                  # FORÊT : dense
	if b == 4 or b == 5 or b == 6 or b == 24:
		return [DRESS_OPEN, 0.085, 11.0]                      # PLAINE/herbe : ARBRES sporadiques (un peu plus)
	if b == 7 or b == 8:
		return [DRESS_STEPPE, 0.22, 9.0]                      # STEPPE/SAVANE (brun) : touffes d'herbe sèche couvrantes, buissons épars
	if b == 9 or b == 10 or b == 11:
		return [DRESS_DRY, 0.04, 9.0]                         # ARIDE/désert : buissons secs épars
	if b == 22:
		return [DRESS_BOG, 0.80, 8.0, 3]                      # TOURBIÈRE/LANDE : lande COUVERTE — lits de mousse/roseaux (3 touffes)
	if b == 15:
		return [DRESS_MARSH, 0.85, 8.5, 3]                    # MARAIS : roselière DENSE (3 touffes/bloc → vasière couverte)
	if b == 21:
		return [DRESS_MANGROVE, 0.85, 9.5, 3]                 # MANGROVE : thicket dense de palétuviers (3 touffes/bloc)
	if b == 16 or b == 17:
		return [DRESS_HILL, 0.10, 9.0]                        # COLLINES/highlands : cailloux + buissons (allégé)
	if b == 18 or b == 23:
		return [DRESS_CLIFF, 0.14, 9.0]                       # FALAISE/montagne/volcan : cailloux + buissons (allégé)
	if b == 19 or b == 20:
		return [DRESS_SNOW, 0.12, 10.0]                       # NEIGE/glacier : pins enneigés, rochers de neige
	return []

func _build_decor() -> void:
	_decor.clear()
	if not DRAW_DECOR:
		return
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(2)   # SCPS_LAYER_BIOME = 2
	if bio == null:
		return
	_ensure_terrain(w)                            # caches relief + albedo → teinte cohérente du dressing
	var sea: Image = w.layer_image(LAYER_WATER)
	var rf: Image = _carved_river_field()         # eau de RIVIÈRE carvée (ce que le shader rend) → pas de BASE dedans
	var mw := bio.get_width()
	var mh := bio.get_height()
	var stride := 3
	for cy in range(0, mh, stride):
		for cx in range(0, mw, stride):
			var b := int(bio.get_pixel(cx, cy).r * 255.0 + 0.5)
			if b <= 3:
				continue                            # eau / plage : rien
			if _clear_set.has(Vector2i(cx, cy)):
				continue                            # clairière de bourg
			if sea != null and int(sea.get_pixel(cx, cy).r * 255.0 + 0.5) >= 1:
				continue                            # eau (lac)
			var rule := _dress_rule(b)
			if rule.is_empty():
				continue
			var h := ((cx * 73856093) ^ (cy * 19349663)) & 0x7fffffff
			if float(h % 1000) / 1000.0 > float(rule[1]):
				continue                            # SPORADIQUE
			var pool: Array = rule[0]
			var rep: int = int(rule[3]) if rule.size() > 3 else 1   # touffes MULTIPLES → lit dense (zones humides)
			for ti in range(rep):
				var hi := (h * (ti * 2 + 1) + ti * 0x9e3779b9) & 0x7fffffff   # hash décorrélé par touffe
				var jx := (float((hi >> 3) % 13) - 6.0) * 0.22   # jitter étalé sur le bloc (stride) → lit continu
				var jy := (float((hi >> 7) % 13) - 6.0) * 0.22
				var bx := cx + jx
				var by := cy + jy
				if _is_sea_cell(sea, int(bx), int(by)) or _in_river_water(rf, int(bx), int(by)):
					continue                        # la BASE tomberait dans l'eau (mer/lac ou rivière) → interdit
				var sz: float = float(rule[2]) * (0.82 + 0.36 * float((hi >> 15) % 10) / 10.0)
				_decor.append({"name": pool[(hi >> 11) % pool.size()], "pos": Vector2(bx, by), "sz": sz})
			if _decor.size() >= 16000:
				break
		if _decor.size() >= 16000:
			break
	_dress_rivers(sea)
	# teinte du dressing CALÉE sur le terrain (relief + sol) — les arbres/cailloux appartiennent au sol
	for d in _decor:
		var p: Vector2 = d["pos"]
		d["tint"] = _asset_tint(Color(1, 1, 1, 1), p.x, p.y, GROUND_TINT_DEC)
	# tri arrière→avant (profondeur iso = y) → l'empilement des arbres/cailloux se lit correctement
	_decor.sort_custom(func(a, c): return (a["pos"] as Vector2).y < (c["pos"] as Vector2).y)

## cailloux/buissons/roseaux SPORADIQUES le long du fil de rivière (le nuage worldgen) — la berge
## habillée. Léger décalage du fil, jamais dans l'eau ni dans une clairière de bourg.
func _dress_rivers(sea: Image) -> void:
	var rf: Image = _carved_river_field()
	for rp in _rivers:
		var cx := int(rp.x)
		var cy := int(rp.y)
		var h := ((cx * 2654435761) ^ (cy * 40503)) & 0x7fffffff
		if (h % 100) >= 10:
			continue                                # ~10 % des points → SPORADIQUE
		var p := Vector2(cx + (float((h >> 3) % 5) - 2.0), cy + (float((h >> 7) % 5) - 2.0))
		if _clear_set.has(Vector2i(int(p.x), int(p.y))):
			continue
		if sea != null and _is_sea_cell(sea, int(p.x), int(p.y)):
			continue
		if _in_river_water(rf, int(p.x), int(p.y)):
			continue                                # la BASE tomberait dans l'eau → la berge, jamais le lit
		_decor.append({"name": DRESS_RIVER[(h >> 11) % DRESS_RIVER.size()], "pos": p, "sz": 7.0})
		if _decor.size() >= 17000:
			return

func _on_tick(_year: int) -> void:
	_struct_dirty = true       # pop & bâtiments ont bougé → le bourg sera reconstruit au prochain dessin zoomé
	var sig := _owner_signature(Sim.world)
	if sig != _owner_sig:      # la souveraineté a changé (conquête/colonisation) →
		_owner_sig = sig       # refaire frontières ET réseau de routes (villes neuves/captées)
		_borders_dirty = true
		_roads_dirty = true
	_ensure_roads()            # date les chantiers neufs dès maintenant (même non zoomé)
	queue_redraw()

## signature de la photo des propriétaires de régions → détecte conquête/colonisation.
func _owner_signature(w) -> int:
	if w == null:
		return -1
	var sig := 0
	for r in range(w.region_count()):
		sig = (sig * 1000003 + (w.region_owner(r) + 2)) & 0x3fffffff
	return sig

## reconstruit les segments de frontière (région + pays) depuis la façade (port bseg).
func _rebuild_borders() -> void:
	var w = Sim.world
	if w == null:
		return
	_borders[1] = w.border_segments(1)   # RÉGIONS
	_borders[2] = w.border_segments(2)   # PAYS
	_owner_sig = _owner_signature(w)
	_borders_dirty = false

## (re)charge le réseau de routes + sa méta + l'habillage, et DATE les chantiers neufs.
## Appelé hors zoom (générate/tick) → les routes initiales démarrent dès l'an de fondation,
## même si le joueur n'a pas encore zoomé (sinon elles « repartiraient » au premier zoom).
## `prebuild` (monde MÛR : chargement de save / re-génération à l'an N>0) → les routes initiales
## sont datées DANS LE PASSÉ (déjà bâties) au lieu de re-construire de zéro sous les yeux.
func _ensure_roads(prebuild := false) -> void:
	if not _roads_dirty:
		return
	var w = Sim.world
	if w == null:
		return
	_roads = w.road_paths()
	_augment_roads(w)
	var yr0: int = w.year()
	for rd in _roads:
		if not _road_start.has(rd["key"]):
			if prebuild:
				_road_start[rd["key"]] = yr0 - int(rd.get("nprov", 1)) - 1   # déjà bâtie (monde mûr)
			else:
				_road_start[rd["key"]] = yr0     # route NEUVE → chantier daté à maintenant (croît)
	_build_main_streets()                    # rue principale (sud) de chaque bourg — la façade western
	_build_road_dress()
	_build_road_cells()                      # empreinte des routes (+ rues principales) → le bourg les évite
	_roads_dirty = false
	_struct_dirty = true                     # les routes ont bougé → le bourg se recale (évitement)

## marque les cellules occupées par une route (+ marge 1) → le bourg en spirale les ÉVITE
## (le bâti ne pousse pas sur la chaussée ; les ruelles serpentent ENTRE).
func _build_road_cells() -> void:
	_road_cells.clear()
	for rd in _roads:
		var pts: PackedVector2Array = rd["points"]
		for p in pts:
			_stamp_cell(int(p.x), int(p.y))
	for ms in _main_streets:                         # la rue principale compte AUSSI comme chaussée (±1)
		var a: Vector2 = ms["a"]
		var s: Vector2 = ms["s"]
		var n := maxi(1, int(a.distance_to(s)))
		for k in range(n + 1):
			var p := a.lerp(s, float(k) / float(n))
			_stamp_cell(int(p.x), int(p.y))

func _stamp_cell(bx: int, by: int) -> void:
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			_road_cells[Vector2i(bx + dx, by + dy)] = true

## extrémité de la RUE PRINCIPALE : on pousse vers l'AVANT (iso-sud = +x+y monde) ; si l'avant strict
## a de l'eau SUR SON TRAJET (bourg côtier), on tente avant-est puis avant-ouest ; sinon pas de rue.
func _main_street_end(a: Vector2, sea: Image, rf: Image) -> Vector2:
	for d in [Vector2(0.707, 0.707), Vector2(0.917, 0.4), Vector2(0.4, 0.917)]:
		var clear := true
		for k in range(1, 6):                        # tout le tronçon doit être au sec (pas qu'au bout)
			var p: Vector2 = a + d * (MAIN_ST_LEN * float(k) / 5.0)
			if _is_sea_cell(sea, int(p.x), int(p.y)) or _in_river_water(rf, int(p.x), int(p.y)):
				clear = false
				break
		if clear:
			return a + d * MAIN_ST_LEN
	return a

## pour chaque bourg : la RUE PRINCIPALE = un court tronçon de l'ancre (pied du centre) vers l'avant
## (sud). Elle rejoint les autres routes À L'ANCRE (pas un stub flottant) et porte la façade western.
func _build_main_streets() -> void:
	_main_streets.clear()
	var w = Sim.world
	if w == null:
		return
	var mv := _mv_ref()
	if mv == null or not mv.has_method("tile_anchor_world"):
		return
	var sea: Image = w.layer_image(LAYER_WATER)
	var rf: Image = _carved_river_field()
	for r in range(w.region_count()):
		if w.region_tier(r) < 0 or not _region_anchor.has(r):
			continue
		var ctr: Vector2 = _region_anchor[r]
		var a: Vector2 = mv.tile_anchor_world(ctr.x, ctr.y)
		var s: Vector2 = _main_street_end(a, sea, rf)
		if a.distance_to(s) >= 2.0:
			_main_streets.append({"a": a, "s": s, "r": r})

## méta LOCALE par route (sans toucher la façade) : nombre de PROVINCES traversées (pour
## la cadence « 1 an/province ») + une CLÉ stable (paire de régions des extrémités) pour
## retenir l'année de début de chantier à travers les reconstructions du réseau.
func _augment_roads(w) -> void:
	var sea: Image = w.layer_image(LAYER_WATER)
	var rf: Image = _carved_river_field()
	var mv := _mv_ref()
	for rd in _roads:
		var pts: PackedVector2Array = rd["points"]
		# PROVINCES traversées (cadence du chantier 1 an/province) — sur le tracé BRUT (A*).
		var np := 1
		var last := -999
		for p in pts:
			var pv: int = w.province_at(int(p.x), int(p.y))
			if pv >= 0 and pv != last:
				if last != -999:
					np += 1
				last = pv
		rd["nprov"] = np
		var ra: int = w.province_region(w.province_at(int(pts[0].x), int(pts[0].y)))
		var rb: int = w.province_region(w.province_at(int(pts[pts.size() - 1].x), int(pts[pts.size() - 1].y)))
		rd["key"] = (mini(ra, rb) & 0xfff) * 4096 + (maxi(ra, rb) & 0xfff)
		# 1) PATHFINDING (rendu) : on LISSE d'abord le chemin BRUT (resample + Chaikin gardé-eau) — l'A*
		#    moteur reste la vérité ; courbe propre, points réguliers, jamais sur la côte.
		pts = _smooth_resample_road(pts, sea, rf)
		# 2) SNAP : raccord PROPRE au pied des marches de l'asset (l'ancre = sommet bas de tuile, où la
		#    petite route du sprite débouche) ; trim des points qui tanglent → approche radiale nette.
		if mv != null and mv.has_method("tile_anchor_world"):
			if _region_anchor.has(ra):
				var a0: Vector2 = _region_anchor[ra]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a0.x, a0.y), true)
			if _region_anchor.has(rb):
				var a1: Vector2 = _region_anchor[rb]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a1.x, a1.y), false)
		rd["ra"] = ra            # mémorisé : le bâti du bourg s'organise le long des routes de SA ville
		rd["rb"] = rb
		rd["points"] = pts

## portion BÂTIE d'un tracé (du départ, par longueur) — `frac` ∈ [0,1] → croissance organique.
func _road_partial(pts: PackedVector2Array, frac: float) -> PackedVector2Array:
	if frac >= 1.0:
		return pts
	if frac <= 0.0 or pts.size() < 2:
		return PackedVector2Array()
	var total := 0.0
	for i in range(pts.size() - 1):
		total += pts[i].distance_to(pts[i + 1])
	var target := total * frac
	var out := PackedVector2Array()
	out.append(pts[0])
	var acc := 0.0
	for i in range(pts.size() - 1):
		var seg := pts[i].distance_to(pts[i + 1])
		if acc + seg >= target:
			out.append(pts[i].lerp(pts[i + 1], (target - acc) / maxf(seg, 0.001)))
			break
		acc += seg
		out.append(pts[i + 1])
	return out

## SNAP d'extrémité : retire les points qui tanglent dans le rayon de l'ancre, puis raccorde l'ancre
## (pied des marches de l'asset) au 1er point survivant → approche RADIALE nette (toujours ≥ 2 points).
func _snap_endpoint(pts: PackedVector2Array, anchor: Vector2, from_start: bool) -> PackedVector2Array:
	if pts.size() < 3:
		var two := PackedVector2Array()
		if from_start:
			two.append(anchor); two.append(pts[pts.size() - 1])
		else:
			two.append(pts[0]); two.append(anchor)
		return two
	var out := PackedVector2Array()
	if from_start:
		var i := 0
		while i < pts.size() - 2 and pts[i].distance_to(anchor) < ROAD_SNAP_TRIM:
			i += 1
		out.append(anchor)
		for k in range(i, pts.size()):
			out.append(pts[k])
	else:
		var j := pts.size() - 1
		while j > 1 and pts[j].distance_to(anchor) < ROAD_SNAP_TRIM:
			j -= 1
		for k in range(0, j + 1):
			out.append(pts[k])
		out.append(anchor)
	return out

## ré-échantillonne un tracé à PAS CONSTANT (cellules) — points réguliers, water-safe (on interpole
## sur des segments DÉJÀ sûrs). Garde exactement la 1re et la dernière position (le snapping tient).
func _resample_polyline(pts: PackedVector2Array, spacing: float) -> PackedVector2Array:
	var out := PackedVector2Array()
	if pts.size() < 2:
		return pts
	out.append(pts[0])
	var cur: Vector2 = pts[0]
	var acc := 0.0
	var i := 1
	while i < pts.size():
		var nxt: Vector2 = pts[i]
		var seg := cur.distance_to(nxt)
		if seg < 0.0001:
			cur = nxt; i += 1; continue
		if acc + seg >= spacing:
			cur = cur.lerp(nxt, (spacing - acc) / seg)
			out.append(cur)
			acc = 0.0
		else:
			acc += seg
			cur = nxt
			i += 1
	var endp: Vector2 = pts[pts.size() - 1]
	if out[out.size() - 1].distance_to(endp) > 0.001:
		out.append(endp)
	return out

## Chaikin (corner-cutting) GARDÉ-EAU : un point coupé qui tomberait en eau (mer/lac/rivière) est
## remplacé par le coin d'origine → la route se LISSE partout SAUF à la côte, qu'elle continue d'épouser.
func _chaikin_safe(pts: PackedVector2Array, sea: Image, rf: Image) -> PackedVector2Array:
	if pts.size() < 3:
		return pts
	var out := PackedVector2Array()
	out.append(pts[0])
	for i in range(pts.size() - 1):
		var a: Vector2 = pts[i]
		var b: Vector2 = pts[i + 1]
		var q := a.lerp(b, 0.25)
		var r := a.lerp(b, 0.75)
		out.append(a if (_is_sea_cell(sea, int(q.x), int(q.y)) or _in_river_water(rf, int(q.x), int(q.y))) else q)
		out.append(b if (_is_sea_cell(sea, int(r.x), int(r.y)) or _in_river_water(rf, int(r.x), int(r.y))) else r)
	out.append(pts[pts.size() - 1])
	return out

## tracé RENDU : ré-échantillonné régulier + 2 passes Chaikin gardées-eau (courbe propre, extrémités fixes).
func _smooth_resample_road(pts: PackedVector2Array, sea: Image, rf: Image) -> PackedVector2Array:
	if pts.size() < 3:
		return pts
	var rs := _resample_polyline(pts, ROAD_RESAMPLE)
	rs = _chaikin_safe(rs, sea, rf)
	rs = _chaikin_safe(rs, sea, rf)
	return rs

## petit hash LCG (déterministe) pour les tirages par route — gaps & tailles de clumps du mobilier.
func _rh(s: int) -> int:
	return (s * 1103515245 + 12345) & 0x7fffffff

## sème le MOBILIER de bord de route en PETITS CLUMPS, distance & densité VARIÉES (pas un chapelet
## régulier) : marche à l'arc, on saute un GAP variable puis on dépose un GROUPE de 1-4 items serrés
## (le long & la marge sud jittés). Chaque item retient SA route → apparaît au chantier ACHEVÉ.
const DRAW_ROAD_DRESS := true    # MOBILIER de bord de route ON : petits bosquets de buissons/cailloux,
                                 # côté SUD de la chaussée (devant, proj.y plus grand), groupés et épars.
func _build_road_dress() -> void:
	_road_dress.clear()
	if not DRAW_ROAD_DRESS:
		return
	var w = Sim.world
	if w == null:
		return
	var sea: Image = w.layer_image(LAYER_WATER)
	var rf: Image = _carved_river_field()          # le mobilier ne tombe pas dans l'eau de rivière (pont)
	for ri in range(_roads.size()):
		var pts: PackedVector2Array = _roads[ri]["points"]
		if pts.size() < 2:
			continue
		var rs := (ri * 374761393 + 2246822519) & 0x7fffffff   # graine de route
		var next_at := 3.0 + float(rs % 700) / 100.0           # 1er clump à ~3..10 cellules
		rs = _rh(rs)
		var acc := 0.0
		for i in range(1, pts.size()):
			var a: Vector2 = pts[i - 1]
			var b: Vector2 = pts[i]
			var seg := a.distance_to(b)
			if seg < 0.0001:
				continue
			var dir := (b - a) / seg
			var perp := Vector2(-dir.y, dir.x)
			if perp.x + perp.y < 0.0:
				perp = -perp                       # marge SUD écran (proj.y plus GRAND = devant/bas)
			while next_at <= acc + seg:
				var center: Vector2 = a + dir * (next_at - acc)
				var nclump := 1 + (rs % 4); rs = _rh(rs)        # 1..4 items dans le bosquet
				for k in range(nclump):
					var jl := (float(rs % 100) / 100.0 - 0.4) * 3.4; rs = _rh(rs)   # le long : -1.4..+2.0
					var jo := ROAD_DRESS_OFF + float(rs % 100) / 100.0 * 1.5; rs = _rh(rs)  # marge sud 0.95..2.45
					var p: Vector2 = center + dir * jl + perp * jo
					if not _is_sea_cell(sea, int(p.x), int(p.y)) and not _in_river_water(rf, int(p.x), int(p.y)):
						_road_dress.append({"name": ROADSIDE[rs % ROADSIDE.size()], "pos": p, "road": ri})
					rs = _rh(rs)
				next_at += 5.0 + float(rs % 1100) / 100.0; rs = _rh(rs)   # GAP variable ~5..16 cellules
			acc += seg

## VRAI si la cellule est de l'eau (mer, toute classe ≥ 1) — pour ne rien semer dessus.
func _is_sea_cell(sea: Image, ix: int, iy: int) -> bool:
	if sea == null or ix < 0 or iy < 0 or ix >= sea.get_width() or iy >= sea.get_height():
		return false
	return int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1

## le CHAMP DÉBIT carvé (L8) que le shader iso_blend rend en EAU — récupéré du nœud IsoGround voisin.
## Sert à interdire la BASE d'un asset dans l'eau de RIVIÈRE (la mer/lac est déjà couverte par la couche WATER).
const RIVER_WATER_MIN := 0.36   ## champ ≥ ça = eau rendue (shader river_water 0.40) + marge de berge
func _carved_river_field() -> Image:
	var p := get_parent()
	if p == null:
		return null
	var g = p.get_node_or_null("IsoGround")
	if g != null and g.has_method("river_field"):
		return g.river_field(Sim.world)
	return null

## VRAI si la BASE (ix,iy) tombe dans l'eau de rivière carvée (≥ river_water) → pas d'asset dessus.
func _in_river_water(rf: Image, ix: int, iy: int) -> bool:
	if rf == null or ix < 0 or iy < 0 or ix >= rf.get_width() or iy >= rf.get_height():
		return false
	return rf.get_pixel(ix, iy).r >= RIVER_WATER_MIN

func _process(_dt: float) -> void:
	# pendant un cataclysme, on redessine en continu pour PULSER l'épicentre
	# (horloge MUR, hors déterminisme). Sinon : aucun coût (le tick suffit).
	if _cataclysm:
		queue_redraw()

## pop d'une région → bande de ville 1-8 (les paliers des sprites CITY_POP_BAND).
const CITY_POP_BANDS := [150, 400, 900, 1800, 3500, 7000, 14000]   # 7 seuils → 8 bandes
func _city_band(pop: int) -> int:
	var b := 1
	for thr in CITY_POP_BANDS:
		if pop >= thr:
			b += 1
	return b

## couleur stable par pays (display-only : un hash d'indice → teinte ; -1 = neutre)
func _country_color(c: int) -> Color:
	if c < 0:
		return Color(0.7, 0.7, 0.72)
	return Color.from_hsv(fmod(c * 0.137, 1.0), 0.72, 0.90)

func _phase_color(phase: int) -> Color:
	match phase:
		PHASE_SIEGE:  return Color(0.95, 0.6, 0.2)   # orange : siège
		PHASE_BATTLE: return Color(0.95, 0.25, 0.2)  # rouge : bataille
		PHASE_MARCH:  return Color(0.95, 0.95, 0.95) # blanc : marche
		_:            return Color(0.8, 0.8, 0.85)   # gris : au repos
	return Color.WHITE

## choisit le JETON d'armée selon sa TAILLE puis sa composition DOMINANTE.
## (units/inf/arch/cav/mages sont en PAQUETS de 100 — campaign_units.)
func _army_token_name(a: Dictionary) -> String:
	var tot: int = a.get("units", 0)
	if tot >= 22:
		return "ARMY_TOKEN_LARGE_HOST"
	if tot <= 4:
		return "ARMY_TOKEN_SCOUT_COLUMN"
	var inf: int = a.get("inf", 0)
	var arch: int = a.get("arch", 0)
	var cav: int = a.get("cav", 0)
	var mages: int = a.get("mages", 0)
	var m := maxi(maxi(inf, arch), maxi(cav, mages))
	if mages == m and mages > 0:
		return "ARMY_TOKEN_SORCERER_ESCORT"
	if cav == m and cav > 0:
		return "ARMY_TOKEN_HEAVY_CAVALRY"
	if arch == m and arch > 0:
		return "ARMY_TOKEN_MIXED_ARCHERS"
	return "ARMY_TOKEN_PIKE_BLOCK"

## bâti de CAMPAGNE rabattu : un peu transparent + assombri (« masque de fondu ») → la terre lissée
## transparaît, le contraste cuivré/noir baisse, le contour de socle se FOND (plus d'effet d'îlot).
## Le centre-ville (landmark), lui, reste OPAQUE et pleinement présent (cf. _draw_city).
const STRUCT_MUTE := Color(0.88, 0.86, 0.83, 0.84)

## dessine UN bâti parsemé à la POSITION ISO `sp` (espace-monde iso, la Camera2D met à l'échelle) —
## taille MONDE, ancré au pied, MIROIR éventuel (variété), RABATTU pour s'intégrer à la campagne.
## ombre portée d'un asset : ellipse aplatie au PIED, décalée vers l'anti-lumière (cohérent
## avec le shader). Tracée en passe SÉPARÉE (toutes les ombres d'abord) → jamais par-dessus
## un sprite d'arrière-plan (le bug « ombres placées aléatoirement » = ordre de tracé).
# ════════════════════════ COHÉRENCE DE LUMIÈRE (relief + sol) ════════════════════════
func _lum(c: Color) -> float:
	return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b

## charge (paresseux) les caches de lumière : la couche HEIGHT (relief) + l'albedo TERRAIN (sol).
func _ensure_terrain(w) -> void:
	if _himg_l == null:
		_himg_l = w.layer_image(0)            # SCPS_LAYER_HEIGHT (canal R = altitude)
	if _alb_l == null and w.has_method("map_image"):
		_alb_l = w.map_image(0, -1)           # rendu TERRAIN (mode 0), sans sélection → couleur du sol

## ombrage relief en (wx,wy) : pente du champ d'altitude DOT le même soleil que le terrain →
## ~0.80 (versant à l'ombre) .. 1.20 (versant éclairé), 1.0 à plat. La cohérence avec le shader.
func _hillshade(wx: float, wy: float) -> float:
	if _himg_l == null:
		return 1.0
	var hw := _himg_l.get_width()
	var hh := _himg_l.get_height()
	var x := clampi(int(wx), 1, hw - 2)
	var y := clampi(int(wy), 1, hh - 2)
	var hl := _himg_l.get_pixel(x - 1, y).r
	var hr := _himg_l.get_pixel(x + 1, y).r
	var hu := _himg_l.get_pixel(x, y - 1).r
	var hd := _himg_l.get_pixel(x, y + 1).r
	var grad := Vector2(hr - hl, hd - hu)        # pente : pointe vers le HAUT du relief
	if grad.length() < 0.0008:
		return 1.0                               # plat → pas d'ombrage
	var al := grad.normalized().dot(LIGHT_WORLD) # pente FACE à la source = éclairée
	return clampf(1.0 + SHADE_K * al, 0.80, 1.20)

## couleur du SOL rendu en (wx,wy) (albedo terrain), mise à l'échelle si l'albedo n'est pas à la
## résolution monde. Gris moyen en repli (cache absent).
func _ground_color(wx: float, wy: float) -> Color:
	if _alb_l == null:
		return Color(0.5, 0.5, 0.5, 1.0)
	var aw := _alb_l.get_width()
	var ah := _alb_l.get_height()
	var mw := aw
	var mh := ah
	if _himg_l != null:
		mw = _himg_l.get_width()
		mh = _himg_l.get_height()
	var px := clampi(int(wx / float(maxi(1, mw)) * aw), 0, aw - 1)
	var py := clampi(int(wy / float(maxi(1, mh)) * ah), 0, ah - 1)
	return _alb_l.get_pixel(px, py)

## teinte FINALE d'un asset en (wx,wy) : base × ombrage-relief × clarté-du-sol, puis une POINTE de
## la teinte du sol mêlée (lumière rebondie) → l'asset APPARTIENT au lieu au lieu d'y être collé.
func _asset_tint(base: Color, wx: float, wy: float, tint_amt: float) -> Color:
	var shade := _hillshade(wx, wy)
	var g := _ground_color(wx, wy)
	var lumf := 0.80 + 0.30 * clampf(_lum(g) / 0.60, 0.0, 1.0)   # sol noir 0.80 .. sol clair 1.10
	var f := shade * lumf
	return Color(
		lerpf(base.r * f, g.r, tint_amt),
		lerpf(base.g * f, g.g, tint_amt),
		lerpf(base.b * f, g.b, tint_amt),
		base.a)

func _blob_shadow(foot: Vector2, wd: float) -> void:
	draw_set_transform(foot + SHADOW_DIR * wd * 0.14, 0.0, Vector2(1.0, 0.42))
	draw_circle(Vector2.ZERO, wd * 0.30, SHADOW_COL)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

func _draw_struct(s: Dictionary, sp: Vector2) -> void:
	var sspr := UIKit.structure_sprite(s["name"])
	if sspr == null:
		return
	var ss: float = s.get("sz", 9.0)
	var tint: Color = s.get("tint", STRUCT_MUTE)               # calé sur le terrain (relief + sol)
	if bool(s.get("flip", false)):
		draw_set_transform(sp, 0.0, Vector2(-1, 1))             # miroir horizontal autour du pied
		draw_texture_rect(sspr, Rect2(-ss * 0.5, -ss, ss, ss), false, tint)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	else:
		draw_texture_rect(sspr, Rect2(sp - Vector2(ss * 0.5, ss), Vector2(ss, ss)), false, tint)

## dessine LA ville de la région `r` à la POSITION ISO `ctr` — variante TERRAIN sinon bande de
## pop ; taille MONDE BORNÉE au sec ; repli en marqueur sobre. Le BOURG (spiral) l'entoure.
func _draw_city(w, r: int, ctr: Vector2) -> void:
	var t: int = w.region_tier(r)
	var band := _city_band(w.region_pop(r))
	var spr: Texture2D = UIKit.city_centre(_region_centre.get(r, "plaine"), clampi(band, 1, 7))
	if spr == null and _region_variant.has(r):
		spr = UIKit.city_biome(_region_variant[r])
	if spr == null:
		spr = UIKit.city_sprite(band, (r * 2654435761) % 8)
	var sz: float = min(CITY_CORE_SIZE, _region_citymax.get(r, CITY_CORE_SIZE))
	if spr != null and sz >= 6.0:
		draw_texture_rect(spr, Rect2(ctr - Vector2(sz * 0.5, sz), Vector2(sz, sz)), false)  # ancré au pied
	else:
		var radius := 1.2 + t * 0.4
		draw_circle(ctr, radius, Color(0.62, 0.47, 0.30))
		draw_arc(ctr, radius, 0.0, TAU, 16, Color(0.15, 0.10, 0.05, 0.8), 0.4, true)

## mouchetis de POUSSIÈRE le long d'un tracé iso : petits points clairs/sombres jittered, alpha bas →
## la chaussée a du GRAIN au lieu d'une bande lisse. Déterministe (hash position), display-only.
func _road_dust(ip: PackedVector2Array, wln: float) -> void:
	for k in range(ip.size() - 1):
		var a := ip[k]
		var seg := ip[k + 1] - a
		var L := seg.length()
		if L < 0.6:
			continue
		var dir := seg / L
		var perp := Vector2(-dir.y, dir.x)
		var n := maxi(1, int(L / 0.8))                   # DENSE → un vrai voile de grain
		for j in range(n):
			var hh := ((int(a.x * 8.0) * 73856093) ^ (int(a.y * 8.0) * 19349663) ^ (j * 2654435761)) & 0x7fffffff
			var t := (float(j) + 0.5) / float(n)
			var off := (float(hh % 100) / 100.0 - 0.5) * wln * 0.84   # étalé sur presque toute la largeur
			var p := a + dir * (t * L) + perp * off
			var rad := wln * (0.10 + float((hh >> 7) % 20) / 100.0 * 0.11)
			var dust := Color(0.26, 0.18, 0.10, 0.20) if (hh & 1) == 0 else Color(0.97, 0.90, 0.72, 0.16)
			draw_circle(p, rad, dust)

## largeur MONDE de la surface d'une route selon son niveau (artère/desserte/mineure),
## bornée à ~2.6 px d'écran (÷zoom) au minimum — la route RESSORT comme le fil conducteur.
func _road_width(level: int, zoom: float) -> float:
	var maj := 1.0 if level == 0 else (0.78 if level == 1 else 0.6)
	return maxf(0.7 * maj, 2.6 / zoom)

func _mv_ref() -> Node2D:
	if _mv == null:
		_mv = get_parent() as Node2D
	return _mv

# ── projection GLOBE (segments, deux bouts visibles) ───────────────────────────
func _project_segs_globe(mv: Node2D, segs: PackedVector2Array) -> PackedVector2Array:
	var out := PackedVector2Array()
	var i := 0
	while i + 1 < segs.size():
		var a: Dictionary = mv.globe_to_screen(segs[i].x, segs[i].y)
		var b: Dictionary = mv.globe_to_screen(segs[i + 1].x, segs[i + 1].y)
		if a["vis"] and b["vis"]:
			out.append(a["pos"])
			out.append(b["pos"])
		i += 2
	return out

# ── projection ISO (segments, tous projetés sur le relief) ─────────────────────
func _project_segs_iso(mv: Node2D, segs: PackedVector2Array) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(segs.size())
	for i in range(segs.size()):
		out[i] = mv.iso_pos(segs[i].x, segs[i].y)
	return out

# ════════════════════════ dispatch GLOBE / ISO ════════════════════════
func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	var mv := _mv_ref()
	if mv == null or not mv.has_method("globe_to_screen"):
		return
	var vm := 0
	if mv.get("view_mode") != null:
		vm = int(mv.get("view_mode"))
	if vm == 0:
		_draw_globe(w, mv)
	else:
		_draw_iso(w, mv)

## GLOBE (vue d'ensemble) — UNIQUEMENT frontières + noms d'empire (se repérer). Aucun asset.
func _draw_globe(w, mv: Node2D) -> void:
	var vp := get_viewport_rect().size
	var mode := 0
	if mv.get("mode") != null:
		mode = int(mv.get("mode"))
	if _borders_dirty:
		_rebuild_borders()
	# régions seulement en mode régions ; PAYS TOUJOURS (le repère d'orientation).
	if mode >= 1 and mode <= 2 and _borders.has(1):
		var rseg := _project_segs_globe(mv, _borders[1])
		if rseg.size() >= 2:
			draw_multiline(rseg, Color(0.078, 0.102, 0.149, 0.85), 1.4)
	if _borders.has(2):
		var cseg := _project_segs_globe(mv, _borders[2])
		if cseg.size() >= 2:
			draw_multiline(cseg, Color(0.039, 0.055, 0.086, 0.95), 2.6)
	# NOMS d'empire (toujours, sur le globe : c'est la fonction « se repérer »).
	for c in range(w.country_count()):
		if c >= _country_names.size():
			break
		var nm: String = _country_names[c]
		if nm == "":
			continue
		var sx := 0.0
		var sy := 0.0
		var n := 0
		for r in range(w.region_count()):
			if w.region_owner(r) == c:
				var ctr: Vector2 = w.region_centroid(r)
				if ctr.x >= 0:
					sx += ctr.x
					sy += ctr.y
					n += 1
		if n < 2:
			continue
		var pr: Dictionary = mv.globe_to_screen(sx / n, sy / n)
		if not pr["vis"]:
			continue
		var sp: Vector2 = pr["pos"]
		if sp.x < 50 or sp.y < 60 or sp.x > vp.x - 50 or sp.y > vp.y - 56:
			continue
		var lw := VKit.text_w(nm, VKit.FS_SMALL)
		draw_rect(Rect2(sp.x - lw * 0.5 - 3.0, sp.y - 8.0, lw + 6.0, 15.0), Color(0.03, 0.05, 0.08, 0.72))
		VKit.text(self, Vector2(sp.x - lw * 0.5, sp.y - 7.0), Color(0.97, 0.93, 0.84, 1.0), nm, VKit.FS_SMALL)

## ISO (surface de JEU) — ROUTES & ASSETS posés SUR le relief (mv.iso_pos), la Camera2D met à
## l'échelle. Détail croissant au zoom : rivières → routes → bourg. C'est ici que se lit le jeu.
func _draw_iso(w, mv: Node2D) -> void:
	var zoom := get_viewport_transform().get_scale().x
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	_ensure_terrain(w)            # caches de lumière prêts (routes ombrées comme le terrain)

	# (RIVIÈRES : plus dessinées ici — CARVÉES dans le terrain par iso_blend.gdshader, cf. en-tête.)

	# ── DRESSING : arbres/bosquets/buissons/cailloux/roseaux (DERRIÈRE les villes), cullés au viewport ──
	if zoom >= DECOR_ZOOM_MIN:
		for d in _decor:
			var spr := UIKit.dressing_named(d["name"])
			if spr == null:
				continue
			var ip: Vector2 = mv.iso_pos((d["pos"] as Vector2).x, (d["pos"] as Vector2).y)
			var ss: Vector2 = vt * ip
			if ss.x < -30 or ss.y < -30 or ss.x > vp.x + 30 or ss.y > vp.y + 30:
				continue
			var ts: float = d.get("sz", 10.0)
			var dt: Color = d.get("tint", Color(1, 1, 1, 1))   # calé sur le terrain (relief + sol)
			draw_texture_rect(spr, Rect2(ip - Vector2(ts * 0.5, ts), Vector2(ts, ts)), false, dt)

	# ── FRONTIÈRES (gameplay) : sur le relief iso. Pays + régions selon le mode. ──
	var mode := 0
	if mv.get("mode") != null:
		mode = int(mv.get("mode"))
	if mode >= 1:
		if _borders_dirty:
			_rebuild_borders()
		if mode <= 2 and _borders.has(1):
			var rseg := _project_segs_iso(mv, _borders[1])
			if rseg.size() >= 2:
				draw_multiline(rseg, Color(0.078, 0.102, 0.149, 0.85), 1.6 / zoom)
		if _borders.has(2):
			var cseg := _project_segs_iso(mv, _borders[2])
			if cseg.size() >= 2:
				draw_multiline(cseg, Color(0.039, 0.055, 0.086, 0.95), 3.0 / zoom)

	# ── ROUTES : réseau à jonctions, 3 passes (halo/casing/surface), sur le relief iso.
	#    Croissance organique 1 an/province ; mobilier de bord à la FIN du chantier. ──
	if zoom >= ROAD_ZOOM_MIN:
		_ensure_roads()
		if not _roads.is_empty():
			var year: int = w.year()
			var built := []                       # [{poly:[iso Vector2], w}]
			var done := {}
			for ri in range(_roads.size()):
				var rd: Dictionary = _roads[ri]
				var pts: PackedVector2Array = rd["points"]
				if pts.size() < 2:
					continue
				var st: int = _road_start.get(rd["key"], year)
				var nprov: int = maxi(1, int(rd.get("nprov", 1)))
				var frac := clampf(float(year - st) / float(nprov), 0.0, 1.0)
				var poly := _road_partial(pts, frac)
				if poly.size() >= 2:
					var ipoly := PackedVector2Array()
					var spoly := PackedFloat32Array()
					ipoly.resize(poly.size())
					spoly.resize(poly.size())
					for k in range(poly.size()):
						ipoly[k] = mv.iso_pos(poly[k].x, poly[k].y)
						spoly[k] = _hillshade(poly[k].x, poly[k].y)   # ombrage = celui du terrain
					built.append({"poly": ipoly, "shade": spoly, "w": _road_width(int(rd["level"]), zoom)})
				if frac >= 1.0:
					done[ri] = true
			# RUES PRINCIPALES (toujours bâties) : court tronçon SUD de chaque bourg → la route forcée vers
			# le sud, qui SE FOND dans le réseau (même largeur que les autres) ; rejoint l'ANCRE.
			for ms in _main_streets:
				var msa: Vector2 = ms["a"]
				var mss: Vector2 = ms["s"]
				var msp := PackedVector2Array([mv.iso_pos(msa.x, msa.y), mv.iso_pos(mss.x, mss.y)])
				var msh := PackedFloat32Array([_hillshade(msa.x, msa.y), _hillshade(mss.x, mss.y)])
				built.append({"poly": msp, "shade": msh, "w": _road_width(1, zoom)})
			# 3 passes UNION (casing/fill OPAQUES → les recouvrements FUSIONNENT sans double-assombrir) ;
			# le halo doux est TIGHT et discret (l'ancien +4/zoom α.22 boursouflait/floutait la route).
			for bp in built:
				draw_polyline(bp["poly"], ROAD_SOFT, bp["w"] + 0.10 + 1.4 / zoom, true)
			for bp in built:
				draw_polyline(bp["poly"], ROAD_CASING, bp["w"] + 0.10 + 0.9 / zoom, true)
			for bp in built:
				draw_polyline(bp["poly"], ROAD_FILL, bp["w"], true)
			# RELIEF : ombrage par segment SOUS le même soleil que le terrain (voile chaud au versant
			# éclairé, sombre à l'ombre, par-dessus le fill continu → pas de trou aux jonctions).
			for bp in built:
				var ip2: PackedVector2Array = bp["poly"]
				var sh: PackedFloat32Array = bp.get("shade", PackedFloat32Array())
				if sh.size() != ip2.size():
					continue
				var wln: float = bp["w"]
				for k in range(ip2.size() - 1):
					var f := (sh[k] + sh[k + 1]) * 0.5 - 1.0
					if absf(f) < 0.015:
						continue
					var oc := Color(1.0, 0.95, 0.80, minf(0.40, f * 1.7)) if f > 0.0 \
						else Color(0.12, 0.08, 0.05, minf(0.42, -f * 1.8))
					draw_line(ip2[k], ip2[k + 1], oc, wln * 0.80, true)
			# POUSSIÈRE : mouchetis discret (grain sablonneux) → la chaussée n'est plus lisse.
			for bp in built:
				_road_dust(bp["poly"], bp["w"])
			for d in _road_dress:
				if not done.has(d["road"]):
					continue
				var spr := UIKit.dressing_named(d["name"])
				if spr == null:
					continue
				var ip: Vector2 = mv.iso_pos((d["pos"] as Vector2).x, (d["pos"] as Vector2).y)
				var ds := 4.0
				# petit buisson BAS, CENTRÉ sur le bord SUD de la chaussée : la moitié haute CHEVAUCHE/CACHE
				# le bord, la moitié basse est la marge visible au SUD → effet sud + masquage (pas une tour nord).
				draw_texture_rect(spr, Rect2(ip - Vector2(ds * 0.5, ds * 0.46), Vector2(ds, ds)), false)

	# ── VILLES + BOURG : posés sur le relief iso, triés par PROFONDEUR (y iso). ──
	if zoom >= CITY_ZOOM_MIN:
		if _struct_dirty:
			_build_structures()
			_struct_dirty = false
		var props := []
		for s in _structures:
			var sp2: Vector2 = s["pos"]
			var ip: Vector2 = mv.iso_pos(sp2.x, sp2.y)
			var ss: Vector2 = vt * ip
			if ss.x < -60 or ss.y < -80 or ss.x > vp.x + 60 or ss.y > vp.y + 60:
				continue
			props.append({"d": sp2.x + sp2.y, "city": -1, "s": s, "sp": ip})   # profondeur iso = wx+wy
		for r in range(w.region_count()):
			if w.region_tier(r) < 0:
				continue
			var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))
			if ctr.x < 0:
				continue
			# le centre-ville s'ANCRE au SOMMET BAS de sa tuile (même point que l'entrée de route).
			var aw: Vector2 = ctr
			if mv.has_method("tile_anchor_world"):
				aw = mv.tile_anchor_world(ctr.x, ctr.y)
			props.append({"d": aw.x + aw.y, "city": r, "sp": mv.iso_pos(aw.x, aw.y)})
		props.sort_custom(func(a, b): return a["d"] < b["d"])   # arrière (nord) → avant (sud), profondeur iso
		# PASSE 1 : toutes les ombres SOUS tout (sinon l'ombre d'un asset AVANT mord le sprite d'un ARRIÈRE)
		for prp in props:
			var wd: float
			if prp["city"] < 0:
				wd = float((prp["s"] as Dictionary).get("sz", BLD_SIZE))
			else:
				wd = minf(CITY_CORE_SIZE, float(_region_citymax.get(prp["city"], CITY_CORE_SIZE)))
			_blob_shadow(prp["sp"], wd)
		# PASSE 2 : les sprites par-dessus, dans l'ordre de profondeur
		for prp in props:
			if prp["city"] < 0:
				_draw_struct(prp["s"], prp["sp"])
			else:
				_draw_city(w, prp["city"], prp["sp"])

	# ── ARMÉES : jeton + ligne de marche + anneau de phase, sur le relief iso. ──
	for c in range(w.country_count()):
		var a: Dictionary = w.army_info(c)
		if not bool(a.get("active", false)):
			continue
		var reg: int = a.get("region", -1)
		if reg < 0:
			continue
		var rctr: Vector2 = w.region_centroid(reg)
		if rctr.x < 0:
			continue
		var ctr: Vector2 = mv.iso_pos(rctr.x, rctr.y)
		var col := _country_color(c)
		var phase: int = a.get("phase_id", 0)
		var dest: int = a.get("dest", -1)
		if dest >= 0 and dest != reg:
			var dw: Vector2 = w.region_centroid(dest)
			if dw.x >= 0:
				draw_line(ctr, mv.iso_pos(dw.x, dw.y), Color(_phase_color(phase), 0.7), 0.5)
		var ac := ctr + Vector2(0, -2.0)
		var token := UIKit.army_token(_army_token_name(a))
		if token != null:
			var ts := 26.0
			draw_circle(ac, 2.6, Color(col, 0.95))
			draw_arc(ac, 3.4, 0.0, TAU, 20, Color(_phase_color(phase), 0.95), 1.1, true)
			draw_texture_rect(token, Rect2(ac - Vector2(ts * 0.5, ts * 0.9), Vector2(ts, ts)), false)
		else:
			var s := 4.5
			draw_circle(ac, s + 1.6, Color(_phase_color(phase), 0.85))
			var diamond := PackedVector2Array([
				ac + Vector2(0, -s), ac + Vector2(s, 0),
				ac + Vector2(0, s), ac + Vector2(-s, 0)])
			var bord := PackedVector2Array([
				ac + Vector2(0, -s), ac + Vector2(s, 0),
				ac + Vector2(0, s), ac + Vector2(-s, 0), ac + Vector2(0, -s)])
			draw_polyline(bord, Color(0, 0, 0, 0.9), 1.4, true)
			draw_colored_polygon(diamond, col)

	# ── ÉPICENTRE du cataclysme §27 : anneaux pulsants, sur le relief iso. ──
	var eg: Dictionary = w.endgame_info()
	var epi: int = eg.get("epicenter_reg", -1)
	var fin: int = eg.get("fin", 0)
	_cataclysm = (fin > 0 and epi >= 0)
	if epi >= 0:
		var ew: Vector2 = w.region_centroid(epi)
		if ew.x >= 0:
			var ec: Vector2 = mv.iso_pos(ew.x, ew.y)
			var col := _fin_color(fin)
			var t := Time.get_ticks_msec() / 1000.0
			for k in range(3):
				var rad := 7.0 + k * 6.0 + fmod(t * 5.0, 6.0)
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), 1.0, true)

func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.30, 0.55, 0.95)   # EAU : bleu
		2: return Color(0.80, 0.92, 1.00)   # FROID : blanc glacé
		3: return Color(0.35, 0.70, 0.30)   # RONCES : vert
		_: return Color(0.70, 0.35, 0.85)   # Brèche / indéterminé : violet
