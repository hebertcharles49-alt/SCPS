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
/* RE-KEY PROVINCE (doctrine « la province est la seule réalité économique ») :
 * CMD_BUILD a={Edifice, PROVINCE (pid, <0 ⇒ capitale du joueur), 0, 0} — plus une
 * région à résoudre via econ_region_rep_province. */
enum { CMD_NONE=0, CMD_BUILD, CMD_RECRUIT, CMD_SET_LEVY, CMD_RESEARCH,
       /* §3 diplo (capstone #26) */
       CMD_DECLARE_WAR, CMD_MAKE_PEACE, CMD_OFFER_ALLIANCE, CMD_OFFER_PACT, CMD_EMBARGO,
       /* §3 intérieur · commerce · guerre — plomberie additive (même motif : verbe + case au drain) */
       CMD_REPRESS, CMD_ASSIMILATE, CMD_PURGE, CMD_COUNCIL_HIRE, CMD_COUNCIL_DISMISS,
       CMD_ROUTE, CMD_MARKET_BUY, CMD_MARKET_SELL,
       CMD_CAMPAIGN, CMD_REFILL, CMD_NAVY_BUILD, CMD_DISBAND,
       /* ALLOCATION de main-d'œuvre (onglet province) : poids par puits, fermeture, intrant,
        * retour AUTO. RE-KEY PROVINCE : a[0] est un PID direct (jamais une région). */
       CMD_ALLOC_RAW, CMD_ALLOC_BLD, CMD_ALLOC_INPUT, CMD_ALLOC_AUTO,
       /* §7 — l'ENGAGEMENT D'ÂGE du joueur (l'IA s'engage auto ; le joueur CHOISIT — ce verbe) */
       CMD_AGE_ENGAGE,
       /* COLONISATION (charte : « le joueur colonise n'importe quelle province ») — a[0] = province cible */
       CMD_COLONIZE,
       /* BRASSAGE — le joueur PROPOSE un pacte migratoire (le vis-à-vis ÉVALUE via ai_consider_offer) */
       CMD_OFFER_MIGRATION,
       /* PANNEAU B — le joueur pose une MANUFACTURE civile (a[0]=PROVINCE, a[1]=BuildingType).
        * Le §NF exclut le joueur (« il construit à la main ») : voici la main. Gates au
        * drain = miroir d'ai_build_civmanuf (staffage/tier/intrant/slot libre/or). RE-KEY
        * PROVINCE (doctrine « la province est la seule réalité économique ») : a[0] est
        * un pid direct, jamais une région à résoudre via econ_region_rep_province. */
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
       CMD_CORPS_RAISE,     /* a: packets, target */
       CMD_CORPS_SPLIT,     /* a: id, packets */
       CMD_CORPS_MERGE,     /* a: dst_id, src_id */
       CMD_CORPS_MOVE,      /* a: id, target */
       CMD_CORPS_REFILL,    /* a: id */
       CMD_CORPS_DISBAND,   /* a: id */
       /* BUDGET : a[0]=0 fiscalité par classe / 1 enveloppe de dépense,
        * a[1]=index, a[2]=multiplicateur ×100 (10..200). */
       CMD_BUDGET_POLICY,
       /* Offre de paix composée : a={cible, drapeaux, score-or, n régions,
        * régions...}. Le pays ne peut posséder que 32 régions. */
       CMD_PEACE_OFFER,
       /* Retour joueur 2026-07-13 — l'onglet province par classe (RE-KEY PROVINCE : a[0]
        * est un PID direct, jamais une région) :
        * CMD_MANUF_LEVEL  a={province, BuildingType, dir(+1/-1)} : monte (payant) /
        *   descend (retire sous plancher) le niveau d'une manufacture bâtie.
        * CMD_DEMOLISH_EDI a={province, Edifice} : démolir un édifice d'un cran. */
       CMD_MANUF_LEVEL, CMD_DEMOLISH_EDI,
       /* MONNAIE M3d — LA BANQUEROUTE VOLONTAIRE (décision joueur 2026-07-15) : répudiation
        * TOTALE de la dette du pays (granularité PAYS, une politique — motif CMD_MANUMIT,
        * pas d'arguments, agit sur p = s->human_player). Débuff −75 % prod/croissance/moral
        * décroissant (BANKRUPTCY_SCAR_YEARS) + grief de la cité-état créancière frappée. */
       CMD_BANKRUPTCY,
       CMD_REPAY,          /* 2026-07-21 (KoH2 « Repay All ») : rembourser VOLONTAIREMENT le principal — a[0]=montant (≤0 : tout ce que le surplus permet) */
       /* MONNAIE M9 — V1 : « EMPRUNTER À UN ORDRE » (panneau éco, décision joueur 2026-07-16).
        * a[0]=SocialClass (CLASS_ELITE/CLASS_BOURGEOIS seules prêtent, motif M3c), a[1]=montant
        * demandé (<=0 ⇒ le MAXIMUM disponible). La classe NE REFUSE JAMAIS — credit_borrow_class
        * couvre ce qu'elle PEUT (capacité épuisée ≠ refus). */
       CMD_BORROW_CLASS,
       /* MONNAIE M9 — V2 : « DEMANDER UN EMPRUNT À UN ÉTAT » (diplomatie, décision joueur
        * 2026-07-16). a[0]=État cible, a[1]=montant demandé (<=0 ⇒ le maximum structurel,
        * cf. drain). L'État PEUT REFUSER (ai_consider_offer/OFFER_LOAN — value SUBJECTIVE) —
        * throttlé par le MÊME émissaire que les autres verbes diplo (1 action/2 mois). */
       CMD_REQUEST_LOAN,
       CMD_COUNT };
#define SCPS_CMDQ_MAX 64
#define SCPS_PEACE_MAX_TERRITORIES 32
enum {
    PEACE_REPARATIONS = 1u<<0,
    PEACE_HUMILIATE   = 1u<<1,
    PEACE_PILLAGE     = 1u<<2,
    PEACE_LIBERATE    = 1u<<3,
    PEACE_VASSALIZE   = 1u<<4,
    PEACE_FRAGMENT    = 1u<<5
};
typedef struct { uint8_t verb; int32_t a[4 + SCPS_PEACE_MAX_TERRITORIES]; } PlayerCmd;

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
