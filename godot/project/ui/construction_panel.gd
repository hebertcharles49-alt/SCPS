extends Control
## ConstructionPanel — les BOUTONS de construction. Lit la façade pour le PAYS joueur :
## `unit_roster` (nom·classe·coût·éthos·entretien·contres·recrutable) et `building_roster`
## (nom·matériaux·or·jours·débloqué) — display-only, prix RÉELS du moteur. Le clic émet
## `build_requested` (l'actionneur joueur viendra). Immediate-mode _draw ; le SURVOL
## donne le détail (le mockup : nom + éthos/entretien + efficace/faible contre).

const VKit = preload("res://ui/vkit.gd")

signal build_requested(kind: String, type: int)

const PW := 472
const PH := 484
const ROW := 15
const COLW := 214

var _units := []
var _builds := []
var _hover_zones := []     # [{rect, head, lines}]
var _click_zones := []     # [{rect, kind, type}]
var _has_hover := false
var _hover_rect := Rect2()
var _hover_head := ""
var _hover_lines := PackedStringArray()
var _hover_pos := Vector2.ZERO

func _ready() -> void:
	size = Vector2(PW, PH)
	custom_minimum_size = size
	mouse_filter = Control.MOUSE_FILTER_STOP
	Sim.generated.connect(_refresh)
	Sim.ticked.connect(func(_y): _refresh())
	if Sim.world != null:
		_refresh()

func _refresh() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	_units = Sim.world.unit_roster(me)
	_builds = Sim.world.building_roster(me)
	queue_redraw()

func _draw() -> void:
	_hover_zones.clear()
	_click_zones.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	VKit.text(self, Vector2(14, 8), VKit.COL_PARCH, "CONSTRUCTION", VKit.FS_BIG)
	if Sim.world != null:
		var ci: Dictionary = Sim.world.country_info(Sim.world.player())
		VKit.text(self, Vector2(176, 13), VKit.COL_DIM, String(ci.get("nom", "")))

	# ── UNITÉS (colonne gauche) — bouton = nom ; survol = le détail ──────────
	var xu := 14.0
	var y := VKit.section(self, xu, 30, "UNITÉS")
	for u in _units:
		var on: bool = bool(u.get("recrutable", false))
		var rect := Rect2(xu, y - 1, COLW, ROW)
		if _has_hover and _hover_rect == rect:
			VKit.fill(self, rect, VKit.COL_PANEL_HI)
		VKit.text(self, Vector2(xu + 5, y), (VKit.COL_PARCH if on else VKit.COL_DIM),
			String(u.get("nom", "")), VKit.FS_SMALL)
		if not on:
			VKit.text(self, Vector2(xu + COLW - 16, y), VKit.COL_COPPER, "✦", VKit.FS_SMALL)  # verrou tech
		_hover_zones.append({"rect": rect, "head": String(u.get("nom", "")), "lines": PackedStringArray([
			"%s · %s" % [u.get("classe", ""), u.get("arme", "")],
			"Coût : %s" % u.get("cout", ""),
			"Éthos : %s" % u.get("ethos", ""),
			"Entretien : %.1f or · %d vivre /100" % [float(u.get("entretien_or10", 5)) / 10.0, int(u.get("entretien_vivre", 1))],
			"Efficace contre ▸ %s" % u.get("fort", "—"),
			"Faible contre ▸ %s" % u.get("faible", "—"),
		])})
		if on:
			_click_zones.append({"rect": rect, "kind": "unit", "type": int(u.get("type", -1))})
		y += ROW

	# ── ÉDIFICES (colonne droite) — nom + matériaux compacts ; survol = prix complet ─
	var xb := 244.0
	var yb := VKit.section(self, xb, 30, "ÉDIFICES")
	for b in _builds:
		var on2: bool = bool(b.get("debloque", false))
		var rect2 := Rect2(xb, yb - 1, COLW, ROW)
		if _has_hover and _hover_rect == rect2:
			VKit.fill(self, rect2, VKit.COL_PANEL_HI)
		VKit.text(self, Vector2(xb + 5, yb), (VKit.COL_PARCH if on2 else VKit.COL_DIM),
			String(b.get("nom", "")), VKit.FS_SMALL)
		var cost: Array = b.get("cost", [])
		var cs := ""
		for c in cost:
			cs += "%s%d " % [String(c.get("res", "")).substr(0, 3), int(c.get("qty", 0))]
		VKit.text(self, Vector2(xb + 104, yb), VKit.COL_DIM, cs.strip_edges(), VKit.FS_SMALL)
		var lines := PackedStringArray()
		for c in cost:
			lines.append("%s : %d" % [c.get("res", ""), int(c.get("qty", 0))])
		lines.append("Or : %d   ·   %d jours" % [int(b.get("gold", 0)), int(b.get("days", 0))])
		_hover_zones.append({"rect": rect2, "head": String(b.get("nom", "")), "lines": lines})
		if on2:
			_click_zones.append({"rect": rect2, "kind": "build", "type": int(b.get("type", -1))})
		yb += ROW

	if _has_hover and _hover_lines.size() > 0:
		_draw_tooltip()

func _draw_tooltip() -> void:
	var w := VKit.text_w(_hover_head, VKit.FS)
	for ln in _hover_lines:
		w = maxf(w, VKit.text_w(ln, VKit.FS_SMALL))
	var bw := w + 18
	var bh := 24.0 + _hover_lines.size() * 14.0 + 6.0
	var px := minf(_hover_pos.x + 16, PW - bw - 6)
	var py := minf(_hover_pos.y + 2, PH - bh - 6)
	VKit.fill(self, Rect2(px, py, bw, bh), VKit.COL_PANEL2)
	VKit.box(self, Rect2(px, py, bw, bh), VKit.COL_COPPER)
	VKit.text(self, Vector2(px + 9, py + 5), VKit.COL_COPPER, _hover_head, VKit.FS)
	var yy := py + 24.0
	for ln in _hover_lines:
		VKit.text(self, Vector2(px + 9, yy), VKit.COL_PARCH, ln, VKit.FS_SMALL)
		yy += 14.0

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseMotion:
		var found := false
		for z in _hover_zones:
			if z["rect"].has_point(e.position):
				_has_hover = true
				_hover_rect = z["rect"]
				_hover_head = z["head"]
				_hover_lines = z["lines"]
				_hover_pos = e.position
				found = true
				break
		if not found:
			_has_hover = false
		queue_redraw()
	elif e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		for z in _click_zones:
			if z["rect"].has_point(e.position):
				build_requested.emit(z["kind"], z["type"])
				print("[construction] %s type=%d — actionneur joueur à venir" % [z["kind"], z["type"]])
				break
