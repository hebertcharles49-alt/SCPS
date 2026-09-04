extends Node
## player_session — UNE PARTIE JOUÉE (mission OPUS JOUEUR, 2026-09-04).
## Boote le VRAI shell (Main.tscn, motif d7_icons_shot) pour que chaque capture
## montre l'écran TEL QUE LE JOUEUR LE VOIT (topbar + rails + panneau).
## FENÊTRÉE seulement (--headless = noir/hang connu).
##   Godot --path godot/project res://player_session.tscn -- seed=7 mode=recon
var _main: Node = null
var _map: Node2D = null
var _ui: CanvasLayer = null
var _army_panel: Control = null
var _dir := "res://shots_player/"
var _log: Array[String] = []

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

func _note(s: String) -> void:
	_log.append(s)
	print("[JOURNAL] ", s)

func _shot(nom: String) -> void:
	for i in range(8):
		await get_tree().process_frame
	# un joueur a cliqué « Vu » depuis longtemps : on ne laisse pas la boîte d'évènement
	# manger le panneau qu'on veut juger (elle se rouvre au fil du moteur).
	for nom2 in ["EventPopup", "EventDialog"]:
		var n: Node = _ui.get_node_or_null(nom2)
		if n != null and n is Control:
			(n as Control).visible = false
	for i in range(2):
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
	# Le panneau ARMÉE n'est PAS dans la pile Échap (il se ferme par désélection sur la
	# carte) et le popup « OYEZ » se ferme par son propre bouton : sans ça, les deux
	# restent AU-DESSUS du tiroir gauche et de tout panneau ouvert ensuite.
	if _army_panel != null:
		_army_panel.visible = false
	for nom in ["EventPopup", "EventDialog"]:
		var n: Node = _ui.get_node_or_null(nom)
		if n != null and n is Control:
			(n as Control).visible = false

func _years(n: int) -> void:
	for i in range(n):
		Sim.world.advance_days(360)

func _find_army_panel() -> Control:
	for c in _ui.get_children():
		if c is Control and c.has_method("set_army") and c.has_method("show_feedback"):
			return c
	return null

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	var seed_v := int(_arg("seed=", "7"))
	Sim.regenerate(seed_v)
	await get_tree().process_frame
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)
	_map = _main.get_node("MapView")
	_ui = _main.get_node("UI")
	_army_panel = _find_army_panel()
	print("army_panel trouvé = ", _army_panel != null)

	if _arg("mode=", "recon") == "recon":
		_recon()
		get_tree().quit(0)
		return
	await _play()
	get_tree().quit(0)

# ─────────────────────────── RECON (repérage du monde) ───────────────────────────
func _recon() -> void:
	var w = Sim.world
	var me: int = w.player()
	print("=== SEED ", Sim.current_seed, " · joueur = pays ", me, " ===")
	print("pays total = ", w.country_count(), " · provinces = ", w.province_count())
	print("--- MOI, an ", w.year(), " ---")
	print("country_info(me) = ", w.country_info(me))
	print("province_count(me) = ", w.country_province_count(me))
	print("capitale prov = ", w.country_capital_province(me))
	print("--- TAILLES DES PAYS ---")
	for c in range(int(w.country_count())):
		var inf: Dictionary = w.country_info(c)
		print("  pays %d : provs=%d pop=%s nom=%s" % [c, int(w.country_province_count(c)), str(w.country_pop(c)), str(inf.get("nom", "?"))])
	print("--- APRÈS 30 ANS ---")
	_years(30)
	print("an ", w.year(), " · provs=", w.country_province_count(me), " pop=", w.country_pop(me))
	print("country_info = ", w.country_info(me))
	print("country_budget = ", w.country_budget(me))
	print("country_stocks = ", w.country_stocks(me))
	print("influence_info = ", w.influence_info(me))
	print("doctrine_slots = ", w.doctrine_slots(me))
	print("doctrine_catalog(0) = ", w.doctrine_catalog(0))
	print("mission_info = ", w.mission_info(me))
	print("research_status = ", w.research_status())
	print("age_state = ", w.age_state())
	# une province à moi
	var mine: Array[int] = []
	for p in range(int(w.province_count())):
		var i2: Dictionary = w.province_info(p)
		if bool(i2.get("valide", false)) and int(i2.get("owner", -1)) == me:
			mine.append(p)
	print("mes provinces (", mine.size(), ") = ", mine.slice(0, 20))
	if mine.size() > 0:
		var pid: int = mine[0]
		print("province_info(", pid, ") = ", w.province_info(pid))
		print("  raws = ", w.province_raws(pid))
		print("  edifices = ", w.province_edifices(pid))
		print("  buildings = ", w.province_buildings(pid))
		print("  market = ", w.province_market(pid))
		print("  alloc = ", w.province_alloc(pid))
		print("  developpement = ", w.province_developpement(pid))
		print("  income = ", w.province_income(pid))
		print("  build_legal(0,edifice?) = ", w.build_legal(pid, 0))
	print("--- ARMÉE ---")
	print("country_army = ", w.country_army(me))
	print("corps_ids = ", w.corps_ids(me))
	print("unit_roster = ", w.unit_roster(me))
	print("--- DIPLO ---")
	print("country_relations = ", w.country_relations(me))
	print("country_known = ", w.country_known(me))
	print("diplo_options = ", w.diplo_options(1))
	print("--- DECRETS ---")
	print("decrees_list = ", w.decrees_list(me))
	print("--- COLONISATION ---")
	print("colony_status = ", w.colony_status())
	print("--- ANNALES ---")
	print("annals = ", w.annals())

# ─────────────────────────── LA PARTIE ───────────────────────────
## Chaque étape : je LIS l'écran (les mêmes readers que le panneau), je DÉCIDE,
## j'agis par le VERBE de la façade, je capture. Le journal part sur stdout.

## LES DILEMMES : un joueur RÉPOND aux boîtes de décision avant de reprendre la main
## (sinon elles s'empilent par-dessus tous les autres panneaux et le jeu reste en pause).
var _dilemmes := 0
func _answer_events() -> void:
	var w = Sim.world
	for guard in range(40):
		var n: int = int(w.pending_count())
		if n <= 0:
			return
		var pe: Dictionary = w.pending_event(0)
		if _dilemmes < 4:
			_note("  DILEMME : « %s » — %s" % [str(pe.get("title", pe.get("titre", "?"))), str(pe)])
		var opts: Array = pe.get("options", [])
		w.player_event_choice(0, 0)
		_dilemmes += 1
		if _dilemmes < 4:
			_note("  → je prends le 1er choix sur %d proposé(s)" % opts.size())
		_years(1)

func _tick(n: int) -> void:
	## avance n ANS et réveille les panneaux (Sim._process ne tourne pas ici).
	## PIÈGE : la topbar calcule ses deltas « /mois » comme (valeur − valeur au dernier
	## month_ticked). Si on saute 10 ans puis qu'on émet UN seul month_ticked, le delta
	## affiché vaut le saut ENTIER — un artefact de probe, pas un bug du jeu. On termine
	## donc chaque étape par DEUX vrais mois de 30 jours, chacun avec son month_ticked :
	## le dernier delta lu par la topbar est alors un VRAI mois.
	_years(n)
	_answer_events()
	Sim.generated.emit()
	for k in range(2):
		Sim.world.advance_days(30)
		if Sim.has_signal("month_ticked"):
			Sim.month_ticked.emit(Sim.world.year())
	if Sim.has_signal("ticked"):
		Sim.ticked.emit(Sim.world.year())

func _camera_on_prov(pid: int) -> void:
	var reg: int = Sim.world.province_region(pid)
	if reg < 0:
		return
	var cc: Vector2 = Sim.world.region_centroid(reg)
	_map._camera.zoom = Vector2(2.2, 2.2)
	_map._camera.position = _map.iso_pos(cc.x, cc.y)
	_map.queue_redraw()

func _my_provs() -> Array[int]:
	var w = Sim.world
	var me: int = w.player()
	var out: Array[int] = []
	for p in range(int(w.province_count())):
		var i2: Dictionary = w.province_info(p)
		if bool(i2.get("valide", false)) and int(i2.get("owner", -1)) == me:
			out.append(p)
	return out

func _show_prov(pid: int, tab: int) -> void:
	_reset()
	_main._sel_prov = pid
	_main._prov_panel_v2.show_province(pid)
	_main._prov_panel_v2.visible = true
	_main._prov_panel_v2.select_tab(tab)

func _open_construct(pid: int, tab: int) -> void:
	_main._construct.target_pid = pid
	_main._construct.open_on(tab)
	_main._construct.visible = true
	# comme main.gd:331 — le menu Construction se COLLE au bord droit de la fiche.
	# (Sans ça il retombe sur sa position par défaut, calculée pour une fiche de
	#  348 px, et la fiche actuelle est plus large : le panneau passe DESSOUS et
	#  la 1re lettre de chaque ligne est mangée.)
	var pp: Control = _main._prov_panel_v2
	if pp != null and pp.visible:
		_main._construct.position = Vector2(pp.position.x + pp.size.x + 6.0, pp.position.y)

func _play() -> void:
	var w = Sim.world
	var me: int = w.player()
	var cap: int = w.country_capital_province(me)
	_note("Je suis « %s » (pays %d), capitale = province %d. An %d." % [
		str(w.country_info(me).get("nom", "?")), me, cap, w.year()])

	# ── 1. LE MONDE À L'AN 0 : la carte, la topbar, les deux rails ────────────
	_tick(0)          # deux mois réels : la topbar a de VRAIS deltas à afficher
	_camera_on_prov(cap)
	_note("ÉTAPE 1 — j'ouvre le jeu. 0 clic. Je cherche : où suis-je, combien j'ai, ça va ?")
	_note("  topbar lit : " + str(w.country_info(me)))
	await _shot("01_carte_an0")

	# ── 1b. LA BOÎTE DE DÉCISION (le moteur m'interrompt : je dois trancher) ───
	_years(6)
	Sim.generated.emit()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.emit(w.year())
	for i in range(10):
		await get_tree().process_frame
	var dlg: Node = _ui.get_node_or_null("EventDialog")
	var pop: Node = _ui.get_node_or_null("EventPopup")
	_note("ÉTAPE 1b — an %d, le jeu m'interrompt. %d décision(s) en attente." % [w.year(), int(w.pending_count())])
	if int(w.pending_count()) > 0:
		_note("  dilemme brut = " + str(w.pending_event(0)))
	if (dlg != null and (dlg as Control).visible) or (pop != null and (pop as Control).visible):
		for i in range(6):
			await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png(_dir + "02_decision.png")
		print("SHOT 02_decision")
	_answer_events()

	# ── 2. LA FICHE PROVINCE (1 clic sur ma capitale) ─────────────────────────
	_tick(4)
	_note("ÉTAPE 2 — an %d. 1 clic sur ma capitale → la fiche." % w.year())
	_note("  province_info = " + str(w.province_info(cap)))
	_note("  raws = %s · edifices = %s · manufactures = %s" % [
		str(w.province_raws(cap)), str(w.province_edifices(cap)), str(w.province_buildings(cap))])
	_note("  marché de la tuile = " + str(w.province_market(cap)))
	_show_prov(cap, 0)
	await _shot("03_province_infrastructure")

	# ── 3. LE MENU CONSTRUCTION — édifices (2 clics) + JE CONSTRUIS ───────────
	_note("ÉTAPE 3 — 2e clic : « Construire… ». Je veux savoir ce qui est possible ET ce que ça coûte.")
	var roster: Array = w.building_roster(me)
	_note("  building_roster(%d entrées) = %s" % [roster.size(), str(roster.slice(0, 6))])
	_open_construct(cap, 0)
	await _shot("04_construction_edifices")
	# décision : le 1er édifice LÉGAL et différent de ceux déjà là
	var built := -1
	for eid in range(24):
		var lg: Dictionary = w.build_legal(cap, eid)
		if bool(lg.get("legal", false)) and bool(lg.get("allowed", true)):
			var nm: String = str(w.edifice_name(eid))
			var have := false
			for e in w.province_edifices(cap):
				if str(e.get("nom", "")) == nm:
					have = true
			if have:
				continue
			var rc: bool = bool(w.player_build(eid, cap))
			_note("  DÉCISION : je bâtis « %s » (édifice %d) → verbe rendu %s" % [nm, eid, str(rc)])
			built = eid
			break
	if built < 0:
		_note("  RIEN de légal à bâtir — je ne sais pas pourquoi en lisant l'écran.")
	_tick(2)
	_show_prov(cap, 0)
	_open_construct(cap, 0)
	await _shot("05_construction_apres_achat")

	# ── 4. LES MANUFACTURES (l'autre onglet) ──────────────────────────────────
	_note("ÉTAPE 4 — onglet Manufactures. Je cherche : quoi transformer avec MES 2 brutes ?")
	var manuf_ok := -1
	for b in range(40):
		if int(w.manuf_legal(cap, b)) == 1:      # 1 = légale (comme construction_panel.gd:513)
			if manuf_ok < 0:
				manuf_ok = b
			_note("  manuf %d « %s » LÉGALE · recette = %s · coût = %s · entretien = %s" % [
				b, str(w.manuf_name(b)), str(w.manuf_recipe(b)),
				str(w.manuf_cost()), str(w.manuf_upkeep_month(cap, b))])
	_open_construct(cap, 1)
	await _shot("06_construction_manufactures")
	if manuf_ok >= 0:
		var rc2: bool = bool(w.player_build_manuf(cap, manuf_ok))
		_note("  DÉCISION : manufacture « %s » → %s" % [str(w.manuf_name(manuf_ok)), str(rc2)])
	else:
		_note("  AUCUNE manufacture légale ici — l'écran ne me dit pas laquelle viser.")

	# ── 5. L'ALLOCATION (le levier joueur sur la main-d'œuvre) ────────────────
	_tick(8)
	_note("ÉTAPE 5 — an %d. Où vont mes bras ? Onglet allocation de la tuile." % w.year())
	var al: Dictionary = w.province_alloc(cap)
	_note("  alloc AVANT = " + str(al))
	var sinks: Array = al.get("sinks", [])
	if sinks.size() >= 2:
		var s0: Dictionary = sinks[0]
		var rc3: bool = bool(w.player_alloc_raw(cap, int(s0.get("id", 0)), 90))
		_note("  DÉCISION : je pousse « %s » à 90 %% → %s" % [str(s0.get("name", "?")), str(rc3)])
	_tick(1)
	_note("  alloc APRÈS = " + str(w.province_alloc(cap)))
	_show_prov(cap, 0)
	await _shot("07_province_apres_allocation")

	# ── 6. LES STOCKS (rail gauche, F3) ───────────────────────────────────────
	_tick(6)
	_note("ÉTAPE 6 — an %d. Ai-je de quoi ? Rail gauche → Stocks (1 clic)." % w.year())
	var st: Array = w.country_stocks(me)
	for l in st.slice(0, 6):
		_note("  stock : %s — %s · %s en réserve · prix %s" % [
			str(l.get("name", "?")), str(l.get("marche", "?")), str(l.get("stock", "?")), str(l.get("price", "?"))])
	_reset()
	_main._sidebar.open_tab(2)
	await _shot("08_tiroir_stocks")

	# ── 7. LE MARCHÉ (F4) ─────────────────────────────────────────────────────
	_note("ÉTAPE 7 — rail gauche → Marché. Je veux acheter ce qui manque.")
	_note("  market_quote(Outils=33, 100) = " + str(w.market_quote(me, 33, 100)))
	_reset()
	_main._sidebar.open_tab(3)
	await _shot("09_tiroir_marche")

	# ── 8. LE TRÉSOR (touche B) ───────────────────────────────────────────────
	_note("ÉTAPE 8 — touche B, le Trésor. Combien j'ai, combien j'encaisse PAR MOIS ?")
	_note("  country_gold = %s · reserve = %s" % [str(w.country_gold(me)), str(w.country_reserve(me))])
	_note("  budget_summary = " + str(w.budget_summary(me)))
	_note("  country_budget = " + str(w.country_budget(me)))
	_reset()
	_main._budget_v2.visible = true
	_main._budget_v2.select_tab(0)
	await _shot("10_tresor_balance")

	# ── 9. LES DOCTRINES (cellule Influence de la topbar) ─────────────────────
	_tick(10)
	_note("ÉTAPE 9 — an %d. L'influence s'accumule : à quoi elle sert ?" % w.year())
	var inf: Dictionary = w.influence_info(me)
	_note("  influence_info = " + str(inf))
	var cat: Array = w.doctrine_catalog(me)
	for c in cat.slice(0, 4):
		_note("  doctrine « %s » — coût %s · dispo %s · %s" % [
			str(c.get("name", "?")), str(c.get("cost", "?")), str(c.get("available", "?")), str(c.get("reason", ""))])
	_reset()
	_main._doctrine_panel.open()
	await _shot("11_doctrines_avant")
	# DÉCISION : la 1re doctrine abordable
	var adopted := -1
	for c in cat:
		if bool(c.get("available", false)):
			var did: int = int(c.get("id", -1))
			var rc4 = w.doctrine_adopt(0, did)
			_note("  DÉCISION : j'adopte « %s » dans le slot 0 → %s" % [str(c.get("name", "?")), str(rc4)])
			adopted = did
			break
	if adopted < 0:
		_note("  AUCUNE doctrine abordable — le panneau me dit-il quand je pourrai ?")
	_tick(6)
	if adopted >= 0:
		_note("  doctrine_detail = " + str(w.doctrine_detail(me, adopted)))
		var rc5 = w.doctrine_buy_idea(adopted)
		_note("  DÉCISION : j'achète une idée dans « %d » → %s" % [adopted, str(rc5)])
		_tick(3)
	_reset()
	_main._doctrine_panel.open()
	await _shot("12_doctrines_apres")

	# ── 10. LA RECHERCHE (touche T) ───────────────────────────────────────────
	_note("ÉTAPE 10 — touche T, l'arbre. Que puis-je apprendre ?")
	_note("  research_status = " + str(w.research_status()))
	_note("  tech_info = " + str(w.tech_info()))
	var nodes: Array = w.tech_nodes()
	_note("  %d nœuds ; les 2 premiers = %s" % [nodes.size(), str(nodes.slice(0, 2))])
	var picked := -1
	for i in range(nodes.size()):
		var nd: Dictionary = nodes[i]
		if bool(nd.get("allowed", false)) and str(nd.get("reason_code", "")) == "ok":
			picked = i
			_note("  je vise « %s » : %s · j'ai %s points, il en manque %s" % [
				str(nd.get("name", "?")), str(nd.get("reason_label", "")),
				str(nd.get("points_have", "?")), str(nd.get("points_missing", "?"))])
			break
	if picked >= 0:
		_note("  DÉCISION : recherche %d → %s" % [picked, str(w.player_research(picked))])
		_tick(4)
		_note("  research_status APRÈS = " + str(w.research_status()))
	else:
		_note("  AUCUN nœud ouvert — l'arbre ne me propose rien à viser.")
	_reset()
	_main._tech.visible = true
	_main._tech.queue_redraw()
	await _shot("13_technologie")

	# ── 11. LA DIPLOMATIE (clic droit sur un voisin) ───────────────────────────
	_tick(10)
	_note("ÉTAPE 11 — an %d. Qui sont mes voisins et que puis-je leur dire ?" % w.year())
	var rel: Array = w.country_relations(me)
	_note("  relations = " + str(rel))
	var target := -1
	if rel.size() > 0:
		target = int(rel[0].get("country", -1))
	if target >= 0:
		var op: Dictionary = w.diplo_options(target)
		_note("  diplo_options(%d) = %s" % [target, str(op)])
		_note("  opinion_summary = " + str(w.opinion_summary(target)))
		if bool(op.get("can_offer_pact", false)):
			_note("  DÉCISION : pacte à « %s » → %s" % [str(rel[0].get("name", "?")), str(w.player_offer_pact(target))])
		elif bool(op.get("can_offer_alliance", false)):
			_note("  DÉCISION : alliance à « %s » → %s" % [str(rel[0].get("name", "?")), str(w.player_offer_alliance(target))])
	_reset()
	if target >= 0:
		_main._country_actions.open_country(target)
	await _shot("14_diplomatie_pays")
	_reset()
	_main._sidebar.open_tab(6)
	await _shot("15_tiroir_diplomatie")

	# ── 12. L'ARMÉE (lever un corps) ──────────────────────────────────────────
	_tick(8)
	_note("ÉTAPE 12 — an %d. Je lève une armée. Que puis-je recruter, et avec quoi ?" % w.year())
	_note("  country_army = " + str(w.country_army(me)))
	var roster2: Array = w.unit_roster(me)
	var recruited := 0
	for u in roster2:
		if not bool(u.get("recrutable", false)):
			continue
		_note("  recrutable : « %s » — %s · entretien %s or/10 %s vivre" % [
			str(u.get("nom", "?")), str(u.get("cout", "?")),
			str(u.get("entretien_or10", "?")), str(u.get("entretien_vivre", "?"))])
		for k in range(4):
			if bool(w.player_recruit(int(u.get("type", 0)))):
				recruited += 1
		if recruited >= 8:
			break
	_tick(2)
	_note("  après recrutement : country_army = %s (%d ordres passés)" % [str(w.country_army(me)), recruited])
	var cap_reg: int = w.country_capital_region(me)
	var reserve: int = int(w.country_army(me).get("regiments", 0))
	var packets: int = maxi(1, int(reserve / 2))
	var rc6 = w.player_raise_corps(packets, cap_reg)
	_note("  DÉCISION : je lève %d paquet(s) sur la région %d (réserve %d) → %s" % [packets, cap_reg, reserve, str(rc6)])
	w.player_set_levy(2)
	_note("  je monte la levée d'un cran (niveau 2)")
	_tick(3)
	var ids: Array = w.corps_ids(me)
	_note("  corps_ids = %s" % str(ids))
	for i in ids:
		_note("  corps_info(%s) = %s" % [str(i), str(w.corps_info(int(i)))])
	_reset()
	_main._sidebar.open_tab(4)
	await _shot("16_tiroir_armee")
	if ids.size() > 0 and _army_panel != null:
		_army_panel.set_army(ids)
		_army_panel.visible = true
		await _shot("17_panneau_armee")

	# ── 13. LES ORIENTATIONS / DÉCRETS (Conseil → sous-onglet) ────────────────
	_note("ÉTAPE 13 — Conseil → Orientations. Je cherche un levier durable et lisible.")
	var decs: Array = w.decrees_list(me)
	for d in decs.slice(0, 4):
		_note("  décret « %s » — légal %s · %s" % [str(d.get("nom", "?")), str(d.get("legal", "?")), str(d.get("plateaux", ""))])
	for d in decs:
		if bool(d.get("legal", false)) and not bool(d.get("active", false)):
			_note("  DÉCISION : j'active « %s » → %s" % [str(d.get("nom", "?")), str(w.player_decree(int(d.get("id", 0)), true))])
			break
	_tick(2)
	_reset()
	_main._sidebar.open_tab(7)
	_main._sidebar._drawer._conseil_tab = 1
	_main._sidebar._drawer.queue_redraw()
	await _shot("18_conseil_orientations")

	# ── 14. LA COLONISATION ───────────────────────────────────────────────────
	_tick(10)
	_note("ÉTAPE 14 — an %d. Il reste des terres vierges ? Je regarde la carte." % w.year())
	_note("  colony_status = " + str(w.colony_status()))
	var free_pid := -1
	for p in range(int(w.province_count())):
		var cc2 = w.can_colonize(p)
		var okc := false
		if typeof(cc2) == TYPE_DICTIONARY:
			okc = bool(cc2.get("ok", cc2.get("legal", false)))
		else:
			okc = bool(cc2)
		if okc:
			free_pid = p
			_note("  colonisable : province %d (%s) — %s" % [p, str(w.province_info(p).get("nom", "?")), str(cc2)])
			break
	if free_pid >= 0:
		_show_prov(free_pid, 0)
		await _shot("19_province_vierge")
		_note("  DÉCISION : je colonise %d → %s" % [free_pid, str(w.player_colonize(free_pid))])
		_tick(2)
		_note("  colony_status = " + str(w.colony_status()))
	else:
		_note("  RIEN de colonisable — le monde est-il plein ? l'écran ne le dit pas.")

	# ── 15. LA FENÊTRE EMPIRE (touche E) ──────────────────────────────────────
	_tick(15)
	_note("ÉTAPE 15 — an %d. Bilan de règne : touche E." % w.year())
	_note("  country_info = " + str(w.country_info(me)))
	_note("  provinces = %d · pop = %s" % [int(w.country_province_count(me)), str(w.country_pop(me))])
	_reset()
	_main._empire_win.open()
	_main._empire_win.select_tab(0)
	await _shot("20_empire_economie")
	_main._empire_win.select_tab(1)
	await _shot("21_empire_population")

	# ── 16. LES ANNALES (touche H) ────────────────────────────────────────────
	_note("ÉTAPE 16 — touche H, les Annales. Qu'est-ce que j'ai vécu ?")
	var an: Array = w.annals()
	_note("  %d lignes d'annales" % an.size())
	for a in an:
		_note("  · " + str(a.get("ligne", "?")))
	_reset()
	_main._chronique.open()
	await _shot("22_annales")

	# ── 17. LA CARTE À LA FIN ─────────────────────────────────────────────────
	_reset()
	_camera_on_prov(cap)
	_note("ÉTAPE 17 — an %d, fin de partie. mission_info = %s" % [w.year(), str(w.mission_info(me))])
	_note("  age_state = " + str(w.age_state()))
	_note("  endgame_info = " + str(w.endgame_info()))
	_note("  mes provinces = %d" % _my_provs().size())
	# le MOT « influence » dit-il la même chose partout ? (topbar vs panneau doctrines)
	_note("  topbar lit country_info.influence = %s" % str(w.country_info(me).get("influence", "?")))
	_note("  doctrines lisent influence_info = %s" % str(w.influence_info(me)))
	_note("  research_status final = %s · tech_info = %s" % [str(w.research_status()), str(w.tech_info())])
	_note("  stocks (prix) = %s" % str(w.country_stocks(me).slice(0, 3)))
	_note("  budget_summary final = %s" % str(w.budget_summary(me)))
	_note("  country_budget final = %s" % str(w.country_budget(me)))
	await _shot("23_carte_fin")
	_reset()
	_main._sidebar.open_tab(0)
	await _shot("24_tiroir_economie_fin")
	print("=== FIN DE PARTIE : an ", w.year(), " ===")
