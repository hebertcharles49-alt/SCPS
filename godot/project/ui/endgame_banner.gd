extends PanelContainer
## EndgameBanner — le DESTIN du monde (§27), en haut-centre. Lit endgame_info :
## tant que l'entropie monte, une jauge + l'augure ; quand une FIN éclôt, un
## bandeau rouge la nomme. DISPLAY-ONLY (la membrane, côté Godot). Caché tant que
## l'entropie est basse et qu'aucune fin n'a éclos.

const SHOW_FROM := 25   ## % d'entropie à partir duquel on affiche la jauge
const MARGIN := 8.0

## noms de fin (chrome GDScript ; i18n Phase 5). 1 EAU · 2 FROID · 3 RONCES · 4 ascension
const FIN_NAMES := {
	1: "Le Grand Engloutissement",
	2: "Le Grand Hiver",
	3: "L'Étreinte des Ronces",
	4: "L'Ascension",
}

var _line: Label
var _augure: Label

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var vb := VBoxContainer.new()
	vb.alignment = BoxContainer.ALIGNMENT_CENTER
	vb.add_theme_constant_override("separation", 1)
	add_child(vb)
	_line = Label.new()
	_line.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_line.add_theme_font_size_override("font_size", 18)
	vb.add_child(_line)
	_augure = Label.new()
	_augure.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_augure.add_theme_color_override("font_color", Color(0.78, 0.74, 0.62))
	vb.add_child(_augure)
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

func _refresh() -> void:
	var w = Sim.world
	if w == null:
		hide()
		return
	var e: Dictionary = w.endgame_info()
	var fin: int = e.get("fin", 0)
	if fin > 0:
		_line.text = "⚠  %s  ⚠" % FIN_NAMES.get(fin, "La Fin du Monde")
		_line.add_theme_color_override("font_color", Color(0.96, 0.3, 0.24))
		var aug := String(e.get("augure", ""))
		_augure.text = aug
		_augure.visible = aug != ""
		show()
		_reposition.call_deferred()
		return
	var pct: int = e.get("entropie_pct", 0)
	if pct >= SHOW_FROM:
		_line.text = "Entropie  %d  —  %s" % [pct, e.get("entropie", "")]
		_line.add_theme_color_override("font_color", _entropy_color(pct))
		var aug := String(e.get("augure", ""))
		_augure.text = aug
		_augure.visible = aug != ""
		show()
		_reposition.call_deferred()
	else:
		hide()

## colle le bandeau en HAUT-CENTRE (recalculé : la taille suit le texte)
func _reposition() -> void:
	var sz := get_combined_minimum_size()
	var vp := get_viewport_rect().size
	reset_size()
	position = Vector2((vp.x - sz.x) * 0.5, MARGIN)
