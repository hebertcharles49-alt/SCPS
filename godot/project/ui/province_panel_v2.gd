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

const PW := 356.0   ## largeur plafond (~360, brief)

var _pid := -1
var _tab := 0                       ## 0 Infrastructure · 1 Militaire · 2 Démographie
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
	var names := ["Infrastructure", "Militaire", "Démographie"]
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
	_update_header(w, info, cap)
	for c in _body.get_children():
		c.queue_free()
	match _tab:
		1: _build_militaire(w, info, cap)
		2: _build_demographie(w, info, cap)
		_: _build_infrastructure(w, info, cap)
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

# ── ONGLET INFRASTRUCTURE (le contenu plein) ──────────────────────────────────
func _build_infrastructure(w, info: Dictionary, cap: Dictionary) -> void:
	# TERRAIN
	_section("TERRAIN")
	var def_pct := int(w.province_defense_pct(_pid)) if w.has_method("province_defense_pct") else 100
	_line("%s · %s · tenue de siège %+d%%" % [
		String(info.get("climat", "")), String(info.get("relief", "")), def_pct - 100], "RowDim")

	# STATISTIQUES clés (label → valeur, couleur sémantique)
	_section("PROVINCE")
	var grid := _grid()
	_kv(grid, "Population", _grp(info.get("ames", 0)), ParchTheme.INK)
	var tax := float(w.province_tax(_pid)) if w.has_method("province_tax") else 0.0
	_kv(grid, "Impôts", "~%s or/an" % _grp(int(round(tax))), ParchTheme.INK)
	var prod_tot := 0.0
	var inc: Array = w.province_income(_pid) if w.has_method("province_income") else []
	for l in inc:
		prod_tot += float(l.get("per_day", 0.0))
	_kv(grid, "Production", "+%.1f/j" % prod_tot, ParchTheme.GREEN)
	var aisance := int(info.get("aisance_val", 0))
	_kv(grid, "Prospérité", "%d%%" % aisance, _score_col(aisance))
	var mood := int(info.get("humeur_val", 0))
	_kv(grid, "Loyauté", "%d%%" % mood, _score_col(mood))
	var agit := int(info.get("agitation", 0))
	if w.has_method("province_agitation"):
		agit = int(w.province_agitation(_pid).get("value", agit))
	_kv(grid, "Agitation", "%d%%" % agit, ParchTheme.RED if agit >= 50 else ParchTheme.DIM_INK)
	var dwork := String(info.get("defense", ""))
	_kv(grid, "Ouvrage", dwork if dwork != "" else "—", ParchTheme.DIM_INK)
	if bool(info.get("seuil_revolte", false)):
		_line("⚠ Au bord de la révolte (agitation %d)" % agit, "Expense")

	# CLASSES (pop + petite barre de satisfaction)
	_section("CLASSES")
	var cls: Dictionary = w.province_classes(_pid) if w.has_method("province_classes") else {}
	var csat: Dictionary = w.province_class_sat(_pid) if w.has_method("province_class_sat") else {}
	var slaves := int(w.province_slave_count(_pid)) if w.has_method("province_slave_count") else 0
	for row in [["Laboureurs", "laboureurs"], ["Artisans", "artisans"],
			["Noblesse", "noblesse"], ["Esclaves", "esclaves"]]:
		var pop := (slaves if row[1] == "esclaves" else int(cls.get(row[1], 0)))
		var sv := int(csat.get(row[1], -1))
		_class_row(String(row[0]), pop, sv)

	# RESSOURCES (icône + nom des gisements)
	_section("RESSOURCES")
	var res_line := HBoxContainer.new()
	res_line.add_theme_constant_override("separation", 10)
	_body.add_child(res_line)
	var shown := 0
	for l in inc:
		if bool(l.get("manufactured", false)):
			continue
		if shown >= 3:
			break
		_res_chip(res_line, int(l.get("res_id", -1)), String(l.get("source", "")))
		shown += 1
	if shown == 0:
		var rnom := String(info.get("ressource", "—"))
		_res_chip(res_line, -1, rnom)

	# PRODUCTION (flux réalisés /j)
	_section("PRODUCTION")
	if inc.size() == 0:
		_line("rien de notable", "RowDim")
	else:
		for l in inc:
			var pl := HBoxContainer.new()
			pl.add_theme_constant_override("separation", 8)
			_body.add_child(pl)
			var amt := Label.new()
			amt.theme_type_variation = "Income"
			amt.text = "+%.1f/j" % float(l.get("per_day", 0.0))
			amt.custom_minimum_size = Vector2(56, 0)
			pl.add_child(amt)
			_res_chip(pl, int(l.get("res_id", -1)), String(l.get("source", "")))

	# BÂTIMENTS (grille d'emplacements — icônes du pack, repli libellé)
	var blds: Array = w.province_buildings(_pid) if w.has_method("province_buildings") else []
	var edis: Array = w.province_edifices(_pid) if w.has_method("province_edifices") else []
	if blds.size() > 0 or edis.size() > 0:
		_section("BÂTIMENTS")
		var bg := GridContainer.new()
		bg.columns = 8
		bg.add_theme_constant_override("h_separation", 4)
		bg.add_theme_constant_override("v_separation", 4)
		_body.add_child(bg)
		for e in edis:
			_bld_slot(bg, UIKit.building_sprite(int(e.get("type", -1))), String(e.get("nom", "")))
		for b in blds:
			var tip := "%s — niveau %d · %d ouvriers" % [
				String(b.get("nom", "")), int(b.get("niveau", 0)), int(b.get("ouvriers", 0))]
			_bld_slot(bg, UIKit.manuf_sprite(String(b.get("nom", ""))), tip)

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

# ── ONGLET DÉMOGRAPHIE : classes (pop + satisfaction) + frises culture/foi ────
## Réutilise la MÊME barre de proportions (pop_bar.gd) que l'onglet Population de
## l'empire — DRY : culture & foi rendus en frises segmentées.
func _build_demographie(w, info: Dictionary, _cap: Dictionary) -> void:
	# PEUPLE (résumé)
	_section("PEUPLE")
	var grid := _grid()
	var pop := float(info.get("ames", 0))
	_kv(grid, "Population", _grp(int(pop)), ParchTheme.INK)
	_kv(grid, "Héritage", String(info.get("heritage", "—")), ParchTheme.INK)

	# CLASSES (pop + barre de satisfaction — même rangée que l'infrastructure)
	_section("CLASSES")
	var cls: Dictionary = w.province_classes(_pid) if w.has_method("province_classes") else {}
	var csat: Dictionary = w.province_class_sat(_pid) if w.has_method("province_class_sat") else {}
	var slaves := int(w.province_slave_count(_pid)) if w.has_method("province_slave_count") else 0
	for row in [["Laboureurs", "laboureurs"], ["Artisans", "artisans"],
			["Noblesse", "noblesse"], ["Esclaves", "esclaves"]]:
		var cpop := (slaves if row[1] == "esclaves" else int(cls.get(row[1], 0)))
		var sv := int(csat.get(row[1], -1))
		_class_row(String(row[0]), cpop, sv)

	# CULTURE / FOI — frises de proportions (âmes = pop × part du groupe)
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
		_section("CULTURE")
		PopBar.build_group(_body, cmap, pop)
		_section("FOI / RELIGION")
		PopBar.build_group(_body, fmap, pop)

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

## une ligne de classe : nom · pop · barre de satisfaction (ProgressBar natif)
func _class_row(name: String, pop: int, sat: int) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	_body.add_child(hb)
	var nm := Label.new()
	nm.theme_type_variation = "RowDim"
	nm.text = name
	nm.custom_minimum_size = Vector2(78, 0)
	hb.add_child(nm)
	var pl := Label.new()
	pl.theme_type_variation = "RowLabel"
	pl.text = _grp(pop)
	pl.custom_minimum_size = Vector2(58, 0)
	pl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	hb.add_child(pl)
	var bar := ProgressBar.new()
	bar.min_value = 0.0
	bar.max_value = 100.0
	bar.value = float(maxi(sat, 0))
	bar.show_percentage = false
	bar.custom_minimum_size = Vector2(0, 12)
	bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bar.add_theme_stylebox_override("background",
		ParchTheme.sb(Color("caa768"), ParchTheme.BORDER, 1, 2, 0, 0, 0, 0))
	var fill_col := _score_col(sat) if sat >= 0 else ParchTheme.DIM_INK
	bar.add_theme_stylebox_override("fill",
		ParchTheme.sb(fill_col, Color(0, 0, 0, 0), 0, 2, 0, 0, 0, 0))
	if sat < 0:
		bar.value = 0.0
		bar.tooltip_text = "aucun"
	hb.add_child(bar)

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
