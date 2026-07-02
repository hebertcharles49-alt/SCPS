extends RefCounted
## VKit — le KIT visuel de viewer.c, porté en Godot À L'IDENTIQUE : la palette
## (consommé via `const VKit = preload("res://ui/vkit.gd")` — robuste en headless,
##  pas de dépendance au cache de class_name de l'éditeur).
## EXACTE (COL_*), sense_color, SLICE_PAL et les primitives immédiates (panel_bg,
## box, gauge, pie, face, section, row, text). Les panneaux dessinent AVEC ce kit
## dans leur _draw → même look que le viewer SDL (parchemin/cuivre/navy, jauges
## rouge→vert, camemberts, visages d'humeur). Aucune logique de sim : display-only.

# ── palette (hex exacts de viewer.c, lignes 1136-1143) ─────────────────────
const COL_PANEL    := Color(0x0d/255.0, 0x14/255.0, 0x20/255.0, 0xf6/255.0)
const COL_PANEL2   := Color(0x17/255.0, 0x23/255.0, 0x35/255.0, 0xf6/255.0)
const COL_PANEL_HI := Color(0x26/255.0, 0x36/255.0, 0x4c/255.0, 0x4d/255.0)
const COL_COPPER   := Color(0xc8/255.0, 0x82/255.0, 0x3e/255.0, 1.0)
const COL_PARCH    := Color(0xed/255.0, 0xe3/255.0, 0xcd/255.0, 1.0)
const COL_DIM      := Color(0x96/255.0, 0x8d/255.0, 0x79/255.0, 1.0)
const COL_EDGE     := Color(0x34/255.0, 0x42/255.0, 0x57/255.0, 1.0)
const COL_SHADOW   := Color(0x00/255.0, 0x02/255.0, 0x05/255.0, 0x6e/255.0)

# palette de parts (camemberts, barres empilées) — viewer.c SLICE_PAL[8]
const SLICE_PAL := [
	Color(0xb8/255.0,0x73/255.0,0x33/255.0), Color(0x4e/255.0,0x8d/255.0,0x8a/255.0),
	Color(0xc9/255.0,0xa2/255.0,0x4b/255.0), Color(0x7a/255.0,0x5c/255.0,0x99/255.0),
	Color(0x9a/255.0,0x8f/255.0,0x78/255.0), Color(0x5f/255.0,0x8a/255.0,0xb0/255.0),
	Color(0xa8/255.0,0x5a/255.0,0x5a/255.0), Color(0x6f/255.0,0x9a/255.0,0x5a/255.0),
]

# tailles de police (g_font / g_font_small / g_font_big)
const FS := 14
const FS_SMALL := 11
const FS_BIG := 18

# ── POLICES (DA parchemin) : Alegreya Sans = l'UI (humaniste, lisible en bouton/panneau/
#    tooltip) ; IM Fell English SC = la CARTE (cartouches, noms de lieux — le vieux livre
#    imprimé). Chargées paresseusement depuis assets/fonts ; ABSENTES → fallback système
#    (le projet tourne sans). L'encre de carte n'est JAMAIS un noir pur : #2a2419, posée
#    sur un HALO brun clair (le noir plat « autocollant » disparaît).
const COL_INK_MAP  := Color(0x2a / 255.0, 0x24 / 255.0, 0x19 / 255.0)
const COL_INK_HALO := Color(0.87, 0.80, 0.65, 0.55)
static var _font_ui: Font = null
static var _font_map: Font = null
static var _fonts_tried := false

static func _ttf(path: String) -> Font:
	if ResourceLoader.exists(path):
		return load(path)
	if FileAccess.file_exists(path):          # pas d'import éditeur → chargement dynamique
		var ff := FontFile.new()
		if ff.load_dynamic_font(path) == OK:
			return ff
	return null

static func _load_fonts() -> void:
	_fonts_tried = true
	_font_ui = _ttf("res://assets/fonts/AlegreyaSans-Regular.ttf")
	_font_map = _ttf("res://assets/fonts/IMFellEnglishSC-Regular.ttf")

static func font() -> Font:
	if not _fonts_tried:
		_load_fonts()
	return _font_ui if _font_ui != null else ThemeDB.fallback_font

static func font_map() -> Font:
	if not _fonts_tried:
		_load_fonts()
	return _font_map if _font_map != null else font()

## sense_color : 0 = rouge … 0.5 = ambre … 1 = vert (viewer.c, ligne 1146)
static func sense(good: float) -> Color:
	good = clampf(good, 0.0, 1.0)
	if good >= 0.5:
		var t := (good - 0.5) * 2.0
		return Color(lerpf(0xc8, 0x6a, t)/255.0, lerpf(0xa0, 0x9a, t)/255.0, lerpf(0x4a, 0x5b, t)/255.0)
	var u := good * 2.0
	return Color(lerpf(0xb1, 0xc8, u)/255.0, lerpf(0x50, 0xa0, u)/255.0, lerpf(0x3c, 0x4a, u)/255.0)

# ── texte : pos = COIN HAUT-GAUCHE (comme viewer) ; renvoie la largeur ──────
static func text(ci: CanvasItem, pos: Vector2, col: Color, s: String, size: int = FS) -> float:
	var f := font()
	ci.draw_string(f, Vector2(pos.x, pos.y + f.get_ascent(size)), s,
		HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
	return f.get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

static func text_w(s: String, size: int = FS) -> float:
	return font().get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

## texte de CARTE (IM Fell) : encre #2a2419 + HALO brun clair doux (contour) — pour les
## cartouches, noms de lieux et noms d'empire. Renvoie la largeur.
static func text_map(ci: CanvasItem, pos: Vector2, s: String, size: int = FS,
		col: Color = COL_INK_MAP, outline: int = 2, halo: Color = COL_INK_HALO) -> float:
	var f := font_map()
	var p := Vector2(pos.x, pos.y + f.get_ascent(size))
	if outline > 0:
		ci.draw_string_outline(f, p, s, HORIZONTAL_ALIGNMENT_LEFT, -1, size, outline, halo)
	ci.draw_string(f, p, s, HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
	return f.get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

static func text_map_w(s: String, size: int = FS) -> float:
	return font_map().get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

# ── primitives rectangulaires ──────────────────────────────────────────────
static func box(ci: CanvasItem, r: Rect2, c: Color) -> void:
	ci.draw_rect(r, c, false, 1.0)

static func fill(ci: CanvasItem, r: Rect2, c: Color) -> void:
	ci.draw_rect(r, c, true)

## panel_bg : ombre portée + corps navy arrondi + voile clair + double liseré cuivre
static func panel_bg(ci: CanvasItem, r: Rect2) -> void:
	# PARCHEMIN d'abord : le cadre CUIR riveté (planche 1, pièce 01) en 9-slice —
	# fond sombre, le texte clair existant reste lisible. Repli = l'aplat navy.
	var UIKit := load("res://ui/uikit.gd")
	var psb: StyleBox = UIKit.parch_box("sheet01_panel_chrome_01", 26)
	if psb != null:
		var sh := StyleBoxFlat.new()
		sh.bg_color = COL_SHADOW; sh.set_corner_radius_all(10)
		ci.draw_style_box(sh, Rect2(r.position + Vector2(4, 6), r.size))
		ci.draw_style_box(psb, r)
		return
	var sb_shadow := StyleBoxFlat.new()
	sb_shadow.bg_color = COL_SHADOW; sb_shadow.set_corner_radius_all(10)
	ci.draw_style_box(sb_shadow, Rect2(r.position + Vector2(4, 6), r.size))
	var sb := StyleBoxFlat.new()
	sb.bg_color = COL_PANEL; sb.set_corner_radius_all(8)
	sb.border_color = COL_COPPER; sb.set_border_width_all(2)
	ci.draw_style_box(sb, r)
	var sheen := StyleBoxFlat.new()
	sheen.bg_color = COL_PANEL_HI; sheen.set_corner_radius_all(7)
	ci.draw_style_box(sheen, Rect2(r.position + Vector2(2, 2), Vector2(r.size.x - 4, r.size.y / 5.0)))

## jauge 0-100 : dégradé rouge→vert (strips) + bord + curseur clair à la valeur
static func gauge(ci: CanvasItem, x: float, y: float, w: float, h: float, value: int) -> void:
	value = clampi(value, 0, 100)
	var strips := 40
	for i in range(strips):
		var t := float(i) / (strips - 1)
		fill(ci, Rect2(x + t * (w - 1), y, w / float(strips) + 1.0, h), sense(t))
	box(ci, Rect2(x - 1, y - 1, w + 2, h + 2), COL_EDGE)
	var mx := x + value / 100.0 * (w - 1)
	fill(ci, Rect2(mx - 1, y - 2, 3, h + 4), COL_PARCH)

## camembert : parts (percent[]) en couleurs (cols[]) — 0 en haut, sens horaire
static func pie(ci: CanvasItem, center: Vector2, radius: float, percents: Array, cols: Array) -> void:
	var acc := 0.0
	for i in range(percents.size()):
		var f0 := acc / 100.0
		acc += percents[i]
		var f1 := acc / 100.0
		var segs := maxi(2, int((f1 - f0) * 48))
		var pts := PackedVector2Array()
		pts.append(center)
		for k in range(segs + 1):
			var f := lerpf(f0, f1, float(k) / segs)
			var th := f * TAU
			pts.append(center + Vector2(sin(th), -cos(th)) * radius)
		var col: Color = cols[i] if i < cols.size() else COL_PANEL2
		ci.draw_colored_polygon(pts, col)
	ci.draw_arc(center, radius, 0, TAU, 48, COL_DIM, 1.0, true)

## un VISAGE : cercle + yeux + bouche parabolique (courbure = humeur 0..1)
static func face(ci: CanvasItem, center: Vector2, r: float, mood: float, lit: bool) -> void:
	var c := sense(mood) if lit else Color(0x4a/255.0, 0x52/255.0, 0x5e/255.0)
	ci.draw_arc(center, r, 0, TAU, 24, c, 1.0, true)
	fill(ci, Rect2(center.x - r/2.0, center.y - r/4.0, 2, 2), c)
	fill(ci, Rect2(center.x + r/2.0 - 1, center.y - r/4.0, 2, 2), c)
	var curve := (mood - 0.5) * 2.0
	var span := r / 2.0
	var my := center.y + r/4.0
	var prev := Vector2.ZERO
	for k in range(9):
		var t := float(k) / 8.0 * 2.0 - 1.0
		var p := Vector2(center.x + t * span, my + curve * (r/3.0) * (1.0 - t*t))
		if k > 0:
			ci.draw_line(prev, p, c, 1.0)
		prev = p

# ── sections & rangées (ui_section / ui_row). y est un [valeur] muté → on
#    renvoie le nouveau y (GDScript n'a pas de int*). ─────────────────────────
static func section(ci: CanvasItem, x: float, y: float, title: String) -> float:
	y += 9
	text(ci, Vector2(x, y), COL_COPPER, title)
	return y + 21

static func row(ci: CanvasItem, x: float, y: float, cat: String, word: String, wc: Color) -> float:
	text(ci, Vector2(x, y), COL_DIM, cat)
	text(ci, Vector2(x + 104, y), wc, word)
	return y + 20
