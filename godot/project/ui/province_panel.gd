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
const PW := 348.0   ## élargi (retour joueur 2026-07-10 : « adapte le menu de gauche
                    ## en taille » — la police +1 mordait les jauges à 312)

# HOVERS (point : « je ne sais pas ce qu'est un laborer, ni pourquoi l'humeur varie »).
const TIPS := {
	"Laboureurs": "La masse : fournit le travail (extraction, manufactures). Son panier est simple (vivres, étoffe, bois de feu) et sa satisfaction fait la paix sociale.",
	"Artisans":   "Marchands et artisans : possèdent les manufactures, captent le profit. Leur panier demande les biens manufacturés.",
	"Noblesse":   "L'aristocratie : vit de la taxe et de la rente, produit le savoir. Son panier exige le luxe.",
	"humeur":     "La loyauté de la province : monte avec l'ordre et la satisfaction ; chute sous la surtaxe, la coercition et les cicatrices.",
}
var _tips: Array = []

signal build_requested
signal close_requested   ## ✕ — la désélection pleine vit dans main (_clear_selection)
signal detail_requested  ## « Détail » — ouvre province_detail (main-d'œuvre & cie)

var _pid := -1
var _build_rect := Rect2()
var _colonize_rect := Rect2()   ## bouton COLONISER (province vierge légale — scps_can_colonize)
var _colonize_ms := -100000     ## horloge MUR du dernier ordre (feedback « ordre émis », 3 s)
var _close_rect := Rect2()
var _purge_armed := false       ## UI-4 : Purger EXIGE 2 clics (motif _servile_manumit_armed) — armé par le 1er
var _purge_armed_ms := -100000  ## horloge MUR de l'armement — retombe après 4 s sans confirmation
var _collapsed := false         ## REPLIÉ : bande-paysage + nom seuls (retour joueur « rétractable »)
var _collapse_rect := Rect2()
var _acts := []                 ## [[Rect2, verbe:String], …] — chips d'action contextuels (posés au _draw)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # le panneau capte ses propres clics
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.month_ticked.connect(_on_tick)   # chiffres de province : cadence mensuelle
	hide()

var _ph := 480.0                ## hauteur AU CONTENU (latchée au _draw — le panneau plein-colonne
                                ## laissait un grand vide sous les boutons, retour joueur)

func _layout() -> void:
	# FLOTTANTE façon EU4 (retour joueur 2026-07-10 : « délocke la fenêtre Province ») :
	# décalée à DROITE du rail + sous le haut, avec une marge — plus une colonne collée.
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 12.0)
	var hmax := get_viewport_rect().size.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 24.0
	size = Vector2(PW, clampf(_ph, 80.0, hmax))

func show_province(pid: int) -> void:
	if pid != _pid:
		_purge_armed = false          # changer de province désarme une confirmation en attente
	_pid = pid
	visible = pid >= 0
	queue_redraw()

func _on_tick(_year: int) -> void:
	if visible:
		queue_redraw()

## la fenêtre de confirmation (4 s) retombe même hors tick/clic — indépendant de la
## pause du jeu (Sim.ticked ne tourne pas en pause ; ce Control, si).
func _process(_dt: float) -> void:
	if _purge_armed and Time.get_ticks_msec() - _purge_armed_ms > 4000:
		_purge_armed = false
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
	# fenêtre FLOTTANTE : le panel_bg porte son propre cadre (le liseré or « collé au
	# rail » de l'ancienne colonne est retiré)
	VKit.panel_bg(self, Rect2(0, 0, PW, ph))
	_tips.clear()
	var x := 16.0
	var y := 14.0

	# ── BANDE DE BIOME (planches 20-22), GRANDE et NUE (retour joueur : « le biome doit
	#    être plus visible, le display suit ») : le paysage plein cadre, le texte DESSOUS.
	var bio: Texture2D = UIKit.biome_painting(String(info.get("relief", "")), String(info.get("climat", "")))
	var bioh := 40.0 if _collapsed else 96.0   # ⚠ pas « bh » : redéclaré plus bas (POPULATION)
	if bio != null:
		var tw := float(bio.get_width())
		var reg_h := tw * bioh / (PW - 4.0)
		draw_texture_rect_region(bio, Rect2(2, 2, PW - 4.0, bioh),
			Rect2(0, tw * 0.10, tw, reg_h), Color(0.98, 0.95, 0.90))
		draw_rect(Rect2(2, 2, PW - 4.0, bioh), Color(0.05, 0.04, 0.03, 0.10), true)
		# fondu bas → le paysage se dissout dans le panneau
		for i in range(5):
			draw_rect(Rect2(2, 2 + bioh - 10 + i * 2, PW - 4.0, 2),
				Color(VKit.COL_PANEL.r, VKit.COL_PANEL.g, VKit.COL_PANEL.b, 0.18 + 0.16 * i), true)
		# HOVER DÉFENSE : le terrain PROLONGE la tenue de siège — % lisible dérivé du moteur.
		var def_pct := int(w.province_defense_pct(_pid))
		var def_word := String(info.get("defense", ""))
		var def_hover := "Le paysage de la province — terrain : %s · climat : %s. Tenue de siège +%d%%" \
			% [String(info.get("relief", "")), String(info.get("climat", "")), def_pct - 100]
		if def_word != "" and def_word != "aucune":
			def_hover += " · %s" % def_word
		_tips.append([Rect2(2.0, 2.0, PW - 4.0, bioh), def_hover])
		y = bioh + 10.0

	# ✕ (fermer) + chevron (REPLIER — retour joueur : « barre gauche à adapter/rétractable »)
	_close_rect = Rect2(PW - 20, 3, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	_collapse_rect = Rect2(PW - 40, 3, 16, 16)
	VKit.fill(self, _collapse_rect, VKit.COL_PANEL2)
	VKit.box(self, _collapse_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_collapse_rect.position.x + 4, _collapse_rect.position.y + 1),
		VKit.COL_PARCH, "+" if _collapsed else "–")
	_tips.append([_collapse_rect, "Déplier le panneau" if _collapsed else "Replier le panneau (la carte respire)"])

	# ── EN-TÊTE (SOUS la peinture) : armes du propriétaire · nom · climat/relief/statut ──
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
	var cap: Dictionary = w.province_capitale(_pid)
	VKit.text(self, Vector2(x + hsz + 8, y + 18), VKit.COL_PARCH,
		"%s · %s · %s" % [info["climat"], info["relief"], cap.get("statut", "")])
	y += hsz + 12

	# REPLIÉ : la bande-paysage + le nom suffisent — la carte respire.
	if _collapsed:
		var wantc := y + 4.0
		if absf(_ph - wantc) > 3.0:
			_ph = wantc
			set_deferred("size", Vector2(PW, _ph))
			queue_redraw()
		return

	# ── HABITANTS + PROSPÉRITÉ (sortie du header : elle y chevauchait paysage & ✕) ──
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "%s habitants" % _grp(info["ames"]))
	var gw := 64.0
	VKit.gauge(self, x + rw - gw, y + 4, gw, 10, int(info["aisance_val"]))
	var plab := "Prospérité %d" % int(info["aisance_val"])
	VKit.text(self, Vector2(x + rw - gw - VKit.text_w(plab, VKit.FS_SMALL) - 10, y + 1), VKit.COL_DIM, plab, VKit.FS_SMALL)
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
		# cerne d'encre (une culture à 100 % rendait un disque plat « cassé ») + le NOM
		# dominant sous chaque disque — le joueur lit QUI, pas juste une couleur.
		draw_arc(Vector2(cx1, cyc), pr + 0.5, 0.0, TAU, 48, VKit.COL_DIM, 1.2, true)
		draw_arc(Vector2(cx2, cyc), pr + 0.5, 0.0, TAU, 48, VKit.COL_DIM, 1.2, true)
		var relig_dom := 0
		for i in range(rper.size()):
			if rper[i] > rper[relig_dom]:
				relig_dom = i
		VKit.text(self, Vector2(cx1 - pr, cyc + pr + 3), VKit.COL_DIM,
			"Culture · %s" % String(groups[0].get("culture", "")), VKit.FS_SMALL)
		VKit.text(self, Vector2(cx2 - pr, cyc + pr + 3), VKit.COL_DIM,
			"Idéologie · %s" % String(rnames[relig_dom]), VKit.FS_SMALL)
		y = cyc + pr + 16
	else:
		y = VKit.section(self, x, y, "PEUPLE")
		y = VKit.row(self, x, y, "Héritage", String(info["heritage"]), VKit.COL_PARCH)

	# ── SATISFACTION par CLASSE (retour joueur : « humeur » → satisfaction, une barre
	#    par pop, la strate SERVILE visible même vide) + la LOYAUTÉ (l'ex-humeur =
	#    légitimité locale, un AUTRE concept — gardée en ligne compacte, tip dédié). ──
	var hy0 := y
	y = VKit.section(self, x, y, "SATISFACTION")
	if w.has_method("province_class_sat"):
		var csat: Dictionary = w.province_class_sat(_pid)
		for srow in [["Laboureurs", "laboureurs"], ["Artisans", "artisans"],
			["Noblesse", "noblesse"], ["Esclaves", "esclaves"]]:
			var sv2 := int(csat.get(srow[1], -1))
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, srow[0], VKit.FS_SMALL)
			if sv2 >= 0:
				VKit.gauge(self, x + 92, y + 3, rw - 136, 9, sv2)
				VKit.text(self, Vector2(x + rw - 30, y), VKit.sense(sv2 / 100.0), str(sv2), VKit.FS_SMALL)
			else:
				VKit.text(self, Vector2(x + 92, y), VKit.COL_DIM, "—", VKit.FS_SMALL)
			y += 17
		_tips.append([Rect2(0.0, hy0, PW, y - hy0),
			"La part des BESOINS couverts par classe (vivres, biens, confort — le panier). Basse chez les laboureurs = misère → agitation, révolte."])
	var moodv := float(info["humeur_val"]) / 100.0
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Loyauté", VKit.FS_SMALL)
	VKit.gauge(self, x + 92, y + 3, rw - 136, 9, int(info["humeur_val"]))
	VKit.text(self, Vector2(x + rw - 30, y), VKit.sense(moodv), str(info["humeur_val"]), VKit.FS_SMALL)
	_tips.append([Rect2(0.0, y - 1.0, PW, 17.0), String(TIPS["humeur"])])
	y += 20

	# ── POPULATION : barre empilée des classes (+ ESCLAVES, 4e segment) + légende ──
	var cls: Dictionary = w.province_classes(_pid)
	var slaves: int = int(w.province_slave_count(_pid))
	var cp := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	# la strate SERVILE est TOUJOURS listée (retour joueur : « visible même si aucun »)
	cp.append(slaves)
	cc.append(Color(0.28, 0.26, 0.24))   # gris sombre — distincte des classes libres
	cnames.append("Esclaves")
	var tot: float = maxf(1.0, cp[0] + cp[1] + cp[2] + slaves)
	y = VKit.section(self, x, y, "POPULATION")
	var bh := 12.0
	var acc := 0.0
	var nseg := cp.size()
	for i in range(nseg):
		var segw: float = (rw - acc) if i == nseg - 1 else float(cp[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, y, segw, bh), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, y, rw, bh), VKit.COL_DIM)
	y += bh + 5
	for i in range(nseg):
		VKit.fill(self, Rect2(x, y + 3, 9, 9), cc[i])
		var lbl: String = String(cnames[i])
		if i == 3:
			lbl = "%s (%d%%)" % [cnames[i], int(round(100.0 * float(cp[i]) / tot))]
		VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH, "%s %s" % [lbl, _grp(cp[i])])
		_tips.append([Rect2(0.0, y - 1.0, PW, 18.0), String(TIPS.get(cnames[i], ""))])
		y += 18

	# ── RESSOURCES + PRODUCTION ───────────────────────────────────────────────
	var inc: Array = w.province_income(_pid)
	var yres := y
	y = VKit.section(self, x, y, "RESSOURCES")
	_tips.append([Rect2(0.0, yres, PW, 30.0),
		"Ce que la terre DONNE ici (les deux gisements majeurs). La province les extrait selon ses bras et les prix."])
	# chaque ressource porte son ICÔNE (retour joueur) — sprite du pack + nom
	var rx := x
	var shown := 0
	var rlim := x + rw - 128.0   # borne : le libellé Impôts vit à droite (plus de chevauchement)
	for l in inc:
		if bool(l["manufactured"]):
			continue
		if shown >= 3:
			break
		var rw2 := 22.0 + VKit.text_w(String(l["source"])) + 14.0
		if rx + rw2 > rlim:
			break
		var rspr: Texture2D = UIKit.resource_sprite(int(l.get("res_id", -1)), String(l["source"]))
		if rspr != null:
			draw_texture_rect(rspr, Rect2(rx, y - 2, 18, 18), false)
			rx += 22
		VKit.text(self, Vector2(rx, y), VKit.COL_PARCH, String(l["source"]))
		rx += VKit.text_w(String(l["source"])) + 14
		shown += 1
	if shown == 0:
		var rnom := String(info["ressource"])
		var rspr0: Texture2D = UIKit.resource_icon(rnom)
		if rspr0 != null:
			draw_texture_rect(rspr0, Rect2(x, y - 2, 18, 18), false)
			VKit.text(self, Vector2(x + 22, y), VKit.COL_PARCH, rnom)
		else:
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, rnom)
	var tax := float(w.province_tax(_pid))
	if tax > 0.5:
		var taxtxt := "Impôts ~%s or/an" % _grp(int(round(tax)))
		VKit.text(self, Vector2(x + rw - VKit.text_w(taxtxt) - 8.0, y), VKit.COL_DIM, taxtxt)
		_tips.append([Rect2(x + rw - VKit.text_w(taxtxt) - 12.0, y - 2.0, VKit.text_w(taxtxt) + 12.0, 20.0),
			"Ce que la couronne LÈVE ici par an : ~42 % de la richesse des classes, rogné par l'évasion quand la satisfaction baisse ou que l'éthos tolère mal l'impôt."])
	y += 22
	var yprod := y
	y = VKit.section(self, x, y, "PRODUCTION")
	_tips.append([Rect2(0.0, yprod, PW, 30.0),
		"Les flux RÉALISÉS, en unités par JOUR : extraction des gisements + sortie des ateliers. Suit les bras disponibles et les prix du marché."])
	if inc.size() == 0:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien de notable")
		y += 18
	else:
		for l in inc:
			# Calibrage « Anno » : flux en unités/JOUR à 1 décimale + l'ICÔNE du bien.
			VKit.text(self, Vector2(x, y), VKit.sense(0.62), "+%.1f/j" % float(l["per_day"]))
			var pspr: Texture2D = UIKit.resource_sprite(int(l.get("res_id", -1)), String(l["source"]))
			if pspr != null:
				draw_texture_rect(pspr, Rect2(x + 66, y - 2, 16, 16), false)
			VKit.text(self, Vector2(x + 86, y), VKit.COL_DIM, String(l["source"]))
			y += 18
	y += 4

	# ── seuil de révolte ──────────────────────────────────────────────────────
	if bool(info.get("seuil_revolte", false)):
		UIKit.draw_icon(self, "alert_revolt", Vector2(x, y - 2), 18)
		VKit.text(self, Vector2(x + 22, y), VKit.sense(0.06),
			"Au bord de la révolte (agitation %d)" % int(info["agitation"]))
		y += 22

	# ── CAPITALE — chaque ligne porte son POURQUOI au survol (retour joueur) ──
	y = VKit.section(self, x, y, "CAPITALE")
	_tips.append([Rect2(0.0, y, PW, 20.0),
		"Le rang du bourg : il monte avec la POPULATION (2 000 âmes = tier 2 … 10 000 = tier 7) et ouvre des bâtiments de palier."])
	y = VKit.row(self, x, y, "Statut", "%s · tier %d" % [cap.get("statut", ""), int(cap.get("tier", 0))], VKit.COL_GOLD)
	var libres: int = int(cap.get("logement_cap", 0)) - int(cap.get("pop", 0))
	_tips.append([Rect2(0.0, y, PW, 20.0),
		"Âmes logées / capacité : ½ vient de la terre nue, le reste du BÂTI (manufactures-logements, confort). Plein = la croissance s'étouffe."])
	# UI-5 (retour joueur : « la couleur seule ne suffit pas ») : la saturation ne se lit
	# plus qu'à la teinte rouge — un « ⚠ » double le canal pour les daltoniens/le survol rapide.
	var housing_txt := "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("logement_cap", 0))]
	if libres < 0:
		housing_txt = "⚠ " + housing_txt
	y = VKit.row(self, x, y, "Logement", housing_txt, VKit.COL_PARCH if libres >= 0 else VKit.sense(0.12))
	_tips.append([Rect2(0.0, y, PW, 20.0),
		"Âmes servies / capacité de SERVICES (échoppes, bains, cultes) : au-delà, le confort décroche."])
	y = VKit.row(self, x, y, "Services", "%s/%s" % [_grp(cap.get("pop", 0)), _grp(cap.get("service_cap", 0))], VKit.COL_PARCH)
	if int(cap.get("prod_pct", 0)) > 0:
		_tips.append([Rect2(0.0, y, PW, 20.0),
			"Le bonus de production du bourg : OUTILS en circulation + édifices de savoir-faire + tier — il multiplie extraction et ateliers de la province."])
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
	# ── BÂTIMENTS façon CK3 (retour joueur) : les carrés du BÂTI (icône + hover
	#    nom·niveau·ouvriers) + la case « + » = construire (remplace le gros bouton). ──
	var blds: Array = w.province_buildings(_pid)
	var edis: Array = w.province_edifices(_pid) if w.has_method("province_edifices") else []
	if blds.size() > 0 or edis.size() > 0 or powner == me:
		var ybld := y
		y = VKit.section(self, x, y, "BÂTIMENTS")
		_tips.append([Rect2(0.0, ybld, PW, 30.0),
			"Les édifices et manufactures ÉLEVÉS ici. Le palier suit le TIER du bourg (population) et les techs."])
		var bs := 28.0
		var bx := x
		# d'abord les ÉDIFICES DE BASE (grenier, marché, temple… — retour joueur :
		# « on ne voit pas les bâtiments de base »), puis les manufactures.
		for e in edis:
			if bx + bs > x + rw - 4.0:
				bx = x
				y += bs + 4.0
			var er := Rect2(bx, y, bs, bs)
			VKit.fill(self, er, VKit.COL_PANEL2)
			VKit.box(self, er, VKit.COL_GOLD)
			var et: Texture2D = UIKit.building_sprite(int(e.get("type", -1)))
			if et != null:
				draw_texture_rect(et, Rect2(er.position + Vector2(3, 3), Vector2(bs - 6, bs - 6)), false)
			else:
				UIKit.draw_icon(self, "capital_tower", er.position + Vector2(6, 6), bs - 12)
			_tips.append([er, String(e["nom"])])
			bx += bs + 4.0
		for b in blds:
			if bx + bs > x + rw - 4.0:
				bx = x
				y += bs + 4.0
			var br := Rect2(bx, y, bs, bs)
			VKit.fill(self, br, VKit.COL_PANEL2)
			VKit.box(self, br, VKit.COL_GOLD)
			var mt: Texture2D = UIKit.manuf_sprite(String(b["nom"]))
			if mt != null:
				draw_texture_rect(mt, Rect2(br.position + Vector2(3, 3), Vector2(bs - 6, bs - 6)), false)
			else:
				UIKit.draw_icon(self, "build_hammer", br.position + Vector2(6, 6), bs - 12)
			_tips.append([br, "%s — niveau %d · %d ouvriers" % [String(b["nom"]), int(b["niveau"]), int(b["ouvriers"])]])
			bx += bs + 4.0
		_build_rect = Rect2()
		if powner == me:
			# la case « + » : bâtir (pointillé or, hover explicite)
			_build_rect = Rect2(bx, y, bs, bs)
			VKit.fill(self, _build_rect, Color(VKit.COL_PANEL2.r, VKit.COL_PANEL2.g, VKit.COL_PANEL2.b, 0.55))
			VKit.box(self, _build_rect, VKit.COL_GOLD)
			VKit.text(self, Vector2(bx + bs * 0.5 - 5.0, y + 3.0), VKit.COL_GOLD, "+", VKit.FS_BIG)
			_tips.append([_build_rect, "Construire — ouvrir le panneau de bâti (unités & édifices)"])
		y += bs + 10.0
	if powner == me:
		_draw_gov_actions(x, y, w)
	elif w.has_method("can_colonize") and w.can_colonize(_pid):
		# le verbe d'EXPANSION du joueur (charte : « le joueur colonise n'importe quelle
		# province ») — visible seulement si LÉGAL (cible vierge + une source aux portes).
		_colonize_rect = Rect2(x, y, bw + 16, bbh)
		VKit.fill(self, _colonize_rect, VKit.COL_PANEL2)
		VKit.box(self, _colonize_rect, Color(0.55, 0.62, 0.38))
		UIKit.draw_icon(self, "action_build", Vector2(x + 6, y + 5), 18)
		VKit.text(self, Vector2(x + 28, y + 5), Color(0.62, 0.70, 0.42), "Coloniser")
		# FEEDBACK (retour joueur) : l'ordre vient d'être émis → on le DIT sous le bouton
		if Time.get_ticks_msec() - _colonize_ms < 3000:
			VKit.box(self, _colonize_rect, Color(0.75, 0.85, 0.55))
			VKit.text(self, Vector2(x, y + bbh + 4), Color(0.62, 0.78, 0.52),
				"Ordre émis — la colonne part sous peu", VKit.FS_SMALL)
	elif powner >= 0 and powner != me:
		var dop: Dictionary = w.diplo_options(powner) if w.has_method("diplo_options") else {}
		if bool(dop.get("can_make_peace", false)):
			# EN GUERRE avec ce pays → projeter l'ost sur CETTE région (depuis la capitale)
			_act_chips(x, y, [["Attaquer ici", "campaign"]])
		else:
			# en paix → tenter une ROUTE commerciale depuis ma capitale (le moteur gate ports/pactes)
			_act_chips(x, y, [["Route terre", "route_land"], ["Route mer", "route_sea"]])
		# LOT P — PILLER LA CÔTE (pillage unifié : 20% du revenu annuel de la victime +
		# razzia si gate esclavagiste). Acte GRIS (pas de guerre requise — miroir de la
		# course pirate IA) ; gaté côtier/ni-allié-ni-pacte/CD/coque par can_raid_coast.
		if w.has_method("can_raid_coast"):
			var rd: Dictionary = w.can_raid_coast(_pid)
			var rr := int(rd.get("reason", 1))
			if bool(rd.get("legal", false)):
				_act_chips(x, y + 26, [["Piller la côte", "raid_coast"]])
			elif rr == 3:
				# le rappel de la scar : la vache est traite, l'immunité court encore
				VKit.text(self, Vector2(x, y + 29), VKit.COL_DIM,
					"Côte balafrée — %d j" % int(rd.get("cd_days", 0)), VKit.FS_SMALL)
			elif rr == 4:
				VKit.text(self, Vector2(x, y + 29), VKit.COL_DIM,
					"Piller la côte : aucune coque pirate", VKit.FS_SMALL)

	# ── HAUTEUR AU CONTENU : latch (1 frame de convergence) — plus de colonne vide.
	#    ⚠ set_deferred : poser `size` PENDANT _draw() est ignoré par Godot (le cadre
	#    restait court, le contenu débordait dessous — capture itération 1). ──
	var want := clampf(y + 60.0, 220.0,
		get_viewport_rect().size.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 24.0)
	if absf(_ph - want) > 3.0:
		_ph = want
		set_deferred("size", Vector2(PW, _ph))
		queue_redraw()

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

## UI-4 (retour joueur 2026-07-10) : Réprimer/Assimiler/Purger/Détail n'ont PLUS le même
## traitement — leur portée diffère radicalement (Purger est une punition de masse,
## irréversible ; Détail n'est qu'une navigation). Rendu à la main (pas _act_chips) :
##   · Réprimer/Assimiler : secondaires neutres (chip standard, comme avant).
##   · Purger : DESTRUCTIF rouge sombre, CONFIRMATION 2 clics (motif _servile_manumit_armed
##     de sidebar_drawer.gd), armé 4 s (cf. _process).
##   · Détail : navigation légère — texte seul, sans cadre, dans le ton discret.
## Chaque chip porte son HOVER de conséquences (façade action_preview si présente, sinon
## un texte factuel SANS chiffre inventé — discipline membrane).
func _draw_gov_actions(x: float, y: float, w) -> void:
	var reg: int = w.province_region(_pid)
	var cx := x
	for it in [["Réprimer", "repress"], ["Assimiler", "assimilate"]]:
		var label: String = it[0]
		var verb: String = it[1]
		var cw := VKit.text_w(label, VKit.FS_SMALL) + 14.0
		var r := Rect2(cx, y, cw, 20.0)
		VKit.fill(self, r, VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_GOLD)
		VKit.text(self, Vector2(cx + 7, y + 3), VKit.COL_PARCH, label, VKit.FS_SMALL)
		_acts.append([r, verb])
		_tips.append([r, _action_hover(verb, w, reg)])
		cx += cw + 6.0
	# ── PURGER : rouge sombre, destructif, 2 clics ──
	var plabel := "Confirmer la purge ?" if _purge_armed else "Purger"
	var pcw := VKit.text_w(plabel, VKit.FS_SMALL) + 14.0
	var pr := Rect2(cx, y, pcw, 20.0)
	var pcol := Color(0.86, 0.32, 0.22) if _purge_armed else Color(0.58, 0.17, 0.13)
	VKit.fill(self, pr, Color(0.18, 0.05, 0.04))
	VKit.box(self, pr, pcol)
	VKit.text(self, Vector2(cx + 7, y + 3), pcol, plabel, VKit.FS_SMALL)
	_acts.append([pr, "purge"])
	_tips.append([pr, "IRRÉVERSIBLE — cliquez de nouveau pour confirmer (4 s)" if _purge_armed
		else _action_hover("purge", w, reg)])
	cx += pcw + 12.0
	# ── DÉTAIL : navigation légère, pas un acte — pas de cadre ──
	var dlabel := "Détail"
	var dw := VKit.text_w(dlabel, VKit.FS_SMALL)
	var dr := Rect2(cx, y + 3, dw, 15.0)
	VKit.text(self, Vector2(cx, y + 4), VKit.COL_DIM, dlabel, VKit.FS_SMALL)
	_acts.append([dr, "detail"])
	_tips.append([dr, "Ouvrir le détail complet de la province (peuples, production, bâti, journal, main-d'œuvre)."])

## conséquences AVANT décision (retour joueur UI-4) : lit action_preview si la façade
## l'expose (verbe 0=RÉPRIMER 1=ASSIMILER 2=PURGER ; clés EXACTES du binding
## scps_sim_node.cpp:1276 — cost_gold · duration_days · pop_delta · satisfaction_delta ·
## agitation_delta · coercition_delta · risque), sinon un texte factuel SANS chiffre
## inventé — la membrane ne promet jamais un nombre que le moteur ne donne pas.
const _ACT_VERB_ID := {"repress": 0, "assimilate": 1, "purge": 2}
const _ACT_FALLBACK := {
	"repress": "Réprimer — mate l'agitation par la force. Baisse l'agitation ; hausse la coercition et le ressentiment.",
	"assimilate": "Assimiler — pousse la culture dominante sur les minorités. Réduit la friction culturelle ; prend du temps.",
	"purge": "Purger — réprime dans le sang la minorité la plus agitée. Pertes de population, chute de satisfaction, risque diplomatique. IRRÉVERSIBLE.",
}
func _action_hover(verb: String, w, reg: int) -> String:
	var fallback := String(_ACT_FALLBACK.get(verb, ""))
	if not w.has_method("action_preview") or not _ACT_VERB_ID.has(verb):
		return fallback
	var pv: Dictionary = w.action_preview(reg, int(_ACT_VERB_ID[verb]))
	var parts := PackedStringArray()
	var pop_d := int(pv.get("pop_delta", 0))
	if pop_d != 0:
		parts.append("%+d habitants" % pop_d)
	var sat_d := int(pv.get("satisfaction_delta", 0))
	if sat_d != 0:
		parts.append("satisfaction %+d" % sat_d)
	var agit_d := int(pv.get("agitation_delta", 0))
	if agit_d != 0:
		parts.append("agitation %+d" % agit_d)
	var coerc_d := int(pv.get("coercition_delta", 0))
	if coerc_d != 0:
		parts.append("coercition %+d" % coerc_d)
	var gold_d := int(round(float(pv.get("cost_gold", 0.0))))
	if gold_d != 0:
		parts.append("%d or" % gold_d)
	var days_d := int(pv.get("duration_days", 0))
	if days_d != 0:
		parts.append("%d j" % days_d)
	var risk := String(pv.get("risque", ""))
	# tout à zéro ET pas de risque nommé ⇒ la façade n'a rien à dire ici : texte factuel
	if parts.is_empty() and risk == "":
		return fallback
	var txt := (" · ".join(parts)) if not parts.is_empty() else fallback
	if risk != "":
		txt += (" · " if not parts.is_empty() else " ") + "risque : %s" % risk
	return txt

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
				Sound.play("moment_army_march")   # feedback : l'ost se met en marche
		"route_land", "route_sea":
			var capr2: int = w.province_region(w.country_capital_province(w.player()))
			if capr2 >= 0 and reg >= 0:
				w.player_route(capr2, reg, verb == "route_sea")
		"raid_coast":
			# LOT P — enfilé (drain revalidé) ; au tick suivant le chip devient
			# « Côte balafrée — X j » (le CD posé par le pillage réussi).
			w.player_raid_coast(_pid)
	Sim.notify_action()   # pause : les panneaux se rafraîchissent au clic
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
		if _collapse_rect.has_point(event.position):
			_collapsed = not _collapsed
			queue_redraw()
			accept_event()
			return
		if _build_rect.size.x > 0 and _build_rect.has_point(event.position):
			build_requested.emit()
		elif _colonize_rect.size.x > 0 and _colonize_rect.has_point(event.position):
			if Sim.world != null and Sim.world.has_method("player_colonize"):
				Sim.world.player_colonize(_pid); Sim.notify_action()   # enfilé ; refresh au drain (live)
				_colonize_ms = Time.get_ticks_msec()   # feedback : « ordre émis » 3 s
				Sound.play("ui_click")
				queue_redraw()
		else:
			for a in _acts:
				if (a[0] as Rect2).has_point(event.position):
					var verb := String(a[1])
					if verb == "purge" and not _purge_armed:
						# UI-4 : 1er clic ARME la confirmation (irréversible) — n'exécute rien
						_purge_armed = true
						_purge_armed_ms = Time.get_ticks_msec()
						Sound.play("ui_click")
						queue_redraw()
					else:
						if verb == "purge":
							_purge_armed = false
						_act_fire(verb)
					break

## HOVER natif : Godot appelle ceci au survol → texte de la zone touchée (classe / humeur).
func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""
