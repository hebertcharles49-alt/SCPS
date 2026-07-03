extends Node
## plateau_audit — probe HÉRALDIQUE & PIONS (série 4). (1) compose les ARMES des
## pays vivants en une planche-contact (heraldry_contact.png) ; (2) cadre la carte
## sur une ARMÉE en campagne et capture le PION de plateau (plateau_map.png).
## Fenêtré : Godot --path godot/project res://plateau_audit.tscn
var _map: Node2D = null

func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	_map = load("res://map/map_view.gd").new()
	_map.name = "MapView"
	add_child(_map)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	Sim.regenerate(9)
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("no world")
		get_tree().quit(1)
		return
	var w = Sim.world
	# avance jusqu'à trouver une ARMÉE active (guerres ≈ dès la 1re décennie)
	var army_c := -1
	var army_reg := -1
	for burst in range(20):
		for i in range(5):
			w.advance_days(360)
		for c in range(w.country_count()):
			var a: Dictionary = w.army_info(c)
			if bool(a.get("active", false)) and int(a.get("region", -1)) >= 0:
				army_c = c
				army_reg = int(a.get("region", -1))
				break
		if army_c >= 0:
			break
	print("ARMY country=", army_c, " region=", army_reg, " an=", w.year())
	Sim.generated.emit()
	for i in range(6):
		await get_tree().process_frame

	# ── (1) planche-contact d'armoiries : 8 pays vivants (dont cité-état + wild si présents) ──
	var Her = load("res://ui/heraldry.gd")
	var picks := []
	var want_roles := [2, 4]                    # au moins une cité-état + un hameau
	for c in range(w.country_count()):
		if picks.size() >= 8:
			break
		var role := int(w.country_role(c))
		var a2: Dictionary = w.country_info(c)
		if not bool(a2.get("valide", false)):
			continue
		if role in want_roles:
			want_roles.erase(role)
			picks.append(c)
		elif picks.size() < 6:
			picks.append(c)
	var contact := Image.create(4 * 140, 2 * 150, false, Image.FORMAT_RGBA8)
	contact.fill(Color(0.91, 0.86, 0.72))
	for i in range(picks.size()):
		var t: Texture2D = Her.arms(picks[i])
		if t == null:
			continue
		var img := t.get_image()
		img.resize(128, 128, Image.INTERPOLATE_LANCZOS)
		contact.blend_rect(img, Rect2i(0, 0, 128, 128),
			Vector2i((i % 4) * 140 + 6, (i / 4) * 150 + 8))
	contact.save_png("res://heraldry_contact.png")
	print("SAVED heraldry_contact.png (", picks.size(), " pays)")

	# ── (2) la carte cadrée sur l'armée : le pion d'étain posé sur la table ──
	if army_reg >= 0:
		var cc: Vector2 = w.region_centroid(army_reg)
		var ov = _map.get_node_or_null("Overlay")
		for i in range(10):
			_map._camera.zoom = Vector2(7.0, 7.0)
			_map._camera.position = _map.iso_pos(cc.x, cc.y)
			if ov != null:
				ov.queue_redraw()
			_map.queue_redraw()
			await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		_map.get_viewport().get_texture().get_image().save_png("res://plateau_map.png")
		print("SAVED plateau_map.png")
	get_tree().quit(0)
