extends Control
## Épilogue — l'écran de FIN DE PARTIE : quand endgame_info signale une fin déclenchée
## (1-3 apocalypse) ou l'ascension/victoire Merveille (fin 4), main.gd ouvre CET écran
## une seule fois (latch côté main). « Votre règne en une phrase » — composée en GDScript
## depuis les annales + l'épithète émergente + la nature de la fin — puis la frise
## COMPLÈTE des annales (même rendu que chronique.gd). « Contempler » referme et laisse
## regarder le monde muter. RÈGLE D'OR : lecture seule de la façade, aucun verbe.

const VKit = preload("res://ui/vkit.gd")
const Epithet = preload("res://ui/epithet.gd")

signal goto_region(region: int)

## la nature de la fin, en queue de phrase (fin de endgame_info ; 4 = ascension, 5 = sang)
const FIN_PHRASE := {
	1: "vit les flots engloutir le monde",
	2: "vit le Grand Hiver éteindre les feux",
	3: "vit les Ronces dévorer le monde",
	4: "s'éleva au-delà du monde — la Merveille accomplie",
	5: "noya son règne dans le sang — la Main ne s'arrêta plus",
}

## fond de fin (écrans Codex 1920×1080) — indexé par endgame_info["fin"]
const FIN_SCREEN_DIR := "res://assets/scps/ui/endgame/"
const FIN_SCREEN := {
	1: "end_submersion_arcane_1920x1080.png",
	2: "end_cold_copper_1920x1080.png",
	3: "end_thorns_green_1920x1080.png",
	4: "end_ascension_brass_1920x1080.png",
	5: "end_blood_hand_1920x1080.png",
}

var _bg: TextureRect
var _phrase: Label
var _list: VBoxContainer
var _prev_speed := -1

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false
	# la vitesse d'avant est restaurée QUAND l'écran se cache — quel que soit le
	# chemin (bouton « Contempler », Échap via _close_topmost) : un seul point de sortie.
	visibility_changed.connect(func():
		if not visible and _prev_speed >= 0:
			Sim.set_speed(_prev_speed)
			_prev_speed = -1)

	# le FOND DE FIN (écran d'apocalypse/ascension) sous un voile plus léger pour garder
	# le panneau lisible. Texture posée à l'ouverture (open) ; absente ⇒ juste le voile.
	_bg = TextureRect.new()
	_bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_bg)

	var veil := ColorRect.new()
	veil.color = Color(0.03, 0.02, 0.01, 0.52)
	veil.set_anchors_preset(Control.PRESET_FULL_RECT)
	veil.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(veil)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(620, 620)
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.10, 0.08, 0.06, 0.97)
	sb.border_color = VKit.COL_GOLD
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(16)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	panel.add_child(col)

	var title := Label.new()
	title.text = "Épilogue"
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", VKit.COL_GOLD)
	col.add_child(title)

	_phrase = Label.new()
	_phrase.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_phrase.add_theme_font_size_override("font_size", 17)
	_phrase.add_theme_color_override("font_color", VKit.COL_PARCH)
	col.add_child(_phrase)
	col.add_child(HSeparator.new())

	var sub := Label.new()
	sub.text = "Les Annales, en entier — ce que les chroniqueurs ont daigné retenir :"
	sub.add_theme_color_override("font_color", VKit.COL_DIM)
	col.add_child(sub)

	var sc := ScrollContainer.new()
	sc.custom_minimum_size = Vector2(580, 380)
	sc.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(sc)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 4)
	sc.add_child(_list)

	var foot := HBoxContainer.new()
	col.add_child(foot)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	var btn := Button.new(); btn.text = "Contempler"
	btn.pressed.connect(func(): visible = false)   # referme, laisse REGARDER le monde
	foot.add_child(btn)

## OUVRE l'épilogue (main.gd, au latch de fin). `fin` = endgame_info["fin"] (1-4).
func open(fin: int) -> void:
	var w = Sim.world
	if w == null:
		return
	# le fond d'apocalypse/ascension correspondant à la fin
	var scr: String = FIN_SCREEN.get(fin, "")
	if scr != "" and ResourceLoader.exists(FIN_SCREEN_DIR + scr):
		_bg.texture = load(FIN_SCREEN_DIR + scr)
	else:
		_bg.texture = null
	var entries: Array = w.annals() if w.has_method("annals") else []
	_phrase.text = _compose(w, entries, fin)
	for c in _list.get_children():
		c.queue_free()
	if entries.is_empty():
		var l := Label.new()
		l.text = "Rien. Un règne entier, et rien à raconter."
		l.add_theme_color_override("font_color", VKit.COL_DIM)
		_list.add_child(l)
	for e in entries:
		var row := Button.new()
		row.flat = true
		row.alignment = HORIZONTAL_ALIGNMENT_LEFT
		row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		row.text = String(e.get("ligne", ""))
		var region := int(e.get("region", -1))
		row.disabled = region < 0
		row.focus_mode = Control.FOCUS_NONE
		if region >= 0:
			row.pressed.connect(func(): goto_region.emit(region))
		_list.add_child(row)
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)               # la fin du monde mérite bien un arrêt
	visible = true

## « Votre règne en une phrase » — nom + épithète + durée + fait dominant + la fin.
func _compose(w, entries: Array, fin: int) -> String:
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var nom := String(ci.get("nom", "Un royaume oublié"))
	var ep: String = Epithet.derive(entries)
	var years: int = int(w.year())
	# le FAIT dominant, compté des annales (mêmes kinds que l'épithète)
	var wins := 0; var losses := 0; var monuments := 0; var dilemmes := 0
	for e in entries:
		match int(e.get("kind", -1)):
			Epithet.K_GUERRE_GAGNEE: wins += 1
			Epithet.K_GUERRE_PERDUE: losses += 1
			Epithet.K_MONUMENT: monuments += 1
			Epithet.K_DILEMME: dilemmes += 1
	var fait := "ne fit trembler personne"
	if wins >= 3:
		fait = "fit trembler %d empires" % wins
	elif wins >= 1:
		fait = "fit plier %d rival(aux)" % wins
	elif monuments >= 2:
		fait = "éleva %d monuments" % monuments
	elif dilemmes >= 3:
		fait = "trancha %d querelles" % dilemmes
	elif losses >= 2:
		fait = "encaissa %d défaites" % losses
	var fin_txt: String = FIN_PHRASE.get(fin, "vit son histoire s'arrêter là")
	return "%s, dit %s, régna %d ans, %s, et %s." % [nom, ep, years, fait, fin_txt]
