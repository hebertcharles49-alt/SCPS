extends Control
## ConstructionPanel — les BOUTONS de construction, en GRILLE DE TUILES (le sprite
## remplace le nom ; le survol dicte le nom + le détail — le motif demandé). Lit la
## façade pour le PAYS joueur : `unit_roster` (22) et `building_roster` (édifices).
## Tuile grisée + ✦ si verrouillée par la tech ; grisée SANS ✦ si illégale maintenant
## (or/matière/palier — `build_legal`, miroir read-only du drain, avec la raison au
## survol). Le clic émet `build_requested`. Immediate-mode _draw, prix RÉELS du moteur.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")

signal build_requested(kind: String, type: int)

const COLS := 6
const CELL := 48
const TILE := 44
const PADX := 12
const PW := PADX * 2 + COLS * CELL     # 312
const PH := 556

var _units := []
var _builds := []
var _blegal := {}          # type → {legal, reason} — miroir read-only du drain CMD_BUILD (lot M)
var _hover_zones := []     # [{rect, head, lines}]
var _click_zones := []     # [{rect, kind, type}]
var _has_hover := false
var _close_rect := Rect2()
var _hover_rect := Rect2()
var _hover_head := ""
var _hover_lines := PackedStringArray()
var _hover_pos := Vector2.ZERO
var _flash := ""           # retour de la dernière action (chantier mis / unité levée / refus)
var _flash_ok := true

func _ready() -> void:
	size = Vector2(PW, PH)
	custom_minimum_size = size
	mouse_filter = Control.MOUSE_FILTER_STOP
	Sim.generated.connect(_refresh)
	Sim.month_ticked.connect(func(_y): _refresh())   # ressources dispo : cadence mensuelle
	if Sim.world != null:
		_refresh()

func _refresh() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	_units = Sim.world.unit_roster(me)
	_builds = Sim.world.building_roster(me)
	# lot M — la LÉGALITÉ réelle (or/matière/palier, miroir du drain qui refusait en
	# silence) : rafraîchie au tick, consommée par _draw (griser) et _act (flash honnête).
	_blegal.clear()
	if Sim.world.has_method("build_legal"):
		for b in _builds:
			if bool(b.get("debloque", false)):
				var t := int(b.get("type", -1))
				_blegal[t] = Sim.world.build_legal(-1, t)
	queue_redraw()

## la raison du refus, en mot (reason de build_legal : 2 or · 3 matière · 4 tech de palier · 1 structurel)
func _reason_word(reason: int) -> String:
	match reason:
		2: return "or insuffisant"
		3: return "matière manquante"
		4: return "tech de palier manquante"
		_: return "indisponible ici (palier/déjà bâti)"

func _draw() -> void:
	_hover_zones.clear()
	_click_zones.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	VKit.text(self, Vector2(PADX, 8), VKit.COL_PARCH, "CONSTRUCTION", VKit.FS_BIG)

	# ✕ — tout panneau se ferme (Échap le ferme aussi via main)
	_close_rect = Rect2(PW - 26, 6, 20, 20)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 6, _close_rect.position.y + 3), VKit.COL_PARCH, "x")

	# ── UNITÉS (grille de tuiles) ──────────────────────────────────────────
	var y0 := VKit.section(self, PADX, 30, "UNITÉS")
	for i in range(_units.size()):
		var u: Dictionary = _units[i]
		var on: bool = bool(u.get("recrutable", false))
		var cell := Rect2(PADX + (i % COLS) * CELL, y0 + (i / COLS) * CELL, TILE, TILE)
		_tile(cell, UIKit.unit_sprite(int(u.get("type", -1))), String(u.get("nom", "")), on)
		_hover_zones.append({"rect": cell, "head": String(u.get("nom", "")), "lines": PackedStringArray([
			"%s · %s" % [u.get("classe", ""), u.get("arme", "")],
			"Coût : %s" % u.get("cout", ""),
			"Éthos : %s" % u.get("ethos", ""),
			"Entretien : %.1f or · %d vivre /100" % [float(u.get("entretien_or10", 5)) / 10.0, int(u.get("entretien_vivre", 1))],
			"Efficace contre ▸ %s" % u.get("fort", "—"),
			"Faible contre ▸ %s" % u.get("faible", "—"),
		])})
		if on:
			_click_zones.append({"rect": cell, "kind": "unit", "type": int(u.get("type", -1)), "nom": String(u.get("nom", ""))})
	var rows_u := int(ceil(_units.size() / float(COLS)))
	var yb := y0 + rows_u * CELL

	# ── ÉDIFICES (grille de tuiles) ────────────────────────────────────────
	var y1 := VKit.section(self, PADX, yb + 4, "ÉDIFICES")
	for i in range(_builds.size()):
		var b: Dictionary = _builds[i]
		var on2: bool = bool(b.get("debloque", false))
		var btype := int(b.get("type", -1))
		# lot M — au-delà de la TECH (debloque), la légalité RÉELLE du drain (or/matière/palier)
		var leg: Dictionary = _blegal.get(btype, {})
		var affordable: bool = bool(leg.get("legal", true)) if on2 else false
		var cell := Rect2(PADX + (i % COLS) * CELL, y1 + (i / COLS) * CELL, TILE, TILE)
		# ✦ réservé au verrou TECH ; le grisé sans ✦ = illégal MAINTENANT (or/matière/palier)
		_tile(cell, UIKit.building_sprite(btype), String(b.get("nom", "")), on2 and affordable, not on2)
		var lines := PackedStringArray()
		for c in b.get("cost", []):
			lines.append("%s : %d" % [c.get("res", ""), int(c.get("qty", 0))])
		lines.append("Or : %d   ·   %d jours" % [int(b.get("gold", 0)), int(b.get("days", 0))])
		if on2 and not affordable:
			lines.append("✗ %s" % _reason_word(int(leg.get("reason", 1))))
		_hover_zones.append({"rect": cell, "head": String(b.get("nom", "")), "lines": lines})
		if on2 and affordable:
			_click_zones.append({"rect": cell, "kind": "build", "type": btype, "nom": String(b.get("nom", ""))})

	if _flash != "":
		VKit.text(self, Vector2(PADX, PH - 18), (VKit.sense(1.0) if _flash_ok else VKit.sense(0.05)), _flash, VKit.FS_SMALL)
	if _has_hover and _hover_lines.size() > 0:
		_draw_tooltip()

## une tuile : le sprite (grisé si verrouillé), repli texte si l'asset manque, ✦ si
## verrouillé par la TECH (star ; le grisé sans ✦ = illégal maintenant), cadre or au survol.
func _tile(cell: Rect2, tex: Texture2D, name: String, enabled: bool, star: bool = true) -> void:
	if tex != null:
		draw_texture_rect(tex, cell, false, (Color.WHITE if enabled else Color(0.5, 0.5, 0.55, 0.65)))
	else:
		VKit.fill(self, cell, VKit.COL_PANEL2)
		VKit.box(self, cell, VKit.COL_EDGE)
		VKit.text(self, cell.position + Vector2(3, 15), (VKit.COL_PARCH if enabled else VKit.COL_DIM), name.substr(0, 7), VKit.FS_SMALL)
	if not enabled and star:
		VKit.text(self, cell.position + Vector2(TILE - 13, 1), VKit.COL_GOLD, "✦", VKit.FS_SMALL)
	if _has_hover and _hover_rect == cell:
		VKit.box(self, Rect2(cell.position - Vector2(1, 1), cell.size + Vector2(2, 2)), VKit.COL_GOLD)

func _draw_tooltip() -> void:
	var w := VKit.text_w(_hover_head, VKit.FS)
	for ln in _hover_lines:
		w = maxf(w, VKit.text_w(ln, VKit.FS_SMALL))
	var bw := w + 18
	var bh := 24.0 + _hover_lines.size() * 14.0 + 6.0
	var px := minf(_hover_pos.x + 16, PW - bw - 6)
	var py := minf(_hover_pos.y + 2, PH - bh - 6)
	VKit.fill(self, Rect2(px, py, bw, bh), VKit.COL_PANEL2)
	VKit.box(self, Rect2(px, py, bw, bh), VKit.COL_GOLD)
	VKit.text(self, Vector2(px + 9, py + 5), VKit.COL_GOLD, _hover_head, VKit.FS)
	var yy := py + 24.0
	for ln in _hover_lines:
		VKit.text(self, Vector2(px + 9, yy), VKit.COL_PARCH, ln, VKit.FS_SMALL)
		yy += 14.0

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(e.position):
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
			return
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
				_act(String(z["kind"]), int(z["type"]), String(z["nom"]))
				break

## le CLIC agit : on appelle l'actionneur joueur (façade) et on affiche le retour.
func _act(kind: String, type: int, nom: String) -> void:
	if Sim.world == null:
		return
	# Les ordres sont ENFILÉS (journal déterministe) : ils s'appliquent au prochain
	# tick (après agency_advance). En pause, l'ordre attend la reprise. Le retour
	# n'est donc que « mis en file », pas le verdict d'application (qui tombe au tick).
	# lot M — le drain refuse en SILENCE (or/matière) : on ne dit « ordre émis » que
	# si build_legal passe AU MOMENT DU CLIC ; sinon on nomme le refus.
	if kind == "build":
		if Sim.world.has_method("build_legal"):
			var bl: Dictionary = Sim.world.build_legal(-1, type)
			if not bool(bl.get("legal", true)):
				_flash_ok = false
				_flash = "✗ %s — %s" % [nom, _reason_word(int(bl.get("reason", 1)))]
				Sound.play("ui_click")
				_refresh()
				return
		var ok: bool = Sim.world.player_build(type, -1)
		_flash_ok = ok
		_flash = ("⚒ %s — ordre émis" % nom) if ok else ("✗ %s — file pleine" % nom)
	else:
		var ok2: bool = Sim.world.player_recruit(type) > 0
		_flash_ok = ok2
		_flash = ("⚔ %s — levée ordonnée" % nom) if ok2 else ("✗ %s — file pleine" % nom)
	if not _flash_ok:
		Sound.play("ui_click")
	build_requested.emit(kind, type)
	_refresh()
	Sim.notify_action()   # verbe joueur (bâtir / lever) → refresh des chiffres au drain (live)
