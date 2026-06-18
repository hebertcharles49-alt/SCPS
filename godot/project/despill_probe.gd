extends SceneTree
## Sonde HEADLESS du despill magenta (CPU pur, aucun rendu requis). Charge les atlas
## via UIKit (donc par le NOUVEAU _key_magenta), recompose une planche témoin sur fond
## sombre (le rose résiduel y SAUTE aux yeux) et SCANNE objectivement : combien de
## pixels VISIBLES (alpha>10) penchent encore vers le magenta (m = min(R,B)−G > 25) ?
## Cible : ~0. Lance : godot --headless -s despill_probe.gd

const UIKit = preload("res://ui/uikit.gd")

func _initialize() -> void:
	print("=== SONDE DESPILL MAGENTA ===")
	var bad := 0
	bad += _scan("settlement(t=3,g=4 marché)", UIKit.settlement_sprite(3, 4))
	bad += _scan("settlement(t=5,g=5 fortifié)", UIKit.settlement_sprite(5, 5))
	bad += _scan("settlement(t=1,g=0 montagne)", UIKit.settlement_sprite(1, 0))
	bad += _scan("dressing[16 broadleaf]", UIKit.dressing_sprite(16))
	bad += _scan("dressing[73 chêne]", UIKit.dressing_sprite(73))
	bad += _scan("dressing[19 conifères]", UIKit.dressing_sprite(19))
	_composite()
	print("=== TOTAL pixels magenta VISIBLES : ", bad, " (cible ~0) ===")
	quit()

## compte les pixels visibles qui penchent encore vers le magenta. Imprime aussi le
## pire m résiduel (diag). Renvoie le compte de « franchement roses » (m>25).
func _scan(label: String, tex: Texture2D) -> int:
	if tex == null:
		print("  ", label, " : NULL (atlas absent)")
		return 0
	var img: Image = tex.get_image()
	if img == null:
		print("  ", label, " : image NULL")
		return 0
	if img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	var data := img.get_data()
	var bad := 0
	var worst := 0
	var opaque := 0
	for i in range(0, data.size(), 4):
		if data[i + 3] <= 10:
			continue
		opaque += 1
		var m := mini(data[i], data[i + 2]) - data[i + 1]
		if m > worst:
			worst = m
		if m > 25:
			bad += 1
	print("  ", label, " : ", bad, " roses / ", opaque, " opaques (pire m=", worst, ")")
	return bad

## planche témoin : grille settlements + rangée d'arbres, sur fond sombre → PNG.
func _composite() -> void:
	var cell := 96
	var W := 6 * cell
	var H := 6 * cell + 48
	var sheet := Image.create(W, H, false, Image.FORMAT_RGBA8)
	sheet.fill(Color(0.10, 0.10, 0.13))   # fond sombre : tout rose résiduel saute
	for g in range(6):
		for t in range(6):
			var tex := UIKit.settlement_sprite(t, g)
			if tex == null:
				continue
			var si := tex.get_image()
			if si.get_format() != Image.FORMAT_RGBA8:
				si.convert(Image.FORMAT_RGBA8)
			sheet.blend_rect(si, Rect2i(0, 0, cell, cell), Vector2i(t * cell, g * cell))
	var trees := [16, 73, 19, 79, 75, 72, 23, 22]
	for k in range(trees.size()):
		var tex := UIKit.dressing_sprite(trees[k])
		if tex == null:
			continue
		var ti := tex.get_image()
		if ti.get_format() != Image.FORMAT_RGBA8:
			ti.convert(Image.FORMAT_RGBA8)
		sheet.blend_rect(ti, Rect2i(0, 0, 32, 32), Vector2i(k * 36 + 6, 6 * cell + 8))
	sheet.save_png("/tmp/despill_proof.png")
	print("  planche → /tmp/despill_proof.png")
