/*
 * chronicle.c — le moteur VIVANT, sans écran : N mondes × M années
 *
 *   make chronicle && ./chronicle [graine_base] [n_mondes=10] [années=200]
 *
 * Fait tourner exactement la boucle du jeu (sim_day du viewer, mais headless) et
 * tient la CHRONIQUE de ce qui émerge : âges qui s'éveillent, guerres, révoltes,
 * empires qui croissent, pays absorbés. Aucune entrée joueur — on regarde le
 * monde vivre par les seuls acteurs IA et le moteur d'ordre.
 */
#define _POSIX_C_SOURCE 199309L   /* PROF : clock_gettime/CLOCK_MONOTONIC visibles sous -std=c99 strict */
#include "scps_tune.h"
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"
#include "scps_statecraft.h"
#include "scps_agency.h"
#include "scps_credit.h"
#include "scps_routes.h"
#include "scps_intertrade.h"
#include "scps_warhost.h"
#include "scps_campaign.h"
#include "scps_navy.h"
#include "scps_diplo.h"
#include "scps_endgame.h"  /* capstone §27 : entropie + 4 fins + merveille */
#include "scps_events.h"
#include "scps_modifier.h"
#include "scps_demography.h"
#include "scps_revolt.h"
#include "scps_missions.h"
#include "scps_factions.h"
#include "scps_labor.h"
#include "scps_ai.h"
#include "scps_heritage.h"
#include "scps_sim.h"       /* le TICK PARTAGÉ : Sim, sim_init, sim_day, regions_of (ex-inline) */
#include "scps_religion.h"  /* RELIGION : télémétrie (root_count, of_country/region, g_religion_count) */
#include "miniz.h"          /* HARNAIS DE DÉTERMINISME : mz_crc32 (vendoré, third_party) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>   /* sqrt : σ du lissage des prix (E3 §16) */
#include <time.h>   /* PROF : horloge monotone (profiler de boucle, OFF par défaut) */


#define CORR_CAPTURED 30   /* §C3 : seuil « polity tenue par une faction » (corr 0-100) */


/* ── Lectures de la chronique ────────────────────────────────────────────── */
/* Pays VIVANTS : ceux qui tiennent ≥1 région. */
static int living_countries(const World *w, const WorldEconomy *e){
    int n=0;
    for (int c=0;c<w->n_countries;c++)
        if (w->country[c].role!=POLITY_UNCLAIMED && regions_of(e,c)>0) n++;
    return n;
}
/* Pactes d'ALLIANCE actifs : paires de pays vivants liés (DIPLO_ALLIED). §D1/D2 :
 * les blocs se nouent (menace partagée RELATIVE) et se dénouent (désuétude/trahison) —
 * ce compteur dit si la diplomatie RESPIRE ou se fige. */
static int active_alliances(const World *w, const WorldEconomy *e, const DiploState *dp){
    int n=0;
    for (int a=0;a<w->n_countries;a++){
        if (w->country[a].role==POLITY_UNCLAIMED || regions_of(e,a)==0) continue;
        for (int b=a+1;b<w->n_countries;b++){
            if (w->country[b].role==POLITY_UNCLAIMED || regions_of(e,b)==0) continue;
            if (diplo_status(dp,a,b)==DIPLO_ALLIED) n++;
        }
    }
    return n;
}
/* Recensement des manufactures PAR TYPE (combien de joailleries, de scieries…) — lit
 * si la construction par rétroaction négative peuple bien la carte (et où ça manque). */
static void print_building_census(const WorldEconomy *e){
    long count[BLD_TYPE_COUNT]; for (int b=0;b<BLD_TYPE_COUNT;b++) count[b]=0;
    for (int r=0;r<e->n_regions;r++){
        const RegionEconomy *re=&e->region[r];
        if (!re->active || !re->colonized) continue;
        for (int i=0;i<re->n_bld;i++)
            if (re->bld[i].type>=0 && re->bld[i].type<BLD_TYPE_COUNT) count[re->bld[i].type]++;
    }
    printf("              manufactures :");
    for (int b=0;b<BLD_TYPE_COUNT;b++) if (count[b]>0) printf(" %ld×%s", count[b], building_name((BuildingType)b));
    printf("\n");
}
/* Satisfaction par CLASSE (pop-pondérée, sur les régions vivantes) — la vraie mesure du
 * bien-être, AVEC la distribution régionale active (≠ econ_scan, sans commerce). */
static void world_class_sat(const WorldEconomy *e, double out[CLASS_COUNT]){
    double sw[CLASS_COUNT]={0}, pw[CLASS_COUNT]={0};
    for (int r=0;r<e->n_regions;r++){
        const RegionEconomy *re=&e->region[r];
        if (!re->active || !re->colonized) continue;
        for (int c=0;c<CLASS_COUNT;c++){ double p=re->strata[c].pop; sw[c]+=re->strata[c].satisfaction*p; pw[c]+=p; }
    }
    for (int c=0;c<CLASS_COUNT;c++) out[c] = pw[c]>0 ? 100.0*sw[c]/pw[c] : 0.0;
}
/* Pays le plus étendu (par régions). */
static int top_power(const World *w, const WorldEconomy *e, int *out_regions){
    int best=-1, bn=0;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        int n=regions_of(e,c); if (n>bn){bn=n;best=c;}
    }
    if (out_regions) *out_regions=bn;
    return best;
}
/* Nombre de guerres en cours (paires DIPLO_WAR). */
static int wars_active(const World *w, const DiploState *dp){
    int n=0;
    for (int a=0;a<w->n_countries;a++) for (int b=a+1;b<w->n_countries;b++)
        if (diplo_status(dp,a,b)==DIPLO_WAR) n++;
    return n;
}
/* Compte les rôles : empires (joueur + antagonistes) et cités-états. */
static void role_counts(const World *w, int *emp, int *city){
    *emp=*city=0;
    for (int c=0;c<w->n_countries;c++){
        PolityRole r=w->country[c].role;
        if (r==POLITY_PLAYER||r==POLITY_ANTAGONIST) (*emp)++;
        else if (r==POLITY_CITY_STATE) (*city)++;
    }
}
/* Prospérité & stabilité MOYENNES des empires vivants (lues par la membrane). */
static void empire_avg(const World *w, const WorldEconomy *e, const WorldProsperity *wp,
                       const TechState *ts, int *prosp, int *stab){
    long sp=0,ss=0; int n=0;
    for (int c=0;c<w->n_countries;c++){
        PolityRole rl=w->country[c].role;
        if ((rl==POLITY_PLAYER||rl==POLITY_ANTAGONIST) && regions_of(e,c)>0){
            CountryReadout r=country_readout(wp,ts,w,c);
            sp+=r.m_prosperite.value; ss+=r.m_stabilite.value; n++;
        }
    }
    *prosp = n? (int)(sp/n):0;
    *stab  = n? (int)(ss/n):0;
}
/* Population totale (somme des strates économiques de toutes les régions). */
static double total_pop(const WorldEconomy *e){
    double p=0.0;
    for (int r=0;r<e->n_regions;r++) for (int c=0;c<CLASS_COUNT;c++) p+=e->region[r].strata[c].pop;
    return p;
}
/* Population PAR CONTINENT (remplit pc[0..ncont-1]). */
static void continent_pop(const World *w, const WorldEconomy *e, double *pc, int ncont){
    for (int i=0;i<ncont;i++) pc[i]=0.0;
    for (int r=0;r<e->n_regions && r<w->n_regions;r++){
        int ci=w->region[r].continent;
        if (ci<0||ci>=ncont) continue;
        for (int c=0;c<CLASS_COUNT;c++) pc[ci]+=e->region[r].strata[c].pop;
    }
}
/* POURQUOI les révoltes : moyennes L/K/SI des polities EN révolution (mode 2)
 * vs stables → on lit la cause (légitimité ? capacité ?). */
static void revolt_cause(const World *w, const WorldEconomy *e, const WorldProsperity *wp,
                         int *nr, float *Lr, float *Kr, float *SIr,
                         int *ns, float *Ls, float *Ks, float *SIs){
    *nr=*ns=0; *Lr=*Kr=*SIr=*Ls=*Ks=*SIs=0.f;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || regions_of(e,c)==0) continue;
        const CountryProsperity *cp=&wp->country[c];
        if (cp->mode==2){ (*nr)++; *Lr+=cp->L; *Kr+=cp->K; *SIr+=cp->SI; }
        else            { (*ns)++; *Ls+=cp->L; *Ks+=cp->K; *SIs+=cp->SI; }
    }
    if (*nr){ *Lr/=*nr; *Kr/=*nr; *SIr/=*nr; }
    if (*ns){ *Ls/=*ns; *Ks/=*ns; *SIs/=*ns; }
}

/* Armée TOTALE du monde (somme des puissances militaires). */
static float total_army(const World *w, const WorldEconomy *e){
    float a=0.f; for (int c=0;c<w->n_countries;c++) a+=diplo_mil_power(w,e,c); return a;
}
/* Provinces COLONISÉES (régions peuplées × leurs provinces). */
static int colonized_provinces(const World *w, const WorldEconomy *e){
    int n=0;
    for (int r=0;r<e->n_regions && r<w->n_regions;r++)
        if (e->region[r].colonized) n+=w->region[r].n_provinces;
    return n;
}

/* ── TÉLÉMÉTRIE PAR ÂGE : marché · or par empire · tech, figés à l'avènement ──
 * (country_gold → econ_country_gold, public dans scps_econ.c, partagé avec scps_credit) */
#define country_gold econ_country_gold
/* Population PAR CLASSE d'un pays (somme des strates de ses régions). */
static void country_class_pop(const WorldEconomy *e, int c, double out[CLASS_COUNT]){
    for (int k=0;k<CLASS_COUNT;k++) out[k]=0.0;
    for (int r=0;r<e->n_regions;r++) if (e->region[r].owner==c)
        for (int k=0;k<CLASS_COUNT;k++) out[k]+=e->region[r].strata[k].pop;
}
/* ENTREPÔTS d'un pays : stock agrégé par bien (toutes ses régions). */
static void country_stocks(const WorldEconomy *e, int c, double out[RES_COUNT]){
    for (int g=0;g<RES_COUNT;g++) out[g]=0.0;
    for (int r=0;r<e->n_regions;r++) if (e->region[r].owner==c)
        for (int g=0;g<RES_COUNT;g++) out[g]+=e->region[r].stock[g];
}
static float avg_price(const WorldEconomy *e, Resource res){
    double s=0.0; int n=0;
    for (int r=0;r<e->n_regions;r++) if (e->region[r].colonized){ s+=e->region[r].price[res]; n++; }
    return n? (float)(s/n):0.f;
}
typedef struct {
    int    year;                                 /* -1 = âge pas (encore) avéné */
    double gold_total, gdp_total, pop_total;
    int    tech_total, living;
    float  p_grain, p_cloth, p_ware, p_tools;    /* le « marché » figé par âge */
    int    n_top; char top_name[3][32]; double top_gold[3]; int top_reg[3];
} AgeSnap;

static void capture_age_snap(AgeSnap *snap, int year, const World *w, const Sim *s){
    const WorldEconomy *e=s->econ;
    snap->year=year;
    double gold=0.0, gdp=0.0;
    for (int r=0;r<e->n_regions;r++){ gold+=e->region[r].treasury; gdp+=e->region[r].gdp; }
    snap->gold_total=gold; snap->gdp_total=gdp; snap->pop_total=total_pop(e);
    int tech=0; for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++) if (s->ai_on[c]) tech+=s->ai[c].stats.techs;
    snap->tech_total=tech; snap->living=living_countries(w,e);
    snap->p_grain=avg_price(e,RES_GRAIN);         snap->p_cloth=avg_price(e,RES_CLOTH);
    snap->p_ware =avg_price(e,RES_PRECIOUS_WARE); snap->p_tools=avg_price(e,RES_TOOLS);
    /* or des 3 plus gros empires (par régions) */
    int idx[SCPS_MAX_COUNTRY], ne=0;
    for (int c=0;c<w->n_countries;c++){ PolityRole rl=w->country[c].role;
        if ((rl==POLITY_PLAYER||rl==POLITY_ANTAGONIST) && regions_of(e,c)>0 && ne<SCPS_MAX_COUNTRY) idx[ne++]=c; }
    for (int a=0;a<ne;a++) for (int b=a+1;b<ne;b++)
        if (regions_of(e,idx[b])>regions_of(e,idx[a])){ int t=idx[a];idx[a]=idx[b];idx[b]=t; }
    snap->n_top = ne<3?ne:3;
    for (int a=0;a<snap->n_top;a++){ int c=idx[a];
        snprintf(snap->top_name[a],sizeof snap->top_name[a],"%s",w->country[c].name);
        snap->top_gold[a]=country_gold(e,c); snap->top_reg[a]=regions_of(e,c); }
}

/* ════════════════════════════════════════════════════════════════════════
 * LE HARNAIS DE DÉTERMINISME (brief build §2) — le JUGE DE PAIX.
 * En mode --hash, chaque sim émet « HASH <graine> <crc> » : un CRC32 (miniz,
 * vendoré) de sa propre SORTIE OBSERVABLE, rendue ici sous forme TEXTE
 * canonique (le flux du monde : qui tient quoi, l'or, la tech, révoltes,
 * diplomatie, la flotte, la course, les batailles). Le texte porte les
 * flottants en %.9g — round-trippables : toute réduction dont l'ORDRE
 * d'accumulation change (une parallélisation §4 mal placée) décale un bit,
 * décale le hash, et `make determinism` vire au rouge. Même graine → même
 * hash, toujours : c'est lui qui arbitre toute modif du moteur. */
typedef struct { mz_ulong crc; } HashAcc;
static void hx(HashAcc *h, const char *fmt, ...){
    char b[256]; va_list ap; va_start(ap,fmt);
    int n=vsnprintf(b,sizeof b,fmt,ap); va_end(ap);
    if (n<0) return;
    if (n>(int)sizeof b-1) n=(int)sizeof b-1;
    h->crc = mz_crc32(h->crc, (const unsigned char*)b, (size_t)n);
}
static uint32_t chronicle_sim_hash(uint32_t seed, const Sim *s, const World *w){
    HashAcc h; h.crc = MZ_CRC32_INIT;
    hx(&h,"seed=%u nc=%d nr=%d np=%d nk=%d\n",seed,w->n_countries,w->n_regions,w->n_provinces,w->n_continents);
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        hx(&h,"C%d reg=%d gold=%.9g tech=%d role=%d\n",
           c,regions_of(s->econ,c),country_gold(s->econ,c),s->ts[c].n_unlocked,(int)w->country[c].role);
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){ const RegionEconomy *re=&s->econ->region[r];
        double pop=0; for(int k=0;k<CLASS_COUNT;k++) pop+=re->strata[k].pop;
        hx(&h,"R%d o=%d t=%.9g p=%.9g c=%d e=%d b=%.9g\n",
           r,re->owner,(double)re->treasury,pop,re->colonized,re->estuary,(double)re->balafre_days);
    }
    hx(&h,"rev ig=%d se=%d co=%d cc=%d cr=%d pl=%ld\n",
       s->rs->n_ignited,s->rs->n_seceded,s->rs->n_coup,s->rs->n_concession,s->rs->n_crushed,(long)s->rs->pop_lost);
    hx(&h,"dip sv=%d pr=%d cc=%d ci=%d df=%d ap=%d\n",
       s->dp->n_servage,s->dp->n_protectorat,s->dp->n_concordat,s->dp->n_cite,s->dp->n_defections,s->dp->n_war_antipirate);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ const Navy *nv=&s->navy->n[c];
        if (!nv->built_total && !nv->raids_done && !nv->crew && !nv->intercepts) continue;
        hx(&h,"N%d b=%d r=%d pz=%d nv=%d ic=%d dr=%ld cw=%d su=%.9g\n",
           c,nv->built_total,nv->raids_done,nv->prises,nv->navals,nv->intercepts,(long)nv->drowned,nv->crew,(double)nv->supplies_eaten);
    }
    hx(&h,"camp bt=%d dc=%ld dp=%ld sl=%d\n",
       s->camp->n_battles,(long)s->camp->dead_choc,(long)s->camp->dead_pursuit,s->camp->n_sails);
    return (uint32_t)h.crc;
}

/* Arc J2 — collecte pour les MÉDIANES (flux d'or, trésor) sur tout le balayage. */
static double g_flux_all[8192], g_treas_all[8192];
static int    g_flux_n;
static int dcmp(const void *a, const void *b){ double x=*(const double*)a-*(const double*)b; return (x<0)?-1:(x>0); }
static double dmedian(double *v, int n){
    if (n<=0) return -1.0;
    qsort(v, n, sizeof *v, dcmp);
    return (n&1)? v[n/2] : 0.5*(v[n/2-1]+v[n/2]);
}

int main(int argc, char **argv){
    tune_init();   /* Arc J : lit SCPS_TUNE une fois (nom inconnu → exit 2). */
    /* positionnels FILTRÉS de l'option --hash (le harnais de déterminisme). */
    const char *pos[8]; int np=0, hash_mode=0;
    for (int i=1;i<argc;i++){
        if (!strcmp(argv[i],"--hash")) hash_mode=1;
        else if (!strcmp(argv[i],"--tunables")){ tune_list(stdout); return 0; }  /* Arc J : liste nom·défaut·actif */
        else if (!strcmp(argv[i],"--dump-data")){ econ_moddata_dump(stdout); tech_moddata_dump(stdout); army_moddata_dump(stdout); return 0; }  /* MODTOOLS : toutes les valeurs éditables */
        else if (np<8) pos[np++]=argv[i];
    }
    tune_print_active(stderr);   /* surcharges actives en tête (stderr → stdout reste byte-identique sans env) */
    { const char *m=getenv("SCPS_MODS");   /* MODTOOLS : surcharge des valeurs si défini (sinon vanilla → golden intact) */
      if (m && *m){ econ_moddata_load(m); tech_moddata_load(m); army_moddata_load(m); } }
    uint32_t base = (np>0)?(uint32_t)strtoul(pos[0],NULL,10):20240607u;
    int nsims     = (np>1)?atoi(pos[1]):10;      /* sim i : 2+i empires, 5+i cités (2→11 / 5→14) */
    int years     = (np>2)?atoi(pos[2]):200;
    int fix_emp   = (np>3)?atoi(pos[3]):0;       /* >0 : empires FIXES (sinon cycle 2+k) */
    int fix_cs    = (np>4)?atoi(pos[4]):0;       /* >0 : cités-états FIXES (sinon cycle 5+k) */
    int fix_cont  = (np>5)?atoi(pos[5]):0;       /* >0 : continents FIXES (mer §8 : bancs 2..4) */
    if (nsims<1) nsims=1;
    if (years<1) years=1;

    World *w = malloc(sizeof(World));
    Sim s;
    s.econ=malloc(sizeof(WorldEconomy)); s.wp=malloc(sizeof(WorldProsperity));
    s.wl=malloc(sizeof(WorldLegitimacy)); s.net=malloc(sizeof(TradeNetwork));
    s.ts=calloc(SCPS_MAX_COUNTRY,sizeof(TechState)); s.sc=malloc(sizeof(Statecraft));
    s.ag=malloc(sizeof(AgencyState)); s.ev=malloc(sizeof(EventsState));
    s.drift=malloc(sizeof(ModifierStack)); s.labor=malloc(sizeof(LaborEcon));
    s.dp=malloc(sizeof(DiploState)); s.rn=malloc(sizeof(RouteNetwork));
    s.ai=calloc(SCPS_MAX_COUNTRY,sizeof(AiActor)); s.ai_on=calloc(SCPS_MAX_COUNTRY,sizeof(bool));
    s.rs=malloc(sizeof(RevoltState)); s.host=calloc(1,sizeof(WarHost));   /* P1 : scratch NULL d'emblée (warhost_free sûr) */
    s.missions=malloc(sizeof(MissionsState)); s.camp=malloc(sizeof(Campaign));
    s.navy=malloc(sizeof(NavyState)); s.eg=calloc(1,sizeof(EndgameState));
    if (!w||!s.econ||!s.wp||!s.wl||!s.net||!s.ts||!s.sc||!s.ag||!s.ev||!s.drift
        ||!s.labor||!s.dp||!s.rn||!s.ai||!s.ai_on||!s.rs||!s.host||!s.missions||!s.camp||!s.navy||!s.eg){
        fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════════════\n");
    if (fix_emp>0 || fix_cs>0)
        printf(" CHRONIQUE — balayage : %d sims, %d ans (empires %d, cités %d FIXES ; sans joueur)\n",
               nsims, years, fix_emp>0?fix_emp:2, fix_cs>0?fix_cs:5);
    else
        printf(" CHRONIQUE — balayage : %d sims, %d ans (empires 2→%d, cités 5→%d ; sans joueur)\n",
               nsims, years, 1+nsims, 4+nsims);
    printf("══════════════════════════════════════════════════════════════════════\n");

    /* Agrégats sur toutes les sims */
    long tot_wars=0, tot_absorbed=0, tot_emerged=0, tot_peakrev=0, tot_ages=0, tot_conq=0;
    long tot_ignited=0, tot_seceded=0, tot_coup=0, tot_concession=0, tot_crushed=0, tot_revdead=0;
    long tot_techs=0, tot_faustian=0, tot_campaign=0, tot_alliances=0;   /* §D : pactes actifs */
    long tot_sync=0, tot_sync_distinct=0;   /* §syncrétique : nœuds à porte culturelle + dispersion */
    long tot_relig_roots=0, tot_relig_schisms=0, tot_relig_faith=0, tot_relig_minority=0;   /* RELIGION : foi émergente */
    long tot_tree_pct=0; int tot_tree_sims=0;   /* §A : fraction d'arbre déverrouillée (le coût force les choix) */
    long tot_reloc=0;   /* §reloc : ensemencements de pop pour combler une pénurie */
    long tot_repress=0, tot_assim=0, tot_purge=0, tot_purge_dead=0;       /* leviers intérieurs */
    long tot_serv=0, tot_prot=0, tot_conc=0, tot_cite=0, tot_defect=0, tot_annex=0;    /* suzeraineté */
    long tot_wcb[CB_ANTIPIRATERIE+1]={0};    /* guerres motivées : déclarations par casus belli (cumul) */
    long tot_ligues=0, tot_frondes=0, tot_indep=0, tot_renvers=0, tot_ecrase=0;   /* fronde */
    long tot_bt=0, tot_btj=0, tot_routs=0, tot_rallies=0, tot_mchoc=0, tot_mpour=0, tot_deseng=0, tot_renf=0, tot_nul=0;   /* batailles */
    double tot_sat[CLASS_COUNT]={0}; double tot_trade=0;   /* §distrib : satisfaction par classe + commerce */
    long tot_emp_n=0, tot_emp_hub=0;   /* par-empire : moyennes de fin de sim */
    double tot_emp_gold=0, tot_emp_flux=0, tot_emp_imp=0, tot_emp_exp=0, tot_emp_expgold=0;
    long tot_tier_y[3]={0,0,0}; int tot_tier_n[3]={0,0,0};   /* E1 §9 : fenêtres d'accession */
    double tot_spec_vol=0, tot_spec_gold=0; long tot_spec_ent=0;   /* E3 §16 : l'IA stockeuse */
    double tot_sd0=0, tot_sd1=0; int tot_sd0_n=0, tot_sd1_n=0;     /* σ avant/après (moy. des sims) */
    double tot_sp0=0, tot_sp1=0; int tot_sp0_n=0, tot_sp1_n=0;     /* σ SPATIAL : à entrepôt vs sans */
    double tot_hub_cs=0, tot_hub_all=0;   /* P3.20 : part des cités-états dans le commerce (via Centres) */
    long tot_chokes=0, tot_choke_held=0, tot_choke_routes=0; double tot_choke_toll=0;   /* WG : les détroits, agrégés */
    int  worlds_with_choke_toll=0;   /* WG : sims où un tenant a EFFECTIVEMENT encaissé un péage */
    long tot_captured=0, tot_worstcorr=0; int worlds_with_capture=0;   /* §C3 : le rot, agrégé */
    int  worlds_with_ironorder=0, worlds_with_uprising=0;
    int  worlds_hegemon_cracked=0;   /* A5 : sims où un hégémon ≥10 rég est passé sous Stab 50 OU a subi coup/renversement */
    long tot_hegemon_floor=0; int n_hegemon_sims=0;   /* Stab plancher des hégémons (la fragilité A1 les rend mortels) */
    long tot_dir_fired[DIR_EV_COUNT]={0}; long tot_dir_destab=0, tot_dir_stab=0, tot_dir_overcap=0;   /* §F : le directeur, agrégé */
    int  worlds_dir_topok=0, worlds_dir_stabok=0, tot_dir_toppct=0;   /* G0.1 preuves : top ≤30 % · stabilisateur si T>0.5 */
    double tot_ampl_pic=0.0, tot_ampl_fin=0.0; long tot_omens=0; int worlds_ampl_vibre=0;   /* §G2 : le directeur-amplitude, agrégé */
    double tot_ipm=0.0, max_ipm=0.0;   /* §C : l'inflation monétaire, agrégée */
    long tot_hulls=0, tot_sails=0, tot_searoutes=0, tot_colonies_om=0;   /* mer §10 */
    double tot_supplies=0, tot_saildays=0;
    long tot_raids=0, tot_prises=0, tot_navals=0, tot_disarm=0, tot_warpir=0, tot_balafres=0;
    double tot_loot=0;   /* coques §8 : la course, agrégée */
    long tot_intercepts=0, tot_drowned=0;

    for (int k=0;k<nsims;k++){
        uint32_t seed = base + (uint32_t)k*101u;
        WorldParams p = worldparams_default(seed);
        /* La taille du monde CYCLE (2→11 empires, 5→14 cités) — identique aux 10
         * premières sims, puis on REBOUCLE : un balayage de 100 sims reste faisable
         * (sinon 2+k saturerait SCPS_MAX_COUNTRY=56 et chaque sim tardive ramperait).
         * Graines toutes distinctes (base+k·101) → 100 mondes différents. */
        p.n_empires     = (fix_emp>0)? fix_emp : 2 + (k % 10);
        p.n_city_states = (fix_cs >0)? fix_cs  : 5 + (k % 10);
        if (fix_cont>0) p.n_continents = fix_cont;
        world_generate(w, &p);
        /* silence le bruit de génération : on a déjà tout imprimé par sim plus bas */
        sim_init(&s, w);

        /* LA VOLTA, mesurée (mer §10) : entre deux côtes éloignées, l'aller ne vaut
         * pas le retour — l'asymétrie du champ de courants, en jours. */
        { int rA=-1, rB=-1;
          for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].coastal){ rA=r; break; }
          for (int r=s.econ->n_regions-1;r>=0;r--) if (s.econ->region[r].coastal){ rB=r; break; }
          if (rA>=0 && rB>=0 && rA!=rB){
              float aller=navy_sea_days_regions(w,rA,rB), retour=navy_sea_days_regions(w,rB,rA);
              if (aller>=0.f && retour>=0.f)
                  printf("   mer : volta mesurée région %d ⇄ %d — aller %.0f j · retour %.0f j (asymétrie ×%.2f)\n",
                         rA, rB, aller, retour, (aller>0.f)?retour/aller:0.f);
          } }

        int cont = w->n_continents;
        int n_emp, n_city; role_counts(w, &n_emp, &n_city);
        int c0 = living_countries(w, s.econ);
        /* qui est VIVANT au départ : on tracera les morts (absorbés) ET les
         * naissances (sécessions) — la carte politique BOUGE, plus d'invariant figé. */
        bool was_alive[SCPS_MAX_COUNTRY];
        for (int c=0;c<SCPS_MAX_COUNTRY;c++)
            was_alive[c] = (c<w->n_countries && w->country[c].role!=POLITY_UNCLAIMED && regions_of(s.econ,c)>0);

        int age_year[AGE_COUNT]; AgeSnap age_snap[AGE_COUNT];
        for (int a=0;a<AGE_COUNT;a++){ age_year[a]=-1; age_snap[a].year=-1; }
        int tier_year[3]={-1,-1,-1};   /* E1 §9 : année du PREMIER édifice 360/540/960 j bâti */
        /* E3 §16 — le LISSAGE par les stocks : σ d'un indice de prix (panier
         * grain·métal·outils, normalisé par les ancres), échantillonné chaque
         * année, AVANT vs APRÈS l'existence du premier Entrepôt au monde. */
        double sg_n[2]={0,0}, sg_mean[2]={0,0}, sg_m2[2]={0,0};
        /* … et la mesure CAUSALE (le temps long charrie guerres et âges) : le σ
         * TEMPOREL PAR RÉGION (Welford par région, échantillon annuel), moyenné
         * ensuite par seau — régions À entrepôt vs SANS, même monde même époque :
         * l'amortisseur se lit là où il est bâti, pas dans la dispersion géo. */
        static double pr_n[SCPS_MAX_REG], pr_mean[SCPS_MAX_REG], pr_m2[SCPS_MAX_REG];
        memset(pr_n,0,sizeof pr_n); memset(pr_mean,0,sizeof pr_mean); memset(pr_m2,0,sizeof pr_m2);
        int war_onsets=0, prev_wars=0, peak_wars=0;
        int peak_rev=0, peak_rev_year=0;
        int min_living=c0;
        int hegemon_max_reg=0, hegemon_stab_floor=100;   /* A5 : le plus grand empire & son plancher de Stabilité */
        /* suivi des transferts de propriété : conquête vs colonisation */
        int conq_prov=0;                              /* provinces TRANSFÉRÉES à la paix (cumul) — la propriété ne change qu'au règlement */
        int16_t prev_owner[SCPS_MAX_REG];
        for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++) prev_owner[r]=s.econ->region[r].owner;

        printf("\n── Sim %d (graine %u) — %d empires · %d cités-états · %d continents · %d régions ──\n",
               k+1, seed, n_emp, n_city, cont, s.econ->n_regions);
        /* RESSOURCES DU MONDE (genèse) — vérifier que la worldgen colle aux specs : par brute
         * EXTRACTIBLE, nb de régions qui la portent (raw_cap>0) et la capacité totale Σ. La coupe
         * REGION_RAW_KEEP garde ≈ 2 brutes/région ⇒ Σ(régions-brutes) ≈ 2×régions. */
        {
            int act=0; for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].active) act++;
            printf("              ressources (extraction, %d rég actives) :", act);
            for (int g=1; g<RES_PROD_FIRST; g++){
                int n=0; double cap=0.0;
                for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].raw_cap[g]>0.f){ n++; cap+=s.econ->region[r].raw_cap[g]; }
                if (n>0) printf(" %s %d(Σ%.0f)", resource_name((Resource)g), n, cap);
            }
            printf("\n");
        }

        int snap[4]={years/5, years*2/5, years*3/5, years*4/5}, si=0;  /* instantanés mis à l'échelle */
        /* photo des trésors au seuil de la DERNIÈRE année → flux d'or net par MOIS
         * (un pays né en cours d'année part de 0 : son flux englobe sa dotation). */
        double gold_y0[SCPS_MAX_COUNTRY]={0};
        for (int yr=0; yr<years; yr++){
            if (yr==years-1){
                for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++) gold_y0[c]=country_gold(s.econ,c);
                econ_flux_reset();   /* I0 : la décomposition du flux porte sur la DERNIÈRE année */
            }
            for (int d=0; d<365; d++) sim_day(&s, w);
            /* conquêtes de l'année : régions passées d'un PAYS à un autre (de force) */
            for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++){
                int16_t no=s.econ->region[r].owner, po=prev_owner[r];
                if (po>=0 && no>=0 && no!=po) conq_prov += w->region[r].n_provinces;
                prev_owner[r]=no;
            }
            /* E3 §16 — échantillon ANNUEL de l'indice de prix (Welford), classé
             * avant/après l'existence du premier Entrepôt (le lissage se mesure). */
            { bool any_ent=false;
              for (int r=0;r<s.econ->n_regions && !any_ent;r++)
                  if (s.econ->region[r].n_entrepot>0) any_ent=true;
              float idx = (avg_price(s.econ,RES_GRAIN)/econ_base_price(RES_GRAIN)
                         + avg_price(s.econ,RES_IRON)/econ_base_price(RES_IRON)
                         + avg_price(s.econ,RES_TOOLS)/econ_base_price(RES_TOOLS))/3.f;
              int b = any_ent?1:0;
              sg_n[b]+=1.0; double d=idx-sg_mean[b]; sg_mean[b]+=d/sg_n[b]; sg_m2[b]+=d*(idx-sg_mean[b]);
              /* σ temporel PAR RÉGION : chaque région colonisée suit SON indice
               * d'une année à l'autre (le seau se tranche en fin de sim). */
              for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++){
                  const RegionEconomy *re2=&s.econ->region[r];
                  if (!re2->colonized) continue;
                  double ir = (re2->price[RES_GRAIN]/econ_base_price(RES_GRAIN)
                             + re2->price[RES_IRON]/econ_base_price(RES_IRON)
                             + re2->price[RES_TOOLS]/econ_base_price(RES_TOOLS))/3.0;
                  pr_n[r]+=1.0; double d2=ir-pr_mean[r]; pr_mean[r]+=d2/pr_n[r]; pr_m2[r]+=d2*(ir-pr_mean[r]);
              } }
            /* E1 §9 — fenêtres d'ACCESSION : l'année où le premier édifice de chaque
             * palier (360/540/960 j) s'achève quelque part. La loi des prix se LIT. */
            if (tier_year[0]<0 || tier_year[1]<0 || tier_year[2]<0)
                for (int r=0;r<s.econ->n_regions;r++){
                    uint32_t built=s.econ->region[r].edi_built;
                    if (!built) continue;
                    for (int e2=0;e2<EDIFICE_COUNT;e2++){
                        if (!(built&(1u<<e2))) continue;
                        int d=edifice_def((Edifice)e2)->days;
                        int ti = (d==360)?0:(d==540)?1:(d==960)?2:-1;
                        if (ti>=0 && tier_year[ti]<0) tier_year[ti]=s.year;
                    }
                }
            /* âges : on imprime À L'AVÈNEMENT → la ligne du temps est chronologique */
            for (int a=0;a<AGE_COUNT;a++)
                if (age_year[a]<0 && ages_dawned(s.ev,(AgeId)a)){
                    age_year[a]=s.year;
                    capture_age_snap(&age_snap[a], s.year, w, &s);   /* marché·or·tech figés à l'avènement */
                    printf("   an %3d  ÂGE : %s\n", s.year, age_name((AgeId)a));
                }
            int wa = wars_active(w, s.dp);
            /* §G2 — SCPS_AMPLDIAG : la COURBE D'AMPLITUDE année par année (la preuve
             * que l'amplitude MONTE après un choc — guerre/révolte/T haute — et
             * REDESCEND au calme). T = température directeur ; ★ = présage émis cette
             * année (la boucle « tale » a bouclé). Une seule sim suffit (la 1re). */
            if (getenv("SCPS_AMPLDIAG") && k==0){
                static int prev_omens=0; if (s.year==1) prev_omens=0;
                int om=director_omens(s.ev); int newom=om-prev_omens; prev_omens=om;
                printf("   AMPL an %3d : ampl %.3f  adapt %5.0f j  T %.2f  budget %5.0f  guerres %d  révoltés %d  présages +%d%s\n",
                       s.year, director_amplitude(s.ev), director_adapt_days(s.ev),
                       director_temperature(s.ev), director_budget(s.ev),
                       wa, events_count_revolutionary(w,s.wp), newom, newom>0?" ★":"");
            }
            if (wa>prev_wars) war_onsets += (wa-prev_wars);
            if (wa>peak_wars) peak_wars=wa;
            prev_wars = wa;
            int rv = events_count_revolutionary(w, s.wp);
            if (rv>peak_rev){ peak_rev=rv; peak_rev_year=s.year; }
            int lv = living_countries(w, s.econ);
            if (lv<min_living) min_living=lv;
            /* A5 — L'HÉGÉMON EST-IL MORTEL ? Le plus grand empire, dès qu'il tient
             * ≥10 régions, voit-il sa Stabilité passer sous 50 ? (la fragilité A1
             * rend le géant coercitif cassable — avant, il restait lisse à jamais). */
            { int hr=0, ht=top_power(w,s.econ,&hr);
              if (ht>=0 && hr>=10){
                  int hs=country_readout(s.wp,s.ts,w,ht).m_stabilite.value;
                  if (hr>hegemon_max_reg) hegemon_max_reg=hr;
                  if (hs<hegemon_stab_floor) hegemon_stab_floor=hs;
              } }
            /* instantané tous les 50 ans — les courbes DANS LE TEMPS :
             * population · armée totale · provinces colonisées · prises de force */
            if (si<4 && s.year>=snap[si]){
                int treg=0; top_power(w,s.econ,&treg);
                int ap,as_; empire_avg(w,s.econ,s.wp,s.ts,&ap,&as_);
                printf("   an %3d : %2d pays | pop %5.0fk | armée %5.0f | colonisées %3d prov | transf. paix %3d prov | 1er empire %2d rég | prosp %2d stab %2d | %2d révolté(s)\n",
                       snap[si], lv, total_pop(s.econ)/1000.0, total_army(w,s.econ),
                       colonized_provinces(w,s.econ), conq_prov, treg, ap, as_, rv);
                si++;
            }
        }

        int c1 = living_countries(w, s.econ);
        int treg=0, tp = top_power(w, s.econ, &treg);
        CountryReadout r = (tp>=0)? country_readout(s.wp,s.ts,w,tp)
                                  : country_readout(s.wp,s.ts,w,0);
        /* MORTS (absorbés) vs NAISSANCES (sécessions) : la carte politique respire. */
        int absorbed=0, emerged=0;
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
            bool alive_now = (w->country[c].role!=POLITY_UNCLAIMED && regions_of(s.econ,c)>0);
            if (was_alive[c] && !alive_now) absorbed++;
            if (!was_alive[c] && alive_now) emerged++;
        }
        int share = (s.econ->n_regions>0)? treg*100/s.econ->n_regions : 0;
        int nages=0; for (int a=0;a<AGE_COUNT;a++) if (age_year[a]>=0) nages++;

        printf("   BILAN an %d : %d pays subsistent (%d absorbés · %d émergés ; plancher %d) | %d âge(s) ; %d guerre(s) au total, pic %d ; pic de révolte %d (an %d)\n",
               years, c1, absorbed, emerged, min_living, nages, war_onsets, peak_wars, peak_rev, peak_rev_year);
        /* FAU/F8 — la boucle faustienne (transmuteurs + entropie) ET la demande de fer (forge
         * militaire). Conso cumulée par rare ; entropie monde ; prix moyen du fer (la preuve F8). */
        { double pir=0.0, pmax=0.0, arms=0.0; int npr=0; long fract=0;
          for (int r=0;r<s.econ->n_regions;r++){ if (s.econ->region[r].owner<0) continue;
              double p=s.econ->region[r].price[RES_IRON]; pir+=p; if(p>pmax)pmax=p; npr++;
              arms += s.econ->region[r].supply[RES_ARMS_LIGHT]+s.econ->region[r].supply[RES_ARMS_HEAVY]
                    + s.econ->region[r].supply[RES_ARMS_RANGED]+s.econ->region[r].supply[RES_FIREARM]; }
          for (int c=0;c<w->n_countries;c++) if (s.ai_on[c] && tech_crisis_proximity(&s.ts[c])>0.85f) fract++;
          printf("              faustien : entropie monde %.0f%s · conso foreuse %.0f · réplicateur %.0f · corne %.0f · %ld empire(s) au bord de la Brèche | FER prix moy %.1f / max %.1f (base %.1f) · armes produites %.0f\n",
                 (double)s.wp->entropy, s.wp->entropy_terminal?" [TERMINAL]":"",
                 s.wp->faust_consumed[0], s.wp->faust_consumed[1], s.wp->faust_consumed[2],
                 fract, npr?pir/npr:0.0, pmax, (double)econ_base_price(RES_IRON), arms); }
        /* CAPSTONE §27 — la FIN, si elle s'est déclenchée (la preuve d'émergence). */
        if (s.eg && s.eg->fired){
            static const char *FN[]={"—","ENGLOUTISSEMENT","GRAND HIVER","RONCES","ASCENSION"};
            int fin=(int)s.eg->fin; if(fin<0||fin>4)fin=0;
            printf("              §27 FIN : %s (an %d) · épicentre rég %d · fauteur pays %d",
                   FN[fin], s.eg->fin_year, s.eg->epicenter_reg, s.eg->fauteur_country);
            if (s.eg->fin==FIN_EAU)        printf(" · %d région(s) englouties (%d en cours)", s.eg->n_sunken, s.eg->sink_pending);
            else if (s.eg->fin==FIN_FROID) printf(" · refroidissement %.0f%%", (double)s.eg->cold_offset*100.0);
            else if (s.eg->fin==FIN_RONCES)printf(" · front de ronces %d cellule(s)", s.eg->thorn_front_n);
            printf("\n");
        }
        if (getenv("SCPS_BASKETDIAG")){
            /* CONSO (demande agrégée) vs OUTPUT RÉEL (offre) par ressource — pour voir QUELLE brute/bien
             * le panier ne couvre pas (couv < 100 % = pénurie réelle). demande/offre sont mensuelles. */
            double bd[RES_COUNT]={0}, bs[RES_COUNT]={0}, bk[RES_COUNT]={0};
            for (int r=0;r<s.econ->n_regions;r++){ if(s.econ->region[r].owner<0)continue;
                for (int g=1;g<RES_COUNT;g++){ bd[g]+=s.econ->region[r].demand[g];
                    bs[g]+=s.econ->region[r].supply[g]; bk[g]+=s.econ->region[r].stock[g]; } }
            fprintf(stderr,"[BASKET an %d] conso vs output (monde) :\n", s.year);
            for (int g=1;g<RES_COUNT;g++){
                if (bd[g]<0.5 && bs[g]<0.5) continue;
                double cov=(bd[g]>1e-3)? bs[g]/bd[g] : 9.9;
                fprintf(stderr,"   %-22s dem %.0f · out %.0f · stk %.0f · couv %.0f%%\n",
                        resource_name((Resource)g), bd[g], bs[g], bk[g], cov*100.0);
            }
        }
        if (getenv("SCPS_FORGEDIAG")){
            long bld[BLD_TYPE_COUNT]; memset(bld,0,sizeof bld);
            double sup[RES_COUNT]; for(int g=0;g<RES_COUNT;g++)sup[g]=0.0;
            for (int r=0;r<s.econ->n_regions;r++){ if (s.econ->region[r].owner<0) continue;
                for (int b=0;b<s.econ->region[r].n_bld;b++){ int ty=s.econ->region[r].bld[b].type; if(ty>=0&&ty<BLD_TYPE_COUNT)bld[ty]++; }
                for (int g=0;g<RES_COUNT;g++) sup[g]+=s.econ->region[r].supply[g]; }
            { long btot=0; long b_clay=0,b_stone=0,b_iron=0,b_wood=0;
              for (int r=0;r<s.econ->n_regions;r++){ if(s.econ->region[r].owner<0)continue;
                  for (int g=1;g<RES_PROD_FIRST;g++) btot+=s.econ->region[r].raw_boost[g];
                  b_clay+=s.econ->region[r].raw_boost[RES_CLAY]; b_stone+=s.econ->region[r].raw_boost[RES_STONE];
                  b_iron+=s.econ->region[r].raw_boost[RES_IRON]; b_wood+=s.econ->region[r].raw_boost[RES_WOOD]; }
              fprintf(stderr,"[FORGEDIAG] EXPLOITATIONS (paliers de boost) : total %ld | argile %ld · pierre %ld · fer %ld · bois %ld\n",
                      btot,b_clay,b_stone,b_iron,b_wood); }
            long u[U_COUNT]; memset(u,0,sizeof u);
            for (int c=0;c<w->n_countries;c++) for (int i=0;i<s.host->army[c].n_units;i++){
                int ty=s.host->army[c].units[i].type; if(ty>=0&&ty<U_COUNT) u[ty]+=s.host->army[c].units[i].count; }
            fprintf(stderr,"[FORGEDIAG] FABRIQUES : lourde %ld · arc %ld · arquebuserie %ld · réplicateur %ld · corne %ld | mage %ld · forge céleste %ld\n",
                    bld[BLD_ARMORY_HEAVY],bld[BLD_BOWYER],bld[BLD_ARQUEBUS],bld[BLD_REPLICATEUR],bld[BLD_CORNE],bld[BLD_MAGE_WORKSHOP],bld[BLD_CELESTIAL_FORGE]);
            fprintf(stderr,"[FORGEDIAG] ARMES produites/tick : lourde %.1f · trait %.1f · feu %.1f · bâton %.1f · kit %.1f · enchantées %.1f\n",
                    sup[RES_ARMS_HEAVY],sup[RES_ARMS_RANGED],sup[RES_FIREARM],sup[RES_MAGE_STAFF],sup[RES_ALCHEMIST_KIT],sup[RES_ENCHANTED_ARMS]);
            fprintf(stderr,"[FORGEDIAG] TROUPES (snapshot an %d) : hallebardier %ld · arquebusier %ld · alchimiste %ld · garde runique %ld · archer %ld · cav lourde %ld | piquier %ld · épéiste %ld\n",
                    s.year,u[U_HALLEBARDIER],u[U_ARQUEBUSIER],u[U_ALCHIMISTE],u[U_GARDE_RUNIQUE],u[U_ARCHER],u[U_CAV_LOURDE],u[U_PIQUIER],u[U_EPEISTE]);
            fprintf(stderr,"[FORGEDIAG] PIC (sur le siècle) : hallebardier %ld · arquebusier %ld · alchimiste %ld · garde runique %ld · archer %ld · cav lourde %ld | piquier %ld · épéiste %ld\n",
                    g_peak_u[U_HALLEBARDIER],g_peak_u[U_ARQUEBUSIER],g_peak_u[U_ALCHIMISTE],g_peak_u[U_GARDE_RUNIQUE],g_peak_u[U_ARCHER],g_peak_u[U_CAV_LOURDE],g_peak_u[U_PIQUIER],g_peak_u[U_EPEISTE]);
            fprintf(stderr,"[FORGEDIAG] PIC roster-22 (les 10 neuves) : arb.lourd %ld · berserker %ld · lancier-choc %ld · milice %ld · harceleur %ld · traqueur %ld · lame-franche %ld · garde-escorte %ld · cuirassée %ld · cav-raid %ld\n",
                    g_peak_u[U_ARBALETE_LOURDE],g_peak_u[U_BERSERKER],g_peak_u[U_LANCIER_CHOC],g_peak_u[U_MILICE],g_peak_u[U_HARCELEUR],g_peak_u[U_TRAQUEUR],g_peak_u[U_LAME_FRANCHE],g_peak_u[U_GARDE_ESCORTE],g_peak_u[U_CAV_CUIRASSEE],g_peak_u[U_CAV_RAID]);
            { double stk[RES_COUNT]; for(int g=0;g<RES_COUNT;g++)stk[g]=0.0;
              for (int r=0;r<s.econ->n_regions;r++){ if(s.econ->region[r].owner<0)continue;
                  for(int g=0;g<RES_COUNT;g++) stk[g]+=s.econ->region[r].stock[g]; }
              fprintf(stderr,"[FORGEDIAG] ARSENAL (stock, an %d) : lég %.0f · lourde %.0f · trait %.0f · feu %.0f · enchantées %.0f (seuil 1 paquet = %d)\n",
                    s.year,stk[RES_ARMS_LIGHT],stk[RES_ARMS_HEAVY],stk[RES_ARMS_RANGED],stk[RES_FIREARM],stk[RES_ENCHANTED_ARMS],POP_PER_UNIT);
              fprintf(stderr,"[FORGEDIAG] CHAÎNE OUTILS (fer+bois→outils DIRECT) : taillanderie %ld bâtie | fer extr %.0f → outils out %.0f stk %.0f (dem %.0f)\n",
                    bld[BLD_TOOLWORKS], sup[RES_IRON],sup[RES_TOOLS],stk[RES_TOOLS],
                    ({ double dt=0; for(int r=0;r<s.econ->n_regions;r++) if(s.econ->region[r].owner>=0) dt+=s.econ->region[r].demand[RES_TOOLS]; dt; })); }
            { int cc=0,ca=0,cs=0,cu=0; double rcc=0,rca=0,rcs=0,rcu=0,spc=0,spa=0,sps=0,spu=0;   /* RAW spéciaux sur TOUTE la carte (≠ owned) */
              for (int r=0;r<s.econ->n_regions;r++){ RegionEconomy *re=&s.econ->region[r];
                  if(re->raw_cap[RES_CELESTIAL_IRON]>0.f){cc++;rcc+=re->raw_cap[RES_CELESTIAL_IRON];} spc+=re->supply[RES_CELESTIAL_IRON];
                  if(re->raw_cap[RES_ARCANE_CRYSTAL]>0.f){ca++;rca+=re->raw_cap[RES_ARCANE_CRYSTAL];} spa+=re->supply[RES_ARCANE_CRYSTAL];
                  if(re->raw_cap[RES_SALTPETER]>0.f){cs++;rcs+=re->raw_cap[RES_SALTPETER];} sps+=re->supply[RES_SALTPETER];
                  if(re->raw_cap[RES_COPPER]>0.f){cu++;rcu+=re->raw_cap[RES_COPPER];} spu+=re->supply[RES_COPPER]; }
              fprintf(stderr,"[FORGEDIAG] RAW SPÉCIAUX (carte, %d rég) : fer céleste %d rég·cap %.1f·extr %.1f | cristal arcane %d·%.1f·%.1f | salpêtre %d·%.1f·%.1f | cuivre %d·%.1f·%.1f\n",
                    s.econ->n_regions, cc,rcc,spc, ca,rca,spa, cs,rcs,sps, cu,rcu,spu); }
            { int nemp=0,caserne=0,poudr=0,runes=0,magie=0,alch=0;   /* la DEMANDE : qui peut RECRUTER (tech débloquée) */
              for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){ if(!s.ai_on[c])continue; nemp++;
                  if(s.ts[c].unlocked[TECH_CASERNE])      caserne++;
                  if(s.ts[c].unlocked[TECH_POUDRIERE])    poudr++;
                  if(s.ts[c].unlocked[TECH_FORGE_RUNES])  runes++;
                  if(s.ts[c].unlocked[TECH_MAGIE_BATAILLE])magie++;
                  if(s.ts[c].unlocked[TECH_ALCHIMIE])     alch++; }
              fprintf(stderr,"[FORGEDIAG] TECH MILITAIRES (%d empires) : caserne→hallebardier %d · poudrière→arquebusier %d · forge runes→garde %d · magie→mage %d · alchimie→alchimiste %d\n",
                    nemp, caserne,poudr,runes,magie,alch); }
            { int ncol=0,under=0; double tpop=0,rawk=0,goods=0;   /* DÉV : colonies peuplées ? raw processé ? */
              for (int r=0;r<s.econ->n_regions;r++){ RegionEconomy *re=&s.econ->region[r];
                  if(re->owner<0||!re->colonized)continue;
                  ncol++;
                  double pop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
                  tpop+=pop; if(pop<1000.0)under++;
                  rawk += re->stock[RES_IRON]+re->stock[RES_WOOD]+re->stock[RES_COAL]+re->stock[RES_WOOL]+re->stock[RES_GRAIN];
                  goods+= re->supply[RES_CLOTH]+re->supply[RES_TOOLS]+re->supply[RES_PAPER]; }
              fprintf(stderr,"[FORGEDIAG] DÉV : %d colonies · pop moy %.0f · %d sous 1000 hab | stock RAW %.0f · biens manuf/tick %.0f (le raw doit DESCENDRE, les biens MONTER)\n",
                    ncol, ncol?tpop/ncol:0.0, under, rawk, goods); }
            { double dem[RES_COUNT]; for(int g=0;g<RES_COUNT;g++)dem[g]=0.0;
              for (int r=0;r<s.econ->n_regions;r++){ if(s.econ->region[r].owner<0)continue;
                  for(int g=0;g<RES_COUNT;g++) dem[g]+=s.econ->region[r].demand[g]; }
              fprintf(stderr,"[FORGEDIAG] MANUFACTURÉ produit/tick : étoffe %.0f · outils %.0f · armes lég %.0f | RAW consommé (demande) : fer %.0f · bois %.0f · charbon %.0f · laine %.0f\n",
                    sup[RES_CLOTH],sup[RES_TOOLS],sup[RES_ARMS_LIGHT], dem[RES_IRON],dem[RES_WOOD],dem[RES_COAL],dem[RES_WOOL]); }
            /* POURQUOI 0 ? — la fabrique se bâtit si prix_sortie ≥ 1.8×base (pénurie) ET intrants dispo.
             * On regarde le MAX (sur régions poss.) du prix des armes neuves vs le seuil, + le bois. */
            double pmh=0,pmr=0,pmf=0,dmh=0,dmr=0,df=0,woodmax=0,woodsup=0;
            for (int r=0;r<s.econ->n_regions;r++){
                if(s.econ->region[r].owner<0) continue;
                const RegionEconomy*re=&s.econ->region[r];
                pmh=fmax(pmh,re->price[RES_ARMS_HEAVY]); pmr=fmax(pmr,re->price[RES_ARMS_RANGED]); pmf=fmax(pmf,re->price[RES_FIREARM]);
                dmh=fmax(dmh,re->demand[RES_ARMS_HEAVY]); dmr=fmax(dmr,re->demand[RES_ARMS_RANGED]); df=fmax(df,re->demand[RES_FIREARM]);
                woodmax=fmax(woodmax,re->raw_cap[RES_WOOD]+re->supply[RES_WOOD]); woodsup+=re->supply[RES_WOOD];
            }
            fprintf(stderr,"[FORGEDIAG] BÂTIR ? seuil pénurie = 1.8×base | LOURDE prix_max %.2f (seuil %.1f) dem_max %.2f | TRAIT prix %.2f (seuil %.1f) dem %.2f | FEU prix %.2f (seuil %.1f) dem %.2f | BOIS max %.1f sup %.1f\n",
                    pmh,1.8*econ_base_price(RES_ARMS_HEAVY),dmh, pmr,1.8*econ_base_price(RES_ARMS_RANGED),dmr, pmf,1.8*econ_base_price(RES_FIREARM),df, woodmax,woodsup);
        }
        if (tp>=0)
            printf("              1er empire « %s » : %d régions (%d%% des terres) | Stabilité %d  Prospérité %d  Légitimité %d  Cohésion %d — Assise %s\n",
                   w->country[tp].name, treg, share, r.m_stabilite.value, r.m_prosperite.value,
                   r.m_legitimite.value, r.m_cohesion.value, label_assise(r.assise));

        /* MÉTRIQUES PAR EMPIRE (chaque empire vivant, trié par taille) — métriques 0-100,
         * puis le DÉTAIL : population par classe · or (trésor + flux/mois) · armée ·
         * usage du marché inter-pays (hub, routes, import/export) · entrepôts. */
        {
            int idx[SCPS_MAX_COUNTRY], ne=0;
            for (int c=0;c<w->n_countries;c++){ PolityRole rl=w->country[c].role;
                if ((rl==POLITY_PLAYER||rl==POLITY_ANTAGONIST) && regions_of(s.econ,c)>0) idx[ne++]=c; }
            for (int a=0;a<ne;a++) for (int b=a+1;b<ne;b++)
                if (regions_of(s.econ,idx[b])>regions_of(s.econ,idx[a])){ int t=idx[a];idx[a]=idx[b];idx[b]=t; }
            printf("              empires vivants (%d) — métriques 0-100 :\n", ne);
            for (int a=0;a<ne;a++){ int c=idx[a];
                CountryReadout cr=country_readout(s.wp,s.ts,w,c);
                int ctech = s.ai_on[c]? s.ai[c].stats.techs : 0;
                printf("                · %-16s %3d rég · pop %5.0fk · Stab %3d Prosp %3d Légit %3d Cohés %3d Corr %3d · %2d tech%s\n",
                       w->country[c].name, regions_of(s.econ,c), ai_country_population(w,s.econ,c)/1000.0,
                       cr.m_stabilite.value, cr.m_prosperite.value, cr.m_legitimite.value, cr.m_cohesion.value,
                       cr.corruption, ctech, (c==tp)?" ★":"");
                /* pyramide de classes (E0.7) + or : trésor & flux net de la dernière année */
                double cp[CLASS_COUNT]; country_class_pop(s.econ, c, cp);
                double cpt=cp[0]+cp[1]+cp[2]; if (cpt<1) cpt=1;
                double g1=country_gold(s.econ,c), flux=(g1-gold_y0[c])/12.0;
                if (g_flux_n<8192){ g_flux_all[g_flux_n]=flux; g_treas_all[g_flux_n]=g1; g_flux_n++; }  /* J2 : pour la médiane */
                printf("                    classes : J %.1fk (%.0f%%) · B %.1fk (%.0f%%) · É %.1fk (%.0f%%) | or %.0f (%+.1f/mois, dern. année) · armée %.0f (%ld rgt)\n",
                       cp[CLASS_LABORER]/1000.0,   100*cp[CLASS_LABORER]/cpt,
                       cp[CLASS_BOURGEOIS]/1000.0, 100*cp[CLASS_BOURGEOIS]/cpt,
                       cp[CLASS_ELITE]/1000.0,     100*cp[CLASS_ELITE]/cpt,
                       g1, flux, diplo_mil_power(w,s.econ,c), warhost_units(s.host,c));
                /* usage du marché inter-pays (dernier tick annuel) + entrepôts (top 4) */
                double vimp=0, vexp=0;
                for (int g=1;g<RES_COUNT;g++){ vimp+=intertrade_import_vol(c,g); vexp+=intertrade_export_vol(c,g); }
                bool hub=intertrade_country_has_centre(s.econ,c);
                int  nrt=intertrade_active_routes(s.econ,s.rn,s.dp,c);
                double stk[RES_COUNT]; country_stocks(s.econ,c,stk);
                double stot=0; for (int g=1;g<RES_COUNT;g++) stot+=stk[g];
                int o4[4]={-1,-1,-1,-1};
                for (int g=1;g<RES_COUNT;g++)
                    for (int t=0;t<4;t++)
                        if (o4[t]<0 || stk[g]>stk[o4[t]]){ for (int u=3;u>t;u--) o4[u]=o4[u-1]; o4[t]=g; break; }
                printf("                    marché : hub %s · %d route(s) · import %.1f · export %.1f (+%.0f or/an) | stocks Σ %.0f —",
                       hub?"OUI":"non", nrt, vimp, vexp, intertrade_export_gold(c), stot);
                for (int t=0;t<4;t++) if (o4[t]>=0 && stk[o4[t]]>=0.5)
                    printf(" %s %.0f", resource_name((Resource)o4[t]), stk[o4[t]]);
                printf("\n");
                tot_emp_n++; tot_emp_gold+=g1; tot_emp_flux+=flux;
                tot_emp_imp+=vimp; tot_emp_exp+=vexp; tot_emp_expgold+=intertrade_export_gold(c);
                if (hub) tot_emp_hub++;
            }
        }
        /* VIVIER DE CITÉS-ÉTATS : combien du pool initial reste DISPONIBLE (vivant). */
        { int cs=0; for (int c=0;c<w->n_countries;c++)
              if (w->country[c].role==POLITY_CITY_STATE && regions_of(s.econ,c)>0) cs++;
          printf("              vivier cités-états : %d disponibles / %d au départ (%d absorbées)\n",
                 cs, n_city, n_city-cs); }
        /* P3.20/E3 — LES PREMIERS HUBS VIVANTS : part du commerce mondial passée
         * par les Centres des CITÉS-ÉTATS (valeur du dernier tick annuel). */
        { double cs_val=0, all_val=0;
          for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++){
              float v=intertrade_centre_value(r);
              if (v<=0.f) continue;
              all_val+=v;
              int o=s.econ->region[r].owner;
              if (o>=0 && o<w->n_countries && w->country[o].role==POLITY_CITY_STATE) cs_val+=v;
          }
          printf("              hubs : %.0f%% du commerce mondial passe par les Centres des cités-états (%.0f / %.0f)\n",
                 all_val>0? 100.0*cs_val/all_val : 0.0, cs_val, all_val);
          tot_hub_cs+=cs_val; tot_hub_all+=all_val; }

        /* POPULATION : totale + par continent (les 4 plus peuplés). */
        {
            double pc[SCPS_MAX_CONTINENT]; continent_pop(w, s.econ, pc, cont);
            int ord[SCPS_MAX_CONTINENT]; for (int i=0;i<cont;i++) ord[i]=i;
            for (int i=0;i<cont;i++) for (int j=i+1;j<cont;j++) if (pc[ord[j]]>pc[ord[i]]){int t=ord[i];ord[i]=ord[j];ord[j]=t;}
            printf("              population : %.0fk au total ; par continent :", total_pop(s.econ)/1000.0);
            for (int i=0;i<cont && i<4;i++) printf(" C%d %.0fk", ord[i], pc[ord[i]]/1000.0);
            printf("\n");
            if (getenv("SCPS_CAPDIAG")) {
                double poptot=0, cap_col=0, cap_act=0, fsat_w=0, fsat_p=0, bldlvl=0, effcap=0, nm_w=0; int ncol=0, nact=0;
                for (int r=0;r<s.econ->n_regions;r++){
                    const RegionEconomy *re=&s.econ->region[r];
                    double p=0; for(int cc=0;cc<CLASS_COUNT;cc++) p+=re->strata[cc].pop;
                    poptot+=p;
                    if (re->active)   { nact++; cap_act+=re->cap_pop; }
                    if (re->colonized){ ncol++; cap_col+=re->cap_pop; fsat_w+=re->food_sat*p; fsat_p+=p; nm_w+=re->needs_met*p;
                        double mh=0; for(int b=0;b<re->n_bld;b++){ bldlvl+=re->bld[b].level; mh+=re->bld[b].level; }
                        mh = fmin(mh*100.0, re->cap_pop*0.5);                       /* HOUSE_MANUF=100 (diag) */
                        effcap += re->cap_pop*0.5 + mh + re->build.food_cap*250.0;  /* eff_cap réel (Q6) */
                    }
                }
                fprintf(stderr,"[FILLDIAG] pop=%.0f | colonisées=%d/%d cap_col=%.0f cap_act=%.0f | remplissage_col=%.0f%% cap_act=%.0f%% | pop/EFF_CAP=%.0f%% | food_sat=%.2f | needs_met=%.2f | Σmanuf_lvl=%.0f\n",
                        poptot, ncol, nact, cap_col, cap_act,
                        cap_col>0?100.0*poptot/cap_col:0, cap_act>0?100.0*poptot/cap_act:0,
                        effcap>0?100.0*poptot/effcap:0,
                        fsat_p>0?fsat_w/fsat_p:0, fsat_p>0?nm_w/fsat_p:0, bldlvl);
            }
        }
        /* EXPANSION : provinces colonisées (vierges peuplées) vs PRISES de force. */
        int n_alliances = active_alliances(w, s.econ, s.dp);
        printf("              diplomatie : %d pacte(s) d'alliance actif(s)\n", n_alliances);
        { double csat[CLASS_COUNT]; world_class_sat(s.econ, csat);
          double tradev = intertrade_imports_value(s.econ);
          printf("              satisfaction (pop-pondérée, AVEC distribution) : Laborer %.0f%% · Bourgeois %.0f%% · Élite %.0f%% | commerce inter-pays/an %.0f\n",
                 csat[CLASS_LABORER], csat[CLASS_BOURGEOIS], csat[CLASS_ELITE], tradev);
          for (int c=0;c<CLASS_COUNT;c++) tot_sat[c]+=csat[c];
          tot_trade += tradev; }
        { double cp[CLASS_COUNT]={0};   /* E0.7 parts de classe + E1bis.10 friche */
          for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].colonized)
              for (int c=0;c<CLASS_COUNT;c++) cp[c]+=s.econ->region[r].strata[c].pop;
          double tp=cp[0]+cp[1]+cp[2]; if(tp<1)tp=1;
          printf("              classes (E0.7, départ 80/15/5) : Laborer %.0f%% · Bourgeois %.0f%% · Élite %.0f%% | friche (E1bis.10) : %ld rég impayée(s)\n",
                 100*cp[0]/tp, 100*cp[1]/tp, 100*cp[2]/tp, econ_friche_count()); }
        print_building_census(s.econ);
        printf("              expansion : %d prov colonisées · %d prov TRANSFÉRÉES à la paix · armée finale %.0f\n",
               colonized_provinces(w,s.econ), conq_prov, total_army(w,s.econ));
        /* E1 §9 — la loi prix/durée se LIT : année du premier édifice par palier. */
        printf("              accession (1er édifice bâti) : 360 j an %d · 540 j an %d · 960 j an %d  (-1 = jamais)\n",
               tier_year[0], tier_year[1], tier_year[2]);
        /* E3 §16 — l'IA stockeuse : volumes, or d'arbitrage, et LE LISSAGE (σ). */
        { float sv=0, sgold=0; int sb=0, ss=0, nent=0;
          for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++) if (s.ai_on[c]){
              sv+=s.ai[c].stats.spec_vol; sgold+=s.ai[c].stats.spec_gold;
              sb+=s.ai[c].stats.spec_buys; ss+=s.ai[c].stats.spec_sells; }
          for (int r=0;r<s.econ->n_regions;r++) nent+=s.econ->region[r].n_entrepot;
          double sd0=(sg_n[0]>1)? sqrt(sg_m2[0]/(sg_n[0]-1)) : 0.0;
          double sd1=(sg_n[1]>1)? sqrt(sg_m2[1]/(sg_n[1]-1)) : 0.0;
          /* moyenne des σ TEMPORELS par région, tranchée ENTRE CENTRES COMMERCIAUX
           * seulement (même classe de région — l'IA bâtit l'entrepôt là où ça
           * brasse : comparer un hub à un arrière-pays mesurerait la hubness). */
          double sp0=0, sp1=0; int np0=0, np1=0;
          for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++){
              if (pr_n[r]<=10.0) continue;                /* trop jeune : pas de σ honnête */
              if (!intertrade_has_centre(r)) continue;    /* centres vs centres, rien d'autre */
              double sd=sqrt(pr_m2[r]/(pr_n[r]-1.0));
              if (s.econ->region[r].n_entrepot>0){ sp1+=sd; np1++; }
              else                                { sp0+=sd; np0++; }
          }
          sp0 = np0? sp0/np0 : 0.0;
          sp1 = np1? sp1/np1 : 0.0;
          printf("              spéculation (E3) : %d entrepôt(s) · vol %.0f (%.1f/an) · or net %+.0f · %d achat(s)/%d vente(s) | σ prix avant %.3f (%d ans) → après %.3f (%d ans)\n",
                 nent, sv, sv/(float)years, sgold, sb, ss,
                 sd0, (int)sg_n[0], sd1, (int)sg_n[1]);
          printf("              lissage CENTRES (même époque, même classe) : σ centres À entrepôt %.3f vs centres SANS %.3f\n",
                 sp1, sp0);
          tot_spec_vol+=sv; tot_spec_gold+=sgold; tot_spec_ent+=nent;
          if (sg_n[0]>1){ tot_sd0+=sd0; tot_sd0_n++; }
          if (sg_n[1]>1){ tot_sd1+=sd1; tot_sd1_n++; }
          if (np1>0){ tot_sp1+=sp1; tot_sp1_n++; }
          if (np0>0){ tot_sp0+=sp0; tot_sp0_n++; } }
        /* TÉLÉMÉTRIE PAR ÂGE : le marché, l'or par empire et la tech à chaque avènement. */
        { bool any=false; for (int a=0;a<AGE_COUNT;a++) if (age_snap[a].year>=0) any=true;
          if (any){
            printf("              ── par âge (instantané à l'avènement) ──\n");
            int ord[AGE_COUNT]; for (int a=0;a<AGE_COUNT;a++) ord[a]=a;   /* tri chronologique */
            for (int a=0;a<AGE_COUNT;a++) for (int b=a+1;b<AGE_COUNT;b++){
                int ya=age_snap[ord[a]].year, yb=age_snap[ord[b]].year;
                if (ya<0) ya=99999;
                if (yb<0) yb=99999;
                if (yb<ya){ int t=ord[a];ord[a]=ord[b];ord[b]=t; }
            }
            for (int oi=0;oi<AGE_COUNT;oi++){ int a=ord[oi];
                const AgeSnap *sn=&age_snap[a]; if (sn->year<0) continue;
                printf("                an %3d %-24s | pays %2d · pop %5.0fk · or Σ %7.0f · PIB %6.0f · tech %3d\n",
                       sn->year, age_name((AgeId)a), sn->living, sn->pop_total/1000.0,
                       sn->gold_total, sn->gdp_total, sn->tech_total);
                printf("                       marché : grain %.2f · étoffe %.2f · orfèvr. %.2f · outils %.2f | or/empire :",
                       sn->p_grain, sn->p_cloth, sn->p_ware, sn->p_tools);
                for (int t=0;t<sn->n_top;t++) printf(" %s %.0f(%dr)", sn->top_name[t], sn->top_gold[t], sn->top_reg[t]);
                printf("\n");
            }
          }
        }
        /* POURQUOI les révoltes : la cause LUE (légitimité ? capacité ?). */
        {
            int nr,ns; float Lr,Kr,SIr,Ls,Ks,SIs;
            revolt_cause(w, s.econ, s.wp, &nr,&Lr,&Kr,&SIr, &ns,&Ls,&Ks,&SIs);
            printf("              révoltes : %d en révolution (Légit moy %.1f · capacité K %.1f · stab.int SI %.1f) "
                   "vs %d stables (Légit %.1f · K %.1f · SI %.1f)\n",
                   nr,Lr,Kr,SIr, ns,Ls,Ks,SIs);
        }
        /* SOULÈVEMENTS INCARNÉS : qui s'est levé et ce qu'il est advenu (acteurs réels). */
        printf("              soulèvements : %d allumés → %d sécession(s) · %d coup(s) · %d concession(s) · %d écrasé(s) (%ld morts au combat)\n",
               s.rs->n_ignited, s.rs->n_seceded, s.rs->n_coup, s.rs->n_concession, s.rs->n_crushed, s.rs->pop_lost);
        /* §C3 — LA CONCESSION A UN PRIX, EN CLAIR : le rot s'accumule sur la polity qui
         * CÈDE (souvent une marche faible ou une cité-état, PAS le 1er empire), donc on
         * balaie TOUTES les polities vivantes — pas seulement les empires affichés plus
         * haut — sinon la capture reste invisible là où elle mord vraiment. */
        {
            int worst_c=-1, worst_corr=0, n_captured=0;
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (w->country[c].role==POLITY_UNCLAIMED || regions_of(s.econ,c)==0) continue;
                int cor = faction_corruption_0_100(c);
                if (cor>=CORR_CAPTURED) n_captured++;
                if (cor>worst_corr){ worst_corr=cor; worst_c=c; }
            }
            if (worst_c>=0 && worst_corr>0)
                printf("              corruption : %d polity(s) capturée(s) (corr≥%d) · la plus pourrie « %s » corr %d — tenue par les %s\n",
                       n_captured, CORR_CAPTURED, w->country[worst_c].name, worst_corr,
                       faction_name(faction_captor(worst_c)));
            else
                printf("              corruption : aucune capture notable (l'élite n'a pas eu à céder)\n");
            tot_captured += n_captured; tot_worstcorr += worst_corr;
            if (n_captured>0) worlds_with_capture++;
        }

        /* RECHERCHE : l'arbre VIT — nœuds déverrouillés (dont des bouts faustiens). */
        { int sim_techs=0, sim_faust=0, sim_reloc=0;
          for (int c=0;c<w->n_countries;c++) if (s.ai_on[c]){ sim_techs+=s.ai[c].stats.techs; sim_faust+=s.ai[c].stats.techs_faustian; sim_reloc+=s.ai[c].stats.relocations; }
          printf("              recherche : %d nœuds déverrouillés (dont %d faustiens) · %d relocalisation(s) pour combler une pénurie (peupler sa province-ressource)\n", sim_techs, sim_faust, sim_reloc);
          tot_techs += sim_techs; tot_faustian += sim_faust; tot_reloc += sim_reloc; }

        /* PRÉVISION (pipeline IA éco étages 1-2) : l'IA n'est plus AVEUGLE de ses flux. Le forecast
         * distingue le STRUCTUREL (cap < conso à PLEIN eff_cap : le manque PERMANENT qui ARME
         * import/colonisation) du transitoire. On surface l'existentiel — le déficit VIVRIER
         * structurel (ne peut se nourrir même à plein → import vital) + la tension de runway. */
        { int n_foodstruct=0, n_foodten=0, nctry=0;
          for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
              if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
              nctry++;
              EconForecast fc; econ_country_forecast(s.econ, c, 25.f, &fc);
              if (fc.struct_deficit[RES_GRAIN] || fc.struct_deficit[RES_FISH] || fc.struct_deficit[RES_LIVESTOCK]) n_foodstruct++;
              if (fc.food_runway < 12.f) n_foodten++;
          }
          printf("              prévision : %d/%d pays en déficit vivrier STRUCTUREL (ne se nourrit pas à plein → import vital) · %d sous tension de runway (< 12 ans)\n",
                 n_foodstruct, nctry, n_foodten); }

        /* MÉTABOLISATION (Temps 1) : le creuset DIGÉRÉ accélère la recherche. Le signal
         * est ~0 tôt (l'assimilation prend des décennies) → la fenêtre golden ne bouge
         * pas ; on MESURE ici qu'il DÉCOLLE sur le long cours (la preuve d'équilibre). */
        { int nm=0, ncreuset=0; float sm=0.f, mx=0.f;
          for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
              if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
              nm++;
              float m = econ_country_metabolized(w, s.econ, c);
              sm += m; if (m>mx) mx=m; if (m>0.01f) ncreuset++;
          }
          float wgt = tune_f("AI_METAB_RES_W",AI_METAB_RES_W);
          printf("              métabolisation : %d/%d empire(s) creuset (>1%% digéré) · moyenne %.1f%% · max %.1f%% → +%.1f%% recherche au plus métabolisé\n",
                 ncreuset, nm, nm?sm/nm*100.f:0.f, mx*100.f, mx*wgt*100.f); }

        /* COMBOS tier-4 (fusion de 2 héritages métabolisés/possédés) : la PREUVE que la matrice
         * de combos s'allume sur le long cours — combien d'empires en tiennent ≥1, et combien de
         * combos au total (Forge runique + les 14 paires). Rare (double accès plein + coût tier-4). */
        { int ncombo_emp=0, ncombo_tot=0;
          for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
              if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
              int got=0;
              for (int id=TECH_FORGE_RUNES; id<TECH_COUNT; id++){
                  if (id!=TECH_FORGE_RUNES && id<TECH_COMBO_POUDRE) continue;   /* FORGE_RUNES + la plage COMBO */
                  if (s.ts[c].unlocked[id]) got++;
              }
              if (got>0){ ncombo_emp++; ncombo_tot+=got; }
          }
          printf("              combos tier-4 : %d empire(s) tiennent une fusion d'héritages · %d combo(s) au total\n",
                 ncombo_emp, ncombo_tot); }

        /* REMISE DE DIFFUSION (métabolisation, 3e effet) : le savoir RÉPANDU se (re)découvre moins
         * cher — combien de techs sont escomptées (≥5 %) et la remise la plus forte (g_tech_diff
         * rafraîchi au dernier tick par sim_day). La PREUVE que le catch-up des retardataires vit. */
        { int ndisc=0; float best=0.f;
          for (int id=0; id<TECH_COUNT; id++){
              float m = tech_diffusion_mult((TechId)id);
              if (m < 0.95f) ndisc++;
              if (1.f-m > best) best = 1.f-m;
          }
          printf("              remise diffusion : %d tech(s) escomptée(s) (−5%%+) · remise max −%.0f%%\n",
                 ndisc, best*100.f); }

        /* CALIBRATION coût-tech-N (gated SCPS_NDIAG) : la relation pop↔provinces par empire,
         * pour caler k de tech_cost(N) — l'ancien coût ∝ pop/5000 (popf), le neuf ∝ √N. */
        if (getenv("SCPS_NDIAG")){
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
                int nreg=0; double pop=0;
                for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].owner==c){
                    nreg++;
                    pop += s.econ->region[r].strata[CLASS_LABORER].pop
                         + s.econ->region[r].strata[CLASS_BOURGEOIS].pop
                         + s.econ->region[r].strata[CLASS_ELITE].pop;
                }
                if (nreg<=0) continue;
                printf("              NDIAG c%d : N=%2d pop=%6.0f pop/N=%5.0f tech=%2d coût(Académie t2)=%.0f\n",
                       c, nreg, pop, pop/nreg, s.ts[c].n_unlocked, tech_cost(TECH_ACADEMIE, (float)nreg));
            }
        }

        /* LEVIERS & SUZERAINETÉ (brief leviers) : l'usage par sim — sans ces lignes,
         * on ne sait ni si l'IA s'en sert, ni si elle s'en sert TROP. */
        { int rep,ass,pur; long dead; agency_levier_stats(&rep,&ass,&pur,&dead);
          printf("              leviers : %d matage(s) · %d formation(s) · %d purge(s) (%ld morts) | suzeraineté : %d servage · %d protectorat · %d concordat · %d cité · %d défection(s) · %d annexion(s) par digestion\n",
                 rep, ass, pur, dead,
                 s.dp->n_servage, s.dp->n_protectorat, s.dp->n_concordat, s.dp->n_cite, s.dp->n_defections, s.dp->n_annex);
          { int ndette=0, nlien=0;
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (country_gold(s.econ,c) < 0.0) ndette++;
                if (credit_of(c)>=0) nlien++;
            }
            printf("              dette : %d débiteur(s) (or net < 0) · %d créancier(s) assigné(s) [scps_credit]\n", ndette, nlien); }
          printf("              fronde : %d ligue(s) · %d fronde(s) → %d indépendance(s) · %d RENVERSEMENT(s) · %d écrasée(s) | défections %d paisibles · %d par reprise | contre-leviers : %d don · %d allègement · %d division · %d intimidation\n",
                 s.dp->n_ligues, s.dp->n_frondes, s.dp->n_indep, s.dp->n_renvers, s.dp->n_ecrase,
                 s.dp->n_defect_paix, s.dp->n_defect_guerre,
                 s.dp->n_lev_don, s.dp->n_lev_allege, s.dp->n_lev_divise, s.dp->n_lev_intim);
          tot_ligues+=s.dp->n_ligues; tot_frondes+=s.dp->n_frondes; tot_indep+=s.dp->n_indep;
          tot_renvers+=s.dp->n_renvers; tot_ecrase+=s.dp->n_ecrase;
          /* GUERRES MOTIVÉES (pipeline diplo) : le casus belli DIT le pourquoi — la part
           * ÉCONOMIQUE (convoitise d'un bien, étage 2) à côté de la territoriale/subjugation. */
          { int wc[CB_ANTIPIRATERIE+1]; diplo_war_cb_counts(wc);
            printf("              guerres motivées : %d territoriale(s) · %d économique(s) · %d subjugation · %d religieuse(s) · %d anti-piraterie\n",
                   wc[CB_TERRITORIAL], wc[CB_ECONOMIC], wc[CB_SUBJUGATION], wc[CB_RELIGIOUS], wc[CB_ANTIPIRATERIE]);
            for (int i=0;i<=CB_ANTIPIRATERIE;i++) tot_wcb[i]+=wc[i]; }

        /* BATAILLES DANS LE TEMPS (§8) : durées, déroutes, et LA vérif — la poursuite
         * doit dominer le choc, sinon on a juste ralenti l'ancien modèle. */
        printf("              batailles : %d livrée(s) · %.0f j en moy. · %d déroute(s) · %d ralliement(s) · %d décrochage(s) · %d renfort(s) · %d nul(s) | morts : %ld au CHOC vs %ld en POURSUITE\n",
               s.camp->n_battles, s.camp->n_battles? (double)s.camp->battle_days/s.camp->n_battles:0.0,
               s.camp->n_routs, s.camp->n_rallies, s.camp->n_disengage, s.camp->n_reinforce, s.camp->n_stalemate,
               s.camp->dead_choc, s.camp->dead_pursuit);
        tot_bt+=s.camp->n_battles; tot_btj+=s.camp->battle_days; tot_routs+=s.camp->n_routs; tot_rallies+=s.camp->n_rallies;
        tot_mchoc+=s.camp->dead_choc; tot_mpour+=s.camp->dead_pursuit;
        tot_deseng+=s.camp->n_disengage; tot_renf+=s.camp->n_reinforce; tot_nul+=s.camp->n_stalemate;
          tot_repress+=rep; tot_assim+=ass; tot_purge+=pur; tot_purge_dead+=dead;
          tot_serv+=s.dp->n_servage; tot_prot+=s.dp->n_protectorat; tot_conc+=s.dp->n_concordat;
          tot_cite+=s.dp->n_cite; tot_defect+=s.dp->n_defections; tot_annex+=s.dp->n_annex; }

        /* ARBRE (§A) : fraction de l'arbre déverrouillée PAR EMPIRE (cible < 100 % → l'empire
         * doit CHOISIR) + thème DOMINANT (deux empires aux choix différents → divergence). */
        { int nemp=0, fmin=999, fmax=0; long fsum=0; int dom[THM_COUNT]={0};
          for (int c=0;c<w->n_countries;c++){
              if (!s.ai_on[c] || regions_of(s.econ,c)==0) continue;
              int pct=(TECH_COUNT>0)?100*s.ts[c].n_unlocked/TECH_COUNT:0;
              fsum+=pct; if(pct<fmin)fmin=pct; if(pct>fmax)fmax=pct; nemp++;
              int th_cnt[THM_COUNT]={0};
              for (int id=0;id<TECH_COUNT;id++) if (s.ts[c].unlocked[id] && !tech_is_base((TechId)id)){
                  TechTheme th=tech_node((TechId)id)->theme; if(th>=0&&th<THM_COUNT) th_cnt[th]++; }
              int best=0; for (int t=1;t<THM_COUNT;t++) if (th_cnt[t]>th_cnt[best]) best=t;
              dom[best]++;
          }
          if (nemp>0){
              printf("              arbre : %ld%% déverrouillé/empire (min %d%% · max %d%%) · spécialisation — %d Savoir · %d Forge · %d Société\n",
                     fsum/nemp, fmin, fmax, dom[THM_SAVOIR], dom[THM_FORGE], dom[THM_SOCIETE]);
              tot_tree_pct += fsum/nemp; tot_tree_sims++;
          }
        }

        /* SYNCRÉTISME (§tech culturelle) : les nœuds à PORTE D'ARCHÉTYPE (ex-signatures de
         * heritage, désormais ouvertes par la CULTURE gouvernée — soi ou contact) — combien
         * acquis, et la DISPERSION entre empires : deux contacts différents → arbres différents. */
        { int sync_total=0, nmax=0, nmin=999, nemp=0, combo=0, diff_total=0; int arch_reached[HERITAGE_COUNT]={0};
          for (int c=0;c<w->n_countries;c++){
              if (!s.ai_on[c] || regions_of(s.econ,c)==0) continue;     /* vivant seulement */
              int n=0;
              for (int id=0; id<TECH_COUNT; id++)
                  if (s.ts[c].unlocked[id] && tech_node((TechId)id)->native!=HERITAGE_COUNT){
                      n++; arch_reached[tech_node((TechId)id)->native]=1; }
              if (s.ts[c].unlocked[TECH_FORGE_RUNES]) combo++;          /* §18.3 : armes enchantées = forge runique × arcane */
              diff_total += s.ts[c].n_sync;                             /* §8 : nœuds de diffusion (contact peu profond) loqués */
              sync_total+=n; if(n>nmax)nmax=n; if(n<nmin)nmin=n; nemp++;
          }
          int distinct=0; for (int ar=0;ar<HERITAGE_COUNT;ar++) distinct+=arch_reached[ar];   /* ar : ne pas masquer le r extérieur (-Wshadow) */
          if (nemp>0){
              printf("              syncrétisme : %d nœud(s) profond(s) (gouvernance) · %d diffusion(s) (commerce/frontière/foi) · %d/%d archétype(s) · dispersion %d–%d/empire · %d ont la COMBINAISON forge runique × arcane · %ld cristallisation(s) culturelle(s) par contact (S2)\n",
                     sync_total, diff_total, distinct, (int)HERITAGE_COUNT, nmin, nmax, combo, demography_contact_count());
              tot_sync += sync_total; tot_sync_distinct += distinct;
          }
        }

        /* CAMPAGNE : les armées ont-elles VÉCU sur la carte (marche/siège/bataille) ?
         * Réductions = sièges menés à terme (enregistrés, PAS appliqués à econ). */
        { int reduced=0, moving=0;
          for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
              reduced += campaign_taken(s.camp,c);
              if (campaign_active(s.camp,c) && campaign_phase(s.camp,c)!=FA_IDLE) moving++;
          }
          printf("              campagne : %d région(s) réduite(s) par les armées de terrain · %d en mouvement (non-invasif)\n",
                 reduced, moving);
          tot_campaign += reduced; }
        { long hulls=0; double sup=0;   /* mer §10 : la chaîne navale TIRE */
          for (int c=0;c<SCPS_MAX_COUNTRY;c++){ hulls+=s.navy->n[c].built_total; sup+=s.navy->n[c].supplies_eaten; }
          int searoutes=0;
          for (int i=0;i<s.rn->n;i++) if (s.rn->route[i].maritime && s.rn->route[i].open) searoutes++;
          long col_om=0;   /* colonies OUTRE-MER : la région vit sur un autre continent que sa couronne */
          for (int r=0;r<s.econ->n_regions && r<w->n_regions;r++){
              const RegionEconomy *re=&s.econ->region[r];
              if (!re->colonized || re->owner<0 || re->owner>=w->n_countries) continue;
              int cp=w->country[re->owner].capital_prov;
              if (cp<0||cp>=w->n_provinces) continue;
              if (w->region[r].continent != w->province[cp].continent) col_om++;
          }
          printf("              mer : %ld coque(s) bâtie(s) · %.0f fournitures navales consommées · %d traversée(s) (%.0f j/trav.) · %d route(s) maritime(s) · %ld colonie(s) outre-mer\n",
                 hulls, sup, s.camp->n_sails + s.navy->n_colony_sails,
                 (s.camp->n_sails + s.navy->n_colony_sails>0)
                   ? ((double)s.camp->sail_days_sum + s.navy->colony_sail_days)
                     / (s.camp->n_sails + s.navy->n_colony_sails) : 0.0,
                 searoutes, col_om);
          tot_hulls+=hulls; tot_supplies+=sup; tot_sails+=s.camp->n_sails + s.navy->n_colony_sails;
          tot_saildays+=s.camp->sail_days_sum + s.navy->colony_sail_days; tot_searoutes+=searoutes; tot_colonies_om+=col_om; }
        { long raids=0, prises=0, navals=0, disarmed=0; double loot=0, blocj=0; int pirates=0, nids=0;
          long intercepts=0, drowned=0, crews=0;
          for (int c=0;c<SCPS_MAX_COUNTRY;c++){ const Navy *nv=&s.navy->n[c];
              raids+=nv->raids_done; prises+=nv->prises; navals+=nv->navals; disarmed+=nv->disarmed;
              loot+=nv->loot_gold; blocj+=nv->blocus_days;
              intercepts+=nv->intercepts; drowned+=nv->drowned; crews+=nv->crew;
              pirates+=nv->hull[HULL_PIRATE]; if (nv->nest_region>=0) nids++; }
          int balafres=0; for (int r=0;r<s.econ->n_regions;r++) if (s.econ->region[r].balafre_days>0.f) balafres++;
          printf("              course : %ld raid(s) (%.0f or pillés) - %d pirate(s) (%d nid(s) en eaux mortes) - %ld bataille(s) navale(s) - %ld prise(s) - %.0f j de blocus - %ld désarmement(s) - %d guerre(s) anti-piraterie - %d balafre(s) actives\n",
                 raids, loot, pirates, nids, navals, prises, blocj, disarmed, s.dp->n_war_antipirate, balafres);
          printf("              marine : %ld marin(s) embarqués - %ld interception(s) - %ld paquet(s) noyés\n",
                 crews, intercepts, drowned);
        { float vd,vu,bd,bu,pd,pu;   /* commerce asym. §6 : le tri par SENS, mesuré */
          intertrade_asym_stats(&vd,&vu,&bd,&bu,&pd,&pu);
          int nfluv=0, nmar=0;
          for (int i=0;i<s.rn->n;i++){ if (s.rn->route[i].fluvial) nfluv++; if (s.rn->route[i].maritime&&s.rn->route[i].open) nmar++; }
          printf("              commerce asym. : aval %.2f vs amont %.2f (ratio %.1fx) - vrac %.2f/%.2f - précieux %.2f/%.2f (aval/amont, %d remontée(s) de luxe) - %d route(s) fluviale(s), %d maritime(s) ouverte(s)\n",
                 vd, vu, (vu>0.01f)?vd/vu:0.f, bd, bu, pd, pu, intertrade_precious_upstream_events(), nfluv, nmar);
          /* le Carrefour des EMBOUCHURES : les estuaires doivent monter d'eux-mêmes */
          { int top_est=0;
            for (int r=0;r<s.econ->n_regions;r++){
                if (!s.econ->region[r].estuary) continue;
                if (routes_count_for_region(s.rn,r)>=2) top_est++;
            }
            printf("              entrepôts : %d estuaire(s) en carrefour (≥2 routes)\n", top_est); }
          /* LA VÉRIF des briefs : un continent PLUS RICHE qu'un autre (géographie, pas script) */
          { double cw[SCPS_MAX_CONTINENT]={0}; double cpop[SCPS_MAX_CONTINENT]={0};
            for (int r=0;r<s.econ->n_regions && r<w->n_regions;r++){
                int ct2=w->region[r].continent;
                if (ct2<0||ct2>=SCPS_MAX_CONTINENT) continue;
                cw[ct2]+=s.econ->region[r].treasury;
                for (int k=0;k<CLASS_COUNT;k++) cpop[ct2]+=s.econ->region[r].strata[k].pop;
            }
            printf("              continents :");
            for (int ct2=0;ct2<w->n_continents && ct2<SCPS_MAX_CONTINENT;ct2++)
                if (cpop[ct2]>0) printf(" C%d %.0f or (%.0fk hab)", ct2, cw[ct2], cpop[ct2]/1000.0);
            printf("\n"); }
          /* WG — LES DÉTROITS : combien le monde en porte, combien sont TENUS (région-flanc
           * possédée), et le PÉAGE encaissé au dernier tick par le meilleur tenant. */
          { const Chokepoint *ck=NULL; int nck=world_chokepoints(w,&ck);
            int16_t ownmap[SCPS_MAX_REG];
            for (int r=0;r<s.econ->n_regions && r<SCPS_MAX_REG;r++) ownmap[r]=s.econ->region[r].owner;
            int held=0;
            for (int k=0;k<nck;k++){
                int h=world_chokepoint_holder(w,k,ownmap, (s.econ->n_regions<SCPS_MAX_REG)?s.econ->n_regions:SCPS_MAX_REG);
                if (h>=0) held++;
            }
            /* le PÉAGE : cumul de la SIM (la valeur probante — un tick converge vers ~0
             * quand les prix s'égalisent, mais le cumul DEMEURE) + le meilleur tenant. */
            double toll_cumul=intertrade_choke_toll_cumul_total(); int toll_routes=intertrade_choke_routes();
            int best_holder=-1; double best_toll=0.0;
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                double t=intertrade_choke_toll_cumul_country(c);
                if (t>best_toll){ best_toll=t; best_holder=c; }
            }
            printf("              détroits : %d goulet(s) émergent(s) · %d tenu(s) · %d route(s) maritime(s) taxée(s) · péage CUMULÉ %.0f or", nck, held, toll_routes, toll_cumul);
            if (best_holder>=0)
                printf(" · meilleur tenant : pays %d a encaissé %.0f or au verrou", best_holder, best_toll);
            printf("\n");
            if (getenv("SCPS_CHOKEDIAG")){   /* WG : pourquoi le péage tombe (ou pas) — routes maritimes vs détroits */
                int mar_open=0, mar_choke=0, mar_third=0, mar_party=0, mar_vierge=0;
                for (int i=0;i<s.rn->n;i++){ const TradeRoute *rt=&s.rn->route[i];
                    if (!rt->maritime||!rt->open) continue;
                    mar_open++;
                    if (rt->choke_region<0) continue;
                    mar_choke++;
                    int h=(rt->choke_region<s.econ->n_regions)?s.econ->region[rt->choke_region].owner:-1;
                    int ca=(rt->ra<s.econ->n_regions)?s.econ->region[rt->ra].owner:-1;
                    int cb=(rt->rb<s.econ->n_regions)?s.econ->region[rt->rb].owner:-1;
                    if (h<0) mar_vierge++; else if (h==ca||h==cb) mar_party++; else mar_third++;
                }
                printf("              [CHOKEDIAG] %d route(s) mar. ouverte(s) · %d franchissent un détroit · tenant TIERS %d · tenant=partie %d · goulet vierge %d\n",
                       mar_open, mar_choke, mar_third, mar_party, mar_vierge);
            }
            tot_chokes+=nck; tot_choke_held+=held; tot_choke_routes+=toll_routes; tot_choke_toll+=toll_cumul;
            if (toll_cumul>0.0) worlds_with_choke_toll++;
          } }
          tot_raids+=raids; tot_loot+=loot; tot_prises+=prises; tot_navals+=navals;
          tot_disarm+=disarmed; tot_warpir+=s.dp->n_war_antipirate; tot_balafres+=balafres;
          tot_intercepts+=intercepts; tot_drowned+=drowned; }

        for (int t=0;t<3;t++) if (tier_year[t]>=0){ tot_tier_y[t]+=tier_year[t]; tot_tier_n[t]++; }
        tot_alliances += n_alliances;
        tot_wars += war_onsets; tot_absorbed += absorbed; tot_emerged += emerged; tot_peakrev += peak_rev; tot_ages += nages;
        tot_conq += conq_prov;
        tot_ignited += s.rs->n_ignited; tot_seceded += s.rs->n_seceded; tot_coup += s.rs->n_coup;
        tot_concession += s.rs->n_concession; tot_crushed += s.rs->n_crushed; tot_revdead += s.rs->pop_lost;
        /* RELIGION : foi(s) fondée(s) (racines) + schismes + pays fidèles + régions minoritaires. */
        { int rr_roots = religion_root_count();
          tot_relig_roots   += rr_roots;
          tot_relig_schisms += (g_religion_count - rr_roots);
          for (int c=0;c<w->n_countries;c++) if (religion_of_country(c)>=0) tot_relig_faith++;
          for (int r=0;r<s.econ->n_regions;r++){ int rg=religion_of_region(r), o=s.econ->region[r].owner;
              if (rg>=0 && o>=0 && rg!=religion_of_country(o)) tot_relig_minority++; } }
        if (age_year[AGE_ORDRE_FER]>=0)   worlds_with_ironorder++;
        if (age_year[AGE_SOULEVEMENTS]>=0) worlds_with_uprising++;
        /* A5 — L'HÉGÉMON MORTEL : ce monde a-t-il vu un grand empire (≥10 rég)
         * passer sous Stabilité 50 OU subir un coup/renversement ? (preuve que
         * la fragilité A1 mord : le géant n'est plus éternellement lisse.) */
        { bool cracked = (hegemon_max_reg>=10 && hegemon_stab_floor<50)
                       || s.rs->n_coup>0 || s.dp->n_renvers>0;
          if (cracked) worlds_hegemon_cracked++;
          if (hegemon_max_reg>=10){ tot_hegemon_floor+=hegemon_stab_floor; n_hegemon_sims++; }
          printf("              hégémon (A5) : 1er empire %d rég · Stabilité plancher %d%s\n",
                 hegemon_max_reg, hegemon_max_reg>=10?hegemon_stab_floor:-1,
                 cracked?" — CRAQUÉ (sous 50 ou coup/renversement)":"");
        }
        /* LE DIRECTEUR (§F) : combien d'événements dirigés, par pool, et le plus
         * fréquent de chaque famille ; la température finale ; l'anti-acharnement
         * (le dépassement DOIT rester 0). */
        { const Director *D=&s.ev->director;
          int top_e=-1,top_n=0, total=0;
          for (int e=0;e<DIR_EV_COUNT;e++){ total+=D->fired[e]; if (D->fired[e]>top_n){top_n=D->fired[e];top_e=e;} tot_dir_fired[e]+=D->fired[e]; }
          int toppct = total? (100*top_n)/total : 0;
          /* G0.1 preuves : top ≤30 % · ≥1 stabilisateur si T a franchi 0.5 · distribution. */
          bool stab_ok = (D->max_T<=0.5f) || (D->fired_stab>0);
          if (toppct<=30) worlds_dir_topok++;
          if (stab_ok) worlds_dir_stabok++;
          tot_dir_destab+=D->fired_destab; tot_dir_stab+=D->fired_stab; tot_dir_overcap+=D->neg_over_cap;
          if (toppct>tot_dir_toppct) tot_dir_toppct=toppct;
          printf("              directeur (F) : T fin %.2f (pic %.2f) · %d total — top %s %d%% (≤30) · %d déstab · %d stab%s · acharnement %d (=0)\n",
                 director_temperature(s.ev), D->max_T, total, top_e>=0?director_event_name(top_e):"—", toppct,
                 D->fired_destab, D->fired_stab, stab_ok?"":" ✗(T>0.5 sans stab)", D->neg_over_cap);
        }
        /* §G2 — LE DIRECTEUR-AMPLITUDE : l'amplitude dramatique a-t-elle vécu (pic)
         * et où finit-elle (le monde s'est-il calmé) ; combien de présages la boucle
         * « tale » a émis (fait NOTABLE → MÉMOIRE → AUGURE). La courbe année-par-année
         * est sous SCPS_AMPLDIAG (l'amplitude monte au choc, retombe au calme). */
        { const Director *D=&s.ev->director;
          tot_ampl_pic+=D->max_amplitude; tot_ampl_fin+=D->amplitude; tot_omens+=D->omens;
          if (D->max_amplitude>0.3f) worlds_ampl_vibre++;
          printf("              amplitude (G2) : pic %.2f · fin %.2f%s · trauma %.0f j · budget %.0f · %d présage(s) · %d mémoire(s)\n",
                 D->max_amplitude, D->amplitude,
                 (D->amplitude < D->max_amplitude-0.1f)?" (le monde s'est calmé)":"",
                 D->adapt_days, D->budget, D->omens, director_memories(s.ev));
        }
        /* §C — l'inflation monétaire : l'IPM final (1.0 = neutre ; >1 = les prix ont
         * monté, l'or a flué plus vite que les biens). 1.00 partout = effet inerte. */
        { float ipm=econ_world_ipm(s.econ); tot_ipm+=ipm; if(ipm>max_ipm)max_ipm=ipm;
          printf("              inflation (C) : IPM final %.2f%s\n", ipm,
                 ipm>=1.20f?" — l'or a dilué la monnaie":(ipm<=0.85f?" — les biens ont fait baisser les prix":"")); }
        if (hash_mode) printf("HASH %u %08x\n", seed, chronicle_sim_hash(seed, &s, w));
    }

    printf("\n══════════════════════════════════════════════════════════════════════\n");
    printf(" SYNTHÈSE (%d sims × %d ans)\n", nsims, years);
    printf("   âges éveillés (total) ....... %ld   (moy. %.1f/sim)\n", tot_ages, (double)tot_ages/nsims);
    printf("   guerres déclenchées (total) . %ld   (moy. %.1f/sim)\n", tot_wars, (double)tot_wars/nsims);
    printf("   alliances actives (fin de sim) %ld   (moy. %.1f/sim ; la diplomatie respire)\n", tot_alliances, (double)tot_alliances/nsims);
    printf("   satisfaction moy (AVEC distribution) : Laborer %.0f%% · Bourgeois %.0f%% · Élite %.0f%% | commerce/an moy %.0f\n",
           tot_sat[CLASS_LABORER]/nsims, tot_sat[CLASS_BOURGEOIS]/nsims, tot_sat[CLASS_ELITE]/nsims, tot_trade/nsims);
    printf("   par empire (fin de sim) ..... trésor moy %.0f or · flux moy %+.1f or/mois (dern. année) · import moy %.1f · export moy %.1f (+%.0f or/an) · hub tenu %ld/%ld\n",
           tot_emp_n? tot_emp_gold/tot_emp_n:0.0,  tot_emp_n? tot_emp_flux/tot_emp_n:0.0,
           tot_emp_n? tot_emp_imp /tot_emp_n:0.0,  tot_emp_n? tot_emp_exp /tot_emp_n:0.0,
           tot_emp_n? tot_emp_expgold/tot_emp_n:0.0, tot_emp_hub, tot_emp_n);
    /* I0 — LA DÉCOMPOSITION DU FLUX (dernière sim · dernière année · moyenne par empire ·
     * or/mois) : l'instrument du robinet, ligne à ligne. Quand une bande échoue, c'est ICI
     * qu'on lit la ligne dominante — pas un taux au hasard. */
    { int ne=0; double comp[FX_COUNT]={0};
      for (int c=0;c<SCPS_MAX_COUNTRY;c++){ bool act=false;
          for (int k=0;k<FX_COUNT;k++){ double v=econ_flux_get(c,(FluxComp)k); comp[k]+=v; if(v!=0.0) act=true; }
          if (act) ne++; }
      if (ne>0){
          printf("   flux décomposé (I0, dern. année · or/mois/empire) :");
          for (int k=0;k<FX_COUNT;k++) printf(" %s %+.1f", econ_flux_name((FluxComp)k), comp[k]/ne/12.0);
          printf("\n");
      } }
    if (getenv("EDI_DBG")) agency_edi_dump();   /* G0.3 : pourquoi les paliers ne montent pas */
    printf("   accession (E1 §9) ........... 360 j an %.1f (%d/%d) · 540 j an %.1f (%d/%d) · 960 j an %.1f (%d/%d)  (moy. du 1er bâti)\n",
           tot_tier_n[0]? (double)tot_tier_y[0]/tot_tier_n[0]:-1.0, tot_tier_n[0], nsims,
           tot_tier_n[1]? (double)tot_tier_y[1]/tot_tier_n[1]:-1.0, tot_tier_n[1], nsims,
           tot_tier_n[2]? (double)tot_tier_y[2]/tot_tier_n[2]:-1.0, tot_tier_n[2], nsims);
    printf("   l'IA stockeuse (E3 §16) ..... %ld entrepôt(s) · vol spéculé %.0f (%.1f/sim) · or net %+.0f | σ prix avant %.3f → après %.3f (les stocks doivent LISSER)\n",
           tot_spec_ent, tot_spec_vol, tot_spec_vol/nsims, tot_spec_gold,
           tot_sd0_n? tot_sd0/tot_sd0_n : 0.0, tot_sd1_n? tot_sd1/tot_sd1_n : 0.0);
    printf("   lissage CENTRES (E3 §16) .... σ centres À entrepôt %.3f vs centres SANS %.3f (même époque, même classe — la mesure causale)\n",
           tot_sp1_n? tot_sp1/tot_sp1_n : 0.0, tot_sp0_n? tot_sp0/tot_sp0_n : 0.0);
    printf("   hubs des cités-états ........ %.0f%% du commerce mondial passe par leurs Centres (les premiers hubs vivants)\n",
           tot_hub_all>0? 100.0*tot_hub_cs/tot_hub_all : 0.0);
    printf("   provinces transférées à la paix %ld   (moy. %.1f/sim ; la propriété ne change qu'au RÈGLEMENT)\n", tot_conq, (double)tot_conq/nsims);
    printf("   occupations (terrain) ....... %ld posée(s) · %ld levée(s)   (les sièges tiennent le sol entre deux paix)\n", g_tot_occ_posed, g_tot_occ_lifted);
    printf("   pays absorbés (morts) ....... %ld   (moy. %.1f/sim)\n", tot_absorbed, (double)tot_absorbed/nsims);
    printf("   pays émergés (sécession) .... %ld   (moy. %.1f/sim ; la carte politique respire)\n", tot_emerged, (double)tot_emerged/nsims);
    printf("   hameaux libres (WILD) ....... %.1f semés/sim · %ld ralliés culturellement (%.1f/sim · pop moy. %.0f) ; l'absorption MILITAIRE passe par la conquête\n",
           (double)g_wild_spawned/nsims, g_wild_defected, (double)g_wild_defected/nsims,
           g_wild_defected>0?g_wild_absorb_pop/(double)g_wild_defected:0.0);
    printf("   nœuds de tech débloqués ..... %ld   (moy. %.1f/sim ; %ld faustiens)\n", tot_techs, (double)tot_techs/nsims, tot_faustian);
    printf("   arbre déverrouillé / empire . %ld%%   (le coût force les choix : cible < 100 %% → spécialisation)\n",
           tot_tree_sims>0? tot_tree_pct/tot_tree_sims : 0);
    printf("   relocalisations (pénurie) ... %ld   (moy. %.1f/sim ; l'IA peuple ses provinces-ressource sous-exploitées)\n",
           tot_reloc, (double)tot_reloc/nsims);
    printf("   leviers intérieurs .......... %ld matage(s) · %ld formation(s) · %ld purge(s) (%ld morts — RARE attendu)\n",
           tot_repress, tot_assim, tot_purge, tot_purge_dead);
    printf("   suzeraineté ................. %ld servage · %ld protectorat · %ld concordat · %ld cité · %ld défection(s) · %ld annexion(s) par digestion (étage 3)\n",
           tot_serv, tot_prot, tot_conc, tot_cite, tot_defect, tot_annex);
    printf("   guerres motivées ............ %ld territoriale(s) · %ld économique(s) · %ld subjugation · %ld religieuse(s) · %ld anti-piraterie (le casus belli DIT le pourquoi)\n",
           tot_wcb[CB_TERRITORIAL], tot_wcb[CB_ECONOMIC], tot_wcb[CB_SUBJUGATION], tot_wcb[CB_RELIGIOUS], tot_wcb[CB_ANTIPIRATERIE]);
    printf("   fronde vassale .............. %ld ligue(s) · %ld fronde(s) → %ld indép. · %ld renversement(s) · %ld écrasée(s)  (les TROIS fins doivent exister)\n",
           tot_ligues, tot_frondes, tot_indep, tot_renvers, tot_ecrase);
    printf("   batailles dans le temps ..... %ld livrées · %.0f j/bataille · %ld déroutes · %ld ralliement(s) · %ld décrochages · %ld renforts · %ld nuls | morts choc %ld vs POURSUITE %ld (ratio %.1fx — la poursuite doit DOMINER le choc si la cavalerie domine la compo)\n",
           tot_bt, tot_bt? (double)tot_btj/tot_bt:0.0, tot_routs, tot_rallies, tot_deseng, tot_renf, tot_nul,
           tot_mchoc, tot_mpour, tot_mchoc? (double)tot_mpour/tot_mchoc:0.0);
    printf("   syncrétisme culturel ........ %.1f nœud(s)/sim · %.1f archétype(s) distincts/sim (porte = CULTURE, plus heritage ; la diffusion par contact DIVERGE)\n",
           (double)tot_sync/(nsims>0?nsims:1), (double)tot_sync_distinct/(nsims>0?nsims:1));
    printf("   religion .................... %.1f foi(s) fondée(s)/sim · %.1f schisme(s)/sim · %.1f pays fidèle(s)/sim · %.1f région(s) minoritaire(s)/sim (monde ATHÉE au départ ; racines ≤ ⌈empires/3⌉ genèse · ≤ 2 schismes/racine)\n",
           (double)tot_relig_roots/(nsims>0?nsims:1), (double)tot_relig_schisms/(nsims>0?nsims:1),
           (double)tot_relig_faith/(nsims>0?nsims:1), (double)tot_relig_minority/(nsims>0?nsims:1));
    printf("   régions réduites (campagne) . %ld   (moy. %.1f/sim ; armées de terrain, hors conquête abstraite)\n", tot_campaign, (double)tot_campaign/nsims);
    printf("   la mer ...................... %ld coque(s) · %.0f fournitures consommées (NE doit plus être zéro) · %ld traversée(s) (%.0f j moy.) · %ld route(s) maritime(s) · %ld colonie(s) outre-mer\n",
           tot_hulls, tot_supplies, tot_sails, (tot_sails>0)?tot_saildays/(double)tot_sails:0.0, tot_searoutes, tot_colonies_om);
    printf("   détroits (WG) ............... %.1f goulet(s)/sim · %.1f tenu(s)/sim · %ld route(s) taxée(s) · péage CUMULÉ moy. %.0f or/sim · %d/%d sim(s) avec péage ENCAISSÉ (le verrou rapporte à qui le tient)\n",
           (double)tot_chokes/nsims, (double)tot_choke_held/nsims, tot_choke_routes, tot_choke_toll/nsims, worlds_with_choke_toll, nsims);
    printf("   la course ................... %ld raid(s) (%.0f or) · %ld bataille(s) navale(s) · %ld prise(s) · %ld désarmement(s) · %ld guerre(s) anti-piraterie · %ld balafre(s) en fin de sim\n",
           tot_raids, tot_loot, tot_navals, tot_prises, tot_disarm, tot_warpir, tot_balafres);
    printf("   l'interception .............. %ld convoi(s) coulé(s) · %ld paquet(s) noyés (le transport sans escorte est une PROIE)\n",
           tot_intercepts, tot_drowned);
    printf("   pic de révolte moyen ........ %.1f pays\n", (double)tot_peakrev/nsims);
    printf("   soulèvements incarnés ....... %ld allumés → %ld sécession(s) · %ld coup(s) · %ld concession(s) · %ld écrasé(s)\n",
           tot_ignited, tot_seceded, tot_coup, tot_concession, tot_crushed);
    printf("   corruption (le prix des concessions) : %d/%d sims avec ≥1 polity capturée · %.1f capturée(s)/sim · pire-corr moy %.0f/100\n",
           worlds_with_capture, nsims, (double)tot_captured/nsims, (double)tot_worstcorr/nsims);
    printf("   morts au combat (révoltes) .. %ld   (moy. %.0f/sim)\n", tot_revdead, (double)tot_revdead/nsims);
    printf("   sims atteignant les Soulèvements : %d/%d   l'Ordre de Fer : %d/%d\n",
           worlds_with_uprising, nsims, worlds_with_ironorder, nsims);
    printf("   hégémon MORTEL (A5) .......... %d/%d sims où un empire ≥10 rég est passé sous Stab 50 OU a subi coup/renversement · Stab plancher moy %ld (la fragilité A1 mord)\n",
           worlds_hegemon_cracked, nsims, n_hegemon_sims? tot_hegemon_floor/n_hegemon_sims : -1);
    printf("   directeur (F) ............... %ld déstabilisateur(s) · %ld stabilisateur(s) (%.1f/%.1f par sim) · acharnement %ld (DOIT être 0)\n",
           tot_dir_destab, tot_dir_stab, (double)tot_dir_destab/nsims, (double)tot_dir_stab/nsims, tot_dir_overcap);
    { printf("      par événement :");
      for (int e=0;e<DIR_EV_COUNT;e++) if (tot_dir_fired[e]>0)
          printf(" %s×%ld", director_event_name(e), tot_dir_fired[e]);
      printf("\n"); }
    printf("      G0.1 preuves : top ≤30%% dans %d/%d sims (pire %d%%) · stabilisateur-si-T>0.5 dans %d/%d\n",
           worlds_dir_topok, nsims, tot_dir_toppct, worlds_dir_stabok, nsims);
    printf("   directeur-amplitude (G2) .... pic moy %.2f · fin moy %.2f (le monde se calme) · %ld présage(s) (%.1f/sim) · l'amplitude a VIBRÉ dans %d/%d sims [courbe : SCPS_AMPLDIAG]\n",
           tot_ampl_pic/nsims, tot_ampl_fin/nsims, tot_omens, (double)tot_omens/nsims, worlds_ampl_vibre, nsims);
    printf("   inflation (C) ............... IPM final moyen %.2f · pic %.2f (1.00 = neutre ; SCPS_IPM=0 le retire)\n",
           tot_ipm/nsims, max_ipm);
    printf("══════════════════════════════════════════════════════════════════════\n");

    /* ═══ Arc J2 — SORTIE MACHINE (CSV, opt-in via SCPS_CSV=chemin) ═══════════
     * Une ligne SUMMARY par balayage. stdout INCHANGÉ (le CSV va au fichier).
     * Colonnes STABLES (le pilote J3 les lit par nom). « jamais » = -1. */
    { const char *csv = getenv("SCPS_CSV");
      if (csv && *csv){
        double flux_med  = dmedian(g_flux_all,  g_flux_n);
        double tresor_med= dmedian(g_treas_all, g_flux_n);
        double acc360 = tot_tier_n[0]? (double)tot_tier_y[0]/tot_tier_n[0] : -1.0;
        double acc540 = tot_tier_n[1]? (double)tot_tier_y[1]/tot_tier_n[1] : -1.0;
        double acc960 = tot_tier_n[2]? (double)tot_tier_y[2]/tot_tier_n[2] : -1.0;
        double ratio  = (tot_mchoc>0)? (double)tot_mpour/(double)tot_mchoc : -1.0;
        long top_fired=0, tot_fired=0;
        for (int e=0;e<DIR_EV_COUNT;e++){ tot_fired+=tot_dir_fired[e]; if (tot_dir_fired[e]>top_fired) top_fired=tot_dir_fired[e]; }
        double top_share = tot_fired? 100.0*top_fired/tot_fired : -1.0;
        FILE *cf = fopen(csv, "ab");
        if (cf){
            fseek(cf, 0, SEEK_END);
            if (ftell(cf)==0)   /* fichier neuf → en-tête */
                fprintf(cf, "row,seed,sims,years,tune,flux_or_med,tresor_med,ipm_final,ipm_pic,"
                            "acc360,acc540,acc960,ratio_poursuite,batailles,top_event_share,"
                            "n_stab,n_destab,acharnement,hegemon_cracked\n");
            fprintf(cf, "SUMMARY,%u,%d,%d,\"%s\",%.3f,%.1f,%.3f,%.3f,%.2f,%.2f,%.2f,%.3f,%ld,%.1f,%ld,%ld,%ld,%d\n",
                    base, nsims, years, tune_active_string(),
                    flux_med, tresor_med, tot_ipm/nsims, max_ipm,
                    acc360, acc540, acc960, ratio, tot_bt, top_share,
                    tot_dir_stab, tot_dir_destab, tot_dir_overcap, worlds_hegemon_cracked);
            fclose(cf);
        }
      } }

    free(w); free(s.econ); free(s.wp); free(s.wl); free(s.net); free(s.ts); free(s.sc);
    free(s.ag); free(s.ev); free(s.drift); free(s.labor); free(s.dp); free(s.rn);
    warhost_free(s.host); free(s.camp); free(s.ai); free(s.ai_on); free(s.rs); free(s.host);
    free(s.missions);   /* fuyait (6 496 o, vu par LeakSanitizer) */
    free(s.navy); free(s.eg);
    return 0;
}
