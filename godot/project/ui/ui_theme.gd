extends RefCounted
## UI THEME — socle PARCHEMIN (ivoire, encre, or fané) + feedback universel. Réaligné
## (2026-07-14, retour joueur : « elle est passée où la DA juste avant ? ») sur la même
## famille de tons que ParchTheme/VKit — les boutons/champs/tooltips du chrome VKit
## deviennent des chips parchemin au lieu de graphite sombre. « Rendre du feedback à
## chaque bouton qui existe » : (1) des ÉTATS visibles (normal / hover clair / pressed
## enfoncé / disabled fané) posés au niveau du THÈME de la fenêtre → chaque Button/
## OptionButton/CheckBox présent ET futur en hérite sans câblage ; (2) un FLASH de clic
## (pulse de modulate) accroché à CHAQUE BaseButton via le signal d'arbre node_added —
## universel, aucun panneau à retoucher. Display-only.

static func _box(bg: Color, border: Color, bw: int = 1, shift_down := false) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = bg
	sb.border_color = border
	sb.set_border_width_all(bw)
	sb.set_corner_radius_all(1)
	sb.content_margin_left = 10.0
	sb.content_margin_right = 10.0
	sb.content_margin_top = 5.0 + (2.0 if shift_down else 0.0)
	sb.content_margin_bottom = 5.0 - (2.0 if shift_down else 0.0)
	return sb

static func build() -> Theme:
	var th := Theme.new()
	# POLICE DE BASE : Alegreya Sans (humaniste, calligraphique mais lisible) — toute
	# l'UI Control (boutons, panneaux, tooltips) en hérite via le thème de la fenêtre.
	var VKit := preload("res://ui/vkit.gd")
	var fui: Font = VKit.font()
	if fui != null:
		th.default_font = fui
		th.default_font_size = 16   # +1 cran (retour joueur 2026-07-10 : « agrandis la police »)
	# ── BOUTONS : chips parchemin (chrome VKit désormais aligné sur ParchTheme).
	# Les planches sheet02 ont des fleurons AU MILIEU des bords → AUCUN 9-slice ne peut les
	# étirer sans les déformer (le « découpage » raté) ; le cadre plat s'étire NET à toute
	# taille (survol plus clair, appui enfoncé & plus sombre, désactivé fané).
	var normal := _box(VKit.COL_PANEL2, VKit.COL_EDGE, 1)
	var hover := _box(VKit.COL_PANEL, VKit.COL_GOLD, 2)
	var press := _box(Color(0xc3/255.0, 0xad/255.0, 0x78/255.0), Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.85), 2, true)
	var disab := _box(Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.55), Color(VKit.COL_EDGE.r, VKit.COL_EDGE.g, VKit.COL_EDGE.b, 0.5), 1)
	var focus := StyleBoxFlat.new()
	focus.draw_center = false
	focus.border_color = Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.85)
	focus.set_border_width_all(2)
	focus.set_corner_radius_all(1)
	for cls in ["Button", "OptionButton", "CheckBox", "MenuButton", "CheckButton"]:
		th.set_stylebox("normal", cls, normal)
		th.set_stylebox("hover", cls, hover)
		th.set_stylebox("pressed", cls, press)
		th.set_stylebox("disabled", cls, disab)
		th.set_stylebox("focus", cls, focus)
		th.set_color("font_color", cls, VKit.COL_PARCH)
		th.set_color("font_hover_color", cls, VKit.COL_PARCH)
		th.set_color("font_pressed_color", cls, VKit.COL_GOLD)
		th.set_color("font_disabled_color", cls, Color(VKit.COL_DIM.r, VKit.COL_DIM.g, VKit.COL_DIM.b, 0.65))
	# LineEdit : champ lisible + focus doré (feuillet blanchi, plus clair que le panneau)
	var le := _box(Color(0xe7/255.0, 0xd6/255.0, 0xa8/255.0), VKit.COL_EDGE)
	th.set_stylebox("normal", "LineEdit", le)
	th.set_stylebox("focus", "LineEdit", _box(Color(0xe7/255.0, 0xd6/255.0, 0xa8/255.0), VKit.COL_GOLD, 2))
	th.set_color("font_color", "LineEdit", VKit.COL_PARCH)
	# Panneaux natifs : même plaque que les panneaux immédiats, cohérente avec ParchTheme.
	var native_panel := _box(VKit.COL_PANEL, VKit.COL_EDGE, 1)
	for cls in ["Panel", "PanelContainer", "PopupPanel"]:
		th.set_stylebox("panel", cls, native_panel)
	# TOOLTIP : encart parchemin quasi opaque, liseré or — le tooltip système gris
	# cassait la charte partout (chaque bouton en a un). ENCRE sur IVOIRE, pas l'inverse.
	var tip := StyleBoxFlat.new()
	tip.bg_color = Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.985)
	tip.border_color = VKit.COL_GOLD
	tip.set_border_width_all(1)
	tip.set_border_width(SIDE_LEFT, 3)
	tip.set_corner_radius_all(1)
	tip.set_content_margin_all(8)
	th.set_stylebox("panel", "TooltipPanel", tip)
	th.set_color("font_color", "TooltipLabel", VKit.COL_PARCH)
	th.set_font_size("font_size", "TooltipLabel", 14)
	return th

## FLASH DE CLIC universel : chaque BaseButton (présent + futur) pulse à l'appui.
static func attach_feedback(tree: SceneTree) -> void:
	tree.node_added.connect(func(n: Node):
		if n is BaseButton:
			_wire(n))
	_walk(tree.root)

static func _walk(n: Node) -> void:
	if n is BaseButton:
		_wire(n)
	for c in n.get_children():
		_walk(c)

static func _wire(b: BaseButton) -> void:
	if b.has_meta("_scps_fb"):
		return
	b.set_meta("_scps_fb", true)
	# Le focus clavier RESTE VIVANT (Tab/Entrée/focus visible — audit 2026-07-10) :
	# la barre d'espace est interceptée EN AMONT du focus par main._input (la pause
	# répond après un clic SANS sacrifier l'accessibilité clavier).
	b.button_down.connect(func():
		Sound.play("ui_click")   # LE clic universel (façon iPhone) — chaque bouton tape
		Sim.notify_action()      # PAUSE : l'UI se rafraîchit au clic (retour joueur 2026-07-09)
		var tw := b.create_tween()
		b.modulate = Color(1.35, 1.28, 1.05)
		tw.tween_property(b, "modulate", Color(1, 1, 1), 0.22).set_trans(Tween.TRANS_QUAD))
