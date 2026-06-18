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

static var _settle_tex: Texture2D = null
static var _settle_tried := false

## charge l'atlas SETTLEMENTS (BMP magenta-keyé → alpha), une fois (caché).
static func _settlements() -> Texture2D:
	if _settle_tried:
		return _settle_tex
	_settle_tried = true
	var img := Image.load_from_file(MAP + "settlements.bmp")
	if img != null:
		if img.get_format() != Image.FORMAT_RGBA8:
			img.convert(Image.FORMAT_RGBA8)
		var data := img.get_data()
		for i in range(0, data.size(), 4):   # magenta (≈255,0,255) → transparent
			if data[i] > 200 and data[i + 1] < 60 and data[i + 2] > 200:
				data[i + 3] = 0
		var keyed := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, data)
		_settle_tex = ImageTexture.create_from_image(keyed)
	return _settle_tex

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
	var img := Image.load_from_file(path)
	var tex: Texture2D = null
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
