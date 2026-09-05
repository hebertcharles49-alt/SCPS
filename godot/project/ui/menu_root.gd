extends Control
## MenuRoot — le SHELL de menus : écran-titre (Jouer · Charger · Options · Quitter) qui
## héberge l'écran Nouvelle partie, Options et Charger. Affiché par-dessus le jeu au
## démarrage (monde en pause). Zéro logique de sim : il NAVIGUE et délègue à la façade.

signal game_started   ## une partie vient d'être lancée → le shell se referme
signal codex_requested ## bouton Codex (F1 est parti aux onglets du rail, 2026-07-10)

const NewGame = preload("res://ui/new_game_panel.gd")
const Options = preload("res://ui/options_panel.gd")
const UIKit = preload("res://ui/uikit.gd")
const VKit = preload("res://ui/vkit.gd")

const C_BG    := Color(0.04, 0.03, 0.02, 0.98)
## panneaux SEMI-TRANSPARENTS : la table du cartographe transparaît derrière
const C_PANEL := Color(0.07, 0.06, 0.05, 0.84)
const ShellPalette = preload("res://ui/shell_palette.gd")
const C_EDGE  := ShellPalette.EDGE
const C_TEXT  := ShellPalette.TEXT
const C_DIM   := ShellPalette.DIM
const C_TITLE := ShellPalette.TITLE

var _main: Control
var _new_game: Control
var _options: Control
var _load: Control
var _load_box: VBoxContainer = null
var _load_msg: Label = null
## UI-1 (retour joueur 2026-09-04 : « Échap quitte la simulation, ne propose pas le
## menu ») — Échap en jeu ouvrait CET écran-titre, sans aucune porte de sortie : ni
## bouton « Reprendre », ni Échap qui referme. Le joueur, devant Jouer/Charger/Options/
## Codex/QUITTER, lisait à juste titre « ma partie est finie ». Le bouton n'apparaît
## qu'EN JEU (au premier boot, avant toute partie, il n'aurait aucune cible).
var _resume_btn: Button = null


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


## FOND : l'écran-titre livré (title_screen.png, 1920×1080, opaque) en COVER plein
## cadre + un voile léger pour la lisibilité — le _draw() sombre reste en repli si
## l'image manque. CHROME 2026-08-26 : composition à l'encre VIDE sur le TIERS
## GAUCHE (table sombre) et la carte peinte sur les deux tiers droits — remplace
## l'ancien fond `menu_main_background.png` (composé, lui, avec un vide au CENTRE ;
## cf. `_build_main()` qui décale le menu en conséquence).
func _build_bg() -> void:
	var tex: Texture2D = UIKit.title_screen()
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

	# CHROME 2026-08-26 : title_screen.png compose son vide à l'encre sur le TIERS
	# GAUCHE (la carte peinte occupe les deux tiers droits) — le menu se décale ICI
	# au lieu du plein-cadre centré d'avant (qui visait le vide CENTRAL de l'ancien
	# fond `menu_main_background.png`, cf. `_build_bg()`).
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.anchor_right = 0.34
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
	# lisible sur le fond photo (il se perdait dans le parchemin sombre) : encre claire
	# + fin liséré sombre, corps un cran au-dessus du défaut.
	sub.add_theme_color_override("font_color", Color(0.86, 0.80, 0.66))
	sub.add_theme_color_override("font_outline_color", Color(0.05, 0.04, 0.03, 0.7))
	sub.add_theme_constant_override("outline_size", 4)
	sub.add_theme_font_size_override("font_size", 18)
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

	# REPRENDRE en TÊTE (UI-1) : la première chose lue doit être le retour au jeu, pas
	# « Jouer » (une nouvelle partie) ni « Quitter ». Caché tant qu'aucune partie ne tourne.
	_resume_btn = _menu_button(tr("T_MENU_RESUME"), func(): resume())
	_resume_btn.visible = false
	col.add_child(_resume_btn)
	col.add_child(_menu_button(tr("T_MENU_PLAY"), func(): _show(_new_game)))
	col.add_child(_menu_button(tr("T_MENU_LOAD"), func(): _show(_load)))
	col.add_child(_menu_button(tr("T_MENU_OPTIONS"), func(): _show(_options)))
	col.add_child(_menu_button("Codex", func(): codex_requested.emit()))
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
		_load_msg.text = tr("T_LOAD_RESTART") if rc == 3 else tr("T_LOAD_FAIL")


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
	if _resume_btn != null:
		_resume_btn.visible = Sim.game_on     # UI-1 : la porte de sortie, visible d'emblée
	Sound.play_music("main_menu")   # le thème du menu reprend

## UI-1 — LE RETOUR AU JEU : le menu se referme, la musique de menu s'éteint, le monde
## reste en PAUSE (le joueur reprend la main par la barre de vitesse, comme après un
## chargement — `_on_load`). Aucune autre conséquence : rien n'est quitté.
func resume() -> void:
	if not Sim.game_on:
		return
	hide()
	Sound.stop_music()

## UI-1 — ÉCHAP DANS LE SHELL (appelé par main.gd::_unhandled_input, qui ne connaît pas
## les écrans d'ici). Règle du joueur : « Échap doit quitter les pop-ups, les fenêtres,
## revenir au menu OU au jeu si on est dans le menu in-game, jamais rien quitter. »
##   · un SOUS-ÉCRAN ouvert (Nouvelle partie / Charger / Options) → retour au menu ;
##   · le menu racine, une partie en cours → RETOUR AU JEU ;
##   · le menu racine sans partie (écran-titre au boot) → rien (Échap ne quitte JAMAIS).
## Renvoie true si l'appui a été consommé.
func escape() -> bool:
	for p in [_new_game, _options, _load]:
		if p != null and p.visible:
			_show(_main)
			return true
	if Sim.game_on:
		resume()
		return true
	return false
