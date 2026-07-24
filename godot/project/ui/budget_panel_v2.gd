extends PanelContainer
## Trésor : display-only, pilote les curseurs via player_budget_policy. Touche B.

const ParchTheme = preload("res://ui/parch_theme.gd")
const Concepts = preload("res://ui/concepts.gd")

const INCOME  := ParchTheme.INCOME
const EXPENSE := ParchTheme.EXPENSE
const DIVIDER := ParchTheme.DIVIDER

# 6 = Débase absent volontairement : son curseur dédié vit dans l'onglet Monnaie.
const SPEND_HAS_SLIDER := {0: true, 1: true, 2: true, 3: true, 4: true, 5: true}

const TABS := ["Balance", "Monnaie", "Marché", "Commerce"]
const CLASS_NAMES := ["Journaliers", "Bourgeois", "Élite"]

var _built := false
var _treasury_lbl: Label = null
var _balance_lbl: Label = null
const BalanceGraph = preload("res://ui/balance_graph.gd")
var _graph: Control = null        # widget persistant (stateful)
var _borrow_btn: Button = null
var _repay_btn: Button = null     # armé 4 s (anti-course)
var _repay_armed := false
var _repay_armed_ms := -100000
var _reserve_lbl: Label = null
var _left_col: VBoxContainer = null
var _right_col: VBoxContainer = null
var _val_lbls := {}
var _sliders := {}
var _flux_of := {}
var _tab_group: ButtonGroup = null
var _tab := 0
var _tab_btns: Array = []
var _pages: Array = []

var _monnaie_page: VBoxContainer = null
var _monnaie_built := false
var _m_val_lbls := {}
var _m_sliders := {}
var _m_loan_btns := {}
var _m_loan_armed := {}   # armé 4 s (anti-course)
var _m_loan_armed_ms := {}
var _m_bankrupt_btn: Button = null
var _m_bankrupt_armed := false
var _m_bankrupt_armed_ms := -100000

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	# largeur plancher ; hauteur au contenu (aucun min.y forcé)
	custom_minimum_size = Vector2(420, 0)
	position = Vector2(120, 90)
	theme = ParchTheme.build()
	_build_shell()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): refresh())
	if Sim.has_signal("generated"):
		Sim.generated.connect(func(): _built = false; _monnaie_built = false; refresh())
	refresh()

## la confirmation (4 s) retombe même en pause (anti-course, jamais de popup modal).
func _process(_dt: float) -> void:
	if _m_bankrupt_armed and Time.get_ticks_msec() - _m_bankrupt_armed_ms > 4000:
		_m_bankrupt_armed = false
		if visible and _tab == 1:
			_update_monnaie(int(Sim.world.player()) if Sim.world != null and Sim.world.has_method("player") else 0)
	for cls in _m_loan_armed.keys():
		if bool(_m_loan_armed[cls]) and Time.get_ticks_msec() - int(_m_loan_armed_ms.get(cls, 0)) > 4000:
			_m_loan_armed[cls] = false
			if visible and _tab == 1:
				_update_monnaie(int(Sim.world.player()) if Sim.world != null and Sim.world.has_method("player") else 0)

func _select_tab(idx: int) -> void:
	_tab = idx
	for i in range(_pages.size()):
		_pages[i].visible = (i == idx)
	refresh()

## sélection d'un onglet par code (met à jour le bouton actif) — utilisée par la sonde de capture.
func select_tab(idx: int) -> void:
	if idx >= 0 and idx < _tab_btns.size():
		_tab_btns[idx].button_pressed = true
	_select_tab(idx)

func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	var title := Label.new()
	title.theme_type_variation = "Title"
	title.text = "Trésor de l'Empire"
	hb.add_child(title)
	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(spacer)
	var rcol := VBoxContainer.new()
	rcol.add_theme_constant_override("separation", 0)
	hb.add_child(rcol)
	_treasury_lbl = Label.new()
	_treasury_lbl.theme_type_variation = "Treasury"
	_treasury_lbl.text = "—"
	_treasury_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_treasury_lbl.size_flags_horizontal = Control.SIZE_SHRINK_END
	rcol.add_child(_treasury_lbl)
	_balance_lbl = Label.new()
	_balance_lbl.theme_type_variation = "RowDim"
	_balance_lbl.text = "—"
	_balance_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_balance_lbl.size_flags_horizontal = Control.SIZE_SHRINK_END
	rcol.add_child(_balance_lbl)
	_reserve_lbl = Label.new()
	_reserve_lbl.theme_type_variation = "RowDim"
	_reserve_lbl.text = "—"
	_reserve_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_reserve_lbl.size_flags_horizontal = Control.SIZE_SHRINK_END
	rcol.add_child(_reserve_lbl)

	# widget persistant (stateful) : ne pas reconstruire
	_graph = BalanceGraph.new()
	root.add_child(_graph)
	var verbs := HBoxContainer.new()
	verbs.add_theme_constant_override("separation", 8)
	root.add_child(verbs)
	_borrow_btn = Button.new()
	_borrow_btn.focus_mode = Control.FOCUS_NONE
	_borrow_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_borrow_btn.text = "Emprunter"
	_borrow_btn.pressed.connect(func(): _select_tab(1))
	verbs.add_child(_borrow_btn)
	_repay_btn = Button.new()
	_repay_btn.focus_mode = Control.FOCUS_NONE
	_repay_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_repay_btn.text = "Rembourser"
	_repay_btn.pressed.connect(_repay_press)
	verbs.add_child(_repay_btn)

	# « Commerce » reste un onglet vide (routes commerciales, hors périmètre)
	var tabpanel := PanelContainer.new()
	tabpanel.theme_type_variation = "LedTabStrip"
	root.add_child(tabpanel)
	var tabs := HBoxContainer.new()
	tabs.add_theme_constant_override("separation", 2)
	tabpanel.add_child(tabs)
	_tab_group = ButtonGroup.new()
	_tab_btns.clear()
	for i in range(TABS.size()):
		var b := Button.new()
		b.theme_type_variation = "Tab"
		b.toggle_mode = true
		b.button_group = _tab_group
		b.text = TABS[i]
		b.focus_mode = Control.FOCUS_NONE
		if i == _tab:
			b.button_pressed = true
		var idx := i
		b.pressed.connect(func(): _select_tab(idx))
		tabs.add_child(b)
		_tab_btns.append(b)

	# les 4 pages coexistent, une seule visible
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	bodypanel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(bodypanel)
	var stack := VBoxContainer.new()
	bodypanel.add_child(stack)
	_pages.clear()

	# PAGE 0 — Balance : deux colonnes + diviseur
	var page0 := HBoxContainer.new()
	page0.add_theme_constant_override("separation", 12)
	stack.add_child(page0)
	_pages.append(page0)
	_left_col = VBoxContainer.new()
	_left_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	page0.add_child(_left_col)
	var div := Panel.new()
	div.custom_minimum_size = Vector2(1, 0)
	div.add_theme_stylebox_override("panel", ParchTheme.sb(DIVIDER, Color(0, 0, 0, 0), 0, 0, 0, 0, 0, 0))
	page0.add_child(div)
	_right_col = VBoxContainer.new()
	_right_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	page0.add_child(_right_col)

	# PAGE 1 — Monnaie : bâtie une fois (curseurs persistants), valeurs rafraîchies en place
	_monnaie_page = VBoxContainer.new()
	_monnaie_page.add_theme_constant_override("separation", 4)
	_monnaie_page.visible = false
	stack.add_child(_monnaie_page)
	_pages.append(_monnaie_page)

	# PAGE 2 — Marché : reconstruite à chaque refresh (aucun curseur à préserver)
	var page2 := VBoxContainer.new()
	page2.add_theme_constant_override("separation", 4)
	page2.visible = false
	stack.add_child(page2)
	_pages.append(page2)

	# PAGE 3 — Commerce : vide (hors périmètre)
	var page3 := VBoxContainer.new()
	page3.visible = false
	stack.add_child(page3)
	_pages.append(page3)

func _section(col: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	var def := Concepts.def_of_label(txt)
	if def != "":
		l.tooltip_text = def
		l.mouse_filter = Control.MOUSE_FILTER_STOP
	col.add_child(l)

## une ligne : label … valeur (+ curseur optionnel dans la même rangée).
func _row(col: VBoxContainer, label: String, key: String, value_variation: String,
		slider_family := -1, slider_index := -1) -> void:
	var line := HBoxContainer.new()
	line.add_theme_constant_override("separation", 8)
	col.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowLabel"
	lab.text = label
	var def := Concepts.def_of_label(label)
	if def != "":
		lab.tooltip_text = def
		lab.mouse_filter = Control.MOUSE_FILTER_STOP
	line.add_child(lab)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	line.add_child(sp)
	var val := Label.new()
	val.theme_type_variation = value_variation
	val.text = "—"
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	val.custom_minimum_size = Vector2(64, 0)   # largeur fixe : le curseur qui suit ne "danse" pas
	line.add_child(val)
	_val_lbls[key] = val
	if slider_family >= 0:
		var s := HSlider.new()
		s.min_value = 2.0
		s.max_value = 100.0
		s.step = 1.0
		# fiscal (famille 0) : départ doux 20 % ; le 1er refresh resynchronise sur le moteur
		s.value = 20.0 if slider_family == 0 else 100.0
		s.custom_minimum_size = Vector2(96, 14)
		s.size_flags_horizontal = Control.SIZE_SHRINK_END
		s.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		var fam := slider_family
		var idx := slider_index
		s.value_changed.connect(func(v): _apply_slider(fam, idx, v))
		line.add_child(s)
		_sliders["%d:%d" % [slider_family, slider_index]] = s

func _flux_row(col: VBoxContainer, label: String, flux_name: String, value_variation: String) -> void:
	var key := "flux:%s" % flux_name
	_row(col, label, key, value_variation)
	_flux_of[key] = flux_name

func _apply_slider(family: int, index: int, v: float) -> void:
	var w = Sim.world
	if w == null or not w.has_method("player_budget_policy"):
		return
	var mult := clampf(v / 100.0, 0.02, 1.0)
	w.player_budget_policy(family, index, mult)
	if Sim.has_method("notify_action"):
		Sim.notify_action()

## Rembourser : armé 4 s (anti-course) ; -1 = tout le surplus, le moteur borne (jamais de découvert).
func _repay_press() -> void:
	var w = Sim.world
	if w == null:
		return
	if not _repay_armed:
		_repay_armed = true
		_repay_armed_ms = Time.get_ticks_msec()
		refresh()
		return
	_repay_armed = false
	if w.has_method("player_repay"):
		w.player_repay(-1)
		if Sim.has_method("notify_action"):
			Sim.notify_action()
	refresh()

func refresh() -> void:
	var w = Sim.world
	if w == null:
		return
	var me: int = int(w.player()) if w.has_method("player") else 0
	_update_header(w, me)
	match _tab:
		1:
			if not _monnaie_built:
				_build_monnaie(me)
				_monnaie_built = true
			_update_monnaie(me)
		2:
			_build_marche(me)
		3:
			pass   # Commerce : page vide, hors périmètre
		_:
			if not _built:
				_build_body(me)
				_built = true
			_update_values(me)

func _update_header(w, me: int) -> void:
	if w.has_method("country_reserve"):
		var res: Dictionary = w.country_reserve(me)
		_reserve_lbl.text = "Réserve : %s or · %s cuivre" % [_grp(int(round(float(res.get("gold", 0.0))))), _grp(int(round(float(res.get("copper", 0.0)))))]
	if w.has_method("budget_summary"):
		var b: Dictionary = w.budget_summary(me)
		_treasury_lbl.text = "%s or" % _grp(int(b.get("gold", 0)))
		var net := float(b.get("monthly_net", 0.0))
		var pos := net >= 0.0
		_balance_lbl.text = "%s%s or/mois" % ["+" if pos else "−", _grp(int(round(absf(net))))]
		_balance_lbl.add_theme_color_override("font_color", INCOME if pos else EXPENSE)
		# un point par mois, dédupliqué par étiquette (pas de doublon à l'ouverture/action)
		if _graph != null and w.has_method("year") and w.has_method("day_of_year"):
			var mo := 1 + int(w.day_of_year()) / 30
			_graph.push("an %d · m%d" % [int(w.year()), mini(mo, 12)], net)
		if _borrow_btn != null:
			_borrow_btn.text = "Emprunter — jusqu'à %s or" % _grp(int(round(float(b.get("credit_line", 0.0)))))
		if _repay_btn != null and w.has_method("country_debt"):
			var deb_r: Dictionary = w.country_debt(me)
			var owed := float(deb_r.get("total", 0.0))
			if _repay_armed and Time.get_ticks_msec() - _repay_armed_ms > 4000:
				_repay_armed = false
			_repay_btn.text = "Confirmer le remboursement ?" if _repay_armed else ("Rembourser %s or" % _grp(int(round(owed))))
			_repay_btn.disabled = owed < 0.5 and not _repay_armed

## construit les lignes une fois ; ensuite on ne rafraîchit que les valeurs (ne pas perdre l'état de glisse).
func _build_body(me: int) -> void:
	_val_lbls.clear()
	_sliders.clear()
	_flux_of.clear()
	for c in _left_col.get_children():
		c.queue_free()
	for c in _right_col.get_children():
		c.queue_free()

	var ctl := {}
	if Sim.world.has_method("budget_controls"):
		ctl = Sim.world.budget_controls(me)

	_section(_left_col, "RENTRÉES")
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls := int(row.get("id", 0))
		_row(_left_col, String(row.get("name", "Impôt")), "tax:%d" % cls, "Income", 0, cls)
	_flux_row(_left_col, "Export", "export", "Income")
	_flux_row(_left_col, "Péages", "péages+", "Income")

	_section(_right_col, "SORTIES")
	for raw2 in ctl.get("spending", []):
		var row2: Dictionary = raw2
		var idx := int(row2.get("id", 0))
		var has_slider: bool = bool(SPEND_HAS_SLIDER.get(idx, false))
		_row(_right_col, String(row2.get("name", "Dépense")), "sp:%d" % idx, "Expense",
			1 if has_slider else -1, idx if has_slider else -1)
	_flux_row(_right_col, "Conseil", "conseil", "Expense")
	_flux_row(_right_col, "Cour", "cour", "Expense")

func _update_values(me: int) -> void:
	var w = Sim.world
	# flux annuel → mensuel
	var doy := 1
	if w.has_method("day_of_year"):
		doy = maxi(1, int(w.day_of_year()))
	var mf := 30.0 / float(doy)
	var ctl: Dictionary = w.budget_controls(me) if w.has_method("budget_controls") else {}
	var fo_sat := {}
	if w.has_method("country_fiscal_orders"):
		var fo_b: Array = w.country_fiscal_orders(me)
		for cls_b in range(mini(3, fo_b.size())):
			fo_sat[cls_b] = int(fo_b[cls_b].get("satisfaction", -1))
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls := int(row.get("id", 0))
		var lbl: Label = _val_lbls.get("tax:%d" % cls, null)
		if lbl != null:
			if w.has_method("tax_class_month"):
				var sat_i := int(fo_sat.get(cls, -1))
				var base_txt := "%s or/mois" % _grp(int(round(float(w.tax_class_month(cls)))))
				# ▲ marge (≥60), ▼ fragile (<40)
				var mood := ""
				if sat_i >= 60: mood = " ▲"
				elif sat_i >= 0 and sat_i < 40: mood = " ▼"
				lbl.text = ("%s · %d %%%s" % [base_txt, sat_i, mood]) if sat_i >= 0 else base_txt
			else:
				# repli si le lecteur par-classe manque (DLL antérieure) : le taux visé
				lbl.text = "taux %d %%" % int(round(float(row.get("mult", 1.0)) * 100.0))
		var sl: HSlider = _sliders.get("0:%d" % cls, null)
		if sl != null and not sl.has_focus():
			sl.set_value_no_signal(clampf(float(row.get("mult", 1.0)) * 100.0, 2.0, 100.0))
	var flux := {}
	if w.has_method("country_budget"):
		for p in w.country_budget(me):
			flux[String(p.get("name", ""))] = float(p.get("amount", 0.0)) * mf
	for k in _flux_of:
		var lbl2: Label = _val_lbls.get(k, null)
		if lbl2 != null:
			var fname: String = _flux_of[k]
			lbl2.text = "%s or/mois" % _grp(int(round(absf(float(flux.get(fname, 0.0))))))
	var spend_flux := ["invest.", "entretien", "soldes", "marine", "routes"]
	for raw2 in ctl.get("spending", []):
		var row2: Dictionary = raw2
		var idx := int(row2.get("id", 0))
		var lbl3: Label = _val_lbls.get("sp:%d" % idx, null)
		if lbl3 != null:
			if idx == 5 and w.has_method("country_mint_month"):
				# Frappe : lecteur dédié, miroir exact du point fixe moteur (revenu, pas dépense)
				lbl3.text = "+%s or/mois" % _grp(int(round(float(w.country_mint_month(me)))))
			else:
				var amt := 0.0
				if idx >= 0 and idx < spend_flux.size():
					amt = absf(float(flux.get(spend_flux[idx], 0.0)))
				lbl3.text = "%s or/mois" % _grp(int(round(amt)))
		var sl2: HSlider = _sliders.get("1:%d" % idx, null)
		if sl2 != null and not sl2.has_focus():
			sl2.set_value_no_signal(clampf(float(row2.get("mult", 1.0)) * 100.0, 2.0, 100.0))

## comme _row mais stocke dans _m_val_lbls/_m_sliders : dicts SÉPARÉS de la page Balance
## (mêmes family/index existent ailleurs — partager le dict casserait l'onglet non visible).
func _m_row(parent: VBoxContainer, label: String, key: String, value_variation: String,
		slider_family := -1, slider_index := -1) -> void:
	var line := HBoxContainer.new()
	parent.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowLabel"
	lab.text = label
	var def := Concepts.def_of_label(label)
	if def != "":
		lab.tooltip_text = def
		lab.mouse_filter = Control.MOUSE_FILTER_STOP
	line.add_child(lab)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	line.add_child(sp)
	var val := Label.new()
	val.theme_type_variation = value_variation
	val.text = "—"
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	line.add_child(val)
	_m_val_lbls[key] = val
	if slider_family >= 0:
		var s := HSlider.new()
		s.min_value = 2.0
		s.max_value = 100.0
		s.step = 1.0
		# fiscal (famille 0) : départ doux 20 % ; le 1er refresh resynchronise sur le moteur
		s.value = 20.0 if slider_family == 0 else 100.0
		s.custom_minimum_size = Vector2(140, 14)
		s.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
		var fam := slider_family
		var idx := slider_index
		s.value_changed.connect(func(v): _apply_slider(fam, idx, v))
		parent.add_child(s)
		_m_sliders["%d:%d" % [slider_family, slider_index]] = s

## bâtit l'onglet une fois (curseurs persistants, ne pas perdre l'état de glisse) ; _update_monnaie rafraîchit.
func _build_monnaie(me: int) -> void:
	_m_val_lbls.clear()
	_m_sliders.clear()
	_m_loan_btns.clear()
	for c in _monnaie_page.get_children():
		c.queue_free()

	_section(_monnaie_page, "RÉSERVE MÉTALLIQUE")
	_m_row(_monnaie_page, "Réserve", "reserve", "Income")
	_m_row(_monnaie_page, "Apparié", "reserve_paired", "Income")
	_m_row(_monnaie_page, "Célibataire", "reserve_single", "RowLabel")

	_section(_monnaie_page, "FRAPPE")
	_m_row(_monnaie_page, "Frappe", "mint_flow", "Income")
	_m_row(_monnaie_page, "Métal fondu", "mint_metal", "RowLabel")
	_m_row(_monnaie_page, "Part de la réserve frappée", "mint_slider", "RowLabel", 1, 5)

	_section(_monnaie_page, "DÉBASE")
	_m_row(_monnaie_page, "Débase", "debase_state", "Expense")
	_m_row(_monnaie_page, "Billon fondu", "debase_metal", "Expense")
	_m_row(_monnaie_page, "Prix", "debase_prices", "RowLabel")
	_m_row(_monnaie_page, "Sur-frappe au-delà de la parité", "debase_slider", "RowLabel", 1, 6)

	_section(_monnaie_page, "DETTE")
	_m_row(_monnaie_page, "Dette totale", "debt_total", "Expense")
	_m_row(_monnaie_page, "Aux ordres du royaume", "debt_class", "Expense")
	_m_row(_monnaie_page, "Au créancier", "debt_cs", "Expense")
	_m_row(_monnaie_page, "Revenu fiscal annuel", "debt_revenue", "Income")
	_m_row(_monnaie_page, "Dette / revenu", "debt_leverage", "RowLabel")
	_m_row(_monnaie_page, "Crédit disponible maintenant", "debt_available", "RowLabel")
	_m_row(_monnaie_page, "Exposition / marge du créancier", "debt_exposure", "RowLabel")
	_m_row(_monnaie_page, "Taux proposé (coût fixe)", "debt_rate", "RowLabel")
	_m_row(_monnaie_page, "Échéance — réglée 1×/an", "debt_due", "Expense")

	_section(_monnaie_page, "EMPRUNTER À UN ORDRE")
	for cls in range(3):
		var b := Button.new()
		b.focus_mode = Control.FOCUS_NONE
		b.text = "…"
		var c2 := cls
		b.pressed.connect(func(): _m_loan_press(c2))
		_monnaie_page.add_child(b)
		_m_loan_btns[cls] = b

	_section(_monnaie_page, "BANQUEROUTE VOLONTAIRE")
	var bw := Label.new()
	bw.theme_type_variation = "RowDim"
	bw.autowrap_mode = TextServer.AUTOWRAP_WORD
	bw.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bw.custom_minimum_size = Vector2(0, 32)
	bw.text = "Répudie TOUTE la dette : tes créanciers saisiront une part de ta production pendant ~10 ans (cicatrice −75 %, decroissante) ; ton armée vacille."
	_monnaie_page.add_child(bw)
	_m_bankrupt_btn = Button.new()
	_m_bankrupt_btn.focus_mode = Control.FOCUS_NONE
	# stylebox danger explicite : ParchTheme n'a pas de "Button" de base (sinon graphite) ;
	# couleurs reprises du ruban Pause (topbar.gd)
	var bsb := ParchTheme.sb(Color(0.38, 0.08, 0.07, 0.94), Color(0.78, 0.62, 0.30), 1, 3, 10, 10, 6, 6)
	var bsb_hover := ParchTheme.sb(Color(0.48, 0.11, 0.09, 0.96), Color(0.78, 0.62, 0.30), 1, 3, 10, 10, 6, 6)
	var bsb_pressed := ParchTheme.sb(Color(0.30, 0.06, 0.05, 0.96), Color(0.78, 0.62, 0.30), 2, 3, 10, 10, 6, 6)
	_m_bankrupt_btn.add_theme_stylebox_override("normal", bsb)
	_m_bankrupt_btn.add_theme_stylebox_override("hover", bsb_hover)
	_m_bankrupt_btn.add_theme_stylebox_override("pressed", bsb_pressed)
	_m_bankrupt_btn.add_theme_stylebox_override("focus", StyleBoxEmpty.new())
	var bcream := Color(0.94, 0.88, 0.74)
	_m_bankrupt_btn.add_theme_color_override("font_color", bcream)
	_m_bankrupt_btn.add_theme_color_override("font_hover_color", bcream)
	_m_bankrupt_btn.add_theme_color_override("font_pressed_color", bcream)
	_m_bankrupt_btn.pressed.connect(_m_bankrupt_press)
	_monnaie_page.add_child(_m_bankrupt_btn)

	_section(_monnaie_page, "FISCALITÉ PAR ORDRE")
	for cls in range(3):
		_m_row(_monnaie_page, CLASS_NAMES[cls], "fiscal:%d" % cls, "RowLabel", 0, cls)

func _m_loan_press(cls: int) -> void:
	var w = Sim.world
	if w == null:
		return
	if not bool(_m_loan_armed.get(cls, false)):
		_m_loan_armed[cls] = true
		_m_loan_armed_ms[cls] = Time.get_ticks_msec()
		_update_monnaie(int(w.player()) if w.has_method("player") else 0)
		return
	_m_loan_armed[cls] = false
	if w.has_method("player_borrow_class"):
		w.player_borrow_class(cls, -1.0)   # <=0 ⇒ le maximum disponible
		if Sim.has_method("notify_action"):
			Sim.notify_action()
	_update_monnaie(int(w.player()) if w.has_method("player") else 0)

func _m_bankrupt_press() -> void:
	var w = Sim.world
	if w == null:
		return
	if not _m_bankrupt_armed:
		_m_bankrupt_armed = true
		_m_bankrupt_armed_ms = Time.get_ticks_msec()
		_update_monnaie(int(w.player()) if w.has_method("player") else 0)
		return
	_m_bankrupt_armed = false
	if w.has_method("player_bankruptcy"):
		w.player_bankruptcy()
		if Sim.has_method("notify_action"):
			Sim.notify_action()
	_update_monnaie(int(w.player()) if w.has_method("player") else 0)

func _update_monnaie(me: int) -> void:
	var w = Sim.world
	if w == null:
		return
	if w.has_method("country_reserve"):
		var res: Dictionary = w.country_reserve(me)
		var rg := float(res.get("gold", 0.0))
		var rc := float(res.get("copper", 0.0))
		_set_m("reserve", "%s or · %s cuivre" % [_grp(int(round(rg))), _grp(int(round(rc)))])
		# pure présentation des deux chiffres déjà exposés (aucune mécanique côté GDScript)
		var paired := minf(rg, rc)
		var single := absf(rg - rc)
		_set_m("reserve_paired", ("%s paires" % _grp(int(round(paired)))) if paired >= 0.5 else "—",
			ParchTheme.INCOME if paired >= 0.5 else ParchTheme.DIM_INK)
		var single_metal := "or" if rg > rc else "cuivre"
		_set_m("reserve_single", ("%s %s" % [_grp(int(round(single))), single_metal]) if single >= 0.5 else "—",
			ParchTheme.EXPENSE if single > paired * 4.0 and single >= 0.5 else ParchTheme.DIM_INK)
	var ctl: Dictionary = w.budget_controls(me) if w.has_method("budget_controls") else {}
	var mint_mult := 1.0
	var debase_mult := 0.0
	for raw in ctl.get("spending", []):
		var row: Dictionary = raw
		var idx := int(row.get("id", -1))
		if idx == 5:
			mint_mult = float(row.get("mult", 0.0))
		elif idx == 6:
			debase_mult = float(row.get("mult", 0.0))
	if w.has_method("country_mint_month"):
		_set_m("mint_flow", "+%s or/mois" % _grp(int(round(float(w.country_mint_month(me))))))
	_set_m("mint_slider", "%d %%" % int(round(mint_mult * 100.0)))
	_sync_slider("1:5", mint_mult * 100.0)
	var md: Dictionary = w.country_mint_detail(me) if w.has_method("country_mint_detail") else {}
	var pair_t := float(md.get("pair", 0.0))
	var bill_g := float(md.get("billon_gold", 0.0))
	var bill_c := float(md.get("billon_copper", 0.0))
	var dbg_or := float(md.get("debase", 0.0))
	_set_m("mint_metal", ("%.1f or · %.1f cuivre t/mois" % [pair_t + bill_g, pair_t + bill_c])
		if pair_t + bill_g + bill_c > 0.05 else "—")
	var debase_active := dbg_or > 0.05
	_set_m("debase_state", ("+%s or/mois" % _grp(int(round(dbg_or)))) if debase_active else "—",
		ParchTheme.EXPENSE if debase_active else ParchTheme.DIM_INK)
	_set_m("debase_metal", ("%.1f or · %.1f cuivre t/mois" % [bill_g, bill_c])
		if bill_g + bill_c > 0.05 else "—",
		ParchTheme.EXPENSE if bill_g + bill_c > 0.05 else ParchTheme.DIM_INK)
	if w.has_method("country_price_level"):
		var pl := float(w.country_price_level(me))
		_set_m("debase_prices", "×%.2f" % pl,
			ParchTheme.EXPENSE if pl > 1.05 else (ParchTheme.INCOME if pl < 0.95 else ParchTheme.DIM_INK))
	_set_m("debase_slider", "%d %%" % int(round(debase_mult * 100.0)))
	_sync_slider("1:6", debase_mult * 100.0)
	if w.has_method("country_debt"):
		var deb: Dictionary = w.country_debt(me)
		var total := float(deb.get("total", 0.0))
		var to_class := float(deb.get("to_class", 0.0))
		var to_cs := float(deb.get("to_cs", 0.0))
		var taux := float(deb.get("taux", 0.0))
		var annual_revenue := float(deb.get("annual_revenue", 0.0))
		var leverage := float(deb.get("leverage", 0.0))
		var available := float(deb.get("available", 0.0))
		var foreign_exposure := float(deb.get("foreign_exposure", 0.0))
		var foreign_room := float(deb.get("foreign_room", 0.0))
		# `due` = échéance réellement prélevée (10 %/an du stock, credit_year_tick) ;
		# `taux` n'est que le taux d'origination d'un nouvel emprunt, pas la dette inscrite
		var due := float(deb.get("due", 0.0))
		var creditor := int(deb.get("creditor", -1))
		var creditor_name := String(deb.get("creditor_name", ""))
		_set_m("debt_total", "%s or" % _grp(int(round(total))), ParchTheme.EXPENSE if total > 0.5 else ParchTheme.DIM_INK)
		_set_m("debt_class", "%s or" % _grp(int(round(to_class))))
		_set_m("debt_cs", ("%s : %s or" % [creditor_name, _grp(int(round(to_cs)))]) if creditor >= 0 and to_cs > 0.5 else "—")
		_set_m("debt_revenue", "%s or/an" % _grp(int(round(annual_revenue))))
		_set_m("debt_leverage", "%.2f année(s) de revenu" % leverage if total > 0.5 else "0.00")
		_set_m("debt_available", "%s or" % _grp(int(round(available))), ParchTheme.INCOME if available > 0.5 else ParchTheme.EXPENSE)
		_set_m("debt_exposure", ("%s / +%s or" % [_grp(int(round(foreign_exposure))), _grp(int(round(foreign_room)))]) if creditor >= 0 else "Aucun créancier étranger")
		_set_m("debt_rate", "%.1f %% forfaitaires" % (taux * 100.0))
		# `due` prélevé 1×/an, pas un flux continu : la cadence est dite en toutes lettres
		# (valeur réelle, jamais le calcul fictif due/12)
		_set_m("debt_due", "~%s or (prélevés 1×/an)" % _grp(int(round(due))) if total > 0.5 else "—")
	if w.has_method("country_loan_capacity"):
		var caps: Array = w.country_loan_capacity(me)
		for cls in range(mini(3, caps.size())):
			var c: Dictionary = caps[cls]
			var montant := float(c.get("montant_max", 0.0))
			var taux2 := float(c.get("taux", 0.0))
			var btn: Button = _m_loan_btns.get(cls, null)
			if btn == null:
				continue
			var armed := bool(_m_loan_armed.get(cls, false))
			if armed:
				btn.text = "Confirmer l'emprunt aux %s ?" % CLASS_NAMES[cls]
			else:
				btn.text = "Emprunter aux %s — max %s or (%.1f %% fixes)" % [CLASS_NAMES[cls], _grp(int(round(montant))), taux2 * 100.0]
			btn.disabled = montant <= 0.5 and not armed
			btn.tooltip_text = "" if montant > 0.5 else "cet ordre n'a rien à prêter maintenant"
	if _m_bankrupt_btn != null:
		_m_bankrupt_btn.text = "Confirmer la banqueroute ?" if _m_bankrupt_armed else "Répudier la dette (banqueroute)"
		# armé, on ÉCLAIRCIT (modulate >1) : le fond est déjà rouge sombre
		_m_bankrupt_btn.modulate = Color(1.35, 1.2, 1.15) if _m_bankrupt_armed else Color(1, 1, 1)
	if w.has_method("country_fiscal_orders"):
		var fo: Array = w.country_fiscal_orders(me)
		for cls in range(mini(3, fo.size())):
			var f: Dictionary = fo[cls]
			var sat := int(f.get("satisfaction", -1))
			var taux3 := float(f.get("taux", 1.0))
			var revenu := float(f.get("revenu_mois", 0.0))
			# satisfaction=-1 = sentinel légitime (cet ordre n'a encore aucune âme dans le
			# pays, pas un bug) : on affiche « — » partout, jamais un nombre à côté d'un tiret
			var sat_txt := ("%d %% sat." % sat) if sat >= 0 else "—"
			var revenu_txt := _grp(int(round(revenu))) if sat >= 0 else "—"
			_set_m("fiscal:%d" % cls, "%s · %s or/mois" % [sat_txt, revenu_txt],
				_score_col(sat) if sat >= 0 else ParchTheme.DIM_INK)
			_sync_slider("0:%d" % cls, taux3 * 100.0)
			var lbl: Label = _m_val_lbls.get("fiscal:%d" % cls, null)
			if lbl != null:
				lbl.mouse_filter = Control.MOUSE_FILTER_STOP   # sans ça, pas de survol (Label ignore la souris par défaut)
				var hint := ""
				if sat < 0:
					hint = "aucun %s dans ce pays pour l'instant" % CLASS_NAMES[cls]
				elif sat >= 60:
					hint = "marge : peut serrer"
				elif sat < 40:
					hint = "fragile : baisser plutôt que casser"
				lbl.tooltip_text = "taux %d %% — %s" % [int(round(taux3 * 100.0)), hint]

func _set_m(key: String, text: String, col: Color = Color(0, 0, 0, 0)) -> void:
	var lbl: Label = _m_val_lbls.get(key, null)
	if lbl == null:
		return
	lbl.text = text
	if col.a > 0.0:
		lbl.add_theme_color_override("font_color", col)

func _sync_slider(key: String, pct: float) -> void:
	var s: HSlider = _m_sliders.get(key, null)
	if s != null and not s.has_focus():
		s.set_value_no_signal(clampf(pct, 2.0, 100.0))

## couleur de score 0-100 (rouge bas / vert haut)
func _score_col(v: int) -> Color:
	if v >= 60:
		return ParchTheme.GREEN
	if v < 40:
		return ParchTheme.RED
	return ParchTheme.INK

## reconstruite à chaque refresh (aucun curseur à préserver). La tendance est un suivi
## CLIENT (historique local non sérialisé) : le moteur n'expose pas de delta, on observe.
var _marche_hist := {}

func _build_marche(me: int) -> void:
	var pg: VBoxContainer = _pages[2]
	for c in pg.get_children():
		c.queue_free()
	var w = Sim.world
	if not w.has_method("country_stocks"):
		var d := Label.new()
		d.theme_type_variation = "RowDim"
		d.text = "(marché indisponible)"
		pg.add_child(d)
		return
	_section(pg, "PRIX COURANTS")
	var abs_day := int(w.year()) * 365 + (int(w.day_of_year()) if w.has_method("day_of_year") else 0)
	var rows: Array = w.country_stocks(me)
	rows.sort_custom(func(a, b): return float(a.get("price", 0.0)) > float(b.get("price", 0.0)))
	var shown := 0
	for raw in rows:
		var st: Dictionary = raw
		var price := float(st.get("price", 0.0))
		if price <= 0.0:
			continue
		var rid := int(st.get("res_id", -1))
		var nm := String(st.get("name", "?"))
		var line := HBoxContainer.new()
		line.add_theme_constant_override("separation", 6)
		pg.add_child(line)
		var lab := Label.new()
		lab.theme_type_variation = "RowLabel"
		lab.text = nm
		lab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		lab.clip_text = true
		line.add_child(lab)
		var band := Label.new()
		band.theme_type_variation = "RowDim"
		band.text = String(st.get("marche", ""))
		band.custom_minimum_size = Vector2(80, 0)
		line.add_child(band)
		# delta depuis le dernier échantillon observé, affiché /mois (jamais l'annuel)
		var dtxt := ""
		var dcol := ParchTheme.DIM_INK
		if rid >= 0 and _marche_hist.has(rid):
			var h: Dictionary = _marche_hist[rid]
			var prev := float(h.get("prev", price))
			var pday := int(h.get("day", abs_day))
			if abs_day > pday and prev > 0.0:
				var pct := (price - prev) / prev * 100.0 / float(abs_day - pday) * 30.0
				if absf(pct) >= 0.05:
					dtxt = "%s%.1f %%/mois" % ["↗ +" if pct > 0.0 else "↘ ", pct]
					dcol = ParchTheme.INCOME if pct > 0.0 else ParchTheme.EXPENSE
				else:
					dtxt = "→ stable"
		var tr := Label.new()
		tr.theme_type_variation = "RowDim"
		tr.text = dtxt
		tr.custom_minimum_size = Vector2(90, 0)
		tr.add_theme_color_override("font_color", dcol)
		line.add_child(tr)
		var val := Label.new()
		val.theme_type_variation = "Income"
		val.text = "%.2f" % price
		val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val.custom_minimum_size = Vector2(56, 0)
		line.add_child(val)
		if rid >= 0:
			_marche_hist[rid] = {"prev": price, "day": abs_day}
		shown += 1
	if shown == 0:
		_dim_line(pg, "aucun prix observé")

func _dim_line(pg: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = txt
	pg.add_child(l)

func _grp(n: int) -> String:
	var s := str(int(abs(n)))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("−" if n < 0 else "") + out
