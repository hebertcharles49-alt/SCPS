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

#define CAP_ADMIN_PER_TIER 100    /* pop de Nobles à l'administration, par tier */
#define CAP_PROD_PER_TIER  0.05f  /* +5 % de productivité par tier SERVI */
#define SOLDE_GOLD_PER100  0.5f   /* solde d'armée : ½ or / 100 enrôlés / jour */
#define SOLDE_FOOD_PER100  1L     /* ration de campagne : 1 nourriture / 100 / jour */

/* Tier que la POPULATION débloque (plafond) — la pop OUVRE, la recette PAIE. */
int capitale_max_tier(long pop){
    if (pop>=10000) return 7;
    if (pop>= 8000) return 6;
    if (pop>= 5000) return 5;
    if (pop>= 4000) return 4;
    if (pop>= 3000) return 3;
    if (pop>= 2000) return 2;
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
