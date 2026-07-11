extends Control
## ProvinceDetail — le détail d'une province en SOUS-ONGLETS (touche V), read-only.
##   • Peuples    : camembert CULTURE + camembert RELIGION + classes + la jauge
##                  d'AGITATION et ses MODIFICATEURS (nom · apport signé · résorption
##                  /an pour les temporaires comme « Conquête récente »). Pas de prose.
##   • Production : les flux +X/j par bien (sprite de ressource sous la barre).
##   • Bâtiments  : les manufactures bâties (niveau + ouvriers).
## Pattern onglets + survol calqué sur sidebar_drawer. Charte parchemin (cuir/or).

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const VKitDropdown = preload("res://ui/vkit_dropdown.gd")
const Frame = preload("res://ui/frame.gd")
# taille ADAPTATIVE à la fenêtre (recalculée dans _layout ; plancher = l'ancienne taille fixe)
var PW := 648.0
var PH := 512.0
const HEAD := 34.0
const BODY := 92.0          # y de départ du corps d'onglet (sous titre + onglets)
const TABS := ["Peuples", "Production", "Constructions", "Journal", "Main-d'œuvre", "Contexte"]
const ALLOC_STEP := 10      # pas d'ajustement de poids (clic [−]/[+])

var _pid := -1
var _tab := 0
var _tab_rects := []        # [{rect, idx}] onglets cliquables
var _hover_zones := []      # [{rect, text}] survol des entrées de journal (effets)
var _hover_text := ""
var _hover_pos := Vector2.ZERO
var _alloc_btns := []       # [{rect, act, sink}] boutons de l'onglet Main-d'œuvre
var _alloc_cache := {}      # dernier readout region_alloc (pour pousser l'allocation COMPLÈTE)
var _close_rect := Rect2()
var _build_btn := Rect2()   # onglet Constructions : « Bâtir… » (ouvre le panneau de construction)
var _manuf_btns := []       # [{rect, bld}] onglet Constructions : boutons « Bâtir » (manufacture civile)
var _manuf_flash := ""      # retour du dernier ordre de manufacture
var _manuf_flash_ok := true

# ── LOT G — RÉINCORPORATION DE POP : deux menus déroulants (région A/B, mes régions),
#    un sélecteur de classe, une quantité, un bouton « Déplacer ». ──
const REINCORP_CLASSES := ["Laboureurs", "Artisans", "Noblesse", "Esclaves"]
const REINCORP_STEP := 500
var _reinc_owned := []        # [{region:int, nom:String}] mes régions (rafraîchi par frame)
var _reinc_a := 0             # index dans _reinc_owned (source)
var _reinc_b := 0             # index dans _reinc_owned (destination)
var _reinc_klass := 0
var _reinc_qty := 1000
var _reinc_dd_a
var _reinc_dd_b
var _reinc_btns := []          # [{rect, act}] classe ±, quantité ±, Déplacer
var _reinc_flash := ""
var _reinc_flash_ok := true

signal build_requested      ## Constructions → ouvrir le panneau de construction (sa maison)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.month_ticked.connect(func(_y): if visible: queue_redraw())   # flux/stocks : cadence mensuelle
	# LOT G — les deux menus déroulants région A/B (dessinés PAR-DESSUS le contenu
	# quand ouverts — ordre d'enfant = ordre de dessin, comme economy_panel).
	_reinc_dd_a = VKitDropdown.new()
	_reinc_dd_a.custom_minimum_size = Vector2(190, 24)
	_reinc_dd_a.size = Vector2(190, 24)
	_reinc_dd_a.visible = false
	add_child(_reinc_dd_a)
	_reinc_dd_a.selected.connect(func(i): _reinc_a = i; queue_redraw())
	_reinc_dd_b = VKitDropdown.new()
	_reinc_dd_b.custom_minimum_size = Vector2(190, 24)
	_reinc_dd_b.size = Vector2(190, 24)
	_reinc_dd_b.visible = false
	add_child(_reinc_dd_b)
	_reinc_dd_b.selected.connect(func(i): _reinc_b = i; queue_redraw())
	hide()

func _layout() -> void:
	# ZONE CONTEXTUELLE UNIQUE (UI-3, retour joueur 2026-07-10) : le détail REMPLACE le
	# panneau province — il s'ancre à la MÊME position (au lieu du centre) pour que le
	# regard reste au même endroit quand l'un cède la place à l'autre.
	var vp := get_viewport_rect().size
	PW = clampf(vp.x * 0.44, 648.0, 1000.0)
	PH = clampf(vp.y * 0.58, 512.0, 840.0)
	size = Vector2(PW, PH)
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 12.0)

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
	# ── BLASON de la PROVINCE (seed déterministe) + du PAYS PROPRIÉTAIRE, côte à côte ──
	var Heraldry = load("res://ui/heraldry.gd")
	var prov_arms: Texture2D = Heraldry.province_arms(_pid)
	var hx := 6.0
	if prov_arms != null:
		draw_texture_rect(prov_arms, Rect2(hx, 6, 22, 22), false)
		hx += 24.0
	var powner0 := int(info.get("owner", -1))
	if powner0 >= 0:
		var owner_arms: Texture2D = Heraldry.arms(powner0)
		if owner_arms != null:
			draw_texture_rect(owner_arms, Rect2(hx, 6, 22, 22), false)
			hx += 24.0
	if hx <= 6.0:
		UIKit.draw_icon(self, "capital_tower", Vector2(14, 8), 18)
		hx = 40.0
	VKit.text(self, Vector2(hx + 4.0, 9), VKit.COL_GOLD, "Province — %s" % String(info["nom"]), VKit.FS_BIG)

	# ✕ — tout panneau se ferme (Échap le ferme aussi via main)
	_close_rect = Rect2(PW - 26, 6, 20, 20)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 6, _close_rect.position.y + 3), VKit.COL_PARCH, "x")

	var hd_val_w: float = VKit.value(self, Vector2(x, HEAD + 4), "%s habitants" % _grp(info["ames"]), VKit.FS_SMALL)
	VKit.detail(self, Vector2(x + hd_val_w, HEAD + 4), " · %s · %s" % [info["climat"], info["relief"]], VKit.FS_SMALL)

	# ── onglets (chips cliquables) ─────────────────────────────────────────────
	_tab_rects.clear()
	var tx := x
	var ty := HEAD + 24.0
	for i in range(TABS.size()):
		var label: String = TABS[i]
		var tw := VKit.text_w(label, VKit.FS_SMALL) + 18.0
		var r := Rect2(tx, ty, tw, 20.0)
		var active := (_tab == i)
		VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		VKit.text(self, Vector2(tx + 9, ty + 2), VKit.COL_PANEL if active else VKit.COL_PARCH, label, VKit.FS_SMALL)
		_tab_rects.append({"rect": r, "idx": i})
		tx += tw + 6
	VKit.fill(self, Rect2(12, BODY - 6, PW - 24, 1), VKit.COL_EDGE)

	if _tab != 0:
		_reinc_dd_a.visible = false
		_reinc_dd_b.visible = false
	match _tab:
		0: _draw_peuples(x, BODY, w, info)
		1: _draw_flux(x, BODY + 4.0, PW - 32.0, PH - BODY - 18.0, w)
		2: _draw_batiments(x, BODY, w)
		3: _draw_journal(x, BODY, w)
		4: _draw_alloc(x, BODY, w, info)
		5: _draw_empire(x, BODY, w)

	# tooltip de survol (Journal : les effets de l'entrée)
	if _hover_text != "":
		var tip_lines := _hover_text.split("\n")
		var tw := 0.0
		for line in tip_lines:
			tw = maxf(tw, VKit.text_w(String(line), VKit.FS_SMALL) + 14.0)
		tw = minf(tw, PW - 12.0)
		var th := 6.0 + 15.0 * float(tip_lines.size())
		var tx2 := minf(_hover_pos.x + 14.0, PW - tw - 6.0)
		var ty2 := minf(maxf(4.0, _hover_pos.y - th - 4.0), PH - th - 4.0)
		VKit.fill(self, Rect2(tx2, ty2, tw, th), VKit.COL_PANEL2)
		VKit.box(self, Rect2(tx2, ty2, tw, th), VKit.COL_GOLD)
		for i in range(tip_lines.size()):
			VKit.text(self, Vector2(tx2 + 7, ty2 + 2 + i * 15), VKit.COL_PARCH, String(tip_lines[i]), VKit.FS_SMALL)

# ── ONGLET PEUPLES : DEUX COLONNES (LOT UI 2.5, 2026-07-11) — aperçu (médaillon de
#    culture + légende + classes + modificateurs) à GAUCHE, actions (foi + agitation
#    + réincorporation) à DROITE. Les grands camemberts culture/religion (76px de
#    diamètre chacun) deviennent des MÉDAILLONS compacts. ──
func _draw_peuples(x: float, y: float, w, info: Dictionary) -> void:
	var groups: Array = w.province_groups(_pid)
	if groups.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "province inhabitée", VKit.FS_SMALL)
		return
	var col_gap := 24.0
	var avail := PW - 2.0 * x
	var two_col := avail >= 560.0   # « quand le contenu le permet » — sinon empilé (colonne unique)
	var leftW: float = floor((avail - col_gap) * 0.56) if two_col else avail
	var rightW: float = (avail - col_gap - leftW) if two_col else avail
	var rx: float = (x + leftW + col_gap) if two_col else x

	var ly := _draw_peuples_apercu(x, y, leftW, w, info, groups)
	var ry := y if two_col else (ly + 10.0)
	_draw_peuples_actions(rx, ry, rightW, w, info)

# ── colonne GAUCHE : médaillon de CULTURE (compact) + légende + classes + modif. ──
func _draw_peuples_apercu(x: float, y: float, colw: float, w, info: Dictionary, groups: Array) -> float:
	# médaillon de culture : la DOMINANTE en pastille+nom+% (remplace le camembert
	# 76px) ; la légende (déjà TEXTE, déjà compacte) détaille les autres groupes.
	var dom_i := 0
	var dom_pct := -1
	for i in range(groups.size()):
		var p := int(groups[i]["percent"])
		if p > dom_pct:
			dom_pct = p; dom_i = i
	VKit.fill(self, Rect2(x + 2, y + 2, 18, 18), VKit.SLICE_PAL[dom_i % 8])
	VKit.box(self, Rect2(x + 2, y + 2, 18, 18), VKit.COL_GOLD)
	VKit.text(self, Vector2(x + 26, y), VKit.COL_GOLD, "Culture", VKit.FS_SMALL)
	var dom_lbl_w: float = VKit.text(self, Vector2(x + 26, y + 14), VKit.COL_PARCH,
		"%s (" % String(groups[dom_i]["culture"]), VKit.FS_SMALL)
	var dom_val_w: float = VKit.value(self, Vector2(x + 26 + dom_lbl_w, y + 14), "%d%%" % dom_pct, VKit.FS_SMALL)
	VKit.text(self, Vector2(x + 26 + dom_lbl_w + dom_val_w, y + 14), VKit.COL_PARCH, ")", VKit.FS_SMALL)
	var dom_lineage := String(groups[dom_i].get("lineage", ""))
	if dom_lineage != "":
		_hover_zones.append({"rect": Rect2(x + 22, y, colw - 22, 32.0), "text": dom_lineage})
	y += 36
	for i in range(mini(groups.size(), 6)):
		VKit.fill(self, Rect2(x + 4, y + 3, 9, 9), VKit.SLICE_PAL[i % 8])
		VKit.text(self, Vector2(x + 18, y), VKit.COL_PARCH,
			"%s %d%% · %s · %s" % [String(groups[i]["culture"]), int(groups[i]["percent"]),
			String(groups[i]["klass"]), String(groups[i]["etat"])], VKit.FS_SMALL)
		var lineage := String(groups[i].get("lineage", ""))
		if lineage != "":
			_hover_zones.append({"rect": Rect2(x, y - 1, colw, 15.0), "text": lineage})
		y += 15
	y += 8

	# classes (barre empilée, largeur de COLONNE) + ESCLAVES (4e segment, si présents)
	var cls: Dictionary = w.province_classes(_pid)
	var slaves: int = int(w.province_slave_count(_pid))
	var ccnt := [int(cls["laboureurs"]), int(cls["artisans"]), int(cls["noblesse"])]
	var cc := [VKit.SLICE_PAL[0], VKit.SLICE_PAL[1], VKit.SLICE_PAL[3]]
	var cnames := ["Laboureurs", "Artisans", "Noblesse"]
	if slaves > 0:
		ccnt.append(slaves)
		cc.append(Color(0.28, 0.26, 0.24))
		cnames.append("Esclaves")
	var tot: float = maxf(1.0, ccnt[0] + ccnt[1] + ccnt[2] + (slaves if slaves > 0 else 0))
	var rw := colw
	var acc := 0.0
	var nseg := ccnt.size()
	for i in range(nseg):
		var segw: float = (rw - acc) if i == nseg - 1 else float(ccnt[i]) / tot * rw
		segw = maxf(0.0, segw)
		VKit.fill(self, Rect2(x + acc, y, segw, 14), cc[i])
		acc += segw
	VKit.box(self, Rect2(x, y, rw, 14), VKit.COL_DIM)
	y += 19
	# légende des classes — EMPILÉE (une colonne de ~300px n'a pas la place des
	# 4×200px côte à côte de l'ancienne mise en page pleine largeur).
	for i in range(nseg):
		VKit.fill(self, Rect2(x + 2, y + 3, 9, 9), cc[i])
		var lbl: String = String(cnames[i])
		if i == 3:
			lbl = "%s (%d%%)" % [cnames[i], int(round(100.0 * float(ccnt[i]) / tot))]
		VKit.text(self, Vector2(x + 16, y), VKit.COL_PARCH, "%s %s" % [lbl, _grp(ccnt[i])], VKit.FS_SMALL)
		y += 15

	# ── FAVEURS & FLÉAUX (ScpsProvInfo.mods) ──
	var mods: Array = info.get("mods", [])
	if not mods.is_empty():
		y += 8
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "MODIFICATEURS", VKit.FS_SMALL)
		y += 16
		for m in mods:
			var faveur := bool(m.get("faveur", false))
			var mcol2 := VKit.sense(0.85) if faveur else VKit.sense(0.10)   # vert faveur / rouge fléau
			var mark2 := "+" if faveur else "−"
			VKit.text(self, Vector2(x + 8, y), mcol2, mark2, VKit.FS_SMALL)
			VKit.text(self, Vector2(x + 22, y), VKit.COL_PARCH, String(m.get("nom", "")), VKit.FS_SMALL)
			var heff := String(m.get("effet", ""))
			if heff != "":
				_hover_zones.append({"rect": Rect2(x, y - 1, colw, 15.0), "text": heff})
			y += 15
	return y

# ── colonne DROITE : médaillon de FOI (compact, absence NOMMÉE) + agitation + la
#    réincorporation (l'ACTION du joueur — LOT G). ──
func _draw_peuples_actions(x: float, y: float, colw: float, w, info: Dictionary) -> float:
	# médaillon de RELIGION : l'ÉTABLI (module scps_religion, P1-P8 — le monde naît
	# ATHÉE, cf. CLAUDE.md « FONDATION PAR ÉDIFICE ») — symbole (pastille) + mot ;
	# une ABSENCE (aucune foi encore fondée dans cette région) est NOMMÉE, jamais un
	# disque vide. Distinct de la « vision du monde » par groupe (légende de gauche).
	var region: int = w.province_region(_pid)
	var owner := int(info.get("owner", -2))
	var C_FAITH := Color(0.55, 0.42, 0.78)   # même violet que religion_panel.gd (cohérence de code couleur)
	var rid := (int(w.religion_of_region(region)) if w.has_method("religion_of_region") else -1)
	VKit.fill(self, Rect2(x + 2, y + 2, 18, 18), C_FAITH if rid >= 0 else VKit.COL_PANEL2)
	VKit.box(self, Rect2(x + 2, y + 2, 18, 18), C_FAITH)
	VKit.text(self, Vector2(x + 26, y), VKit.COL_GOLD, "Religion", VKit.FS_SMALL)
	if rid >= 0 and owner >= 0 and w.has_method("religion_name"):
		VKit.text(self, Vector2(x + 26, y + 14), VKit.COL_PARCH, String(w.religion_name(owner)), VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(x + 26, y + 14), VKit.COL_DIM, "Aucune foi établie", VKit.FS_SMALL)
	y += 40

	# ── AGITATION : la jauge + les MODIFICATEURS (nom · apport signé · résorption/an) ──
	var ag: Dictionary = w.province_agitation(_pid)
	var val := int(ag.get("value", 0))
	var acol := VKit.COL_DIM if val < 35 else (VKit.sense(0.45) if val < 70 else VKit.sense(0.10))
	UIKit.draw_icon(self, "menu_diplomacy", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 22, y), VKit.COL_PARCH, "Agitation", VKit.FS_SMALL)
	y += 17
	UIKit.bar(self, Rect2(x, y, colw - 40.0, 14), val)
	VKit.text(self, Vector2(x + colw - 34.0, y), acol, "%d/100" % val, VKit.FS_SMALL)
	y += 22
	var causes: Array = ag.get("causes", [])
	if causes.is_empty():
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, "province paisible", VKit.FS_SMALL)
		y += 16
	else:
		VKit.text(self, Vector2(x + 8, y), VKit.COL_DIM, "Modificateurs", VKit.FS_SMALL)
		y += 16
		for c in causes:
			var d := int(c["delta"])
			var dec := int(c.get("decay", 0))
			var mcol := VKit.sense(0.16) if d > 0 else VKit.sense(0.78)   # soulève rouge / apaise vert
			VKit.text(self, Vector2(x + 12, y), VKit.COL_PARCH, String(c["cause"]), VKit.FS_SMALL)
			var dlab := "%+d" % d
			VKit.text(self, Vector2(x + colw - 68.0, y), mcol, dlab, VKit.FS_SMALL)
			if dec > 0:
				VKit.text(self, Vector2(x + colw - 34.0, y), VKit.COL_DIM, "−%d/an" % dec, VKit.FS_SMALL)
			y += 16

	# ── RÉINCORPORATION DE POP (LOT G) — read-only si la province n'est pas au joueur ──
	var mine := (owner == int(w.player()))
	if mine and y < PH - 60.0:
		y += 8
		VKit.fill(self, Rect2(x, y, colw, 1), VKit.COL_EDGE)
		y += 10
		y = _draw_reincorp(x, y, w, colw)
	else:
		_reinc_dd_a.visible = false
		_reinc_dd_b.visible = false
	return y

# ── RÉINCORPORATION DE POP : sélecteur de classe + deux menus déroulants (région
#    A source / B destination, mes régions) + quantité + « Déplacer ». ──
func _refresh_reinc_owned(w) -> void:
	_reinc_owned.clear()
	var seen := {}
	var me := int(w.player())
	for pid in range(w.province_count()):
		var pi: Dictionary = w.province_info(pid)
		if int(pi.get("owner", -2)) != me:
			continue
		var region: int = w.province_region(pid)
		if seen.has(region):
			continue
		seen[region] = true
		_reinc_owned.append({"region": region, "nom": String(pi.get("nom", "région %d" % region))})

## réincorporation, LOT UI 2.5 : menus EMPILÉS (De : / Vers :, plus de côte à côte —
## une colonne de ~300px n'a pas la place de deux menus 190px + flèche) ; le bouton
## « Déplacer » indisponible affiche sa raison EXACTE au survol — miroir du SEUL gate
## que le drain (CMD_POP_TRANSFER, scps_sim.c) peut encore opposer une fois régions
## à soi + quantité>0 + classe valide déjà garanties par l'UI : source == destination.
func _draw_reincorp(x: float, y: float, w, colw: float) -> float:
	_reinc_btns.clear()
	_refresh_reinc_owned(w)
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Réincorporation", VKit.FS_SMALL)
	y += 18
	if _reinc_owned.size() < 2:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "il faut au moins DEUX régions à soi", VKit.FS_SMALL)
		_reinc_dd_a.visible = false
		_reinc_dd_b.visible = false
		return y + 16.0
	_reinc_a = clampi(_reinc_a, 0, _reinc_owned.size() - 1)
	_reinc_b = clampi(_reinc_b, 0, _reinc_owned.size() - 1)
	var names := []
	for r in _reinc_owned:
		names.append(String(r["nom"]))
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "De :", VKit.FS_SMALL)
	y += 14
	_reinc_dd_a.setup(names, _reinc_a)
	_reinc_dd_a.position = Vector2(x, y)
	_reinc_dd_a.visible = true
	y += 28
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "Vers :", VKit.FS_SMALL)
	y += 14
	_reinc_dd_b.setup(names, _reinc_b)
	_reinc_dd_b.position = Vector2(x, y)
	_reinc_dd_b.visible = true
	y += 28
	# classe ciblée (± cycle)
	var klass_name: String = REINCORP_CLASSES[_reinc_klass]
	var cx := _reinc_btn(x, y, "◂", "klass_prev")
	VKit.text(self, Vector2(cx + 4, y), VKit.COL_PARCH, klass_name, VKit.FS_SMALL)
	cx += VKit.text_w(klass_name, VKit.FS_SMALL) + 14
	cx = _reinc_btn(cx, y, "▸", "klass_next")
	y += 24
	# quantité (± STEP)
	cx = x
	cx = _reinc_btn(cx, y, "−", "qty_minus")
	VKit.value(self, Vector2(cx + 4, y), "%s âmes" % _grp(_reinc_qty), VKit.FS_SMALL)
	cx += VKit.text_w("%s âmes" % _grp(_reinc_qty), VKit.FS_SMALL) + 14
	cx = _reinc_btn(cx, y, "+", "qty_plus")
	y += 26
	# coût — coercition à la source pour les strates LIBRES ; esclave = aucune (main servile)
	var a_nom := String(_reinc_owned[_reinc_a]["nom"])
	var cost_txt := ("aucune — main servile" if _reinc_klass == 3 else "coercition sur %s" % a_nom)
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "coût : %s" % cost_txt, VKit.FS_SMALL)
	y += 22
	# bouton Déplacer — INTERDIT (grisé + raison) quand source == destination.
	var same := (_reinc_a == _reinc_b)
	var br := Rect2(x, y, 110, 22)
	VKit.fill(self, br, VKit.COL_PANEL2 if not same else VKit.COL_PANEL)
	VKit.box(self, br, VKit.COL_GOLD if not same else VKit.COL_DIM)
	VKit.text(self, Vector2(br.position.x + 12, br.position.y + 4), VKit.COL_PARCH if not same else VKit.COL_DIM, "Déplacer", VKit.FS_SMALL)
	if not same:
		_reinc_btns.append({"rect": br, "act": "move"})
	else:
		_hover_zones.append({"rect": br,
			"text": "Source et destination identiques — choisissez deux régions différentes (le moteur refuse un transfert région → elle-même)."})
	y += 22
	if _reinc_flash != "":
		VKit.text(self, Vector2(x, y), (VKit.sense(0.85) if _reinc_flash_ok else VKit.sense(0.10)), _reinc_flash, VKit.FS_SMALL)
		y += 16
	return y

func _reinc_btn(bx: float, by: float, label: String, act: String) -> float:
	var bw := VKit.text_w(label, VKit.FS_SMALL) + 12.0
	var r := Rect2(bx, by - 1, bw, 17.0)
	VKit.fill(self, r, VKit.COL_PANEL2); VKit.box(self, r, VKit.COL_EDGE)
	VKit.text(self, Vector2(bx + 6, by), VKit.COL_PARCH, label, VKit.FS_SMALL)
	_reinc_btns.append({"rect": r, "act": act})
	return bx + bw + 4.0

# ── ONGLET BÂTIMENTS : les manufactures bâties + BÂTIR (manufacture civile) ────
func _draw_batiments(x: float, y: float, w) -> void:
	# CONSTRUCTIONS : les bâtiments EXISTANTS (slots) + le sous-onglet BÂTIR (le
	# panneau de construction s'ouvre d'ici — c'est SA maison, plus un raccourci épars).
	_manuf_btns.clear()
	_build_btn = Rect2()
	var info: Dictionary = w.province_info(_pid)
	var mine := (int(info.get("owner", -2)) == int(w.player()))
	if mine:
		_build_btn = Rect2(PW - 130.0, y - 2.0, 110.0, 24.0)
		VKit.fill(self, _build_btn, VKit.COL_PANEL2)
		VKit.box(self, _build_btn, VKit.COL_GOLD)
		UIKit.draw_icon(self, "action_build", Vector2(_build_btn.position.x + 6, _build_btn.position.y + 4), 16)
		VKit.text(self, Vector2(_build_btn.position.x + 28, _build_btn.position.y + 4), VKit.COL_GOLD, "Bâtir…", VKit.FS_SMALL)
	var blds: Array = w.province_buildings(_pid)
	if blds.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune manufacture bâtie (carte nue : l'IA/le joueur élèvent dans le temps)", VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "manufacture                niveau   ouvriers", VKit.FS_SMALL)
		y += 18
		var maxlv := 1
		for b in blds:
			maxlv = maxi(maxlv, int(b["niveau"]))
		for b in blds:
			if y > PH - 24:
				break
			# vignette de manufacture (planche 8) quand elle existe, sinon le marteau
			var mt: Texture2D = UIKit.manuf_sprite(String(b["nom"]))
			if mt != null:
				draw_texture_rect(mt, Rect2(x - 1, y - 2, 16, 16), false)
			else:
				UIKit.draw_icon(self, "build_hammer", Vector2(x, y - 1), 14)
			VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, String(b["nom"]), VKit.FS_SMALL)
			var lv := int(b["niveau"])
			VKit.fill(self, Rect2(x + 230, y + 2, 90.0 * float(lv) / float(maxlv), 10), VKit.COL_GOLD)
			VKit.value(self, Vector2(x + 326, y), str(lv), VKit.FS_SMALL)
			VKit.text(self, Vector2(x + 380, y), VKit.COL_DIM, _grp(b["ouvriers"]), VKit.FS_SMALL)
			y += 19

	# — BÂTIR une manufacture civile ICI (région de la province sélectionnée) —
	if not mine or y > PH - 40:
		return
	y += 10
	VKit.fill(self, Rect2(x, y, PW - 32.0, 1), VKit.COL_EDGE)
	y += 10
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Bâtir une manufacture", VKit.FS_SMALL)
	y += 18
	var region: int = w.province_region(_pid)
	# lot M — le PRIX du chantier (le montant que le drain débite : MANUF_BUILD_COST×ipm)
	var mcost: int = int(w.manuf_cost()) if w.has_method("manuf_cost") else 0
	var any_legal := false
	for bld in range(24):   # BLD_TYPE_COUNT (miroir display-only côté binding)
		if y > PH - 22:
			break
		if int(w.manuf_legal(region, bld)) != 1:
			continue
		any_legal = true
		var nom := String(w.manuf_name(bld))
		var mt2: Texture2D = UIKit.manuf_sprite(nom)
		if mt2 != null:
			draw_texture_rect(mt2, Rect2(x - 1, y - 2, 16, 16), false)
		else:
			UIKit.draw_icon(self, "build_hammer", Vector2(x, y - 1), 14)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, nom, VKit.FS_SMALL)
		var blab := ("Bâtir · %d or" % mcost) if mcost > 0 else "Bâtir"
		var bw := VKit.text_w(blab, VKit.FS_SMALL) + 20.0
		var br := Rect2(x + 334.0 - bw, y - 2, bw, 18)
		VKit.fill(self, br, VKit.COL_PANEL2)
		VKit.box(self, br, VKit.COL_GOLD)
		VKit.value(self, Vector2(br.position.x + 10, br.position.y + 2), blab, VKit.FS_SMALL)
		_manuf_btns.append({"rect": br, "bld": bld})
		y += 20
	if not any_legal:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune manufacture posable ici pour l'instant", VKit.FS_SMALL)
	if _manuf_flash != "":
		VKit.text(self, Vector2(x, PH - 18), (VKit.sense(0.85) if _manuf_flash_ok else VKit.sense(0.10)), _manuf_flash, VKit.FS_SMALL)

# ── ONGLET CONTEXTE (ex-« Empire ») : le MARCHÉ LOCAL de cette province (prix/
#    stock/bande des biens majeurs + le port s'il existe) + les JAUGES d'État de
#    l'EMPIRE entier (sorties de la barre du haut ; savoir en topbar). ──
const EMPIRE_BANDS := [
	["stabilite",  "Stabilité",  "stability_shield"],
	["prosperite", "Prospérité", "prosperity_sprout"],
	["legitimite", "Légitimité", "politics_crown"],
	["cohesion",   "Cohésion",   "happiness_medallion"],
]
func _draw_empire(x: float, y: float, w) -> void:
	# ── MARCHÉ LOCAL (LOT 6) : prix/stock des biens de LA PROVINCE + le port ──
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "Marché local", VKit.FS_SMALL)
	y += 18
	var mk: Dictionary = w.province_market(_pid)
	var port := String(mk.get("port", ""))
	if port != "":
		UIKit.draw_icon(self, "trade_ship", Vector2(x, y - 2), 16)
		VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, port, VKit.FS_SMALL)
		y += 18
	var lines: Array = mk.get("lines", [])
	if lines.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucun bien notable ici", VKit.FS_SMALL)
		y += 18
	else:
		for l in lines:
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, String(l.get("name", "")), VKit.FS_SMALL)
			VKit.value(self, Vector2(x + 140, y), "%.1f or" % float(l.get("price", 0.0)), VKit.FS_SMALL)
			VKit.text(self, Vector2(x + 220, y), VKit.COL_DIM, "stock %s" % _grp(int(l.get("stock", 0))), VKit.FS_SMALL)
			VKit.text(self, Vector2(x + 340, y), VKit.sense(0.62), String(l.get("marche", "")), VKit.FS_SMALL)
			y += 17
	y += 10
	VKit.fill(self, Rect2(x, y, PW - 32.0, 1), VKit.COL_EDGE)
	y += 12

	# ── L'ÉTAT DE L'EMPIRE ENTIER (inchangé — la province n'en est qu'une part) ──
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	if not bool(ci.get("valide", false)):
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "(pays invalide)", VKit.FS_SMALL)
		return
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD, String(ci.get("nom", "")), VKit.FS_BIG)
	y += 26
	VKit.text(self, Vector2(x, y), VKit.COL_DIM,
		"L'état de l'EMPIRE entier (la province n'en est qu'une part).", VKit.FS_SMALL)
	y += 22
	for band in EMPIRE_BANDS:
		var v := int(ci.get(band[0], 0))
		UIKit.draw_icon(self, band[2], Vector2(x, y - 2), 18)
		VKit.text(self, Vector2(x + 24, y), VKit.COL_PARCH, band[1], VKit.FS_SMALL)
		UIKit.bar(self, Rect2(x + 130, y + 2, 200, 12), v)
		VKit.value(self, Vector2(x + 338, y), str(v), VKit.FS_SMALL)
		y += 24
	y += 8
	UIKit.draw_icon(self, "knowledge_book", Vector2(x, y - 2), 16)
	VKit.text(self, Vector2(x + 22, y), VKit.COL_DIM,
		"Savoir : %d (affiché en barre du haut)" % int(ci.get("savoir", 0)), VKit.FS_SMALL)

# ── ONGLET MAIN-D'ŒUVRE : allocation des bras par PUITS (extraction + manufactures) ──
#    Régler les % (somme normalisée), FERMER un bâtiment (poids 0), choisir l'INTRANT.
#    Le joueur ne règle QUE ses régions ; sinon lecture seule. Les verbes ENFILENT (drain).
func _draw_alloc(x: float, y: float, w, info: Dictionary) -> void:
	_alloc_btns.clear()
	var region: int = w.province_region(_pid)
	var mine := (int(info.get("owner", -1)) == int(w.player()))
	var al: Dictionary = w.region_alloc(region)
	_alloc_cache = al
	var on := bool(al.get("on", false))
	var sinks: Array = al.get("sinks", [])
	# — entête : bassin + mode + bouton Auto —
	VKit.text(self, Vector2(x, y), VKit.COL_GOLD,
		"Bassin : %s bras (journaliers + bourgeois)" % _grp(al.get("pool", 0)), VKit.FS_SMALL)
	var mode_txt := ("RÉPARTI (manuel)" if on else "AUTO (réparti par le marché)")
	VKit.text(self, Vector2(x, y + 18), VKit.COL_DIM, "mode : %s" % mode_txt, VKit.FS_SMALL)
	if mine and on:
		var ar := Rect2(PW - 96, y - 2, 78, 18)
		VKit.fill(self, ar, VKit.COL_PANEL2); VKit.box(self, ar, VKit.COL_GOLD)
		VKit.text(self, Vector2(ar.position.x + 10, ar.position.y + 2), VKit.COL_PARCH, "↻ Auto", VKit.FS_SMALL)
		_alloc_btns.append({"rect": ar, "act": "auto", "sink": -1})
	if not mine:
		VKit.text(self, Vector2(x, y + 36), VKit.sense(0.5), "(province non gouvernée — lecture seule)", VKit.FS_SMALL)
	if sinks.is_empty():
		VKit.text(self, Vector2(x, y + 54), VKit.COL_DIM, "aucun puits de main-d'œuvre ici", VKit.FS_SMALL)
		return
	VKit.fill(self, Rect2(12, y + 38, PW - 24, 1), VKit.COL_EDGE)
	var ry := y + 46.0
	for i in range(sinks.size()):
		if ry > PH - 26:
			break
		var s: Dictionary = sinks[i]
		var is_bld := (int(s.get("kind", 0)) == 1)
		var closed := bool(s.get("closed", false))
		# icône + nom (→ sortie pour les manufactures)
		if is_bld:
			var mt2: Texture2D = UIKit.manuf_sprite(String(s.get("name", "")))
			if mt2 != null:
				draw_texture_rect(mt2, Rect2(x - 1, ry - 2, 16, 16), false)
			else:
				UIKit.draw_icon(self, "build_hammer", Vector2(x, ry - 1), 14)
		else:
			var spr := UIKit.resource_sprite(int(s.get("id", -1)), String(s.get("name", "")))
			if spr != null: draw_texture_rect(spr, Rect2(x - 2, ry - 2, 18, 18), false)
		var nm := String(s.get("name", ""))
		var ncol := VKit.COL_DIM if closed else VKit.COL_PARCH
		VKit.text(self, Vector2(x + 20, ry), ncol, nm, VKit.FS_SMALL)
		if is_bld and String(s.get("output", "")) != "":
			VKit.text(self, Vector2(x + 150, ry), VKit.COL_DIM, "→ %s" % String(s["output"]), VKit.FS_SMALL)
		# barre de part (%)
		var pct := int(s.get("pct", 0))
		var bx := x + 250.0
		VKit.fill(self, Rect2(bx, ry + 2, 80, 10), VKit.COL_PANEL2)
		if not closed:
			VKit.fill(self, Rect2(bx, ry + 2, 80.0 * float(pct) / 100.0, 10), VKit.COL_GOLD)
		VKit.box(self, Rect2(bx, ry + 2, 80, 10), VKit.COL_EDGE)
		VKit.value(self, Vector2(bx + 86, ry), "%d%%" % pct, VKit.FS_SMALL)
		# contrôles (région à soi seulement)
		if mine:
			var cx := bx + 122.0
			cx = _alloc_btn(cx, ry, "−", "minus", i)
			cx = _alloc_btn(cx, ry, "+", "plus", i)
			if is_bld:
				cx = _alloc_btn(cx, ry, ("Ouvrir" if closed else "Fermer"), "close", i)
				if String(s.get("alt_name", "")) != "" and int(s.get("input", -1)) >= 0:
					var inp := int(s["input"])
					var lab := ("Intr.: %s" % (String(s["alt_name"]) if inp == 1 else String(s.get("in_name", "?"))))
					cx = _alloc_btn(cx, ry, lab, "input", i)
		ry += 20.0

# petit bouton cliquable de l'onglet alloc ; renvoie le x suivant
func _alloc_btn(bx: float, by: float, label: String, act: String, sink: int) -> float:
	var bw := VKit.text_w(label, VKit.FS_SMALL) + 12.0
	var r := Rect2(bx, by - 1, bw, 17.0)
	VKit.fill(self, r, VKit.COL_PANEL2); VKit.box(self, r, VKit.COL_EDGE)
	VKit.text(self, Vector2(bx + 6, by), VKit.COL_PARCH, label, VKit.FS_SMALL)
	_alloc_btns.append({"rect": r, "act": act, "sink": sink})
	return bx + bw + 4.0

# applique une édition d'allocation : pousse l'allocation COMPLÈTE (un seul puits réglé
# mettrait les autres à 0) — région à soi, revalidée au drain.
func _alloc_apply(region: int, idx: int, new_w: int) -> void:
	var w = Sim.world
	if w == null: return
	var sinks: Array = _alloc_cache.get("sinks", [])
	for i in range(sinks.size()):
		var s: Dictionary = sinks[i]
		var ww := (new_w if i == idx else int(s.get("weight", 0)))
		ww = clampi(ww, 0, 100)
		if int(s.get("kind", 0)) == 0:
			w.player_alloc_raw(region, int(s.get("id", 0)), ww)
		else:
			w.player_alloc_bld(region, int(s.get("id", 0)), ww)

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
	VKit.text(self, Vector2(fx, fy), VKit.COL_GOLD, "Production en direct (par an)", VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx + 200.0, fy + 2.0, 9, 9), VKit.COL_GOLD)
	VKit.text(self, Vector2(fx + 214.0, fy), VKit.COL_DIM, "ressource brute", VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx + 330.0, fy + 2.0, 9, 9), VKit.SLICE_PAL[7])
	VKit.text(self, Vector2(fx + 344.0, fy), VKit.COL_DIM, "sortie d'atelier", VKit.FS_SMALL)
	var tax := float(w.province_tax(_pid))
	if tax > 0.5:
		var tax_txt := "Impôts : ~%s or/an" % _grp(int(round(tax)))
		VKit.value(self, Vector2(fx + fw - VKit.text_w(tax_txt, VKit.FS_SMALL), fy), tax_txt, VKit.FS_SMALL)
	var inc: Array = w.province_income(_pid)
	if inc.is_empty():
		VKit.text(self, Vector2(fx, fy + 24.0), VKit.COL_DIM, "rien de notable", VKit.FS_SMALL)
		return
	var n := inc.size()
	var vals := []
	var maxv := 1.0
	for l in inc:
		# Calibrage « Anno » : les FLUX se lisent en unités/JOUR à 1 décimale (le readout
		# livre déjà per_day) — plus de ×365 qui gonflait l'échelle en dizaines de milliers.
		var v := float(l["per_day"])
		vals.append(v)
		maxv = maxf(maxv, absf(v))
	var top := fy + 24.0
	var base := fy + fh - 26.0
	var barmax := base - top - 12.0
	for g in range(0, 3):
		var gy := base - float(g) / 2.0 * barmax
		VKit.fill(self, Rect2(fx, gy, fw, 1), VKit.COL_EDGE)
		VKit.text(self, Vector2(fx + fw - 44.0, gy - 12.0), VKit.COL_DIM, "%.1f" % (float(g) / 2.0 * maxv), VKit.FS_SMALL)
	var slot := fw / float(n)
	var bw := minf(36.0, slot * 0.62)
	for i in range(n):
		var cx := fx + (float(i) + 0.5) * slot
		var v: float = vals[i]
		var bhh: float = absf(v) / maxv * barmax
		var manuf := bool(inc[i]["manufactured"])
		var col := VKit.SLICE_PAL[7] if manuf else VKit.COL_GOLD
		VKit.fill(self, Rect2(cx - bw / 2.0, base - bhh, bw, bhh), col)
		var vs := "+%.1f/j" % v
		VKit.value(self, Vector2(cx - VKit.text_w(vs, VKit.FS_SMALL) / 2.0, base - bhh - 13.0), vs, VKit.FS_SMALL)
		var spr := UIKit.resource_sprite(int(inc[i].get("res_id", -1)), String(inc[i]["source"]))
		if spr != null:
			draw_texture_rect(spr, Rect2(cx - 10.0, base + 3.0, 20.0, 20.0), false)
		else:
			VKit.text(self, Vector2(cx - 14.0, base + 5.0), VKit.COL_DIM, String(inc[i]["source"]).substr(0, 5), VKit.FS_SMALL)
	VKit.fill(self, Rect2(fx, base, fw, 1), VKit.COL_DIM)

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(event.position):
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
			return
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
		# onglet Constructions : « Bâtir… » ouvre le panneau de construction
		if _tab == 2 and _build_btn.size.x > 0 and _build_btn.has_point(event.position):
			build_requested.emit()
			accept_event()
			return
		# onglet Constructions : « Bâtir » une manufacture civile DANS cette province
		if _tab == 2:
			for b in _manuf_btns:
				if b.rect.has_point(event.position):
					var w2 = Sim.world
					if w2 == null:
						return
					var region2: int = w2.province_region(_pid)
					var nom2 := String(w2.manuf_name(int(b.bld)))
					# Les ordres sont ENFILÉS (journal déterministe) : le retour n'est que
					# « mis en file », pas le verdict d'application (qui tombe au tick).
					var ok2: bool = w2.player_build_manuf(region2, int(b.bld)); Sim.notify_action()  # → refresh au drain (live)
					_manuf_flash_ok = ok2
					_manuf_flash = ("⚒ %s — ordre émis" % nom2) if ok2 else ("✗ %s — refusé" % nom2)
					if not ok2:
						Sound.play("ui_click")
					queue_redraw()
					accept_event()
					return
		# onglet Main-d'œuvre : boutons d'allocation (poids ± · fermer · intrant · auto)
		for b in _alloc_btns:
			if b.rect.has_point(event.position):
				var w = Sim.world
				if w == null:
					return
				var region: int = w.province_region(_pid)
				var idx: int = b.sink
				var sinks: Array = _alloc_cache.get("sinks", [])
				match b.act:
					"auto":
						w.player_alloc_auto(region)
					"minus":
						if idx >= 0 and idx < sinks.size():
							_alloc_apply(region, idx, int(sinks[idx].get("weight", 0)) - ALLOC_STEP)
					"plus":
						if idx >= 0 and idx < sinks.size():
							_alloc_apply(region, idx, int(sinks[idx].get("weight", 0)) + ALLOC_STEP)
					"close":
						if idx >= 0 and idx < sinks.size():
							var cl := bool(sinks[idx].get("closed", false))
							_alloc_apply(region, idx, (ALLOC_STEP if cl else 0))
					"input":
						if idx >= 0 and idx < sinks.size():
							w.player_alloc_input(region, int(sinks[idx].get("id", 0)), 1 - int(sinks[idx].get("input", 0)))
				queue_redraw()
				accept_event()
				return
		# onglet Peuples : RÉINCORPORATION DE POP (LOT G)
		if _tab == 0:
			for b in _reinc_btns:
				if b.rect.has_point(event.position):
					match b.act:
						"klass_prev":
							_reinc_klass = (_reinc_klass - 1 + REINCORP_CLASSES.size()) % REINCORP_CLASSES.size()
						"klass_next":
							_reinc_klass = (_reinc_klass + 1) % REINCORP_CLASSES.size()
						"qty_minus":
							_reinc_qty = maxi(REINCORP_STEP, _reinc_qty - REINCORP_STEP)
						"qty_plus":
							_reinc_qty += REINCORP_STEP
						"move":
							var w3 = Sim.world
							if w3 != null and _reinc_a < _reinc_owned.size() and _reinc_b < _reinc_owned.size():
								var ra: int = _reinc_owned[_reinc_a]["region"]
								var rb: int = _reinc_owned[_reinc_b]["region"]
								var ok3: bool = w3.player_pop_transfer(ra, rb, _reinc_klass, _reinc_qty)
								_reinc_flash_ok = ok3
								_reinc_flash = ("→ %s — ordre émis" % REINCORP_CLASSES[_reinc_klass]) if ok3 else "✗ refusé"
								if not ok3:
									Sound.play("ui_click")
					queue_redraw()
					accept_event()
					return
		for t in _tab_rects:
			if t.rect.has_point(event.position):
				if _tab != t.idx:
					Sound.play("ui_click")
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
