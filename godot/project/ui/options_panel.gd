extends Control
## OptionsPanel — l'écran Options du shell : LANGUE (Français / English) et PLEIN ÉCRAN.
##
## Persisté dans user://options.cfg (motif ConfigFile de audio/sound.gd). La langue
## s'applique DEUX étages à la fois : TranslationServer.set_locale (le chrome Godot,
## clés du CSV i18n/ui.csv) + Sim.world.lang_set (la table compilée du MOTEUR — les
## readouts traversants rendent la langue au prochain appel ; la surcharge
## scps_lang.txt reste au-dessus). Zéro logique de sim : réglages d'affichage purs.
##
## `boot()` est appelé par menu_root AVANT de poser le moindre tr() — la config
## sauvée gouverne les textes dès la première construction du menu.

signal back
signal language_changed   ## les textes tr() sont posés à la construction → le menu se rebâtit

const CFG_PATH := "user://options.cfg"

# palette (charte parchemin du shell — miroir menu_root)
const C_PANEL := Color(0.07, 0.06, 0.05, 0.84)
const C_EDGE  := Color(0.79, 0.64, 0.29)
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.66, 0.62, 0.56)
const C_TITLE := Color(0.90, 0.76, 0.48)

## Les LOCALES offertes — index de l'OptionButton. Les noms sont des ENDONYMES
## (jamais traduits : « Français » se lit en français quelle que soit la langue).
const LOCALES := ["fr", "en"]
const LOCALE_NAMES := ["Français", "English"]

## Les ÉCHELLES d'interface offertes (audit 2026-07-10 : « échelle UI 100/125/150 % »).
## Appliquées via content_scale_factor (stretch canvas_items, cf. project.godot).
const SCALES := [1.0, 1.25, 1.5]
const SCALE_NAMES := ["100 %", "125 %", "150 %"]

var lang := "fr"           ## "fr" | "en"
var fullscreen := true     ## défaut projet (window/size/mode=3) ; relu au boot
var ui_scale := 1.0        ## 1.0 | 1.25 | 1.5
var _booted := false
var _box: Control = null


## Charge + applique la config sauvée. Appelé par menu_root AVANT toute
## construction de texte (l'instance n'a pas besoin d'être dans l'arbre).
func boot() -> void:
	if _booted:
		return
	_booted = true
	fullscreen = _is_fullscreen()       # défaut : l'état courant (projet = plein écran)
	_load_cfg()
	_apply()

func _ready() -> void:
	boot()                              # filet si menu_root ne l'a pas appelé
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	_build_ui()


## ── application ────────────────────────────────────────────────────────────
func _apply() -> void:
	TranslationServer.set_locale(lang)
	if Sim.world != null and Sim.world.has_method("lang_set"):
		Sim.world.lang_set(1 if lang == "en" else 0)
	if fullscreen != _is_fullscreen():
		DisplayServer.window_set_mode(
			DisplayServer.WINDOW_MODE_FULLSCREEN if fullscreen else DisplayServer.WINDOW_MODE_WINDOWED)
	# échelle UI : le facteur logique de la fenêtre (1.0 = pixel-perfect, défaut)
	var w := get_window() if is_inside_tree() else null
	if w == null and Engine.get_main_loop() is SceneTree:
		w = (Engine.get_main_loop() as SceneTree).root
	if w != null and absf(w.content_scale_factor - ui_scale) > 0.01:
		w.content_scale_factor = ui_scale

func _is_fullscreen() -> bool:
	var m := DisplayServer.window_get_mode()
	return m == DisplayServer.WINDOW_MODE_FULLSCREEN or m == DisplayServer.WINDOW_MODE_EXCLUSIVE_FULLSCREEN


## ── persistance (user://options.cfg) ───────────────────────────────────────
func _load_cfg() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(CFG_PATH) != OK:
		return
	var l := String(cfg.get_value("options", "lang", lang))
	if l in LOCALES:
		lang = l
	fullscreen = bool(cfg.get_value("options", "fullscreen", fullscreen))
	var sc := float(cfg.get_value("options", "ui_scale", ui_scale))
	if sc in SCALES:
		ui_scale = sc

func _save_cfg() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("options", "lang", lang)
	cfg.set_value("options", "fullscreen", fullscreen)
	cfg.set_value("options", "ui_scale", ui_scale)
	cfg.save(CFG_PATH)


## ── UI (rebâtie à chaque changement de langue — les tr() sont posés ici) ───
func _build_ui() -> void:
	if _box != null:
		_box.queue_free()
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	_box = center

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

	var t := Label.new(); t.text = tr("T_OPT_TITLE")
	t.add_theme_font_size_override("font_size", 24)
	t.add_theme_color_override("font_color", C_TITLE)
	col.add_child(t)

	# langue : Français / English
	var lrow := HBoxContainer.new()
	lrow.add_theme_constant_override("separation", 8)
	var llab := Label.new(); llab.text = tr("T_OPT_LANG")
	llab.custom_minimum_size = Vector2(150, 0)
	llab.add_theme_color_override("font_color", C_TEXT)
	lrow.add_child(llab)
	var opt := OptionButton.new()
	for nom in LOCALE_NAMES:
		opt.add_item(nom)
	opt.selected = LOCALES.find(lang)
	opt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	opt.item_selected.connect(_on_lang)
	lrow.add_child(opt)
	col.add_child(lrow)

	# plein écran
	var fs := CheckButton.new()
	fs.text = tr("T_OPT_FULLSCREEN")
	fs.button_pressed = fullscreen
	fs.add_theme_color_override("font_color", C_TEXT)
	fs.toggled.connect(_on_fullscreen)
	col.add_child(fs)

	# échelle de l'interface : 100 / 125 / 150 % (audit 2026-07-10)
	var srow := HBoxContainer.new()
	srow.add_theme_constant_override("separation", 8)
	var slab := Label.new(); slab.text = tr("T_OPT_SCALE")
	slab.custom_minimum_size = Vector2(150, 0)
	slab.add_theme_color_override("font_color", C_TEXT)
	srow.add_child(slab)
	var sopt := OptionButton.new()
	for nom in SCALE_NAMES:
		sopt.add_item(nom)
	sopt.selected = SCALES.find(ui_scale)
	sopt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sopt.item_selected.connect(_on_scale)
	srow.add_child(sopt)
	col.add_child(srow)

	var hint := Label.new(); hint.text = tr("T_OPT_HINT")
	hint.add_theme_color_override("font_color", C_DIM)
	hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hint.custom_minimum_size = Vector2(480, 0)
	col.add_child(hint)

	var bk := Button.new(); bk.text = tr("T_BACK")
	bk.pressed.connect(func(): back.emit())
	col.add_child(bk)


func _on_lang(i: int) -> void:
	var nl: String = LOCALES[clampi(i, 0, LOCALES.size() - 1)]
	if nl == lang:
		return
	lang = nl
	_save_cfg()
	_apply()
	_build_ui()               # ce panneau se retraduit lui-même
	language_changed.emit()   # le menu (parent) rebâtit les siens

func _on_fullscreen(on: bool) -> void:
	fullscreen = on
	_save_cfg()
	_apply()

func _on_scale(i: int) -> void:
	ui_scale = SCALES[clampi(i, 0, SCALES.size() - 1)]
	_save_cfg()
	_apply()
