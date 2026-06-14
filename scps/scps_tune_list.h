#ifndef SCPS_TUNE_LIST_H
#define SCPS_TUNE_LIST_H
/*
 * scps_tune_list.h — LE REGISTRE DES TUNABLES (Arc J, X-macro).
 *
 * Liste UNIQUE des constantes de calibrage RUNTIME surchargeables par l'env
 * SCPS_TUNE="NOM=VAL,…". Source de vérité : nom + défaut compilé. Elle pilote
 * le registre, la validation (nom inconnu → exit 2) et `--tunables`.
 *
 * RÈGLE : uniquement des constantes de CALIBRAGE lues au RUNTIME — jamais une
 * taille de tableau ni un initialiseur statique. Étendre = une ligne.
 *
 * Le défaut ici DOIT égaler la valeur passée au site d'appel tune_f("NOM", v).
 */
#define SCPS_TUNABLES(X) \
    /* §G0.4/H7 — le robinet d'or (les bandes de flux) */ \
    X(ENTRETIEN_DIV,        400.0f) \
    X(MANUF_UPKEEP_DAY,       0.05f) \
    X(COURT_FLOOR,         4000.0f) \
    X(COURT_RATE,             0.010f) \
    X(ADMIN_BASE,             0.4f) \
    X(ADMIN_EXP,              1.3f) \
    X(SINK_FLOOR,           500.0f) \
    /* I6 — le marché n'est pas 1:1 : marge d'import sur les achats de chantier */ \
    X(IMPORT_MARGIN_OWN,      1.3f) \
    X(IMPORT_MARGIN_THIRD,    1.8f) \
    X(IMPORT_MARGIN_NONE,     2.0f) \
    X(IMPORT_TOLL_FRAC,       0.30f) \
    /* §G0.1 — le directeur (les fenêtres de température) */ \
    X(DIR_T_HOT,              0.50f) \
    X(DIR_T_COLD,             0.32f) \
    /* §G0.2/C3 — soulèvements & concessions */ \
    X(C3_K_HOLLOW,            0.20f) \
    X(C3_L_HOLLOW,            0.30f) \
    X(CONCEDE_GOLD,         150.0f) \
    /* §H4/L3 — la curée & le choc (le ratio poursuite/choc se CALIBRE, registre J).
     * Alias spec L3 : CHOC_KILL_RATE≡BT_CHOC_MORTS · CUREE_CAP_FRAC≡CUREE_CAP.
     * H4/L4 : la CAVALERIE fait la poursuite — sa part dans la force du vainqueur pousse
     * la curée (CAV_PURSUIT/part) et en relève le plafond (CAV_CUREE_CAP/part). */ \
    X(CUREE_CAP,              0.22f) \
    X(CAV_PURSUIT,            0.45f) \
    X(CAV_CUREE_CAP,          0.40f) \
    X(BT_DMG_K,               0.057f) \
    X(BT_CHOC_MORTS,          0.006f) \
    X(BT_RUPTURE,             0.20f) \
    X(CHOC_ROUNDS_BONUS,      2.0f) \
    /* §B3 — le palier 540 (l'accession) */ \
    X(REGIMENT_PAY,           1.5f) \
    X(REGIMENT_PRICE,        12.0f) \
    X(NAVY_UPKEEP_GOLD,       1.5f) \
    X(AI_SAVOIR_K,            2.5f) \
    /* §spéculation (E3) — les bandes du stockeur IA */ \
    X(SPEC_BUY_BAND,          0.80f) \
    X(SPEC_SELL_BAND,         1.25f) \
    X(SPEC_GOLD_FLOOR,      350.0f) \
    /* P-bis — déclaration de paix : score décisif & timeout de paix blanche */ \
    X(AI_WAR_DECISIVE,       50.0f) \
    X(AI_WAR_EXHAUST,        10.0f) \
    /* Q6 re-baseline — le DOUBLEMENT 48k→96k PAR LE DÉVELOPPEMENT. cap_pop = la
     * taille PLEINE nourrie (socle vivrier) ; eff_cap = ½·cap_pop (plancher) +
     * grenier + logements BÂTIS (manufactures, +HOUSE_MANUF/niveau, plafonné à
     * ½·cap_pop). La graine ensemence ½·cap_pop ; bâtir double la région vers son
     * plein → le monde passe de ~48k à ~96k au siècle (la nourriture suit cap_pop). */ \
    X(EMPIRE_CAP,         13000.0f) \
    X(CITY_CAP,            6500.0f) \
    X(SEED_POP,           48000.0f) \
    X(HOUSE_MANUF,          100.0f) \
    /* #5 — le PUMP À 2 ÉTAGES : le marché local de la cité-état la plus proche sert à
     * RENDEMENT DÉGRESSIF (la marge d'achat monte de MARKET_DIST_FALLOFF par saut). */ \
    X(MARKET_DIST_FALLOFF,    0.12f) \
    /* M4 — l'arbitrage des cités-états (leur moteur), BORNÉ : volume/tick capé, spread
     * MINIMAL pour agir, part CAPTÉE du spread → pas de runaway spéculatif. */ \
    X(ARB_VOL_CAP,            3.0f) \
    X(ARB_MIN_SPREAD,         0.20f) \
    X(ARB_CAPTURE,            0.35f) \
    /* S1 (syncrétisme) — LE COMMERCE OUVRE L'ARCHÉTYPE (Venise ← Grèce) : un contact
     * COMMERCIAL soutenu (route OUVERTE) avec une polity qui PORTE l'archétype X creuse
     * la profondeur de contact, sommée sur les ENTITÉS distinctes. La MER pèse FORT
     * (SEA_W > LAND_W : Venise/Hanse/Gujarat). Seuils : MÉTIER (diffusion) puis PROFOND
     * (= l'accès recherche, la porte d'archétype). YIELDREF module par le VOLUME. */ \
    X(SYNC_TRADE_SEA_W,       2.0f) \
    X(SYNC_TRADE_LAND_W,      1.0f) \
    X(SYNC_TRADE_METIER,      1.0f) \
    X(SYNC_TRADE_PROFOND,     2.0f) \
    X(SYNC_TRADE_YIELDREF,    5.0f) \
    /* S2 (syncrétisme) — la CRISTALLISATION culturelle suit le contact : fraction du fossé
     * de contenu comblée/an par un contact commercial maritime SOUTENU (porte-modulée). */ \
    X(SYNC_FUSE_RATE,         0.10f) \
    /* FAU (faustien — la pente vers la Brèche). FAUST_SPAWN_CHARGE : charge ajoutée par unité
     * de sortie d'un transmuteur (le VOLUME = la fracture). CHARGE_DECAY : décrue passive/tick
     * de l'entropie régionale hors péché (≪ accumulation sous spawn soutenu). ENTROPY_TERMINAL :
     * seuil d'entropie MONDE qui arme le signal terminal (capstone §27). */ \
    X(FAUST_SPAWN_CHARGE,     0.15f) \
    X(CHARGE_DECAY,           0.04f) \
    X(ENTROPY_TERMINAL,       4000.0f) \
    /* FAU5 : au-DESSUS de cette proximité de Brèche (tech_crisis_proximity), l'IA NE cède PLUS à
     * l'échappatoire faustienne du bois/nourriture (prudence) ; en-dessous + famine, OUI. */ \
    X(FAUST_BRECHE_CAUTION,   0.55f) \
    /* F-arc : coût d'or de base (× tier × IPM) pour qu'une IA POSE une manufacture militaire — la
     * « puissance économique » qui gate « combien de fabriques je peux poser ». */ \
    X(MANUF_BUILD_COST,       50.0f) \
    /* F-arc ARSENAL : une manufacture d'ARMES verse ×N au STOCK (l'arsenal que la levée POMPE via
     * econ_arms_take ; le recrutement = stock/POP_PER_UNIT). Le marché (supply/prix), la valeur
     * ajoutée (PIB) et la charge faustienne restent sur la sortie de BASE → l'éco & la Brèche
     * INCHANGÉES ; seul l'arsenal de guerre enfle, ce qu'il faut pour lever les régiments. */ \
    X(MANUF_ARMS_MULT,        10.0f)

#endif /* SCPS_TUNE_LIST_H */
