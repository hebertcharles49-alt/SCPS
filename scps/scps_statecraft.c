/*
 * scps_statecraft.c — Influence, Opinion, Diplomates & Révolte (voir .h)
 *
 * Tout est ancré : l'Influence SUIT la prospérité/taille/prestige réels ;
 * l'Opinion PROJETTE les lecteurs de relation + l'historique ; une mission
 * OCCUPE un agent en jours puis agit par la couche d'action ; la révolte naît
 * d'une agitation soutenue que la stabilité/H/L abattent (mécanique existante).
 */
#include "scps_statecraft.h"
#include "scps_readout.h"   /* metric_agitation / metric_stability (réutilisés) */
#include "scps_lang.h"      /* Q1 : STR_COUNCIL_NAME_* (noms de candidats) */
#include "scps_culture.h"   /* Q1 : ETHOS_* (l'IA recrute selon l'éthos) */
#include "scps_tune.h"      /* #26 : tunables OPINION_* */
#include "scps_intertrade.h"/* #26 : intertrade_embargoed (la mémoire de l'embargo) */
#include <string.h>

/* ---- Calibrage --------------------------------------------------------- */
#define SC_INFLUENCE_RATE   0.010f   /* vitesse de convergence vers le standing /jour */
#define SC_OPINION_RATE     0.006f   /* l'opinion bouge lentement (l'histoire colle)  */
#define SC_PRESTIGE_DECAY   0.010f   /* le prestige s'érode sans entretien /jour       */
#define SC_AGIT_RATE        0.020f   /* lissage de l'agitation soutenue /jour          */
#define REVOLT_SUSTAIN_DAYS 365      /* agitation au seuil pendant un an → révolte      */
#define DIP_PER_INFLUENCE   25       /* +1 diplomate par 25 d'Influence                 */

static inline float clampf(float v,float lo,float hi){ return v!=v?lo:(v<lo?lo:(v>hi?hi:v)); }
static inline int   iclamp(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
static inline float toward(float cur,float tgt,float k){ return cur + (tgt-cur)*clampf(k,0.f,1.f); }
static inline float absf(float v){ return v<0?-v:v; }

static float pc_dist(const PopCulture *a, const PopCulture *b){
    float dv=absf(a->valeurs-b->valeurs), ds=absf(a->subsistance-b->subsistance);
    float dp=absf(a->parente-b->parente), dr=absf(a->religion-b->religion);
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}
static const PopCulture *ruling_culture(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0 || cid>=w->n_countries) return NULL;
    int cp=w->country[cid].capital_prov;
    if (cp<0 || cp>=w->n_provinces) return NULL;
    int cr=w->province[cp].region;
    return (cr>=0 && cr<econ->n_regions) ? &econ->region[cr].culture : NULL;
}
static int cap_region_of(const World *w, int cid){
    if (cid<0 || cid>=w->n_countries) return -1;
    int cp=w->country[cid].capital_prov;
    return (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;
}
static int country_size(const WorldEconomy *econ, int cid){
    int n=0;
    for (int r=0;r<econ->n_regions;r++)
        if (econ->region[r].owner==cid && econ->region[r].culture.settled) n++;
    return n;
}

const char *dip_mission_name(DipMission m){
    switch(m){
        case DIP_RELATIONS: return "Améliorer les relations";
        case DIP_ALLIANCE:  return "Proposer une alliance";
        case DIP_INTEGRATE: return "Intégrer une province";
        case DIP_ROUTE:     return "Établir une route";
        default:            return "Au repos";
    }
}

/* ======================================================================= */
void statecraft_init(Statecraft *sc, const World *w){
    memset(sc, 0, sizeof(*sc));
    sc->n_countries = w->n_countries;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)                /* Q1 : memset→0 = slot 0 valide : tous VACANTS (-1) */
        for (int s=0;s<SC_COUNCIL_SEATS;s++){ sc->council[c][s]=-1; sc->council_gen[c][s]=-1; }
    for (int c=0;c<w->n_countries;c++){
        sc->influence[c]   = 35.f;             /* une réputation initiale modeste */
        sc->prestige[c]    = 8.f;
        sc->staff[c].count = SC_BASE_DIPLOMATS;
        for (int b=0;b<w->n_countries;b++) sc->opinion[c][b]=0.f;
    }
}

/* ═══ Q1 — LE CONSEIL (I7) ═══════════════════════════════════════════════════
 * 3 sièges (Savoir/Société/Industrie). Chaque siège propose SC_COUNCIL_CANDS
 * candidats DÉTERMINISTES (tier + nom dérivés du seed) ; le joueur/IA en NOMME un
 * (council[cid][seat] = slot) ou RENVOIE (-1). L'effet est un MULTIPLICATEUR lecteur
 * (×savoir / ×promo / ×manuf), le coût une ponction mensuelle (×IPM). Rien d'autre. */
static const float SC_SEAT_BASE[SC_COUNCIL_SEATS] = { 0.20f, 0.12f, 0.15f }; /* +20/+12/+15 % à tier-effet 1 */
static const float SC_TIER_EFFET[4] = { 0.f, 1.0f, 1.5f, 2.0f };            /* index = tier 1..3 */
static const float SC_TIER_COST [4] = { 0.f, 8.f, 16.f, 28.f };             /* or/mois, ×IPM */

static uint32_t sc_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d){
    uint32_t h = a*2654435761u ^ b*40503u ^ c*2246822519u ^ d*3266489917u;
    h ^= h>>16; h *= 2246822519u; h ^= h>>13; h *= 3266489917u; h ^= h>>16;
    return h;
}
/* ── GÉNÉRATIONS DE POOL + ÂGE (dérivés, jamais sérialisés) ────────────────────
 * La pool se renouvelle par GÉNÉRATION (année/GEN_YEARS) : toujours 3 candidats
 * par siège, jamais épuisée. gen 0 laisse la graine INTACTE → mêmes hash qu'avant
 * (fenêtre golden intouchée). L'âge = base 30-51 + années écoulées dans la
 * génération ; la retraite (66-73, jitter par identité) est IMPOSSIBLE avant
 * l'an 16 (> golden 12) par construction. */
static uint32_t sc_genseed(uint32_t seed, int gen){
    return seed ^ ((uint32_t)(gen>0?gen:0) * 0x9E3779B9u);
}
int statecraft_council_gen(int year){
    return (year>0 ? year : 0) / SC_COUNCIL_GEN_YEARS;
}
int statecraft_council_cand_tier(uint32_t seed, int cid, int seat, int slot, int gen){
    uint32_t h = sc_hash(sc_genseed(seed,gen)^0xC0FFEEu, (uint32_t)cid, (uint32_t)seat, (uint32_t)slot) % 100u;
    return (h<55)?1:(h<85)?2:3;   /* 55 % tier1 · 30 % tier2 · 15 % tier3 (les grands sont rares) */
}
int statecraft_council_cand_name(uint32_t seed, int cid, int seat, int slot, int gen){
    uint32_t h = sc_hash(sc_genseed(seed,gen)^0x5EAB011u, (uint32_t)cid, (uint32_t)(seat*7+slot), 0x9E37u);
    return (int)STR_COUNCIL_NAME_0 + (int)(h % (uint32_t)SC_COUNCIL_NAMES);
}
int statecraft_council_cand_age(uint32_t seed, int cid, int seat, int slot, int gen, int year){
    int base = 30 + (int)(sc_hash(sc_genseed(seed,gen)^0xA6E11u, (uint32_t)cid, (uint32_t)seat, (uint32_t)slot) % 22u);
    int el = year - (gen>0?gen:0)*SC_COUNCIL_GEN_YEARS;
    return base + (el>0 ? el : 0);      /* l'âge GRANDIT avec l'année */
}
static int sc_retire_age(uint32_t seed, int cid, int seat, int slot, int gen){
    return 66 + (int)(sc_hash(sc_genseed(seed,gen)^0x0DDA6Eu, (uint32_t)cid, (uint32_t)seat, (uint32_t)slot) % 8u);
}
int statecraft_council_seated(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return -1;
    int s=sc->council[cid][seat];
    return (s>=0 && s<SC_COUNCIL_CANDS) ? s : -1;
}
int statecraft_council_seated_gen(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return 0;
    int g=sc->council_gen[cid][seat];
    return (g>=0) ? g : 0;              /* garde : vacant/état legacy → génération 0 */
}
int statecraft_council_seated_age(const Statecraft *sc, uint32_t seed, int cid, int seat, int year){
    int slot=statecraft_council_seated(sc,cid,seat);
    if (slot<0) return -1;
    return statecraft_council_cand_age(seed,cid,seat,slot,statecraft_council_seated_gen(sc,cid,seat),year);
}
void statecraft_council_hire(Statecraft *sc, int cid, int seat, int slot, int gen){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    if (slot<0||slot>=SC_COUNCIL_CANDS) return;
    sc->council[cid][seat]=(int8_t)slot;
    sc->council_gen[cid][seat]=(int8_t)((gen>=0 && gen<=120) ? gen : 0);   /* identité ÉPINGLÉE au moment de l'embauche */
}
void statecraft_council_dismiss(Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    sc->council[cid][seat]=-1;
    sc->council_gen[cid][seat]=-1;
}
/* LES ANNÉES PASSENT (annuel) : la retraite VIDE le siège — l'IA repourvoit au
 * mois suivant (statecraft_council_ai), le joueur par l'UI. */
void statecraft_council_age_tick(Statecraft *sc, uint32_t seed, int year){
    if (!sc) return;
    for (int c=0;c<sc->n_countries && c<SCPS_MAX_COUNTRY;c++)
        for (int seat=0;seat<SC_COUNCIL_SEATS;seat++){
            int slot=statecraft_council_seated(sc,c,seat);
            if (slot<0) continue;
            int gen=statecraft_council_seated_gen(sc,c,seat);
            if (statecraft_council_cand_age(seed,c,seat,slot,gen,year)
                >= sc_retire_age(seed,c,seat,slot,gen))
                statecraft_council_dismiss(sc,c,seat);
        }
}
float statecraft_council_seat_mult(const Statecraft *sc, uint32_t seed, int cid, int seat){
    int slot=statecraft_council_seated(sc,cid,seat);
    if (slot<0) return 1.f;
    int tier=statecraft_council_cand_tier(seed,cid,seat,slot,statecraft_council_seated_gen(sc,cid,seat));
    return 1.f + SC_SEAT_BASE[seat]*SC_TIER_EFFET[tier];
}
float statecraft_council_cost(const Statecraft *sc, uint32_t seed, int cid, float ipm){
    if (ipm<=0.f) ipm=1.f;
    float tot=0.f;
    for (int s=0;s<SC_COUNCIL_SEATS;s++){
        int slot=statecraft_council_seated(sc,cid,s);
        if (slot<0) continue;
        tot += SC_TIER_COST[ statecraft_council_cand_tier(seed,cid,s,slot,statecraft_council_seated_gen(sc,cid,s)) ] * ipm;
    }
    return tot;
}
float statecraft_council_cand_cost(uint32_t seed, int cid, int seat, int slot, int gen, float ipm){
    if (ipm<=0.f) ipm=1.f;
    if (cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS||slot<0||slot>=SC_COUNCIL_CANDS) return 0.f;
    return SC_TIER_COST[ statecraft_council_cand_tier(seed,cid,seat,slot,gen) ] * ipm;
}
void statecraft_council_apply(const Statecraft *sc, const World *w, WorldEconomy *e, uint32_t seed, float dt_year){
    if (!sc||!w||!e) return;
    float ipm = econ_world_ipm(e);
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++){
        for (int s=0;s<SC_COUNCIL_SEATS;s++)
            econ_set_council_mult(c, s, statecraft_council_seat_mult(sc,seed,c,s));   /* LECTEUR : ×savoir/×promo/×manuf */
        float cost = statecraft_council_cost(sc,seed,c,ipm) * dt_year * 12.f;          /* or/mois × mois écoulés */
        if (cost<=0.f) continue;
        int cap=w->country[c].capital_prov;                                            /* ponction au trésor de la couronne */
        int cr =(cap>=0&&cap<w->n_provinces)?w->province[cap].region:-1;
        /* RE-KEY PROVINCE : treasury province-owned — route sur la représentative. */
        if (cr>=0 && cr<e->n_regions){
            int crp=econ_region_rep_province(e,cr);
            if (crp>=0 && crp<e->n_prov){ e->prov[crp].treasury -= cost; econ_flux_add(c, FX_CONSEIL, -cost); }
        }
    }
}
/* Q1 — IA du conseil : l'éthos de la capitale privilégie un siège ; on le pourvoit
 * du MEILLEUR candidat tenable dans la GARDE DE BUDGET (≤ 6 mois de loyer en réserve).
 * Une nomination à la fois ; no-op si déjà pourvu ou trésor trop court. */
static int sc_ethos_seat(int ethos){
    switch (ethos){
        case ETHOS_BUREAUCRATE: case ETHOS_PACIFISTE: return 0;  /* Savoir */
        case ETHOS_ORDRE:                              return 1;  /* Société */
        default:                                       return 2;  /* Industrie (Dominateur/Honneur/Mercantile) */
    }
}
void statecraft_council_ai(Statecraft *sc, const World *w, const WorldEconomy *e, uint32_t seed, int cid, int year){
    if (!sc||!w||!e||cid<0||cid>=SCPS_MAX_COUNTRY) return;
    int cap=w->country[cid].capital_prov;
    int cr =(cap>=0&&cap<w->n_provinces)?w->province[cap].region:-1;
    if (cr<0||cr>=e->n_regions || !e->region[cr].culture.settled) return;
    int seat=sc_ethos_seat((int)e->region[cr].culture.ethos);
    if (statecraft_council_seated(sc,cid,seat)>=0) return;                 /* déjà pourvu */
    float tres=e->region[cr].treasury, ipm=econ_world_ipm(e);
    int gen=statecraft_council_gen(year);                                  /* la pool COURANTE (toujours 3 candidats) */
    int best=-1, bestt=0;
    for (int slot=0; slot<SC_COUNCIL_CANDS; slot++){
        int t=statecraft_council_cand_tier(seed,cid,seat,slot,gen);
        if (SC_TIER_COST[t]*ipm*6.f > tres) continue;                     /* hors garde de budget (6 mois) */
        if (t>bestt){ bestt=t; best=slot; }
    }
    if (best>=0) statecraft_council_hire(sc,cid,seat,best,gen);
}

/* ---- Lecteurs ---------------------------------------------------------- */
float statecraft_influence_flux(const Statecraft *sc, const WorldEconomy *econ,
                                const WorldProsperity *wp, int cid){
    if (!sc||!econ||!wp||cid<0||cid>=SCPS_MAX_COUNTRY) return 0.f;
    float prosp = (cid<wp->n_countries)? clampf(wp->country[cid].P_realise,0.f,10.f):0.f;
    float size  = (float)country_size(econ, cid);
    float standing = clampf(prosp*4.f + size*3.f + sc->prestige[cid], 0.f, 100.f);
    return (standing - sc->influence[cid]) * SC_INFLUENCE_RATE;   /* /jour */
}

int statecraft_influence(const Statecraft *sc, int cid){
    if (cid<0||cid>=sc->n_countries) return 0;
    return iclamp((int)(sc->influence[cid]+0.5f), 0, 100);
}
int statecraft_opinion(const Statecraft *sc, int a, int b){
    if (a<0||a>=sc->n_countries||b<0||b>=sc->n_countries) return 0;
    return iclamp((int)(sc->opinion[a][b]+(sc->opinion[a][b]<0?-0.5f:0.5f)), -100, 100);
}
static int staff_cap(const Statecraft *sc, int cid){
    int c = SC_BASE_DIPLOMATS + (int)(sc->influence[cid]/DIP_PER_INFLUENCE);
    return iclamp(c, SC_BASE_DIPLOMATS, SC_MAX_DIPLOMATS);
}
int statecraft_missions_cap(const Statecraft *sc, int cid){
    if (cid<0||cid>=sc->n_countries) return 0;
    return staff_cap(sc, cid);
}
int statecraft_missions_active(const Statecraft *sc, int cid){
    if (cid<0||cid>=sc->n_countries) return 0;
    int n=0; const DiplomaticStaff *st=&sc->staff[cid];
    for (int i=0;i<SC_MAX_DIPLOMATS;i++) if (st->agents[i].mission!=DIP_IDLE) n++;
    return n;
}
int statecraft_agitation(const Statecraft *sc, int region){
    if (region<0||region>=SCPS_MAX_REG) return 0;
    return iclamp((int)(sc->agitation[region]+0.5f), 0, 100);
}
bool statecraft_revolt_fired(const Statecraft *sc, int region){
    return (region>=0 && region<SCPS_MAX_REG) ? sc->revolt_fired[region] : false;
}

/* ---- Événements d'influence ------------------------------------------- */
void statecraft_on_accord_kept(Statecraft *sc, int cid){
    if (cid<0||cid>=sc->n_countries) return;
    sc->prestige[cid]  = clampf(sc->prestige[cid]+4.f, 0.f, 30.f);
    sc->influence[cid] = clampf(sc->influence[cid]+3.f, 0.f, 100.f);
}
void statecraft_on_betrayal(Statecraft *sc, int cid){
    if (cid<0||cid>=sc->n_countries) return;
    sc->prestige[cid]  = clampf(sc->prestige[cid]-12.f, 0.f, 30.f);
    sc->influence[cid] = clampf(sc->influence[cid]-15.f, 0.f, 100.f);
    /* La parole rompue crève l'opinion que les AUTRES ont du traître — #26 : la trahison entre
     * dans la MÉMOIRE DURABLE (opinion_mem), elle SURVIT au statut et s'estompe sur des années
     * (≠ un coup transient qui se reconvergerait en un mois). */
    float bet = tune_f("OPINION_MEM_BETRAYAL",35.f), cap = tune_f("OPINION_MEM_CAP",100.f);
    for (int b=0;b<sc->n_countries;b++) if (b!=cid)
        sc->opinion_mem[b][cid] = clampf(sc->opinion_mem[b][cid]-bet, -cap, cap);
}

/* ---- Missions ---------------------------------------------------------- */
bool statecraft_send(Statecraft *sc, const World *w, const WorldEconomy *econ,
                     int cid, DipMission mission, int target){
    if (cid<0 || cid>=sc->n_countries || mission==DIP_IDLE) return false;
    /* Cible valide selon le type. */
    if (mission==DIP_RELATIONS || mission==DIP_ALLIANCE){
        if (target<0 || target>=w->n_countries || target==cid) return false;
    } else { /* INTEGRATE / ROUTE : une région */
        if (target<0 || target>=econ->n_regions) return false;
    }
    int cap = staff_cap(sc, cid);
    DiplomaticStaff *st=&sc->staff[cid];
    int slot=-1;
    for (int i=0;i<cap;i++) if (st->agents[i].mission==DIP_IDLE){ slot=i; break; }
    if (slot<0) return false;                  /* vivier saturé : plafond d'Influence */

    int home = cap_region_of(w, cid);
    int days;
    switch (mission){
        case DIP_RELATIONS: days=180; break;
        case DIP_ALLIANCE:  days=30;  break;
        case DIP_ROUTE:     days=90;  break;
        case DIP_INTEGRATE: {
            /* ∝ D∞ : avaler du lointain prend des générations. */
            const PopCulture *rul=ruling_culture(w,econ,cid);
            float d = rul ? pc_dist(&econ->region[target].culture, rul) : 5.f;
            days = 300 + (int)(d*150.f);
        } break;
        default: return false;
    }
    st->agents[slot].mission=mission;
    st->agents[slot].target=target;
    st->agents[slot].home_region=home;
    st->agents[slot].days_left=days;
    return true;
}

/* Effet d'une mission mûre — par la couche d'ACTION (mêmes verbes que le joueur). */
static void mission_complete(Statecraft *sc, World *w, WorldEconomy *econ,
                             WorldLegitimacy *wl, DiploState *diplo, RouteNetwork *rn,
                             int cid, const Diplomat *ag){
    switch (ag->mission){
        case DIP_RELATIONS:
            sc->opinion[cid][ag->target] = clampf(sc->opinion[cid][ag->target]+20.f, -100.f, 100.f);
            sc->opinion[ag->target][cid] = clampf(sc->opinion[ag->target][cid]+12.f, -100.f, 100.f);
            break;
        case DIP_ALLIANCE:
            if (diplo && sc->opinion[cid][ag->target] >= 0.f &&
                diplo_status(diplo,cid,ag->target)!=DIPLO_WAR &&
                diplo_ally_count(diplo,cid)        < DIPLO_ALLY_SLOTS &&   /* §D-sat : 2 slots, */
                diplo_ally_count(diplo,ag->target) < DIPLO_ALLY_SLOTS){    /* invariant GLOBAL */
                diplo_form_alliance(diplo, cid, ag->target);
                statecraft_on_accord_kept(sc, cid);     /* un pacte tenu : prestige↑ */
            } else {
                sc->opinion[cid][ag->target] = clampf(sc->opinion[cid][ag->target]+8.f,-100.f,100.f);
            }
            break;
        case DIP_ROUTE:
            if (rn) routes_order(rn, NULL, econ, ag->home_region, ag->target, false);
            break;
        case DIP_INTEGRATE:
            /* Accélère la montée de Légitimité : l'intégration fait un bond
             * (ancienneté de tutelle) et remonte le consentement local. */
            if (wl && ag->target>=0 && ag->target<SCPS_MAX_REG){
                wl->years_held[ag->target] += 25.f;
                wl->L[ag->target] = clampf(wl->L[ag->target]+2.f, 0.f, 10.f);
            }
            break;
        default: break;
    }
    (void)w;
}

/* ======================================================================= */
void statecraft_tick(Statecraft *sc, World *w, WorldEconomy *econ,
                     WorldProsperity *wp, WorldLegitimacy *wl,
                     DiploState *diplo, RouteNetwork *rn, int days){
    int NC = w->n_countries;
    float fd = (float)days;

    /* ---- Influence → standing (prospérité + taille + prestige) ---------- */
    for (int c=0;c<NC;c++){
        sc->prestige[c] = clampf(sc->prestige[c] - SC_PRESTIGE_DECAY*fd, 0.f, 30.f);
        float prosp = (c<wp->n_countries) ? clampf(wp->country[c].P_realise,0.f,10.f) : 0.f;
        float size  = (float)country_size(econ, c);
        float standing = clampf(prosp*4.f + size*3.f + sc->prestige[c], 0.f, 100.f);
        sc->influence[c] = clampf(toward(sc->influence[c], standing, SC_INFLUENCE_RATE*fd), 0.f, 100.f);
        sc->staff[c].count = staff_cap(sc, c);
    }

    /* ---- Opinion (#26) → MODIFICATEURS DE STATUT (temporaires) + MÉMOIRE d'actes (durable) ----
     * Les relations TENDENT VERS 0 (decay naturelle) : un statut ACTIF (alliance/guerre/vassalité/
     * pacte/embargo) tire l'opinion TANT QU'IL TIENT ; à la RUPTURE, le modificateur DISPARAÎT →
     * l'opinion retombe vers 0. La TRAHISON laisse une mémoire DURABLE (opinion_mem) qui s'estompe
     * sur des années. La STRUCTURE (kinship/complément) reste dans diplo_relation (le « avec qui »).
     * O(n²) au pas MENSUEL (diplo rep tous les mois) → coût tenu même à 50+ pays. */
    { float memdecay = tune_f("OPINION_MEM_DECAY",0.0003f)*fd;
      float oally=tune_f("OPINION_ALLY",50.f),   owar=tune_f("OPINION_WAR",60.f),
            ovas=tune_f("OPINION_VASSAL",30.f),   opact=tune_f("OPINION_PACT",15.f),
            oemb=tune_f("OPINION_EMBARGO",25.f),  orw=tune_f("OPINION_RANCOR_W",8.f),
            omcap=tune_f("OPINION_MEM_CAP",100.f);
      for (int a=0;a<NC;a++) for (int b=0;b<NC;b++){
        if (a==b) continue;
        sc->opinion_mem[a][b] = clampf(sc->opinion_mem[a][b]*(1.f-memdecay), -omcap, omcap); /* la mémoire → 0 */
        float tgt = sc->opinion_mem[a][b];               /* socle = ce dont `a` se SOUVIENT de `b` */
        if (diplo){
            DiploStatus st = diplo_status(diplo,a,b);
            if      (st==DIPLO_ALLIED) tgt += oally;     /* +50 TANT QUE l'alliance tient */
            else if (st==DIPLO_WAR)    tgt -= owar;      /* la guerre crève l'opinion (transient) */
            if (diplo_suzerain(diplo,a)==b || diplo_suzerain(diplo,b)==a) tgt += ovas;   /* lien de vassalité */
            if (diplo_trade_pact(diplo,a,b)) tgt += opact;
            tgt -= orw * diplo_rancor(diplo,a,b);        /* la RIVALITÉ territoriale (décroît déjà) */
        }
        if (intertrade_embargoed(a,b)) tgt -= oemb;      /* l'embargo, TANT QU'il dure */
        tgt = clampf(tgt, -100.f, 100.f);
        sc->opinion[a][b] = clampf(toward(sc->opinion[a][b], tgt, SC_OPINION_RATE*fd), -100.f, 100.f);
      }
    }

    /* ---- Diplomates : avancer, appliquer à l'échéance ------------------- */
    for (int c=0;c<NC;c++){
        DiplomaticStaff *st=&sc->staff[c];
        for (int i=0;i<SC_MAX_DIPLOMATS;i++){
            Diplomat *ag=&st->agents[i];
            if (ag->mission==DIP_IDLE) continue;
            ag->days_left -= days;
            if (ag->days_left<=0){
                mission_complete(sc, w, econ, wl, diplo, rn, c, ag);
                ag->mission=DIP_IDLE; ag->target=-1; ag->days_left=0;
            }
        }
    }

    /* ---- Agitation soutenue → révolte ---------------------------------- */
    for (int r=0;r<econ->n_regions;r++){
        sc->revolt_fired[r]=false;
        RegionEconomy *re=&econ->region[r];
        int owner=re->owner;
        if (owner<0 || !re->culture.settled){ sc->agitation[r]=0.f; sc->unrest_days[r]=0.f; continue; }

        const PopCulture *rul=ruling_culture(w,econ,owner);
        float L_local = (r<SCPS_MAX_REG) ? wl->L[r] : 5.f;
        float div_tension = rul ? pc_dist(&re->culture, rul) : 0.f;
        float yh = (r<SCPS_MAX_REG) ? wl->years_held[r] : 50.f;
        float shock = (yh<5.f) ? (1.f - yh/5.f) : 0.f;
        if (re->coercion>shock) shock=re->coercion;
        int cstab = (owner<wp->n_countries) ? metric_stability(wp->country[owner].SI,0.f) : 50;
        int agit  = metric_agitation(L_local, re->coercion, div_tension, shock,
                                     cstab, re->build.H_coerc);

        sc->agitation[r] = clampf(toward(sc->agitation[r], (float)agit, SC_AGIT_RATE*fd), 0.f, 100.f);

        if (sc->agitation[r] >= AGIT_REVOLT_SEUIL) sc->unrest_days[r] += fd;
        else                                       sc->unrest_days[r]  = 0.f;

        if (sc->unrest_days[r] >= REVOLT_SUSTAIN_DAYS){
            /* RÉVOLTE : le consentement s'effondre, loi martiale, le trône perd
             * la face (l'effet existant : L↓ → fracture↑ dans le moteur d'ordre). */
            sc->revolt_fired[r]=true;
            if (r<SCPS_MAX_REG){ wl->L[r] *= 0.40f; }
            re->coercion = 1.f;
            sc->prestige[owner]  = clampf(sc->prestige[owner]-6.f, 0.f, 30.f);
            sc->influence[owner] = clampf(sc->influence[owner]-8.f, 0.f, 100.f);
            sc->unrest_days[r] = 0.f;
            sc->agitation[r]  *= 0.5f;                  /* la colère se vide dans l'émeute */
        }
    }
    (void)wl;
}
