extends Control
## ProvinceDetail — le détail d'une province en SOUS-ONGLETS (touche V), read-only.
##   • Peuples    : camembert CULTURE + camembert RELIGION + classes + la jauge
##                  d'AGITATION et ses MODIFICATEURS (nom · apport signé · résorption
##                  /an pour les temporaires comme « Conquête récente »). Pas de prose.
##   • Production : les flux +X/j par bien (sprite de ressource sous la barre).
##   • Bâtiments  : les manufactures bâties (niveau + ouvriers).
## Pattern onglets + survol calqué sur sidebar_drawer. Charte bleu nuit / cuivre.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const PW := 648.0
const PH := 512.0
const HEAD := 34.0
const BODY := 92.0          # y de départ du corps d'onglet (sous titre + onglets)
const TABS := ["Peuples", "Production", "Bâtiments", "Journal"]

var _pid := -1
var _tab := 0
var _tab_rects := []        # [{rect, idx}] onglets cliquables
var _hover_zones := []      # [{rect, text}] survol des entrées de journal (effets)
var _hover_text := ""
var _hover_pos := Vector2.ZERO

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: queue_redraw())
	hide()

func _layout() -> void:
	var vp := get_viewport_rect().size
	position = Vector2((vp.x - PW) * 0.5, (vp.y - PH) * 0.5)

func show_province(pid: int) -> void:
	_pid = pid
	queue_redraw()

func _draw() -> void:
	var w = Sim.world
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	if w == null or _pid < 0:
		return
	var info: Dictionary = w.province_info(_pid)
	if not bool(info.get("valide", false)):
		VKit.text(self, Vector2(16, HEAD), VKit.COL_DIM, "(aucune province sélectionnée)", VKit.FS_SMALL)
		return
	_hover_zones.clear()
	var x := 16.0
	UIKit.draw_icon(self, "capital_tower", Vector2(14, 8), 18)
	VKit.text(self, Vector2(40, 9), VKit.COL_COPPER, "Province — %s" % String(info["nom"]), VKit.FS_BIG)
	VKit.text(self, Vector2(PW - 96, 11), VKit.COL_DIM, "[V] fermer", VKit.FS_SMALL)
	VKit.text(self, Vector2(x, HEAD + 4), VKit.COL_PARCH,
		"%s habitants · %s · %s" % [_grp(info["ames"]), info["climat"], info["relief"]], VKit.FS_SMALL)

	# ── onglets (chips cliquables) ─────────────────────────────────────────────
	_tab_rects.clear()
	var tx := x
	var ty := HEAD + 24.0
	for i in range(TABS.size()):
		var label: String = TABS[i]
		var tw := VKit.text_w(label, VKit.FS_SMALL) + 18.0
		var r := Rect2(tx, ty, tw, 20.0)
		var active := (_tab == i)
		VKit.fill(self, r, VKit.COL_COPPER if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		VKit.text(self, Vector2(tx + 9, ty + 2), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
		_tab_rects.append({"rect": r, "idx": i})
		tx += tw + 6
	VKit.fill(self, Rect2(12, BODY - 6, PW - 24, 1), VKit.COL_EDGE)

	match _tab:
		0: _draw_peuples(x, BODY, w)
		1: _draw_flux(x, BODY + 4.0, PW - 32.0, PH - BODY - 18.0, w)
		2: _draw_batiments(x, BODY, w)
		3: _draw_journal(x, BODY, w)

	# tooltip de survol (Journal : les effets de l'entrée)
	if _hover_text != "":
		var tw := VKit.text_w(_hover_text, VKit.FS_SMALL) + 14.0
		var tx2 := minf(_hover_pos.x + 14.0, PW - tw - 6.0)
		var ty2 := minf(maxf(4.0, _hover_pos.y - 22.0), PH - 22.0)
		VKit.fill(self, Rect2(tx2, ty2, tw, 18), VKit.COL_PANEL2)
		VKit.box(self, Rect2(tx2, ty2, tw, 18), VKit.COL_COPPER)
		VKit.text(self, Vector2(tx2 + 7, ty2 + 2), VKit.COL_PARCH, _hover_text, VKit.FS_SMALL)

# ── ONGLET PEUPLES : camemberts culture/religion + classes + agitation (hover) ──
func _draw_peuples(x: float, y: float, w) -> void:
	var groups: Array = w.province_groups(_pid)
	var cy := y + 44.0
	if groups.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "province inhabitée", VKit.FS_SMALL)
		return
	# camembert CULTURE (une part par groupe) + camembert RELIGION (dédupliqué)
	var cper := []
	var ccol := []
	for i in range(groups.size()):
		cper.append(groups[i]["percent"])
		ccol.append(VKit.SLICE_PAL[i % 8])
	var rnames := []
	var rper := []
	var rcol := []
	for g in groups:
		var idx: int = rnames.find(g["religion"])
		if idx < 0:
			rnames.append(g["religion"]); rper.append(g["percent"]); rcol.append(VKit.SLICE_PAL[(rnames.size() - 1) % 8])
		else:
			rper[idx] += g["percent"]
	var pr := 38.0
	VKit.pie(self, Vector2(x + pr + 10, cy), pr, cper, ccol)
	VKit.pie(self, Vector2(x + 3 * pr + 44, cy), pr, rper, rcol)
	VKit.text(self, Vector2(x + 2, cy + pr + 6), VKit.COL_DIM, "Culture", VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 2 * pr + 36, cy + pr + 6), VKit.COL_DIM, "Religion", VKit.FS_SMALL)
	# légende des cultures (nom + part) à droite des camemberts
	var lx := x + 4 * pr + 80
	var ly := cy - pr - 4.0
	for i in range(mini(groups.size(), 6)):
		VKit.fill(self, Rect2(lx, ly + 3, 9, 9), VKit.SLICE_PAL[i % 8])
		VKit.text(self, Vector2(lx + 15, ly), VKit.COL_PARCH,
			"%s %d%% · %s" % [String(groups[i]["culture"]), int(groups[i]["percent"]), String(groups[i]["etat"])], VKit.FS_SMALL)
		ly += 16

	# classes (barre empilée)
	var cls: Dictionary = w.province_classes(_pid)
	var ccnt := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var tot: float = maxf(1.0, ccnt[0] + ccnt[1] + ccnt[2])
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	var py := cy + pr + 28.0
	var rw := PW - 32.0
	var acc := 0.0
	for i in range(3):
		var segw: float = (rw - acc) if i == 2 else float(ccnt[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, py, segw, 14), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, py, rw, 14), VKit.COL_DIM)
	py += 19
	for i in range(3):
		VKit.fill(self, Rect2(x + i * 200.0, py + 3, 9, 9), cc[i])
		VKit.text(self, Vector2(x + i * 200.0 + 15, py), VKit.COL_PARCH, "%s %s" % [cnames[i], _grp(ccnt[i])], VKit.FS_SMALL)

	# ── AGITATION : la jauge + les MODIFICATEURS (nom · apport signé · résorption/an) ──
	py += 34
	var ag: Dictionary = w.province_agitation(_pid)
	var val := int(ag.get("value", 0))
	var acol := VKit.COL_DIM if val < 35 else (VKit.sense(0.45) if val < 70 else VKit.sense(0.10))
	UIKit.draw_icon(self, "menu_diplomacy", Vector2(x, py - 1), 16)
	VKit.text(self, Vector2(x + 22, py), VKit.COL_PARCH, "Agitation", VKit.FS_SMALL)
	UIKit.bar(self, Rect2(x + 100, py, 150, 14), val)
	VKit.text(self, Vector2(x + 258, py), acol, "%d / 100" % val, VKit.FS_SMALL)
	py += 22
	var causes: Array = ag.get("causes", [])
	if causes.is_empty():
		VKit.text(self, Vector2(x + 16, py), VKit.COL_DIM, "province paisible", VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(x + 16, py), VKit.COL_DIM, "Modificateurs", VKit.FS_SMALL)
		py += 16
		for c in causes:
			var d := int(c["delta"])
			var dec := int(c.get("decay", 0))
			var mcol := VKit.sense(0.16) if d > 0 else VKit.sense(0.78)   # soulève rouge / apaise vert
			VKit.text(self, Vector2(x + 24, py), VKit.COL_PARCH, String(c["cause"]), VKit.FS_SMALL)
			VKit.text(self, Vector2(x + 200, py), mcol, "%+d" % d, VKit.FS_SMALL)
			if dec > 0:
				VKit.text(self, Vector2(x + 250, py), VKit.COL_DIM, "−%d/an" % dec, VKit.FS_SMALL)
			py += 16

# ── ONGLET BÂTIMENTS : les manufactures bâties ─────────────────────────────────
func _draw_batiments(x: float, y: float, w) -> void:
	var blds: Array = w.province_buildings(_pid)
	if blds.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune manufacture bâtie (carte nue : l'IA/le joueur élèvent dans le temps)", VKit.FS_SMALL)
		return
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "manufacture                niveau   ouvriers", VKit.FS_SMALL)
	y += 18
	var maxlv := 1
	for b in blds:
		maxlv = maxi(maxlv, int(b["niveau"]))
	for b in blds:
		if y > PH - 24:
			break
		UIKit.draw_icon(self, "build_hammer", Vector2(x, y - 1), 14)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, String(b["nom"]), VKit.FS_SMALL)
		var lv := int(b["niveau"])
		VKit.fill(self, Rect2(x + 230, y + 2, 90.0 * float(lv) / float(maxlv), 10), VKit.COL_COPPER)
		VKit.text(self, Vector2(x + 326, y), VKit.COL_PARCH, str(lv), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 380, y), VKit.COL_DIM, _grp(b["ouvriers"]), VKit.FS_SMALL)
		y += 19

# ── ONGLET JOURNAL : le fil chronologique des évènements & modificateurs ───────
func _draw_journal(x: float, y: float, w) -> void:
	var entries: Array = w.province_log(_pid)
	if entries.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucun évènement notable jusqu'ici", VKit.FS_SMALL)
		return
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "an      évènement  (survol → effets)", VKit.FS_SMALL)
	y += 18
	for e in entries:
		if y > PH - 22:
			break
		var sign := int(e["sign"])
		var col := VKit.sense(0.16) if sign > 0 else (VKit.sense(0.78) if sign < 0 else VKit.COL_PARCH)
		var mark := "▲" if sign > 0 else ("▼" if sign < 0 else "·")
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "an %d" % int(e["year"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 58, y), col, "%s %s" % [mark, String(e["label"])], VKit.FS_SMALL)
		var ht := String(e.get("hover", ""))
		if ht != "":
			_hover_zones.append({"rect": Rect2(x, y - 1, PW - 32.0, 17.0), "text": ht})
		y += 17

# ── ONGLET PRODUCTION : flux +X/j par bien (sprite de ressource dessous) ───────
func _draw_flux(fx: float, fy: float, fw: float, fh: float, w) -> void:
	VKit.text(self, Vector2(fx, fy), VKit.COL_COPPER, "Production en direct (par an)", VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx + 200.0, fy + 2.0, 9, 9), VKit.COL_COPPER)
	VKit.text(self, Vector2(fx + 214.0, fy), VKit.COL_DIM, "ressource brute", VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx + 330.0, fy + 2.0, 9, 9), VKit.SLICE_PAL[7])
	VKit.text(self, Vector2(fx + 344.0, fy), VKit.COL_DIM, "sortie d'atelier", VKit.FS_SMALL)
	var inc: Array = w.province_income(_pid)
	if inc.is_empty():
		VKit.text(self, Vector2(fx, fy + 24.0), VKit.COL_DIM, "rien de notable", VKit.FS_SMALL)
		return
	var n := inc.size()
	var vals := []
	var maxv := 1.0
	for l in inc:
		var v := float(l["per_day"]) * 365.0
		vals.append(v)
		maxv = maxf(maxv, absf(v))
	var top := fy + 24.0
	var base := fy + fh - 26.0
	var barmax := base - top - 12.0
	for g in range(0, 3):
		var gy := base - float(g) / 2.0 * barmax
		VKit.fill(self, Rect2(fx, gy, fw, 1), VKit.COL_EDGE)
		VKit.text(self, Vector2(fx + fw - 44.0, gy - 12.0), VKit.COL_DIM, "%.0f" % (float(g) / 2.0 * maxv), VKit.FS_SMALL)
	var slot := fw / float(n)
	var bw := minf(36.0, slot * 0.62)
	for i in range(n):
		var cx := fx + (float(i) + 0.5) * slot
		var v: float = vals[i]
		var bhh: float = absf(v) / maxv * barmax
		var manuf := bool(inc[i]["manufactured"])
		var col := VKit.SLICE_PAL[7] if manuf else VKit.COL_COPPER
		VKit.fill(self, Rect2(cx - bw / 2.0, base - bhh, bw, bhh), col)
		var vs := "+%.0f" % v
		VKit.text(self, Vector2(cx - VKit.text_w(vs, VKit.FS_SMALL) / 2.0, base - bhh - 13.0), VKit.COL_PARCH, vs, VKit.FS_SMALL)
		var spr := UIKit.resource_sprite(int(inc[i].get("res_id", -1)), String(inc[i]["source"]))
		if spr != null:
			draw_texture_rect(spr, Rect2(cx - 10.0, base + 3.0, 20.0, 20.0), false)
		else:
			VKit.text(self, Vector2(cx - 14.0, base + 5.0), VKit.COL_DIM, String(inc[i]["source"]).substr(0, 5), VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx, base, fw, 1), VKit.COL_DIM)

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var h := ""
		for z in _hover_zones:
			if z.rect.has_point(event.position):
				h = z.text
				break
		if h != _hover_text:
			_hover_text = h
			_hover_pos = event.position
			queue_redraw()
		elif h != "":
			_hover_pos = event.position
			queue_redraw()
		return
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for t in _tab_rects:
			if t.rect.has_point(event.position):
				_tab = t.idx
				_hover_text = ""
				queue_redraw()
				accept_event()
				return

func _grp(n) -> String:
	var s := str(absi(int(n)))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" if int(n) < 0 else "") + out
