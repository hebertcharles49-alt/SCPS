extends Control
## ProvincePanel — PORT FIDÈLE de draw_province_panel (viewer.c). _draw immédiat
## avec VKit (mêmes couleurs, mêmes widgets). Lit la membrane via la façade
## (province_info + groups + income + classes + capitale). Display-only.
##
## Le Control occupe la bande GAUCHE (clic bloqué dessus) ; tout est dessiné en
## coordonnées LOCALES (le panneau est à 0,0 local, posé à y=102 à l'écran).

const VKit = preload("res://ui/vkit.gd")
const PW := 312.0
const TOP := 102.0

var _pid := -1

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # le panneau capte ses propres clics
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(_on_tick)
	hide()

func _layout() -> void:
	position = Vector2(0, TOP)
	size = Vector2(PW, maxf(80.0, get_viewport_rect().size.y - TOP - 26.0))

func show_province(pid: int) -> void:
	_pid = pid
	visible = pid >= 0
	queue_redraw()

func _on_tick(_year: int) -> void:
	if visible:
		queue_redraw()

func _draw() -> void:
	var w = Sim.world
	if w == null or _pid < 0:
		return
	var info: Dictionary = w.province_info(_pid)
	if not bool(info.get("valide", false)):
		return
	var ph := size.y
	var rw := PW - 30.0
	VKit.panel_bg(self, Rect2(0, 0, PW, ph))
	VKit.fill(self, Rect2(PW - 2, 0, 2, ph), VKit.COL_COPPER)
	var x := 16.0
	var y := 14.0

	# ── EN-TÊTE : héraldique · nom · jauge de prospérité ──────────────────────
	var hsz := 30.0
	VKit.box(self, Rect2(x, y + 2, hsz, hsz), VKit.COL_COPPER)
	VKit.fill(self, Rect2(x + 1, y + 3, hsz - 2, hsz - 2), VKit.COL_PANEL2)
	VKit.text(self, Vector2(x + hsz + 8, y), VKit.COL_COPPER, String(info["nom"]), VKit.FS_BIG)
	var gw := 64.0
	var gx := PW - 16.0 - gw
	VKit.gauge(self, gx, y + 4, gw, 10, int(info["aisance_val"]))
	var nb := str(info["aisance_val"])
	VKit.text(self, Vector2(gx - VKit.text_w(nb) - 6, y), VKit.COL_PARCH, nb)
	# climat · relief · statut de capitale
	var cap: Dictionary = w.province_capitale(_pid)
	VKit.text(self, Vector2(x + hsz + 8, y + 18), VKit.COL_PARCH,
		"%s · %s · %s" % [info["climat"], info["relief"], cap.get("statut", "")])
	y += hsz + 8

	# ── HABITANTS ─────────────────────────────────────────────────────────────
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "%s habitants" % _grp(info["ames"]))
	y += 22

	# ── CAMEMBERTS culture / idéologie (ou repli PEUPLE) ──────────────────────
	var groups: Array = w.province_groups(_pid)
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
		var pr := 22.0
		var cyc := y + pr + 4
		var cx1 := x + pr + 6
		var cx2 := x + rw / 2.0 + pr + 2
		VKit.pie(self, Vector2(cx1, cyc), pr, cper, ccol)
		VKit.pie(self, Vector2(cx2, cyc), pr, rper, rcol)
		VKit.text(self, Vector2(cx1 - pr, cyc + pr + 3), VKit.COL_DIM, "Culture", VKit.FS_SMALL)
		VKit.text(self, Vector2(cx2 - pr, cyc + pr + 3), VKit.COL_DIM, "Idéologie", VKit.FS_SMALL)
		y = cyc + pr + 16
	else:
		y = VKit.section(self, x, y, "PEUPLE")
		y = VKit.row(self, x, y, "Race", String(info["race"]), VKit.COL_PARCH)

	# ── HUMEUR : rangée de visages + chiffre ──────────────────────────────────
	y = VKit.section(self, x, y, "HUMEUR")
	var nf := 5
	var fr := 9.0
	var gap := 8.0
	var fy := y + fr
	var moodv := float(info["humeur_val"]) / 100.0
	var lit := int(moodv * (nf - 1) + 0.5)
	for i in range(nf):
		VKit.face(self, Vector2(x + fr + i * (2 * fr + gap), fy), fr, float(i) / (nf - 1), i == lit)
	VKit.text(self, Vector2(x + nf * (2 * fr + gap) + 6, y), VKit.sense(moodv), str(info["humeur_val"]))
	y = fy + fr + 8

	# ── POPULATION : barre empilée des classes + légende ──────────────────────
	var cls: Dictionary = w.province_classes(_pid)
	var cp := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var tot: float = maxf(1.0, cp[0] + cp[1] + cp[2])
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	y = VKit.section(self, x, y, "POPULATION")
	var bh := 12.0
	var acc := 0.0
	for i in range(3):
		var segw: float = (rw - acc) if i == 2 else float(cp[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, y, segw, bh), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, y, rw, bh), VKit.COL_DIM)
	y += bh + 5
	for i in range(3):
		VKit.fill(self, Rect2(x, y + 3, 9, 9), cc[i])
		VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH, "%s %s" % [cnames[i], _grp(cp[i])])
		y += 18

	# ── RESSOURCES + PRODUCTION ───────────────────────────────────────────────
	var inc: Array = w.province_income(_pid)
	y = VKit.section(self, x, y, "RESSOURCES")
	var res := ""
	var shown := 0
	for l in inc:
		if bool(l["manufactured"]):
			continue
		if shown >= 2:
			break
		res += ("" if shown == 0 else " · ") + String(l["source"])
		shown += 1
	if shown == 0:
		res = String(info["ressource"])
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, res)
	y += 22
	y = VKit.section(self, x, y, "PRODUCTION")
	if inc.size() == 0:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien de notable")
		y += 18
	else:
		for l in inc:
			VKit.text(self, Vector2(x, y), VKit.sense(0.62), "+%.1f/j" % l["per_day"])
			VKit.text(self, Vector2(x + 74, y), VKit.COL_DIM, String(l["source"]))
			y += 18
	y += 4

	# ── seuil de révolte ──────────────────────────────────────────────────────
	if bool(info.get("seuil_revolte", false)):
		VKit.text(self, Vector2(x, y), VKit.sense(0.06),
			"⚑ Au bord de la révolte (agitation %d)" % int(info["agitation"]))
		y += 22

	# ── CAPITALE ──────────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "CAPITALE")
	y = VKit.row(self, x, y, "Statut", "%s · tier %d" % [cap.get("statut", ""), int(cap.get("tier", 0))], VKit.COL_COPPER)
	var libres: int = int(cap.get("logement_cap", 0)) - int(cap.get("pop", 0))
	y = VKit.row(self, x, y, "Logement", "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("logement_cap", 0))],
		VKit.COL_PARCH if libres >= 0 else VKit.sense(0.12))
	y = VKit.row(self, x, y, "Services", "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("service_cap", 0))], VKit.COL_PARCH)
	if int(cap.get("prod_pct", 0)) > 0:
		y = VKit.row(self, x, y, "Productivité", "+%d%%" % int(cap["prod_pct"]), VKit.sense(0.7))

# milliers lisibles : 12345 → "12 345"
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
