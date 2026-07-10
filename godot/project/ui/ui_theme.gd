extends RefCounted
## UI THEME — le THÈME GLOBAL (parchemin sombre) + le FEEDBACK DE CLIC universel.
## « Rendre du feedback à chaque bouton qui existe » : (1) des ÉTATS visibles (normal /
## hover clair / pressed enfoncé / disabled fané) posés au niveau du THÈME de la fenêtre
## → chaque Button/OptionButton/CheckBox présent ET futur en hérite sans câblage ;
## (2) un FLASH de clic (pulse de modulate) accroché à CHAQUE BaseButton via le signal
## d'arbre node_added — universel, aucun panneau à retoucher. Display-only.

static func _box(bg: Color, border: Color, bw: int = 1, shift_down := false) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = bg
	sb.border_color = border
	sb.set_border_width_all(bw)
	sb.set_corner_radius_all(3)
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
	# ── BOUTONS : cadre CUIR + bordure OR (StyleBoxFlat), 4 états nettement distincts.
	# Les planches sheet02 ont des fleurons AU MILIEU des bords → AUCUN 9-slice ne peut les
	# étirer sans les déformer (le « découpage » raté) ; le cadre plat s'étire NET à toute
	# taille (survol plus clair, appui enfoncé & sombre, désactivé fané).
	var normal := _box(Color(0.26, 0.20, 0.14), Color(0.74, 0.58, 0.30), 2)
	var hover := _box(Color(0.35, 0.28, 0.19), Color(0.93, 0.75, 0.41), 2)
	var press := _box(Color(0.17, 0.13, 0.09), Color(0.66, 0.52, 0.28), 2, true)
	var disab := _box(Color(0.20, 0.17, 0.14, 0.6), Color(0.42, 0.35, 0.24, 0.55), 1)
	var focus := StyleBoxFlat.new()
	focus.draw_center = false
	focus.border_color = Color(0.86, 0.68, 0.26, 0.55)   # l'or de sélection, discret
	focus.set_border_width_all(1)
	focus.set_corner_radius_all(3)
	for cls in ["Button", "OptionButton", "CheckBox", "MenuButton", "CheckButton"]:
		th.set_stylebox("normal", cls, normal)
		th.set_stylebox("hover", cls, hover)
		th.set_stylebox("pressed", cls, press)
		th.set_stylebox("disabled", cls, disab)
		th.set_stylebox("focus", cls, focus)
		th.set_color("font_color", cls, Color(0.90, 0.85, 0.72))
		th.set_color("font_hover_color", cls, Color(0.99, 0.95, 0.82))
		th.set_color("font_pressed_color", cls, Color(0.80, 0.72, 0.55))
		th.set_color("font_disabled_color", cls, Color(0.55, 0.50, 0.42))
	# LineEdit : champ lisible + focus doré
	var le := _box(Color(0.12, 0.10, 0.08), Color(0.38, 0.32, 0.22))
	th.set_stylebox("normal", "LineEdit", le)
	th.set_stylebox("focus", "LineEdit", _box(Color(0.14, 0.12, 0.09), Color(0.86, 0.68, 0.26)))
	th.set_color("font_color", "LineEdit", Color(0.92, 0.88, 0.76))
	# TOOLTIP façon EU4 (rendu attendu 2026-07-09) : encart SOMBRE quasi opaque, liseré
	# OR — le tooltip système gris cassait la charte partout (chaque bouton en a un).
	var tip := StyleBoxFlat.new()
	tip.bg_color = Color(0.075, 0.06, 0.045, 0.97)
	tip.border_color = Color(0.78, 0.62, 0.30)
	tip.set_border_width_all(1)
	tip.set_corner_radius_all(3)
	tip.set_content_margin_all(8)
	th.set_stylebox("panel", "TooltipPanel", tip)
	th.set_color("font_color", "TooltipLabel", Color(0.92, 0.88, 0.76))
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
