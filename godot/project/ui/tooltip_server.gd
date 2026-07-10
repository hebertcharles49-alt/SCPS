extends Control
## TOOLTIP SERVER — tooltips à CONCEPTS en CASCADE (retour joueur 2026-07-10 :
## « double hover » façon CK3). Trois étages de comportement :
##   1. SURVOL (0.45 s) : le tooltip apparaît, mots-concepts turquoise + définitions.
##   2. VERROU (1 s de survol total) : le tooltip se FIGE dans une hitbox élargie,
##      son liseré vire turquoise — son contenu devient interactif.
##   3. CASCADE : survoler un mot turquoise DANS un tooltip verrouillé ouvre le
##      tooltip-enfant de ce concept (né verrouillé) — récursif, chaque définition
##      en appelant d'autres. La chaîne se ferme du plus profond au plus proche
##      quand la souris quitte les hitbox (grâce 0.3 s).
## Display-only : lecture du Control survolé + du registre ui/concepts.gd.

const Concepts = preload("res://ui/concepts.gd")

const DELAY := 0.45          ## s de survol stable avant apparition
const LOCK_AT := 1.0         ## s de survol total avant VERROUILLAGE (hitbox élargie)
const SUB_DELAY := 0.30      ## s sur un mot turquoise avant d'ouvrir son enfant
const GROW := 16.0           ## marge de la hitbox élargie (verrouillé)
const GRACE := 0.30          ## s hors de toute hitbox avant fermeture de la chaîne
const MAXW := 440.0          ## largeur max d'un panneau
const MAXDEPTH := 6          ## garde-fou de cascade

const COL_EDGE_N := Color(0.78, 0.62, 0.30)   ## liseré normal (or)
const COL_EDGE_L := Color(0.35, 0.78, 0.76)   ## liseré VERROUILLÉ (turquoise)

var _levels := []            ## [{panel, rtl, sb}] — niveau 0 = le tooltip racine
var _hover_ctrl: Control = null
var _hover_text := ""
var _t := 0.0                ## temps de survol cumulé sur la source racine
var _locked := false         ## niveau 0 verrouillé (la cascade est ouverte)
var _grace := 0.0
var _sub_level := -1         ## mot turquoise en cours de survol : niveau…
var _sub_key := ""           ## …concept…
var _sub_t := 0.0            ## …et temps accumulé
var _shrink_t := 0.0         ## grâce de RÉGRESSION (souris à un étage moins profond)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)

## fabrique un étage (panneau + RichText) — ajouté par-dessus les précédents
func _mk_level() -> Dictionary:
	var panel := PanelContainer.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.075, 0.06, 0.045, 0.97)
	sb.border_color = COL_EDGE_N
	sb.set_border_width_all(1)
	sb.set_corner_radius_all(3)
	sb.set_content_margin_all(9)
	panel.add_theme_stylebox_override("panel", sb)
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(panel)
	var rtl := RichTextLabel.new()
	rtl.bbcode_enabled = true
	rtl.fit_content = true
	rtl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	rtl.custom_minimum_size = Vector2(MAXW - 18.0, 0)
	rtl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rtl.add_theme_color_override("default_color", Color(0.92, 0.88, 0.76))
	rtl.add_theme_font_size_override("normal_font_size", 14)
	panel.add_child(rtl)
	var lvl := {"panel": panel, "rtl": rtl, "sb": sb}
	var idx := _levels.size()
	rtl.meta_hover_started.connect(func(meta):
		_sub_level = idx
		_sub_key = String(meta)
		_sub_t = 0.0)
	rtl.meta_hover_ended.connect(func(_meta):
		if _sub_level == idx:
			_sub_level = -1
			_sub_key = ""
			_sub_t = 0.0)
	return lvl

func _teardown(from_level: int) -> void:
	while _levels.size() > from_level:
		var lvl: Dictionary = _levels.pop_back()
		(lvl["panel"] as PanelContainer).queue_free()
	if _sub_level >= _levels.size():
		_sub_level = -1
		_sub_key = ""
		_sub_t = 0.0
	if _levels.is_empty():
		_locked = false
		_t = 0.0
		_grace = 0.0

## la souris est-elle dans la CHAÎNE (source racine + hitbox élargies des étages) ?
func _in_chain(mp: Vector2) -> int:
	# renvoie l'étage le plus PROFOND contenant la souris ; -1 = dehors partout
	var deepest := -1
	if _hover_ctrl != null and is_instance_valid(_hover_ctrl) \
			and _hover_ctrl.get_global_rect().grow(8.0).has_point(mp):
		deepest = 0
	for i in range(_levels.size()):
		var r: Rect2 = (_levels[i]["panel"] as PanelContainer).get_global_rect().grow(GROW)
		if r.has_point(mp):
			deepest = maxi(deepest, i + 1)   # étage i = profondeur i+1 (0 = la source)
	return deepest

func _process(delta: float) -> void:
	var vp := get_viewport()
	if vp == null:
		return
	var mp := get_global_mouse_position()

	# ── CHAÎNE VERROUILLÉE : elle vit tant que la souris reste dans les hitbox ──
	if _locked:
		var depth := _in_chain(mp)
		if depth < 0:
			_grace += delta
			if _grace >= GRACE:
				_teardown(0)
			return
		_grace = 0.0
		# la souris est remontée à un étage moins profond → les enfants au-delà ferment,
		# mais avec la MÊME grâce que la sortie de chaîne : un enfant qui vient de
		# naître est HORS de la souris (posé à côté du curseur) — sans grâce il
		# mourrait à la frame suivante (le bug « le niveau 2 saute instantanément »).
		if _levels.size() > maxi(depth, 1):
			_shrink_t += delta
			if _shrink_t >= GRACE:
				_teardown(maxi(depth, 1))
				_shrink_t = 0.0
		else:
			_shrink_t = 0.0
		# CASCADE : un mot turquoise couve → son enfant s'ouvre
		if _sub_level >= 0 and _sub_key != "" and _levels.size() < MAXDEPTH:
			_sub_t += delta
			if _sub_t >= SUB_DELAY and _sub_level == _levels.size() - 1:
				_spawn_child(_sub_key, mp)
				_sub_t = -1000.0   # une fois par survol de mot
		return

	# ── NON VERROUILLÉ : comportement de survol classique + montée vers le verrou ──
	var ctrl := vp.gui_get_hovered_control()
	# survoler NOS panneaux ne compte pas comme un changement de source
	if ctrl != null and is_instance_valid(ctrl) and is_ancestor_of(ctrl):
		ctrl = _hover_ctrl
	var text := ""
	if ctrl != null and is_instance_valid(ctrl):
		text = ctrl.get_tooltip(ctrl.get_local_mouse_position())
	if ctrl != _hover_ctrl or text != _hover_text:
		_hover_ctrl = ctrl
		_hover_text = text
		_t = 0.0
		_teardown(0)
		return
	if text == "":
		return
	_t += delta
	if _levels.is_empty() and _t >= DELAY:
		_show_root(text)
	elif not _levels.is_empty() and _t >= LOCK_AT:
		_lock()

## Politique de contenu (retour joueur) : le hover = nom, raccourci, explication
## FACTUELLE — JAMAIS la définition des concepts dans le corps ; le joueur survole
## le MOT TURQUOISE pour l'obtenir (cascade). Aucune ligne méta d'explication.
func _decorated(text: String, header_key: String = "") -> String:
	var d: Dictionary = Concepts.decorate(text)
	var bb := String(d["bb"])
	if header_key != "":
		var hic: String = Concepts.icon_of(header_key)
		var hpre := ("[img=18x18]%s[/img] " % hic) if hic != "" and ResourceLoader.exists(hic) else ""
		bb = "%s[b][color=#%s]%s[/color][/b]\n%s" % [hpre, Concepts.COL, header_key, bb]
	return bb

func _show_root(text: String) -> void:
	var lvl := _mk_level()
	(lvl["rtl"] as RichTextLabel).text = _decorated(text)
	_levels.append(lvl)
	_place(lvl, get_global_mouse_position() + Vector2(18, 22))

func _lock() -> void:
	_locked = true
	_grace = 0.0
	var lvl: Dictionary = _levels[0]
	(lvl["sb"] as StyleBoxFlat).border_color = COL_EDGE_L
	(lvl["sb"] as StyleBoxFlat).set_border_width_all(2)
	(lvl["panel"] as PanelContainer).mouse_filter = Control.MOUSE_FILTER_STOP
	(lvl["rtl"] as RichTextLabel).mouse_filter = Control.MOUSE_FILTER_STOP

## un ENFANT de cascade : la définition du concept, elle-même décorée (récursif) —
## né VERROUILLÉ (interactif d'emblée : ses mots turquoise cascadent à leur tour).
func _spawn_child(key: String, at: Vector2) -> void:
	var body: String = Concepts.def_of(key)
	if body == "":
		return
	var lvl := _mk_level()
	(lvl["sb"] as StyleBoxFlat).border_color = COL_EDGE_L
	(lvl["sb"] as StyleBoxFlat).set_border_width_all(2)
	(lvl["panel"] as PanelContainer).mouse_filter = Control.MOUSE_FILTER_STOP
	(lvl["rtl"] as RichTextLabel).mouse_filter = Control.MOUSE_FILTER_STOP
	# le corps de l'enfant : décoré (decorate émet déjà les LIENS de cascade)
	var d: Dictionary = Concepts.decorate(body)
	var bb := String(d["bb"])
	var hic: String = Concepts.icon_of(key)
	var hpre := ("[img=18x18]%s[/img] " % hic) if hic != "" and ResourceLoader.exists(hic) else ""
	(lvl["rtl"] as RichTextLabel).text = "%s[b][color=#%s]%s[/color][/b]\n%s" % [
		hpre, Concepts.COL, key, bb]
	_levels.append(lvl)
	_place(lvl, at + Vector2(20, 18))

## pose un étage près d'un point, clampé au viewport (après une frame de layout)
func _place(lvl: Dictionary, at: Vector2) -> void:
	var panel := lvl["panel"] as PanelContainer
	panel.position = at   # position provisoire (le clamp suit la frame de layout)
	await get_tree().process_frame
	if not is_instance_valid(panel):
		return
	var vp := get_viewport_rect().size
	var ps := panel.size
	panel.position = Vector2(clampf(at.x, 4.0, vp.x - ps.x - 4.0),
		clampf(at.y, 4.0, vp.y - ps.y - 4.0))
