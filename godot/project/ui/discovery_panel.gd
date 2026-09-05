extends Control
## Découvertes — aide contextuelle et volontaire.
## Chaque carte est calculée depuis l'état courant et ouvre une surface existante.

const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")

const PW := 420.0
const PH := 430.0
const PAGE_SIZE := 3
signal navigate_requested(request: Dictionary)
signal dessein_requested()

var _cards: Array = []
var _dismissed := {}
var _hits: Array = []
var _cid := -1
var _page := 0

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	custom_minimum_size = size
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 48.0)
	add_to_group("draggable")
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(_on_month_ticked)
	if Sim.has_signal("generated"):
		Sim.generated.connect(_on_generated)
	hide()

func _on_month_ticked(_year: int) -> void:
	if visible:
		refresh()

func _on_generated() -> void:
	_dismissed.clear()
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
	_cards.clear()
	if Sim.world == null:
		queue_redraw()
		return
	_cid = int(Sim.world.player()) if Sim.world.has_method("player") else 0
	# Les cartes sont des portes courtes vers les panneaux déjà maîtrisés. Elles
	# disparaissent seulement quand le joueur les termine ou les masque.
	if Sim.world.has_method("country_capital_region"):
		var cap := int(Sim.world.country_capital_region(_cid))
		if cap >= 0 and not _dismissed.has("capital"):
			_cards.append({"key": "capital", "title": tr("T_DISC_CAPITAL_TITLE"),
				"body": tr("T_DISC_CAPITAL_BODY"),
				"request": InfoRef.request(InfoRef.make(InfoRef.REGION, cap), "map", {"focus_map": true})})
	if Sim.world.has_method("country_research_income") and not _dismissed.has("research"):
		var researchable := false
		if Sim.world.has_method("tech_nodes"):
			for raw in Sim.world.tech_nodes():
				var nd: Dictionary = raw
				if int(nd.get("state", 0)) != 2 and bool(nd.get("allowed", false)):
					researchable = true
					break
		if researchable:
			_cards.append({"key": "research", "title": tr("T_DISC_RESEARCH_TITLE"),
				"body": tr("T_DISC_RESEARCH_BODY"),
				"request": InfoRef.request(InfoRef.make(InfoRef.TECH, -1))})
	var build_pid := _first_owned_province()
	if build_pid >= 0 and not _dismissed.has("build"):
		_cards.append({"key": "build", "title": tr("T_DISC_BUILD_TITLE"),
			"body": tr("T_DISC_BUILD_BODY"),
			"request": InfoRef.request(InfoRef.make(InfoRef.PROVINCE, build_pid), "map", {"focus_map": true})})
	# La piste matières ne se déclenche que sur le forecast moteur exposé par la
	# façade. Elle ouvre le vrai onglet Stocks et sélectionne la ressource urgente.
	if Sim.world.has_method("country_shortages") and not _dismissed.has("shortage"):
		var shortages: Array = Sim.world.country_shortages(_cid)
		if not shortages.is_empty() and shortages[0] is Dictionary:
			var shortage: Dictionary = shortages[0]
			var resource_name := String(shortage.get("nom", ""))
			var runway := float(shortage.get("runway_days", -1.0))
			var runway_text := (tr("T_DISC_SHORTAGE_UNKNOWN") % resource_name) if runway < 0.0 else ("%d" % int(round(runway)))
			_cards.append({"key": "shortage", "title": tr("T_DISC_SHORTAGE_TITLE"),
				"body": (tr("T_DISC_SHORTAGE_BODY") % [resource_name, runway_text]) if runway >= 0.0 else runway_text,
				"request": InfoRef.request(InfoRef.make(InfoRef.RESOURCE, int(shortage.get("res_id", -1))),
					"sidebar", {"tab": 2})})
	# Une fiscalité déficitaire est une décision concrète : le lien arrive dans
	# BudgetPanelV2, là où les curseurs de recettes/dépenses sont réellement actifs.
	if Sim.world.has_method("budget_summary") and not _dismissed.has("budget"):
		var budget: Dictionary = Sim.world.budget_summary(_cid)
		var monthly_net := float(budget.get("monthly_net", budget.get("net", 0.0)))
		if monthly_net < 0.0:
			_cards.append({"key": "budget", "title": tr("T_DISC_BUDGET_TITLE"),
				"body": tr("T_DISC_BUDGET_BODY"),
				"request": InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 0), "budget")})
	var known_country := _first_known_country()
	if known_country >= 0 and not _dismissed.has("known_country"):
		_cards.append({"key": "known_country", "title": tr("T_DISC_NEIGHBOR_TITLE"),
			"body": tr("T_DISC_NEIGHBOR_BODY"),
			"request": InfoRef.request(InfoRef.make(InfoRef.COUNTRY, known_country), "actions")})
	if Sim.world.has_method("dessein_info") and not _dismissed.has("dessein"):
		var d: Dictionary = Sim.world.dessein_info(_cid, 0)
		if bool(d.get("active", false)):
			_cards.append({"key": "dessein", "title": tr("T_DISC_DESSEIN_TITLE"),
				"body": tr("T_DISC_DESSEIN_BODY"), "dessein": true})
	var pages := maxi(1, int(ceil(float(_cards.size()) / float(PAGE_SIZE))))
	_page = clampi(_page, 0, pages - 1)
	queue_redraw()

func _draw() -> void:
	_hits.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	VKit.text(self, Vector2(20, 15), VKit.COL_PARCH, tr("T_DISC_TITLE"), VKit.FS_BIG)
	var close_r := Rect2(PW - 42.0, 12.0, 25.0, 25.0)
	VKit.fill(self, close_r, VKit.COL_PANEL2); VKit.box(self, close_r, VKit.COL_EDGE)
	VKit.text(self, Vector2(close_r.position.x + 7, close_r.position.y + 2), VKit.COL_PARCH, "×", VKit.FS)
	_hits.append({"rect": close_r, "act": "close"})
	VKit.text_wrapped(self, Vector2(20, 52), VKit.COL_DIM, tr("T_DISC_HELP"), PW - 40, 2, VKit.FS_SMALL)
	var y := 90.0
	if _cards.is_empty():
		VKit.text_wrapped(self, Vector2(20, y), VKit.COL_DIM, tr("T_DISC_NONE"), PW - 40, 3, VKit.FS)
		return
	var page_count := maxi(1, int(ceil(float(_cards.size()) / float(PAGE_SIZE))))
	var start := _page * PAGE_SIZE
	var finish := mini(start + PAGE_SIZE, _cards.size())
	for card_i in range(start, finish):
		var card: Dictionary = _cards[card_i]
		var card_r := Rect2(18, y, PW - 36, 86)
		VKit.fill(self, card_r, VKit.COL_PANEL2)
		VKit.box(self, card_r, VKit.COL_EDGE)
		VKit.text(self, Vector2(30, y + 8), VKit.COL_GOLD, String(card.get("title", "")), VKit.FS)
		VKit.text_wrapped(self, Vector2(30, y + 29), VKit.COL_PARCH,
			String(card.get("body", "")), PW - 190, 3, VKit.FS_SMALL)
		var open_r := Rect2(PW - 146, y + 26, 112, 27)
		VKit.fill(self, open_r, VKit.COL_GOLD); VKit.box(self, open_r, VKit.COL_EDGE)
		VKit.text(self, Vector2(open_r.position.x + 12, open_r.position.y + 4), VKit.COL_PARCH, tr("T_DISC_OPEN"), VKit.FS_SMALL)
		_hits.append({"rect": open_r, "act": "open", "card": card})
		var done_r := Rect2(PW - 146, y + 56, 112, 20)
		VKit.text(self, Vector2(done_r.position.x + 12, done_r.position.y + 1), VKit.COL_DIM, tr("T_DISC_DONE"), VKit.FS_SMALL)
		_hits.append({"rect": done_r, "act": "done", "key": String(card.get("key", ""))})
		y += 94.0
	if page_count > 1:
		var prev_r := Rect2(20, PH - 34, 100, 22)
		var next_r := Rect2(PW - 120, PH - 34, 100, 22)
		VKit.text(self, prev_r.position + Vector2(8, 1), VKit.COL_DIM, tr("T_DISC_PREVIOUS"), VKit.FS_SMALL)
		VKit.text(self, next_r.position + Vector2(8, 1), VKit.COL_GOLD, tr("T_DISC_NEXT"), VKit.FS_SMALL)
		VKit.text(self, Vector2(PW * 0.5 - 24, PH - 32), VKit.COL_DIM,
			tr("T_DISC_PAGE") % [_page + 1, page_count], VKit.FS_SMALL)
		_hits.append({"rect": prev_r, "act": "page", "delta": -1})
		_hits.append({"rect": next_r, "act": "page", "delta": 1})

func _gui_input(e: InputEvent) -> void:
	if not (e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT):
		return
	for h in _hits:
		if not (h["rect"] as Rect2).has_point(e.position):
			continue
		match String(h.get("act", "")):
			"close": close_panel()
			"done":
				_dismissed[String(h.get("key", ""))] = true
				refresh()
			"page":
				_page += int(h.get("delta", 0))
				refresh()
			"open":
				var card: Dictionary = h.get("card", {})
				if bool(card.get("dessein", false)):
					dessein_requested.emit()
				else:
					var req: Dictionary = card.get("request", {})
					if not req.is_empty(): navigate_requested.emit(req)
				close_panel()
		accept_event()
		return

func _first_owned_province() -> int:
	if not Sim.world.has_method("province_count") or not Sim.world.has_method("province_info"):
		return -1
	for pid in range(int(Sim.world.province_count())):
		var p: Dictionary = Sim.world.province_info(pid)
		if bool(p.get("valide", false)) and int(p.get("owner", -1)) == _cid:
			return pid
	return -1

func _first_known_country() -> int:
	if not Sim.world.has_method("country_count") or not Sim.world.has_method("country_info") \
		or not Sim.world.has_method("country_known"):
		return -1
	for cid in range(int(Sim.world.country_count())):
		if cid == _cid: continue
		var c: Dictionary = Sim.world.country_info(cid)
		if bool(c.get("valide", false)) and bool(Sim.world.country_known(cid)):
			return cid
	return -1
