extends Control
## ALERTES (façon EU4/CK3) — la pile des « ÉLÉMENTS EN ATTENTE du gameplay », ancrée au
## bord DROIT sous la topbar. Chaque alerte = un chip carré à CODE COULEUR par domaine :
##   ÉTATIQUE violet (conseil vacant, âge à engager) · ARMÉE rouge (guerre sans levée/ost)
##   · SOCIAL vert (édifice constructible) · SAVOIR bleu (aucune recherche) · FOI doré
##   (fondation prête). Clic = ouvre le panneau concerné (ou exécute le geste) ; survol =
## tooltip. Display-only : tout est LU de la façade, les clics émettent des signaux —
## main câble les panneaux. Recalculé au tick (queue_redraw), jamais un état propre.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

signal open_tab(i: int)     ## onglet de la sidebar (4 = Armée · 7 = Conseil)
signal open_tech
signal open_construct
signal open_religion

const COL_ETAT   := Color(0.55, 0.38, 0.66)   ## violet — étatique
const COL_ARMEE  := Color(0.72, 0.28, 0.24)   ## rouge — armée
const COL_SOCIAL := Color(0.44, 0.62, 0.36)   ## vert — social/développement
const COL_SAVOIR := Color(0.37, 0.54, 0.70)   ## bleu — savoir
const COL_FOI    := Color(0.79, 0.64, 0.30)   ## doré — foi

const CHIP := 30.0
const GAP := 6.0

var _alerts := []   ## [{icon, col, tip, act}] recalculées à chaque _refresh

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	Sim.generated.connect(_refresh)
	Sim.ticked.connect(func(_y): _refresh())
	get_viewport().size_changed.connect(_refresh)
	_refresh.call_deferred()

## recalcule la pile + repositionne le contrôle (il n'occupe QUE sa colonne de chips —
## jamais un plein-écran qui mangerait les clics de la carte).
func _refresh() -> void:
	_alerts = _collect()
	var vw := get_viewport_rect().size.x
	position = Vector2(vw - CHIP - 10.0, Frame.TOPBAR_H + 10.0)
	size = Vector2(CHIP, maxf(1.0, _alerts.size() * (CHIP + GAP)))
	visible = _alerts.size() > 0
	queue_redraw()

## LA COLLECTE : chaque « élément en attente » du gameplay, lu de la façade.
func _collect() -> Array:
	var out := []
	var w = Sim.world
	if w == null or not w.has_method("country_council"):
		return out
	var me: int = w.player()
	if me < 0:
		return out
	# ÉTATIQUE — siège(s) du conseil VACANT(S) (la pool par générations est toujours pleine)
	var vac := 0
	for seat in w.country_council(me):
		if not bool(seat["filled"]):
			vac += 1
	if vac > 0:
		out.append({"icon": "menu_council", "col": COL_ETAT, "act": "council",
			"tip": "%d siège(s) du conseil VACANT(S) — recruter un candidat (clic : onglet Conseil)" % vac})
	# ÉTATIQUE — un âge s'est levé et n'est pas engagé
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			out.append({"icon": "politics_crown", "col": COL_ETAT, "act": "age",
				"tip": "Un âge s'est levé : %s — clic pour l'ENGAGER (une fois par âge)" % String(ag.get("name", ""))})
	# SAVOIR — aucune recherche en cours
	var rs: Dictionary = w.research_status()
	if int(rs.get("target", -1)) < 0:
		out.append({"icon": "knowledge_book", "col": COL_SAVOIR, "act": "tech",
			"tip": "Aucune RECHERCHE en cours — clic : choisir une cible dans l'arbre"})
	# SOCIAL — au moins un édifice CONSTRUCTIBLE (débloqué + or suffisant)
	var ci: Dictionary = w.country_info(me)
	var gold: float = float(ci.get("or", 0))
	for b in w.building_roster(me):
		if bool(b.get("debloque", false)) and float(b.get("gold", 1e18)) <= gold:
			out.append({"icon": "action_build", "col": COL_SOCIAL, "act": "construct",
				"tip": "Un ÉDIFICE est constructible (ex. %s, %d or) — clic : panneau Construction" % [String(b.get("nom", "")), int(b.get("gold", 0))]})
			break
	# ARMÉE — EN GUERRE : levée à zéro, ou pas d'armée de campagne déployée
	var at_war := false
	for rel in w.country_relations(me):
		if bool(rel.get("at_war", false)):
			at_war = true
			break
	if at_war:
		var a: Dictionary = w.country_army(me)
		if int(a.get("levy", 0)) <= 0:
			out.append({"icon": "menu_army", "col": COL_ARMEE, "act": "army",
				"tip": "EN GUERRE et levée à ZÉRO — clic : monter la levée (onglet Armée)"})
		elif not bool(w.army_info(me).get("active", false)):
			out.append({"icon": "menu_army", "col": COL_ARMEE, "act": "army",
				"tip": "EN GUERRE sans armée de campagne — clic : onglet Armée (puis « Attaquer ici » sur la cible)"})
	# FOI — la fondation est PRÊTE (1er édifice religieux bâti, pas encore de foi)
	if w.has_method("religion_founding_ready") and int(w.religion_founding_ready(me)) == 1:
		out.append({"icon": "faith_candle", "col": COL_FOI, "act": "religion",
			"tip": "Votre peuple a bâti son premier sanctuaire — clic : FONDER la foi"})
	return out

func _draw() -> void:
	var y := 0.0
	for al in _alerts:
		var r := Rect2(0, y, CHIP, CHIP)
		VKit.fill(self, r, VKit.COL_PANEL)
		var c: Color = al["col"]
		draw_rect(r, c, false, 2.0)                       # le CODE COULEUR : liseré épais du domaine
		draw_rect(Rect2(0, y + CHIP - 4, CHIP, 4), c)     # + socle plein (lisible même en périphérie)
		UIKit.draw_icon(self, String(al["icon"]), Vector2(5, y + 3), 20)
		y += CHIP + GAP

func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT):
		return
	var idx := int(event.position.y / (CHIP + GAP))
	if idx < 0 or idx >= _alerts.size():
		return
	accept_event()
	match String(_alerts[idx]["act"]):
		"council":
			open_tab.emit(7)
		"army":
			open_tab.emit(4)
		"tech":
			open_tech.emit()
		"construct":
			open_construct.emit()
		"religion":
			open_religion.emit()
		"age":
			if Sim.world != null and Sim.world.has_method("player_age_engage"):
				Sim.world.player_age_engage()   # le geste EST l'action (drainé au tick)
	_refresh()

## HOVER natif : le tooltip de l'alerte survolée.
func _get_tooltip(at_position: Vector2) -> String:
	var idx := int(at_position.y / (CHIP + GAP))
	if idx >= 0 and idx < _alerts.size():
		return String(_alerts[idx]["tip"])
	return ""
