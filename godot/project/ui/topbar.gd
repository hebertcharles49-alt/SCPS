extends Control
## Topbar — bandeau PLEINE LARGEUR (cadre d'écran) : capsule de date (An N) + le
## roll-up du PAYS JOUÉ (nom · or · pop empire · régions · savoir) à gauche, contrôle
## de VITESSE cliquable à DROITE. Suit la largeur de la fenêtre (size_changed).
## Display-only sauf le verbe vitesse. Lit Sim.

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const H := Frame.TOPBAR_H

## RESSOURCES RAW DE BASE affichées dans le bandeau (retour joueur) — indices d'enum
## Resource MIROIR de scps_types.h : bois · argile · pierre · fer · armes (la nourriture
## a déjà son propre chip via country_food). Ordre d'affichage = ordre du panier de bâti.
const TOPBAR_RAWS := [9, 24, 25, 13, 36]   # RES_WOOD · RES_CLAY · RES_STONE · RES_IRON · RES_ARMS
const RAW_NAMES := {9: "Bois", 24: "Argile", 25: "Pierre", 13: "Fer", 36: "Armes"}   # hover : le NOM même à stock 0

signal tech_requested

var _speed_rect := Rect2()
var _speed_btns := []   ## boutons de vitesse DISCRETS façon RimWorld : [[Rect2, index], …]
var _savoir_rect := Rect2()
var _age_rect := Rect2()   # §7 : chip « Âge levé — Engager » (vide quand rien à engager)

## DELTAS MENSUELS (rendu attendu EU4 : « 12 125 +45 » vert/rouge) — photo du mois
## précédent, prise à chaque month_ticked (display-only, aucun état moteur).
var _last_gold := 0.0
var _last_pop := 0
var _d_gold := 0.0
var _d_pop := 0

func _delta_txt(d: float) -> String:
	if absf(d) < 0.5:
		return ""
	return "%+d" % int(round(d))

## CELLULE DE RESSOURCE façon CK3 (hud.gui:6148-6207) : icône 22 px à gauche, VALEUR
## empilée sur son DELTA (vert si ≥0, rouge sinon), séparateur vertical léger. `icon` =
## pièce du pack d'icônes OU `rid` ≥ 0 = sprite de ressource. `tip` = le HOVER (retour
## joueur : « un explicatif sur chaque display ») ; `vcol.a > 0` teinte la valeur.
func _cell(px: float, icon: String, rid_or_val, val: String, dtxt: String, dpos: bool,
		tip: String = "", vcol: Color = Color(0, 0, 0, 0)) -> float:
	var rid := -1
	if icon == "" and typeof(rid_or_val) == TYPE_INT:
		rid = int(rid_or_val)
	if rid >= 0:
		var spr: Texture2D = UIKit.resource_sprite(rid, "")
		if spr != null:
			draw_texture_rect(spr, Rect2(px, (H - 22.0) * 0.5, 22, 22), false)
	elif icon != "":
		UIKit.draw_icon(self, icon, Vector2(px, (H - 22.0) * 0.5), 22)
	var tx := px + 26.0
	VKit.text(self, Vector2(tx, 6.0), vcol if vcol.a > 0.0 else VKit.COL_PARCH, val)
	var wv := VKit.text_w(val)
	var wd := 0.0
	if dtxt != "":
		VKit.text(self, Vector2(tx, 26.0), VKit.sense(0.85) if dpos else VKit.sense(0.12), dtxt, VKit.FS_SMALL)
		wd = VKit.text_w(dtxt, VKit.FS_SMALL)
	var cw := 26.0 + maxf(wv, wd) + 10.0
	if tip != "":
		_tips.append([Rect2(px - 4.0, 0.0, cw + 8.0, H), tip])
	VKit.fill(self, Rect2(px + cw, 9.0, 1.0, H - 20.0),
		Color(VKit.COL_GOLD.r, VKit.COL_GOLD.g, VKit.COL_GOLD.b, 0.22))
	return px + cw + 10.0

var _tips: Array = []   ## [[Rect2, texte], …] — reconstruit au _draw, hit-testé au survol

func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""

var _date: Control = null   ## la date, contrôle ENFANT à cadence QUOTIDIENNE (cf. date_chip.gd)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # la barre capte ses clics (pas la carte dessous)
	_date = load("res://ui/date_chip.gd").new()
	add_child(_date)
	_resize()
	get_viewport().size_changed.connect(_resize)
	Sim.generated.connect(_on_change)
	Sim.month_ticked.connect(_on_tick)   # les chiffres (or·pop·savoir·food) s'updatent au MOIS

func _resize() -> void:
	position = Vector2.ZERO
	size = Vector2(get_viewport_rect().size.x, H)
	if _date != null:
		var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
		_date.position = Vector2(size.x - 116.0 - dtw - 18.0, 0)
		_date.size = Vector2(dtw + 8.0, H)
	queue_redraw()

func _on_tick(_year: int) -> void:
	# photo mensuelle → deltas or/pop (rendu attendu EU4)
	var w = Sim.world
	if w != null and Sim.game_on:
		var me: int = w.player()
		var ci: Dictionary = w.country_info(me)
		if bool(ci.get("valide", false)):
			var g := float(ci["or"])
			var p := int(ci["pop"])
			_d_gold = g - _last_gold
			_d_pop = p - _last_pop
			_last_gold = g
			_last_pop = p
	queue_redraw()

func _on_change() -> void:
	queue_redraw()

func _draw() -> void:
	var ww := size.x
	# barre PLEINE LARGEUR : cuir sombre + liseré or en bas (cadre franc, pas arrondi)
	VKit.fill(self, Rect2(0, 0, ww, H), VKit.COL_PANEL)
	VKit.fill(self, Rect2(0, H - 2, ww, 2), VKit.COL_GOLD)
	var cy := (H - 18.0) * 0.5     # centrage vertical du contenu

	if Sim.world == null:
		VKit.text(self, Vector2(16, cy), VKit.COL_DIM, "(libscps absente — voir README)")
		return
	if not Sim.world.has_method("province_at"):
		VKit.text(self, Vector2(12, cy), VKit.sense(0.5),
			"⚠ libscps OBSOLÈTE — rebâtir : scons platform=windows use_mingw=yes")
		return

	var w = Sim.world
	_tips.clear()
	# (la capsule de chrome à gauche est RETIRÉE — panneaux plats, retour joueur 2026-07-10)

	# LE PAYS JOUÉ — CELLULES façon CK3 (hud.gui : icône + VALEUR empilée sur son DELTA
	# signé vert/rouge + séparateur vertical léger). Le delta = mensuel (or·pop) ou
	# net/jour (stocks, ScpsStock.net_day) : la barre dit l'état ET la tendance.
	var me: int = w.player()
	var ci: Dictionary = w.country_info(me)
	var px := 16.0   # (la capsule de chrome qui occupait x=10..102 est retirée)
	if bool(ci.get("valide", false)):
		# les ARMES du joueur (héraldique dérivée) — repli couronne si pièces absentes
		var parms: Texture2D = load("res://ui/heraldry.gd").arms(me)
		if parms != null:
			draw_texture_rect(parms, Rect2(px - 3, (H - 30.0) * 0.5, 30, 30), false)
		else:
			UIKit.draw_icon(self, "politics_crown", Vector2(px, cy - 2), 18)
		px += 30
		var nom := String(ci["nom"])
		VKit.text(self, Vector2(px, cy), VKit.COL_GOLD, nom); px += VKit.text_w(nom) + 18
		px = _cell(px, "fine_coin", "", _grp(ci["or"]), _delta_txt(_d_gold), _d_gold >= 0.0,
			"Trésor royal (solde mensuel)")
		px = _cell(px, "population_group", "", _grp(ci["pop"]), _delta_txt(float(_d_pop)), _d_pop >= 0,
			"Âmes de l'empire")
		# modèle province (EU4) : le pays compte ses PROVINCES (pas ses régions)
		var rt := "%d prov." % w.country_province_count(me)
		_tips.append([Rect2(px - 4.0, 0.0, VKit.text_w(rt) + 8.0, H), "Provinces colonisées"])
		VKit.text(self, Vector2(px, cy), VKit.COL_DIM, rt); px += VKit.text_w(rt) + 14
		var CPTips: Dictionary = load("res://ui/country_panel.gd").TIPS
		var sx0 := px
		px = _cell(px, "fine_knowledge", "", "%d" % int(ci["savoir"]), "", true,
			String(CPTips.get("savoir", "")) + " (clic : l'arbre de technologie)")
		_savoir_rect = Rect2(sx0 - 4, 0, px - sx0, H)
		# NOURRITURE (v50) : Σ stock vivrier de l'empire — la réserve en rations
		if w.has_method("country_food"):
			px = _cell(px, "fine_grain", "", _grp(int(w.country_food(me))), "", true,
				"Réserve vivrière (rations)")
		# RESSOURCES RAW DE BASE : bois · argile · pierre · fer · armes — stock + net/jour
		if w.has_method("country_stocks"):
			var smap := {}
			var nmap := {}
			var rnames := {}
			for st in w.country_stocks(me):
				smap[int(st["res_id"])] = int(st["stock"])
				nmap[int(st["res_id"])] = float(st.get("net_day", 0.0))
				rnames[int(st["res_id"])] = String(st.get("name", ""))
			for rid in TOPBAR_RAWS:
				var net := float(nmap.get(rid, 0.0))
				var dtx := ("%+.1f" % net) if absf(net) >= 0.05 else ""
				var rnm := String(rnames.get(rid, RAW_NAMES.get(rid, "Ressource")))
				px = _cell(px, "", rid, _grp(smap.get(rid, 0)), dtx, net >= 0.0,
					"%s — stock national" % rnm)
		# ── JAUGES NATIONALES (doctrine joueur : « national = topbar ») : l'état du royaume
		#    en un coup d'œil, valeur 0-100 colorée par sens ; le POURQUOI vit au survol.
		for gk in [["stabilite", "stability_shield"], ["prosperite", "prosperity_sprout"],
			["legitimite", "politics_crown"], ["cohesion", "happiness_medallion"]]:
			var gv := int(ci.get(gk[0], 0))
			px = _cell(px, gk[1], "", str(gv), "", true,
				String(CPTips.get(gk[0], "")), VKit.sense(clampf(gv / 100.0, 0.0, 1.0)))
		var infl := int(ci.get("influence", 0))
		px = _cell(px, "politics_law", "", str(infl), "", true,
			"Influence — réputation diplomatique (offres, alliances, ligues)")
		# CHANTIER DE COLONISATION (v50) : la colonie qui mûrit
		if w.has_method("colony_status"):
			var cs: Dictionary = w.colony_status()
			if bool(cs.get("active", false)):
				var tot := maxi(1, int(cs.get("total_days", 1)))
				var pct := int(round(100.0 * float(tot - int(cs.get("days_left", 0))) / float(tot)))
				var ctxt := "Colonie %d %%" % pct
				UIKit.draw_icon(self, "settlement_cluster", Vector2(px, cy - 2), 16); px += 20
				VKit.text(self, Vector2(px, cy), Color(0.62, 0.78, 0.52), ctxt); px += VKit.text_w(ctxt) + 20

		# ── BONHEUR + FACTIONS (retour joueur 2026-07-10 : « les factions doivent être
		#    en top bar » — doctrine national = topbar). Bonheur = satisfaction pondérée
		#    des classes (détail au survol) ; puis un BLASON par faction, mini-barre
		#    d'ADHÉSION dessous, ★ dominante, rancœur en liseré rouge — détail au survol.
		var dmt: Dictionary = w.country_demo(me) if w.has_method("country_demo") else {}
		var clst: Array = dmt.get("classes", [])
		if clst.size() >= 3:
			var sat_avg := 0.0
			var wsum := 0.0
			for cl in clst:
				var p := float(cl.get("pop", 0))
				sat_avg += float(cl.get("satisfaction", 0)) * p
				wsum += p
			sat_avg = sat_avg / maxf(wsum, 1.0)
			px = _cell(px, "happiness_medallion", "", "%d %%" % int(round(sat_avg)), "", true,
				"Bonheur — satisfaction pondérée du peuple\nLaboureurs %d · Artisans %d · Noblesse %d" % [
					int(clst[0].get("satisfaction", 0)), int(clst[1].get("satisfaction", 0)),
					int(clst[2].get("satisfaction", 0))],
				VKit.sense(clampf(sat_avg / 100.0, 0.0, 1.0)))
		if w.has_method("country_factions"):
			var fx: Dictionary = w.country_factions(me)
			var coup := int(fx.get("coup", 0))
			var cor := int(fx.get("corruption", 0))
			var court := "Tension de coup %d · corruption %d" % [coup, cor]
			for fe in fx.get("list", []):
				var fnm := String(fe.get("name", ""))
				var part := int(fe.get("part", 0))
				var dom := bool(fe.get("dominant", false))
				var g := int(fe.get("grief", 0))
				var bl: Texture2D = UIKit.faction_blason(fnm, g >= 25)
				if bl != null:
					draw_texture_rect(bl, Rect2(px, 6, 20, 20), false)
				else:
					UIKit.draw_icon(self, "politics_law", Vector2(px, 6), 20)
				if dom:
					VKit.text(self, Vector2(px + 13, -1), VKit.COL_GOLD, "★", VKit.FS_SMALL)
				# mini-barre d'ADHÉSION sous le blason (rancœur ≥25 : liseré rouge)
				VKit.fill(self, Rect2(px, 30, 20, 5), VKit.COL_PANEL2)
				VKit.fill(self, Rect2(px, 30, 20.0 * clampf(part / 100.0, 0.0, 1.0), 5),
					VKit.sense(0.15) if g >= 25 else VKit.sense(0.72))
				VKit.box(self, Rect2(px, 30, 20, 5), VKit.COL_EDGE)
				var ftip := "%s — adhésion %d %%" % [fnm, part]
				if dom:
					ftip += " (★ dominante)"
				if g > 0:
					ftip += " · rancœur %d" % g
				_tips.append([Rect2(px - 2.0, 0.0, 24.0, H), ftip + "\n" + court])
				px += 26.0
			if coup >= 20 or cor > 0:
				var wtxt := "⚑"
				VKit.text(self, Vector2(px + 2, cy), VKit.sense(0.10) if coup >= 45 else VKit.sense(0.35), wtxt)
				_tips.append([Rect2(px - 2.0, 0.0, VKit.text_w(wtxt) + 10.0, H), court])
				px += VKit.text_w(wtxt) + 12.0

	# LA DATE vit dans son PROPRE contrôle (_date, rafraîchi CHAQUE JOUR — le topbar,
	# lui, reste à la cadence mensuelle anti-danse : sans ça le compteur sautait par
	# paquets de 8-9 jours entre deux redraws, retour joueur 2026-07-10). On ne fait
	# ici que RÉSERVER sa place (largeur max fixe) pour ancrer le chip d'âge.
	var dtw := VKit.text_w("Jour 30 · mois 12 · an 9999")
	var dtx0 := ww - 116.0 - dtw - 18.0

	# §7 — ENGAGEMENT D'ÂGE : un âge s'est levé et le joueur ne l'a pas engagé →
	# chip ambre cliquable (l'IA s'engage auto ; le joueur choisit — verbe CMD_AGE_ENGAGE).
	# Chip PLAT (les bandes parchemin sont retirées — panneaux plats 2026-07-10).
	_age_rect = Rect2()
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			var lab := "Engager : %s" % String(ag.get("name", ""))
			var aw := VKit.text_w(lab) + 34.0
			_age_rect = Rect2(dtx0 - aw - 14.0, 6, aw, H - 12)
			VKit.fill(self, _age_rect, Color(0.24, 0.17, 0.07, 0.95))
			VKit.box(self, _age_rect, Color(0.90, 0.72, 0.34))
			UIKit.draw_icon(self, "fine_age", Vector2(_age_rect.position.x + 6, cy - 2), 16)
			VKit.text(self, Vector2(_age_rect.position.x + 26, cy), Color(0.90, 0.72, 0.34), lab)

	# RUBAN PAUSE (rendu attendu EU4) : le monde figé se DIT, pas juste un glyphe dans
	# le coin. ANCRÉ sous les contrôles de vitesse (bord droit) — c'est là que l'œil
	# va quand le temps est en cause ; la barre d'entropie garde le centre-haut.
	if Sim.game_on and Sim.speed_index == 0:
		var prw := 128.0
		# à GAUCHE du ledger (bande droite, dessinée après nous → elle couvrirait)
		var prr := Rect2(ww - Frame.LEDGER_W - prw - 12.0, H + 6.0, prw, 26.0)
		VKit.fill(self, prr, Color(0.38, 0.08, 0.07, 0.94))
		VKit.box(self, prr, Color(0.78, 0.62, 0.30))
		var ptxt := "Pause"
		VKit.text(self, Vector2(prr.position.x + (prw - VKit.text_w(ptxt)) * 0.5, prr.position.y + 4),
			Color(0.94, 0.88, 0.74), ptxt)

	# CONTRÔLE DE VITESSE façon RimWorld (TimeControls) : 4 boutons DISCRETS — l'état se
	# VOIT et on clique CE qu'on veut, plus de cycle aveugle. Espace bascule toujours.
	_speed_btns.clear()
	var sbw := 27.0
	var sx := ww - 8.0 - 4.0 * sbw
	_speed_rect = Rect2(sx, 6, 4.0 * sbw, H - 12)   # (gardé : zone de hit globale)
	var glyphs := ["❙❙", "▶", "▶▶", "▶▶▶"]
	var stips := ["Pause (Espace)", "Vitesse lente", "Vitesse normale", "Vitesse rapide"]
	for i in range(4):
		var r := Rect2(sx + float(i) * sbw, 6, sbw - 3.0, H - 12)
		var active := (Sim.speed_index == i)
		VKit.fill(self, r, VKit.COL_GOLD if active else VKit.COL_PANEL2)
		VKit.box(self, r, VKit.COL_EDGE)
		var g: String = glyphs[i]
		VKit.text(self, Vector2(r.position.x + (r.size.x - VKit.text_w(g, VKit.FS_SMALL)) * 0.5,
			r.position.y + (r.size.y - 16.0) * 0.5), VKit.COL_PANEL if active else VKit.COL_PARCH, g, VKit.FS_SMALL)
		_speed_btns.append([r, i])
		_tips.append([r, String(stips[i])])

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if _savoir_rect.has_point(event.position):
			tech_requested.emit()
		elif _age_rect.size.x > 0 and _age_rect.has_point(event.position):
			if Sim.world != null and Sim.world.has_method("player_age_engage"):
				Sim.world.player_age_engage()   # enfilé ; le chip s'éteint au drain (engaged=true)
				queue_redraw()
		elif _speed_rect.has_point(event.position):
			# boutons DISCRETS (RimWorld) : on clique LA vitesse voulue
			for sb in _speed_btns:
				if (sb[0] as Rect2).has_point(event.position):
					Sim.set_speed(int(sb[1]))
					Sound.play("ui_click")
					break
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
