#ifndef SCPS_CAMPAIGN_H
#define SCPS_CAMPAIGN_H
/*
 * scps_campaign.h — LES ARMÉES SONT SUR LA CARTE : la campagne dans le temps
 *
 * scps_army donnait les PRIMITIVES (déplacement §1, bataille §2, doctrine §3,
 * siège) ; scps_warhost faisait MOBILISER chaque pays (une force nationale posée
 * sur la capitale → mil_power). Mais ces forces ne BOUGEAIENT pas : la guerre
 * restait un diplo abstrait (score → budget → prix).
 *
 * Ce module pose l'ARMÉE DE CAMPAGNE : une force EXPÉDITIONNAIRE par pays, avec une
 * POSITION (une région), qui MARCHE de région en région (au pas du convoi, le
 * terrain décidant des jours — §1), ASSIÈGE une région ennemie en arrivant (14 j
 * si nue, jusqu'à 2 ans selon fortif/vivres/terrain — le siège), et LIVRE BATAILLE
 * (§2/§3, doctrine + phases + poursuite) quand deux armées hostiles se croisent.
 *
 * NON-INVASIF : la campagne ne TOUCHE PAS la conquête abstraite. Elle LIT l'éco
 * (terrain, fortifications, vivres, propriété) et fait vivre les armées sur la
 * carte ; la réduction d'une région est ENREGISTRÉE (taken / région réduite),
 * jamais appliquée à econ->owner ici — l'intégration (et l'UI §4) viendront
 * ensuite. Les prix/volume de conquête restent intacts.
 *
 * Granularité : la RÉGION (on réutilise econ->adj, la même adjacence que la
 * conquête). Membrane : les lecteurs renvoient des nombres tangibles (paquets de
 * 100, identifiant de région, mots de phase).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_army.h"
#include "scps_diplo.h"

/* Phase d'une armée de campagne. L'EMBARQUEMENT (mer §6) : une armée à un port
 * + capacité de flotte → embarque (jours), navigue (jours ∝ coût directionnel
 * du trajet — en mer elle est intouchable ET aveugle), débarque (plus lent et
 * exposé hors port). */
typedef enum { FA_IDLE = 0, FA_MARCH, FA_SIEGE, FA_BATTLE,
               FA_EMBARK, FA_SAIL, FA_LAND, FA_PHASE_COUNT } FieldPhase;

/* Une armée expéditionnaire posée sur la carte. */
typedef struct {
    bool       active;      /* déployée ? */
    int        owner;       /* pays */
    int        loc;         /* région occupée */
    int        dest;        /* région-but (marche/siège) ; -1 = aucune */
    int        next;        /* prochaine région de la marche en cours ; -1 = aucune */
    FieldPhase phase;
    float      days_left;   /* jours restants de l'étape (marche) ou du siège */
    float      leg_days;    /* durée totale de l'étape en cours (pour l'attrition) */
    ArmyState  force;       /* la composition (détachement) */
    /* journal (UI/IA) */
    int        taken;       /* régions RÉDUITES (sièges menés à terme) — cumul */
    int        taken_region;/* dernière région réduite NON ENCORE récoltée (-1 = aucune) :
                             * la couche sim la lit → occupation/libération, puis remet -1 */
    int        legs;        /* étapes de marche franchies */
    int        battles;     /* batailles livrées */
    int        posture;     /* §5 sidebar : 0 prudente · 1 standard · 2 agressive (module marche/siège) */
    int        broken_days; /* armée BRISÉE (déroute) : inapte au combat tant que > 0 (se reconstitue) */
    /* L2 — LE RALLIEMENT (H4.3) : une armée en déroute ne s'évapore plus — elle se
     * reforme à 40-60 % de son effectif d'avant-déroute après 30-60 j, UNE fois par
     * armée et par guerre (rally_used se relâche à la paix). SAVE_VERSION 14. */
    bool       rally_used;     /* le ralliement de cette guerre est consommé */
    float      rally_days;     /* compte à rebours avant la re-formation (0 = aucun) */
    int        rally_packets;  /* effectif-cible de la re-formation (paquets de 100) */
    /* LA TRAVERSÉE (mer §6) */
    int        sail_transports;  /* transports réservés (rendus au débarquement) */
    float      sail_days;        /* jours de mer du trajet ordonné (volta comprise) */
    bool       land_at_port;     /* débarque à un port (sinon : plus lent, petit malus) */
    bool       intercept_done;   /* une CHASSE par traversée (coques §3) */
} FieldArmy;

/* ── LA BATAILLE DANS LE TEMPS (brief bataille) — un ÉTAT, plus un événement ──
 * Deux armées hostiles qui se croisent S'ACCROCHENT (FA_BATTLE, épinglées) : des
 * CHOCS de 3 jours (jets, pertes, le moral s'use) alternent avec des ACCALMIES de
 * 2 jours (le moral se stabilise — moins qu'un choc ne coûte) jusqu'à la RUPTURE
 * d'une réserve → la DÉROUTE, puis la POURSUITE — où tombe l'essentiel des morts. */
typedef struct {
    bool  active;
    int   a, b;            /* les deux camps (indices pays) ; helpers = marche au canon */
    int   helpA, helpB;    /* un renfort par camp (-1) — allié/suzerain/vassal adjacent */
    int   loc;             /* la région du champ */
    int   cycle;           /* jour dans le cycle 0..4 (0-2 choc · 3-4 accalmie) */
    int   days, chocs;     /* durée totale · jours de choc livrés */
    float resA, resB;      /* RÉSERVES de moral (Σ paquets·moral·moral_mul) — ce qui se joue */
    float resA0, resB0;    /* réserves d'ouverture (le seuil de rupture s'y réfère) */
    float lossA, lossB;    /* report fractionnaire des pertes de CHOC (paquets) */
} FieldBattle;
#define CAMPAIGN_MAX_BATTLES 8

typedef struct Campaign {
    FieldArmy army[SCPS_MAX_COUNTRY];   /* une force expéditionnaire par pays */
    int       n_regions;
    /* table de terrain par région (bâtie à l'init depuis le World) */
    Biome     reg_biome [SCPS_MAX_REG];
    float     reg_height[SCPS_MAX_REG];
    bool      reg_river [SCPS_MAX_REG];   /* un cours d'eau notable à franchir (pénalité de choc) */
    FieldBattle battle[CAMPAIGN_MAX_BATTLES];   /* les champs où l'on s'accroche */
    /* télémétrie (chronicle §8) — cumul sim */
    int   n_battles, n_routs, n_disengage, n_reinforce, n_stalemate;
    int   n_rallies;                      /* L2 : armées reformées après déroute */
    long  dead_choc, dead_pursuit;        /* LA vérif : la poursuite DOMINE le choc si cavalerie dominante */
    long  battle_days;                    /* Σ durées (jours) */
    int   n_sails;                        /* mer §10 : traversées ordonnées */
    float sail_days_sum;                  /* Σ jours de mer des traversées */
} Campaign;

/* Bâtit la table de terrain par région et remet les armées à zéro. */
void campaign_init(Campaign *c, const World *w, const WorldEconomy *econ);

/* Ordonne à la force expéditionnaire de `owner` de partir de `from_region` vers
 * `target_region` (région ennemie à réduire) en TRANSFÉRANT `src_force` (p.ex.
 * l'armée mobilisée du warhost) — LOT 1 : ce n'est plus une COPIE, `src_force` est
 * VIDÉ au succès (army_merge_into) : les mêmes âmes existent SOIT en garnison SOIT
 * en campagne, jamais les deux (warhost_units reflète enfin la réserve NON déployée).
 * Si une force est DÉJÀ active pour `owner`, son reliquat est d'abord RENDU à
 * `src_force` (donc à l'appelant, typiquement host->army[owner]) avant que le
 * nouveau détachement parte — un réordonnancement ne perd ni ne double personne.
 * Calcule l'itinéraire (BFS sur l'adjacence des régions praticables). Renvoie false
 * si la cible est injoignable par terre ou la force vide (src_force INCHANGÉ alors). */
bool campaign_order(Campaign *c, const WorldEconomy *econ, int owner,
                    int from_region, int target_region, ArmyState *src_force);

/* L'EMBARQUEMENT (mer §6) : ordonne la traversée depuis `from_region` (un port
 * RÉEL du pays) vers `target_region` (région CÔTIÈRE). Exige assez de capacité
 * d'emport libre (10 paquets/transport) ; les transports sont RÉSERVÉS jusqu'au
 * débarquement. `navy` mutable (réservation). `src_force` TRANSFÉRÉ (LOT 1, même
 * contrat que campaign_order). false si pas de port / pas de flotte / mer
 * infranchissable / force vide (src_force INCHANGÉ alors). */
struct NavyState;
bool campaign_order_sea(Campaign *c, const World *w, const WorldEconomy *econ,
                        struct NavyState *navy, int owner,
                        int from_region, int target_region, ArmyState *src_force);

/* Avance toutes les armées de `dt_days` jours : la marche (§1) étape par étape,
 * le siège à l'arrivée, la bataille (§2/§3) quand deux forces hostiles (en
 * guerre, lu de `dp`) partagent une région. NE MODIFIE PAS econ (lecture seule) :
 * la propriété des régions reste la vérité de la conquête abstraite. `rng` =
 * graine xorshift avancée en place. */
void campaign_tick(Campaign *c, const World *w, const WorldEconomy *econ,
                   DiploState *dp, uint32_t *rng, float dt_days);   /* dp MUTABLE : les batailles nourrissent le bras-de-fer (§6) */
/* Rend les transports d'une armée débarquée/morte à la flotte (appelé par le
 * harnais APRÈS campaign_tick : campaign ne LIE pas la marine — il marque). */
void campaign_release_transports(Campaign *c, struct NavyState *navy);

/* L1 — REDIRIGER une armée DÉJÀ déployée vers une nouvelle cible SANS recopier la
 * force (les pertes restent payées ; un siège en cours est ABANDONNÉ). Refus si
 * l'armée est épinglée en bataille, en mer, brisée, ou la cible injoignable.
 * Sur place : re-décide comme une arrivée (notre terre libre → IDLE ; ennemie ou
 * occupée → SIÈGE). C'est le verbe de l'interception : le défenseur marche À LA
 * RENCONTRE de l'assiégeant, l'attaquant ne dort pas après une prise. */
bool campaign_redirect(Campaign *c, const WorldEconomy *econ, const DiploState *dp,
                       int owner, int target_region);

/* ---- Lecteurs (membrane : tangibles) ---------------------------------- */
bool        campaign_active       (const Campaign *c, int owner);
int         campaign_location     (const Campaign *c, int owner);  /* région ou -1 */
/* POSTURE (§5 sidebar) : prudente conserve (marche/siège lents), agressive presse.
 * Un palier + un mot — module marche & siège côté campaign, rien ne fuit. */
#define FA_PRUDENTE  0
#define FA_STANDARD  1
#define FA_AGRESSIVE 2
void        campaign_set_posture  (Campaign *c, int owner, int posture);
int         campaign_posture      (const Campaign *c, int owner);
const char *campaign_posture_name (int posture);
FieldPhase  campaign_phase        (const Campaign *c, int owner);
long        campaign_units        (const Campaign *c, int owner);  /* paquets de 100 */
int         campaign_taken        (const Campaign *c, int owner);  /* régions réduites */
/* DÉMOBILISER (§4 sidebar) : dissout l'armée de campagne — elle quitte la carte.
 * LOT 1 — si `dst_host_army` est fourni (p.ex. &host->army[owner]), les SURVIVANTS
 * (unités, armes, pop affectée) y sont TRANSFÉRÉS (army_merge_into) : le host
 * retrouve ce qui revient du front, rien n'est perdu ni dupliqué. NULL = ancien
 * comportement (le détachement s'évapore, cf. warhost_disband pour la réserve).
 * Renvoie les paquets de 100 dissous (0 si rien). La POSTURE (réglage joueur) est
 * préservée. */
long        campaign_disband      (Campaign *c, int owner, ArmyState *dst_host_army);
const char *campaign_phase_name   (FieldPhase ph);

/* Composition d'une armée par GRAND TYPE d'arme (paquets de 100) — pour le survol
 * de l'UI §4 (« cav / inf / arch »). Tangible, jamais de coordonnée SCPS. */
typedef struct { long infanterie, archers, cavalerie, mages, total; } ArmyComposition;
ArmyComposition campaign_composition(const Campaign *c, int owner);

/* (army_host_word RETIRÉ — P1.10 : effectif EXACT affiché sur toute armée.) */

/* ---- RENFORT (« remplir ») — recompléter une armée en TERRITOIRE AMI -------- */
/* Peut-on renforcer l'armée de `owner` ? (active ET la région où elle se tient lui
 * appartient — on ne se renforce que chez soi). */
bool campaign_can_refill(const Campaign *c, const WorldEconomy *econ, int owner);
/* Coût d'un renfort (+1 paquet de 100 par type d'unité présent) : `men` hommes
 * levés, `mat` matériaux pour les armes (achetés au marché, or si manque). Lecture. */
void campaign_refill_cost(const Campaign *c, int owner, long *men, long *mat);
/* RENFORCE l'armée : +1 paquet par type d'unité (fabrique l'arme en pompant le
 * marché si besoin, lève la pop). Le POOL par classe est lu des strates econ du
 * pays `owner` (pool pop UNIFIÉ — fin de LaborEcon). Renvoie les paquets ajoutés. */
int  campaign_refill(Campaign *c, int owner, WorldEconomy *econ);  /* F6 : pompe les armes macro */

#endif /* SCPS_CAMPAIGN_H */
