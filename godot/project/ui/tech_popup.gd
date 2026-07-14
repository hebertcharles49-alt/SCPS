extends Control
## TechPopup — le popup de DÉCOUVERTE technologique (une recherche vient de s'ACHEVER) :
## nom, EFFETS (le mécanique — coordonnées « effet » + « hover » chiffré du nœud) et
## FLAVOR. C'est la SEULE apparition du flavor dans tout l'arbre de technologie (le
## reste du panneau — carte, survol, dossier — n'en montre jamais).
## Enfant PERSISTANT de tech_panel.gd (jamais instancié/détruit à la volée) : NE MET PAS
## le jeu en pause et ne couvre qu'une partie du panneau parent (centré dedans) — un clic
## suffit à le refermer, aucun input critique n'est bloqué ailleurs à l'écran.
## Display-only : lit un Dictionary déjà résolu (tech_nodes()[i]), n'appelle aucun verbe.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")

signal closed

const W := 420.0

var _nd: Dictionary = {}
var _lines: PackedStringArray = []
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false

## affiche la découverte d'un nœud (Dictionary issu de tech_nodes()).
func show_tech(nd: Dictionary) -> void:
	_nd = nd
	_lines = _build_lines(nd)
	visible = true
	queue_redraw()

func _build_lines(nd: Dictionary) -> PackedStringArray:
	var out := PackedStringArray()
	var effet := String(nd.get("effet", ""))
	if effet != "":
		out.append(effet)
	# "hover" = "<mot mécanique>[ — d1 · d2 · ...]" (scps_api.c : tech_hover + les deltas
	# chiffrés VIVANTS — dK/dL/dH/dPuissance/dFracture/production%/efficacité%/charge/flux).
	var hov := String(nd.get("hover", ""))
	if hov != "":
		var cut := hov.find(" — ")
		var intro := hov if cut < 0 else hov.substr(0, cut)
		var rest := "" if cut < 0 else hov.substr(cut + 3)
		if intro != "" and intro != effet:
			out.append(intro)
		for b in rest.split(" · "):
			var s := String(b).strip_edges()
			if s != "":
				out.append(s)
	var unl := String(nd.get("unlocks", ""))
	if unl != "":
		out.append("Débouche sur : " + unl)
	if out.is_empty():
		out.append("(aucun effet chiffré)")
	return out

func _height() -> float:
	var h := 50.0 + float(_lines.size()) * 18.0 + 10.0
	if String(_nd.get("flavor", "")) != "":
		h += 1.0 + 10.0 + 3.0 * 17.0
	return h + 12.0

func _draw() -> void:
	if not visible:
		return
	var h := _height()
	size = Vector2(W, h)
	var parent_ctrl := get_parent() as Control
	if parent_ctrl != null:
		position = ((parent_ctrl.size - size) * 0.5).floor()
	VKit.panel_bg(self, Rect2(0, 0, W, h))
	VKit.box(self, Rect2(0, 0, W, h), VKit.COL_GOLD)
	VKit.text(self, Vector2(16, 12), VKit.COL_GOLD, "Découverte : " + String(_nd.get("name", "?")), VKit.FS_BIG)
	VKit.fill(self, Rect2(12, 38, W - 24, 1), VKit.COL_EDGE)
	var y := 48.0
	VKit.detail(self, Vector2(16, y), "Effets :", VKit.FS_SMALL)
	y += 18.0
	for l in _lines:
		VKit.text(self, Vector2(24, y), VKit.COL_PARCH, "• " + String(l), VKit.FS_SMALL)
		y += 18.0
	var flavor := String(_nd.get("flavor", ""))
	if flavor != "":
		y += 3.0
		VKit.fill(self, Rect2(12, y, W - 24, 1), VKit.COL_EDGE)
		y += 9.0
		VKit.text_wrapped(self, Vector2(16, y), VKit.COL_DIM, flavor, W - 32.0, 3, VKit.FS_SMALL)
	_close_rect = Rect2(W - 26, 6, 20, 20)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 6, _close_rect.position.y + 3), VKit.COL_PARCH, "x")
	var hint := "Cliquer pour fermer"
	VKit.detail(self, Vector2(W - 16 - VKit.text_w(hint, VKit.FS_SMALL), h - 16), hint, VKit.FS_SMALL)

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		accept_event()
		visible = false
		Sound.play("ui_click")
		closed.emit()
