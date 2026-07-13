extends Control
## Mémoire de campagne — récents, épingles durables et comparaison live.
## Aucun instantané métier n'est conservé : chaque ligne relit le moteur à l'ouverture.

const InfoRef = preload("res://ui/info_ref.gd")

signal navigate_requested(request: Dictionary)

var _hub: Node
var _frame: PanelContainer
var _current: Label
var _pin_current: Button
var _compare_current: Button
var _tabs: TabContainer
var _pins: ItemList
var _recent: ItemList
var _compare_title: Label
var _compare_body: RichTextLabel
var _open_a: Button
var _open_b: Button

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var veil := ColorRect.new()
	veil.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	veil.color = Color(0.01, 0.015, 0.018, 0.72)
	veil.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(veil)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	_frame = PanelContainer.new()
	_frame.custom_minimum_size = Vector2(860, 620)
	var box := StyleBoxFlat.new()
	box.bg_color = Color(0.055, 0.065, 0.068)
	box.border_color = Color(0.55, 0.43, 0.22)
	box.set_border_width_all(1)
	box.set_border_width(SIDE_TOP, 3)
	box.set_content_margin_all(16)
	_frame.add_theme_stylebox_override("panel", box)
	center.add_child(_frame)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 9)
	_frame.add_child(col)
	var head := HBoxContainer.new()
	col.add_child(head)
	var title := Label.new()
	title.text = "Mémoire de campagne"
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_font_size_override("font_size", 21)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	head.add_child(title)
	var close := Button.new()
	close.text = "Fermer"
	close.pressed.connect(close_panel)
	head.add_child(close)
	var intro := Label.new()
	intro.text = "Gardez une question stratégique sous la main. Les chiffres comparés restent live."
	intro.add_theme_color_override("font_color", Color(0.68, 0.66, 0.60))
	col.add_child(intro)
	var tools := HBoxContainer.new()
	tools.add_theme_constant_override("separation", 8)
	col.add_child(tools)
	_current = Label.new()
	_current.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_current.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	tools.add_child(_current)
	_pin_current = Button.new()
	_pin_current.pressed.connect(_toggle_current_pin)
	tools.add_child(_pin_current)
	_compare_current = Button.new()
	_compare_current.text = "Ajouter à la comparaison"
	_compare_current.pressed.connect(_add_current_compare)
	tools.add_child(_compare_current)
	_tabs = TabContainer.new()
	_tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(_tabs)
	_build_pins_tab()
	_build_recent_tab()
	_build_compare_tab()
	Sim.month_ticked.connect(func(_year): if visible: _refresh())
	hide()

func setup(hub: Node) -> void:
	_hub = hub
	if _hub != null and _hub.has_signal("memory_changed"):
		_hub.memory_changed.connect(_refresh)
	_refresh()

func open_panel(tab: int = -1) -> void:
	_refresh()
	if tab >= 0:
		_tabs.current_tab = clampi(tab, 0, 2)
	show()
	Sound.play("ui_parchment_open")

func close_panel() -> void:
	if not visible:
		return
	hide()
	Sound.play("ui_parchment_close")

func _build_pins_tab() -> void:
	var root := VBoxContainer.new()
	root.name = "Épingles"
	_tabs.add_child(root)
	var hint := Label.new()
	hint.text = "Clic gauche : ouvrir · clic droit : retirer l'épingle"
	hint.add_theme_color_override("font_color", Color(0.58, 0.56, 0.52))
	root.add_child(hint)
	_pins = ItemList.new()
	_pins.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_pins.item_clicked.connect(_on_pin_clicked)
	root.add_child(_pins)
	var remove := Button.new()
	remove.text = "Retirer la sélection"
	remove.pressed.connect(func():
		var sel := _pins.get_selected_items()
		if _hub != null and not sel.is_empty():
			_hub.remove_pin(int(sel[0])))
	root.add_child(remove)

func _build_recent_tab() -> void:
	var root := VBoxContainer.new()
	root.name = "Récents"
	_tabs.add_child(root)
	var hint := Label.new()
	hint.text = "Les 24 dernières vues distinctes, les plus récentes d'abord."
	hint.add_theme_color_override("font_color", Color(0.58, 0.56, 0.52))
	root.add_child(hint)
	_recent = ItemList.new()
	_recent.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_recent.item_clicked.connect(func(index, _at, button):
		if button == MOUSE_BUTTON_LEFT:
			_open_list_item(_recent, int(index)))
	root.add_child(_recent)

func _build_compare_tab() -> void:
	var root := VBoxContainer.new()
	root.name = "Comparer"
	_tabs.add_child(root)
	_compare_title = Label.new()
	_compare_title.add_theme_font_size_override("font_size", 17)
	_compare_title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	root.add_child(_compare_title)
	_compare_body = RichTextLabel.new()
	_compare_body.bbcode_enabled = true
	_compare_body.fit_content = false
	_compare_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_compare_body.add_theme_font_size_override("normal_font_size", 14)
	root.add_child(_compare_body)
	var actions := HBoxContainer.new()
	root.add_child(actions)
	_open_a = Button.new()
	_open_a.text = "Ouvrir A"
	_open_a.pressed.connect(func(): _open_compare(0))
	actions.add_child(_open_a)
	_open_b = Button.new()
	_open_b.text = "Ouvrir B"
	_open_b.pressed.connect(func(): _open_compare(1))
	actions.add_child(_open_b)
	var remove_a := Button.new()
	remove_a.text = "Retirer A"
	remove_a.pressed.connect(func(): if _hub != null: _hub.remove_compare(0))
	actions.add_child(remove_a)
	var remove_b := Button.new()
	remove_b.text = "Retirer B"
	remove_b.pressed.connect(func(): if _hub != null: _hub.remove_compare(1))
	actions.add_child(remove_b)
	var clear := Button.new()
	clear.text = "Effacer"
	clear.pressed.connect(func(): if _hub != null: _hub.clear_compare())
	actions.add_child(clear)

func _refresh() -> void:
	if not is_node_ready() or _hub == null:
		return
	var current: Dictionary = _hub.current_request()
	var current_desc := _describe(current)
	_current.text = "Vue courante : %s" % String(current_desc.get("title", "aucune"))
	_pin_current.disabled = current.is_empty()
	_pin_current.text = "Désépingler" if not current.is_empty() and _hub.is_pinned(current) else "Épingler la vue"
	var ref: Dictionary = current.get("ref", {})
	_compare_current.disabled = current.is_empty() or not _hub.COMPARE_KINDS.has(String(ref.get("kind", "")))
	_fill_list(_pins, _hub.pinned_requests())
	_fill_list(_recent, _hub.recent_requests())
	_refresh_compare()

func _fill_list(list: ItemList, requests: Array) -> void:
	list.clear()
	for raw in requests:
		if not (raw is Dictionary):
			continue
		var req: Dictionary = raw
		var desc := _describe(req)
		var title := String(desc.get("title", "Référence indisponible"))
		var subtitle := String(desc.get("subtitle", ""))
		list.add_item("%s%s" % [title, " — " + subtitle if subtitle != "" else ""])
		list.set_item_metadata(list.item_count - 1, req.duplicate(true))
		if not bool(desc.get("valid", false)):
			list.set_item_custom_fg_color(list.item_count - 1, Color(0.72, 0.38, 0.35))

func _on_pin_clicked(index: int, _at: Vector2, button: int) -> void:
	if _hub == null:
		return
	if button == MOUSE_BUTTON_RIGHT:
		_hub.remove_pin(index)
	elif button == MOUSE_BUTTON_LEFT:
		_open_list_item(_pins, index)

func _open_list_item(list: ItemList, index: int) -> void:
	if index < 0 or index >= list.item_count:
		return
	var req = list.get_item_metadata(index)
	if req is Dictionary:
		close_panel()
		navigate_requested.emit((req as Dictionary).duplicate(true))

func _toggle_current_pin() -> void:
	if _hub != null:
		_hub.toggle_pin()

func _add_current_compare() -> void:
	if _hub != null and _hub.add_compare():
		_tabs.current_tab = 2

func _open_compare(index: int) -> void:
	if _hub == null:
		return
	var items: Array = _hub.comparison_requests()
	if index >= 0 and index < items.size():
		close_panel()
		navigate_requested.emit((items[index] as Dictionary).duplicate(true))

func _refresh_compare() -> void:
	var items: Array = _hub.comparison_requests()
	_open_a.disabled = items.size() < 1
	_open_b.disabled = items.size() < 2
	if items.is_empty():
		_compare_title.text = "Aucun objet à comparer"
		_compare_body.text = "Ouvrez un pays, une province, un corps ou une ressource, puis utilisez « Ajouter à la comparaison »."
		return
	var a := _snapshot(items[0])
	if items.size() < 2:
		_compare_title.text = "%s · en attente de B" % String(a.get("title", "A"))
		_compare_body.text = "Ajoutez un second objet du même type. Un autre type recommencera la comparaison."
		return
	var b := _snapshot(items[1])
	_compare_title.text = "%s  ↔  %s" % [String(a.get("title", "A")), String(b.get("title", "B"))]
	var rows_a: Array = a.get("rows", [])
	var rows_b: Array = b.get("rows", [])
	var bb := "[table=3][cell][b]Indicateur[/b][/cell][cell][b]A[/b][/cell][cell][b]B[/b][/cell]"
	for i in range(mini(rows_a.size(), rows_b.size())):
		var ra: Dictionary = rows_a[i]
		var rb: Dictionary = rows_b[i]
		bb += "[cell]%s[/cell][cell]%s[/cell][cell]%s[/cell]" % [
			String(ra.get("label", "")), String(ra.get("value", "—")), String(rb.get("value", "—"))]
	bb += "[/table]"
	_compare_body.text = bb

func _describe(request: Dictionary) -> Dictionary:
	if request.is_empty() or Sim.world == null:
		return {"valid": false, "title": "aucune", "subtitle": ""}
	var snap := _snapshot(request)
	return {"valid": bool(snap.get("valid", false)), "title": String(snap.get("title", "Référence indisponible")),
		"subtitle": String(snap.get("subtitle", ""))}

func _snapshot(request: Dictionary) -> Dictionary:
	var ref: Dictionary = request.get("ref", {})
	var kind := String(ref.get("kind", ""))
	var id := int(ref.get("id", -1)) if typeof(ref.get("id", -1)) == TYPE_INT else -1
	var w = Sim.world
	if w == null:
		return {"valid": false, "title": "Référence indisponible", "rows": []}
	match kind:
		InfoRef.COUNTRY:
			var d: Dictionary = w.country_info(id)
			if not bool(d.get("valide", false)): return _invalid(kind, id)
			return {"valid": true, "title": String(d.get("nom", "Pays")), "subtitle": String(d.get("ethos", "")), "rows": [
				_row("Population", _num(int(d.get("pop", 0)))), _row("Trésor", "%.0f" % float(d.get("or", 0.0))),
				_row("Régions", str(int(d.get("regions", 0)))), _row("Stabilité", _pct(d.get("stabilite", 0))),
				_row("Prospérité", _pct(d.get("prosperite", 0))), _row("Légitimité", _pct(d.get("legitimite", 0))),
				_row("Cohésion", _pct(d.get("cohesion", 0))), _row("Savoir", _pct(d.get("savoir", 0))),
				_row("Influence", _pct(d.get("influence", 0))), _row("Corruption", _pct(d.get("corruption", 0)))]}
		InfoRef.PROVINCE:
			var d: Dictionary = w.province_info(id)
			if not bool(d.get("valide", false)): return _invalid(kind, id)
			return {"valid": true, "title": String(d.get("nom", "Province")),
				"subtitle": "%s · %s" % [String(d.get("terrain", "")), String(d.get("vocation", ""))], "rows": [
				_row("Population", _num(int(d.get("ames", 0)))), _row("Agitation", _pct(d.get("agitation", 0))),
				_row("Loyauté locale", _pct(d.get("humeur_val", 0))), _row("Aisance", _pct(d.get("aisance_val", 0))),
				_row("Logements libres", "%s / %s" % [_num(int(d.get("logements_libres", 0))), _num(int(d.get("logements_cap", 0)))]),
				_row("Services libres", "%s / %s" % [_num(int(d.get("services_libres", 0))), _num(int(d.get("services_cap", 0)))]),
				_row("Défense", String(d.get("defense", "—"))), _row("Ressource", String(d.get("ressource", "—")))]}
		InfoRef.CORPS:
			var d: Dictionary = w.corps_info(id)
			if not bool(d.get("active", false)): return _invalid(kind, id)
			return {"valid": true, "title": "Corps %d" % id, "subtitle": String(d.get("location", "")), "rows": [
				_row("Effectif", _num(int(d.get("units", 0)))), _row("Infanterie", _num(int(d.get("inf", 0)))),
				_row("Archers", _num(int(d.get("arch", 0)))), _row("Cavalerie", _num(int(d.get("cav", 0)))),
				_row("Mages", _num(int(d.get("mages", 0)))), _row("Phase", String(d.get("phase", "—"))),
				_row("Destination", String(d.get("destination", "—"))), _row("Progression", _pct(d.get("progress_pct", 0))),
				_row("Batailles", str(int(d.get("battles", 0))))]}
		InfoRef.RESOURCE:
			for raw in w.country_stocks(w.player()):
				var d: Dictionary = raw
				if int(d.get("res_id", -1)) != id: continue
				return {"valid": true, "title": String(d.get("name", "Ressource")), "subtitle": String(d.get("marche", "")), "rows": [
					_row("Stock", _num(int(d.get("stock", 0)))), _row("Flux / jour", "%+.2f" % float(d.get("net_day", 0.0))),
					_row("Offre / mois", "%.1f" % float(d.get("supply_month", 0.0))), _row("Demande / mois", "%.1f" % float(d.get("demand_month", 0.0))),
					_row("Couverture", "%d j" % int(d.get("coverage_days", 0))), _row("Prix", "%.2f" % float(d.get("price", 0.0)))]}
			return _invalid(kind, id)
		InfoRef.TECH:
			var techs: Array = w.tech_nodes()
			if id < 0: return {"valid": true, "title": "Arbre de technologie", "subtitle": "Savoir", "rows": []}
			if id >= techs.size(): return _invalid(kind, id)
			var d: Dictionary = techs[id]
			return {"valid": true, "title": String(d.get("name", "Technologie")), "subtitle": String(d.get("effet", "")), "rows": []}
		InfoRef.REGION:
			var c: Vector2 = w.region_centroid(id)
			return {"valid": c.x >= 0.0, "title": "Région %d" % id, "subtitle": "Carte", "rows": []}
		InfoRef.SIDEBAR_TAB:
			var names := ["Économie", "Démographie", "Stocks", "Marché", "Armée", "Filtres", "Diplomatie", "Conseil"]
			return {"valid": id >= 0 and id < names.size(), "title": names[id] if id >= 0 and id < names.size() else "Panneau", "subtitle": "Tiroir", "rows": []}
		InfoRef.MAP_MODE:
			return {"valid": id >= 0, "title": "Mode de carte %d" % id, "subtitle": "Carte", "rows": []}
		InfoRef.CODEX:
			return {"valid": true, "title": String(ref.get("id", "Codex")), "subtitle": "Codex", "rows": []}
	return _invalid(kind, id)

func _invalid(kind: String, id: int) -> Dictionary:
	return {"valid": false, "title": "%s #%d indisponible" % [kind, id], "subtitle": "", "rows": []}

func _row(label: String, value: String) -> Dictionary:
	return {"label": label, "value": value}

func _pct(value) -> String:
	return "%d %%" % int(round(float(value)))

func _num(value: int) -> String:
	var s := str(absi(value))
	var out := ""
	while s.length() > 3:
		out = " " + s.substr(s.length() - 3, 3) + out
		s = s.substr(0, s.length() - 3)
	return ("-" if value < 0 else "") + s + out
