extends Control
## EventDialog — LA MEMBRANE DE DÉCISION : un évènement à VRAIE décision (Marbrive…) qui
## concerne le joueur ATTEND ici son choix — l'IA ne tranche PAS à sa place (elle en aurait
## le pouvoir : les autres pays le font). Modal, met le jeu en PAUSE à l'ouverture (comme
## event_popup.gd) ; plusieurs décisions en attente s'enchaînent une par une. Le survol
## d'un bouton montre CE QUE RACONTE le choix (flavor) — jamais un nom SCPS, jamais un
## nombre de coordonnée : la façade ne passe que des MOTS (scps_pending_event).
## Display-only : le clic ENFILE le choix (scps_player_event_choice, drain déterministe) ;
## zéro logique de sim ici.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")

const W := 460.0
const BTN_H := 34.0
const BTN_GAP := 8.0

var _slot := -1              ## slot COURANT en cours de résolution (-1 = aucun affiché)
var _pending := {}           ## le Dictionary lu de pending_event(slot)
var _btn_rects := []         ## [[Rect2, option:int]] posés au _draw
var _hover_option := -1      ## option SURVOLÉE (pour le tooltip de flavor)
var _prev_speed := -1        ## la vitesse d'avant l'ouverture (restaurée à la fermeture)

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	get_viewport().size_changed.connect(_center)
	Sim.ticked.connect(func(_y): _poll())

## Vérifie s'il y a une décision en attente ; l'ouvre si le dialogue n'est pas déjà visible.
func _poll() -> void:
	if visible or not Sim.game_on:
		return   # avant que la PARTIE ne commence, le monde de fond ne concerne pas le joueur
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
	_hover_option = -1
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)              # la décision mérite le regard : le monde attend
	visible = true
	_center()
	queue_redraw()

func _body_lines(situation: String) -> PackedStringArray:
	var out := PackedStringArray()
	var line := ""
	for word in situation.split(" "):
		if VKit.text_w(line + " " + word, VKit.FS_BIG) > W - 40.0:
			out.append(line); line = word
		else:
			line = word if line == "" else line + " " + word
	if line != "":
		out.append(line)
	return out

func _height() -> float:
	var n: int = int(_pending.get("n_options", 0))
	var body_h: float = _body_lines(String(_pending.get("situation", ""))).size() * 22.0
	return 78.0 + body_h + 10.0 + n * (BTN_H + BTN_GAP) + 14.0

func _center() -> void:
	var vp := get_viewport_rect().size
	size = Vector2(W, _height())
	position = Vector2((vp.x - W) * 0.5, (vp.y - size.y) * 0.42)
	queue_redraw()

func _draw() -> void:
	if _slot < 0:
		return
	size = Vector2(W, _height())
	VKit.panel_bg(self, Rect2(0, 0, W, size.y))
	VKit.box(self, Rect2(0, 0, W, size.y), VKit.COL_GOLD)
	# — bandeau : « UNE DÉCISION S'IMPOSE » —
	VKit.fill(self, Rect2(0, 0, W, 34), Color(0.09, 0.07, 0.05, 0.96))
	VKit.fill(self, Rect2(0, 32, W, 2), VKit.COL_GOLD)
	var head := "— UNE DÉCISION S'IMPOSE —"
	VKit.text(self, Vector2((W - VKit.text_w(head, VKit.FS_BIG)) * 0.5, 8), VKit.COL_GOLD, head, VKit.FS_BIG)
	# — la SITUATION (le nom de l'évènement, résolu — membrane) —
	var y := 46.0
	for l in _body_lines(String(_pending.get("situation", ""))):
		VKit.text(self, Vector2(18, y), VKit.COL_PARCH, l, VKit.FS_BIG)
		y += 22.0
	y += 8.0
	# — LES CHOIX : label plein + liseré doré si survolé —
	_btn_rects.clear()
	var labels: Array = _pending.get("labels", [])
	var n: int = int(_pending.get("n_options", 0))
	for i in range(n):
		var r := Rect2(18, y, W - 36.0, BTN_H)
		var hovered := (i == _hover_option)
		VKit.fill(self, r, VKit.COL_PANEL2 if not hovered else VKit.COL_PANEL_HI)
		VKit.box(self, r, VKit.COL_GOLD if hovered else VKit.COL_DIM)
		var lbl := String(labels[i]) if i < labels.size() else "—"
		VKit.text(self, Vector2(r.position.x + 12, r.position.y + 9), VKit.COL_PARCH, lbl)
		_btn_rects.append([r, i])
		y += BTN_H + BTN_GAP

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
	_slot = -1
	visible = false
	if _prev_speed >= 0:
		Sim.set_speed(_prev_speed)
		_prev_speed = -1
	# PAS de _poll() immédiat ici : le choix vient d'être ENFILÉ, pas encore DRAINÉ — ce
	# MÊME pending est encore compté par pending_count() jusqu'au prochain tick (Sim.ticked),
	# qui rappellera _poll() naturellement. Un poll immédiat rouvrirait CE pending à l'instant.

## HOVER natif : ce que RACONTE le choix survolé (flavor — jamais un nom SCPS).
func _get_tooltip(at_position: Vector2) -> String:
	for br in _btn_rects:
		if (br[0] as Rect2).has_point(at_position):
			var flavors: Array = _pending.get("flavors", [])
			var i: int = int(br[1])
			return String(flavors[i]) if i < flavors.size() else ""
	return ""
