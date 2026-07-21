extends PanelContainer
## BudgetPanelV2 — PILOTE « grand livre parchemin » construit avec des CONTENEURS
## Godot NATIFS (PanelContainer/VBox/HBox/GridContainer/Margin) + un THEME + des
## StyleBoxFlat pour TOUT le style (une seule source). Aucun `_draw` maison : la
## mise en page est auto-espacée par les conteneurs, le style vit dans le Theme.
## COEXISTE avec economy_panel.gd / sidebar_drawer.gd (ne les touche pas).
## Display-only : lit la façade (Sim.world), pilote les curseurs par les verbes
## joueur existants (player_budget_policy). Bascule touche B (câblée dans main.gd).
##
## Le POINT du pilote : sortir de la « brique dessinée à la main ». Concept 3.

const ParchTheme = preload("res://ui/parch_theme.gd")   # THEME parchemin PARTAGÉ (palette + styleboxes)
const Concepts = preload("res://ui/concepts.gd")   # D4 — glossaire hover (registre centralisé)

# palette réutilisée hors du Theme (couleur du solde mensuel + diviseur) — source unique : ParchTheme
const INCOME  := ParchTheme.INCOME
const EXPENSE := ParchTheme.EXPENSE
const DIVIDER := ParchTheme.DIVIDER

# les postes de DÉPENSE pilotables (family 1) portent un curseur ; les autres sont lus.
# 5 = Frappe (MONNAIE M2) : même motif que les 5 enveloppes existantes (curseur générique).
# NB : 6 = Débase existe aussi (family 1) mais N'EST PAS ajouté ici — la page Balance
# (SORTIES) n'a pas de ligne de flux pour elle (spend_flux, _update_values) ; son curseur
# DÉDIÉ vit dans l'onglet MONNAIE (UI-MONNAIE 2026-07-16, _build_monnaie), hors périmètre
# de cette liste historique.
const SPEND_HAS_SLIDER := {0: true, 1: true, 2: true, 3: true, 4: true, 5: true}

const TABS := ["Balance", "Monnaie", "Marché", "Commerce"]
const CLASS_NAMES := ["Journaliers", "Bourgeois", "Élite"]   # SocialClass 0-2 (curseurs fiscaux/emprunt)

var _built := false
var _treasury_lbl: Label = null
var _balance_lbl: Label = null
var _reserve_lbl: Label = null   # MONNAIE M1/M2 — « Réserve : X or · Y cuivre » (lecteur pur)
var _left_col: VBoxContainer = null
var _right_col: VBoxContainer = null
# refs de valeurs vivantes : clé "family:index" -> Label ; sliders idem
var _val_lbls := {}
var _sliders := {}
# lignes LUES : clé de _val_lbls -> nom de poste de flux (country_budget)
var _flux_of := {}
var _tab_group: ButtonGroup = null
var _tab := 0
var _tab_btns: Array = []
var _pages: Array = []

# ── UI-MONNAIE (2026-07-16) — l'onglet MONNAIE (page 1) ───────────────────────────
var _monnaie_page: VBoxContainer = null
var _monnaie_built := false
var _m_val_lbls := {}     # clé -> Label de valeur (rafraîchi en place)
var _m_sliders := {}      # "family:index" -> HSlider (fiscal/frappe/débase)
var _m_loan_btns := {}    # classe (0..2) -> Button « Emprunter… »
var _m_loan_armed := {}   # classe -> bool (confirmation UI-4, 4 s)
var _m_loan_armed_ms := {}
var _m_bankrupt_btn: Button = null
var _m_bankrupt_armed := false
var _m_bankrupt_armed_ms := -100000
var _m_debase_warn: Label = null

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	# hug content : largeur plancher ~420 (le contenu peut pousser), HAUTEUR pilotée par
	# les conteneurs — aucun min.y forcé (fin de l'étendue de parchemin vide en bas).
	custom_minimum_size = Vector2(420, 0)
	position = Vector2(120, 90)
	theme = ParchTheme.build()
	_build_shell()
	# rafraîchissement : cadence mensuelle (les chiffres joueur) + à la (re)génération.
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): refresh())
	if Sim.has_signal("generated"):
		Sim.generated.connect(func(): _built = false; _monnaie_built = false; refresh())
	refresh()

## la fenêtre de confirmation « Banqueroute »/« Emprunter » (4 s) retombe même en pause
## (motif country_actions.gd _war_press — jamais de popup modal, doctrine UI-4).
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

## sélection PUBLIQUE d'un onglet (par code) : met aussi à jour le bouton actif (le
## soulignement suit). Utilisée par la sonde de capture (motif empire_window.gd).
func select_tab(idx: int) -> void:
	if idx >= 0 and idx < _tab_btns.size():
		_tab_btns[idx].button_pressed = true
	_select_tab(idx)

# ── LE SQUELETTE (conteneurs natifs) ─────────────────────────────────────────
func _build_shell() -> void:
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER
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
	# côté droit : trésor + solde mensuel empilés, alignés à droite.
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
	# MONNAIE M1 — la réserve métallique (redevance minière), lecteur pur.
	_reserve_lbl = Label.new()
	_reserve_lbl.theme_type_variation = "RowDim"
	_reserve_lbl.text = "—"
	_reserve_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_reserve_lbl.size_flags_horizontal = Control.SIZE_SHRINK_END
	rcol.add_child(_reserve_lbl)

	# TAB BAR — UI-MONNAIE (2026-07-16) : les 3 onglets étaient posés mais JAMAIS câblés
	# (aucun .pressed.connect — TROUVAILLES.md) ; « Monnaie » rejoint « Marché » désormais
	# WIRÉS (motif page-stack d'empire_window.gd). « Commerce » reste un onglet VIDE
	# (comportement inchangé : cliquer dessus ne faisait déjà rien) — hors périmètre de
	# cette mission (routes commerciales, pas la monnaie).
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

	# CORPS : les 4 pages coexistent, une seule visible (motif empire_window.gd).
	var bodypanel := PanelContainer.new()
	bodypanel.theme_type_variation = "Body"
	bodypanel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(bodypanel)
	var stack := VBoxContainer.new()
	bodypanel.add_child(stack)
	_pages.clear()

	# PAGE 0 — Balance : deux colonnes + diviseur (INCHANGÉ, comportement d'origine).
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

	# PAGE 1 — Monnaie : bâtie UNE FOIS (curseurs persistants, motif page0/EconomyPage),
	# valeurs rafraîchies en place.
	_monnaie_page = VBoxContainer.new()
	_monnaie_page.add_theme_constant_override("separation", 4)
	_monnaie_page.visible = false
	stack.add_child(_monnaie_page)
	_pages.append(_monnaie_page)

	# PAGE 2 — Marché : lue seule (reconstruite à chaque refresh, motif empire_window
	# onglets Population/Diplomatie — aucun curseur à préserver).
	var page2 := VBoxContainer.new()
	page2.add_theme_constant_override("separation", 4)
	page2.visible = false
	stack.add_child(page2)
	_pages.append(page2)

	# PAGE 3 — Commerce : VIDE (hors périmètre, cf. commentaire ci-dessus).
	var page3 := VBoxContainer.new()
	page3.visible = false
	stack.add_child(page3)
	_pages.append(page3)

## une tête de section — D4 : porte la définition du concept si le titre en nomme un
## (« DÉBASE », « FRAPPE »… — casse/pluriel tolérés, cf. Concepts.def_of_label).
func _section(col: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	var def := Concepts.def_of_label(txt)
	if def != "":
		l.tooltip_text = def
		l.mouse_filter = Control.MOUSE_FILTER_STOP
	col.add_child(l)

## une ligne : label … valeur (colorée) [· curseur optionnel, MÊME rangée — UI-POLISH
## #12 : le curseur vivait en SIBLING sous la ligne (une ligne label/valeur, puis une
## ligne curseur juste en dessous, 3px d'écart) — à la densité du panneau, l'œil ne
## sait plus quel curseur appartient à quelle ligne (tassés, rythme uniforme). Il vit
## maintenant DANS la même HBoxContainer, colonne dédiée à droite de la valeur : plus
## d'ambiguïté ligne↔curseur, sans toucher à la structure onglets/colonnes.
## Retourne la clé (pour retrouver le Label de valeur en refresh).
func _row(col: VBoxContainer, label: String, key: String, value_variation: String,
		slider_family := -1, slider_index := -1) -> void:
	var line := HBoxContainer.new()
	line.add_theme_constant_override("separation", 8)
	col.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowLabel"
	lab.text = label
	# D4 — glossaire hover : si le libellé nomme un concept du registre (ex. « Péages »,
	# « Sur-frappe au-delà de la parité »), sa définition vit derrière le survol.
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
	val.custom_minimum_size = Vector2(64, 0)   # colonne de valeur fixe : le curseur qui suit ne "danse" pas
	line.add_child(val)
	_val_lbls[key] = val
	if slider_family >= 0:
		var s := HSlider.new()
		s.min_value = 2.0
		s.max_value = 100.0
		s.step = 1.0
		s.value = 100.0
		# colonne DÉDIÉE à droite de la valeur, SUR la rangée qu'il contrôle.
		s.custom_minimum_size = Vector2(96, 14)
		s.size_flags_horizontal = Control.SIZE_SHRINK_END
		s.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		var fam := slider_family
		var idx := slider_index
		s.value_changed.connect(func(v): _apply_slider(fam, idx, v))
		line.add_child(s)
		_sliders["%d:%d" % [slider_family, slider_index]] = s

## une ligne LUE (or/mois) d'un poste de flux nommé, sans curseur.
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

# ── DONNÉES VIVANTES ──────────────────────────────────────────────────────────
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

## le BANDEAU (trésor + solde + réserve) : commun aux 4 onglets, rafraîchi TOUJOURS
## (motif empire_window._update_header) — extrait de _update_values (UI-MONNAIE).
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

## construit les LIGNES à partir de budget_controls (une fois — puis on ne fait
## que rafraîchir les valeurs, pour ne pas perdre l'état de glisse des curseurs).
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

	# LEFT — RENTRÉES : impôt par classe (curseur family 0, or/mois via tax_class_month)
	#   + postes LUS Export / Péages (lignes de flux positives).
	_section(_left_col, "RENTRÉES")
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls := int(row.get("id", 0))
		_row(_left_col, String(row.get("name", "Impôt")), "tax:%d" % cls, "Income", 0, cls)
	_flux_row(_left_col, "Export", "export", "Income")
	_flux_row(_left_col, "Péages", "péages+", "Income")

	# RIGHT — SORTIES : enveloppes pilotables (curseur family 1, 5 postes dont routes)
	#   + postes LUS Conseil / Cour (lignes de flux négatives).
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
	# facteur mois (flux annuel → mensuel)
	var doy := 1
	if w.has_method("day_of_year"):
		doy = maxi(1, int(w.day_of_year()))
	var mf := 30.0 / float(doy)
	# rentrées : impôt par classe (or/mois)
	var ctl: Dictionary = w.budget_controls(me) if w.has_method("budget_controls") else {}
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls := int(row.get("id", 0))
		var lbl: Label = _val_lbls.get("tax:%d" % cls, null)
		if lbl != null:
			if w.has_method("tax_class_month"):
				lbl.text = "%s or/mois" % _grp(int(round(float(w.tax_class_month(cls)))))
			else:
				# repli honnête si le lecteur par-classe n'existe pas (DLL antérieure) :
				# on montre le taux visé (mult), la seule donnée disponible.
				lbl.text = "taux %d %%" % int(round(float(row.get("mult", 1.0)) * 100.0))
		var sl: HSlider = _sliders.get("0:%d" % cls, null)
		if sl != null and not sl.has_focus():
			sl.set_value_no_signal(clampf(float(row.get("mult", 1.0)) * 100.0, 2.0, 100.0))
	# flux nommés (rentrées lues + sorties), ramenés au mois comme le reste de l'onglet
	var flux := {}
	if w.has_method("country_budget"):
		for p in w.country_budget(me):
			flux[String(p.get("name", ""))] = float(p.get("amount", 0.0)) * mf
	# postes LUS (Export / Péages / Conseil / Cour) : |montant| en or/mois
	for k in _flux_of:
		var lbl2: Label = _val_lbls.get(k, null)
		if lbl2 != null:
			var fname: String = _flux_of[k]
			lbl2.text = "%s or/mois" % _grp(int(round(absf(float(flux.get(fname, 0.0))))))
	# sorties : enveloppe réalisée (or/mois) + curseur
	var spend_flux := ["invest.", "entretien", "soldes", "marine", "routes"]
	for raw2 in ctl.get("spending", []):
		var row2: Dictionary = raw2
		var idx := int(row2.get("id", 0))
		var lbl3: Label = _val_lbls.get("sp:%d" % idx, null)
		if lbl3 != null:
			if idx == 5 and w.has_method("country_mint_month"):
				# MONNAIE M2 — LA FRAPPE : lecteur DÉDIÉ, miroir exact du point fixe moteur
				# (pas un poste de FLUX générique — c'est un revenu, pas une dépense).
				lbl3.text = "+%s or/mois" % _grp(int(round(float(w.country_mint_month(me)))))
			else:
				var amt := 0.0
				if idx >= 0 and idx < spend_flux.size():
					amt = absf(float(flux.get(spend_flux[idx], 0.0)))
				lbl3.text = "%s or/mois" % _grp(int(round(amt)))
		var sl2: HSlider = _sliders.get("1:%d" % idx, null)
		if sl2 != null and not sl2.has_focus():
			sl2.set_value_no_signal(clampf(float(row2.get("mult", 1.0)) * 100.0, 2.0, 100.0))

# ══════════════════════════════════════════════════════════════════════════════
# ── UI-MONNAIE (2026-07-16) — ONGLET MONNAIE : réserve/frappe/débase/dette/emprunt/
# banqueroute/fiscalité, l'arc M0→M12 rendu visible. Chaque verbe/lecteur moteur
# EXISTAIT déjà (M1→M9) — seule cette façade manquait (cf. TROUVAILLES.md).
# ══════════════════════════════════════════════════════════════════════════════

## une ligne : label … valeur (colorée), + curseur optionnel — MÊME moule que `_row`
## mais stocke dans _m_val_lbls/_m_sliders (dicts SÉPARÉS de la page Balance : le
## curseur family=1 index=5/6, family=0 index=0..2 existe déjà sur d'autres pages —
## partager le dict casserait le rafraîchissement de l'onglet non visible).
func _m_row(parent: VBoxContainer, label: String, key: String, value_variation: String,
		slider_family := -1, slider_index := -1) -> void:
	var line := HBoxContainer.new()
	parent.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowLabel"
	lab.text = label
	# D4 — même motif que _row() : le libellé porte la définition s'il nomme un concept.
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
		s.value = 100.0
		s.custom_minimum_size = Vector2(140, 14)
		s.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
		var fam := slider_family
		var idx := slider_index
		s.value_changed.connect(func(v): _apply_slider(fam, idx, v))
		parent.add_child(s)
		_m_sliders["%d:%d" % [slider_family, slider_index]] = s

## bâtit l'onglet UNE FOIS (curseurs persistants — ne pas perdre l'état de glisse) ;
## _update_monnaie rafraîchit les valeurs à chaque tick/changement d'onglet.
func _build_monnaie(me: int) -> void:
	_m_val_lbls.clear()
	_m_sliders.clear()
	_m_loan_btns.clear()
	for c in _monnaie_page.get_children():
		c.queue_free()

	_section(_monnaie_page, "RÉSERVE MÉTALLIQUE")
	_m_row(_monnaie_page, "Réserve", "reserve", "Income")

	_section(_monnaie_page, "FRAPPE")
	_m_row(_monnaie_page, "Frappe", "mint_flow", "Income")
	_m_row(_monnaie_page, "Part de la réserve frappée", "mint_slider", "RowLabel", 1, 5)

	_section(_monnaie_page, "DÉBASE")
	_m_row(_monnaie_page, "Débase", "debase_state", "Expense")
	_m_row(_monnaie_page, "Sur-frappe au-delà de la parité", "debase_slider", "RowLabel", 1, 6)
	_m_debase_warn = Label.new()
	_m_debase_warn.theme_type_variation = "RowDim"
	_m_debase_warn.autowrap_mode = TextServer.AUTOWRAP_WORD
	_m_debase_warn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_m_debase_warn.custom_minimum_size = Vector2(0, 32)
	_m_debase_warn.text = "⚠ sur-frappe payée en confiance — ronge la réserve de confiance (K) de la capitale, attise les marchands."
	_monnaie_page.add_child(_m_debase_warn)

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
	# UI-POLISH #5 : bouton NU (aucun override) retombait au thème Godot par défaut
	# (graphite) faute de style "Button" de base dans ParchTheme (qui ne définit que la
	# variation "Tab") — doctrine UI-4 : danger = rouge sombre, même famille que le ruban
	# Pause (topbar.gd, Color(0.38,0.08,0.07) fond / Color(0.78,0.62,0.30) liseré or).
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
	# RÉSERVE
	if w.has_method("country_reserve"):
		var res: Dictionary = w.country_reserve(me)
		_set_m("reserve", "%s or · %s cuivre" % [_grp(int(round(float(res.get("gold", 0.0))))), _grp(int(round(float(res.get("copper", 0.0)))))])
	# FRAPPE + curseur (mult lu via budget_controls — même valeur que la Balance)
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
	# DÉBASE : la fraction EFFECTIVE (pas seulement le curseur — country_debase_frac
	# reflète le kill-switch DEBASE_MAX/la cicatrice de banqueroute).
	var debase_frac := float(w.country_debase_frac(me)) if w.has_method("country_debase_frac") else 0.0
	var debase_active := debase_frac > 0.001
	_set_m("debase_state", ("active — +%.0f %%" % (debase_frac * 100.0)) if debase_active else "inactive",
		ParchTheme.EXPENSE if debase_active else ParchTheme.DIM_INK)
	_set_m("debase_slider", "%d %%" % int(round(debase_mult * 100.0)))
	_sync_slider("1:6", debase_mult * 100.0)
	if _m_debase_warn != null:
		_m_debase_warn.add_theme_color_override("font_color", ParchTheme.EXPENSE if debase_active else ParchTheme.DIM_INK)
	# DETTE
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
		# MONNAIE M14 — B7 : `due` est l'échéance RÉELLEMENT prélevée (credit_year_tick,
		# scps_credit.c — 10 %/an du stock sous DEBT_FIXED) — `taux` n'est QUE le taux
		# d'origination d'un NOUVEL emprunt, jamais appliqué à la dette déjà inscrite ;
		# l'ancien calcul (total*taux, 2-5 %) affichait un montant bien trop bas. Reader
		# dédié (scps_api.c scps_country_debt) — aucune constante dupliquée ici.
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
		# D3 — RÉSIDU DOCTRINE : `due` est un prélèvement RÉELLEMENT annuel (credit_year_tick,
		# scps_credit.c, 1×/an) — pas un flux continu comme l'impôt. « or/an » resterait
		# ambigu (lu comme un débit récurrent /mois mal étiqueté, cf. le bug province_panel.
		# gd:317 corrigé en D1) ; la cadence est dite en toutes lettres au lieu du calcul
		# fictif due/12 (qui ne correspond à AUCUN prélèvement réel — VALEUR RÉELLE, jamais
		# le calcul).
		_set_m("debt_due", "~%s or (prélevés 1×/an)" % _grp(int(round(due))) if total > 0.5 else "—")
	# EMPRUNTER À UN ORDRE
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
	# BANQUEROUTE
	if _m_bankrupt_btn != null:
		_m_bankrupt_btn.text = "Confirmer la banqueroute ?" if _m_bankrupt_armed else "Répudier la dette (banqueroute)"
		# UI-POLISH #5 : le bouton est DÉJÀ rouge danger par défaut (stylebox dédié
		# ci-dessus) — armé, on l'ÉCLAIRCIT (au lieu de l'assombrir comme le ferait
		# l'ancien modulate 1.0/0.55/0.5 multiplié sur un fond déjà sombre).
		_m_bankrupt_btn.modulate = Color(1.35, 1.2, 1.15) if _m_bankrupt_armed else Color(1, 1, 1)
	# FISCALITÉ PAR ORDRE — taux (curseur) + satisfaction (l'info qui rend le levier
	# jouable : « Bourgeois 70 % → tu peux serrer »).
	if w.has_method("country_fiscal_orders"):
		var fo: Array = w.country_fiscal_orders(me)
		for cls in range(mini(3, fo.size())):
			var f: Dictionary = fo[cls]
			var sat := int(f.get("satisfaction", -1))
			var taux3 := float(f.get("taux", 1.0))
			var revenu := float(f.get("revenu_mois", 0.0))
			# UI-POLISH #8 — VÉRIFIÉ, PAS UN BUG D'ENUM : satisfaction=-1 est le sentinel
			# légitime d'econ_country_class_satisfaction (scps_econ.c) quand cet ORDRE n'a
			# ENCORE aucune âme dans le pays (ex. Bourgeois en tout début de partie — les
			# Journaliers/Élite peuvent très bien être peuplés pendant que Bourgeois=0).
			# « — · 0 or/mois » mélangeait un « rien à mesurer » (tiret) et un chiffre
			# (zéro) pour la MÊME absence : les deux disent « — » désormais (cohérence
			# avec la règle #9 : jamais un nombre à côté d'un tiret pour un état inexistant).
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

## couleur de score 0-100 (rouge bas / vert haut) — motif empire_window._score_col.
func _score_col(v: int) -> Color:
	if v >= 60:
		return ParchTheme.GREEN
	if v < 40:
		return ParchTheme.RED
	return ParchTheme.INK

# ── ONGLET MARCHÉ (U2) : prix courants par bien + tendance /mois ──────────────
## lue seule (reconstruite à chaque refresh — pas de curseur à préserver, motif
## empire_window onglets Population/Diplomatie). La TENDANCE est un suivi CLIENT
## (historique local, non sérialisé, motif economy_panel.gd) : le moteur n'expose pas
## de delta de prix, on OBSERVE la variation d'un refresh à l'autre.
var _marche_hist := {}   # res_id -> {"prev": float, "day": int}

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
		# tendance : delta depuis le dernier ÉCHANTILLON observé (motif topbar _d_gold),
		# affiché /mois (doctrine : jamais un calcul, jamais l'annuel).
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

# ── util : séparateur de milliers (fin espace insécable) ──────────────────────
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
