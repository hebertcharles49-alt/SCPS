extends SceneTree

const InfoRef = preload("res://ui/info_ref.gd")
const TooltipServer = preload("res://ui/tooltip_server.gd")

func _initialize() -> void:
	var server = TooltipServer.new()
	var request := InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 0), "sidebar")
	var bb: String = server._card_bb({
		"title": "Trésor",
		"state": "1 250 or disponibles",
		"trend": "+42 / mois",
		"lines": [{"label": "Impôts", "value": "+60 / mois", "tone": "positive"}],
		"actions": [{"label": "Ouvrir le budget", "request": request}],
	})
	_check(bb.contains("Trésor"), "titre absent")
	_check(bb.contains("Impôts"), "décomposition absente")
	_check(bb.contains("7fd18a"), "couleur sémantique positive absente")
	_check(bb.contains("url=nav:0"), "deep-link absent")
	_check(bb.contains("url=pin") and bb.contains("url=close"), "commandes d'épinglage absentes")
	server.free()
	print("info_card_test: OK")
	quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("info_card_test: " + message)
	quit(1)
