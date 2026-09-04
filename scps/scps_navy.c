/*
 * scps_navy.c — LA FLOTTE : trois coques + conversion, chantier, entretien,
 * colonisation outre-mer (briefs mer §5/§8 & coques §2).
 *
 * La flotte est LE consommateur de RES_NAVAL_SUPPLIES (chaîne bois → Scierie
 * navale → fournitures → coques) : l'achat suit le patron d'agency_build —
 * l'or paie le marché, le stock se consomme, la demande se REGISTRE (le marché
 * voit la flotte et la production tire).
 */
#include "scps_navy.h"
#include "scps_tune.h"   /* Arc I9 : entretien naval calibrable */
#include "scps_econ.h"   /* econ_world_ipm */
#include "scps_diplo.h"  /* I9 : country_at_war → entretien ×1.5 en guerre */
#include <string.h>
#include <math.h>
#include "scps_math.h"   /* clampf partagé */

/* ── Surface d'équilibrage ──────────────────────────────────────────────── */
#define NAVY_MIN_PRICE       0.5f     /* plancher de prix (même rôle que BUILD_MIN_PRICE) */
#define NAVY_UPKEEP_WAR      1.5f     /* fournitures / an / coque                  */
#define NAVY_UPKEEP_OTHER    0.8f
#define NAVY_STARVE_YEAR     365.f    /* > 1 an sans entretien : une coque pourrit/an */
/* DÉPOUILLEMENT 2026-08-11 (« ton monde a neuf continents et se joue sur un seul ») :
 * 90 j en dur < la traversée RÉELLE moyenne (96 j mesurés, armées) — l'outre-mer ne
 * tirait qu'un monde sur vingt. 240 j ouvre les continents ; le score ÷(1+jours/15)
 * préfère toujours le proche. */
#define NAVY_COLONY_MAX_DAYS (tune_f("NAVY_COLONY_MAX_DAYS",240.f))
#define NAVY_COLONY_CD       (tune_f("NAVY_COLONY_CD_DAYS",180.f))  /* répit entre deux
                                          * colonies outre-mer (décision joueur 2026-07-31 :
                                          * « imagine une game de civ où la moitié du monde
                                          * est vide » — 2 ANS interdisaient de peupler les
                                          * 170 provinces hors de portée terrestre ; 6 mois
                                          * laisse la FLOTTE et la POP faire le mur, pas une
                                          * horloge). 730 = l'ancien rythme. */
#define NAVY_TRANSPORT_PKTS  10       /* 1 transport = 10 paquets = 1 000 hommes   */

typedef struct { float supplies, wood, copper; int days; } HullCost;   /* la coque de guerre coûte du CUIVRE (clous/doublage), pas un « métal » manufacturé */
static const HullCost HULLS[HULL_COUNT]={
    [HULL_WAR]      ={ 30.f, 40.f, 25.f, 360 },   /* E1.7 : la coque LOURDE — recalée au palier 360 j ; 25 = CUIVRE (ex-métal) */
    [HULL_TRANSPORT]={ 20.f, 30.f,  0.f, 180 },   /* E1 : coque LÉGÈRE — alignée au palier 180 j */
    [HULL_MERCHANT] ={ 15.f, 25.f,  0.f, 180 },   /* E1 : coque LÉGÈRE — alignée au palier 180 j */
    [HULL_PIRATE]   ={  6.f,  8.f,  0.f,  60 },   /* la CONVERSION coûte peu : c'est sa nature */
};
int navy_hull_crew(HullType t){ return (t==HULL_WAR)?NAVY_CREW_WAR:NAVY_CREW_LIGHT; }
const char *navy_hull_name(HullType t){
    static const char *N[HULL_COUNT]={"navire de combat","transport","marchand","pirate"};
    return (t>=0&&t<HULL_COUNT)?N[t]:"?";
}

void navy_init(NavyState *ns){
    memset(ns,0,sizeof *ns);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        ns->n[c].build_hull=-1; ns->n[c].home_port=-1;
        ns->n[c].mission_target=-1; ns->n[c].nest_region=-1;
    }
}

/* Une région est côtière si l'une de ses provinces touche la mer. */
static bool region_coastal(const World *w, int region){
    if (region<0 || region>=w->n_regions) return false;
    const Region *rg=&w->region[region];
    for (int k=0;k<rg->n_provinces;k++){
        int p=rg->province_ids[k];
        if (p>=0 && p<w->n_provinces && w->province[p].coastal) return true;
    }
    return false;
}
bool navy_region_is_port(const World *w, const WorldEconomy *econ, int region){
    if (region<0 || region>=econ->n_regions) return false;
    return econ->region[region].build.port>0.f && region_coastal(w,region);
}
int navy_best_port(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0 || cid>=w->n_countries) return -1;
    int cap_reg=-1;
    { int cp=w->country[cid].capital_prov;
      if (cp>=0 && cp<w->n_provinces) cap_reg=w->province[cp].region; }
    if (cap_reg>=0 && navy_region_is_port(w,econ,cap_reg)
        && econ->region[cap_reg].owner==cid) return cap_reg;
    int best=-1; float bpop=-1.f;
    for (int r=0;r<econ->n_regions;r++){
        const RegionEconomy *re=&econ->region[r];
        if (re->owner!=cid || !navy_region_is_port(w,econ,r)) continue;
        float pop=0.f; for (int c=0;c<CLASS_COUNT;c++) pop+=re->strata[c].pop;
        if (pop>bpop){ bpop=pop; best=r; }
    }
    return best;
}
/* V3/WG — la meilleure CÔTE du pays pour y asseoir une rade, qu'un port y soit DÉJÀ
 * bâti ou non. -1 si le pays n'a aucune région côtière : un empire à capitale enclavée
 * ouvre ainsi quand même sa fenêtre sur la mer (sans cela, pas de port → pas de flotte,
 * pas de route maritime, pas de colonie).
 *
 * WG (worldgen-graphe) — la rade SUIT LA FORME DU LITTORAL : on LIT l'aptitude portuaire
 * (Region.harbor, posée à la genèse : abri de baie + profondeur de rade + longueur de
 * côte) plutôt que la seule pop. Score = harbor (la géographie portuaire) + un appoint de
 * population (une rade vide ne se bâtit pas) + l'AVANTAGE DE SIÈGE de la capitale côtière
 * (le pouvoir préfère son propre port, mais une baie franche ailleurs peut l'emporter
 * sur un cap capital exposé). On lit une coordonnée du monde, on n'assigne pas un bonus. */
int navy_best_coast(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0 || cid>=w->n_countries) return -1;
    int cap_reg=-1;
    { int cp=w->country[cid].capital_prov;
      if (cp>=0 && cp<w->n_provinces) cap_reg=w->province[cp].region; }
    /* échelle de pop pour normaliser l'appoint (la rade la plus peuplée du pays = ~1) */
    float popmax=1.f;
    for (int r=0;r<econ->n_regions;r++){
        const RegionEconomy *re=&econ->region[r];
        if (re->owner!=cid || !region_coastal(w,r)) continue;
        float pop=0.f; for (int c=0;c<CLASS_COUNT;c++) pop+=re->strata[c].pop;
        if (pop>popmax) popmax=pop;
    }
    int best=-1; float bscore=-1.f;
    for (int r=0;r<econ->n_regions;r++){
        const RegionEconomy *re=&econ->region[r];
        if (re->owner!=cid || !region_coastal(w,r)) continue;
        float harbor=(r<w->n_regions)?w->region[r].harbor:0.f;   /* la FORME du littoral */
        float pop=0.f; for (int c=0;c<CLASS_COUNT;c++) pop+=re->strata[c].pop;
        float score = harbor                       /* l'aptitude portuaire DOMINE   */
                    + 0.35f*(pop/popmax)           /* l'appoint de population        */
                    + ((r==cap_reg)?0.40f:0.f);    /* l'avantage de siège (non absolu) */
        if (score>bscore){ bscore=score; best=r; }
    }
    return best;
}

float navy_build_gold(const WorldEconomy *econ, int region, HullType t){
    if (t<0||t>=HULL_COUNT||region<0||region>=econ->n_regions) return 0.f;
    const RegionEconomy *re=&econ->region[region];
    const HullCost *h=&HULLS[t];
    float gold=0.f, p;
    p=re->price[RES_NAVAL_SUPPLIES]; if (p<NAVY_MIN_PRICE) p=NAVY_MIN_PRICE; gold+=h->supplies*p;
    p=re->price[RES_WOOD];           if (p<NAVY_MIN_PRICE) p=NAVY_MIN_PRICE; gold+=h->wood*p;
    if (h->copper>0.f){ p=re->price[RES_COPPER]; if (p<NAVY_MIN_PRICE) p=NAVY_MIN_PRICE; gold+=h->copper*p; }
    return gold;
}

bool navy_order_build(NavyState *ns, const World *w, WorldEconomy *econ, int cid, HullType t){
    if (t<0||t>=HULL_COUNT||cid<0||cid>=SCPS_MAX_COUNTRY) return false;
    Navy *n=&ns->n[cid];
    if (n->build_hull>=0) return false;                  /* un chantier à la fois */
    int port=navy_best_port(w,econ,cid);
    if (port<0) return false;                            /* un pays sans port ne bâtit rien */
    RegionEconomy *re=&econ->region[port];
    float gold=navy_build_gold(econ,port,t);
    if ((float)econ_country_gold(econ,cid) < gold) return false;
    if (re->strata[CLASS_LABORER].pop < (float)navy_hull_crew(t)+200.f) return false;  /* pas les bras */
    const HullCost *h=&HULLS[t];
    /* TRÉSOR/STOCK NATIONAUX (2026-09-03) : le chantier se paie sur LE trésor du pays et
     * puise dans SON entrepôt — plus sur la seule caisse de la rade. Le port ne fournit
     * que le marché (prix), les bras et la demande visible. */
    econ_flux_add(cid, FX_NAVY, econ_nation_gold_add(econ, cid, -gold));   /* I0 : le chantier naval (A1) */
    econ_nation_stock_add(econ, cid, RES_NAVAL_SUPPLIES, -h->supplies);
    econ_nation_stock_add(econ, cid, RES_WOOD,           -h->wood);
    if (h->copper>0.f) econ_nation_stock_add(econ, cid, RES_COPPER, -h->copper);
    re->demand[RES_NAVAL_SUPPLIES]+=h->supplies;         /* le marché VOIT le chantier (flux recalculé/tick, transitoire assumé) */
    re->demand[RES_WOOD]          +=h->wood;
    n->supplies_eaten+=h->supplies;
    /* L'ÉQUIPAGE se lève sur la pop du port (le warhost de la mer) : 50 bras
     * par coque légère, 100 par bordée — vérifié AVANT le paiement. */
    { int crew=navy_hull_crew(t);
      econ_region_pop_add(econ, port, CLASS_LABORER, -(float)crew);
      n->crew += crew; }
    n->build_hull=(int)t; n->build_days=(float)h->days; n->home_port=port;
    return true;
}

bool navy_convert(NavyState *ns, const World *w, WorldEconomy *econ, int cid, bool to_pirate){
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return false;
    Navy *n=&ns->n[cid];
    int port=navy_best_port(w,econ,cid);
    if (port<0) return false;                            /* la conversion se fait AU CHANTIER */
    int from = to_pirate?HULL_MERCHANT:HULL_PIRATE;
    int to   = to_pirate?HULL_PIRATE  :HULL_MERCHANT;
    if (n->hull[from]<=0) return false;
    const HullCost *h=&HULLS[HULL_PIRATE];               /* le coût léger de la conversion */
    float gold=navy_build_gold(econ,port,HULL_PIRATE);
    if ((float)econ_country_gold(econ,cid) < gold) return false;
    /* TRÉSOR/STOCK NATIONAUX (2026-09-03) : la conversion se paie au pays, pas à la rade. */
    econ_flux_add(cid, FX_NAVY, econ_nation_gold_add(econ, cid, -gold));   /* I0 : la conversion (A1) */
    econ_nation_stock_add(econ, cid, RES_NAVAL_SUPPLIES, -h->supplies);
    n->supplies_eaten+=h->supplies;
    n->hull[from]--; n->hull[to]++;
    if (!to_pirate) n->nest_region=-1;                   /* désarmé : le nid se vide */
    return true;
}

void navy_tick(NavyState *ns, const World *w, WorldEconomy *econ, struct DiploState *dp, float dt_days){
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        Navy *n=&ns->n[c];
        bool at_war=false;                               /* I9 : la marine en guerre coûte ×1.5 */
        if (dp) for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++)
            if (b!=c && diplo_status(dp,c,b)==DIPLO_WAR){ at_war=true; break; }
        /* la rade suit la vie du pays (port perdu/conquis → meilleure rade restante) */
        if (n->home_port>=0 && (n->home_port>=econ->n_regions
            || econ->region[n->home_port].owner!=c
            || !navy_region_is_port(w,econ,n->home_port)))
            n->home_port=navy_best_port(w,econ,c);
        if (n->colony_cd>0.f) n->colony_cd-=dt_days;
        /* chantier */
        if (n->build_hull>=0){
            n->build_days-=dt_days;
            if (n->build_days<=0.f){
                n->hull[n->build_hull]++; n->built_total++;
                n->build_hull=-1; n->build_days=0.f;
            }
        }
        /* entretien : fournitures au fil de l'eau, consommées à la rade */
        int hulls=0; float need_y=0.f;
        for (int t=0;t<HULL_COUNT;t++){
            hulls+=n->hull[t];
            need_y += (float)n->hull[t] * ((t==HULL_WAR)?NAVY_UPKEEP_WAR:NAVY_UPKEEP_OTHER);
        }
        if (hulls<=0){ n->starve_days=0.f; continue; }
        if (n->home_port<0){ n->home_port=navy_best_port(w,econ,c); }
        if (n->home_port<0){                              /* sans rade, rien ne s'entretient */
            n->starve_days+=dt_days;
        } else {
            RegionEconomy *re=&econ->region[n->home_port];
            /* I9 — LA MARINE SE PAIE (le sink en OR ; les fournitures en biens restent) :
             * ~1.5 or/mois par coque × IPM. Impayé → la flotte affame (starve_days →
             * désarmement existant plus bas) — c'est le « désarme des coques » d'IG. */
            { float base_gold = (float)hulls * tune_f("NAVY_UPKEEP_GOLD",1.5f)
                              * econ_world_ipm(econ) * (at_war?1.5f:1.f) * (dt_days/30.f);
              float navy_mult = econ_country_budget_mult(econ,c,BUDGET_NAVY);
              float gold = base_gold * navy_mult;
              /* TRÉSOR NATIONAL (2026-09-03) : l'entretien se prélève sur LE trésor du pays
               * (débit borné : le retour dit ce qui a réellement été payé). */
              float paid = -econ_nation_gold_add(econ, c, -gold);
              if (paid < gold) n->starve_days += dt_days;
              /* Sous-payer la marine (curseur NAVY) ne la fait plus se DÉLABRER :
               * elle perd le MORAL au combat (navy_pay_morale, plus bas). */
              econ_flux_add(c, FX_NAVY, -paid);              /* I0 : la ligne marine */
              /* MONNAIE M3b-v2 — item 5 : les ÉQUIPAGES → LABORERS, même région (rade). */
              if (paid > 0.f) econ_region_wealth_add(econ, n->home_port, CLASS_LABORER, paid); }
            float need=need_y*(dt_days/365.f);
            re->demand[RES_NAVAL_SUPPLIES]+=need;         /* la demande se VOIT au marché (flux/tick, transitoire assumé) */
            { float got = econ_country_stock_take(econ, c, RES_NAVAL_SUPPLIES, need);   /* POOL NATIONAL (2026-08-16) :
                                                       * la flotte mangeait LOCAL et pourrissait pendant que
                                                       * l'empire avait du matériel ailleurs */
              n->supplies_eaten+=got;
              if (got >= need-1e-4f) n->starve_days=0.f;
              else                   n->starve_days+=dt_days; }
        }
        if (n->starve_days>NAVY_STARVE_YEAR){             /* la flotte pourrit à quai */
            int big=-1, bc=0;
            for (int t=0;t<HULL_COUNT;t++){
                int avail=n->hull[t];
                if (t==HULL_TRANSPORT) avail-=n->at_sea;   /* audit 2026-08-12 : une coque EN MER
                                                            * (armée à bord) ne pourrit pas à quai */
                if (avail>bc){ bc=avail; big=t; }
            }
            if (big>=0){
                n->hull[big]--;
                int crew=navy_hull_crew((HullType)big);   /* les marins DÉBARQUENT et rentrent */
                n->crew-=crew; if (n->crew<0) n->crew=0;
                if (n->home_port>=0 && n->home_port<econ->n_regions)
                    econ_region_pop_add(econ, n->home_port, CLASS_LABORER, (float)crew);   /* RE-KEY : rentrent aux provinces */
            }
            n->starve_days-=365.f;
        }
    }
}

int navy_transport_packets_free(const NavyState *ns, int cid){
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return 0;
    int free_tr=ns->n[cid].hull[HULL_TRANSPORT]-ns->n[cid].at_sea;
    return (free_tr>0)?free_tr*NAVY_TRANSPORT_PKTS:0;
}

float navy_sea_days_regions(const World *w, int reg_a, int reg_b){
    int ax,ay,bx,by;
    if (!world_region_sea_anchor(w,reg_a,&ax,&ay)) return -1.f;
    if (!world_region_sea_anchor(w,reg_b,&bx,&by)) return -1.f;
    return world_sea_days(w,ax,ay,bx,by);
}

/* ── LA COLONISATION OUTRE-MER (mer §8) : on découvre ce que la volta touche ── */
int navy_colonize_tick(NavyState *ns, const World *w, WorldEconomy *econ, float dt_days, int skip_cid){
    (void)dt_days;
    int founded=0;
    for (int cid=0;cid<w->n_countries && cid<SCPS_MAX_COUNTRY;cid++){
        if (cid==skip_cid) continue;   /* le JOUEUR essaime outre-mer à la main (gate IA-off, skip_cid=-1 ⇒ no-op chronique) */
        const Country *ct=&w->country[cid];
        if (ct->role!=POLITY_PLAYER && ct->role!=POLITY_ANTAGONIST) continue;
        Navy *n=&ns->n[cid];
        if (n->colony_cd>0.f) continue;
        if (tune_f("NAVY_COMBAT_ON",0.f)>0.f
            && n->hull[HULL_TRANSPORT]-n->at_sea<1) continue;   /* flotte requise en mode COMBAT seul —
                                                                 * à OFF le matériel du pool est le convoi */
        int port=navy_best_port(w,econ,cid);
        if (port<0) continue;
        const RegionEconomy *src=&econ->region[port];
        float spop=0.f; for (int k=0;k<CLASS_COUNT;k++) spop+=src->strata[k].pop;
        /* MÊMES SEUILS QUE LA TERRESTRE — pour de vrai (2026-07-31) : ils étaient FIGÉS à
         * 500/0.35 alors que la vague colonisation a descendu la terrestre à
         * COLONY_MIN_POP (300) / COLONY_FOOD_GATE (0.25) ; le commentaire promettait un
         * alignement que le code ne tenait plus. On lit les MÊMES tunables : une seule
         * vérité pour essaimer, que ce soit à pied ou à la voile. */
        if (spop < tune_f("COLONY_MIN_POP",300.f)) continue;
        if (src->food_sat < tune_f("COLONY_FOOD_GATE",0.25f)) continue;
        int best=-1; float bscore=-1.f, bdays=0.f;
        for (int rd=0;rd<econ->n_regions;rd++){
            const RegionEconomy *dst=&econ->region[rd];
            if (!dst->active || dst->colonized || econ->adj[port][rd]) continue;
            if (!region_coastal(w,rd)) continue;             /* on atterrit par la côte */
            float days=navy_sea_days_regions(w,port,rd);
            if (days<0.f || days>NAVY_COLONY_MAX_DAYS) continue;
            float score=dst->cap_pop*0.001f/(1.f+days/15.f); /* les courants RAPPROCHENT */
            if (score>bscore){ bscore=score; best=rd; bdays=days; }
        }
        if (best>=0 && econ_colonize_overseas(econ,port,best,cid)){
            /* L5 — le convoi coûte ×2 en pop (econ_colonize_overseas) et CONSOMME
             * une traversée (télémétrie agrégée par le harnais avec celles des armées). */
            n->colony_cd=NAVY_COLONY_CD;
            ns->n_colony_sails++; ns->colony_sail_days+=bdays;
            founded++;
        }
    }
    return founded;
}

/* ════════════════════════════════════════════════════════════════════════
 * LA COURSE (brief coques) — le marchand recyclé en pirate chez les éthos
 * de razzia ; les eaux mortes deviennent des NIDS ; le raid pille — LOT P
 * (2026-07-07, règle joueur unifiée) : 20% du revenu ANNUEL de la victime
 * (diplo_pillage_value), RAZZIE 5% de sa pop si le pirate a le gate
 * esclavagiste (tech OU éthos conquérant) — balafre 1 an, immunise 5 ans ;
 * la saignée pèse sur les routes (marchands +5 %/coque plafond 50 %,
 * l'escorte chasse SANS plafond) ; la rancune s'arme en CB. Le combat naval
 * réutilise les OS des batailles-phases : choc (2 j) · accalmie (1 j) ·
 * déroute · poursuite (prises).
 * ════════════════════════════════════════════════════════════════════════ */
#include "scps_diplo.h"
#include "scps_routes.h"
#include "scps_culture.h"

#define COURSE_BALAFRE_J    365.f        /* la balafre : 1 an                 */
#define COURSE_IMMUNITE_J   (5.f*365.f)  /* l'immunité : 5 ans (par PROVINCE) */
#define COURSE_RAID_CD_J    240.f        /* cadence d'un commanditaire        */
#define COURSE_PORTEE_J     60.f         /* portée de chasse (jours de mer)   */
#define COURSE_PROT_PAR_M   0.05f        /* +5 % de protection par marchand   */
#define COURSE_PROT_CAP     0.50f        /* plafond 50 % — l'escorte fait le reste */
#define COURSE_GRIEF_RAID   1.0f         /* rancune par raid IDENTIFIÉ        */
#define COURSE_GRIEF_SAIGNEE 0.10f       /* rancune/mois de saignée identifiée */

static uint32_t crs_rng(uint32_t *r){ uint32_t x=*r; x^=x<<13; x^=x>>17; x^=x<<5; return *r=x; }
static float crs_f(uint32_t *r){ return (crs_rng(r)&0xFFFFFF)*(1.f/16777216.f); }

/* Le COMBAT NAVAL — mêmes os que la bataille terrestre, chair marine : des
 * CHOCS de 2 jours pondérés par les bordées et LE COURANT (qui a le courant
 * dans le dos gagne un multiplicateur), une ACCALMIE de 1 jour, jusqu'à la
 * rupture du moral d'équipage ; la DÉROUTE livre les coques à la POURSUITE
 * (coulées ou PRISES). Renvoie +1 si A l'emporte, -1 si B, 0 si nul. */
/* Curseur NAVY (joueur seul) : une flotte sous-payée n'a plus le MORAL (elle ne
 * se délabre plus — cf. plus haut). ×1.0 au neutre (défaut IA/chronique) ⇒ IDENTIQUE. */
static float navy_pay_morale(const WorldEconomy *e, int owner){
    if(!e || owner<0) return 1.f;
    float m=econ_country_budget_mult(e,owner,BUDGET_NAVY);
    return m>=1.f ? 1.f : clampf(m,0.35f,1.f);
}

static int navy_battle(const World *w, int cell_x, int cell_y,
                       int shipsA, int shipsB, float curA_dot,
                       uint32_t *rng, int *lossA, int *lossB, int *prises,
                       float payA, float payB){
    (void)w; (void)cell_x; (void)cell_y;
    float resA=(float)shipsA, resB=(float)shipsB;            /* moral d'équipage ≈ bordées */
    float res0A=resA+0.01f, res0B=resB+0.01f;
    float multA=1.f+0.35f*clampf(curA_dot,-1.f,1.f);         /* le courant dans le dos ARME */
    float multB=2.f-multA;
    multA*=payA; multB*=payB;                                /* solde impayée → moral amputé */
    *lossA=*lossB=*prises=0;
    for (int day=0; day<30 && resA>0.25f*res0A && resB>0.25f*res0B; day++){
        if (day%3==2) continue;                              /* accalmie (1 j sur 3) */
        float jA=resA*multA*(0.8f+0.4f*crs_f(rng));
        float jB=resB*multB*(0.8f+0.4f*crs_f(rng));
        if (jA>jB) resB-=0.5f+0.25f*(jA-jB); else resA-=0.5f+0.25f*(jB-jA);
    }
    int verdict = (resA<=0.25f*res0A && resB<=0.25f*res0B) ? 0 : (resB<=0.25f*res0B ? +1 : (resA<=0.25f*res0A ? -1 : 0));
    *lossA=(int)clampf((float)shipsA-resA+0.5f, 0.f, (float)shipsA);
    *lossB=(int)clampf((float)shipsB-resB+0.5f, 0.f, (float)shipsB);
    if (verdict!=0) *prises = (crs_f(rng)<0.5f)?1:0;         /* la poursuite COULE ou CAPTURE */
    return verdict;
}

/* L'éthos du trône (capitale) — la doctrine navale en découle. */
static Ethos navy_ethos(const World *w, const WorldEconomy *econ, int cid){
    int cp=(cid>=0&&cid<w->n_countries)?w->country[cid].capital_prov:-1;
    int cr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
    return (cr>=0&&cr<econ->n_regions)?econ->region[cr].culture.ethos:(Ethos)0;
}

long g_navy_raid_slaves=0;   /* LOT P (2026-07-07) : âmes razziées cumulées (course pirate) */

/* LOT P (2026-07-07) — pose balafre+CD sur la province représentative de `region`,
 * RÉUTILISÉ par la course IA (ci-dessous) et CMD_RAID_COAST (scps_sim.c). */
void navy_mark_raided(WorldEconomy *econ, int region){
    if (!econ || region<0 || region>=econ->n_regions) return;
    int bpid=econ_region_rep_province(econ, region);
    if (bpid>=0 && bpid<econ->n_prov){
        econ->prov[bpid].balafre_days=COURSE_BALAFRE_J;   /* « côte balafrée » */
        econ->prov[bpid].raid_cd_days=COURSE_IMMUNITE_J;
    }
}

void navy_course_tick(NavyState *ns, const World *w, WorldEconomy *econ,
                      DiploState *dp, RouteNetwork *rn, uint32_t *rng,
                      const TechState *ts, int player, float dt_days){
    /* 0. la pression des routes se REPOSE chaque passe (re-mesurée ci-dessous) */
    for (int i=0;i<rn->n;i++) rn->route[i].pirate_press=0.f;

    /* COMBAT NAVAL + PIRATERIE : OFF PAR DÉFAUT (décision joueur 2026-08-12 — « on
     * garde les routes et la colonisation »). NAVY_COMBAT_ON=1 réveille la course,
     * les bordées, blocus, interception, nids, raids. Les TRANSPORTS, la
     * construction, les fournitures et la colonisation vivent AILLEURS (navy_tick /
     * navy_colonize_tick) et restent intacts. */
    bool combat = tune_f("NAVY_COMBAT_ON",0.f)>0.f;
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        Navy *n=&ns->n[c];

        /* 1. VERDICT anti-piraterie : la paix l'exige — on désarme (réversible). */
        if (dp->pirate_disarm[c]){
            n->hull[HULL_MERCHANT]+=n->hull[HULL_PIRATE];
            n->hull[HULL_PIRATE]=0; n->nest_region=-1; n->mission=NAVY_RADE;
            dp->pirate_disarm[c]=0; n->disarmed++;
        }

        /* 2. DOCTRINE PAR ÉTHOS (IA seule — le joueur peut tout, et le paiera). */
        if (c!=player){
            Ethos e=navy_ethos(w,econ,c);
            int port=navy_best_port(w,econ,c);
            /* OFF = OFF (retour joueur 2026-08-16) : AUCUNE coque ne sort des chantiers —
             * le MATÉRIEL NAVAL (EMBARK_NAVAL_COST au pool) EST le convoi ; les
             * traversées et la colonisation ne dépendent plus d'une flotte. */
            if (combat && port>=0){
                if (e==ETHOS_HONNEUR && n->build_hull<0 && n->hull[HULL_WAR]<1
                    && n->hull[HULL_PIRATE]>0)
                    navy_order_build(ns,w,econ,c,HULL_WAR);     /* la course s'escorte */
                if (e==ETHOS_HONNEUR && n->hull[HULL_WAR]>=1){   /* l'Honneur CHASSE en guerre */
                    bool guerre=false;
                    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY && !guerre;b++)
                        if (b!=c && diplo_status(dp,c,b)==DIPLO_WAR) guerre=true;
                    if (guerre && n->mission==NAVY_RADE) n->mission=NAVY_INTERCEPTION;
                }
                if ((e==ETHOS_HONNEUR || e==ETHOS_DOMINATEUR)){
                    /* la razzia : convertir tôt (Honneur) / armer contre le rival (Dominateur) */
                    int veut=(e==ETHOS_HONNEUR)?3:2; bool rival=(e==ETHOS_HONNEUR);
                    if (!rival) for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY && !rival;b++)
                        if (b!=c && (diplo_status(dp,c,b)==DIPLO_WAR || dp->rancor[c][b]>0.5f)) rival=true;
                    if (rival && n->hull[HULL_PIRATE]<veut && n->hull[HULL_MERCHANT]>0)
                        navy_convert(ns,w,econ,c,true);
                } else if (n->hull[HULL_PIRATE]>0){
                    navy_convert(ns,w,econ,c,false);   /* les autres éthos ne s'encanaillent pas d'eux-mêmes */
                }
                /* le Mercantile harcelé MONTE : marchands au plafond, puis l'escorte sort */
                if (e==ETHOS_MERCANTILE || e==ETHOS_PACIFISTE){
                    float worst=0.f;
                    for (int i=0;i<rn->n;i++){ const TradeRoute *t=&rn->route[i];
                        if (!t->open||!t->maritime) continue;
                        if (econ->region[t->ra].owner!=c && econ->region[t->rb].owner!=c) continue;
                        if (t->pirate_press>worst) worst=t->pirate_press; }
                    if (n->build_hull<0 && worst>0.5f){
                        if (n->hull[HULL_MERCHANT]<10) navy_order_build(ns,w,econ,c,HULL_MERCHANT);
                        else if (e==ETHOS_MERCANTILE && n->hull[HULL_WAR]<3) navy_order_build(ns,w,econ,c,HULL_WAR);
                    }
                    if (n->hull[HULL_WAR]>0) n->mission=NAVY_ESCORTE;
                }
                /* l'Ordre patrouille ses eaux ; en guerre, le BLOCUS du port ennemi.
                 * Dominateur ET Bureaucrate ARMENT des bordées (sans elles, blocus
                 * et interception restaient lettre morte — 0/308 au balayage). */
                if (e==ETHOS_ORDRE || e==ETHOS_DOMINATEUR || e==ETHOS_BUREAUCRATE){
                    if (n->build_hull<0 && n->hull[HULL_WAR]<2)
                        navy_order_build(ns,w,econ,c,HULL_WAR);
                    int foe=-1, foe_any=-1;
                    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++){
                        if (b==c || diplo_status(dp,c,b)!=DIPLO_WAR) continue;
                        if (foe_any<0) foe_any=b;
                        if (foe<0 && navy_best_port(w,econ,b)>=0) foe=b;
                    }
                    if (foe>=0 && n->hull[HULL_WAR]>=2){ n->mission=NAVY_BLOCUS; n->mission_target=foe; }
                    else if (foe_any>=0 && n->hull[HULL_WAR]>=1){ n->mission=NAVY_INTERCEPTION; n->mission_target=foe_any; }
                    else if (n->mission==NAVY_BLOCUS||n->mission==NAVY_INTERCEPTION){ n->mission=NAVY_RADE; n->mission_target=-1; }
                }
                /* LE TRANSPORT EN DERNIER (dépouillement 2026-08-11 : « 82 coques, 21
                 * routes, et personne ne traverse pour s'installer ») : AUCUNE doctrine
                 * ne commandait jamais de coque TRANSPORT — l'outre-mer ET les traversées
                 * d'armées vivaient sur les seules prises de guerre. Toute IA portuaire
                 * entretient sa projection (1 coque ; 2 pour Mercantile/Dominateur) —
                 * mais SEULEMENT sur un chantier OISIF : les bordées de guerre passent
                 * d'abord (banc doctrine ORDRE). NAVY_TRANSPORT_MIN=0 = l'hier exact. */
                { int veut_tr=(int)tune_f("NAVY_TRANSPORT_MIN",1.f);
                  if (veut_tr>0 && (e==ETHOS_MERCANTILE||e==ETHOS_DOMINATEUR)) veut_tr++;
                  if (n->build_hull<0 && n->hull[HULL_TRANSPORT]<veut_tr)
                      navy_order_build(ns,w,econ,c,HULL_TRANSPORT); }
            }
        }
        if (!combat){                          /* OFF : ni mission de combat, ni nid, ni raid */
            if (n->mission!=NAVY_RADE){ n->mission=NAVY_RADE; n->mission_target=-1; }
            if (n->hull[HULL_PIRATE]>0) navy_convert(ns,w,econ,c,false);   /* le pirate se range */
            continue;
        }
        if (n->mission==NAVY_BLOCUS){
            int t=n->mission_target;
            if (t<0||t>=SCPS_MAX_COUNTRY||diplo_status(dp,c,t)!=DIPLO_WAR||n->hull[HULL_WAR]<1){
                n->mission=NAVY_RADE; n->mission_target=-1;   /* le blocus ne survit pas à la paix */
            }
        }

        /* 3. LE NID : les pirates mouillent dans les EAUX MORTES proches de la rade. */
        if (n->hull[HULL_PIRATE]>0 && n->nest_region<0){
            int port=navy_best_port(w,econ,c);
            int ax,ay;
            if (port>=0 && world_region_sea_anchor(w,port,&ax,&ay)){
                int best=-1; int32_t bd=0x7FFFFFFF;
                for (int y=0;y<SCPS_H;y+=4) for (int x=0;x<SCPS_W;x+=4){
                    const Cell *cc=&w->cell[scps_idx(x,y)];
                    if (cc->sea!=SEA_MORTE) continue;
                    int32_t d2=(x-ax)*(x-ax)+(y-ay)*(y-ay);
                    if (d2<bd){ bd=d2; best=scps_idx(x,y); }
                }
                n->nest_region=best;                          /* une CELLULE de mer morte */
            }
        }
        if (n->hull[HULL_PIRATE]<=0) n->nest_region=-1;

        /* 4. LE RAID CÔTIER : 1/10 des stocks · balafre 1 an · immunité 5 ans. */
        if (n->raid_cd>0.f) n->raid_cd-=dt_days;
        if (n->hull[HULL_PIRATE]>0 && n->raid_cd<=0.f){
            int port=navy_best_port(w,econ,c);
            int best=-1; float bv=-1.f;
            for (int r=0;r<econ->n_regions;r++){
                RegionEconomy *re=&econ->region[r];
                if (re->owner==c || re->owner<0 || !re->coastal || !re->culture.settled) continue;
                if (re->raid_cd_days>0.f) continue;                    /* la vache est traite */
                if (diplo_status(dp,c,re->owner)==DIPLO_ALLIED) continue;
                if (port>=0){ float dj=navy_sea_days_regions(w,port,r);
                              if (dj<0.f || dj>COURSE_PORTEE_J) continue; }
                /* STOCK NATIONAL (2026-09-03) : l'entrepôt est celui du PAYS — le pirate
                 * jauge la richesse de l'empire visé, la côte n'apporte que son prix
                 * de marché (et le bonus de port ci-dessous départage les rades). */
                float v=0.f; for (int g=1;g<RES_COUNT;g++)
                    v+=econ_country_stock_sum(econ,re->owner,(Resource)g)*re->price[g];
                if (re->build.port>0.f) v*=1.3f;                       /* le port : la cible juteuse */
                if (v>bv){ bv=v; best=r; }
            }
            if (best>=0){
                RegionEconomy *re=&econ->region[best];
                int victim=re->owner;
                int defense=(victim>=0&&victim<SCPS_MAX_COUNTRY)?ns->n[victim].hull[HULL_WAR]:0;
                bool success=true, identified=false;
                if (defense>0){
                    /* l'escorte/patrouille FORCE la bataille — le courant local arme */
                    int ax2,ay2; float dot=0.f;
                    if (world_region_sea_anchor(w,best,&ax2,&ay2)){
                        const Cell *cc=&w->cell[scps_idx(ax2,ay2)];
                        dot=(cc->cur_vx*0.7f+cc->cur_vy*0.3f)/100.f;   /* proxy : l'attaquant vient du large */
                    }
                    int lA,lB,pr;
                    float effA=(float)n->hull[HULL_PIRATE]*(n->starve_days>0.f?0.7f:1.f);
                    float effB=(float)defense*(ns->n[victim].starve_days>0.f?0.7f:1.f);
                    int v2=navy_battle(w,0,0,(int)(effA+0.5f),(int)(effB+0.5f),dot,rng,&lA,&lB,&pr,
                                       navy_pay_morale(econ,c),navy_pay_morale(econ,victim));
                    if (lA>n->hull[HULL_PIRATE]) lA=n->hull[HULL_PIRATE];
                    if (lB>ns->n[victim].hull[HULL_WAR]) lB=ns->n[victim].hull[HULL_WAR];
                    n->hull[HULL_PIRATE]-=lA;
                    ns->n[victim].hull[HULL_WAR]-=lB;
                    n->crew-=lA*NAVY_CREW_LIGHT; if (n->crew<0) n->crew=0;             /* sombrés avec la coque */
                    ns->n[victim].crew-=lB*NAVY_CREW_WAR; if (ns->n[victim].crew<0) ns->n[victim].crew=0;
                    n->navals++; ns->n[victim].navals++;
                    if (v2>0){ if (pr && ns->n[victim].hull[HULL_MERCHANT]>0){
                        ns->n[victim].hull[HULL_MERCHANT]--; n->hull[HULL_PIRATE]++; n->prises++;
                        ns->n[victim].crew-=NAVY_CREW_LIGHT; if (ns->n[victim].crew<0) ns->n[victim].crew=0;
                        n->crew+=NAVY_CREW_LIGHT;            /* l'équipage pris est ENRÔLÉ de force */ } }
                    else { success=false; identified=(crs_f(rng)<0.85f); }   /* capturé, il DÉSIGNE */
                } else identified=(crs_f(rng)<0.35f);
                if (success && n->hull[HULL_PIRATE]>0){
                    /* LOT P (2026-07-07) — PILLAGE UNIFIÉ (règle joueur : « Piraterie,
                     * raids, tout type d'occupation = pillage ») : la dîme COURSE_RAID_TITHE
                     * (1/10 des stocks, une fraction PLATE) est REMPLACÉE par
                     * diplo_pillage_value — 20% du revenu ANNUEL de la victime, transféré
                     * RÉELLEMENT (province représentative, jamais de fantôme), BORNÉ par
                     * ce qui existe. Le MOVER NU ne pose aucun cooldown : la course garde
                     * SON PROPRE marqueur (raid_cd_days/balafre, posé juste après) — pas de
                     * 3e système (cf. diplo_pillage_fresh au sac de siège/occupation).
                     * dst_region = le meilleur port du pirate (le butin y est crédité). */
                    int hp=navy_best_port(w,econ,c);
                    float loot = diplo_pillage_value(econ, best, hp, victim);
                    /* RAZZIA (esclavage §4c → 2026-07-21, pratique universelle) — la
                     * fraction résolue par econ_country_slave_fraction (0 = abolition ·
                     * 5 % coutume · 15 % tech), diplo l'exécute telle quelle. */
                    if (ts){ float nf = econ_country_slave_fraction(w, econ, &ts[c], c);
                        if (nf > 0.f && diplo_enslave_capture(w, econ, c, best, nf) > 0) g_navy_raid_slaves++; }
                    navy_mark_raided(econ, best);   /* balafre+CD (partagé avec CMD_RAID_COAST) */
                    n->raids_done++; n->loot_gold+=loot;
                    if (identified && victim>=0) diplo_pirate_grief(dp,victim,c,COURSE_GRIEF_RAID);
                } else if (identified && victim>=0)
                    diplo_pirate_grief(dp,victim,c,COURSE_GRIEF_RAID*0.6f);
                n->raid_cd=COURSE_RAID_CD_J*(0.8f+0.4f*crs_f(rng));
            } else n->raid_cd=120.f;                                    /* rien à portée : on patiente */
        }

        /* 5. LA SAIGNÉE : les pirates pèsent sur la route la plus riche à portée. */
        if (n->hull[HULL_PIRATE]>0){
            int port=navy_best_port(w,econ,c), tgt=-1; float bv=-1.f;
            for (int i=0;i<rn->n;i++){
                TradeRoute *t=&rn->route[i];
                if (!t->open || !t->maritime) continue;
                int oa=econ->region[t->ra].owner, ob=econ->region[t->rb].owner;
                if (oa==c||ob==c) continue;
                if (port>=0){ float dj=navy_sea_days_regions(w,port,t->ra);
                              if (dj<0.f||dj>COURSE_PORTEE_J) continue; }
                if (t->yield>bv){ bv=t->yield; tgt=i; }
            }
            if (tgt>=0){
                TradeRoute *t=&rn->route[tgt];
                float press=(float)n->hull[HULL_PIRATE];
                int oa=econ->region[t->ra].owner, ob=econ->region[t->rb].owner;
                int merch=0, escort=0;
                if (oa>=0&&oa<SCPS_MAX_COUNTRY){ merch+=ns->n[oa].hull[HULL_MERCHANT];
                    if (ns->n[oa].mission==NAVY_ESCORTE) escort+=ns->n[oa].hull[HULL_WAR]; }
                if (ob>=0&&ob<SCPS_MAX_COUNTRY){ merch+=ns->n[ob].hull[HULL_MERCHANT];
                    if (ns->n[ob].mission==NAVY_ESCORTE) escort+=ns->n[ob].hull[HULL_WAR]; }
                float prot=fminf(COURSE_PROT_CAP, COURSE_PROT_PAR_M*(float)merch);
                float net=press*(1.f-prot)-(float)escort;               /* l'escorte CHASSE, sans plafond */
                if (net>0.f){
                    t->pirate_press+=net;
                    float idp=0.15f*(dt_days/30.f);
                    if (oa>=0 && crs_f(rng)<idp) diplo_pirate_grief(dp,oa,c,COURSE_GRIEF_SAIGNEE);
                    if (ob>=0 && crs_f(rng)<idp) diplo_pirate_grief(dp,ob,c,COURSE_GRIEF_SAIGNEE);
                }
            }
        }

        /* 6. LE BLOCUS : mouillé devant le port ennemi, il COUPE ses liens.
         * B6 — le BLOCUS FANTÔME : on ne ferme les liens et ne COMPTE les jours
         * QUE si la guerre est ACTIVE entre c et la cible (DIPLO_WAR). Sans cette
         * garde au point d'accumulation, un blocus survivait à la paix (la mission
         * n'était relue qu'au prochain tick) et gonflait blocus_days hors guerre —
         * 27-34k jours fantômes sans une seule bataille au balayage. */
        if (n->mission==NAVY_BLOCUS && n->mission_target>=0){
            int t=n->mission_target;
            if (t<SCPS_MAX_COUNTRY && diplo_status(dp,c,t)==DIPLO_WAR){
                for (int i=0;i<rn->n;i++){
                    TradeRoute *rt=&rn->route[i];
                    if (!rt->open||!rt->maritime) continue;
                    if (econ->region[rt->ra].owner==t || econ->region[rt->rb].owner==t)
                        rt->pirate_press=99.f;                          /* le lien est tenu fermé */
                }
                n->blocus_days+=dt_days;
            } else {
                n->mission=NAVY_RADE; n->mission_target=-1;             /* la paix lève le blocus, net */
            }
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * L'INTERCEPTION (coques §3 — le job FINI) : les transports pleins sont des
 * cibles stratégiques. Un convoi hostile qui embarque ou navigue croise les
 * patrouilles d'INTERCEPTION : l'escorte du convoi (les bordées de son
 * pays), c'est tout ce qui le sépare du fond — un transport sans escorte
 * MEURT SEUL. La bataille réutilise les phases ; le convoi coulé NOIE ses
 * paquets (le warhost embarqué sombre avec les coques).
 * ════════════════════════════════════════════════════════════════════════ */
#include "scps_campaign.h"

void navy_interception_tick(NavyState *ns, struct Campaign *camp, const World *w,
                            WorldEconomy *econ, struct DiploState *dp, uint32_t *rng){
    for (int i=0;i<CAMPAIGN_ARMY_CAP;i++){
        FieldArmy *a=&camp->army[i];
        /* L'embarquement doit être exposé lui aussi : une traversée courte peut
         * passer EMBARK→SAIL→LAND à l'intérieur d'un unique tick mensuel et ne
         * serait sinon jamais observable par la marine. */
        if (!a->active || (a->phase!=FA_EMBARK && a->phase!=FA_SAIL) || a->intercept_done) continue;
        int owner=a->owner;
        if (owner<0||owner>=SCPS_MAX_COUNTRY) continue;
        for (int e=0;e<w->n_countries && e<SCPS_MAX_COUNTRY;e++){
            if (e==owner) continue;
            Navy *pat=&ns->n[e];
            /* Une bordée en BLOCUS est déjà devant les ports de SA cible : elle
             * doit chasser les convois de ce pays au même titre qu'une patrouille
             * d'interception. Avant ce raccord, la doctrine Dominateur/Bureaucrate
             * choisissait presque toujours BLOCUS quand l'ennemi avait un port,
             * puis devenait paradoxalement aveugle à ses transports. */
            bool hunting = pat->mission==NAVY_INTERCEPTION
                        || (pat->mission==NAVY_BLOCUS && pat->mission_target==owner);
            if (!hunting || pat->hull[HULL_WAR]<1) continue;
            if (diplo_status(dp,e,owner)!=DIPLO_WAR) continue;
            if (crs_f(rng)>0.45f) continue;                   /* la mer est grande : on ne trouve pas toujours */
            a->intercept_done=true;                            /* une chasse par traversée */
            Navy *esc=&ns->n[owner];
            int escort=esc->hull[HULL_WAR];
            int lA,lB,pr;
            float effA=(float)pat->hull[HULL_WAR]*(pat->starve_days>0.f?0.7f:1.f);
            float effB=(float)escort*(esc->starve_days>0.f?0.7f:1.f);
            int v=(escort>0)
                ? navy_battle(w,0,0,(int)(effA+0.5f),(int)(effB+0.5f),0.f,rng,&lA,&lB,&pr,
                              navy_pay_morale(econ,e),navy_pay_morale(econ,owner))
                : (+1);                                        /* sans escorte : PROIE */
            if (escort>0){
                if (lA>pat->hull[HULL_WAR]) lA=pat->hull[HULL_WAR];
                if (lB>esc->hull[HULL_WAR]) lB=esc->hull[HULL_WAR];
                pat->hull[HULL_WAR]-=lA; pat->crew-=lA*NAVY_CREW_WAR; if (pat->crew<0) pat->crew=0;
                esc->hull[HULL_WAR]-=lB; esc->crew-=lB*NAVY_CREW_WAR; if (esc->crew<0) esc->crew=0;
                pat->navals++; esc->navals++;
            }
            if (v>0){                                          /* le convoi COULE */
                long pk=0;
                /* `i` est l'identité du CORPS intercepté. Passer `owner` relisait
                 * l'ancien corps principal du pays et faussait les noyés dès qu'un
                 * détachement secondaire prenait la mer. */
                { ArmyComposition ac=campaign_corps_composition(camp,i); pk=ac.total; }
                int tr=a->sail_transports;
                esc->hull[HULL_TRANSPORT]-=tr; if (esc->hull[HULL_TRANSPORT]<0) esc->hull[HULL_TRANSPORT]=0;
                esc->at_sea-=tr; if (esc->at_sea<0) esc->at_sea=0;
                esc->crew-=tr*NAVY_CREW_LIGHT; if (esc->crew<0) esc->crew=0;
                a->sail_transports=0;
                a->active=false; a->phase=FA_IDLE; a->dest=-1; a->next=-1;
                pat->intercepts++; pat->drowned+=pk;           /* les paquets SOMBRENT */
                if (pr && esc->hull[HULL_TRANSPORT]>0){        /* la poursuite fait une PRISE */
                    esc->hull[HULL_TRANSPORT]--; pat->hull[HULL_TRANSPORT]++;
                    pat->crew+=NAVY_CREW_LIGHT; pat->prises++;
                }
            }
            break;                                             /* une bataille par convoi et par mois */
        }
    }
}
