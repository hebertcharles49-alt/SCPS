extends Control
## VKitDropdown — un menu déroulant DESSINÉ À LA CHARTE (cuir / or /
## parchemin), en immédiat VKit comme le reste de l'UI (pas d'OptionButton natif,
## impossible à teinter proprement). Émet `selected(index)`. Le Control grandit en
## hauteur quand il est ouvert (la zone cliquable suit la liste visible).
## À placer APRÈS le contenu qu'il recouvre (ordre d'enfant = ordre de dessin).

const VKit = preload("res://ui/vkit.gd")

signal selected(index: int)

const ROW := 24.0

var _items: Array = []
var _idx := 0
var _open := false

func setup(items: Array, idx: int = 0) -> void:
	_items = items
	_idx = clampi(idx, 0, maxi(0, items.size() - 1))
	_set_open(false)

func current() -> int:
	return _idx

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(custom_minimum_size.x, ROW)
	size.y = ROW

func _set_open(v: bool) -> void:
	_open = v
	var h: float = ROW * float(1 + _items.size()) if v else ROW
	custom_minimum_size.y = h
	size.y = h
	queue_redraw()

func _draw() -> void:
	var w: float = size.x
	# la boîte repliée : nom courant + chevron or
	VKit.fill(self, Rect2(0, 0, w, ROW), VKit.COL_PANEL2)
	VKit.box(self, Rect2(0, 0, w, ROW), VKit.COL_GOLD)
	var cur := String(_items[_idx]) if _idx < _items.size() else ""
	VKit.text(self, Vector2(8, 4), VKit.COL_PARCH, cur, VKit.FS_SMALL)
	VKit.text(self, Vector2(w - 16, 4), VKit.COL_GOLD, "▾", VKit.FS_SMALL)
	# la liste déroulée
	if _open:
		for i in _items.size():
			var ry := ROW + float(i) * ROW
			var hot := (i == _idx)
			VKit.fill(self, Rect2(0, ry, w, ROW), VKit.COL_PANEL_HI if hot else VKit.COL_PANEL)
			VKit.box(self, Rect2(0, ry, w, ROW), VKit.COL_EDGE)
			VKit.text(self, Vector2(8, ry + 4), VKit.COL_GOLD if hot else VKit.COL_PARCH,
				String(_items[i]), VKit.FS_SMALL)

func _gui_input(e: InputEvent) -> void:
	if not (e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT):
		return
	var w: float = size.x
	if Rect2(0, 0, w, ROW).has_point(e.position):
		_set_open(not _open)
		accept_event()
		return
	if _open:
		for i in _items.size():
			if Rect2(0, ROW + float(i) * ROW, w, ROW).has_point(e.position):
				_idx = i
				_set_open(false)
				selected.emit(i)
				accept_event()
				return
		_set_open(false)
		accept_event()
