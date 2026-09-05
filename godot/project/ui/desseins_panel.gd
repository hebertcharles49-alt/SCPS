extends Control
## Desseins — l'arbre d'ambitions du joueur, devant la façade moteur.
##
## Le panneau ne reconstruit aucune condition : il lit dessein_info() et enfile
## seal_dessein(). Les cibles sont des liens vers les objets déjà connus du shell.

const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")
const InfoRef = preload("res://ui/info_ref.gd")

const PW := 500.0
const PH := 610.0
const BRANCHE_SOL := 0

signal target_requested(request: Dictionary)

var _info: Dictionary = {}
var _cid := -1
var _selected_voie := 0
var _flash := ""
var _flash_ok := true
var _hits: Array = []

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, PH)
	size = Vector2(PW, PH)
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 48.0)
	add_to_group("draggable")
	get_viewport().size_changed.connect(_layout)
	_layout()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(_on_month_ticked)
	if Sim.has_signal("generated"):
		Sim.generated.connect(_on_generated)
	hide()

func _on_month_ticked(_year: int) -> void:
	if visible:
		refresh()

func _on_generated() -> void:
	_info = {}
	_flash = ""
	if visible:
		refresh()

func open() -> void:
	if not Sim.game_on or Sim.world == null:
		return
	visible = true
	_selected_voie = 0
	_flash = ""
	_layout()
	refresh()

func _layout() -> void:
	if get_viewport() == null:
		return
	var vp := get_viewport_rect().size
	var top := Frame.TOPBAR_H + 8.0
	var available := maxf(300.0, vp.y - top - Frame.BOTTOMBAR_H - 8.0)
	var factor := minf(1.0, available / PH)
	scale = Vector2(factor, factor)
	var visual_w := PW * factor
	position = Vector2(clampf(Frame.SIDEBAR_W + 14.0, 8.0, maxf(8.0, vp.x - visual_w - 8.0)), top)

func close_panel() -> void:
	visible = false
	_flash = ""

func refresh() -> void:
	if Sim.world == null or not Sim.world.has_method("dessein_info"):
		_info = {"active": false}
		queue_redraw()
		return
	_cid = int(Sim.world.player()) if Sim.world.has_method("player") else 0
	_info = Sim.world.dessein_info(_cid, BRANCHE_SOL)
	if not bool(_info.get("pivot", false)):
		_selected_voie = 0
	queue_redraw()

func _process(_delta: float) -> void:
	if visible and not Sim.game_on:
		hide()

func _draw() -> void:
	_hits.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	VKit.text(self, Vector2(22, 15), VKit.COL_PARCH, tr("T_DESS_TITLE"), VKit.FS_BIG)
	var close_r := Rect2(PW - 42.0, 12.0, 25.0, 25.0)
	VKit.fill(self, close_r, VKit.COL_PANEL2)
	VKit.box(self, close_r, VKit.COL_EDGE)
	VKit.text(self, Vector2(close_r.position.x + 7, close_r.position.y + 2), VKit.COL_PARCH, "×", VKit.FS)
	_hits.append({"rect": close_r, "act": "close"})
	VKit.fill(self, Rect2(18, 48, PW - 36, 1), VKit.COL_EDGE)

	if not bool(_info.get("active", false)):
		VKit.text_wrapped(self, Vector2(24, 80), VKit.COL_DIM,
			tr("T_DESS_UNAVAILABLE"), PW - 48, 4, VKit.FS)
		return
	if bool(_info.get("done", false)):
		VKit.text(self, Vector2(24, 78), VKit.COL_GOLD, tr("T_DESS_DONE"), VKit.FS_BIG)
		VKit.text_wrapped(self, Vector2(24, 112), VKit.COL_DIM,
			tr("T_DESS_DONE_HELP"), PW - 48, 4, VKit.FS)
		return

	var branch := String(_info.get("branche", tr("T_DESS_BRANCH")))
	var voie := String(_info.get("voie", ""))
	var branch_line := branch if voie == "" else "%s · %s" % [branch, voie]
	VKit.text(self, Vector2(24, 70), VKit.COL_GOLD, branch_line, VKit.FS)
	var rung := int(_info.get("rung", 0))
	var total := maxi(1, int(_info.get("rungs_total", 8)))
	VKit.text(self, Vector2(24, 98), VKit.COL_DIM,
		tr("T_DESS_PROGRESS") % [rung + 1, total], VKit.FS_SMALL)
	for i in range(total):
		var rr := Rect2(24 + i * 53, 124, 43, 7)
		VKit.fill(self, rr, VKit.COL_GOLD if i <= rung else VKit.COL_PANEL2)
		VKit.box(self, rr, VKit.COL_EDGE)

	var nom := String(_info.get("nom", ""))
	VKit.text(self, Vector2(24, 154), VKit.COL_PARCH, nom, VKit.FS_BIG)
	var y := 188.0
	VKit.text(self, Vector2(24, y), VKit.COL_GOLD, tr("T_DESS_OBJECTIVE"), VKit.FS_SMALL)
	y += 19.0
	y += VKit.text_wrapped(self, Vector2(24, y), VKit.COL_PARCH,
		String(_info.get("objectif", tr("T_DESS_WAITING"))), PW - 48, 3, VKit.FS)

	var cible := String(_info.get("cible", ""))
	if cible != "":
		y += 6.0
		var target_text := tr("T_DESS_TARGET") % cible
		var tw := VKit.text(self, Vector2(24, y), VKit.COL_VALUE, target_text, VKit.FS_SMALL)
		var target_r := Rect2(20, y - 3, minf(PW - 40, tw + 10), 22)
		VKit.box(self, target_r, VKit.COL_EDGE)
		_hits.append({"rect": target_r, "act": "target"})
		y += 26.0

	VKit.text(self, Vector2(24, y), VKit.COL_GOLD, tr("T_DESS_REWARD"), VKit.FS_SMALL)
	y += 19.0
	y += VKit.text_wrapped(self, Vector2(24, y), VKit.COL_VALUE,
		String(_info.get("recompense", "")), PW - 48, 2, VKit.FS)
	y += 8.0
	y += VKit.text_wrapped(self, Vector2(24, y), VKit.COL_DIM,
		String(_info.get("saveur", "")), PW - 48, 3, VKit.FS_SMALL)
	y += 12.0

	if bool(_info.get("pivot", false)):
		y = _draw_pivot(y)
	else:
		var ready := bool(_info.get("pret", false))
		var status := tr("T_DESS_READY") if ready else tr("T_DESS_WAITING")
		VKit.text(self, Vector2(24, y), VKit.sense(0.82 if ready else 0.45), status, VKit.FS)
		y += 28.0
		_draw_seal(Rect2(24, y, PW - 48, 36), ready, 0)

	if _flash != "":
		VKit.text_wrapped(self, Vector2(24, PH - 40),
			VKit.sense(0.82 if _flash_ok else 0.15), _flash, PW - 48, 2, VKit.FS_SMALL)

func _draw_pivot(y0: float) -> float:
	var y := y0
	VKit.text(self, Vector2(24, y), VKit.COL_GOLD, tr("T_DESS_CHOOSE"), VKit.FS)
	y += 25.0
	var labels := [String(_info.get("voie_a", tr("T_DESS_WAY_A"))), String(_info.get("voie_b", tr("T_DESS_WAY_B")))]
	var oks := [bool(_info.get("voie_a_ok", false)), bool(_info.get("voie_b_ok", false))]
	for i in 2:
		var voie_id := i + 1
		var r := Rect2(24, y, PW - 48, 29)
		var chosen := _selected_voie == voie_id
		VKit.fill(self, r, VKit.COL_GOLD if chosen else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		var label: String = ("● " if chosen else "○ ") + String(labels[i])
		VKit.text(self, Vector2(34, y + 3), VKit.COL_PARCH, label, VKit.FS_SMALL)
		VKit.text(self, Vector2(PW - 155, y + 3),
			VKit.sense(0.85 if oks[i] else 0.25),
			tr("T_DESS_PROOF") % (tr("T_DESS_PROOF_OK") if oks[i] else tr("T_DESS_PROOF_MISSING")), VKit.FS_SMALL)
		_hits.append({"rect": r, "act": "voie", "voie": voie_id, "ok": oks[i]})
		y += 35.0
	var cost := int(_info.get("pivot_cout", 0))
	VKit.text(self, Vector2(24, y), VKit.COL_DIM, tr("T_DESS_COST") % cost, VKit.FS_SMALL)
	y += 24.0
	_draw_seal(Rect2(24, y, PW - 48, 36), bool(_info.get("pret", false)) and _selected_voie > 0, _selected_voie)
	return y + 40.0

func _draw_seal(r: Rect2, enabled: bool, voie: int) -> void:
	VKit.fill(self, r, VKit.COL_GOLD if enabled else VKit.COL_PANEL2)
	VKit.box(self, r, VKit.COL_EDGE)
	var label := tr("T_DESS_SEAL") if enabled else tr("T_DESS_SEAL_LOCKED")
	VKit.text(self, Vector2(r.position.x + 12, r.position.y + 6),
		VKit.COL_PARCH if enabled else VKit.COL_DIM, label, VKit.FS)
	_hits.append({"rect": r, "act": "seal", "enabled": enabled, "voie": voie})

func _gui_input(e: InputEvent) -> void:
	if not (e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT):
		return
	for h in _hits:
		if not (h["rect"] as Rect2).has_point(e.position):
			continue
		var act := String(h["act"])
		if act == "close":
			close_panel()
		elif act == "target":
			var req := _target_request(String(_info.get("cible", "")))
			if not req.is_empty(): target_requested.emit(req)
		elif act == "voie":
			if bool(h.get("ok", false)):
				_selected_voie = int(h.get("voie", 0))
		elif act == "seal" and bool(h.get("enabled", false)):
			_seal(int(h.get("voie", 0)))
		accept_event()
		return

func _seal(voie: int) -> void:
	if Sim.world == null or not Sim.world.has_method("seal_dessein"):
		_flash_ok = false
		_flash = tr("T_DESS_NO_ENGINE")
		queue_redraw()
		return
	var ok := bool(Sim.world.seal_dessein(BRANCHE_SOL, int(_info.get("rung", 0)), voie))
	_flash_ok = ok
	_flash = tr("T_DESS_ORDER_SENT") if ok else tr("T_DESS_ORDER_REFUSED")
	if ok:
		Sim.notify_action()
	queue_redraw()

func _target_request(label: String) -> Dictionary:
	if Sim.world == null:
		return {}
	# La façade expose désormais les IDs moteur stables. Ils priment sur le
	# libellé (les noms peuvent changer avec la culture, la langue ou une fusion).
	var target_cid := int(_info.get("target_cid", -1))
	if target_cid >= 0:
		return InfoRef.request(InfoRef.make(InfoRef.COUNTRY, target_cid), "actions")
	var target_pid := int(_info.get("target_pid", -1))
	if target_pid >= 0:
		return InfoRef.request(InfoRef.make(InfoRef.PROVINCE, target_pid), "map", {"focus_map": true})
	var target_region := int(_info.get("target_region", -1))
	if target_region >= 0:
		return InfoRef.request(InfoRef.make(InfoRef.REGION, target_region), "map", {"focus_map": true})
	# Repli de compatibilité avec une DLL plus ancienne, avant publication des IDs.
	if label == "":
		return {}
	if Sim.world.has_method("country_count") and Sim.world.has_method("country_info"):
		for c in range(int(Sim.world.country_count())):
			var ci: Dictionary = Sim.world.country_info(c)
			if bool(ci.get("valide", false)) and String(ci.get("nom", "")) == label:
				return InfoRef.request(InfoRef.make(InfoRef.COUNTRY, c), "actions")
	if Sim.world.has_method("region_count"):
		for r in range(int(Sim.world.region_count())):
			var names := []
			if Sim.world.has_method("region_label"): names.append(String(Sim.world.region_label(r)))
			if Sim.world.has_method("region_city_name"): names.append(String(Sim.world.region_city_name(r)))
			if names.has(label):
				return InfoRef.request(InfoRef.make(InfoRef.REGION, r), "map", {"focus_map": true})
	return {}
