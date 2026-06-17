/*
 * scps_campaign.c — les armées sur la carte : marche, siège, bataille (voir .h)
 *
 * Une force expéditionnaire par pays, posée sur une RÉGION, qui marche de région
 * en région (au pas du convoi, le terrain décidant des jours — §1), assiège en
 * arrivant (le siège : 14 j si nu, ≤ 2 ans sinon) et livre bataille (§2/§3) quand
 * deux forces hostiles se croisent. Lecture seule sur econ : non-invasif vis-à-vis
 * de la conquête abstraite.
 */
#include "scps_campaign.h"
#include "scps_tune.h"   /* Arc J : constantes de calibrage surchargeables */
#include "scps_navy.h"   /* réservation de transports (accès aux champs, pas d'appel) */
#include "scps_labor.h"   /* capitale_defense / capitale_max_tier : la défense passive de la capitale */
#include <math.h>
#include <string.h>

/* ---- Calibrage : ce que l'éco régionale dit au siège ------------------ */
#define DEF_BASE          1.0f   /* défense de base d'une région colonisée */
#define DEF_PER_BLD       0.25f  /* chaque édifice durcit la place */
#define FOOD_MONTHS_FULL  12.f   /* food_sat 1.0 → un an de vivres en magasin */
#define RIVER_BATTLE_MIN  40     /* débit (0..255) au-delà duquel la région a une ligne d'eau */
#define RIVER_COMBAT_EDGE 1.25f  /* franchir sous le feu : le défenseur ×1.25 (annulé par un pont) */

/* ---- Outils ----------------------------------------------------------- */
static long force_units(const ArmyState *a){
    long t=0; for (int i=0;i<a->n_units;i++) if (a->units[i].count>0) t+=a->units[i].count;
    return t;
}
static bool region_ok(const Campaign *c, const WorldEconomy *e, int r){
    if (r<0 || r>=e->n_regions) return false;
    if (e->region[r].impassable) return false;          /* zone morte */
    if (terrain_impassable(c->reg_biome[r])) return false; /* roche/glace/eau */
    return true;
}

/* Prochaine région depuis `from` vers `dest` (BFS sur l'adjacence praticable).
 * Renvoie le voisin de `from` au plus court chemin, ou -1 si injoignable. */
static int next_hop(const Campaign *c, const WorldEconomy *e, int from, int dest){
    if (from==dest) return dest;
    int nr=e->n_regions;
    static int dist[SCPS_MAX_REG];           /* sim mono-thread : static, comme scps_trade */
    static int queue[SCPS_MAX_REG];
    for (int i=0;i<nr;i++) dist[i]=-1;
    int qh=0,qt=0;
    if (!region_ok(c,e,dest)) return -1;
    dist[dest]=0; queue[qt++]=dest;
    while (qh<qt){
        int u=queue[qh++];
        for (int v=0;v<nr;v++){
            if (!e->adj[u][v] || dist[v]>=0) continue;
            if (!region_ok(c,e,v)) continue;
            dist[v]=dist[u]+1; queue[qt++]=v;
        }
    }
    if (from<0 || from>=nr || dist[from]<0) return -1;   /* from injoignable */
    int best=-1, bd=1<<30;
    for (int v=0;v<nr;v++){
        if (!e->adj[from][v] || dist[v]<0) continue;
        if (dist[v]<bd){ bd=dist[v]; best=v; }            /* le voisin qui rapproche le plus */
    }
    return best;
}

static float region_defense(const WorldEconomy *e, int r){
    const RegionEconomy *R=&e->region[r];
    if (!R->colonized) return 0.f;                        /* vacante : on entre (14 j) */
    /* la CAPITALE ajoute une défense PASSIVE : un niveau par tier (que la pop débloque)
     * — elle allonge le siège comme un rempart (mais sans bonus défenseur au combat). */
    long pop = (long)(R->strata[CLASS_LABORER].pop + R->strata[CLASS_BOURGEOIS].pop + R->strata[CLASS_ELITE].pop);
    float cap_def = (float)capitale_defense(capitale_max_tier(pop));
    return DEF_BASE + DEF_PER_BLD * (float)R->n_bld + cap_def;
}
static float region_food_months(const WorldEconomy *e, int r){
    float f=e->region[r].food_sat;
    if (f<0.f) f=0.f;
    if (f>1.f) f=1.f;
    return f * FOOD_MONTHS_FULL;
}

/* ---- Init ------------------------------------------------------------- */
/* §5 posture (déclarations — définies après campaign_order) */
static float posture_march_mult(int p);
static float posture_siege_mult(int p);

void campaign_init(Campaign *c, const World *w, const WorldEconomy *econ){
    memset(c,0,sizeof(*c));
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){ c->army[i].posture=FA_STANDARD;  /* défaut : standard */
                                          c->army[i].taken_region=-1; }    /* memset→0 = région 0 valide : remettre -1 */
    c->n_regions = econ->n_regions;
    for (int r=0; r<econ->n_regions && r<SCPS_MAX_REG; r++){
        const Region *R=&w->region[r];
        Biome b=BIO_PLAINS; float hsum=0.f; int n=0;
        for (int k=0; k<R->n_provinces && k<12; k++){
            int pid=R->province_ids[k];
            if (pid<0 || pid>=w->n_provinces) continue;
            if (n==0) b=w->province[pid].biome_dominant;  /* biome représentatif = 1re province */
            hsum += w->province[pid].height_avg; n++;
        }
        c->reg_biome[r]  = b;
        c->reg_height[r] = (n>0) ? hsum/(float)n : 0.2f;
    }
    /* lignes d'eau : une région qu'un cours d'eau notable traverse (franchissement
     * coûteux au choc — sauf pont, cf. field_battle). */
    for (int i=0;i<SCPS_N;i++){
        const Cell *cell=&w->cell[i];
        if (cell->region<0 || cell->region>=SCPS_MAX_REG) continue;
        if (cell->river >= RIVER_BATTLE_MIN) c->reg_river[cell->region]=true;
    }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        c->army[i].active=false; c->army[i].owner=i;
        c->army[i].loc=-1; c->army[i].dest=-1; c->army[i].next=-1;
        c->army[i].phase=FA_IDLE;
    }
}

/* ---- Ordre de campagne ------------------------------------------------ */
bool campaign_order(Campaign *c, const WorldEconomy *econ, int owner,
                    int from_region, int target_region, const ArmyState *src_force){
    if (owner<0 || owner>=SCPS_MAX_COUNTRY || !src_force) return false;
    if (from_region<0 || from_region>=econ->n_regions) return false;
    if (target_region<0 || target_region>=econ->n_regions) return false;
    if (force_units(src_force)<=0) return false;
    int hop = next_hop(c, econ, from_region, target_region);
    if (hop<0) return false;                              /* injoignable par terre */

    FieldArmy *a=&c->army[owner];
    a->active=true; a->owner=owner; a->loc=from_region; a->dest=target_region;
    a->force=*src_force;                                  /* copie du détachement */
    a->taken=0; a->legs=0; a->battles=0; a->taken_region=-1;
    if (from_region==target_region){                     /* déjà sur place */
        a->phase=FA_IDLE; a->next=-1; a->days_left=0.f; a->leg_days=0.f; return true;
    }
    a->next=hop; a->phase=FA_MARCH;
    a->leg_days  = army_step_days(&a->force, c->reg_biome[hop], c->reg_height[hop], false, false)
                 * posture_march_mult(a->posture);
    a->days_left = a->leg_days;
    return true;
}

/* L1 — la REDIRECTION : nouvelle cible, MÊME force (les pertes restent payées). */
bool campaign_redirect(Campaign *c, const WorldEconomy *econ, const DiploState *dp,
                       int owner, int target_region){
    if (owner<0 || owner>=SCPS_MAX_COUNTRY) return false;
    FieldArmy *a=&c->army[owner];
    if (!a->active || a->phase==FA_BATTLE || a->phase>=FA_EMBARK) return false;  /* épinglée / en mer */
    if (a->broken_days>0) return false;                                          /* brisée : elle fuit */
    if (target_region<0 || target_region>=econ->n_regions) return false;
    if (force_units(&a->force)<=0) return false;
    if (a->loc==target_region){
        /* sur place : on re-décide comme une ARRIVÉE (mêmes règles que la marche). */
        bool ours=(econ->region[a->loc].owner==a->owner);
        int  occ = dp ? dp->occupier[a->loc] : -1;
        a->dest=target_region;
        if (occ==a->owner || (ours && occ<0)){ a->phase=FA_IDLE; a->dest=-1; a->next=-1; return true; }
        a->phase=FA_SIEGE; a->next=-1;
        a->days_left = siege_days(region_defense(econ,a->loc), region_food_months(econ,a->loc),
                                  terrain_defense_mult(c->reg_biome[a->loc], c->reg_height[a->loc]))
                     * posture_siege_mult(a->posture);
        return true;
    }
    int hop=next_hop(c, econ, a->loc, target_region);
    if (hop<0) return false;                                                     /* injoignable par terre */
    a->dest=target_region; a->next=hop; a->phase=FA_MARCH;
    a->leg_days  = army_step_days(&a->force, c->reg_biome[hop], c->reg_height[hop], false, false)
                 * posture_march_mult(a->posture);
    a->days_left = a->leg_days;
    return true;
}

/* ── L'EMBARQUEMENT (mer §6) : port → mer → côte, tout en jours ─────────── */
bool campaign_order_sea(Campaign *c, const World *w, const WorldEconomy *econ,
                        struct NavyState *navy, int owner,
                        int from_region, int target_region, const ArmyState *src_force){
    if (owner<0 || owner>=SCPS_MAX_COUNTRY || !src_force || !navy || !w) return false;
    if (from_region<0 || from_region>=econ->n_regions) return false;
    if (target_region<0 || target_region>=econ->n_regions || from_region==target_region) return false;
    long packets=force_units(src_force);
    if (packets<=0) return false;
    const RegionEconomy *pr=&econ->region[from_region];
    if (pr->owner!=owner || pr->build.port<=0.f || !pr->coastal) return false;  /* on n'embarque qu'à SON port */
    if (!econ->region[target_region].coastal) return false;                     /* on atterrit par la côte */
    int ax,ay,bx,by;
    if (!world_region_sea_anchor(w,from_region,&ax,&ay))  return false;
    if (!world_region_sea_anchor(w,target_region,&bx,&by)) return false;
    float days=world_sea_days(w,ax,ay,bx,by);
    if (days<0.f) return false;                                                 /* bassins séparés */
    int need_tr=(int)((packets+9)/10); if (need_tr<1) need_tr=1;                /* 1 transport = 10 paquets */
    if (navy->n[owner].hull[HULL_TRANSPORT]-navy->n[owner].at_sea < need_tr) return false;
    for (int e=0;e<SCPS_MAX_COUNTRY;e++)                                        /* coques §3 : le BLOCUS tient le port */
        if (navy->n[e].mission==NAVY_BLOCUS && navy->n[e].mission_target==owner
            && navy->n[e].hull[HULL_WAR]>0) return false;
    FieldArmy *a=&c->army[owner];
    a->active=true; a->owner=owner; a->loc=from_region; a->dest=target_region; a->next=-1;
    a->force=*src_force;
    a->taken=0; a->legs=0; a->battles=0; a->taken_region=-1;
    a->phase=FA_EMBARK;
    a->leg_days = 4.f + (float)packets/15.f;            /* charger 1 000 hommes prend des jours */
    a->days_left= a->leg_days;
    a->sail_days=days; a->sail_transports=need_tr;
    a->intercept_done=false;
    a->land_at_port = (econ->region[target_region].build.port>0.f);
    navy->n[owner].at_sea += need_tr;                   /* la flotte est ENGAGÉE jusqu'au débarquement */
    c->n_sails++; c->sail_days_sum += days;
    return true;
}

/* Rend à la flotte les transports des armées revenues à terre (ou mortes). */
void campaign_release_transports(Campaign *c, struct NavyState *navy){
    if (!navy) return;
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        FieldArmy *a=&c->army[i];
        if (a->sail_transports<=0) continue;
        bool at_sea = a->active && (a->phase==FA_EMBARK || a->phase==FA_SAIL || a->phase==FA_LAND);
        if (at_sea) continue;
        int o=a->owner;
        if (o>=0 && o<SCPS_MAX_COUNTRY){
            navy->n[o].at_sea -= a->sail_transports;
            if (navy->n[o].at_sea<0) navy->n[o].at_sea=0;
        }
        a->sail_transports=0;
    }
}

/* ---- POSTURE (§5 sidebar) : prudente marche/assiège LENTEMENT (préserve), -----
 * l'agressive PRESSE (marche vive, siège mené tambour battant). Un palier, un mot. */
static float posture_march_mult(int p){ return (p==FA_PRUDENTE)?1.15f : (p==FA_AGRESSIVE)?0.88f : 1.f; }
static float posture_siege_mult(int p){ return (p==FA_PRUDENTE)?1.15f : (p==FA_AGRESSIVE)?0.85f : 1.f; }
void campaign_set_posture(Campaign *c, int owner, int posture){
    if (!c || owner<0 || owner>=SCPS_MAX_COUNTRY) return;
    if (posture<FA_PRUDENTE) posture=FA_PRUDENTE;
    if (posture>FA_AGRESSIVE) posture=FA_AGRESSIVE;
    c->army[owner].posture=posture;
}
int campaign_posture(const Campaign *c, int owner){
    return (c && owner>=0 && owner<SCPS_MAX_COUNTRY) ? c->army[owner].posture : FA_STANDARD;
}
const char *campaign_posture_name(int p){
    static const char *N[3]={ "prudente","standard","agressive" };
    return (p>=0&&p<3)?N[p]:"?";
}

/* ---- La bataille de rencontre (§2/§3 + terrain défensif) -------------- *
 * Le défenseur d'un FORT paie au choc selon le terrain (pente/couvert) et profite
 * d'une rivière non pontée. EST défenseur : celui qui RELÈVE le siège (l'autre
 * assiège), sinon le propriétaire de la région (garnison). Forts uniquement — une
 * province sans défense ne donne aucun bonus.                                  */
/* ═══ LA BATAILLE DANS LE TEMPS (brief bataille) — choc · accalmie · déroute · poursuite ═══
 * resolve_battle (l'instantané) est REMPLACÉ par un état qui dure : le moral est la
 * vraie réserve, la poursuite fait l'essentiel des morts, le bras-de-fer est nourri. */
#define BT_CHOC_J      3      /* jours de choc par cycle */
#define BT_ACCALMIE_J  2      /* jours d'accalmie */
#define BT_RUPTURE     0.20f  /* H4 : la réserve rompt sous 20 % (était 25) — le choc dure 1-2 cycles de plus */
#define BT_RECUP       0.015f /* l'accalmie rend 1.5 %/j de l'ouverture (+0.7 % chez soi) */
#define BT_DMG_K       0.057f /* P3-bis : calibré → ratio poursuite/choc 1.3x ∈ [0.8,1.8] (graine 7, 1×60 ; SANS cavalerie) */
#define BT_CHOC_MORTS  0.006f  /* L3/L4 (recalibré L4-2026-06, graine 7 1×30 nouvelle genèse) : ratio poursuite/choc 2.4x ∈ [2,5] — la curée domine, le choc tue encore */
#define BT_MAX_JOURS   120    /* au-delà : les deux camps se délitent (nul sanglant) */
#define BT_BRISEE_J    45     /* une armée en déroute est INAPTE ce temps */

static float xs01(uint32_t *st){            /* xorshift32 → [0..1) */
    uint32_t x=*st?*st:0xA341B2C7u; x^=x<<13; x^=x>>17; x^=x<<5; *st=x;
    return (float)(x>>8)/16777216.f;
}

static float side_reserve(const ArmyState *f){
    float r=0.f;
    for (int u=0;u<f->n_units;u++){
        const UnitDef *d=unit_def(f->units[u].type);
        if (d) r += (float)f->units[u].count * d->moral;
    }
    return r * (f->doctrine.moral_mul>0.f?f->doctrine.moral_mul:1.f);
}
static float side_power(const ArmyState *f){
    float p=0.f;
    for (int u=0;u<f->n_units;u++){
        const UnitDef *d=unit_def(f->units[u].type);
        if (d) p += (float)f->units[u].count * (1.f + d->discipline);
    }
    p *= (f->doctrine.weapon_power>0.f?f->doctrine.weapon_power:1.f);
    return powf(p, 0.85f);            /* la MASSE aide, à rendements décroissants */
}
/* P-bis — LE CONTRE PREND SES DENTS : matchup moyen pondéré par effectifs des unités de
 * `self` contre celles de `foe` (∈ [0.5,2]). >1 = la composition de self CONTRAIT celle de
 * foe. C'est ce qui rend la matrice pierre-feuille-ciseaux EFFECTIVE en bataille de campagne
 * (avant : seul resolve_battle/le duel la lisait). Mélanges miroir → ~1.0 (neutre) ; un côté
 * mal apparié (cavalerie contre un mur de hampes drillé) → loin de 1.0. */
static float side_counter(const ArmyState *self, const ArmyState *foe){
    long ts=force_units(self), tf=force_units(foe);
    if (ts<=0 || tf<=0) return 1.f;
    float acc=0.f;
    for (int i=0;i<self->n_units;i++){
        long ni=self->units[i].count; if (ni<=0) continue;
        for (int j=0;j<foe->n_units;j++){
            long nj=foe->units[j].count; if (nj<=0) continue;
            acc += (float)ni * (float)nj * matchup(self->units[i].type, foe->units[j].type);
        }
    }
    return acc / ((float)ts * (float)tf);
}
static long kill_packets(ArmyState *f, long packets){
    long tot=force_units(f); if (tot<=0||packets<=0) return 0;
    if (packets>tot) packets=tot;
    long left=packets;
    for (int u=0;u<f->n_units && left>0;u++){
        long share=(f->units[u].count*packets)/tot;
        if (share>f->units[u].count) share=f->units[u].count;
        f->units[u].count-=share; left-=share;
    }
    for (int u=0;u<f->n_units && left>0;u++){
        long take=(f->units[u].count<left)?f->units[u].count:left;
        f->units[u].count-=take; left-=take;
    }
    return packets-left;
}
static float bt_terrainA(const Campaign *c, const WorldEconomy *e, int loc, int ownA, int ownB){
    float terrainA=1.f;
    if (region_defense(e,loc)>0.f){
        int defender=0;
        if      (e->region[loc].owner==ownA) defender=-1;
        else if (e->region[loc].owner==ownB) defender=+1;
        if (defender!=0){
            float adv=terrain_combat_bonus(c->reg_biome[loc]);
            bool bridged=e->region[loc].route_pe>0.f;
            if (c->reg_river[loc] && !bridged) adv*=RIVER_COMBAT_EDGE;
            /* P3 — le bonus défensif reste LÉGER (~10 % max) : le relief (qui charge
             * les assiégeants) doit pouvoir l'emporter, sinon le défenseur gagne TOUT
             * et la guerre ne prend jamais de terrain (217 batailles, 0 occupation). */
            adv=fminf(adv, 1.f + tune_f("BT_DEF_EDGE",0.10f));
            terrainA=(defender<0)?adv:(1.f/adv);
        }
    }
    return terrainA;
}
/* le SCORE de guerre encaisse (orientation : l'attaquant porte le cb). */
static void bt_score(DiploState *dp, int win, int lose, float pts){
    if (!dp) return;
    if (diplo_war_goal(dp,win,lose)!=CB_NONE)
        dp->battle_score[win][lose] = fminf(50.f, dp->battle_score[win][lose]+pts);
    else if (diplo_war_goal(dp,lose,win)!=CB_NONE)
        dp->battle_score[lose][win] = fmaxf(-100.f, dp->battle_score[lose][win]-pts);
}
/* P1 — LE VAINQUEUR PRESSE : il tient le champ → il ASSIÈGE la région contestée au
 * lieu de décrocher en FA_IDLE (la racine des 0 occupations : 217 batailles, 0 siège).
 * Cible SEULEMENT si loc vaut un siège (ennemie, OU nôtre mais OCCUPÉE → libération) —
 * exactement les règles de l'arrivée de marche (151-159). false = rien à réduire (on
 * tient déjà la région, ou c'est notre terre libre : le défenseur reste, re-ordonnable). */
static bool bt_press_siege(Campaign *c, const WorldEconomy *e, const DiploState *dp,
                           FieldArmy *V, int loc){
    if (!V->active || V->broken_days>0 || force_units(&V->force)<=0) return false;
    if (loc<0 || loc>=e->n_regions) return false;
    bool ours = (e->region[loc].owner==V->owner);
    int  occ  = dp ? dp->occupier[loc] : -1;
    if (occ==V->owner || (ours && occ<0)) return false;
    V->loc=loc; V->dest=loc; V->next=-1; V->phase=FA_SIEGE;
    /* P2 — le RELIEF est défait : la place, privée d'espoir de secours, CAPITULE vite.
     * On borne le compte de siège à BT_RELIEF_FALL (≤ la fenêtre où l'armée ennemie gît
     * BRISÉE : BT_BRISEE_J=45 j) — sans quoi le secours se reforme et revient resetter
     * le siège, qui ne tombe JAMAIS (la racine des 0 occupations MALGRÉ des batailles
     * gagnées). days_left est sérialisé → sauver/recharger reste fidèle (pas de bump). */
    float full = siege_days(region_defense(e,loc), region_food_months(e,loc),
                            terrain_defense_mult(c->reg_biome[loc], c->reg_height[loc]))
               * posture_siege_mult(V->posture);
    V->days_left = fminf(full, tune_f("BT_RELIEF_FALL",30.f));
    return true;
}
static void bt_end(Campaign *c, const WorldEconomy *e, const DiploState *dp, FieldBattle *bt){
    /* Le VAINQUEUR = le principal encore en lice (FA_BATTLE, actif, NON brisé) au moral
     * restant le plus haut ; il PRESSE (assiège la région contestée). Les autres — le
     * brisé déjà en fuite, les renforts, ou le nul de stalemate (les DEUX brisés) —
     * rentrent au repos. Aucun vainqueur si la PAIX a éclaté (on n'assiège pas hors guerre). */
    int victor=-1;
    if (bt->a>=0 && bt->b>=0
        && diplo_status(dp,c->army[bt->a].owner,c->army[bt->b].owner)==DIPLO_WAR){
        FieldArmy *A=&c->army[bt->a], *B=&c->army[bt->b];
        float fA=bt->resA/(bt->resA0+1.f), fB=bt->resB/(bt->resB0+1.f);
        bool okA=(A->phase==FA_BATTLE && A->active && A->broken_days==0);
        bool okB=(B->phase==FA_BATTLE && B->active && B->broken_days==0);
        if      (okA && (!okB || fA>=fB)) victor=bt->a;
        else if (okB)                     victor=bt->b;
    }
    int ids[4]={bt->a,bt->b,bt->helpA,bt->helpB};
    for (int k=0;k<4;k++){
        if (ids[k]<0) continue;
        FieldArmy *A=&c->army[ids[k]];
        if (A->phase!=FA_BATTLE) continue;
        if (ids[k]==victor && bt_press_siege(c,e,dp,A,bt->loc)) continue;  /* P1 : il presse */
        A->phase=FA_IDLE;
    }
    bt->active=false;
}
/* la part MONTÉE d'une force [0..1] : la cavalerie court-sus aux fuyards — c'est ELLE
 * qui fait la poursuite. Dominante dans la compo ⇒ la curée doit l'emporter sur le choc. */
static float army_cav_frac(const ArmyState *a){
    long cav=0, tot=0;
    for (int i=0;i<a->n_units;i++){
        long n=a->units[i].count; if (n<=0) continue; tot+=n;
        if (a->units[i].type==U_CAV_LEGERE || a->units[i].type==U_CAV_LOURDE) cav+=n;
    }
    return tot>0 ? (float)cav/(float)tot : 0.f;
}
/* la DÉROUTE : la poursuite fauche (posture, moral restant, terrain de fuite) ; le
 * vaincu fuit BRISÉ vers sa capitale ; le score encaisse le grand swing. */
static void bt_rout(Campaign *c, const World *w, const WorldEconomy *e, DiploState *dp,
                    FieldBattle *bt, int loser_side /* 0=A 1=B */){
    int ia=bt->a, ib=bt->b;
    FieldArmy *L=&c->army[loser_side?ib:ia], *V=&c->army[loser_side?ia:ib];
    float vfrac=(loser_side? bt->resA/(bt->resA0+1.f) : bt->resB/(bt->resB0+1.f));
    /* P3 — la curée est ALLÉGÉE (plafond ≤ 12 %, socle 6 %) : une armée battue SURVIT
     * pour revenir — la guerre s'inscrit dans la DURÉE (ré-assauts) au lieu d'annihiler
     * l'assaillant au premier choc. Toujours poussée par la posture agressive.
     * H4/L4 — LA CAVALERIE FAIT LA POURSUITE : la part montée du VAINQUEUR pousse la curée
     * ET en RELÈVE le plafond → cavalerie DOMINANTE ⇒ la poursuite DOMINE le choc ;
     * infanterie pure ⇒ le choc peut primer (le slugfest frontal). */
    float cavf=army_cav_frac(&V->force);
    float ctrv=side_counter(&V->force,&L->force);   /* le vainqueur qui CONTRAIT le vaincu fait une curée plus totale */
    float P=0.06f + ((V->posture==FA_AGRESSIVE)?0.08f:(V->posture==FA_PRUDENTE)?-0.03f:0.f)
          + 0.04f*vfrac + tune_f("CAV_PURSUIT",0.45f)*cavf
          + tune_f("CTR_PURSUIT",0.30f)*fmaxf(0.f, ctrv-1.f);
    if (terrain_combat_bonus(c->reg_biome[bt->loc])>1.10f) P-=0.04f;   /* la montagne couvre la fuite */
    float cap=tune_f("CUREE_CAP",0.22f) + tune_f("CAV_CUREE_CAP",0.40f)*cavf;  /* la cavalerie relève le plafond de curée */
    P=fminf(cap,fmaxf(0.03f,P));
    long lp=force_units(&L->force);
    long to_kill=(long)((float)lp*P+0.5f);
    if (!L->rally_used && to_kill>=lp) to_kill=lp-1;   /* L2 : le NOYAU survit pour se rallier */
    long pursued=kill_packets(&L->force,to_kill);
    c->dead_pursuit += pursued*100;                                    /* la curée : l'essentiel des morts */
    c->n_routs++;
    /* L2 — LE RALLIEMENT : les fuyards se reformeront (40-60 % de l'avant-déroute,
     * 30-60 j) — une fois par guerre. Déterministe : dérivé de l'effectif. */
    if (!L->rally_used && force_units(&L->force)>0 && lp>0){
        L->rally_used   = true;
        L->rally_days   = 30.f + (float)(lp % 31);                     /* 30-60 j */
        L->rally_packets= (int)((float)lp * (0.40f + 0.01f*(float)(lp % 21)));  /* 40-60 % */
        if (L->rally_packets < 1) L->rally_packets = 1;
    }
    bt_score(dp, V->owner, L->owner, 6.f+fminf(12.f,(float)pursued*0.6f));
    L->broken_days=BT_BRISEE_J;
    L->phase=FA_IDLE; L->dest=-1; L->next=-1;
    { int cp=(L->owner>=0&&L->owner<w->n_countries)?w->country[L->owner].capital_prov:-1;
      int cr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
      if (cr>=0 && cr!=L->loc){
          int hop=next_hop(c,e,L->loc,cr);
          if (hop>=0){ L->dest=cr; L->next=hop; L->phase=FA_MARCH;
                       L->leg_days=army_step_days(&L->force,c->reg_biome[hop],c->reg_height[hop],false,false)*0.8f;
                       L->days_left=L->leg_days; }
      } }
    if (force_units(&L->force)<=0){ L->active=false; L->phase=FA_IDLE; }
    /* P1 — le vainqueur V reste EN LICE (FA_BATTLE) : bt_end, appelé juste après, le
     * fait PRESSER le siège de la région contestée au lieu de décrocher (avant : V→IDLE
     * et la guerre se vidait — 217 batailles pour 0 occupation). */
    (void)V;
}
/* MARCHE AU CANON : un allié/suzerain/vassal ADJACENT et dispo rejoint le camp. */
static void bt_reinforce(Campaign *c, const WorldEconomy *e, const DiploState *dp, FieldBattle *bt){
    for (int side=0; side<2; side++){
        int *slot = side? &bt->helpB : &bt->helpA;
        if (*slot>=0) continue;
        int own = side? c->army[bt->b].owner : c->army[bt->a].owner;
        for (int k=0;k<SCPS_MAX_COUNTRY;k++){
            FieldArmy *H=&c->army[k];
            if (!H->active || H->broken_days>0 || H->phase==FA_BATTLE) continue;
            if (H->owner==own) continue;
            bool lie = (diplo_status(dp,H->owner,own)==DIPLO_ALLIED)
                    || (diplo_suzerain(dp,H->owner)==own) || (diplo_suzerain(dp,own)==H->owner);
            if (!lie) continue;
            if (H->loc!=bt->loc && !(H->loc>=0 && bt->loc>=0 && e->adj[H->loc][bt->loc])) continue;
            float add=side_reserve(&H->force);            /* ses paquets s'ajoutent au jour d'arrivée */
            if (side){ bt->resB+=add; bt->resB0+=add; } else { bt->resA+=add; bt->resA0+=add; }
            *slot=k; H->phase=FA_BATTLE; H->loc=bt->loc; H->dest=-1; H->next=-1;
            c->n_reinforce++;
            break;
        }
    }
}
/* UN JOUR de bataille : choc (jets, pertes, moral) ou accalmie (récup, décrochage). */
static void bt_day(Campaign *c, const World *w, const WorldEconomy *e, DiploState *dp,
                   FieldBattle *bt, uint32_t *rng){
    FieldArmy *A=&c->army[bt->a], *B=&c->army[bt->b];
    if (!A->active||!B->active){ bt_end(c,e,dp,bt); return; }
    if (diplo_status(dp,A->owner,B->owner)!=DIPLO_WAR){ bt_end(c,e,dp,bt); return; }   /* la paix a éclaté */
    bt->days++; c->battle_days++;
    int ph=bt->cycle % (BT_CHOC_J+BT_ACCALMIE_J);
    if (ph<BT_CHOC_J){
        bt->chocs++;
        float tA=bt_terrainA(c,e,bt->loc,A->owner,B->owner);
        float ctrA=powf(side_counter(&A->force,&B->force), tune_f("CTR_BITE",0.6f)); /* le contre PRIME sur la qualité brute */
        float ctrB=powf(side_counter(&B->force,&A->force), tune_f("CTR_BITE",0.6f));
        float pA=side_power(&A->force)*tA*ctrA *((A->posture==FA_AGRESSIVE)?1.10f:(A->posture==FA_PRUDENTE)?0.92f:1.f);
        float pB=side_power(&B->force)/tA*ctrB *((B->posture==FA_AGRESSIVE)?1.10f:(B->posture==FA_PRUDENTE)?0.92f:1.f);
        pA*=0.85f+0.30f*xs01(rng); pB*=0.85f+0.30f*xs01(rng);
        float tot=pA+pB+1e-3f;
        float dmgk=tune_f("BT_DMG_K",BT_DMG_K);
        bt->resB -= bt->resB0*dmgk*(2.f*pA/tot);
        bt->resA -= bt->resA0*dmgk*(2.f*pB/tot);
        bt->lossB += (float)force_units(&B->force)*tune_f("BT_CHOC_MORTS",BT_CHOC_MORTS)*(2.f*pA/tot);
        bt->lossA += (float)force_units(&A->force)*tune_f("BT_CHOC_MORTS",BT_CHOC_MORTS)*(2.f*pB/tot);
        long mB=0,mA=0;
        if (bt->lossB>=1.f){ mB=kill_packets(&B->force,(long)bt->lossB); bt->lossB-=(float)mB; }
        if (bt->lossA>=1.f){ mA=kill_packets(&A->force,(long)bt->lossA); bt->lossA-=(float)mA; }
        c->dead_choc += (mA+mB)*100;
        bt_score(dp,(pA>=pB)?A->owner:B->owner,(pA>=pB)?B->owner:A->owner,0.35f);  /* le jour gagné pèse un peu */
        /* L3/H4.2 — LE CHOC TIENT : pas de test de déroute avant CHOC_ROUNDS_BONUS
         * chocs livrés (les batailles durent, le positionnement compte). */
        if ((float)bt->chocs >= tune_f("CHOC_ROUNDS_BONUS",2.f)){
        if (bt->resA<=tune_f("BT_RUPTURE",BT_RUPTURE)*bt->resA0){ bt_rout(c,w,e,dp,bt,0); bt_end(c,e,dp,bt); return; }
        if (bt->resB<=tune_f("BT_RUPTURE",BT_RUPTURE)*bt->resB0){ bt_rout(c,w,e,dp,bt,1); bt_end(c,e,dp,bt); return; }
        }
    } else {
        float rA=BT_RECUP + ((e->region[bt->loc].owner==A->owner)?0.007f:0.f);
        float rB=BT_RECUP + ((e->region[bt->loc].owner==B->owner)?0.007f:0.f);
        bt->resA=fminf(bt->resA0, bt->resA+bt->resA0*rA);
        bt->resB=fminf(bt->resB0, bt->resB+bt->resB0*rB);
        if (ph==BT_CHOC_J){                            /* DÉCROCHER : en ordre, poursuite réduite */
            float fA=bt->resA/(bt->resA0+1.f), fB=bt->resB/(bt->resB0+1.f);
            /* P3 — le décrochage est l'EXCEPTION : on ne rompt qu'à moral BAS (seuil
             * abaissé, tunable). Sinon la bataille se DÉCIDE (déroute) — c'est ce qui
             * fait tomber les sièges et prendre le terrain (avant : 98 % de décrochages,
             * 0 occupation). Posture prudente = plus prompte à rompre (+0.10). */
            float base=tune_f("BT_DECROCHE",0.22f);
            float sA=(A->posture==FA_PRUDENTE)?base+0.10f:base, sB=(B->posture==FA_PRUDENTE)?base+0.10f:base;
            int who=(fA<sA && fA<fB-0.08f)?0:(fB<sB && fB<fA-0.08f)?1:-1;
            if (who>=0){
                FieldArmy *L=&c->army[who?bt->b:bt->a], *V=&c->army[who?bt->a:bt->b];
                long lp=force_units(&L->force);
                long pursued=kill_packets(&L->force,(long)((float)lp*0.08f+0.5f));
                c->dead_pursuit+=pursued*100; c->n_disengage++;
                bt_score(dp,V->owner,L->owner,2.f);
                L->phase=FA_IDLE; L->dest=-1; L->next=-1; L->broken_days=10;
                bt_end(c,e,dp,bt); return;
            }
        }
    }
    if (bt->days>=BT_MAX_JOURS){                       /* le délitement : nul sanglant */
        c->n_stalemate++;
        int up=(bt->resA/(bt->resA0+1.f)>=bt->resB/(bt->resB0+1.f))?1:0;
        bt_score(dp, up?A->owner:B->owner, up?B->owner:A->owner, 2.f);
        A->broken_days=15; B->broken_days=15;
        bt_end(c,e,dp,bt); return;
    }
    bt->cycle++;
}
static void bt_engage(Campaign *c, int i, int j, int loc){
    for (int k=0;k<CAMPAIGN_MAX_BATTLES;k++){
        FieldBattle *bt=&c->battle[k];
        if (bt->active) continue;
        memset(bt,0,sizeof *bt);
        bt->active=true; bt->a=i; bt->b=j; bt->helpA=-1; bt->helpB=-1; bt->loc=loc;
        bt->resA=bt->resA0=side_reserve(&c->army[i].force);
        bt->resB=bt->resB0=side_reserve(&c->army[j].force);
        c->army[i].phase=FA_BATTLE; c->army[j].phase=FA_BATTLE;
        c->army[i].battles++; c->army[j].battles++;
        c->n_battles++;
        return;
    }
}

/* ---- Le tick ---------------------------------------------------------- */
void campaign_tick(Campaign *c, const World *w, const WorldEconomy *e,
                   DiploState *dp, uint32_t *rng, float dt_days){
    if (dt_days<=0.f) return;

    /* §terrain : une armée dont le PAYS est MORT (annexé → role UNCLAIMED) se DISSOUT
     * — pas de zombie en campagne. (warhost/marine se taisent par le même skip ailleurs.) */
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        FieldArmy *a=&c->army[i];
        if (a->active && a->owner>=0 && a->owner<w->n_countries
            && w->country[a->owner].role==POLITY_UNCLAIMED){
            a->active=false; a->phase=FA_IDLE; a->dest=a->next=-1; a->taken_region=-1;
        }
    }

    /* 1. batailles : paires hostiles (en guerre) partageant une région. */
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        if (!c->army[i].active) continue;
        for (int j=i+1;j<SCPS_MAX_COUNTRY;j++){
            if (!c->army[j].active) continue;
            if (c->army[i].loc != c->army[j].loc) continue;
            if (c->army[i].owner == c->army[j].owner) continue;
            if (diplo_status(dp, c->army[i].owner, c->army[j].owner)!=DIPLO_WAR) continue;
            if (c->army[i].phase==FA_BATTLE || c->army[j].phase==FA_BATTLE) continue;   /* déjà accrochées */
            if (c->army[i].phase>=FA_EMBARK || c->army[j].phase>=FA_EMBARK) continue;    /* en mer : intouchable (v2 : l'interception) */
            if (c->army[i].broken_days>0 || c->army[j].broken_days>0) continue;          /* une brisée FUIT, ne s'accroche pas */
            bt_engage(c, i, j, c->army[i].loc);
        }
    }
    /* 1b. les BATAILLES vivent leurs jours : choc · accalmie · déroute · poursuite —
     * et le renfort allié adjacent MARCHE AU CANON. */
    { int nd=(int)dt_days; if (nd<1) nd=1;
      for (int d0=0; d0<nd; d0++){
          for (int k=0;k<CAMPAIGN_MAX_BATTLES;k++)
              if (c->battle[k].active){ bt_reinforce(c,e,dp,&c->battle[k]); bt_day(c,w,e,dp,&c->battle[k],rng); }
      }
      for (int i=0;i<SCPS_MAX_COUNTRY;i++)
          if (c->army[i].broken_days>0){ c->army[i].broken_days-= nd; if (c->army[i].broken_days<0) c->army[i].broken_days=0; }
      /* L2 — LE RALLIEMENT : le compte à rebours s'égrène ; à zéro, les fuyards se
       * REFORMENT — l'effectif remonte à la cible (40-60 % de l'avant-déroute, JAMAIS
       * au-dessus : les déserteurs reviennent, les morts non), la brisure se lève. */
      for (int i=0;i<SCPS_MAX_COUNTRY;i++){
          FieldArmy *a2=&c->army[i];
          if (a2->rally_days<=0.f) continue;
          a2->rally_days -= (float)nd;
          if (a2->rally_days>0.f) continue;
          a2->rally_days=0.f;
          long cur=force_units(&a2->force);
          if (!a2->active || cur<=0){ a2->rally_packets=0; continue; }  /* morte en route : rien à rallier */
          if (cur < a2->rally_packets){
              /* remonte chaque unité au prorata vers la cible (composition conservée) */
              float k2=(float)a2->rally_packets/(float)cur;
              long total=0;
              for (int u=0;u<a2->force.n_units;u++){
                  a2->force.units[u].count=(long)((float)a2->force.units[u].count*k2);
                  if (a2->force.units[u].count<1) a2->force.units[u].count=1;
                  total+=a2->force.units[u].count;
              }
              while (total>a2->rally_packets && a2->force.n_units>0){   /* cap STRICT : jamais au-dessus */
                  for (int u=0;u<a2->force.n_units && total>a2->rally_packets;u++)
                      if (a2->force.units[u].count>1){ a2->force.units[u].count--; total--; }
                  if (total>a2->rally_packets) break;                   /* tout à 1 : on s'arrête */
              }
          }
          a2->broken_days=0;                                            /* reformée : apte au combat */
          a2->rally_packets=0;
          c->n_rallies++;
      }
      /* L2 — « une fois par GUERRE » : la paix relâche le ralliement consommé. */
      if (dp) for (int i=0;i<SCPS_MAX_COUNTRY;i++){
          if (!c->army[i].rally_used || c->army[i].rally_days>0.f) continue;
          bool at_war=false;
          for (int b=0;b<SCPS_MAX_COUNTRY;b++)
              if (b!=i && diplo_status(dp,i,b)==DIPLO_WAR){ at_war=true; break; }
          if (!at_war) c->army[i].rally_used=false;
      }
    }

    /* 2. avancement : on consomme dt_days à travers les étapes ET le siège. */
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        FieldArmy *a=&c->army[i];
        if (!a->active) continue;
        if (a->phase==FA_BATTLE) continue;            /* ÉPINGLÉE : le champ la tient */
        float t=dt_days; int guard=0;
        while (t>0.f && a->active && ++guard<100000){
            if (a->phase==FA_MARCH){
                if (t < a->days_left){ a->days_left-=t; t=0.f; break; }
                t -= a->days_left; a->days_left=0.f;
                int to=a->next;                                  /* on arrive */
                army_march_attrition(&a->force, c->reg_biome[to], a->leg_days);  /* la marche use */
                a->loc=to; a->legs++;
                if (force_units(&a->force)<=0){ a->active=false; a->phase=FA_IDLE; break; }
                if (a->loc==a->dest){
                    bool ours = (e->region[a->loc].owner==a->owner);  /* même index que partout : pas de borne neuve */
                    int  occ  = dp ? dp->occupier[a->loc] : -1;
                    if (occ==a->owner || (ours && occ<0)){        /* déjà tenue par nous, ou notre terre LIBRE : rien à réduire */
                        a->phase=FA_IDLE; a->dest=-1; a->next=-1; break;
                    }
                    a->phase=FA_SIEGE; a->next=-1;                /* ennemie, OU notre terre OCCUPÉE : on assiège (libération) */
                    a->days_left = siege_days(region_defense(e,a->loc),
                                              region_food_months(e,a->loc),
                                              terrain_defense_mult(c->reg_biome[a->loc], c->reg_height[a->loc]))
                                 * posture_siege_mult(a->posture);
                } else {
                    int hop=next_hop(c,e,a->loc,a->dest);         /* étape suivante */
                    if (hop<0){ a->phase=FA_IDLE; a->dest=-1; a->next=-1; break; }
                    a->next=hop;
                    a->leg_days  = army_step_days(&a->force, c->reg_biome[hop], c->reg_height[hop], false, false)
                                 * posture_march_mult(a->posture);
                    a->days_left = a->leg_days;
                }
            } else if (a->phase==FA_SIEGE){
                if (t < a->days_left){ a->days_left-=t; t=0.f; break; }
                t -= a->days_left; a->days_left=0.f;
                a->taken++;
                a->taken_region=a->loc;                          /* à récolter : occupation/libération (couche sim) */
                a->phase=FA_IDLE; a->dest=-1;
                break;
            } else if (a->phase==FA_EMBARK){                     /* mer §6 : on charge au port */
                if (t < a->days_left){ a->days_left-=t; t=0.f; break; }
                t -= a->days_left;
                a->phase=FA_SAIL; a->leg_days=a->sail_days; a->days_left=a->sail_days;
            } else if (a->phase==FA_SAIL){                       /* en mer : intouchable, AVEUGLE */
                if (t < a->days_left){ a->days_left-=t; t=0.f; break; }
                t -= a->days_left;
                a->loc=a->dest; a->legs++;
                a->phase=FA_LAND;
                a->leg_days = (3.f + (float)force_units(&a->force)/20.f)
                            * (a->land_at_port?1.f:1.6f);        /* hors port : plus lent */
                a->days_left=a->leg_days;
            } else if (a->phase==FA_LAND){
                if (t < a->days_left){ a->days_left-=t; t=0.f; break; }
                t -= a->days_left; a->days_left=0.f;
                if (!a->land_at_port)                            /* la grève EXPOSE — léger, pas un mur */
                    army_march_attrition(&a->force, c->reg_biome[a->loc], a->leg_days*0.5f);
                if (force_units(&a->force)<=0){ a->active=false; a->phase=FA_IDLE; break; }
                if (e->region[a->loc].owner==a->owner){          /* notre terre : débarqué, c'est tout */
                    a->phase=FA_IDLE; a->dest=-1; a->next=-1; break;
                }
                a->phase=FA_SIEGE; a->next=-1;                   /* l'ennemi : on assiège depuis la côte */
                a->days_left = siege_days(region_defense(e,a->loc),
                                          region_food_months(e,a->loc),
                                          terrain_defense_mult(c->reg_biome[a->loc], c->reg_height[a->loc]))
                             * posture_siege_mult(a->posture);
                break;
            } else break;                                        /* FA_IDLE */
        }
    }
}

/* ---- Lecteurs --------------------------------------------------------- */
bool campaign_active(const Campaign *c, int o){
    return (o>=0 && o<SCPS_MAX_COUNTRY) && c->army[o].active;
}
int campaign_location(const Campaign *c, int o){
    return (o>=0 && o<SCPS_MAX_COUNTRY && c->army[o].active) ? c->army[o].loc : -1;
}
FieldPhase campaign_phase(const Campaign *c, int o){
    return (o>=0 && o<SCPS_MAX_COUNTRY) ? c->army[o].phase : FA_IDLE;
}
long campaign_units(const Campaign *c, int o){
    if (o<0 || o>=SCPS_MAX_COUNTRY) return 0;
    return force_units(&c->army[o].force);
}
int campaign_taken(const Campaign *c, int o){
    return (o>=0 && o<SCPS_MAX_COUNTRY) ? c->army[o].taken : 0;
}
long campaign_disband(Campaign *c, int o){
    if (!c || o<0 || o>=SCPS_MAX_COUNTRY) return 0;
    FieldArmy *a=&c->army[o];
    long packets = force_units(&a->force);            /* ce qu'on dissout (UI / restitution) */
    int posture = a->posture;                          /* on garde le réglage joueur */
    army_init(&a->force);                              /* la composition s'évapore */
    a->active=false; a->loc=-1; a->dest=-1; a->next=-1; a->phase=FA_IDLE;
    a->days_left=0.f; a->leg_days=0.f; a->taken=0; a->taken_region=-1;
    a->legs=0; a->battles=0; a->broken_days=0;
    a->sail_transports=0; a->sail_days=0.f; a->land_at_port=false; a->intercept_done=false;
    a->posture=posture;
    return packets;
}
const char *campaign_phase_name(FieldPhase ph){
    switch (ph){
        case FA_IDLE:  return "Au repos";
        case FA_MARCH: return "En marche";
        case FA_SIEGE: return "En siège";
        case FA_BATTLE:return "En mêlée";
        case FA_EMBARK:return "Embarque";
        case FA_SAIL:  return "En mer";
        case FA_LAND:  return "Débarque";
        default:       return "?";
    }
}

/* ---- RENFORT (« remplir ») ------------------------------------------------- */
bool campaign_can_refill(const Campaign *c, const WorldEconomy *econ, int owner){
    if (!campaign_active(c,owner) || !econ) return false;
    int loc = campaign_location(c, owner);
    return loc>=0 && loc<econ->n_regions && econ->region[loc].owner==owner;  /* chez soi */
}
void campaign_refill_cost(const Campaign *c, int owner, long *men, long *mat){
    long m=0, mt=0;
    if (owner>=0 && owner<SCPS_MAX_COUNTRY){
        const ArmyState *a=&c->army[owner].force;
        for (int i=0;i<a->n_units;i++) if (a->units[i].count>0){ m+=POP_PER_UNIT; mt+=2; } /* 1 paquet + ~2 mat/arme */
    }
    if (men) *men=m;
    if (mat) *mat=mt;
}
int campaign_refill(Campaign *c, int owner, WorldEconomy *econ, LaborEcon *labor){
    if (owner<0 || owner>=SCPS_MAX_COUNTRY || !labor) return 0;
    ArmyState *a=&c->army[owner].force;
    int n=a->n_units, added=0;
    for (int i=0;i<n;i++){
        if (a->units[i].count<=0) continue;
        UnitType t=a->units[i].type; const UnitDef *d=unit_def(t); if(!d) continue;
        /* F6 (Option B) — le RENFORT POMPE les armes MACRO (RES_*) comme la levée : 100 armes = 1
         * paquet, puisées au stock de l'empire (→ demande → fer). Pas d'armes → pas de renfort. */
        if (econ && econ_arms_take(econ, owner, unit_res_arm(t), POP_PER_UNIT) < POP_PER_UNIT) continue;
        a->weapons[d->weapon] += 1;                       /* le tampon de combat (source : macro) */
        if (army_recruit(a, labor, t, 1)) added++;        /* lève un paquet de 100 */
    }
    return added;
}

ArmyComposition campaign_composition(const Campaign *c, int o){
    ArmyComposition z; memset(&z,0,sizeof z);
    if (o<0 || o>=SCPS_MAX_COUNTRY) return z;
    const ArmyState *a=&c->army[o].force;
    for (int i=0;i<a->n_units;i++){
        long n=a->units[i].count; if (n<=0) continue;
        switch (a->units[i].type){
            case U_PIQUIER: case U_LANCIER: case U_EPEISTE:   z.infanterie+=n; break;
            case U_ARCHER:  case U_ARBALETE:                  z.archers   +=n; break;
            case U_CAV_LEGERE: case U_CAV_LOURDE:             z.cavalerie +=n; break;
            case U_MAGE:                                      z.mages     +=n; break;
            default: break;
        }
        z.total+=n;
    }
    return z;
}

/* (army_host_word RETIRÉ — P1.10 : l'effectif EXACT s'affiche, plus de mot de
 * brouillard ; l'asymétrie d'information ennemie tombe.) */
