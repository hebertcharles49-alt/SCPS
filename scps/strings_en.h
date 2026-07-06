/* strings_en.h — l'ANGLAIS : la table jumelle, née copie conforme du FR.
 * La traduction est une session de PUR REMPLISSAGE (diffable, zéro logique).
 * MÊME LISTE, MÊME ORDRE que strings_ids.h — la table se construit
 * positionnellement : une ligne manquante/excédentaire casse le build
 * (assert de taille dans scps_lang.c).
 * Démonstration §5.4 (ordre des mots non universel) : STR_SLOT_ANCIEN est
 * traduite, et les emplacements {k} sont POSITIONNELS — une langue peut
 * écrire "{1} … {0}" sans toucher l'appelant. */
#define SCPS_STRINGS_EN(X) \
    X(STR_BANDE_STAB_0, "Submergée") \
    X(STR_BANDE_STAB_1, "Vacillante") \
    X(STR_BANDE_STAB_2, "Tenue") \
    X(STR_BANDE_STAB_3, "Assurée") \
    X(STR_BANDE_STAB_4, "Inébranlable") \
    X(STR_BANDE_ASSISE_0, "Consentie") \
    X(STR_BANDE_ASSISE_1, "Partagée") \
    X(STR_BANDE_ASSISE_2, "Contrainte") \
    X(STR_BANDE_ASSISE_3, "Tyrannique") \
    X(STR_BANDE_LEGIT_0, "Usurpée") \
    X(STR_BANDE_LEGIT_1, "Contestée") \
    X(STR_BANDE_LEGIT_2, "Tolérée") \
    X(STR_BANDE_LEGIT_3, "Reconnue") \
    X(STR_BANDE_LEGIT_4, "Sacrée") \
    X(STR_BANDE_CONCORDE_0, "Unie") \
    X(STR_BANDE_CONCORDE_1, "Murmurante") \
    X(STR_BANDE_CONCORDE_2, "Fracturée") \
    X(STR_BANDE_CONCORDE_3, "Sécession") \
    X(STR_BANDE_PROSP_0, "Misère") \
    X(STR_BANDE_PROSP_1, "Disette") \
    X(STR_BANDE_PROSP_2, "Suffisance") \
    X(STR_BANDE_PROSP_3, "Aisance") \
    X(STR_BANDE_PROSP_4, "Opulence") \
    X(STR_BANDE_SAVOIR_0, "Obscurité") \
    X(STR_BANDE_SAVOIR_1, "Lueur") \
    X(STR_BANDE_SAVOIR_2, "Foyer") \
    X(STR_BANDE_SAVOIR_3, "Phare") \
    X(STR_BANDE_PRESAGE_0, "Calme") \
    X(STR_BANDE_PRESAGE_1, "Frémissement") \
    X(STR_BANDE_PRESAGE_2, "Ombre grandissante") \
    X(STR_BANDE_PRESAGE_3, "Le seuil") \
    X(STR_BANDE_STATURE_0, "Désert") \
    X(STR_BANDE_STATURE_1, "Hameau") \
    X(STR_BANDE_STATURE_2, "Bourg") \
    X(STR_BANDE_STATURE_3, "Cité") \
    X(STR_BANDE_STATURE_4, "Métropole") \
    X(STR_BANDE_FLUX_0, "Exode") \
    X(STR_BANDE_FLUX_1, "Saignée") \
    X(STR_BANDE_FLUX_2, "Stable") \
    X(STR_BANDE_FLUX_3, "Afflux") \
    X(STR_BANDE_FLUX_4, "Ruée") \
    X(STR_BANDE_AISANCE_0, "Misère") \
    X(STR_BANDE_AISANCE_1, "Suffisance") \
    X(STR_BANDE_AISANCE_2, "Aisance") \
    X(STR_BANDE_AISANCE_3, "Faste") \
    X(STR_BANDE_CARREFOUR_0, "—") \
    X(STR_BANDE_CARREFOUR_1, "Florissante") \
    X(STR_BANDE_CARREFOUR_2, "Bouillonnante") \
    X(STR_BANDE_CARREFOUR_3, "En surchauffe") \
    X(STR_BANDE_HUMEUR_0, "Révoltée") \
    X(STR_BANDE_HUMEUR_1, "Frondeuse") \
    X(STR_BANDE_HUMEUR_2, "Tiède") \
    X(STR_BANDE_HUMEUR_3, "Loyale") \
    X(STR_BANDE_HUMEUR_4, "Dévouée") \
    X(STR_BANDE_LIGNEE_0, "Du même sang") \
    X(STR_BANDE_LIGNEE_1, "Cousine") \
    X(STR_BANDE_LIGNEE_2, "Sœur lointaine") \
    X(STR_BANDE_LIGNEE_3, "Étrangère") \
    X(STR_BANDE_LIGNEE_4, "Hérétique proche") \
    X(STR_BANDE_LIGNEE_5, "Inassimilable") \
    X(STR_BANDE_AGITATION_0, "Calme") \
    X(STR_BANDE_AGITATION_1, "Frémissante") \
    X(STR_BANDE_AGITATION_2, "Agitée") \
    X(STR_BANDE_AGITATION_3, "Insurgée") \
    X(STR_BANDE_FOI_0, "Dévote") \
    X(STR_BANDE_FOI_1, "Tiède") \
    X(STR_BANDE_FOI_2, "Hérétique") \
    X(STR_BANDE_SEDITION_0, "Concorde") \
    X(STR_BANDE_SEDITION_1, "Murmures") \
    X(STR_BANDE_SEDITION_2, "Tendue") \
    X(STR_BANDE_SEDITION_3, "Séditieuse") \
    X(STR_FORGE_0, "Forge rudimentaire") \
    X(STR_FORGE_1, "Forge artisanale") \
    X(STR_FORGE_2, "Manufacture") \
    X(STR_FORGE_3, "Industrie") \
    X(STR_PROF_0, "hors de portée") \
    X(STR_PROF_1, "savoir de surface") \
    X(STR_PROF_2, "savoir-faire d'atelier") \
    X(STR_PROF_3, "art profond") \
    X(STR_PROF_4, "secret jalousement gardé") \
    X(STR_ACCES_0, "lointain") \
    X(STR_ACCES_1, "à portée") \
    X(STR_ACCES_2, "imminent") \
    X(STR_ACCES_3, "acquis") \
    X(STR_MORAL_0, "ferme") \
    X(STR_MORAL_1, "éprouvé") \
    X(STR_MORAL_2, "vacillant") \
    X(STR_MORAL_3, "rompu") \
    X(STR_FIDELITE_0, "fidèle") \
    X(STR_FIDELITE_1, "tiède") \
    X(STR_FIDELITE_2, "frondeur") \
    X(STR_FIDELITE_3, "ligueur") \
    X(STR_MARCHE_0, "marché mort") \
    X(STR_MARCHE_1, "pénurie sévère") \
    X(STR_MARCHE_2, "tendu") \
    X(STR_MARCHE_3, "sain") \
    X(STR_MARCHE_4, "engorgé") \
    X(STR_LENS_0, "—") \
    X(STR_LENS_1, "Prospérité") \
    X(STR_LENS_2, "Humeur") \
    X(STR_LENS_3, "Marché") \
    X(STR_HOVER_STAB, "La solidité de l'ordre : un royaume assuré encaisse les chocs, un royaume vacillant cède au premier vent.") \
    X(STR_HOVER_ASSISE, "Sur quoi repose l'obéissance : l'adhésion des cœurs, ou le seul poids des armes.") \
    X(STR_HOVER_LEGIT, "Le droit reconnu au trône de régner ; sacrée, nul ne la conteste — usurpée, chacun guette la chute.") \
    X(STR_HOVER_CONCORDE, "L'unité des peuples sous une même couronne ; quand les coutures lâchent, les marges rêvent d'indépendance.") \
    X(STR_HOVER_PROSP, "La richesse qui circule et qu'on parvient à lever ; un royaume opulent rayonne, une disette le vide.") \
    X(STR_HOVER_SAVOIR, "Le savoir né aux carrefours des cultures ; il nourrit les arts et les arcanes.") \
    X(STR_HOVER_PRESAGE, "Ce que la quête de puissance attire ; plus on force l'arcane, plus l'ombre s'épaissit.") \
    X(STR_HOVER_STATURE, "L'ampleur de l'établissement humain, du hameau perdu à la cité grouillante.") \
    X(STR_HOVER_FLUX, "Le mouvement des âmes : un afflux gonfle la province, un exode la vide.") \
    X(STR_HOVER_AISANCE, "La richesse qui circule ici ; les carrefours prospèrent, les culs-de-sac s'étiolent.") \
    X(STR_HOVER_CARREFOUR, "Quand des cultures se croisent ici, la richesse afflue — jusqu'à ce que le flux déborde et que la ville-monde se déchire.") \
    X(STR_HOVER_HUMEUR, "Le cœur de la province envers la couronne ; loyale, elle paie sans broncher — frondeuse, elle attend l'étincelle.") \
    X(STR_HOVER_LIGNEE, "Ce qui la lie à la culture du trône ; le même sang se gouverne aisément, l'inassimilable jamais sans la force.") \
    X(STR_HOVER_AGITATION, "La colère qui monte dans la province ; soutenue, elle vire à la révolte — qu'apaisent la stabilité du royaume, la garnison et la légitimité.") \
    X(STR_HOVER_FOI, "The province's adherence to the throne's ideology; convinced, it feeds legitimacy — dissident, it breeds schism.") \
    X(STR_HOVER_SEDITION, "La tension d'une faction forte dont les valeurs s'opposent à la direction du régime ; séditieuse, elle complote le coup d'État pour imposer son éthos.") \
    X(STR_AGIT_CAUSE_COERCION,  "Coercion") \
    X(STR_AGIT_CAUSE_CULTURE,   "Foreign culture") \
    X(STR_AGIT_CAUSE_CHOC,      "Recent conquest") \
    X(STR_AGIT_CAUSE_GARNISON,  "Garrison") \
    X(STR_JLOG_CHOC_EFF, "Heightened unrest, fades over time") \
    X(STR_JLOG_POP,    "Population") \
    X(STR_JLOG_PROD,   "Production") \
    X(STR_JLOG_TRESOR, "Treasury") \
    X(STR_TUTO_TITLE_0, "1 · Ce monde se lit.") \
    X(STR_TUTO_TITLE_1, "2 · Le temps coule en jours.") \
    X(STR_TUTO_TITLE_2, "3 · Ton empire.") \
    X(STR_TUTO_TITLE_3, "4 · Décider coûte.") \
    X(STR_TUTO_TITLE_4, "5 · Les autres.") \
    X(STR_TUTO_TITLE_5, "6 · Le savoir voyage.") \
    X(STR_TUTO_TITLE_6, "7 · La Brèche.") \
    X(STR_TUTO_PAGE_0, "Ici, pas de pourcentages cachés : l'état des choses se dit en MOTS.\nUne province est Unie ou Fracturée, un peuple Loyal ou Frondeur,\nun marché Sain ou En pénurie. Survole : tout se définit en bas d'écran.") \
    X(STR_TUTO_PAGE_1, "En haut à droite : la date, l'âge du monde, la vitesse.\nESPACE met en pause — et en pause, tu peux tout consulter,\ntout ordonner. Rien ne presse jamais que toi.") \
    X(STR_TUTO_PAGE_2, "En haut : ton or, tes vivres, tes matériaux, et la santé de ta couronne —\nStabilité, Légitimité, Cohésion, Prospérité. Clique une province pour la voir\nde près ; ouvre la BARRE DE GAUCHE pour l'empire entier :\nÉconomie, Démographie, Stocks, Armée, Filtres.") \
    X(STR_TUTO_PAGE_3, "Tout ordre — bâtir, exploiter, déplacer, lever — entre dans une FILE\net prend des JOURS. Le prix s'affiche AVANT. Certains leviers rapportent\nvite et coûtent longtemps : mater une révolte tait la rue, pas la colère.") \
    X(STR_TUTO_PAGE_4, "Tes voisins vivent : ils commercent, s'allient, jalousent.\nOn peut les lier — l'allié, le protégé, le serf, la cité marchande —\net chaque lien a son prix. Un embargo est une arme ; une guerre se gagne\nsur le champ, au MORAL, pas au nombre.") \
    X(STR_TUTO_PAGE_5, "Ton arbre a un cœur et un cercle : le cœur se recherche,\nle CERCLE se gagne par le contact — commerce, frontières, peuples gouvernés.\nUne culture qu'on assimile est un savoir qu'on tarit.\nChoisis ce que tu fonds et ce que tu gardes distinct.") \
    X(STR_TUTO_PAGE_6, "Certaines voies sont plus que puissantes — elles sont AVIDES.\nChaque pacte sombre, chaque forge interdite, chaque culte imposé CHARGE le monde.\nLa Brèche n'interdit rien : elle attend. Ton empire tombera — ils tombent tous.\nLa seule question est COMMENT, et ce que tu laisseras debout.") \
    X(STR_MENU_SOUS_TITRE, "un monde qui ne vous attend pas — et qui se lit") \
    X(STR_MENU_JOUER, "Jouer") \
    X(STR_MENU_CHARGER, "Charger") \
    X(STR_MENU_TUTORIEL, "Tutoriel") \
    X(STR_MENU_QUITTER, "Quitter") \
    X(STR_MENU_LANGUE, "Language: {0}") \
    X(STR_SETUP_TITRE, "FORGER UN MONDE") \
    X(STR_PAUSE_TITRE, "PAUSE") \
    X(STR_PM_REPRENDRE, "Reprendre") \
    X(STR_PM_SAUVER, "Sauver") \
    X(STR_PM_TUTORIEL, "Tutoriel") \
    X(STR_PM_MENU, "Menu principal") \
    X(STR_PM_QUITTER, "Quitter") \
    X(STR_PICK_SAUVER, "SAUVER — choisir un slot") \
    X(STR_PICK_CHARGER, "CHARGER — choisir un slot") \
    X(STR_SLOT_LINE, "Slot {0} — {1}") \
    X(STR_SLOT_ANCIEN, "Slot {0} — a save from an earlier era") \
    X(STR_SLOT_VIDE, "Slot {0} — vide") \
    X(STR_TUTO_PREC, "◀ préc.") \
    X(STR_TUTO_SUIV, "suiv. ▶") \
    X(STR_TUTO_PAGEFMT, "{0} / 7") \
    X(STR_OCCUPEE_PAR, "Occupied by {0}") \
    X(STR_RAIL_DIPLO, "Diplomacy (G) — living realms · wars · casus belli") \
    X(STR_DIPLO_TITRE, "DIPLOMACY") \
    X(STR_DIPLO_NEUTRE, "Neutral") \
    X(STR_DIPLO_ALLIE, "Allied") \
    X(STR_DIPLO_GUERRE, "At war") \
    X(STR_DIPLO_VASSAL, "Vassal") \
    X(STR_DIPLO_SUZERAIN, "Suzerain") \
    X(STR_DIPLO_DECLARER, "Declare war") \
    X(STR_DIPLO_NEGOCIER, "Negotiate peace") \
    X(STR_DIPLO_SANS_CB, "No ground for war holds (no casus belli)") \
    X(STR_DIPLO_SCORE_FMT, "score {0}") \
    X(STR_DIPLO_PAIX_FMT, "Peace: score {0}/50 or {1}/10 yrs") \
    X(STR_DIPLO_RANCUNE, "bitter grievance") \
    X(STR_PACT_ACTIF,  "trade pact \xe2\x9c\x93") \
    X(STR_PACT_GLOBAL, "pact \xe2\x9c\x93 \xc2\xb7 global market access") \
    X(STR_PACT_AUCUN,  "no trade pact") \
    X(STR_PACT_SIGN,   "Sign a pact") \
    X(STR_PACT_BREAK,  "Break the pact") \
    X(STR_PACT_HOV,    "Trade pact: RECIPROCAL access to the partner's GLOBAL market if either holds a Centre. Revocable.") \
    X(STR_DIPLO_MOTIF_FMT, "cause: {0}") \
    X(STR_DIPLO_RACE_FMT, "Heritage: {0}") \
    X(STR_DIPLO_STATUT_FMT, "Status: {0}") \
    X(STR_DIPLO_MENACE_FMT, "Threat: {0}%") \
    X(STR_DIPLO_ACTIONS, "Actions — Diplomacy tab (G)") \
    X(STR_CB_TERRITORIAL, "claimed border") \
    X(STR_CB_RELIGIOUS, "ideological schism") \
    X(STR_CB_ECONOMIC, "monopolised good") \
    X(STR_CB_SUBJUGATION, "subjugation") \
    X(STR_CB_ANTIPIRATERIE, "raiding to curb") \
    X(STR_PAIX_REFUS, "The enemy refuses: the war has not bled enough") \
    X(STR_JRN_GUERRE_PAR, "War declared by {0}") \
    X(STR_JRN_GUERRE_CONTRE, "War declared on {0}") \
    X(STR_JRN_PAIX, "Peace signed with {0}") \
    X(STR_JRN_CAPITULE, "Capitulation to {0}") \
    X(STR_JRN_MORT, "{0} has vanished from the map") \
    X(STR_DEFAITE_TITRE, "DEFEAT") \
    X(STR_DEFAITE_LIGNE, "Year {0} — your realm is no more") \
    X(STR_DEFAITE_OBSERVER, "Observe the world") \
    X(STR_DEFAITE_MENU, "Main menu") \
    X(STR_ARMEE_DEMOB, "[disband]  {0} regiments return home") \
    X(STR_ARMEE_DEMOB_HOV, "Disband the army: the WEAPONS are consumed (no materials refunded); the men RETURN to their place of origin and become labour again.") \
    X(STR_ARMEE_LEVY_LOCK_GUERRE, "Locked: build the BARRACKS (tech tree) to open the war footing.") \
    X(STR_ARMEE_LEVY_LOCK_MASSE, "Locked: CONSCRIPTION (after the Barracks) opens the mass levy.") \
    X(STR_FACTION_ETHOS_0, "the way of force — army, war, expansion") \
    X(STR_FACTION_ETHOS_1, "the way of gold — routes, markets, profit") \
    X(STR_FACTION_ETHOS_2, "the way of order — law, administration, stability") \
    X(STR_FACTION_ETHOS_3, "the way of tradition — land, ideology, continuity") \
    X(STR_FACTION_ETHOS_4, "the way of the forbidden — arcana, risk, taboo crossed") \
    X(STR_FACTION_ETHOS_5, "the common folk — bread, peace, daily safety") \
    X(STR_FACTION_HOV_FMT, "{0} · {1}. Satisfaction {2}% = their buy-in to the regime; share {3}% = their political weight.") \
    X(STR_FACTION_HOV_COUP, " — ALIENATED & POWERFUL: a coup is brewing.") \
    X(STR_CENTRE_RESEAU_OUVERT, "Inter-country network OPEN (a Trade Hub held)") \
    X(STR_CENTRE_RESEAU_FERME, "Inter-country network CLOSED — no Trade Hub (conquer one)") \
    X(STR_CENTRE_COMMERCIAL, "Trade Hub — node of the inter-regional network (hold it = trade)") \
    X(STR_PAN_MARCHE, "MARKET") \
    X(STR_MARCHE_PRIX_FMT, "current price {0} gold per unit") \
    X(STR_MARCHE_PRIX_HOV, "The domestic market price: demand pulls it up, supply and selling ease it. Buying and selling happen at THIS price.") \
    X(STR_MARCHE_ROW_HOV, "{0} — in reserve {1}. Buy or sell in lots of 10 at the current price (gold flows through the treasury).") \
    X(STR_MARCHE_HDR_LOCAL,  "good            stock     ref.price") \
    X(STR_MARCHE_HDR_MARCHE, "good          price   avail") \
    X(STR_MARCHE_BUY_HOV,    "Buy (pumps the treasury) \xe2\x80\x94 step 10, Shift = 100.") \
    X(STR_MARCHE_SELL_HOV,   "Sell to the market \xe2\x80\x94 step 10, Shift = 100.") \
    X(STR_SLOT_VERROU_FMT, "{0} — locked ({1})") \
    X(STR_BTN_COMPTOIR_FMT, "Build a Trading Post here  ({0} gold)") \
    X(STR_COMPTOIR_HOV, "The Trading Post links the province to the nearest Trade Hub: the transport margin drops by a third at this end of merchant routes.") \
    X(STR_BTN_CENTER_FMT, "Build a Trade Centre  ({0} gold)") \
    X(STR_CENTER_HOV, "A Trade Centre makes this province a HUB of the GLOBAL network: buy/sell on the world market here. Requires coastal/estuary + mercantile vocation.") \
    X(STR_ENTREPOT_CAP_FMT, "stock {0}/{1} — Warehouses ×{2}") \
    X(STR_ROW_ENTREPOTS, "Warehouses") \
    X(STR_TOPBAR_MATERIAUX, "Materials") \
    X(STR_RES_BOIS, "Wood") \
    X(STR_RES_ARGILE, "Clay") \
    X(STR_RES_PIERRE, "Stone") \
    X(STR_RES_OUTILS, "Tools") \
    X(STR_ENTREPOT_HOV, "Without a Warehouse, regional stock saturates at 200 per resource (surplus is lost); each Warehouse built adds +500. Buy low, sell high.") \
    X(STR_MER_CABOTAGE, "coastal · fixed speed") \
    X(STR_MER_MORTE,    "dead waters · ×3 time") \
    X(STR_MER_VIVE,     "lively waters") \
    X(STR_MER_COURANT,  "current · with ÷2.2 · against ×2.5") \
    X(STR_MER_DIR_FMT,  "{0} · {1}") \
    X(STR_MER_DIR_EST,  "eastward") \
    X(STR_MER_DIR_OUEST,"westward") \
    X(STR_MER_DIR_SUD,  "southward") \
    X(STR_MER_DIR_NORD, "northward") \
    X(STR_EDI_TRIBUNAL,     "Courthouse") \
    X(STR_EDI_CHANCELLERIE, "Chancellery") \
    X(STR_EDI_ACADEMIE,     "Academy") \
    X(STR_EDI_GARNISON,     "Garrison") \
    X(STR_EDI_FORTERESSE,   "Fortress") \
    X(STR_EDI_CITADELLE,    "Citadel") \
    X(STR_EDI_PORT,         "Port") \
    X(STR_EDI_CARAVANSERAIL,"Caravanserai") \
    X(STR_EDI_MARCHE,       "Market") \
    X(STR_EDI_ENTREPOT,     "Warehouse") \
    X(STR_EDI_GRENIER,      "Granary") \
    X(STR_EDI_IRRIGATION,   "Irrigation") \
    X(STR_EDI_AQUEDUC,      "Aqueduct") \
    X(STR_EDI_SANCTUAIRE,   "Shrine") \
    X(STR_EDI_TEMPLE,       "Temple") \
    X(STR_EDI_CATHEDRALE,   "Cathedral") \
    X(STR_EDI_BIBLIOTHEQUE, "Library") \
    X(STR_EDI_MONASTERE,    "Monastery") \
    X(STR_EDI_ARSENAL,      "Arsenal") \
    X(STR_EDI_AMIRAUTE,     "Admiralty") \
    X(STR_EDI_PORT_MARCHAND,"Merchant Harbour") \
    X(STR_EDI_BIBLIO_MIL,   "War Library") \
    X(STR_EDI_OBSERVATOIRE, "Observatory") \
    /* M7 — fork chronicle templates (3 variants each, {0}=place). */ \
    X(STR_FORK_ARSENAL_0,      "The war-masters of {0} turn the docks into an Arsenal.") \
    X(STR_FORK_ARSENAL_1,      "{0} arms its docks: the Arsenal rises.") \
    X(STR_FORK_ARSENAL_2,      "At {0}, the harbour becomes an Arsenal — fleet before trade.") \
    X(STR_FORK_AMIRAUTE_0,     "The Chancellery of {0} imposes a maritime doctrine: the Admiralty is born.") \
    X(STR_FORK_AMIRAUTE_1,     "{0} grants its harbour an Admiralty — the sea enters the registers.") \
    X(STR_FORK_AMIRAUTE_2,     "The Admiralty of {0} takes the sea in hand.") \
    X(STR_FORK_PORT_MARCH_0,   "The merchants of {0} win privileges and warehouses: the Merchant Harbour becomes the heart of the city.") \
    X(STR_FORK_PORT_MARCH_1,   "{0} opens its docks to trade: the Merchant Harbour prevails.") \
    X(STR_FORK_PORT_MARCH_2,   "At the Merchant Harbour of {0}, everything is for sale — even peace.") \
    X(STR_FORK_FORGE_0,        "The Celestial Iron is entrusted to the rune-smiths of {0}. Victory draws near; reality, less steady.") \
    X(STR_FORK_FORGE_1,        "{0} lights its Rune Forge — the flux thickens.") \
    X(STR_FORK_FORGE_2,        "At {0}, sky-fallen metal becomes a weapon. The flux remembers.") \
    X(STR_FORK_ALAMBIC_0,      "Saltpetre distilled at {0} curbs flux accidents — the guilds claim their share.") \
    X(STR_FORK_ALAMBIC_1,      "{0} distils stability: the Alembic soothes the flux.") \
    X(STR_FORK_ALAMBIC_2,      "The Alembic of {0} sells calm — priced in saltpetre.") \
    X(STR_EDI_COMPTOIR,     "Trading Post") \
    X(STR_EDI_BANQUE,       "Bank") \
    X(STR_EDI_TRADE_CENTER, "Trade Center") \
    X(STR_FAC_CONQUERANT,    "Conquerors") \
    X(STR_FAC_MARCHAND,      "Merchants") \
    X(STR_FAC_LEGISTE,       "Legalists") \
    X(STR_FAC_GARDIEN,       "Guardians") \
    X(STR_FAC_TRANSGRESSEUR, "Transgressors") \
    X(STR_FAC_COMMUNAUTAIRE, "Communalists") \
    X(STR_FATAL_TITRE, "SCPS — startup failed") \
    X(STR_FATAL_SDL,   "SCPS could not initialize the display.\n\n{0}") \
    X(STR_LOADING_MONDE, "Shaping the world…") \
    X(STR_LOADING_EVEIL, "The world awakens — years are passing…") \
    X(STR_COUNCIL_TITRE, "COUNCIL") \
    X(STR_COUNCIL_SEAT_0, "Knowledge") \
    X(STR_COUNCIL_SEAT_1, "Society") \
    X(STR_COUNCIL_SEAT_2, "Industry") \
    X(STR_COUNCIL_EFF_0, "research") \
    X(STR_COUNCIL_EFF_1, "promotion") \
    X(STR_COUNCIL_EFF_2, "manufactures") \
    X(STR_COUNCIL_VACANT, "— seat vacant —") \
    X(STR_COUNCIL_NOMMER, "Appoint") \
    X(STR_COUNCIL_RENVOYER, "Dismiss") \
    X(STR_COUNCIL_SEAT_FMT, "{0} — +{1}% {2}") \
    X(STR_COUNCIL_SEATED_FMT, "{0} · tier {1} · {2} gold/mo") \
    X(STR_COUNCIL_CAND_FMT, "{0} · tier {1} · {2} gold") \
    X(STR_COUNCIL_NAME_0, "House Vœrn") \
    X(STR_COUNCIL_NAME_1, "Aldric Counting-house") \
    X(STR_COUNCIL_NAME_2, "Harmel Guild") \
    X(STR_COUNCIL_NAME_3, "Orlec Bank") \
    X(STR_COUNCIL_NAME_4, "House Tessari") \
    X(STR_COUNCIL_NAME_5, "Velmor Circle") \
    X(STR_COUNCIL_NAME_6, "Brask Lodge") \
    X(STR_COUNCIL_NAME_7, "Dovric Syndic") \
    /* V2a — THE LIVING COUNCIL: the minister's mood (word derived from 0-100 loyalty). */ \
    X(STR_COUNCIL_MOOD_DEVOUE, "devoted") \
    X(STR_COUNCIL_MOOD_LOYAL, "loyal") \
    X(STR_COUNCIL_MOOD_TIEDE, "lukewarm") \
    X(STR_COUNCIL_MOOD_AIGRI, "embittered") \
    X(STR_COUNCIL_MOOD_TRAHISON, "ON THE VERGE OF BETRAYAL") \
    X(STR_COUNCIL_PAY_LABEL, "Pay") \
    /* CAPSTONE §27 — world Entropy (shared fate, not per-country). */ \
    X(STR_BANDE_ENTROPIE_0, "Stable") \
    X(STR_BANDE_ENTROPIE_1, "Stirring") \
    X(STR_BANDE_ENTROPIE_2, "Unstable") \
    X(STR_BANDE_ENTROPIE_3, "On the brink") \
    X(STR_HOVER_ENTROPIE, "The world's drift toward the Breach: faustian knowledge and transmutation stoke it. At the threshold, the real gives way.") \
    X(STR_AUGURE_ENTROPIE_0, "The sky takes on a hue no almanac can name.") \
    X(STR_AUGURE_ENTROPIE_1, "Needles spin wild; matter falters on its own laws.") \
    X(STR_AUGURE_ENTROPIE_2, "The real grows thin. The threshold of the Breach awaits only its shape.") \
    /* PROVINCE MODIFIERS (diegetic) — province UI slot (multiple). */ \
    X(STR_PMOD_SECTION,        "MODIFIERS") \
    X(STR_PMOD_FAVEUR,         "Boon") \
    X(STR_PMOD_FLEAU,          "Bane") \
    X(STR_PMOD_CICATRICE_NOM,  "Scar of Revolt") \
    X(STR_PMOD_CICATRICE_EFF,  "A province recently risen or sacked develops poorly: growth and production are gashed until the wound closes.") \
    X(STR_PMOD_ABONDANCE_NOM,  "Land of Plenty") \
    X(STR_PMOD_ABONDANCE_EFF,  "A wide, well-fed land at peace draws families in: births soar while its fields are not yet full.") \
    X(STR_PMOD_FERVEUR_NOM,    "Founding Fervor") \
    X(STR_PMOD_FERVEUR_EFF,    "A freshly founded colony hungers for the future: its early years carry a surge of births that settles as it takes root.") \
    X(STR_PMOD_RECONSTRUCTION_NOM, "Reconstruction") \
    X(STR_PMOD_RECONSTRUCTION_EFF, "Once the wound of revolt or sack has closed, the province rebounds: post-shock reconstruction quickens births.") \
    X(STR_PMOD_LIMON_NOM,      "Fertile Silt") \
    X(STR_PMOD_LIMON_EFF,      "A great river's mouth lays down rich silt: the delta's fields feed a dense population.") \
    X(STR_PMOD_GIBIER_NOM,     "Abundant Game") \
    X(STR_PMOD_GIBIER_EFF,     "The woods teem with game: the hunt fills tables and sustains a denser population.") \
    X(STR_PMOD_HALIEU_NOM,     "Fishery Bounty") \
    X(STR_PMOD_HALIEU_EFF,     "Shoals of fish swarm offshore: the catch feeds a populous coast.") \
    X(STR_PMOD_ADMIN_NOM,      "Good Governance") \
    X(STR_PMOD_ADMIN_EFF,      "Solid institutions keep order and services: sheltered from disorder, families prosper.") \
    X(STR_PMOD_ANNEX_NOM,      "Recent Annexation") \
    X(STR_PMOD_ANNEX_EFF,      "A province torn from its former master by annexation carries a wound of pride: stability stays brittle and the mood sulks until minds have settled under the new banner.") \
    /* GLOSSARY concept TITLES (hover_*) — twin order; definitions reuse STR_HOVER_*. */ \
    X(STR_GLOSS_STAB,      "Stability") \
    X(STR_GLOSS_LEGIT,     "Legitimacy") \
    X(STR_GLOSS_CONCORDE,  "Cohesion") \
    X(STR_GLOSS_ASSISE,    "Footing") \
    X(STR_GLOSS_PROSP,     "Prosperity") \
    X(STR_GLOSS_MARCHE,    "Market") \
    X(STR_GLOSS_AISANCE,   "Affluence") \
    X(STR_GLOSS_HUMEUR,    "Mood") \
    X(STR_GLOSS_LIGNEE,    "Lineage") \
    X(STR_GLOSS_AGITATION, "Unrest") \
    X(STR_GLOSS_SAVOIR,    "Knowledge") \
    X(STR_GLOSS_PRESAGE,   "Omen") \

