extends PanelContainer
## WindowPanel — LA fenêtre contextuelle GÉNÉRIQUE (décision joueur 2026-07-21 : « pas
## dessiné à la main : UNE fenêtre, déclinable selon le contexte — Godot sait faire des
## panneaux »). Chrome COMMUN en conteneurs NATIFS + ParchTheme (le thème existant) :
##   header parchemin (titre · sous-titre contextuel · ✕) + corps DÉFILABLE (hug-content
##   jusqu'à un plafond, puis ScrollContainer). Chaque contexte (province, pays, empire…)
##   REMPLIT body() de Controls natifs — plus aucun _draw, plus aucun VKit.text à la main.
##
## Usage :
##   var win := preload("res://ui/window_panel.gd").new()
##   parent.add_child(win)
##   win.set_header("Nom", "Région · Tier 3")
##   win.body().add_child(<mon Label / ma ligne native>)
##   win.place(Vector2(x, y), ceiling)      # position + plafond de hauteur
##   win.closed.connect(_on_close)

const ParchTheme = preload("res://ui/parch_theme.gd")

signal closed

var _title: Label
var _subtitle: Label
var _body: VBoxContainer
var _scroll: ScrollContainer

func _ready() -> void:
	theme = ParchTheme.build()            # le thème parchemin PARTAGÉ (jamais un style ad-hoc)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	# HEADER (variation HeaderStrip de ParchTheme) : titre · sous-titre · ✕
	var head := PanelContainer.new()
	head.theme_type_variation = "HeaderStrip"
	root.add_child(head)
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	head.add_child(hb)
	var tcol := VBoxContainer.new()
	tcol.add_theme_constant_override("separation", 0)
	tcol.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hb.add_child(tcol)
	_title = Label.new()
	_title.theme_type_variation = "Title"
	tcol.add_child(_title)
	_subtitle = Label.new()
	_subtitle.theme_type_variation = "RowDim"
	_subtitle.visible = false             # masqué tant qu'aucun sous-titre (pas de ligne vide)
	tcol.add_child(_subtitle)
	var close := Button.new()
	close.theme_type_variation = "Tab"
	close.text = "✕"
	close.focus_mode = Control.FOCUS_NONE
	close.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	close.pressed.connect(func(): closed.emit())
	hb.add_child(close)

	# CORPS DÉFILABLE (variation Body, fond transparent) : le contenu contextuel vit ici.
	_scroll = ScrollContainer.new()
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(_scroll)
	_body = VBoxContainer.new()
	_body.add_theme_constant_override("separation", 4)
	_body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scroll.add_child(_body)

## le VBox où le contexte pose son contenu (Controls natifs, jamais _draw).
func body() -> VBoxContainer:
	return _body

func set_header(title: String, subtitle: String = "") -> void:
	if _title != null:
		_title.text = title
	if _subtitle != null:
		_subtitle.text = subtitle
		_subtitle.visible = subtitle != ""

## vide le corps (rebuild par contexte — le contenu n'a pas d'état ; les widgets à état
## d'un contexte donné se reconstruisent avec, motif _ARCHITECTURE §1).
func clear_body() -> void:
	if _body == null:
		return
	for c in _body.get_children():
		c.queue_free()

## POSE la fenêtre : coin haut-gauche + plafond de hauteur. La hauteur HUG le contenu
## visible (get_combined_minimum_size) jusqu'au plafond ; au-delà, le ScrollContainer
## prend le relais (le pattern validé sur le tiroir diplo — plus de rectangle à moitié vide).
func place(top_left: Vector2, ceiling: float, width: float = 380.0) -> void:
	position = top_left
	relayout(ceiling, width)

func relayout(ceiling: float, width: float = 380.0) -> void:
	var content := 120.0
	if _body != null:
		content = _body.get_combined_minimum_size().y + _header_h() + 20.0   # +20 = marges panneau
	custom_minimum_size = Vector2(width, 0)
	size = Vector2(width, clampf(content, 120.0, maxf(120.0, ceiling)))

func _header_h() -> float:
	var root := get_child(0) if get_child_count() > 0 else null
	if root != null and root.get_child_count() > 0:
		return (root.get_child(0) as Control).get_combined_minimum_size().y
	return 30.0
