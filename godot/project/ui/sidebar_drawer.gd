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
var _hover_zones := []         # [{rect, text}] survols (sprites de ressource → nom)
var _hover_text := ""
var _hover_pos := Vector2.ZERO

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
	_hover_text = ""
	visible = i >= 0
	queue_redraw()

func _draw() -> void:
	if _tab < 0:
		return
	_hover_zones.clear()
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
		0: _draw_eco(x, y, me)
		1: _draw_demo(x, y, me)
		2: _draw_stocks(x, y, me)
		3: _draw_marche(x, y, me)
		4: _draw_armee(x, y, me)
		5: _draw_filtres(x, y)
		6: _draw_diplo(x, y, me)
		7: _draw_conseil(x, y, me)
		_: VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(panneau à venir — port viewer.c)")

	# tooltip de survol (sprite de ressource → son nom)
	if _hover_text != "":
		var tw := VKit.text_w(_hover_text, VKit.FS_SMALL) + 12.0
		var tx := minf(_hover_pos.x + 12.0, DW - tw - 4.0)
		var ty := maxf(2.0, _hover_pos.y - 20.0)
		VKit.fill(self, Rect2(tx, ty, tw, 17), VKit.COL_PANEL2)
		VKit.box(self, Rect2(tx, ty, tw, 17), VKit.COL_COPPER)
		VKit.text(self, Vector2(tx + 6, ty + 1), VKit.COL_PARCH, _hover_text, VKit.FS_SMALL)

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
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          stock   net/j   couv.", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		if y > size.y - 18:
			break
		var col := _marche_col(int(st["market_band"]))
		_res_cell(x, y, int(st["res_id"]), String(st["name"]), col)
		VKit.text(self, Vector2(x + 110, y), col, _grp(st["stock"]), VKit.FS_SMALL)
		var net: float = st["net_day"]
		VKit.text(self, Vector2(x + 165, y), col, ("%+.1f" % net) if net != 0.0 else "0.0", VKit.FS_SMALL)
		var cov: int = int(st["coverage_days"])
		var covs := ("" if cov < 0 else (">1 an" if cov >= 366 else "%d j" % cov))
		VKit.text(self, Vector2(x + 225, y), col, covs, VKit.FS_SMALL)
		y += 18

## cellule d'identité d'une ressource : le SPRITE (assets/scps/pack/resources, par
## id), sinon le nom en texte ; survol → le nom dans tous les cas.
func _res_cell(x: float, y: float, res_id: int, name: String, col: Color) -> void:
	var spr := UIKit.resource_sprite(res_id, name)
	if spr != null:
		draw_texture_rect(spr, Rect2(x, y - 3, 18, 18), false)
	else:
		VKit.text(self, Vector2(x, y), col, name, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x - 2, y - 3, 104, 18), "text": name})

# ── ÉCONOMIE (sb_panel_eco, onglet Commerce, read-only) ────────────────────
func _draw_eco(x: float, y: float, me: int) -> void:
	var t: Dictionary = Sim.world.country_trade(me)
	UIKit.draw_icon(self, "menu_economy", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH,
		"%d route(s) · export %d or/an" % [int(t["routes"]), int(t["export_gold"])])
	y += 22
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "partenaires :", VKit.FS_SMALL)
	y += 16
	var partners: Array = t["partners"]
	if partners.is_empty():
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, "(aucun)", VKit.FS_SMALL)
		return
	for p in partners:
		if y > size.y - 18:
			break
		var col := VKit.sense(0.12) if bool(p["at_war"]) else (VKit.COL_COPPER if bool(p["embargo"]) else VKit.COL_PARCH)
		VKit.text(self, Vector2(x + 8, y), col, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), VKit.COL_DIM, "%d or/an" % int(p["value"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 228, y), col, String(p["status"]), VKit.FS_SMALL)
		y += 15

# ── MARCHÉ (sb_panel_marche, table des prix, read-only) ────────────────────
func _draw_marche(x: float, y: float, me: int) -> void:
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          prix(or)   marché", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		if y > size.y - 18:
			break
		var col := _marche_col(int(st["market_band"]))
		_res_cell(x, y, int(st["res_id"]), String(st["name"]), col)
		VKit.text(self, Vector2(x + 110, y), col, "%.2f" % float(st["price"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 178, y), VKit.COL_DIM, String(st["marche"]), VKit.FS_SMALL)
		y += 18

# ── CONSEIL (sb_panel_conseil, read-only) ──────────────────────────────────
func _draw_conseil(x: float, y: float, me: int) -> void:
	for seat in Sim.world.country_council(me):
		UIKit.draw_icon(self, "menu_council", Vector2(x, y - 1), 16)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_COPPER, String(seat["seat"]))
		y += 18
		if bool(seat["filled"]):
			VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH,
				"%s — tier %d" % [seat["councilor"], int(seat["tier"])], VKit.FS_SMALL)
		else:
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "(siège vacant)", VKit.FS_SMALL)
		y += 22

# ── ARMÉE (sb_panel_armee, read-only) ──────────────────────────────────────
func _draw_armee(x: float, y: float, me: int) -> void:
	var a: Dictionary = Sim.world.country_army(me)
	UIKit.draw_icon(self, "menu_army", Vector2(x, y - 1), 18)
	VKit.text(self, Vector2(x + 22, y), VKit.COL_PARCH, "force mobilisée : %d régiments" % int(a["regiments"]))
	y += 22
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "levée :")
	VKit.text(self, Vector2(x + 52, y), VKit.COL_COPPER, String(a["levy_name"]))
	y += 24
	var ar: Dictionary = Sim.world.army_info(me)
	if bool(ar.get("active", false)):
		VKit.text(self, Vector2(x, y), VKit.COL_COPPER,
			"armée de campagne — région %d · %s" % [int(ar["region"]), ar["phase"]], VKit.FS_SMALL)
		y += 16
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
			"inf %d · arch %d · cav %d · mages %d  (Σ %d)" % [
				int(ar["inf"]), int(ar["arch"]), int(ar["cav"]), int(ar["mages"]), int(ar["units"])], VKit.FS_SMALL)
		y += 20
	else:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(pas d'armée de campagne déployée)", VKit.FS_SMALL)
		y += 20
	UIKit.draw_icon(self, "harbor_anchor", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_DIM, "Flotte : %d coque(s)" % int(a["fleet"]))

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
	if event is InputEventMouseMotion:
		var h := ""
		for z in _hover_zones:
			if z.rect.has_point(event.position):
				h = z.text
				break
		if h != _hover_text:
			_hover_text = h
			_hover_pos = event.position
			queue_redraw()
		return
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
