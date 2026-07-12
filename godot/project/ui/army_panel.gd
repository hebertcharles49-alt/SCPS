extends Control
## ARMY PANEL — la barre de COMMANDEMENT du pion sélectionné. Statut de l'armée + TOUS les
## verbes déjà câblés : MARCHE/ATTAQUE (clic-destination sur la carte : à soi = repositionner,
## ennemie = siège → assaut → occupation & butin), POSTURE, RECOMPLÉTER, PILLER la côte,
## DISSOUDRE. Zéro logique sim : lit army_info/country_army, enfile des verbes journalisés.
## Montré/caché par map_view.army_selection_changed (main le câble).

const VKit = preload("res://ui/vkit.gd")

signal raid_requested   ## « Piller la côte » → main arme le sous-mode raid de la carte

var _panel: PanelContainer
var _head: Label
var _hint: Label
var _flash: Label
var _flash_ms := -100000
var _disband_btn: Button
var _disband_armed := false
var _disband_ms := -100000

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # plein écran : laisse passer les clics carte ; seul le panneau STOP
	_build()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: _refresh())

func _process(_dt: float) -> void:
	if _disband_armed and Time.get_ticks_msec() - _disband_ms > 4000:
		_disband_armed = false
		if visible:
			_refresh_disband()
	if _flash != null and _flash.text != "" and Time.get_ticks_msec() - _flash_ms > 3000:
		_flash.text = ""

## appelé par main sur map_view.army_selection_changed(on)
func set_army(on: bool) -> void:
	visible = on
	if on:
		_disband_armed = false
		_refresh()

func _build() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_panel = PanelContainer.new()
	_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	_panel.custom_minimum_size = Vector2(400, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = VKit.COL_PANEL
	sb.border_color = VKit.COL_EDGE
	sb.set_border_width_all(1)
	sb.set_border_width(SIDE_TOP, 3)
	sb.border_color = VKit.COL_GOLD
	sb.set_corner_radius_all(3)
	sb.content_margin_left = 12 ; sb.content_margin_right = 12
	sb.content_margin_top = 10 ; sb.content_margin_bottom = 10
	_panel.add_theme_stylebox_override("panel", sb)
	add_child(_panel)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 6)
	_panel.add_child(v)

	_head = Label.new()
	_head.add_theme_font_size_override("font_size", VKit.FS_BIG)
	_head.add_theme_color_override("font_color", VKit.COL_GOLD)
	v.add_child(_head)

	_hint = Label.new()
	_hint.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	_hint.add_theme_color_override("font_color", VKit.COL_DIM)
	_hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_hint.custom_minimum_size = Vector2(376, 0)
	_hint.text = "Cliquez une province : à vous → repositionner · ennemie → attaquer (siège, assaut, occupation & butin)."
	v.add_child(_hint)

	# POSTURE
	var pl := Label.new()
	pl.text = "Posture"
	pl.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	pl.add_theme_color_override("font_color", VKit.COL_DIM)
	v.add_child(pl)
	var ph := HBoxContainer.new()
	ph.add_theme_constant_override("separation", 4)
	v.add_child(ph)
	var pnames := ["Prudente", "Standard", "Agressive"]
	for i in 3:
		var b := Button.new()
		b.text = pnames[i]
		b.custom_minimum_size = Vector2(0, 34)
		b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		var idx := i
		b.pressed.connect(func(): _do_posture(idx))
		ph.add_child(b)

	# ACTIONS
	var ah := HBoxContainer.new()
	ah.add_theme_constant_override("separation", 4)
	v.add_child(ah)
	var brf := Button.new()
	brf.text = "Recompléter"
	brf.custom_minimum_size = Vector2(0, 34)
	brf.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	brf.tooltip_text = "Comble les pertes de l'armée depuis les strates de la population."
	brf.pressed.connect(_do_refill)
	ah.add_child(brf)
	var brd := Button.new()
	brd.text = "Piller la côte"
	brd.custom_minimum_size = Vector2(0, 34)
	brd.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	brd.tooltip_text = "Arme le pillage : cliquez ensuite une province côtière étrangère (nécessite une coque pirate)."
	brd.pressed.connect(_do_raid)
	ah.add_child(brd)
	_disband_btn = Button.new()
	_disband_btn.custom_minimum_size = Vector2(0, 34)
	_disband_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_disband_btn.pressed.connect(_do_disband)
	ah.add_child(_disband_btn)

	_flash = Label.new()
	_flash.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	v.add_child(_flash)
	_refresh_disband()
	_layout()

func _layout() -> void:
	if _panel == null:
		return
	var vp := get_viewport_rect().size
	_panel.reset_size()
	var w: float = maxf(_panel.size.x, _panel.custom_minimum_size.x)
	var h: float = _panel.size.y
	_panel.position = Vector2((vp.x - w) * 0.5, vp.y - h - 96.0)

func _refresh() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	var a: Dictionary = Sim.world.army_info(me)
	if bool(a.get("active", false)):
		var compo := "%d inf · %d dist · %d cav · %d mages" % [
			int(a.get("inf", 0)), int(a.get("arch", 0)), int(a.get("cav", 0)), int(a.get("mages", 0))]
		_head.text = "⚔ Votre armée — %s · %d hommes" % [String(a.get("phase", "?")), int(a.get("units", 0))]
		_hint.text = "%s\nCliquez une province : à vous → repositionner · ennemie → attaquer (siège, assaut, occupation & butin)." % compo
	else:
		var reg_n := 0
		if Sim.world.has_method("country_army"):
			reg_n = int(Sim.world.country_army(me).get("regiments", 0))
		_head.text = "⚔ Votre armée — réserve : %d régiment(s)" % reg_n
		_hint.text = "Cliquez une province : à vous → repositionner · ennemie → attaquer (siège, assaut, occupation & butin)."
	_refresh_disband()
	_layout.call_deferred()

func _refresh_disband() -> void:
	if _disband_btn == null:
		return
	_disband_btn.text = "Confirmer ?" if _disband_armed else "Dissoudre"
	_disband_btn.add_theme_color_override("font_color",
		Color(0.92, 0.42, 0.36) if _disband_armed else Color(0.86, 0.55, 0.50))

func _say(msg: String, good: bool) -> void:
	_flash.text = msg
	_flash.add_theme_color_override("font_color", VKit.sense(0.80) if good else VKit.sense(0.20))
	_flash_ms = Time.get_ticks_msec()

func _do_posture(i: int) -> void:
	if Sim.world != null and Sim.world.has_method("player_posture"):
		Sim.world.player_posture(i)
		_say("Posture : %s." % ["prudente", "standard", "agressive"][i], true)

func _do_refill() -> void:
	if Sim.world != null and Sim.world.has_method("player_refill"):
		var ok: bool = Sim.world.player_refill()
		_say("Recomplètement ordonné." if ok else "Rien à recompléter.", ok)

func _do_raid() -> void:
	raid_requested.emit()   # main arme le sous-mode raid de la carte
	_say("Pillage armé — cliquez une province côtière étrangère.", true)

func _do_disband() -> void:
	if not _disband_armed:
		_disband_armed = true
		_disband_ms = Time.get_ticks_msec()
		_refresh_disband()
		return
	_disband_armed = false
	_refresh_disband()
	if Sim.world != null and Sim.world.has_method("player_disband"):
		var ok: bool = Sim.world.player_disband()
		_say("Armée dissoute." if ok else "Aucune armée à dissoudre.", ok)
