extends Control
## Chronique — LES ANNALES DU RÈGNE : le récit SÉLECTIF de la partie, en lecture seule
## (touche H). Une frise déroulante an+ligne (moteur : scps_events.c annals[], façade :
## scps_annals) ; le clic sur une entrée localisée (region>=0) centre la carte dessus.
## RÈGLE D'OR : zéro logique de sim — on LIT annals() et on émet un signal de navigation ;
## rien n'est muté ici (pas de verbe, pas d'écriture moteur).
##
## Lot 4.4 (retour joueur 2026-07-10 : « hauteur adaptée ou entrées enrichies ») : les deux.
## (a) la fenêtre/le tiroir défilant s'adapte au NOMBRE de faits retenus — un règne à 2 faits
## n'ouvre plus 500px de vide, un règne à 40 faits garde son plafond et défile ; (b) chaque
## entrée porte un GLYPHE + une COULEUR dérivés de `kind` (le type de fait, déjà renvoyé par
## la façade — scps_events.h ANNAL_*) : « mot + couleur + icône » (la règle générale du
## document d'audit), rien d'inventé — juste la catégorie que le moteur donnait déjà et que
## l'ancien rendu ignorait.

signal goto_region(region: int)   ## main.gd centre la carte (même motif que alerts/event_popup)

const Epithet = preload("res://ui/epithet.gd")
const VKit = preload("res://ui/vkit.gd")

## un glyphe + une couleur par ANNAL_* (scps_events.h, MÊME ORDRE que l'enum C ; scps_api.c
## sérialise `kind` en int brut, index direct) — la catégorie du fait, pas son contenu.
const KIND_GLYPH := ["◆", "⤷", "❖", "⚔", "⚔", "✂", "☠", "⌂", "★", "🗡", "✨"]
const KIND_COLOR := [
	Color(0.86, 0.82, 0.74),   # 0 DILEMME          : un choix résolu — neutre, parchemin
	Color(0.72, 0.62, 0.50),   # 1 CICATRICE        : une conséquence mûrie — ambre sourd
	Color(0.55, 0.62, 0.72),   # 2 AGE              : un âge advenu — bleu (comme "Où" ailleurs)
	Color(0.46, 0.74, 0.42),   # 3 GUERRE_GAGNEE    : vert
	Color(0.82, 0.40, 0.34),   # 4 GUERRE_PERDUE    : rouge
	Color(0.82, 0.60, 0.34),   # 5 SECESSION        : orange
	Color(0.60, 0.24, 0.24),   # 6 HEGEMON_BRISE    : (réservé, non accroché en v1)
	Color(0.79, 0.64, 0.29),   # 7 MONUMENT         : or (le premier édifice)
	Color(0.90, 0.80, 0.40),   # 8 FIN              : doré vif (§27/Merveille)
	Color(0.82, 0.42, 0.62),   # 9 TRAHISON         : magenta sourd (un ministre a trahi)
	Color(0.55, 0.80, 0.86),   # 10 MERVEILLE_ETAPE : cyan (un palier de la Merveille)
]
const KIND_DEFAULT_GLYPH := "•"
const KIND_DEFAULT_COLOR := Color(0.93, 0.89, 0.80)

## hauteur de contenu : une ligne de fait ≈ ROW_H px ; le tiroir tient entre MIN et MAX,
## ni un gouffre vide pour 2 faits, ni une fenêtre qui déborde pour un long règne (défile).
const ROW_H  := 30.0
const MIN_SC := 90.0
const MAX_SC := 480.0
const CHROME := 168.0   ## titre+règne+statut+séparateur+pied, hors liste (pour la taille du panneau)

var _panel: PanelContainer
var _scroll: ScrollContainer
var _list: VBoxContainer
var _status: Label
var _reign: Label   ## « Règne de {pays}, dit {épithète} » — l'épithète ÉMERGE des annales

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)
	_panel = PanelContainer.new()
	_panel.custom_minimum_size = Vector2(560, MIN_SC + CHROME)
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

	var title := Label.new()
	title.text = "Les Annales du Règne"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	col.add_child(title)

	_reign = Label.new()
	_reign.add_theme_color_override("font_color", Color(0.93, 0.89, 0.80))
	col.add_child(_reign)

	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
	col.add_child(_status)
	col.add_child(HSeparator.new())

	_scroll = ScrollContainer.new()
	_scroll.custom_minimum_size = Vector2(530, MIN_SC)
	col.add_child(_scroll)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 4)
	_scroll.add_child(_list)

	var foot := HBoxContainer.new()
	col.add_child(foot)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(func(): hide())
	foot.add_child(close)

	visibility_changed.connect(func(): if visible: _rebuild())
	hide()

func _rebuild() -> void:
	if _list == null:
		return
	for c in _list.get_children():
		c.queue_free()
	if Sim.world == null or not Sim.world.has_method("annals"):
		_status.text = "(moteur des Annales absent)"
		_resize_to(0)
		return
	var entries: Array = Sim.world.annals()
	# l'EN-TÊTE de règne : nom du pays + épithète émergente (dérivée du contenu même)
	var me: int = Sim.world.player()
	if me >= 0:
		var ci: Dictionary = Sim.world.country_info(me)
		_reign.text = "Règne de %s, dit %s" % [String(ci.get("nom", "—")), Epithet.derive(entries)]
	else:
		_reign.text = ""
	if entries.is_empty():
		_status.text = "Le règne n'a encore rien à raconter."
		_resize_to(0)
		return
	_status.text = "%d fait(s) retenu(s) — le panthéon du règne." % entries.size()
	for e in entries:
		var kind := int(e.get("kind", -1))
		var glyph: String = KIND_GLYPH[kind] if kind >= 0 and kind < KIND_GLYPH.size() else KIND_DEFAULT_GLYPH
		var col: Color = KIND_COLOR[kind] if kind >= 0 and kind < KIND_COLOR.size() else KIND_DEFAULT_COLOR
		var row := Button.new()
		row.flat = true
		row.alignment = HORIZONTAL_ALIGNMENT_LEFT
		row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		row.text = "%s  %s" % [glyph, String(e.get("ligne", ""))]
		row.add_theme_color_override("font_color", col)
		row.add_theme_color_override("font_hover_color", col.lightened(0.25))
		row.add_theme_color_override("font_disabled_color", Color(col.r, col.g, col.b, 0.72))
		var region := int(e.get("region", -1))
		row.disabled = region < 0   # rien à centrer : pas cliquable (pas un bouton mort visuel non plus — juste inerte)
		row.focus_mode = Control.FOCUS_NONE
		if region >= 0:
			row.pressed.connect(func(): goto_region.emit(region))
		_list.add_child(row)
	_resize_to(entries.size())

## le tiroir/panneau s'adapte au nombre de faits — ni gouffre vide, ni débordement
## (le plafond MAX_SC fait défiler un long règne au lieu de pousser le panneau hors écran).
func _resize_to(n: int) -> void:
	var content_h := clampf(float(maxi(n, 1)) * ROW_H + 12.0, MIN_SC, MAX_SC)
	if _scroll != null:
		_scroll.custom_minimum_size = Vector2(530, content_h)
	if _panel != null:
		_panel.custom_minimum_size = Vector2(560, content_h + CHROME)

## rouvre le panneau (touche H en jeu).
func open() -> void:
	show()
	Sound.play("ui_parchment_open")
	_rebuild()
