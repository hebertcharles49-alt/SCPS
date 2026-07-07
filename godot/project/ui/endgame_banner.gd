extends PanelContainer
## EndgameBanner — le DESTIN du monde (§27), en haut-centre. Lit endgame_info :
## tant que l'entropie monte, l'augure textuel + une BARRE D'ENTROPIE (rail enluminé +
## curseur vortex teinté) ; quand une FIN éclôt, un bandeau rouge la nomme et la barre
## se remplit à fond, teintée par la nature de la fin. DISPLAY-ONLY (la membrane, côté
## Godot). Caché tant que l'entropie est basse et qu'aucune fin n'a éclos.

const Frame = preload("res://ui/frame.gd")
const SHOW_FROM := 25   ## % d'entropie à partir duquel on affiche la jauge
const MARGIN := 8.0

## barre d'entropie (assets Codex : rail 640×96 + vortex tintable blanc 96)
const RAIL_PATH := "res://assets/scps/ui/endgame/entropy_bar_frame_empty_640x96.png"
const CUR_PATH  := "res://assets/scps/ui/endgame/entropy_vortex_tintable_white_96.png"
const RAIL_W := 360.0
const RAIL_H := 54.0
const CUR := 46.0

## noms de fin (chrome GDScript ; i18n Phase 5). 1 EAU · 2 FROID · 3 RONCES · 4 ascension · 5 sang
const FIN_NAMES := {
	1: "Le Grand Engloutissement",
	2: "Le Grand Hiver",
	3: "L'Étreinte des Ronces",
	4: "L'Ascension",
	5: "Le Règne de Sang",
}
## teinte du curseur par fin (fallback avant fin = dégradé blanc→cuivre→rouge)
const FIN_TINT := {
	1: Color(0.32, 0.56, 0.95),   # EAU — bleu
	2: Color(0.82, 0.90, 1.00),   # FROID — blanc gelé
	3: Color(0.48, 0.60, 0.22),   # RONCES — olive
	4: Color(0.88, 0.72, 0.32),   # ASCENSION — cuivre-or
	5: Color(0.62, 0.06, 0.12),   # SANG — bordeaux
}

var _line: Label
var _bar: Control
var _rail: TextureRect
var _cursor: TextureRect
var _has_bar := false

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var vb := VBoxContainer.new()
	vb.alignment = BoxContainer.ALIGNMENT_CENTER
	vb.add_theme_constant_override("separation", 2)
	add_child(vb)
	_line = Label.new()
	_line.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_line.add_theme_font_size_override("font_size", 18)
	vb.add_child(_line)

	# la BARRE : rail enluminé (fond) + curseur vortex (glisse selon l'entropie). Bâtie
	# seulement si les deux assets existent — sinon on garde la banniere texte seule.
	var rail_t: Texture2D = load(RAIL_PATH) if ResourceLoader.exists(RAIL_PATH) else null
	var cur_t: Texture2D = load(CUR_PATH) if ResourceLoader.exists(CUR_PATH) else null
	if rail_t != null and cur_t != null:
		_has_bar = true
		_bar = Control.new()
		_bar.custom_minimum_size = Vector2(RAIL_W, RAIL_H)
		_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(_bar)
		_rail = TextureRect.new()
		_rail.texture = rail_t
		_rail.expand_mode = TextureRect.EXPAND_IGNORE_SIZE   # honore la taille manuelle
		_rail.stretch_mode = TextureRect.STRETCH_SCALE
		_rail.position = Vector2.ZERO
		_rail.size = Vector2(RAIL_W, RAIL_H)
		_rail.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_bar.add_child(_rail)
		_cursor = TextureRect.new()
		_cursor.texture = cur_t
		_cursor.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		_cursor.stretch_mode = TextureRect.STRETCH_SCALE
		_cursor.size = Vector2(CUR, CUR)
		_cursor.position = Vector2(0.0, (RAIL_H - CUR) * 0.5)
		_cursor.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_bar.add_child(_cursor)

	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(_on_gen)
	hide()

func _on_tick(_year: int) -> void:
	_refresh()

func _on_gen() -> void:
	_refresh()

func _entropy_color(pct: int) -> Color:
	# vert paisible → ambre → rouge à mesure que l'entropie monte
	return Color.from_hsv(lerpf(0.33, 0.0, clampf(pct / 100.0, 0.0, 1.0)), 0.7, 0.95)

## teinte du curseur : blanc → cuivre → rouge selon le %, ou la couleur de la fin latchée
func _cursor_tint(pct: int, fin: int) -> Color:
	if fin > 0 and FIN_TINT.has(fin):
		return FIN_TINT[fin]
	var f := clampf(pct / 100.0, 0.0, 1.0)
	if f < 0.5:
		return Color(1, 1, 1).lerp(Color(0.82, 0.55, 0.28), f / 0.5)      # blanc → cuivre
	return Color(0.82, 0.55, 0.28).lerp(Color(0.92, 0.20, 0.15), (f - 0.5) / 0.5)  # cuivre → rouge

## place le curseur à x = pct/100 sur le rail et le teinte
func _set_bar(pct: int, fin: int) -> void:
	if not _has_bar:
		return
	var f := clampf(pct / 100.0, 0.0, 1.0)
	_cursor.position.x = f * (RAIL_W - CUR)
	_cursor.modulate = _cursor_tint(pct, fin)

func _refresh() -> void:
	var w = Sim.world
	if w == null:
		hide()
		return
	var e: Dictionary = w.endgame_info()
	var fin: int = e.get("fin", 0)
	# JUSTE LA MÉTRIQUE : « Entropie X » (pas de prose). À la FIN, on adjoint le NOM de la
	# fin (le FAIT : quelle apocalypse), la barre se remplit et se teinte à sa couleur.
	if fin > 0:
		_line.text = "Entropie  100  —  %s" % FIN_NAMES.get(fin, "Fin du monde")
		_line.add_theme_color_override("font_color", Color(0.96, 0.3, 0.24))
		_set_bar(100, fin)
		show()
		_reposition.call_deferred()
		return
	var pct: int = e.get("entropie_pct", 0)
	if pct >= SHOW_FROM:
		_line.text = "Entropie  %d" % pct
		_line.add_theme_color_override("font_color", _entropy_color(pct))
		_set_bar(pct, 0)
		show()
		_reposition.call_deferred()
	else:
		hide()

## colle le bandeau en HAUT-CENTRE (recalculé : la taille suit le texte)
func _reposition() -> void:
	var sz := get_combined_minimum_size()
	var vp := get_viewport_rect().size
	reset_size()
	position = Vector2((vp.x - sz.x) * 0.5, Frame.TOPBAR_H + MARGIN)   # sous le bandeau haut
