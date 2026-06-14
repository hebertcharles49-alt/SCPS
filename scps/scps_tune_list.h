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
     * Alias spec L3 : CHOC_KILL_RATE≡BT_CHOC_MORTS · CUREE_CAP_FRAC≡CUREE_CAP. */ \
    X(CUREE_CAP,              0.22f) \
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
    X(AI_WAR_EXHAUST,        10.0f)

#endif /* SCPS_TUNE_LIST_H */
