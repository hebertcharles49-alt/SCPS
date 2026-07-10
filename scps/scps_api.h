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
 * L'overlay lit WATER pour TOUS ses tests d'assise (snap-terre, débord, poussée vers l'intérieur).
 * SCPS_LAYER_RIVER : DÉBIT accumulé par cellule (c->river, 0-255) — le worldgen. L'hôte CARVE les
 * cellules à fort débit comme de l'EAU dans la carte des biomes → le shader de terrain rend la
 * rivière comme la mer (carvée DANS le relief, pas un asset par-dessus). Seuil haut = fleuves majeurs.
 * SCPS_LAYER_CLIFF : intensité de FALAISE maritime (0-255) — dérivée à la lecture (dénivelé côtier,
 * world_cliff_intensity ; l'à-pic ÉMERGE de la dureté au worldgen). Le shader y pose des hachures
 * d'atlas ancien. Jamais sérialisée, jamais lue par le sim. */
enum { SCPS_LAYER_HEIGHT = 0, SCPS_LAYER_SEA, SCPS_LAYER_BIOME, SCPS_LAYER_COAST, SCPS_LAYER_WATER, SCPS_LAYER_RIVER, SCPS_LAYER_CLIFF };
void scps_map_layer(ScpsSim *s, uint8_t *dst, int layer);

/* ---- nombres TANGIBLES (membrane) ------------------------------------ */
int    scps_year         (const ScpsSim *s);
int    scps_day_of_year  (const ScpsSim *s);   /* jour dans l'année 0-364 (date d'affichage) */
int    scps_player       (const ScpsSim *s);
int    scps_country_count(const ScpsSim *s);
int    scps_region_count (const ScpsSim *s);
int    scps_province_count(const ScpsSim *s);
/* nombre de PROVINCES possédées par `country` (charte : la province est la vérité de
 * propriété — la topbar "provinces" compte ceci, pas un nombre de régions). */
int    scps_country_province_count(const ScpsSim *s, int country);
long   scps_world_pop    (const ScpsSim *s);
long   scps_country_pop  (const ScpsSim *s, int country);
double scps_country_gold (const ScpsSim *s, int country);
/* RÔLE de polité (PolityRole) : 0 joueur · 1 antagoniste(IA) · 2 cité-état · 3 vierge · 4 libre ; -1 hors-borne. */
int    scps_country_role (const ScpsSim *s, int country);
/* BROUILLARD : le JOUEUR a-t-il DÉCOUVERT ce pays ? (country_knows — la liste diplo ne
 * montre pas ce que le voile cache). Sans joueur engagé : tout est connu. */
int    scps_country_known(const ScpsSim *s, int country);

/* ---- par RÉGION (overlays / sprites côté hôte) ----------------------- */
int   scps_region_owner    (const ScpsSim *s, int region);
long  scps_region_pop      (const ScpsSim *s, int region);
bool  scps_region_colonized(const ScpsSim *s, int region);
/* centroïde MONDE (cellules) d'une région ; false si vide. Figé par worldgen. */
bool  scps_region_centroid (const ScpsSim *s, int region, float *x, float *y);
/* SIÈGE de ville : centroïde de la PROVINCE REPRÉSENTATIVE de la région (le bourg vit
 * dans une province, pas au barycentre de la région) ; repli centroïde région. */
bool  scps_region_seat     (const ScpsSim *s, int region, float *x, float *y);

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
    const char *nom, *terrain, *climat, *relief, *heritage;
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
    int         metab_pct;              /* MÉTABOLISATION : +X% de recherche (creuset digéré) — pour le hover savoir */
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

/* W-GUERRE UI (lot A) — ÉTAT DE GUERRE d'une région (pour les HACHURES de siège/
 * occupation sur la carte) : 0 = paix (rien à hachurer) · 1 = ASSIÉGÉE (une armée
 * de campagne ENNEMIE s'y tient en phase FA_SIEGE — le siège n'a pas encore abouti,
 * `region.owner` n'a PAS changé) · 2 = OCCUPÉE (dp->occupier[r] tient militairement
 * la région, ≠ owner — le siège a abouti mais la paix n'a pas encore transféré la
 * propriété). `belligerent_out` reçoit le pays qui assiège/occupe (teinte de la
 * hachure) ; -1 si état 0. Une région à la fois SIÉGÉE ET occupée par un AUTRE
 * (rare : ré-invasion) rapporte OCCUPÉE (l'état le plus avancé domine). PUR read
 * (aucune mutation), golden-safe. */
int scps_region_war_state(ScpsSim *s, int region, int *belligerent_out);

/* W-GUERRE UI (lot B) — LE PANNEAU DE COMBAT : quand deux forces s'affrontent (une
 * armée ATTAQUANTE en SIÈGE ou BATAILLE sur une région DÉFENDUE), le clic sur le
 * jeton doit pouvoir montrer les DEUX camps. `attacker`/`defender` = pays (-1 si
 * indéterminé — ex. région vierge). Compositions lues via campaign_composition
 * (déjà exposée par pays, réutilisée telle quelle). `war_score` = point de vue de
 * l'ATTAQUANT (diplo_war_score(a,d), [-100..+100], la même jauge que la diplomatie).
 * `in_battle` = 1 si une FieldBattle est ACTIVE sur cette région (chocs/accalmies en
 * cours) — alors `loss_*` portent les pertes CUMULÉES du combat en cours (paquets de
 * 100, report fractionnaire inclus) ; sinon 0 (pas de choc à exposer, seulement le
 * siège). `valid`=0 si `region` ne porte ni siège ni bataille (out mis à zéro). */
typedef struct {
    int    valid;
    int    region;
    int    attacker, defender;          /* pays ; -1 si indéterminé */
    int    phase_id;    const char *phase;   /* FA_SIEGE ou FA_BATTLE, mot résolu */
    long   atk_units, atk_inf, atk_arch, atk_cav, atk_mages;
    long   def_units, def_inf, def_arch, def_cav, def_mages;
    int    in_battle;                   /* 1 = FieldBattle active (chocs en cours) */
    float  loss_atk, loss_def;          /* pertes de choc cumulées (paquets), si in_battle */
    float  war_score;                   /* [-100..+100], point de vue attaquant */
} ScpsBattleInfo;
void scps_battle_info(ScpsSim *s, int region, ScpsBattleInfo *out);

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
    int         fin;            /* 0 aucune · 1 EAU · 2 FROID · 3 RONCES · 4 ascension · 5 SANG · 6 CHAUD */
    int         merv;           /* phase de merveille (0 aucune … 4 ascensionné) */
    int         merv_pct;       /* 0-100 : avancée du palier de merveille */
    int         cold_pct;       /* 0-100 : intensité du refroidissement (FROID) */
    int         sink_pct;       /* 0-100 : intensité de l'engloutissement (EAU) */
    int         fin_raw;        /* FinType MOTEUR brut — même échelle que `fin` depuis que le
                                  * miroir RFIN porte SANG (5) ; gardé pour compat (lavis V3). */
    int         epicenter_reg;  /* région-foyer du cataclysme, -1 si aucune */
    /* #32 (LE SANG SIGNE TON RÈGNE) — nombres TANGIBLES pour le hover de la bande
     * d'entropie/bandeau SANG : « le monde a saigné, et TOI, à quel point ? ». */
    int         blood_pct;         /* 0-100 : endgame_blood_ratio / ENDGAME_BLOOD_FRAC, plafonné 100 */
    int         blood_player_pct;  /* 0-100 : endgame_blood_player_share (la part DU joueur dans ce sang) */
} ScpsEndgameInfo;
void scps_endgame_info(ScpsSim *s, ScpsEndgameInfo *out);
/* état d'engloutissement d'une région (fin EAU) : 0 = non · 1 = programmée · 2 = engloutie. */
int  scps_region_sunken(const ScpsSim *s, int region);

/* LOT V3 — LAVIS PAR VARIANTE : intensité [0..1] d'une région selon la fin latchée
 * (EAU/FROID/RONCES/SANG ; 0 si AUCUNE ou ASCENSION). Pur wrapper d'endgame_region_
 * intensity (déjà écrit, LOT D) — jamais lu par viewer.c. */
float scps_endgame_region_intensity(const ScpsSim *s, int region);
/* CARTE DE VARIANTE (une passe C, jamais 512k appels GDScript) : remplit dst
 * (map_w*map_h octets) avec l'intensité 0-255 de la RÉGION de chaque cellule — la
 * même intensité qu'endgame_region_intensity, précalculée UNE FOIS par région
 * (≤832) puis recopiée par cellule (motif scps_map_owner). 0 partout si pas de
 * fin en cours (fin_raw==FIN_AUCUNE) — le lavis reste inerte, coût nul. */
void  scps_map_endgame_variant(ScpsSim *s, uint8_t *dst);

/* ---- DÉTAIL DE PROVINCE (port fidèle de viewer.c) --------------------- *
 * Les données que draw_province_panel lit EN PLUS de province_info : la
 * composition (camemberts culture/idéologie), les revenus (production), la pop
 * par classe (barre empilée) et l'ossature de capitale. Tous membrane/tangibles. */
typedef struct {
    const char *heritage, *culture, *religion, *klass, *etat, *loyaute;  /* mots résolus */
    int         percent;     /* part de la province */
} ScpsGroup;
/* remplit out[0..min(n,max)-1] avec les groupes pop ; retourne le nombre écrit. */
int scps_province_groups(ScpsSim *s, int province, ScpsGroup *out, int max);

typedef struct { const char *source; float per_day; int manufactured; int res_id; } ScpsIncome;
int scps_province_income(ScpsSim *s, int province, ScpsIncome *out, int max);

/* le POURQUOI de l'agitation comme MODIFICATEURS nommés : nom + apport SIGNÉ en
 * points (+ soulève / − apaise) + résorption/an si temporaire (0 = permanent). */
typedef struct { const char *cause; int delta; int decay; } ScpsBreakdownLine;
/* *out_value ← agitation 0-100 ; out[] ← les causes triées par poids. Retourne n. */
int scps_province_agitation(ScpsSim *s, int province, int *out_value, ScpsBreakdownLine *out, int max);

/* les MANUFACTURES bâties dans la province : nom + niveau + ouvriers. Retourne n. */
typedef struct { const char *nom; int niveau; int ouvriers; } ScpsProvBld;
int scps_province_buildings(ScpsSim *s, int province, ScpsProvBld *out, int max);
/* les ÉDIFICES de BASE bâtis (grenier, marché, temple, remparts…) — lus du masque
 * edi_built de la province. niveau/ouvriers = 0 (un édifice n'a pas d'équipe). */
int scps_province_edifices(ScpsSim *s, int province, ScpsProvBld *out, int max);

/* le JOURNAL d'évènements de la province : an + libellé résolu + signe
 * (+1 fléau · -1 faveur · 0 neutre) + HOVER « complet » (effets : production ↓,
 * population ↓, agitation ↑ … pour un évènement ; la ligne d'effet pour un
 * modificateur). La PLUS RÉCENTE en tête. Retourne n. */
typedef struct { int year; const char *label; int sign; char hover[128]; } ScpsLogEntry;
int scps_province_log(ScpsSim *s, int province, ScpsLogEntry *out, int max);

/* pop par classe (laboureurs · artisans/bourgeois · noblesse/élite). */
void scps_province_classes(ScpsSim *s, int province, long *laboureurs, long *artisans, long *noblesse);
/* SATISFACTION 0-100 par classe (−1 = classe vide), grain PROVINCE — incl. serviles. */
void scps_province_class_sat(ScpsSim *s, int province, int *lab, int *art, int *nob, int *slv);

/* UI PROVINCE — LOT 1 : le 4e SEGMENT (ESCLAVES) de la barre de proportions.
 * ⚠ ADDITIF (piège documenté TROUVAILLES.md) : scps_province_classes garde sa
 * signature 3-classes figée (des appelants en dépendent) — un reader À PART pour
 * ne rien casser. Somme des groupes CLASS_SLAVE de la province (0 si aucun). */
long scps_province_slave_count(ScpsSim *s, int province);

/* UI PROVINCE — LOT 3 : IMPÔTS DE LA PROVINCE. Le signal le plus HONNÊTE : la
 * MÊME formule que la collecte fiscale d'econ_tick (§6-7 : taux visé borné par la
 * tolérance éthos×classe, évasion au-delà), rejouée en LECTURE PURE sur les
 * strates DE LA PROVINCE (ProvinceEconomy.strata — pas la région agrégée),
 * projetée en or/AN (×12, la collecte réelle est mensuelle/dt mais econ_tick(dt)
 * n'expose pas dt à la façade — dt=1/12 est l'invariant du tick économique dans
 * tout le moteur, cf. sim_day). PUR read, aucune mutation, jamais désynchronisé
 * du réel (même econ_tax_tolerance, même STATE_TAX_AMBITION mirroré ici en
 * commentaire — scps_econ.c ne l'expose pas dans le .h, valeur 0.42f). */
double scps_province_tax(ScpsSim *s, int province);

/* UI PROVINCE — LOT 4 : le TERRAIN comme % de tenue de siège. DÉRIVE en lisible
 * ce que le moteur APPLIQUE déjà au siège (terrain_defense_mult(biome,height),
 * scps_army.c) — ⚠ scps_army.{h,c} sont en cours d'édition PARALLÈLE (agent
 * W-GUERRE, TROUVAILLES.md) : ne PAS y dépendre (risque de casse de build
 * partagé). La formule est REPLIQUÉE ici en lecture pure (coefficients par
 * biome copiés à l'identique, documentés en tête de fonction dans scps_api.c
 * avec le fichier:ligne source) — 100 = neutre (plaine, altitude nulle) ; >100 =
 * le terrain PROLONGE le siège d'autant (multiplicateur de DURÉE, pas un bonus
 * de combat — cf. terrain_combat_bonus, distinct, non répliqué ici). */
int scps_province_defense_pct(ScpsSim *s, int province);

/* UI PROVINCE — LOT 5 : SEED déterministe pour l'héraldique procédurale PAR
 * PROVINCE (miroir de compose_arms(cid) côté pays — heraldry.gd factorise en
 * compose_arms_generic(seed,...), ce seed en est l'entrée). Dérivé du hash
 * seed_x/seed_y de la province (figé worldgen) — jamais aléatoire, stable d'une
 * partie à l'autre. -1 si province hors-borne. */
int scps_province_seed(const ScpsSim *s, int province);

/* UI PROVINCE — LOT 6 : le marché LOCAL — prix/stock/bande des biens de la
 * province (dérivé pur, ProvinceEconomy.price/stock + le mot de bande de marché
 * existant, scps_readout.h). Remplit jusqu'à 3 lignes (les biens produits/consommés
 * les plus significatifs — même tri que province_income, brute+manufacturé),
 * retourne n. Le PORT (mot) est un extra sorti en *port_out ("" si aucun). */
typedef struct { const char *name; float price; float stock; const char *marche; } ScpsMarketLine;
int scps_province_market(ScpsSim *s, int province, ScpsMarketLine *out, int max, const char **port_out);

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

/* §5 PUISSANCE COMMERCIALE (menu marché) : le pool MENSUEL de volume échangeable + ce qu'il RESTE
 * + les sources (pop marchande, bonus de la chaîne commerciale) — la membrane du hover explicatif. */
typedef struct {
    float pool;        /* budget mensuel du volume échangeable au marché */
    float remaining;   /* ce qu'il RESTE à acheter ce mois-ci */
    float bourgeois;   /* pop marchande (source ×0.04) */
    float elite;       /* élite (source ×0.01) */
    int   bonus_pct;   /* bonus de la chaîne commerciale (édifices), en % */
} ScpsCommerce;
void scps_commerce_power(ScpsSim *s, int country, ScpsCommerce *out);

/* CONSEIL (sb_panel_conseil) : 3 sièges (Savoir · Société · Industrie). */
typedef struct {
    const char *seat;       /* nom du siège */
    int   filled;           /* 1 si pourvu */
    const char *councilor;  /* nom du conseiller (tr) si pourvu, "" sinon */
    int   tier;             /* 1-3 (effet ×1/×1.5/×2) si pourvu */
    int   age;              /* ÂGE du ministre assis (grandit avec l'année ; retraite 66-73) ; 0 si vacant */
    /* V2a — LE CONSEIL VIVANT : */
    const char *faction;    /* mot de SA faction-éthos (tr, faction_name) ; "" si vacant */
    int   loyalty;          /* 0..100 (0 si vacant) */
    float pay;              /* 0..2, curseur de paie (1.0 si vacant) */
    const char *mood;       /* mot d'ambiance (« dévoué »…« AU BORD DE LA TRAHISON ») ; "" si vacant */
} ScpsCouncilSeat;
int scps_country_council(ScpsSim *s, int country, ScpsCouncilSeat *out, int max);
/* Le curseur de PAIE du joueur (a[1]=paie ×100, 0..200) — verbe journalisé, revalidé
 * au drain (siège pourvu, borné [0,2]). */
int scps_player_council_pay(ScpsSim *s, int seat, float pay);
/* L'état de la PAIRE (a,b) de sièges du pays courant — 0=neutre 1=rivalité
 * 2=alliance 3=conspiration (V2b y branchera les événements). */
int scps_council_pair_state(ScpsSim *s, int seat_a, int seat_b);
/* CANDIDATS d'un siège (la pool de la génération COURANTE — se renouvelle tous les
 * SC_COUNCIL_GEN_YEARS, toujours pleine) : nom résolu + tier + ÂGE + coût/mois (×IPM).
 * Pour l'embauche ÉCLAIRÉE du joueur (player_council_hire(seat, slot)). */
typedef struct { int slot; const char *nom; int tier; int age; float cost; } ScpsCouncilCand;
int scps_council_candidates(ScpsSim *s, int seat, ScpsCouncilCand *out, int max);

/* DÉCRETS DU JOUEUR (civics) — sb_panel_decrets. Chaque décret DÉPLACE un levier
 * existant tant qu'il est actif ; `plateaux` en donne les DEUX faces (gain/contrepartie).
 * `legal` = la condition d'entrée est remplie MAINTENANT (pour griser le bouton) — une
 * réforme DÉJÀ active reste toujours "legal" (affichée verrouillée, pas refusée).
 * `reforme` = 1 si le type est une RÉFORME (irréversible une fois active). */
typedef struct {
    int         id;         /* DecreeId — à repasser à scps_player_decree */
    const char *nom;
    const char *flavor;     /* une ligne cynique */
    const char *plateaux;   /* description des deux faces */
    int         reforme;    /* 1 = irréversible une fois actif */
    int         active;     /* 1 = actif pour ce pays */
    int         legal;      /* 1 = la condition d'entrée est remplie (activable maintenant) */
} ScpsDecree;
/* liste TOUS les décrets pour `country` (état + légalité). Retourne le nombre écrit. */
int scps_decrees_list(ScpsSim *s, int country, ScpsDecree *out, int max);

/* RELATIONS diplomatiques d'un pays (sb_panel_diplo, read-only). */
typedef struct {
    const char *name;       /* le pays */
    const char *status;     /* Guerre · Allié · Vassal · Suzerain · Neutre */
    int   at_war;           /* 1 si en guerre */
    int   allied;           /* 1 si allié */
    int   opinion;          /* #26 : opinion ±100 de l'AUTRE pays envers nous (la mémoire des actes) */
    int   country;          /* §3 : l'index pays de l'AUTRE (cible des verbes/options diplo) */
} ScpsRelation;
int scps_country_relations(ScpsSim *s, int country, ScpsRelation *out, int max);

/* §3 — OPTIONS DIPLO : la LÉGALITÉ des verbes du JOUEUR contre `target` (pour GRISER les boutons).
 * `can_*` = structurellement permis ; `would_accept_*` = APERÇU du consentement du vis-à-vis
 * (ai_consider_offer — l'opinion #26) : le bouton peut être actif mais l'offre refusée. Retour
 * 1 si `target` est une cible valide (rempli), 0 sinon (tout à 0). */
typedef struct {
    int can_declare_war;        /* pas déjà en guerre · pas de trêve · cible vivante ≠ soi */
    int can_make_peace;         /* en guerre avec la cible */
    int can_offer_alliance;     /* pas en guerre · pas déjà allié · un slot libre des DEUX côtés */
    int can_offer_pact;         /* pas en guerre · pas déjà de pacte */
    int can_offer_migration;    /* BRASSAGE : pas en guerre · pas déjà de pacte migratoire */
    int can_embargo;            /* pas d'embargo en cours */
    int can_lift_embargo;       /* embargo en cours */
    int would_accept_alliance;  /* le vis-à-vis CONSENTIRAIT (opinion + relation) */
    int would_accept_pact;      /* idem (opinion + complémentarité) */
    int would_accept_migration; /* BRASSAGE : le vis-à-vis CONSENTIRAIT au pacte migratoire */
    int would_accept_peace;     /* le vis-à-vis CÉDERAIT (score de guerre / épuisement) */
    /* W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ : can_declare_war reste la légalité STRUCTURELLE
     * (pas déjà en guerre, pas de trêve) ; ces trois champs disent si une déclaration
     * OFFENSIVE (territorial/économique/religieux) est actuellement ARMÉE. Un CB « gratuit »
     * (défensif/subjugation/anti-piraterie) n'a besoin d'AUCUN de ces trois. */
    int   can_fabricate;           /* peut fabriquer MAINTENANT (or suffisant, pas déjà en cours) */
    float fabricate_cost;          /* le prix (2 ans de revenu de la cible), pour l'affichage */
    int   fabricating;             /* une intrigue est EN COURS de maturation contre cette cible */
    float fabricating_days_left;   /* jours avant maturité (fabricating==1) */
    int   cb_ready;                 /* une intrigue MÛRE (utilisable) existe contre cette cible */
    float cb_ready_years_left;     /* années avant expiration (cb_ready==1) */
} ScpsDiploOptions;
int scps_diplo_options(ScpsSim *s, int target, ScpsDiploOptions *out);

/* ── le RÉSUMÉ D'OPINION (#26) : POURQUOI ce pays nous voit ainsi. `total` = l'opinion
 * COURANTE ±100 (lissée) ; le reste = les COMPOSANTES de la cible vers laquelle elle
 * converge : la MÉMOIRE des actes (durable — trahison, sécession d'une guerre civile —
 * s'estompe sur des années) + les modificateurs de STATUT (0 = inactif) + la rancune
 * territoriale. Des nombres tangibles, membrane tenue. */
typedef struct {
    int total;                                /* opinion courante ±100 */
    int memory;                               /* mémoire des actes (±, durable) */
    int ally, war, vassal, pact, embargo;     /* statuts actifs (0 si inactif) */
    int rancor;                               /* rivalité territoriale (−) */
} ScpsOpinionParts;
/* ce que `country` pense du JOUEUR, décomposé. 0 rempli · -1 cible invalide. */
int scps_opinion_summary(ScpsSim *s, int country, ScpsOpinionParts *out);

/* ── le JOURNAL D'ACTES : la SOUS-DÉTAILLE de « Mémoire » — l'histoire DATÉE des actes
 * entre le JOUEUR et `country` (« a déclaré la guerre », « pacte commercial », « a
 * trahi », « né d'une sécession »…). `act` = DiplogAct (scps_provlog.h) ; a_id → b_id
 * = qui a agi envers qui. `delta_now` ≠ 0 = acte de MÉMOIRE : le poids RESTANT dans
 * l'opinion (décayé — s'estompe sur des années). Le plus RÉCENT d'abord. Runtime :
 * l'histoire d'avant un chargement repart vide (même charte que le journal province). */
typedef struct { int year, act, a_id, b_id, delta_now; } ScpsDiploAct;
int scps_diplo_journal(ScpsSim *s, int country, ScpsDiploAct *out, int max);
/* §3 — LÉGALITÉ de CONSTRUCTION par RÉGION : 1 si le joueur peut bâtir `edifice` dans `region`
 * MAINTENANT. Miroir READ-ONLY des gates du drain CMD_BUILD (agency_build_acct, même ordre) :
 * région à soi · géo (port/Centre) · palier de FAMILLE · file F5 · TECH DE PALIER (LOT T :
 * edifice_tier ⇐ econ_country_has_tier, T1 libre) · MATIÈRE au marché atteignable · OR.
 * Le roster (debloque) gate la tech SPÉCIFIQUE de quelques édifices (Comptoir/Entrepôt,
 * edifice_unlocked) ; ce prédicat gate en plus, pour TOUT édifice de palier ≥2, la PREUVE
 * générique que le pays a atteint ce tier de recherche. region<0 ⇒ capitale.
 * `_ex` rapporte la RAISON du refus : 0 OK · 1 structurel · 2 or insuffisant · 3 matière
 * manquante · 4 tech de palier manquante. */
int scps_build_legal(ScpsSim *s, int region, int edifice);
int scps_build_legal_ex(ScpsSim *s, int region, int edifice, int *reason_out);

/* PANNEAU B — le joueur pose une MANUFACTURE civile (le §NF l'exclut : voici la main).
 * bld = BuildingType (l'index que scps_region_alloc nomme déjà). Le verbe ENFILE
 * (drain revalidé) ; la légalité est le miroir read-only des gates du drain
 * (à soi · civil · slot libre · staffage · tier · intrant nourrissable · or). */
int scps_player_build_manuf(ScpsSim *s, int region, int bld);
int scps_manuf_legal(ScpsSim *s, int region, int bld);
/* le PRIX du chantier de manufacture — LE montant que le drain débite (MANUF_BUILD_COST
 * × ipm, même formule que scps_manuf_legal/CMD_BUILD_MANUF) : or, arrondi (tangible). */
int scps_manuf_cost(ScpsSim *s);

/* ── ESCLAVAGE — la strate CLASS_SLAVE : garder/affranchir/vendre --------------
 * L'AFFRANCHISSEMENT (granularité PAYS, une politique) : CMD_MANUMIT, aucun
 * argument (agit sur le joueur). Renvoie 1 si le verbe a pu s'enfiler. */
int scps_player_manumit(ScpsSim *s);
/* LE MARCHÉ DES CENTRES : region = une région AU JOUEUR, count = âmes. La vente
 * retire des groupes esclaves du joueur (crédite le pool mondial + l'or) ; l'achat
 * est REVALIDÉ au drain contre le gate éthos/tech (un abolitionniste voit son ordre
 * silencieusement sans effet, comme les offres diplo non consenties). */
int scps_player_slave_sell(ScpsSim *s, int region, long count);
int scps_player_slave_buy (ScpsSim *s, int region, long count);
/* LECTEUR (membrane) : le pool mondial par héritage (noms résolus) + le total, et
 * si LE JOUEUR peut acheter maintenant (would_accept-like : un aperçu, pas une garantie
 * — le pool peut se vider avant le drain). */
typedef struct { const char *heritage; long count; } ScpsSlavePoolLine;
int  scps_slave_market(ScpsSim *s, ScpsSlavePoolLine *out, int max, long *total_out, int *can_buy_out);
/* PRIX COURANTS du marché servile (or par âme, arrondis) : le SPREAD que le drain débite —
 * achat = ×2 (double taxe des Centres), vente = ×1, tous deux × ipm × respiration du pool
 * (rare ⇒ cher). Miroir READ-ONLY d'intertrade_slave_buy/_sell (la formule du pool y est
 * static : recomposée ici depuis les lecteurs publics — cf. TROUVAILLES lot M). */
void scps_slave_prices(ScpsSim *s, int *buy_out, int *sell_out);

/* LOT G — RÉINCORPORATION DE POP : déplace `count` âmes de la classe `klass`
 * (SocialClass) de `src_region` vers `dst_region` (les deux AU JOUEUR — le verbe
 * ENFILE, revalidé au drain). Les groupes culturels suivent PROPORTIONNELLEMENT
 * (migration_move : heritage/arrival/integration/klass conservés). Coût : coercition
 * à la source pour les strates LIBRES ; CLASS_SLAVE exempt (on déplace une propriété). */
int scps_player_pop_transfer(ScpsSim *s, int src_region, int dst_region, int klass, long count);

/* LOT J — L'APERÇU DE MANUMISSION : ce que l'affranchissement du JOUEUR libérerait
 * MAINTENANT (lecture PURE, aucune mutation) — les mots + les nombres, avant le
 * choix. `friction_after` estime la part off-culture (économique) que ces âmes
 * représenteront une fois DANS la membrane libre (motif des options-readers). */
typedef struct {
    long  souls;            /* âmes qui seraient affranchies */
    int   n_groups;         /* groupes esclaves concernés */
    float pct_of_country;   /* part de la pop TOTALE du pays [0..100] */
    float friction_after;   /* estimation de friction off-culture post-affranchissement [0..1] */
} ScpsManumitPreview;
int scps_manumit_preview(ScpsSim *s, ScpsManumitPreview *out);

/* ARMÉE d'un pays (sb_panel_armee, read-only) : mobilisation + flotte. L'armée de
 * CAMPAGNE (position/phase/composition) se lit via scps_army_info. */
typedef struct {
    long regiments;         /* force mobilisée (warhost_units) */
    int  levy;              /* cran de levée 0-3 */
    const char *levy_name;  /* Basse · Garde · Pied de guerre · Levée en masse */
    int  fleet;             /* total de coques */
    int  posture;           /* posture de campagne 0 prudente · 1 standard · 2 agressive (lue du moteur) */
    const char *posture_name;
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
    int           tier;        /* palier familial (edifice_tier : Sanctuaire 1 → Temple 2 → …) */
    int           prev;        /* Edifice précédent de la famille (-1 = base de famille) */
    int           prev_built;  /* 1 si le précédent est bâti QUELQUE PART dans l'empire
                                * (l'UI ne MONTRE un palier que si le précédent existe) */
    const char   *effet;       /* l'effet RÉEL chiffré, composé du delta ProvBuild (membrane) */
    const char   *flavor;      /* la ligne CYNIQUE du conseiller (display-only) */
} ScpsEdificeDef;
int scps_building_roster(ScpsSim *s, int country, ScpsEdificeDef *out, int max);

/* ---- ALLOCATION DE MAIN-D'ŒUVRE (onglet province) ---------------------- *
 * Les PUITS de main-d'œuvre d'une région : chaque brute extraite (kind 0) et chaque
 * manufacture bâtie (kind 1). Le joueur règle un POIDS par puits (somme normalisée à
 * 100 %) ; un poids 0 sur une manufacture = FERMÉE. `on` indique si l'override est
 * actif (sinon le moteur répartit en AUTO et `weight` reflète la part ACTUELLE). */
typedef struct {
    int         kind;       /* 0 = extraction (brute) ; 1 = manufacture (bâtiment) */
    int         id;         /* kind 0 : Resource ; kind 1 : BuildingType */
    const char *name;       /* nom du puits (resource_name / building_name) */
    const char *output;     /* kind 1 : bien produit (resource_name) ; kind 0 : NULL */
    int         weight;     /* poids brut 0-255 (override) OU part suggérée (mode AUTO) */
    int         pct;        /* part normalisée 0-100 du bassin */
    float       workers;    /* bras employés au dernier tick (kind 1 : réel ; kind 0 : estimé) */
    int         closed;     /* kind 1 : fermé (override & poids 0) */
    int         input;      /* kind 1 : choix d'intrant 0/1 ; -1 si pas d'alternative */
    const char *alt_name;   /* kind 1 : nom de l'intrant ALTERNATIF (NULL si aucun) */
    const char *in_name;    /* kind 1 : nom de l'intrant PRIMAIRE (NULL si aucun) */
} ScpsAllocSink;
#define SCPS_ALLOC_MAX 48
typedef struct {
    int           region;   /* région lue (-1 si invalide) */
    int           on;       /* override actif ? */
    float         pool;     /* bassin de bras de la région (journaliers + bourgeois) */
    int           n;        /* nombre de puits remplis */
    ScpsAllocSink sink[SCPS_ALLOC_MAX];
} ScpsAlloc;
/* Lit l'allocation d'une RÉGION (pas une province : l'unité moteur). PUR read. */
void scps_region_alloc(ScpsSim *s, int region, ScpsAlloc *out);

/* ---- ACTIONS DU JOUEUR (la main humaine sur le pays joueur) ----------- *
 * ENFILÉES dans le journal de commandes (scps_sim.h) : ces verbes n'appliquent
 * RIEN à l'instant de l'appel — ils déposent un ordre que le prochain sim_day
 * VIDE à un point fixe (après agency_advance, avant l'IA), chacun REVALIDÉ. Le
 * monde reste DÉTERMINISTE et REJOUABLE (graine + journal). Retour = accepté-
 * dans-la-file, PAS le verdict d'application (trésor/matière), qui tombe au tick. */
/* Bâtir un édifice. region<0 ⇒ la CAPITALE du joueur ; sinon la région visée (le
 * drain revalide l'appartenance). 1 = mis en file ; 0 = file pleine / argument hors domaine. */
int  scps_player_build  (ScpsSim *s, int edifice, int region);
/* Lever 1 paquet (100) d'un TYPE d'unité. 1 = mis en file ; 0 = refus d'enfilement.
 * (Le verdict réel — tech, élite, armes en stock — tombe au drain.) */
long scps_player_recruit(ScpsSim *s, int unit);
/* Régler la jauge de levée du joueur (0 basse · 1 garde · 2 guerre · 3 masse). */
void scps_player_set_levy(ScpsSim *s, int level);
/* RECHERCHE : fixer la cible de tech du joueur (file de 1). tech<0 ⇒ annule.
 * 1 = mis en file ; 0 = refus. La progression (income SAVOIR × prospérité) tombe au tick. */
int  scps_player_research(ScpsSim *s, int tech);
/* §3 — VERBES DIPLO (capstone #26). Le joueur PROPOSE ; le vis-à-vis ÉVALUE au drain via
 * ai_consider_offer (l'opinion ±100 + la relation + le score de guerre) → l'offre n'aboutit que
 * si l'autre CONSENT. declare_war / embargo sont unilatéraux. Retour = mis-en-file (1) / refus (0) ;
 * le VERDICT (accepté ?) se lit ensuite dans country_relations (statut/allié/at_war). `target` = cid. */
int  scps_player_declare_war   (ScpsSim *s, int target);
int  scps_player_make_peace    (ScpsSim *s, int target);   /* offre de paix BLANCHE (si l'autre cède) */
int  scps_player_offer_alliance(ScpsSim *s, int target);
int  scps_player_offer_pact    (ScpsSim *s, int target);
int  scps_player_offer_migration(ScpsSim *s, int target); /* BRASSAGE : pacte migratoire (échange passif de pop) */
int  scps_player_embargo       (ScpsSim *s, int target, int on);
/* W-GUERRE-3 — FABRIQUER une revendication (payante, 2 ans de revenu de la cible) contre
 * `target` : mûrit 1 an, valide 5 ans une fois mûre. Un CB offensif (territorial/économique/
 * religieux) n'est déclarable qu'avec une intrigue MÛRE — cf. scps_diplo_options pour l'état. */
int  scps_player_fabricate_cb  (ScpsSim *s, int target);
/* §3 — INTÉRIEUR · COMMERCE · GUERRE (plomberie additive ; ENFILENT, revalidé au drain).
 * `region` = index de région À SOI ; `seat` ∈ [0,3) ; `good` ∈ Resource ; `hull` ∈ HullType ;
 * `posture` 0 prudente/1 standard/2 agressive. Retour = mis-en-file (1) / refus (0). */
int  scps_player_repress       (ScpsSim *s, int region);
int  scps_player_assimilate    (ScpsSim *s, int region, int creuset);
int  scps_player_purge         (ScpsSim *s, int region);
int  scps_player_council_hire  (ScpsSim *s, int seat, int slot);
int  scps_player_council_dismiss(ScpsSim *s, int seat);
/* DÉCRETS (civics) : bascule le décret `id` (on=1/0). Revalidé au drain (id borné,
 * l'activation exige la condition d'entrée, une réforme active refuse le off). */
int  scps_player_decree        (ScpsSim *s, int id, int on);
int  scps_player_route         (ScpsSim *s, int ra, int rb, int maritime);
int  scps_player_market_buy    (ScpsSim *s, int region, int good, long qty, int tier);
int  scps_player_market_sell   (ScpsSim *s, int region, int good, long qty, int tier);
int  scps_player_campaign      (ScpsSim *s, int from_region, int target_region);
int  scps_player_posture       (ScpsSim *s, int posture);
int  scps_player_refill        (ScpsSim *s);
int  scps_player_navy_build    (ScpsSim *s, int hull);
int  scps_player_disband       (ScpsSim *s);
/* LOT P (2026-07-07) — PILLER LA CÔTE : une province CÔTIÈRE d'un AUTRE pays (ni allié,
 * ni pacte), la piraterie restant un acte GRIS (miroir de la course pirate IA : la
 * guerre n'est PAS requise). Exige ≥1 coque PIRATE. ENFILE (drain revalidé) : le MÊME
 * chemin de pillage unifié (20% du revenu annuel de la victime + esclavage 5% si le
 * gate — tech OU éthos conquérant) que le sac de siège/l'occupation, + pose la balafre/
 * l'immunité (5 ans, comme un raid pirate réussi). `scps_can_raid_coast` = lecture PURE
 * pour GRISER le bouton ; `reason_out` optionnel : 0 OK · 1 pas côtière/peuplée · 2 à
 * soi/allié/pacte · 3 balafre active (CD) · 4 pas de coque pirate. */
int  scps_can_raid_coast       (ScpsSim *s, int prov, int *reason_out);
int  scps_player_raid_coast    (ScpsSim *s, int prov);
/* le CD restant (jours) de la balafre/immunité de la province — « côte balafrée — X j ». */
int  scps_raid_cd_days         (ScpsSim *s, int prov);
/* BANC SEULEMENT (motif intertrade_debug_set_hub_of, lot A) : pose N coques pirate au
 * joueur — le round-trip de scps_api_demo exige une coque. Jamais appelé par le jeu. */
void scps_debug_set_pirate_hulls(ScpsSim *s, int n);
/* ALLOCATION (onglet province) — ENFILENT, revalidé au drain (région à soi ; res/bld bornés).
 * Poser un poids ACTIVE l'override de la région ; weight 0 sur un bâtiment = FERMÉ. */
int  scps_player_alloc_raw     (ScpsSim *s, int region, int resource, int weight);
int  scps_player_alloc_bld     (ScpsSim *s, int region, int bld_type, int weight);
int  scps_player_alloc_input   (ScpsSim *s, int region, int bld_type, int input);
int  scps_player_alloc_auto    (ScpsSim *s, int region);   /* retour au split AUTO */
/* §7 — ENGAGEMENT D'ÂGE : l'IA s'engage AUTO au lever d'un âge, le joueur CHOISIT.
 * scps_age_state = l'âge COURANT (-1 = aucun levé), *engaged ← le joueur l'a déjà
 * engagé, name ← son NOM (mot résolu — membrane). scps_player_age_engage ENFILE
 * CMD_AGE_ENGAGE (drain déterministe, une fois par âge). */
int  scps_age_state         (ScpsSim *s, int *engaged, char *name, int cap);
int  scps_player_age_engage (ScpsSim *s);
/* COLONISATION (charte : « le joueur colonise n'importe quelle province ») — ENFILE
 * CMD_COLONIZE (source = sa province la plus peuplée, portes au drain) ; scps_can_colonize
 * = le read de légalité (cible vierge + une source aux portes + CADENCE v50) pour griser. */
int  scps_player_colonize   (ScpsSim *s, int prov);
int  scps_can_colonize      (ScpsSim *s, int prov);
/* CHANTIER de colonisation du joueur (v50 : une colonie MÛRIT — ~1 an frontalier, plus
 * loin = plus long ; rendement log-distance capitale ; 1 ordre/an). Renvoie 1 si un
 * chantier est en cours (dst/days/total remplis) ; cd_days/yield_pct toujours remplis. */
int  scps_colony_status     (ScpsSim *s, int *dst_prov, int *days_left, int *total_days,
                             int *cd_days, int *yield_pct);
/* NOURRITURE disponible d'un pays (Σ stock vivrier de ses provinces — topbar). */
double scps_country_food    (const ScpsSim *s, int c);
/* LE DIPLOMATE (v50) : jours avant le prochain acte diplo joueur permis (0 = prêt). */
int  scps_diplo_cd          (const ScpsSim *s);
/* total de provinces COLONISÉES (signature de souveraineté du front — une colonisation
 * intra-région ne bouge pas l'owner agrégé de région) + province-CAPITALE d'un pays. */
int  scps_colonized_total   (const ScpsSim *s);
int  scps_country_capital_province(const ScpsSim *s, int c);
/* LECTURE : cible de recherche courante (-1 = aucune) ; *progress01 ← fraction [0..1]. */
int  scps_research_target(ScpsSim *s, float *progress01);

/* ── ALERTES (les DEUX voies du front, façon EU4/CK3) ─────────────────────────────
 * VOIE ÉVÈNEMENTS : le FIL poussé par sim_day (observation d'état gatée joueur —
 * guerre/paix, place tombée/reprise, pillage, révolte, sécession ; kind = FeedKind,
 * scps_provlog.h). Les NOMS sont résolus (membrane). Poll incrémental par seq. */
typedef struct {
    int seq, year, kind, region;   /* region -1 si non localisé */
    int v;                         /* valeur libre du kind (FEED_PEACE : SCORE de guerre ±100 · FEED_DIRECTOR : EvId) */
    int a_id, b_id;                /* index pays BRUTS (le filtre de pertinence du front) */
    const char *a_name, *b_name;   /* pays concernés ("" si -1) */
    const char *label;             /* FEED_DIRECTOR : le NOM de l'évènement (résolu) ; "" sinon */
} ScpsFeedEvent;
int scps_feed_poll(ScpsSim *s, int after_seq, ScpsFeedEvent *out, int max);

/* ── MEMBRANE DE DÉCISION — LA FILE JOUEUR (3e voie des alertes) ────────────────────
 * Un évènement à VRAIE décision (n_options>1) qui concerne le JOUEUR n'est pas tranché
 * par l'IA à sa place : il ATTEND dans EventsState.pending[] (EventsState, scps_events.h).
 * `situation` = le nom de l'évènement (résolu, "Le contremaître réclame"…) ; `labels`/
 * `flavors` = les 3 choix (label court + ce qu'il raconte, en mots — jamais un nom SCPS,
 * le même gate events_text_clean les couvre) ; `region` = la province concernée (-1 si
 * EV_COUNTRY) ; `days_left` = jusqu'à l'auto-résolution (ai_chance), pour l'urgence UI. */
typedef struct {
    const char *situation;         /* le NOM de l'évènement (résolu — membrane) */
    const char *labels[4];         /* les choix — jusqu'à 4 (le max de la table) */
    const char *flavors[4];        /* ce que RACONTE chaque choix (tooltip) */
    const char *advisors[4];       /* QUI porte ce choix au conseil (mot de faction, "" si aucun) —
                                    * les trois choix ont des VISAGES : trahir une option = trahir
                                    * quelqu'un qui reste au conseil (hook.faction → faction_name) */
    const char *effets[4];         /* l'EFFET MÉCANIQUE en mots+signes (« Ça veut dire quoi ? ») :
                                    * trésor ±N % du revenu mensuel · légitimité ↑/↓ · agitation ↑/↓ ·
                                    * population ±N % · cicatrice durable · pari (N %). "" si neutre. */
    int n_options;
    int region;                    /* -1 si le sujet est un PAYS (EV_COUNTRY) */
    int days_left;                 /* avant auto-résolution (180 j au total) */
    int evid;                      /* EvId moteur — clé de l'ILLUSTRATION thématique (event_art.gd) */
} ScpsPendingEvent;
/* nombre d'évènements EN ATTENTE du choix du joueur. */
int scps_pending_count(ScpsSim *s);
/* lit le pending au slot `slot` (0..scps_pending_count()). 0 = slot invalide (out inchangé). */
int scps_pending_event(ScpsSim *s, int slot, ScpsPendingEvent *out);
/* CHOISIT l'option `option` du pending au slot `slot` — ENFILE CMD_EVENT_CHOICE
 * (drain déterministe, revalidé). 1 = mis en file, 0 = refus (slot/option hors-borne). */
int scps_player_event_choice(ScpsSim *s, int slot, int option);

/* ── LES ANNALES DU RÈGNE — un récit SÉLECTIF de la partie (§ Annales) ──────────────
 * L'anneau EventsState.annals[] n'accroche QUE le pays JOUEUR (scps_events.c,
 * resolve_choice/world_events_tick/sim_day) : dilemmes résolus, cicatrices mûries,
 * âges advenus, la Fin du monde. `ligne` est une phrase DIÉGÉTIQUE composée à la
 * lecture (motif event_title : gabarits + noms RÉELS), buffer STABLE par slot —
 * l'hôte copie aussitôt (Godot String). Trié par ANNÉE CROISSANTE (la frise). */
typedef struct {
    int year;
    int kind;                 /* AnnalKind (scps_events.h) */
    const char *ligne;         /* la phrase composée (jamais NULL, "" au pire) */
    int region;                /* -1 = aucune (pays/monde) */
} ScpsAnnal;
/* Remplit `out` (jusqu'à `max` entrées), TRIÉES par année croissante. Renvoie le
 * nombre écrit. Read-only : ne mute jamais le moteur (pure lecture de l'anneau). */
int scps_annals(ScpsSim *s, ScpsAnnal *out, int max);

/* VOIE CONDITIONS : les alertes d'ÉTAT du joueur, calculées en UN appel (C scanne,
 * le front affiche) : révolte qui gronde · famine · siège ennemi sur mon sol ·
 * prix EXORBITANT au marché · bien de conso INTROUVABLE. -1 / "" = pas d'alerte. */
typedef struct {
    int revolt_region;  int revolt_agit;          /* pire agitation ≥ 60 sur MON sol */
    int famine_region;  int famine_pct;           /* pire food_sat < 60 % (colonisée) */
    int siege_region;   const char *siege_by;     /* siège ENNEMI en cours sur mon sol */
    int price_good;     int price_x10; const char *price_name;   /* prix ≥ 3× l'ancre (×10) */
    int conso_good;     const char *conso_name;   /* demandé, stock ET offre ~nuls */
} ScpsPlayerAlerts;
void scps_player_alerts(ScpsSim *s, ScpsPlayerAlerts *out);

/* ---- TRACÉS DE CARTE (overlays vectoriels du viewer) ----------------- *
 * RIVIÈRES : un point par cellule de fil (centre cellule) + l'ANGLE du fil (rad,
 * direction monde) — l'hôte y pose un sprite de rivière TOURNÉ, comme draw_map_rivers.
 * Figé par worldgen. Remplit out[0..min(n,max)-1], renvoie le nombre écrit. */
typedef struct { float x, y, ang; } ScpsRiverPt;
int scps_river_points(ScpsSim *s, ScpsRiverPt *out, int max);

/* RIVIÈRES STRUCTURÉES (par FLEUVE, ordonnées) — pour un rendu STRATÉGIQUE. La worldgen
 * sème des fleuves PARTOUT ; le rendu ne peut pas tous les montrer. L'hôte choisit donc
 * les fleuves MAJEURS (flow_max le plus fort) et les trace en FIL CONTINU au lieu d'un
 * nuage de points. `scps_river_count` = nombre de fleuves ; `scps_river_path` remplit le
 * i-ème (points ORDONNÉS, centres de cellule, *flow = débit max = le POIDS du fleuve). */
int scps_river_count(ScpsSim *s);
int scps_river_path(ScpsSim *s, int i, ScpsRiverPt *out, int max, float *flow);

/* FRONTIÈRES : segments d'arête (coins, unités-cellule) entre souverainetés. Niveau
 * `level` : 0 = PROVINCE · 1 = RÉGION · 2 = PAYS (le joint est classé à son niveau le
 * plus FORT : pays > région > province — jamais doublé). Port du balayage bseg de
 * viewer.c. Recalculé à la demande (souveraineté = colonisation/conquête/sécession).
 * Remplit out[0..min(n,max)-1] (un segment = (x0,y0)-(x1,y1)), renvoie le nombre écrit. */
typedef struct { float x0, y0, x1, y1; } ScpsSeg;
int scps_border_segments(ScpsSim *s, int level, ScpsSeg *out, int max);

/* Idem mais chaque segment porte l'OWNER (pays, côté terre) ET l'autre côté `other` — pour colorer
 * PAR EMPIRE et savoir si la frontière touche une AUTRE entité (hachures). `other` >= 0 = un autre
 * pays (frontière INTER-EMPIRE → à hachurer) · -1 = terre libre (marche) · -2 = MER/lac.
 * ⚠ Niveau 2 : les joints qui touchent la MER (côte) NE SONT PAS émis (le rivage suffit). */
typedef struct { float x0, y0, x1, y1; int owner, other; float nx, ny; } ScpsSegC;  /* nx,ny = normale unité vers l'EXTÉRIEur */
int scps_border_segments_col(ScpsSim *s, int level, ScpsSegC *out, int max);

/* éthos DOMINANT d'un pays (capitale, sinon 1re région) → 0..5 (DOMINATEUR..PACIFISTE), -1 si indéfini.
 * Pour colorer la frontière sur l'axe ordre↔chaos (la membrane mappe l'index → teinte). */
int scps_country_ethos(const ScpsSim *s, int c);
/* HÉRITAGE (culture de base) d'un pays → 0..HERITAGE_COUNT-1, -1 si indéfini. Outline par CULTURE. */
int scps_country_heritage(const ScpsSim *s, int c);
/* RÉGION-capitale d'un pays (-1) + CONTOUR d'une région (normale extérieure) — liseré pourpre capitale. */
int scps_country_capital_region(const ScpsSim *s, int c);
int scps_region_border_segments(ScpsSim *s, int region, ScpsSegC *out, int max);
/* CONTOUR d'une PROVINCE (même forme) — la SURBRILLANCE DE SÉLECTION (le grain de panneau). */
int scps_province_border_segments(ScpsSim *s, int prov, ScpsSegC *out, int max);
/* LAVIS POLITIQUE — l'owner EFFECTIF par CELLULE (pays d'une région colonisée, -1 sinon :
 * mer, terre vierge). `out` = SCPS_W×SCPS_H int16. Le front en teinte un wash de territoire. */
void scps_map_owner(ScpsSim *s, int16_t *out);

/* BROUILLARD DE GUERRE (étape 1/2 — le VOILE visuel du joueur ; aucune décision de
 * simulation n'en dépend ici, cf. scps_fog.h). Régions visibles pour human_player
 * MAINTENANT : {ses régions} ∪ {BFS radius 2} ∪ {tout empire CONNU, cumulatif}.
 * human_player<0 (chronique/viewer sans joueur humain) ⇒ tout visible.
 * `scps_fog_visible` : masque par CELLULE (out ≥ scps_map_w()×scps_map_h() octets,
 * motif scps_map_owner) — 1=visible, 0=voilé ; pour un lavis d'encre estompé sur
 * la carte.
 * `scps_fog_region_mask` : même connaissance par RÉGION (out ≥ scps_region_count(s)
 * octets — PAS un cap moteur interne, motif region_owner/region_tier ; calculé UNE
 * fois, consommé en boucle) — pour griser/cacher les acteurs overlay (villes/
 * armées/noms) sans re-sonder l'image cellule par cellule. */
void scps_fog_visible(ScpsSim *s, uint8_t *out);
void scps_fog_region_mask(ScpsSim *s, uint8_t *out);

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

/* ====================================================================== */
/* LECTURES DE FENÊTRES (read-only) : arbre de tech · budget · missions     */
/* La membrane : des MOTS résolus + des nombres TANGIBLES (or, points,      */
/* projections 0-100), jamais un flottant moteur. Toutes const, golden-safe.*/
/* ====================================================================== */

/* ---- ARBRE DE TECHNOLOGIE (tech_tree_readout) ------------------------- *
 * L'arbre concentrique du JOUEUR : un nœud = un angle (quarter 0-8 = thème×3
 * + fonction) × un rayon (tier 0-5), un état, un coût en POINTS de recherche,
 * l'effet concret. scps_tech_nodes remplit la grille ; scps_tech_info donne le
 * total de points, les libellés thèmes/fonctions et la BANDE de risque faustien
 * (jamais le flottant brut : présage + projection 0-100). */
typedef struct {
    int  quarter;   /* 0-8 : thème×3 + fonction (l'angle) */
    int  tier;      /* 0-5 : le rayon (profondeur) */
    int  state;     /* 0 verrouillé · 1 ouvert (recherchable) · 2 acquis */
    int  faustian;  /* 1 = bout interdit (charge la Brèche) */
    int  orphan;    /* 1 = signature d'une AUTRE lignée, accès manquant */
    int  is_base;   /* 1 = bâtiment de base (tier 0) */
    const char *name;     /* nom du nœud */
    const char *unlocks;  /* ce qu'il déverrouille */
    const char *effet;    /* l'utilité concrète */
    int  cost;      /* points de recherche (0 pour une base) */
    int  prereq;    /* INDICE du nœud prérequis dans CE tableau (-1 = aucun : une base) — pour tracer les arêtes */
    /* PACK FLAVOR (display-only, 2026-07-05) : le survol Medusa (UI Godot) affiche ces deux
     * lignes sous le nom — hover = l'effet mécanique réel (tech_hover), flavor = le mot
     * cynique du conseiller (tech_flavor). "" si absent — jamais NULL côté appelant. */
    const char *hover;
    const char *flavor;
} ScpsTechNode;
int scps_tech_nodes(ScpsSim *s, ScpsTechNode *out, int max);

typedef struct {
    int  points;            /* points de recherche DISPONIBLES (tangible) */
    const char *theme[3];   /* libellés de thèmes (ordre THM_*) */
    const char *function[3];/* libellés de fonctions (ordre FN_*) */
    const char *presage;    /* bande de risque faustien (présage de la Brèche) */
    int  crise_pct;         /* 0-100 : proximité de crise (projection tangible) */
    int  metab_pct;         /* MÉTABOLISATION : +X% de recherche (le creuset digéré) — pour le hover */
} ScpsTechInfo;

/* ACCÈS D'HÉRITAGE (barre de métabolisation) — par héritage, le TIER d'accès tech atteint
 * (0..3 : commerce/gouvernance/métabolisation) + la part d'âmes diaspora DIGÉRÉES. Lu par
 * l'UI de l'arbre (Medusa) : la « barre de progression » qui ouvre les signatures par tier. */
typedef struct {
    const char *nom;       /* nom de l'héritage */
    int tier;              /* accès recherchable atteint : 0..3 */
    int digested_pct;      /* % d'âmes diaspora de cet héritage DIGÉRÉES (la métabolisation) */
    int native;            /* 1 = héritage natif de l'empire (toujours tier 3) */
} ScpsHeritageAccess;
int scps_player_heritage_access(ScpsSim *s, ScpsHeritageAccess *out, int max);  /* retourne HERITAGE_COUNT */

/* MÉTABOLISATION POUR LA VICTOIRE (Merveille §27) — DISTINCT de ScpsHeritageAccess
 * ci-dessus : celle-ci lit l'accès TECH (pop-share, tier 0..3) ; celle-ci lit ce que
 * `endgame_metab_count` compte réellement pour faire progresser un palier de la
 * Merveille (natif toujours · gouvernance = contact profond · diaspora = ratio
 * individualisé ≥60% + 500 âmes). Un héritage peut être "prêt" côté tech SANS
 * compter ici, et l'inverse — ne pas fusionner les deux lectures côté UI. */
typedef struct {
    const char *nom;
    int  metabolized;     /* 1 = compte pour la Merveille (gate du palier courant) */
    const char *voie;     /* "natif" | "gouvernance" | "diaspora" | "" (aucune) */
    int  progress_pct;    /* 0..100 : progression de la meilleure voie */
    int  native;          /* 1 = héritage natif de l'empire */
} ScpsMervHeritage;
/* out : HERITAGE_COUNT entrées (une par héritage). count/required : le compte
 * total X/6 actuel et le requis du palier COURANT de la Merveille (0 si aucun
 * palier actif — rien à gater). Retourne HERITAGE_COUNT (0 si sim non prête). */
int scps_merv_metab(ScpsSim *s, ScpsMervHeritage *out, int max, int *count, int *required);

/* MODTOOLS — registre des TUNABLES (panneau dev : lister + éditer en direct). GLOBAL (pas
 * par-sim). scps_tune_set_val applique la surcharge LIVE (effet là où tune_f est relu au tick). */
typedef struct {
    const char *nom;
    double      value;       /* valeur active (surcharge ou défaut) */
    double      def_value;   /* défaut compilé */
    int         overridden;  /* 1 si surchargé (env ou panneau) */
} ScpsTunable;
int  scps_tune_count(void);
void scps_tune_at(int i, ScpsTunable *out);
void scps_tune_set_val(const char *nom, double value);

/* ---- LANGUE (i18n moteur) --------------------------------------------- *
 * Bascule la TABLE COMPILÉE que tr() résout (scps_lang.h : FR de référence /
 * EN jumelle) — à CHAUD, GLOBAL (pas par-sim), display-only : les readouts
 * traversants (province_info, bandes, conseils…) rendent la langue courante
 * au PROCHAIN appel — aucune chaîne n'est cachée dans un état persistant.
 * La surcharge fichier scps_lang.txt reste AU-DESSUS (tr() la lit d'abord). */
void scps_lang_set(int lang);   /* 0 = FR · 1 = EN (autre valeur : ignorée) */
int  scps_lang_get(void);       /* la langue active (0/1) */
void scps_tech_info(ScpsSim *s, ScpsTechInfo *out);

/* ---- BUDGET / FISCAL (econ_flux_get × FluxComp + crédit) -------------- *
 * La décomposition du flux d'or de l'ANNÉE COURANTE (la façade RAZ le flux à
 * chaque passage d'année dans advance_days) : une ligne par poste non vide,
 * signe = revenu (+) ou dépense (−). Plus le trésor, la ligne de crédit
 * (capacité d'emprunt ∝ pop) et le pays prêteur s'il y a dette. */
typedef struct {
    const char *name;   /* libellé du poste (econ_flux_name) */
    double amount;      /* or de l'année (signé : + revenu, − dépense) */
} ScpsFluxLine;
int scps_country_budget(ScpsSim *s, int cid, ScpsFluxLine *out, int max);

typedef struct {
    double gold;            /* trésor courant */
    double income;          /* Σ des postes positifs (année) */
    double expense;         /* Σ des dépenses, en valeur absolue (année) */
    double net;             /* income − expense */
    double credit_line;     /* capacité d'emprunt (dette max) */
    int    creditor;        /* pays prêteur (-1 = aucune dette / aucun prêteur) */
    const char *creditor_name;
} ScpsBudget;
void scps_budget_summary(ScpsSim *s, int cid, ScpsBudget *out);

/* ---- MISSION DÉCENNALE (mission_of) ----------------------------------- *
 * L'objectif courant du pays (au plus un actif) : texte (membrane moteur),
 * récompense (or + matière), année d'émission. La progression n'est pas
 * stockée (le moteur la re-dérive) → on surface l'objectif et sa prime. */
typedef struct {
    int    active;          /* 0/1 : une mission est-elle en cours ? */
    const char *text;       /* texte de la mission (FR, membrane moteur) */
    double reward_gold;     /* prime en or à l'accomplissement */
    const char *reward_mat; /* matière de prime (resource_name ; vide si reward_qty=0) */
    double reward_qty;      /* quantité de la matière */
    int    issued_year;     /* année d'émission */
    int    done;            /* 1 = accomplie (récompense versée ce tour) */
} ScpsMission;
void scps_mission_info(ScpsSim *s, int cid, ScpsMission *out);

/* ---- FACTIONS (le spectre d'éthos interne — §9 UI) --------------------- *
 * La distribution EFFECTIVE (groupes + stance des leviers), la rancœur par
 * faction, la dominante ; + la tension de COUP et la CORRUPTION (capture). */
typedef struct {
    const char *name;    /* mot résolu (faction_name — membrane) */
    int  part;           /* 0-100 : part du spectre effectif */
    int  grief;          /* 0-100 : rancœur (couve un coup) */
    int  dominant;       /* 0/1 : donne la direction (l'éthos effectif) */
} ScpsFaction;
int scps_country_factions(ScpsSim *s, int cid, ScpsFaction *out, int max,
                          int *coup, int *corruption);

/* ====================================================================== */
/* CRÉATEUR DE CULTURE (façon Stellaris) — le JOUEUR compose son empire :   */
/* un HÉRITAGE (la lignée → les NOMS) · un ÉTHOS (les valeurs → le nom du    */
/* pays + les factions) · 3 TRADITIONS (une par AXE, EXACTEMENT 1 majeur +   */
/* 1 mineur + 1 défaut → les leviers du moteur pour SON empire). La membrane  */
/* traverse : des MOTS et des SIGNES, jamais un flottant de levier. Les       */
/* listes + la validation + l'aperçu sont PURS (aucun sim requis) → utilisables*/
/* AVANT scps_sim_generate ; la composition est appliquée À la génération.    */
/* ====================================================================== */

/* HÉRITAGE (lignée culturelle) : id + nom + sphère + un ethnonyme-exemple + une
 * phrase d'ambiance (`flavor`, champ ajouté en FIN de struct — display-only,
 * docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md §HÉRITAGES). */
typedef struct { int id; const char *nom; const char *sphere; const char *exemple; const char *flavor; } ScpsHeritage;
int scps_heritage_list(ScpsHeritage *out, int max);     /* retourne HERITAGE_COUNT */

/* ÉTHOS (axe de valeurs) : id + nom + épithète de pays (« Horde »…) + une ligne +
 * une phrase d'ambiance (`flavor`, champ ajouté en FIN de struct — display-only,
 * docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md §ÉTHOS). */
typedef struct { int id; const char *nom; const char *epithete; const char *hint; const char *flavor; } ScpsEthosDef;
int scps_ethos_list(ScpsEthosDef *out, int max);        /* retourne ETHOS_COUNT */

/* TRADITION (= un trait) : id + nom + axe (0 Physique·1 Social·2 Intellectuel) +
 * nom d'axe + rang (+2 majeur · +1 mineur · −1 défaut) + définition (survol). */
typedef struct { int id; const char *nom; int axe; const char *axe_nom; int rang; const char *hover; } ScpsTradition;
int scps_tradition_list(ScpsTradition *out, int max);   /* retourne TRAIT_COUNT (36) */

/* VALIDE une composition : t0∈Physique, t1∈Social, t2∈Intellectuel, et le trio porte
 * EXACTEMENT 1 majeur + 1 mineur + 1 défaut, sans antonymes. 1 = valide, 0 = non. */
int scps_culture_validate(int t0, int t1, int t2);

/* APERÇU des leviers d'une composition (membrane : un MOT + un SIGNE + la MAGNITUDE),
 * pour l'aperçu chiffré. signe : +1 atout · −1 revers (les zéros sont omis). `value` =
 * le delta brut ; `is_pct` = 1 si RELATIF (s'applique en 1+x → l'UI l'affiche « +15 % »),
 * 0 si ABSOLU (additif sur l'échelle 0..10 → l'UI l'affiche « +1.5 »). Retourne le nb de lignes. */
typedef struct { const char *nom; int signe; float value; int is_pct; } ScpsLevierLine;
int scps_culture_preview(int t0, int t1, int t2, ScpsLevierLine *out, int max);

/* NOM DE CULTURE (ethnonyme façon Stellaris) pour un héritage + une graine — pour
 * l'aperçu live du créateur (aucun monde requis). Pointe un buffer statique. */
const char *scps_culture_name(int heritage, uint32_t seed);

/* COMPOSE la culture d'un EMPIRE par SLOT (façon Stellaris) : slot 0 = JOUEUR, 1..N-1 =
 * empires IA dans l'ordre de génération. À appeler AVANT scps_sim_generate (la compo est
 * gravée à la genèse : héritage→noms/couleur, éthos→nom/factions, traditions→les leviers
 * de cet empire). Retourne 1 si VALIDE et retenue, 0 sinon (rien). */
int  scps_set_empire_culture(int slot, int heritage, int ethos, int t0, int t1, int t2);
/* Raccourci slot 0 (le joueur). */
int  scps_set_player_culture(int heritage, int ethos, int t0, int t1, int t2);
/* EFFACE TOUTES les compositions (retour au tirage IA + héritage ADAPTATIF + éthos émergent). */
void scps_clear_player_culture(void);
/* NOM PERSONNALISÉ : le joueur (re)nomme un État — champ d'affichage (jamais hashé),
 * sérialisé avec le pays (survit aux saves). Vide/hors-borne = no-op. */
void scps_set_country_name(ScpsSim *s, int cid, const char *name);

/* ====================================================================== */
/* RELIGION (façade) — fonder · schismer · lire. Le moteur tient le registre */
/* (g_religions) + les liens pays/région ; l'hôte FONDE et SCHISME, lit des  */
/* mots via relig_*_name côté binding. credo ∈ Credo · t0/t1/t2 ∈ ReligPole. */
/* ====================================================================== */
/* FONDE une religion pour `cid` (3 traditions une-par-axe) : centre = capitale du pays ;
 * le pays + ses régions en héritent. Retour : id de la religion (-1 si invalide). */
int scps_religion_found(ScpsSim *s, int cid, int credo, int t0, int t1, int t2);
/* éligibilité au schisme : 0 aucune · 1 RUPTURE · 2 DERIVE. */
int scps_religion_eligible(ScpsSim *s, int cid);
/* SCHISME interne : crée l'enfant (repick 2 slots du parent) + FRACTURE les régions
 * distantes/peu légitimes vers l'enfant. *out_flipped ← nb régions basculées. Retour :
 * id enfant (-1 si invalide). */
int scps_religion_schism(ScpsSim *s, int cid, int slot_a, int pole_a, int slot_b, int pole_b,
                         int new_credo, int *out_flipped);
int scps_religion_of_country(ScpsSim *s, int cid);
int scps_religion_of_region (ScpsSim *s, int region);
/* recrute un LETTRÉ pour `cid` sur `region` (rôle dérivé du crédo : Missionnaire/Gourou/
 * Moine) ; retourne le ScholarRole (>=0) ou -1 si le pays n'a pas de foi. */
int scps_religion_recruit_scholar(ScpsSim *s, int cid, int region);
int scps_religion_scholar_role(ScpsSim *s, int cid);   /* ScholarRole courant / -1 */
/* lot M — le LETTRÉ jouable : le rôle qu'un recrutement DONNERAIT maintenant (résolu
 * du crédo de la foi d'État), -1 si pays sans foi — distinct du rôle ACTIF ci-dessus.
 * scps_scholar_role_name : le mot résolu (Missionnaire/Gourou/Moine), "" hors-borne. */
int scps_religion_scholar_expected(ScpsSim *s, int cid);
const char *scps_scholar_role_name(int role);      /* Missionnaire/Gourou/Moine, "?" hors-borne */
const char *scps_scholar_role_ability(int role);   /* Conversion/Résistance/Stabilisation */

/* LISTES pour l'UI religion (membrane : mots résolus). */
typedef struct { int id; const char *nom; int axe; const char *axe_nom; const char *tip; } ScpsReligPole;
int scps_religion_pole_list(ScpsReligPole *out, int max);   /* RP_COUNT (16) */
typedef struct { int id; const char *nom; } ScpsCredoDef;
int scps_credo_list(ScpsCredoDef *out, int max);            /* CREDO_COUNT (3) */
/* VALIDE 3 pôles (axes distincts) — pour griser l'UI. */
int scps_religion_picks_valid(int p0, int p1, int p2);
/* nom de la religion d'un pays = « <crédo> · <pôle0>/<pôle1>/<pôle2> » (buffer statique). */
const char *scps_religion_name(ScpsSim *s, int cid);
/* DÉCLENCHEUR « créateur de foi » : 1 si `cid` a bâti un ÉDIFICE RELIGIEUX (sanctuaire/
 * temple/cathédrale/monastère) ET n'a PAS encore de foi. Avant : le monde est ATHÉE. */
int scps_religion_founding_ready(ScpsSim *s, int cid);
/* PLAFOND mondial de religions FONDÉES = ⌈n_empires/3⌉. can_found=0 ⇒ on RALLIE une foi. */
int scps_religion_cap(ScpsSim *s);
int scps_religion_can_found(ScpsSim *s);

/* ====================================================================== */
/* PARAMÈTRES DE GÉNÉRATION (l'écran « Nouvelle partie ») — les sliders.    */
/* Ce sont les champs RÉELS de WorldParams que le moteur consomme (taille,  */
/* âge, climat, relief). POD membrane (ints/floats), aucun type moteur.      */
/* ====================================================================== */
typedef struct {
    int   n_empires;       /* empires visés (taille : tiny 2 … huge 12) */
    int   n_city_states;   /* cités-états visées */
    int   n_continents;    /* 1..8 masses continentales */
    float world_age;       /* 0..1 — vieux = relief usé */
    float land_amount;     /* 0..1 — 0.5 neutre ; haut = plus de terres */
    float mountains;       /* 0..1 — amplitude du relief */
    float erosion;         /* 0..1 — creusement hydraulique */
    float temperature;     /* 0..1 — 0.5 neutre ; haut = chaud */
    float humidity;        /* 0..1 — 0.5 neutre ; haut = humide */
} ScpsWorldParams;

/* Les DÉFAUTS « monde standard » pour une graine (pour pré-remplir les sliders). */
void scps_worldparams_default(uint32_t seed, ScpsWorldParams *out);
/* OVERRIDE des paramètres pour la PROCHAINE scps_sim_generate (clampé en interne).
 * Reste actif jusqu'à scps_worldgen_clear (ou la prochaine partie). */
void scps_worldgen_set(const ScpsWorldParams *p);
void scps_worldgen_clear(void);

/* ====================================================================== */
/* SAUVEGARDE (l'écran « Charger ») — format PARTAGÉ avec le viewer (scps_save).*/
/* 3 emplacements (1..3). La section CULT persiste les cultures composées.   */
/* ====================================================================== */
int  scps_sim_save(ScpsSim *s, int slot);   /* 1 = écrit · 0 = échec */
int  scps_sim_load(ScpsSim *s, int slot);   /* 0 ok · 1 absent/corrompu · 2 « ère antérieure » */
/* infos des slots (pour la liste « Charger ») : used + année + ligne résumée. */
typedef struct { int used; int year; char line[96]; } ScpsSaveSlot;
void scps_save_slots(ScpsSaveSlot *out, int max);   /* remplit out[0..max) = slots 1..max */

#ifdef __cplusplus
}
#endif
#endif /* SCPS_API_H */
