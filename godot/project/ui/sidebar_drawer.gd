extends Control
## Le TIROIR de la sidebar : 8 onglets read-only (Économie/Démographie/Stocks/Marché/Armée/Filtres/Diplomatie/Conseil). Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")
const TooltipFactory = preload("res://ui/tooltip_factory.gd")   # formule fiche-de-bien partagée
const HoverZones = preload("res://ui/hover_zones.gd")               # stockage/hit-test de survol partagé
const DX := Frame.SIDEBAR_W + 10.0
const DY := Frame.TOPBAR_H + 10.0
const DW := 380.0

const TAB_ICON := ["menu_economy", "menu_demography", "menu_stocks", "menu_market",
	"menu_army", "menu_filters", "menu_diplomacy", "menu_council"]
const TAB_NAME := ["Économie", "Démographie", "Stocks", "Marché",
	"Armée", "Filtres", "Diplomatie", "Conseil"]

# [label, ViewMode] — modes render_map (culture/foi omis : exigent des teintes).
const FILT_GROUPS := [
	["Souveraineté", [["Politique", 1], ["Pays", 3], ["Régions", 2], ["Continents", 4]]],
	["Gouvernance", [["Stabilité", 13], ["Commerce", 14], ["Guerre", 15], ["Diplomatie", 16]]],
	["Terre", [["Relief", 0], ["Altitude", 5], ["Fertilité", 6], ["Humidité", 7],
		["Température", 8], ["Ressources", 9], ["Habitabilité", 10]]],
]

signal charts_requested        ## → ouvre le panneau Easy Charts
signal open_country(cid: int)  ## → la fenêtre d'actions du pays cliqué

var _tab := -1
var _map                       # MapView (pour Filtres → set_mode)
var _active_mode := 0
var _chips := []               # [{rect, mode}] cliquables (Filtres)
var _chart_btn := Rect2()      # bouton « Courbes dans le temps » (onglet Économie)
var _diplo_btns := []          # [{rect, act="open", target, nom}] fiches pays cliquables (onglet Diplomatie)
var _hover := HoverZones.new()   # zones de survol (stockage/hit-test partagés — ui/hover_zones.gd)
var _hover_text := ""
var _hover_pos := Vector2.ZERO
var _focus: Dictionary = {}       # contexte optionnel fourni par le routeur
var _eco_sliders := []             # multiplicateurs fiscaux/dépenses du panneau Économie
var _active_slider: Dictionary = {}
var _slider_preview := {}          # clé -> valeur en attente du prochain drain moteur

func setup(map) -> void:
	_map = map

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true   # SCROLL générique : le contenu défilé se coupe au bord du tiroir
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y):
		_slider_preview.clear()
		if visible: queue_redraw())
	hide()

var _hmax := 600.0   ## hauteur MAX (viewport) — la hauteur réelle épouse le contenu (latch _draw)

func _layout() -> void:
	position = Vector2(DX, DY)
	_hmax = maxf(80.0, get_viewport_rect().size.y - DY - 26.0)
	size = Vector2(DW, minf(size.y, _hmax))

func show_tab(i: int, context: Dictionary = {}) -> void:
	_tab = i
	_focus = context.duplicate(true)
	if i == 7 and String(context.get("section", "")) == "factions":
		_conseil_tab = 2
	_hover_text = ""
	_servile_manumit_armed = false   # jamais une confirmation qui traverse une fermeture d'onglet
	_marche_hover_res = -1           # le survol ne survit pas à un changement d'onglet (la sélection, si)
	if context.has("resource_id"):
		_marche_selected_res = int(context["resource_id"])
	visible = i >= 0
	queue_redraw()

const CONTENT_Y := 46.0   ## haut du CONTENU (sous l'en-tête fixe de 36 px + marge)

## SCROLL générique : offset par onglet, molette, clamp au contenu, en-tête fixe par-dessus.
var _scroll := {}         ## {tab: offset px}
var _maxscroll := 0.0     ## du DERNIER _draw (pour la molette)

func _draw_header(x: float) -> void:
	VKit.fill(self, Rect2(0, 0, DW, 36), VKit.COL_PANEL2)
	VKit.fill(self, Rect2(0, 0, 4.0, 36), VKit.COL_GOLD)
	VKit.fill(self, Rect2(4, 35, DW - 4, 1), VKit.COL_EDGE)
	UIKit.draw_icon(self, TAB_ICON[_tab], Vector2(x + 2, 5), 26)
	VKit.text(self, Vector2(x + 36, 7), VKit.COL_VALUE, TAB_NAME[_tab], VKit.FS_BIG)

func _draw() -> void:
	if _tab < 0:
		return
	_hover.clear()
	_tips.clear()
	VKit.panel_bg(self, Rect2(0, 0, DW, size.y))
	VKit.fill(self, Rect2(DW - 1, 3, 1, size.y - 3), VKit.COL_EDGE)
	var x := 14.0
	var w = Sim.world
	if w == null:
		_draw_header(x)
		return
	var me: int = w.player()
	var off := float(_scroll.get(_tab, 0.0))
	var y := CONTENT_Y - off
	var yend := y
	match _tab:
		0: yend = _draw_eco(x, y, me)
		1: yend = _draw_demo(x, y, me)
		2: yend = _draw_stocks(x, y, me)
		3: yend = _draw_marche(x, y, me)
		4: yend = _draw_armee(x, y, me)
		5: yend = _draw_filtres(x, y)
		6: yend = _draw_diplo(x, y, me)
		7: yend = _draw_conseil(x, y, me)
		_: VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(panneau à venir — port viewer.c)")
	var content_h := yend - y   # hauteur RÉELLE du contenu (indépendante de l'offset)

	# la fenêtre épouse le contenu (latch différé, borné au viewport ; au-delà ça défile).
	var want := clampf(CONTENT_Y + content_h + 12.0, 120.0, _hmax)
	if absf(want - size.y) > 0.5:
		set_deferred("size", Vector2(DW, want))
	var visible_h := want - CONTENT_Y - 12.0
	_maxscroll = maxf(0.0, content_h - visible_h)
	if off > _maxscroll:   # le contenu a rétréci sous l'offset mémorisé : re-clamp
		_scroll[_tab] = _maxscroll
		queue_redraw()

	_draw_header(x)
	if _maxscroll > 0.0:
		var track := Rect2(DW - 9.0, CONTENT_Y, 4.0, size.y - CONTENT_Y - 10.0)
		VKit.fill(self, track, VKit.COL_PANEL2)
		var thumb_h := maxf(24.0, track.size.y * visible_h / maxf(content_h, 1.0))
		var thumb_y := track.position.y + (minf(off, _maxscroll) / _maxscroll) * (track.size.y - thumb_h)
		VKit.fill(self, Rect2(track.position.x, thumb_y, 4.0, thumb_h), VKit.COL_GOLD)

	# zones défilées sous l'en-tête fixe écartées (sinon un fantôme répondrait au survol dans le bandeau de titre).
	_tips.append_array(_hover.to_tips(36.0))

func _draw_demo(x: float, y: float, me: int) -> float:
	var d: Dictionary = Sim.world.country_demo(me)
	var total: int = int(d["pop_total"])
	var demo_val_w: float = VKit.value(self, Vector2(x, y), "population : %s" % _grp(total))
	VKit.detail(self, Vector2(x + demo_val_w, y), " · %d région(s)" % int(d["n_regions"]), VKit.FS)
	y += 24
	var row_i := 0
	for cl in d["classes"]:
		VKit.list_row_bg(self, Rect2(x - 4, y - 2, DW - 2.0 * x + 8, 19), row_i)
		var pct: int = 0 if total == 0 else int(round(100.0 * int(cl["pop"]) / total))
		UIKit.draw_icon(self, "population_group", Vector2(x, y), 16)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, String(cl["nom"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 110, y), VKit.COL_PARCH, "%s (%d%%)" % [_grp(cl["pop"]), pct], VKit.FS_SMALL)
		UIKit.bar(self, Rect2(x + 200, y, 84, 12), int(cl["satisfaction"]))
		y += 20
		row_i += 1
	return y

func _draw_stocks(x: float, y: float, me: int) -> float:
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          stock   net/j   couv.", VKit.FS_SMALL)
	y += 16
	var row_i := 0
	for st in Sim.world.country_stocks(me):
		var stock_row := Rect2(x - 4, y - 2, DW - 2.0 * x + 8, 18)
		VKit.list_row_bg(self, stock_row, row_i)
		if int(_focus.get("resource_id", -1)) == int(st["res_id"]):
			VKit.box(self, stock_row, VKit.COL_GOLD)
		_hover.add(stock_row, _stock_tip(st), _stock_info_card(st))
		var col := _marche_col(int(st["market_band"]))
		_res_cell(x, y, int(st["res_id"]), String(st["name"]), col)
		VKit.text(self, Vector2(x + 110, y), col, _grp(st["stock"]), VKit.FS_SMALL)
		var net: float = st["net_day"]
		VKit.text(self, Vector2(x + 165, y), col, ("%+.1f" % net) if net != 0.0 else "0.0", VKit.FS_SMALL)
		var cov: int = int(st["coverage_days"])
		var covs := ("" if cov < 0 else (">1 an" if cov >= 366 else "%d j" % cov))
		VKit.text(self, Vector2(x + 225, y), col, covs, VKit.FS_SMALL)
		y += 18
		row_i += 1
	return y

## les 4 formules vivent dans ui/tooltip_factory.gd (testées par tests/stock_info_card_test.gd) — ici : délégations pures.
func _stock_tip(st: Dictionary) -> String:
	return TooltipFactory.stock_tip(st)

func _stock_info_card(st: Dictionary) -> Dictionary:
	return TooltipFactory.stock_info_card(Sim.world, st)

func _stock_territory_detail(st: Dictionary) -> Dictionary:
	return TooltipFactory.territory_detail(Sim.world, st)

func _market_info_card(st: Dictionary, quote: Dictionary = {}) -> Dictionary:
	return TooltipFactory.market_info_card(Sim.world, st, quote,
		_marche_category_word(int(st.get("res_id", -1))))

## identité d'une ressource : le SPRITE (par id), sinon le nom en texte ; survol → le nom.
func _res_cell(x: float, y: float, res_id: int, name: String, col: Color) -> void:
	var spr := UIKit.resource_sprite(res_id, name)
	if spr != null:
		draw_texture_rect(spr, Rect2(x, y - 3, 18, 18), false)
	else:
		VKit.text(self, Vector2(x, y), col, name, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x - 2, y - 3, 104, 18), "text": name})

const _MAT_RAWS := [9, 24, 25, 13, 36]   # RES_WOOD · RES_CLAY · RES_STONE · RES_IRON · RES_ARMS
const _MAT_NAMES := {9: "bois", 24: "argile", 25: "pierre", 13: "fer", 36: "armes"}

func _slider_key(data: Dictionary) -> String:
	return "%s:%d:%d" % [String(data.get("kind", "eco")), int(data.get("family", -1)),
		int(data.get("index", data.get("seat", -1)))]

func _draw_multiplier_slider(x: float, y: float, label: String, current: float,
		zones: Array, data: Dictionary, tip: String, live: String = "") -> float:
	## `live` (opt.) : valeur en direct à la place du % brut. Curseur LINÉARISÉ 0–100 % (0.02..1.0).
	var key := _slider_key(data)
	var value := float(_slider_preview.get(key, current))
	value = clampf(value, 0.02, 1.0)
	var row := Rect2(x - 3.0, y - 2.0, DW - 2.0 * x + 6.0, 22.0)
	VKit.list_row_bg(self, row, zones.size())
	VKit.text(self, Vector2(x + 4.0, y + 3.0), VKit.COL_PARCH, label, VKit.FS_SMALL)
	var track := Rect2(x + 132.0, y + 5.0, 156.0, 8.0)
	VKit.fill(self, track, Color("caa768"))
	VKit.box(self, track.grow(1.0), VKit.COL_EDGE)
	var frac := (value - 0.02) / 0.98
	VKit.fill(self, Rect2(track.position, Vector2(track.size.x * frac, track.size.y)), VKit.COL_GOLD)
	var kx := track.position.x + track.size.x * frac
	draw_circle(Vector2(kx, track.get_center().y), 5.0, VKit.COL_PARCH)
	var pct := "%d %%" % int(round(frac * 100.0))
	if live != "":
		# right-aligné au bord du tiroir (les montants sont plus larges que « 60 % »)
		var lw := VKit.text_w(live, VKit.FS_SMALL)
		VKit.value(self, Vector2(minf(x + 296.0, DW - 8.0 - lw), y + 2.0), live, VKit.FS_SMALL)
	else:
		VKit.value(self, Vector2(x + 300.0, y + 2.0), pct, VKit.FS_SMALL)
	var z := data.duplicate(true)
	z["rect"] = Rect2(track.position.x - 7.0, y - 1.0, track.size.x + 14.0, 20.0)
	z["track"] = track
	z["value"] = value
	zones.append(z)
	var cur_txt := pct if live == "" else "%s  (%s)" % [live, pct]
	_hover.add(row, "%s\n• 0 à 100 %%\n• Actuellement : %s" % [tip, cur_txt])
	return y + 24.0

func _draw_budget_controls(x: float, y: float, me: int) -> float:
	_eco_sliders.clear()
	if not Sim.world.has_method("budget_controls"):
		return y
	var ctl: Dictionary = Sim.world.budget_controls(me)
	var doy := maxi(1, int(Sim.world.day_of_year())) if Sim.world.has_method("day_of_year") else 1
	var mf := 30.0 / float(doy)
	var flux := {}
	if Sim.world.has_method("country_budget"):
		for p in Sim.world.country_budget(me):
			flux[String(p.get("name", ""))] = float(p.get("amount", 0.0)) * mf   # or/mois signé
	y = VKit.section(self, x, y, "Pilotage budgétaire")
	# rendement fiscal agrégé (le détail par classe n'est pas exposé par la façade)
	var tax_month: float = absf(float(flux.get("taxes", 0.0)))
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD,
		"Fiscalité par classe · rendement %s or/mois" % _grp(int(round(tax_month))), VKit.FS_SMALL)
	y += 18.0
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls_idx := int(row.get("id", 0))
		# rendement réel de la classe en or/mois (à la place du %)
		var tax_live := ""
		if Sim.world.has_method("tax_class_month"):
			tax_live = "%s or/mois" % _grp(int(round(float(Sim.world.tax_class_month(cls_idx)))))
		y = _draw_multiplier_slider(x, y, String(row.get("name", "Impôt")), float(row.get("mult", 1.0)),
			_eco_sliders, {"kind": "eco", "family": 0, "index": cls_idx},
			"Taux visé de cette classe. Monter accroît l'évasion et la grogne au-delà de sa tolérance.", tax_live)
	y += 3.0
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Dépenses", VKit.FS_SMALL)
	y += 18.0
	var spend_tips := [
		"Finance le capital institutionnel K (0 à +10 %). Coûte une part du revenu chaque mois.",
		"Finance l'infrastructure. Sous 100 %, les bâtiments passent en friche.",
		"Finance les soldes. Sous 100 %, l'armée perd le moral au combat (elle ne déserte plus).",
		"Finance les coques. Sous 100 %, la flotte perd le moral au combat (plus de délabrement).",
		"Finance la connectivité (routes) : −20 % (sous-financé) à +10 % de prospérité/commerce. Coûte une part du revenu chaque mois ; au minimum, aucune dépense mais −20 % de connectivité.",
		"MONNAIE : frappe la réserve métallique (redevance minière) en monnaie, au prix courant du métal. Ne coûte rien au trésor — mais épuise la réserve.",
	]
	# poste réalisé (or/mois) de chaque enveloppe
	var spend_flux := ["invest.", "entretien", "soldes", "marine", "routes"]
	for raw in ctl.get("spending", []):
		var row: Dictionary = raw
		var idx := int(row.get("id", 0))
		var mult := float(row.get("mult", 1.0))
		var live := ""
		if idx == 5 and Sim.world.has_method("country_mint_month"):
			live = "+%s or/mois" % _grp(int(round(float(Sim.world.country_mint_month(me)))))
		elif idx >= 0 and idx < spend_flux.size():
			live = "%s or/mois" % _grp(int(round(absf(float(flux.get(spend_flux[idx], 0.0))))))
		var tip: String = spend_tips[idx] if idx >= 0 and idx < spend_tips.size() else "Enveloppe budgétaire nationale."
		if idx == 0:
			tip += "\n• Effet actuel : +%.0f %% K" % (clampf(mult, 0.0, 1.0) * 10.0)
		elif idx == 4:
			var rfrac := clampf((mult - 0.02) / 0.98, 0.0, 1.0)
			tip += "\n• Effet actuel : %+.0f %% de connectivité" % (-20.0 + rfrac * 30.0)
		y = _draw_multiplier_slider(x, y, String(row.get("name", "Dépense")), mult,
			_eco_sliders, {"kind": "eco", "family": 1, "index": idx}, tip, live)
	return y + 3.0

func _draw_mat_line(x: float, y: float, me: int) -> float:
	if not Sim.world.has_method("country_stocks"):
		return y
	var smap := {}
	for st in Sim.world.country_stocks(me):
		smap[int(st["res_id"])] = int(st["stock"])
	var parts := []
	for rid in _MAT_RAWS:
		parts.append("%s %s" % [String(_MAT_NAMES[rid]), _grp(smap.get(rid, 0))])
	var line := "Matières : " + " · ".join(parts)
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, line, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x - 2, y - 3, VKit.text_w(line, VKit.FS_SMALL) + 8, 16),
		"text": "Stocks nationaux de matières brutes — détail (net/jour, couverture) dans l'onglet Stocks"})
	return y + 18.0

func _draw_eco(x: float, y: float, me: int) -> float:
	y = _draw_mat_line(x, y, me)
	_chart_btn = Rect2(x, y, DW - 2.0 * x, 20.0)
	VKit.fill(self, _chart_btn, VKit.COL_PANEL2)
	VKit.box(self, _chart_btn, VKit.COL_GOLD)
	UIKit.draw_icon(self, "menu_economy", Vector2(x + 4, y + 2), 16)
	VKit.text(self, Vector2(x + 24, y + 3), VKit.COL_GOLD, "Courbes dans le temps  ▸", VKit.FS_SMALL)
	y += 28
	y = _draw_budget_controls(x, y, me)
	var b: Dictionary = Sim.world.budget_summary(me)
	UIKit.draw_icon(self, "gold_coin", Vector2(x, y - 1), 16)
	VKit.value(self, Vector2(x + 20, y), "Trésor : %s or" % _grp(b["gold"]))
	y += 18
	var doy := maxi(1, int(Sim.world.day_of_year())) if Sim.world.has_method("day_of_year") else 1
	var month_factor := 30.0 / float(doy)
	var income_month := float(b.get("monthly_income", float(b["income"]) * month_factor))
	var expense_month := float(b.get("monthly_expense", float(b["expense"]) * month_factor))
	var net: float = float(b.get("monthly_net", float(b["net"]) * month_factor))
	var ncol := VKit.sense(0.80) if net >= 0 else VKit.sense(0.12)
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Solde mensuel", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 92, y), ncol,
		"%s%s or/mois" % ["+" if net >= 0 else "−", _grp(absf(net))], VKit.FS_SMALL)
	y += 16
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "crédit : %s or" % _grp(b["credit_line"]), VKit.FS_SMALL)
	if int(b.get("creditor", -1)) >= 0:
		VKit.text(self, Vector2(x + 140, y), VKit.sense(0.30), "dette → %s" % String(b.get("creditor_name", "")), VKit.FS_SMALL)
	y += 22
	var projection := float(b.get("projected_year_end", b.get("gold", 0.0)))
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "fin d'année au rythme actuel", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 210, y), VKit.sense(0.80) if projection >= 0.0 else VKit.sense(0.12),
		"%s or" % _grp(projection), VKit.FS_SMALL)
	y += 16
	var runway := float(b.get("runway_months", -1.0))
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "autonomie trésor + crédit", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 210, y), VKit.COL_DIM if runway < 0.0 else (VKit.sense(0.12) if runway < 6.0 else VKit.COL_PARCH),
		"stable" if runway < 0.0 else "%.1f mois" % runway, VKit.FS_SMALL)
	y += 22
	var revenues := []
	var expenses := []
	for p in Sim.world.country_budget(me):
		var monthly := float(p["amount"]) * month_factor
		if monthly >= 0.0:
			revenues.append({"name": String(p["name"]), "amount": monthly})
		else:
			expenses.append({"name": String(p["name"]), "amount": monthly})
	VKit.fill(self, Rect2(x, y - 3, DW - 2.0 * x, 20), Color(0.08, 0.14, 0.12, 0.92))
	VKit.text(self, Vector2(x + 6, y), VKit.sense(0.80), "Revenus", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 210, y), VKit.sense(0.80), "+%s/mois" % _grp(income_month), VKit.FS_SMALL)
	y += 20
	var shown := 0
	for p in revenues:
		VKit.list_row_bg(self, Rect2(x + 4, y - 1, DW - 2.0 * x - 8, 16), shown)
		VKit.text(self, Vector2(x + 8, y), VKit.COL_PARCH, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 210, y), VKit.sense(0.78), "+%s" % _grp(p["amount"]), VKit.FS_SMALL)
		y += 16
		shown += 1
	VKit.fill(self, Rect2(x, y - 3, DW - 2.0 * x, 20), Color(0.16, 0.08, 0.08, 0.92))
	VKit.text(self, Vector2(x + 6, y), VKit.sense(0.18), "Dépenses", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 210, y), VKit.sense(0.18), "−%s/mois" % _grp(expense_month), VKit.FS_SMALL)
	y += 20
	shown = 0
	for p in expenses:
		VKit.list_row_bg(self, Rect2(x + 4, y - 1, DW - 2.0 * x - 8, 16), shown)
		VKit.text(self, Vector2(x + 8, y), VKit.COL_PARCH, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 210, y), VKit.sense(0.18), "−%s" % _grp(absf(float(p["amount"]))), VKit.FS_SMALL)
		y += 16
		shown += 1
	y += 4
	VKit.fill(self, Rect2(x, y, DW - 2.0 * x, 1), VKit.COL_EDGE)
	y += 8
	var t: Dictionary = Sim.world.country_trade(me)
	UIKit.draw_icon(self, "menu_economy", Vector2(x, y - 1), 16)
	VKit.value(self, Vector2(x + 20, y),
		"%d route(s) · export %d or/mois" % [int(t["routes"]), int(round(float(t["export_gold"]) / 12.0))])
	y += 20
	var partners: Array = t["partners"]
	if partners.is_empty():
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, "(aucun partenaire)", VKit.FS_SMALL)
		return y + 16.0
	for p in partners:
		var col := VKit.sense(0.12) if bool(p["at_war"]) else (VKit.COL_GOLD if bool(p["embargo"]) else VKit.COL_PARCH)
		VKit.text(self, Vector2(x + 8, y), col, String(p["name"]), VKit.FS_SMALL)
		VKit.value(self, Vector2(x + 150, y), "%d or/mois" % int(round(float(p["value"]) / 12.0)), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 228, y), col, String(p["status"]), VKit.FS_SMALL)
		y += 15
	return y

# MARCHÉ : Acheter/Vendre MARCHE_QTY sur la région-capitale (verbes player_market_buy/_sell, journalisés).
# Tri par prix/pénurie/catégorie : état persistant PAR SESSION (variable d'instance, jamais sur disque).
var _marche_btns := []       # [{rect, act, res_id}] boutons Acheter/Vendre
var _marche_rows := []       # [{rect, res_id}] la ligne ENTIÈRE (survol + sélection)
var _marche_sort_btns := []  # [{rect, key}] les 3 chips de tri
var _marche_flash := ""
var _marche_flash_ok := true
var _marche_hover_res := -1     # ligne sous la SOURIS (motion) — remis à -1 au changement d'onglet
var _marche_selected_res := -1  # dernière ligne CLIQUÉE — persiste (comme le tri)
var _marche_sort_key := ""      # "" (ordre moteur) · "prix" · "penurie" · "categorie"
var _marche_sort_dir := 1       # 1 = croissant · -1 = décroissant (reclic sur le chip actif)
const MARCHE_QTY := 10
## frontière brute/manufacturée du MOTEUR (RES_PROD_FIRST, scps/scps_types.h) : RES_NONE=0 → RES_STONE inclus = 26 entrées.
const MARCHE_CAT_SPLIT := 26

func _marche_category_word(res_id: int) -> String:
	return "brute" if res_id < MARCHE_CAT_SPLIT else "manufacturée"

## texte tronqué à une largeur max (le nom complet reste dans l'infobulle native).
func _fit_text(s: String, max_w: float, fs: int) -> String:
	if VKit.text_w(s, fs) <= max_w:
		return s
	var out := s
	while out.length() > 1 and VKit.text_w(out + "…", fs) > max_w:
		out = out.substr(0, out.length() - 1)
	return out + "…"

## tri STABLE (départage par res_id) — AFFICHAGE seulement ; les verbes restent adressés par res_id.
func _marche_sorted(me: int) -> Array:
	var arr: Array = Sim.world.country_stocks(me).duplicate()
	if _marche_sort_key == "":
		return arr
	var key := _marche_sort_key
	var dir := _marche_sort_dir
	var cat_split := MARCHE_CAT_SPLIT
	arr.sort_custom(func(a, b):
		var va := 0.0
		var vb := 0.0
		match key:
			"prix":
				va = float(a["price"]); vb = float(b["price"])
			"penurie":
				va = float(a["market_band"]); vb = float(b["market_band"])
			"categorie":
				va = 0.0 if int(a["res_id"]) < cat_split else 1.0
				vb = 0.0 if int(b["res_id"]) < cat_split else 1.0
		if va == vb:
			return int(a["res_id"]) < int(b["res_id"])
		return (va < vb) if dir > 0 else (va > vb))
	return arr

func _marche_sort_act(key: String) -> void:
	if _marche_sort_key == key:
		_marche_sort_dir = -_marche_sort_dir
	else:
		_marche_sort_key = key
		_marche_sort_dir = 1
	Sound.play("ui_click")
	queue_redraw()

func _draw_marche(x: float, y: float, me: int) -> float:
	_marche_btns.clear()
	_marche_rows.clear()
	_marche_sort_btns.clear()
	var cap_region := -1
	var cap_prov: int = Sim.world.country_capital_province(me)
	if cap_prov >= 0:
		cap_region = Sim.world.province_region(cap_prov)
	# PUISSANCE COMMERCIALE : volume achetable au marché ce mois-ci (borne les achats).
	var cpow: Dictionary = Sim.world.commerce_power(me)
	var cp_pool := float(cpow.get("pool", 0.0))
	var cp_rem := float(cpow.get("remaining", 0.0))
	var cp_bonus := int(cpow.get("bonus_pct", 0))
	var cp_col := VKit.sense(clampf(cp_rem / maxf(cp_pool, 1.0), 0.0, 1.0))   # vert plein → rouge à sec
	var cp_lbl := "Puissance comm. : %d / %d ce mois" % [int(round(cp_rem)), int(round(cp_pool))]
	VKit.text(self, Vector2(x, y), cp_col, cp_lbl, VKit.FS_SMALL)
	var cp_tip := "Volume de biens achetable au marché ce mois-ci (0.04/bourgeois + 0.01/élite × la chaîne commerciale)."
	if cp_bonus > 0:
		cp_tip += "\nDont +%d %% apportés par vos édifices de commerce." % cp_bonus
	_hover.add_dict({"rect": Rect2(x - 2, y - 3, 264, 16), "text": cp_tip})
	y += 20

	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Trier :", VKit.FS_SMALL)
	var scx := x + VKit.text_w("Trier :", VKit.FS_SMALL) + 8.0
	for it in [["prix", "Prix"], ["penurie", "Pénurie"], ["categorie", "Catégorie"]]:
		var key: String = it[0]
		var lbl: String = it[1]
		var active := (_marche_sort_key == key)
		var disp := (lbl + (" ▲" if _marche_sort_dir > 0 else " ▼")) if active else lbl
		var tw := VKit.text_w(disp, VKit.FS_SMALL) + 12.0
		var r := Rect2(scx, y - 2, tw, 17)
		VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		VKit.text(self, Vector2(scx + 6, y - 1), VKit.COL_PANEL if active else VKit.COL_PARCH, disp, VKit.FS_SMALL)
		_marche_sort_btns.append({"rect": r, "key": key})
		scx += tw + 6.0
	y += 22

	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien              prix          état                actions", VKit.FS_SMALL)
	y += 16
	if cap_region < 0:
		VKit.text(self, Vector2(x, y), VKit.sense(0.30), "Aucune capitale connue — achats/ventes indisponibles.", VKit.FS_SMALL)
		y += 16

	for st in _marche_sorted(me):
		var row_y0 := y
		var band := int(st["market_band"])
		var col := _marche_col(band)
		var res_id := int(st["res_id"])
		var name := String(st["name"])
		var is_active := (res_id == _marche_hover_res) or (res_id == _marche_selected_res)
		var spr := UIKit.resource_sprite(res_id, name)
		if spr != null:
			draw_texture_rect(spr, Rect2(x, y - 3, 18, 18), false)
			if is_active:
				VKit.text(self, Vector2(x + 22, y), col, _fit_text(name, 108.0, VKit.FS_SMALL), VKit.FS_SMALL)
		else:
			VKit.text(self, Vector2(x, y), col, _fit_text(name, 122.0, VKit.FS_SMALL), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 140, y), col, "%.2f or" % float(st["price"]), VKit.FS_SMALL)
		# état du marché : couleur + MOT (jamais la couleur seule — le mot vient du moteur).
		VKit.text(self, Vector2(x + 212, y), col, String(st["marche"]), VKit.FS_SMALL)
		y += 18

		var can_trade := cap_region >= 0
		var ax := x + 8.0
		var lab_b := "Acheter %d" % MARCHE_QTY
		var bw := maxf(32.0, VKit.text_w(lab_b, VKit.FS_SMALL) + 14.0)
		var rb := Rect2(ax, y, bw, 20.0)
		VKit.fill(self, rb, VKit.COL_PANEL2)
		VKit.box(self, rb, VKit.sense(0.80) if can_trade else VKit.COL_EDGE)
		VKit.text(self, Vector2(rb.position.x + 7, y + 2), VKit.sense(0.80) if can_trade else VKit.COL_DIM, lab_b, VKit.FS_SMALL)
		if can_trade:
			_marche_btns.append({"rect": rb, "act": "buy", "res_id": res_id})
		else:
			_hover.add(rb, "Aucune capitale connue — achat indisponible.")
		var lab_s := "Vendre %d" % MARCHE_QTY
		var sw := maxf(32.0, VKit.text_w(lab_s, VKit.FS_SMALL) + 14.0)
		var rs := Rect2(ax + bw + 8.0, y, sw, 20.0)
		VKit.fill(self, rs, VKit.COL_PANEL2)
		VKit.box(self, rs, VKit.sense(0.12) if can_trade else VKit.COL_EDGE)
		VKit.text(self, Vector2(rs.position.x + 7, y + 2), VKit.sense(0.12) if can_trade else VKit.COL_DIM, lab_s, VKit.FS_SMALL)
		if can_trade:
			_marche_btns.append({"rect": rs, "act": "sell", "res_id": res_id})
		else:
			_hover.add(rs, "Aucune capitale connue — vente indisponible.")
		y += 24

		var row_rect := Rect2(x - 4.0, row_y0 - 3.0, DW - 2.0 * x + 8.0, y - row_y0 - 2.0)
		_marche_rows.append({"rect": row_rect, "res_id": res_id})
		var tip := "%s — %s — %s en stock (%.2f or, %s)" % [
			name, _marche_category_word(res_id), _grp(int(st["stock"])), float(st["price"]), String(st["marche"])]
		var quote: Dictionary = Sim.world.market_quote(me, res_id, MARCHE_QTY) \
			if Sim.world.has_method("market_quote") else {}
		_hover.add(row_rect, tip, _market_info_card(st, quote))
		y += 3
		VKit.fill(self, Rect2(x, y, DW - 2.0 * x, 1), VKit.COL_EDGE)
		y += 5

	if _marche_flash != "":
		y += 4
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _marche_flash_ok else VKit.sense(0.10)), _marche_flash, VKit.FS_SMALL)
		y += 16
	return y

# CONSEIL : Recruter/Renvoyer par siège (verbes player_council_hire/_dismiss, journalisés).
var _conseil_btns := []   # [{rect, act, seat}] boutons Recruter/Renvoyer
var _conseil_flash := ""
var _conseil_flash_ok := true
var _conseil_tab := 0   ## 0 = Gouvernement · 1 = Politiques · 2 = Factions
var _ctab_btns := []
## assiette des coûts % (revenu fiscal annuel + IPM, rafraîchis à chaque _draw_conseil).
var _cons_rev := 0.0
var _cons_ipm := 1.0

func _council_seat_info_card(seat: Dictionary) -> Dictionary:
	var loyalty := int(seat.get("loyalty", 0))
	var target := int(seat.get("loyalty_target", loyalty))
	return {
		"title": "%s %s" % [String(seat.get("firstname", "")), String(seat.get("house", ""))],
		"state": "%s · rang %d · %s" % [String(seat.get("seat", "Conseil")), int(seat.get("tier", 0)), String(seat.get("faction", ""))],
		"trend": "loyauté %d → cible %d" % [loyalty, target],
		"trend_tone": "positive" if target >= loyalty else "negative",
		"lines": [
			{"label": "Bonus de rang", "value": "+%.1f %%" % float(seat.get("rank_bonus_pct", 0.0))},
			{"label": "Efficacité", "value": "%.1f %%" % float(seat.get("efficiency_pct", 0.0))},
			{"label": "Administration", "value": "+%.1f points" % float(seat.get("eff_admin_points", 0.0)), "tone": "positive"},
			{"label": "Loyauté", "value": "+%.1f points" % float(seat.get("eff_loyalty_points", 0.0)), "tone": "positive"},
			{"label": "Corruption", "value": "−%.1f points" % float(seat.get("eff_corruption_points", 0.0)), "tone": "negative"},
			{"label": "Effet net", "value": "+%.1f %% %s" % [float(seat.get("final_bonus_pct", 0.0)), String(seat.get("domain", ""))], "tone": "positive"},
			{"label": "Traitement", "value": "%s or / mois" % _grp(int(round(float(seat.get("cost_year", 0.0)) / 12.0 * float(seat.get("pay", 1.0)))))},
		],
	}

func _council_candidate_info_card(cand: Dictionary) -> Dictionary:
	return {
		"title": "%s %s" % [String(cand.get("firstname", "")), String(cand.get("house", ""))],
		"state": "candidat · rang %d · %s" % [int(cand.get("tier", 0)), String(cand.get("faction", ""))],
		"trend": "loyauté de départ %d" % int(cand.get("predicted_loyalty", 0)),
		"trend_tone": "positive" if int(cand.get("predicted_loyalty", 0)) >= 50 else "negative",
		"lines": [
			{"label": "Efficacité prévue", "value": "%.1f %%" % float(cand.get("efficiency_pct", 0.0))},
			{"label": "Administration", "value": "+%.1f points" % float(cand.get("eff_admin_points", 0.0)), "tone": "positive"},
			{"label": "Loyauté", "value": "+%.1f points" % float(cand.get("eff_loyalty_points", 0.0)), "tone": "positive"},
			{"label": "Corruption", "value": "−%.1f points" % float(cand.get("eff_corruption_points", 0.0)), "tone": "negative"},
			{"label": "Effet net", "value": "+%.1f %% %s" % [float(cand.get("final_bonus_pct", 0.0)), String(cand.get("domain", ""))], "tone": "positive"},
			{"label": "Traitement", "value": "%s or / mois" % _grp(int(round(float(cand.get("cost_year", 0.0)) / 12.0)))},
		],
	}

func _faction_info_card(fe: Dictionary, coup: int, corruption: int) -> Dictionary:
	var delta := int(fe.get("policy_delta", 0))
	var lines: Array = [
		{"label": "Assise sociale", "value": "%d %%" % int(fe.get("base_part", 0))},
		{"label": "Effet des politiques", "value": "%+d points" % delta,
			"tone": "positive" if delta > 0 else ("negative" if delta < 0 else "dim")},
		{"label": "Rancœur", "value": "%d / 100" % int(fe.get("grief", 0)),
			"tone": "negative" if int(fe.get("grief", 0)) >= 40 else "dim"},
		{"label": "Pression de coup", "value": "%d / 100" % int(fe.get("coup_pressure", 0)),
			"tone": "negative" if bool(fe.get("coup_driver", false)) else "dim"},
	]
	if bool(fe.get("captor", false)):
		lines.append({"label": "Capture de l'État", "value": "faction la plus favorisée", "tone": "negative"})
	return {
		"title": String(fe.get("name", "Faction")),
		"state": "%d %% de soutien%s" % [int(fe.get("part", 0)), " · dominante" if bool(fe.get("dominant", false)) else ""],
		"trend": "risque national %d / 100" % coup,
		"trend_tone": "negative" if coup >= 40 else "dim",
		"lines": lines + [{"label": "Corruption nationale", "value": "%d / 100" % corruption}],
	}

func _draw_factions(x: float, y: float, me: int) -> float:
	var fx: Dictionary = Sim.world.country_factions(me) if Sim.world.has_method("country_factions") else {}
	var coup := int(fx.get("coup", 0))
	var corruption := int(fx.get("corruption", 0))
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Rapport de forces", VKit.FS_BIG)
	y += 22
	var summary := "Tension de coup %d / 100 · Corruption %d / 100" % [coup, corruption]
	VKit.text(self, Vector2(x, y), VKit.sense(0.18 if coup >= 40 else 0.62), summary, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x - 2, y - 2, VKit.text_w(summary, VKit.FS_SMALL) + 6, 16),
		"text": "Le risque vient de la faction signalée ci-dessous ; la corruption mesure la capture cumulée de l'État."})
	y += 22
	var row_i := 0
	for raw in fx.get("list", []):
		var fe: Dictionary = raw
		var row := Rect2(x - 4, y - 3, DW - 2.0 * x + 8, 42)
		VKit.list_row_bg(self, row, row_i)
		if bool(fe.get("coup_driver", false)):
			VKit.fill(self, Rect2(row.position, Vector2(3, row.size.y)), VKit.sense(0.15))
		var name := String(fe.get("name", "Faction"))
		var suffix := " ★" if bool(fe.get("dominant", false)) else ""
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH, name + suffix, VKit.FS_SMALL)
		VKit.text(self, Vector2(DW - x - 42, y), VKit.COL_GOLD, "%d %%" % int(fe.get("part", 0)), VKit.FS_SMALL)
		y += 15
		VKit.gauge(self, x, y, DW - 2.0 * x, 7.0, int(fe.get("part", 0)))
		y += 11
		var detail := "assise %d %% · politiques %+d · rancœur %d" % [
			int(fe.get("base_part", 0)), int(fe.get("policy_delta", 0)), int(fe.get("grief", 0))]
		if bool(fe.get("coup_driver", false)):
			detail += " · coup %d" % int(fe.get("coup_pressure", 0))
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, detail, VKit.FS_SMALL)
		_hover.add_dict({"rect": row, "text": "%s — %d %% de soutien." % [name, int(fe.get("part", 0))],
			"card": _faction_info_card(fe, coup, corruption)})
		y += 18
		row_i += 1
	return y

func _draw_conseil(x: float, y: float, me: int) -> float:
	_conseil_btns.clear()
	if Sim.world.has_method("country_revenue_year"):
		_cons_rev = float(Sim.world.country_revenue_year(me))
		_cons_ipm = float(Sim.world.world_ipm())
	# SOUS-ONGLETS : Gouvernement / Politiques / Factions.
	_ctab_btns.clear()
	var cxx := x
	for ti in range(3):
		var lbl: String = ["Gouvernement", "Politiques", "Factions"][ti]
		var tww := VKit.text_w(lbl, VKit.FS_SMALL) + 16.0
		var tr := Rect2(cxx, y, tww, 20)
		VKit.fill(self, tr, VKit.COL_GOLD if _conseil_tab == ti else VKit.COL_PANEL2)
		VKit.box(self, tr, VKit.COL_EDGE)
		VKit.text(self, Vector2(cxx + 8, y + 2),
			VKit.COL_PANEL if _conseil_tab == ti else VKit.COL_PARCH, lbl, VKit.FS_SMALL)
		_ctab_btns.append({"rect": tr, "t": ti})
		cxx += tww + 6
	y += 28
	if _conseil_tab == 1:
		y = _draw_decrets(x, y, me)
		y += 6
		y = _draw_servile(x, y, me)
		if _servile_flash != "":
			VKit.text(self, Vector2(x, y),
				(VKit.sense(0.85) if _servile_flash_ok else VKit.sense(0.10)), _servile_flash, VKit.FS_SMALL)
			y += 16
		elif _decret_flash != "":
			VKit.text(self, Vector2(x, y),
				(VKit.sense(0.85) if _decret_flash_ok else VKit.sense(0.10)), _decret_flash, VKit.FS_SMALL)
			y += 16
		return y
	if _conseil_tab == 2:
		return _draw_factions(x, y, me)
	var idx := 0
	for seat in Sim.world.country_council(me):
		var filled := bool(seat["filled"])
		# BUSTE : sièges Savoir/Société/Industrie → portraits [5,0,3] ; fem. par hash du nom.
		var pt: Texture2D = null
		if filled:
			var pmap := [5, 0, 3]
			pt = UIKit.advisor_portrait(pmap[idx] if idx < pmap.size() else idx % 8,
				String(seat["councilor"]).hash() % 2 == 1)
		if pt != null:
			draw_texture_rect(pt, Rect2(x - 2, y - 3, 20, 20), false)
		else:
			UIKit.draw_icon(self, "menu_council", Vector2(x, y - 1), 20)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_GOLD, String(seat["seat"]))
		y += 18
		if filled:
			var fname := String(seat.get("firstname", ""))
			var house := String(seat.get("house", ""))
			var pname := (fname + " " + house).strip_edges() if fname != "" else String(seat["councilor"])
			var idnom := String(seat.get("identite", ""))
			var idflav := String(seat.get("id_flavor", ""))
			var pdisp := pname + (" · " + idnom if idnom != "" else "")
			VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH, pdisp, VKit.FS_SMALL)
			if idflav != "":
				_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(pdisp, VKit.FS_SMALL) + 6, 16),
					"text": "%s — %s" % [idnom, idflav], "card": _council_seat_info_card(seat)})
			var bw := VKit.text_w("Renvoyer", VKit.FS_SMALL) + 14.0
			var r := Rect2(DW - 14.0 - bw, y - 1, bw, 16)
			VKit.fill(self, r, VKit.COL_PANEL2)
			VKit.box(self, r, VKit.sense(0.12))
			VKit.text(self, Vector2(r.position.x + 7, y), VKit.sense(0.12), "Renvoyer", VKit.FS_SMALL)
			_hover.add(r, "Renvoyer : rancœur +0,10 à sa faction.")
			_conseil_btns.append({"rect": r, "act": "dismiss", "seat": idx, "slot": -1})
			y += 15
			var sline := "%s · rang %d · %d ans" % [String(seat["seat"]), int(seat["tier"]), int(seat.get("age", 0))]
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, sline, VKit.FS_SMALL)
			_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(sline, VKit.FS_SMALL) + 6, 16),
				"text": "Rang %d : base du siège ×%s (I ×1 · II ×1,5 · III ×2) = bonus de rang +%.1f %%." % [
					int(seat["tier"]), ["1", "1,5", "2"][clampi(int(seat["tier"]), 1, 3) - 1], float(seat.get("rank_bonus_pct", 0.0))]})
			y += 18
			var faction := String(seat.get("faction", ""))
			var loyalty := int(seat.get("loyalty", 0))
			var mood := String(seat.get("mood", ""))
			var fline := "Faction : %s" % faction
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, fline, VKit.FS_SMALL)
			_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(fline, VKit.FS_SMALL) + 6, 16),
				"text": "Sa faction gagne du pouvoir tant qu'il siège ; loyauté et Corruption décident de combien le bonus est réellement délivré."})
			y += 15
			VKit.gauge(self, x + 16, y, DW - 32.0, 8.0, loyalty)
			y += 13
			var lline := "Loyauté %d — %s" % [loyalty, mood]
			VKit.text(self, Vector2(x + 16, y), VKit.sense(float(loyalty) / 100.0), lline, VKit.FS_SMALL)
			var loyalty_target := int(seat.get("loyalty_target", loyalty))
			_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(lline, VKit.FS_SMALL) + 6, 16),
				"text": "Loyauté %d/100 · cible actuelle %d/100 → +%.1f pts d'efficacité." % [
					loyalty, loyalty_target, float(seat.get("eff_loyalty_points", 0.0))]})
			y += 18
			# le curseur de PAIE LINÉARISÉ 0–100 % — verbe CMD_COUNCIL_PAY, journalisé
			var pay := float(seat.get("pay", 1.0))
			var pay_live := ("%s or/mois" % _grp(int(round(float(seat["cost_year"]) / 12.0 * pay)))) if seat.has("cost_year") else ""
			y = _draw_multiplier_slider(x + 12.0, y, "Paie", pay, _conseil_btns,
				{"kind": "pay", "family": 2, "seat": idx, "act": "pay", "slot": 0},
				"Traitement du conseiller. Payer moins réduit son coût mais fait chuter sa loyauté.",
				pay_live)
			# membrane : « Administration », jamais « K ».
			if seat.has("rank_bonus_pct"):
				var domain := String(seat.get("domain", ""))
				var rankp := float(seat["rank_bonus_pct"])
				var effp := float(seat["efficiency_pct"])
				var finalp := float(seat["final_bonus_pct"])
				var kpts := float(seat.get("eff_admin_points", 0.0))
				var lpts := float(seat.get("eff_loyalty_points", 0.0))
				var cpts := float(seat.get("eff_corruption_points", 0.0))
				var bline := "%s +%.1f %%" % [domain, finalp]
				var bline_lbl_w: float = VKit.detail(self, Vector2(x + 16, y), "%s " % domain, VKit.FS_SMALL)
				VKit.value(self, Vector2(x + 16 + bline_lbl_w, y), "+%.1f %%" % finalp, VKit.FS_SMALL)
				_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(bline, VKit.FS_SMALL) + 6, 16),
					"text": "Rang : +%.1f %% · Administration : +%.1f pts · Loyauté : +%.1f pts · Corruption : −%.1f pts · Efficacité : %.1f %% ⇒ +%.1f %% net." % [
						rankp, kpts, lpts, cpts, effp, finalp]})
				y += 16
				var cyear := float(seat.get("cost_year", 0.0))
				var cmonth := cyear / 12.0 * pay
				var cline := "%s or / mois" % _grp(int(round(cmonth)))
				VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
				_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
					"text": "Traitement prélevé chaque mois sur le trésor, à la paie actuelle."})
				y += 15
				var rlo := int(seat.get("retire_lo", -1))
				var rhi := int(seat.get("retire_hi", -1))
				if rlo >= 0:
					var rline := "Retraite : %d à %d ans" % [rlo, rhi]
					VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, rline, VKit.FS_SMALL)
					_hover.add_dict({"rect": Rect2(x + 14, y - 2, VKit.text_w(rline, VKit.FS_SMALL) + 6, 16),
						"text": "Départ entre 66 et 73 ans — il en a %d : le siège se libère dans %d à %d ans." % [
							int(seat.get("age", 0)), rlo, rhi]})
					y += 15
			y += 6
		else:
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "(siège vacant : la pool se renouvelle par génération)", VKit.FS_SMALL)
			y += 20
			if Sim.world.has_method("council_candidates"):
				for cand in Sim.world.council_candidates(idx):
					var cx := x + 16
					var cy0 := y
					var cfname := String(cand.get("firstname", ""))
					var chouse := String(cand.get("house", ""))
					var cpname := (cfname + " " + chouse).strip_edges() if cfname != "" else String(cand["nom"])
					var cidnom := String(cand.get("identite", ""))
					var cidflav := String(cand.get("id_flavor", ""))
					var cpdisp := cpname + (" · " + cidnom if cidnom != "" else "")
					VKit.text(self, Vector2(cx, y), VKit.COL_PARCH, cpdisp, VKit.FS_SMALL)
					if cidflav != "":
						_hover.add_dict({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cpdisp, VKit.FS_SMALL) + 6, 16),
							"text": "%s — %s" % [cidnom, cidflav], "card": _council_candidate_info_card(cand)})
					y += 15
					var cfline := "Faction : %s · rang %d · %d ans" % [String(cand.get("faction", "")), int(cand["tier"]), int(cand["age"])]
					VKit.text(self, Vector2(cx, y), VKit.COL_DIM, cfline, VKit.FS_SMALL)
					_hover.add_dict({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cfline, VKit.FS_SMALL) + 6, 16),
						"text": "Rang %d : base du siège ×%s (I ×1 · II ×1,5 · III ×2). Sa faction gagnera du pouvoir s'il siège." % [
							int(cand["tier"]), ["1", "1,5", "2"][clampi(int(cand["tier"]), 1, 3) - 1]]})
					y += 15
					if cand.has("rank_bonus_pct"):
						var cdomain := String(cand.get("domain", ""))
						var crankp := float(cand["rank_bonus_pct"])
						var ceffp := float(cand["efficiency_pct"])
						var cfinalp := float(cand["final_bonus_pct"])
						var ckpts := float(cand.get("eff_admin_points", 0.0))
						var ccpts := float(cand.get("eff_corruption_points", 0.0))
						var clpts := float(cand.get("eff_loyalty_points", 0.0))
						var cbline := "%s +%.1f %%" % [cdomain, cfinalp]
						VKit.text(self, Vector2(cx, y), VKit.sense(0.70), cbline, VKit.FS_SMALL)
						_hover.add_dict({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cbline, VKit.FS_SMALL) + 6, 16),
							"text": "Rang : +%.1f %% · Administration : +%.1f pts · Loyauté de départ : +%.1f pts · Corruption : −%.1f pts · Efficacité prévue : %.1f %% ⇒ +%.1f %% net." % [
								crankp, ckpts, clpts, ccpts, ceffp, cfinalp]})
						y += 15
						var ccyear := float(cand.get("cost_year", 0.0))
						var ccline := "%s or / mois" % _grp(int(round(ccyear / 12.0)))
						VKit.text(self, Vector2(cx, y), VKit.COL_DIM, ccline, VKit.FS_SMALL)
						_hover.add_dict({"rect": Rect2(cx - 2, y - 2, VKit.text_w(ccline, VKit.FS_SMALL) + 6, 16),
							"text": "Traitement de base, prélevé chaque mois une fois le candidat en poste."})
						y += 15
						var crlo := int(cand.get("retire_lo", -1))
						if crlo >= 0:
							var crline := "Retraite : %d à %d ans" % [crlo, int(cand.get("retire_hi", -1))]
							VKit.text(self, Vector2(cx, y), VKit.COL_DIM, crline, VKit.FS_SMALL)
							_hover.add_dict({"rect": Rect2(cx - 2, y - 2, VKit.text_w(crline, VKit.FS_SMALL) + 6, 16),
								"text": "Départ entre 66 et 73 ans — il en a %d : le siège se libérerait dans %d à %d ans." % [
									int(cand["age"]), crlo, int(cand.get("retire_hi", -1))]})
							y += 15
					else:
						# repli binding sans les champs carte : le coût MENSUEL (cand["cost"]).
						VKit.text(self, Vector2(cx, y), VKit.COL_DIM,
							"%d ans · rang %d · %.0f or / mois" % [int(cand["age"]), int(cand["tier"]), float(cand["cost"])], VKit.FS_SMALL)
						y += 15
					var cw := DW - 32.0
					var lab := "Recruter"
					var lw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
					var hr := Rect2(x + 16 + cw - 10.0 - lw, y, lw, 16)
					VKit.fill(self, hr, VKit.COL_PANEL2)
					VKit.box(self, hr, VKit.sense(0.80))
					VKit.text(self, Vector2(hr.position.x + 7, y + 1), VKit.sense(0.80), lab, VKit.FS_SMALL)
					_conseil_btns.append({"rect": hr, "act": "hire", "seat": idx, "slot": int(cand["slot"])})
					y += 22
					var cardh := y - cy0
					var cr := Rect2(x + 12, cy0 - 3, cw, cardh)
					VKit.box(self, cr, VKit.COL_EDGE)
			y += 8
		idx += 1
	# MISSION DÉCENNALE : siège responsable (mission_responsible_seat) + bonus + récompense prévue.
	if Sim.world.has_method("mission_info"):
		var mi: Dictionary = Sim.world.mission_info(me)
		if bool(mi.get("active", false)):
			y += 4
			VKit.fill(self, Rect2(x, y, DW - 28.0, 1), VKit.COL_EDGE)
			y += 8
			VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "✦ Mission décennale", VKit.FS_SMALL)
			y += 16
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, String(mi.get("text", "")), VKit.FS_SMALL)
			y += 16
			var rname := String(mi.get("resp_name", ""))
			var rseat := String(mi.get("resp_seat", ""))
			if rseat != "":
				var rtxt := ("Responsable : %s, %s, rang %d" % [rname, rseat, int(mi.get("resp_tier", 0))]) \
					if rname != "" else ("Responsable : %s — siège vacant" % rseat)
				VKit.text(self, Vector2(x, y), VKit.COL_DIM, rtxt, VKit.FS_SMALL)
				y += 15
				if rname != "":
					VKit.text(self, Vector2(x, y), VKit.sense(0.70),
						"Bonus du responsable : +%.1f %%" % float(mi.get("resp_bonus_pct", 0.0)), VKit.FS_SMALL)
					y += 15
			var rw := "%.0f or" % float(mi.get("reward_gold_adj", mi.get("reward_gold", 0)))
			var mat := String(mi.get("reward_mat", ""))
			if mat != "" and float(mi.get("reward_qty_adj", mi.get("reward_qty", 0))) > 0.0:
				rw += " + %.0f %s" % [float(mi.get("reward_qty_adj", mi.get("reward_qty", 0))), mat]
			var rw_lbl_w2: float = VKit.detail(self, Vector2(x, y), "Récompense prévue : ", VKit.FS_SMALL)
			VKit.value(self, Vector2(x + rw_lbl_w2, y), rw, VKit.FS_SMALL)
			y += 16
	if _conseil_flash != "":
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _conseil_flash_ok else VKit.sense(0.10)), _conseil_flash, VKit.FS_SMALL)
		y += 16
	return y

# DÉCRETS : un ÉDIT se bascule librement ; une RÉFORME activée est VERROUILLÉE (irréversible) ;
# une DÉCISION est PONCTUELLE, gatée par condition + cooldown. Grisé si legal==0.
const DCR_EDIT := 0
const DCR_REFORME := 1
const DCR_POSTURE := 2
const DCR_DECISION := 3

var _decret_btns := []   # [{rect, id, on}] — on=true active un ÉDIT / tire une DÉCISION
var _decret_flash := ""
var _decret_flash_ok := true

func _draw_decrets(x: float, y: float, me: int) -> float:
	_decret_btns.clear()
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Orientations politiques", VKit.FS_BIG)
	y += 20
	if not Sim.world.has_method("decrees_list"):
		return y
	var decs: Array = Sim.world.decrees_list(me)
	# nom par id — pour la note d'exclusivité « ⊥ exclusif avec : X ».
	var names_by_id := {}
	for dd in decs:
		names_by_id[int(dd["id"])] = String(dd["nom"])
	var decisions := []
	for dec in decs:
		if int(dec.get("type", DCR_EDIT)) == DCR_DECISION:
			decisions.append(dec)
			continue
		y = _draw_decree_card(x, y, dec, names_by_id)
	if decisions.size() > 0:
		y += 6
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Décisions ponctuelles", VKit.FS_BIG)
		y += 20
		for dec in decisions:
			y = _draw_decision_card(x, y, dec, me)
	return y

func _draw_decree_card(x: float, y: float, dec: Dictionary, names_by_id: Dictionary) -> float:
	var id := int(dec["id"])
	var active := bool(dec["active"])
	var legal := bool(dec["legal"])
	var reforme := bool(dec["reforme"])
	var nom := String(dec["nom"])
	var label := nom + (" [RÉFORME]" if reforme else "") + (" — actif" if active else "")
	VKit.text(self, Vector2(x, y), (VKit.sense(0.80) if active else (VKit.COL_PARCH if legal else VKit.COL_DIM)), label, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x, y - 2, VKit.text_w(label, VKit.FS_SMALL), 14),
		"text": String(dec["plateaux"]) + "\n" + String(dec["flavor"])})
	y += 15
	var rate := float(dec.get("cost_rate_pct", 0.0))
	var cyear := float(dec.get("cost_year", 0.0))
	var cmonth := cyear / 12.0
	var cline := ("%s or par mois" % _grp(int(round(cmonth)))) if rate > 0.0 else "0 or par mois"
	VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
	if rate > 0.0:
		_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
			"text": "%.2f %% du revenu (%s or) × IPM %.2f — prélevé chaque mois ; mois impayé ⇒ sans effet ce mois-là." % [
				rate, _grp(int(round(_cons_rev))), _cons_ipm]})
	else:
		_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
			"text": "Aucun prélèvement d'or — la contrepartie est dans l'effet (survolez le nom)."})
	y += 15
	var excl := int(dec.get("exclusive_id", -1))
	if excl >= 0 and names_by_id.has(excl):
		var eline := "⊥ exclusif avec : %s" % String(names_by_id[excl])
		VKit.text(self, Vector2(x + 8, y), VKit.sense(0.35), eline, VKit.FS_SMALL)
		_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(eline, VKit.FS_SMALL) + 6, 16),
			"text": "Paire radio : activer celle-ci désactive automatiquement l'autre."})
		y += 15
	if reforme and active:
		VKit.text(self, Vector2(x + 8, y), VKit.sense(0.30), "verrouillé (irréversible)", VKit.FS_SMALL)
		y += 18
	else:
		var lab := "Désactiver" if active else "Activer"
		var enabled := active or legal   # OFF toujours permis ; ON gate sur legal
		var bw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
		var r := Rect2(x + 8, y - 1, bw, 16)
		var col := (VKit.sense(0.12) if active else VKit.sense(0.80)) if enabled else VKit.COL_EDGE
		VKit.fill(self, r, VKit.COL_PANEL2)
		VKit.box(self, r, col)
		VKit.text(self, Vector2(r.position.x + 7, y), col if enabled else VKit.COL_DIM, lab, VKit.FS_SMALL)
		if enabled:
			_decret_btns.append({"rect": r, "id": id, "on": not active})
		elif not legal:
			_hover.add(r, "condition d'entrée non remplie")
		y += 20
	y += 4
	return y

func _draw_decision_card(x: float, y: float, dec: Dictionary, me: int) -> float:
	var id := int(dec["id"])
	var legal := bool(dec["legal"])
	var cond_met := bool(dec.get("cond_met", legal))
	var cooldown := bool(dec.get("cooldown_active", false))
	var nom := String(dec["nom"])
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH if legal else VKit.COL_DIM, nom, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x, y - 2, VKit.text_w(nom, VKit.FS_SMALL), 14),
		"text": String(dec["plateaux"]) + "\n" + String(dec["flavor"])})
	y += 15
	var cnd := "Condition : %s" % ("remplie" if cond_met else "non remplie")
	VKit.text(self, Vector2(x + 8, y), VKit.sense(0.70 if cond_met else 0.15), cnd, VKit.FS_SMALL)
	var corr_now := -1
	if Sim.world.has_method("country_factions"):
		corr_now = int(Sim.world.country_factions(me).get("corruption", -1))
	if corr_now >= 0:
		_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(cnd, VKit.FS_SMALL) + 6, 16),
			"text": "Corruption %d/100 — exige ≥ 20." % corr_now})
	y += 15
	if cooldown:
		# jours restants non exposés par la façade — on affiche l'état, pas de compte à rebours inventé.
		var cdl := "Cooldown : en cours"
		VKit.text(self, Vector2(x + 8, y), VKit.sense(0.30), cdl, VKit.FS_SMALL)
		_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(cdl, VKit.FS_SMALL) + 6, 16),
			"text": "5 ans entre deux audits — réutilisable à la fin du délai."})
		y += 15
	var rate := float(dec.get("cost_rate_pct", 0.0))
	var cyear := float(dec.get("cost_year", 0.0))
	var cline := "%s or (une fois)" % _grp(int(round(cyear)))
	VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
	_hover.add_dict({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
		"text": "%.0f %% du revenu (%s or) × IPM %.2f = %s or, prélevé au tir." % [
			rate, _grp(int(round(_cons_rev))), _cons_ipm, _grp(int(round(cyear)))]})
	y += 16
	var lab := "Décréter"
	var bw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
	var r := Rect2(x + 8, y - 1, bw, 16)
	var col := VKit.sense(0.80) if legal else VKit.COL_EDGE
	VKit.fill(self, r, VKit.COL_PANEL2)
	VKit.box(self, r, col)
	VKit.text(self, Vector2(r.position.x + 7, y), col if legal else VKit.COL_DIM, lab, VKit.FS_SMALL)
	if legal:
		_decret_btns.append({"rect": r, "id": id, "on": true})
	else:
		var reason := "condition non remplie" if not cond_met else ("cooldown en cours" if cooldown else "indisponible")
		_hover.add(r, reason)
	y += 22
	return y

func _decret_act(id: int, on: bool) -> void:
	var w = Sim.world
	if w == null:
		return
	var ok: bool = w.player_decree(id, on)
	_decret_flash_ok = ok
	_decret_flash = ("⚑ décret — ordre émis" if ok else "✗ décret — refusé")
	queue_redraw()

# PEUPLE SERVILE : ACHETER/VENDRE (gate abolitionniste grisé + MOT), AFFRANCHIR avec confirmation 2 clics (verbe irréversible).
var _servile_btns := []   # [{rect, act, qty}]  act: "buy"|"sell"|"manumit_arm"|"manumit_confirm"
var _servile_flash := ""
var _servile_flash_ok := true
var _servile_manumit_armed := false   # 1er clic arme la confirmation, 2e clic l'exécute

func _draw_servile(x: float, y: float, me: int) -> float:
	_servile_btns.clear()
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Peuple servile", VKit.FS_BIG)
	y += 20
	if not Sim.world.has_method("manumit_preview"):
		return y
	var w = Sim.world
	var mp: Dictionary = w.manumit_preview()
	var souls := int(mp.get("souls", 0))
	VKit.value(self, Vector2(x, y),
		"âmes serviles : %s (%.1f%% du pays)" % [_grp(souls), float(mp.get("pct_of_country", 0.0))], VKit.FS_SMALL)
	y += 18

	if w.has_method("slave_market"):
		var mk: Dictionary = w.slave_market()
		var can_buy := bool(mk.get("can_buy", false))
		VKit.text(self, Vector2(x, y), VKit.COL_DIM,
			"marché mondial : %s âme(s)" % _grp(int(mk.get("total", 0))), VKit.FS_SMALL)
		y += 15
		# SPREAD débité au drain (achat ×2 / vente ×1), jamais montré ici.
		var pb := int(mk.get("price_buy", 0))
		var ps := int(mk.get("price_sell", 0))
		if pb > 0 or ps > 0:
			VKit.text(self, Vector2(x, y), VKit.COL_DIM,
				"prix courant : achat %d or/âme · vente %d or/âme" % [pb, ps], VKit.FS_SMALL)
			y += 15
		for ln in mk.get("lines", []):
			VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM,
				"%s — %s" % [String(ln.get("heritage", "?")), _grp(int(ln.get("count", 0)))], VKit.FS_SMALL)
			y += 14

		var cap_prov: int = w.country_capital_province(me)
		var cap_region: int = w.province_region(cap_prov) if cap_prov >= 0 else -1
		for qty in [50, 200]:
			var lab_b := "Acheter %d" % qty
			var bw := VKit.text_w(lab_b, VKit.FS_SMALL) + 12.0
			var rb := Rect2(x, y, bw, 16)
			var buy_ok := can_buy and cap_region >= 0
			VKit.fill(self, rb, VKit.COL_PANEL2)
			VKit.box(self, rb, VKit.sense(0.80) if buy_ok else VKit.COL_EDGE)
			VKit.text(self, Vector2(rb.position.x + 6, y), VKit.COL_PARCH if buy_ok else VKit.COL_DIM, lab_b, VKit.FS_SMALL)
			if buy_ok:
				_servile_btns.append({"rect": rb, "act": "buy", "qty": qty})
			else:
				_hover.add(rb, "gate éthos/tech — un pays abolitionniste ne peut pas acheter")

			var lab_s := "Vendre %d" % qty
			var sw := VKit.text_w(lab_s, VKit.FS_SMALL) + 12.0
			var rs := Rect2(rb.position.x + bw + 6.0, y, sw, 16)
			var sell_ok := cap_region >= 0 and souls > 0
			VKit.fill(self, rs, VKit.COL_PANEL2)
			VKit.box(self, rs, VKit.sense(0.80) if sell_ok else VKit.COL_EDGE)
			VKit.text(self, Vector2(rs.position.x + 6, y), VKit.COL_PARCH if sell_ok else VKit.COL_DIM, lab_s, VKit.FS_SMALL)
			if sell_ok:
				_servile_btns.append({"rect": rs, "act": "sell", "qty": qty})
			y += 19
		if not can_buy:
			VKit.text(self, Vector2(x, y), VKit.sense(0.30), "achat interdit — éthos/tech abolitionniste", VKit.FS_SMALL)
			y += 16

	y += 4
	if souls > 0:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM,
			"aperçu : %d groupe(s) · friction attendue %.0f%%" %
			[int(mp.get("n_groups", 0)), float(mp.get("friction_after", 0.0)) * 100.0], VKit.FS_SMALL)
		y += 16
		var lab := "Confirmer l'affranchissement" if _servile_manumit_armed else "Affranchir les esclaves du pays"
		var mw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
		var mr := Rect2(x, y, mw, 17)
		var mcol := VKit.sense(0.10) if _servile_manumit_armed else VKit.sense(0.80)
		VKit.fill(self, mr, VKit.COL_PANEL2)
		VKit.box(self, mr, mcol)
		VKit.text(self, Vector2(mr.position.x + 7, y + 1), mcol, lab, VKit.FS_SMALL)
		_servile_btns.append({"rect": mr, "act": "manumit_confirm" if _servile_manumit_armed else "manumit_arm", "qty": 0})
		y += 20
		if _servile_manumit_armed:
			VKit.text(self, Vector2(x, y), VKit.sense(0.30), "irréversible — cliquez de nouveau pour confirmer", VKit.FS_SMALL)
			y += 16
	else:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(aucune âme servile à affranchir)", VKit.FS_SMALL)
		y += 16
	return y

func _servile_act(act: String, qty: int, me: int) -> void:
	var w = Sim.world
	if w == null:
		return
	if act == "manumit_arm":
		_servile_manumit_armed = true
		_servile_flash = ""
		queue_redraw()
		return
	var ok := false
	var label := ""
	var is_trade := false
	if act == "manumit_confirm":
		ok = bool(w.player_manumit())
		label = "affranchissement"
		_servile_manumit_armed = false
	else:
		# RE-KEY PROVINCE : slave_buy/slave_sell prennent un PID direct.
		var cap_prov: int = w.country_capital_province(me)
		if cap_prov < 0:
			_servile_flash_ok = false
			_servile_flash = "✗ aucune capitale — refusé"
			Sound.play("ui_click")
			queue_redraw()
			return
		is_trade = true
		if act == "buy":
			ok = bool(w.player_slave_buy(cap_prov, qty))
			label = "achat"
		else:
			ok = bool(w.player_slave_sell(cap_prov, qty))
			label = "vente"
	_servile_flash_ok = ok
	_servile_flash = ("⚑ %s — ordre émis" % label) if ok else ("✗ %s — refusé" % label)
	if is_trade:
		Sound.play("ui_click")
	elif not ok:
		Sound.play("ui_click")
	queue_redraw()

# ARMÉE : readouts + verbes joueur (recruter/flotte).
const HULL_LABELS := [["+Guerre", 0], ["+Transport", 1], ["+Marchand", 2]]   # HullType : HULL_WAR·HULL_TRANSPORT·HULL_MERCHANT

var _army_btns := []      # [{rect, act}] Recompléter / Dissoudre
var _navy_btns := []      # [{rect, hull}] +Guerre / +Transport / +Marchand

func _draw_armee(x: float, y: float, me: int) -> float:
	_army_btns.clear(); _navy_btns.clear()
	var a: Dictionary = Sim.world.country_army(me)
	UIKit.draw_icon(self, "menu_army", Vector2(x, y - 1), 22)
	VKit.value(self, Vector2(x + 26, y), "force mobilisée : %d régiments" % int(a["regiments"]))
	y += 24
	var ar: Dictionary = Sim.world.army_info(me)
	if bool(ar.get("active", false)):
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD,
			"armée de campagne — région %d · %s" % [int(ar["region"]), ar["phase"]], VKit.FS_SMALL)
		y += 16
		VKit.value(self, Vector2(x, y),
			"inf %d · arch %d · cav %d · mages %d  (Σ %d)" % [
				int(ar["inf"]), int(ar["arch"]), int(ar["cav"]), int(ar["mages"]), int(ar["units"])], VKit.FS_SMALL)
		y += 20
	else:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(pas d'armée de campagne déployée)", VKit.FS_SMALL)
		y += 20
	# Recompléter / Dissoudre (verbes player_refill / player_disband)
	var b1w := VKit.text_w("Recompléter", VKit.FS_SMALL) + 14.0
	var r1 := Rect2(x, y, b1w, 18)
	VKit.fill(self, r1, VKit.COL_PANEL2); VKit.box(self, r1, VKit.COL_GOLD)
	VKit.text(self, Vector2(x + 7, y + 1), VKit.COL_GOLD, "Recompléter", VKit.FS_SMALL)
	_army_btns.append({"rect": r1, "act": "refill"})
	var b2x := x + b1w + 6.0
	var b2w := VKit.text_w("Dissoudre", VKit.FS_SMALL) + 14.0
	var r2 := Rect2(b2x, y, b2w, 18)
	VKit.fill(self, r2, VKit.COL_PANEL2); VKit.box(self, r2, VKit.COL_GOLD)
	VKit.text(self, Vector2(b2x + 7, y + 1), VKit.COL_GOLD, "Dissoudre", VKit.FS_SMALL)
	_army_btns.append({"rect": r2, "act": "disband"})
	y += 26
	UIKit.draw_icon(self, "harbor_anchor", Vector2(x, y - 1), 18)
	VKit.value(self, Vector2(x + 22, y), "Flotte : %d coque(s)" % int(a["fleet"]))
	y += 20
	# Flotte : mise en chantier (verbe player_navy_build)
	var hull_boat := ["sheet24_topbar_boats_menu_11", "sheet24_topbar_boats_menu_13", "sheet24_topbar_boats_menu_10"]
	var cx := x
	for it in HULL_LABELS:
		var label: String = it[0]
		var hull: int = it[1]
		var bt: Texture2D = UIKit.parch_tex(hull_boat[hull]) if hull < hull_boat.size() else null
		var iw := 18.0 if bt != null else 0.0
		var tw := VKit.text_w(label, VKit.FS_SMALL) + 12.0 + iw
		if cx + tw > DW - 12.0:
			cx = x; y += 20
		var r := Rect2(cx, y, tw, 18)
		VKit.fill(self, r, VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_GOLD)
		if bt != null:
			draw_texture_rect(bt, Rect2(cx + 3, y + 1, 16, 16), false)
		VKit.text(self, Vector2(cx + 6 + iw, y + 1), VKit.COL_GOLD, label, VKit.FS_SMALL)
		_navy_btns.append({"rect": r, "hull": hull})
		cx += tw + 4
	y += 26

	# COMPOSER L'ARMÉE : le recrutement est NATIONAL (pas dans l'UI province). Clic = player_recruit (journalisé).
	_unit_btns.clear()
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Composer l'armée", VKit.FS_SMALL)
	y += 16
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "clic pour lever une unité", VKit.FS_SMALL)
	y += 16
	# on n'affiche QUE les unités recrutables (les verrouillées par la tech disparaissent).
	var rec: Array = []
	for u in Sim.world.unit_roster(me):
		if bool(u.get("recrutable", false)):
			rec.append(u)
	var ucell := 40.0
	var ucols := maxi(1, int((DW - 2.0 * x) / ucell))
	if rec.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(aucune unité disponible — recherche militaire requise)", VKit.FS_SMALL)
		y += 18
	for i in range(rec.size()):
		var u: Dictionary = rec[i]
		var ur := Rect2(x + (i % ucols) * ucell, y + (i / ucols) * ucell, ucell - 4.0, ucell - 4.0)
		var ut: Texture2D = UIKit.unit_sprite(int(u.get("type", -1)))
		if ut != null:
			draw_texture_rect(ut, ur, false)
		else:
			VKit.fill(self, ur, VKit.COL_PANEL2)
			VKit.text(self, ur.position + Vector2(2, 10), VKit.COL_DIM, String(u.get("nom", "")).substr(0, 5), VKit.FS_SMALL)
		VKit.box(self, ur, VKit.COL_GOLD)
		_unit_btns.append({"rect": ur, "type": int(u.get("type", -1)), "nom": String(u.get("nom", "")), "on": true})
		_tips.append([ur, "%s — %s · %s\nEfficace contre : %s\nFaible contre : %s\nCoût : %s · Entretien : %.1f or/100" % [
			String(u.get("nom", "")), String(u.get("categorie", "")), String(u.get("arme", "")),
			String(u.get("fort", "—")), String(u.get("faible", "—")),
			String(u.get("cout", "")), float(u.get("entretien_or10", 5)) / 10.0]])
	y += ceilf(rec.size() / float(ucols)) * ucell + 4.0

	if _armee_flash != "":
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _armee_flash_ok else VKit.sense(0.10)), _armee_flash, VKit.FS_SMALL)
		y += 16
	return y

var _armee_flash := ""
var _armee_flash_ok := true
var _unit_btns := []   ## composeur d'armée : [{rect, type, nom, on}]
var _tips: Array = []  ## [[Rect2, texte], …]

func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""

func get_info_card(at_position: Vector2) -> Dictionary:
	for t in _tips:
		if t.size() >= 3 and (t[0] as Rect2).has_point(at_position):
			return (t[2] as Dictionary).duplicate(true)
	return {}

func _draw_filtres(x: float, y: float) -> float:
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
			VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
			VKit.box(self, r, VKit.COL_EDGE)
			VKit.text(self, Vector2(cx + 7, y + 1), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
			_chips.append({"rect": r, "mode": mode})
			cx += tw + 4
		y += 26
	return y

# DIPLOMATIE : liste-résumé read-only ; les ACTIONS vivent dans la fenêtre par pays (clic sur la ligne).
## JOURNAL D'ACTES (DiplogAct moteur) : [libellé quand LUI agit, quand NOUS agissons, hostile?].
const DACT_LABEL := {
	1: ["nous a déclaré la GUERRE", "guerre déclarée par nous", true],
	2: ["paix signée", "paix signée", false],
	3: ["alliance nouée", "alliance nouée", false],
	4: ["pacte commercial scellé", "pacte commercial scellé", false],
	5: ["pacte commercial rompu", "pacte rompu par nous", true],
	6: ["nous a mis sous EMBARGO", "embargo décrété par nous", true],
	7: ["a levé son embargo", "embargo levé par nous", false],
	8: ["a TRAHI sa parole", "notre parole rompue", true],
	9: ["né d'une SÉCESSION de notre couronne", "sécession", true],
	10: ["a soigné les relations", "relations soignées par nous", false],
}

func _draw_diplo(x: float, y: float, me: int) -> float:
	_diplo_btns.clear()
	# BROUILLARD : un pays jamais découvert n'existe pas dans la liste — filtré D'ABORD (pour l'état vide explicatif).
	var rels: Array = []
	for rel in Sim.world.country_relations(me):
		var target0: int = int(rel["country"])
		if Sim.world.has_method("country_known") and int(Sim.world.country_known(target0)) == 0:
			continue
		rels.append(rel)
	if rels.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "Aucun pays étranger connu.", VKit.FS_SMALL)
		y += 18
		y += VKit.text_wrapped(self, Vector2(x, y), VKit.COL_DIM,
			"Explorez la carte, ouvrez une route ou attendez qu'un contact soit établi.",
			DW - 2.0 * x, 3, VKit.FS_SMALL)
		y += 8
		VKit.fill(self, Rect2(x, y, DW - 2.0 * x, 1), VKit.COL_EDGE)
		y += 10
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Moyens existants", VKit.FS_SMALL)
		y += 18
		for line in [
			"coloniser une région voisine étend vos frontières — et le rayon de découverte avec elles",
			"une route commerciale approfondit le contact avec un partenaire déjà croisé",
			"la découverte est cumulative, sur un rayon de 2 régions autour des vôtres",
		]:
			VKit.text(self, Vector2(x + 6, y), VKit.COL_DIM, "•", VKit.FS_SMALL)
			y += VKit.text_wrapped(self, Vector2(x + 18, y), VKit.COL_DIM, line, DW - 2.0 * x - 18.0, 2, VKit.FS_SMALL)
			y += 4
		return y + 4.0
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "▸ cliquer une fiche : actions diplomatiques", VKit.FS_SMALL)
	y += 18
	for rel in rels:
		var target: int = int(rel["country"])
		var row_y0 := y
		var at_war: bool = bool(rel["at_war"])
		var allied: bool = bool(rel["allied"])
		var col := VKit.sense(0.12) if at_war else (VKit.sense(0.78) if allied else VKit.COL_PARCH)
		VKit.text(self, Vector2(x, y), col, String(rel["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), VKit.COL_DIM, String(rel["status"]), VKit.FS_SMALL)
		y += 14
		# opinion ±100 = ce que CE pays pense de nous
		var op: int = int(rel["opinion"])
		_opinion_bar(x, y, 150.0, op)
		VKit.text(self, Vector2(x + 158, y - 3), _opinion_col(op), "%+d" % op, VKit.FS_SMALL)
		y += 14
		# les composantes vers lesquelles l'opinion converge ; seules les NON NULLES s'affichent.
		var parts: Dictionary = Sim.world.opinion_summary(target)
		if not parts.is_empty():
			var tx := x
			var drew := false
			for pk in [["Alliance", "ally"], ["Guerre", "war"], ["Vassalité", "vassal"],
				["Pacte", "pact"], ["Embargo", "embargo"], ["Rancune", "rancor"], ["Mémoire", "memory"]]:
				var v: int = int(parts.get(pk[1], 0))
				if v == 0:
					continue
				var seg := "%s %+d" % [pk[0], v]
				var segw := VKit.text_w(seg, VKit.FS_SMALL)
				if tx + segw > DW - 16.0:
					break
				VKit.text(self, Vector2(tx, y), VKit.sense(0.78) if v > 0 else VKit.sense(0.15), seg, VKit.FS_SMALL)
				tx += segw + 10.0
				drew = true
			if drew:
				y += 13
		# JOURNAL : les 3 actes datés les plus récents (le poids RESTANT décayé « s'estompe »).
		var me2: int = Sim.world.player()
		var shown := 0
		for e in Sim.world.diplo_journal(target):
			if shown >= 3:
				break
			if not DACT_LABEL.has(int(e["act"])):
				continue
			var lab: Array = DACT_LABEL[int(e["act"])]
			var by_us: bool = (int(e["a"]) == me2)
			var line := "an %d · %s" % [int(e["year"]), String(lab[1] if by_us else lab[0])]
			var dv: int = int(e["delta"])
			if dv != 0:
				line += "  (%+d, s'estompe)" % dv
			var lc: Color = VKit.sense(0.20) if bool(lab[2]) else VKit.COL_DIM
			VKit.text(self, Vector2(x + 6, y), lc, line, VKit.FS_SMALL)
			y += 12
			shown += 1
		# toute la fiche est cliquable → la fenêtre d'actions du pays (pas de boutons ici)
		var row_rect := Rect2(x - 4.0, row_y0 - 2.0, DW - 2.0 * x + 8.0, (y - row_y0) + 4.0)
		_diplo_btns.append({"rect": row_rect, "act": "open", "target": target, "nom": String(rel["name"])})
		y += 6
		VKit.fill(self, Rect2(x, y - 3, DW - 2.0 * x, 1), VKit.COL_EDGE)
	return y

## barre d'opinion ±100 : remplissage vert (favorable) / rouge (hostile) depuis le centre.
func _opinion_bar(x: float, y: float, w: float, op: int) -> void:
	VKit.fill(self, Rect2(x, y, w, 8), VKit.COL_PANEL2)
	VKit.box(self, Rect2(x, y, w, 8), VKit.COL_EDGE)
	var mid := x + w * 0.5
	VKit.fill(self, Rect2(mid, y, 1, 8), VKit.COL_DIM)
	var frac := clampf(op / 100.0, -1.0, 1.0)
	if frac >= 0.0:
		VKit.fill(self, Rect2(mid, y + 1, (w * 0.5) * frac, 6), VKit.sense(0.80))
	else:
		var ww := (w * 0.5) * (-frac)
		VKit.fill(self, Rect2(mid - ww, y + 1, ww, 6), VKit.sense(0.12))

func _opinion_col(op: int) -> Color:
	if op > 15: return VKit.sense(0.80)
	if op < -15: return VKit.sense(0.15)
	return VKit.COL_DIM

## Armée : recompléter/dissoudre/chantier — verbes journalisés ; aucun n'échoue localement sauf navy_build.
func _armee_act(kind: String, val: int) -> void:
	var w = Sim.world
	if w == null:
		return
	Sim.notify_action()   # pause : l'UI se rafraîchit au clic (le drain suit à la reprise)
	match kind:
		"refill":
			var ok: bool = w.player_refill()
			_armee_flash_ok = ok
			_armee_flash = "⚑ recomplètement — ordre émis" if ok else "✗ recomplètement — refusé"
		"disband":
			var ok: bool = w.player_disband()
			_armee_flash_ok = ok
			_armee_flash = "⚑ dissolution — ordre émis" if ok else "✗ dissolution — refusé"
		"navy":
			var ok: bool = w.player_navy_build(val)
			_armee_flash_ok = ok
			_armee_flash = "⚑ coque en chantier — ordre émis" if ok else "✗ chantier naval — refusé"
	queue_redraw()

## Marché : achat/vente de 10 unités sur la région-capitale (verbe journalisé).
func _marche_act(act: String, res_id: int, me: int) -> void:
	var w = Sim.world
	if w == null:
		return
	Sim.notify_action()   # pause : l'UI se rafraîchit au clic
	var cap_prov: int = w.country_capital_province(me)
	var cap_region: int = w.province_region(cap_prov) if cap_prov >= 0 else -1
	if cap_region < 0:
		_marche_flash_ok = false
		_marche_flash = "✗ aucune capitale — refusé"
		Sound.play("ui_click")
		queue_redraw()
		return
	var ok := false
	if act == "buy":
		ok = w.player_market_buy(cap_region, res_id, MARCHE_QTY, 0)
	else:
		ok = w.player_market_sell(cap_region, res_id, MARCHE_QTY, 0)
	_marche_flash_ok = ok
	_marche_flash = ("⚑ %s — ordre émis" % ("achat" if act == "buy" else "vente")) if ok \
		else ("✗ %s — refusé" % ("achat" if act == "buy" else "vente"))
	Sound.play("ui_click")
	queue_redraw()

## Conseil : recruter / renvoyer / payer (curseur 0–100 %) — verbes journalisés.
func _conseil_act(act: String, seat: int, slot: int, pay: float = 1.0) -> void:
	var w = Sim.world
	if w == null:
		return
	var ok := false
	var label := "recrutement"
	if act == "hire":
		ok = w.player_council_hire(seat, slot)   # le candidat choisi
	elif act == "pay":
		ok = w.player_council_pay(seat, pay)
		label = "paie"
	else:
		ok = w.player_council_dismiss(seat)
		label = "renvoi"
	_conseil_flash_ok = ok
	_conseil_flash = ("⚑ %s — ordre émis" % label) if ok else ("✗ %s — refusé" % label)
	queue_redraw()

func _slider_value(data: Dictionary, mouse_x: float) -> float:
	var track: Rect2 = data.get("track", Rect2())
	if track.size.x <= 0.0:
		return 1.0
	var frac := clampf((mouse_x - track.position.x) / track.size.x, 0.0, 1.0)
	# LINÉARISÉ 0–100 % : extrême gauche = 0.02 (« 0 % »), extrême droite = 1.0 (« 100 % »).
	return clampf(roundf((0.02 + frac * 0.98) * 100.0) / 100.0, 0.02, 1.0)

func _apply_multiplier_slider(data: Dictionary, mouse_x: float) -> void:
	var w = Sim.world
	if w == null:
		return
	var value := _slider_value(data, mouse_x)
	var key := _slider_key(data)
	if absf(float(_slider_preview.get(key, -10.0)) - value) < 0.01:
		return
	var ok := false
	if String(data.get("kind", "")) == "pay":
		ok = bool(w.player_council_pay(int(data.get("seat", -1)), value))
	elif w.has_method("player_budget_policy"):
		ok = bool(w.player_budget_policy(int(data.get("family", -1)), int(data.get("index", -1)), value))
	if ok:
		_slider_preview[key] = value
		Sim.notify_action()
		queue_redraw()

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and not _active_slider.is_empty() \
		and (event.button_mask & MOUSE_BUTTON_MASK_LEFT) != 0:
		_apply_multiplier_slider(_active_slider, event.position.x)
		accept_event()
		return
	if event is InputEventMouseButton and not event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_active_slider.clear()
		return
	# HOVER Marché : ligne sous la souris, immédiat (fait apparaître le nom de la ressource).
	if event is InputEventMouseMotion and _tab == 3:
		var hov := -1
		for r in _marche_rows:
			if (r["rect"] as Rect2).has_point(event.position):
				hov = int(r["res_id"])
				break
		if hov != _marche_hover_res:
			_marche_hover_res = hov
			queue_redraw()
	# SCROLL générique : molette = défilement de l'onglet courant (clampé au contenu).
	if event is InputEventMouseButton and event.pressed \
		and (event.button_index == MOUSE_BUTTON_WHEEL_DOWN or event.button_index == MOUSE_BUTTON_WHEEL_UP):
		var off := float(_scroll.get(_tab, 0.0))
		var step := 40.0 if event.button_index == MOUSE_BUTTON_WHEEL_DOWN else -40.0
		_scroll[_tab] = clampf(off + step, 0.0, _maxscroll)
		queue_redraw()
		accept_event()
		return
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if event.position.y < 36.0:
			return   # en-tête fixe : jamais un clic vers un bouton DÉFILÉ dessous
		var slider_zones: Array = _eco_sliders if _tab == 0 else (_conseil_btns if _tab == 7 and _conseil_tab == 0 else [])
		for slider in slider_zones:
			if slider.has("track") and (slider["rect"] as Rect2).has_point(event.position):
				_active_slider = slider.duplicate(true)
				_apply_multiplier_slider(_active_slider, event.position.x)
				Sound.play("ui_click")
				accept_event()
				return
		if _tab == 0 and _chart_btn.has_point(event.position):   # Économie → ouvre les courbes
			charts_requested.emit()
			accept_event()
			return
		if _tab == 3:
			for b in _marche_sort_btns:
				if (b["rect"] as Rect2).has_point(event.position):
					_marche_sort_act(String(b["key"]))
					accept_event()
					return
			for b in _marche_btns:
				if b.rect.has_point(event.position):
					var w = Sim.world
					var me: int = w.player() if w != null else -1
					_marche_act(String(b.act), int(b.res_id), me)
					accept_event()
					return
			for r in _marche_rows:
				if (r["rect"] as Rect2).has_point(event.position):
					_marche_selected_res = int(r["res_id"])
					queue_redraw()
					accept_event()
					return
		if _tab == 4:
			for b in _army_btns:
				if b.rect.has_point(event.position):
					_armee_act(String(b.act), 0)
					accept_event()
					return
			for b in _navy_btns:
				if b.rect.has_point(event.position):
					_armee_act("navy", int(b.hull))
					accept_event()
					return
			# clic tuile = levée (journalisée)
			for ub in _unit_btns:
				if (ub.rect as Rect2).has_point(event.position):
					if bool(ub.on) and Sim.world != null:
						var okr: bool = int(Sim.world.player_recruit(int(ub.type))) > 0
						_armee_flash_ok = okr
						_armee_flash = ("⚔ %s — levée ordonnée" % String(ub.nom)) if okr else ("✗ %s — file pleine" % String(ub.nom))
						Sound.play("ui_click")
						Sim.notify_action()
						queue_redraw()
					accept_event()
					return
		if _tab == 5:
			for ch in _chips:
				if ch.rect.has_point(event.position):
					_active_mode = ch.mode
					if _map != null:
						_map.set_mode(ch.mode)
					queue_redraw()
					accept_event()
					return
		if _tab == 6:
			for b in _diplo_btns:
				if b.rect.has_point(event.position):
					open_country.emit(int(b.target))   # la fiche → la FENÊTRE d'actions du pays
					accept_event()
					return
		if _tab == 7:
			for tb in _ctab_btns:
				if (tb.rect as Rect2).has_point(event.position):
					_conseil_tab = int(tb.t)
					_scroll[7] = 0.0   # changer de sous-onglet remet le défilement en haut
					Sound.play("ui_click")
					queue_redraw()
					accept_event()
					return
			for b in _conseil_btns:
				if b.rect.has_point(event.position):
					_conseil_act(String(b.act), int(b.seat), int(b.get("slot", 0)), float(b.get("pay", 1.0)))
					accept_event()
					return
			for b in _decret_btns:
				if b.rect.has_point(event.position):
					_decret_act(int(b.id), bool(b.on))
					accept_event()
					return
			for b in _servile_btns:
				if b.rect.has_point(event.position):
					var w = Sim.world
					var me: int = w.player() if w != null else -1
					_servile_act(String(b.act), int(b.qty), me)
					accept_event()
					return

# couleur d'état de marché (BandMarche : mort · pénurie · tendu · sain · engorgé)
func _marche_col(band: int) -> Color:
	match band:
		1: return VKit.sense(0.10)   # pénurie : rouge
		2: return VKit.sense(0.40)   # tendu : ambre
		3: return VKit.sense(0.80)   # sain : vert
		4: return VKit.COL_GOLD    # engorgé : or
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
