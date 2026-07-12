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
    /* W-GUERRE-3 — L'ARMÉE À SON VRAI PRIX : mesuré (audit de guerre) à 1-1.5 % des
     * DÉPENSES d'État (soldes+marine / Σdépenses, DIAG SCPS_MILDIAG) — un budget militaire
     * FANTÔME. Relevé ×60 (1.5→90) : 10.3 %/6-10.2 % selon graine sur 200 ans (dans la cible
     * 10-15 %), guerres/sim EN HAUSSE (22.7→44/sim seed 9 — l'armée payée reste viable plus
     * longtemps), hégémon mortel INCHANGÉ, aucune spirale de dette (credit_demo 16/16). */ \
    X(REGIMENT_PAY,          90.0f) \
    X(REGIMENT_PRICE,        12.0f) \
    /* LOT 3 (audit de guerre) — le SIÈGE LIT LA GARNISON : chaque point de H_coerc
     * (Garnison/Forteresse/Citadelle bâties, re->build.H_coerc) durcit la place en
     * plus du simple compte de bâtiments — poids modeste (~5-10 % sur la durée
     * finale du siège pour une garnison consistante), jamais l'immortalité. */ \
    X(DEF_PER_H,              0.05f) \
    /* LOT 4 (audit de guerre) — LE PILLAGE DE SIÈGE : fraction de la PRODUCTION
     * mensuelle (supply[], pas le stock accumulé) détournée par une force EN SIÈGE
     * vers le trésor du besiégeur — matière RÉELLEMENT prise (stock décrémenté).
     * Distinct du butin final (LOT P : PILLAGE_INCOME_FRAC, au règlement) ; gaté par
     * le MÊME cooldown anti-re-saccage (pillage_cd). */ \
    X(SIEGE_LOOT_FRAC,        0.25f) \
    /* LOT P (2026-07-07) — PILLAGE UNIFIÉ (règle joueur verbatim : « Piraterie, raids,
     * tout type d'occupation = pillage »). La VALEUR d'un pillage (sac de siège,
     * occupation-capture, raid côtier) = cette fraction du revenu ANNUEL de la
     * VICTIME (econ_country_tax_year), transférée RÉELLEMENT et BORNÉE par ce qui
     * existe. Remplace l'ancien PILLAGE_GOLD_FRAC/PILLAGE_STOCK_FRAC (fraction plate
     * du trésor/stock local, non tunable). */ \
    X(PILLAGE_INCOME_FRAC,    0.20f) \
    /* W-GUERRE-3 : relevé de concert avec REGIMENT_PAY (même ×60) */ \
    X(NAVY_UPKEEP_GOLD,      90.0f) \
    X(AI_SAVOIR_K,            2.5f) \
    /* RELIGION — seuil de zèle : w_faith ≥ ce seuil ⇒ crédo prosélyte FONDE sa foi proactivement */ \
    X(AI_FAITH_ZEAL,          0.5f) \
    /* RELIGION — la DÉRIVE (Réforme) : 1 chance sur N par tour d'empire éligible (une marche
     * culturellement distante dérive vers un schisme adapté à sa culture) → dose le rythme
     * (la Réforme MÛRIT sur des décennies, elle n'éclate pas d'un bloc). Plus haut = plus rare. */ \
    X(AI_DERIVE_ODDS,         8.0f) \
    /* PRÉVISION DIPLO — frein à la menace ENTRANTE : war_risk > GATE ⇒ on freine l'offensive de BRAKE×risk */ \
    X(AI_THREAT_GATE,         0.55f) \
    X(AI_THREAT_BRAKE,        0.5f) \
    X(AI_WAR_LOSING,        -25.0f) \
    X(AI_ALLY_NEED_W,         1.0f) \
    /* MÉTABOLISATION — la recherche accélère ∝ part d'âmes ÉTRANGÈRES DIGÉRÉES (creuset) :
     * income ×= 1 + W·métabolisé. Le signal est ~0 tôt (l'assimilation prend des décennies)
     * ⇒ golden-safe. W=1 ⇒ « métabolisation X% = +X% recherche » (lisible au hover) */ \
    X(AI_METAB_RES_W,         1.0f) \
    /* REVENU DE RECHERCHE — multiplicateur GLOBAL du revenu de savoir (IA + joueur), le levier
     * « même les mauvaises IA font 60 % de l'arbre » (Civ). Le revenu de base est LABOR/POP-bound
     * et sous-alimenté sur 250 ans → à défaut, l'arbre plafonne ~28 % médian. Ce W relève le débit
     * SANS cheapener le coût des nœuds : l'expansion faustienne (charge → Brèche) reste au plein tarif
     * ⇒ l'arbre gonfle par les nœuds NON-faustiens (savoir/société/production, charge nulle), la
     * fenêtre des fins §27 ne s'effondre PAS vers le gate an-180 (contrairement à une coupe de coût,
     * mesuré). Appliqué au revenu de la POP, AVANT la métabolisation (elle module ce débit relevé).
     * DÉCOUPLAGE : le coût des nœuds FAUSTIENS est ×W (ai_effective_cost) → le boost s'y annule,
     * leur cadence reste ≈ baseline, la charge §27 ne s'emballe pas (l'arbre gonfle par le NON-faustien).
     * Calé à 4.5 (arbre méd ~28 % → ~50 %, « les mauvaises IA font 60 % » ; ENTROPY_TECH_W abaissé
     * en regard pour tenir la fenêtre des fins). Au-delà (≥5), un edge LATENT de scps_province_market
     * (stock de province transitoirement négatif émis par la membrane, data-dependent) fait rougir
     * scps_api_demo — laissé à l'orchestrateur (fix membrane + W≥6 pour ~53 %). 1.0 = neutre (golden-safe). */ \
    X(AI_RESEARCH_INCOME_W,   4.5f) \
    /* SAVOIR — la POP produit la recherche (0.01·élite + 0.005·bourgeois + 0.001·journalier /an) ;
     * la branche BIBLIOTHÈQUE module en % (Σ build.savoir · PER, plafonné MAX). Unifié joueur+IA. */ \
    X(SAVOIR_W_ELITE,         0.01f) \
    X(SAVOIR_W_BOURGEOIS,     0.005f) \
    X(SAVOIR_W_LABORER,       0.001f) \
    X(SAVOIR_LIB_PER,         0.067f) \
    X(SAVOIR_LIB_MAX,         0.33f) \
    /* LOT I — CATCH-UP DE SAVOIR : un pays sous CATCHUP_FRAC × médiane mondiale (savoir/tête)
     * bâtit sa Bibliothèque/Monastère quel que soit son éthos (Dominateur/Mercantile/Bureaucrate
     * compris) — la panne mesurée était la FRÉQUENCE de pose (éthos-gated 2/6), pas le revenu. */ \
    X(AI_SAVOIR_CATCHUP_FRAC, 0.45f) \
    /* PUISSANCE COMMERCIALE — la POP MARCHANDE produit le volume échangeable au marché (0.04·bourgeois
     * + 0.01·élite /mois) ; la CHAÎNE COMMERCIALE (Σ build.PE_infra) module en % (BLD_PER/point, plafond
     * BLD_MAX) ; ECO_W = son poids dans la puissance éco diplo. Pool MENSUEL, drainé par les achats. */ \
    X(COMMERCE_W_BOURGEOIS,   0.04f) \
    X(COMMERCE_W_ELITE,       0.01f) \
    X(COMMERCE_BLD_PER,       0.10f) \
    X(COMMERCE_BLD_MAX,       0.50f) \
    X(COMMERCE_ECO_W,         0.05f) \
    /* BARRE D'ACCÈS TECH (Temps 2) — la part d'âmes DIGÉRÉES d'un héritage qui débloque ses
     * signatures par TIER : ≥T1 ⇒ tier-1, ≥T2 ⇒ tier-2, ≥T3 ⇒ la signature tier-3. Voie ACTIVE,
     * en MAX avec la profondeur de contact (commerce/gouvernance). « X% de B ⇒ techs de B ». */ \
    X(METAB_TIER1,            0.10f) \
    X(METAB_TIER2,            0.20f) \
    X(METAB_TIER3,            0.35f) \
    /* BRASSAGE — coeff de DIFFUSION du savoir par MODE d'arrivée (Arrival) : migrant &
     * soumis diffusent PLEIN (1.0, câblé) ; le DÉPORTÉ (esclave) diffuse FAIBLE — savoir
     * arraché, fragmenté, réprimé (janissaire/forge/créole : réel mais mineur). */ \
    X(METAB_DIFFUSE_SLAVE,   0.30f) \
    /* SLAVE_FRACTION — part de la population prise déportée à chaque pillage/razzia/occupation
     * d'un esclavagiste (tech Économie servile OU éthos conquérant Dominateur/Honneur). LOT P
     * (2026-07-07, règle joueur verbatim « 5% de la pop locale ») : calé à 5% — l'esclavage
     * apporte (savoir arraché) sans jamais dominer — volume faible × diffusion faible
     * (METAB_DIFFUSE_SLAVE). Ex-0.08 (brassage 2026-07-03) ; le joueur a fixé la valeur EXACTE. */ \
    X(SLAVE_FRACTION,        0.05f) \
    /* SLAVE_PRICE — prix de base d'une âme au marché des Centres (×ipm à la vente, ×2
     * ipm à l'achat — la double taxe du tier mondial, motif de intertrade_market_buy). */ \
    X(SLAVE_PRICE,           40.0f) \
    /* PACTE MIGRATOIRE (BRASSAGE) — l'échange passif annuel : fraction du groupe dominant qui
     * migre (×0..2 selon l'attractivité relative de la destination) + plancher anti-poussière.
     * LOT G (2026-07-08) — DEUX taux (`demography_migration_pact_tick`) : FRAC (canal ouvert
     * par le pacte COMMERCIAL seul, scps_ai.c §2c élargi) reste au calibrage D'ORIGINE — un
     * pacte commercial peut se former BIEN avant l'an-12 (contrairement à l'alliance),
     * bumper ce taux fait franchir MIN dans la fenêtre golden (mesuré : casse seeds 7/209).
     * FRAC_ALLY (×3, NOUVEAU) : le canal ALLIÉ, dont l'invariant « aucune alliance avant
     * l'an-12 » (golden) tient par construction (déjà vrai avant ce lot) — le taux élevé
     * n'y risque rien. MIN inchangé (30, golden-safe, vérifié). */ \
    X(MIG_PACT_FRAC,        0.006f) \
    X(MIG_PACT_FRAC_ALLY,   0.05f) \
    /* MIG_PACT_FRAC_LATE (VOLUME, 2026-07-08) — le taux de BASE monte lui aussi après la
     * fenêtre golden (même porte GATE_DAYS) : « c'est pas 100 pélos qui vont déstabiliser
     * un pays » — un pacte tenu déplace ~2 %/an du dominant (allié : 5 %), des MILLIERS
     * d'âmes sur des décennies. Avant l'an-12 : FRAC d'origine (golden-safe). */ \
    X(MIG_PACT_FRAC_LATE,   0.02f) \
    X(MIG_PACT_MIN,          30.0f) \
    /* MIG_PACT_ALLY_GATE_DAYS — le taux ÉLEVÉ (FRAC_ALLY) n'entre en vigueur qu'APRÈS ce
     * jour (12 ans, la fenêtre golden) : un pacte (même allié) peut en pratique se former
     * plus tôt que « prévu » sur certaines graines — golden cassait sinon (mesuré, seeds
     * 7/209). Avant le cap, TOUS les pactes utilisent FRAC (comportement D'ORIGINE). */ \
    X(MIG_PACT_ALLY_GATE_DAYS, 4380.0f) \
    /* RÉFUGIÉS (BRASSAGE) — la guerre fait FUIR, l'apaisement fait RESPIRER. FLEE : une région
     * ravagée (revolt_scar > SCAR : sac/révolte) déverse FRAC/an de chaque groupe (≥ MIN) vers la
     * voisine la moins ravagée. HOME_CALM : foyer sous ce seuil ⇒ retour possible. RETURN_PULL :
     * part du réfugié qui rentre/an (× (1−intégration) : le fixé reste) ; MIGRANT_RETURN ténu (le
     * migrant économique respire aussi). SETTLE_INTEG : intégré au-delà ⇒ le réfugié se FIXE.
     * FLEE_FRAC (0.12, VOLUME 2026-07-08) : la fuite déplace cette part/an d'un groupe d'une région
     * ravagée — un sac qui dure 3-4 ans vide ~1/3 de la province (l'exode HISTORIQUE, des milliers
     * d'âmes, « c'est pas 100 pélos qui vont déstabiliser un pays »). L'ancien 0.03-0.04 datait du
     * runaway de révolte (spirale d'écrasement, morts en millions) TUÉ depuis par la Phase 1
     * (grâce empire-wide + cooldown sérialisé) — la déstabilisation de l'hôte par l'afflux est
     * désormais un TRAIT (minorité restive, satisfaction), plus une spirale. Mesuré apparié. */ \
    X(REFUGEE_FLEE_SCAR,     0.40f) \
    X(REFUGEE_FLEE_FRAC,     0.12f) \
    X(REFUGEE_FLEE_MIN,      30.0f) \
    X(REFUGEE_HOME_CALM,     0.25f) \
    X(REFUGEE_RETURN_PULL,   0.12f) \
    X(MIGRANT_RETURN_PULL,   0.015f) \
    X(REFUGEE_SETTLE_INTEG,  0.90f) \
    /* ABSORPTION DU DÉPLACÉ — les INSTITUTIONS de l'hôte accélèrent l'intégration. ASSIM_K : la
     * vitesse d'intégration lit les institutions RÉELLES (build.K_inst) au lieu d'un K plat —
     * K_eff = K + (K_inst−REF)·AMP → institutions solides (école/service/état) assimilent VITE
     * (Italiens/Polonais absorbés), institutions faibles assimilent LENTEMENT (minorité restive). */ \
    X(ASSIM_K_INST_REF,      1.5f) \
    X(ASSIM_K_INST_AMP,      4.0f) \
    /* ATTRACTIVITÉ MIGRATOIRE — un empire ULTRA-BÂTI + ULTRA-PROSPÈRE est un AIMANT : attractivité =
     * prospérité + INST_W·bâti ; le flux de migration ÉCHELONNE avec le gradient d'attractivité
     * (jusqu'à PULL_MAX× la base) au lieu d'un seuil binaire — « migration très élevée » pour l'ultra. */ \
    X(MIG_ATTRACT_INST_W,    1.0f) \
    X(MIG_PULL_MAX,          5.0f) \
    /* REMISE DE PRIX PAR DIFFUSION (métabolisation) — une tech possédée par TOUS les autres
     * empires coûte −MAX % (le savoir répandu se (re)découvre plus vite ; catch-up des retardataires) */ \
    X(AI_TECH_DIFFUSE_MAX,    0.40f) \
    /* EXPLOITATION — boost d'EXTRACTION par brute (modificateur provincial à construire) : +PER_TIER
     * par palier d'amélioration (scale sur les bras), plafonné à MAX_TIER paliers · coût d'or par palier
     * · seuil de déficit (forecast) qui ARME l'amélioration */ \
    X(RAW_BOOST_PER_TIER,     0.05f) \
    X(RAW_BOOST_MAX_TIER,     8.0f) \
    X(RAW_BOOST_COST,        40.0f) \
    /* le +5% d'extraction doit rembourser le palier en ≤ PAYBACK ans (ROI) */ \
    X(RAW_BOOST_PAYBACK,      8.0f) \
    X(RAW_WORKS_NEED,        25.0f) /* recalé avec la 2e passe de coûts (÷3 sur 540/960j) : ≈ le coût du plus gros monument (Académie/Citadelle ~32-37 pierre) */ \
    /* argile/pierre/fer/bois FORCÉS près de la capitale joueur si le biome n'en donne pas */ \
    X(PLAYER_GUARANTEE_RAW,   4.0f) \
    /* CONFORT (poterie+statuaire servies) → bonheur AU-DESSUS du panier (hors-besoin, sans pénalité)
     * + −15 % de besoin de logement (densité tolérée) */ \
    X(COMFORT_JOY,            0.08f) \
    X(COMFORT_HOUSE_RELIEF,   0.15f) \
    /* MANUFACTURES D'ÉTHOS (désir croisé) — le bien de luxe de l'éthos OPPOSÉ ne devient un
     * besoin (Laborer+Élite) qu'à une capitale AVANCÉE : active_needs (=1+capitale_max_tier(pop))
     * ≥ ce seuil. Calé à 4 (capitale MOYENNEMENT développée) : le désir MORD vraiment — les 6
     * ateliers se posent abondamment (commerce culturel vivant) et la satisfaction MONTE (82 % vs
     * 78 % à gate 6 : le luxe est un CONFORT hors-panier `comfort_joy`, servi = bonheur en plus,
     * jamais d'affamage — mesuré). Un gate plus haut (6) laisse le système à moitié endormi ;
     * plus bas (2-3) saturerait le monde d'ateliers de luxe trop tôt. */ \
    X(ETHOS_LUXURY_MIN_TIER,  4.0f) \
    /* ETHOS_LUXURY_WEIGHT — le POIDS du luxe d'éthos dans le panier (× BASE_PRICE·need). C'est
     * le LEVIER : plus haut = le luxe non servi pèse plus lourd → satisfaction tombe plus au début
     * (avant que le commerce culturel s'établisse), remonte plus une fois servi. 1.0 = poids nominal
     * (~8 % du panier). Baisser pour adoucir la chute de démarrage, monter pour un levier plus mordant. */ \
    X(ETHOS_LUXURY_WEIGHT,    1.0f) \
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
    /* COMPACITÉ BAS RÉGIME (T8) : à peu d'empires, 120 prov/empire NOIE le duel dans le
     * vide (le monde reste trop grand pour eux — l'hégémon ne plafonne jamais). Le confort
     * par empire MONTE de _LOW (à 2 empires) à 1.0 (dès _FULL empires) : rare ⇒ duel frontal
     * serré ; 6+ ⇒ vaste & confortable (inchangé, intention Q6). 0 = off (linéaire pur). */ \
    X(WORLD_EMP_COMFORT_LOW, 0.58f) \
    X(WORLD_EMP_COMFORT_FULL, 6.0f) \
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
    /* F1 (implémenteur colonisation/construction IA) — LA CADENCE SUIT LA PERSONNALITÉ :
     * gate_years = 1 + (1−w_expand)·AI_COLONY_TEMPO. Un Dominateur (w_expand≈0.9) fonde
     * quasi chaque année (gate≈1.3→1) ; un Pacifiste (w_expand≈0.15) attend ~1+0.85×3≈3-4
     * ans entre deux essaimages. 0 = tout le monde fonde chaque année (comportement d'avant,
     * cadence uniforme) ; plus haut = l'appétit compte davantage. */ \
    X(AI_COLONY_TEMPO,        3.0f) \
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
     *     sur des années (MEM_DECAY/jour) : les marques qui SURVIVENT au statut — la TRAHISON (BETRAYAL)
     *     et la SÉCESSION (le pays né d'une guerre civile aime moins l'empire père — Flandre vs France).
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
    X(OPINION_MEM_SECESSION,    45.0f) \
    X(OPINION_MEM_CAP,         100.0f) \
    X(AI_OFFER_ALLY_OPINION,    10.0f) \
    X(AI_OFFER_PACT_OPINION,     0.0f) \
    /* BRASSAGE — le pacte migratoire (frontières ouvertes) exige plus de confiance que le
     * commercial : l'IA ne le propose qu'à un ALLIÉ, ou à une opinion ≥ ce seuil. */ \
    X(AI_OFFER_MIG_OPINION,     40.0f) \
    /* HAMEAUX LIBRES (POLITY_WILD) — Peuples Libres épars près des jouables (tue le « siècle
     * d'inertie » : chaque empire a 2 objectifs voisins dès l'an 0). WILD_PER_PLAYABLE hameaux
     * par jouable (0 = DÉSACTIVE) · WILD_POP graine EXACTE (750 ; WILD_POP_VAR=0 → an-0 LOCKÉ sur
     * la formule, plus de jitter) · WILD_CAP plafond d'accueil (≥2·WILD_POP : la graine TIENT) ·
     * WILD_SPAWN_HOPS rayon BFS (2-3 tuiles : les hameaux restent PRÈS du spawn, jamais à l'autre
     * bout du monde) · WILD_CULTURE_DISTINCT (1 = culture distincte du voisin) · WILD_DEFECT_YEARS
     * ans de contact pacifique avant ralliement culturel — 0 = DÉSACTIVÉ (défaut : les Peuples libres
     * ne rallient jamais seuls, owner change SEULEMENT par conquête/vassalité, comme un pays normal) ·
     * WILD_HOARD réserve de brutes · WILD_REGIMENTS régiments défensifs levés. */ \
    X(WILD_PER_PLAYABLE,      2.0f) \
    X(WILD_POP,             750.0f) \
    X(WILD_POP_VAR,           0.0f) \
    X(WILD_CAP,            1600.0f) \
    X(WILD_FOOD,              8.0f) \
    X(WILD_SPAWN_HOPS,        3.0f) \
    X(WILD_CULTURE_DISTINCT,  1.0f) \
    X(WILD_DEFECT_YEARS,      0.0f) \
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
    /* IDENTITÉ CULTURELLE : seuil de poids pour qu'un apport entre dans le nom ;
     * à partir de la 2e fusion, le composé reçoit un ethnonyme autonome. Un
     * peuple disparu ne survit dans une colonie que comme substrat mémoriel. */ \
    X(CULTURE_NAME_MINOR_MIN,       0.10f) \
    X(CULTURE_AUTONYM_GENERATION,   2.0f) \
    X(CULTURE_RUIN_SUBSTRATE_W,     0.10f) \
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
    X(BUILD_RESERVE_BULK,     15.0f) /* recalé avec la 2e passe de coûts (÷3 sur 540/960j) : le fond de réserve suit l'échelle des chantiers */ \
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
    X(POP_R_BASE,             0.0198f) \
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
     * COLD_RAMP_PER_YEAR : décalage de température annuel (froid, C4).
     * THORN_CELLS_PER_YEAR : cellules corrompues/an (ronces, C5).
     * THORN_RANDOM_FRAC : fraction de voisins choisis aléatoirement (erratique, C5).
     * MERV_PHASE_DAYS : durée de chaque palier de la Merveille en jours (C6).
     * MERV_CHARGE_PER_TICK : charge faustienne ajoutée par tick de chantier (C6). */ \
    X(ENTROPY_FIN,           55.0f) \
    X(ENDGAME_YEAR_OPEN,    180.0f) \
    /* 1.0→1.35 (recalage 2026-07-06, ENTDIAG seed 9) : les refontes éco avaient fait
     * RECULER le tir de ~80 ans (croisement an ~260, historique ~184-195) — à 1.35 le
     * tir médian revient ~an 235-270, les mondes sanglants plus tôt (ENTROPY_BLOOD_W).
     * 1.35→0.20 (push arbre 2026-07-08) : AI_RESEARCH_INCOME_W=6 ~double l'arbre ⇒ ~6× de
     * charge faustienne (Σ tech charge) ⇒ l'entropie franchissait 55 vers l'an 130 et TOUTES
     * les fins s'effondraient sur le gate an-180 (MESURÉ, ENTDIAG). À 0.20, chaque nœud faustien
     * pèse moins mais l'arbre en porte plus ⇒ le tir médian revient ~an 200-235 (spread baseline,
     * gate respecté), au prix d'une rareté accrue sur les mondes bas-charge (fragmentés). */ \
    X(ENTROPY_TECH_W,         0.20f) \
    X(COLD_RAMP_PER_YEAR,     0.005f) \
    X(THORN_CELLS_PER_YEAR, 200.0f) \
    X(THORN_RANDOM_FRAC,      0.35f) \
    X(MERV_PHASE_DAYS,     3650.0f) \
    X(MERV_CHARGE_PER_TICK,   0.5f) \
    /* V1a — ENDGAME UNIFIÉ (2026-07-06) : une barre, quatre nourritures + la Merveille
     * gatée par métabolisation. ENTROPY_BREACH_W : poids de la pression de l'Âge de la
     * Brèche (wp->age_breach_flux, EXISTANT — juste additionné, décision #1). ENTROPY_
     * BLOOD_W : poids du ratio morts-de-guerre/pop_ref dans l'entropie mondiale — calé
     * pour qu'une guerre mondiale MAJEURE (ratio ~ENDGAME_BLOOD_FRAC) pèse comme une
     * charge tech soutenue (même ordre que ENTROPY_TECH_W), PAS plus. ENDGAME_BLOOD_
     * FRAC : au-delà de ce ratio, LE SANG L'EMPORTE quelle que soit la nature dominante
     * (décision #1 — une barre, un visage dominant). Le drain/plancher-pop de SANG (ex-
     * SANG_DRAIN_PER_YEAR/SANG_POP_FLOOR) est SUPPRIMÉ (2026-07-11, « fins corrigées ») :
     * SANG est désormais un plancher PERMANENT sur revolt_scar (SANG_SCAR_MIN, en fin de
     * fichier) — les moteurs EXISTANTS (prod/croissance/exode par REFUGEE_FLEE_SCAR) font
     * tout le reste, aucun canal neuf. */ \
    X(ENTROPY_BREACH_W,       0.6f) /* 0.3→0.6 : spec ÂGES 2026-07-11 (« poids de la
                                     * Brèche dans l'entropie : 0,60 ») — appliqué par
                                     * l'orchestrateur, les 2 agents ayant atterri. */ \
    /* ENTROPY_BLOOD_W recalé 1→8 (2e passe, mesuré 5 graines) : le ratio passe sur la
     * pop VIVANTE (0.05-1.5 observé) — à 8, un monde sanglant (ratio ~1) pousse ~8 pts
     * d'entropie (précipite sa fin), un monde calme ~0.4 (négligeable). */ \
    X(ENTROPY_BLOOD_W,        8.0f) \
    /* 0.20→0.09 (investigation 2026-07-11, SCPS_FINDIAG 6 graines × 2 sims) : le ratio
     * AU TIR (an ~180, mémoire décrue HL 40 ans / pop vivante) s'étale 0.014-0.112 —
     * le seuil 0.20 datait de l'ère PRÉ-Phase-1 où la spirale de révolte gonflait les
     * morts (÷10 000 depuis) ; il n'était JAMAIS atteint (SANG 0/200 au gigasweep).
     * À 0.09, les ~2 mondes les plus sanglants sur 12 franchissent — « SANG présent
     * dans AU MOINS les mondes les plus sanglants ». L'assiette (Campaign dead_choc+
     * dead_pursuit) est SAINE : les morts de révolte (~150-2665/sim) sont négligeables
     * devant les ~30-80k morts de bataille — c'était le SEUIL qui était d'une autre
     * ère, pas l'assiette. */ \
    X(ENDGAME_BLOOD_FRAC,     0.09f) \
    /* SANG_MEMORY_HL : demi-vie (ans) de la MÉMOIRE des morts de guerre — sans décrue,
     * le cumul à vie dépassait la pop renouvelée (40-961 % en sweep) et toute partie
     * longue devenait SANG ; à 40 ans le seuil BLOOD_FRAC mesure « une génération qui
     * a perdu un cinquième du monde ». */ \
    X(SANG_MEMORY_HL,         40.f) \
    /* #32 (LE SANG SIGNE TON RÈGNE, 2026-07-06) — le CORRECTIF joueur : le ratio de sang
     * ci-dessus est MONDIAL, un pacifiste dans un monde IA sanglant le franchissait sans
     * avoir combattu. Quand une main humaine existe (campaign_get_human()≥0), SANG
     * n'est retenue que si SA PART dans ce sang (war_dead_player/war_dead, même mémoire
     * décrue) atteint BLOOD_PLAYER_SHARE — sinon on retombe au sélecteur normal (rare
     * dominant/hash). Sans main humaine (chronique/viewer), la garde est INACTIVE. */ \
    X(BLOOD_PLAYER_SHARE,     0.25f) \
    /* LOTERIE DE FIN EAU/RONCES/FROID (refonte, investigation « lisse les
     * déclencheurs, trouve pourquoi certaines fins ne viennent jamais », 2026-07-11,
     * cf. TROUVAILLES.md) — MESURÉ (SCPS_FINDIAG, 6 graines × 2 sims × 250 ans) :
     * la fin EAU (essence/foreuse) était structurellement IMPOSSIBLE, pas juste
     * rare — côté IA (scps_ai.c, hors périmètre de cette mission), la foreuse n'a
     * qu'un seul couplage de construction (famine de fer, tier-4, sans beeline),
     * la conso d'essence mesurée était à 0.0 dans 100% des tirs observés ; l'ancien
     * argmax-ou-climat ne pouvait donc JAMAIS choisir EAU (0×poids=0, et l'autre
     * branche climat n'était jamais atteinte puisqu'un des 2 autres compteurs
     * dépasse quasi toujours le seuil de dominance). FIX : endgame_pick_fin_lottery
     * fusionne les deux anciens modes en UNE loterie — poids = PLANCHER climatique
     * (FIN_BASE_*, jamais nul, modulé par la température/humidité RÉELLES du monde,
     * cf. world_avg_climate) + bonus proportionnel à la PART de production
     * (FIN_PROD_W_* × share ∈[0,1]). FIN_BASE_EAU est monté à 1.5 (vs 1.0 pour
     * RONCES/FROID) pour COMPENSER que sa part de production est structurellement
     * ~0 — sans ce plancher relevé, EAU resterait écrasée par la production réelle
     * des deux autres. Arithmétique (clim≈1, un transmuteur dominant share≈1) :
     * P(EAU)=1.5/4.5≈33 % · P(dominant)=2/4.5≈44 % · P(l'autre)≈22 % — mélangé sur
     * la population fer-dominant (~2/3 des tirs) et flux-dominant (~1/3), les trois
     * fins sortent ~29-37 % chacune. CALIBRÉ sur 6 graines × 2 sims × 250 ans
     * (SCPS_FINDIAG) : voir TROUVAILLES.md pour la mesure AVANT/APRÈS. */ \
    X(FIN_BASE_EAU,           1.5f) \
    X(FIN_BASE_RONCES,        1.0f) \
    X(FIN_BASE_FROID,         1.0f) \
    X(FIN_PROD_W_ESSENCE,     1.0f) \
    X(FIN_PROD_W_FLUX,        1.0f) \
    X(FIN_PROD_W_FER,         1.0f) \
    /* LOT F (2026-07-08) — L'EXODE AVANT LA MORT : EAU/FROID/RONCES/CHAUD routent une
     * PART de leur pression (habitabilité effondrée) vers la machinerie de réfugiés au
     * lieu de tuer sur place. EXODUS_INTENSITY_MIN : la fin doit mordre franchement
     * (endgame_region_intensity) avant qu'on fuie — calé BAS (mesuré : le gate temporel
     * ENDGAME_YEAR_OPEN=180 laisse souvent peu de RUNWAY — une fin qui latche à l'an
     * 217-246 n'a que 4-33 ans pour monter ; à 0.30 la plupart des fins n'auraient JAMAIS
     * le temps de déclencher l'exode). EXODUS_FRAC_PER_YEAR : part de la pop d'une région
     * en fuite, évacuée/an. SANG N'Y PARTICIPE PAS (2026-07-11, « fins corrigées ») : le
     * plancher permanent sur revolt_scar (SANG_SCAR_MIN) fait déjà fuir via le seuil
     * EXISTANT REFUGEE_FLEE_SCAR (scps_demography.c) — un second canal ferait doublon. */ \
    X(EXODUS_INTENSITY_MIN,  0.15f) \
    X(EXODUS_FRAC_PER_YEAR,  0.10f) \
    /* FIN_CHAUD (2026-07-08 ; REPLI 2026-07-08b) — LE RÉCHAUFFEMENT, la fin de REPLI
     * des MONDES CALMES. Le combustible RÉELLEMENT brûlé (bois de feu SERVI au panier +
     * charbon consommé en intrant de manufacture — l'offre servie ∝ pop prospère, jamais
     * un bonus plat) s'ACCUMULE (endgame_fuel_ratio, mémoire décrue / pop vivante,
     * ~8-11 en monde prospère à l'an 180+). ⚠ Il NE charge PLUS l'entropie mondiale
     * (design REPLI, demande joueur : « seconde position derrière la fin prévue ») —
     * s'il la poussait, il PRÉCIPITAIT le seuil et VOLAIT la fin naturelle (mesuré :
     * RÉCHAUFFEMENT 48 % des fins au sweep combiné, HIVER/RONCES/EAU rabotés de moitié).
     * Le réchauffement est un DÉCLENCHEUR SÉPARÉ (endgame_select_and_fire, branche « sous
     * le seuil ») : il ne s'active QUE sur un monde qui, sans lui, resterait SANS FIN.
     * FUEL_FALLBACK_DELAY : années après ENDGAME_YEAR_OPEN avant que le repli s'arme (la
     * fin naturelle a eu tout ce temps pour sortir). FUEL_FALLBACK_MIN : fuel_ratio
     * minimal — un monde calme ET SOBRE (peu brûlé) reste sans fin (cohérent : seul qui
     * a pollué cuit). FUEL_COAL_W : le charbon pèse plus lourd que le bois (~×3 —
     * l'industrie fossile : poudrière/forge céleste — vs l'âtre). FUEL_MEMORY_HL :
     * demi-vie (ans) de la mémoire de combustible — plus LONGUE que celle du sang (le
     * CO2 persiste plus qu'un souvenir de guerre). HEAT_RAMP_PER_YEAR : décalage de
     * température annuel (miroir COLD_RAMP). SEA_RISE_CELLS_PER_YEAR : montée des eaux
     * passive — N cellules de terre côtière les plus BASSES noyées/an (tri à clé entière,
     * déterministe). CALIBRAGE (sweep 2026-07-08b) : DELAY=30/MIN=4 — le réchauffement
     * ne prend QUE les sans-fin (les autres fins RESTAURÉES à leur niveau pré-CHAUD). */ \
    X(FUEL_FALLBACK_DELAY,    60.0f) \
    /* 4.0→2.0 (investigation 2026-07-11) : les 24 mondes « sans fin » du gigasweep
     * avaient un combustible/tête final de 0.9-3.9 — TOUS sous l'ancien seuil 4.0
     * (le repli n'a rattrapé que 2/24). À 2.0, 22/24 sont rattrapés ; les 2 mondes
     * réellement SOBRES (0.9, 1.0) restent sans fin — cohérent (« un monde calme ET
     * sobre reste sans fin », le design d'origine, seul le niveau était trop haut :
     * il avait été calé sur des mondes prospères d'AVANT la refonte éco per-capita). */ \
    X(FUEL_FALLBACK_MIN,       2.0f) \
    X(FUEL_COAL_W,             3.0f) \
    X(FUEL_MEMORY_HL,         60.0f) \
    X(HEAT_RAMP_PER_YEAR,      0.010f) \
    X(HEAT_DROUGHT,            0.6f) \
    X(SEA_RISE_CELLS_PER_YEAR, 140.0f) \
    /* LOT F — CATASTROPHES DU MONDE CALME : un monde SANS fin en vue (aucune fin
     * latchée, entropie loin du seuil, passé un an-butoir) reçoit une pression accrue
     * de catastrophes GÉO (quake/flood/drought/fire/plague, EVENTS[] existant — motif
     * data-driven réutilisé, aucun système neuf) — motive l'IA à réagir (greniers/
     * relocalisations) au lieu de sommeiller un siècle. CALM_DISASTER_YEAR : an-butoir
     * (le monde a eu le temps de se stabiliser). CALM_DISASTER_ENTFRAC : fraction du
     * seuil ENTROPY_FIN sous laquelle le monde est jugé « calme » (loin de toute fin).
     * CALM_DISASTER_MULT : multiplicateur de fréquence des chocs (divise leur mtth). */ \
    X(CALM_DISASTER_YEAR,   200.0f) \
    X(CALM_DISASTER_ENTFRAC,  0.5f) \
    X(CALM_DISASTER_MULT,     2.5f) \
    /* CORRECTIF Merveille (relecture joueur) : la métabolisation-VICTOIRE juge CHAQUE
     * héritage sur SA PROPRE diaspora (dénominateur par-héritage), pas la pop totale de
     * l'empire (piège : ce dernier dénominateur rend 6 cultures ≥0.35 simultanément
     * IMPOSSIBLE, 6×0.35>1). METAB_MERV_RATIO : part digérée DE SA COMMUNAUTÉ propre
     * (ratio, pas de la pop totale) ; METAB_MERV_MIN : plancher d'âmes digérées (pas de
     * culture « métabolisée » à 30 personnes noyées dans un grand empire). */ \
    X(METAB_MERV_RATIO,       0.60f) \
    X(METAB_MERV_MIN,       500.0f) \
    /* DÉDUP RÉVOLTE (Option B, 2026-07-04) — statecraft ne fait plus fire de révolte lui-même ;
     * scps_revolt.c est le SEUL acteur. Il replie le SIGNAL d'agitation legacy (L/coercion/choc
     * de conquête/stabilité/garnison, statecraft_agitation 0-100) dans son propre `worst` — le
     * grief politique/de légitimité que la misère-de-groupe (faim/taxe/aliénation/répression/
     * non-intégration) ne capte pas. W_AGITATION_UNREST : poids du bump (agitation/100 × W). */ \
    X(W_AGITATION_UNREST,     0.20f) \
    /* PHASE 3a suite — SOUTIEN ÉTRANGER AUX REBELLES (2026-07-04) : une guerre civile ACTIVE
     * (rebel_country≥0, Phase 3a) attire l'opportunisme d'un rival hostile de la couronne — un
     * SECOND FRONT (déclaration de guerre à la couronne assiégée) et, modestement, un renfort
     * matériel à l'armée rebelle. Gaté sur une guerre civile INCARNÉE (> an-12) ⇒ golden intact.
     * OPINION : seuil de score d'hostilité (déjà en guerre ailleurs + opinion basse/négative +
     * menace/rancune envers la couronne, mêmes signaux que diplo_relation/diplo_rancor) au-delà
     * duquel un rival BACKS les rebelles — délibérément HAUT (une combinaison de signaux, pas un
     * seul suffit) : mesuré, le monde est CHAOTIQUE (une guerre de plus en reformule 100 autres
     * sur 250 ans) donc ce seuil vise l'ORDRE de grandeur « occasionnel », pas un pourcentage
     * exact — l'orchestrateur affine par un balayage multi-graines. ATWAR_W : poids du bonus
     * « déjà belliqueux ». MAXWARS : le bailleur doit compter STRICTEMENT MOINS de guerres en
     * cours que ce plafond (1 = doit être en PAIX PARTOUT ailleurs — capacité stricte, ne pas
     * piocher un pays déjà surétendu).
     * MATERIEL_FRAC : renfort de milice PROPORTIONNEL à la force rebelle ACTUELLE (jamais un
     * paquet ABSOLU — un plancher fixe doublerait un soulèvement minimal tout en restant
     * anecdotique pour un gros) ; la fraction reste MODESTE aux deux échelles. */ \
    X(AI_REBEL_BACKING_OPINION,   1.60f) \
    X(AI_REBEL_BACKING_ATWAR_W,   0.35f) \
    X(AI_REBEL_BACKING_MAXWARS,   1.0f) \
    X(AI_REBEL_MATERIEL_FRAC,     0.20f) \
    /* REBEL_VET_ADD — noyau de VÉTÉRANS (déserteurs/anciens soldats) AJOUTÉ à l'armée
     * rebelle : des paquets de piquiers disciplinés (≫ la milice paysanne) qui REJOIGNENT
     * la révolte EN PLUS de la masse — ÉPARS mais RÉEL. L'armée rebelle nue (1-2 paquets)
     * se fait anéantir ; ce noyau la rend RÉELLE → ~1 révolte sur 20 bat la couronne. */ \
    X(REBEL_VET_ADD,              2.0f) \
    /* LOT H — LA RÉVOLTE SERVILE STRUCTURELLE : au-delà de SLAVE_REVOLT_SHARE (0.20 —
     * Rome tient 30 % d'esclaves, pas 60), la part servile d'une région pousse
     * STRUCTURELLEMENT le déficit de révolte (revolt_scan, même motif que
     * W_AGITATION_UNREST : un FOLD sur `worst`, jamais un tirage plat). SLAVE_REVOLT_W :
     * poids du terme au-delà du seuil (le contrepoids du mécanisme H — sans lui, garder
     * ses esclaves est pur profit). */ \
    X(SLAVE_REVOLT_SHARE,         0.20f) \
    X(SLAVE_REVOLT_W,             1.20f) \
    /* LOT I — LE PRIX DU POOL RESPIRE : le prix plat (SLAVE_PRICE×ipm) faisait un
     * pool-poubelle (l'IA vend, peu achète, or gratuit). SLAVE_POOL_REF = profondeur
     * de RÉFÉRENCE du pool mondial (âmes, toutes origines confondues) : pool ≪ REF ⇒
     * rare ⇒ CHER (jusqu'à ×2.5) ; pool ≫ REF ⇒ surabondant ⇒ BON MARCHÉ (jusqu'à
     * ×0.5). Borné [0.5, 2.5] (même discipline que les paliers de prix intertrade). */ \
    X(SLAVE_POOL_REF,           600.0f) \
    /* P4 — LA VENTE IA DU SURPLUS (le pool VIT) : l'esclavagiste garde KEEP_FRAC de la
     * pop régionale en mains serviles et vend SELL_FRAC de l'excédent par an. Sans
     * cette règle le pool restait à 0 (mesuré 5 graines) — le canal d'achat mort. */ \
    X(SLAVE_AI_KEEP_FRAC,        0.02f) \
    X(SLAVE_AI_SELL_FRAC,        0.25f) \
    /* LOT G (2026-07-08) — L'AUTRE SENS DU CANAL : un esclavagiste EN PÉNURIE DE BRAS
     * (Σ level×labor demandé par son bâti > son bassin labor_avail) achète au pool des
     * Centres — sans quoi les âmes s'y entassaient (sweep giga : rachats/sim ≈ 0 malgré
     * un pool profond). BUY_FRAC : part du DÉFICIT comblée/an (le pool se vide en
     * décennies, pas d'un coup — miroir de SELL_FRAC). intertrade_slave_buy borne déjà
     * le budget (trésor de la province) et le pool disponible : aucun aspirateur possible. */ \
    X(SLAVE_AI_BUY_FRAC,         0.20f) \
    /* W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ (payant) : fabriquer une revendication contre
     * une cible coûte FAB_CB_COST_YEARS années de SON revenu (corrompre des élites riches
     * coûte cher — l'or SORT du trésor du fabricant et disparaît, la corruption quitte
     * l'État). FAB_MATURE_DAYS = maturation (l'intrigue mûrit avant d'être exploitable) ;
     * FAB_VALID_DAYS = fenêtre de validité une fois mûre (le grief acheté s'évente, pas
     * de cooldown sur les AUTRES cibles). */ \
    X(FAB_CB_COST_YEARS,         2.0f) \
    X(FAB_MATURE_DAYS,         365.0f) \
    X(FAB_VALID_DAYS,          1825.0f) \
    /* V2a — LE CONSEIL VIVANT : faction, loyauté, paie. RECRUTER pousse SA
     * faction (HIRE_LEVER) ; RENVOYER froisse l'opposée (DISMISS_GRIEF). La
     * loyauté CONVERGE (LOYAL_RATE/mois) vers une cible (grief de SA faction ×
     * PAY_ADJ de la paie) ; le rot (capture d'État) ACCÉLÈRE la chute
     * (ROT_BOOST), jamais la remontée. BETRAYAL_THRESHOLD = « au bord ». */ \
    X(COUNCIL_HIRE_LEVER,        0.10f) \
    X(COUNCIL_DISMISS_GRIEF,     0.10f) \
    X(COUNCIL_LOYAL_RATE,        0.05f) \
    X(COUNCIL_ROT_BOOST,         1.5f) \
    X(COUNCIL_PAY_ADJ,          30.0f) \
    X(COUNCIL_BETRAYAL_THRESHOLD,15.0f) \
    /* GOULOT D'ARMES (2026-07-06) — l'arsenal d'État est une demande de MARCHÉ ∝ pop
     * (comme les outils) : l'État vise un stock d'armes capable de lever sa force
     * (ARMS_PER_LABORER × bras) → prix ↑ → §NF bâtit l'armurerie. ARSENAL_DECAY : les
     * armes ne pourrissent pas au mois (rouille lente 1 %, vs 15 % des périssables) —
     * sinon l'équilibre de stock restait sous ce qu'une levée annuelle demande. 0 = éteint. */ \
    X(ARMS_PER_LABORER,          0.05f) \
    X(ARSENAL_DECAY,             0.99f) \
    /* LOT T (2026-07-07) — le TIER par POP (doctrine joueur), SOURCE UNIQUE
     * (capitale_max_tier, scps_labor.c) : T2 2000 · T3 3000 · T4 4000 · T5 5000 ·
     * T6/T7 au-delà (non spécifiés par le joueur, conservés à leur valeur d'origine).
     * Lus UNE fois (cache statique, cf. scps_labor.c) — pas de F10 live sur ces 6-là. */ \
    X(TIER2_POP,              2000.0f) \
    X(TIER3_POP,              3000.0f) \
    X(TIER4_POP,              4000.0f) \
    X(TIER5_POP,              5000.0f) \
    X(TIER6_POP,              8000.0f) \
    X(TIER7_POP,             10000.0f) \
    /* KIT DE DÉPART (2026-07-10, demande joueur) : petit STOCK déposé sur la province-
     * capitale de chaque EMPIRE/JOUEUR à la genèse (scps_econ.c, site du CS_TRADE_POOL) —
     * de quoi tenir le premier hiver et lever une garde. 0 = désactivé. */ \
    X(SPAWN_KIT_WOOD,          250.0f) \
    X(SPAWN_KIT_FOOD,         2000.0f) \
    X(SPAWN_KIT_CLAY,           20.0f) \
    X(SPAWN_KIT_IRON,           20.0f) \
    X(SPAWN_KIT_STONE,          20.0f) \
    X(SPAWN_KIT_TOOLS,          20.0f) \
    X(SPAWN_KIT_ARMS,          100.0f) \
    X(SPAWN_KIT_RANGED,        100.0f) \
    X(SPAWN_KIT_BEER,           20.0f) \
    /* TRADITIONS → CIRCUIT DES TUNABLES (2026-07-10, demande joueur : « elles ne sont
     * pas dans le registre, elles n'ont aucun levier ») : chaque coefficient route un
     * levier de HeritageLeviers (scps_heritage.h) vers une ENTRÉE moteur EXISTANTE,
     * PAR PAYS (culture_build_for) — jamais un tune_set global. 0 = levier débranché.
     *  · REND   → tech_prod des provinces (rang NODE_PROD_PCT) : ±0.30 levier → ±30 % prod
     *  · INFL   → standing statecraft (assiette de l'Influence, rang prestige) : ±0.75 → ±7.5 pts
     *  · CAP    → pénalité off-culture de society_sat (la tenue de la diversité) : ±1.0 → ±30 %
     *  · PERM   → entrée P d'assimilation_tick (vitesse d'assimilation) : ±0.5 → ±1.5 P
     *  · ARCANE → coût des nœuds FAUSTIENS (rang remise de diffusion) : ±1.0 → ∓25 %
     *  · DERIVE → fuse_rate du contact culturel (S2) : ±0.20 → ±20 %
     *  · FRACT  → fold du grief de révolte (rang W_AGITATION_UNREST) : ±1.0 → ±0.06 */ \
    X(TRAD_REND_W,               1.0f) \
    X(TRAD_INFL_W,              10.0f) \
    X(TRAD_CAP_W,                0.30f) \
    X(TRAD_PERM_W,               3.0f) \
    X(TRAD_ARCANE_W,             0.25f) \
    X(TRAD_DERIVE_W,             1.0f) \
    X(TRAD_FRACT_W,              0.06f) \
    /* CONSEIL — RANGS & COÛTS (2026-07-10, docs/CONSEIL_ORIENTATIONS_2026-07-10.md) :
     * bonus de rang I = BASE (par siège, Savoir/Royaume/Ouvrages) ; II ×TIER2_MULT,
     * III ×TIER3_MULT. Coût = econ_country_tax_year(cid) × TIERn_REVENUE_RATE × IPM,
     * prélevé /12 (mensuel) — REMPLACE l'ancien prix nominal (SC_TIER_COST). */ \
    X(COUNCIL_SAVOIR_BASE,       0.12f) \
    X(COUNCIL_ROYAUME_BASE,      0.15f) \
    X(COUNCIL_OUVRAGES_BASE,     0.20f) \
    X(COUNCIL_TIER2_MULT,        1.50f) \
    X(COUNCIL_TIER3_MULT,        2.00f) \
    X(COUNCIL_TIER1_REVENUE_RATE,0.015f) \
    X(COUNCIL_TIER2_REVENUE_RATE,0.030f) \
    X(COUNCIL_TIER3_REVENUE_RATE,0.050f) \
    /* CONSEIL — EFFICACITÉ POLITIQUE : clamp(BASE + K_PER·K + LOY_W·loyauté/100 −
     * CORRUPTION_PER_POINT·Corruption, MIN, MAX). Multiplie SEULEMENT la part
     * conseiller (bonus final du siège = bonus de rang × efficacité). */ \
    X(COUNCIL_EFF_BASE,          0.70f) \
    X(COUNCIL_EFF_K_PER,         0.03f) \
    X(COUNCIL_EFF_LOY_W,         0.15f) \
    X(COUNCIL_EFF_CORRUPTION_PER_POINT, 0.0035f) \
    X(COUNCIL_EFF_MIN,           0.50f) \
    X(COUNCIL_EFF_MAX,           1.15f) \
    /* CONSEIL — MISSION DÉCENNALE au siège responsable (P3) : bonus de récompense
     * (or ET matières) = PER_RANK × (rang−1) × efficacité ; réussite/échec bougent
     * la loyauté du titulaire du siège responsable (déduit du type, aucun état neuf). */ \
    X(COUNCIL_MISSION_REWARD_PER_RANK,  0.05f) \
    X(COUNCIL_MISSION_SUCCESS_LOYALTY,  5.0f) \
    X(COUNCIL_MISSION_FAILURE_LOYALTY, 10.0f) \
    /* ORIENTATIONS POLITIQUES DU JOUEUR (2026-07-10, docs/CONSEIL_ORIENTATIONS_2026-07-10.md)
     * — REMPLACENT les 4 anciens grands décrets (scps_decrees.{h,c}). RÈGLE : jamais
     * tune_set — chaque site de lecture applique tune_f("CLÉ") × decree_mult(cid,
     * DECREE_X, mult) (1.0 si inactif/impayé ce mois). Coût = econ_country_tax_year(cid)
     * × REVENUE_RATE × IPM, prélevé /12. RATIONS⊥FOYERS et CIRCULATION⊥FRONTIÈRES
     * (radio-boutons, decree_toggle). LA POLITIQUE DE TRIBUT SORT du catalogue (retirée
     * de l'enum DecreeId ; son levier diplo scps_diplo.c reste intact, désexposé). */ \
    X(DECREE_RATIONS_REVENUE_RATE,        0.005f) \
    X(DECREE_RATIONS_FOOD_NEED_MULT,      0.95f) \
    X(DECREE_RATIONS_POP_R_BASE_MULT,     0.97f) \
    X(DECREE_FOYERS_REVENUE_RATE,         0.015f) \
    X(DECREE_FOYERS_POP_R_BASE_MULT,      1.05f) \
    X(DECREE_FOYERS_FOOD_NEED_MULT,       1.04f) \
    X(DECREE_ECOLES_REVENUE_RATE,         0.02f) \
    X(DECREE_ECOLES_SAVOIR_W_MULT,        1.05f) \
    X(DECREE_ATELIERS_REVENUE_RATE,       0.02f) \
    X(DECREE_ATELIERS_MANUF_COST_MULT,    0.95f) \
    X(DECREE_COMPTOIRS_REVENUE_RATE,      0.015f) \
    X(DECREE_COMPTOIRS_COMMERCE_W_MULT,   1.05f) \
    X(DECREE_CIRCULATION_REVENUE_RATE,    0.0075f) \
    X(DECREE_CIRCULATION_MIG_PACT_MULT,   1.10f) \
    X(DECREE_FRONTIERES_REVENUE_RATE,     0.0f) \
    X(DECREE_FRONTIERES_MIG_PACT_MULT,    0.0f) \
    X(DECREE_FRONTIERES_COMMERCE_W_MULT,  0.95f) \
    /* ex-DECREE_MECENAT (bit RÉUTILISÉ — spec : « aucun enum/état/save neuf ») : le nom/
     * flavor affichés sont « Fêtes publiques », les clés gardent le nom de code MECENAT. */ \
    X(DECREE_MECENAT_REVENUE_RATE,        0.015f) \
    X(DECREE_MECENAT_UNREST_MULT,         0.95f) \
    X(DECREE_LEGATIONS_REVENUE_RATE,      0.015f) \
    X(DECREE_LEGATIONS_INFLUENCE_PER_MONTH, 0.25f) \
    X(DECREE_LEVEE_REVENUE_RATE,          0.0f) \
    X(DECREE_LEVEE_MIN_LEVEL,             2.0f) \
    /* DÉCISIONS PONCTUELLES — AFFRANCHISSEMENT (verbe CMD_MANUMIT existant, scps_sim.c) +
     * AUDIT DES OFFICES (DECISION_AUDIT_OFFICES, scps_decrees.c : condition + coût
     * ponctuel + cooldown SÉRIALISÉ + effet immédiat faction_audit/L capitale). Le
     * delta de Corruption de l'audit (-20 pts) est DÉJÀ hardcodé dans faction_audit
     * (scps_factions.c, hors périmètre de cette mission) — non dupliqué ici en
     * registre décoratif (règle du fichier : uniquement des constantes RÉELLEMENT
     * lues au runtime). */ \
    X(DECISION_MANUMIT_COMMUNAUTAIRE_BIAS, 0.10f) \
    X(DECISION_AUDIT_CORRUPTION_MIN,      20.0f) \
    X(DECISION_AUDIT_REVENUE_RATE,        0.25f) \
    X(DECISION_AUDIT_COOLDOWN_YEARS,       5.0f) \
    X(DECISION_AUDIT_L_DELTA,              0.3f) \
    /* FINS CORRIGÉES (2026-07-11, docs/AGES_FINS_2026-07-11.md) — les 3 fins réécrites pour
     * frapper d'un coup / dégrader plutôt qu'effacer / marquer plutôt que drainer, sur des
     * moteurs EXISTANTS (aucun canal neuf). WATER_RIFT_ARMS/_LENGTH/_STEP : géométrie du
     * masque du rift d'eau (promus depuis les #define locaux RIFT_ARMS/RIFT_ARM_LEN/
     * RIFT_ARM_STEP de scps_endgame.c, valeurs INCHANGÉES) — cataclysm_water_seed trace le
     * masque COMPLET puis TOUTES les régions marquées sombrent AU TICK DE DÉCLENCHEMENT
     * (SINK_RIFTS_PER_YEAR, le budget par-an, est SUPPRIMÉ) ; adjacency + refragmentation
     * recalculées UNE seule fois après. THORN_CELLS_PER_YEAR/_RANDOM_FRAC (déjà au registre,
     * plus haut, INCHANGÉS) gardent leur rôle ; THORN_FLIP_FRAC (le seuil qui détachait une
     * région majoritairement ronces) est SUPPRIMÉ — BIO_THORNS garde son habitabilité 0,05,
     * plus aucune région n'est détachée/supprimée, seule l'habitabilité+le grain dégradent
     * (econ_cold_refresh, déjà appelé après chaque propagation annuelle). SANG_SCAR_MIN :
     * promu depuis le #define local de scps_endgame.c (valeur INCHANGÉE) — le seuil au-delà
     * duquel une région ravagée (revolt_scar) entre dans la marque PERMANENTE sang_scar[r]
     * (le plancher qui ne guérit plus : chaque tick, si revolt_scar régionale ≥ ce seuil,
     * sang_scar[r]=max(sang_scar[r],revolt_scar), puis CHAQUE province de la région voit son
     * revolt_scar planché à sang_scar[r] — les moteurs EXISTANTS, pas un nouveau canal). */ \
    X(WATER_RIFT_ARMS,        5.0f) \
    X(WATER_RIFT_LENGTH,     96.0f) \
    X(WATER_RIFT_STEP,        3.0f) \
    X(SANG_SCAR_MIN,          0.15f) \
    /* ÂGES SANS ORDRE IMPOSÉ (2026-07-11, docs/AGES_FINS_2026-07-11.md, raccord 10) —
     * chaque âge = déclencheur matériel + jitter déterministe + effet + citation,
     * AUCUN ordre imposé (Soulèvements↔Tyrans restent la SEULE paire mutuellement
     * exclusive, revérifiée à l'avènement — scps_events.c). AGE_TRIGGER_JITTER_YEARS :
     * 0-N ans d'attente entre éligibilité et avènement (hash seed×âge×année éligible).
     * AGE_STRUCTURAL_DECAY_DAY : résorption/jour des bonus TRANSITOIRES (I/L/H/myth +
     * P/mig_mult/research_mult ; remplace le 0,0004 codé en dur). Les seuils *_C/_P/_I/
     * _L/_H/_DIVERSITY/_FLUX sont les DELTAS pointés par le nom de l'âge (Échanges/
     * Découvertes/Empires/Brèche/Lumières/Soulèvements/Tyrans) ; les *_LEVER/_GRIEF
     * sont les leviers de faction SCOPÉS aux pays MATÉRIELLEMENT concernés (posés une
     * fois à l'avènement de CHAQUE âge, jamais un pays au hasard). */ \
    X(AGE_TRIGGER_JITTER_YEARS,        4.0f) \
    X(AGE_STRUCTURAL_DECAY_DAY,        0.00015f) \
    X(AGE_EXCHANGE_NODE_VALUE,         1.0f) \
    X(AGE_EXCHANGE_NODE_MIN,           4.0f) \
    X(AGE_EXCHANGE_NODE_SHARE,         0.08f) \
    X(AGE_EXCHANGE_C,                  0.50f) \
    X(AGE_EXCHANGE_P,                  0.50f) \
    X(AGE_EXCHANGE_MIG_PACT_MULT,      1.15f) \
    X(AGE_EXCHANGE_MERCHANT_LEVER,     0.08f) \
    X(AGE_DISCOVERY_KNOWN_PAIR_SHARE,  0.12f) /* 0.35 (spec) etait INATTEIGNABLE sous la
                                            * diplo-fog (1/200 au gigasweep 2026-07-11 :
                                            * le ratio compte cites-etats+hameaux) ;
                                            * 0.12 mesure = 7/8 sims sur 4 graines. */ \
    X(AGE_DISCOVERY_COUNTRY_MIN,       6.0f) \
    X(AGE_DISCOVERY_C,                 0.50f) \
    X(AGE_DISCOVERY_RESEARCH_MULT,     1.10f) \
    X(AGE_DISCOVERY_FOG_RADIUS_ADD,    1.0f) \
    X(AGE_DISCOVERY_TRANSGRESSEUR_LEVER, 0.06f) \
    X(AGE_DISCOVERY_MERCHANT_LEVER,    0.04f) \
    X(AGE_EMPIRES_REGIONS_WORLD,       8.0f) \
    X(AGE_EMPIRES_REGIONS_ONE_COUNTRY, 4.0f) \
    X(AGE_EMPIRES_HELD_YEARS,          35.0f) \
    X(AGE_EMPIRES_INTEGRATION_MULT,    1.20f) \
    X(AGE_EMPIRES_CONQUEROR_LEVER,     0.10f) \
    X(AGE_HERO_EFFICIENCY_MIN,         1.00f) \
    X(AGE_HERO_LOYALTY_MIN,            75.0f) \
    X(AGE_HERO_MISSION_REWARD,         1.20f) \
    X(AGE_HERO_MISSION_REWARD_CAPTURED, 1.30f) \
    X(AGE_HERO_FACTION_LEVER,          0.08f) \
    X(AGE_HERO_REFUSED_GRIEF,          0.08f) \
    X(AGE_BREACH_CHARGE,               6.0f) \
    X(AGE_BREACH_FLUX,                 1.50f) \
    X(AGE_BREACH_TRANSGRESSEUR_LEVER,  0.12f) \
    X(AGE_LUMIERES_SAVOIR_MEAN,        5.0f) \
    X(AGE_LUMIERES_C_MEAN,             4.5f) \
    X(AGE_LUMIERES_I,                  1.50f) \
    X(AGE_LUMIERES_SOLVENT,            1.25f) \
    X(AGE_LUMIERES_LEGISTE_LEVER,      0.06f) \
    X(AGE_LUMIERES_COMMUNAUTAIRE_LEVER, 0.04f) \
    /* SOULÈVEMENTS↔TYRANS — le VRAI embranchement (investigation 2026-07-11,
     * SCPS_AGEDIAG 6 graines × 2 sims × 250 ans, cf. TROUVAILLES.md). MESURÉ :
     * « ≥2 pays en révolution » était quasi UNIVERSEL (la vague de révoltes des
     * ans 5-13 atteint 2+ dans 12/12 sims → Soulèvements advenait an 8-13 partout,
     * verrouillant Tyrans À VIE — 0/200 au gigasweep) ; et les seuils Tyrans
     * étaient INATTEIGNABLES de toute façon (fracture_moy réelle plafonne 0.36-0.72
     * vs seuil 3.0 — un facteur 5-8× ; SI_moy ne redescend jamais sous ~6 après
     * l'an 2 vs seuil <5). Recalage aux mondes RÉELS : MIN_COUNTRIES 2→8 (la
     * simultanéité STRICTE — une vraie vague mondiale, atteinte dans ~10/12 sims,
     * médiane an 8-9 inchangée, plus tardive/jamais dans les mondes plus calmes) ·
     * FRACTURE 3.0→0.30 · SI 5.0→8.5 (DEREAL 1.25 inchangé — il est atteignable).
     * Table de course mesurée (éligibilité, jitter 0-4 des deux côtés) :
     * Soulèvements 10/12 · Tyrans 2/12 (~17 %, cible 15-40 %) · aucun 0/12 —
     * l'exclusivité devient un embranchement d'histoire, plus un verrou. */ \
    X(AGE_SOULEVEMENTS_MIN_COUNTRIES,  8.0f) \
    X(AGE_SOULEVEMENTS_L,              1.50f) \
    X(AGE_SOULEVEMENTS_COMMUNAUTAIRE_LEVER, 0.12f) \
    X(AGE_TYRANS_FRACTURE,             0.30f) \
    X(AGE_TYRANS_DEREAL,               1.25f) \
    X(AGE_TYRANS_SI,                   8.5f) \
    X(AGE_TYRANS_H,                    1.75f) \
    X(AGE_TYRANS_DIVERSITY,            1.50f) \
    X(AGE_TYRANS_CONQUEROR_LEVER,      0.08f) \
    X(AGE_TYRANS_LEGISTE_LEVER,        0.04f)

#endif /* SCPS_TUNE_LIST_H */
