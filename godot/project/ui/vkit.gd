extends RefCounted
## VKit — le KIT visuel de l'UI Godot : palette, sense_color, SLICE_PAL et les
## primitives immédiates (panel_bg, box, gauge, pie, face, section, row, text).
## (consommé via `const VKit = preload("res://ui/vkit.gd")` — robuste en headless,
##  pas de dépendance au cache de class_name de l'éditeur).
## DA PARCHEMIN : cuir sombre + or vieilli + encre brune — le code couleur
## bleu nuit/cuivre de la v0 (viewer SDL) est SUPPRIMÉ. Display-only.

# ── palette (la famille du chrome parchemin — cuir / or / encre) ────────────
const COL_PANEL    := Color(0x17/255.0, 0x11/255.0, 0x09/255.0, 0xf6/255.0)   # cuir profond
const COL_PANEL2   := Color(0x2a/255.0, 0x21/255.0, 0x15/255.0, 0xf6/255.0)   # cuir clair (chips/champs)
const COL_PANEL_HI := Color(0x4a/255.0, 0x3a/255.0, 0x24/255.0, 0x4d/255.0)   # reflet chaud
const COL_GOLD     := Color(0xc9/255.0, 0xa2/255.0, 0x4b/255.0, 1.0)          # or vieilli (accent)
const COL_PARCH    := Color(0xed/255.0, 0xe3/255.0, 0xcd/255.0, 1.0)
const COL_DIM      := Color(0x96/255.0, 0x8d/255.0, 0x79/255.0, 1.0)
const COL_EDGE     := Color(0x4a/255.0, 0x3b/255.0, 0x26/255.0, 1.0)          # filet brun doré
const COL_SHADOW   := Color(0x05/255.0, 0x03/255.0, 0x01/255.0, 0x6e/255.0)

# palette de parts (camemberts, barres empilées) — viewer.c SLICE_PAL[8]
const SLICE_PAL := [
	Color(0xb8/255.0,0x73/255.0,0x33/255.0), Color(0x4e/255.0,0x8d/255.0,0x8a/255.0),
	Color(0xc9/255.0,0xa2/255.0,0x4b/255.0), Color(0x7a/255.0,0x5c/255.0,0x99/255.0),
	Color(0x9a/255.0,0x8f/255.0,0x78/255.0), Color(0x5f/255.0,0x8a/255.0,0xb0/255.0),
	Color(0xa8/255.0,0x5a/255.0,0x5a/255.0), Color(0x6f/255.0,0x9a/255.0,0x5a/255.0),
]

# tailles de police (g_font / g_font_small / g_font_big) — RELEVÉES (audit
# 2026-07-10 : « texte courant 16-17 px minimum, secondaire 14 px minimum ») ;
# les layouts en pas de 14-18 px encaissent (le texte se dessine depuis le HAUT).
const FS := 16
const FS_SMALL := 14
const FS_BIG := 20

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

## panel_bg : fond de panneau PLAT (retour joueur 2026-07-10 : « débarrasse-toi des
## assets de panneau — les joueurs pardonnent le cheap quand c'est lisible », RimWorld).
## Ombre douce + cuir plat + liseré fin — plus AUCUN 9-slice de chrome (le cadre
## parchemin étiré/riveté disparaît ; les icônes de RESSOURCE restent, elles).
static var _pb_shadow: StyleBoxFlat = null
static var _pb_body: StyleBoxFlat = null

static func panel_bg(ci: CanvasItem, r: Rect2) -> void:
	if _pb_shadow == null:
		_pb_shadow = StyleBoxFlat.new()
		_pb_shadow.bg_color = COL_SHADOW
		_pb_shadow.set_corner_radius_all(8)
		_pb_body = StyleBoxFlat.new()
		_pb_body.bg_color = COL_PANEL
		_pb_body.set_corner_radius_all(6)
		_pb_body.border_color = Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.55)
		_pb_body.set_border_width_all(1)
	ci.draw_style_box(_pb_shadow, Rect2(r.position + Vector2(3, 5), r.size))
	ci.draw_style_box(_pb_body, r)

## jauge 0-100 : piste sombre + REMPLISSAGE proportionnel teinté par le sens (l'ancien
## arc-en-ciel plein + curseur lisait comme une palette, pas comme une valeur).
static func gauge(ci: CanvasItem, x: float, y: float, w: float, h: float, value: int) -> void:
	value = clampi(value, 0, 100)
	fill(ci, Rect2(x, y, w, h), COL_PANEL2)
	box(ci, Rect2(x - 1, y - 1, w + 2, h + 2), COL_EDGE)
	var fw := (w - 2.0) * float(value) / 100.0
	if fw > 0.5:
		fill(ci, Rect2(x + 1, y + 1, fw, h - 2), sense(float(value) / 100.0))

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
	var c := sense(mood) if lit else Color(0x52/255.0, 0x4a/255.0, 0x3e/255.0)
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

## HEADER DE FENÊTRE (discipline Stellaris minée 2026-07-10 : bande de titre 36 px ·
## titre en grand corps · filet séparateur sous la bande · close en HAUT-DROITE).
## Renvoie le rect du bouton ✕ (à tester dans _gui_input du panneau). Le contenu
## démarre à HDR_H + ~8. Remplace les titres nus posés à des y variables.
const HDR_H := 36.0
static func header(ci: CanvasItem, w: float, title: String) -> Rect2:
	fill(ci, Rect2(0, 0, w, HDR_H), Color(0.055, 0.042, 0.028, 0.92))
	text(ci, Vector2(12, 7), COL_PARCH, title, FS_BIG)
	fill(ci, Rect2(0, HDR_H - 1.0, w, 1), Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.6))
	var cr := Rect2(w - 30.0, 8.0, 20.0, 20.0)
	fill(ci, cr, COL_PANEL2)
	box(ci, cr, COL_GOLD)
	text(ci, Vector2(cr.position.x + 6, cr.position.y + 2), COL_PARCH, "x")
	return cr

# ── sections & rangées (ui_section / ui_row). y est un [valeur] muté → on
#    renvoie le nouveau y (GDScript n'a pas de int*). ─────────────────────────
## HEADER DE SECTION en BANDEAU (rendu attendu Paradox 2026-07-09) : bande sombre +
## filets or haut/bas, titre or — remplace le texte nu, même consommation verticale
## (30 px) donc AUCUN layout appelant ne bouge. La largeur suit le Control porteur.
static func section(ci: CanvasItem, x: float, y: float, title: String) -> float:
	y += 6
	var bw := 220.0
	if ci is Control:
		bw = maxf(80.0, (ci as Control).size.x - 2.0 * x)
	fill(ci, Rect2(x - 4, y - 3, bw + 8, 20), Color(0.085, 0.066, 0.048, 0.88))
	fill(ci, Rect2(x - 4, y - 3, bw + 8, 1), Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.55))
	fill(ci, Rect2(x - 4, y + 16, bw + 8, 1), Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.55))
	text(ci, Vector2(x + 2, y - 1), COL_GOLD, title)
	return y + 24

static func row(ci: CanvasItem, x: float, y: float, cat: String, word: String, wc: Color) -> float:
	text(ci, Vector2(x, y), COL_DIM, cat)
	text(ci, Vector2(x + 104, y), wc, word)
	return y + 20
