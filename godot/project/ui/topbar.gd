extends Control
## Topbar — habillée au pack : un bandeau de chrome (caps + centre) portant la
## capsule de date (An N), la pop et le nombre de pays avec leurs icônes, et un
## contrôle de VITESSE cliquable (icône pause/avance). Display-only sauf le verbe
## vitesse (cadence d'affichage). Lit Sim.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const W := 452.0
const H := 34.0
const CAP := 26.0   # largeur des embouts

var _speed_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	position = Vector2(8, 8)
	size = Vector2(W, H)
	Sim.generated.connect(_on_change)
	Sim.ticked.connect(_on_tick)

func _on_tick(_year: int) -> void:
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	# bandeau : le panneau DESSINÉ (navy + liseré cuivre, comme les panneaux) — crisp
	# et cohérent. (On NE tuile/n'étire PAS la pièce de chrome : étirée à 34 px de
	# haut, elle bavait en barre grise — pire sous Godot 4.6.)
	VKit.panel_bg(self, Rect2(0, 0, W, H))

	if Sim.world == null:
		VKit.text(self, Vector2(16, 9), VKit.COL_DIM, "(libscps absente — voir README)")
		return
	if not Sim.world.has_method("province_at"):
		VKit.text(self, Vector2(12, 9), VKit.sense(0.5),
			"⚠ libscps OBSOLÈTE — rebâtir : scons platform=windows use_mingw=yes")
		return

	var w = Sim.world
	# capsule de date : An N
	UIKit.draw_chrome(self, "topbar_date_capsule", Rect2(10, 4, 92, H - 8))
	VKit.text(self, Vector2(22, 9), VKit.COL_PARCH, "An %d" % w.year())

	# pop · pays, chacun avec icône
	UIKit.draw_icon(self, "population_group", Vector2(116, 7), 18)
	VKit.text(self, Vector2(138, 9), VKit.COL_PARCH, _grp(w.world_pop()))
	UIKit.draw_icon(self, "politics_crown", Vector2(232, 7), 18)
	VKit.text(self, Vector2(254, 9), VKit.COL_PARCH, "%d pays" % w.country_count())

	# contrôle de vitesse (cliquable) : icône + libellé
	_speed_rect = Rect2(W - 116, 4, 108, H - 8)
	UIKit.draw_chrome(self, "topbar_resource_chip", _speed_rect)
	var paused: bool = Sim.speed_index == 0
	UIKit.draw_icon(self, "control_pause" if not paused else "control_fast_forward",
		Vector2(_speed_rect.position.x + 6, 7), 18)
	VKit.text(self, Vector2(_speed_rect.position.x + 28, 9), VKit.COL_COPPER, Sim.speed_label())

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _speed_rect.has_point(event.position):
			Sim.cycle_speed()
			queue_redraw()

func _grp(n) -> String:
	var s := str(absi(int(n)))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" if int(n) < 0 else "") + out
