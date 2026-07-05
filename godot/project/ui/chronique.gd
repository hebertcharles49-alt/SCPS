extends Control
## Chronique — LES ANNALES DU RÈGNE : le récit SÉLECTIF de la partie, en lecture seule
## (touche H). Une frise déroulante an+ligne (moteur : scps_events.c annals[], façade :
## scps_annals) ; le clic sur une entrée localisée (region>=0) centre la carte dessus.
## RÈGLE D'OR : zéro logique de sim — on LIT annals() et on émet un signal de navigation ;
## rien n'est muté ici (pas de verbe, pas d'écriture moteur).

signal goto_region(region: int)   ## main.gd centre la carte (même motif que alerts/event_popup)

var _list: VBoxContainer
var _status: Label

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(560, 640)
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.10, 0.08, 0.06, 0.97)
	sb.border_color = Color(0.62, 0.52, 0.30)
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(14)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	panel.add_child(col)

	var title := Label.new()
	title.text = "Les Annales du Règne"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	col.add_child(title)

	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
	col.add_child(_status)
	col.add_child(HSeparator.new())

	var sc := ScrollContainer.new()
	sc.custom_minimum_size = Vector2(530, 520)
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
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(func(): hide())
	foot.add_child(close)

	visibility_changed.connect(func(): if visible: _rebuild())
	hide()

func _rebuild() -> void:
	if _list == null:
		return
	for c in _list.get_children():
		c.queue_free()
	if Sim.world == null or not Sim.world.has_method("annals"):
		_status.text = "(moteur des Annales absent)"
		return
	var entries: Array = Sim.world.annals()
	if entries.is_empty():
		_status.text = "Le règne n'a encore rien à raconter."
		return
	_status.text = "%d fait(s) retenu(s) — le panthéon du règne." % entries.size()
	for e in entries:
		var row := Button.new()
		row.flat = true
		row.alignment = HORIZONTAL_ALIGNMENT_LEFT
		row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		row.text = String(e.get("ligne", ""))
		var region := int(e.get("region", -1))
		row.disabled = region < 0   # rien à centrer : pas cliquable (pas un bouton mort visuel non plus — juste inerte)
		row.focus_mode = Control.FOCUS_NONE
		if region >= 0:
			row.pressed.connect(func(): goto_region.emit(region))
		_list.add_child(row)

## rouvre le panneau (touche H en jeu).
func open() -> void:
	show()
	_rebuild()
