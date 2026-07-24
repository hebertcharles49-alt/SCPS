extends Control
## COUNTRY ACTIONS — la fenêtre DIPLOMATIQUE d'un pays cible : le résumé exhaustif de la
## relation (statut · opinion ±100 · composantes · mémoire d'actes) + LES VERBES (guerre /
## paix / alliance / pacte / embargo), grisés par la légalité (diplo_options) ET par le
## DIPLOMATE (un émissaire, 1 acte / 2 mois — scps_diplo_cd). Ouverte par la liste diplo
## de la sidebar OU par le CLIC DROIT sur la carte. Zéro logique sim : lit la façade,
## enfile des verbes journalisés.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const InfoRef = preload("res://ui/info_ref.gd")
const DrawerK = preload("res://ui/sidebar_drawer.gd")   # DACT_LABEL partagé (mémoire datée)
const Frame = preload("res://ui/frame.gd")
const OpinionBar = preload("res://ui/opinion_bar.gd")
const Concepts = preload("res://ui/concepts.gd")   # D4 — glossaire hover
const TooltipFactory = preload("res://ui/tooltip_factory.gd")   # checklist de refus (2026-07-21)

signal navigate_requested(request: Dictionary)

const PW := 380.0

var _cid := -1
var _btns := {}          ## verbe → Button
var _action_details := {} ## verbe → Label toujours visible (verrou · coût · délai · conséquence)
var _panel: PanelContainer
var _scroll: ScrollContainer
var _head: Label
var _arms_rect: TextureRect   ## les ARMES du pays cible (héraldique dérivée)
var _status: Label
var _opinion_course: Label   ## opinion actuelle → point d'équilibre calculé par ses composantes
var _opinion_widget: Control
var _sum_lbl: Label
var _engagement_lbl: Label
var _capital_btn: Button
var _capital_region := -1
var _cd_lbl: Label
var _cb_lbl: Label   ## W-GUERRE-3 : état de l'intrigue fabriquée (en cours / prête / coût)
var _context_hint: Label   ## rappel des panneaux où suivre les conséquences de la relation
var _flash: Label
var _legal_by_verb := {} ## verbe -> décision structurée du moteur (autorisé, raison, coût, délai)
var _econ_box: VBoxContainer
var _antag_box: VBoxContainer
var _peace_box: VBoxContainer
var _peace_open := false
var _peace_preview := {}
var _peace_selected := {}       ## region -> true
var _peace_checks := {}         ## terme -> CheckButton
var _peace_territory_box: VBoxContainer
var _peace_gold: HSlider
var _peace_gold_lbl: Label
var _peace_total_lbl: Label
var _peace_submit: Button

const PEACE_FLAGS := {"reparations": 1, "humiliate": 2, "pillage": 4,
	"liberate": 8, "vassalize": 16, "fragment": 32}

# UI-4 (retour joueur 2026-07-10) : hiérarchie d'actions — Guerre est DESTRUCTIF (rouge
# sombre + confirmation 2 clics, motif _servile_manumit_armed/province_panel _purge_armed) ;
# Paix/Allier/Pacte/Migration/Embargo restent SECONDAIRES (thème neutre inchangé).
const BTN_LABELS := {"war": "Déclarer la guerre", "peace": "Faire la paix", "ally": "Proposer une alliance", "pact": "Pacte commercial",
	"migration": "Pacte migratoire", "embargo": "Embargo"}
const ACTION_HELP := {
	"war": "Déclare une guerre. La relation bascule immédiatement ; les opérations se suivent dans l'onglet Armée.",
	"peace": "Ouvre les conditions de paix : territoires, or, réparations, humiliation, pillage, libération, vasselage ou fragmentation.",
	"ally": "Propose une alliance bilatérale. L'opinion décide de l'acceptation.",
	"pact": "Propose un pacte commercial. Il ouvre les échanges et nourrit le contact entre les peuples.",
	"migration": "Propose un pacte migratoire. Des populations pourront circuler entre les deux pays.",
	"embargo": "Ferme ou rouvre unilatéralement le commerce avec ce pays ; l'opinion et les routes suivent.",
	"fabricate": "Finance pendant un an une revendication sur le territoire nommé. À maturité, elle ouvre temporairement un casus belli.",
}
const DIPLO_ACTION_ID := {
	"war": 0, "peace": 1, "ally": 2, "pact": 3,
	"migration": 4, "embargo": 5, "fabricate": 6,
}
var _war_armed := false
var _war_armed_ms := -100000
var _war_sb_idle: StyleBoxFlat
var _war_sb_hover: StyleBoxFlat
var _war_sb_press: StyleBoxFlat
var _war_sb_armed: StyleBoxFlat

func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build()
	get_viewport().size_changed.connect(_layout)
	Sim.ticked.connect(func(_y): if visible: _refresh())

## la fenêtre de confirmation « Guerre » (4 s) retombe même en pause (Sim.ticked ne
## tourne pas si le jeu est arrêté ; ce Control, si — miroir province_panel._process).
func _process(_dt: float) -> void:
	if _war_armed and Time.get_ticks_msec() - _war_armed_ms > 4000:
		_war_armed = false
		if visible:
			_refresh()

func _build() -> void:
	_panel = PanelContainer.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = VKit.COL_PANEL
	sb.border_color = VKit.COL_EDGE
	sb.set_border_width_all(1)
	sb.set_border_width(SIDE_TOP, 3)
	sb.set_corner_radius_all(1)
	sb.set_content_margin_all(10)   # RÉDUIT (retour joueur 2026-07-21 « trop chunky ») : 14→10
	_panel.add_theme_stylebox_override("panel", sb)
	add_child(_panel)

	_scroll = ScrollContainer.new()
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_panel.add_child(_scroll)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 4)
	col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.add_child(col)

	var hrow := HBoxContainer.new()
	col.add_child(hrow)
	_arms_rect = TextureRect.new()
	_arms_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_arms_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_arms_rect.custom_minimum_size = Vector2(34, 34)
	hrow.add_child(_arms_rect)
	_head = Label.new()
	_head.add_theme_font_size_override("font_size", 18)
	_head.add_theme_color_override("font_color", Color(0.86, 0.70, 0.42))
	hrow.add_child(_head)
	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hrow.add_child(sp)
	var closeb := Button.new()
	closeb.text = "✕"
	closeb.pressed.connect(func():
		visible = false
		Sound.play("ui_parchment_close"))
	hrow.add_child(closeb)

	var resume_head := Label.new()
	resume_head.text = "RÉSUMÉ DU PAYS"
	resume_head.add_theme_font_size_override("font_size", 12)
	resume_head.add_theme_color_override("font_color", VKit.COL_GOLD)
	col.add_child(resume_head)
	_sum_lbl = Label.new()
	_sum_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_sum_lbl.add_theme_font_size_override("font_size", 12)
	_sum_lbl.add_theme_color_override("font_color", VKit.COL_PARCH)
	col.add_child(_sum_lbl)

	var opinion_head := Label.new()
	opinion_head.text = "OPINION  −100 / +100"
	opinion_head.add_theme_font_size_override("font_size", 12)
	opinion_head.add_theme_color_override("font_color", VKit.COL_GOLD)
	# D4 — glossaire hover : la jauge n'a autrement AUCUN survol nommant le concept.
	opinion_head.tooltip_text = Concepts.def_of("Opinion")
	opinion_head.mouse_filter = Control.MOUSE_FILTER_STOP
	col.add_child(opinion_head)
	_opinion_widget = OpinionBar.new()
	col.add_child(_opinion_widget)

	_opinion_course = Label.new()
	_opinion_course.add_theme_font_size_override("font_size", 12)
	_opinion_course.add_theme_color_override("font_color", VKit.COL_PARCH)
	col.add_child(_opinion_course)

	var status_head := Label.new()
	status_head.text = "STATUT DIPLOMATIQUE"
	status_head.add_theme_font_size_override("font_size", 12)
	status_head.add_theme_color_override("font_color", VKit.COL_GOLD)
	col.add_child(status_head)
	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(0.75, 0.72, 0.68))
	col.add_child(_status)
	_engagement_lbl = Label.new()
	_engagement_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_engagement_lbl.custom_minimum_size = Vector2(PW - 40.0, 0)
	_engagement_lbl.add_theme_font_size_override("font_size", 12)
	_engagement_lbl.add_theme_color_override("font_color", VKit.COL_PARCH)
	col.add_child(_engagement_lbl)

	_capital_btn = Button.new()
	_capital_btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	_capital_btn.pressed.connect(func():
		if _capital_region >= 0:
			navigate_requested.emit(InfoRef.request(InfoRef.make(InfoRef.REGION, _capital_region), "map")))
	col.add_child(_capital_btn)

	_cd_lbl = Label.new()
	_cd_lbl.add_theme_color_override("font_color", Color(0.85, 0.65, 0.30))
	col.add_child(_cd_lbl)

	_cb_lbl = Label.new()
	_cb_lbl.add_theme_color_override("font_color", Color(0.70, 0.55, 0.75))
	col.add_child(_cb_lbl)

	# UI-4 : les styleboxes DESTRUCTIFS de « Guerre » (rouge sombre — distinct du chrome
	# cuir/or par défaut des verbes secondaires) — précalculés une fois.
	_war_sb_idle = _mkbox(Color(0.22, 0.06, 0.05), Color(0.58, 0.18, 0.13), 2)
	_war_sb_hover = _mkbox(Color(0.30, 0.09, 0.07), Color(0.80, 0.26, 0.18), 2)
	_war_sb_press = _mkbox(Color(0.14, 0.04, 0.03), Color(0.46, 0.13, 0.10), 2, true)
	_war_sb_armed = _mkbox(Color(0.48, 0.12, 0.09), Color(0.95, 0.36, 0.25), 2)

	var actions_head := Label.new()
	actions_head.text = "ACTIONS DIPLOMATIQUES"
	actions_head.add_theme_font_size_override("font_size", 12)
	actions_head.add_theme_color_override("font_color", VKit.COL_GOLD)
	col.add_child(actions_head)
	_add_action(col, "ally", "Proposer une alliance")
	_add_action(col, "war", "⚔ Déclarer la guerre")
	_add_action(col, "peace", "Faire la paix", func(): _toggle_peace())
	_peace_box = VBoxContainer.new()
	_peace_box.add_theme_constant_override("separation", 3)
	_peace_box.visible = false
	col.add_child(_peace_box)
	_build_peace_drawer()

	var eco_toggle := Button.new()
	eco_toggle.text = "▸ ACTIONS ÉCONOMIQUES"
	eco_toggle.alignment = HORIZONTAL_ALIGNMENT_LEFT
	col.add_child(eco_toggle)
	_econ_box = VBoxContainer.new()
	_econ_box.add_theme_constant_override("separation", 3)
	_econ_box.visible = false
	col.add_child(_econ_box)
	_add_action(_econ_box, "migration", "Pacte migratoire")
	_add_action(_econ_box, "pact", "Pacte commercial")
	# UI-MONNAIE (2026-07-16) — U3 : L'EMPRUNT D'ÉTAT (MONNAIE M9 V2). Verbe et lecteurs
	# à PART (player_request_loan/country_loan_status) — pas dans DIPLO_ACTION_ID/diplo_
	# action_legal (motif « fabricate », géré hors boucle générique ci-dessous). Le detail
	# Label posé par _add_action (motif fab_btn) porte l'état « [État] étudie/accorde/refuse ».
	_add_action(_econ_box, "request_loan", "Demander un emprunt", func(): _loan_press())
	eco_toggle.pressed.connect(func():
		_econ_box.visible = not _econ_box.visible
		eco_toggle.text = ("▾" if _econ_box.visible else "▸") + " ACTIONS ÉCONOMIQUES")

	var antag_toggle := Button.new()
	antag_toggle.text = "▸ ACTIONS ANTAGONISTES"
	antag_toggle.alignment = HORIZONTAL_ALIGNMENT_LEFT
	col.add_child(antag_toggle)
	_antag_box = VBoxContainer.new()
	_antag_box.add_theme_constant_override("separation", 3)
	_antag_box.visible = false
	col.add_child(_antag_box)
	_add_action(_antag_box, "fabricate", "Revendiquer un territoire")
	_add_action(_antag_box, "embargo", "Embargo")
	antag_toggle.pressed.connect(func():
		_antag_box.visible = not _antag_box.visible
		antag_toggle.text = ("▾" if _antag_box.visible else "▸") + " ACTIONS ANTAGONISTES")

	_context_hint = Label.new()
	_context_hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_context_hint.add_theme_font_size_override("font_size", 12)
	_context_hint.add_theme_color_override("font_color", VKit.COL_DIM)
	col.add_child(_context_hint)

	_flash = Label.new()
	_flash.add_theme_color_override("font_color", Color(0.46, 0.74, 0.42))
	col.add_child(_flash)
	_layout()

func _add_action(parent: VBoxContainer, verb: String, label: String, custom_press: Callable = Callable()) -> void:
	var b := Button.new()
	b.text = label
	b.alignment = HORIZONTAL_ALIGNMENT_LEFT
	b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	b.custom_minimum_size = Vector2(0, 32)
	if verb == "war":
		b.add_theme_stylebox_override("normal", _war_sb_idle)
		b.add_theme_stylebox_override("hover", _war_sb_hover)
		b.add_theme_stylebox_override("pressed", _war_sb_press)
		b.add_theme_color_override("font_color", Color(0.94, 0.82, 0.78))
		b.pressed.connect(func(): _war_press())
	elif custom_press.is_valid():
		b.pressed.connect(custom_press)
	else:
		b.pressed.connect(func(): _act(verb))
	parent.add_child(b)
	_btns[verb] = b
	var detail := Label.new()
	detail.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	detail.add_theme_font_size_override("font_size", 11)
	detail.add_theme_color_override("font_color", VKit.COL_DIM)
	parent.add_child(detail)
	_action_details[verb] = detail

func _build_peace_drawer() -> void:
	var title := Label.new()
	title.text = "CONDITIONS DE PAIX"
	title.add_theme_color_override("font_color", VKit.COL_GOLD)
	_peace_box.add_child(title)
	_peace_territory_box = VBoxContainer.new()
	_peace_territory_box.add_theme_constant_override("separation", 2)
	_peace_box.add_child(_peace_territory_box)
	var gold_title := Label.new()
	gold_title.text = "Or à prendre — 1 score = 3 % du revenu mensuel"
	gold_title.add_theme_font_size_override("font_size", 11)
	gold_title.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_peace_box.add_child(gold_title)
	_peace_gold = HSlider.new()
	_peace_gold.min_value = 0
	_peace_gold.max_value = 25
	_peace_gold.step = 1
	_peace_gold.value_changed.connect(func(_v): _peace_update_total())
	_peace_box.add_child(_peace_gold)
	_peace_gold_lbl = Label.new()
	_peace_gold_lbl.add_theme_font_size_override("font_size", 11)
	_peace_box.add_child(_peace_gold_lbl)
	var terms := [
		["reparations", "Réparations de guerre — 10 score", "10 % du revenu mensuel pendant 10 ans"],
		["humiliate", "Humilier — 20 score", "Tous les conseillers en fonction meurent"],
		["pillage", "Piller — 10 score", "5 % de chaque stock national est transféré"],
		["liberate", "Libérer — 50 score", "L'éthos de tout le pays devient le vôtre"],
		["vassalize", "Vassaliser", "Coût additif de toutes les provinces restantes"],
		["fragment", "Fragmenter — 100 score", "Chaque région devient une entité indépendante"],
	]
	for row in terms:
		var cb := CheckButton.new()
		cb.text = row[1]
		cb.toggled.connect(func(_on): _peace_update_total())
		# D4 — « Vassaliser » nomme directement le concept (le libellé porte son propre
		# coût en score, pas le mot « Vassalité » lui-même — sinon aucun survol ne l'explique).
		if row[0] == "vassalize":
			cb.tooltip_text = Concepts.def_of("Vassalité")
		_peace_box.add_child(cb)
		_peace_checks[row[0]] = cb
		var why := Label.new()
		why.text = row[2]
		why.add_theme_font_size_override("font_size", 11)
		why.add_theme_color_override("font_color", VKit.COL_DIM)
		why.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		_peace_box.add_child(why)
	_peace_total_lbl = Label.new()
	_peace_total_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_peace_total_lbl.add_theme_color_override("font_color", VKit.COL_GOLD)
	_peace_box.add_child(_peace_total_lbl)
	_peace_submit = Button.new()
	_peace_submit.text = "Envoyer les conditions"
	_peace_submit.pressed.connect(func(): _submit_peace())
	_peace_box.add_child(_peace_submit)

func _toggle_peace() -> void:
	if _btns.get("peace") == null or (_btns["peace"] as Button).disabled:
		return
	_peace_open = not _peace_open
	_peace_box.visible = _peace_open
	if _peace_open:
		_peace_selected.clear()
		_peace_gold.value = 0
		for cb in _peace_checks.values():
			(cb as CheckButton).button_pressed = false
		_refresh()

func _peace_refresh() -> void:
	if not _peace_open or Sim.world == null:
		return
	_peace_preview = Sim.world.peace_terms(_cid) if Sim.world.has_method("peace_terms") else {}
	_rebuild_peace_territories()
	var per := float(_peace_preview.get("gold_per_score", 0.0))
	var have := float(_peace_preview.get("gold_available", 0.0))
	_peace_gold.max_value = minf(25.0, floorf(have / per)) if per > 0.0 else 0.0
	_peace_gold.value = minf(_peace_gold.value, _peace_gold.max_value)
	var vcost := float(_peace_preview.get("vassal_score", 0.0))
	(_peace_checks["vassalize"] as CheckButton).text = "Vassaliser — %.1f score" % vcost
	var frag: CheckButton = _peace_checks["fragment"]
	frag.disabled = not bool(_peace_preview.get("fragment_possible", false))
	frag.tooltip_text = "Aucun emplacement de pays disponible" if frag.disabled else "Divise le pays en États d'une région."
	if frag.disabled:
		frag.button_pressed = false
	_peace_update_total()

func _rebuild_peace_territories() -> void:
	for child in _peace_territory_box.get_children():
		child.queue_free()
	var head := Label.new()
	head.text = "PRENDRE DES TERRITOIRES"
	head.add_theme_font_size_override("font_size", 11)
	head.add_theme_color_override("font_color", VKit.COL_GOLD)
	_peace_territory_box.add_child(head)
	var still := {}
	for tr in _peace_preview.get("territories", []):
		var region := int(tr.get("region", -1))
		var occupied := bool(tr.get("occupied", false))
		var cb := CheckButton.new()
		cb.text = "%s — %.1f score%s" % [String(tr.get("name", "Territoire")),
			float(tr.get("score_cost", 0.0)), "" if occupied else " — non occupé"]
		cb.disabled = not occupied
		cb.button_pressed = occupied and bool(_peace_selected.get(region, false))
		cb.toggled.connect(func(on: bool):
			if on: _peace_selected[region] = true
			else: _peace_selected.erase(region)
			_peace_update_total())
		_peace_territory_box.add_child(cb)
		if cb.button_pressed: still[region] = true
	_peace_selected = still

func _peace_cost() -> float:
	var total := float(_peace_gold.value)
	for tr in _peace_preview.get("territories", []):
		if _peace_selected.has(int(tr.get("region", -1))):
			total += float(tr.get("score_cost", 0.0))
	if (_peace_checks["reparations"] as CheckButton).button_pressed: total += 10.0
	if (_peace_checks["humiliate"] as CheckButton).button_pressed: total += 20.0
	if (_peace_checks["pillage"] as CheckButton).button_pressed: total += 10.0
	if (_peace_checks["liberate"] as CheckButton).button_pressed: total += 50.0
	if (_peace_checks["vassalize"] as CheckButton).button_pressed: total += float(_peace_preview.get("vassal_score", 0.0))
	if (_peace_checks["fragment"] as CheckButton).button_pressed: total += 100.0
	return total

func _peace_update_total() -> void:
	if _peace_total_lbl == null:
		return
	var gold_score := int(round(_peace_gold.value))
	var gold := minf(float(_peace_preview.get("gold_available", 0.0)),
		float(gold_score) * float(_peace_preview.get("gold_per_score", 0.0)))
	_peace_gold_lbl.text = "%d score → %.2f or physique" % [gold_score, gold]
	var total := _peace_cost()
	var available := maxf(0.0, float(_peace_preview.get("war_score", 0.0)))
	var cd := int(Sim.world.diplo_cd()) if Sim.world != null and Sim.world.has_method("diplo_cd") else 0
	_peace_total_lbl.text = "Coût total %.1f / %.1f score disponible" % [total, available]
	_peace_total_lbl.add_theme_color_override("font_color", VKit.sense(0.80) if total <= available else VKit.sense(0.15))
	_peace_submit.disabled = total > available + 0.01 or cd > 0
	_peace_submit.tooltip_text = ("Émissaire disponible dans %d j" % cd) if cd > 0 else ("Score de guerre insuffisant" if total > available else "Conditions exécutées au prochain tick si elles restent valides.")

func _submit_peace() -> void:
	var regs := PackedInt32Array()
	var keys := _peace_selected.keys()
	keys.sort()
	for r in keys: regs.append(int(r))
	var flags := 0
	for key in PEACE_FLAGS:
		if (_peace_checks[key] as CheckButton).button_pressed: flags |= int(PEACE_FLAGS[key])
	var ok := bool(Sim.world.player_peace_offer(_cid, regs, int(round(_peace_gold.value)), flags))
	_flash.text = "Conditions de paix envoyées." if ok else "Offre de paix refusée à l'enfilement."
	_flash.add_theme_color_override("font_color", VKit.sense(0.80) if ok else VKit.sense(0.15))

## petit StyleBoxFlat cuir/bordure (miroir ui_theme._box, dupliqué ici : country_actions
## n'a pas licence d'éditer ui_theme.gd, et une couleur DESTRUCTIVE n'a pas sa place dans
## le thème global neutre).
static func _mkbox(bg: Color, border: Color, bw: int = 2, shift_down := false) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = bg
	sb.border_color = border
	sb.set_border_width_all(bw)
	sb.set_corner_radius_all(3)
	sb.content_margin_left = 10.0
	sb.content_margin_right = 10.0
	sb.content_margin_top = 5.0 + (2.0 if shift_down else 0.0)
	sb.content_margin_bottom = 5.0 - (2.0 if shift_down else 0.0)
	return sb

## UI-4 : « Guerre » exige 2 clics — le 1er ARME la confirmation (rien n'est déclaré), le
## 2e (dans les 4 s, cf. _process) déclare pour de vrai. Jamais de popup modal.
func _war_press() -> void:
	if not _war_armed:
		_war_armed = true
		_war_armed_ms = Time.get_ticks_msec()
		_refresh()
		return
	_war_armed = false
	_act("war")

func _layout() -> void:
	var vp := get_viewport_rect().size
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	# TIROIR DIPLOMATIQUE — RÉDUIT/DÉCOLLÉ/DÉFILABLE (retour joueur 2026-07-21 « trop
	# chunky ») : plus étroit (0.28→0.24, 420-540→360-440), DÉCOLLÉ du rail/topbar par une
	# marge, et sa hauteur HUG le contenu VISIBLE au lieu du plein écran — au-delà d'un
	# plafond, le ScrollContainer existant prend le relais (la barre n'apparaissait jamais
	# tant que le panneau se forçait à toute la hauteur).
	var margin := 12.0
	var drawer_w := clampf(vp.x * 0.24, 360.0, 440.0)
	_panel.position = Vector2(Frame.SIDEBAR_W + margin, Frame.TOPBAR_H + margin)
	var ceiling := maxf(160.0, vp.y - Frame.TOPBAR_H - 2.0 * margin)
	var content_h := 160.0
	if _scroll != null and _scroll.get_child_count() > 0:
		# +20 = les content_margin haut+bas du panneau (10×2) autour du scroll
		content_h = (_scroll.get_child(0) as Control).get_combined_minimum_size().y + 20.0
	_panel.custom_minimum_size = Vector2(drawer_w, 0)
	_panel.size = Vector2(drawer_w, clampf(content_h, 160.0, ceiling))

func open_country(cid: int) -> void:
	if Sim.world == null or cid < 0 or cid == int(Sim.world.player()):
		return
	# BROUILLARD : un pays jamais découvert ne se laisse pas approcher (retour joueur)
	if Sim.world.has_method("country_known") and int(Sim.world.country_known(cid)) == 0:
		return
	if _cid != cid:
		_peace_open = false
		_peace_selected.clear()
		if _peace_box != null: _peace_box.visible = false
	_cid = cid
	visible = true
	Sound.play("ui_parchment_open")
	_flash.text = ""
	if _arms_rect != null:
		_arms_rect.texture = load("res://ui/heraldry.gd").arms(cid)
	_refresh()
	_layout()

func _refresh() -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var info: Dictionary = w.country_info(_cid)
	_head.text = String(info.get("nom", "?"))
	var roles := {0: "Royaume joueur", 1: "Grande puissance", 2: "Cité-État", 3: "Terre sans État", 4: "Peuple libre"}
	_sum_lbl.text = "Habitants : %s\nÉthos / régime effectif : %s\nStatut politique : %s · %d territoire(s)" % [
		str(int(info.get("pop", 0))), String(info.get("ethos", "—")),
		String(roles.get(int(w.country_role(_cid)), "Entité politique")), int(info.get("regions", 0))]
	# la relation vue du joueur : statut + opinion (ce que LUI pense de NOUS)
	var rel := {}
	for rl in w.country_relations(w.player()):
		if int(rl.get("country", -1)) == _cid:
			rel = rl
			break
	var op := int(rel.get("opinion", 0))
	var stat_word := String(rel.get("status", "—"))
	_status.text = "Statut : %s" % stat_word
	# D4 — « Vassal »/« Suzerain » sont un mot NU (pas la clé DEFS « Vassalité ») :
	# sans ce mapping manuel, ce statut n'a jamais de survol nommant le concept.
	if stat_word == "Vassal" or stat_word == "Suzerain":
		_status.tooltip_text = Concepts.def_of("Vassalité")
		_status.mouse_filter = Control.MOUSE_FILTER_STOP
	else:
		_status.tooltip_text = ""
	# le RÉSUMÉ : composantes d'opinion + les 3 derniers actes (la mémoire)
	var parts := PackedStringArray()
	var opinion_target := 0
	if w.has_method("opinion_summary"):
		var ps: Dictionary = w.opinion_summary(_cid)
		for pk in [["Alliance", "ally"], ["Guerre", "war"], ["Vassalité", "vassal"],
			["Pacte", "pact"], ["Embargo", "embargo"], ["Rancune", "rancor"], ["Mémoire", "memory"]]:
			var v := int(ps.get(pk[1], 0))
			if v != 0:
				parts.append("%s %+d" % [pk[0], v])
			opinion_target += v
	opinion_target = clampi(opinion_target, -100, 100)
	var drift := opinion_target - op
	var arrow := "→" if drift == 0 else ("↗" if drift > 0 else "↘")
	var course := "stable" if drift == 0 else ("se réchauffe" if drift > 0 else "se dégrade")
	_opinion_widget.set_values(op, opinion_target)
	_opinion_course.text = "Tendance %s  ·  %s" % [arrow, course]
	_opinion_course.add_theme_color_override("font_color",
		VKit.COL_DIM if drift == 0 else (VKit.sense(0.80) if drift > 0 else VKit.sense(0.15)))
	var mem := PackedStringArray()
	if w.has_method("diplo_journal"):
		var me2: int = int(w.player())
		var nj := 0
		for a in w.diplo_journal(_cid):
			if nj >= 3:
				break
			if not DrawerK.DACT_LABEL.has(int(a.get("act", -1))):
				continue
			var lab: Array = DrawerK.DACT_LABEL[int(a.get("act", -1))]
			var by_us: bool = int(a.get("a", -1)) == me2
			mem.append("an %d · %s" % [int(a.get("year", 0)), String(lab[1] if by_us else lab[0])])
			nj += 1
	if parts.size() > 0:
		_opinion_course.text += "\nFacteurs : " + ", ".join(parts)
	if mem.size() > 0:
		_opinion_course.text += "\nMémoire : " + " — ".join(mem)
	# P8 — engagements, portée et lieux viennent d'un lecteur unique. La fiche ne
	# reconstitue ni pacte, ni slots d'alliance, ni liaison commerciale.
	var ctx: Dictionary = w.diplo_context(_cid) if w.has_method("diplo_context") else {}
	var engagements := PackedStringArray()
	if bool(ctx.get("at_war", false)):
		engagements.append("guerre (score %+d)" % int(round(float(ctx.get("war_score", 0.0)))))
	if bool(ctx.get("allied", false)):
		engagements.append("alliance")
	if bool(ctx.get("trade_pact", false)):
		engagements.append("pacte commercial")
	if bool(ctx.get("migration_pact", false)):
		engagements.append("pacte migratoire")
	if bool(ctx.get("embargo", false)):
		engagements.append("embargo")
	var vdir := int(ctx.get("vassal_direction", 0))
	if vdir != 0:
		engagements.append(("notre vassal" if vdir > 0 else "notre suzerain") +
			(" · %s" % String(ctx.get("contract", "")) if String(ctx.get("contract", "")) != "" else ""))
	if engagements.is_empty():
		engagements.append("aucun engagement actif")
	var scope := "Alliances : nous %d/%d · eux %d/%d" % [int(ctx.get("ally_slots_player", 0)),
		int(ctx.get("ally_slots_max", 2)), int(ctx.get("ally_slots_target", 0)), int(ctx.get("ally_slots_max", 2))]
	var route := "Aucune liaison commerciale directe"
	var shared := int(ctx.get("shared_routes", 0))
	if shared > 0:
		route = "%s ↔ %s · %d/%d route(s) ouverte(s) · rendement %.1f" % [
			String(ctx.get("route_a_name", "?")), String(ctx.get("route_b_name", "?")),
			int(ctx.get("open_routes", 0)), shared, float(ctx.get("route_yield", 0.0))]
		if bool(ctx.get("route_maritime", false)):
			route += " · mer %.0f j" % float(ctx.get("route_sea_days", 0.0))
	_engagement_lbl.text = "En cours : %s\n%s\n%s" % [", ".join(engagements), scope, route]
	_capital_region = int(ctx.get("target_capital_region", -1))
	var capital_name := String(ctx.get("target_capital_name", ""))
	_capital_btn.text = "⌖ Voir la capitale%s" % (" — " + capital_name if capital_name != "" else "")
	_capital_btn.disabled = _capital_region < 0
	_capital_btn.tooltip_text = "Centrer la carte sur la capitale de ce pays."
	# le DIPLOMATE : cooldown → tous les verbes grisés + la raison affichée
	var cd := int(w.diplo_cd()) if w.has_method("diplo_cd") else 0
	_cd_lbl.text = ("Émissaire : retour dans %d j" % cd) if cd > 0 else "Émissaire : disponible"
	var op2: Dictionary = w.diplo_options(_cid) if w.has_method("diplo_options") else {}
	_legal_by_verb.clear()
	for legal_verb in DIPLO_ACTION_ID:
		_legal_by_verb[legal_verb] = _read_legal(w, legal_verb, op2, cd)
	# L'état de relation ne sert plus qu'au conseil contextuel. La légalité et sa raison
	# viennent exclusivement de diplo_action_legal : l'UI ne reconstruit aucune règle.
	var psr: Dictionary = w.opinion_summary(_cid) if w.has_method("opinion_summary") else {}
	var at_war: bool = int(psr.get("war", 0)) != 0
	var allied: bool = int(psr.get("ally", 0)) != 0
	var has_pact: bool = int(psr.get("pact", 0)) != 0
	if at_war:
		_context_hint.text = "À suivre : opérations dans Armée · ravages et occupations sur la carte · mémoire ici."
	elif allied or has_pact:
		_context_hint.text = "À suivre : échanges dans Marché · brassage dans les provinces · mémoire diplomatique ici."
	else:
		_context_hint.text = "Rappel : clic droit sur un pays ouvre cette fiche ; l'onglet Diplomatie compare tous les voisins."
	for verb in _btns:
		if verb == "fabricate" or verb == "request_loan":
			continue   # géré à part plus bas (texte/état dynamiques)
		var b: Button = _btns[verb]
		var legal: Dictionary = _legal_by_verb.get(verb, {})
		b.disabled = not bool(legal.get("allowed", false))
		if verb == "war" and b.disabled:
			_war_armed = false          # plus légal ⇒ la confirmation en attente retombe
		# AMBRE : permis mais l'offre serait REFUSÉE (l'opinion #26 prévisualisée)
		var amber: bool = (not b.disabled) and not bool(legal.get("unilateral", true)) \
			and not bool(legal.get("would_accept", true))
		# UI-5 (retour joueur : « la couleur seule ne suffit pas ») : l'ambre « il
		# refusera » ne se voyait qu'à la teinte du bouton (invisible avant le survol) —
		# un « ⚠ » sur le LIBELLÉ double le canal, visible sans survoler.
		var base_label: String = String(BTN_LABELS.get(verb, verb))
		if verb == "embargo" and not bool(legal.get("toggle_on", true)):
			base_label = "Lever l'embargo"
		if verb == "war":
			# DESTRUCTIF : le libellé PORTE la confirmation (« Confirmer la guerre ? »),
			# le fond bascule à un rouge plus vif tant que l'armement tient (4 s).
			b.text = "Confirmer la guerre ?" if _war_armed else "⚔ %s" % base_label
			b.add_theme_stylebox_override("normal", _war_sb_armed if _war_armed else _war_sb_idle)
			b.modulate = Color(1, 1, 1)
		else:
			b.text = ("%s ⚠" % base_label) if amber else base_label
			b.modulate = Color(1.0, 0.82, 0.5) if amber else Color(1, 1, 1)
		# RETOUR JOUEUR : chaque verbe GRISÉ nomme sa raison au survol (« pourquoi je peux pas ? »)
		if b.disabled:
			b.tooltip_text = _legal_tooltip(legal, "")
		elif verb == "war" and _war_armed:
			b.tooltip_text = "irréversible — cliquez de nouveau pour confirmer (4 s)"
		elif amber:
			b.tooltip_text = "il refusera (opinion trop basse)"
		else:
			b.tooltip_text = String(ACTION_HELP.get(verb, ""))
		_update_action_detail(verb, legal, amber)
	# W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ : « Guerre » reste grisé sans motif gratuit NI
	# intrigue mûre (can_declare_war le dit déjà côté moteur) ; « Fabriquer » porte l'état
	# de l'intrigue en cours/mûre/coût — un bouton de CORRUPTION, distinct de la déclaration.
	var fabricating: bool = bool(op2.get("fabricating", false))
	var cb_ready: bool = bool(op2.get("cb_ready", false))
	var cost := float(op2.get("fabricate_cost", 0.0))
	var claim_name := String(op2.get("claim_name", "territoire inconnu"))
	var fab_btn: Button = _btns.get("fabricate")
	if fab_btn != null:
		var fab_legal: Dictionary = _legal_by_verb.get("fabricate", {})
		if fabricating:
			var dleft := int(ceili(float(op2.get("fabricating_days_left", 0.0))))
			fab_btn.text = "Revendication sur %s — %d j" % [claim_name, dleft]
			fab_btn.disabled = true
		elif cb_ready:
			var yleft := float(op2.get("cb_ready_years_left", 0.0))
			fab_btn.text = "%s revendiquée — expire dans %.1f an" % [claim_name, yleft]
			fab_btn.disabled = true   # rien à refaire tant qu'elle est valide — déclarez la guerre
		else:
			fab_btn.text = "Revendiquer %s — %d or" % [claim_name, int(round(cost))]
			fab_btn.disabled = not bool(fab_legal.get("allowed", false))
			fab_btn.tooltip_text = _legal_tooltip(fab_legal, "Lance une intrigue qui produira un casus belli temporaire.")
		_update_action_detail("fabricate", fab_legal, false)
	if fabricating:
		_cb_lbl.text = "Une intrigue mûrit contre ce pays."
	elif cb_ready:
		_cb_lbl.text = "Une revendication est prête : déclarez la guerre avant qu'elle ne s'évente."
	else:
		_cb_lbl.text = ""
	# UI-MONNAIE — U3 : L'EMPRUNT D'ÉTAT. Grisé par le MÊME émissaire (cd) que les
	# autres verbes ; l'état de la DERNIÈRE demande (country_loan_status, mot résolu
	# côté moteur — « [État] accorde/refuse le prêt »/« Aucune demande ») s'affiche
	# SEULEMENT si elle concerne CE pays (country_loan_request_target == _cid).
	var loan_btn: Button = _btns.get("request_loan")
	if loan_btn != null:
		var loan_detail: Label = _action_details.get("request_loan")
		if w.has_method("player"):
			var me3 := int(w.player())
			var quote: Dictionary = w.country_loan_quote(me3, _cid) if w.has_method("country_loan_quote") else {}
			var loan_max := float(quote.get("montant_max", 0.0))
			var loan_rate := float(quote.get("taux", 0.0))
			var exposure := float(quote.get("exposure", 0.0))
			var exposure_limit := float(quote.get("exposure_limit", 0.0))
			var lender_surplus := float(quote.get("lender_surplus", 0.0))
			var other_creditor := bool(quote.get("blocked_by_other_creditor", false))
			loan_btn.disabled = cd > 0 or loan_max <= 0.5
			var reason := ""
			if cd > 0:
				reason = "Émissaire en tournée — retour dans %d j" % cd
			elif other_creditor:
				reason = "Indisponible : une autre dette étrangère doit d'abord être éteinte."
			elif loan_max <= 0.5:
				reason = "Indisponible : réserve ou limite d'exposition du prêteur atteinte."
			else:
				reason = "Demander jusqu'à %s or ; l'État conserve sa décision diplomatique." % _grp(int(round(loan_max)))
			loan_btn.tooltip_text = "%s\n• Taux fixe : %.1f %%\n• Exposition : %s / %s or\n• Surplus liquide : %s or" % [
				reason, loan_rate * 100.0, _grp(int(round(exposure))), _grp(int(round(exposure_limit))),
				_grp(int(round(lender_surplus)))]
			if loan_detail != null:
				var target := int(w.country_loan_request_target(me3)) if w.has_method("country_loan_request_target") else -1
				var status := String(w.country_loan_status(me3)) if target == _cid and w.has_method("country_loan_status") else "Aucune demande antérieure auprès de ce pays."
				loan_detail.text = "%s\nDisponible : %s or · %.1f %% fixe · exposition %s/%s" % [
					status, _grp(int(round(loan_max))), loan_rate * 100.0,
					_grp(int(round(exposure))), _grp(int(round(exposure_limit)))]
	# « Faire la paix » est un TIROIR : il reste accessible pendant la guerre même si
	# l'émissaire est occupé (le bouton d'envoi, lui, porte le cooldown). En paix il
	# est toujours visible et franchement grisé.
	var peace_btn: Button = _btns.get("peace")
	if peace_btn != null:
		peace_btn.disabled = not bool(ctx.get("at_war", false))
		peace_btn.text = ("▾ " if _peace_open else "▸ ") + "Faire la paix"
		if peace_btn.disabled:
			peace_btn.tooltip_text = "Indisponible en temps de paix"
			_peace_open = false
			_peace_box.visible = false
	_peace_refresh()
	# la hauteur SUIT le contenu visible (guerre/paix/sections repliées changent le total) —
	# recalculée au refresh pour que le panneau reste compact et la barre de scroll juste.
	_layout()

func _act(verb: String) -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var ok := false
	match verb:
		"war": ok = bool(w.player_declare_war(_cid))
		"peace": ok = bool(w.player_make_peace(_cid))
		"ally": ok = bool(w.player_offer_alliance(_cid))
		"pact": ok = bool(w.player_offer_pact(_cid))
		"migration": ok = bool(w.player_offer_migration(_cid))
		"embargo":
			var embargo_legal: Dictionary = _legal_by_verb.get("embargo", {})
			ok = bool(w.player_embargo(_cid, 1 if bool(embargo_legal.get("toggle_on", true)) else 0))
		"fabricate": ok = bool(w.player_fabricate_cb(_cid))
	var action_name := String(BTN_LABELS.get(verb, verb)).to_lower()
	_flash.text = ("Ordre émis : %s · l'émissaire part." % action_name) if ok \
		else ("Ordre refusé : %s · survolez l'action pour connaître le verrou." % action_name)
	_flash.add_theme_color_override("font_color", VKit.sense(0.80) if ok else VKit.sense(0.15))
	if ok:
		# l'ÉMISSAIRE PART : on mémorise SON objectif (display-only) pour le menu de droite,
		# tant qu'il est « en tournée » (diplo_cd). Phrase franche + le pays cible.
		var target := String(_head.text)
		var embargo_on := bool(_legal_by_verb.get("embargo", {}).get("toggle_on", true))
		var obj: String = {
			"war": "Déclarer la guerre à %s", "peace": "Proposer la paix à %s",
			"ally": "Proposer une alliance à %s", "pact": "Proposer un pacte à %s",
			"migration": "Proposer un pacte migratoire à %s",
			"embargo": ("Décréter un embargo contre %s" if embargo_on else "Lever l'embargo contre %s"),
			"fabricate": "Fabriquer une revendication contre %s",
		}.get(verb, "Émissaire dépêché auprès de %s")
		Sim.note_emissary(obj % target)
	if ok and verb == "war":
		Sound.play("moment_war_horn")
	elif not ok:
		Sound.play("ui_click")
	_refresh()

## UI-MONNAIE — U3 : L'EMPRUNT D'ÉTAT (player_request_loan, CMD_REQUEST_LOAN — motif _act,
## mais un verbe À PART : la résolution SYNCHRONE au drain n'a pas de « would_accept »
## préalable comme les autres offres diplo, motif M9 « pas d'état en cours »).
func _loan_press() -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var ok := bool(w.player_request_loan(_cid, -1.0)) if w.has_method("player_request_loan") else false   # <=0 ⇒ le maximum
	_flash.text = ("Ordre émis : demander un emprunt · l'émissaire part." if ok \
		else "Ordre refusé : demander un emprunt · survolez l'action pour connaître le verrou.")
	_flash.add_theme_color_override("font_color", VKit.sense(0.80) if ok else VKit.sense(0.15))
	if ok:
		Sim.note_emissary("Demander un emprunt à %s" % String(_head.text))
	Sound.play("ui_click")
	_refresh()

## Décision structurée fournie par le moteur. Le fallback ne sert qu'aux anciennes DLL
## de développement verrouillées : il reprend les flags de façade sans déduire la cause.
func _read_legal(w, verb: String, op2: Dictionary, cd: int) -> Dictionary:
	if w.has_method("diplo_action_legal"):
		return w.diplo_action_legal(_cid, int(DIPLO_ACTION_ID[verb]))
	var allowed := false
	match verb:
		"war": allowed = bool(op2.get("can_declare_war", false))
		"peace": allowed = bool(op2.get("can_make_peace", false))
		"ally": allowed = bool(op2.get("can_offer_alliance", false))
		"pact": allowed = bool(op2.get("can_offer_pact", false))
		"migration": allowed = bool(op2.get("can_offer_migration", false))
		"embargo": allowed = bool(op2.get("can_embargo", false)) or bool(op2.get("can_lift_embargo", false))
		"fabricate": allowed = bool(op2.get("can_fabricate", false))
	allowed = allowed and cd <= 0
	return {
		"allowed": allowed, "would_accept": true, "unilateral": verb in ["war", "embargo", "fabricate"],
		"toggle_on": not bool(op2.get("can_lift_embargo", false)),
		"reason_label": "Disponible" if allowed else ("Émissaire en tournée" if cd > 0 else "Indisponible"),
		"duration_days": cd if cd > 0 else 0, "cost_gold": float(op2.get("fabricate_cost", 0.0)) if verb == "fabricate" else 0.0,
		"gold_missing": 0.0,
	}

func _legal_tooltip(legal: Dictionary, help: String) -> String:
	# CHECKLIST DE REFUS (2026-07-21) : les conditions ✓/✗ du moteur remplacent la raison
	# figée — le joueur voit TOUT ce qu'il faut, pas juste le premier verrou. En-tête = la
	# ligne d'aide quand l'action est permise ; sinon les ✗ parlent. Repli legacy si le
	# moteur ne fournit pas de conds (DLL antérieure).
	var allowed := bool(legal.get("allowed", false))
	var conds: Array = legal.get("conds", [])
	var header := help if allowed and help != "" else ""
	var txt := TooltipFactory.gate_checklist(conds, header)
	if txt == "":
		txt = help if allowed and help != "" else String(legal.get("reason_label", "Indisponible"))
	var days := int(legal.get("duration_days", 0))
	var cost := float(legal.get("cost_gold", 0.0))
	var missing := float(legal.get("gold_missing", 0.0))
	if days > 0:
		txt += " · %d j" % days
	if cost > 0.0:
		txt += " · coût %.0f or" % cost
	if missing > 0.0:
		txt += " · manque %.0f or" % missing
	return txt

## Une action ne se comprend jamais au survol seulement : cette ligne reste affichée
## sous le verbe, disponible ou non, avec la conséquence et les nombres du moteur.
func _update_action_detail(verb: String, legal: Dictionary, amber: bool) -> void:
	var lbl: Label = _action_details.get(verb)
	if lbl == null:
		return
	var allowed := bool(legal.get("allowed", false))
	var unilateral := bool(legal.get("unilateral", true))
	var would_accept := bool(legal.get("would_accept", true))
	var state := "Disponible"
	var col := VKit.sense(0.80)
	if not allowed:
		state = "Indisponible — %s" % String(legal.get("reason_label", "verrou inconnu"))
		col = VKit.sense(0.15)
	elif amber or (not unilateral and not would_accept):
		state = "Offre légale — refus probable"
		col = Color(0.95, 0.70, 0.28)
	elif unilateral:
		state = "Disponible — décision unilatérale"
	else:
		state = "Disponible — acceptation probable"
	var facts := PackedStringArray()
	var cost := float(legal.get("cost_gold", 0.0))
	var missing := float(legal.get("gold_missing", 0.0))
	var days := int(legal.get("duration_days", 0))
	if cost > 0.0:
		facts.append("%.0f or" % cost)
	if missing > 0.0:
		facts.append("manque %.0f or" % missing)
	if days > 0:
		facts.append("%d j" % days)
	var consequence := String(ACTION_HELP.get(verb, ""))
	lbl.text = state + ((" · " + " · ".join(facts)) if not facts.is_empty() else "") \
		+ (("\n" + consequence) if consequence != "" else "")
	lbl.add_theme_color_override("font_color", col)

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
