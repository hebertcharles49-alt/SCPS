/*
 * scps_routes.c — routes commerciales (voir scps_routes.h)
 *
 * Le rendement EST la cloche f(D̄) du moteur, faite action : on commerce le
 * mieux avec un partenaire à distance intermédiaire, et la porte se ferme aux
 * extrêmes (sauf forte ouverture P/port).
 */
#include "scps_routes.h"
#include <string.h>
#include <math.h>
#include "scps_math.h"   /* absf partagé */
#include "scps_tune.h"   /* M15 — F4 : CHOKE_REAL_PATH */

void routes_init(RouteNetwork *rn){ memset(rn,0,sizeof(*rn)); }

/* Le PORT RÉEL (mer §5) : l'édifice Port sur une côte — pas le Caravansérail. */
static bool has_true_port(const WorldEconomy *econ, int r){
    return (r>=0 && r<econ->n_regions
            && econ->region[r].build.port > 0.f && econ->region[r].coastal);
}
#define SEA_ROUTE_MAX_DAYS 60.f   /* V3 — plus un plafond de REJET : la portée de référence du rendement
                                   * maritime (2× = la borne du calcul de distance ET la distance virtuelle
                                   * d'un lien hors-portée — le lien existe, ténu). */

bool routes_order(RouteNetwork *rn, const World *w, const WorldEconomy *econ,
                  int ra, int rb, bool maritime){
    if (rn->n>=SCPS_MAX_ROUTES) return false;
    if (ra<0||rb<0||ra>=econ->n_regions||rb>=econ->n_regions||ra==rb) return false;
    for (int i=0;i<rn->n;i++){   /* une seule route par PAIRE (sinon l'IA sature le réseau) */
        const TradeRoute *t=&rn->route[i];
        if ((t->ra==ra&&t->rb==rb)||(t->ra==rb&&t->rb==ra)) return false;
    }
    if (!econ->region[ra].culture.settled || !econ->region[rb].culture.settled) return false;
    float sea_days=0.f;
    if (maritime){
        if (!has_true_port(econ,ra) || !has_true_port(econ,rb)) return false;  /* DEUX PORTS : la condition RÉELLE */
        if (!w) return false;
        int ax,ay,bx,by;
        if (!world_region_sea_anchor(w,ra,&ax,&ay)) return false;
        if (!world_region_sea_anchor(w,rb,&bx,&by)) return false;
        /* V3 — L'INTERACTION EST VIRTUELLE : deux ports SUFFISENT au lien. La distance de
         * mer ne FERME plus la route (le monde re-baseliné a écarté les côtes — la plus
         * proche d'un voisin tombait à 72 j, hors du vieux plafond de 60). Elle ne fait que
         * MODULER le rendement (routes_advance : yield ∝ 1/(1+sea_days/40) — loin = ténu,
         * jamais nul). Hors de portée du calcul (bassins séparés / au-delà de la borne) :
         * distance VIRTUELLE = la borne (le lien le plus mince), pas un rejet. */
        float cap=2.f*SEA_ROUTE_MAX_DAYS;
        float aller =world_sea_days_capped(w,ax,ay,bx,by, cap);
        float retour=world_sea_days_capped(w,bx,by,ax,ay, cap);
        sea_days = (aller<0.f || retour<0.f) ? cap : 0.5f*(aller+retour);   /* la route vit dans les DEUX sens */
    }
    TradeRoute *t=&rn->route[rn->n++];
    t->ra=ra; t->rb=rb; t->maritime=maritime;
    t->capacity=1.0f;
    t->days_total = maritime ? 120 : 90;   /* mer plus long (90-180 / 60-120) */
    t->days_done=0; t->open=false; t->yield=0.f;
    t->sea_days=sea_days; t->days_ab=0.f; t->days_ba=0.f;
    t->fluvial=0; t->flow=0.f; t->pirate_press=0.f;
    t->choke_region=-1; t->choke_block=0.f;
    if (maritime && w){
        int ax,ay,bx,by;
        if (world_region_sea_anchor(w,ra,&ax,&ay) && world_region_sea_anchor(w,rb,&bx,&by)){
            t->days_ab=world_sea_days_capped(w,ax,ay,bx,by, 2.f*SEA_ROUTE_MAX_DAYS);  /* route acceptée : legs < borne → exact */
            t->days_ba=world_sea_days_capped(w,bx,by,ax,ay, 2.f*SEA_ROUTE_MAX_DAYS);
            /* WG — LE DÉTROIT que cette route FRANCHIT.
             * M16 — C1 : LES CHOKES ÉMERGENTS (défaut ON, registre J CHOKE_EMERGENT) — la
             * concentration de trafic est une propriété du RÉSEAU ENTIER, pas d'une route
             * seule à sa création : on pose ICI un placeholder NEUTRE (-1, aucun péage) ;
             * routes_recompute_chokes (plus bas, appelée périodiquement par scps_sim.c,
             * ≤180 j) l'assigne d'après le trafic réel dès le prochain passage — jamais
             * plus de 180 j sans assignation correcte. CHOKE_EMERGENT=0 : legacy M15 EXACT
             * (table statique, posée UNE FOIS ici comme avant — CHOKE_REAL_PATH bascule le
             * test du SEGMENT DROIT au CHEMIN RÉEL, 0 = golden pré-M15 intact). */
            int ck = -1;
            if (tune_f("CHOKE_EMERGENT",1.f)<=0.f){
                ck = (tune_f("CHOKE_REAL_PATH",0.f)>0.f)
                     ? world_route_chokepoint_path(w,ax,ay,bx,by, 2.f*SEA_ROUTE_MAX_DAYS)
                     : world_route_chokepoint(w,ax,ay,bx,by);
            }
            if (ck>=0){
                const Chokepoint *tab=NULL; int nck=world_chokepoints(w,&tab);
                if (tab && ck<nck){ t->choke_region=tab[ck].region; t->choke_block=tab[ck].blockade; }
            }
        }
    }
    /* LA VOIE D'EAU INTÉRIEURE (commerce asym. §4) : si les deux régions vivent
     * le long du MÊME fleuve, la route terrestre l'emprunte — le tracé ordonné
     * (source → mer) donne le sens ; le débit, la capacité. */
    if (!maritime && w){
        for (int rv=0; rv<w->n_rivers && !t->fluvial; rv++){
            const River *R=&w->river[rv];
            int ia=-1, ib=-1;
            for (int k=0;k<R->len;k++){
                const Cell *c=scps_cellc(w, R->x[k], R->y[k]);
                if (c->region==ra && ia<0) ia=k;
                if (c->region==rb && ib<0) ib=k;
                if (ia>=0 && ib>=0) break;
            }
            if (ia>=0 && ib>=0 && ia!=ib){
                t->fluvial = (ia<ib) ? 1 : 2;              /* le plus proche de la SOURCE est l'amont */
                t->flow    = (R->flow_max>1.f)?1.f:(R->flow_max<0.f?0.f:R->flow_max);
            }
        }
    }
    return true;
}

/* Distance de CONTENU (L∞) entre les cultures de deux régions. */
static float route_dbar(const WorldEconomy *econ, int ra, int rb){
    const PopCulture *a=&econ->region[ra].culture, *b=&econ->region[rb].culture;
    float dv=absf(a->valeurs-b->valeurs), ds=absf(a->subsistance-b->subsistance);
    float dp=absf(a->parente-b->parente), dr=absf(a->religion-b->religion);
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}

void routes_advance(RouteNetwork *rn, const World *w, WorldEconomy *econ, int days){
    (void)w;
    /* RE-KEY PROVINCE : route_pe est PROVINCE-OWNED (Σ-agrégé, charte règle 1) —
     * region[r].route_pe est un DÉRIVÉ, écrasé au prochain econ_tick s'il est écrit
     * directement. Route le reset ET l'accumulation (plus bas) sur la représentative. */
    for (int r=0;r<econ->n_regions;r++){
        int rp=econ_region_rep_province(econ,r);
        if (rp>=0 && rp<econ->n_prov) econ->prov[rp].route_pe=0.f;
    }
    for (int i=0;i<rn->n;i++){
        TradeRoute *t=&rn->route[i];
        if (!t->open){
            t->days_done+=days;
            if (t->days_done>=t->days_total) t->open=true;
        }
        if (!t->open){ t->yield=0.f; continue; }
        if (t->ra>=econ->n_regions||t->rb>=econ->n_regions){ t->yield=0.f; continue; }

        float dbar=route_dbar(econ,t->ra,t->rb);
        float bell=dbar*(10.f-dbar)/25.f;                 /* CLOCHE : pic à D̄=5 */
        /* Porte : l'ouverture (ports/caravansérails) ouvre aux partenaires
         * lointains. P effectif = base + P_open moyen des deux bouts. */
        float Pavg=5.f + 0.5f*(econ->region[t->ra].build.P_open+econ->region[t->rb].build.P_open);
        float gate=1.f/(1.f+expf(-(0.8f*(Pavg-dbar))));
        t->yield = t->capacity * 10.f * bell * gate;       /* échelle ~ PE */
        if (t->maritime)                                    /* mer §7 : le COURANT fait la distance */
            t->yield *= 1.f/(1.f+t->sea_days/40.f);
        if (t->pirate_press>0.f){                           /* coques §4 : la SAIGNÉE pèse sur le flux */
            if (t->pirate_press>=90.f) t->yield=0.f;        /* blocus : le lien est COUPÉ */
            else t->yield *= 1.f/(1.f+0.08f*t->pirate_press);
        }
        { int rap=econ_region_rep_province(econ,t->ra), rbp=econ_region_rep_province(econ,t->rb);
          if (rap>=0 && rap<econ->n_prov) econ->prov[rap].route_pe += t->yield;
          if (rbp>=0 && rbp<econ->n_prov) econ->prov[rbp].route_pe += t->yield; }
    }
}

float routes_pe_for_region(const RouteNetwork *rn, int region){
    float s=0.f;
    for (int i=0;i<rn->n;i++) if (rn->route[i].open &&
        (rn->route[i].ra==region||rn->route[i].rb==region)) s+=rn->route[i].yield;
    return s;
}
int routes_count_for_region(const RouteNetwork *rn, int region){
    int n=0;
    for (int i=0;i<rn->n;i++) if (rn->route[i].open &&
        (rn->route[i].ra==region||rn->route[i].rb==region)) n++;
    return n;
}

/* M16 — C1 : LES CHOKES ÉMERGENTS — reconstruction PÉRIODIQUE (scps_sim.c, ≤180 j,
 * jamais au tick). Rassemble les ancres de TOUTES les routes maritimes OUVERTES (le
 * trafic RÉEL — une route encore en formation ne porte encore aucune cargaison),
 * DÉTERMINISTE (ordre de RouteNetwork, aucun tirage), délègue le calcul de
 * concentration à world_chokepoints_emergent_rebuild (scps_world.c), puis réécrit
 * choke_region/choke_block de CHAQUE route considérée — AUCUN champ neuf : les deux
 * mêmes champs sérialisés que posait déjà routes_order (SAVE_VERSION inchangée).
 * CHOKE_EMERGENT=0 : no-op — la table statique posée à la création reste seule
 * vérité (comportement M15 exact, gate 1). */
void routes_recompute_chokes(RouteNetwork *rn, const World *w){
    if (!rn || !w) return;
    if (tune_f("CHOKE_EMERGENT",1.f)<=0.f) return;
    int ax[SCPS_MAX_ROUTES], ay[SCPS_MAX_ROUTES], bx[SCPS_MAX_ROUTES], by[SCPS_MAX_ROUTES];
    int ridx[SCPS_MAX_ROUTES];
    int n=0;
    for (int i=0;i<rn->n && n<SCPS_MAX_ROUTES;i++){
        TradeRoute *t=&rn->route[i];
        if (!t->maritime || !t->open) continue;
        int a_x,a_y,b_x,b_y;
        if (!world_region_sea_anchor(w,t->ra,&a_x,&a_y)) continue;
        if (!world_region_sea_anchor(w,t->rb,&b_x,&b_y)) continue;
        ax[n]=a_x; ay[n]=a_y; bx[n]=b_x; by[n]=b_y; ridx[n]=i; n++;
    }
    int out_choke[SCPS_MAX_ROUTES];
    /* cap_days = 8×SEA_ROUTE_MAX_DAYS, volontairement PLUS LARGE que le cap 2× de la
     * création de route : mesuré (EMDIAG, seed 9) que le cap 2× exclut ~80 % des routes
     * maritimes OUVERTES du calcul (« injoignable dans CE rayon » — pas « pas de chemin » :
     * la portée de route est désormais VIRTUELLE, cf. V3 routes_order) avant même de
     * pouvoir compter la concentration — la table émergente restait vide alors que 31-55
     * routes étaient vivantes. SANS BORNE (-1) mesuré ENSUITE bien PIRE : une paire
     * réellement en bassins séparés fait explorer tout le bassin atteignable (le garde-fou
     * `cap_days` de sea_dijkstra_core EST ce qui borne ce coût — le retirer l'enlève aussi).
     * 8× est un compromis mesuré : couvre l'essentiel du trafic réel sans perdre le
     * garde-fou de coût sur les paires génuinement séparées. */
    world_chokepoints_emergent_rebuild(w, ax,ay,bx,by, n, 8.f*SEA_ROUTE_MAX_DAYS, out_choke);
    const Chokepoint *tab=NULL; int nck=world_chokepoints(w,&tab);
    for (int k=0;k<n;k++){
        TradeRoute *t=&rn->route[ridx[k]];
        int ck=out_choke[k];
        if (ck>=0 && tab && ck<nck){ t->choke_region=tab[ck].region; t->choke_block=tab[ck].blockade; }
        else { t->choke_region=-1; t->choke_block=0.f; }
    }
}
