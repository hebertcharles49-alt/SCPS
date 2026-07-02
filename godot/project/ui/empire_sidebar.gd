extends Control
## EMPIRE SIDEBAR — la bande DROITE permanente (en jeu) : le RÉSUMÉ D'EMPIRE en haut
## (villes + habitants · armées · flotte · colonisation en cours) et le LOG de
## notifications en bas (le fil, persistant — détails minimes mais exhaustifs).
## Display-only : tout est LU de la façade ; aucun bouton, aucun verbe.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

const AlertsK = preload("res://ui/alerts.gd")   # la TABLE DU FIL (FEED_KINDS) partagée

const W := 268.0
const LOG_MAX := 14

var _seen_seq := 0
var _log := []              ## [{txt, y(an)}] — fil accumulé (le plus récent en tête)
var _city_names := {}       ## region → nom (cache, résolu via province_at(siège))

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # lecture seule : la carte reste cliquable au travers ? Non — bande opaque
	mouse_filter = Control.MOUSE_FILTER_STOP
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.generated.connect(func(): _log.clear(); _city_names.clear(); _seen_seq = 0; queue_redraw())
	Sim.ticked.connect(func(_y): _poll(); queue_redraw())

func _layout() -> void:
	var vp := get_viewport_rect().size
	position = Vector2(vp.x - W, Frame.TOPBAR_H)
	size = Vector2(W, vp.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H)

func _poll() -> void:
	var w = Sim.world
	if w == null or not w.has_method("feed_poll"):
		return
	for ev in w.feed_poll(_seen_seq):
		_seen_seq = maxi(_seen_seq, int(ev["seq"]))
		if not Sim.game_on:
			continue                              # le fil pré-partie est jeté (vitrine)
		var meta: Dictionary = AlertsK.FEED_KINDS.get(int(ev.get("kind", 0)), {})
		var txt := String(meta.get("fmt", String(ev.get("label", "évènement"))))
		txt = txt.replace("{a}", String(ev.get("a", "?"))) \
			.replace("{b}", String(ev.get("b", "?"))) \
			.replace("{r}", _region_name(int(ev.get("region", -1)))) \
			.replace("{label}", String(ev.get("label", ""))) \
			.replace(" (an {y})", "")             # l'an vit déjà en préfixe de ligne
		_log.push_front({"txt": txt, "y": int(ev.get("year", 0))})
	while _log.size() > LOG_MAX:
		_log.pop_back()

func _region_name(r: int) -> String:
	if _city_names.has(r):
		return _city_names[r]
	var w = Sim.world
	var nm := "—"
	var c: Vector2 = w.region_centroid(r)
	if c.x >= 0 and w.has_method("province_at"):
		var pid: int = w.province_at(int(c.x), int(c.y))
		if pid >= 0:
			nm = String(w.province_info(pid).get("nom", "—"))
	_city_names[r] = nm
	return nm

func _draw() -> void:
	visible = Sim.game_on
	if not visible or Sim.world == null:
		return
	var w = Sim.world
	var me: int = w.player()
	VKit.fill(self, Rect2(0, 0, W, size.y), Color(VKit.COL_PANEL.r, VKit.COL_PANEL.g, VKit.COL_PANEL.b, 0.94))
	VKit.fill(self, Rect2(0, 0, 2, size.y), VKit.COL_COPPER)
	var x := 12.0
	var y := 10.0

	# ── VILLES : régions habitées du joueur, triées par âmes ──
	y = VKit.section(self, x, y, "VILLES")
	var cities := []
	for r in range(w.region_count()):
		if int(w.region_owner(r)) != me:
			continue
		var p: int = int(w.region_pop(r))
		if p >= 150:
			cities.append([p, r])
	cities.sort_custom(func(a, b): return a[0] > b[0])
	var shown := 0
	for cd in cities:
		if shown >= 8:
			break
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH, _region_name(cd[1]))
		var pg := _grp(cd[0])
		VKit.text(self, Vector2(W - 14.0 - VKit.text_w(pg), y), VKit.COL_DIM, pg)
		y += 16
		shown += 1
	if cities.size() > shown:
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "… et %d autres" % (cities.size() - shown))
		y += 16
	if cities.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune ville")
		y += 16
	y += 6

	# ── ARMÉES : l'ost de campagne + la réserve levée ──
	y = VKit.section(self, x, y, "ARMÉES")
	var ca: Dictionary = w.country_army(me) if w.has_method("country_army") else {}
	var ai: Dictionary = w.army_info(me)
	if bool(ai.get("active", false)):
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
			"En campagne : %s (%s)" % [_grp(int(ai.get("units", 0))), String(ai.get("phase", ""))])
		y += 16
	var res_n := int(ca.get("regiments", 0))
	VKit.text(self, Vector2(x, y), VKit.COL_PARCH if res_n > 0 else VKit.COL_DIM,
		"Réserve : %s · levée %s" % [_grp(res_n), String(ca.get("levy_name", "—"))])
	y += 16
	var fl := int(ca.get("fleet", 0))
	if fl > 0:
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "Flotte : %d coque(s) disponibles" % fl)
		y += 16
	y += 6

	# ── COLONISATION : le chantier qui mûrit / la cadence ──
	if w.has_method("colony_status"):
		y = VKit.section(self, x, y, "COLONISATION")
		var cs: Dictionary = w.colony_status()
		if bool(cs.get("active", false)):
			var dstp := int(cs.get("dst", -1))
			var nm := String(w.province_info(dstp).get("nom", "—")) if dstp >= 0 else "—"
			var tot := maxi(1, int(cs.get("total_days", 1)))
			var left := int(cs.get("days_left", 0))
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, "Vers %s" % nm)
			y += 16
			UIKit.bar(self, Rect2(x, y + 2, W - 28.0, 10), int(round(100.0 * float(tot - left) / float(tot))))
			y += 16
			VKit.text(self, Vector2(x, y), VKit.COL_DIM,
				"%d j restants · rendement %d %%" % [left, int(cs.get("yield_pct", 0))])
			y += 16
		else:
			var cd := int(cs.get("cd_days", 0))
			VKit.text(self, Vector2(x, y), VKit.COL_DIM,
				("prochain ordre dans %d j" % cd) if cd > 0 else "aucun chantier (ordre possible)")
			y += 16
		y += 6

	# ── LE LOG : le fil de notifications (persistant, le plus récent en tête) ──
	y = VKit.section(self, x, y, "JOURNAL")
	if _log.is_empty():
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien à signaler")
	for e in _log:
		if y > size.y - 20.0:
			break
		var line := "an %d · %s" % [int(e["y"]), String(e["txt"])]
		# tronqué à la largeur (les détails vivent dans les alertes/panneaux)
		while VKit.text_w(line) > W - 26.0 and line.length() > 8:
			line = line.substr(0, line.length() - 4) + "…"
		VKit.text(self, Vector2(x, y), VKit.COL_DIM, line, VKit.FS_SMALL)
		y += 14

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
