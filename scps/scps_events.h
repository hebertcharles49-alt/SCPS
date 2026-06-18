#ifndef SCPS_EVENTS_H
#define SCPS_EVENTS_H
/*
 * scps_events.h — ÉVÈNEMENTS & ÂGES : la dynamique du monde (§ chocs/politique/ères)
 *
 * Approche EU4, mais rien d'aléatoire hors-sol — tout est ANCRÉ :
 *   - les CHOCS suivent la GÉOGRAPHIE (la tectonique qui a fait les montagnes,
 *     les rivières, la pluviométrie, les routes) ;
 *   - les évènements POLITIQUES/CULTURELS suivent la FICHE (éthos/sphère/credo) :
 *     un même état du monde produit un récit différent par culture ;
 *   - les ÂGES suivent l'ÉTAT DU MONDE : un âge n'arrive pas à date, il est
 *     RECONNU quand le monde atteint un état émergent (et déplace une coordonnée
 *     globale + ouvre un palier de tech).
 *
 * Membrane : les effets déplacent des COORDONNÉES (K/H/L/food…) ou des MÉTRIQUES
 * (agitation, influence) ; les TEXTES parlent en mots diégétiques — jamais un
 * nom SCPS. (Les champs de delta sont du code, pas de l'affichage.)
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_statecraft.h"
#include "scps_routes.h"
#include "scps_tech.h"
#include "scps_diplo.h"   /* §F : le directeur lit les guerres (T) et l'Amnistie ÉPONGE la rancune */

/* ===================================================================== */
/* CADRE D'ÉVÈNEMENT (data-driven)                                        */
/* ===================================================================== */
typedef enum { EV_PROVINCE = 0, EV_COUNTRY, EV_WORLD } EvScope;

/* L'effet d'une option : un paquet de déplacements de coordonnées/métriques.
 * 0 = neutre ; pop_mult = 1 = neutre. Rien ici n'est un « +X % » de jeu : ce
 * sont des coordonnées que le moteur relit (build/L/agitation/treasury…). */
typedef struct {
    float d_K_inst;     /* institutions bâties (région)      */
    float d_H_coerc;    /* coercition bâtie (région)         */
    float d_food_cap;   /* nourriture/fertilité (région)     */
    float d_L;          /* légitimité (région ou pays)       */
    float d_coercion;   /* coercition vécue (région, [0..1]) */
    float d_agitation;  /* agitation (métrique, région)      */
    float pop_mult;     /* multiplie la population (région)  */
    float d_treasury;   /* trésor (région/capitale)          */
    float d_influence;  /* influence (pays)                  */
    /* Échelle monde (Âges) : */
    float d_C_global;   /* connectivité mondiale             */
    float d_breach;     /* pression de brèche mondiale       */
    int   unlock_branch;/* TechTheme ouvert (-1 = aucune)  */
    int   unlock_tier;  /* palier ouvert                     */
} EvEffect;

typedef struct {
    const char *label;   /* le choix, en mots diégétiques */
    const char *blurb;   /* ce qu'il fait, en mots (jamais un nom SCPS) */
    EvEffect    eff;
    float       ai_chance;/* poids du choix pour l'IA (la fiche l'a déjà sélectionné) */
} EvOption;

typedef struct EventCtx EventCtx;   /* fwd : bundle de pointeurs systèmes */

typedef struct {
    int         id;
    EvScope     scope;
    const char *name;                                 /* titre diégétique */
    bool      (*trigger)(const EventCtx *, int subject);/* lit géo/coords/métriques/fiche */
    float       mtth_days;                            /* temps moyen avant déclenchement */
    float     (*mtth_mod)(const EventCtx *, int subject);/* facteurs d'accélération (ou NULL) */
    EvOption    options[4];
    int         n_options;
} EventDef;

/* Identifiants (chocs géo puis politiques/culturels par fiche). */
typedef enum {
    EVID_QUAKE = 0, EVID_FLOOD, EVID_DROUGHT, EVID_FIRE, EVID_PLAGUE,
    EVID_INTEG_DOMINATEUR, EVID_INTEG_MERCANTILE, EVID_INTEG_BUREAUCRATE, EVID_INTEG_ANCIEN,
    EVID_SUCCESSION, EVID_SCHISM,
    EVID_XENOPHILE,        /* floraison cosmopolite : diversité + éthos accueillant + paix */
    EVID_XENOPHOBE,        /* cohésion du foyer : homogénéité + éthos martial (Dominateur/Honneur) */
    EVID_COUNT
} EvId;

/* ===================================================================== */
/* LE DIRECTEUR D'ÉVÉNEMENTS (§F) — stabilise / déstabilise, sans s'acharner */
/* ===================================================================== */
/* Les 14 événements DIRIGÉS : 7 déstabilisateurs (le monde ronronne → on remue)
 * puis 7 stabilisateurs (le monde brûle → on apaise). Chacun = un CHOC sur des
 * variables EXISTANTES (P/C/I/H/K/L/fertilité/or/foi/fracture/rancune), aucun
 * système neuf — le motif world_events_tick/apply_effect est réemployé tel quel. */
typedef enum {
    DIR_CHARISMA = 0, DIR_PESTE, DIR_PALAIS, DIR_FILON, DIR_ANNEE, DIR_SCHISME, DIR_DEBASE,
    DIR_CONCILE, DIR_REFORME, DIR_CADASTRE, DIR_MOISSONS, DIR_MARCHAND, DIR_HEROS, DIR_AMNISTIE,
    DIR_EV_COUNT
} DirEvId;
#define DIR_STAB_FIRST DIR_CONCILE   /* [0..DIR_STAB_FIRST) déstabilisent ; [DIR_STAB_FIRST..) apaisent */

/* ===================================================================== */
/* §G2 — LE DIRECTEUR-AMPLITUDE (« tale ») : traumatisme → amplitude → présage */
/* ===================================================================== */
/* Une MÉMOIRE durable du directeur : un fait NOTABLE (événement marquant ou âge)
 * laisse une trace horodatée qui, plus tard, nourrit un AUGURE/PRÉSAGE. Tout en
 * jours de jeu — déterministe, sérialisable (un anneau de taille fixe). */
#define DIR_MEM_CAP 16                 /* l'anneau de mémoire : 16 hauts faits récents */
/* La nature d'un haut fait mémorisé (lisible par le présage à venir). */
typedef enum { DMEM_NONE=0, DMEM_DESTAB, DMEM_STAB, DMEM_AGE, DMEM_KIND_COUNT } DirMemKind;
typedef struct {
    int   day;           /* le jour où le fait s'est inscrit (0 = case vide) */
    int   kind;          /* DirMemKind (la nature du haut fait) */
    int   subject;       /* pays/région/continent concerné (UI/présage), -1 = monde */
    float weight;        /* poids dramatique du fait (∝ amplitude au moment) */
} DirMemory;

/* L'état du directeur : cadence, anti-acharnement (F2), télémétrie (F5), et le
 * DIRECTEUR-AMPLITUDE (§G2 : intégrateur de trauma, budget, mémoire, présages).
 * Tout est en JOURS de jeu (cumul des ticks) — déterministe, sérialisable. */
typedef struct {
    int     next_check_day;                       /* le directeur scanne à cette échéance (~annuel) */
    int     prov_cd_until [SCPS_MAX_REG];          /* anti-acharnement : province en repos jusqu'à ce jour (15 ans) */
    int     pays_cd_until [SCPS_MAX_COUNTRY];      /* pays en repos jusqu'à ce jour (5 ans) */
    signed char prov_last_neg[SCPS_MAX_REG];       /* 1 si le dernier ciblage de cette province fut NÉGATIF (jamais deux d'affilée) */
    int     fam_active_until[DIR_EV_COUNT];        /* G0.1 : même ÉVÉNEMENT ne rejoue pas avant ≥15 ans (monde) */
    int     cont_cd_until[SCPS_MAX_CONTINENT];     /* G0.1 : événement continent-large ≥25 ans sur le même continent */
    int     last_fired_day[DIR_EV_COUNT];          /* G0.1 : tirage sans remise — poids ÷4 pendant 30 ans après un tir */
    unsigned char prov_neg_century[SCPS_MAX_REG];  /* nb d'événements négatifs subis dans le siècle courant (preuve F2 : ≤3) */
    int     century_base_day;                      /* début du siècle courant (remet prov_neg_century à zéro) */
    int     fired[DIR_EV_COUNT];                   /* télémétrie : occurrences par événement */
    int     fired_stab, fired_destab;
    int     neg_over_cap;                          /* fois où une province a dépassé 3 négatifs/siècle (DOIT rester 0) */
    float   last_T;                                /* dernière température mondiale [0..1] (UI/télémétrie) */
    float   max_T;                                 /* G0.1 : T maximale atteinte (preuve : ≥1 stabilisateur si > 0.5) */
    /* §G2 — LE DIRECTEUR-AMPLITUDE (la boucle « tale »). Tout déterministe. */
    float   adapt_days;       /* INTÉGRATEUR DE TRAUMATISME : monte sous les chocs (T), redescend au calme. [0..AMPL_TRAUMA_MAX] */
    float   budget;           /* points de mise en scène accumulés (∝ pop+richesse+temps), dépensés en présages */
    float   amplitude;        /* dernière amplitude dramatique [0..1] = f(adapt_days) (UI/télémétrie) */
    float   max_amplitude;    /* pic d'amplitude atteint (preuve : monte après un choc) */
    DirMemory mem[DIR_MEM_CAP]; /* anneau des hauts faits durables (NOTABLE → MÉMOIRE) */
    int     mem_head;          /* prochaine case d'écriture de l'anneau */
    int     omens;             /* AUGURES/PRÉSAGES émis (un haut fait ressurgit) */
} Director;

/* ===================================================================== */
/* ÂGES — déclenchés par une LECTURE du monde (§4)                        */
/* ===================================================================== */
typedef enum {
    AGE_COMMERCE = 0, AGE_REASON, AGE_EMPIRES, AGE_BREACH,
    /* Âges STRUCTURELS — lisent la crise & les idées, poussent les ENTRÉES du
     * moteur d'ordre (I/L/H), laissent le verdict §2.4 faire les conséquences.
     * Chaîne causale : les Lumières d'abord (société de masse), puis le fork
     * Soulèvements (le consentement renverse) ↔ Ordre de Fer (la poigne écrase). */
    AGE_LUMIERES, AGE_SOULEVEMENTS, AGE_ORDRE_FER,
    AGE_COUNT
} AgeId;

typedef struct {
    bool  dawned[AGE_COUNT];
    bool  tier_open[THM_COUNT][8];   /* paliers de tech ouverts par les âges */
    float breach_pressure;           /* l'endgame faustien mondial */
    float research_mult;             /* palier de la Raison : recherche plus vive */
    float integration_mult;          /* palier des Empires : intégration accélérée */
    int   last_dawned;               /* -1 ou AgeId du dernier avènement */
    int   days_elapsed;              /* temps de jeu écoulé (cumul des ticks) */
    int   last_dawn_year;            /* an du dernier avènement (rythme : 1 âge / 30 ans mini) */
} AgesState;

/* ===================================================================== */
/* L'ÉTAT DU MODULE                                                       */
/* ===================================================================== */
typedef struct {
    float relief;     /* fraction de relief tectonique (montagnes/volcans) [0..1] */
    float river01;    /* débit fluvial max normalisé [0..1] */
    float rainfall;   /* pluviométrie moyenne [0..1] */
    float temp;       /* température moyenne [0..1] */
    float arid;       /* fraction de biomes arides [0..1] */
    float forest;     /* fraction de biomes boisés [0..1] */
    float lowland;    /* fraction de basses terres inondables [0..1] */
    int   cells;
} RegionGeo;

typedef struct {
    RegionGeo   geo[SCPS_MAX_REG];   /* la géo RELUE par région (le worldgen exposé) */
    AgesState   ages;
    Director    director;            /* §F : le directeur d'événements (dirigés) */
    uint32_t    rng;
    const char *last_name;           /* dernier évènement déclenché (UI/journal) */
    int         last_id;
    int         n_fired;             /* total déclenché (debug) */
} EventsState;

/* ===================================================================== */
/* API                                                                   */
/* ===================================================================== */
/* Relit la géo du worldgen en agrégats par région (failles via le relief,
 * rivières, pluie, aridité, forêt). À appeler après gen_population. */
void events_init(EventsState *ev, const World *w, uint32_t seed);

/* La boucle (§5) : chocs géo (à risque) + évènements (mtth par sujet, trigger
 * lit la fiche) + LE DIRECTEUR (§F) + scan d'âges. Mute le monde via les leviers.
 * `dp` peut être NULL (le directeur tourne sans le terme « guerres » de T et sans
 * l'Amnistie). */
void world_events_tick(EventsState *ev, World *w, WorldEconomy *econ,
                       WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                       RouteNetwork *rn, const TechState ts[], DiploState *dp, int days);

/* ---- LE DIRECTEUR (§F) : lecteurs (UI / télémétrie) -------------------- */
float       director_temperature(const EventsState *ev);          /* dernière T [0..1] (0 ronron, 1 chaos) */
const char *director_event_name(int dir_id);                      /* nom diégétique d'un événement dirigé */
int         director_fired(const EventsState *ev, int dir_id);     /* occurrences d'un événement dirigé */
bool        director_is_destab(int dir_id);                        /* déstabilisateur ? */

/* ---- §G2 LE DIRECTEUR-AMPLITUDE : lecteurs (UI / télémétrie) ----------- */
/* L'AMPLITUDE dramatique [0..1] dérivée du traumatisme intégré : HAUTE juste
 * après un choc (le monde « vibre »), BASSE au calme (le récit s'apaise). */
float       director_amplitude(const EventsState *ev);
/* L'intégrateur de traumatisme brut (jours de tension cumulés, [0..max]). */
float       director_adapt_days(const EventsState *ev);
/* Le budget de mise en scène accumulé (∝ pop·richesse·temps), en points. */
float       director_budget(const EventsState *ev);
/* Combien d'AUGURES/PRÉSAGES ont été émis (la boucle « tale » a bouclé). */
int         director_omens(const EventsState *ev);
/* Combien de hauts faits l'anneau de mémoire porte (faits NOTABLES retenus). */
int         director_memories(const EventsState *ev);

/* §G2 — un PAS de l'amplitude exposé pour le BANC (déterministe) : intègre le
 * traumatisme depuis une température `T` donnée, recalcule amplitude/budget,
 * et émet au plus un présage. `pop`/`gold` dimensionnent le budget (monde
 * riche/peuplé ⇒ plus de budget) ; `days` = pas de temps. Retour : true si un
 * présage a été émis ce pas. (La sim appelle l'équivalent INTERNE via le tick.) */
bool        director_amplitude_step(EventsState *ev, float T, double pop, double gold, int days);

/* §G2 — REVALIDATION du directeur-amplitude désérialisé (save_sane l'appelle).
 * mem_head BORNE l'écriture dans l'anneau mem[DIR_MEM_CAP], chaque mem.kind est
 * une étiquette [0..DMEM_KIND_COUNT) et mem.subject un index que le présage relit :
 * une valeur folle (save forgé) est REFUSÉE. true = sain. `max_subject` = la borne
 * haute admise pour subject (l'appelant la connaît : SCPS_MAX_COUNTRY² couvre
 * l'encodage Amnistie a·MAX+b). Exposé pour test headless au banc. */
bool        director_save_sane(const EventsState *ev, int max_subject);

/* ---- Lecteurs de RISQUE géo (relisent la géo ; 0 = la géo l'interdit) -- */
float events_quake_risk  (const EventsState *ev, int region);
float events_flood_risk  (const EventsState *ev, int region);
float events_drought_risk(const EventsState *ev, int region);
float events_fire_risk   (const EventsState *ev, int region);

/* Applique un choc géo précis à une région (effets ancrés). Pour la sim et le
 * banc d'essai (déterministe). */
void  events_strike(EventsState *ev, World *w, WorldEconomy *econ,
                    WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                    int region, EvId shock);

/* Peste : diffusion BFS le long des ROUTES ouvertes depuis une région-foyer.
 * Un empire fermé (sans routes) est épargné. Renvoie le nb de régions touchées. */
int   events_plague_spread(EventsState *ev, World *w, WorldEconomy *econ,
                           WorldLegitimacy *wl, Statecraft *sc, RouteNetwork *rn,
                           int seed_region);

/* ---- Évènements politiques/culturels : la VARIANTE par la fiche -------- */
/* Renvoie l'EvId de l'évènement d'intégration qui MATCHE la fiche du pays
 * propriétaire d'une région instable (ou -1). Même état → variante par éthos. */
int          events_match_political(const EventsState *ev, World *w, WorldEconomy *econ,
                                    WorldLegitimacy *wl, Statecraft *sc, int region);
const EventDef *event_def(int evid);

/* ---- Âges : scan d'interprétation du monde ---------------------------- */
/* Évalue les triggers d'âge ; fait advenir tout âge nouvellement éligible
 * (palier + coordonnée globale). Renvoie true si un âge est advenu. */
bool  events_check_ages(EventsState *ev, World *w, WorldEconomy *econ,
                        WorldProsperity *wp, WorldLegitimacy *wl, const TechState ts[]);
bool  ages_dawned(const EventsState *ev, AgeId a);
bool  ages_tier_open(const EventsState *ev, TechTheme br, int tier);
float ages_breach_pressure(const EventsState *ev);
const char *age_name(AgeId a);
/* Verdict du MOTEUR agrégé : combien de pays sont en mode révolutionnaire
 * (SI<5 & pression≥fracture) — la masse critique des Soulèvements, sans aucun
 * code de révolution dédié. */
int   events_count_revolutionary(const World *w, const WorldProsperity *wp);

/* ---- Garde-fou membrane : aucun nom SCPS dans les textes joueur -------- */
bool  events_text_clean(void);

#endif /* SCPS_EVENTS_H */
