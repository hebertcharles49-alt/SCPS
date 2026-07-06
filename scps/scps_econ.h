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
    /* ESCLAVE — APPENDU en fin d'enum (les valeurs 0-2 restent stables) : la strate
     * SERVILE. Présente SANS appartenance (§II.6, H) — compte dans labor_avail (les
     * bras), REPRODUIT en interne (croissance régionale ordinaire, cf. la boucle
     * for-CLASS_COUNT de la démographie), mais N'ENTRE JAMAIS dans la mobilité de
     * classe (ni promotion ni démotion — on ne devient/cesse d'être esclave que par
     * capture/vente/affranchissement) ni dans la friction culturelle (econ_off_
     * culture_fraction exclut ses âmes : la pression d'intégration ne les touche
     * pas, c'est le prix du GARDER). Panier au plancher vital (grain seul). */
    CLASS_SLAVE,
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
/* MODE D'ARRIVÉE d'un groupe allophone (le RAPPORT AU POUVOIR détermine la
 * diffusion du savoir ET le comportement social — la « marge historique » entre
 * une diaspora et une pop soumise). Coeff de diffusion = combien ce groupe fait
 * MÉTABOLISER son héritage au maître (Song : le savoir cloisonné ne diffuse pas).
 *  NATIF   : de souche (hétérogénéité de naissance) — NE diffuse PAS (choix initial).
 *  MIGRANT : venu volontairement (migration/pacte) — diffuse PLEIN, s'assimile.
 *  SOUMIS  : nation native CONQUISE (sur son sol, absorbée) — diffuse PLEIN, irrédentiste.
 *  DÉPORTÉ : esclave arraché — diffuse FAIBLE (savoir fragmenté : forge, créole, danse).
 *  RÉFUGIÉ : a FUI la guerre (violence chez lui) vers une province voisine sûre — diffuse
 *            PLEIN (il apporte ses métiers : Huguenots, Arméniens), mais RESPIRE : quand son
 *            foyer s'apaise, une part RENTRE (Vichy, Espagne post-Franco), le reste se fixe
 *            (Huguenots devenus prussiens) → métabolisation PARTIELLE. AUCUNE migration
 *            n'est définitive : tout groupe DÉPLACÉ (migrant/réfugié) garde son `home_reg`. */
typedef enum { ARR_NATIF=0, ARR_MIGRANT, ARR_SOUMIS, ARR_DEPORTE, ARR_REFUGIE, ARR_COUNT } Arrival;
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
    bool         diaspora;       /* PAS de souche (migration/conquête/déportation) */
    uint8_t      arrival;        /* Arrival — mode d'arrivée (diffusion + comportement) */
    int          drift_id;       /* clé dans la pile de dérive du monde */
    int          home_reg;       /* RÉGION d'origine d'un groupe DÉPLACÉ (migrant/réfugié) — le
                                  * foyer vers lequel il RESPIRE (retour partiel quand il s'apaise) ;
                                  * -1 = de souche/soumis/déporté (aucun « ailleurs » où rentrer). */
    /* C9 (audit éco) : DUALITÉ VOULUE de jure ≠ de facto. `faith` = adhésion INSTITUTIONNELLE
     * (culte d'État, id de registre ; ce que religion_refresh_region compte ; écrit par
     * region_set_native_faith). L'axe CONTINU origin.religion + rel_branch/credo (écrit par
     * demography faith_convert_tick) = pratique CULTURELLE. Deux concepts distincts, PAS un
     * double-écrivain du même champ — ne pas fusionner. */
    int          faith;          /* FOI PORTÉE par le groupe (id dans g_religions[], -1 = athée). La
                                  * religion descend au NIVEAU DU GROUPE (comme la culture) : un migrant/
                                  * réfugié PORTE sa foi, une région peut mêler plusieurs cultes (diversité
                                  * intrinsèque), l'hérésie = un groupe de foi dissidente. religion_of_region
                                  * = le culte DOMINANT (cache dérivé). ⚠ -1 par DÉFAUT (≠ 0 = religion 0) :
                                  * tout site de création DOIT poser faith=-1 explicitement. */
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

/* ---- Économie d'une PROVINCE (charte PROVINCE_MODEL.md) ----------------
 * L'unité individualisée — l'économie BRUTE (au sens du modèle EU4) VIT ici :
 * pop, strates, culture locale, EXACTEMENT 2 ressources brutes, bâtiments,
 * production, stock, croissance, colonisé, propriétaire. La RÉGION (plus bas)
 * n'est plus qu'un AGRÉGAT calculé depuis ses provinces — elle ne produit rien
 * elle-même. Champ-à-champ identique à l'ex-RegionEconomy (même sémantique,
 * mêmes tunables) — seul le GRAIN change (province au lieu de région). */
typedef struct {
    PopStratum strata[CLASS_COUNT];
    PopCulture culture;   /* profil culturel DOMINANT (synchronisé sur le plus gros groupe) */
    ProvincePop pop;      /* les GROUPES de la province (clé de voûte) — n_groups=0 si non attaché */
    ProvBuild  build;     /* densité institutionnelle bâtie par le joueur */
    uint32_t   edi_built; /* E1bis.11 : masque des ÉDIFICES bâtis (par-édifice → upgrades familiales) */
    float      route_pe;  /* PE apporté par les routes commerciales (transitoire) */
    float      import_margin;     /* I6 : marge d'achat au marché (1.0 défaut) — ÉCRITE par intertrade, LUE par agency */
    int16_t    import_toll_region;/* I6 : province-Centre qui touche le péage (-1 si aucun) — transitoire */
    /* M3 (forks §8) — L'HYSTÉRÉSIS DU PÔLE : last_pole = le pôle VALIDÉ (celui que
     * les forks lisent) ; pole_since_day = depuis quand un pôle DIFFÉRENT s'observe
     * (0 = aligné). Un candidat doit tenir ≥ 360 j avant de devenir le pôle lu. */
    int8_t     last_pole;         /* TechPole validé (POLE_ORDRE par défaut) */
    int32_t    pole_since_day;    /* jour où le candidat divergent est apparu (0 = aucun) */

    float      raw_cap[RES_COUNT];   /* extraction max/tick par matière première (EXACTEMENT 2 brutes non-nulles après vocation) */
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

    float      stock [RES_COUNT];    /* entrepôt local — PLAFONNÉ : 200/ressource + 500 par Entrepôt bâti (E2 §11) */
    float      price [RES_COUNT];    /* prix de marché courant (province ISOLÉE) / miroir du prix RÉGIONAL projeté */
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
    float      faust_charge;        /* FAU0/FAU1 : entropie CUMULÉE de la province (accumule l'activité, décrue passive) */
    float      faust_consumed[3];   /* FAU0 #3 : conso cumulée (0 essence/foreuse · 1 flux/réplicateur · 2 fer céleste/corne) — caché */
    float      mil_stock;           /* F6 : canal de FORCE D'ARMÉE (warhost → diplo_mil_power), DÉCOUPLÉ du RES_ARMS économique que la levée consomme */

    float      habitability;          /* habitabilité [0..1] (propre à la province) */
    bool       active;               /* terre habitable (colonisable) */
    bool       impassable;           /* zone morte : infranchissable pour colonisation et commerce */
    bool       colonized;            /* effectivement peuplée/settlée */
    int16_t    owner;                /* pays qui contrôle la province (-1 = vierge) */
    int16_t    region;               /* RÉGION géographique qui la groupe (miroir de World.province[].region,
                                       * caché ici pour qu'econ_tick puisse agréger SANS World* — la signature
                                       * publique econ_tick(WorldEconomy*, dt) ne change pas). Posé à econ_init. */
    bool       coastal;              /* la province touche la mer (posé à econ_init) */
    bool       estuary;              /* une EMBOUCHURE vit ici (mer ∩ gros fleuve) — l'entrepôt naturel */
    bool       is_capital;           /* province-SIÈGE (porte la capitale d'un empire/cité) — EXEMPTE du malus d'habitabilité (la province de départ) */
    uint8_t    prov_geo;             /* dons GÉO sélectifs (drapeaux PROVF_*, posés à econ_init) : gibier/halieutique */
    /* LA COURSE (coques §4) : balafre côtière et immunité au raid. */
    float      balafre_days;         /* > 0 : côte balafrée (production entaillée ~1 an) */
    float      raid_cd_days;         /* > 0 : immunisée (~5 ans — on ne trait pas la même vache) */
} ProvinceEconomy;

/* ---- Agrégat d'une RÉGION (charte PROVINCE_MODEL.md) --------------------
 * La région ne produit RIEN elle-même : elle REFLÈTE ses provinces (prospérité,
 * satisfaction, marché, légitimité, agitation = sommes/moyennes pop-pondérées
 * des ProvinceEconomy membres, recalculées par econ_tick après la boucle
 * province). Elle reste le grain lu par guerre/diplo/commerce/endgame (règle 4
 * de la charte) et porte le MARCHÉ (prix/offre/demande) — le pool de ses
 * provinces. Champs identiques à l'ancien RegionEconomy pour ne rien casser
 * côté lecteurs (legitimacy/revolt/statecraft/diplo/prosperity/campaign). */
typedef struct {
    PopStratum strata[CLASS_COUNT];
    PopCulture culture;   /* profil culturel DOMINANT (synchronisé sur le plus gros groupe/province) */
    ProvincePop pop;      /* GROUPES agrégés (miroir de la province-capitale/dominante) */
    ProvBuild  build;     /* densité institutionnelle bâtie — somme des provinces membres */
    uint32_t   edi_built; /* union des masques d'édifices des provinces membres */
    float      route_pe;
    float      import_margin;
    int16_t    import_toll_region;
    int8_t     last_pole;
    int32_t    pole_since_day;

    float      raw_cap[RES_COUNT];   /* Σ provinces */
    uint8_t    raw_boost[RES_COUNT]; /* max provinces */
    Building   bld[ECON_MAX_BLD];    /* miroir informatif (agrégat simple, cf. econ_tick) */
    int        n_bld;

    uint8_t    alloc_on;
    uint8_t    alloc_raw[RES_PROD_FIRST];
    uint8_t    alloc_bld[BLD_TYPE_COUNT];
    uint8_t    bld_input[BLD_TYPE_COUNT];

    float      stock [RES_COUNT];    /* pool régional = Σ stocks provinces */
    float      price [RES_COUNT];    /* prix de MARCHÉ régional (soldé ici, lu par les provinces membres) */
    float      demand[RES_COUNT];    /* Σ demande des provinces */
    float      supply[RES_COUNT];    /* Σ offre des provinces */
    uint8_t    n_entrepot;

    float      treasury;             /* Σ trésor des provinces */
    float      tech;                 /* Σ tech des provinces */
    float      tech_prod;
    bool       tech_foreuse;
    bool       tech_alchimie;
    bool       tech_replicateur;
    bool       tech_corne;
    bool       tech_arquebus;
    float      gdp;                  /* Σ PIB des provinces */
    float      satisfaction;         /* pop-pondérée */
    float      food_sat;             /* pop-pondérée */
    float      society_sat;          /* pop-pondérée */
    float      needs_met;            /* pop-pondérée */
    float      over_tax;             /* pop-pondérée */
    float      cap_pop;              /* Σ provinces */
    float      prosperity;           /* PIB/tête régional */

    float      diaspora_pop;
    float      diaspora_innovation;

    float      coercion;             /* max provinces */
    float      revolt_scar;          /* pop-pondérée (max des cicatrices vives) */
    float      ferveur;
    float      reconstruction;
    float      annex_scar;
    float      pillage_cd;

    float      arcane_charge;        /* Σ provinces */
    float      faust_charge;         /* Σ provinces */
    float      faust_consumed[3];    /* Σ provinces */
    float      mil_stock;            /* Σ provinces */

    float      habitability;         /* pop-pondérée (repli : moyenne surface si pop nulle) */
    bool       active;               /* au moins une province active */
    bool       impassable;           /* TOUTES les provinces membres sont infranchissables */
    bool       colonized;            /* au moins une province colonisée */
    int16_t    owner;                /* pays de la province-CAPITALE (ou majoritaire à défaut) */
    bool       coastal;              /* au moins une province côtière */
    bool       estuary;              /* au moins une province-embouchure */
    bool       is_capital;           /* la région porte la province-capitale d'un empire/cité */
    uint8_t    prov_geo;             /* union des drapeaux PROVF_* des provinces membres */
    float      balafre_days;         /* max provinces */
    float      raid_cd_days;         /* max provinces */
} RegionEconomy;

/* Conteneur — possédé par l'appelant, séparé du World pour ne pas alourdir
 * son ABI. */
typedef struct {
    ProvinceEconomy prov[SCPS_MAX_PROV];   /* LA VÉRITÉ : l'économie vit ici (charte PROVINCE_MODEL.md) */
    int              n_prov;
    RegionEconomy   region[SCPS_MAX_REG]; /* AGRÉGAT (reflète prov[]) — lu tel quel par guerre/diplo/commerce/endgame */
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
    /* Adjacence de PROVINCES (terre, 4-connexe) — colonisation par-province (charte règle 5). */
    uint8_t       *prov_adj;   /* alloué dynamiquement [SCPS_MAX_PROV*SCPS_MAX_PROV] (1664² ≈ 2.7 Mo — trop gros pour la pile/BSS d'un tableau statique 2D) */
    /* Province REPRÉSENTATIVE de chaque région (capitale, sinon la plus peuplée, sinon la
     * première active) — cache posé par econ_build_adjacency (qui a le World*). Résout le
     * grain public historique « région » (econ_build_manufacture, …) vers la province où
     * l'économie VIT réellement (charte), sans changer ces signatures publiques. -1 = aucune. */
    int16_t       region_rep_prov[SCPS_MAX_REG];

    /* ── CHANTIER DE COLONISATION (voie JOUEUR — l'IA garde son tick ANNUEL instantané) :
     * une colonie MÛRIT (~1 an si frontalière, plus loin = plus long, borné 3 ans) ; UNE à
     * la fois par pays + cadence 1 ordre/an (cd_days). Le convoi est PAYÉ à l'ordre
     * (ponction pop immédiate) ; l'arrivée sème `seed_base × yield` — rendement dégressif
     * LOGARITHMIQUE en f(distance à la CAPITALE, en sauts de provinces). Tout à -1/0 =
     * inactif : la chronique n'ordonne jamais → aucun flux moteur ne bouge (golden). ── */
    struct ColonyWork {
        int16_t src, dst;        /* provinces (-1 = pas de chantier) */
        int16_t days_left;       /* jours avant la fondation */
        int16_t total_days;      /* durée totale (readout de progression) */
        int16_t cd_days;         /* cadence : jours avant le PROCHAIN ordre permis */
        float   seed_base;       /* colons embarqués (ponction déjà faite) */
        float   yield;           /* rendement à l'arrivée (0..1) */
    } colony[SCPS_MAX_COUNTRY];
} WorldEconomy;

/* ---- API -------------------------------------------------------------- */

/* ---- Friction culturelle (définitions PARTAGÉES, scps_econ.c) ----------- *
 * econ_content_dist : D∞ sur les 4 axes de CONTENU (langue exclue — horloge).
 * econ_content_dist_faith : + plancher de friction quand la BRANCHE de foi
 *   diffère (la FOI est ACTIVE — lu par legitimacy & demography).
 * econ_ruling_culture : culture régnante d'un pays = sa région-capitale. */
float econ_content_dist(const PopCulture *a, const PopCulture *b);
float econ_content_dist_faith(const PopCulture *a, const PopCulture *b);
const PopCulture *econ_ruling_culture(const World *w, const WorldEconomy *econ, int cid);
/* ESCLAVAGE — gate ACHETEUR au marché des Centres (miroir du gate de capture IA) :
 * TECH_ESCLAVAGE débloquée OU éthos conquérant (Dominateur/Honneur) de la couronne. */
bool econ_country_can_enslave(const World *w, const WorldEconomy *econ, const TechState *ts, int cid);

/* Initialise pops, capacités d'extraction et manufactures à partir de la
 * géographie/ressources du monde déjà généré. */
void econ_init(WorldEconomy *e, const World *w);
/* MODTOOLS — surcharge des VALEURS éco (prix/rendement) par fichier TSV name-keyed.
 * dump : écrit le point de départ éditable. load : applique les surcharges (auto à
 * econ_init si l'env SCPS_MODS pointe un fichier). Sans fichier ⇒ valeurs compilées. */
void econ_moddata_dump(FILE *f);
int  econ_moddata_load(const char *path);
/* (Re)construit l'adjacence de régions (terre 4-connexe, barrières = infranchissable).
 * Appelée par econ_init ; exposée pour le recalcul du capstone §27 (carve eau/ronces). */
void econ_build_adjacency(WorldEconomy *e, const World *w);
/* CHARGEMENT : rebâtit le SEUL prov_adj (pointeur tas, jamais sérialisable) SANS toucher
 * adj/region_rep_prov (états SÉRIALISÉS — les recalculer à l'état courant casserait la
 * continuation déterministe sauve-recharge ; le rep « plus peuplée » bouge avec la pop). */
void econ_rebuild_prov_adj(WorldEconomy *e, const World *w);
/* Province REPRÉSENTATIVE d'une région (capitale, sinon la plus peuplée, sinon la première
 * active — cache posé par econ_build_adjacency). RE-KEY PROVINCE (PROVINCE_MODEL.md) : tout
 * écrivain HORS TICK (agency/diplo/credit/revolt/…) qui touchait `region[r].<champ>` entre
 * deux econ_tick() voyait son écriture EFFACÉE au tick suivant (econ_aggregate_regions
 * reconstruit region[] EN ENTIER depuis prov[]) — router l'écriture ICI, sur la province
 * représentative, la fait SURVIVRE (l'agrégation la re-somme/re-reflète au tick d'après).
 * -1 si la région n'a aucune province active. */
int econ_region_rep_province(const WorldEconomy *e, int region);
/* RE-KEY — écritures PERSISTANTES au grain RÉGION (stock/trésor/pop) : routent sur
 * les PROVINCES (représentative d'abord, débit qui déborde sur les sœurs) ET
 * tiennent la VUE region[] du mois courant. Écrire region[] directement serait
 * EFFACÉ à la clôture (econ_aggregate_regions reconstruit la vue depuis prov[]).
 * Rendent le delta réellement appliqué (débit borné au dispo ; trésor : le résidu
 * passe en dette sur la représentative, philosophie credit_spend). */
float econ_region_stock_add(WorldEconomy *e, int region, int good, float delta);
float econ_region_treasury_add(WorldEconomy *e, int region, float delta);
float econ_region_pop_add(WorldEconomy *e, int region, int cls, float delta);
/* Reconstruit region[] EN ENTIER depuis prov[] (le CŒUR d'econ_tick, exposé nu — PURE
 * fonction de prov[], AUCUN effet de temps/dt). Exposée pour les BANCS : un fixture qui
 * pose l'économie directement sur prov[] (charte : la vérité vit là) doit pouvoir rafraîchir
 * l'agrégat region[] SANS faire tourner un tick entier (croissance/production/décroissance)
 * juste pour que les lecteurs qui lisent encore region[] (agency/diplo/revolt/…, grain public
 * historique) voient un état à jour. Le jeu réel n'en a jamais besoin (econ_tick l'appelle
 * déjà en clôture, chaque jour). */
void econ_aggregate_regions(WorldEconomy *e);
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
/* eff_cap d'une PROVINCE (MERGÉ : tier housing + manufacture housing + grenier).
 * INLINE, générique par macro : partagée par le moteur (croissance, par-province) ET la
 * membrane (readout région/pays) sans dépendance de LIEN. Pure, sans libc-math.
 *   eff_cap = ½·cap_pop (terre nue)
 *           + tier_housing (capital tier × 1000 — le bâtiment principal)
 *           + manuf_housing (Σ bld.level × HOUSE_MANUF — les manufactures AJOUTENT)
 *           + grenier (food_cap × 250)
 * Le tier est DÉRIVÉ de la pop (pas un champ stocké) — même table que capitale_max_tier. */
#define ECON_EFFCAP_BODY(re) \
    do { \
        float _tpop = 0.f; \
        for (int _ci = 0; _ci < CLASS_COUNT; _ci++) _tpop += (re)->strata[_ci].pop; \
        int _tier = (_tpop>=10000.f)?7:(_tpop>=8000.f)?6:(_tpop>=5000.f)?5: \
                    (_tpop>=4000.f)?4:(_tpop>=3000.f)?3:(_tpop>=2000.f)?2:1; \
        long _admin = (long)_tier * 100; \
        if (_admin > (long)_tpop) _admin = ((long)_tpop/100)*100; \
        long _packs = _admin / 100; \
        float tier_h = (float)((_packs < _tier ? _packs : _tier) * 1000); \
        float house_manuf = tune_f("HOUSE_MANUF", 100.f); \
        float manuf_h = 0.f; \
        for (int bi = 0; bi < (re)->n_bld; bi++) manuf_h += (re)->bld[bi].level; \
        float cap_half = (re)->cap_pop * 0.5f; \
        float housed = manuf_h * house_manuf; if (housed > cap_half) housed = cap_half; \
        float pot_d=(re)->demand[RES_POTTERY], pot_av=(re)->supply[RES_POTTERY]+(re)->stock[RES_POTTERY]; \
        float sta_d=(re)->demand[RES_STATUE],  sta_av=(re)->supply[RES_STATUE] +(re)->stock[RES_STATUE]; \
        if (pot_d>0.1f && pot_av>=pot_d*0.95f && sta_d>0.1f && sta_av>=sta_d*0.95f){ \
            float relief = tune_f("COMFORT_HOUSE_RELIEF", 0.15f); \
            if (relief>0.f && relief<0.9f) housed *= 1.f/(1.f-relief); \
        } \
        return cap_half + tier_h + housed + (re)->build.food_cap * 250.f; \
    } while (0)
static inline float econ_prov_effcap(const ProvinceEconomy *re){ ECON_EFFCAP_BODY(re); }
static inline float econ_region_effcap(const RegionEconomy *re){ ECON_EFFCAP_BODY(re); }
#undef ECON_EFFCAP_BODY

/* Collecte les modificateurs provinciaux ACTIFS, DÉRIVÉS de l'état (aucun champ stocké).
 * Même motif générique : ECON_PROVMOD_BODY instancié pour ProvinceEconomy (moteur) et
 * RegionEconomy (membrane/readout agrégé). `effcap_fn` = econ_prov_effcap|econ_region_effcap. */
#define ECON_PROVMOD_BODY(re, out, max, effcap_fn) \
    do { \
        int n = 0; \
        float scar = (re)->revolt_scar; if (scar < 0.f) scar = 0.f; else if (scar > 1.f) scar = 1.f; \
        if (scar > 0.05f && n < (max)){ (out)[n].kind = PMOD_CICATRICE; (out)[n].intensity = scar; (out)[n].demo_bonus = 0.f; n++; } \
        { float as = (re)->annex_scar; if (as<0.f) as=0.f; else if (as>1.f) as=1.f; \
          if (as > 0.05f && n < (max)){ (out)[n].kind = PMOD_ANNEX_SCAR; (out)[n].intensity = as; (out)[n].demo_bonus = 0.f; n++; } } \
        float eff = effcap_fn(re); \
        if (eff > 1.f && n < (max)){ \
            float pop = 0.f; for (int c = 0; c < CLASS_COUNT; c++) pop += (re)->strata[c].pop; \
            float ref   = tune_f("PROVMOD_ABOND_REF", 0.45f); \
            float fill  = pop / eff; \
            float under = (ref > fill) ? (ref - fill) : 0.f; \
            float denom = (ref > 1e-3f) ? ref : 1e-3f; \
            float inten = (under / denom) * (re)->food_sat * (1.f - scar); \
            if (inten > 0.02f){ \
                if (inten > 1.f) inten = 1.f; \
                (out)[n].kind = PMOD_ABONDANCE; (out)[n].intensity = inten; \
                (out)[n].demo_bonus = tune_f("PROVMOD_ABOND_K", 2.0f) * under * (re)->food_sat * (1.f - scar); \
                n++; \
            } \
        } \
        if ((re)->ferveur > 0.02f && n < (max)){ \
            float f = (re)->ferveur > 1.f ? 1.f : (re)->ferveur; \
            (out)[n].kind = PMOD_FERVEUR; (out)[n].intensity = f; \
            (out)[n].demo_bonus = tune_f("PROVMOD_FERVEUR_K", 0.5f) * f; \
            n++; \
        } \
        { \
            float recon = (re)->reconstruction * (1.f - scar); \
            if (recon > 0.02f && n < (max)){ \
                (out)[n].kind = PMOD_RECONSTRUCTION; (out)[n].intensity = recon > 1.f ? 1.f : recon; \
                (out)[n].demo_bonus = tune_f("PROVMOD_RECON_K", 0.6f) * recon; \
                n++; \
            } \
        } \
        if ((re)->estuary && n < (max)){ \
            (out)[n].kind = PMOD_LIMON; (out)[n].intensity = 1.f; \
            (out)[n].demo_bonus = tune_f("PROVMOD_LIMON_K", 0.15f); \
            n++; \
        } \
        if (((re)->prov_geo & PROVF_GIBIER) && n < (max)){ \
            (out)[n].kind = PMOD_GIBIER; (out)[n].intensity = 1.f; \
            (out)[n].demo_bonus = tune_f("PROVMOD_GIBIER_K", 0.10f); \
            n++; \
        } \
        if (((re)->prov_geo & PROVF_HALIEUTIQUE) && n < (max)){ \
            (out)[n].kind = PMOD_HALIEUTIQUE; (out)[n].intensity = 1.f; \
            (out)[n].demo_bonus = tune_f("PROVMOD_HALIEU_K", 0.10f); \
            n++; \
        } \
        if ((re)->build.K_inst > 1.5f && n < (max)){ \
            float k = (re)->build.K_inst - 1.5f; if (k > 4.f) k = 4.f; \
            (out)[n].kind = PMOD_ADMIN; (out)[n].intensity = k/4.f; \
            (out)[n].demo_bonus = tune_f("PROVMOD_ADMIN_K", 0.06f) * k; \
            n++; \
        } \
        return n; \
    } while (0)
static inline int provmod_collect_prov(const ProvinceEconomy *re, ProvModHit out[], int max){
    ECON_PROVMOD_BODY(re, out, max, econ_prov_effcap);
}
static inline int provmod_collect(const RegionEconomy *re, ProvModHit out[], int max){
    ECON_PROVMOD_BODY(re, out, max, econ_region_effcap);
}
#undef ECON_PROVMOD_BODY

/* OUTILLAGE (télémétrie/diagnostic uniquement, jamais lu par le moteur) : prix moyen
 * d'un bien sur toutes les régions colonisées. Dédoublonné (chronicle.c ET econ_scan.c
 * portaient chacun une copie IDENTIQUE) — même corps, byte-identique par construction. */
static inline float econ_avg_price(const WorldEconomy *e, Resource res){
    double s=0.0; int n=0;
    for (int r=0;r<e->n_regions;r++) if (e->region[r].colonized){ s+=e->region[r].price[res]; n++; }
    return n? (float)(s/n):0.f;
}

/* Avance la simulation d'un pas. dt = années/tick (1 en annuel, 1/12 en mensuel) :
 * les processus cumulatifs (croissance, tech, impôt→trésor) suivent dt, les flux
 * production/consommation s'équilibrent par tick (satisfaction préservée). */
void econ_tick(WorldEconomy *e, float dt);
void econ_set_human(int cid);   /* §NF skippe les provinces du joueur humain (-1 = aucun) */
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
/* SAVETEST FIX — g_friche + g_lowsat_streak sont des ACCUMULATEURS croisant les ticks (pas du
 * scratch : ils pilotent prod_mult/mobility_move) que econ_mobility_reset() remet à zéro sur un
 * démarrage FRAIS mais que scps_load_game ne restaurait ni ne remettait à zéro. État statique →
 * save/load dédiés (même patron que econ_prodcap_save/_load ci-dessus). */
void econ_mobility_save(FILE *f);
bool econ_mobility_load(FILE *f);
/* F1 (v61) — le répit de colonisation g_colony_cd[] est un accumulateur inter-ticks :
 * sérialisé (section COLC) sinon un reload divergeait du fil continu (--savetest). */
void econ_colony_cd_save(FILE *f);
bool econ_colony_cd_load(FILE *f);
/* E1bis.10 — régions EN FRICHE (entretien impayé) au dernier econ_tick (télémétrie). */
long econ_friche_count(void);
/* I0 — L'INSTRUMENT : décomposition du flux d'or par empire (le robinet, ligne à ligne).
 * Chaque puits/source incrémente sa composante ; le chronicle RAZ par fenêtre et publie
 * la moyenne par empire. Signe : revenus +, dépenses −. Lecture seule, diagnostic. */
typedef enum {
    FX_TAX=0, FX_EXPORT, FX_TOLL_RECV,                /* revenus (+) */
    FX_UPKEEP, FX_COURT, FX_ADMIN, FX_ENCADR,         /* dépenses (−) : édifices/cour/admin/manuf */
    FX_SOLDE, FX_NAVY, FX_AUDIT, FX_TOLL_PAID, FX_INVEST, FX_CONSEIL, FX_IMPORT,
    FX_BUILD,                                         /* chantiers (agency : or des édifices commandés) */
    FX_REDEP,                                         /* redépense publique I3bis (le trésor circule le surplus) */
    FX_CREDIT,                                        /* intérêts de la dette (credit_year_tick) */
    FX_COUNT
} FluxComp;
void   econ_flux_add(int cid, FluxComp comp, float amount);   /* incrémente (signé par convention ci-dessus) */
double econ_flux_get(int cid, FluxComp comp);                 /* cumul depuis le dernier reset */
void   econ_flux_reset(void);                                 /* RAZ (début de fenêtre de mesure) */
const char *econ_flux_name(FluxComp comp);                    /* libellé court (français, outillage) */
/* MEMBRANE DE DÉCISION — le REVENU ANNUEL par pays (Σtaxes de la fenêtre écoulée),
 * capturé PUIS remis à zéro (remplace les sites nus d'econ_flux_reset : chronicle.c
 * et scps_api.c appellent désormais CETTE fonction au roulement d'année — un SEUL
 * site fait les deux gestes). `g_tax_lastyear` est un ACCUMULATEUR INTER-TICKS
 * (persiste d'une année sur l'autre) → SÉRIALISÉ (economy_flux_year_save/load,
 * section façade v62) pour que --savetest reste byte-identique après un reload. */
void   econ_flux_year_capture(void);
/* Le revenu annuel COURANT (dernière capture) d'un pays — 0 hors-borne. Sert de
 * base à d_treasury_mois (EvEffect) : montant = d_treasury_mois × (tax_year/12). */
float  econ_country_tax_year(int cid);
/* sérialisation de g_tax_lastyear[] (section façade, motif econ_colony_cd_save/load). */
void   econ_flux_year_save(FILE *f);
bool   econ_flux_year_load(FILE *f);
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

/* MÉTABOLISATION — part [0..1] des âmes de l'empire qui sont d'un AUTRE héritage que
 * la capitale ET assimilées (×integration) : le signal « creuset digéré ». Lu par la
 * recherche (boost) et la membrane (hover « métabolisation +X% »). Twin-inverse de
 * econ_off_culture_fraction (qui pèse le foreign NON encore digéré). */
float econ_country_metabolized(const World *w, const WorldEconomy *econ, int cid);
float econ_country_savoir(const WorldEconomy *econ, int cid);   /* SAVOIR : Σ strates pondérées × bonus bibliothèque (annuel) — la pop produit la recherche */
float econ_country_commerce(const WorldEconomy *econ, int cid); /* PUISSANCE COMMERCIALE : pool MENSUEL de volume échangeable (Σ strates marchandes × bonus chaîne commerciale) — drainé par les achats au marché */
/* MÉTABOLISATION VENTILÉE — out[r] = part [0..1] des âmes diaspora d'héritage r digérées.
 * Lu par la barre d'accès tech (Temps 2 : digérer le peuple r ⇒ accès aux signatures de r). */
void econ_country_heritage_digested(const World *w, const WorldEconomy *econ, int cid,
                                    float out[HERITAGE_COUNT]);
/* Poids du boost de recherche par métabolisation (Temps 1) — fallback compilé du
 * tunable runtime AI_METAB_RES_W (registre scps_tune_list.h). income ×= 1 + W·métabolisé. */
#ifndef AI_METAB_RES_W
#define AI_METAB_RES_W 1.0f
#endif
/* SAVOIR — fallbacks compilés (registre scps_tune_list.h) : la POP produit la recherche
 * (0.01·élite + 0.005·bourgeois + 0.001·journalier /an), la bibliothèque module en % (PER, plafond MAX). */
#ifndef SAVOIR_W_ELITE
#define SAVOIR_W_ELITE     0.01f
#endif
#ifndef SAVOIR_W_BOURGEOIS
#define SAVOIR_W_BOURGEOIS 0.005f
#endif
#ifndef SAVOIR_W_LABORER
#define SAVOIR_W_LABORER   0.001f
#endif
#ifndef SAVOIR_LIB_PER
#define SAVOIR_LIB_PER     0.067f
#endif
#ifndef SAVOIR_LIB_MAX
#define SAVOIR_LIB_MAX     0.33f
#endif
/* PUISSANCE COMMERCIALE — fallbacks compilés (registre scps_tune_list.h) : la POP MARCHANDE produit
 * le volume échangeable au marché (0.04·bourgeois + 0.01·élite /mois), la CHAÎNE COMMERCIALE module
 * en % (BLD_PER par point de PE_infra bâti, plafond BLD_MAX) ; ECO_W = son poids dans la puissance
 * éco de la diplo (diplo_eco_power). */
#ifndef COMMERCE_W_BOURGEOIS
#define COMMERCE_W_BOURGEOIS 0.04f
#endif
#ifndef COMMERCE_W_ELITE
#define COMMERCE_W_ELITE     0.01f
#endif
#ifndef COMMERCE_BLD_PER
#define COMMERCE_BLD_PER     0.10f
#endif
#ifndef COMMERCE_BLD_MAX
#define COMMERCE_BLD_MAX     0.50f
#endif
#ifndef COMMERCE_ECO_W
#define COMMERCE_ECO_W       0.05f
#endif

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
 * nouvellement colonisées ce tick.
 * F1/F2 (implémenteur colonisation/construction IA) — deux entrées OPTIONNELLES
 * (NULL ⇒ comportement intégral d'avant, les bancs restent inchangés) :
 *   w_expand[cid] : l'appétit d'expansion de l'éthos (0..~1) — CADENCE la
 *                   fondation (un Dominateur essaime presque chaque année, un
 *                   Pacifiste attend) au lieu du même rythme pour tous.
 *   at_war[cid]   : gate de guerre — un pays EN GUERRE (n'importe laquelle) ne
 *                   fonde PAS de colonie cette année (il consolide, il ne
 *                   s'étend pas sous le feu). PRÉ-CALCULÉ par l'appelant (plutôt
 *                   qu'un `DiploState*` brut) : scps_econ.c/.h reste feuille et
 *                   ne dépend PAS de scps_diplo (qui inclut DÉJÀ scps_econ.h —
 *                   un lien direct romprait plusieurs bancs légers qui ne lient
 *                   pas scps_diplo.o, sans toucher au Makefile). */
int econ_colonize_tick(WorldEconomy *e, const World *w, int skip_cid,
                        const float *w_expand, const bool *at_war);
/* E7 — TÉLÉMÉTRIE colonisation (statics de module, RAZ à econ_init, non sérialisés — motif
 * econ_friche_count) : cumul depuis la genèse de CETTE sim. `survival` = le sous-ensemble
 * fondé par la voie SURVIE anti-spirale (grenier vide, gates de pop/food_sat LEVÉS) —
 * TOUJOURS ⊆ `founded`. Pointeurs NULL ignorés individuellement. */
void econ_colony_stats(long *founded, long *survival);
/* L5 — colonie OUTRE-MER : mêmes portes (pop/vivres/cible vierge) mais coût pop ×2.
 * L'appelant (harnais) a vérifié Port + coque + portée de courants. */
bool econ_colonize_overseas(WorldEconomy *e, int src_rid, int dst_rid, int cid);
/* Fonde une colonie de `cid` sur `dst` depuis `src` (pop essaimée, owner posé).
 * Exposé pour la colonisation OUTRE-MER (scps_navy §8) — même acte fondateur. */
void econ_colonize_from(WorldEconomy *e, int src_rid, int dst_rid, int cid);
/* VERBE JOUEUR au grain PROVINCE (charte : « le joueur colonise n'importe quelle province ») :
 * portes de l'essaimage terrestre (pop/vivres/cible vierge) + CADENCE (un chantier à la
 * fois, 1 ordre/an). L'ordre OUVRE un CHANTIER (ponction pop immédiate) ; la colonie est
 * FONDÉE par econ_colony_day au terme (~1 an frontalier, plus loin = plus long).
 * false = refus, rien ne bouge. */
bool econ_colonize_province(WorldEconomy *e, const World *w, int src_pid, int dst_pid, int cid);
/* QUOTIDIEN — mûrit les chantiers de colonisation (cadences, délais, fondation à l'arrivée
 * au rendement log-distance). No-op intégral quand aucun chantier (chronique → golden). */
void econ_colony_day(WorldEconomy *e, const World *w);
/* RE-KEY PROVINCE — transfert de PROPRIÉTÉ D'UNE RÉGION ENTIÈRE (conquête/annexion/
 * sécession/cataclysme) : pose `new_owner` (et `colonized`) sur TOUTES les provinces
 * membres de `region`. econ->region[r].owner est un DÉRIVÉ (capitale, sinon meilleure
 * pop) recalculé par econ_aggregate_regions à chaque econ_tick — écrire region[r].owner
 * directement ne survit PAS au tick suivant. new_owner<0 = perte (colonized suit à
 * false). N'écrit PAS World (le pays/cellules) : l'appelant garde region_set_country
 * ou équivalent pour la hiérarchie World, si besoin. */
void econ_region_set_owner(WorldEconomy *e, const World *w, int region, int new_owner);

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
