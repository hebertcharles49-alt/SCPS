extends Control
## Topbar — bandeau PLEINE LARGEUR (cadre d'écran) : capsule de date (An N) + le
## roll-up du PAYS JOUÉ (nom · or · pop empire · régions · savoir) à gauche, contrôle
## de VITESSE cliquable à DROITE. Suit la largeur de la fenêtre (size_changed).
## Display-only sauf le verbe vitesse. Lit Sim.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const H := Frame.TOPBAR_H

signal tech_requested

var _speed_rect := Rect2()
var _savoir_rect := Rect2()
var _age_rect := Rect2()   # §7 : chip « Âge levé — Engager » (vide quand rien à engager)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics (pas la carte dessous)
	_resize()
	get_viewport().size_changed.connect(_resize)
	Sim.generated.connect(_on_change)
	Sim.ticked.connect(_on_tick)

func _resize() -> void:
	position = Vector2.ZERO
	size = Vector2(get_viewport_rect().size.x, H)
	queue_redraw()

func _on_tick(_year: int) -> void:
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	var ww := size.x
	# barre PLEINE LARGEUR : navy + liseré cuivre en bas (cadre franc, pas arrondi)
	VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, H - 2, ww, 2), VKit.COL_COPPER)
	var cy := (H - 18.0) * 0.5     # centrage vertical du contenu

	if Sim.world == null:
		VKit.text(self, Vector2(16, cy), VKit.COL_DIM, "(libscps absente — voir README)")
		return
	if not Sim.world.has_method("province_at"):
		VKit.text(self, Vector2(12, cy), VKit.sense(0.5),
			"⚠ libscps OBSOLÈTE — rebâtir : scons platform=windows use_mingw=yes")
		return

	var w = Sim.world
	# capsule de date : An N (la capsule porte déjà sa rose des vents — pas d'icône en plus)
	UIKit.draw_chrome(self, "topbar_date_capsule", Rect2(10, 4, 92, H - 8))
	VKit.text(self, Vector2(22, cy), VKit.COL_PARCH, "An %d" % w.year())

	# LE PAYS JOUÉ (roll-up d'affichage — le pays n'a pas d'incidence propre : juste
	# l'agrégat de ses provinces) : nom · or · pop EMPIRE · régions · savoir.
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var px := 116.0
	if bool(ci.get("valide", false)):
		UIKit.draw_icon(self, "politics_crown", Vector2(px, cy - 2), 18); px += 22
		var nom := String(ci["nom"])
		VKit.text(self, Vector2(px, cy), VKit.COL_COPPER, nom); px += VKit.text_w(nom) + 20
		UIKit.draw_icon(self, "fine_coin", Vector2(px, cy - 2), 16); px += 20
		var org := _grp(ci["or"]); VKit.text(self, Vector2(px, cy), VKit.COL_PARCH, org); px += VKit.text_w(org) + 20
		UIKit.draw_icon(self, "population_group", Vector2(px, cy - 2), 16); px += 20
		var popg := _grp(ci["pop"]); VKit.text(self, Vector2(px, cy), VKit.COL_PARCH, popg); px += VKit.text_w(popg) + 20
		# modèle province (EU4) : le pays compte ses PROVINCES (pas ses régions)
		var rt := "%d provinces" % w.country_province_count(me)
		VKit.text(self, Vector2(px, cy), VKit.COL_DIM, rt); px += VKit.text_w(rt) + 18
		var sx0 := px
		UIKit.draw_icon(self, "fine_knowledge", Vector2(px, cy - 2), 16); px += 20
		var sv := "%d" % int(ci["savoir"])
		VKit.text(self, Vector2(px, cy), VKit.COL_PARCH, sv); px += VKit.text_w(sv) + 20
		_savoir_rect = Rect2(sx0 - 4, 0, 24 + VKit.text_w(sv) + 8, H)
		# NOURRITURE DISPONIBLE (v50) : Σ stock vivrier de l'empire — la réserve en rations
		if w.has_method("country_food"):
			UIKit.draw_icon(self, "fine_grain", Vector2(px, cy - 2), 16); px += 20
			var fg := _grp(int(w.country_food(me)))
			VKit.text(self, Vector2(px, cy), VKit.COL_PARCH, fg); px += VKit.text_w(fg) + 20
		# CHANTIER DE COLONISATION (v50) : la colonie qui mûrit / la cadence de l'ordre suivant
		if w.has_method("colony_status"):
			var cs: Dictionary = w.colony_status()
			if bool(cs.get("active", false)):
				var tot := maxi(1, int(cs.get("total_days", 1)))
				var pct := int(round(100.0 * float(tot - int(cs.get("days_left", 0))) / float(tot)))
				var ctxt := "Colonie %d %%" % pct
				UIKit.draw_icon(self, "settlement_cluster", Vector2(px, cy - 2), 16); px += 20
				VKit.text(self, Vector2(px, cy), Color(0.62, 0.78, 0.52), ctxt); px += VKit.text_w(ctxt) + 20

	# §7 — ENGAGEMENT D'ÂGE : un âge s'est levé et le joueur ne l'a pas engagé →
	# chip ambre cliquable (l'IA s'engage auto ; le joueur choisit — verbe CMD_AGE_ENGAGE).
	_age_rect = Rect2()
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			var lab := "Engager : %s" % String(ag.get("name", ""))
			var aw := VKit.text_w(lab) + 34.0
			_age_rect = Rect2(ww - 116 - aw - 10, 4, aw, H - 8)
			UIKit.draw_chrome(self, "topbar_resource_chip", _age_rect)
			UIKit.draw_icon(self, "fine_age", Vector2(_age_rect.position.x + 6, cy - 2), 16)
			VKit.text(self, Vector2(_age_rect.position.x + 26, cy), Color(0.85, 0.65, 0.3), lab)

	# contrôle de vitesse, ancré à DROITE de la barre
	_speed_rect = Rect2(ww - 116, 4, 108, H - 8)
	UIKit.draw_chrome(self, "topbar_resource_chip", _speed_rect)
	var paused: bool = Sim.speed_index == 0
	UIKit.draw_icon(self, "tool_speed" if paused else "tool_pause",
		Vector2(_speed_rect.position.x + 6, cy - 2), 18)
	VKit.text(self, Vector2(_speed_rect.position.x + 28, cy), VKit.COL_COPPER, Sim.speed_label())

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _savoir_rect.has_point(event.position):
			tech_requested.emit()
		elif _age_rect.size.x > 0 and _age_rect.has_point(event.position):
			if Sim.world != null and Sim.world.has_method("player_age_engage"):
				Sim.world.player_age_engage()   # enfilé ; le chip s'éteint au drain (engaged=true)
				queue_redraw()
		elif _speed_rect.has_point(event.position):
			Sim.cycle_speed()
			queue_redraw()

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
