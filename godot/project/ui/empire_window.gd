extends PanelContainer
## EmpireWindow — fenêtre de gestion, onglets Économie/Population/Diplomatie/Conseil.
## Display-only ; l'onglet Économie est read-only (le réglage vit au Trésor, touche B). Touche E.

const ParchTheme  = preload("res://ui/parch_theme.gd")
const EconomyPage = preload("res://ui/economy_page.gd")
const PopBar      = preload("res://ui/pop_bar.gd")
const Concepts    = preload("res://ui/concepts.gd")

signal open_budget_requested   ## onglet Économie → ouvre le Trésor (B)
signal open_religion_requested ## onglet Population → ouvre le Créateur de Foi (R)

const PW := 440.0
## pas d'onglet Militaire : contextuel (barre de commandement à la sélection d'un corps)
const TABS := ["Économie", "Population", "Diplomatie", "Conseil"]

var _tab := 0
var _treasury_lbl: Label = null
var _balance_lbl: Label = null
var _title_lbl: Label = null
var _tab_group: ButtonGroup = null
var _tab_btns: Array = []
var _pages: Array = []
var _eco_page: Control = null
var _prov_sort := 2               # 0 ressources · 1 revenu · 2 pop
var _pop_last_total := -1.0       # croissance : pop totale au refresh précédent
var _pop_last_day := -1           # jour absolu correspondant (year()×365+day_of_year())

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, 0)   # largeur plancher ; hauteur AU CONTENU
	position = Vector2(150, 80)
	theme = ParchTheme.build()
	add_to_group("draggable")
	_build_shell()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): if visible: refresh())
	if Sim.has_signal("generated"):
		Sim.generated.connect(func(): if visible: refresh())
	hide()

func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "Empire"
	# pas de clip_text : sur un Label SHRINK, il force une largeur mini de 0 (titre disparaît)
	hb.add_child(_title_lbl)
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

	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	bodypanel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(bodypanel)
	var stack := VBoxContainer.new()
	bodypanel.add_child(stack)
	_pages.clear()
	# 0 — Économie : page réutilisable, lecture seule ici (réglage au Trésor)
	_eco_page = EconomyPage.new()
	_eco_page.interactive = false
	_eco_page.open_budget_requested.connect(func(): open_budget_requested.emit())
	stack.add_child(_eco_page)
	_pages.append(_eco_page)
	# 1-4 — pages lues : rebâties à chaque refresh
	for i in range(1, TABS.size()):
		var pg := VBoxContainer.new()
		pg.add_theme_constant_override("separation", 4)
		pg.visible = false
		stack.add_child(pg)
		_pages.append(pg)

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

func open() -> void:
	visible = true
	for i in range(_pages.size()):
		_pages[i].visible = (i == _tab)
	refresh()

func refresh() -> void:
	var w = Sim.world
	if w == null:
		return
	var me: int = int(w.player()) if w.has_method("player") else 0
	_update_header(w, me)
	match _tab:
		0:
			if _eco_page != null and _eco_page.has_method("refresh"):
				_eco_page.refresh()
		1: _build_population(w, me)
		2: _build_diplomatie(w, me)
		3: _build_conseil(w, me)
	reset_size.call_deferred()

func _update_header(w, me: int) -> void:
	if w.has_method("country_info"):
		var ci: Dictionary = w.country_info(me)
		_title_lbl.text = String(ci.get("nom", "Empire"))
	if w.has_method("budget_summary"):
		var b: Dictionary = w.budget_summary(me)
		_treasury_lbl.text = "%s or" % _grp(int(b.get("gold", 0)))
		var net := float(b.get("monthly_net", 0.0))
		var pos := net >= 0.0
		_balance_lbl.text = "%s%s or/mois" % ["+" if pos else "−", _grp(int(round(absf(net))))]
		_balance_lbl.add_theme_color_override("font_color", ParchTheme.INCOME if pos else ParchTheme.EXPENSE)

## pas de lecteur façade culture/foi au grain PAYS : on agrège province_groups sur les
## provinces du joueur, pondéré par les âmes (le centroïde de région tombait parfois hors du territoire).
func _build_population(w, me: int) -> void:
	var pg: VBoxContainer = _pages[1]
	for c in pg.get_children():
		c.queue_free()

	var cult := {}      # nom -> âmes (pondérées)
	var faith := {}
	var total := 0.0
	var prov_rows := []   # {nom, pop, revenu, res} — provinces triables
	if w.has_method("province_count") and w.has_method("province_info"):
		for p in range(int(w.province_count())):
			var info: Dictionary = w.province_info(p)
			if not bool(info.get("valide", false)) or int(info.get("owner", -1)) != me:
				continue
			var pop := float(info.get("ames", 0))
			if pop <= 0.0:
				continue
			total += pop
			var revenu := float(w.province_tax(p)) if w.has_method("province_tax") else 0.0
			var res := 0.0   # richesse en gisements bruts (flux brut /j)
			if w.has_method("province_income"):
				for l in w.province_income(p):
					if not bool(l.get("manufactured", false)):
						res += float(l.get("per_day", 0.0))
			prov_rows.append({"nom": String(info.get("nom", "province %d" % p)),
				"pop": pop, "revenu": revenu, "res": res})
			if not w.has_method("province_groups"):
				continue
			for g in w.province_groups(p):
				var frac := float(g.get("percent", 0)) / 100.0
				var wgt := pop * frac
				var cn := String(g.get("culture", "?"))
				cult[cn] = float(cult.get(cn, 0.0)) + wgt
				var fn := String(g.get("faith", ""))
				if fn == "":
					fn = "Sans foi"
				faith[fn] = float(faith.get(fn, 0.0)) + wgt

	# delta signé depuis le refresh précédent (display-only), normalisé /mois sur le jour
	# absolu réel (les refresh ne tombent pas pile tous les 30 j)
	_pop_section(pg, "CROISSANCE")
	var abs_day := int(w.year()) * 365 + (int(w.day_of_year()) if w.has_method("day_of_year") else 0)
	if _pop_last_total >= 0.0 and abs_day > _pop_last_day:
		var per_month := (total - _pop_last_total) / float(abs_day - _pop_last_day) * 30.0
		var pos := per_month >= 0.0
		_kv_row(pg, "Croissance",
			"%s%s âmes/mois" % ["+" if pos else "−", _grp(int(round(absf(per_month))))],
			ParchTheme.INCOME if pos else ParchTheme.EXPENSE)
	else:
		_kv_row(pg, "Croissance", "—", ParchTheme.DIM_INK)
	_pop_last_total = total
	_pop_last_day = abs_day

	_pop_section(pg, "CULTURE")
	PopBar.build_group(pg, cult, total)
	_pop_section(pg, "FOI / RELIGION")
	PopBar.build_group(pg, faith, total)
	# le Créateur de Foi devenait injoignable après la 1re fondation (ses portes ne se
	# redéclenchent jamais) : ce lien (+ touche R) le rouvre
	var faith_btn := Button.new()
	var has_faith := int(w.religion_of_country(me)) >= 0 if w.has_method("religion_of_country") else false
	faith_btn.text = ("Foi d'État : %s (R)" % String(w.religion_name(me))) \
		if (has_faith and w.has_method("religion_name")) else "Fonder une religion (R)"
	faith_btn.focus_mode = Control.FOCUS_NONE
	faith_btn.tooltip_text = "Ouvre le Créateur de Foi : crédo, traditions, schisme, recrutement du Lettré."
	faith_btn.pressed.connect(func(): open_religion_requested.emit())
	pg.add_child(faith_btn)
	_pop_section(pg, "CLASSE")
	var clsmap := {}
	var cls_total := 0.0
	if w.has_method("country_demo"):
		var d: Dictionary = w.country_demo(me)
		for cl in d.get("classes", []):
			clsmap[String(cl.get("nom", "?"))] = float(cl.get("pop", 0))
			cls_total += float(cl.get("pop", 0))
	PopBar.build_group(pg, clsmap, cls_total)

	_pop_section(pg, "PROVINCES")
	_prov_sort_bar(pg)
	var keys := ["res", "revenu", "pop"]
	var key: String = keys[clampi(_prov_sort, 0, 2)]
	prov_rows.sort_custom(func(a, b): return float(a[key]) > float(b[key]))
	if prov_rows.is_empty():
		_dim_line(pg, "aucune province")
	else:
		var shown := 0
		for rp in prov_rows:
			if shown >= 8:
				break
			var val := ""
			match _prov_sort:
				0: val = "+%.1f/j" % float(rp["res"])
				1: val = "~%s or/mois" % _grp(int(round(float(rp["revenu"]))))
				_: val = _grp(int(rp["pop"]))
			_kv_row(pg, String(rp["nom"]), val, ParchTheme.INK)
			shown += 1

func _prov_sort_bar(pg: VBoxContainer) -> void:
	var bar := HBoxContainer.new()
	bar.add_theme_constant_override("separation", 4)
	pg.add_child(bar)
	var lab := Label.new()
	lab.theme_type_variation = "RowDim"
	lab.text = "Trier par"
	bar.add_child(lab)
	var grp := ButtonGroup.new()
	var names := ["Ressources", "Revenu", "Pop"]
	for i in range(names.size()):
		var b := Button.new()
		b.theme_type_variation = "Tab"
		b.toggle_mode = true
		b.button_group = grp
		b.text = names[i]
		b.focus_mode = Control.FOCUS_NONE
		b.add_theme_font_size_override("font_size", 12)
		if i == _prov_sort:
			b.button_pressed = true
		var idx := i
		b.pressed.connect(func():
			_prov_sort = idx
			var pw = Sim.world
			_build_population(pw, int(pw.player()) if pw.has_method("player") else 0))
		bar.add_child(b)

func _build_diplomatie(w, me: int) -> void:
	var pg: VBoxContainer = _pages[2]
	for c in pg.get_children():
		c.queue_free()
	if not w.has_method("country_relations"):
		_dim_line(pg, "(diplomatie indisponible)")
		return
	var rows: Array = w.country_relations(me)
	rows.sort_custom(func(a, b):
		var aw := bool(a.get("at_war", false))
		var bw := bool(b.get("at_war", false))
		if aw != bw:
			return aw
		return int(a.get("opinion", 0)) > int(b.get("opinion", 0)))
	var wars := 0
	for r in rows:
		if bool(r.get("at_war", false)):
			wars += 1
	_pop_section(pg, "RELATIONS  ·  %d pays  ·  %d guerre(s)" % [rows.size(), wars])
	if rows.is_empty():
		_dim_line(pg, "aucun voisin connu")
		return
	for r in rows:
		var line := HBoxContainer.new()
		line.add_theme_constant_override("separation", 6)
		pg.add_child(line)
		var at_war := bool(r.get("at_war", false))
		var nm := Label.new()
		nm.theme_type_variation = "RowLabel"
		nm.text = ("⚔ " if at_war else "") + String(r.get("name", "?"))
		nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		nm.clip_text = true
		if at_war:
			nm.add_theme_color_override("font_color", ParchTheme.EXPENSE)
		line.add_child(nm)
		var st := Label.new()
		st.theme_type_variation = "RowDim"
		st.text = String(r.get("status", ""))
		# « Vassal »/« Suzerain » ne sont pas la clé DEFS « Vassalité » : hover mappé à la main
		if st.text == "Vassal" or st.text == "Suzerain":
			st.tooltip_text = Concepts.def_of("Vassalité")
			st.mouse_filter = Control.MOUSE_FILTER_STOP
		line.add_child(st)
		var op := int(r.get("opinion", 0))
		var val := Label.new()
		val.theme_type_variation = "RowLabel"
		val.text = "%+d" % op
		val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val.custom_minimum_size = Vector2(44, 0)
		val.add_theme_color_override("font_color", _diverge_col(op))
		val.tooltip_text = Concepts.def_of("Opinion")
		val.mouse_filter = Control.MOUSE_FILTER_STOP
		line.add_child(val)

func _build_conseil(w, me: int) -> void:
	var pg: VBoxContainer = _pages[3]
	for c in pg.get_children():
		c.queue_free()
	var fx := {}
	if w.has_method("country_factions"):
		fx = w.country_factions(me)
	_pop_section(pg, "FACTIONS  ·  tension de coup %d%%" % int(fx.get("coup", 0)))
	var flist: Array = fx.get("list", [])
	if flist.is_empty():
		_dim_line(pg, "aucune faction")
	else:
		for raw in flist:
			var fe: Dictionary = raw
			var line := HBoxContainer.new()
			line.add_theme_constant_override("separation", 6)
			pg.add_child(line)
			var nm := Label.new()
			nm.theme_type_variation = "RowLabel"
			nm.text = String(fe.get("name", "Faction")) + (" ★" if bool(fe.get("dominant", false)) else "")
			nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			nm.clip_text = true
			line.add_child(nm)
			var sup := Label.new()
			sup.theme_type_variation = "RowLabel"
			sup.text = "%d %%" % int(fe.get("part", 0))
			sup.custom_minimum_size = Vector2(46, 0)
			sup.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
			line.add_child(sup)
			var delta := int(fe.get("policy_delta", 0))
			var tr := Label.new()
			tr.theme_type_variation = "RowDim"
			tr.text = "%+d" % delta
			tr.custom_minimum_size = Vector2(40, 0)
			tr.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
			tr.add_theme_color_override("font_color",
				ParchTheme.INCOME if delta > 0 else (ParchTheme.EXPENSE if delta < 0 else ParchTheme.DIM_INK))
			line.add_child(tr)
	_pop_section(pg, "CONSEIL")
	if not w.has_method("country_council"):
		_dim_line(pg, "(conseil indisponible)")
		return
	var seats: Array = w.country_council(me)
	if seats.is_empty():
		_dim_line(pg, "aucun siège")
		return
	for seat in seats:
		var line2 := HBoxContainer.new()
		line2.add_theme_constant_override("separation", 6)
		pg.add_child(line2)
		var post := Label.new()
		post.theme_type_variation = "RowDim"
		post.text = String(seat.get("seat", "Siège"))
		post.custom_minimum_size = Vector2(120, 0)
		var pdef := Concepts.def_of_label(post.text)
		if pdef != "":
			post.tooltip_text = pdef
			post.mouse_filter = Control.MOUSE_FILTER_STOP
		line2.add_child(post)
		var filled := bool(seat.get("filled", false))
		var who := Label.new()
		who.theme_type_variation = "RowLabel"
		if filled:
			var fname := String(seat.get("firstname", ""))
			var house := String(seat.get("house", ""))
			var pname := (fname + " " + house).strip_edges()
			who.text = pname if pname != "" else String(seat.get("councilor", "—"))
		else:
			who.text = "(vacant)"
			who.add_theme_color_override("font_color", ParchTheme.DIM_INK)
		who.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		who.clip_text = true
		line2.add_child(who)
		if filled:
			var loy := int(seat.get("loyalty", 0))
			var ll := Label.new()
			ll.theme_type_variation = "RowLabel"
			ll.text = "%d%%" % loy
			ll.custom_minimum_size = Vector2(36, 0)
			ll.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
			ll.add_theme_color_override("font_color", _score_col(loy))
			line2.add_child(ll)

func _pop_section(pg: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	var def := Concepts.def_of_label(txt)
	if def != "":
		l.tooltip_text = def
		l.mouse_filter = Control.MOUSE_FILTER_STOP
	pg.add_child(l)

func _dim_line(pg: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = txt
	pg.add_child(l)

func _kv_row(pg: VBoxContainer, label: String, value: String, col: Color) -> void:
	var line := HBoxContainer.new()
	pg.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowDim"
	lab.text = label
	lab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lab.clip_text = true
	var def := Concepts.def_of_label(label)
	if def != "":
		lab.tooltip_text = def
		lab.mouse_filter = Control.MOUSE_FILTER_STOP
	line.add_child(lab)
	var val := Label.new()
	val.theme_type_variation = "RowLabel"
	val.text = value
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	val.add_theme_color_override("font_color", col)
	line.add_child(val)

func _score_col(v: int) -> Color:
	if v >= 60:
		return ParchTheme.GREEN
	if v < 40:
		return ParchTheme.RED
	return ParchTheme.INK

func _diverge_col(v: int) -> Color:
	if v > 5:
		return ParchTheme.GREEN
	if v < -5:
		return ParchTheme.RED
	return ParchTheme.DIM_INK

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
