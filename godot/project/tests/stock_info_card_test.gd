extends Node

const InfoRef = preload("res://ui/info_ref.gd")
const SidebarDrawer = preload("res://ui/sidebar_drawer.gd")

func _ready() -> void:
	var drawer = SidebarDrawer.new()
	var card: Dictionary = drawer._stock_info_card({
		"name": "Fer", "res_id": 13, "stock": 90, "net_day": -2.0,
		"supply_month": 40.0, "demand_month": 100.0,
		"coverage_days": 45, "price": 2.5, "marche": "pénurie",
	})
	_check(String(card.get("title", "")) == "Fer", "titre de ressource absent")
	_check(String(card.get("trend", "")).contains("-60"), "flux mensuel net incorrect")
	_check(String(card.get("trend_tone", "")) == "negative", "déficit non signalé")
	var lines: Array = card.get("lines", [])
	_check(lines.size() >= 4, "décomposition production/consommation/couverture/prix incomplète")
	_check(String(lines[0].get("value", "")).contains("40.0"), "production brute absente")
	_check(String(lines[1].get("value", "")).contains("100.0"), "consommation brute absente")
	var actions: Array = card.get("actions", [])
	_check(actions.size() >= 1, "deep-link marché absent")
	var request: Dictionary = actions[0].get("request", {})
	_check(InfoRef.request_key(request).begins_with("resource:13"), "deep-link vers le mauvais bien")
	var market_card: Dictionary = drawer._market_info_card({
		"name": "Fer", "res_id": 13, "stock": 90, "net_day": -2.0,
		"supply_month": 40.0, "demand_month": 100.0,
		"coverage_days": 45, "price": 2.5, "marche": "pénurie",
	})
	_check(String(market_card.get("state", "")).contains("2.50 or"), "prix de marché absent")
	var market_lines: Array = market_card.get("lines", [])
	_check(market_lines.size() >= 4, "fiche marché incomplète")
	_check(String(market_lines[0].get("value", "")) == "90", "stock national absent du marché")
	var market_actions: Array = market_card.get("actions", [])
	_check(market_actions.size() >= 1, "retour vers Stocks absent")
	var stock_request: Dictionary = market_actions[0].get("request", {})
	_check(int((stock_request.get("context", {}) as Dictionary).get("tab", -1)) == 2,
		"retour vers le mauvais onglet")
	var sourced: Dictionary = drawer._market_info_card({
		"name": "Fer", "res_id": 13, "stock": 90, "net_day": -2.0,
		"supply_month": 40.0, "demand_month": 100.0,
		"coverage_days": 45, "price": 2.5, "marche": "pénurie",
	}, {
		"valid": true, "request_qty": 10, "hub_region": 4, "hub_name": "Ligue de Sore",
		"margin": 1.25, "local_available": 80.0, "local_qty": 10, "local_cost": 31.25,
		"global_access": true, "global_available": 600.0, "global_qty": 10,
		"global_cost": 62.5, "commerce_remaining": 44.0,
	})
	var sourced_lines: Array = sourced.get("lines", [])
	_check(sourced_lines.size() >= 10, "chaîne d'approvisionnement incomplète")
	_check(String(sourced_lines[5].get("value", "")).contains("Ligue de Sore"), "Centre proche absent")
	_check(String(sourced_lines[6].get("value", "")).contains("31 or"), "devis local absent")
	_check(String(sourced_lines[7].get("value", "")).contains("×2.50"), "double marge mondiale absente")
	_check(String(sourced_lines[8].get("value", "")).contains("63 or"), "devis mondial absent")
	if Sim.world != null and Sim.world.has_method("stock_regions"):
		var live_stocks: Array = Sim.world.country_stocks(int(Sim.world.player()))
		if not live_stocks.is_empty():
			var territory: Dictionary = drawer._stock_territory_detail(live_stocks[0])
			var territory_lines: Array = territory.get("lines", [])
			_check(territory_lines.size() <= 2, "synthèse territoriale trop longue")
			for action in territory.get("actions", []):
				_check(InfoRef.request_key(action.get("request", {})).begins_with("region:"),
					"deep-link territorial invalide")
	drawer.free()
	print("stock_info_card_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("stock_info_card_test: " + message)
	get_tree().quit(1)
