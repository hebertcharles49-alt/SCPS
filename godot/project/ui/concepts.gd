extends RefCounted
## CONCEPTS — le REGISTRE des mots de jeu (retour joueur 2026-07-10 : « tout mot qui
## veut dire quelque chose doit changer de couleur dans le hover et avoir une
## définition » + « chaque mot du codex doit être accompagné de son icône ») :
## UNE source de vérité, consommée par (1) le TooltipServer — colore chaque concept
## en turquoise dans TOUT hover et appose sa définition (avec icône) dessous — et
## (2) le CODEX (domaine « Concepts & jauges » généré d'ici, icône par entrée).
## Chaque définition est ANCRÉE sur le mécanisme moteur réel (discipline membrane).

const COL := "5ac8c0"   ## turquoise des concepts (hex BBCode)
const ICON_DIR := "res://assets/scps/ui/icons/"

## nom affiché → { d: définition d'une ligne, i: icône (assets/scps/ui/icons) }
const DEFS := {
	# ── jauges nationales ──
	"Stabilité": {"d": "L'ordre intérieur du royaume (0-100). Haute : moins d'agitation et de révoltes. Nourrie par les institutions, la légitimité et certaines techs.", "i": "stability_shield"},
	"Prospérité": {"d": "La richesse vécue (0-100), lue de la production, du commerce et des besoins comblés. Elle nourrit la croissance.", "i": "prosperity_sprout"},
	"Légitimité": {"d": "Le droit reconnu de régner (0-100). Basse : coups d'État et sécessions menacent. Portée par la foi, les institutions, les victoires.", "i": "politics_crown"},
	"Cohésion": {"d": "L'unité des peuples de l'empire (0-100). Haute quand les cultures sont intégrées ; basse quand la fracture travaille.", "i": "happiness_medallion"},
	"Influence": {"d": "La réputation diplomatique. Elle ouvre les offres : alliances, pactes, ligues.", "i": "influence_compass"},
	"Savoir": {"d": "Les points de recherche produits par l'empire (noblesse, bibliothèques, académies). C'est lui qui paie les technologies.", "i": "knowledge_book"},
	"Trésor": {"d": "L'or du royaume : taxes moins entretien, cour, chantiers et redépense publique.", "i": "gold_coin"},
	# ── province & peuples ──
	"Satisfaction": {"d": "La part du panier (vivres, biens, confort) réellement servie à une classe. Basse chez les laboureurs : misère, agitation, révolte.", "i": "health_food_bowl"},
	"Panier": {"d": "Les besoins d'une classe : vivres d'abord, puis biens et confort. La part servie fait la Satisfaction.", "i": "menu_stocks"},
	"Loyauté": {"d": "La légitimité locale d'une province. Elle chute sous la surtaxe, la coercition et les cicatrices.", "i": "politics_law"},
	"Agitation": {"d": "La colère accumulée d'une province. Au seuil, le soulèvement — une vraie guerre civile.", "i": "unrest_flame"},
	"Cicatrice": {"d": "La plaie durable d'une révolte, d'un sac ou d'une annexion : elle pèse sur la province des années puis se referme.", "i": "alert_warning"},
	"Coercition": {"d": "La puissance de contrainte (armée, répression). Elle tient les provinces rétives mais use leur satisfaction.", "i": "public_order_gavel"},
	"Sécurité": {"d": "La poigne locale d'une province (0-100) : elle tient les rétifs et décourage la révolte, mais use la loyauté de tous.", "i": "public_order_gavel"},
	"Développement": {"d": "La capacité productive de la province (0-100) : outillage, ville, techniques. 50 = production doublée ; pillage, friche et terre rude la rongent.", "i": "capital_tower"},
	"Capacité admin.": {"d": "Ce que l'État tient dans la province (0-100) : institutions et coercition bâties. S'use avec le temps — rénover la restaure.", "i": "tax_ledger"},
	"Service": {"d": "Ce qui apaise une province (culte, aménités) : l'agitation retombe, la loyauté tient.", "i": "faith_candle"},
	"Fracture": {"d": "La tension entre peuples tenus de force. Elle nourrit les sécessions et fragilise la diversité.", "i": "alert_warning"},
	"Sécession": {"d": "Une marche qui proclame son indépendance quand fracture et agitation l'emportent sur la couronne.", "i": "layer_border"},
	"Démographie": {"d": "La croissance de population, portée par la nourriture, la paix et le logement.", "i": "population_group"},
	"Famine": {"d": "Une région qui ne mange plus à sa faim : la croissance s'inverse, l'agitation monte. Greniers, imports ou colonie vivrière la brisent.", "i": "alert_famine"},
	"Grenier": {"d": "La réserve vivrière : elle amortit les disettes et suit le besoin de tout l'empire.", "i": "grain_bundle"},
	"Logement": {"d": "Ce que le bâti peut porter : la population croît vers ce plafond, les manufactures l'élèvent.", "i": "capital_tower"},
	# ── culture & métabolisation ──
	"Héritage": {"d": "La lignée d'un peuple : elle donne ses noms et ouvre nativement sa branche de l'arbre des techs.", "i": "culture_lyre"},
	"Éthos": {"d": "L'âme d'un État : elle compose son armée, ses factions de cour, son épithète et ses évènements.", "i": "prestige_laurel"},
	"Tradition": {"d": "Un trait de peuple (physique, social ou intellectuel) : un levier chiffré, en atout ou en défaut.", "i": "action_upgrade"},
	"Perméabilité": {"d": "L'ouverture d'un peuple : à quelle vitesse il assimile les minorités et se laisse lui-même assimiler au contact.", "i": "alert_migration"},
	"Dérive culturelle": {"d": "La vitesse de changement d'une culture : haute, elle épouse le lieu ; basse, elle se transmet intacte.", "i": "culture_lyre"},
	"Capacité": {"d": "L'aptitude d'un État à tenir la diversité et la distance (institutions, administration) : les services suivent, la pénalité minoritaire s'allège.", "i": "tax_ledger"},
	"Métabolisation": {"d": "La digestion d'un peuple étranger arrivé chez vous : elle déverrouille ses techs, accélère votre recherche et compte pour l'Ascension.", "i": "menu_demography"},
	"Productivité": {"d": "Le rendement du travail : extraction, ateliers et recherche.", "i": "development_tools"},
	# ── foi ──
	"Foi": {"d": "La religion d'État : fondée au premier temple, elle nourrit la légitimité et peut se fendre en schismes.", "i": "faith_candle"},
	"Crédo": {"d": "Le tempérament d'une foi : pluraliste (tolère), évangéliste (convertit) ou purificateur (impose).", "i": "faith_candle"},
	"Schisme": {"d": "Une foi qui se fend : la secte-fille emporte les régions distantes ou mal tenues (2 sectes max par racine).", "i": "faith_candle"},
	# ── arcane & fin de partie ──
	"Affinité arcane": {"d": "L'accès à la branche Magie de l'arbre. Chaque pas y est une pente faustienne.", "i": "action_research"},
	"Charge faustienne": {"d": "La dette accumulée envers la Brèche par les techs interdites. Elle nourrit l'Entropie du monde.", "i": "alert_event_bell"},
	"Flux": {"d": "La magie qui fuit en permanence d'un ouvrage arcanique. Il faut la contenir : elle nourrit la Brèche.", "i": "layer_river"},
	"Entropie": {"d": "L'usure du monde (0-100), chargée par le faustien et les transmuteurs. Au seuil, une FIN se déclenche.", "i": "alert_disease"},
	"Brèche": {"d": "La déchirure que la magie interdite agrandit. Toutes les fins arcanes en sortent.", "i": "alert_event_bell"},
	"Merveille": {"d": "L'ouvrage de l'Ascension : trois paliers (Forge, Société, Savoir) nourris de rares — la victoire du joueur.", "i": "capital_tower"},
	"Ascension": {"d": "La victoire par la Merveille : achever Forge, Société puis Savoir avant une apocalypse. Les paliers exigent 3, 4 puis 6 héritages intégrés ou en contact profond, et leurs ressources rares.", "i": "capital_tower"},
	# ── diplomatie & guerre ──
	"Opinion": {"d": "Ce qu'un pays pense de nous (±100) : statuts actifs + mémoire des actes (la trahison marque), le tout tendant vers zéro.", "i": "dipl_alliance"},
	"Casus belli": {"d": "Le motif de guerre utilisable : gratuit (défense, subjugation) ou revendication payante qui mûrit un an.", "i": "dipl_rivalry"},
	"Revendication": {"d": "Un casus belli fabriqué : il coûte deux ans du revenu de la cible, mûrit un an, reste valable cinq ans.", "i": "action_decree"},
	"Trêve": {"d": "La paix imposée après une guerre : redéclarer avant son terme est une trahison qui marque l'opinion.", "i": "tool_lock"},
	"Embargo": {"d": "La fermeture unilatérale du commerce avec un pays : son opinion en souffre, vos routes aussi.", "i": "alert_shortage"},
	"Vassalité": {"d": "Un État soumis qui verse selon sa fonction (vivres, soldats ou couronnes). Bien intégré, un maître annexeur peut le digérer.", "i": "dipl_vassal"},
	"Annexion": {"d": "La digestion d'un vassal intégré : ses régions passent à la couronne, une cicatrice douce en mémoire.", "i": "action_treaty"},
	"Rancœur": {"d": "Le grief d'une faction envers la couronne. Haute, sa barre vire au rouge : elle conspire.", "i": "alert_revolt"},
	"Faction": {"d": "Un parti de la cour (l'éthos les compose). Son adhésion la rend puissante, sa rancœur la rend dangereuse.", "i": "menu_council"},
	"Tension de coup": {"d": "Le risque de coup d'État porté par la cour : il monte avec les factions aigries et la corruption.", "i": "action_spy"},
	"Corruption": {"d": "Part de l'appareil d'État capturée par les factions. Elle réduit la recherche, le rendement de la capitale et la capacité des services, puis accélère les chutes de loyauté.", "i": "corruption_coin"},
	"Levée": {"d": "La réserve mobilisable du royaume, en régiments. Elle se règle au tiroir Armée.", "i": "action_recruit"},
	"Ost": {"d": "L'armée de campagne déployée sur la carte, avec sa phase (marche, siège, bataille).", "i": "menu_army"},
	"Siège": {"d": "L'investissement d'une région : le terrain prolonge sa tenue, la chute donne l'occupation et le score.", "i": "alert_siege"},
	# ── économie ──
	"Puissance commerciale": {"d": "Le volume de biens achetable au marché ce mois-ci (bourgeois et élite × chaîne commerciale).", "i": "action_trade"},
	"Marché": {"d": "Le réseau des Centres tenus par les cités-états : le local au plus proche, le mondial à double taxe.", "i": "menu_market"},
	"Route commerciale": {"d": "Un lien de négoce ouvert : deux ports + deux marchés + (pacte ou même couronne). La distance module le rendement, jamais ne le tue.", "i": "layer_road"},
	"Port": {"d": "La rade d'une région côtière : sans port, ni route de mer ni flotte.", "i": "harbor_anchor"},
	"Colonisation": {"d": "La fondation d'une province vierge : une colonne part, mûrit selon la distance, et la ferveur fondatrice porte les premières années.", "i": "settlement_cluster"},
	"Impôts": {"d": "La part de la richesse levée par la couronne, par mois — rognée par l'évasion quand la satisfaction baisse ou que l'éthos tolère mal l'impôt.", "i": "tax_ledger"},
	"Services": {"d": "Les âmes SERVIES (échoppes, bains, cultes) sur la capacité que porte le bâti — au-delà, le confort décroche.", "i": "capital_tower"},
	"Friche": {"d": "Un édifice ou une manufacture dont l'entretien reste impayé : sa production tombe à 60 % tant que la dette n'est pas résorbée.", "i": "alert_warning"},
	"Entretien": {"d": "Le coût récurrent d'un bâti, par mois — impayé, la province tombe EN FRICHE.", "i": "tax_ledger"},
	"Palier": {"d": "Le rang suivant d'un édifice : chaque palier suit le TIER du bourg et déverrouille de nouveaux effets.", "i": "layer_terrain"},
	# ── monnaie ──
	"Débase": {"d": "La frappe au-delà de la parité : elle gonfle la masse en circulation et dilue la valeur de la monnaie. Elle ronge la confiance des institutions de la capitale et attise les marchands — un prix qui se répare sur des décennies.", "i": "corruption_coin"},
	"Billon fondu": {"d": "Le métal célibataire (sans son pair d'alliage) que la Débase frappe quand même, à sa vieille parité : la mauvaise monnaie qui garde le royaume liquide.", "i": "corruption_coin"},
	"Apparié": {"d": "La part de la réserve qui a son pair : chaque tonne d'or mariée à sa tonne de cuivre frappera la bonne monnaie d'alliage.", "i": "gold_coin"},
	"Célibataire": {"d": "Le métal en trop, sans pair à épouser : il attend — mine, commerce ou conquête de l'autre métal — ou finit en billon si la Débase s'en mêle.", "i": "corruption_coin"},
	"Métal fondu": {"d": "La matière réellement convertie en monnaie ce mois-ci : l'alliage exige une tonne d'or POUR une tonne de cuivre — le métal rare borne tout.", "i": "gold_coin"},
	"Parité": {"d": "Le taux de conversion fixe entre l'or et le cuivre (16 pour 5,2) : l'étalon de toute la monnaie d'État.", "i": "debt_scales"},
	"Étalon": {"d": "La règle de conversion fixe qui ancre la monnaie du royaume — frapper au-delà, c'est la débase.", "i": "gold_coin"},
	"Péage": {"d": "La taxe perçue sur une route commerciale à son passage — un revenu tiers, pas un coût du joueur.", "i": "layer_road"},
	"Détroit": {"d": "Un passage maritime resserré : il concentre le commerce qui le traverse, d'où sa valeur stratégique.", "i": "layer_river"},
	"Frappe": {"d": "La conversion de la réserve métallique en monnaie de compte, au taux fixé par la Parité — au-delà, c'est la Débase.", "i": "gold_coin"},
	"Dette": {"d": "L'emprunt d'État aux ordres ou à un pays : intérêt annuel, saisie de production si tu la répudies (banqueroute).", "i": "debt_scales"},
}

static var _re: RegEx = null

static func _regex() -> RegEx:
	if _re != null:
		return _re
	# multi-mots d'abord (« Dérive culturelle » avant « Dérive »), bornes de mots FR
	var keys := DEFS.keys()
	keys.sort_custom(func(a, b): return String(a).length() > String(b).length())
	var alts := PackedStringArray()
	for k in keys:
		alts.append(String(k).replace(" ", "\\s") + "s?")   # pluriel toléré (« cicatrices »)
	_re = RegEx.new()
	_re.compile("(?i)(?<![\\wÀ-ÿ])(" + "|".join(alts) + ")(?![\\wÀ-ÿ])")
	return _re

## retrouve la clé canonique d'un match (insensible à la casse, pluriel toléré)
static func _key_of(m: String) -> String:
	var low := m.to_lower()
	for k in DEFS.keys():
		var kl := String(k).to_lower()
		if kl == low or kl + "s" == low:
			return String(k)
	return ""

static func def_of(k: String) -> String:
	return String(DEFS[k]["d"]) if DEFS.has(k) else ""

static func icon_of(k: String) -> String:
	return (ICON_DIR + String(DEFS[k]["i"]) + ".png") if DEFS.has(k) else ""

## D4 — GLOSSAIRE HOVER : comme def_of(), mais tolère un LABEL qui contient un concept
## plutôt que de L'ÊTRE exactement (casse indifférente, pluriel toléré, ex. « Entretien
## des bâtiments » → la définition d'Entretien, « Péages » → Péage) — réutilise le MÊME
## moteur de correspondance que decorate() (mots-bornés, clé la plus longue d'abord), pas
## une seconde règle à maintenir. Le PREMIER concept trouvé dans le texte gagne. Pour les
## panneaux dont le libellé moteur n'est pas le nom nu du concept (cf. province_panel_v2.
## gd::_kv, qui n'a besoin, lui, que de la correspondance EXACTE).
static func def_of_label(label: String) -> String:
	if DEFS.has(label):
		return def_of(label)
	# PIÈGE VÉRIFIÉ : `(?i)` (PCRE2, moteur RegEx de Godot) NE REPLIE PAS la casse des
	# majuscules ACCENTUÉES — « DÉBASE » ne matche PAS la clé « Débase » malgré le flag
	# case-insensitive (testé : « Débase »/« débase »/« SIÈGE » minuscule matchent tous,
	# seul le COUPLE majuscule-accentuée→clé échoue). `String.to_lower()` de Godot, lui,
	# replie correctement les accents français — on abaisse la casse AVANT decorate() :
	# seul l'APPARIEMENT compte ici (pas la casse affichée d'un BBCode, contrairement à
	# decorate() en usage hover normal, qu'on ne touche donc pas).
	var found: Array = decorate(label.to_lower()).get("defs", [])
	return def_of(String(found[0])) if not found.is_empty() else ""

## DÉCORE un texte : {bb: BBCode, defs: [Nom, …] (uniques)}. Chaque concept devient
## un LIEN [url=CLÉ] turquoise — inerte tant que le RichTextLabel ignore la souris,
## CASCADE quand le tooltip est verrouillé (tooltip_server).
static func decorate(text: String) -> Dictionary:
	var safe := text.replace("[", "[lb]")
	var out := ""
	var defs := []
	var seen := {}
	var pos := 0
	for m in _regex().search_all(safe):
		out += safe.substr(pos, m.get_start() - pos)
		var word := m.get_string()
		var k := _key_of(word)
		if k != "":
			out += "[url=%s][color=#%s]%s[/color][/url]" % [k, COL, word]
			if not seen.has(k):
				seen[k] = true
				defs.append(k)
		else:
			out += "[color=#%s]%s[/color]" % [COL, word]
		pos = m.get_end()
	out += safe.substr(pos)
	return {"bb": out, "defs": defs}
