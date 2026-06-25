extends Control
## ProvinceDetail — le DÉTAIL graphique d'une province (read-only), bascule touche V
## (montre la province sélectionnée). Donne de l'AIR aux métriques que le panneau
## étroit de gauche ne peut que résumer : un graphe Easy Charts des FLUX (production
## + ressources, par source), les CAMEMBERTS culture & idéologie (VKit, réutilisés),
## et la barre des CLASSES. Charte bleu nuit / cuivre. Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const BARCHART = preload("res://addons/easy_charts/control_charts/BarChart/bar_chart.tscn")
const PW := 648.0
const PH := 520.0
const HEAD := 34.0

var _pid := -1
var _chart

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	_chart = BARCHART.instantiate()
	_chart.set_anchors_preset(Control.PRESET_TOP_LEFT)
	_chart.position = Vector2(16, 250)
	_chart.size = Vector2(PW - 32, PH - 250 - 16)
	_chart.custom_minimum_size = _chart.size
	_chart.visible = false
	add_child(_chart)
	Sim.ticked.connect(func(_y): if visible: _refresh())
	hide()

func _layout() -> void:
	var vp := get_viewport_rect().size
	position = Vector2((vp.x - PW) * 0.5, (vp.y - PH) * 0.5)

func show_province(pid: int) -> void:
	_pid = pid
	if visible:
		_refresh()
	queue_redraw()

func _notification(what: int) -> void:
	if what == NOTIFICATION_VISIBILITY_CHANGED and is_inside_tree() and visible:
		_refresh()
		queue_redraw()

# ── le graphe EC des flux (production + ressources, par source) ─────────────
func _refresh() -> void:
	if Sim.world == null or _pid < 0:
		if _chart != null:
			_chart.visible = false
		return
	var inc: Array = Sim.world.province_income(_pid)
	var x := []
	var y := []
	for l in inc:
		x.append(String(l["source"]))
		y.append(float(l["per_day"]) * 365.0)   # PAR AN (le par-jour n'est pas parlant)
	if x.size() >= 1:
		var cp = ChartProperties.new()
		cp.title = "Flux (production & ressources, /an)"
		cp.x_label = ""
		cp.y_label = "/an"
		cp.grid = true
		cp.points = false
		cp.origin = false       # un axe X catégoriel (chaînes) → pas d'origine numérique (évite remap(Nil))
		cp.x_scale = maxi(1, x.size())
		cp.y_scale = 5
		cp.bar_width = 18.0
		cp.background = false
		cp.colors.bounding_box = VKit.COL_PARCH
		cp.colors.grid = VKit.COL_EDGE
		cp.colors.functions = [VKit.COL_COPPER]
		_chart.visible = true
		_chart.plot(x, y, cp)
	else:
		_chart.visible = false
	queue_redraw()

func _draw() -> void:
	var w = Sim.world
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	if w == null or _pid < 0:
		return
	var info: Dictionary = w.province_info(_pid)
	if not bool(info.get("valide", false)):
		VKit.text(self, Vector2(16, HEAD), VKit.COL_DIM, "(aucune province sélectionnée)", VKit.FS_SMALL)
		return
	var x := 16.0
	UIKit.draw_icon(self, "capital_tower", Vector2(14, 8), 18)
	VKit.text(self, Vector2(40, 9), VKit.COL_COPPER, "Province — %s" % String(info["nom"]), VKit.FS_BIG)
	VKit.text(self, Vector2(PW - 96, 11), VKit.COL_DIM, "[V] fermer", VKit.FS_SMALL)
	VKit.text(self, Vector2(x, HEAD + 4), VKit.COL_PARCH,
		"%s habitants · %s · %s" % [_grp(info["ames"]), info["climat"], info["relief"]], VKit.FS_SMALL)

	# ── CAMEMBERTS culture & idéologie (VKit, réutilisés du panneau de gauche) ──
	var groups: Array = w.province_groups(_pid)
	var cy := HEAD + 64.0
	if groups.size() > 0:
		var cper := []
		var ccol := []
		for i in range(groups.size()):
			cper.append(groups[i]["percent"])
			ccol.append(VKit.SLICE_PAL[i % 8])
		var rnames := []
		var rper := []
		var rcol := []
		for g in groups:
			var idx: int = rnames.find(g["religion"])
			if idx < 0:
				rnames.append(g["religion"])
				rper.append(g["percent"])
				rcol.append(VKit.SLICE_PAL[(rnames.size() - 1) % 8])
			else:
				rper[idx] += g["percent"]
		var pr := 44.0
		VKit.pie(self, Vector2(x + pr + 20, cy), pr, cper, ccol)
		VKit.pie(self, Vector2(x + 3 * pr + 70, cy), pr, rper, rcol)
		VKit.text(self, Vector2(x + 12, cy + pr + 6), VKit.COL_DIM, "Culture", VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 2 * pr + 62, cy + pr + 6), VKit.COL_DIM, "Idéologie", VKit.FS_SMALL)
		# légende culture (noms + part)
		var lx := x + 4 * pr + 110
		var ly := cy - pr
		for i in range(mini(groups.size(), 6)):
			VKit.fill(self, Rect2(lx, ly + 3, 9, 9), VKit.SLICE_PAL[i % 8])
			VKit.text(self, Vector2(lx + 15, ly), VKit.COL_PARCH,
				"%s %d%%" % [String(groups[i]["culture"]), int(groups[i]["percent"])], VKit.FS_SMALL)
			ly += 17

	# ── POPULATION : barre empilée des classes ────────────────────────────────
	var cls: Dictionary = w.province_classes(_pid)
	var cp := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var tot: float = maxf(1.0, cp[0] + cp[1] + cp[2])
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	var py := HEAD + 64.0 + 44.0 + 30.0
	var rw := PW - 32.0
	var bh := 14.0
	var acc := 0.0
	for i in range(3):
		var segw: float = (rw - acc) if i == 2 else float(cp[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, py, segw, bh), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, py, rw, bh), VKit.COL_DIM)
	py += bh + 5
	for i in range(3):
		VKit.fill(self, Rect2(x + i * 200.0, py + 3, 9, 9), cc[i])
		VKit.text(self, Vector2(x + i * 200.0 + 15, py), VKit.COL_PARCH,
			"%s %s" % [cnames[i], _grp(cp[i])], VKit.FS_SMALL)

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
