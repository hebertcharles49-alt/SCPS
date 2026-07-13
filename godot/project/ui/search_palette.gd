extends Control
## Palette universelle Ctrl+K : index de navigation uniquement. Une action de jeu
## n'est jamais exécutée depuis la recherche ; les verbes ouvrent leur explication.

const InfoRef = preload("res://ui/info_ref.gd")
const SearchRank = preload("res://ui/search_rank.gd")
const Concepts = preload("res://ui/concepts.gd")
const Codex = preload("res://ui/codex.gd")
const Drawer = preload("res://ui/sidebar_drawer.gd")

signal navigate_requested(request: Dictionary)

const MAX_RESULTS := 18
var _query: LineEdit
var _results: ItemList
var _status: Label
var _entries: Array = []
var _visible_entries: Array = []

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
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(720, 520)
	var box := StyleBoxFlat.new()
	box.bg_color = Color(0.055, 0.065, 0.068)
	box.border_color = Color(0.55, 0.43, 0.22)
	box.set_border_width_all(1); box.set_border_width(SIDE_TOP, 3)
	box.set_content_margin_all(16)
	panel.add_theme_stylebox_override("panel", box)
	center.add_child(panel)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	panel.add_child(col)
	var title := Label.new()
	title.text = "Recherche universelle"
	title.add_theme_font_size_override("font_size", 21)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	col.add_child(title)
	_query = LineEdit.new()
	_query.placeholder_text = "Action, pays, province, corps, ressource, technologie…"
	_query.text_changed.connect(func(_text): _refresh_results())
	_query.gui_input.connect(_on_query_input)
	col.add_child(_query)
	_results = ItemList.new()
	_results.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_results.allow_reselect = true
	_results.item_activated.connect(func(index): _activate(int(index)))
	_results.item_clicked.connect(func(index, _at, button): if button == MOUSE_BUTTON_LEFT: _activate(int(index)))
	col.add_child(_results)
	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(0.58, 0.56, 0.52))
	col.add_child(_status)
	visibility_changed.connect(func():
		if visible:
			_query.grab_focus())
	hide()

func open_palette() -> void:
	_entries = build_entries(Sim.world)
	_query.text = ""
	visible = true
	_refresh_results()
	_query.grab_focus()
	Sound.play("ui_parchment_open")

func close_palette() -> void:
	if not visible:
		return
	hide()
	Sound.play("ui_parchment_close")

func _entry(title: String, kind: String, subtitle: String, request: Dictionary, extra: String = "") -> Dictionary:
	return {"title": title, "kind": kind, "subtitle": subtitle, "request": request,
		"search": title + " " + subtitle + " " + extra}

## Public pour le banc headless P5. L'index respecte le brouillard : pays inconnus et
## territoires de propriétaires inconnus ne traversent jamais la palette.
func build_entries(w) -> Array:
	var out: Array = []
	# Portes sûres vers les surfaces existantes.
	for p in [["Économie", 0], ["Démographie", 1], ["Stocks", 2], ["Marché", 3],
		["Armée", 4], ["Filtres de carte", 5], ["Diplomatie", 6], ["Conseil", 7]]:
		out.append(_entry(String(p[0]), "Panneau", "Ouvrir le tiroir", InfoRef.request(
			InfoRef.make(InfoRef.SIDEBAR_TAB, int(p[1])), "sidebar")))
	out.append(_entry("Arbre de technologie", "Panneau", "Ouvrir le savoir",
		InfoRef.request(InfoRef.make(InfoRef.TECH, -1))))
	out.append(_entry("Mémoire de campagne", "Panneau", "Récents, épingles et comparaison · Ctrl+M",
		InfoRef.request(InfoRef.make(InfoRef.MEMORY, 0), "memory"), "favoris suivi comparer historique"))
	# Tous les verbes et concepts du Codex : ouvrir l'explication, jamais exécuter.
	for domain in Codex.DOMAINS:
		for raw in domain[1]:
			var action: Dictionary = raw
			if bool(action.get("bientot", false)):
				continue
			var name := String(action.get("nom", "Action"))
			out.append(_entry(name, "Action", String(action.get("ou", "")), InfoRef.request(
				InfoRef.make(InfoRef.CODEX, name), "codex", {"query": name}), String(action.get("regle", ""))))
	for concept in Concepts.DEFS:
		var cname := String(concept)
		out.append(_entry(cname, "Concept", Concepts.def_of(cname), InfoRef.request(
			InfoRef.make(InfoRef.CODEX, cname), "codex", {"query": cname})))
	# Modes de carte : le catalogue reste celui du tiroir Filtres.
	for group in Drawer.FILT_GROUPS:
		for mode in group[1]:
			out.append(_entry(String(mode[0]), "Carte", "Mode %s" % String(group[0]),
				InfoRef.request(InfoRef.make(InfoRef.MAP_MODE, int(mode[1])), "map")))
	if w == null:
		return out
	var me := int(w.player())
	for cid in range(int(w.country_count())):
		if cid != me and w.has_method("country_known") and int(w.country_known(cid)) == 0:
			continue
		var ci: Dictionary = w.country_info(cid)
		if not bool(ci.get("valide", false)) or int(ci.get("regions", 0)) <= 0:
			continue
		out.append(_entry(String(ci.get("nom", "Pays")), "Pays",
			"%s · %d régions" % [String(ci.get("ethos", "")), int(ci.get("regions", 0))],
			InfoRef.request(InfoRef.make(InfoRef.COUNTRY, cid))))
	var seen_regions := {}
	for pid in range(int(w.province_count())):
		var pi: Dictionary = w.province_info(pid)
		if not bool(pi.get("valide", false)):
			continue
		var owner := int(pi.get("owner", -1))
		if owner < 0 or (owner != me and w.has_method("country_known") and int(w.country_known(owner)) == 0):
			continue
		var pname := String(pi.get("nom", "Province"))
		out.append(_entry(pname, "Province", "%s · %s" % [String(pi.get("terrain", "")), String(pi.get("vocation", ""))],
			InfoRef.request(InfoRef.make(InfoRef.PROVINCE, pid), "map", {"focus_map": true})))
		var rid := int(w.province_region(pid))
		if rid >= 0 and not seen_regions.has(rid):
			seen_regions[rid] = true
			out.append(_entry("Région de %s" % pname, "Région", "Centrer la carte",
				InfoRef.request(InfoRef.make(InfoRef.REGION, rid), "map")))
	if w.has_method("corps_ids"):
		for raw_id in w.corps_ids(me):
			var corps_id := int(raw_id)
			var army: Dictionary = w.corps_info(corps_id)
			if not bool(army.get("active", false)):
				continue
			out.append(_entry("Corps %d" % corps_id, "Corps", "%s · %s hommes" % [
				String(army.get("location", "?")), str(int(army.get("units", 0)))],
				InfoRef.request(InfoRef.make(InfoRef.CORPS, corps_id), "map")))
	if w.has_method("country_stocks"):
		for stock in w.country_stocks(me):
			out.append(_entry(String(stock.get("name", "Ressource")), "Ressource",
				"stock %s · %s" % [str(int(stock.get("stock", 0))), String(stock.get("marche", ""))],
				InfoRef.request(InfoRef.make(InfoRef.RESOURCE, int(stock.get("res_id", -1))), "sidebar", {"tab": 2})))
	if w.has_method("tech_nodes"):
		var techs: Array = w.tech_nodes()
		for tid in range(techs.size()):
			var tech: Dictionary = techs[tid]
			out.append(_entry(String(tech.get("name", "Technologie")), "Technologie",
				String(tech.get("effet", "")), InfoRef.request(InfoRef.make(InfoRef.TECH, tid)),
				String(tech.get("unlocks", "")) + " " + String(tech.get("reason_label", ""))))
	return out

func rank_entries(query: String, entries: Array = _entries) -> Array:
	var ranked: Array = []
	for entry in entries:
		var score := SearchRank.score(query, String(entry.get("title", "")), String(entry.get("search", "")))
		if score >= 0:
			var row: Dictionary = entry.duplicate(true)
			row["score"] = score
			ranked.append(row)
	ranked.sort_custom(func(a, b):
		if int(a["score"]) != int(b["score"]): return int(a["score"]) > int(b["score"])
		if String(a["kind"]) != String(b["kind"]): return String(a["kind"]) < String(b["kind"])
		return String(a["title"]) < String(b["title"]))
	return ranked

func _refresh_results() -> void:
	if _results == null:
		return
	_results.clear()
	_visible_entries = rank_entries(_query.text)
	var shown := mini(MAX_RESULTS, _visible_entries.size())
	for i in range(shown):
		var e: Dictionary = _visible_entries[i]
		_results.add_item("%-12s · %s — %s" % [String(e["kind"]), String(e["title"]), String(e["subtitle"])])
		_results.set_item_tooltip(i, String(e["search"]))
	if shown > 0:
		_results.select(0)
	_status.text = "%d résultat(s)%s · ↑↓ choisir · Entrée ouvrir · Échap fermer" % [
		_visible_entries.size(), " · %d affichés" % shown if _visible_entries.size() > shown else ""]

func _move_selection(delta: int) -> void:
	if _results.item_count <= 0:
		return
	var selected := _results.get_selected_items()
	var current := int(selected[0]) if not selected.is_empty() else 0
	current = clampi(current + delta, 0, _results.item_count - 1)
	_results.select(current)
	_results.ensure_current_is_visible()

func _activate(index: int) -> void:
	if index < 0 or index >= mini(MAX_RESULTS, _visible_entries.size()):
		return
	var request: Dictionary = _visible_entries[index].get("request", {})
	if not request.is_empty():
		hide()
		navigate_requested.emit(request)
		Sound.play("ui_click")

func _on_query_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and not event.echo):
		return
	match event.keycode:
		KEY_DOWN:
			_move_selection(1); _query.accept_event()
		KEY_UP:
			_move_selection(-1); _query.accept_event()
		KEY_ENTER, KEY_KP_ENTER:
			var selected := _results.get_selected_items()
			_activate(int(selected[0]) if not selected.is_empty() else 0)
			_query.accept_event()
		KEY_ESCAPE:
			close_palette(); _query.accept_event()
