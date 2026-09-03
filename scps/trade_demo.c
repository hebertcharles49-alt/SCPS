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
/* STOCK NATIONAL (2026-09-03) : l'entrepôt vit au grain PAYS (nat_stock), plus à la
 * région. Mêmes stubs, même sémantique bornée — seule la clé a changé. */
float econ_country_stock_sum(const WorldEconomy *e, int cid, Resource r){
    if(cid<0||cid>=SCPS_MAX_COUNTRY) return 0.f;
    return e->nat_stock[cid][r];
}
float econ_country_stock_take(WorldEconomy *e, int cid, Resource r, float need){
    if(cid<0||cid>=SCPS_MAX_COUNTRY||need<=0.f) return 0.f;
    float *s=&e->nat_stock[cid][r];
    float take=fminf(*s,need); *s-=take; return take;
}
float econ_nation_stock_add(WorldEconomy *e, int cid, int good, float delta){
    if(cid<0||cid>=SCPS_MAX_COUNTRY) return 0.f;
    float *s=&e->nat_stock[cid][good];
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
    /* STOCK NATIONAL : un lien ne déplace de la matière QUE s'il franchit une
     * frontière (entre deux régions du même pays l'entrepôt est déjà commun) — les
     * deux régions appartiennent donc à DEUX pays distincts, et l'entrepôt de
     * l'exportateur est celui du pays 0. */
    e->region[0].owner=0;
    e->region[1].owner=1;
    e->region[0].price[RES_GRAIN]=1.f;
    e->region[1].price[RES_GRAIN]=10.f;
    e->nat_stock[0][RES_GRAIN]=20.f;
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
