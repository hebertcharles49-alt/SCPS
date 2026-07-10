#ifndef SCPS_DECREES_H
#define SCPS_DECREES_H
/*
 * scps_decrees.h — LES ORIENTATIONS POLITIQUES DU JOUEUR (civics légers, Stellaris) +
 * les DÉCISIONS PONCTUELLES (CK3), au-dessus des verbes ponctuels (§3) et des évènements
 * RÉACTIFS (scps_events). Refonte 2026-07-10 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md,
 * sections « Orientations politiques LÉGÈRES » / « Décisions ponctuelles ») — REMPLACE les
 * 4 anciens grands décrets (levée permanente/mécénat/ambassades/politique de tribut) par
 * les 9 orientations légères + 2 décisions ponctuelles de la spec.
 *
 * RÈGLE TECHNIQUE ABSOLUE (jamais tune_set global) : chaque site de lecture MOTEUR
 * applique `tune_f("CLÉ", défaut) × decree_mult(cid, DECREE_X, mult_actif)` — le lecteur
 * FOURNIT le pays, le multiplicateur ne bouge le monde QUE pour ce pays (jamais un
 * tune_set qui changerait la valeur pour tout le monde, IA comprise). `decree_mult`
 * renvoie 1.0 si le décret est inactif OU si le trésor n'a pas pu payer LE MOIS COURANT
 * (règle joueur : « trésor insuffisant CE mois ⇒ désactivée et sans effet CE mois »).
 *
 * Deux types :
 *   ÉDIT     — permanent tant qu'actif, réversible librement (toggle on/off, decree_toggle).
 *              Les 9 orientations légères sont TOUTES des ÉDITS (jamais de Corruption,
 *              jamais irréversibles — spec : « orientations LÉGÈRES »). Deux paires
 *              MUTUELLEMENT EXCLUSIVES (radio-boutons, appliqué par decree_toggle) :
 *              RATIONS ⊥ FOYERS · CIRCULATION ⊥ FRONTIÈRES FERMÉES.
 *   DÉCISION — ponctuelle : condition d'entrée + coût UNIQUE + effet IMMÉDIAT + cooldown
 *              (jamais un état "actif" persistant — decree_toggle la REFUSE ; le tir passe
 *              par `decree_fire_decision`). Le cooldown est un ACCUMULATEUR INTER-TICKS
 *              (jurisprudence EMOB/COLC/TXYR/RVLT) → SÉRIALISÉ (section DCRE).
 *
 * Le COÛT mensuel des ÉDITS est LA contrepartie (jamais une monnaie séparée) :
 * `econ_country_tax_year(cid) × REVENUE_RATE × IPM`, prélevé /12. `decrees_tick` applique
 * ces effets MENSUELLEMENT — et SEULEMENT au joueur humain (`human_player`) : la
 * chronique (IA pure, human_player=-1) ne l'appelle jamais ⇒ golden INTACT PAR
 * CONSTRUCTION (aucune orientation ne peut jamais s'activer pour un pays IA : le bit
 * g_decree_mask d'un pays IA reste à 0 pour toute la durée du monde).
 *
 * ⚠ LA POLITIQUE DE TRIBUT SORT du catalogue (spec : « futur réglage de suzeraineté »).
 * Son levier diplo (diplo_set_tribute_decree/diplo_tribute_decree, scps_diplo.c) reste
 * INTACT mais n'est plus jamais atteint depuis ce module — le flag g_tribute_decree y
 * reste à `false` pour toujours (défaut du module diplo), ce qui EST le comportement
 * voulu (désexposé, pas supprimé).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_statecraft.h"
#include "scps_warhost.h"
#include "scps_diplo.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    /* ---- Orientations légères (ÉDIT, toutes réversibles, jamais de Corruption) ---- */
    DECREE_RATIONS = 0,      /* RATIONS MESURÉES     — FOOD_NEED↓ POP_R_BASE↓  ⊥ FOYERS */
    DECREE_FOYERS,            /* PRIMES AUX FOYERS     — POP_R_BASE↑ FOOD_NEED↑ ⊥ RATIONS */
    DECREE_ECOLES,             /* ÉCOLES SOUTENUES       — SAVOIR_W_{ELITE,BOURGEOIS,LABORER}↑ */
    DECREE_ATELIERS,           /* ATELIERS SOUTENUS      — MANUF_BUILD_COST↓ */
    DECREE_COMPTOIRS,          /* COMPTOIRS SOUTENUS     — COMMERCE_W_{BOURGEOIS,ELITE}↑ */
    DECREE_CIRCULATION,        /* CIRCULATION ENCOURAGÉE — MIG_PACT_*↑           ⊥ FRONTIÈRES */
    DECREE_FRONTIERES,         /* FRONTIÈRES FERMÉES     — MIG_PACT_*→0 COMMERCE_W_*↓ (0 or) ⊥ CIRCULATION */
    DECREE_MECENAT,            /* ex-Mécénat — RÉUTILISE le bit (spec : « aucun enum/état/save
                                 * neuf ») : affichage/flavor renommés FÊTES PUBLIQUES ; effet
                                 * REMPLACÉ (prestige → W_AGITATION_UNREST↓). */
    DECREE_LEGATIONS,          /* LÉGATIONS PERMANENTES  — Statecraft.influence +/mois, cap 100 */
    DECREE_LEVEE_ENTRETENUE,   /* LEVÉE ENTRETENUE       — verrouille la levée au plancher (0 or) */
    /* ---- Décisions ponctuelles (DCR_DECISION, hors modèle toggle) ---- */
    DECISION_AUDIT_OFFICES,    /* AUDIT DES OFFICES — condition Corruption≥seuil, coût ponctuel,
                                 * cooldown, effet immédiat (faction_audit + L capitale). */
    DECREE_COUNT
} DecreeId;

typedef enum { DCR_EDIT=0, DCR_REFORME=1, DCR_POSTURE=2, DCR_DECISION=3 } DecreeType;

/* condition d'entrée : lit l'état réel, jamais un flag gratuit. */
typedef bool (*DecreeCond)(const World *w, const WorldEconomy *econ, const TechState *ts,
                           const WorldLegitimacy *wl, const Statecraft *sc,
                           const DiploState *dp, int cid);

typedef struct {
    const char *nom;            /* nom diégétique (FR, littéral — hors table STR_*, module hors membrane UI stricte) */
    const char *flavor;         /* une ligne cynique */
    const char *plateaux;       /* description des DEUX plateaux (le gain / la contrepartie) */
    DecreeType  type;
    DecreeCond  cond;           /* condition d'entrée ; NULL = toujours permis */
} DecreeDef;

extern const DecreeDef DECREES[DECREE_COUNT];

/* état par pays (bitmask, 1 bit/décret) — persistant (section DCRE du save). Les IDs
 * DCR_DECISION (DECISION_AUDIT_OFFICES) n'y possèdent JAMAIS de bit (decree_toggle les
 * refuse) : leur cooldown vit dans g_audit_cd, sérialisé séparément. */
extern uint32_t g_decree_mask[SCPS_MAX_COUNTRY];

bool decree_active(int cid, DecreeId id);
/* bascule (ÉDIT/POSTURE : libre — auto-exclusif sur les deux paires RATIONS/FOYERS et
 * CIRCULATION/FRONTIÈRES, activer l'une désactive l'autre ; RÉFORME : refuse le retour
 * arrière ; DÉCISION : no-op, cf. decree_fire_decision). */
void decree_toggle(int cid, DecreeId id, bool on);
/* la condition d'entrée est-elle remplie MAINTENANT ? (pour griser le bouton). */
bool decree_legal(const World *w, const WorldEconomy *econ, const TechState *ts,
                  const WorldLegitimacy *wl, const Statecraft *sc,
                  const DiploState *dp, int cid, DecreeId id);

/* Le décret `id` est-il ACTIF **et** FINANCÉ ce mois-ci (le trésor a pu payer le coût
 * intégral) — jamais un état à moitié payé. `mult_actif` est le multiplicateur QUAND
 * effectif ; sinon 1.0 (identité, aucun effet). Usage AUX SITES DE LECTURE MOTEUR :
 *   tune_f("FOOD_NEED", 1.0f) * decree_mult(cid, DECREE_RATIONS, 0.95f)
 *                             * decree_mult(cid, DECREE_FOYERS,  1.04f);
 * JAMAIS de tune_set : le pays FOURNI par l'appelant scope l'effet. */
float decree_mult(int cid, DecreeId id, float mult_actif);

/* ---- Composites PAR SITE (une orientation touche parfois PLUSIEURS clés à des
 * multiplicateurs DIFFÉRENTS — ces helpers combinent les décrets pertinents pour
 * un site de lecture donné ; 1.0 si aucun n'est actif/financé). ---------------- */
float decree_food_need_mult   (int cid);  /* RATIONS ×0.95 · FOYERS ×1.04 */
float decree_pop_r_base_mult  (int cid);  /* RATIONS ×0.97 · FOYERS ×1.05 */
float decree_savoir_w_mult    (int cid);  /* ÉCOLES ×1.05 */
float decree_manuf_cost_mult  (int cid);  /* ATELIERS ×0.95 */
float decree_commerce_w_mult  (int cid);  /* COMPTOIRS ×1.05 · FRONTIÈRES ×0.95 */
/* MIG_PACT — le flux d'un pacte A↔B = base × decree_mig_pact_mult(A) × decree_mig_pact_mult(B)
 * (CIRCULATION ×1.10, FRONTIÈRES ×0 — ne bloque QUE le pacte, jamais les réfugiés :
 * demography_refugee_tick est un chemin séparé, jamais touché par ce module). */
float decree_mig_pact_mult    (int cid);
float decree_unrest_mult      (int cid);  /* FÊTES (ex-DECREE_MECENAT) ×0.95 */

/* Applique les décrets ACTIFS du joueur pour le mois écoulé (prélève le coût mensuel de
 * chaque ÉDIT actif — tout ou rien — et pose le flag "financé" du mois ; +applique les
 * effets ADDITIFS d'état — LÉGATIONS/influence, LEVÉE/plancher). Appelé UNIQUEMENT pour
 * cid==human_player (le seul rôle humain) — la chronique headless ne l'appelle jamais. */
void decrees_tick(World *w, WorldEconomy *econ, Statecraft *sc, WorldLegitimacy *wl,
                  WarHost *host, DiploState *dp, int cid, int days);

/* DÉCISION PONCTUELLE — tir immédiat (hors du cycle mensuel) : revalide condition +
 * cooldown, prélève le coût ponctuel (ce qui est disponible, jamais en négatif imposé),
 * applique l'effet, pose le cooldown. Renvoie false si illégal (rien n'est modifié). */
bool decree_fire_decision(World *w, WorldEconomy *econ, WorldLegitimacy *wl, int cid, DecreeId id);

/* section DCRE du save (motif WILD/CULT : sim_wild_save/load). */
#include <stdio.h>
void decrees_save(FILE *f);
bool decrees_load(FILE *f);
void decrees_reset(void);   /* RAZ (nouvelle partie) */

#endif /* SCPS_DECREES_H */
