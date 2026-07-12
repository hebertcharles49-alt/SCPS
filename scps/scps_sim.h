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
#include "scps_heritage.h"
#include "scps_decrees.h"
#include "scps_fog.h"      /* BROUILLARD DE GUERRE : connaissance des empires (étape 1/2, VISUEL seulement) */
#include <stdbool.h>
#include <stdio.h>   /* FILE : sim_wild_save/load (section WILD du save partagé) */

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
       /* §7 — l'ENGAGEMENT D'ÂGE du joueur (l'IA s'engage auto ; le joueur CHOISIT — ce verbe) */
       CMD_AGE_ENGAGE,
       /* COLONISATION (charte : « le joueur colonise n'importe quelle province ») — a[0] = province cible */
       CMD_COLONIZE,
       /* BRASSAGE — le joueur PROPOSE un pacte migratoire (le vis-à-vis ÉVALUE via ai_consider_offer) */
       CMD_OFFER_MIGRATION,
       /* PANNEAU B — le joueur pose une MANUFACTURE civile (a[0]=région, a[1]=BuildingType).
        * Le §NF exclut le joueur (« il construit à la main ») : voici la main. Gates au
        * drain = miroir d'ai_build_civmanuf (staffage/tier/intrant/slot libre/or). */
       CMD_BUILD_MANUF,
       /* MEMBRANE DE DÉCISION — le joueur CHOISIT parmi les options d'un évènement en
        * attente (a[0]=slot dans EventsState.pending[], a[1]=option). Revalidé au drain
        * (slot occupé, option<n_options, sujet toujours au joueur) → pending_event_resolve. */
       CMD_EVENT_CHOICE,
       /* DÉCRETS DU JOUEUR (civics) — a[0]=DecreeId, a[1]=on/off. Revalidé au drain :
        * id borné, condition d'entrée remplie (ON), une RÉFORME active refuse le OFF. */
       CMD_DECREE,
       /* ESCLAVAGE — le joueur AFFRANCHIT tout son pays (granularité PAYS, une politique,
        * pas une province). Pas d'arguments (agit sur p = s->human_player). */
       CMD_MANUMIT,
       /* ESCLAVAGE — le MARCHÉ des Centres. a[0]=région (au joueur), a[1]=count.
        * ACHAT gaté éthos/tech (miroir diplo_enslave_capture) ; VENTE sans gate (on
        * vend ce qu'on tient déjà). */
       CMD_SLAVE_BUY, CMD_SLAVE_SELL,
       /* LOT G — RÉINCORPORATION DE POP : a={région A (source), région B (dest),
        * classe (SocialClass), count}. REVALIDÉ : A≠B toutes deux au joueur. */
       CMD_POP_TRANSFER,
       /* W-GUERRE-3 — FABRIQUER un casus belli PAYANT contre a[0] (cible). Revalidé au
        * drain : cible valide, or suffisant (diplo_can_fabricate) → sinon silencieux. */
       CMD_FABRICATE_CB,
       /* V2a — LE CONSEIL VIVANT : le curseur de PAIE d'un siège pourvu.
        * a[0]=seat, a[1]=paie ×100 (0..200 → 0×..2×). Revalidé au drain : siège
        * pourvu (sinon rien à payer), clampé [0,2]. */
       CMD_COUNCIL_PAY,
       /* LOT P — PILLER LA CÔTE (règle joueur : « piraterie, raids, tout type
        * d'occupation = pillage »). a[0]=province CIBLE (côtière, à un AUTRE pays,
        * ni allié ni pacte — miroir de la course pirate IA). Revalidé au drain :
        * province valide/peuplée/côtière, pas à soi/allié/pacte, pas de balafre
        * active (raid_cd_days), le joueur tient ≥1 coque PIRATE. Exécution = le
        * MÊME chemin de pillage unifié (20% du revenu annuel + esclavage 5% si
        * gate) que le sac de siège/l'occupation, + pose du CD/balafre. */
       CMD_RAID_COAST,
       /* MOUVEMENT D'ARMÉE LIBRE (clic-armée → clic-destination, RTS/Stellaris). a[0]=région
        * cible. Réutilise la campagne : armée EN campagne → campaign_redirect (re-cible, self-
        * gardé bataille/mer/brisée) ; réserve au repos → campaign_order depuis la CAPITALE.
        * Arriver sur une région À SOI = l'armée s'y POSE (FA_IDLE) ; ennemie = siège/assaut. */
       CMD_MOVE_ARMY,
       CMD_COUNT };
#define SCPS_CMDQ_MAX 64
typedef struct { uint8_t verb; int32_t a[4]; } PlayerCmd;

/* L'ÉTAT PLEIN d'une partie (tous les sous-systèmes). Membres alloués sur le tas
 * (sim_alloc) ; les pointeurs sont assignés par l'hôte ou sim_alloc. */
typedef struct {
    WorldEconomy *econ; WorldProsperity *wp; WorldLegitimacy *wl; TradeNetwork *net;
    TechState *ts; Statecraft *sc; AgencyState *ag; EventsState *ev; ModifierStack *drift;
    DiploState *dp; RouteNetwork *rn; AiActor *ai; bool *ai_on;   /* (LaborEcon dissous : la levée LIT les strates econ) */
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
    int player_age_engaged;   /* §7 : dernier âge ENGAGÉ par le joueur (-1 = aucun) — persiste (SaveMisc v48) */
    int diplo_ready_day;   /* le DIPLOMATE : jour où le prochain acte diplo JOUEUR est permis
                            * (UN émissaire, 1 action / 2 mois) — persiste (SaveMisc v49) */
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

/* HAMEAUX LIBRES — sérialisation des compteurs de contact pacifique (section WILD, v48).
 * Sans elle, un CHARGEMENT en processus frais remettait le ralliement à zéro (retardé
 * jusqu'à WILD_DEFECT_YEARS vs le fil continu) : continuation ≠ sauve-recharge. */
void sim_wild_save(FILE *f);
bool sim_wild_load(FILE *f);

/* télémétrie partagée (la chronique les lit pour ses bilans) */
extern long g_tot_occ_posed, g_tot_occ_lifted;   /* occupations posées / levées */
/* LOT 4 (audit de guerre) — pillage de siège : or-équivalent détourné cumulé,
 * captures de sac (déportations à la CHUTE, avant règlement). */
extern double g_siege_loot_total;
extern long   g_siege_sack_captures;
/* LOT P (2026-07-07) — pillage unifié : valeur pillée cumulée à l'occupation-capture
 * (la chute d'un siège) — 20% du revenu annuel de la victime, cf. scps_diplo.h. */
extern double g_occ_pillage_total;
extern long g_peak_u[U_COUNT];                    /* FORGEDIAG : pic d'effectif par type */
extern long g_wild_spawned, g_wild_defected;     /* HAMEAUX LIBRES : semés · ralliés culturellement */
extern double g_wild_absorb_pop;                  /* pop CUMULÉE ralliée (÷ g_wild_defected = moyenne) */

#endif /* SCPS_SIM_H */
