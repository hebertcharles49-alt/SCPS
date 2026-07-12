extends Control
## NewGamePanel — l'écran « Nouvelle partie » (retour joueur 2026-07-10 : UN SEUL réglage
## monde pour l'instant — la TAILLE en slider Tiny→Huge ; le reste du monde vient de
## l'ARCHÉTYPE de la graine, et la graine se tire au DÉ) + liste des EMPIRES (chacun
## composable façon Stellaris : slot 0 = Vous, 1..N = IA) + « Lancer la partie ».
##
## RÈGLE D'OR : zéro logique de sim. On LIT/ÉCRIT la façade C (Sim.world.*) : worldgen_set
## (défauts d'archétype worldparams_default(graine) + la taille choisie — passer un dict
## PARTIEL écraserait l'archétype par des constantes), set_empire_culture (les compos),
## puis Sim.regenerate. Le monde naît en PAUSE.

signal launched   ## la partie est lancée (le monde régénéré, en pause an 0)
signal back       ## retour au menu principal

const Creator = preload("res://ui/culture_creator.gd")

# palette
const C_BG    := Color(0.04, 0.03, 0.02, 0.92)
const C_PANEL := Color(0.10, 0.08, 0.055, 0.98)   # cuir sombre
const C_EDGE  := Color(0.79, 0.64, 0.29)          # or vieilli
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.62, 0.60, 0.58)
const C_TITLE := Color(0.86, 0.70, 0.42)

# presets de TAILLE : clé tr() (i18n/ui.csv) → [n_empires, n_city_states]
const SIZES := [
	["T_SIZE_0", 2, 4],
	["T_SIZE_1", 4, 8],
	["T_SIZE_2", 6, 12],
	["T_SIZE_3", 8, 16],
	["T_SIZE_4", 10, 20],
	["T_SIZE_5", 12, 24],
]
# clés tr() — les noms rendus suivent la langue choisie aux Options
const HERITAGE_KEYS := ["T_HERITAGE_0", "T_HERITAGE_1", "T_HERITAGE_2", "T_HERITAGE_3", "T_HERITAGE_4", "T_HERITAGE_5"]
const ETHOS_KEYS := ["T_ETHOS_0", "T_ETHOS_1", "T_ETHOS_2", "T_ETHOS_3", "T_ETHOS_4", "T_ETHOS_5"]

var _size_sld: HSlider      # TAILLE Tiny→Huge — le seul réglage monde exposé pour l'instant
var _size_val: Label
var _size_explain: Label   ## « → N empires · M cités-états » — la taille en termes concrets
var _preview_lbl: Label    ## aperçu compact du monde (continents/terres/climat/âge, façade worldparams_default)
var _seed_edit: LineEdit
var _rng := RandomNumberGenerator.new()
var _empire_box: VBoxContainer
var _empire_rows := []      # [{summary:Label, btn:Button}]
var _observer_chk: CheckButton   ## « Observateur » : lancer sans jouer d'empire (tout IA)
var _compos := {}           # slot:int → {heritage,ethos,t0,t1,t2}
var _creator: Control = null


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_rng.randomize()
	_build_ui()
	_rebuild_empire_list()

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(880, 600)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(2)
	sb.set_corner_radius_all(6); sb.set_content_margin_all(18)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	panel.add_child(root)

	var title := Label.new()
	title.text = tr("T_NG_TITLE")
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", C_TITLE)
	root.add_child(title)

	# deux colonnes : MONDE | EMPIRES
	var cols := HBoxContainer.new()
	cols.add_theme_constant_override("separation", 24)
	cols.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(cols)

	# ── colonne MONDE ──
	var world := VBoxContainer.new()
	world.add_theme_constant_override("separation", 6)
	world.custom_minimum_size = Vector2(400, 0)
	cols.add_child(world)
	world.add_child(_section(tr("T_NG_WORLD")))

	# TAILLE : un SLIDER Tiny→Huge (retour joueur : « réglage autorisé pour l'instant :
	# taille du monde ») — les autres traits du monde viennent de l'ARCHÉTYPE de la graine.
	var size_row := HBoxContainer.new()
	var size_lab := Label.new(); size_lab.text = tr("T_NG_SIZE"); size_lab.custom_minimum_size = Vector2(150, 0)
	size_lab.add_theme_color_override("font_color", C_TEXT)
	size_row.add_child(size_lab)
	_size_sld = HSlider.new()
	_size_sld.min_value = 0; _size_sld.max_value = SIZES.size() - 1; _size_sld.step = 1
	_size_sld.value = 2   # Normal
	_size_sld.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_size_sld.custom_minimum_size = Vector2(180, 0)
	size_row.add_child(_size_sld)
	_size_val = Label.new(); _size_val.custom_minimum_size = Vector2(96, 0)
	_size_val.add_theme_color_override("font_color", C_DIM)
	size_row.add_child(_size_val)
	_size_sld.value_changed.connect(func(_v):
		_size_val.text = tr(String(_cur_size()[0]))
		_refresh_size_explain()
		_rebuild_empire_list())
	world.add_child(size_row)

	# EXPLIQUER LA TAILLE en termes concrets (retour joueur 2026-07-10, Lot 4.4 :
	# « aperçu compact du monde + expliquer la taille ») — les décomptes empires/
	# cités-états sont RÉELS (SIZES, exactement ce que worldgen_set va poser) ; le
	# nombre de RÉGIONS en dépend aussi mais varie avec la graine/l'archétype —
	# on ne fabrique pas un chiffre non lisible sans générer, on le dit en clair.
	_size_explain = Label.new()
	_size_explain.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_size_explain.add_theme_color_override("font_color", C_TEXT)
	_size_explain.add_theme_font_size_override("font_size", 12)
	world.add_child(_size_explain)

	var hint := Label.new()
	hint.text = tr("T_NG_ARCH_HINT") if tr("T_NG_ARCH_HINT") != "T_NG_ARCH_HINT" \
		else "Le relief, le climat et les continents suivent le caractère de la graine."
	hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hint.add_theme_color_override("font_color", C_DIM)
	hint.add_theme_font_size_override("font_size", 12)
	world.add_child(hint)

	# GRAINE : champ + DÉ (retour joueur : « un randomiseur plutôt qu'un up/down »)
	var seed_row := HBoxContainer.new()
	var seed_lab := Label.new(); seed_lab.text = tr("T_NG_SEED"); seed_lab.custom_minimum_size = Vector2(150, 0)
	seed_lab.add_theme_color_override("font_color", C_TEXT)
	seed_row.add_child(seed_lab)
	_seed_edit = LineEdit.new()
	_seed_edit.text = "9"
	_seed_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_seed_edit.text_changed.connect(func(_t): _refresh_world_preview())
	seed_row.add_child(_seed_edit)
	var dice := Button.new()
	dice.text = "🎲"
	dice.tooltip_text = "Tirer une graine au hasard"
	dice.pressed.connect(func():
		_seed_edit.text = str(_rng.randi_range(0, 999999))
		_refresh_world_preview())   # .text= ne déclenche pas text_changed en direct
	seed_row.add_child(dice)
	world.add_child(seed_row)
	_size_val.text = tr(String(_cur_size()[0]))

	# ── APERÇU COMPACT DU MONDE : lit worldparams_default(graine) — l'archétype
	# réel qui sera généré (continents/terres/relief/climat/âge), en MOTS (jamais
	# les flottants bruts de la façade) ; se rafraîchit avec la graine. Zéro logique
	# de sim : lecture pure d'une fonction façade déterministe, aucune écriture.
	world.add_child(HSeparator.new())
	world.add_child(_section("Aperçu du monde"))
	_preview_lbl = Label.new()
	_preview_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_preview_lbl.add_theme_color_override("font_color", C_DIM)
	_preview_lbl.add_theme_font_size_override("font_size", 12)
	world.add_child(_preview_lbl)
	_refresh_size_explain()
	_refresh_world_preview()

	# ── colonne EMPIRES ──
	var emp := VBoxContainer.new()
	emp.add_theme_constant_override("separation", 6)
	emp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cols.add_child(emp)
	emp.add_child(_section(tr("T_NG_EMPIRES")))
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.custom_minimum_size = Vector2(380, 380)
	emp.add_child(scroll)
	_empire_box = VBoxContainer.new()
	_empire_box.add_theme_constant_override("separation", 4)
	_empire_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(_empire_box)

	# ── pied : actions ──
	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	root.add_child(foot)
	var back_btn := Button.new(); back_btn.text = tr("T_BACK")
	back_btn.pressed.connect(func(): back.emit())
	foot.add_child(back_btn)
	var spacer := Control.new(); spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(spacer)
	# MODE OBSERVATEUR : regarder le monde tourner sans jouer d'empire (tout IA).
	_observer_chk = CheckButton.new()
	_observer_chk.text = "Observateur"
	_observer_chk.tooltip_text = "Regardez le monde évoluer sans jouer d'empire — tout est piloté par l'IA."
	foot.add_child(_observer_chk)
	var go := Button.new(); go.text = tr("T_NG_LAUNCH")
	go.add_theme_font_size_override("font_size", 16)
	go.pressed.connect(_on_lancer)
	foot.add_child(go)


func _section(txt: String) -> Label:
	var l := Label.new(); l.text = txt
	l.add_theme_color_override("font_color", C_EDGE)
	return l

func _cur_seed() -> int:
	return maxi(0, int(_seed_edit.text)) if _seed_edit != null else 9

func _cur_size() -> Array:
	var i: int = clampi(int(_size_sld.value), 0, SIZES.size() - 1)
	return SIZES[i]

## « Petit — ~2 empires · ~98 régions » (retour joueur 2026-07-10) : les décomptes
## d'empires/cités sont EXACTS (SIZES, ce que worldgen_set posera) ; le nombre de
## régions varie avec l'archétype de la graine — on ne l'invente pas, on le dit.
func _refresh_size_explain() -> void:
	if _size_explain == null:
		return
	var sz := _cur_size()
	_size_explain.text = "→ %d empires · %d cités-états. (Le nombre de régions suit : plus d'empires colonisent davantage de terres.)" % [int(sz[1]), int(sz[2])]

## mot de 3 paliers pour une valeur normalisée [0..1] — jamais le flottant brut.
func _bucket_word(v: float, lo: String, mid: String, hi: String) -> String:
	return lo if v < 0.34 else (hi if v > 0.66 else mid)

## aperçu compact du monde : lit worldparams_default(graine) — l'archétype réel de
## CETTE graine (continents/terres/relief/climat/âge), traduit en mots. N'affecte
## pas la genèse ; se rafraîchit à chaque changement de graine.
func _refresh_world_preview() -> void:
	if _preview_lbl == null:
		return
	if Sim.world == null or not Sim.world.has_method("worldparams_default"):
		_preview_lbl.text = ""
		return
	var p: Dictionary = Sim.world.worldparams_default(_cur_seed())
	var land := _bucket_word(float(p.get("land_amount", 0.5)), "îles éparses", "terres équilibrées", "grandes étendues")
	var mount := _bucket_word(float(p.get("mountains", 0.5)), "plaines", "relief vallonné", "hautes montagnes")
	var temp := _bucket_word(float(p.get("temperature", 0.5)), "froid", "tempéré", "chaud")
	var humid := _bucket_word(float(p.get("humidity", 0.5)), "aride", "modéré", "humide")
	var age := _bucket_word(float(p.get("world_age", 0.5)), "jeune", "mûr", "vieux et fendu")
	var n_cont := int(p.get("n_continents", 0))
	_preview_lbl.text = "%s : %d\n%s : %s · %s : %s\nClimat : %s, %s\n%s : %s" % [
		tr("T_NG_CONTINENTS"), n_cont,
		tr("T_NG_LAND"), land, tr("T_NG_MOUNTAINS"), mount,
		temp, humid,
		tr("T_NG_AGE"), age]

func _rebuild_empire_list() -> void:
	for c in _empire_box.get_children():
		c.queue_free()
	_empire_rows.clear()
	var n_emp: int = int(_cur_size()[1])
	for slot in range(n_emp):
		var row := PanelContainer.new()
		var rb := StyleBoxFlat.new()
		rb.bg_color = Color(0.16, 0.13, 0.09, 0.9); rb.set_corner_radius_all(4); rb.set_content_margin_all(6)
		row.add_theme_stylebox_override("panel", rb)
		var hb := HBoxContainer.new()
		row.add_child(hb)
		var name_lab := Label.new()
		name_lab.text = (tr("T_NG_EMPIRE_YOU") % (slot + 1)) if slot == 0 else (tr("T_NG_EMPIRE_AI") % (slot + 1))
		name_lab.add_theme_color_override("font_color", C_TITLE if slot == 0 else C_TEXT)
		name_lab.custom_minimum_size = Vector2(120, 0)
		hb.add_child(name_lab)
		var summary := Label.new()
		summary.add_theme_color_override("font_color", C_DIM)
		summary.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		hb.add_child(summary)
		var btn := Button.new(); btn.text = tr("T_NG_COMPOSE")
		var s := slot
		btn.pressed.connect(func(): _on_compose(s))
		hb.add_child(btn)
		_empire_box.add_child(row)
		_empire_rows.append({"summary": summary, "btn": btn})
		_refresh_row(slot)

func _refresh_row(slot: int) -> void:
	if slot < 0 or slot >= _empire_rows.size():
		return
	var summary: Label = _empire_rows[slot]["summary"]
	if _compos.has(slot):
		var c = _compos[slot]
		var h: int = int(c["heritage"]); var e: int = int(c["ethos"])
		summary.text = "%s · %s" % [tr(HERITAGE_KEYS[h]) if h < 6 else "?", tr(ETHOS_KEYS[e]) if e < 6 else "?"]
		summary.add_theme_color_override("font_color", C_TEXT)
	else:
		summary.text = tr("T_NG_RANDOM")
		summary.add_theme_color_override("font_color", C_DIM)


func _ensure_creator() -> void:
	if _creator != null:
		return
	_creator = Creator.new()
	_creator.name = "SlotCreator"
	add_child(_creator)
	_creator.hide()
	_creator.composed.connect(_on_composed)

func _on_compose(slot: int) -> void:
	if Sim.world == null:
		return
	_ensure_creator()
	_creator.open_for_slot(slot)

func _on_composed(slot: int, heritage: int, ethos: int, t0: int, t1: int, t2: int) -> void:
	_compos[slot] = {"heritage": heritage, "ethos": ethos, "t0": t0, "t1": t1, "t2": t2,
		"name": String(_creator.custom_name()) if _creator != null else ""}
	_refresh_row(slot)


## les params de genèse : l'ARCHÉTYPE de la graine (worldparams_default) + la TAILLE
## choisie. ⚠ worldgen_set remplit les clés ABSENTES par des constantes fixes — passer
## un dict partiel écraserait l'archétype (les cartes redeviendraient toutes pareilles).
func _gather_params(seed_v: int) -> Dictionary:
	var sz := _cur_size()
	var d := {}
	if Sim.world != null and Sim.world.has_method("worldparams_default"):
		d = Sim.world.worldparams_default(seed_v)
	d["n_empires"] = int(sz[1])
	d["n_city_states"] = int(sz[2])
	return d

func _on_lancer() -> void:
	if Sim.world == null:
		return
	var seed_v := _cur_seed()
	Sim.world.clear_player_culture()              # repart d'une ardoise propre (tous slots)
	Sim.world.worldgen_set(_gather_params(seed_v))  # archétype de graine + taille choisie
	for slot in _compos.keys():
		var c = _compos[slot]
		Sim.world.set_empire_culture(int(slot), int(c["heritage"]), int(c["ethos"]),
			int(c["t0"]), int(c["t1"]), int(c["t2"]))
	Sim.regenerate(seed_v)                        # == chargement == → monde neuf
	# MODE OBSERVATEUR : retirer la main humaine APRÈS la genèse (qui rétablit human_player).
	if _observer_chk != null and _observer_chk.button_pressed and Sim.world.has_method("set_observer"):
		Sim.world.set_observer(true)
	# le NOM personnalisé du joueur (slot 0) s'applique APRÈS la genèse (elle nomme d'office)
	var nm := String(_compos.get(0, {}).get("name", ""))
	if nm != "" and Sim.world.has_method("set_country_name"):
		Sim.world.set_country_name(int(Sim.world.player()), nm)
	Sim.set_speed(0)                              # Pause — année 0
	hide()
	launched.emit()
