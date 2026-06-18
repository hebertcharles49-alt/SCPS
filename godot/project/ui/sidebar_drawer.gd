extends Control
## SidebarDrawer — le TIROIR de la sidebar : s'ouvre à droite du rail (même bande
## que le panneau de province, mutuellement exclusifs). En-tête à plaque + icône,
## puis le contenu de l'onglet. Portés (read-only) : Démographie, Stocks. Les
## autres affichent un cadre « à venir » (leur port viewer.c suit). Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const DX := 46.0
const DY := 102.0
const DW := 312.0

const TAB_ICON := ["menu_economy", "menu_demography", "menu_stocks", "menu_market",
	"menu_army", "menu_filters", "menu_diplomacy", "menu_council"]
const TAB_NAME := ["Économie", "Démographie", "Stocks", "Marché",
	"Armée", "Filtres", "Diplomatie", "Conseil"]

# Filtres : modes render_map offerts (culture/foi exigent des teintes → omis).
# [label, ViewMode]. Groupés comme viewer.c.
const FILT_GROUPS := [
	["Souveraineté", [["Politique", 1], ["Pays", 3], ["Régions", 2], ["Continents", 4]]],
	["Terre", [["Relief", 0], ["Altitude", 5], ["Fertilité", 6], ["Humidité", 7],
		["Température", 8], ["Ressources", 9], ["Habitabilité", 10]]],
]

var _tab := -1
var _map                       # MapView (pour Filtres → set_mode)
var _active_mode := 0
var _chips := []               # [{rect, mode}] cliquables (Filtres)

func setup(map) -> void:
	_map = map

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: queue_redraw())
	hide()

func _layout() -> void:
	position = Vector2(DX, DY)
	size = Vector2(DW, maxf(80.0, get_viewport_rect().size.y - DY - 26.0))

func show_tab(i: int) -> void:
	_tab = i
	visible = i >= 0
	queue_redraw()

func _draw() -> void:
	if _tab < 0:
		return
	VKit.panel_bg(self, Rect2(0, 0, DW, size.y))
	VKit.fill(self, Rect2(DW - 2, 0, 2, size.y), VKit.COL_COPPER)
	var x := 14.0
	var y := 10.0
	UIKit.draw_chrome(self, "panel_title_plaque", Rect2(8, 6, DW - 16, 30))
	UIKit.draw_icon(self, TAB_ICON[_tab], Vector2(x, y + 1), 22)
	VKit.text(self, Vector2(x + 28, y + 3), VKit.COL_COPPER, TAB_NAME[_tab], VKit.FS_BIG)
	y += 42
	var w = Sim.world
	if w == null:
		return
	var me: int = w.player()
	match _tab:
		1: _draw_demo(x, y, me)
		2: _draw_stocks(x, y, me)
		5: _draw_filtres(x, y)
		6: _draw_diplo(x, y, me)
		_: VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(panneau à venir — port viewer.c)")

# ── DÉMOGRAPHIE (sb_panel_demo, read-only) ─────────────────────────────────
func _draw_demo(x: float, y: float, me: int) -> void:
	var d: Dictionary = Sim.world.country_demo(me)
	var total: int = int(d["pop_total"])
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
		"population : %s · %d région(s)" % [_grp(total), int(d["n_regions"])])
	y += 24
	for cl in d["classes"]:
		var pct: int = 0 if total == 0 else int(round(100.0 * int(cl["pop"]) / total))
		UIKit.draw_icon(self, "population_group", Vector2(x, y), 14)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, String(cl["nom"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 110, y), VKit.COL_PARCH, "%s (%d%%)" % [_grp(cl["pop"]), pct], VKit.FS_SMALL)
		UIKit.bar(self, Rect2(x + 200, y, 84, 12), int(cl["satisfaction"]))
		y += 20

# ── STOCKS (sb_panel_stocks, read-only) ────────────────────────────────────
func _draw_stocks(x: float, y: float, me: int) -> void:
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien            stock   net/j   couv.", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		if y > size.y - 18:
			break
		var col := _marche_col(int(st["market_band"]))
		VKit.text(self, Vector2(x, y), col, String(st["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 110, y), col, _grp(st["stock"]), VKit.FS_SMALL)
		var net: float = st["net_day"]
		VKit.text(self, Vector2(x + 165, y), col, ("%+.1f" % net) if net != 0.0 else "0.0", VKit.FS_SMALL)
		var cov: int = int(st["coverage_days"])
		var covs := ("" if cov < 0 else (">1 an" if cov >= 366 else "%d j" % cov))
		VKit.text(self, Vector2(x + 225, y), col, covs, VKit.FS_SMALL)
		y += 15

# ── FILTRES (sb_panel_filtres) : sélecteur de mode carte, FONCTIONNEL ──────
func _draw_filtres(x: float, y: float) -> void:
	_chips.clear()
	if _map != null:
		_active_mode = _map.mode
	for grp in FILT_GROUPS:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, String(grp[0]), VKit.FS_SMALL)
		y += 16
		var cx := x
		for it in grp[1]:
			var label: String = it[0]
			var mode: int = it[1]
			var tw := VKit.text_w(label, VKit.FS_SMALL) + 14.0
			if cx + tw > DW - 12.0:
				cx = x; y += 22
			var active := (_active_mode == mode)
			var r := Rect2(cx, y, tw, 18)
			VKit.fill(self, r, VKit.COL_COPPER if active else VKit.COL_PANEL2)
			VKit.box(self, r, VKit.COL_EDGE)
			VKit.text(self, Vector2(cx + 7, y + 1), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
			_chips.append({"rect": r, "mode": mode})
			cx += tw + 4
		y += 26

# ── DIPLOMATIE (sb_panel_diplo, read-only) ─────────────────────────────────
func _draw_diplo(x: float, y: float, me: int) -> void:
	for rel in Sim.world.country_relations(me):
		if y > size.y - 18:
			break
		var col := VKit.sense(0.12) if bool(rel["at_war"]) else (VKit.sense(0.78) if bool(rel["allied"]) else VKit.COL_PARCH)
		VKit.text(self, Vector2(x, y), col, String(rel["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 158, y), VKit.COL_DIM, String(rel["status"]), VKit.FS_SMALL)
		y += 16

func _gui_input(event: InputEvent) -> void:
	if _tab == 5 and event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for ch in _chips:
			if ch.rect.has_point(event.position):
				_active_mode = ch.mode
				if _map != null:
					_map.set_mode(ch.mode)
				queue_redraw()
				accept_event()
				return

# couleur d'état de marché (BandMarche : mort · pénurie · tendu · sain · engorgé)
func _marche_col(band: int) -> Color:
	match band:
		1: return VKit.sense(0.10)   # pénurie : rouge
		2: return VKit.sense(0.40)   # tendu : ambre
		3: return VKit.sense(0.80)   # sain : vert
		4: return VKit.COL_COPPER    # engorgé : cuivre
		_: return VKit.COL_DIM       # mort

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
