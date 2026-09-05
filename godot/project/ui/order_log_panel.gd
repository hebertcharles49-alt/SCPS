extends Control
## Journal des ordres joueur — lecture du feedback transient du moteur.
## Un ordre en file reste visible à la pause ; son état devient exécuté/démarré
## ou refusé seulement après le drain, avec le motif fourni par l'API.

const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")

const PW := 440.0
const PH := 360.0
const MAX_ROWS := 8

const VERB_KEYS := {
	0: "T_ORDER_VERB_NONE", 1: "T_ORDER_VERB_BUILD", 2: "T_ORDER_VERB_RECRUIT",
	3: "T_ORDER_VERB_LEVY", 4: "T_ORDER_VERB_RESEARCH", 5: "T_ORDER_VERB_WAR",
	6: "T_ORDER_VERB_PEACE", 7: "T_ORDER_VERB_ALLIANCE", 8: "T_ORDER_VERB_PACT",
	9: "T_ORDER_VERB_EMBARGO", 10: "T_ORDER_VERB_REPRESS", 11: "T_ORDER_VERB_ASSIMILATE",
	12: "T_ORDER_VERB_PURGE", 13: "T_ORDER_VERB_COUNCIL_HIRE", 14: "T_ORDER_VERB_COUNCIL_DISMISS",
	15: "T_ORDER_VERB_ROUTE", 16: "T_ORDER_VERB_MARKET_BUY", 17: "T_ORDER_VERB_MARKET_SELL",
	18: "T_ORDER_VERB_CAMPAIGN", 19: "T_ORDER_VERB_REFILL", 20: "T_ORDER_VERB_NAVY",
	21: "T_ORDER_VERB_DISBAND", 22: "T_ORDER_VERB_ALLOC_RAW", 23: "T_ORDER_VERB_ALLOC_BLD",
	24: "T_ORDER_VERB_ALLOC_INPUT", 25: "T_ORDER_VERB_ALLOC_AUTO", 26: "T_ORDER_VERB_AGE",
	27: "T_ORDER_VERB_COLONIZE", 28: "T_ORDER_VERB_MIGRATION", 29: "T_ORDER_VERB_MANUF",
	30: "T_ORDER_VERB_EVENT", 31: "T_ORDER_VERB_DECREE", 32: "T_ORDER_VERB_MANUMIT",
	33: "T_ORDER_VERB_SLAVE_BUY", 34: "T_ORDER_VERB_SLAVE_SELL", 35: "T_ORDER_VERB_POP_TRANSFER",
	36: "T_ORDER_VERB_FABRICATE", 37: "T_ORDER_VERB_COUNCIL_PAY", 38: "T_ORDER_VERB_RAID",
	39: "T_ORDER_VERB_MOVE", 40: "T_ORDER_VERB_CORPS_RAISE", 41: "T_ORDER_VERB_CORPS_SPLIT",
	42: "T_ORDER_VERB_CORPS_MERGE", 43: "T_ORDER_VERB_CORPS_MOVE", 44: "T_ORDER_VERB_CORPS_REFILL",
	45: "T_ORDER_VERB_CORPS_DISBAND", 46: "T_ORDER_VERB_BUDGET", 47: "T_ORDER_VERB_PEACE_OFFER",
	48: "T_ORDER_VERB_MANUF_LEVEL", 49: "T_ORDER_VERB_DEMOLISH", 50: "T_ORDER_VERB_BANKRUPTCY",
	51: "T_ORDER_VERB_REPAY", 52: "T_ORDER_VERB_BORROW_CLASS", 53: "T_ORDER_VERB_REQUEST_LOAN",
	54: "T_ORDER_VERB_RENOVER", 55: "T_ORDER_VERB_CORPS_SPLIT_COMP", 56: "T_ORDER_VERB_DESSEIN",
	57: "T_ORDER_VERB_DOCT_ADOPT", 58: "T_ORDER_VERB_DOCT_IDEA", 59: "T_ORDER_VERB_DOCT_ABANDON"
}
const BUDGET_FAMILY_KEYS := {0: "T_ORDER_BUDGET_TAXES", 1: "T_ORDER_BUDGET_SPENDING"}
const BUDGET_TAX_KEYS := {
	0: "T_ORDER_BUDGET_LABORER", 1: "T_ORDER_BUDGET_BOURGEOIS", 2: "T_ORDER_BUDGET_ELITE"
}
const BUDGET_SPENDING_KEYS := {
	0: "T_ORDER_BUDGET_INVEST", 1: "T_ORDER_BUDGET_UPKEEP", 2: "T_ORDER_BUDGET_ARMY",
	3: "T_ORDER_BUDGET_NAVY", 4: "T_ORDER_BUDGET_ROADS", 5: "T_ORDER_BUDGET_MINT",
	6: "T_ORDER_BUDGET_DEBASE"
}

var _entries: Array = []
var _hits: Array = []

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	custom_minimum_size = size
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 48.0)
	add_to_group("draggable")
	if Sim.has_signal("command_feedback_changed"):
		Sim.command_feedback_changed.connect(_on_feedback_changed)
	if Sim.has_signal("generated"):
		Sim.generated.connect(_on_generated)
	hide()

func _on_feedback_changed() -> void:
	if visible:
		refresh()

func _on_generated() -> void:
	_entries.clear()
	if visible:
		refresh()

func open() -> void:
	if not Sim.game_on or Sim.world == null:
		return
	visible = true
	refresh()

func close_panel() -> void:
	visible = false

func refresh() -> void:
	_entries = Sim.command_feedback() if Sim.has_method("command_feedback") else []
	if _entries.size() > MAX_ROWS:
		_entries = _entries.slice(_entries.size() - MAX_ROWS)
	queue_redraw()

func _draw() -> void:
	_hits.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	VKit.text(self, Vector2(22, 15), VKit.COL_PARCH, tr("T_ORDER_TITLE"), VKit.FS_BIG)
	var close_r := Rect2(PW - 42.0, 12.0, 25.0, 25.0)
	VKit.fill(self, close_r, VKit.COL_PANEL2)
	VKit.box(self, close_r, VKit.COL_EDGE)
	VKit.text(self, close_r.position + Vector2(7, 2), VKit.COL_PARCH, "×", VKit.FS)
	_hits.append({"rect": close_r, "act": "close"})
	VKit.fill(self, Rect2(18, 48, PW - 36, 1), VKit.COL_EDGE)
	if _entries.is_empty():
		VKit.text_wrapped(self, Vector2(24, 82), VKit.COL_DIM,
			tr("T_ORDER_EMPTY"), PW - 48, 3, VKit.FS)
		return
	var y := 68.0
	for entry in _entries:
		var row := Rect2(20, y, PW - 40, 31)
		VKit.list_row_bg(self, row, int(entry.get("id", 0)), false, true)
		var status := _status_text(entry)
		var col := _status_color(entry)
		VKit.text_wrapped(self, Vector2(28, y + 4), col,
			tr("T_ORDER_LINE") % [int(entry.get("id", 0)), _verb_text(int(entry.get("verb", 0))),
				_status_line(entry, status)], PW - 56, 1, VKit.FS_SMALL)
		var detail := _reason_text(int(entry.get("reason", 0))) if int(entry.get("status", 0)) == 2 else _detail_text(entry)
		if detail != "":
			VKit.text_wrapped(self, Vector2(28, y + 17), VKit.COL_DIM, detail, PW - 58, 1, VKit.FS_SMALL)
		y += 35.0

func _status_text(entry: Dictionary) -> String:
	match int(entry.get("status", 0)):
		0: return tr("T_ORDER_PENDING")
		1: return tr("T_ORDER_EXECUTED")
		2: return tr("T_ORDER_REFUSED")
	return tr("T_ORDER_PENDING")

func _outcome_text(entry: Dictionary) -> String:
	if int(entry.get("status", 0)) == 0:
		return tr("T_ORDER_RESUME") if Sim.speed_index == 0 else tr("T_ORDER_NEXT_TICK")
	match int(entry.get("outcome", 0)):
		1: return tr("T_ORDER_STARTED")
		2: return tr("T_ORDER_COMPLETED")
		3: return tr("T_ORDER_MUTATED")
	return ""

func _status_line(entry: Dictionary, status: String) -> String:
	var outcome := _outcome_text(entry)
	return status if outcome == "" else "%s · %s" % [status, outcome]

func _reason_text(reason: int) -> String:
	var key := "T_ORDER_REASON_%d" % reason
	return tr(key)

func _verb_text(verb: int) -> String:
	return tr(String(VERB_KEYS.get(verb, "T_ORDER_VERB_UNKNOWN")))

func _detail_text(entry: Dictionary) -> String:
	var verb := int(entry.get("verb", 0))
	var a0 := int(entry.get("a0", 0))
	match verb:
		1: return tr("T_ORDER_TARGET_PROVINCE") % _province_name(int(entry.get("a1", -1)))
		4: return tr("T_ORDER_RESEARCH_CANCEL") if a0 < 0 else tr("T_ORDER_TARGET_TECH") % _tech_name(a0)
		5, 6, 7, 8, 9, 28, 36, 47, 53: return tr("T_ORDER_TARGET_COUNTRY") % _country_name(a0)
		10, 11, 12, 29, 48, 49, 54: return tr("T_ORDER_TARGET_PROVINCE") % _province_name(a0)
		15: return tr("T_ORDER_TARGET_REGION_PAIR") % [_region_name(a0), _region_name(int(entry.get("a1", -1)))]
		16, 17: return tr("T_ORDER_TARGET_PROVINCE_QUANTITY") % [_province_name(a0), int(entry.get("a2", 0))]
		18: return tr("T_ORDER_TARGET_REGION_PAIR") % [_region_name(a0), _region_name(int(entry.get("a1", -1)))]
		22, 23, 24, 25: return tr("T_ORDER_TARGET_PROVINCE") % _province_name(a0)
		26, 31, 32, 37, 50, 51, 52, 57, 58, 59: return _numeric_detail(entry)
		46: return _budget_detail(entry)
		27: return tr("T_ORDER_TARGET_PROVINCE") % _province_name(a0)
		38:
			var detail := tr("T_ORDER_TARGET_PROVINCE") % _province_name(a0)
			if int(entry.get("status", 0)) == 1:
				# Le butin est porté par amount au feedback du drain ; afficher aussi
				# zéro évite de confondre un raid exécuté avec un raid sans résultat.
				detail += " · " + (tr("T_ORDER_RAID_LOOT") % int(round(float(entry.get("amount", 0.0)))))
				if Sim.world != null and Sim.world.has_method("can_raid_coast"):
					var legal: Dictionary = Sim.world.can_raid_coast(a0)
					var cd := int(legal.get("cd_days", 0))
					if cd > 0:
						detail += " · cooldown %d j" % cd
			return detail
		33, 34: return tr("T_ORDER_TARGET_PROVINCE_QUANTITY") % [_province_name(a0), int(entry.get("a1", 0))]
		35: return tr("T_ORDER_TRANSFER_DETAIL") % [_province_name(a0), _province_name(int(entry.get("a1", -1))), int(entry.get("a3", 0))]
		39: return tr("T_ORDER_TARGET_REGION") % _region_name(a0)
		40: return tr("T_ORDER_RAISE_DETAIL") % [a0, _region_name(int(entry.get("a1", -1)))]
		41, 42, 43, 44, 45, 55: return tr("T_ORDER_TARGET_CORPS") % a0
		56: return tr("T_ORDER_TARGET_DESSEIN") % [a0, int(entry.get("a1", 0)), int(entry.get("a2", 0))]
		2: return tr("T_ORDER_RECRUIT_DETAIL") % [a0, int(entry.get("a1", 1))]
	return ""

func _numeric_detail(entry: Dictionary) -> String:
	var verb := int(entry.get("verb", 0))
	match verb:
		3: return tr("T_ORDER_LEVY_DETAIL") % int(entry.get("a0", 0))
		13, 14, 37: return tr("T_ORDER_COUNCIL_DETAIL") % int(entry.get("a0", 0))
		20: return tr("T_ORDER_NAVY_DETAIL") % int(entry.get("a0", 0))
		30: return tr("T_ORDER_EVENT_DETAIL") % [int(entry.get("a0", 0)), int(entry.get("a1", 0))]
		31: return tr("T_ORDER_DECREE_DETAIL") % int(entry.get("a0", 0))
		48, 49: return tr("T_ORDER_TARGET_PROVINCE") % _province_name(int(entry.get("a0", -1)))
		52: return tr("T_ORDER_BORROW_DETAIL") % [int(entry.get("a0", 0)), int(entry.get("a1", 0))]
		57, 59: return tr("T_ORDER_SLOT_DETAIL") % int(entry.get("a0", 0))
		58: return tr("T_ORDER_DOCTRINE_DETAIL") % int(entry.get("a0", 0))
		51: return tr("T_ORDER_AMOUNT_DETAIL") % int(entry.get("a0", 0))
		50, 32: return tr("T_ORDER_NO_TARGET")
	return ""

func _budget_detail(entry: Dictionary) -> String:
	var family := int(entry.get("a0", -1))
	var index := int(entry.get("a1", -1))
	var family_key := String(BUDGET_FAMILY_KEYS.get(family, "T_ORDER_BUDGET_UNKNOWN"))
	var line_key := "T_ORDER_BUDGET_UNKNOWN"
	if family == 0:
		line_key = String(BUDGET_TAX_KEYS.get(index, line_key))
	elif family == 1:
		line_key = String(BUDGET_SPENDING_KEYS.get(index, line_key))
	# Lire la liste réelle valide le couple enum/ligne avant de l'afficher.
	if Sim.world != null and Sim.world.has_method("budget_controls"):
		var controls: Dictionary = Sim.world.budget_controls(int(Sim.world.player()))
		var rows: Array = controls.get("taxes", []) if family == 0 else controls.get("spending", [])
		var found := false
		for raw in rows:
			if raw is Dictionary and int(raw.get("id", -1)) == index:
				found = true
				break
		if not found:
			line_key = "T_ORDER_BUDGET_UNKNOWN"
	return tr("T_ORDER_BUDGET_DETAIL") % [tr(family_key), tr(line_key)]

func _province_name(pid: int) -> String:
	if Sim.world != null and pid >= 0 and Sim.world.has_method("province_info"):
		var p: Dictionary = Sim.world.province_info(pid)
		if bool(p.get("valide", false)):
			return String(p.get("nom", ""))
	return tr("T_ORDER_UNKNOWN_PLACE")

func _country_name(cid: int) -> String:
	if Sim.world != null and cid >= 0 and Sim.world.has_method("country_info"):
		var c: Dictionary = Sim.world.country_info(cid)
		if bool(c.get("valide", false)):
			return String(c.get("nom", ""))
	return tr("T_ORDER_UNKNOWN_COUNTRY")

func _region_name(rid: int) -> String:
	if Sim.world != null and rid >= 0 and Sim.world.has_method("region_label"):
		var label: String = String(Sim.world.region_label(rid))
		if label != "":
			return label
	return tr("T_ORDER_UNKNOWN_PLACE")

func _tech_name(index: int) -> String:
	if Sim.world != null and index >= 0 and Sim.world.has_method("tech_nodes"):
		var nodes: Array = Sim.world.tech_nodes()
		if index < nodes.size() and nodes[index] is Dictionary:
			var node: Dictionary = nodes[index]
			var name: String = String(node.get("name", ""))
			if name != "":
				return name
	return tr("T_ORDER_UNKNOWN_TECH")

func _status_color(entry: Dictionary) -> Color:
	match int(entry.get("status", 0)):
		1: return VKit.sense(0.82)
		2: return VKit.sense(0.15)
	return VKit.COL_GOLD

func _gui_input(e: InputEvent) -> void:
	if not (e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT):
		return
	for h in _hits:
		if (h["rect"] as Rect2).has_point(e.position):
			if String(h.get("act", "")) == "close":
				close_panel()
			accept_event()
			return
