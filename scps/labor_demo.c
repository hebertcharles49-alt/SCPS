/*
 * labor_demo.c — l'économie des populations : jobs, nourriture, marché
 *
 *   make labor_demo && ./labor_demo [graine]
 *
 * Prouve les deux principes (la prod scale sur les JOBS REMPLIS ; les sorties
 * LISENT la géo) et les règles de l'arc « une économie, une bouche » :
 *   1. Jobs, pas bâtiments : doubler les jobs double la prod ; un bâtiment vide = 0.
 *   2. Géo du marché : un marché au désert ~+0.3 ; le même florissant ~+15.
 *   3. Départ canonique E0.5 : 4000 pop, collecteurs DIMENSIONNÉS sur la géo,
 *      PAS de marché ; BOUCHE UNIQUE = pop/100 (E0.2).
 *   4. P-arc : la couche MATÉRIAU labor est éradiquée (elle vit dans le pool éco) —
 *      extraction & atelier sont INERTES (aucune sortie de matériau).
 *   5. Marché dynamique : prix par la demande.
 *   6. Niveaux : un bâtiment niveau 6 offre 2200 pop de jobs.
 *   7. Plein dev : province pleine à Prospérité 100 → pop +15 % plus vite.
 *   8. E0.1/E0.3 : labor ne fait plus NAÎTRE personne (resync depuis le monde) ;
 *      le solde de l'armée sort.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_labor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static bool near_f(float a,float b,float eps){ float d=a-b; if(d<0)d=-d; return d<=eps; }

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World)); LaborEcon *e=malloc(sizeof(LaborEcon));
    if(!w||!e){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" ÉCONOMIE DES POPULATIONS — jobs, matériaux, marché (graine %u)\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    labor_init(e,w);   /* relit la géographie du worldgen */

    /* ═══ 1. JOBS, PAS BÂTIMENTS ════════════════════════════════════════ */
    printf("\n── 1. La prod scale sur les JOBS REMPLIS (pas le nombre de bâtiments) ──\n");
    e->n_prov=1;
    memset(&e->prov[0],0,sizeof(LProvince));
    e->prov[0].prov=0; e->prov[0].colonized=true; e->prov[0].n_bld=1;
    e->prov[0].bld[0]=(LBuilding){ LB_COLLECTOR, 0, 0, 0 };   /* niveau 0, VIDE */
    LRes res;
    float out0=labor_building_output(e,0,0,&res);
    e->prov[0].bld[0].jobs_filled=1; float out1=labor_building_output(e,0,0,&res);
    e->prov[0].bld[0].jobs_filled=2; float out2=labor_building_output(e,0,0,&res);
    printf("   collecteur : 0 job → %.2f | 1 job → %.2f | 2 jobs → %.2f\n", out0,out1,out2);
    ok("un bâtiment VIDE (0 job) ne produit rien", near_f(out0,0.f,0.001f));
    ok("doubler les jobs remplis DOUBLE la prod", out1>0.f && near_f(out2, 2.f*out1, 0.001f));

    /* ═══ 2. LA GÉO DU MARCHÉ (le carrefour, pas le bonus plat) ═════════ */
    printf("\n── 2. Le marché LIT le flux : désert ~+0.3, florissant ~+15 ──\n");
    int desert=-1, floris=-1; float lo=1e9f, hi=-1.f;
    for (int pr=0; pr<w->n_provinces; pr++){
        float f=province_trade_flow(e,pr);
        if (f<lo){ lo=f; desert=pr; }
        if (f>hi){ hi=f; floris=pr; }
    }
    float md=labor_market_output(e,desert,1), mf=labor_market_output(e,floris,1);
    printf("   marché (1 job) au désert (prov %d, flux %.2f) → +%.2f | florissant (prov %d, flux %.2f) → +%.2f\n",
           desert, lo, md, floris, hi, mf);
    ok("le même marché rend une misère à la marge et une fortune au carrefour (lit le flux)",
       mf > 8.0f && md < mf/6.0f);

    /* ═══ 3. DÉPART CANONIQUE E0.5 (géo-posé, bouche unique, viable) ═════ */
    printf("\n── 3. Le départ canonique E0.5 : collecteurs dimensionnés, pas de marché ──\n");
    labor_seed_start(e, 0);
    int ncol=0, nate=0, nmar=0, next_=0;
    for (int b=0;b<e->prov[0].n_bld;b++){
        LBuildType t=e->prov[0].bld[b].type;
        if (t==LB_COLLECTOR) ncol++;
        else if (t==LB_WORKSHOP) nate++;
        else if (t==LB_MARKET) nmar++;
        else next_++;
    }
    long bouche=labor_food_consumed(e), collecte=labor_food_collected(e);
    printf("   pop=%ld  collecteurs=%d extraction=%d ateliers=%d marchés=%d | bouche %ld/j · collecte %ld/j · stock F=%ld\n",
           e->prov[0].pop, ncol, next_, nate, nmar, bouche, collecte,
           e->stock[LR_FOOD]);
    ok("4000 pop, 200 F ; un emplacement d'extraction (inerte) est posé (E0.5 ; P-arc : matériau → pool éco)",
       e->prov[0].pop==4000 && ncol>=1 && next_==1 && nate==1 &&
       e->stock[LR_FOOD]==200);
    ok("PAS de marché au départ — le commerce early passe par les Centres (P3.20)", nmar==0);
    ok("LA BOUCHE UNIQUE (E0.2) : conso == pop/100 (tous mangent, employés ou non)",
       bouche == e->prov[0].pop/100);
    ok("le départ est VIABLE : les collecteurs dimensionnés couvrent la bouche",
       labor_food_balance(e) >= 0);
    { long fields=0;
      for (int b=0;b<e->prov[0].n_bld;b++)
          if (e->prov[0].bld[b].type==LB_COLLECTOR) fields+=e->prov[0].bld[b].jobs_filled;
      printf("   pop aux champs : %ld%% (cible ~15-25 %%)\n", fields*POP_PER_SLOT*100/e->prov[0].pop);
      ok("~15-25 %% de la pop aux champs (1 slot nourrit 400-800 âmes selon la fertilité)",
         fields*POP_PER_SLOT*100/e->prov[0].pop >= 10 && fields*POP_PER_SLOT*100/e->prov[0].pop <= 30); }

    /* ═══ 4. P-arc : EXTRACTION & ATELIER INERTES (le matériau vit dans le pool éco) ═══ */
    printf("\n── 4. La couche matériau labor est ÉRADIQUÉE : extraction & atelier ne produisent rien ──\n");
    /* Une province : scierie + mine + atelier, 1 job chacun — autrefois la chaîne d'outils. */
    memset(e,0,sizeof(*e)); labor_init(e,w); e->n_prov=1;
    LProvince *pc=&e->prov[0];
    memset(pc,0,sizeof(*pc)); pc->prov=0; pc->colonized=true; pc->n_bld=3;
    pc->bld[0]=(LBuilding){ LB_SAWMILL, 0, 1, 0 };
    pc->bld[1]=(LBuilding){ LB_MINE,    0, 1, 0 };
    pc->bld[2]=(LBuilding){ LB_WORKSHOP,0, 1, 0 };
    long chain_pop = (pc->bld[0].jobs_filled+pc->bld[1].jobs_filled+pc->bld[2].jobs_filled)*POP_PER_SLOT;
    /* aucun stock labor ne doit gonfler : la sortie matériau a disparu. On vérifie que
     * la ressource PROPRE (nourriture) ne reçoit rien de ces bâtiments. */
    long food0=e->stock[LR_FOOD];
    LRes ores; float owk=labor_building_output(e,0,2,&ores);   /* la sortie de l'atelier */
    for (int t=0;t<3;t++) labor_tick(e);
    printf("   chaîne = %ld pop (scierie+mine+atelier) ; sortie atelier = %.2f (sentinelle, aucune ressource)\n",
           chain_pop, owk);
    ok("les emplacements existent toujours (3 jobs = 300 pop)", chain_pop==300);
    ok("extraction & atelier sont INERTES : aucune sortie de matériau (sentinelle LR_COUNT)",
       ores==LR_COUNT && owk==0.f);
    ok("la nourriture n'est pas produite par scierie/mine/atelier (passe extraction sautée)",
       e->stock[LR_FOOD]<=food0);

    /* ═══ 5. LE MARCHÉ DYNAMIQUE (demande → prix) ════════════════════════ */
    printf("\n── 5. Le prix des matériaux est GÉNÉRÉ par la demande ──\n");
    memset(e,0,sizeof(*e)); labor_init(e,w);
    e->market.supply=10.f; e->market.demand=5.f;  float price_low=labor_material_price(e);
    e->market.demand=80.f;                          float price_high=labor_material_price(e);
    printf("   prix matériaux : faible demande → %.2f | tout le monde bâtit (forte demande) → %.2f\n",
           price_low, price_high);
    ok("forte demande (tout le monde bâtit) → le prix MONTE", price_high > price_low);

    /* ═══ 6. LES NIVEAUX — les jobs qui scalent ════════════════════════ */
    printf("\n── 6. Six niveaux ; capacité de jobs cumulée 100→1000 ──\n");
    printf("   capacité (pop) par niveau atteint : ");
    for (int L=0;L<6;L++) printf("%d ", building_job_capacity_pop(L));
    printf("\n");
    ok("un bâtiment niveau 6 offre 2200 pop de jobs (100+100+200+300+500+1000)",
       building_job_capacity_pop(5)==2200 && building_job_slots(5)==22);
    ok("un bâtiment niveau 1 offre 100 pop de jobs (1 slot)", building_job_capacity_pop(0)==100);

    /* ═══ 7. LE BONUS DE PLEIN DÉVELOPPEMENT ═══════════════════════════ */
    printf("\n── 7. Plein dev (6 bâtiments) + Prospérité 100 → +15 %% de vitesse ──\n");
    LProvince full; memset(&full,0,sizeof full);
    full.colonized=true; full.n_bld=LAB_BUILDINGS_PER_PROV;   /* 6 emplacements occupés */
    LProvince partial=full; partial.n_bld=4;                  /* pas pleine */
    printf("   dev : pleine@100=%.2f | pleine@80=%.2f | partielle@100=%.2f\n",
           labor_pop_dev_speed(&full,100), labor_pop_dev_speed(&full,80), labor_pop_dev_speed(&partial,100));
    ok("une province pleine à Prospérité 100 développe sa pop +15 %% plus vite",
       near_f(labor_pop_dev_speed(&full,100), 1.15f, 0.001f));
    ok("sinon (pas pleine, ou Prospérité < 100) → vitesse normale",
       near_f(labor_pop_dev_speed(&full,80),1.0f,0.001f) && near_f(labor_pop_dev_speed(&partial,100),1.0f,0.001f));

    /* ═══ 8. LA POP EST UN POOL — et le monde en est le PROPRIÉTAIRE (E0.1) ═══ */
    printf("\n── 8. E0.1/E0.3 : pop relue du monde · taxes quotidiennes · solde d'armée ──\n");
    memset(e,0,sizeof(*e)); labor_init(e,w); e->n_prov=1;
    LProvince *pp=&e->prov[0]; memset(pp,0,sizeof(*pp));
    pp->prov=0; pp->region=-1; pp->colonized=true; pp->pop=1000; pp->pop_by_class[LAB_LABORER]=1000;
    /* on AFFECTE tout : 6 collecteurs (600 en job) + 400 enrôlés → 0 libre. */
    for (int b=0;b<6;b++) pp->bld[b]=(LBuilding){ LB_COLLECTOR, 0, 1, 0 };
    pp->n_bld=6; pp->pop_in_army=400;
    e->stock[LR_FOOD]=500;                               /* pas de famine */
    PopBreakdown k0=labor_pop_breakdown(e);
    labor_print_topbar(e);
    ok("répartition cohérente : total = libre + en job + en armée",
       k0.total==1000 && k0.in_jobs==600 && k0.in_army==400 && k0.free==0 &&
       (k0.free+k0.in_jobs+k0.in_army)==k0.total);
    long pop_before=labor_pop_total(e);
    for (int t=0;t<30;t++) labor_tick(e);                /* un MOIS : prod, bouche, taxes, solde */
    PopBreakdown k1=labor_pop_breakdown(e);
    printf("   après un mois (libre=0 au départ) :\n");
    labor_print_topbar(e);
    ok("E0.1 : labor ne fait plus NAÎTRE personne — le pool est stable sans le monde",
       k1.total == pop_before);
    ok("emploi & armée restent des AFFECTATIONS (inchangées)",
       k1.in_jobs==600 && k1.in_army==400);
    /* E0.3 — le SOLDE : 400 enrôlés → 2 or + 4 nourriture par jour (0.5+1 par 100). */
    { long sg, sf; labor_army_upkeep(e,&sg,&sf);
      printf("   solde : %ld or/j + %ld ration(s)/j pour 400 enrôlés\n", sg, sf);
      ok("E0.3 : le solde vaut 0.5 or + 1 ration par 100 enrôlés et par jour", sg==2 && sf==4); }
    /* E0.1 — RESYNC : la pop labor RELIT les strates de sa région adossée. */
    { WorldEconomy *we=malloc(sizeof(WorldEconomy));
      if (we){
        memset(we,0,sizeof(*we)); we->n_regions=6;
        we->region[5].strata[CLASS_LABORER].pop=800.f;
        we->region[5].strata[CLASS_BOURGEOIS].pop=150.f;
        we->region[5].strata[CLASS_ELITE].pop=50.f;
        memset(e,0,sizeof(*e)); labor_init(e,w); e->n_prov=1;
        LProvince *pr=&e->prov[0]; memset(pr,0,sizeof(*pr));
        pr->prov=0; pr->region=5; pr->colonized=true; pr->pop=99; pr->pop_by_class[LAB_LABORER]=99;
        for (int b=0;b<6;b++) pr->bld[b]=(LBuilding){ LB_COLLECTOR, 0, 1, 0 };
        pr->n_bld=6;
        labor_resync_pop(e,we);
        long after1=pr->pop;
        we->region[5].strata[CLASS_LABORER].pop=250.f;   /* la peste : la région s'effondre */
        we->region[5].strata[CLASS_BOURGEOIS].pop=40.f;
        we->region[5].strata[CLASS_ELITE].pop=10.f;
        labor_resync_pop(e,we);
        long slots=0; for (int b=0;b<pr->n_bld;b++) slots+=pr->bld[b].jobs_filled;
        printf("   resync : pop 99 → %ld, puis effondrement → %ld (emplois réduits à %ld slots)\n",
               after1, pr->pop, slots);
        ok("E0.1 : labor RELIT la pop des strates de sa région (1000 puis 300)",
           after1==1000 && pr->pop==300);
        ok("E0.1 : la pop effondrée VIDE les emplois excédentaires (3 slots pour 300 âmes)",
           slots==3);
        free(we);
      } else { ok("(OOM resync — ignoré)", true); ok("(idem)", true); } }

    /* ═══ 9. INTÉGRATION — l'économie d'un VRAI pays lit sa géographie ══ */
    printf("\n── 9. Intégration : seeder un pays du monde ; la richesse suit la terre ──\n");
    WorldEconomy *econ=malloc(sizeof(WorldEconomy));
    if (econ){
        econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,RACE_HUMAIN);
        /* moyenne du flux géo par pays sur ses régions possédées → riche vs pauvre. */
        int cRich=-1,cPoor=-1; float bestF=-1.f, worstF=1e9f;
        for (int c=0;c<w->n_countries;c++){
            if (w->country[c].role==POLITY_UNCLAIMED) continue;
            double sf=0; int n=0;
            for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c && econ->region[r].culture.settled){
                int pid=w->region[r].province_ids[0]; sf+=province_trade_flow(e,pid); n++;
            }
            if (n<1) continue;
            float mf=(float)(sf/n);
            if (mf>bestF){ bestF=mf; cRich=c; }
            if (mf<worstF){ worstF=mf; cPoor=c; }
        }
        if (cRich>=0 && cPoor>=0 && cRich!=cPoor){
            labor_seed_from_world(e,w,econ,cRich); for(int t=0;t<5;t++) labor_tick(e);
            float idxR=labor_prosperity_index(e);
            labor_seed_from_world(e,w,econ,cPoor); for(int t=0;t<5;t++) labor_tick(e);
            float idxP=labor_prosperity_index(e);
            printf("   pays RICHE (flux %.2f) : indice prospérité %.1f → Prospérité %d\n",
                   bestF, idxR, (int)(idxR*10.f+0.5f));
            printf("   pays PAUVRE (flux %.2f) : indice prospérité %.1f → Prospérité %d\n",
                   worstF, idxP, (int)(idxP*10.f+0.5f));
            ok("l'économie est seedée du vrai pays et son indice tient dans [0..10]",
               idxR>=0.f && idxR<=10.f && idxP>=0.f && idxP<=10.f);
            ok("la richesse SUIT la géographie (meilleur flux → meilleure prospérité)",
               idxR >= idxP);
        } else { ok("(monde trop homogène pour le test d'intégration — ignoré)", true);
                 ok("(idem)", true); }
        free(econ);
    } else { ok("(OOM econ — ignoré)", true); ok("(idem)", true); }

    /* ═══ §CAPITALE : bâtiment obligatoire AMÉLIORABLE + mobilité de classe ═══ */
    printf("\n── §capitale : capitale obligatoire/améliorable (pop-gatée, gratuite) + mobilité de classe ──\n");
    {
        LaborEcon *ce = malloc(sizeof(LaborEcon));
        if (ce){
            labor_init(ce, w);
            labor_seed_start(ce, 0);
            LProvince *cp = &ce->prov[0];
            /* 1. OBLIGATOIRE + STATUT issu du TIER. */
            ok("capitale OBLIGATOIRE ; le tier de départ suit la pop débloquée (4000→4 ; une fondation 500→1)",
               cp->cap_tier==capitale_max_tier(4000) && cp->cap_tier>=1 && capitale_max_tier(500)==1);
            ok("le STATUT vient du TIER : tier 1 « Hameau », 5 « Cité », 7 « Mégapole »",
               !strcmp(capitale_status(1),"Hameau") && !strcmp(capitale_status(5),"Cité")
               && !strcmp(capitale_status(7),"Mégapole"));
            /* 2. la POP PLAFONNE le tier ; l'amélioration est GRATUITE (pop-gatée). */
            ok("la population PLAFONNE le tier (500→1, 4000→4, 10000→7)",
               capitale_max_tier(500)==1 && capitale_max_tier(4000)==4 && capitale_max_tier(10000)==7);
            /* pop suffisante : l'amélioration RÉUSSIT et monte le tier (gratuite). */
            cp->pop=4000; cp->cap_tier=1;
            bool up=capitale_upgrade(cp,ce);
            printf("   amélioration tier 1→2 (pop=4000) : %s ; tier=%d\n",
                   up?"réussie":"échouée", cp->cap_tier);
            ok("pop suffisante → capitale_upgrade RÉUSSIT et monte le tier (gratuit)",
               up && cp->cap_tier==2);
            /* pop trop faible : l'amélioration ÉCHOUE au plafond pop. */
            cp->pop=500; cp->cap_tier=1;
            ok("la POP plafonne : 500 hab ne débloque RIEN au-delà du tier 1", !capitale_upgrade(cp,ce));
            /* 3-5. PAQUETS DE 100 + GATING + productivité. */
            ok("Nobles par PAQUETS de 100 : l'admin emploie tier·100 (tier 3 → 300)", capitale_admin_pop(3)==300);
            ok("GATING : 0 paquet noble → 0 logement, +0 %, MÊME à haut tier",
               capitale_housing(5,0)==0 && capitale_prodmult(5,0)==1.f);
            ok("délivrance au PRORATA : 1 paquet/tier 5 → 1000 log. +5 % ; 3 paquets → 3000 log. +15 %",
               capitale_housing(5,100)==1000 && capitale_prodmult(5,100)==1.05f
               && capitale_housing(5,300)==3000 && capitale_prodmult(5,300)>1.149f);
            /* 6. MOBILITÉ : les classes ÉMERGENT des emplois (par 100). */
            cp->pop=3000; cp->cap_tier=3; cp->n_bld=0;
            capitale_mobility_tick(cp);
            ok("sans atelier : 0 Bourgeois ; les Nobles ÉMERGENT de la capitale (tier 3 → 300)",
               cp->pop_by_class[LAB_ARTISAN]==0 && cp->pop_by_class[LAB_ELITE]==300);
            cp->bld[0]=(LBuilding){LB_WORKSHOP,2,2, 0 }; cp->n_bld=1;
            capitale_mobility_tick(cp);
            printf("   3000 hab, capitale tier 3 + 1 atelier : Nobles %ld · Bourgeois %ld · Journaliers %ld\n",
                   cp->pop_by_class[LAB_ELITE], cp->pop_by_class[LAB_ARTISAN], cp->pop_by_class[LAB_LABORER]);
            ok("ouvrir un atelier fait ÉMERGER des Bourgeois (par 100) ; la somme reste le pool",
               cp->pop_by_class[LAB_ARTISAN]==200 &&
               cp->pop_by_class[LAB_ELITE]+cp->pop_by_class[LAB_ARTISAN]+cp->pop_by_class[LAB_LABORER]==3000);
            /* DÉFENSE passive : un niveau de défense par tier (siège plus long, pas de bonus combat). */
            ok("chaque tier de capitale apporte un niveau de DÉFENSE provinciale passive",
               capitale_defense(1)==1 && capitale_defense(7)==7);
            /* AGITATION : la pop SANS logement/service monte la grogne. */
            cp->pop=5000; cp->house_cap=2000; cp->serv_cap=5000;
            ok("la pop SANS LOGEMENT (surpeuplement) compte comme mal-lotie → agitation",
               capitale_unhoused(cp)==3000 && capitale_unserved(cp)==0 && capitale_unrest(cp)>0.59f);
            cp->house_cap=5000; cp->serv_cap=5000;
            ok("bien logée ET servie : aucune grogne de capacité (unrest 0)",
               capitale_unhoused(cp)==0 && capitale_unserved(cp)==0 && capitale_unrest(cp)==0.f);
            free(ce);
        } else ok("(OOM capitale — ignoré)", true);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(e);
    return g_fail?1:0;
}
