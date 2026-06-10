/*
 * scps_labor.c — l'économie des populations (voir scps_labor.h)
 *
 * La prod scale sur les JOBS REMPLIS ; les sorties LISENT la géo. Toutes les
 * constantes sont calées sur les ancres du cahier : 4000 pop, +4 nourriture
 * plancher, 200/200/200 au départ, suite de jobs 100→1000.
 */
#include "scps_labor.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- Calibrage (surface d'équilibrage) -------------------------------- */
#define COLLECTOR_BASE   4.0f   /* nourriture/slot, PLANCHER géo-indépendant */
#define COLLECTOR_GEO    4.0f   /* bonus de fertilité (×[0..1]) — ne retire jamais la base */
#define GRANARY_BASE     6.0f   /* grenier : lit la fertilité */
#define FOOD_PER_SLOT    1.0f   /* 100 pop EMPLOYÉE consomment 1 nourriture (§2) */
#define MARKET_BASE      3.0f   /* or = MARKET_BASE × flux × jobs (désert→0.3, floris→15) */
#define EXTRACT_BASE     5.0f   /* brute/slot × présence de ressource */
#define WORKSHOP_BASE    1.0f   /* matériaux/slot (gaté par les intrants) */
#define BASE_PRICE       1.0f
#define PRICE_MIN        0.5f
#define PRICE_MAX        5.0f
/* E0.3 — taxes par tête et par JOUR, créditées au trésor unique à chaque tick.
 * Recalées ÷10 (l'ancien barème, jamais appelé, aurait noyé le marché) : cible
 * ~60-70 % du revenu early en taxes, le reste venant des marchés bâtis. */
#define TAX_LABORER      0.001f
#define TAX_ARTISAN      0.003f
#define TAX_ELITE        0.008f
/* E0.3 — le SOLDE : 0.5 or + 1 nourriture (ration de campagne, EN SUS de la
 * bouche universelle) par 100 enrôlés et par jour. Démobiliser prend son sens. */
#define SOLDE_GOLD_PER100 0.5f
#define SOLDE_FOOD_PER100 1L
#define COLONIZE_MAT     100
#define COLONIZE_FOOD    50
#define POP_DEV_BASE     1.0f

static const int LEVEL_JOBS[6] = { 100,100,200,300,500,1000 };  /* capacité AJOUTÉE par niveau */

/* La chaîne de matériaux (§4) : extraire A (job) + extraire B (job) + atelier
 * qui combine (job) → la sortie. 300 pop = les trois étages. */
typedef struct { LRes in_a, in_b, out; int stages_pop; } Recipe;
static const Recipe RECIPES[] = {
    /* P3.16 — plus de « matériaux » : l'atelier RAFFINE des outils (bois+métal).
     * La construction consomme désormais des ressources RÉELLES + or, pas un blob. */
    { LR_BOIS,   LR_METAL,    LR_OUTILS,    300 },
};
#define N_RECIPES ((int)(sizeof(RECIPES)/sizeof(RECIPES[0])))

static inline float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }

/* ===================================================================== */
/* JOBS & NIVEAUX (§9)                                                    */
/* ===================================================================== */
int building_job_capacity_pop(int level){
    if (level<0) level=0;
    if (level>5) level=5;
    int s=0; for (int i=0;i<=level;i++) s+=LEVEL_JOBS[i]; return s;   /* niv.5 = 2200 */
}
int building_job_slots(int level){ return building_job_capacity_pop(level)/POP_PER_SLOT; }

/* ===================================================================== */
/* LA CAPITALE & LA MOBILITÉ DE CLASSE (§capitale)                        */
/* ===================================================================== */
#define CAP_ADMIN_PER_TIER  100   /* pop de Nobles à l'administration, par tier (1 paquet/tier) */
#define CAP_PROD_PER_TIER   0.05f /* +5 % de productivité par tier SERVI */

/* Tier que la POPULATION débloque (plafond) — la pop OUVRE, la recette PAIE. */
int capitale_max_tier(long pop){
    if (pop>=10000) return 7;
    if (pop>= 8000) return 6;
    if (pop>= 5000) return 5;
    if (pop>= 4000) return 4;
    if (pop>= 3000) return 3;
    if (pop>= 2000) return 2;
    return 1;                       /* toute province : tier 1 dès la fondation */
}
/* Le STATUT d'urbanisation VIENT DU TIER bâti (pas seulement de la pop). */
const char *capitale_status(int tier){
    static const char *S[8]={ "Hameau","Hameau","Village","Bourg","Ville","Cité","Métropole","Mégapole" };
    if (tier<1) tier=1;
    if (tier>7) tier=7;
    return S[tier];
}
/* DÉFENSE provinciale passive : un niveau par tier (allonge le SIÈGE comme un
 * rempart — mais SANS le bonus défenseur au combat, cf. spec). */
int capitale_defense(int tier){ return tier<0 ? 0 : tier; }
long capitale_admin_pop(int tier){ return (long)tier * CAP_ADMIN_PER_TIER; }
/* Logement/service délivré : min(paquets de Nobles en poste, tier) × 1000 (gaté). */
long capitale_housing(int tier, long admin_pop){
    long packs = admin_pop / POP_PER_SLOT;
    long actifs = packs < tier ? packs : tier;
    if (actifs<0) actifs=0;
    return actifs * 1000;
}
float capitale_prodmult(int tier, long admin_pop){
    long packs = admin_pop / POP_PER_SLOT;
    long actifs = packs < tier ? packs : tier;
    if (actifs<0) actifs=0;
    return 1.f + CAP_PROD_PER_TIER * (float)actifs;
}
/* Recette d'amélioration vers `to_tier` : DE PLUS EN PLUS PRÉCIEUSE (bois → métal
 * → outils ; les paliers exotiques approximés par l'outil, le plus précieux du
 * module labor). Même BuildCost-logique que le reste. */
CapCost capitale_upgrade_cost(int to_tier){
    switch (to_tier){
        case 2:  return (CapCost){ LR_BOIS,  LR_BOIS,   400, 0   };   /* bois */
        case 3:  return (CapCost){ LR_BOIS,  LR_METAL,  400, 200 };   /* bois + métal */
        case 4:  return (CapCost){ LR_METAL, LR_OUTILS, 400, 200 };   /* métal + outils */
        case 5:  return (CapCost){ LR_METAL, LR_OUTILS, 600, 400 };   /* + (joaillerie) */
        case 6:  return (CapCost){ LR_OUTILS,LR_METAL,  800, 600 };   /* + (fer céleste) */
        case 7:  return (CapCost){ LR_OUTILS,LR_OUTILS, 1200,0   };   /* + (essence) : le plus précieux */
        default: return (CapCost){ LR_BOIS,  LR_BOIS,   200, 0   };   /* tier 1 : fondation */
    }
}
bool capitale_upgrade(LProvince *p, LaborEcon *e){
    if (!p || !e) return false;
    int maxt = capitale_max_tier(p->pop);
    if (p->cap_tier >= maxt) return false;             /* la pop ne débloque pas plus haut */
    int to = p->cap_tier + 1;
    CapCost c = capitale_upgrade_cost(to);
    if (e->stock[c.a] < c.qa) return false;            /* recette non payable (produire d'abord) */
    if (c.qb>0 && e->stock[c.b] < c.qb) return false;
    e->stock[c.a] -= c.qa;
    if (c.qb>0) e->stock[c.b] -= c.qb;
    p->cap_tier = to;                                  /* PAYÉ → tier monté */
    return true;
}
/* Les classes ÉMERGENT des emplois (par paquets de 100) ; la capitale délivre
 * logement/services/productivité au prorata des Nobles en poste. N'achète rien. */
void capitale_mobility_tick(LProvince *p){
    if (!p) return;
    if (p->cap_tier < 1) p->cap_tier = 1;              /* obligatoire : toujours ≥ tier 1 */
    long pool = p->pop; if (pool<0) pool=0;
    long noble_jobs   = (capitale_admin_pop(p->cap_tier)/100)*100;   /* admin, par 100 */
    long artisan_jobs = 0;
    for (int b=0;b<p->n_bld;b++)
        if (p->bld[b].type==LB_WORKSHOP) artisan_jobs += (long)p->bld[b].jobs_filled * POP_PER_SLOT;
    artisan_jobs = (artisan_jobs/100)*100;
    long elites   = noble_jobs   < pool ? noble_jobs : pool;          /* promotion → Nobles */
    long rem      = pool - elites; if (rem<0) rem=0;
    long artisans = artisan_jobs < rem ? artisan_jobs : rem;          /* → Bourgeois */
    long laborers = pool - elites - artisans;                         /* le reste : Journaliers */
    p->pop_by_class[LAB_ELITE]   = elites;
    p->pop_by_class[LAB_ARTISAN] = artisans;
    p->pop_by_class[LAB_LABORER] = laborers;
    p->house_cap = capitale_housing(p->cap_tier, elites);             /* gaté par les paquets nobles */
    p->serv_cap  = capitale_housing(p->cap_tier, elites);
    p->prod_mult = capitale_prodmult(p->cap_tier, elites);
}

long capitale_unhoused(const LProvince *p){ long u=p->pop-p->house_cap; return u>0?u:0; }
long capitale_unserved(const LProvince *p){ long u=p->pop-p->serv_cap; return u>0?u:0; }
float capitale_unrest(const LProvince *p){
    if (!p || p->pop<=0) return 0.f;
    long worst = capitale_unhoused(p);
    long us = capitale_unserved(p);
    if (us>worst) worst=us;                       /* le pire des deux manques */
    float f = (float)worst/(float)p->pop;
    return f<0.f?0.f:(f>1.f?1.f:f);
}

/* ===================================================================== */
/* labor_init — RELIT la géographie du worldgen en agrégats par province  */
/* ===================================================================== */
static bool biome_forest(Biome b){ return b==BIO_FOREST||b==BIO_WOODS||b==BIO_JUNGLE||b==BIO_MANGROVE; }
static bool biome_hills (Biome b){ return b==BIO_HILLS||b==BIO_HIGHLANDS; }
static bool biome_mtn   (Biome b){ return b==BIO_MOUNTAINS||b==BIO_PEAK||b==BIO_VOLCANO; }

void labor_init(LaborEcon *e, const World *w){
    memset(e, 0, sizeof(*e));
    e->market.price = BASE_PRICE; e->market.supply = 1.f;
    labor_publish_capitals(NULL);   /* E0.4 : registre des capitales bâties — RAZ par partie */

    /* accumulateurs par province */
    static double fsum[SCPS_MAX_PROV]; static int pcells[SCPS_MAX_PROV], rmax[SCPS_MAX_PROV];
    static int nforest[SCPS_MAX_PROV], nhill[SCPS_MAX_PROV], nmtn[SCPS_MAX_PROV];
    memset(fsum,0,sizeof fsum); memset(pcells,0,sizeof pcells); memset(rmax,0,sizeof rmax);
    memset(nforest,0,sizeof nforest); memset(nhill,0,sizeof nhill); memset(nmtn,0,sizeof nmtn);

    for (int i=0;i<SCPS_N;i++){
        const Cell *c=&w->cell[i]; int pr=c->province;
        if (pr<0||pr>=SCPS_MAX_PROV) continue;
        pcells[pr]++; fsum[pr]+=c->fertility;
        if (c->river>rmax[pr]) rmax[pr]=c->river;
        if (biome_forest(c->biome)) nforest[pr]++;
        if (biome_hills(c->biome))  nhill[pr]++;
        if (biome_mtn(c->biome))    nmtn[pr]++;
    }
    for (int pr=0; pr<w->n_provinces && pr<SCPS_MAX_PROV; pr++){
        int n=pcells[pr]; if (n<=0) continue;
        float fert  = (float)(fsum[pr]/n);
        float riv01 = (float)rmax[pr]/255.f;
        bool  coast = w->province[pr].coastal;
        e->g_fert[pr] = fert;
        /* Le FLUX commercial (le carrefour) : la géographie des flux fait le
         * marchand — côte + fleuve + bonne terre. Désert intérieur ~0.1. */
        e->g_flow[pr] = 0.1f + (coast?1.8f:0.f) + riv01*1.5f + fert*1.6f;
        /* Présence de ressource [0..1] — lue des biomes + ressource dominante. */
        e->g_pres[pr][LR_BOIS]     = clampf((float)nforest[pr]/n*2.f, 0.f, 1.f);
        e->g_pres[pr][LR_ARGILE]   = 0.15f + clampf(riv01*1.4f, 0.f, 1.f);  /* P3.17 : 0.15 PARTOUT, jusqu'à 1.15 en plaine alluviale (fleuve) */
        e->g_pres[pr][LR_CALCAIRE] = clampf((float)nhill[pr]/n*2.f, 0.f, 1.f);
        e->g_pres[pr][LR_PIERRE]   = clampf((float)(nhill[pr]+nmtn[pr])/n*1.5f, 0.f, 1.f);
        Resource rr=w->province[pr].resource;
        float metal = (rr==RES_IRON||rr==RES_COPPER||rr==RES_GOLD||rr==RES_COAL||rr==RES_PRECIOUS_METAL)?0.8f:0.f;
        if ((float)nmtn[pr]/n*1.5f > metal) metal=clampf((float)nmtn[pr]/n*1.5f,0.f,1.f);
        e->g_pres[pr][LR_METAL]    = metal;
    }
}

/* ===================================================================== */
/* DÉPART CANONIQUE (§1, refondu E0.5)                                    */
/* ===================================================================== */
/* le plus petit niveau dont la capacité couvre `slots` (plafonné à 5) */
static int collector_level_for(long slots){
    for (int l=0;l<=5;l++) if ((long)building_job_slots(l)>=slots) return l;
    return 5;
}
/* E0.5/E0.2 — pose 1-3 collecteurs DIMENSIONNÉS pour nourrir `pop` bouches
 * (pop/100 nourriture/j) à la fertilité du lieu : ~15-20 % de la pop aux champs
 * à fertilité médiane. Les jobs sont pré-remplis (société établie, pas un camp). */
static void seed_collectors(LaborEcon *e, LProvince *p, long pop){
    float per_slot = COLLECTOR_BASE + COLLECTOR_GEO*e->g_fert[p->prov];
    if (per_slot < 1.f) per_slot = 1.f;
    long need = (long)ceilf((float)(pop/POP_PER_SLOT) / per_slot);   /* slots à remplir */
    if (need < 1) need = 1;
    long max_slots = pop/POP_PER_SLOT;            /* on ne staffe pas plus que la pop */
    if (need > max_slots) need = max_slots;
    for (int k=0; k<3 && need>0 && p->n_bld<LAB_BUILDINGS_PER_PROV; k++){
        int  lvl   = collector_level_for(need);
        long fill  = building_job_slots(lvl); if (fill>need) fill=need;
        p->bld[p->n_bld++] = (LBuilding){ LB_COLLECTOR, lvl, (int)fill, 0 };
        need -= fill;
    }
}
/* la meilleure EXTRACTION que la géo du lieu offre (motif de seed_from_world) */
static LBuildType best_extraction(const LaborEcon *e, int pid){
    LBuildType ex = LB_QUARRY;
    if      (e->g_pres[pid][LR_METAL] >0.3f) ex=LB_MINE;
    else if (e->g_pres[pid][LR_BOIS]  >0.3f) ex=LB_SAWMILL;
    else if (e->g_pres[pid][LR_ARGILE]>0.3f) ex=LB_CLAYPIT;
    return ex;
}
void labor_seed_start(LaborEcon *e, int prov0){
    e->n_prov=1;
    LProvince *p=&e->prov[0];
    memset(p,0,sizeof(*p));
    p->prov=prov0; p->region=-1; p->colonized=true;
    p->pop=4000;
    p->cap_tier=capitale_max_tier(p->pop); p->prod_mult=1.f;   /* capitale développée (la pop débloque) */
    p->pop_by_class[LAB_LABORER]=3200; p->pop_by_class[LAB_ARTISAN]=600; p->pop_by_class[LAB_ELITE]=200;
    /* collecteurs DIMENSIONNÉS (la bouche est totale désormais) + l'extraction que
     * la géo OFFRE + un atelier gaté intrants (la dépendance au commerce est voulue).
     * PAS de marché : le commerce early passe par les Centres commerciaux (P3.20). */
    seed_collectors(e, p, p->pop);
    if (p->n_bld<LAB_BUILDINGS_PER_PROV)
        p->bld[p->n_bld++]=(LBuilding){ best_extraction(e,prov0), 1, 2, 0 };
    if (p->n_bld<LAB_BUILDINGS_PER_PROV)
        p->bld[p->n_bld++]=(LBuilding){ LB_WORKSHOP, 0, 1, 0 };
    e->stock[LR_FOOD]=200; e->stock[LR_GOLD]=200; e->stock[LR_BOIS]=200; e->stock[LR_OUTILS]=100;
    e->treasury=200;
}

/* ===================================================================== */
/* INTÉGRATION — seeder l'économie depuis un VRAI pays du monde           */
/* ===================================================================== */
void labor_seed_from_world(LaborEcon *e, const World *w, const WorldEconomy *econ, int cid){
    /* on garde la géo précalculée (labor_init) ; on (ré)installe l'économie. */
    e->n_prov=0;
    memset(e->stock,0,sizeof e->stock); memset(e->flow,0,sizeof e->flow);
    e->stock[LR_FOOD]=200; e->stock[LR_GOLD]=200; e->stock[LR_BOIS]=200; e->stock[LR_OUTILS]=100;
    e->market.supply=1.f; e->market.price=BASE_PRICE; e->treasury=200;
    e->tax_acc=0.f; e->solde_acc=0.f;

    for (int r=0; r<econ->n_regions && e->n_prov<LAB_MAX_PROV; r++){
        if (econ->region[r].owner!=cid || !econ->region[r].culture.settled) continue;
        int pid = (r<w->n_regions) ? w->region[r].province_ids[0] : -1;
        if (pid<0 || pid>=w->n_provinces) continue;
        long pop = (long)(econ->region[r].strata[CLASS_LABORER].pop
                        + econ->region[r].strata[CLASS_BOURGEOIS].pop
                        + econ->region[r].strata[CLASS_ELITE].pop);
        if (pop<100) pop=100;
        LProvince *p=&e->prov[e->n_prov++];
        memset(p,0,sizeof(*p));
        p->prov=pid; p->region=r; p->colonized=true; p->pop=pop;   /* E0.1 : adossée à SA région (resync) */
        p->cap_tier=capitale_max_tier(pop); p->prod_mult=1.f;   /* capitale développée que la pop débloque */
        p->pop_by_class[LAB_LABORER]=pop*8/10;       /* repli ; les classes ÉMERGENT au 1er tick (§5) */
        p->pop_by_class[LAB_ARTISAN]=pop*15/100;
        p->pop_by_class[LAB_ELITE]  =pop - p->pop_by_class[LAB_LABORER] - p->pop_by_class[LAB_ARTISAN];
        /* Bâtiments sur la GÉO réelle : collecteurs DIMENSIONNÉS (E0.2 : la bouche
         * est la pop totale) + marché (ville établie) + extraction du lieu. */
        seed_collectors(e, p, pop);
        if (p->n_bld<LAB_BUILDINGS_PER_PROV)
            p->bld[p->n_bld++]=(LBuilding){ LB_MARKET, 0, 1, 0 };
        if (p->n_bld<LAB_BUILDINGS_PER_PROV)
            p->bld[p->n_bld++]=(LBuilding){ best_extraction(e,pid), 0, 1, 0 };
    }
}

float labor_prosperity_index(const LaborEcon *e){
    long pop = labor_pop_total(e); if (pop<1) pop=1;
    float per100  = (float)pop/100.f;
    float foodsec = (labor_food_balance(e) >= 0) ? 3.0f : 0.0f;   /* le pain d'abord */
    float gold_pc = (float)e->flow[LR_GOLD]   / per100;          /* revenu par tête */
    float out_pc  = (float)e->flow[LR_OUTILS] / per100;          /* P3.16 : OUTILS par tête (le raffiné) */
    float idx = foodsec + clampf(gold_pc*5.0f, 0.f, 4.f) + clampf(out_pc*8.0f, 0.f, 3.f);
    return clampf(idx, 0.f, 10.f);
}

/* ===================================================================== */
/* SORTIES LUES DE LA GÉO (§5)                                            */
/* ===================================================================== */
float province_trade_flow(const LaborEcon *e, int prov){
    return (prov>=0&&prov<SCPS_MAX_PROV) ? e->g_flow[prov] : 0.f;
}
float province_fertility(const LaborEcon *e, int prov){
    return (prov>=0&&prov<SCPS_MAX_PROV) ? e->g_fert[prov] : 0.f;
}
float labor_market_output(const LaborEcon *e, int prov, int jobs_filled){
    return MARKET_BASE * province_trade_flow(e,prov) * (float)jobs_filled;
}
static float per_job_output(const LaborEcon *e, int wprov, LBuildType t, LRes *out){
    switch(t){
        case LB_COLLECTOR: *out=LR_FOOD;      return COLLECTOR_BASE + COLLECTOR_GEO*e->g_fert[wprov];
        case LB_GRANARY:   *out=LR_FOOD;      return GRANARY_BASE * e->g_fert[wprov];
        case LB_MARKET:    *out=LR_GOLD;      return MARKET_BASE * e->g_flow[wprov];
        case LB_SAWMILL:   *out=LR_BOIS;      return EXTRACT_BASE * e->g_pres[wprov][LR_BOIS];
        case LB_CLAYPIT:   *out=LR_ARGILE;    return EXTRACT_BASE * e->g_pres[wprov][LR_ARGILE];
        case LB_QUARRY:    *out=LR_CALCAIRE;  return EXTRACT_BASE * e->g_pres[wprov][LR_CALCAIRE];
        case LB_MINE:      *out=LR_METAL;     return EXTRACT_BASE * e->g_pres[wprov][LR_METAL];
        case LB_WORKSHOP:  *out=LR_OUTILS;    return WORKSHOP_BASE;   /* P3.16 : l'atelier raffine des OUTILS (gaté par les intrants) */
        default:           *out=LR_COUNT;     return 0.f;
    }
}
float labor_building_output(const LaborEcon *e, int prov, int bld_idx, LRes *out_res){
    LRes dummy; if(!out_res) out_res=&dummy;
    if (prov<0||prov>=e->n_prov||bld_idx<0||bld_idx>=e->prov[prov].n_bld){ *out_res=LR_COUNT; return 0.f; }
    const LProvince *p=&e->prov[prov];
    const LBuilding *b=&p->bld[bld_idx];
    /* LA RÈGLE : per-job (lu de la géo) × JOBS REMPLIS. Vide → 0. */
    return per_job_output(e, p->prov, b->type, out_res) * (float)b->jobs_filled;
}

/* ===================================================================== */
/* NOURRITURE & FAMINE (§2)                                               */
/* ===================================================================== */
long labor_food_collected(const LaborEcon *e){
    double f=0.0;
    for (int i=0;i<e->n_prov;i++) for (int b=0;b<e->prov[i].n_bld;b++){
        const LBuilding *bd=&e->prov[i].bld[b];
        if (bd->type==LB_COLLECTOR||bd->type==LB_GRANARY){
            LRes o; f += per_job_output(e, e->prov[i].prov, bd->type, &o)*(float)bd->jobs_filled;
        }
    }
    return (long)(f+0.5);
}
long labor_food_consumed(const LaborEcon *e){
    /* E0.2 — LA BOUCHE UNIQUE : pop TOTALE/100 × 1.0/j. Employés, libres, armée,
     * admin — TOUS mangent. La famine redevient possible. */
    return (long)((float)(labor_pop_total(e)/POP_PER_SLOT)*FOOD_PER_SLOT);
}
void labor_army_upkeep(const LaborEcon *e, long *gold_j, long *food_j){
    long packs = labor_pop_in_army(e)/POP_PER_SLOT;
    if (gold_j) *gold_j = (long)(SOLDE_GOLD_PER100*(float)packs + 0.5f);
    if (food_j) *food_j = packs*SOLDE_FOOD_PER100;
}
long labor_food_balance(const LaborEcon *e){
    long ration; labor_army_upkeep(e,NULL,&ration);
    return labor_food_collected(e) - labor_food_consumed(e) - ration;
}

/* ===================================================================== */
/* MARCHÉ & PRIX DYNAMIQUE (§7)                                           */
/* ===================================================================== */
float labor_material_price(const LaborEcon *e){
    return BASE_PRICE * clampf(e->market.demand / fmaxf(e->market.supply,1.f), PRICE_MIN, PRICE_MAX);
}
long labor_pump_market(LaborEcon *e, LRes res, long amount){
    if (amount<=0 || res<0 || res>=LR_COUNT || res==LR_GOLD) return 0;
    float price = labor_material_price(e);
    long cost = (long)(amount*price + 0.5f);
    e->stock[res]     += amount;       /* E0.6 : la pompe livre la ressource DEMANDÉE */
    e->stock[LR_GOLD] -= cost; e->treasury=e->stock[LR_GOLD];
    e->market.demand  += (float)amount;   /* pomper TIRE la demande → le prix monte */
    return cost;
}

/* ===================================================================== */
/* POPULATION — le POOL et sa répartition (topbar)                        */
/* ===================================================================== */
long labor_pop_total(const LaborEcon *e){
    long t=0; for (int i=0;i<e->n_prov;i++) t+=e->prov[i].pop; return t;
}
long labor_pop_employed(const LaborEcon *e){
    long slots=0;
    for (int i=0;i<e->n_prov;i++) for (int b=0;b<e->prov[i].n_bld;b++)
        slots += e->prov[i].bld[b].jobs_filled;
    return slots * POP_PER_SLOT;          /* affectés au travail — TOUJOURS dans le pool */
}
long labor_pop_in_army(const LaborEcon *e){
    long a=0; for (int i=0;i<e->n_prov;i++) a+=e->prov[i].pop_in_army; return a;
}
PopBreakdown labor_pop_breakdown(const LaborEcon *e){
    PopBreakdown k;
    k.total   = labor_pop_total(e);
    k.in_jobs = labor_pop_employed(e);
    k.in_army = labor_pop_in_army(e);
    k.free    = k.total - k.in_jobs - k.in_army;   /* le reste non affecté */
    if (k.free<0) k.free=0;                          /* sur-affectation : bornée */
    return k;
}
void labor_print_topbar(const LaborEcon *e){
    PopBreakdown k=labor_pop_breakdown(e);
    printf("   TOPBAR  Or %ld (%+ld/j) · Nourriture %ld (%+ld/j) · Outils %ld (%+ld/j)\n",
           e->stock[LR_GOLD], e->flow[LR_GOLD], e->stock[LR_FOOD], e->flow[LR_FOOD],
           e->stock[LR_OUTILS], e->flow[LR_OUTILS]);
    printf("           Pop %ld :  libre %ld · en job %ld · en armée %ld\n",
           k.total, k.free, k.in_jobs, k.in_army);
}

/* ===================================================================== */
/* TAXES (§8)                                                             */
/* ===================================================================== */
float labor_taxes(const LProvince *p){
    return p->pop_by_class[LAB_LABORER]*TAX_LABORER
         + p->pop_by_class[LAB_ARTISAN]*TAX_ARTISAN
         + p->pop_by_class[LAB_ELITE]  *TAX_ELITE;
}

/* ===================================================================== */
/* COLONISATION (§6)                                                      */
/* ===================================================================== */
ColonizeCost labor_colonize_cost(const LaborEcon *e, int prov){
    ColonizeCost c; c.materials=COLONIZE_MAT; c.food=COLONIZE_FOOD;
    /* une bonne terre coûte plus cher à réclamer (la géo, encore). */
    if (prov>=0&&prov<SCPS_MAX_PROV) c.materials += (long)(e->g_flow[prov]*15.f);
    return c;
}
bool labor_can_colonize(const LaborEcon *e, ColonizeCost cost){
    return e->stock[LR_BOIS]>=cost.materials && e->stock[LR_FOOD]>=cost.food;
}
bool labor_colonize(LaborEcon *e, int prov){
    if (e->n_prov>=LAB_MAX_PROV) return false;
    ColonizeCost cost=labor_colonize_cost(e,prov);
    if (!labor_can_colonize(e,cost)) return false;
    e->stock[LR_BOIS]-=cost.materials; e->stock[LR_FOOD]-=cost.food;
    LProvince *p=&e->prov[e->n_prov++];
    memset(p,0,sizeof(*p));
    p->prov=prov; p->region=-1; p->colonized=true; p->pop=500; p->pop_by_class[LAB_LABORER]=500;
    p->cap_tier=1; p->prod_mult=1.f;                /* la capitale obligatoire dès la colonie (§1) */
    return true;
}

/* ===================================================================== */
/* DÉVELOPPEMENT DE LA POP (§10)                                          */
/* ===================================================================== */
bool province_fully_colonized(const LProvince *p){
    return p->colonized && p->n_bld>=LAB_BUILDINGS_PER_PROV;
}
float labor_pop_dev_speed(const LProvince *p, int prosperity_0_100){
    float s=POP_DEV_BASE;
    if (province_fully_colonized(p) && prosperity_0_100>=100) s*=1.15f;   /* plein dev (§10) */
    return s;
}

/* ===================================================================== */
/* PRODUCTION & CHAÎNES (§4) — le cœur de la boucle                       */
/* ===================================================================== */
/* Un atelier combine : pour chaque slot rempli, trouve une recette dont les
 * DEUX intrants sont en stock, les consomme → 1 sortie. Sans intrants, rien. */
static long run_workshop(LaborEcon *e, int jobs){
    long made=0;
    for (int j=0;j<jobs;j++){
        for (int r=0;r<N_RECIPES;r++){
            const Recipe *R=&RECIPES[r];
            if (e->stock[R->in_a]>=1 && e->stock[R->in_b]>=1){
                e->stock[R->in_a]-=1; e->stock[R->in_b]-=1; e->stock[R->out]+=1; made++;
                break;
            }
        }
    }
    return made;
}

/* E0.1 — LA CROISSANCE A QUITTÉ LABOR. Un seul propriétaire : la démographie du
 * monde (econ_tick fait croître les strates à 0.5-6 %/AN, modulé nourriture/
 * société/capacité, famine → négatif). labor RELIT — resync mensuel ci-dessous. */
void labor_resync_pop(LaborEcon *e, const WorldEconomy *econ){
    if (!e || !econ) return;
    for (int i=0;i<e->n_prov;i++){
        LProvince *p=&e->prov[i];
        if (p->region<0 || p->region>=econ->n_regions) continue;   /* autonome : pop figée */
        const RegionEconomy *re=&econ->region[p->region];
        long pop=(long)(re->strata[CLASS_LABORER].pop
                      + re->strata[CLASS_BOURGEOIS].pop
                      + re->strata[CLASS_ELITE].pop);
        if (pop<0) pop=0;
        p->pop=pop;
        if (p->pop_in_army>p->pop) p->pop_in_army=p->pop;   /* l'armée ne dépasse pas le pool */
        /* la pop a pu DÉCROÎTRE : les emplois excédentaires se vident (du dernier
         * bâtiment au premier) — un slot ne travaille pas sans bras. */
        long max_slots=p->pop/POP_PER_SLOT, have=0;
        for (int b=0;b<p->n_bld;b++) have+=p->bld[b].jobs_filled;
        for (int b=p->n_bld-1;b>=0 && have>max_slots;b--){
            long drop=have-max_slots;
            if (drop>p->bld[b].jobs_filled) drop=p->bld[b].jobs_filled;
            p->bld[b].jobs_filled-=(int)drop; have-=drop;
        }
        /* les classes ré-émergeront au prochain tick (capitale_mobility_tick) */
    }
}

/* E0.4 — le registre des capitales BÂTIES (tier payé, pas tier débloqué) : publié
 * par le viewer après le tick labor, LU par le moteur de révolte (mal-logés/
 * mal-servis réels du joueur). -1 = région non gouvernée par le modèle labor. */
static int8_t g_cap_tier_reg[SCPS_MAX_REG];
void labor_publish_capitals(const LaborEcon *e){
    memset(g_cap_tier_reg, -1, sizeof g_cap_tier_reg);
    if (!e) return;
    for (int i=0;i<e->n_prov;i++){
        const LProvince *p=&e->prov[i];
        if (p->region>=0 && p->region<SCPS_MAX_REG)
            g_cap_tier_reg[p->region]=(int8_t)(p->cap_tier<1?1:(p->cap_tier>7?7:p->cap_tier));
    }
}
int labor_region_cap_tier(int region){
    return (region>=0 && region<SCPS_MAX_REG)? (int)g_cap_tier_reg[region] : -1;
}

/* P3.19 — main-d'œuvre LIBRE d'une province (pop − armée − admin − déjà employés). */
static long prov_free_pop(const LProvince *p){
    long employed=0; for (int b=0;b<p->n_bld;b++) employed += (long)p->bld[b].jobs_filled*POP_PER_SLOT;
    long admin = capitale_admin_pop(p->cap_tier);
    long fr = p->pop - p->pop_in_army - admin - employed;
    return fr>0?fr:0;
}
/* STAFFING ADDITIF (P3.19) : remplit les slots VIDES avec la main-d'œuvre libre —
 * jamais ne RETIRE un emploi posé (un job est une affectation). Sans pop libre ni
 * slot vide : sans effet. C'est lui qui fait MONTER la prod quand la pop croît ou
 * qu'un bâtiment se développe → les flux VIVENT. */
static void labor_staff(LaborEcon *e){
    for (int i=0;i<e->n_prov;i++){
        LProvince *p=&e->prov[i];
        long free_slots = prov_free_pop(p)/POP_PER_SLOT;
        for (int b=0;b<p->n_bld && free_slots>0;b++){
            int empty = building_job_slots(p->bld[b].level) - p->bld[b].jobs_filled;
            if (empty<=0) continue;
            int take = (free_slots<(long)empty)?(int)free_slots:empty;
            p->bld[b].jobs_filled += take; free_slots -= take;
        }
    }
}
/* DÉVELOPPEMENT (P3.19) : un bâtiment PLEIN, là où il reste de la main-d'œuvre
 * libre ET un surplus alimentaire, monte de niveau PEU À PEU (accumulateur ∝ surplus).
 * Plus de slots → la passe de staffing les remplit → la prod et les flux montent.
 * Un seul bâtiment se développe par province et par tick (croissance organique).
 * E1bis.12 — la montée n'est PLUS gratuite : le niveau n coûte 20·n bois + 10·n
 * argile au stock du PAYS ; le dev-accumulateur reste le TEMPS. Recette non payable
 * → le dev ATTEND AU SEUIL (pas de dette, pas de saut). */
#define DEV_THRESH 900u
static void labor_develop(LaborEcon *e){
    long bal = labor_food_balance(e);                       /* surplus = on peut grandir (la GÂCHE) */
    if (bal<=0) return;
    int inc = 6;                                            /* cadence FIXE ; la demande de travail (pop) PACE le reste */
    for (int i=0;i<e->n_prov;i++){
        LProvince *p=&e->prov[i];
        if (prov_free_pop(p) < POP_PER_SLOT) continue;       /* pas de demande de travail : on ne sur-bâtit pas */
        for (int b=0;b<p->n_bld;b++){
            LBuilding *bd=&p->bld[b];
            if (bd->level>=5) continue;
            if (bd->jobs_filled < building_job_slots(bd->level)) continue;   /* pas encore plein */
            if ((unsigned)bd->dev + (unsigned)inc >= DEV_THRESH){
                long n = bd->level+1;                        /* niveau visé */
                long need_b = 20*n, need_a = 10*n;           /* E1bis.12 : recette de montée */
                if (e->stock[LR_BOIS]>=need_b && e->stock[LR_ARGILE]>=need_a){
                    e->stock[LR_BOIS]-=need_b; e->stock[LR_ARGILE]-=need_a;
                    bd->level++; bd->dev=0;                  /* PAYÉ → niveau monté */
                } else {
                    bd->dev = (uint16_t)DEV_THRESH;          /* le TEMPS est prêt ; on ATTEND la recette (pas de dette) */
                }
            } else bd->dev=(uint16_t)(bd->dev+inc);
            break;                                           /* un seul par province/tick */
        }
    }
}

void labor_tick(LaborEcon *e){
    long before[LR_COUNT]; memcpy(before, e->stock, sizeof before);
    float supply_mat=0.f;
    labor_staff(e);                                          /* P3.19 : la main-d'œuvre libre garnit les slots vides */

    /* 0. CAPITALE : améliorer si la pop le débloque & la recette est payable, puis
     *    faire ÉMERGER les classes des emplois (tier→Nobles, ateliers→Bourgeois) et
     *    délivrer logement/services/productivité (gatés par les Nobles en poste). */
    for (int i=0;i<e->n_prov;i++) capitale_upgrade(&e->prov[i], e);
    for (int i=0;i<e->n_prov;i++) capitale_mobility_tick(&e->prov[i]);

    /* 1. EXTRACTION + collecte + marché : jobs remplis → sorties (lues de la géo),
     *    × la PRODUCTIVITÉ de la capitale (+5 %/tier servi). */
    for (int i=0;i<e->n_prov;i++){
        LProvince *p=&e->prov[i];
        for (int b=0;b<p->n_bld;b++){
            LBuilding *bd=&p->bld[b];
            if (bd->type==LB_WORKSHOP) continue;     /* les ateliers passent en 2 */
            LRes o; float out=per_job_output(e,p->prov,bd->type,&o)*(float)bd->jobs_filled*p->prod_mult;
            if (o<LR_COUNT) e->stock[o]+=(long)(out+0.5f);
        }
    }
    /* 2. ATELIERS : chaînes de matériaux (consomment les bruts extraits). */
    for (int i=0;i<e->n_prov;i++){
        LProvince *p=&e->prov[i];
        for (int b=0;b<p->n_bld;b++) if (p->bld[b].type==LB_WORKSHOP)
            supply_mat += (float)run_workshop(e, p->bld[b].jobs_filled);
    }
    /* 3. NOURRITURE (E0.2) : la collecte a été ajoutée en passe 1 ; on retire LA
     * BOUCHE (pop totale/100) + la RATION de l'armée. Plancher 0 : on ne mange pas
     * ce qui n'existe pas — le déficit se LIT au flux (la famine du joueur). */
    { long ration_g, ration_f; labor_army_upkeep(e,&ration_g,&ration_f);
      e->stock[LR_FOOD] -= labor_food_consumed(e) + ration_f;
      if (e->stock[LR_FOOD]<0) e->stock[LR_FOOD]=0;

      /* 4. TAXES (E0.3) : le trésor unique ENCAISSE chaque jour — par classe, par
       * tête (accumulateurs : les fractions ne se perdent pas). Puis le SOLDE sort. */
      (void)ration_g;   /* le reader arrondit pour l'UI ; le débit exact passe par solde_acc */
      float tax_today=0.f;
      for (int i=0;i<e->n_prov;i++) tax_today += labor_taxes(&e->prov[i]);
      e->tax_acc += tax_today;
      long tg=(long)e->tax_acc; e->tax_acc-=(float)tg;
      e->stock[LR_GOLD] += tg;
      e->solde_acc += SOLDE_GOLD_PER100*(float)(labor_pop_in_army(e)/POP_PER_SLOT);
      long sg=(long)e->solde_acc; e->solde_acc-=(float)sg;
      e->stock[LR_GOLD] -= sg;
      if (e->stock[LR_GOLD]<0) e->stock[LR_GOLD]=0; }   /* l'armée impayée ne crée pas de dette (pas encore de désertion) */

    /* 5. MARCHÉ : l'offre de matériaux du tick met à jour le prix. */
    e->market.supply = fmaxf(1.f, supply_mat);
    e->market.price  = labor_material_price(e);
    e->treasury      = e->stock[LR_GOLD];

    /* 6. (E0.1 : PLUS de croissance ici — la démographie monde possède la pop ;
     * labor_resync_pop la relit chaque mois.) On RE-ÉMERGE les classes, puis dev. */
    for (int i=0;i<e->n_prov;i++) capitale_mobility_tick(&e->prov[i]);
    labor_develop(e);                                       /* P3.19 : les bâtiments pleins se développent (ouvre des slots) */
    for (int r=0;r<LR_COUNT;r++) e->flow[r]=e->stock[r]-before[r];
}

/* ===================================================================== */
/* LIBELLÉS                                                              */
/* ===================================================================== */
const char *lres_name(LRes r){
    static const char *N[LR_COUNT]={ "Nourriture","Or","Bois","Argile",
                                     "Calcaire","Pierre","Métal","Outils" };
    return (r>=0&&r<LR_COUNT)?N[r]:"?";
}
const char *lbuild_name(LBuildType b){
    static const char *N[LB_TYPE_COUNT]={ "—","Collecteur","Grenier","Marché","Scierie",
                                          "Argilière","Carrière","Mine","Atelier" };
    return (b>=0&&b<LB_TYPE_COUNT)?N[b]:"?";
}
