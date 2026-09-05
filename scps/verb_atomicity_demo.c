/* Focused regression checks for action atomicity.  This file is a test harness;
 * it is intentionally not part of the production command path. */
#include "scps_tune.h"
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_agency.h"
#include "scps_campaign.h"
#include "scps_navy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int pass_count, fail_count;
static void check(const char *name, int yes){
    if (yes) pass_count++;
    else { fail_count++; printf("FAIL %s\n", name); }
}

static int first_owned_province(WorldEconomy *e){
    for (int p=0; p<e->n_prov; p++) if (e->prov[p].owner>=0) return p;
    return -1;
}

static int find_sea_pair(World *w, WorldEconomy *e, int *src, int *dst){
    for (int a=0; a<e->n_regions; a++){
        int ax,ay;
        if (!world_region_sea_anchor(w,a,&ax,&ay)) continue;
        for (int b=0; b<e->n_regions; b++){
            int bx,by;
            if (a==b || !world_region_sea_anchor(w,b,&bx,&by)) continue;
            if (world_sea_days(w,ax,ay,bx,by)>=0.f){ *src=a; *dst=b; return 1; }
        }
    }
    return 0;
}

static ArmyState one_packet(void){
    ArmyState a; army_init(&a); a.n_units=1;
    a.units[0].type=U_MILICE; a.units[0].count=1;
    return a;
}

int main(void){
    World *w=(World*)calloc(1,sizeof *w);
    WorldEconomy *e=(WorldEconomy*)calloc(1,sizeof *e);
    if (!w || !e){ free(w); free(e); return 1; }
    tune_set("NAVY_COMBAT_ON",1.f);
    tune_set("EMBARK_NAVAL_COST",10.f);
    WorldParams wp=worldparams_default(42u);
    world_generate(w,&wp);
    econ_init(e,w);
    gen_population(w,e);
    worldgen_seed_peoples(w,e,HERITAGE_ADAPTATIF);
    for (int i=0;i<4;i++) econ_tick(e,1.f);

    /* Renovation: a full action queue must reject before touching the treasury or
     * province wealth. The same fixture then proves the accepted debit/enqueue. */
    {
        int pid=first_owned_province(e), reg=(pid>=0)?e->prov[pid].region:-1;
        AgencyState a; agency_init(&a);
        float before_gold=0.f, before_lab=0.f, before_bour=0.f;
        int ready=(pid>=0 && reg>=0);
        if (ready){
            e->prov[pid].owner=0; e->prov[pid].colonized=true;
            e->region[reg].owner=0; e->region[reg].colonized=true;
            e->prov[pid].edi_built=(1u<<EDI_MARCHE);
            e->nat_treasury[0]=1000000.f;
            e->nat_stock[0][RES_WOOD]=10000.f;
            e->nat_stock[0][RES_CLAY]=10000.f;
            e->nat_stock[0][RES_STONE]=10000.f;
            before_gold=e->nat_treasury[0];
            before_lab=e->prov[pid].strata[CLASS_LABORER].wealth;
            before_bour=e->prov[pid].strata[CLASS_BOURGEOIS].wealth;
            a.n=SCPS_MAX_BUILDS;
            check("renovation queue-full refuses", !agency_renover_acct(&a,e,w,reg,0,pid));
            check("renovation queue-full keeps treasury", e->nat_treasury[0]==before_gold);
            check("renovation queue-full keeps wages", e->prov[pid].strata[CLASS_LABORER].wealth==before_lab
                  && e->prov[pid].strata[CLASS_BOURGEOIS].wealth==before_bour);
            a.n=0;
            check("renovation accepted enqueues", agency_renover_acct(&a,e,w,reg,0,pid));
            check("renovation accepted debits and queues", a.n==1 && e->nat_treasury[0]<before_gold);
        } else check("renovation fixture available",0);
    }

    /* Reserve embarkment: all geographic and transport failures must precede the
     * supplies debit and the transfer out of the source reserve. */
    {
        int src=-1,dst=-1, ready=find_sea_pair(w,e,&src,&dst);
        static Campaign c; NavyState n; ArmyState force=one_packet();
        campaign_init(&c,w,e); navy_init(&n);
        if (ready){
            e->region[src].owner=0; e->region[src].coastal=true; e->region[src].build.port=1.f;
            e->region[dst].owner=1; e->region[dst].coastal=true;
            e->nat_stock[0][RES_NAVAL_SUPPLIES]=100.f;
            e->nat_treasury[0]=100000.f;
            float stock=100.f; long troops=force.units[0].count;
            e->region[dst].coastal=false;
            check("embark invalid coast refuses", !campaign_order_sea(&c,w,e,&n,0,src,dst,&force));
            check("embark invalid coast keeps stock", e->nat_stock[0][RES_NAVAL_SUPPLIES]==stock);
            check("embark invalid coast keeps reserve", force.units[0].count==troops && c.n_sails==0);
            e->region[dst].coastal=true;
            check("embark no transport refuses", !campaign_order_sea(&c,w,e,&n,0,src,dst,&force));
            check("embark no transport keeps stock/reserve", e->nat_stock[0][RES_NAVAL_SUPPLIES]==stock
                  && force.units[0].count==troops && c.n_sails==0);
            n.n[0].hull[HULL_TRANSPORT]=1;
            e->nat_stock[0][RES_NAVAL_SUPPLIES]=0.f;
            check("embark missing supplies refuses", !campaign_order_sea(&c,w,e,&n,0,src,dst,&force));
            check("embark missing supplies keeps reserve", force.units[0].count==troops && c.n_sails==0);
            n.n[0].hull[HULL_TRANSPORT]=1;
            e->nat_stock[0][RES_NAVAL_SUPPLIES]=100.f;
            check("embark valid succeeds", campaign_order_sea(&c,w,e,&n,0,src,dst,&force));
            check("embark success transfers and debits", force.units[0].count==0 && c.n_sails==1
                  && c.army[0].phase==FA_EMBARK && e->nat_stock[0][RES_NAVAL_SUPPLIES]==90.f);
        } else check("sea pair fixture available",0);
    }

    /* Naval build: each required stock must be present before the treasury/population
     * debit and before build_hull is set. War hull exercises supplies, wood and copper. */
    {
        int cid=-1,port=-1;
        for (int c=0;c<w->n_countries && cid<0;c++){
            int r=navy_best_coast(w,e,c);
            if (r>=0){ cid=c; port=r; }
        }
        NavyState n; navy_init(&n);
        if (cid>=0){
            e->region[port].owner=(int16_t)cid; e->region[port].coastal=true; e->region[port].build.port=1.f;
            e->region[port].strata[CLASS_LABORER].pop=10000.f;
            e->nat_treasury[cid]=1000000.f;
            e->nat_stock[cid][RES_NAVAL_SUPPLIES]=1000.f;
            e->nat_stock[cid][RES_WOOD]=1000.f;
            e->nat_stock[cid][RES_COPPER]=1000.f;
            const Resource missing[3]={RES_NAVAL_SUPPLIES,RES_WOOD,RES_COPPER};
            for (int i=0;i<3;i++){
                e->nat_stock[cid][missing[i]]=0.f;
                float gold=e->nat_treasury[cid], pop=e->region[port].strata[CLASS_LABORER].pop;
                check("navy missing kit item refuses", !navy_order_build(&n,w,e,cid,HULL_WAR));
                check("navy missing kit item is atomic", e->nat_treasury[cid]==gold
                      && e->region[port].strata[CLASS_LABORER].pop==pop && n.n[cid].build_hull<0);
                e->nat_stock[cid][missing[i]]=1000.f;
            }
            check("navy complete kit succeeds", navy_order_build(&n,w,e,cid,HULL_WAR));
            check("navy success starts one hull", n.n[cid].build_hull==HULL_WAR);
        } else check("navy port fixture available",0);
    }

    printf("ATOMICITY %d passed, %d failed\n",pass_count,fail_count);
    free(e); free(w);
    return fail_count ? 1 : 0;
}
