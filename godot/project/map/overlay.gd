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
const STRUCT_SIZE := 12.0    ## hauteur MONDE d'un MONUMENT EDI_* épars (repli ; voir hiérarchie SZ_* ci-dessous)
const DWELL_SIZE := 7.5      ## hauteur MONDE d'une MAISON (faubourg) — plus petite que les monuments
# HIÉRARCHIE DES TAILLES (lisibilité : on voit d'un coup l'important vs le décor). CENTRE(18) >> CIVIC >
# CRAFT > DWELL > FIELD > CLUTTER. Un monument civique IMPOSE, une maison est petite, un champ est bas.
const SZ_CIVIC := 11.0       ## mairie/temple/marché — imposants (juste après le centre-ville)
const SZ_CRAFT := 9.5        ## ateliers — gros, industriels
const SZ_FIELD := 6.0        ## champs/greniers — bas, étalés
# anneau de CENTRES DE TUILES (offsets en tuiles) autour du centre où poser les monuments — déterministe,
# grille-aligné, rayon 2-3 tuiles (dégage l'emprise du centre). Ordre = priorité de pose.
const EDI_RING := [
	Vector2i(2, 0), Vector2i(0, 2), Vector2i(-2, 0), Vector2i(0, -2),
	Vector2i(2, 2), Vector2i(-2, 2), Vector2i(2, -2), Vector2i(-2, -2),
	Vector2i(3, 1), Vector2i(1, 3), Vector2i(-3, -1), Vector2i(-1, -3),
	Vector2i(3, -1), Vector2i(-1, 3), Vector2i(-3, 1), Vector2i(1, -3),
]
# CLUTTER (barils/bûches/charrettes/puits) : anneau PÉRIPHÉRIE du bourg (rayon 3-5 tuiles), hash-déterministe
# + petit jitter sub-tuile (vivant sans être « posé au hasard »). Plus large que EDI_RING (au-delà des monuments).
const CLUTTER_SIZE := 5.0    ## hauteur MONDE d'un prop de clutter (petit)
var _clutter := []           ## [{name, pos(world), sz, flip, tint}]
var _clutter_dirty := true

# ── FALAISES : le RELIEF des highlands (face de roche) est désormais un système de micro-mesh 3D
# (cliff_3d.gd, instancié par iso_ground) rendu dans un SubViewport ortho calé à l'iso. L'overlay n'y
# touche plus ; le shader iso_blend garde le lift + le sommet herbeux + une ombre de contact douce.

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
# FONDATION : dalle de terre battue (diamant iso 2:1) sous chaque centre/bâtiment → l'édifice REPOSE sur
# le terrain (fin du « posé là » : l'art n'a pas de base baked). Teintée au sol local (fond FORT).
const FOUND_DIR := "res://assets/scps/pack/foundations/"
const FOUND_BASE := Color(0.98, 0.95, 0.90, 1.0)    ## terre claire ; la dalle DOIT se voir autour du pied
const GROUND_TINT_FOUND := 0.18                     ## se mêle au sol mais reste LISIBLE (terre battue)
const FOUND_W_MULT := 2.1                            ## dalle nettement plus large que le bâti → les bords débordent
var _found_tex: Texture2D = null


var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre
var _decor := []          ## [{name, pos}] — arbres/forêts (dressing nature), bâti au générate
var _structures := []
var _town_streets := []      ## A1 : squelette de rues des bourgs (pour rendu/clutter)     ## [{name, pos}] — bâti de terrain parsemé autour des villes
var _bio_img: Image = null ## couche biome (cache) → interdit le PIED d'un asset sur une tuile falaise
var _region_variant := {} ## région colonisée → nom de variante de ville TERRAIN (petits bourgs)
var _region_raws := {}    ## région → [{id, name}] : les BRUTES extraites (≤2) — mode carte RESSOURCES (9)
var _raws_dirty := true    ## la production a bougé (an-0 nu → extraction établie) → recache les brutes
var _region_centre := {}  ## région colonisée → TERRAIN du centre-ville (plaine/foret/montagne/estuaire/portuaire/lacustre)
var _region_anchor := {}  ## région colonisée → assise de ville CALÉE SUR TERRE (centroïde snappé + rabat côtier)
var _region_citymax := {} ## région colonisée → plus grande taille de sprite de ville TENANT au sec (anti-débord mer)
var _bk := {}             ## noms de structures triés en bancs (civic/craft/dwell/field), calculé 1×
var _clear_set := {}      ## clairance 0-1 par cellule autour des villes (1 = cœur déboisé -> 0 = lisière) — fondu, pas binaire
var _country_names := []  ## nom de chaque pays (figé au générate) — pour les étiquettes d'empire
var _borders := {}        ## 0 = TRAME FINE (provinces+régions) → PackedVector2Array jittée
# DÉGRADÉ de frontière : un RUBAN par entité, BLENDÉ (N couches du ton EXTÉRIEUR au ton INTÉRIEUR,
# décalées le long de la normale → vrai dégradé, pas deux traits posés). OUTLINE = CULTURE (héritage,
# 6 familles + variation RGB par pays) ; INLINE = ÉTHOS (axe martial↔ordre, fluide). Cités-états or↔argent.
var _b_segs := {}         ## entité → PackedVector2Array : segments de frontière (jittés)
var _b_norm := {}         ## entité → PackedVector2Array : normale vers l'INTÉRIEUR, 1 par segment
var _cap_segs := {}       ## pays → PackedVector2Array : contour de sa CAPITALE (liseré pourpre)
var _cap_norm := {}       ## pays → PackedVector2Array : normale intérieure du contour capitale
# PALETTE de PIGMENTS LIMITÉE (anti-néon) : des encres NATURELLES choisies à la main (terre de Sienne,
# ocre, ardoise, olive…), pas un échantillonnage de la roue HSV (qui donne des bleus/magentas fluo même
# désaturés). On reste dans une gamme TERREUSE compatible parchemin → fin de l'effet cyberpunk.
const PARCHMENT := Color(0.80, 0.72, 0.54)       ## le ton du papier (toutes les teintes y tendent)
const CAP_INK := Color(0.40, 0.24, 0.36)         ## pourpre SOURD (liseré FIN de capitale, pas une bande)
const N_BAND := 5         ## nb de couches du blend (extérieur→intérieur)
const BAND_OUT_PX := 1.0  ## décalage EXTÉRIEUR de la 1re couche (px écran)
const BAND_IN_PX := 4.2   ## décalage INTÉRIEUR de la dernière (px écran)
const LAYER_W_PX := 2.6   ## largeur d'une couche (px écran) — > l'espacement ⇒ les couches se FONDENT
## OUTLINE par HÉRITAGE (6 cultures) : Éso · Métal · Méca · Adapt · Agra · Clan — ENCRES SOMBRES terreuses.
const HERITAGE_PIG := [
	Color(0.31, 0.35, 0.42),   ## Ésotérique  : ardoise (bleu-gris sourd)
	Color(0.46, 0.29, 0.23),   ## Métallurgiste : rouille de fer
	Color(0.48, 0.35, 0.21),   ## Mécaniste   : terre de Sienne brûlée
	Color(0.33, 0.37, 0.25),   ## Adaptatif   : olive
	Color(0.47, 0.39, 0.22),   ## Agraire     : ocre brun
	Color(0.42, 0.28, 0.33),   ## Clanique    : prune sourde
]
## INLINE par ÉTHOS (lavis CLAIR de la même gamme, axe martial↔ordre) : Dom·Hon·Ordre·Bur·Merc·Pac.
const ETHOS_PIG := [
	Color(0.74, 0.57, 0.49),   ## Dominateur  : terre cuite poussiéreuse (martial chaud)
	Color(0.76, 0.61, 0.47),   ## Honneur     : sable chaud
	Color(0.57, 0.63, 0.66),   ## Ordre       : ardoise pâle (ordre froid)
	Color(0.56, 0.65, 0.63),   ## Bureaucrate : céladon terne
	Color(0.78, 0.67, 0.47),   ## Mercantile  : ocre pâle
	Color(0.60, 0.66, 0.61),   ## Pacifiste   : sauge douce
]
const BORDER_JIT := 0.18  ## amplitude du wobble « plume » des frontières d'EMPIRE (unités monde) — continuité
const FINE_JIT   := 0.5   ## wobble PLUS FORT de la trame fine (provinces) → casse l'escalier des arêtes
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
const ROAD_INK    := Color(0.52, 0.40, 0.24, 0.62) ## route : sépia CLAIR (≠ frontières de province NOIRES)
const ROAD_DASH   := 4.5    ## longueur de tiret MOYENNE (px écran) — jittée par tiret (≠ points uniformes)
const ROAD_GAP    := 5.5    ## espace entre tirets (px écran) — trous bien ouverts (carte au trésor)
const ROAD_WOBBLE := 0.7    ## déplacement perpendiculaire par tiret (noise directionnel, unités monde)
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

# ── ROUTES EN TUILES (autotile cardinal, pack « SCPS Full Terrain Tiles ») ──────────────────────
# Chaque cellule-losange traversée par une route reçoit une TUILE plate 256² choisie par le masque
# CARDINAL de ses 4 voisins (n=1,e=2,s=4,w=8). Posée en SPLAT iso ÉLARGI à bord ALPHA-fondu (blend
# TRÈS PROFOND) → les tuiles cardinales-adjacentes se CHEVAUCHENT et FUSIONNENT dans le terrain ; les
# pas diagonaux sont COMBLÉS (cellule intermédiaire) pour qu'aucun lien ne soit seulement diagonal.
const ROADS_IN_SHADER := true    ## les routes sont rendues au niveau TERRAIN (iso_blend) → overlay muet
const DRAW_BRIDGES := true        ## asset pont réparé (alpha OK) → ponts réactivés
const USE_ROAD_TILES := true
const ROUTE_TILE_DIR := "res://assets/scps/pack/iso_tiles/"
const ROUTE_GRID_K := 5          ## DOIT égaler map_view.TILE_K (cellules-monde par losange)
const ROUTE_SURFACE := "road_cobble"
const ROUTE_SPLAT_EXP := 1.6     ## extension des arêtes CONNECTÉES (chevauchement du voisin → continuité)
const ROUTE_CORE_A := 0.96       ## alpha de la route (cœur + arêtes connectées)
var _road_tex := {}              ## masque cardinal 1-15 → Array[Texture2D] (variantes)
var _road_tiles := []            ## [{ctr:Vector2(iso), tex, mask}] — splats précalculés
var _road_tiles_dirty := true    ## le réseau a bougé → recalculer la pose
var _route_meshes := {}          ## masque → ArrayMesh : splat UNIDIRECTIONNEL (plein LE LONG de la route,
                                 ## fondu EN TRAVERS — comme une rivière), bâti/caché par masque

# ── PONTS (kit modulaire RGBA) : là où une route franchit un FLEUVE (le moteur l'y route déjà), on
# pose start→span×N→end en overlay AU-DESSUS de l'eau. Orientation EW (horizontal écran) / NS (vertical)
# selon la direction du franchissement. Sprite 384² centré sur le centre de la tuile-route (losange). ──
const BRIDGE_DIR := "res://assets/scps/pack/bridges/"
var _bridge_tex := {}            ## "ew_start".."ns_end" → Texture2D
var _bridges := []               ## [{tex, tl:Vector2(iso coin haut-gauche), sz:float}]
var _bridges_dirty := true

# ── ROUTE EN COBBLES TRANSPARENTS : la tuile cobble (RGBA épars) est traitée comme une VRAIE TUILE, pas
# un sprite — échantillonnée au niveau TERRAIN (iso_blend) sur le plan du sol (UV losange), donc à l'angle
# iso correct, exactement comme `cliff_atlas`. iso_ground charge la tuile et la passe au shader ; l'overlay
# n'a plus rien à poser pour la route (seuls les PONTS restent en overlay, eux sont au-dessus de l'eau). ──

# biome (couche, valeurs Biome) → NOMS de sprites dressing. FOREST=12 · WOODS=13 · JUNGLE=14
# PREMIER LOT de dressing : chêne ×4 · buisson ×4 · rocher ×4 (découpés des planches de variantes).
# Les milieux non encore couverts (marais, neige, mangrove, tourbière) RETOMBENT sur buisson/rocher
# en attendant les lots suivants (roseaux, sapins enneigés, palétuviers…).
const FOREST_TREES := {
	# variété ÉLARGIE (tout le lot chêne/bouleau/feuillu) → une forêt n'est jamais deux fois la même essence
	12: ["DRESS_OAK_01", "DRESS_OAK_05", "DRESS_OAK_03", "DRESS_OAK_07", "DRESS_OAK_02", "DRESS_LEAF_01", "DRESS_LEAF_03", "DRESS_BIRCH_01"],  # FOREST : chênes + feuillus
	13: ["DRESS_BIRCH_01", "DRESS_BIRCH_02", "DRESS_BIRCH_03", "DRESS_BIRCH_04", "DRESS_OAK_06", "DRESS_LEAF_02", "DRESS_OAK_02"],  # WOODS : bouleaux dominants
	14: ["DRESS_LEAF_01", "DRESS_LEAF_02", "DRESS_LEAF_03", "DRESS_LEAF_04", "DRESS_OAK_08", "DRESS_OAK_04"],  # JUNGLE : feuillus denses
}
const DRESS_OPEN := ["DRESS_OAK_01", "DRESS_OAK_05", "DRESS_LEAF_02", "DRESS_LEAF_04", "DRESS_BIRCH_02", "DRESS_BIRCH_04", "DRESS_BUSH_01", "DRESS_BUSH_03", "DRESS_BUSH_02"]  # plaine : arbres ÉPARS + buissons (plus de variété)
const DRESS_DRY := ["DRESS_BUSH_01", "DRESS_BUSH_02", "DRESS_BUSH_04", "DRESS_ROCK_01", "DRESS_ROCK_02"]               # aride : buissons secs + cailloux
const DRESS_MARSH := ["DRESS_BUSH_01", "DRESS_BUSH_03", "DRESS_BUSH_02", "DRESS_BUSH_04"]                              # (roseaux à venir) → buissons
const DRESS_MANGROVE := ["DRESS_OAK_03", "DRESS_BUSH_03", "DRESS_BUSH_01", "DRESS_BUSH_04"]                            # (palétuviers à venir)
const DRESS_HILL := ["DRESS_PINE_01", "DRESS_ROCK_01", "DRESS_PINE_03", "DRESS_ROCK_04", "DRESS_BUSH_02"]              # collines : cailloux + buissons
const DRESS_CLIFF := ["DRESS_ROCK_01", "DRESS_ROCK_02", "DRESS_ROCK_03", "DRESS_PINE_02", "DRESS_ROCK_04"]             # falaise : rochers
const DRESS_SNOW := ["DRESS_PINE_01", "DRESS_PINE_02", "DRESS_PINE_03", "DRESS_PINE_04", "DRESS_ROCK_04"]                                                # (sapins enneigés à venir) → rochers
const DRESS_RIVER := ["DRESS_LEAF_03", "DRESS_BIRCH_01", "DRESS_ROCK_01", "DRESS_BUSH_03"]                              # berge : cailloux + buissons
const DRESS_STEPPE := ["DRESS_BUSH_02", "DRESS_BUSH_04", "DRESS_BUSH_01", "DRESS_BUSH_03"]                             # steppe : buissons secs
const DRESS_BOG := ["DRESS_BUSH_01", "DRESS_BUSH_03", "DRESS_BUSH_02", "DRESS_BUSH_04"]                                # (mousse/roseaux à venir)

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
		_build_region_raws()            # brutes extraites par région (mode carte RESSOURCES)
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
	_road_tiles_dirty = true    # monde neuf → reposer les tuiles de route
	_bridges_dirty = true       # … et les ponts
	_borders_dirty = true       # monde neuf → frontières ET routes à refaire
	_roads_dirty = true
	_road_start.clear()         # chantiers remis à zéro (le monde neuf rebâtit ses routes)
	_owner_sig = -1
	_build_names()
	_build_anchors()
	_ensure_roads(Sim.world.year() > 0)   # an 0 (monde neuf) ⇒ croît ; an N (save/monde mûr) ⇒ déjà bâtie
	_build_region_raws()        # brutes extraites par région (mode carte RESSOURCES)
	queue_redraw()

## lit le nuage de points (anti-bâti) PUIS sélectionne les fleuves MAJEURS (tracé en ruban).
## Calculé 1× au générate, comme le reste du fil.
func _set_rivers() -> void:
	_rivers = Sim.world.river_points()    # gardé : _build_structures évite de bâtir SUR le fil

## pré-calcule la variante de ville TERRAIN de chaque région colonisée (échantillon
## du biome au centroïde ; l'hydro via le groupe de settlement) — pour les petits bourgs.
func _build_region_raws() -> void:
	_region_raws.clear()
	var w = Sim.world
	if w == null:
		return
	for r in range(w.region_count()):
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var pid: int = w.province_at(int(ctr.x), int(ctr.y))
		if pid < 0:
			continue
		var raws := []
		for line in w.province_income(pid):
			if bool(line.get("manufactured", false)):
				continue                                  # on ne veut QUE la brute extraite
			raws.append({"id": int(line.get("res_id", -1)), "name": String(line.get("source", ""))})
			if raws.size() >= 2:                          # règle moteur : 2 brutes/province
				break
		if not raws.is_empty():
			_region_raws[r] = raws

## MODE RESSOURCES (9) : l'icône de chaque brute extraite, à la tuile (centroïde projeté).
## Sprite si dispo, sinon une PASTILLE nommée (3 lettres) → couverture complète. Taille
## ÉCRAN-CONSTANTE (÷zoom) → lisible à tout niveau de zoom.
func _draw_resources(w, mv: Node2D, is_iso: bool) -> void:
	if _raws_dirty:
		_build_region_raws()         # rebâti à la demande (mode RESSOURCES seulement)
		_raws_dirty = false
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var zoom := maxf(0.01, vt.get_scale().x)
	var sz := 18.0 / zoom if is_iso else 13.0           # MONDE (iso : constant écran) · ÉCRAN (globe)
	for r in range(w.region_count()):
		var raws: Array = _region_raws.get(r, [])
		if raws.is_empty():
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var sp: Vector2
		if is_iso:
			sp = vt * mv.iso_pos(ctr.x, ctr.y)
		else:
			var pr: Dictionary = mv.globe_to_screen(ctr.x, ctr.y)
			if not pr["vis"]:
				continue
			sp = pr["pos"]
		if sp.x < -20 or sp.y < -20 or sp.x > vp.x + 20 or sp.y > vp.y + 20:
			continue
		var n := raws.size()
		for i in range(n):
			var rr: Dictionary = raws[i]
			var off := Vector2((float(i) - float(n - 1) * 0.5) * (sz + 2.0), 0.0)
			var tl := sp + off - Vector2(sz * 0.5, sz * 0.5)
			var spr := UIKit.resource_sprite(int(rr.get("id", -1)), String(rr.get("name", "")))
			if spr != null:
				draw_texture_rect(spr, Rect2(tl, Vector2(sz, sz)), false)
			else:                                          # pas de sprite : pastille nommée (couverture complète)
				draw_rect(Rect2(tl, Vector2(sz, sz)), Color(0.10, 0.11, 0.14, 0.92))
				draw_rect(Rect2(tl, Vector2(sz, sz)), VKit.COL_COPPER, false, 1.0)
				if sz >= 11.0:
					VKit.text(self, tl + Vector2(2.0, sz * 0.5 - 5.0), Color(0.92, 0.86, 0.70), String(rr.get("name", "?")).substr(0, 3), VKit.FS_SMALL)

## terrain du centre-ville (pack centres/) : 6 familles, dérivées de l'hydro/biome/côte.
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
		var r_in := 10.0 + t * 2.0
		var r_out := r_in + 12.0
		var bcx := int(best.x)
		var bcy := int(best.y)
		var rr := int(ceil(r_out))
		for dy in range(-rr, rr + 1):
			for dx in range(-rr, rr + 1):
				var cv := 1.0 - smoothstep(r_in, r_out, sqrt(float(dx * dx + dy * dy)))
				if cv <= 0.0:
					continue
				var ckey := Vector2i(bcx + dx, bcy + dy)
				if cv > float(_clear_set.get(ckey, 0.0)):
					_clear_set[ckey] = cv

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
func _on_tick(_year: int) -> void:
	_struct_dirty = true       # pop & bâtiments ont bougé → le bourg sera reconstruit au prochain dessin zoomé
	_raws_dirty = true         # l'extraction a pu s'établir (an-0 nu) → recache les brutes au prochain dessin RESSOURCES
	_clutter_dirty = true     # le clutter suit le bourg
	_road_tiles_dirty = true   # le réseau a pu croître/changer → reposer les tuiles de route
	_bridges_dirty = true      # … et les ponts (un franchissement neuf)
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
	# 1px : la TRAME FINE — provinces (0) + régions (1), FORTEMENT jittée (casse l'escalier des arêtes).
	var fine := PackedVector2Array()
	fine.append_array(_jitter_segs(w.border_segments(0), FINE_JIT))
	fine.append_array(_jitter_segs(w.border_segments(1), FINE_JIT))
	_borders[0] = fine
	# BLOCS (2) en RUBAN int.→ext. : par ENTITÉ, on bâtit l'INLINE (décalé vers l'intérieur, ton clair)
	# et l'OUTLINE (sur l'arête, ton foncé), le long de la normale extérieure. La façade exclut les côtes
	# d'EMPIRE (le rivage suffit) mais GARDE celles des cités-états (leur ruban or-argent doit se voir).
	_b_segs.clear()
	_b_norm.clear()
	var cd: Dictionary = w.border_segments_col(2)
	var pts: PackedVector2Array = cd.get("pts", PackedVector2Array())
	var nrm: PackedVector2Array = cd.get("nrm", PackedVector2Array())
	var own: PackedInt32Array = cd.get("owner", PackedInt32Array())
	var oth: PackedInt32Array = cd.get("other", PackedInt32Array())
	var role_cache := {}
	for i in range(own.size()):
		var o: int = own[i]
		var ot: int = oth[i] if i < oth.size() else -1
		var n: Vector2 = nrm[i] if i < nrm.size() else Vector2.ZERO   # extérieur DEPUIS own
		var pa: Vector2 = pts[i * 2] + _jit(pts[i * 2])
		var pb: Vector2 = pts[i * 2 + 1] + _jit(pts[i * 2 + 1])
		if not role_cache.has(o):
			role_cache[o] = int(w.country_role(o))
		# ENTITÉ qui colore + DIRECTION de son intérieur : par défaut `own` (intérieur = −normale).
		# Si une CITÉ-ÉTAT est de l'AUTRE côté, c'est ELLE qui colore (intérieur = +normale).
		var entity := o
		var idir := -1.0
		if int(role_cache[o]) != 2 and ot >= 0:
			if not role_cache.has(ot):
				role_cache[ot] = int(w.country_role(ot))
			if int(role_cache[ot]) == 2:
				entity = ot; idir = 1.0
		if not _b_segs.has(entity):
			_b_segs[entity] = PackedVector2Array()
			_b_norm[entity] = PackedVector2Array()
		var s: PackedVector2Array = _b_segs[entity]
		s.push_back(pa); s.push_back(pb); _b_segs[entity] = s
		var nm: PackedVector2Array = _b_norm[entity]
		nm.push_back(n * idir); _b_norm[entity] = nm        # normale vers l'INTÉRIEUR de l'entité
	# CAPITALES : contour de la province-capitale de chaque EMPIRE → liseré POURPRE (au-dessus).
	_cap_segs.clear()
	_cap_norm.clear()
	for c in range(w.country_count()):
		var rl := int(w.country_role(c))
		if rl != 0 and rl != 1:                              # empires (joueur/IA) seulement
			continue
		var creg := int(w.country_capital_region(c))
		if creg < 0:
			continue
		var rc: Dictionary = w.region_border_segments(creg)
		var rp: PackedVector2Array = rc.get("pts", PackedVector2Array())
		var rn: PackedVector2Array = rc.get("nrm", PackedVector2Array())
		if rp.size() < 2:
			continue
		var cs := PackedVector2Array(); var cn := PackedVector2Array()
		for i in range(rn.size()):
			cs.push_back(rp[i * 2] + _jit(rp[i * 2])); cs.push_back(rp[i * 2 + 1] + _jit(rp[i * 2 + 1]))
			cn.push_back(-rn[i])                             # normale extérieure → on stocke l'INTÉRIEURE
		_cap_segs[c] = cs; _cap_norm[c] = cn
	_owner_sig = _owner_signature(w)
	_borders_dirty = false

## assombrit/éclaircit un pigment d'un cheveu (variation par pays SANS sortir de la gamme : on touche
## la VALEUR seulement, jamais la teinte → pas de dérive néon). `dv` ∈ ~[-0.06, +0.06].
func _shade(c: Color, dv: float) -> Color:
	return Color(clampf(c.r + dv, 0.0, 1.0), clampf(c.g + dv, 0.0, 1.0), clampf(c.b + dv, 0.0, 1.0), c.a)

## or/argent FANÉS des cités-états (encres, pas du métal brillant) — dans la même gamme terreuse.
const CS_GOLD := Color(0.62, 0.50, 0.28)         ## or vieilli (extérieur)
const CS_SILVER := Color(0.66, 0.66, 0.62)       ## argent terne (intérieur)

## paire [outline (extérieur, FONCÉ), inline (intérieur, lavis CLAIR)] piochée dans la palette LIMITÉE :
## OUTLINE = encre d'HÉRITAGE (culture) ; INLINE = lavis d'ÉTHOS (axe martial↔ordre) ; cité-état or↔argent.
func _border_pair(e: int) -> Array:
	if e < 0:
		return [Color(0.30, 0.24, 0.18, 0.92), Color(0.55, 0.46, 0.34, 0.85)]
	if int(Sim.world.country_role(e)) == 2:
		return [CS_GOLD, CS_SILVER]
	var h := int(Sim.world.country_heritage(e))
	var jv := (_h1(float(e) * 3.11) - 0.5) * 0.10            # variation par pays : VALEUR seulement (gamme tenue)
	var outline: Color = HERITAGE_PIG[h] if (h >= 0 and h < HERITAGE_PIG.size()) else Color(0.40, 0.32, 0.24)
	var et := int(Sim.world.country_ethos(e))
	var inline: Color = ETHOS_PIG[et] if (et >= 0 and et < ETHOS_PIG.size()) else Color(0.66, 0.58, 0.46)
	return [_shade(outline, jv * 0.6), _shade(inline, jv)]

## RUBAN BLENDÉ : N couches de l'extérieur (outline) à l'intérieur (inline), décalées le long de la
## normale intérieure (px ÉCRAN ÷ zoom). La TEINTE lerp outline→inline ET l'ALPHA RAMPE d'un trait
## d'encre net (extérieur, opaque) à un LAVIS ténu (intérieur) → le parchemin/terrain transparaît, la
## frontière se FOND dans le territoire (jamais un ruban plastique saturé = fin de l'effet néon).
const BAND_A_OUT := 0.90   ## alpha de l'arête extérieure (le trait d'encre net)
const BAND_A_IN := 0.16    ## alpha du bord intérieur (lavis presque effacé → le papier respire)
func _draw_band(mv: Node2D, segs: PackedVector2Array, norms: PackedVector2Array, outline: Color, inline: Color, zoom: float) -> void:
	var nseg := norms.size()
	if nseg < 1:
		return
	for k in range(N_BAND):
		var t := float(k) / float(N_BAND - 1)
		var off := lerpf(-BAND_OUT_PX, BAND_IN_PX, t) / zoom      # extérieur(−) → intérieur(+), en monde
		var col := outline.lerp(inline, t)
		var a := lerpf(BAND_A_OUT, BAND_A_IN, t)                  # encre nette → lavis ténu (le papier transparaît)
		var layer := PackedVector2Array()
		layer.resize(segs.size())
		for i in range(nseg):
			var ni: Vector2 = norms[i]
			layer[i * 2] = segs[i * 2] + ni * off
			layer[i * 2 + 1] = segs[i * 2 + 1] + ni * off
		var proj := _project_segs_iso(mv, layer)
		if proj.size() >= 2:
			draw_multiline(proj, Color(col.r, col.g, col.b, a), LAYER_W_PX / zoom, true)

## LISERÉ de capitale : un SEUL trait FIN pourpre sourd, posé JUSTE à l'intérieur du contour (décalé
## le long de la normale intérieure) — un filet discret, PAS une bande qui prend toute la capitale.
func _draw_cap_lisere(mv: Node2D, segs: PackedVector2Array, norms: PackedVector2Array, zoom: float) -> void:
	var nseg := norms.size()
	if nseg < 1:
		return
	var off := 1.4 / zoom                                   # rentré d'un cheveu (px écran) → le filet borde l'intérieur
	var layer := PackedVector2Array()
	layer.resize(segs.size())
	for i in range(nseg):
		var ni: Vector2 = norms[i]
		layer[i * 2] = segs[i * 2] + ni * off
		layer[i * 2 + 1] = segs[i * 2 + 1] + ni * off
	var proj := _project_segs_iso(mv, layer)
	if proj.size() >= 2:
		draw_multiline(proj, Color(CAP_INK.r, CAP_INK.g, CAP_INK.b, 0.28), 2.2 / zoom, true)  # halo doux
		draw_multiline(proj, Color(CAP_INK.r, CAP_INK.g, CAP_INK.b, 0.85), 1.1 / zoom, true)  # filet net

## offset déterministe ∝ position (hash), amplitude `amt` ; MÊME point monde → MÊME offset (segments
## partagés restent JOINTS, pas de trou). L'effet plume/calligraphie + casse l'escalier des arêtes.
func _jit_a(p: Vector2, amt: float) -> Vector2:
	var a := sin(p.x * 12.9898 + p.y * 78.233) * 43758.5453
	var b := sin(p.x * 39.346 + p.y * 11.135) * 24634.6345
	return Vector2((a - floor(a)) - 0.5, (b - floor(b)) - 0.5) * amt

func _jit(p: Vector2) -> Vector2:
	return _jit_a(p, BORDER_JIT)

func _jitter_segs(segs: PackedVector2Array, amt := BORDER_JIT) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(segs.size())
	for i in range(segs.size()):
		out[i] = segs[i] + _jit_a(segs[i], amt)
	return out

## TRAIT DE PINCEAU : pile de passes translucides (bave d'encre) du LARGE plumé au cœur dense,
## TOUTES antialiasées → feutre le crénelage des arêtes + bord doux = effet brosse. `core_w`/`feather`
## en px ÉCRAN (÷ zoom). Plus de passes larges = halo plus « mouillé ».
func _ink_brush(segs: PackedVector2Array, col: Color, core_w: float, feather: float, zoom: float) -> void:
	var bands := [
		[feather,        0.07],
		[feather * 0.72, 0.13],
		[feather * 0.46, 0.22],
		[feather * 0.26, 0.38],
	]
	for b in bands:
		var ww: float = core_w + float(b[0])
		draw_multiline(segs, Color(col.r, col.g, col.b, col.a * float(b[1])), ww / zoom, true)
	draw_multiline(segs, col, core_w / zoom, true)            # plume nette (cœur)

## découpe une polyligne en TIRETS (pointillé) — phase continue d'un segment à l'autre (suit la
## longueur cumulée). `dash`/`gap` en unités MONDE (l'appelant divise les px écran par le zoom).
func _dash_poly(poly: PackedVector2Array, dash: float, gap: float) -> PackedVector2Array:
	var out := PackedVector2Array()
	if poly.size() < 2 or dash <= 0.0:
		return out
	var period := dash + gap
	if period < 0.001:
		return out
	var d := 0.0                                  # longueur cumulée (abscisse curviligne globale)
	for i in range(poly.size() - 1):
		var a := poly[i]
		var b := poly[i + 1]
		var L := a.distance_to(b)
		if L < 0.0001:
			continue
		var dir := (b - a) / L
		var perp := Vector2(-dir.y, dir.x)
		# parcourt les MULTIPLES de période qui couvrent ce segment [d, d+L] (k croît STRICTEMENT → fini)
		var k := int(floor(d / period))
		while true:
			var ds := float(k) * period          # début du tiret (abscisse globale)
			if ds > d + L:
				break
			# JITTER par tiret (hash de k+position) : longueur VARIÉE + déplacement perpendiculaire
			# (noise directionnel) → trait « tracé à la main », plus des points uniformes.
			var hh := _h1(float(k) * 0.7321 + a.x * 0.013 + a.y * 0.017)
			var hw := _h1(float(k) * 1.9731 + a.x * 0.029 + a.y * 0.011)
			var dlen := dash * (0.45 + 1.1 * hh)  # 0.45..1.55 × dash → tirets de longueurs INÉGALES
			var wob := (hw - 0.5) * ROAD_WOBBLE   # offset perpendiculaire (même des deux bouts → tiret droit décalé)
			var s0 := maxf(ds, d)
			var s1 := minf(ds + dlen, d + L)
			if s1 > s0:
				out.push_back(a + dir * (s0 - d) + perp * wob)
				out.push_back(a + dir * (s1 - d) + perp * wob)
			k += 1
		d += L
	return out

## hash scalaire → [0,1) (déterministe, display-only) — varie tirets/wobble des routes.
func _h1(x: float) -> float:
	var v := sin(x * 12.9898) * 43758.5453
	return v - floor(v)

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
	_roads_dirty = false

## marque les cellules occupées par une route (+ marge 1) → le bourg en spirale les ÉVITE
## (le bâti ne pousse pas sur la chaussée ; les ruelles serpentent ENTRE).
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
func _is_sea_cell(sea: Image, ix: int, iy: int) -> bool:
	if sea == null or ix < 0 or iy < 0 or ix >= sea.get_width() or iy >= sea.get_height():
		return false
	return int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1

## le CHAMP DÉBIT carvé (L8) que le shader iso_blend rend en EAU — récupéré du nœud IsoGround voisin.
## Sert à interdire la BASE d'un asset dans l'eau de RIVIÈRE (la mer/lac est déjà couverte par la couche WATER).
const RIVER_WATER_MIN := 0.26   ## champ ≥ ça = ZONE INTERDITE aux assets : l'eau rendue (shader 0.40) + une
                                ## VRAIE marge de berge (les arbres/bâtis débordaient sur le fleuve) → plus rien dans/au bord du fleuve
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

func _mv_ref() -> Node2D:
	if _mv == null:
		_mv = get_parent() as Node2D
		if _mv != null and _mv.has_signal("mode_changed") and not _mv.mode_changed.is_connected(_on_mode_changed):
			_mv.mode_changed.connect(_on_mode_changed)   # mode RESSOURCES ↔ autre → redraw immédiat (même en pause)
	return _mv

func _on_mode_changed(_m: int) -> void:
	queue_redraw()

# ── projection GLOBE (segments, deux bouts visibles) ───────────────────────────
func _project_segs_iso(mv: Node2D, segs: PackedVector2Array) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(segs.size())
	for i in range(segs.size()):
		out[i] = mv.iso_pos(segs[i].x, segs[i].y)
	return out

# ════════════════════════ dispatch (parchemin, ISO unique) ════════════════════════
func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	var mv := _mv_ref()
	if mv == null:
		return
	_draw_iso(w, mv)
	# MODE RESSOURCES (9) : les icônes de brutes par tuile, AU-DESSUS de tout
	if int(mv.get("mode")) == 9:
		_draw_resources(w, mv, true)

## CARTE PARCHEMIN — acteurs tracés en ENCRE vectorielle (zéro sprite) : frontières, routes,
## villes (glyphes), noms d'empire, armées, épicentre §27. La Camera2D met à l'échelle ; les
## tailles d'encre sont en px ÉCRAN (÷ zoom) → lisibles à tous les zooms.
func _draw_iso(w, mv: Node2D) -> void:
	var zoom := get_viewport_transform().get_scale().x
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var INK := Color(0.20, 0.14, 0.09, 0.95)         # encre brun-sépia (le trait de plume)

	# ── FRONTIÈRES à l'ENCRE (calligraphie) : TRAME FINE 1px (toutes provinces+régions) + BLOCS
	#    d'empire 3px, COULEUR PAR ENTITÉ, en 2 passes (bave d'encre douce + plume nette, jittées). ──
	if _borders_dirty:
		_rebuild_borders()
	# la TRAME FINE fond en survol (sinon mosaïque illisible) et se révèle au plan rapproché — toutes
	# les provinces RESTENT tracées (1px), juste graduées au zoom (LOD ; les blocs d'empire, eux, toujours).
	if _borders.has(0):
		var fine_a := clampf((zoom - 1.6) / 2.4, 0.0, 1.0) * 0.45
		if fine_a > 0.02:
			var fseg := _project_segs_iso(mv, _borders[0])
			if fseg.size() >= 2:
				# 1px provinces NOIRES : fort feutrage (3 passes du large doux au cœur fin) → l'escalier
				# des arêtes se fond (anti-alias), aspect tracé à l'encre, plus de marches.
				var fink := Color(0.07, 0.06, 0.05, fine_a)
				draw_multiline(fseg, Color(fink.r, fink.g, fink.b, fine_a * 0.22), 3.2 / zoom, true)
				draw_multiline(fseg, Color(fink.r, fink.g, fink.b, fine_a * 0.5), 1.8 / zoom, true)
				draw_multiline(fseg, fink, 0.9 / zoom, true)
	# BLOCS : RUBAN BLENDÉ (dégradé extérieur→intérieur). OUTLINE = CULTURE (héritage) ; INLINE = ÉTHOS
	# (martial↔ordre) ; cités-états or↔argent. Puis le LISERÉ POURPRE FIN de chaque capitale, AU-DESSUS.
	for entity in _b_segs:
		var pair := _border_pair(entity)
		_draw_band(mv, _b_segs[entity], _b_norm[entity], pair[0], pair[1], zoom)
	for cc in _cap_segs:
		_draw_cap_lisere(mv, _cap_segs[cc], _cap_norm[cc], zoom)

	# ── ROUTES : POINTILLÉ + trait de PINCEAU, sépia RENFORCÉ à OPACITÉ LIMITÉE (encre sur parchemin).
	#    Croissance organique (1 an/province) ; tous les tirets cumulés → UN seul pinceau (batch). ──
	if zoom >= ROAD_ZOOM_MIN:
		_ensure_roads()
		if not _roads.is_empty():
			var year: int = w.year()
			var alldash := PackedVector2Array()
			for ri in range(_roads.size()):
				var rd: Dictionary = _roads[ri]
				var pts: PackedVector2Array = rd["points"]
				if pts.size() < 2:
					continue
				var st: int = _road_start.get(rd["key"], year)
				var nprov: int = maxi(1, int(rd.get("nprov", 1)))
				var frac := clampf(float(year - st) / float(nprov), 0.0, 1.0)
				var poly := _road_partial(pts, frac)
				if poly.size() < 2:
					continue
				var ipoly := PackedVector2Array()
				ipoly.resize(poly.size())
				for k in range(poly.size()):
					ipoly[k] = mv.iso_pos(poly[k].x, poly[k].y)
				alldash.append_array(_dash_poly(ipoly, ROAD_DASH / zoom, ROAD_GAP / zoom))
			if alldash.size() >= 2:
				# CARTE PIRATE : direction en POINTILLÉ FIN — un seul trait délicat (halo MINUSCULE qui ne
				# comble pas les trous), sépia léger. Allégé (plus de gros web brun).
				draw_multiline(alldash, Color(ROAD_INK.r, ROAD_INK.g, ROAD_INK.b, ROAD_INK.a * 0.22), 2.1 / zoom, true)
				draw_multiline(alldash, ROAD_INK, 1.2 / zoom, true)

	# ── VILLES : glyphe d'encre par région (taille ∝ tier), capitale étoilée. ──
	if zoom >= CITY_ZOOM_MIN:
		for r in range(w.region_count()):
			var tier: int = w.region_tier(r)
			if tier < 0:
				continue
			var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))
			if ctr.x < 0:
				continue
			var ip: Vector2 = mv.iso_pos(ctr.x, ctr.y)
			var ss: Vector2 = vt * ip
			if ss.x < -20 or ss.y < -20 or ss.x > vp.x + 20 or ss.y > vp.y + 20:
				continue
			_draw_town(ip, tier, zoom, INK)

	# ── ARMÉES : jeton vectoriel (losange + anneau de phase) + ligne de marche. ──
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
				draw_line(ctr, mv.iso_pos(dw.x, dw.y), Color(_phase_color(phase), 0.7), 1.4 / zoom)
		var s := 5.0 / zoom
		draw_circle(ctr, s + 1.8 / zoom, Color(_phase_color(phase), 0.9))
		var diamond := PackedVector2Array([
			ctr + Vector2(0, -s), ctr + Vector2(s, 0), ctr + Vector2(0, s), ctr + Vector2(-s, 0)])
		draw_colored_polygon(diamond, col)
		var bord := PackedVector2Array([
			ctr + Vector2(0, -s), ctr + Vector2(s, 0), ctr + Vector2(0, s),
			ctr + Vector2(-s, 0), ctr + Vector2(0, -s)])
		draw_polyline(bord, Color(0.12, 0.09, 0.06, 0.9), 1.2 / zoom, true)

	# ── NOMS D'EMPIRE : SUIVENT LA FORME du pays (axe principal par ACP des centroïdes projetés →
	#    Chili vertical, Russie en travers de la Sibérie), à l'encre, taille ÉCRAN constante. ──
	for c in range(w.country_count()):
		if c >= _country_names.size():
			break
		var nm: String = _country_names[c]
		if nm == "":
			continue
		# centroïdes PROJETÉS (espace écran : iso_pos comprime Y → angle visuel correct)
		var ps := PackedVector2Array()
		for r in range(w.region_count()):
			if w.region_owner(r) == c:
				var rc: Vector2 = w.region_centroid(r)
				if rc.x >= 0:
					ps.push_back(mv.iso_pos(rc.x, rc.y))
		if ps.size() < 2:
			continue
		# moyenne + matrice de covariance → axe principal (ACP 2D)
		var mx := 0.0; var my := 0.0
		for p in ps:
			mx += p.x; my += p.y
		mx /= ps.size(); my /= ps.size()
		var sxx := 0.0; var syy := 0.0; var sxy := 0.0
		for p in ps:
			var dx := p.x - mx; var dy := p.y - my
			sxx += dx * dx; syy += dy * dy; sxy += dx * dy
		var ang := 0.0
		# élongation (rapport des valeurs propres) : on n'oriente QUE les pays nettement allongés
		var tr := sxx + syy
		var det := sxx * syy - sxy * sxy
		var disc := sqrt(maxf(tr * tr * 0.25 - det, 0.0))
		var l1 := tr * 0.5 + disc
		var l2 := tr * 0.5 - disc
		if l2 > 0.001 and l1 / l2 > 1.8:
			ang = 0.5 * atan2(2.0 * sxy, sxx - syy)   # ∈ [-π/2, π/2] : jamais à l'envers
		# ancre = le BARYCENTRE des centroïdes (espace OUVERT, hors des hubs routiers) → lisible.
		var ip := Vector2(mx, my)
		var lw := VKit.text_w(nm, VKit.FS_SMALL)
		# CALLIGRAPHIE : AUCUNE boîte (fond transparent) — encre directe + halo papier, le nom écrit à
		# la plume LE LONG du pays (échelle ÉCRAN constante, un peu agrandie pour la lisibilité).
		var nsc := 1.35 / zoom
		draw_set_transform(ip, ang, Vector2(nsc, nsc))
		VKit.text(self, Vector2(-lw * 0.5 + 0.7, -6.3), Color(0.97, 0.91, 0.74, 0.6), nm, VKit.FS_SMALL)  # halo papier
		VKit.text(self, Vector2(-lw * 0.5, -7.0), Color(0.18, 0.12, 0.07, 0.96), nm, VKit.FS_SMALL)        # encre
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# ── ÉPICENTRE du cataclysme §27 : anneaux pulsants à l'encre de la fin. ──
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
				var rad := (7.0 + k * 6.0 + fmod(t * 5.0, 6.0)) / zoom
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), 1.0 / zoom, true)

## glyphe de ville à l'encre : cercle crème cerné d'encre, taille ∝ tier ; capitale (tier≥4) étoilée.
func _draw_town(ip: Vector2, tier: int, zoom: float, ink: Color) -> void:
	var cream := Color(0.94, 0.89, 0.74, 1.0)
	var r := lerpf(2.2, 6.5, clampf(float(tier) / 5.0, 0.0, 1.0)) / zoom
	draw_circle(ip, r, ink)
	draw_circle(ip, r - 1.2 / zoom, cream)
	draw_circle(ip, r * 0.34, ink)
	if tier >= 4:
		draw_arc(ip, r + 2.0 / zoom, 0.0, TAU, 28, ink, 1.0 / zoom, true)
		for k in range(4):
			var ang := float(k) * (PI / 2.0)
			var d := Vector2(cos(ang), sin(ang))
			draw_line(ip + d * (r + 0.6 / zoom), ip + d * (r + 4.0 / zoom), ink, 1.2 / zoom, true)

func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.30, 0.55, 0.95)   # EAU : bleu
		2: return Color(0.80, 0.92, 1.00)   # FROID : blanc glacé
		3: return Color(0.35, 0.70, 0.30)   # RONCES : vert
		_: return Color(0.70, 0.35, 0.85)   # Brèche / indéterminé : violet
