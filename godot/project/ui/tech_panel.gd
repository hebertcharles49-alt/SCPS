extends Control
## TechPanel — l'arbre de technologie du JOUEUR (read-only), bascule touche T.
## Rendu en GRAPHE avec l'addon MEDUSA : un Atom (cercle teinté) par nœud de tech,
## disposé en RADIAL (angle = quadrant 0-8, rayon = tier 0-5), relié à son prérequis
## par une arête (GraphSoftLine). La COULEUR dit l'état (verrouillé/recherchable/
## acquis) ; le bout faustien vire au rouge. Clic sur un nœud → son détail en pied.
## En-tête : points · présage (bande) · crise %. Lit tech_info/tech_nodes (la façade).
## ACTIONNABLE : cliquer un nœud RECHERCHABLE le fixe comme cible (player_research) ;
## l'en-tête montre la recherche EN COURS + sa jauge de progression (research_status).

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")

## POPUP « métabolisation prête » (V1b) : quand un héritage NON natif atteint tier 3
## (digestion pleine), on notifie UNE FOIS — le fil de la victoire Merveille (paliers
## culture) doit se VOIR. Latch LOCAL au nœud (non sérialisé) : un re-lancement re-notifie,
## acceptable pour un signal purement display-only.
signal metab_ready(nom: String)
var _metab_seen := {}   ## nom héritage (natif à part) → true une fois notifié tier==3
# taille ADAPTATIVE à la fenêtre (recalculée dans _layout ; plancher = l'ancienne taille fixe)
var PW := 720.0
var PH := 560.0
const HEAD := 52.0          # hauteur d'en-tête (titre + jauges)
const FOOT := 22.0          # pied (détail du nœud sélectionné)
const METAH := 92.0         # bande de MÉTABOLISATION (le +% recherche + accès par héritage + compte Ascension)

# couleurs d'état (sans bibliothèque d'animation Medusa : on teinte le cercle)
const COL_LOCKED   := Color(0.40, 0.40, 0.46)
const COL_AVAIL    := Color(0.85, 0.60, 0.28)
const COL_UNLOCKED := Color(0.40, 0.80, 0.46)
const COL_FAUST    := Color(0.88, 0.24, 0.24)

var _graph                 # Medusa Graph (Control)
var _atoms := []           # Atom par indice de nœud
var _info := {}            # Atom -> dict du nœud (pour le détail au clic)
var _nodes := []           # le tableau de nœuds (lookup index → nom pour la cible)
var _built := false
var _sel := ""             # détail du nœud sélectionné (pied)
var _close_rect := Rect2()

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_layout()
	get_viewport().size_changed.connect(_layout)
	visibility_changed.connect(_on_visibility)
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(func(_y):
		_check_metab_ready()          # surveille le franchissement de tier — même panneau FERMÉ
		if visible: queue_redraw())
	hide()

## surveille `merv_metab()` — CE QUI COMPTE POUR LA MERVEILLE (endgame_metab_count),
## PAS l'accès tech (heritage_access/tier) : un héritage non natif dont `metabolized`
## bascule à vrai déclenche `metab_ready` UNE fois (latch `_metab_seen`). Avant P5,
## ce chip lisait heritage_access() (sémantique tech) — un joueur pouvait voir le ✓
## sans que la culture compte pour l'Ascension, et l'inverse ; corrigé (cf. TROUVAILLES).
func _check_metab_ready() -> void:
	if Sim.world == null or not Sim.world.has_method("merv_metab"):
		return
	var mm: Dictionary = Sim.world.merv_metab()
	var heritages: Array = mm.get("heritages", [])
	for h in heritages:
		var nativ := bool(h.get("native", false))
		if nativ:
			continue
		var nom := String(h.get("nom", ""))
		if nom == "" or _metab_seen.get(nom, false):
			continue
		if bool(h.get("metabolized", false)):
			_metab_seen[nom] = true
			metab_ready.emit(nom)

func _layout() -> void:
	var vp := get_viewport_rect().size
	var pw0 := PW
	var ph0 := PH
	# TRÈS grand format (retour joueur 2026-07-10 : « agrandis sérieusement ») — la
	# géométrie des nœuds est FIXE et généreuse, la hauteur SCROLLE (barre latérale).
	# Centré ENTRE le rail gauche et le ledger droit (il passait sous le ledger).
	var free_x0 := Frame.SIDEBAR_W + 8.0
	var free_x1 := vp.x - 274.0 - 8.0
	PW = clampf((free_x1 - free_x0) * 0.98, 900.0, 1780.0)
	PH = clampf(vp.y * 0.92, 620.0, 1200.0)
	size = Vector2(PW, PH)
	position = Vector2(free_x0 + (free_x1 - free_x0 - PW) * 0.5, (vp.y - PH) * 0.5)
	if _built and (absf(PW - pw0) > 1.0 or absf(PH - ph0) > 1.0):
		_built = false                       # le graphe se rebâtit à la nouvelle taille
		if visible:
			_build()

func _on_visibility() -> void:
	if visible and not _built and Sim.world != null:
		_build()

func _on_generated() -> void:
	_built = false
	_sel = ""
	_metab_seen.clear()
	if visible:
		_build()

# ── construction du graphe Medusa ──────────────────────────────────────────
func _build() -> void:
	if Sim.world == null:
		return
	if _scroll != null and is_instance_valid(_scroll):
		_scroll.queue_free()
	_atoms.clear()
	_info.clear()

	var nodes: Array = Sim.world.tech_nodes()
	_nodes = nodes
	if nodes.is_empty():
		_built = true
		return

	# ── COULOIRS THÉMATIQUES à géométrie FIXE et GÉNÉREUSE (retour joueur 2026-07-10 :
	# « agrandis sérieusement, barre de scroll latérale, qu'on puisse lire les icônes ») :
	# rangée 64 px · médaillon ~53 px. La hauteur totale déborde → ScrollContainer. ──
	var cnt := {}
	var tiers_set := {}
	for i in nodes.size():
		var t := int(nodes[i]["tier"])
		var l := int(float(int(nodes[i]["quarter"])) / 3.0)
		tiers_set[t] = true
		var key := l * 16 + t
		cnt[key] = int(cnt.get(key, 0)) + 1
	var tiers: Array = tiers_set.keys()
	tiers.sort()
	_ncol = tiers.size()
	_rowh = 64.0
	var rr := 28.0
	_lane_rows = [0, 0, 0]
	for l in range(3):
		for t in tiers:
			_lane_rows[l] = maxi(int(_lane_rows[l]), int(cnt.get(l * 16 + int(t), 0)))
	var total_rows: int = int(_lane_rows[0]) + int(_lane_rows[1]) + int(_lane_rows[2])
	_lane_y = [26.0, 0.0, 0.0]
	_lane_y[1] = float(_lane_y[0]) + float(_lane_rows[0]) * _rowh + 16.0
	_lane_y[2] = float(_lane_y[1]) + float(_lane_rows[1]) * _rowh + 16.0
	var content_h: float = float(_lane_y[2]) + float(_lane_rows[2]) * _rowh + 26.0
	var view_w := PW - 24.0
	var content_w := view_w - 14.0            # place de la barre latérale
	var colw: float = (content_w - 116.0) / maxf(1.0, float(_ncol))

	# le SCROLL (barre latérale) : fenêtre fixe, contenu haut — fond de couloirs + graphe
	_scroll = ScrollContainer.new()
	_scroll.position = Vector2(12, HEAD)
	_scroll.size = Vector2(view_w, PH - HEAD - FOOT - METAH)
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	add_child(_scroll)
	var content := Control.new()
	content.custom_minimum_size = Vector2(content_w, content_h)
	_scroll.add_child(content)
	_bg = Control.new()
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_bg.size = Vector2(content_w, content_h)
	content.add_child(_bg)
	_bg.draw.connect(_draw_rings)

	# un Graph configuré EN CODE : pas de physique (disposition figée), une ligne
	# douce grise pour les arêtes, pas de fondu d'amorçage.
	_graph = Graph.new()
	_graph.physics_enabled = false
	_graph.enable_start_up_modulation = false
	_graph.drag_input = false
	var line := GraphSoftLine.new()
	line.connection_color = Color(0.70, 0.66, 0.58, 0.85)
	line.connection_width = 2.0
	_graph.graph_lines = line
	_graph.position = Vector2.ZERO
	_graph.size = Vector2(content_w, content_h)
	# ⚠ le Graph n'entre dans l'arbre QU'APRÈS la pose des Atomes (son _ready les recense)
	_pending_graph_parent = content
	# placement ANTI-CROISEMENT : colonne par colonne (gauche→droite), chaque nœud d'une
	# pile (couloir, tier) est trié par la HAUTEUR de son PRÉREQUIS déjà placé (barycentre)
	# — l'arête coule tout droit au lieu de croiser (l'ancien tri était ALPHABÉTIQUE).
	_atoms.resize(nodes.size())
	var rowpos := {}   # idx nœud → yy posé (lu par les colonnes suivantes)
	for tcol in range(tiers.size()):
		var t2 := int(tiers[tcol])
		for l2 in range(3):
			var col_idx := []
			for i in nodes.size():
				if int(nodes[i]["tier"]) == t2 and int(float(int(nodes[i]["quarter"])) / 3.0) == l2:
					col_idx.append(i)
			if col_idx.is_empty():
				continue
			col_idx.sort_custom(func(a, b):
				var ka: float = rowpos.get(int(nodes[a].get("prereq", -1)), 1e9)
				var kb: float = rowpos.get(int(nodes[b].get("prereq", -1)), 1e9)
				if absf(ka - kb) > 0.01:
					return ka < kb
				return String(nodes[a]["name"]) < String(nodes[b]["name"]))
			var k := col_idx.size()
			var xx: float = 100.0 + colw * float(tcol) + colw * 0.5
			for j in range(k):
				var idx: int = col_idx[j]
				var yy: float = float(_lane_y[l2]) + (float(int(_lane_rows[l2]) - k) * 0.5 + float(j) + 0.5) * _rowh
				rowpos[idx] = yy
				var atom = _make_atom(nodes[idx], Vector2(xx, yy), rr)
				_graph.add_child(atom)
				_atoms[idx] = atom
				_info[atom] = nodes[idx]

	_pending_graph_parent.add_child(_graph) # → _ready du Graph : il recense les Atomes
	# arêtes : chaque nœud relié à son prérequis (indice dans CE tableau)
	for i in nodes.size():
		var pr := int(nodes[i].get("prereq", -1))
		if pr >= 0 and pr < _atoms.size() and _atoms[i] != null and _atoms[pr] != null:
			_graph.connect_atoms(_atoms[pr], _atoms[i])
	if _graph.has_signal("atom_selected") and not _graph.atom_selected.is_connected(_on_atom_selected):
		_graph.atom_selected.connect(_on_atom_selected)
	_built = true
	queue_redraw()

## géométrie des COULOIRS (posée par _build, lue par _draw_rings — fonds/étiquettes)
var _lane_rows: Array = [0, 0, 0]
var _lane_y: Array = [0.0, 0.0, 0.0]
var _rowh := 64.0
var _ncol := 6
var _scroll: ScrollContainer = null       ## la fenêtre scrollable (barre latérale)
var _bg: Control = null                   ## fond de couloirs/tiers, DANS le scroll
var _pending_graph_parent: Control = null ## le parent du Graph (posé après les Atomes)

func _make_atom(nd: Dictionary, target: Vector2, rr: float = 13.0):
	var atom = Atom.new()
	var d = AtomData.new()
	d.id = StringName("tech_%s" % String(nd["name"]))
	d.title = String(nd["name"])
	atom.data = d
	atom.is_static = true
	atom.drag_input = false
	atom.radius = rr
	atom.size = Vector2(rr * 2.0, rr * 2.0)
	atom.position = target - Vector2(rr, rr)
	# PACK FLAVOR — survol (tooltip natif Godot, Atom hérite de Control) : 2 lignes,
	# le mécanique (hover, ce que le nœud fait vraiment) puis le mot du conseiller (flavor,
	# cynique à la Civ) — même motif que culture_creator.gd (nom/hover en tooltip_text).
	var hov := String(nd.get("hover", ""))
	var fla := String(nd.get("flavor", ""))
	var tip := String(nd["name"])
	if hov != "":
		tip += "\n" + hov
	if fla != "":
		tip += "\n— " + fla
	atom.tooltip_text = tip
	var st := int(nd["state"])
	var col := COL_LOCKED
	if st == 1:
		col = COL_AVAIL
	elif st == 2:
		col = COL_UNLOCKED
	if bool(nd.get("faustian", false)):
		col = col.lerp(COL_FAUST, 0.6)
	atom.color = col
	# l'état Medusa (LOCKED/AVAILABLE/UNLOCKED) — miroir de l'état moteur
	match st:
		1: atom.status = Atom.Status.AVAILABLE
		2: atom.status = Atom.Status.UNLOCKED
		_: atom.status = Atom.Status.LOCKED
	# MÉDAILLON parchemin (nom connu > apex/combo/faustien > fonction du quartier)
	var md: Texture2D = UIKit.tech_medallion(String(nd["name"]),
		bool(nd.get("faustian", false)), int(nd["tier"]), int(nd["quarter"]))
	if md != null:
		var mr := TextureRect.new()
		mr.texture = md
		mr.stretch_mode = TextureRect.STRETCH_SCALE
		mr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE   # sinon min-size = 256² (texture)
		var ms := rr * 1.9   # le médaillon domine FRANCHEMENT le disque (retour joueur ×2)
		mr.size = Vector2(ms, ms)
		mr.position = Vector2(rr - ms * 0.5, rr - ms * 0.5)
		mr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		# verrouillé = fané ; le lavis d'état (couleur d'atome) reste lisible derrière
		mr.modulate = Color(1, 1, 1, 0.55) if st == 0 else Color(1, 1, 1, 0.95)
		atom.add_child(mr)
	return atom

func _on_atom_selected(atom) -> void:
	var nd = _info.get(atom, null)
	if nd == null:
		_sel = ""
		queue_redraw()
		return
	var states := ["verrouillé", "recherchable", "acquis"]
	var stt := int(nd["state"])
	_sel = "%s — %s · %s" % [String(nd["name"]), String(nd.get("effet", "")), states[clampi(stt, 0, 2)]]
	if int(nd.get("cost", 0)) > 0 and stt != 2:
		_sel += " (%d pts)" % int(nd["cost"])
	# ACTIONNABLE : un nœud RECHERCHABLE (state==1) cliqué devient la CIBLE de recherche
	# (l'indice de _atoms == TechId ; la façade enfile CMD_RESEARCH, le déblocage tombe au tick).
	if stt == 1 and Sim.world != null:
		var idx := _atoms.find(atom)
		if idx >= 0 and Sim.world.player_research(idx) != 0:
			_sel += "   → recherche lancée"
	queue_redraw()

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(e.position):
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
			return

# ── chrome du panneau : fond + en-tête + pied (le graphe se dessine seul) ───
func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	# (les couloirs/tiers se dessinent sur le FOND scrollable — cf. _draw_rings)
	var info: Dictionary = w.tech_info()
	UIKit.draw_icon(self, "knowledge_book", Vector2(14, 12), 20)
	VKit.text(self, Vector2(42, 13), VKit.COL_GOLD, "Arbre de technologie", VKit.FS_BIG)

	# ✕ — tout panneau se ferme (Échap le ferme aussi via main)
	_close_rect = Rect2(PW - 26, 6, 20, 20)
	VKit.fill(self, _close_rect, VKit.COL_PANEL2)
	VKit.box(self, _close_rect, VKit.COL_GOLD)
	VKit.text(self, Vector2(_close_rect.position.x + 6, _close_rect.position.y + 3), VKit.COL_PARCH, "x")

	VKit.text(self, Vector2(PW - 250, 13), VKit.COL_PARCH, "Points : %d" % int(info.get("points", 0)), VKit.FS_SMALL)
	var crise := int(info.get("crise_pct", 0))
	var pcol := VKit.COL_DIM if crise < 25 else (VKit.sense(0.40) if crise < 60 else VKit.sense(0.10))
	VKit.text(self, Vector2(PW - 250, 30), pcol,
		"Présage : %s (crise %d%%)" % [String(info.get("presage", "")), crise], VKit.FS_SMALL)
	# RECHERCHE EN COURS : la cible + sa jauge de progression (research_status)
	var rs: Dictionary = w.research_status()
	var rt := int(rs.get("target", -1))
	if rt >= 0 and rt < _nodes.size():
		var rname := String(_nodes[rt].get("name", "?"))
		var prog := clampf(float(rs.get("progress", 0.0)), 0.0, 1.0)
		VKit.text(self, Vector2(220, 13), VKit.COL_GOLD, "Recherche : %s" % rname, VKit.FS_SMALL)
		var bx := 220.0
		var bw := 200.0
		VKit.box(self, Rect2(bx, 30, bw, 9), VKit.COL_DIM)
		VKit.fill(self, Rect2(bx + 1, 31, (bw - 2) * prog, 7), COL_UNLOCKED)
		VKit.text(self, Vector2(bx + bw + 8, 30), VKit.COL_PARCH, "%d%%" % int(prog * 100.0), VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(220, 13), VKit.COL_DIM, "Recherche : (cliquez un nœud recherchable)", VKit.FS_SMALL)
	# légende d'état
	VKit.text(self, Vector2(14, 33), COL_UNLOCKED, "● acquis", VKit.FS_SMALL)
	VKit.text(self, Vector2(86, 33), COL_AVAIL, "● recherchable", VKit.FS_SMALL)
	VKit.text(self, Vector2(196, 33), COL_LOCKED, "● verrouillé", VKit.FS_SMALL)
	VKit.text(self, Vector2(286, 33), COL_FAUST, "● faustien", VKit.FS_SMALL)
	VKit.fill(self, Rect2(12, HEAD - 4, PW - 24, 1), VKit.COL_EDGE)
	# bande de MÉTABOLISATION : le +% recherche + l'accès tech par héritage (la barre)
	_draw_metab(info)
	# pied : détail du nœud cliqué + rappel touche
	VKit.fill(self, Rect2(12, PH - FOOT, PW - 24, 1), VKit.COL_EDGE)
	if _sel != "":
		VKit.text(self, Vector2(16, PH - FOOT + 4), VKit.COL_PARCH, _sel, VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(16, PH - FOOT + 4), VKit.COL_DIM, "Cliquez un nœud pour son détail.", VKit.FS_SMALL)

# ── bande de MÉTABOLISATION : le +% recherche du creuset + l'accès tech par héritage ──
# Le "+X% recherche" répond à « métabolisation = +% tech visible sous la barre de savoir » ;
# les 6 barres (tier 0-3 en pips + part digérée) sont la « barre de progression par tier » :
# digérer un peuple OUVRE ses signatures (tier 1 commerce → tier 3 plein/métabolisé).
# Anneaux de TIER (rayon = profondeur 1-5) — rend la structure en tiers LISIBLE sous le graphe
# Medusa (l'« arbre cohérent par tier » voulu, sans toucher aux prix/équilibre). Display-only.
func _draw_rings() -> void:
	# COULOIRS THÉMATIQUES (miroir du _build), dessinés sur le FOND scrollable (_bg,
	# coordonnées CONTENU) : fond alterné + ruban de thème + étiquette + tiers.
	if _bg == null or not is_instance_valid(_bg):
		return
	var gs: Vector2 = _bg.size
	var lane_names := ["Savoir", "Forge", "Société"]          # ordre THM_* (scps_tech.h)
	var lane_ink := [Color(0.35, 0.45, 0.62, 0.75), Color(0.66, 0.34, 0.22, 0.75), Color(0.45, 0.55, 0.30, 0.75)]
	for l in range(3):
		var y0: float = float(_lane_y[l]) - 6.0
		var lh: float = float(_lane_rows[l]) * _rowh + 12.0
		if l % 2 == 1:
			VKit.fill(_bg, Rect2(8.0, y0, gs.x - 16.0, lh), Color(0.32, 0.27, 0.20, 0.10))
		VKit.fill(_bg, Rect2(8.0, y0, 3.0, lh), lane_ink[l])
		VKit.text(_bg, Vector2(18.0, y0 + lh * 0.5 - 9.0), VKit.COL_GOLD, lane_names[l], VKit.FS_SMALL)
	var colw: float = (gs.x - 116.0) / float(maxi(1, _ncol))
	for t in range(_ncol):
		var cx: float = 100.0 + colw * float(t)
		if t > 0:
			VKit.fill(_bg, Rect2(cx, 8.0, 1.0, gs.y - 16.0), Color(0.58, 0.52, 0.42, 0.14))
		VKit.text(_bg, Vector2(cx + colw * 0.5 - 8.0, 2.0), VKit.COL_DIM, "T%d" % t, VKit.FS_SMALL)

## DEUX LECTURES DISTINCTES, séparées visuellement (P5 — une seule source de vérité
## pour la victoire) :
##   1) « accès aux signatures » = heritage_access() (tech, pop-share, tier 0-3) —
##      ouvre les nœuds de l'arbre, ne dit RIEN de la victoire Merveille.
##   2) « compte pour l'Ascension » = merv_metab() (endgame_metab_count) — ce que
##      wonder_tick gate réellement (X/N du palier courant), la SEULE jauge de victoire.
func _draw_metab(info: Dictionary) -> void:
	var y0 := PH - FOOT - METAH
	VKit.fill(self, Rect2(12, y0, PW - 24, 1), VKit.COL_EDGE)
	var mp := int(info.get("metab_pct", 0))
	UIKit.draw_icon(self, "knowledge_book", Vector2(14, y0 + 4), 14)
	VKit.text(self, Vector2(36, y0 + 5), VKit.COL_GOLD,
		"Peuples intégrés : +%d%% de recherche" % mp, VKit.FS_SMALL)
	if Sim.world == null:
		return

	# ── Rangée 1 : ACCÈS AUX SIGNATURES (tech, heritage_access — pas la victoire) ──
	# Chaque héritage = une CELLULE CENTRÉE dans sa colonne : nom, puis BARRE de %
	# de digestion (retour joueur : « centre mieux, donne une barre de % ») + 3 pips
	# de tier à droite de la barre.
	var acc: Array = Sim.world.heritage_access()
	var n := acc.size()
	if n > 0:
		var cw := (PW - 28.0) / float(n)
		var ry := y0 + 30.0
		VKit.text(self, Vector2(16, ry - 12), VKit.COL_DIM, "Accès aux signatures (arbre) :", VKit.FS_SMALL)
		for i in n:
			var h: Dictionary = acc[i]
			var x := 16.0 + i * cw
			var nm := String(h.get("nom", ""))
			if nm.length() > 14:
				nm = nm.substr(0, 14)
			var nativ := bool(h.get("native", false))
			var tier := int(h.get("tier", 0))
			var mark := "★ " if nativ else ""
			var lbl := mark + nm
			var lw := VKit.text_w(lbl, VKit.FS_SMALL)
			VKit.text(self, Vector2(x + (cw - lw) * 0.5, ry), (VKit.COL_GOLD if nativ else VKit.COL_PARCH),
				lbl, VKit.FS_SMALL)
			# la BARRE de digestion (native = pleine, or) + les pips de tier à sa droite
			var dp := int(h.get("digested_pct", 0))
			var bw := cw - 58.0
			var bx := x + (cw - bw - 34.0) * 0.5
			var by := ry + 16.0
			VKit.fill(self, Rect2(bx, by, bw, 7), VKit.COL_PANEL2)
			VKit.box(self, Rect2(bx, by, bw, 7), VKit.COL_EDGE)
			if nativ:
				VKit.fill(self, Rect2(bx + 1, by + 1, bw - 2, 5), VKit.COL_GOLD)
			elif dp > 0:
				VKit.fill(self, Rect2(bx + 1, by + 1, (bw - 2) * clampf(dp / 100.0, 0.0, 1.0), 5), COL_UNLOCKED)
			var ptxt := "★" if nativ else "%d%%" % dp
			VKit.text(self, Vector2(bx + bw + 5, by - 4), VKit.COL_DIM, ptxt, VKit.FS_SMALL)
			for k in 3:                                   # 3 pips de tier (rempli = accessible)
				VKit.fill(self, Rect2(bx + bw + 5 + 24.0 + k * 8.0, by, 6, 6),
					(COL_UNLOCKED if tier > k else COL_LOCKED))

	# ── Rangée 2 : COMPTE POUR L'ASCENSION (merv_metab — la seule jauge de victoire) ──
	if not Sim.world.has_method("merv_metab"):
		return
	var mm: Dictionary = Sim.world.merv_metab()
	var mh: Array = mm.get("heritages", [])
	var mcount := int(mm.get("count", 0))
	var mreq := int(mm.get("required", 0))
	var m := mh.size()
	if m == 0:
		return
	var cw2 := (PW - 28.0) / float(m)
	var ry2 := y0 + 68.0
	var req_txt := (" — requis palier : %d" % mreq) if mreq > 0 else ""
	VKit.text(self, Vector2(16, ry2 - 12), VKit.COL_GOLD,
		"Compte pour l'Ascension : %d/%d%s" % [mcount, m, req_txt], VKit.FS_SMALL)
	for i in m:
		var h2: Dictionary = mh[i]
		var x2 := 16.0 + i * cw2
		var nm2 := String(h2.get("nom", ""))
		if nm2.length() > 14:
			nm2 = nm2.substr(0, 14)
		var nativ2 := bool(h2.get("native", false))
		var metab2: bool = bool(h2.get("metabolized", false))
		var voie2 := String(h2.get("voie", ""))
		var lbl2 := ("★" if nativ2 else ("✓" if metab2 else "·")) + " " + nm2
		var lw2 := VKit.text_w(lbl2, VKit.FS_SMALL)
		VKit.text(self, Vector2(x2 + (cw2 - lw2) * 0.5, ry2), (VKit.COL_GOLD if (nativ2 or metab2) else VKit.COL_PARCH),
			lbl2, VKit.FS_SMALL)
		if not nativ2 and voie2 != "":
			var vw2 := VKit.text_w(voie2, VKit.FS_SMALL)
			VKit.text(self, Vector2(x2 + (cw2 - vw2) * 0.5, ry2 + 13), VKit.COL_DIM, voie2, VKit.FS_SMALL)
