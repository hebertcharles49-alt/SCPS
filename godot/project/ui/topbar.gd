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
var _age_rect := Rect2()   # §7 : chip « Âge levé — Engager » (vide quand rien à engager)

## DELTAS MENSUELS (rendu attendu EU4 : « 12 125 +45 » vert/rouge) — photo du mois
## précédent, prise à chaque month_ticked (display-only, aucun état moteur).
var _last_gold := 0.0
var _last_pop := 0
var _d_gold := 0.0
var _d_pop := 0

func _delta_txt(d: float) -> String:
	if absf(d) < 0.5:
		return ""
	return "%+d" % int(round(d))

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
func _budget_parts_txt(w, me: int) -> String:
	if not w.has_method("country_budget"):
		return ""
	var parts := []
	for p in w.country_budget(me):
		var amt: float = float(p.get("amount", 0.0))
		if absf(amt) < 0.5:
			continue
		parts.append("%s %+d" % [String(p.get("name", "")), int(round(amt))])
	return " · ".join(parts)

## LE TRÉSOR EN QUOI+COMBIEN : au lieu de raconter ce qu'est l'argent. `budget_summary`
## donne le net de l'année en cours (le flux RAZ à chaque roulement d'année,
## scps_api.c:149-159 — ce n'est pas un solde "mensuel", contrairement à l'ancien libellé).
func _treasury_tip(w, me: int) -> String:
	var tip := "Trésor"
	var parts := _budget_parts_txt(w, me)
	if parts != "":
		tip += " — " + parts
	if w.has_method("budget_summary"):
		var net := float((w.budget_summary(me) as Dictionary).get("net", 0.0))
		tip += " (net %+d cette année)" % int(round(net))
	return tip

## LE REVENU NET EN QUOI+COMBIEN (UI-3.1, permanent séparé du Trésor — la VALEUR
## affichée sur la cellule EST déjà le net ; le survol détaille les postes qui le
## composent, jamais un second calcul).
func _net_income_tip(w, me: int) -> String:
	var tip := "Revenu net (année en cours)"
	var parts := _budget_parts_txt(w, me)
	if parts != "":
		tip += " — " + parts
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
			parts.append("%s %+.1f/j" % [nm, float(st.get("net_day", 0.0))])
	if parts.is_empty():
		return "Grenier"
	return "Grenier — " + " · ".join(parts)

## SÉPARATEUR DE BLOC (audit UI-2 : « regrouper en 4 blocs ») — barre verticale ÉPAISSE
## et opaque, à distinguer du filet fin (alpha 0.22) que chaque _cell pose déjà entre
## ses propres cellules internes. Pas de micro-label textuel : la barre ne fait que
## 48 px de haut (Frame.TOPBAR_H, hors fichiers autorisés) — une 3e ligne de texte y
## serait illisible (< 10 px), ce que l'audit lisibilité (point 1) proscrit justement ;
## on prend l'alternative offerte par la mission (« OU séparateur simple »).
func _block_sep(px: float) -> float:
	px += 8.0
	VKit.fill(self, Rect2(px, 5.0, 2.0, H - 10.0),
		Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.55))
	return px + 2.0 + 14.0

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
		tip: String = "", vcol: Color = Color(0, 0, 0, 0)) -> float:
	var rid := -1
	if icon == "" and typeof(rid_or_val) == TYPE_INT:
		rid = int(rid_or_val)
	if rid >= 0:
		var spr: Texture2D = UIKit.resource_sprite(rid, "")
		if spr != null:
			draw_texture_rect(spr, Rect2(px, (H - 22.0) * 0.5, 22, 22), false)
	elif icon != "":
		UIKit.draw_icon(self, icon, Vector2(px, (H - 22.0) * 0.5), 22)
	var tx := px + 26.0
	VKit.text(self, Vector2(tx, 6.0), vcol if vcol.a > 0.0 else VKit.COL_PARCH, val)
	var wv := VKit.text_w(val)
	var wd := 0.0
	if dtxt != "":
		VKit.text(self, Vector2(tx, 26.0), VKit.sense(0.85) if dpos else VKit.sense(0.12), dtxt, VKit.FS_SMALL)
		wd = VKit.text_w(dtxt, VKit.FS_SMALL)
	var cw := 26.0 + maxf(wv, wd) + 10.0
	if tip != "":
		_tips.append([Rect2(px - 4.0, 0.0, cw + 8.0, H), tip])
	VKit.fill(self, Rect2(px + cw, 9.0, 1.0, H - 20.0),
		Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.22))
	return px + cw + 10.0

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
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	var ww := size.x
	# barre PLEINE LARGEUR : cuir sombre + liseré or en bas (cadre franc, pas arrondi)
	VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, H - 2, ww, 2), VKit.COL_GOLD)
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

		# ═══ BLOC ROYAUME : nom · trésor · revenu net annuel · population ═══
		var nom := String(ci["nom"])
		VKit.text(self, Vector2(px, cy), VKit.COL_GOLD, nom); px += VKit.text_w(nom) + 18
		px = _cell(px, "fine_coin", "", _grp(ci["or"]), _delta_txt(_d_gold), _d_gold >= 0.0,
			_treasury_tip(w, me))
		var _net := 0.0
		if w.has_method("budget_summary"):
			_net = float((w.budget_summary(me) as Dictionary).get("net", 0.0))
		px = _cell(px, "tax_ledger", "", "%+d" % int(round(_net)), "", _net >= 0.0,
			_net_income_tip(w, me), VKit.sense(0.85) if _net >= 0.0 else VKit.sense(0.12))
		# Population : provinces colonisées + stabilité + colonisation en chantier
		# DÉMOTÉES ici (retour joueur « topbar surchargé ») — le mot avant le chiffre,
		# jamais une coordonnée moteur nue.
		var _pop_tip := "Population — %d province(s) colonisée(s) · %s" % [
			w.country_province_count(me), _gauge_line(ci, CPTips, "stabilite")]
		var _dp_txt := _delta_txt(float(_d_pop))
		if _dp_txt != "":
			_pop_tip += " — %s ce mois" % _dp_txt
		if w.has_method("colony_status"):
			var cs: Dictionary = w.colony_status()
			if bool(cs.get("active", false)):
				var tot := maxi(1, int(cs.get("total_days", 1)))
				var pct := int(round(100.0 * float(tot - int(cs.get("days_left", 0))) / float(tot)))
				_pop_tip += " · colonie en chantier %d %%" % pct
		px = _cell(px, "population_group", "", _grp(ci["pop"]), _delta_txt(float(_d_pop)), _d_pop >= 0,
			_pop_tip)
		px = _block_sep(px)

		# ═══ BLOC ÉCONOMIE : nourriture (+ rupture) · recherche. Les 5 cellules de
		#     MATIÈRES BRUTES bois/argile/pierre/fer/armes vivent dans le tiroir
		#     Économie/Stocks ; prospérité démotée au survol du Revenu net ci-dessus. ═══
		var short := worst_shortage(w, me)
		var _food_dtxt := ""
		var _food_pos := true
		var _food_vcol := Color(0, 0, 0, 0)
		var _food_full_tip := _food_tip(w, me)
		if not short.is_empty():
			var djs := int(short["days"])
			var sname := String(short["name"])
			if _FOOD_NAMES.has(sname):
				# la pénurie EST vivrière : elle se lit directement sur la cellule
				# (UI-5 : le texte chiffré ET le symbole ▼ portent le même sens).
				_food_dtxt = "rupture %d j" % djs
				_food_pos = false
				_food_vcol = VKit.sense(0.10)
				_food_full_tip += "\nPénurie — %s : rupture prévue dans %d jour(s)" % [sname, djs]
			else:
				# pénurie d'une AUTRE ressource (déjà relayée par les alertes) :
				# secondaire ici, une ligne dans le survol du grenier.
				_food_full_tip += "\nAutre pénurie — %s : rupture prévue dans %d jour(s) (voir alertes)" % [sname, djs]
		if w.has_method("country_food"):
			px = _cell(px, "fine_grain", "", _grp(int(w.country_food(me))), _food_dtxt, _food_pos,
				_food_full_tip, _food_vcol)
		var sx0 := px
		var _savoir_tip := String(CPTips.get("savoir", "Savoir"))
		var _metab := int(ci.get("metab_pct", 0))
		if _metab > 0:
			_savoir_tip += " — métabolisation +%d%% recherche" % _metab
		_savoir_tip += " · %s (clic : l'arbre de technologie)" % _gauge_line(ci, CPTips, "prosperite")
		px = _cell(px, "fine_knowledge", "", "%d" % int(ci["savoir"]), "", true, _savoir_tip)
		_savoir_rect = Rect2(sx0 - 4, 0, px - sx0, H)
		px = _block_sep(px)

		# ═══ BLOC POLITIQUE : Influence · Corruption. Légitimité/cohésion/bonheur/
		#     factions/tension de coup DÉMOTÉS au survol (retour joueur : ne garder
		#     QUE les 2 permanents chiffrés — le détail politique reste à un clic,
		#     jamais perdu). ═══
		var infl := int(ci.get("influence", 0))
		px = _cell(px, "influence_compass", "", str(infl), "", true,
			"Influence — %s · %s" % [_gauge_line(ci, CPTips, "legitimite"), _gauge_line(ci, CPTips, "cohesion")])
		var cor := int(fx.get("corruption", 0))
		var cor_tip := "Corruption"
		if not _happy_tip.is_empty():
			cor_tip += " — " + _happy_tip
		var coup := int(fx.get("coup", 0))
		if w.has_method("country_factions"):
			cor_tip += "\nTension de coup %d" % coup
			var flist := []
			for fe in fx.get("list", []):
				var fnm := String(fe.get("name", ""))
				var part := int(fe.get("part", 0))
				var dom := bool(fe.get("dominant", false))
				var g := int(fe.get("grief", 0))
				var fline := "%s %d %%" % [fnm, part]
				if dom:
					fline += " ★"
				if g > 0:
					fline += " (rancœur %d)" % g
				flist.append(fline)
			if not flist.is_empty():
				cor_tip += " · Factions : " + " · ".join(flist)
		px = _cell(px, "corruption_coin", "", str(cor), "", true, cor_tip,
			VKit.sense(0.20) if (coup >= 45 or cor >= 50) else Color(0, 0, 0, 0))
		content_end = px

	# séparateur visuel avant le BLOC TEMPS — ANCRÉ au contenu RÉELLEMENT dessiné (pas
	# une position fixe) : un contenu politique long (pays à beaucoup de factions, longue
	# pénurie nommée) ne doit JAMAIS chevaucher le chip Engager ci-dessous (capturé en
	# capture d'écran — le repli `_age_rect` en tient compte aussi, cf. plus bas).
	if content_end > 16.0:
		content_end = _block_sep(content_end)

	# ═══ BLOC TEMPS : âge/chip Engager · date (date_chip) · vitesse (audit UI-2) ═══
	# LA DATE vit dans son PROPRE contrôle (_date, rafraîchi CHAQUE JOUR — le topbar,
	# lui, reste à la cadence mensuelle anti-danse : sans ça le compteur sautait par
	# paquets de 8-9 jours entre deux redraws, retour joueur 2026-07-10). On ne fait
	# ici que RÉSERVER sa place (largeur max fixe) pour ancrer le chip d'âge.
	var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
	var dtx0 := ww - 116.0 - dtw - 18.0

	# §7 — ENGAGEMENT D'ÂGE : un âge s'est levé et le joueur ne l'a pas engagé →
	# chip ambre cliquable (l'IA s'engage auto ; le joueur choisit — verbe CMD_AGE_ENGAGE).
	# Chip PLAT (les bandes parchemin sont retirées — panneaux plats 2026-07-10).
	_age_rect = Rect2()
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			var lab := "Engager : %s" % String(ag.get("name", ""))
			var aw := VKit.text_w(lab) + 34.0
			# GARDE ANTI-CHEVAUCHEMENT (audit UI-2) : le chip reste ANCRÉ à gauche de
			# la date (jamais poussé DESSUS — 1re tentative fautive, capturée) ; quand
			# le bloc POLITIQUE arrive trop près, c'est le LABEL qui se TRONQUE
			# (« Engager : Âge du Comm… ») — le nom complet vit au survol (tip).
			var avail := dtx0 - 14.0 - content_end
			if aw > avail:
				# tronque jusqu'au minimum « Engager… » (~105 px) — un léger recouvrement
				# résiduel du dernier blason reste possible en cas extrême (le chip est
				# opaque et dessiné PAR-DESSUS : lisible et cliquable, jamais l'inverse).
				while lab.length() > 7 and VKit.text_w(lab + "…") + 34.0 > avail:
					lab = lab.substr(0, lab.length() - 1)
				lab += "…"
				aw = VKit.text_w(lab) + 34.0
			_age_rect = Rect2(dtx0 - aw - 14.0, 6, aw, H - 12)
			VKit.fill(self, _age_rect, Color(0.24, 0.17, 0.07, 0.95))
			VKit.box(self, _age_rect, Color(0.90, 0.72, 0.34))
			UIKit.draw_icon(self, "fine_age", Vector2(_age_rect.position.x + 6, cy - 2), 16)
			VKit.text(self, Vector2(_age_rect.position.x + 26, cy), Color(0.90, 0.72, 0.34), lab)
			# push_front : le chip est dessiné PAR-DESSUS le contenu → son tip doit GAGNER
			# le hit-test (sinon un blason recouvert répondrait au survol du chip).
			_tips.push_front([_age_rect,
				"Un âge s'est levé : %s — clic pour l'ENGAGER (une fois par âge)" % String(ag.get("name", ""))])

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
		VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		var g: String = glyphs[i]
		VKit.text(self, Vector2(r.position.x + (r.size.x - VKit.text_w(g, VKit.FS_SMALL)) * 0.5,
			r.position.y + (r.size.y - 16.0) * 0.5), VKit.COL_PANEL if active else VKit.COL_PARCH, g, VKit.FS_SMALL)
		_speed_btns.append([r, i])
		_tips.append([r, String(stips[i])])

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _savoir_rect.has_point(event.position):
			tech_requested.emit()
		elif _age_rect.size.x > 0 and _age_rect.has_point(event.position):
			if Sim.world != null and Sim.world.has_method("player_age_engage"):
				Sim.world.player_age_engage()   # enfilé ; le chip s'éteint au drain (engaged=true)
				queue_redraw()
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
