extends Control
## CultureCreator — la fenêtre « créateur d'empire » façon Stellaris. Le joueur compose :
##   · un HÉRITAGE  (la lignée → les NOMS de son pays et de ses provinces) ;
##   · un ÉTHOS     (l'axe de valeurs → l'épithète du pays + ses factions) ;
##   · 3 TRADITIONS (une par AXE — Physique / Social / Intellectuel — FORCÉ à
##                   EXACTEMENT 1 majeur (+2) + 1 mineur (+1) + 1 défaut (−1)).
##
## RÈGLE D'OR : zéro logique de simulation ici. TOUT (listes, validation, aperçu des
## leviers, composition) passe par la façade C `Sim.world.*` (le moteur reste 100 % C,
## déterministe). Sur « Commencer », on grave la composition (set_player_culture) puis
## on régénère le monde — le pays du joueur naît avec SA culture.

signal started   ## le joueur a lancé son empire (le monde vient d'être régénéré)
signal cancelled ## le joueur a fermé sans composer (on garde le monde par défaut)
signal composed(slot: int, heritage: int, ethos: int, t0: int, t1: int, t2: int)  ## mode-slot : compo validée pour un empire

## Deux modes :
##  · AUTONOME (touche C en jeu) : « Commencer » applique la culture au JOUEUR + régénère.
##  · SLOT (écran Nouvelle partie) : « Valider » émet composed(slot,…) sans toucher le monde —
##    l'écran de setup collecte les compos de tous les empires puis lance la partie.
var _slot_mode := false
var _target_slot := 0

const AXES := ["Physique", "Social", "Intellectuel"]

# palette (cohérente avec le chrome sombre du jeu)
const C_BG     := Color(0.04, 0.04, 0.06, 0.74)   # voile plein écran
const C_PANEL  := Color(0.09, 0.085, 0.12, 0.98)
const C_EDGE   := Color(0.78, 0.55, 0.30)         # liseré cuivre
const C_TEXT   := Color(0.88, 0.86, 0.82)
const C_DIM    := Color(0.62, 0.60, 0.58)
const C_GOOD   := Color(0.46, 0.74, 0.42)
const C_BAD    := Color(0.82, 0.40, 0.34)
const C_TITLE  := Color(0.86, 0.70, 0.42)

# données de la façade
var _her: Array = []                  # héritages : [{id,nom,sphere,exemple}]
var _eth: Array = []                  # éthos     : [{id,nom,epithete,hint}]
var _axis_traits := [[], [], []]      # traditions par axe : [{id,nom,rang,hover}]

# widgets
var _her_opt: OptionButton
var _her_info: Label
var _eth_opt: OptionButton
var _eth_info: Label
var _trad_opt := [null, null, null]
var _trad_hover := [null, null, null]
var _culture_lbl: Label
var _valid_lbl: Label
var _preview_lbl: Label
var _seed_spin: SpinBox
var _start_btn: Button
var _rng := RandomNumberGenerator.new()


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP   # capte tout : modale
	_rng.randomize()
	_build_ui()
	_load_data()
	_refresh()


# ── le voile plein écran (assombrit la carte derrière la modale) ───────────────
func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


# ── CONSTRUCTION de l'interface (en code, comme les autres panneaux) ───────────
func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(640, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL
	sb.border_color = C_EDGE
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(20)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	panel.add_child(col)

	# titre
	var title := Label.new()
	title.text = "Créateur d'empire"
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", C_TITLE)
	col.add_child(title)

	var sub := Label.new()
	sub.text = "Composez votre peuple — héritage, éthos et trois traditions."
	sub.add_theme_color_override("font_color", C_DIM)
	col.add_child(sub)

	col.add_child(_sep())

	# ── HÉRITAGE ──
	col.add_child(_section("Héritage  (votre lignée — donne ses noms à votre empire)"))
	_her_opt = OptionButton.new()
	_her_opt.item_selected.connect(func(_i): _refresh())
	col.add_child(_her_opt)
	_her_info = _hint_label()
	col.add_child(_her_info)

	# ── ÉTHOS ──
	col.add_child(_sep())
	col.add_child(_section("Éthos  (l'âme de l'État — son épithète et ses factions)"))
	_eth_opt = OptionButton.new()
	_eth_opt.item_selected.connect(func(_i): _refresh())
	col.add_child(_eth_opt)
	_eth_info = _hint_label()
	col.add_child(_eth_info)

	# ── TRADITIONS (une par axe) ──
	col.add_child(_sep())
	col.add_child(_section("Traditions  (une par axe : 1 majeur +2 · 1 mineur +1 · 1 défaut −1)"))
	for ax in range(3):
		var row := VBoxContainer.new()
		row.add_theme_constant_override("separation", 2)
		var lab := Label.new()
		lab.text = AXES[ax]
		lab.add_theme_color_override("font_color", C_TEXT)
		row.add_child(lab)
		var opt := OptionButton.new()
		opt.item_selected.connect(func(_i): _refresh())
		row.add_child(opt)
		_trad_opt[ax] = opt
		var hv := _hint_label()
		row.add_child(hv)
		_trad_hover[ax] = hv
		col.add_child(row)

	# ── APERÇU live ──
	col.add_child(_sep())
	_culture_lbl = Label.new()
	_culture_lbl.add_theme_color_override("font_color", C_TITLE)
	_culture_lbl.add_theme_font_size_override("font_size", 16)
	col.add_child(_culture_lbl)

	_preview_lbl = Label.new()
	_preview_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_preview_lbl.custom_minimum_size = Vector2(600, 0)
	_preview_lbl.add_theme_color_override("font_color", C_DIM)
	col.add_child(_preview_lbl)

	_valid_lbl = Label.new()
	col.add_child(_valid_lbl)

	# ── pied : graine + actions ──
	col.add_child(_sep())
	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	col.add_child(foot)

	var seed_lab := Label.new()
	seed_lab.text = "Graine du monde"
	seed_lab.add_theme_color_override("font_color", C_TEXT)
	foot.add_child(seed_lab)

	_seed_spin = SpinBox.new()
	_seed_spin.min_value = 0
	_seed_spin.max_value = 999999
	_seed_spin.value = 9
	foot.add_child(_seed_spin)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(spacer)

	var rnd_btn := Button.new()
	rnd_btn.text = "Aléatoire"
	rnd_btn.pressed.connect(_on_randomize)
	foot.add_child(rnd_btn)

	var cancel_btn := Button.new()
	cancel_btn.text = "Passer"
	cancel_btn.pressed.connect(_on_cancel)
	foot.add_child(cancel_btn)

	_start_btn = Button.new()
	_start_btn.text = "Commencer l'empire"
	_start_btn.pressed.connect(_on_start)
	foot.add_child(_start_btn)


func _sep() -> HSeparator:
	var s := HSeparator.new()
	return s

func _section(txt: String) -> Label:
	var l := Label.new()
	l.text = txt
	l.add_theme_color_override("font_color", C_EDGE)
	return l

func _hint_label() -> Label:
	var l := Label.new()
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.custom_minimum_size = Vector2(600, 0)
	l.add_theme_color_override("font_color", C_DIM)
	l.add_theme_font_size_override("font_size", 12)
	return l


# ── DONNÉES : tout vient de la façade C ────────────────────────────────────────
func _load_data() -> void:
	if Sim.world == null:
		_valid_lbl.text = "Moteur (libscps) absent — bâtir : cd godot && scons."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
		_start_btn.disabled = true
		return
	if not Sim.world.has_method("heritage_list"):
		_valid_lbl.text = "libscps obsolète (créateur absent) — recompiler : cd godot && scons."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
		_start_btn.disabled = true
		return

	_her = Sim.world.heritage_list()
	for h in _her:
		_her_opt.add_item(String(h["nom"]))
		_her_opt.set_item_metadata(_her_opt.item_count - 1, int(h["id"]))

	_eth = Sim.world.ethos_list()
	for e in _eth:
		_eth_opt.add_item(String(e["nom"]))
		_eth_opt.set_item_metadata(_eth_opt.item_count - 1, int(e["id"]))

	_axis_traits = [[], [], []]
	for t in Sim.world.tradition_list():
		var ax := int(t["axe"])
		if ax >= 0 and ax < 3:
			_axis_traits[ax].append(t)
	for ax in range(3):
		var opt: OptionButton = _trad_opt[ax]
		for t in _axis_traits[ax]:
			opt.add_item("%s  (%s)" % [String(t["nom"]), _rang_str(int(t["rang"]))])
			opt.set_item_metadata(opt.item_count - 1, int(t["id"]))

	# défaut sensé : une compo VALIDE d'entrée (majeur Phys / mineur Soc / défaut Int)
	_preset_default()


func _rang_str(r: int) -> String:
	if r >= 2: return "+2 majeur"
	elif r == 1: return "+1 mineur"
	elif r < 0: return "−1 défaut"
	return "0"


# choisit, par axe, le 1er trait du rang voulu (rôle[ax] : 0 majeur · 1 mineur · 2 défaut)
func _select_roles(role: Array) -> void:
	for ax in range(3):
		var want_rank := 2 if role[ax] == 0 else (1 if role[ax] == 1 else -1)
		var opt: OptionButton = _trad_opt[ax]
		for i in range(opt.item_count):
			var id := int(opt.get_item_metadata(i))
			if _trait_rank(id) == want_rank or (want_rank == 2 and _trait_rank(id) >= 2):
				opt.select(i)
				break

func _preset_default() -> void:
	_her_opt.select(0)
	_eth_opt.select(0)
	_select_roles([0, 1, 2])   # Phys majeur · Soc mineur · Int défaut


func _trait_rank(id: int) -> int:
	for ax in range(3):
		for t in _axis_traits[ax]:
			if int(t["id"]) == id:
				return int(t["rang"])
	return 0

func _trait_hover(id: int) -> String:
	for ax in range(3):
		for t in _axis_traits[ax]:
			if int(t["id"]) == id:
				return String(t["hover"])
	return ""


func _cur_heritage() -> int:
	return int(_her_opt.get_item_metadata(_her_opt.selected)) if _her_opt.selected >= 0 else 0

func _cur_ethos() -> int:
	return int(_eth_opt.get_item_metadata(_eth_opt.selected)) if _eth_opt.selected >= 0 else 0

func _cur_trait(ax: int) -> int:
	var opt: OptionButton = _trad_opt[ax]
	return int(opt.get_item_metadata(opt.selected)) if opt.selected >= 0 else -1


# ── RAFRAÎCHIT l'aperçu (nom de culture, hovers, leviers, validité) ────────────
func _refresh() -> void:
	# garde : pas de monde, OU libscps obsolète/incomplet (listes non peuplées par
	# _load_data) → ne PAS appeler les méthodes du créateur (elles n'existent pas sur
	# un vieux binding). Couvre l'appel de _ready() et de _on_randomize().
	if Sim.world == null or _her_opt == null or _her_opt.item_count == 0:
		return
	var her := _cur_heritage()
	var eth := _cur_ethos()
	var seed := int(_seed_spin.value)

	# héritage : sphère + ethnonyme-exemple (live, via la façade)
	for h in _her:
		if int(h["id"]) == her:
			_her_info.text = "Sphère %s · ex. « %s »" % [String(h["sphere"]), Sim.world.culture_name(her, seed)]
			break
	# éthos : la ligne d'ambiance
	for e in _eth:
		if int(e["id"]) == eth:
			_eth_info.text = "« %s … » — %s" % [String(e["epithete"]), String(e["hint"])]
			break
	# traditions : le survol du trait choisi sous chaque axe
	for ax in range(3):
		_trad_hover[ax].text = _trait_hover(_cur_trait(ax))

	# nom de culture (le PEUPLE)
	_culture_lbl.text = "Votre peuple : les %s" % Sim.world.culture_name(her, seed)

	# aperçu des leviers (membrane : mots + signe)
	var t0 := _cur_trait(0)
	var t1 := _cur_trait(1)
	var t2 := _cur_trait(2)
	var parts := PackedStringArray()
	for lv in Sim.world.culture_preview(t0, t1, t2):
		# CHIFFRE (plus la flèche seule) : relatif → « +15 % » · absolu → « +1.5 »
		var val := float(lv.get("value", 0.0))
		var num := ""
		if int(lv.get("is_pct", 0)) != 0:
			num = "%+d %%" % int(round(val * 100.0))
		else:
			num = "%+.1f" % val
		parts.append("%s %s" % [String(lv["nom"]), num])
	_preview_lbl.text = ("Effets : " + ", ".join(parts)) if parts.size() > 0 else "Effets : —"

	# validité (la façade fait foi) + message d'aide
	var ok: bool = Sim.world.culture_validate(t0, t1, t2)
	_start_btn.disabled = not ok
	if ok:
		_valid_lbl.text = "✓ Composition valide."
		_valid_lbl.add_theme_color_override("font_color", C_GOOD)
	else:
		_valid_lbl.text = "✗ Il faut EXACTEMENT 1 majeur (+2), 1 mineur (+1) et 1 défaut (−1)."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)


# ── ACTIONS ────────────────────────────────────────────────────────────────────
func _on_randomize() -> void:
	if Sim.world == null:
		return
	_her_opt.select(_rng.randi_range(0, max(0, _her_opt.item_count - 1)))
	_eth_opt.select(_rng.randi_range(0, max(0, _eth_opt.item_count - 1)))
	# permutation aléatoire des rôles {majeur, mineur, défaut} sur les 3 axes
	var roles := [0, 1, 2]
	for i in range(roles.size() - 1, 0, -1):
		var j := _rng.randi_range(0, i)
		var tmp = roles[i]; roles[i] = roles[j]; roles[j] = tmp
	# dans chaque axe, un trait ALÉATOIRE du rang imposé
	for ax in range(3):
		var want_rank := 2 if roles[ax] == 0 else (1 if roles[ax] == 1 else -1)
		var opt: OptionButton = _trad_opt[ax]
		var cands := []
		for i in range(opt.item_count):
			var id := int(opt.get_item_metadata(i))
			var rk := _trait_rank(id)
			if rk == want_rank or (want_rank == 2 and rk >= 2):
				cands.append(i)
		if cands.size() > 0:
			opt.select(cands[_rng.randi_range(0, cands.size() - 1)])
	_refresh()


func _on_start() -> void:
	if Sim.world == null:
		return
	var her := _cur_heritage()
	var eth := _cur_ethos()
	var t0 := _cur_trait(0)
	var t1 := _cur_trait(1)
	var t2 := _cur_trait(2)
	if not Sim.world.culture_validate(t0, t1, t2):
		_refresh()   # invalide : le bouton n'aurait pas dû être actif
		return
	if _slot_mode:
		# écran de setup : on rend la compo, sans toucher le monde (lancé à « Lancer »).
		composed.emit(_target_slot, her, eth, t0, t1, t2)
		hide()
		return
	# mode autonome : applique au joueur + régénère immédiatement
	if not Sim.world.set_player_culture(her, eth, t0, t1, t2):
		_refresh()
		return
	Sim.regenerate(int(_seed_spin.value))   # le monde renaît AVEC la culture du joueur
	hide()
	started.emit()

## ouvre le créateur en mode SLOT (composer la culture de l'empire `slot`).
func open_for_slot(slot: int) -> void:
	_slot_mode = true
	_target_slot = slot
	if _start_btn != null:
		_start_btn.text = "Valider la culture"
	if _seed_spin != null:
		_seed_spin.editable = false   # la graine est gérée par l'écran de setup
	open()


func _on_cancel() -> void:
	if Sim.world != null:
		Sim.world.clear_player_culture()   # pas de composition : retour au défaut (IA aléatoire)
	hide()
	cancelled.emit()


## ouvre la fenêtre (réinitialise une composition valide par défaut)
func open() -> void:
	show()
	if Sim.world != null and _her_opt.item_count > 0:
		_refresh()
	queue_redraw()
