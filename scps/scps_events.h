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
#include "scps_endgame.h" /* V2b LOT 1 : la Merveille en 3 étapes (merv/MervPhase/metab_count) */
#include "scps_missions.h" /* raccord 7 : l'Âge des Héros naît d'une mission décennale réussie */

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
    float d_treasury;   /* trésor (région/capitale) — montant FIXE (événements historiques) */
    float d_influence;  /* influence (pays)                  */
    /* Échelle monde (Âges) : */
    float d_C_global;   /* connectivité mondiale             */
    float d_breach;     /* pression de brèche mondiale       */
    int   unlock_branch;/* TechTheme ouvert (-1 = aucune)  */
    int   unlock_tier;  /* palier ouvert                     */
    /* MEMBRANE DE DÉCISION — trésor PROPORTIONNEL AU REVENU (les évènements NEUFS l'utilisent
     * à la place d'un montant fixe ; SIGNÉ — un choix peut aussi RAPPORTER) : fraction du
     * revenu MENSUEL du pays (Σtaxes de l'an écoulé / 12), déflaté par econ_world_ipm — un même
     * choix pèse pareil en monnaie constante quel que soit l'âge de la partie. Résolu à
     * l'application (apply_effect), routé econ_region_treasury_add. 0 = neutre (défaut). */
    float d_treasury_mois;
} EvEffect;

/* ===================================================================== */
/* CICATRICES DE DÉCISION — un choix laisse une trace qui MÛRIT (§ boucle) */
/* ===================================================================== */
/* La nature d'une cicatrice posée par un choix (consommée par le trigger d'un
 * évènement ultérieur — la boucle « ce choix aura des conséquences »).
 * ⚠ SCAR_NONE=0 EN TÊTE (piège du memset/de l'initialisation implicite) : les 13
 * évènements existants n'initialisent JAMAIS leur `.hook` (struct EvChoiceHook
 * omis de leur initialisation désignée) → `hook.scar_kind` vaut implicitement 0.
 * Si 0 désignait une VRAIE cicatrice (SCAR_SABOTAGE_CHANTIER), CHAQUE tir de
 * QUAKE/FLOOD/SUCCESSION/… en poserait une par accident (piégé en sweep : 212
 * cicatrices pour 4 vrais choix « Envoyer les prévôts » avant ce fix). 0 = NONE
 * rend l'initialisation implicite SÛRE par construction — aucun code appelant
 * n'a besoin d'écrire explicitement .scar_kind=-1 pour être sans-cicatrice. */
typedef enum {
    SCAR_NONE = 0,
    SCAR_SABOTAGE_CHANTIER, SCAR_DETTE_OBEISSANCE, SCAR_RANCUNE_MARCHANDE,
    SCAR_RADICALISATION, SCAR_DEFENSE_AFFAIBLIE, SCAR_SCANDALE_SANITAIRE,
    SCAR_FLOOD_AMORCE, SCAR_FUITE_CERVEAUX, SCAR_EXEMPTION_ACHETEE,
    SCAR_CAPTURE_NOBLE, SCAR_PROLIFERATION_ARME, SCAR_LEGION_FACTION,
    SCAR_REVOLTE_SERVILE, SCAR_FRACTURE_CULTURELLE,
    /* ═══ CONTENU W2 (lot 2) — cicatrices neuves (§A tech · §D chaînage) ═══ */
    SCAR_EVEIL_SOMMEIL,     /* A5 : « Refermer » l'Éveil — ce qui dort se souvient */
    SCAR_KIND_COUNT
} ScarKind;

/* Un HOOK optionnel accroché à une option : levier de faction + cicatrice + répit
 * (anti-relance immédiate). Défauts zéro/NULL (faction=-1, scar_kind=SCAR_NONE=0,
 * cooldown=0) → les 13 évènements existants compilent SANS retouche (initialisation
 * désignée) ET restent SANS EFFET de bord (SCAR_NONE=0 est la valeur implicite d'un
 * hook omis — voir le commentaire de ScarKind pour le piège que ça évite). */
typedef struct {
    int   faction;          /* EthosFaction visée (-1 = aucune) */
    float faction_strength; /* >0 = faction_lever_apply (un vote) ; 0 = faction_concede (une capture) */
    int   scar_kind;        /* ScarKind pointée par ce choix (SCAR_NONE = aucune) */
    int   cooldown_days;    /* répit avant que CE MÊME évènement puisse retirer sur ce sujet (0 = aucun) */
} EvChoiceHook;

typedef struct {
    const char *label;   /* le choix, en mots diégétiques */
    const char *blurb;   /* ce qu'il fait, en mots (jamais un nom SCPS) */
    EvEffect    eff;
    float       ai_chance;/* poids du choix pour l'IA (la fiche l'a déjà sélectionné) */
    EvChoiceHook hook;    /* faction/cicatrice/cooldown de CE choix (défaut : rien) */
    const char *flavor;   /* ce que RACONTE ce choix (tooltip du bouton) — NULL = aucun */
    /* LE PARI (§ contenu W1) — un choix à issue INCERTAINE : APRÈS l'effet CERTAIN
     * (`eff`), un tirage frand(&ev->rng) < gamble_p applique `gamble_eff` EN PLUS
     * (résolu dans resolve_choice, même rng d'évènements — déterministe) et pousse
     * une ligne provlog (« Le pari a tourné », scope province). Défauts zéro
     * (gamble_p=0) : les 15 évènements existants n'initialisent JAMAIS ces deux
     * champs (initialisation désignée) → gamble_p=0 ⇒ jamais de pari, table
     * existante intacte. */
    EvEffect    gamble_eff;
    float       gamble_p;   /* [0..1] : probabilité que le pari « tourne » (0 = pas de pari) */
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
    /* MEMBRANE DE DÉCISION — la crise phare (province agitée + bâtie + coercitive) et sa
     * suite CONSÉQUENTE (une cicatrice posée par le choix « Envoyer les prévôts » mûrit ici). */
    EVID_MARBRIVE,          /* le contremaître réclame — 3 choix imparfaits */
    EVID_PONT_EFFONDRE,     /* le sabotage (cicatrice SABOTAGE_CHANTIER) mûrit en catastrophe */
    /* ═══ CONTENU W1 — six évènements neufs (le PARI + le déclenchement « tech
     * just-latched ») : voir la table EVENTS[] pour les ancres de calibrage. ═══ */
    EVID_CLOCHES,           /* la surtaxe fait taire les cloches — province agitée, peu légitime */
    EVID_ENTREPOTS_FERMES,  /* le grief marchand ferme les entrepôts d'un carrefour saturé */
    EVID_DEUX_CARTES,       /* une conquête récente, mal intégrée, double son péage */
    EVID_EAU_NOIRE,         /* le puits vire noir — la Brèche s'invite dans le grenier */
    EVID_DERNIERE_DECISION, /* une cicatrice PENDANTE (non mûre) hante encore la province */
    EVID_SALVE_RUNIQUE,     /* la première salve runique (tech apex, déclenchement UNIQUE) */
    /* ═══ CONTENU W2 (lot 2) — §A tech (déclenchement unique) · §B culturel ·
     * §C religieux · §D chaînage de cicatrices. Voir la table EVENTS[] pour les
     * ancres de calibrage. ═══ */
    EVID_CHAINES_RAPPORTENT,  /* A1 : Économie servile déverrouillée (déclenchement unique) */
    EVID_OEUVRE_NOIRE,        /* A2 : L'Œuvre noire ne s'éteint pas la nuit (déclenchement unique) */
    EVID_SAVOIR_INTERDIT,     /* A3 : le Savoir interdit tient ses promesses (déclenchement unique) */
    EVID_CULTE_IMPERIAL,      /* A4 : le trône est devenu un autel (déclenchement unique) */
    EVID_EVEIL,               /* A5 : quelque chose s'est éveillé dans les glyphes (déclenchement unique) */
    EVID_FOREUSE_SAIGNE,      /* A6 : la foreuse mord dans quelque chose qui saigne (province) */
    EVID_DROIT_INTEGRATION,   /* B1 : le droit d'intégration divise ceux qu'il unit (pays) */
    EVID_DIASPORA_COMPTOIRS,  /* B4 : la diaspora tient les comptoirs (pays) */
    EVID_FOI_FENDRE,          /* C1 : la foi va se fendre (schisme éligible, pays) */
    EVID_PROPHETE_BRECHE,     /* C5 : la brèche a trouvé son prophète (pays) */
    EVID_RELIQUE_DOUTEUSE,    /* C6 : la relique fait des miracles douteux (province) */
    EVID_REMEDE_MORTS,        /* K2 : le remède fait des morts (SCAR_SCANDALE_SANITAIRE mûrit) */
    EVID_CELLULE_FAUBOURGS,   /* K3 : une cellule dans les faubourgs (SCAR_RADICALISATION mûrit) */
    EVID_FUSILS_REVIENNENT,   /* K5 : nos propres fusils nous reviennent (SCAR_PROLIFERATION_ARME mûrit) */
    EVID_SAVANTS_ENNEMI,      /* K6 : les savants sont passés à l'ennemi (SCAR_FUITE_CERVEAUX mûrit) */
    EVID_TARIF_APPRIS,        /* K7 : les autres villes ont appris le tarif (SCAR_EXEMPTION_ACHETEE mûrit) */
    /* ═══ V2b LOT 1 — LA MERVEILLE EN 3 ÉTAPES (pays, joueur seul — human gate) ═══ */
    EVID_MERV_FONDATION,      /* le monde reconnaît le palier 1 (metab_count≥3) : foi/science/force */
    EVID_MERV_SACRIFICE,      /* le chantier réclame — récurrent tant que merv est actif */
    EVID_MERV_ASCENSION,      /* le dernier choix, à MERV_SAVOIR_DONE : activer/refuser/détruire */
    /* ═══ V2b LOT 2 — LE CONSEIL (V2a : betrayal_ready/pair_state) ═══ */
    EVID_TRAHISON_SAVOIR,     /* le savant publie tes secrets (siège Savoir, betrayal_ready) */
    EVID_TRAHISON_SOCIETE,    /* le notable place ses familles (siège Société, betrayal_ready) */
    EVID_TRAHISON_INDUSTRIE,  /* le marchand détourne (siège Industrie, betrayal_ready) */
    /* ═══ ÂGES SANS ORDRE IMPOSÉ — L'ÂGE DES HÉROS (raccord 7) : « Le nom du siècle »,
     * un par siège (miroir EXACT du motif TRAHISON_* : le siège concerné est fixe par
     * EVID, résolu dynamiquement dans resolve_choice). ═══ */
    EVID_HERO_SAVOIR,         /* la mission décennale du siège Savoir consacre un Grand Esprit */
    EVID_HERO_SOCIETE,        /* … du siège Société consacre un Père/Mère de la nation */
    EVID_HERO_INDUSTRIE,      /* … du siège Industrie consacre un Grand(e) Capitaine */
    EVID_CONSEIL_SUCCESSION,  /* la retraite d'un loyal (>20 ans en poste) */
    EVID_CONSEIL_R1,          /* Savoir vs Société : trancher, ou renvoyer les deux */
    EVID_CONSEIL_R2,          /* Industrie vs Société : la route */
    EVID_CONSEIL_R3,          /* Savoir vs Industrie : le cadastre */
    EVID_CONSEIL_A1,          /* l'alliance de sièges : laisser/contrebalancer/séparer */
    EVID_CONSEIL_A2,          /* leur candidat au 3e siège */
    EVID_CONSEIL_C1,          /* la conspiration : renvoyer/sacrifier/céder */
    /* ═══ V2b LOT 3 — LE CONTENU DÉBLOQUÉ (lecteurs P7a) ═══ */
    EVID_RIVAUX_VOISINS,      /* B2 : deux cultures voisines ne s'accordent plus (culture_relation_of) */
    EVID_PARENTE_LOINTAINE,   /* B3 : une parenté culturelle lointaine se souvient (culture_relation_of) */
    EVID_MARCHE_ETHOS,        /* B6 : une région marche loin de l'éthos régnant (region_ethos_drift) */
    EVID_TOLERANCE_CREDO,     /* C2 : le décret de tolérance (religion_fracture_level) */
    EVID_LETTRE_PERIME,       /* C3 : le lettré porte une face périmée (religion_scholar_drift) */
    EVID_PRATIQUE_DERIVE,     /* C4 : la pratique dérive de la foi professée (religion_credo_drift) */
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
/* ÂGES SANS ORDRE IMPOSÉ (2026-07-11, docs/AGES_FINS_2026-07-11.md) — AUCUNE
 * chronologie fixe : chaque âge devient ÉLIGIBLE la première fois que son
 * déclencheur matériel est vrai (ACQUIS pour toujours, même si la condition
 * redescend), puis ADVIENT après un jitter déterministe (0..AGE_TRIGGER_JITTER_
 * YEARS ans, hash(seed,âge,année éligible)). Les deux paires causales de
 * l'ancien code (Lumières→{Soulèvements,Ordre de Fer}) sont DÉFAITES : seule la
 * paire mutuellement exclusive Soulèvements↔Tyrans subsiste (qui avient EMPÊCHE
 * l'autre). */
typedef enum {
    AGE_EXCHANGE = 0,   /* L'Ère des Échanges (ex-Commerce) */
    AGE_DISCOVERY,      /* L'Âge des Découvertes (ex-Raison) */
    AGE_EMPIRES,        /* L'Âge des Empires */
    AGE_HEROES,         /* L'Âge des Héros — déclenché par une mission décennale, pas un scan */
    AGE_BREACH,         /* L'Âge de la Brèche */
    AGE_LUMIERES,       /* L'Âge des Lumières */
    AGE_SOULEVEMENTS,   /* L'Âge des Soulèvements (exclut l'Ère des Tyrans) */
    AGE_TYRANS,         /* L'Ère des Tyrans (ex-Ordre de Fer ; exclut les Soulèvements) */
    AGE_COUNT
} AgeId;

typedef struct {
    bool    dawned[AGE_COUNT];
    /* JITTER SANS CHRONOLOGIE CACHÉE : année où le déclencheur est devenu vrai la
     * PREMIÈRE fois (-1 = pas encore éligible). ACQUIS — ne redescend jamais même
     * si la condition matérielle cesse d'être vraie. L'année d'AVÈNEMENT n'est
     * PAS stockée : dérivée à chaque tick de year_eligible + hash(seed,a,eligible)
     * % (AGE_TRIGGER_JITTER_YEARS+1) — recalcul pur, rien à faire dériver. */
    int16_t year_eligible[AGE_COUNT];
    float breach_pressure;           /* l'endgame faustien mondial (Brèche) */
    int   last_dawned;               /* -1 ou AgeId du dernier avènement */
    int   days_elapsed;              /* temps de jeu écoulé (cumul des ticks) */
    int   last_dawn_year;            /* an du dernier avènement RÉEL (throttle : au plus 1/an,
                                       * ce qui résout les collisions — « les autres suivent les
                                       * années suivantes » sans état de plus : le perdant d'une
                                       * collision reste candidat et gagne l'an prochain). */
} AgesState;

/* ===================================================================== */
/* LA MÉMOIRE DE DÉCISION — un choix laisse une trace qui MÛRIT (§ boucle)  */
/* ===================================================================== */
/* Anneau de CICATRICES : un choix pousse ici (subject, kind, délai tiré au
 * rng d'events → ripe_day) ; un trigger ultérieur la LIT (has_ripe) puis la
 * CONSOMME. Taille fixe, déterministe, sérialisable (fwrite BRUT du module). */
#define DECISION_SCAR_CAP 128
typedef struct {
    int16_t subject;    /* région ou pays visé (-1 = case vide) */
    int8_t  kind;       /* ScarKind */
    int32_t ripe_day;   /* jour où la cicatrice devient LISIBLE (mûrit) */
} DecisionScar;

/* Anneau de RÉPITS par (évènement, sujet) : anti-relance immédiate d'un même
 * évènement sur la même cible (indépendant du cooldown province/pays du F2,
 * qui porte sur les événements DIRIGÉS §F, pas la table EVENTS[]). */
#define DECISION_CD_CAP 96
typedef struct {
    int16_t subject;
    uint8_t evid;
    int32_t until_day;
} EvCooldown;

/* ===================================================================== */
/* LA FILE JOUEUR — un évènement provincial/pays qui CONCERNE le joueur       */
/* n'est PAS résolu par l'IA : il ATTEND son choix (membrane de décision).   */
/* ===================================================================== */
#define PENDING_EVENT_CAP 8
typedef struct {
    uint8_t evid;
    int16_t subject;
    int32_t fire_day;    /* jour où l'évènement a été enfilé */
    int32_t expire_day;  /* jour au-delà duquel l'auto-résolution (ai_chance) tranche */
} PendingEvent;

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

/* ===================================================================== */
/* LES ANNALES DU RÈGNE — un récit SÉLECTIF de la partie (§ Annales)       */
/* ===================================================================== */
/* La nature d'une entrée des Annales (lue par la façade pour composer une
 * phrase diégétique ; `a`/`b`/`region`/`option` sont des index BRUTS que la
 * façade résout en noms — la membrane elle-même ne porte que des faits). */
typedef enum {
    ANNAL_DILEMME = 0,      /* un évènement-décision RÉSOLU : a=evid, option=choix, region=sujet */
    ANNAL_CICATRICE,        /* une cicatrice a MÛRI en évènement : origin=index de l'ANNAL_DILEMME d'origine (-1 si perdu) */
    ANNAL_AGE,              /* un âge est ADVENU : a=AgeId */
    ANNAL_GUERRE_GAGNEE,    /* paix signée, score favorable : a=l'autre pays, b=score */
    ANNAL_GUERRE_PERDUE,    /* paix signée, score défavorable : a=l'autre pays, b=score */
    ANNAL_SECESSION,        /* un pays est NÉ d'une sécession : a=le nouveau pays */
    ANNAL_HEGEMON_BRISE,    /* un hégémon s'est effondré (réservé — non accroché en v1) */
    ANNAL_MONUMENT,         /* le PREMIER édifice du pays (960 j) : a=Edifice */
    ANNAL_FIN,              /* §27/Merveille : a=EndgameFin ou MERV_ASCENDED */
    /* V2b : les grands moments du Conseil et de la Merveille en 3 étapes. */
    ANNAL_TRAHISON,         /* un ministre a trahi (ou a été trahi) : a=evid, region=siège */
    ANNAL_MERVEILLE_ETAPE,  /* un palier de la Merveille est franchi/tranché : a=evid, option=choix */
    ANNAL_KIND_COUNT
} AnnalKind;

/* Anneau des ANNALES : SÉLECTION PAR POIDS (jamais un simple FIFO) — les faits
 * LOURDS et ANCIENS forment le panthéon, le récent tourne. subject/kind/evid
 * sont revalidés au chargement (save_sane) comme les autres anneaux du module.
 * `origin` = index (dans l'anneau, au moment du push) de l'entrée ANNAL_DILEMME
 * qui a posé la cicatrice devenue ANNAL_CICATRICE (-1 = introuvable/non pertinent). */
#define ANNALS_CAP 96
typedef struct {
    int16_t year;        /* l'an du fait (days_elapsed/365) */
    uint8_t kind;         /* AnnalKind */
    int16_t a, b;         /* charge utile (evid/pays/age/score…), selon kind */
    int16_t region;       /* région-sujet (-1 = aucune / pays) */
    uint8_t weight;       /* poids de sélection (survit si lourd+ancien) */
    int8_t  option;        /* ANNAL_DILEMME : le choix retenu (-1 sinon) */
    int16_t origin;        /* ANNAL_CICATRICE : entrée d'origine dans l'anneau (-1 = aucune) */
} AnnalEntry;

typedef struct {
    RegionGeo   geo[SCPS_MAX_REG];   /* la géo RELUE par région (le worldgen exposé) */
    AgesState   ages;
    Director    director;            /* §F : le directeur d'événements (dirigés) */
    uint32_t    rng;
    const char *last_name;           /* dernier évènement déclenché (UI/journal) */
    int         last_id;
    int         n_fired;             /* total déclenché (debug) */
    /* MEMBRANE DE DÉCISION (§ boucle) */
    DecisionScar scars[DECISION_SCAR_CAP];
    int          scar_head;                        /* prochaine case d'écriture de l'anneau */
    EvCooldown   cds[DECISION_CD_CAP];
    int          cd_head;
    PendingEvent pending[PENDING_EVENT_CAP];
    int          pending_n;
    /* LES ANNALES DU RÈGNE (§ Annales) — n'accroche QUE le pays JOUEUR (human_player≥0) */
    AnnalEntry   annals[ANNALS_CAP];
    int          annal_head;      /* prochaine case d'écriture (round-robin avant remplissage) */
    int          annal_n;         /* nombre d'entrées valides (<=ANNALS_CAP) */
    /* PLAFOND DE TIRS À VIE (demande joueur : « 3-5 triggers PAR évènement ») :
     * chaque dilemme n'arrive que 3-5 fois dans TOUTE LA PARTIE, monde entier.
     * fire_cap[e] est tiré à events_init (3-5, hash déterministe graine×evid ;
     * 0 = illimité — chocs géo/directeur et chaînage §D, borné par ses cicatrices
     * = par les CHOIX). fires[e] compte les tirs (saturé 255), incrémenté à
     * l'ENFILAGE joueur comme à la résolution IA (l'évènement « a eu lieu »). */
    uint8_t      fires[EVID_COUNT];
    uint8_t      fire_cap[EVID_COUNT];
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
 * l'Amnistie). `human_player` : -1 = chronique (aucun évènement n'est jamais
 * enfilé — golden intact) ; ≥0 = MEMBRANE DE DÉCISION : un évènement à VRAIE
 * décision (n_options>1) qui concerne ce pays est ENFILÉ (pending_event_*) au
 * lieu d'être auto-résolu par l'IA — le joueur choisit, ou l'auto-résolution
 * (ai_chance) tranche à expiration (180 j, pending_event_tick_expire, appelée
 * ici même en fin de tick). `eg` (V2b LOT 1, peut être NULL — les bancs/anciens
 * appelants qui n'ont pas d'endgame gardent leur comportement : les trois
 * évènements Merveille ne tirent alors jamais, le reste du tick est inchangé)
 * porte l'état de la Merveille (merv/merv_country/metab_count) que les triggers
 * V2b LOT 1 relisent, et que EVID_MERV_FONDATION/EVID_MERV_ASCENSION MUTENT via
 * endgame_start_wonder (déjà idempotent/public — pas un nouvel état à créer). */
void world_events_tick(EventsState *ev, World *w, WorldEconomy *econ,
                       WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                       RouteNetwork *rn, const TechState ts[], DiploState *dp,
                       EndgameState *eg, int days, int human_player);

/* ---- LE DIRECTEUR (§F) : lecteurs (UI / télémétrie) -------------------- */
float       director_temperature(const EventsState *ev);          /* dernière T [0..1] (0 ronron, 1 chaos) */
const char *director_event_name(int dir_id);                      /* nom diégétique d'un événement dirigé */
const char *events_name_of(int evid);                              /* nom d'un EvId de la table ("" hors borne) — le fil le porte par id */
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
/* TITRE PRÉSENTÉ (display-only) : le nom de table est un GABARIT — un « %s » y
 * coud le NOM RÉEL du sujet (région/pays) à la présentation. Rend buf. */
const char *event_title(const World *w, int evid, int subject, char *buf, int n);

/* ---- Âges : scan d'interprétation du monde ---------------------------- */
/* Évalue les triggers d'âge ; fait advenir tout âge nouvellement éligible
 * (palier + coordonnée globale). Renvoie true si un âge est advenu. */
bool  events_check_ages(EventsState *ev, World *w, WorldEconomy *econ,
                        WorldProsperity *wp, WorldLegitimacy *wl, const TechState ts[]);
bool  ages_dawned(const EventsState *ev, AgeId a);
/* raccord 3 — le palier est-il RÉELLEMENT ouvert (Société 3/Échanges · Savoir 4/
 * Découvertes · Société 5/Empires · Savoir 5/Brèche) ? Lu de wp->age_tech_mask
 * (posé par apply_effect, comme age_C_bonus — pas dans AgesState : ai_research_step
 * a déjà `wp` sous la main, jamais `ev`). */
bool  ages_tier_open(const WorldProsperity *wp, TechTheme br, int tier);
/* raccord 3, la porte RÉELLEMENT posée à l'éligibilité technologique : SEULS les
 * 4 paliers explicitement nommés par la spec (Société 3/Échanges · Savoir 4/
 * Découvertes · Société 5/Empires · Savoir 5/Brèche) sont soumis à ages_tier_open ;
 * tout le reste de l'arbre (Forge entière, tiers 0-2 partout, Savoir 3…) reste
 * INCHANGÉ (jamais gated — sinon tout l'arbre existant se verrouillerait au silence
 * d'un âge qui ne le mentionne pas). Appelée par scps_ai.c (IA), scps_sim.c (voie
 * joueur) et scps_api.c (readout) — un SEUL point de vérité. */
bool  ages_tech_researchable(const WorldProsperity *wp, TechTheme br, int tier);
float ages_breach_pressure(const EventsState *ev);
const char *age_name(AgeId a);
/* La citation de l'âge (membrane, champ flavor du readout d'âge — scps_age_state) ;
 * age<0 renvoie la citation de L'AUBE (l'état initial, jamais dans AgeId). */
const char *age_citation(int age /* AgeId ou -1 = l'Aube */);
/* raccord 6 — le ratio de pays connus [0..1] : Σ country_knows(a,b) ordonné sur les
 * paires de pays VIVANTS / Σ paires. Alimente le déclencheur des Découvertes (35 %)
 * ET un readout (scps_api.c). */
float ages_known_pair_share(const World *w);
/* raccord Découvertes — le bonus de radius de brouillard (0 tant que l'âge n'est
 * pas advenu, sinon AGE_DISCOVERY_FOG_RADIUS_ADD au registre J) : un seul point
 * de vérité pour les 4 sites d'appel de fog_update/fog_visible_regions. */
int ages_fog_radius_add(const EventsState *ev);
/* raccord 7 — L'ÂGE DES HÉROS : appelé par scps_sim.c juste après missions_tick,
 * pour CHAQUE (pays,siège) qui vient de compléter une mission décennale en
 * satisfaisant rang III + efficacité + loyauté + encore assis (le test lui-même
 * vit dans scps_sim.c, qui a accès à Statecraft/MissionsState). Fait advenir
 * AGE_HEROES la première fois, puis pousse « Le nom du siècle » (membrane de
 * décision pour le joueur, auto-résolu pour l'IA). */
void  ages_hero_fire(EventsState *ev, World *w, WorldEconomy *econ, WorldLegitimacy *wl,
                     WorldProsperity *wp, Statecraft *sc, RouteNetwork *rn,
                     const TechState ts[], DiploState *dp, EndgameState *eg,
                     MissionsState *ms, int cid, int seat, int slot, int gen,
                     int human_player);
/* Verdict du MOTEUR agrégé : combien de pays sont en mode révolutionnaire
 * (SI<5 & pression≥fracture) — la masse critique des Soulèvements, sans aucun
 * code de révolution dédié. */
int   events_count_revolutionary(const World *w, const WorldProsperity *wp);

/* MEMBRANE DE DÉCISION — télémétrie (« la télémétrie est la preuve d'équilibre ») : combien
 * de fois Marbrive/Pont Effondré ont tiré (RAZ à events_init, par partie/sim). */
long  events_marbrive_fired(void);
long  events_pont_effondre_fired(void);

/* LOT F (2026-07-08) — CATASTROPHES DU MONDE CALME : combien de chocs géo ont tiré
 * alors que le monde était jugé CALME (aucune fin en vue). RAZ à events_init. */
long  events_calm_shocks_fired(void);

/* CONTENU W1 — télémétrie des six évènements neufs (même motif : RAZ à events_init,
 * PAS sérialisés — le câblage chronicle viendra après, côté orchestrateur). */
long  events_cloches_fired(void);
long  events_entrepots_fermes_fired(void);
long  events_deux_cartes_fired(void);
long  events_eau_noire_fired(void);
long  events_derniere_decision_fired(void);
long  events_salve_runique_fired(void);

/* CONTENU W2 (lot 2) — même motif (RAZ à events_init, non sérialisés). */
long  events_chaines_rapportent_fired(void);
long  events_oeuvre_noire_fired(void);
long  events_savoir_interdit_fired(void);
long  events_culte_imperial_fired(void);
long  events_eveil_fired(void);
long  events_foreuse_saigne_fired(void);
long  events_droit_integration_fired(void);
long  events_diaspora_comptoirs_fired(void);
long  events_foi_fendre_fired(void);
long  events_prophete_breche_fired(void);
long  events_relique_douteuse_fired(void);
long  events_remede_morts_fired(void);
long  events_cellule_faubourgs_fired(void);
long  events_fusils_reviennent_fired(void);
long  events_savants_ennemi_fired(void);
long  events_tarif_appris_fired(void);

/* V2b — même motif (statics de module, RAZ à events_init, PAS sérialisés). */
long  events_merv_fondation_fired(void);
long  events_merv_sacrifice_fired(void);
long  events_merv_ascension_fired(void);
long  events_trahison_savoir_fired(void);
long  events_trahison_societe_fired(void);
long  events_trahison_industrie_fired(void);
long  events_conseil_succession_fired(void);
long  events_conseil_r1_fired(void);
long  events_conseil_r2_fired(void);
long  events_conseil_r3_fired(void);
long  events_conseil_a1_fired(void);
long  events_conseil_a2_fired(void);
long  events_conseil_c1_fired(void);
long  events_rivaux_voisins_fired(void);
long  events_parente_lointaine_fired(void);
long  events_marche_ethos_fired(void);
long  events_tolerance_credo_fired(void);
long  events_lettre_perime_fired(void);
long  events_pratique_derive_fired(void);

/* PLAFOND DE TIRS À VIE — lecteurs (UI/bancs) : le plafond MONDIAL d'un évènement
 * (0 = illimité ; sinon 3-5, tiré de la graine à events_init) et le compteur
 * courant (tous deux état sérialisé EventsState). */
int   events_fire_cap(const EventsState *ev, int evid);
int   events_fire_count(const EventsState *ev, int evid);

/* ---- Garde-fou membrane : aucun nom SCPS dans les textes joueur -------- */
bool  events_text_clean(void);

/* ===================================================================== */
/* MÉMOIRE DE DÉCISION — API (cicatrices posées par un choix, lues par un   */
/* trigger ultérieur : « ce choix aura des conséquences »)                 */
/* ===================================================================== */
/* Pousse une cicatrice `kind` sur `subject`, mûrissant dans [delai_min_j,
 * delai_max_j] jours (tiré au rng D'ÉVÈNEMENTS — déterministe). Écrase la
 * plus ancienne case de l'anneau si plein (bornes fixes, jamais de heap). */
void decision_memory_push(EventsState *ev, int subject, ScarKind kind,
                          int delai_min_j, int delai_max_j);
/* true si une cicatrice `kind` sur `subject` a MÛRI (ripe_day <= today) ET
 * n'a pas déjà été consommée. Ne CONSOMME PAS (le trigger n'a que la lecture). */
bool decision_memory_has_ripe(const EventsState *ev, int subject, ScarKind kind, int today);
/* CONSOMME (efface) la première cicatrice mûre `kind` sur `subject` trouvée
 * (à appeler au tir de l'évènement qui la consulte — une cicatrice = un tir). */
void decision_memory_consume(EventsState *ev, int subject, ScarKind kind, int today);

/* CONTENU W1 — EVID_DERNIERE_DECISION : une cicatrice PENDANTE (posée mais qui n'a
 * PAS ENCORE mûri, ripe_day > today) hante encore la province. Toute nature de
 * cicatrice compte (kind quelconque), au contraire de has_ripe (une kind précise,
 * déjà mûre). true si au moins une case `subject` a un ripe_day futur. */
bool decision_memory_pending(const EventsState *ev, int subject, int today);
/* Choix « Corriger publiquement » : EFFACE la cicatrice PENDANTE la plus proche
 * (ripe_day minimal encore futur) sur `subject` — la dernière décision n'arrivera
 * jamais. Ne fait rien si aucune cicatrice pendante. */
void decision_memory_cancel_next(EventsState *ev, int subject, int today);
/* Choix « Achever la décision par la force » : la cicatrice PENDANTE la plus proche
 * MÛRIT PLUS VITE (ripe_day rapproché du présent, mais jamais avant `today`+1 —
 * elle reste future, juste imminente). Ne fait rien si aucune cicatrice pendante. */
void decision_memory_hasten(EventsState *ev, int subject, int today);

/* Répit : after ce push, l'évènement `evid` ne peut retirer sur `subject`
 * avant `until_day`. has_cooldown = encore sous répit ? */
void decision_cooldown_push(EventsState *ev, int evid, int subject, int until_day);
bool decision_cooldown_active(const EventsState *ev, int evid, int subject, int today);

/* ===================================================================== */
/* LA FILE JOUEUR — API                                                    */
/* ===================================================================== */
/* Enfile un évènement en ATTENTE du choix du joueur (fire_day=today,
 * expire_day=today+180). false si la file est pleine (l'évènement est alors
 * silencieusement PERDU — comme un ordre qui déborderait le journal CMD ;
 * un monde qui empile >8 décisions en attente au joueur a un problème plus
 * grave que cette perte). */
bool pending_event_push(EventsState *ev, int evid, int subject, int today);
/* Nombre d'évènements actuellement EN ATTENTE. */
int  pending_event_count(const EventsState *ev);
/* Lit le pending au slot `slot` (0..pending_event_count). false si hors-borne. */
bool pending_event_at(const EventsState *ev, int slot, PendingEvent *out);
/* RÉSOUT le pending au slot `slot` avec l'option `option` — même chemin que
 * fire_event (apply_effect + hooks + cicatrice + cooldown + journal), puis
 * RETIRE le pending de la file (swap avec le dernier). false si slot/option
 * invalide (silencieux — la façade a REVALIDÉ avant d'enfiler CMD_EVENT_CHOICE,
 * mais l'état a pu changer entre-temps ; miroir save_sane : jamais déréférencé).
 * `human_player` : le sujet du pending appartient TOUJOURS au joueur à ce stade
 * (revalidé par l'appelant, CMD_EVENT_CHOICE) — passé à resolve_choice pour que
 * LES ANNALES accrochent CE choix (sans ça, la résolution d'un pending du joueur
 * paraîtrait venir de la chronique — human=-1 — et n'accrocherait jamais rien). */
bool pending_event_resolve(EventsState *ev, World *w, WorldEconomy *econ,
                           WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                           RouteNetwork *rn, const TechState ts[], DiploState *dp,
                           EndgameState *eg,
                           int slot, int option, int today, int human_player);
/* Auto-résolution (ai_chance) de tout pending EXPIRÉ (expire_day <= today) —
 * appelée par world_events_tick chaque jour (le joueur qui ignore une
 * décision finit par la subir, comme l'IA l'aurait tranchée). */
void pending_event_tick_expire(EventsState *ev, World *w, WorldEconomy *econ,
                               WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                               RouteNetwork *rn, const TechState ts[], DiploState *dp,
                               EndgameState *eg,
                               int today);

/* ===================================================================== */
/* save_sane — REVALIDATION des trois anneaux désérialisés (subject/kind/evid */
/* indexent SCPS_MAX_REG/SCPS_MAX_COUNTRY/ScarKind/EvId : un save forgé qui   */
/* les déborderait est REFUSÉ net, jamais déréférencé).                      */
/* ===================================================================== */
bool decision_memory_save_sane(const EventsState *ev, int max_subject);

/* ===================================================================== */
/* LES ANNALES DU RÈGNE — API (§ Annales)                                 */
/* ===================================================================== */
/* Pousse une entrée dans l'anneau des Annales. SÉLECTION PAR POIDS : tant que
 * l'anneau n'est pas plein, écriture round-robin normale (annal_head) ; une
 * fois plein, on ÉVINCE l'entrée de poids MINIMAL parmi la MOITIÉ la plus
 * ANCIENNE de l'anneau (balayage d'index croissant, déterministe — départage
 * par le premier trouvé) : les faits lourds & anciens forment le panthéon, la
 * moitié récente tourne librement. `weight` mesure l'importance dramatique du
 * fait (dilemme ∝ amplitude, âge/fin = poids plein). */
/* Renvoie l'index où l'entrée a été écrite dans l'anneau (-1 = rien écrit — le
 * fait était trop léger face au panthéon). Utile pour poser `origin` d'une
 * entrée liée juste après coup (ANNAL_CICATRICE → l'ANNAL_DILEMME qui l'a posée). */
int  annal_push(EventsState *ev, int year, int kind, int a, int b, int region,
                int weight, int option);
/* Lecture brute (la façade compose le texte) : nombre d'entrées valides. */
int  annals_count(const EventsState *ev);
/* i=0..annals_count()-1, dans l'ORDRE DE L'ANNEAU (pas trié) ; NULL/false hors-borne. */
bool annals_at(const EventsState *ev, int i, AnnalEntry *out);
/* REVALIDATION (save_sane) : year/kind/region/option bornés, jamais déréférencés. */
bool annals_save_sane(const EventsState *ev, int max_subject);

#endif /* SCPS_EVENTS_H */
