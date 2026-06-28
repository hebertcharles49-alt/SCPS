/*
 * scps_econ.h — couche ÉCONOMIE & CLASSES SOCIALES (moteur, sans UI)
 *
 * Tout est intriqué autour de la POPULATION, répartie en trois strates :
 *
 *   LABORERS   — la masse. Fournit le travail (RGO + manufactures), demande
 *                des biens de base (vivres, étoffe, bois de feu).
 *   BOURGEOIS  — marchands/artisans. Possèdent et gèrent les manufactures,
 *                captent le profit, demandent des biens manufacturés.
 *   ELITES     — aristocratie. Vivent de la taxe et de la rente, produisent
 *                la TECH (recherche) et exigent des biens de luxe (joaillerie,
 *                étoffe précieuse, eau-de-vie).
 *
 * Unité de simulation : la RÉGION (= le « marché régional » du cahier des
 * charges). Chaque région a :
 *   - une capacité d'extraction de matières premières (héritée de la
 *     géographie des provinces qui la composent) ;
 *   - des manufactures (laine → tissu, bois → papier, or → joaillerie…) ;
 *   - un entrepôt (stock par bien) et un PRIX par bien déterminé par la
 *     demande face au stock+offre.
 *
 * Boucle (econ_tick) :
 *   1. Extraction des matières premières (emploie des laborers).
 *   2. Manufacture : consomme intrants → produit biens finis (laborers +
 *      encadrement bourgeois).
 *   3. Revenus : salaires → laborers ; profit → bourgeois ; taxe → elites.
 *   4. Demande : besoin par tête × population, par strate.
 *   5. Marché : prix = base × demande/(stock+offre) ; allocation au budget ;
 *      satisfaction par strate.
 *   6. Mise à jour : stock reporté, satisfaction → croissance de pop, elites
 *      convertissent richesse+satisfaction en tech.
 */
#ifndef SCPS_ECON_H
#define SCPS_ECON_H

#include <stdio.h>          /* FILE* : helpers de save (prod_cap) */
#include "scps_types.h"
#include "scps_culture.h"   /* PopCulture embarque les traits dérivés (Ethos, …) */
#include "scps_heritage.h"   /* couche biologique : heritage + traits (leviers) */
#include "scps_tech.h"      /* TechState : §B1 abonde prod_mult par les techs de production */
#include "scps_tune.h"      /* tune_f : lu par les modificateurs provinciaux (inline ci-dessous) */

/* §C — L'INTERRUPTEUR de l'inflation monétaire. 1 = active ; 0 = RETIRE tout effet
 * (l'IPM reste à 1.0, le multiplieur de prix est l'identité — la variable est gardée
 * facile à virer, l'idée étant peut-être mauvaise). Un seul define à basculer. */
#ifndef SCPS_IPM
#define SCPS_IPM 1
#endif

/* ---- Strates sociales ------------------------------------------------- */
typedef enum {
    CLASS_LABORER = 0,
    CLASS_BOURGEOIS,
    CLASS_ELITE,
    CLASS_COUNT
} SocialClass;

typedef struct {
    float pop;            /* effectif */
    float wealth;         /* trésor accumulé (monnaie) */
    float satisfaction;   /* [0..1] : fraction des besoins couverts au dernier tick */
} PopStratum;

/* ---- Manufactures (transforment matières premières → biens finis) ------ */
typedef enum {
    BLD_TEXTILE = 0,   /* laine   → tissu               */
    BLD_SAWMILL,       /* bois    → fournitures navales */
    BLD_PAPERMILL,     /* bois    → papier              */
    BLD_DISTILLERY,        /* sucre   → eau-de-vie                 */
    BLD_BREWERY,       /* grain   → bière (palier moral des Clans/Souterrain) */
    BLD_JEWELER,       /* or+métal précieux → joaillerie (bien d'élite) */
    BLD_WEAVER_LUX,    /* tissu   → étoffe précieuse    */
    BLD_MAGE_WORKSHOP, /* ARCANE : cristal → essence (mana) — sa combustion MONTE la Brèche */
    BLD_CELESTIAL_FORGE,/* ARCANE militaire : fer céleste + essence → armes enchantées */
    BLD_TOOLWORKS,     /* fer + bois → outils (DIRECT) → MULTIPLICATEUR de productivité */
    BLD_ARMORY,        /* fer → armes (armurerie) → puissance militaire de base */
    BLD_POWDERMILL,    /* salpêtre + charbon → poudre (poudrière) → puissance militaire */
    BLD_APOTHECARY,    /* simples → remèdes (apothicaire) → santé/confort */
    BLD_TUNIC,         /* étoffe → TUNIQUE (1:1) — vêtement fini des journaliers (chaîne séparée du luxe) */
    BLD_CHARCOAL,      /* 2 bois → 1 charbon (charbonnière, tech de base) — libère la fonderie de la rareté du charbon minier */
    BLD_FOREUSE,       /* §B2 FAUSTIEN : essence → GROS rendement de FER (foreuse arcanique) — l'issue à la famine de matière, payée en charge (tech) */
    BLD_ALAMBIC,       /* F3 : salpêtre → flux + nécessaire d'alchimiste (gate TECH_ALCHIMIE) */
    /* === FAU2 — LES TRANSMUTEURS (le doigt dedans) : stabilisent un bien vital, chaque
     * spawn AJOUTE de la charge → la pente vers la Brèche. La Foreuse (fer) existe déjà. === */
    BLD_REPLICATEUR,   /* FAU2 : flux → BOIS (gate faustien TECH_TRANSMUTATION) */
    BLD_CORNE,         /* FAU2 : fer céleste → NOURRITURE (gate faustien TECH_FORGE_RUNES) */
    /* === F2 — FABRIQUES D'ARMES CONVENTIONNELLES (bâtiments séparés, régions spécialisées) === */
    BLD_ARMORY_HEAVY,  /* F2 : fer ×3 → armes LOURDES */
    BLD_BOWYER,        /* F2 : fer + bois → armes de TRAIT */
    BLD_ARQUEBUS,      /* F2 : fer + poudre (cuivre repli) → armes à FEU (gate TECH_POUDRIERE, F7) */
    /* === CONFORT du brut de bâti : la DEMANDE qui tire l'extraction d'argile/pierre (manuf normale) === */
    BLD_POTTERY,       /* poterie            : argile → poterie (confort journalier/bourgeois → bonheur) */
    BLD_SCULPTURE,     /* atelier de sculpture : pierre → statuaire (confort bourgeois/élite → bonheur) */
    BLD_TYPE_COUNT
} BuildingType;

typedef struct {
    BuildingType type;
    float        level;     /* capacité (échelle de production) */
    float        workers;   /* emploi effectif au dernier tick  */
} Building;

#define ECON_MAX_BLD BLD_TYPE_COUNT  /* une manufacture de CHAQUE type par région — calé sur
                                      * l'enum (était figé à 6 alors que les types ont grimpé à
                                      * 14 : les bâtiments communs saturaient les slots et
                                      * éeau-de-vieçaient les rares — joaillerie, fonderie, outillage —
                                      * d'où leur pénurie. Désormais auto-calé : plus de désync). */

/* E2 §11 — plafond de STOCK régional par ressource : base sans Entrepôt, +cap
 * par Entrepôt bâti. Lu par econ_tick (clamp), l'UI (jauge) et l'IA stockeuse. */
#define ECON_STOCK_CAP_BASE     200.f
#define ECON_STOCK_CAP_ENTREPOT 500.f

/* ---- Profil culturel de la population d'une région --------------------- *
 * Distinct de la géographie : ce sont les gens qui ont une culture, pas la
 * terre. Initialisé depuis le biome dominant à la génération (gen_population),
 * puis mutable via syncrétisme, dérive (world_tick) et migration. */
typedef struct {
    /* Les cinq axes [0..10] — seuls axes lus pour la distance culturelle. */
    float langue;      /* horloge phylogénétique */
    float valeurs;
    float subsistance; /* ancrée sur le biome dominant à l'init */
    float parente;
    float religion;
    /* Traits dérivés (résolus par culture_make, remutés par syncrétisme). */
    Ethos        ethos;
    Lifeway      lifeway;
    Structure    structure;
    Credo        credo;
    ReligionBranch rel_branch;
    MartialTrait martial;
    EconTrait    econ;
    int  age;       /* ticks d'existence (dérive) */
    bool settled;   /* false = région vierge, pas encore peuplée */
    /* Couche BIOLOGIQUE (superposée à la culture) : la heritage et ses leviers
     * (démographie, K, P, H, dérive, fracture…). Posée par worldgen_seed_peoples. */
    Heritage heritage;
} PopCulture;

/* ---- GROUPES de population (clé de voûte démographique) ---------------- *
 * Une province ne contient plus une fiche homogène mais des GROUPES
 * (heritage, culture, classe, effectif). D interne = distance ENTRE groupes ;
 * H agit SUR eux ; l'assimilation fait DÉRIVER la culture d'une minorité.
 * `culture` est la fiche EFFECTIVE (cache recalculé = origine + dérive) — ce que
 * lisent prosperity/legitimacy ; `origin` est le substrat FIXE. Rétro-compat :
 * une province à UN groupe reproduit exactement les nombres d'hier. */
#define SCPS_MAX_GROUPS 8
typedef struct {
    Heritage heritage;
    Sphere       origin_sphere;  /* FIXE : pour le gouffre */
    PopCulture   origin;         /* substrat FIXE */
    PopCulture   culture;        /* fiche EFFECTIVE (cache = origine + dérive) */
    SocialClass  klass;          /* (hérité) la classe « principale » du groupe */
    long         count;
    /* CLASSE ÉMERGENTE (§pop précise) : combien de ce groupe (heritage×culture×foi) sont
     * Journaliers / Bourgeois / Nobles — sort des EMPLOIS (capitale + ateliers), par
     * paquets de 100. Σ pop_by_class = count. Jamais posé : recalculé au tick. */
    long         pop_by_class[CLASS_COUNT];
    float        L;              /* légitimité du groupe envers la couronne */
    float        agit_base;      /* agitation VRAIE (la suppression la masque) */
    float        integration;    /* 0..1 → pilote l'assimilation */
    bool         diaspora;       /* installé loin de sa terre (migration) */
    int          drift_id;       /* clé dans la pile de dérive du monde */
} PopGroup;
typedef struct {
    PopGroup groups[SCPS_MAX_GROUPS];
    int      n_groups;           /* 0 = non attaché → repli sur RegionEconomy.culture */
    int      prov;               /* province du monde (géo) — optionnel */
    float    prosperity;         /* prospérité locale [0..10] (gradient de migration) */
} ProvincePop;

/* ---- Densité institutionnelle bâtie (couche d'agency) ----------------- *
 * Un bâtiment n'est pas un bonus : c'est de la densité institutionnelle
 * RÉALISÉE qui déplace une coordonnée que le moteur d'ordre LIT. Ces
 * accumulateurs sont remplis par scps_agency (à l'achèvement d'un édifice) et
 * relus par prosperity_tick (K/P/H) et legitimacy_tick (H ronge L). */
typedef struct {
    float K_inst;    /* institutionnel → monte K du pays */
    float H_coerc;   /* coercitif      → monte H, RONGE L local */
    float P_open;    /* ouverture      → monte P (perméabilité) */
    float PE_infra;  /* prospérité     → capte plus de PE local */
    float food_cap;  /* rendement/stockage alimentaire → croissance */
    float faith;     /* foi (temple/sanctuaire) → SOUTIENT L local (apaise l'agitation) */
    float savoir;    /* savoir bâti (bibliothèque/monastère) → ACCÉLÈRE la recherche locale */
    float port;      /* le PORT réel (chantier·rade·débouché — mer §5) ; ≠ P_open (le
                      * Caravansérail ouvre P sans donner de rade). Posé par EDI_PORT. */
} ProvBuild;

/* ---- Économie d'une région -------------------------------------------- */
typedef struct {
    PopStratum strata[CLASS_COUNT];
    PopCulture culture;   /* profil culturel DOMINANT (synchronisé sur le plus gros groupe) */
    ProvincePop pop;      /* les GROUPES de la province (clé de voûte) — n_groups=0 si non attaché */
    ProvBuild  build;     /* densité institutionnelle bâtie par le joueur */
    uint32_t   edi_built; /* E1bis.11 : masque des ÉDIFICES bâtis (par-édifice → upgrades familiales) */
    float      route_pe;  /* PE apporté par les routes commerciales (transitoire) */
    float      import_margin;     /* I6 : marge d'achat au marché (1.0 défaut) — ÉCRITE par intertrade, LUE par agency */
    int16_t    import_toll_region;/* I6 : région-Centre qui touche le péage (-1 si aucun) — transitoire */
    /* M3 (forks §8) — L'HYSTÉRÉSIS DU PÔLE : last_pole = le pôle VALIDÉ (celui que
     * les forks lisent) ; pole_since_day = depuis quand un pôle DIFFÉRENT s'observe
     * (0 = aligné). Un candidat doit tenir ≥ 360 j avant de devenir le pôle lu. */
    int8_t     last_pole;         /* TechPole validé (POLE_ORDRE par défaut) */
    int32_t    pole_since_day;    /* jour où le candidat divergent est apparu (0 = aucun) */

    float      raw_cap[RES_COUNT];   /* extraction max/tick par matière première */
    uint8_t    raw_boost[RES_COUNT]; /* EXPLOITATION : palier de boost d'extraction PAR brute (+RAW_BOOST_PER_TIER/palier) — bâti & amélioré comme une manufacture, scale sur les bras d'extraction */
    Building   bld[ECON_MAX_BLD];
    int        n_bld;

    /* ALLOCATION DE MAIN-D'ŒUVRE (joueur/IA) — override du split AUTO du tick.
     *  · alloc_on=0 (DÉFAUT) ⇒ comportement AUTO : extraction ∝ geo×prix sous
     *    EXTRACT_LABOR_SHARE, manufacture gloutonne par ordre de bâtiment.
     *  · alloc_on=1 ⇒ le bassin labor_avail (journaliers+bourgeois) est réparti par les
     *    POIDS ci-dessous — extraction ET manufacture dans UN SEUL budget (somme=100 % à l'UI).
     *    Un poids 0 sur un bâtiment = FERMÉ (aucune sortie, aucun intrant consommé). */
    uint8_t    alloc_on;                       /* 0 = auto (défaut, déterminisme préservé) ; 1 = override */
    uint8_t    alloc_raw[RES_PROD_FIRST];      /* poids d'allocation par brute extraite (0-255) */
    uint8_t    alloc_bld[BLD_TYPE_COUNT];      /* poids d'allocation par type de bâtiment (0 = fermé) */
    uint8_t    bld_input[BLD_TYPE_COUNT];      /* choix d'intrant : 0 = primaire (in1) ; 1 = repli (alt1) */

    float      stock [RES_COUNT];    /* entrepôt régional — PLAFONNÉ : 200/ressource + 500 par Entrepôt bâti (E2 §11) */
    float      price [RES_COUNT];    /* prix de marché courant */
    float      demand[RES_COUNT];    /* demande agrégée (dernier tick) */
    float      supply[RES_COUNT];    /* offre agrégée (dernier tick) */
    uint8_t    n_entrepot;           /* E2 §11 : Entrepôts BÂTIS ici (chacun +500 de cap de stock) */

    float      treasury;             /* taxe captée par les élites (cumul) */
    float      tech;                 /* recherche cumulée */
    float      tech_prod;            /* §B1 : multiplicateur de prod issu des techs de PRODUCTION du pays (1 = aucun) */
    bool       tech_foreuse;         /* §B2 : le pays a-t-il débloqué la foreuse arcanique ? (gate de BLD_FOREUSE) */
    bool       tech_alchimie;        /* F3 : le pays a-t-il débloqué l'alchimie ? (gate de BLD_ALAMBIC) */
    bool       tech_replicateur;     /* FAU4 : TECH_TRANSMUTATION débloquée ? (gate de BLD_REPLICATEUR) */
    bool       tech_corne;           /* FAU4 : TECH_FORGE_RUNES débloquée ? (gate de BLD_CORNE) */
    bool       tech_arquebus;        /* F7 : TECH_POUDRIERE débloquée ? (gate de BLD_ARQUEBUS) */
    float      gdp;                  /* valeur produite au dernier tick */
    float      satisfaction;         /* satisfaction générale [0..1] */
    float      food_sat;             /* satisfaction alimentaire [0..1] (grain+fish) */
    float      society_sat;          /* satisfaction sociale [0..1] (cloth+eau-de-vie+…) */
    float      needs_met;            /* [0..1] : fraction ABSOLUE du panier couverte (catégories ≥τ / total du panier), pop-pondérée — pilote la FERTILITÉ (×2 subsistance → ×4 plein) */
    float      over_tax;             /* surtaxe ressentie par les laboureurs [0..1] (grief → révolte) */
    float      cap_pop;              /* capacité d'accueil (pop cible à terme) */
    float      prosperity;           /* PIB/tête normalisé — cache pour migration */

    /* Diaspora & innovation culturelle */
    float      diaspora_pop;         /* immigrants non-primaires installés (bourgeois+élites) */
    float      diaspora_innovation;  /* score d'innovation cumulé (diminue par acculturation) */

    /* Coercition temporaire (relocalisation forcée) — décroît chaque tick */
    float      coercion;             /* [0..1] : 0=libre, 1=état d'urgence */
    /* Cicatrice de révolte [0..1] : une province récemment soulevée se développe
     * MAL (−50 % de croissance ET de production) ; décroît sur quelques années. */
    float      revolt_scar;
    /* FAVEURS provinciales À ÉTAT (modificateurs diégétiques, lot 2) — [0..1], décroissent
     * chaque tick. ferveur = l'élan d'une colonie fraîchement fondée (semée à la colonisation) ;
     * reconstruction = la renaissance d'après-choc (amorcée par une cicatrice PROFONDE, elle
     * OUTLASTE la plaie → la reprise suit la paix). Routées par l'entrée DÉMO (provmod_collect). */
    float      ferveur;
    float      reconstruction;
    /* CICATRICE D'ANNEXION [0..1] (pipeline diplo, étage 3d) : une province ANNEXÉE par voie
     * de vassalité porte une plaie DOUCE — frappe la STABILITÉ (satisfaction/légitimité), PAS
     * la croissance (distincte de revolt_scar) — qui décroît sur ~5 ans. L'intégration la
     * RABAISSE → la voie patiente paie. Surfacée dans le slot MODIFICATEURS (fléau décroissant). */
    float      annex_scar;
    /* Anti-saccage (§4 guerre) : une province DÉPOUILLÉE ne peut l'être à nouveau
     * avant ~5 ans (plus rien à prendre) — compteur en années, décroît chaque tick. */
    float      pillage_cd;

    /* ARCANE (§ fil arcane) : essence brûlée ce tick par les ateliers de mage.
     * prosperity_tick l'agrège dans le flux faustien → déréalisation/Brèche. */
    float      arcane_charge;       /* FAU0 : activité faustienne CE tick (reset/tick, += par faust_charge_add) */
    float      faust_charge;        /* FAU0/FAU1 : entropie CUMULÉE de la région (accumule l'activité, décrue passive) */
    float      faust_consumed[3];   /* FAU0 #3 : conso cumulée (0 essence/foreuse · 1 flux/réplicateur · 2 fer céleste/corne) — caché */
    float      mil_stock;           /* F6 : canal de FORCE D'ARMÉE (warhost → diplo_mil_power), DÉCOUPLÉ du RES_ARMS économique que la levée consomme */

    float      habitability;          /* habitabilité moyenne [0..1] — héritée des provinces */
    bool       active;               /* terre habitable (colonisable) */
    bool       impassable;           /* zone morte : infranchissable pour colonisation et commerce */
    bool       colonized;            /* effectivement peuplée/settlée */
    int16_t    owner;                /* pays qui contrôle la région (-1 = vierge) */
    bool       coastal;              /* une province au moins touche la mer (posé à econ_init) */
    bool       estuary;              /* une EMBOUCHURE vit ici (mer ∩ gros fleuve) — l'entrepôt naturel */
    bool       is_capital;           /* région-SIÈGE (porte la capitale d'un empire/cité) — EXEMPTE du malus d'habitabilité (la province de départ) */
    uint8_t    prov_geo;             /* dons GÉO sélectifs (drapeaux PROVF_*, posés à econ_init) : gibier/halieutique */
    /* LA COURSE (coques §4) : balafre côtière et immunité au raid. */
    float      balafre_days;         /* > 0 : côte balafrée (production entaillée ~1 an) */
    float      raid_cd_days;         /* > 0 : immunisée (~5 ans — on ne trait pas la même vache) */
} RegionEconomy;

/* Conteneur — possédé par l'appelant, séparé du World pour ne pas alourdir
 * son ABI. */
typedef struct {
    RegionEconomy region[SCPS_MAX_REG];
    int           n_regions;
    int           tick;

    /* §C — INFLATION MONÉTAIRE (indice des prix monétaires). Un seul interrupteur :
     * SCPS_IPM (cf. plus bas). Quand il vaut 0, `ipm` reste à 1.0 et le multiplieur
     * de prix devient l'identité — la fonctionnalité est INERTE, facile à retirer. */
    float         ipm;            /* indice des prix [~1.0] : trop d'or / trop peu de biens → >1 */
    float         ipm_ref;        /* ratio or/biens de RÉFÉRENCE (capté au 1er tick ; 0 = pas encore) */

    /* Adjacence de régions (terre, 4-connexe) — calculée à l'init, sert à
     * la colonisation (expansion vers une région vierge voisine). */
    uint8_t       adj[SCPS_MAX_REG][SCPS_MAX_REG];
} WorldEconomy;

/* ---- API -------------------------------------------------------------- */

/* Initialise pops, capacités d'extraction et manufactures à partir de la
 * géographie/ressources du monde déjà généré. */
void econ_init(WorldEconomy *e, const World *w);
/* (Re)construit l'adjacence de régions (terre 4-connexe, barrières = infranchissable).
 * Appelée par econ_init ; exposée pour le recalcul du capstone §27 (carve eau/ronces). */
void econ_build_adjacency(WorldEconomy *e, const World *w);
/* GAMEPLAY — garantit argile + pierre dans le rayon 1-2 de la capitale du joueur (force si absent).
 * À appeler après econ_init (adjacence) ET l'assignation des capitales (worldgen_seed_peoples). */
void econ_guarantee_player_construction(WorldEconomy *e, const World *w, int player_cid);
/* Capstone §27 FROID : re-dérive la fertilité vivrière (raw_cap[RES_GRAIN]) de
 * l'habitabilité COURANTE (carte refroidie) → la famine émerge sous le gel. */
void econ_cold_refresh(WorldEconomy *e, const World *w);

/* ---- MODIFICATEURS PROVINCIAUX (diégétiques) -------------------------- *
 * Un effet NOMMÉ, DÉRIVÉ de l'état réel de la région (remplissage, nourriture,
 * cicatrice) — PAS un champ stocké : recalculé à chaque lecture (le moteur ET la
 * membrane appellent la même fonction), donc AUCUN bump de save. L'effet passe par
 * une ENTRÉE moteur (la démographie de la croissance), jamais un bonus plat sur la
 * sortie ; le readout le traduit en mots. Les modificateurs À ÉTAT (ferveur fondatrice,
 * reconstruction) viendront avec un champ sérialisé (et son bump). */
/* Dons géo sélectifs (RegionEconomy.prov_geo), posés à econ_init (tirage déterministe). */
#define PROVF_GIBIER       0x01u   /* ~1/3 des régions boisées : gibier abondant */
#define PROVF_HALIEUTIQUE  0x02u   /* ~1/3 des régions côtières : manne halieutique */
typedef enum { PMOD_NONE=0, PMOD_CICATRICE, PMOD_ABONDANCE,
               PMOD_FERVEUR, PMOD_RECONSTRUCTION, PMOD_LIMON,
               PMOD_GIBIER, PMOD_HALIEUTIQUE, PMOD_ADMIN,
               PMOD_ANNEX_SCAR, PMOD_COUNT } ProvModKind;
typedef struct {
    uint8_t kind;        /* ProvModKind */
    float   intensity;   /* [0..1] — vivacité (pour la bande d'affichage) */
    float   demo_bonus;  /* delta ajouté à l'entrée DÉMOGRAPHIE de la croissance (0 = fléau, pur affichage) */
} ProvModHit;
/* eff_cap d'une région (Q6 : ½·cap_pop + logements bâtis plafonnés + grenier). INLINE :
 * partagée par le moteur (croissance) ET la membrane (readout) sans dépendance de LIEN
 * (les bancs readout/factions ne tirent pas scps_econ.o). Pure, sans libc-math. */
static inline float econ_region_effcap(const RegionEconomy *re){
    float house_manuf = tune_f("HOUSE_MANUF", 100.f);
    float manuf_h = 0.f;
    for (int bi = 0; bi < re->n_bld; bi++) manuf_h += re->bld[bi].level;
    float cap_half = re->cap_pop * 0.5f;
    float housed = manuf_h * house_manuf; if (housed > cap_half) housed = cap_half;
    /* BONUS CONFORT — poterie ET statuaire SERVIES (demande ACTIVE + couverte) : −15% de besoin de
     * LOGEMENT → le bâti loge davantage d'âmes (un foyer pourvu de confort tolère la densité). Appliqué
     * APRÈS le plafond ½·cap_pop (le confort permet de le DÉPASSER). Dérivé des flux stockés, pur. */
    float pot_d=re->demand[RES_POTTERY], pot_av=re->supply[RES_POTTERY]+re->stock[RES_POTTERY];
    float sta_d=re->demand[RES_STATUE],  sta_av=re->supply[RES_STATUE] +re->stock[RES_STATUE];
    if (pot_d>0.1f && pot_av>=pot_d*0.95f && sta_d>0.1f && sta_av>=sta_d*0.95f){
        float relief = tune_f("COMFORT_HOUSE_RELIEF", 0.15f);
        if (relief>0.f && relief<0.9f) housed *= 1.f/(1.f-relief);   /* −15 % de besoin ⇒ ×1/0.85 de capacité-logement */
    }
    return cap_half + housed + re->build.food_cap * 250.f;
}
/* Collecte les modificateurs provinciaux ACTIFS, DÉRIVÉS de l'état (aucun champ stocké). */
static inline int provmod_collect(const RegionEconomy *re, ProvModHit out[], int max){
    int n = 0;
    float scar = re->revolt_scar; if (scar < 0.f) scar = 0.f; else if (scar > 1.f) scar = 1.f;
    /* FLÉAU — la cicatrice (mécanique −50 % appliquée AILLEURS ; ici on la SURFACE, demo_bonus=0). */
    if (scar > 0.05f && n < max){ out[n].kind = PMOD_CICATRICE; out[n].intensity = scar; out[n].demo_bonus = 0.f; n++; }
    /* FLÉAU — CICATRICE D'ANNEXION (étage 3d) : plaie douce de STABILITÉ (appliquée à la
     * satisfaction AILLEURS), surfacée ici (demo_bonus=0 — n'entre pas dans la croissance). */
    { float as = re->annex_scar; if (as<0.f) as=0.f; else if (as>1.f) as=1.f;
      if (as > 0.05f && n < max){ out[n].kind = PMOD_ANNEX_SCAR; out[n].intensity = as; out[n].demo_bonus = 0.f; n++; } }
    /* FAVEUR — TERRE D'ABONDANCE : sous-peuplée + nourrie + en paix → +natalité (entrée DÉMO). */
    float eff = econ_region_effcap(re);
    if (eff > 1.f && n < max){
        float pop = 0.f; for (int c = 0; c < CLASS_COUNT; c++) pop += re->strata[c].pop;
        float ref   = tune_f("PROVMOD_ABOND_REF", 0.45f);
        float fill  = pop / eff;
        float under = (ref > fill) ? (ref - fill) : 0.f;
        float denom = (ref > 1e-3f) ? ref : 1e-3f;
        float inten = (under / denom) * re->food_sat * (1.f - scar);
        if (inten > 0.02f){
            if (inten > 1.f) inten = 1.f;
            out[n].kind = PMOD_ABONDANCE; out[n].intensity = inten;
            out[n].demo_bonus = tune_f("PROVMOD_ABOND_K", 2.0f) * under * re->food_sat * (1.f - scar);
            n++;
        }
    }
    /* FAVEUR — FERVEUR FONDATRICE : une colonie/jeune nation fraîchement fondée croît avec élan. */
    if (re->ferveur > 0.02f && n < max){
        float f = re->ferveur > 1.f ? 1.f : re->ferveur;
        out[n].kind = PMOD_FERVEUR; out[n].intensity = f;
        out[n].demo_bonus = tune_f("PROVMOD_FERVEUR_K", 0.5f) * f;
        n++;
    }
    /* FAVEUR — RECONSTRUCTION : la renaissance d'après-choc. Amorcée par une cicatrice profonde,
     * elle CULMINE à mesure que la plaie se referme (recon·(1−scar)) → la reprise SUIT la paix. */
    {
        float recon = re->reconstruction * (1.f - scar);
        if (recon > 0.02f && n < max){
            out[n].kind = PMOD_RECONSTRUCTION; out[n].intensity = recon > 1.f ? 1.f : recon;
            out[n].demo_bonus = tune_f("PROVMOD_RECON_K", 0.6f) * recon;
            n++;
        }
    }
    /* FAVEUR — LIMON FERTILE : une embouchure (delta, mer ∩ grand fleuve) nourrit une natalité dense. */
    if (re->estuary && n < max){
        out[n].kind = PMOD_LIMON; out[n].intensity = 1.f;
        out[n].demo_bonus = tune_f("PROVMOD_LIMON_K", 0.15f);
        n++;
    }
    /* FAVEUR — GIBIER ABONDANT : une terre boisée giboyeuse, bien nourrie, soutient plus de bouches. */
    if ((re->prov_geo & PROVF_GIBIER) && n < max){
        out[n].kind = PMOD_GIBIER; out[n].intensity = 1.f;
        out[n].demo_bonus = tune_f("PROVMOD_GIBIER_K", 0.10f);
        n++;
    }
    /* FAVEUR — MANNE HALIEUTIQUE : une côte poissonneuse nourrit une population dense. */
    if ((re->prov_geo & PROVF_HALIEUTIQUE) && n < max){
        out[n].kind = PMOD_HALIEUTIQUE; out[n].intensity = 1.f;
        out[n].demo_bonus = tune_f("PROVMOD_HALIEU_K", 0.10f);
        n++;
    }
    /* FAVEUR — BONNE ADMINISTRATION : des institutions solides (K bâti) tiennent l'ordre et les
     * services → les familles prospèrent. Le pendant DÉMO du levier K (« admin efficace → dévelop. »). */
    if (re->build.K_inst > 1.5f && n < max){
        float k = re->build.K_inst - 1.5f; if (k > 4.f) k = 4.f;   /* k ∈ [0,4] → k/4 ∈ [0,1] (clamp manuel, en-tête sans math.h) */
        out[n].kind = PMOD_ADMIN; out[n].intensity = k/4.f;
        out[n].demo_bonus = tune_f("PROVMOD_ADMIN_K", 0.06f) * k;
        n++;
    }
    return n;
}

/* Avance la simulation d'un pas. dt = années/tick (1 en annuel, 1/12 en mensuel) :
 * les processus cumulatifs (croissance, tech, impôt→trésor) suivent dt, les flux
 * production/consommation s'équilibrent par tick (satisfaction préservée). */
void econ_tick(WorldEconomy *e, float dt);
/* Q1 — LE CONSEIL : pose le multiplicateur d'un siège (0=Savoir 1=Société 2=Industrie)
 * pour un pays. Rafraîchi chaque tick par la couche sim depuis l'état conseil. 1.0=neutre. */
void econ_set_council_mult(int cid, int seat, float m);

/* §C — l'indice des prix monétaires mondial [~1.0] (1.0 = neutre / IPM désactivé).
 * Lecture seule, pour la télémétrie (chronique) et l'UI. */
float econ_world_ipm(const WorldEconomy *e);
/* Pool empire d'un bien : Σ stock des régions de même owner. Ce que le joueur POSSÈDE
 * (hors import) — la topbar/Stocks le lisent pour ne pas mentir. */
long econ_empire_stock(const WorldEconomy *e, int owner, Resource g);
/* or NET d'un pays = Σ trésor de ses régions (négatif = dette). Partagé chronicle/credit. */
double econ_country_gold(const WorldEconomy *e, int c);
/* E0.7 — RAZ de la mobilité de classe (panier capté + séries de mauvaise sat.) ;
 * appelé à chaque nouvelle partie/sim depuis econ_init. (RAZ aussi la friche E1bis.10.) */
void econ_mobility_reset(void);
/* E1bis.10 — régions EN FRICHE (entretien impayé) au dernier econ_tick (télémétrie). */
long econ_friche_count(void);
/* I0 — L'INSTRUMENT : décomposition du flux d'or par empire (le robinet, ligne à ligne).
 * Chaque puits/source incrémente sa composante ; le chronicle RAZ par fenêtre et publie
 * la moyenne par empire. Signe : revenus +, dépenses −. Lecture seule, diagnostic. */
typedef enum {
    FX_TAX=0, FX_EXPORT, FX_TOLL_RECV,                /* revenus (+) */
    FX_UPKEEP, FX_COURT, FX_ADMIN, FX_ENCADR,         /* dépenses (−) : édifices/cour/admin/manuf */
    FX_SOLDE, FX_NAVY, FX_AUDIT, FX_TOLL_PAID, FX_INVEST, FX_CONSEIL, FX_IMPORT,
    FX_COUNT
} FluxComp;
void   econ_flux_add(int cid, FluxComp comp, float amount);   /* incrémente (signé par convention ci-dessus) */
double econ_flux_get(int cid, FluxComp comp);                 /* cumul depuis le dernier reset */
void   econ_flux_reset(void);                                 /* RAZ (début de fenêtre de mesure) */
const char *econ_flux_name(FluxComp comp);                    /* libellé court (français, outillage) */
/* E3 §16 — le prix d'ANCRE d'un bien (BASE_PRICE) : pour normaliser les indices
 * de prix (télémétrie du lissage par les stocks). 0 si hors borne. */
float econ_base_price(Resource r);
/* Le FOND de matière qu'une région garde pour SA construction avant d'exporter (le gate de chantier
 * exige la matière présente ; sans réserve l'export auto vide les régions). 0 hors matériaux de bâti. */
float econ_build_reserve(Resource r);

/* §B1 — pousse le bonus de PRODUCTION (techs Forge/Société/Savoir·Production) du PAYS
 * propriétaire vers chaque région (re->tech_prod), lu par econ_tick pour abonder prod_mult.
 * À appeler par le harnais avant econ_tick (la recherche pays → la prod région). */
void econ_apply_country_tech(WorldEconomy *e, const TechState *ts, int n_ts);

/* Tolérance fiscale [0..1] par ÉTHOS × classe (§7) : le seuil (×satisfaction)
 * au-delà duquel on fuit l'impôt et l'on gronde. Exposé pour les bancs d'essai. */
float econ_tax_tolerance(Ethos e, SocialClass c);

/* §4 (catalogue des biens) — fraction de pop « mal servie » d'une province :
 * une minorité d'une autre SPHÈRE réclame ses variantes ; l'assimilation efface
 * la pénalité. 0 si la province est homogène. Frappe la satisfaction SOCIALE. */
float econ_off_culture_fraction(const ProvincePop *pp);

/* ── PIPELINE IA — PRÉVISION (étage 1) : ce que l'IA LIT pour voir le mur venir ──
 * Forecast par PAYS et par flux, DÉRIVÉ des seules coordonnées du moteur (pop, raw_cap,
 * demande, offre, stock, eff_cap, needs_met). Recalculé au tick, JAMAIS sérialisé.
 *   runway[g]        : années avant le déficit (demande projetée > offre+stock) ; +inf si jamais.
 *   shortfall_proj[g]: besoin(HORIZON) − offre actuelle (annualisé ; >0 = on sera court).
 *   struct_deficit[g]: la production MAX possible (au plein eff_cap) < la conso à plein → déficit
 *                      DURABLE (import/colonie/plafond), pas un creux passager.
 *   food_runway      : le runway AGRÉGÉ des sources vivrières (grain+poisson+viande) — l'existentiel. */
typedef struct {
    float runway[RES_COUNT];
    float shortfall_proj[RES_COUNT];
    unsigned char struct_deficit[RES_COUNT];
    float food_runway;
    float pop, eff_cap, growth_r;     /* trajectoire f(pop) (lecture/télémétrie) */
} EconForecast;
/* Calcule le forecast du pays `cid` à l'horizon `horizon` ans. Lecture pure (const econ). */
void econ_country_forecast(const WorldEconomy *e, int cid, float horizon, EconForecast *out);
/* Conso ANNUELLE par tête d'un bien (table NEED × parts de classe × tension × FOOD_NEED si food). */
float econ_conso_per_capita_year(Resource g);

/* Pas de colonisation : joueur et antagonistes essaiment vers les régions
 * vierges voisines ; les cités-états colonisent leurs propres territoires
 * non encore peuplés. À appeler après econ_tick(). Renvoie le nb de régions
 * nouvellement colonisées ce tick. */
int econ_colonize_tick(WorldEconomy *e, const World *w, int skip_cid);
/* L5 — colonie OUTRE-MER : mêmes portes (pop/vivres/cible vierge) mais coût pop ×2.
 * L'appelant (harnais) a vérifié Port + coque + portée de courants. */
bool econ_colonize_overseas(WorldEconomy *e, int src_rid, int dst_rid, int cid);
/* Fonde une colonie de `cid` sur `dst` depuis `src` (pop essaimée, owner posé).
 * Exposé pour la colonisation OUTRE-MER (scps_navy §8) — même acte fondateur. */
void econ_colonize_from(WorldEconomy *e, int src_rid, int dst_rid, int cid);

/* Migration interne basée sur la prospérité : les bourgeois et élites
 * migrent vers les régions plus riches adjacentes. Crée de la diaspora et
 * de l'innovation dans la destination. Renvoie le nb de flux migrateurs
 * actifs ce tick. */
int econ_migrate_tick(WorldEconomy *e, const World *w);

/* Relocalisation forcée : déplace `amount` habitants (surtout laborers)
 * de src_rid vers dst_rid. Provoque un pic de coercition dans la source.
 * Peut être appelée par le joueur ou par un événement scénarisé. */
void econ_relocate_pop(WorldEconomy *e, int src_rid, int dst_rid, float amount);

/* Affiche un tableau récapitulatif d'une région sur stdout. */
void econ_print_region(const WorldEconomy *e, const World *w, int region_id);

/* Affiche un sommaire monde (totaux pop/tech/PIB + top régions). */
void econ_print_summary(const WorldEconomy *e, const World *w);

/* Libellés */
const char *social_class_name(SocialClass c);
const char *building_name(BuildingType b);
/* Recette d'un bâtiment (intrants → extrant) — pour la perception IA. */
void        building_recipe(BuildingType b, Resource *in1, Resource *in2, Resource *out);
Resource    building_alt_input(BuildingType b);   /* intrant alternatif (repli) — UI d'allocation */
/* M6 (forks §14) — le DELTA DE FLUX d'une manufacture arcane : Forge Céleste +1.2 ·
 * Atelier du Mage +0.8 · Alambic −0.3 (le puits) · 0 sinon. Table de design, lue par
 * le banc ; le PUITS est branché dans econ_tick (l'essence purifiée neutralise la charge). */
float econ_bld_flux_delta(BuildingType b);
bool  bld_is_faustian(BuildingType b);   /* FAU0 #4 : les 3 transmuteurs (foreuse/réplicateur/corne) */
void  faust_charge_add(RegionEconomy *re, float amount);  /* FAU0 #2 : le hook de charge UNIQUE */
long  econ_arms_take(WorldEconomy *econ, int cid, Resource arm, long need);  /* F6 : conso d'armes macro (levée/renfort) */
void  econ_set_arms_pump(float (*pump)(WorldEconomy*, int, int, float, float));   /* F-arc : pompe marché (intertrade_market_pull, +prix) ; NULL = stock propre seul */
/* §11.4 — LIMITEUR DE PRODUCTION (cap joueur/ressource/pays). -1 = désactivé (∞). Au plafond,
 * la manufacture cesse de produire ce bien (intrants libérés). État statique → save/load dédiés. */
void  econ_set_prod_cap(int country, int good, float maxv);
float econ_prod_cap   (int country, int good);
void  econ_prodcap_save(FILE *f);
bool  econ_prodcap_load(FILE *f);
int   bld_min_tier(BuildingType b);                       /* F-arc : tier de capitale requis pour poser la manufacture */
bool  econ_build_manufacture(WorldEconomy *econ, int region, BuildingType b);  /* F-arc : bâti délibéré (tier+or vérifiés par l'appelant) */
/* M6 — la MATIÈRE gate la manufacture arcane : Forge ↔ fer céleste, Atelier ↔ cristal,
 * Alambic ↔ salpêtre (raw_cap de la région). true pour les manufactures ordinaires. */
bool  econ_bld_can_build(const WorldEconomy *e, int region, BuildingType b);

#endif /* SCPS_ECON_H */
