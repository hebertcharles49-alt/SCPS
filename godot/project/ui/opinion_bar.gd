extends Control
## Jauge diplomatique canonique : -100 ← 0 → +100. Toute opinion utilise ce
## langage, avec la valeur courante en remplissage et l'équilibre en repère fin.

const VKit = preload("res://ui/vkit.gd")

var opinion := 0
var target := 0

func _ready() -> void:
	custom_minimum_size = Vector2(220.0, 34.0)
	mouse_filter = Control.MOUSE_FILTER_PASS

func set_values(current: int, equilibrium: int) -> void:
	opinion = clampi(current, -100, 100)
	target = clampi(equilibrium, -100, 100)
	tooltip_text = "Opinion\n• Actuelle : %+d / 100\n• Équilibre : %+d / 100" % [opinion, target]
	queue_redraw()

func _draw() -> void:
	var x := 8.0
	var y := 12.0
	var w := maxf(120.0, size.x - 66.0)
	var h := 10.0
	var mid := x + w * 0.5
	VKit.fill(self, Rect2(x, y, w, h), Color(0.045, 0.05, 0.05, 1.0))
	VKit.box(self, Rect2(x - 1.0, y - 1.0, w + 2.0, h + 2.0), VKit.COL_EDGE)
	VKit.fill(self, Rect2(mid - 1.0, y - 3.0, 2.0, h + 6.0), VKit.COL_PARCH)
	var extent := absf(float(opinion)) / 100.0 * w * 0.5
	var col := VKit.sense(0.15 if opinion < 0 else 0.82)
	if extent > 0.0:
		VKit.fill(self, Rect2(mid - extent if opinion < 0 else mid, y + 1.0, extent, h - 2.0), col)
	var tx := x + (float(target) + 100.0) / 200.0 * w
	VKit.fill(self, Rect2(tx - 1.0, y - 5.0, 2.0, h + 10.0), VKit.COL_GOLD)
	VKit.text(self, Vector2(x, y + 13.0), VKit.COL_DIM, "-100", VKit.FS_SMALL)
	VKit.text(self, Vector2(mid - 3.0, y + 13.0), VKit.COL_DIM, "0", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + w - VKit.text_w("+100", VKit.FS_SMALL), y + 13.0), VKit.COL_DIM, "+100", VKit.FS_SMALL)
	var value := "%+d" % opinion
	VKit.text(self, Vector2(size.x - VKit.text_w(value) - 6.0, y - 2.0), col, value)
