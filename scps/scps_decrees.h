#ifndef SCPS_DECREES_H
#define SCPS_DECREES_H
/*
 * scps_decrees.h — LES DÉCRETS DU JOUEUR (civics Stellaris / décisions CK3) :
 * la flexibilité PROACTIVE, au-dessus des verbes ponctuels (§3) et des
 * évènements RÉACTIFS (scps_events). Un décret DÉPLACE un levier existant du
 * moteur, tant qu'il est ACTIF — jamais un bonus plat, jamais un système neuf.
 *
 * Trois types :
 *   ÉDIT    — permanent tant qu'actif, réversible librement (toggle on/off).
 *   RÉFORME — bascule UNE fois, souvent IRRÉVERSIBLE (decree_toggle refuse le
 *             retour arrière : un désengagement rendrait le monde incohérent —
 *             p.ex. la levée en masse ne "dé-conscrit" pas une génération).
 *   POSTURE — réversible librement (alias d'ÉDIT côté code : la distinction
 *             est narrative — "posture" = sans condition d'entrée forte).
 *
 * Le COÛT est la CONTREPARTIE (jamais une monnaie séparée) : chaque décret qui
 * donne quelque chose (+prestige, +influence, levée verrouillée haute) prend
 * quelque chose de RÉEL par les entrées existantes (ponction treasury, grief
 * vassal, bras immobilisés). `decrees_tick` applique ces effets MENSUELLEMENT
 * — et SEULEMENT au joueur humain (`human_player`) : la chronique (IA pure,
 * human_player=-1) ne l'appelle jamais ⇒ golden INTACT par construction.
 *
 * Portes d'entrée = CONDITIONS lues sur l'état réel (tech/légitimité/trésor/
 * vassaux), jamais une case à cocher gratuite.
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
    DECREE_LEVEE_PERMANENTE = 0,   /* ÉDIT   — verrouille la levée haute (WH_LEVY_GUERRE) */
    DECREE_MECENAT,                /* ÉDIT   — +prestige/mois, ponction treasury */
    DECREE_AMBASSADES,             /* ÉDIT   — +influence/mois, ponction treasury (moindre) */
    DECREE_TRIBUT,                 /* RÉFORME (irréversible) — ×1.5 la contribution vassale, +grief */
    DECREE_COUNT
} DecreeId;

typedef enum { DCR_EDIT=0, DCR_REFORME=1, DCR_POSTURE=2 } DecreeType;

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

/* état par pays (bitmask, 1 bit/décret) — persistant (section DCRE du save). */
extern uint32_t g_decree_mask[SCPS_MAX_COUNTRY];

bool decree_active(int cid, DecreeId id);
/* bascule (édit/posture : libre ; réforme : REFUSE le retour arrière une fois active). */
void decree_toggle(int cid, DecreeId id, bool on);
/* la condition d'entrée est-elle remplie MAINTENANT ? (pour griser le bouton). */
bool decree_legal(const World *w, const WorldEconomy *econ, const TechState *ts,
                  const WorldLegitimacy *wl, const Statecraft *sc,
                  const DiploState *dp, int cid, DecreeId id);

/* Applique les décrets ACTIFS du joueur pour le mois écoulé (contrepartie +
 * effet). Appelé UNIQUEMENT pour cid==human_player (le seul rôle humain) —
 * la chronique headless n'appelle jamais cette fonction. */
void decrees_tick(World *w, WorldEconomy *econ, Statecraft *sc, WorldLegitimacy *wl,
                  WarHost *host, DiploState *dp, int cid, int days);

/* section DCRE du save (motif WILD/CULT : sim_wild_save/load). */
#include <stdio.h>
void decrees_save(FILE *f);
bool decrees_load(FILE *f);
void decrees_reset(void);   /* RAZ (nouvelle partie) */

#endif /* SCPS_DECREES_H */
