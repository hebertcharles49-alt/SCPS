extends Node2D
## Overlay — les ACTEURS sur la carte, en espace MONDE (enfant de MapView → suit
## la caméra zoom/pan automatiquement). DISPLAY-ONLY : lit la façade (region_tier,
## army_info, centroïdes), ne calcule rien. Redessine au tick (les données bougent).
##
## Villes : un disque au centroïde, dimensionné au tier (0-5), teinté au pays.
## Armées : un losange au centroïde de leur région + une ligne vers leur but
## (marche), un anneau coloré par phase (marche/siège/bataille).

const UIKit = preload("res://ui/uikit.gd")
const PHASE_MARCH := 1
const PHASE_SIEGE := 2
const PHASE_BATTLE := 3
const CITY_ZOOM_MIN := 4.5   ## les villes n'apparaissent qu'AU-DELÀ de ce zoom (sinon : carte d'ensemble encombrée)

var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre
var _decor := []          ## [{name, pos}] — arbres/forêts (dressing nature), bâti au générate
var _structures := []     ## [{name, pos}] — bâti de terrain parsemé autour des villes
var _region_variant := {} ## région colonisée → nom de variante de ville TERRAIN (petits bourgs)
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
		_build_decor()
		_build_structures()
		_build_city_skins()
	queue_redraw()

func _on_generated() -> void:
	_rivers = Sim.world.river_points()
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
	for r in range(w.region_count()):
		if w.region_tier(r) < 0:
			continue
		var ctr: Vector2 = w.region_centroid(r)
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
		if nm != "":
			_region_variant[r] = nm

## parsème du bâti de terrain (maisons/ateliers/champs) AUTOUR de chaque ville
## colonisée — une couronne ∝ tier, hors de l'eau. Un bourg vivant, pas un point.
func _build_structures() -> void:
	_structures.clear()
	var w = Sim.world
	if w == null:
		return
	var names := UIKit.structure_names()
	if names.is_empty():
		return
	var sea: Image = w.layer_image(1)   # SCPS_LAYER_SEA : 0 = terre, ≥ 1 = eau (toute classe)
	# ensemble des cellules de RIVIÈRE (on ne bâtit pas dessus non plus) ──────────
	var rset := {}
	for rp in _rivers:
		rset[Vector2i(int(rp.x), int(rp.y))] = true
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		if t < 0:
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var n := 1 + t                      # densité MODÉRÉE (anti-empilement)
		for k in range(n):
			var h := ((r * 2654435761) ^ (k * 2246822519 + 374761393)) & 0x7fffffff
			var ang := float(h % 628) * 0.01
			var rad := 6.0 + float((h >> 9) % 100) / 100.0 * (6.0 + t * 2.5)   # ÉCARTÉ de la ville
			var p := ctr + Vector2(cos(ang), sin(ang)) * rad
			var ix := int(p.x)
			var iy := int(p.y)
			# pas dans l'EAU : ni mer (toute classe ≥ 1), ni rivière (cellule + voisins)
			if sea != null and ix >= 0 and iy >= 0 and ix < sea.get_width() and iy < sea.get_height():
				if int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1:
					continue
			if rset.has(Vector2i(ix, iy)) or rset.has(Vector2i(ix + 1, iy)) or rset.has(Vector2i(ix - 1, iy)) \
					or rset.has(Vector2i(ix, iy + 1)) or rset.has(Vector2i(ix, iy - 1)):
				continue
			_structures.append({"name": names[h % names.size()], "pos": p})
			if _structures.size() >= 1400:
				return

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
			var h := ((cx * 73856093) ^ (cy * 19349663)) & 0x7fffffff
			var arr: Array = FOREST_TREES[b]
			var jx := float((h >> 3) % 3) - 1.0
			var jy := float((h >> 6) % 3) - 1.0
			_decor.append({"name": arr[h % arr.size()], "pos": Vector2(cx + jx, cy + jy)})
			if _decor.size() >= 14000:
				return

func _on_tick(_year: int) -> void:
	queue_redraw()

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

	# ── VILLES : le SPRITE de settlement (atlas, tier × groupe) au centroïde.
	#    MASQUÉES sur la carte d'ensemble — elles ne paraissent qu'en ZOOM TRÈS
	#    HAUT (sinon les marqueurs encombrent). Repli sur un disque si atlas absent. ─
	if zoom >= CITY_ZOOM_MIN:
		# le BOURG autour : bâti de terrain parsemé, SOUS les villes ──────────────
		for s in _structures:
			var sspr := UIKit.structure_sprite(s["name"])
			if sspr == null:
				break
			var sp: Vector2 = s["pos"]
			var ss := 9.0
			draw_texture_rect(sspr, Rect2(sp - Vector2(ss * 0.5, ss), Vector2(ss, ss)), false)  # ancré au pied
		for r in range(w.region_count()):
			var t: int = w.region_tier(r)
			if t < 0:
				continue
			var ctr: Vector2 = w.region_centroid(r)
			if ctr.x < 0:
				continue
			# la CITÉ : terrain DISTINCTIF → variante TERRAIN ; sinon sprite par BANDE DE POP
			var band := _city_band(w.region_pop(r))
			var spr: Texture2D = null
			if _region_variant.has(r):
				spr = UIKit.city_biome(_region_variant[r])
			if spr == null:
				spr = UIKit.city_sprite(band, (r * 2654435761) % 8)
			if spr != null:
				var sz := 16.0 + t * 6.0                       # taille monde ∝ tier
				draw_texture_rect(spr, Rect2(ctr - Vector2(sz * 0.5, sz), Vector2(sz, sz)), false)  # ancré au pied
			else:
				var col := _country_color(w.region_owner(r))
				var radius := 0.8 + t * 0.55
				draw_circle(ctr, radius, col)
				draw_arc(ctr, radius, 0.0, TAU, 16, Color(0, 0, 0, 0.6), 0.3, true)
				draw_circle(ctr, radius * 0.35, Color(1, 1, 1, 0.55))

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

func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.30, 0.55, 0.95)   # EAU : bleu
		2: return Color(0.80, 0.92, 1.00)   # FROID : blanc glacé
		3: return Color(0.35, 0.70, 0.30)   # RONCES : vert
		_: return Color(0.70, 0.35, 0.85)   # Brèche / indéterminé : violet
