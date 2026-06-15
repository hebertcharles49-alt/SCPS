#ifndef SCPS_CREDIT_H
#define SCPS_CREDIT_H
/*
 * scps_credit.h — DETTE & PRÊTS (incrément 1, côté IA).
 *
 * On remplace le plafond artificiel « can't afford → can't act » par « tu dépenses,
 * tu t'endettes, et la dette te rattrape » : trésor NET négatif = dette, l'intérêt
 * est la rétroaction, le plafond ÉMERGE de la solvabilité (taille éco). Prêteurs
 * diégétiques : cités-états + mercantiles/pacifistes solvables. Ancré sur
 * econ_country_gold (Σ trésor des régions). La dette du JOUEUR = incrément 2.
 */
#include <stdbool.h>
#include <stdio.h>
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_legitimacy.h"

void  credit_init(void);                                       /* g_creditor[] = -1 */
int   credit_of(int c);                                        /* créancier de c (-1 = aucun) — readout/diplo */
float credit_line(const World *w, const WorldEconomy *e, int c); /* plafond ÉMERGENT (taille éco) */
bool  credit_can_spend(const WorldEconomy *e, const World *w, int c, float cost);
void  credit_spend(WorldEconomy *e, const World *w, int c, float cost);
void  credit_year_tick(WorldEconomy *e, const WorldLegitimacy *wl, const World *w);
bool  credit_save(FILE *f);
bool  credit_load(FILE *f);

#endif /* SCPS_CREDIT_H */
