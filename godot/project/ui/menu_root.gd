extends Control
## MenuRoot — le SHELL de menus : écran-titre (Jouer · Charger · Options · Quitter) qui
## héberge l'écran Nouvelle partie, Options et Charger. Affiché par-dessus le jeu au
## démarrage (monde en pause). Zéro logique de sim : il NAVIGUE et délègue à la façade.

signal game_started   ## une partie vient d'être lancée → le shell se referme

const NewGame = preload("res://ui/new_game_panel.gd")
const Options = preload("res://ui/options_panel.gd")
const UIKit = preload("res://ui/uikit.gd")
const VKit = preload("res://ui/vkit.gd")

const C_BG    := Color(0.04, 0.03, 0.02, 0.98)
## panneaux SEMI-TRANSPARENTS : la table du cartographe transparaît derrière
const C_PANEL := Color(0.07, 0.06, 0.05, 0.84)
const C_EDGE  := Color(0.79, 0.64, 0.29)          # or vieilli (charte parchemin)
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.66, 0.62, 0.56)
const C_TITLE := Color(0.90, 0.76, 0.48)

var _main: Control
var _new_game: Control
var _options: Control
var _load: Control
var _load_box: VBoxContainer = null
var _load_msg: Label = null


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	# Options D'ABORD : boot() applique la config sauvée (locale + table moteur +
	# plein écran) AVANT que le moindre tr() ne pose un texte.
	_options = Options.new()
	_options.name = "OptionsPanel"
	_options.boot()
	_build_bg()
	add_child(_options); _options.hide()
	_options.back.connect(func(): _show(_main))
	_options.language_changed.connect(_on_language_changed)
	_build_main()
	_spawn_new_game()
	_load = _build_load()
	add_child(_load); _load.hide()
	_show(_main)
	Sound.play_music("main_menu")   # le thème du menu, en boucle (bus Ambiance)

func _spawn_new_game() -> void:
	_new_game = NewGame.new()
	_new_game.name = "NewGamePanel"
	add_child(_new_game)
	_new_game.hide()
	_new_game.back.connect(func(): _show(_main))
	_new_game.launched.connect(_on_launched)

## Les textes tr() sont posés à la CONSTRUCTION : au changement de langue on
## rebâtit le shell (le panneau Options se retraduit lui-même ; les panneaux en
## jeu suivent à leur prochain rafraîchissement).
func _on_language_changed() -> void:
	for p in [_main, _new_game, _load]:
		if p != null:
			p.visible = false
			p.queue_free()
	_build_main()
	_spawn_new_game()
	_load = _build_load()
	add_child(_load); _load.hide()
	_show(_options)   # on reste sur l'écran Options

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


## FOND : la table du cartographe (1920×1080) en COVER plein cadre + un voile léger
## pour la lisibilité — le _draw() sombre reste en repli si l'image manque.
func _build_bg() -> void:
	var tex: Texture2D = null
	if FileAccess.file_exists("res://assets/scps/ui/menu_main_background.png"):
		var img := Image.load_from_file("res://assets/scps/ui/menu_main_background.png")
		if img != null:
			tex = ImageTexture.create_from_image(img)
	if tex == null:
		return
	var tr := TextureRect.new()
	tr.name = "MenuBg"
	tr.texture = tex
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	tr.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(tr)
	var veil := ColorRect.new()
	veil.color = Color(0.02, 0.02, 0.04, 0.28)
	veil.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	veil.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(veil)


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
	var fmap: Font = VKit.font_map()
	if fmap != null:
		title.add_theme_font_override("font", fmap)   # IM Fell : le titre appartient à la carte
	title.add_theme_font_size_override("font_size", 64)
	title.add_theme_color_override("font_color", C_TITLE)
	title.add_theme_color_override("font_outline_color", Color(0.05, 0.04, 0.03, 0.75))
	title.add_theme_constant_override("outline_size", 6)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	col.add_child(title)

	var sub := Label.new()
	sub.text = tr("T_MENU_SUBTITLE")
	sub.add_theme_color_override("font_color", C_DIM)
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	col.add_child(sub)

	# fleuron cartographique (planche 24) sous le titre — recadré à son encre (bbox)
	var fp: Dictionary = UIKit.parch_piece("sheet24_topbar_boats_menu_15")
	if fp.has("tex"):
		var at := AtlasTexture.new()
		at.atlas = fp["tex"]
		at.region = fp["rect"]
		var flr := TextureRect.new()
		flr.texture = at
		flr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		flr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		flr.custom_minimum_size = Vector2(300, 86)
		flr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		col.add_child(flr)

	col.add_child(_spacer(20))

	col.add_child(_menu_button(tr("T_MENU_PLAY"), func(): _show(_new_game)))
	col.add_child(_menu_button(tr("T_MENU_LOAD"), func(): _show(_load)))
	col.add_child(_menu_button(tr("T_MENU_OPTIONS"), func(): _show(_options)))
	col.add_child(_menu_button(tr("T_MENU_QUIT"), func(): get_tree().quit()))


func _menu_button(txt: String, cb: Callable) -> Button:
	var b := Button.new()
	b.text = txt
	b.custom_minimum_size = Vector2(260, 44)
	b.add_theme_font_size_override("font_size", 20)
	b.pressed.connect(cb)
	return b

func _spacer(h: int) -> Control:
	var c := Control.new(); c.custom_minimum_size = Vector2(0, h); return c


## ── CHARGER / SAUVEGARDER ──────────────────────────────────────────────────
func _build_load() -> Control:
	var panel := Control.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(center)
	var box := PanelContainer.new()
	box.custom_minimum_size = Vector2(580, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(2)
	sb.set_corner_radius_all(6); sb.set_content_margin_all(20)
	box.add_theme_stylebox_override("panel", sb)
	center.add_child(box)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	box.add_child(col)
	var t := Label.new(); t.text = tr("T_LOADSAVE_TITLE")
	t.add_theme_font_size_override("font_size", 24); t.add_theme_color_override("font_color", C_TITLE)
	col.add_child(t)
	_load_box = VBoxContainer.new()
	_load_box.add_theme_constant_override("separation", 6)
	col.add_child(_load_box)
	_load_msg = Label.new(); _load_msg.add_theme_color_override("font_color", C_DIM)
	col.add_child(_load_msg)
	var back := Button.new(); back.text = tr("T_BACK")
	back.pressed.connect(func(): _show(_main))
	col.add_child(back)
	return panel

func _refresh_load() -> void:
	if _load_box == null:
		return
	for c in _load_box.get_children():
		c.queue_free()
	if Sim.world == null or not Sim.world.has_method("save_slots"):
		var l := Label.new(); l.text = tr("T_ENGINE_MISSING")
		l.add_theme_color_override("font_color", C_DIM)
		_load_box.add_child(l)
		return
	for info in Sim.save_slots():
		var slot := int(info["slot"])
		var used: bool = bool(info["used"])
		var row := HBoxContainer.new()
		row.add_theme_constant_override("separation", 8)
		var lab := Label.new()
		lab.text = (tr("T_SLOT_LINE") % [slot, String(info["line"])]) if used else (tr("T_SLOT_EMPTY") % slot)
		lab.add_theme_color_override("font_color", C_TEXT if used else C_DIM)
		lab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(lab)
		var save_btn := Button.new(); save_btn.text = tr("T_SAVE")
		var s1 := slot
		save_btn.pressed.connect(func(): _on_save(s1))
		row.add_child(save_btn)
		var load_btn := Button.new(); load_btn.text = tr("T_LOAD"); load_btn.disabled = not used
		var s2 := slot
		load_btn.pressed.connect(func(): _on_load(s2))
		row.add_child(load_btn)
		_load_box.add_child(row)

func _on_save(slot: int) -> void:
	if Sim.world == null: return
	var ok: bool = Sim.save_game(slot)
	_load_msg.text = (tr("T_SAVED_OK") % slot) if ok else tr("T_SAVE_FAIL")
	_refresh_load()

func _on_load(slot: int) -> void:
	if Sim.world == null: return
	var rc: int = Sim.load_game(slot)
	if rc == 0:
		hide()              # partie chargée : on referme le menu (monde en pause)
		Sound.stop_music()  # la musique de menu s'éteint
		Sim.set_speed(0)
		Sim.game_on = true  # la partie EST commencée : alertes & popups s'éveillent
		game_started.emit()
	else:
		_load_msg.text = tr("T_LOAD_FAIL")


func _show(which: Control) -> void:
	for p in [_main, _new_game, _options, _load]:
		if p != null:
			p.visible = (p == which)
	if which == _new_game and _new_game.has_method("queue_redraw"):
		_new_game.queue_redraw()
	if which == _load:
		_refresh_load()
	queue_redraw()

func _on_launched() -> void:
	hide()                 # le shell se referme : la carte (en pause an 0) apparaît
	Sound.stop_music()     # la musique de menu s'éteint : la partie commence
	Sim.game_on = true     # la partie EST commencée : alertes & popups s'éveillent
	game_started.emit()

## ré-ouvre le menu (touche Échap en jeu) — met le monde en pause.
func open() -> void:
	Sim.set_speed(0)
	show()
	_show(_main)
	Sound.play_music("main_menu")   # le thème du menu reprend
