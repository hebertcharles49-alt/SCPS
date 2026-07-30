extends RefCounted
## GeoNames — les ENSEMBLES nommés de la carte (forêts, lacs, rivières, massifs) :
## détection display-only sur les couches façade + pool de noms PROCÉDURAUX semés par la
## graine du monde (déterministe : même monde ⇒ mêmes noms, jamais sérialisé). Le rendu
## (calligraphie fadée dans le terrain) vit dans overlay.gd — ici : les DONNÉES.
## Détection sur grille SOUS-ÉCHANTILLONNÉE (pas de 4) : 1024×512 → 256×128, flood-fill
## bon marché, largement assez précis pour poser un nom.

const STEP := 4                 # sous-échantillonnage (cellules monde par texel de détection)
const MIN_CELLS := { "foret": 26, "lac": 5, "massif": 16 }   # taille mini (en texels) d'un ensemble nommé
const CAP := { "foret": 12, "lac": 10, "riviere": 10, "massif": 8 }

## syllabes — même famille sonore que les noms de pays du monde (Dornyana, Fizzyn, Lórdor)
const ONS := ["Bel", "Cor", "Dor", "Fal", "Gar", "Hal", "Ker", "Lor", "Mal", "Nor",
	"Or", "Per", "Sar", "Tor", "Val", "Wen", "Yr", "Zan", "Bren", "Cael"]
const MID := ["a", "e", "i", "o", "u", "ae", "ia", "ó", "y"]
const FIN := ["dan", "dor", "gard", "lin", "mar", "nak", "ric", "thal", "van", "wick",
	"zyn", "nel", "rune", "vech", "moth"]

const FMT := {
	"foret":   ["Bois de %s", "Forêt de %s", "La Sylve de %s", "Le Couvert de %s"],
	"lac":     ["Lac %s", "Lac de %s", "Les Eaux de %s", "Étang de %s"],
	"riviere": ["La %s", "Le %s", "Fleuve %s", "La Vieille %s"],
	"massif":  ["Monts de %s", "Les Dents de %s", "Massif de %s", "Crêtes de %s"],
}

static func _name(rng: RandomNumberGenerator, kind: String) -> String:
	var base: String = ONS[rng.randi() % ONS.size()] + MID[rng.randi() % MID.size()] + FIN[rng.randi() % FIN.size()]
	var fmts: Array = FMT[kind]
	return String(fmts[rng.randi() % fmts.size()]) % base

## construit la liste des ensembles nommés : [{text, kind, pos(Vector2 monde), ang, span, water}]
static func build(w, seed_: int) -> Array:
	var out := []
	if w == null:
		return out
	var rng := RandomNumberGenerator.new()
	rng.seed = int(seed_) * 2654435761 + 97
	var bio: Image = w.layer_image(2)          # SCPS_LAYER_BIOME
	var sea: Image = w.layer_image(4)          # eau visible (mer + lacs)
	if bio == null or sea == null:
		return out
	var W := bio.get_width()
	var H := bio.get_height()
	var gw := W / STEP
	var gh := H / STEP
	# grilles de détection : forêt (12-14) · relief (18-19) · eau (couche 4)
	var forest := PackedByteArray(); forest.resize(gw * gh)
	var mont := PackedByteArray(); mont.resize(gw * gh)
	var water := PackedByteArray(); water.resize(gw * gh)
	for gy in range(gh):
		for gx in range(gw):
			var b := int(bio.get_pixel(gx * STEP, gy * STEP).r * 255.0 + 0.5)
			var i := gy * gw + gx
			forest[i] = 1 if (b >= 12 and b <= 14) else 0
			mont[i] = 1 if (b == 18 or b == 19) else 0
			water[i] = 1 if sea.get_pixel(gx * STEP, gy * STEP).r > 0.5 else 0
	# l'OCÉAN = l'eau atteignable depuis le bord (flood) ; le reste de l'eau = LACS
	var ocean := PackedByteArray(); ocean.resize(gw * gh)
	var q: Array[int] = []
	for gx in range(gw):
		q.append(gx); q.append((gh - 1) * gw + gx)
	for gy in range(gh):
		q.append(gy * gw); q.append(gy * gw + gw - 1)
	while not q.is_empty():
		var i: int = q.pop_back()
		if i < 0 or i >= gw * gh or ocean[i] == 1 or water[i] == 0:
			continue
		ocean[i] = 1
		q.append(i - 1); q.append(i + 1); q.append(i - gw); q.append(i + gw)
	var lake := PackedByteArray(); lake.resize(gw * gh)
	for i in range(gw * gh):
		lake[i] = 1 if (water[i] == 1 and ocean[i] == 0) else 0
	# composantes connexes → un nom par ensemble assez grand (les CAP plus grands d'abord)
	_components(out, forest, gw, gh, "foret", rng, false)
	_components(out, lake, gw, gh, "lac", rng, true)
	_components(out, mont, gw, gh, "massif", rng, false)
	# RIVIÈRES : les chemins de la façade (les plus longs d'abord), nom RÉPÉTÉ le long
	# du cours (motif carto : au zoom proche, le tronçon à l'écran porte toujours son
	# nom — une seule ancre médiane laissait les fleuves anonymes de près). L'ancre
	# médiane est marquée `mid` : au zoom lointain, seule elle s'affiche.
	if w.has_method("river_paths"):
		var rivers: Array = w.river_paths()
		rivers.sort_custom(func(a, b): return (a["points"] as PackedVector2Array).size() > (b["points"] as PackedVector2Array).size())
		var nr := 0
		for rv in rivers:
			if nr >= int(CAP["riviere"]): break
			var pts: PackedVector2Array = rv["points"]
			if pts.size() < 30: continue
			var nom := _name(rng, "riviere")
			var n := pts.size()
			# ancres DENSES (le label glisse d'ancre en ancre avec la caméra — placement
			# dynamique côté draw : UNE seule s'affiche, jamais de répétition)
			var fracs: Array = [0.3, 0.5, 0.7] if n < 80 else ([0.15, 0.3, 0.45, 0.6, 0.75, 0.9] if n < 180 else [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9])
			for f in fracs:
				var ki := clampi(int(float(n) * float(f)), 4, n - 5)
				var tang: Vector2 = (pts[mini(ki + 4, n - 1)] - pts[maxi(ki - 4, 0)]).normalized()
				var ang := atan2(tang.y, tang.x)
				if ang > PI * 0.5: ang -= PI          # jamais la tête en bas
				elif ang < -PI * 0.5: ang += PI
				out.append({"text": nom, "kind": "riviere", "pos": pts[ki],
					"ang": ang, "span": float(n) * 0.35, "water": true, "rid": nr})
			nr += 1
	return out

## composantes connexes 4-voisins d'un masque sous-échantillonné → entrées nommées
static func _components(out: Array, mask: PackedByteArray, gw: int, gh: int,
		kind: String, rng: RandomNumberGenerator, water: bool) -> void:
	var seen := PackedByteArray(); seen.resize(gw * gh)
	var comps := []
	for start in range(gw * gh):
		if mask[start] == 0 or seen[start] == 1:
			continue
		var q: Array[int] = [start]
		seen[start] = 1
		var cells: Array[int] = []
		while not q.is_empty():
			var i: int = q.pop_back()
			cells.append(i)
			for d in [i - 1, i + 1, i - gw, i + gw]:
				if d >= 0 and d < gw * gh and seen[d] == 0 and mask[d] == 1:
					# garde-bord : pas de wrap horizontal entre fins de ligne
					if absi((d % gw) - (i % gw)) <= 1:
						seen[d] = 1
						q.append(d)
		if cells.size() >= int(MIN_CELLS[kind]):
			comps.append(cells)
	comps.sort_custom(func(a, b): return (a as Array).size() > (b as Array).size())
	var n := 0
	for cells in comps:
		if n >= int(CAP[kind]): break
		var cx := 0.0; var cy := 0.0
		var minx := gw; var maxx := 0
		for i in cells:
			cx += float(i % gw); cy += float(i / gw)
			minx = mini(minx, i % gw); maxx = maxi(maxx, i % gw)
		cx /= float(cells.size()); cy /= float(cells.size())
		out.append({"text": _name(rng, kind), "kind": kind,
			"pos": Vector2(cx * STEP + STEP * 0.5, cy * STEP + STEP * 0.5), "ang": 0.0,
			"span": float(maxx - minx + 1) * float(STEP) * 0.8, "water": water})
		n += 1
