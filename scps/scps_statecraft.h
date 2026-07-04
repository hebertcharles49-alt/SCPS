#ifndef SCPS_STATECRAFT_H
#define SCPS_STATECRAFT_H
/*
 * scps_statecraft.h — INFLUENCE, OPINION, DIPLOMATES & RÉVOLTE
 *
 * La couche d'ÉTAT diplomatique, au-dessus des lecteurs de scps_diplo. Trois
 * systèmes de jeu, tous ANCRÉS sur des coordonnées existantes (jamais un +X
 * libre) :
 *
 *   INFLUENCE  (0-100, réserve à inertie) — la réputation : monte avec la
 *              prospérité, la taille et les accords TENUS, chute aux trahisons,
 *              s'érode sans entretien. Elle plafonne le nombre de missions
 *              diplomatiques simultanées (le « prestige » d'EU4, branché au réel).
 *
 *   OPINION    (−100..+100 par paire) — l'opinion d'un pays envers un autre,
 *              PROJECTION des lecteurs de relation (parenté/sphère, menace,
 *              complément, schisme) + l'historique (la trahison crève l'opinion,
 *              elle remonte lentement vers sa cible).
 *
 *   DIPLOMATES (vivier limité) — un personnel ; chaque mission OCCUPE un agent
 *              pendant des JOURS (modèle de temps), puis applique son effet par
 *              la couche d'action (route, alliance, intégration). L'Influence
 *              fixe la taille du vivier.
 *
 * Et l'AGITATION (0-100, un pur SIGNAL) : lissée depuis L/coercion/choc de
 * conquête/stabilité/garnison (H), c'est la ligne UI « ⚑ Au bord de la révolte ».
 * Dédup Option B (2026-07-04) : ce module NE FAIT PLUS FIRE de révolte lui-même
 * (l'ancien seuil-soutenu → L*=0.40/coercion=1/prestige↓/influence↓ est retiré) ;
 * scps_revolt.c (la révolte INCARNÉE, groupe par groupe) est le SEUL acteur —
 * il lit `statecraft_agitation` comme un grief politique de PLUS dans son propre
 * allumage (aux côtés de la faim/taxe/aliénation/répression/non-intégration).
 * `revolt_fired`/`unrest_days` restent des champs de struct INERTES (jamais
 * écrits vrai/accumulés) — `statecraft_revolt_fired` renvoie donc toujours faux.
 *
 * Membrane : ce module est SIM (il lit des flottants SCPS comme diplo/prosperity).
 * Son API ne renvoie que des nombres de JEU (int 0-100, −100..100) — jamais un
 * flottant SCPS ni son nom. Le renderer peut donc l'appeler sans franchir la cloison.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_prosperity.h"
#include "scps_legitimacy.h"
#include "scps_diplo.h"
#include "scps_routes.h"

/* ---- Diplomates -------------------------------------------------------- */
#define SC_MAX_DIPLOMATS   12
#define SC_BASE_DIPLOMATS  3      /* vivier de départ (avant bonus d'Influence) */

/* ---- Q1 — LE CONSEIL (I7) : 3 sièges, conseillers tier 1-3 ------------- */
#define SC_COUNCIL_SEATS  3    /* 0 = Savoir (+20 % savoir) · 1 = Société (+12 % promo) · 2 = Industrie (+15 % manuf) */
#define SC_COUNCIL_CANDS  3    /* candidats tirés au seed par siège (licenciables) */
#define SC_COUNCIL_NAMES  8    /* taille de la bande STR_COUNCIL_NAME_* (maisons) */

typedef enum {
    DIP_IDLE = 0,
    DIP_RELATIONS,   /* ~180 j — améliore l'opinion (lent, continu)            */
    DIP_ALLIANCE,    /* ~30 j  — ouvre une alliance (si l'opinion le porte)    */
    DIP_INTEGRATE,   /* ∝ D∞   — accélère la montée de Légitimité d'une région */
    DIP_ROUTE        /* ~90 j  — ouvre une route commerciale                   */
} DipMission;

typedef struct {
    DipMission mission;
    int        target;       /* pays (RELATIONS/ALLIANCE) | région (INTEGRATE/ROUTE) */
    int        home_region;  /* d'où part l'agent (capitale) */
    int        days_left;
} Diplomat;

typedef struct {
    Diplomat agents[SC_MAX_DIPLOMATS];
    int      count;          /* vivier disponible = base + bonus(Influence) */
} DiplomaticStaff;

/* ---- L'état de statecraft ---------------------------------------------- */
typedef struct {
    float           influence[SCPS_MAX_COUNTRY];                 /* 0..100, inertie */
    float           prestige [SCPS_MAX_COUNTRY];                 /* mémoire des accords (0..30) */
    float           opinion  [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];/* −100..100 (EFFECTIF, lissé) */
    /* #26 — la MÉMOIRE DURABLE des actes : ledger ±(borné) de ce que `a` retient de `b`
     * (trahison, embargo, alliance TENUE, guerre endurée, frères d'armes). Décroît sur une
     * génération ; PÈSE sur la cible vers laquelle `opinion` converge → l'opinion a une
     * mémoire (≠ la projection memoryless d'avant). Sérialisé (blob SVT_STAT). */
    float           opinion_mem[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    DiplomaticStaff staff    [SCPS_MAX_COUNTRY];
    float           agitation  [SCPS_MAX_REG];                   /* 0..100 soutenue (SEUL champ vivant) */
    float           unrest_days[SCPS_MAX_REG];                   /* INERTE (dédup Option B) : jamais accumulé.
                                                                   * Champ conservé pour NE PAS bumper SAVE_VERSION. */
    bool            revolt_fired[SCPS_MAX_REG];                  /* INERTE (dédup Option B) : jamais écrit vrai —
                                                                   * scps_revolt.c est le SEUL acteur de révolte.
                                                                   * Champ conservé pour NE PAS bumper SAVE_VERSION. */
    int8_t          council[SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS]; /* Q1 : slot pourvu par siège (-1 = vacant) */
    int8_t          council_gen[SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS]; /* GÉNÉRATION du ministre ASSIS (identité
                                                                      * épinglée : la pool tourne, lui vieillit ;
                                                                      * -1 = vacant). v49. */
    int             n_countries;
} Statecraft;

/* Q1 — LE CONSEIL. Les candidats (tier 1-3 + nom + ÂGE) sont DÉTERMINISTES (dérivés du seed)
 * → régénérés au chargement, jamais sérialisés ; persistent le siège POURVU (council[]) et
 * sa GÉNÉRATION (council_gen[]). La POOL se RENOUVELLE par génération (année/GEN_YEARS) :
 * toujours SC_COUNCIL_CANDS candidats par siège, jamais épuisée — gen 0 laisse la graine
 * INTACTE (hash identique à l'ancien monde). L'ÂGE = base 30-51 (hash) + années écoulées
 * dans la génération : il GRANDIT avec l'année ; la RETRAITE (66-73 ans, jitter par identité)
 * vide le siège — premier départ possible an 16 > fenêtre golden (12 ans). Les
 * multiplicateurs restent des LECTEURS (×savoir/×promo/×manuf), jamais des poses. */
#define SC_COUNCIL_GEN_YEARS 20   /* longueur d'une génération de pool (années) */
int   statecraft_council_gen      (int year);                                     /* génération de pool courante */
int   statecraft_council_cand_tier(uint32_t seed, int cid, int seat, int slot, int gen); /* effet : 1/2/3 → ×1 / ×1.5 / ×2 */
int   statecraft_council_cand_name(uint32_t seed, int cid, int seat, int slot, int gen); /* StrId du nom (maison) */
int   statecraft_council_cand_age (uint32_t seed, int cid, int seat, int slot, int gen, int year); /* 30-51 + années */
int   statecraft_council_seated   (const Statecraft *sc, int cid, int seat);      /* slot pourvu, -1 sinon */
int   statecraft_council_seated_gen(const Statecraft *sc, int cid, int seat);     /* génération du ministre assis (0 si legacy) */
int   statecraft_council_seated_age(const Statecraft *sc, uint32_t seed, int cid, int seat, int year); /* -1 si vacant */
void  statecraft_council_hire     (Statecraft *sc, int cid, int seat, int slot, int gen);
void  statecraft_council_dismiss  (Statecraft *sc, int cid, int seat);
/* LES ANNÉES PASSENT (annuel) : tout ministre à l'âge de la retraite VIDE son siège —
 * l'IA repourvoit au mois suivant (statecraft_council_ai), le joueur par l'UI. */
void  statecraft_council_age_tick (Statecraft *sc, uint32_t seed, int year);
float statecraft_council_seat_mult(const Statecraft *sc, uint32_t seed, int cid, int seat); /* 1+base·effet ; 1 si vacant */
float statecraft_council_cost     (const Statecraft *sc, uint32_t seed, int cid, float ipm); /* or/mois total (×IPM) */
float statecraft_council_cand_cost(uint32_t seed, int cid, int seat, int slot, int gen, float ipm); /* coût d'UN candidat (×IPM), pour l'UI */
/* Applique le conseil à l'éco pour le tick (mensuel) : pousse les multiplicateurs LECTEURS
 * et ponctionne le coût (×IPM) sur le trésor de la capitale, ligne FX_CONSEIL. Appelé
 * IDENTIQUEMENT par viewer ET chronicle (mêmes décisions de monde). dt_year = 1/12. */
void  statecraft_council_apply    (const Statecraft *sc, const World *w, WorldEconomy *e, uint32_t seed, float dt_year);
/* L'IA pourvoit le siège que son éthos privilégie, dans la garde de budget (no-op sinon).
 * `year` : la pool évaluée est celle de la génération COURANTE. */
void  statecraft_council_ai       (Statecraft *sc, const World *w, const WorldEconomy *e, uint32_t seed, int cid, int year);

void statecraft_init(Statecraft *sc, const World *w);

/* ---- Lecteurs (nombres de JEU, jamais un flottant SCPS) ---------------- */
int  statecraft_influence      (const Statecraft *sc, int cid);          /* 0..100 */
/* Variation d'INFLUENCE par JOUR (convergence vers le standing prospérité+taille+
 * prestige) — pour le bandeau (Influence accumulable + flux +N/j). */
float statecraft_influence_flux(const Statecraft *sc, const WorldEconomy *econ,
                                const WorldProsperity *wp, int cid);
int  statecraft_opinion        (const Statecraft *sc, int a, int b);     /* −100..100 */

/* ---- #26 — le RÉSUMÉ d'opinion (UI) : les COMPOSANTES de la cible d'opinion de `a`
 * envers `b` — mémoire d'actes (durable) + modificateurs de STATUT actifs. L'opinion
 * COURANTE (statecraft_opinion) CONVERGE vers leur somme (lissage SC_OPINION_RATE) :
 * total ≠ somme en transitoire, c'est voulu. Mêmes lectures que le bloc du tick. */
typedef struct {
    float mem;                                /* mémoire des actes (trahison, sécession…) */
    float ally, war, vassal, pact, embargo;   /* statuts actifs (0 = inactif) */
    float rancor;                             /* rivalité territoriale (−) */
} OpinionParts;
void statecraft_opinion_parts(const Statecraft *sc, const DiploState *diplo,
                              int a, int b, OpinionParts *out);
int  statecraft_missions_cap   (const Statecraft *sc, int cid);          /* plafond simultané */
int  statecraft_missions_active(const Statecraft *sc, int cid);
int  statecraft_agitation      (const Statecraft *sc, int region);       /* 0..100 soutenue */
/* INERTE (dédup Option B) : statecraft ne fait plus fire de révolte — renvoie
 * toujours faux. Conservée pour compat d'API (l'appelant feed du sim ne pousse
 * donc plus jamais le FEED_REVOLT générique par cette voie). */
bool statecraft_revolt_fired   (const Statecraft *sc, int region);

/* ---- Événements qui déplacent l'Influence (la réputation suit l'acte) --- */
void statecraft_on_accord_kept(Statecraft *sc, int cid);   /* alliance tenue, paix honorée */
void statecraft_on_betrayal   (Statecraft *sc, int cid);   /* trahir un allié : chute nette */
/* #26bis — la MÉMOIRE DU SÉCESSIONNISTE : le pays né d'une guerre civile garde une dent
 * DURABLE contre l'empire père (opinion_mem fils→père ; s'estompe comme la trahison). */
void statecraft_on_secession  (Statecraft *sc, int child, int parent);

/* ---- Missions : occupe un agent pendant des JOURS ---------------------- *
 * false si le vivier est saturé (plafond d'Influence) ou la cible invalide.  */
bool statecraft_send(Statecraft *sc, const World *w, const WorldEconomy *econ,
                     int cid, DipMission mission, int target);

/* ---- Un pas (jours) ---------------------------------------------------- *
 * Influence → standing (prospérité+taille+prestige) ; opinion → relation+histo ;
 * agents avancent et appliquent leur effet à l'échéance ; agitation lissée (SIGNAL
 * pur, dédup Option B — plus d'allumage de révolte ici). econ/diplo/wl/rn peuvent
 * être mutés par l'effet de mission mûrie (route/alliance/intégration). */
void statecraft_tick(Statecraft *sc, World *w, WorldEconomy *econ,
                     WorldProsperity *wp, WorldLegitimacy *wl,
                     DiploState *diplo, RouteNetwork *rn, int days);

const char *dip_mission_name(DipMission m);

#endif /* SCPS_STATECRAFT_H */
