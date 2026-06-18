extends RefCounted
## UIKit — HABILLAGE : le pack d'assets (chrome + icônes) appliqué à l'UI Godot.
## Charge les PNG en RUNTIME (Image.load_from_file → ImageTexture : robuste en
## headless ET en dev, comme le viewer SDL charge ses BMP ; pas de dépendance au
## système d'import de l'éditeur). Cache par chemin. Display-only.
##
## Consommé via `const UIKit = preload("res://ui/uikit.gd")`.

const CHROME := "res://assets/scps/ui/chrome/"
const ICONS  := "res://assets/scps/ui/icons/"

static var _cache := {}

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
