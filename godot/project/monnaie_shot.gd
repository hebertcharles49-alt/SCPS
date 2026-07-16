extends Node
## monnaie_shot — CAPTURE DE L'ONGLET MONNAIE (UI-MONNAIE, 2026-07-16). Probe hors make,
## FENÊTRÉE (--headless = noir). Boote le VRAI shell (Main.tscn), lance une partie,
## avance le temps (dette/frappe/prix vivants), ouvre chaque écran concerné et sauve un
## PNG par état dans res://shots_monnaie/ + imprime les lecteurs bruts en console (preuve
## textuelle des lignes qui ne se capturent pas bien en PNG : hover, journal rare).
##   Godot --path godot/project res://monnaie_shot.tscn -- seed=9 years=60
var _main: Node = null
var _dir := "res://shots_monnaie/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_dir = "res://shots_monnaie/"
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

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return

	Sim.regenerate(int(_arg("seed=", "9")))
	await get_tree().process_frame
	var years := int(_arg("years=", "60"))
	for i in range(years):
		Sim.world.advance_days(360)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)

	var w = Sim.world
	var me: int = w.player()

	# ── PREUVE TEXTUELLE (console) — les lecteurs bruts, avant tout écran ──
	print("── UI-MONNAIE readers (seed=", _arg("seed=", "9"), " years=", years, ") ──")
	if w.has_method("country_price_level"):
		print("country_price_level(me)=", w.country_price_level(me))
	if w.has_method("world_price_index"):
		print("world_price_index()=", w.world_price_index())
	if w.has_method("country_debt"):
		print("country_debt(me)=", w.country_debt(me))
	if w.has_method("country_fiscal_orders"):
		print("country_fiscal_orders(me)=", w.country_fiscal_orders(me))
	if w.has_method("country_loan_capacity"):
		print("country_loan_capacity(me)=", w.country_loan_capacity(me))
	if w.has_method("country_debase_frac"):
		print("country_debase_frac(me)=", w.country_debase_frac(me))
	if w.has_method("country_bankruptcy_scar"):
		print("country_bankruptcy_scar(me)=", w.country_bankruptcy_scar(me))
	var cap_prov: int = w.country_capital_province(me)
	if w.has_method("province_income") and w.has_method("province_res_price"):
		for l in w.province_income(cap_prov):
			var rid := int(l.get("res_id", -1))
			var price: float = w.province_res_price(cap_prov, rid) if rid >= 0 else 0.0
			print("  raw ", l.get("source", "?"), " per_day=", l.get("per_day", 0.0),
				" res_id=", rid, " price=", price,
				" value/mois=", float(l.get("per_day", 0.0)) * 30.0 * price)
		# DIAGNOSTIC — tous les prix res_id 0..40 de la capitale (repérer si TOUS les
		# biens sont à 0 — price_level[me] très bas/nul — ou seulement les non-précieux).
		var nz := []
		for rid2 in range(40):
			var pr: float = w.province_res_price(cap_prov, rid2)
			if pr > 0.0:
				nz.append("%d=%.3f" % [rid2, pr])
		print("  prix non-nuls (res_id=valeur) : ", nz)

	# ── CAMÉRA sur la capitale ──
	var _map = _main.get_node("MapView")
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		_map._camera.zoom = Vector2(3.0, 3.0)
		_map._camera.position = _map.iso_pos(cc.x, cc.y)
		_map.queue_redraw()

	# ── 1. FICHE PROVINCE V2 (touche V) — la capitale, ses raws + manufactures ──
	_main._prov_panel_v2.show_province(cap_prov)
	await _shot("01_province_v2")
	_main._prov_panel_v2.hide()   # sinon le panneau MONNAIE (étape 2) est masqué dessous

	# ── 2. PANNEAU MONNAIE (touche B) — Balance, Monnaie, Marché ──
	_main._budget_v2.visible = true
	if _main._budget_v2.has_method("refresh"):
		_main._budget_v2.refresh()
	await _shot("02_budget_balance")
	if _main._budget_v2.has_method("select_tab"):
		_main._budget_v2.select_tab(1)
	await _shot("03_budget_monnaie")
	if _main._budget_v2.has_method("select_tab"):
		_main._budget_v2.select_tab(2)
	await _shot("04_budget_marche")
	_main._budget_v2.visible = false

	# ── 3. TOPBAR seule (le cell « Prix ») — HUD nu, rien d'ouvert ──
	if _main._prov_panel_v2.has_method("hide"):
		_main._prov_panel_v2.hide()
	await _shot("05_hud_topbar")

	# ── 4. FENÊTRE DIPLOMATIQUE — « Demander un emprunt » (ACTIONS ÉCONOMIQUES dépliées) ──
	var foe := -1
	for pass_role in [true, false]:
		if foe >= 0:
			break
		for c in range(w.country_count()):
			if c == me or w.country_province_count(c) <= 0:
				continue
			if pass_role and int(w.country_role(c)) != 1:
				continue
			if String(w.country_info(c).get("nom", "")).begins_with("Rebelles"):
				continue
			if w.has_method("country_known") and int(w.country_known(c)) == 0:
				continue
			foe = c
			break
	if foe >= 0:
		_main._country_actions.open_country(foe)
		# déplie « ACTIONS ÉCONOMIQUES » (migration/pacte/emprunt) — repli par défaut.
		var econ_box = _main._country_actions.get("_econ_box")
		if econ_box != null:
			econ_box.visible = true
		await _shot("06_diplo_emprunt")
		_main._country_actions.visible = false
	else:
		print("(aucun pays étranger connu — 06_diplo_emprunt omis)")

	# ── 5. JOURNAL (rail droit, empire_sidebar.gd) — PERMANENT dès Sim.game_on : déjà
	#    dans le cadre du HUD nu (05_hud_topbar) et de toutes les captures ci-dessus,
	#    pas de shot dédié (les conditions monétaires n'ont pas forcément eu le temps
	#    de se déclencher sur la durée du run — preuve textuelle ci-dessous à défaut). ──
	if w.has_method("country_bankruptcy_scar") or w.has_method("country_debase_frac"):
		print("journal U4 — scar(me)=", w.country_bankruptcy_scar(me) if w.has_method("country_bankruptcy_scar") else -1,
			" debase(me)=", w.country_debase_frac(me) if w.has_method("country_debase_frac") else -1,
			" (>0 ⇒ une ligne serait apparue au journal cette année)")

	print("MONNAIE SHOTS OK — ", _dir)
	get_tree().quit(0)
