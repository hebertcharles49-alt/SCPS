extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène et
## relier la sélection de carte aux panneaux de lecture (la membrane → UI).

const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")
const NavigationHub = preload("res://ui/navigation_hub.gd")
const UIKit = preload("res://ui/uikit.gd")   # load_img() export-safe (curseur, etc.)

var _country_panel: Control
var _sidebar: Control
var _construct: Control
var _tech: Control
var _econ: Control
var _budget_v2: Control        # PILOTE « grand livre parchemin » (conteneurs natifs + Theme), touche B
var _prov_panel_v2: Control    # PILOTE fiche province « conteneurs natifs + Theme parchemin », touche V
var _empire_win: Control       # FENÊTRE EMPIRE à onglets (Économie/Population/Diplomatie/Conseil), touche E
var _prov_detail: Control
var _menu: Control
var _religion: Control
var _country_actions: Control  # fenêtre diplomatique par pays (liste diplo + clic droit)
var _devpanel: Control         # MODTOOLS : panneau dev (tunables live, F10)
var _chronique: Control        # LES ANNALES DU RÈGNE : le récit sélectif (lecture seule, H)
var _age_recap: Control        # ÉCRAN DE CHAPITRE : récap d'âge au clic du chip « Engager »
var _page_turn: CanvasLayer    # LA PAGE QUI SE TOURNE : transition d'âge (codex, horloge mur)
var _epilogue: Control         # ÉPILOGUE : la fin de partie en une phrase + la frise complète
var _battle_panel: Control     # W-GUERRE UI (lot B) : panneau de combat, ouvert par clic sur un jeton d'armée
var _codex: Control            # LE CODEX DES VERBES (touche F1) : tout ce que le joueur peut faire
var _search_palette: Control   # P5 : Ctrl+K, accès universel aux objets et explications
var _memory_panel: Control     # P9 : récents, épingles et comparaison live
var _faith_prompted := false   # le créateur de foi ne s'ouvre qu'UNE fois (1er édifice religieux)
var _epilogue_shown := false   # l'épilogue ne s'ouvre qu'UNE fois par partie (latch UI)
var _nav: Node
var _sel_prov := -1
var _sel_owner := -1           # dernier propriétaire vu (restaure CountryPanel à la fermeture d'un écran profond)

# ── UI-POLISH #13 : PILE « dernier ouvert » + exclusivité des panneaux MAJEURS ────────
# Échap doit fermer le panneau au PREMIER PLAN (le dernier ouvert), pas un ordre fixe
# arbitraire ; et un panneau MAJEUR (Trésor/Diplomatie) doit refermer les POPUPS
# FLOTTANTS non ancrés (Construction) à l'ouverture — la fiche province, contextuelle-
# ancrée, coexiste toujours (jamais dans cette liste). Un SEUL point d'écoute par
# panneau (`visibility_changed`) : marche quel que soit le chemin de code qui a posé
# `.visible` (raccourci, bouton, signal…), sans toucher aux dizaines de sites d'ouverture.
var _panel_stack: Array = []

# PANNEAUX FLOTTANTS DÉPLAÇABLES (display-only) : glisser par le bandeau-titre.
# Un simple CLIC sur un bouton d'en-tête ne bouge jamais (aucun mouvement) → le bouton
# tire encore ; seul un press-puis-déplace fait glisser. C'est la désambiguïsation.
const DRAG_HEADER_H := 40.0
var _drag_panel: Control = null
var _drag_press := Vector2.ZERO
var _drag_off := Vector2.ZERO
var _dragging := false

func _ready() -> void:
	_nav = NavigationHub.new()
	_nav.name = "NavigationHub"
	add_child(_nav)
	_nav.navigate_requested.connect(_route_navigation)
	Sim.generated.connect(func(): _nav.clear())
	Sim.new_game_started.connect(func(_seed): _nav.clear_memory())
	Sim.game_saved.connect(func(slot): _nav.save_memory(slot))
	Sim.game_loaded.connect(func(slot): _nav.load_memory(slot))

	# THÈME GLOBAL + feedback de clic : états de bouton visibles (hover/pressed/disabled)
	# hérités par TOUTE l'UI, flash de clic accroché à chaque BaseButton (présent + futur).
	var UiTheme := load("res://ui/ui_theme.gd")
	get_window().theme = UiTheme.build()
	UiTheme.attach_feedback(get_tree())
	# TOOLTIPS À CONCEPTS (retour joueur 2026-07-10) : le tooltip natif est neutralisé
	# (délai énorme) — le TooltipServer le remplace partout (mots-concepts turquoise +
	# définitions, registre ui/concepts.gd, relié au codex).
	ProjectSettings.set_setting("gui/timers/tooltip_delay_sec", 100000.0)
	var tts := CanvasLayer.new()
	tts.name = "TooltipLayer"
	tts.layer = 120
	add_child(tts)
	var tooltip_server = load("res://ui/tooltip_server.gd").new()
	tts.add_child(tooltip_server)
	tooltip_server.navigate_requested.connect(func(request): _nav.go(request))
	_setup_cursor()
	# les ARMOIRIES dérivent des faits du monde → le cache se vide à chaque genèse
	Sim.generated.connect(func(): load("res://ui/heraldry.gd").reset())

	# la carte (Node2D, caméra dedans)
	var map_script := load("res://map/map_view.gd")
	var map: Node2D = map_script.new()
	map.name = "MapView"
	add_child(map)

	# l'UI, sur une couche écran (au-dessus de la carte, indépendante de la caméra)
	var ui := CanvasLayer.new()
	ui.name = "UI"
	add_child(ui)

	# (cadre enluminé RETIRÉ — demande joueur : pas de bordure autour de la carte)

	var topbar_script := load("res://ui/topbar.gd")
	var topbar: Control = topbar_script.new()
	topbar.name = "Topbar"
	ui.add_child(topbar)
	topbar.navigate_requested.connect(func(request): _nav.go(request))
	topbar.tech_requested.connect(func():
		_tech.visible = not _tech.visible
		if _tech.visible:
			Sound.play("ui_parchment_open")
		_tech.queue_redraw())

	_country_panel = load("res://ui/country_panel.gd").new()
	_country_panel.name = "CountryPanel"
	ui.add_child(_country_panel)
	_country_panel.close_requested.connect(_clear_selection)

	# le DESTIN du monde (§27) : barre d'entropie + bandeau de fin, haut-centre
	var endgame = load("res://ui/endgame_banner.gd").new()
	endgame.name = "EndgameBanner"
	ui.add_child(endgame)

	# le RAIL de sidebar (onglets menu_*) + tiroir — gauche
	_sidebar = load("res://ui/sidebar.gd").new()
	_sidebar.name = "Sidebar"
	ui.add_child(_sidebar)
	_sidebar.setup(map)                       # le tiroir Filtres pilote la carte
	# tiroir ouvert ⇒ on cache le panneau de province (même bande, exclusifs)
	_sidebar.tab_selected.connect(func(i): if i >= 0: _prov_panel_v2.show_province(-1))

	# barres de carte : sélecteur de mode (bas-gauche) + zoom (bas-droite)
	var controls = load("res://ui/controls.gd").new()
	controls.name = "MapControls"
	controls.setup(map)
	ui.add_child(controls)

	# CONSTRUCTION : les boutons de levée & de bâti (touche B) — lit la façade
	# (roster 22 unités + édifices, prix réels). Caché par défaut, bascule au clavier.
	_construct = load("res://ui/construction_panel.gd").new()
	_construct.name = "ConstructionPanel"
	# à DROITE du panneau province FLOTTANT (SIDEBAR_W+14, 348 de large) : ouvert
	# depuis lui, il le recouvrait
	_construct.position = Vector2(Frame.SIDEBAR_W + 14 + 348 + 12, Frame.TOPBAR_H + 12)
	_construct.visible = false
	ui.add_child(_construct)

	# W-GUERRE UI (lot B) : le panneau de combat, ouvert par clic sur un jeton d'armée
	# (siège ou bataille en cours) — cf. _on_province_picked. Caché par défaut.
	_battle_panel = load("res://ui/battle_panel.gd").new()
	_battle_panel.name = "BattlePanel"
	_battle_panel.visible = false
	ui.add_child(_battle_panel)
	_battle_panel.close_requested.connect(func(): _battle_panel.visible = false)

	# ARBRE DE TECH (touche T) : l'arbre du joueur, rendu en GRAPHE (medusa) sur
	# une trame radiale (chart 2D). Lit tech_info/tech_nodes. Caché par défaut.
	_tech = load("res://ui/tech_panel.gd").new()
	_tech.name = "TechPanel"
	_tech.visible = false
	ui.add_child(_tech)

	# ÉCONOMIE DANS LE TEMPS (touche G) : graphes Easy Charts (pop · trésor ·
	# prospérité), historique accumulé an par an. Caché par défaut, read-only.
	_econ = load("res://ui/economy_panel.gd").new()
	_econ.name = "EconomyPanel"
	_econ.visible = false
	ui.add_child(_econ)
	# les COURBES sont DERRIÈRE le sous-menu Économie (sidebar) — pas affichées d'office
	_sidebar.charts_requested.connect(func():
		_econ.visible = not _econ.visible
		_econ.queue_redraw())

	# PROVINCE — DÉTAIL (touche V) : graphes Easy Charts des flux + camemberts +
	# classes de la province SÉLECTIONNÉE. Caché par défaut, read-only.
	_prov_detail = load("res://ui/province_detail.gd").new()
	_prov_detail.name = "ProvinceDetail"
	_prov_detail.visible = false
	ui.add_child(_prov_detail)

	# Construction depuis le panneau province V2 (ex-legacy, D1-UNIFICATION) : le
	# wiring build_requested vit désormais avec l'instanciation de _prov_panel_v2
	# (plus bas) — la fiche province est UNIQUE, ce site double n'existe plus.
	# … et depuis l'onglet CONSTRUCTIONS du détail (sa maison désormais)
	_prov_detail.build_requested.connect(func():
		_construct.target_pid = _sel_prov
		var was_visible := _construct.visible
		_construct.visible = true
		if not was_visible:
			Sound.play("ui_parchment_open")
		_construct.queue_redraw())

	# EMPIRE SIDEBAR (droite) : résumé d'empire (villes/armées/colonisation/flotte) + LOG
	var esb = load("res://ui/empire_sidebar.gd").new()
	esb.name = "EmpireSidebar"
	ui.add_child(esb)

	# FENÊTRE DIPLOMATIQUE PAR PAYS : ouverte par la liste diplo (sidebar) et le CLIC DROIT
	_country_actions = load("res://ui/country_actions.gd").new()
	_country_actions.name = "CountryActions"
	ui.add_child(_country_actions)
	_country_actions.navigate_requested.connect(func(request: Dictionary):
		_nav.go(request))
	map.country_context.connect(func(owner):
		if Sim.game_on and owner != Sim.world.player():
			_sidebar.close()
			navigate_to(InfoRef.make(InfoRef.COUNTRY, owner), "actions"))
	_sidebar.open_country.connect(func(cid):
		_sidebar.close()
		navigate_to(InfoRef.make(InfoRef.COUNTRY, cid), "actions"))

	# ZONE CONTEXTUELLE UNIQUE (retour joueur 2026-07-10, UI-3) : un écran profond
	# REMPLACE le panneau contextuel qu'il détaille, il ne s'y ajoute jamais — le regard
	# reste sur 3-4 zones, pas 5. Hooké sur le SIGNAL (visibility_changed) plutôt que sur
	# chaque site d'ouverture : couvre la pile Échap (_close_topmost), les ouvertures ET
	# la probe shot_ui (qui pose `.visible` en direct, hors des signaux dédiés).
	_prov_detail.visibility_changed.connect(func():
		if _prov_detail.visible:
			_prov_panel_v2.visible = false             # le détail REMPLACE le panneau province
		elif _sel_prov >= 0:
			_prov_panel_v2.show_province(_sel_prov))    # fermeture du détail → le panneau REVIENT
	_country_actions.visibility_changed.connect(func():
		if _country_actions.visible:
			_country_panel.visible = false           # la fenêtre diplo REMPLACE le panneau pays
		elif _sel_owner >= 0:
			_country_panel.show_country(_sel_owner))

	# ARMÉE : le pion sélectionné ouvre sa barre de COMMANDEMENT (recompléter/piller/
	# dissoudre) ; le clic-destination sur la carte donne l'ordre de marche/attaque.
	var army_panel: Control = load("res://ui/army_panel.gd").new()
	ui.add_child(army_panel)
	map.army_selection_changed.connect(army_panel.set_army)
	map.army_order_feedback.connect(army_panel.show_feedback)
	map.army_move_preview_changed.connect(army_panel.set_move_preview)
	army_panel.selection_replaced.connect(map._set_selected_corps)
	army_panel.raid_requested.connect(func(): map.arm_raid())

	# la carte SÉLECTIONNE → on remplit les panneaux (lecture seule de la membrane)
	map.province_picked.connect(func(province, region, owner):
		if province >= 0:
			navigate_to(InfoRef.make(InfoRef.PROVINCE, province))
		else:
			_on_province_picked(province, region, owner))

	# MENU PRINCIPAL (Jouer/Charger/Options/Quitter) par-dessus la carte, au démarrage.
	# Topmost (ajouté en dernier). Le monde par défaut est déjà généré derrière ; « Lancer
	# la partie » le régénère selon le setup (sliders + cultures) puis laisse en PAUSE an 0.
	_menu = load("res://ui/menu_root.gd").new()
	_menu.name = "MenuRoot"
	ui.add_child(_menu)
	# CODEX depuis le menu Échap (F1 est parti aux onglets du rail, 2026-07-10)
	if _menu.has_signal("codex_requested"):
		_menu.codex_requested.connect(func():
			_menu.hide()
			if _codex != null:
				_codex.toggle())

	# RELIGION — le CRÉATEUR DE FOI : s'ouvre quand le joueur bâtit son 1er édifice religieux
	# (avant, le monde est ATHÉE). Rouvrable à la touche R. Caché par défaut.
	_religion = load("res://ui/religion_panel.gd").new()
	_religion.name = "ReligionPanel"
	_religion.visible = false
	ui.add_child(_religion)
	_religion.closed.connect(func(): Sim.set_speed(2))   # fermer le créateur → le jeu reprend
	Sim.ticked.connect(_on_tick_faith)                   # surveille la pose du 1er édifice religieux

	_devpanel = load("res://ui/devpanel.gd").new()       # MODTOOLS : tunables live (F10)
	_devpanel.name = "DevPanel"
	_devpanel.visible = false
	ui.add_child(_devpanel)

	# LE CODEX DES VERBES (touche F1) : l'enseignement — tout ce que le joueur peut
	# FAIRE, et où. Lecture seule, zéro logique de sim. Caché par défaut.
	_codex = load("res://ui/codex.gd").new()
	_codex.name = "Codex"
	_codex.visible = false
	ui.add_child(_codex)
	# PILOTE budget « grand livre parchemin » (conteneurs Godot natifs + Theme, touche B).
	# COEXISTE avec l'économie existante (economy_panel/sidebar) — ne remplace rien.
	_budget_v2 = load("res://ui/budget_panel_v2.gd").new()
	_budget_v2.name = "BudgetPanelV2"
	_budget_v2.visible = false
	_budget_v2.add_to_group("draggable")
	ui.add_child(_budget_v2)
	# LA fiche province (D1-UNIFICATION, 2026-07-18) : conteneurs natifs + Theme
	# parchemin, doctrine « bâti seul + hover + /mois ». province_panel.gd (legacy,
	# dessin immédiat, nomenclature divergente Laboureurs/Artisans/Noblesse) est
	# SUPPRIMÉ — cette fiche reçoit désormais le clic-carte ET la touche V.
	_prov_panel_v2 = load("res://ui/province_panel_v2.gd").new()
	_prov_panel_v2.name = "ProvincePanelV2"
	_prov_panel_v2.visible = false
	_prov_panel_v2.add_to_group("draggable")
	ui.add_child(_prov_panel_v2)
	_prov_panel_v2.close_requested.connect(_clear_selection)   # ✕ = désélection pleine
	_prov_panel_v2.detail_requested.connect(func():
		if _prov_detail != null and _sel_prov >= 0:
			_prov_detail.show_province(_sel_prov)            # le DÉTAIL (main-d'œuvre & cie) s'ouvre enfin
			_prov_detail.visible = true
			Sound.play("ui_parchment_open")
			_prov_detail.queue_redraw())
	# le bouton « Construire… » de la fiche ouvre le MENU CONSTRUCTION sur la
	# province visée, à l'onglet demandé (0 Édifices · 1 Manufactures).
	if _prov_panel_v2.has_signal("build_requested"):
		_prov_panel_v2.build_requested.connect(func(kind: int):
			_construct.target_pid = _sel_prov
			if _construct.has_method("open_on"):
				_construct.open_on(kind)
			else:
				_construct.visible = true
			_construct.position = Vector2(_prov_panel_v2.position.x + _prov_panel_v2.size.x + 6.0,
										  _prov_panel_v2.position.y)
			Sound.play("ui_parchment_open"))
	# FENÊTRE EMPIRE : UNE fenêtre à onglets (Économie · Population · Diplomatie · Conseil,
	# touche E). L'architecture réelle de gestion — coexiste avec la sidebar/les pilotes.
	_empire_win = load("res://ui/empire_window.gd").new()
	_empire_win.name = "EmpireWindow"
	_empire_win.visible = false
	_empire_win.add_to_group("draggable")
	ui.add_child(_empire_win)
	# D1-UNIFICATION : l'onglet Économie de la Fenêtre Empire est lecture seule — son
	# lien « Régler… » ouvre LE Trésor (seule surface de réglage fiscal/budgétaire).
	if _empire_win.has_signal("open_budget_requested"):
		_empire_win.open_budget_requested.connect(func():
			if _budget_v2 != null:
				_budget_v2.visible = true
				if _budget_v2.has_method("refresh"):
					_budget_v2.refresh())

	# UI-POLISH #13 — la pile d'Échap : chaque panneau suivi s'auto-empile/désempile via
	# SON PROPRE visibility_changed (peu importe le chemin de code qui a posé .visible).
	# _construct = le seul POPUP FLOTTANT non ancré de la liste ; _budget_v2/_empire_win/
	# _country_actions = MAJEURS (Trésor/Diplomatie) — leur ouverture referme _construct.
	# La fiche province (_prov_panel_v2/_country_panel), contextuelle-ancrée,
	# N'EST PAS dans cette liste : elle coexiste toujours (règle joueur explicite).
	for maj in [_budget_v2, _empire_win, _country_actions]:
		if maj != null:
			var m: Control = maj
			m.visibility_changed.connect(func():
				_panel_stack.erase(m)
				if m.visible:
					_panel_stack.append(m)
					if _construct != null and _construct.visible:
						_construct.visible = false)
	if _construct != null:
		_construct.visibility_changed.connect(func():
			_panel_stack.erase(_construct)
			if _construct.visible:
				_panel_stack.append(_construct))

	_search_palette = load("res://ui/search_palette.gd").new()
	_search_palette.name = "SearchPalette"
	ui.add_child(_search_palette)
	_search_palette.navigate_requested.connect(func(request): _nav.go(request))
	_memory_panel = load("res://ui/memory_panel.gd").new()
	_memory_panel.name = "CampaignMemory"
	ui.add_child(_memory_panel)
	_memory_panel.setup(_nav)
	_memory_panel.navigate_requested.connect(func(request): _nav.go(request))

	# ALERTES (façon EU4/CK3) : la pile des « éléments en attente » au bord droit —
	# code couleur par domaine, clic = le panneau concerné (ou le geste direct).
	var alerts = load("res://ui/alerts.gd").new()
	alerts.name = "Alerts"
	ui.add_child(alerts)
	# Les alertes ne flottent plus sur la carte : le ledger droit les rend en liste,
	# tandis que ce nœud conserve leur collecte et leurs actions.
	alerts.set_ledger_mode(true)
	esb.set_alert_source(alerts)
	# AUDIT UI 1.4 : alerts n'a pas de référence à Main → un Callable lu chaque frame
	# (major_open() n'existe qu'ICI, sur Main, où vivent tous les panneaux majeurs).
	alerts.major_open_fn = Callable(self, "major_open")
	alerts.open_tab.connect(func(i): navigate_to(InfoRef.make(InfoRef.SIDEBAR_TAB, i), "sidebar"))
	alerts.open_tech.connect(func():
		navigate_to(InfoRef.make(InfoRef.TECH, -1)))
	alerts.open_construct.connect(func():
		if not _construct.visible:
			Sound.play("ui_parchment_open")
		_construct.visible = true
		_construct.queue_redraw())
	alerts.open_religion.connect(func():
		if _religion != null:
			_religion.open())
	# MÉTABOLISATION PRÊTE (V1b) : tech_panel surveille heritage_access() et notifie au
	# franchissement du tier 3 — la pile d'alertes pousse un chip transient, dont le
	# clic ROUVRE l'arbre (route déjà existante open_tech).
	if _tech.has_signal("metab_ready"):
		_tech.metab_ready.connect(func(nom): alerts.push_metab_ready(nom))
	alerts.open_tech_metab.connect(func():
		navigate_to(InfoRef.make(InfoRef.TECH, -1), "", {"section": "metabolisation"}))
	var goto_fn := func(r):
		if r >= 0:
			navigate_to(InfoRef.make(InfoRef.REGION, r), "map")
	alerts.goto_region.connect(goto_fn)
	esb.goto_region.connect(goto_fn)
	esb.open_country.connect(func(cid):
		navigate_to(InfoRef.make(InfoRef.COUNTRY, cid), "actions"))

	# OYEZ OYEZ : le popup d'évènement (directeur + alertes majeures) — PAUSE + boutons
	# adaptatifs ; les kinds majeurs du fil y sont ROUTÉS par alerts (popup_requested).
	var popup = load("res://ui/event_popup.gd").new()
	popup.name = "EventPopup"
	ui.add_child(popup)
	alerts.popup_requested.connect(popup.enqueue)
	popup.goto_region.connect(goto_fn)
	popup.open_tab.connect(func(i): navigate_to(InfoRef.make(InfoRef.SIDEBAR_TAB, i), "sidebar"))

	# LES ANNALES DU RÈGNE (touche H) : le récit sélectif de la partie, lecture seule.
	# Le clic sur une entrée localisée centre la carte (même motif que les alertes).
	_chronique = load("res://ui/chronique.gd").new()
	_chronique.name = "Chronique"
	_chronique.visible = false
	ui.add_child(_chronique)
	_chronique.goto_region.connect(goto_fn)

	# LA PAGE QUI SE TOURNE : CanvasLayer INDÉPENDANT (layer 60, au-dessus de `ui`) — le
	# codex qui referme un âge. Ajouté à la racine (pas dans `ui`) pour ne jamais hériter
	# du thème/anchors de la couche panneau ; son propre layer le place au-dessus de tout.
	_page_turn = load("res://ui/page_turn.gd").new()
	_page_turn.name = "PageTurn"
	add_child(_page_turn)

	# ÉCRAN DE CHAPITRE : le chip d'âge n'engage plus directement — il ouvre CE récap
	# (monde en pause, tranche d'annales de l'âge écoulé, bilan) ; le verbe s'émet là.
	_age_recap = load("res://ui/age_recap.gd").new()
	_age_recap.name = "AgeRecap"
	ui.add_child(_age_recap)
	_age_recap.set_page(_page_turn)
	_age_recap.goto_region.connect(goto_fn)
	alerts.age_recap_requested.connect(func(): _age_recap.open())

	# ÉPILOGUE : à la PREMIÈRE fin signalée par endgame_info (apocalypse 1-3 ou
	# ascension 4), l'écran « votre règne en une phrase » s'ouvre — une seule fois.
	_epilogue = load("res://ui/epilogue.gd").new()
	_epilogue.name = "Epilogue"
	ui.add_child(_epilogue)
	_epilogue.goto_region.connect(goto_fn)
	Sim.ticked.connect(_on_tick_endgame)
	Sim.generated.connect(func(): _epilogue_shown = false)

	# MEMBRANE DE DÉCISION : un évènement à VRAIE décision (Marbrive…) qui concerne le
	# joueur ATTEND son choix — distinct du popup OYEZ OYEZ (notification après coup) :
	# ici RIEN n'est encore appliqué tant que le joueur n'a pas choisi.
	var event_dialog = load("res://ui/event_dialog.gd").new()
	event_dialog.name = "EventDialog"
	ui.add_child(event_dialog)

	# RETOUR JOUEUR : bouton « Signaler un bug » (toujours visible en jeu) + détection de
	# crash au redémarrage + export LOCAL d'un rapport (remarque · log · screenshot ·
	# contexte). CanvasLayer propre à la RACINE (layer 80, au-dessus de tout, menu compris).
	var feedback = load("res://ui/feedback.gd").new()
	feedback.name = "Feedback"
	add_child(feedback)

	# ⚠ THÈME : la propagation s'arrête au CanvasLayer (ni Control ni Window) — le thème
	# de la fenêtre n'atteint JAMAIS les panneaux de la couche UI tout seul. On le pose
	# donc sur chaque Control de premier niveau QUI N'A PAS LE SIEN : les panneaux du
	# squelette parchemin (province_v2, empire, budget_v2, armée…) posent ParchTheme.build()
	# dans leur _ready — l'écraser ici leur retirait les variations (HeaderStrip, Tab…)
	# en jeu réel, invisible aux probes qui contournent main.gd (piège signalé par D1).
	for c in ui.get_children():
		if c is Control and c.theme == null:
			c.theme = get_window().theme

	# ENRÔLEMENT « draggable » : chaque panneau flottant devient déplaçable par son
	# bandeau-titre (cf. _input). Les vars LOCALES army_panel/esb sont encore en scope.
	for p in [_country_panel, _construct, _battle_panel, _tech, _econ,
			_prov_detail, _country_actions, _religion, _codex, _memory_panel,
			_search_palette, _devpanel, army_panel, esb]:
		if p != null:
			p.add_to_group("draggable")

	Sim.set_speed(0)            # monde en pause tant que le menu est ouvert

## ESPACE = pause, intercepté EN AMONT du focus GUI (_input passe avant les boutons
## focusés) — le focus clavier reste VIVANT partout (Tab/Entrée, audit 2026-07-10) ;
## on ne vole la barre d'espace qu'aux boutons, jamais à un champ de saisie.
func _input(e: InputEvent) -> void:
	# --- GLISSER-DÉPOSER des panneaux flottants (par le bandeau-titre) ---
	if e is InputEventMouseButton and e.button_index == MOUSE_BUTTON_LEFT:
		if e.pressed:
			var c := get_viewport().gui_get_hovered_control()
			var p := _drag_top(c)
			# press dans la BANDE d'en-tête seulement → on arme le glissé (sans consommer
			# l'event : le panneau reçoit quand même le press, ses boutons d'en-tête tirent)
			if p != null and p.visible and e.position.y >= p.position.y and e.position.y <= p.position.y + DRAG_HEADER_H:
				_drag_panel = p
				_drag_press = e.position
				_drag_off = e.position - p.position
				_dragging = false
		else:
			_drag_panel = null
			_dragging = false
		return
	if e is InputEventMouseMotion and _drag_panel != null and (e.button_mask & MOUSE_BUTTON_MASK_LEFT):
		if not is_instance_valid(_drag_panel):
			_drag_panel = null
			_dragging = false
			return
		if not _dragging and e.position.distance_to(_drag_press) > 6.0:
			_dragging = true
		if _dragging:
			var vp := get_viewport().get_visible_rect().size
			var np: Vector2 = e.position - _drag_off
			# garder ≥ 60 px du panneau à l'écran, haut ≥ 0
			np.x = clamp(np.x, 60.0 - _drag_panel.size.x, vp.x - 60.0)
			np.y = clamp(np.y, 0.0, vp.y - 60.0)
			_drag_panel.position = np
			_drag_panel.queue_redraw()
			get_viewport().set_input_as_handled()   # ni la carte ni le panneau ne réagissent au glissé
		return
	if e is InputEventKey and e.pressed and not e.echo and e.ctrl_pressed and e.keycode == KEY_M:
		if _memory_panel != null and Sim.game_on:
			if _memory_panel.visible:
				_memory_panel.close_panel()
			else:
				_memory_panel.open_panel()
			get_viewport().set_input_as_handled()
		return
	if e is InputEventKey and e.pressed and not e.echo and e.ctrl_pressed and e.keycode == KEY_K:
		if _search_palette != null and Sim.game_on:
			if _search_palette.visible:
				_search_palette.close_palette()
			else:
				_search_palette.open_palette()
			get_viewport().set_input_as_handled()
		return
	if not (e is InputEventKey and e.pressed and not e.echo and e.keycode == KEY_SPACE):
		return
	var fo := get_viewport().gui_get_focus_owner()
	if fo is LineEdit or fo is TextEdit:
		return                       # on tape un espace dans un champ : pas de pause
	Sim.toggle_pause()
	get_viewport().set_input_as_handled()

## remonte de `c` vers le premier ancêtre (ou lui-même) du groupe « draggable ».
func _drag_top(c: Node) -> Control:
	while c != null:
		if c is Control and c.is_in_group("draggable"):
			return c
		c = c.get_parent()
	return null

func _unhandled_input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo):
		return
	var fo := get_viewport().gui_get_focus_owner()
	if e.alt_pressed and not (fo is LineEdit or fo is TextEdit):
		if e.keycode == KEY_LEFT and _nav != null:
			_nav.back()
			get_viewport().set_input_as_handled()
			return
		if e.keycode == KEY_RIGHT and _nav != null:
			_nav.forward()
			get_viewport().set_input_as_handled()
			return
	match e.keycode:
		KEY_ESCAPE:
			# PILE DE FERMETURE : Échap ferme d'abord le panneau flottant visible (un par
			# pression), puis la sélection (panneau province/pays), et SEULEMENT ensuite
			# ouvre le menu — « tout panneau affiché doit pouvoir être dismiss ».
			if _menu != null and _menu.visible:
				pass                                   # le menu gère ses écrans (Retour)
			elif _close_topmost():
				pass
			elif _menu != null:
				_menu.open()
		KEY_F10:
			if _devpanel != null:
				_devpanel.visible = not _devpanel.visible
		# F1-F8 = les onglets du RAIL GAUCHE dans l'ordre de leur emplacement (retour
		# joueur 2026-07-10) : Économie · Démographie · Stocks · Marché · Armée ·
		# Filtres · Diplomatie · Conseil. Le CODEX (ex-F1) vit au menu Échap.
		KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8:
			if _sidebar != null and Sim.game_on:
				_sidebar.toggle_tab(e.keycode - KEY_F1)
		KEY_T:
			# ARBRE DE TECHNOLOGIE (le tech_panel documente « bascule touche T » mais le
			# raccourci n'était jamais câblé : le seul opener était la cellule Savoir de la
			# topbar — retirée par la refonte « topbar définitive ». On rétablit T comme
			# porte du savoir, via la MÊME route que le clic Savoir d'hier).
			if _tech != null and Sim.game_on:
				_tech.visible = not _tech.visible
				if _tech.visible:
					Sound.play("ui_parchment_open")
				_tech.queue_redraw()
		KEY_B:
			# PILOTE budget « grand livre parchemin » (coexiste avec l'éco existante)
			if _budget_v2 != null and Sim.game_on:
				_budget_v2.visible = not _budget_v2.visible
				if _budget_v2.visible and _budget_v2.has_method("refresh"):
					_budget_v2.refresh()
		KEY_V:
			# LA fiche province (D1-UNIFICATION) : bascule visibilité — sans province
			# sélectionnée, ouvre la première possédée par le joueur.
			if _prov_panel_v2 != null and Sim.game_on:
				if _prov_panel_v2.visible:
					_prov_panel_v2.hide()
				else:
					var pid := _sel_prov if _sel_prov >= 0 else _first_owned_province()
					_prov_panel_v2.show_province(pid)
		KEY_E:
			# FENÊTRE EMPIRE à onglets (Économie · Population · Diplomatie · Conseil)
			if _empire_win != null and Sim.game_on:
				if _empire_win.visible:
					_empire_win.hide()
					Sound.play("ui_parchment_close")
				else:
					_empire_win.open()
					Sound.play("ui_parchment_open")
		KEY_H:
			if _chronique != null:
				if _chronique.visible:
					_chronique.hide()
					Sound.play("ui_parchment_close")
				else:
					_chronique.open()
		KEY_EQUAL, KEY_PLUS, KEY_KP_ADD:
			Sim.faster()
		KEY_MINUS, KEY_KP_SUBTRACT:
			Sim.slower()

## DÉCLENCHEUR « créateur de foi » : à chaque pas, si le joueur a bâti son 1er édifice
## religieux et n'a pas encore de foi, on ouvre le créateur (monde en pause). Une seule fois.
## MODE OBSERVATEUR : aucune main humaine → on ne prompte pas le joueur pour des décisions
## d'un empire piloté par l'IA (l'IA les tranche elle-même).
## Porte publique de la navigation contextuelle. Les contrôles décrivent une cible ;
## Main reste seul responsable du panneau concret et des règles d'exclusivité.
func navigate_to(ref: Dictionary, surface: String = "", context: Dictionary = {}) -> bool:
	if _nav == null:
		return false
	return _nav.go(InfoRef.request(ref, surface, context))

## première province possédée par le joueur (repli quand aucune n'est sélectionnée) — sert
## d'ouverture par défaut au pilote fiche-province (touche V).
func _first_owned_province() -> int:
	var w = Sim.world
	if w == null or not w.has_method("province_count"):
		return -1
	var me: int = int(w.player()) if w.has_method("player") else 0
	for pid in range(int(w.province_count())):
		var info: Dictionary = w.province_info(pid)
		if bool(info.get("valide", false)) and int(info.get("owner", -1)) == me:
			return pid
	return 0

func _focus_region(region: int) -> bool:
	if region < 0 or Sim.world == null:
		return false
	var map = get_node_or_null("MapView")
	if map == null:
		return false
	var c: Vector2 = Sim.world.region_centroid(region)
	if c.x < 0:
		return false
	map._camera.position = map.iso_pos(c.x, c.y)
	map.queue_redraw()
	return true

## Résolution unique des destinations. Une route invalide échoue sans fermer la vue
## actuelle et sans reconstituer de donnée métier côté interface.
func _route_navigation(request: Dictionary) -> void:
	var ref = request.get("ref", {})
	if not (ref is Dictionary) or not InfoRef.is_valid(ref):
		return
	var kind := String(ref.get("kind", ""))
	var id = ref.get("id", -1)
	var context: Dictionary = request.get("context", {})
	var surface := String(request.get("surface", ""))
	var map = get_node_or_null("MapView")
	match kind:
		InfoRef.SIDEBAR_TAB:
			if _sidebar != null:
				_sidebar.open_tab(int(id), context)
		InfoRef.RESOURCE:
			if _sidebar != null:
				var tab := int(context.get("tab", 2))
				var focus := context.duplicate(true)
				if typeof(id) == TYPE_INT:
					focus["resource_id"] = int(id)
				else:
					focus["resource_name"] = String(id)
				_sidebar.open_tab(tab, focus)
		InfoRef.TECH:
			if _tech != null and Sim.game_on:
				var was_visible := _tech.visible
				_tech.visible = true
				if not was_visible:
					Sound.play("ui_parchment_open")
				if int(id) >= 0 and _tech.has_method("focus_tech"):
					_tech.focus_tech(int(id))
				_tech.queue_redraw()
		InfoRef.CODEX:
			if _codex != null:
				_codex.open_search(String(context.get("query", id)))
		InfoRef.COUNTRY:
			if Sim.world == null:
				return
			var cid := int(id)
			if surface == "actions" and _country_actions != null and cid != Sim.world.player():
				_country_actions.open_country(cid)
			else:
				_sel_owner = cid
				if _sidebar != null:
					_sidebar.close()
				if _country_panel != null:
					_country_panel.show_country(cid)
		InfoRef.PROVINCE:
			if Sim.world == null:
				return
			var pid := int(id)
			var pi: Dictionary = Sim.world.province_info(pid)
			if not bool(pi.get("valide", false)):
				return
			var region := int(Sim.world.province_region(pid))
			if map != null:
				map._selected_prov = pid
			_on_province_picked(pid, region, int(pi.get("owner", -1)))
			if surface == "map" or bool(context.get("focus_map", false)):
				_focus_region(region)
		InfoRef.REGION:
			_focus_region(int(id))
		InfoRef.CORPS:
			if map == null:
				return
			map._set_selected_corps([int(id)])
			if Sim.world != null and Sim.world.has_method("corps_info"):
				var army: Dictionary = Sim.world.corps_info(int(id))
				if bool(context.get("focus_map", true)) and bool(army.get("active", false)):
					_focus_region(int(army.get("region", -1)))
		InfoRef.MAP_MODE:
			if map != null:
				map.set_mode(int(id))
		InfoRef.MEMORY:
			if _memory_panel != null and Sim.game_on:
				_memory_panel.open_panel(int(context.get("tab", -1)))

func _observing() -> bool:
	return Sim.world != null and Sim.world.has_method("is_observer") and Sim.world.is_observer()

func _on_tick_faith(_year: int) -> void:
	if _faith_prompted or _religion == null or Sim.world == null or _observing():
		return
	if not Sim.world.has_method("religion_founding_ready"):
		return
	if int(Sim.world.religion_founding_ready(Sim.world.player())) == 1:
		_faith_prompted = true
		Sim.set_speed(0)             # le monde s'arrête : moment de fondation
		_religion.open()

## DÉCLENCHEUR « épilogue » : la TRANSITION vers une fin (§27 apocalypse ou ascension
## Merveille) ouvre l'écran d'épilogue — une seule fois par partie (latch UI).
func _on_tick_endgame(_year: int) -> void:
	if _epilogue_shown or _epilogue == null or Sim.world == null or not Sim.game_on:
		return
	var e: Dictionary = Sim.world.endgame_info()
	var fin: int = int(e.get("fin", 0))
	if fin > 0:
		_epilogue_shown = true
		_epilogue.open(fin)

## AUDIT UI 1.4 (« alertes vs fenêtres majeures ») : vrai si l'une des FENÊTRES MAJEURES
## de lecture/décision joueur est ouverte — le même sous-ensemble de `_close_topmost`
## (moins `_devpanel` outil de MOD et `_battle_panel` déjà son propre panneau de combat,
## non nommés par l'audit). alerts.gd lit ceci à CHAQUE frame (via un Callable, il n'a
## pas de référence à Main) pour masquer sa pile ordinaire derrière un compteur compact.
## UI-POLISH #13 (cartographie UI 284bc3b) : `_budget_v2`/`_empire_win`
## étaient ABSENTS d'ici aussi (même bug de non-enrôlement que `_close_topmost` — ajoutés
## après coup, jamais raccordés aux listes « panneau majeur » historiques) : le Trésor ou
## la fenêtre Diplomatie ouverts ne masquaient pas la pile d'alertes ordinaire derrière
## le compteur compact, contrairement aux autres écrans profonds.
## D1-UNIFICATION : `_prov_panel_v2` (LA fiche province) n'est PAS un panneau majeur — elle
## s'ouvre au moindre clic de province, une interaction ROUTINIÈRE, pas un écran profond ;
## la classer « majeure » aurait fait collapse la pile d'alertes en continu (retiré ici).
func major_open() -> bool:
	for p in [_memory_panel, _search_palette, _tech, _econ, _codex, _construct, _prov_detail, _country_actions,
			_chronique, _age_recap, _epilogue, _religion, _budget_v2, _empire_win]:
		if p != null and p.visible:
			return true
	return false

## ferme le PANNEAU FLOTTANT visible le plus haut (un par pression d'Échap), puis la
## sélection. true = quelque chose a été fermé (Échap consommé avant le menu).
## UI-POLISH #13 : Échap ferme d'abord le DERNIER panneau OUVERT (Trésor/Diplomatie/
## Construction — pile `_panel_stack`, cf. leur wiring `visibility_changed`), puis
## retombe sur l'ordre historique fixe pour tout le reste (aucun de ces panneaux N'A
## de notion d'ordre d'ouverture, un ordre arbitraire raisonnable suffit). `_budget_v2`/
## `_empire_win` étaient ABSENTS de cette liste (bug historique : Échap
## ne les fermait jamais, sautait direct au menu PAR-DESSUS eux) — ajoutés en repli.
## D1-UNIFICATION : `_prov_panel_v2` (LA fiche province, contextuelle-ancrée) N'EST PAS
## dans cette liste générique — comme la fiche legacy avant elle, elle se ferme en UN
## SEUL Échap via `_clear_selection()` ci-dessous (pleine désélection : panneau ET
## contour doré de la carte), pas via un hide() sec qui laisserait la sélection en l'air.
func _close_topmost() -> bool:
	while not _panel_stack.is_empty():
		var top: Control = _panel_stack[_panel_stack.size() - 1]
		_panel_stack.remove_at(_panel_stack.size() - 1)
		if top != null and is_instance_valid(top) and top.visible:
			top.visible = false
			Sound.play("ui_parchment_close")
			return true
	for p in [_memory_panel, _search_palette, _construct, _tech, _econ, _religion, _prov_detail,
			_devpanel, _country_actions, _chronique, _age_recap, _epilogue, _battle_panel, _codex,
			_budget_v2, _empire_win]:
		if p != null and p.visible:
			p.visible = false
			Sound.play("ui_parchment_close")
			return true
	if (_prov_panel_v2 != null and _prov_panel_v2.visible) or (_country_panel != null and _country_panel.visible):
		_clear_selection()
		return true
	return false

## CURSEUR PLUME (planche 28) : la pièce a la pointe en bas-droite → rotation 180°
## pour poser le bec en HAUT-GAUCHE (hotspot 2,2). Absente → curseur système.
func _setup_cursor() -> void:
	var path := "res://assets/scps/ui/parch/sheet28_end_rituals_loading_cursors_09.png"
	if not UIKit.has(path):
		return
	var img := UIKit.load_img(path)
	if img == null:
		return
	var used := img.get_used_rect()
	if used.size.x < 4:
		return
	img = img.get_region(used)
	img.rotate_180()   # bec en HAUT-GAUCHE → le curseur POINTE en haut-gauche (convention)
	var h := 38
	var wpx := int(round(float(img.get_width()) * float(h) / float(img.get_height())))
	img.resize(wpx, h, Image.INTERPOLATE_LANCZOS)
	Input.set_custom_mouse_cursor(ImageTexture.create_from_image(img),
		Input.CURSOR_ARROW, Vector2(2, 2))

## désélection PLEINE : panneaux de sélection refermés + le contour doré s'éteint.
func _clear_selection() -> void:
	_sel_prov = -1
	_sel_owner = -1
	if _prov_panel_v2 != null:
		_prov_panel_v2.show_province(-1)
	if _country_panel != null:
		_country_panel.show_country(-1)
	var map := get_node_or_null("MapView")
	if map != null:
		map._selected_prov = -1
		var ov := map.get_node_or_null("Overlay")
		if ov != null:
			ov.queue_redraw()

func _on_province_picked(province: int, region: int, owner: int) -> void:
	if Sim.world == null:
		return
	if province < 0:
		_prov_panel_v2.show_province(-1)         # clic en mer → on referme
		_country_panel.show_country(-1)
		return
	if _sidebar != null:
		_sidebar.close()                      # un clic province referme le tiroir (exclusifs)
	_sel_prov = province                      # mémorisé pour le détail (touche V)
	_sel_owner = owner                        # mémorisé pour restaurer CountryPanel à la fermeture d'un écran profond
	# ZONE CONTEXTUELLE UNIQUE (UI-3) : un écran profond déjà ouvert garde la main —
	# on ne réaffiche pas le panneau qu'il a remplacé par-dessus lui.
	if _prov_detail == null or not _prov_detail.visible:
		_prov_panel_v2.show_province(province)
	if _country_actions == null or not _country_actions.visible:
		_country_panel.show_country(owner)        # -1 (terre libre) → panneau caché
	if _prov_detail != null and _prov_detail.visible:
		_prov_detail.show_province(province)  # détail ouvert → suit la sélection

	# W-GUERRE UI (lot B) : la région cliquée porte-t-elle un COMBAT (siège/bataille) ?
	# Le jeton d'armée (overlay.gd) est planté au centroïde de région — cliquer dessus
	# résout à une province de CETTE région, d'où ce test après la résolution normale.
	if _battle_panel != null and region >= 0 and Sim.world.has_method("battle_info"):
		var bi: Dictionary = Sim.world.battle_info(region)
		if bool(bi.get("valid", false)):
			_battle_panel.open_region(region)
