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
#include "scps_heritage.h"
#include "scps_navy.h"
#include "scps_campaign.h"
#include "scps_diplo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
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
          re->stock[RES_NAVAL_SUPPLIES]=5000.f; re->stock[RES_WOOD]=5000.f; re->stock[RES_COPPER]=5000.f; }
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

        /* (10a) LE BLOCUS EST UN RISQUE, PAS UN MUR. On prend une vraie côte
         * joignable, on place une bordée ennemie devant la rade et on vérifie les
         * deux chemins d'ordre. Le convoi principal doit PARTIR en FA_EMBARK puis
         * pouvoir être physiquement intercepté dès ce chargement (les traversées
         * courtes peuvent sinon disparaître dans un seul tick mensuel). */
        if (w->n_countries>=2){
            int sea_target=-1;
            for (int r=0;r<econ->n_regions;r++)
                if (r!=coast && econ->region[r].coastal
                    && navy_sea_days_regions(w,coast,r)>=0.f){ sea_target=r; break; }
            int blocker=(cid==0)?1:0;
            Campaign *sail=calloc(1,sizeof *sail);
            DiploState *sdp=calloc(1,sizeof *sdp);
            ok("blocus-risque : une autre côte du même bassin existe", sea_target>=0);
            ok("blocus-risque : fixtures allouées", sail!=NULL && sdp!=NULL);
            if (sea_target>=0 && sail && sdp){
                NavyState risk; navy_init(&risk);
                campaign_init(sail,w,econ); diplo_init(sdp); diplo_declare_war(sdp,blocker,cid);
                risk.n[cid].hull[HULL_TRANSPORT]=1;
                risk.n[blocker].hull[HULL_WAR]=1;
                risk.n[blocker].mission=NAVY_BLOCUS;
                risk.n[blocker].mission_target=cid;
                ArmyState convoy; army_init(&convoy); convoy.n_units=1;
                convoy.units[0].type=U_MILICE; convoy.units[0].count=7;
                bool sails=campaign_order_sea(sail,w,econ,&risk,cid,coast,sea_target,&convoy);
                ok("blocus-risque : le convoi part malgré le blocus (FA_EMBARK)",
                   sails && sail->army[cid].phase==FA_EMBARK && risk.n[cid].at_sea==1);
                uint32_t rrng=seed^0xA511E9B3u;
                for (int tries=0;tries<64 && sail->army[cid].active;tries++)
                    navy_interception_tick(&risk,sail,w,econ,sdp,&rrng);
                ok("blocus-risque : la bordée peut couler le convoi pendant l'embarquement",
                   !sail->army[cid].active && risk.n[blocker].intercepts==1);

                campaign_init(sail,w,econ); navy_init(&risk);
                int sid=campaign_corps_id(cid,1); FieldArmy *secondary=campaign_corps(sail,sid);
                secondary->id=sid; secondary->owner=cid; secondary->active=true;
                secondary->loc=coast; secondary->phase=FA_IDLE; secondary->force.n_units=1;
                secondary->force.units[0].type=U_MILICE; secondary->force.units[0].count=7;
                risk.n[cid].hull[HULL_TRANSPORT]=1;
                risk.n[blocker].hull[HULL_WAR]=1;
                risk.n[blocker].mission=NAVY_BLOCUS; risk.n[blocker].mission_target=cid;
                ok("blocus-risque : un corps secondaire peut aussi réembarquer sous blocus",
                   campaign_redirect_corps_sea(sail,w,econ,&risk,sid,sea_target)
                   && secondary->phase==FA_EMBARK && risk.n[cid].at_sea==1);
            }
            free(sdp); free(sail);
        }

        /* (10b) DOCTRINE D'ORDRE — le texte moteur promet une patrouille : l'éthos
         * ORDRE doit donc armer une bordée en guerre, puis la déployer au lieu de
         * rester hors de la branche Dominateur/Bureaucrate. */
        if (w->n_countries>=2){
            NavyState ord; navy_init(&ord);
            DiploState *dpord=calloc(1,sizeof *dpord);
            RouteNetwork *rnord=calloc(1,sizeof *rnord);
            ok("doctrine ORDRE : fixtures allouées", dpord!=NULL && rnord!=NULL);
            if (dpord && rnord){
                diplo_init(dpord); diplo_declare_war(dpord,cid,(cid==0)?1:0);
                econ->region[coast].culture.ethos=ETHOS_ORDRE;
                navy_course_tick(&ord,w,econ,dpord,rnord,&seed,NULL,-1,30.f);
                ok("doctrine ORDRE : une bordée est mise en chantier pendant la guerre",
                   ord.n[cid].build_hull==HULL_WAR);
                navy_tick(&ord,w,econ,dpord,ord.n[cid].build_days+1.f);
                navy_course_tick(&ord,w,econ,dpord,rnord,&seed,NULL,-1,30.f);
                ok("doctrine ORDRE : la première bordée patrouille/intercepte",
                   ord.n[cid].hull[HULL_WAR]>=1 && ord.n[cid].mission==NAVY_INTERCEPTION);
            }
            free(rnord); free(dpord);
        }
    }

    /* (11) INTERCEPTION — un corps secondaire en FA_SAIL est vu AVANT son
     * avancement de campagne ; le compteur noyé doit lire CE corps, pas le corps
     * principal du même pays. On répète seulement le jet de rencontre (45 %),
     * jamais l'issue : sans escorte, le convoi trouvé est une proie certaine. */
    if (w->n_countries>=2){
        int hunter=0, victim=1;
        Campaign *camp=calloc(1,sizeof *camp);
        DiploState *dp=calloc(1,sizeof *dp);
        NavyState ix; navy_init(&ix);
        ok("interception : fixtures allouées", camp!=NULL && dp!=NULL);
        if (camp && dp){
            diplo_init(dp); diplo_declare_war(dp,hunter,victim);
            int id=campaign_corps_id(victim,1);              /* surtout PAS id==owner */
            FieldArmy *fa=campaign_corps(camp,id);
            fa->id=id; fa->owner=victim; fa->active=true; fa->phase=FA_SAIL;
            fa->sail_transports=1; fa->force.n_units=1;
            fa->force.units[0].type=U_MILICE; fa->force.units[0].count=7;
            /* Le cas important du raccord live : la doctrine met les bordées en
             * BLOCUS si l'ennemi a un port ; ce blocus doit voir SA cible en mer. */
            ix.n[hunter].mission=NAVY_BLOCUS; ix.n[hunter].mission_target=victim;
            ix.n[hunter].hull[HULL_WAR]=1;
            ix.n[victim].hull[HULL_TRANSPORT]=1; ix.n[victim].at_sea=1;
            uint32_t irng=seed^0x9e3779b9u;
            for (int tries=0; tries<64 && fa->active; tries++)
                navy_interception_tick(&ix,camp,w,econ,dp,&irng);
            ok("interception : le blocus voit le convoi de SA cible en FA_SAIL", !fa->active && ix.n[hunter].intercepts==1);
            ok("interception : les noyés sont ceux du corps secondaire exact", ix.n[hunter].drowned==7);
            ok("interception : transport et réservation sont physiquement détruits",
               ix.n[victim].hull[HULL_TRANSPORT]==0 && ix.n[victim].at_sea==0 && fa->sail_transports==0);
        }
        free(dp); free(camp);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ);
    return g_fail?1:0;
}
