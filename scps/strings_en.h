/* strings_en.h — l'ANGLAIS : la table jumelle, née copie conforme du FR.
 * La traduction est une session de PUR REMPLISSAGE (diffable, zéro logique).
 * MÊME LISTE, MÊME ORDRE que strings_ids.h — la table se construit
 * positionnellement : une ligne manquante/excédentaire casse le build
 * (assert de taille dans scps_lang.c).
 * Démonstration §5.4 (ordre des mots non universel) : STR_SLOT_ANCIEN est
 * traduite, et les emplacements {k} sont POSITIONNELS — une langue peut
 * écrire "{1} … {0}" sans toucher l'appelant. */
#define SCPS_STRINGS_EN(X) \
    X(STR_BANDE_STAB_0, "Submerged") \
    X(STR_BANDE_STAB_1, "Faltering") \
    X(STR_BANDE_STAB_2, "Holding") \
    X(STR_BANDE_STAB_3, "Secure") \
    X(STR_BANDE_STAB_4, "Unshakable") \
    X(STR_BANDE_ASSISE_0, "Consensual") \
    X(STR_BANDE_ASSISE_1, "Shared") \
    X(STR_BANDE_ASSISE_2, "Coerced") \
    X(STR_BANDE_ASSISE_3, "Tyrannical") \
    X(STR_BANDE_LEGIT_0, "Usurped") \
    X(STR_BANDE_LEGIT_1, "Contested") \
    X(STR_BANDE_LEGIT_2, "Tolerated") \
    X(STR_BANDE_LEGIT_3, "Recognised") \
    X(STR_BANDE_LEGIT_4, "Sacred") \
    X(STR_BANDE_CONCORDE_0, "United") \
    X(STR_BANDE_CONCORDE_1, "Murmuring") \
    X(STR_BANDE_CONCORDE_2, "Fractured") \
    X(STR_BANDE_CONCORDE_3, "Secession") \
    X(STR_BANDE_PROSP_0, "Misery") \
    X(STR_BANDE_PROSP_1, "Dearth") \
    X(STR_BANDE_PROSP_2, "Sufficiency") \
    X(STR_BANDE_PROSP_3, "Affluence") \
    X(STR_BANDE_PROSP_4, "Opulence") \
    X(STR_BANDE_SAVOIR_0, "Darkness") \
    X(STR_BANDE_SAVOIR_1, "Glimmer") \
    X(STR_BANDE_SAVOIR_2, "Hearth") \
    X(STR_BANDE_SAVOIR_3, "Beacon") \
    X(STR_BANDE_PRESAGE_0, "Calm") \
    X(STR_BANDE_PRESAGE_1, "Stirring") \
    X(STR_BANDE_PRESAGE_2, "Growing Shadow") \
    X(STR_BANDE_PRESAGE_3, "The Threshold") \
    X(STR_BANDE_STATURE_0, "Wilderness") \
    X(STR_BANDE_STATURE_1, "Hamlet") \
    X(STR_BANDE_STATURE_2, "Town") \
    X(STR_BANDE_STATURE_3, "City") \
    X(STR_BANDE_STATURE_4, "Metropolis") \
    X(STR_BANDE_FLUX_0, "Exodus") \
    X(STR_BANDE_FLUX_1, "Drain") \
    X(STR_BANDE_FLUX_2, "Stable") \
    X(STR_BANDE_FLUX_3, "Inflow") \
    X(STR_BANDE_FLUX_4, "Rush") \
    X(STR_BANDE_AISANCE_0, "Misery") \
    X(STR_BANDE_AISANCE_1, "Sufficiency") \
    X(STR_BANDE_AISANCE_2, "Affluence") \
    X(STR_BANDE_AISANCE_3, "Splendour") \
    X(STR_BANDE_CARREFOUR_0, "—") \
    X(STR_BANDE_CARREFOUR_1, "Flourishing") \
    X(STR_BANDE_CARREFOUR_2, "Bustling") \
    X(STR_BANDE_CARREFOUR_3, "Overheating") \
    X(STR_BANDE_HUMEUR_0, "Rebellious") \
    X(STR_BANDE_HUMEUR_1, "Defiant") \
    X(STR_BANDE_HUMEUR_2, "Lukewarm") \
    X(STR_BANDE_HUMEUR_3, "Loyal") \
    X(STR_BANDE_HUMEUR_4, "Devoted") \
    X(STR_BANDE_LIGNEE_0, "Kindred Blood") \
    X(STR_BANDE_LIGNEE_1, "Cousin") \
    X(STR_BANDE_LIGNEE_2, "Distant Sister") \
    X(STR_BANDE_LIGNEE_3, "Foreign") \
    X(STR_BANDE_LIGNEE_4, "Near-Heretic") \
    X(STR_BANDE_LIGNEE_5, "Unassimilable") \
    X(STR_BANDE_AGITATION_0, "Calm") \
    X(STR_BANDE_AGITATION_1, "Stirring") \
    X(STR_BANDE_AGITATION_2, "Agitated") \
    X(STR_BANDE_AGITATION_3, "Insurgent") \
    X(STR_BANDE_FOI_0, "Devout") \
    X(STR_BANDE_FOI_1, "Lukewarm") \
    X(STR_BANDE_FOI_2, "Heretical") \
    X(STR_BANDE_SEDITION_0, "Concord") \
    X(STR_BANDE_SEDITION_1, "Murmurs") \
    X(STR_BANDE_SEDITION_2, "Tense") \
    X(STR_BANDE_SEDITION_3, "Seditious") \
    X(STR_FORGE_0, "Rudimentary Forge") \
    X(STR_FORGE_1, "Artisan Forge") \
    X(STR_FORGE_2, "Manufactory") \
    X(STR_FORGE_3, "Industry") \
    X(STR_PROF_0, "out of reach") \
    X(STR_PROF_1, "surface knowledge") \
    X(STR_PROF_2, "workshop know-how") \
    X(STR_PROF_3, "deep mastery") \
    X(STR_PROF_4, "jealously guarded secret") \
    X(STR_ACCES_0, "distant") \
    X(STR_ACCES_1, "within reach") \
    X(STR_ACCES_2, "imminent") \
    X(STR_ACCES_3, "attained") \
    X(STR_MORAL_0, "steady") \
    X(STR_MORAL_1, "strained") \
    X(STR_MORAL_2, "wavering") \
    X(STR_MORAL_3, "broken") \
    X(STR_FIDELITE_0, "faithful") \
    X(STR_FIDELITE_1, "lukewarm") \
    X(STR_FIDELITE_2, "defiant") \
    X(STR_FIDELITE_3, "rebel") \
    X(STR_MARCHE_0, "dead market") \
    X(STR_MARCHE_1, "severe shortage") \
    X(STR_MARCHE_2, "tight") \
    X(STR_MARCHE_3, "healthy") \
    X(STR_MARCHE_4, "glutted") \
    X(STR_LENS_0, "—") \
    X(STR_LENS_1, "Prosperity") \
    X(STR_LENS_2, "Mood") \
    X(STR_LENS_3, "Market") \
    X(STR_HOVER_STAB, "The solidity of order: a secure realm absorbs the shocks, a faltering realm gives way at the first gust.") \
    X(STR_HOVER_ASSISE, "What obedience rests on: the consent of hearts, or the weight of arms alone.") \
    X(STR_HOVER_LEGIT, "The throne's recognised right to rule; sacred, none dispute it — usurped, everyone watches for the fall.") \
    X(STR_HOVER_CONCORDE, "The unity of peoples under one crown; when the seams give way, the borderlands dream of independence.") \
    X(STR_HOVER_PROSP, "The wealth that circulates and can be levied; an opulent realm shines, a dearth empties it.") \
    X(STR_HOVER_SAVOIR, "Knowledge born at the crossroads of cultures; it feeds the arts and the arcane.") \
    X(STR_HOVER_PRESAGE, "What the quest for power draws in; the harder the arcane is forced, the thicker the shadow grows.") \
    X(STR_HOVER_STATURE, "The scale of human settlement, from the lost hamlet to the teeming city.") \
    X(STR_HOVER_FLUX, "The movement of souls: an inflow swells the province, an exodus empties it.") \
    X(STR_HOVER_AISANCE, "The wealth that circulates here; crossroads flourish, dead ends wither.") \
    X(STR_HOVER_CARREFOUR, "When cultures cross paths here, wealth pours in — until the flow overflows and the world-city tears itself apart.") \
    X(STR_HOVER_HUMEUR, "The province's heart toward the crown; loyal, it pays without flinching — defiant, it waits for the spark.") \
    X(STR_HOVER_LIGNEE, "What binds it to the throne's culture; the same blood is governed with ease, the unassimilable never without force.") \
    X(STR_HOVER_AGITATION, "The anger rising in the province; sustained, it turns to revolt — eased by the realm's stability, its garrison, and its legitimacy.") \
    X(STR_HOVER_FOI, "The province's adherence to the throne's ideology; convinced, it feeds legitimacy — dissident, it breeds schism.") \
    X(STR_HOVER_SEDITION, "The tension of a powerful faction whose values oppose the regime's direction; seditious, it plots a coup to impose its ethos.") \
    X(STR_AGIT_CAUSE_COERCION,  "Coercion") \
    X(STR_AGIT_CAUSE_CULTURE,   "Foreign culture") \
    X(STR_AGIT_CAUSE_CHOC,      "Recent conquest") \
    X(STR_AGIT_CAUSE_GARNISON,  "Garrison") \
    X(STR_DEV_CAUSE_OUTIL,      "Tooling") \
    X(STR_DEV_CAUSE_VILLE,      "City") \
    X(STR_DEV_CAUSE_TECH,       "Techniques") \
    X(STR_DEV_CAUSE_PILLAGE,    "Pillage") \
    X(STR_DEV_CAUSE_FRICHE,     "Disrepair") \
    X(STR_DEV_CAUSE_TERRE,      "Harsh land") \
    X(STR_CAPADM_CAUSE_INST,    "Institutions") \
    X(STR_CAPADM_CAUSE_COERC,   "Built coercion") \
    X(STR_SERV_CAUSE_SAVOIR,    "Built learning") \
    X(STR_SERV_CAUSE_FOI,       "Built faith") \
    X(STR_AGE_FX_EXCHANGE,      "Trade +%d (lasting) · Prosperity +%d · migration pacts +%d %%\nUnlocks Society III · Merchants +%d (living routes)") \
    X(STR_AGE_FX_DISCOVERY,     "Trade +%d (lasting) · research +%d %%\nUnlocks Knowledge IV · Transgressors +%d · Merchants +%d (known worlds)") \
    X(STR_AGE_FX_EMPIRES,       "Integration +%d %% (lasting)\nUnlocks Society V · Conquerors +%d (provinces held %d years)") \
    X(STR_AGE_FX_HEROES,        "Heroes arise — their deeds mark the annals") \
    X(STR_AGE_FX_BREACH,        "Breach +%d — feeds Entropy\nUnlocks Knowledge V · Transgressors +%d (faustian charge)") \
    X(STR_AGE_FX_LUMIERES,      "Learning +%d · Legitimacy up to −%d (lettered men against a coercive throne)\nLegists +%d · Communals +%d") \
    X(STR_AGE_FX_SOULEVEMENTS,  "Legitimacy −%d\nCommunals +%d (revolutionary lands)") \
    X(STR_AGE_FX_TYRANS,        "Security +%d · Diversity −%d\nConquerors +%d · Legists +%d") \
    X(STR_JLOG_CHOC_EFF, "Heightened unrest, fades over time") \
    X(STR_JLOG_POP,    "Population") \
    X(STR_JLOG_PROD,   "Production") \
    X(STR_JLOG_TRESOR, "Treasury") \
    X(STR_TUTO_TITLE_0, "1 · This world can be read.") \
    X(STR_TUTO_TITLE_1, "2 · Time flows in days.") \
    X(STR_TUTO_TITLE_2, "3 · Your empire.") \
    X(STR_TUTO_TITLE_3, "4 · Deciding costs.") \
    X(STR_TUTO_TITLE_4, "5 · The others.") \
    X(STR_TUTO_TITLE_5, "6 · Knowledge travels.") \
    X(STR_TUTO_TITLE_6, "7 · The Breach.") \
    X(STR_TUTO_PAGE_0, "Here, no hidden percentages: the state of things is told in WORDS.\nA province is United or Fractured, a people Loyal or Defiant,\na market Healthy or In Shortage. Hover: everything is defined at the bottom of the screen.") \
    X(STR_TUTO_PAGE_1, "Top right: the date, the age of the world, the speed.\nSPACE pauses — and while paused, you can review everything,\norder everything. Nothing is ever urgent — except you.") \
    X(STR_TUTO_PAGE_2, "Top bar: your crowns, your food, your materials, and the health of your crown —\nStability, Legitimacy, Cohesion, Prosperity. Click a province to see it\nup close; open the LEFT SIDEBAR for the whole empire:\nEconomy, Demography, Stocks, Army, Filters.") \
    X(STR_TUTO_PAGE_3, "Every order — build, harvest, move, levy — enters a QUEUE\nand takes DAYS. The cost shows BEFORE you commit. Some levers pay off\nfast but cost long: crushing a revolt silences the street, not the anger.") \
    X(STR_TUTO_PAGE_4, "Your neighbours are alive: they trade, ally, and grow jealous.\nYou can bind them — the ally, the protectorate, the vassal, the merchant city —\nand every bond has its price. An embargo is a weapon; a war is won\non the field, on MORALE, not numbers.") \
    X(STR_TUTO_PAGE_5, "Your tree has a core and a ring: the core is researched,\nthe RING is earned through contact — trade, borders, ruled peoples.\nA culture you assimilate is knowledge you dry up.\nChoose what you merge and what you keep distinct.") \
    X(STR_TUTO_PAGE_6, "Some paths are more than powerful — they are HUNGRY.\nEvery dark pact, every forbidden forge, every imposed cult CHARGES the world.\nThe Breach forbids nothing: it waits. Your empire will fall — they all do.\nThe only question is HOW, and what you will leave standing.") \
    X(STR_MENU_SOUS_TITRE, "a world that won't wait for you — and can be read") \
    X(STR_MENU_JOUER, "Play") \
    X(STR_MENU_CHARGER, "Load") \
    X(STR_MENU_TUTORIEL, "Tutorial") \
    X(STR_MENU_QUITTER, "Quit") \
    X(STR_MENU_LANGUE, "Language: {0}") \
    X(STR_SETUP_TITRE, "FORGE A WORLD") \
    X(STR_PAUSE_TITRE, "PAUSE") \
    X(STR_PM_REPRENDRE, "Resume") \
    X(STR_PM_SAUVER, "Save") \
    X(STR_PM_TUTORIEL, "Tutorial") \
    X(STR_PM_MENU, "Main Menu") \
    X(STR_PM_QUITTER, "Quit") \
    X(STR_PICK_SAUVER, "SAVE — choose a slot") \
    X(STR_PICK_CHARGER, "LOAD — choose a slot") \
    X(STR_SLOT_LINE, "Slot {0} — {1}") \
    X(STR_SLOT_ANCIEN, "Slot {0} — a save from an earlier era") \
    X(STR_SLOT_VIDE, "Slot {0} — empty") \
    X(STR_TUTO_PREC, "◀ prev.") \
    X(STR_TUTO_SUIV, "next ▶") \
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
    X(STR_MARCHE_PRIX_FMT, "current price {0} crowns per unit") \
    X(STR_MARCHE_PRIX_HOV, "The domestic market price: demand pulls it up, supply and selling ease it. Buying and selling happen at THIS price.") \
    X(STR_MARCHE_ROW_HOV, "{0} — in reserve {1}. Buy or sell in lots of 10 at the current price (crowns flow through the treasury).") \
    X(STR_MARCHE_HDR_LOCAL,  "good            stock     ref.price") \
    X(STR_MARCHE_HDR_MARCHE, "good          price   avail") \
    X(STR_MARCHE_BUY_HOV,    "Buy (pumps the treasury) \xe2\x80\x94 step 10, Shift = 100.") \
    X(STR_MARCHE_SELL_HOV,   "Sell to the market \xe2\x80\x94 step 10, Shift = 100.") \
    X(STR_SLOT_VERROU_FMT, "{0} — locked ({1})") \
    X(STR_BTN_COMPTOIR_FMT, "Build a Trading Post here  ({0} crowns)") \
    X(STR_COMPTOIR_HOV, "The Trading Post links the province to the nearest Trade Hub: the transport margin drops by a third at this end of merchant routes.") \
    X(STR_BTN_CENTER_FMT, "Build a Trade Centre  ({0} crowns)") \
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
    X(STR_COUNCIL_EFF_2, "manufacturing") \
    X(STR_COUNCIL_VACANT, "— seat vacant —") \
    X(STR_COUNCIL_NOMMER, "Appoint") \
    X(STR_COUNCIL_RENVOYER, "Dismiss") \
    X(STR_COUNCIL_SEAT_FMT, "{0} — +{1}% {2}") \
    X(STR_COUNCIL_SEATED_FMT, "{0} · tier {1} · {2} crowns/mo") \
    X(STR_COUNCIL_CAND_FMT, "{0} · tier {1} · {2} crowns") \
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
    X(STR_PMOD_MUTATION_NOM,   "Mutations") \
    X(STR_PMOD_MUTATION_EFF,   "The Replicator transmutes flux into wood at great yield — but something else transmutes too: bodies adapt, multiply faster. A blessing that is not without a price.") \
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
    /* ETHOS-SIGNATURE MANUFACTURES (cross-desire, docs/DESIGN_manufactures_ethos.md) —
     * 6 goods + 6 workshops, one per ethos. */ \
    X(STR_RES_HEAUMES,       "War Helms") \
    X(STR_RES_PARURES,       "Adornments of Glory") \
    X(STR_RES_HORLOGES,      "Tuned Clocks") \
    X(STR_RES_REGISTRES,     "Sealed Ledgers") \
    X(STR_RES_COLIFICHETS,   "Exotic Trinkets") \
    X(STR_RES_OUVRAGES,      "Leisure Works") \
    X(STR_BLD_HEAUMERIE,         "Helm Forge") \
    X(STR_BLD_PARURIER,          "Adornment Workshop") \
    X(STR_BLD_HORLOGER,          "Clockmaker's Workshop") \
    X(STR_BLD_CHANCELLERIE_LUX,  "Fine Chancery") \
    X(STR_BLD_COMPTOIR_ARTISAN,  "Artisan Trading Post") \
    X(STR_BLD_ATELIER_SEREIN,    "Serene Workshop") \
    X(STR_CULTURE_PARENTS, "Parents: ") \
    X(STR_CULTURE_RACINES, "Roots: ") \
    X(STR_CULTURE_SUBSTRAT,"Substrate: ") \
    X(STR_CULTURE_PARENTE, "Ancestry") \
    X(STR_LOAN_AUCUNE,  "No request") \
    X(STR_LOAN_ACCORDE, "The state grants the loan") \
    X(STR_LOAN_REFUSE,  "The state refuses the loan") \
    /* VAGUE STR_* (2026-08-15) — twin of the FR block, same order. */ \
    X(STR_MARCH_REASON_DEFAULT, "Preview unavailable") \
    X(STR_MARCH_REASON_0, "Route passable") \
    X(STR_MARCH_REASON_1, "Invalid corps") \
    X(STR_MARCH_REASON_2, "Corps engaged in battle") \
    X(STR_MARCH_REASON_3, "Corps at sea or landing") \
    X(STR_MARCH_REASON_4, "Corps broken and routed") \
    X(STR_MARCH_REASON_5, "Invalid destination") \
    X(STR_MARCH_REASON_6, "Corps with no strength") \
    X(STR_MARCH_REASON_7, "No land route") \
    X(STR_MARCH_ARRIVAL_0, "Stay in place") \
    X(STR_MARCH_ARRIVAL_1, "Repositioning") \
    X(STR_MARCH_ARRIVAL_2, "March toward a siege") \
    X(STR_REFILL_REASON_INVALID,      "Invalid corps") \
    X(STR_REFILL_REASON_NOT_NATIONAL, "Resupply is only possible on a national region") \
    X(STR_REFILL_REASON_NO_LINES,     "No unit line to reinforce") \
    X(STR_REFILL_REASON_FULL,         "Corps already at full strength (nominal reached)") \
    X(STR_REFILL_REASON_NO_POP,       "No population of the right class is mobilisable") \
    X(STR_REFILL_REASON_COVERED,      "Reinforcement covered by population and the national arsenal") \
    X(STR_REFILL_REASON_PARTIAL,      "Partial reinforcement guaranteed; the market can supply the missing weapons") \
    X(STR_BATTLE_STAGE_CHOC,     "Clash") \
    X(STR_BATTLE_STAGE_ACCALMIE, "Lull") \
    X(STR_FOI_SANS, "Faithless") \
    X(STR_FUSION_AUCUN_CONTACT,   "No sustained trade contact") \
    X(STR_FUSION_PIVOT_TRANSFORME,"Trade contact runs through the region's hub province instead") \
    X(STR_FUSION_NON_SEDENTARISE, "Local culture not yet settled") \
    X(STR_FUSION_PORTE_OUVERTE,   "Merger gateway open") \
    X(STR_FUSION_PORTE_FERMEE,    "Gateway closed: insufficient contact or institutions") \
    X(STR_TRADE_STATUT_GUERRE,     "war") \
    X(STR_TRADE_STATUT_EMBARGO,    "embargo") \
    X(STR_TRADE_STATUT_FLORISSANT, "flourishing") \
    X(STR_TRADE_STATUT_MODESTE,    "modest") \
    X(STR_CONS_NOM_0, "Rigorist") \
    X(STR_CONS_NOM_1, "Courtier") \
    X(STR_CONS_NOM_2, "Austere") \
    X(STR_CONS_NOM_3, "Reformer") \
    X(STR_CONS_NOM_4, "Veteran") \
    X(STR_CONS_NOM_5, "Ambitious") \
    X(STR_CONS_NOM_6, "Loyalist") \
    X(STR_CONS_NOM_7, "Venal") \
    X(STR_CONS_FLAVOR_0, "To him, every exception looks like the first stone of a ruin.") \
    X(STR_CONS_FLAVOR_1, "He knows who must be greeted, who must be paid, and who must believe the two gestures are worth the same.") \
    X(STR_CONS_FLAVOR_2, "His whole household fits in two chests. So does his gratitude.") \
    X(STR_CONS_FLAVOR_3, "No institution seems finished to him while it can still be taken apart.") \
    X(STR_CONS_FLAVOR_4, "He has served three reigns and learned never to mistake any of them for the State.") \
    X(STR_CONS_FLAVOR_5, "He calls it service — the distance still keeping him from power.") \
    X(STR_CONS_FLAVOR_6, "He serves the crown with enough fervour to worry the one who wears it.") \
    X(STR_CONS_FLAVOR_7, "He knows the price of every secret except the last one.") \
    X(STR_RELATION_GUERRE, "War") \
    X(STR_DIPLO_TERRITOIRE_INCONNU, "unknown territory") \
    X(STR_GATE_EMISSAIRE_DISPO,      "Envoy available") \
    X(STR_GATE_PAS_DEJA_GUERRE,      "Not already at war") \
    X(STR_GATE_AUCUNE_TREVE,         "No truce") \
    X(STR_GATE_CASUS_BELLI,          "Usable casus belli") \
    X(STR_GATE_EN_GUERRE_CIBLE,      "At war with the target") \
    X(STR_GATE_PAS_GUERRE,           "Not at war") \
    X(STR_GATE_PAS_ALLIES,           "Not already allied") \
    X(STR_GATE_CRENEAU_ALLIANCE,     "Free alliance slot") \
    X(STR_GATE_PAS_PACTE_COMMERCIAL, "No trade pact in force") \
    X(STR_GATE_PAS_PACTE_MIGRATOIRE, "No migration pact in force") \
    X(STR_GATE_RELATION_COMMERCABLE, "Tradeable relation") \
    X(STR_GATE_OR_SUFFISANT,         "Sufficient crowns") \
    X(STR_GATE_AUCUNE_INTRIGUE,      "No plot already underway") \
    X(STR_GATE_INFLUENCE_SUFFISANTE, "Sufficient political influence") \
    X(STR_DIPLO_REASON_INVALID_TARGET,        "Invalid or unknown diplomatic target") \
    X(STR_DIPLO_REASON_EMISSARY_BUSY,         "Envoy on tour") \
    X(STR_DIPLO_REASON_OK,                    "Action available") \
    X(STR_DIPLO_REASON_ALREADY_WAR,           "Already at war with this country") \
    X(STR_DIPLO_REASON_TRUCE_ACTIVE,          "Truce in force") \
    X(STR_DIPLO_REASON_NO_CB,                 "No usable casus belli") \
    X(STR_DIPLO_REASON_NOT_AT_WAR,            "You are not at war with this country") \
    X(STR_DIPLO_REASON_AT_WAR,                "Impossible during war") \
    X(STR_DIPLO_REASON_ALREADY_ALLIED,        "Alliance already sealed") \
    X(STR_DIPLO_REASON_NO_ALLIANCE_SLOT,      "No free alliance slot") \
    X(STR_DIPLO_REASON_PACT_EXISTS,           "Trade pact already sealed") \
    X(STR_DIPLO_REASON_MIGRATION_PACT_EXISTS, "Migration pact already sealed") \
    X(STR_DIPLO_REASON_EMBARGO_UNAVAILABLE,   "Embargo unavailable") \
    X(STR_DIPLO_REASON_INTRIGUE_IN_PROGRESS,  "Claim being fabricated") \
    X(STR_DIPLO_REASON_CLAIM_READY,           "A claim is already ready") \
    X(STR_DIPLO_REASON_INSUFFICIENT_GOLD,     "Not enough crowns to fund the plot") \
    X(STR_DIPLO_REASON_INSUFFICIENT_INFLUENCE,"Not enough political influence for this act") \
    X(STR_DIPLO_REASON_FABRICATION_UNAVAILABLE,"Plot unavailable for now") \
    X(STR_INFLUENCE_HOVER,      "%d nobles · %d bourgeois · %d laborers × the Council (average rank %s)") \
    X(STR_INFLUENCE_HOVER_VIDE, "%d nobles · %d bourgeois · %d laborers × the Council (no minister seated — floor)") \
    X(STR_INFLUENCE_RANK_I,     "I") \
    X(STR_INFLUENCE_RANK_II,    "II") \
    X(STR_INFLUENCE_RANK_III,   "III") \
    X(STR_INFLUENCE_COURANT_ARISTO,    " — Aristocracy raises the nobles") \
    X(STR_INFLUENCE_COURANT_BOURGEOIS, " — Bourgeoisie raises the bourgeois") \
    X(STR_INFLUENCE_COURANT_LABORER,   " — Popular raises the laborers") \
    X(STR_INFLUENCE_COURANT_DIVIN,     " · %d believers") \
    X(STR_HERITAGE_FLAVOR_0, "Their genealogies begin before the first calendars, in centuries only ruins still remember.") \
    X(STR_HERITAGE_FLAVOR_1, "They say every oath is like a metal: it reveals its worth only once heated enough to break it.") \
    X(STR_HERITAGE_FLAVOR_2, "Their first clock measured the seasons. The second measured labour. The third taught the two to yield gold.") \
    X(STR_HERITAGE_FLAVOR_3, "They have worn so many laws, tongues and crowns that they now call tradition the art of changing without disappearing.") \
    X(STR_HERITAGE_FLAVOR_4, "Their borders follow the canals, their festivals the harvests, and their memories the fields their ancestors refused to abandon.") \
    X(STR_HERITAGE_FLAVOR_5, "A stranger once asked them where family ended. They were shown the graves, the herds, the warriors, and finally the horizon.") \
    X(STR_ETHOS_EPITHETE_0, "Horde") \
    X(STR_ETHOS_EPITHETE_1, "Clans") \
    X(STR_ETHOS_EPITHETE_2, "Order") \
    X(STR_ETHOS_EPITHETE_3, "Crown") \
    X(STR_ETHOS_EPITHETE_4, "League") \
    X(STR_ETHOS_EPITHETE_5, "Haven") \
    X(STR_ETHOS_HINT_0, "Conquest: pushes coercion, a poor integrator.") \
    X(STR_ETHOS_HINT_1, "Glory & raiding: martial honour, digests poorly.") \
    X(STR_ETHOS_HINT_2, "Hierarchy & discipline: the State that holds order.") \
    X(STR_ETHOS_HINT_3, "Institution-builder: holds diversity together.") \
    X(STR_ETHOS_HINT_4, "Profit & crossroads: thrives on trade.") \
    X(STR_ETHOS_HINT_5, "Consent alone: never fractures, peaceful.") \
    X(STR_ETHOS_FLAVOR_0, "They do not ask whether a border can be crossed, only how many men it will take for it to stop existing.") \
    X(STR_ETHOS_FLAVOR_1, "A debt can be forgotten, a defeat repaired. A shame, though, waits patiently for the grandsons.") \
    X(STR_ETHOS_FLAVOR_2, "Every person knows their place, every place its duty, and every duty the seal that makes it beyond dispute.") \
    X(STR_ETHOS_FLAVOR_3, "The realm does not rest on one man's will, but on a thousand ledgers that stubbornly refuse to contradict each other.") \
    X(STR_ETHOS_FLAVOR_4, "They do not conquer harbours. They lend them gold until the keys become a form of repayment.") \
    X(STR_ETHOS_FLAVOR_5, "They swore to take no life. Their neighbours still debate whether that promise is a virtue or an invitation.") \
    X(STR_LEVIER_NOM_0, "Population growth") \
    X(STR_LEVIER_NOM_1, "Production") \
    X(STR_LEVIER_NOM_2, "Diplomatic influence") \
    X(STR_LEVIER_NOM_3, "Coercion") \
    X(STR_LEVIER_NOM_4, "State capacity") \
    X(STR_LEVIER_NOM_5, "Minority assimilation") \
    X(STR_LEVIER_NOM_6, "Faustian magic") \
    X(STR_LEVIER_NOM_7, "Cultural drift") \
    X(STR_LEVIER_NOM_8, "Fracture") \
    X(STR_SYNC_CHEMIN_ACQUIS,  "attained — spread by contact, and kept even if the source has merged away") \
    X(STR_SYNC_CHEMIN_JAMAIS,  "tradition never encountered — you must make contact with its bearers") \
    X(STR_SYNC_CHEMIN_SURFACE, "surface knowledge: a trading post does not pass on deep mastery — rule or neighbour this culture, and legitimise the land") \
    X(STR_SYNC_CHEMIN_PORTEE,  "within reach — the foundation is missing (research the ring's parent node)") \
    X(STR_AUGURE_SECESSION,           "The borderlands speak of governing themselves.") \
    X(STR_AUGURE_REVOLTE,             "The street rumbles against the throne.") \
    X(STR_AUGURE_COERCITION_FRAGILE,  "Order holds — but by fear alone.") \
    X(STR_VOC_GRENIER,    "Granary") \
    X(STR_VOC_PATURES,    "Pastures") \
    X(STR_VOC_PECHERIES,  "Fisheries") \
    X(STR_VOC_MINE,       "Mine") \
    X(STR_VOC_ATELIER,    "Workshop") \
    X(STR_VOC_COMPTOIR,   "Trading Post") \
    X(STR_VOC_SANCTUAIRE, "Shrine") \
    X(STR_VOC_MARCHE,     "Market") \
    X(STR_MODE_DEFAUT,    "Map: terrain and borders") \
    X(STR_MODE_POLITIQUE, "Map: territories by country") \
    X(STR_MODE_NATURE,    "Map: terrain only, no borders") \
    X(STR_MODE_MARCHE,    "Map: trade catchments") \
    X(STR_MARCHE_DE,      "Market of") \
    X(STR_MARCHE_AUCUN,   "No market reachable") \
    X(STR_MODE_RELIGION,  "Map: dominant faith") \
    X(STR_MODE_CULTURE,   "Map: dominant culture") \
    /* ── THE DESIGNS — the Soil branch (mirror of strings_ids.h : four parallel
     * bands of 12 slots, name / objective / reward / flavour). */ \
    X(STR_DESS_SOL,        "The Soil") \
    X(STR_DESS_SOL_VOIE_A, "Conquest") \
    X(STR_DESS_SOL_VOIE_B, "Vassalage") \
    X(STR_DESS_ATTENTE,    "no land within reach") \
    X(STR_DESS_SOL_N0,  "Unification") \
    X(STR_DESS_SOL_N1,  "Expansion") \
    X(STR_DESS_SOL_N2,  "The Rival") \
    X(STR_DESS_SOL_N3,  "The Choice of Soil") \
    X(STR_DESS_SOL_N4,  "Pacification") \
    X(STR_DESS_SOL_N5,  "The Marches") \
    X(STR_DESS_SOL_N6,  "Rival Capital") \
    X(STR_DESS_SOL_N7,  "Hegemony") \
    X(STR_DESS_SOL_N8,  "First Vassal") \
    X(STR_DESS_SOL_N9,  "Three Vassals") \
    X(STR_DESS_SOL_N10, "Integration") \
    X(STR_DESS_SOL_N11, "Hegemony") \
    X(STR_DESS_SOL_O0,  "Hold and settle the whole valley of {0}") \
    X(STR_DESS_SOL_O1,  "Take {0}, by the sword or by the plough") \
    X(STR_DESS_SOL_O2,  "Wrest {0} from the rival, or bring him to his knee") \
    X(STR_DESS_SOL_O3,  "Choose how the soil is held: the sword or the oath") \
    X(STR_DESS_SOL_O4,  "Close the wound of {0}") \
    X(STR_DESS_SOL_O5,  "Hold the three marches, starting with {0}") \
    X(STR_DESS_SOL_O6,  "Own {0}, the rival's capital") \
    X(STR_DESS_SOL_O7,  "Hold two fifths of the continent") \
    X(STR_DESS_SOL_O8,  "Make {0} a vassal") \
    X(STR_DESS_SOL_O9,  "Bind three crowns, starting with {0}") \
    X(STR_DESS_SOL_O10, "See {0} become indistinguishable from us") \
    X(STR_DESS_SOL_O11, "Hold or be sworn two fifths of the continent") \
    X(STR_DESS_SOL_R0,  "Stronger institutions in the capital") \
    X(STR_DESS_SOL_R1,  "A claim on the next march, and rebuilding") \
    X(STR_DESS_SOL_R2,  "Grievances kept: claims stay valid 8 years, not 5, for 20 years") \
    X(STR_DESS_SOL_R3,  "The choice IS the reward — and it cannot be taken back") \
    X(STR_DESS_SOL_R4,  "Gentler annexations for 20 years, and rebuilding") \
    X(STR_DESS_SOL_R5,  "A claim on the rival capital, faster digestion for 20 years") \
    X(STR_DESS_SOL_R6,  "The best minister of the Realm joins the Council, free") \
    X(STR_DESS_SOL_R7,  "The Gate of Soil: institutions and garrison built in the capital") \
    X(STR_DESS_SOL_R8,  "The credit of the oath: vassalage better regarded for 20 years") \
    X(STR_DESS_SOL_R9,  "Vassals ready to absorb sooner, for 20 years") \
    X(STR_DESS_SOL_R10, "The vassal's heir joins the Council, free") \
    X(STR_DESS_SOL_R11, "The Chamber of Oaths: institutions built in the capital") \
    X(STR_DESS_SOL_F0,  "The hamlets are counted no more: every hearth answers one ban.") \
    X(STR_DESS_SOL_F1,  "The boundary stone moved one notch. It is the first time.") \
    X(STR_DESS_SOL_F2,  "He has a name now, in a ledger that no longer closes.") \
    X(STR_DESS_SOL_F3,  "The soil is held by the sword, or by the oath. Never both.") \
    X(STR_DESS_SOL_F4,  "Taking takes a summer; being forgiven takes a reign.") \
    X(STR_DESS_SOL_F5,  "One march is an accident; three marches are a border.") \
    X(STR_DESS_SOL_F6,  "His crown is in your chapel and his chancellor at your table.") \
    X(STR_DESS_SOL_F7,  "Realm and continent are no longer two words.") \
    X(STR_DESS_SOL_F8,  "One man knelt, and three others watched.") \
    X(STR_DESS_SOL_F9,  "Three taut threads are not three bonds: they are a surface.") \
    X(STR_DESS_SOL_F10, "His son speaks our tongue without an accent.") \
    X(STR_DESS_SOL_F11, "You do not own the continent. It answers you, and that is cheaper.") \
    /* LES DOCTRINES — la table jumelle (même ordre). */ \
    X(STR_DOCT_OFFENSE_NAME,  "Offense") \
    X(STR_DOCT_OFFENSE_HOVER, "Steel first: arms, discipline, loot and pretexts. The doctrine of those who strike first.") \
    X(STR_IDEA_OFFENSE_ARSENAUX_NAME,  "Arsenals") \
    X(STR_IDEA_OFFENSE_ARSENAUX_BONUS, "+25% arms produced, rust halved") \
    X(STR_IDEA_OFFENSE_DISCIPLINE_NAME,  "Discipline") \
    X(STR_IDEA_OFFENSE_DISCIPLINE_BONUS, "+10% damage in battle") \
    X(STR_IDEA_OFFENSE_OST_NAME,  "Standing Host") \
    X(STR_IDEA_OFFENSE_OST_BONUS, "Armies refill on their own, war pay borne in peacetime") \
    X(STR_IDEA_OFFENSE_BUTIN_NAME,  "Plunder") \
    X(STR_IDEA_OFFENSE_BUTIN_BONUS, "+30% siege loot, +15% when sacking") \
    X(STR_IDEA_OFFENSE_PRETEXTES_NAME,  "Pretexts") \
    X(STR_IDEA_OFFENSE_PRETEXTES_BONUS, "−40% claim cost, −30% maturing time") \
    X(STR_IDEA_OFFENSE_LEVEE_NAME,  "Great Levy") \
    X(STR_IDEA_OFFENSE_LEVEE_BONUS, "+30% force limit") \
    X(STR_DOCT_DEFENSE_NAME,  "Defence") \
    X(STR_DOCT_DEFENSE_HOVER, "You do not win the war: you make them lose it. Ramparts, stores, scorched earth.") \
    X(STR_IDEA_DEFENSE_REMPARTS_NAME,  "Ramparts") \
    X(STR_IDEA_DEFENSE_REMPARTS_BONUS, "+30% fortress defence") \
    X(STR_IDEA_DEFENSE_MAGASINS_NAME,  "Stores") \
    X(STR_IDEA_DEFENSE_MAGASINS_BONUS, "+25% siege victuals") \
    X(STR_IDEA_DEFENSE_BAN_NAME,  "The Ban") \
    X(STR_IDEA_DEFENSE_BAN_BONUS, "Militia raised at once in an invaded province") \
    X(STR_IDEA_DEFENSE_CORVEES_NAME,  "Corvées") \
    X(STR_IDEA_DEFENSE_CORVEES_BONUS, "−20% fortification cost") \
    X(STR_IDEA_DEFENSE_TERRE_BRULEE_NAME,  "Scorched Earth") \
    X(STR_IDEA_DEFENSE_TERRE_BRULEE_BONUS, "−40% loot taken by the invader") \
    X(STR_IDEA_DEFENSE_GENIE_NAME,  "Engineers") \
    X(STR_IDEA_DEFENSE_GENIE_BONUS, "Fortifications one tech step ahead") \
    X(STR_DOCT_COMMERCE_NAME,  "Trade") \
    X(STR_DOCT_COMMERCE_HOVER, "What flows freely enriches more than what is locked away. Excludes Mercantilism.") \
    X(STR_IDEA_COMMERCE_FRANCHISES_NAME,  "Franchises") \
    X(STR_IDEA_COMMERCE_FRANCHISES_BONUS, "−25% customs duties") \
    X(STR_IDEA_COMMERCE_ROUTES_LONGUES_NAME,  "Long Roads") \
    X(STR_IDEA_COMMERCE_ROUTES_LONGUES_BONUS, "+30% market reach") \
    X(STR_IDEA_COMMERCE_COMPTOIR_NAME,  "Trading Post") \
    X(STR_IDEA_COMMERCE_COMPTOIR_BONUS, "Found a post in a city-state, tolls shared") \
    X(STR_IDEA_COMMERCE_NEGOCE_NAME,  "Brokerage") \
    X(STR_IDEA_COMMERCE_NEGOCE_BONUS, "−15% import margin with third parties") \
    X(STR_IDEA_COMMERCE_GUILDES_NAME,  "Merchant Guilds") \
    X(STR_IDEA_COMMERCE_GUILDES_BONUS, "+30% burgher trade volume") \
    X(STR_IDEA_COMMERCE_LIBRE_ECHANGE_NAME,  "Free Trade") \
    X(STR_IDEA_COMMERCE_LIBRE_ECHANGE_BONUS, "Immune to embargoes — and may declare none") \
    X(STR_DOCT_MERCANTILISME_NAME,  "Mercantilism") \
    X(STR_DOCT_MERCANTILISME_HOVER, "What comes in passes my staple, pays my due, or does not pass. Excludes Trade.") \
    X(STR_IDEA_MERCANTILISME_RESERVES_NAME,  "Reserves") \
    X(STR_IDEA_MERCANTILISME_RESERVES_BONUS, "+30% building reserves and state cushion") \
    X(STR_IDEA_MERCANTILISME_REGIE_NAME,  "State Board") \
    X(STR_IDEA_MERCANTILISME_REGIE_BONUS, "The state stockpiler buys and sells sooner") \
    X(STR_IDEA_MERCANTILISME_BLOCUS_NAME,  "Blockade") \
    X(STR_IDEA_MERCANTILISME_BLOCUS_BONUS, "My embargo cuts through pacts and shuts my Hubs") \
    X(STR_IDEA_MERCANTILISME_ETAPE_NAME,  "Staple") \
    X(STR_IDEA_MERCANTILISME_ETAPE_BONUS, "Name a staple province served first, barred from export") \
    X(STR_IDEA_MERCANTILISME_PEAGES_NAME,  "Tolls") \
    X(STR_IDEA_MERCANTILISME_PEAGES_BONUS, "Imports at par at home, 75% of tolls to the crown") \
    X(STR_IDEA_MERCANTILISME_HALLES_NAME,  "Halls") \
    X(STR_IDEA_MERCANTILISME_HALLES_BONUS, "+30% warehouse capacity, fewer stock losses") \
    X(STR_DOCT_PEUPLE_NAME,  "Peoples") \
    X(STR_DOCT_PEUPLE_HOVER, "The stranger becomes an arm, a trade, a technique. Welcome, integration, mixing.") \
    X(STR_IDEA_PEUPLE_ACCUEIL_NAME,  "Welcome") \
    X(STR_IDEA_PEUPLE_ACCUEIL_BONUS, "Migration pacts accepted more readily") \
    X(STR_IDEA_PEUPLE_ECOLES_NAME,  "Schools") \
    X(STR_IDEA_PEUPLE_ECOLES_BONUS, "+6%/month integration") \
    X(STR_IDEA_PEUPLE_ASILE_NAME,  "Asylum") \
    X(STR_IDEA_PEUPLE_ASILE_BONUS, "Refugees settle sooner and leave less") \
    X(STR_IDEA_PEUPLE_AFFRANCHISSEMENT_NAME,  "Manumission") \
    X(STR_IDEA_PEUPLE_AFFRANCHISSEMENT_BONUS, "Under pact, the deported become migrants") \
    X(STR_IDEA_PEUPLE_TOLERANCE_NAME,  "Tolerance") \
    X(STR_IDEA_PEUPLE_TOLERANCE_BONUS, "−25% friction from foreign cultures") \
    X(STR_IDEA_PEUPLE_METISSAGE_NAME,  "Mixing") \
    X(STR_IDEA_PEUPLE_METISSAGE_BONUS, "Foreign heritages reached sooner, +20% crucible research") \
    X(STR_DOCT_COLONISATION_NAME,  "Colonisation") \
    X(STR_DOCT_COLONISATION_HOVER, "Set out with less, hold on harsh ground. Settlers, victuals, climates.") \
    X(STR_IDEA_COLONISATION_COLONS_NAME,  "Settlers") \
    X(STR_IDEA_COLONISATION_COLONS_BONUS, "−15% population required per colony") \
    X(STR_IDEA_COLONISATION_RAVITAILLEMENT_NAME,  "Supply") \
    X(STR_IDEA_COLONISATION_RAVITAILLEMENT_BONUS, "−25% food reserve required") \
    X(STR_IDEA_COLONISATION_ACCLIMATATION_NAME,  "Acclimation") \
    X(STR_IDEA_COLONISATION_ACCLIMATATION_BONUS, "−20% harsh-ground penalty") \
    X(STR_IDEA_COLONISATION_DOUBLE_CHANTIER_NAME,  "Twin Works") \
    X(STR_IDEA_COLONISATION_DOUBLE_CHANTIER_BONUS, "Two colonial works at once") \
    X(STR_IDEA_COLONISATION_CLIMATS_NAME,  "Climates") \
    X(STR_IDEA_COLONISATION_CLIMATS_BONUS, "Climates learned 10% sooner") \
    X(STR_IDEA_COLONISATION_GRAND_LARGE_NAME,  "Open Sea") \
    X(STR_IDEA_COLONISATION_GRAND_LARGE_BONUS, "+50% yield from distant colonies") \
    X(STR_DOCT_DIPLOMATIE_NAME,  "Diplomacy") \
    X(STR_DOCT_DIPLOMATIE_HOVER, "Speak more often, faster, in two voices. Opinion, envoys, congresses.") \
    X(STR_IDEA_DIPLOMATIE_PRESTIGE_NAME,  "Prestige") \
    X(STR_IDEA_DIPLOMATIE_PRESTIGE_BONUS, "+25% opinion from allies and partners") \
    X(STR_IDEA_DIPLOMATIE_CHANCELLERIE_NAME,  "Chancery") \
    X(STR_IDEA_DIPLOMATIE_CHANCELLERIE_BONUS, "−20% influence cost for envoys") \
    X(STR_IDEA_DIPLOMATIE_OUBLI_NAME,  "Oblivion") \
    X(STR_IDEA_DIPLOMATIE_OUBLI_BONUS, "Your grievances fade 30% faster") \
    X(STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_NAME,  "Second Envoy") \
    X(STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_BONUS, "Two diplomatic actions at once") \
    X(STR_IDEA_DIPLOMATIE_PERSUASION_NAME,  "Persuasion") \
    X(STR_IDEA_DIPLOMATIE_PERSUASION_BONUS, "Your offers accepted more readily") \
    X(STR_IDEA_DIPLOMATIE_CONGRES_NAME,  "Congress") \
    X(STR_IDEA_DIPLOMATIE_CONGRES_BONUS, "Wars end sooner — yours as well") \
    X(STR_DOCT_VASSAUX_NAME,  "Vassals") \
    X(STR_DOCT_VASSAUX_HOVER, "Make them swear, make them pay, let them ripen. Oaths, tribute, annexation.") \
    X(STR_IDEA_VASSAUX_SERMENTS_NAME,  "Oaths") \
    X(STR_IDEA_VASSAUX_SERMENTS_BONUS, "Vassals integrated 15% faster") \
    X(STR_IDEA_VASSAUX_TRIBUT_VASSAL_NAME,  "Vassal Tribute") \
    X(STR_IDEA_VASSAUX_TRIBUT_VASSAL_BONUS, "Contribution sooner and +20%") \
    X(STR_IDEA_VASSAUX_CONTRATS_NAME,  "Contracts") \
    X(STR_IDEA_VASSAUX_CONTRATS_BONUS, "Choose the vassalage contract at the peace") \
    X(STR_IDEA_VASSAUX_LEVIERS_NAME,  "Levers") \
    X(STR_IDEA_VASSAUX_LEVIERS_BONUS, "Gift, relief, division, intimidation of vassals") \
    X(STR_IDEA_VASSAUX_ANNEXION_NAME,  "Annexation") \
    X(STR_IDEA_VASSAUX_ANNEXION_BONUS, "May annex your vassals, −25% duration") \
    X(STR_IDEA_VASSAUX_SUZERAINETE_NAME,  "Suzerainty") \
    X(STR_IDEA_VASSAUX_SUZERAINETE_BONUS, "Offer vassalage in full peace") \
    X(STR_DOCT_PRODUCTION_NAME,  "Production") \
    X(STR_DOCT_PRODUCTION_HOVER, "Dig deeper, equip more hands. Tooling, manufactories, tiers.") \
    X(STR_IDEA_PRODUCTION_EXTRACTION_NAME,  "Extraction") \
    X(STR_IDEA_PRODUCTION_EXTRACTION_BONUS, "+12% hands at extraction") \
    X(STR_IDEA_PRODUCTION_OUTILLAGE_NAME,  "Tooling") \
    X(STR_IDEA_PRODUCTION_OUTILLAGE_BONUS, "+30% tools per worker") \
    X(STR_IDEA_PRODUCTION_EXPLOITATION_NAME,  "Deep Working") \
    X(STR_IDEA_PRODUCTION_EXPLOITATION_BONUS, "Working tiers up to 12 instead of 8") \
    X(STR_IDEA_PRODUCTION_MANUFACTURES_NAME,  "Manufactories") \
    X(STR_IDEA_PRODUCTION_MANUFACTURES_BONUS, "+15% manufactory capacity") \
    X(STR_IDEA_PRODUCTION_GAGES_NAME,  "Wages") \
    X(STR_IDEA_PRODUCTION_GAGES_BONUS, "−15% manufactory cost, −20% state wages") \
    X(STR_IDEA_PRODUCTION_RENDEMENT_NAME,  "Yield") \
    X(STR_IDEA_PRODUCTION_RENDEMENT_BONUS, "+6% extraction per tier, tiers −25%") \
    X(STR_DOCT_INFRASTRUCTURE_NAME,  "Infrastructure") \
    X(STR_DOCT_INFRASTRUCTURE_HOVER, "Stone once laid never returns to rubble. Cheaper works, buildings that last.") \
    X(STR_IDEA_INFRASTRUCTURE_MACONS_NAME,  "Masons") \
    X(STR_IDEA_INFRASTRUCTURE_MACONS_BONUS, "−10% materials per building site") \
    X(STR_IDEA_INFRASTRUCTURE_CARRIERES_NAME,  "Quarries") \
    X(STR_IDEA_INFRASTRUCTURE_CARRIERES_BONUS, "+30% construction reserves") \
    X(STR_IDEA_INFRASTRUCTURE_ENTRETIEN_NAME,  "Upkeep") \
    X(STR_IDEA_INFRASTRUCTURE_ENTRETIEN_BONUS, "Building wear −25%") \
    X(STR_IDEA_INFRASTRUCTURE_RENOVATION_NAME,  "Mass Renovation") \
    X(STR_IDEA_INFRASTRUCTURE_RENOVATION_BONUS, "National renovation queue, −20% cost") \
    X(STR_IDEA_INFRASTRUCTURE_LOGEMENTS_NAME,  "Housing") \
    X(STR_IDEA_INFRASTRUCTURE_LOGEMENTS_BONUS, "+25% housing per manufactory") \
    X(STR_IDEA_INFRASTRUCTURE_INTENDANCE_NAME,  "Stewardship") \
    X(STR_IDEA_INFRASTRUCTURE_INTENDANCE_BONUS, "−30% extent surcharge") \
    X(STR_DOCT_TECHNOLOGIE_NAME,  "Technology") \
    X(STR_DOCT_TECHNOLOGIE_HOVER, "Research becomes a policy. Libraries, schools, copyists.") \
    X(STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_NAME,  "Libraries") \
    X(STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_BONUS, "+25% bonus from the Library chain") \
    X(STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_NAME,  "Town Schools") \
    X(STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_BONUS, "+30% burgher research, +25% labourer") \
    X(STR_IDEA_TECHNOLOGIE_PROGRAMME_NAME,  "Programme") \
    X(STR_IDEA_TECHNOLOGIE_PROGRAMME_BONUS, "Steer research: −20% on one chosen quarter") \
    X(STR_IDEA_TECHNOLOGIE_COPISTES_NAME,  "Copyists") \
    X(STR_IDEA_TECHNOLOGIE_COPISTES_BONUS, "Widespread techs up to −52%") \
    X(STR_IDEA_TECHNOLOGIE_DISPENSE_NAME,  "Dispensation") \
    X(STR_IDEA_TECHNOLOGIE_DISPENSE_BONUS, "Two age steps ahead, faustian nodes excluded") \
    X(STR_IDEA_TECHNOLOGIE_SOBRIETE_NAME,  "Sobriety") \
    X(STR_IDEA_TECHNOLOGIE_SOBRIETE_BONUS, "−10% clean techs, +50% faustian techs") \
    X(STR_DOCT_CONNAISSANCES_NAME,  "World Knowledge") \
    X(STR_DOCT_CONNAISSANCES_HOVER, "Know the world before taking it. Coasts revealed, contacts, expeditions.") \
    X(STR_IDEA_CONNAISSANCES_PORTULANS_NAME,  "Portolans") \
    X(STR_IDEA_CONNAISSANCES_PORTULANS_BONUS, "Twice the coastline revealed around the known") \
    X(STR_IDEA_CONNAISSANCES_TRUCHEMENTS_NAME,  "Interpreters") \
    X(STR_IDEA_CONNAISSANCES_TRUCHEMENTS_BONUS, "Cultural contacts 20% faster") \
    X(STR_IDEA_CONNAISSANCES_EXPEDITION_NAME,  "Expedition") \
    X(STR_IDEA_CONNAISSANCES_EXPEDITION_BONUS, "Reveal a distant land and open a contact") \
    X(STR_IDEA_CONNAISSANCES_DICTIONNAIRES_NAME,  "Dictionaries") \
    X(STR_IDEA_CONNAISSANCES_DICTIONNAIRES_BONUS, "Foreign heritages reached sooner") \
    X(STR_IDEA_CONNAISSANCES_COLLEGES_NAME,  "Colleges of Tongues") \
    X(STR_IDEA_CONNAISSANCES_COLLEGES_BONUS, "+40% research from absorbed peoples") \
    X(STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_NAME,  "Lingua Franca") \
    X(STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_BONUS, "Allies and routes share their maps") \
    X(STR_DOCT_FAUSTIEN_NAME,  "Faustian") \
    X(STR_DOCT_FAUSTIEN_HOVER, "Power now against slow damnation. Transmuters, mutations, charge.") \
    X(STR_IDEA_FAUSTIEN_PAGES_INTERDITES_NAME,  "Forbidden Pages") \
    X(STR_IDEA_FAUSTIEN_PAGES_INTERDITES_BONUS, "−15% cost of faustian techs") \
    X(STR_IDEA_FAUSTIEN_CREUSETS_NAME,  "Crucibles") \
    X(STR_IDEA_FAUSTIEN_CREUSETS_BONUS, "Alembic and Mage's Workshop unlocked") \
    X(STR_IDEA_FAUSTIEN_PACTE_NAME,  "The Pact") \
    X(STR_IDEA_FAUSTIEN_PACTE_BONUS, "All three transmuters unlocked, no refusal left") \
    X(STR_IDEA_FAUSTIEN_OR_DU_PUITS_NAME,  "Gold of the Well") \
    X(STR_IDEA_FAUSTIEN_OR_DU_PUITS_BONUS, "+30% gold from the Drill — the coin debases") \
    X(STR_IDEA_FAUSTIEN_TERRE_CHANGEE_NAME,  "Changed Earth") \
    X(STR_IDEA_FAUSTIEN_TERRE_CHANGEE_BONUS, "+25% mutations, charge washes off −35%") \
    X(STR_IDEA_FAUSTIEN_PRIX_CONSENTI_NAME,  "Price Consented") \
    X(STR_IDEA_FAUSTIEN_PRIX_CONSENTI_BONUS, "+25% machine output, +50% charge") \
    X(STR_DOCT_ARISTOCRATIE_NAME,  "Aristocracy") \
    X(STR_DOCT_ARISTOCRATIE_HOVER, "Influence springs from the elites. Fiefs, offices, knighting. One current at a time.") \
    X(STR_IDEA_ARISTOCRATIE_BANNERETS_NAME,  "Bannerets") \
    X(STR_IDEA_ARISTOCRATIE_BANNERETS_BONUS, "+25% vassal contribution") \
    X(STR_IDEA_ARISTOCRATIE_OFFICES_NAME,  "Offices") \
    X(STR_IDEA_ARISTOCRATIE_OFFICES_BONUS, "+30% loyalty bought, dismissal +50% grievance") \
    X(STR_IDEA_ARISTOCRATIE_ADOUBEMENT_NAME,  "Knighting") \
    X(STR_IDEA_ARISTOCRATIE_ADOUBEMENT_BONUS, "Raise burghers into the elite") \
    X(STR_IDEA_ARISTOCRATIE_FIEFS_NAME,  "Fiefs") \
    X(STR_IDEA_ARISTOCRATIE_FIEFS_BONUS, "+35% elite posts per building") \
    X(STR_IDEA_ARISTOCRATIE_BAN_FEODAL_NAME,  "Feudal Ban") \
    X(STR_IDEA_ARISTOCRATIE_BAN_FEODAL_BONUS, "+15% morale, −25% elite income tax") \
    X(STR_IDEA_ARISTOCRATIE_CLOTURE_NAME,  "Enclosure") \
    X(STR_IDEA_ARISTOCRATIE_CLOTURE_BONUS, "Nobility more open, burghers more closed") \
    X(STR_DOCT_BOURGEOISIE_NAME,  "Bourgeoisie") \
    X(STR_DOCT_BOURGEOISIE_HOVER, "Influence springs from the burghers. Charters, credit, guilds. One current at a time.") \
    X(STR_IDEA_BOURGEOISIE_CHARTES_NAME,  "Charters") \
    X(STR_IDEA_BOURGEOISIE_CHARTES_BONUS, "−15% administrative cost") \
    X(STR_IDEA_BOURGEOISIE_JURANDES_NAME,  "Sworn Guilds") \
    X(STR_IDEA_BOURGEOISIE_JURANDES_BONUS, "+20% trade volume, +10% accession") \
    X(STR_IDEA_BOURGEOISIE_EMPRUNT_NAME,  "Domestic Loan") \
    X(STR_IDEA_BOURGEOISIE_EMPRUNT_BONUS, "The burghers lend to the state") \
    X(STR_IDEA_BOURGEOISIE_CREDIT_NAME,  "Credit") \
    X(STR_IDEA_BOURGEOISIE_CREDIT_BONUS, "−20% interest rate") \
    X(STR_IDEA_BOURGEOISIE_ROBE_NAME,  "The Robe") \
    X(STR_IDEA_BOURGEOISIE_ROBE_BONUS, "One extra Council seat") \
    X(STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_NAME,  "Keys to the City") \
    X(STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_BONUS, "Burgher accession −25%") \
    X(STR_DOCT_POPULAIRE_NAME,  "Popular") \
    X(STR_DOCT_POPULAIRE_HOVER, "Influence springs from the day labourers. Bread, grievances, mass levy. One current at a time.") \
    X(STR_IDEA_POPULAIRE_DOLEANCES_NAME,  "Grievances") \
    X(STR_IDEA_POPULAIRE_DOLEANCES_BONUS, "Politics felt +20%, −15% agitation") \
    X(STR_IDEA_POPULAIRE_PAIN_NAME,  "Bread") \
    X(STR_IDEA_POPULAIRE_PAIN_BONUS, "Vital basket tax-free, content provinces grow faster") \
    X(STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_NAME,  "Mass Levy") \
    X(STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_BONUS, "Conscription beyond the limit for five years") \
    X(STR_IDEA_POPULAIRE_CONCESSION_NAME,  "Concession") \
    X(STR_IDEA_POPULAIRE_CONCESSION_BONUS, "Calm a province before revolt, −30% cost") \
    X(STR_IDEA_POPULAIRE_IMPOT_DU_RANG_NAME,  "Tax of Rank") \
    X(STR_IDEA_POPULAIRE_IMPOT_DU_RANG_BONUS, "+20% elite income tax, ranks closed") \
    X(STR_IDEA_POPULAIRE_SOUVERAINETE_NAME,  "Sovereignty") \
    X(STR_IDEA_POPULAIRE_SOUVERAINETE_BONUS, "Yielding costs neither legitimacy nor capacity") \
    X(STR_DOCT_DIVIN_NAME,  "Divine") \
    X(STR_DOCT_DIVIN_HOVER, "Influence springs from built faith. Anointing, fervour, priesthood. One current at a time.") \
    X(STR_IDEA_DIVIN_ONCTION_NAME,  "Anointing") \
    X(STR_IDEA_DIVIN_ONCTION_BONUS, "+25% legitimacy from faith") \
    X(STR_IDEA_DIVIN_FERVEUR_NAME,  "Fervour") \
    X(STR_IDEA_DIVIN_FERVEUR_BONUS, "+20% fervour, lasting five years longer") \
    X(STR_IDEA_DIVIN_SACERDOCE_NAME,  "Priesthood") \
    X(STR_IDEA_DIVIN_SACERDOCE_BONUS, "Missionary for every creed, foundation past the cap") \
    X(STR_IDEA_DIVIN_APPEL_NAME,  "Call of Faith") \
    X(STR_IDEA_DIVIN_APPEL_BONUS, "Rouse the fervour: concord or zealots") \
    X(STR_IDEA_DIVIN_CLERGE_NAME,  "Clergy") \
    X(STR_IDEA_DIVIN_CLERGE_BONUS, "Two scholars, missions 40% longer") \
    X(STR_IDEA_DIVIN_ORTHODOXIE_NAME,  "Orthodoxy") \
    X(STR_IDEA_DIVIN_ORTHODOXIE_BONUS, "Schisms halved, minorities +60% resentment") \
    X(STR_DOCT_REASON_SLOT,      "No free slot") \
    X(STR_DOCT_REASON_ALREADY,   "Already adopted") \
    X(STR_DOCT_REASON_PAIR,      "Trade or Mercantilism, never both") \
    X(STR_DOCT_REASON_CURRENT,   "One political current at a time") \
    X(STR_DOCT_REASON_INFLUENCE, "Not enough influence") \
    X(STR_INFLUENCE_DEPENSES,    "Upkeep for %d doctrine(s): %d per month") \
    X(STR_INFLUENCE_DEPENSES_0,  "No doctrine to maintain")
