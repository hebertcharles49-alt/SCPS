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
        for (int s=0;s<SC_COUNCIL_SEATS;s++) sc->council[c][s]=-1;
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
int statecraft_council_cand_tier(uint32_t seed, int cid, int seat, int slot){
    uint32_t h = sc_hash(seed^0xC0FFEEu, (uint32_t)cid, (uint32_t)seat, (uint32_t)slot) % 100u;
    return (h<55)?1:(h<85)?2:3;   /* 55 % tier1 · 30 % tier2 · 15 % tier3 (les grands sont rares) */
}
int statecraft_council_cand_name(uint32_t seed, int cid, int seat, int slot){
    uint32_t h = sc_hash(seed^0x5EAB011u, (uint32_t)cid, (uint32_t)(seat*7+slot), 0x9E37u);
    return (int)STR_COUNCIL_NAME_0 + (int)(h % (uint32_t)SC_COUNCIL_NAMES);
}
int statecraft_council_seated(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return -1;
    int s=sc->council[cid][seat];
    return (s>=0 && s<SC_COUNCIL_CANDS) ? s : -1;
}
void statecraft_council_hire(Statecraft *sc, int cid, int seat, int slot){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    if (slot<0||slot>=SC_COUNCIL_CANDS) return;
    sc->council[cid][seat]=(int8_t)slot;
}
void statecraft_council_dismiss(Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    sc->council[cid][seat]=-1;
}
float statecraft_council_seat_mult(const Statecraft *sc, uint32_t seed, int cid, int seat){
    int slot=statecraft_council_seated(sc,cid,seat);
    if (slot<0) return 1.f;
    int tier=statecraft_council_cand_tier(seed,cid,seat,slot);
    return 1.f + SC_SEAT_BASE[seat]*SC_TIER_EFFET[tier];
}
float statecraft_council_cost(const Statecraft *sc, uint32_t seed, int cid, float ipm){
    if (ipm<=0.f) ipm=1.f;
    float tot=0.f;
    for (int s=0;s<SC_COUNCIL_SEATS;s++){
        int slot=statecraft_council_seated(sc,cid,s);
        if (slot<0) continue;
        tot += SC_TIER_COST[ statecraft_council_cand_tier(seed,cid,s,slot) ] * ipm;
    }
    return tot;
}
float statecraft_council_cand_cost(uint32_t seed, int cid, int seat, int slot, float ipm){
    if (ipm<=0.f) ipm=1.f;
    if (cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS||slot<0||slot>=SC_COUNCIL_CANDS) return 0.f;
    return SC_TIER_COST[ statecraft_council_cand_tier(seed,cid,seat,slot) ] * ipm;
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
        if (cr>=0 && cr<e->n_regions){ e->region[cr].treasury -= cost; econ_flux_add(c, FX_CONSEIL, -cost); }
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
void statecraft_council_ai(Statecraft *sc, const World *w, const WorldEconomy *e, uint32_t seed, int cid){
    if (!sc||!w||!e||cid<0||cid>=SCPS_MAX_COUNTRY) return;
    int cap=w->country[cid].capital_prov;
    int cr =(cap>=0&&cap<w->n_provinces)?w->province[cap].region:-1;
    if (cr<0||cr>=e->n_regions || !e->region[cr].culture.settled) return;
    int seat=sc_ethos_seat((int)e->region[cr].culture.ethos);
    if (statecraft_council_seated(sc,cid,seat)>=0) return;                 /* déjà pourvu */
    float tres=e->region[cr].treasury, ipm=econ_world_ipm(e);
    int best=-1, bestt=0;
    for (int slot=0; slot<SC_COUNCIL_CANDS; slot++){
        int t=statecraft_council_cand_tier(seed,cid,seat,slot);
        if (SC_TIER_COST[t]*ipm*6.f > tres) continue;                     /* hors garde de budget (6 mois) */
        if (t>bestt){ bestt=t; best=slot; }
    }
    if (best>=0) statecraft_council_hire(sc,cid,seat,best);
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
    /* La parole rompue crève l'opinion que les AUTRES ont du traître. */
    for (int b=0;b<sc->n_countries;b++) if (b!=cid)
        sc->opinion[b][cid] = clampf(sc->opinion[b][cid]-35.f, -100.f, 100.f);
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

    /* ---- Opinion → relation (lecteurs) + statut de guerre, avec inertie ----
     * O(n²) : appelé au pas MENSUEL par la boucle (diplo rep tous les mois, pas
     * tous les jours) → coût tenu même à 50+ pays. */
    for (int a=0;a<NC;a++) for (int b=0;b<NC;b++){
        if (a==b) continue;
        Relation rel = diplo_relation(w, econ, wp, diplo, a, b);
        float base = (3.5f - rel.kinship) * 10.f;          /* même sphère +35 … étranger −35 */
        float thr  = clampf(rel.threat*2.f, 0.f, 40.f);
        float tgt  = base + rel.complement*30.f - thr - rel.schism*40.f;
        if (diplo && diplo_status(diplo,a,b)==DIPLO_WAR)   tgt -= 60.f;   /* la guerre crève l'opinion */
        if (diplo && diplo_status(diplo,a,b)==DIPLO_ALLIED)tgt += 25.f;
        tgt = clampf(tgt, -100.f, 100.f);
        sc->opinion[a][b] = clampf(toward(sc->opinion[a][b], tgt, SC_OPINION_RATE*fd), -100.f, 100.f);
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
