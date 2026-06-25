extends Control
## ProvinceDetail — le DÉTAIL graphique d'une province (read-only), bascule touche V
## (montre la province sélectionnée). Donne de l'AIR aux métriques que la bande
## étroite de gauche ne fait que résumer : un GRAPHE de barres des FLUX (production
## + ressources, /an) où chaque ressource est NOMMÉE PAR SON SPRITE sous la barre ;
## les CAMEMBERTS culture & idéologie (VKit, réutilisés) ; la barre des CLASSES.
## Tout en immédiat VKit (catégoriel + sprites → la charte tient, sans EC qui cale
## sur un axe X de chaînes ; Easy Charts reste le moteur des SÉRIES temporelles).
## Charte bleu nuit / cuivre. Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const PW := 648.0
const PH := 512.0
const HEAD := 34.0

var _pid := -1

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
	var x := 16.0
	UIKit.draw_icon(self, "capital_tower", Vector2(14, 8), 18)
	VKit.text(self, Vector2(40, 9), VKit.COL_COPPER, "Province — %s" % String(info["nom"]), VKit.FS_BIG)
	VKit.text(self, Vector2(PW - 96, 11), VKit.COL_DIM, "[V] fermer", VKit.FS_SMALL)
	VKit.text(self, Vector2(x, HEAD + 4), VKit.COL_PARCH,
		"%s habitants · %s · %s" % [_grp(info["ames"]), info["climat"], info["relief"]], VKit.FS_SMALL)

	# ── CAMEMBERTS culture & idéologie (VKit, réutilisés du panneau de gauche) ──
	var groups: Array = w.province_groups(_pid)
	var cy := HEAD + 60.0
	if groups.size() > 0:
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
				rnames.append(g["religion"])
				rper.append(g["percent"])
				rcol.append(VKit.SLICE_PAL[(rnames.size() - 1) % 8])
			else:
				rper[idx] += g["percent"]
		var pr := 40.0
		VKit.pie(self, Vector2(x + pr + 16, cy), pr, cper, ccol)
		VKit.pie(self, Vector2(x + 3 * pr + 56, cy), pr, rper, rcol)
		VKit.text(self, Vector2(x + 6, cy + pr + 6), VKit.COL_DIM, "Culture", VKit.FS_SMALL)
		VKit.text(self, Vector2(x + 2 * pr + 46, cy + pr + 6), VKit.COL_DIM, "Idéologie", VKit.FS_SMALL)
		# légende des cultures (nom + part)
		var lx := x + 4 * pr + 96
		var ly := cy - pr
		for i in range(mini(groups.size(), 6)):
			VKit.fill(self, Rect2(lx, ly + 3, 9, 9), VKit.SLICE_PAL[i % 8])
			VKit.text(self, Vector2(lx + 15, ly), VKit.COL_PARCH,
				"%s %d%%" % [String(groups[i]["culture"]), int(groups[i]["percent"])], VKit.FS_SMALL)
			ly += 17

	# ── POPULATION : barre empilée des classes ────────────────────────────────
	var cls: Dictionary = w.province_classes(_pid)
	var ccnt := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var tot: float = maxf(1.0, ccnt[0] + ccnt[1] + ccnt[2])
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	var py := HEAD + 60.0 + 40.0 + 30.0
	var rw := PW - 32.0
	var bh := 14.0
	var acc := 0.0
	for i in range(3):
		var segw: float = (rw - acc) if i == 2 else float(ccnt[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, py, segw, bh), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, py, rw, bh), VKit.COL_DIM)
	py += bh + 5
	for i in range(3):
		VKit.fill(self, Rect2(x + i * 200.0, py + 3, 9, 9), cc[i])
		VKit.text(self, Vector2(x + i * 200.0 + 15, py), VKit.COL_PARCH,
			"%s %s" % [cnames[i], _grp(ccnt[i])], VKit.FS_SMALL)

	# ── FLUX (production & ressources, /an) : barres + SPRITE de ressource dessous ──
	_draw_flux(x, py + 30.0, rw, PH - (py + 30.0) - 14.0, w)

func _draw_flux(fx: float, fy: float, fw: float, fh: float, w) -> void:
	VKit.text(self, Vector2(fx, fy), VKit.COL_COPPER, "Flux (production & ressources, /an)", VKit.FS_SMALL)
	var inc: Array = w.province_income(_pid)
	if inc.is_empty():
		VKit.text(self, Vector2(fx, fy + 20.0), VKit.COL_DIM, "rien de notable", VKit.FS_SMALL)
		return
	var n := inc.size()
	var vals := []
	var maxv := 1.0
	for l in inc:
		var v := float(l["per_day"]) * 365.0
		vals.append(v)
		maxv = maxf(maxv, absf(v))
	var top := fy + 20.0
	var base := fy + fh - 26.0          # ligne de base (place dessous pour le sprite)
	var barmax := base - top - 12.0     # place au-dessus pour la valeur
	# lignes de repère + valeur (0 · ½ · max)
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
		var col := VKit.SLICE_PAL[7] if manuf else VKit.COL_COPPER   # production = vert · ressource = cuivre
		VKit.fill(self, Rect2(cx - bw / 2.0, base - bhh, bw, bhh), col)
		var vs := "%.0f" % v
		VKit.text(self, Vector2(cx - VKit.text_w(vs, VKit.FS_SMALL) / 2.0, base - bhh - 13.0), VKit.COL_PARCH, vs, VKit.FS_SMALL)
		var spr := UIKit.resource_sprite(int(inc[i].get("res_id", -1)), String(inc[i]["source"]))
		if spr != null:
			draw_texture_rect(spr, Rect2(cx - 10.0, base + 3.0, 20.0, 20.0), false)
		else:
			VKit.text(self, Vector2(cx - 14.0, base + 5.0), VKit.COL_DIM, String(inc[i]["source"]).substr(0, 5), VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx, base, fw, 1), VKit.COL_DIM)
	# légende des teintes
	VKit.fill(self, Rect2(fx, fy + 2.0, 9, 9), VKit.COL_COPPER)
	VKit.text(self, Vector2(fx + 220.0, fy), VKit.COL_DIM, "■ ressource", VKit.FS_SMALL)
	VKit.text(self, Vector2(fx + 300.0, fy), VKit.SLICE_PAL[7], "■ production", VKit.FS_SMALL)

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
