/*
 * scps_endgame.c — Capstone §27 : Entropie mondiale + 4 fins + Merveille.
 *
 * Ce fichier inclut scps_core.h. Il N'EST JAMAIS inclus par viewer.c.
 *
 * Séquence du tick orchestrateur :
 *   C1 : élargir l'entropie (tech faustienne)
 *   C2 : select_and_fire si !fired
 *   C3-C5 : dérouler la fin latchée (eau / froid / ronces)
 *   C6 : wonder_tick (peut déclencher FIN_ASCENSION)
 *
 * C0 : squelette vide — infrastructure (init + stub tick).
 */
#include "scps_endgame.h"
#include "scps_tune.h"
#include <string.h>

void endgame_init(EndgameState *eg) {
    memset(eg, 0, sizeof *eg);
    eg->epicenter_reg   = -1;
    eg->fauteur_country = -1;
    eg->fin_year        = -1;
    eg->merv_country    = -1;
    eg->merv_site_reg   = -1;
}

void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  int player, int year) {
    (void)w; (void)econ; (void)wp; (void)ts;
    (void)rn; (void)navy; (void)dp; (void)player; (void)year;
    /* C0 : corps vide — le tick est accroché mais ne fait RIEN.
     * C1 élargira l'entropie (ENTROPY_TECH_W × Σts[c].charge).
     * C2 ajoutera select_and_fire.
     * C3-C6 ajouteront les effets de chaque fin. */
    if (eg->fired) return;
}
