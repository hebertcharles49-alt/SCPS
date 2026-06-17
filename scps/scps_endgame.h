#ifndef SCPS_ENDGAME_H
#define SCPS_ENDGAME_H
/*
 * scps_endgame.h — Capstone §27 : Entropie mondiale + 4 fins + Merveille.
 *
 * Ce module appartient au MOTEUR (jamais inclus par viewer.c).
 * Communication avec le renderer : EndgameReadout (scps_readout.h).
 *
 * La struct EndgameState est PLATE (aucun pointeur) : compatible sv_w.
 */
#include <stdint.h>
#include <stdbool.h>
#include "scps_types.h"
#include "scps_econ.h"
#include "scps_prosperity.h"
#include "scps_tech.h"
#include "scps_trade.h"
#include "scps_navy.h"
#include "scps_diplo.h"

/* ---- Tailles (capacités — PAS dans le registre J) --------------------- */
#define SCPS_THORN_FRONT_MAX 16384  /* cellules max dans le front BFS des ronces */

/* ---- Enums ------------------------------------------------------------ */
typedef enum {
    FIN_AUCUNE = 0, FIN_EAU, FIN_FROID, FIN_RONCES, FIN_ASCENSION
} FinType;

typedef enum {
    MERV_NONE = 0,
    MERV_FORGE, MERV_FORGE_DONE,
    MERV_SOCIETE, MERV_SOCIETE_DONE,
    MERV_SAVOIR, MERV_SAVOIR_DONE,
    MERV_ASCENDED
} MervPhase;

/* ---- État cataclysme (struct PLATE — sans pointeur pour sv_w) --------- */
typedef struct {
    FinType  fin;
    bool     fired;                           /* latch : un seul déclenchement */

    int  epicenter_reg;                       /* région-foyer (figée au fire) */
    int  fauteur_country;                     /* empire fauteur (figé) */
    int  fin_year;                            /* année du déclenchement */

    uint8_t  sunken[SCPS_MAX_REG];           /* EAU : bitmap des régions englouties */
    int      n_sunken;                        /* nb régions réellement englouties */
    int      sink_pending;                    /* régions encore à engloutir */

    float    cold_offset;                     /* FROID : décalage cumulé (plafond) */

    int      thorn_front[SCPS_THORN_FRONT_MAX]; /* RONCES : front BFS cellules */
    int      thorn_front_n;                   /* taille du front actuel */
    uint32_t thorn_rng;                       /* RNG dédié (déterminisme) */

    MervPhase merv;                           /* MERVEILLE : phase courante */
    int       merv_country;                   /* empire qui bâtit */
    int       merv_site_reg;                  /* région-chantier */
    float     merv_progress;                  /* progression [0..1] du palier courant */
} EndgameState;

/* ---- API -------------------------------------------------------------- */
void endgame_init(EndgameState *eg);

/* Tick orchestrateur : appelé UNE fois par an (après prosperity_tick,
 * avant world_tick/diplo/factions) depuis sim_step / chronicle. */
void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  int player, int year);

#endif /* SCPS_ENDGAME_H */
