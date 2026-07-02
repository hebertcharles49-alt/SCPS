extends Control
## OYEZ OYEZ — le POPUP d'évènement (directeur & alertes MAJEURES : révolte, guerre,
## paix, sécession). Met le jeu en PAUSE à l'ouverture ; le CRIEUR en tête (bandeau
## reconnaissable) ; boutons ADAPTATIFS à la situation en bas (« Y aller », « Réprimer »,
## « Lever l'ost », « Voir la diplomatie », « Vu »). File d'attente : plusieurs évènements
## s'enchaînent un par un ; la vitesse d'AVANT est restaurée à la fermeture du dernier.
## Display-only : les clics émettent des signaux (main câble) ou un verbe journalisé.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")

signal goto_region(r: int)
signal open_tab(i: int)

const W := 470.0
const BTN_H := 26.0

var _queue := []          ## [{title, body, buttons:[{label, act, region}]}]
var _cur := {}
var _btn_rects := []      ## [[Rect2, act, region]] posés au _draw
var _prev_speed := -1     ## la vitesse d'avant l'ouverture (restaurée à la fin de la file)

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	get_viewport().size_changed.connect(_center)

## ENFILE un évènement (depuis alerts.gd) — ouvre si rien d'affiché.
func enqueue(e: Dictionary) -> void:
	_queue.append(e)
	if not visible:
		_show_next()

func _show_next() -> void:
	if _queue.is_empty():
		visible = false
		if _prev_speed >= 0:
			Sim.set_speed(_prev_speed)             # la vie reprend à la vitesse d'avant
			_prev_speed = -1
		return
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)                           # PAUSE : l'évènement mérite le regard
	_cur = _queue.pop_front()
	visible = true
	_center()
	queue_redraw()

func _body_lines() -> PackedStringArray:
	# repli manuel (~54 caractères par ligne) du corps de texte
	var out := PackedStringArray()
	for para in String(_cur.get("body", "")).split("\n"):
		var line := ""
		for word in para.split(" "):
			if line.length() + word.length() + 1 > 54:
				out.append(line)
				line = word
			else:
				line = word if line == "" else line + " " + word
		out.append(line)
	return out

func _height() -> float:
	return 96.0 + _body_lines().size() * 16.0 + BTN_H + 18.0

func _center() -> void:
	var vp := get_viewport_rect().size
	size = Vector2(W, _height())
	position = Vector2((vp.x - W) * 0.5, (vp.y - size.y) * 0.42)
	queue_redraw()

## kind d'évènement → TAMPON à l'encre (planche 3) : guerre=étoile · paix=colombe ·
## révolte=flamme · sécession/directeur=couronne.
const STAMP_OF := {1: "sheet03_popup_seals_11", 2: "sheet03_popup_seals_15",
	6: "sheet03_popup_seals_14", 7: "sheet03_popup_seals_12", 10: "sheet03_popup_seals_12"}

func _draw() -> void:
	size = Vector2(W, _height())
	VKit.panel_bg(self, Rect2(0, 0, W, size.y))
	VKit.box(self, Rect2(0, 0, W, size.y), VKit.COL_COPPER)
	# — filigrane rosace (presque invisible) au centre du parchemin —
	var flg: Texture2D = UIKit.parch_tex("sheet03_popup_seals_16")
	if flg != null:
		var fs := minf(W, size.y) * 0.72
		draw_texture_rect(flg, Rect2((W - fs) * 0.5, (size.y - fs) * 0.5, fs, fs),
			false, Color(1, 1, 1, 0.10))
	# — LE CRIEUR : bandeau sombre + « ⚜ OYEZ OYEZ ⚜ » cuivre, reconnaissable entre tous —
	VKit.fill(self, Rect2(0, 0, W, 34), Color(0.09, 0.07, 0.05, 0.96))
	VKit.fill(self, Rect2(0, 32, W, 2), VKit.COL_COPPER)
	var oy := "— OYEZ  OYEZ —"
	VKit.text(self, Vector2((W - VKit.text_w(oy, VKit.FS_BIG)) * 0.5, 8), VKit.COL_COPPER, oy, VKit.FS_BIG)
	# — TAMPON du kind, posé de biais sur le coin haut-droit (la lettre marquée) —
	var stp: String = STAMP_OF.get(int(_cur.get("kind", -1)), "")
	if stp != "":
		var st: Texture2D = UIKit.parch_tex(stp)
		if st != null:
			var ss := 84.0
			draw_set_transform(Vector2(W - 58.0, 76.0), -0.20, Vector2.ONE)
			draw_texture_rect(st, Rect2(-ss * 0.5, -ss * 0.5, ss, ss), false, Color(1, 1, 1, 0.95))
			draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	# — titre + an —
	var title := String(_cur.get("title", ""))
	VKit.text(self, Vector2(18, 44), VKit.COL_PARCH, title, VKit.FS_BIG)
	# — corps —
	var y := 70.0
	for l in _body_lines():
		VKit.text(self, Vector2(18, y), VKit.COL_DIM, l)
		y += 16.0
	# — BOUTONS ADAPTATIFS (droite → gauche : « Vu » toujours en dernier à droite) —
	_btn_rects.clear()
	var bx := W - 14.0
	var btns: Array = _cur.get("buttons", [])
	for i in range(btns.size() - 1, -1, -1):
		var b: Dictionary = btns[i]
		var label := String(b["label"])
		var bw := VKit.text_w(label) + 22.0
		bx -= bw
		var r := Rect2(bx, size.y - BTN_H - 12.0, bw, BTN_H)
		VKit.fill(self, r, VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_COPPER)
		VKit.text(self, Vector2(r.position.x + 11, r.position.y + 5), VKit.COL_COPPER, label)
		_btn_rects.append([r, String(b.get("act", "close")), int(b.get("region", -1))])
		bx -= 8.0

func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT):
		return
	accept_event()
	for br in _btn_rects:
		if (br[0] as Rect2).has_point(event.position):
			_fire(String(br[1]), int(br[2]))
			return

## le VERBE du bouton — puis l'évènement suivant de la file.
func _fire(act: String, region: int) -> void:
	match act:
		"goto":
			if region >= 0:
				goto_region.emit(region)
		"repress":
			if region >= 0 and Sim.world != null and Sim.world.has_method("player_repress"):
				Sim.world.player_repress(region)   # verbe journalisé (drainé au tick)
		"army":
			open_tab.emit(4)
		"diplo":
			open_tab.emit(6)
		_:
			pass                                    # « Vu » : rien d'autre à faire
	_show_next()
