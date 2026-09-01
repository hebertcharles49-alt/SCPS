#ifndef SCPS_INFLUENCE_H
#define SCPS_INFLUENCE_H
/*
 * scps_influence.h — L'INFLUENCE POLITIQUE (docs/DESIGN_MISSIONS_DOCTRINES.md §3).
 *
 * La monnaie du jeu politique, ENDOGÈNE (dérivée des pops SIMULÉES — la correction
 * EU5 du point de monarque EU4 tombé du ciel) : générée par les élites du pays × le
 * niveau du Conseil, dépensée sur les verbes diplomatiques du joueur (le coût
 * REMPLACE le cooldown de l'émissaire, scps_sim.c : CMD_OFFER_ALLIANCE/PACT/
 * MIGRATION, CMD_EMBARGO, CMD_PEACE_OFFER, CMD_FABRICATE_CB). Plus tard : pivots de Dessein,
 * doctrines (hors périmètre P1).
 *
 * P1 — JOUEUR SEUL (`human_player`) : génération ET dépense gatées côté appelant
 * (motif décrets, scps_sim.c:1143) ⇒ golden intact PAR CONSTRUCTION (la chronique,
 * human_player=-1, n'appelle jamais influence_tick). Accumulateur INTER-TICKS par
 * pays ⇒ SÉRIALISÉ (section INFL, save v104, jurisprudence EMOB/COLC/TXYR).
 */
#include "scps_world.h"      /* SCPS_MAX_COUNTRY */
#include "scps_econ.h"       /* WorldEconomy, CLASS_ELITE, prov[] — LA VÉRITÉ éco (doctrine province) */
#include "scps_statecraft.h" /* Statecraft, statecraft_council_seated/_gen, cand_tier (I=1..III=3) */

typedef struct {
    float influence[SCPS_MAX_COUNTRY];   /* l'accumulateur, PAR PAYS — jamais de plafond dur par défaut */
} InfluenceState;

void influence_init(InfluenceState *is);   /* RAZ (0 par pays — genèse/nouvelle partie) */

/* Génération MENSUELLE — appelée UNE fois pour `cid` (l'appelant gate human_player>=0,
 * motif décrets) :
 *   gain/mois = INFLUENCE_PER_NOBLE × élites(cid) × mult_conseil
 * élites = influence_elites (somme PROVINCE, jamais region[].pop). mult_conseil =
 * influence_council_mult (rang moyen des sièges POURVUS, plancher INFLUENCE_COUNCIL_FLOOR
 * si aucun siège pourvu — sinon un Conseil vide rend le joueur muet en diplomatie, choix
 * signalé). Clampe à INFLUENCE_CAP si >0 (0 = sans plafond, décision joueur 2026-09-01). */
void influence_tick(InfluenceState *is, const World *w, const WorldEconomy *econ,
                     const Statecraft *sc, uint32_t seed, int cid);

float influence_get(const InfluenceState *is, int cid);
/* épsilon 0.001 : tolère l'arrondi flottant d'un coût pile au centime du stock. */
int   influence_can_spend(const InfluenceState *is, int cid, float cost);
/* dépense INCONDITIONNELLE (l'appelant a déjà vérifié influence_can_spend) — clampe à
 * 0 (ne descend jamais négatif, motif econ_flux/credit). */
void  influence_spend(InfluenceState *is, int cid, float cost);

/* La moyenne de RANG (I=1..III=3, statecraft_council_cand_tier) des sièges du Conseil
 * EN SIÈGE — plancher INFLUENCE_COUNCIL_FLOOR si aucun siège pourvu. `out_n_seated`
 * (peut être NULL) reçoit le nombre de sièges pourvus (0 = plancher appliqué), pour que
 * la membrane façade distingue « aucun ministre » de « rang moyen I ». */
float influence_council_mult(const Statecraft *sc, uint32_t seed, int cid, int *out_n_seated);

/* Effectif national de la classe ÉLITE — somme des PROVINCES au pays (prov[], JAMAIS
 * region[].pop, miroir stale — doctrine « la province est la seule réalité économique »,
 * CLAUDE.md). */
double influence_elites(const WorldEconomy *econ, int cid);

#endif /* SCPS_INFLUENCE_H */
