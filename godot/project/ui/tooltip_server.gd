extends Control
## TOOLTIP SERVER — tooltips à CONCEPTS en CASCADE (retour joueur 2026-07-10 :
## « double hover » façon CK3). Trois étages de comportement :
##   1. SURVOL (0.45 s) : le tooltip apparaît, mots-concepts turquoise + définitions.
##   2. VERROU (1 s de survol total) : le tooltip se FIGE dans une hitbox élargie,
##      son liseré vire turquoise — son contenu devient interactif.
##   3. CASCADE : survoler un mot turquoise DANS un tooltip verrouillé ouvre le
##      tooltip-enfant de ce concept (né verrouillé) — récursif, chaque définition
##      en appelant d'autres. La chaîne se ferme du plus profond au plus proche
##      quand la souris quitte les hitbox élargies.
## Display-only : lecture du Control survolé + du registre ui/concepts.gd.

const Concepts = preload("res://ui/concepts.gd")

signal navigate_requested(request: Dictionary)

const DELAY := 0.45          ## s de survol stable avant apparition
const LOCK_AT := 1.0         ## s de survol total avant VERROUILLAGE (hitbox élargie)
const SUB_DELAY := 0.30      ## s sur un mot turquoise avant d'ouvrir son enfant
const GROW := 22.0           ## marge de la hitbox élargie (verrouillé)
const GRACE := 0.12          ## juste assez pour franchir l'espace vers un enfant
const MAXW := 440.0          ## largeur max d'un panneau
const MAXDEPTH := 6          ## garde-fou de cascade

const COL_EDGE_N := Color(0.78, 0.62, 0.30)   ## liseré normal (or)
const COL_EDGE_L := Color(0.35, 0.78, 0.76)   ## liseré VERROUILLÉ (turquoise)

var _levels := []            ## [{panel, rtl, sb}] — niveau 0 = le tooltip racine
var _hover_ctrl: Control = null
var _hover_text := ""
var _hover_card: Dictionary = {}
var _t := 0.0                ## temps de survol cumulé sur la source racine
var _locked := false         ## niveau 0 verrouillé (la cascade est ouverte)
var _grace := 0.0
var _sub_level := -1         ## mot turquoise en cours de survol : niveau…
var _sub_key := ""           ## …concept…
var _sub_t := 0.0            ## …et temps accumulé
var _shrink_t := 0.0         ## grâce de RÉGRESSION (souris à un étage moins profond)
var _anchor := Vector2.ZERO  ## point-souris à l'ouverture du tooltip racine (ancre de dismiss)

const SRC_LEAVE := 72.0      ## px : au-delà de cette distance de l'ancre, le SURVOL de la
                             ## source ne maintient plus la chaîne verrouillée (sans ça un
                             ## Control LARGE — la topbar dessine ses cellules sur UN seul
                             ## Control pleine largeur — gardait le tooltip figé tant que la
                             ## souris restait n'importe où sur la barre : « ne se dismiss
                             ## qu'au clic », le clic invalidant la source. La souris SUR le
                             ## tooltip le garde vivant par la hitbox du PANNEAU, pas ceci.)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)

## UI-POLISH #11 (spec joueur corrigée : dismiss par MOUVEMENT hors cible, PAS par clic) —
## la règle reste « visible tant que la souris est sur la cible, disparaît dès qu'elle la
## quitte » ; le bug n'était PAS dans cette règle mais dans sa DÉTECTION : Godot ne
## recalcule `gui_get_hovered_control()` qu'au prochain évènement de MOUVEMENT — un clic
## qui fait apparaître un nouveau panneau SOUS une souris IMMOBILE (ex. « Construction »
## ouvre le menu construction sans que la souris bouge) laisse le survol PÉRIMÉ pointer
## sur l'ancien bouton, dont le tooltip reste donc rendu PAR-DESSUS le nouveau panneau.
## Fix : après tout clic, on POUSSE un mouvement souris SYNTHÉTIQUE (position inchangée)
## pour forcer Godot à refaire le hit-test — la cible « quittée » (couverte) est alors
## correctement détectée comme telle, et le tooltip suit la règle normale.
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		call_deferred("_refresh_hover_after_click")

func _refresh_hover_after_click() -> void:
	var vp := get_viewport()
	if vp == null:
		return
	var mm := InputEventMouseMotion.new()
	mm.position = vp.get_mouse_position()
	mm.global_position = mm.position
	Input.parse_input_event(mm)

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
		if String(meta).begins_with("nav:") or String(meta) == "close":
			return
		_sub_level = idx
		_sub_key = String(meta)
		_sub_t = 0.0)
	rtl.meta_hover_ended.connect(func(_meta):
		if _sub_level == idx:
			_sub_level = -1
			_sub_key = ""
			_sub_t = 0.0)
	rtl.meta_clicked.connect(func(meta):
		var mk := String(meta)
		if mk == "close":
			_teardown(0)
			return
		if not mk.begins_with("nav:"):
			return
		var ai := int(mk.trim_prefix("nav:"))
		var actions: Array = lvl.get("actions", [])
		if ai >= 0 and ai < actions.size():
			var action = actions[ai]
			if action is Dictionary and action.get("request", {}) is Dictionary:
				navigate_requested.emit((action["request"] as Dictionary).duplicate(true)))
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
	# La SOURCE ne maintient la chaîne que si la souris est encore DESSUS *et* proche de
	# l'ancre : un Control large ne fige plus le tooltip une fois qu'on s'en éloigne.
	if _hover_ctrl != null and is_instance_valid(_hover_ctrl) \
			and _hover_ctrl.get_global_rect().grow(8.0).has_point(mp) \
			and mp.distance_to(_anchor) <= SRC_LEAVE:
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
	var card: Dictionary = {}
	if ctrl != null and is_instance_valid(ctrl):
		text = ctrl.get_tooltip(ctrl.get_local_mouse_position())
		if ctrl.has_method("get_info_card"):
			var payload = ctrl.call("get_info_card", ctrl.get_local_mouse_position())
			if payload is Dictionary:
				card = payload
	if ctrl != _hover_ctrl or text != _hover_text or var_to_str(card) != var_to_str(_hover_card):
		_hover_ctrl = ctrl
		_hover_text = text
		_hover_card = card.duplicate(true)
		_t = 0.0
		_teardown(0)
		return
	if text == "" and card.is_empty():
		return
	_t += delta
	if _levels.is_empty() and _t >= DELAY:
		_show_root(text, card)
	elif not _levels.is_empty() and _t >= LOCK_AT:
		_lock()

## Politique de contenu (retour joueur) : le hover = nom, raccourci, explication
## FACTUELLE — JAMAIS la définition des concepts dans le corps ; le joueur survole
## le MOT TURQUOISE pour l'obtenir (cascade). Aucune ligne méta d'explication.
func _decorated(text: String, header_key: String = "") -> String:
	var raw_lines := text.split("\n", false)
	var bb := ""
	if raw_lines.size() <= 1:
		bb = String(Concepts.decorate(text).get("bb", text))
	else:
		var head := String(raw_lines[0]).strip_edges()
		bb = "[b]%s[/b]" % String(Concepts.decorate(head).get("bb", head))
		for i in range(1, raw_lines.size()):
			var line := String(raw_lines[i]).strip_edges()
			if line.begins_with("•"):
				line = line.trim_prefix("•").strip_edges()
			if line != "":
				bb += "\n• " + String(Concepts.decorate(line).get("bb", line))
	if header_key != "":
		var hic: String = Concepts.icon_of(header_key)
		var hpre := ("[img=18x18]%s[/img] " % hic) if hic != "" and ResourceLoader.exists(hic) else ""
		bb = "%s[b][color=#%s]%s[/color][/b]\n%s" % [hpre, Concepts.COL, header_key, bb]
	return bb

func _show_root(text: String, card: Dictionary = {}) -> void:
	var lvl := _mk_level()
	# TOOLTIP CUSTOM (2026-07-25) : le serveur avait ENTERRÉ le canal natif
	# `_make_custom_tooltip` (tooltip_delay_sec=100000 → il ne tirait plus jamais) — la
	# peinture de biome (BiomeTip, fiche province) avait disparu. On l'EMBARQUE ici :
	# instancié UNE fois au show (jamais dans le poll de _process — ça fuirait), inséré
	# au-dessus du texte ; _teardown le libère avec le panneau.
	if _hover_ctrl != null and is_instance_valid(_hover_ctrl) and _hover_ctrl.has_method("_make_custom_tooltip"):
		var mc = _hover_ctrl.call("_make_custom_tooltip", text)
		if mc is Control:
			var panel := lvl["panel"] as PanelContainer
			var rtl := lvl["rtl"] as RichTextLabel
			panel.remove_child(rtl)
			var vb := VBoxContainer.new()
			vb.mouse_filter = Control.MOUSE_FILTER_IGNORE
			panel.add_child(vb)
			(mc as Control).mouse_filter = Control.MOUSE_FILTER_IGNORE
			vb.add_child(mc)
			vb.add_child(rtl)
			rtl.visible = text.strip_edges() != "" or not card.is_empty()
	if card.is_empty():
		(lvl["rtl"] as RichTextLabel).text = _decorated(text)
	else:
		lvl["actions"] = card.get("actions", []).duplicate(true)
		(lvl["rtl"] as RichTextLabel).text = _card_bb(card)
	_levels.append(lvl)
	_anchor = get_global_mouse_position()
	_place(lvl, _anchor + Vector2(18, 22))

## Payload structuré optionnel. Les contrôles non migrés continuent à fournir une
## simple String ; les cartes utilisent les mêmes décorations de concepts.
func _card_bb(card: Dictionary) -> String:
	var title := String(card.get("title", ""))
	var state := String(card.get("state", ""))
	var trend := String(card.get("trend", ""))
	var trend_tone := String(card.get("trend_tone", "positive"))
	var body := String(card.get("body", ""))
	var out := "[b]%s[/b]" % String(Concepts.decorate(title).get("bb", title))
	if state != "" or trend != "":
		out += "\n• [color=#e8d9b0]%s[/color]" % state
		if trend != "":
			var trend_col := "df746d" if trend_tone == "negative" else "a9c98e"
			out += "  [color=#%s]%s[/color]" % [trend_col, trend]
	for line in card.get("lines", []):
		var txt := ""
		var tone := ""
		if line is Dictionary:
			txt = "%s  %s" % [String(line.get("label", "")), String(line.get("value", ""))]
			tone = String(line.get("tone", ""))
		else:
			txt = String(line)
		if txt != "":
			var decorated := String(Concepts.decorate(txt).get("bb", txt))
			var tone_col: String = {"positive": "7fd18a", "negative": "df746d",
				"heading": "d7bd75", "dim": "99917f"}.get(tone, "")
			out += "\n• [color=#%s]%s[/color]" % [tone_col, decorated] if tone_col != "" else "\n• " + decorated
	if body != "":
		out += "\n• " + String(Concepts.decorate(body).get("bb", body))
	var actions: Array = card.get("actions", [])
	if not actions.is_empty():
		out += "\n"
		for i in range(actions.size()):
			var action = actions[i]
			if action is Dictionary:
				out += "\n[url=nav:%d][color=#%s]› %s[/color][/url]" % [
					i, Concepts.COL, String(action.get("label", "Ouvrir"))]
	out += "\n\n[url=close][color=#%s]Fermer[/color][/url]" % Concepts.COL
	return out

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
