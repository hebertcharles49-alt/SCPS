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
const DW := 312.0

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
	_servile_manumit_armed = false   # jamais une confirmation qui traverse une fermeture d'onglet
	visible = i >= 0
	queue_redraw()

func _draw() -> void:
	if _tab < 0:
		return
	_hover_zones.clear()
	VKit.panel_bg(self, Rect2(0, 0, DW, size.y))
	VKit.fill(self, Rect2(DW - 2, 0, 2, size.y), VKit.COL_GOLD)
	var x := 14.0
	var y := 10.0
	UIKit.draw_chrome(self, "panel_title_plaque", Rect2(8, 6, DW - 16, 30))
	UIKit.draw_icon(self, TAB_ICON[_tab], Vector2(x, y + 1), 22)
	VKit.text(self, Vector2(x + 28, y + 3), VKit.COL_GOLD, TAB_NAME[_tab], VKit.FS_BIG)
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
		VKit.box(self, Rect2(tx, ty, tw, 17), VKit.COL_GOLD)
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

# ── ÉCONOMIE : Budget (econ_flux) + Commerce (intertrade), read-only ───────
func _draw_eco(x: float, y: float, me: int) -> void:
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
		if shown >= 5 or y > size.y - 96:
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
		return
	for p in partners:
		if y > size.y - 18:
			break
		var col := VKit.sense(0.12) if bool(p["at_war"]) else (VKit.COL_GOLD if bool(p["embargo"]) else VKit.COL_PARCH)
		VKit.text(self, Vector2(x + 8, y), col, String(p["name"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 150, y), VKit.COL_DIM, "%d or/an" % int(p["value"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 228, y), col, String(p["status"]), VKit.FS_SMALL)
		y += 15

# ── MARCHÉ (sb_panel_marche, table des prix) : [A]cheter/[V]endre 10 sur la
#    région-capitale (verbes : player_market_buy/_sell, journalisés) ──────────
var _marche_btns := []   # [{rect, act, res_id}] boutons Acheter/Vendre
var _marche_flash := ""
var _marche_flash_ok := true
const MARCHE_QTY := 10

func _draw_marche(x: float, y: float, me: int) -> void:
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
	var cp_lbl := "Puissance comm. : %d / %d ce mois" % [int(round(cp_rem)), int(round(cp_pool))]
	if cp_bonus > 0:
		cp_lbl += "  (+%d%% édifices)" % cp_bonus
	VKit.text(self, Vector2(x, y), cp_col, cp_lbl, VKit.FS_SMALL)
	_hover_zones.append({"rect": Rect2(x - 2, y - 3, 264, 16),
		"text": "Volume de biens achetable au marché ce mois-ci (0.04/bourgeois + 0.01/élite × la chaîne commerciale) — évite de rafler tout le stock d'un coup ; pèse aussi dans ta puissance éco diplomatique."})
	y += 18
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "bien          prix(or)   marché", VKit.FS_SMALL)
	y += 16
	for st in Sim.world.country_stocks(me):
		if y > size.y - 18:
			break
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
		VKit.text(self, Vector2(x, size.y - 18),
			(VKit.sense(0.85) if _marche_flash_ok else VKit.sense(0.10)), _marche_flash, VKit.FS_SMALL)

# ── CONSEIL (sb_panel_conseil) : [Recruter]/[Renvoyer] par siège (verbes :
#    player_council_hire/_dismiss, journalisés — drainés au tick) ─────────────
var _conseil_btns := []   # [{rect, act, seat}] boutons Recruter/Renvoyer
var _conseil_flash := ""
var _conseil_flash_ok := true

func _draw_conseil(x: float, y: float, me: int) -> void:
	_conseil_btns.clear()
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
			# le ministre ASSIS : nom · tier · ÂGE (il vieillit ; la retraite vide le siège vers 66-73)
			VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH,
				"%s — tier %d · %d ans" % [seat["councilor"], int(seat["tier"]), int(seat.get("age", 0))], VKit.FS_SMALL)
			var bw := VKit.text_w("Renvoyer", VKit.FS_SMALL) + 14.0
			var r := Rect2(DW - 14.0 - bw, y - 1, bw, 16)
			VKit.fill(self, r, VKit.COL_PANEL2)
			VKit.box(self, r, VKit.sense(0.12))
			VKit.text(self, Vector2(r.position.x + 7, y), VKit.sense(0.12), "Renvoyer", VKit.FS_SMALL)
			_conseil_btns.append({"rect": r, "act": "dismiss", "seat": idx, "slot": -1})
			y += 22
			# V2a — LE CONSEIL VIVANT : faction (mot) + barre de LOYAUTÉ (rouge→vert) + mot d'ambiance
			var faction := String(seat.get("faction", ""))
			var loyalty := int(seat.get("loyalty", 0))
			var mood := String(seat.get("mood", ""))
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "Faction : %s" % faction, VKit.FS_SMALL)
			y += 15
			VKit.gauge(self, x + 16, y, DW - 32.0, 8.0, loyalty)
			y += 13
			VKit.text(self, Vector2(x + 16, y), VKit.sense(float(loyalty) / 100.0),
				"Loyauté %d — %s" % [loyalty, mood], VKit.FS_SMALL)
			y += 18
			# le curseur de PAIE (0.5×/1×/1.5×/2×) — verbe CMD_COUNCIL_PAY, journalisé
			var pay := float(seat.get("pay", 1.0))
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "Paie", VKit.FS_SMALL)
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
		else:
			VKit.text(self, Vector2(x + 16, y), VKit.COL_DIM, "(siège vacant — la pool se renouvelle par génération)", VKit.FS_SMALL)
			y += 18
			# l'embauche ÉCLAIRÉE : les CANDIDATS de la pool courante (nom · âge · ×tier · coût/mois)
			if Sim.world.has_method("council_candidates"):
				for cand in Sim.world.council_candidates(idx):
					var lab := "%s · %d ans · ×%d · %.0f or/mois" % [
						String(cand["nom"]), int(cand["age"]), int(cand["tier"]), float(cand["cost"])]
					var cw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
					var cr := Rect2(x + 16, y - 1, cw, 16)
					VKit.fill(self, cr, VKit.COL_PANEL2)
					VKit.box(self, cr, VKit.sense(0.80))
					VKit.text(self, Vector2(cr.position.x + 7, y), VKit.COL_PARCH, lab, VKit.FS_SMALL)
					_conseil_btns.append({"rect": cr, "act": "hire", "seat": idx, "slot": int(cand["slot"])})
					y += 19
			y += 4
		idx += 1
	y += 6
	y = _draw_decrets(x, y, me)
	y += 6
	y = _draw_servile(x, y, me)
	if _servile_flash != "":
		VKit.text(self, Vector2(x, size.y - 18),
			(VKit.sense(0.85) if _servile_flash_ok else VKit.sense(0.10)), _servile_flash, VKit.FS_SMALL)
	elif _decret_flash != "":
		VKit.text(self, Vector2(x, size.y - 18),
			(VKit.sense(0.85) if _decret_flash_ok else VKit.sense(0.10)), _decret_flash, VKit.FS_SMALL)
	elif _conseil_flash != "":
		VKit.text(self, Vector2(x, size.y - 18),
			(VKit.sense(0.85) if _conseil_flash_ok else VKit.sense(0.10)), _conseil_flash, VKit.FS_SMALL)

# ── DÉCRETS (sb_panel_decrets) : la flexibilité PROACTIVE du joueur (civics) —
#    section sous le Conseil (même onglet). Chaque décret DÉPLACE un levier moteur
#    tant qu'il est actif ; `plateaux` (survol) en donne les DEUX faces. Un ÉDIT se
#    bascule librement (Activer/Désactiver) ; une RÉFORME activée s'affiche VERROUILLÉE
#    (irréversible — pas de bouton retour). Grisé si `legal`==0 (condition d'entrée absente).
var _decret_btns := []   # [{rect, id, on}]
var _decret_flash := ""
var _decret_flash_ok := true

func _draw_decrets(x: float, y: float, me: int) -> float:
	_decret_btns.clear()
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Décrets", VKit.FS_BIG)
	y += 20
	if not Sim.world.has_method("decrees_list"):
		return y
	for dec in Sim.world.decrees_list(me):
		var id := int(dec["id"])
		var active := bool(dec["active"])
		var legal := bool(dec["legal"])
		var reforme := bool(dec["reforme"])
		var nom := String(dec["nom"])
		var label := nom + (" [RÉFORME]" if reforme else "")
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH if legal or active else VKit.COL_DIM, label, VKit.FS_SMALL)
		_hover_zones.append({"rect": Rect2(x, y - 2, VKit.text_w(label, VKit.FS_SMALL), 14), "text": String(dec["plateaux"])})
		y += 15
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, String(dec["flavor"]), VKit.FS_SMALL)
		y += 16
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
			y += 20
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
			Sound.play("ui_deny")
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
		Sound.play("ui_coin" if ok else "ui_deny")
	elif not ok:
		Sound.play("ui_deny")
	queue_redraw()

# ── ARMÉE (sb_panel_armee) : readouts + VERBES joueur (levée/posture/flotte) ──
const POSTURE_LABELS := ["Prudente", "Standard", "Agressive"]
const HULL_LABELS := [["+Guerre", 0], ["+Transport", 1], ["+Marchand", 2]]   # HullType : HULL_WAR·HULL_TRANSPORT·HULL_MERCHANT

var _levy_btns := []      # [{rect, delta}] boutons [-]/[+] de la jauge de levée
var _posture_btns := []   # [{rect, p}] chips de posture
var _army_btns := []      # [{rect, act}] Recompléter / Dissoudre
var _navy_btns := []      # [{rect, hull}] +Guerre / +Transport / +Marchand
var _posture_sel := 1     # dernier clic (affichage seul — aucun lecteur de posture actuelle)

func _draw_armee(x: float, y: float, me: int) -> void:
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
	if _armee_flash != "":
		VKit.text(self, Vector2(x, size.y - 18),
			(VKit.sense(0.85) if _armee_flash_ok else VKit.sense(0.10)), _armee_flash, VKit.FS_SMALL)

var _armee_flash := ""
var _armee_flash_ok := true

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
			VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
			VKit.box(self, r, VKit.COL_EDGE)
			VKit.text(self, Vector2(cx + 7, y + 1), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
			_chips.append({"rect": r, "mode": mode})
			cx += tw + 4
		y += 26

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

func _draw_diplo(x: float, y: float, me: int) -> void:
	_diplo_btns.clear()
	for rel in Sim.world.country_relations(me):
		if y > size.y - 58:
			break
		var row_y0 := y
		var target: int = int(rel["country"])
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
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "▸ cliquer : actions diplomatiques", VKit.FS_SMALL)
		y += 16
		VKit.fill(self, Rect2(x, y - 6, DW - 2.0 * x, 1), VKit.COL_EDGE)

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
	var cap_prov: int = w.country_capital_province(me)
	var cap_region: int = w.province_region(cap_prov) if cap_prov >= 0 else -1
	if cap_region < 0:
		_marche_flash_ok = false
		_marche_flash = "✗ aucune capitale — refusé"
		Sound.play("ui_deny")
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
	Sound.play("ui_coin" if ok else "ui_deny")
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
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
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
