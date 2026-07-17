extends VBoxContainer
## EconomyPage — le CORPS du grand livre (deux colonnes Rentrées/Sorties), extrait pour
## être RÉUTILISÉ comme onglet « Économie » de la fenêtre Empire.
##
## D1-UNIFICATION (2026-07-18) : cette page était REBÂTIE en doublon interactif de
## budget_panel_v2.gd (onglet Balance, touche B) — MÊME verbe joueur
## (player_budget_policy), même family/index, mais deux HSlider DISTINCTS sans lien
## entre eux (cartographie UI §D.1.3). Décision : LE TRÉSOR (budget_panel_v2, poli
## récemment) reste la SEULE surface de RÉGLAGE ; cette page redevient LECTURE SEULE
## (interactive=false) — les valeurs restent visibles ici (vue d'ensemble légitime de
## la Fenêtre Empire), un lien explicite renvoie régler au Trésor. Le pilote budget_v2
## garde ses propres curseurs, non touché.
const ParchTheme = preload("res://ui/parch_theme.gd")

const INCOME  := ParchTheme.INCOME
const EXPENSE := ParchTheme.EXPENSE
const DIVIDER := ParchTheme.DIVIDER

# les postes de DÉPENSE pilotables (family 1) portent un curseur EN MODE interactif ;
# les autres sont lus. 5 = Frappe (MONNAIE M2) : même motif que les 5 enveloppes.
const SPEND_HAS_SLIDER := {0: true, 1: true, 2: true, 3: true, 4: true, 5: true}

## D1-UNIFICATION : false ici (Fenêtre Empire, doublon retiré) — le RÉGLAGE vit
## uniquement au Trésor (budget_panel_v2.gd, onglet Balance). Un futur appelant qui
## voudrait de VRAIS curseurs ailleurs peut repasser ceci à true (comportement
## d'origine préservé, juste plus le défaut).
var interactive := true
signal open_budget_requested   ## « Régler… » (mode non-interactif) → ouvrir le Trésor (B)

var _built := false
var _left_col: VBoxContainer = null
var _right_col: VBoxContainer = null
var _val_lbls := {}      # clé "tax:c" / "sp:i" / "flux:name" -> Label de valeur
var _sliders := {}       # "family:index" -> HSlider
var _flux_of := {}       # clé de _val_lbls -> nom du poste de flux (country_budget)

func _ready() -> void:
	add_theme_constant_override("separation", 4)
	_build_shell()
	if Sim.has_signal("month_ticked"):
		Sim.month_ticked.connect(func(_y): if visible: refresh())
	refresh()

func _build_shell() -> void:
	var body := HBoxContainer.new()
	body.add_theme_constant_override("separation", 12)
	add_child(body)
	_left_col = VBoxContainer.new()
	_left_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body.add_child(_left_col)
	var div := Panel.new()
	div.custom_minimum_size = Vector2(1, 0)
	div.add_theme_stylebox_override("panel", ParchTheme.sb(DIVIDER, Color(0, 0, 0, 0), 0, 0, 0, 0, 0, 0))
	body.add_child(div)
	_right_col = VBoxContainer.new()
	_right_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body.add_child(_right_col)

func _section(col: VBoxContainer, txt: String) -> void:
	var l := Label.new()
	l.theme_type_variation = "Section"
	l.text = txt
	col.add_child(l)

## une ligne : label … valeur (colorée), + curseur optionnel dessous.
func _row(col: VBoxContainer, label: String, key: String, value_variation: String,
		slider_family := -1, slider_index := -1) -> void:
	var line := HBoxContainer.new()
	col.add_child(line)
	var lab := Label.new()
	lab.theme_type_variation = "RowLabel"
	lab.text = label
	line.add_child(lab)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	line.add_child(sp)
	var val := Label.new()
	val.theme_type_variation = value_variation
	val.text = "—"
	val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	line.add_child(val)
	_val_lbls[key] = val
	if slider_family >= 0 and interactive:
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
		col.add_child(s)
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

func refresh() -> void:
	var w = Sim.world
	if w == null:
		return
	var me: int = int(w.player()) if w.has_method("player") else 0
	if not _built:
		_build_body(me)
		_built = true
	_update_values(me)

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

	# LEFT — RENTRÉES : impôt par classe (curseur family 0) + postes LUS Export / Péages.
	_section(_left_col, "RENTRÉES")
	for raw in ctl.get("taxes", []):
		var row: Dictionary = raw
		var cls := int(row.get("id", 0))
		_row(_left_col, String(row.get("name", "Impôt")), "tax:%d" % cls, "Income", 0, cls)
	_flux_row(_left_col, "Export", "export", "Income")
	_flux_row(_left_col, "Péages", "péages+", "Income")
	# MONNAIE M1 — la réserve métallique (redevance minière), lecteur pur, sans curseur.
	_row(_left_col, "Réserve", "reserve", "Income")

	# RIGHT — SORTIES : enveloppes pilotables (curseur family 1) + postes LUS Conseil / Cour.
	_section(_right_col, "SORTIES")
	for raw2 in ctl.get("spending", []):
		var row2: Dictionary = raw2
		var idx := int(row2.get("id", 0))
		var has_slider: bool = bool(SPEND_HAS_SLIDER.get(idx, false))
		_row(_right_col, String(row2.get("name", "Dépense")), "sp:%d" % idx, "Expense",
			1 if has_slider else -1, idx if has_slider else -1)
	_flux_row(_right_col, "Conseil", "conseil", "Expense")
	_flux_row(_right_col, "Cour", "cour", "Expense")

	# D1-UNIFICATION : lecture seule ici → lien explicite vers l'UNIQUE surface de
	# réglage (le Trésor, touche B) plutôt que de dupliquer les curseurs.
	if not interactive:
		var link := Button.new()
		link.text = "Régler… → Trésor (B)"
		link.focus_mode = Control.FOCUS_NONE
		link.tooltip_text = "Ouvre le Trésor (onglet Balance) : c'est là que se règlent taux d'imposition et enveloppes."
		link.pressed.connect(func(): open_budget_requested.emit())
		add_child(link)

func _update_values(me: int) -> void:
	var w = Sim.world
	var doy := 1
	if w.has_method("day_of_year"):
		doy = maxi(1, int(w.day_of_year()))
	var mf := 30.0 / float(doy)
	# MONNAIE M1 — la réserve métallique (redevance minière, jamais marchande).
	var reserve_lbl: Label = _val_lbls.get("reserve", null)
	if reserve_lbl != null and w.has_method("country_reserve"):
		var res: Dictionary = w.country_reserve(me)
		reserve_lbl.text = "%s or · %s cuivre" % [_grp(int(round(float(res.get("gold", 0.0))))), _grp(int(round(float(res.get("copper", 0.0)))))]
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
				lbl.text = "taux %d %%" % int(round(float(row.get("mult", 1.0)) * 100.0))
		var sl: HSlider = _sliders.get("0:%d" % cls, null)
		if sl != null and not sl.has_focus():
			sl.set_value_no_signal(clampf(float(row.get("mult", 1.0)) * 100.0, 2.0, 100.0))
	# flux nommés (rentrées lues + sorties), ramenés au mois
	var flux := {}
	if w.has_method("country_budget"):
		for p in w.country_budget(me):
			flux[String(p.get("name", ""))] = float(p.get("amount", 0.0)) * mf
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
				# MONNAIE M2 — LA FRAPPE : lecteur DÉDIÉ, miroir exact (revenu, pas dépense).
				lbl3.text = "+%s or/mois" % _grp(int(round(float(w.country_mint_month(me)))))
			else:
				var amt := 0.0
				if idx >= 0 and idx < spend_flux.size():
					amt = absf(float(flux.get(spend_flux[idx], 0.0)))
				lbl3.text = "%s or/mois" % _grp(int(round(amt)))
		var sl2: HSlider = _sliders.get("1:%d" % idx, null)
		if sl2 != null and not sl2.has_focus():
			sl2.set_value_no_signal(clampf(float(row2.get("mult", 1.0)) * 100.0, 2.0, 100.0))

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
