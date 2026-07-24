extends Control
## Topbar plein-largeur : blocs ROYAUME · ÉCONOMIE · POLITIQUE · TEMPS ; display-only sauf le verbe vitesse. Lit Sim.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")
const H := Frame.TOPBAR_H

signal tech_requested
signal navigate_requested(request: Dictionary)

var _speed_rect := Rect2()
var _speed_btns := []   ## [[Rect2, index], …]
var _savoir_rect := Rect2()
var _nav_zones: Array = []     ## [{rect, request, hint}]

## photo mensuelle → deltas or/pop (display-only).
var _last_gold := 0.0
var _last_pop := 0
var _d_gold := 0.0
var _d_pop := 0

## indice des prix national — photo mensuelle (display-only).
var _last_price_lvl := 1.0
var _d_price_lvl := 0.0

## part de chaque faction (par nom) au roulement du mois → delta « +x/mois » (display-only).
var _fac_last := {}
var _fac_d := {}

func _delta_txt(d: float) -> String:
	if absf(d) < 0.5:
		return ""
	return "%+d" % int(round(d))

## STOCK + FLUX MENSUEL par nom (net_day×30). {} si la ressource n'est pas listée.
func _res_pair(w, me: int, rname: String) -> Dictionary:
	if not w.has_method("country_stocks"):
		return {}
	for st in w.country_stocks(me):
		if String(st.get("name", "")) == rname:
			return {"stock": int(st.get("stock", 0)), "permo": float(st.get("net_day", 0.0)) * 30.0}
	return {}

## une CELLULE de matière par nom : sprite · stock · delta « +N/mois » · hover nommé.
func _matter_cell(px: float, w, me: int, rname: String) -> float:
	var rp := _res_pair(w, me, rname)
	if rp.is_empty():
		return _cell(px, "", "", "0", "", true, rname, VKit.COL_DIM, rname)
	var permo := float(rp["permo"])
	var tip := rname
	if absf(permo) >= 0.5:
		tip += " %+d/mois" % int(round(permo))
	var delta := "%+d/mois" % int(round(permo))
	return _cell(px, "", "", _grp(int(rp["stock"])), delta, permo >= 0.0,
		tip, Color(0, 0, 0, 0), rname)

## LOYAUTÉ = moyenne de la loyauté des sièges du conseil (0-100) ; -1 si pas de conseil.
func _council_loyalty(w, me: int) -> int:
	if not w.has_method("country_council"):
		return -1
	var sum := 0.0
	var n := 0
	for s in w.country_council(me):
		if bool(s.get("filled", false)):
			sum += float(s.get("loyalty", 0))
			n += 1
	return int(round(sum / float(n))) if n > 0 else -1

## LA PIRE PÉNURIE : le plus petit coverage_days ≥0 (≤366 = sentinel « >1 an »/pas de pénurie=-1).
## Static : consommée aussi par alerts.gd (même calcul, DRY).
static func worst_shortage(w, me: int) -> Dictionary:
	if w == null or me < 0:
		return {}
	if w.has_method("country_shortages"):
		# binding scps_sim_node.cpp:1474 — [{nom, res_id, runway_days, structurel}], trié urgence croissante
		var sh = w.country_shortages(me)
		if sh is Array:
			var worst := {}
			var worst_days := 1 << 30
			for s in sh:
				var dj := int(s.get("runway_days", s.get("days", -1)))
				# > 1 an = pas une pénurie à AFFICHER (même sentinel que coverage_days)
				if dj >= 0 and dj <= 366 and dj < worst_days:
					worst_days = dj
					worst = {"name": String(s.get("nom", s.get("name", "Ressource"))), "days": dj}
			return worst
		return {}
	if w.has_method("country_stocks"):
		var worst_name := ""
		var worst_days := 1 << 30
		for st in w.country_stocks(me):
			var cov := int(st.get("coverage_days", -1))
			if cov >= 0 and cov <= 366 and cov < worst_days:
				worst_days = cov
				worst_name = String(st.get("name", "Ressource"))
		if worst_name != "":
			return {"name": worst_name, "days": worst_days}
	return {}

## LES POSTES de l'or (country_budget → FX_*) en TAUX MENSUEL : l'accu est RAZ chaque
## année ⇒ « /mois » = total-à-ce-jour ÷ jours-écoulés × 30.
func _budget_parts_txt(w, me: int, doy: int) -> String:
	if not w.has_method("country_budget"):
		return ""
	var parts := []
	for p in w.country_budget(me):
		var amt: float = float(p.get("amount", 0.0)) / float(doy) * 30.0
		if absf(amt) < 0.5:
			continue
		parts.append("%s %+d" % [String(p.get("name", "")), int(round(amt))])
	return " · ".join(parts)

func _treasury_tip(w, me: int) -> String:
	var doy := 1
	if w.has_method("day_of_year"):
		doy = maxi(1, int(w.day_of_year()))
	var net_m := 0.0
	if w.has_method("budget_summary"):
		net_m = float((w.budget_summary(me) as Dictionary).get("net", 0.0)) / float(doy) * 30.0
	var tip := "Trésor %+d/mois" % int(round(net_m))
	var parts := _budget_parts_txt(w, me, doy)
	if parts != "":
		tip += "\n" + parts
	return tip

func _treasury_card(w, me: int, gold: float) -> Dictionary:
	var doy := maxi(1, int(w.day_of_year())) if w.has_method("day_of_year") else 1
	var budget: Dictionary = w.budget_summary(me) if w.has_method("budget_summary") else {}
	var net_month := float(budget.get("monthly_net", float(budget.get("net", 0.0)) / float(doy) * 30.0))
	var income_lines := []
	var expense_lines := []
	if w.has_method("country_budget"):
		for p in w.country_budget(me):
			var amount := float(p.get("amount", 0.0)) / float(doy) * 30.0
			if absf(amount) >= 0.5:
				var line := {"label": String(p.get("name", "Poste")),
					"value": "%+d / mois" % int(round(amount)),
					"tone": "positive" if amount >= 0.0 else "negative"}
				if amount >= 0.0:
					income_lines.append(line)
				else:
					expense_lines.append(line)
	var lines := [{"label": "Revenus", "value": "%+d / mois" % int(round(
		float(budget.get("monthly_income", float(budget.get("income", 0.0)) / float(doy) * 30.0)))),
		"tone": "heading"}]
	lines.append_array(income_lines)
	lines.append({"label": "Dépenses", "value": "−%d / mois" % int(round(
		float(budget.get("monthly_expense", float(budget.get("expense", 0.0)) / float(doy) * 30.0)))),
		"tone": "heading"})
	lines.append_array(expense_lines)
	lines.append({"label": "Ligne de crédit", "value": "%d or" % int(round(
		float(budget.get("credit_line", 0.0)))), "tone": "dim"})
	lines.append({"label": "Fin d'année (projection)", "value": "%s or" % _grp(int(round(
		float(budget.get("projected_year_end", gold))))),
		"tone": "positive" if float(budget.get("projected_year_end", gold)) >= 0.0 else "negative"})
	var runway := float(budget.get("runway_months", -1.0))
	lines.append({"label": "Autonomie trésor + crédit", "value": "solde stable" if runway < 0.0 else \
		("< 1 mois" if runway < 1.0 else "%.1f mois" % runway),
		"tone": "dim" if runway < 0.0 else ("negative" if runway < 6.0 else "")})
	return {
		"title": "Trésor",
		"state": "%s or disponibles" % _grp(int(round(gold))),
		"trend": "%+d / mois" % int(round(net_month)),
		"trend_tone": "positive" if net_month >= 0.0 else "negative",
		"lines": lines,
	}

func _faction_card(fe: Dictionary, fx: Dictionary, monthly_delta: int) -> Dictionary:
	var policy := int(fe.get("policy_delta", 0))
	var lines: Array = [
		{"label": "Assise sociale", "value": "%d %%" % int(fe.get("base_part", 0))},
		{"label": "Effet des politiques", "value": "%+d points" % policy,
			"tone": "positive" if policy > 0 else ("negative" if policy < 0 else "dim")},
		{"label": "Rancœur", "value": "%d / 100" % int(fe.get("grief", 0)),
			"tone": "negative" if int(fe.get("grief", 0)) >= 40 else "dim"},
		{"label": "Pression de coup", "value": "%d / 100" % int(fe.get("coup_pressure", 0)),
			"tone": "negative" if bool(fe.get("coup_driver", false)) else "dim"},
		{"label": "Corruption nationale", "value": "%d / 100" % int(fx.get("corruption", 0))},
	]
	if bool(fe.get("captor", false)):
		lines.append({"label": "Capture de l'État", "value": "faction la plus favorisée", "tone": "negative"})
	return {
		"title": String(fe.get("name", "Faction")),
		"state": "%d %% de soutien%s" % [int(fe.get("part", 0)), " · dominante" if bool(fe.get("dominant", false)) else ""],
		"trend": "%+d / mois" % monthly_delta if monthly_delta != 0 else "stable ce mois",
		"trend_tone": "positive" if monthly_delta > 0 else ("negative" if monthly_delta < 0 else "dim"),
		"lines": lines,
	}

const _FOOD_NAMES := ["Céréales", "Poisson", "Bétail", "Fruits"]
func _food_tip(w, me: int) -> String:
	if not w.has_method("country_stocks"):
		return "Grenier"
	var parts := []
	for st in w.country_stocks(me):
		var nm := String(st.get("name", ""))
		if _FOOD_NAMES.has(nm):
			parts.append("%s %+d/mois" % [nm, int(round(float(st.get("net_day", 0.0)) * 30.0))])
	if parts.is_empty():
		return "Grenier"
	return "Grenier — " + " · ".join(parts)

## MATÉRIAUX — un onglet : le TOTAL bois+argile+pierre ; hover = détail par matière (chiffres mensuels).
func _materials_cell(px: float, w, me: int) -> float:
	var total := 0
	var permo_total := 0.0
	var parts := PackedStringArray()
	for rname in ["Bois", "Argile", "Pierre"]:
		var rp := _res_pair(w, me, rname)
		var stock := int(rp.get("stock", 0))
		var permo := float(rp.get("permo", 0.0))
		total += stock
		permo_total += permo
		var line := "%s %s" % [rname, _grp(stock)]
		if absf(permo) >= 0.5:
			line += " (%+d/mois)" % int(round(permo))
		parts.append(line)
	return _cell(px, "action_build", "", _grp(total), "%+d/mois" % int(round(permo_total)), permo_total >= 0.0,
		"Matériaux — " + " · ".join(parts))

func _research_tip(w, me: int) -> String:
	if not w.has_method("country_research_income"):
		return "Recherche"
	var ri: Dictionary = w.country_research_income(me)
	var perm := int(round(float(ri.get("per_day", 0.0)) * 30.0))
	var popm := int(round(float(ri.get("pop_daily", 0.0)) * 30.0))
	var parts := PackedStringArray()
	parts.append("Pops +%d" % popm)
	var ym := float(ri.get("yield_mult", 1.0))
	if absf(ym - 1.0) >= 0.05:
		parts.append("Institutions ×%.1f" % ym)
	var am := float(ri.get("age_mult", 1.0))
	if am > 1.005:
		parts.append("Lumière ×%.1f" % am)
	var mp := int(ri.get("metab_pct", 0))
	if mp > 0:
		parts.append("Métabolisation +%d%%" % mp)
	return "Recherche +%d/mois\n%s\n(clic : arbre de tech)" % [perm, " · ".join(parts)]

## SÉPARATEUR DE BLOC — l'unique filet or vertical de la barre.
func _block_sep(px: float) -> float:
	px += 10.0
	VKit.fill(self, Rect2(px, 13.0, 1.0, H - 26.0),
		Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.34))
	return px + 1.0 + 12.0

## formate « Mot NN[ ▲|▼] » pour les tooltips composites de jauge 0-100.
func _gauge_line(ci: Dictionary, cptips: Dictionary, key: String) -> String:
	var gv := int(ci.get(key, 0))
	var glyph := ""
	if gv >= 66:
		glyph = " ▲"
	elif gv <= 33:
		glyph = " ▼"
	return "%s %d%s" % [String(cptips.get(key, key)), gv, glyph]

## CELLULE : icône 32px OU rid≥0 = sprite ressource ; valeur empilée sur delta ; tip = hover ; vcol.a>0 teinte la valeur.
func _cell(px: float, icon: String, rid_or_val, val: String, dtxt: String, dpos: bool,
		tip: String = "", vcol: Color = Color(0, 0, 0, 0), rname: String = "") -> float:
	var rid := -1
	if icon == "" and typeof(rid_or_val) == TYPE_INT:
		rid = int(rid_or_val)
	if rname != "":
		# ressource par NOM : resource_sprite(-1, nom) — même sprite que le tiroir Stocks
		var rspr: Texture2D = UIKit.resource_sprite(-1, rname)
		if rspr != null:
			draw_texture_rect(rspr, Rect2(px, (H - 32.0) * 0.5, 32, 32), false)
	elif rid >= 0:
		var spr: Texture2D = UIKit.resource_sprite(rid, "")
		if spr != null:
			draw_texture_rect(spr, Rect2(px, (H - 32.0) * 0.5, 32, 32), false)
	elif icon != "":
		UIKit.draw_icon(self, icon, Vector2(px, (H - 32.0) * 0.5), 32)
	var tx := px + 36.0
	# vcol explicite (sense bon/mauvais) reste PRIORITAIRE sur COL_VALUE
	if vcol.a > 0.0:
		VKit.text(self, Vector2(tx, 6.0), vcol, val)
	else:
		VKit.value(self, Vector2(tx, 6.0), val)
	var wv := VKit.text_w(val)
	var wd := 0.0
	if dtxt != "":
		VKit.text(self, Vector2(tx, 26.0), VKit.sense(0.85) if dpos else VKit.sense(0.12), dtxt, VKit.FS_SMALL)
		wd = VKit.text_w(dtxt, VKit.FS_SMALL)
	var cw := 36.0 + maxf(wv, wd) + 10.0
	if tip != "":
		_tips.append([Rect2(px - 4.0, 0.0, cw + 8.0, H), tip])
	return px + cw + 14.0

var _tips: Array = []   ## [[Rect2, texte], …]

func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			var tip := String(t[1])
			for z in _nav_zones:
				if (z["rect"] as Rect2).has_point(at_position):
					return tip + "\nClic : " + String(z.get("hint", "ouvrir le détail"))
			return tip
	return ""

func _add_nav(rect: Rect2, request: Dictionary, hint: String, card: Dictionary = {}) -> void:
	_nav_zones.append({"rect": rect, "request": request, "hint": hint, "card": card})

## Contrat lu optionnellement par TooltipServer (zone sans carte = tooltip texte).
func get_info_card(at_position: Vector2) -> Dictionary:
	for z in _nav_zones:
		if not (z["rect"] as Rect2).has_point(at_position):
			continue
		var card: Dictionary = z.get("card", {}).duplicate(true)
		if card.is_empty():
			return {}
		if not card.has("actions"):
			card["actions"] = [{"label": String(z.get("hint", "Ouvrir")),
				"request": (z["request"] as Dictionary).duplicate(true)}]
		return card
	return {}

var _date: Control = null   ## date = contrôle ENFANT à cadence QUOTIDIENNE (cf. date_chip.gd)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics (pas la carte dessous)
	_date = load("res://ui/date_chip.gd").new()
	add_child(_date)
	_resize()
	get_viewport().size_changed.connect(_resize)
	Sim.generated.connect(_on_change)
	Sim.month_ticked.connect(_on_tick)   # les chiffres s'updatent au MOIS (anti-danse)

func _resize() -> void:
	position = Vector2.ZERO
	size = Vector2(get_viewport_rect().size.x, H)
	if _date != null:
		var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
		_date.position = Vector2(size.x - 116.0 - dtw - 18.0, 0)
		_date.size = Vector2(dtw + 8.0, H)
	queue_redraw()

func _on_tick(_year: int) -> void:
	var w = Sim.world
	if w != null and Sim.game_on:
		var me: int = w.player()
		var ci: Dictionary = w.country_info(me)
		if bool(ci.get("valide", false)):
			var g := float(ci["or"])
			var p := int(ci["pop"])
			_d_gold = g - _last_gold
			_d_pop = p - _last_pop
			_last_gold = g
			_last_pop = p
			if w.has_method("country_price_level"):
				var pl := float(w.country_price_level(me))
				_d_price_lvl = pl - _last_price_lvl
				_last_price_lvl = pl
			if w.has_method("country_factions"):
				var fx: Dictionary = w.country_factions(me)
				var seen := {}
				for fe in fx.get("list", []):
					var fnm := String(fe.get("name", ""))
					if fnm == "":
						continue
					var part := int(fe.get("part", 0))
					if _fac_last.has(fnm):
						_fac_d[fnm] = part - int(_fac_last[fnm])
					_fac_last[fnm] = part
					seen[fnm] = true
				# oublie les factions disparues (évite un delta figé sur un nom mort)
				for k in _fac_last.keys():
					if not seen.has(k):
						_fac_last.erase(k)
						_fac_d.erase(k)
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	var ww := size.x
	VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, 0, ww, 1), Color(1.0, 1.0, 1.0, 0.07))
	VKit.fill(self, Rect2(0, H - 3, ww, 2), Color(0.02, 0.025, 0.025, 0.9))
	VKit.fill(self, Rect2(0, H - 1, ww, 1), VKit.COL_GOLD)
	var cy := (H - 18.0) * 0.5

	if Sim.world == null:
		VKit.text(self, Vector2(16, cy), VKit.COL_DIM, "(libscps absente — voir README)")
		return
	if not Sim.world.has_method("province_at"):
		VKit.text(self, Vector2(12, cy), VKit.sense(0.5),
			"⚠ libscps OBSOLÈTE — rebâtir : scons platform=windows use_mingw=yes")
		return

	var w = Sim.world
	_tips.clear()
	_nav_zones.clear()

	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var px := 16.0
	var content_end := px   # ancre le BLOC TEMPS à droite (anti-chevauchement, cf. plus bas)
	if bool(ci.get("valide", false)):
		# armes du joueur (repli couronne si absentes)
		var parms: Texture2D = load("res://ui/heraldry.gd").arms(me)
		if parms != null:
			draw_texture_rect(parms, Rect2(px - 3, (H - 30.0) * 0.5, 30, 30), false)
		else:
			UIKit.draw_icon(self, "politics_crown", Vector2(px + 2.0, (H - 26.0) * 0.5), 26)
		px += 30
		var CPTips: Dictionary = load("res://ui/country_panel.gd").TIPS

		var dmt: Dictionary = w.country_demo(me) if w.has_method("country_demo") else {}
		var clst: Array = dmt.get("classes", [])
		var _happy_tip := ""
		if clst.size() >= 3:
			var sat_avg := 0.0
			var wsum := 0.0
			for cl in clst:
				var p := float(cl.get("pop", 0))
				sat_avg += float(cl.get("satisfaction", 0)) * p
				wsum += p
			sat_avg = sat_avg / maxf(wsum, 1.0)
			var bglyph := ""
			if sat_avg >= 66.0:
				bglyph = " ▲"
			elif sat_avg <= 33.0:
				bglyph = " ▼"
			_happy_tip = "Bonheur %d %%%s (Journaliers %d · Bourgeois %d · Élites %d)" % [
				int(round(sat_avg)), bglyph,
				int(clst[0].get("satisfaction", 0)), int(clst[1].get("satisfaction", 0)),
				int(clst[2].get("satisfaction", 0))]
		var fx: Dictionary = w.country_factions(me) if w.has_method("country_factions") else {}

		var nom := String(ci["nom"])
		var nomw := VKit.text_w(nom)
		var nr := Rect2(px - 6.0, 6.0, nomw + 14.0, H - 12.0)
		VKit.fill(self, nr, Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.72))
		VKit.fill(self, Rect2(nr.position, Vector2(3.0, nr.size.y)), VKit.COL_GOLD)
		VKit.box(self, nr, VKit.COL_EDGE)
		VKit.text(self, Vector2(px + 2.0, cy), VKit.COL_GOLD, nom)
		var _dp_txt := _delta_txt(float(_d_pop))
		var _id_tip := "%s — Population %s%s · %d province(s) · %s" % [nom,
			_grp(ci["pop"]), (" %s/mois" % _dp_txt) if _dp_txt != "" else "",
			w.country_province_count(me), _gauge_line(ci, CPTips, "stabilite")]
		if w.has_method("colony_status"):
			var cs: Dictionary = w.colony_status()
			if bool(cs.get("active", false)):
				var tot := maxi(1, int(cs.get("total_days", 1)))
				var pct := int(round(100.0 * float(tot - int(cs.get("days_left", 0))) / float(tot)))
				_id_tip += " · colonie en chantier %d %%" % pct
		_tips.append([nr, _id_tip])
		_add_nav(nr, InfoRef.request(InfoRef.make(InfoRef.COUNTRY, me)), "ouvrir le royaume")
		px += nomw + 18

		var treasury_x := px
		var budget_now: Dictionary = w.budget_summary(me) if w.has_method("budget_summary") else {}
		var budget_doy := maxi(1, int(w.day_of_year())) if w.has_method("day_of_year") else 1
		var budget_month := float(budget_now.get("net", 0.0)) / float(budget_doy) * 30.0
		px = _cell(px, "fine_coin", "", _grp(ci["or"]), "%+d/mois" % int(round(budget_month)),
			budget_month >= 0.0, _treasury_tip(w, me))
		_add_nav(Rect2(treasury_x - 4, 0, px - treasury_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 0), "sidebar", {"section": "budget"}),
			"ouvrir le budget", _treasury_card(w, me, float(ci["or"])))

		if w.has_method("country_price_level"):
			var price_lvl := float(w.country_price_level(me))
			var price_pct := (_d_price_lvl / price_lvl * 100.0) if price_lvl > 0.0 else 0.0
			var price_dtxt := ("%+.1f%%/mois" % price_pct) if absf(price_pct) >= 0.05 else ""
			var price_word := "stables" if absf(price_pct) < 0.05 else ("en hausse" if price_pct > 0.0 else "en baisse")
			px = _cell(px, "action_trade", "", "%.2f" % price_lvl, price_dtxt, price_pct <= 0.0,
				"Prix : %.2f — %s\n(1.00 = neutre ; onglet Monnaie pour le détail)" % [price_lvl, price_word])
		px = _block_sep(px)

		var materials_x := px
		px = _materials_cell(px, w, me)
		_add_nav(Rect2(materials_x - 4, 0, px - materials_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar"), "ouvrir les stocks")
		var arms_x := px
		px = _matter_cell(px, w, me, "Armes légères")
		_add_nav(Rect2(arms_x - 4, 0, px - arms_x, H),
			InfoRef.request(InfoRef.make(InfoRef.RESOURCE, 36), "sidebar", {"tab": 2}),
			"ouvrir le stock d'armes")
		var short := worst_shortage(w, me)
		var _food_full_tip := _food_tip(w, me)
		if not short.is_empty():
			var djs := int(short["days"])
			var sname := String(short["name"])
			_food_full_tip += "\n%s : rupture dans %d j" % [sname, djs]
		if w.has_method("country_food"):
			var food_x := px
			var food_month := 0.0
			if w.has_method("country_stocks"):
				for stock in w.country_stocks(me):
					if _FOOD_NAMES.has(String(stock.get("name", ""))):
						food_month += float(stock.get("net_day", 0.0)) * 30.0
			px = _cell(px, "fine_grain", "", _grp(int(w.country_food(me))),
				"%+d/mois" % int(round(food_month)), food_month >= 0.0, _food_full_tip)
			_add_nav(Rect2(food_x - 4, 0, px - food_x, H),
				InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar", {"section": "food"}),
				"ouvrir les stocks alimentaires")
		px = _block_sep(px)

		var sx0 := px
		var research_month := float((w.country_research_income(me) as Dictionary).get("per_day", 0.0)) * 30.0 \
			if w.has_method("country_research_income") else 0.0
		px = _cell(px, "fine_knowledge", "", "%d" % int(ci["savoir"]),
			"%+d/mois" % int(round(research_month)), research_month >= 0.0, _research_tip(w, me))
		_savoir_rect = Rect2(sx0 - 4, 0, px - sx0, H)
		_add_nav(_savoir_rect, InfoRef.request(InfoRef.make(InfoRef.TECH, -1)), "ouvrir l'arbre de technologie")
		px = _block_sep(px)

		var coup := int(fx.get("coup", 0))
		var flist: Array = fx.get("list", [])
		var nfac := mini(flist.size(), 4)   # garde-fou de largeur (rarement > 3 factions)
		for fi in range(nfac):
			var faction_x := px
			var fe: Dictionary = flist[fi]
			var fnm := String(fe.get("name", ""))
			var part := int(fe.get("part", 0))
			var dom := bool(fe.get("dominant", false))
			var grief := int(fe.get("grief", 0))
			var fd := int(_fac_d.get(fnm, 0))
			var fdtxt := "%+d/mois" % fd if fd != 0 else ""
			var ftip := "%s — %d %% de soutien%s" % [fnm, part, " ★ dominante" if dom else ""]
			if fd != 0:
				ftip += " · %+d/mois" % fd
			if grief > 0:
				ftip += " · rancœur %d" % grief
			ftip += " · assise %d %% · politiques %+d" % [int(fe.get("base_part", part)), int(fe.get("policy_delta", 0))]
			if bool(fe.get("coup_driver", false)):
				ftip += " · porte le risque de coup (%d)" % int(fe.get("coup_pressure", 0))
			if fi == 0:
				ftip += "\nTension de coup %d" % coup
			var fval := ("★ %d%%" % part) if dom else ("%d%%" % part)
			px = _cell(px, "influence_compass", "", fval, fdtxt, fd >= 0, ftip,
				VKit.sense(0.20) if (grief >= 60 or coup >= 45) else Color(0, 0, 0, 0))
			_add_nav(Rect2(faction_x - 4, 0, px - faction_x, H),
				InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 7), "sidebar", {"section": "factions"}),
				"ouvrir le rapport de forces", _faction_card(fe, fx, fd))
		px = _block_sep(px)

		# LOYAUTÉ : fidélité du conseil si un ministre siège ; sinon la LÉGITIMITÉ.
		var loy := _council_loyalty(w, me)
		var loy_tip := "Loyauté du conseil %d / 100" % loy
		if loy < 0:
			loy = int(ci.get("legitimite", 0))
			loy_tip = "Loyauté %d / 100 (légitimité)" % loy
		var loyalty_x := px
		px = _cell(px, "politics_crown", "", "%d" % loy, "", true, loy_tip,
			VKit.sense(float(loy) / 100.0))
		_add_nav(Rect2(loyalty_x - 4, 0, px - loyalty_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 7), "sidebar"), "ouvrir le conseil")
		var prosp := int(ci.get("prosperite", 0))
		var _prosp_tip := "Prospérité — %s (%d / 100)" % [String(ci.get("prosperite_mot", "")), prosp]
		if not _happy_tip.is_empty():
			_prosp_tip += "\n" + _happy_tip
		var prosperity_x := px
		px = _cell(px, "prosperity_sprout", "", "%d" % prosp, "", true, _prosp_tip,
			VKit.sense(float(prosp) / 100.0))
		_add_nav(Rect2(prosperity_x - 4, 0, px - prosperity_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 1), "sidebar"), "ouvrir la démographie")
		content_end = px

	# ANCRÉ au contenu réel : un bloc politique long ne doit jamais chevaucher le bloc date/vitesse à droite.
	if content_end > 16.0:
		content_end = _block_sep(content_end)

	# _date = contrôle propre rafraîchi CHAQUE JOUR (anti-danse : sinon le compteur saute
	# par paquets de 8-9 j) ; ici on RÉSERVE seulement sa place.
	var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
	var dtx0 := ww - 116.0 - dtw - 18.0

	if Sim.game_on and Sim.speed_index == 0:
		var prw := 128.0
		# à GAUCHE du ledger (bande droite, dessinée après nous → elle couvrirait)
		var prr := Rect2(ww - Frame.LEDGER_W - prw - 12.0, H + 6.0, prw, 26.0)
		VKit.fill(self, prr, Color(0.38, 0.08, 0.07, 0.94))
		VKit.box(self, prr, Color(0.78, 0.62, 0.30))
		var ptxt := "Pause"
		VKit.text(self, Vector2(prr.position.x + (prw - VKit.text_w(ptxt)) * 0.5, prr.position.y + 4),
			Color(0.94, 0.88, 0.74), ptxt)

	# CONTRÔLE DE VITESSE : 4 boutons DISCRETS. Espace bascule toujours.
	_speed_btns.clear()
	var sbw := 34.0
	var sx := ww - 8.0 - 4.0 * sbw
	_speed_rect = Rect2(sx, 6, 4.0 * sbw, H - 12)   # zone de hit globale
	var glyphs := ["❙❙", "▶", "▶▶", "▶▶▶"]
	var stips := ["Pause (Espace)", "Vitesse lente", "Vitesse normale", "Vitesse rapide"]
	for i in range(4):
		var r := Rect2(sx + float(i) * sbw, 6, sbw - 3.0, H - 12)
		var active := (Sim.speed_index == i)
		VKit.fill(self, r, VKit.COL_GOLD if active else Color(0.075, 0.085, 0.085, 0.96))
		VKit.box(self, r, VKit.COL_GOLD if active else VKit.COL_EDGE)
		if not active:
			VKit.fill(self, Rect2(r.position + Vector2(1, 1), Vector2(r.size.x - 2, 1)), Color(1, 1, 1, 0.08))
		var g: String = glyphs[i]
		VKit.text(self, Vector2(r.position.x + (r.size.x - VKit.text_w(g, VKit.FS_SMALL)) * 0.5,
			r.position.y + (r.size.y - 16.0) * 0.5), VKit.COL_PANEL if active else VKit.COL_PARCH, g, VKit.FS_SMALL)
		_speed_btns.append([r, i])
		_tips.append([r, String(stips[i])])

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var clickable := _speed_rect.has_point(event.position)
		for z in _nav_zones:
			if (z["rect"] as Rect2).has_point(event.position):
				clickable = true
				break
		mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND if clickable else Control.CURSOR_ARROW
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for z in _nav_zones:
			if (z["rect"] as Rect2).has_point(event.position):
				navigate_requested.emit((z["request"] as Dictionary).duplicate(true))
				Sound.play("ui_click")
				return
		if _savoir_rect.has_point(event.position):
			tech_requested.emit() # compat migration
		elif _speed_rect.has_point(event.position):
			for sb in _speed_btns:
				if (sb[0] as Rect2).has_point(event.position):
					Sim.set_speed(int(sb[1]))
					Sound.play("ui_click")
					break
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
