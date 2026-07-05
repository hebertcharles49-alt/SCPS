extends Control
## ProvincePanel — PORT FIDÈLE de draw_province_panel (viewer.c). _draw immédiat
## avec VKit (mêmes couleurs, mêmes widgets). Lit la membrane via la façade
## (province_info + groups + income + classes + capitale). Display-only.
##
## Le Control occupe la bande GAUCHE (clic bloqué dessus) ; tout est dessiné en
## coordonnées LOCALES (le panneau est à 0,0 local, posé à y=102 à l'écran).

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const PW := 312.0

# HOVERS (point : « je ne sais pas ce qu'est un laborer, ni pourquoi l'humeur varie »).
const TIPS := {
	"Laboureurs": "La masse : fournit le travail (extraction + manufactures) et demande les biens de base (vivres, étoffe, bois de feu).",
	"Artisans":   "Marchands & artisans : possèdent les manufactures, captent le profit, demandent les biens manufacturés.",
	"Noblesse":   "L'aristocratie : vit de la taxe et de la rente, produit la recherche, exige le luxe (joaillerie, étoffe précieuse).",
	"humeur":     "L'humeur locale (légitimité de la province) : monte avec l'ordre et les besoins comblés ; chute sous la surtaxe, la coercition et les cicatrices de révolte.",
}
var _tips: Array = []

signal build_requested
signal close_requested   ## ✕ — la désélection pleine vit dans main (_clear_selection)
signal detail_requested  ## « Détail » — ouvre province_detail (main-d'œuvre & cie)

var _pid := -1
var _build_rect := Rect2()
var _colonize_rect := Rect2()   ## bouton COLONISER (province vierge légale — scps_can_colonize)
var _close_rect := Rect2()
var _acts := []                 ## [[Rect2, verbe:String], …] — chips d'action contextuels (posés au _draw)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # le panneau capte ses propres clics
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(_on_tick)
	hide()

func _layout() -> void:
	# entre les bandeaux : à droite du rail, sous le haut, au-dessus du bas
	position = Vector2(Frame.SIDEBAR_W, Frame.TOPBAR_H + 4.0)
	var h := get_viewport_rect().size.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 8.0
	size = Vector2(PW, maxf(80.0, h))

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
	VKit.fill(self, Rect2(PW - 2, 0, 2, ph), VKit.COL_GOLD)
	_tips.clear()
	var x := 16.0
	var y := 14.0

	# ── FOND DE BIOME (planches 20-22) : le paysage enluminé derrière l'en-tête ──
	var bio: Texture2D = UIKit.biome_painting(String(info.get("relief", "")), String(info.get("climat", "")))
	if bio != null:
		var bh := 58.0
		var tw := float(bio.get_width())
		var reg_h := tw * bh / (PW - 4.0)
		draw_texture_rect_region(bio, Rect2(2, 2, PW - 4.0, bh),
			Rect2(0, tw * 0.16, tw, reg_h), Color(0.88, 0.84, 0.78))
		draw_rect(Rect2(2, 2, PW - 4.0, bh), Color(0.05, 0.04, 0.03, 0.30), true)
		# fondu bas → le paysage se dissout dans le panneau
		for i in range(5):
			draw_rect(Rect2(2, 2 + bh - 10 + i * 2, PW - 4.0, 2),
				Color(VKit.COL_PANEL.r, VKit.COL_PANEL.g, VKit.COL_PANEL.b, 0.18 + 0.16 * i), true)

	# ── EN-TÊTE : les ARMES du propriétaire (héraldique dérivée) · nom · prospérité ───
	var hsz := 30.0
	var owner_arms: Texture2D = null
	if int(info.get("owner", -1)) >= 0:
		owner_arms = load("res://ui/heraldry.gd").arms(int(info["owner"]))
	if owner_arms != null:
		draw_texture_rect(owner_arms, Rect2(x - 2, y - 1, hsz + 6, hsz + 6), false)
	else:
		VKit.box(self, Rect2(x, y + 2, hsz, hsz), VKit.COL_GOLD)
		VKit.fill(self, Rect2(x + 1, y + 3, hsz - 2, hsz - 2), VKit.COL_PANEL2)
		UIKit.draw_icon(self, "capital_tower", Vector2(x + 3, y + 5), hsz - 6)
	VKit.text(self, Vector2(x + hsz + 8, y), VKit.COL_GOLD, String(info["nom"]), VKit.FS_BIG)
	var gw := 64.0
	var gx := PW - 16.0 - gw
	UIKit.bar(self, Rect2(gx, y + 2, gw, 14), int(info["aisance_val"]))
	var nb := str(info["aisance_val"])
	VKit.text(self, Vector2(gx - VKit.text_w(nb) - 6, y), VKit.COL_PARCH, nb)
	# ✕ — tout panneau se ferme (Échap aussi, via la pile de main) ; ferme = DÉSÉLECTIONNE
	_close_rect = Rect2(PW - 20, 3, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	# labelle la jauge (c'était un « 9 » nu — le joueur ne savait pas ce que c'était)
	VKit.text(self, Vector2(gx, y + 17), VKit.COL_DIM, "Prospérité", VKit.FS_SMALL)
	# climat · relief · statut de capitale
	var cap: Dictionary = w.province_capitale(_pid)
	VKit.text(self, Vector2(x + hsz + 8, y + 18), VKit.COL_PARCH,
		"%s · %s · %s" % [info["climat"], info["relief"], cap.get("statut", "")])
	y += hsz + 8
	if bio != null:
		y = maxf(y, 70.0)   # le contenu reprend SOUS la bande-paysage (fondu compris)

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
		y = VKit.row(self, x, y, "Héritage", String(info["heritage"]), VKit.COL_PARCH)

	# ── HUMEUR : rangée de visages + chiffre ──────────────────────────────────
	var hy0 := y
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
	_tips.append([Rect2(0.0, hy0, PW, y - hy0), String(TIPS["humeur"])])

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
		_tips.append([Rect2(0.0, y - 1.0, PW, 18.0), String(TIPS.get(cnames[i], ""))])
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
			# Calibrage « Anno » : flux en unités/JOUR à 1 décimale (per_day du readout, tel quel).
			VKit.text(self, Vector2(x, y), VKit.sense(0.62), "+%.1f/j" % float(l["per_day"]))
			VKit.text(self, Vector2(x + 74, y), VKit.COL_DIM, String(l["source"]))
			y += 18
	y += 4

	# ── seuil de révolte ──────────────────────────────────────────────────────
	if bool(info.get("seuil_revolte", false)):
		UIKit.draw_icon(self, "alert_revolt", Vector2(x, y - 2), 18)
		VKit.text(self, Vector2(x + 22, y), VKit.sense(0.06),
			"Au bord de la révolte (agitation %d)" % int(info["agitation"]))
		y += 22

	# ── CAPITALE ──────────────────────────────────────────────────────────────
	y = VKit.section(self, x, y, "CAPITALE")
	y = VKit.row(self, x, y, "Statut", "%s · tier %d" % [cap.get("statut", ""), int(cap.get("tier", 0))], VKit.COL_GOLD)
	var libres: int = int(cap.get("logement_cap", 0)) - int(cap.get("pop", 0))
	y = VKit.row(self, x, y, "Logement", "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("logement_cap", 0))],
		VKit.COL_PARCH if libres >= 0 else VKit.sense(0.12))
	y = VKit.row(self, x, y, "Services", "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("service_cap", 0))], VKit.COL_PARCH)
	if int(cap.get("prod_pct", 0)) > 0:
		y = VKit.row(self, x, y, "Productivité", "+%d%%" % int(cap["prod_pct"]), VKit.sense(0.7))

	# ── ACTIONS CONTEXTUELLES (selon la propriété ; chaque verbe est journalisé,
	#    le drain revalide) : à MOI = construire + intérieur + détail · VIERGE légale =
	#    coloniser · ENNEMI en guerre = attaquer · ÉTRANGER en paix = routes. ──
	y += 8
	var bw := 120.0
	var bbh := 28.0
	_build_rect = Rect2()
	_colonize_rect = Rect2()
	_acts.clear()
	var me: int = w.player()
	var powner := int(info.get("owner", -2))
	if powner == me:
		_build_rect = Rect2(x, y, bw, bbh)
		VKit.fill(self, _build_rect, VKit.COL_PANEL2)
		VKit.box(self, _build_rect, VKit.COL_GOLD)
		UIKit.draw_icon(self, "action_build", Vector2(x + 6, y + 5), 18)
		VKit.text(self, Vector2(x + 28, y + 5), VKit.COL_GOLD, "Construire")
		y += bbh + 6
		_act_chips(x, y, [["Réprimer", "repress"], ["Assimiler", "assimilate"],
			["Purger", "purge"], ["Détail", "detail"]])
	elif w.has_method("can_colonize") and w.can_colonize(_pid):
		# le verbe d'EXPANSION du joueur (charte : « le joueur colonise n'importe quelle
		# province ») — visible seulement si LÉGAL (cible vierge + une source aux portes).
		_colonize_rect = Rect2(x, y, bw + 16, bbh)
		VKit.fill(self, _colonize_rect, VKit.COL_PANEL2)
		VKit.box(self, _colonize_rect, Color(0.55, 0.62, 0.38))
		UIKit.draw_icon(self, "action_build", Vector2(x + 6, y + 5), 18)
		VKit.text(self, Vector2(x + 28, y + 5), Color(0.62, 0.70, 0.42), "Coloniser")
	elif powner >= 0 and powner != me:
		var dop: Dictionary = w.diplo_options(powner) if w.has_method("diplo_options") else {}
		if bool(dop.get("can_make_peace", false)):
			# EN GUERRE avec ce pays → projeter l'ost sur CETTE région (depuis la capitale)
			_act_chips(x, y, [["Attaquer ici", "campaign"]])
		else:
			# en paix → tenter une ROUTE commerciale depuis ma capitale (le moteur gate ports/pactes)
			_act_chips(x, y, [["Route terre", "route_land"], ["Route mer", "route_sea"]])

## une rangée de CHIPS d'action (petits boutons) — les rects sont mémorisés dans _acts
## avec leur verbe, hit-testés au clic. Retourne rien (le layout suit x fixe).
func _act_chips(x: float, y: float, items: Array) -> void:
	var cx := x
	for it in items:
		var label: String = it[0]
		var cw := VKit.text_w(label, VKit.FS_SMALL) + 14.0
		var r := Rect2(cx, y, cw, 20.0)
		VKit.fill(self, r, VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_GOLD)
		VKit.text(self, Vector2(cx + 7, y + 3), VKit.COL_PARCH, label, VKit.FS_SMALL)
		_acts.append([r, String(it[1])])
		cx += cw + 6.0

## dispatch d'un chip d'action → le VERBE journalisé (drainé au tick, revalidé là-bas).
func _act_fire(verb: String) -> void:
	var w = Sim.world
	if w == null or _pid < 0:
		return
	var reg: int = w.province_region(_pid)
	match verb:
		"repress":
			w.player_repress(reg)
		"assimilate":
			w.player_assimilate(reg, false)
		"purge":
			w.player_purge(reg)
		"detail":
			detail_requested.emit()
		"campaign":
			var capr: int = w.province_region(w.country_capital_province(w.player()))
			if capr >= 0 and reg >= 0:
				w.player_campaign(capr, reg)
		"route_land", "route_sea":
			var capr2: int = w.province_region(w.country_capital_province(w.player()))
			if capr2 >= 0 and reg >= 0:
				w.player_route(capr2, reg, verb == "route_sea")
	queue_redraw()

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

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(event.position):
			close_requested.emit()             # main désélectionne (panneau + contour doré)
			accept_event()
			return
		if _build_rect.size.x > 0 and _build_rect.has_point(event.position):
			build_requested.emit()
		elif _colonize_rect.size.x > 0 and _colonize_rect.has_point(event.position):
			if Sim.world != null and Sim.world.has_method("player_colonize"):
				Sim.world.player_colonize(_pid)   # enfilé ; fondé au drain → le bouton s'éteint
				queue_redraw()
		else:
			for a in _acts:
				if (a[0] as Rect2).has_point(event.position):
					_act_fire(String(a[1]))
					break

## HOVER natif : Godot appelle ceci au survol → texte de la zone touchée (classe / humeur).
func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""
