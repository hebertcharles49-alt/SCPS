extends Control
## DevPanel — MODTOOLS : édite les ~168 TUNABLES EN DIRECT (touche F10).
##
## Lit Sim.world.tunables() (nom · valeur · défaut · surchargé) ; tune_set applique la
## surcharge LIVE (l'effet apparaît là où le moteur relit tune_f au tick). Dev-only :
## zéro logique sim, c'est de l'édition de coefficients que le moteur LIT déjà (discipline
## « l'effet passe par les entrées du moteur »). RÈGLE D'OR : GUI → façade (jamais les
## structs moteur). Un monde ainsi modé n'est plus rejouable vanilla (cf. SCPS_TUNE).

var _list: VBoxContainer
var _filter: LineEdit
var _status: Label

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var panel := PanelContainer.new()
	panel.set_anchors_preset(Control.PRESET_CENTER)
	panel.custom_minimum_size = Vector2(580, 660)
	add_child(panel)
	var vb := VBoxContainer.new()
	panel.add_child(vb)
	var title := Label.new()
	title.text = "MODTOOLS — Tunables (édition LIVE · F10 pour fermer)"
	vb.add_child(title)
	_filter = LineEdit.new()
	_filter.placeholder_text = "filtrer par nom…"
	_filter.text_changed.connect(func(_t): _rebuild())
	vb.add_child(_filter)
	var sc := ScrollContainer.new()
	sc.custom_minimum_size = Vector2(560, 580)
	vb.add_child(sc)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sc.add_child(_list)
	_status = Label.new()
	vb.add_child(_status)
	visibility_changed.connect(func(): if visible: _rebuild())
	hide()

func _rebuild() -> void:
	if _list == null:
		return
	for c in _list.get_children():
		c.queue_free()
	if Sim.world == null:
		_status.text = "(pas de monde)"
		return
	var flt := _filter.text.to_lower()
	var shown := 0
	for t in Sim.world.tunables():
		var nom := String(t["nom"])
		if flt != "" and not nom.to_lower().contains(flt):
			continue
		var row := HBoxContainer.new()
		var lbl := Label.new()
		lbl.text = nom + ("  *" if bool(t["overridden"]) else "")
		lbl.custom_minimum_size = Vector2(330, 0)
		row.add_child(lbl)
		var ed := LineEdit.new()
		ed.text = str(t["value"])
		ed.custom_minimum_size = Vector2(120, 0)
		ed.text_submitted.connect(_apply.bind(nom))
		row.add_child(ed)
		_list.add_child(row)
		shown += 1
	_status.text = "%d tunable(s) — Entrée pour appliquer (live). * = surchargé." % shown

func _apply(value_str: String, nom: String) -> void:
	if Sim.world == null:
		return
	Sim.world.tune_set(nom, float(value_str))
	_rebuild()
