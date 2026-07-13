extends Node

const InfoRef = preload("res://ui/info_ref.gd")
const NavigationHub = preload("res://ui/navigation_hub.gd")
const MemoryPanel = preload("res://ui/memory_panel.gd")

func _ready() -> void:
	call_deferred("_run")

func _run() -> void:
	await get_tree().process_frame
	_check(Sim.world != null, "monde de test absent")
	var hub := NavigationHub.new()
	add_child(hub)
	var panel := MemoryPanel.new()
	add_child(panel)
	panel.setup(hub)
	var me := int(Sim.world.player())
	var mine := InfoRef.request(InfoRef.make(InfoRef.COUNTRY, me))
	_check(hub.go(mine), "navigation pays refusée")
	_check(hub.toggle_pin(), "épingle refusée")
	panel.show()
	panel._refresh()
	await get_tree().process_frame
	_check(panel._pins.item_count == 1, "l'épingle n'apparaît pas dans le panneau")
	_check(panel._recent.item_count == 1, "le récent n'apparaît pas dans le panneau")
	_check(hub.add_compare(), "comparaison A refusée")
	var other := -1
	for cid in range(int(Sim.world.country_count())):
		if cid == me:
			continue
		var ci: Dictionary = Sim.world.country_info(cid)
		if bool(ci.get("valide", false)) and int(ci.get("regions", 0)) > 0:
			other = cid
			break
	_check(other >= 0, "second pays introuvable")
	_check(hub.add_compare(InfoRef.request(InfoRef.make(InfoRef.COUNTRY, other))), "comparaison B refusée")
	await get_tree().process_frame
	_check(hub.comparison_requests().size() == 2, "la paire comparée n'est pas conservée")
	_check(panel._compare_title.text.contains("↔"), "la comparaison live n'est pas rendue")
	_check(panel._compare_body.text.contains("Population"), "les indicateurs partagés sont absents")
	_check(hub.save_memory(99), "sidecar mémoire impossible à écrire")
	hub.clear_memory()
	_check(hub.load_memory(99), "sidecar mémoire impossible à relire")
	_check(hub.pinned_requests().size() == 1 and hub.comparison_requests().size() == 2,
		"épingles/comparaison ne survivent pas au cycle sauvegarde/chargement")
	DirAccess.remove_absolute(ProjectSettings.globalize_path("user://campaign_memory_99.cfg"))
	print("memory_panel_test: OK")
	panel.free()
	hub.free()
	get_tree().quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("memory_panel_test: " + message)
	get_tree().quit(1)
