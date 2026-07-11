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
#include "scps_factions.h"   /* V2a : EthosFaction — la faction de chaque siège du Conseil */

/* ---- Diplomates -------------------------------------------------------- */
#define SC_MAX_DIPLOMATS   12
#define SC_BASE_DIPLOMATS  3      /* vivier de départ (avant bonus d'Influence) */

/* ---- Q1 — LE CONSEIL (I7) : 3 sièges, conseillers tier 1-3 ------------- */
#define SC_COUNCIL_SEATS  3    /* 0 = Savoir (+12 % recherche) · 1 = Royaume/Société (+15 % promo) · 2 = Ouvrages/Industrie (+20 % manuf) */
#define SC_COUNCIL_CANDS  3    /* candidats tirés au seed par siège (licenciables) */
#define SC_COUNCIL_NAMES  8    /* taille de la bande STR_COUNCIL_NAME_* (maisons, ancien format) */
/* P0-4 — PERSONNE + MAISON (docs/CONSEIL_ORIENTATIONS_2026-07-10.md). Tirages
 * INDÉPENDANTS (salts distincts de tier/nom/âge/faction), tables LOCALES en
 * C brut (pas de STR_* : strings_ids.h/strings_en.h sont hors du périmètre de
 * cette mission — cf. TROUVAILLES.md, un futur agent façade migrera si besoin). */
#define SC_COUNCIL_FIRSTNAMES 24
#define SC_COUNCIL_HOUSES     12

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
    /* V2a — LE CONSEIL VIVANT : par siège POURVU, une LOYAUTÉ (0-100, CONVERGE vers
     * une cible — jamais un saut) et un curseur de PAIE (0-2, multiplie le coût). v70. */
    float           loyalty[SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS];   /* 0..100 */
    float           pay    [SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS];   /* 0..2, défaut 1.0 */
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
/* P2 — LA FACTION RÉELLE DU TITULAIRE D'UN SIÈGE (EthosFaction en int, -1 = vacant) —
 * pour les hooks DYNAMIQUES des évènements du Conseil (docs/CONSEIL_ORIENTATIONS_2026-07-10.md). */
int   statecraft_council_seat_faction(const Statecraft *sc, uint32_t seed, int cid, int seat);
/* V2a : `seed` sert à dériver la FACTION du candidat pourvu/renvoyé — RECRUTER
 * pousse SA faction (faction_lever_apply, ~0.10) ; RENVOYER froisse (grief sur
 * les factions qui S'OPPOSENT à la sienne — même canal que la concession/le
 * levier, jamais un canal parallèle). */
void  statecraft_council_hire     (Statecraft *sc, uint32_t seed, int cid, int seat, int slot, int gen);
void  statecraft_council_dismiss  (Statecraft *sc, uint32_t seed, int cid, int seat);
/* LES ANNÉES PASSENT (annuel) : tout ministre à l'âge de la retraite VIDE son siège —
 * l'IA repourvoit au mois suivant (statecraft_council_ai), le joueur par l'UI. */
void  statecraft_council_age_tick (Statecraft *sc, uint32_t seed, int year);
float statecraft_council_seat_mult(const Statecraft *sc, uint32_t seed, int cid, int seat); /* 1+base·effet ; 1 si vacant */
float statecraft_council_cost     (const Statecraft *sc, uint32_t seed, int cid, float ipm); /* or/mois total (×IPM) */
float statecraft_council_cand_cost(uint32_t seed, int cid, int seat, int slot, int gen, float ipm); /* coût d'UN candidat (×IPM), pour l'UI */
/* Applique le conseil à l'éco pour le tick (mensuel) : pousse les multiplicateurs LECTEURS
 * (bonus final = bonus de rang × efficacité — P1-1, LIT wp->country[cid].K, jamais une
 * approximation depuis les bâtiments) et ponctionne le coût (×IPM) sur le trésor de la
 * capitale, ligne FX_CONSEIL. Appelé IDENTIQUEMENT par viewer ET chronicle (mêmes
 * décisions de monde). dt_year = 1/12. `wp` peut être NULL (K=0, dégrade proprement). */
void  statecraft_council_apply    (const Statecraft *sc, const World *w, WorldEconomy *e,
                                   const WorldProsperity *wp, uint32_t seed, float dt_year);
/* L'IA pourvoit le siège que son éthos privilégie, dans la garde de budget (no-op sinon).
 * `year` : la pool évaluée est celle de la génération COURANTE. */
void  statecraft_council_ai       (Statecraft *sc, const World *w, const WorldEconomy *e, uint32_t seed, int cid, int year);
/* Télémétrie (chronique) : combien de fois l'IA a REMPLACÉ un ministre au bord
 * (betrayal_ready) — compteur GLOBAL du module, RAZ par statecraft_init (comme
 * les autres compteurs de sim, ex. revolt_civilwar_count). */
long  statecraft_council_ai_replace_count(void);

/* ═══ V2a — LE CONSEIL VIVANT : faction, loyauté, paie ════════════════════════
 * Chaque conseiller PENCHE vers une faction-éthos (attribution DÉTERMINISTE par
 * (siège, maison) — rien à sérialiser). Sa LOYAUTÉ (0-100, par siège POURVU)
 * CONVERGE vers une cible dérivée de la satisfaction de SA faction (1−grief) et de
 * la PAIE ; jamais de saut. Le « rot » (faction_capture_total) accélère la CHUTE,
 * jamais la remontée (la corruption aide à tomber, pas à se refaire une vertu). */

/* P0-1 — LA FACTION D'UN CANDIDAT : plus de spectre par siège (les 6 factions sont
 * candidates sur les 3 sièges). Par (siège, génération) : un mélange DÉTERMINISTE
 * des 6 factions (seed×pays×siège×génération) — les SC_COUNCIL_CANDS premières du
 * mélange sont les 3 candidates du siège, TOUJOURS distinctes (préfixe d'une
 * permutation). Re-tirage à chaque génération (le mélange dépend de `gen`). */
EthosFaction statecraft_council_faction(uint32_t seed, int cid, int seat, int slot, int gen);

/* P0-4 — PERSONNE + MAISON : deux tirages INDÉPENDANTS (salts distincts l'un de
 * l'autre ET de tier/âge/faction) — le prénom et la maison d'un candidat ne se
 * déduisent d'aucun autre trait. `statecraft_council_cand_name` (StrId, 8 maisons
 * historiques) reste INCHANGÉE pour compat façade ; ces deux lecteurs ADDITIFS
 * donnent la paire complète (« Aveline » + « Vœrn » → « Aveline Vœrn »). */
const char *statecraft_council_cand_firstname(uint32_t seed, int cid, int seat, int slot, int gen);
const char *statecraft_council_cand_house    (uint32_t seed, int cid, int seat, int slot, int gen);
/* raccord 7 — le genre du candidat (même hash que le prénom, index<12 = masculin) */
bool        statecraft_council_cand_female   (uint32_t seed, int cid, int seat, int slot, int gen);

/* Lecteurs — loyauté (0..100) et curseur de paie (0..2, 1.0 = normal) du siège
 * POURVU (0/1.0 par défaut si vacant — lu par convention, jamais accédé nu). */
int   statecraft_council_loyalty  (const Statecraft *sc, int cid, int seat);       /* 0..100 */
float statecraft_council_pay      (const Statecraft *sc, int cid, int seat);       /* 0..2 */
void  statecraft_council_set_pay  (Statecraft *sc, int cid, int seat, float pay);  /* verbe : le curseur de paie */
/* P3 — écrivain DIRECT de loyauté (borné 0-100), pour la mission décennale
 * (réussite/échec) — n'affecte QUE le siège pourvu (no-op si vacant). */
void  statecraft_council_loyalty_add(Statecraft *sc, int cid, int seat, float delta);

/* P1-1 — EFFICACITÉ POLITIQUE (0.50-1.15) : clamp(BASE + K_PER·K + LOY_W·loyauté/100
 * − CORRUPTION_PER_POINT·Corruption(faction_corruption_0_100), MIN, MAX). 1.0 (base
 * neutre) si le siège est VACANT — rien à multiplier. `wp` peut être NULL (K=0). */
float statecraft_council_efficiency(const Statecraft *sc, const WorldProsperity *wp, int cid, int seat);

/* SIGNAUX pour V2b (pas les événements eux-mêmes) : */
/* Le ministre est-il À L'AGONIE (loyauté ≤ seuil ~15) depuis au moins N mois ? —
 * un booléen dérivé de la loyauté COURANTE (rien à sérialiser de plus qu'elle). */
bool  statecraft_council_betrayal_ready(const Statecraft *sc, int cid, int seat);
typedef enum { COUNCIL_PAIR_NEUTRE=0, COUNCIL_PAIR_RIVALITE, COUNCIL_PAIR_ALLIANCE, COUNCIL_PAIR_CONSPIRATION } CouncilPairState;
/* L'état d'une PAIRE de sièges pourvus (a,b) du même pays : RIVALITÉ (factions
 * opposées, tous deux en poste depuis longtemps) · ALLIANCE (factions proches,
 * grief bas) · CONSPIRATION (les DEUX factions aliénées — grief haut). V2b
 * branchera les événements dessus (trahisons, complots). */
CouncilPairState statecraft_council_pair_state(const Statecraft *sc, const World *w, const WorldEconomy *econ,
                                               uint32_t seed, int cid, int a, int b, int year);

/* Un pas de LOYAUTÉ (mensuel, appelé depuis statecraft_council_apply) : fait
 * converger chaque siège pourvu vers sa cible, taux asymétrique par le rot. */
void  statecraft_council_loyalty_tick(Statecraft *sc, const World *w, const WorldEconomy *econ,
                                      uint32_t seed, float dt_year);

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
