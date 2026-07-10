extends Control
## CultureCreator — la fenêtre « créateur d'empire » façon Stellaris, en ONGLETS
## (retour joueur 2026-07-10 : « dispatcher avec plusieurs onglets, expliquer,
## pas tout mettre dans les menus déroulants, expliquer combien ») :
##   · onglet HÉRITAGE  : la lignée (noms + accès natif à sa branche d'arbre) ;
##   · onglet ÉTHOS     : l'âme de l'État — « ça m'apporte quoi » dit en clair ;
##   · onglet TRADITIONS : une par AXE, en BOUTONS visibles avec leurs CHIFFRES
##                   (+2 majeur · +1 mineur · −1 défaut) — plus de déroulants.
## La GRAINE ne vit plus ici (écran Nouvelle partie) — le mode autonome régénère
## sur la graine COURANTE (Sim.current_seed).
##
## RÈGLE D'OR : zéro logique de simulation ici. TOUT (listes, validation, aperçu des
## leviers, composition) passe par la façade C `Sim.world.*` (le moteur reste 100 % C,
## déterministe). Sur « Commencer », on grave la composition (set_player_culture) puis
## on régénère le monde — le pays du joueur naît avec SA culture.

signal started   ## le joueur a lancé son empire (le monde vient d'être régénéré)
signal cancelled ## le joueur a fermé sans composer (on garde le monde par défaut)
signal composed(slot: int, heritage: int, ethos: int, t0: int, t1: int, t2: int)  ## mode-slot : compo validée pour un empire

## Deux modes :
##  · AUTONOME (touche C en jeu) : « Commencer » applique la culture au JOUEUR + régénère.
##  · SLOT (écran Nouvelle partie) : « Valider » émet composed(slot,…) sans toucher le monde —
##    l'écran de setup collecte les compos de tous les empires puis lance la partie.
var _slot_mode := false
var _target_slot := 0

const AXES := ["Physique", "Social", "Intellectuel"]

## LORE EXPLICATIVE (« verbose explicative autorisée », retour joueur 2026-07-10) —
## chaque ligne est ANCRÉE sur un mécanisme moteur RÉEL (jamais une promesse) :
## esclavage par coutume = gate can_enslave (Dominateur/Honneur) · annexion-digestion =
## diplo étage 3 · creuset gardé/digéré = évènements xénophile/xénophobe · greffe
## culturelle = épargne S1 des investisseurs · drill à poudre = doctrine P2 (chaîne à feu).
## Indexée par id d'éthos, MÊME ORDRE que la façade (scps_api.c EPI[] : Horde·Clans·
## Ordre·Couronne·Ligue·Havre).
const ETHOS_LORE := [
	"La conquête est la seule légitimité que cet État reconnaisse. Son armée frappe fort et tôt, ses guerres visent ce dont l'empire a besoin, et les vaincus sont réduits en servitude par coutume. Les peuples soumis ne sont pas accueillis : ils sont DIGÉRÉS, lentement, jusqu'à ne faire qu'un seul sang. Les vassaux bien intégrés finissent annexés. Mauvais hôte, grand prédateur.",
	"La gloire au combat et la razzia fondent le rang de chacun. Même famille martiale que la Horde : servitude par coutume, vassaux digérés, cohésion par la fonte des peuples plutôt que par leur accueil. L'honneur rend les guerres fréquentes mais codifiées, et l'armée privilégie le duel et le raid.",
	"La hiérarchie et la discipline tiennent tout. C'est l'État le plus apte aux armes à FEU : le drill à poudre exige des rangs qui ne rompent pas, et sa doctrine militaire complète la chaîne arquebuse, poudrière, charbonnière. L'ordre intérieur amortit l'agitation, au prix d'une société rigide.",
	"Le bâtisseur d'institutions : chancelleries, cadastres, scriptoriums. Cet État TIENT la diversité au lieu de la fondre : plusieurs peuples peuvent prospérer sous sa couronne, et le creuset qui réussit y déclenche des ères fastes. Ses archétypes administratifs diffusent par le commerce vers ses voisins.",
	"Le profit décide, les carrefours enrichissent. L'État-Ligue prospère par les routes et les comptoirs, tient la diversité comme la Couronne, et surtout INVESTIT : c'est l'éthos qui épargne pour les greffes culturelles, la recherche des signatures d'autres peuples ouvertes par le négoce. Il refuse en revanche les pentes faustiennes coûteuses.",
	"Le consentement seul gouverne : cet État ne fracture jamais sa société de force. Havre des réfugiés et des minorités, il assimile par l'attrait plutôt que par le fer, et son creuset gardé prospère. Sa faiblesse est militaire : il n'aime ni lever l'ost ni le payer.",
]

## Même geste pour l'HÉRITAGE : la branche d'arbre que la lignée ouvre nativement
## (les rungs de l'ÉTOFFE + la signature, noms réels de scps_tech.c), même ordre
## que heritage_list (Ésotérique·Métallurgiste·Mécaniste·Adaptatif·Agraire·Clanique).
const HER_LORE := [
	"Sa branche d'arbre : les glyphes éthérés et la communion éthérée, jusqu'aux signatures arcanes profondes. Le peuple le plus sensible à la magie, et le plus exposé à ses pentes faustiennes.",
	"Sa branche d'arbre : les alliages des profondeurs et la gravure runique, jusqu'à la forge à runes. Le métal, les armes, la montagne.",
	"Sa branche d'arbre : les rouages de précision et le mécanisme d'horlogerie. L'ingénierie, les machines, l'efficacité d'emploi.",
	"Sa branche d'arbre : le droit coutumier et la langue franque. Le peuple-pont, qui intègre vite et tient les fédérations.",
	"Sa branche d'arbre : les vergers étagés et les pâturages intégrés. La terre nourricière, la croissance patiente.",
	"Sa branche d'arbre : les rites guerriers et les hordes conquérantes. Le clan, le moral au combat, la loyauté du sang.",
]

# palette (cohérente avec le chrome sombre du jeu)
const C_BG     := Color(0.04, 0.03, 0.02, 0.74)   # voile plein écran
const C_PANEL  := Color(0.10, 0.08, 0.055, 0.98)  # cuir sombre
const C_EDGE   := Color(0.79, 0.64, 0.29)         # liseré or vieilli
const C_TEXT   := Color(0.88, 0.86, 0.82)
const C_DIM    := Color(0.62, 0.60, 0.58)
const C_GOOD   := Color(0.46, 0.74, 0.42)
const C_BAD    := Color(0.82, 0.40, 0.34)
const C_TITLE  := Color(0.86, 0.70, 0.42)

# données de la façade
var _her: Array = []                  # héritages : [{id,nom,sphere,exemple}]
var _eth: Array = []                  # éthos     : [{id,nom,epithete,hint}]
var _axis_traits := [[], [], []]      # traditions par axe : [{id,nom,rang,hover}]

# widgets
var _tabs: TabContainer
var _her_opt: OptionButton
var _her_info: Label
var _eth_opt: OptionButton
var _eth_info: Label
var _trad_btns := [[], [], []]        # par axe : [{btn:Button, id:int}]
var _trad_sel := [-1, -1, -1]         # trait CHOISI par axe (id façade, -1 = aucun)
var _trad_hover := [null, null, null]
var _culture_lbl: Label
var _valid_lbl: Label
var _preview_lbl: Label
var _start_btn: Button
var _panel: PanelContainer
var _rng := RandomNumberGenerator.new()
var _arms_rect: TextureRect        ## aperçu d'armoiries (variante cyclable)
var _arms_var := 0                 ## variante 0..15 (hash → Heraldry.set_player_arms)
var _name_edit: LineEdit           ## nom d'empire personnalisé (vide = procédural)

## le hash de la variante d'armes courante (même famille que le hash de cid)
func _arms_hash() -> int:
	return int(fmod(float(_arms_var * 7919 + 13) * 2654435.7, 65536.0))

## l'aperçu d'armoiries : la COMPOSITION choisie (éthos → famille de meuble,
## héritage → accent) + la variante — teinte neutre (la couleur d'entité vient en jeu).
func _update_arms() -> void:
	if _arms_rect == null:
		return
	var Heraldry = load("res://ui/heraldry.gd")
	var img: Image = Heraldry.compose_arms_generic(_arms_hash(), 0.09, _cur_ethos(), _cur_heritage())
	_arms_rect.texture = ImageTexture.create_from_image(img) if img != null else null

## le nom personnalisé saisi (l'écran Nouvelle partie le lit en mode slot)
func custom_name() -> String:
	return _name_edit.text.strip_edges() if _name_edit != null else ""


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP   # capte tout : modale
	_rng.randomize()
	_build_ui()
	_load_data()
	_refresh()
	get_viewport().size_changed.connect(_adapt)
	_adapt()

## taille ADAPTATIVE : le panneau suit la fenêtre (46 % de large, borné) — natif plein écran.
func _adapt() -> void:
	if _panel == null:
		return
	var vp := get_viewport_rect().size
	var w := clampf(vp.x * 0.46, 680.0, 980.0)
	_panel.custom_minimum_size = Vector2(w, 0)
	if _preview_lbl != null:
		_preview_lbl.custom_minimum_size = Vector2(w - 40.0, 0)


# ── le voile plein écran (assombrit la carte derrière la modale) ───────────────
func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), C_BG, true)


# ── CONSTRUCTION de l'interface (en code, comme les autres panneaux) ───────────
func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(680, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL
	sb.border_color = C_EDGE
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(20)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)
	_panel = panel

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	panel.add_child(col)

	# titre
	var title := Label.new()
	title.text = "Créateur d'empire"
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", C_TITLE)
	col.add_child(title)

	var sub := Label.new()
	sub.text = "Composez votre peuple : héritage, éthos et trois traditions."
	sub.add_theme_color_override("font_color", C_DIM)
	col.add_child(sub)

	# ── ONGLETS (façon Stellaris) : Héritage / Éthos / Traditions ──
	_tabs = TabContainer.new()
	_tabs.custom_minimum_size = Vector2(0, 300)
	_tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(_tabs)

	# — onglet HÉRITAGE —
	var t_her := VBoxContainer.new()
	t_her.name = "Héritage"
	t_her.add_theme_constant_override("separation", 8)
	_tabs.add_child(t_her)
	t_her.add_child(_section("Votre lignée : elle nomme votre empire et ouvre sa branche de l'arbre."))
	_her_opt = OptionButton.new()
	_her_opt.item_selected.connect(func(_i): _refresh())
	t_her.add_child(_her_opt)
	_her_info = _hint_label()
	t_her.add_child(_her_info)

	# — onglet ÉTHOS —
	var t_eth := VBoxContainer.new()
	t_eth.name = "Éthos"
	t_eth.add_theme_constant_override("separation", 8)
	_tabs.add_child(t_eth)
	t_eth.add_child(_section("L'âme de l'État : épithète, armée, factions de la cour."))
	_eth_opt = OptionButton.new()
	_eth_opt.item_selected.connect(func(_i): _refresh())
	t_eth.add_child(_eth_opt)
	_eth_info = _hint_label()
	t_eth.add_child(_eth_info)

	# — onglet TRADITIONS : les CHOIX en RANGÉES par groupe (retour joueur : « regrouper
	#   par négatif / positif mineur / positif majeur », aucun +1/−1 affiché) —
	var t_trad := VBoxContainer.new()
	t_trad.name = "Traditions"
	t_trad.add_theme_constant_override("separation", 4)
	_tabs.add_child(t_trad)
	t_trad.add_child(_section("Une tradition par axe : un atout majeur, un atout mineur, un défaut."))
	for ax in range(3):
		var lab := Label.new()
		lab.text = AXES[ax]
		lab.add_theme_color_override("font_color", C_TITLE)
		t_trad.add_child(lab)
		var box := VBoxContainer.new()
		box.add_theme_constant_override("separation", 2)
		t_trad.add_child(box)
		_trad_btns[ax] = []          # peuplé par _load_data (les traits de la façade)
		_trad_btns[ax].append(box)   # [0] = le conteneur ; les boutons suivent
		var hv := _hint_label()
		t_trad.add_child(hv)
		_trad_hover[ax] = hv

	# ── IDENTITÉ : armoiries (variantes) + NOM d'empire (retour joueur 2026-07-10 :
	#    « éditeur de nom » + « éditeur d'héraldique non présent ») ──
	col.add_child(_sep())
	var idrow := HBoxContainer.new()
	idrow.add_theme_constant_override("separation", 8)
	col.add_child(idrow)
	_arms_rect = TextureRect.new()
	_arms_rect.custom_minimum_size = Vector2(44, 44)
	_arms_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_arms_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	idrow.add_child(_arms_rect)
	var aprev := Button.new()
	aprev.text = "◀"
	aprev.pressed.connect(func(): _arms_var = (_arms_var + 15) % 16; _update_arms())
	idrow.add_child(aprev)
	var anext := Button.new()
	anext.text = "▶"
	anext.pressed.connect(func(): _arms_var = (_arms_var + 1) % 16; _update_arms())
	idrow.add_child(anext)
	var nlab := Label.new()
	nlab.text = "  Nom de l'empire :"
	nlab.add_theme_color_override("font_color", C_TEXT)
	idrow.add_child(nlab)
	_name_edit = LineEdit.new()
	_name_edit.placeholder_text = "(laisser vide : nom procédural)"
	_name_edit.max_length = 30
	_name_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	idrow.add_child(_name_edit)

	# ── APERÇU live ──
	col.add_child(_sep())
	_culture_lbl = Label.new()
	_culture_lbl.add_theme_color_override("font_color", C_TITLE)
	_culture_lbl.add_theme_font_size_override("font_size", 16)
	col.add_child(_culture_lbl)

	_preview_lbl = Label.new()
	_preview_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_preview_lbl.custom_minimum_size = Vector2(600, 0)
	_preview_lbl.add_theme_color_override("font_color", C_DIM)
	col.add_child(_preview_lbl)

	_valid_lbl = Label.new()
	col.add_child(_valid_lbl)

	# ── pied : actions (la GRAINE vit dans l'écran Nouvelle partie, plus ici) ──
	col.add_child(_sep())
	var foot := HBoxContainer.new()
	foot.add_theme_constant_override("separation", 10)
	col.add_child(foot)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(spacer)

	var rnd_btn := Button.new()
	rnd_btn.text = "Aléatoire"
	rnd_btn.pressed.connect(_on_randomize)
	foot.add_child(rnd_btn)

	var cancel_btn := Button.new()
	cancel_btn.text = "Passer"
	cancel_btn.pressed.connect(_on_cancel)
	foot.add_child(cancel_btn)

	_start_btn = Button.new()
	_start_btn.text = "Commencer l'empire"
	_start_btn.pressed.connect(_on_start)
	foot.add_child(_start_btn)


func _sep() -> HSeparator:
	var s := HSeparator.new()
	return s

func _section(txt: String) -> Label:
	var l := Label.new()
	l.text = txt
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.add_theme_color_override("font_color", C_EDGE)
	return l

func _hint_label() -> Label:
	var l := Label.new()
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.custom_minimum_size = Vector2(600, 0)
	l.add_theme_color_override("font_color", C_DIM)
	l.add_theme_font_size_override("font_size", 12)
	return l

## la graine du MONDE : gérée ailleurs (Nouvelle partie / monde courant)
func _world_seed() -> int:
	return int(Sim.current_seed)


# ── DONNÉES : tout vient de la façade C ────────────────────────────────────────
func _load_data() -> void:
	if Sim.world == null:
		_valid_lbl.text = "Moteur (libscps) absent — bâtir : cd godot && scons."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
		_start_btn.disabled = true
		return
	if not Sim.world.has_method("heritage_list"):
		_valid_lbl.text = "libscps obsolète (créateur absent) — recompiler : cd godot && scons."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)
		_start_btn.disabled = true
		return

	_her = Sim.world.heritage_list()
	for h in _her:
		_her_opt.add_item(String(h["nom"]))
		_her_opt.set_item_metadata(_her_opt.item_count - 1, int(h["id"]))

	_eth = Sim.world.ethos_list()
	for e in _eth:
		_eth_opt.add_item(String(e["nom"]))
		_eth_opt.set_item_metadata(_eth_opt.item_count - 1, int(e["id"]))

	# REDONDANCES ÉCARTÉES du sélecteur (retour joueur 2026-07-10 : « rester logique
	# et enlever les redondances ») — preuve par la TABLE moteur (scps_heritage.c) :
	# Industrieux/Indolent = doublons EXACTS de Studieux/Inculte (±10 % Productivité) ;
	# Endurant/Fragile au climat = le même levier Démographie que Prolifique/Lent à
	# croître, en plus faible. Ils EXISTENT toujours côté moteur (compos IA, saves).
	const REDONDANTS := ["Endurant", "Fragile au climat", "Industrieux", "Indolent"]
	_axis_traits = [[], [], []]
	for t in Sim.world.tradition_list():
		var ax := int(t["axe"])
		if ax >= 0 and ax < 3 and not (String(t["nom"]) in REDONDANTS):
			_axis_traits[ax].append(t)
	# les BOUTONS de tradition : NOM SEUL, une RANGÉE par groupe avec son étiquette
	# alignée (« Positif majeur · Positif mineur · Négatif ») — AUCUN chiffre sur les
	# boutons ni les étiquettes ; l'effet PRÉCIS (chiffré) vit au survol et sous l'axe.
	for ax in range(3):
		var box: VBoxContainer = _trad_btns[ax][0]
		var entries := []
		entries.append(box)
		for grp in [[2, "Positif majeur"], [1, "Positif mineur"], [-1, "Négatif"]]:
			var row := HBoxContainer.new()
			row.add_theme_constant_override("separation", 6)
			box.add_child(row)
			var glab := Label.new()
			glab.text = String(grp[1])
			glab.custom_minimum_size = Vector2(110, 0)
			glab.add_theme_color_override("font_color", C_DIM)
			glab.add_theme_font_size_override("font_size", 12)
			row.add_child(glab)
			var flow := HFlowContainer.new()
			flow.add_theme_constant_override("h_separation", 6)
			flow.add_theme_constant_override("v_separation", 4)
			flow.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			row.add_child(flow)
			for t in _axis_traits[ax]:
				var rk := int(t["rang"])
				var nrk := 2 if rk >= 2 else (1 if rk == 1 else -1)
				if nrk != int(grp[0]):
					continue
				var id := int(t["id"])
				var b := Button.new()
				b.toggle_mode = true
				b.text = String(t["nom"])
				b.tooltip_text = String(t["hover"])
				var axc := ax
				var idc := id
				b.pressed.connect(func(): _on_trait_pick(axc, idc))
				flow.add_child(b)
				entries.append({"btn": b, "id": id})
		_trad_btns[ax] = entries

	# défaut sensé : une compo VALIDE d'entrée (majeur Phys / mineur Soc / défaut Int)
	_preset_default()


# (les libellés « +2/+1/−1 » par bouton sont retirés — le rang se lit au GROUPE
#  majeur/mineur/défaut ; la règle en tête garde les chiffres.)


## clic sur un trait : sélection EXCLUSIVE dans son axe
func _on_trait_pick(ax: int, id: int) -> void:
	_trad_sel[ax] = id
	_sync_trait_buttons()
	_refresh()

## reflète _trad_sel sur l'état pressé des boutons (sans re-signal)
func _sync_trait_buttons() -> void:
	for ax in range(3):
		for i in range(1, _trad_btns[ax].size()):
			var e: Dictionary = _trad_btns[ax][i]
			(e["btn"] as Button).set_pressed_no_signal(int(e["id"]) == _trad_sel[ax])


# choisit, par axe, le 1er trait du rang voulu (rôle[ax] : 0 majeur · 1 mineur · 2 défaut)
func _select_roles(role: Array) -> void:
	for ax in range(3):
		var want_rank := 2 if role[ax] == 0 else (1 if role[ax] == 1 else -1)
		for t in _axis_traits[ax]:
			var rk := int(t["rang"])
			if rk == want_rank or (want_rank == 2 and rk >= 2):
				_trad_sel[ax] = int(t["id"])
				break
	_sync_trait_buttons()

func _preset_default() -> void:
	_her_opt.select(0)
	_eth_opt.select(0)
	_select_roles([0, 1, 2])   # Phys majeur · Soc mineur · Int défaut


func _trait_rank(id: int) -> int:
	for ax in range(3):
		for t in _axis_traits[ax]:
			if int(t["id"]) == id:
				return int(t["rang"])
	return 0

func _trait_hover(id: int) -> String:
	for ax in range(3):
		for t in _axis_traits[ax]:
			if int(t["id"]) == id:
				return String(t["hover"])
	return ""

## GRISE les traits IMPOSSIBLES : la compo exige EXACTEMENT {majeur, mineur, défaut}
## sur les 3 axes — vu les DEUX autres choix courants, un rang déjà « épuisé » se grise.
func _regray() -> void:
	for ax in range(3):
		var others := []
		for bx in range(3):
			if bx != ax:
				others.append(_norm_rank(_trait_rank(_trad_sel[bx])))
		for i in range(1, _trad_btns[ax].size()):
			var e: Dictionary = _trad_btns[ax][i]
			var r := _norm_rank(_trait_rank(int(e["id"])))
			# permis ssi {r} ∪ others peut ENCORE devenir {2,1,-1} : r ne doit pas
			# être déjà porté par LES DEUX autres axes (un doublon reste réparable).
			var dup := 0
			for o in others:
				if o == r:
					dup += 1
			(e["btn"] as Button).disabled = dup >= 2

func _norm_rank(r: int) -> int:
	return 2 if r >= 2 else (1 if r == 1 else -1)


func _cur_heritage() -> int:
	return int(_her_opt.get_item_metadata(_her_opt.selected)) if _her_opt.selected >= 0 else 0

func _cur_ethos() -> int:
	return int(_eth_opt.get_item_metadata(_eth_opt.selected)) if _eth_opt.selected >= 0 else 0

func _cur_trait(ax: int) -> int:
	return _trad_sel[ax]


# ── RAFRAÎCHIT l'aperçu (nom de culture, hovers, leviers, validité) ────────────
func _refresh() -> void:
	# garde : pas de monde, OU libscps obsolète/incomplet (listes non peuplées par
	# _load_data) → ne PAS appeler les méthodes du créateur (elles n'existent pas sur
	# un vieux binding). Couvre l'appel de _ready() et de _on_randomize().
	if Sim.world == null or _her_opt == null or _her_opt.item_count == 0:
		return
	var her := _cur_heritage()
	var eth := _cur_ethos()
	var seed := _world_seed()

	# héritage : sphère + ethnonyme-exemple + CE QUE ÇA OUVRE (l'accès d'arbre natif)
	# + la LORE de branche (« verbose explicative autorisée »)
	for h in _her:
		if int(h["id"]) == her:
			var hlore := String(HER_LORE[her]) if her >= 0 and her < HER_LORE.size() else ""
			_her_info.text = "Sphère %s · vos lieux et gens porteront des noms comme « %s ».\n\nAccès natif : la branche « %s » de l'arbre des techs vous est ouverte d'entrée (ses signatures, tier 3). Les autres branches s'ouvrent par le commerce ou par l'intégration de peuples (la métabolisation : déverrouille leurs signatures, accélère votre recherche, escompte les techs répandues).\n\n%s" % [
				String(h["sphere"]), Sim.world.culture_name(her, seed), String(h["nom"]), hlore]
			break
	# éthos : « ça m'apporte quoi » — épithète, hint moteur, LORE mécanique détaillée
	for e in _eth:
		if int(e["id"]) == eth:
			var elore := String(ETHOS_LORE[eth]) if eth >= 0 and eth < ETHOS_LORE.size() else ""
			_eth_info.text = "Votre État sera une « %s … ». %s\n\n%s\n\nDans tous les cas, l'éthos compose votre ARMÉE (le type de troupes que votre doctrine favorise), vos FACTIONS de cour (qui vous soutient, qui conspire), l'épithète de votre État et les évènements qui vous ressemblent." % [
				String(e["epithete"]), String(e["hint"]), elore]
			break
	# traditions : l'effet PRÉCIS du trait choisi sous chaque axe (le hover moteur
	# porte les chiffres réels ; le rang se lit au groupe, pas en préfixe)
	for ax in range(3):
		_trad_hover[ax].text = _trait_hover(_cur_trait(ax))
	_regray()

	# nom de culture (le PEUPLE)
	_culture_lbl.text = "Votre peuple : les %s" % Sim.world.culture_name(her, seed)

	# aperçu des leviers — des MOTS + une flèche ; les CHIFFRES au survol (tooltip)
	var t0 := _cur_trait(0)
	var t1 := _cur_trait(1)
	var t2 := _cur_trait(2)
	# les CHIFFRES en clair (retour joueur : « je veux des chiffres, pas du verbeux
	# pour cet écran ») — plus de flèches-seules avec les nombres cachés au survol.
	var tips := PackedStringArray()
	for lv in Sim.world.culture_preview(t0, t1, t2):
		var val := float(lv.get("value", 0.0))
		var num := ""
		if int(lv.get("is_pct", 0)) != 0:
			num = "%+d %%" % int(round(val * 100.0))
		else:
			num = "%+.2f" % val
		tips.append("%s %s" % [String(lv["nom"]), num])
	_preview_lbl.text = ("Effets : " + " · ".join(tips)) if tips.size() > 0 else "Effets : aucun"
	_preview_lbl.tooltip_text = ""
	_update_arms()

	# validité (la façade fait foi) + message d'aide
	var ok: bool = Sim.world.culture_validate(t0, t1, t2)
	_start_btn.disabled = not ok
	if ok:
		_valid_lbl.text = "✓ Composition valide."
		_valid_lbl.add_theme_color_override("font_color", C_GOOD)
	else:
		_valid_lbl.text = "✗ Il faut exactement un atout majeur, un atout mineur et un défaut."
		_valid_lbl.add_theme_color_override("font_color", C_BAD)


# ── ACTIONS ────────────────────────────────────────────────────────────────────
func _on_randomize() -> void:
	if Sim.world == null:
		return
	_her_opt.select(_rng.randi_range(0, max(0, _her_opt.item_count - 1)))
	_eth_opt.select(_rng.randi_range(0, max(0, _eth_opt.item_count - 1)))
	_arms_var = _rng.randi_range(0, 15)   # les armes suivent le tirage
	# permutation aléatoire des rôles {majeur, mineur, défaut} sur les 3 axes
	var roles := [0, 1, 2]
	for i in range(roles.size() - 1, 0, -1):
		var j := _rng.randi_range(0, i)
		var tmp = roles[i]; roles[i] = roles[j]; roles[j] = tmp
	# dans chaque axe, un trait ALÉATOIRE du rang imposé
	for ax in range(3):
		var want_rank := 2 if roles[ax] == 0 else (1 if roles[ax] == 1 else -1)
		var cands := []
		for t in _axis_traits[ax]:
			var rk := int(t["rang"])
			if rk == want_rank or (want_rank == 2 and rk >= 2):
				cands.append(int(t["id"]))
		if cands.size() > 0:
			_trad_sel[ax] = cands[_rng.randi_range(0, cands.size() - 1)]
	_sync_trait_buttons()
	_refresh()


func _on_start() -> void:
	if Sim.world == null:
		return
	var her := _cur_heritage()
	var eth := _cur_ethos()
	var t0 := _cur_trait(0)
	var t1 := _cur_trait(1)
	var t2 := _cur_trait(2)
	if not Sim.world.culture_validate(t0, t1, t2):
		_refresh()   # invalide : le bouton n'aurait pas dû être actif
		return
	# l'IDENTITÉ choisie (armes + nom) — les armes sont display-only (Heraldry),
	# le nom s'applique au pays du joueur APRÈS la genèse (elle nomme d'office).
	var Heraldry = load("res://ui/heraldry.gd")
	if _target_slot == 0 or not _slot_mode:
		Heraldry.set_player_arms(_arms_hash())
	if _slot_mode:
		# écran de setup : on rend la compo, sans toucher le monde (lancé à « Lancer »).
		composed.emit(_target_slot, her, eth, t0, t1, t2)
		hide()
		return
	# mode autonome : applique au joueur + régénère immédiatement (graine COURANTE)
	if not Sim.world.set_player_culture(her, eth, t0, t1, t2):
		_refresh()
		return
	Sim.regenerate(_world_seed())   # le monde renaît AVEC la culture du joueur
	var nm := custom_name()
	if nm != "" and Sim.world.has_method("set_country_name"):
		Sim.world.set_country_name(int(Sim.world.player()), nm)
	hide()
	started.emit()

## ouvre le créateur en mode SLOT (composer la culture de l'empire `slot`).
func open_for_slot(slot: int) -> void:
	_slot_mode = true
	_target_slot = slot
	if _start_btn != null:
		_start_btn.text = "Valider la culture"
	open()


func _on_cancel() -> void:
	if Sim.world != null:
		Sim.world.clear_player_culture()   # pas de composition : retour au défaut (IA aléatoire)
	hide()
	cancelled.emit()


## ouvre la fenêtre (réinitialise une composition valide par défaut)
func open() -> void:
	show()
	if Sim.world != null and _her_opt.item_count > 0:
		_refresh()
	queue_redraw()
