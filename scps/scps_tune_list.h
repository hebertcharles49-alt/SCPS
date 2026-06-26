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
    /* CONSERVATION du commerce : le prélèvement de l'importateur (Y = X + levy) sur les routes —
     * l'exportateur encaisse tout (gross + levy), l'importateur paie tout : zéro faucet, zéro sink. */ \
    X(TRADE_LEVY,             0.10f) \
    /* §G0.1 — le directeur (les fenêtres de température) */ \
    X(DIR_T_HOT,              0.50f) \
    X(DIR_T_COLD,             0.32f) \
    /* §G2 — LE DIRECTEUR-AMPLITUDE (la boucle « tale ») : un intégrateur de TRAUMATISME
     * (adapt_days) monte sous les chocs (température T·CHARGE jours/an) et redescend au calme
     * (demi-vie HALF jours) ; l'AMPLITUDE dramatique = adapt_days/SCALE saturé [0..1]. Le
     * BUDGET de mise en scène accumule ∝ pop·richesse·temps (BUDGET_POP par 1000 hab/an +
     * BUDGET_GOLD par 1000 or/an), borné BUDGET_CAP. Un PRÉSAGE coûte OMEN_COST points et ne
     * sort qu'au-dessus de OMEN_AMPL (le monde doit « vibrer » pour qu'un augure prenne). */ \
    X(AMPL_TRAUMA_CHARGE,   180.0f) \
    X(AMPL_TRAUMA_HALF,     900.0f) \
    X(AMPL_TRAUMA_MAX,     2000.0f) \
    X(AMPL_TRAUMA_SCALE,    500.0f) \
    X(AMPL_BUDGET_POP,        0.02f) \
    X(AMPL_BUDGET_GOLD,       0.01f) \
    X(AMPL_BUDGET_CAP,      400.0f) \
    X(AMPL_OMEN_COST,        60.0f) \
    X(AMPL_OMEN_AMPL,         0.35f) \
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
    /* P-bis — le CONTRE composition-vs-composition (bt_day) : mordant au choc (^CTR_BITE) et
     * part de curée ∝ avantage de contre du vainqueur. C'est ce qui donne des dents à la matrice. */ \
    X(CTR_BITE,               0.6f) \
    X(CTR_PURSUIT,            0.30f) \
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
    /* §war-smoothing — lisse la distribution des guerres : SOCLE d'appétit (les mondes consolidés
     * voient quand même la guerre) ÷ (1 + SATURATION × paires en guerre) (les mondes fendus ne
     * spiralent plus). */ \
    X(AI_WAR_BASELINE,        0.05f) \
    X(AI_WAR_SATURATION,      0.20f) \
    X(AI_WAR_CAP,             3.0f) \
    /* Q6 re-baseline — le DOUBLEMENT PAR LE DÉVELOPPEMENT. cap_pop = la taille PLEINE
     * nourrie (socle vivrier) ; eff_cap = ½·cap_pop (plancher) + grenier + logements
     * BÂTIS (manufactures, +HOUSE_MANUF/niveau, plafonné à ½·cap_pop). La graine ensemence
     * sous le plancher ; bâtir double la région vers son plein (la nourriture suit cap_pop). */ \
    X(EMPIRE_CAP,         13000.0f) \
    X(CITY_CAP,            6500.0f) \
    /* GENÈSE PAR-POLITÉ (re-baseline) — la pop an-0 est SEMÉE PAR ENTITÉ, plus un total
     * plat : chaque EMPIRE naît avec EMPIRE_SEED âmes, chaque CITÉ-ÉTAT CITY_SEED, répartis
     * uniformément sur ses régions actives (sous ½·cap_pop). Avec les WILD (2/empire ·
     * WILD_POP), an-0 ≈ n·EMPIRE_SEED + nCS·CITY_SEED + 2n·WILD_POP. La pop CROÎT ensuite
     * vers EMPIRE_CAP/CITY_CAP (l'apex visé, Passe 2). */ \
    X(EMPIRE_SEED,         4000.0f) \
    X(CITY_SEED,           2000.0f) \
    /* SCALE DU MONDE — le nombre de TERRITOIRES (et donc régions/pays, par agglomération) SUIT le
     * nombre d'empires, façon Civ. PRESETS de TAILLE = nombre d'empires : tiny 2 · petit 4 · normal 6
     * (défaut) · grand 8 · énorme 10 · HUGE 12. PROV = BASE + PER_EMPIRE·n_empires + PER_CITY·n_city_states,
     * SANS clamp artificiel (seul SCPS_MAX_PROV — calibré HUGE=12 — borne ⇒ ≤12 empires JAMAIS rogné).
     * PER_EMPIRE = la DENSITÉ (place par empire : capitale + colonisation + tampon de spawn) ; ~95 ⇒
     * ~32 régions-terre/empire (1.3× la limite de packing à SPAWN_SAFE_HOPS). tiny ⇒ ~234 terr.,
     * normal ⇒ ~654, huge ⇒ ~1284. Baisser PER_EMPIRE = plus serré ; monter = plus de friche vierge. */ \
    X(WORLD_PROV_BASE,       24.0f) \
    X(WORLD_PROV_PER_EMPIRE,120.0f) \
    X(WORLD_PROV_PER_CITY,    5.0f) \
    /* Calage de saturation Poisson : la terre tient ~SAT_K/pas² germes (384 à pas 18 ⇒ 384·18²).
     * assign_provinces en DÉRIVE le pas pour atteindre le nombre de territoires visé. */ \
    X(WORLD_PROV_SAT_K,  124416.0f) \
    /* SPAWN « SAFE » — distance-région MIN (sauts d'adjacence terrestre) entre deux EMPIRES à la
     * genèse : aucun empire ne se colle à un voisin. Cités-états & hameaux libres y sont permis (zone
     * tampon « habitée mais pas rivale »). La mer coupe l'adjacence ⇒ une île isolée passe toujours
     * (les « Angleterre » insulaires émergent). Trop grand sur un petit monde ⇒ moins d'empires posés. */ \
    X(SPAWN_SAFE_HOPS,        6.0f) \
    /* Rayon de spawn ADAPTATIF : on tente SPAWN_SAFE_HOPS, et si la géométrie ne case pas tous les
     * empires demandés, on resserre d'un cran jusqu'à SPAWN_SAFE_HOPS_MIN. « Tout caser » prime, à
     * l'espacement max possible (HUGE=12 retombe sur 5 ; les presets qui tiennent à 6 le gardent). */ \
    X(SPAWN_SAFE_HOPS_MIN,    5.0f) \
    /* VOCATION — nb de brutes (hors vivrier & stratégiques) gardées par région : la tuile
     * produit sa spécialité, pas la liste complète (la traîne mineure vient du commerce). */ \
    X(REGION_RAW_KEEP,        2.0f) \
    /* REFONTE A0 — EXTRACTION LABOR-BOUND (ressource PAR OUVRIER). out = ouvriers × YIELD ×
     * geo_eff × prix. GEO_REF = raw_cap donnant geo_eff=1 (la tuile standard) ; GEO_CAP =
     * plafond de qualité ; LABOR_SHARE = part des journaliers à l'extraction (le levier de
     * CALIBRAGE du volume brut ; le reste staffe les manufactures). */ \
    X(EXTRACT_GEO_REF,        4.5f) \
    X(EXTRACT_GEO_CAP,        3.0f) \
    X(EXTRACT_LABOR_SHARE,    0.65f) \
    /* REFONTE A2 — multiplicateur de la BOUCHE vivrière (grain/poisson/viande). La cible
     * « décidée » est 100/100hab/an (table NEED) mais la géographie des vocations (2 brutes/
     * région) + le commerce bornent ce que le monde NOURRIT : FOOD_NEED calibre la demande
     * vivrière sans toucher la table (1.0 = la table telle quelle). Levier anti-famine. */ \
    X(FOOD_NEED,              1.0f) \
    /* REFONTE A5 — la NOURRITURE DU SPAWN : socle de grain (raw_cap) sur la capitale de
     * chaque empire (geo_eff = SPAWN_FOOD_RAW/EXTRACT_GEO_REF). La SEULE règle vivrière de
     * worldgen ; tout le reste est géologie + commerce (0 = aucun grenier de spawn). */ \
    X(SPAWN_FOOD_RAW,        12.0f) \
    /* PIPELINE IA ÉCO — la PRÉVISION (forecast) qui rend l'IA voyante de ses flux.
     * SAFETY_HORIZON : un runway sous ce nb d'années est URGENT (le stress monte). PROJ_HORIZON :
     * fenêtre de projection du shortfall (colonisation/priorités anticipent à cet horizon).
     * SAFE_STOCK_MONTHS : coussin de réserve visé (mois de conso) pour les flux critiques.
     * COLONY_SURVIVE_SEED : fraction de COLONY_MIN_POP semée par une colonie de SURVIE
     * (gate vivrier levé vers une tuile-déficit, anti-spirale poule-œuf). */ \
    X(AI_SAFETY_HORIZON,     12.0f) \
    X(AI_PROJ_HORIZON,       25.0f) \
    X(AI_SAFE_STOCK_MONTHS,   6.0f) \
    X(COLONY_SURVIVE_SEED,    0.5f) \
    /* COLONISATION : poids du STEER needs-aware (biais vers les tuiles d'un flux à déficit
     * URGENT) au-dessus du score d'expansion de CAPACITÉ. La capacité reste le défaut (pop
     * saine) ; le besoin oriente la cible quand ça presse. 0 = colonisation aveugle (capacité). */ \
    X(AI_COLONY_NEEDS_W,      1.5f) \
    /* PIPELINE DIPLO — la VALEUR SUBJECTIVE oriente la CIBLE (pas l'éthos, qui décide la
     * MÉTHODE). COVET_W : poids du BESOIN (Σ raw_cap × stress(runway) × prix) dans la valeur
     * d'une province d'autrui → l'IA convoite qui TIENT ce qui lui manque. COMPLEMENT_W :
     * poids de MON manque dans le choix d'allié (s'allier à qui me COMPLÈTE). */ \
    X(AI_COVET_W,             0.5f) \
    X(AI_COMPLEMENT_W,        1.0f) \
    /* PIPELINE DIPLO étage 3 — LA VASSALITÉ SUR LA DURÉE (la VALEUR cible, l'ÉTHOS décide la
     * MÉTHODE : tenir-et-traire vs digérer). INTÉGRATION : un vassal TENU à la paix se rapproche
     * de son maître (INTEGRATE_YEARS = ~temps de pleine intégration à culture identique ; freiné
     * par la distance culturelle réelle et le grief). CONTRIBUTION TYPÉE : passé le seuil
     * CONTRIB_GATE d'intégration (bond MÛRI), le vassal verse selon sa FONCTION (commerce→or /
     * agraire→vivres / martial→force) × son appréciation (1−grief), à hauteur CONTRIB_BASE de son
     * potentiel. ANNEXION : un maître ANNEXEUR (éthos Dominateur/Honneur) DIGÈRE sa province vassale
     * INTÉGRÉE (≥ ANNEX_MIN_INTEGRATION) — un PROCESSUS de durée ∝ prix × (1 − DISCOUNT·intégration),
     * payé GOLD_PER_PRICE or/an ; à terme, transfert + cicatrice DOUCE (SOFT_SCAR·(1−intégration)).
     * Le seuil CONTRIB_GATE 0.65 est INATTEIGNABLE en 12 ans (max 12/20=0.60) ⇒ déterminisme 12 ans
     * INCHANGÉ par construction (tout l'étage mord APRÈS la fenêtre golden). */ \
    X(AI_VASSAL_INTEGRATE_YEARS, 20.0f) \
    X(AI_VASSAL_CONTRIB_GATE,     0.65f) \
    X(AI_VASSAL_CONTRIB_BASE,     0.05f) \
    X(AI_ANNEX_MIN_INTEGRATION,   0.65f) \
    X(AI_ANNEX_YEARS_PER_PRICE,   0.5f) \
    X(AI_ANNEX_GOLD_PER_PRICE,    2.0f) \
    X(ANNEX_INTEGRATION_DISCOUNT, 0.6f) \
    X(ANNEX_SOFT_SCAR,            0.4f) \
    X(ANNEX_SCAR_DECAY,          0.20f) \
    X(ANNEX_SAT_W,                0.5f) \
    /* #26 — OPINION ±100 (les relations TENDENT VERS 0 : decay naturelle). DEUX couches :
     * (1) MODIFICATEURS DE STATUT — temporaires, calculés chaque tick, DISPARAISSENT à la rupture
     *     (alliance +ALLY tant qu'elle tient → à la rupture l'opinion retombe vers 0 ; guerre −WAR ;
     *     vassalité +VASSAL ; pacte +PACT ; embargo −EMBARGO ; rancune territoriale −RANCOR_W·rancor,
     *     la RIVALITÉ, qui décroît déjà). (2) MÉMOIRE D'ACTES — `opinion_mem`, durable, décroît vers 0
     *     sur des années (MEM_DECAY/jour) : la TRAHISON (BETRAYAL) — la seule marque qui survit au statut.
     * La STRUCTURE (kinship/complément) reste dans diplo_relation (le « avec qui ») ; l'opinion porte
     * l'HISTOIRE. AI_OFFER_*_OPINION : seuil d'acceptation d'une offre entrante (ai_consider_offer). */ \
    X(OPINION_ALLY,             50.0f) \
    X(OPINION_WAR,              60.0f) \
    X(OPINION_VASSAL,           30.0f) \
    X(OPINION_PACT,             15.0f) \
    X(OPINION_EMBARGO,          25.0f) \
    X(OPINION_RANCOR_W,          8.0f) \
    X(OPINION_MEM_DECAY,         0.0003f) \
    X(OPINION_MEM_BETRAYAL,     35.0f) \
    X(OPINION_MEM_CAP,         100.0f) \
    X(AI_OFFER_ALLY_OPINION,    10.0f) \
    X(AI_OFFER_PACT_OPINION,     0.0f) \
    /* HAMEAUX LIBRES (POLITY_WILD) — Peuples Libres épars près des jouables (tue le « siècle
     * d'inertie » : chaque empire a 2 objectifs voisins dès l'an 0). WILD_PER_PLAYABLE hameaux
     * par jouable (0 = DÉSACTIVE) · WILD_POP graine EXACTE (750 ; WILD_POP_VAR=0 → an-0 LOCKÉ sur
     * la formule, plus de jitter) · WILD_CAP plafond d'accueil (≥2·WILD_POP : la graine TIENT) ·
     * WILD_SPAWN_HOPS rayon BFS (2-3 tuiles : les hameaux restent PRÈS du spawn, jamais à l'autre
     * bout du monde) · WILD_CULTURE_DISTINCT (1 = culture distincte du voisin) · WILD_DEFECT_YEARS ans
     * de contact pacifique avant ralliement culturel · WILD_HOARD réserve de brutes · WILD_REGIMENTS
     * régiments défensifs levés. */ \
    X(WILD_PER_PLAYABLE,      2.0f) \
    X(WILD_POP,             750.0f) \
    X(WILD_POP_VAR,           0.0f) \
    X(WILD_CAP,            1600.0f) \
    X(WILD_FOOD,              8.0f) \
    X(WILD_SPAWN_HOPS,        3.0f) \
    X(WILD_CULTURE_DISTINCT,  1.0f) \
    X(WILD_DEFECT_YEARS,      8.0f) \
    X(WILD_HOARD,            60.0f) \
    X(WILD_REGIMENTS,         2.0f) \
    /* POOL CITÉ-ÉTAT — réserve TRADABLE de matières brutes (bois/fer/argile/pierre) déposée sur
     * la région-pivot de chaque cité-état : le marché mondial (#5) la revend aux empires nés
     * NUS, qui importent ainsi de quoi BÂTIR au lieu de stagner au plancher ½·cap_pop. */ \
    X(CS_TRADE_POOL,       1000.0f) \
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
    X(MANUF_ARMS_MULT,        10.0f) \
    /* Le FOND du TRIO de bâti (bois/pierre/argile, econ_build_reserve) : ce qu'une région GARDE avant
     * d'exporter son surplus — sans quoi l'export auto la vide et le gate de chantier la refuse. */ \
    X(BUILD_RESERVE_BULK,     60.0f) \
    /* DETTE (scps_credit, incrément 1) — la ligne de crédit ÉMERGE de la taille éco (capacité à
     * rembourser ∝ pop) ; le taux price le risque (ratio de dette + chute de légitimité). */ \
    X(CREDIT_LINE_BASE,       0.5f) \
    X(CREDIT_RATE_BASE,       0.05f) \
    /* ANTI-EMBALLEMENT de la dette : au-delà de ce ratio dette/ligne, taux ET assiette
     * d'intérêt PLAFONNENT → l'intérêt devient constant, la dette croît linéairement
     * (sans ça : intérêt ∝ dette² → spirale géométrique → treasury -1e31 → NaN ~105 ans). */ \
    X(CREDIT_RATIO_CAP,       8.0f) \
    /* FERTILITÉ = f(besoins satisfaits) — doublement ~40 ans au plancher (R_BASE=ln2/40),
     * ~20 ans au panier plein (le bonus DOUBLE la base). needs_met (poids 0.85) + prospérité
     * normalisée PIB/tête (MID/SPAN, poids 0.15). TAU = seuil de couverture (got≥τ) qui compte
     * une catégorie comme « satisfaite ».
     * ⚠ R_BASE = LE LEVIER DE VITALITÉ (le monde est BISTABLE) : à ln2/100 la pop se FIGE au
     *   bassin BAS (≈½·cap_pop, monde « mou »), tout build/colonisation glisse dessus ; le seuil
     *   de bascule est entre /50 et /40. ln2/40 sort du bassin bas — le monde vif est plus CONTESTÉ
     *   (turbulence mesurée ~12 guerres/sim, coups, IPM ~1.31 : le PRIX de la vie, COMPARABLE à /30)
     *   mais DIFFÉRENCIÉ (riche=plein, pauvre=modeste, pas truqué) ; /40 retenu pour le RÉALISME —
     *   le doublement réalisé ≈20-40 ans encadre la cible ~30 ans. Dialable d'UNE ligne (ou
     *   SCPS_TUNE=POP_R_BASE=…) vers /35 ou /30 (plus plein, turbulence comparable). */ \
    X(POP_R_BASE,             0.01733f) \
    X(POP_PROSP_MID,          0.2f) \
    X(POP_PROSP_SPAN,         1.8f) \
    X(POP_PROSP_W,            0.15f) \
    X(POP_NEEDS_W,            0.85f) \
    /* COUPLAGE SATISFACTION (asymétrique) : une province CONTENTE (satisfaction > 0.5) croît un
     * peu plus vite ; la satisfaction BASSE ne PUNIT PAS (plancher à 0) → un peuple nourri mais
     * grognon se reproduit quand même (pas de creusement du trou des low seeds en turbulence) ;
     * la récompense PRIME la reprise une fois la province apaisée. W = échelle du surcroît. */ \
    X(POP_SAT_W,              0.20f) \
    X(NEEDS_MET_TAU,          0.5f) \
    /* UTILITÉ DE L'HABITABILITÉ — la terre RUDE produit ET peuple moins : malus = (1−hab)·K
     * sur prod ET popgrowth (habitabilité 50 % → −10 %). EXEMPTE la région-siège (province de
     * départ). Lit la coordonnée habitability ∈ [0,1] — aucun bonus plat. K=0 désactive. */ \
    X(HAB_MALUS_K,            0.20f) \
    /* MODIFICATEURS PROVINCIAUX (diégétiques) — TERRE D'ABONDANCE : une région
     * SOUS-PEUPLÉE + NOURRIE + en paix se repeuple vite (le rebond des low seeds,
     * routé par l'entrée DÉMO de la croissance, PAS un bonus plat sur la sortie).
     * REF = remplissage sous lequel ça s'active ; K = échelle du surcroît de natalité.
     * Auto-ciblé : une terre déjà pleine (fill ≥ REF) n'y touche pas → les seeds
     * RICHES restent inchangés, seuls les low/assommés-sous-REF décollent. */ \
    X(PROVMOD_ABOND_REF,      0.45f) \
    X(PROVMOD_ABOND_K,        2.0f) \
    /* Lot 2 — FAVEURS provinciales À ÉTAT (entrée DÉMO). FERVEUR : élan d'une colonie fondée,
     * semé à 1, décru sur ~15 ans (DECAY). RECONSTRUCTION : renaissance d'après-choc, amorcée
     * par une cicatrice profonde, libérée à mesure qu'elle se referme (recon·(1−scar)), décrue
     * sur ~10 ans. LIMON : natalité dense d'un delta (embouchure). K = échelle du bonus démo. */ \
    X(PROVMOD_FERVEUR_K,      0.5f) \
    X(PROVMOD_FERVEUR_DECAY,  0.067f) \
    X(PROVMOD_RECON_K,        0.6f) \
    X(PROVMOD_RECON_DECAY,    0.10f) \
    X(PROVMOD_LIMON_K,        0.15f) \
    /* Dons GÉO sélectifs (entrée DÉMO) : gibier abondant (1/3 des bois) · manne halieutique
     * (1/3 des côtes) — la richesse vivrière du biome soutient une natalité un peu plus dense. */ \
    X(PROVMOD_GIBIER_K,       0.10f) \
    X(PROVMOD_HALIEU_K,       0.10f) \
    /* BONNE ADMINISTRATION (entrée DÉMO) : des institutions bâties (K) tiennent l'ordre/services
     * → natalité un peu plus dense (le pendant DÉMO de « admin efficace → développement »). */ \
    X(PROVMOD_ADMIN_K,        0.06f) \
    /* CAPSTONE §27 — Entropie mondiale + 4 fins + Merveille.
     * ENTROPY_FIN : seuil terminal qui déclenche une fin (~200 ans sur seed 9).
     * ENDGAME_YEAR_OPEN : gate dur — aucune apocalypse avant cette année (victoire
     *   Merveille exemptée : le joueur peut vaincre à tout moment).
     * ENTROPY_TECH_W : poids de la charge de tech faustienne dans l'entropie mondiale
     *   (décision C1 — élargie hors transmuteurs seuls).
     * SINK_RIFTS_PER_YEAR : régions englouties/an (eau, C3).
     * COLD_RAMP_PER_YEAR : décalage de température annuel (froid, C4).
     * THORN_CELLS_PER_YEAR : cellules corrompues/an (ronces, C5).
     * THORN_RANDOM_FRAC : fraction de voisins choisis aléatoirement (erratique, C5).
     * MERV_PHASE_DAYS : durée de chaque palier de la Merveille en jours (C6).
     * MERV_CHARGE_PER_TICK : charge faustienne ajoutée par tick de chantier (C6). */ \
    X(ENTROPY_FIN,           55.0f) \
    X(ENDGAME_YEAR_OPEN,    180.0f) \
    X(ENTROPY_TECH_W,         1.0f) \
    X(SINK_RIFTS_PER_YEAR,    3.0f) \
    X(COLD_RAMP_PER_YEAR,     0.005f) \
    X(THORN_CELLS_PER_YEAR, 200.0f) \
    X(THORN_RANDOM_FRAC,      0.35f) \
    X(MERV_PHASE_DAYS,     3650.0f) \
    X(MERV_CHARGE_PER_TICK,   0.5f)

#endif /* SCPS_TUNE_LIST_H */
