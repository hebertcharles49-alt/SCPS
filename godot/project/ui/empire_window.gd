extends PanelContainer
## EmpireWindow — LA fenêtre de gestion d'empire : UNE fenêtre, cinq ONGLETS
## (Économie · Population · Diplomatie · Militaire · Conseil), bâtie avec des
## CONTENEURS Godot NATIFS + le THEME parchemin PARTAGÉ (parch_theme.gd). ZÉRO `_draw` :
## la mise en page s'auto-espace, la hauteur suit le contenu.
##
## Display-only, LECTURE SEULE (sauf les curseurs budgétaires de l'onglet Économie, qui
## enfilent le verbe joueur EXISTANT player_budget_policy) : chaque page lit la MÊME
## membrane que les panneaux d'aujourd'hui (budget_controls, country_relations, corps_ids,
## country_factions, country_council, province_groups…), tout `has_method`-gardé.
## Bascule touche E (câblée dans main.gd). COEXISTE avec budget_panel_v2 / la sidebar.

const ParchTheme  = preload("res://ui/parch_theme.gd")
const EconomyPage = preload("res://ui/economy_page.gd")

const PW := 440.0
## Militaire est CONTEXTUEL (barre de commandement à la sélection d'un corps), pas un
## onglet de gestion — d'où quatre onglets seulement.
const TABS := ["Économie", "Population", "Diplomatie", "Conseil"]

var _tab := 0
var _treasury_lbl: Label = null
var _balance_lbl: Label = null
var _title_lbl: Label = null
var _tab_group: ButtonGroup = null
var _tab_btns: Array = []         # [Button] pour piloter l'onglet actif par code (probe)
var _pages: Array = []            # [Control] une VBox par onglet (visibilité togglée)
var _eco_page: Control = null     # la page Économie (auto-refresh, curseurs)

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

# ── LE SQUELETTE (header + barre d'onglets + corps à pages) ───────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER : nom du royaume à gauche · trésor + solde mensuel empilés à droite
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	head.add_child(hb)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "Empire"
	# PAS de clip_text : sur un Label SHRINK dans un HBox, clip_text force une largeur
	# minimale de 0 → le titre se réduit à rien (les noms de royaume sont courts).
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

	# BARRE D'ONGLETS (soulignement 2px de l'actif via la variation "Tab")
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

	# CORPS (fond transparent — laisse voir le parchemin) : les 5 pages coexistent,
	# une seule visible.
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	bodypanel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(bodypanel)
	var stack := VBoxContainer.new()
	bodypanel.add_child(stack)
	_pages.clear()
	# 0 — Économie : la page réutilisable (curseurs + valeurs vivantes).
	_eco_page = EconomyPage.new()
	stack.add_child(_eco_page)
	_pages.append(_eco_page)
	# 1-4 — pages LUES : VBox rebâtis à chaque refresh.
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

## sélection PUBLIQUE d'un onglet (par code) : met aussi à jour le bouton actif (le
## soulignement suit). Utilisée par la sonde de capture.
func select_tab(idx: int) -> void:
	if idx >= 0 and idx < _tab_btns.size():
		_tab_btns[idx].button_pressed = true
	_select_tab(idx)

# ── API publique ──────────────────────────────────────────────────────────────
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

# ── ONGLET POPULATION : légendes codées par couleur (Culture · Foi · Classe) ──
## Aucun lecteur façade ne donne la composition culture/foi au grain PAYS : on AGRÈGE
## province_groups sur les PROVINCES du joueur (owner == me), pondéré par les âmes de
## chaque province (× le % du groupe). La province est l'unité d'habitation (doctrine
## « la province habite ») ; le centroïde de région tombait parfois hors du territoire.
## Les Classes viennent, elles, du lecteur direct country_demo.
func _build_population(w, me: int) -> void:
	var pg: VBoxContainer = _pages[1]
	for c in pg.get_children():
		c.queue_free()

	var cult := {}      # nom -> âmes (pondérées)
	var faith := {}
	var total := 0.0
	var prov_pops := []   # [pop, nom] pour Top provinces
	if w.has_method("province_count") and w.has_method("province_info"):
		for p in range(int(w.province_count())):
			var info: Dictionary = w.province_info(p)
			if not bool(info.get("valide", false)) or int(info.get("owner", -1)) != me:
				continue
			var pop := float(info.get("ames", 0))
			if pop <= 0.0:
				continue
			total += pop
			prov_pops.append([pop, String(info.get("nom", "province %d" % p))])
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

	# CULTURE
	_pop_section(pg, "CULTURE")
	_legend(pg, cult, total)
	# FOI / RELIGION
	_pop_section(pg, "FOI / RELIGION")
	_legend(pg, faith, total)
	# CLASSE (lecteur direct — pop exactes)
	_pop_section(pg, "CLASSE")
	var clsmap := {}
	var cls_total := 0.0
	if w.has_method("country_demo"):
		var d: Dictionary = w.country_demo(me)
		for cl in d.get("classes", []):
			clsmap[String(cl.get("nom", "?"))] = float(cl.get("pop", 0))
			cls_total += float(cl.get("pop", 0))
	_legend(pg, clsmap, cls_total)

	# TOP PROVINCES (par âmes)
	_pop_section(pg, "TOP PROVINCES")
	prov_pops.sort_custom(func(a, b): return a[0] > b[0])
	if prov_pops.is_empty():
		_dim_line(pg, "aucune province")
	else:
		var shown := 0
		for rp in prov_pops:
			if shown >= 6:
				break
			_kv_row(pg, String(rp[1]), _grp(int(rp[0])), ParchTheme.INK)
			shown += 1

## une légende codée par couleur : swatch + nom + % + effectif, triés décroissants.
func _legend(pg: VBoxContainer, m: Dictionary, total: float) -> void:
	if m.is_empty() or total <= 0.0:
		_dim_line(pg, "—")
		return
	var arr := []
	for k in m:
		arr.append([float(m[k]), String(k)])
	arr.sort_custom(func(a, b): return a[0] > b[0])
	var shown := 0
	for e in arr:
		if shown >= 8:
			break
		var cnt := float(e[0])
		if cnt < 1.0:
			continue
		var name := String(e[1])
		var pct := int(round(100.0 * cnt / total))
		var line := HBoxContainer.new()
		line.add_theme_constant_override("separation", 6)
		pg.add_child(line)
		# swatch de couleur (Panel teinté, clé stable par nom)
		var sw := Panel.new()
		sw.custom_minimum_size = Vector2(12, 12)
		sw.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		sw.add_theme_stylebox_override("panel",
			ParchTheme.sb(_key_color(name), ParchTheme.BORDER, 1, 2, 0, 0, 0, 0))
		line.add_child(sw)
		var nm := Label.new()
		nm.theme_type_variation = "RowLabel"
		nm.text = name
		nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		nm.clip_text = true
		line.add_child(nm)
		var pc := Label.new()
		pc.theme_type_variation = "RowDim"
		pc.text = "%d %% · %s" % [pct, _grp(int(round(cnt)))]
		pc.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		line.add_child(pc)
		shown += 1

# ── ONGLET DIPLOMATIE : relations en lignes (nom + opinion), guerres en tête ──
func _build_diplomatie(w, me: int) -> void:
	var pg: VBoxContainer = _pages[2]
	for c in pg.get_children():
		c.queue_free()
	if not w.has_method("country_relations"):
		_dim_line(pg, "(diplomatie indisponible)")
		return
	var rows: Array = w.country_relations(me)
	# guerres d'abord, puis opinion décroissante
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
		line.add_child(st)
		var op := int(r.get("opinion", 0))
		var val := Label.new()
		val.theme_type_variation = "RowLabel"
		val.text = "%+d" % op
		val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val.custom_minimum_size = Vector2(44, 0)
		val.add_theme_color_override("font_color", _diverge_col(op))
		line.add_child(val)

# ── ONGLET CONSEIL : factions (soutien + tendance) + sièges (titulaire + loyauté) ─
func _build_conseil(w, me: int) -> void:
	var pg: VBoxContainer = _pages[3]
	for c in pg.get_children():
		c.queue_free()
	# FACTIONS
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
	# CONSEIL (sièges)
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

# ── PRIMITIVES DE PAGE ────────────────────────────────────────────────────────
func _pop_section(pg: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	pg.add_child(l)

func _dim_line(pg: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = txt
	pg.add_child(l)

## une ligne label … valeur (colorée)
func _kv_row(pg: VBoxContainer, label: String, value: String, col: Color) -> void:
	var line := HBoxContainer.new()
	pg.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowDim"
	lab.text = label
	lab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lab.clip_text = true
	line.add_child(lab)
	var val := Label.new()
	val.theme_type_variation = "RowLabel"
	val.text = value
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	val.add_theme_color_override("font_color", col)
	line.add_child(val)

# ── util ──────────────────────────────────────────────────────────────────────
## couleur de LÉGENDE stable par nom (clé visuelle, dérivée du hash — display-only ;
## aucune prétention de couleur moteur, juste un codage cohérent d'une session à l'autre).
func _key_color(name: String) -> Color:
	var h := absi(name.hash())
	var hue := float(h % 360) / 360.0
	var sat := 0.42 + float((h / 360) % 100) / 100.0 * 0.28   # 0.42..0.70
	return Color.from_hsv(hue, sat, 0.72)

## couleur de score 0-100 (rouge bas / vert haut)
func _score_col(v: int) -> Color:
	if v >= 60:
		return ParchTheme.GREEN
	if v < 40:
		return ParchTheme.RED
	return ParchTheme.INK

## couleur divergente autour de 0 (opinion ±100)
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
