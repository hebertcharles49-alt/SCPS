#ifndef SCPS_API_H
#define SCPS_API_H
/*
 * scps_api — façade C STABLE pour piloter le moteur SCPS depuis un hôte natif
 * (binding Godot/GDExtension, et plus tard le viewer lui-même).
 *
 * LE MOTEUR RESTE 100 % C : tout le calcul, le déterminisme et la sauvegarde
 * vivent ici ; l'hôte ne fait qu'AFFICHER et SAISIR. C'est la matérialisation de
 * la membrane — des OCTETS de carte, des NOMBRES tangibles, des VERBES — jamais
 * un flottant de physique §2.4 exposé tel quel.
 *
 * ── Fidélité ───────────────────────────────────────────────────────────────
 * `scps_sim_advance_days` roule le TICK PLEIN : `sim_day` (le cœur PARTAGÉ avec
 * la chronique, `scps_sim.c`) — agency, IA, events, économie, statecraft,
 * démographie, navy, révolte, world_tick, légitimité, commerce, intertrade,
 * prospérité, endgame, warhost, campagne, diplo, crédit, missions, factions.
 * Le monde Godot VIT pleinement (guerres, sécessions, prospérité réelle) et reste
 * DÉTERMINISTE — le hash de la chronique est inchangé (`make determinism`). La
 * surface de cette façade n'a PAS bougé en passant de la colonne éco au tick plein.
 */
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ScpsSim ScpsSim;   /* opaque : l'hôte ne voit jamais les structs moteur */

/* ---- cycle de vie ---------------------------------------------------- */
ScpsSim *scps_sim_new(void);
void     scps_sim_generate(ScpsSim *s, uint32_t seed);
void     scps_sim_advance_days(ScpsSim *s, int days);
void     scps_sim_free(ScpsSim *s);

/* ---- dimensions carte (constantes worldgen) -------------------------- */
int  scps_map_w(void);
int  scps_map_h(void);

/* ---- RENDU : remplit dst (map_w*map_h*4 octets, ordre RGBA) via render_map.
 * mode = ViewMode (0 = VIEW_TERRAIN). selected_prov = province surlignée (-1 = aucune ;
 * c'est de l'état d'AFFICHAGE, pas de simulation — il ne touche pas le déterminisme).
 * Le moteur peint, l'hôte uploade en texture. */
void scps_map_rgba(ScpsSim *s, uint8_t *dst, int mode, int selected_prov);

/* ---- COUCHES brutes (1 octet/cellule) pour shaders côté hôte ----------
 * SCPS_LAYER_WATER : masque d'EAU complet (mer OU lac). La couche SEA seule ignore les lacs
 * intérieurs (c->lake est un drapeau DISTINCT de c->sea) — d'où des bourgs posés SUR un lac.
 * L'overlay lit WATER pour TOUS ses tests d'assise (snap-terre, débord, poussée vers l'intérieur). */
enum { SCPS_LAYER_HEIGHT = 0, SCPS_LAYER_SEA, SCPS_LAYER_BIOME, SCPS_LAYER_COAST, SCPS_LAYER_WATER };
void scps_map_layer(ScpsSim *s, uint8_t *dst, int layer);

/* ---- nombres TANGIBLES (membrane) ------------------------------------ */
int    scps_year         (const ScpsSim *s);
int    scps_player       (const ScpsSim *s);
int    scps_country_count(const ScpsSim *s);
int    scps_region_count (const ScpsSim *s);
long   scps_world_pop    (const ScpsSim *s);
long   scps_country_pop  (const ScpsSim *s, int country);
double scps_country_gold (const ScpsSim *s, int country);

/* ---- par RÉGION (overlays / sprites côté hôte) ----------------------- */
int   scps_region_owner    (const ScpsSim *s, int region);
long  scps_region_pop      (const ScpsSim *s, int region);
bool  scps_region_colonized(const ScpsSim *s, int region);
/* centroïde MONDE (cellules) d'une région ; false si vide. Figé par worldgen. */
bool  scps_region_centroid (const ScpsSim *s, int region, float *x, float *y);

/* ---- PICKING : cellule MONDE (x,y) → province · -1 hors-monde / mer ---- *
 * La province est l'entité de panneau (province_readout) ; chaque cellule de
 * terre en porte une. Sa région (pour les getters par-région) = scps_province_region. */
int scps_province_at    (const ScpsSim *s, int x, int y);
int scps_province_region(const ScpsSim *s, int province);   /* province → région (-1) */

/* ====================================================================== */
/* READOUTS — la MEMBRANE traverse le binding : des MOTS déjà résolus et   */
/* des nombres TANGIBLES, jamais un flottant moteur. Remplis en UN appel ; */
/* les chaînes pointent dans les tables compilées (tr) ou le World/Econ du */
/* sim — STABLES tant que le sim vit ; l'hôte les copie (Godot String).    */
/* ====================================================================== */
typedef struct {
    const char *nom;     /* le mot du modificateur (faveur/fléau) */
    const char *effet;   /* une ligne : ce qu'il fait (survol) */
    int         faveur;  /* 1 = faveur (boon) ; 0 = fléau (malus) */
} ScpsProvMod;

#define SCPS_PROV_MODS 8
typedef struct {
    int         valid;                  /* 0 si province hors-borne */
    const char *nom, *terrain, *climat, *relief, *race;
    const char *stature, *flux, *vocation, *ressource;
    const char *humeur, *lignee;
    const char *aisance;                /* mot de la jauge prospérité locale */
    const char *defense, *specialisation;  /* peuvent être "" */
    long        ames;                   /* population (tangible) */
    int         owner;                  /* pays propriétaire (-1 si libre) */
    int         agitation;              /* 0-100 */
    int         aisance_val;            /* 0-100 : prospérité locale */
    int         humeur_val;             /* 0-100 : légitimité locale (l'humeur) */
    int         seuil_revolte;          /* 1 si l'agitation a franchi le seuil */
    long        logements_libres, logements_cap;
    long        services_libres,  services_cap;
    int         n_mods;
    ScpsProvMod mods[SCPS_PROV_MODS];
} ScpsProvInfo;

typedef struct {
    int         valid;                  /* 0 si pays hors-borne */
    const char *nom;
    const char *ethos;                  /* faction dominante (le régime effectif) */
    long        pop;
    double      gold;
    int         n_regions;
    int         stabilite;   const char *stabilite_mot;
    int         prosperite;  const char *prosperite_mot;
    int         legitimite;  const char *legitimite_mot;
    int         cohesion;    const char *cohesion_mot;
    int         savoir;      const char *savoir_mot;
    int         influence;              /* 0-100 : réputation diplomatique */
    int         corruption;             /* 0-100 : capture de l'État */
} ScpsCountryInfo;

void scps_province_info(ScpsSim *s, int province, ScpsProvInfo  *out);
void scps_country_info (ScpsSim *s, int country,  ScpsCountryInfo *out);

/* ---- ACTEURS SUR LA CARTE (Phase 3) ---------------------------------- *
 * Une ARMÉE de campagne par pays (si déployée) : où elle est, vers où elle marche,
 * sa phase (mot + brut pour l'anim), son effectif et sa composition. Tangibles. */
typedef struct {
    int         active;     /* 1 = armée de campagne déployée */
    int         region;     /* loc (où la dessiner ; centroïde via scps_region_centroid) ; -1 */
    int         dest;       /* région-but (ligne de marche) ; -1 = aucune */
    int         owner;      /* pays (= argument ; pratique côté hôte) */
    int         phase_id;   /* FieldPhase brut 0..6 (pour l'animation) */
    const char *phase;      /* mot de phase (Marche/Siège/Bataille/…) */
    long        units;      /* effectif (= paquets × 100) */
    long        inf, arch, cav, mages;   /* composition (effectifs) */
} ScpsArmyInfo;
void scps_army_info(ScpsSim *s, int country, ScpsArmyInfo *out);

/* TIER de ville d'une région (0-5 selon la pop ; capitale ≥ 4) ; -1 si non
 * colonisée / vide. Pour planter un marqueur de cité dimensionné au centroïde. */
int scps_region_tier(const ScpsSim *s, int region);

/* GROUPE de sprite de settlement (atlas scps_map_settlements : ligne) d'une région :
 * 0 montagne · 1 rivière · 2 estuaire · 3 rural · 4 marché · 5 fortifié (capitale).
 * -1 si non colonisée. Réplique (simplifiée) de la logique du viewer SDL. */
int scps_region_settle_group(const ScpsSim *s, int region);

/* ---- ENDGAME §27 (Phase 4) : entropie monde, fin latchée, merveille --- *
 * La membrane endgame (mots résolus + projections 0-100). Le moteur MUTE déjà le
 * monde quand une fin éclôt (régions englouties → mer, biomes blanchis, ronces) →
 * `render_map` reflète l'apocalypse PHYSIQUE ; ces champs ajoutent la LECTURE
 * (barre d'entropie, augure, bandeau de fin, épicentre). */
typedef struct {
    int         entropie_pct;   /* 0-100 : ratio entropy/seuil projeté */
    const char *entropie;       /* mot de bande (Stable/Frémissante/Instable/Au bord) */
    const char *augure;         /* ligne d'ambiance, ou "" */
    int         fin;            /* 0 aucune · 1 EAU · 2 FROID · 3 RONCES · 4 ascension */
    int         merv;           /* phase de merveille (0 aucune … 4 ascensionné) */
    int         merv_pct;       /* 0-100 : avancée du palier de merveille */
    int         cold_pct;       /* 0-100 : intensité du refroidissement (FROID) */
    int         sink_pct;       /* 0-100 : intensité de l'engloutissement (EAU) */
    int         epicenter_reg;  /* région-foyer du cataclysme, -1 si aucune */
} ScpsEndgameInfo;
void scps_endgame_info(ScpsSim *s, ScpsEndgameInfo *out);
/* état d'engloutissement d'une région (fin EAU) : 0 = non · 1 = programmée · 2 = engloutie. */
int  scps_region_sunken(const ScpsSim *s, int region);

/* ---- DÉTAIL DE PROVINCE (port fidèle de viewer.c) --------------------- *
 * Les données que draw_province_panel lit EN PLUS de province_info : la
 * composition (camemberts culture/idéologie), les revenus (production), la pop
 * par classe (barre empilée) et l'ossature de capitale. Tous membrane/tangibles. */
typedef struct {
    const char *race, *culture, *religion, *klass, *etat, *loyaute;  /* mots résolus */
    int         percent;     /* part de la province */
} ScpsGroup;
/* remplit out[0..min(n,max)-1] avec les groupes pop ; retourne le nombre écrit. */
int scps_province_groups(ScpsSim *s, int province, ScpsGroup *out, int max);

typedef struct { const char *source; float per_day; int manufactured; } ScpsIncome;
int scps_province_income(ScpsSim *s, int province, ScpsIncome *out, int max);

/* pop par classe (laboureurs · artisans/bourgeois · noblesse/élite). */
void scps_province_classes(ScpsSim *s, int province, long *laboureurs, long *artisans, long *noblesse);

typedef struct {
    const char *statut;   /* Hameau … Métropole (capitale_status) */
    int  tier;            /* 1..7 */
    long pop;             /* habitants */
    long logement_cap;    /* capacité de logement (capitale) */
    long service_cap;     /* capacité de services */
    int  prod_pct;        /* bonus de productivité, en % */
} ScpsCapitale;
void scps_province_capitale(ScpsSim *s, int province, ScpsCapitale *out);

/* ---- SIDEBAR : agrégats PAYS (port des sb_panel_*, read-only) --------- *
 * Démographie (classes + satisfaction) et stocks (par bien : stock, flux net,
 * couverture, état de marché). Agrégés sur les régions colonisées du pays. */
typedef struct {
    long pop_total;
    int  n_regions;
    long cls_pop[3];   /* journaliers · bourgeois · nobles */
    int  cls_sat[3];   /* satisfaction 0-100 par classe */
} ScpsCountryDemo;
void scps_country_demo(ScpsSim *s, int country, ScpsCountryDemo *out);

typedef struct {
    const char *name;       /* bien (resource_name) */
    const char *marche;     /* état de marché (label_marche) */
    long  stock;
    float net_day;          /* flux net /jour (offre−demande) */
    int   coverage_days;    /* jours de couverture si net<0 (366 = >1 an) ; -1 sinon */
    int   market_band;      /* 0..4 BandMarche (pour la couleur) */
    float price;            /* prix moyen (or) — pour l'onglet Marché */
    int   res_id;           /* indice Resource (enum) — pour le SPRITE de ressource */
} ScpsStock;
int scps_country_stocks(ScpsSim *s, int country, ScpsStock *out, int max);

/* COMMERCE (sb_panel_eco, onglet Commerce, read-only). */
typedef struct {
    const char *name;       /* le partenaire */
    float value;            /* or/an échangé */
    const char *status;     /* guerre · embargo · florissant · modeste */
    int   at_war, embargo;
} ScpsTradePartner;
/* head (routes · or exporté · tient-un-Centre) en out-params ; partenaires en retour. */
int scps_country_trade(ScpsSim *s, int country, int *routes, double *export_gold,
                       int *has_centre, ScpsTradePartner *out, int max);

/* CONSEIL (sb_panel_conseil, read-only) : 3 sièges (Savoir · Société · Industrie). */
typedef struct {
    const char *seat;       /* nom du siège */
    int   filled;           /* 1 si pourvu */
    const char *councilor;  /* nom du conseiller (tr) si pourvu, "" sinon */
    int   tier;             /* 1-3 (effet ×1/×1.5/×2) si pourvu */
} ScpsCouncilSeat;
int scps_country_council(ScpsSim *s, int country, ScpsCouncilSeat *out, int max);

/* RELATIONS diplomatiques d'un pays (sb_panel_diplo, read-only). */
typedef struct {
    const char *name;       /* le pays */
    const char *status;     /* Guerre · Allié · Vassal · Suzerain · Neutre */
    int   at_war;           /* 1 si en guerre */
    int   allied;           /* 1 si allié */
} ScpsRelation;
int scps_country_relations(ScpsSim *s, int country, ScpsRelation *out, int max);

/* ARMÉE d'un pays (sb_panel_armee, read-only) : mobilisation + flotte. L'armée de
 * CAMPAGNE (position/phase/composition) se lit via scps_army_info. */
typedef struct {
    long regiments;         /* force mobilisée (warhost_units) */
    int  levy;              /* cran de levée 0-3 */
    const char *levy_name;  /* Basse · Garde · Pied de guerre · Levée en masse */
    int  fleet;             /* total de coques */
} ScpsArmy;
void scps_country_army(ScpsSim *s, int country, ScpsArmy *out);

/* ====================================================================== */
/* CONSTRUCTION — les boutons « je veux telle unité / tel édifice », avec  */
/* le PRIX RÉEL et le survol (membrane : des mots résolus + des nombres).  */
/* ====================================================================== */

/* ROSTER MILITAIRE d'un pays : une ligne par type d'unité (les 22). Le survol
 * porte l'éthos, l'entretien et les contres ; le bouton est grisé si non recrutable. */
typedef struct {
    int         type;          /* UnitType brut (icône/sélection) */
    const char *nom;           /* unit_name */
    const char *classe;        /* « Journalier » / « Élite » */
    const char *arme;          /* weapon_name */
    const char *cout;          /* prix de levée : « 100 Armes légères » (la catégorie macro) */
    const char *ethos;         /* éthos qui le favorisent (affinité ≥ 2), joints ; « — » sinon */
    const char *fort;          /* « efficace contre » : unités battues (joints) ; « — » sinon */
    const char *faible;        /* « faible contre » : unités qui le battent ; « — » sinon */
    int         entretien_or10;/* solde : or/100/jour ×10 (uniforme) */
    int         entretien_vivre;/* ration/100/jour */
    int         recrutable;    /* 1 si la tech du pays débloque cette unité */
} ScpsUnitDef;
int scps_unit_roster(ScpsSim *s, int country, ScpsUnitDef *out, int max);

/* ÉDIFICES qu'un pays peut BÂTIR : nom + RECETTE de matériaux (jusqu'à 4) + prix OR
 * de la recette au marché de la capitale + durée ; grisé si la tech ne l'ouvre pas. */
typedef struct { const char *res; int qty; } ScpsBuildCost;
#define SCPS_BUILD_COSTS 4
typedef struct {
    int           type;        /* Edifice brut */
    const char   *nom;         /* edifice_name */
    int           n_cost;
    ScpsBuildCost cost[SCPS_BUILD_COSTS];   /* matériaux : nom de ressource + quantité */
    int           gold;        /* prix OR de la recette au marché de la capitale (agency_build_gold) */
    int           days;        /* durée de chantier */
    int           debloque;    /* 1 si la tech du pays l'ouvre */
} ScpsEdificeDef;
int scps_building_roster(ScpsSim *s, int country, ScpsEdificeDef *out, int max);

/* ---- ACTIONS DU JOUEUR (la main humaine sur le pays joueur) ----------- *
 * Les MÊMES actionneurs que l'IA (agency / warhost), jamais l'appel direct. Le
 * monde reste DÉTERMINISTE : ces verbes n'entrent pas dans sim_day (la chronique
 * ne les appelle pas) ; ils déposent un ordre que le prochain tick traitera. */
/* Bâtir un édifice sur la CAPITALE du joueur (chantier payé, agency_build_acct).
 * 1 = mis en file ; 0 = refus (trésor/ligne de crédit insuffisants, pas de capitale). */
int  scps_player_build  (ScpsSim *s, int edifice);
/* Lever 1 paquet (100) d'un TYPE d'unité. Renvoie les paquets levés (0 si tech
 * absente, pas d'élite, ou pas d'armes en stock). L'armée du joueur n'est plus
 * auto-gérée (warhost_set_human posé à la génération). */
long scps_player_recruit(ScpsSim *s, int unit);
/* Régler la jauge de levée du joueur (0 basse · 1 garde · 2 guerre · 3 masse). */
void scps_player_set_levy(ScpsSim *s, int level);

/* ---- TRACÉS DE CARTE (overlays vectoriels du viewer) ----------------- *
 * RIVIÈRES : un point par cellule de fil (centre cellule) + l'ANGLE du fil (rad,
 * direction monde) — l'hôte y pose un sprite de rivière TOURNÉ, comme draw_map_rivers.
 * Figé par worldgen. Remplit out[0..min(n,max)-1], renvoie le nombre écrit. */
typedef struct { float x, y, ang; } ScpsRiverPt;
int scps_river_points(ScpsSim *s, ScpsRiverPt *out, int max);

/* FRONTIÈRES : segments d'arête (coins, unités-cellule) entre souverainetés. Niveau
 * `level` : 0 = PROVINCE · 1 = RÉGION · 2 = PAYS (le joint est classé à son niveau le
 * plus FORT : pays > région > province — jamais doublé). Port du balayage bseg de
 * viewer.c. Recalculé à la demande (souveraineté = colonisation/conquête/sécession).
 * Remplit out[0..min(n,max)-1] (un segment = (x0,y0)-(x1,y1)), renvoie le nombre écrit. */
typedef struct { float x0, y0, x1, y1; } ScpsSeg;
int scps_border_segments(ScpsSim *s, int level, ScpsSeg *out, int max);

/* ROUTES TERRAIN-AWARE (port de viewer.c, en RÉSEAU à JONCTIONS) : A* sur la grille
 * (coût = relief + biome ; mer/lac contournés, fleuve = pont), reliant les régions des
 * routes commerciales TERRESTRES majeures. « Les routes attirent les routes » : une
 * cellule déjà sur une route est moins chère → les tracés FUSIONNENT en jonctions Y/T/X
 * au lieu de se croiser en désordre. Lissés (moyenne mobile). Display-only, mis en cache
 * (recalcul si le réseau change). `scps_roads_build` (re)bâtit + renvoie le nombre de
 * routes ; `scps_road_path` remplit la i-ème (points = centres de cellule), *level :
 * 0 = artère · 1 = desserte · 2 = mineure (du rang de capacité). */
typedef struct { float x, y; } ScpsRoadPt;
int scps_roads_build(ScpsSim *s);
int scps_road_path(ScpsSim *s, int i, ScpsRoadPt *out, int max, int *level);

#ifdef __cplusplus
}
#endif
#endif /* SCPS_API_H */
