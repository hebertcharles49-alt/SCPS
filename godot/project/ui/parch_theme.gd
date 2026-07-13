extends RefCounted
## ParchTheme — le THEME « parchemin » PARTAGÉ, extrait du pilote budget_panel_v2.
## Une SEULE source de style pour les panneaux à conteneurs natifs (PanelContainer/
## VBox/HBox/Grid) : styleboxes du panneau/HeaderStrip/LedTabStrip/Body, variations de
## Label (Title/Section/RowLabel/RowDim/Income/Expense/Treasury), onglets Button, HSlider.
## Aucun `_draw` : la mise en page s'auto-espace, le style vit ICI.
##
## Usage : `theme = load("res://ui/parch_theme.gd").build()`  (ou preload + .build()).
## Palette accessible aux consommateurs : `const PT = preload(...)` puis `PT.INK`, `PT.GREEN`…
## Display-only (couleurs/fonts/styleboxes ; ne touche NI moteur NI déterminisme).

const VKit = preload("res://ui/vkit.gd")

# ── PALETTE PARCHEMIN (concept 3) ─────────────────────────────────────────────
const PANEL_BG      := Color("e7d9b6")
const BORDER        := Color("b39a63")
const HEADER_BG     := Color("d8c69a")
const TABBAR_BG     := Color("dfcfa4")
const TAB_UNDERLINE := Color("7a5c22")
const DIVIDER       := Color("c3ad78")
const ROW_ALT       := Color("ded0aa")   # ligne alternée discrète
const INK           := Color("3a2f1c")   # texte primaire
const DIM_INK       := Color("8a7643")   # labels / têtes de section
const HEADER_INK    := Color("5b4a2a")   # titres
const INCOME        := Color("3f6b3a")   # vert = rentrée / positif
const EXPENSE       := Color("9c3b2e")   # rouge = sortie / négatif
# alias sémantiques (bien / mal), pour les deltas signés des consommateurs
const GREEN         := INCOME
const RED           := EXPENSE

const FS_TITLE   := 15
const FS_ROW     := 13
const FS_SECTION := 11
const FS_TAB     := 13

# ── LE THEME : tout le style ici, une seule fois ──────────────────────────────
static func build() -> Theme:
	var th := Theme.new()
	var serif: Font = VKit.font_map()   # IMFell English SC — la voix « parchemin »
	var body: Font = VKit.font()        # Alegreya Sans — lisible pour les chiffres
	if serif != null:
		th.default_font = serif
	th.default_font_size = FS_ROW

	# 1. le PANNEAU (parchemin, bord 1px, coin 3)
	th.set_stylebox("panel", "PanelContainer", sb(PANEL_BG, BORDER, 1, 3, 10, 10, 8, 8))

	# 2. bandeau HEADER (variation)
	th.set_type_variation("HeaderStrip", "PanelContainer")
	th.set_stylebox("panel", "HeaderStrip", sb(HEADER_BG, BORDER, 0, 0, 12, 12, 7, 7))

	# 3. barre d'ONGLETS (variation — nom NON built-in : « TabBar » est une classe Godot)
	th.set_type_variation("LedTabStrip", "PanelContainer")
	th.set_stylebox("panel", "LedTabStrip", sb(TABBAR_BG, BORDER, 0, 0, 6, 6, 0, 0))

	# 4. corps (variation, fond transparent — laisse voir le parchemin)
	th.set_type_variation("Body", "PanelContainer")
	th.set_stylebox("panel", "Body", sb(Color(0, 0, 0, 0), Color(0, 0, 0, 0), 0, 0, 10, 10, 8, 8))

	# — LABELS : variations de couleur/taille/police —
	_label_variation(th, "Title", serif, FS_TITLE, HEADER_INK)
	_label_variation(th, "Section", serif, FS_SECTION, DIM_INK)
	_label_variation(th, "RowLabel", body, FS_ROW, INK)
	_label_variation(th, "RowDim", body, FS_ROW, DIM_INK)
	_label_variation(th, "Income", body, FS_ROW, INCOME)
	_label_variation(th, "Expense", body, FS_ROW, EXPENSE)
	_label_variation(th, "Treasury", serif, FS_TITLE, HEADER_INK)

	# — ONGLETS (Button en mode bascule ; l'actif porte le soulignement) —
	th.set_type_variation("Tab", "Button")
	if serif != null:
		th.set_font("font", "Tab", serif)
	th.set_font_size("font_size", "Tab", FS_TAB)
	th.set_color("font_color", "Tab", DIM_INK)
	th.set_color("font_hover_color", "Tab", INK)
	th.set_color("font_pressed_color", "Tab", HEADER_INK)
	th.set_color("font_hover_pressed_color", "Tab", HEADER_INK)
	th.set_stylebox("normal", "Tab", _sb_flat_tab(false))
	th.set_stylebox("hover", "Tab", _sb_flat_tab(false))
	th.set_stylebox("pressed", "Tab", _sb_flat_tab(true))
	th.set_stylebox("hover_pressed", "Tab", _sb_flat_tab(true))
	th.set_stylebox("focus", "Tab", StyleBoxEmpty.new())

	# — HSlider : piste + grabber lisibles sur parchemin —
	var track := sb(Color("caa768"), BORDER, 1, 2, 0, 0, 0, 0)
	track.content_margin_top = 3.0
	track.content_margin_bottom = 3.0
	th.set_stylebox("slider", "HSlider", track)
	th.set_stylebox("grabber_area", "HSlider", sb(TAB_UNDERLINE, Color(0, 0, 0, 0), 0, 2, 0, 0, 0, 0))
	th.set_stylebox("grabber_area_highlight", "HSlider", sb(HEADER_INK, Color(0, 0, 0, 0), 0, 2, 0, 0, 0, 0))
	var grab := ImageTexture.create_from_image(_dot_image(14, TAB_UNDERLINE))
	th.set_icon("grabber", "HSlider", grab)
	th.set_icon("grabber_highlight", "HSlider", grab)

	# séparation par défaut des conteneurs
	th.set_constant("separation", "VBoxContainer", 3)
	th.set_constant("separation", "HBoxContainer", 8)
	return th

static func _label_variation(th: Theme, name: String, f: Font, sz: int, col: Color) -> void:
	th.set_type_variation(name, "Label")
	if f != null:
		th.set_font("font", name, f)
	th.set_font_size("font_size", name, sz)
	th.set_color("font_color", name, col)

## StyleBoxFlat helper — PUBLIC (les consommateurs bâtissent un diviseur/accent avec).
static func sb(bg: Color, border: Color, bw: int, radius: int,
		ml: int, mr: int, mt: int, mb: int) -> StyleBoxFlat:
	var s := StyleBoxFlat.new()
	s.bg_color = bg
	if bw > 0:
		s.set_border_width_all(bw)
		s.border_color = border
	s.set_corner_radius_all(radius)
	s.content_margin_left = float(ml)
	s.content_margin_right = float(mr)
	s.content_margin_top = float(mt)
	s.content_margin_bottom = float(mb)
	return s

## onglet plat : soulignement 2px #7a5c22 seulement pour l'actif (pressed)
static func _sb_flat_tab(active: bool) -> StyleBoxFlat:
	var s := StyleBoxFlat.new()
	s.bg_color = Color(0, 0, 0, 0)
	s.content_margin_left = 12.0
	s.content_margin_right = 12.0
	s.content_margin_top = 5.0
	s.content_margin_bottom = 5.0
	if active:
		s.border_width_bottom = 2
		s.border_color = TAB_UNDERLINE
	return s

static func _dot_image(d: int, col: Color) -> Image:
	var img := Image.create(d, d, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	var r := float(d) * 0.5
	for y in range(d):
		for x in range(d):
			if Vector2(x - r + 0.5, y - r + 0.5).length() <= r - 0.5:
				img.set_pixel(x, y, col)
	return img
