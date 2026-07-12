extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène et
## relier la sélection de carte aux panneaux de lecture (la membrane → UI).

const Frame = preload("res://ui/frame.gd")
const UIKit = preload("res://ui/uikit.gd")   # load_img() export-safe (curseur, etc.)

var _prov_panel: Control
var _country_panel: Control
var _sidebar: Control
var _construct: Control
var _tech: Control
var _econ: Control
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
var _faith_prompted := false   # le créateur de foi ne s'ouvre qu'UNE fois (1er édifice religieux)
var _epilogue_shown := false   # l'épilogue ne s'ouvre qu'UNE fois par partie (latch UI)
var _sel_prov := -1
var _sel_owner := -1           # dernier propriétaire vu (restaure CountryPanel à la fermeture d'un écran profond)

func _ready() -> void:
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
	tts.add_child(load("res://ui/tooltip_server.gd").new())
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
	topbar.tech_requested.connect(func():
		_tech.visible = not _tech.visible
		if _tech.visible:
			Sound.play("ui_parchment_open")
		_tech.queue_redraw())

	_prov_panel = load("res://ui/province_panel.gd").new()
	_prov_panel.name = "ProvincePanel"
	ui.add_child(_prov_panel)
	_prov_panel.close_requested.connect(_clear_selection)   # ✕ = désélection pleine
	_prov_panel.detail_requested.connect(func():
		if _prov_detail != null and _sel_prov >= 0:
			_prov_detail.show_province(_sel_prov)            # le DÉTAIL (main-d'œuvre & cie) s'ouvre enfin
			_prov_detail.visible = true
			Sound.play("ui_parchment_open")
			_prov_detail.queue_redraw())

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
	_sidebar.tab_selected.connect(func(i): if i >= 0: _prov_panel.show_province(-1))

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

	# Construction depuis le panneau province → toggle du panneau de construction
	# (target_pid = la province visée : les MANUFACTURES s'y élèvent — lot 5)
	_prov_panel.build_requested.connect(func():
		_construct.target_pid = _sel_prov
		_construct.visible = not _construct.visible
		if _construct.visible:
			Sound.play("ui_parchment_open")
		_construct.queue_redraw())
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
	map.country_context.connect(func(owner):
		if Sim.game_on and owner != Sim.world.player():
			_country_actions.open_country(owner))
	_sidebar.open_country.connect(func(cid):
		_sidebar.close()
		_country_actions.open_country(cid))

	# ZONE CONTEXTUELLE UNIQUE (retour joueur 2026-07-10, UI-3) : un écran profond
	# REMPLACE le panneau contextuel qu'il détaille, il ne s'y ajoute jamais — le regard
	# reste sur 3-4 zones, pas 5. Hooké sur le SIGNAL (visibility_changed) plutôt que sur
	# chaque site d'ouverture : couvre la pile Échap (_close_topmost), les ouvertures ET
	# la probe shot_ui (qui pose `.visible` en direct, hors des signaux dédiés).
	_prov_detail.visibility_changed.connect(func():
		if _prov_detail.visible:
			_prov_panel.visible = false             # le détail REMPLACE le panneau province
		elif _sel_prov >= 0:
			_prov_panel.show_province(_sel_prov))    # fermeture du détail → le panneau REVIENT
	_country_actions.visibility_changed.connect(func():
		if _country_actions.visible:
			_country_panel.visible = false           # la fenêtre diplo REMPLACE le panneau pays
		elif _sel_owner >= 0:
			_country_panel.show_country(_sel_owner))

	# ARMÉE : le pion sélectionné ouvre sa barre de COMMANDEMENT (posture/recompléter/piller/
	# dissoudre) ; le clic-destination sur la carte donne l'ordre de marche/attaque.
	var army_panel: Control = load("res://ui/army_panel.gd").new()
	ui.add_child(army_panel)
	map.army_selection_changed.connect(army_panel.set_army)
	army_panel.raid_requested.connect(func(): map.arm_raid())

	# la carte SÉLECTIONNE → on remplit les panneaux (lecture seule de la membrane)
	map.province_picked.connect(_on_province_picked)

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

	# ALERTES (façon EU4/CK3) : la pile des « éléments en attente » au bord droit —
	# code couleur par domaine, clic = le panneau concerné (ou le geste direct).
	var alerts = load("res://ui/alerts.gd").new()
	alerts.name = "Alerts"
	ui.add_child(alerts)
	# AUDIT UI 1.4 : alerts n'a pas de référence à Main → un Callable lu chaque frame
	# (major_open() n'existe qu'ICI, sur Main, où vivent tous les panneaux majeurs).
	alerts.major_open_fn = Callable(self, "major_open")
	alerts.open_tab.connect(func(i): _sidebar.open_tab(i))
	alerts.open_tech.connect(func():
		if not _tech.visible:
			Sound.play("ui_parchment_open")
		_tech.visible = true
		_tech.queue_redraw())
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
		if not _tech.visible:
			Sound.play("ui_parchment_open")
		_tech.visible = true
		_tech.queue_redraw())
	var goto_fn := func(r):
		if r >= 0 and Sim.world != null:
			var c: Vector2 = Sim.world.region_centroid(r)
			if c.x >= 0:
				map._camera.position = map.iso_pos(c.x, c.y)   # centre la carte sur l'alerte
				map.queue_redraw()
	alerts.goto_region.connect(goto_fn)

	# OYEZ OYEZ : le popup d'évènement (directeur + alertes majeures) — PAUSE + boutons
	# adaptatifs ; les kinds majeurs du fil y sont ROUTÉS par alerts (popup_requested).
	var popup = load("res://ui/event_popup.gd").new()
	popup.name = "EventPopup"
	ui.add_child(popup)
	alerts.popup_requested.connect(popup.enqueue)
	popup.goto_region.connect(goto_fn)
	popup.open_tab.connect(func(i): _sidebar.open_tab(i))

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
	# donc sur CHAQUE Control de premier niveau (leurs enfants en héritent normalement).
	for c in ui.get_children():
		if c is Control:
			c.theme = get_window().theme

	Sim.set_speed(0)            # monde en pause tant que le menu est ouvert

## ESPACE = pause, intercepté EN AMONT du focus GUI (_input passe avant les boutons
## focusés) — le focus clavier reste VIVANT partout (Tab/Entrée, audit 2026-07-10) ;
## on ne vole la barre d'espace qu'aux boutons, jamais à un champ de saisie.
func _input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo and e.keycode == KEY_SPACE):
		return
	var fo := get_viewport().gui_get_focus_owner()
	if fo is LineEdit or fo is TextEdit:
		return                       # on tape un espace dans un champ : pas de pause
	Sim.toggle_pause()
	get_viewport().set_input_as_handled()

func _unhandled_input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo):
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
func _on_tick_faith(_year: int) -> void:
	if _faith_prompted or _religion == null or Sim.world == null:
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
func major_open() -> bool:
	for p in [_tech, _econ, _codex, _construct, _prov_detail, _country_actions,
			_chronique, _age_recap, _epilogue, _religion]:
		if p != null and p.visible:
			return true
	return false

## ferme le PANNEAU FLOTTANT visible le plus haut (un par pression d'Échap), puis la
## sélection. true = quelque chose a été fermé (Échap consommé avant le menu).
func _close_topmost() -> bool:
	for p in [_construct, _tech, _econ, _religion, _prov_detail, _devpanel, _country_actions, _chronique, _age_recap, _epilogue, _battle_panel, _codex]:
		if p != null and p.visible:
			p.visible = false
			Sound.play("ui_parchment_close")
			return true
	if (_prov_panel != null and _prov_panel.visible) or (_country_panel != null and _country_panel.visible):
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
	if _prov_panel != null:
		_prov_panel.show_province(-1)
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
		_prov_panel.show_province(-1)         # clic en mer → on referme
		_country_panel.show_country(-1)
		return
	if _sidebar != null:
		_sidebar.close()                      # un clic province referme le tiroir (exclusifs)
	_sel_prov = province                      # mémorisé pour le détail (touche V)
	_sel_owner = owner                        # mémorisé pour restaurer CountryPanel à la fermeture d'un écran profond
	# ZONE CONTEXTUELLE UNIQUE (UI-3) : un écran profond déjà ouvert garde la main —
	# on ne réaffiche pas le panneau qu'il a remplacé par-dessus lui.
	if _prov_detail == null or not _prov_detail.visible:
		_prov_panel.show_province(province)
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
