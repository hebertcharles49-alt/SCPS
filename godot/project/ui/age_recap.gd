extends Control
## AgeRecap — L'ÉCRAN DE CHAPITRE : quand un âge s'est levé et que le joueur clique le
## chip « Engager », on ouvre CE récap modal (monde en PAUSE, comme event_dialog) au lieu
## d'émettre directement le verbe : titre, la TRANCHE d'annales de l'âge écoulé (filtrée
## par année côté GDScript — l'année du dernier engage est mémorisée ICI, état d'UI),
## un bilan en nombres tangibles (country_info), puis « Engager l'âge suivant » (émet
## scps_player_age_engage — verbe journalisé, drainé au tick) ou « Plus tard » (referme,
## le chip reste). RÈGLE D'OR : zéro logique de sim — on LIT la façade, on ENFILE un verbe.
##
## LA PAGE QUI SE TOURNE (V1b) : l'ouverture ne pose plus le modal cash — la page (PageTurn,
## CanvasLayer horloge-mur) MONTE d'abord depuis le bas et couvre l'écran ; le contenu (ce
## Control, son voile+panneau) ne s'affiche QU'UNE FOIS la page montée (signal `risen`). Au
## clic « Engager » : le verbe s'émet PUIS la page TOURNE (révèle le monde muté). « Plus
## tard » : la page redescend, rien n'est engagé. Repli : si le shader ne charge pas,
## PageTurn.rise()/turn() sont des no-op immédiats → comportement modal IDENTIQUE à avant.

const VKit = preload("res://ui/vkit.gd")
const Epithet = preload("res://ui/epithet.gd")
const PageTurn = preload("res://ui/page_turn.gd")

signal goto_region(region: int)   ## clic sur une annale localisée → main centre la carte

var _panel: PanelContainer
var _title: Label
var _epithet_line: Label
var _bilan: Label
var _list: VBoxContainer
var _empty: Label
var _prev_speed := -1             ## vitesse d'avant l'ouverture (restaurée à la fermeture)
var _last_engage_year := -1000000 ## an du dernier engage (état d'UI ; la 1re tranche = tout)
var _prev_age_name := ""          ## nom de l'âge PRÉCÉDEMMENT engagé ("" = l'aube du règne)
var _page: CanvasLayer            ## la page qui se tourne (nœud frère, ajouté par main.gd)

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false
	Sim.generated.connect(func():
		_last_engage_year = -1000000
		_prev_age_name = ""
		_prev_speed = -1           # nouvelle partie : ne pas restaurer une vitesse d'avant
		visible = false)
	# la vitesse d'avant est restaurée QUAND l'écran se cache — quel que soit le chemin
	# (boutons, Échap via _close_topmost) : un seul point de sortie.
	visibility_changed.connect(func():
		if not visible and _prev_speed >= 0:
			Sim.set_speed(_prev_speed)
			_prev_speed = -1)

	# voile semi-opaque plein viewport (le monde attend derrière)
	var veil := ColorRect.new()
	veil.color = Color(0.03, 0.02, 0.01, 0.62)
	veil.set_anchors_preset(Control.PRESET_FULL_RECT)
	veil.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(veil)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	_panel = PanelContainer.new()
	_panel.custom_minimum_size = Vector2(560, 560)
	var sb := StyleBoxFlat.new()
	sb.bg_color = VKit.COL_PANEL
	sb.border_color = VKit.COL_EDGE
	sb.set_border_width_all(1)
	sb.set_border_width(SIDE_TOP, 3)
	sb.set_corner_radius_all(1)
	sb.set_content_margin_all(14)
	_panel.add_theme_stylebox_override("panel", sb)
	center.add_child(_panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	_panel.add_child(col)

	_title = Label.new()
	_title.add_theme_font_size_override("font_size", 22)
	_title.add_theme_color_override("font_color", VKit.COL_GOLD)
	col.add_child(_title)

	_epithet_line = Label.new()
	_epithet_line.add_theme_color_override("font_color", VKit.COL_PARCH)
	col.add_child(_epithet_line)

	_bilan = Label.new()
	_bilan.add_theme_color_override("font_color", VKit.COL_DIM)
	col.add_child(_bilan)
	col.add_child(HSeparator.new())

	var sc := ScrollContainer.new()
	sc.custom_minimum_size = Vector2(530, 340)
	sc.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(sc)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 4)
	sc.add_child(_list)
	_empty = Label.new()
	_empty.add_theme_color_override("font_color", VKit.COL_DIM)
	_empty.visible = false
	col.add_child(_empty)

	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	col.add_child(foot)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	var later := Button.new(); later.text = "Plus tard"
	later.pressed.connect(_later)     # le chip RESTE (rien d'engagé)
	foot.add_child(later)
	var engage := Button.new(); engage.text = "Engager l'âge suivant"
	engage.pressed.connect(_engage)
	foot.add_child(engage)

## câble le nœud PageTurn (ajouté par main.gd comme frère, layer au-dessus de tout).
func set_page(page: CanvasLayer) -> void:
	_page = page

## OUVRE le récap (clic du chip d'âge). Lit la façade, remplit, pause le monde, PUIS la
## page MONTE (le contenu n'apparaît qu'une fois montée — cf. `risen`).
func open() -> void:
	var w = Sim.world
	if w == null or not w.has_method("age_state"):
		return
	var ag: Dictionary = w.age_state()
	if int(ag.get("age", -1)) < 0 or bool(ag.get("engaged", false)):
		return   # rien à engager : le chip aurait déjà disparu
	_build(w, String(ag.get("name", "")))
	if not visible:
		_prev_speed = Sim.speed_index
		Sim.set_speed(0)              # le chapitre mérite le regard : le monde attend
	if _page != null and _page.has_method("rise"):
		visible = false                # caché tant que la page n'est pas montée
		var bilan_txt: String = _title.text + "\n\n" + _epithet_line.text
		if not _page.risen.is_connected(_on_page_risen):
			_page.risen.connect(_on_page_risen, CONNECT_ONE_SHOT)
		_page.rise(bilan_txt)
	else:
		visible = true                 # pas de page (repli) : modal cash, comme avant

func _on_page_risen() -> void:
	visible = true

## remplit titre + épithète + bilan + tranche d'annales de l'âge écoulé.
func _build(w, next_age_name: String) -> void:
	var closing := _prev_age_name if _prev_age_name != "" else "l'aube du règne"
	if _prev_age_name != "":
		_title.text = "L'âge de %s s'achève" % _prev_age_name
	else:
		_title.text = "L'aube du règne s'achève"
	var entries: Array = w.annals() if w.has_method("annals") else []
	var tranche: Array = Epithet.slice_since(entries, _last_engage_year)
	var ep: String = Epithet.derive(tranche)
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var nom := String(ci.get("nom", "—"))
	_epithet_line.text = "Ainsi passa %s pour %s, dit %s. L'âge de %s se lève." \
		% [closing, nom, ep, next_age_name]
	_bilan.text = "Bilan : %s âmes · %s or · %d province(s) · savoir %d" % [
		_fmt(int(ci.get("pop", 0))), _fmt(int(ci.get("or", 0))),
		int(ci.get("regions", 0)), int(ci.get("savoir", 0))]
	for c in _list.get_children():
		c.queue_free()
	_empty.visible = tranche.is_empty()
	if tranche.is_empty():
		_empty.text = "Cet âge n'a rien laissé aux Annales. Les chroniqueurs ont dormi."
	for e in tranche:
		var row := Button.new()
		row.flat = true
		row.alignment = HORIZONTAL_ALIGNMENT_LEFT
		row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		row.text = String(e.get("ligne", ""))
		var region := int(e.get("region", -1))
		row.disabled = region < 0
		row.focus_mode = Control.FOCUS_NONE
		if region >= 0:
			row.pressed.connect(func(): goto_region.emit(region))
		_list.add_child(row)
	# le nom du prochain âge devient « l'âge précédent » au moment de l'ENGAGE seulement
	set_meta("next_age_name", next_age_name)

## « Plus tard » : la page redescend (aucun verbe émis, le chip reste).
func _later() -> void:
	visible = false
	if _page != null and _page.has_method("lower"):
		_page.lower()

func _engage() -> void:
	var w = Sim.world
	if w != null and w.has_method("player_age_engage") and w.player_age_engage():
		_last_engage_year = int(w.year())              # la prochaine tranche part d'ici
		_prev_age_name = String(get_meta("next_age_name", ""))
	visible = false
	# le verbe est ENFILÉ (drainé au tick suivant) ; la page TOURNE et révèle le monde MUTÉ —
	# elle couvre le raccord entre l'ancien âge affiché et le nouveau tick.
	if _page != null and _page.has_method("turn"):
		_page.turn()

## 12345678 → « 12 345 678 » (lisibilité des nombres tangibles)
static func _fmt(v: int) -> String:
	var s := str(absi(v))
	var out := ""
	var n := s.length()
	for i in range(n):
		out += s[i]
		var left := n - 1 - i
		if left > 0 and left % 3 == 0:
			out += " "
	return ("-" + out) if v < 0 else out
