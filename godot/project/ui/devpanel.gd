extends Control
## DevPanel — MODTOOLS : édite le registre des TUNABLES EN DIRECT (touche F10).
##
## Lit Sim.world.tunables() (nom · valeur · défaut · surchargé · phase) ; tune_set_checked applique la
## surcharge LIVE seulement si le moteur l'accepte (l'effet apparaît là où il relit tune_f).
## Dev-only :
## zéro logique sim, c'est de l'édition de coefficients que le moteur LIT déjà (discipline
## « l'effet passe par les entrées du moteur »). RÈGLE D'OR : GUI → façade (jamais les
## structs moteur). Un monde ainsi modé n'est plus rejouable vanilla (cf. SCPS_TUNE).

var _list: VBoxContainer
var _filter: LineEdit
var _status: Label
var _panel: PanelContainer
var _scroll: ScrollContainer

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_panel = PanelContainer.new()
	_panel.set_anchors_preset(Control.PRESET_TOP_LEFT)
	add_child(_panel)
	var vb := VBoxContainer.new()
	_panel.add_child(vb)
	var title := Label.new()
	title.text = "MODTOOLS — Tunables (édition LIVE · F10 pour fermer)"
	title.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vb.add_child(title)
	_filter = LineEdit.new()
	_filter.placeholder_text = "filtrer par nom…"
	_filter.text_changed.connect(func(_t): _rebuild())
	vb.add_child(_filter)
	var copy_btn := Button.new()
	copy_btn.text = "Copier les réglages"
	copy_btn.tooltip_text = "Copier les surcharges actives au format SCPS_TUNE."
	copy_btn.pressed.connect(_copy_overrides)
	vb.add_child(copy_btn)
	_scroll = ScrollContainer.new()
	_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vb.add_child(_scroll)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.add_child(_list)
	_status = Label.new()
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_status.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_status.custom_minimum_size = Vector2(0, 28)
	vb.add_child(_status)
	get_viewport().size_changed.connect(_layout)
	_layout()
	visibility_changed.connect(_on_visibility_changed)
	hide()

func _on_visibility_changed() -> void:
	if not visible:
		return
	_rebuild()
	# Les textes repliés ont besoin de leur largeur effective avant que le
	# conteneur connaisse sa hauteur minimale, surtout à la première ouverture.
	await get_tree().process_frame
	_layout()

func _layout() -> void:
	if _panel == null:
		return
	var viewport := get_viewport_rect().size
	var width := minf(580.0, maxf(520.0, viewport.x - 24.0))
	var height := minf(660.0, maxf(260.0, viewport.y - 24.0))
	_panel.custom_minimum_size = Vector2(width, height)
	_panel.size = Vector2(width, height)
	_panel.position = Vector2((viewport.x - width) * 0.5, (viewport.y - height) * 0.5)

func _phase_label(raw: String) -> String:
	match raw:
		"inactive": return "inactif"
		"diagnostic": return "diagnostic"
		"new_world": return "nouveau monde"
		"rule_read": return "lecture de la règle"
		"next_action": return "prochaine action"
	return raw if raw != "" else "règle moteur"

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
		var active := bool(t.get("active", true))
		var phase := _phase_label(String(t.get("phase", "règle moteur")))
		var row := HBoxContainer.new()
		row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		var lbl := Label.new()
		lbl.text = nom + ("  *" if bool(t["overridden"]) else "") + " · " + phase
		lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		lbl.custom_minimum_size = Vector2(0, 24)
		lbl.tooltip_text = lbl.text
		row.add_child(lbl)
		var ed := LineEdit.new()
		ed.text = str(t["value"])
		ed.editable = active
		ed.tooltip_text = "Phase moteur : " + phase
		ed.custom_minimum_size = Vector2(120, 0)
		ed.size_flags_horizontal = Control.SIZE_SHRINK_END
		if active:
			ed.text_submitted.connect(_apply.bind(nom))
		row.add_child(ed)
		if active and bool(t["overridden"]):
			var reset := Button.new()
			reset.text = "Défaut"
			reset.tooltip_text = "Revenir à la valeur par défaut du registre."
			reset.pressed.connect(_reset.bind(nom))
			row.add_child(reset)
		_list.add_child(row)
		shown += 1
	_status.text = "%d tunable(s) — Entrée pour appliquer (live). * = surchargé." % shown

func _apply(value_str: String, nom: String) -> void:
	if Sim.world == null:
		return
	var raw := value_str.strip_edges()
	if raw == "" or not raw.is_valid_float():
		_status.text = tr("T_TUNE_INVALID")
		return
	var value := float(raw)
	if is_nan(value) or is_inf(value):
		_status.text = tr("T_TUNE_INVALID")
		return
	var known := false
	for item in Sim.world.tunables():
		var t: Dictionary = item
		if String(t.get("nom", "")) == nom:
			known = true
			if not bool(t.get("active", true)):
				_status.text = tr("T_TUNE_INACTIVE") % nom
				return
			break
	if not known:
		_status.text = tr("T_TUNE_UNKNOWN") % nom
		return
	if not Sim.world.has_method("tune_set_checked"):
		_status.text = tr("T_TUNE_NO_CHECK")
		return
	if not bool(Sim.world.tune_set_checked(nom, value)):
		_status.text = tr("T_TUNE_REJECTED")
		return
	_rebuild()
	var accepted := value
	for item in Sim.world.tunables():
		var t: Dictionary = item
		if String(t.get("nom", "")) == nom:
			accepted = float(t.get("value", value))
			break
	var phase := "règle moteur"
	for item in Sim.world.tunables():
		var t: Dictionary = item
		if String(t.get("nom", "")) == nom and t.has("phase"):
			phase = _phase_label(String(t.get("phase", phase)))
			break
	_status.text = tr("T_TUNE_ACCEPTED") % [nom, str(accepted), phase]

func _reset(nom: String) -> void:
	if Sim.world == null or not Sim.world.has_method("tune_reset"):
		_status.text = tr("T_TUNE_RESET_NO_CHECK")
		return
	if not bool(Sim.world.tune_reset(nom)):
		_status.text = tr("T_TUNE_RESET_REJECTED")
		return
	_rebuild()
	_status.text = tr("T_TUNE_RESET_DONE") % nom

func _copy_overrides() -> void:
	if Sim.world == null or not Sim.world.has_method("tunables"):
		return
	var parts: Array[String] = []
	for raw in Sim.world.tunables():
		var t: Dictionary = raw
		if bool(t.get("active", true)) and bool(t.get("overridden", false)):
			parts.append("%s=%s" % [String(t.get("nom", "")), str(float(t.get("value", 0.0)))])
	var line := "SCPS_TUNE=\"%s\"" % ",".join(parts)
	DisplayServer.clipboard_set(line)
	_status.text = tr("T_TUNE_COPIED") % parts.size()
