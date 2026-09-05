/* api_cache_demo.c — banc isolé des allocations transitoires de la façade.
 *
 * Ce fichier inclut scps_api.c exprès : ses wrappers d'allocation ne sont pas
 * la production et permettent de faire échouer chaque malloc/calloc demandé
 * par les centroïdes ou l'A*. Le target Makefile doit donc omettre l'objet
 * scps_api.o pour ce banc.
 */
#include "scps_api.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long fail_at;
static long alloc_count;

static void *demo_malloc(size_t n){
    if (fail_at>0 && ++alloc_count==fail_at) return NULL;
    return malloc(n);
}
static void *demo_calloc(size_t n, size_t sz){
    if (fail_at>0 && ++alloc_count==fail_at) return NULL;
    return calloc(n,sz);
}

#define malloc demo_malloc
#define calloc demo_calloc
#include "scps_api.c"
#undef malloc
#undef calloc

static int pass_count, fail_count;
static void check(const char *label, int ok){
    printf("   %s %s\n",ok?"OK":"FAIL",label);
    if(ok) pass_count++; else fail_count++;
}
static void inject(long nth){ fail_at=nth; alloc_count=0; }

static int astar_buffers_empty(void){
    return !g_ag && !g_afrom && !g_agen && !g_aclosed && !g_aheapf && !g_aheapi;
}
static int astar_buffers_complete(void){
    return g_ag && g_afrom && g_agen && g_aclosed && g_aheapf && g_aheapi;
}

int main(void){
    ScpsSim *s=scps_sim_new();
    check("façade allocated",s!=NULL);
    if(!s) return 1;
    fail_at=0; alloc_count=0;
    scps_sim_generate(s,9);
    check("baseline centroids available",s->n_cent>0 && s->n_pcent>0);

    /* api_centroids = 4 malloc + 6 calloc. Chaque panne doit rester propre,
     * puis un appel sain doit reconstruire les deux familles intégralement. */
    for(long n=1;n<=10;n++){
        float *old_cx=s->cx, *old_cy=s->cy, *old_px=s->ppx, *old_py=s->ppy;
        inject(n);
        api_centroids(s);
        check("centroid failure reaches its injected allocation",
              alloc_count>=n);
        check("centroid failure publishes no partial cache",
              s->n_cent==0 && s->n_pcent==0
              && s->cx==old_cx && s->cy==old_cy
              && s->ppx==old_px && s->ppy==old_py);
    }
    inject(0);
    api_centroids(s);
    check("centroids recover after injected failures",s->n_cent>0 && s->n_pcent>0
          && s->cx && s->cy && s->ppx && s->ppy);

    /* Les deux A* partagent le même scratch. On libère avant chaque série pour
     * faire échouer chacune des six allocations, puis on exige une reprise. */
    api_astar_buffers_release();
    for(long n=1;n<=6;n++){
        inject(n);
        bool ok=api_astar_buffers_ensure();
        check("terrestrial A* reaches each injected allocation",
              alloc_count>=n);
        check("terrestrial A* allocation failure is contained",
              !ok && astar_buffers_empty());
        api_astar_buffers_release();
    }
    inject(0);
    check("terrestrial A* scratch recovers",api_astar_buffers_ensure()
          && astar_buffers_complete());
    api_astar_buffers_release();
    {
        int roads=scps_roads_build(s);
        check("terrestrial route build returns a real count",roads>=0);
        if(roads>0){
            ScpsRoadPt path[8]; int level=-1;
            check("terrestrial route path is readable",
                  scps_road_path(s,0,path,8,&level)>0 && level>=0);
        }
    }

    api_astar_buffers_release();
    for(long n=1;n<=6;n++){
        inject(n);
        bool ok=api_astar_buffers_ensure();
        check("maritime A* reaches each injected allocation",
              alloc_count>=n);
        check("maritime A* allocation failure is contained",
              !ok && astar_buffers_empty());
        api_astar_buffers_release();
    }
    inject(0);
    check("maritime A* scratch recovers",api_astar_buffers_ensure()
          && astar_buffers_complete());
    api_astar_buffers_release();
    {
        int lanes=scps_sea_lanes_build(s);
        check("maritime lane build returns a real count",lanes>=0);
        if(lanes==0){
            /* Seed 9 peut ne poser aucune route maritime au jour zéro. Pour
             * exercer quand même le vrai A* marin, on installe dans le banc
             * une paire déterministe d'ancres qui admet un chemin dans la
             * carte déjà générée, sans passer par un hook de production. */
            int fra=-1,frb=-1;
            api_astar_buffers_ensure();
            for(int ra=0;ra<s->sim.econ->n_regions && fra<0;ra++){
                int ax,ay; if(!world_region_sea_anchor(s->w,ra,&ax,&ay)) continue;
                for(int rb=ra+1;rb<s->sim.econ->n_regions;rb++){
                    int bx,by; ApiLane probe;
                    if(!world_region_sea_anchor(s->w,rb,&bx,&by)) continue;
                    if(api_lane_astar(s->w,ax,ay,bx,by,&probe)){
                        fra=ra; frb=rb; break;
                    }
                }
            }
            check("deterministic maritime A* fixture exists",fra>=0 && frb>=0);
            if(fra>=0){
                memset(s->sim.rn,0,sizeof *s->sim.rn);
                s->sim.rn->n=1;
                s->sim.rn->route[0].ra=fra; s->sim.rn->route[0].rb=frb;
                s->sim.rn->route[0].maritime=1; s->sim.rn->route[0].open=1;
                lanes=scps_sea_lanes_build(s);
            }
        }
        if(lanes>0){
            ScpsRoadPt path[8]; int open=-1,choke=-2,ra=-2,rb=-2;
            check("maritime lane path is readable",
                  scps_sea_lane_path(s,0,path,8,&open,&choke,&ra,&rb)>0
                  && open>=0 && ra>=0 && rb>=0);
        }
    }

    scps_sim_free(s);
    printf("api_cache_demo: %d/%d\n",pass_count,pass_count+fail_count);
    return fail_count?1:0;
}
