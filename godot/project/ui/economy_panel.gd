extends Control
## EconomyPanel — les GRAPHES d'économie du JOUEUR dans le temps (read-only),
## bascule touche G. UN graphe Easy Charts + un MENU DÉROULANT pour choisir la
## métrique (Population · Trésor · Prospérité) — chacune prend toute l'échelle
## verticale (auto-ajustée à sa plage). L'historique s'accumule an par an depuis
## la façade (country_pop / budget_summary / country_info), même panneau caché.
## Charte EU4 × RimWorld. Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const VKitDropdown = preload("res://ui/vkit_dropdown.gd")
const LINECHART = preload("res://addons/easy_charts/control_charts/LineChart/line_chart.tscn")
# taille ADAPTATIVE à la fenêtre (recalculée dans _layout ; plancher = l'ancienne taille fixe)
var PW := 720.0
var PH := 482.0
const HEAD := 40.0
const CAP := 300             # plafond d'historique (≈ 3 siècles)

const METRICS := [
	{"name": "Population", "ylabel": "âmes",   "col": Color(0.42, 0.78, 0.50)},
	{"name": "Trésor",     "ylabel": "or",     "col": Color(0.86, 0.72, 0.30)},
	{"name": "Prospérité", "ylabel": "indice", "col": Color(0.45, 0.65, 0.92)},
]

var _years := []
var _series := [[], [], []]   # une pile par métrique (même ordre que METRICS)
var _last_year := -1
var _sel := 0
var _chart
var _dropdown
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	# le graphe d'ABORD…
	_chart = LINECHART.instantiate()
	_chart.set_anchors_preset(Control.PRESET_TOP_LEFT)   # scène EC en plein-cadre → on FIXE la taille
	_chart.position = Vector2(16, HEAD + 6)
	_chart.size = Vector2(PW - 32, PH - HEAD - 24)
	_chart.custom_minimum_size = _chart.size
	_chart.visible = false
	add_child(_chart)
	# … le menu déroulant APRÈS (il se dessine PAR-DESSUS le graphe quand il s'ouvre) —
	# ancré à DROITE du header (il TRONQUAIT le titre à x=210, capture 2026-07-10)
	_dropdown = VKitDropdown.new()
	_dropdown.position = Vector2(PW - 30.0 - 176.0, 6.0)
	_dropdown.size = Vector2(170, 24)
	_dropdown.custom_minimum_size = Vector2(170, 24)
	add_child(_dropdown)
	var names := []
	for m in METRICS:
		names.append(String(m["name"]))
	_dropdown.setup(names, 0)
	_dropdown.selected.connect(_on_metric)
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(_on_tick)
	hide()

func _layout() -> void:
	var vp := get_viewport_rect().size
	PW = clampf(vp.x * 0.48, 720.0, 1100.0)
	PH = clampf(vp.y * 0.55, 482.0, 820.0)
	size = Vector2(PW, PH)
	position = Vector2((vp.x - PW) * 0.5, (vp.y - PH) * 0.5)
	if _chart != null and is_instance_valid(_chart):
		_chart.size = Vector2(PW - 32, PH - HEAD - 24)
		_chart.custom_minimum_size = _chart.size
	if _dropdown != null and is_instance_valid(_dropdown):
		_dropdown.position = Vector2(PW - 30.0 - 176.0, 6.0)

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(e.position):
			visible = false
			accept_event()
			return

func _on_metric(idx: int) -> void:
	_sel = clampi(idx, 0, METRICS.size() - 1)
	if visible:
		_replot()
	queue_redraw()

func _on_generated() -> void:
	_years.clear()
	_series = [[], [], []]
	_last_year = -1
	if _chart != null:
		_chart.visible = false

func _on_tick(year: int) -> void:
	if Sim.world == null or year == _last_year:
		return
	_last_year = year
	var w = Sim.world
	var me: int = w.player()
	_years.append(float(year))
	_series[0].append(float(w.country_pop(me)))
	var b: Dictionary = w.budget_summary(me)
	_series[1].append(float(b.get("gold", 0.0)))
	var info: Dictionary = w.country_info(me)
	_series[2].append(float(int(info.get("prosperite", 0))) if bool(info.get("valide", false)) else 0.0)
	if _years.size() > CAP:
		_years.pop_front()
		for s in _series:
			s.pop_front()
	if visible:
		_replot()

func _replot() -> void:
	if _years.size() < 2:
		return
	var m: Dictionary = METRICS[_sel]
	var cp = ChartProperties.new()
	cp.title = String(m["name"])
	cp.x_label = "année"
	cp.y_label = String(m["ylabel"])
	cp.grid = true
	cp.ticks = true
	cp.points = false
	cp.origin = false
	cp.x_scale = 8
	cp.y_scale = 6                       # l'échelle Y s'auto-ajuste à la plage de CETTE métrique
	# CHARTE : fond transparent (cuir du panneau), axes+texte parchemin, grille discrète
	cp.background = false
	cp.colors.bounding_box = VKit.COL_PARCH
	cp.colors.grid = VKit.COL_EDGE
	cp.colors.functions = [m["col"]]
	_chart.visible = true
	_chart.plot(_years, [_series[_sel]], cp)

func _notification(what: int) -> void:
	if what == NOTIFICATION_VISIBILITY_CHANGED and is_inside_tree() and visible:
		_replot()
		queue_redraw()

func _draw() -> void:
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	_close_rect = VKit.header(self, PW, "Économie dans le temps")

	if _years.size() < 2:
		VKit.text(self, Vector2(16, HEAD + 24), VKit.COL_DIM,
			"Collecte des données… (laissez le temps avancer)", VKit.FS_SMALL)
