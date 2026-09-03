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

/* Corps manoeuvrables. L'identifiant est stable dans une sauvegarde : le slot 0
 * conserve volontairement l'ancien index `owner`, puis viennent les slots suivants.
 * Cette disposition garde les anciens lecteurs du corps principal compatibles tout
 * en offrant 32 corps indépendants par pays. */
#define CAMPAIGN_MAX_CORPS 32
#define CAMPAIGN_ARMY_CAP  (SCPS_MAX_COUNTRY * CAMPAIGN_MAX_CORPS)
#define CAMPAIGN_CORPS_ID(owner, slot) ((owner) + (slot) * SCPS_MAX_COUNTRY)

/* Une armée expéditionnaire posée sur la carte. */
typedef struct {
    int        id;         /* identifiant stable CAMPAIGN_CORPS_ID(owner,slot) */
    bool       active;      /* déployée ? */
    int        owner;       /* pays */
    int        loc;         /* région occupée */
    int        dest;        /* région-but (marche/siège) ; -1 = aucune */
    int        next;        /* prochaine région de la marche en cours ; -1 = aucune */
    FieldPhase phase;
    float      days_left;   /* jours restants de l'étape (marche) ou du siège */
    float      leg_days;    /* durée totale de l'étape en cours (pour l'attrition) */
    ArmyState  force;       /* la composition (détachement) */
    /* RENFORCER = COMBLER LE DÉFICIT — FORCE NOMINALE (paquets de 100) : le PLEIN de
     * référence de ce corps, jamais un plafond dur. Posée à la levée (campaign_raise,
     * campaign_order — le corps « slot 0 » historique n'appelle jamais campaign_raise) ;
     * répartie au PRORATA sur un split (les deux moitiés se partagent l'ancien nominal,
     * campaign_split/campaign_split_comp) ; SOMMÉE sur un merge ; RELEVÉE dès que le
     * courant dépasse l'ancien pic (un refill, un merge généreux, une nouvelle levée sur
     * un corps déjà actif — « le nominal suit le pic »). Le déficit visé par le renfort
     * = max(0, nominal − courant) ; 0 (plein) grise le bouton côté façade. SAVE_VERSION
     * 97 (champ neuf ⇒ struct Campaign plus large, <97 refusé). */
    long       nominal;
    /* journal (UI/IA) */
    int        taken;       /* régions RÉDUITES (sièges menés à terme) — cumul */
    int        taken_region;/* dernière région réduite NON ENCORE récoltée (-1 = aucune) :
                             * la couche sim la lit → occupation/libération, puis remet -1 */
    int        legs;        /* étapes de marche franchies */
    int        battles;     /* batailles livrées */
    int        posture;     /* tombstone SAVE historique, sans effet ni contrôle joueur */
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
    float lossA, lossB;    /* pertes CUMULÉES (paquets, fraction en attente incluse) */
} FieldBattle;
#define CAMPAIGN_MAX_BATTLES 8

typedef struct Campaign {
    FieldArmy army[CAMPAIGN_ARMY_CAP];  /* slots stables ; slot 0 = ancien corps principal */
    int       n_corps[SCPS_MAX_COUNTRY];/* corps actifs par pays (cache sérialisé/validé) */
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
    /* #32 (LE SANG SIGNE TON RÈGNE) — jumeau du couple ci-dessus, ne comptant QUE les
     * morts des batailles IMPLIQUANT le joueur humain (un des deux belligerents ==
     * g_campaign_human) : additif au MÊME site que dead_choc/dead_pursuit (bt_day/
     * bt_rout), jamais un calcul dérivé après coup. g_campaign_human=-1 (chronique,
     * viewer sans main humaine) ⇒ ce compteur reste à 0 pour toujours. */
    long  dead_choc_player, dead_pursuit_player;
    long  battle_days;                    /* Σ durées (jours) */
    int   n_sails;                        /* mer §10 : traversées ordonnées */
    float sail_days_sum;                  /* Σ jours de mer des traversées */
    /* AUDIT 2026-08-12 : « les morts de bataille terrestre ne meurent JAMAIS côté
     * pop » — dead_choc/pursuit n'étaient que des MOTS. Chaque paquet tué s'inscrit
     * ICI (par pays × classe d'origine, unit_def->from) ; sim draine après
     * campaign_tick : la strate paie ses morts (econ_region_pop_add, capitale) et
     * l'AFFECTATION se rend (pop_by_class_in_army — sinon le recrutement
     * s'asphyxiait, army_class_free fondait en monotone). Sérialisé avec le blob
     * CAMP (v103). */
    long  dead_class_pending[SCPS_MAX_COUNTRY][3];
} Campaign;

/* Lecture tactique PURE d'un champ actif. Les puissances sont celles du prochain
 * choc AVANT son aléa journalier ±15 %, avec les mêmes fonctions que bt_day. */
typedef struct {
    int valid, region, owner_a, owner_b;
    int stage;              /* 0 choc · 1 accalmie */
    int terrain_owner;      /* pays avantagé par le sol ; -1 si neutre */
    int river, bridged;
    float terrain_a;        /* multiplicateur du camp A ; B reçoit l'inverse */
    float counter_a, counter_b;
    float power_a, power_b; /* puissance déterministe pré-aléa */
    int balance_a_pct;      /* part A de power_a+power_b */
    int rupture_pct;        /* cohésion sous laquelle la déroute devient possible */
} CampaignBattleFactors;
bool campaign_battle_factors(const Campaign *c, const WorldEconomy *e, int region,
                             CampaignBattleFactors *out);

/* Lecture PURE d'un siège en cours. `full_days` recalcule la résistance de
 * référence avec les ouvrages, vivres et terrain ACTUELS ; `progress_pct` est
 * donc une estimation d'interface, tandis que `days_left` est le compte à
 * rebours exact et sérialisé du moteur. */
typedef struct {
    int valid, region, owner;
    float days_left, full_days;
    int progress_pct;
    float defense_level, food_months;
    int terrain_pct;       /* multiplicateur de tenue ×100 */
} CampaignSiegeFactors;
bool campaign_siege_factors(const Campaign *c, const WorldEconomy *e, int region,
                            CampaignSiegeFactors *out);

/* Bâtit la table de terrain par région et remet les armées à zéro. */
void campaign_init(Campaign *c, const World *w, const WorldEconomy *econ);

/* #32 — la MAIN HUMAINE (miroir de warhost_set_human/econ_set_human) : quel pays est
 * le joueur, pour isoler SES morts de guerre du cumul mondial. Global (comme les deux
 * autres) car campaign_init memset le Campaign entier à chaque régénération/reload —
 * posé APRÈS, survit à travers les ticks. -1 = aucun humain (défaut ; chronique ne
 * l'appelle jamais). */
void campaign_set_human(int cid);
int  campaign_get_human(void);   /* lecteur : -1 si aucun (chronique/viewer sans main humaine) */

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

/* API explicite par corps. Les anciens verbes par owner ci-dessous restent des
 * wrappers sur le corps principal pour les systèmes qui n'ont pas encore besoin
 * de distinguer les détachements. */
int  campaign_corps_id(int owner, int slot);
FieldArmy       *campaign_corps(Campaign *c, int id);
const FieldArmy *campaign_corps_const(const Campaign *c, int id);
int  campaign_corps_count(const Campaign *c, int owner);
int  campaign_corps_id_at(const Campaign *c, int owner, int ordinal);
int  campaign_raise(Campaign *c, const WorldEconomy *econ, int owner,
                    int from_region, int target_region, ArmyState *src_force,
                    long packets);
bool campaign_redirect_corps(Campaign *c, const WorldEconomy *econ,
                             const DiploState *dp, int id, int target_region);
/* Aperçu PUR d'une redirection : chemin terrestre exact (régions, départ inclus),
 * durée estimée des étapes et issue d'arrivée. reason: 0 ok · 1 corps invalide ·
 * 2 bataille · 3 mer · 4 brisé · 5 cible invalide · 6 force vide · 7 injoignable.
 * arrival: 0 arrêt sur place · 1 territoire tenu · 2 siège. */
int campaign_preview_corps(const Campaign *c, const WorldEconomy *econ,
                           const DiploState *dp, int id, int target_region,
                           int *path, int max_path, float *days_out,
                           int *reason_out, int *arrival_out);
int  campaign_split(Campaign *c, int id, long packets);
/* SPLIT COMPOSÉ : comme campaign_split, mais détache une composition EXACTE par GRAND
 * TYPE — `inf_p`/`arch_p`/`cav_p`/`mages_p` (paquets de 100, ≥0, cf. ArmyComposition/
 * campaign_corps_composition pour le classement des 22 types en 4 familles). Refuse si
 * le total demandé est nul, couvre le corps ENTIER (il doit en rester), ou si un type
 * dépasse ce que le corps possède RÉELLEMENT de ce type — jamais un clamp silencieux, la
 * composition livrée est EXACTEMENT celle demandée ou l'appel échoue net (-1). Le
 * nominal se répartit au même prorata que campaign_split (packets déplacés / effectif
 * courant du corps source). Renvoie l'id du nouveau corps, -1 sinon. */
int  campaign_split_comp(Campaign *c, int id, long inf_p, long arch_p, long cav_p, long mages_p);
bool campaign_merge(Campaign *c, int dst_id, int src_id);

/* L'EMBARQUEMENT (mer §6) : ordonne la traversée depuis `from_region` (un port
 * RÉEL du pays) vers `target_region` (région CÔTIÈRE). Exige assez de capacité
 * d'emport libre (10 paquets/transport) ; les transports sont RÉSERVÉS jusqu'au
 * débarquement. `navy` mutable (réservation). `src_force` TRANSFÉRÉ (LOT 1, même
 * contrat que campaign_order). Un blocus n'interdit pas le départ : il expose le
 * convoi à l'interception physique. false si pas de port / pas de flotte / mer
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

/* M15 — F3 : redirige un corps ACTIF vers `target_region` en repliant sur
 * l'EMBARQUEMENT (mêmes conditions que campaign_order_sea : port ami à SA
 * position actuelle, côte à l'arrivée, transports libres) — pour le cas
 * où campaign_redirect_corps refuse (cible injoignable par terre). N'appeler QUE
 * depuis le dispatch des verbes joueur (jamais le chemin partagé avec l'IA) :
 * golden-neutre par construction. false si déjà en mer, brisée, pas de port à
 * soi, pas de côte d'arrivée ou pas assez de transports. Un blocus expose ensuite
 * le corps à l'interception ; il ne transforme pas la mer en mur. */
bool campaign_redirect_corps_sea(Campaign *c, const World *w, const WorldEconomy *econ,
                                 struct NavyState *navy, int id, int target_region);

long        campaign_corps_units(const Campaign *c, int id);
/* Σ de la pop AFFECTÉE (par classe) que les corps actifs de `owner` portent au front.
 * Lu par la levée (warhost_tick) pour que le pool de recrutement voie TOUT ce que le
 * pays a sous les armes, host ET corps de campagne (CALIB_ARMEE 2026-09-03 §4.2).
 * INLINE D'EN-TÊTE À DESSEIN : `scps_warhost.c` en a besoin, mais DIX bancs lient
 * `scps_warhost.o` SANS `scps_campaign.o` (army/demography/social/agency/ai/forks/
 * credit/cap/religion_demo) — un symbole de plus dans campaign.c casserait leur édition
 * de liens. Pure lecture de champs déjà publics : aucun état, aucun coût. */
static inline long campaign_deployed_class(const Campaign *c, int owner, LaborClass cl){
    if (!c || owner<0 || owner>=SCPS_MAX_COUNTRY || cl<0 || cl>=LAB_CLASS_COUNT) return 0;
    long n=0;
    for (int i=0;i<CAMPAIGN_ARMY_CAP;i++)
        if (c->army[i].active && c->army[i].owner==owner)
            n += c->army[i].force.pop_by_class_in_army[cl];
    return n;
}
long        campaign_disband_corps(Campaign *c, int id, ArmyState *dst_host_army);
bool        campaign_can_refill_corps(const Campaign *c, const WorldEconomy *econ, int id);
void        campaign_refill_corps_cost(const Campaign *c, int id, long *men, long *mat);
/* RENFORCER = COMBLER LE DÉFICIT (nominal − courant). Refuse net (0, rien consommé) si
 * le corps est DÉJÀ à son nominal (ou au-delà) — le déficit est nul, l'appelant façade
 * doit avoir grisé le bouton (cf. scps_corps_refill_preview). Sinon : MÊME granularité
 * qu'avant (« cap par vague » conservé : +1 paquet de 100 par type d'unité présent et
 * finançable ce tick, pas tout le déficit d'un coup) ; en fin d'appel, si le total
 * dépasse malgré tout l'ancien nominal (plusieurs lignes remplies dans la même vague),
 * le nominal est RELEVÉ au nouveau pic. Renvoie les paquets ajoutés. */
int         campaign_refill_corps(Campaign *c, int id, WorldEconomy *econ);

/* ---- Lecteurs (membrane : tangibles) ---------------------------------- */
bool        campaign_active       (const Campaign *c, int owner);
int         campaign_location     (const Campaign *c, int owner);  /* région ou -1 */
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
ArmyComposition campaign_corps_composition(const Campaign *c, int id);

/* (army_host_word RETIRÉ — P1.10 : effectif EXACT affiché sur toute armée.) */

/* ---- RENFORT (« remplir ») — recompléter une armée en TERRITOIRE AMI -------- */
/* Peut-on renforcer l'armée de `owner` ? (active ET la région où elle se tient lui
 * appartient — on ne se renforce que chez soi). */
bool campaign_can_refill(const Campaign *c, const WorldEconomy *econ, int owner);
/* Coût d'un renfort (+1 paquet de 100 par type d'unité présent) : `men` hommes
 * levés, `mat` matériaux pour les armes (achetés au marché, or si manque). Lecture. */
void campaign_refill_cost(const Campaign *c, int owner, long *men, long *mat);
/* RENFORCE l'armée : comble le DÉFICIT (nominal − courant), « cap par vague » +1 paquet
 * par type d'unité présent et finançable ce tick (fabrique l'arme en pompant le marché
 * si besoin, lève la pop) — no-op si déjà au nominal, cf. campaign_refill_corps ci-
 * dessous. Le POOL par classe est lu des strates econ du pays `owner` (pool pop UNIFIÉ —
 * fin de LaborEcon). Renvoie les paquets ajoutés. */
int  campaign_refill(Campaign *c, int owner, WorldEconomy *econ);  /* F6 : pompe les armes macro */

/* Sécurité de CHARGEMENT (scps_save.c, appelée après un load réussi, motif
 * demography_dyn_id_rebase) : tout corps ACTIF dont le nominal désérialisé est SOUS son
 * effectif courant est relevé au courant — le nominal ne doit jamais laisser un déficit
 * négatif (corps_refill_preview le clamperait à 0 de toute façon, mais un save cohérent
 * ne doit pas dépendre de ce clamp). Idempotente, sans effet sur un save déjà cohérent. */
void campaign_backfill_nominal(Campaign *c);

#endif /* SCPS_CAMPAIGN_H */
