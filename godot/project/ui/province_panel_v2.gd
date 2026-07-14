extends PanelContainer
## ProvincePanelV2 — la fiche PROVINCE bâtie avec des CONTENEURS Godot NATIFS
## (PanelContainer/VBox/HBox/Grid/Margin) + le THEME parchemin PARTAGÉ (parch_theme.gd).
## ZÉRO `_draw` : la mise en page s'auto-espace, la hauteur suit le contenu.
## COEXISTE avec province_panel.gd (ne le touche pas) — c'est le pendant « conteneurs
## natifs » du concept parchemin, comme budget_panel_v2 l'a fait pour le budget.
##
## Display-only, LECTURE SEULE : lit la MÊME membrane que province_panel (province_info,
## province_capitale, province_classes/class_sat, income, buildings/edifices…), tout
## `has_method`-gardé. Bascule touche V (câblée dans main.gd). Sous-onglets façon Vic3.

const ParchTheme = preload("res://ui/parch_theme.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const PopBar = preload("res://ui/pop_bar.gd")

const PW := 384.0   ## largeur plafond (lignes par classe + boutons collés)
const ALLOC_STEP := 10   ## pas de répartition raw (poids 0-100)

## le MENU CONSTRUCTION s'ouvre depuis la fiche (bouton « Construire… ») — kind : 0 Édifices, 1 Manufactures.
signal build_requested(kind: int)

var _pid := -1
var _region := -1                   ## région moteur (agrégat lu SEULEMENT par l'onglet RÉGION —
                                     ## RE-KEY PROVINCE : les verbes/alloc utilisent _pid directement)
var _alloc := {}                    ## dernier province_alloc (pousser l'allocation COMPLÈTE)
var _name2bld := {}                 ## nom de manufacture → BuildingType (résout le type pour les verbes)
var _income := {}                   ## dernier province_income : nom du bien → per_day (manufacturés SEULEMENT)
var _grow_pid := -2                 ## CROISSANCE (display-only) : pid mesuré au refresh précédent
var _grow_total := -1.0             ## … pop totale à ce refresh
var _grow_day := -1                 ## … et le jour absolu (year()×365+day_of_year())
var _flash := ""                    ## retour transitoire « ordre émis » (effacé au refresh)
var _tab := 0                       ## 0 Infrastructure (fusionné) · 1 Militaire
var _body: VBoxContainer = null     ## corps rebâti à chaque refresh / changement d'onglet
var _title_lbl: Label = null
var _sub_lbl: Label = null
var _owner_lbl: Label = null
var _ownersub_lbl: Label = null
var _tab_group: ButtonGroup = null
var _tab_btns: Array = []            ## [Button] pour piloter l'onglet actif par code (probe)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, 0)   # largeur plancher/plafond ; hauteur AU CONTENU
	position = Vector2(Frame.SIDEBAR_W + 14.0, Frame.TOPBAR_H + 12.0)
	theme = ParchTheme.build()
	_build_shell()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): if visible: refresh())
	hide()

# ── LE SQUELETTE (header + onglets + corps) ───────────────────────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER : nom + tier/biome à gauche · propriétaire aligné à droite
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	var lcol := VBoxContainer.new()
	lcol.add_theme_constant_override("separation", 0)
	lcol.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(lcol)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "—"
	_title_lbl.clip_text = true
	lcol.add_child(_title_lbl)
	_sub_lbl = Label.new()
	_sub_lbl.theme_type_variation = "RowDim"
	_sub_lbl.text = "—"
	lcol.add_child(_sub_lbl)
	var rcol := VBoxContainer.new()
	rcol.add_theme_constant_override("separation", 0)
	rcol.size_flags_horizontal = Control.SIZE_SHRINK_END
	hb.add_child(rcol)
	_owner_lbl = Label.new()
	_owner_lbl.theme_type_variation = "RowLabel"
	_owner_lbl.text = ""
	_owner_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	rcol.add_child(_owner_lbl)
	_ownersub_lbl = Label.new()
	_ownersub_lbl.theme_type_variation = "RowDim"
	_ownersub_lbl.text = ""
	_ownersub_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	rcol.add_child(_ownersub_lbl)

	# BARRE D'ONGLETS façon Vic3
	var tabpanel := PanelContainer.new()
	tabpanel.theme_type_variation = "LedTabStrip"
	root.add_child(tabpanel)
	var tabs := HBoxContainer.new()
	tabs.add_theme_constant_override("separation", 2)
	tabpanel.add_child(tabs)
	_tab_group = ButtonGroup.new()
	_tab_btns.clear()
	var names := ["Infrastructure", "Région", "Militaire"]
	for i in range(names.size()):
		var b := Button.new()
		b.theme_type_variation = "Tab"
		b.toggle_mode = true
		b.button_group = _tab_group
		b.text = names[i]
		b.focus_mode = Control.FOCUS_NONE
		if i == _tab:
			b.button_pressed = true
		var idx := i
		b.pressed.connect(func(): _tab = idx; refresh())
		tabs.add_child(b)
		_tab_btns.append(b)

	# CORPS (fond transparent — laisse voir le parchemin)
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	root.add_child(bodypanel)
	_body = VBoxContainer.new()
	_body.add_theme_constant_override("separation", 4)
	bodypanel.add_child(_body)

# ── API publique ──────────────────────────────────────────────────────────────
func show_province(pid: int) -> void:
	if pid != _pid:
		_grow_pid = -2   # nouvelle province : la CROISSANCE repart de zéro (pas de faux delta)
	_pid = pid
	visible = pid >= 0
	if visible:
		refresh()

## sélection PUBLIQUE d'un onglet (par code) : met aussi à jour le bouton actif (le
## soulignement suit). Utilisée par la sonde de capture.
func select_tab(idx: int) -> void:
	if idx >= 0 and idx < _tab_btns.size():
		_tab_btns[idx].button_pressed = true
	_tab = idx
	refresh()

func refresh() -> void:
	var w = Sim.world
	if w == null or _pid < 0 or _body == null:
		return
	var info: Dictionary = w.province_info(_pid)
	if not bool(info.get("valide", false)):
		return
	var cap: Dictionary = w.province_capitale(_pid)
	# région moteur + allocation (grain des verbes) + carte nom→BuildingType (résout les types)
	_region = int(w.province_region(_pid)) if w.has_method("province_region") else -1
	_alloc = w.province_alloc(_pid) if (_pid >= 0 and w.has_method("province_alloc")) else {}
	if _name2bld.is_empty() and w.has_method("manuf_name"):
		for bld in range(24):   # BLD_TYPE_COUNT (miroir display-only)
			var nm := String(w.manuf_name(bld))
			if nm != "" and nm != "?":
				_name2bld[nm] = bld
	# PRODUCTION manufacturée (chantier 3) : nom du BIEN produit → unités/mois (per_day×30),
	# pour le hover des chips de manufacture (matché par le bien via manuf_recipe(bld).out).
	_income.clear()
	if w.has_method("province_income"):
		for l in w.province_income(_pid):
			if bool(l.get("manufactured", false)):
				_income[String(l.get("source", ""))] = float(l.get("per_day", 0.0))
	_update_header(w, info, cap)
	for c in _body.get_children():
		c.queue_free()
	match _tab:
		1: _build_region(w, info, cap)
		2: _build_militaire(w, info, cap)
		_: _build_infrastructure(w, info, cap)
	if _flash != "":
		_line(_flash, "Income")
	# hug content après reconstruction (largeur plafonnée par custom_minimum_size)
	reset_size.call_deferred()

func _update_header(w, info: Dictionary, cap: Dictionary) -> void:
	_title_lbl.text = String(info.get("nom", "Province"))
	_sub_lbl.text = "tier %d · %s" % [int(cap.get("tier", 0)), String(info.get("relief", ""))]
	var owner := int(info.get("owner", -1))
	if owner >= 0 and w.has_method("country_info"):
		var ci: Dictionary = w.country_info(owner)
		_owner_lbl.text = String(ci.get("nom", ""))
	elif owner < 0:
		_owner_lbl.text = "Terre libre"
	else:
		_owner_lbl.text = ""
	_ownersub_lbl.text = String(cap.get("statut", ""))

# ── ONGLET INFRASTRUCTURE (fusionné : la province PAR CLASSE) ─────────────────
## Retour joueur 2026-07-13 : Infrastructure + Démographie FONDUES. Chaque CLASSE
## sociale porte SA colonne d'activité — Journaliers/Esclaves la RÉPARTITION par raw
## (extraction), Bourgeois les MANUFACTURES, Élites les ÉDIFICES — avec des [−][+]
## collés (répartition, niveau, poser/démolir). Culture & foi en frises au HOVER.
func _build_infrastructure(w, info: Dictionary, _cap: Dictionary) -> void:
	var mine := (int(info.get("owner", -2)) == int(w.player())) if w.has_method("player") else false

	# TERRAIN — hover = image du biome + tenue de siège + habitabilité (chantier 5)
	var def_pct := int(w.province_defense_pct(_pid)) if w.has_method("province_defense_pct") else 100
	_terrain_row(info, def_pct)
	var grid := _grid()
	_kv(grid, "Population", _grp(info.get("ames", 0)), ParchTheme.INK)
	# CROISSANCE (display-only, motif empire_window) : delta signé /mois depuis le
	# dernier refresh, normalisé sur le jour absolu réel — « — » sans mesure précédente
	# ou si la province vient de changer (pas de faux delta entre deux provinces).
	var pop_now := float(info.get("ames", 0))
	var abs_day := int(w.year()) * 365 + (int(w.day_of_year()) if w.has_method("day_of_year") else 0)
	if _grow_pid == _pid and _grow_total >= 0.0 and abs_day > _grow_day:
		var per_month := (pop_now - _grow_total) / float(abs_day - _grow_day) * 30.0
		var pos := per_month >= 0.0
		_kv(grid, "Croissance", "%s%s âmes/mois" % ["+" if pos else "−", _grp(int(round(absf(per_month))))],
			ParchTheme.INCOME if pos else ParchTheme.EXPENSE)
	else:
		_kv(grid, "Croissance", "—", ParchTheme.DIM_INK)
	_grow_pid = _pid; _grow_total = pop_now; _grow_day = abs_day
	var tax := float(w.province_tax(_pid)) if w.has_method("province_tax") else 0.0
	_kv(grid, "Impôts", "~%s or/mois" % _grp(int(round(tax))), ParchTheme.INK)
	var aisance := int(info.get("aisance_val", 0))
	_kv(grid, "Prospérité", "%d%%" % aisance, _score_col(aisance))
	var mood := int(info.get("humeur_val", 0))
	_kv(grid, "Loyauté", "%d%%" % mood, _score_col(mood))
	var agit := int(info.get("agitation", 0))
	if w.has_method("province_agitation"):
		agit = int(w.province_agitation(_pid).get("value", agit))
	_kv(grid, "Agitation", "%d%%" % agit, ParchTheme.RED if agit >= 50 else ParchTheme.DIM_INK)
	# LOGEMENTS & SERVICES (chantier 4) : âmes logées/servies sur la capacité — le
	# BÂTI (manufactures-logements, confort) monte ces plafonds au-delà de la terre nue.
	var lc := int(info.get("logements_cap", 0))
	if lc > 0:
		_kv(grid, "Logements", "%s / %s" % [_grp(info.get("logements_libres", 0)), _grp(lc)],
			ParchTheme.RED if int(info.get("logements_libres", 0)) <= 0 else ParchTheme.DIM_INK)
	var sc := int(info.get("services_cap", 0))
	if sc > 0:
		_kv(grid, "Services", "%s / %s" % [_grp(info.get("services_libres", 0)), _grp(sc)],
			ParchTheme.RED if int(info.get("services_libres", 0)) <= 0 else ParchTheme.DIM_INK)
	if bool(info.get("seuil_revolte", false)):
		_line("⚠ Au bord de la révolte (agitation %d)" % agit, "Expense")
	# FRICHE (E1bis.10) : entretien/encadrement impayé ⇒ production ×0.6 — retour joueur
	# 2026-07-14 (« le mécanisme d'entretien... passé à la trappe ? ») : le moteur le fait,
	# il fallait le DIRE.
	if w.has_method("province_friche") and int(w.province_friche(_pid)) == 1:
		_line("⚠ En friche — entretien impayé (production ×0.6)", "Expense")

	# PEUPLES — frises culture/foi, DÉTAIL AU HOVER (pas de légende toujours affichée)
	var pop := float(info.get("ames", 0))
	var groups: Array = w.province_groups(_pid) if w.has_method("province_groups") else []
	if groups.size() > 0 and pop > 0.0:
		var cmap := {}
		var fmap := {}
		for g in groups:
			var wgt := pop * float(g.get("percent", 0)) / 100.0
			var cn := String(g.get("culture", "?"))
			cmap[cn] = float(cmap.get(cn, 0.0)) + wgt
			var fn := String(g.get("faith", ""))
			if fn == "":
				fn = "Sans foi"
			fmap[fn] = float(fmap.get(fn, 0.0)) + wgt
		_section("PEUPLES  ·  survol → détail")
		PopBar.build_bar_only(_body, cmap, pop)
		PopBar.build_bar_only(_body, fmap, pop)

	# CLASSES FUSIONNÉES : chaque classe → sa colonne d'activité (avec [−][+])
	var cls: Dictionary = w.province_classes(_pid) if w.has_method("province_classes") else {}
	var csat: Dictionary = w.province_class_sat(_pid) if w.has_method("province_class_sat") else {}
	var slaves := int(w.province_slave_count(_pid)) if w.has_method("province_slave_count") else 0

	# JOURNALIERS → la terre : répartition par raw
	_class_row("Journaliers", int(cls.get("laboureurs", 0)), int(csat.get("laboureurs", -1)))
	_alloc_section(w, mine, 0)

	# BOURGEOIS → les manufactures (poser + niveau)
	_class_row("Bourgeois", int(cls.get("artisans", 0)), int(csat.get("artisans", -1)))
	_manuf_section(w, mine)

	# ÉLITES → les édifices (poser + palier)
	_class_row("Élites", int(cls.get("noblesse", 0)), int(csat.get("noblesse", -1)))
	_edifice_section(w, mine)

	# ESCLAVES → la terre aussi : répartition par raw (idem journaliers), si présents
	if slaves > 0:
		_class_row("Esclaves", slaves, int(csat.get("esclaves", -1)))
		_alloc_section(w, mine, 0)

# ── SECTIONS PAR CLASSE (interactives) ────────────────────────────────────────
## RÉPARTITION par raw (kind 0) : une ligne par gisement · part % · [−][+] · ↻ Auto.
func _alloc_section(w, mine: bool, kind: int) -> void:
	var sinks: Array = _alloc.get("sinks", [])
	var any := false
	for i in range(sinks.size()):
		var s: Dictionary = sinks[i]
		if int(s.get("kind", -1)) != kind:
			continue
		any = true
		_alloc_row(w, mine, s, i)
	if not any:
		_line("  aucune extraction ici", "RowDim")
		return
	if mine:
		var wrap := HBoxContainer.new()
		wrap.alignment = BoxContainer.ALIGNMENT_END
		_body.add_child(wrap)
		var auto := _sq_btn("↻ Auto", 52)
		auto.tooltip_text = "rendre la répartition au moteur"
		auto.pressed.connect(func():
			if _pid >= 0:
				w.player_alloc_auto(_pid)
				_fire("↻ répartition automatique"))
		wrap.add_child(auto)

func _alloc_row(w, mine: bool, s: Dictionary, idx: int) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 6)
	_body.add_child(hb)
	var nm := Label.new()
	nm.theme_type_variation = "RowDim"
	nm.text = "   " + String(s.get("name", "?"))
	nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	nm.clip_text = true
	hb.add_child(nm)
	var pc := Label.new()
	pc.theme_type_variation = "RowLabel"
	pc.text = "%d%%" % int(s.get("pct", 0))
	pc.custom_minimum_size = Vector2(40, 0)
	pc.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	hb.add_child(pc)
	if mine:
		var minus := _sq_btn("−")
		minus.pressed.connect(func(): _alloc_apply(idx, int(s.get("weight", 0)) - ALLOC_STEP))
		hb.add_child(minus)
		var plus := _sq_btn("+")
		plus.pressed.connect(func(): _alloc_apply(idx, int(s.get("weight", 0)) + ALLOC_STEP))
		hb.add_child(plus)

## applique une édition d'allocation : pousse l'allocation COMPLÈTE (régler un seul puits
## mettrait les autres à 0) — région à soi, revalidée au drain. (motif province_detail.)
func _alloc_apply(idx: int, new_w: int) -> void:
	var w = Sim.world
	if w == null or _pid < 0:
		return
	var sinks: Array = _alloc.get("sinks", [])
	for i in range(sinks.size()):
		var s: Dictionary = sinks[i]
		var ww := (new_w if i == idx else int(s.get("weight", 0)))
		ww = clampi(ww, 0, 100)
		if int(s.get("kind", 0)) == 0:
			w.player_alloc_raw(_pid, int(s.get("id", 0)), ww)
		else:
			w.player_alloc_bld(_pid, int(s.get("id", 0)), ww)
	_fire("répartition ajustée")

## MANUFACTURES (ligne Bourgeois) : une STRIP d'icônes-chips DU BÂTI SEUL (icône +
## [−][+] EN LIGNE, nom+détail au HOVER) — poser un NOUVEAU chantier se fait dans le
## MENU CONSTRUCTION (bouton « Construire… », retour joueur : plus de chips fantômes ici).
func _manuf_section(w, mine: bool) -> void:
	var blds: Array = w.province_buildings(_pid) if w.has_method("province_buildings") else []
	if blds.is_empty():
		_line("  aucune manufacture", "RowDim")
	else:
		var flow := _flow()
		for b in blds:
			var nom := String(b.get("nom", ""))
			flow.add_child(_manuf_chip(w, mine, nom, int(b.get("niveau", 0)),
				int(b.get("ouvriers", 0)), int(_name2bld.get(nom, -1))))
	if mine:
		_construct_btn(1)

## ÉDIFICES (ligne Élites) : idem — strip d'icônes-chips DU BÂTI SEUL ([−] démolir ·
## [+] palier suivant) ; poser un nouvel édifice = le MENU CONSTRUCTION.
func _edifice_section(w, mine: bool) -> void:
	var edis: Array = w.province_edifices(_pid) if w.has_method("province_edifices") else []
	if edis.is_empty():
		_line("  aucun édifice", "RowDim")
	else:
		var flow := _flow()
		for e in edis:
			flow.add_child(_edi_chip(w, mine, String(e.get("nom", "")), int(e.get("type", -1))))
	if mine:
		_construct_btn(0)

## le bouton « Construire… » (ouvre le MENU CONSTRUCTION sur l'onglet Édifices/
## Manufactures, la province courante visée — signal câblé côté main.gd).
func _construct_btn(kind: int) -> void:
	var wrap := HBoxContainer.new()
	_body.add_child(wrap)
	var b := _sq_btn("⚒ Construire…", 108)
	b.pressed.connect(func(): build_requested.emit(kind))
	wrap.add_child(b)

# ── LES CHIPS (icône + [−][+] en ligne ; le NOM en hover seul) ────────────────
## cadre d'un chip : PanelContainer + HBox serrée. `built` = ton plein (bâti) vs ghost (à bâtir).
func _chip_frame(tip: String, built: bool) -> Array:
	var pc := PanelContainer.new()
	var bg := ParchTheme.HEADER_BG if built else ParchTheme.PANEL_BG
	var bd := ParchTheme.BORDER if built else ParchTheme.DIVIDER
	pc.add_theme_stylebox_override("panel", ParchTheme.sb(bg, bd, 1, 4, 3, 3, 2, 2))
	pc.tooltip_text = tip
	pc.mouse_filter = Control.MOUSE_FILTER_PASS
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 1)
	pc.add_child(hb)
	return [pc, hb]

## un chip de manufacture bâtie : [icône][niv][−][+] — nom + détail au HOVER, chantier 3 :
## « Nom — niveau N · X ouvriers · produit +Y/mois » (le bien réel, matché par manuf_recipe(bld).out).
func _manuf_chip(w, mine: bool, nom: String, niv: int, ouv: int, bid: int) -> Control:
	var tip := "%s — niveau %d · %d ouvriers" % [nom, niv, ouv]
	if bid >= 0 and w.has_method("manuf_recipe"):
		var rec: Dictionary = w.manuf_recipe(bid)
		var out_nom := String(rec.get("out", ""))
		if out_nom != "" and _income.has(out_nom):
			tip += " · produit +%s %s/mois" % [_grp(int(round(float(_income[out_nom]) * 30.0))), out_nom]
	if bid >= 0 and w.has_method("manuf_upkeep_month"):
		var upk := int(w.manuf_upkeep_month(_pid, bid))
		if upk > 0:
			tip += " · entretien ~%d or/mois" % upk
	var fr := _chip_frame(tip, true)
	var hb: HBoxContainer = fr[1]
	_icon(hb, UIKit.manuf_sprite(nom), 26)
	var lv := Label.new()
	lv.theme_type_variation = "RowDim"
	lv.text = str(niv)
	lv.add_theme_font_size_override("font_size", 12)
	lv.custom_minimum_size = Vector2(14, 0)
	lv.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hb.add_child(lv)
	if mine and bid >= 0:
		var minus := _chip_btn("−")
		minus.tooltip_text = "baisser le niveau (démolir un cran)"
		minus.pressed.connect(func(): w.player_manuf_level(_pid, bid, -1); _fire("%s : niveau ↓" % nom))
		hb.add_child(minus)
		var plus := _chip_btn("+")
		plus.tooltip_text = "monter le niveau (payant)"
		plus.pressed.connect(func(): w.player_manuf_level(_pid, bid, 1); _fire("%s : niveau ↑" % nom))
		hb.add_child(plus)
	return fr[0]

## un chip d'édifice bâti : [icône][−][+palier?] — nom au HOVER.
func _edi_chip(w, mine: bool, nom: String, type: int) -> Control:
	var fr := _chip_frame(nom, true)
	var hb: HBoxContainer = fr[1]
	_icon(hb, UIKit.building_sprite(type), 26)
	if mine and type >= 0:
		var minus := _chip_btn("−")
		minus.tooltip_text = "démolir d'un cran"
		minus.pressed.connect(func(): w.player_demolish_edifice(_pid, type); _fire("%s : démoli d'un cran" % nom))
		hb.add_child(minus)
		var succ := int(w.edifice_succ(type)) if w.has_method("edifice_succ") else -1
		if succ >= 0 and w.has_method("build_legal") and bool(w.build_legal(_pid, succ).get("legal", false)):
			var plus := _chip_btn("+")
			plus.tooltip_text = "monter au palier suivant"
			plus.pressed.connect(func(): w.player_build(succ, _pid); _fire("%s : palier suivant" % nom))
			hb.add_child(plus)
	return fr[0]

## une strip qui enveloppe (les chips passent à la ligne suivante quand la largeur manque).
func _flow() -> HFlowContainer:
	var f := HFlowContainer.new()
	f.add_theme_constant_override("h_separation", 4)
	f.add_theme_constant_override("v_separation", 4)
	_body.add_child(f)
	return f

## un mini bouton de chip (les [−][+] collés à l'icône).
func _chip_btn(txt: String) -> Button:
	var b := Button.new()
	b.text = txt
	b.focus_mode = Control.FOCUS_NONE
	b.custom_minimum_size = Vector2(18, 24)
	b.add_theme_font_size_override("font_size", 14)
	b.add_theme_stylebox_override("normal", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.BORDER, 1, 3, 2, 2, 0, 0))
	b.add_theme_stylebox_override("hover", ParchTheme.sb(Color("f0e6c8"), ParchTheme.TAB_UNDERLINE, 1, 3, 2, 2, 0, 0))
	b.add_theme_stylebox_override("pressed", ParchTheme.sb(ParchTheme.DIVIDER, ParchTheme.TAB_UNDERLINE, 1, 3, 2, 2, 0, 0))
	b.add_theme_color_override("font_color", ParchTheme.INK)
	b.add_theme_color_override("font_hover_color", ParchTheme.INK)
	b.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	return b

# ── ONGLET RÉGION : l'OUTPUT économique agrégé (la région = N tuiles) ─────────
## Read-only. La RÉGION est l'unité éco du moteur (les journaliers y sont mis en commun,
## répartis sur les raws des ~3 tuiles) ; cet onglet en montre le RÉSULTAT : production
## brute + manufacturée (sommée sur les provinces de la région) + résumé stab/prospérité.
func _build_region(w, info: Dictionary, _cap: Dictionary) -> void:
	# agréger l'output sur toutes les provinces de la même région
	var raws := {}      # source -> [per_day, res_id]
	var manu := {}      # source -> [per_day, res_id]
	var n := int(w.province_count()) if w.has_method("province_count") else 0
	var nprov := 0
	for p in range(n):
		if not w.has_method("province_region") or int(w.province_region(p)) != _region:
			continue
		nprov += 1
		if not w.has_method("province_income"):
			continue
		for l in w.province_income(p):
			var nm := String(l.get("source", ""))
			if nm == "":
				continue
			var tgt: Dictionary = manu if bool(l.get("manufactured", false)) else raws
			var cur: Array = tgt.get(nm, [0.0, int(l.get("res_id", -1))])
			cur[0] = float(cur[0]) + float(l.get("per_day", 0.0))
			tgt[nm] = cur

	_section("PRODUCTION — RESSOURCES  ·  %d tuile(s)" % nprov)
	_output_list(raws)
	_section("PRODUCTION — MANUFACTURÉS")
	_output_list(manu)

	# RÉSUMÉ : prospérité + stabilité (jauges de la région)
	_section("RÉSUMÉ")
	_gauge("Prospérité", int(info.get("aisance_val", 0)))
	_gauge("Stabilité", int(info.get("humeur_val", 0)))
	var agit := int(info.get("agitation", 0))
	if w.has_method("province_agitation"):
		agit = int(w.province_agitation(_pid).get("value", agit))
	_gauge("Ordre", 100 - clampi(agit, 0, 100))

## la liste d'output triée décroissante : icône + nom + « +X/mois » (per_day × 30).
func _output_list(m: Dictionary) -> void:
	if m.is_empty():
		_line("  aucune production", "RowDim")
		return
	var rows := []
	for nm in m:
		rows.append([nm, float(m[nm][0]), int(m[nm][1])])
	rows.sort_custom(func(a, b): return float(a[1]) > float(b[1]))
	for r in rows:
		var hb := HBoxContainer.new()
		hb.add_theme_constant_override("separation", 6)
		_body.add_child(hb)
		var spr: Texture2D = null
		if int(r[2]) >= 0:
			spr = UIKit.resource_sprite(int(r[2]), String(r[0]))
		if spr == null:
			spr = UIKit.resource_icon(String(r[0]))
		_icon(hb, spr, 18)
		var nm := Label.new()
		nm.theme_type_variation = "RowLabel"
		nm.text = String(r[0])
		nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		nm.clip_text = true
		hb.add_child(nm)
		var amt := Label.new()
		amt.theme_type_variation = "Income"
		amt.text = "+%s/mois" % _grp(int(round(float(r[1]) * 30.0)))
		amt.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		hb.add_child(amt)

## une jauge de résumé : label + barre + % (couleur de score).
func _gauge(label: String, v: int) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	_body.add_child(hb)
	var nm := Label.new()
	nm.theme_type_variation = "RowDim"
	nm.text = label
	nm.custom_minimum_size = Vector2(90, 0)
	hb.add_child(nm)
	var bar := ProgressBar.new()
	bar.min_value = 0.0
	bar.max_value = 100.0
	bar.value = float(clampi(v, 0, 100))
	bar.show_percentage = false
	bar.custom_minimum_size = Vector2(0, 12)
	bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bar.add_theme_stylebox_override("background",
		ParchTheme.sb(Color("caa768"), ParchTheme.BORDER, 1, 2, 0, 0, 0, 0))
	bar.add_theme_stylebox_override("fill",
		ParchTheme.sb(_score_col(v), Color(0, 0, 0, 0), 0, 2, 0, 0, 0, 0))
	hb.add_child(bar)
	var pc := Label.new()
	pc.theme_type_variation = "RowLabel"
	pc.text = "%d%%" % clampi(v, 0, 100)
	pc.custom_minimum_size = Vector2(40, 0)
	pc.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	pc.add_theme_color_override("font_color", _score_col(v))
	hb.add_child(pc)

# ── ONGLET MILITAIRE : défense de la province + menace intérieure ─────────────
## N'INVENTE rien : la façade n'expose ni garnison, ni réserves, ni marins par
## province — on lit ce qui existe (tenue de siège, ouvrage/fort, terrain, agitation).
func _build_militaire(w, info: Dictionary, _cap: Dictionary) -> void:
	# DÉFENSE (tenir la place)
	_section("DÉFENSE")
	var def_pct := int(w.province_defense_pct(_pid)) if w.has_method("province_defense_pct") else 100
	var grid := _grid()
	_kv(grid, "Tenue de siège", "%+d%%" % (def_pct - 100),
		ParchTheme.GREEN if def_pct > 100 else (ParchTheme.RED if def_pct < 100 else ParchTheme.INK))
	var dw := String(info.get("defense", ""))
	_kv(grid, "Ouvrage", dw if (dw != "" and dw != "aucune") else "aucun",
		ParchTheme.INK if (dw != "" and dw != "aucune") else ParchTheme.DIM_INK)
	_kv(grid, "Terrain", ("%s · %s" % [String(info.get("relief", "—")), String(info.get("climat", ""))]).strip_edges(),
		ParchTheme.DIM_INK)

	# MENACE INTÉRIEURE (tenir les gens) — agitation + loyauté + seuil de révolte
	_section("MENACE INTÉRIEURE")
	var agit := int(info.get("agitation", 0))
	if w.has_method("province_agitation"):
		agit = int(w.province_agitation(_pid).get("value", agit))
	var grid2 := _grid()
	_kv(grid2, "Agitation", "%d%%" % agit, ParchTheme.RED if agit >= 50 else ParchTheme.DIM_INK)
	var mood := int(info.get("humeur_val", 0))
	_kv(grid2, "Loyauté", "%d%%" % mood, _score_col(mood))
	if bool(info.get("seuil_revolte", false)):
		_line("⚠ Au bord de la révolte (agitation %d)" % agit, "Expense")

# ── PETITS BOUTONS + retour d'action ──────────────────────────────────────────
## un bouton compact au thème parchemin (les [−][+] collés, les « Bâtir »).
func _sq_btn(txt: String, wide := 24) -> Button:
	var b := Button.new()
	b.text = txt
	b.focus_mode = Control.FOCUS_NONE
	b.custom_minimum_size = Vector2(wide, 20)
	b.add_theme_font_size_override("font_size", 13)
	b.add_theme_stylebox_override("normal", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("hover", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("pressed", ParchTheme.sb(ParchTheme.DIVIDER, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_color_override("font_color", ParchTheme.INK)
	b.add_theme_color_override("font_hover_color", ParchTheme.INK)
	b.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	return b

## une petite icône du pack (cadrée au slot ; rien si absente).
func _icon(into: HBoxContainer, tex: Texture2D, sz := 18) -> void:
	if tex == null:
		return
	var tr := TextureRect.new()
	tr.texture = tex
	tr.custom_minimum_size = Vector2(sz, sz)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	into.add_child(tr)

## un ordre a été émis : flash transitoire + drain live + refresh, effacé après 1,8 s.
func _fire(msg: String) -> void:
	_flash = msg
	if Sim.has_method("notify_action"):
		Sim.notify_action()
	refresh()
	var t := get_tree().create_timer(1.8)
	t.timeout.connect(func():
		_flash = ""
		if visible:
			refresh())

# ── PRIMITIVES DE LAYOUT (conteneurs natifs, aucune ligne dessinée) ────────────
func _section(txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	_body.add_child(l)

func _line(txt: String, variation: String) -> void:
	var l := Label.new()
	l.theme_type_variation = variation
	l.text = txt
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.custom_minimum_size = Vector2(PW - 28.0, 0)
	_body.add_child(l)

func _grid() -> GridContainer:
	var g := GridContainer.new()
	g.columns = 2
	g.add_theme_constant_override("h_separation", 12)
	g.add_theme_constant_override("v_separation", 3)
	_body.add_child(g)
	return g

# ── TERRAIN (chantier 5) : ligne compacte, hover = image du biome + détail ────
## la carte-hover : image + lignes de texte, construite au moment du survol (Godot
## rappelle `_make_custom_tooltip` sur la ligne mère, cf. `_terrain_row` ci-dessous).
class BiomeTip:
	extends PanelContainer
	const ParchTheme = preload("res://ui/parch_theme.gd")
	func setup(tex: Texture2D, lines: PackedStringArray) -> void:
		add_theme_stylebox_override("panel", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.BORDER, 1, 4, 8, 8, 8, 8))
		var vb := VBoxContainer.new()
		vb.add_theme_constant_override("separation", 4)
		add_child(vb)
		if tex != null:
			var tr := TextureRect.new()
			tr.texture = tex
			tr.custom_minimum_size = Vector2(220, 104)
			tr.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
			tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
			vb.add_child(tr)
		for i in range(lines.size()):
			var lb := Label.new()
			lb.text = String(lines[i])
			lb.add_theme_color_override("font_color", ParchTheme.INK if i == 0 else ParchTheme.DIM_INK)
			lb.add_theme_font_size_override("font_size", 13 if i == 0 else 12)
			vb.add_child(lb)

## une petite HBoxContainer qui porte le hover-image (le survol NATIF, TooltipServer).
class TerrainRow:
	extends HBoxContainer
	var _tex: Texture2D
	var _lines := PackedStringArray()
	func setup(tex: Texture2D, lines: PackedStringArray) -> void:
		_tex = tex
		_lines = lines
		tooltip_text = " "   # non-vide : active le hover natif (le contenu vient de _make_custom_tooltip)
		mouse_filter = Control.MOUSE_FILTER_STOP
	func _make_custom_tooltip(_for_text: String) -> Object:
		var tip := BiomeTip.new()
		tip.setup(_tex, _lines)
		return tip

## la ligne TERRAIN : climat/relief + tenue de siège, en un coup d'œil ; le détail
## (image du biome, habitabilité) attend le survol — rien de plus qu'un chiffre en trop.
func _terrain_row(info: Dictionary, def_pct: int) -> void:
	var row := TerrainRow.new()
	row.custom_minimum_size = Vector2(PW - 28.0, 16.0)
	var lb := Label.new()
	lb.theme_type_variation = "RowDim"
	lb.text = "%s · %s · tenue de siège %+d%%" % [
		String(info.get("climat", "")), String(info.get("relief", "")), def_pct - 100]
	lb.mouse_filter = Control.MOUSE_FILTER_IGNORE
	lb.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(lb)
	var hab := int(info.get("habitabilite_pct", -1))
	var lines := PackedStringArray([
		"%s · %s" % [String(info.get("relief", "")), String(info.get("climat", ""))],
		"Tenue de siège : %+d%%" % (def_pct - 100),
	])
	if hab >= 0:
		lines.append("Habitabilité : %d%%" % hab)
	row.setup(UIKit.biome_painting(String(info.get("relief", "")), String(info.get("climat", ""))), lines)
	_body.add_child(row)

## une paire label → valeur dans une grille 2-colonnes (valeur alignée à droite, colorée)
func _kv(grid: GridContainer, label: String, value: String, col: Color) -> void:
	var lab := Label.new()
	lab.theme_type_variation = "RowDim"
	lab.text = label
	lab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid.add_child(lab)
	var val := Label.new()
	val.theme_type_variation = "RowLabel"
	val.text = value
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	val.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	val.add_theme_color_override("font_color", col)
	grid.add_child(val)

## une ligne de classe : nom + effectif — la satisfaction en petit « N% » coloré, à
## droite (retour joueur : « les barres pour désigner un job dans sa classe n'ont
## AUCUN sens » — plus de ProgressBar ici ; le détail (revenu, panier) reste au hover).
func _class_row(name: String, pop: int, sat: int) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	_body.add_child(hb)
	var nm := Label.new()
	nm.theme_type_variation = "RowDim"
	nm.text = name
	nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(nm)
	var pl := Label.new()
	pl.theme_type_variation = "RowLabel"
	pl.text = _grp(pop)
	pl.custom_minimum_size = Vector2(58, 0)
	pl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	hb.add_child(pl)
	var sl := Label.new()
	sl.theme_type_variation = "RowDim"
	sl.text = ("%d%%" % sat) if sat >= 0 else "—"
	sl.custom_minimum_size = Vector2(38, 0)
	sl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	sl.add_theme_color_override("font_color", _score_col(sat) if sat >= 0 else ParchTheme.DIM_INK)
	sl.tooltip_text = "satisfaction (panier de besoins couverts)" if sat >= 0 else "aucun"
	hb.add_child(sl)

## une puce ressource : icône du pack (si dispo) + nom
func _res_chip(into: HBoxContainer, res_id: int, name: String) -> void:
	var chip := HBoxContainer.new()
	chip.add_theme_constant_override("separation", 3)
	into.add_child(chip)
	var spr: Texture2D = null
	if res_id >= 0:
		spr = UIKit.resource_sprite(res_id, name)
	if spr == null:
		spr = UIKit.resource_icon(name)
	if spr != null:
		var tr := TextureRect.new()
		tr.texture = spr
		tr.custom_minimum_size = Vector2(18, 18)
		tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE   # sans ça la TextureRect prend la taille NATIVE du sprite
		tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		chip.add_child(tr)
	var lb := Label.new()
	lb.theme_type_variation = "RowLabel"
	lb.text = name
	chip.add_child(lb)

## un emplacement de bâtiment : icône (repli libellé court) + tooltip
func _bld_slot(grid: GridContainer, tex: Texture2D, tip: String) -> void:
	if tex != null:
		var tr := TextureRect.new()
		tr.texture = tex
		tr.custom_minimum_size = Vector2(28, 28)
		tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE   # cadre les grosses textures du pack au slot 28²
		tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tr.tooltip_text = tip
		grid.add_child(tr)
	else:
		var lb := Label.new()
		lb.theme_type_variation = "RowDim"
		lb.text = tip.substr(0, 3)
		lb.tooltip_text = tip
		lb.custom_minimum_size = Vector2(28, 28)
		grid.add_child(lb)

# ── util : couleur de score 0-100 (rouge bas / vert haut) ─────────────────────
func _score_col(v: int) -> Color:
	if v < 0:
		return ParchTheme.DIM_INK
	if v >= 60:
		return ParchTheme.GREEN
	if v < 40:
		return ParchTheme.RED
	return ParchTheme.INK

# ── util : séparateur de milliers ─────────────────────────────────────────────
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
