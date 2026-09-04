extends Node
## ui1_audit — la sonde des SIX CORRECTIFS D'INTERFACE (mission UI-1, 2026-09-04).
## Motif d7_icons_shot/player_session : on instancie le VRAI shell (Main.tscn) — une
## sonde qui monte un panneau SEUL perd la topbar, les deux rails et le thème de
## fenêtre, donc ne montre PAS l'écran du joueur (piège noté par OPUS JOUEUR).
## FENÊTRÉE seulement (--headless = noir/hang connu sur ce projet).
##   Godot --path godot/project res://ui1_audit.tscn -- seed=7 years=25 tag=avant
## Chaque point du brief a SA capture et SES lignes de journal — le même binaire tourne
## AVANT et APRÈS le correctif, seul `tag=` change.
var _main: Node = null
var _ui: CanvasLayer = null
var _dir := "res://shots_ui1/"
var _tag := "avant"
var _viol := 0

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
	for i in range(8):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(_dir + _tag + "_" + nom + ".png")
	print("SHOT ", _tag, "_", nom)

func _hide_modals() -> void:
	for nom in ["EventPopup", "EventDialog"]:
		var n: Node = _ui.get_node_or_null(nom)
		if n != null and n is Control:
			(n as Control).visible = false

func _reset() -> void:
	var guard := 0
	while _main._close_topmost() and guard < 30:
		guard += 1
	var sb: Control = _main._sidebar
	if sb != null and sb.has_method("close"):
		sb.close()
	_hide_modals()

func _ko(msg: String) -> void:
	_viol += 1
	print("  ✗ ", msg)

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("ui1_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	_tag = _arg("tag=", "avant")
	Sim.regenerate(int(_arg("seed=", "7")))
	await get_tree().process_frame
	for i in range(int(_arg("years=", "25"))):
		Sim.world.advance_days(360)
	Sim.generated.emit()
	Sim.month_ticked.emit(int(Sim.world.year()))
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sound.stop_music()
	Sim.game_on = true
	Sim.set_speed(0)
	_ui = _main.get_node("UI")
	var map = _main.get_node("MapView")
	var w = Sim.world
	var me: int = w.player()
	var cap_prov: int = w.country_capital_province(me)
	var cap_reg: int = w.province_region(cap_prov)
	if cap_reg >= 0:
		var cc: Vector2 = w.region_centroid(cap_reg)
		map._camera.zoom = Vector2(3.0, 3.0)
		map._camera.position = map.iso_pos(cc.x, cc.y)
		map.queue_redraw()
	# deux mois PROPRES : les deltas « /mois » de la topbar valent (valeur − valeur au
	# dernier month_ticked) ; sans ça la barre affiche le saut ENTIER des 25 ans (piège
	# de sonde relevé par OPUS JOUEUR).
	for i in range(2):
		w.advance_days(30)
		Sim.month_ticked.emit(int(w.year()))
	_reset()
	print("=== UI1 AUDIT (%s) — six correctifs d'interface ===" % _tag)

	# ── 1. TOPBAR + RAILS (contraste des nombres, icônes sur le cuir) ──────────
	await _shot("01_topbar_rails")

	# ── 2. MENU CONSTRUCTION : les VERBES d'effet sont-ils DÉFINIS ? ───────────
	_main._prov_panel_v2.show_province(cap_prov)
	_main._construct.target_pid = cap_prov
	_main._construct.open_on(0)
	# le menu retombe sur une position calculée pour une fiche de 348 px (piège OPUS
	# JOUEUR) : on le repose à droite de la fiche RÉELLE.
	var pp: Control = _main._prov_panel_v2
	_main._construct.position.x = pp.position.x + pp.size.x + 6.0
	await _shot("02_construction_edifices")
	_audit_gloss()
	# LE HOVER, POUR DE VRAI : on pose la souris sur une carte et on laisse le
	# TooltipServer monter (0,45 s de survol, cf. sa constante DELAY).
	await _hover_shot("02b_hover_effet", "Port")
	_main._construct.open_on(1)
	await _shot("03_construction_manufactures")
	_dump_cards("MANUFACTURES")
	await _hover_shot("03b_hover_manufacture", "")
	_reset()

	# ── 3. LA PILE D'ÉCHAP ────────────────────────────────────────────────────
	await _audit_escape()

	# ── 4. DEUX MODALES À LA FOIS ─────────────────────────────────────────────
	await _audit_modals()

	# ── 5. LE JOURNAL nomme-t-il les lieux ? ──────────────────────────────────
	_reset()
	await _shot("06_journal_rail_droit")
	_audit_journal()

	print("UI1 AUDIT (%s) — %d anomalie(s)" % [_tag, _viol])
	get_tree().quit(0)

## ── 2. les MOTS d'effet des cartes de construction, et leur définition ────────
func _audit_gloss() -> void:
	var w = Sim.world
	var me: int = w.player()
	var panel = _main._construct
	print("  — TERMES D'EFFET des cartes (édifices) —")
	var termes := {}
	for b in w.building_roster(me):
		for t in String(b.get("effet", "")).split(" · ", false):
			var mot := String(t).strip_edges()
			# « +3 prospérité » → le MOT seul (le nombre est la force de l'édifice)
			var parts := mot.split(" ", false)
			if parts.size() > 1 and String(parts[0]).begins_with("+"):
				mot = " ".join(parts.slice(1))
			termes[mot] = true
	var gloss := {}
	if w.has_method("glossary"):
		for g in w.glossary():
			gloss[String(g.get("mot", "")).to_lower()] = String(g.get("def", ""))
	for mot in termes.keys():
		var d := String(gloss.get(String(mot).to_lower(), ""))
		if d == "":
			_ko("terme d'effet SANS définition de façade : « %s »" % mot)
		else:
			print("    ✓ %s → %s" % [mot, d])
	# et ce que le HOVER de CHAQUE carte dit réellement (le dossier get_info_card lu par
	# le TooltipServer — c'est LUI que le joueur verra)
	var cards := _cards_of(panel)
	if cards.is_empty():
		_ko("aucune carte dans le menu Construction")
		return
	_dump_cards("ÉDIFICES")

## le dossier get_info_card de chaque carte visible — c'est LUI que le joueur survole.
func _dump_cards(onglet: String) -> void:
	print("  — HOVERS de l'onglet %s —" % onglet)
	for c in _cards_of(_main._construct):
		var card: Dictionary = c.card_data
		print("    « %s »" % String(card.get("title", "")))
		for l in card.get("lines", []):
			print("      • %s : %s" % [String(l.get("label", "")), String(l.get("value", ""))])

## pose RÉELLEMENT la souris sur la carte dont le titre contient `titre`, laisse le
## TooltipServer monter, puis capture. Fenêtré obligatoire (le survol GUI n'existe pas
## en headless). No-op silencieux si la carte est absente.
func _hover_shot(nom: String, titre: String) -> void:
	var cible: Control = null
	for c in _cards_of(_main._construct):
		if titre == "" or String((c.card_data as Dictionary).get("title", "")).contains(titre):
			cible = c        # titre vide = la PREMIÈRE carte de l'onglet courant
			break
	if cible == null:
		print("    (hover : carte « %s » absente)" % titre)
		return
	var at := cible.get_global_rect().position + Vector2(60, 14)
	Input.warp_mouse(at)
	for i in range(90):                     # ≈1,5 s : DELAY 0,45 s + la frame de layout
		await get_tree().process_frame
	await _shot(nom)
	Input.warp_mouse(Vector2(8, 8))         # on range la souris : pas de tooltip résiduel
	for i in range(10):
		await get_tree().process_frame

func _cards_of(panel) -> Array:
	var out := []
	var stack := [panel]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		for c in n.get_children():
			if c is Control and "card_data" in c:
				out.append(c)
			stack.append(c)
	return out

## ── 3. Échap : pop-ups → fenêtres → menu → JEU, jamais un quit ───────────────
func _audit_escape() -> void:
	print("  — PILE D'ÉCHAP —")
	var esc := InputEventKey.new()
	esc.keycode = KEY_ESCAPE
	esc.pressed = true
	# trois fenêtres ouvertes en pile
	_main._budget_v2.visible = true
	_main._empire_win.visible = true
	_main._construct.target_pid = Sim.world.country_capital_province(Sim.world.player())
	_main._construct.open_on(0)
	await get_tree().process_frame
	var ouverts := _open_panels()
	print("    départ : %d panneau(x) ouvert(s) — %s" % [ouverts.size(), str(ouverts)])
	for i in range(6):
		_main._unhandled_input(esc)
		await get_tree().process_frame
		var o := _open_panels()
		var m: bool = _main._menu != null and _main._menu.visible
		print("    Échap #%d → %d ouvert(s), menu=%s" % [i + 1, o.size(), str(m)])
		if m:
			await _shot("04_menu_ingame")
			# LE POINT DU JOUEUR : un Échap de plus doit RENDRE LA MAIN AU JEU.
			_main._unhandled_input(esc)
			await get_tree().process_frame
			if _main._menu.visible:
				_ko("Échap dans le menu in-game ne rend PAS la main au jeu (le menu reste)")
				await _shot("05_menu_bloque")
			else:
				print("    ✓ Échap referme le menu in-game : retour au jeu")
				await _shot("05_retour_au_jeu")
			return
		if o.is_empty() and i >= 3:
			break
	_ko("Échap n'a jamais ouvert le menu in-game")

func _open_panels() -> Array:
	var out := []
	for c in _ui.get_children():
		if c is Control and (c as Control).visible and c.name in [
				"ConstructionPanel", "BudgetV2", "EmpireWindow", "TechPanel",
				"BudgetPanelV2", "Chronique", "Codex", "EventPopup", "EventDialog"]:
			out.append(String(c.name))
	return out

## ── 4. deux modales d'évènement : une seule à la fois, jamais l'une SOUS l'autre ──
func _audit_modals() -> void:
	print("  — DEUX MODALES —")
	var w = Sim.world
	if _main._menu != null:
		_main._menu.hide()            # le menu du point 3 ne doit pas voiler la scène jugée ici
	var popup: Control = _ui.get_node_or_null("EventPopup")
	var dialog: Control = _ui.get_node_or_null("EventDialog")
	if popup == null or dialog == null:
		_ko("EventPopup/EventDialog introuvables")
		return
	_hide_modals()
	# une VRAIE décision en attente (le dilemme du joueur), au plus tard à l'an 80
	var found := false
	for i in range(60):
		if w.has_method("pending_count") and int(w.pending_count()) > 0:
			found = true
			break
		w.advance_days(360)
	if found:
		dialog._open_slot(0)
		await get_tree().process_frame
	else:
		print("    (aucune décision en attente — on juge la file des OYEZ seuls)")
	# … et un OYEZ OYEZ qui arrive PENDANT
	popup.enqueue({"title": "Première nouvelle", "body": "La première arrive.",
		"kind": 1, "buttons": [{"label": "Vu", "act": "close"}]})
	popup.enqueue({"title": "Deuxième nouvelle", "body": "La deuxième arrive juste après.",
		"kind": 6, "buttons": [{"label": "Vu", "act": "close"}]})
	await get_tree().process_frame
	print("    dialogue=%s · popup=%s · file=%d" % [str(dialog.visible), str(popup.visible),
		popup._queue.size()])
	await _shot("07_double_modale")
	if dialog.visible and popup.visible:
		_ko("DEUX modales visibles en même temps (l'une est en sous-couche)")
	elif found and not dialog.visible:
		_ko("le dialogue de décision a été recouvert/perdu")
	else:
		print("    ✓ une seule modale à l'écran")
	# la file doit reprendre à la fermeture
	if dialog.visible:
		dialog._choose(0)
		await get_tree().process_frame
		print("    après le choix : popup=%s file=%d" % [str(popup.visible), popup._queue.size()])
		if not popup.visible:
			_ko("l'OYEZ en attente ne s'affiche pas après la fermeture du dialogue")
		else:
			await _shot("08_oyez_apres_dialogue")
	_hide_modals()

## ── 5. le JOURNAL nomme-t-il ses lieux ? ─────────────────────────────────────
func _audit_journal() -> void:
	print("  — JOURNAL (rail droit) —")
	var alerts: Node = _ui.get_node_or_null("Alerts")
	if alerts == null:
		_ko("nœud Alerts introuvable")
		return
	alerts._refresh()
	var rows: Array = alerts.journal_rows()
	print("    %d ligne(s)" % rows.size())
	var re := RegEx.new()
	re.compile("(?:Prov\\.\\d+|r[ée]gion\\s+\\d+|province\\s+\\d+)")
	for e in rows:
		var t := String(e.get("tip", ""))
		print("      · %s" % t)
		if re.search(t) != null:
			_ko("le journal nomme un lieu par un NUMÉRO : « %s »" % t)
	# le lecteur de façade lui-même, sur TOUTES les régions
	var w = Sim.world
	var stub := 0
	for r in range(w.region_count()):
		var nm := String(w.region_label(r)) if w.has_method("region_label") else ""
		if nm == "" or re.search(nm) != null:
			stub += 1
	print("    region_label : %d région(s) sur %d rendues par un numéro" % [stub, w.region_count()])
	if stub > 0:
		_ko("region_label rend encore un identifiant pour %d région(s)" % stub)
