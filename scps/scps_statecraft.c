/*
 * scps_statecraft.c — Influence, Opinion, Diplomates & Agitation (voir .h)
 *
 * Tout est ancré : l'Influence SUIT la prospérité/taille/prestige réels ;
 * l'Opinion PROJETTE les lecteurs de relation + l'historique ; une mission
 * OCCUPE un agent en jours puis agit par la couche d'action ; l'AGITATION
 * (0-100, lissée depuis L/coercion/choc de conquête/stabilité/H) est un pur
 * SIGNAL — dédup Option B : ce module ne fait plus fire de révolte ni ne mute
 * L/coercion/prestige/influence sur agitation soutenue. scps_revolt.c (le
 * module INCARNÉ) est le SEUL acteur de révolte ; il lit `statecraft_agitation`
 * comme un grief politique SUPPLÉMENTAIRE dans son propre allumage.
 */
#include "scps_statecraft.h"
#include "scps_readout.h"   /* metric_agitation / metric_stability (réutilisés) */
#include "scps_lang.h"      /* Q1 : STR_COUNCIL_NAME_* (noms de candidats) */
#include "scps_culture.h"   /* Q1 : ETHOS_* (l'IA recrute selon l'éthos) */
#include "scps_tune.h"      /* #26 : tunables OPINION_* */
#include "scps_intertrade.h"/* #26 : intertrade_embargoed (la mémoire de l'embargo) */
#include "scps_provlog.h"   /* le JOURNAL diplomatique (trahison/sécession/relations, display) */
#include "scps_math.h"      /* clampf/iclamp/absf partagés */
#include "scps_heritage.h"  /* TRADITIONS : le levier INFLUENCE entre dans le standing */
#include <string.h>

/* ---- Calibrage --------------------------------------------------------- */
#define SC_INFLUENCE_RATE   0.010f   /* vitesse de convergence vers le standing /jour */
#define SC_OPINION_RATE     0.006f   /* l'opinion bouge lentement (l'histoire colle)  */
#define SC_PRESTIGE_DECAY   0.010f   /* le prestige s'érode sans entretien /jour       */
#define SC_AGIT_RATE        0.020f   /* lissage de l'agitation soutenue /jour          */
#define DIP_PER_INFLUENCE   25       /* +1 diplomate par 25 d'Influence                 */

static uint32_t sc_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d);   /* fwd : utilisé par statecraft_init (jitter de loyauté) */

static inline float toward(float cur,float tgt,float k){ return cur + (tgt-cur)*clampf(k,0.f,1.f); }
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

/* V2a — télémétrie (chronique) : combien de fois l'IA a REMPLACÉ un ministre au
 * bord (betrayal_ready). Compteur GLOBAL du module, RAZ par statecraft_init
 * (comme g_civilwars/g_rebel_victories de scps_revolt.c). */
static long g_council_ai_replace = 0;
long statecraft_council_ai_replace_count(void){ return g_council_ai_replace; }

/* ======================================================================= */
void statecraft_init(Statecraft *sc, const World *w){
    memset(sc, 0, sizeof(*sc));
    g_council_ai_replace = 0;
    sc->n_countries = w->n_countries;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)                /* Q1 : memset→0 = slot 0 valide : tous VACANTS (-1) */
        for (int s=0;s<SC_COUNCIL_SEATS;s++){
            sc->council[c][s]=-1; sc->council_gen[c][s]=-1;
            sc->pay[c][s]=1.f;                          /* V2a : paie NORMALE par défaut */
            /* Loyauté de DÉPART ~50 + jitter DÉTERMINISTE (siège, pays) — jamais un
             * ministre tout neuf parfaitement identique d'un pays à l'autre. */
            uint32_t h = sc_hash((uint32_t)(w?w->seed:0u)^0x10AD17Bu, (uint32_t)c, (uint32_t)s, 0x50u);
            sc->loyalty[c][s] = 45.f + (float)(h % 21u);   /* 45..65 */
        }
    for (int c=0;c<w->n_countries;c++){
        sc->influence[c]   = 35.f;             /* une réputation initiale modeste */
        sc->prestige[c]    = 8.f;
        sc->staff[c].count = SC_BASE_DIPLOMATS;
        for (int b=0;b<w->n_countries;b++) sc->opinion[c][b]=0.f;
    }
}

/* ═══ Q1 — LE CONSEIL (I7) ═══════════════════════════════════════════════════
 * 3 sièges (Savoir/Royaume/Ouvrages). Chaque siège propose SC_COUNCIL_CANDS
 * candidats DÉTERMINISTES (tier + nom dérivés du seed) ; le joueur/IA en NOMME un
 * (council[cid][seat] = slot) ou RENVOIE (-1). L'effet est un MULTIPLICATEUR lecteur
 * (×savoir / ×promo / ×manuf), le coût une ponction mensuelle (×IPM). Rien d'autre. */
/* P1 — RANGS : bonus de rang I = BASE(siège), II ×TIER2_MULT, III ×TIER3_MULT
 * (registre J, scps_tune_list.h). base_of() lit le tune_f par SIÈGE (Savoir/
 * Royaume/Ouvrages) — jamais un tableau statique, pour rester F10-dialable. */
static float sc_seat_base(int seat){
    switch (seat){
        case 0:  return tune_f("COUNCIL_SAVOIR_BASE",   0.12f);
        case 1:  return tune_f("COUNCIL_ROYAUME_BASE",  0.15f);
        default: return tune_f("COUNCIL_OUVRAGES_BASE", 0.20f);
    }
}
static float sc_tier_mult(int tier){
    switch (tier){
        case 1:  return 1.f;
        case 2:  return tune_f("COUNCIL_TIER2_MULT", 1.50f);
        case 3:  return tune_f("COUNCIL_TIER3_MULT", 2.00f);
        default: return 0.f;
    }
}
/* P1 — COÛTS : assiette = econ_country_tax_year(cid) (le revenu ANNUEL RÉEL, pas
 * une approximation) × taux par rang × IPM, PRÉLEVÉ /12 (mensuel) — remplace
 * l'ancien prix nominal plat (SC_TIER_COST). */
static float sc_tier_revenue_rate(int tier){
    switch (tier){
        case 1:  return tune_f("COUNCIL_TIER1_REVENUE_RATE", 0.015f);
        case 2:  return tune_f("COUNCIL_TIER2_REVENUE_RATE", 0.030f);
        case 3:  return tune_f("COUNCIL_TIER3_REVENUE_RATE", 0.050f);
        default: return 0.f;
    }
}
static float sc_tier_monthly_cost(int cid, int tier, float ipm){
    if (tier<1||tier>3) return 0.f;
    if (ipm<=0.f) ipm=1.f;
    return econ_country_tax_year(cid) * sc_tier_revenue_rate(tier) * ipm / 12.f;
}

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
/* P0-4 — PERSONNE + MAISON (docs/CONSEIL_ORIENTATIONS_2026-07-10.md). Les 24
 * prénoms et les 12 maisons de la spec, VERBATIM ; tirages INDÉPENDANTS (salts
 * propres) — ni la maison ni le prénom ne dépendent l'un de l'autre, du tier,
 * de l'âge ou de la faction. Tables LOCALES (pas de STR_* : strings_ids.h est
 * hors périmètre de cette mission — cf. TROUVAILLES.md). */
static const char *const SC_FIRSTNAMES[SC_COUNCIL_FIRSTNAMES] = {
    "Aldren","Corven","Edras","Isarn","Maëlor","Odran","Orsan","Séverac",
    "Solvar","Tévran","Vaudric","Ysarn","Althéa","Aveline","Ilyne","Isolde",
    "Maëra","Mirenne","Néris","Oriane","Téliane","Ysilde","Zélie","Vésane",
};
static const char *const SC_HOUSES[SC_COUNCIL_HOUSES] = {
    "Vœrn","Aldric","Harmel","Orlec","Tessari","Velmor","Brask","Dovric",
    "Sarnel","Corvane","Istrane","Vaulserre",
};
const char *statecraft_council_cand_firstname(uint32_t seed, int cid, int seat, int slot, int gen){
    if (seat<0||seat>=SC_COUNCIL_SEATS) return SC_FIRSTNAMES[0];
    uint32_t h = sc_hash(sc_genseed(seed,gen)^0x91A2E3u, (uint32_t)cid, (uint32_t)(seat*7+slot), 0xB4C1u);
    return SC_FIRSTNAMES[h % (uint32_t)SC_COUNCIL_FIRSTNAMES];
}
const char *statecraft_council_cand_house(uint32_t seed, int cid, int seat, int slot, int gen){
    if (seat<0||seat>=SC_COUNCIL_SEATS) return SC_HOUSES[0];
    uint32_t h = sc_hash(sc_genseed(seed,gen)^0x40C51Eu, (uint32_t)cid, (uint32_t)(seat*7+slot), 0xD00D5Eu);
    return SC_HOUSES[h % (uint32_t)SC_COUNCIL_HOUSES];
}
/* P0-1 — LA FACTION D'UN CANDIDAT : plus de spectre par siège — un mélange
 * DÉTERMINISTE des 6 factions (Fisher-Yates, xs32 amorcé par un hash de
 * seed×pays×siège×GÉNÉRATION) ; les SC_COUNCIL_CANDS premières du mélange sont
 * les factions des 3 candidats du siège — TOUJOURS distinctes (préfixe d'une
 * permutation), et le mélange change à chaque génération (re-tirage). Rien à
 * sérialiser (dérivée, comme le tier/l'âge). */
static void sc_faction_shuffle(uint32_t seed, int cid, int seat, int gen, EthosFaction out[FAC_COUNT]){
    for (int f=0; f<FAC_COUNT; f++) out[f]=(EthosFaction)f;
    uint32_t rng = sc_hash(sc_genseed(seed,gen)^0xFAC7104u, (uint32_t)cid, (uint32_t)seat, 0x51AFu);
    if (!rng) rng = 1u;
    for (int i=FAC_COUNT-1; i>0; i--){
        uint32_t r = xs32(&rng);
        int j = (int)(r % (uint32_t)(i+1));
        EthosFaction t=out[i]; out[i]=out[j]; out[j]=t;
    }
}
EthosFaction statecraft_council_faction(uint32_t seed, int cid, int seat, int slot, int gen){
    if (seat<0||seat>=SC_COUNCIL_SEATS) return FAC_COMMUNAUTAIRE;
    if (slot<0) slot=0;
    if (slot>=FAC_COUNT) slot=FAC_COUNT-1;
    EthosFaction perm[FAC_COUNT];
    sc_faction_shuffle(seed,cid,seat,gen,perm);
    return perm[slot];
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
/* P2 — LA FACTION RÉELLE DU TITULAIRE D'UN SIÈGE (docs/CONSEIL_ORIENTATIONS_2026-07-10.md
 * §« Événements Conseil : hooks DYNAMIQUES ») : compose seated+seated_gen+council_faction —
 * -1 si le siège est VACANT (jamais une faction par défaut qui laisserait croire à un
 * titulaire fictif — statecraft_council_faction, elle, renvoie toujours une faction valide
 * même hors-borne : ce lecteur est le SEUL qui sache dire "personne n'est assis là"). */
int statecraft_council_seat_faction(const Statecraft *sc, uint32_t seed, int cid, int seat){
    int slot = statecraft_council_seated(sc, cid, seat);
    if (slot<0) return -1;
    int gen = statecraft_council_seated_gen(sc, cid, seat);
    return (int)statecraft_council_faction(seed, cid, seat, slot, gen);
}
void statecraft_council_hire(Statecraft *sc, uint32_t seed, int cid, int seat, int slot, int gen){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    if (slot<0||slot>=SC_COUNCIL_CANDS) return;
    /* P1-4 — une NOMINATION n'écrase JAMAIS un titulaire sans renvoi explicite :
     * le siège doit être vacant (RENVOYER d'abord, statecraft_council_dismiss). */
    if (statecraft_council_seated(sc,cid,seat)>=0) return;
    sc->council[cid][seat]=(int8_t)slot;
    sc->council_gen[cid][seat]=(int8_t)((gen>=0 && gen<=120) ? gen : 0);   /* identité ÉPINGLÉE au moment de l'embauche */
    /* V2a : loyauté de DÉPART ~50 + jitter (même motif que statecraft_init), le
     * ministre n'entre pas en poste déjà aimé ou détesté. */
    uint32_t h = sc_hash(seed^0x10AD17Bu, (uint32_t)cid, (uint32_t)seat, (uint32_t)(slot*13+gen));
    sc->loyalty[cid][seat] = 45.f + (float)(h % 21u);
    sc->pay[cid][seat] = 1.f;                                              /* paie NORMALE au départ */
    /* RECRUTER = un vote pour SA faction (le motif des leviers, §4) : elle gagne
     * du poids effectif, ses opposées s'aigrissent (faction_lever_apply le fait
     * déjà, table d'opposition hardcodée). */
    EthosFaction fac = statecraft_council_faction(seed, cid, seat, slot, gen);
    faction_lever_apply(cid, fac, tune_f("COUNCIL_HIRE_LEVER",0.10f));
}
void statecraft_council_dismiss(Statecraft *sc, uint32_t seed, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    /* P1-3 — RENVOYER aigrit DIRECTEMENT la faction CONGÉDIÉE (elle perd son
     * siège) : plus de push artificiel sur la faction la plus opposée à elle
     * (l'ancien motif « pousser l'opposée » gagnait un pouvoir qu'un renvoi ne
     * devrait pas donner à un tiers). */
    int slot = statecraft_council_seated(sc, cid, seat);
    if (slot>=0){
        int gen = statecraft_council_seated_gen(sc, cid, seat);
        EthosFaction fac = statecraft_council_faction(seed, cid, seat, slot, gen);
        faction_grievance_add(cid, fac, tune_f("COUNCIL_DISMISS_GRIEF",0.10f));
    }
    sc->council[cid][seat]=-1;
    sc->council_gen[cid][seat]=-1;
    sc->loyalty[cid][seat]=0.f;      /* le siège est vacant : la loyauté n'a plus de sens (repose au prochain hire) */
    sc->pay[cid][seat]=1.f;
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
                >= sc_retire_age(seed,c,seat,slot,gen)){
                /* La RETRAITE n'est pas un RENVOI (pas de grief joueur) : reset direct. */
                sc->council[c][seat]=-1; sc->council_gen[c][seat]=-1;
                sc->loyalty[c][seat]=0.f; sc->pay[c][seat]=1.f;
            }
        }
}
/* P1 — bonus de RANG (1 + base(siège)·tier_mult) — SEUL, jamais l'efficacité (elle
 * multiplie l'INCRÉMENT, pas ce lecteur : voir statecraft_council_apply). */
float statecraft_council_seat_mult(const Statecraft *sc, uint32_t seed, int cid, int seat){
    int slot=statecraft_council_seated(sc,cid,seat);
    if (slot<0) return 1.f;
    int tier=statecraft_council_cand_tier(seed,cid,seat,slot,statecraft_council_seated_gen(sc,cid,seat));
    return 1.f + sc_seat_base(seat)*sc_tier_mult(tier);
}
/* P1 — COÛT MENSUEL total (×IPM, ×paie) — assiette = revenu ANNUEL RÉEL du pays. */
float statecraft_council_cost(const Statecraft *sc, uint32_t seed, int cid, float ipm){
    float tot=0.f;
    for (int s=0;s<SC_COUNCIL_SEATS;s++){
        int slot=statecraft_council_seated(sc,cid,s);
        if (slot<0) continue;
        float pay = statecraft_council_pay(sc,cid,s);                     /* V2a : le curseur multiplie le coût */
        int tier  = statecraft_council_cand_tier(seed,cid,s,slot,statecraft_council_seated_gen(sc,cid,s));
        tot += sc_tier_monthly_cost(cid, tier, ipm) * pay;
    }
    return tot;
}
/* P1 — coût MENSUEL d'UN candidat (×IPM), pour l'UI (preview avant nomination). */
float statecraft_council_cand_cost(uint32_t seed, int cid, int seat, int slot, int gen, float ipm){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS||slot<0||slot>=SC_COUNCIL_CANDS) return 0.f;
    int tier = statecraft_council_cand_tier(seed,cid,seat,slot,gen);
    return sc_tier_monthly_cost(cid, tier, ipm);
}
/* ═══ V2a — LE CONSEIL VIVANT : la LOYAUTÉ (le cœur) ══════════════════════════
 * Chaque siège pourvu porte une loyauté 0-100 qui CONVERGE (jamais un saut) vers
 * une cible dérivée de la satisfaction de SA faction (1−grief), de la PAIE et du
 * ROT (la capture d'État — faction_capture_total). L'asymétrie du motif
 * COERCION_DECAY : le taux de convergence est PLUS RAPIDE quand la loyauté
 * CHUTE (le rot y aide), plus LENT quand elle REMONTE (le rot ne restaure rien —
 * la corruption aide à tomber, jamais à se refaire une vertu). */
int statecraft_council_loyalty(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return 0;
    if (statecraft_council_seated(sc,cid,seat)<0) return 0;
    return iclamp((int)(sc->loyalty[cid][seat]+0.5f), 0, 100);
}
float statecraft_council_pay(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return 1.f;
    float p = sc->pay[cid][seat];
    return (p>0.f) ? clampf(p, 0.f, 2.f) : 1.f;                            /* legacy (0 = jamais posé) → normal */
}
void statecraft_council_set_pay(Statecraft *sc, int cid, int seat, float pay){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    sc->pay[cid][seat] = clampf(pay, 0.f, 2.f);
}
/* P3 — écrivain DIRECT de loyauté (mission décennale : réussite/échec du siège
 * responsable). No-op si le siège est VACANT (personne à créditer/blâmer). */
void statecraft_council_loyalty_add(Statecraft *sc, int cid, int seat, float delta){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return;
    if (statecraft_council_seated(sc,cid,seat)<0) return;
    sc->loyalty[cid][seat] = clampf(sc->loyalty[cid][seat] + delta, 0.f, 100.f);
}
/* P1-1 — EFFICACITÉ POLITIQUE : clamp(BASE + K_PER·K + LOY_W·loyauté/100 −
 * CORRUPTION_PER_POINT·Corruption, MIN, MAX). LIT wp->country[cid].K (jamais une
 * approximation depuis les bâtiments) ; 1.0 si le siège est VACANT (rien à
 * multiplier) ; `wp` NULL ou hors-borne ⇒ K=0 (dégrade proprement). */
float statecraft_council_efficiency(const Statecraft *sc, const WorldProsperity *wp, int cid, int seat){
    if (statecraft_council_seated(sc,cid,seat)<0) return 1.f;
    float K = (wp && cid>=0 && cid<wp->n_countries) ? wp->country[cid].K : 0.f;
    float loy  = (float)statecraft_council_loyalty(sc,cid,seat);
    float corr = (float)faction_corruption_0_100(cid);
    float eff = tune_f("COUNCIL_EFF_BASE", 0.70f)
              + tune_f("COUNCIL_EFF_K_PER", 0.03f) * K
              + tune_f("COUNCIL_EFF_LOY_W", 0.15f) * (loy/100.f)
              - tune_f("COUNCIL_EFF_CORRUPTION_PER_POINT", 0.0035f) * corr;
    return clampf(eff, tune_f("COUNCIL_EFF_MIN",0.50f), tune_f("COUNCIL_EFF_MAX",1.15f));
}
/* « au bord de la trahison » : loyauté ≤ seuil — un simple lecteur de l'état
 * COURANT (la loyauté converge lentement, demi-vie de plusieurs mois : par
 * construction, si elle EST sous le seuil elle y est depuis un moment — pas
 * besoin d'un compteur de mois séparé à sérialiser). */
bool statecraft_council_betrayal_ready(const Statecraft *sc, int cid, int seat){
    if (!sc||cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=SC_COUNCIL_SEATS) return false;
    if (statecraft_council_seated(sc,cid,seat)<0) return false;
    return sc->loyalty[cid][seat] <= tune_f("COUNCIL_BETRAYAL_THRESHOLD",15.f);
}
/* La CIBLE de loyauté d'un siège pourvu : satisfaction de SA faction (1−grief,
 * dans [0,1]) × 100, modulée par la PAIE (payer plus achète de la loyauté,
 * payer moins en coûte) — jamais un +X plat, toujours ancré sur le grief réel. */
static float council_loyalty_target(const Statecraft *sc, int cid, int seat, uint32_t seed){
    int slot=statecraft_council_seated(sc,cid,seat);
    if (slot<0) return 50.f;
    int gen=statecraft_council_seated_gen(sc,cid,seat);
    EthosFaction fac = statecraft_council_faction(seed,cid,seat,slot,gen);
    float grief = faction_grievance(cid, fac);                            /* 0..1 : la rancœur de SA faction */
    float base  = (1.f - grief) * 100.f;                                  /* satisfaite → loyale */
    float pay   = statecraft_council_pay(sc,cid,seat);
    float pay_adj = (pay-1.f) * tune_f("COUNCIL_PAY_ADJ",30.f);           /* 0×→ -30 · 1×→ 0 · 2×→ +30 */
    return clampf(base + pay_adj, 0.f, 100.f);
}
void statecraft_council_loyalty_tick(Statecraft *sc, const World *w, const WorldEconomy *econ,
                                     uint32_t seed, float dt_year){
    if (!sc||!w) return;
    (void)econ;
    float base_rate = tune_f("COUNCIL_LOYAL_RATE",0.05f);                 /* vitesse de convergence de base /mois */
    float rot_boost = tune_f("COUNCIL_ROT_BOOST",1.5f);                   /* le rot ACCÉLÈRE la chute (× additionnel) */
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float rot = faction_capture_total(c);                             /* 0..CAPTURE_MAX(0.85) */
        for (int s=0;s<SC_COUNCIL_SEATS;s++){
            if (statecraft_council_seated(sc,c,s)<0) continue;            /* vacant : rien à faire converger */
            float tgt = council_loyalty_target(sc,c,s,seed);
            float cur = sc->loyalty[c][s];
            /* Asymétrie du rot (motif COERCION_DECAY) : le rot ACCÉLÈRE la chute,
             * jamais la remontée — la corruption aide à tomber, pas à se refaire. */
            float rate = base_rate * (1.f + ((tgt<cur) ? rot_boost*rot : 0.f));
            sc->loyalty[c][s] = clampf(toward(cur, tgt, rate*dt_year*12.f), 0.f, 100.f);
        }
    }
}
/* L'état d'une PAIRE de sièges (V2b s'en sert pour les événements) : RIVALITÉ
 * (factions opposées ≥0.6, tous deux en poste >10 ans) · ALLIANCE (opposition
 * <0.3, grief bas des deux côtés) · CONSPIRATION (les DEUX factions aliénées,
 * grief>0.6 chacune) · NEUTRE sinon. */
CouncilPairState statecraft_council_pair_state(const Statecraft *sc, const World *w, const WorldEconomy *econ,
                                               uint32_t seed, int cid, int a, int b, int year){
    (void)econ;
    if (!sc||!w||cid<0||cid>=SCPS_MAX_COUNTRY||a<0||a>=SC_COUNCIL_SEATS||b<0||b>=SC_COUNCIL_SEATS||a==b)
        return COUNCIL_PAIR_NEUTRE;
    int slotA=statecraft_council_seated(sc,cid,a), slotB=statecraft_council_seated(sc,cid,b);
    if (slotA<0||slotB<0) return COUNCIL_PAIR_NEUTRE;
    int genA=statecraft_council_seated_gen(sc,cid,a), genB=statecraft_council_seated_gen(sc,cid,b);
    EthosFaction facA=statecraft_council_faction(seed,cid,a,slotA,genA);
    EthosFaction facB=statecraft_council_faction(seed,cid,b,slotB,genB);
    float opp = faction_opposition(facA, facB);
    float griefA = faction_grievance(cid, facA), griefB = faction_grievance(cid, facB);
    int ageA=statecraft_council_seated_age(sc,seed,cid,a,year), ageB=statecraft_council_seated_age(sc,seed,cid,b,year);
    int tenureA = ageA - statecraft_council_cand_age(seed,cid,a,slotA,genA,genA*SC_COUNCIL_GEN_YEARS);
    int tenureB = ageB - statecraft_council_cand_age(seed,cid,b,slotB,genB,genB*SC_COUNCIL_GEN_YEARS);
    if (griefA>0.6f && griefB>0.6f) return COUNCIL_PAIR_CONSPIRATION;
    if (opp>=0.6f && tenureA>10 && tenureB>10) return COUNCIL_PAIR_RIVALITE;
    if (opp<0.3f && griefA<0.3f && griefB<0.3f) return COUNCIL_PAIR_ALLIANCE;
    return COUNCIL_PAIR_NEUTRE;
}

void statecraft_council_apply(const Statecraft *sc, const World *w, WorldEconomy *e,
                              const WorldProsperity *wp, uint32_t seed, float dt_year){
    if (!sc||!w||!e) return;
    float ipm = econ_world_ipm(e);
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++){
        for (int s=0;s<SC_COUNCIL_SEATS;s++){
            /* P1-1 — bonus final du siège = bonus de RANG × EFFICACITÉ (l'efficacité
             * multiplie SEULEMENT la part conseiller, jamais un multiplicateur brut). */
            float rank_mult = statecraft_council_seat_mult(sc,seed,c,s);
            float eff       = statecraft_council_efficiency(sc,wp,c,s);
            float final_mult = 1.f + (rank_mult - 1.f) * eff;
            econ_set_council_mult(c, s, final_mult);   /* LECTEUR : ×savoir/×promo/×manuf */
        }
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
    float tres=e->region[cr].treasury, ipm=econ_world_ipm(e);
    int gen=statecraft_council_gen(year);                                  /* la pool COURANTE (toujours 3 candidats) */
    /* V2a — l'IA PAIE la paie par défaut (1.0, posé à l'embauche) et ne subit
     * pas la trahison narrative (réservée au joueur, V2b) : un ministre AU BORD
     * (loyauté ≤ seuil depuis longtemps) est REMPLACÉ plutôt que gardé jusqu'au
     * bout. Renvoi PUIS re-nomination au même siège dans le même tick. */
    if (statecraft_council_seated(sc,cid,seat)>=0){
        if (statecraft_council_betrayal_ready(sc,cid,seat)){
            statecraft_council_dismiss(sc,seed,cid,seat);
            g_council_ai_replace++;
        } else
            return;                                                       /* déjà pourvu et loyal : rien à faire */
    }
    int best=-1, bestt=0;
    for (int slot=0; slot<SC_COUNCIL_CANDS; slot++){
        int t=statecraft_council_cand_tier(seed,cid,seat,slot,gen);
        if (sc_tier_monthly_cost(cid,t,ipm)*6.f > tres) continue;         /* hors garde de budget (6 mois) */
        if (t>bestt){ bestt=t; best=slot; }
    }
    if (best>=0) statecraft_council_hire(sc,seed,cid,seat,best,gen);
}

/* ---- Lecteurs ---------------------------------------------------------- */
/* TRADITIONS — le levier INFLUENCE (Charismatique/Prosélyte vs Rebutant/Réservé) entre
 * dans le STANDING (l'assiette vers laquelle l'Influence converge), au MÊME RANG que le
 * prestige — PAR PAYS (culture_build_for), jamais un bonus global. ±0.75 levier ×
 * TRAD_INFL_W=10 → ±7.5 pts sur 0..100 (1 diplomate = 25 pts). MIROIR statecraft_tick. */
static float sc_trad_influence(int cid){
    HeritageBuild hb = culture_build_for((uint32_t)cid);
    return tune_f("TRAD_INFL_W", 10.f) * build_leviers(&hb).influence;
}
float statecraft_influence_flux(const Statecraft *sc, const WorldEconomy *econ,
                                const WorldProsperity *wp, int cid){
    if (!sc||!econ||!wp||cid<0||cid>=SCPS_MAX_COUNTRY) return 0.f;
    float prosp = (cid<wp->n_countries)? clampf(wp->country[cid].P_realise,0.f,10.f):0.f;
    float size  = (float)country_size(econ, cid);
    float standing = clampf(prosp*4.f + size*3.f + sc->prestige[cid] + sc_trad_influence(cid), 0.f, 100.f);
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
/* #26 — le RÉSUMÉ : les composantes de la CIBLE d'opinion de `a` envers `b`. MIROIR EXACT
 * du bloc opinion de statecraft_tick (garder les deux synchrones si l'un évolue) — l'UI
 * montre POURQUOI l'opinion est ce qu'elle est (statuts + rancune + mémoire des actes). */
void statecraft_opinion_parts(const Statecraft *sc, const DiploState *diplo,
                              int a, int b, OpinionParts *out){
    if (!out) return;
    memset(out, 0, sizeof *out);
    if (!sc||a<0||a>=sc->n_countries||b<0||b>=sc->n_countries||a==b) return;
    out->mem = sc->opinion_mem[a][b];
    if (diplo){
        DiploStatus st = diplo_status(diplo,a,b);
        if      (st==DIPLO_ALLIED) out->ally =  tune_f("OPINION_ALLY",50.f);
        else if (st==DIPLO_WAR)    out->war  = -tune_f("OPINION_WAR",60.f);
        if (diplo_suzerain(diplo,a)==b || diplo_suzerain(diplo,b)==a) out->vassal = tune_f("OPINION_VASSAL",30.f);
        if (diplo_trade_pact(diplo,a,b)) out->pact = tune_f("OPINION_PACT",15.f);
        out->rancor = -tune_f("OPINION_RANCOR_W",8.f) * diplo_rancor(diplo,a,b);
    }
    if (intertrade_embargoed(a,b)) out->embargo = -tune_f("OPINION_EMBARGO",25.f);
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
    diplog_push(DACT_BETRAYAL, cid, -1, -bet);   /* journal : « a trahi » (b<0 → le pays suivi) */
}
void statecraft_on_secession(Statecraft *sc, int child, int parent){
    /* #26bis — le pays NÉ D'UNE GUERRE CIVILE aime moins l'empire père (Flandre vs France) :
     * même canal que la trahison — mémoire DURABLE (opinion_mem), s'estompe sur des années
     * (OPINION_MEM_DECAY). UN SEUL SENS : c'est le sécessionniste qui porte la plaie.
     * Bornes SCPS_MAX_COUNTRY (pas n_countries) : le fils vient de naître, le compteur
     * statecraft peut encore retarder d'un tick sur le monde. */
    if (!sc||child<0||child>=SCPS_MAX_COUNTRY||parent<0||parent>=SCPS_MAX_COUNTRY||child==parent) return;
    float sec = tune_f("OPINION_MEM_SECESSION",45.f), cap = tune_f("OPINION_MEM_CAP",100.f);
    sc->opinion_mem[child][parent] = clampf(sc->opinion_mem[child][parent]-sec, -cap, cap);
    diplog_push(DACT_SECESSION, child, parent, -sec);   /* journal : « né d'une sécession de » */
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

    int home = world_capital_region(w, cid);
    int days;
    switch (mission){
        case DIP_RELATIONS: days=180; break;
        case DIP_ALLIANCE:  days=30;  break;
        case DIP_ROUTE:     days=90;  break;
        case DIP_INTEGRATE: {
            /* ∝ D∞ : avaler du lointain prend des générations. */
            const PopCulture *rul=econ_ruling_culture(w,econ,cid);
            float d = rul ? econ_content_dist(&econ->region[target].culture, rul) : 5.f;
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
            diplog_push(DACT_RELATIONS, cid, ag->target, 0.f);   /* journal : « a soigné les relations » */
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

    /* ---- Influence → standing (prospérité + taille + prestige + traditions) ---------- */
    for (int c=0;c<NC;c++){
        sc->prestige[c] = clampf(sc->prestige[c] - SC_PRESTIGE_DECAY*fd, 0.f, 30.f);
        float prosp = (c<wp->n_countries) ? clampf(wp->country[c].P_realise,0.f,10.f) : 0.f;
        float size  = (float)country_size(econ, c);
        float standing = clampf(prosp*4.f + size*3.f + sc->prestige[c] + sc_trad_influence(c), 0.f, 100.f);
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

    /* ---- Agitation soutenue (SIGNAL, plus d'allumage indépendant) --------
     * Option B (dédup des deux systèmes de révolte) : statecraft ne fait plus QUE
     * lisser l'agitation 0-100 (le lecteur `statecraft_agitation` — la ligne UI
     * « ⚑ Au bord de la révolte » — reste vivant) et NE MUTE PLUS L/coercion/
     * prestige/influence, ne fait plus fire de révolte. L'INCARNÉ (scps_revolt.c,
     * revolt_scan/_ignite) est désormais le SEUL acteur : il lit cette agitation
     * comme un grief SUPPLÉMENTAIRE (le canal légitimité/coercition/culture que sa
     * misère-de-groupe n'a pas) dans son propre `worst`, en plus de la faim/taxe/
     * aliénation/répression/non-intégration. `revolt_fired`/`unrest_days` restent
     * des CHAMPS de struct (INERTES : jamais accumulés/vrais) pour ne pas bumper
     * SAVE_VERSION (sizeof(Statecraft) inchangé) ; `statecraft_revolt_fired`
     * renverra donc toujours `false`. */
    for (int r=0;r<econ->n_regions;r++){
        sc->revolt_fired[r]=false;
        RegionEconomy *re=&econ->region[r];
        int owner=re->owner;
        if (owner<0 || !re->culture.settled){ sc->agitation[r]=0.f; sc->unrest_days[r]=0.f; continue; }

        const PopCulture *rul=econ_ruling_culture(w,econ,owner);
        float L_local = (r<SCPS_MAX_REG) ? wl->L[r] : 5.f;
        float div_tension = rul ? econ_content_dist(&re->culture, rul) : 0.f;
        float yh = (r<SCPS_MAX_REG) ? wl->years_held[r] : 50.f;
        float shock = (yh<5.f) ? (1.f - yh/5.f) : 0.f;
        if (re->coercion>shock) shock=re->coercion;
        int cstab = (owner<wp->n_countries) ? metric_stability(wp->country[owner].SI,0.f) : 50;
        int agit  = metric_agitation(L_local, re->coercion, div_tension, shock,
                                     cstab, re->build.H_coerc);

        sc->agitation[r] = clampf(toward(sc->agitation[r], (float)agit, SC_AGIT_RATE*fd), 0.f, 100.f);
    }
    (void)wl;
}
