extends Node
## map_modes_shot — probe visuelle CHANTIER C/E (4 MODES CARTE) + EXTENSION RELIGION/
## CULTURE (2026-08-19, même jour) : un PNG par mode (Défaut/Politique/Nature/Marché/
## Religion/Culture, 6 au total), + un plan rapproché du mode Marché pour vérifier les
## icônes de brutes sur tuile. Motif de map_art_shot.gd (mêmes primitives caméra/
## capture). Le switcheur (controls.gd) n'est PAS piloté ici — on appelle directement
## map.set_mode()/set_nature() (la même API que controls.gd, sans passer par le clic).
##   Godot --audio-driver Dummy --path godot/project res://map_modes_shot.tscn -- seed=205

const MapView = preload("res://map/map_view.gd")

var _main: Node = null
var _dir := "res://shots_modes/"

func _arg(p: String, d: String) -> String:
	for a in OS.get_cmdline_user_args():
		if a.begins_with(p):
			return a.substr(p.length())
	return d

func _ready() -> void:
	get_window().size = Vector2i(1280, 720)
	get_window().unfocusable = true
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	_run.call_deferred()

func _shot(nom: String) -> void:
	for i in range(6):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(ProjectSettings.globalize_path(_dir + nom + ".png"))
	print("SHOT ", nom)

func _cam(map: Node, wx: float, wy: float, z: float) -> void:
	map._camera.zoom = Vector2(z, z)
	map._camera.position = map.iso_pos(wx, wy)
	map._nav_redraw()

func _run() -> void:
	_main = load("res://main/Main.tscn").instantiate()
	add_child(_main)
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(int(_arg("seed=", "205")))
	for i in range(30):
		await get_tree().process_frame
	if Sim.world == null:
		push_error("no world"); get_tree().quit(1); return
	# 80 ans : le temps que des Marchés/Comptoirs/Centres commerciaux se bâtissent
	# (sinon AUCUN centre de bassin n'existe encore — la teinte serait toute -1/grise) ;
	# et qu'une religion se fonde quelque part (rare : gate Temple T2+, cap ⌈N/2⌉).
	Sim.world.advance_days(360 * 80)
	Sim.generated.emit()
	var menu: Control = _main._menu
	if menu != null:
		menu.hide()
	Sim.game_on = true
	Sim.speed_index = 0
	var map: Node = _main.get_node_or_null("MapView")
	var w = Sim.world
	for i in range(45):   # sol/fog/lavis se construisent sur les premières frames
		await get_tree().process_frame

	var cap_reg := int(w.country_capital_region(int(w.player())))
	var cx := float(w.map_w()) * 0.5
	var cy := float(w.map_h()) * 0.5
	if cap_reg >= 0:
		var ctr: Vector2 = w.region_centroid(cap_reg)
		if ctr.x >= 0:
			cx = ctr.x; cy = ctr.y

	# 1) DÉFAUT (mode 0) — plan large : terrain + lavis politique léger + tout le chrome.
	map.set_nature(false)
	map.set_mode(MapView.MODE_DEFAUT)
	_cam(map, cx, cy, 0.9)
	await _shot("01_defaut")

	# 2) POLITIQUE (mode 1) — même plan : pays par couleur pleine (political_image).
	map.set_mode(MapView.MODE_POLITIQUE)
	_cam(map, cx, cy, 0.9)
	await _shot("02_politique")

	# 3) NATURE (le nature_mode existant, promu en mode exclusif) — terrain + dressing SEULS.
	map.set_nature(true)
	map.set_mode(MapView.MODE_NATURE)
	_cam(map, cx, cy, 3.5)
	await _shot("03_nature")
	map.set_nature(false)

	# 4) MARCHÉ (mode 21) — d'ABORD un plan LARGE : on doit voir des BASSINS (poignée de
	#    couleurs, plusieurs provinces chacune) — si chaque province a sa propre couleur,
	#    le catchment est cassé (cf. brief §5.2).
	map.set_mode(MapView.MODE_MARCHE)
	_cam(map, cx, cy, 0.9)
	await _shot("04_marche_large")
	# … puis un plan RAPPROCHÉ (zoom ≥ RAW_ICON_ZOOM_MIN=6.0, overlay.gd) : les icônes de
	#    brutes DOIVENT apparaître au centre de chaque province, côte à côte, sur leur
	#    socle parchemin — SANS mur de cartouches qui se chevauchent (retour probe 1).
	_cam(map, cx, cy, 7.0)
	await _shot("05_marche_tuiles")

	# 5)/6) RELIGION (22) / CULTURE (23, EXTENSION 2026-08-19) : religion est RARE (gate
	#    Temple T2+, cap ⌈N/2⌉ des empires) — un plan centré capitale sous fog risque de
	#    ne montrer AUCUNE foi posée. Fog LEVÉ pour ces deux shots (motif shot_parch/
	#    map_art_shot --at=, ov.fog_off) + cadrage MONDE ENTIER (map.fit()) pour maximiser
	#    la chance d'attraper un foyer de foi/une frontière culturelle dans le cadre.
	var ov = map.get_node_or_null("Overlay")
	if ov != null:
		ov.fog_off = true
		ov.queue_redraw()
	map.set_mode(MapView.MODE_RELIGION)
	map.fit()
	await _shot("06_religion")
	map.set_mode(MapView.MODE_CULTURE)
	map.fit()
	await _shot("07_culture")
	if ov != null:
		ov.fog_off = false
		ov.queue_redraw()

	print("MAP MODES SHOTS OK — ", _dir)
	get_tree().quit()
