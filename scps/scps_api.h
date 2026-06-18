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
 * ── Fidélité du SPIKE ──────────────────────────────────────────────────────
 * `scps_sim_advance_days` roule pour l'instant la COLONNE économique (la même
 * boucle que le banc audit_eco : agency/labor/econ/world/colonize/migrate) sur
 * un monde RÉEL → le monde naît, peuple, colonise, se développe. Le tick PLEIN
 * (IA, guerre, diplo, intertrade, prospérité, endgame — fidèle au HASH de
 * chronicle) viendra de l'extraction de chronicle::sim_day vers un module
 * partagé `scps_sim` ; la surface de cette façade NE CHANGERA PAS (seul le corps
 * de advance grossira). C'est l'intérêt de fixer l'API maintenant.
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
 * mode = ViewMode (0 = VIEW_TERRAIN). Le moteur peint, l'hôte uploade en texture. */
void scps_map_rgba(ScpsSim *s, uint8_t *dst, int mode);

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

#ifdef __cplusplus
}
#endif
#endif /* SCPS_API_H */
