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
#include "scps_provlog.h"  /* le JOURNAL diplomatique (embargo décrété/levé, display) */
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ---- calibrage --------------------------------------------------------- */
#define IT_TRANSPORT   0.06f   /* coût de transport longue distance — la BASE de la fonction directionnelle */
#define IT_FLUV_DOWN   0.35f   /* descente : quasi gratuit (on flotte le bois, on descend le grain) */
#define IT_FLUV_UP     2.50f   /* remontée : le halage — cher, JAMAIS infini (plancher, pas mur)   */
#define IT_VOL_FLOOR   0.40f   /* la capacité à contre-courant : divisée, jamais nulle             */
#define IT_EXPORT_FRAC 0.25f   /* part du surplus que l'exportateur lâche par tick */
/* IT_PRICE_CONV RETIRÉ (LOT 2, réparations 2026-07-06) : nudge de convergence de prix
 * mort sous le régime de PRIX NATIONAL (2026-06-28) — il écrivait region[].price, la vue
 * TRANSIENTE rebâtie chaque mois par econ_aggregate_regions depuis prov[].price (jamais
 * touché ici), puis re-projetée par la formule nationale (pure fonction demand_nat/pool/
 * supply_nat, sans mémoire du nudge). Voir CLAUDE.md « MISSION ÉCO — restes assumés ». */
#define IT_MARGIN_TO_GOLD 0.50f/* part de la valeur transportée qui devient de l'OR pour l'exportateur */
#define IT_CHOKE_TOLL  0.12f   /* WG : part de la valeur transportée que SKIME le tenant d'un détroit (transfert exportateur→tenant, modulée par l'étroitesse du goulet) */

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
static float  g_commerce_budget[SCPS_MAX_COUNTRY];  /* §5 PUISSANCE COMMERCIALE : pool MENSUEL de volume échangeable (fixé au roulement de mois, scalé pop marchande × chaîne commerciale) */
static float  g_commerce_spent [SCPS_MAX_COUNTRY];  /* volume déjà tiré du marché ce mois-ci (reset mensuel) */
static long   g_comm_capped=0;   /* §5 télémétrie (non sérialisée, RAZ/sim) : nb d'achats BORNÉS par le pool — la preuve que ça mord */
static double g_comm_drawn =0.0; /* §5 télémétrie : volume total tiré du marché (drainé du pool) */
static bool   g_commerce_active=false;  /* §5 : le cap n'agit QUE dans une vraie sim (sim_day/load l'active) → pool non semé hors sim ⇒ bancs INCHANGÉS */
static float  g_pair[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
static bool   g_embargo[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];   /* [qui décrète][contre qui] */
static bool   g_centre[SCPS_MAX_REG];   /* P3.20 : cette région EST un Centre commercial (hub) */
static float  g_centre_val[SCPS_MAX_REG]; /* valeur du commerce passée par chaque Centre (dernier tick) */
/* WG (worldgen-graphe) — LE PÉAGE DE DÉTROIT (dernier tick) : ce que chaque pays a
 * encaissé en TENANT un détroit que franchit une route maritime, + le nombre de
 * routes ainsi taxées. Le tenant prélève une PART de la valeur transportée (un
 * transfert exportateur→tenant : l'importateur ne paie pas plus, le verrou skime). */
static float  g_choke_toll[SCPS_MAX_COUNTRY];
static int    g_n_choke_routes=0;     /* routes maritimes franchissant un détroit tenu par un TIERS */
static float  g_choke_toll_total=0.f; /* somme des péages encaissés (dernier tick) */
/* CUMUL sur la sim (RAZ uniquement par intertrade_reset, pas chaque tick) : le total
 * encaissé par chaque tenant sur toute la partie — la preuve chiffrée (la valeur d'un
 * tick converge vers ~0 quand les prix s'égalisent ; le cumul, lui, RESTE). */
static double g_choke_toll_cumul[SCPS_MAX_COUNTRY];
static double g_choke_toll_cumul_total=0.0;
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
/* ESCLAVAGE — LE MARCHÉ DES CENTRES : le pool mondial d'âmes, PAR héritage (sérialisé,
 * cf. intertrade_save/load). Déclaré ICI (avant intertrade_reset qui le RAZ). */
static float g_slave_pool[HERITAGE_COUNT];

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
    memset(g_choke_toll,0,sizeof g_choke_toll); g_n_choke_routes=0; g_choke_toll_total=0.f;  /* WG */
    memset(g_choke_toll_cumul,0,sizeof g_choke_toll_cumul); g_choke_toll_cumul_total=0.0;    /* WG : cumul RAZ par sim */
    memset(g_commerce_budget,0,sizeof g_commerce_budget); memset(g_commerce_spent,0,sizeof g_commerce_spent);  /* §5 : pool commercial RAZ */
    g_comm_capped=0; g_comm_drawn=0.0; g_commerce_active=false;   /* §5 : télémétrie RAZ + cap INACTIF tant que sim_day n'a pas fixé le pool */
    g_last_value=0.f;
    memset(g_slave_pool,0,sizeof g_slave_pool);   /* ESCLAVAGE : pas de FUITE du pool entre parties d'une même session */
}
static void flows_clear(void){
    g_vol_down=g_vol_up=0.f; g_bulk_down=g_bulk_up=0.f; g_prec_down=g_prec_up=0.f; g_nprec_up=0;
    memset(g_centre_val,0,sizeof g_centre_val);
    memset(g_imp,0,sizeof g_imp);   memset(g_expt,0,sizeof g_expt);
    memset(g_imp_best,0,sizeof g_imp_best); memset(g_expt_best,0,sizeof g_expt_best);
    memset(g_imp_from,-1,sizeof g_imp_from); memset(g_expt_to,-1,sizeof g_expt_to);
    memset(g_gold,0,sizeof g_gold); memset(g_pair,0,sizeof g_pair);
    memset(g_choke_toll,0,sizeof g_choke_toll); g_n_choke_routes=0; g_choke_toll_total=0.f;  /* WG : péages du tick */
}
static inline bool cid_ok(int c){ return c>=0 && c<SCPS_MAX_COUNTRY; }

/* ═══ §5 PUISSANCE COMMERCIALE — le pool MENSUEL de volume échangeable au marché ═══════════════════
 * Fixé au ROULEMENT de mois (intertrade_commerce_reset, appelé par sim_day) : le budget = la pop
 * MARCHANDE × la chaîne commerciale (econ_country_commerce) ; les achats (market_consume/_buy) le
 * DRAINENT (commerce_draw) ; à sec, plus d'import ce mois — l'équilibrage qui empêche de rafler tout
 * le stock d'un coup. Budget ET dépensé sont SÉRIALISÉS (état intra-mois → savetest byte-identique). */
void  intertrade_commerce_reset(const WorldEconomy *e){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        g_commerce_budget[c] = e ? econ_country_commerce(e, c) : 0.f;
        g_commerce_spent [c] = 0.f;
    }
    g_commerce_active = true;   /* le cap AGIT désormais (vraie sim) */
}
float intertrade_commerce_pool     (int cid){ return cid_ok(cid)? g_commerce_budget[cid] : 0.f; }
float intertrade_commerce_remaining(int cid){
    if (!cid_ok(cid)) return 0.f;
    float rem = g_commerce_budget[cid] - g_commerce_spent[cid];
    return rem>0.f ? rem : 0.f;
}
static void commerce_draw(int cid, float amount){ if (cid_ok(cid) && amount>0.f){ g_commerce_spent[cid] += amount; g_comm_drawn += (double)amount; } }
void intertrade_commerce_diag(long *capped, double *drawn){ if(capped)*capped=g_comm_capped; if(drawn)*drawn=g_comm_drawn; }  /* §5 : la télémétrie de chronique */

/* RE-KEY PROVINCE : treasury est PROVINCE-owned (Σ-agrégé dans region[].treasury à
 * chaque econ_tick/econ_aggregate_regions) — écrire region[r].treasury directement
 * serait effacé au prochain tick (même piège déjà réglé côté scps_credit.c
 * credit_spend/credit_year_tick). Toutes les écritures/lectures-fraîches de trésor
 * d'intertrade sont routées vers la province REPRÉSENTATIVE de la région (capitale,
 * sinon la plus peuplée — cf. econ_region_rep_province). Renvoie NULL si la région
 * n'a pas (encore) de province représentative — appelant doit alors no-op. */
static inline float *it_treasury(WorldEconomy *e, int region){
    int pid=econ_region_rep_province(e, region);
    if (pid<0 || pid>=e->n_prov) return NULL;
    return &e->prov[pid].treasury;
}

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
 * hubs « dirty » QUE si un Centre apparaît/disparaît (perf : pas de BFS chaque tick).
 * LECTURE seule ici (region[].edi_built = l'agrégat OR des provinces membres, maintenu
 * par econ_aggregate_regions) — jamais d'écriture, cf. les deux poseurs ci-dessous. */
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
     * pas un flag → g_centre en dérive ; un EMPIRE marchand côtier peut AUSSI en bâtir un.
     * RE-KEY PROVINCE (T4) : edi_built est PROVINCE-owned — region[].edi_built est un
     * AGRÉGAT (OR des membres) reconstruit CHAQUE TICK par econ_aggregate_regions ; y
     * écrire directement est effacé au 1er tick (même piège qu'apply_action/scps_agency.c,
     * déjà réglé là). On pose sur la province REPRÉSENTATIVE (la vérité) + un miroir
     * immédiat sur region[best] pour que refresh_centres() voie déjà l'état CETTE frame
     * (avant le 1er econ_tick), comme econ_build_manufacture. */
    if (w) for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role!=POLITY_CITY_STATE) continue;
        int best=-1; float bestc=-1.f;
        for (int r=0;r<n;r++){
            if (e->region[r].owner!=c) continue;
            float fc=region_flow_score(e,r); if(fc>bestc){bestc=fc;best=r;}
        }
        if (best<0) continue;
        int pid=econ_region_rep_province(e,best);
        if (pid>=0 && pid<e->n_prov) e->prov[pid].edi_built |= (1u<<EDI_TRADE_CENTER);
        e->region[best].edi_built |= (1u<<EDI_TRADE_CENTER);   /* miroir immédiat (informatif) */
    }
    refresh_centres(e);
}

/* F-arc — CHAQUE CITÉ-ÉTAT NAÎT ARMURIER. Sur sa MEILLEURE région (le Centre, même critère que
 * _seed_centres → la fabrique et le hub coïncident), on POSE une manufacture d'armes tirée AU SORT
 * (déterministe : graine × index), et on GARANTIT son intrant brut (raw_cap) pour qu'elle morde.
 * Les cités-états deviennent les ARMURIERS du monde : les empires y pompent leurs armes spécialisées
 * (econ_arms_take → intertrade_market_pull). À semer APRÈS intertrade_seed_centres. */
void intertrade_seed_citystate_arms(const World *w, WorldEconomy *e){
    if (!w||!e) return;
    static const BuildingType ARMS[6]={ BLD_ARMORY_HEAVY, BLD_BOWYER, BLD_MAGE_WORKSHOP,
                                        BLD_CELESTIAL_FORGE, BLD_ALAMBIC, BLD_ARQUEBUS };
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role!=POLITY_CITY_STATE) continue;
        int best=-1; float bestc=-1.f;
        for (int r=0;r<n;r++){ if (e->region[r].owner!=c) continue;
            float fc=region_flow_score(e,r); if(fc>bestc){bestc=fc;best=r;} }
        if (best<0) continue;
        uint32_t h = w->seed ^ (uint32_t)c*2654435761u;   /* mélange déterministe seed×index */
        h ^= h>>13; h *= 0x5bd1e995u; h ^= h>>15;
        BuildingType b = ARMS[h % 6u];
        econ_build_manufacture(e, best, b);
        /* L'ARMURIER À POUDRE bâtit TOUTE la chaîne (3 manufactures, demandée) : charbonnière
         * (bois→charbon) → poudrière (salpêtre+charbon→poudre) → arquebuserie (fer+poudre→feu).
         * On NE FORCE PAS les ressources (discipline : on ne lit que la géographie) — la chaîne
         * MORD là où le salpêtre/fer/bois NATURELS la nourrissent (la genèse répartit ces gisements). */
        if (b==BLD_ARQUEBUS){ econ_build_manufacture(e, best, BLD_POWDERMILL);
                              econ_build_manufacture(e, best, BLD_CHARCOAL); }
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
 * M2 — on déplace le BÂTIMENT (edi_built) ; g_centre en re-dérive.
 * RE-KEY PROVINCE (T4) : même piège que _seed_centres — on écrit sur les provinces
 * représentatives (la vérité), avec un miroir region[] immédiat pour cette frame. */
bool intertrade_relocate_centre(WorldEconomy *e, int from, int to){
    if (!e||from<0||from>=e->n_regions||to<0||to>=e->n_regions) return false;
    if (!((e->region[from].edi_built>>EDI_TRADE_CENTER)&1u) || ((e->region[to].edi_built>>EDI_TRADE_CENTER)&1u)) return false;
    int pf=econ_region_rep_province(e,from), pt=econ_region_rep_province(e,to);
    if (pf>=0 && pf<e->n_prov) e->prov[pf].edi_built &= ~(1u<<EDI_TRADE_CENTER);
    if (pt>=0 && pt<e->n_prov) e->prov[pt].edi_built |=  (1u<<EDI_TRADE_CENTER);
    e->region[from].edi_built &= ~(1u<<EDI_TRADE_CENTER);   /* miroir immédiat (informatif) */
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
    /* V3 — LA PASSERELLE DE MER, ROBUSTE : on ne dépend plus d'UN Centre côtier. Toute
     * côte DÉJÀ branchée à un marché par terre (un Centre côtier OU un relais terrestre
     * vers un Centre de l'intérieur) est une porte ; la MEILLEURE (la plus proche d'un
     * marché, plus petit g_hub_dist) sert de passerelle aux côtes encore orphelines, au
     * coût d'une traversée. Sans aucune côte branchée → la côte reste enclavée (m_none),
     * mais plus aucun Centre côtier unique ne fait seul la loi sur toutes les mers. */
    int sea_gw=-1, sea_gw_dist=0;
    for (int r=0;r<n;r++)
        if (g_hub_of[r]>=0 && e->region[r].coastal && (sea_gw<0 || g_hub_dist[r]<sea_gw_dist)){
            sea_gw=r; sea_gw_dist=g_hub_dist[r];
        }
    if (sea_gw>=0){ int16_t hub=g_hub_of[sea_gw];
        for (int r=0;r<n;r++)
            if (g_hub_of[r]<0 && e->region[r].coastal){
                g_hub_of[r]=hub; g_hub_dist[r]=(int16_t)(sea_gw_dist+IT_SEA_HOPS);
            }
    }
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
/* FUZZTEST HOOK (défaut #5, viewer.c --fuzztest) — g_hub_of est un static de MODULE, pas
 * un champ Sim-visible (contrairement à AgencyState/RevoltState) : c'est la SEULE façon
 * de le forger hors-borne pour exercer intertrade_save_sane sur un état FORGÉ, du même
 * motif que les autres cas FZ(...) (poke direct → save_sane → restauration). */
void  intertrade_debug_set_hub_of(int region, int value){
    if (region>=0 && region<SCPS_MAX_REG) g_hub_of[region] = (int16_t)value;
}
float intertrade_global_stock(const WorldEconomy *e, int good){
    (void)e; return (good>RES_NONE && good<RES_COUNT)? g_global_cache[good] : 0.f;   /* cache 1×/tick */
}
/* F4 — TOCTOU chantier × pool commercial §5 : cette fonction est le GATE D'ADMISSION lu par
 * agency_build_acct (qty requise > avail ⇒ refus), mais elle ignorait le pool MENSUEL de l'empire
 * (intertrade_commerce_remaining) que intertrade_market_consume APPLIQUE en aval (silencieusement
 * tronqué) : le devis d'or payait la quantité PLEINE mais la matière livrée pouvait être PARTIELLE
 * (le chantier passait le gate, payait plein tarif, recevait moins). Fix : la composante IMPORTÉE
 * (celle qui vient des Centres HORS-EMPIRE, `imp` ci-dessous — le stock EMPIRE `emp` reste GRATUIT
 * et illimité, comme _consume/_buy_cost le traitent déjà) est bornée par ce qu'il RESTE à acheter
 * ce mois-ci — le gate reflète alors EXACTEMENT ce qui sera réellement consommable.
 * `imp_capped_out` (option, NULL ⇒ ignoré) = la part de l'import qui a été COUPÉE par le pool (0 si
 * non-capé/non-applicable) — sert à agency_build_acct pour distinguer un refus « le pool a mordu »
 * (g_edi_nocap) d'un refus « la matière est vraiment absente » (g_edi_nomat). */
float intertrade_market_avail_ex(const WorldEconomy *e, int region, int good, float *imp_capped_out){
    if (imp_capped_out) *imp_capped_out=0.f;
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT) return 0.f;
    int owner=e->region[region].owner, hub=(region<SCPS_MAX_REG)?g_hub_of[region]:-1;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    /* EMPIRE : toute sa matière est FONGIBLE pour SON chantier (réseau marchés/ports) — GRATUIT,
     * hors du pool commercial (le pool ne borne QUE l'import de Centres étrangers, cf. _consume). */
    float emp=0.f, owned_centre=0.f;
    if (owner>=0){ for (int r=0;r<n;r++) if (e->region[r].owner==owner){
            emp+=e->region[r].stock[good];
            if (g_centre[r]) owned_centre+=e->region[r].stock[good]; } }
    else emp=e->region[region].stock[good];                      /* hors empire : la seule région */
    /* IMPORT : les Centres atteignables NON possédés (cache mondial moins ses propres Centres). */
    float imp=(hub>=0)? g_global_cache[good]-owned_centre : 0.f;
    if (imp<0.f) imp=0.f;
    /* §5 : borne par le pool MENSUEL restant — le MÊME frein que intertrade_market_consume applique
     * en aval (g_commerce_active && cid_ok) → le gate ne promet plus ce que la conso tronquera. */
    if (g_commerce_active && cid_ok(owner)){
        float rem=intertrade_commerce_remaining(owner);
        if (imp>rem){ if (imp_capped_out) *imp_capped_out = imp-rem; imp=rem; }
    }
    return emp+imp;
}
/* Disponibilité d'un bien au marché ATTEIGNABLE depuis `region` : stock propre + Centre le plus
 * proche + réseau mondial des Centres (cache 1×/tick), BORNÉ au pool commercial §5 restant (F4).
 * Lu par le gate de matière (agency) : qty requise > avail ⇒ chantier REFUSÉ. Même découpe
 * own/local/glob que intertrade_buy_cost. Enveloppe fine de _ex pour les appelants existants
 * (bancs compris) qui n'ont pas besoin de savoir SI le pool a mordu. */
float intertrade_market_avail(const WorldEconomy *e, int region, int good){
    return intertrade_market_avail_ex(e, region, good, NULL);
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
/* #5 — LE COÛT DU PUMP (devis, sans mutation), EMPIRE-AWARE (comme _avail/_consume).
 * Sourcer `qty` de `good` dans `region` : la matière de SON empire est GRATUITE pour SON
 * chantier (réseau marchés/ports, marge 0) ; l'or ne paie QUE le déficit importé des Centres
 * ÉTRANGERS — le plus proche (×marge de base, distance incluse) → le reste du réseau étranger
 * (×marge×2 : la double taxe). `unit_price` = le prix de marché local du bien (lu par l'appelant).
 * `import_base_out` (option, NULL ⇒ ignoré) = le NU (marge 1) de la part IMPORTÉE seule — la base
 * du PÉAGE : (devis − import_base) = la marge de transport routée à la cité-état hôte. L'empire
 * étant GRATUIT, le nu de bâti n'est PAS la quantité totale mais la seule part importée. */
float intertrade_buy_cost(const WorldEconomy *e, int region, int good, float qty, float unit_price, float *import_base_out){
    if (import_base_out) *import_base_out=0.f;
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||qty<=0.f) return 0.f;
    const RegionEconomy *re=&e->region[region];
    float base = re->import_margin; if (base<1.f) base=1.f;
    int owner=re->owner, hub=(region<SCPS_MAX_REG)?g_hub_of[region]:-1;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    /* EMPIRE : tout son stock est GRATUIT pour SON chantier (réseau marchés/ports) — comme _consume/_avail. */
    float emp=0.f, owned_centre=0.f;
    if (owner>=0){ for (int r=0;r<n;r++) if (e->region[r].owner==owner){
            emp+=e->region[r].stock[good];
            if (g_centre[r]) owned_centre+=e->region[r].stock[good]; } }
    else emp=re->stock[good];                                       /* hors empire : la seule région */
    /* IMPORT (or) : Centres ÉTRANGERS seuls. Le plus proche (le hub s'il n'est pas à nous) à `base` ;
     * le reste du réseau étranger (cache mondial − nos Centres − ce hub) à `base×2`. Pool empire = 0. */
    float fnear = (hub>=0 && hub!=region && e->region[hub].owner!=owner)? e->region[hub].stock[good] : 0.f;
    float fdist = g_global_cache[good] - owned_centre - fnear; if(fdist<0.f) fdist=0.f;
    float p_emp,p_near,p_dist; market_split(emp, fnear, fdist, qty, &p_emp,&p_near,&p_dist);
    (void)p_emp;                                                    /* l'empire est GRATUIT (marge 0) */
    if (import_base_out) *import_base_out = unit_price * (p_near + p_dist);   /* NU de l'import (marge 1) → base du péage */
    return unit_price * (p_near*base + p_dist*base*2.f);
}
/* Ponctionne le stock d'une région ; si c'est un Centre, décrémente AUSSI le cache mondial
 * (g_global_cache = Σ stocks des Centres) → un gate qui le lit n'est plus berné en cours d'an.
 * Renvoie la quantité prélevée. */
static float centre_take(WorldEconomy *e, int r, int good, float want){
    if (want<=0.f) return 0.f;
    /* RE-KEY : débit PERSISTANT (provinces) — écrire la seule vue region[] serait
     * effacé à la clôture (la matière « consommée » réapparaissait chaque mois). */
    float t = -econ_region_stock_add(e, r, good, -want);
    if (r<SCPS_MAX_REG && g_centre[r]){
        g_global_cache[good]-=t; if(g_global_cache[good]<0.f) g_global_cache[good]=0.f;
    }
    return t;
}
/* Pompe `want` de `good` dans les AUTRES régions de l'empire `owner` (≠ skip, déjà traitée) : le
 * réseau de marchés/ports rend la matière de l'empire FONGIBLE pour SON chantier — les ports
 * ABSORBENT le flux terrestre. Renvoie le prélevé ; décrémente le cache pour les régions-Centre. */
static float empire_take(WorldEconomy *e, int owner, int skip, int good, float want){
    if (owner<0 || want<=0.f) return 0.f;
    float got=0.f;
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG && want>1e-3f; r++){
        if (r==skip || e->region[r].owner!=owner) continue;
        float t=centre_take(e, r, good, want); got+=t; want-=t;
    }
    return got;
}
/* #5 — CONSOMMER pour un CHANTIER (mutation) : région de chantier → RESTE de l'empire (son réseau de
 * marchés/ports absorbe le flux terrestre) → Centre local → réseau mondial (import). Cache cohérent. */
void intertrade_market_consume(WorldEconomy *e, int region, int good, float qty, float unit_price){
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||qty<=0.f) return;
    int hub = (region<SCPS_MAX_REG)? g_hub_of[region] : -1;
    qty -= centre_take(e, region, good, qty);                          /* 1. région de chantier — GRATUIT */
    if (qty<=1e-3f) return;
    qty -= empire_take(e, e->region[region].owner, region, good, qty); /* 2. RESTE de l'empire — GRATUIT */
    if (qty<=1e-3f) return;
    /* §5 PUISSANCE COMMERCIALE : l'IMPORT (Centres ÉTRANGERS) est borné au pool MENSUEL de l'empire.
     * Les stages propre+empire ci-dessus sont GRATUITS (production maison) ; seul le MARCHÉ (import)
     * tire le pool → on ne rafle plus tout le stock d'un coup. */
    int cc_owner=e->region[region].owner;
    if (g_commerce_active && cid_ok(cc_owner)){
        float rem=intertrade_commerce_remaining(cc_owner);
        if (qty>rem){ g_comm_capped++; qty=rem; }   /* §5 : le pool a MORDU (import borné) */
        if (qty<=1e-3f) return;
    }
    float cc_imp0=qty;                                                 /* volume d'import autorisé */
    if (hub>=0 && hub!=region){                                        /* 3. Centre local étranger : IMPORT */
        float t = centre_take(e, hub, good, qty);
        float *tr=it_treasury(e,hub); if (tr) *tr += t * unit_price;   /* la SOURCE encaisse le NU (conservation) */
        qty -= t;
    }
    if (hub>=0) for (int r=0;r<e->n_regions && r<SCPS_MAX_REG && qty>1e-3f; r++){   /* 4. réseau mondial étranger : IMPORT */
        if (!g_centre[r]||r==hub||r==region) continue;
        float t = centre_take(e, r, good, qty);
        float *tr=it_treasury(e,r); if (tr) *tr += t * unit_price;      /* idem : la source encaisse le NU */
        qty -= t;
        if (qty<=1e-3f) break;
    }
    if (g_commerce_active && cid_ok(cc_owner)) commerce_draw(cc_owner, cc_imp0-qty);   /* le volume réellement importé draine le pool */
}

/* F-arc — POMPE D'ARMES : même cascade que _market_consume (propre → Centre local de la cité-état
 * la + proche → réseau mondial des Centres) mais RENVOIE le total prélevé. La levée doit savoir ce
 * qu'elle a pu armer ; les cités-états (armuriers du monde) fournissent ce que la région ne fait pas. */
float intertrade_market_pull(WorldEconomy *e, int region, int good, float want, float unit_price){
    if (!e||region<0||region>=e->n_regions||region>=SCPS_MAX_REG||good<=RES_NONE||good>=RES_COUNT||want<=0.f) return 0.f;
    if (g_hub_dirty){ hub_map_build(e); global_cache_refresh(e); g_hub_dirty=false; }   /* carte/cache sûrs hors tick (levée annuelle) */
    int hub=g_hub_of[region], owner=e->region[region].owner;
    float price=unit_price; if (price<0.2f) price=0.2f;                 /* plancher (cf. MARKET_MIN_PRICE) */
    int toll_r=e->region[region].import_toll_region;
    bool has_host=(toll_r>=0 && toll_r<e->n_regions);
    float base=e->region[region].import_margin; if(base<1.f)base=1.f;
    float got=0.f,t;
    t=centre_take(e,region,good,want); got+=t; want-=t;                 /* 1. PROPRE : gratuit (fer déjà payé) */
    /* étage marchand : la région ACHÈTE aux Centres ÉTRANGERS, borné par son trésor.
       nu(prix)→source ; marge→cité-état hôte si hôte. CONSERVÉ (zéro sink).
       Trésor PROVINCE-owned (it_treasury) : buyer_tr est lu/écrit à chaque appel de la
       macro (elle peut s'invoquer 2× dans le même intertrade_market_pull) → pas de
       staleness, prov[] EST la source (pas un dérivé region[] ré-agrégé plus tard). */
    #define PULL_BUY(R,MULT) do{ if(want>1e-3f){ \
        float *buyer_tr=it_treasury(e,region); \
        float up=price*((has_host)?(MULT):1.f); long aff=(up>0.f&&buyer_tr)?(long)(*buyer_tr/up):0; \
        float w=fminf(want,(float)aff); if(w>1e-3f){ \
            float tk=centre_take(e,(R),good,w); float nu=tk*price, tot=tk*up; \
            float *src_tr=it_treasury(e,(R)); if(src_tr) *src_tr+=nu; \
            if(has_host&&tot>nu){ float *toll_tr=it_treasury(e,toll_r); if(toll_tr) *toll_tr+=(tot-nu); } \
            if(buyer_tr) *buyer_tr-=tot; \
            if(owner>=0) econ_flux_add(owner,FX_IMPORT,-tot); \
            got+=tk; want-=tk; } } }while(0)
    if (hub>=0 && hub!=region && e->region[hub].owner!=owner) PULL_BUY(hub, base);          /* 2. local */
    if (want>1e-3f && hub>=0) for (int r=0;r<e->n_regions && r<SCPS_MAX_REG; r++){           /* 3. mondial */
        if(!g_centre[r]||r==hub||r==region||e->region[r].owner==owner) continue;
        PULL_BUY(r, base*2.f); if(want<=1e-3f) break; }
    #undef PULL_BUY
    return got;   /* les Centres PROPRES (hub/monde) sont fournis par leur propre itération dans econ_arms_take */
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
    /* §5 PUISSANCE COMMERCIALE : l'achat direct au marché est borné au pool MENSUEL de l'empire. */
    int cc_owner=re->owner;
    if (g_commerce_active && cid_ok(cc_owner)){
        float rem=intertrade_commerce_remaining(cc_owner);
        if ((float)want>rem){ g_comm_capped++; want=(long)rem; }   /* §5 : le pool a MORDU */
        if (want<=0) return 0;
    }
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
    float *buyer_tr=it_treasury(e,region);                        /* trésor PROVINCE-owned (représentante) */
    if (!buyer_tr) return 0;
    long can=(long)(*buyer_tr/up); if (qty>can) qty=can;         /* borné par le trésor */
    if (qty<=0) return 0;
    /* RE-KEY + CONSERVATION : on PRÉLÈVE d'abord le RÉEL (provinces, persistant —
     * la vue seule serait effacée à la clôture), on facture et on livre CE réel :
     * jamais de matière créée si la vue promettait plus que les provinces ne tiennent. */
    long got=0; float cost=0.f;
    if (tier<=0){
        got=(long)(-econ_region_stock_add(e, hub, good, -(float)qty));
        if (got<=0) return 0;
        cost=(float)got*up;
        *buyer_tr -= cost;                                        /* PUMP du trésor */
        econ_region_stock_add(e, region, good, (float)got);       /* le bien entre au stock (province) */
        float *hub_tr=it_treasury(e,hub); if (hub_tr) *hub_tr += cost;   /* CONSERVATION : le hub (source+hôte) encaisse le plein */
    } else {
        long rem=qty; float nu_credited=0.f;
        for (int r=0;r<e->n_regions && r<SCPS_MAX_REG && rem>0;r++){
            if (!g_centre[r] || r==region) continue;             /* V2 : JAMAIS sa propre région */
            long t=(long)(-econ_region_stock_add(e, r, good, -(float)rem));
            if (t<=0) continue;
            rem-=t;
            float nu=(float)t*price;
            float *src_tr=it_treasury(e,r); if (src_tr) *src_tr += nu; /* la source encaisse le NU */
            nu_credited += nu;
        }
        got=qty-rem;
        if (got<=0) return 0;
        cost=(float)got*up;
        *buyer_tr -= cost;                                        /* PUMP du trésor (facturé sur le RÉEL) */
        econ_region_stock_add(e, region, good, (float)got);
        float toll = cost - nu_credited;                          /* la marge → cité-état hôte */
        if (toll>0.f && re->import_toll_region>=0 && re->import_toll_region<e->n_regions){
            float *toll_tr=it_treasury(e,re->import_toll_region); if (toll_tr) *toll_tr += toll;
        }
    }
    g_global_cache[good]-=(float)got; if(g_global_cache[good]<0.f)g_global_cache[good]=0.f;  /* V2.2 : cache à jour (anti sur-tirage intra-tick) */
    if (g_commerce_active && cid_ok(cc_owner)) commerce_draw(cc_owner, (float)got);   /* §5 : l'achat draine le pool commercial */
    if (spent) *spent=(long)(cost+0.5f);
    return got;
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
    float *seller_tr=it_treasury(e,region);
    float *dep_tr=it_treasury(e,dep);
    if (!seller_tr || !dep_tr) return 0;
    { float g0=(float)qty*price;
      if (g0 > *dep_tr){                                          /* CONSERVATION : borné au trésor de l'absorbeur */
          float k = (g0>0.f)? *dep_tr/g0 : 0.f;
          qty=(long)((float)qty*k); if (qty<=0) return 0; } }
    /* RE-KEY : débit RÉEL du vendeur (provinces, persistant) — l'encaissé suit CE réel. */
    long sold=(long)(-econ_region_stock_add(e, region, good, -(float)qty));
    if (sold<=0) return 0;
    float gain=(float)sold*price;
    *seller_tr+=gain;
    *dep_tr-=gain;                                                /* l'absorbeur PAIE (vendeur +gain == dep −gain) */
    econ_region_stock_add(e, dep, good, (float)sold);            /* le bien rejoint le marché (un AUTRE Centre, province) */
    g_global_cache[good]+=(float)sold;                           /* V2.2 : cache à jour */
    if (gained) *gained=(long)(gain+0.5f);
    return sold;
}

/* ═══ ESCLAVAGE — LE MARCHÉ DES CENTRES (achat/vente d'esclaves au POOL mondial) ═══
 * Canal des CENTRES (cités-états), PAS un troc bilatéral : un vendeur retire des âmes
 * de SES groupes esclaves (les plus nombreux d'abord), les crédite au POOL MONDIAL
 * (sérialisé, un compteur PAR HÉRITAGE — le pool garde QUI ils sont, la vente ne
 * blanchit pas l'identité) ; un acheteur en tire (le plus nombreux du pool, choix
 * déterministe), crée/renforce un groupe ARR_DEPORTE/CLASS_SLAVE de cet héritage.
 * Prix = SLAVE_PRICE de base × ipm (le même déflateur que le reste du marché). Les
 * cités-états prennent une marge au même taux que market_buy tier mondial (IT_MARGIN
 * implicite via le prix de vente < prix d'achat, motif déjà utilisé par market_buy/
 * sell). matière réelle : les ÂMES sont conservées (Σ pool + Σ groupes = constant),
 * l'OR suit econ_region_treasury_add (jamais un `+=` direct qui fuirait à la clôture). */
#define SLAVE_MARKET_DRIFT_BASE 900000   /* plage de drift_id réservée aux groupes achetés au marché (distincte de SLAVE_DRIFT_BASE) */

static long slave_pool_total(void){
    long t=0; for (int h=0;h<HERITAGE_COUNT;h++) t+=(long)g_slave_pool[h];
    return t;
}
/* l'héritage le plus nombreux du pool (déterministe : départage par index croissant). */
static int slave_pool_top(void){
    int best=-1; float bv=0.f;
    for (int h=0;h<HERITAGE_COUNT;h++) if (g_slave_pool[h]>bv){ bv=g_slave_pool[h]; best=h; }
    return best;
}
void intertrade_slave_pool(float out[HERITAGE_COUNT]){
    if (!out) return;
    for (int h=0;h<HERITAGE_COUNT;h++) out[h]=g_slave_pool[h];
}
long intertrade_slave_pool_count(void){ return slave_pool_total(); }

/* LOT I — LE PRIX DU POOL RESPIRE : pool ≪ RÉFÉRENCE ⇒ rare ⇒ cher (jusqu'à ×2.5),
 * pool ≫ RÉFÉRENCE ⇒ surabondant ⇒ bon marché (jusqu'à ×0.5). Motif des paliers de
 * prix intertrade (borné), appliqué à la profondeur du pool plutôt qu'à la distance. */
static float slave_pool_price_mult(void){
    float ref=tune_f("SLAVE_POOL_REF", 600.f);
    if (ref<1.f) ref=1.f;
    float pool=(float)slave_pool_total();
    float mult=ref/(pool+ref*0.10f);         /* pool=0 ⇒ ~×10 avant clamp (rareté totale) */
    if (mult<0.5f) mult=0.5f; else if (mult>2.5f) mult=2.5f;
    return mult;
}

/* VENTE — `region` (au vendeur `cid`, implicite via re->owner) retire `count` âmes de
 * ses groupes esclaves (les plus nombreux d'abord, à travers toutes ses provinces),
 * les crédite au pool (par héritage), encaisse l'or (prix × ipm, marge Centre incluse
 * dans l'écart achat/vente comme market_buy/sell). Renvoie les âmes réellement vendues. */
long intertrade_slave_sell(WorldEconomy *e, int region, long count){
    if (!e || region<0 || region>=e->n_regions || count<=0) return 0;
    RegionEconomy *re=&e->region[region];
    int cid=re->owner; if (!cid_ok(cid)) return 0;
    float price=tune_f("SLAVE_PRICE",40.f)*econ_world_ipm(e)*slave_pool_price_mult();
    long remaining=count, sold=0;
    /* scanne TOUTES les provinces du vendeur (le stock d'esclaves est dispersé) — les plus
     * gros groupes esclaves d'abord, pour vider peu de provinces plutôt qu'écrémer partout. */
    int np=e->n_prov; if (np>SCPS_MAX_PROV) np=SCPS_MAX_PROV;
    while (remaining>0){
        int bp=-1, bg=-1; long best=0;
        for (int p=0;p<np;p++){
            ProvinceEconomy *pe=&e->prov[p];
            if (pe->owner!=cid) continue;
            ProvincePop *pp=&pe->pop;
            for (int i=0;i<pp->n_groups;i++)
                if (pp->groups[i].klass==CLASS_SLAVE && pp->groups[i].count>best){
                    best=pp->groups[i].count; bp=p; bg=i;
                }
        }
        if (bp<0 || bg<0) break;   /* plus aucun groupe esclave à vendre */
        ProvinceEconomy *pe=&e->prov[bp];
        PopGroup *g=&pe->pop.groups[bg];
        long take=(remaining<g->count)?remaining:g->count;
        if (take<=0) break;
        int h=(int)g->heritage; if (h<0||h>=HERITAGE_COUNT) h=0;
        g_slave_pool[h] += (float)take;
        g->count -= take;
        pe->strata[CLASS_SLAVE].pop = fmaxf(0.f, pe->strata[CLASS_SLAVE].pop - (float)take);
        if (g->count<=0){ pe->pop.groups[bg]=pe->pop.groups[pe->pop.n_groups-1]; pe->pop.n_groups--; }
        remaining -= take; sold += take;
    }
    if (sold<=0) return 0;
    econ_region_treasury_add(e, region, (float)sold*price);
    return sold;
}

/* ACHAT — gate ÉTHOS/TECH (miroir diplo_enslave_capture : `can_enslave` passé par
 * l'appelant, comme le combat) : un abolitionniste (ni éthos esclavagiste, ni
 * TECH_ESCLAVAGE) ne peut PAS acheter. Débite l'or (matière réelle : treasury_add
 * signé), tire l'héritage le PLUS NOMBREUX du pool (déterministe), crée/renforce un
 * groupe ARR_DEPORTE/CLASS_SLAVE sur la province représentative de `region`. */
long intertrade_slave_buy(WorldEconomy *e, int region, long count, bool can_enslave){
    if (!can_enslave) return 0;
    if (!e || region<0 || region>=e->n_regions || count<=0) return 0;
    RegionEconomy *re=&e->region[region];
    int cid=re->owner; if (!cid_ok(cid)) return 0;
    int h=slave_pool_top(); if (h<0) return 0;
    long avail=(long)g_slave_pool[h];
    long want=(count<avail)?count:avail;
    if (want<=0) return 0;
    /* ×2 : la double taxe des Centres (motif tier mondial) ; LOT I : le pool RESPIRE
     * (rare ⇒ cher, surabondant ⇒ bon marché — la marge des Centres reste par-dessus). */
    float price=tune_f("SLAVE_PRICE",40.f)*econ_world_ipm(e)*2.f*slave_pool_price_mult();
    float *buyer_tr=it_treasury(e,region);
    if (!buyer_tr) return 0;
    long can=(long)(*buyer_tr/fmaxf(price,1e-4f));
    if (want>can) want=can;
    if (want<=0) return 0;
    int pid=econ_region_rep_province(e,region);
    if (pid<0 || pid>=e->n_prov) return 0;
    ProvinceEconomy *pe=&e->prov[pid];
    ProvincePop *pp=&pe->pop;
    /* fusionne dans un groupe DÉPORTÉ existant du même héritage si possible, sinon crée. */
    int gi=-1;
    for (int i=0;i<pp->n_groups;i++)
        if (pp->groups[i].klass==CLASS_SLAVE && pp->groups[i].heritage==(Heritage)h){ gi=i; break; }
    if (gi<0){
        if (pp->n_groups>=SCPS_MAX_GROUPS) return 0;
        PopGroup ng; memset(&ng,0,sizeof ng);
        ng.heritage=(Heritage)h; ng.origin_sphere=heritage_sphere((Heritage)h);
        /* le pool est FONGIBLE (l'or blanchit l'origine précise) : la fiche culturelle
         * exacte du vendeur n'est pas conservée à travers le marché — un substrat neutre
         * (axes à 0, settled) porte l'héritage/l'arrival, ce que la friction/diffusion lisent. */
        ng.origin.settled=true; ng.origin.heritage=(Heritage)h;
        ng.culture=ng.origin;
        ng.klass=CLASS_SLAVE; ng.count=want; ng.diaspora=true; ng.arrival=ARR_DEPORTE; ng.integration=0.f;
        ng.home_reg=-1; ng.faith=-1;
        ng.drift_id=SLAVE_MARKET_DRIFT_BASE + region*SCPS_MAX_GROUPS + pp->n_groups;
        memset(ng.pop_by_class,0,sizeof ng.pop_by_class); ng.pop_by_class[CLASS_SLAVE]=want;
        pp->groups[pp->n_groups++]=ng;
    } else {
        pp->groups[gi].count += want;
        pp->groups[gi].pop_by_class[CLASS_SLAVE] += want;
    }
    pe->strata[CLASS_SLAVE].pop += (float)want;
    g_slave_pool[h] -= (float)want;
    *buyer_tr -= (float)want*price;
    return want;
}

void  intertrade_order_embargo(int cid, int target, bool on){
    if (cid_ok(cid) && cid_ok(target) && cid!=target){
        if ((g_embargo[cid][target]!=0)!=on)                  /* journal : au FLIP seulement */
            diplog_push(on?DACT_EMBARGO:DACT_EMBARGO_LIFT, cid, target, 0.f);
        g_embargo[cid][target]=on;
    }
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

/* SAVETEST FIX — la carte des hubs (g_hub_of/dist, rebâtie SEULEMENT à un changement de Centres,
 * pas chaque jour) + le cache mondial + le drapeau dirty sont LUS quotidiennement (agency_build_gold
 * via intertrade_market_avail/buy_cost/consume) mais NON dérivés du tick : sans les sérialiser, un
 * reload gardait la carte de FIN du run précédent → dérive économique (--savetest A≠B). On les
 * restaure À L'IDENTIQUE — surtout PAS de g_hub_dirty=true forcé, qui rebâtirait une carte plus
 * FRAÎCHE que celle du jour de save (le vrai bug de la 1re tentative dirty-check). */
void intertrade_save(FILE *f){ fwrite(g_embargo,sizeof g_embargo,1,f); fwrite(g_centre,sizeof g_centre,1,f);
                               fwrite(g_hub_of,sizeof g_hub_of,1,f);   fwrite(g_hub_dist,sizeof g_hub_dist,1,f);
                               fwrite(&g_hub_dirty,sizeof g_hub_dirty,1,f); fwrite(g_global_cache,sizeof g_global_cache,1,f);
                               fwrite(g_commerce_budget,sizeof g_commerce_budget,1,f); fwrite(g_commerce_spent,sizeof g_commerce_spent,1,f);  /* §5 : pool commercial intra-mois */
                               fwrite(g_slave_pool,sizeof g_slave_pool,1,f); }   /* ESCLAVAGE : pool mondial par héritage */
bool intertrade_load(FILE *f){ bool ok = fread(g_embargo,sizeof g_embargo,1,f)==1
                                   && fread(g_centre,sizeof g_centre,1,f)==1
                                   && fread(g_hub_of,sizeof g_hub_of,1,f)==1
                                   && fread(g_hub_dist,sizeof g_hub_dist,1,f)==1
                                   && fread(&g_hub_dirty,sizeof g_hub_dirty,1,f)==1
                                   && fread(g_global_cache,sizeof g_global_cache,1,f)==1
                                   && fread(g_commerce_budget,sizeof g_commerce_budget,1,f)==1
                                   && fread(g_commerce_spent,sizeof g_commerce_spent,1,f)==1  /* §5 */
                                   && fread(g_slave_pool,sizeof g_slave_pool,1,f)==1;   /* ESCLAVAGE */
                               g_commerce_active=true;   /* une partie chargée est une sim ACTIVE (le cap agit) */
                               return ok; }
/* OOB FIX (défaut #5, 2026-07-07) — g_hub_of/g_hub_dist sont désérialisés BRUT ci-dessus
 * (int16_t, plage jusqu'à 32767) mais lus SANS borne haute par les devis de marché
 * (intertrade_buy_cost:426 `e->region[hub]`, entre autres) — e->region[] est un tableau
 * FIXE de taille SCPS_MAX_REG : un save FORGÉ y écrivant une valeur ≥ n_regions cause une
 * lecture hors-bornes dès le 1er tick de bâti post-load (agency_build_gold, AUCUN dirty-
 * rebuild sur ce chemin). Le tableau ENTIER (SCPS_MAX_REG slots) est toujours validé —
 * pas seulement [0,n_regions) — car c'est ainsi qu'il est POPULÉ en jeu légitime
 * (hub_map_build memset -1 sur TOUT SCPS_MAX_REG puis ne remplit que r<n, cf. ligne
 * ~318) : un jeu sain n'a JAMAIS de garbage au-delà de n_regions, donc valider le plein
 * tableau ne rejette aucune save légitime tout en couvrant une forge n'importe où.
 * g_hub_dist n'est jamais utilisé comme INDEX (juste plafonné à 8 en lecture) — bornage
 * en profondeur de défense seulement, sur la portée BFS + le forfait de traversée mer. */
bool intertrade_save_sane(int n_regions){
    if (n_regions<0) n_regions=0;
    if (n_regions>SCPS_MAX_REG) n_regions=SCPS_MAX_REG;
    for (int r=0;r<SCPS_MAX_REG;r++){
        if (g_hub_of[r] < -1 || g_hub_of[r] >= n_regions) return false;
        if (g_hub_dist[r] < 0 || g_hub_dist[r] > SCPS_MAX_REG + IT_SEA_HOPS) return false;
    }
    return true;
}

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
            if (any_centre && o>=0){
                int hub = (r<SCPS_MAX_REG)? g_hub_of[r] : -1;
                if (hub<0){ re->import_margin=m_none; }           /* aucun marché atteignable : enclavé */
                else {
                    bool own = (e->region[hub].owner==o) || (re->edi_built & (1u<<EDI_COMPTOIR));
                    int d = (r<SCPS_MAX_REG)? g_hub_dist[r] : 0; if (d>8) d=8;   /* rendement dégressif, plafonné */
                    re->import_margin = (own?m_own:m_third) * (1.f + dfall*(float)d);
                    if (!own) re->import_toll_region=(int16_t)hub;   /* péage à la cité-état hôte (la plus proche) */
                }
            }
            /* RE-KEY : MIROIR sur la province représentative — l'agrégation recopie
             * margin/toll depuis ELLE ; sans miroir, ils retombaient à 1.0/-1 dès le
             * mois suivant (la marge d'import ne « tenait » qu'un mois sur douze). */
            { int pp=econ_region_rep_province(e,r);
              if (pp>=0 && pp<e->n_prov){ e->prov[pp].import_margin=re->import_margin;
                                          e->prov[pp].import_toll_region=re->import_toll_region; } }
        }
    }
    float trade_levy = tune_f("TRADE_LEVY", 0.10f);   /* prélèvement importateur (conservation) */
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
        /* WG — LE PÉAGE DE DÉTROIT : si la route maritime franchit un goulet (posé à la
         * création) TENU par un TIERS (ni ca ni cb), à la paix avec les deux bouts, son
         * propriétaire SKIME une part de chaque échange (transfert exportateur→tenant,
         * ∝ étroitesse du goulet). Le verrou rapporte à qui le tient. */
        int   choke_hold_reg=-1, choke_hold_cid=-1; float choke_rate=0.f; bool choke_tax_route=false;
        if (rt->maritime && rt->choke_region>=0 && rt->choke_region<e->n_regions){
            int hc=e->region[rt->choke_region].owner;
            if (cid_ok(hc) && hc!=ca && hc!=cb
                && pair_at_peace(dp,hc,ca) && pair_at_peace(dp,hc,cb)
                && !intertrade_embargoed(hc,ca) && !intertrade_embargoed(hc,cb)){
                choke_hold_reg=rt->choke_region; choke_hold_cid=hc;
                choke_rate=IT_CHOKE_TOLL*(0.4f+0.6f*rt->choke_block);   /* l'étroitesse durcit le droit */
            }
        }
        for (int g=1; g<RES_COUNT; g++){
            float pa=A->price[g], pb=B->price[g];
            bool a_to_b=(pa<pb);                      /* le bien va du bon marché vers le cher */
            float cost=it_transport_frac(rt,a_to_b)*((pa+pb)*0.5f)*conn_mult;
            if (fabsf(pa-pb) <= cost) continue;       /* marge trop mince POUR CE SENS → rien ne bouge */
            RegionEconomy *src=a_to_b?A:B, *dst=a_to_b?B:A;     /* on achète au moins cher */
            int src_r=a_to_b?ra:rb, dst_r=a_to_b?rb:ra;         /* trésor PROVINCE-owned : indices région → représentante */
            float vol=fminf(cap*it_volume_mult(rt,a_to_b), fmaxf(0.f, src->stock[g]-econ_build_reserve((Resource)g))*IT_EXPORT_FRAC);  /* garde le FOND de bâti */
            if (vol<=0.001f) continue;
            float *src_tr=it_treasury(e,src_r), *dst_tr=it_treasury(e,dst_r);
            if (!src_tr || !dst_tr) continue;
            /* CONSERVATION (zéro faucet) : l'acheteur PAIE total=gross·(1+levy), le vendeur
             * l'ENCAISSE intégralement. Borné au trésor de l'importateur (sans or → pas d'achat). */
            if (*dst_tr <= 0.f) continue;
            float price=(pa+pb)*0.5f;            /* médian : les deux bouts y gagnent */
            float gross=vol*price;
            float total=gross*(1.f+trade_levy);
            if (total > *dst_tr){ float k=*dst_tr/total; vol*=k; gross*=k; total=*dst_tr; }
            if (vol<=0.001f) continue;
            /* RE-KEY : le bien bouge PROVINCE-persistant (la vue seule serait effacée
             * à la clôture) — on livre et on facture le RÉEL pris à la source. */
            { float moved = -econ_region_stock_add(e, src_r, g, -vol);
              if (moved<=0.001f) continue;
              if (moved<vol){ float k=moved/vol; gross*=k; total*=k; vol=moved; } }
            econ_region_stock_add(e, dst_r, g, vol);
            { bool down=it_is_downstream(rt,a_to_b);  /* télémétrie : le tri par sens ÉMERGE */
              if (rt->maritime||rt->fluvial){
                  if (down) g_vol_down+=vol; else g_vol_up+=vol;
                  if (it_is_bulk(g))     { if (down) g_bulk_down+=vol; else g_bulk_up+=vol; }
                  if (it_is_precious(g)) { if (down) g_prec_down+=vol; else { g_prec_up+=vol; g_nprec_up++; } }
              } }
            *dst_tr -= total;                                   /* l'acheteur PAIE Y */
            *src_tr += total;                                   /* le vendeur ENCAISSE Y (trésor réel) */
            if (src->owner>=0){ econ_flux_add(src->owner, FX_EXPORT, gross);
                                econ_flux_add(src->owner, FX_TOLL_RECV, total-gross); }  /* I0 */
            if (dst->owner>=0) econ_flux_add(dst->owner, FX_IMPORT, -total);
            /* WG — le tenant du détroit prélève SA part (transfert exportateur→tenant :
             * conservation préservée, l'importateur ne paie pas plus, le verrou skime). */
            if (choke_hold_reg>=0){
                float toll=gross*choke_rate;
                if (toll>*src_tr) toll=*src_tr;
                if (toll<0.f) toll=0.f;
                if (toll>0.f){
                    *src_tr -= toll;
                    float *choke_tr=it_treasury(e,choke_hold_reg); if (choke_tr) *choke_tr += toll;
                    if (cid_ok(choke_hold_cid)){
                        g_choke_toll[choke_hold_cid]+=toll; econ_flux_add(choke_hold_cid, FX_TOLL_RECV, toll);
                        g_choke_toll_cumul[choke_hold_cid]+=toll;   /* le CUMUL de sim (la preuve) */
                    }
                    g_choke_toll_total+=toll; g_choke_toll_cumul_total+=toll; choke_tax_route=true;
                }
            }
            g_last_value += gross;
            /* la valeur PASSE par les Centres des deux couronnes (moitié chacun) —
             * la part des cités-états dans le commerce mondial se lit là. */
            { int cca=intertrade_country_centre(e,ca), ccb=intertrade_country_centre(e,cb);
              if (cca>=0&&cca<SCPS_MAX_REG) g_centre_val[cca]+=gross*0.5f;
              if (ccb>=0&&ccb<SCPS_MAX_REG) g_centre_val[ccb]+=gross*0.5f; }
            /* comptabilité des flux (sidebar) : qui exporte/importe quoi, vers/depuis qui */
            { int cs=(src==A)?ca:cb, cd=(dst==A)?ca:cb;
              if (cid_ok(cs)&&cid_ok(cd)){
                  g_expt[cs][g]+=vol; g_imp[cd][g]+=vol; g_gold[cs]+=total;
                  g_pair[cs][cd]+=gross; g_pair[cd][cs]+=gross;
                  if (vol>g_expt_best[cs][g]){ g_expt_best[cs][g]=vol; g_expt_to [cs][g]=(int16_t)cd; }
                  if (vol>g_imp_best [cd][g]){ g_imp_best [cd][g]=vol; g_imp_from[cd][g]=(int16_t)cs; }
              } }
            /* RETIRÉ (LOT 2, réparations) : le nudge de convergence de prix écrivait
             * A->price[g]/B->price[g] — pointeurs dans e->region[], la VUE TRANSIENTE
             * rebâtie CHAQUE mois par econ_aggregate_regions (depuis prov[].price, le
             * store RÉEL — jamais touché ici). Le mois suivant, econ_tick écrase aussi
             * prov[].price via la formule « PRIX NATIONAL » (pure fonction demand_nat/
             * pool/supply_nat, sans mémoire de ce nudge). Le nudge ne pouvait donc
             * jamais survivre au-delà du jour où il s'écrivait : mort par construction
             * sous le régime de prix national (2026-06-28). Preuve : econ_aggregate_regions
             * (scps_econ.c) recopie ag->price[]=pe->price[] à chaque appel ; aucun site
             * n'écrit prov[].price depuis intertrade. Voir CLAUDE.md « MISSION ÉCO ». */
        }
        if (choke_tax_route) g_n_choke_routes++;   /* WG : cette route a payé le verrou ce tick */
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
            /* RE-KEY : drain/dépôt PROVINCE-persistants — l'or suit le RÉEL pris. */
            vol = -econ_region_stock_add(e, src, g, -vol);
            if (vol<0.5f) continue;
            econ_region_stock_add(e, h, g, vol);                 /* le Centre stocke son marché local */
            { float *src_tr=it_treasury(e,src); if (src_tr) *src_tr += vol*sp*IT_MARGIN_TO_GOLD; } /* l'exportateur source encaisse l'or */
            float profit=vol*(lp-sp)*cap;                        /* le CE encaisse une PART BORNÉE du spread */
            { float *h_tr=it_treasury(e,h); if (h_tr) *h_tr += profit; } g_centre_val[h]+=profit;
            if (H->owner>=0) econ_flux_add(H->owner, FX_EXPORT, profit);
            /* RETIRÉ (LOT 2, réparations) : même nudge mort que le bloc route ci-dessus —
             * H->price[g]/e->region[src].price[g] sont la vue transiente, écrasée au
             * prochain econ_aggregate_regions puis re-projetée depuis le PRIX NATIONAL.
             * L'arbitrage réel (stock/trésor déplacés, borné par vcap/minsp) reste INTACT :
             * seul le nudge de prix, sans effet observable, disparaît. */
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

/* WG — lecteurs du PÉAGE DE DÉTROIT (dernier tick + cumul de sim). */
float intertrade_choke_toll_total(void){ return g_choke_toll_total; }
int   intertrade_choke_routes(void){ return g_n_choke_routes; }
float intertrade_choke_toll_country(int cid){ return cid_ok(cid)?g_choke_toll[cid]:0.f; }
double intertrade_choke_toll_cumul_total(void){ return g_choke_toll_cumul_total; }
double intertrade_choke_toll_cumul_country(int cid){ return cid_ok(cid)?g_choke_toll_cumul[cid]:0.0; }

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
