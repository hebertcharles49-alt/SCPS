extends Control
## ConstructionPanel — le menu de bâti en DEUX ONGLETS (Édifices | Manufactures),
## LA VÉRITÉ ABSOLUE (retour joueur 2026-07-14) : une CARTE par bâtiment —
## Rendement (l'effet réel, delta ProvBuild) / Ressources (la recette, en icônes) /
## Prix (or + jours) / Prochain palier (edifice_succ, affiché MÊME verrouillé — un
## bâtiment tech-verrouillé n'est JAMAIS listé comme posable, seulement en tag sur
## la carte de son palier courant). Molette = défilement. Immediate-mode _draw.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

signal build_requested(kind: String, type: int)

const PADX := 12
const RH_ED := 84.0    ## carte ÉDIFICE (nom+prix / rendement / ressources / prochain palier)
const RH_MF := 58.0    ## carte MANUFACTURE (nom+prix / recette réelle)
const PW := 396.0

var _ph := 360.0       ## hauteur latchée (contenu, borné viewport — le surplus SCROLLE)
var _tab := 0          ## 0 = Édifices · 1 = Manufactures
var _tab_rects := []
var _scrolloff := 0.0
var _maxscroll := 0.0

var target_pid := -1       ## la PROVINCE visée (posée par main à l'ouverture) — les manufactures y vivent
var _builds := []
var _bytype := {}          # type(int) → b(Dictionary) — pour résoudre le « Prochain palier » (edifice_succ)
var _blegal := {}          # type → {legal, reason} — miroir read-only du drain CMD_BUILD (lot M)
var _hover_zones := []     # [{rect, head, lines}]
var _click_zones := []     # [{rect, kind, type}]
var _has_hover := false
var _close_rect := Rect2()
var _hover_rect := Rect2()
var _hover_head := ""
var _hover_lines := PackedStringArray()
var _hover_pos := Vector2.ZERO
var _flash := ""           # retour de la dernière action (chantier mis / unité levée / refus)
var _flash_ok := true

func _ready() -> void:
	size = Vector2(PW, _ph)
	custom_minimum_size = Vector2(PW, 0)
	clip_contents = true   # la liste défile SOUS le header (molette)
	mouse_filter = Control.MOUSE_FILTER_STOP
	Sim.generated.connect(_refresh)
	Sim.month_ticked.connect(func(_y): _refresh())   # ressources dispo : cadence mensuelle
	if Sim.world != null:
		_refresh()

func _refresh() -> void:
	if Sim.world == null:
		return
	var me: int = Sim.world.player()
	_builds = Sim.world.building_roster(me)
	_bytype.clear()
	for b in _builds:
		_bytype[int(b.get("type", -1))] = b
	# lot M — la LÉGALITÉ réelle (or/matière/palier, miroir du drain qui refusait en
	# silence) : rafraîchie au tick, consommée par _draw (griser) et _act (flash honnête).
	_blegal.clear()
	if Sim.world.has_method("build_legal"):
		for b in _builds:
			if bool(b.get("debloque", false)):
				var t := int(b.get("type", -1))
				_blegal[t] = Sim.world.build_legal(-1, t)
	queue_redraw()

## ouvre le panneau directement sur un onglet (0 Édifices · 1 Manufactures) — appelé
## depuis la fiche province (bouton « Construire… »).
func open_on(tab: int) -> void:
	_tab = clampi(tab, 0, 1)
	_scrolloff = 0.0
	visible = true
	_refresh()

## la raison du refus, en mot (reason de build_legal : 2 or · 3 matière · 4 tech de palier · 1 structurel)
func _reason_word(reason: int) -> String:
	match reason:
		2: return "or insuffisant"
		3: return "matière manquante"
		4: return "tech de palier manquante"
		_: return "indisponible ici (palier/déjà bâti)"

func _reason_label(result: Dictionary) -> String:
	var label := String(result.get("reason_label", ""))
	return label if label != "" else _reason_word(int(result.get("reason", 1)))

func _build_info_card(b: Dictionary, legal: Dictionary) -> Dictionary:
	var allowed := bool(legal.get("allowed", legal.get("legal", true)))
	var me: int = Sim.world.player()
	var ci: Dictionary = Sim.world.country_info(me)
	var gold_have := int(floor(float(ci.get("or", 0.0))))
	var gold_need := int(b.get("gold", 0))
	var lines := [{
		"label": "Or",
		"value": "coût %d · trésor %d" % [gold_need, gold_have],
	}]
	var stocks := {}
	if Sim.world.has_method("country_stocks"):
		for st in Sim.world.country_stocks(me):
			stocks[String(st.get("name", ""))] = int(st.get("stock", 0))
	for cost in b.get("cost", []):
		var name := String(cost.get("res", "Matière"))
		var need := int(cost.get("qty", 0))
		var have := int(stocks.get(name, 0))
		lines.append({
			"label": name,
			"value": "recette %d · stock national %d" % [need, have],
		})
	var effect := String(b.get("effet", ""))
	if effect != "":
		lines.append({"label": "Effet", "value": effect})
	return {
		"title": String(b.get("nom", "Construction")),
		"state": "Constructible" if allowed else "Bloqué — %s" % _reason_label(legal),
		"trend": "%d jours" % int(b.get("days", 0)),
		"lines": lines,
		"body": "Cliquez la ligne pour ordonner le chantier." if allowed else
			"Premier verrou opposé par le moteur : %s." % _reason_label(legal),
	}

## la RECETTE réelle d'une manufacture, en mots : « Laine ×1.5 (ou Coton) → Étoffe ×2.8 ».
func _recipe_text(rec: Dictionary) -> String:
	var in1 := String(rec.get("in1", ""))
	if in1 == "":
		return "hors-sol (aucun intrant de tuile)"
	var s := "%s ×%s" % [in1, _fmt1(rec.get("q1", 0.0))]
	var in2 := String(rec.get("in2", ""))
	if in2 != "":
		s += " + %s ×%s" % [in2, _fmt1(rec.get("q2", 0.0))]
	var alt1 := String(rec.get("alt1", ""))
	if alt1 != "" and alt1 != in1:
		s += " (ou %s)" % alt1
	var out := String(rec.get("out", ""))
	if out != "":
		s += " → %s ×%s" % [out, _fmt1(rec.get("qout", 0.0))]
	return s

## un nombre à 1 décimale, sans zéro inutile (1.0 → "1", 2.8 → "2.8").
func _fmt1(v) -> String:
	var f := float(v)
	return ("%d" % int(round(f))) if absf(f - round(f)) < 0.05 else ("%.1f" % f)

## tronque un texte à une largeur en px (petit corps)
func _fit(s: String, wpx: float) -> String:
	while VKit.text_w(s, VKit.FS_SMALL) > wpx and s.length() > 6:
		s = s.substr(0, s.length() - 4) + "…"
	return s

func _draw() -> void:
	_hover_zones.clear()
	_click_zones.clear()
	_tab_rects.clear()
	VKit.panel_bg(self, Rect2(0, 0, PW, _ph))
	_close_rect = VKit.header(self, PW, "CONSTRUCTION")

	# ── ONGLETS (retour joueur 2026-07-10) : Édifices | Manufactures ──
	var tx := PADX
	var ty := VKit.HDR_H + 6.0
	for ti in range(2):
		var lbl: String = ["Édifices", "Manufactures"][ti]
		var tw := VKit.text_w(lbl, VKit.FS_SMALL) + 18.0
		var tr := Rect2(tx, ty, tw, 22.0)
		VKit.fill(self, tr, VKit.COL_GOLD if _tab == ti else VKit.COL_PANEL2)
		VKit.box(self, tr, VKit.COL_EDGE)
		VKit.text(self, Vector2(tx + 9, ty + 3), VKit.COL_PANEL if _tab == ti else VKit.COL_PARCH, lbl, VKit.FS_SMALL)
		_tab_rects.append({"rect": tr, "t": ti})
		tx += tw + 6
	var ly0 := ty + 30.0                       # haut de la LISTE (défilable)
	var rw := PW - 2.0 * PADX - 10.0           # place de la barre latérale
	var yrow := ly0 - _scrolloff
	var content_h := 0.0

	if _tab == 0:
		# ── ÉDIFICES : une CARTE par bâtiment — Nom+Prix / Rendement / Ressources /
		# Prochain palier. Un édifice verrouillé par la tech N'EST JAMAIS listé comme
		# posable : il n'apparaît qu'en tag « Prochain palier » sur la carte de son
		# palier COURANT (celui qu'on peut réellement bâtir maintenant).
		var w = Sim.world
		for i in range(_builds.size()):
			var b: Dictionary = _builds[i]
			if int(b.get("prev", -1)) >= 0 and not bool(b.get("prev_built", false)):
				continue   # palier hors de portée : son précédent n'existe pas encore chez nous
			if not bool(b.get("debloque", false)):
				continue   # verrouillé par la tech : surfacé en tag sur son prédécesseur, pas ici
			var btype := int(b.get("type", -1))
			var leg: Dictionary = _blegal.get(btype, {})
			var affordable: bool = bool(leg.get("legal", true))
			var row := Rect2(PADX, yrow, rw, RH_ED - 4.0)
			content_h += RH_ED
			if yrow > _ph or yrow < ly0 - 4.0:
				yrow += RH_ED
				continue                        # hors fenêtre (une ligne partielle repeindrait les onglets)
			if _has_hover and _hover_rect == row:
				VKit.fill(self, row, Color(0.30, 0.24, 0.15, 0.35))
			VKit.box(self, row, Color(VKit.COL_EDGE.r, VKit.COL_EDGE.g, VKit.COL_EDGE.b, 0.5))
			var tex: Texture2D = UIKit.building_sprite(btype)
			if tex != null:
				draw_texture_rect(tex, Rect2(PADX + 4, yrow + 4, 34, 34), false,
					Color.WHITE if affordable else Color(0.5, 0.5, 0.55, 0.65))
			var ncol := VKit.COL_PARCH if affordable else VKit.COL_DIM
			# L1 — NOM (gauche) · PRIX + DURÉE (droite)
			VKit.text(self, Vector2(PADX + 48, yrow + 3), ncol, String(b.get("nom", "")))
			var ctx := "%d or · %d j" % [int(b.get("gold", 0)), int(b.get("days", 0))]
			VKit.value(self, Vector2(PADX + rw - VKit.text_w(ctx, VKit.FS_SMALL) - 6, yrow + 5),
				ctx, VKit.FS_SMALL)
			# L2 — RENDEMENT (l'effet RÉEL, delta ProvBuild — la membrane, pas une promesse)
			var eff := String(b.get("effet", ""))
			VKit.text(self, Vector2(PADX + 48, yrow + 21), VKit.sense(0.72), _fit(eff, rw - 54.0), VKit.FS_SMALL)
			# L3 — RESSOURCES (la recette, en icônes)
			var cx := PADX + 48.0
			var cost: Array = b.get("cost", [])
			for c in cost:
				var rnom := String(c.get("res", ""))
				var rspr: Texture2D = UIKit.resource_icon(rnom)
				if rspr != null:
					draw_texture_rect(rspr, Rect2(cx, yrow + 38, 20, 20), false)
					cx += 23
				else:
					VKit.text(self, Vector2(cx, yrow + 41), VKit.COL_DIM, rnom + " ", VKit.FS_SMALL)
					cx += VKit.text_w(rnom + " ", VKit.FS_SMALL)
				VKit.text(self, Vector2(cx, yrow + 41), VKit.COL_PARCH, "×%d" % int(c.get("qty", 0)), VKit.FS_SMALL)
				cx += VKit.text_w("×%d" % int(c.get("qty", 0)), VKit.FS_SMALL) + 10
			if cost.is_empty():
				VKit.text(self, Vector2(cx, yrow + 41), VKit.COL_DIM, "structurel", VKit.FS_SMALL)
			if not affordable:
				VKit.text(self, Vector2(cx + 4, yrow + 41), VKit.sense(0.12),
					_fit("✗ %s" % _reason_label(leg), (PADX + rw) - (cx + 4) - 4.0), VKit.FS_SMALL)
			# L4 — PROCHAIN PALIER (edifice_succ), affiché MÊME s'il est verrouillé par la tech
			var succ := int(w.edifice_succ(btype)) if w.has_method("edifice_succ") else -1
			var succ_b: Dictionary = _bytype.get(succ, {})
			if not succ_b.is_empty():
				var slocked := not bool(succ_b.get("debloque", false))
				VKit.text(self, Vector2(PADX + 48, yrow + 58),
					VKit.COL_DIM if slocked else VKit.sense(0.65),
					"Prochain palier : %s%s" % [String(succ_b.get("nom", "")), " (verrou tech)" if slocked else ""],
					VKit.FS_SMALL)
			# HOVER : le détail complet, en mots
			var lines := PackedStringArray()
			if eff != "":
				lines.append(eff)
			for c in cost:
				lines.append("%s : %d" % [c.get("res", ""), int(c.get("qty", 0))])
			lines.append("Or : %d   ·   %d jours" % [int(b.get("gold", 0)), int(b.get("days", 0))])
			if not affordable:
				lines.append("✗ %s" % _reason_label(leg))
			if not succ_b.is_empty():
				lines.append("Prochain palier : %s" % String(succ_b.get("nom", "")))
			_hover_zones.append({"rect": row, "head": String(b.get("nom", "")), "lines": lines,
				"card": _build_info_card(b, leg)})
			if affordable:
				_click_zones.append({"rect": row, "kind": "build", "type": btype, "nom": String(b.get("nom", ""))})
			yrow += RH_ED
	else:
		# ── MANUFACTURES — sur la province visée (target_pid, RE-KEY : pid direct) ──
		# CARTE : Nom + Prix (L1) · la RECETTE réelle intrants → produit (L2, chantier
		# « vérité absolue » — matcher manuf_recipe(bld), plus une phrase d'ambiance).
		var w = Sim.world
		if target_pid < 0:
			VKit.text(self, Vector2(PADX, yrow), VKit.COL_DIM, "sélectionnez une de vos provinces", VKit.FS_SMALL)
			content_h = 24.0
		elif w.has_method("manuf_legal"):
			var mcost: int = int(w.manuf_cost()) if w.has_method("manuf_cost") else 0
			var mi := 0
			for bld in range(24):   # BLD_TYPE_COUNT (miroir display-only, motif province_detail)
				if int(w.manuf_legal(target_pid, bld)) != 1:
					continue
				var mnom := String(w.manuf_name(bld))
				var rec: Dictionary = w.manuf_recipe(bld) if w.has_method("manuf_recipe") else {}
				var rtxt := _recipe_text(rec)
				var rowm := Rect2(PADX, yrow, rw, RH_MF - 4.0)
				content_h += RH_MF
				if yrow > _ph or yrow < ly0 - 4.0:
					yrow += RH_MF
					mi += 1
					continue
				if _has_hover and _hover_rect == rowm:
					VKit.fill(self, rowm, Color(0.30, 0.24, 0.15, 0.35))
				VKit.box(self, rowm, Color(VKit.COL_EDGE.r, VKit.COL_EDGE.g, VKit.COL_EDGE.b, 0.5))
				var mtex: Texture2D = UIKit.manuf_sprite(mnom)
				if mtex != null:
					draw_texture_rect(mtex, Rect2(PADX + 4, yrow + 4, 32, 32), false)
				VKit.text(self, Vector2(PADX + 46, yrow + 4), VKit.COL_PARCH, mnom)
				if mcost > 0:
					var mctx := "%d or" % mcost
					VKit.value(self, Vector2(PADX + rw - VKit.text_w(mctx, VKit.FS_SMALL) - 6, yrow + 6),
						mctx, VKit.FS_SMALL)
				VKit.text(self, Vector2(PADX + 46, yrow + 23), VKit.sense(0.72), _fit(rtxt, rw - 52.0), VKit.FS_SMALL)
				_hover_zones.append({"rect": rowm, "head": mnom, "lines": PackedStringArray([
					"Recette : %s" % rtxt,
					("Or (chantier) : %d" % mcost) if mcost > 0 else "coût au drain",
				])})
				_click_zones.append({"rect": rowm, "kind": "manuf", "type": bld, "nom": mnom})
				mi += 1
				yrow += RH_MF
			if mi == 0:
				VKit.text(self, Vector2(PADX, yrow), VKit.COL_DIM, "aucune manufacture posable ici (intrants/tech)", VKit.FS_SMALL)
				content_h = 24.0

	# hauteur AU CONTENU, bornée au VIEWPORT — le surplus défile (molette + barre)
	var hmax := get_viewport_rect().size.y - Frame.TOPBAR_H - Frame.BOTTOMBAR_H - 24.0
	var want := clampf(ly0 + content_h + 28.0, 240.0, hmax)
	if absf(want - _ph) > 0.5:
		_ph = want
		set_deferred("size", Vector2(PW, _ph))
	_maxscroll = maxf(0.0, content_h - (_ph - ly0 - 24.0))
	_scrolloff = clampf(_scrolloff, 0.0, _maxscroll)
	if _maxscroll > 0.0:
		# BARRE LATÉRALE : piste + pouce ∝ fenêtre/contenu
		var track := Rect2(PW - 10.0, ly0, 5.0, _ph - ly0 - 24.0)
		VKit.fill(self, track, VKit.COL_PANEL2)
		var frac := (track.size.y) / maxf(content_h, 1.0)
		var thumb_h := maxf(24.0, track.size.y * frac)
		var thumb_y := track.position.y + (_scrolloff / _maxscroll) * (track.size.y - thumb_h)
		VKit.fill(self, Rect2(track.position.x, thumb_y, 5.0, thumb_h), VKit.COL_GOLD)
	# le bandeau d'onglets reste AU-DESSUS de la liste défilée : re-fond + re-dessin léger
	if _flash != "":
		VKit.text(self, Vector2(PADX, _ph - 18), (VKit.sense(1.0) if _flash_ok else VKit.sense(0.05)), _flash, VKit.FS_SMALL)
	# (le détail passe par le TOOLTIP NATIF → TooltipServer : concepts + définitions)

## le TOOLTIP NATIF (→ TooltipServer, mots-concepts) : « Nom\nlignes de coût/refus »
func _get_tooltip(at_position: Vector2) -> String:
	for z in _hover_zones:
		if (z["rect"] as Rect2).has_point(at_position):
			var lines: PackedStringArray = z["lines"]
			return String(z["head"]) + ("\n" + "\n".join(lines) if lines.size() > 0 else "")
	return ""

func get_info_card(at_position: Vector2) -> Dictionary:
	for z in _hover_zones:
		if (z["rect"] as Rect2).has_point(at_position):
			return (z.get("card", {}) as Dictionary).duplicate(true)
	return {}

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed:
		# MOLETTE = défilement par LIGNE entière (les rangées restent alignées)
		var step := RH_ED if _tab == 0 else RH_MF
		if e.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_scrolloff = clampf(_scrolloff + step, 0.0, _maxscroll)
			queue_redraw()
			accept_event()
			return
		if e.button_index == MOUSE_BUTTON_WHEEL_UP:
			_scrolloff = clampf(_scrolloff - step, 0.0, _maxscroll)
			queue_redraw()
			accept_event()
			return
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(e.position):
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
			return
		for t in _tab_rects:
			if (t["rect"] as Rect2).has_point(e.position):
				_tab = int(t["t"])
				_scrolloff = 0.0
				Sound.play("ui_click")
				queue_redraw()
				accept_event()
				return
	if e is InputEventMouseMotion:
		var found := false
		for z in _hover_zones:
			if z["rect"].has_point(e.position):
				_has_hover = true
				_hover_rect = z["rect"]
				_hover_head = z["head"]
				_hover_lines = z["lines"]
				_hover_pos = e.position
				found = true
				break
		if not found:
			_has_hover = false
		queue_redraw()
	elif e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		for z in _click_zones:
			if z["rect"].has_point(e.position):
				_act(String(z["kind"]), int(z["type"]), String(z["nom"]))
				break

## le CLIC agit : on appelle l'actionneur joueur (façade) et on affiche le retour.
func _act(kind: String, type: int, nom: String) -> void:
	if Sim.world == null:
		return
	# Les ordres sont ENFILÉS (journal déterministe) : ils s'appliquent au prochain
	# tick (après agency_advance). En pause, l'ordre attend la reprise. Le retour
	# n'est donc que « mis en file », pas le verdict d'application (qui tombe au tick).
	# lot M — le drain refuse en SILENCE (or/matière) : on ne dit « ordre émis » que
	# si build_legal passe AU MOMENT DU CLIC ; sinon on nomme le refus.
	if kind == "build":
		if Sim.world.has_method("build_legal"):
			var bl: Dictionary = Sim.world.build_legal(-1, type)
			if not bool(bl.get("legal", true)):
				_flash_ok = false
				_flash = "✗ %s — %s" % [nom, _reason_label(bl)]
				Sound.play("ui_click")
				_refresh()
				return
		var ok: bool = Sim.world.player_build(type, -1)
		_flash_ok = ok
		_flash = ("⚒ %s — ordre émis" % nom) if ok else ("✗ %s — file pleine" % nom)
	elif kind == "manuf":
		var okm: bool = target_pid >= 0 and bool(Sim.world.player_build_manuf(target_pid, type))
		_flash_ok = okm
		_flash = ("⚒ %s — chantier ordonné" % nom) if okm else ("✗ %s — refusé" % nom)
	else:
		var ok2: bool = Sim.world.player_recruit(type) > 0
		_flash_ok = ok2
		_flash = ("⚔ %s — levée ordonnée" % nom) if ok2 else ("✗ %s — file pleine" % nom)
	if not _flash_ok:
		Sound.play("ui_click")
	build_requested.emit(kind, type)
	_refresh()
	Sim.notify_action()   # verbe joueur (bâtir / lever) → refresh des chiffres au drain (live)
