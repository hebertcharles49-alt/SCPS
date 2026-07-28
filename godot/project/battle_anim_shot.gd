extends Node
## battle_anim_shot — preuve visuelle du tableau vivant du choc (battle_anim.gd), en
## SYNTHÉTIQUE (le widget est display-only : on le nourrit d'un battle_info fabriqué,
## pas besoin de monde ni de guerre vive). 4 moments : formations · choc (étincelles) ·
## après-pertes · renfort qui entre par le bord.
##   Godot --path godot/project res://battle_anim_shot.tscn

var _dir := "res://shots_battle/"

func _ready() -> void:
	get_window().size = Vector2i(420, 200)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_dir))
	var bgp := ColorRect.new()
	bgp.color = Color(0xda / 255.0, 0xc4 / 255.0, 0x8f / 255.0)   # parchemin sépia autour
	bgp.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(bgp)
	var anim: Control = preload("res://ui/battle_anim.gd").new()
	anim.position = Vector2(120, 120)
	anim.scale = Vector2(4, 4)   # le viewport projet est 1080p : ×4 pour lire les formes
	add_child(anim)
	_run.call_deferred(anim)

func _run(anim: Control) -> void:
	# atk = petit corps (300 hommes = 3 régiments purs infanterie ⇒ 3 carrés, 1 colonne) ;
	# def = gros corps (3000 hommes = 30 régiments ⇒ pile la limite MAX_SHAPES, aucune
	# troncature). Désert/Aride pour bien voir l'aplat de terrain (vs l'ex-image biome).
	var bi := {
		"region": 42, "units_are_humans": false, "in_battle": true, "chocs": 0,
		"relief": "Désert", "climat": "Aride",
		"atk_inf": 3, "atk_arch": 0, "atk_cav": 0, "atk_mages": 0, "atk_units": 3,
		"def_inf": 30, "def_arch": 0, "def_cav": 0, "def_mages": 0, "def_units": 30,
		"atk_morale_pct": 100, "def_morale_pct": 100, "loss_atk": 0.0, "loss_def": 0.0,
	}
	anim.setup(bi)
	await _wait(0.4)
	await _shot("01_formations")
	bi["chocs"] = 1; bi["loss_atk"] = 1.0; bi["loss_def"] = 4.0
	bi["atk_units"] = 2; bi["def_units"] = 26
	bi["atk_morale_pct"] = 80; bi["def_morale_pct"] = 60
	anim.on_tick(bi)
	await _wait(0.72)
	await _shot("02_choc")
	await _wait(1.3)
	await _shot("03_apres")
	bi["atk_units"] = 8   # renfort côté atk : +6 régiments, aucune perte nouvelle
	anim.on_tick(bi)
	await _wait(0.35)
	await _shot("04_renfort")
	print("BATTLE ANIM SHOTS OK — ", _dir)
	get_tree().quit()

func _wait(s: float) -> void:
	var t := 0.0
	while t < s:
		t += get_process_delta_time()
		await get_tree().process_frame

func _shot(nom: String) -> void:
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(ProjectSettings.globalize_path(_dir + nom + ".png"))
	print("SHOT ", nom)
