extends Control
## TechPanel — l'arbre de technologie du JOUEUR (read-only sauf le clic « recherche »),
## bascule touche T. Rendu en TROIS COULOIRS façon Civ 6 : Forge · Société · Savoir,
## empilés en bandes HORIZONTALES de hauteur FIXE (jamais de scroll vertical) ; les
## TIERS (0-5) sont des COLONNES qui défilent LATÉRALEMENT (ScrollContainer horizontal
## seul). Chaque nœud = une CARTE compacte (titre + petites icônes d'effet) ; le détail
## chiffré (tous les effets) vit au SURVOL (tooltip natif) — jamais de flavor dans la
## carte ni son dossier. Une flèche légère relie chaque carte à son prérequis.
## Quand une recherche S'ACHÈVE, un POPUP (tech_popup.gd) affiche effets + flavor —
## la SEULE apparition du flavor dans tout ce panneau.
## En-tête : points · présage (bande) · crise % · recherche en cours. Pied : dossier du
## nœud sélectionné + bande de MÉTABOLISATION (inchangée). Lit tech_info/tech_nodes/
## heritage_access/merv_metab/research_status (la façade). ACTIONNABLE : cliquer une
## carte RECHERCHABLE la fixe comme cible (player_research).

const VKit  = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
const ParchTheme = preload("res://ui/parch_theme.gd")
const Concepts = preload("res://ui/concepts.gd")   # D4 — glossaire hover

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
const FOOT := 128.0         # dossier persistant : nom/état/méta/effets (SANS flavor)
const METAH := 92.0         # bande de MÉTABOLISATION (le +% recherche + accès par héritage + compte Ascension)

# géométrie des COULOIRS (Civ 6) : 3 bandes de hauteur FIXE, colonnes = TIERS qui défilent
# latéralement. Aucun de ces nombres ne dépend du nombre de nœuds en hauteur — seule la
# LARGEUR (nombre de sous-colonnes par tier) grandit avec le contenu.
const LANE_LABEL_W := 66.0  # colonne de gauche FIGÉE (ne défile pas avec le scroll latéral)
const TIER_LABEL_H := 18.0  # bandeau "T0..T5" en tête de la zone scrollable
const CARD_GUTTER  := 8.0
# ENCARTS (2026-08-26, lot13_techs) : la carte source est 512×256 (2:1 EXACT) — on la
# montre à taille FIXE (ni étirée ni écrasée, "nette" comme demandé) et on choisit le
# nombre de rangées empilées par sous-colonne à PARTIR de cette taille (inverse de
# l'ancien calcul qui étirait card_h pour remplir le couloir).
const CARD_W       := 192.0
const CARD_H       := 96.0

# couleurs d'état (sans bibliothèque d'animation Medusa : on teinte la carte)
const COL_LOCKED   := Color(0.40, 0.40, 0.46)
const COL_AVAIL    := Color(0.85, 0.60, 0.28)
const COL_UNLOCKED := Color(0.40, 0.80, 0.46)
const COL_FAUST    := Color(0.88, 0.24, 0.24)

const LANE_NAMES := ["Savoir", "Forge", "Société"]           # ordre THM_* (scps_tech.h)
const LANE_INK := [Color(0.35, 0.45, 0.62, 0.85), Color(0.66, 0.34, 0.22, 0.85), Color(0.45, 0.55, 0.30, 0.85)]

## ── ENCART Civ 6 (2026-08-26) : la carte illustrée lot13_techs EST le corps du
## nœud (plus de médaillon + titre nu) ; bandeau bas discret (nom + état) plaqué
## par-dessus. Hit-test = tout le rect (Control par défaut), pas de flavor, tout
## le détail chiffré reste au SURVOL (tooltip_text, posé par _make_card, inchangé).
## Chargement PARESSEUX par visibilité (cf. _ensure_tex) : 74 cartes 512×256 ≈
## 37 Mo — seuls les encarts dans/près de la fenêtre du scroll tirent leur PNG.
class TechCard extends Control:
	const CVKit  = preload("res://ui/vkit.gd")
	const CUIKit = preload("res://ui/uikit.gd")
	const CParch = preload("res://ui/parch_theme.gd")
	signal activated(idx: int)
	var idx := -1
	var title := ""
	var state_num := 0        # 0 verrouillé · 1 recherchable · 2 acquis (miroir readout)
	var faustian := false
	var icons: Array = []
	var host_scroll: ScrollContainer = null   # fenêtre visible pour le chargement paresseux
	var card_tex: Texture2D = null
	var tex_checked := false  # a-t-on déjà tenté le chargement (réussi ou raté) ?

	const COL_LOCKED   := Color(0.40, 0.40, 0.46)
	const COL_AVAIL    := Color(0.85, 0.60, 0.28)
	const COL_UNLOCKED := Color(0.40, 0.80, 0.46)
	const COL_FAUST    := Color(0.88, 0.24, 0.24)

	func _ready() -> void:
		mouse_filter = Control.MOUSE_FILTER_STOP

	func _gui_input(e: InputEvent) -> void:
		if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
			accept_event()
			activated.emit(idx)

	## n'ESSAIE qu'une fois : dès que le rect global de la carte touche (± une
	## largeur de carte, anticipe le scroll) le rect visible du ScrollContainer,
	## on tire le PNG (cache UIKit) et on ne retente plus jamais pour cette carte.
	func _ensure_tex() -> void:
		if tex_checked:
			return
		if host_scroll != null and is_instance_valid(host_scroll):
			var vis: Rect2 = host_scroll.get_global_rect().grow(size.x)
			if not vis.intersects(get_global_rect()):
				return   # pas encore visible : un futur _draw() (scroll → queue_redraw) réessaiera
		tex_checked = true
		card_tex = CUIKit.tech_card(idx)

	func _draw() -> void:
		_ensure_tex()
		var r := Rect2(Vector2.ZERO, size)
		# ÉTAT sur l'illustration : VERROUILLÉE = modulate sombre (~55%, PAS une autre
		# texture) ; DISPONIBLE/ACQUISE = pleine (modulate blanc).
		var mod := Color(0.45, 0.45, 0.45, 1.0) if state_num == 0 else Color(1, 1, 1, 1)
		if card_tex != null:
			draw_texture_rect(card_tex, r, false, mod)
		else:
			CVKit.fill(self, r, Color(0.30, 0.28, 0.24, 0.35))   # repli : pas encore chargée/absente

		# LISERÉ d'état : gris fin (verrouillé) · or épais (recherchable) · or PULSÉ
		# (en cours — la cible de research_status()) · vert fin (acquise). Faustien
		# teinte le liseré vers le rouge, sauf verrouillé (rien à signaler encore).
		var rs: Dictionary = Sim.world.research_status() if Sim.world != null else {}
		var researching: bool = state_num == 1 and int(rs.get("target", -1)) == idx
		var edge := COL_LOCKED
		var edge_w := 1.0
		if researching:
			var t := float(Time.get_ticks_msec() % 1200) / 1200.0
			edge = COL_AVAIL.lerp(Color(1, 0.95, 0.75), 0.35 + 0.35 * sin(t * TAU))
			edge_w = 3.0
		elif state_num == 1:
			edge = COL_AVAIL
			edge_w = 2.5
		elif state_num == 2:
			edge = COL_UNLOCKED
			edge_w = 1.5
		if faustian and state_num != 0:
			edge = edge.lerp(COL_FAUST, 0.55)
		draw_rect(r, edge, false, edge_w)

		# BANDEAU BAS discret : nom (STR_* déjà résolu côté moteur, `title`) + pastille
		# d'état — jamais de flavor ici (motif du dossier pied, inchangé). MÊME langage
		# chrome que le reste du panneau (HEADER_BG tan + encre sombre) — pas un verre
		# noir : sur certaines illustrations sombres (encre gravée), un bandeau NOIR sur
		# encre sombre + texte COL_PARCH (sombre lui aussi) devenait illisible.
		var band_h: float = clampf(size.y * 0.24, 16.0, 24.0)
		var band := Rect2(0.0, size.y - band_h, size.x, band_h)
		var bg := CParch.HEADER_BG
		CVKit.fill(self, band, Color(bg.r, bg.g, bg.b, 0.55 if state_num == 0 else 0.88))
		var tcol := CVKit.COL_DIM if state_num == 0 else CVKit.COL_PARCH
		CVKit.text_wrapped(self, Vector2(6.0, size.y - band_h + 2.0), tcol, title, size.x - 34.0, 1, CVKit.FS_SMALL)
		var dotc := COL_LOCKED
		if researching:
			dotc = edge
		elif state_num == 2:
			dotc = COL_UNLOCKED
		elif state_num == 1:
			dotc = COL_AVAIL
		CVKit.fill(self, Rect2(size.x - 20.0, size.y - band_h * 0.5 - 4.0, 8.0, 8.0), dotc)

		# BARRE FINE de progression (EN COURS) — sous le nom, dans le bandeau ; la
		# donnée existe déjà au panneau (research_status), rien de neuf à sérialiser.
		if researching:
			var prog := clampf(float(rs.get("progress", 0.0)), 0.0, 1.0)
			var bx := 6.0
			var bw2 := size.x - 34.0
			var by2 := size.y - 5.0
			CVKit.fill(self, Rect2(bx, by2, bw2, 3.0), Color(0, 0, 0, 0.5))
			CVKit.fill(self, Rect2(bx, by2, bw2 * prog, 3.0), COL_AVAIL)

		# icônes d'effet (glyphes compacts, hérités de l'ancienne carte) — coin
		# haut-gauche, sur puce sombre pour rester lisibles sur l'illustration.
		if icons.size() > 0:
			CVKit.fill(self, Rect2(2.0, 2.0, 14.0 * icons.size() + 4.0, 16.0), Color(0, 0, 0, 0.5))
			var ix := 4.0
			for nm in icons:
				CUIKit.draw_icon(self, String(nm), Vector2(ix, 3.0), 12.0, Color(1, 1, 1, 0.55 if state_num == 0 else 1.0))
				ix += 14.0

var _cards: Array = []     # idx nœud → TechCard (ou null)
var _info := {}            # TechCard -> dict du nœud (non utilisé pour le clic, gardé pour le focus)
var _nodes := []           # le tableau de nœuds (lookup index → nom pour la cible)
var _pos := {}             # idx nœud → centre (coordonnées CONTENU, pour les arêtes + le focus)
var _built := false
var _sel := ""             # détail du nœud sélectionné (pied)
var _sel_node := {}        # nœud sélectionné : dossier détaillé, sans dépendre du survol
var _sel_flash := ""       # retour immédiat après lancement/refus d'une recherche
var _close_rect := Rect2()
# CHROME parchemin (motif ParchTheme, cf. construction_panel/province_panel_v2) — LA
# STRUCTURE Civ 6 ne bouge pas, seuls cadre/bandeau/pied portent le même style que les
# fiches à conteneurs natifs (bords 1px BORDER, bandeaux HEADER_BG). Styleboxes CACHÉES
# (motif VKit._pb_body) : `_draw()` tourne à chaque tick visible, pas de ré-allocation.
static var _sb_panel: StyleBoxFlat = null
static var _sb_band: StyleBoxFlat = null
static func _chrome() -> void:
	if _sb_panel != null:
		return
	_sb_panel = ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.BORDER, 1, 3, 0, 0, 0, 0)
	_sb_band = ParchTheme.sb(ParchTheme.HEADER_BG, ParchTheme.BORDER, 0, 0, 0, 0, 0, 0)
var _popup: Control = null                 ## le popup de DÉCOUVERTE (tech_popup.gd), enfant persistant
var _pending_discoveries: Array = []       ## idx de nœuds achevés pendant que le panneau était FERMÉ
var _last_research_target := -1           ## suivi de la cible — détecte la complétion (miroir sound.gd)
var _last_research_prog := 0.0
var _tips: Array = []   ## D4 — [ [Rect2, définition], … ] reconstruit à chaque _draw (motif country_panel.gd)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	size = Vector2(PW, PH)
	_popup = load("res://ui/tech_popup.gd").new()
	add_child(_popup)
	if _popup.has_signal("closed"):
		_popup.closed.connect(_on_popup_closed)
	_layout()
	get_viewport().size_changed.connect(_layout)
	visibility_changed.connect(_on_visibility)
	Sim.generated.connect(_on_generated)
	Sim.ticked.connect(func(_y):
		_check_metab_ready()          # surveille le franchissement de tier — même panneau FERMÉ
		_check_research_complete()    # surveille la complétion d'une recherche — même panneau FERMÉ
		if visible:
			queue_redraw()
			# la SEULE carte dont l'état interne bouge sans reconstruction complète du
			# panneau : celle EN COURS (liseré pulsé + barre de progression) — on ne
			# réarme que celle-là, les autres restent statiques jusqu'au prochain _build.
			if Sim.world != null and Sim.world.has_method("research_status"):
				var rt := int(Sim.world.research_status().get("target", -1))
				if rt >= 0 and rt < _cards.size() and _cards[rt] != null:
					_cards[rt].queue_redraw()
	)
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

## surveille research_status() : la CIBLE change ⇒ la précédente s'est achevée (≥85 % de
## progression, même seuil que sound.gd/tech_notif — la recherche annulée à bas % ne compte
## pas comme une découverte). Le popup ne doit interrompre AUCUN input critique : si le
## panneau est FERMÉ, la découverte est mise en attente et s'ouvre au prochain visible=true.
func _check_research_complete() -> void:
	if Sim.world == null or not Sim.world.has_method("research_status"):
		return
	var rs: Dictionary = Sim.world.research_status()
	var rt := int(rs.get("target", -1))
	var prog := float(rs.get("progress", 0.0))
	if _last_research_target >= 0 and rt != _last_research_target and _last_research_prog >= 0.85:
		_queue_discovery(_last_research_target)
	_last_research_target = rt
	_last_research_prog = prog

func _queue_discovery(idx: int) -> void:
	_pending_discoveries.append(idx)
	if visible:
		_flush_pending_discoveries()

func _flush_pending_discoveries() -> void:
	if _popup == null or _pending_discoveries.is_empty():
		return
	if _popup.visible:
		return   # un popup à la fois — le suivant s'ouvrira à la fermeture de celui-ci
	var idx: int = _pending_discoveries.pop_front()
	if Sim.world == null:
		return
	var nodes: Array = Sim.world.tech_nodes()
	if idx < 0 or idx >= nodes.size():
		return
	move_child(_popup, get_child_count() - 1)   # toujours au-dessus (cartes/scroll ajoutés depuis)
	_popup.show_tech(nodes[idx])

func _on_popup_closed() -> void:
	_flush_pending_discoveries()

func _layout() -> void:
	var vp := get_viewport_rect().size
	var pw0 := PW
	var ph0 := PH
	# TRÈS grand format (retour joueur 2026-07-10 : « agrandis sérieusement ») — la
	# géométrie des cartes est FIXE et généreuse, la LARGEUR défile (scroll latéral).
	# Centré ENTRE le rail gauche et le ledger droit (il passait sous le ledger).
	var free_x0 := Frame.SIDEBAR_W + 8.0
	var free_x1 := vp.x - Frame.LEDGER_W - 8.0
	PW = clampf((free_x1 - free_x0) * 0.98, 900.0, 1780.0)
	PH = clampf(vp.y * 0.92, 620.0, 1200.0)
	size = Vector2(PW, PH)
	position = Vector2(free_x0 + (free_x1 - free_x0 - PW) * 0.5, (vp.y - PH) * 0.5)
	if _built and (absf(PW - pw0) > 1.0 or absf(PH - ph0) > 1.0):
		_built = false                       # la grille se rebâtit à la nouvelle taille
		if visible:
			_build()

func _on_visibility() -> void:
	if visible and not _built and Sim.world != null:
		_build()
	if visible:
		_requeue_card_draws()   # réarme le chargement paresseux à chaque réouverture
		_flush_pending_discoveries()
	else:
		_release_card_textures()   # perf (mission) : purge les 74 encarts à la FERMETURE

## réarme le _draw() de toutes les cartes — motif partagé par le hook de scroll
## (drag/molette révèle des cartes hors-écran) et par la réouverture du panneau
## (textures purgées à la fermeture, cf. _release_card_textures).
func _requeue_card_draws() -> void:
	for c in _cards:
		if c != null:
			c.queue_redraw()

## PERF (mission) : 74 encarts 512×256 ≈ 37 Mo — on ne les garde en VRAM que
## panneau OUVERT. `tex_checked=false` fait retenter _ensure_tex() au prochain
## _draw() (donc au prochain open, via _requeue_card_draws) plutôt que de rester
## bloqué sur un ancien échec/succès qui n'a plus de texture derrière.
func _release_card_textures() -> void:
	for c in _cards:
		if c != null:
			c.card_tex = null
			c.tex_checked = false
	UIKit.tech_card_release_all()

func _on_generated() -> void:
	_built = false
	_sel = ""
	_sel_node.clear()
	_sel_flash = ""
	_metab_seen.clear()
	_pending_discoveries.clear()
	_last_research_target = -1
	_last_research_prog = 0.0
	if _popup != null:
		_popup.visible = false
	if visible:
		_build()

## P5 : une technologie trouvée par Ctrl+K ouvre directement son dossier sans
## déclencher la recherche. Le clic volontaire sur la carte reste le seul actionneur.
func focus_tech(id: int) -> void:
	if not visible:
		visible = true
	if not _built:
		_build()
	if id < 0 or id >= _nodes.size():
		return
	_sel_node = (_nodes[id] as Dictionary).duplicate(true)
	_sel = String(_sel_node.get("name", ""))
	_sel_flash = "Ouvert depuis la recherche universelle."
	if _scroll != null and id < _cards.size() and _cards[id] != null:
		_scroll.ensure_control_visible(_cards[id])
	queue_redraw()

# ── construction de la grille Civ 6 (3 couloirs × colonnes de tiers) ───────────────
func _build() -> void:
	if Sim.world == null:
		return
	if _scroll != null and is_instance_valid(_scroll):
		_scroll.queue_free()
	_cards.clear()
	_info.clear()
	_pos.clear()

	var nodes: Array = Sim.world.tech_nodes()
	_nodes = nodes
	if nodes.is_empty():
		_built = true
		return
	_cards.resize(nodes.size())

	# regroupement par (couloir l, tier t) — l = quartier / 3 (3 quartiers par couloir)
	var tiers_set := {}
	var cell := {}   # clé l*100+t → Array[idx]
	for i in nodes.size():
		var t := int(nodes[i]["tier"])
		var l := int(float(int(nodes[i]["quarter"])) / 3.0)
		tiers_set[t] = true
		var key := l * 100 + t
		var arr: Array = cell.get(key, [])
		arr.append(i)
		cell[key] = arr
	var tiers: Array = tiers_set.keys()
	tiers.sort()

	var view_w := PW - 24.0
	var lane_area_h: float = PH - HEAD - FOOT - METAH
	var usable_h: float = maxf(lane_area_h - TIER_LABEL_H, 60.0)
	_lane_h = usable_h / 3.0
	_lane_y0 = [TIER_LABEL_H, TIER_LABEL_H + _lane_h, TIER_LABEL_H + 2.0 * _lane_h]
	# ENCARTS Civ 6 : taille FIXE 192×96 (2:1, l'aspect exact de la source 512×256 —
	# on ne l'étire NI ne l'écrase). rows_per_cell = combien en tiennent, PAS l'inverse
	# (l'ancien calcul stretchait card_h pour remplir le couloir ; les cartes illustrées
	# doivent rester nettes) — AUCUN scroll vertical : le surplus déborde en LARGEUR.
	var lane_avail_h: float = _lane_h - 8.0
	var rows_per_cell: int = clampi(int(floor((lane_avail_h + CARD_GUTTER) / (CARD_H + CARD_GUTTER))), 1, 3)
	var card_h := CARD_H
	var card_w := CARD_W

	# le SCROLL LATÉRAL SEUL : la colonne des noms de couloir reste FIGÉE à gauche
	# (dessinée hors du scroll, cf. _draw) ; content_w grandit avec le nombre de tiers/
	# sous-colonnes, content_h == lane_area_h PILE (jamais plus → aucun scroll vertical).
	_scroll = ScrollContainer.new()
	_scroll.position = Vector2(12.0 + LANE_LABEL_W, HEAD)
	_scroll.size = Vector2(view_w - LANE_LABEL_W, lane_area_h)
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	add_child(_scroll)
	# CHARGEMENT PARESSEUX par visibilité : le drag/molette latéral ne redessine pas
	# les cartes de lui-même (Control ne redraw que sur queue_redraw explicite) — on
	# réarme toutes les cartes à chaque défilement pour qu'elles retestent leur propre
	# visibilité (_ensure_tex) ; déjà chargées → le test est un simple intersects().
	var hbar := _scroll.get_h_scroll_bar()
	if hbar != null:
		hbar.value_changed.connect(func(_v): _requeue_card_draws())
	var content := Control.new()
	_scroll.add_child(content)
	_bg = Control.new()
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_child(_bg)
	_bg.draw.connect(_draw_bg)

	# placement TIER PAR TIER (gauche→droite) : chaque tier occupe autant de
	# SOUS-COLONNES que le couloir le plus chargé en réclame (bornées à `rows_per_cell`
	# cartes empilées par sous-colonne) — les séparateurs de tier restent alignés
	# entre les 3 couloirs. Anti-croisement : tri par la position Y du prérequis déjà posé.
	var rowpos := {}   # idx → Y déjà posé (lu par les tiers suivants)
	_tier_x.clear()
	_tier_w.clear()
	var x_cursor := 10.0
	for t in tiers:
		var lane_lists := {}
		var subcols := 1
		for l in range(3):
			var arr: Array = cell.get(l * 100 + int(t), [])
			if arr.is_empty():
				continue
			arr = arr.duplicate()
			arr.sort_custom(func(a, b):
				var ka: float = rowpos.get(int(nodes[a].get("prereq", -1)), 1e9)
				var kb: float = rowpos.get(int(nodes[b].get("prereq", -1)), 1e9)
				if absf(ka - kb) > 0.01:
					return ka < kb
				return String(nodes[a]["name"]) < String(nodes[b]["name"]))
			lane_lists[l] = arr
			subcols = maxi(subcols, int(ceil(float(arr.size()) / float(rows_per_cell))))
		var tier_w: float = float(subcols) * card_w + float(subcols - 1) * CARD_GUTTER
		_tier_x[int(t)] = x_cursor
		_tier_w[int(t)] = tier_w
		for l in range(3):
			if not lane_lists.has(l):
				continue
			var arr2: Array = lane_lists[l]
			var lane_top: float = _lane_y0[l] + 4.0
			for j in range(arr2.size()):
				var idx: int = arr2[j]
				var subcol: int = j / rows_per_cell
				var row: int = j % rows_per_cell
				var start: int = subcol * rows_per_cell
				var n_in_subcol: int = mini(rows_per_cell, arr2.size() - start)
				var block_h: float = float(n_in_subcol) * card_h + float(n_in_subcol - 1) * CARD_GUTTER
				var by: float = lane_top + (lane_avail_h - block_h) * 0.5 + float(row) * (card_h + CARD_GUTTER)
				var bx: float = x_cursor + float(subcol) * (card_w + CARD_GUTTER)
				var card := _make_card(nodes[idx], idx, Vector2(bx, by), card_w, card_h)
				content.add_child(card)
				_cards[idx] = card
				_info[card] = nodes[idx]
				var center := Vector2(bx + card_w * 0.5, by + card_h * 0.5)
				_pos[idx] = center
				rowpos[idx] = center.y
		x_cursor += tier_w + 26.0   # respiration + place pour le séparateur/étiquette du tier suivant

	_content_w = x_cursor + 14.0
	content.custom_minimum_size = Vector2(_content_w, lane_area_h)
	content.size = Vector2(_content_w, lane_area_h)
	_bg.size = Vector2(_content_w, lane_area_h)
	_bg.position = Vector2.ZERO
	_built = true
	queue_redraw()

## géométrie de la grille (posée par _build, lue par _draw/_draw_bg)
var _lane_h := 0.0
var _lane_y0: Array = [0.0, 0.0, 0.0]
var _tier_x := {}
var _tier_w := {}
var _content_w := 0.0
var _scroll: ScrollContainer = null       ## la fenêtre scrollable (LATÉRALE seule)
var _bg: Control = null                   ## fond de couloirs/tiers + arêtes, DANS le scroll

func _make_card(nd: Dictionary, idx: int, pos: Vector2, w: float, h: float) -> TechCard:
	var card := TechCard.new()
	card.idx = idx
	card.position = pos
	card.size = Vector2(w, h)
	card.state_num = int(nd["state"])
	card.faustian = bool(nd.get("faustian", false))
	card.title = String(nd["name"])
	card.icons = _effect_icons(nd)
	card.host_scroll = _scroll   # fenêtre visible pour le chargement paresseux de l'encart
	card.tooltip_text = _tooltip_for(nd)
	card.activated.connect(_on_card_activated)
	return card

## mots-clés → icône (le mot mécanique du hover chiffré, PAS le flavor) — la carte
## affiche 1-3 glyphes compacts, le détail chiffré complet reste au SURVOL (tooltip).
const ICON_KEYWORDS := [
	["prospérité", "gold_coin"],
	["stabilité", "laurel_success"],
	["coercition", "politics_crown"],
	["puissance", "action_recruit"],
	["fracture", "dipl_rivalry"],
	["production", "build_hammer"],
	["efficacité", "action_research"],
	["charge faustienne", "alert_warning"],
	["flux", "action_trade"],
]
func _effect_icons(nd: Dictionary) -> Array:
	var hov := String(nd.get("hover", "")).to_lower()
	var out := []
	for pair in ICON_KEYWORDS:
		if hov.find(String(pair[0])) >= 0:
			out.append(String(pair[1]))
			if out.size() >= 3:
				break
	return out

## le détail COMPLET (plusieurs effets) — UNIQUEMENT au survol, JAMAIS de flavor ici.
func _tooltip_for(nd: Dictionary) -> String:
	var tip := String(nd["name"])
	var st := int(nd["state"])
	var states := ["Verrouillée", "Recherchable", "Acquise"]
	tip += "\n%s · palier %d" % [states[clampi(st, 0, 2)], int(nd.get("tier", 0))]
	if not bool(nd.get("allowed", st == 1)) and st != 2:
		tip += "\nPourquoi : %s" % String(nd.get("reason_label", "Indisponible"))
	var pr := int(nd.get("prereq", -1))
	if pr >= 0 and pr < _nodes.size():
		tip += "\nPrérequis : %s" % String(_nodes[pr].get("name", "?"))
	if int(nd.get("steps_remaining", 0)) > 1:
		tip += "\nChemin : %s" % String(nd.get("path_label", ""))
	if int(nd.get("cost", 0)) > 0 and st != 2:
		tip += " · %d points (réserve %d, manque %d)" % [int(nd["cost"]),
			int(nd.get("points_have", 0)), int(nd.get("points_missing", 0))]
	var effet := String(nd.get("effet", ""))
	if effet != "":
		tip += "\nEffet : " + effet
	var hov := String(nd.get("hover", ""))
	if hov != "":
		tip += "\nDétail : " + hov
	var unl := String(nd.get("unlocks", ""))
	if unl != "":
		tip += "\nDébouche sur : " + unl
	return tip

## D4 — HOVER natif (motif country_panel.gd) : Godot appelle ceci au survol de tout
## point du panneau sans Control dédié → on rend la définition de la zone touchée.
func _get_tooltip(at_position: Vector2) -> String:
	for t in _tips:
		if (t[0] as Rect2).has_point(at_position) and String(t[1]) != "":
			return String(t[1])
	return ""

func _on_card_activated(idx: int) -> void:
	if idx < 0 or idx >= _nodes.size():
		return
	var nd: Dictionary = _nodes[idx]
	var states := ["verrouillé", "recherchable", "acquis"]
	var stt := int(nd["state"])
	_sel_node = nd.duplicate()
	_sel_flash = ""
	_sel = "%s — %s · %s" % [String(nd["name"]), String(nd.get("effet", "")), states[clampi(stt, 0, 2)]]
	if int(nd.get("cost", 0)) > 0 and stt != 2:
		_sel += " (%d pts)" % int(nd["cost"])
	# ACTIONNABLE : la décision structurée du moteur commande le clic ; l'UI ne
	# reconstruit ni l'accès d'héritage, ni les ruines, ni l'âge, ni les prérequis.
	# (l'indice de _cards == TechId ; la façade enfile CMD_RESEARCH, le déblocage tombe au tick).
	if bool(nd.get("allowed", stt == 1)) and Sim.world != null:
		var ok: bool = Sim.world.player_research(idx) != 0
		_sel_flash = "Recherche lancée : %s" % String(nd["name"]) if ok else "Recherche refusée : la situation a changé."
		Sound.play("ui_click")
		Sim.notify_action()
	queue_redraw()

func _gui_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_LEFT:
		if _close_rect.has_point(e.position):
			visible = false
			Sound.play("ui_parchment_close")
			accept_event()
			return

# ── chrome du panneau : fond + en-tête + couloirs figés + pied ──────────────
func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	_chrome()
	_tips.clear()   # D4 — repeuplé plus bas par ce même _draw() (motif country_panel.gd)
	# CADRE : mêmes bords que les fiches à conteneurs natifs (1px BORDER, coin 3) —
	# plus de plaque RimWorld (ombre/grain/filet couronnes) : le FRÈRE des fiches, pas une
	# fenêtre à part.
	draw_style_box(_sb_panel, Rect2(0, 0, PW, PH))
	var info: Dictionary = w.tech_info()
	# BANDEAU d'en-tête (motif HeaderStrip) : la même bande HEADER_BG que les fiches,
	# le titre au corps SERIF (font_map, comme la variation "Title" de ParchTheme).
	draw_style_box(_sb_band, Rect2(0, 0, PW, HEAD))
	UIKit.draw_icon(self, "knowledge_book", Vector2(14, 12), 20)
	VKit.text_map(self, Vector2(42, 13), "Arbre de technologie", VKit.FS_BIG, ParchTheme.HEADER_INK, 0)

	# ✕ — tout panneau se ferme (Échap le ferme aussi via main)
	_close_rect = Rect2(PW - 26, 6, 20, 20)
	VKit.fill(self, _close_rect, VKit.COL_PANEL)
	VKit.box(self, _close_rect, VKit.COL_EDGE)
	VKit.text(self, Vector2(_close_rect.position.x + 6, _close_rect.position.y + 3), VKit.COL_PARCH, "x")

	var pts_lbl_w: float = VKit.detail(self, Vector2(PW - 250, 13), "Points : ", VKit.FS_SMALL)
	VKit.value(self, Vector2(PW - 250 + pts_lbl_w, 13), str(int(info.get("points", 0))), VKit.FS_SMALL)
	var crise := int(info.get("crise_pct", 0))
	var pcol := VKit.COL_DIM if crise < 25 else (VKit.sense(0.40) if crise < 60 else VKit.sense(0.10))
	VKit.text(self, Vector2(PW - 250, 30), pcol,
		"Présage : %s (crise %d%%)" % [String(info.get("presage", "")), crise], VKit.FS_SMALL)
	# RECHERCHE EN COURS : la cible + sa jauge de progression (research_status)
	# (x=380 : le titre SERIF est plus large que l'ancien Alegreya — 220 le chevauchait)
	var rs: Dictionary = w.research_status()
	var rt := int(rs.get("target", -1))
	if rt >= 0 and rt < _nodes.size():
		var rname := String(_nodes[rt].get("name", "?"))
		var prog := clampf(float(rs.get("progress", 0.0)), 0.0, 1.0)
		VKit.text(self, Vector2(380, 13), VKit.COL_GOLD, "Recherche : %s" % rname, VKit.FS_SMALL)
		var bx := 380.0
		var bw := 200.0
		VKit.box(self, Rect2(bx, 30, bw, 9), VKit.COL_DIM)
		VKit.fill(self, Rect2(bx + 1, 31, (bw - 2) * prog, 7), COL_UNLOCKED)
		VKit.value(self, Vector2(bx + bw + 8, 30), "%d%%" % int(prog * 100.0), VKit.FS_SMALL)
	else:
		VKit.text(self, Vector2(380, 13), VKit.COL_DIM, "Recherche : (cliquez une carte recherchable)", VKit.FS_SMALL)
	# légende d'état
	VKit.text(self, Vector2(14, 33), COL_UNLOCKED, "● acquis", VKit.FS_SMALL)
	VKit.text(self, Vector2(86, 33), COL_AVAIL, "● recherchable", VKit.FS_SMALL)
	VKit.text(self, Vector2(196, 33), COL_LOCKED, "● verrouillé", VKit.FS_SMALL)
	VKit.text(self, Vector2(286, 33), COL_FAUST, "● faustien", VKit.FS_SMALL)
	VKit.fill(self, Rect2(12, HEAD - 4, PW - 24, 1), VKit.COL_EDGE)

	# COULOIRS FIGÉS (ne défilent pas avec le scroll latéral) : nom + ruban de couleur,
	# alignés sur la géométrie posée par _build (_lane_y0/_lane_h).
	if _scroll != null:
		for l in range(3):
			var band_y: float = _scroll.position.y + _lane_y0[l] + 2.0
			VKit.fill(self, Rect2(10.0, band_y, 3.0, _lane_h - 4.0), LANE_INK[l])
			var ly: float = _scroll.position.y + _lane_y0[l] + _lane_h * 0.5 - 7.0
			VKit.text(self, Vector2(18.0, ly), VKit.COL_GOLD, LANE_NAMES[l], VKit.FS_SMALL)
			# D4 — le couloir « Savoir » nomme directement un concept du registre.
			if LANE_NAMES[l] == "Savoir":
				var lw: float = VKit.text_w(LANE_NAMES[l], VKit.FS_SMALL)
				_tips.append([Rect2(18.0, ly - 2.0, lw + 4.0, 16.0), Concepts.def_of("Savoir")])
		VKit.fill(self, Rect2(_scroll.position.x - 6.0, HEAD, 1.0, PH - HEAD - FOOT - METAH), VKit.COL_EDGE)

	# PIED (bandeau, motif HeaderStrip) : métabolisation + dossier persistant — le même
	# bandeau HEADER_BG que l'en-tête, pour que le panneau se referme comme une fiche
	# (bande haute / corps clair / bande basse), pas juste un fond uniforme.
	draw_style_box(_sb_band, Rect2(0, PH - FOOT - METAH, PW, FOOT + METAH))
	# bande de MÉTABOLISATION : le +% recherche + l'accès tech par héritage (la barre)
	_draw_metab(info)
	# pied : DOSSIER persistant de la carte cliquée. Le survol découvre ; le clic permet
	# de comparer sans garder la souris immobile (profondeur RimWorld/EU4). JAMAIS de flavor.
	VKit.fill(self, Rect2(12, PH - FOOT, PW - 24, 1), VKit.COL_EDGE)
	if not _sel_node.is_empty():
		_draw_selected_node(PH - FOOT + 7.0)
	else:
		VKit.text(self, Vector2(16, PH - FOOT + 8), VKit.COL_DIM,
			"Sélectionnez une carte : effets, coût, prérequis et débouchés resteront affichés ici.", VKit.FS_SMALL)

## fond de la zone scrollable : bandes de couloir alternées + séparateurs/étiquettes de
## tier + arêtes de prérequis (dessin léger — le nom du prérequis reste de toute façon
## nommé au survol si les lignes se croisent trop).
func _draw_bg() -> void:
	if _bg == null or not is_instance_valid(_bg):
		return
	var gs: Vector2 = _bg.size
	for l in range(3):
		if l % 2 == 1:
			VKit.fill(_bg, Rect2(0.0, _lane_y0[l], gs.x, _lane_h), Color(0.32, 0.27, 0.20, 0.10))
	var tiers: Array = _tier_x.keys()
	tiers.sort()
	for i in tiers.size():
		var t = tiers[i]
		var tx: float = _tier_x[t]
		var tw: float = _tier_w[t]
		if i > 0:
			VKit.fill(_bg, Rect2(tx - 13.0, TIER_LABEL_H, 1.0, gs.y - TIER_LABEL_H), Color(0.58, 0.52, 0.42, 0.16))
		VKit.text(_bg, Vector2(tx + tw * 0.5 - 8.0, 2.0), VKit.COL_DIM, "T%d" % int(t), VKit.FS_SMALL)
	# arêtes de prérequis : léger trait clair, sous les cartes
	for i in _nodes.size():
		var pr := int(_nodes[i].get("prereq", -1))
		if pr >= 0 and _pos.has(pr) and _pos.has(i):
			_bg.draw_line(_pos[pr], _pos[i], Color(0.70, 0.66, 0.58, 0.55), 1.5, true)

func _draw_selected_node(y: float) -> void:
	var nd: Dictionary = _sel_node
	var st := int(nd.get("state", 0))
	var states := ["VERROUILLÉE", "RECHERCHABLE · CLIQUER POUR LANCER", "ACQUISE"]
	if st == 0 and String(nd.get("reason_label", "")) != "":
		states[0] = "BLOQUÉE · %s" % String(nd.get("reason_label", ""))
	var scol := COL_LOCKED if st == 0 else (COL_AVAIL if st == 1 else COL_UNLOCKED)
	if bool(nd.get("faustian", false)):
		scol = COL_FAUST
	VKit.text(self, Vector2(16, y), VKit.COL_GOLD, String(nd.get("name", "?")), VKit.FS_BIG)
	VKit.text(self, Vector2(300, y + 2), scol, states[clampi(st, 0, 2)], VKit.FS_SMALL)
	var meta := "Palier %d" % int(nd.get("tier", 0))
	var pr := int(nd.get("prereq", -1))
	if pr >= 0 and pr < _nodes.size():
		meta += " · Prérequis : %s" % String(_nodes[pr].get("name", "?"))
	elif bool(nd.get("is_base", false)):
		meta += " · Fondation"
	if int(nd.get("cost", 0)) > 0 and st != 2:
		meta += " · Coût %d · réserve %d · manque %d" % [int(nd.get("cost", 0)),
			int(nd.get("points_have", 0)), int(nd.get("points_missing", 0))]
	VKit.text(self, Vector2(16, y + 22), VKit.COL_DIM, meta, VKit.FS_SMALL)
	var body_y := y + 39.0
	if int(nd.get("steps_remaining", 0)) > 1:
		var ns := int(nd.get("next_step", -1))
		var next_name := String(_nodes[ns].get("name", "?")) if ns >= 0 and ns < _nodes.size() else "?"
		var path := "Chemin suggéré : commencer par %s · %d étapes" % [next_name, int(nd.get("steps_remaining", 0))]
		VKit.text(self, Vector2(16, body_y), VKit.COL_PARCH, path, VKit.FS_SMALL)
		body_y += 17.0
	# EFFETS (plusieurs) — JAMAIS de flavor dans le dossier du menu (mission : le flavor
	# n'apparaît QUE dans le popup de découverte, cf. tech_popup.gd).
	var eff := String(nd.get("effet", ""))
	if eff != "":
		VKit.text(self, Vector2(16, body_y), VKit.COL_PARCH, "Effet : " + eff, VKit.FS_SMALL)
	var unl := String(nd.get("unlocks", ""))
	if unl != "":
		VKit.text(self, Vector2(PW * 0.52, body_y), VKit.COL_PARCH, "Débouche sur : " + unl, VKit.FS_SMALL)
	var hov := String(nd.get("hover", ""))
	if _sel_flash != "":
		VKit.text(self, Vector2(16, body_y + 19), COL_UNLOCKED if _sel_flash.begins_with("Recherche lancée") else COL_FAUST,
			_sel_flash, VKit.FS_SMALL)
	elif hov != "":
		VKit.text_wrapped(self, Vector2(16, body_y + 19), VKit.COL_DIM, "Détail : " + hov, PW - 32.0, 2, VKit.FS_SMALL)

# ── bande de MÉTABOLISATION : le +% recherche du creuset + l'accès tech par héritage ──
# Le "+X% recherche" répond à « métabolisation = +% tech visible sous la barre de savoir » ;
# les 6 barres (tier 0-3 en pips + part digérée) sont la « barre de progression par tier » :
# digérer un peuple OUVRE ses signatures (tier 1 commerce → tier 3 plein/métabolisé).
func _draw_metab(info: Dictionary) -> void:
	var y0 := PH - FOOT - METAH
	VKit.fill(self, Rect2(12, y0, PW - 24, 1), VKit.COL_EDGE)
	var mp := int(info.get("metab_pct", 0))
	UIKit.draw_icon(self, "knowledge_book", Vector2(14, y0 + 4), 14)
	var mlbl_w: float = VKit.detail(self, Vector2(36, y0 + 5), "Peuples intégrés : ", VKit.FS_SMALL)
	var mval_w: float = VKit.value(self, Vector2(36 + mlbl_w, y0 + 5), "+%d%%" % mp, VKit.FS_SMALL)
	var mtail_w: float = VKit.detail(self, Vector2(36 + mlbl_w + mval_w, y0 + 5), " de recherche", VKit.FS_SMALL)
	# D4 — « Peuples intégrés » EST la Métabolisation (le mot moteur, jamais montré tel
	# quel côté joueur — cf. doctrine cultures « jamais espèce/sphère »).
	_tips.append([Rect2(36.0, y0 + 3.0, mlbl_w + mval_w + mtail_w, 16.0), Concepts.def_of("Métabolisation")])
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
			# la BARRE de digestion (native = pleine, couronnes) + les pips de tier à sa droite
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
	var asc_lbl_w: float = VKit.detail(self, Vector2(16, ry2 - 12), "Compte pour l'Ascension : ", VKit.FS_SMALL)
	var asc_val_w0: float = VKit.value(self, Vector2(16 + asc_lbl_w, ry2 - 12), "%d/%d" % [mcount, m], VKit.FS_SMALL)
	var asc_row_w := asc_lbl_w + asc_val_w0
	if req_txt != "":
		var asc_val_w: float = VKit.text_w("%d/%d" % [mcount, m], VKit.FS_SMALL)
		var asc_tail_w: float = VKit.detail(self, Vector2(16 + asc_lbl_w + asc_val_w, ry2 - 12), req_txt, VKit.FS_SMALL)
		asc_row_w += asc_tail_w
	# D4 — glossaire hover : la seule jauge de victoire du panneau, sans autre survol.
	_tips.append([Rect2(16.0, ry2 - 14.0, asc_row_w, 16.0), Concepts.def_of("Ascension")])
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
