extends Node
## micro_herald — compose les armes en HEADLESS (Image seule, aucun rendu) pour
## itérer la composition sans fenêtre.
## Godot --headless --path godot/project res://micro_herald.tscn
func _ready() -> void:
	print("HERALDRY MICRO v4")
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	Sim.regenerate(9)
	await get_tree().process_frame
	if Sim.world == null:
		print("NO WORLD")
		get_tree().quit(1)
		return
	var t0 := Time.get_ticks_msec()
	var Her = load("res://ui/heraldry.gd")
	var w = Sim.world
	var picks := []
	var want := [2, 4]
	for c in range(w.country_count()):
		if picks.size() >= 8:
			break
		var role := int(w.country_role(c))
		if role in want:
			want.erase(role)
			picks.append(c)
		elif picks.size() < 6:
			picks.append(c)
	var contact := Image.create(4 * 140, 2 * 150, false, Image.FORMAT_RGBA8)
	contact.fill(Color(0.91, 0.86, 0.72))
	for i in range(picks.size()):
		var cid: int = picks[i]
		var img: Image = Her.compose_arms(w, cid)
		if img == null:
			continue
		contact.blend_rect(img, Rect2i(0, 0, 128, 128),
			Vector2i((i % 4) * 140 + 6, (i / 4) * 150 + 8))
	contact.save_png("res://heraldry_contact.png")
	print("SAVED micro contact (", picks.size(), " pays) en ", Time.get_ticks_msec() - t0, " ms")
	get_tree().quit(0)
