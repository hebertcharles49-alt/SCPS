extends Control
## CountryPanel — le bandeau du ROYAUME, porté au thème viewer.c (VKit). _draw
## immédiat : nom · éthos · pop · trésor, puis les jauges 0-100 (stabilité,
## prospérité, légitimité, cohésion, savoir) avec leur MOT de bande, + influence
## et corruption. Lit country_info (la membrane). Display-only.

const VKit = preload("res://ui/vkit.gd")
const PW := 300.0
const PH := 232.0
const MARGIN := 8.0

var _cid := -1

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(_on_tick)
	hide()

func _layout() -> void:
	position = Vector2(get_viewport_rect().size.x - PW - MARGIN, MARGIN)

func show_country(cid: int) -> void:
	_cid = cid
	visible = cid >= 0
	queue_redraw()

func _on_tick(_year: int) -> void:
	if visible:
		queue_redraw()

func _draw() -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var info: Dictionary = w.country_info(_cid)
	if not bool(info.get("valide", false)):
		return
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	var x := 16.0
	var y := 12.0

	VKit.text(self, Vector2(x, y), VKit.COL_COPPER, String(info["nom"]), VKit.FS_BIG)
	y += 24
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "%s · %d régions" % [info["ethos"], int(info["regions"])])
	y += 22
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "%s âmes · %s or" % [_grp(info["pop"]), _grp(info["or"])])
	y += 24

	_gauge_row(x, y, "Stabilité", int(info["stabilite"]), String(info["stabilite_mot"])); y += 22
	_gauge_row(x, y, "Prospérité", int(info["prosperite"]), String(info["prosperite_mot"])); y += 22
	_gauge_row(x, y, "Légitimité", int(info["legitimite"]), String(info["legitimite_mot"])); y += 22
	_gauge_row(x, y, "Cohésion", int(info["cohesion"]), String(info["cohesion_mot"])); y += 22
	_gauge_row(x, y, "Savoir", int(info["savoir"]), String(info["savoir_mot"])); y += 24

	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Influence"); VKit.text(self, Vector2(x + 104, y), VKit.COL_PARCH, str(info["influence"]))
	if int(info.get("corruption", 0)) > 0:
		VKit.text(self, Vector2(x + 160, y), VKit.COL_DIM, "Corruption %d" % int(info["corruption"]))

## une rangée jauge : libellé · barre rouge→vert à la valeur · "valeur · mot"
func _gauge_row(x: float, y: float, label: String, value: int, word: String) -> void:
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, label)
	VKit.gauge(self, x + 96, y + 3, 64, 9, value)
	VKit.text(self, Vector2(x + 170, y), VKit.COL_PARCH, "%d · %s" % [value, word])

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
