extends Node2D

const Overlay = preload("res://map/overlay.gd")

class SoldierPreview extends Overlay:
	func _ready() -> void:
		set_process(false)
	func _draw() -> void:
		draw_rect(Rect2(0, 0, 640, 320), Color("c8c49b"))
		draw_line(Vector2(70, 220), Vector2(560, 120), Color("665737"), 2)
		_draw_army_soldier(Vector2(150, 205), 64.0, 1.0, 1)
		_draw_army_soldier(Vector2(350, 164), 40.0, 1.0, 1)
		_draw_army_soldier(Vector2(520, 129), 24.0, 1.0, 0)

func _ready() -> void:
	get_window().size = Vector2i(640, 320)
	get_window().content_scale_size = Vector2i(640, 320)
	var preview := SoldierPreview.new()
	add_child(preview)
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var err := get_viewport().get_texture().get_image().save_png("user://army_soldier_visual.png")
	print("army_soldier_visual_test: ", err)
	get_tree().quit(0 if err == OK else 1)
