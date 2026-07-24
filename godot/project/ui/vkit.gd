extends RefCounted
## VKit — kit visuel de l'UI (palette, sense(), SLICE_PAL, primitives immédiates). Palette
## alignée sur ParchTheme (ivoire/brun/or) : COL_PANEL=fond, COL_PARCH=texte, COL_DIM=
## secondaire, COL_GOLD=accent, COL_EDGE=bordure. Display-only.

# palette (parchemin ivoire / encre / or fané — alignée sur parch_theme.gd).
# HIÉRARCHIE TYPO : le rang se lit par la COULEUR/GRAISSE, jamais la taille — les layouts
# sont vérifiés serré à 1280×720, +1px de police déborde. TITRE/SECTION/VALEUR/DÉTAIL.
const COL_PANEL    := Color(0xda/255.0, 0xc4/255.0, 0x8f/255.0, 1.0)          # parchemin sépia — ParchTheme.PANEL_BG
const COL_PANEL2   := Color(0xcb/255.0, 0xb3/255.0, 0x7c/255.0, 1.0)          # bandeau/chip sépia — ParchTheme.HEADER_BG
const COL_PANEL_HI := Color(0xc9/255.0, 0xa2/255.0, 0x4b/255.0, 0.30)         # sélection ambrée
const COL_GOLD     := Color(0x7a/255.0, 0x5c/255.0, 0x22/255.0, 1.0)          # or fané — ParchTheme.TAB_UNDERLINE
const COL_VALUE    := Color(0x5b/255.0, 0x4a/255.0, 0x2a/255.0, 1.0)          # valeur lisible — ParchTheme.HEADER_INK
const COL_PARCH    := Color(0x3a/255.0, 0x2f/255.0, 0x1c/255.0, 1.0)          # encre — ParchTheme.INK
const COL_DIM      := Color(0x8a/255.0, 0x76/255.0, 0x43/255.0, 1.0)          # encre fanée — ParchTheme.DIM_INK
const COL_EDGE     := Color(0xb3/255.0, 0x9a/255.0, 0x63/255.0, 1.0)          # filet parchemin — ParchTheme.BORDER
const COL_SHADOW   := Color(0x3a/255.0, 0x2f/255.0, 0x1c/255.0, 0x35/255.0)   # ombre chaude (encre diluée)

# palette de parts (camemberts, barres empilées) — viewer.c SLICE_PAL[8], assombrie pour le fond clair
const SLICE_PAL := [
	Color(0x8f/255.0,0x52/255.0,0x22/255.0), Color(0x2f/255.0,0x66/255.0,0x63/255.0),
	Color(0x8a/255.0,0x6a/255.0,0x1f/255.0), Color(0x55/255.0,0x3d/255.0,0x74/255.0),
	Color(0x6e/255.0,0x63/255.0,0x4d/255.0), Color(0x35/255.0,0x5f/255.0,0x80/255.0),
	Color(0x7e/255.0,0x3c/255.0,0x3c/255.0), Color(0x47/255.0,0x70/255.0,0x39/255.0),
]

# tailles de police (courant 16, secondaire 14, gros 20) — ne pas baisser sous ce plancher
const FS := 16
const FS_SMALL := 14
const FS_BIG := 20

# Polices : Alegreya Sans = UI, IM Fell English SC = carte. Chargées paresseusement ;
# absentes → fallback système. L'encre de carte n'est jamais un noir pur (#2a2419 + halo).
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

## sense_color : 0 = rouge … 0.5 = ambre … 1 = vert (viewer.c ligne 1146), tons parchemin sombres.
static func sense(good: float) -> Color:
	good = clampf(good, 0.0, 1.0)
	if good >= 0.5:
		var t := (good - 0.5) * 2.0
		return Color(lerpf(0x7a, 0x3f, t)/255.0, lerpf(0x5c, 0x6b, t)/255.0, lerpf(0x22, 0x3a, t)/255.0)
	var u := good * 2.0
	return Color(lerpf(0x9c, 0x7a, u)/255.0, lerpf(0x3b, 0x5c, u)/255.0, lerpf(0x2e, 0x22, u)/255.0)

# texte : pos = coin haut-gauche ; renvoie la largeur
static func text(ci: CanvasItem, pos: Vector2, col: Color, s: String, size: int = FS) -> float:
	var f := font()
	ci.draw_string(f, Vector2(pos.x, pos.y + f.get_ascent(size)), s,
		HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
	return f.get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

static func text_w(s: String, size: int = FS) -> float:
	return font().get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x

## VALEUR (hiérarchie typo niv. 3) : le nombre-clé, ton le plus lumineux. Ne pas agrandir
## la police (les layouts sont bornés serré). Renvoie la largeur.
static func value(ci: CanvasItem, pos: Vector2, s: String, size: int = FS) -> float:
	return text(ci, pos, COL_VALUE, s, size)

## DÉTAIL (hiérarchie typo niv. 4) : flavor/annexe, contraste réduit. Renvoie la largeur.
static func detail(ci: CanvasItem, pos: Vector2, s: String, size: int = FS_SMALL) -> float:
	return text(ci, pos, COL_DIM, s, size)

## texte enveloppé aux mots, borné à largeur_max/max_lignes : ne dessine jamais hors du rect
## (casse un mot trop long lettre par lettre), ellipse si coupé. Renvoie la hauteur consommée.
static func text_wrapped(ci: CanvasItem, pos: Vector2, col: Color, texte: String,
		largeur_max: float, max_lignes: int, fs: int = FS) -> float:
	var lines := PackedStringArray()
	var cur := ""
	for word in texte.split(" ", false):
		if text_w(word, fs) > largeur_max:
			# un mot seul déborde : cassé caractère par caractère (jamais hors du rect)
			if cur != "":
				lines.append(cur)
				cur = ""
			var chunk := ""
			for ch in word:
				if chunk != "" and text_w(chunk + ch, fs) > largeur_max:
					lines.append(chunk)
					chunk = ch
				else:
					chunk += ch
			cur = chunk
			continue
		var cand := word if cur == "" else cur + " " + word
		if cur != "" and text_w(cand, fs) > largeur_max:
			lines.append(cur)
			cur = word
		else:
			cur = cand
	if cur != "":
		lines.append(cur)
	var truncated := lines.size() > max_lignes
	if truncated:
		lines = lines.slice(0, max_lignes)
	var lh := float(fs) + 4.0
	for i in range(lines.size()):
		var s := String(lines[i])
		if truncated and i == lines.size() - 1:
			while s.length() > 1 and text_w(s + "…", fs) > largeur_max:
				s = s.substr(0, s.length() - 1)
			s += "…"
		text(ci, Vector2(pos.x, pos.y + float(i) * lh), col, s, fs)
	return float(lines.size()) * lh

## texte de carte (IM Fell) : encre #2a2419 + halo brun clair (contour). Renvoie la largeur.
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

static func box(ci: CanvasItem, r: Rect2, c: Color) -> void:
	ci.draw_rect(r, c, false, 1.0)

static func fill(ci: CanvasItem, r: Rect2, c: Color) -> void:
	ci.draw_rect(r, c, true)

## grain de papier procédural : bruit fbm cuit une fois, caché. Dessiné à alpha uniforme
## (le RGB varie = le grain, l'alpha non = pas de motif qui saute). get_image() synchrone,
## pas de NoiseTexture2D (génération asynchrone).
static var _grain_tex: ImageTexture = null
static func _grain() -> ImageTexture:
	if _grain_tex == null:
		var fnl := FastNoiseLite.new()
		fnl.seed = 4242
		fnl.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
		fnl.frequency = 0.22
		fnl.fractal_type = FastNoiseLite.FRACTAL_FBM
		fnl.fractal_octaves = 3
		var img := fnl.get_image(256, 256)
		img.convert(Image.FORMAT_RGBA8)
		_grain_tex = ImageTexture.create_from_image(img)
	return _grain_tex

## fleuron : losange d'encre or, seule décoration de titre du kit (vectoriel, pas un asset).
static func _fleuron(ci: CanvasItem, center: Vector2, r: float, a: float = 0.85) -> void:
	ci.draw_colored_polygon(PackedVector2Array([
		center + Vector2(0.0, -r), center + Vector2(r, 0.0),
		center + Vector2(0.0, r), center + Vector2(-r, 0.0)
	]), Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, a))

## panel_bg : plaque + ombre courte + filet + arête or.
static var _pb_shadow: StyleBoxFlat = null
static var _pb_body: StyleBoxFlat = null

static func panel_bg(ci: CanvasItem, r: Rect2) -> void:
	if _pb_shadow == null:
		_pb_shadow = StyleBoxFlat.new()
		_pb_shadow.bg_color = COL_SHADOW
		_pb_shadow.set_corner_radius_all(3)
		_pb_body = StyleBoxFlat.new()
		_pb_body.bg_color = COL_PANEL
		_pb_body.set_corner_radius_all(2)
		_pb_body.border_color = COL_EDGE
		_pb_body.set_border_width_all(1)
	ci.draw_style_box(_pb_shadow, Rect2(r.position + Vector2(4, 4), r.size))
	ci.draw_style_box(_pb_body, r)
	fill(ci, Rect2(r.position + Vector2(1, 1), Vector2(maxf(0.0, r.size.x - 2.0), 2.0)),
		Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.72))
	fill(ci, Rect2(r.position + Vector2(1, 3), Vector2(maxf(0.0, r.size.x - 2.0), 1.0)),
		Color(1.0, 1.0, 1.0, 0.035))
	var g := _grain()
	if g != null:
		var gr := r.grow(-3.0)
		if gr.size.x > 4.0 and gr.size.y > 4.0:
			# tuilé à sa taille native → grain fin ~4 px (un 256² étiré blobait en taches ~16 px)
			ci.draw_texture_rect(g, gr, true, Color(0.78, 0.80, 0.75, 0.016))

## jauge 0-100 : piste creusée (le trou reste sombre même sur panneau clair), remplissage + graduations.
static func gauge(ci: CanvasItem, x: float, y: float, w: float, h: float, value: int) -> void:
	value = clampi(value, 0, 100)
	fill(ci, Rect2(x, y, w, h), Color(0x2b/255.0, 0x22/255.0, 0x14/255.0, 1.0))
	box(ci, Rect2(x - 1, y - 1, w + 2, h + 2), COL_EDGE)
	var fw := (w - 2.0) * float(value) / 100.0
	if fw > 0.5:
		var fc := sense(float(value) / 100.0)
		fill(ci, Rect2(x + 1, y + 1, fw, h - 2), Color(fc.r * 0.82, fc.g * 0.82, fc.b * 0.82, 1.0))
		fill(ci, Rect2(x + 1, y + 1, fw, 1.0), Color(1.0, 1.0, 1.0, 0.24))
	for mark in [0.25, 0.50, 0.75]:
		fill(ci, Rect2(x + floor(w * mark), y + 1, 1.0, maxf(1.0, h - 2.0)), Color(1.0, 1.0, 1.0, 0.18))

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

## header de fenêtre : renvoie le rect du bouton ✕ (à tester dans _gui_input) ; le contenu démarre à HDR_H + ~8.
const HDR_H := 36.0
static func header(ci: CanvasItem, w: float, title: String) -> Rect2:
	fill(ci, Rect2(0, 0, w, HDR_H), COL_PANEL2)
	fill(ci, Rect2(0, 0, 4.0, HDR_H), COL_GOLD)
	fill(ci, Rect2(4.0, 0, maxf(0.0, w - 4.0), 1.0), Color(1.0, 1.0, 1.0, 0.20))
	text(ci, Vector2(14, 7), COL_PARCH, title, FS_BIG)
	fill(ci, Rect2(4.0, HDR_H - 1.0, maxf(0.0, w - 4.0), 1), COL_EDGE)
	var cr := Rect2(w - 31.0, 7.0, 23.0, 23.0)
	fill(ci, cr, Color(COL_PANEL.r, COL_PANEL.g, COL_PANEL.b, 0.9))
	box(ci, cr, COL_EDGE)
	text(ci, Vector2(cr.position.x + 7, cr.position.y + 3), COL_PARCH, "x")
	return cr

# sections & rangées : y muté → on renvoie le nouveau y (GDScript n'a pas de int*).
## header de section : bande compacte, repère or à gauche, largeur déduite de ci.size.x.
static func section(ci: CanvasItem, x: float, y: float, title: String) -> float:
	y += 3
	var bw := 220.0
	if ci is Control:
		bw = maxf(80.0, (ci as Control).size.x - 2.0 * x)
	fill(ci, Rect2(x - 4, y - 2, bw + 8, 18), Color(COL_PANEL2.r, COL_PANEL2.g, COL_PANEL2.b, 0.94))
	fill(ci, Rect2(x - 4, y - 2, 3.0, 18), COL_GOLD)
	fill(ci, Rect2(x - 1, y + 15, bw + 5, 1), COL_EDGE)
	text(ci, Vector2(x + 4, y), COL_GOLD, title.to_upper(), FS_SMALL)
	return y + 21

static func row(ci: CanvasItem, x: float, y: float, cat: String, word: String, wc: Color) -> float:
	text(ci, Vector2(x, y), COL_DIM, cat)
	text(ci, Vector2(x + 104, y), wc, word)
	var rw := 220.0
	if ci is Control:
		rw = maxf(80.0, (ci as Control).size.x - 2.0 * x)
	fill(ci, Rect2(x, y + 16.0, rw, 1.0), Color(COL_EDGE.r, COL_EDGE.g, COL_EDGE.b, 0.28))
	return y + 18

## ligne de ledger : alternance à peine visible, séparateur bas, sélection à gauche.
static func list_row_bg(ci: CanvasItem, r: Rect2, index: int, selected: bool = false) -> void:
	if selected:
		fill(ci, r, COL_PANEL_HI)
		fill(ci, Rect2(r.position, Vector2(3.0, r.size.y)), COL_GOLD)
	elif index % 2 == 0:
		fill(ci, r, Color(COL_PANEL2.r, COL_PANEL2.g, COL_PANEL2.b, 0.34))
	fill(ci, Rect2(r.position.x, r.position.y + r.size.y - 1.0, r.size.x, 1.0),
		Color(COL_EDGE.r, COL_EDGE.g, COL_EDGE.b, 0.22))
