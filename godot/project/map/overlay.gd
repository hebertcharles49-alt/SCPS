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
const CITY_ZOOM_MIN := 4.5   ## les villes n'apparaissent qu'AU-DELÀ de ce zoom (sinon : carte d'ensemble encombrée)
const BLD_SIZE := 9.0        ## taille MONDE UNIFORME d'un bâti de bourg (égalisée — variété par le sprite, pas l'échelle)
const LABEL_ZOOM_MAX := 6.5  ## les NOMS d'empire s'estompent À MESURE qu'on zoome (l'inverse des villes)

var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre
var _decor := []          ## [{name, pos}] — arbres/forêts (dressing nature), bâti au générate
var _structures := []     ## [{name, pos}] — bâti de terrain parsemé autour des villes
var _region_variant := {} ## région colonisée → nom de variante de ville TERRAIN (petits bourgs)
var _region_anchor := {}  ## région colonisée → assise de ville CALÉE SUR TERRE (centroïde snappé + rabat côtier)
var _region_citymax := {} ## région colonisée → plus grande taille de sprite de ville TENANT au sec (anti-débord mer)
var _bk := {}             ## noms de structures triés en bancs (civic/craft/dwell/field), calculé 1×
var _clear_set := {}      ## cellules DÉBOISÉES autour des villes (le bourg respire, pas noyé sous la forêt)
var _country_names := []  ## nom de chaque pays (figé au générate) — pour les étiquettes d'empire
var _borders := {}        ## niveau (1=région · 2=pays) → PackedVector2Array de segments (façade)
var _borders_dirty := true ## la souveraineté a bougé (conquête/colonisation) → refaire les frontières
var _owner_sig := -1      ## signature de la photo des propriétaires → détecte le changement de souveraineté
var _struct_dirty := false ## le bourg dépend de pop+bâtiments (évolue) → reconstruit à la demande
var _rivers := []         ## [Vector3(x, y, ang)] — fil de rivière (façade), figé au générate

const RIVER_ZOOM_MIN := 2.5   ## les rivières paraissent au-delà de ce zoom
const RIVER_CAL := 0.0        ## calibration de l'orientation du sprite (ajusté au rendu)

# biome (couche, valeurs Biome) → NOMS de sprites dressing. FOREST=12 · WOODS=13 · JUNGLE=14
const FOREST_TREES := {
	12: ["DRESS_TREE_OAK_SUMMER", "DRESS_TREE_OAK_GOLD", "DRESS_GROVE_BROADLEAF", "DRESS_TREE_POPLAR"],  # FOREST : feuillus
	13: ["DRESS_GROVE_MIXED", "DRESS_TREE_PINE", "DRESS_GROVE_PINE_A", "DRESS_GROVE_PINE_B"],            # WOODS : mixte/conifères
	14: ["DRESS_GROVE_BROADLEAF", "DRESS_TREE_GNARLED", "DRESS_GROVE_MIXED"],                            # JUNGLE : dense
}

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
		_rivers = Sim.world.river_points()
		_build_names()
		_build_anchors()
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
	_rivers = Sim.world.river_points()
	_borders_dirty = true       # monde neuf → frontières à refaire
	_owner_sig = -1
	_build_names()
	_build_anchors()
	_build_decor()
	_build_structures()
	_build_city_skins()
	queue_redraw()

## pré-calcule la variante de ville TERRAIN de chaque région colonisée (échantillon
## du biome au centroïde ; l'hydro via le groupe de settlement) — pour les petits bourgs.
func _build_city_skins() -> void:
	_region_variant.clear()
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(2)   # SCPS_LAYER_BIOME
	var sea: Image = w.layer_image(1)   # SCPS_LAYER_SEA
	for r in range(w.region_count()):
		if w.region_tier(r) < 0:
			continue
		var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))   # assise CALÉE SUR TERRE
		if ctr.x < 0:
			continue
		var nm := ""
		var sg: int = w.region_settle_group(r)
		if sg == 2:                       # estuaire
			nm = "CITY_BIOME_ESTUARY_STILTS"
		elif sg == 1:                     # rivière
			nm = "CITY_BIOME_RIVERBANK_QUAY"
		elif bio != null and ctr.x < bio.get_width() and ctr.y < bio.get_height():
			var b := int(bio.get_pixel(int(ctr.x), int(ctr.y)).r * 255.0 + 0.5)
			nm = BIOME_CITY.get(b, "")
		# si l'assise jouxte l'eau et qu'aucune variante AQUATIQUE n'a été choisie,
		# bascule sur le village de PÊCHE (sprite à quais) → une ville au bord de l'eau
		# lit toujours comme un PORT voulu, jamais comme un bâti générique débordant.
		if nm == "" and not _footprint_clear(sea, {}, ctr, 4.5, 9.0):
			nm = "CITY_BIOME_COAST_FISHING"
		if nm != "":
			_region_variant[r] = nm

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
	var sea: Image = w.layer_image(1)   # SCPS_LAYER_SEA : 0 = terre, ≥ 1 = eau
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
		var clr := int(15.0 + t * 2.2)
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
func _place_zone(pool: Array, count: int, ctr: Vector2, rbase: float, rspan: float,
		base_sz: float, flipok: bool, idx: int, jit: float, sea: Image, rset: Dictionary, r: int) -> int:
	if pool.is_empty():
		return idx
	for j in range(count):
		var ang := float(idx) * 2.399963 + jit          # angle d'or → étalement régulier
		var hh := ((r * 374761393) ^ (idx * 2246822519 + 668265263)) & 0x7fffffff
		var rad := rbase + float(hh % 100) / 100.0 * rspan
		var p := ctr + Vector2(cos(ang), sin(ang)) * rad
		var sz := base_sz                                 # taille UNIFORME : la variété vient du SPRITE (+miroir), pas de l'échelle
		idx += 1
		if not _footprint_clear(sea, rset, p, sz * 0.5, sz):
			continue                                      # déborde l'eau → on saute (ville sur terre)
		var nm: String = pool[(hh ^ (hh >> 5)) % pool.size()]   # pick mieux brassé
		_structures.append({"name": nm, "pos": p, "sz": sz, "flip": flipok and (((hh >> 17) & 1) == 1)})
		if _structures.size() >= 1400:
			break
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
	var sea: Image = w.layer_image(1)   # SCPS_LAYER_SEA : 0 = terre, ≥ 1 = eau (toute classe)
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
		var pop: int = w.region_pop(r)
		var nb := _region_craft_count(w, ctr)            # bâtiments posés (UI provinciale)
		# combien par zone — ∝ tier (civique), bâtiments (ateliers), population (logements)
		var civic_n := 1 + (1 if t >= 2 else 0) + (1 if t >= 4 else 0)
		var craft_n: int = clampi(nb, 0, 6)
		var dwell_n: int = clampi(int(pop / 250.0), 1, 6 + t * 2)
		var field_n: int = clampi(t - 1, 0, 3)           # le disparate : RARE, au large
		var jit := float((r * 2654435761) & 0xffff) / 65536.0 * TAU
		var idx := 0
		# zone : rayon · empan · taille · miroir. TAILLE UNIFORME (`BLD_SIZE`) pour tous —
		# la variété tient au SPRITE et au miroir, pas à l'échelle (bâtiments « égalisés »).
		# Civique non miroité (repères) ; logements/champs miroités (silhouettes doublées).
		idx = _place_zone(bk["civic"], civic_n, ctr, 1.5, 2.2, BLD_SIZE, false, idx, jit, sea, rset, r)
		idx = _place_zone(bk["craft"], craft_n, ctr, 2.8, 3.6, BLD_SIZE, false, idx, jit, sea, rset, r)
		idx = _place_zone(bk["dwell"], dwell_n, ctr, 3.2, 5.4, BLD_SIZE, true, idx, jit, sea, rset, r)
		idx = _place_zone(bk["field"], field_n, ctr, 7.6, 4.8, BLD_SIZE, true, idx, jit, sea, rset, r)
		if _structures.size() >= 1400:
			break
	# tri arrière→avant (par y) → l'empilement du bourg se lit correctement
	_structures.sort_custom(func(a, b): return a["pos"].y < b["pos"].y)

## scan de la couche biome (un pas régulier) → liste fixe d'arbres sur les forêts.
func _build_decor() -> void:
	_decor.clear()
	var w = Sim.world
	if w == null:
		return
	var img: Image = w.layer_image(2)   # SCPS_LAYER_BIOME = 2
	if img == null:
		return
	var mw := img.get_width()
	var mh := img.get_height()
	var stride := 4                       # forêt DENSE (ex-7) : un arbre toutes les 4 cellules
	for cy in range(0, mh, stride):
		for cx in range(0, mw, stride):
			var b := int(img.get_pixel(cx, cy).r * 255.0 + 0.5)
			if not FOREST_TREES.has(b):
				continue
			if _clear_set.has(Vector2i(cx, cy)):
				continue                       # clairière de bourg → pas d'arbre ici
			var h := ((cx * 73856093) ^ (cy * 19349663)) & 0x7fffffff
			var arr: Array = FOREST_TREES[b]
			var jx := float((h >> 3) % 3) - 1.0
			var jy := float((h >> 6) % 3) - 1.0
			_decor.append({"name": arr[h % arr.size()], "pos": Vector2(cx + jx, cy + jy)})
			if _decor.size() >= 14000:
				return

func _on_tick(_year: int) -> void:
	_struct_dirty = true       # pop & bâtiments ont bougé → le bourg sera reconstruit au prochain dessin zoomé
	var sig := _owner_signature(Sim.world)
	if sig != _owner_sig:      # la souveraineté a changé → refaire les frontières
		_owner_sig = sig
		_borders_dirty = true
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

## dessine UN bâti parsemé — à SA taille, ancré au pied, MIROIR éventuel (variété).
func _draw_struct(s: Dictionary) -> void:
	var sspr := UIKit.structure_sprite(s["name"])
	if sspr == null:
		return
	var sp: Vector2 = s["pos"]
	var ss: float = s.get("sz", 9.0)
	if bool(s.get("flip", false)):
		draw_set_transform(sp, 0.0, Vector2(-1, 1))             # miroir horizontal autour du pied
		draw_texture_rect(sspr, Rect2(-ss * 0.5, -ss, ss, ss), false)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	else:
		draw_texture_rect(sspr, Rect2(sp - Vector2(ss * 0.5, ss), Vector2(ss, ss)), false)

## dessine LA ville de la région `r` à l'assise `ctr` — variante TERRAIN sinon bande de
## pop ; taille ∝ tier BORNÉE au sec ; repli en marqueur sobre si l'assise est trop pincée.
func _draw_city(w, r: int, ctr: Vector2) -> void:
	var t: int = w.region_tier(r)
	var band := _city_band(w.region_pop(r))
	var spr: Texture2D = null
	if _region_variant.has(r):
		spr = UIKit.city_biome(_region_variant[r])
	if spr == null:
		spr = UIKit.city_sprite(band, (r * 2654435761) % 8)
	var want := 16.0 + t * 6.0
	var sz: float = min(want, _region_citymax.get(r, want))
	if spr != null and sz >= 6.0:
		draw_texture_rect(spr, Rect2(ctr - Vector2(sz * 0.5, sz), Vector2(sz, sz)), false)  # ancré au pied
	else:
		# assise trop pincée pour le moindre sprite (îlot/langue de terre) → marqueur SOBRE
		var radius := 1.2 + t * 0.4
		draw_circle(ctr, radius, Color(0.62, 0.47, 0.30))
		draw_arc(ctr, radius, 0.0, TAU, 16, Color(0.15, 0.10, 0.05, 0.8), 0.4, true)

func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	var zoom := get_viewport_transform().get_scale().x

	# ── RIVIÈRES : un sprite TOURNÉ le long du fil (sous tout le reste), au zoom.
	#    Culling par viewport (seuls les points visibles sont tracés). ─────────────
	if zoom >= RIVER_ZOOM_MIN and not _rivers.is_empty():
		var rtex := UIKit.river_sprite()
		if rtex != null:
			var sc := 3.2 / float(rtex.get_width())     # ~3.2 unités-monde de large
			var half := rtex.get_size() * 0.5
			var vt := get_viewport_transform()
			var vp := get_viewport_rect().size
			for p in _rivers:                            # Vector3(x, y, ang)
				var sp: Vector2 = vt * Vector2(p.x, p.y)
				if sp.x < -30 or sp.x > vp.x + 30 or sp.y < -30 or sp.y > vp.y + 30:
					continue                             # hors champ
				draw_set_transform(Vector2(p.x, p.y), p.z + RIVER_CAL, Vector2(sc, sc))
				draw_texture(rtex, -half)
			draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# ── DRESSING : arbres/forêts (sprites nature RGBA), DERRIÈRE les villes. Bâti une
	#    fois au générate (liste fixe, pas de culling fragile). Ancré au PIED de la
	#    cellule (l'arbre « pousse » du sol). No-op si les assets manquent. ──────────
	for d in _decor:
		var spr := UIKit.dressing_named(d["name"])
		if spr == null:
			break
		var p: Vector2 = d["pos"]
		var ts := 10.0
		draw_texture_rect(spr, Rect2(p - Vector2(ts * 0.5, ts), Vector2(ts, ts)), false)

	# ── FRONTIÈRES (port du balayage bseg de viewer.c) : segments d'arête entre
	#    souverainetés, selon le MODE de carte (0=Terrain : aucune ; 1=Politique &
	#    2=Régions : région+pays ; 3=Pays : pays seul). Largeur à taille ÉCRAN (÷zoom),
	#    sous les villes. Reconstruites SEULEMENT quand la souveraineté bouge. ─────────
	var mode := 0
	var par := get_parent()
	if par != null and par.get("mode") != null:
		mode = int(par.get("mode"))
	if mode >= 1:
		if _borders_dirty:
			_rebuild_borders()
		if mode <= 2 and _borders.has(1):
			var rseg: PackedVector2Array = _borders[1]
			if rseg.size() >= 2:
				draw_multiline(rseg, Color(0.078, 0.102, 0.149, 0.85), 1.6 / zoom)
		if _borders.has(2):
			var cseg: PackedVector2Array = _borders[2]
			if cseg.size() >= 2:
				draw_multiline(cseg, Color(0.039, 0.055, 0.086, 0.95), 3.0 / zoom)

	# ── VILLES : le SPRITE de settlement (atlas, tier × groupe) au centroïde.
	#    MASQUÉES sur la carte d'ensemble — elles ne paraissent qu'en ZOOM TRÈS
	#    HAUT (sinon les marqueurs encombrent). Repli sur un disque si atlas absent. ─
	if zoom >= CITY_ZOOM_MIN:
		# le bourg dépend de pop+bâtiments (évolue) → reconstruit ICI, seulement quand on
		# est assez zoomé pour le voir ET qu'un tick l'a invalidé (jamais en vue d'ensemble).
		if _struct_dirty:
			_build_structures()
			_struct_dirty = false
		# VILLES + BOURG dans une SEULE liste triée par PROFONDEUR (y du pied) → un bâti
		# au sud d'une ville la masque, et inversement (plus de ville « posée sur » son bourg).
		var props := []
		for s in _structures:
			props.append({"y": (s["pos"] as Vector2).y, "city": -1, "s": s})
		for r in range(w.region_count()):
			if w.region_tier(r) < 0:
				continue
			var ctr: Vector2 = _region_anchor.get(r, w.region_centroid(r))   # assise CALÉE SUR TERRE
			if ctr.x < 0:
				continue
			props.append({"y": ctr.y, "city": r, "ctr": ctr})
		props.sort_custom(func(a, b): return a["y"] < b["y"])   # arrière (nord) → avant (sud)
		for pr in props:
			if pr["city"] < 0:
				_draw_struct(pr["s"])
			else:
				_draw_city(w, pr["city"], pr["ctr"])

	# ── ARMÉES (dessus) : losange + ligne de marche + anneau de phase ─────────
	for c in range(w.country_count()):
		var a: Dictionary = w.army_info(c)
		if not bool(a.get("active", false)):
			continue
		var reg: int = a.get("region", -1)
		if reg < 0:
			continue
		var ctr: Vector2 = w.region_centroid(reg)
		if ctr.x < 0:
			continue
		var col := _country_color(c)
		var phase: int = a.get("phase_id", 0)

		# ligne de marche vers le but
		var dest: int = a.get("dest", -1)
		if dest >= 0 and dest != reg:
			var dctr: Vector2 = w.region_centroid(dest)
			if dctr.x >= 0:
				draw_line(ctr, dctr, Color(_phase_color(phase), 0.7), 0.5)

		# le JETON d'armée (sprite RGBA selon la composition), posé au-dessus de la ville.
		var ac := ctr + Vector2(0, -2.0)
		var token := UIKit.army_token(_army_token_name(a))
		if token != null:
			var ts := 26.0
			# socle DISCRET au pied : pastille teintée au PAYS + anneau de PHASE
			draw_circle(ac, 2.6, Color(col, 0.95))
			draw_arc(ac, 3.4, 0.0, TAU, 20, Color(_phase_color(phase), 0.95), 1.1, true)
			# le JETON, PROÉMINENT, ancré au pied (les troupes « tiennent » le sol)
			draw_texture_rect(token, Rect2(ac - Vector2(ts * 0.5, ts * 0.9), Vector2(ts, ts)), false)
		else:
			# repli losange (jeton absent)
			var s := 4.5
			draw_circle(ac, s + 1.6, Color(_phase_color(phase), 0.85))
			var diamond := PackedVector2Array([
				ac + Vector2(0, -s), ac + Vector2(s, 0),
				ac + Vector2(0, s), ac + Vector2(-s, 0)])
			var border := PackedVector2Array([
				ac + Vector2(0, -s), ac + Vector2(s, 0),
				ac + Vector2(0, s), ac + Vector2(-s, 0), ac + Vector2(0, -s)])
			draw_polyline(border, Color(0, 0, 0, 0.9), 1.4, true)
			draw_colored_polygon(diamond, col)

	# ── ÉPICENTRE du cataclysme §27 : anneaux pulsants (le foyer de la fin) ────
	var eg: Dictionary = w.endgame_info()
	var epi: int = eg.get("epicenter_reg", -1)
	var fin: int = eg.get("fin", 0)
	_cataclysm = (fin > 0 and epi >= 0)
	if epi >= 0:
		var ec: Vector2 = w.region_centroid(epi)
		if ec.x >= 0:
			var col := _fin_color(fin)
			var t := Time.get_ticks_msec() / 1000.0
			for k in range(3):
				var rad := 7.0 + k * 6.0 + fmod(t * 5.0, 6.0)
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), 1.0, true)

	# ── ÉTIQUETTES d'empire (au-dessus de tout) : nom au centroïde, en vue d'ensemble ─
	_draw_empire_labels(w, zoom)

## ÉTIQUETTES d'empire : le nom du pays au centroïde de ses régions, S'ESTOMPANT à
## mesure qu'on ZOOME (l'inverse des villes — lisible en vue d'ensemble). Texte à
## taille ÉCRAN constante (contre-échelle 1/zoom). Port de draw_empire_labels.
func _draw_empire_labels(w, zoom: float) -> void:
	var fade := (LABEL_ZOOM_MAX - zoom) / 3.5
	if fade <= 0.0:
		return
	fade = minf(fade, 1.0)
	var inv := 1.0 / zoom
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
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
		var wp := Vector2(sx / n, sy / n)
		var sp: Vector2 = vt * wp
		if sp.x < 50 or sp.y < 60 or sp.x > vp.x - 50 or sp.y > vp.y - 56:
			continue                                       # hors champ / sous les barres
		var lw := VKit.text_w(nm, VKit.FS_SMALL)
		draw_set_transform(wp, 0.0, Vector2(inv, inv))      # contre-échelle → texte à taille ÉCRAN
		draw_rect(Rect2(-lw * 0.5 - 3.0, -8.0, lw + 6.0, 15.0), Color(0.03, 0.05, 0.08, fade * 0.66))
		VKit.text(self, Vector2(-lw * 0.5, -7.0), Color(0.95, 0.91, 0.82, fade), nm, VKit.FS_SMALL)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.30, 0.55, 0.95)   # EAU : bleu
		2: return Color(0.80, 0.92, 1.00)   # FROID : blanc glacé
		3: return Color(0.35, 0.70, 0.30)   # RONCES : vert
		_: return Color(0.70, 0.35, 0.85)   # Brèche / indéterminé : violet
