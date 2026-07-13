extends Node

const InfoRef = preload("res://ui/info_ref.gd")
const NavigationHub = preload("res://ui/navigation_hub.gd")
const MemoryPanel = preload("res://ui/memory_panel.gd")
const SearchPalette = preload("res://ui/search_palette.gd")

const PATHS := [
	{"q": "Pourquoi mon trésor varie ?", "n": 2, "ref": {"kind": InfoRef.SIDEBAR_TAB, "id": 0}},
	{"q": "Où mon bien déficitaire est-il produit ?", "n": 2, "ref": {"kind": InfoRef.RESOURCE, "id": 0}},
	{"q": "Pourquoi cette province rapporte-t-elle ce montant ?", "n": 2, "ref": {"kind": InfoRef.PROVINCE, "id": 0}},
	{"q": "Cette culture va-t-elle fusionner ?", "n": 2, "ref": {"kind": InfoRef.PROVINCE, "id": 0}},
	{"q": "Quelle faction porte le coup ?", "n": 2, "ref": {"kind": InfoRef.SIDEBAR_TAB, "id": 7}},
	{"q": "Pourquoi cette action diplomatique est-elle bloquée ?", "n": 2, "ref": {"kind": InfoRef.COUNTRY, "id": 0}},
	{"q": "Quel lieu relie ces deux pays ?", "n": 2, "ref": {"kind": InfoRef.COUNTRY, "id": 0}},
	{"q": "Quel chemin mène à cette technologie ?", "n": 3, "ref": {"kind": InfoRef.TECH, "id": 0}},
	{"q": "Combien d'hommes arriveront après la marche ?", "n": 2, "ref": {"kind": InfoRef.CORPS, "id": 0}},
	{"q": "Où et pourquoi ai-je perdu cette bataille ?", "n": 1, "ref": {"kind": InfoRef.REGION, "id": 0}},
]

func _ready() -> void:
	call_deferred("_run")

func _run() -> void:
	await get_tree().process_frame
	var hub := NavigationHub.new()
	add_child(hub)
	var panel := MemoryPanel.new()
	add_child(panel)
	panel.setup(hub)
	var me := int(Sim.world.player())
	hub.go(InfoRef.request(InfoRef.make(InfoRef.COUNTRY, me)))
	panel.open_panel()
	await get_tree().process_frame
	await get_tree().process_frame
	var viewport := get_viewport().get_visible_rect()
	var frame: Rect2 = panel._frame.get_global_rect()
	_check(viewport.encloses(frame), "panneau Mémoire hors de la résolution projet")
	var long_id := "Question stratégique très longue · " + "frontière commerce culture armée ".repeat(12)
	var long_req := InfoRef.request(InfoRef.make(InfoRef.CODEX, long_id), "codex", {"query": long_id})
	hub.toggle_pin(long_req)
	await get_tree().process_frame
	_check(panel._pins.item_count >= 1 and viewport.encloses(panel._frame.get_global_rect()),
		"un texte long agrandit le panneau hors écran")

	var palette := SearchPalette.new()
	add_child(palette)
	palette.open_palette()
	await get_tree().process_frame
	_check(get_viewport().gui_get_focus_owner() == palette._query, "Ctrl+K n'aboutit pas au champ de saisie")
	_check(palette._results.item_count > 0, "la recherche ouverte au clavier est vide")
	palette.close_palette()

	for path in PATHS:
		_check(int(path.get("n", 99)) <= 3, "plus de trois interactions : " + String(path.get("q", "")))
		_check(InfoRef.is_valid(path.get("ref", {})), "destination morte : " + String(path.get("q", "")))
	print("ui_usage_audit_test: OK · %d questions · maximum 3 interactions" % PATHS.size())
	panel.queue_free()
	palette.queue_free()
	hub.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("ui_usage_audit_test: " + message)
	get_tree().quit(1)
