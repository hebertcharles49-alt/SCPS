extends Control
## BattlePanel — W-GUERRE UI (lot B). Le clic sur un jeton d'armée (overlay.gd) qui
## s'affronte (phase Siège OU Bataille) ouvre ce panneau sobre : les DEUX camps
## (nom, effectif, composition inf/arch/cav/mages en barres), la PHASE (mot), les
## pertes de choc si exposées (bataille en cours) et le war_score du conflit. Motif
## province_panel/VKit (immediate draw). Lit scps_battle_info (scps_api) via le
## binding `battle_info(region)`. Ferme sur Échap (pile _close_topmost de main.gd)
## ou sur le ✕. RÈGLE D'OR : zéro logique de sim — lecture pure de la membrane.

const VKit = preload("res://ui/vkit.gd")
const PW := 320.0

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
	size = Vector2(PW, 260.0)
	position = Vector2((vp.x - PW) * 0.5, 90.0)

func open_region(region: int) -> void:
	_region = region
	visible = true
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
	y += 26

	var atk: int = int(bi.get("attacker", -1))
	var def: int = int(bi.get("defender", -1))
	var atk_name := _country_name(atk)
	var def_name := _country_name(def)

	# ── ATTAQUANT ───────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "ATTAQUANT")
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, atk_name)
	y += 18
	var atk_units := int(bi.get("atk_units", 0))
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "%s hommes" % _grp(atk_units))
	y += 16
	y = _compo_bar(x, y, rw, int(bi.get("atk_inf", 0)), int(bi.get("atk_arch", 0)),
		int(bi.get("atk_cav", 0)), int(bi.get("atk_mages", 0)))
	y += 6

	# ── DÉFENSEUR ───────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "DÉFENSEUR")
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, def_name)
	y += 18
	var def_units := int(bi.get("def_units", 0))
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "%s hommes" % _grp(def_units))
	y += 16
	y = _compo_bar(x, y, rw, int(bi.get("def_inf", 0)), int(bi.get("def_arch", 0)),
		int(bi.get("def_cav", 0)), int(bi.get("def_mages", 0)))
	y += 6

	# ── PERTES DE CHOC (si une bataille est ENGAGÉE — sinon rien à montrer) ──
	if bool(bi.get("in_battle", false)):
		y = VKit.section(self, x, y, "PERTES DE CHOC")
		var loss_atk: float = float(bi.get("loss_atk", 0.0))
		var loss_def: float = float(bi.get("loss_def", 0.0))
		y = VKit.row(self, x, y, "Attaquant", "%s hommes" % _grp(int(loss_atk * 100.0)), VKit.sense(0.15))
		y = VKit.row(self, x, y, "Défenseur", "%s hommes" % _grp(int(loss_def * 100.0)), VKit.sense(0.15))
		y += 4

	# ── WAR SCORE (point de vue de l'attaquant, la même jauge que la diplomatie) ──
	y = VKit.section(self, x, y, "SCORE DE GUERRE")
	var ws: float = float(bi.get("war_score", 0.0))
	var wsn := clampf((ws + 100.0) / 200.0, 0.0, 1.0)
	UIKit_bar(x, y, rw, 14.0, int(clampf(ws, -100.0, 100.0)))
	VKit.text(self, Vector2(x, y + 18), VKit.sense(wsn), "%+.0f (%s)" % [ws, atk_name])
	y += 40

func UIKit_bar(x: float, y: float, w: float, h: float, value_signed: int) -> void:
	# jauge SIGNÉE [-100..100] : même dégradé que VKit.gauge, recentrée sur 0.
	var v01 := clampi((value_signed + 100) / 2, 0, 100)
	VKit.gauge(self, x, y, w, h, v01)
	var midx := x + w * 0.5
	VKit.fill(self, Rect2(midx - 1, y - 2, 2, h + 4), VKit.COL_DIM)

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
			accept_event()
