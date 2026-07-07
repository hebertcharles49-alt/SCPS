#ifndef SCPS_LABOR_H
#define SCPS_LABOR_H
/*
 * scps_labor.h — utilitaires PURS de la CAPITALE (post-dissolution LaborEcon)
 *
 * Le module LaborEcon (une économie de population PARALLÈLE : LProvince,
 * mobilité de classe, marché LMarket, tick quotidien, pop_in_army) a été
 * DISSOUS. La pop est désormais UNIQUE (les strates de scps_econ) ; la levée
 * militaire LIT ces strates (scps_army/scps_warhost/scps_campaign), et la
 * topbar qui affichait le pool labor a été retirée avec l'UI SDL du viewer.
 *
 * Ne restent ici que des fonctions PURES, sans état, lues par ai/api/campaign/
 * demography/econ : le TIER de capitale (que la POP débloque) et son barème
 * (statut, défense passive, admin, logement, productivité), plus le solde
 * d'entretien de l'armée par paquet de 100. Aucune structure sérialisée
 * (la section save LABO a disparu ⇒ SAVE_VERSION 59).
 */
#include "scps_world.h"

#define POP_PER_SLOT 100   /* un paquet = 100 pop (capitale : Nobles à l'admin par 100) */

/* Classes sociales — indices ALIGNÉS sur SocialClass de scps_econ (LABORER=0,
 * ARTISAN≡BOURGEOIS=1, ELITE=2) : la levée mappe UnitDef.from sur strata[]. */
typedef enum { LAB_LABORER=0, LAB_ARTISAN, LAB_ELITE, LAB_CLASS_COUNT } LaborClass;

/* ---- La CAPITALE : la POP débloque le TIER (plafond), le barème en découle. ----
 * LOT T (2026-07-07) — SOURCE UNIQUE du tier par POP pour TOUT le moteur (readout,
 * façade/viewer, T-gate ai.c, agency) : T1 <2000 (libre, hameau de colonisation) ·
 * T2 ≥2000 · T3 ≥3000 · T4 ≥4000 · T5 ≥5000 · T6 ≥8000 · T7 ≥10000 (seuils T2-T7
 * dialables, registre J : TIER2_POP…TIER7_POP). ⚠ GRAIN : cette fonction ne sait
 * PAS elle-même si `pop` est une province ou une région — chaque appelant choisit
 * (cf. commentaires aux sites d'appel : les T-gates de construction (ai.c, agency)
 * lisent la PROVINCE-hôte, charte PROVINCE_MODEL.md ; quelques lecteurs historiques
 * région-grain — revolt/campaign/demography, hors périmètre de ce lot — restent au
 * grain RÉGION par choix de conception antérieur, documenté à part). */
int         capitale_max_tier (long pop);            /* tier autorisé : <2000→1 … 10000→7 */
const char *capitale_status   (int tier);            /* Hameau … Mégapole */
int         capitale_defense  (int tier);            /* défense passive (siège) : 1 par tier */
long        capitale_admin_pop(int tier);            /* Nobles employés à l'administration : tier·100 */
long        capitale_housing  (int tier, long admin_pop); /* logement/service : min(paquets,tier)·1000 */
float       capitale_prodmult (int tier, long admin_pop); /* productivité : 1 + 0.05·min(paquets,tier) */

/* ---- Solde d'entretien de l'armée par PAQUET de 100 (UI de recrutement) :
 * gold_x10 = or/100/jour ×10 (½ or → 5) ; food = ration/100/jour. */
void        labor_upkeep_per100(int *gold_x10, int *food);

#endif /* SCPS_LABOR_H */
