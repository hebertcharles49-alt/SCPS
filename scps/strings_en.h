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
    X(STR_TUTO_PAGE_2, "Top bar: your gold, your food, your materials, and the health of your crown —\nStability, Legitimacy, Cohesion, Prosperity. Click a province to see it\nup close; open the LEFT SIDEBAR for the whole empire:\nEconomy, Demography, Stocks, Army, Filters.") \
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
    X(STR_COUNCIL_EFF_2, "manufacturing") \
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

