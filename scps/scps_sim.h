#ifndef SCPS_SIM_H
#define SCPS_SIM_H
/*
 * scps_sim — LE TICK DE JEU PARTAGÉ (extrait de chronicle.c, À L'IDENTIQUE).
 *
 * `sim_day` est la boucle de jeu PLEINE : agency · IA · events · économie ·
 * statecraft · démographie · navy · révolte · world_tick · légitimité · commerce ·
 * intertrade · contact · prospérité · endgame · warhost · campagne · diplo · crédit ·
 * missions · factions. La chronique (headless) la roulait déjà ; ce module la rend
 * LITTÉRALEMENT commune, pour que la façade scps_api (Godot) avance EXACTEMENT le
 * même tick déterministe — fin des « zéros an-0 » (la colonne éco seule).
 *
 * DÉTERMINISME : le code est déplacé VERBATIM depuis chronicle.c — le hash de la
 * chronique NE BOUGE PAS (`make determinism` le prouve). chronicle.c inclut ce
 * header et garde sa boucle/sa télémétrie ; seules les définitions de tick migrent.
 *
 * ⚠ UN SEUL Sim ACTIF À LA FOIS PAR PROCESSUS : intertrade (embargos/flux),
 * faction_levers et econ_set_arms_pump portent un état GLOBAL remis à plat par
 * sim_init ; deux sims concurrents se marcheraient dessus. La chronique (sims
 * séquentiels) et la façade (un monde) respectent cette règle.
 */
#include "scps_tune.h"
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_statecraft.h"
#include "scps_agency.h"
#include "scps_credit.h"
#include "scps_routes.h"
#include "scps_intertrade.h"
#include "scps_warhost.h"
#include "scps_campaign.h"
#include "scps_navy.h"
#include "scps_diplo.h"
#include "scps_endgame.h"
#include "scps_events.h"
#include "scps_modifier.h"
#include "scps_demography.h"
#include "scps_revolt.h"
#include "scps_missions.h"
#include "scps_factions.h"
#include "scps_labor.h"
#include "scps_ai.h"
#include "scps_species.h"
#include <stdbool.h>

/* ---- JOURNAL DE COMMANDES JOUEUR (déterministe) -------------------------
 * Les ordres du joueur (la façade Godot) ne s'appliquent PAS à l'instant de
 * l'appel (hors-tick, en temps réel) : ils sont ENFILÉS, puis VIDÉS à un point
 * FIXE du tick (après agency_advance, AVANT l'IA), chacun REVALIDÉ contre l'état
 * courant (miroir de save_sane : un index périmé est ignoré, jamais déréférencé).
 * C'est la base de la réplicabilité : une partie = (graine + ce journal) → rejeu
 * au bit, classements auto-vérifiés, repro de bug. La CHRONIQUE n'enfile jamais
 * (cmd_n=0) → le drain est un no-op et son hash reste IDENTIQUE (golden intact). */
/* §3 ajoute les verbes DIPLO (capstone #26 : le joueur PROPOSE, le vis-à-vis ÉVALUE via
 * ai_consider_offer). Étendre = un verbe ici (avant CMD_COUNT) + un case au drain. */
enum { CMD_NONE=0, CMD_BUILD, CMD_RECRUIT, CMD_SET_LEVY, CMD_RESEARCH,
       /* §3 diplo (capstone #26) */
       CMD_DECLARE_WAR, CMD_MAKE_PEACE, CMD_OFFER_ALLIANCE, CMD_OFFER_PACT, CMD_EMBARGO,
       /* §3 intérieur · commerce · guerre — plomberie additive (même motif : verbe + case au drain) */
       CMD_REPRESS, CMD_ASSIMILATE, CMD_PURGE, CMD_COUNCIL_HIRE, CMD_COUNCIL_DISMISS,
       CMD_ROUTE, CMD_MARKET_BUY, CMD_MARKET_SELL,
       CMD_CAMPAIGN, CMD_POSTURE, CMD_REFILL, CMD_NAVY_BUILD, CMD_DISBAND,
       /* ALLOCATION de main-d'œuvre (onglet province) : poids par puits, fermeture, intrant, retour AUTO */
       CMD_ALLOC_RAW, CMD_ALLOC_BLD, CMD_ALLOC_INPUT, CMD_ALLOC_AUTO,
       CMD_COUNT };
#define SCPS_CMDQ_MAX 64
typedef struct { uint8_t verb; int32_t a[4]; } PlayerCmd;

/* L'ÉTAT PLEIN d'une partie (tous les sous-systèmes). Membres alloués sur le tas
 * (sim_alloc) ; les pointeurs sont assignés par l'hôte ou sim_alloc. */
typedef struct {
    WorldEconomy *econ; WorldProsperity *wp; WorldLegitimacy *wl; TradeNetwork *net;
    TechState *ts; Statecraft *sc; AgencyState *ag; EventsState *ev; ModifierStack *drift;
    LaborEcon *labor; DiploState *dp; RouteNetwork *rn; AiActor *ai; bool *ai_on;
    RevoltState *rs;
    WarHost     *host;   /* armées levées par pays (mobilisation) */
    Campaign    *camp;   /* armées de campagne : marche/siège/bataille sur la carte (non-invasif) */
    uint32_t     camp_rng;
    MissionsState *missions; /* missions décennales (rythme + injection de ressources) */
    NavyState   *navy;   /* la flotte (mer §5) : coques, chantier, entretien */
    EndgameState *eg;   /* capstone §27 : état cataclysme (entropie + fin + merveille) */
    int16_t prev_owner_mo[SCPS_MAX_REG];   /* propriétaires au mois précédent (détection de conquête) */
    int prev_dawned;         /* dernier âge avéné traité (engagement d'âge §7) */
    int day, year, player;
    int human_player;        /* index du pays piloté À LA MAIN (-1 = aucun : la chronique headless reste 100 % IA) */
    PlayerCmd cmdq[SCPS_CMDQ_MAX]; int cmd_n;   /* journal de commandes JOUEUR (vidé au tick, déterministe) */
    int research_target;   /* cible de recherche du JOUEUR (-1 = aucune ; file de 1, modèle viewer) */
} Sim;

/* allocation/libération des MEMBRES (heap) — la chronique alloue inline (intacte) ;
 * la façade scps_api passe par ces helpers (DRY). false = OOM. */
bool sim_alloc(Sim *s);
void sim_free_members(Sim *s);

/* le cœur PARTAGÉ (verbatim chronicle) */
void sim_init(Sim *s, World *w);   /* RAZ pleine + seed du monde */
void sim_day (Sim *s, World *w);   /* un jour de jeu PLEIN */
int  regions_of(const WorldEconomy *e, int c);   /* régions tenues par un pays */

/* enfile un ordre JOUEUR (façade). false si la file est pleine. L'ordre est
 * REVALIDÉ et appliqué au prochain sim_day (drain déterministe). */
bool sim_cmd_push(Sim *s, PlayerCmd c);

/* télémétrie partagée (la chronique les lit pour ses bilans) */
extern long g_tot_occ_posed, g_tot_occ_lifted;   /* occupations posées / levées */
extern long g_peak_u[U_COUNT];                    /* FORGEDIAG : pic d'effectif par type */
extern long g_wild_spawned, g_wild_defected;     /* HAMEAUX LIBRES : semés · ralliés culturellement */
extern double g_wild_absorb_pop;                  /* pop CUMULÉE ralliée (÷ g_wild_defected = moyenne) */

#endif /* SCPS_SIM_H */
