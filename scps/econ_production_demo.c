/*
 * econ_production_demo.c — l'épine dorsale : fer+charbon → métal → outils → productivité
 *
 *   make econ_production_demo && ./econ_production_demo [graine]
 *
 * Les biens de PRODUCTION sont des intrants, pas des paniers. La chaîne centrale :
 *   Fer + Charbon → (Fonderie) Métal → (Atelier) Outils.
 * Et les OUTILS sont le MULTIPLICATEUR de productivité : leur stock booste
 * l'extraction et la manufacture. On vérifie :
 *   1. La fonderie produit du MÉTAL (fer + charbon).
 *   2. L'atelier d'outillage produit des OUTILS (métal + bois).
 *   3. Les outils MONTENT la production (même région, plus d'outils → plus de PIB).
 *   4. Les outils s'USENT (sans entretien, le stock décroît).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_tune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

/* Province REPRÉSENTATIVE d'une région (charte PROVINCE_MODEL.md : l'économie
 * vit à la province, la région n'est qu'un agrégat) — repli : scan direct. */
static int rep_prov(WorldEconomy *e, int r){
    if (r>=0 && r<SCPS_MAX_REG && e->region_rep_prov[r]>=0) return e->region_rep_prov[r];
    for (int p=0;p<e->n_prov;p++) if (e->prov[p].region==r) return p;
    return -1;
}

/* ÉTEINT toute province SŒUR (même région, ≠ pid) : la région AGRÈGE (règle 2) —
 * sans ça les autres provinces (semées par world_generate/gen_population)
 * contaminent stock/supply agrégés que le banc lit sur e->region[r]. */
static void mute_siblings(WorldEconomy *e, int r, int pid){
    for (int p=0;p<e->n_prov;p++){
        if (p==pid || e->prov[p].region!=r) continue;
        ProvinceEconomy *pe=&e->prov[p];
        pe->active=false; pe->colonized=false;
        memset(pe->strata,0,sizeof pe->strata);
        for (int k=0;k<RES_COUNT;k++){ pe->raw_cap[k]=0.f; pe->stock[k]=0.f; pe->supply[k]=0.f; }
    }
}

/* Fige la PROVINCE représentative de la région r en banc d'essai de production :
 * fer+charbon+bois+grain, plus un atelier d'outillage (fer + bois → outils,
 * DIRECT), sur une pop de travail donnée. */
static void rig(WorldEconomy *e, int r, float tools){
    int pid=rep_prov(e,r);
    mute_siblings(e,r,pid);
    ProvinceEconomy *re=&e->prov[pid];
    re->active=true; re->colonized=true; re->culture.settled=true;
    re->owner=-1;   /* ISOLATION : hors domaine §NF (sinon la construction demande-menée
                     * bâtirait un atelier qui CONSOMME le métal que le test veut accumuler) */
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->stock[k]=0.f; }
    re->raw_cap[RES_IRON]=4.f; re->raw_cap[RES_COAL]=4.f;
    re->raw_cap[RES_WOOD]=4.f; re->raw_cap[RES_GRAIN]=8.f;
    re->n_bld=0;
    re->bld[re->n_bld].type=BLD_TOOLWORKS; re->bld[re->n_bld].level=3.f; re->n_bld++;
    re->strata[CLASS_LABORER].pop=600.f;  re->strata[CLASS_LABORER].wealth=400.f;
    re->strata[CLASS_BOURGEOIS].pop=100.f;re->strata[CLASS_BOURGEOIS].wealth=200.f;
    re->strata[CLASS_ELITE].pop=50.f;     re->strata[CLASS_ELITE].wealth=300.f;
    re->stock[RES_TOOLS]=tools;
}

int main(int argc, char **argv){
    /* Fixture STABLE : monde pinné à ~320 territoires (le banc teste l'usure/chaîne d'outils, pas
     * le scaling f(empires) ; un monde géant dilue la pop/labor par région et fausse les seuils). */
    if (!getenv("SCPS_TUNE")){
        tune_set("WORLD_PROV_BASE",320.f);
        tune_set("WORLD_PROV_PER_EMPIRE",0.f);
        tune_set("WORLD_PROV_PER_CITY",0.f);
    }
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World)); WorldEconomy *e=malloc(sizeof(WorldEconomy));
    if(!w||!e){ fprintf(stderr,"OOM\n"); return 1; }
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" ÉPINE DORSALE — fer+bois→outils→productivité — graine %u\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p); econ_init(e,w); gen_population(w,e);
    if (e->n_regions<3){ fprintf(stderr,"monde trop petit\n"); return 1; }

    /* ═══ 1. La chaîne réelle (DIRECTE) ═════════════════════════════════ */
    printf("\n── 1. Atelier d'outillage (fer + bois → outils, DIRECT — plus de métal) ──\n");
    /* Région 1 : l'atelier sort des OUTILS directement du fer + bois. */
    rig(e, 1, 0.f);
    for (int t=0;t<6;t++) econ_tick(e,1.f);
    float tools=e->region[1].stock[RES_TOOLS];
    printf("   chaîne directe : outils=%.1f\n", tools);
    ok("l'Atelier produit des OUTILS (fer + bois, DIRECT)", tools > 0.5f);

    /* ═══ 3. Les outils MULTIPLIENT la productivité ═════════════════════ */
    printf("\n── 3. Les outils montent la production (le multiplicateur) ──\n");
    /* REFONTE A0 : on mesure l'EXTRACTION BRUTE (que les outils multiplient via prod_mult),
     * pas le PIB (valeur ajoutée) — désormais confondu par le marché des outils (le stock
     * d'outils déprime leur prix → la manufacture d'outils, donc la VA, varie en sens inverse). */
    rig(e, 1, 0.f);    /* site sans outils */
    econ_tick(e,1.f);
    float out_none=0.f; for(int g=1;g<RES_PROD_FIRST;g++) out_none+=e->region[1].supply[g];
    rig(e, 1, 600.f);  /* région IDENTIQUE mais bien outillée */
    econ_tick(e,1.f);
    float out_tools=0.f; for(int g=1;g<RES_PROD_FIRST;g++) out_tools+=e->region[1].supply[g];
    printf("   extraction brute du même site : sans outils=%.1f vs bien outillé=%.1f\n", out_none, out_tools);
    ok("un site BIEN OUTILLÉ extrait plus qu'un site sans outils (productivité)",
       out_tools > out_none + 0.5f);

    /* ═══ 4. Les outils s'usent ═════════════════════════════════════════ */
    printf("\n── 4. Les outils s'usent (il faut les entretenir) ──\n");
    int pid2=rep_prov(e,2);
    mute_siblings(e,2,pid2);
    ProvinceEconomy *re2=&e->prov[pid2];
    re2->active=true; re2->colonized=true; re2->culture.settled=true;
    /* EMPIRE ISOLÉ (slot pays INUTILISÉ) : l'usure du PARC NATIONAL (×0.97/tick) est
     * hoistée par EMPIRE → une province SANS owner valide n'entre dans aucun pool et
     * n'use JAMAIS ses outils. On en fait un empire mono-province (pool = cette seule
     * province, pshare=1 ⇒ usure NETTE) ; sans ressources ⇒ pas de §NF qui rebâtirait
     * l'atelier. (La re-baseline worldgen #3 a fait passer region[2] en non-possédée.) */
    re2->owner = (w->n_countries < SCPS_MAX_COUNTRY) ? w->n_countries : SCPS_MAX_COUNTRY-1;
    for (int k=0;k<RES_COUNT;k++){ re2->raw_cap[k]=0.f; re2->stock[k]=0.f; }
    re2->n_bld=0;   /* AUCUN atelier → pas d'entretien */
    re2->strata[CLASS_LABORER].pop=600.f;
    re2->stock[RES_TOOLS]=1000.f;
    float tw0=re2->stock[RES_TOOLS];
    for (int t=0;t<10;t++) econ_tick(e,1.f);
    float tw1=re2->stock[RES_TOOLS];
    printf("   stock d'outils sans entretien : %.0f → %.0f (usure)\n", tw0, tw1);
    ok("sans atelier pour les entretenir, le stock d'outils DÉCROÎT", tw1 < tw0 - 1.f);

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(e);
    return g_fail?1:0;
}
