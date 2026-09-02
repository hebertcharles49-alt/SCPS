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

typedef struct InfluenceState {          /* tag nommé : scps_missions.h le forward-déclare (hook pivot) */
    float influence[SCPS_MAX_COUNTRY];   /* l'accumulateur, PAR PAYS — jamais de plafond dur par défaut */
} InfluenceState;

void influence_init(InfluenceState *is);   /* RAZ (0 par pays — genèse/nouvelle partie) */

/* ── L'ASSIETTE (le COURANT politique la RE-SIED, §4.3bis) ────────────────
 * Le courant adopté (doctrines Aristocratie/Bourgeoisie/Populaire/Divin) déplace
 * la BASE de génération sur SA classe. Aucun courant ⇒ l'assiette par défaut
 * (élites × INFLUENCE_PER_NOBLE). Enum PROPRE au module : scps_influence ne
 * connaît pas les doctrines (l'appelant fait la traduction DoctrineId → base),
 * ce qui garde les deux modules indépendants. */
typedef enum {
    INFL_BASE_DEFAUT = 0,   /* élites × INFLUENCE_PER_NOBLE (0.002) */
    INFL_BASE_ARISTO,       /* élites × INFLUENCE_PER_NOBLE_ARISTO (0.0025) */
    INFL_BASE_BOURGEOIS,    /* bourgeois × INFLUENCE_PER_BOURGEOIS (0.0006) */
    INFL_BASE_LABORER,      /* journaliers × INFLUENCE_PER_LABORER (0.00012) */
    INFL_BASE_FAITH         /* Σ foi bâtie × (1+ferveur moyenne) × INFLUENCE_PER_FAITH (0.08) */
} InfluenceBase;

/* Le GAIN MENSUEL AVANT le multiplicateur du Conseil — l'assiette du courant.
 * Source UNIQUE : influence_tick l'appelle, et la façade (scps_influence_info)
 * aussi, pour que le nombre affiché soit CELUI qui sera crédité. */
double influence_base_gain(const WorldEconomy *econ, int cid, InfluenceBase base);
/* L'EFFECTIF de l'assiette (nobles, bourgeois, journaliers — ou la foi bâtie
 * arrondie), pour le hover en MOTS. */
double influence_base_pop(const WorldEconomy *econ, int cid, InfluenceBase base);

/* ── L'ÉCHELLE D'ASSIETTE (décision joueur 2026-09-02) ────────────────────
 * Combien de fois l'assiette de RÉFÉRENCE ce pays pèse-t-il ?
 *     é = influence_base_gain(econ, cid, base) / INFLUENCE_BASE_REF   (plancher 0.25)
 * Elle LINÉARISE les dépenses politiques sur la population : un empire deux
 * fois plus noble gagne deux fois plus ET paie deux fois plus — le temps
 * d'acquisition d'une doctrine est le MÊME à toute échelle, le joueur reste
 * libre de faire grandir sa noblesse sans que ça brade l'arbre.
 * ⚠ L'ÉCHELLE SE CALCULE SUR L'ASSIETTE SEULE, JAMAIS × le rang du Conseil :
 * sinon accumuler à Conseil plein puis RENVOYER ses ministres brade tous les
 * prix — l'exploit exact que ce choix évite.
 * INFLUENCE_BASE_REF = 0 ⇒ é ≡ 1.0 (kill-switch : prix plats d'avant). */
float influence_scale(const WorldEconomy *econ, int cid, InfluenceBase base);

/* Génération MENSUELLE — appelée UNE fois pour `cid` (l'appelant gate human_player>=0,
 * motif décrets) :
 *   gain/mois = influence_base_gain(econ, cid, base) × mult_conseil
 * L'assiette par défaut = élites (somme PROVINCE, jamais region[].pop). mult_conseil =
 * influence_council_mult (rang moyen des sièges POURVUS, plancher INFLUENCE_COUNCIL_FLOOR
 * si aucun siège pourvu — sinon un Conseil vide rend le joueur muet en diplomatie, choix
 * signalé). Clampe à INFLUENCE_CAP si >0 (0 = sans plafond, décision joueur 2026-09-01). */
void influence_tick(InfluenceState *is, const World *w, const WorldEconomy *econ,
                     const Statecraft *sc, uint32_t seed, int cid, InfluenceBase base);

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

/* TÉLÉMÉTRIE (print-only, chronicle) — Σ influence GÉNÉRÉE depuis la genèse de cette
 * sim (statique de module, RAZ à influence_init, JAMAIS sérialisée — motif
 * econ_colony_stats). N'entre dans AUCUN calcul moteur. */
void influence_stats_get(double *generated);

/* Effectif national de la classe ÉLITE — somme des PROVINCES au pays (prov[], JAMAIS
 * region[].pop, miroir stale — doctrine « la province est la seule réalité économique »,
 * CLAUDE.md). */
double influence_elites(const WorldEconomy *econ, int cid);

#endif /* SCPS_INFLUENCE_H */
