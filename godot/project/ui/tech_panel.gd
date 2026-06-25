extends Control
## TechPanel — l'arbre de technologie du JOUEUR (read-only), bascule touche T.
## Lit tech_info (points · présage · crise) + tech_nodes (la grille concentrique
## aplatie). Rendu en 3 colonnes (un thème par colonne), nœuds triés par tier ;
## l'état est une COULEUR (verrouillé/ouvert/acquis), le bout faustien marqué ⚠.
## Display-only : choisir une recherche se fera via une commande (pas encore).
## Membrane : des MOTS résolus + des nombres tangibles (points, coût, crise %).

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const PW := 624.0
const PH := 472.0
const COLW := 200.0

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: queue_redraw())
	hide()

func _layout() -> void:
	var vp := get_viewport_rect().size
	position = Vector2((vp.x - PW) * 0.5, (vp.y - PH) * 0.5)

func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	var info: Dictionary = w.tech_info()

	# en-tête : titre · points dispo · présage faustien (bande) · proximité de crise
	UIKit.draw_icon(self, "knowledge_book", Vector2(14, 12), 20)
	VKit.text(self, Vector2(42, 13), VKit.COL_COPPER, "Arbre de technologie", VKit.FS_BIG)
	VKit.text(self, Vector2(PW - 250, 13), VKit.COL_PARCH, "Points : %d" % int(info.get("points", 0)), VKit.FS_SMALL)
	var crise := int(info.get("crise_pct", 0))
	var pcol := VKit.COL_DIM if crise < 25 else (VKit.sense(0.40) if crise < 60 else VKit.sense(0.10))
	VKit.text(self, Vector2(PW - 250, 29), pcol,
		"Présage : %s (crise %d%%)" % [String(info.get("presage", "")), crise], VKit.FS_SMALL)
	VKit.fill(self, Rect2(12, 46, PW - 24, 1), VKit.COL_EDGE)

	# 3 colonnes (un thème par colonne) ; nœuds du thème triés par tier
	var themes: Array = info.get("themes", [])
	var nodes: Array = w.tech_nodes()
	for th in range(3):
		var cx := 16.0 + th * COLW
		var cy := 54.0
		VKit.text(self, Vector2(cx, cy), VKit.COL_COPPER,
			String(themes[th]) if th < themes.size() else "?", VKit.FS_SMALL)
		cy += 18
		var col_nodes := []
		for n in nodes:
			if int(n["quarter"]) / 3 == th:
				col_nodes.append(n)
		col_nodes.sort_custom(func(a, b): return int(a["tier"]) < int(b["tier"]))
		for n in col_nodes:
			if cy > PH - 16:
				break
			var st := int(n["state"])
			var col := VKit.COL_DIM            # verrouillé
			if st == 1:
				col = VKit.COL_COPPER          # ouvert (recherchable)
			elif st == 2:
				col = VKit.sense(0.82)         # acquis
			var label := String(n["name"])
			if bool(n.get("faustian", false)):
				label += " ⚠"
			VKit.text(self, Vector2(cx, cy), col, label, VKit.FS_SMALL)
			if int(n.get("cost", 0)) > 0 and st != 2:
				VKit.text(self, Vector2(cx + COLW - 46, cy), VKit.COL_DIM, "%d" % int(n["cost"]), VKit.FS_SMALL)
			cy += 13

	# pied : légende d'état + rappel touche
	var ly := PH - 16.0
	VKit.text(self, Vector2(16, ly), VKit.sense(0.82), "■ acquis", VKit.FS_SMALL)
	VKit.text(self, Vector2(92, ly), VKit.COL_COPPER, "■ recherchable", VKit.FS_SMALL)
	VKit.text(self, Vector2(210, ly), VKit.COL_DIM, "■ verrouillé   ⚠ faustien", VKit.FS_SMALL)
	VKit.text(self, Vector2(PW - 96, ly), VKit.COL_DIM, "[T] fermer", VKit.FS_SMALL)
