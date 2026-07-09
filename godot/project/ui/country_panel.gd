extends Control
## CountryPanel — le bandeau du ROYAUME, habillé : panneau VKit + jauges TEXTURÉES
## (UIKit.bar) + ICÔNES par métrique. Lit country_info (la membrane). Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const PW := 322.0
const PH := 296.0
const MARGIN := 8.0

# métrique → (libellé, nom d'icône du pack)
const ROWS := [
	["stabilite",  "Stabilité",  "stability_shield"],
	["prosperite", "Prospérité", "prosperity_sprout"],
	["legitimite", "Légitimité", "politics_crown"],
	["cohesion",   "Cohésion",   "happiness_medallion"],
	["savoir",     "Savoir",     "knowledge_book"],
]

# HOVERS (point : « je ne sais pas ce qu'est la stabilité ») — explications au survol.
const TIPS := {
	"stabilite":  "Solidité du régime : haute = ordre tenu ; basse = risque de coup d'État ou de révolte. Nourrie par la légitimité et la prospérité.",
	"prosperite": "Richesse par tête de l'empire (biens produits et consommés). Haute = population comblée.",
	"legitimite": "Consentement des gouvernés envers la couronne. Basse = agitation, puis sécession.",
	"cohesion":   "Unité culturelle de l'empire. Basse = fractures, minorités frondeuses.",
	"savoir":     "Avancée de la recherche : c'est elle qui ouvre les technologies.",
}
var _tips: Array = []   ## [ [Rect2, texte], … ] reconstruit à chaque _draw, hit-testé au survol

var _cid := -1
signal close_requested   ## ✕ — la désélection pleine vit dans main (_clear_selection)
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.month_ticked.connect(_on_tick)   # chiffres du pays : cadence mensuelle
	hide()

func _layout() -> void:
	position = Vector2(get_viewport_rect().size.x - PW - MARGIN, Frame.TOPBAR_H + MARGIN)

func show_country(cid: int) -> void:
	_cid = cid
	visible = cid >= 0
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

	# titre + couronne
	UIKit.draw_icon(self, "politics_crown", Vector2(x, y - 1), 20)
	VKit.text(self, Vector2(x + 26, y), VKit.COL_GOLD, String(info["nom"]), VKit.FS_BIG)
	# ✕ — tout panneau se ferme (Échap aussi, via la pile de main)
	_close_rect = Rect2(PW - 22, 4, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	y += 24
	VKit.text(self, Vector2(x, y), VKit.COL_DIM, "%s · %d régions" % [info["ethos"], int(info["regions"])])
	y += 22
	# pop + or, chacun avec son icône
	UIKit.draw_icon(self, "population_group", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_PARCH, _grp(info["pop"]))
	UIKit.draw_icon(self, "gold_coin", Vector2(x + 150, y - 1), 16)
	VKit.text(self, Vector2(x + 170, y), VKit.COL_PARCH, _grp(info["or"]))
	y += 26

	for r in ROWS:
		_gauge_row(x, y, String(r[1]), String(r[2]), int(info[r[0]]))
		_tips.append([Rect2(0.0, y - 2.0, PW, 22.0), String(TIPS.get(r[0], ""))])
		y += 24

	y += 2
	UIKit.draw_icon(self, "influence_compass", Vector2(x, y - 1), 16)
	VKit.text(self, Vector2(x + 20, y), VKit.COL_DIM, "Influence %d" % int(info["influence"]))
	if int(info.get("corruption", 0)) > 0:
		UIKit.draw_icon(self, "corruption_coin", Vector2(x + 150, y - 1), 16)
		VKit.text(self, Vector2(x + 170, y), VKit.COL_DIM, "%d" % int(info["corruption"]))

	# mission décennale (l'objectif courant du pays — mission_of via la façade)
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
			var rew := "prime : "
			if rg > 0:
				rew += "%d or" % rg
			if rq > 0:
				rew += (" + " if rg > 0 else "") + "%d %s" % [rq, String(mis.get("reward_mat", ""))]
			VKit.text(self, Vector2(x + 4, y), VKit.COL_DIM, rew, VKit.FS_SMALL)

## icône · libellé · jauge texturée · CHIFFRE (plus de mot de bande — chiffre + nom seuls)
func _gauge_row(x: float, y: float, label: String, icon: String, value: int) -> void:
	UIKit.draw_icon(self, icon, Vector2(x, y - 1), 18)
	VKit.text(self, Vector2(x + 22, y), VKit.COL_DIM, label, VKit.FS_SMALL)
	UIKit.bar(self, Rect2(x + 96, y, 88, 14), value)
	VKit.text(self, Vector2(x + 190, y), VKit.COL_PARCH, str(value), VKit.FS_SMALL)

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

## HOVER natif : Godot appelle ceci au survol → on rend le texte de la zone touchée.
func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""
