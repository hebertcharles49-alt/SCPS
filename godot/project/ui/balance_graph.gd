extends Control
## BalanceGraph — la COURBE du solde mensuel (observation joueur KoH2, 2026-07-21) :
## 12 mois d'historique, min/max annotés, dates aux extrémités — LA TENDANCE se lit
## avant tout chiffre. Widget STATEFUL (l'historique) → PERSISTANT (règle #1,
## _ARCHITECTURE.md). Display-only : échantillonné au month_ticked par le panneau
## (déduplication par étiquette), jamais sérialisé — l'historique se reconstruit en
## jouant, c'est un instrument de bord, pas un état de simulation.

const ParchTheme = preload("res://ui/parch_theme.gd")
const MAX_POINTS := 12

var _pts: Array[Dictionary] = []   # [{label:String, net:float}]

func _ready() -> void:
	custom_minimum_size = Vector2(0, 64)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

## push dédupliqué : un point par mois (l'étiquette est la clé — les refresh
## d'ouverture/action ne créent jamais de doublon).
func push(label: String, net: float) -> void:
	if not _pts.is_empty() and String(_pts[-1]["label"]) == label:
		_pts[-1]["net"] = net
		queue_redraw()
		return
	_pts.append({"label": label, "net": net})
	while _pts.size() > MAX_POINTS:
		_pts.pop_front()
	queue_redraw()

func _draw() -> void:
	if _pts.size() < 2:
		return
	var w := size.x
	var h := size.y
	var pad_x := 8.0
	var pad_y := 12.0
	var lo := 1e30
	var hi := -1e30
	for p in _pts:
		lo = minf(lo, float(p["net"])); hi = maxf(hi, float(p["net"]))
	if hi - lo < 1.0:
		hi += 0.5; lo -= 0.5
	var n := _pts.size()
	var pts := PackedVector2Array()
	var imin := 0
	var imax := 0
	for i in range(n):
		var v := float(_pts[i]["net"])
		if v < float(_pts[imin]["net"]): imin = i
		if v > float(_pts[imax]["net"]): imax = i
		var x := pad_x + (w - 2.0 * pad_x) * float(i) / float(n - 1)
		var y := pad_y + (h - 2.0 * pad_y) * (1.0 - (v - lo) / (hi - lo))
		pts.push_back(Vector2(x, y))
	# la ligne de zéro (repère : au-dessus on gagne, dessous on saigne)
	if lo < 0.0 and hi > 0.0:
		var y0 := pad_y + (h - 2.0 * pad_y) * (1.0 - (0.0 - lo) / (hi - lo))
		draw_line(Vector2(pad_x, y0), Vector2(w - pad_x, y0), Color(ParchTheme.DIM_INK, 0.35), 1.0)
	# la courbe (encre, antialiasée)
	draw_polyline(pts, ParchTheme.INK, 1.6, true)
	# min/max annotés (rouge/vert), puis les extrémités datées avec leur net
	var f := get_theme_default_font()
	var fs := 10
	draw_string(f, pts[imax] + Vector2(-6, -3), "%+d" % int(round(float(_pts[imax]["net"]))),
		HORIZONTAL_ALIGNMENT_LEFT, -1, fs, ParchTheme.INCOME)
	draw_string(f, pts[imin] + Vector2(-6, 11), "%+d" % int(round(float(_pts[imin]["net"]))),
		HORIZONTAL_ALIGNMENT_LEFT, -1, fs, ParchTheme.EXPENSE)
	draw_string(f, Vector2(pad_x, h - 1), String(_pts[0]["label"]),
		HORIZONTAL_ALIGNMENT_LEFT, -1, fs, ParchTheme.DIM_INK)
	var last := String(_pts[-1]["label"])
	var lw := f.get_string_size(last, HORIZONTAL_ALIGNMENT_LEFT, -1, fs).x
	draw_string(f, Vector2(w - pad_x - lw, h - 1), last,
		HORIZONTAL_ALIGNMENT_LEFT, -1, fs, ParchTheme.DIM_INK)
