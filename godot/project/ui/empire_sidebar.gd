extends Control
## EMPIRE SIDEBAR — la bande DROITE permanente (en jeu) : le RÉSUMÉ D'EMPIRE en haut
## (villes + habitants · armées · flotte · colonisation en cours) et le LOG de
## notifications en bas (le fil, persistant — détails minimes mais exhaustifs).
## Les données sont LUES de la façade ; seuls les raccourcis de navigation et les
## verbes déjà assumés par la bande (âge, renfort) sont interactifs.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

const AlertsK = preload("res://ui/alerts.gd")   # la TABLE DU FIL (FEED_KINDS) partagée

signal goto_region(region: int)
signal open_country(country: int)

const W := 288.0            ## élargie (retour joueur 2026-07-10 : « laisse respirer »)
const HANDLE_W := 14.0      ## bande réduite quand la sidebar est REPLIÉE (rabat)

var _city_names := {}       ## region → nom (cache, résolu via province_at(siège))
var _collapsed := false     ## rabat (pièces planche 23 : 01 replier · 02 déplier)
var _handle_rect := Rect2()
var _refill_rect := Rect2() ## chip RECOMPLÉTER (déménagé du tiroir Armée — retour joueur)
var _age_rect := Rect2()    ## encart d'ÂGE en haut de la bande (déménagé de la topbar,
var _age_engageable := false ## retour joueur 2026-07-11 : « sous le temps, au-dessus du menu »)
var _age_fx := ""            ## bonus/contraintes de l'âge courant (hover du chip nominatif)
var _fold := {}             ## titre de section → replié (retour joueur 2026-07-10 :
                            ## « tous les menus de droite doivent pouvoir se collapser »)
var _sec_rects := []        ## [{rect, title}] bandeaux cliquables (reconstruit au _draw)
var _journal_rects := []    ## [{rect, data}] lignes du JOURNAL (clic = même action que l'origine)
var _war_rects := []        ## [{rect, country}] guerres actives
var _notif_rects := []      ## [{rect, data}] notifications actives
var _alerts_source: Control
var _scrolloff := 0.0
var _maxscroll := 0.0

func set_alert_source(source: Control) -> void:
	_alerts_source = source
	if source != null and source.has_signal("ledger_changed"):
		source.connect("ledger_changed", Callable(self, "queue_redraw"))
	queue_redraw()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # lecture seule : la carte reste cliquable au travers ? Non — bande opaque
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	_layout()
	get_viewport().size_changed.connect(_layout)
	Sim.generated.connect(func(): _city_names.clear(); queue_redraw())
	Sim.month_ticked.connect(func(_y): queue_redraw())   # résumé d'empire : cadence mensuelle (le
		# JOURNAL lui-même est tenu par alerts.gd, rafraîchi via ledger_changed — set_alert_source)

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
	# REPLIÉ : le rabat occupe la bande entière. DÉPLIÉ : la hauteur se DÉCOUPE au
	# contenu (latchée dans _draw) — plus de « brique » pleine hauteur quand rien ne se
	# passe ; le panneau grandit avec les guerres/notifications, plafonné à _maxh.
	if _collapsed:
		size = Vector2(w, _maxh)
	else:
		size = Vector2(w, clampf(size.y, 140.0, _maxh))

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and not _collapsed:
		if e.button_index == MOUSE_BUTTON_WHEEL_UP or e.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			var direction := -1.0 if e.button_index == MOUSE_BUTTON_WHEEL_UP else 1.0
			_scrolloff = clampf(_scrolloff + direction * 52.0, 0.0, _maxscroll)
			queue_redraw()
			accept_event()
			return
		# Les notifications acceptent gauche ET droite (agir / acquitter).
		var np: Vector2 = e.position + Vector2(0.0, _scrolloff)
		for nr in _notif_rects:
			if (nr["rect"] as Rect2).has_point(np):
				if _alerts_source != null and _alerts_source.has_method("activate_ledger"):
					_alerts_source.call("activate_ledger", nr["data"], e.button_index)
				accept_event()
				return
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _handle_rect.has_point(e.position):
			_collapsed = not _collapsed
			_layout()
			queue_redraw()
			accept_event()
			return
		# ENCART D'ÂGE : clic sur « Engager » → verbe CMD_AGE_ENGAGE (enfilé, déterministe)
		var cp: Vector2 = e.position + Vector2(0.0, _scrolloff)
		if not _collapsed and _age_engageable and _age_rect.size.x > 0 and _age_rect.has_point(cp):
			if Sim.world != null and Sim.world.has_method("player_age_engage"):
				Sim.world.player_age_engage()
				Sound.play("ui_click")
				Sim.notify_action()
				queue_redraw()
			accept_event()
			return
		if not _collapsed and _refill_rect.size.x > 0 and _refill_rect.has_point(cp):
			if Sim.world != null and Sim.world.has_method("player_refill"):
				Sim.world.player_refill()
				Sound.play("ui_click")
				Sim.notify_action()
				queue_redraw()
			accept_event()
			return
		# JOURNAL : le clic route la MÊME action que la notification d'origine (centrer
		# la carte / ouvrir le panneau concerné) via alerts.gd (résolution UNIQUE,
		# cf. activate_journal) — jamais reconstruite ici.
		if not _collapsed:
			for wr in _war_rects:
				if (wr["rect"] as Rect2).has_point(cp):
					open_country.emit(int(wr["country"]))
					Sound.play("ui_click")
					accept_event()
					return
			for jr in _journal_rects:
				if (jr["rect"] as Rect2).has_point(cp):
					if _alerts_source != null and _alerts_source.has_method("activate_journal"):
						_alerts_source.call("activate_journal", jr["data"], e.button_index)
					accept_event()
					return
		# PLIAGE PAR SECTION : clic sur un bandeau → replie/déplie son contenu
		if not _collapsed:
			for sr in _sec_rects:
				if (sr["rect"] as Rect2).has_point(cp):
					var t := String(sr["title"])
					_fold[t] = not bool(_fold.get(t, false))
					Sound.play("ui_click")
					queue_redraw()
					accept_event()
					return

## RÉSOLUTION DES NOMS DE RÉGION (cache) — utilisée UNIQUEMENT pour l'affichage :
## les entrées du JOURNAL (alerts.gd, résolution des textes/couleurs/actions) portent
## un `region` id + un `tip` qui contient encore « région <N> » (numérique, le seul
## format que le moteur connaît) ; on y substitue ICI le nom lisible, en aval de la
## résolution — cosmétique de rendu, pas une seconde logique de résolution.
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

## le texte COMPLET d'une entrée de journal, région substituée en nom lisible quand
## elle est connue (« région 42 » → « Marbrive ») — utilisé pour la ligne ET le hover.
## L'an vit déjà en préfixe de ligne (« an %d · … ») : le « (an %d) » du gabarit
## moteur (FEED_KINDS, cf. alerts.gd) est donc retiré ici (redondance d'affichage
## seulement — le tip d'origine, lui, n'est jamais réécrit).
## UI-POLISH #10 : « la région %d »/« La région %d » portait un article FIGÉ (féminin,
## accordé sur « région ») qui ne convient plus une fois substitué par un nom de
## PROVINCE au genre propre (ex. « la Désert Brûlant » — désert est masculin en
## français). On retire l'article ET le mot « région » ensemble : le nom propre se
## suffit à lui-même (« Désert Brûlant ne mange qu'à 45 % » — pas d'article requis
## devant un nom propre, comme pour toute ville). Le repli nu « région %d » (ex. dans
## « notre région %d », genre-neutre) reste couvert en dernier.
func _journal_full_text(entry: Dictionary, region: int) -> String:
	var tip := String(entry.get("tip", ""))
	if region >= 0:
		var nm := _region_name(region)
		tip = tip.replace("la région %d" % region, nm)
		tip = tip.replace("La région %d" % region, nm)
		tip = tip.replace("région %d" % region, nm)
	tip = tip.replace(" (an %d)" % int(entry.get("year", 0)), "")
	return tip

func _draw() -> void:
	_journal_rects.clear()
	_war_rects.clear()
	_notif_rects.clear()
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
	# Le chrome reste fixe ; tout le contenu du ledger défile sous la souris.
	draw_set_transform(Vector2(0.0, -_scrolloff))
	var x := 12.0
	var y := 10.0
	_sec_rects.clear()

	# ── ENCART D'ÂGE (haut de la bande, sous le bloc TEMPS de la topbar, AU-DESSUS du
	#    menu — retour joueur 2026-07-11) : « Engager : <âge> » ambre CLIQUABLE quand un
	#    âge s'est levé sans être engagé (verbe CMD_AGE_ENGAGE) ; sinon « Âge : <âge> »
	#    en contexte discret. Rien tant qu'aucun âge n'a percé (l'Aube). ──
	y = _draw_age(x, y)

	# ── ÉMISSAIRE (sous l'âge) : disponibilité · temps de retour · objectif du dernier
	#    envoi diplomatique (retour joueur 2026-07-12) ──
	y = _draw_emissary(x, y)

	# ── GUERRES : un conflit = une ligne, score signé du point de vue joueur. ──
	y = _draw_wars(x, y, w, me)

	# ── NOTIFICATIONS ACTIVES : les anciennes letters flottantes vivent ici. ──
	y = _draw_notifications(x, y)

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
			VKit.value(self, Vector2(W - 14.0 - VKit.text_w(pg), y), pg)
			y += 16
			shown += 1
		if cities.size() > shown:
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "… et %d autres" % (cities.size() - shown))
			y += 16
		if cities.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune ville")
			y += 16
	y += 3

	# ── ARMÉES : l'ost de campagne + la réserve levée ──
	y = _lsection(x, y, "ARMÉES", Color(0.66, 0.22, 0.18), "")
	_refill_rect = Rect2()   # zone morte quand la section est repliée
	if not _folded("ARMÉES"):
		var ca: Dictionary = w.country_army(me) if w.has_method("country_army") else {}
		var ai: Dictionary = w.army_info(me)
		if bool(ai.get("active", false)):
			var camp_lbl_w: float = VKit.detail(self, Vector2(x, y), "En campagne : ", VKit.FS)
			var camp_val_w: float = VKit.value(self, Vector2(x + camp_lbl_w, y), _grp(int(ai.get("units", 0))), VKit.FS)
			VKit.detail(self, Vector2(x + camp_lbl_w + camp_val_w, y), " (%s)" % String(ai.get("phase", "")), VKit.FS)
			y += 16
		var res_n := int(ca.get("regiments", 0))
		var res_lbl_w: float = VKit.detail(self, Vector2(x, y), "Réserve : ", VKit.FS)
		if res_n > 0:
			VKit.value(self, Vector2(x + res_lbl_w, y), _grp(res_n), VKit.FS)
		else:
			VKit.detail(self, Vector2(x + res_lbl_w, y), "0", VKit.FS)
		y += 16
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
	y += 3

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
		y += 3

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
			var rw := "%d or" % int(mi.get("reward_gold", 0))
			var mat := String(mi.get("reward_mat", ""))
			if mat != "" and float(mi.get("reward_qty", 0)) > 0.0:
				rw += " + %d %s" % [int(mi.get("reward_qty", 0)), mat]
			var rw_lbl_w: float = VKit.detail(self, Vector2(x, y), "récompense : ", VKit.FS_SMALL)
			var rw_val_w: float = VKit.value(self, Vector2(x + rw_lbl_w, y), rw, VKit.FS_SMALL)
			VKit.detail(self, Vector2(x + rw_lbl_w + rw_val_w, y), " (an %d)" % int(mi.get("issued_year", 0)), VKit.FS_SMALL)
			y += 17

	# ── LE JOURNAL : TOUTE notification colorée un jour apparue (guerres, batailles,
	#    révoltes, sécessions, évènements du directeur, conditions de conseil/armée/
	#    marché/foi/tech…) — persistant (ring ~200, la plus récente en tête), MÊME
	#    SOURCE que les chips (alerts.gd::journal_rows) : aucune notification n'existe
	#    qu'en éphémère (règle joueur). Liseré + icône = la couleur d'ORIGINE conservée ;
	#    clic gauche = même action que la notification (goto/panneau) ; survol = détail
	#    complet + le nom de lieu résolu (le tip moteur ne porte que le numéro).
	var jrows: Array = _alerts_source.call("journal_rows") if _alerts_source != null and _alerts_source.has_method("journal_rows") else []
	y = _lsection(x, y, "JOURNAL", Color(0.45, 0.45, 0.42), str(jrows.size()) if not jrows.is_empty() else "")
	if not _folded("JOURNAL"):
		if jrows.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien à signaler")
			y += 16
		for e in jrows:
			var entry: Dictionary = e
			var region := int(entry.get("region", -1))
			var col: Color = entry.get("col", VKit.COL_DIM)
			var rr := Rect2(x - 2.0, y, W - 22.0, 20.0)
			VKit.fill(self, Rect2(rr.position.x, rr.position.y + 1.0, 3.0, rr.size.y - 2.0), col)
			UIKit.draw_icon(self, String(entry.get("icon", "alert_event_bell")), Vector2(x + 6.0, y + 1.0), 16)
			var full := _journal_full_text(entry, region)
			var line := "an %d · %s" % [int(entry.get("year", 0)), full]
			# tronqué à la largeur (le détail COMPLET reste dans l'infobulle native)
			while VKit.text_w(line, VKit.FS_SMALL) > W - 52.0 and line.length() > 8:
				line = line.substr(0, line.length() - 4) + "…"
			VKit.text(self, Vector2(x + 26.0, y + 2.0), VKit.COL_PARCH, line, VKit.FS_SMALL)
			_journal_rects.append({"rect": rr, "data": entry})
			y += 20.0

	# HAUTEUR ADAPTATIVE : le panneau se découpe à SON contenu (plus de brique pleine
	# hauteur). Latché ici — le bg/scroll de la frame suivante suivent la nouvelle taille.
	var content_h := clampf(y + 10.0, 140.0, _maxh)
	if not _collapsed and absf(size.y - content_h) > 2.0:
		set_deferred("size", Vector2(W, content_h))
	_maxscroll = maxf(0.0, y + 10.0 - size.y)
	_scrolloff = clampf(_scrolloff, 0.0, _maxscroll)
	draw_set_transform(Vector2.ZERO)
	if _maxscroll > 0.0:
		var track := Rect2(W - 5.0, 5.0, 2.0, size.y - 10.0)
		VKit.fill(self, track, Color(VKit.COL_DIM, 0.35))
		var thumb_h := maxf(28.0, track.size.y * size.y / (size.y + _maxscroll))
		var thumb_y := track.position.y + (track.size.y - thumb_h) * (_scrolloff / _maxscroll)
		VKit.fill(self, Rect2(track.position.x - 1.0, thumb_y, 4.0, thumb_h), VKit.COL_GOLD)

## ENCART D'ÂGE — dessiné tout en haut de la bande. NOMINATIF (décision joueur 2026-07-28 :
## « Engager » mentait, le verbe est un accusé de réception depuis le raccord 8) : le NOM
## de l'âge seul — AMBRE cliquable tant que le chapitre n'est pas lu (ouvre le récap),
## discret ensuite. Hover = les bonus/contraintes de l'âge (age_state.effects, membrane).
## Retourne le y APRÈS l'encart (0 avancement tant qu'aucun âge n'a percé).
func _draw_age(x: float, y: float) -> float:
	_age_rect = Rect2()
	_age_engageable = false
	var w = Sim.world
	if w == null or not w.has_method("age_state"):
		return y
	var ag: Dictionary = w.age_state()
	var age := int(ag.get("age", -1))
	var nm := String(ag.get("name", ""))
	_age_fx = String(ag.get("effects", ""))
	if age < 0 or nm == "":
		return y                                  # l'Aube : aucun âge levé → encart vide
	if not bool(ag.get("engaged", true)):
		# âge levé, chapitre non lu → chip AMBRE cliquable, pleine largeur
		var r := Rect2(x - 2.0, y, W - 20.0, 26.0)
		_age_engageable = true
		_age_rect = r
		VKit.fill(self, r, Color(0.24, 0.17, 0.07, 0.95))
		VKit.box(self, r, Color(0.90, 0.72, 0.34))
		UIKit.draw_icon(self, "fine_age", Vector2(r.position.x + 5, y + 2), 22)
		var lab := nm
		while VKit.text_w(lab) > r.size.x - 34.0 and lab.length() > 10:
			lab = lab.substr(0, lab.length() - 2) + "…"
		VKit.text(self, Vector2(r.position.x + 32, y + 5), Color(0.90, 0.72, 0.34), lab)
		return y + 32.0
	# chapitre lu → ligne discrète (l'ère où l'on vit) — même hover
	_age_rect = Rect2(x - 2.0, y, W - 20.0, 20.0)
	UIKit.draw_icon(self, "fine_age", Vector2(x, y), 22)
	VKit.text(self, Vector2(x + 28.0, y + 3), Color(0.72, 0.60, 0.36), nm, VKit.FS_SMALL)
	return y + 20.0

## ÉMISSAIRE — disponibilité · retour · objectif. Le moteur ne stocke que le cooldown
## (diplo_cd) ; l'objectif vient de Sim.emissary_objective (posé au verbe diplo joueur).
## Disponible = ligne verte discrète ; en tournée = ambre + retour + objectif.
func _draw_emissary(x: float, y: float) -> float:
	var w = Sim.world
	if w == null or not w.has_method("diplo_cd"):
		return y
	var cd := int(w.diplo_cd())
	UIKit.draw_icon(self, "menu_diplomacy", Vector2(x, y), 22)
	if cd <= 0:
		VKit.text(self, Vector2(x + 28.0, y + 3), Color(0.52, 0.72, 0.48),
			"Émissaire : disponible", VKit.FS_SMALL)
		return y + 20.0
	VKit.text(self, Vector2(x + 28.0, y + 3), Color(0.82, 0.66, 0.36),
		"Émissaire : retour dans %d j" % cd, VKit.FS_SMALL)
	y += 20.0
	var obj := String(Sim.emissary_objective)
	if obj != "":
		var lab := "Objectif : %s" % obj
		while VKit.text_w(lab, VKit.FS_SMALL) > W - x - 12.0 and lab.length() > 12:
			lab = lab.substr(0, lab.length() - 2) + "…"
		VKit.text(self, Vector2(x + 28.0, y + 1), VKit.COL_DIM, lab, VKit.FS_SMALL)
		y += 16.0
	return y + 4.0

static func war_score_text(score: float) -> String:
	# Affichage brut : les valeurs négatives sont de l'information, jamais une erreur.
	return "%+.0f" % score

func _draw_wars(x: float, y: float, w, me: int) -> float:
	var wars := []
	for rel in w.country_relations(me):
		if bool(rel.get("at_war", false)):
			wars.append(rel)
	y = _lsection(x, y, "GUERRES", AlertsK.COL_ARMEE, str(wars.size()))
	if not _folded("GUERRES"):
		if wars.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "aucune guerre", VKit.FS_SMALL)
			y += 16.0
		for rel in wars:
			var cid := int(rel.get("country", -1))
			var ctx: Dictionary = w.diplo_context(cid) if w.has_method("diplo_context") else {}
			var score := float(ctx.get("war_score", rel.get("war_score", 0.0)))
			var rr := Rect2(x - 2.0, y, W - 22.0, 32.0)
			VKit.list_row_bg(self, rr, _war_rects.size())
			UIKit.draw_icon(self, "dipl_rivalry", Vector2(x + 2.0, y + 3.0), 26)
			var nm := String(rel.get("name", w.country_info(cid).get("nom", "?")))
			while VKit.text_w(nm, VKit.FS_SMALL) > 142.0 and nm.length() > 8:
				nm = nm.substr(0, nm.length() - 2) + "…"
			VKit.text(self, Vector2(x + 34.0, y + 4.0), VKit.COL_PARCH, nm, VKit.FS_SMALL)
			var val := war_score_text(score)
			var scol := VKit.sense((clampf(score, -100.0, 100.0) + 100.0) / 200.0)
			VKit.text(self, Vector2(W - 14.0 - VKit.text_w(val), y + 4.0), scol, val)
			# Jauge divergente autour de zéro ; seule sa longueur est bornée, pas la valeur lue.
			var bx := x + 34.0
			var bw := W - bx - 18.0
			VKit.fill(self, Rect2(bx, y + 23.0, bw, 3.0), VKit.COL_EDGE)
			var mid := bx + bw * 0.5
			VKit.fill(self, Rect2(mid - 1.0, y + 20.0, 2.0, 9.0), VKit.COL_DIM)
			var extent := absf(clampf(score, -100.0, 100.0)) / 100.0 * bw * 0.5
			VKit.fill(self, Rect2(mid - extent if score < 0.0 else mid, y + 22.0, extent, 5.0), scol)
			_war_rects.append({"rect": rr, "country": cid, "score": score, "name": nm})
			y += 34.0
	return y + 3.0

func _draw_notifications(x: float, y: float) -> float:
	var rows: Array = _alerts_source.call("ledger_rows") if _alerts_source != null and _alerts_source.has_method("ledger_rows") else []
	y = _lsection(x, y, "NOTIFICATIONS", Color(0.70, 0.54, 0.28), str(rows.size()))
	if not _folded("NOTIFICATIONS"):
		if rows.is_empty():
			VKit.text(self, Vector2(x, y), VKit.COL_DIM, "rien en attente", VKit.FS_SMALL)
			y += 16.0
		for al in rows:
			var data: Dictionary = al
			var rr := Rect2(x - 2.0, y, W - 22.0, 34.0)
			VKit.list_row_bg(self, rr, _notif_rects.size())
			var col: Color = data.get("col", VKit.COL_GOLD)
			VKit.fill(self, Rect2(rr.position.x, rr.position.y, 3.0, rr.size.y), col)
			UIKit.draw_icon(self, String(data.get("icon", "alert_warning")), Vector2(x + 5.0, y + 4.0), 26)
			var lab := String(_alerts_source.call("ledger_short", data)) if _alerts_source.has_method("ledger_short") else String(data.get("tip", ""))
			while VKit.text_w(lab, VKit.FS_SMALL) > W - 64.0 and lab.length() > 8:
				lab = lab.substr(0, lab.length() - 2) + "…"
			VKit.text(self, Vector2(x + 38.0, y + 9.0), VKit.COL_PARCH, lab, VKit.FS_SMALL)
			if data.has("seq"):
				draw_circle(Vector2(W - 16.0, y + 7.0), 3.0, Color(0.95, 0.90, 0.75))
			_notif_rects.append({"rect": rr, "data": data})
			y += 36.0
	return y + 3.0

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
	"GUERRES": "Conflits actifs. Score signé depuis votre point de vue ; clic : ouvrir la diplomatie.",
	"NOTIFICATIONS": "Conditions et évènements actifs. Clic : agir ; clic droit : acquitter un évènement.",
	"VILLES": "Vos régions habitées, triées par âmes.",
	"ARMÉES": "Réserve levée et ost de campagne. Recompléter paie or et matière.",
	"COLONISATION": "Le chantier de Colonisation en cours et son avancement.",
	"MISSION": "La mission décennale et sa récompense.",
	"JOURNAL": "Toute notification déjà apparue, couleur d'origine conservée. Clic sur une ligne : même action que la notification ; clic sur le bandeau : replier.",
}

func _get_tooltip(at_position: Vector2) -> String:
	if _collapsed:
		return ""
	var cp := at_position + Vector2(0.0, _scrolloff)
	if _age_rect.size.x > 0 and _age_rect.has_point(cp):
		var tip := _age_fx
		if _age_engageable:
			tip += "\nClic : lire le chapitre."
		return tip.strip_edges()
	for wr in _war_rects:
		if (wr["rect"] as Rect2).has_point(cp):
			return "%s\n• Score de guerre : %s\n• Clic : ouvrir la diplomatie" % [
				String(wr.get("name", "?")), war_score_text(float(wr.get("score", 0.0)))]
	for nr in _notif_rects:
		if (nr["rect"] as Rect2).has_point(cp):
			return "Notification\n• %s" % String((nr["data"] as Dictionary).get("tip", ""))
	for jr in _journal_rects:
		if (jr["rect"] as Rect2).has_point(cp):
			var entry: Dictionary = jr["data"]
			var region := int(entry.get("region", -1))
			var full := _journal_full_text(entry, region)
			var act := String(entry.get("act", ""))
			var hint := "\nClic : y aller." if act == "goto" else \
				("\nClic : ouvrir le panneau." if act != "" else "")
			return "an %d · %s%s" % [int(entry.get("year", 0)), full, hint]
	for sr in _sec_rects:
		if (sr["rect"] as Rect2).has_point(cp):
			return String(SEC_TIPS.get(String(sr["title"]), ""))
	return ""
