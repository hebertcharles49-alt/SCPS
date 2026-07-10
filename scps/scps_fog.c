/*
 * scps_fog.c — voir scps_fog.h. État de module GLOBAL (motif scps_decrees.c/
 * scps_intertrade.c) : known[a][b] = 1 ⇔ l'empire a connaît l'empire b.
 */
#include "scps_fog.h"
#include <string.h>

static uint8_t g_known[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];

void fog_reset(void){
    memset(g_known, 0, sizeof g_known);
}

void fog_update(const World *w, const WorldEconomy *econ){
    if (!w || !econ) return;
    int n = econ->n_regions; if (n > SCPS_MAX_REG) n = SCPS_MAX_REG;
    int nc = w->n_countries; if (nc > SCPS_MAX_COUNTRY) nc = SCPS_MAX_COUNTRY;

    /* scratch RÉUTILISÉ à chaque empire (motif hub_map_build : static, pas de
     * malloc/free répété — RAZ ciblée [0,n) à chaque tour de boucle). */
    static uint8_t  visited[SCPS_MAX_REG];
    static uint8_t  depth[SCPS_MAX_REG];
    static int16_t  q[SCPS_MAX_REG];

    for (int a=0; a<nc; a++){
        g_known[a][a] = 1;   /* on se connaît toujours soi-même (même sans région : slot mort inoffensif) */
        int qh=0, qt=0;
        memset(visited, 0, (size_t)n);
        for (int r=0; r<n; r++){
            if (econ->region[r].owner==a){
                visited[r]=1; depth[r]=0; q[qt++]=(int16_t)r;
            }
        }
        while (qh<qt){
            int r=q[qh++];
            int b=econ->region[r].owner;
            if (b>=0 && b<SCPS_MAX_COUNTRY) g_known[a][b]=1;   /* découverte SUR LE TERRAIN, cumulative */
            if (depth[r] >= FOG_RADIUS) continue;               /* radius 2 : n'étend plus au-delà */
            for (int s=0; s<n; s++){
                if (!econ->adj[r][s] || visited[s]) continue;
                visited[s]=1; depth[s]=(uint8_t)(depth[r]+1);
                q[qt++]=(int16_t)s;
            }
        }
    }
}

bool country_knows(int a, int b){
    if (a<0 || a>=SCPS_MAX_COUNTRY || b<0 || b>=SCPS_MAX_COUNTRY) return false;
    return g_known[a][b] != 0;
}

void fog_visible_regions(const World *w, const WorldEconomy *econ, int viewer_cid, uint8_t *out_region){
    if (!out_region) return;
    if (!w || !econ || viewer_cid<0 || viewer_cid>=SCPS_MAX_COUNTRY){
        memset(out_region, 1, (size_t)SCPS_MAX_REG);   /* pas de joueur engagé : aucun voile */
        return;
    }
    memset(out_region, 0, (size_t)SCPS_MAX_REG);
    int n = econ->n_regions; if (n > SCPS_MAX_REG) n = SCPS_MAX_REG;

    /* BFS radius-2 depuis les régions du viewer — out_region sert AUSSI de
     * marqueur "atteint" (évite un second tableau : {ses régions} ∪ {radius 2}
     * sont exactement l'ensemble atteint par CETTE BFS). */
    static uint8_t depth[SCPS_MAX_REG];
    static int16_t q[SCPS_MAX_REG];
    int qh=0, qt=0;
    for (int r=0; r<n; r++){
        if (econ->region[r].owner==viewer_cid){
            out_region[r]=1; depth[r]=0; q[qt++]=(int16_t)r;
        }
    }
    while (qh<qt){
        int r=q[qh++];
        if (depth[r] >= FOG_RADIUS) continue;
        for (int s=0; s<n; s++){
            if (!econ->adj[r][s] || out_region[s]) continue;
            out_region[s]=1; depth[s]=(uint8_t)(depth[r]+1);
            q[qt++]=(int16_t)s;
        }
    }
    /* ∪ toute région dont l'owner est CONNU (découverte cumulative passée) —
     * l'empire DÉCOUVERT tout entier, pas seulement sa frange radius-2 actuelle. */
    for (int r=0; r<n; r++){
        if (out_region[r]) continue;
        int o = econ->region[r].owner;
        if (o>=0 && country_knows(viewer_cid, o)) out_region[r]=1;
    }
}

/* BANC/FUZZ SEULEMENT — cf. scps_fog.h. */
void fog_debug_meet_all(int a){
    if (a<0 || a>=SCPS_MAX_COUNTRY) return;
    for (int b=0; b<SCPS_MAX_COUNTRY; b++) g_known[a][b]=1;
}

/* ---- Sérialisation (section FOGV, motif WILD/DCRE : sim_wild_save/decrees_save) ---- */
void fog_save(FILE *f){
    fwrite(g_known, sizeof g_known, 1, f);
}
bool fog_load(FILE *f){
    fog_reset();   /* défensif : un fread partiel ne laisse jamais une ligne d'un AUTRE monde */
    return fread(g_known, sizeof g_known, 1, f) == 1;
}
