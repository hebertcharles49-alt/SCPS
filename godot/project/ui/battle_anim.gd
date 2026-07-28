extends Control
## BattleAnim — le tableau vivant du combat : deux FORMATIONS de formes (■ infanterie ·
## ▲ archers · ● cavalerie · ▬ mages) sur fond biomesque, qui chargent, s'entrechoquent
## et encaissent à CHAQUE CHOC réel (le compteur `chocs` de battle_info ; le jet ±15 %
## du moteur se LIT dans l'issue : deltas de pertes/cohésion — display-only, la membrane
## ne voit jamais le tirage lui-même). RENFORTS (bt_reinforce) : l'effectif peut MONTER
## en pleine bataille — les morts se déduisent des pertes CUMULÉES (loss_*, monotones),
## jamais du delta d'effectif ; le surplus réconcilié = des formes qui ENTRENT par le bord.
## Petit, représentatif, zéro logique de sim.

const UIKit = preload("res://ui/uikit.gd")
const VKit = preload("res://ui/vkit.gd")

const W := 318.0
const H := 120.0
const MAX_SHAPES := 22          # par camp — « petit, représentatif »
const ROWS := 5                 # rangs de la grille de formation
const COL_ATK := Color(0x8f/255.0, 0x52/255.0, 0x22/255.0)   # rouille (SLICE_PAL[0])
const COL_DEF := Color(0x35/255.0, 0x5f/255.0, 0x80/255.0)   # acier  (SLICE_PAL[5])

var _bg: Texture2D = null
var _shapes := []               # [{side,kind,slot:Vector2,pos:Vector2,alive,fade,arriving}]
var _per := [1.0, 1.0]          # hommes par forme (fixé au setup, par camp)
var _n0 := [0, 0]               # formes initiales par camp
var _units0 := [0.0, 0.0]       # effectif initial (hommes)
var _loss_seen := [0.0, 0.0]    # pertes cumulées déjà animées (hommes)
var _units_seen := [0.0, 0.0]
var _morale := [100, 100]
var _chocs_seen := 0
var _drift := 0.0               # dérive de la ligne de front (choc perdu = recul), ±24 px
var _phase := 0                 # 0 idle · 1 charge · 2 choc · 3 reflux
var _t := 0.0
var _loser := -1                # camp qui recule au reflux du choc courant
var _sparks := []               # [Vector2] étincelles du choc courant
var _rng := RandomNumberGenerator.new()

func _ready() -> void:
	custom_minimum_size = Vector2(W, H)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

## (ré)arme le tableau depuis un battle_info frais — appelé à l'ouverture du panneau.
func setup(bi: Dictionary) -> void:
	_rng.seed = int(bi.get("region", 0)) * 7919 + 13   # stable par bataille (display-only)
	_bg = UIKit.biome_painting(String(bi.get("relief", "")), String(bi.get("climat", "")))
	_shapes.clear()
	_chocs_seen = int(bi.get("chocs", 0))
	_drift = 0.0
	_phase = 0
	_t = 0.0
	var scale := 1 if bool(bi.get("units_are_humans", false)) else 100
	for side in range(2):
		var pfx := "atk_" if side == 0 else "def_"
		var comp: Array[int] = [int(bi.get(pfx + "inf", 0)) * scale, int(bi.get(pfx + "arch", 0)) * scale,
			int(bi.get(pfx + "cav", 0)) * scale, int(bi.get(pfx + "mages", 0)) * scale]
		var total: int = comp[0] + comp[1] + comp[2] + comp[3]
		_units0[side] = float(maxi(total, 1))
		_units_seen[side] = float(total)
		_loss_seen[side] = float(bi.get("loss_atk" if side == 0 else "loss_def", 0.0)) * 100.0
		_per[side] = maxf(1.0, ceilf(float(total) / float(MAX_SHAPES)))
		var n := 0
		# ordre des colonnes depuis le front : infanterie · archers · mages ; la
		# cavalerie flanque (rangs extrêmes). kind : 0 ■ · 1 ▲ · 2 ● · 3 ▬
		for k in [0, 1, 3]:
			var cnt := int(ceilf(float(comp[k]) / _per[side])) if comp[k] > 0 else 0
			for i in range(cnt):
				if n >= MAX_SHAPES: break
				_shapes.append(_mk_shape(side, k, n))
				n += 1
		var cav := int(ceilf(float(comp[2]) / _per[side])) if comp[2] > 0 else 0
		for i in range(mini(cav, 4)):
			if n >= MAX_SHAPES: break
			_shapes.append(_mk_shape(side, 2, n, true, i))
			n += 1
		_n0[side] = n
		_morale[side] = int(bi.get(pfx + "morale_pct", 100))
	queue_redraw()

## place la forme n dans la grille du camp (colonnes du front vers l'arrière).
func _mk_shape(side: int, kind: int, n: int, flank := false, fi := 0) -> Dictionary:
	var dir := 1.0 if side == 0 else -1.0
	var front := W * 0.5 - dir * 26.0
	var slot: Vector2
	if flank:
		# cavalerie : au-dessus/en dessous du bloc, près du front
		slot = Vector2(front - dir * (4.0 + 10.0 * float(fi / 2)),
			(20.0 if fi % 2 == 0 else H - 16.0))
	else:
		var col := n / ROWS
		var row := n % ROWS
		slot = Vector2(front - dir * float(col) * 11.0, H * 0.5 - float(ROWS - 1) * 5.0 + float(row) * 10.0)
	return {"side": side, "kind": kind, "slot": slot, "pos": slot,
		"alive": true, "fade": 1.0, "arriving": 0.0}

## FORMATION AU REPOS (panneau d'armée, « en bas, la formation ») : un seul camp, centré,
## sur le fond biomesque — aucun ennemi, aucun choc. comp en HOMMES : {inf,arch,cav,mages,
## relief,climat,region}. Le biome du lieu n'a pas de reader hors combat → défaut plaines.
func setup_parade(comp: Dictionary) -> void:
	var bi := {
		"region": int(comp.get("region", 0)), "units_are_humans": true, "chocs": 0,
		"relief": String(comp.get("relief", "")), "climat": String(comp.get("climat", "")),
		"atk_inf": int(comp.get("inf", 0)), "atk_arch": int(comp.get("arch", 0)),
		"atk_cav": int(comp.get("cav", 0)), "atk_mages": int(comp.get("mages", 0)),
		"atk_units": int(comp.get("inf", 0)) + int(comp.get("arch", 0)) + int(comp.get("cav", 0)) + int(comp.get("mages", 0)),
		"def_inf": 0, "def_arch": 0, "def_cav": 0, "def_mages": 0, "def_units": 0,
		"atk_morale_pct": 100, "def_morale_pct": 100, "loss_atk": 0.0, "loss_def": 0.0,
	}
	setup(bi)
	_drift = 34.0   # sans vis-à-vis, le bloc se recentre sur le tableau

## un tick de bataille : morts (delta de loss_*), renforts (réconciliation), choc (anim).
func on_tick(bi: Dictionary) -> void:
	var scale := 1 if bool(bi.get("units_are_humans", false)) else 100
	for side in range(2):
		var pfx := "atk_" if side == 0 else "def_"
		var loss := float(bi.get("loss_atk" if side == 0 else "loss_def", 0.0)) * 100.0
		var units := float(int(bi.get(pfx + "units", 0)) * scale)
		_morale[side] = int(bi.get(pfx + "morale_pct", 100))
		var dloss: float = loss - float(_loss_seen[side])
		if dloss > 0.0:
			_kill(side, int(floorf(loss / float(_per[side]))) - int(floorf(float(_loss_seen[side]) / float(_per[side]))))
		# RENFORT : effectif au-delà de (précédent − morts du tick) ⇒ des arrivants
		var expected: float = float(_units_seen[side]) - dloss
		var reinf: float = units - expected
		if reinf > float(_per[side]) * 0.5:
			_spawn(side, int(roundf(reinf / float(_per[side]))))
		_loss_seen[side] = loss
		_units_seen[side] = units
	var chocs := int(bi.get("chocs", 0))
	if chocs > _chocs_seen:
		_chocs_seen = chocs
		_start_clash()
	queue_redraw()

func _kill(side: int, n: int) -> void:
	# le front meurt d'abord : les vivants les plus proches du centre
	if n <= 0: return
	var alive := []
	for s in _shapes:
		if s["side"] == side and s["alive"]:
			alive.append(s)
	alive.sort_custom(func(a, b):
		return absf((a["slot"] as Vector2).x - W * 0.5) < absf((b["slot"] as Vector2).x - W * 0.5))
	for i in range(mini(n, alive.size() - 1)):   # jamais la dernière forme (le camp existe encore)
		alive[i]["alive"] = false

func _spawn(side: int, n: int) -> void:
	var dir := 1.0 if side == 0 else -1.0
	var idx := 0
	for s in _shapes:
		if s["side"] == side: idx += 1
	for i in range(n):
		if idx >= MAX_SHAPES: break
		var sh := _mk_shape(side, 0, idx)
		sh["pos"] = Vector2(-14.0 if side == 0 else W + 14.0, (sh["slot"] as Vector2).y)
		sh["arriving"] = 1.0
		_shapes.append(sh)
		idx += 1

func _start_clash() -> void:
	_phase = 1
	_t = 0.0
	# le perdant du choc = la plus grosse perte RELATIVE depuis le dernier choc
	var ra: float = float(_loss_seen[0]) / float(_units0[0])
	var rd: float = float(_loss_seen[1]) / float(_units0[1])
	_loser = 0 if ra > rd else 1
	_drift = clampf(_drift + (6.0 if _loser == 1 else -6.0), -24.0, 24.0)
	_sparks.clear()
	for i in range(5):
		_sparks.append(Vector2(W * 0.5 + _drift + _rng.randf_range(-8.0, 8.0),
			H * 0.5 + _rng.randf_range(-26.0, 26.0)))

func _process(dt: float) -> void:
	if not visible: return
	_t += dt
	match _phase:
		1: if _t > 0.45: _phase = 2; _t = 0.0   # charge → choc
		2: if _t > 0.60: _phase = 3; _t = 0.0   # choc → reflux
		3: if _t > 0.50: _phase = 0; _t = 0.0   # reflux → repos
	for s in _shapes:
		if s["arriving"] > 0.0:
			s["arriving"] = maxf(0.0, s["arriving"] - dt * 1.2)
			s["pos"] = (s["pos"] as Vector2).lerp(s["slot"] as Vector2, 1.0 - s["arriving"])
		if not s["alive"] and s["fade"] > 0.0:
			s["fade"] = maxf(0.0, s["fade"] - dt * 1.4)
	queue_redraw()

## avancée du camp vers le front selon la phase (charge/choc tiennent, reflux recule le perdant)
func _advance(side: int) -> float:
	var dir := 1.0 if side == 0 else -1.0
	var a := 0.0
	match _phase:
		1: a = 14.0 * minf(1.0, _t / 0.45)
		2: a = 14.0
		3: a = 14.0 * (1.0 - _t / 0.50) - ((10.0 * (_t / 0.50)) if side == _loser else 0.0)
	return dir * (a + _drift * dir)

func _draw() -> void:
	# fond biomesque, assombri pour que les encres portent
	if _bg != null:
		draw_texture_rect(_bg, Rect2(0, 0, W, H), false, Color(0.62, 0.60, 0.55))
	else:
		draw_rect(Rect2(0, 0, W, H), Color(0.72, 0.65, 0.47))
	draw_rect(Rect2(0, 0, W, H), VKit.COL_EDGE, false, 1.0)
	for s in _shapes:
		if s["fade"] <= 0.0: continue
		var side := int(s["side"])
		var col := COL_ATK if side == 0 else COL_DEF
		if not s["alive"]:
			col.a = s["fade"] * 0.5
		# tremblement : ampleur au moral (cohésion basse = formation qui flotte) + choc
		var amp := lerpf(0.5, 2.2, 1.0 - float(_morale[side]) / 100.0) + (2.6 if _phase == 2 else 0.0)
		var jit := Vector2(_rng.randf_range(-amp, amp), _rng.randf_range(-amp, amp)) if amp > 0.6 else Vector2.ZERO
		var p := (s["pos"] as Vector2) + Vector2(_advance(side), 0) + jit
		match int(s["kind"]):
			0: draw_rect(Rect2(p.x - 3.5, p.y - 3.5, 7, 7), col)                       # ■ infanterie
			1: draw_colored_polygon(PackedVector2Array([p + Vector2(0, -4.5),          # ▲ archers
				p + Vector2(4, 3.5), p + Vector2(-4, 3.5)]), col)
			2: draw_circle(p, 4.0, col)                                                # ● cavalerie
			3: draw_rect(Rect2(p.x - 5.0, p.y - 2.5, 10, 5), col)                      # ▬ mages
	if _phase == 2:
		for sp in _sparks:
			var c := Color(0.92, 0.84, 0.55, 0.9 * (1.0 - _t / 0.60))
			draw_line(sp + Vector2(-3, -3), sp + Vector2(3, 3), c, 1.5)
			draw_line(sp + Vector2(-3, 3), sp + Vector2(3, -3), c, 1.5)
		VKit.text(self, Vector2(W * 0.5 - 22 + _drift, 4), Color(0.25, 0.18, 0.08), "choc %d" % _chocs_seen, VKit.FS_SMALL)