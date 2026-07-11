extends Control
## Topbar — bandeau PLEINE LARGEUR (cadre d'écran) : capsule de date (An N) + le
## roll-up du PAYS JOUÉ (nom · or · pop empire · régions · savoir) à gauche, contrôle
## de VITESSE cliquable à DROITE. Suit la largeur de la fenêtre (size_changed).
## Display-only sauf le verbe vitesse. Lit Sim.
##
## RETOUR JOUEUR UI-2 (2026-07-10, docs/RETOURS_2026-07-10.md point 2 « TOPBAR
## SURCHARGÉ ») : le contenu se lit désormais en 4 BLOCS visuellement séparés (barre
## verticale épaisse, cf. _block_sep) — ROYAUME · ÉCONOMIE · POLITIQUE · TEMPS. Les 5
## cellules de matières brutes (bois/argile/pierre/fer/armes) SORTENT d'ici vers le
## tiroir Économie/Stocks ; seule la PIRE PÉNURIE remonte, en alerte explicite.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const H := Frame.TOPBAR_H

## (les 5 cellules de MATIÈRES BRUTES bois·argile·pierre·fer·armes, posées le 07-09,
## SORTENT de la topbar — retour joueur UI-2 « TOPBAR SURCHARGÉ » : elles vivent
## désormais dans le tiroir Économie/Stocks, cf. sidebar_drawer.gd. Seule reste ici la
## PIRE pénurie, dérivée dans worst_shortage() ci-dessous — consommée aussi par
## alerts.gd (préchargement statique du script, même donnée, un seul calcul).

signal tech_requested

var _speed_rect := Rect2()
var _speed_btns := []   ## boutons de vitesse DISCRETS façon RimWorld : [[Rect2, index], …]
var _savoir_rect := Rect2()
# §7 — l'encart d'âge (« Engager : … » / « Âge : … ») a DÉMÉNAGÉ en haut du menu de
# droite (empire_sidebar.gd), sous le bloc TEMPS — retour joueur 2026-07-11.

## DELTAS MENSUELS (rendu attendu EU4 : « 12 125 +45 » vert/rouge) — photo du mois
## précédent, prise à chaque month_ticked (display-only, aucun état moteur).
var _last_gold := 0.0
var _last_pop := 0
var _d_gold := 0.0
var _d_pop := 0

## INFLUENCE DE FACTION — la « tendance » demandée : chaque faction voit sa PART du
## spectre bouger d'un mois à l'autre (le decay du moteur, mesuré à la source). On
## photographie la part de chaque faction (par NOM) au roulement du mois et on affiche
## le delta « +x/mois » — display-only, aucune coordonnée moteur, juste le mouvement OBSERVÉ.
var _fac_last := {}      ## nom de faction → part du mois précédent
var _fac_d := {}         ## nom de faction → delta mensuel (+/-)

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

## une CELLULE de matière (bois/argile/pierre/armes) : sprite par nom · valeur = stock ·
## delta = « +N/mois » vert/rouge · hover nommé. Repli discret si la ressource manque.
func _matter_cell(px: float, w, me: int, rname: String) -> float:
	var rp := _res_pair(w, me, rname)
	if rp.is_empty():
		# ressource absente du bilan (jamais produite/stockée) : la cellule reste
		# PRÉSENTE à « 0 » — la matière de construction/armement de la barre définitive
		# ne doit pas clignoter selon ce qui est en stock (« vous n'en avez aucune »).
		return _cell(px, "", "", "0", "", true, "%s — aucun en stock" % rname,
			VKit.COL_DIM, rname)
	# FACE = le nombre seul (retour joueur : « l'UI fournit l'information, PAS PLUS ») ;
	# le flux mensuel vit dans le HOVER.
	var permo := float(rp["permo"])
	var tip := "%s — %s en stock" % [rname, _grp(int(rp["stock"]))]
	if absf(permo) >= 0.5:
		tip += " · %+d/mois" % int(round(permo))
	return _cell(px, "", "", _grp(int(rp["stock"])), "", true, tip, Color(0, 0, 0, 0), rname)

## LOYAUTÉ du royaume = moyenne de la loyauté des sièges du CONSEIL (0-100) — la fidélité
## des grands du royaume envers la couronne (country_council → loyalty). -1 si pas de conseil.
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

## LA NOURRITURE EN QUOI+COMBIEN : production vs consommation, bien par bien
## (country_stocks → net_day, le flux RÉEL du jour) — pas une leçon sur le Grenier.
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

## MATÉRIAUX — UN SEUL onglet : le TOTAL des matières de construction (bois + argile +
## pierre) ; le hover DÉTAILLE chaque matière (stock + flux mensuel). Chiffres MENSUELS.
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
	# FACE = le total seul (« genre 82 ») ; le détail par matière est dans le HOVER.
	return _cell(px, "action_build", "", _grp(total), "", true,
		"Matériaux — " + " · ".join(parts))

## HOVER SAVOIR — direct/franc, chiffres MENSUELS : le revenu de recherche décomposé —
## Pops (base) · Institutions (×) · Lumière de l'âge (×, si portée) · Métabolisation (+%).
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
	return "Recherche +%d/mois\n%s\n(clic : l'arbre de technologie)" % [perm, " · ".join(parts)]

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

## RETOUR JOUEUR UI-3.1 (2026-07-11, docs/UI_RECO_2026-07-10.md §3.1 « topbar
## simplifiée ») : la barre ne garde que ~8 PERMANENTS (trésor · revenu net annuel ·
## pop · nourriture+rupture · recherche · Influence · Corruption · date+vitesse).
## Les jauges 0-100 (stabilité/prospérité/légitimité/cohésion), le décompte de
## provinces, la colonisation en chantier, le bonheur détaillé et les blasons de
## faction ne DISPARAISSENT PAS — ils se lisent au SURVOL du bloc le plus proche
## (Pop pour l'état du royaume, Revenu net pour l'économie, Influence/Corruption
## pour la politique) : `_gauge_line` formate un « Mot NN[ ▲|▼] » textuel pour ces
## tooltips composites (UI-5 : la couleur ne porte jamais seule l'état, même en
## texte — le chiffre + le signe restent visibles hors couleur).
func _gauge_line(ci: Dictionary, cptips: Dictionary, key: String) -> String:
	var gv := int(ci.get(key, 0))
	var glyph := ""
	if gv >= 66:
		glyph = " ▲"
	elif gv <= 33:
		glyph = " ▼"
	return "%s %d%s" % [String(cptips.get(key, key)), gv, glyph]

## CELLULE DE RESSOURCE façon CK3 (hud.gui:6148-6207) : icône 22 px à gauche, VALEUR
## empilée sur son DELTA (vert si ≥0, rouge sinon), séparateur vertical léger. `icon` =
## pièce du pack d'icônes OU `rid` ≥ 0 = sprite de ressource. `tip` = le HOVER (retour
## joueur : « un explicatif sur chaque display ») ; `vcol.a > 0` teinte la valeur.
func _cell(px: float, icon: String, rid_or_val, val: String, dtxt: String, dpos: bool,
		tip: String = "", vcol: Color = Color(0, 0, 0, 0), rname: String = "") -> float:
	var rid := -1
	if icon == "" and typeof(rid_or_val) == TYPE_INT:
		rid = int(rid_or_val)
	if rname != "":
		# cellule de RESSOURCE par NOM (bois/argile/pierre/armes) : le chip parchemin de
		# la ressource, résolu par resource_sprite(-1, nom) — même sprite que le tiroir Stocks.
		var rspr: Texture2D = UIKit.resource_sprite(-1, rname)
		if rspr != null:
			draw_texture_rect(rspr, Rect2(px, (H - 22.0) * 0.5, 22, 22), false)
	elif rid >= 0:
		var spr: Texture2D = UIKit.resource_sprite(rid, "")
		if spr != null:
			draw_texture_rect(spr, Rect2(px, (H - 22.0) * 0.5, 22, 22), false)
	elif icon != "":
		UIKit.draw_icon(self, icon, Vector2(px, (H - 22.0) * 0.5), 22)
	var tx := px + 26.0
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
	var cw := 26.0 + maxf(wv, wd) + 10.0
	if tip != "":
		_tips.append([Rect2(px - 4.0, 0.0, cw + 8.0, H), tip])
	# (plus de filet PAR cellule — retour joueur « le bordel » : dans un bloc, les cellules
	#  ne sont séparées que par l'ESPACE ; SEUL _block_sep trace un trait, entre blocs.)
	return px + cw + 14.0

var _tips: Array = []   ## [[Rect2, texte], …] — reconstruit au _draw, hit-testé au survol

func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""

var _date: Control = null   ## la date, contrôle ENFANT à cadence QUOTIDIENNE (cf. date_chip.gd)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics (pas la carte dessous)
	_date = load("res://ui/date_chip.gd").new()
	add_child(_date)
	_resize()
	get_viewport().size_changed.connect(_resize)
	Sim.generated.connect(_on_change)
	Sim.month_ticked.connect(_on_tick)   # les chiffres (or·pop·savoir·food) s'updatent au MOIS

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
			# tendance des factions : delta mensuel de la PART de chaque faction (le decay observé)
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
	# ledger EU4 sur plaque RimWorld : graphite, arête froide, un seul liseré or.
	VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, 0, ww, 1), Color(1.0, 1.0, 1.0, 0.07))
	VKit.fill(self, Rect2(0, H - 3, ww, 2), Color(0.02, 0.025, 0.025, 0.9))
	VKit.fill(self, Rect2(0, H - 1, ww, 1), VKit.COL_GOLD)
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
	# (la capsule de chrome à gauche est RETIRÉE — panneaux plats, retour joueur 2026-07-10)

	# LE PAYS JOUÉ — CELLULES façon CK3 (hud.gui : icône + VALEUR empilée sur son DELTA
	# signé vert/rouge + séparateur vertical léger). Le delta = mensuel (or·pop) ou
	# net/jour (stocks, ScpsStock.net_day) : la barre dit l'état ET la tendance.
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var px := 16.0   # (la capsule de chrome qui occupait x=10..102 est retirée)
	var content_end := px   # où finit le contenu à gauche — ancre le BLOC TEMPS (age chip)
	if bool(ci.get("valide", false)):
		# les ARMES du joueur (héraldique dérivée) — repli couronne si pièces absentes
		var parms: Texture2D = load("res://ui/heraldry.gd").arms(me)
		if parms != null:
			draw_texture_rect(parms, Rect2(px - 3, (H - 30.0) * 0.5, 30, 30), false)
		else:
			UIKit.draw_icon(self, "politics_crown", Vector2(px, cy - 2), 18)
		px += 30
		var CPTips: Dictionary = load("res://ui/country_panel.gd").TIPS

		# RETOUR JOUEUR UI-3.1 (2026-07-11) : la barre ne garde que ~8 PERMANENTS —
		# trésor · revenu net annuel · pop · nourriture(+rupture) · recherche ·
		# Influence · Corruption · date+vitesse. Tout le SECONDAIRE d'hier (provinces,
		# stabilité, prospérité, colonisation en chantier, légitimité, cohésion,
		# bonheur détaillé, blasons de faction, tension de coup) ne disparaît pas :
		# il se lit désormais au SURVOL du bloc le plus proche (jamais un calcul
		# inventé — les mêmes lectures façade qu'avant, juste reléguées en tooltip).
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
			_happy_tip = "Bonheur %d %%%s (Laboureurs %d · Artisans %d · Noblesse %d)" % [
				int(round(sat_avg)), bglyph,
				int(clst[0].get("satisfaction", 0)), int(clst[1].get("satisfaction", 0)),
				int(clst[2].get("satisfaction", 0))]
		var fx: Dictionary = w.country_factions(me) if w.has_method("country_factions") else {}

		# ═══ IDENTITÉ (ancre) : nom du royaume. Population · recherche · provinces ·
		#     stabilité · métabolisation · colonie s'y lisent au SURVOL — la barre
		#     DÉFINITIVE (retour joueur 2026-07-11) ne les garde plus en permanents. ═══
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
		px += nomw + 18

		# ═══ OR — face : le trésor seul. Hover : le REVENU DÉTAILLÉ MENSUEL (impôts ·
		#     corruption · entretiens · salaires · armée…) via country_budget (I0). ═══
		px = _cell(px, "fine_coin", "", _grp(ci["or"]), "", true, _treasury_tip(w, me))
		px = _block_sep(px)

		# ═══ MATÉRIAUX : UN onglet (total bois+argile+pierre) · hover = détail par matière ══
		px = _materials_cell(px, w, me)
		# ═══ ARMES : stock + flux mensuel (même modèle) ══
		px = _matter_cell(px, w, me, "Armes légères")
		# ═══ NOURRITURE : stock du grenier. La rupture SORT de la cellule → seul le hover
		#     porte l'income par bien ET « pénurie dans X » (retour joueur : l'UI = le
		#     nombre, le hover = le détail). ══
		var short := worst_shortage(w, me)
		var _food_full_tip := _food_tip(w, me)
		if not short.is_empty():
			var djs := int(short["days"])
			var sname := String(short["name"])
			_food_full_tip += "\nPénurie — %s : rupture dans %d j" % [sname, djs]
		if w.has_method("country_food"):
			px = _cell(px, "fine_grain", "", _grp(int(w.country_food(me))), "", true, _food_full_tip)
		px = _block_sep(px)

		# ═══ SAVOIR : le niveau · hover = income de recherche MENSUEL décomposé (Pops ·
		#     Institutions · Lumière · Métabolisation) · CLIC = l'arbre de technologie. ══
		var sx0 := px
		px = _cell(px, "fine_knowledge", "", "%d" % int(ci["savoir"]), "", true, _research_tip(w, me))
		_savoir_rect = Rect2(sx0 - 4, 0, px - sx0, H)
		px = _block_sep(px)

		# ═══ INFLUENCE DE FACTION : chaque faction · sa PART · sa TENDANCE « +x/mois »
		#     (le decay observé au fil des mois, cf. _on_tick). La dominante porte ★. ═══
		var coup := int(fx.get("coup", 0))
		var flist: Array = fx.get("list", [])
		var nfac := mini(flist.size(), 4)   # garde-fou de largeur (rarement > 3 factions)
		for fi in range(nfac):
			var fe: Dictionary = flist[fi]
			var fnm := String(fe.get("name", ""))
			var part := int(fe.get("part", 0))
			var dom := bool(fe.get("dominant", false))
			var grief := int(fe.get("grief", 0))
			var fd := int(_fac_d.get(fnm, 0))
			var fdtxt := "%+d/mois" % fd if fd != 0 else ""
			var ftip := "%s — %d %% de soutien%s" % [fnm, part, " ★ dominante" if dom else ""]
			if fd != 0:
				ftip += " · tend %+d/mois" % fd
			if grief > 0:
				ftip += " · rancœur %d" % grief
			if fi == 0:
				ftip += "\nTension de coup %d" % coup
			var fval := ("★ %d%%" % part) if dom else ("%d%%" % part)
			px = _cell(px, "influence_compass", "", fval, fdtxt, fd >= 0, ftip,
				VKit.sense(0.20) if (grief >= 60 or coup >= 45) else Color(0, 0, 0, 0))
		px = _block_sep(px)

		# ═══ LOYAUTÉ · PROSPÉRITÉ (jauges 0-100, valeur teintée bon/mauvais — UI-5 :
		#     le chiffre reste lisible hors couleur). ═══
		# LOYAUTÉ : la fidélité du conseil si un ministre siège ; sinon la LÉGITIMITÉ
		# (l'adhésion du royaume au droit de régner) — la barre garde toujours la paire.
		var loy := _council_loyalty(w, me)
		var loy_tip := "Loyauté du conseil %d / 100" % loy
		if loy < 0:
			loy = int(ci.get("legitimite", 0))
			loy_tip = "Loyauté %d / 100 (légitimité — conseil vacant)" % loy
		px = _cell(px, "politics_crown", "", "%d" % loy, "", true, loy_tip,
			VKit.sense(float(loy) / 100.0))
		var prosp := int(ci.get("prosperite", 0))
		var _prosp_tip := "Prospérité — %s (%d / 100)" % [String(ci.get("prosperite_mot", "")), prosp]
		if not _happy_tip.is_empty():
			_prosp_tip += "\n" + _happy_tip
		px = _cell(px, "prosperity_sprout", "", "%d" % prosp, "", true, _prosp_tip,
			VKit.sense(float(prosp) / 100.0))
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
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _savoir_rect.has_point(event.position):
			tech_requested.emit()
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
