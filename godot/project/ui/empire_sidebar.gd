extends Control
## EMPIRE SIDEBAR — la bande DROITE permanente (en jeu) : le RÉSUMÉ D'EMPIRE en haut
## (villes + habitants · armées · flotte · colonisation en cours) et le LOG de
## notifications en bas (le fil, persistant — détails minimes mais exhaustifs).
## Display-only : tout est LU de la façade ; aucun bouton, aucun verbe.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

const AlertsK = preload("res://ui/alerts.gd")   # la TABLE DU FIL (FEED_KINDS) partagée

const W := 288.0            ## élargie (retour joueur 2026-07-10 : « laisse respirer »)
const HANDLE_W := 14.0      ## bande réduite quand la sidebar est REPLIÉE (rabat)
const LOG_MAX := 18         ## (les factions parties en topbar, le journal respire)

var _seen_seq := 0
var _log := []              ## [{txt, y(an)}] — fil accumulé (le plus récent en tête)
var _city_names := {}       ## region → nom (cache, résolu via province_at(siège))
var _collapsed := false     ## rabat (pièces planche 23 : 01 replier · 02 déplier)
var _handle_rect := Rect2()
var _refill_rect := Rect2() ## chip RECOMPLÉTER (déménagé du tiroir Armée — retour joueur)
var _fold := {}             ## titre de section → replié (retour joueur 2026-07-10 :
                            ## « tous les menus de droite doivent pouvoir se collapser »)
var _sec_rects := []        ## [{rect, title}] bandeaux cliquables (reconstruit au _draw)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # lecture seule : la carte reste cliquable au travers ? Non — bande opaque
	mouse_filter = Control.MOUSE_FILTER_STOP
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.generated.connect(func(): _log.clear(); _city_names.clear(); _seen_seq = 0; queue_redraw())
	Sim.month_ticked.connect(func(_y): _poll(); queue_redraw())   # résumé d'empire : cadence mensuelle

## ⚠ la visibilité vivait DANS _draw (`visible = Sim.game_on`) : un Control caché ne
## redessine JAMAIS → masqué une fois au menu, le ledger ne se remontrait jamais (il
## n'apparaissait dans AUCUNE capture). Pilotée ici, à la frame — trivial et robuste.
func _process(_d: float) -> void:
	if visible != Sim.game_on:
		visible = Sim.game_on
		if visible:
			queue_redraw()

var _maxh := 600.0          ## hauteur DISPONIBLE (bande topbar→bas) — le panneau s'y borne
                            ## mais se DÉCOUPE au contenu (retour joueur : « adapte la taille »)

func _layout() -> void:
	var vp := get_viewport_rect().size
	var w := HANDLE_W if _collapsed else W
	_maxh = maxf(140.0, vp.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H)
	position = Vector2(vp.x - w, Frame.TOPBAR_H)
	size = Vector2(w, _maxh)   # plein cadre au layout ; le _draw re-latche à son contenu

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _handle_rect.has_point(e.position):
			_collapsed = not _collapsed
			_layout()
			queue_redraw()
			accept_event()
			return
		if not _collapsed and _refill_rect.size.x > 0 and _refill_rect.has_point(e.position):
			if Sim.world != null and Sim.world.has_method("player_refill"):
				Sim.world.player_refill()
				Sound.play("ui_click")
				Sim.notify_action()
				queue_redraw()
			accept_event()
			return
		# PLIAGE PAR SECTION : clic sur un bandeau → replie/déplie son contenu
		if not _collapsed:
			for sr in _sec_rects:
				if (sr["rect"] as Rect2).has_point(e.position):
					var t := String(sr["title"])
					_fold[t] = not bool(_fold.get(t, false))
					Sound.play("ui_click")
					queue_redraw()
					accept_event()
					return

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
	if Sim.world == null:
		return
	var w = Sim.world
	var me: int = w.player()
	# RABAT (planche 23) : languette au bord gauche, à mi-hauteur — la flèche pointe
	# le sens du geste (chevron droit 02 = replier vers le bord · gauche 01 = rouvrir).
	_handle_rect = Rect2(0, size.y * 0.5 - 22, HANDLE_W, 44)
	if _collapsed:
		VKit.fill(self, Rect2(0, 0, HANDLE_W, size.y), Color(VKit.COL_PANEL.r, VKit.COL_PANEL.g, VKit.COL_PANEL.b, 0.90))
		VKit.fill(self, Rect2(0, 0, 2, size.y), VKit.COL_GOLD)
		var hd: Texture2D = UIKit.parch_tex("sheet23_remaining_chrome_sidebar_01")
		if hd != null:
			draw_texture_rect(hd, _handle_rect, false)
		else:
			VKit.text(self, Vector2(5, 22), VKit.COL_GOLD, "«")
		return
	VKit.fill(self, Rect2(0, 0, W, size.y), Color(VKit.COL_PANEL.r, VKit.COL_PANEL.g, VKit.COL_PANEL.b, 0.94))
	VKit.fill(self, Rect2(0, 0, 2, size.y), VKit.COL_GOLD)
	VKit.fill(self, Rect2(0, size.y - 2.0, W, 2), VKit.COL_GOLD)   # le panneau se FERME au contenu
	var hd2: Texture2D = UIKit.parch_tex("sheet23_remaining_chrome_sidebar_02")
	if hd2 != null:
		draw_texture_rect(hd2, Rect2(_handle_rect.position - Vector2(2, 0),
			_handle_rect.size + Vector2(4, 0)), false, Color(1, 1, 1, 0.70))
	var x := 12.0
	var y := 10.0
	_sec_rects.clear()

	# ── VILLES : régions habitées du joueur, triées par âmes ──
	var cities := []
	for r in range(w.region_count()):
		if int(w.region_owner(r)) != me:
			continue
		var p: int = int(w.region_pop(r))
		if p >= 150:
			cities.append([p, r])
	cities.sort_custom(func(a, b): return a[0] > b[0])
	y = _lsection(x, y, "VILLES", Color(0.78, 0.62, 0.30), str(cities.size()))
	if not _folded("VILLES"):
		var shown := 0
		for cd in cities:
			if shown >= 10:
				break
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, _region_name(cd[1]))
			var pg := _grp(cd[0])
			VKit.text(self, Vector2(W - 14.0 - VKit.text_w(pg), y), VKit.COL_DIM, pg)
			y += 18
			shown += 1
		if cities.size() > shown:
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "… et %d autres" % (cities.size() - shown))
			y += 18
		if cities.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune ville")
			y += 18
	y += 6

	# ── ARMÉES : l'ost de campagne + la réserve levée ──
	y = _lsection(x, y, "ARMÉES", Color(0.66, 0.22, 0.18), "")
	_refill_rect = Rect2()   # zone morte quand la section est repliée
	if not _folded("ARMÉES"):
		var ca: Dictionary = w.country_army(me) if w.has_method("country_army") else {}
		var ai: Dictionary = w.army_info(me)
		if bool(ai.get("active", false)):
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH,
				"En campagne : %s (%s)" % [_grp(int(ai.get("units", 0))), String(ai.get("phase", ""))])
			y += 18
		var res_n := int(ca.get("regiments", 0))
		VKit.text(self, Vector2(x, y), VKit.COL_PARCH if res_n > 0 else VKit.COL_DIM,
			"Réserve : %s · levée %s" % [_grp(res_n), String(ca.get("levy_name", "—"))])
		y += 18
		# RECOMPLÉTER (retour joueur : « doit être dans la side bar droite ») — verbe journalisé
		_refill_rect = Rect2(x, y, 104, 20)
		VKit.fill(self, _refill_rect, VKit.COL_PANEL2)
		VKit.box(self, _refill_rect, VKit.COL_GOLD)
		VKit.text(self, Vector2(x + 8, y + 3), VKit.COL_PARCH, "Recompléter", VKit.FS_SMALL)
		y += 26
		var fl := int(ca.get("fleet", 0))
		if fl > 0:
			# nef de guerre gravée (planche 24) devant la ligne de flotte
			var bt: Texture2D = UIKit.parch_tex("sheet24_topbar_boats_menu_11")
			if bt != null:
				draw_texture_rect(bt, Rect2(x - 2, y - 3, 18, 18), false)
			VKit.text(self, Vector2(x + (20 if bt != null else 0), y), VKit.COL_PARCH,
				"Flotte : %d coque(s) disponibles" % fl)
			y += 16
	y += 6

	# ── COLONISATION : le chantier qui mûrit / la cadence ──
	if w.has_method("colony_status"):
		y = _lsection(x, y, "COLONISATION", Color(0.45, 0.62, 0.32), "")
		if not _folded("COLONISATION"):
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

	# (COUR & FACTIONS a DÉMÉNAGÉ en TOPBAR — retour joueur 2026-07-10 : « les factions
	#  doivent être en top bar », doctrine national = topbar. Bonheur/classes/blasons/
	#  tension de coup y vivent en cellules + hovers ; l'influence y était déjà.)

	# ── MISSION décennale : le texte + la récompense promise ──
	if w.has_method("mission_info"):
		var mi: Dictionary = w.mission_info(me)
		if bool(mi.get("active", false)):
			y = _lsection(x, y, "MISSION", Color(0.38, 0.52, 0.66), "")
			if _folded("MISSION"):
				y += 2
				mi = {}   # court-circuite le corps (le bloc suivant lit mi vide)
		if bool(mi.get("active", false)):
			var mtxt := String(mi.get("text", ""))
			# coupe en 2 lignes max à la largeur de la bande
			while VKit.text_w(mtxt, VKit.FS_SMALL) > (W - 26.0) * 2.0 and mtxt.length() > 10:
				mtxt = mtxt.substr(0, mtxt.length() - 6) + "…"
			var line1 := mtxt
			var line2 := ""
			if VKit.text_w(mtxt, VKit.FS_SMALL) > W - 26.0:
				var cut := int(mtxt.length() * (W - 26.0) / maxf(VKit.text_w(mtxt, VKit.FS_SMALL), 1.0))
				var sp := mtxt.rfind(" ", cut)
				if sp > 4:
					line1 = mtxt.substr(0, sp)
					line2 = mtxt.substr(sp + 1)
			VKit.text(self, Vector2(x, y), VKit.COL_PARCH, line1, VKit.FS_SMALL)
			y += 14
			if line2 != "":
				VKit.text(self, Vector2(x, y), VKit.COL_PARCH, line2, VKit.FS_SMALL)
				y += 14
			var rw := "récompense : %d or" % int(mi.get("reward_gold", 0))
			var mat := String(mi.get("reward_mat", ""))
			if mat != "" and float(mi.get("reward_qty", 0)) > 0.0:
				rw += " + %d %s" % [int(mi.get("reward_qty", 0)), mat]
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, rw + " (an %d)" % int(mi.get("issued_year", 0)), VKit.FS_SMALL)
			y += 17

	# ── LE LOG : le fil de notifications (persistant, le plus récent en tête) ──
	y = _lsection(x, y, "JOURNAL", Color(0.45, 0.45, 0.42), str(_log.size()) if not _log.is_empty() else "")
	if not _folded("JOURNAL"):
		if _log.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien à signaler")
			y += 16
		for e in _log:
			if y > _maxh - 20.0:   # borné à la BANDE dispo (pas à size.y, qui s'adapte)
				break
			var line := "an %d · %s" % [int(e["y"]), String(e["txt"])]
			# tronqué à la largeur (les détails vivent dans les alertes/panneaux)
			while VKit.text_w(line) > W - 26.0 and line.length() > 8:
				line = line.substr(0, line.length() - 4) + "…"
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, line, VKit.FS_SMALL)
			y += 16

	# ── DÉCOUPE AU CONTENU : le panneau s'arrête à sa dernière ligne (latch —
	# la taille converge en une frame ; l'empreinte STOP cesse de bloquer la carte
	# sous le vide, retour joueur 2026-07-10 « ADAPTER LA TAILLE DU MENU DE DROITE »).
	var want := clampf(y + 10.0, 140.0, _maxh)
	if absf(want - size.y) > 2.0:
		set_deferred("size", Vector2(size.x, want))

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

## SECTION DU LEDGER (motif outliner EU5, outliner.gui:184) : le bandeau VKit + un
## RUBAN de catégorie coloré à gauche + le COMPTE à droite + un CHEVRON de pliage —
## le bandeau entier est CLIQUABLE (replie/déplie, cf. _gui_input/_fold).
func _lsection(x: float, y: float, title: String, rib: Color, count: String) -> float:
	var y2 := VKit.section(self, x, y, title)
	VKit.fill(self, Rect2(x - 8.0, y + 3.0, 4.0, 20.0), rib)
	var chev := "▸" if bool(_fold.get(title, false)) else "▾"
	var cx := W - 16.0 - VKit.text_w(chev, VKit.FS_SMALL)
	VKit.text(self, Vector2(cx, y + 5.0), VKit.COL_DIM, chev, VKit.FS_SMALL)
	if count != "":
		VKit.text(self, Vector2(cx - 8.0 - VKit.text_w(count, VKit.FS_SMALL), y + 5.0),
			VKit.COL_DIM, count, VKit.FS_SMALL)
	_sec_rects.append({"rect": Rect2(0.0, y, W, 26.0), "title": title})
	return y2

## la section est-elle repliée ? (helper de lisibilité des blocs de _draw)
func _folded(title: String) -> bool:
	return bool(_fold.get(title, false))

## HOVER des bandeaux — politique joueur : nom, raccourci, FACTUEL (pas de leçon ;
## les mots turquoise portent les définitions via la cascade).
const SEC_TIPS := {
	"VILLES": "Vos régions habitées, triées par âmes.",
	"ARMÉES": "Réserve levée et ost de campagne. Recompléter paie or et matière.",
	"COLONISATION": "Le chantier de Colonisation en cours et son avancement.",
	"MISSION": "La mission décennale et sa récompense.",
	"JOURNAL": "Le fil des évènements passés. Clic sur un bandeau : replier.",
}

func _get_tooltip(at_position: Vector2) -> String:
	if _collapsed:
		return ""
	for sr in _sec_rects:
		if (sr["rect"] as Rect2).has_point(at_position):
			return String(SEC_TIPS.get(String(sr["title"]), ""))
	return ""
