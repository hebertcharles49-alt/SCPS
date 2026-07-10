extends Control
## SidebarDrawer — le TIROIR de la sidebar : s'ouvre à droite du rail (même bande
## que le panneau de province, mutuellement exclusifs). En-tête à plaque + icône,
## puis le contenu de l'onglet. Les 8 onglets sont PORTÉS (read-only, lus de la
## façade) : Économie (budget + commerce), Démographie, Stocks, Marché, Armée,
## Filtres (pilote la carte), Diplomatie, Conseil. Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const DX := 46.0
const DY := 102.0
const DW := 380.0   ## élargi (retour joueur 2026-07-10 : « laisse respirer, on a de la place »)

const TAB_ICON := ["menu_economy", "menu_demography", "menu_stocks", "menu_market",
	"menu_army", "menu_filters", "menu_diplomacy", "menu_council"]
const TAB_NAME := ["Économie", "Démographie", "Stocks", "Marché",
	"Armée", "Filtres", "Diplomatie", "Conseil"]

# Filtres : modes render_map offerts (culture/foi exigent des teintes → omis).
# [label, ViewMode]. Groupés comme viewer.c.
const FILT_GROUPS := [
	["Souveraineté", [["Politique", 1], ["Pays", 3], ["Régions", 2], ["Continents", 4]]],
	["Gouvernance", [["Stabilité", 13], ["Commerce", 14], ["Guerre", 15], ["Diplomatie", 16]]],
	["Terre", [["Relief", 0], ["Altitude", 5], ["Fertilité", 6], ["Humidité", 7],
		["Température", 8], ["Ressources", 9], ["Habitabilité", 10]]],
]

signal charts_requested        ## Économie → « Courbes dans le temps » : ouvre le panneau Easy Charts
signal open_country(cid: int)  ## Diplomatie → la FENÊTRE d'actions du pays cliqué

var _tab := -1
var _map                       # MapView (pour Filtres → set_mode)
var _active_mode := 0
var _chips := []               # [{rect, mode}] cliquables (Filtres)
var _chart_btn := Rect2()      # bouton « Courbes dans le temps » (onglet Économie)
var _diplo_btns := []          # [{rect, act="open", target, nom}] fiches pays cliquables (onglet Diplomatie)
var _hover_zones := []         # [{rect, text}] survols (sprites de ressource → nom)
var _hover_text := ""
var _hover_pos := Vector2.ZERO

func setup(map) -> void:
	_map = map

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true   # SCROLL générique : le contenu défilé se coupe au bord du tiroir
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: queue_redraw())
	hide()

var _hmax := 600.0   ## hauteur MAX (viewport) — la hauteur réelle épouse le contenu (latch _draw)

func _layout() -> void:
	position = Vector2(DX, DY)
	_hmax = maxf(80.0, get_viewport_rect().size.y - DY - 26.0)
	size = Vector2(DW, minf(size.y, _hmax))

func show_tab(i: int) -> void:
	_tab = i
	_hover_text = ""
	_servile_manumit_armed = false   # jamais une confirmation qui traverse une fermeture d'onglet
	visible = i >= 0
	queue_redraw()

const CONTENT_Y := 46.0   ## haut du CONTENU (sous l'en-tête fixe de 36 px + marge)

## SCROLL GÉNÉRIQUE du tiroir (motif construction_panel) : offset PAR ONGLET,
## molette, clamp au contenu, clip (clip_contents), barre piste+pouce, EN-TÊTE
## FIXE redessiné par-dessus le contenu défilé.
var _scroll := {}         ## {tab: offset px}
var _maxscroll := 0.0     ## du DERNIER _draw (pour la molette)

func _draw_header(x: float) -> void:
	# titre PLAT (la plaque de chrome ornée est retirée — lisibilité d'abord) —
	# dessiné EN DERNIER (fixe, par-dessus le contenu défilé).
	VKit.fill(self, Rect2(0, 0, DW, 36), Color(0.055, 0.042, 0.028, 0.92))
	VKit.fill(self, Rect2(0, 35, DW, 1), Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.6))
	UIKit.draw_icon(self, TAB_ICON[_tab], Vector2(x, 8), 22)
	VKit.text(self, Vector2(x + 28, 7), VKit.COL_GOLD, TAB_NAME[_tab], VKit.FS_BIG)

func _draw() -> void:
	if _tab < 0:
		return
	_hover_zones.clear()
	_tips.clear()
	VKit.panel_bg(self, Rect2(0, 0, DW, size.y))
	VKit.fill(self, Rect2(DW - 2, 0, 2, size.y), VKit.COL_GOLD)
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

	# HAUTEUR AU CONTENU (règle Stellaris : la fenêtre épouse ce qu'elle montre —
	# fini la colonne pleine hauteur aux trois quarts vide). Latch différé, borné
	# au viewport ; AU-DELÀ, le surplus DÉFILE (molette + barre).
	var want := clampf(CONTENT_Y + content_h + 12.0, 120.0, _hmax)
	if absf(want - size.y) > 0.5:
		set_deferred("size", Vector2(DW, want))
	var visible_h := want - CONTENT_Y - 12.0
	_maxscroll = maxf(0.0, content_h - visible_h)
	if off > _maxscroll:   # le contenu a rétréci sous l'offset mémorisé : re-clamp
		_scroll[_tab] = _maxscroll
		queue_redraw()

	# EN-TÊTE FIXE par-dessus le contenu défilé, puis la BARRE (piste + pouce ∝ fenêtre/contenu).
	_draw_header(x)
	if _maxscroll > 0.0:
		var track := Rect2(DW - 9.0, CONTENT_Y, 4.0, size.y - CONTENT_Y - 10.0)
		VKit.fill(self, track, VKit.COL_PANEL2)
		var thumb_h := maxf(24.0, track.size.y * visible_h / maxf(content_h, 1.0))
		var thumb_y := track.position.y + (minf(off, _maxscroll) / _maxscroll) * (track.size.y - thumb_h)
		VKit.fill(self, Rect2(track.position.x, thumb_y, 4.0, thumb_h), VKit.COL_GOLD)

	# les zones de survol passent par le TOOLTIP NATIF (→ TooltipServer : mots-concepts
	# turquoise + définitions) — les zones DÉFILÉES sous l'en-tête fixe sont écartées
	# (sinon un fantôme invisible répondrait au survol dans le bandeau de titre).
	for z in _hover_zones:
		if (z["rect"] as Rect2).get_center().y < 36.0:
			continue
		_tips.append([z["rect"], z["text"]])

# ── DÉMOGRAPHIE (sb_panel_demo, read-only) ─────────────────────────────────
func _draw_demo(x: float, y: float, me: int) -> float:
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
	return y

# ── STOCKS (sb_panel_stocks, read-only) ────────────────────────────────────
func _draw_stocks(x: float, y: float, me: int) -> float:
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          stock   net/j   couv.", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		var col := _marche_col(int(st["market_band"]))
		_res_cell(x, y, int(st["res_id"]), String(st["name"]), col)
		VKit.text(self, Vector2(x + 110, y), col, _grp(st["stock"]), VKit.FS_SMALL)
		var net: float = st["net_day"]
		VKit.text(self, Vector2(x + 165, y), col, ("%+.1f" % net) if net != 0.0 else "0.0", VKit.FS_SMALL)
		var cov: int = int(st["coverage_days"])
		var covs := ("" if cov < 0 else (">1 an" if cov >= 366 else "%d j" % cov))
		VKit.text(self, Vector2(x + 225, y), col, covs, VKit.FS_SMALL)
		y += 18
	return y

## cellule d'identité d'une ressource : le SPRITE (assets/scps/pack/resources, par
## id), sinon le nom en texte ; survol → le nom dans tous les cas.
func _res_cell(x: float, y: float, res_id: int, name: String, col: Color) -> void:
	var spr := UIKit.resource_sprite(res_id, name)
	if spr != null:
		draw_texture_rect(spr, Rect2(x, y - 3, 18, 18), false)
	else:
		VKit.text(self, Vector2(x, y), col, name, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x - 2, y - 3, 104, 18), "text": name})

# ── ÉCONOMIE : Budget (econ_flux) + Commerce (intertrade), read-only ───────
## MATIÈRES (retour joueur UI-2 : les 5 cellules de brut SORTENT de la topbar) —
## ligne compacte en tête, même source que l'ancienne topbar (country_stocks) ; le
## détail complet (net/j, couverture) vit déjà plus bas dans l'onglet STOCKS.
const _MAT_RAWS := [9, 24, 25, 13, 36]   # RES_WOOD · RES_CLAY · RES_STONE · RES_IRON · RES_ARMS
const _MAT_NAMES := {9: "bois", 24: "argile", 25: "pierre", 13: "fer", 36: "armes"}

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
	_hover_zones.append({"rect": Rect2(x - 2, y - 3, VKit.text_w(line, VKit.FS_SMALL) + 8, 16),
		"text": "Stocks nationaux de matières brutes — détail (net/jour, couverture) dans l'onglet Stocks"})
	return y + 18.0

func _draw_eco(x: float, y: float, me: int) -> float:
	y = _draw_mat_line(x, y, me)
	# bouton : les COURBES dans le temps sont DERRIÈRE ce sous-menu (pas affichées d'office)
	_chart_btn = Rect2(x, y, DW - 2.0 * x, 20.0)
	VKit.fill(self, _chart_btn, VKit.COL_PANEL2)
	VKit.box(self, _chart_btn, VKit.COL_GOLD)
	UIKit.draw_icon(self, "menu_economy", Vector2(x + 4, y + 3), 13)
	VKit.text(self, Vector2(x + 24, y + 3), VKit.COL_GOLD, "Courbes dans le temps  ▸", VKit.FS_SMALL)
	y += 28
	# — Trésor & budget de l'année (la décomposition du flux d'or) —
	var b: Dictionary = Sim.world.budget_summary(me)
	UIKit.draw_icon(self, "gold_coin", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, "Trésor : %s or" % _grp(b["gold"]))
	y += 18
	var net: float = b["net"]
	var ncol := VKit.sense(0.80) if net >= 0 else VKit.sense(0.12)
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Budget (an)", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 74, y), VKit.sense(0.80), "+%s" % _grp(b["income"]), VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 138, y), VKit.sense(0.12), "−%s" % _grp(b["expense"]), VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 206, y), ncol, "net %s%s" % ["+" if net >= 0 else "−", _grp(absf(net))], VKit.FS_SMALL)
	y += 16
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "crédit : %s or" % _grp(b["credit_line"]), VKit.FS_SMALL)
	if int(b.get("creditor", -1)) >= 0:
		VKit.text(self, Vector2(x + 140, y), VKit.sense(0.30), "dette → %s" % String(b.get("creditor_name", "")), VKit.FS_SMALL)
	y += 18
	# postes de flux (signés : revenu vert / dépense rouge) — quelques-uns
	var shown := 0
	for p in Sim.world.country_budget(me):
		if shown >= 5:   # limite de COMPTE (résumé) — le scroll gère la hauteur désormais
			break
		var amt: float = p["amount"]
		var pcol := VKit.sense(0.78) if amt >= 0 else VKit.sense(0.18)
		VKit.text(self, Vector2(x + 8, y), VKit.COL_PARCH, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), pcol, "%s%s" % ["+" if amt >= 0 else "−", _grp(absf(amt))], VKit.FS_SMALL)
		y += 14
		shown += 1
	y += 4
	VKit.fill(self, Rect2(x, y, DW - 2.0 * x, 1), VKit.COL_EDGE)
	y += 8
	# — Commerce (routes + partenaires) —
	var t: Dictionary = Sim.world.country_trade(me)
	UIKit.draw_icon(self, "menu_economy", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH,
		"%d route(s) · export %d or/an" % [int(t["routes"]), int(t["export_gold"])])
	y += 20
	var partners: Array = t["partners"]
	if partners.is_empty():
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, "(aucun partenaire)", VKit.FS_SMALL)
		return y + 16.0
	for p in partners:
		var col := VKit.sense(0.12) if bool(p["at_war"]) else (VKit.COL_GOLD if bool(p["embargo"]) else VKit.COL_PARCH)
		VKit.text(self, Vector2(x + 8, y), col, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), VKit.COL_DIM, "%d or/an" % int(p["value"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 228, y), col, String(p["status"]), VKit.FS_SMALL)
		y += 15
	return y

# ── MARCHÉ (sb_panel_marche, table des prix) : [A]cheter/[V]endre 10 sur la
#    région-capitale (verbes : player_market_buy/_sell, journalisés) ──────────
var _marche_btns := []   # [{rect, act, res_id}] boutons Acheter/Vendre
var _marche_flash := ""
var _marche_flash_ok := true
const MARCHE_QTY := 10

func _draw_marche(x: float, y: float, me: int) -> float:
	_marche_btns.clear()
	var cap_region := -1
	var cap_prov: int = Sim.world.country_capital_province(me)
	if cap_prov >= 0:
		cap_region = Sim.world.province_region(cap_prov)
	# §5 PUISSANCE COMMERCIALE : le volume achetable au marché ce mois-ci (borne les achats au marché).
	var cpow: Dictionary = Sim.world.commerce_power(me)
	var cp_pool := float(cpow.get("pool", 0.0))
	var cp_rem := float(cpow.get("remaining", 0.0))
	var cp_bonus := int(cpow.get("bonus_pct", 0))
	var cp_col := VKit.sense(clampf(cp_rem / maxf(cp_pool, 1.0), 0.0, 1.0))   # vert plein → rouge à sec
	# le « +X % édifices » vit au SURVOL (retour joueur : le chiffre inline sans contexte)
	var cp_lbl := "Puissance comm. : %d / %d ce mois" % [int(round(cp_rem)), int(round(cp_pool))]
	VKit.text(self, Vector2(x, y), cp_col, cp_lbl, VKit.FS_SMALL)
	var cp_tip := "Volume de biens achetable au marché ce mois-ci (0.04/bourgeois + 0.01/élite × la chaîne commerciale)."
	if cp_bonus > 0:
		cp_tip += "\nDont +%d %% apportés par vos édifices de commerce." % cp_bonus
	_hover_zones.append({"rect": Rect2(x - 2, y - 3, 264, 16), "text": cp_tip})
	y += 18
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          prix(or)   marché", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		var col := _marche_col(int(st["market_band"]))
		var res_id := int(st["res_id"])
		_res_cell(x, y, res_id, String(st["name"]), col)
		VKit.text(self, Vector2(x + 110, y), col, "%.2f" % float(st["price"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 178, y), VKit.COL_DIM, String(st["marche"]), VKit.FS_SMALL)
		if cap_region >= 0:
			var ra := Rect2(x + 240, y - 2, 18, 16)
			VKit.fill(self, ra, VKit.COL_PANEL2); VKit.box(self, ra, VKit.sense(0.80))
			VKit.text(self, Vector2(ra.position.x + 4, y - 1), VKit.sense(0.80), "A", VKit.FS_SMALL)
			_marche_btns.append({"rect": ra, "act": "buy", "res_id": res_id})
			var rv := Rect2(x + 262, y - 2, 18, 16)
			VKit.fill(self, rv, VKit.COL_PANEL2); VKit.box(self, rv, VKit.sense(0.12))
			VKit.text(self, Vector2(rv.position.x + 4, y - 1), VKit.sense(0.12), "V", VKit.FS_SMALL)
			_marche_btns.append({"rect": rv, "act": "sell", "res_id": res_id})
		y += 18
	if _marche_flash != "":
		y += 4
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _marche_flash_ok else VKit.sense(0.10)), _marche_flash, VKit.FS_SMALL)
		y += 16
	return y

# ── CONSEIL (sb_panel_conseil) : [Recruter]/[Renvoyer] par siège (verbes :
#    player_council_hire/_dismiss, journalisés — drainés au tick) ─────────────
var _conseil_btns := []   # [{rect, act, seat}] boutons Recruter/Renvoyer
var _conseil_flash := ""
var _conseil_flash_ok := true
var _conseil_tab := 0   ## 0 = Gouvernement (sièges) · 1 = Politiques (décrets + servile)
var _ctab_btns := []
## l'ASSIETTE des coûts % (hovers quantitatifs — « 3 % du revenu (2033 or) × IPM 1,12
## = 68 or/an ») : revenu fiscal annuel + IPM, rafraîchis à chaque _draw_conseil.
var _cons_rev := 0.0
var _cons_ipm := 1.0

func _draw_conseil(x: float, y: float, me: int) -> float:
	_conseil_btns.clear()
	if Sim.world.has_method("country_revenue_year"):
		_cons_rev = float(Sim.world.country_revenue_year(me))
		_cons_ipm = float(Sim.world.world_ipm())
	# SOUS-ONGLETS (retour joueur : « diviser l'UI statecraft pour sa lisibilité ») :
	# GOUVERNEMENT (les sièges du Conseil) / POLITIQUES (décrets + peuple servile).
	_ctab_btns.clear()
	var cxx := x
	for ti in range(2):
		var lbl: String = ["Gouvernement", "Politiques"][ti]
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
	var idx := 0
	for seat in Sim.world.country_council(me):
		var filled := bool(seat["filled"])
		# BUSTE du conseiller assis (planche 13) — sièges moteur Savoir/Société/Industrie
		# → Maître des savoirs (06) / Chancelier (01) / Intendant (04) ; fem. par hash du nom.
		var pt: Texture2D = null
		if filled:
			var pmap := [5, 0, 3]
			pt = UIKit.advisor_portrait(pmap[idx] if idx < pmap.size() else idx % 8,
				String(seat["councilor"]).hash() % 2 == 1)
		if pt != null:
			draw_texture_rect(pt, Rect2(x - 2, y - 3, 20, 20), false)
		else:
			UIKit.draw_icon(self, "menu_council", Vector2(x, y - 1), 16)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_GOLD, String(seat["seat"]))
		y += 18
		if filled:
			# CARTE SIÈGE — TERSE (retour joueur 2026-07-10, prime sur la spec doc) :
			# nom · identité (MOT seul, phrase au survol) · siège rang âge · faction ·
			# loyauté (jauge) · paie · bonus final · N or par an · retraite. CHAQUE
			# élément a un hover factuel ; les formules/taux vivent AU SURVOL seulement.
			var fname := String(seat.get("firstname", ""))
			var house := String(seat.get("house", ""))
			var pname := (fname + " " + house).strip_edges() if fname != "" else String(seat["councilor"])
			var idnom := String(seat.get("identite", ""))
			var idflav := String(seat.get("id_flavor", ""))
			var pdisp := pname + (" · " + idnom if idnom != "" else "")
			VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH, pdisp, VKit.FS_SMALL)
			if idflav != "":
				_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(pdisp, VKit.FS_SMALL) + 6, 16),
					"text": "%s — %s" % [idnom, idflav]})
			var bw := VKit.text_w("Renvoyer", VKit.FS_SMALL) + 14.0
			var r := Rect2(DW - 14.0 - bw, y - 1, bw, 16)
			VKit.fill(self, r, VKit.COL_PANEL2)
			VKit.box(self, r, VKit.sense(0.12))
			VKit.text(self, Vector2(r.position.x + 7, y), VKit.sense(0.12), "Renvoyer", VKit.FS_SMALL)
			_hover_zones.append({"rect": r, "text": "Renvoyer : rancœur +0,10 à sa faction."})
			_conseil_btns.append({"rect": r, "act": "dismiss", "seat": idx, "slot": -1})
			y += 15
			var sline := "%s · rang %d · %d ans" % [String(seat["seat"]), int(seat["tier"]), int(seat.get("age", 0))]
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, sline, VKit.FS_SMALL)
			_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(sline, VKit.FS_SMALL) + 6, 16),
				"text": "Rang %d : base du siège ×%s (I ×1 · II ×1,5 · III ×2) = bonus de rang +%.1f %%." % [
					int(seat["tier"]), ["1", "1,5", "2"][clampi(int(seat["tier"]), 1, 3) - 1], float(seat.get("rank_bonus_pct", 0.0))]})
			y += 18
			# V2a — LE CONSEIL VIVANT : faction (mot) + barre de LOYAUTÉ (rouge→vert) + mot d'ambiance
			var faction := String(seat.get("faction", ""))
			var loyalty := int(seat.get("loyalty", 0))
			var mood := String(seat.get("mood", ""))
			var fline := "Faction : %s" % faction
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, fline, VKit.FS_SMALL)
			_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(fline, VKit.FS_SMALL) + 6, 16),
				"text": "Sa faction gagne du pouvoir tant qu'il siège ; loyauté et Corruption décident de combien le bonus est réellement délivré."})
			y += 15
			VKit.gauge(self, x + 16, y, DW - 32.0, 8.0, loyalty)
			y += 13
			var lline := "Loyauté %d — %s" % [loyalty, mood]
			VKit.text(self, Vector2(x + 16, y), VKit.sense(float(loyalty) / 100.0), lline, VKit.FS_SMALL)
			_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(lline, VKit.FS_SMALL) + 6, 16),
				"text": "Loyauté %d/100 → +%.1f pts d'efficacité (15 pts à 100). Paie ×0,5 → cible −15 pts · ×2 → +30 pts." % [
					loyalty, float(loyalty) * 0.15]})
			y += 18
			# le curseur de PAIE (0.5×/1×/1.5×/2×) — verbe CMD_COUNCIL_PAY, journalisé
			var pay := float(seat.get("pay", 1.0))
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "Paie", VKit.FS_SMALL)
			_hover_zones.append({"rect": Rect2(x + 14, y - 2, 40, 16),
				"text": "Traitement ×%.1f → loyauté cible %+d pts (30 × (paie − 1))." % [pay, int(round((pay - 1.0) * 30.0))]})
			var px := x + 60.0
			for mult in [0.5, 1.0, 1.5, 2.0]:
				var lab := "%.1f×" % mult
				var lw := VKit.text_w(lab, VKit.FS_SMALL) + 10.0
				var pr := Rect2(px, y - 1, lw, 16)
				var on := absf(pay - mult) < 0.05
				VKit.fill(self, pr, VKit.COL_PANEL2 if not on else VKit.sense(0.80))
				VKit.box(self, pr, VKit.sense(0.80) if on else VKit.COL_EDGE)
				VKit.text(self, Vector2(pr.position.x + 5, y), VKit.COL_PARCH if on else VKit.COL_DIM, lab, VKit.FS_SMALL)
				_conseil_btns.append({"rect": pr, "act": "pay", "seat": idx, "slot": 0, "pay": mult})
				px += lw + 4.0
			y += 22
			# BONUS FINAL (rang × efficacité) — la DÉCOMPOSITION vit AU SURVOL, jamais à
			# l'écran (retour joueur) ; membrane : « Administration », jamais « K ».
			if seat.has("rank_bonus_pct"):
				var domain := String(seat.get("domain", ""))
				var rankp := float(seat["rank_bonus_pct"])
				var effp := float(seat["efficiency_pct"])
				var finalp := float(seat["final_bonus_pct"])
				var kpts := float(seat.get("k_admin", 0.0)) * 3.0
				var lpts := float(loyalty) * 0.15
				var cpts := float(int(seat.get("corruption_pct", 0))) * 0.35
				var bline := "%s +%.1f %%" % [domain, finalp]
				VKit.text(self, Vector2(x + 16, y), VKit.COL_GOLD, bline, VKit.FS_SMALL)
				_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(bline, VKit.FS_SMALL) + 6, 16),
					"text": "Rang : +%.1f %% · Administration : +%.1f pts · Loyauté : +%.1f pts · Corruption : −%.1f pts · Efficacité : %.1f %% ⇒ +%.1f %% net." % [
						rankp, kpts, lpts, cpts, effp, finalp]})
				y += 16
				# PRIX FUSIONNÉ : une seule ligne « N or par an » ; la formule au survol,
				# en MONTANTS (« 3 % du revenu (2033 or) × IPM 1,12 = 68 or/an »).
				var rate := float(seat.get("cost_rate_pct", 0.0))
				var cyear := float(seat.get("cost_year", 0.0))
				var cline := "%s or par an" % _grp(int(round(cyear)))
				VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
				_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
					"text": "%.1f %% du revenu (%s or) × IPM %.2f = %s or par an — prélevé chaque mois (/12)." % [
						rate, _grp(int(round(_cons_rev))), _cons_ipm, _grp(int(round(cyear)))]})
				y += 15
				var rlo := int(seat.get("retire_lo", -1))
				var rhi := int(seat.get("retire_hi", -1))
				if rlo >= 0:
					var rline := "Retraite : %d à %d ans" % [rlo, rhi]
					VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, rline, VKit.FS_SMALL)
					_hover_zones.append({"rect": Rect2(x + 14, y - 2, VKit.text_w(rline, VKit.FS_SMALL) + 6, 16),
						"text": "Départ entre 66 et 73 ans — il en a %d : le siège se libère dans %d à %d ans." % [
							int(seat.get("age", 0)), rlo, rhi]})
					y += 15
			y += 6
		else:
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "(siège vacant : la pool se renouvelle par génération)", VKit.FS_SMALL)
			y += 20
			# l'embauche ÉCLAIRÉE : les CANDIDATS de la pool courante — CARTE TERSE
			# (retour joueur 2026-07-10) : nom+identité · faction rang âge · bonus final ·
			# N or par an · retraite. Tout le reste (phrases, taux, décomposition) au SURVOL.
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
						_hover_zones.append({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cpdisp, VKit.FS_SMALL) + 6, 16),
							"text": "%s — %s" % [cidnom, cidflav]})
					y += 15
					var cfline := "Faction : %s · rang %d · %d ans" % [String(cand.get("faction", "")), int(cand["tier"]), int(cand["age"])]
					VKit.text(self, Vector2(cx, y), VKit.COL_DIM, cfline, VKit.FS_SMALL)
					_hover_zones.append({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cfline, VKit.FS_SMALL) + 6, 16),
						"text": "Rang %d : base du siège ×%s (I ×1 · II ×1,5 · III ×2). Sa faction gagnera du pouvoir s'il siège." % [
							int(cand["tier"]), ["1", "1,5", "2"][clampi(int(cand["tier"]), 1, 3) - 1]]})
					y += 15
					if cand.has("rank_bonus_pct"):
						var cdomain := String(cand.get("domain", ""))
						var crankp := float(cand["rank_bonus_pct"])
						var ceffp := float(cand["efficiency_pct"])
						var cfinalp := float(cand["final_bonus_pct"])
						# décomposition membrane-safe : Administration/Corruption depuis le SEAT
						# (mêmes valeurs pays, remplies même vacant) ; loyauté de DÉPART déduite
						# (efficacité − base 70 − Administration + Corruption — arithmétique sur
						# les lecteurs réels, aucun chiffre inventé).
						var ckpts := float(seat.get("k_admin", 0.0)) * 3.0
						var ccpts := float(int(seat.get("corruption_pct", 0))) * 0.35
						var clpts := ceffp - 70.0 - ckpts + ccpts
						var cbline := "%s +%.1f %%" % [cdomain, cfinalp]
						VKit.text(self, Vector2(cx, y), VKit.sense(0.70), cbline, VKit.FS_SMALL)
						_hover_zones.append({"rect": Rect2(cx - 2, y - 2, VKit.text_w(cbline, VKit.FS_SMALL) + 6, 16),
							"text": "Rang : +%.1f %% · Administration : +%.1f pts · Loyauté de départ : +%.1f pts · Corruption : −%.1f pts · Efficacité prévue : %.1f %% ⇒ +%.1f %% net." % [
								crankp, ckpts, clpts, ccpts, ceffp, cfinalp]})
						y += 15
						# PRIX FUSIONNÉ : « N or par an » ; la formule au survol, en MONTANTS.
						var ccyear := float(cand.get("cost_year", 0.0))
						var ccline := "%s or par an" % _grp(int(round(ccyear)))
						VKit.text(self, Vector2(cx, y), VKit.COL_DIM, ccline, VKit.FS_SMALL)
						_hover_zones.append({"rect": Rect2(cx - 2, y - 2, VKit.text_w(ccline, VKit.FS_SMALL) + 6, 16),
							"text": "%.1f %% du revenu (%s or) × IPM %.2f = %s or par an — prélevé chaque mois (/12)." % [
								float(cand.get("cost_rate_pct", 0.0)), _grp(int(round(_cons_rev))), _cons_ipm, _grp(int(round(ccyear)))]})
						y += 15
						var crlo := int(cand.get("retire_lo", -1))
						if crlo >= 0:
							var crline := "Retraite : %d à %d ans" % [crlo, int(cand.get("retire_hi", -1))]
							VKit.text(self, Vector2(cx, y), VKit.COL_DIM, crline, VKit.FS_SMALL)
							_hover_zones.append({"rect": Rect2(cx - 2, y - 2, VKit.text_w(crline, VKit.FS_SMALL) + 6, 16),
								"text": "Départ entre 66 et 73 ans — il en a %d : le siège se libérerait dans %d à %d ans." % [
									int(cand["age"]), crlo, int(cand.get("retire_hi", -1))]})
							y += 15
					else:
						# repli binding sans les champs carte : annualisé quand même (prix fusionné).
						VKit.text(self, Vector2(cx, y), VKit.COL_DIM,
							"%d ans · rang %d · %.0f or par an" % [int(cand["age"]), int(cand["tier"]), float(cand["cost"]) * 12.0], VKit.FS_SMALL)
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
	# ── MISSION DÉCENNALE (§ Interface (cartes) « MISSION ») : le siège responsable
	#    (mission_responsible_seat, moteur — successeur repris à chaque lecture) +
	#    son bonus + la récompense PRÉVUE (base × rang×efficacité). ──
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
			var rw := "Récompense prévue : %.0f or" % float(mi.get("reward_gold_adj", mi.get("reward_gold", 0)))
			var mat := String(mi.get("reward_mat", ""))
			if mat != "" and float(mi.get("reward_qty_adj", mi.get("reward_qty", 0))) > 0.0:
				rw += " + %.0f %s" % [float(mi.get("reward_qty_adj", mi.get("reward_qty", 0))), mat]
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, rw, VKit.FS_SMALL)
			y += 16
	# (Décrets + Peuple servile vivent dans le sous-onglet POLITIQUES — lot 5)
	if _conseil_flash != "":
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _conseil_flash_ok else VKit.sense(0.10)), _conseil_flash, VKit.FS_SMALL)
		y += 16
	return y

# ── DÉCRETS (sb_panel_decrets) — recâblé 2026-07-10 sur le catalogue orientations/
#    décisions (docs/CONSEIL_ORIENTATIONS_2026-07-10.md, scps_decrees.h refondu).
#    Section sous le Conseil (même onglet, sous-onglet Politiques). Chaque ORIENTATION
#    (type DCR_EDIT/REFORME/POSTURE) DÉPLACE un levier moteur tant qu'elle est active ;
#    `plateaux` (survol) donne DÉJÀ effet + clé tunable + multiplicateur en mots
#    (carte orientation). Un ÉDIT se bascule librement (Activer/Désactiver) ; une
#    RÉFORME activée s'affiche VERROUILLÉE (irréversible). Une DÉCISION (type
#    DCR_DECISION, ex. Audit des offices) est PONCTUELLE — bouton « Décréter » gaté
#    par condition d'entrée + cooldown (raison au survol), jamais de toggle on/off.
#    Grisé si `legal`==0.
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
	# nom par id — pour la note d'exclusivité (« ⊥ exclusif avec : X »), spec §
	# « Orientations LÉGÈRES » (RATIONS⊥FOYERS · CIRCULATION⊥FRONTIÈRES).
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

# CARTE ORIENTATION — TERSE (retour joueur 2026-07-10) : nom [+RÉFORME] · prix
# fusionné « N or par an » · exclusivité · bouton. L'effet (plateaux, avec clé+mult)
# ET le flavor vivent au SURVOL du nom ; la formule du prix au SURVOL du prix.
func _draw_decree_card(x: float, y: float, dec: Dictionary, names_by_id: Dictionary) -> float:
	var id := int(dec["id"])
	var active := bool(dec["active"])
	var legal := bool(dec["legal"])
	var reforme := bool(dec["reforme"])
	var nom := String(dec["nom"])
	var label := nom + (" [RÉFORME]" if reforme else "") + (" — actif" if active else "")
	VKit.text(self, Vector2(x, y), (VKit.sense(0.80) if active else (VKit.COL_PARCH if legal else VKit.COL_DIM)), label, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x, y - 2, VKit.text_w(label, VKit.FS_SMALL), 14),
		"text": String(dec["plateaux"]) + "\n" + String(dec["flavor"])})
	y += 15
	# PRIX FUSIONNÉ : « N or par an » seul ; la formule (taux × revenu × IPM) au survol.
	var rate := float(dec.get("cost_rate_pct", 0.0))
	var cyear := float(dec.get("cost_year", 0.0))
	var cline := ("%s or par an" % _grp(int(round(cyear)))) if rate > 0.0 else "0 or par an"
	VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
	if rate > 0.0:
		_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
			"text": "%.2f %% du revenu (%s or) × IPM %.2f = %s or par an — prélevé chaque mois (/12) ; mois impayé ⇒ sans effet ce mois-là." % [
				rate, _grp(int(round(_cons_rev))), _cons_ipm, _grp(int(round(cyear)))]})
	else:
		_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
			"text": "Aucun prélèvement d'or — la contrepartie est dans l'effet (survolez le nom)."})
	y += 15
	var excl := int(dec.get("exclusive_id", -1))
	if excl >= 0 and names_by_id.has(excl):
		var eline := "⊥ exclusif avec : %s" % String(names_by_id[excl])
		VKit.text(self, Vector2(x + 8, y), VKit.sense(0.35), eline, VKit.FS_SMALL)
		_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(eline, VKit.FS_SMALL) + 6, 16),
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
			_hover_zones.append({"rect": r, "text": "condition d'entrée non remplie"})
		y += 20
	y += 4
	return y

# CARTE DÉCISION (ponctuelle) — TERSE : nom · condition · cooldown éventuel ·
# « N or (une fois) » · bouton « Décréter ». Effet+flavor au survol du nom ;
# la formule du coût au survol du prix ; jamais de toggle.
func _draw_decision_card(x: float, y: float, dec: Dictionary, me: int) -> float:
	var id := int(dec["id"])
	var legal := bool(dec["legal"])
	var cond_met := bool(dec.get("cond_met", legal))
	var cooldown := bool(dec.get("cooldown_active", false))
	var nom := String(dec["nom"])
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH if legal else VKit.COL_DIM, nom, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x, y - 2, VKit.text_w(nom, VKit.FS_SMALL), 14),
		"text": String(dec["plateaux"]) + "\n" + String(dec["flavor"])})
	y += 15
	var cnd := "Condition : %s" % ("remplie" if cond_met else "non remplie")
	VKit.text(self, Vector2(x + 8, y), VKit.sense(0.70 if cond_met else 0.15), cnd, VKit.FS_SMALL)
	# hover QUANTITATIF : la Corruption courante contre le seuil (Audit — seule décision
	# du catalogue ; lecteur country_factions déjà bindé, aucun chiffre inventé).
	var corr_now := -1
	if Sim.world.has_method("country_factions"):
		corr_now = int(Sim.world.country_factions(me).get("corruption", -1))
	if corr_now >= 0:
		_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(cnd, VKit.FS_SMALL) + 6, 16),
			"text": "Corruption %d/100 — exige ≥ 20." % corr_now})
	y += 15
	if cooldown:
		# le nombre EXACT de jours restants n'est pas exposé par la façade (accumulateur
		# privé à scps_decrees.c) — on affiche l'état SANS fabriquer de compte à rebours.
		var cdl := "Cooldown : en cours"
		VKit.text(self, Vector2(x + 8, y), VKit.sense(0.30), cdl, VKit.FS_SMALL)
		_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(cdl, VKit.FS_SMALL) + 6, 16),
			"text": "5 ans entre deux audits — réutilisable à la fin du délai."})
		y += 15
	# PRIX FUSIONNÉ : « N or (une fois) » ; la formule au survol, en MONTANTS.
	var rate := float(dec.get("cost_rate_pct", 0.0))
	var cyear := float(dec.get("cost_year", 0.0))
	var cline := "%s or (une fois)" % _grp(int(round(cyear)))
	VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, cline, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x + 6, y - 2, VKit.text_w(cline, VKit.FS_SMALL) + 6, 16),
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
		_hover_zones.append({"rect": r, "text": reason})
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

# ── PEUPLE SERVILE (V3, câblage servile) : section sous Décrets (même onglet
#    Conseil — c'est une politique intérieure, comme un décret). Le compte d'âmes
#    (scps_manumit_preview), le POOL des Centres par héritage + prix courant
#    (scps_slave_market), ACHETER/VENDRE (quantités 50/200, gate abolitionniste
#    grisé AVEC LE MOT), et AFFRANCHIR avec APERÇU AVANT + confirmation 2 clics
#    (pas de misclick sur un verbe irréversible).
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
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
		"âmes serviles : %s (%.1f%% du pays)" % [_grp(souls), float(mp.get("pct_of_country", 0.0))], VKit.FS_SMALL)
	y += 18

	# le POOL des Centres, par héritage, avec le compte courant.
	if w.has_method("slave_market"):
		var mk: Dictionary = w.slave_market()
		var can_buy := bool(mk.get("can_buy", false))
		VKit.text(self, Vector2(x, y), VKit.COL_DIM,
			"marché mondial : %s âme(s)" % _grp(int(mk.get("total", 0))), VKit.FS_SMALL)
		y += 15
		# lot M — le SPREAD que le drain débite (achat ×2 double taxe / vente ×1), jamais montré
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

		# ACHETER / VENDRE — quantités 50/200 ; achat grisé + MOT si non-abolitionniste-eligible.
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
				_hover_zones.append({"rect": rb, "text": "gate éthos/tech — un pays abolitionniste ne peut pas acheter"})

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
	# AFFRANCHIR — l'APERÇU avant + confirmation 2 clics (verbe irréversible).
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
		var cap_prov: int = w.country_capital_province(me)
		var cap_region: int = w.province_region(cap_prov) if cap_prov >= 0 else -1
		if cap_region < 0:
			_servile_flash_ok = false
			_servile_flash = "✗ aucune capitale — refusé"
			Sound.play("ui_click")
			queue_redraw()
			return
		is_trade = true
		if act == "buy":
			ok = bool(w.player_slave_buy(cap_region, qty))
			label = "achat"
		else:
			ok = bool(w.player_slave_sell(cap_region, qty))
			label = "vente"
	_servile_flash_ok = ok
	_servile_flash = ("⚑ %s — ordre émis" % label) if ok else ("✗ %s — refusé" % label)
	if is_trade:
		Sound.play("ui_click")
	elif not ok:
		Sound.play("ui_click")
	queue_redraw()

# ── ARMÉE (sb_panel_armee) : readouts + VERBES joueur (levée/posture/flotte) ──
const POSTURE_LABELS := ["Prudente", "Standard", "Agressive"]
const HULL_LABELS := [["+Guerre", 0], ["+Transport", 1], ["+Marchand", 2]]   # HullType : HULL_WAR·HULL_TRANSPORT·HULL_MERCHANT

var _levy_btns := []      # [{rect, delta}] boutons [-]/[+] de la jauge de levée
var _posture_btns := []   # [{rect, p}] chips de posture
var _army_btns := []      # [{rect, act}] Recompléter / Dissoudre
var _navy_btns := []      # [{rect, hull}] +Guerre / +Transport / +Marchand
var _posture_sel := 1     # dernier clic (affichage seul — aucun lecteur de posture actuelle)

func _draw_armee(x: float, y: float, me: int) -> float:
	_levy_btns.clear(); _posture_btns.clear(); _army_btns.clear(); _navy_btns.clear()
	var a: Dictionary = Sim.world.country_army(me)
	UIKit.draw_icon(self, "menu_army", Vector2(x, y - 1), 18)
	VKit.text(self, Vector2(x + 22, y), VKit.COL_PARCH, "force mobilisée : %d régiments" % int(a["regiments"]))
	y += 22
	# — Levée : [-] nom [+] (verbe : player_set_levy, journalisé — drainé au tick) —
	var levy: int = int(a["levy"])
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "levée :")
	var bx := x + 52.0
	var rm := Rect2(bx, y - 2, 16, 16)
	VKit.fill(self, rm, VKit.COL_PANEL2); VKit.box(self, rm, VKit.COL_EDGE if levy <= 0 else VKit.COL_GOLD)
	VKit.text(self, Vector2(bx + 5, y - 1), VKit.COL_DIM if levy <= 0 else VKit.COL_GOLD, "−", VKit.FS_SMALL)
	if levy > 0:
		_levy_btns.append({"rect": rm, "delta": -1})
	bx += 20
	var lw := VKit.text_w(String(a["levy_name"]), VKit.FS) + 8.0
	VKit.text(self, Vector2(bx, y), VKit.COL_GOLD, String(a["levy_name"]))
	bx += lw + 4
	var rp := Rect2(bx, y - 2, 16, 16)
	VKit.fill(self, rp, VKit.COL_PANEL2); VKit.box(self, rp, VKit.COL_EDGE if levy >= 3 else VKit.COL_GOLD)
	VKit.text(self, Vector2(bx + 5, y - 1), VKit.COL_DIM if levy >= 3 else VKit.COL_GOLD, "+", VKit.FS_SMALL)
	if levy < 3:
		_levy_btns.append({"rect": rp, "delta": 1})
	y += 24
	var ar: Dictionary = Sim.world.army_info(me)
	if bool(ar.get("active", false)):
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD,
			"armée de campagne — région %d · %s" % [int(ar["region"]), ar["phase"]], VKit.FS_SMALL)
		y += 16
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
			"inf %d · arch %d · cav %d · mages %d  (Σ %d)" % [
				int(ar["inf"]), int(ar["arch"]), int(ar["cav"]), int(ar["mages"]), int(ar["units"])], VKit.FS_SMALL)
		y += 20
	else:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(pas d'armée de campagne déployée)", VKit.FS_SMALL)
		y += 20
	# — Posture : 3 chips (verbe : player_posture) — surbrillance = l'état MOTEUR (reader v49) —
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "posture :", VKit.FS_SMALL)
	y += 16
	var post_now: int = int(a.get("posture", _posture_sel))
	var cx := x
	for p in range(POSTURE_LABELS.size()):
		var label: String = POSTURE_LABELS[p]
		var tw := VKit.text_w(label, VKit.FS_SMALL) + 12.0
		var active := (post_now == p)
		var r := Rect2(cx, y, tw, 18)
		VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		VKit.text(self, Vector2(cx + 6, y + 1), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
		_posture_btns.append({"rect": r, "p": p})
		cx += tw + 4
	y += 26
	# — Recompléter / Dissoudre (verbes : player_refill / player_disband) —
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
	UIKit.draw_icon(self, "harbor_anchor", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_DIM, "Flotte : %d coque(s)" % int(a["fleet"]))
	y += 20
	# — Flotte : mise en chantier (verbe : player_navy_build) — bateau gravé par coque
	var hull_boat := ["sheet24_topbar_boats_menu_11", "sheet24_topbar_boats_menu_13", "sheet24_topbar_boats_menu_10"]
	cx = x
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

	# ── COMPOSER L'ARMÉE (retour joueur : le RECRUTEMENT vit ICI, pas dans l'UI
	#    province — l'armée est NATIONALE). Grille de tuiles du roster ; tuile grisée
	#    = verrouillée (tech/éthos) ; clic = player_recruit (journalisé). ──
	_unit_btns.clear()
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Composer l'armée", VKit.FS_SMALL)
	y += 18
	var units: Array = Sim.world.unit_roster(me)
	var ucell := 40.0
	var ucols := int((DW - 2.0 * x) / ucell)
	for i in range(units.size()):
		var u: Dictionary = units[i]
		var on: bool = bool(u.get("recrutable", false))
		var ur := Rect2(x + (i % ucols) * ucell, y + (i / ucols) * ucell, ucell - 4.0, ucell - 4.0)
		var ut: Texture2D = UIKit.unit_sprite(int(u.get("type", -1)))
		if ut != null:
			draw_texture_rect(ut, ur, false, Color.WHITE if on else Color(0.5, 0.5, 0.55, 0.6))
		else:
			VKit.fill(self, ur, VKit.COL_PANEL2)
			VKit.text(self, ur.position + Vector2(2, 10), VKit.COL_DIM, String(u.get("nom", "")).substr(0, 5), VKit.FS_SMALL)
		VKit.box(self, ur, VKit.COL_GOLD if on else VKit.COL_EDGE)
		if not on:
			VKit.text(self, ur.position + Vector2(ur.size.x - 12, 0), VKit.COL_GOLD, "✦", VKit.FS_SMALL)
		_unit_btns.append({"rect": ur, "type": int(u.get("type", -1)), "nom": String(u.get("nom", "")), "on": on})
		_tips.append([ur, "%s — %s · %s\nCoût : %s · Entretien : %.1f or/100" % [
			String(u.get("nom", "")), String(u.get("classe", "")), String(u.get("arme", "")),
			String(u.get("cout", "")), float(u.get("entretien_or10", 5)) / 10.0]])
	y += ceilf(units.size() / float(ucols)) * ucell + 4.0

	if _armee_flash != "":
		VKit.text(self, Vector2(x, y),
			(VKit.sense(0.85) if _armee_flash_ok else VKit.sense(0.10)), _armee_flash, VKit.FS_SMALL)
		y += 16
	return y

var _armee_flash := ""
var _armee_flash_ok := true
var _unit_btns := []   ## composeur d'armée : [{rect, type, nom, on}]
var _tips: Array = []  ## [[Rect2, texte], …] — hover générique du tiroir (reconstruit au _draw)

func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""

# ── FILTRES (sb_panel_filtres) : sélecteur de mode carte, FONCTIONNEL ──────
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

# ── DIPLOMATIE (sb_panel_diplo) : la LISTE-RÉSUMÉ, SANS boutons — chaque pays connu :
# nom + statut, BARRE D'OPINION (±100), le POURQUOI (composantes) et la MÉMOIRE datée.
# Les ACTIONS vivent dans la FENÊTRE PAR PAYS (country_actions) : CLIC sur la ligne
# (ou clic droit carte) → elle s'ouvre. (Le brouillard de guerre limitera la liste.)
## le JOURNAL D'ACTES (DiplogAct moteur) : [libellé quand LUI agit, libellé quand NOUS
## agissons, hostile?] — la sous-détaille datée de « Mémoire ».
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
	# UN SEUL hint en tête (il était répété sous CHAQUE pays — bruit, capture 2026-07-09)
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "▸ cliquer une fiche : actions diplomatiques", VKit.FS_SMALL)
	y += 18
	for rel in Sim.world.country_relations(me):
		var target: int = int(rel["country"])
		# BROUILLARD (retour joueur : « vision diplomatique complète alors qu'on a un
		# fog ») : un pays JAMAIS DÉCOUVERT n'existe pas dans la liste.
		if Sim.world.has_method("country_known") and int(Sim.world.country_known(target)) == 0:
			continue
		var row_y0 := y
		var at_war: bool = bool(rel["at_war"])
		var allied: bool = bool(rel["allied"])
		var col := VKit.sense(0.12) if at_war else (VKit.sense(0.78) if allied else VKit.COL_PARCH)
		# ligne 1 : nom + statut
		VKit.text(self, Vector2(x, y), col, String(rel["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), VKit.COL_DIM, String(rel["status"]), VKit.FS_SMALL)
		y += 14
		# ligne 2 : opinion ±100 (ce que CE pays pense de nous)
		var op: int = int(rel["opinion"])
		_opinion_bar(x, y, 150.0, op)
		VKit.text(self, Vector2(x + 158, y - 3), _opinion_col(op), "%+d" % op, VKit.FS_SMALL)
		y += 14
		# ligne 2bis : le RÉSUMÉ — POURQUOI (les composantes vers lesquelles l'opinion
		# converge : statuts actifs · rancune territoriale · MÉMOIRE des actes — trahison,
		# sécession d'une guerre civile). Seules les composantes NON NULLES s'affichent.
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
		# ligne 2ter : le JOURNAL — chaque acte DATÉ (« mémoire » sous-détaillée) : les
		# 3 plus récents ; un acte de MÉMOIRE (trahison, sécession) porte son poids
		# RESTANT (décayé) — « s'estompe » quand il a déjà fondu.
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
		# PAS de boutons ici : toute la FICHE est cliquable → la fenêtre d'actions du pays
		var row_rect := Rect2(x - 4.0, row_y0 - 2.0, DW - 2.0 * x + 8.0, (y - row_y0) + 4.0)
		_diplo_btns.append({"rect": row_rect, "act": "open", "target": target, "nom": String(rel["name"])})
		y += 6
		VKit.fill(self, Rect2(x, y - 3, DW - 2.0 * x, 1), VKit.COL_EDGE)
	return y

## barre d'opinion ±100 : repère central (zéro), remplissage vert (favorable) ou
## rouge (hostile) depuis le centre.
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

## Armée : levée [-]/[+], posture, recompléter/dissoudre, mise en chantier de coque —
## verbes journalisés (drainés au tick), aucun n'échoue localement sauf navy_build.
func _armee_act(kind: String, val: int) -> void:
	var w = Sim.world
	if w == null:
		return
	Sim.notify_action()   # pause : l'UI se rafraîchit au clic (le drain suit à la reprise)
	match kind:
		"levy":
			w.player_set_levy(val)
			_armee_flash_ok = true
			_armee_flash = "⚑ levée réglée — ordre émis"
		"posture":
			_posture_sel = val
			var ok: bool = w.player_posture(val)
			_armee_flash_ok = ok
			_armee_flash = ("⚑ posture %s — ordre émis" % POSTURE_LABELS[val]) if ok else "✗ posture — refusé"
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

## Conseil : recruter (siège vacant, slot 0) / renvoyer (siège pourvu) / payer
## (curseur 0.5×..2×, V2a) — verbes journalisés.
func _conseil_act(act: String, seat: int, slot: int, pay: float = 1.0) -> void:
	var w = Sim.world
	if w == null:
		return
	var ok := false
	var label := "recrutement"
	if act == "hire":
		ok = w.player_council_hire(seat, slot)   # le CANDIDAT choisi (embauche éclairée)
	elif act == "pay":
		ok = w.player_council_pay(seat, pay)
		label = "paie"
	else:
		ok = w.player_council_dismiss(seat)
		label = "renvoi"
	_conseil_flash_ok = ok
	_conseil_flash = ("⚑ %s — ordre émis" % label) if ok else ("✗ %s — refusé" % label)
	queue_redraw()

func _gui_input(event: InputEvent) -> void:
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
		if _tab == 0 and _chart_btn.has_point(event.position):   # Économie → ouvre les courbes
			charts_requested.emit()
			accept_event()
			return
		if _tab == 3:
			for b in _marche_btns:
				if b.rect.has_point(event.position):
					var w = Sim.world
					var me: int = w.player() if w != null else -1
					_marche_act(String(b.act), int(b.res_id), me)
					accept_event()
					return
		if _tab == 4:
			for b in _levy_btns:
				if b.rect.has_point(event.position):
					var a: Dictionary = Sim.world.country_army(Sim.world.player())
					_armee_act("levy", clampi(int(a["levy"]) + int(b.delta), 0, 3))
					accept_event()
					return
			for b in _posture_btns:
				if b.rect.has_point(event.position):
					_armee_act("posture", int(b.p))
					accept_event()
					return
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
			# COMPOSEUR D'ARMÉE : clic tuile = levée (journalisée) — lot 5
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
