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

## UI-1 (retour joueur 2026-09-04 : « les events se superposent : le pop-up apparaît, et
## le deuxième OYEZ OYEZ arrive en sous-couche ») — UNE SEULE MODALE D'ÉVÈNEMENT À
## L'ÉCRAN. Le vrai coupable n'était pas cette file (elle marchait) mais l'AUTRE modale :
## event_dialog.gd est ajouté APRÈS ce popup dans `ui` (main.gd), donc dessiné PAR-DESSUS,
## et les deux s'ouvraient chacune de leur côté — l'OYEZ restait dessous, invisible et
## pourtant vivant (il volait aussi la vitesse d'avant à la restauration). Les deux nœuds
## partagent le groupe « event_modal » : chacun attend que l'autre ait rendu la main.
## (Le même couple de fonctions vit dans event_dialog.gd — deux nœuds, une règle.)
func _other_modal_open() -> bool:
	for m in get_tree().get_nodes_in_group("event_modal"):
		if m != self and m is Control and (m as Control).visible:
			return true
	return false

## réveille l'autre modale : c'est son tour si elle a quelque chose en attente.
func _wake_others() -> void:
	for m in get_tree().get_nodes_in_group("event_modal"):
		if m != self and m.has_method("wake"):
			m.wake()

## appelée par l'autre modale quand elle se referme (rien à faire si la file est vide).
func wake() -> void:
	if not visible and not _queue.is_empty():
		_show_next()

## ÉCHAP (main.gd::_close_topmost, tier 0) : « Vu » sans cliquer — la file poursuit.
func dismiss() -> void:
	_fire("close", -1)

func _show_next() -> void:
	if _queue.is_empty():
		visible = false
		if _prev_speed >= 0:
			Sim.set_speed(_prev_speed)             # la vie reprend à la vitesse d'avant
			_prev_speed = -1
		_wake_others()
		return
	if _other_modal_open():
		visible = false                            # notre tour viendra (file conservée)
		return
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)                           # PAUSE : l'évènement mérite le regard
	_cur = _queue.pop_front()
	visible = true
	move_to_front()                                # jamais sous un panneau ouvert après nous
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
	VKit.box(self, Rect2(0, 0, W, size.y), VKit.COL_EDGE)
	# — filigrane rosace (presque invisible) au centre du parchemin —
	var flg: Texture2D = UIKit.parch_tex("sheet03_popup_seals_16")
	if flg != null:
		var fs := minf(W, size.y) * 0.72
		draw_texture_rect(flg, Rect2((W - fs) * 0.5, (size.y - fs) * 0.5, fs, fs),
			false, Color(1, 1, 1, 0.10))
	# — LE CRIEUR : bandeau + « ⚜ OYEZ OYEZ ⚜ » doré, reconnaissable entre tous — UI-POLISH
	# #6 : le bandeau était SOMBRE (même reliquat graphite que sidebar_drawer.gd/
	# event_dialog.gd, Color(0.075,0.085,0.086) codée en dur, oubliée par le re-skin DA
	# parchemin) ; passé au HeaderStrip parchemin, cohérent avec le reste du panneau.
	VKit.fill(self, Rect2(0, 0, W, 34), VKit.COL_PANEL2)
	VKit.fill(self, Rect2(0, 0, 4, 34), VKit.COL_GOLD)
	VKit.fill(self, Rect2(4, 33, W - 4, 1), VKit.COL_EDGE)
	var oy := "— OYEZ  OYEZ —"
	VKit.text(self, Vector2((W - VKit.text_w(oy, VKit.FS_BIG)) * 0.5, 8), VKit.COL_GOLD, oy, VKit.FS_BIG)
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
		VKit.box(self, r, VKit.COL_GOLD)
		VKit.text(self, Vector2(r.position.x + 11, r.position.y + 5), VKit.COL_GOLD, label)
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

## région → 1re province possédée qui s'y trouve (RE-KEY PROVINCE : le bouton
## « Réprimer » de l'évènement porte une RÉGION — l'évènement de révolte est
## région-grain à sa source — mais player_repress veut un PID direct ; le journal
## d'évènement n'est pas dans le périmètre de cette bascule, on résout ici, côté UI).
func _first_owned_prov_in_region(w, region: int) -> int:
	if w == null or region < 0:
		return -1
	var me := int(w.player())
	for pid in range(w.province_count()):
		if int(w.province_region(pid)) != region:
			continue
		var pi: Dictionary = w.province_info(pid)
		if int(pi.get("owner", -2)) == me:
			return pid
	return -1

## le VERBE du bouton — puis l'évènement suivant de la file.
func _fire(act: String, region: int) -> void:
	match act:
		"goto":
			if region >= 0:
				goto_region.emit(region)
		"repress":
			if region >= 0 and Sim.world != null and Sim.world.has_method("player_repress"):
				var pid := _first_owned_prov_in_region(Sim.world, region)
				if pid >= 0:
					Sim.world.player_repress(pid)   # verbe journalisé (drainé au tick)
		"army":
			open_tab.emit(4)
		"diplo":
			open_tab.emit(6)
		_:
			pass                                    # « Vu » : rien d'autre à faire
	_show_next()
