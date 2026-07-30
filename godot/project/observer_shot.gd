extends Node
## observer_shot — preuve visuelle du MODE OBSERVATEUR (mission « Menu audio + mode
## observateur », 2026-07-30) : le chrome empire (topbar national, bande droite
## empire_sidebar, popups d'alerte) doit DISPARAÎTRE, la carte/date/vitesse doivent
## RESTER. Motif calqué sur d1_after_shot.gd (boot du VRAI Main.tscn, fenêtré seulement
## — --headless = hang connu, cf. TROUVAILLES).
##   Godot --path godot/project res://observer_shot.tscn -- seed=9 years=5
var _main: Node = null
var _dir := "res://shots_observer/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	get_window().unfocusable = true   # ne VOLE PAS le focus (« 25 alt-tab par minute », décision joueur)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
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

## boot commun à chaque capture : régénère, avance `years`, saute le menu, centre la
## carte sur la capitale du slot de focus (empire 0) — motif d1_after_shot.gd.
func _boot_world(seed_v: int, years: int) -> void:
	Sim.regenerate(seed_v)
	await get_tree().process_frame
	for i in range(years):
		Sim.world.advance_days(360)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)
	var map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		map._camera.zoom = Vector2(2.2, 2.2)
		map._camera.position = map.iso_pos(cc.x, cc.y)
		map.queue_redraw()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	var seed_v := int(_arg("seed=", "9"))
	var years := int(_arg("years=", "5"))

	# ── 01 : partie NORMALE (référence — le chrome empire complet) ──────────────
	await _boot_world(seed_v, years)
	await _shot("01_normal_chrome_complet")

	# ── 02 : MODE OBSERVATEUR — même graine/même avancement, set_observer(true)
	#    APRÈS la genèse (motif new_game_panel.gd::_on_lancer). Le chrome empire
	#    (topbar national, bande droite VILLES/ARMÉES/COLONISATION/MISSION/JOURNAL)
	#    doit disparaître ; la carte/date/vitesse restent. ─────────────────────────
	await _boot_world(seed_v, years)
	if Sim.world.has_method("set_observer"):
		Sim.world.set_observer(true)
	Sim.generated.emit()
	await _shot("02_observateur_chrome_masque")

	# ── 03 : en observateur, les fiches PROVINCE/PAYS restent consultables en
	#    LECTURE — preuve que "GARDER... les fiches province/pays en LECTURE" n'a
	#    pas été cassé par le masquage du chrome. country_panel.gd ne s'ouvre que
	#    pour un pays ÉTRANGER (show_country(me) se re-ferme volontairement, motif
	#    "chez soi = province panel" — donc on cible un AUTRE pays que le focus). ──
	var w = Sim.world
	var me: int = w.player()
	var foreign_cid := -1
	for c in range(int(w.country_count())):
		if c == me:
			continue
		if bool(w.country_info(c).get("valide", false)):
			foreign_cid = c
			break
	if foreign_cid >= 0 and _main._country_panel != null and _main._country_panel.has_method("show_country"):
		_main._country_panel.show_country(foreign_cid)
	await _shot("03_observateur_fiche_pays_etranger_lecture")

	# fiche PROVINCE (clic sur la capitale du focus, motif d1_after_shot.gd) —
	# reste disponible aussi, chrome empire toujours masqué autour.
	if _main._country_panel != null:
		_main._country_panel.show_country(-1)
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	_main._on_province_picked(cap_prov, cap_reg, me)
	await _shot("04_observateur_fiche_province_lecture")

	print("OBSERVER SHOTS OK — ", _dir)
	get_tree().quit(0)
