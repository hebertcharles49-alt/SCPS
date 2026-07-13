extends Control
## ARMY PANEL — la barre de COMMANDEMENT du pion sélectionné. Statut de l'armée + TOUS les
## verbes déjà câblés : MARCHE/ATTAQUE (clic-destination sur la carte : à soi = repositionner,
## ennemie = siège → assaut → occupation & butin), RENFORCER, PILLER la côte,
## DISSOUDRE. Zéro logique sim : lit army_info/country_army, enfile des verbes journalisés.
## Montré/caché par map_view.army_selection_changed (main le câble).

const VKit = preload("res://ui/vkit.gd")

signal raid_requested   ## « Piller la côte » → main arme le sous-mode raid de la carte
signal selection_replaced(ids: Array) ## fusion : le corps survivant devient l'unique sélection

var _panel: PanelContainer
var _head: Label
var _hint: Label
var _preview_label: Label
var _stack_label: Label
var _refill_label: Label
var _corps_box: VBoxContainer
var _flash: Label
var _flash_ms := -100000
var _disband_btn: Button
var _split_btn: Button
var _merge_btn: Button
var _refill_btn: Button
var _disband_armed := false
var _disband_ms := -100000
var _selected_ids: Array[int] = []
var _move_preview: Dictionary = {}
var _refill_previews: Array[Dictionary] = []

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
	_preview_label = Label.new()
	_preview_label.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	_preview_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_preview_label.visible = false
	v.add_child(_preview_label)

	_corps_box = VBoxContainer.new()
	_corps_box.add_theme_constant_override("separation", 2)
	v.add_child(_corps_box)
	_stack_label = Label.new()
	_stack_label.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	_stack_label.add_theme_color_override("font_color", VKit.COL_GOLD)
	_stack_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_stack_label.visible = false
	v.add_child(_stack_label)
	_refill_label = Label.new()
	_refill_label.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	_refill_label.add_theme_color_override("font_color", VKit.COL_PARCH)
	_refill_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_refill_label.visible = false
	v.add_child(_refill_label)

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
	_refill_btn = Button.new()
	_refill_btn.text = "Renforcer"
	_refill_btn.custom_minimum_size = Vector2(0, 34)
	_refill_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_refill_btn.tooltip_text = "Mobilise une vague de renfort depuis une région nationale."
	_refill_btn.pressed.connect(_do_refill)
	ah.add_child(_refill_btn)
	var brd := Button.new()
	brd.text = "Piller la côte"
	brd.custom_minimum_size = Vector2(0, 34)
	brd.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	brd.tooltip_text = "Arme le pillage : cliquez ensuite une province côtière étrangère (nécessite une coque pirate)."
	brd.pressed.connect(_do_raid)
	ah.add_child(brd)
	_split_btn = Button.new()
	_split_btn.text = "Scinder"
	_split_btn.tooltip_text = "Sépare chaque corps sélectionné en deux détachements égaux."
	_split_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_split_btn.pressed.connect(_do_split)
	ah.add_child(_split_btn)
	_merge_btn = Button.new()
	_merge_btn.text = "Fusionner"
	_merge_btn.tooltip_text = "Fusionne les corps sélectionnés présents dans la même région."
	_merge_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_merge_btn.pressed.connect(_do_merge)
	ah.add_child(_merge_btn)
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
	_clear_corps_rows()
	var total:=0; var inf:=0; var arch:=0; var cav:=0; var mages:=0; var active:=0; var phase:="Réserve"
	var phases: Array[String] = []
	var regions: Array[int] = []
	var corps_data: Array[Dictionary] = []
	for id in _selected_ids:
		var a: Dictionary = Sim.world.corps_info(id) if Sim.world.has_method("corps_info") else Sim.world.army_info(me)
		if not bool(a.get("active",false)): continue
		if not bool(a.get("units_are_humans", false)):
			# Compatibilité avec la DLL debug précédente : elle exposait encore des paquets.
			for key in ["units", "inf", "arch", "cav", "mages"]:
				a[key] = int(a.get(key, 0)) * 100
		corps_data.append(a)
		active+=1; total+=int(a.get("units",0)); inf+=int(a.get("inf",0)); arch+=int(a.get("arch",0)); cav+=int(a.get("cav",0)); mages+=int(a.get("mages",0)); phase=String(a.get("phase","?"))
		if phase not in phases: phases.append(phase)
		var region := int(a.get("region", -1))
		if region not in regions: regions.append(region)
	if active>0:
		var compo := "%s inf · %s dist · %s cav · %s mages" % [_grp(inf),_grp(arch),_grp(cav),_grp(mages)]
		var phase_summary := phase if phases.size() == 1 else "%d phases" % phases.size()
		_head.text = "⚔ %d corps — %s · %s hommes" % [active,phase_summary,_grp(total)] if active>1 else "⚔ Votre corps — %s · %s hommes" % [phase,_grp(total)]
		_hint.text = "%s\nSélection conservée après l'ordre · clic droit pour annuler." % compo
		for i in range(mini(corps_data.size(), 6)):
			_add_corps_row(corps_data[i])
		if corps_data.size() > 6:
			var more := Label.new()
			more.text = "… et %d autres corps" % (corps_data.size() - 6)
			more.add_theme_font_size_override("font_size", VKit.FS_SMALL)
			more.add_theme_color_override("font_color", VKit.COL_DIM)
			_corps_box.add_child(more)
	else:
		var reg_n := 0
		if Sim.world.has_method("country_army"):
			reg_n = int(Sim.world.country_army(me).get("regiments", 0))
		_head.text = "⚔ Votre armée — réserve : %d régiment(s)" % reg_n
		_hint.text = "Cliquez une province : à vous → repositionner · ennemie → attaquer (siège, assaut, occupation & butin)."
	if _merge_btn != null:
		var merge_ok := active >= 2 and regions.size() == 1
		for merge_corps in corps_data:
			var merge_phase_id := int(merge_corps.get("phase_id", 0))
			if merge_phase_id == 3 or merge_phase_id >= 4: merge_ok = false
		_merge_btn.disabled = not merge_ok
		_merge_btn.tooltip_text = "Fusion possible : tous les corps sont au même endroit." if merge_ok else \
			"Fusion impossible : sélectionnez au moins deux corps co-localisés, hors bataille et hors mer."
	if _stack_label != null:
		_stack_label.text = _stack_summary_text(corps_data, regions, total)
		_stack_label.visible = _stack_label.text != ""
	if _split_btn != null:
		var split_ok := active > 0
		for split_corps in corps_data:
			var split_phase_id := int(split_corps.get("phase_id", 0))
			if int(split_corps.get("units", 0)) < 200 or split_phase_id == 3 or split_phase_id >= 4: split_ok = false
		_split_btn.disabled = not split_ok
	_refresh_refill()
	_refresh_disband()
	_layout.call_deferred()

func _clear_corps_rows() -> void:
	if _corps_box == null:
		return
	for child in _corps_box.get_children():
		_corps_box.remove_child(child)
		child.queue_free()

func _corps_status_text(a: Dictionary) -> String:
	var loc := String(a.get("location", ""))
	if loc == "": loc = "région %d" % int(a.get("region", -1))
	var phase := String(a.get("phase", "Inconnu"))
	var text := "Corps #%d · %s · %s · %s hommes" % [int(a.get("id", -1)), loc, phase, _grp(int(a.get("units", 0)))]
	var dest := String(a.get("destination", ""))
	if int(a.get("dest", -1)) >= 0:
		if dest == "": dest = "région %d" % int(a.get("dest", -1))
		text += " → %s" % dest
	var progress := int(a.get("progress_pct", -1))
	if progress >= 0:
		text += " · %d%% · %.0f j restants" % [progress, float(a.get("days_left", 0.0))]
	elif float(a.get("days_left", 0.0)) > 0.5:
		text += " · %.0f j" % float(a.get("days_left", 0.0))
	if int(a.get("broken_days", 0)) > 0:
		text += " · BRISÉ %d j" % int(a.get("broken_days", 0))
	if float(a.get("rally_days", 0.0)) > 0.5:
		text += " · ralliement %.0f j (%s hommes)" % [float(a.get("rally_days", 0.0)), _grp(int(a.get("rally_units", 0)))]
	return text

func _stack_summary_text(corps_data: Array[Dictionary], regions: Array[int], total: int) -> String:
	if corps_data.size() < 2:
		return ""
	if regions.size() != 1:
		return "Stack dispersé · %d corps dans %d régions · fusion impossible" % [corps_data.size(), regions.size()]
	for corps in corps_data:
		var phase_id := int(corps.get("phase_id", 0))
		if phase_id == 3 or phase_id >= 4:
			return "Stack · %d corps · %s hommes · fusion bloquée pendant bataille/mer" % [corps_data.size(), _grp(total)]
	return "Stack · %d corps · %s hommes · fusion → corps #%d (%s hommes)" % [
		corps_data.size(), _grp(total), int(corps_data[0].get("id", -1)), _grp(total)]

func _grp(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var count := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		count += 1
		if count % 3 == 0 and i > 0: out = " " + out
	return ("-" if n < 0 else "") + out

func _split_packets(humans: int) -> int:
	return maxi(0, humans / 200)

func _add_corps_row(a: Dictionary) -> void:
	var row := Label.new()
	row.text = _corps_status_text(a)
	row.tooltip_text = "%s\nCampagne : %d étape(s) · %d bataille(s) · %d région(s) réduite(s)." % [
		row.text, int(a.get("legs", 0)), int(a.get("battles", 0)), int(a.get("taken", 0))]
	row.add_theme_font_size_override("font_size", VKit.FS_SMALL)
	row.add_theme_color_override("font_color", VKit.COL_PARCH)
	row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_corps_box.add_child(row)

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

func show_feedback(message: String, good: bool) -> void:
	_say(message, good)
	_refresh()

func set_move_preview(preview: Dictionary) -> void:
	_move_preview = preview.duplicate(true)
	_refresh_move_preview()

func _move_preview_text(preview: Dictionary) -> String:
	if preview.is_empty():
		return ""
	var count := int(preview.get("corps_count", 0))
	var target := String(preview.get("target_name", ""))
	if target == "": target = "région %d" % int(preview.get("target_region", -1))
	if not bool(preview.get("valid", false)):
		return "Impossible vers %s · %s (%d/%d corps bloqués)" % [target,
			String(preview.get("reason", "route refusée")), int(preview.get("invalid_count", count)), count]
	var text := "Aperçu · %d corps → %s · ~%.0f j · %s" % [count, target,
		float(preview.get("travel_days", 0.0)), String(preview.get("arrival", "Déplacement"))]
	var start := int(preview.get("units_start", 0))
	var loss := int(preview.get("attrition_loss", 0))
	if start > 0:
		var arrival := int(preview.get("units_arrival", maxi(0, start - loss)))
		var pct := int(preview.get("attrition_pct", round(100.0 * float(loss) / float(start))))
		text += "\nArrivée projetée · %s → %s hommes" % [_grp(start), _grp(arrival)]
		if loss > 0:
			text += " · −%s en marche (%d%%)" % [_grp(loss), pct]
		else:
			text += " · aucune perte de marche projetée"
		var worst10 := int(preview.get("worst_daily_pct10", 0))
		if worst10 > 0:
			text += " · pire terrain %.1f%%/j" % (float(worst10) / 10.0)
	return text

func _refresh_move_preview() -> void:
	if _preview_label == null:
		return
	_preview_label.text = _move_preview_text(_move_preview)
	_preview_label.visible = _preview_label.text != ""
	var tone := 0.75
	if not bool(_move_preview.get("valid", false)):
		tone = 0.15
	elif int(_move_preview.get("attrition_pct", 0)) >= 10:
		tone = 0.20
	elif int(_move_preview.get("attrition_pct", 0)) >= 3:
		tone = 0.48
	_preview_label.add_theme_color_override("font_color", VKit.sense(tone))
	_layout.call_deferred()

func _refill_totals(previews: Array) -> Dictionary:
	var out := {"valid": 0, "allowed": 0, "requested": 0, "population": 0,
		"guaranteed_raw": 0, "guaranteed": 0, "weapons_needed": 0,
		"weapons_owned": 0, "reason": "Aucun corps sélectionné", "needs": {}}
	for raw in previews:
		var p: Dictionary = raw
		if not bool(p.get("valid", false)):
			continue
		out.valid += 1
		if bool(p.get("allowed", false)):
			out.allowed += 1
		else:
			if out.reason == "Aucun corps sélectionné": out.reason = String(p.get("reason", "Renfort indisponible"))
			continue
		out.requested += int(p.get("requested_humans", 0))
		out.population += int(p.get("population_ready_humans", 0))
		out.guaranteed_raw += int(p.get("guaranteed_humans", 0))
		out.weapons_needed += int(p.get("weapons_needed", 0))
		for raw_need in p.get("needs", []):
			var need: Dictionary = raw_need
			var key := str(int(need.get("resource", -1)))
			if not out.needs.has(key):
				out.needs[key] = {"name": String(need.get("name", "Armes")), "needed": 0, "owned": 0}
			var agg: Dictionary = out.needs[key]
			agg.needed += int(need.get("needed", 0))
			agg.owned = maxi(int(agg.owned), int(need.get("owned", 0)))
			out.needs[key] = agg
	var weapon_cover := 0
	for key in out.needs:
		var agg: Dictionary = out.needs[key]
		weapon_cover += mini(int(agg.needed), int(agg.owned))
	out.weapons_owned = weapon_cover
	var fortune_humans := maxi(0, int(out.requested) - int(out.weapons_needed))
	out.guaranteed = mini(int(out.guaranteed_raw), mini(int(out.population), fortune_humans + weapon_cover))
	if int(out.allowed) > 0: out.reason = ""
	return out

func _refill_summary_text(previews: Array) -> String:
	var t := _refill_totals(previews)
	if int(t.allowed) <= 0:
		return "Renfort indisponible · %s" % String(t.reason)
	var text := "Renfort · jusqu'à +%s hommes · %s garantis par vos stocks" % [
		_grp(int(t.requested)), _grp(int(t.guaranteed))]
	text += "\nCoût de la vague : %s hommes mobilisables · %s armes (%s nationales)" % [
		_grp(int(t.population)), _grp(int(t.weapons_needed)), _grp(int(t.weapons_owned))]
	if int(t.guaranteed) < int(t.population):
		text += " · marché sollicité pour les armes manquantes"
	if int(t.population) < int(t.requested):
		text += " · certaines classes sont épuisées"
	return text

func _refill_tooltip(previews: Array) -> String:
	var t := _refill_totals(previews)
	if int(t.allowed) <= 0:
		return String(t.reason)
	var lines: Array[String] = ["Une vague ajoute au plus 100 hommes par type d'unité présent."]
	for key in t.needs:
		var need: Dictionary = t.needs[key]
		lines.append("%s : %s en arsenal / %s requis" % [String(need.name), _grp(int(need.owned)), _grp(int(need.needed))])
	if int(t.guaranteed) < int(t.population):
		lines.append("Le manque peut être acheté au marché au prix et au trésor du prochain drain.")
	return "\n".join(lines)

func _refresh_refill() -> void:
	if _refill_btn == null or _refill_label == null:
		return
	_refill_previews.clear()
	if Sim.world == null or not Sim.world.has_method("corps_refill_preview"):
		_refill_label.visible = false
		_refill_btn.disabled = false
		_refill_btn.text = "Recompléter"
		_refill_btn.tooltip_text = "Mobilise une vague de renfort depuis une région nationale."
		return
	for id in _selected_ids:
		_refill_previews.append(Sim.world.corps_refill_preview(id))
	var totals := _refill_totals(_refill_previews)
	_refill_label.text = _refill_summary_text(_refill_previews)
	_refill_label.visible = not _refill_previews.is_empty()
	_refill_label.add_theme_color_override("font_color",
		VKit.sense(0.75) if int(totals.allowed) > 0 else VKit.sense(0.18))
	_refill_btn.disabled = int(totals.allowed) <= 0
	_refill_btn.text = "Renforcer (+%s)" % _grp(int(totals.requested)) if int(totals.allowed) > 0 else "Renforcer"
	_refill_btn.tooltip_text = _refill_tooltip(_refill_previews)

func _do_refill() -> void:
	if Sim.world != null and Sim.world.has_method("player_refill_corps"):
		var ok:=false
		for i in range(_selected_ids.size()):
			var allowed := true
			if i < _refill_previews.size(): allowed = bool(_refill_previews[i].get("allowed", false))
			if not allowed: continue
			ok = Sim.world.player_refill_corps(_selected_ids[i]) or ok
		if ok and _refill_previews.is_empty():
			_say("Recomplètement ordonné.", true) # ancienne DLL debug
		else:
			var totals := _refill_totals(_refill_previews)
			_say("Vague ordonnée · jusqu'à +%s hommes (%s garantis avant imports)." % [
				_grp(int(totals.requested)), _grp(int(totals.guaranteed))] if ok else "Rien à renforcer.", ok)

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
		# La membrane expose des HOMMES ; la commande moteur attend des paquets de 100.
		var humans := int(a.get("units", 0))
		if not bool(a.get("units_are_humans", false)): humans *= 100
		var half:=_split_packets(humans)
		if half>0: ok=Sim.world.player_split_corps(id,half) or ok
	_say("Scission ordonnée." if ok else "Scission impossible.",ok)

func _do_merge() -> void:
	if Sim.world == null or not Sim.world.has_method("player_merge_corps") or _selected_ids.size()<2:
		_say("Sélectionnez au moins deux corps au même endroit.",false); return
	var dst:=_selected_ids[0]; var ok:=false
	for i in range(1,_selected_ids.size()): ok=Sim.world.player_merge_corps(dst,_selected_ids[i]) or ok
	_say("Fusion ordonnée · le corps #%d conserve son identité." % dst if ok else "Les corps doivent être dans la même région.",ok)
	if ok:
		selection_replaced.emit([dst])
