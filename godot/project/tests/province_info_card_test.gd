extends Node

const InfoRef = preload("res://ui/info_ref.gd")
const ProvincePanel = preload("res://ui/province_panel.gd")

func _ready() -> void:
	var panel = ProvincePanel.new()
	var info := {
		"nom": "Valdor", "ames": 1200, "climat": "tempéré", "relief": "collines",
		"aisance_val": 72, "humeur_val": 35, "agitation": 64,
	}
	var cap := {"statut": "Bourg", "logement_cap": 1000, "service_cap": 1500}
	var summary: Dictionary = panel._province_summary_card(info, cap, {
		"tax_year": 321.0, "defense_pct": 135,
	})
	_check(String(summary.get("trend_tone", "")) == "negative", "agitation critique non signalée")
	var lines: Array = summary.get("lines", [])
	_check(lines.size() == 6, "résumé territorial incomplet")
	_check(String(lines[0].get("value", "")).contains("dépassement 200"), "surpopulation masquée")
	_check(String(lines[4].get("value", "")).contains("321"), "fiscalité absente")
	_check(String(lines[5].get("value", "")).contains("+35%"), "défense absente")
	var actions: Array = summary.get("actions", [])
	_check(actions.size() == 1, "lien économie absent")
	var req: Dictionary = actions[0].get("request", {})
	_check(InfoRef.request_key(req).begins_with("sidebar_tab:0"), "lien économie invalide")

	var satisfaction: Dictionary = panel._province_satisfaction_card(info, {
		"laboureurs": 28, "artisans": 74, "noblesse": 55, "esclaves": -1,
	}, {"value": 64, "causes": [
		{"cause": "Conquête récente", "delta": 18.0, "decay": 2.0},
		{"cause": "Ordre local", "delta": -4.0, "decay": 0.0},
	]})
	var sat_lines: Array = satisfaction.get("lines", [])
	_check(sat_lines.size() == 7, "causes d'agitation incomplètes")
	_check(String(sat_lines[0].get("tone", "")) == "negative", "misère des laboureurs non signalée")
	_check(String(sat_lines[3].get("value", "")) == "absents", "classe absente mal rendue")
	_check(String(sat_lines[5].get("value", "")).contains("résorption 2.0/an"), "résorption absente")
	panel.free()
	print("province_info_card_test: OK")
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("province_info_card_test: " + message)
	get_tree().quit(1)
