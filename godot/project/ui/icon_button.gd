extends Control
## IconButton — un bouton habillé au pack : un fond de chrome (button_square_*) +
## une icône, OU une pièce de chrome auto-suffisante (map_zoom_*, control_*). Gère
## survol/sélection (états de chrome quand ils existent, sinon une teinte). Émet
## `pressed`. Réutilisé par les barres de mode, de zoom et le rail de sidebar.

const UIKit = preload("res://ui/uikit.gd")

signal pressed

var bg := "button_square_normal"   ## chrome de fond ("" = aucun, l'avant-plan EST le bouton)
var fg := ""                       ## icône (icons/) OU pièce de chrome si fg_is_chrome
var fg_is_chrome := false
var selected := false
var pad_frac := 0.18

var _hover := false

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	mouse_entered.connect(func(): _hover = true; queue_redraw())
	mouse_exited.connect(func(): _hover = false; queue_redraw())

## raccourci de config (icône sur fond carré)
func setup_icon(icon_name: String, sz: float, background := "button_square_normal") -> void:
	fg = icon_name; fg_is_chrome = false; bg = background
	custom_minimum_size = Vector2(sz, sz); size = Vector2(sz, sz)

## raccourci de config (pièce de chrome auto-suffisante : map_zoom_in, control_gear…)
func setup_chrome(chrome_name: String, sz: float) -> void:
	fg = chrome_name; fg_is_chrome = true; bg = ""
	custom_minimum_size = Vector2(sz, sz); size = Vector2(sz, sz)

func _draw() -> void:
	var r := Rect2(Vector2.ZERO, size)
	# fond : état sélectionné/survolé via les variantes de chrome si présentes
	if bg != "":
		var name := bg
		if selected and UIKit.chrome("button_square_selected") != null:
			name = "button_square_selected"
		elif _hover and UIKit.chrome("button_square_hover") != null:
			name = "button_square_hover"
		UIKit.draw_chrome(self, name, r)
	# avant-plan
	var mod := Color.WHITE
	if bg == "":
		# pièce auto-suffisante : la teinte porte l'état
		if selected: mod = Color(1.15, 1.05, 0.8)
		elif _hover: mod = Color(1.12, 1.12, 1.12)
		else: mod = Color(0.92, 0.92, 0.92)
	else:
		# icône d'encre sombre sur chrome cuir sombre : un LIFT rend le glyphe lisible
		# (les onglets du rail étaient des taches illisibles — capture 2026-07-09)
		if selected: mod = Color(1.45, 1.35, 1.05)
		elif _hover: mod = Color(1.40, 1.40, 1.35)
		else: mod = Color(1.28, 1.26, 1.20)
	if fg != "":
		if fg_is_chrome:
			UIKit.draw_chrome(self, fg, r, mod)
		else:
			var p := size.x * pad_frac
			UIKit.draw_icon(self, fg, Vector2(p, p), size.x - 2 * p, mod)
	# icône NUE (sans chrome) : l'état sélectionné se dit par un soulignement or
	if bg == "" and not fg_is_chrome:
		if selected:
			draw_rect(Rect2(3, size.y - 3, size.x - 6, 2), Color(0.86, 0.68, 0.26))
		elif _hover:
			draw_rect(Rect2(3, size.y - 3, size.x - 6, 2), Color(0.86, 0.68, 0.26, 0.4))

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		pressed.emit()
		accept_event()
