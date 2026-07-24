extends Control
## Bandeau d'un royaume ÉTRANGER : lit country_info (la membrane). Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const Concepts = preload("res://ui/concepts.gd")
const PW := 322.0
const PH := 240.0   ## royaume étranger : pas de jauges internes
const MARGIN := 8.0

# le hover NOMME juste le concept (défini derrière son clic turquoise, via TooltipServer) ;
# aucun breakdown moteur au grain pays (seule l'agitation de province en a un).
const TIPS := {
	"stabilite":  "Stabilité",
	"prosperite": "Prospérité",
	"legitimite": "Légitimité",
	"cohesion":   "Cohésion",
	"savoir":     "Savoir",
}
var _tips: Array = []   ## [[Rect2, texte], …] reconstruit à chaque _draw

var _cid := -1
signal close_requested   ## la désélection pleine vit dans main (_clear_selection)
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.month_ticked.connect(_on_tick)   # chiffres du pays : cadence mensuelle
	hide()

func _layout() -> void:
	# décalé à gauche du ledger (empire_sidebar) — sinon caché dessous
	var off := 272.0 if Sim.game_on else 0.0
	position = Vector2(get_viewport_rect().size.x - PW - MARGIN - off, Frame.TOPBAR_H + MARGIN)

func show_country(cid: int) -> void:
	# national = topbar : ce panneau ne s'ouvre que pour un pays ÉTRANGER
	if cid >= 0 and Sim.world != null and cid == int(Sim.world.player()):
		cid = -1
	_cid = cid
	visible = cid >= 0
	_layout()
	queue_redraw()

func _on_tick(_year: int) -> void:
	if visible:
		queue_redraw()

func _draw() -> void:
	var w = Sim.world
	if w == null or _cid < 0:
		return
	var info: Dictionary = w.country_info(_cid)
	if not bool(info.get("valide", false)):
		return
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	_tips.clear()
	var x := 16.0
	var y := 12.0

	UIKit.draw_icon(self, "politics_crown", Vector2(x, y - 1), 20)
	VKit.text(self, Vector2(x + 26, y), VKit.COL_GOLD, String(info["nom"]), VKit.FS_BIG)
	_close_rect = Rect2(PW - 22, 4, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	y += 24
	var eth_w: float = VKit.detail(self, Vector2(x, y), "%s · %d régions" % [info["ethos"], int(info["regions"])], VKit.FS)
	# cette ligne est le seul survol nommant l'Éthos dans ce panneau
	_tips.append([Rect2(x, y - 2.0, eth_w, 16.0), Concepts.def_of("Éthos")])
	y += 22
	# taille du peuple : la valeur principale d'un panneau étranger
	UIKit.draw_icon(self, "population_group", Vector2(x, y - 1), 16)
	VKit.value(self, Vector2(x + 20, y), _grp(info["pop"]))
	y += 26

	# panneau étranger : pas de trésor ni jauges internes — seulement éthos, taille, influence publique
	UIKit.draw_icon(self, "influence_compass", Vector2(x, y - 1), 16)
	var infl_lbl_w: float = VKit.detail(self, Vector2(x + 20, y), "Influence ", VKit.FS)
	VKit.value(self, Vector2(x + 20 + infl_lbl_w, y), str(int(info["influence"])), VKit.FS)
	_tips.append([Rect2(0.0, y - 2.0, PW, 20.0), "Influence"])
	y += 4

	# mission courante du pays (mission_of)
	var mis: Dictionary = w.mission_info(_cid)
	if bool(mis.get("active", false)):
		y += 26
		VKit.fill(self, Rect2(x, y, PW - 2.0 * x, 1), VKit.COL_EDGE)
		y += 6
		VKit.text(self, Vector2(x, y), VKit.COL_GOLD, "✦ Mission", VKit.FS_SMALL)
		y += 16
		VKit.text(self, Vector2(x + 4, y), VKit.COL_PARCH, String(mis["text"]), VKit.FS_SMALL)
		var rg := int(mis.get("reward_gold", 0))
		var rq := int(mis.get("reward_qty", 0))
		if rg > 0 or rq > 0:
			y += 15
			var rew := ""
			if rg > 0:
				rew += "%d or" % rg
			if rq > 0:
				rew += (" + " if rg > 0 else "") + "%d %s" % [rq, String(mis.get("reward_mat", ""))]
			var rew_x: float = VKit.detail(self, Vector2(x + 4, y), "prime : ", VKit.FS_SMALL)
			VKit.value(self, Vector2(x + 4 + rew_x, y), rew, VKit.FS_SMALL)

func _grp(n) -> String:
	var s := str(absi(int(n)))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" if int(n) < 0 else "") + out

# hover natif : rend le texte de la zone touchée.
func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""
