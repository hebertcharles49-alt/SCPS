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
const METAH := 66.0         # bande de MÉTABOLISATION (le +% recherche + accès par héritage + compte Ascension)

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
	PW = clampf(vp.x * 0.52, 720.0, 1150.0)
	PH = clampf(vp.y * 0.64, 560.0, 900.0)
	size = Vector2(PW, PH)
	position = Vector2((vp.x - PW) * 0.5, (vp.y - PH) * 0.5)
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
	if _graph != null and is_instance_valid(_graph):
		_graph.queue_free()
	_atoms.clear()
	_info.clear()

	var nodes: Array = Sim.world.tech_nodes()
	_nodes = nodes
	if nodes.is_empty():
		_built = true
		return

	# un Graph configuré EN CODE : pas de physique (disposition radiale figée), une
	# ligne douce grise pour les arêtes, pas de fondu d'amorçage.
	_graph = Graph.new()
	_graph.physics_enabled = false
	_graph.enable_start_up_modulation = false
	_graph.drag_input = false
	var line := GraphSoftLine.new()
	line.connection_color = Color(0.70, 0.66, 0.58, 0.85)
	line.connection_width = 2.0
	_graph.graph_lines = line
	_graph.position = Vector2(0, HEAD)
	_graph.size = Vector2(PW, PH - HEAD - FOOT - METAH)

	var center: Vector2 = _graph.size * 0.5
	var max_r: float = minf(_graph.size.x, _graph.size.y) * 0.5 - 36.0
	var ring := max_r / 6.0

	# anti-chevauchement : on regroupe par (quadrant, tier) et on évente tangentiellement
	var bucket := {}
	for i in nodes.size():
		var key := Vector2i(int(nodes[i]["quarter"]), int(nodes[i]["tier"]))
		if not bucket.has(key):
			bucket[key] = []
		bucket[key].append(i)

	# créer les Atomes (AVANT d'attacher le Graph → son _ready les recensera)
	_atoms.resize(nodes.size())
	for key in bucket:
		var members: Array = bucket[key]
		var q: int = key.x
		var tier: int = key.y
		var base_ang := (float(q) + 0.5) / 9.0 * TAU - PI * 0.5
		var r := (float(tier) + 0.7) * ring
		var m := members.size()
		for j in m:
			var idx: int = members[j]
			var nd: Dictionary = nodes[idx]
			var rad := Vector2(cos(base_ang), sin(base_ang))
			var tan := Vector2(-rad.y, rad.x)
			var off := (float(j) - float(m - 1) * 0.5) * 26.0
			var target: Vector2 = center + rad * r + tan * off
			var atom = _make_atom(nd, target)
			_graph.add_child(atom)
			_atoms[idx] = atom
			_info[atom] = nd

	add_child(_graph)                       # → _ready du Graph : il recense les Atomes
	# arêtes : chaque nœud relié à son prérequis (indice dans CE tableau)
	for i in nodes.size():
		var pr := int(nodes[i].get("prereq", -1))
		if pr >= 0 and pr < _atoms.size() and _atoms[i] != null and _atoms[pr] != null:
			_graph.connect_atoms(_atoms[pr], _atoms[i])
	if _graph.has_signal("atom_selected") and not _graph.atom_selected.is_connected(_on_atom_selected):
		_graph.atom_selected.connect(_on_atom_selected)
	_built = true
	queue_redraw()

func _make_atom(nd: Dictionary, target: Vector2):
	var atom = Atom.new()
	var d = AtomData.new()
	d.id = StringName("tech_%s" % String(nd["name"]))
	d.title = String(nd["name"])
	atom.data = d
	atom.is_static = true
	atom.drag_input = false
	var rr := 13.0
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
		var ms := rr * 1.5
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
			accept_event()
			return

# ── chrome du panneau : fond + en-tête + pied (le graphe se dessine seul) ───
func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	VKit.panel_bg(self, Rect2(0, 0, PW, PH))
	_draw_tier_rings()          # guides de TIER (rayon = profondeur) sous le graphe — lisibilité
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
func _draw_tier_rings() -> void:
	if _graph == null or not is_instance_valid(_graph):
		return
	var c: Vector2 = _graph.position + _graph.size * 0.5
	var max_r: float = minf(_graph.size.x, _graph.size.y) * 0.5 - 36.0
	if max_r <= 0.0:
		return
	var ring := max_r / 6.0
	for t in range(1, 6):
		var r := (float(t) + 0.7) * ring
		draw_arc(c, r, 0.0, TAU, 64, Color(0.58, 0.52, 0.42, 0.16), 1.0, true)
		VKit.text(self, Vector2(c.x - 7.0, c.y - r - 11.0), VKit.COL_DIM, "T%d" % t, VKit.FS_SMALL)

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
		"Métabolisation : +%d%% recherche (le creuset digéré)" % mp, VKit.FS_SMALL)
	if Sim.world == null:
		return

	# ── Rangée 1 : ACCÈS AUX SIGNATURES (tech, heritage_access — pas la victoire) ──
	var acc: Array = Sim.world.heritage_access()
	var n := acc.size()
	if n > 0:
		var cw := (PW - 28.0) / float(n)
		var ry := y0 + 20.0
		VKit.text(self, Vector2(16, ry - 10), VKit.COL_DIM, "Accès aux signatures (arbre) :", VKit.FS_SMALL)
		for i in n:
			var h: Dictionary = acc[i]
			var x := 16.0 + i * cw
			var nm := String(h.get("nom", ""))
			if nm.length() > 8:
				nm = nm.substr(0, 8)
			var nativ := bool(h.get("native", false))
			var tier := int(h.get("tier", 0))
			var mark := "★ " if nativ else ""
			VKit.text(self, Vector2(x, ry), (VKit.COL_GOLD if nativ else VKit.COL_PARCH),
				mark + nm, VKit.FS_SMALL)
			for k in 3:                                   # 3 pips de tier (rempli = accessible)
				VKit.fill(self, Rect2(x + k * 9.0, ry + 14, 6, 6),
					(COL_UNLOCKED if tier > k else COL_LOCKED))
			var dp := int(h.get("digested_pct", 0))
			if dp > 0 and not nativ:                       # la digestion EN COURS (dénominateur : pop totale)
				VKit.text(self, Vector2(x + 32, ry + 13), VKit.COL_DIM, "%d%%" % dp, VKit.FS_SMALL)

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
	var ry2 := y0 + 46.0
	var req_txt := (" — requis palier : %d" % mreq) if mreq > 0 else ""
	VKit.text(self, Vector2(16, ry2 - 10), VKit.COL_GOLD,
		"Compte pour l'Ascension : %d/%d%s" % [mcount, m, req_txt], VKit.FS_SMALL)
	for i in m:
		var h2: Dictionary = mh[i]
		var x2 := 16.0 + i * cw2
		var nm2 := String(h2.get("nom", ""))
		if nm2.length() > 8:
			nm2 = nm2.substr(0, 8)
		var nativ2 := bool(h2.get("native", false))
		var metab2: bool = bool(h2.get("metabolized", false))
		var voie2 := String(h2.get("voie", ""))
		var mark2 := "★" if nativ2 else ("✓" if metab2 else "·")
		VKit.text(self, Vector2(x2, ry2), (VKit.COL_GOLD if (nativ2 or metab2) else VKit.COL_PARCH),
			mark2 + " " + nm2, VKit.FS_SMALL)
		if not nativ2 and voie2 != "":
			VKit.text(self, Vector2(x2, ry2 + 13), VKit.COL_DIM, voie2, VKit.FS_SMALL)
