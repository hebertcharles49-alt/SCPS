/* Banc auto-vérifiant du règlement monétaire du commerce régional. */
#include "scps_trade.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ok=0, bad=0;
#define CHECK(name,cond) do { if(cond){ ok++; printf("  OK  %s\n",name); } \
                              else { bad++; printf("  NON %s\n",name); } } while(0)

/* Stubs du grain province : ces fixtures utilisent volontairement la vue région
 * seule. Le moteur de commerce doit rester conservatif dans les deux grains. */
int econ_region_rep_province(const WorldEconomy *e, int region){ (void)e; (void)region; return -1; }
float econ_build_reserve(Resource r){ (void)r; return 0.f; }
const char *resource_name(Resource r){ (void)r; return "bien"; }
float econ_region_stock_add(WorldEconomy *e, int region, int good, float delta){
    float *s=&e->region[region].stock[good];
    if(delta>=0.f){ *s+=delta; return delta; }
    float take=fminf(*s,-delta); *s-=take; return -take;
}

static float money(const WorldEconomy *e){
    float m=0.f;
    for(int r=0;r<e->n_regions;r++)
        for(int c=0;c<CLASS_COUNT;c++) m+=e->region[r].strata[c].wealth;
    return m;
}

static void fixture(WorldEconomy *e, TradeNetwork *n, float buyer_wealth){
    memset(e,0,sizeof(*e)); memset(n,0,sizeof(*n)); e->n_regions=2;
    e->region[0].price[RES_GRAIN]=1.f;
    e->region[1].price[RES_GRAIN]=10.f;
    e->region[0].stock[RES_GRAIN]=20.f;
    /* Composition volontairement différente : l'ancien code regardait le
     * bourgeois exportateur pour décider si le laborer importateur payait. */
    e->region[0].strata[CLASS_BOURGEOIS].pop=100.f;
    e->region[1].strata[CLASS_LABORER].pop=1000.f;
    e->region[1].strata[CLASS_LABORER].wealth=buyer_wealth;
    n->n_links=1; n->link[0].ra=0; n->link[0].rb=1;
    n->link[0].capacity=10.f; n->link[0].transport_cost=0.10f;
}

int main(void){
    WorldEconomy *e=calloc(1,sizeof(*e)); TradeNetwork *n=calloc(1,sizeof(*n));
    if(!e||!n){ fprintf(stderr,"OOM\n"); return 2; }

    fixture(e,n,100.f);
    float before=money(e); trade_tick(e,n,NULL); float after=money(e);
    CHECK("composition sociale différente : un flux existe",n->n_flows==1);
    CHECK("composition sociale différente : monnaie conservée",fabsf(after-before)<0.01f);
    CHECK("composition sociale différente : importateur débité",e->region[1].strata[CLASS_LABORER].wealth<2.f);
    CHECK("composition sociale différente : vendeur payé",e->region[0].strata[CLASS_BOURGEOIS].wealth>98.f);

    fixture(e,n,25.f);
    before=money(e); trade_tick(e,n,NULL); after=money(e);
    CHECK("acheteur pauvre : monnaie conservée",fabsf(after-before)<0.01f);
    CHECK("acheteur pauvre : dépense bornée",e->region[1].strata[CLASS_LABORER].wealth<0.01f);
    CHECK("acheteur pauvre : recette bornée",fabsf(e->region[0].strata[CLASS_BOURGEOIS].wealth-25.f)<0.01f);
    CHECK("acheteur pauvre : volume borné",n->n_flows==1 && n->flow[0].volume<3.f);

    free(e); free(n);
    printf("BILAN : %d réussis, %d échoués\n",ok,bad);
    return bad?1:0;
}
