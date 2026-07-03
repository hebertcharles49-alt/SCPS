extends RefCounted
## HERALDRY — le générateur d'ARMOIRIES dérivées (jamais stockées) + la teinture
## des PIONS de plateau (planches 29-32). Tout se recalcule des faits du monde :
## écu (forme ← hash pays) TEINTÉ à la couleur d'entité (même roue que les
## frontières/lavis : _entity_hue) · partition (2e teinte, hash) · MEUBLE
## (famille ← ÉTHOS, accent ← HÉRITAGE, pièce ← hash) · ornement de rang
## (couronne murale = cité-état · totem = hameau libre). Display-only, zéro
## sérialisation — le SAVE ne bouge pas. Cache par pays, vidé via reset().

const PARCH := "res://assets/scps/ui/parch/"
const S29 := "sheet29_heraldry_shields_structure_"
const S30 := "sheet30_heraldry_charges_martial_order_"
const S31 := "sheet31_heraldry_charges_faith_nature_arcane_"
const S32 := "sheet32_army_tokens_iso_pewter_"

const SHIELDS := ["01_ecu_ancien_heater", "02_ecu_amande_kite", "03_targe_ronde", "05_ecu_ancien_use"]
const PARTITIONS := ["", "13_partition_chef", "14_partition_pal", "15_partition_bande", "16_partition_bordure"]

## MEUBLES par ÉTHOS (l'enum moteur : 0 Dominateur · 1 Honneur · 2 Ordre ·
## 3 Bureaucrate · 4 Mercantile · 5 Pacifiste) — le partage entre familles est
## héraldiquement naturel.
const CHARGES := [
	[S30 + "01_lion_rampant", S30 + "02_masse_armes_pal", S30 + "03_serre_aigle", S30 + "04_tour_crenelee", S31 + "16_dragon_love"],
	[S30 + "05_aigle_deployee", S30 + "06_epee_haute_pal", S30 + "07_heaume_face", S30 + "08_etoile_huit_rais"],
	[S30 + "09_clef_pal", S30 + "10_soleil_rayonnant", S30 + "07_heaume_face", S30 + "04_tour_crenelee"],
	[S30 + "11_balance", S30 + "12_pont_trois_arches", S31 + "04_ruche", S31 + "03_gerbe_ble_liee"],
	[S30 + "13_nef_voile_carree", S30 + "14_besant", S30 + "15_corne_abondance", S30 + "16_ancre"],
	[S31 + "01_colombe_essorante", S31 + "02_rameau_olivier", S31 + "14_cerf_passant", S31 + "03_gerbe_ble_liee"],
]
## accent d'HÉRITAGE (0 Éso · 1 Métal · 2 Méca · 3 Adaptatif · 4 Agraire · 5 Clanique)
const HER_CHARGE := [
	[S31 + "05_oeil_ouvert", S31 + "06_croissant_lune", S31 + "08_serpent_noue", S31 + "07_flamme_trois_langues"],
	[S31 + "11_marteau_forge"],
	[S31 + "13_rouage_dente"],
	[S31 + "15_corbeau"],
	[S31 + "12_arbre_deracine"],
	[S31 + "09_corne_guerre", S31 + "10_tete_sanglier"],
]
## pion d'armée par PHASE moteur (FA_* : 0 idle · 1 march · 2 siege · 3 battle ·
## 4 embark · 5 sail · 6 land)
const PIONS := ["01_army_idle", "02_army_march", "05_army_siege", "03_army_combat",
	"02_army_march", "02_army_march", "02_army_march"]

## Les pièces travaillent à 128² (l'affichage max ≈ 58 px) et la teinture passe
## par les OCTETS bruts (get_data → une PackedByteArray, un seul set à la fin) —
## le per-pixel get/set_pixel à 256² bloquait la frame (fenêtre qui clignote).
const WORK := 128

static var _img_cache := {}    # chemin → Image 128² RGBA8 (source, jamais mutée)
static var _arms_cache := {}   # cid → ImageTexture
static var _pion_cache := {}   # "phase:cid" → ImageTexture

static func reset() -> void:
	_arms_cache.clear()
	_pion_cache.clear()

static func _img(piece: String) -> Image:
	if _img_cache.has(piece):
		return _img_cache[piece]
	var img: Image = null
	var path := PARCH + piece + ".png"
	if FileAccess.file_exists(path):
		img = Image.load_from_file(path)
		if img != null:
			if img.get_format() != Image.FORMAT_RGBA8:
				img.convert(Image.FORMAT_RGBA8)
			img.resize(WORK, WORK, Image.INTERPOLATE_LANCZOS)
	_img_cache[piece] = img
	return img

## la MÊME roue de teinte que l'overlay (frontières/lavis) — une famille par entité.
static func entity_hue(e: int) -> float:
	return fmod(float(e) * 0.1607 + 0.04, 1.0)

## teinte les pixels GRIS NEUTRES et CLAIRS (les zones teintables des planches)
## par MULTIPLICATION — l'encre, l'or et l'étain (sombres ou saturés) survivent.
## Renvoie le MASQUE des pixels teintés (1 octet/pixel) — le CHAMP de l'écu.
static func _tint_gray(img: Image, col: Color, vmin: int) -> PackedByteArray:
	var d := img.get_data()
	var mask := PackedByteArray()
	mask.resize(d.size() / 4)
	var cr := int(col.r * 255.0)
	var cg := int(col.g * 255.0)
	var cb := int(col.b * 255.0)
	for i in range(0, d.size(), 4):
		if d[i + 3] < 200:
			continue          # les franges semi-transparentes ne prennent JAMAIS la teinte
		var r := d[i]
		var g := d[i + 1]
		var b := d[i + 2]
		var mx := maxi(r, maxi(g, b))
		if mx - mini(r, mini(g, b)) < 26 and mx >= vmin:
			d[i] = r * cr / 255
			d[i + 1] = g * cg / 255
			d[i + 2] = b * cb / 255
			mask[i / 4] = 1
	img.set_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, d)
	return mask

## extrait la BANDE de partition (le gris moyen #9a9a9a de la pièce) teintée à la
## 2e couleur — le champ clair, le cerne d'or et l'encre sont écartés.
static func _partition_band(piece: String, col: Color) -> Image:
	var src := _img(piece)
	if src == null:
		return null
	var d := src.get_data()
	var out := PackedByteArray()
	out.resize(d.size())
	var cr := int(col.r * 255.0)
	var cg := int(col.g * 255.0)
	var cb := int(col.b * 255.0)
	for i in range(0, d.size(), 4):
		if d[i + 3] < 128:
			continue
		var r := d[i]
		var g := d[i + 1]
		var b := d[i + 2]
		var mx := maxi(r, maxi(g, b))
		# plage RESSERRÉE : le gris de bande (~154) seul — les pixels de FRONTIÈRE
		# champ↔bande (blend Lanczos ~186-200) restaient et faisaient un HALO teinté
		if mx - mini(r, mini(g, b)) < 26 and mx >= 110 and mx <= 185:
			out[i] = r * cr / 255
			out[i + 1] = g * cg / 255
			out[i + 2] = b * cb / 255
			out[i + 3] = d[i + 3]
	var img := Image.create_from_data(src.get_width(), src.get_height(), false, Image.FORMAT_RGBA8, out)
	return img

## écrit `band` sur `base` UNIQUEMENT là où le masque (champ teinté) vaut 1.
static func _blend_masked(base: Image, band: Image, mask: PackedByteArray) -> void:
	var db := base.get_data()
	var dd := band.get_data()
	for i in range(0, mini(db.size(), dd.size()), 4):
		if mask[i / 4] == 1 and dd[i + 3] > 60:
			db[i] = dd[i]
			db[i + 1] = dd[i + 1]
			db[i + 2] = dd[i + 2]
	base.set_data(base.get_width(), base.get_height(), false, Image.FORMAT_RGBA8, db)

## LES ARMES d'un pays — composées une fois, cachées. À dessiner à l'échelle.
static func arms(cid: int) -> Texture2D:
	if cid < 0 or Sim.world == null:
		return null
	if _arms_cache.has(cid):
		return _arms_cache[cid]
	var img := compose_arms(Sim.world, cid)
	var tex: ImageTexture = ImageTexture.create_from_image(img) if img != null else null
	_arms_cache[cid] = tex
	return tex

## la COMPOSITION pure (monde explicite — testable headless, sans autoload).
static func compose_arms(w, cid: int) -> Image:
	var role := int(w.country_role(cid))
	if role == 4:
		# HAMEAU LIBRE : le totem fruste, sans écu ni teinte
		var t := _img(S29 + "08_totem_wild")
		return (t.duplicate() as Image) if t != null else null
	var h := int(fmod(float(cid) * 2654435.7, 65536.0))
	var ethos := clampi(int(w.country_ethos(cid)), 0, 5)
	var herit := clampi(int(w.country_heritage(cid)), 0, 5)
	# teintures : PIGMENTS terreux (anti-néon — même discipline que les frontières) :
	# champ = teinte d'entité désaturée, 2e = la même en sombre
	var field := Color.from_hsv(entity_hue(cid), 0.38, 0.60)
	var second := Color.from_hsv(entity_hue(cid), 0.46, 0.33)
	if role == 2:
		field = Color(0.80, 0.67, 0.36)       # cité-état : champ d'or fané
		second = Color(0.50, 0.40, 0.20)
	var src := _img(S29 + SHIELDS[h % SHIELDS.size()])
	if src == null:
		return null
	var base: Image = src.duplicate() as Image
	var mask := _tint_gray(base, field, 140)
	# partition (hash : ~40 % sans) — la bande ne s'écrit QUE sur les pixels du
	# CHAMP réellement teintés (masque) : l'ombre portée autour de la pièce de
	# partition et l'écart de silhouette entre formes d'écu ne débordent jamais
	var pick := [0, 0, 0, 0, 1, 1, 2, 3, 4, 4]
	var pidx: int = pick[(h / 7) % 10]
	if pidx > 0:
		var band := _partition_band(S29 + PARTITIONS[pidx], second)
		if band != null:
			_blend_masked(base, band, mask)
	# meuble : famille d'éthos, accent d'héritage ~40 %
	var pool: Array = CHARGES[ethos]
	if (h / 3) % 5 < 2 and HER_CHARGE[herit].size() > 0:
		pool = HER_CHARGE[herit]
	var charge := _img(String(pool[(h / 11) % pool.size()]))
	if charge != null:
		var c: Image = charge.duplicate() as Image
		var cs := WORK * 116 / 256
		c.resize(cs, cs, Image.INTERPOLATE_LANCZOS)
		base.blend_rect(c, Rect2i(0, 0, cs, cs), Vector2i(WORK * 70 / 256, WORK * 62 / 256))
	# (le rang cité-état se lit à son CHAMP D'OR — la couronne murale demanderait
	# un alignement d'art au pixel ; pièce en réserve, cf. rapport)
	return base

## LE PION d'armée : la pièce d'étain de la PHASE, drapeau teinté au pays.
static func pion(phase_id: int, cid: int) -> Texture2D:
	var key := "%d:%d" % [clampi(phase_id, 0, PIONS.size() - 1), cid]
	if _pion_cache.has(key):
		return _pion_cache[key]
	var src := _img(S32 + PIONS[clampi(phase_id, 0, PIONS.size() - 1)])
	var tex: ImageTexture = null
	if src != null:
		var img: Image = src.duplicate() as Image
		# le drapeau prend la couleur du pays (FRANCHE — il doit lire à 35 px) ;
		# l'étain est RELEVÉ ×1.4 (leçon villes : un sprite sombre ne s'éclaircit
		# pas au modulate — on lift à la charge).
		var col := Color.from_hsv(entity_hue(cid), 0.55, 0.82)
		if Sim.world != null and int(Sim.world.country_role(cid)) == 2:
			col = Color(0.86, 0.72, 0.38)
		var d := img.get_data()
		var cr := int(col.r * 255.0)
		var cg := int(col.g * 255.0)
		var cb := int(col.b * 255.0)
		for i in range(0, d.size(), 4):
			if d[i + 3] < 200:
				continue
			var r := d[i]
			var g := d[i + 1]
			var b := d[i + 2]
			var mx := maxi(r, maxi(g, b))
			if mx - mini(r, mini(g, b)) < 26 and mx >= 148:
				d[i] = mini(255, r * cr / 230)      # drapeau : teinte pays, un peu relevée
				d[i + 1] = mini(255, g * cg / 230)
				d[i + 2] = mini(255, b * cb / 230)
			else:
				d[i] = mini(255, r * 7 / 5)         # étain : lift ×1.4
				d[i + 1] = mini(255, g * 7 / 5)
				d[i + 2] = mini(255, b * 7 / 5)
		img.set_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, d)
		tex = ImageTexture.create_from_image(img)
	_pion_cache[key] = tex
	return tex

## jetons de table (bataille / déroute / siège) — étain neutre, pas de teinte.
static func marker(kind: String) -> Texture2D:
	var piece: String = {"battle": "13_battle_marker_metal", "rout": "14_rout_marker_metal",
		"siege": "15_siege_marker_metal"}.get(kind, "")
	if piece == "":
		return null
	var img := _img(S32 + piece)
	return ImageTexture.create_from_image(img) if img != null else null
