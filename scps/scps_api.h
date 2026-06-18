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

/* ---- COUCHES brutes (1 octet/cellule) pour shaders côté hôte ---------- */
enum { SCPS_LAYER_HEIGHT = 0, SCPS_LAYER_SEA, SCPS_LAYER_BIOME, SCPS_LAYER_COAST };
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

#ifdef __cplusplus
}
#endif
#endif /* SCPS_API_H */
