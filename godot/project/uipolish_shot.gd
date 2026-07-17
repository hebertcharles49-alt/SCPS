extends Node
## uipolish_shot — preuve visuelle AVANT/APRÈS pour la mission UI-POLISH (13 défauts).
## Boote le VRAI shell (Main.tscn, comme shot_ui.gd) — piège connu : une probe isolée
## MENT par contexte (thème de fenêtre, sidebar, empire_sidebar…). FENÊTRÉE seulement
## (--headless = hang connu, cf. TROUVAILLES).
##   Godot --path godot/project res://uipolish_shot.tscn -- seed=9 years=25
var _main: Node = null
var _map: Node2D = null
var _dir := "res://shots_uipolish/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	if FileAccess.file_exists("user://session_running.flag"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path("user://session_running.flag"))
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(_dir + nom + ".png")
	print("SHOT ", nom)

func _reset() -> void:
	while _main._close_topmost():
		pass
	var sb: Control = _main._sidebar
	if sb != null and sb.has_method("close"):
		sb.close()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return

	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	var years := int(_arg("years=", "25"))
	for i in range(years):
		Sim.world.advance_days(360)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)

	_map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		_map._camera.zoom = Vector2(3.0, 3.0)
		_map._camera.position = _map.iso_pos(cc.x, cc.y)
		_map.queue_redraw()
	var ov := _map.get_node_or_null("Overlay")

	# ── 1/9. FICHE PROVINCE — LA CAPITALE (tier 4, "ville") : items 1, 2, 9 ──
	_main._on_province_picked(cap_prov, cap_reg, me)
	_map._selected_prov = cap_prov
	if ov != null: ov.queue_redraw()
	await _shot("01_prov_capitale_ville")
	_reset()

	# ── 1/9bis. FICHE PROVINCE — un HAMEAU (petite pop, à moi si possible) ──
	var hamlet := -1
	var best_pop := 1e18
	var n: int = int(w.province_count())
	for p in range(n):
		var info: Dictionary = w.province_info(p)
		if not bool(info.get("valide", false)):
			continue
		if p == cap_prov:
			continue
		var pop := float(info.get("ames", 0))
		if int(info.get("owner", -1)) == me and pop > 0.0 and pop < best_pop:
			best_pop = pop
			hamlet = p
	if hamlet < 0:
		for p in range(n):
			var info2: Dictionary = w.province_info(p)
			if bool(info2.get("valide", false)) and p != cap_prov:
				hamlet = p
				break
	if hamlet >= 0:
		var hreg: int = w.province_region(hamlet)
		_main._on_province_picked(hamlet, hreg, int(w.province_info(hamlet).get("owner", -1)))
		_map._selected_prov = hamlet
		if ov != null: ov.queue_redraw()
		await _shot("02_prov_hameau")
		_reset()

	# ── 4. DIPLOMATIE (F7 — sidebar tab index 6) : header parchemin ──
	_main._sidebar.open_tab(6)
	await _shot("03_diplomatie_f7")
	_main._sidebar.close()

	# ── 3/10. JOURNAL (rail droit) : troncature + grammaire famine ──
	await _shot("04_journal_rail_droit")

	# ── 7/11. CONSTRUCTION — cartes entretien (plus de "~"), tooltip après clic ──
	_main._on_province_picked(cap_prov, cap_reg, me)
	_main._construct.visible = true
	_main._construct.target_pid = cap_prov
	if _main._construct.has_method("open_on"):
		_main._construct.open_on(0)
	_main._construct.queue_redraw()
	await _shot("05_construction_edifices")
	if _main._construct.has_method("open_on"):
		_main._construct.open_on(1)
		_main._construct.queue_redraw()
	await _shot("06_construction_manufactures")

	# item 11 — simule le hover PUIS le clic (le tooltip du bouton Construction ne doit
	# plus rester collé par-dessus la 1re carte). On force le hover via TooltipServer
	# directement (le timing réel de survol n'est pas reproductible en probe).
	var ttip = _main.get_node_or_null("TooltipServer")
	if ttip == null:
		for c in _main.get_children():
			if c.get_script() != null and String(c.get_script().resource_path).ends_with("tooltip_server.gd"):
				ttip = c
				break
	_reset()

	# ── 5/8/12. TRÉSOR (B) — Balance (sliders alignés), Monnaie (banqueroute rouge,
	#    fiscalité par ordre) ──
	_main._budget_v2.visible = true
	if _main._budget_v2.has_method("refresh"):
		_main._budget_v2.refresh()
	if _main._budget_v2.has_method("select_tab"):
		_main._budget_v2.select_tab(0)
	await _shot("07_tresor_balance")
	if _main._budget_v2.has_method("select_tab"):
		_main._budget_v2.select_tab(1)
		if _main._budget_v2.has_method("refresh"):
			_main._budget_v2.refresh()
	await _shot("08_tresor_monnaie")
	_reset()

	# ── 13. EXCLUSIVITÉ DES PANNEAUX — scénario du brief : Construction OUVRE d'abord
	#    (popup flottant non ancré, ex. bouton « Construire… » de la fiche), PUIS un
	#    panneau MAJEUR (Diplomatie/Trésor) s'ouvre PAR-DESSUS : la règle ne se déclenche
	#    QU'À L'OUVERTURE du majeur (ordre exact du bug rapporté : « la Construction
	#    reste ouverte SOUS la Diplomatie ») — Construction doit se refermer.
	_main._on_province_picked(cap_prov, cap_reg, me)
	_main._construct.visible = true
	_main._construct.target_pid = cap_prov
	await get_tree().process_frame
	print("ITEM13 construct ouvert AVANT le majeur: ", _main._construct.visible)
	_main._budget_v2.visible = true
	await get_tree().process_frame
	print("ITEM13 apres-ouverture-majeur construct.visible=", _main._construct.visible,
		" (attendu: false — l'ouverture du Trésor doit refermer Construction)")
	await _shot("09_item13_construct_ferme_par_major")
	_reset()

	# Échap répété : dépile dans l'ordre d'ouverture (dernier ouvert = premier fermé)
	_main._budget_v2.visible = true
	await get_tree().process_frame
	if _main._empire_win.has_method("open"):
		_main._empire_win.open()
	else:
		_main._empire_win.visible = true
	await get_tree().process_frame
	print("ITEM13 avant Echap: budget=", _main._budget_v2.visible, " empire=", _main._empire_win.visible)
	_main._close_topmost()
	await get_tree().process_frame
	print("ITEM13 apres 1er Echap: budget=", _main._budget_v2.visible, " empire=", _main._empire_win.visible,
		" (attendu: budget=true, empire=false — dernier ouvert = empire)")
	_main._close_topmost()
	await get_tree().process_frame
	print("ITEM13 apres 2e Echap: budget=", _main._budget_v2.visible, " empire=", _main._empire_win.visible,
		" (attendu: budget=false)")
	_reset()

	print("UIPOLISH SHOTS OK — ", _dir)
	get_tree().quit(0)
