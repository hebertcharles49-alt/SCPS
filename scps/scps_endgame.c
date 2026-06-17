/*
 * scps_endgame.c — Capstone §27 : Entropie mondiale + 4 fins + Merveille.
 *
 * Ce fichier inclut scps_core.h indirectement (non) — il lit des structs moteur
 * (econ/prosperity/tech) mais reste autonome. Il N'EST JAMAIS inclus par viewer.c.
 *
 * Séquence du tick orchestrateur :
 *   C1 : élargir l'entropie (charge de tech faustienne — le SAVOIR)
 *   C2 : select_and_fire si !fired (seuil ENTROPY_FIN → latch d'une fin)
 *   C3-C5 : dérouler la fin latchée (eau / froid / ronces) — modules suivants
 *   C6 : wonder_tick (peut déclencher FIN_ASCENSION) — module suivant
 */
#include "scps_endgame.h"
#include "scps_tune.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>   /* getenv : diag de calibration (SCPS_ENTDIAG), OFF par défaut */

void endgame_init(EndgameState *eg) {
    memset(eg, 0, sizeof *eg);
    eg->epicenter_reg   = -1;
    eg->fauteur_country = -1;
    eg->fin_year        = -1;
    eg->merv_country    = -1;
    eg->merv_site_reg   = -1;
}

/* ── C1 — élargir la source d'entropie ────────────────────────────────────────
 * prosperity_tick a posé wp->entropy = Σ faust_charge régional (la transgression
 * ACTIVE des transmuteurs ; plate à 0 en monde stable). On AJOUTE la composante
 * « pression du savoir » : la charge de TECH faustienne accumulée par empire
 * (celle qui ramène déjà l'Âge de la Brèche). Recalculé à neuf chaque tick
 * (prosperity_tick réassigne wp->entropy = esum) → l'ajout est NON cumulatif
 * inter-ticks. C'est une vraie ENTRÉE moteur (la charge), jamais un bonus plat. */
static void endgame_entropy_widen(WorldProsperity *wp, const TechState ts[], int nc) {
    if (!wp || !ts) return;
    float tech_ent = 0.f;
    for (int c = 0; c < nc && c < SCPS_MAX_COUNTRY; c++) tech_ent += ts[c].charge;
    wp->entropy += tune_f("ENTROPY_TECH_W", 1.0f) * tech_ent;
}

/* Foyer de REPLI : si aucune région ne porte de faust_charge (entropie TECH-driven
 * pure → entropy_epicenter == -1), le foyer suit l'empire le PLUS faustien (max
 * ts[].charge) et sa capitale. Renvoie le pays fauteur ; pose *out_reg = sa région
 * capitale (ou -1). */
static int endgame_pick_fauteur(const World *w, const TechState ts[], int *out_reg) {
    int best = -1; float bestc = -1.f;
    for (int c = 0; c < w->n_countries && c < SCPS_MAX_COUNTRY; c++) {
        if (w->country[c].role == POLITY_UNCLAIMED) continue;
        if (ts[c].charge > bestc) { bestc = ts[c].charge; best = c; }
    }
    *out_reg = -1;
    if (best >= 0) {
        int cap = w->country[best].capital_prov;
        if (cap >= 0 && cap < w->n_provinces) *out_reg = w->province[cap].region;
    }
    return best;
}

/* ── C2 — sélecteur + déclencheur (latch : un seul déclenchement) ──────────── */
static void endgame_select_and_fire(EndgameState *eg, const World *w,
                                     const WorldEconomy *econ, const WorldProsperity *wp,
                                     const TechState ts[], int year) {
    if (eg->fired) return;
    if (wp->entropy < tune_f("ENTROPY_FIN", 50.f)) return;

    /* Override MERVEILLE (priorité) : l'Ascension a été menée à bout → c'est ELLE. */
    if (eg->merv == MERV_ASCENDED) {
        eg->fired = true; eg->fin = FIN_ASCENSION; eg->fin_year = year;
        return;
    }

    /* Foyer FIGÉ : l'épicentre régional (région la plus saturée), sinon — entropie
     * tech-driven pure — l'empire le plus faustien et sa capitale. */
    int epi = wp->entropy_epicenter;
    int fauteur = (epi >= 0 && epi < econ->n_regions) ? econ->region[epi].owner : -1;
    if (epi < 0 || fauteur < 0) {
        int fr = -1; int f = endgame_pick_fauteur(w, ts, &fr);
        if (epi < 0)     epi = fr;
        if (fauteur < 0) fauteur = f;
    }
    eg->epicenter_reg   = epi;
    eg->fauteur_country = fauteur;
    eg->fin_year        = year;

    /* Sélecteur par compteur DOMINANT de conso de rare (les transmuteurs) :
     * 0 (essence) → EAU · 1 (flux) → RONCES · 2 (fer céleste) → FROID. */
    int k = 0; double mx = wp->faust_consumed[0];
    for (int i = 1; i < 3; i++) if (wp->faust_consumed[i] > mx) { mx = wp->faust_consumed[i]; k = i; }
    if (mx < 1.0) {
        /* Aucun transmuteur : le SAVOIR faustien seul a mené à la Brèche → la FORME
         * de l'apocalypse suit la signature (déterministe) de l'empire fauteur, pour
         * ne pas figer le monde sur une seule fin. */
        uint32_t h = (uint32_t)((eg->fauteur_country + 1) * 2654435761u)
                   ^ (uint32_t)((eg->epicenter_reg + 1) * 40503u);
        k = (int)(h % 3u);
    }
    static const FinType MAP[3] = { FIN_EAU, FIN_RONCES, FIN_FROID };
    eg->fin   = MAP[k];
    eg->fired = true;
    /* Amorçage spécifique (rift d'eau C3 / front de ronces C5) — ajouté à ces modules. */
}

void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  int player, int year) {
    (void)rn; (void)navy; (void)dp; (void)player;
    if (!eg || !wp) return;

    /* C1 — élargir l'entropie (le savoir faustien). */
    endgame_entropy_widen(wp, ts, w ? w->n_countries : 0);

    /* Diag de CALIBRATION (SCPS_ENTDIAG=1) : la courbe d'entropie année par année,
     * pour viser le déclenchement ~an 180 (100 ans sous le seuil). OFF par défaut. */
    if (getenv("SCPS_ENTDIAG"))
        fprintf(stderr, "[ENTDIAG] an %d : entropie %.1f / fin %.0f%s\n",
                year, (double)wp->entropy, (double)tune_f("ENTROPY_FIN", 50.f),
                eg->fired ? " [DÉCLENCHÉE]" : "");

    /* C2 — pas encore déclenché : tester le seuil combiné et latcher une fin. */
    if (!eg->fired) {
        if (w && econ && ts) endgame_select_and_fire(eg, w, econ, wp, ts, year);
        return;
    }

    /* C3-C6 — dérouler la fin latchée (eau/froid/ronces, merveille) :
     * ajouté aux modules suivants. */
}
