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
const SettlementStamps = preload("res://map/settlement_stamps.gd")  ## tampons de peuplement (lot 1, atlas)
const PHASE_MARCH := 1
const PHASE_SIEGE := 2
const PHASE_BATTLE := 3
const LAYER_WATER := 4       ## SCPS_LAYER_WATER : masque mer OU LAC (≥1 = eau) — l'assise des
                             ## bourgs tient l'EAU COMPLÈTE (les lacs intérieurs, ignorés par SEA seul)
const LAYER_SEA := 1         ## SCPS_LAYER_SEA : mer SALÉE seule (≥1) — pour distinguer LAC (eau douce) de MER
# SIÈGE du tampon — les humains habitent près de l'EAU DOUCE (rivière/lac), sinon le RIVAGE (mer), décalé
# vers l'INTÉRIEUR. Rayons de recherche (cellules) depuis le centroïde + décalage inland du siège.
const SEAT_FRESH_R := 11     ## cherche une eau DOUCE (lac/rivière) à ≤ ce rayon → ville au bord
const SEAT_FRESH_INLAND := 2 ## siège à ~2 cellules en retrait de l'eau douce (pas dans l'eau)
const SEAT_SEA_R := 13       ## sinon, cherche la MER à ≤ ce rayon → ville côtière
const SEAT_SEA_INLAND := 4   ## siège plus en retrait du rivage marin (vers l'intérieur/centre)
## Seuils de zoom ISO (px/unité-monde de la Camera2D). L'ISO est la surface de JEU : on y montre
## ROUTES & ASSETS (bourg). L'entrée en ISO est à zoom ≈ ISO_FAR (4.0) → assets déjà lisibles.
const CITY_ZOOM_MIN := 3.5   ## villes + bourg
const DECOR_ZOOM_MIN := 3.0  ## forêts/arbres + DRESSING de terrain (lot 2)
# ── DRESSING DE TERRAIN (lot 2, marques peintes) : semé par BIOME, display-only, SOUS frontières/villes,
# basse densité, taille ÉCRAN constante. Biome (couche LAYER_BIOME, enum Biome de scps_types.h) → ids.
const LAYER_BIOME := 2       ## SCPS_LAYER_BIOME : index de biome par cellule
const DRESS_SPACING := 9     ## pas de la grille de semis (cellules) — DENSIFIÉ (trame continue, pas des stickers)
const DRESS_ALPHA := 0.50    ## opacité FADE — translucides : le parchemin transparaît, elles se FONDENT
                             ## (chevauchement des marques denses → trame qui s'auto-construit, pas « posé là »)
const DRESS_BY_BIOME := {
	# RELIEF (lot 2)
	18: ["mountain_range_01", "mountain_range_02", "mountain_single_01", "mountain_single_02", "mountain_pass_01"],  # MONTAGNES
	19: ["mountain_single_01", "mountain_single_02", "mountain_range_01"],   # PIC
	23: ["mountain_single_01", "rocky_outcrop_01"],                          # VOLCAN
	16: ["hill_cluster_01", "hill_mark_01", "mountain_single_02", "forest_mass_conifer_01"],  # HAUTES-TERRES (lot 5 : sapinière)
	17: ["hill_mark_01", "hill_cluster_01", "rocky_outcrop_01"],             # COLLINES
	# FORÊTS (lot 2 + MASSES lot 5 — les grandes canopées peintes KCD dominent, les arbres isolés accentuent)
	12: ["forest_mass_broadleaf_01", "forest_mass_broadleaf_02", "forest_mass_mixed_01", "forest_dense_01", "tree_broadleaf_01"],  # FORÊT
	13: ["forest_mass_mixed_01", "forest_edge_01", "forest_sparse_01", "tree_broadleaf_01", "tree_pine_01"],                       # BOIS
	14: ["forest_mass_broadleaf_01", "forest_mass_broadleaf_02", "forest_dense_01", "tree_broadleaf_01"],                          # JUNGLE
	# PLAINES / PRAIRIE (lot 3 — herbe peinte ; comblent les biomes plats jadis NUS)
	4:  ["plain_grass_01", "plain_grass_02", "plain_sparse_tufts_01", "plain_wind_strokes_01"],  # PLAINES
	5:  ["plain_sparse_tufts_01", "plain_grass_02"],                         # CHAMPS (épars, pas de cultures)
	6:  ["plain_grass_01", "plain_grass_02", "plain_sparse_tufts_01"],       # PRAIRIE
	# STEPPE / SÈCHE (lot 3 + lot 2)
	7:  ["steppe_grass_01", "steppe_grass_02", "steppe_dry_strokes_01", "steppe_tufts_01"],      # STEPPE
	9:  ["steppe_dry_strokes_01", "steppe_tufts_01", "scrub_brush_01", "rocky_outcrop_01"],      # TERRES SÈCHES
	8:  ["savanna_grass_01", "savanna_grass_02", "savanna_sparse_tree_01", "acacia_mark_01"],    # SAVANE
	# DÉSERTS (lot 2)
	10: ["dune_wind_lines_01"],                                             # DÉSERT
	11: ["dune_wind_lines_01"],                                             # DÉSERT CÔTIER
	# ZONES HUMIDES (lot 3)
	15: ["marsh_reeds_01", "marsh_reeds_02", "marsh_tufts_01", "marsh_ripple_reeds_01"],  # MARAIS
	21: ["marsh_reeds_01", "marsh_reeds_02", "tree_broadleaf_01"],          # MANGROVE
	22: ["marsh_reeds_02", "marsh_tufts_01", "marsh_ripple_reeds_01"],      # TOURBIÈRE
	20: ["tree_pine_01", "forest_mass_conifer_01", "rocky_outcrop_01"],    # GLACIER (sapins/cailloux épars ; lot 5 : sapinière)
	# EAU — MOUVEMENT seul (lot 3) : rides, houle, courants ; jamais un aplat
	0:  ["ocean_swell_lines_01", "ocean_current_swirl_01"],                # OCÉAN PROFOND (très épars)
	1:  ["sea_ripples_01", "sea_ripples_02", "ocean_swell_lines_01"],      # OCÉAN
	2:  ["sea_ripples_01", "sea_ripples_02"],                              # HAUT-FOND
}
## probabilité de poser une marque par biome (densité) — DENSIFIÉ (trame continue) ; eau reste ÉPARSE.
const DRESS_DENSITY := {
	4: 0.65, 5: 0.38, 6: 0.68,                          # plaines/prairie : herbe DENSE (jadis trop nue)
	7: 0.70, 8: 0.62, 9: 0.68, 10: 0.80, 11: 0.72,
	12: 0.95, 13: 0.90, 14: 0.95, 15: 0.88, 21: 0.72, 22: 0.82,
	16: 0.85, 17: 0.80, 18: 0.96, 19: 0.92, 23: 0.82, 20: 0.52,
	0: 0.07, 1: 0.18, 2: 0.20,                          # eau : épars (mouvement seul)
}
## PASSES SUPPLÉMENTAIRES par biome (marques EN PLUS par cellule de grille) → CANOPÉE dense. Surtout les
## forêts (le « densifié » demandé) : 1 + N marques jittées par cellule → couvert continu, pas des arbres isolés.
const DRESS_EXTRA := {
	12: 3, 14: 3, 13: 2,        # FORÊT/JUNGLE/BOIS : canopée dense (3-4 marques/cellule)
	18: 1, 19: 1, 16: 1,        # relief : un peu plus fourni
}
## LOT 4 — easter eggs RARES (serpents de mer, épaves, récifs, lapins) : placés par une passe à GROS pas.
const EGG_SPACING := 46      ## grille grossière (rare)
const EGG_ALPHA := 0.85      ## moins fadé que le dressing (ce sont des « figures », pas de la trame)
const EGG_WRECKS := ["shipwreck_hull_01", "broken_mast_01", "half_sunk_wreck_01", "floating_debris_01",
	"jagged_reef_01", "low_rocks_01", "sea_stacks_01", "shoal_stones_01"]
const EGG_RABBITS := ["apoc_rabbit_banner_01", "apoc_rabbit_horn_01", "apoc_rabbit_spear_01", "apoc_rabbit_crown_01"]
# TAMPONS de peuplement (lot 1) : taille à l'ÉCRAN (px), par tier t1→t7. Réduite (les tampons doivent
# PONCTUER la carte, pas l'écraser). Constante à l'écran (÷zoom au tracé) → lisible à tous les zooms.
const STAMP_PX_MIN := 22.0   ## tier 1 (hameau) — petit (agrandi légèrement)
const STAMP_PX_MAX := 48.0   ## tier 7 (capitale couronnée) — le plus gros (agrandi légèrement)
const STAMP_ALPHA := 0.90    ## léger FADE des tampons de ville (blend dans le parchemin sans perdre en lisibilité)
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
var _region_label := {}   ## région → NOM du siège (bannière de lieu KCD, cache paresseux)
var _region_centre := {}  ## région colonisée → TERRAIN du centre-ville (plaine/foret/montagne/estuaire/portuaire/lacustre)
var _region_anchor := {}  ## région colonisée → assise de ville CALÉE SUR TERRE (centroïde snappé + rabat côtier)
var _region_citymax := {} ## région colonisée → plus grande taille de sprite de ville TENANT au sec (anti-débord mer)
var _region_seat := {}    ## région colonisée → SIÈGE du tampon : cellule INTÉRIEURE de province (jamais sur une jonction)
var _stamp_tex := {}      ## id de tampon → Texture2D (cache ; chargé paresseux, fallback Image.load)
var _dress_tex := {}      ## id de marque de terrain (lot 2) → Texture2D (cache)
var _dressing := []       ## [{pos(monde), id, scale}] — marques de biome semées (display-only)
var _dressing_dirty := true ## la géo a changé (génération/chargement) → re-semer le dressing
var _dress_clear := []    ## [[Vector2, r²]] — la CLAIRIÈRE des bourgs (aucune marque dedans)
var nature_mode := false  ## MODE NATURE : on ne montre QUE le terrain + le dressing (pas de frontières/
                          ## villes/routes/armées/noms) — la carte « vierge », touche N. Display-only.
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
# FRONTIÈRES en ENCRE GRAVÉE (atlas, pas grille de jeu) : la PROVINCE est un cheveu brun sombre
# (administrative, discrète) ; le PAYS est une DOUBLE PASSE — halo brun très sombre LARGE (le « creux
# gravé ») + pigment politique FIN dessus. Hiérarchie : province émerge, pays domine, sans noir massif.
const PROV_INK := Color(0.165, 0.141, 0.098)     ## #2a2419 brun sombre — frontière de PROVINCE (gravée, ~35 %)
const POL_HALO := Color(0.090, 0.067, 0.043)     ## #17110b brun très sombre — halo LARGE sous le trait de PAYS
# ── ÉPAISSEUR DE TRAIT ADAPTATIVE AU ZOOM (inspirée de CK3) ────────────────────────────────────────
# CK bake ses bordures en géométrie MONDE (elles GROSSISSENT à l'écran quand la caméra descend) puis les
# fond par un shader d'opacité SÉPARÉ (camera.fxh / pdxverticalborder). Un overlay 2D ne peut pas rebaker
# une géométrie par frame → on imite le RENDU : `_w(zoom, base, min, max)` = clamp(base·zoom, min, max)/zoom.
#   - zoom OUT  : plancher min_px → le trait ne DISPARAÎT jamais au plan large.
#   - APPROCHE  : largeur = base (CONSTANTE en monde) → le trait est SOUDÉ au terrain et s'épaissit à l'écran.
#   - zoom IN   : plafond max_px → au zoom profond une bordure n'AVALE jamais une province.
# L'opacité (fondu de la trame fine, gates routes/villes) reste la couche de visibilité INDÉPENDANTE (CK).
# HIÉRARCHIE par asymétrie de rails : la bande d'EMPIRE respire fort (toujours lisible) ; la trame de
# provinces reste un cheveu (≤1.3px) gouvernée par son fondu → l'empire DOMINE, la province ÉMERGE.
# largeurs zoom-adaptatives (_w) du trait de PAYS : halo LARGE + pigment FIN (la province reste un cheveu).
const POL_HALO_BASE := 1.1   ## halo de pays : épaisseur MONDE (≈ 2.6→4.6 px écran selon le zoom)
const POL_HALO_MIN  := 2.6   ## plancher px (survit au plan large)
const POL_HALO_MAX  := 4.6   ## plafond px (large mais jamais une dalle)
const POL_PIG_BASE  := 0.55  ## pigment politique FIN par-dessus le halo (≈ 1.4→2.4 px écran)
const POL_PIG_MIN   := 1.4
const POL_PIG_MAX   := 2.4
## OUTLINE par HÉRITAGE (6 cultures) : Éso · Métal · Méca · Adapt · Agra · Clan — ENCRES SOMBRES terreuses.
const HERITAGE_PIG := [
	Color(0.31, 0.35, 0.42),   ## Ésotérique  : ardoise (bleu-gris sourd)
	Color(0.46, 0.29, 0.23),   ## Métallurgiste : rouille de fer
	Color(0.48, 0.35, 0.21),   ## Mécaniste   : terre de Sienne brûlée
	Color(0.33, 0.37, 0.25),   ## Adaptatif   : olive
	Color(0.47, 0.39, 0.22),   ## Agraire     : ocre brun
	Color(0.42, 0.28, 0.33),   ## Clanique    : prune sourde
]
# LISSAGE des frontières (escaliers → courbes) : on casse la FRÉQUENCE de l'escalier (ré-échantillonnage
# + passe-bas TAUBIN), PUIS on arrondit (Chaikin). ⚠ Taubin et NON Laplacien pur : le Laplacien RÉTRÉCIT
# les boucles vers leur centre (cumulatif) → les frontières dérivaient de leur vraie ligne et BULGEAIENT
# par-dessus les VILLES (« placement avalé »). Taubin alterne un pas adoucissant (λ) et un pas regonflant
# (μ) → lisse SANS rétrécir : la frontière reste sur la diagonale MOYENNE de l'escalier = sa vraie ligne.
const SMOOTH_RESAMPLE := 2.0  ## pas de ré-échantillonnage (cellules) — casse la fréquence SANS écraser la forme
const SMOOTH_TAUBIN := 6      ## itérations Taubin λ|μ (passe-bas non-rétrécissant)
const SMOOTH_CHAIKIN := 2     ## passes de corner-cutting (arrondi final de la courbe)
const TAUBIN_LAMBDA := 0.5    ## pas adoucissant (>0)
const TAUBIN_MU := -0.53      ## pas regonflant (<0, |μ|>λ) → compense le rétrécissement du pas λ
var _borders_dirty := true ## la souveraineté a bougé (conquête/colonisation) → refaire les frontières
var _owner_sig := -1      ## signature de la photo des propriétaires → détecte le changement de souveraineté
# ── LAVIS POLITIQUE (aquarelle de territoire) : owner/cellule teinté au PIGMENT d'entité (une seule
# famille de couleur : lavis = frontière = armée = nom), bâti en C++ (political_image), rebâti avec les
# frontières (même signal de souveraineté). Fort au plan LARGE (la lecture politique du fit), s'efface
# vers le zoom profond (le terrain parle). Transparent hors territoire — le parchemin transparaît. ──
var _pol_tex: ImageTexture = null
## DEUX RÉGIMES (références de DA) : dézoomé = EU4 — le POLITIQUE domine (aplats presque
## pleins, grands noms) ; zoomé = KCD — le TERRAIN domine (politique quasi absent, bannières
## de lieux). La bascule vit ici (lavis) + dans les noms (fondu) + les bannières (éclosion).
const WASH_A_FAR  := 0.72  ## aplat politique au plan large (EU4 : on lit un atlas politique)
const WASH_A_NEAR := 0.06  ## ... au plan rapproché (KCD : le parchemin terrain reprend tout)
const WASH_FADE_LO := 1.8  ## zoom où l'aplat commence à céder
const WASH_FADE_HI := 4.5  ## zoom où le terrain a (presque) tout repris
# ── SÉLECTION : contour DORÉ de la province choisie (le grain de panneau, charte EU4) ──
var _sel_prov_cache := -2
var _sel_segs := PackedVector2Array()
const SEL_GOLD := Color(0.86, 0.68, 0.26)   ## or de sélection (net, au-dessus du creux d'encre)
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
# CHEMIN DE TERRE À 3 TRAITS (le motif cartographique classique — KCD/atlas) : un
# sous-trait sépia sombre (l'ombre du creux), le corps CRÈME pâle (la terre battue),
# un filet clair central. Le pointillé « carte au trésor » s'émiettait au zoom.
const ROAD_EDGE  := Color(0.46, 0.31, 0.17, 0.42)  ## sous-trait : sépia sombre, large
const ROAD_MAIN  := Color(0.957, 0.855, 0.655, 0.92) ## corps : crème (terre battue)
const ROAD_LIGHT := Color(1.0, 0.965, 0.835, 0.45)  ## filet central clair
const ROAD_MINOR_EDGE := Color(0.46, 0.31, 0.17, 0.30) ## desserte : plus ténue
const ROAD_MINOR_MAIN := Color(0.957, 0.855, 0.655, 0.72)
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
	_dressing_dirty = true      # … et le dressing de terrain (biome semé)
	_roads_dirty = true
	_road_start.clear()         # chantiers remis à zéro (le monde neuf rebâtit ses routes)
	_region_label.clear()       # bannières de lieux : noms recachés (monde neuf)
	_town_cache.clear()         # urbaniste : plans de bourgs recalculés (routes neuves)
	_sea_img = null             # couches eau recachées (quais)
	_rf_img = null
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
	_region_seat.clear()
	var w = Sim.world
	if w == null:
		return
	var sea: Image = w.layer_image(LAYER_WATER)   # mer OU lac : 0 = terre, ≥ 1 = eau
	var seaonly: Image = w.layer_image(LAYER_SEA) # mer SALÉE seule → distingue le lac (eau douce)
	var rf: Image = _carved_river_field()         # champ de débit des rivières (eau douce ; peut être null)
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		var rl: int = int(w.country_role(w.region_owner(r))) if w.region_owner(r) >= 0 else -1
		if t < 0 and rl != 2 and rl != 4:
			continue                              # wilderness sans ville (mais on garde cité-état/hameau libre)
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
		_region_seat[r] = _find_seat(w, sea, seaonly, rf, r, ctr)  # SIÈGE : près de l'eau douce → rivage, ≠ jonction
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

## SIÈGE du tampon — où les HUMAINS habiteraient : au bord de l'EAU DOUCE (rivière/lac) si elle est proche,
## sinon sur le RIVAGE marin, décalé vers l'INTÉRIEUR ; à défaut, au cœur intérieur de la région. Toujours
## sur TERRE, dans la région r, de préférence INTÉRIEUR de province (≠ jonction). Le centroïde ne sert que
## d'ORIGINE de recherche / dernier repli (sa position brute tombe pile sur une jonction).
func _find_seat(w, water: Image, seaonly: Image, rf: Image, r: int, c0: Vector2) -> Vector2:
	if water == null:
		return c0
	var cx := int(round(c0.x))
	var cy := int(round(c0.y))
	# 1) EAU DOUCE (lac ou rivière) proche → ville au bord, léger retrait (jamais SUR l'eau).
	var fresh := _nearest_water(water, seaonly, rf, cx, cy, SEAT_FRESH_R, true)
	if fresh.x >= 0:
		var s1 := _seat_inland(w, water, rf, r, fresh, c0, SEAT_FRESH_INLAND)
		if s1.x >= 0:
			return s1
	# 2) sinon MER → ville côtière, retrait plus marqué vers l'intérieur.
	var salt := _nearest_water(water, seaonly, rf, cx, cy, SEAT_SEA_R, false)
	if salt.x >= 0:
		var s2 := _seat_inland(w, water, rf, r, salt, c0, SEAT_SEA_INLAND)
		if s2.x >= 0:
			return s2
	# 3) intérieur : cellule de r, intérieure de province, la plus proche du centroïde.
	return _interior_seat(w, water, rf, r, c0)

## VRAI si (x,y) est de l'EAU DOUCE (lac = eau mais pas mer, OU rivière) quand `fresh`, sinon de la MER.
func _water_match(water: Image, seaonly: Image, rf: Image, x: int, y: int, fresh: bool) -> bool:
	var is_water := int(water.get_pixel(x, y).r * 255.0 + 0.5) >= 1
	var is_sea := seaonly != null and int(seaonly.get_pixel(x, y).r * 255.0 + 0.5) >= 1
	if fresh:
		var is_lake := is_water and not is_sea
		var is_river := rf != null and x < rf.get_width() and y < rf.get_height() and rf.get_pixel(x, y).r >= RIVER_WATER_MIN
		return is_lake or is_river
	return is_sea

## cellule d'eau la plus proche de (cx,cy) (≤ maxrad) qui matche eau-douce/mer ; (-1,-1) si aucune.
func _nearest_water(water: Image, seaonly: Image, rf: Image, cx: int, cy: int, maxrad: int, fresh: bool) -> Vector2:
	var sw := water.get_width()
	var sh := water.get_height()
	for R in range(1, maxrad + 1):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if absi(dx) != R and absi(dy) != R:
					continue                            # bord d'anneau seulement (du plus proche au plus loin)
				var x := cx + dx
				var y := cy + dy
				if x < 0 or y < 0 or x >= sw or y >= sh:
					continue
				if _water_match(water, seaonly, rf, x, y, fresh):
					return Vector2(x, y)
	return Vector2(-1, -1)

## pose le siège sur TERRE SÈCHE de la région r, à ~`inland` cellules de l'eau, vers le centroïde.
func _seat_inland(w, water: Image, rf: Image, r: int, water_pt: Vector2, c0: Vector2, inland: float) -> Vector2:
	var dir := c0 - water_pt
	if dir.length() < 0.5:
		dir = Vector2(0, -1)
	dir = dir.normalized()
	var target := water_pt + dir * inland
	return _snap_region_land(w, water, rf, r, target)

## VRAI si (x,y) est de la TERRE SÈCHE : ni mer/lac (LAYER_WATER), ni RIVIÈRE (champ rf). ⚠ les rivières
## ne sont PAS dans LAYER_WATER → sans ce test on poserait le tampon EN PLEIN MILIEU du fleuve.
func _is_dry_land(water: Image, rf: Image, x: int, y: int) -> bool:
	if int(water.get_pixel(x, y).r * 255.0 + 0.5) >= 1:
		return false                                    # mer ou lac
	return not _in_river_water(rf, x, y)                # ni rivière

## cellule de TERRE SÈCHE de r la plus proche de `target`, de préférence INTÉRIEURE de province ; (-1,-1) sinon.
func _snap_region_land(w, water: Image, rf: Image, r: int, target: Vector2) -> Vector2:
	var sw := water.get_width()
	var sh := water.get_height()
	var tx := int(round(target.x))
	var ty := int(round(target.y))
	var fallback := Vector2(-1, -1)
	for R in range(0, 10):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if R > 0 and absi(dx) != R and absi(dy) != R:
					continue
				var x := tx + dx
				var y := ty + dy
				if x < 1 or y < 1 or x >= sw - 1 or y >= sh - 1:
					continue
				if not _is_dry_land(water, rf, x, y):
					continue                            # eau (mer/lac/rivière)
				var pv: int = w.province_at(x, y)
				if pv < 0 or w.province_region(pv) != r:
					continue                            # hors région r
				if w.province_at(x - 1, y) == pv and w.province_at(x + 1, y) == pv \
				   and w.province_at(x, y - 1) == pv and w.province_at(x, y + 1) == pv:
					return Vector2(x, y)                # intérieur de province
				if fallback.x < 0:
					fallback = Vector2(x, y)
	return fallback

## cœur INTÉRIEUR de la région (cellule SÈCHE de r, intérieure de province, la + proche du centroïde). Dernier repli.
func _interior_seat(w, water: Image, rf: Image, r: int, c0: Vector2) -> Vector2:
	var sw := water.get_width()
	var sh := water.get_height()
	var cx := int(round(c0.x))
	var cy := int(round(c0.y))
	var fallback := Vector2(-1, -1)
	for R in range(0, 14):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if R > 0 and absi(dx) != R and absi(dy) != R:
					continue
				var x := cx + dx
				var y := cy + dy
				if x < 1 or y < 1 or x >= sw - 1 or y >= sh - 1:
					continue
				if not _is_dry_land(water, rf, x, y):
					continue
				var pv: int = w.province_at(x, y)
				if pv < 0 or w.province_region(pv) != r:
					continue
				if w.province_at(x - 1, y) == pv and w.province_at(x + 1, y) == pv \
				   and w.province_at(x, y - 1) == pv and w.province_at(x, y + 1) == pv:
					return Vector2(x, y)
				if fallback.x < 0:
					fallback = Vector2(x, y)
	if fallback.x >= 0:
		return fallback
	return _snap_to_land(water, c0)

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

## signature de la photo des propriétaires → détecte conquête/colonisation. Le compte de
## provinces COLONISÉES y entre : une colonisation INTRA-région ne bouge pas l'owner agrégé
## de région — sans lui, le lavis/frontières (grain PROVINCE, charte) ne se rebâtiraient pas.
func _owner_signature(w) -> int:
	if w == null:
		return -1
	var sig := 0
	for r in range(w.region_count()):
		sig = (sig * 1000003 + (w.region_owner(r) + 2)) & 0x3fffffff
	if w.has_method("colonized_total"):
		sig = (sig * 1000003 + int(w.colonized_total())) & 0x3fffffff
	return sig

## reconstruit les segments de frontière (région + pays) depuis la façade (port bseg).
func _rebuild_borders() -> void:
	var w = Sim.world
	if w == null:
		return
	# TRAME FINE (provinces 0 + régions 1) : SEULEMENT là où la civilisation touche — un joint
	# dont les DEUX rives sont vierges (owner<0 et other<0) n'apprend rien au joueur et noyait
	# la carte sous un filet de « boue craquelée » sur toute la terre sauvage. Puis CHAÎNÉE en
	# polylignes ordonnées et lissée → courbes (fin de l'escalier).
	var fine_raw := PackedVector2Array()
	for lvl in [0, 1]:
		var fd: Dictionary = w.border_segments_col(lvl)
		var fp: PackedVector2Array = fd.get("pts", PackedVector2Array())
		var fo: PackedInt32Array = fd.get("owner", PackedInt32Array())
		var ft: PackedInt32Array = fd.get("other", PackedInt32Array())
		for i in range(fo.size()):
			if fo[i] < 0 and (i >= ft.size() or ft[i] < 0):
				continue                                   # terre vierge des deux côtés : muette
			fine_raw.push_back(fp[i * 2]); fine_raw.push_back(fp[i * 2 + 1])
	var fine := PackedVector2Array()
	for ch in _chain_segments(fine_raw):
		var poly: PackedVector2Array = _smooth_poly(ch)
		for i in range(poly.size() - 1):
			fine.push_back(poly[i]); fine.push_back(poly[i + 1])
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
	var ent_flat := {}    # entité → paires BRUTES (non jittées, entières → chaînables par sommet partagé)
	var ent_nrm := {}     # entité → normale INTÉRIEURE par segment (parallèle à ent_flat)
	for i in range(own.size()):
		var o: int = own[i]
		var ot: int = oth[i] if i < oth.size() else -1
		var n: Vector2 = nrm[i] if i < nrm.size() else Vector2.ZERO   # extérieur DEPUIS own
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
		if not ent_flat.has(entity):
			ent_flat[entity] = PackedVector2Array(); ent_nrm[entity] = PackedVector2Array()
		var ef: PackedVector2Array = ent_flat[entity]
		ef.push_back(pts[i * 2]); ef.push_back(pts[i * 2 + 1]); ent_flat[entity] = ef
		var en: PackedVector2Array = ent_nrm[entity]
		en.push_back(n * idir); ent_nrm[entity] = en        # normale vers l'INTÉRIEUR de l'entité
	# CHAÎNE + Chaikin chaque entité → ruban en COURBES (normale intérieure recalculée le long du tracé).
	for entity in ent_flat:
		var r := _smooth_border(ent_flat[entity], ent_nrm[entity])
		_b_segs[entity] = r[0]; _b_norm[entity] = r[1]
	# CAPITALES : contour de la PROVINCE-capitale de chaque EMPIRE → liseré POURPRE (au-dessus).
	# Grain PROVINCE (charte) : jadis le contour de toute la RÉGION-siège — incohérent depuis
	# que la carte montre la propriété par province (le liseré entourait de la terre vierge).
	_cap_segs.clear()
	_cap_norm.clear()
	for c in range(w.country_count()):
		var rl := int(w.country_role(c))
		if rl != 0 and rl != 1:                              # empires (joueur/IA) seulement
			continue
		var rc: Dictionary
		if w.has_method("country_capital_province") and w.has_method("province_border_segments"):
			var cpp := int(w.country_capital_province(c))
			if cpp < 0:
				continue
			rc = w.province_border_segments(cpp)
		else:
			var creg := int(w.country_capital_region(c))
			if creg < 0:
				continue
			rc = w.region_border_segments(creg)
		var rp: PackedVector2Array = rc.get("pts", PackedVector2Array())
		var rn: PackedVector2Array = rc.get("nrm", PackedVector2Array())
		if rp.size() < 2:
			continue
		var inrm := PackedVector2Array(); inrm.resize(rn.size())
		for i in range(rn.size()):
			inrm[i] = -rn[i]                                 # normale extérieure → INTÉRIEURE
		var rcap := _smooth_border(rp, inrm)             # chaîné + lissé (liseré en courbe)
		_cap_segs[c] = rcap[0]; _cap_norm[c] = rcap[1]
	# LAVIS POLITIQUE : la palette = le pigment d'entité ÉCLAIRCI (aquarelle, pas une dalle) ;
	# l'image owner→teinte est bâtie en C++ (political_image) — même signal que les frontières.
	if w.has_method("political_image"):
		var pal := PackedColorArray()
		pal.resize(w.country_count())
		for c in range(w.country_count()):
			pal[c] = _entity_wash(c)
		var pimg: Image = w.political_image(pal)
		if pimg != null:
			if _pol_tex == null or _pol_tex.get_size() != Vector2(pimg.get_size()):
				_pol_tex = ImageTexture.create_from_image(pimg)
			else:
				_pol_tex.update(pimg)
	_sel_prov_cache = -2                                 # la géographie/souveraineté a bougé → recache la sélection
	_owner_sig = _owner_signature(w)
	_borders_dirty = false

# ── LISSAGE GÉOMÉTRIQUE DES FRONTIÈRES (escaliers → courbes) ──────────────────────────────────────
# La façade rend les arêtes en SEGMENTS UNITAIRES alignés sur la grille (escalier par construction). Le
# SSAA n'y change rien (c'est de la GÉOMÉTRIE, pas de l'aliasing). On CHAÎNE donc les segments en
# polylignes ordonnées puis on les courbe par Chaikin — comme les routes. Pour le RUBAN, la normale
# INTÉRIEURE est recalculée le long de la courbe (l'intérieur d'une chaîne de frontière reste d'un côté
# constant → décidé au 1er segment via la normale d'origine).

## clé entière d'un point de grille (coords façade = entiers, ≤1024×512) → identité de sommet stable.
func _node_key(p: Vector2) -> int:
	return int(round(p.x)) * 4096 + int(round(p.y))

## CHAÎNE une soupe de segments (paires) en polylignes ordonnées. Jonctions (degré≠2) = fin de chaîne ;
## boucles fermées gérées. Retour : Array[PackedVector2Array].
func _chain_segments(flat: PackedVector2Array) -> Array:
	var ctx := _chain_build(flat)
	var chains := []
	for ch in _chain_walk_all(ctx):
		chains.append(ch["poly"])
	return chains

## CHAÎNE avec NORMALE : comme _chain_segments mais renvoie aussi le côté INTÉRIEUR par chaîne (in_left),
## déduit de la normale d'origine du 1er segment. Retour : Array[{poly, in_left}].
func _chain_segments_n(flat: PackedVector2Array, enrm: PackedVector2Array) -> Array:
	var ctx := _chain_build(flat)
	ctx["nrm"] = enrm
	return _chain_walk_all(ctx)

## construit l'index de chaînage (sommets, adjacence) — partagé par les deux variantes.
func _chain_build(flat: PackedVector2Array) -> Dictionary:
	var nseg := flat.size() >> 1
	var node_pt := []            # id → Vector2
	var key2id := {}             # clé → id
	var sa := PackedInt32Array(); var sb := PackedInt32Array()
	sa.resize(nseg); sb.resize(nseg)
	var adj := {}                # id → Array[seg]
	for i in range(nseg):
		var ka := _node_key(flat[i * 2]); var kb := _node_key(flat[i * 2 + 1])
		var ia: int = key2id.get(ka, -1)
		if ia < 0:
			ia = node_pt.size(); key2id[ka] = ia; node_pt.append(flat[i * 2]); adj[ia] = []
		var ib: int = key2id.get(kb, -1)
		if ib < 0:
			ib = node_pt.size(); key2id[kb] = ib; node_pt.append(flat[i * 2 + 1]); adj[ib] = []
		sa[i] = ia; sb[i] = ib
		adj[ia].append(i); adj[ib].append(i)
	return {"sa": sa, "sb": sb, "adj": adj, "node_pt": node_pt, "nseg": nseg, "flat": flat}

## parcourt toutes les chaînes : départs aux noeuds de degré≠2 (bouts/jonctions), puis boucles restantes.
func _chain_walk_all(ctx: Dictionary) -> Array:
	var adj: Dictionary = ctx["adj"]
	var sa: PackedInt32Array = ctx["sa"]
	var nseg: int = ctx["nseg"]
	var node_pt: Array = ctx["node_pt"]
	var used := PackedByteArray(); used.resize(nseg)
	var has_n: bool = ctx.has("nrm")
	var chains := []
	for nid in range(node_pt.size()):
		var inc: Array = adj[nid]
		if inc.size() == 2:
			continue
		for si in inc:
			if used[si] == 0:
				chains.append(_chain_one(si, nid, ctx, used, has_n))
	for i in range(nseg):
		if used[i] == 0:
			chains.append(_chain_one(i, sa[i], ctx, used, has_n))
	return chains

## suit UNE chaîne depuis (start_seg, start_node) jusqu'à une jonction / un bout / le bouclage.
func _chain_one(start_seg: int, start_node: int, ctx: Dictionary, used: PackedByteArray, has_n: bool) -> Dictionary:
	var sa: PackedInt32Array = ctx["sa"]
	var sb: PackedInt32Array = ctx["sb"]
	var adj: Dictionary = ctx["adj"]
	var node_pt: Array = ctx["node_pt"]
	var poly := PackedVector2Array()
	var cur := start_node
	poly.push_back(node_pt[cur])
	# côté INTÉRIEUR (pour le ruban) : direction de marche vs normale d'origine du 1er segment.
	var in_left := true
	if has_n:
		var nrm: PackedVector2Array = ctx["nrm"]
		var nxt0: int = sb[start_seg] if sa[start_seg] == cur else sa[start_seg]
		var d0: Vector2 = node_pt[nxt0] - node_pt[cur]
		in_left = nrm[start_seg].dot(Vector2(-d0.y, d0.x)) > 0.0
	var seg := start_seg
	while true:
		used[seg] = 1
		var nxt: int = sb[seg] if sa[seg] == cur else sa[seg]
		poly.push_back(node_pt[nxt])
		cur = nxt
		var inc: Array = adj[cur]
		if inc.size() != 2:
			break
		var nseg2 := -1
		for s in inc:
			if used[s] == 0:
				nseg2 = s
		if nseg2 < 0:
			break
		seg = nseg2
	return {"poly": poly, "in_left": in_left}

## Chaikin (corner-cutting) : détecte la BOUCLE (1er≈dernier) → lissée cycliquement ; sinon extrémités fixes.
func _chaikin(poly: PackedVector2Array, passes: int) -> PackedVector2Array:
	var p := poly
	for _it in range(passes):
		var n := p.size()
		if n < 3:
			break
		var closed := p[0].distance_to(p[n - 1]) < 0.001
		var out := PackedVector2Array()
		if closed:
			var src := p.slice(0, n - 1)
			var m := src.size()
			for i in range(m):
				var a: Vector2 = src[i]; var b: Vector2 = src[(i + 1) % m]
				out.push_back(a * 0.75 + b * 0.25); out.push_back(a * 0.25 + b * 0.75)
			out.push_back(out[0])                 # referme la boucle
		else:
			out.push_back(p[0])
			for i in range(n - 1):
				var a: Vector2 = p[i]; var b: Vector2 = p[i + 1]
				out.push_back(a * 0.75 + b * 0.25); out.push_back(a * 0.25 + b * 0.75)
			out.push_back(p[n - 1])
		p = out
	return p

## LISSE une polyligne : ré-échantillonnage grossier (casse la fréquence de l'escalier) → passe-bas
## Laplacien (aplatit les marches vers la diagonale) → Chaikin (arrondi). C'est le pipeline qui transforme
## les marches en COURBE (et non plus en « escalier arrondi »). Détecte la boucle (extrémités préservées).
func _smooth_poly(poly: PackedVector2Array) -> PackedVector2Array:
	if poly.size() < 3:
		return poly
	var closed := poly[0].distance_to(poly[poly.size() - 1]) < 0.001
	var p := _resample_polyline(poly, SMOOTH_RESAMPLE) if SMOOTH_RESAMPLE > 0.0 else poly
	p = _taubin(p, SMOOTH_TAUBIN, closed)
	p = _chaikin(p, SMOOTH_CHAIKIN)
	return p

## un pas de lissage Laplacien : p[i] += factor·(moyenne des 2 voisins − p[i]). factor>0 = adoucit (et
## rétrécit), factor<0 = regonfle. Extrémités FIXES (chaîne ouverte → jonctions intactes) ; cyclique (boucle).
func _lap_step(poly: PackedVector2Array, factor: float, closed: bool) -> PackedVector2Array:
	var n := poly.size()
	if n < 3:
		return poly
	if closed:
		var src := poly.slice(0, n - 1)
		var m := src.size()
		var out := PackedVector2Array(); out.resize(m)
		for i in range(m):
			var avg: Vector2 = (src[(i - 1 + m) % m] + src[(i + 1) % m]) * 0.5
			out[i] = src[i] + (avg - src[i]) * factor
		out.push_back(out[0])                  # referme la boucle
		return out
	var out2 := PackedVector2Array(); out2.resize(n)
	out2[0] = poly[0]; out2[n - 1] = poly[n - 1]   # extrémités fixes (jonctions)
	for i in range(1, n - 1):
		var avg2: Vector2 = (poly[i - 1] + poly[i + 1]) * 0.5
		out2[i] = poly[i] + (avg2 - poly[i]) * factor
	return out2

## TAUBIN λ|μ : alterne un pas adoucissant (λ>0) et un pas regonflant (μ<0) → passe-bas qui lisse l'escalier
## SANS rétrécir la forme (la frontière garde sa vraie position → ne bulge pas sur les villes).
func _taubin(poly: PackedVector2Array, iters: int, closed: bool) -> PackedVector2Array:
	if poly.size() < 3 or iters <= 0:
		return poly
	var p := poly
	for _it in range(iters):
		p = _lap_step(p, TAUBIN_LAMBDA, closed)
		p = _lap_step(p, TAUBIN_MU, closed)
	return p

## chaîne + lisse une soupe de segments à NORMALE → [segs (paires), norms (intérieure/segment)].
## La normale est perpendiculaire à la COURBE locale, orientée selon le côté intérieur de la chaîne.
func _smooth_border(flat: PackedVector2Array, enrm: PackedVector2Array) -> Array:
	var out_segs := PackedVector2Array(); var out_norm := PackedVector2Array()
	for ch in _chain_segments_n(flat, enrm):
		var poly: PackedVector2Array = _smooth_poly(ch["poly"])
		if poly.size() < 2:
			continue
		var in_left: bool = ch["in_left"]
		for i in range(poly.size() - 1):
			var a: Vector2 = poly[i]; var b: Vector2 = poly[i + 1]
			var dd: Vector2 = b - a
			if dd.length() < 0.00001:
				continue
			out_segs.push_back(a); out_segs.push_back(b)
			var nn := Vector2(-dd.y, dd.x) if in_left else Vector2(dd.y, -dd.x)
			out_norm.push_back(nn.normalized())
	return [out_segs, out_norm]

## assombrit/éclaircit un pigment d'un cheveu (variation par pays SANS sortir de la gamme : on touche
## la VALEUR seulement, jamais la teinte → pas de dérive néon). `dv` ∈ ~[-0.06, +0.06].
func _shade(c: Color, dv: float) -> Color:
	return Color(clampf(c.r + dv, 0.0, 1.0), clampf(c.g + dv, 0.0, 1.0), clampf(c.b + dv, 0.0, 1.0), c.a)

## or FANÉ des cités-états (encre, pas du métal brillant) — dans la même gamme terreuse.
const CS_GOLD := Color(0.62, 0.50, 0.28)         ## or vieilli

## PIGMENT POLITIQUE d'une entité (le trait fin du pays) : encre d'HÉRITAGE (culture, prune/rouille/
## sienne/olive/ocre/ardoise) + variation par pays sur la VALEUR seule (gamme tenue) ; cité-état = or fané.
## TEINTE (hue) unique d'une entité — LA source partagée de TOUTE sa famille de couleurs
## (frontière · lavis · armée · nom) : golden-ratio par id (voisins bien séparés).
func _entity_hue(e: int) -> float:
	return fmod(float(e) * 0.1607 + 0.04, 1.0)

func _entity_pigment(e: int) -> Color:
	if e < 0:
		return Color(0.30, 0.24, 0.18)
	if int(Sim.world.country_role(e)) == 2:
		return CS_GOLD
	# DISTINCT PAR EMPIRE : jadis la frontière était codée par HÉRITAGE (6 familles) →
	# deux empires du même héritage = MÊME couleur, indistinguables. Désormais une teinte
	# propre à chaque pays (_entity_hue), SATURATION/VALEUR MUETTES (encre terreuse — anti-néon).
	return Color.from_hsv(_entity_hue(e), 0.45, 0.55)

## LAVIS de territoire : MÊME teinte, plus SATURÉE et CLAIRE — l'aquarelle doit TEINTER le
## parchemin (à sat 0.45 le wash lisait GRIS : il assombrissait sans colorer). L'anti-néon
## tient par l'ALPHA bas du wash, pas par la désaturation.
func _entity_wash(e: int) -> Color:
	if e < 0:
		return Color(0.55, 0.50, 0.40)
	if int(Sim.world.country_role(e)) == 2:
		return Color(0.82, 0.68, 0.34)               # cité-état : or clair
	return Color.from_hsv(_entity_hue(e), 0.60, 0.82)

## ÉPAISSEUR ADAPTATIVE (CK) : rend une largeur en unités MONDE à passer DIRECTEMENT à draw_* (le /zoom est
## déjà fait). `base·zoom` = px ÉCRAN voulu à taille monde constante, borné aux rails [min,max] de lisibilité.
## min_px == max_px ⇒ se réduit ALGÉBRIQUEMENT à `min_px/zoom` (px écran constant, l'ancien comportement).
## ⚠ NE JAMAIS écrire `_w(...)/zoom` (le /zoom est inclus) ; ne PAS router des longueurs/motifs ici.
func _w(zoom: float, base_world: float, min_px: float, max_px: float) -> float:
	return clampf(base_world * zoom, min_px, max_px) / maxf(zoom, 0.0001)

## frontière de PAYS en DOUBLE PASSE (gravé, façon Civ/atlas) : (1) halo brun très sombre LARGE = le
## « creux » gravé qui DÉTACHE la frontière du terrain ; (2) pigment politique FIN par-dessus = la couleur
## de l'entité. Tracé SUR la ligne lissée (les normales ne servent plus qu'au liseré de capitale).
func _draw_band(mv: Node2D, segs: PackedVector2Array, pigment: Color, zoom: float) -> void:
	if segs.size() < 2:
		return
	var proj := _project_segs_iso(mv, segs)
	if proj.size() < 2:
		return
	draw_multiline(proj, Color(POL_HALO.r, POL_HALO.g, POL_HALO.b, 0.45), _w(zoom, POL_HALO_BASE, POL_HALO_MIN, POL_HALO_MAX), true)
	draw_multiline(proj, Color(pigment.r, pigment.g, pigment.b, 0.85), _w(zoom, POL_PIG_BASE, POL_PIG_MIN, POL_PIG_MAX), true)

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

## hash scalaire → [0,1) (déterministe, display-only) — varie dressing/orientations.
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
	# les PONTS : là où le tracé LISSE traverse l'eau de rivière carvée — milieu + tangente.
	_ink_bridges.clear()
	var rfb: Image = _carved_river_field()
	if rfb != null:
		for rd0 in _roads:
			var pts0: PackedVector2Array = rd0["points"]
			var inw0 := -1
			for k in range(pts0.size()):
				var inw := _in_river_water(rfb, int(pts0[k].x), int(pts0[k].y))
				if inw and inw0 < 0:
					inw0 = k
				elif (not inw or k == pts0.size() - 1) and inw0 >= 0:
					var mid: Vector2 = pts0[clampi((inw0 + k) / 2, 0, pts0.size() - 1)]
					var t0: Vector2 = (pts0[mini(k, pts0.size() - 1)] - pts0[maxi(inw0 - 1, 0)]).normalized()
					if t0.length() > 0.5:
						_ink_bridges.append({"w": mid, "t": t0})
					inw0 = -1
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
	var bundle := {}   # hash spatial (cellule 1.0) des points DÉJÀ tracés — magnétisme de couloir
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
		# 3) ANTI-DÉDOUBLEMENT — le MAGNÉTISME DE COULOIR : un point qui passe à ≤ 0.65 cellule
		#    d'une route DÉJÀ tracée se COLLE dessus → les A* voisins PARTAGENT la chaussée au
		#    lieu de dessiner deux lignes parallèles « far-west ». Les 3 points d'about restent
		#    libres (le raccord au bourg prime). Hash spatial, une passe par route.
		for k in range(3, pts.size() - 3):
			var p5: Vector2 = pts[k]
			var gx := int(floor(p5.x))
			var gy := int(floor(p5.y))
			var bestd := 0.42          # 0.65² : rayon de collage au couloir
			var bestp := p5
			for oy in range(-1, 2):
				for ox in range(-1, 2):
					var kk := (gx + ox) * 100000 + (gy + oy)
					if bundle.has(kk):
						for q5 in bundle[kk]:
							var dd: float = p5.distance_squared_to(q5)
							if dd < bestd:
								bestd = dd
								bestp = q5
			pts[k] = bestp
		for k in range(pts.size()):    # cette route ENTRE dans le couloir commun
			var kk2 := int(floor(pts[k].x)) * 100000 + int(floor(pts[k].y))
			if not bundle.has(kk2):
				bundle[kk2] = []
			bundle[kk2].append(pts[k])
		rd["ra"] = ra            # mémorisé : le bâti du bourg s'organise le long des routes de SA ville
		rd["rb"] = rb
		rd["points"] = pts

## projette une polyligne MONDE en iso (helper du dessin de routes).
func _road_iso(poly: PackedVector2Array, mv) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(poly.size())
	for k in range(poly.size()):
		out[k] = mv.iso_pos(poly[k].x, poly[k].y)
	return out

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

## VRAI si (x,y) est SUR ou À CÔTÉ d'une rivière VISIBLE (seuil BAS + voisinage 1 cellule) → aucune marque
## de dressing ici (sinon, la rivière étant translucide, la marque transparaît « sous » l'eau = artefact).
const DRESS_RIVER_MIN := 0.08   ## seuil bas (la rivière du shader s'imprime dès ~0.13 ; on l'attrape + marge)
func _near_river(rf: Image, x: int, y: int) -> bool:
	if rf == null:
		return false
	var rw := rf.get_width()
	var rh := rf.get_height()
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			var nx := x + dx
			var ny := y + dy
			if nx < 0 or ny < 0 or nx >= rw or ny >= rh:
				continue
			if rf.get_pixel(nx, ny).r >= DRESS_RIVER_MIN:
				return true
	return false

var _sig_poll := 0.0
func _process(dt: float) -> void:
	# pendant un cataclysme, on redessine en continu pour PULSER l'épicentre
	# (horloge MUR, hors déterminisme). Sinon : aucun coût (le tick suffit).
	if _cataclysm:
		queue_redraw()
	# FRONTIÈRES/ASSETS DÉCOUPLÉS DU TICK : jadis seul `_on_tick` (qui ne fire PAS en
	# pause) recalculait la souveraineté → frontières/routes/villes ne se rafraîchissaient
	# qu'au DÉBLOCAGE de la pause. On sonde ~4×/s (même en pause) : la carte reflète l'état
	# COURANT, elle n'attend plus qu'on relève la pause.
	_sig_poll += dt
	if _sig_poll >= 0.25:
		_sig_poll = 0.0
		if Sim.world != null:
			var sig := _owner_signature(Sim.world)
			if sig != _owner_sig:
				_owner_sig = sig
				_borders_dirty = true
				_roads_dirty = true
				_struct_dirty = true
				_clutter_dirty = true
				queue_redraw()

## pop d'une région → bande de ville 1-8 (les paliers des sprites CITY_POP_BAND).
const CITY_POP_BANDS := [150, 400, 900, 1800, 3500, 7000, 14000]   # 7 seuils → 8 bandes
func _country_color(c: int) -> Color:
	# UNE SEULE FAMILLE de couleur par entité : la teinte du pigment politique (frontière =
	# lavis = armée = nom), en version FORTE pour un acteur posé SUR la carte (le jeton doit
	# se détacher du lavis muet). Jadis une roue HSV vive INDÉPENDANTE (0.137·c, sat 0.72) :
	# l'armée d'un pays n'avait PAS la couleur de sa frontière.
	if c < 0:
		return Color(0.7, 0.7, 0.72)
	return _shade(_entity_pigment(c), 0.22)

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
	# MODE RESSOURCES (9) : les icônes de brutes par tuile, AU-DESSUS de tout (sauf en mode NATURE).
	if not nature_mode and int(mv.get("mode")) == 9:
		_draw_resources(w, mv, true)

## CARTE PARCHEMIN — acteurs tracés en ENCRE vectorielle (zéro sprite) : frontières, routes,
## villes (glyphes), noms d'empire, armées, épicentre §27. La Camera2D met à l'échelle ; les
## tailles d'encre sont en px ÉCRAN (÷ zoom) → lisibles à tous les zooms.
func _draw_iso(w, mv: Node2D) -> void:
	var zoom := get_viewport_transform().get_scale().x
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var INK := Color(0.20, 0.14, 0.09, 0.95)         # encre brun-sépia (le trait de plume)

	# ── LAVIS POLITIQUE (aquarelle) : le territoire teinté SOUS tout — la carte DIT qui tient
	#    quoi d'un regard au plan large ; le lavis s'efface vers le zoom profond (le terrain parle). ──
	if not nature_mode and _pol_tex != null:
		if _borders_dirty:
			_rebuild_borders()                        # le lavis se rebâtit avec les frontières
		var wash_a := lerpf(WASH_A_FAR, WASH_A_NEAR,
			clampf((zoom - WASH_FADE_LO) / (WASH_FADE_HI - WASH_FADE_LO), 0.0, 1.0))
		var p0: Vector2 = mv.iso_pos(0, 0)
		var p1: Vector2 = mv.iso_pos(w.map_w(), w.map_h())
		draw_texture_rect(_pol_tex, Rect2(p0, p1 - p0), false, Color(1, 1, 1, wash_a))

	# ── DRESSING DE TERRAIN (lot 2) : marques de biome (relief/végétation/zones), SOUS tout le reste. ──
	if zoom >= DECOR_ZOOM_MIN:
		if _dressing_dirty:
			_build_dressing()
			_dressing_dirty = false
		var dress_col := Color(1, 1, 1, DRESS_ALPHA)
		var egg_col := Color(1, 1, 1, EGG_ALPHA)
		for d in _dressing:
			var wp: Vector2 = d["pos"]
			var dip: Vector2 = mv.iso_pos(wp.x, wp.y)
			var dss: Vector2 = vt * dip
			if dss.x < -90 or dss.y < -90 or dss.x > vp.x + 90 or dss.y > vp.y + 90:
				continue
			var did: String = d["id"]
			var dtex := _dress_get(did)
			if dtex == null:
				continue
			var is_egg: bool = d.get("egg", false)
			var dh := _dress_size(did) * float(d["scale"]) / zoom        # hauteur MONDE (taille écran constante)
			var dw := dh
			if did.begins_with("sea_serpent"):
				dw = dh * 2.0                                            # serpent : sprite 2:1 (large)
			draw_texture_rect(dtex, Rect2(dip - Vector2(dw * 0.5, dh * 0.5), Vector2(dw, dh)), false,
				egg_col if is_egg else dress_col)
	# MODE NATURE : juste le terrain + le dressing — on saute frontières, routes, villes, armées, noms, §27.
	if nature_mode:
		return

	# ── FRONTIÈRES à l'ENCRE (calligraphie) : TRAME FINE 1px (toutes provinces+régions) + BLOCS
	#    d'empire 3px, COULEUR PAR ENTITÉ, en 2 passes (bave d'encre douce + plume nette, jittées). ──
	if _borders_dirty:
		_rebuild_borders()
	# la TRAME FINE fond en survol (sinon mosaïque illisible) et se révèle au plan rapproché — toutes
	# les provinces RESTENT tracées (1px), juste graduées au zoom (LOD ; les blocs d'empire, eux, toujours).
	if _borders.has(0):
		# PROVINCE = administrative, un CHUCHOTEMENT : déjà restreinte à la terre ADMINISTRÉE
		# (rebuild), elle n'émerge qu'au plan rapproché (zoom 2.2+) et plafonne bas (0.24) —
		# le lavis + la frontière d'empire portent la lecture, la trame ne fait que détailler.
		var fine_a := clampf((zoom - 2.2) / 2.6, 0.0, 1.0) * 0.24
		if fine_a > 0.02:
			var fseg := _project_segs_iso(mv, _borders[0])
			if fseg.size() >= 2:
				draw_multiline(fseg, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a * 0.45), _w(zoom, 0.6, 0.9, 1.5), true)
				draw_multiline(fseg, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a), _w(zoom, 0.34, 0.6, 0.9), true)
	# PAYS : trait GRAVÉ en double passe (halo brun sombre LARGE + pigment politique FIN), pour bien
	# SÉPARER l'administratif (province, cheveu brun) du politique (pays, trait coloré net). Puis le
	# LISERÉ POURPRE FIN de chaque capitale, AU-DESSUS.
	for entity in _b_segs:
		_draw_band(mv, _b_segs[entity], _entity_pigment(entity), zoom)
	for cc in _cap_segs:
		_draw_cap_lisere(mv, _cap_segs[cc], _cap_norm[cc], zoom)

	# ── SÉLECTION : contour DORÉ de la province choisie (creux d'encre + or net), AU-DESSUS
	#    des frontières — le retour visuel du clic (le panneau dit QUOI, le contour dit OÙ). ──
	var selp := int(mv.get("_selected_prov"))
	if selp >= 0:
		if selp != _sel_prov_cache and w.has_method("province_border_segments"):
			_sel_prov_cache = selp
			_sel_segs = PackedVector2Array()
			var sd: Dictionary = w.province_border_segments(selp)
			var sp: PackedVector2Array = sd.get("pts", PackedVector2Array())
			for ch in _chain_segments(sp):
				var poly: PackedVector2Array = _smooth_poly(ch)
				for i in range(poly.size() - 1):
					_sel_segs.push_back(poly[i]); _sel_segs.push_back(poly[i + 1])
		if _sel_segs.size() >= 2:
			var sseg := _project_segs_iso(mv, _sel_segs)
			draw_multiline(sseg, Color(0.12, 0.08, 0.04, 0.80), _w(zoom, 1.3, 2.6, 4.4), true)
			draw_multiline(sseg, Color(SEL_GOLD.r, SEL_GOLD.g, SEL_GOLD.b, 0.95), _w(zoom, 0.7, 1.5, 2.6), true)
	elif _sel_prov_cache != -2:
		_sel_prov_cache = -2
		_sel_segs = PackedVector2Array()

	# ── ROUTES : CHEMIN DE TERRE À 3 TRAITS (sous-trait sépia + corps crème + filet clair) —
	#    le motif cartographique classique, sur les polylignes DÉJÀ lissées (_augment_roads).
	#    Croissance organique (1 an/province) ; segments cumulés → batchs par hiérarchie. ──
	if zoom >= ROAD_ZOOM_MIN:
		_ensure_roads()
		if not _roads.is_empty():
			var year: int = w.year()
			var polys_main := []
			var polys_minor := []
			var seen := {}   # dédup : un TRONÇON partagé (couloir commun) ne s'encre qu'UNE fois
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
				var is_main: bool = int(rd.get("level", 1)) <= 0
				# découpe en SOUS-POLYLIGNES de segments inédits (les tronçons déjà encrés sautent)
				var run := PackedVector2Array()
				for k in range(poly.size() - 1):
					var a7: Vector2 = poly[k]
					var b7: Vector2 = poly[k + 1]
					var ka := int(a7.x * 4.0) * 8388608 + int(a7.y * 4.0)
					var kb := int(b7.x * 4.0) * 8388608 + int(b7.y * 4.0)
					var kseg := str(mini(ka, kb)) + "_" + str(maxi(ka, kb))
					if seen.has(kseg):
						if run.size() >= 2:
							(polys_main if is_main else polys_minor).append(_road_iso(run, mv))
						run = PackedVector2Array()
						continue
					seen[kseg] = true
					if run.is_empty():
						run.append(a7)
					run.append(b7)
				if run.size() >= 2:
					(polys_main if is_main else polys_minor).append(_road_iso(run, mv))
			# PAR POLYLIGNE (joints RONDS aux coudes — le multiline ouvrait des fentes) ; l'ordre
			# des passes fait le modelé : ombre sépia → terre crème → filet de lumière central.
			for pl2 in polys_minor:
				draw_polyline(pl2, ROAD_MINOR_EDGE, _w(zoom, 0.65, 1.4, 2.6), true)
			for pl2 in polys_minor:
				draw_polyline(pl2, ROAD_MINOR_MAIN, _w(zoom, 0.36, 0.8, 1.5), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_EDGE, _w(zoom, 1.1, 2.2, 4.0), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_MAIN, _w(zoom, 0.62, 1.3, 2.4), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_LIGHT, _w(zoom, 0.26, 0.55, 1.0), true)
			# les PONTS D'ENCRE : deux garde-corps bombés en travers du franchissement de rivière
			for br in _ink_bridges:
				var bp: Vector2 = mv.iso_pos((br["w"] as Vector2).x, (br["w"] as Vector2).y)
				var bt: Vector2 = (mv.iso_pos((br["w"] as Vector2).x + (br["t"] as Vector2).x,
					(br["w"] as Vector2).y + (br["t"] as Vector2).y) - bp).normalized()
				var bperp := Vector2(-bt.y, bt.x)
				var bl := _w(zoom, 0.62, 2.4, 5.2)
				var bw := _w(zoom, 0.26, 1.0, 2.2)
				var biw := _w(zoom, 0.07, 0.5, 1.0)
				for sgn in [-1.0, 1.0]:
					var o3 := bperp * bw * float(sgn)
					draw_polyline(PackedVector2Array([bp - bt * bl + o3,
						bp + o3 + bperp * (bw * 0.5 * float(sgn)),   # le bombé du tablier
						bp + bt * bl + o3]), TOWN_INK, biw, true)

	# ── VILLES : TAMPONS d'atlas (lot 1) — cité (t1-t7), cité-état & hameau libre (assets DÉDIÉS). ──
	# CENTRÉS sur le SIÈGE intérieur de province (≠ jonction ; le centroïde brut tombe pile à l'intersection
	# des provinces). Cité-état (rôle 2) & hameau libre (rôle 4) toujours tracés même sans tier de ville.
	if zoom >= CITY_ZOOM_MIN:
		for r in range(w.region_count()):
			var tier: int = w.region_tier(r)
			var owner: int = w.region_owner(r)
			var role: int = int(w.country_role(owner)) if owner >= 0 else -1
			if tier < 0 and role != 2 and role != 4:
				continue                                  # wilderness sans ville → rien (mais on garde cité-état/libre)
			var ctr: Vector2 = _region_seat.get(r, w.region_centroid(r))
			if ctr.x < 0:
				continue
			var ip: Vector2 = mv.iso_pos(ctr.x, ctr.y)
			var ss: Vector2 = vt * ip
			if ss.x < -40 or ss.y < -40 or ss.x > vp.x + 40 or ss.y > vp.y + 40:
				continue
			_draw_settlement(w, r, role, ctr, ip, zoom, mv)
			# RÉGIME KCD : la BANNIÈRE de lieu éclot au plan rapproché — le relais des
			# noms de pays (régime EU4) qui se sont effacés au même seuil de zoom.
			if zoom >= 4.0:
				_draw_banner(w, r, ip, zoom, clampf((zoom - 4.0) / 1.2, 0.0, 1.0))

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
		draw_circle(ctr, s + _w(zoom, 0.45, 1.4, 2.6), Color(_phase_color(phase), 0.9))  # anneau de phase : respire à l'approche, borné
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
		if ps.is_empty():
			continue      # (1 centroïde = valide : ancre au point, pas d'orientation — l'empire
			              #  mono-région du DÉPART charte garde son nom ; l'ACP exige ≥2 sinon)
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
		# la plume LE LONG du pays. AGRANDI (1.35→1.9 : lisible au fit, là où la carte se joue) et
		# TEINTÉ au pigment de l'entité assombri (même famille que frontière/lavis — cohérence).
		# RÉGIME EU4 : les noms de PAYS vivent au plan LARGE — grands, en capitales ESPACÉES
		# le long de l'axe du pays, à l'échelle de sa TAILLE — et s'EFFACENT au zoom (le plan
		# rapproché appartient aux bannières de lieux, régime KCD).
		var name_fade := 1.0 - clampf((zoom - 3.2) / 1.6, 0.0, 1.0)
		if name_fade <= 0.02:
			continue
		var pig := _entity_pigment(c)
		var rl := int(w.country_role(c))
		var is_emp := (rl == 0 or rl == 1)
		var track := 0.45 if is_emp else 0.0            # espacement de capitales (E U 4)
		var name_ink := Color(pig.r * 0.40, pig.g * 0.40, pig.b * 0.40, (0.95 if is_emp else 0.70) * name_fade)
		var halo := Color(0.97, 0.91, 0.74, (0.75 if is_emp else 0.5) * name_fade)
		var disp := nm.to_upper() if is_emp else nm
		# largeur TRACKÉE (par caractère) pour centrer
		var tw := 0.0
		for k in range(disp.length()):
			tw += VKit.text_w(disp[k], VKit.FS_SMALL) + (track * 6.0 if k < disp.length() - 1 else 0.0)
		# LA RÈGLE EU4 : le nom est ANCRÉ MONDE et DIMENSIONNÉ À SON TERRITOIRE — il s'étire
		# sur l'étendue du pays (≈3σ de l'axe ACP majeur), jamais sur la mer d'à côté. Il
		# grossit donc à l'écran en zoomant, jusqu'au fondu (le relais KCD des bannières).
		var nsc: float
		if is_emp:
			var span := clampf(2.8 * sqrt(maxf(l1, 0.0)) + 16.0, 22.0, 220.0)   # étendue-monde du nom
			nsc = span / maxf(tw, 1.0)
		else:
			nsc = 1.1 / zoom                              # petites entités : chip écran-constant
		draw_set_transform(ip, ang, Vector2(nsc, nsc))
		var cx0 := -tw * 0.5
		for k in range(disp.length()):
			var ch := disp[k]
			VKit.text(self, Vector2(cx0 + 0.7, -6.3), halo, ch, VKit.FS_SMALL)
			VKit.text(self, Vector2(cx0, -7.0), name_ink, ch, VKit.FS_SMALL)
			cx0 += VKit.text_w(ch, VKit.FS_SMALL) + track * 6.0
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
				# §27 : l'anneau d'épicentre DOIT se lire au plan large (drame global) ; trait borné, le rayon de pulse reste /zoom.
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), _w(zoom, 0.35, 1.2, 2.4), true)

## charge (paresseux) un TAMPON de peuplement par id → Texture2D. `load()` (importé) puis repli Image.load
## (PNG brut, robuste si l'import n'a pas tourné). Cache (y compris le null) → pas de rechargement par frame.
func _stamp_get(id: String) -> Texture2D:
	if _stamp_tex.has(id):
		return _stamp_tex[id]
	var path := "res://art/map_stamps/lot1/assets_alpha/%s.png" % id
	var tex: Texture2D = null
	if ResourceLoader.exists(path):
		tex = load(path)
	if tex == null:
		var img := Image.new()
		if img.load(path) == OK:
			tex = ImageTexture.create_from_image(img)
	_stamp_tex[id] = tex
	return tex

## ── L'URBANISTE : villes PROCÉDURALES à l'encre, POSÉES SUR LA ROUTE ──────────────────
## L'outil (display-only) qui remplace les tampons : chaque bourg est un AMAS déterministe
## de petites maisons à pignon (murs crème, toits brun-rouge, cerne d'encre — le langage
## des vignettes de cartes anciennes), rangées LE LONG DE LA ROUTE réelle (deux rangées,
## quelle que soit son orientation — le vectoriel tourne gratuitement) ; sans route, un
## amas radial. Monument au siège selon le tier (église t3+, donjon en capitale), ENCEINTE
## pour la cité-état. Ancré au MONDE (la ville tient sur sa rue à tous les zooms), tailles
## bornées en px écran par _w. Cache par région (RAZ au generate ; jamais figé sans routes).
const TOWN_WALL   := Color(0.92, 0.87, 0.74, 0.96)   ## murs crème
const TOWN_INK    := Color(0.16, 0.11, 0.07, 0.90)   ## cerne d'encre
const TOWN_SHADOW := Color(0.16, 0.11, 0.07, 0.16)   ## ombre portée (assoit le bâti)
const TOWN_GROUND := Color(0.88, 0.82, 0.64, 0.30)   ## la CLAIRIÈRE (terre battue du bourg)
const FIELD_FILL  := Color(0.82, 0.78, 0.52, 0.30)   ## champs en lanières (lavis paille)
const FIELD_FURROW:= Color(0.45, 0.38, 0.22, 0.30)   ## sillons
## trois PALETTES de toits — chaque bourg a la sienne (brique · bois · ardoise), la
## variante par maison joue la valeur : un village = une matière, pas un arlequin.
const ROOF_PAL := [
	[Color(0.58, 0.28, 0.19, 0.94), Color(0.48, 0.23, 0.16, 0.94)],   # brique
	[Color(0.46, 0.33, 0.21, 0.94), Color(0.38, 0.27, 0.17, 0.94)],   # bois
	[Color(0.42, 0.40, 0.42, 0.94), Color(0.34, 0.33, 0.36, 0.94)],   # ardoise
]
## v3 — les couleurs restent DANS la famille du fond (l'exigence : ne pas détonner) :
## la place = la clairière en un ton plus clair ; le bois des quais = la palette toit-bois ;
## la fumée = un gris chaud à alpha très bas (un souffle, pas un trait).
const PLAZA_FILL  := Color(0.90, 0.85, 0.68, 0.36)   ## place de marché (terre claire tassée)
const QUAY_WOOD   := Color(0.44, 0.32, 0.22, 0.90)   ## planches de quai (bois patiné)
const BOAT_WOOD   := Color(0.37, 0.27, 0.19, 0.92)   ## coque (bois sombre)
const SMOKE_SOFT  := Color(0.60, 0.56, 0.50, 0.14)   ## fumée de cheminée (souffle gris chaud)
var _town_cache := {}       ## region → plan du bourg (voir _build_town)
var _ink_bridges := []      ## [{w:Vector2, t:Vector2}] — ponts aux franchissements route×rivière
var _sea_img: Image = null  ## couche EAU (cache par monde — quais)
var _rf_img: Image = null   ## champ rivière carvé (cache par monde — quais fluviaux)

## VRAI si la cellule (x,y) est de l'EAU (mer OU rivière carvée) — pour poser les quais.
func _water_at(x: int, y: int) -> bool:
	if _sea_img == null and Sim.world != null:
		_sea_img = Sim.world.layer_image(LAYER_WATER)
	if _rf_img == null:
		_rf_img = _carved_river_field()
	if _sea_img != null and x >= 0 and y >= 0 and x < _sea_img.get_width() and y < _sea_img.get_height():
		if _sea_img.get_pixel(x, y).r > 0.5:
			return true
	return _in_river_water(_rf_img, x, y)

## point de ROUTE le plus proche du siège + TANGENTE locale (espace monde).
## Retourne {d2, p, t} ; d2 = 1e30 si aucune route.
func _seat_road(ctr: Vector2) -> Dictionary:
	var best_d := 1e30
	var best_p := Vector2.ZERO
	var best_t := Vector2.RIGHT
	for rd in _roads:
		var pts: PackedVector2Array = rd["points"]
		for k in range(pts.size()):
			var d := ctr.distance_squared_to(pts[k])
			if d < best_d:
				best_d = d
				best_p = pts[k]
				var k2 := mini(k + 1, pts.size() - 1)
				var k1 := maxi(k - 1, 0)
				var tv := pts[k2] - pts[k1]
				best_t = tv.normalized() if tv.length() > 0.001 else Vector2.RIGHT
	return {"d2": best_d, "p": best_p, "t": best_t}

## bâtit (une fois) le PLAN du bourg : maisons en unités MONDE. k : 0 maison · 1 église ·
## 2 donjon. v : variante de toit. Rangées le long de la route si elle passe à ≤ 3 cellules.
## Le plan porte aussi : la CLAIRIÈRE (blob irrégulier), les CHAMPS en lanières (t2+),
## l'ENCEINTE à TOURS avec PORTES là où les routes la franchissent (cité-état).
func _build_town(r: int, ctr: Vector2, n: int, landmark: int, ring_rad: float, tier: int) -> Dictionary:
	var houses := []
	var rr := _seat_road(ctr)
	var on_road: bool = float(rr["d2"]) < 9.0
	var axis: Vector2 = rr["t"] if on_road else Vector2.from_angle(_h1(float(r) * 3.7) * TAU)
	var side := Vector2(-axis.y, axis.x)
	var org: Vector2 = rr["p"] if on_road else ctr
	var extent := 0.9   # demi-longueur de la rue bâtie (pour clairière/champs)
	# ── la PLACE DE MARCHÉ : au CARREFOUR (≥ 2 routes passent au bourg), grandes villes
	#    seulement — un octogone de terre claire, un puits au centre, une couronne de
	#    maisons AUTOUR (le reste du bâti garde la rue). ──
	var nroads := 0
	var jpt := Vector2.ZERO
	for rd in _roads:
		var pts2: PackedVector2Array = rd["points"]
		var bestd := 1e30
		var bp := Vector2.ZERO
		var k2 := 0
		while k2 < pts2.size():
			var d2 := ctr.distance_squared_to(pts2[k2])
			if d2 < bestd:
				bestd = d2
				bp = pts2[k2]
			k2 += 2
		if bestd < 6.25:
			nroads += 1
			jpt += bp
	var has_plaza: bool = on_road and nroads >= 2 and n >= 8
	var plaza := PackedVector2Array()
	var well := Vector2.ZERO
	var ring_n := 0
	if has_plaza:
		jpt /= float(nroads)
		well = jpt
		ring_n = mini(6, n / 2)
		for k in range(10):
			var pa := TAU * float(k) / 10.0
			var pr := 0.60 * (0.86 + 0.26 * _h1(float(r) * 3.3 + float(k) * 1.9))
			plaza.push_back(jpt + Vector2(cos(pa), sin(pa) * 0.80) * pr)
	# ── le PLAN v4 : rue principale + RANG ARRIÈRE (30 %) + RUELLES perpendiculaires —
	#    fini la « ligne western » ; la relaxation ci-dessous tasse le tout en TISSU. ──
	var lane_n := 0
	var lane_s := []
	if on_road and n >= 8:
		lane_n = 1 if n < 14 else 2
		for k in range(lane_n):
			lane_s.append((_h1(float(r) * 8.3 + float(k) * 3.1) - 0.5) * extent * 1.2)
	for i in range(n):
		var hh := _h1(float(r) * 13.7 + float(i) * 2.31)
		var hv := _h1(float(r) * 7.9 + float(i) * 5.17)
		var wp: Vector2
		if i < ring_n:
			# la COURONNE de la place : les maisons regardent le marché
			var a6 := TAU * (float(i) + 0.5) / float(ring_n) + _h1(float(r) * 1.3) * TAU
			wp = jpt + Vector2(cos(a6), sin(a6) * 0.85) * (0.98 + 0.18 * hh)
			extent = maxf(extent, jpt.distance_to(org) + 1.0)
		elif on_road:
			var i2 := i - ring_n
			var n2 := n - ring_n
			var nlane: int = (n2 / 3) if lane_n > 0 else 0     # un tiers du bâti part en ruelles
			if i2 < nlane:
				# RUELLE : perpendiculaire à la rue, maisons des deux côtés en profondeur
				var li := i2 % lane_n
				var s0: float = lane_s[li]
				var along := 0.62 + 0.60 * float(i2 / (2 * lane_n))
				var lsd := (0.34 + 0.20 * hh) * (1.0 if (i2 % 2) == 0 else -1.0)
				var ldir := 1.0 if _h1(float(r) * 5.9 + float(li) * 2.7) < 0.5 else -1.0
				wp = org + axis * (s0 + lsd) + side * (along * ldir)
			else:
				# la RUE : deux rangées serrées + un RANG ARRIÈRE clairsemé (30 %)
				var i3 := i2 - nlane
				var s := (float(i3 / 2) - float((n2 - nlane + 1) / 2 - 1) * 0.5) * 0.78
				var back := _h1(float(r) * 4.7 + float(i3) * 1.9) < 0.30
				var sd := ((1.08 + 0.25 * hh) if back else (0.5 + 0.22 * hh)) * (1.0 if (i3 % 2) == 0 else -1.0)
				wp = org + axis * (s + (hv - 0.5) * 0.3) + side * sd
				extent = maxf(extent, absf(s) + 0.8)
		else:
			# amas radial (spirale dorée) autour du siège
			var a := (0.618034 * float(i) + _h1(float(r) * 9.1)) * TAU
			var rad := sqrt((float(i) + 0.6) / float(n)) * (0.9 + 0.35 * float(n) / 10.0)
			wp = ctr + Vector2(cos(a), sin(a)) * rad
			extent = maxf(extent, rad + 0.6)
		houses.append({"w": wp, "s": 0.34 + 0.12 * hh, "k": 0, "v": int(hv * 2.0)})
	if landmark > 0:
		# le MONUMENT : posé en retrait de la rue, côté opposé au gros des maisons
		var lm: Vector2 = (org + side * -0.9) if on_road else ctr
		houses.append({"w": lm, "s": 0.46, "k": landmark, "v": 0})
	# ── RELAXATION : 3 passes de séparation (min 0.55 cellule) — plus de chevauchements,
	#    le plan se TASSE organiquement (le bourg, pas la file indienne). ──
	for _it in range(3):
		for i in range(houses.size()):
			for j in range(i + 1, houses.size()):
				var pi: Vector2 = houses[i]["w"]
				var pj: Vector2 = houses[j]["w"]
				var dv := pj - pi
				var d := dv.length()
				if d < 0.55 and d > 0.001:
					var push := dv.normalized() * (0.55 - d) * 0.5
					houses[i]["w"] = pi - push
					houses[j]["w"] = pj + push
	# ── le CENTRE BÂTI : muraille & clairière suivent la VILLE réelle (pas le siège abstrait
	#    — le mur ne coupe plus le bâti quand la route passe loin du centroïde de région). ──
	var bc := Vector2.ZERO
	var brad := 0.0
	for hd0 in houses:
		bc += hd0["w"]
	bc /= float(maxi(houses.size(), 1))
	for hd0 in houses:
		brad = maxf(brad, bc.distance_to(hd0["w"]))
	# ── la CLAIRIÈRE : un blob IRRÉGULIER de terre battue sous le bourg (14 pts jittés),
	#    CENTRÉE SUR LE BÂTI et dimensionnée par lui ──
	var gnd := PackedVector2Array()
	var g_r := brad + 0.7
	for k in range(14):
		var ga := TAU * float(k) / 14.0
		var gr := g_r * (0.82 + 0.36 * _h1(float(r) * 5.3 + float(k) * 1.77))
		# la clairière s'ÉTIRE le long de la rue (ellipse orientée par l'axe)
		var u := cos(ga) * (1.20 if on_road else 1.0)
		var v := sin(ga) * 0.85
		gnd.push_back(bc + axis * (u * gr) + side * (v * gr))
	# ── CHAMPS EN LANIÈRES (t2+) : des bandes PERPENDICULAIRES à la rue, aux abouts du bourg ──
	var fields := []
	if tier >= 2 or ring_rad > 0.0:
		var nf := 2 + tier + (2 if ring_rad > 0.0 else 0)
		for k in range(nf):
			var fh := _h1(float(r) * 21.3 + float(k) * 3.9)
			var fh2 := _h1(float(r) * 17.1 + float(k) * 7.3)
			var endd := 1.0 if (k % 2) == 0 else -1.0
			var fc: Vector2 = org + axis * endd * (extent + 0.9 + 1.5 * fh) + side * (fh2 - 0.5) * 2.6
			var fl := axis.rotated(0.12 * (fh - 0.5))            # lanière ~perpendiculaire à la rue
			var fp := Vector2(-fl.y, fl.x)
			var hl := 0.75 + 0.45 * fh2                          # demi-longueur
			var hw := 0.20 + 0.10 * fh                           # demi-largeur
			fields.append({"q": PackedVector2Array([
				fc + fp * hl + fl * hw, fc + fp * hl - fl * hw,
				fc - fp * hl - fl * hw, fc - fp * hl + fl * hw]),
				"d": fp, "c": fc, "hl": hl, "hw": hw})
	# ── l'ENCEINTE (cité-état) : CENTRÉE SUR LE BÂTI, rayon = le bâti + une marge (le mur
	#    ENCLOT la ville — il ne la coupe plus) ; arcs entre PORTES (routes) + TOURS ──
	var arcs := []
	var towers := []
	var gates := []
	if ring_rad > 0.0:
		var wrad := maxf(brad + 0.60, 1.7)
		for rd in _roads:                          # angles de PORTE : croisement route × muraille
			var pts: PackedVector2Array = rd["points"]
			for k in range(pts.size() - 1):
				var da := pts[k].distance_to(bc) - wrad
				var db := pts[k + 1].distance_to(bc) - wrad
				if da * db < 0.0 and gates.size() < 4:
					var ang := ((pts[k] + pts[k + 1]) * 0.5 - bc).angle()
					var dup := false
					for g2 in gates:
						if absf(angle_difference(float(g2), ang)) < 0.45:
							dup = true
					if not dup:
						gates.append(ang)
		var gw := 0.16                             # demi-ouverture de porte (rad)
		var seg := PackedVector2Array()
		var steps := 72
		for k in range(steps + 1):
			var a4 := TAU * float(k) / float(steps)
			var in_gate := false
			for g3 in gates:
				if absf(angle_difference(a4, float(g3))) < gw:
					in_gate = true
			var wpt := bc + Vector2(cos(a4), sin(a4)) * wrad
			if in_gate:
				if seg.size() >= 2:
					arcs.append(seg)
				seg = PackedVector2Array()
			else:
				seg.push_back(wpt)
		if seg.size() >= 2:
			arcs.append(seg)
		for k in range(7):                         # TOURS régulières, hors portes
			var ta := TAU * float(k) / 7.0 + _h1(float(r) * 2.9) * 0.5
			var skip := false
			for g4 in gates:
				if absf(angle_difference(ta, float(g4))) < 0.30:
					skip = true
			if not skip:
				towers.append(bc + Vector2(cos(ta), sin(ta)) * wrad)
	# ── les QUAIS : si le bourg touche l'EAU (mer ou rivière carvée) à ≤ 3 cellules —
	#    une ou deux jetées de bois perpendiculaires au rivage ; une barque amarrée (t3+). ──
	var quays := []
	var boat := {}
	var bestw := 1e30
	var wdir := Vector2.RIGHT
	var wpt := Vector2.ZERO
	for k in range(16):
		var dirv := Vector2.from_angle(TAU * (float(k) + 0.5 * _h1(float(r) * 6.1)) / 16.0)
		var lastland := ctr
		var t3 := 0.5
		while t3 <= 3.0:
			var pp3: Vector2 = ctr + dirv * t3
			if _water_at(int(pp3.x), int(pp3.y)):
				if t3 < bestw:
					bestw = t3
					wdir = dirv
					wpt = lastland
				break
			lastland = pp3
			t3 += 0.5
	if bestw < 3.0 and (tier >= 2 or ring_rad > 0.0):
		var wside := Vector2(-wdir.y, wdir.x)
		quays.append({"a": wpt, "d": wdir})
		if tier >= 3 or ring_rad > 0.0:
			quays.append({"a": wpt + wside * 0.65, "d": wdir})
			boat = {"c": wpt + wdir * 1.55 + wside * -0.55, "ax": wside}
	# ── les ÉDIFICES LOGIQUES : chaque bâtiment a une RAISON d'être là — l'entrepôt dort
	#    près des barques, la roue du moulin trempe au fil de l'eau, la grange borde les
	#    lanières, le moulin à vent prend le large des champs, la forge FUME en ville. ──
	if not quays.is_empty():
		var qd0: Dictionary = quays[0]
		var wside2 := Vector2(-(qd0["d"] as Vector2).y, (qd0["d"] as Vector2).x)
		houses.append({"w": (qd0["a"] as Vector2) - (qd0["d"] as Vector2) * 0.55 + wside2 * -0.5,
			"s": 0.44, "k": 4, "v": 1})                       # ENTREPÔT (long, toit ardoise)
		if tier >= 3 or ring_rad > 0.0:
			houses.append({"w": (qd0["a"] as Vector2) + wside2 * 1.15, "s": 0.40, "k": 5, "v": 0})  # MOULIN À EAU
	if not fields.is_empty():
		var f0: Dictionary = fields[0]
		houses.append({"w": (f0["c"] as Vector2).lerp(bc, 0.35), "s": 0.42, "k": 4, "v": 0})        # GRANGE
		if quays.is_empty() and tier >= 2:
			houses.append({"w": (f0["c"] as Vector2) + (f0["d"] as Vector2) * (float(f0["hl"]) + 0.9),
				"s": 0.44, "k": 3, "v": 0})                   # MOULIN À VENT
	if tier >= 3 or ring_rad > 0.0:
		var fi := int(_h1(float(r) * 44.1) * float(n))        # la FORGE : une maison de rue qui fume
		if fi < houses.size() and int(houses[fi]["k"]) == 0:
			houses[fi]["f"] = 1
	# tri du FOND vers l'AVANT — APRÈS tous les ajouts (les recouvrements lisent bien)
	houses.sort_custom(func(a, b): return (a["w"].x + a["w"].y) < (b["w"].x + b["w"].y))
	var pal: Array = ROOF_PAL[int(_h1(float(r) * 31.7) * 3.0) % 3]
	return {"h": houses, "arcs": arcs, "towers": towers, "gates": gates, "ring_c": bc,
		"gnd": gnd, "fields": fields, "pal": pal,
		"plaza": plaza, "well": well, "has_plaza": has_plaza, "quays": quays, "boat": boat}

## les POINTS d'une maison à pignon (repère local tourné) — partagés entre l'OMBRE
## (silhouette décalée) et la maison elle-même.
func _house_pts(p: Vector2, half: float, tilt: float) -> Dictionary:
	var ca := cos(tilt)
	var sa := sin(tilt)
	var rot := func(v: Vector2) -> Vector2:
		return p + Vector2(v.x * ca - v.y * sa, v.x * sa + v.y * ca) * half
	return {
		"a": rot.call(Vector2(-1.0, 0.75)),  "b": rot.call(Vector2(1.0, 0.75)),
		"c": rot.call(Vector2(1.0, -0.30)),  "d": rot.call(Vector2(1.18, -0.30)),
		"e": rot.call(Vector2(0.0, -1.25)),  "f": rot.call(Vector2(-1.18, -0.30)),
		"g": rot.call(Vector2(-1.0, -0.30)), "m": rot.call(Vector2(0.0, -0.30)),
	}

## l'OMBRE PORTÉE d'une maison : sa silhouette, décalée vers le SE (soleil de NO) — assoit
## le bâti sur le parchemin. Passe SÉPARÉE (toutes les ombres sous toutes les maisons).
func _house_shadow(p: Vector2, half: float, tilt: float) -> void:
	var off := Vector2(0.34, 0.30) * half
	var q := _house_pts(p + off, half, tilt)
	draw_colored_polygon(PackedVector2Array([q["a"], q["b"], q["c"], q["d"], q["e"], q["f"], q["g"]]), TOWN_SHADOW)

## une MAISON à pignon (encre + lavis), quasi droite (jitter ±7°) — la rangée suit la rue,
## les pignons restent debout comme sur les vignettes d'atlas. `half` en unités monde.
## Toit DEUX-TONS (versant NO éclairé / SE ombré — le soleil des cartes) + PORTE au zoom franc.
func _ink_house(p: Vector2, half: float, tilt: float, roof: Color, zoom: float) -> void:
	var q := _house_pts(p, half, tilt)
	draw_colored_polygon(PackedVector2Array([q["a"], q["b"], q["c"], q["g"]]), TOWN_WALL)
	draw_colored_polygon(PackedVector2Array([q["f"], q["m"], q["e"]]), roof.lightened(0.16))   # versant au soleil
	draw_colored_polygon(PackedVector2Array([q["m"], q["d"], q["e"]]), roof.darkened(0.14))    # versant à l'ombre
	if half * zoom > 3.4:   # la PORTE n'éclot qu'au zoom franc (sinon bruit)
		var db: Vector2 = (q["a"] as Vector2).lerp(q["b"], 0.5)
		var dt: Vector2 = db + ((q["m"] as Vector2) - db) * 0.42
		var dhw: Vector2 = ((q["b"] as Vector2) - (q["a"] as Vector2)) * 0.10
		draw_colored_polygon(PackedVector2Array([db - dhw, db + dhw, dt + dhw, dt - dhw]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.75))
	draw_polyline(PackedVector2Array([q["a"], q["b"], q["c"], q["d"], q["e"], q["f"], q["g"], q["a"]]),
		TOWN_INK, _w(zoom, 0.05, 0.45, 0.85), true)

## ÉGLISE (nef + flèche + croix) / DONJON (tour crénelée + fanion AU PIGMENT DU PAYS) /
## MOULIN À VENT (tour + ailes) / GRANGE-ENTREPÔT (long corps bas) / MOULIN À EAU (roue) —
## les monuments & édifices logiques, mêmes encres. Ombre portée incluse (masse au sol).
func _ink_landmark(p: Vector2, half: float, kind: int, zoom: float, pen: Color) -> void:
	var iw := _w(zoom, 0.06, 0.5, 0.95)
	# ombre de masse (décalée SE, comme les maisons)
	var so := Vector2(0.30, 0.26) * half
	if kind == 3:
		# MOULIN À VENT : tour trapèze + calotte + QUATRE AILES en croix
		var mt := half * 0.42
		var tower3 := PackedVector2Array([p + Vector2(-mt, half * 0.75), p + Vector2(mt, half * 0.75),
			p + Vector2(mt * 0.62, -half * 0.95), p + Vector2(-mt * 0.62, -half * 0.95)])
		draw_colored_polygon(PackedVector2Array([tower3[0] + so, tower3[1] + so, tower3[2] + so, tower3[3] + so]), TOWN_SHADOW)
		draw_colored_polygon(tower3, TOWN_WALL)
		draw_polyline(PackedVector2Array([tower3[0], tower3[1], tower3[2], tower3[3], tower3[0]]), TOWN_INK, iw, true)
		draw_colored_polygon(PackedVector2Array([p + Vector2(-mt * 0.66, -half * 0.95),
			p + Vector2(mt * 0.66, -half * 0.95), p + Vector2(0, -half * 1.28)]), Color(0.42, 0.30, 0.20, 0.94))
		var hub := p + Vector2(0, -half * 1.02)
		for k in range(4):
			var wa := PI * 0.25 + float(k) * PI * 0.5
			draw_line(hub, hub + Vector2(cos(wa), sin(wa)) * half * 1.15, TOWN_INK, iw, true)
		return
	if kind == 4:
		# GRANGE / ENTREPÔT : long corps bas, toit en croupe (bois v=0 · ardoise v=1 via pen? non —
		# le toit suit TOWN_ROOF2/ardoise selon l'appelant ; ici bois patiné, sobre)
		var gw2 := half * 1.7
		var gh := half * 0.62
		var body := PackedVector2Array([p + Vector2(-gw2, gh), p + Vector2(gw2, gh),
			p + Vector2(gw2, -gh * 0.4), p + Vector2(-gw2, -gh * 0.4)])
		draw_colored_polygon(PackedVector2Array([body[0] + so, body[1] + so, body[2] + so, body[3] + so]), TOWN_SHADOW)
		draw_colored_polygon(body, TOWN_WALL)
		var roof4 := PackedVector2Array([p + Vector2(-gw2 * 1.06, -gh * 0.4), p + Vector2(gw2 * 1.06, -gh * 0.4),
			p + Vector2(gw2 * 0.62, -gh * 1.35), p + Vector2(-gw2 * 0.62, -gh * 1.35)])
		draw_colored_polygon(roof4, Color(0.44, 0.32, 0.22, 0.94))
		draw_polyline(PackedVector2Array([body[0], body[1], roof4[1], roof4[2], roof4[3], roof4[0], body[0]]),
			TOWN_INK, iw, true)
		return
	if kind == 5:
		# MOULIN À EAU : petite maison + ROUE à aubes sur le flanc
		var q5 := _house_pts(p, half * 0.9, 0.0)
		draw_colored_polygon(PackedVector2Array([q5["a"], q5["b"], q5["c"], q5["g"]]), TOWN_WALL)
		draw_colored_polygon(PackedVector2Array([q5["f"], q5["m"], q5["e"]]), Color(0.46, 0.33, 0.21, 0.94).lightened(0.12))
		draw_colored_polygon(PackedVector2Array([q5["m"], q5["d"], q5["e"]]), Color(0.46, 0.33, 0.21, 0.94).darkened(0.12))
		draw_polyline(PackedVector2Array([q5["a"], q5["b"], q5["c"], q5["d"], q5["e"], q5["f"], q5["g"], q5["a"]]),
			TOWN_INK, iw, true)
		var wc := p + Vector2(-half * 1.18, half * 0.25)
		draw_arc(wc, half * 0.55, 0.0, TAU, 16, TOWN_INK, iw, true)
		for k in range(4):
			var sa2 := PI * 0.25 + float(k) * PI * 0.5
			draw_line(wc - Vector2(cos(sa2), sin(sa2)) * half * 0.5,
				wc + Vector2(cos(sa2), sin(sa2)) * half * 0.5, TOWN_INK, iw * 0.8, true)
		return
	if kind == 1:
		draw_colored_polygon(PackedVector2Array([p + so + Vector2(-half, half * 0.7),
			p + so + Vector2(half, half * 0.7), p + so + Vector2(half, -half * 0.3),
			p + so + Vector2(0, -half * 2.1), p + so + Vector2(-half, -half * 0.3)]), TOWN_SHADOW)
		# nef basse + flèche haute + croix
		var nave := PackedVector2Array([p + Vector2(-half, half * 0.7), p + Vector2(half, half * 0.7),
			p + Vector2(half, -half * 0.3), p + Vector2(-half, -half * 0.3)])
		draw_colored_polygon(nave, TOWN_WALL)
		var spire := PackedVector2Array([p + Vector2(-half * 0.32, -half * 0.3),
			p + Vector2(half * 0.32, -half * 0.3), p + Vector2(0, -half * 2.1)])
		draw_colored_polygon(spire, Color(0.34, 0.33, 0.36, 0.94))
		draw_polyline(PackedVector2Array([p + Vector2(-half, half * 0.7), p + Vector2(half, half * 0.7),
			p + Vector2(half, -half * 0.3), p + Vector2(half * 0.32, -half * 0.3), p + Vector2(0, -half * 2.1),
			p + Vector2(-half * 0.32, -half * 0.3), p + Vector2(-half, -half * 0.3), p + Vector2(-half, half * 0.7)]),
			TOWN_INK, iw, true)
		draw_line(p + Vector2(0, -half * 2.1), p + Vector2(0, -half * 2.45), TOWN_INK, iw, true)
		draw_line(p + Vector2(-half * 0.14, -half * 2.3), p + Vector2(half * 0.14, -half * 2.3), TOWN_INK, iw, true)
	else:
		var tw := half * 0.72
		draw_colored_polygon(PackedVector2Array([p + so + Vector2(-tw, half * 0.7),
			p + so + Vector2(tw, half * 0.7), p + so + Vector2(tw, -half * 1.5),
			p + so + Vector2(-tw, -half * 1.5)]), TOWN_SHADOW)
		# donjon : tour CRÉNELÉE + fanion au pigment du PAYS
		var tower := PackedVector2Array([p + Vector2(-tw, half * 0.7), p + Vector2(tw, half * 0.7),
			p + Vector2(tw, -half * 1.5), p + Vector2(-tw, -half * 1.5)])
		draw_colored_polygon(tower, TOWN_WALL)
		var top := -half * 1.5
		var cren := PackedVector2Array([p + Vector2(-tw, half * 0.7), p + Vector2(-tw, top)])
		var nt := 4
		for k in range(nt):                        # créneaux : dents carrées sur le parapet
			var x0 := -tw + tw * 2.0 * float(k) / float(nt)
			var x1 := x0 + tw * 2.0 / float(nt) * 0.55
			cren.push_back(p + Vector2(x0, top))
			cren.push_back(p + Vector2(x0, top - half * 0.22))
			cren.push_back(p + Vector2(x1, top - half * 0.22))
			cren.push_back(p + Vector2(x1, top))
		cren.push_back(p + Vector2(tw, top))
		cren.push_back(p + Vector2(tw, half * 0.7))
		cren.push_back(p + Vector2(-tw, half * 0.7))
		draw_polyline(cren, TOWN_INK, iw, true)
		draw_line(p + Vector2(0, top - half * 0.22), p + Vector2(0, -half * 2.4), TOWN_INK, iw, true)
		draw_colored_polygon(PackedVector2Array([p + Vector2(0, -half * 2.4),
			p + Vector2(half * 0.62, -half * 2.12), p + Vector2(0, -half * 1.88)]),
			Color(pen.r, pen.g, pen.b, 0.95))

## charge (paresseux) une MARQUE DE TERRAIN par id → Texture2D (cache). Cherche dans lot 3 (biomes plats/
## eau) PUIS lot 2 (relief/forêt/désert) — les ids sont uniques entre lots. Fallback Image.load (PNG brut).
func _dress_get(id: String) -> Texture2D:
	if _dress_tex.has(id):
		return _dress_tex[id]
	var tex := _dress_load("res://art/map_stamps/lot3_biomes/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot2_painted/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot4_easter_eggs/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot5_kcd/oriented_16/forests/%s.png" % id)   # lot 5 : masses de forêt
	_dress_tex[id] = tex
	return tex

func _dress_load(path: String) -> Texture2D:
	if ResourceLoader.exists(path):
		return load(path)
	if FileAccess.file_exists(path):              # garde : pas d'Image.load sur un fichier absent (≠ spam d'erreurs)
		var img := Image.new()
		if img.load(path) == OK:
			return ImageTexture.create_from_image(img)
	return null

## taille à l'ÉCRAN (px) d'une marque selon sa famille (montagnes grandes, herbe de plaine petite).
func _dress_size(id: String) -> float:
	if id.begins_with("sea_serpent"): return 84.0          # lot 4 : serpent (largeur ×2 au tracé → 2:1)
	if id.begins_with("forest_mass"): return 54.0          # lot 5 : MASSE de canopée (grande, remplit le bloc)
	if id.begins_with("forest_edge"): return 44.0          # lot 5 : lisière allongée
	if id.begins_with("mountain_range"): return 50.0
	if id.begins_with("mountain"): return 42.0
	if id.begins_with("forest"): return 38.0
	if id.begins_with("dune") or id.begins_with("sea_") or id.begins_with("ocean") or id.begins_with("water"): return 34.0
	if id.begins_with("apoc_rabbit"): return 32.0          # lot 4 : lapin marginalia
	if id.begins_with("hill"): return 30.0
	if id.begins_with("shipwreck") or id.begins_with("broken") or id.begins_with("half_sunk") or id.begins_with("floating") \
	   or id.begins_with("jagged") or id.begins_with("low_rocks") or id.begins_with("sea_stacks") or id.begins_with("shoal"): return 30.0  # épaves/récifs
	if id.begins_with("savanna") or id.begins_with("acacia") or id.begins_with("steppe") or id.begins_with("marsh"): return 28.0
	if id.begins_with("tree") or id.begins_with("reeds") or id.begins_with("rocky"): return 26.0
	if id.begins_with("plain"): return 24.0
	return 24.0

## SÈME les marques de terrain par BIOME (grille jittée déterministe), une fois à la génération. Display-only.
func _build_dressing() -> void:
	_dressing.clear()
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(LAYER_BIOME)
	if bio == null:
		return
	var rf: Image = _carved_river_field()      # champ rivière → on N'ENTASSE PAS de marques sur les fleuves
	# la CLAIRIÈRE DES BOURGS : aucune marque de terrain dans le rayon d'un lieu HABITÉ
	# (les arbres ne poussent pas sur les toits ; l'urbaniste y pose sa terre battue).
	_dress_clear.clear()
	for r in range(w.region_count()):
		var tier: int = w.region_tier(r)
		var owner: int = w.region_owner(r)
		var role: int = int(w.country_role(owner)) if owner >= 0 else -1
		if tier < 0 and role != 2 and role != 4:
			continue
		var c: Vector2 = _region_seat.get(r, w.region_centroid(r))
		if c.x < 0:
			continue
		var rad := 2.3 + 0.5 * float(maxi(tier, 1)) + (1.4 if role == 2 else 0.0)
		_dress_clear.append([c, rad * rad])
	var sw := bio.get_width()
	var sh := bio.get_height()
	var i := 0
	var y := roundi(DRESS_SPACING * 0.5)
	while y < sh:
		var x := roundi(DRESS_SPACING * 0.5)
		while x < sw:
			# 1 + N passes selon le biome de la cellule (forêts = canopée DENSE → plusieurs marques/cellule).
			var bb := int(bio.get_pixel(clampi(x, 0, sw - 1), clampi(y, 0, sh - 1)).r * 255.0 + 0.5)
			var passes := 1 + int(DRESS_EXTRA.get(bb, 0))
			for p in range(passes):
				i += 1
				_try_place_dress(i, x, y, bio, rf, sw, sh)
			x += DRESS_SPACING
		y += DRESS_SPACING
	_build_easter_eggs(bio, rf, sw, sh)        # lot 4 : serpents/épaves/récifs/lapins (rares)
	# TRI par id → les marques de même texture sont DESSINÉES À LA SUITE (le batcher 2D les fusionne) : perf
	# tenable malgré le grand nombre de marques (canopée dense).
	_dressing.sort_custom(func(a, b): return String(a["id"]) < String(b["id"]))

## tente UNE marque jittée à partir de (x,y) : hors rivière, biome connu, sous la densité → ajoutée.
func _try_place_dress(i: int, x: int, y: int, bio: Image, rf: Image, sw: int, sh: int) -> void:
	var jx := int((_h1(float(i) * 1.7) - 0.5) * float(DRESS_SPACING))
	var jy := int((_h1(float(i) * 3.3) - 0.5) * float(DRESS_SPACING))
	var px := clampi(x + jx, 0, sw - 1)
	var py := clampi(y + jy, 0, sh - 1)
	if _near_river(rf, px, py):
		return                                 # JAMAIS sur/au bord d'une rivière (sinon elle transparaît sous la marque)
	for cl in _dress_clear:                    # ni dans la CLAIRIÈRE d'un bourg (l'urbaniste y règne)
		if (cl[0] as Vector2).distance_squared_to(Vector2(px, py)) < float(cl[1]):
			return
	var b := int(bio.get_pixel(px, py).r * 255.0 + 0.5)
	if not DRESS_BY_BIOME.has(b):
		return
	var dens: float = DRESS_DENSITY.get(b, 0.6)
	if _h1(float(i) * 5.1) > dens:
		return
	var ids: Array = DRESS_BY_BIOME[b]
	var id: String = ids[int(_h1(float(i) * 7.7) * float(ids.size())) % ids.size()]
	var scl := 0.85 + 0.30 * _h1(float(i) * 9.9)   # 0.85..1.15 : variété d'échelle
	_dressing.append({"pos": Vector2(px, py), "id": id, "scale": scl})

## LOT 4 — easter eggs RARES : serpent sur l'océan profond (cap 3), épave/récif sur le haut-fond, lapin
## marginalia sur terre (cap 2). Grille grossière + faibles probas → rares mais présents.
func _build_easter_eggs(bio: Image, rf: Image, sw: int, sh: int) -> void:
	var serp := 0
	var rab := 0
	var i := 100000
	var y := roundi(EGG_SPACING * 0.5)
	while y < sh:
		var x := roundi(EGG_SPACING * 0.5)
		while x < sw:
			i += 1
			var b := int(bio.get_pixel(clampi(x, 0, sw - 1), clampi(y, 0, sh - 1)).r * 255.0 + 0.5)
			var r := _h1(float(i) * 2.13)
			if b == 0 and serp < 3 and r < 0.10:                       # OCÉAN PROFOND → serpent
				var sid := "sea_serpent_01" if _h1(float(i) * 4.4) < 0.5 else "sea_serpent_02"
				_dressing.append({"pos": Vector2(x, y), "id": sid, "scale": 1.0, "egg": true})
				serp += 1
			elif b == 2 and r < 0.05:                                  # HAUT-FOND → épave/récif
				var wid: String = EGG_WRECKS[int(_h1(float(i) * 6.6) * float(EGG_WRECKS.size())) % EGG_WRECKS.size()]
				_dressing.append({"pos": Vector2(x, y), "id": wid, "scale": 1.0, "egg": true})
			elif b >= 4 and b <= 9 and rab < 2 and r > 0.99 and not _near_river(rf, x, y):  # TERRE → lapin (ultra-rare)
				var rid: String = EGG_RABBITS[int(_h1(float(i) * 8.8) * float(EGG_RABBITS.size())) % EGG_RABBITS.size()]
				_dressing.append({"pos": Vector2(x, y), "id": rid, "scale": 1.0, "egg": true})
				rab += 1
			x += EGG_SPACING
		y += EGG_SPACING

## tier de tampon (1-7) d'après la POPULATION (paliers CITY_POP_BANDS) → un VRAI étalement de tailles
## (le region_tier de la façade se tasse à 4 ; la pop donne la variété t1..t7 demandée).
func _pop_tier(pop: int) -> int:
	var t := 1
	for thr in CITY_POP_BANDS:
		if pop >= thr:
			t += 1
		else:
			break
	return clampi(t, 1, 7)

## TAMPON D'ATLAS d'une région, CENTRÉ sur le siège. Cité-état & hameau libre → assets DÉDIÉS
## (`city_state` / `wild_hamlet`). Cités normales → `city_t1..t4` selon la POP (PAS de t5-t7 : pas de
## faux air de capitale ; la VRAIE capitale est déjà désignée par le liseré pourpre). Pas de variante
## portuaire. Repli sur le glyphe d'encre si le tampon manque. Display-only.
func _draw_settlement(w, r: int, role: int, ctr: Vector2, ip: Vector2, zoom: float, mv) -> void:
	var is_cs := role == 2
	var is_wild := role == 4
	var st := mini(_pop_tier(int(w.region_pop(r))), 4)
	# ── L'URBANISTE : le PLAN du bourg (cache par région) — maisons rangées sur la route.
	#    Le plan se REBÂTIT quand le TIER change : le bourg GRANDIT avec sa population. ──
	if not _town_cache.has(r) or int(_town_cache[r].get("st", -1)) != st:
		if _roads.is_empty():
			_draw_town(ip, maxi(st - 1, 1), zoom, Color(0.20, 0.14, 0.09, 0.95))   # routes pas prêtes : glyphe, sans figer
			return
		var n: int = 3 if is_wild else (22 if is_cs else [4, 4, 8, 13, 18][st])
		var owner0: int = w.region_owner(r)
		var is_cap: bool = owner0 >= 0 and w.province_region(w.country_capital_province(owner0)) == r
		var landmark := 2 if is_cap else (1 if (is_cs or st >= 3) else 0)
		var ring := 2.4 if is_cs else 0.0
		var plan := _build_town(r, ctr, n, landmark, ring, st)
		plan["st"] = st
		_town_cache[r] = plan
	var town: Dictionary = _town_cache[r]
	var pal: Array = town["pal"]
	# 1. la CLAIRIÈRE — le sol du bourg, un lavis de terre battue au bord irrégulier
	var gnd: PackedVector2Array = town["gnd"]
	if gnd.size() > 2:
		var gpts := PackedVector2Array()
		gpts.resize(gnd.size())
		for k in range(gnd.size()):
			gpts[k] = mv.iso_pos(gnd[k].x, gnd[k].y)
		draw_colored_polygon(gpts, TOWN_GROUND)
	# 2. les CHAMPS EN LANIÈRES — lavis paille + 3 sillons, aux abouts de la rue (t2+)
	for fd in town["fields"]:
		var fq: PackedVector2Array = fd["q"]
		var fpts := PackedVector2Array()
		fpts.resize(fq.size())
		for k in range(fq.size()):
			fpts[k] = mv.iso_pos(fq[k].x, fq[k].y)
		draw_colored_polygon(fpts, FIELD_FILL)
		draw_polyline(PackedVector2Array([fpts[0], fpts[1], fpts[2], fpts[3], fpts[0]]),
			Color(FIELD_FURROW.r, FIELD_FURROW.g, FIELD_FURROW.b, 0.22), _w(zoom, 0.04, 0.3, 0.6), true)
		var fc2: Vector2 = fd["c"]
		var fdir: Vector2 = fd["d"]
		var fl2: float = fd["hl"]
		var fw2: float = fd["hw"]
		var fperp := Vector2(-fdir.y, fdir.x)
		for s2 in [-0.5, 0.0, 0.5]:                       # les SILLONS (3 traits le long de la lanière)
			var o2: Vector2 = fperp * (float(s2) * fw2 * 1.3)
			draw_line(mv.iso_pos(fc2.x + fdir.x * fl2 * 0.85 + o2.x, fc2.y + fdir.y * fl2 * 0.85 + o2.y),
				mv.iso_pos(fc2.x - fdir.x * fl2 * 0.85 + o2.x, fc2.y - fdir.y * fl2 * 0.85 + o2.y),
				FIELD_FURROW, _w(zoom, 0.035, 0.3, 0.5), true)
	# 2bis. la PLACE DE MARCHÉ (carrefour) : octogone de terre claire + PUITS au centre
	var plaza: PackedVector2Array = town.get("plaza", PackedVector2Array())
	if plaza.size() > 2:
		var ppts := PackedVector2Array()
		ppts.resize(plaza.size())
		for k in range(plaza.size()):
			ppts[k] = mv.iso_pos(plaza[k].x, plaza[k].y)
		draw_colored_polygon(ppts, PLAZA_FILL)
		draw_polyline(PackedVector2Array(Array(ppts) + [ppts[0]]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.18), _w(zoom, 0.04, 0.3, 0.6), true)
		var wl: Vector2 = town["well"]
		var wip: Vector2 = mv.iso_pos(wl.x, wl.y)
		var wr := _w(zoom, 0.13, 1.0, 2.2)
		draw_circle(wip, wr, TOWN_WALL)
		draw_circle(wip, wr * 0.45, Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.80))
		draw_arc(wip, wr, 0.0, TAU, 14, TOWN_INK, _w(zoom, 0.05, 0.4, 0.8), true)
	# 2ter. les QUAIS : jetées de bois dans l'eau + barque amarrée — le bourg regarde le large
	for qd in town.get("quays", []):
		var qa: Vector2 = qd["a"]
		var qv: Vector2 = qd["d"]
		var q0: Vector2 = mv.iso_pos(qa.x, qa.y)
		var q1: Vector2 = mv.iso_pos(qa.x + qv.x * 1.15, qa.y + qv.y * 1.15)
		var qt := (q1 - q0).normalized()
		var qp := Vector2(-qt.y, qt.x)
		var qw := _w(zoom, 0.10, 0.8, 1.8)
		draw_colored_polygon(PackedVector2Array([q0 + qp * qw, q1 + qp * qw, q1 - qp * qw, q0 - qp * qw]), QUAY_WOOD)
		draw_polyline(PackedVector2Array([q0 + qp * qw, q1 + qp * qw, q1 - qp * qw, q0 - qp * qw, q0 + qp * qw]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.65), _w(zoom, 0.035, 0.3, 0.6), true)
		for ps in [0.3, 0.65, 1.0]:                       # les PIEUX (pointillés d'encre au bord)
			var pp4: Vector2 = q0.lerp(q1, float(ps))
			draw_circle(pp4 + qp * qw, _w(zoom, 0.03, 0.3, 0.55), TOWN_INK)
			draw_circle(pp4 - qp * qw, _w(zoom, 0.03, 0.3, 0.55), TOWN_INK)
	var bt: Dictionary = town.get("boat", {})
	if bt.has("c"):
		var bc: Vector2 = bt["c"]
		var bax: Vector2 = bt["ax"]
		var b0: Vector2 = mv.iso_pos(bc.x, bc.y)
		var bxi: Vector2 = (mv.iso_pos(bc.x + bax.x, bc.y + bax.y) - b0).normalized()
		var bpi := Vector2(-bxi.y, bxi.x)
		var bl := _w(zoom, 0.30, 2.0, 4.4)
		var bw2 := _w(zoom, 0.11, 0.8, 1.7)
		var hull := PackedVector2Array([b0 - bxi * bl * 0.8 + bpi * bw2 * 0.7, b0 + bxi * bl * 0.55 + bpi * bw2,
			b0 + bxi * bl, b0 + bxi * bl * 0.55 - bpi * bw2, b0 - bxi * bl * 0.8 - bpi * bw2 * 0.7])
		draw_colored_polygon(hull, BOAT_WOOD)
		draw_polyline(PackedVector2Array(Array(hull) + [hull[0]]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.70), _w(zoom, 0.035, 0.3, 0.6), true)
		draw_line(b0, b0 + Vector2(0, -bl * 0.9), Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.75),
			_w(zoom, 0.035, 0.3, 0.6), true)              # le mât nu (barque amarrée)
	# 3. l'ENCEINTE — arcs de muraille COUPÉS AUX PORTES (routes), tours crénelées ponctuelles
	for arc in town["arcs"]:
		var apts := PackedVector2Array()
		apts.resize((arc as PackedVector2Array).size())
		for k in range((arc as PackedVector2Array).size()):
			apts[k] = mv.iso_pos(arc[k].x, arc[k].y)
		# la MURAILLE en RUBAN DE PIERRE (3 passes) : ombre portée large → pierre crème → arête d'encre
		draw_polyline(apts, Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.35), _w(zoom, 0.30, 1.8, 3.8), true)
		draw_polyline(apts, Color(TOWN_WALL.r, TOWN_WALL.g, TOWN_WALL.b, 0.95), _w(zoom, 0.20, 1.2, 2.6), true)
		draw_polyline(apts, TOWN_INK, _w(zoom, 0.07, 0.5, 1.0), true)
	for tw2 in town["towers"]:
		var tp: Vector2 = mv.iso_pos((tw2 as Vector2).x, (tw2 as Vector2).y)
		var ts := _w(zoom, 0.19, 1.4, 3.0)
		draw_colored_polygon(PackedVector2Array([tp + Vector2(-ts, ts), tp + Vector2(ts, ts),
			tp + Vector2(ts, -ts), tp + Vector2(-ts, -ts)]), TOWN_WALL)
		draw_polyline(PackedVector2Array([tp + Vector2(-ts, ts), tp + Vector2(ts, ts),
			tp + Vector2(ts, -ts), tp + Vector2(-ts, -ts), tp + Vector2(-ts, ts)]),
			TOWN_INK, _w(zoom, 0.05, 0.45, 0.9), true)
	# 4. les OMBRES PORTÉES (toutes, sous toutes les maisons — le soleil de NO assoit le bourg)
	for hd in town["h"]:
		if int(hd["k"]) == 0:
			var wp0: Vector2 = hd["w"]
			var half0 := _w(zoom, float(hd["s"]), 1.9, 6.5)
			var tilt0 := (_h1(wp0.x * 12.9 + wp0.y * 7.1) - 0.5) * 0.24
			_house_shadow(mv.iso_pos(wp0.x, wp0.y), half0, tilt0)
	# 5. les MAISONS (toit deux-tons à la palette du bourg, porte au zoom franc) + MONUMENT
	var pen := _entity_pigment(w.region_owner(r)) if w.region_owner(r) >= 0 else TOWN_INK
	for hd in town["h"]:
		var wp: Vector2 = hd["w"]
		var p: Vector2 = mv.iso_pos(wp.x, wp.y)
		var half := _w(zoom, float(hd["s"]), 1.9, 6.5)     # taille monde, bornée en px écran
		var kind: int = hd["k"]
		if kind > 0:
			_ink_landmark(p, half * 1.35, kind, zoom, pen)
		else:
			var tilt := (_h1(wp.x * 12.9 + wp.y * 7.1) - 0.5) * 0.24   # ±7° : « dessiné à la main »
			var is_forge: bool = int(hd.get("f", 0)) == 1
			var rc: Color = pal[int(hd["v"]) % 2]
			if is_forge:
				rc = rc.darkened(0.22)                        # la FORGE : toit noirci de suie
			_ink_house(p, half, tilt, rc, zoom)
			# la FUMÉE de cheminée — la FORGE fume toujours (dès le zoom moyen) ; les autres
			# toits, ~1 sur 3 au TRÈS gros plan : un souffle gris chaud à peine posé.
			if (is_forge and zoom >= 6.0) or (zoom >= 9.0 and _h1(wp.x * 3.7 + wp.y * 9.2) < 0.34):
				var sp := p + Vector2(0.0, -1.30 * half)
				var curl := PackedVector2Array([sp,
					sp + Vector2(0.14 * half, -0.55 * half),
					sp + Vector2(-0.04 * half, -1.05 * half),
					sp + Vector2(0.20 * half, -1.60 * half)])
				draw_polyline(curl, Color(SMOKE_SOFT.r, SMOKE_SOFT.g, SMOKE_SOFT.b, SMOKE_SOFT.a * 0.5),
					_w(zoom, 0.11, 0.9, 1.9), true)
				draw_polyline(curl, SMOKE_SOFT, _w(zoom, 0.05, 0.4, 0.9), true)

## BANNIÈRE DE LIEU (référence KCD) : chip parchemin + liseré d'encre + NOM du siège +
## pastille au pigment du propriétaire — taille ÉCRAN constante, posée AU-DESSUS du tampon.
## Éclot au plan rapproché (a = fondu d'éclosion), le relais des noms de pays effacés.
func _draw_banner(w, r: int, ip: Vector2, zoom: float, a: float) -> void:
	if a <= 0.02:
		return
	if not _region_label.has(r):
		var nmv := ""
		var anc: Vector2 = _region_seat.get(r, Vector2(-1, -1))
		if anc.x >= 0 and w.has_method("province_at"):
			var pid: int = w.province_at(int(anc.x), int(anc.y))
			if pid >= 0:
				nmv = String(w.province_info(pid).get("nom", ""))
		_region_label[r] = nmv
	var nm: String = _region_label[r]
	if nm == "":
		return
	var sc := 1.0 / maxf(zoom, 0.0001)
	var tw := VKit.text_w(nm, VKit.FS_SMALL) * sc
	var bh := 14.0 * sc
	var hpad := 5.0 * sc
	var dotw := 7.0 * sc                                   # place de la pastille de propriétaire
	var bw := tw + hpad * 2.0 + dotw
	var top := ip.y - 34.0 * sc - bh                       # au-dessus du tampon (écran constant)
	var rect := Rect2(Vector2(ip.x - bw * 0.5, top), Vector2(bw, bh))
	# ombre portée + parchemin CLAIR + liseré : le chip se détache du terrain pâle (KCD)
	draw_rect(Rect2(rect.position + Vector2(1.2 * sc, 1.4 * sc), rect.size), Color(0.10, 0.07, 0.04, 0.35 * a))
	draw_rect(rect, Color(0.97, 0.93, 0.80, 0.94 * a))                       # le parchemin du chip
	draw_rect(rect, Color(0.25, 0.18, 0.10, 0.95 * a), false, 1.2 * sc)      # liseré d'encre franc
	var own := int(w.region_owner(r))
	var dot: Color = _entity_pigment(own) if own >= 0 else Color(0.52, 0.46, 0.36)
	draw_circle(Vector2(rect.position.x + hpad + 1.5 * sc, rect.position.y + bh * 0.5), 2.6 * sc, Color(dot, a))
	draw_set_transform(Vector2(rect.position.x + hpad + dotw, rect.position.y + 1.0 * sc), 0.0, Vector2(sc, sc))
	VKit.text(self, Vector2.ZERO, Color(0.20, 0.14, 0.08, 0.95 * a), nm, VKit.FS_SMALL)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

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
