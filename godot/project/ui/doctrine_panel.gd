extends PanelContainer
## DoctrinePanel — UI-DOCTRINE P2 (docs/DESIGN_MISSIONS_DOCTRINES.md §4/§5) : le menu
## des 6 slots de Doctrines, bâti avec des CONTENEURS Godot NATIFS (PanelContainer/
## VBox/HBox/Grid/ScrollContainer) + le THEME parchemin PARTAGÉ (parch_theme.gd), MÊME
## squelette que construction_panel/budget_panel_v2 (HeaderStrip + corps déroulant).
## ZÉRO `_draw`. Ouvert par la cellule Influence de la topbar (topbar.gd
## `doctrine_requested`), câblé dans main/main.gd comme budget_panel_v2 (touche B).
##
## TROIS VUES empilées dans le même corps (motif « page-stack », une seule visible) :
##   SLOTS (l'accueil, une grille 6 cases) → CATALOGUE (17 doctrines, choix pour UN
##   slot) → DÉTAIL (une doctrine : bandeau + colonne des 6 idées). Le bouton ← du
##   header revient toujours à SLOTS (≤ 3 clics, doctrine CLAUDE.md).
##
## MOTEUR PAS ENCORE ATTERRI (2026-09-02) : SEUL `influence_info` existe côté binding
## à l'écriture de ce fichier (stock/gain_month/hover — pas encore upkeep_month/
## net_month/hover_depenses) ; AUCUN `doctrine_*` n'existe encore (ni readers ni
## verbes). Chaque appel est donc `has_method`-gardé (règle #2 _ARCHITECTURE.md) :
## méthode absente ⇒ état « chantier » propre (une ligne, pas de crash) — jamais un
## champ vide qui prétend une donnée réelle. Les clés de dictionnaire manquantes
## retombent sur `.get(clé, défaut)` partout (le contrat peut s'étoffer sans casser
## ce fichier, cf. `influence_info` déjà incomplet).

const ParchTheme = preload("res://ui/parch_theme.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

const PW := 460.0

enum View { SLOTS, CATALOGUE, DETAIL }

var _view: int = View.SLOTS
var _cat_slot := -1     ## le slot VIDE ciblé quand on ouvre le catalogue (pour l'adoption)
var _detail_id := -1    ## l'id de doctrine affiché en vue DÉTAIL
var _detail_slot := -1  ## le slot occupé par cette doctrine (pour l'abandon)
var _detail_name := ""

var _abandon_armed := false
var _abandon_armed_ms := -100000

var _title_lbl: Label = null
var _back_btn: Button = null
var _scroll: ScrollContainer = null
var _stack: VBoxContainer = null
var _slots_page: GridContainer = null
var _cat_page: VBoxContainer = null
var _detail_page: VBoxContainer = null
var _fit_gen := 0   ## jeton anti-course (motif construction_panel._fit_scroll)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	custom_minimum_size = Vector2(PW, 0)
	position = Vector2(140, 80)
	theme = ParchTheme.build()
	add_to_group("draggable")
	_build_shell()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): if visible: refresh())
	if Sim.has_signal("generated"):
		Sim.generated.connect(func(): if visible: _show_view(View.SLOTS))
	get_viewport().size_changed.connect(_fit_scroll)
	hide()

## la fenêtre de confirmation « Abandonner » (4 s, motif banqueroute budget_panel_v2)
## retombe même en pause — jamais de popup modale (doctrine UI-4).
func _process(_dt: float) -> void:
	if _abandon_armed and Time.get_ticks_msec() - _abandon_armed_ms > 4000:
		_abandon_armed = false
		if visible and _view == View.DETAIL:
			_build_detail()

## ouverture depuis la topbar (touche/cellule) — motif `empire_window.open()` : toujours
## l'accueil (vue SLOTS), jamais la vue où on l'avait laissé la dernière fois fermée.
func open() -> void:
	visible = true
	_show_view(View.SLOTS)

# ── LE SQUELETTE (header + corps déroulant à 3 pages empilées) ────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER : ← (retour, masqué en vue SLOTS) · titre (dynamique par vue) · ✕
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 6)
	head.add_child(hb)
	_back_btn = Button.new()
	_back_btn.text = "←"
	_back_btn.focus_mode = Control.FOCUS_NONE
	_back_btn.custom_minimum_size = Vector2(26, 26)
	_back_btn.visible = false
	_back_btn.tooltip_text = "Retour aux slots"
	_back_btn.pressed.connect(func(): _show_view(View.SLOTS))
	hb.add_child(_back_btn)
	_title_lbl = Label.new()
	_title_lbl.theme_type_variation = "Title"
	_title_lbl.text = "Doctrines"
	_title_lbl.clip_text = true
	_title_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(_title_lbl)
	hb.add_child(_close_btn())

	# CORPS (fond transparent, laisse voir le parchemin) : déroule sous une hauteur
	# bornée au viewport — motif construction_panel (jamais une fenêtre qui déborde).
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	root.add_child(bodypanel)
	_scroll = ScrollContainer.new()
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	bodypanel.add_child(_scroll)
	_stack = VBoxContainer.new()
	_stack.add_theme_constant_override("separation", 8)
	_stack.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.add_child(_stack)

	# VUE 0 — SLOTS (l'accueil) : grille 3×2.
	_slots_page = GridContainer.new()
	_slots_page.columns = 3
	_slots_page.add_theme_constant_override("h_separation", 10)
	_slots_page.add_theme_constant_override("v_separation", 10)
	_stack.add_child(_slots_page)

	# VUE 1 — CATALOGUE (17 doctrines, choix pour le slot ciblé) : grille aussi.
	_cat_page = VBoxContainer.new()
	_cat_page.add_theme_constant_override("separation", 8)
	_cat_page.visible = false
	_stack.add_child(_cat_page)

	# VUE 2 — DÉTAIL (une doctrine) : bandeau + colonne d'idées + abandon.
	_detail_page = VBoxContainer.new()
	_detail_page.add_theme_constant_override("separation", 8)
	_detail_page.visible = false
	_stack.add_child(_detail_page)

## bouton fermer, au thème parchemin (motif construction_panel._close_btn).
func _close_btn() -> Button:
	var b := Button.new()
	b.text = "✕"
	b.focus_mode = Control.FOCUS_NONE
	b.custom_minimum_size = Vector2(24, 24)
	b.add_theme_font_size_override("font_size", 13)
	b.add_theme_stylebox_override("normal", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("hover", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_stylebox_override("pressed", ParchTheme.sb(ParchTheme.DIVIDER, ParchTheme.TAB_UNDERLINE, 1, 3, 4, 4, 1, 1))
	b.add_theme_color_override("font_color", ParchTheme.INK)
	b.add_theme_color_override("font_hover_color", ParchTheme.INK)
	b.add_theme_color_override("font_pressed_color", ParchTheme.INK)
	b.pressed.connect(func():
		visible = false
		Sound.play("ui_parchment_close"))
	return b

# ── NAVIGATION ENTRE LES 3 VUES ─────────────────────────────────────────────────
func _show_view(v: int) -> void:
	_view = v
	_slots_page.visible = (v == View.SLOTS)
	_cat_page.visible = (v == View.CATALOGUE)
	_detail_page.visible = (v == View.DETAIL)
	_back_btn.visible = (v != View.SLOTS)
	refresh()

func refresh() -> void:
	_update_title()
	match _view:
		View.CATALOGUE:
			_build_catalogue()
		View.DETAIL:
			_build_detail()
		_:
			_build_slots()
	_fit_scroll()

func _update_title() -> void:
	match _view:
		View.CATALOGUE:
			_title_lbl.text = "Choisir une doctrine"
		View.DETAIL:
			_title_lbl.text = _detail_name if _detail_name != "" else "Doctrine"
		_:
			_title_lbl.text = "Doctrines"

# ── VUE SLOTS — grille 6 cases (verrouillé/vide/occupé) ─────────────────────────
## `doctrine_slots(me)` : {slots_total, slots_open, rows:[{slot, state, doctrine, name,
## bg, ideas_owned}]}. `state` ASSUMÉ 0=verrouillé · 1=vide · 2=occupé
## (ordre de la spec P2, aucun enum moteur encore livré à l'écriture — À REVÉRIFIER par
## l'orchestrateur dès l'atterrissage du binding, probe visuelle des 6 cases).
## AUCUN entretien de doctrine (v107) : `suspended` reste dans le contrat moteur
## (toujours faux) mais l'UI ne l'affiche plus — état mort retiré.
func _build_slots() -> void:
	for c in _slots_page.get_children():
		c.queue_free()
	var w = Sim.world
	if w == null or not w.has_method("doctrine_slots"):
		_dim_line(_slots_page, "Doctrines — chantier (binding moteur non atterri)")
		return
	var me := int(w.player()) if w.has_method("player") else 0
	var info: Dictionary = w.doctrine_slots(me)
	var total := int(info.get("slots_total", 6))
	var rows: Array = info.get("rows", [])
	var by_slot := {}
	for raw in rows:
		var r: Dictionary = raw
		by_slot[int(r.get("slot", -1))] = r
	for i in range(total):
		_slots_page.add_child(_slot_card(i, by_slot.get(i, {})))

func _slot_card(slot: int, row: Dictionary) -> Control:
	var state := int(row.get("state", 0))
	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(136, 156)
	card.mouse_filter = Control.MOUSE_FILTER_STOP
	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 4)
	vb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	card.add_child(vb)
	match state:
		2:
			# OCCUPÉ — vignette du fond + nom + pastilles d'idées (N/6) + suspendue.
			card.add_theme_stylebox_override("panel", ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 1, 6, 6, 6, 6, 6))
			card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
			var tex: Texture2D = UIKit.icon2(String(row.get("bg", "")))
			if tex != null:
				var tr := TextureRect.new()
				tr.texture = tex
				tr.custom_minimum_size = Vector2(120, 60)
				tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
				tr.stretch_mode = TextureRect.STRETCH_SCALE
				tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
				vb.add_child(tr)
			var nm := Label.new()
			nm.theme_type_variation = "RowLabel"
			nm.text = String(row.get("name", "Doctrine"))
			nm.clip_text = true
			nm.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			vb.add_child(nm)
			var pips := Label.new()
			pips.theme_type_variation = "RowDim"
			pips.text = "%d/6" % int(row.get("ideas_owned", 0))
			pips.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			vb.add_child(pips)
			var did := int(row.get("doctrine", -1))
			card.gui_input.connect(func(e):
				if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
					_detail_id = did
					_show_view(View.DETAIL))
		1:
			# VIDE — « + », clic → catalogue pour CE slot.
			card.add_theme_stylebox_override("panel", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.BORDER, 1, 6, 8, 8, 8, 8))
			card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
			var plus := Label.new()
			plus.theme_type_variation = "Title"
			plus.text = "+"
			plus.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			plus.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			plus.size_flags_vertical = Control.SIZE_EXPAND_FILL
			vb.add_child(plus)
			var s := slot
			card.gui_input.connect(func(e):
				if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
					_cat_slot = s
					_show_view(View.CATALOGUE))
		_:
			# VERROUILLÉ — assombri, aucun clic.
			card.add_theme_stylebox_override("panel", ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.DIVIDER, 1, 6, 8, 8, 8, 8))
			card.modulate = Color(1, 1, 1, 0.5)
			card.tooltip_text = String(row.get("hover", "s'ouvrira à l'avènement d'un âge engagé"))
			var lock_lbl := Label.new()
			lock_lbl.theme_type_variation = "RowDim"
			lock_lbl.text = "Verrouillé"
			lock_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			lock_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			lock_lbl.size_flags_vertical = Control.SIZE_EXPAND_FILL
			vb.add_child(lock_lbl)
	return card

# ── VUE CATALOGUE — 17 cartes, choix pour `_cat_slot` ───────────────────────────
## `doctrine_catalog(me)` : [{id, name, hover, bg, cost, available, reason}] ×17.
## `reason` = repli LEGACY (une ligne, pas la checklist ScpsGateCond[] — le contrat
## n'expose pas de conds[] pour l'instant, cf. _ARCHITECTURE.md §3bis « repli si le
## moteur n'a pas de conds »).
func _build_catalogue() -> void:
	for c in _cat_page.get_children():
		c.queue_free()
	var w = Sim.world
	if w == null or not w.has_method("doctrine_catalog"):
		_dim_line(_cat_page, "Catalogue — chantier (binding moteur non atterri)")
		return
	var me := int(w.player()) if w.has_method("player") else 0
	var cat: Array = w.doctrine_catalog(me)
	if cat.is_empty():
		_dim_line(_cat_page, "aucune doctrine disponible")
		return
	var grid := GridContainer.new()
	grid.columns = 3
	grid.add_theme_constant_override("h_separation", 10)
	grid.add_theme_constant_override("v_separation", 10)
	_cat_page.add_child(grid)
	for raw in cat:
		grid.add_child(_catalog_card(raw))

func _catalog_card(raw: Dictionary) -> Control:
	var id := int(raw.get("id", -1))
	var nom := String(raw.get("name", "Doctrine"))
	var hover := String(raw.get("hover", ""))
	var cost := int(raw.get("cost", 0))
	var available := bool(raw.get("available", false))
	var reason := String(raw.get("reason", ""))

	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(150, 130)
	card.mouse_filter = Control.MOUSE_FILTER_STOP
	var bg_col := ParchTheme.HEADER_BG if available else ParchTheme.PANEL_BG
	var bd_col := ParchTheme.BORDER if available else ParchTheme.DIVIDER
	card.add_theme_stylebox_override("panel", ParchTheme.sb(bg_col, bd_col, 1, 6, 8, 8, 8, 8))
	var tip := nom
	if hover != "":
		tip += "\n" + hover
	tip += "\nCoût : %d" % cost
	if not available and reason != "":
		tip += "\n✗ " + reason
	card.tooltip_text = tip

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 4)
	card.add_child(vb)
	var tex: Texture2D = UIKit.icon2(String(raw.get("bg", "")))
	if tex != null:
		var tr := TextureRect.new()
		tr.texture = tex
		tr.custom_minimum_size = Vector2(130, 65)
		tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tr.stretch_mode = TextureRect.STRETCH_SCALE
		tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		vb.add_child(tr)
	var nm := Label.new()
	nm.theme_type_variation = "RowLabel"
	nm.text = nom
	nm.clip_text = true
	nm.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	nm.add_theme_color_override("font_color", ParchTheme.INK if available else ParchTheme.DIM_INK)
	vb.add_child(nm)
	var cl := Label.new()
	cl.theme_type_variation = "RowDim"
	cl.text = "%d" % cost
	cl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vb.add_child(cl)
	if available:
		card.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
		card.gui_input.connect(func(e):
			if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
				_adopt(id))
	else:
		var rl := Label.new()
		rl.theme_type_variation = "Expense"
		rl.text = "✗ %s" % (reason if reason != "" else "indisponible")
		rl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		rl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		vb.add_child(rl)
	return card

func _adopt(id: int) -> void:
	var w = Sim.world
	if w != null and w.has_method("doctrine_adopt"):
		w.doctrine_adopt(_cat_slot, id)
		if Sim.has_method("notify_action"):
			Sim.notify_action()
	_show_view(View.SLOTS)

# ── VUE DÉTAIL — bandeau 512×256 + colonne des 6 idées sur le TIERS DROIT ──────
## `doctrine_detail(me, id)` : {name, bg, hover, adopted, slot,
## ideas:[{idx, name, bonus, icon, owned, is_verb, wired, cost, next}]}.
## AUCUN entretien de doctrine (v107) : `suspended`/`upkeep_month` restent dans
## le contrat moteur (toujours faux/0) mais l'UI ne les lit plus.
func _build_detail() -> void:
	for c in _detail_page.get_children():
		c.queue_free()
	var w = Sim.world
	if w == null or not w.has_method("doctrine_detail") or _detail_id < 0:
		_dim_line(_detail_page, "Doctrine — chantier (binding moteur non atterri)")
		return
	var me := int(w.player()) if w.has_method("player") else 0
	var d: Dictionary = w.doctrine_detail(me, _detail_id)
	if d.is_empty():
		_dim_line(_detail_page, "doctrine introuvable")
		return
	_detail_name = String(d.get("name", "Doctrine"))
	_detail_slot = int(d.get("slot", _detail_slot))
	_title_lbl.text = _detail_name

	# LE BANDEAU 512×256 (fond peint calme sur son tiers droit — la colonne d'idées
	# s'y pose). Control brut (pas un Box) : la colonne est ANCRÉE sur le fond, pas
	# empilée dessous.
	var banner := Control.new()
	banner.custom_minimum_size = Vector2(512, 256)
	banner.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_detail_page.add_child(banner)
	var bgtex: Texture2D = UIKit.icon2(String(d.get("bg", "")))
	if bgtex != null:
		var bgr := TextureRect.new()
		bgr.texture = bgtex
		bgr.stretch_mode = TextureRect.STRETCH_SCALE
		bgr.set_anchors_preset(Control.PRESET_FULL_RECT)
		bgr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		banner.add_child(bgr)

	# le NOM vit déjà dans le header du panneau (_title_lbl) — pas de doublon peint
	# sur l'illustration. AUCUN entretien de doctrine (v107, décision joueur
	# 2026-09-02) : la plaque « Entretien : N/mois » + le marqueur « suspendue »
	# ont disparu avec lui (états morts).

	# LA COLONNE DES 6 IDÉES — TIERS DROIT du bandeau (anchors, pas un calcul de pixel).
	var idea_col := VBoxContainer.new()
	idea_col.anchor_left = 2.0 / 3.0
	idea_col.anchor_right = 1.0
	idea_col.anchor_top = 0.0
	idea_col.anchor_bottom = 1.0
	idea_col.offset_top = 4.0
	idea_col.offset_bottom = -4.0
	idea_col.offset_left = 0.0
	idea_col.offset_right = -10.0
	idea_col.alignment = BoxContainer.ALIGNMENT_CENTER
	# 6 × 36 + 5 × 6 = 246 ≤ 248 disponibles (256 − 2×4) : la colonne TIENT dans le
	# bandeau — la 6e icône débordait sous le fond (probe 04_detail).
	idea_col.add_theme_constant_override("separation", 6)
	idea_col.mouse_filter = Control.MOUSE_FILTER_PASS
	banner.add_child(idea_col)
	var ideas: Array = d.get("ideas", [])
	for raw in ideas:
		idea_col.add_child(_idea_icon(raw))

	# ABANDONNER — confirmation simple (armé 4 s, motif banqueroute/rembourser).
	var abtn := Button.new()
	abtn.focus_mode = Control.FOCUS_NONE
	abtn.text = "Confirmer l'abandon ?" if _abandon_armed else "Abandonner"
	abtn.pressed.connect(_abandon_press)
	_detail_page.add_child(abtn)

## une idée : icône seule (owned pleine, next liserée+coût au hover, verrouillée
## assombrie). Hover = nom + la ligne de bonus ; `is_verb` non câblée ajoute
## discrètement « (à venir) ».
func _idea_icon(idea: Dictionary) -> Control:
	var nom := String(idea.get("name", "Idée"))
	var bonus := String(idea.get("bonus", ""))
	var owned := bool(idea.get("owned", false))
	var next := bool(idea.get("next", false))
	var is_verb := bool(idea.get("is_verb", false))
	var wired := bool(idea.get("wired", true))
	var cost := int(idea.get("cost", 0))

	var btn := Button.new()
	btn.custom_minimum_size = Vector2(36, 36)
	btn.focus_mode = Control.FOCUS_NONE
	btn.flat = true
	btn.expand_icon = true
	var tex: Texture2D = UIKit.icon2(String(idea.get("icon", "")))
	if tex != null:
		btn.icon = tex
	else:
		btn.text = nom.left(1)   # repli minimal si l'icône du lot est absente

	var tip := nom
	if bonus != "":
		tip += "\n" + bonus
	if is_verb and not wired:
		tip += "\n(à venir)"
	if next and cost > 0:
		tip += "\nCoût : %d" % cost
	btn.tooltip_text = tip

	if owned:
		btn.disabled = true
		# le thème assombrit l'icône d'un bouton disabled — une idée POSSÉDÉE doit
		# rester pleine (les icônes étaient fantomatiques, probe 04_detail).
		btn.add_theme_color_override("icon_disabled_color", Color(1, 1, 1, 1))
	elif next:
		btn.disabled = false
		btn.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
		btn.add_theme_stylebox_override("normal", ParchTheme.sb(Color(0, 0, 0, 0), ParchTheme.TAB_UNDERLINE, 2, 6, 2, 2, 2, 2))
		btn.pressed.connect(_buy_next_idea)
	else:
		# pas encore atteignable (séquence) — assombrie mais LISIBLE, aucun clic.
		btn.disabled = true
		btn.add_theme_color_override("icon_disabled_color", Color(1, 1, 1, 0.55))
	return btn

func _buy_next_idea() -> void:
	var w = Sim.world
	if w == null or not w.has_method("doctrine_buy_idea") or _detail_id < 0:
		return
	w.doctrine_buy_idea(_detail_id)
	if Sim.has_method("notify_action"):
		Sim.notify_action()
	_build_detail()

func _abandon_press() -> void:
	if not _abandon_armed:
		_abandon_armed = true
		_abandon_armed_ms = Time.get_ticks_msec()
		_build_detail()
		return
	_abandon_armed = false
	var w = Sim.world
	if w != null and w.has_method("doctrine_abandon"):
		w.doctrine_abandon(_detail_slot)
		if Sim.has_method("notify_action"):
			Sim.notify_action()
	_show_view(View.SLOTS)

# ── PRIMITIVES ──────────────────────────────────────────────────────────────────
func _dim_line(pg: Container, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "RowDim"
	l.text = txt
	pg.add_child(l)

## la fenêtre déroulante HUGGE le contenu, bornée au viewport (motif construction_panel).
func _fit_scroll() -> void:
	if _scroll == null or _stack == null:
		return
	_fit_gen += 1
	var gen := _fit_gen
	await get_tree().process_frame
	await get_tree().process_frame
	if gen != _fit_gen or not is_instance_valid(_scroll):
		return
	var vp := get_viewport_rect().size
	var hmax := clampf(vp.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 40.0, 200.0, 760.0)
	var want := clampf(_stack.get_combined_minimum_size().y, 0.0, hmax)
	_scroll.custom_minimum_size = Vector2(0, want)
	reset_size.call_deferred()
