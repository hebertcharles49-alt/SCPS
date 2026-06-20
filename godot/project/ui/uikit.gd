extends RefCounted
## UIKit — HABILLAGE : le pack d'assets (chrome + icônes) appliqué à l'UI Godot.
## Charge les PNG en RUNTIME (Image.load_from_file → ImageTexture : robuste en
## headless ET en dev, comme le viewer SDL charge ses BMP ; pas de dépendance au
## système d'import de l'éditeur). Cache par chemin. Display-only.
##
## Consommé via `const UIKit = preload("res://ui/uikit.gd")`.

const CHROME := "res://assets/scps/ui/chrome/"
const ICONS  := "res://assets/scps/ui/icons/"
const RESOURCES := "res://assets/scps/pack/resources/"
const MAP := "res://assets/scps/pack/map/"

const SETTLE_CELL := 96   # atlas settlements : 6 tiers (col) × 6 groupes (ligne), 96 px

## ÔTE LE MAGENTA d'un atlas BMP. Pas un simple seuil (le seuil laissait des FRANGES
## roses sur les bords anti-aliasés) : on mesure la « magenta-ité » m = min(R,B) − G
## (à quel point R et B dominent le vert). m franc ⇒ transparent ; frange ⇒ alpha
## dégressif + DESPILL (on rabat R et B vers G pour tuer le rose résiduel). Une seule
## passe, partagée par tous les atlas (settlements, dressing, …).
static func _key_magenta(img: Image) -> Texture2D:
	if img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	var data := img.get_data()
	for i in range(0, data.size(), 4):
		var r := data[i]
		var g := data[i + 1]
		var b := data[i + 2]
		var lo := mini(r, b)
		var m := lo - g                       # > 0 ⇒ le pixel penche vers le magenta
		if m > 100:
			data[i + 3] = 0                   # magenta franc → transparent
		elif m > 20:                          # frange : alpha dégressif + despill du rose
			data[i + 3] = int(data[i + 3] * float(120 - m) / 100.0)
			data[i]     = g + (r - lo)        # rebascule R/B sur G (ôte l'excès magenta,
			data[i + 2] = g + (b - lo)        # garde la teinte propre du sprite)
	var keyed := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, data)
	return ImageTexture.create_from_image(keyed)

static var _settle_tex: Texture2D = null
static var _settle_tried := false

## charge l'atlas SETTLEMENTS (magenta ôté → alpha), une fois (caché).
static func _settlements() -> Texture2D:
	if _settle_tried:
		return _settle_tex
	_settle_tried = true
	# La PLACE est gardée : on charge en PRIORITÉ un PNG à alpha (settlements.png →
	# aucun keying) ; à défaut, repli LEGACY sur un BMP magenta-keyé. Tant qu'aucun
	# des deux n'est présent → null → l'overlay retombe sur ses marqueurs (pas de bave).
	var img: Image = null
	if FileAccess.file_exists(MAP + "settlements.png"):
		img = Image.load_from_file(MAP + "settlements.png")
		if img != null:
			_settle_tex = ImageTexture.create_from_image(img)
	elif FileAccess.file_exists(MAP + "settlements.bmp"):
		img = Image.load_from_file(MAP + "settlements.bmp")
		if img != null:
			_settle_tex = _key_magenta(img)
	return _settle_tex

const DRESS_CELL := 32   # atlas dressing : 16 colonnes, 32 px, magenta-keyé
static var _dress_tex: Texture2D = null
static var _dress_tried := false

static func _dressing() -> Texture2D:
	if _dress_tried:
		return _dress_tex
	_dress_tried = true
	# Même règle : PNG à alpha d'abord (dressing.png), BMP magenta-keyé en repli legacy.
	var img: Image = null
	if FileAccess.file_exists(MAP + "dressing.png"):
		img = Image.load_from_file(MAP + "dressing.png")
		if img != null:
			_dress_tex = ImageTexture.create_from_image(img)
	elif FileAccess.file_exists(MAP + "dressing.bmp"):
		img = Image.load_from_file(MAP + "dressing.bmp")
		if img != null:
			_dress_tex = _key_magenta(img)
	return _dress_tex

## sprite de dressing par id MAPD (0-111). null si l'atlas est absent.
static func dressing_sprite(id: int) -> Texture2D:
	var t := _dressing()
	if t == null or id < 0:
		return null
	var at := AtlasTexture.new()
	at.atlas = t
	at.region = Rect2((id % 16) * DRESS_CELL, (id / 16) * DRESS_CELL, DRESS_CELL, DRESS_CELL)
	return at

## sprite de settlement : colonne = tier (0-5), ligne = groupe (0-5). null si absent.
static func settlement_sprite(tier: int, group: int) -> Texture2D:
	var t := _settlements()
	if t == null:
		return null
	tier = clampi(tier, 0, 5)
	group = clampi(group, 0, 5)
	var at := AtlasTexture.new()
	at.atlas = t
	at.region = Rect2(tier * SETTLE_CELL, group * SETTLE_CELL, SETTLE_CELL, SETTLE_CELL)
	return at

static var _cache := {}

# ── TUILES de CONSTRUCTION (unités · édifices) : des PNG auto-encadrés (fond navy
#    arrondi + liseré cuivre + art). On ôte seulement le NOIR des coins → alpha, la
#    tuile survit. Mappage indice d'enum moteur → nom de fichier (certaines clés
#    d'asset diffèrent de l'identifiant C : U_FOUDRIER←U_ARQUEBUSIER, etc.). ────────
const UNITS_DIR := "res://assets/scps/pack/units/"
const BUILDINGS_DIR := "res://assets/scps/pack/buildings/"

# indexé par UnitType (0-21), ordre de scps_army.h
const UNIT_FILE := [
	"U_PIQUIER", "U_LANCIER", "U_EPEISTE", "U_ARCHER", "U_ARBALETRIER",
	"U_CAVALERIE_LEGERE", "U_CAVALERIE_LOURDE", "U_SORCIER", "U_HALLEBARDIER", "U_FOUDRIER",
	"U_ALCHIMISTE", "U_CHAMAN", "U_ARBALETRIER_LOURD", "U_BERSERKER", "U_LANCIER_DE_CHOC",
	"U_MILICE", "U_HARCELEUR", "U_TRAQUEUR", "U_LAME_FRANCHE", "U_GARDE_ESCORTE",
	"U_CAVALERIE_CUIRASSEE", "U_CAVALERIE_DE_RAID",
]
# indexé par Edifice (0-25), ordre de scps_agency.h (noms = ceux des fichiers)
const BLD_FILE := [
	"EDI_TRIBUNAL", "EDI_CHANCELLERIE", "EDI_ACADEMIE", "EDI_GARNISON", "EDI_FORTERESSE", "EDI_CITADELLE",
	"EDI_PORT", "EDI_CARAVANSERAIL", "EDI_MARCHE", "EDI_ENTREPOT", "EDI_GRENIER", "EDI_IRRIGATION", "EDI_AQUEDUC",
	"EDI_SANCTUAIRE", "EDI_TEMPLE", "EDI_CATHEDRALE", "EDI_BIBLIOTHEQUE", "EDI_MONASTERE", "EDI_COMPTOIR", "EDI_BANQUE",
	"EDI_ARSENAL", "EDI_AMIRAUTE", "EDI_PORT_MARCHAND", "EDI_BIBLIO_MIL", "EDI_OBSERVATOIRE", "EDI_TRADE_CENTER",
]
static var _tile_cache := {}

## ôte le NOIR du fond (coins de la tuile, somme RGB < 24) → alpha ; navy/liseré/art survivent.
static func _key_black(img: Image) -> Texture2D:
	if img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	var d := img.get_data()
	for i in range(0, d.size(), 4):
		if int(d[i]) + int(d[i + 1]) + int(d[i + 2]) < 24:
			d[i + 3] = 0
	var keyed := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, d)
	return ImageTexture.create_from_image(keyed)

static func _tile(dir: String, key: String) -> Texture2D:
	var path := dir + key + ".png"
	if _tile_cache.has(path):
		return _tile_cache[path]
	var tex: Texture2D = null
	if FileAccess.file_exists(path):
		var img := Image.load_from_file(path)
		if img != null:
			tex = _key_black(img)
	_tile_cache[path] = tex
	return tex

## tuile d'UNITÉ par UnitType (0-21) ; null si l'asset manque (l'appelant retombe sur le texte).
static func unit_sprite(unit_type: int) -> Texture2D:
	if unit_type < 0 or unit_type >= UNIT_FILE.size():
		return null
	return _tile(UNITS_DIR, UNIT_FILE[unit_type])

## tuile d'ÉDIFICE par Edifice (0-25) ; null si l'asset manque.
static func building_sprite(edi_type: int) -> Texture2D:
	if edi_type < 0 or edi_type >= BLD_FILE.size():
		return null
	return _tile(BUILDINGS_DIR, BLD_FILE[edi_type])

# ── ASSETS MONDE à ALPHA (cités par bande de pop · dressing nature) — RGBA DIRECT
#    (vrai alpha, AUCUN keying) : on réutilise `_tex` (chargement simple + cache). ──
const CITIES_DIR := "res://assets/scps/pack/cities/"
const DRESSING_DIR := "res://assets/scps/pack/dressing/"

## sprite de CITÉ par bande de pop (1-8) × variante (0-7 → A-H) ; null si absente.
static func city_sprite(band: int, variant: int) -> Texture2D:
	var v := "ABCDEFGH"
	return _tex(CITIES_DIR + "CITY_POP_BAND_%02d_%s.png" % [clampi(band, 1, 8), v[clampi(variant, 0, 7)]])

## variante de ville TERRAIN par NOM (CITY_BIOME_*) ; null si absente.
static func city_biome(nm: String) -> Texture2D:
	return _tex(CITIES_DIR + nm + ".png")

## CENTRE de ville (cœur du bourg) par TERRAIN × TIER (pack centres/). `kind` ∈
## {plaine, foret, montagne, estuaire, portuaire, lacustre} · tier 1-7. RGBA direct.
const CENTRES_DIR := "res://assets/scps/pack/centres/"
static func city_centre(kind: String, tier: int) -> Texture2D:
	return _tex(CENTRES_DIR + kind + "/CITY_CENTRE_" + kind.to_upper() + "_T%d.png" % clampi(tier, 1, 7))

## sprite de DRESSING par NOM (DRESS_TREE_*, DRESS_GROVE_*…) ; null si absent.
static func dressing_named(nm: String) -> Texture2D:
	return _tex(DRESSING_DIR + nm + ".png")

## JETON d'armée de campagne par NOM (ARMY_TOKEN_*) ; null si absent. RGBA direct.
const CAMPAIGN_DIR := "res://assets/scps/pack/campaign/"
static func army_token(nm: String) -> Texture2D:
	return _tex(CAMPAIGN_DIR + nm + ".png")

## sprite de RIVIÈRE droit (RIVER_HORIZONTAL, coule en X → tourné le long du fil par
## l'angle de la façade) ; null si absent.
const RIVERS_DIR := "res://assets/scps/pack/rivers/"
## segment de rivière DROIT (tourné le long du fil) — l'eau traverse le losange W↔E, centrée.
static func river_sprite() -> Texture2D:
	return _tex(RIVERS_DIR + "RIVER_EW.png")
## ÉLARGISSEMENT (delta) posé aux EMBOUCHURES — là où le fil rejoint la mer.
static func river_mouth_sprite() -> Texture2D:
	return _tex(RIVERS_DIR + "RIVER_WIDENING.png")

# ── SOL ISO (cf. godot/ASSETS_ISO.md) : UNE tuile PROPRE par BIOME (256×128) dans
#    pack/iso_tiles/biomes/<clé>.png. Le renderer peint la tuile du biome de la cellule PAR-DESSUS
#    le blend procédural. Art remplaçable au même slot (Kenney CC0, IA par-biome, etc.).
const ISO_TILES_DIR := "res://assets/scps/pack/iso_tiles/"
const ISO_TILE_W := 256
const ISO_TILE_H := 128
## clés ALIGNÉES sur l'enum Biome de scps_types.h (index = valeur moteur ; couche SCPS_LAYER_BIOME).
const BIOME_KEYS := [
	"deep_ocean", "ocean", "shallow", "coast", "plains", "farmland", "grassland",
	"steppe", "savanna", "drylands", "desert", "coastal_desert", "forest", "woods",
	"jungle", "marsh", "highlands", "hills", "mountains", "peak", "glacier",
	"mangrove", "bog", "volcano", "thorns",
]

## tuile propre du biome (index moteur) → Texture2D 256×128 ; null si absente. Caché par chemin.
static func biome_tile(biome: int) -> Texture2D:
	if biome < 0 or biome >= BIOME_KEYS.size():
		return null
	return _tex(ISO_TILES_DIR + "biomes/" + String(BIOME_KEYS[biome]) + ".png")

## VRAI si les tuiles de biome sont présentes → sol en tuiles ; sinon repli procédural (rien ne casse).
static func has_iso_tiles() -> bool:
	for b in [4, 6, 12, 18]:   # plains, grassland, forest, mountains
		if biome_tile(b) != null:
			return true
	return false

# ── BANC DE MASQUES DE FONDU (blend.dat) : N variantes (rotations iso + flips + variance + bruit),
#    gzip-compressées. Atlas VERTICAL (256 × N·128, L8) pour le shader iso_blend. Forgé hors-ligne.
const BLEND_DAT := "res://assets/scps/pack/iso_tiles/blend.dat"
static var _blend_tex: Texture2D = null
static var _blend_n := 0
static var _blend_tried := false

static func blend_count() -> int:
	blend_atlas()
	return _blend_n

## atlas des masques de fondu (Texture2D 256 × N·128) ; null si absent → le shader ne s'active pas.
static func blend_atlas() -> Texture2D:
	if _blend_tried:
		return _blend_tex
	_blend_tried = true
	if not FileAccess.file_exists(BLEND_DAT):
		return null
	var f := FileAccess.open(BLEND_DAT, FileAccess.READ)
	if f == null:
		return null
	var magic := f.get_buffer(4).get_string_from_ascii()
	var n := f.get_8()
	var tw := f.get_16()
	var th := f.get_16()
	if magic != "SBD1" or n <= 0 or tw <= 0 or th <= 0:
		return null
	var comp := f.get_buffer(f.get_length() - 9)
	var raw := comp.decompress(n * tw * th, FileAccess.COMPRESSION_GZIP)
	if raw.size() != n * tw * th:
		return null
	var img := Image.create_from_data(tw, th * n, false, Image.FORMAT_L8, raw)
	_blend_tex = ImageTexture.create_from_image(img)
	_blend_n = n
	return _blend_tex

# ── (SECONDAIRE) palette super_biomes — grandes planches de VARIATION, pioche par cellule. ───────
const SUPER_GRID := 10                       ## super_biomes_01 = PALETTE de 100 tuiles découpées
## cellules EAU de la palette (à exclure du tirage TERRE) — repérées sur la planche (r*10+c).
const SUPER_WATER := [0, 8, 9, 10, 11, 12, 19, 20, 21, 22, 30, 40, 49, 50, 60, 70, 80, 90]
static var _iso_region_cache := {}
static var _land_pal: Array = []             ## indices de tuiles TERRE (palette de variation)

## l'atlas super_biomes (Texture2D N·256 × N·128) ; null si absent. Caché par chemin (un seul essai).
static func super_atlas() -> Texture2D:
	return _tex(ISO_TILES_DIR + "super_biomes_01.png")

## tuile (col,row) du champ → région 256×128 (AtlasTexture caché) ; null si l'atlas manque.
static func super_tile(col: int, row: int) -> Texture2D:
	var atl := super_atlas()
	if atl == null:
		return null
	var c := col % SUPER_GRID
	var r := row % SUPER_GRID
	var ck := r * SUPER_GRID + c
	if _iso_region_cache.has(ck):
		return _iso_region_cache[ck]
	var at := AtlasTexture.new()
	at.atlas = atl
	at.region = Rect2(c * ISO_TILE_W, r * ISO_TILE_H, ISO_TILE_W, ISO_TILE_H)
	_iso_region_cache[ck] = at
	return at

## palette des tuiles de TERRE (toutes sauf l'eau) — construite une fois.
static func _land_palette() -> Array:
	if _land_pal.is_empty():
		for i in range(SUPER_GRID * SUPER_GRID):
			if not SUPER_WATER.has(i):
				_land_pal.append(i)
	return _land_pal

## tuile de TERRE pour la cellule monde (tc,tr) : on PIOCHE dans la palette (variation, pas la
## disposition de la planche-exemple) ; le blend derrière fond les tuiles voisines.
static func super_land(tc: int, tr: int) -> Texture2D:
	var pal := _land_palette()
	var idx := absi((tc * 73856093) ^ (tr * 19349663)) % pal.size()
	var t: int = pal[idx]
	return super_tile(t % SUPER_GRID, t / SUPER_GRID)

# ── STRUCTURES de terrain (maisons · ateliers · champs · édifices civiques) : un
#    POOL parsemé autour des villes. On énumère le dossier au 1er appel (pas de
#    const de 96 noms) ; RGBA direct via _tex. ──────────────────────────────────
const STRUCTURES_DIR := "res://assets/scps/pack/structures/"
static var _struct_names: PackedStringArray = []
static var _struct_listed := false

static func structure_names() -> PackedStringArray:
	if _struct_listed:
		return _struct_names
	_struct_listed = true
	var d := DirAccess.open(STRUCTURES_DIR)
	if d != null:
		for f in d.get_files():
			if f.ends_with(".png"):
				_struct_names.append(f.get_basename())
	return _struct_names

static func structure_sprite(nm: String) -> Texture2D:
	return _tex(STRUCTURES_DIR + nm + ".png")


# ressources couvertes par le pack UI (repli tant que le sprite dédié n'est pas posé)
const RES_FALLBACK := {
	"grain": "grain_bundle", "ble": "grain_bundle", "betail": "grain_bundle",
	"poisson": "health_food_bowl", "nourriture": "health_food_bowl", "vivres": "health_food_bowl",
	"pierre": "materials_stone", "argile": "materials_stone",
	"or": "gold_coin", "metal_precieux": "gold_coin", "perle": "gold_coin",
	"outils": "development_tools", "metal": "development_tools",
}

## clé de fichier normalisée d'un nom de ressource : minuscules, accents ôtés,
## espaces→_ . Ex. « Fer »→"fer", « Cristal arcanique »→"cristal_arcanique".
static func resource_key(res_name: String) -> String:
	var s := res_name.to_lower()
	var acc := {"é":"e","è":"e","ê":"e","ë":"e","à":"a","â":"a","ä":"a","î":"i","ï":"i",
		"ô":"o","ö":"o","û":"u","ù":"u","ü":"u","ç":"c","'":"","’":"","-":" "}
	for k in acc:
		s = s.replace(k, acc[k])
	return s.strip_edges().replace(" ", "_")

## le SPRITE d'une ressource (assets/scps/pack/resources/). On essaie, dans l'ordre :
## par INDEX d'enum (<id>.png puis <id zero-paddé 3>.png — l'ordre du jeu de sprites
## fourni), par CLÉ de nom (<clé>.png), puis repli sur une icône du pack, sinon null
## (l'appelant retombe sur le texte). Le NOM va au survol.
const RES_ATLAS_COLS := 16   # feuille « sheet.png » : grille de 16 colonnes (ordre enum)

static func resource_sprite(res_id: int, res_name: String) -> Texture2D:
	if res_id >= 0:
		var t := _tex(RESOURCES + str(res_id) + ".png")          # fichier par indice
		if t != null: return t
		t = _tex(RESOURCES + "%03d.png" % res_id)                # variante zéro-paddée
		if t != null: return t
		var sheet := _tex(RESOURCES + "sheet.png")               # OU une feuille 16-col
		if sheet != null:
			var cell := int(sheet.get_width() / RES_ATLAS_COLS)
			var at := AtlasTexture.new()
			at.atlas = sheet
			at.region = Rect2((res_id % RES_ATLAS_COLS) * cell, (res_id / RES_ATLAS_COLS) * cell, cell, cell)
			return at
	return resource_icon(res_name)

## variante par NOM seul (income/province, où l'on n'a pas l'id).
static func resource_icon(res_name: String) -> Texture2D:
	var key := resource_key(res_name)
	var t := _tex(RESOURCES + key + ".png")
	if t != null:
		return t
	if RES_FALLBACK.has(key):
		return icon(RES_FALLBACK[key])
	return null

static func _tex(path: String) -> Texture2D:
	if _cache.has(path):
		return _cache[path]
	var tex: Texture2D = null
	if FileAccess.file_exists(path):        # garde : pas d'erreur console si l'asset manque
		var img := Image.load_from_file(path)
		if img != null:
			tex = ImageTexture.create_from_image(img)
	_cache[path] = tex            # met aussi en cache les ratés (null) → un seul essai
	return tex

static func icon(name: String) -> Texture2D:
	return _tex(ICONS + name + ".png")

static func chrome(name: String) -> Texture2D:
	return _tex(CHROME + name + ".png")

## dessine une icône (carrée) à `pos`, côté `sizepx`. No-op si absente.
static func draw_icon(ci: CanvasItem, name: String, pos: Vector2, sizepx: float, mod: Color = Color.WHITE) -> void:
	var t := icon(name)
	if t != null:
		ci.draw_texture_rect(t, Rect2(pos, Vector2(sizepx, sizepx)), false, mod)

## dessine une pièce de chrome étirée dans `rect`. No-op si absente.
static func draw_chrome(ci: CanvasItem, name: String, rect: Rect2, mod: Color = Color.WHITE) -> void:
	var t := chrome(name)
	if t != null:
		ci.draw_texture_rect(t, rect, false, mod)

## JAUGE TEXTURÉE : cadre vide + remplissage (région clippée à `value` 0-100).
## La couleur du remplissage suit le sens : vert (haut) · or (moyen) · rouge (bas).
static func bar(ci: CanvasItem, rect: Rect2, value: int) -> void:
	value = clampi(value, 0, 100)
	var empty := chrome("bar_empty_prosperity")
	if empty != null:
		ci.draw_texture_rect(empty, rect, false)
	else:
		ci.draw_rect(rect, Color(0.09, 0.08, 0.12), true)
		ci.draw_rect(rect, Color(0.78, 0.51, 0.24), false, 1.0)
	var kind := "bar_fill_green" if value >= 60 else ("bar_fill_gold" if value >= 35 else "bar_fill_red")
	var fill := chrome(kind)
	# repli en aplat si la texture manque
	var inset := Vector2(3, 3)
	var area := Rect2(rect.position + inset, rect.size - inset * 2.0)
	var fw := area.size.x * value / 100.0
	if fill != null:
		var ts := fill.get_size()
		ci.draw_texture_rect_region(fill,
			Rect2(area.position, Vector2(fw, area.size.y)),
			Rect2(0, 0, ts.x * value / 100.0, ts.y))
	elif fw > 0:
		var col := Color(0.42,0.66,0.36) if value >= 60 else (Color(0.79,0.62,0.29) if value >= 35 else Color(0.78,0.30,0.27))
		ci.draw_rect(Rect2(area.position, Vector2(fw, area.size.y)), col, true)
