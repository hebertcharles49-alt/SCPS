extends Control
## CountryPanel — le bandeau du ROYAUME, habillé : panneau VKit + jauges TEXTURÉES
## (UIKit.bar) + ICÔNES par métrique. Lit country_info (la membrane). Display-only.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const PW := 322.0
const PH := 240.0   ## raccourci : les jauges internes d'un royaume ÉTRANGER ne s'affichent plus
const MARGIN := 8.0

# métrique → (libellé, nom d'icône du pack)
const ROWS := [
	["stabilite",  "Stabilité",  "stability_shield"],
	["prosperite", "Prospérité", "prosperity_sprout"],
	["legitimite", "Légitimité", "politics_crown"],
	["cohesion",   "Cohésion",   "happiness_medallion"],
	["savoir",     "Savoir",     "knowledge_book"],
]

# HOVERS (retour joueur 2026-07-10, « quoi + combien ») : le hover ne DÉFINIT
# plus le concept (c'était une redite du codex) — il donne juste son NOM. Le mot
# lui-même est déjà décoré turquoise et cliquable par le TooltipServer (lit
# ui/concepts.gd) : SA définition vit derrière ce clic, jamais répétée ici.
# Aucune décomposition moteur n'existe pour ces 4 jauges au grain PAYS (seule
# l'agitation de PROVINCE a un breakdown, scps_readout.c:metric_agitation_
# breakdown) — pas de « combien » à ajouter, donc pas de leçon non plus.
const TIPS := {
	"stabilite":  "Stabilité",
	"prosperite": "Prospérité",
	"legitimite": "Légitimité",
	"cohesion":   "Cohésion",
	"savoir":     "Savoir",
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
	# à GAUCHE du ledger (empire_sidebar, 268 px) en partie — il était CACHÉ dessous
	var off := 272.0 if Sim.game_on else 0.0
	position = Vector2(get_viewport_rect().size.x - PW - MARGIN - off, Frame.TOPBAR_H + MARGIN)

func show_country(cid: int) -> void:
	# doctrine joueur « national = topbar » : SON royaume vit dans la barre haute — ce
	# panneau ne s'ouvre que pour un pays ÉTRANGER (le clic chez soi garde la province).
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

	# titre + couronne
	UIKit.draw_icon(self, "politics_crown", Vector2(x, y - 1), 20)
	VKit.text(self, Vector2(x + 26, y), VKit.COL_GOLD, String(info["nom"]), VKit.FS_BIG)
	# ✕ — tout panneau se ferme (Échap aussi, via la pile de main)
	_close_rect = Rect2(PW - 22, 4, 16, 16)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 4, _close_rect.position.y + 1), VKit.COL_PARCH, "x")
	y += 24
	VKit.detail(self, Vector2(x, y), "%s · %d régions" % [info["ethos"], int(info["regions"])], VKit.FS)
	y += 22
	# pop, avec son icône (l'ESTIMATION extérieure — ce qui se voit d'un royaume) —
	# LA valeur principale du panneau étranger : la taille du peuple.
	UIKit.draw_icon(self, "population_group", Vector2(x, y - 1), 16)
	VKit.value(self, Vector2(x + 20, y), _grp(info["pop"]))
	y += 26

	# ON NE LIT PAS DANS LE ROYAUME D'AUTRUI (retour joueur : « pourquoi je vois les
	# métriques des autres entités ? ») — ce panneau est ÉTRANGER-seul depuis la
	# doctrine « national = topbar » : ni trésor, ni jauges internes. Ce qui se SAIT :
	# l'éthos, la taille, les âmes (estimées), l'influence (réputation PUBLIQUE).
	UIKit.draw_icon(self, "influence_compass", Vector2(x, y - 1), 16)
	var infl_lbl_w: float = VKit.detail(self, Vector2(x + 20, y), "Influence ", VKit.FS)
	VKit.value(self, Vector2(x + 20 + infl_lbl_w, y), str(int(info["influence"])), VKit.FS)
	_tips.append([Rect2(0.0, y - 2.0, PW, 20.0), "Influence"])
	y += 4

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
			var rew := ""
			if rg > 0:
				rew += "%d or" % rg
			if rq > 0:
				rew += (" + " if rg > 0 else "") + "%d %s" % [rq, String(mis.get("reward_mat", ""))]
			var rew_x: float = VKit.detail(self, Vector2(x + 4, y), "prime : ", VKit.FS_SMALL)
			VKit.value(self, Vector2(x + 4 + rew_x, y), rew, VKit.FS_SMALL)

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
