extends Control
## BattlePanel — W-GUERRE UI (lot B). Le clic sur un jeton d'armée (overlay.gd) qui
## s'affronte (phase Siège OU Bataille) ouvre ce panneau sobre : les DEUX camps
## (nom, effectif, composition inf/arch/cav/mages en barres), la PHASE (mot), les
## cohésion live (bataille en cours). Le score du conflit vit dans le ledger droit. Motif
## province_panel/VKit (immediate draw). Lit scps_battle_info (scps_api) via le
## binding `battle_info(region)`. Ferme sur Échap (pile _close_topmost de main.gd)
## ou sur le ✕. RÈGLE D'OR : zéro logique de sim — lecture pure de la membrane.

const VKit = preload("res://ui/vkit.gd")
const Frame = preload("res://ui/frame.gd")
const PW := 350.0

signal close_requested

var _region := -1
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(_on_tick)
	visible = false

func _layout() -> void:
	var vp := get_viewport_rect().size
	size = Vector2(PW, 540.0)
	position = Vector2(maxf(Frame.SIDEBAR_W + 14.0,
		vp.x - Frame.LEDGER_W - PW - 12.0), Frame.TOPBAR_H + 12.0)

func open_region(region: int) -> void:
	_region = region
	visible = true
	Sound.play("moment_battle_drums")
	queue_redraw()

func _on_tick(_year: int) -> void:
	if visible:
		# le combat peut se conclure entre deux ticks (siège levé, bataille tranchée) —
		# on referme tout seul si la donnée n'est plus valide (rien à montrer).
		var w = Sim.world
		if w == null or not w.has_method("battle_info"):
			visible = false
			return
		var bi: Dictionary = w.battle_info(_region)
		if not bool(bi.get("valid", false)):
			visible = false
			return
		queue_redraw()

## une barre de composition (inf/arch/cav/mages) empilée — même langage que
## province_panel (barre empilée de classes), réutilisant SLICE_PAL.
func _compo_bar(x: float, y: float, w: float, inf: int, arch: int, cav: int, mages: int) -> float:
	var tot: float = maxf(1.0, float(inf + arch + cav + mages))
	var vals := [inf, arch, cav, mages]
	var cols := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3], VKit.SLICE_PAL[5]]
	var bh := 12.0
	var acc := 0.0
	for i in range(4):
		var segw: float = (w - acc) if i == 3 else float(vals[i]) / tot * w
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, y, segw, bh), cols[i])
		acc += segw
	VKit.box(self, Rect2(x, y, w, bh), VKit.COL_DIM)
	return y + bh + 4.0

func _draw() -> void:
	var w = Sim.world
	if w == null or _region < 0:
		return
	if not w.has_method("battle_info"):
		return
	var bi: Dictionary = w.battle_info(_region)
	if not bool(bi.get("valid", false)):
		return
	var unit_scale := 1 if bool(bi.get("units_are_humans", false)) else 100

	var ph := size.y
	var rw := PW - 30.0
	VKit.panel_bg(self, Rect2(0, 0, PW, ph))
	VKit.fill(self, Rect2(PW - 2, 0, 2, ph), VKit.COL_GOLD)
	var x := 16.0
	var y := 14.0

	# ── EN-TÊTE : phase du combat ──────────────────────────────────────────
	var phase_word: String = String(bi.get("phase", ""))
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, phase_word, VKit.FS_BIG)
	_close_rect = Rect2(PW - 20, 3, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	y += 24
	if bool(bi.get("in_battle", false)) and bi.has("days"):
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Jour %d · %d choc(s) livré(s)" % [
			int(bi.get("days", 0)), int(bi.get("chocs", 0))], VKit.FS_SMALL)
		y += 18
	elif bi.has("siege_days_left"):
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "%.0f j restants · %d%% estimés" % [
			float(bi.get("siege_days_left", 0.0)), int(bi.get("siege_progress_pct", 0))], VKit.FS_SMALL)
		y += 18

	var atk: int = int(bi.get("attacker", -1))
	var def: int = int(bi.get("defender", -1))
	var atk_name := _country_name(atk)
	var def_name := _country_name(def)

	# ── ATTAQUANT ───────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "ATTAQUANT")
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, atk_name)
	y += 18
	var atk_units := int(bi.get("atk_units", 0)) * unit_scale
	var atk_corps := int(bi.get("atk_corps", 1 if atk_units > 0 else 0))
	VKit.value(self, Vector2(x, y), "%s hommes · %d corps" % [_grp(atk_units), atk_corps])
	y += 16
	if atk_units > 0:
		y = _compo_bar(x, y, rw, int(bi.get("atk_inf", 0)) * unit_scale, int(bi.get("atk_arch", 0)) * unit_scale,
			int(bi.get("atk_cav", 0)) * unit_scale, int(bi.get("atk_mages", 0)) * unit_scale)
	if bool(bi.get("in_battle", false)) and bi.has("atk_morale_pct"):
		var amp := int(bi.get("atk_morale_pct", 0))
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Cohésion %d%%" % amp, VKit.FS_SMALL)
		VKit.gauge(self, x + 92, y + 1, rw - 92, 9, amp)
		y += 15
	var atk_helper := int(bi.get("atk_helper", -1))
	if atk_helper >= 0:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Renfort : %s" % _country_name(atk_helper), VKit.FS_SMALL)
		y += 15
	y += 6

	# ── DÉFENSEUR ───────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "DÉFENSEUR")
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, def_name)
	y += 18
	var def_units := int(bi.get("def_units", 0)) * unit_scale
	if def_units > 0:
		var def_corps := int(bi.get("def_corps", 1))
		VKit.value(self, Vector2(x, y), "%s hommes · %d corps" % [_grp(def_units), def_corps])
	else:
		VKit.value(self, Vector2(x, y), _siege_strength_text(bi))
	y += 16
	if def_units > 0:
		y = _compo_bar(x, y, rw, int(bi.get("def_inf", 0)) * unit_scale, int(bi.get("def_arch", 0)) * unit_scale,
			int(bi.get("def_cav", 0)) * unit_scale, int(bi.get("def_mages", 0)) * unit_scale)
	if bool(bi.get("in_battle", false)) and bi.has("def_morale_pct"):
		var dmp := int(bi.get("def_morale_pct", 0))
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Cohésion %d%%" % dmp, VKit.FS_SMALL)
		VKit.gauge(self, x + 92, y + 1, rw - 92, 9, dmp)
		y += 15
	var def_helper := int(bi.get("def_helper", -1))
	if def_helper >= 0:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Renfort : %s" % _country_name(def_helper), VKit.FS_SMALL)
		y += 15
	y += 6

	# ── SIÈGE : compte à rebours exact + causes de la résistance ───────────
	if not bool(bi.get("in_battle", false)) and bi.has("siege_days_left"):
		y = VKit.section(self, x, y, "LECTURE DU SIÈGE")
		var sp := clampi(int(bi.get("siege_progress_pct", 0)), 0, 100)
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Progression estimée %d%%" % sp, VKit.FS_SMALL)
		VKit.gauge(self, x + 132, y + 1, rw - 132, 9, sp)
		y += 16
		y = VKit.row(self, x, y, "Échéance", _siege_duration_text(bi), VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Ouvrages", "défense %.1f" % float(bi.get("siege_defense", 0.0)), VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Vivres", "%.1f mois" % float(bi.get("siege_food_months", 0.0)), VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Terrain", _siege_terrain_text(int(bi.get("siege_terrain_pct", 100))), VKit.COL_PARCH)
		y = VKit.row(self, x, y, "À la chute", _siege_outcome_text(bi), VKit.COL_GOLD)
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Estimation recalculée si la place ou ses vivres évoluent.", VKit.FS_SMALL)
		y += 19

	# ── POURQUOI UN CAMP PREND L'AVANTAGE : facteurs EXACTS du prochain choc ──
	if bool(bi.get("in_battle", false)) and bi.has("stage"):
		y = VKit.section(self, x, y, "LECTURE TACTIQUE")
		y = VKit.row(self, x, y, "Phase", String(bi.get("stage", "—")), VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Terrain", _terrain_text(bi, atk, def), VKit.COL_PARCH)
		if bool(bi.get("river", false)):
			y = VKit.row(self, x, y, "Rivière", "pontée" if bool(bi.get("bridged", false)) else "non pontée", VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Contres", _counter_text(bi), VKit.COL_PARCH)
		var bal := int(bi.get("balance_atk_pct", 50))
		y = VKit.row(self, x, y, "Rapport pré-aléa", "%d / %d (±15%% au choc)" % [bal, 100 - bal], VKit.COL_PARCH)
		y = VKit.row(self, x, y, "Rupture", "sous %d%% de cohésion" % int(bi.get("rupture_pct", 0)), VKit.COL_DIM)
		y += 4
		y = VKit.section(self, x, y, "PERTES CONFIRMÉES")
		y = VKit.row(self, x, y, "Attaquant", "%s hommes" % _grp(int(float(bi.get("loss_atk", 0.0)) * 100.0)), VKit.sense(0.15))
		y = VKit.row(self, x, y, "Défenseur", "%s hommes" % _grp(int(float(bi.get("loss_def", 0.0)) * 100.0)), VKit.sense(0.15))
		y += 4

func _signed_pct(mult_pct: int) -> String:
	return "%+d%%" % (mult_pct - 100)

func _terrain_text(bi: Dictionary, attacker: int, defender: int) -> String:
	var holder := int(bi.get("terrain_holder", -1))
	if holder == attacker:
		return "avantage attaquant %s" % _signed_pct(int(bi.get("atk_terrain_pct", 100)))
	if holder == defender:
		return "avantage défenseur %s" % _signed_pct(int(bi.get("def_terrain_pct", 100)))
	return "neutre"

func _counter_text(bi: Dictionary) -> String:
	return "attaquant %s · défenseur %s" % [
		_signed_pct(int(bi.get("atk_counter_pct", 100))),
		_signed_pct(int(bi.get("def_counter_pct", 100)))]

func _siege_strength_text(bi: Dictionary) -> String:
	if not bi.has("siege_defense"):
		return "Place forte · défense par ouvrages et vivres"
	var defense := float(bi.get("siege_defense", 0.0))
	if defense <= 0.01:
		return "Place ouverte · aucune fortification"
	return "Place forte · défense %.1f · %.1f mois de vivres" % [
		defense, float(bi.get("siege_food_months", 0.0))]

func _siege_duration_text(bi: Dictionary) -> String:
	return "%.0f j restants / %.0f j de résistance actuelle" % [
		float(bi.get("siege_days_left", 0.0)), float(bi.get("siege_full_days", 0.0))]

func _siege_terrain_text(mult_pct: int) -> String:
	if mult_pct == 100:
		return "tenue neutre"
	return "%+d%% de tenue" % (mult_pct - 100)

func _siege_outcome_text(bi: Dictionary) -> String:
	return "libération de la région" if int(bi.get("siege_outcome", 0)) == 1 else "occupation de la région"

func _country_name(cid: int) -> String:
	if cid < 0 or Sim.world == null:
		return "—"
	var info: Dictionary = Sim.world.country_info(cid)
	return String(info.get("nom", "—"))

func _grp(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" if n < 0 else "") + out

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(event.position):
			close_requested.emit()
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
