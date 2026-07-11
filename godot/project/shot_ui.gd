extends Node
## shot_ui — CAPTURE DE L'UI COMPLÈTE (audit visuel). Probe hors make, FENÊTRÉE
## (--headless = noir). Boote le VRAI shell (Main.tscn), lance une partie, ouvre
## chaque panneau et sauve un PNG par état dans res://shots_ui/.
##   Godot --path godot/project res://shot_ui.tscn -- seed=9 years=25
var _main: Node = null
var _map: Node2D = null
var _dir := "res://shots_ui/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	# res=WxH (défaut 1920x1080) + sortie dans un sous-dossier par résolution
	# (probe 3 résolutions, docs/UI_RECO_2026-07-10.md §3.4 — sans clobber).
	var rs := _arg("res=", "1920x1080").split("x")
	var rw := (int(rs[0]) if rs.size() == 2 else 1920)
	var rh := (int(rs[1]) if rs.size() == 2 else 1080)
	if rw < 320: rw = 1920
	if rh < 240: rh = 1080
	get_window().size = Vector2i(rw, rh)
	_dir = "res://shots_ui/%dx%d/" % [rw, rh]
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	# La probe force-quit (get_tree().quit) → feedback.gd ne nettoie jamais son drapeau
	# de session ⇒ la RUN SUIVANTE affiche « Fermeture anormale détectée » par-dessus
	# les captures. On le retire ici (fichier de probe, non embarqué) pour des shots propres.
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

## referme tout panneau flottant + la sélection (état HUD propre entre deux états)
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

	# ── 1. LE MENU (état d'accueil) ──
	await _shot("01_menu")

	# ── 1bis. ÉCRANS DE SETUP (Nouvelle partie + créateur d'empire en onglets) —
	#    dans un CanvasLayer HAUT (le menu vit dans un layer au-dessus du défaut :
	#    sans ça les captures montrent le menu, pas le setup — payé 2026-07-10). ──
	var lay := CanvasLayer.new()
	lay.layer = 100
	add_child(lay)
	var ng: Control = load("res://ui/new_game_panel.gd").new()
	lay.add_child(ng)
	await _shot("01b_newgame")
	ng._on_compose(0)
	await _shot("01c_creator")
	if ng._creator != null:
		ng._creator._tabs.current_tab = 1   # l'onglet ÉTHOS (lore explicative)
		await _shot("01c2_creator_ethos")
		ng._creator._tabs.current_tab = 2   # l'onglet TRADITIONS (3 sections plates)
		await _shot("01d_creator_trad")
		ng._creator._tabs.current_tab = 3   # l'onglet IDENTITÉ (armoiries/nom/peuple)
		await _shot("01e_creator_identite")
	lay.queue_free()
	await get_tree().process_frame

	# ── LANCER LA PARTIE (même geste que « Lancer » : regen + game_on + menu caché) ──
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
	Sim.set_speed(0)   # pause : les captures sont stables

	_map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	# caméra : plan moyen sur la capitale du joueur
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		_map._camera.zoom = Vector2(3.0, 3.0)
		_map._camera.position = _map.iso_pos(cc.x, cc.y)
		_map.queue_redraw()
	var ov := _map.get_node_or_null("Overlay")
	if ov != null:
		ov.queue_redraw()

	# ── 2. LE HUD nu (topbar + rails + alertes) ──
	await _shot("02_hud")

	# ── 3. PROVINCE À SOI (capitale) : panneau province + panneau pays ──
	_main._on_province_picked(cap_prov, cap_reg, me)
	_map._selected_prov = cap_prov
	if ov != null:
		ov.queue_redraw()
	await _shot("03_prov_own")

	# ── 4. LE DÉTAIL de province (touche V) ──
	_main._prov_detail.show_province(cap_prov)
	_main._prov_detail.visible = true
	_main._prov_detail.queue_redraw()
	await _shot("04_prov_detail")
	_reset()

	# ── 5. PROVINCE ÉTRANGÈRE (celle d'une IA) ──
	# Sélection ROBUSTIFIÉE (2026-07-10) : les anciens critères tombaient sur un PAYS
	# REBELLE transitoire (Phase 3a : slot « Rebelles de X », panneau quasi vide) et/ou
	# un pays JAMAIS DÉCOUVERT — country_actions.open_country a un gate de brouillard
	# (country_known==0 → return silencieux) ⇒ 06_diplo capturait une fenêtre ABSENTE.
	# Critères : ≠ joueur · possède des provinces · CONNU (fog) · pas un rebelle.
	var foe := -1
	for pass_role in [true, false]:   # 1er passage : rôle 1 (empire IA) ; 2e : n'importe qui
		if foe >= 0:
			break
		for c in range(w.country_count()):
			if c == me or w.country_province_count(c) <= 0:
				continue
			if pass_role and int(w.country_role(c)) != 1:
				continue
			if String(w.country_info(c).get("nom", "")).begins_with("Rebelles"):
				continue   # rebelle transitoire (guerre civile) — pas une cible diplo stable
			if w.has_method("country_known") and int(w.country_known(c)) == 0:
				continue   # jamais découvert : open_country refuserait (gate de brouillard)
			foe = c
			break
	if foe >= 0:
		var fprov: int = w.country_capital_province(foe)
		var freg: int = w.province_region(fprov)
		_main._on_province_picked(fprov, freg, foe)
		_map._selected_prov = fprov
		if ov != null:
			ov.queue_redraw()
		await _shot("05_prov_foreign")
		_reset()

		# ── 6. LA FENÊTRE DIPLOMATIQUE (verbes + opinion) ──
		_main._country_actions.open_country(foe)
		await _shot("06_diplo")
		_main._country_actions.visible = false

	# ── 7-14. LES 8 TIROIRS de la sidebar ──
	var noms := ["economie", "demographie", "stocks", "marche", "armee", "filtres", "diplomatie", "conseil"]
	for i in range(8):
		_main._sidebar.open_tab(i)
		await _shot("%02d_drawer_%s" % [7 + i, noms[i]])
	# ── 14b. Sous-onglet POLITIQUES du tiroir Conseil (orientations + décisions +
	#    peuple servile) — recâblage 2026-07-10 sur docs/CONSEIL_ORIENTATIONS_2026-07-10.md.
	#    Le tiroir Conseil est encore ouvert (dernière itération ci-dessus) ; on bascule
	#    juste le sous-onglet (var script `_conseil_tab`, cf. sidebar_drawer.gd) avant
	#    de fermer, pour voir le nouveau catalogue rendu réellement.
	var _drawer = _main._sidebar.get("_drawer")   # sidebar_drawer.gd (le contenu réel du tiroir)
	if _drawer != null and "_conseil_tab" in _drawer:
		_drawer._conseil_tab = 1
		_drawer.queue_redraw()
		await _shot("14b_drawer_politiques")
		# ── 14c. le SCROLL GÉNÉRIQUE du tiroir (2026-07-11) : offset poussé au max
		#    (le _draw clampe au contenu) — vérifie que le bas (décision Audit +
		#    Peuple servile) se révèle, que l'en-tête reste FIXE et que la barre
		#    piste+pouce suit. ──
		if "_scroll" in _drawer:
			_drawer._scroll[7] = 10000.0   # le _draw clampe au contenu (offset max réel)
			_drawer.queue_redraw()
			# 2 frames de STABILISATION : le 1er _draw re-clampe l'offset et re-queue —
			# capturer trop tôt fige le transitoire (payé : capture vide, 2026-07-11).
			for _k2 in range(2):
				await get_tree().process_frame
				_drawer.queue_redraw()
			await _shot("14c_drawer_politiques_scrolled")
			_drawer._scroll[7] = 0.0
			_drawer.queue_redraw()
	_main._sidebar.close()

	# ── 15. ARBRE DE TECH (touche T) ──
	_main._tech.visible = true
	_main._tech.queue_redraw()
	await _shot("15_tech")
	_main._tech.visible = false

	# ── 16. CONSTRUCTION (touche B) — sur la capitale sélectionnée ──
	_main._on_province_picked(cap_prov, cap_reg, me)
	_main._construct.visible = true
	_main._construct.queue_redraw()
	await _shot("16_construct")
	_reset()

	# ── 17. ÉCONOMIE DANS LE TEMPS (courbes) ──
	_main._econ.visible = true
	_main._econ.queue_redraw()
	await _shot("17_econ")
	_main._econ.visible = false

	# ── 18. CODEX (F1) ──
	_main._codex.visible = true
	_main._codex.queue_redraw()
	await _shot("18_codex")
	_main._codex.visible = false

	# ── 19. LES ANNALES (H) ──
	if _main._chronique.has_method("open"):
		_main._chronique.open()
	else:
		_main._chronique.visible = true
	await _shot("19_chronique")
	_main._chronique.visible = false

	print("UI SHOTS OK — ", _dir)
	get_tree().quit(0)
