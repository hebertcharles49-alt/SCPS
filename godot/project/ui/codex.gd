extends Control
## Codex — LE CODEX DES VERBES (touche F1) : l'enseignement de tout ce que le joueur
## PEUT FAIRE. Motif chronique.gd (Control statique scrollable, visible/hide) : zéro
## logique de sim ici, on affiche une liste EN DUR (rédigée depuis un scan du moteur —
## enum CMD_* de scps_sim.h + scps_player_* de scps_api.h + leur câblage UI réel au
## 2026-07-06). RÈGLE D'OR : ce fichier ne lit ni ne mute le moteur — un texte, un
## panneau, une touche. Si un verbe change de coût/emplacement, ce texte doit suivre
## (mais rien ici n'appelle la façade).

## Chaque entrée : {nom, ou, regle, bientot (optionnel, true si le verbe n'a pas encore
## d'UI Godot — câblé côté façade C mais aucun bouton/panneau ne l'expose aujourd'hui)}
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
		{"nom": "Changer de posture militaire", "ou": "Tiroir Armée (sidebar)", "regle": "3 chips (défensive/neutre/offensive) — pèse sur l'agressivité/les gates IA miroir"},
		{"nom": "Lancer une campagne (attaquer une région)", "ou": "Panneau province (chip « Attaquer ici », depuis une région ennemie voisine)", "regle": "revalidé au drain : région à soi comme origine, chemin BFS existant — silencieux si échoue"},
	]],
	["Peuples", [
		{"nom": "Réprimer une région (agitation)", "ou": "Panneau province (chip « Réprimer »)", "regle": "fait baisser l'agitation au prix de la légitimité/coercition"},
		{"nom": "Assimiler (creuset culturel)", "ou": "Panneau province (chip « Assimiler »)", "regle": "pousse la région vers la culture dominante de l'empire"},
		{"nom": "Purger une minorité", "ou": "Panneau province (chip « Purger »)", "regle": "réduit une minorité ciblée ; coût en légitimité, jamais une strate esclave (exemptée)"},
		{"nom": "Coloniser une province vierge", "ou": "Panneau province (bouton « Coloniser », sur une province colonisable)", "regle": "durée 360-1080 j selon distance à la frontière ; 1 ordre/an (cadence liée à la personnalité IA-like)"},
		{"nom": "Proposer un pacte migratoire", "ou": "Fenêtre diplomatique par pays (bouton « Migration »)", "regle": "échange passif de population avec un allié d'un autre héritage ; le vis-à-vis évalue via l'opinion"},
		{"nom": "Affranchir les esclaves du pays", "ou": "**(bientôt)** aucun bouton Godot — verbe façade seul (scps_player_manumit)", "regle": "granularité PAYS (une politique, pas une province) ; bascule klass→Laboureur", "bientot": true},
		{"nom": "Acheter / vendre au marché servile (Centres)", "ou": "**(bientôt)** aucun bouton Godot — verbe façade seul (scps_player_slave_buy/_sell)", "regle": "achat gaté éthos/tech (miroir de la capture de guerre) ; vente libre (on vend ce qu'on tient déjà)", "bientot": true},
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
		{"nom": "Régler la paie d'un siège du Conseil", "ou": "**(bientôt)** aucun curseur Godot — verbe façade seul (scps_player_council_pay)", "regle": "curseur 0×-2× ; un siège mal payé perd en efficacité/loyauté", "bientot": true},
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

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	_panel = PanelContainer.new()
	_panel.custom_minimum_size = Vector2(680, 680)
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.10, 0.08, 0.06, 0.97)
	sb.border_color = Color(0.62, 0.52, 0.30)
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(6)
	sb.set_content_margin_all(14)
	_panel.add_theme_stylebox_override("panel", sb)
	center.add_child(_panel)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	_panel.add_child(col)

	var title := Label.new()
	title.text = "Le Codex des Verbes"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
	col.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Tout ce que vous pouvez faire — et où le faire. (F1 pour fermer)"
	subtitle.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
	col.add_child(subtitle)
	col.add_child(HSeparator.new())

	var sc := ScrollContainer.new()
	sc.custom_minimum_size = Vector2(650, 560)
	sc.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(sc)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 10)
	sc.add_child(_list)

	var foot := HBoxContainer.new()
	col.add_child(foot)
	var sp := Control.new(); sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	foot.add_child(sp)
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(func(): hide())
	foot.add_child(close)

	visibility_changed.connect(func(): if visible: _rebuild())
	hide()

func toggle() -> void:
	visible = not visible

func _rebuild() -> void:
	if _list == null:
		return
	for c in _list.get_children():
		c.queue_free()
	for domain in DOMAINS:
		var dname: String = String(domain[0])
		var entries: Array = domain[1]

		var head := Label.new()
		head.text = dname
		head.add_theme_font_size_override("font_size", 17)
		head.add_theme_color_override("font_color", Color(0.86, 0.74, 0.46))
		_list.add_child(head)
		_list.add_child(HSeparator.new())

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

			var ou_lbl := Label.new()
			ou_lbl.text = "  Où : " + String(entry.get("ou", "—"))
			ou_lbl.add_theme_color_override("font_color", Color(0.55, 0.62, 0.72))
			ou_lbl.add_theme_font_size_override("font_size", 13)
			ou_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
			row.add_child(ou_lbl)

			var regle_lbl := Label.new()
			regle_lbl.text = "  Règle : " + String(entry.get("regle", "—"))
			regle_lbl.add_theme_color_override("font_color", Color(0.62, 0.60, 0.58))
			regle_lbl.add_theme_font_size_override("font_size", 13)
			regle_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
			row.add_child(regle_lbl)

			_list.add_child(row)
