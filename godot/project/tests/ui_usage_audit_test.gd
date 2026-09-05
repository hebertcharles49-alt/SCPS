extends Node

const InfoRef = preload("res://ui/info_ref.gd")
const NavigationHub = preload("res://ui/navigation_hub.gd")
const MemoryPanel = preload("res://ui/memory_panel.gd")
const SearchPalette = preload("res://ui/search_palette.gd")
const DesseinsPanel = preload("res://ui/desseins_panel.gd")
const DiscoveryPanel = preload("res://ui/discovery_panel.gd")
const OrderLogPanel = preload("res://ui/order_log_panel.gd")
const CountryActions = preload("res://ui/country_actions.gd")
const SidebarDrawer = preload("res://ui/sidebar_drawer.gd")
const DevPanel = preload("res://ui/devpanel.gd")
const ReligionPanel = preload("res://ui/religion_panel.gd")

var _failed := false
var _opened: Array = []
var _routed: Array = []

func _ready() -> void:
	call_deferred("_run")

func _run() -> void:
	await get_tree().process_frame
	# PARCOURS DE JEU : lecture de la façade Desseins, ouverture d'une cible,
	# puis tentative du verbe réel (aucune fixture de sauvegarde, aucun état moteur
	# forcé). Le test vérifie le chemin et le retour UI, pas une constante de layout.
	var old_game_on := Sim.game_on
	Sim.game_on = true
	var dess := DesseinsPanel.new()
	add_child(dess)
	dess.open()
	await get_tree().process_frame
	if not bool(dess._info.get("active", false)) and Sim.world != null:
		# La génération arme la branche au premier passage de missions_tick ; une
		# clôture de mois suffit pour exercer le contrat en conditions de partie.
		Sim.world.advance_days(30)
		dess.refresh()
	_check(dess.visible and dess._info.has("active"), "le panneau Desseins ne lit pas dessein_info")
	if bool(dess._info.get("active", false)):
		var dess_target := dess._target_request(String(dess._info.get("cible", "")))
		if String(dess._info.get("cible", "")) != "":
			_check(not dess_target.is_empty() and InfoRef.is_valid(dess_target.get("ref", {})),
				"la cible d'un Dessein ne mène à aucune référence valide")
		dess._seal(0)
		_check(dess._flash != "", "le panneau Desseins ne reflète pas le retour de seal_dessein")
	var orders := OrderLogPanel.new()
	add_child(orders)
	orders.open()
	await get_tree().process_frame
	if Sim.world != null and Sim.world.has_method("command_feedback"):
		_check(orders._entries.size() > 0,
			"le journal des ordres ne lit pas le résultat transient de la commande")
	for verb in range(60):
		_check(orders._verb_text(verb) != tr("T_ORDER_VERB_UNKNOWN"),
			"le journal laisse le verbe CMD_%d sans libellé" % verb)
	# PARCOURS ARMÉE : le clic sur un cran de levée enfile CMD_SET_LEVY, puis le
	# drain rend l'état observable. Ce n'est pas un test de constante de rendu.
	if Sim.world != null and Sim.world.has_method("country_army"):
		var drawer := SidebarDrawer.new()
		add_child(drawer)
		await get_tree().process_frame
		var before_army: Dictionary = Sim.world.country_army(int(Sim.world.player()))
		var before_levy := int(before_army.get("levy", 0))
		var requested_levy := (before_levy + 1) % 4
		drawer._set_levy(requested_levy)
		var queued := false
		for raw in Sim.world.command_feedback():
			var f: Dictionary = raw
			if int(f.get("verb", -1)) == 3 and int(f.get("status", 0)) == 0:
				queued = true
		_check(queued, "le clic de levée ne laisse pas l'ordre en attente")
		Sim.world.advance_days(1)
		var after_army: Dictionary = Sim.world.country_army(int(Sim.world.player()))
		_check(int(after_army.get("levy", -1)) == requested_levy,
			"le drain de levée ne modifie pas le cran demandé")
		drawer._set_levy(before_levy)
		Sim.world.advance_days(1)
		drawer.free()
	# PARCOURS F10 : une valeur non réelle est refusée avant tout appel moteur.
	var dev := DevPanel.new()
	add_child(dev)
	dev.show()
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().process_frame
	var dev_viewport := get_viewport().get_visible_rect()
	print("DevPanel geometry: viewport=",dev_viewport," panel=",dev._panel.get_global_rect())
	_check(dev_viewport.encloses(dev._panel.get_global_rect()),
		"DevPanel sort du viewport après son centrage dynamique")
	_check(absf(dev._panel.get_global_rect().get_center().x - dev_viewport.get_center().x) < 1.0 and
		absf(dev._panel.get_global_rect().get_center().y - dev_viewport.get_center().y) < 1.0,
		"DevPanel n'est pas centré sur le viewport")
	var tunables: Array = Sim.world.tunables() if Sim.world != null else []
	if not tunables.is_empty():
		var first_tunable: Dictionary = tunables[0]
		dev._apply("NaN", String(first_tunable.get("nom", "")))
		_check(String(dev._status.text).begins_with("Refusé"),
			"F10 accepte une valeur non finie")
		var inactive_found := false
		var active_found := false
		for raw in tunables:
			var tune: Dictionary = raw
			var tune_name := String(tune.get("nom", ""))
			var tune_phase := String(tune.get("phase", ""))
			_check(tune_phase in ["inactive", "diagnostic", "new_world", "rule_read", "next_action"],
				"F10 reçoit une phase moteur inconnue pour %s" % tune_name)
			if not bool(tune.get("active", true)) and not inactive_found:
				inactive_found = true
				dev._apply("1", tune_name)
				_check(String(dev._status.text).begins_with("Refusé"),
					"F10 modifie un tunable inactif")
			if bool(tune.get("active", true)) and not active_found:
				active_found = true
				var old_value := float(tune.get("value", 0.0))
				dev._apply(str(old_value + 0.125), tune_name)
				var changed := false
				for after_raw in Sim.world.tunables():
					var after: Dictionary = after_raw
					if String(after.get("nom", "")) == tune_name:
						changed = absf(float(after.get("value", old_value)) - old_value) > 0.0001
						break
				_check(changed, "F10 n'applique pas une valeur validée")
				dev._reset(tune_name)
				var restored := false
				for reset_raw in Sim.world.tunables():
					var reset_tune: Dictionary = reset_raw
					if String(reset_tune.get("nom", "")) == tune_name:
						restored = absf(float(reset_tune.get("value", old_value)) - old_value) < 0.0001
						break
				_check(restored, "F10 ne rétablit pas le défaut")
	# Le rachat est un lecteur/curseur économique réel, uniquement lorsque son
	# kill-switch moteur est actif; la valeur est restaurée dans la même fixture.
	if Sim.world != null and Sim.world.has_method("country_buy_rate") and Sim.world.has_method("player_set_buy_rate"):
		var buy_enabled := true
		if Sim.world.has_method("tunables"):
			for raw in Sim.world.tunables():
				var tune: Dictionary = raw
				if String(tune.get("nom", "")) == "BUY_RATE_ON":
					buy_enabled = float(tune.get("value", 1.0)) > 0.0
					break
		if buy_enabled:
			var buy_country := int(Sim.world.player())
			var buy_before := int(Sim.world.country_buy_rate(buy_country, 0))
			var buy_after := 0 if buy_before != 0 else 1
			Sim.world.player_set_buy_rate(0, buy_after)
			_check(int(Sim.world.country_buy_rate(buy_country, 0)) == buy_after,
				"le curseur de rachat ne modifie pas le readout économique")
			Sim.world.player_set_buy_rate(0, buy_before)
	var religion := ReligionPanel.new()
	add_child(religion)
	religion.open()
	await get_tree().process_frame
	if Sim.world != null and Sim.world.has_method("religion_founding_ready") \
		and Sim.world.has_method("religion_can_found") \
		and int(Sim.world.religion_of_country(int(Sim.world.player()))) < 0 \
		and int(Sim.world.religion_can_found()) == 1 \
		and int(Sim.world.religion_founding_ready(int(Sim.world.player()))) == 0:
		_check(religion._found_btn.disabled, "Fonder reste actif sans Temple ou Cathédrale")
		_check(String(religion._valid_lbl.text).contains("Temple"),
			"le panneau religion n'explique pas le verrou Temple")
	if Sim.world != null and Sim.world.has_method("religion_eligible") and int(Sim.world.religion_eligible(int(Sim.world.player()))) <= 0:
		religion._on_schism()
		_check(String(religion._action_lbl.text) != "", "un refus religion n'est pas visible dans le panneau")
	# PARCOURS COMMERCE : la fiche relation lit les compteurs du chantier de route
	# et ne promet pas encore de rendement avant l'ouverture. On crée explicitement
	# une route terrestre vers une région d'un pays connu, puis on laisse un jour
	# moteur drainer l'ordre et faire progresser le chantier.
	if Sim.world != null and Sim.world.has_method("country_relations") \
		and Sim.world.has_method("diplo_context") and Sim.world.has_method("player_route") \
		and Sim.world.has_method("region_count") and Sim.world.has_method("region_owner") \
		and Sim.world.has_method("country_capital_province") and Sim.world.has_method("province_region"):
		var route_probe_done := false
		var route_player := int(Sim.world.player())
		var route_cap_prov := int(Sim.world.country_capital_province(route_player))
		var route_cap := int(Sim.world.province_region(route_cap_prov))
		for raw_rel in Sim.world.country_relations(route_player):
			var rel_probe: Dictionary = raw_rel
			var route_target := int(rel_probe.get("country", -1))
			var route_ctx: Dictionary = Sim.world.diplo_context(route_target)
			if route_cap < 0:
				continue
			for candidate_region in range(Sim.world.region_count()):
				if int(Sim.world.region_owner(candidate_region)) != route_target:
					continue
				var before_shared := int(route_ctx.get("shared_routes", 0))
				if not Sim.world.player_route(route_cap, candidate_region, false):
					continue
				var before: Dictionary = Sim.world.diplo_context(route_target)
				before_shared = int(before.get("shared_routes", before_shared))
				Sim.world.advance_days(1)
				var after: Dictionary = Sim.world.diplo_context(route_target)
				var after_shared := int(after.get("shared_routes", 0))
				if after_shared <= before_shared:
					continue
				var route_total := int(after.get("route_days_total", 0))
				var route_done := int(after.get("route_days_done", 0))
				_check(after_shared > before_shared and route_total > 0,
					"la commande route ne crée pas le chantier relationnel attendu")
				_check(route_done >= 0 and route_done <= route_total,
					"la fiche relation expose une progression de route hors bornes")
				_check(not bool(after.get("route_open", false)) and route_done > 0,
					"la fiche relation ne montre pas la route en formation après le drain")
				var relation_panel := CountryActions.new()
				add_child(relation_panel)
				relation_panel.open_country(route_target)
				await get_tree().process_frame
				_check(String(relation_panel._engagement_lbl.text).contains("formation"),
					"la fiche relation n’affiche pas le chantier de route")
				relation_panel.queue_free()
				route_probe_done = true
				break
			if route_probe_done:
				break
		_check(route_probe_done,
			"le parcours route ne peut pas créer de liaison vers un pays connu")
	religion.queue_free()
	dev.queue_free()
	var discovery := DiscoveryPanel.new()
	add_child(discovery)
	_opened.clear()
	discovery.navigate_requested.connect(_capture_opened)
	discovery.open()
	await get_tree().process_frame
	_check(discovery.visible and discovery._cards.size() > 0,
		"l'aide Découvertes ne propose aucune piste depuis l'état courant")
	if not discovery._hits.is_empty():
		var open_hit: Dictionary = discovery._hits[1] if discovery._hits.size() > 1 else discovery._hits[0]
		if String(open_hit.get("act", "")) == "open":
			var click := InputEventMouseButton.new()
			click.button_index = MOUSE_BUTTON_LEFT
			click.pressed = true
			click.position = (open_hit["rect"] as Rect2).get_center()
			discovery._gui_input(click)
			_check(not discovery.visible, "le parcours Découvertes ne ferme pas la piste ouverte")
			if not bool((open_hit.get("card", {}) as Dictionary).get("dessein", false)):
				_check(_opened.size() == 1 and InfoRef.is_valid(_opened[0].get("ref", {})),
					"la piste Découvertes n'a pas émis la destination de son panneau")
	dess.queue_free()
	orders.queue_free()
	discovery.queue_free()
	Sim.game_on = old_game_on

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

	_routed.clear()
	hub.navigate_requested.connect(_capture_routed)
	var route := InfoRef.request(InfoRef.make(InfoRef.TECH, 0))
	_check(hub.go(route), "une destination de recherche doit être acceptée")
	_check(_routed.size() == 1 and InfoRef.request_key(_routed[0]) == InfoRef.request_key(route),
		"la navigation de recherche n'a pas émis la requête attendue")
	print("ui_usage_audit_test: parcours Desseins + Découvertes + navigation réelle")
	panel.queue_free()
	palette.queue_free()
	hub.queue_free()
	# Le banc ferme aussitôt après des clics : laisser le mixeur libérer ses
	# lectures, sans conserver des WAV en cours au moment de quitter Godot.
	for child in Sound.get_children():
		if child is AudioStreamPlayer:
			child.stop()
			child.stream = null
	await get_tree().create_timer(0.1).timeout
	await get_tree().process_frame
	await get_tree().process_frame
	get_tree().quit(1 if _failed else 0)

func _capture_opened(request: Dictionary) -> void:
	_opened.append(request)

func _capture_routed(request: Dictionary) -> void:
	_routed.append(request)

func _check(ok: bool, message: String) -> void:
	if ok:
		return
	push_error("ui_usage_audit_test: " + message)
	_failed = true
