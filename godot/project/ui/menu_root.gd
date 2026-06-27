extends Control
## MenuRoot — le SHELL de menus : écran-titre (Jouer · Charger · Options · Quitter) qui
## héberge l'écran Nouvelle partie, Options et Charger. Affiché par-dessus le jeu au
## démarrage (monde en pause). Zéro logique de sim : il NAVIGUE et délègue à la façade.

signal game_started   ## une partie vient d'être lancée → le shell se referme

const NewGame = preload("res://ui/new_game_panel.gd")

const C_BG    := Color(0.03, 0.03, 0.05, 0.98)
const C_PANEL := Color(0.09, 0.085, 0.12, 0.99)
const C_EDGE  := Color(0.78, 0.55, 0.30)
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.60, 0.58, 0.56)
const C_TITLE := Color(0.86, 0.70, 0.42)

var _main: Control
var _new_game: Control
var _options: Control
var _load: Control


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_main()
	_new_game = NewGame.new()
	_new_game.name = "NewGamePanel"
	add_child(_new_game)
	_new_game.hide()
	_new_game.back.connect(func(): _show(_main))
	_new_game.launched.connect(_on_launched)
	_options = _build_simple("Options", "Réglages à venir (langue, vitesse par défaut…).\nLa surcharge de langue se fait déjà via scps_lang.txt.")
	_load = _build_simple("Charger", "Le système de sauvegarde sera branché ici\n(emplacements de partie).")
	add_child(_options); _options.hide()
	add_child(_load); _load.hide()
	_show(_main)

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


func _build_main() -> void:
	_main = Control.new()
	_main.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_main.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_main)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_main.add_child(center)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 14)
	center.add_child(col)

	var title := Label.new()
	title.text = "SCPS"
	title.add_theme_font_size_override("font_size", 56)
	title.add_theme_color_override("font_color", C_TITLE)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	col.add_child(title)

	var sub := Label.new()
	sub.text = "Simulateur de civilisations"
	sub.add_theme_color_override("font_color", C_DIM)
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	col.add_child(sub)

	col.add_child(_spacer(20))

	col.add_child(_menu_button("Jouer", func(): _show(_new_game)))
	col.add_child(_menu_button("Charger", func(): _show(_load)))
	col.add_child(_menu_button("Options", func(): _show(_options)))
	col.add_child(_menu_button("Quitter", func(): get_tree().quit()))


func _menu_button(txt: String, cb: Callable) -> Button:
	var b := Button.new()
	b.text = txt
	b.custom_minimum_size = Vector2(260, 44)
	b.add_theme_font_size_override("font_size", 20)
	b.pressed.connect(cb)
	return b

func _spacer(h: int) -> Control:
	var c := Control.new(); c.custom_minimum_size = Vector2(0, h); return c


func _build_simple(title_txt: String, body: String) -> Control:
	var panel := Control.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(center)
	var box := PanelContainer.new()
	box.custom_minimum_size = Vector2(520, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(2)
	sb.set_corner_radius_all(6); sb.set_content_margin_all(20)
	box.add_theme_stylebox_override("panel", sb)
	center.add_child(box)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 12)
	box.add_child(col)
	var t := Label.new(); t.text = title_txt
	t.add_theme_font_size_override("font_size", 24); t.add_theme_color_override("font_color", C_TITLE)
	col.add_child(t)
	var l := Label.new(); l.text = body
	l.add_theme_color_override("font_color", C_TEXT); l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.custom_minimum_size = Vector2(480, 0)
	col.add_child(l)
	var back := Button.new(); back.text = "Retour"
	back.pressed.connect(func(): _show(_main))
	col.add_child(back)
	return panel


func _show(which: Control) -> void:
	for p in [_main, _new_game, _options, _load]:
		if p != null:
			p.visible = (p == which)
	if which == _new_game and _new_game.has_method("queue_redraw"):
		_new_game.queue_redraw()
	queue_redraw()

func _on_launched() -> void:
	hide()                 # le shell se referme : la carte (en pause an 0) apparaît
	game_started.emit()

## ré-ouvre le menu (touche Échap en jeu) — met le monde en pause.
func open() -> void:
	Sim.set_speed(0)
	show()
	_show(_main)
