extends Control
## MapControls — les COMMANDES DE CARTE du bas d'écran : sélecteur de MODE (terrain ·
## politique · régions · pays) à gauche, ZOOM (in/out/fit) à droite. Les icônes
## FLOTTENT sur la carte (façon Stellaris — plus de bande pleine largeur) : le parent
## laisse passer les clics, chaque bouton capte les siens. Câblé à MapView.

const VKit = preload("res://ui/vkit.gd")
const IconButton = preload("res://ui/icon_button.gd")
const Frame = preload("res://ui/frame.gd")
const H := Frame.BOTTOMBAR_H
const BTN := 52.0   ## boutons de mode/zoom agrandis (retour joueur : « très très petits »)

# mode render_map → icône du pack
const MODES := [
	[0, "layer_terrain"],      # Terrain
	[1, "politics_law"],       # Politique
	[2, "settlement_cluster"], # Régions
	[3, "politics_crown"],     # Pays
]

# LÉGENDES de couleur des modes d'ÉTAT (sinon les teintes n'ont aucun sens) — calées
# sur les teintes calculées par map_state_tint (façade). « grad » = échelle continue
# (lo→hi) ; « sw » = pastilles discrètes. La box est VARIABLE (taille au mode courant).
const LEGENDS := {
	13: {"t": "Stabilité", "grad": [Color(0.94, 0.12, 0.16), Color(0.55, 0.50, 0.16), Color(0.10, 0.86, 0.16)], "lo": "instable", "hi": "stable"},
	14: {"t": "Commerce", "grad": [Color(0.19, 0.16, 0.12), Color(0.96, 0.65, 0.22)], "lo": "faible", "hi": "actif"},
	15: {"t": "Guerre", "sw": [[Color(0.75, 0.16, 0.13), "occupé"], [Color(0.75, 0.47, 0.13), "belligérant"], [Color(0.21, 0.31, 0.18), "paix"]]},
	16: {"t": "Diplomatie", "sw": [[Color(0.18, 0.39, 0.75), "soi"], [Color(0.18, 0.63, 0.31), "allié"], [Color(0.75, 0.16, 0.16), "guerre"], [Color(0.44, 0.44, 0.47), "neutre"]]},
}

var _map
var _mode := 0
var _mode_btns := []
var _nature_btn: Control
var _mb: HBoxContainer
var _zb: HBoxContainer

func setup(map) -> void:
	_map = map
	if map != null and map.has_signal("mode_changed"):
		map.mode_changed.connect(func(_m): queue_redraw())   # la légende suit le mode (clic carte OU sidebar)

func _ready() -> void:
	# les icônes flottent : le parent NE capte RIEN (la carte reste cliquable entre
	# les boutons) — chaque IconButton enfant capte ses propres clics.
	mouse_filter = Control.MOUSE_FILTER_IGNORE

	_mb = HBoxContainer.new()
	_mb.add_theme_constant_override("separation", 4)
	add_child(_mb)
	for m in MODES:
		var b = IconButton.new()
		_mb.add_child(b)
		b.setup_icon(String(m[1]), BTN, "")   # SANS fond de chrome (retour joueur)
		b.selected = (int(m[0]) == _mode)
		b.pressed.connect(_on_mode.bind(int(m[0])))
		_mode_btns.append(b)

	# NATURE : toggle indépendant des modes de carte (bascule la carte vierge — terrain seul)
	_nature_btn = IconButton.new()
	_mb.add_child(_nature_btn)
	_nature_btn.setup_icon("layer_forest", BTN, "")
	_nature_btn.pressed.connect(_on_nature)

	_zb = HBoxContainer.new()
	_zb.add_theme_constant_override("separation", 4)
	add_child(_zb)
	for c in [["map_zoom_in", "_zin"], ["map_zoom_out", "_zout"], ["map_fit_view", "_zfit"]]:
		var b = IconButton.new()
		_zb.add_child(b)
		b.setup_chrome(String(c[0]), BTN)
		b.pressed.connect(Callable(self, String(c[1])))

	get_viewport().size_changed.connect(_resize)
	_resize.call_deferred()

func _resize() -> void:
	var vp := get_viewport_rect().size
	position = Vector2(0, vp.y - H)
	size = Vector2(vp.x, H)
	var by := (H - BTN) * 0.5                    # centrage vertical des boutons
	_mb.position = Vector2(Frame.SIDEBAR_W + 10, by)   # à droite du rail
	# largeur EXPLICITE : au 1er _resize le HBox n'est pas layouté (_zb.size.x = 0)
	# → les boutons partaient coupés au bord droit (capture 2026-07-09)
	var zw := 3.0 * BTN + 2.0 * 4.0
	_zb.position = Vector2(vp.x - zw - 12, by)
	queue_redraw()

func _draw() -> void:
	# plus de bande pleine largeur (retour joueur : les icônes FLOTTENT) —
	# seule la légende du mode d'état garde sa box propre.
	_draw_legend()

## la LÉGENDE du mode courant, en BOX au-dessus des boutons de mode (y négatif = au-dessus
## de la barre). Taille VARIABLE au mode. Affichée seulement pour les modes d'état (LEGENDS).
func _draw_legend() -> void:
	var m: int = int(_map.mode) if _map != null else _mode
	if not LEGENDS.has(m):
		return
	var L: Dictionary = LEGENDS[m]
	var x := Frame.SIDEBAR_W + 10.0
	var bw := 168.0
	if L.has("sw"):
		bw = 16.0
		for s in L["sw"]:
			bw += 16.0 + VKit.text_w(String(s[1]), VKit.FS_SMALL) + 12.0
	var bh := 42.0
	var y := -(bh + 6.0)
	VKit.fill(self, Rect2(x, y, bw, bh), VKit.COL_PANEL2)
	VKit.box(self, Rect2(x, y, bw, bh), VKit.COL_GOLD)
	VKit.text(self, Vector2(x + 8, y + 5), VKit.COL_GOLD, String(L["t"]), VKit.FS_SMALL)
	if L.has("grad"):
		var gx := x + 8.0
		var gy := y + 23.0
		var gw := bw - 16.0
		var stops: Array = L["grad"]
		var steps := int(gw)
		for i in range(steps):
			VKit.fill(self, Rect2(gx + i, gy, 1.0, 10.0), _grad_at(stops, float(i) / float(maxi(1, steps - 1))))
		VKit.box(self, Rect2(gx, gy, gw, 10.0), VKit.COL_DIM)
		VKit.text(self, Vector2(gx, gy + 12), VKit.COL_DIM, String(L["lo"]), VKit.FS_SMALL)
		var hw := VKit.text_w(String(L["hi"]), VKit.FS_SMALL)
		VKit.text(self, Vector2(gx + gw - hw, gy + 12), VKit.COL_DIM, String(L["hi"]), VKit.FS_SMALL)
	elif L.has("sw"):
		var sx := x + 8.0
		var sy := y + 22.0
		for s in L["sw"]:
			VKit.fill(self, Rect2(sx, sy, 11, 11), s[0])
			VKit.box(self, Rect2(sx, sy, 11, 11), VKit.COL_DIM)
			VKit.text(self, Vector2(sx + 15, sy - 1), VKit.COL_PARCH, String(s[1]), VKit.FS_SMALL)
			sx += 16.0 + VKit.text_w(String(s[1]), VKit.FS_SMALL) + 12.0

func _grad_at(stops: Array, t: float) -> Color:
	if stops.size() <= 1:
		return stops[0] if stops.size() == 1 else Color.WHITE
	var seg := t * float(stops.size() - 1)
	var i := int(seg)
	if i >= stops.size() - 1:
		i = stops.size() - 2
	return (stops[i] as Color).lerp(stops[i + 1] as Color, seg - float(i))

func _on_mode(m: int) -> void:
	_mode = m
	if _map != null:
		_map.set_mode(m)
	for i in range(_mode_btns.size()):
		_mode_btns[i].selected = (int(MODES[i][0]) == m)
		_mode_btns[i].queue_redraw()

func _on_nature() -> void:
	if _map != null:
		_map.toggle_nature()
		_nature_btn.selected = _map.is_nature()
		_nature_btn.queue_redraw()

func _zin() -> void:  if _map != null: _map.zoom_in()
func _zout() -> void: if _map != null: _map.zoom_out()
func _zfit() -> void: if _map != null: _map.fit()
