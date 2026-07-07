/*
 * scps_labor.c — utilitaires PURS de la CAPITALE (voir scps_labor.h).
 *
 * Le module LaborEcon (LProvince, mobilité de classe, marché LMarket, tick
 * quotidien, colonisation, matériau, pop_in_army) a été DISSOUS : la pop est
 * UNIQUE (les strates de scps_econ), la levée militaire LIT ces strates. Ne
 * restent que ces fonctions PURES sans état, lues par ai/api/campaign/
 * demography/econ. Aucune structure sérialisée (section save LABO retirée).
 */
#include "scps_labor.h"
#include "scps_tune.h"   /* LOT T — seuils T2-T7 dialables (registre J) */

#define CAP_ADMIN_PER_TIER 100    /* pop de Nobles à l'administration, par tier */
#define CAP_PROD_PER_TIER  0.05f  /* +5 % de productivité par tier SERVI */
#define SOLDE_GOLD_PER100  0.5f   /* solde d'armée : ½ or / 100 enrôlés / jour */
#define SOLDE_FOOD_PER100  1L     /* ration de campagne : 1 nourriture / 100 / jour */

/* LOT T (2026-07-07) — SOURCE UNIQUE du tier par POP (doctrine joueur : « T2 2000, T3
 * 3000, T4 4000, T5 5000 et ainsi de suite » — CE barème existait déjà, à l'identique,
 * depuis avant ce lot ; ce qui manquait était que TOUS les lecteurs (readout, façade,
 * viewer, T-gate ai.c) s'y réfèrent au lieu de barèmes ad hoc dupliqués ailleurs — cf.
 * TROUVAILLES). Seuils dialables (registre J), mais mis en cache à la PREMIÈRE lecture
 * (pas un tune_f() par appel) : cette fonction est appelée dans des boucles PAR-PROVINCE
 * PAR-TICK (scps_econ.c, jusqu'à SCPS_MAX_PROV fois/jour) — un lookup registre (linéaire,
 * ~200 noms) par appel coûterait cher sur tout un chronicle. Conséquence ASSUMÉE : une
 * surcharge SCPS_TUNE au lancement s'applique (lue au 1er appel) ; le panneau F10 EN
 * DIRECT ne rafraîchit PAS ces 6 seuils précis sans relancer (documenté, cf. TROUVAILLES).
 */
static long g_tier_pop[6];    /* [0]=T2 … [5]=T7 */
static int  g_tier_pop_init = 0;
static void tier_pop_init(void){
    if (g_tier_pop_init) return;
    g_tier_pop_init = 1;
    g_tier_pop[0] = (long)tune_f("TIER2_POP",  2000.f);
    g_tier_pop[1] = (long)tune_f("TIER3_POP",  3000.f);
    g_tier_pop[2] = (long)tune_f("TIER4_POP",  4000.f);
    g_tier_pop[3] = (long)tune_f("TIER5_POP",  5000.f);
    g_tier_pop[4] = (long)tune_f("TIER6_POP",  8000.f);
    g_tier_pop[5] = (long)tune_f("TIER7_POP", 10000.f);
}
/* Tier que la POPULATION débloque (plafond) — la pop OUVRE, la recette PAIE. */
int capitale_max_tier(long pop){
    tier_pop_init();
    if (pop>=g_tier_pop[5]) return 7;
    if (pop>=g_tier_pop[4]) return 6;
    if (pop>=g_tier_pop[3]) return 5;
    if (pop>=g_tier_pop[2]) return 4;
    if (pop>=g_tier_pop[1]) return 3;
    if (pop>=g_tier_pop[0]) return 2;
    return 1;                       /* toute province : tier 1 dès la fondation */
}
/* Le STATUT d'urbanisation VIENT DU TIER. */
const char *capitale_status(int tier){
    static const char *S[8]={ "Hameau","Hameau","Village","Bourg","Ville","Cité","Métropole","Mégapole" };
    if (tier<1) tier=1;
    if (tier>7) tier=7;
    return S[tier];
}
/* DÉFENSE provinciale passive : un niveau par tier (allonge le SIÈGE). */
int capitale_defense(int tier){ return tier<0 ? 0 : tier; }
long capitale_admin_pop(int tier){ return (long)tier * CAP_ADMIN_PER_TIER; }
/* Logement/service délivré : min(paquets de Nobles en poste, tier) × 1000 (gaté). */
long capitale_housing(int tier, long admin_pop){
    long packs = admin_pop / POP_PER_SLOT;
    long actifs = packs < tier ? packs : tier;
    if (actifs<0) actifs=0;
    return actifs * 1000;
}
float capitale_prodmult(int tier, long admin_pop){
    long packs = admin_pop / POP_PER_SLOT;
    long actifs = packs < tier ? packs : tier;
    if (actifs<0) actifs=0;
    return 1.f + CAP_PROD_PER_TIER * (float)actifs;
}
/* Solde d'entretien par PAQUET de 100 (uniforme) — pour l'UI de recrutement. */
void labor_upkeep_per100(int *gold_x10, int *food){
    if (gold_x10) *gold_x10 = (int)(SOLDE_GOLD_PER100*10.f + 0.5f);
    if (food)     *food     = (int)SOLDE_FOOD_PER100;
}
