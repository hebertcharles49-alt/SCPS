extends Node
## Main — compose le front : la CARTE (monde) + l'UI (CanvasLayer au-dessus).
## Le moteur vit dans l'autoload `Sim` ; ici on ne fait qu'assembler la scène et
## relier la sélection de carte aux panneaux de lecture (la membrane → UI).

const Frame = preload("res://ui/frame.gd")

var _prov_panel: Control
var _country_panel: Control
var _sidebar: Control
var _construct: Control
var _tech: Control
var _econ: Control
var _prov_detail: Control
var _menu: Control
var _religion: Control
var _devpanel: Control         # MODTOOLS : panneau dev (tunables live, F10)
var _faith_prompted := false   # le créateur de foi ne s'ouvre qu'UNE fois (1er édifice religieux)
var _sel_prov := -1

func _ready() -> void:
	# la carte (Node2D, caméra dedans)
	var map_script := load("res://map/map_view.gd")
	var map: Node2D = map_script.new()
	map.name = "MapView"
	add_child(map)

	# l'UI, sur une couche écran (au-dessus de la carte, indépendante de la caméra)
	var ui := CanvasLayer.new()
	ui.name = "UI"
	add_child(ui)

	var topbar_script := load("res://ui/topbar.gd")
	var topbar: Control = topbar_script.new()
	topbar.name = "Topbar"
	ui.add_child(topbar)
	topbar.tech_requested.connect(func():
		_tech.visible = not _tech.visible
		_tech.queue_redraw())

	_prov_panel = load("res://ui/province_panel.gd").new()
	_prov_panel.name = "ProvincePanel"
	ui.add_child(_prov_panel)
	_prov_panel.close_requested.connect(_clear_selection)   # ✕ = désélection pleine
	_prov_panel.detail_requested.connect(func():
		if _prov_detail != null and _sel_prov >= 0:
			_prov_detail.show_province(_sel_prov)            # le DÉTAIL (main-d'œuvre & cie) s'ouvre enfin
			_prov_detail.visible = true
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
	_construct.position = Vector2(Frame.SIDEBAR_W + 8, Frame.TOPBAR_H + 8)
	_construct.visible = false
	ui.add_child(_construct)

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
	_prov_panel.build_requested.connect(func():
		_construct.visible = not _construct.visible
		_construct.queue_redraw())

	# la carte SÉLECTIONNE → on remplit les panneaux (lecture seule de la membrane)
	map.province_picked.connect(_on_province_picked)

	# MENU PRINCIPAL (Jouer/Charger/Options/Quitter) par-dessus la carte, au démarrage.
	# Topmost (ajouté en dernier). Le monde par défaut est déjà généré derrière ; « Lancer
	# la partie » le régénère selon le setup (sliders + cultures) puis laisse en PAUSE an 0.
	_menu = load("res://ui/menu_root.gd").new()
	_menu.name = "MenuRoot"
	ui.add_child(_menu)

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

	Sim.set_speed(0)            # monde en pause tant que le menu est ouvert

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
		KEY_SPACE:
			Sim.toggle_pause()
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

## ferme le PANNEAU FLOTTANT visible le plus haut (un par pression d'Échap), puis la
## sélection. true = quelque chose a été fermé (Échap consommé avant le menu).
func _close_topmost() -> bool:
	for p in [_construct, _tech, _econ, _religion, _prov_detail, _devpanel]:
		if p != null and p.visible:
			p.visible = false
			return true
	if (_prov_panel != null and _prov_panel.visible) or (_country_panel != null and _country_panel.visible):
		_clear_selection()
		return true
	return false

## désélection PLEINE : panneaux de sélection refermés + le contour doré s'éteint.
func _clear_selection() -> void:
	_sel_prov = -1
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

func _on_province_picked(province: int, _region: int, owner: int) -> void:
	if Sim.world == null:
		return
	if province < 0:
		_prov_panel.show_province(-1)         # clic en mer → on referme
		_country_panel.show_country(-1)
		return
	if _sidebar != null:
		_sidebar.close()                      # un clic province referme le tiroir (exclusifs)
	_sel_prov = province                      # mémorisé pour le détail (touche V)
	_prov_panel.show_province(province)
	_country_panel.show_country(owner)        # -1 (terre libre) → panneau caché
	if _prov_detail != null and _prov_detail.visible:
		_prov_detail.show_province(province)  # détail ouvert → suit la sélection
