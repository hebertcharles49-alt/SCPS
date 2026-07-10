extends Control
## ConstructionPanel — le menu de bâti en DEUX ONGLETS (Édifices | Manufactures,
## retour joueur 2026-07-10) : une LIGNE LARGE et DESCRIPTIVE par bâtiment —
## icônes des RESSOURCES de la recette, EFFET chiffré réel (delta ProvBuild via la
## façade), FLAVOR cynique. Paliers familiaux masqués tant que le précédent n'est
## pas bâti (prev_built). Molette = défilement. Immediate-mode _draw, prix RÉELS.

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

signal build_requested(kind: String, type: int)

const PADX := 12
const RH_ED := 76.0    ## ligne ÉDIFICE (nom+coût / recette / effet / flavor)
const RH_MF := 42.0    ## ligne MANUFACTURE (nom+or / note)
const PW := 396.0

var _ph := 360.0       ## hauteur latchée (contenu, borné viewport — le surplus SCROLLE)
var _tab := 0          ## 0 = Édifices · 1 = Manufactures
var _tab_rects := []
var _scrolloff := 0.0
var _maxscroll := 0.0

var target_pid := -1       ## la PROVINCE visée (posée par main à l'ouverture) — les manufactures y vivent
var _builds := []
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
	# lot M — la LÉGALITÉ réelle (or/matière/palier, miroir du drain qui refusait en
	# silence) : rafraîchie au tick, consommée par _draw (griser) et _act (flash honnête).
	_blegal.clear()
	if Sim.world.has_method("build_legal"):
		for b in _builds:
			if bool(b.get("debloque", false)):
				var t := int(b.get("type", -1))
				_blegal[t] = Sim.world.build_legal(-1, t)
	queue_redraw()

## la raison du refus, en mot (reason de build_legal : 2 or · 3 matière · 4 tech de palier · 1 structurel)
func _reason_word(reason: int) -> String:
	match reason:
		2: return "or insuffisant"
		3: return "matière manquante"
		4: return "tech de palier manquante"
		_: return "indisponible ici (palier/déjà bâti)"

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
		# ── ÉDIFICES : lignes LARGES — recette en ICÔNES, EFFET chiffré, FLAVOR ──
		for i in range(_builds.size()):
			var b: Dictionary = _builds[i]
			if int(b.get("prev", -1)) >= 0 and not bool(b.get("prev_built", false)):
				continue   # palier caché : son précédent n'existe pas encore chez nous
			var on2: bool = bool(b.get("debloque", false))
			var btype := int(b.get("type", -1))
			var leg: Dictionary = _blegal.get(btype, {})
			var affordable: bool = bool(leg.get("legal", true)) if on2 else false
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
				draw_texture_rect(tex, Rect2(PADX + 4, yrow + 4, 26, 26), false,
					Color.WHITE if (on2 and affordable) else Color(0.5, 0.5, 0.55, 0.65))
			var ncol := VKit.COL_PARCH if (on2 and affordable) else VKit.COL_DIM
			VKit.text(self, Vector2(PADX + 38, yrow + 5), ncol, String(b.get("nom", "")))
			if not on2:
				VKit.text(self, Vector2(PADX + 38 + VKit.text_w(String(b.get("nom", ""))) + 6, yrow + 5),
					VKit.COL_GOLD, "✦", VKit.FS_SMALL)
			var ctx := "%d or · %d j" % [int(b.get("gold", 0)), int(b.get("days", 0))]
			VKit.text(self, Vector2(PADX + rw - VKit.text_w(ctx, VKit.FS_SMALL) - 6, yrow + 6),
				VKit.COL_DIM, ctx, VKit.FS_SMALL)
			# L2 : la RECETTE en icônes de ressource (retour joueur : « icône ressources »)
			var cx := PADX + 38.0
			for c in b.get("cost", []):
				var rnom := String(c.get("res", ""))
				var rspr: Texture2D = UIKit.resource_icon(rnom)
				if rspr != null:
					draw_texture_rect(rspr, Rect2(cx, yrow + 26, 16, 16), false)
					cx += 19
				else:
					VKit.text(self, Vector2(cx, yrow + 27), VKit.COL_DIM, rnom + " ", VKit.FS_SMALL)
					cx += VKit.text_w(rnom + " ", VKit.FS_SMALL)
				VKit.text(self, Vector2(cx, yrow + 27), VKit.COL_PARCH, "×%d" % int(c.get("qty", 0)), VKit.FS_SMALL)
				cx += VKit.text_w("×%d" % int(c.get("qty", 0)), VKit.FS_SMALL) + 10
			if not on2:
				VKit.text(self, Vector2(cx + 4, yrow + 27), VKit.COL_GOLD, "✦ verrou tech", VKit.FS_SMALL)
			elif not affordable:
				VKit.text(self, Vector2(cx + 4, yrow + 27), VKit.sense(0.12),
					"✗ %s" % _reason_word(int(leg.get("reason", 1))), VKit.FS_SMALL)
			# L3 : l'EFFET RÉEL chiffré (delta ProvBuild, façade)
			var eff := String(b.get("effet", ""))
			if eff != "":
				VKit.text(self, Vector2(PADX + 38, yrow + 43), VKit.sense(0.72), _fit(eff, rw - 44.0), VKit.FS_SMALL)
			# L4 : le FLAVOR cynique
			var fla := String(b.get("flavor", ""))
			if fla != "":
				VKit.text(self, Vector2(PADX + 38, yrow + 58), VKit.COL_DIM,
					_fit("« %s »" % fla, rw - 44.0), VKit.FS_SMALL)
			var lines := PackedStringArray()
			if eff != "":
				lines.append(eff)
			for c in b.get("cost", []):
				lines.append("%s : %d" % [c.get("res", ""), int(c.get("qty", 0))])
			lines.append("Or : %d   ·   %d jours" % [int(b.get("gold", 0)), int(b.get("days", 0))])
			if not on2:
				lines.append("✦ verrouillé par la technologie")
			elif not affordable:
				lines.append("✗ %s" % _reason_word(int(leg.get("reason", 1))))
			if fla != "":
				lines.append("« %s »" % fla)
			_hover_zones.append({"rect": row, "head": String(b.get("nom", "")), "lines": lines})
			if on2 and affordable:
				_click_zones.append({"rect": row, "kind": "build", "type": btype, "nom": String(b.get("nom", ""))})
			yrow += RH_ED
	else:
		# ── MANUFACTURES — sur la province visée (target_pid) ──
		var w = Sim.world
		var mreg: int = w.province_region(target_pid) if target_pid >= 0 else -1
		if mreg < 0:
			VKit.text(self, Vector2(PADX, yrow), VKit.COL_DIM, "sélectionnez une de vos provinces", VKit.FS_SMALL)
			content_h = 24.0
		elif w.has_method("manuf_legal"):
			var mcost: int = int(w.manuf_cost()) if w.has_method("manuf_cost") else 0
			var mi := 0
			for bld in range(24):   # BLD_TYPE_COUNT (miroir display-only, motif province_detail)
				if int(w.manuf_legal(mreg, bld)) != 1:
					continue
				var mnom := String(w.manuf_name(bld))
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
					draw_texture_rect(mtex, Rect2(PADX + 4, yrow + 4, 24, 24), false)
				VKit.text(self, Vector2(PADX + 38, yrow + 4), VKit.COL_PARCH, mnom)
				if mcost > 0:
					var mctx := "%d or" % mcost
					VKit.text(self, Vector2(PADX + rw - VKit.text_w(mctx, VKit.FS_SMALL) - 6, yrow + 6),
						VKit.COL_DIM, mctx, VKit.FS_SMALL)
				VKit.text(self, Vector2(PADX + 38, yrow + 23), VKit.COL_DIM,
					"s'élève dans la province visée (bras & intrants locaux)", VKit.FS_SMALL)
				_hover_zones.append({"rect": rowm, "head": mnom, "lines": PackedStringArray([
					"Manufacture : s'élève dans la province visée",
					("Or : %d" % mcost) if mcost > 0 else "coût au drain",
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
				_flash = "✗ %s — %s" % [nom, _reason_word(int(bl.get("reason", 1)))]
				Sound.play("ui_click")
				_refresh()
				return
		var ok: bool = Sim.world.player_build(type, -1)
		_flash_ok = ok
		_flash = ("⚒ %s — ordre émis" % nom) if ok else ("✗ %s — file pleine" % nom)
	elif kind == "manuf":
		var mreg2: int = Sim.world.province_region(target_pid) if target_pid >= 0 else -1
		var okm: bool = mreg2 >= 0 and bool(Sim.world.player_build_manuf(mreg2, type))
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
