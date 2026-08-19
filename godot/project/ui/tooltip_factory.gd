extends RefCounted
## TooltipFactory — LA formule partagée valeur→icône→fiche d'un BIEN (revue 2026-07-21, #3).
## Un panneau qui montre une ressource au survol appelle CES fonctions — il ne recompose
## jamais sa propre fiche stock/marché (la duplication _stock_info_card/_market_info_card
## de sidebar_drawer est née ici). Display-only, typé (règle #2), STATIC (aucun état).
## Le glossaire des CONCEPTS (mots du jeu) reste ui/concepts.gd : factory = les DONNÉES
## d'un bien, concepts = la DÉFINITION d'un mot.

const InfoRef = preload("res://ui/info_ref.gd")

## séparateur de milliers (l'ex-_grp local dupliqué) — « 12 400 », signe conservé.
static func grp(n) -> String:
	var v := int(n)
	var s := str(absi(v))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" + out) if v < 0 else out

## LA CHECKLIST DE REFUS (décision joueur 2026-07-21, motif CK3/KoH2) : rend une liste
## de conditions [{label, ok}] en ✓/✗ — le « pourquoi c'est grisé » PARTAGÉ par tous les
## refus (diplo, emprunt, esclavage, construction…). `header` = une ligne de résumé/aide
## optionnelle en tête. Vide → chaîne vide (l'appelant retombe sur son texte legacy).
static func gate_checklist(conds: Array, header: String = "") -> String:
	if conds.is_empty():
		return header
	var lines: Array = []
	if header != "":
		lines.append(header)
	for c in conds:
		lines.append("%s %s" % ["✓" if bool(c.get("ok", false)) else "✗", String(c.get("label", "?"))])
	return "\n".join(lines)

static func coverage_text(coverage_days: int) -> String:
	if coverage_days < 0:
		return "stable ou excédentaire"
	return "> 1 an" if coverage_days >= 366 else "%d jours" % coverage_days

## le SURVOL court : « Fer — stock 1 240 · +12/mois · tendu · couverture 40 j »
static func stock_tip(st: Dictionary) -> String:
	var net_month := float(st.get("net_day", 0.0)) * 30.0
	var tip := "%s — stock %s · %+d/mois · %s" % [String(st.get("name", "Bien")),
		grp(st.get("stock", 0)), int(round(net_month)), String(st.get("marche", ""))]
	var coverage := int(st.get("coverage_days", -1))
	if coverage >= 0:
		tip += " · couverture %s" % ("​>1 an" if coverage >= 366 else "%d j" % coverage)
	return tip

## la FICHE stock (onglet Économie) : production/consommation/couverture/prix + territoires.
static func stock_info_card(world: Object, st: Dictionary) -> Dictionary:
	var net_month := float(st.get("net_day", 0.0)) * 30.0
	var lines: Array = [
		{"label": "Production", "value": "%.1f / mois" % float(st.get("supply_month", 0.0)), "tone": "positive"},
		{"label": "Consommation", "value": "%.1f / mois" % float(st.get("demand_month", 0.0)), "tone": "negative"},
		{"label": "Couverture", "value": coverage_text(int(st.get("coverage_days", -1)))},
		{"label": "Prix moyen", "value": "%.2f couronnes" % float(st.get("price", 0.0))},
	]
	var actions: Array = [{"label": "Ouvrir ce bien au Marché", "request": InfoRef.request(
		InfoRef.make(InfoRef.RESOURCE, int(st.get("res_id", -1))), "sidebar", {"tab": 3})}]
	var territory := territory_detail(world, st)
	lines.append_array(territory.get("lines", []))
	actions.append_array(territory.get("actions", []))
	return {
		"title": String(st.get("name", "Bien")),
		"state": "stock %s · marché %s" % [grp(st.get("stock", 0)), String(st.get("marche", ""))],
		"trend": "%+d / mois" % int(round(net_month)),
		"trend_tone": "positive" if net_month >= 0.0 else "negative",
		"lines": lines,
		"actions": actions,
	}

## la FICHE marché (onglet Marché) : le stock + le DEVIS d'achat (local/mondial).
## `category_word` : le mot de catégorie résolu par l'appelant (sa table locale).
static func market_info_card(world: Object, st: Dictionary, quote: Dictionary, category_word: String) -> Dictionary:
	var net_month := float(st.get("net_day", 0.0)) * 30.0
	var lines: Array = [
		{"label": "Stock national", "value": grp(st.get("stock", 0))},
		{"label": "Production", "value": "%.1f / mois" % float(st.get("supply_month", 0.0)), "tone": "positive"},
		{"label": "Consommation", "value": "%.1f / mois" % float(st.get("demand_month", 0.0)), "tone": "negative"},
		{"label": "Couverture", "value": coverage_text(int(st.get("coverage_days", -1)))},
	]
	if bool(quote.get("valid", false)):
		var margin := float(quote.get("margin", 1.0))
		var hub_region := int(quote.get("hub_region", -1))
		var local_qty := int(quote.get("local_qty", 0))
		var global_qty := int(quote.get("global_qty", 0))
		lines.append({"label": "Approvisionnement", "value": "devis pour %d unités" % int(quote.get("request_qty", 10)), "tone": "heading"})
		lines.append({"label": "Centre proche", "value": "aucun marché accessible" if hub_region < 0 else \
			"%s · %s disponibles · marge ×%.2f" % [String(quote.get("hub_name", "Centre")),
				grp(int(float(quote.get("local_available", 0.0)))), margin],
			"tone": "negative" if hub_region < 0 else ""})
		lines.append({"label": "Achat local", "value": "indisponible" if local_qty <= 0 else \
			"%d unités · ~%d couronnes" % [local_qty, int(round(float(quote.get("local_cost", 0.0))))],
			"tone": "negative" if local_qty <= 0 else ""})
		var global_access := bool(quote.get("global_access", false))
		lines.append({"label": "Réseau mondial", "value": "accès fermé" if not global_access else \
			"%s disponibles · marge ×%.2f" % [grp(int(float(quote.get("global_available", 0.0)))), margin * 2.0],
			"tone": "negative" if not global_access else ""})
		if global_access:
			lines.append({"label": "Devis mondial", "value": "indisponible" if global_qty <= 0 else \
				"%d unités · ~%d couronnes" % [global_qty, int(round(float(quote.get("global_cost", 0.0))))],
				"tone": "negative" if global_qty <= 0 else ""})
		lines.append({"label": "Puissance commerciale", "value": "%.0f unités restantes ce mois" % float(quote.get("commerce_remaining", 0.0)), "tone": "dim"})
	var actions: Array = [{"label": "Voir le stock national", "request": InfoRef.request(
		InfoRef.make(InfoRef.RESOURCE, int(st.get("res_id", -1))), "sidebar", {"tab": 2})}]
	var territory := territory_detail(world, st)
	lines.append_array(territory.get("lines", []))
	actions.append_array(territory.get("actions", []))
	return {
		"title": String(st.get("name", "Bien")),
		"state": "%s · marché %s · %.2f couronnes" % [category_word,
			String(st.get("marche", "")), float(st.get("price", 0.0))],
		"trend": "%+d / mois" % int(round(net_month)),
		"trend_tone": "positive" if net_month >= 0.0 else "negative",
		"lines": lines,
		"actions": actions,
	}

## P6 — répond à « où ? » : premier territoire producteur et consommateur, ouvrables sur
## la carte. Le classement vient du lecteur moteur `stock_regions` ; aucune géographie
## n'est reconstruite ici.
static func territory_detail(world: Object, st: Dictionary) -> Dictionary:
	var out := {"lines": [], "actions": []}
	if world == null or not world.has_method("stock_regions"):
		return out
	var rows: Array = world.stock_regions(int(world.player()), int(st.get("res_id", -1)))
	var producers: Array = rows.filter(func(row): return float(row.get("supply_month", 0.0)) > 0.05)
	var consumers: Array = rows.filter(func(row): return float(row.get("demand_month", 0.0)) > 0.05)
	producers.sort_custom(func(a, b): return float(a.get("supply_month", 0.0)) > float(b.get("supply_month", 0.0)))
	consumers.sort_custom(func(a, b): return float(a.get("demand_month", 0.0)) > float(b.get("demand_month", 0.0)))
	var linked := {}
	if not producers.is_empty():
		var p: Dictionary = producers[0]
		out["lines"].append({"label": "Produit surtout à", "value": "%s · %.1f/mois" % [String(p.get("name", "?")), float(p.get("supply_month", 0.0))], "tone": "positive"})
		var pr := int(p.get("region", -1))
		if pr >= 0:
			linked[pr] = true
			out["actions"].append({"label": "Voir %s sur la carte" % String(p.get("name", "la région")),
				"request": InfoRef.request(InfoRef.make(InfoRef.REGION, pr), "map")})
	if not consumers.is_empty():
		var c: Dictionary = consumers[0]
		out["lines"].append({"label": "Consommé surtout à", "value": "%s · %.1f/mois" % [String(c.get("name", "?")), float(c.get("demand_month", 0.0))], "tone": "negative"})
		var cr := int(c.get("region", -1))
		if cr >= 0 and not linked.has(cr):
			out["actions"].append({"label": "Voir %s sur la carte" % String(c.get("name", "la région")),
				"request": InfoRef.request(InfoRef.make(InfoRef.REGION, cr), "map")})
	return out
