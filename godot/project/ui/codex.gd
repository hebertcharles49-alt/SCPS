extends Control
## Codex — LE CODEX DES VERBES (menu Échap · ex-F1, parti aux onglets du rail) :
## l'enseignement de tout ce que le joueur
## PEUT FAIRE. Motif chronique.gd (Control statique scrollable, visible/hide) : zéro
## logique de sim ici, on affiche une liste EN DUR (rédigée depuis un scan du moteur —
## enum CMD_* de scps_sim.h + scps_player_* de scps_api.h + leur câblage UI réel au
## 2026-07-06). RÈGLE D'OR : ce fichier ne lit ni ne mute le moteur — un texte, un
## panneau, une touche. Si un verbe change de coût/emplacement, ce texte doit suivre
## (mais rien ici n'appelle la façade).

## Chaque entrée : {nom, ou, regle, bientot (optionnel, true si le verbe n'a pas encore
## d'UI Godot — câblé côté façade C mais aucun bouton/panneau ne l'expose aujourd'hui)}
const VKit = preload("res://ui/vkit.gd")
const DOMAINS := [
	["Empire & Économie", [
		{"nom": "Bâtir un édifice", "ou": "Panneau Construction (bouton depuis la province ou l'onglet Constructions) · touche via le panneau province", "regle": "coûts en matières + or, réels (roster scps_building_roster) ; grisé si illégal (tech/région/or)"},
		{"nom": "Bâtir une manufacture (Panneau B)", "ou": "Détail province · onglet Main-d'œuvre, section « Bâtir une manufacture »", "regle": "gate miroir de l'IA civile : staffage/tier/intrant/slot libre/or ; un seul chantier du même type à la fois"},
		{"nom": "Recruter une unité", "ou": "Panneau Construction (bouton depuis la province)", "regle": "roster 22 unités, gate technologique (7/22 gatées, 15 libres day-1) + or + armes en stock"},
		{"nom": "Régler la levée (postes vacants)", "ou": "Tiroir Armée (sidebar)", "regle": "curseur de niveau de levée — influence le rythme de recrutement IA-like du joueur"},
		{"nom": "Rechercher une technologie", "ou": "Arbre de tech Medusa (bouton Tech, topbar)", "regle": "coût ∝ √N-provinces × biais d'éthos ÷ remise de diffusion (le prix affiché = le prix payé)"},
		{"nom": "Allouer la main-d'œuvre (puits par région)", "ou": "Détail province · onglet Main-d'œuvre", "regle": "override du split AUTO : poids par ressource/bâtiment, fermeture, choix d'intrant, retour Auto"},
		{"nom": "Réincorporer de la population (transfert entre régions)", "ou": "Détail province · onglet Peuples, section Réincorporation", "regle": "classe choisie, coercition à la source pour les libres ; les esclaves sont exemptés (propriété déplacée, pas réprimée)"},
		{"nom": "Acheter / vendre sur le marché (Centres)", "ou": "Tiroir Marché (sidebar), depuis la capitale", "regle": "prix national réel, marge de distance au Centre le plus proche"},
		{"nom": "Ouvrir / router une route commerciale", "ou": "Panneau province (chips « Route terre » / « Route mer »)", "regle": "gate : deux ports + deux marchés + (pacte ou même empire) ; le rendement décroît avec la distance, jamais nul"},
		{"nom": "Recompléter l'armée / Dissoudre l'armée", "ou": "Tiroir Armée (sidebar)", "regle": "recompléter paie or+matière ; dissoudre libère la pop MAIS ne rend PAS les armes (asymétrie connue, cf. rapport)"},
		{"nom": "Mettre une coque en chantier (marine)", "ou": "Tiroir Armée (sidebar), section Flotte", "regle": "coût en cuivre (depuis le retrait du métal) ; nécessite un port"},
		{"nom": "Lancer une campagne (attaquer une région)", "ou": "Panneau province (chip « Attaquer ici », depuis une région ennemie voisine)", "regle": "revalidé au drain : région à soi comme origine, chemin BFS existant — silencieux si échoue"},
	]],
	["Peuples", [
		{"nom": "Réprimer une région (agitation)", "ou": "Panneau province (chip « Réprimer »)", "regle": "fait baisser l'agitation au prix de la légitimité/coercition"},
		{"nom": "Assimiler (creuset culturel)", "ou": "Panneau province (chip « Assimiler »)", "regle": "pousse la région vers la culture dominante de l'empire"},
		{"nom": "Purger une minorité", "ou": "Panneau province (chip « Purger »)", "regle": "réduit une minorité ciblée ; coût en légitimité, jamais une strate esclave (exemptée)"},
		{"nom": "Coloniser une province vierge", "ou": "Panneau province (bouton « Coloniser », sur une province colonisable)", "regle": "durée 360-1080 j selon distance à la frontière ; 1 ordre/an (cadence liée à la personnalité IA-like)"},
		{"nom": "Proposer un pacte migratoire", "ou": "Fenêtre diplomatique par pays (bouton « Migration »)", "regle": "échange passif de population avec un allié d'un autre héritage ; le vis-à-vis évalue via l'opinion"},
		{"nom": "Affranchir les esclaves du pays", "ou": "Tiroir Conseil (sidebar), section « Peuple servile » — aperçu (âmes/friction) puis confirmation en 2 clics", "regle": "granularité PAYS (une politique, pas une province) ; bascule klass→Laboureur ; irréversible"},
		{"nom": "Acheter / vendre au marché servile (Centres)", "ou": "Tiroir Conseil (sidebar), section « Peuple servile » — quantités 50/200", "regle": "achat gaté éthos/tech (miroir de la capture de guerre, grisé avec le mot) ; vente libre (on vend ce qu'on tient déjà)"},
		{"nom": "Choisir dans un évènement (décision membrane)", "ou": "Boîte de dialogue d'évènement (pause automatique, distincte du popup Oyez Oyez)", "regle": "rien n'est appliqué tant que le choix n'est pas fait ; options bornées par l'évènement"},
	]],
	["Diplomatie & Guerre", [
		{"nom": "Déclarer la guerre", "ou": "Fenêtre diplomatique par pays (bouton « Guerre »)", "regle": "exige un casus belli utilisable : gratuit (défense/anti-piraterie/subjugation) ou une revendication payante MÛRIE"},
		{"nom": "Fabriquer une revendication (CB payant)", "ou": "Fenêtre diplomatique par pays (bouton « Fabriquer une revendication »)", "regle": "coûte 2 ans de revenu de la cible ; mûrit en 1 an ; reste valable 5 ans une fois prête"},
		{"nom": "Proposer la paix (blanche)", "ou": "Fenêtre diplomatique par pays (bouton « Paix »)", "regle": "le vis-à-vis évalue l'offre (opinion + score de guerre) — peut refuser"},
		{"nom": "Proposer une alliance", "ou": "Fenêtre diplomatique par pays (bouton « Allier »)", "regle": "consentement bilatéral requis (ai_consider_offer, opinion ±100)"},
		{"nom": "Proposer un pacte (non-agression)", "ou": "Fenêtre diplomatique par pays (bouton « Pacte »)", "regle": "consentement requis, coût d'opinion plus faible que l'alliance"},
		{"nom": "Imposer / lever un embargo", "ou": "Fenêtre diplomatique par pays (bouton « Embargo »)", "regle": "unilatéral (pas de consentement) ; pèse sur l'opinion à la baisse"},
		{"nom": "Engager conseillers du Conseil (recrutement de siège)", "ou": "Tiroir Conseil (sidebar), un candidat par siège", "regle": "le choix éclairé — chaque candidat a ses propres multiplicateurs"},
		{"nom": "Démettre un conseiller", "ou": "Tiroir Conseil (sidebar)", "regle": "libère le siège, perd les multiplicateurs du titulaire"},
		{"nom": "Régler la paie d'un siège du Conseil", "ou": "Tiroir Conseil (sidebar), curseur 0.5×/1×/1.5×/2× sous chaque siège", "regle": "un siège mal payé perd en efficacité/loyauté"},
		{"nom": "Activer / désactiver un décret (civics)", "ou": "Tiroir Conseil ou Économie (section Décrets)", "regle": "une réforme active refuse le retrait ; condition d'entrée vérifiée à l'activation"},
		{"nom": "Engager l'âge courant", "ou": "Chip topbar « Engager : <âge> » → écran de récap d'âge", "regle": "l'IA s'engage automatiquement ; le joueur choisit son moment — une fois par âge"},
	]],
	["Foi & Savoir", [
		{"nom": "Fonder / rallier une religion", "ou": "Créateur de foi (s'ouvre seul au 1er édifice religieux bâti)", "regle": "sous le plafond mondial (⌈empires/3⌉ racines) : au-delà, on RALLIE une foi existante au lieu d'en fonder une"},
		{"nom": "Provoquer un schisme", "ou": "Panneau Religion (si rouvert)", "regle": "plafonné à 2 sectes par racine ; grisé au plafond"},
		{"nom": "Composer sa culture (héritage/éthos/traditions)", "ou": "Créateur de culture, écran Nouvelle Partie SEULEMENT (avant la genèse)", "regle": "1 héritage + 1 éthos + 3 traditions (une par axe) ; fige le pays du joueur à la régénération"},
	]],
	["Fin de partie", [
		{"nom": "Consulter l'état de l'Entropie / la Merveille", "ou": "Bandeau d'endgame (haut-centre, toujours visible) + arbre de tech (jauge de métabolisation « Compte pour l'Ascension »)", "regle": "lecture seule — la victoire Merveille est un compte séparé de l'accès aux signatures tech"},
		{"nom": "Consulter le combat en cours (siège/bataille)", "ou": "Panneau de combat (clic sur une région en guerre/un jeton d'armée)", "regle": "lecture seule — force, pertes, score de guerre des deux camps"},
	]],
]

var _list: VBoxContainer
var _panel: PanelContainer
var _scroll: ScrollContainer
var _search_edit: LineEdit
var _sommaire_row: HFlowContainer

## catégories repliables + recherche (retour joueur 2026-07-10, Lot 4.4) : la liste
## était plate, alphabétique, sans repère. Les DOMAINES thématiques EXISTAIENT déjà
## (Empire & Économie / Peuples / Diplomatie & Guerre / Foi & Savoir / Fin de partie) —
## on les rend REPLIABLES et on ajoute « Concepts & jauges » comme catégorie de plus,
## plus un sommaire cliquable (saute à la section) et un champ de recherche qui filtre
## les entrées en direct. Zéro logique de sim : pur réarrangement d'affichage, rien
## n'est lu/écrit côté façade dans ce fichier.
var _collapsed: Dictionary = {}   # nom de catégorie -> bool (repliée)
var _sections: Dictionary = {}    # nom -> {header:Button, sep:HSeparator, body:VBoxContainer, entries:[{ctrl,text}]}

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	_panel = PanelContainer.new()
	_panel.custom_minimum_size = Vector2(700, 690)
	var sb := StyleBoxFlat.new()
	sb.bg_color = VKit.COL_PANEL
	sb.border_color = VKit.COL_EDGE
	sb.set_border_width_all(1)
	sb.set_border_width(SIDE_TOP, 3)
	sb.set_corner_radius_all(1)
	sb.set_content_margin_all(14)
	_panel.add_theme_stylebox_override("panel", sb)
	center.add_child(_panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 6)
	_panel.add_child(col)

	var title := Label.new()
	title.text = "Le Codex des Verbes"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	col.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Tout ce que vous pouvez faire, et où le faire. (Échap pour fermer · menu Échap pour rouvrir)"
	subtitle.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
	col.add_child(subtitle)

	# ── en-tête FIXE (ne défile pas) : recherche + sommaire des catégories ──
	var search_row := HBoxContainer.new()
	search_row.add_theme_constant_override("separation", 6)
	col.add_child(search_row)
	var search_ic := Label.new()
	search_ic.text = "🔎"
	search_row.add_child(search_ic)
	_search_edit = LineEdit.new()
	_search_edit.placeholder_text = "Rechercher un verbe ou un concept…"
	_search_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_search_edit.text_changed.connect(_on_search_changed)
	search_row.add_child(_search_edit)
	var clear_btn := Button.new()
	clear_btn.text = "✕"
	clear_btn.tooltip_text = "Effacer la recherche"
	clear_btn.focus_mode = Control.FOCUS_NONE
	clear_btn.pressed.connect(func():
		_search_edit.text = ""
		_apply_filter(""))
	search_row.add_child(clear_btn)

	_sommaire_row = HFlowContainer.new()
	_sommaire_row.add_theme_constant_override("h_separation", 4)
	_sommaire_row.add_theme_constant_override("v_separation", 4)
	col.add_child(_sommaire_row)
	col.add_child(HSeparator.new())

	_scroll = ScrollContainer.new()
	_scroll.custom_minimum_size = Vector2(670, 500)
	_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(_scroll)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 4)
	_scroll.add_child(_list)

	var foot := HBoxContainer.new()
	col.add_child(foot)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(func(): hide(); Sound.play("ui_parchment_close"))
	foot.add_child(close)

	visibility_changed.connect(func(): if visible: _rebuild())
	hide()

func toggle() -> void:
	visible = not visible
	Sound.play("ui_parchment_open" if visible else "ui_parchment_close")

func _rebuild() -> void:
	if _list == null:
		return
	for c in _list.get_children():
		c.queue_free()
	_sections = {}

	# domaine « CONCEPTS & JAUGES » : généré du registre ui/concepts.gd — la MÊME
	# source que les mots turquoise des tooltips (retour joueur 2026-07-10 : le
	# codex est branché sur les concepts). Traité comme une catégorie de plus
	# (repliable, cherchable) au même titre que les domaines de verbes.
	var Concepts = load("res://ui/concepts.gd")
	var concept_entries := []
	var ckeys: Array = Concepts.DEFS.keys()
	ckeys.sort()
	for ck in ckeys:
		# chaque concept avec SON ICÔNE (retour joueur 2026-07-10) — RichText [img]
		var cl := RichTextLabel.new()
		cl.bbcode_enabled = true
		cl.fit_content = true
		cl.autowrap_mode = TextServer.AUTOWRAP_WORD
		cl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var ic: String = Concepts.icon_of(String(ck))
		var pre := ("[img=16x16]%s[/img] " % ic) if ic != "" and ResourceLoader.exists(ic) else ""
		var defx: String = Concepts.def_of(String(ck))
		cl.text = "%s[color=#%s]%s[/color] [color=#8f877a]: %s[/color]" % [
			pre, Concepts.COL, String(ck), defx]
		cl.add_theme_font_size_override("normal_font_size", 13)
		concept_entries.append({"ctrl": cl, "text": (String(ck) + " " + defx).to_lower()})
	_add_section("Concepts & jauges", concept_entries)

	for domain in DOMAINS:
		var dname: String = String(domain[0])
		var entries: Array = domain[1]
		var dom_entries := []
		for e in entries:
			var entry: Dictionary = e
			var row := VBoxContainer.new()
			row.add_theme_constant_override("separation", 1)

			var nom_lbl := Label.new()
			var bientot: bool = bool(entry.get("bientot", false))
			var nom: String = String(entry.get("nom", "?"))
			nom_lbl.text = ("• %s" % nom) if not bientot else ("• %s (bientôt)" % nom)
			nom_lbl.add_theme_color_override("font_color",
				Color(0.93, 0.89, 0.80) if not bientot else Color(0.72, 0.58, 0.40))
			nom_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
			row.add_child(nom_lbl)

			var ou_txt: String = String(entry.get("ou", "—"))
			var ou_lbl := Label.new()
			ou_lbl.text = "  Où : " + ou_txt
			ou_lbl.add_theme_color_override("font_color", Color(0.55, 0.62, 0.72))
			ou_lbl.add_theme_font_size_override("font_size", 13)
			ou_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
			row.add_child(ou_lbl)

			var regle_txt: String = String(entry.get("regle", "—"))
			var regle_lbl := Label.new()
			regle_lbl.text = "  Règle : " + regle_txt
			regle_lbl.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
			regle_lbl.add_theme_font_size_override("font_size", 13)
			regle_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
			row.add_child(regle_lbl)

			dom_entries.append({"ctrl": row, "text": (nom + " " + ou_txt + " " + regle_txt).to_lower()})
		_add_section(dname, dom_entries)

	_rebuild_sommaire()
	_apply_filter(_search_edit.text if _search_edit != null else "")

## titre d'en-tête de section : flèche de repli + nom + compte d'entrées visibles.
## (paramètre nommé `cat`, pas `name` — Control hérite déjà d'un `.name` de Node,
## autant éviter l'ombrage plutôt que compter sur la tolérance du compilateur)
func _section_title(cat: String, collapsed: bool, n: int) -> String:
	return "%s %s (%d)" % (["▸", cat, n] if collapsed else ["▾", cat, n])

## ajoute une section repliable (en-tête cliquable + séparateur + corps) à _list.
func _add_section(cat: String, entries: Array) -> void:
	var collapsed: bool = bool(_collapsed.get(cat, false))
	var header := Button.new()
	header.flat = true
	header.alignment = HORIZONTAL_ALIGNMENT_LEFT
	header.focus_mode = Control.FOCUS_NONE
	header.add_theme_font_size_override("font_size", 17)
	header.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	header.text = _section_title(cat, collapsed, entries.size())
	header.tooltip_text = "Replier / déplier « %s »" % cat
	header.pressed.connect(func(): _toggle_section(cat))
	var sep := HSeparator.new()
	var body := VBoxContainer.new()
	body.add_theme_constant_override("separation", 10)
	body.visible = not collapsed
	for en in entries:
		body.add_child(en["ctrl"])
	_list.add_child(header)
	_list.add_child(sep)
	_list.add_child(body)
	_sections[cat] = {"header": header, "sep": sep, "body": body, "entries": entries}

func _toggle_section(cat: String) -> void:
	if not _sections.has(cat):
		return
	var collapsed: bool = not bool(_collapsed.get(cat, false))
	_collapsed[cat] = collapsed
	var sec: Dictionary = _sections[cat]
	var entries: Array = sec["entries"]
	(sec["header"] as Button).text = _section_title(cat, collapsed, entries.size())
	(sec["body"] as Control).visible = not collapsed

## le sommaire : une puce par catégorie, saute à la section (déplie + scroll).
func _rebuild_sommaire() -> void:
	if _sommaire_row == null:
		return
	for c in _sommaire_row.get_children():
		c.queue_free()
	var names := ["Concepts & jauges"]
	for domain in DOMAINS:
		names.append(String(domain[0]))
	for nm in names:
		var chip := Button.new()
		chip.text = nm
		chip.flat = true
		chip.focus_mode = Control.FOCUS_NONE
		chip.add_theme_font_size_override("font_size", 12)
		chip.add_theme_color_override("font_color", Color(0.55, 0.62, 0.72))
		chip.tooltip_text = "Aller à « %s »" % nm
		var nmc: String = nm
		chip.pressed.connect(func(): _goto_section(nmc))
		_sommaire_row.add_child(chip)

func _goto_section(cat: String) -> void:
	if not _sections.has(cat):
		return
	if _search_edit != null and _search_edit.text != "":
		_search_edit.text = ""
		_apply_filter("")
	_collapsed[cat] = false
	var sec: Dictionary = _sections[cat]
	(sec["header"] as Button).text = _section_title(cat, false, (sec["entries"] as Array).size())
	(sec["body"] as Control).visible = true
	await get_tree().process_frame
	if _scroll != null:
		_scroll.ensure_control_visible(sec["header"])

func _on_search_changed(_new_text: String) -> void:
	_apply_filter(_search_edit.text)

## filtre en direct : sous-chaîne (nom+définition/règle), insensible à la casse.
## recherche vide ⇒ chaque section respecte son état de repli ; recherche active ⇒
## sections sans résultat se masquent, les autres se déplient le temps de la requête.
func _apply_filter(query: String) -> void:
	if _sections.is_empty():
		return
	var q := query.strip_edges().to_lower()
	for cat in _sections.keys():
		var sec: Dictionary = _sections[cat]
		var body: Control = sec["body"]
		var header: Button = sec["header"]
		var sep: Control = sec["sep"]
		var entries: Array = sec["entries"]
		if q == "":
			for en in entries:
				(en["ctrl"] as Control).visible = true
			var collapsed: bool = bool(_collapsed.get(cat, false))
			header.text = _section_title(cat, collapsed, entries.size())
			header.visible = true
			sep.visible = true
			body.visible = not collapsed
			continue
		var n_match := 0
		for en in entries:
			var m: bool = (en["text"] as String).find(q) >= 0
			(en["ctrl"] as Control).visible = m
			if m:
				n_match += 1
		var any_match: bool = n_match > 0
		header.visible = any_match
		sep.visible = any_match
		body.visible = any_match
		if any_match:
			header.text = _section_title(cat, false, n_match)
