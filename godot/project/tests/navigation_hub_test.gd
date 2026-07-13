extends SceneTree

const InfoRef = preload("res://ui/info_ref.gd")
const NavigationHub = preload("res://ui/navigation_hub.gd")

var _seen: Array[Dictionary] = []

func _initialize() -> void:
	var hub = NavigationHub.new()
	root.add_child(hub)
	hub.navigate_requested.connect(func(req): _seen.append(req))

	_check(not hub.go({"ref": {"kind": "unknown", "id": 1}}), "une référence inconnue doit être rejetée")
	var country := InfoRef.request(InfoRef.make(InfoRef.COUNTRY, 4))
	var stocks := InfoRef.request(InfoRef.make(InfoRef.SIDEBAR_TAB, 2), "sidebar", {"resource_id": 9})
	_check(hub.go(country), "la première route doit être acceptée")
	_check(not hub.can_back() and not hub.can_forward(), "la première route ne crée pas d'antécédent")
	_check(hub.go(stocks), "la seconde route doit être acceptée")
	_check(hub.can_back() and not hub.can_forward(), "la seconde route doit ouvrir le retour")
	var before_duplicate := _seen.size()
	_check(hub.go(stocks), "une route identique doit pouvoir restaurer une vue fermée")
	_check(_seen.size() == before_duplicate + 1, "une route identique doit être réémise")
	_check(hub.back(), "retour attendu")
	_check(InfoRef.request_key(_seen.back()) == InfoRef.request_key(country), "retour vers le pays")
	_check(hub.can_forward(), "le retour doit ouvrir l'avance")
	_check(hub.forward(), "avance attendue")
	_check(InfoRef.request_key(_seen.back()) == InfoRef.request_key(stocks), "avance vers les stocks")
	_check(hub.recent_requests().size() == 2, "les récents sont dédupliqués malgré retour/avance")
	_check(hub.toggle_pin(), "la vue courante doit pouvoir être épinglée")
	_check(hub.is_pinned(stocks) and hub.pinned_requests().size() == 1, "l'épingle conserve la requête complète")
	_check(not hub.toggle_pin(stocks) and hub.pinned_requests().is_empty(), "un second geste retire l'épingle")
	_check(hub.add_compare(country), "un pays peut entrer en comparaison")
	var country_b := InfoRef.request(InfoRef.make(InfoRef.COUNTRY, 7))
	_check(hub.add_compare(country_b) and hub.comparison_requests().size() == 2,
		"deux objets du même type restent côte à côte")
	var province := InfoRef.request(InfoRef.make(InfoRef.PROVINCE, 3), "map")
	_check(hub.add_compare(province) and hub.comparison_requests().size() == 1,
		"changer de type recommence une comparaison homogène")
	var current_before := InfoRef.request_key(hub.current_request())
	var memory := InfoRef.request(InfoRef.make(InfoRef.MEMORY, 0), "memory")
	_check(hub.go(memory) and InfoRef.request_key(hub.current_request()) == current_before,
		"ouvrir la mémoire ne remplace pas la vue courante")

	print("navigation_hub_test: OK (%d routes observées)" % _seen.size())
	quit(0)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("navigation_hub_test: " + message)
	quit(1)
