extends Control
## Topbar — bandeau PLEINE LARGEUR (cadre d'écran) : capsule de date (An N) + le
## roll-up du PAYS JOUÉ à gauche, contrôle de VITESSE cliquable à DROITE. Suit la
## largeur de la fenêtre (size_changed). Display-only sauf le verbe vitesse. Lit Sim.
##
## REFONTE 2026-08-19 (docs/BRIEF_CODEX_UI_TOPBAR_MODES.md §1) : 9 cellules FIXES,
## dans l'ordre — (1) nom+blason · (2) Or · (3) Population · (4) Matériaux (Pierre+
## Argile+Bois) · (5) Nourriture (Céréales+Bétail+Poisson+Fruits) · (6) Armes (légères+
## lourdes+Poudre+enchantées) · (7) Produits manufacturés · (8) Satisfaction globale
## (par classe au survol) · (9) vitesse+date. Prix national, Savoir, pénurie-alerte,
## factions/loyauté et prospérité SORTENT de la barre (toujours lisibles au panneau
## pays `country_panel.gd` — clic sur le nom — et au tiroir gauche `sidebar_drawer.gd` ;
## la recherche/tech reste accessible via Ctrl+K `search_palette.gd`). 3 séparateurs
## de bloc (`_block_sep`) : IDENTITÉ (1) · RESSOURCES (2-7) · ÉTAT (8) · TEMPS (9).
##
## MODE OBSERVATEUR (2026-07-30) : les cellules 1-8 sont remplacées par un simple mot
## neutre — `_observing()` — le BLOC TEMPS (date/vitesse/pause), lui, reste identique.
##
## AJOUT UI-DOCTRINE P2 (2026-09-02) : une cellule INFLUENCE (icon2 "influence"), insérée
## à côté de la Population (directive joueur) — stock nu + delta net/mois, hover
## gain/dépenses détaillé. Clic → panneau des Doctrines (doctrine_panel.gd, 6 slots).

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")
const H := Frame.TOPBAR_H

## `worst_shortage()` ci-dessous (la PIRE pénurie du pays) N'EST PLUS affichée en
## face/hover de topbar depuis la refonte 2026-08-19 (§1 « pénurie-alerte » SORT) —
## la fonction reste statique et VIVANTE : `alerts.gd` la consomme toujours (même
## préchargement de ce script, même calcul, DRY) pour son propre chip d'alerte.

signal tech_requested
signal navigate_requested(request: Dictionary)
## UI-DOCTRINE P2 (2026-09-02) : clic sur la cellule Influence → panneau des Doctrines
## (6 slots), motif `tech_requested` (signal direct, pas d'InfoRef — cible hors énum).
signal doctrine_requested

var _speed_rect := Rect2()
var _speed_btns := []   ## boutons de vitesse DISCRETS façon RimWorld : [[Rect2, index], …]
var _savoir_rect := Rect2()
var _influence_rect := Rect2()   ## la cellule Influence — clic → doctrine_requested
var _nav_zones: Array = []     ## [{rect, request, hint}] — mêmes coordonnées que le dessin
# §7 — l'encart d'âge (« Engager : … » / « Âge : … ») a DÉMÉNAGÉ en haut du menu de
# droite (empire_sidebar.gd), sous le bloc TEMPS — retour joueur 2026-07-11.

## DELTAS MENSUELS (rendu attendu EU4 : « 12 125 +45 » vert/rouge) — photo du mois
## précédent, prise à chaque month_ticked (display-only, aucun état moteur).
var _last_gold := 0.0
var _last_pop := 0
var _d_gold := 0.0
var _d_pop := 0

## (l'indice des prix national et la tendance par faction VIVAIENT ici — retirés de
## la topbar avec le prix/savoir/factions·loyauté, brief refonte 2026-08-19 §1 « ce
## qui SORT » ; toujours lisibles au panneau pays (country_panel.gd, clic sur le nom)
## et à l'onglet Conseil/Économie du tiroir.)

func _delta_txt(d: float) -> String:
	if absf(d) < 0.5:
		return ""
	return "%+d" % int(round(d))

## STOCK + FLUX MENSUEL d'une ressource par NOM (country_stocks → stock & net_day). Le
## « +x/mois » = net_day × 30 (le flux RÉEL du jour, projeté au mois — jamais un calcul
## inventé). Renvoie {} si la ressource n'est pas listée pour ce pays.
func _res_pair(w, me: int, rname: String) -> Dictionary:
	if not w.has_method("country_stocks"):
		return {}
	for st in w.country_stocks(me):
		if String(st.get("name", "")) == rname:
			return {"stock": int(st.get("stock", 0)), "permo": float(st.get("net_day", 0.0)) * 30.0}
	return {}

## UNE CELLULE DE GROUPE DE RESSOURCES (brief §1 points 4-7 : Matériaux/Nourriture/
## Armes/Manufacturés) — icône icon2 · valeur = SOMME des stocks des `names` · delta =
## SOMME des flux mensuels (net_day×30) · hover = détail par ressource (seulement
## celles à stock ou flux non nul — pas de ligne à zéro, retour joueur pénurie/piège
## §6). Motif UNIQUE, remplace les cellules dédiées d'hier (une par point du brief).
func _res_group_cell(px: float, w, me: int, icon2name: String, names: PackedStringArray, label: String) -> float:
	var total := 0
	var permo_total := 0.0
	var parts := PackedStringArray()
	for rname in names:
		var rp := _res_pair(w, me, rname)
		if rp.is_empty():
			continue
		var stock := int(rp.get("stock", 0))
		var permo := float(rp.get("permo", 0.0))
		total += stock
		permo_total += permo
		if stock != 0 or absf(permo) >= 0.5:
			var line := "%s %s" % [rname, _grp(stock)]
			if absf(permo) >= 0.5:
				line += " (%+d/mois)" % int(round(permo))
			parts.append(line)
	var tip := label
	if not parts.is_empty():
		tip += " — " + " · ".join(parts)
	return _cell(px, "", "", _grp(total), "%+d/mois" % int(round(permo_total)), permo_total >= 0.0,
		tip, Color(0, 0, 0, 0), "", icon2name)

## LA PIRE PÉNURIE du pays (retour joueur UI-2 point 2/3 : « Fer : rupture dans 12
## jours » remonte en alerte explicite au lieu de noyer 5 chips de stock brut dans la
## barre). Voie 1 : `country_shortages` si la façade l'expose (câblage en cours par un
## agent parallèle — testé via has_method, jamais supposé présent). Voie 2 (repli
## TOUJOURS disponible) : dérivée de `country_stocks()` — LA MÊME donnée que la barre
## lisait déjà pour les 5 raws — dont `coverage_days` EST déjà stock/|net_day| côté
## façade (scps_api.c:1133, plafonné à 366 = « >1 an »/pas de pénurie=-1) : pas besoin
## de recalculer, juste de prendre le pire (le plus petit ≥0) sur TOUTES les ressources
## (pas seulement les 5 raws — la nourriture a son propre chip mais un déficit de
## nourriture DOIT aussi remonter ici s'il est pire que tout le reste).
## Static : consommée aussi par alerts.gd (preload de ce script, même calcul, DRY).
static func worst_shortage(w, me: int) -> Dictionary:
	if w == null or me < 0:
		return {}
	if w.has_method("country_shortages"):
		# binding scps_sim_node.cpp:1474 — [{nom, res_id, runway_days, structurel}],
		# trié urgence croissante ; runway_days = jours avant rupture (double).
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

## RETOUR JOUEUR (2026-07-10, « quoi + combien ») : un hover ne DÉFINIT plus un
## concept (« l'or c'est... ») — le mot lui-même (Trésor, Population, Grenier…) est
## déjà décoré turquoise et cliquable par le TooltipServer (tooltip_server.gd lit
## ui/concepts.gd) ; SA définition vit derrière ce clic, jamais répétée ici. Le
## hover donne le NOM puis les MONTANTS RÉELS qui l'expliquent — jamais un calcul
## inventé : uniquement des lignes déjà exposées par la façade (country_budget/
## country_stocks/country_info), telles quelles.

## LES POSTES RÉELS de l'instrument I0 (l'or, ligne à ligne — country_budget →
## econ_flux_get/FX_*, scps_api.c:2045) — factorisé : consommé par le Trésor (état)
## ET par le Revenu net (le NOUVEAU permanent UI-3.1), même donnée, deux angles.
## Les postes de l'instrument I0 (country_budget → FX_*), convertis en TAUX MENSUEL :
## l'accumulateur est RAZ à chaque année ⇒ « /mois » = total-à-ce-jour ÷ jours-écoulés ×30
## (chiffres MENSUELS, retour joueur). Direct/franc : « Nom +N », pas de prose.
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

## HOVER OR — direct/franc, MENSUEL : le net du trésor par mois + les postes qui le
## composent (impôts · corruption · entretiens · salaires · armée…). Aucune définition,
## aucun détail d'opération — que des nombres.
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
	lines.append({"label": "Ligne de crédit", "value": "%d couronnes" % int(round(
		float(budget.get("credit_line", 0.0)))), "tone": "dim"})
	lines.append({"label": "Fin d'année (projection)", "value": "%s couronnes" % _grp(int(round(
		float(budget.get("projected_year_end", gold))))),
		"tone": "positive" if float(budget.get("projected_year_end", gold)) >= 0.0 else "negative"})
	var runway := float(budget.get("runway_months", -1.0))
	lines.append({"label": "Autonomie trésor + crédit", "value": "solde stable" if runway < 0.0 else \
		("< 1 mois" if runway < 1.0 else "%.1f mois" % runway),
		"tone": "dim" if runway < 0.0 else ("negative" if runway < 6.0 else "")})
	return {
		"title": "Trésor",
		"state": "%s couronnes disponibles" % _grp(int(round(gold))),
		"trend": "%+d / mois" % int(round(net_month)),
		"trend_tone": "positive" if net_month >= 0.0 else "negative",
		"lines": lines,
	}

## SÉPARATEUR DE BLOC — l'UNIQUE trait de la barre (retour joueur « c'est le bordel » :
## fini le double empilement filet-par-cellule + barre-de-bloc). Un SEUL filet or fin,
## inséré verticalement (marge haut/bas) : discret, « l'or = la structure », cohérent
## avec l'arête or du bas de barre. Les cellules d'un même bloc ne sont, elles, séparées
## que par l'espace.
func _block_sep(px: float) -> float:
	px += 10.0
	VKit.fill(self, Rect2(px, 13.0, 1.0, H - 26.0),
		Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.34))
	return px + 1.0 + 12.0

## `_gauge_line` formate un « Mot NN[ ▲|▼] » textuel pour les jauges 0-100 (stabilité…)
## reléguées en tooltip depuis la refonte 2026-08-19 — la barre ne garde que les 9
## cellules FIXES (cf. l'en-tête du fichier) ; le SURVOL de la cellule 1 (identité)
## porte encore la stabilité (UI-5 : la couleur ne porte jamais seule l'état, même
## en texte — le chiffre + le signe restent visibles hors couleur).
func _gauge_line(ci: Dictionary, cptips: Dictionary, key: String) -> String:
	var gv := int(ci.get(key, 0))
	var glyph := ""
	if gv >= 66:
		glyph = " ▲"
	elif gv <= 33:
		glyph = " ▼"
	return "%s %d%s" % [String(cptips.get(key, key)), gv, glyph]

## CELLULE DE RESSOURCE façon CK3 (hud.gui:6148-6207) : icône 32 px à gauche (UI-DOCTRINE
## D7 : 26→32, la cellule fait 48 px de haut, la marge le permettait), VALEUR
## empilée sur son DELTA (vert si ≥0, rouge sinon), séparateur vertical léger. `icon` =
## pièce du pack d'icônes OU `rid` ≥ 0 = sprite de ressource. `tip` = le HOVER (retour
## joueur : « un explicatif sur chaque display ») ; `vcol.a > 0` teinte la valeur.
func _cell(px: float, icon: String, rid_or_val, val: String, dtxt: String, dpos: bool,
		tip: String = "", vcol: Color = Color(0, 0, 0, 0), rname: String = "",
		icon2name: String = "") -> float:
	var rid := -1
	if icon == "" and typeof(rid_or_val) == TYPE_INT:
		rid = int(rid_or_val)
	if icon2name != "":
		# CELLULE série-2 (2026-08-19) : icône du nouveau lot (icons2/), résolue par
		# UIKit.icon2() — le SEUL registre, jamais un load() direct ici.
		var t2: Texture2D = UIKit.icon2(icon2name)
		if t2 != null:
			# le sceau d'influence (lot16) porte un aplat crème qui ressort BLANC sur la
			# barre sombre (probe 06_topbar_plein) — on le réchauffe par modulation pour
			# qu'il rejoigne la famille lot1 ; les autres icônes passent telles quelles.
			var mod := Color(0.96, 0.87, 0.70) if icon2name == "influence" else Color(1, 1, 1)
			draw_texture_rect(t2, Rect2(px, (H - 32.0) * 0.5, 32, 32), false, mod)
	elif rname != "":
		# cellule de RESSOURCE par NOM (bois/argile/pierre/armes) : le chip parchemin de
		# la ressource, résolu par resource_sprite(-1, nom) — même sprite que le tiroir Stocks.
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
	# la VALEUR de la cellule (chiffre-clé du topbar : trésor/pop/nourriture/savoir/…) —
	# COL_VALUE par défaut ; un `vcol` explicite (sense() bon/mauvais, ex. revenu net)
	# reste PRIORITAIRE — ce sens sémantique ne doit jamais être écrasé.
	if vcol.a > 0.0:
		VKit.text(self, Vector2(tx, 6.0), vcol, val)
	else:
		VKit.value(self, Vector2(tx, 6.0), val)
	var wv := VKit.text_w(val)
	var wd := 0.0
	if dtxt != "":
		VKit.text(self, Vector2(tx, 26.0), VKit.sense(0.85) if dpos else VKit.sense(0.12), dtxt, VKit.FS_SMALL)
		wd = VKit.text_w(dtxt, VKit.FS_SMALL)
	var cw := 36.0 + maxf(wv, wd) + 10.0   # (UI-DOCTRINE D7 : icône 26→32, marge +4 assortie)
	if tip != "":
		_tips.append([Rect2(px - 4.0, 0.0, cw + 8.0, H), tip])
	# (plus de filet PAR cellule — retour joueur « le bordel » : dans un bloc, les cellules
	#  ne sont séparées que par l'ESPACE ; SEUL _block_sep trace un trait, entre blocs.)
	return px + cw + 14.0

var _tips: Array = []   ## [[Rect2, texte], …] — reconstruit au _draw, hit-testé au survol

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

## Contrat lu optionnellement par TooltipServer. Une zone sans carte structurée garde
## son tooltip texte historique ; la migration peut donc avancer cellule par cellule.
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

var _date: Control = null   ## la date, contrôle ENFANT à cadence QUOTIDIENNE (cf. date_chip.gd)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics (pas la carte dessous)
	_date = load("res://ui/date_chip.gd").new()
	add_child(_date)
	_resize()
	get_viewport().size_changed.connect(_resize)
	Sim.generated.connect(_on_change)
	Sim.month_ticked.connect(_on_tick)   # les chiffres (or·pop·stocks) s'updatent au MOIS

func _resize() -> void:
	position = Vector2.ZERO
	size = Vector2(get_viewport_rect().size.x, H)
	if _date != null:
		var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
		_date.position = Vector2(size.x - 116.0 - dtw - 18.0, 0)
		_date.size = Vector2(dtw + 8.0, H)
	queue_redraw()

func _on_tick(_year: int) -> void:
	# photo mensuelle → deltas or/pop (rendu attendu EU4)
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
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	var ww := size.x
	# CHROME 2026-08-26 : fond livré (chrome_topbar_bg, 9-slice horizontal cap 32 px) —
	# remplace l'aplat VKit.COL_PANEL + le double liseré du bas (le fond porte déjà SON
	# liseré d'encre au bord inférieur — l'ANCIEN liseré dessiné est retiré, collision).
	# Repli sur l'ancien aplat si l'asset manque encore (.import pas généré, etc).
	var chrome_bg := UIKit.chrome_topbar_bg()
	if chrome_bg != null:
		UIKit.draw_9slice_h(self, chrome_bg, Rect2(0, 0, ww, H), 32.0)
	else:
		VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
		VKit.fill(self, Rect2(0, H - 3, ww, 2), Color(0.02, 0.025, 0.025, 0.9))
		VKit.fill(self, Rect2(0, H - 1, ww, 1), VKit.COL_GOLD)
	VKit.fill(self, Rect2(0, 0, ww, 1), Color(1.0, 1.0, 1.0, 0.07))
	var cy := (H - 18.0) * 0.5     # centrage vertical du contenu

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
	_influence_rect = Rect2()
	# (la capsule de chrome à gauche est RETIRÉE — panneaux plats, retour joueur 2026-07-10)

	# LE PAYS JOUÉ — CELLULES façon CK3 (hud.gui : icône + VALEUR empilée sur son DELTA
	# signé vert/rouge + séparateur vertical léger). Le delta = mensuel (or·pop) ou
	# net/jour (stocks, ScpsStock.net_day) : la barre dit l'état ET la tendance.
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var px := 16.0   # (la capsule de chrome qui occupait x=10..102 est retirée)
	var content_end := px   # où finit le contenu à gauche — ancre le BLOC TEMPS (age chip)
	if _observing():
		# MODE OBSERVATEUR : me/ci restent ceux du SLOT DE FOCUS (empire 0, cf.
		# scps_set_observer côté moteur) — ce n'est PAS « notre » pays. Aucun chiffre
		# national (trésor/pop/factions/etc) ne s'affiche ; un mot neutre remplace le
		# bloc entier. Le BLOC TEMPS (date/vitesse/pause) plus loin n'est pas concerné.
		var otxt := "Mode observateur"
		var orect := Rect2(px - 6.0, 6.0, VKit.text_w(otxt) + 14.0, H - 12.0)
		VKit.fill(self, orect, Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.72))
		VKit.box(self, orect, VKit.COL_EDGE)
		VKit.text(self, Vector2(px + 2.0, cy), VKit.COL_DIM, otxt)
		content_end = px + orect.size.x
	elif bool(ci.get("valide", false)):
		# les ARMES du joueur (héraldique dérivée) — repli couronne si pièces absentes
		var parms: Texture2D = load("res://ui/heraldry.gd").arms(me)
		if parms != null:
			draw_texture_rect(parms, Rect2(px - 3, (H - 30.0) * 0.5, 30, 30), false)
		else:
			# UI-DOCTRINE D7 : 18→26 px — même emplacement que les armes qu'elle remplace
			# (30×30 ci-dessus), légèrement plus petite pour rester sous l'avance fixe
			# de 30 px sans déborder sur la cellule suivante.
			UIKit.draw_icon(self, "politics_crown", Vector2(px + 2.0, (H - 26.0) * 0.5), 26)
		px += 30
		var CPTips: Dictionary = load("res://ui/country_panel.gd").TIPS

		# ═══ 1. IDENTITÉ (ancre) : nom du royaume — blason ci-dessus. Province count ·
		#     stabilité · colonie en chantier s'y lisent au SURVOL (jamais un calcul
		#     inventé — les mêmes lectures façade qu'avant, juste reléguées en tooltip). ═══
		var nom := String(ci["nom"])
		var nomw := VKit.text_w(nom)
		var nr := Rect2(px - 6.0, 6.0, nomw + 14.0, H - 12.0)
		VKit.fill(self, nr, Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.72))
		VKit.fill(self, Rect2(nr.position, Vector2(3.0, nr.size.y)), VKit.COL_GOLD)
		VKit.box(self, nr, VKit.COL_EDGE)
		VKit.text(self, Vector2(px + 2.0, cy), VKit.COL_GOLD, nom)
		var _id_tip := "%s — %d province(s) · %s" % [nom,
			w.country_province_count(me), _gauge_line(ci, CPTips, "stabilite")]
		if w.has_method("colony_status"):
			var cs: Dictionary = w.colony_status()
			if bool(cs.get("active", false)):
				var tot := maxi(1, int(cs.get("total_days", 1)))
				var pct := int(round(100.0 * float(tot - int(cs.get("days_left", 0))) / float(tot)))
				_id_tip += " · colonie en chantier %d %%" % pct
		_tips.append([nr, _id_tip])
		_add_nav(nr, InfoRef.request(InfoRef.make(InfoRef.COUNTRY, me)), "ouvrir le royaume")
		px += nomw + 10
		px = _block_sep(px)   # IDENTITÉ | RESSOURCES

		# ═══ 2. OR — face : le trésor + delta MENSUEL (photo _d_gold, cf. _on_tick).
		#     Hover : le revenu DÉTAILLÉ (impôts · corruption · entretiens · salaires…). ═══
		var treasury_x := px
		var d_gold_txt := ("%+d/mois" % int(round(_d_gold))) if absf(_d_gold) >= 0.5 else ""
		px = _cell(px, "", "", _grp(ci["or"]), d_gold_txt, _d_gold >= 0.0, _treasury_tip(w, me),
			Color(0, 0, 0, 0), "", "top_or")
		_add_nav(Rect2(treasury_x - 4, 0, px - treasury_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 0), "sidebar", {"section": "budget"}),
			"ouvrir le budget", _treasury_card(w, me, float(ci["or"])))

		# ═══ 3. POPULATION TOTALE — face : pop empire + delta mensuel (_d_pop). ═══
		var pop_x := px
		var _dp := _delta_txt(float(_d_pop))
		var d_pop_txt := ("%s/mois" % _dp) if _dp != "" else ""
		px = _cell(px, "", "", _grp(ci["pop"]), d_pop_txt, _d_pop >= 0, "Population",
			Color(0, 0, 0, 0), "", "top_population")
		_add_nav(Rect2(pop_x - 4, 0, px - pop_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 1), "sidebar"), "ouvrir la démographie")

		# ═══ 3bis. INFLUENCE POLITIQUE (UI-DOCTRINE P2, docs/DESIGN_MISSIONS_DOCTRINES.md
		#     §3/§5) — à CÔTÉ de la population (directive joueur) : stock nu (icon2
		#     "influence") + delta net mensuel. Hover : gain « +N/mois » et dépenses
		#     « −N/mois », chacun suivi du détail en mots du reader (hover/hover_depenses).
		#     Clic → panneau des Doctrines (6 slots, doctrine_panel.gd). Le reader
		#     `influence_info` peut encore manquer les clés upkeep_month/net_month/
		#     hover_depenses tant que le binding P2 n'a pas atterri (.get partout, défauts
		#     à 0/"" — cellule TOUJOURS affichée, jamais un crash). ═══
		var infl_x := px
		var infl: Dictionary = w.influence_info(me) if w.has_method("influence_info") else {}
		var infl_stock := int(infl.get("stock", 0))
		var infl_gain := int(infl.get("gain_month", 0))
		var infl_upkeep := int(infl.get("upkeep_month", 0))
		var infl_net := int(infl.get("net_month", infl_gain - infl_upkeep))
		var infl_hover := String(infl.get("hover", ""))
		var infl_hover_dep := String(infl.get("hover_depenses", ""))
		var infl_tip := "Influence %+d/mois" % infl_net
		if infl_gain != 0 or infl_hover != "":
			infl_tip += "\n+%d/mois%s" % [infl_gain, (" — " + infl_hover) if infl_hover != "" else ""]
		if infl_upkeep != 0 or infl_hover_dep != "":
			infl_tip += "\n−%d/mois%s" % [infl_upkeep, (" — " + infl_hover_dep) if infl_hover_dep != "" else ""]
		var infl_dtxt := ("%+d/mois" % infl_net) if absi(infl_net) >= 1 else ""
		px = _cell(px, "", "", _grp(infl_stock), infl_dtxt, infl_net >= 0, infl_tip,
			Color(0, 0, 0, 0), "", "influence")
		_influence_rect = Rect2(infl_x - 4, 0, px - infl_x, H)

		# ═══ 4. MATÉRIAUX DE CONSTRUCTION — somme Pierre+Argile+Bois · hover détaillé. ═══
		var materials_x := px
		px = _res_group_cell(px, w, me, "top_construction",
			PackedStringArray(["Pierre", "Argile", "Bois"]), "Matériaux")
		_add_nav(Rect2(materials_x - 4, 0, px - materials_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar"), "ouvrir les stocks")

		# ═══ 5. NOURRITURE — somme Céréales+Bétail+Poisson+Fruits · hover détaillé. ═══
		var food_x := px
		px = _res_group_cell(px, w, me, "top_nourriture",
			PackedStringArray(["Céréales", "Bétail", "Poisson", "Fruits"]), "Nourriture")
		_add_nav(Rect2(food_x - 4, 0, px - food_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar", {"section": "food"}),
			"ouvrir les stocks alimentaires")

		# ═══ 6. ARMES — somme Armes légères+lourdes+Poudre+Armes enchantées · hover. ═══
		var arms_x := px
		px = _res_group_cell(px, w, me, "top_armes",
			PackedStringArray(["Armes légères", "Armes lourdes", "Poudre", "Armes enchantées"]), "Armes")
		_add_nav(Rect2(arms_x - 4, 0, px - arms_x, H),
			InfoRef.request(InfoRef.make(InfoRef.RESOURCE, 36), "sidebar", {"tab": 2}),
			"ouvrir le stock d'armes")

		# ═══ 7. PRODUITS MANUFACTURÉS — somme de la famille de production · hover
		#     détaillé (seulement les biens à stock/flux non nul — pas 9 lignes à zéro). ═══
		var manuf_x := px
		px = _res_group_cell(px, w, me, "top_manufactures", PackedStringArray([
			"Outils", "Étoffe", "Papier", "Remèdes", "Tunique",
			"Bière", "Eau de vie", "Bien précieux", "Étoffe précieuse"]), "Produits manufacturés")
		_add_nav(Rect2(manuf_x - 4, 0, px - manuf_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar"), "ouvrir les stocks")
		px = _block_sep(px)   # RESSOURCES | ÉTAT

		# ═══ 8. SATISFACTION GLOBALE — moyenne pays pondérée pop (country_demo). Hover :
		#     détail PAR CLASSE + le ±X « Votre politique » (country_class_policy_sat). ═══
		var sat_x := px
		var dmt: Dictionary = w.country_demo(me) if w.has_method("country_demo") else {}
		var clst: Array = dmt.get("classes", [])
		var sat_avg := 0.0
		var sat_tip := "Satisfaction"
		if clst.size() >= 3:
			var wsum := 0.0
			for cl in clst:
				var p := float(cl.get("pop", 0))
				sat_avg += float(cl.get("satisfaction", 0)) * p
				wsum += p
			sat_avg = sat_avg / maxf(wsum, 1.0)
			var cls_names := ["Journaliers", "Bourgeois", "Élite"]
			var cparts := PackedStringArray()
			for i in range(3):
				var line := "%s %d" % [cls_names[i], int(clst[i].get("satisfaction", 0))]
				if w.has_method("country_class_policy_sat"):
					var pd := int(w.country_class_policy_sat(me, i))
					if pd != 0:
						line += " (votre politique %+d)" % pd
				cparts.append(line)
			sat_tip = "Satisfaction — " + " · ".join(cparts)
		px = _cell(px, "", "", "%d" % int(round(sat_avg)), "", true, sat_tip,
			VKit.sense(sat_avg / 100.0), "", "top_satisfaction")
		_add_nav(Rect2(sat_x - 4, 0, px - sat_x, H),
			InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 1), "sidebar"), "ouvrir la démographie")
		content_end = px

	# séparateur visuel avant le BLOC TEMPS — ANCRÉ au contenu RÉELLEMENT dessiné (pas
	# une position fixe) : un contenu politique long (pays à beaucoup de factions, longue
	# pénurie nommée) ne doit JAMAIS chevaucher le bloc date/vitesse à droite.
	if content_end > 16.0:
		content_end = _block_sep(content_end)

	# ═══ BLOC TEMPS : âge/chip Engager · date (date_chip) · vitesse (audit UI-2) ═══
	# LA DATE vit dans son PROPRE contrôle (_date, rafraîchi CHAQUE JOUR — le topbar,
	# lui, reste à la cadence mensuelle anti-danse : sans ça le compteur sautait par
	# paquets de 8-9 jours entre deux redraws, retour joueur 2026-07-10). On ne fait
	# ici que RÉSERVER sa place (largeur max fixe) pour ancrer le chip d'âge.
	var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
	var dtx0 := ww - 116.0 - dtw - 18.0
	# (§7 : l'encart d'âge « Engager/Âge » vit désormais en haut du menu de droite,
	#  empire_sidebar.gd — sous le bloc TEMPS. La topbar ne le dessine plus.)

	# RUBAN PAUSE (rendu attendu EU4) : le monde figé se DIT, pas juste un glyphe dans
	# le coin. ANCRÉ sous les contrôles de vitesse (bord droit) — c'est là que l'œil
	# va quand le temps est en cause ; la barre d'entropie garde le centre-haut.
	if Sim.game_on and Sim.speed_index == 0:
		var prw := 128.0
		# à GAUCHE du ledger (bande droite, dessinée après nous → elle couvrirait)
		var prr := Rect2(ww - Frame.LEDGER_W - prw - 12.0, H + 6.0, prw, 26.0)
		VKit.fill(self, prr, Color(0.38, 0.08, 0.07, 0.94))
		VKit.box(self, prr, Color(0.78, 0.62, 0.30))
		var ptxt := "Pause"
		VKit.text(self, Vector2(prr.position.x + (prw - VKit.text_w(ptxt)) * 0.5, prr.position.y + 4),
			Color(0.94, 0.88, 0.74), ptxt)

	# CONTRÔLE DE VITESSE façon RimWorld (TimeControls) : 4 boutons DISCRETS — l'état se
	# VOIT et on clique CE qu'on veut, plus de cycle aveugle. Espace bascule toujours.
	# Cibles ÉLARGIES à 34 px (audit lisibilité point 1 : « boutons de vitesse trop
	# petits », cible ≥32 px) — la hauteur H-12=36 px l'était déjà.
	_speed_btns.clear()
	var sbw := 34.0
	var sx := ww - 8.0 - 4.0 * sbw
	_speed_rect = Rect2(sx, 6, 4.0 * sbw, H - 12)   # (gardé : zone de hit globale)
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
		var clickable := _speed_rect.has_point(event.position) or _influence_rect.has_point(event.position)
		for z in _nav_zones:
			if (z["rect"] as Rect2).has_point(event.position):
				clickable = true
				break
		mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND if clickable else Control.CURSOR_ARROW
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _influence_rect.has_point(event.position):
			doctrine_requested.emit()
			Sound.play("ui_click")
			return
		for z in _nav_zones:
			if (z["rect"] as Rect2).has_point(event.position):
				navigate_requested.emit((z["request"] as Dictionary).duplicate(true))
				Sound.play("ui_click")
				return
		if _savoir_rect.has_point(event.position):
			tech_requested.emit() # compatibilité pendant la migration des anciens branchements
		elif _speed_rect.has_point(event.position):
			# boutons DISCRETS (RimWorld) : on clique LA vitesse voulue
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

## même check que main.gd::_observing() — dupliqué ici (ce script enfant n'a pas de
## référence à Main), cf. TROUVAILLES « Menu audio + mode observateur ».
func _observing() -> bool:
	return Sim.world != null and Sim.world.has_method("is_observer") and Sim.world.is_observer()
