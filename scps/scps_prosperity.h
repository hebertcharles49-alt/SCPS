#ifndef SCPS_PROSPERITY_H
#define SCPS_PROSPERITY_H
/*
 * scps_prosperity.h — Générateur de Prospérité (PE/SI)
 *
 * Calcule par pays :
 *   PE_interne  = énergie de la diversité interne
 *   PE_externe  = énergie des contacts commerciaux
 *   P_potentiel = PE_int + Σ PE(n)
 *   SI          = stabilité interne (K + H)
 *   P_réalisé   = P_potentiel × rendement
 *   Lumière     = β·P_pot·(K/10)
 *   tresor_tick = γ·P_réalisé
 *   croissance  = δ·P_réalisé·(L/10)
 *   surchauffe  = max(0, (P/10)·C + flux_faustien − K)
 */
#include "scps_types.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"   /* L vivant, entrée de scps_order */

typedef enum {
    POLE_CALME = 0,
    POLE_FLORISSANT,
    POLE_BOUILLONNANT,
    POLE_EN_SURCHAUFFE
} PoleState;

typedef struct {
    float valeurs, subsistance, parente, religion;  /* area-weighted mean */
    float D_bar_int;   /* mean pairwise content distance internally */
    float D_inf_int;   /* max pairwise content distance internally */
} CulturalProfile;

typedef struct {
    CulturalProfile profile;
    float C;               /* connectivité [0..10] */
    float SI;              /* stabilité interne [0..10] */
    float PE_interne;
    float PE_externe;
    float P_potentiel;
    float rendement;
    float P_realise;
    float Lumiere;
    float surchauffe;
    float tresor_tick;
    float croissance_tick;
    PoleState pole;
    /* Sorties du moteur vérifié scps_order (§2.4) — la vraie stabilité. */
    float fragilite;       /* part de l'ordre tenue par la contrainte [0..10] */
    float fracture;        /* sécession latente : diverse ET non consentie     */
    float dereal;          /* déréalisation (§2.3 faustien) — lue par les Âges  */
    float L;               /* légitimité pays agrégée (entrée vivante, exposée) */
    float K;               /* capacité EFFECTIVE (tech+heritage+bâti) — lue par l'IA */
    int   mode;            /* ScpsMode (stocké en int : n'expose pas scps_core) */
} CountryProsperity;

typedef struct {
    CountryProsperity country[SCPS_MAX_COUNTRY];
    int               n_countries;
    /* Coordonnées GLOBALES déplacées par les Âges (§4) — lues par le moteur, pas
     * des buffs plats : l'Âge du Commerce monte C partout (contact plus fécond),
     * l'Âge de la Brèche injecte un flux faustien mondial (la fin cosmologique). */
    float             age_C_bonus;      /* + connectivité mondiale [0..5] */
    float             age_breach_flux;  /* + flux faustien mondial → déréalisation */
    /* Âges STRUCTURELS (Lumières/Soulèvements/Ordre de Fer) — poussent les
     * ENTRÉES du moteur d'ordre ; le verdict §2.4 fait les conséquences. */
    float             age_I_bonus;      /* Lumières : surgissement des idées (+ I) */
    float             age_lumiere_solvent;/* Lumières : la légitimité COERCITIVE se dissout (− L ∝ H) */
    float             age_L_penalty;    /* Soulèvements : la légitimité ne porte plus (− L) */
    float             age_H_bonus;      /* Ordre de Fer : la poigne (+ H) */
    float             age_myth_homogen; /* Ordre de Fer : le mythe nie la diversité (− D̄ effectif) */
    /* ÂGES SANS ORDRE IMPOSÉ (2026-07-11, docs/AGES_FINS_2026-07-11.md) — les leviers
     * MONDIAUX des nouveaux âges (Échanges/Découvertes/Empires), rangés ici au même
     * titre que age_C_bonus/age_breach_flux (les Âges pouss ent des ENTRÉES du moteur,
     * jamais un bonus plat). age_P_bonus (Échanges, TRANSITOIRE — décroît comme
     * age_I_bonus) est lu AVANT la métabolisation (le P de demography_tick) ET la
     * porte de Babel (scps_prosperity.c). age_mig_mult (Échanges, TRANSITOIRE, défaut
     * 1 — décroît VERS 1, pas vers 0) multiplie demography_migration_pact_tick.
     * age_research_mult (Découvertes, TRANSITOIRE, défaut 1) multiplie le revenu de
     * recherche (ai_research_step/voie joueur). age_integration_mult (Empires,
     * PERMANENT, défaut 1 — jamais décroissant) accélère assimilation_tick (demography_tick).
     * age_tech_mask : bits (theme*8+tier) RÉELLEMENT ouverts par un âge (Société 3 —
     * Échanges, Savoir 4 — Découvertes, Société 5 — Empires, Savoir 5 — Brèche) ;
     * PERMANENT une fois posé. */
    float             age_P_bonus;
    float             age_mig_mult;
    float             age_research_mult;
    float             age_integration_mult;
    unsigned          age_tech_mask;
    /* FAU0 — FONDATIONS PARTAGÉES (faustien × capstone §27). L'ENTROPIE MONDIALE CUMULÉE :
     * toute activité faustienne l'incrémente, la décrue passive la grignote ; le capstone la
     * lira comme barre Entropie + seuil terminal. Les compteurs de conso (cachés) disent le
     * VOLUME par rare (le sélecteur d'apocalypse). L'épicentre = qui a maxé. */
    float             entropy;           /* FAU0 #1 : Σ des faust_charge régionales (barre Entropie monde) */
    double            faust_consumed[3]; /* FAU0 #3 : conso cumulée monde (0 essence · 1 flux · 2 fer céleste) */
    bool              entropy_terminal;  /* FAU0 #5 : le seuil terminal est-il franchi ? (signal pour le capstone) */
    int               entropy_epicenter; /* FAU0 #5 : région à l'entropie MAX (épicentre des apocalypses) ; -1 = aucun */
} WorldProsperity;

void prosperity_init(WorldProsperity *wp, const World *w);
void prosperity_tick(WorldProsperity *wp, const World *w,
                     const WorldEconomy *econ, const TradeNetwork *net,
                     const TechState ts[],          /* ts[SCPS_MAX_COUNTRY], can be NULL */
                     const WorldLegitimacy *wl);    /* L vivant ; NULL = tech.L de repli */
void prosperity_print_country(const WorldProsperity *wp, const World *w, int cid);
void prosperity_print_summary(const WorldProsperity *wp, const World *w);
const char *pole_state_name(PoleState s);

#endif /* SCPS_PROSPERITY_H */
