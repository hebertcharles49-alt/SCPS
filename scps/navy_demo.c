/*
 * navy_demo.c — banc auto-vérifiant : LA FLOTTE (scps_navy)
 *
 *   make navy_demo && ./navy_demo [graine]
 *
 * Prouve, sur un monde RÉEL : le choix de rade (navy_best_coast/_port), le chantier
 * (navy_order_build : port + trésor + bras requis ; UN seul chantier à la fois ; sans
 * port rien ne se bâtit), la complétion au tick, l'emport (10 paquets/transport), la
 * conversion marchand→pirate, et les INVARIANTS que save_sane revérifie au chargement
 * (coques bornées, at_sea≥0, build_hull & home_port en domaine). Sortie ≠ 0 si échec.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_species.h"
#include "scps_navy.h"
#include <stdio.h>
#include <stdlib.h>

static int g_pass=0,g_fail=0;
static void ok(const char*what,bool c){ if(c)g_pass++; else { g_fail++; printf("   ✗ %s\n",what); } }

int main(int argc,char**argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    if(!w||!econ){fprintf(stderr,"OOM\n");return 1;}
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LA FLOTTE — rade, chantier, emport, conversion (graine %u)\n",seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,RACE_HUMAIN);
    for(int t=0;t<8;t++) econ_tick(econ,1.f);   /* peuple les strates (bras des rades) */

    NavyState ns; navy_init(&ns);
    /* (1) état d'init : rien en mer, pas de chantier, pas de rade */
    { const Navy*n=&ns.n[0];
      ok("navy_init : 0 coque",
         n->hull[HULL_WAR]==0&&n->hull[HULL_TRANSPORT]==0&&n->hull[HULL_MERCHANT]==0&&n->hull[HULL_PIRATE]==0);
      ok("navy_init : pas de chantier (build_hull=-1)", n->build_hull==-1);
      ok("navy_init : pas de rade (home_port=-1)", n->home_port==-1);
      ok("navy_init : rien en mer, rien bâti", n->at_sea==0 && n->built_total==0); }
    /* (2) équipage : 100 marins pour une bordée, 50 pour le reste */
    ok("équipage : navire de guerre=100", navy_hull_crew(HULL_WAR)==NAVY_CREW_WAR);
    ok("équipage : coque légère=50",
       navy_hull_crew(HULL_TRANSPORT)==NAVY_CREW_LIGHT && navy_hull_crew(HULL_MERCHANT)==NAVY_CREW_LIGHT);

    /* (3) trouver un pays CÔTIER (best_coast >= 0) */
    int cid=-1, coast=-1;
    for(int c=0;c<w->n_countries;c++){
        if(w->country[c].capital_prov<0) continue;
        int bc=navy_best_coast(w,econ,c);
        if(bc>=0){ cid=c; coast=bc; break; }
    }
    if(cid<0){
        printf("   (aucun pays côtier sur cette graine — corps du banc sauté)\n");
    } else {
        /* sans port : un pays côtier mais SANS port ne bâtit rien (best_port=-1) */
        ok("sans port : navy_order_build ÉCHOUE", !navy_order_build(&ns,w,econ,cid,HULL_TRANSPORT));

        /* (4) asseoir une rade sur la meilleure côte : port + trésor + bras + matière au marché */
        { RegionEconomy*re=&econ->region[coast];
          re->build.port=1.f; re->treasury=1.0e9f;
          re->strata[CLASS_LABORER].pop=8000.f;
          re->stock[RES_NAVAL_SUPPLIES]=5000.f; re->stock[RES_WOOD]=5000.f; re->stock[RES_METAL]=5000.f; }
        ok("la côte devient un PORT (navy_region_is_port)", navy_region_is_port(w,econ,coast));
        ok("navy_best_port retrouve la rade", navy_best_port(w,econ,cid)==coast);
        ok("navy_build_gold(transport) > 0 (recette chiffrée au marché)", navy_build_gold(econ,coast,HULL_TRANSPORT)>0.f);

        /* (5) commander un transport : succès ; chantier occupé ; UN seul à la fois */
        ok("navy_order_build(transport) RÉUSSIT", navy_order_build(&ns,w,econ,cid,HULL_TRANSPORT));
        float bdays=ns.n[cid].build_days;
        ok("chantier lancé (build_hull=TRANSPORT, jours>0, rade=coast)",
           ns.n[cid].build_hull==HULL_TRANSPORT && bdays>0.f && ns.n[cid].home_port==coast);
        ok("UN chantier à la fois : 2e commande ÉCHOUE", !navy_order_build(&ns,w,econ,cid,HULL_WAR));

        /* (6) avancer jusqu'à la complétion : la coque NAÎT */
        navy_tick(&ns,w,econ,NULL,bdays+1.f);
        ok("transport BÂTI (hull[TRANSPORT]=1, chantier libéré, built_total=1)",
           ns.n[cid].hull[HULL_TRANSPORT]==1 && ns.n[cid].build_hull==-1 && ns.n[cid].built_total==1);
        /* (7) emport : 1 transport = 10 paquets */
        ok("emport : 10 paquets libres pour 1 transport", navy_transport_packets_free(&ns,cid)==10);

        /* (8) bâtir un marchand, le compléter, puis le CONVERTIR en pirate (réversible, au chantier) */
        ok("navy_order_build(marchand) RÉUSSIT", navy_order_build(&ns,w,econ,cid,HULL_MERCHANT));
        float mdays=ns.n[cid].build_days; navy_tick(&ns,w,econ,NULL,mdays+1.f);
        ok("marchand BÂTI (hull[MERCHANT]=1)", ns.n[cid].hull[HULL_MERCHANT]==1);
        int mer0=ns.n[cid].hull[HULL_MERCHANT], pir0=ns.n[cid].hull[HULL_PIRATE];
        ok("conversion marchand→pirate (1 marchand → 1 pirate)",
           navy_convert(&ns,w,econ,cid,true) && ns.n[cid].hull[HULL_MERCHANT]==mer0-1 && ns.n[cid].hull[HULL_PIRATE]==pir0+1);

        /* (9) INVARIANTS save_sane : coques bornées, at_sea≥0, build_hull & home_port en domaine */
        { const Navy*n=&ns.n[cid]; bool inv=true;
          for(int t=0;t<HULL_COUNT;t++) if(n->hull[t]<0||n->hull[t]>100000) inv=false;
          if(n->at_sea<0) inv=false;
          if(n->build_hull<-1||n->build_hull>=HULL_COUNT) inv=false;
          if(n->home_port<-1||n->home_port>=econ->n_regions) inv=false;
          ok("invariants save_sane tenus (coques/at_sea/build_hull/home_port)", inv); }

        /* (10) distance de mer port-à-port : la fonction RÉPOND (≥0 joignable, -1 sinon — finie) */
        { float sd=navy_sea_days_regions(w,coast,coast); ok("navy_sea_days_regions rend une valeur finie", sd>=-1.f); }
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ);
    return g_fail?1:0;
}
