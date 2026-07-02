extends Node
## ui_audit — capture la scène Main ENTIÈRE (menu par-dessus la carte) pour juger le
## chrome parchemin (boutons, panneaux, thème). Fenêtré.
func _ready() -> void:
	get_window().size = Vector2i(1600, 900)
	var main = load("res://main/Main.tscn").instantiate()
	add_child(main)
	_shot.call_deferred()

func _shot() -> void:
	for i in range(36):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("res://ui1.png")
	print("SAVED")
	get_tree().quit(0)
