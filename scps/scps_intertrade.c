/*
 * scps_intertrade.c — commerce inter-pays (voir scps_intertrade.h)
 *
 * Réutilise les stocks & prix de marché des régions (scps_econ) : pas de nouvel
 * état lourd. Les biens remontent la pente de prix le long des routes ouvertes,
 * la guerre coupe le robinet (embargo).
 */
#include "scps_intertrade.h"
#include "scps_agency.h"   /* E2 §10 : lire EDI_COMPTOIR dans le masque d'édifices bâtis */
#include "scps_tune.h"     /* I6 : marges d'import calibrables */
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ---- calibrage --------------------------------------------------------- */
#define IT_TRANSPORT   0.06f   /* coût de transport longue distance — la BASE de la fonction directionnelle */
#define IT_FLUV_DOWN   0.35f   /* descente : quasi gratuit (on flotte le bois, on descend le grain) */
#define IT_FLUV_UP     2.50f   /* remontée : le halage — cher, JAMAIS infini (plancher, pas mur)   */
#define IT_VOL_FLOOR   0.40f   /* la capacité à contre-courant : divisée, jamais nulle             */
#define IT_EXPORT_FRAC 0.25f   /* part du surplus que l'exportateur lâche par tick */
#define IT_PRICE_CONV  0.20f   /* convergence partielle des prix (lissage) */
#define IT_MARGIN_TO_GOLD 0.50f/* part de la valeur transportée qui devient de l'OR pour l'exportateur */

static float g_last_value = 0.f;   /* valeur totale échangée au dernier tick (reporting) */

/* COMMERCE ASYMÉTRIQUE — télémétrie par SENS : l'aval est le sens FACILE
 * (coût directionnel le plus bas) ; vrac = grain/bois/charbon ; précieux =
 * étoffe précieuse/orfèvrerie/remèdes/armes. Le tri ÉMERGE du test d'arbitrage. */
static float g_vol_down=0.f, g_vol_up=0.f;
static float g_bulk_down=0.f, g_bulk_up=0.f, g_prec_down=0.f, g_prec_up=0.f;
static int   g_nprec_up=0;     /* ÉVÉNEMENTS précieux à contre-courant (la niche du luxe) */
static inline bool it_is_bulk(int g){
    return g==RES_GRAIN||g==RES_WOOD||g==RES_COAL||g==RES_LIVESTOCK
         ||g==RES_CLAY ||g==RES_STONE;   /* E1 : les matériaux de construction voyagent en vrac */
}
static inline bool it_is_precious(int g){
    return g==RES_PRECIOUS_CLOTH||g==RES_PRECIOUS_WARE||g==RES_REMEDE
         ||g==RES_ARMS||g==RES_ENCHANTED_ARMS;
}
/* coût de transport SELON LE SENS (fraction du prix moyen) : terrestre
 * symétrique (intact) ; maritime = f(jours du chemin directionnel — la volta) ;
 * fluvial = descente bon marché / remontée chère. */
static float it_transport_frac(const TradeRoute *rt, bool a_to_b){
    if (rt->maritime){
        float d = a_to_b ? rt->days_ab : rt->days_ba;
        if (d<=0.f) d=rt->sea_days;
        if (d<=0.f) return IT_TRANSPORT;
        return IT_TRANSPORT*(0.5f + d/40.f);
    }
    if (rt->fluvial){
        bool descend = (rt->fluvial==1) ? a_to_b : !a_to_b;
        return IT_TRANSPORT*(descend ? IT_FLUV_DOWN : IT_FLUV_UP);
    }
    return IT_TRANSPORT;
}
/* le VOLUME suit la facilité : capacité × g(sens) — pleine en aval, plancher
 * à contre-courant (on divise, on n'annule jamais). */
static float it_volume_mult(const TradeRoute *rt, bool a_to_b){
    if (rt->maritime){
        float d=a_to_b?rt->days_ab:rt->days_ba; if (d<=0.f) d=rt->sea_days;
        float o=a_to_b?rt->days_ba:rt->days_ab; if (o<=0.f) o=rt->sea_days;
        if (d<=0.f||o<=0.f) return 1.f;
        float m=(o+10.f)/(d+10.f);
        return m<IT_VOL_FLOOR?IT_VOL_FLOOR:(m>1.6f?1.6f:m);
    }
    if (rt->fluvial){
        bool descend=(rt->fluvial==1)?a_to_b:!a_to_b;
        return descend ? 1.4f*(1.f+0.5f*rt->flow) : IT_VOL_FLOOR;   /* le gros fleuve porte plus */
    }
    return 1.f;
}
/* ce sens est-il l'AVAL (le sens facile) ? — pour la télémétrie/les mots. */
static bool it_is_downstream(const TradeRoute *rt, bool a_to_b){
    return it_transport_frac(rt,a_to_b) < it_transport_frac(rt,!a_to_b)-1e-6f;
}

/* ---- DÉTAIL par pays × bien + embargos décrétés (sidebar / décisions) ------
 * Flux du DERNIER tick (≈ l'année écoulée) : volume importé/exporté par bien,
 * volume par paire de partenaires (→ partenaire dominant), or encaissé, valeur
 * par paire (le prix d'un embargo). L'embargo décrété est UNILATÉRAL à poser
 * mais coupe la paire entière (personne ne commerce à travers un blocus). */
static float  g_imp [SCPS_MAX_COUNTRY][RES_COUNT];
static float  g_expt[SCPS_MAX_COUNTRY][RES_COUNT];
static float  g_imp_best [SCPS_MAX_COUNTRY][RES_COUNT];  /* meilleur volume vu (élection du dominant) */
static float  g_expt_best[SCPS_MAX_COUNTRY][RES_COUNT];
static int16_t g_imp_from[SCPS_MAX_COUNTRY][RES_COUNT];
static int16_t g_expt_to [SCPS_MAX_COUNTRY][RES_COUNT];
static float  g_gold[SCPS_MAX_COUNTRY];
static float  g_pair[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
static bool   g_embargo[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];   /* [qui décrète][contre qui] */
static bool   g_centre[SCPS_MAX_REG];   /* P3.20 : cette région EST un Centre commercial (hub) */
static float  g_centre_val[SCPS_MAX_REG]; /* valeur du commerce passée par chaque Centre (dernier tick) */
/* #5 — LA CARTE DU MARCHÉ LOCAL : chaque région se relie à la cité-état (Centre) la
 * PLUS PROCHE. g_hub_of = la région-Centre de rattachement (-1 = aucune atteignable :
 * autarcie) ; g_hub_dist = la distance (sauts) qui porte le rendement DÉGRESSIF.
 * Recalculé à chaque tick (les Centres bougent peu, mais la possession évolue). */
static int16_t g_hub_of  [SCPS_MAX_REG];
static int16_t g_hub_dist[SCPS_MAX_REG];
static bool    g_hub_dirty = true;          /* #5 : la carte des hubs ne dépend QUE des positions des Centres (statiques entre conquêtes) — on ne la RECALCULE qu'à un changement (seed/relocate), pas chaque jour */
static bool    g_global_access[SCPS_MAX_COUNTRY];  /* M3 : pays ayant accès au marché GLOBAL (Centre propre OU pacte avec un porteur de Centre) — recalculé 1×/tick */
static float   g_global_cache[RES_COUNT];   /* #5 : profondeur du marché mondial (Σ stocks des Centres), CALCULÉE 1×/tick (le devis d'achat la LIT en O(1), pas de re-somme dans la boucle chaude de l'IA) */
#define IT_SEA_HOPS 4   /* une région côtière sans hub par terre atteint un marché d'outre-mer à ce coût */

void intertrade_reset(void){
    memset(g_imp,0,sizeof g_imp);   memset(g_expt,0,sizeof g_expt);
    memset(g_imp_best,0,sizeof g_imp_best); memset(g_expt_best,0,sizeof g_expt_best);
    memset(g_imp_from,-1,sizeof g_imp_from); memset(g_expt_to,-1,sizeof g_expt_to);
    memset(g_gold,0,sizeof g_gold); memset(g_pair,0,sizeof g_pair);
    memset(g_embargo,0,sizeof g_embargo);
    memset(g_hub_of,-1,sizeof g_hub_of); memset(g_hub_dist,0,sizeof g_hub_dist);  /* #5 : autarcie tant qu'aucun tick n'a bâti la carte */
    memset(g_global_cache,0,sizeof g_global_cache); g_hub_dirty=true;
    memset(g_centre,0,sizeof g_centre);                 /* V2 : pas de FUITE de Centres entre parties d'une même session viewer */
    memset(g_global_access,0,sizeof g_global_access);   /* V2 : ni d'accès global hérité */
    g_last_value=0.f;
}
static void flows_clear(void){
    g_vol_down=g_vol_up=0.f; g_bulk_down=g_bulk_up=0.f; g_prec_down=g_prec_up=0.f; g_nprec_up=0;
    memset(g_centre_val,0,sizeof g_centre_val);
    memset(g_imp,0,sizeof g_imp);   memset(g_expt,0,sizeof g_expt);
    memset(g_imp_best,0,sizeof g_imp_best); memset(g_expt_best,0,sizeof g_expt_best);
    memset(g_imp_from,-1,sizeof g_imp_from); memset(g_expt_to,-1,sizeof g_expt_to);
    memset(g_gold,0,sizeof g_gold); memset(g_pair,0,sizeof g_pair);
}
static inline bool cid_ok(int c){ return c>=0 && c<SCPS_MAX_COUNTRY; }

/* ── P3.20 — CENTRES COMMERCIAUX (les hubs du réseau inter-régional) ──────────
 * Des POINTS STRATÉGIQUES, un par batch de ~4-5 régions, plantés là où le FLUX
 * est le plus fort (carrefour : degré d'adjacence + débouché côtier). Un pays
 * SANS Centre commercial dans son territoire n'a PAS accès au réseau d'échanges
 * inter-pays : il faut en CONQUÉRIR un (le thème du module). Seedés déterministes
 * (glouton, sans rng) ; sérialisés ; identiques au même monde à graine égale.   */
static float region_flow_score(const WorldEconomy *e, int r){
    int deg=0; for (int s=0;s<e->n_regions;s++) if (s!=r && e->adj[r][s]) deg++;
    float c=(float)deg;                       /* carrefour : plus de voisins = plus de flux */
    if (e->region[r].coastal) c+=2.5f;         /* débouché maritime : un grand multiplicateur de flux */
    return c;
}
/* M2 — g_centre DÉRIVE DU BÂTI : une région EST un Centre SSI EDI_TRADE_CENTER y est
 * posé (edi_built). Le flag spawné a disparu — le hub est CAUSAL (un bâtiment), donc
 * conquérable (le bâti voyage avec la région) et persistant. On ne marque la carte des
 * hubs « dirty » QUE si un Centre apparaît/disparaît (perf : pas de BFS chaque tick). */
static void refresh_centres(const WorldEconomy *e){
    if (!e) return;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    for (int r=0;r<n;r++){
        bool now = ((e->region[r].edi_built >> EDI_TRADE_CENTER) & 1u) != 0;
        if (now != g_centre[r]){ g_centre[r]=now; g_hub_dirty=true; }
    }
}
void intertrade_seed_centres(const World *w, WorldEconomy *e){
    if (!e) return;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    /* M2 — CHAQUE cité-état NAÎT avec un Centre commercial BÂTI sur sa MEILLEURE région
     * (carrefour : adjacence + débouché côtier) : POLITY_CITY_STATE = hub PAR NATURE,
     * garanti dès l'an 0 (pas de cold-start du global). C'est un BÂTIMENT (edi_built),
     * pas un flag → g_centre en dérive ; un EMPIRE marchand côtier peut AUSSI en bâtir un. */
    if (w) for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role!=POLITY_CITY_STATE) continue;
        int best=-1; float bestc=-1.f;
        for (int r=0;r<n;r++){
            if (e->region[r].owner!=c) continue;
            float fc=region_flow_score(e,r); if(fc>bestc){bestc=fc;best=r;}
        }
        if (best>=0) e->region[best].edi_built |= (1u<<EDI_TRADE_CENTER);
    }
    refresh_centres(e);
}
bool intertrade_has_centre(int region){
    return (region>=0&&region<SCPS_MAX_REG)?g_centre[region]:false;
}
bool intertrade_country_has_centre(const WorldEconomy *e, int cid){
    if (!e||!cid_ok(cid)) return false;
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++)
        if (g_centre[r] && e->region[r].owner==cid) return true;
    return false;
}
int intertrade_country_centre(const WorldEconomy *e, int cid){
    if (!e||!cid_ok(cid)) return -1;
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++)
        if (g_centre[r] && e->region[r].owner==cid) return r;
    return -1;
}
/* M3 — un pays a-t-il accès au marché GLOBAL ? (Centre propre OU pacte avec un porteur).
 * Lit le cache recalculé 1×/tick par intertrade_tick (O(1)). */
bool intertrade_has_global_access(int cid){ return cid_ok(cid) && g_global_access[cid]; }
/* RELOCALISER un Centre commercial (coût en or côté appelant) : le hub se DÉPLACE
 * de `from` vers `to` — il ne meurt pas, il bouge. `to` ne doit pas déjà en être un.
 * M2 — on déplace le BÂTIMENT (edi_built) ; g_centre en re-dérive. */
bool intertrade_relocate_centre(WorldEconomy *e, int from, int to){
    if (!e||from<0||from>=e->n_regions||to<0||to>=e->n_regions) return false;
    if (!((e->region[from].edi_built>>EDI_TRADE_CENTER)&1u) || ((e->region[to].edi_built>>EDI_TRADE_CENTER)&1u)) return false;
    e->region[from].edi_built &= ~(1u<<EDI_TRADE_CENTER);
    e->region[to].edi_built   |=  (1u<<EDI_TRADE_CENTER);
    refresh_centres(e); return true;
}

/* #5 — RATTACHER CHAQUE RÉGION À SON MARCHÉ LOCAL (la cité-état la plus proche).
 * BFS MULTI-SOURCE sur l'adjacence depuis TOUS les Centres à la fois : la 1re vague
 * qui touche une région donne le hub le plus proche + la distance (sauts). Une 2e
 * passe relie les régions CÔTIÈRES non atteintes par terre au marché d'outre-mer
 * (un Centre côtier, au coût forfaitaire d'une traversée). Déterministe (ordre
 * d'index pour les ex æquo), recalculé chaque tick. */
static void hub_map_build(const WorldEconomy *e){
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    static int16_t q[SCPS_MAX_REG]; int qh=0,qt=0;
    for (int r=0;r<SCPS_MAX_REG;r++){ g_hub_of[r]=-1; g_hub_dist[r]=0; }
    for (int r=0;r<n;r++) if (g_centre[r]){ g_hub_of[r]=(int16_t)r; q[qt++]=(int16_t)r; }
    while (qh<qt){
        int r=q[qh++];
        for (int s=0;s<n;s++){
            if (!e->adj[r][s] || g_hub_of[s]>=0) continue;     /* déjà rattaché (ou source) → on garde le plus proche */
            g_hub_of[s]=g_hub_of[r]; g_hub_dist[s]=(int16_t)(g_hub_dist[r]+1); q[qt++]=(int16_t)s;
        }
    }
    int sea_hub=-1; for (int r=0;r<n;r++) if (g_centre[r] && e->region[r].coastal){ sea_hub=r; break; }
    if (sea_hub>=0) for (int r=0;r<n;r++)
        if (g_hub_of[r]<0 && e->region[r].coastal){ g_hub_of[r]=(int16_t)sea_hub; g_hub_dist[r]=IT_SEA_HOPS; }
}
/* profondeur du marché mondial mise en cache (1×/tick, bon marché) : la somme des stocks
 * de tous les Centres, lue en O(1) par le devis d'achat (la boucle d'évaluation de l'IA). */
static void global_cache_refresh(const WorldEconomy *e){
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    memset(g_global_cache,0,sizeof g_global_cache);
    for (int r=0;r<n;r++) if (g_centre[r])
        for (int g=1; g<RES_COUNT; g++) g_global_cache[g] += e->region[r].stock[g];
}

/* #5 (lecteurs) — le hub local d'une région ; le stock du MARCHÉ MONDIAL = le réseau
 * des cités-états (la somme des stocks de tous les Centres : pas un pool parallèle à
 * resynchroniser, le réseau LUI-MÊME est le marché global ; un VRAI stock, drainé par
 * les achats, alimenté par la production qui converge vers les hubs). */
int   intertrade_region_hub(int region){ return (region>=0&&region<SCPS_MAX_REG)?g_hub_of[region]:-1; }
float intertrade_global_stock(const WorldEconomy *e, int good){
    (void)e; return (good>RES_NONE && good<RES_COUNT)? g_global_cache[good] : 0.f;   /* cache 1×/tick */
}
/* LES 3 ÉTAGES D'UN APPROVISIONNEMENT : combien sortira du stock PROPRE (production
 * sur place, marge nue), du marché LOCAL (le Centre le plus proche, marge de base),
 * du marché MONDIAL (les autres Centres, DOUBLE marge). Pur (pas de mutation). */
static void market_split(float own, float local, float glob, float qty,
                         float *po, float *pl, float *pg){
    float t = own  <qty?own  :qty; *po=t; qty-=t; if(qty<0)qty=0;
    t       = local<qty?local:qty; *pl=t; qty-=t; if(qty<0)qty=0;
    t       = glob <qty?glob :qty; *pg=t;
}
/* #5 — LE COÛT DU PUMP À 2 ÉTAGES (devis, sans mutation). Sourcer `qty` de `good` dans
 * `region` : stock propre (×1) → marché LOCAL cité-état (×marge de base, distance déjà
 * incluse) → marché MONDIAL (×marge×2 : la double taxe) → pénurie introuvable (×marge×2).
 * `unit_price` = le prix de marché local du bien (lu par l'appelant). */
float intertrade_buy_cost(const WorldEconomy *e, int region, int good, float qty, float unit_price){
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||qty<=0.f) return 0.f;
    const RegionEconomy *re=&e->region[region];
    float base = re->import_margin; if (base<1.f) base=1.f;
    int hub = (region<SCPS_MAX_REG)? g_hub_of[region] : -1;
    float own   = re->stock[good];
    float local = (hub>=0 && hub!=region)? e->region[hub].stock[good] : 0.f;
    /* le mondial = le réseau (cache 1×/tick) MOINS l'étage local déjà compté (le hub, et
     * la région elle-même si elle est un Centre) ; O(1), pas de re-somme dans l'IA. */
    float glob  = 0.f;
    if (hub>=0){ glob = g_global_cache[good] - local;
        if (region<SCPS_MAX_REG && g_centre[region]) glob -= own;
        if (glob<0.f) glob=0.f; }
    float po,pl,pg; market_split(own,local,glob,qty,&po,&pl,&pg);
    float deficit = qty-(po+pl+pg); if (deficit<0) deficit=0;
    return unit_price * (po + pl*base + (pg+deficit)*base*2.f);
}
/* #5 — CONSOMMER au marché à 2 étages (mutation) : DÉPLÉTÉ d'abord le stock propre,
 * puis le Centre le plus proche (marché local), puis les autres Centres (marché
 * mondial). Les achats touchent de VRAIS stocks → la production a une destination.
 * Le reliquat introuvable n'est pas inventé (pénurie : on a payé, le marché était vide). */
void intertrade_market_consume(WorldEconomy *e, int region, int good, float qty){
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||qty<=0.f) return;
    RegionEconomy *re=&e->region[region];
    int hub = (region<SCPS_MAX_REG)? g_hub_of[region] : -1;
    float t = re->stock[good]<qty?re->stock[good]:qty; re->stock[good]-=t; qty-=t;   /* 1. propre */
    if (qty<=1e-3f) return;
    if (hub>=0 && hub!=region){                                                       /* 2. marché local */
        t = e->region[hub].stock[good]<qty?e->region[hub].stock[good]:qty;
        e->region[hub].stock[good]-=t; qty-=t;
    }
    if (qty<=1e-3f || hub<0) return;
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG && qty>1e-3f; r++){                  /* 3. marché mondial */
        if (!g_centre[r]||r==hub||r==region) continue;
        t = e->region[r].stock[good]<qty?e->region[r].stock[good]:qty;
        e->region[r].stock[good]-=t; qty-=t;
    }
}

/* #5 (action joueur) — l'ACHAT/VENTE DIRECT AU MARCHÉ (l'actionneur que l'UI appelle ;
 * jamais une écriture directe côté viewer). Le joueur, à `region`, pompe `want` unités de
 * `good` depuis un étage : tier 0 = RÉGIONAL (le Centre le plus proche), tier 1 = MONDIAL
 * (le réseau des Centres, exige un accès direct : le pays tient un Centre). Il paie le prix
 * courant × marge (×2 au mondial — la double taxe), DÉPLÉTÉ le marché, crédite SON stock,
 * débite SON trésor. N'achète QUE le disponible ET le finançable. Renvoie les unités
 * obtenues (*spent = l'or débité). La carte des hubs est rafraîchie si besoin. */
#define MARKET_MIN_PRICE 0.2f
long intertrade_market_buy(WorldEconomy *e, int region, int good, long want, int tier, long *spent){
    if (spent) *spent=0;
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||want<=0) return 0;
    if (g_hub_dirty){ hub_map_build(e); global_cache_refresh(e); g_hub_dirty=false; }   /* carte/cache sûrs hors tick */
    RegionEconomy *re=&e->region[region];
    int hub=g_hub_of[region];
    if (hub<0) return 0;                                          /* aucun marché atteignable */
    if (tier<=0 && hub==region) return 0;                        /* V2 : pas de marché LOCAL avec SOI-MÊME (anti-exploit) */
    float base=re->import_margin; if (base<1.f) base=1.f;
    float price=re->price[good]; if (price<MARKET_MIN_PRICE) price=MARKET_MIN_PRICE;
    float avail, mult;
    if (tier<=0){ avail=e->region[hub].stock[good]; mult=base; }              /* régional */
    else { if (!intertrade_country_has_centre(e, re->owner)                   /* M3 : accès mondial = */
               && !(cid_ok(re->owner)&&g_global_access[re->owner])) return 0; /* Centre propre OU pacte */
           avail=g_global_cache[good];                                        /* mondial : le réseau… */
           if (region<SCPS_MAX_REG && g_centre[region]) avail-=e->region[region].stock[good];  /* V2 : …MOINS son propre stock (sinon sur-tirage) */
           if (avail<0.f) avail=0.f;
           mult=base*2.f; }                                                  /* double taxe */
    if (avail<1.f) return 0;                                      /* rien à acheter : « uniquement s'il est dispo » */
    float up=price*mult; if (up<1e-4f) up=1e-4f;
    long qty=want;
    if (qty>(long)avail) qty=(long)avail;                        /* borné par le disponible */
    long can=(long)(re->treasury/up); if (qty>can) qty=can;      /* borné par le trésor */
    if (qty<=0) return 0;
    float cost=(float)qty*up;
    re->treasury -= cost;                                         /* PUMP du trésor */
    re->stock[good] += (float)qty;                               /* le bien entre au stock */
    if (tier<=0){ e->region[hub].stock[good]-=(float)qty; if(e->region[hub].stock[good]<0.f)e->region[hub].stock[good]=0.f; }
    else {
        long rem=qty;
        for (int r=0;r<e->n_regions && r<SCPS_MAX_REG && rem>0;r++){
            if (!g_centre[r] || r==region) continue;             /* V2 : JAMAIS sa propre région */
            long t=(long)e->region[r].stock[good]; if (t>rem) t=rem;
            e->region[r].stock[good]-=(float)t; rem-=t;
        }
    }
    g_global_cache[good]-=(float)qty; if(g_global_cache[good]<0.f)g_global_cache[good]=0.f;  /* V2.2 : cache à jour (anti sur-tirage intra-tick) */
    if (spent) *spent=(long)(cost+0.5f);
    return qty;
}
/* la VENTE, l'inverse : on lâche `want` du stock propre AU marché (régional/mondial), on
 * encaisse au prix courant (sans marge — on vend au prix, on n'achète pas), le bien
 * rejoint le Centre. Renvoie les unités vendues (*gained = l'or encaissé). */
long intertrade_market_sell(WorldEconomy *e, int region, int good, long want, int tier, long *gained){
    if (gained) *gained=0;
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||want<=0) return 0;
    if (g_hub_dirty){ hub_map_build(e); global_cache_refresh(e); g_hub_dirty=false; }
    RegionEconomy *re=&e->region[region];
    int hub=g_hub_of[region];
    if (hub<0) return 0;
    if (tier<=0 && hub==region) return 0;                        /* V2 : pas de vente LOCALE à SOI-MÊME (anti-exploit) */
    if (tier>0 && !intertrade_country_has_centre(e, re->owner)
        && !(cid_ok(re->owner)&&g_global_access[re->owner])) return 0;   /* M3 : Centre OU pacte */
    int dep=hub;                                                 /* V2 : on ne DÉPOSE jamais dans sa propre région */
    if (dep==region){ dep=-1; for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++) if (g_centre[r] && r!=region){ dep=r; break; } }
    if (dep<0) return 0;                                         /* aucun autre Centre où déposer → refus */
    long qty=want; if (qty>(long)re->stock[good]) qty=(long)re->stock[good];
    if (qty<=0) return 0;
    float price=re->price[good]; if (price<MARKET_MIN_PRICE) price=MARKET_MIN_PRICE;
    float gain=(float)qty*price;
    re->stock[good]-=(float)qty;
    re->treasury+=gain;
    e->region[dep].stock[good]+=(float)qty;                      /* le bien rejoint le marché (un AUTRE Centre) */
    g_global_cache[good]+=(float)qty;                            /* V2.2 : cache à jour */
    if (gained) *gained=(long)(gain+0.5f);
    return qty;
}

void  intertrade_order_embargo(int cid, int target, bool on){
    if (cid_ok(cid) && cid_ok(target) && cid!=target) g_embargo[cid][target]=on;
}
bool  intertrade_embargoed(int cid, int target){
    if (!cid_ok(cid) || !cid_ok(target)) return false;
    return g_embargo[cid][target] || g_embargo[target][cid];
}
float intertrade_import_vol (int cid,int g){ return (cid_ok(cid)&&g>0&&g<RES_COUNT)?g_imp [cid][g]:0.f; }
float intertrade_export_vol (int cid,int g){ return (cid_ok(cid)&&g>0&&g<RES_COUNT)?g_expt[cid][g]:0.f; }
int   intertrade_import_from(int cid,int g){ return (cid_ok(cid)&&g>0&&g<RES_COUNT)?g_imp_from[cid][g]:-1; }
int   intertrade_export_to  (int cid,int g){ return (cid_ok(cid)&&g>0&&g<RES_COUNT)?g_expt_to [cid][g]:-1; }
float intertrade_export_gold(int cid){ return cid_ok(cid)?g_gold[cid]:0.f; }
float intertrade_pair_value (int cid,int other){
    return (cid_ok(cid)&&cid_ok(other))? g_pair[cid][other] : 0.f;
}

void intertrade_save(FILE *f){ fwrite(g_embargo,sizeof g_embargo,1,f); fwrite(g_centre,sizeof g_centre,1,f); }
bool intertrade_load(FILE *f){ g_hub_dirty=true;   /* #5 : Centres rechargés → carte des hubs à refaire */
                               return fread(g_embargo,sizeof g_embargo,1,f)==1
                                   && fread(g_centre,sizeof g_centre,1,f)==1; }

static bool pair_at_peace(const DiploState *dp, int ca, int cb){
    return !dp || diplo_status(dp, ca, cb) != DIPLO_WAR;
}

void intertrade_tick(WorldEconomy *e, const RouteNetwork *rn, const DiploState *dp){
    g_last_value = 0.f;
    flows_clear();
    if (!e || !rn) return;
    refresh_centres(e);   /* M2 : g_centre suit le BÂTI (Centres conquis/bâtis/relocalisés) */
    /* M3 — ACCÈS AU MARCHÉ GLOBAL : on tient un Centre, OU on a un PACTE commercial avec un
     * pays qui en tient un (réciproque). Précalcul O(R)+O(pays²) (les readers lisent en O(1)). */
    { bool hc[SCPS_MAX_COUNTRY]; for (int c=0;c<SCPS_MAX_COUNTRY;c++) hc[c]=false;
      for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++)
          if (g_centre[r]){ int o=e->region[r].owner; if(cid_ok(o)) hc[o]=true; }
      for (int c=0;c<SCPS_MAX_COUNTRY;c++) g_global_access[c]=hc[c];
      if (dp) for (int a=0;a<SCPS_MAX_COUNTRY;a++) if (!g_global_access[a])
          for (int b=0;b<SCPS_MAX_COUNTRY;b++)
              if (b!=a && hc[b] && diplo_trade_pact(dp,a,b)){ g_global_access[a]=true; break; }
    }

    /* #5 — LE MARCHÉ À 2 ÉTAGES (étage local). Chaque région se branche au marché de la
     * cité-état la PLUS PROCHE (hub_map_build), à RENDEMENT DÉGRESSIF : la marge d'achat
     * = base × (1 + MARKET_DIST_FALLOFF·distance). Base : via SON hub/Comptoir → ×1.3 ;
     * via le hub d'un TIERS (la cité-état hôte) → ×1.8 + un péage versé à ce hub ;
     * aucun hub atteignable → enclavé ×2.0 ; monde SANS aucun Centre → 1:1. La couche
     * commerce ÉCRIT re->import_margin ; agency la LIT (+ la double taxe locale/mondiale
     * par bien via intertrade_buy_margin). Le 2e étage (mondial) est dans buy_margin. */
    if (g_hub_dirty){ hub_map_build(e); g_hub_dirty=false; }   /* BFS seulement à un changement de Centres */
    global_cache_refresh(e);                                   /* la profondeur du marché, elle, bouge chaque tick */
    {
        int any_centre=0;
        for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++) if (g_centre[r]){ any_centre=1; break; }
        float m_own=tune_f("IMPORT_MARGIN_OWN",1.3f), m_third=tune_f("IMPORT_MARGIN_THIRD",1.8f),
              m_none=tune_f("IMPORT_MARGIN_NONE",2.0f), dfall=tune_f("MARKET_DIST_FALLOFF",0.12f);
        for (int r=0;r<e->n_regions;r++){
            RegionEconomy *re=&e->region[r];
            re->import_margin=1.f; re->import_toll_region=-1;
            int o=re->owner;
            if (!any_centre || o<0) continue;
            int hub = (r<SCPS_MAX_REG)? g_hub_of[r] : -1;
            if (hub<0){ re->import_margin=m_none; continue; }     /* aucun marché atteignable : enclavé */
            bool own = (e->region[hub].owner==o) || (re->edi_built & (1u<<EDI_COMPTOIR));
            int d = (r<SCPS_MAX_REG)? g_hub_dist[r] : 0; if (d>8) d=8;   /* rendement dégressif, plafonné */
            re->import_margin = (own?m_own:m_third) * (1.f + dfall*(float)d);
            if (!own) re->import_toll_region=(int16_t)hub;        /* péage à la cité-état hôte (la plus proche) */
        }
    }
    for (int i=0;i<rn->n;i++){
        const TradeRoute *rt=&rn->route[i];
        if (!rt->open) continue;
        int ra=rt->ra, rb=rt->rb;
        if (ra<0||rb<0||ra>=e->n_regions||rb>=e->n_regions) continue;
        int ca=e->region[ra].owner, cb=e->region[rb].owner;
        if (ca<0||cb<0||ca==cb) continue;            /* intra-pays : déjà couvert par scps_trade */
        /* M1 — LE COMMERCE RÉGIONAL (empire↔empire, le long des routes) N'EXIGE PAS de
         * Centre : il coule à la paix, hors embargo, AMÉLIORÉ par Marché/Comptoir (conn_mult
         * ci-dessous). Le gate « il faut un Centre » est la signature de la strate GLOBALE
         * (le réseau des cités-états), géré ailleurs — pas une condition du régional. */
        bool pact=diplo_trade_pact(dp,ca,cb);        /* cité marchande : route GARANTIE */
        if (!pact && !pair_at_peace(dp,ca,cb)) continue;      /* EMBARGO : guerre commerciale */
        if (!pact && intertrade_embargoed(ca,cb)) continue;   /* EMBARGO DÉCRÉTÉ (joueur/IA) */
        RegionEconomy *A=&e->region[ra], *B=&e->region[rb];
        float cap=rt->capacity>0.f?rt->capacity:1.f;
        /* E2 §10 — le COMPTOIR branche la province au réseau : à chaque bout
         * CONNECTÉ (Centre commercial, ou Comptoir bâti), la marge de transport
         * tombe d'un tiers. Sans Comptoir ni Centre : marge IT_TRANSPORT pleine —
         * la province commerce mal (lecture d'édifice, jamais un bonus plat). */
        float conn_mult = 1.f;
        if ((ra<SCPS_MAX_REG && g_centre[ra]) || (e->region[ra].edi_built & (1u<<EDI_COMPTOIR))) conn_mult *= 0.67f;
        if ((rb<SCPS_MAX_REG && g_centre[rb]) || (e->region[rb].edi_built & (1u<<EDI_COMPTOIR))) conn_mult *= 0.67f;
        for (int g=1; g<RES_COUNT; g++){
            float pa=A->price[g], pb=B->price[g];
            bool a_to_b=(pa<pb);                      /* le bien va du bon marché vers le cher */
            float cost=it_transport_frac(rt,a_to_b)*((pa+pb)*0.5f)*conn_mult;
            if (fabsf(pa-pb) <= cost) continue;       /* marge trop mince POUR CE SENS → rien ne bouge */
            RegionEconomy *src=a_to_b?A:B, *dst=a_to_b?B:A;     /* on achète au moins cher */
            float vol=fminf(cap*it_volume_mult(rt,a_to_b), src->stock[g]*IT_EXPORT_FRAC);
            if (vol<=0.001f) continue;
            { bool down=it_is_downstream(rt,a_to_b);  /* télémétrie : le tri par sens ÉMERGE */
              if (rt->maritime||rt->fluvial){
                  if (down) g_vol_down+=vol; else g_vol_up+=vol;
                  if (it_is_bulk(g))     { if (down) g_bulk_down+=vol; else g_bulk_up+=vol; }
                  if (it_is_precious(g)) { if (down) g_prec_down+=vol; else { g_prec_up+=vol; g_nprec_up++; } }
              } }
            src->stock[g]-=vol; dst->stock[g]+=vol;             /* le bien remonte la pente de prix */
            float value=vol*dst->price[g];
            src->treasury += value*IT_MARGIN_TO_GOLD;           /* l'exportateur encaisse l'or */
            if (src->owner>=0) econ_flux_add(src->owner, FX_EXPORT, value*IT_MARGIN_TO_GOLD);  /* I0 */
            g_last_value += value;
            /* la valeur PASSE par les Centres des deux couronnes (moitié chacun) —
             * la part des cités-états dans le commerce mondial se lit là. */
            { int cca=intertrade_country_centre(e,ca), ccb=intertrade_country_centre(e,cb);
              if (cca>=0&&cca<SCPS_MAX_REG) g_centre_val[cca]+=value*0.5f;
              if (ccb>=0&&ccb<SCPS_MAX_REG) g_centre_val[ccb]+=value*0.5f; }
            /* comptabilité des flux (sidebar) : qui exporte/importe quoi, vers/depuis qui */
            { int cs=(src==A)?ca:cb, cd=(dst==A)?ca:cb;
              if (cid_ok(cs)&&cid_ok(cd)){
                  g_expt[cs][g]+=vol; g_imp[cd][g]+=vol; g_gold[cs]+=value*IT_MARGIN_TO_GOLD;
                  g_pair[cs][cd]+=value; g_pair[cd][cs]+=value;
                  if (vol>g_expt_best[cs][g]){ g_expt_best[cs][g]=vol; g_expt_to [cs][g]=(int16_t)cd; }
                  if (vol>g_imp_best [cd][g]){ g_imp_best [cd][g]=vol; g_imp_from[cd][g]=(int16_t)cs; }
              } }
            float mid=(pa+pb)*0.5f;                              /* les prix convergent partiellement */
            A->price[g]+=(mid-A->price[g])*IT_PRICE_CONV;
            B->price[g]+=(mid-B->price[g])*IT_PRICE_CONV;
        }
    }
    /* M4 — L'ARBITRAGE DES CITÉS-ÉTATS (leur MOTEUR : elles vivent du commerce, pas de
     * l'impôt). Chaque Centre IMPORTE un volume BORNÉ des biens où son marché LOCAL paie
     * plus cher que le Centre le moins cher du réseau ; il encaisse une PART du spread
     * (l'exportateur source encaisse aussi son or), DÉPLÉTÉ la source, stocke son local ;
     * les prix CONVERGENT. Volume/tick CAPÉ + spread MINIMAL + on ne vide jamais la source
     * → pas de runaway spéculatif ; σ prix reste borné. */
    { float vcap=tune_f("ARB_VOL_CAP",3.f), minsp=tune_f("ARB_MIN_SPREAD",0.20f), cap=tune_f("ARB_CAPTURE",0.35f);
      int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
      for (int h=0;h<n;h++){ if(!g_centre[h]) continue; RegionEconomy *H=&e->region[h];
        for (int g=1; g<RES_COUNT; g++){
            float lp=H->price[g]; int src=-1; float sp=lp; int ho=H->owner;
            for (int c=0;c<n;c++){ if(c==h||!g_centre[c]) continue;
                int co=e->region[c].owner;
                if (!cid_ok(co)||!cid_ok(ho)||co==ho) continue;          /* INTER-pays seulement (l'intra = scps_trade) */
                if (!pair_at_peace(dp,ho,co) || intertrade_embargoed(ho,co)) continue;  /* ni guerre ni embargo */
                if (e->region[c].price[g]<sp && e->region[c].stock[g]>1.f){ sp=e->region[c].price[g]; src=c; } }
            if (src<0 || lp<=sp*(1.f+minsp)) continue;            /* spread trop mince → rien */
            float vol=vcap; if (vol>e->region[src].stock[g]*0.20f) vol=e->region[src].stock[g]*0.20f;  /* ne vide pas la source */
            if (vol<0.5f) continue;
            e->region[src].stock[g]-=vol; H->stock[g]+=vol;      /* le Centre stocke son marché local */
            e->region[src].treasury += vol*sp*IT_MARGIN_TO_GOLD; /* l'exportateur source encaisse l'or */
            float profit=vol*(lp-sp)*cap;                        /* le CE encaisse une PART BORNÉE du spread */
            H->treasury += profit; g_centre_val[h]+=profit;
            if (H->owner>=0) econ_flux_add(H->owner, FX_EXPORT, profit);
            float mid=(lp+sp)*0.5f;                               /* les prix CONVERGENT (local baisse, source monte) */
            H->price[g]            += (mid-lp)*IT_PRICE_CONV;
            e->region[src].price[g]+= (mid-sp)*IT_PRICE_CONV;
        }
      }
    }
}

float intertrade_imports_value(const WorldEconomy *e){ (void)e; return g_last_value; }
float intertrade_centre_value(int region){
    return (region>=0&&region<SCPS_MAX_REG)? g_centre_val[region] : 0.f;
}
/* COMMERCE ASYMÉTRIQUE — lecteurs (chronicle §6) : volumes par sens et leur
 * composition. L'aval doit charrier le VRAC ; la remontée, le PRÉCIEUX. */
void intertrade_asym_stats(float *vdown, float *vup,
                           float *bulk_down, float *bulk_up,
                           float *prec_down, float *prec_up){
    if (vdown)     *vdown=g_vol_down;
    if (vup)       *vup=g_vol_up;
    if (bulk_down) *bulk_down=g_bulk_down;
    if (bulk_up)   *bulk_up=g_bulk_up;
    if (prec_down) *prec_down=g_prec_down;
    if (prec_up)   *prec_up=g_prec_up;
}
int intertrade_precious_upstream_events(void){ return g_nprec_up; }

int intertrade_active_routes(const WorldEconomy *e, const RouteNetwork *rn,
                             const DiploState *dp, int cid){
    if (!e||!rn) return 0;
    int n=0;
    for (int i=0;i<rn->n;i++){
        const TradeRoute *rt=&rn->route[i];
        if (!rt->open) continue;
        int ra=rt->ra, rb=rt->rb;
        if (ra<0||rb<0||ra>=e->n_regions||rb>=e->n_regions) continue;
        int ca=e->region[ra].owner, cb=e->region[rb].owner;
        if (ca==cb || (ca!=cid && cb!=cid)) continue;
        if (!diplo_trade_pact(dp,ca,cb)){
            if (!pair_at_peace(dp,ca,cb)) continue;
            if (intertrade_embargoed(ca,cb)) continue;   /* l'embargo décrété ferme aussi la route */
        }
        n++;
    }
    return n;
}
