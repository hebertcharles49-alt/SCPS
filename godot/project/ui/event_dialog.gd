extends Control
## EventDialog — LA MEMBRANE DE DÉCISION : un évènement à VRAIE décision (Marbrive…) qui
## concerne le joueur ATTEND ici son choix — l'IA ne tranche PAS à sa place (elle en aurait
## le pouvoir : les autres pays le font). Modal, met le jeu en PAUSE à l'ouverture (comme
## event_popup.gd) ; plusieurs décisions en attente s'enchaînent une par une. Chaque carte
## montre directement action, flavor, deltas chiffrés et prix physique résolu par le moteur.
## Display-only : le clic ENFILE le choix (scps_player_event_choice, drain déterministe) ;
## zéro logique de sim ici.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const EventArt = preload("res://ui/event_art.gd")   ## illustrations par THÈME (réutilisées)

const W := 600.0
const BTN_H := 66.0
const BTN_GAP := 8.0
const BANNER_H := W / 4.0    ## bannière 4:1 (512×128 → 460×115), sous le bandeau-titre

var _slot := -1              ## slot COURANT en cours de résolution (-1 = aucun affiché)
var _pending := {}           ## le Dictionary lu de pending_event(slot)
var _tex: Texture2D = null   ## l'illustration du thème de l'évènement courant
var _btn_rects := []         ## [[Rect2, option:int]] posés au _draw
var _hover_option := -1      ## option SURVOLÉE (pour le tooltip de flavor)
var _prev_speed := -1        ## la vitesse d'avant l'ouverture (restaurée à la fermeture)

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	get_viewport().size_changed.connect(_center)
	Sim.ticked.connect(func(_y): _poll())

## UI-1 — UNE SEULE MODALE D'ÉVÈNEMENT À L'ÉCRAN (cf. le commentaire jumeau dans
## event_popup.gd) : ce dialogue se dessine PAR-DESSUS l'OYEZ (ajouté après lui dans
## `ui`), les deux s'ouvraient chacune de leur côté et l'une passait en sous-couche.
## Groupe partagé « event_modal » ; chacun attend que l'autre ait rendu la main.
func _other_modal_open() -> bool:
	for m in get_tree().get_nodes_in_group("event_modal"):
		if m != self and m is Control and (m as Control).visible:
			return true
	return false

func _wake_others() -> void:
	for m in get_tree().get_nodes_in_group("event_modal"):
		if m != self and m.has_method("wake"):
			m.wake()

## appelée par l'autre modale quand elle se referme : une décision attend peut-être.
func wake() -> void:
	_poll()

## ÉCHAP (main.gd::_close_topmost, tier 0) : on RANGE la boîte sans trancher — le
## pending reste en attente côté moteur et reviendra au prochain tick (aucun choix
## n'est enfilé). La vitesse d'avant est rendue, comme après un choix.
func dismiss() -> void:
	if not visible:
		return
	_slot = -1
	visible = false
	if _prev_speed >= 0:
		Sim.set_speed(_prev_speed)
		_prev_speed = -1
	_wake_others()

## Vérifie s'il y a une décision en attente ; l'ouvre si le dialogue n'est pas déjà visible.
func _poll() -> void:
	if visible or not Sim.game_on:
		return   # avant que la PARTIE ne commence, le monde de fond ne concerne pas le joueur
	if _other_modal_open():
		return   # un OYEZ tient l'écran : la décision attend son tour (wake() la rouvrira)
	var w = Sim.world
	if w == null or not w.has_method("pending_count") or int(w.pending_count()) <= 0:
		return
	_open_slot(0)

func _open_slot(slot: int) -> void:
	var w = Sim.world
	if w == null:
		return
	var pe: Dictionary = w.pending_event(slot)
	if not bool(pe.get("valid", false)):
		return
	_slot = slot
	_pending = pe
	_tex = EventArt.texture_for(int(pe.get("evid", -1)))
	_hover_option = -1
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)              # la décision mérite le regard : le monde attend
		Sound.play("ui_quill")
	visible = true
	move_to_front()               # UI-1 : au-dessus de tout panneau ouvert après nous
	_center()
	queue_redraw()

func _body_lines(situation: String) -> PackedStringArray:
	return _wrap_lines(situation, W - 40.0, VKit.FS_BIG)

func _wrap_lines(text: String, max_width: float, font_size: int = VKit.FS_SMALL) -> PackedStringArray:
	var out := PackedStringArray()
	var line := ""
	for word in text.split(" "):
		if line != "" and VKit.text_w(line + " " + word, font_size) > max_width:
			out.append(line); line = word
		else:
			line = word if line == "" else line + " " + word
	if line != "":
		out.append(line)
	return out

func _option_height(i: int) -> float:
	var blurbs: Array = _pending.get("blurbs", [])
	var effets: Array = _pending.get("effets", [])
	var flavors: Array = _pending.get("flavors", [])
	var lines := 0
	var advisors: Array = _pending.get("advisors", [])
	if i < advisors.size() and String(advisors[i]) != "":
		lines += 1
	for source in [blurbs, effets, flavors]:
		var txt := String(source[i]) if i < source.size() else ""
		if txt != "":
			lines += _wrap_lines(txt, W - 62.0, VKit.FS_SMALL).size()
	return maxf(BTN_H, 34.0 + float(lines) * 15.0 + 8.0)

func _height() -> float:
	var n: int = int(_pending.get("n_options", 0))
	var body_h: float = _body_lines(String(_pending.get("situation", ""))).size() * 22.0
	var choices_h := 0.0
	for i in range(n):
		choices_h += _option_height(i) + BTN_GAP
	return 78.0 + BANNER_H + 6.0 + body_h + 10.0 + choices_h + 14.0

func _center() -> void:
	var vp := get_viewport_rect().size
	size = Vector2(W, _height())
	position = Vector2((vp.x - W) * 0.5, maxf(8.0, (vp.y - size.y) * 0.42))
	queue_redraw()

func _draw() -> void:
	if _slot < 0:
		return
	size = Vector2(W, _height())
	VKit.panel_bg(self, Rect2(0, 0, W, size.y))
	VKit.box(self, Rect2(0, 0, W, size.y), VKit.COL_EDGE)
	# — bandeau : « UNE DÉCISION S'IMPOSE » — UI-POLISH #6 : même reliquat graphite que
	# sidebar_drawer.gd/event_popup.gd (Color(0.075,0.085,0.086) codée en dur, oubliée
	# par le re-skin DA parchemin — TROUVAILLES §DA parchemin a7c9945).
	VKit.fill(self, Rect2(0, 0, W, 34), VKit.COL_PANEL2)
	VKit.fill(self, Rect2(0, 0, 4, 34), VKit.COL_GOLD)
	VKit.fill(self, Rect2(4, 33, W - 4, 1), VKit.COL_EDGE)
	var head := "— UNE DÉCISION S'IMPOSE —"
	VKit.text(self, Vector2((W - VKit.text_w(head, VKit.FS_BIG)) * 0.5, 8), VKit.COL_GOLD, head, VKit.FS_BIG)
	# — l'ILLUSTRATION du thème (bannière 4:1, réutilisée par famille d'évènements) —
	if _tex != null:
		draw_texture_rect(_tex, Rect2(1, 35, W - 2.0, BANNER_H), false)
	else:
		VKit.fill(self, Rect2(1, 35, W - 2.0, BANNER_H), VKit.COL_PANEL2)
	VKit.fill(self, Rect2(0, 35 + BANNER_H, W, 1), VKit.COL_EDGE)
	# — la SITUATION (le nom de l'évènement, résolu — membrane) —
	var y := 46.0 + BANNER_H + 6.0
	for l in _body_lines(String(_pending.get("situation", ""))):
		VKit.text(self, Vector2(18, y), VKit.COL_PARCH, l, VKit.FS_BIG)
		y += 22.0
	y += 8.0
	# — LES CHOIX : label plein + liseré doré si survolé —
	_btn_rects.clear()
	var labels: Array = _pending.get("labels", [])
	var blurbs: Array = _pending.get("blurbs", [])
	var effets: Array = _pending.get("effets", [])
	var flavors: Array = _pending.get("flavors", [])
	var gold_delta: Array = _pending.get("gold_delta", [])
	var n: int = int(_pending.get("n_options", 0))
	for i in range(n):
		var r := Rect2(18, y, W - 36.0, _option_height(i))
		var hovered := (i == _hover_option)
		VKit.fill(self, r, VKit.COL_PANEL2 if not hovered else VKit.COL_PANEL_HI)
		VKit.box(self, r, VKit.COL_GOLD if hovered else VKit.COL_DIM)
		var lbl := String(labels[i]) if i < labels.size() else "—"
		VKit.text(self, Vector2(r.position.x + 12, r.position.y + 7), VKit.COL_PARCH, lbl)
		# Prix physique résolu par le moteur au cours courant, toujours visible — 0 compris.
		var gd := float(gold_delta[i]) if i < gold_delta.size() else 0.0
		var price := "Gain %+d couronnes" % int(round(gd)) if gd > 0.49 else ("Coût %d couronnes" % int(round(-gd)) if gd < -0.49 else "Coût 0 couronnes")
		var pcol := VKit.sense(0.82 if gd >= 0.0 else 0.15)
		VKit.text(self, Vector2(r.end.x - VKit.text_w(price, VKit.FS_SMALL) - 10, r.position.y + 9), pcol, price, VKit.FS_SMALL)
		# — le VISAGE du choix : la faction qui le porte au conseil (aligné à droite, discret) —
		var advisors: Array = _pending.get("advisors", [])
		var adv := String(advisors[i]) if i < advisors.size() else ""
		if adv != "":
			var atxt := "— " + adv
			VKit.text(self, Vector2(r.position.x + r.size.x - VKit.text_w(atxt, VKit.FS_SMALL) - 10,
				r.position.y + 25), VKit.COL_DIM, atxt, VKit.FS_SMALL)
		var ty := r.position.y + 25.0 + (15.0 if adv != "" else 0.0)
		for entry in [[blurbs, VKit.COL_PARCH], [effets, VKit.COL_GOLD], [flavors, VKit.COL_DIM]]:
			var arr: Array = entry[0]
			var txt := String(arr[i]) if i < arr.size() else ""
			if txt == "":
				continue
			for line in _wrap_lines(txt, r.size.x - 24.0, VKit.FS_SMALL):
				VKit.text(self, Vector2(r.position.x + 12, ty), entry[1], line, VKit.FS_SMALL)
				ty += 15.0
		_btn_rects.append([r, i])
		y += r.size.y + BTN_GAP

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var prev := _hover_option
		_hover_option = -1
		for br in _btn_rects:
			if (br[0] as Rect2).has_point(event.position):
				_hover_option = int(br[1])
				break
		if _hover_option != prev:
			queue_redraw()
		return
	if not (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT):
		return
	accept_event()
	for br in _btn_rects:
		if (br[0] as Rect2).has_point(event.position):
			_choose(int(br[1]))
			return

func _choose(option: int) -> void:
	if Sim.world != null and Sim.world.has_method("player_event_choice"):
		Sim.world.player_event_choice(_slot, option)   # verbe journalisé (drainé au tick suivant)
	Sound.play("ui_seal")
	_slot = -1
	visible = false
	if _prev_speed >= 0:
		Sim.set_speed(_prev_speed)
		_prev_speed = -1
	_wake_others()   # UI-1 : un OYEZ patientait peut-être derrière — à lui l'écran
	# PAS de _poll() immédiat ici : le choix vient d'être ENFILÉ, pas encore DRAINÉ — ce
	# MÊME pending est encore compté par pending_count() jusqu'au prochain tick (Sim.ticked),
	# qui rappellera _poll() naturellement. Un poll immédiat rouvrirait CE pending à l'instant.

## HOVER natif : l'EFFET MÉCANIQUE d'abord (retour joueur : « Ça veut dire quoi ? »),
## puis ce que RACONTE le choix (flavor — jamais un nom SCPS).
func _get_tooltip(at_position: Vector2) -> String:
	for br in _btn_rects:
		if (br[0] as Rect2).has_point(at_position):
			var i: int = int(br[1])
			var effets: Array = _pending.get("effets", [])
			var flavors: Array = _pending.get("flavors", [])
			var eff := String(effets[i]) if i < effets.size() else ""
			var fla := String(flavors[i]) if i < flavors.size() else ""
			if eff != "" and fla != "":
				return eff + "\n" + fla
			return eff if eff != "" else fla
	return ""
