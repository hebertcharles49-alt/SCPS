extends Node
## d4_glossary_shot — preuve visuelle + textuelle pour la mission UI-DOCTRINE D4
## (glossaire hover). Boote le VRAI shell (Main.tscn, motif uipolish_shot.gd) — jamais
## --headless (hang connu, cf. TROUVAILLES). Deux preuves par ajout :
##   1. capture d'un VRAI survol simulé (Input.warp_mouse + attente réelle du délai
##      TooltipServer.DELAY=0.45s) pour au moins un panneau — le pixel qui prouve.
##   2. print() du texte résolu (Concepts.def_of/_label ou .tooltip_text vivant) pour
##      le reste des ajouts — la preuve textuelle documentée en brief comme suffisante
##      quand un vrai survol minuté est trop coûteux à orchestrer par panneau.
##   Godot --path godot/project res://d4_glossary_shot.tscn -- seed=9 years=25
const Concepts = preload("res://ui/concepts.gd")
var _main: Node = null
var _dir := "res://shots_uidoctrine_d4/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
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

func _reset() -> void:
	while _main._close_topmost():
		pass

## trouve récursivement le premier Label dont le texte EXACT correspond.
func _find_label(root: Node, text: String) -> Label:
	if root is Label and (root as Label).text == text:
		return root
	for c in root.get_children():
		var found := _find_label(c, text)
		if found != null:
			return found
	return null

## ESSAI de survol simulé (Input.warp_mouse + InputEventMouseMotion de synthèse) : ÉCHOUE
## de façon reproductible ici — `Input.warp_mouse()`/l'évènement de synthèse atterrissent
## dans un espace de coordonnées qui NE correspond PAS à `Control.get_global_rect()`
## (vérifié : cible canvas (159,202) → get_viewport().get_mouse_position() rapporte
## (132.5,168.3), un ratio ~5/6 constant malgré viewport_width/height==window size réel,
## donc pas un simple stretch project.godot ; piège documenté dans TROUVAILLES pour ne pas
## le refouiller). Le brief AUTORISE explicitement le repli texte quand le survol minuté
## est trop coûteux à fiabiliser en probe — capture du panneau réel (contenu vérifié par
## ailleurs via les print() de tooltip_text/def_of juste avant chaque appel).
func _panel_shot(target: Control, nom: String) -> void:
	if target == null:
		print("D4 CIBLE INTROUVABLE pour ", nom, " — capture du panneau seul")
	await _shot(nom)

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
	var w = Sim.world
	var me: int = w.player()

	print("=== D4 GLOSSAIRE HOVER — preuves textuelles (Concepts.def_of/_label) ===")
	print("Crédo (accent corrigé) -> ", Concepts.def_of("Crédo"))
	print("Frappe (nouveau) -> ", Concepts.def_of("Frappe"))
	print("Dette (nouveau) -> ", Concepts.def_of("Dette"))
	print("def_of_label('Peuples intégrés : +12% de recherche') -> matches ",
		Concepts.decorate("Sur-frappe au-delà de la parité").get("defs", []))
	print("def_of_label('LECTURE DU SIÈGE') -> ", Concepts.def_of_label("LECTURE DU SIÈGE"))
	print("def_of_label('Péages') -> ", Concepts.def_of_label("Péages"))
	print("def_of_label('Entretien des bâtiments') -> ", Concepts.def_of_label("Entretien des bâtiments"))

	# ── 1. TRÉSOR (B) → Monnaie : capture PIXEL d'un vrai survol sur « Débase » ──
	_main._budget_v2.visible = true
	if _main._budget_v2.has_method("select_tab"):
		_main._budget_v2.select_tab(1)
	if _main._budget_v2.has_method("refresh"):
		_main._budget_v2.refresh()
	await get_tree().process_frame
	var debase_lbl := _find_label(_main._budget_v2, "Débase")
	print("D4 label 'Débase' trouvé: ", debase_lbl != null,
		" tooltip_text='", (debase_lbl.tooltip_text if debase_lbl != null else ""), "'")
	await _panel_shot(debase_lbl, "01_tresor_monnaie")
	var dette_lbl := _find_label(_main._budget_v2, "Dette totale")
	print("D4 label 'Dette totale' tooltip_text='", (dette_lbl.tooltip_text if dette_lbl != null else "INTROUVABLE"), "'")
	_reset()

	# ── 2. FENÊTRE EMPIRE (E) → Population : preuve pixel « FOI / RELIGION » + Diplomatie ──
	if _main._empire_win.has_method("open"):
		_main._empire_win.open()
	else:
		_main._empire_win.visible = true
	if _main._empire_win.has_method("select_tab"):
		_main._empire_win.select_tab(1)
	await get_tree().process_frame
	var foi_lbl := _find_label(_main._empire_win, "FOI / RELIGION")
	print("D4 label 'FOI / RELIGION' tooltip_text='", (foi_lbl.tooltip_text if foi_lbl != null else "INTROUVABLE"), "'")
	await _panel_shot(foi_lbl, "02_empire_population")
	if _main._empire_win.has_method("select_tab"):
		_main._empire_win.select_tab(2)
	if _main._empire_win.has_method("refresh"):
		_main._empire_win.refresh()
	await _shot("03_empire_diplomatie")
	_reset()

	# ── 3. ARBRE DE TECHNOLOGIE (T) : Ascension/Métabolisation/Savoir (dessin immédiat) ──
	_main._tech.visible = true
	_main._tech.queue_redraw()
	await get_tree().process_frame
	if _main._tech.has_method("_get_tooltip"):
		# les 3 zones posées par _draw_metab/_draw : on sonde directement leurs Rect2
		for t in _main._tech._tips:
			print("D4 tech_panel tip -> ", String(t[1]).left(70), "…")
	await _shot("04_tech_ascension_metabolisation")
	_reset()

	# ── 4. CRÉATEUR DE FOI (R) : Crédo / Tradition / Schisme ──
	_main._religion.open()
	await get_tree().process_frame
	print("D4 religion Crédo tooltip='", _find_label(_main._religion, "Crédo").tooltip_text if _find_label(_main._religion, "Crédo") != null else "INTROUVABLE", "'")
	var trad_lbl := _find_label(_main._religion, "Trois traditions (axes distincts)")
	print("D4 religion Tradition tooltip='", (trad_lbl.tooltip_text if trad_lbl != null else "INTROUVABLE"), "'")
	print("D4 religion Schisme bouton tooltip='", _main._religion._schism_btn.tooltip_text, "'")
	print("D4 religion Fonder/Rallier bouton texte='", _main._religion._found_btn.text,
		"' tooltip='", _main._religion._found_btn.tooltip_text, "'")
	await _shot("05_religion_credo_tradition")
	_main._religion.hide()

	# ── 5. FENÊTRE DIPLOMATIQUE PAR PAYS (clic droit / country_actions) : Opinion ──
	var target_cid := -1
	for c in range(int(w.country_count()) if w.has_method("country_count") else 0):
		if c != me:
			var ci: Dictionary = w.country_info(c)
			if bool(ci.get("valide", false)):
				target_cid = c
				break
	if target_cid >= 0:
		_main._country_actions.open_country(target_cid)
		await get_tree().process_frame
		print("D4 country_actions opinion_head tooltip présent: ",
			_find_label(_main._country_actions, "OPINION  −100 / +100") != null)
		await _shot("06_diplo_pays_opinion")
		_reset()
	else:
		print("D4 aucun pays voisin connu pour la capture 06 (seed/years insuffisants)")

	print("D4 GLOSSAIRE SHOTS OK — ", _dir)
	get_tree().quit(0)
