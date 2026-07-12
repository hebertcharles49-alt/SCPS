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
var _selected_ids: Array[int] = []

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
func set_army(ids: Array) -> void:
	_selected_ids.clear()
	for id in ids: _selected_ids.append(int(id))
	visible = not _selected_ids.is_empty()
	if visible:
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

	# (POSTURE retirée — retour joueur : feature sans intérêt, jamais demandée.)

	# ACTIONS
	var ah := HBoxContainer.new()
	ah.add_theme_constant_override("separation", 4)
	v.add_child(ah)
	var bra := Button.new()
	bra.text = "Lever un corps"
	bra.tooltip_text = "Détache la moitié de la réserve nationale à la capitale."
	bra.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bra.pressed.connect(_do_raise)
	ah.add_child(bra)
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
	var bsp := Button.new()
	bsp.text = "Scinder"
	bsp.tooltip_text = "Sépare chaque corps sélectionné en deux détachements égaux."
	bsp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bsp.pressed.connect(_do_split)
	ah.add_child(bsp)
	var bmg := Button.new()
	bmg.text = "Fusionner"
	bmg.tooltip_text = "Fusionne les corps sélectionnés présents dans la même région."
	bmg.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bmg.pressed.connect(_do_merge)
	ah.add_child(bmg)
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
	var total:=0; var inf:=0; var arch:=0; var cav:=0; var mages:=0; var active:=0; var phase:="Réserve"
	for id in _selected_ids:
		var a: Dictionary = Sim.world.corps_info(id) if Sim.world.has_method("corps_info") else Sim.world.army_info(me)
		if not bool(a.get("active",false)): continue
		active+=1; total+=int(a.get("units",0)); inf+=int(a.get("inf",0)); arch+=int(a.get("arch",0)); cav+=int(a.get("cav",0)); mages+=int(a.get("mages",0)); phase=String(a.get("phase","?"))
	if active>0:
		var compo := "%d inf · %d dist · %d cav · %d mages" % [inf,arch,cav,mages]
		_head.text = "⚔ %d corps — %s · %d hommes" % [active,phase,total] if active>1 else "⚔ Votre corps — %s · %d hommes" % [phase,total]
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

func _do_refill() -> void:
	if Sim.world != null and Sim.world.has_method("player_refill_corps"):
		var ok:=false
		for id in _selected_ids: ok = Sim.world.player_refill_corps(id) or ok
		_say("Recomplètement ordonné." if ok else "Rien à recompléter.", ok)

func _do_raise() -> void:
	if Sim.world == null or not Sim.world.has_method("player_raise_corps"): return
	var me:=int(Sim.world.player())
	var reserve:=int(Sim.world.country_army(me).get("regiments",0))
	var capital:=int(Sim.world.country_capital_region(me)) if Sim.world.has_method("country_capital_region") else -1
	var packets:=maxi(1,int(reserve/2))
	var ok:bool=reserve>0 and capital>=0 and Sim.world.player_raise_corps(packets,capital)
	_say("Nouveau corps levé à la capitale." if ok else "Réserve insuffisante.",ok)

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
	if Sim.world != null and Sim.world.has_method("player_disband_corps"):
		var ok:=false
		for id in _selected_ids: ok = Sim.world.player_disband_corps(id) or ok
		_say("Armée dissoute." if ok else "Aucune armée à dissoudre.", ok)

func _do_split() -> void:
	if Sim.world == null or not Sim.world.has_method("player_split_corps"): return
	var ok:=false
	for id in _selected_ids:
		var a: Dictionary=Sim.world.corps_info(id)
		var half:=int(a.get("units",0))/2
		if half>0: ok=Sim.world.player_split_corps(id,half) or ok
	_say("Scission ordonnée." if ok else "Scission impossible.",ok)

func _do_merge() -> void:
	if Sim.world == null or not Sim.world.has_method("player_merge_corps") or _selected_ids.size()<2:
		_say("Sélectionnez au moins deux corps au même endroit.",false); return
	var dst:=_selected_ids[0]; var ok:=false
	for i in range(1,_selected_ids.size()): ok=Sim.world.player_merge_corps(dst,_selected_ids[i]) or ok
	_say("Fusion ordonnée." if ok else "Les corps doivent être dans la même région.",ok)
