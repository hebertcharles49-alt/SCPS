/*
 * scps_credit.c — DETTE & PRÊTS — M3c : LE CRÉDIT RÉEL. Voir scps_credit.h.
 *
 * Le plafond n'est pas une constante : il ÉMERGE de la capacité à rembourser (taille
 * éco, credit_line — INCHANGÉ). Ce qui change (M3c) : un emprunt DÉPLACE des pièces qui
 * existent déjà (péréquation nationale, puis les PROPRES classes du pays, puis une
 * cité-état solvable) au lieu de laisser le trésor passer "monnaie négative". La dette
 * est un PASSIF SÉPARÉ (CountryDebt), ventilé par créancier ; l'intérêt annuel CRÉDITE
 * ce créancier (la cité-état/l'élite rentière vit enfin de tes intérêts) ; un surplus
 * SUBSTANTIEL amortit le principal ; les cités-états/mercantiles riches peuvent RACHETER
 * la dette-classes à sa valeur faciale (le marché secondaire, les Fugger).
 */
#include "scps_credit.h"
#include "scps_culture.h"
#include "scps_types.h"
#include "scps_tune.h"
#include "scps_math.h"   /* clampf partagé (M3d : formule de taux) */
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define CR_EPS 1e-4f

typedef struct {
    float   to_class;    /* dette due aux PROPRES classes du pays (élites+bourgeois, agrégé) */
    float   to_cs;        /* dette due à LA cité-état créancière (agrégé) */
    int16_t cs_id;         /* pays créancier cité-état, -1 = aucun */
    /* M3d — années CONSÉCUTIVES passées AU PLAFOND (credit_year_tick) : la « chronique »
     * qui déclenche la banqueroute FORCÉE (BANKRUPTCY_GRACE_YEARS). Sérialisée (v90) —
     * inter-ticks (persiste d'une année sur l'autre), motif EMOB/COLC/TXYR. */
    int16_t insolvent_streak;
} CountryDebt;

static CountryDebt g_debt[SCPS_MAX_COUNTRY];
static long g_buybacks=0, g_defaults=0;   /* télémétrie MONDE, RAZ par credit_init (par partie/sim) */
/* M3d — banqueroutes/sim : FORCÉE (chronique, l'IA aussi) vs VOLONTAIRE (CMD_BANKRUPTCY,
 * joueur seul) — RAZ par credit_init (par partie/sim), non sérialisées (télémétrie pure,
 * motif g_buybacks/g_defaults). g_forced_pending est un flag TRANSIENT (posé par
 * credit_year_tick, consommé par scps_sim.c juste après — RAZ à chaque appel). */
static long g_bankrupt_forced=0, g_bankrupt_voluntary=0;
static bool g_forced_pending[SCPS_MAX_COUNTRY];

void credit_init(void){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
        g_forced_pending[c]=false;
    }
    g_buybacks=0; g_defaults=0; g_bankrupt_forced=0; g_bankrupt_voluntary=0;
}
int  credit_of(int c){ return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].cs_id : -1; }
float credit_debt_class(int c)     { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_class : 0.f; }
float credit_debt_citystate(int c) { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_cs    : 0.f; }
float credit_debt_total(int c)     { return credit_debt_class(c)+credit_debt_citystate(c); }
int   credit_insolvent_streak(int c){ return (c>=0&&c<SCPS_MAX_COUNTRY)? (int)g_debt[c].insolvent_streak : 0; }
void credit_stats_get(long *buybacks, long *defaults){
    if (buybacks) *buybacks=g_buybacks;
    if (defaults) *defaults=g_defaults;
}
void credit_bankruptcy_stats(long *forced, long *voluntary){
    if (forced)    *forced=g_bankrupt_forced;
    if (voluntary) *voluntary=g_bankrupt_voluntary;
}
/* M3d — LE PLAFOND (brief §1) : dette max = DEBT_CEILING_YEARS × revenu annuel NOMINAL
 * (econ_country_tax_year — la MEMBRANE DE DÉCISION déjà établie, cf. d_treasury_mois).
 * 0 revenu (bootstrap <90j, cf. econ_country_tax_year) ⇒ plafond 0 : AUCUN emprunt tant que
 * le pays n'a encore rien perçu — cohérent (mesuré/documenté au rapport, pas un bug). */
float credit_debt_ceiling(int c){
    return tune_f("DEBT_CEILING_YEARS", 3.0f) * fmaxf(0.f, econ_country_tax_year(c));
}
/* LE DRAW MAXIMAL d'un pays MAINTENANT — l'intersection du plafond (headroom restant) et
 * de LA TRANCHE (brief §4 : DEBT_TRANCHE_FRAC × revenu annuel, PAR SOURCE — classes et
 * cité-état ont chacune leur propre tranche, motif des capacités déjà indépendantes
 * CLASS_LEND_SHARE/CITYSTATE_LEND_SHARE). Ne mord QUE la dette RÉELLE (credit_borrow_local
 * l'applique APRÈS la péréquation, qui n'est pas un prêt — brief §1 « plus personne ne
 * prête », la péréquation ne prête personne, elle redistribue le pays à lui-même). */
static float debt_draw_cap(int c){
    float ceiling = credit_debt_ceiling(c);
    float room = ceiling - credit_debt_total(c); if (room<0.f) room=0.f;
    float tranche = tune_f("DEBT_TRANCHE_FRAC", 0.20f) * fmaxf(0.f, econ_country_tax_year(c));
    return fminf(room, tranche);
}

static int home_reg(const World *w, int c){
    if(!w||c<0||c>=w->n_countries) return -1;
    int cp=w->country[c].capital_prov;
    return (cp>=0&&cp<w->n_provinces)? w->province[cp].region : -1;
}
static double cpop(const WorldEconomy *e, int c){
    double p=0.0; int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    for(int r=0;r<n;r++) if(e->region[r].owner==c)
        for(int k=0;k<CLASS_COUNT;k++) p+=e->region[r].strata[k].pop;
    return p;
}
/* PLAFOND ÉMERGENT — capacité à rembourser ∝ taille éco (pop). INCHANGÉ par M3c (gate de
 * credit_can_spend seulement — la chaîne d'emprunt RÉELLE, elle, a ses propres capacités
 * par étage, cf. plus bas). La légitimité entre dans le TAUX (credit_year_tick). */
float credit_line(const World *w, const WorldEconomy *e, int c){
    (void)w; return tune_f("CREDIT_LINE_BASE",0.5f) * (float)cpop(e,c);
}
bool credit_can_spend(const WorldEconomy *e, const World *w, int c, float cost){
    /* M3c : la dette EXISTANTE (passif RÉEL désormais, plus une "trésorerie négative"
     * invisible) mange la ligne — un pays déjà endetté a MOINS de marge, pas plus. */
    double room = (double)credit_line(w,e,c) - (double)credit_debt_total(c);
    return econ_country_gold(e,c) - (double)cost >= -room;
}

/* éthos pays = éthos de la culture de sa région-capitale (convention scps_ai.c). */
static Ethos country_ethos(const WorldEconomy *e, const World *w, int c){
    int hr=home_reg(w,c);
    return (hr>=0&&hr<e->n_regions)? e->region[hr].culture.ethos : ETHOS_COUNT;
}
/* PRÊTEUR ÉLIGIBLE : cité-état OU mercantile/pacifiste, ≠ c, avec du surplus RÉEL
 * (au-dessus de SINK_FLOOR — un prêteur ne se saigne pas lui-même). Le plus riche. */
static int pick_lender(const WorldEconomy *e, const World *w, int c, float floor_){
    int best=-1; float bests=0.f;
    for(int k=0;k<w->n_countries && k<SCPS_MAX_COUNTRY;k++){
        if(k==c) continue;
        bool lender=(w->country[k].role==POLITY_CITY_STATE);
        if(!lender){ Ethos et=country_ethos(e,w,k); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE); }
        if(!lender) continue;
        float s=0.f; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
        for(int p=0;p<n;p++) if(e->prov[p].owner==k && e->prov[p].active && e->prov[p].colonized) s += fmaxf(0.f, e->prov[p].treasury - floor_);
        if (s<=0.f) continue;
        if (s>bests){ bests=s; best=k; }
    }
    return best;
}
/* or NET d'un pays lu DIRECTEMENT sur prov[] (Σ) — contrepartie province-fraîche
 * d'econ_country_gold (qui lit region[], un DÉRIVÉ pas encore ré-agrégé juste après
 * une écriture prov[]). */
static double country_gold_prov(const WorldEconomy *e, int c){
    double g=0.0; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c) g+=e->prov[p].treasury;
    return g;
}

/* ---- M3c : LA CHAÎNE D'EMPRUNT ------------------------------------------------------- */

/* Σ surplus (>floor_) des provinces ACTIVES d'un pays — la "caisse nationale" que
 * price_level (scps_econ.c) mesure déjà ; ici on l'ATTEINT réellement (péréquation). */
static float country_surplus(const WorldEconomy *e, int c, float floor_){
    float s=0.f; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c && e->prov[p].active && e->prov[p].colonized)
        s += fmaxf(0.f, e->prov[p].treasury - floor_);
    return s;
}
/* Débite `amount` (<=country_surplus(e,c,floor_)) au PRORATA du surplus de chaque
 * province — aucune ne descend sous floor_ par construction (amount borné par l'appelant). */
static void debit_surplus_prorata(WorldEconomy *e, int c, float floor_, float amount){
    if (amount<=CR_EPS) return;
    float tot=country_surplus(e,c,floor_); if (tot<=CR_EPS) return;
    int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++){
        if (e->prov[p].owner!=c || !e->prov[p].active || !e->prov[p].colonized) continue;
        float s=fmaxf(0.f, e->prov[p].treasury - floor_); if (s<=0.f) continue;
        float share=amount*(s/tot);
        e->prov[p].treasury -= share;
    }
}
/* Σ richesse LENDABLE (élites+bourgeois, pondérées) d'un pays — les laborers n'ont pas
 * d'épargne (brief), poids registre J. */
static void country_lendable(const WorldEconomy *e, int c, float ew, float bw, float *cap_elite, float *cap_bourg){
    float el=0.f, bo=0.f; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++){
        if (e->prov[p].owner!=c || !e->prov[p].active || !e->prov[p].colonized) continue;
        el += e->prov[p].strata[CLASS_ELITE].wealth;
        bo += e->prov[p].strata[CLASS_BOURGEOIS].wealth;
    }
    if (cap_elite) *cap_elite = el*ew;
    if (cap_bourg) *cap_bourg = bo*bw;
}
/* Débite `amount` de la richesse ÉLITE (ou BOURGEOIS) d'un pays, au prorata province par
 * province (même motif que debit_surplus_prorata, sur strata[cls].wealth). */
static void debit_wealth_prorata(WorldEconomy *e, int c, int cls, float amount){
    if (amount<=CR_EPS) return;
    float tot=0.f; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c && e->prov[p].active && e->prov[p].colonized) tot += e->prov[p].strata[cls].wealth;
    if (tot<=CR_EPS) return;
    for(int p=0;p<n;p++){
        if (e->prov[p].owner!=c || !e->prov[p].active || !e->prov[p].colonized) continue;
        float w_=e->prov[p].strata[cls].wealth; if (w_<=0.f) continue;
        e->prov[p].strata[cls].wealth -= amount*(w_/tot);
    }
}
/* Crédite `amount` à la richesse d'une classe d'un pays, au prorata de la richesse
 * ACTUELLE de la classe dans chaque province (les provinces déjà riches en reçoivent
 * plus — même logique de fongibilité que le pool d'héritage, doctrine M0 §TRANSFERT).
 * Si la classe est partout à 0 (dette naissante d'un pays neuf), répartit ÉGALEMENT
 * entre provinces ACTIVES pour ne pas perdre le crédit. */
static void credit_wealth_prorata(WorldEconomy *e, int c, int cls, float amount){
    if (amount<=CR_EPS) return;
    float tot=0.f; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    int nactive=0;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c && e->prov[p].active && e->prov[p].colonized){ tot += e->prov[p].strata[cls].wealth; nactive++; }
    if (nactive<=0) return;
    if (tot>CR_EPS){
        for(int p=0;p<n;p++){
            if (e->prov[p].owner!=c || !e->prov[p].active || !e->prov[p].colonized) continue;
            float w_=e->prov[p].strata[cls].wealth;
            e->prov[p].strata[cls].wealth += amount*(w_/tot);
        }
    } else {
        float each=amount/(float)nactive;
        for(int p=0;p<n;p++) if(e->prov[p].owner==c && e->prov[p].active && e->prov[p].colonized) e->prov[p].strata[cls].wealth += each;
    }
}

float credit_borrow_local(WorldEconomy *e, int c, float need){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY || need<=CR_EPS) return 0.f;
    float floor_=tune_f("SINK_FLOOR",500.f);
    float covered=0.f;

    /* 1) PÉRÉQUATION — le surplus des AUTRES provinces du pays. Pure comptabilité
     * interne (le crédit aux classes a déjà eu lieu, cf. scps_econ.c §3) : DÉBIT seul,
     * aucun compte à créditer ici. */
    float nat=country_surplus(e,c,floor_);
    float perq=fminf(need, nat);
    if (perq>CR_EPS){ debit_surplus_prorata(e,c,floor_,perq); covered+=perq; }
    float rem=need-covered; if (rem<=CR_EPS) return covered;

    /* M3d — LE PLAFOND + LA TRANCHE (brief §1/§4) mordent ICI, PAS à la péréquation ci-
     * dessus (un transfert entre les PROPRES provinces d'un pays n'est pas un prêt — brief
     * §1 « plus personne ne prête » vise la vraie dette). Au plafond : rem_capped=0, les
     * CLASSES ne refusent toujours pas — il n'y a simplement plus de capacité à couvrir
     * (§2 : « sous le plafond, les classes ne refusent jamais » — AU plafond, plus
     * personne, classes incluses, brief §1). */
    float rem_capped = fminf(rem, debt_draw_cap(c));
    if (rem_capped<=CR_EPS) return covered;

    /* 2) EMPRUNT AUX PROPRES CLASSES — ∝ richesse (élites pondérées plus que bourgeois,
     * les laborers n'ont pas d'épargne), capacité PAR TICK plafonnée (registre J) : la
     * classe "prête" (sa richesse baisse) en échange d'une créance RÉELLE (g_debt.to_class,
     * remboursée+intérêt via credit_year_tick). DÉBIT seul ici aussi (même raison). */
    float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
    float share=tune_f("CLASS_LEND_SHARE",0.05f);
    float cap_e=0.f, cap_b=0.f; country_lendable(e,c,ew,bw,&cap_e,&cap_b);
    cap_e*=share; cap_b*=share;
    float cap_tot=cap_e+cap_b;
    if (cap_tot>CR_EPS){
        float borrow=fminf(rem_capped, cap_tot);
        float b_elite = borrow*(cap_e/cap_tot), b_bourg = borrow-b_elite;
        if (b_elite>CR_EPS) debit_wealth_prorata(e,c,CLASS_ELITE,b_elite);
        if (b_bourg>CR_EPS) debit_wealth_prorata(e,c,CLASS_BOURGEOIS,b_bourg);
        g_debt[c].to_class += borrow;
        covered += borrow;
    }
    return covered;
}

float credit_borrow_citystate(WorldEconomy *e, const World *w, int c, float need){
    if (!e || !w || c<0 || c>=SCPS_MAX_COUNTRY || need<=CR_EPS) return 0.f;
    /* M3d — LE PLAFOND + LA TRANCHE (brief §1/§4), même motif que credit_borrow_local
     * (cette fonction n'a PAS de péréquation : toute la fonction est de la vraie dette). */
    need = fminf(need, debt_draw_cap(c));
    if (need<=CR_EPS) return 0.f;
    float floor_=tune_f("SINK_FLOOR",500.f);
    float share=tune_f("CITYSTATE_LEND_SHARE",0.5f);

    /* prêteur : le créancier EXISTANT s'il est encore éligible/solvable (on ne
     * fragmente pas la dette-cité-état — simplicité v1, brief §5), sinon le plus riche
     * prêteur éligible (pick_lender).
     * M3d §2 — LE REFUS (« les cités-états PEUVENT refuser… trésor sous leur propre
     * plancher ») : DÉJÀ le motif ICI — `country_surplus(e,L,floor_)>CR_EPS` (existing_ok)
     * et `pick_lender` (n'élit qu'un pays au surplus RÉEL >SINK_FLOOR) refusent TOUS DEUX
     * un prêteur sous son propre plancher opérationnel. Choix documenté (brief : « choisis
     * au motif existant ») — pas de nouveau mécanisme relation/embargo (aurait exigé de
     * faire voyager DiploState/WorldProsperity jusqu'ici, à travers credit_spend/
     * credit_settle_monthly/econ_tick : hors scope, « la solution la plus simple »). */
    int L=g_debt[c].cs_id;
    bool existing_ok=false;
    if (L>=0 && L<w->n_countries && L!=c){
        bool lender=(w->country[L].role==POLITY_CITY_STATE);
        if (!lender){ Ethos et=country_ethos(e,w,L); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE); }
        existing_ok = lender && (country_surplus(e,L,floor_)>CR_EPS);
    }
    if (!existing_ok) L=pick_lender(e,w,c,floor_);
    if (L<0) return 0.f;

    float avail=country_surplus(e,L,floor_)*share;
    float borrow=fminf(need, avail);
    if (borrow<=CR_EPS) return 0.f;
    debit_surplus_prorata(e,L,floor_,borrow);   /* DÉBIT seul (même raison que credit_borrow_local) */
    g_debt[c].to_cs += borrow;
    g_debt[c].cs_id = (int16_t)L;
    return borrow;
}

float credit_borrow(WorldEconomy *e, const World *w, int c, float need){
    if (need<=CR_EPS) return 0.f;
    float covered=credit_borrow_local(e,c,need);
    float rem=need-covered;
    if (rem>CR_EPS) covered += credit_borrow_citystate(e,w,c,rem);
    if (covered+CR_EPS < need) g_defaults++;   /* ÉPUISEMENT : la chaîne complète n'a pas suffi */
    return covered;
}

void credit_settle_monthly(WorldEconomy *e, const World *w){
    if (!e || !w) return;
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float need = econ_va_shortfall_pending(c);
        if (need<=CR_EPS) continue;
        float covered = credit_borrow_citystate(e,w,c,need);
        econ_va_shortfall_resolve(c, covered);
        if (covered+CR_EPS < need) g_defaults++;   /* le canal se ferme : reliquat jamais créé (résidu mesuré, cf. econ_va_shortfall_resolve) */
    }
}

/* Débite le trésor RÉEL d'un pays (ad-hoc : chantiers/soldes/manufactures…). Passe par
 * la province représentative (capitale, charte — RE-KEY PROVINCE, cf. M3b-v1). Si le
 * trésor NET manque, DÉCLENCHE la chaîne d'emprunt (péréquation→classes→cité-état) au
 * lieu de laisser "monnaie négative" : c'est le refonte item 6 du brief M3c. */
void credit_spend(WorldEconomy *e, const World *w, int c, float cost){
    int hr=home_reg(w,c); if(hr<0||hr>=e->n_regions) return;
    int pid=econ_region_rep_province(e, hr); if(pid<0||pid>=e->n_prov) return;
    e->prov[pid].treasury-=cost;
    float short_=(float)(-country_gold_prov(e,c));   /* découvert NET du pays, s'il y en a un */
    if (short_>CR_EPS){
        float covered=credit_borrow(e,w,c,short_);
        /* le trésor RÉEL couvert doit revenir dans la province représentative (c'est
         * elle qui a essuyé le débit ci-dessus) — les autres provinces/classes/prêteurs
         * ont, eux, été DÉBITÉS par la chaîne (péréquation/classes/cité-état). */
        e->prov[pid].treasury += covered;
    }
}

/* INTÉRÊT ANNUEL = la rétroaction (rentier). Taux ↑ avec le ratio de dette TOTALE
 * (to_class+to_cs) ET la chute de légitimité. Financé par credit_borrow (la MÊME chaîne
 * — si le trésor national ne suffit pas, l'État réemprunte pour honorer l'intérêt,
 * plafonné comme tout emprunt : jamais d'argent créé). Réparti aux DEUX créanciers ∝
 * leur part de la dette. Puis AMORTISSEMENT (surplus substantiel → rembourse le
 * principal) et RACHAT DE CRÉDIT (le marché secondaire). */
void credit_year_tick(WorldEconomy *e, const WorldLegitimacy *wl, const World *w){
    (void)wl;   /* M3d : le taux ne lit plus la légitimité (brief §3, remplace l'incrément 1) */
    float floor_=tune_f("SINK_FLOOR",500.f);
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float debt_total = g_debt[c].to_class + g_debt[c].to_cs;
        if (debt_total<=CR_EPS){
            g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1;
            g_debt[c].insolvent_streak=0; g_forced_pending[c]=false;   /* M3d : pas de dette, pas de chronique */
            continue;
        }
        /* M3d §3 — LE TAUX (remplace l'incrément 1 : plus de légitimité/ligne∝pop) :
         * taux = BASE + SLOPE×(dette/plafond), clampé [MIN,MAX] — la prime de risque EST
         * le levier envers le plafond des 300 % (brief : « le service croissant étouffe la
         * dépense, la spirale espagnole émergente, voulue »). Le plafond CAPE STRUCTURELLEMENT
         * debt_total (credit_borrow* refusent au-delà, §1) : plus besoin d'un CREDIT_RATIO_CAP
         * séparé sur l'ASSIETTE d'intérêt (idebt de l'incrément 1) — l'assiette EST debt_total. */
        float ceiling=credit_debt_ceiling(c); if (ceiling<1.f) ceiling=1.f;
        float lev=debt_total/ceiling;
        float rate=clampf(tune_f("DEBT_RATE_BASE",0.02f) + tune_f("DEBT_RATE_SLOPE",0.03f)*lev,
                           tune_f("DEBT_RATE_MIN",0.02f), tune_f("DEBT_RATE_MAX",0.05f));
        float interest=debt_total*rate;

        /* L'intérêt se paie du SURPLUS COURANT du pays SEUL (jamais via credit_borrow*,
         * qui EMPRUNTE et grossirait le PRINCIPAL du même montant qu'on vient de "payer" —
         * un double-compte qui fabriquerait de la dette sans contrepartie réelle). Si le
         * surplus ne suffit pas, l'intérêt de cette année-là est simplement PLUS PETIT
         * (auto-limité) — jamais capitalisé, jamais créé. */
        float avail=country_surplus(e,c,floor_);
        float covered=fminf(interest, avail);
        if (covered>CR_EPS) debit_surplus_prorata(e,c,floor_,covered);
        if (covered+CR_EPS < interest) g_defaults++;   /* intérêt de l'année sous-servi (auto-limité) */
        if (covered>CR_EPS){
            float i_class=covered*(g_debt[c].to_class/debt_total);
            float i_cs   =covered-i_class;
            if (i_class>CR_EPS){   /* l'ÉLITE RENTIÈRE (et le bourgeois-créancier) vivent de l'intérêt d'État */
                float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
                float tot=ew+bw; if (tot<=CR_EPS) tot=1.f;
                credit_wealth_prorata(e,c,CLASS_ELITE,     i_class*(ew/tot));
                credit_wealth_prorata(e,c,CLASS_BOURGEOIS, i_class*(bw/tot));
            }
            if (i_cs>CR_EPS && g_debt[c].cs_id>=0){
                int hc=home_reg(w,g_debt[c].cs_id);
                if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) e->prov[cp].treasury+=i_cs; }
            }
        }
        econ_flux_add(c, FX_CREDIT, -covered);   /* I0 : la ligne intérêts (montant RÉELLEMENT servi) */

        /* AMORTISSEMENT — un pays au trésor GRAS rembourse une part du PRINCIPAL depuis
         * son surplus (au-dessus de COURT_FLOOR, le seuil de hoarding — même réserve que
         * le faste de cour) : "la dette VIT" — elle ne fait pas que grossir. */
        float hof=tune_f("COURT_FLOOR",4000.f);
        float surplus=country_surplus(e,c,hof);
        if (surplus>CR_EPS){
            float repay=fminf(debt_total, fminf(surplus, debt_total*tune_f("PRINCIPAL_REPAY_RATE",0.10f)));
            if (repay>CR_EPS){
                debit_surplus_prorata(e,c,hof,repay);
                float r_class=repay*(g_debt[c].to_class/debt_total), r_cs=repay-r_class;
                if (r_class>CR_EPS){
                    float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
                    float tot=ew+bw; if (tot<=CR_EPS) tot=1.f;
                    credit_wealth_prorata(e,c,CLASS_ELITE,     r_class*(ew/tot));
                    credit_wealth_prorata(e,c,CLASS_BOURGEOIS, r_class*(bw/tot));
                }
                if (r_cs>CR_EPS && g_debt[c].cs_id>=0){
                    int hc=home_reg(w,g_debt[c].cs_id);
                    if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) e->prov[cp].treasury+=r_cs; }
                }
                g_debt[c].to_class -= r_class; if (g_debt[c].to_class<0.f) g_debt[c].to_class=0.f;
                g_debt[c].to_cs    -= r_cs;    if (g_debt[c].to_cs<0.f)    g_debt[c].to_cs=0.f;
            }
        }
        if (g_debt[c].to_cs<=CR_EPS) g_debt[c].cs_id=-1;

        /* M3d §5 — LA BANQUEROUTE FORCÉE : « plafond atteint ET insolvable (épuisement
         * CHRONIQUE) ». Motif des grâces existantes (g_lowsat_streak/g_colony_cd, EMOB/
         * COLC) : un compteur d'années CONSÉCUTIVES au plafond (ré-évalué APRÈS intérêt+
         * amortissement de CETTE année, sur la dette FINALE) — BANKRUPTCY_GRACE_YEARS
         * (2 ans, registre J) de répit avant le couperet, jamais un pic isolé. g_forced_
         * pending est un flag TRANSIENT : scps_sim.c l'exécute juste après credit_year_
         * tick (RAZ dette + cicatrice + effet diplo, motif CMD_MANUMIT — sim.c orchestre,
         * credit.c fait le cœur). */
        float debt_final = g_debt[c].to_class + g_debt[c].to_cs;
        float ceiling_final = credit_debt_ceiling(c);
        if (debt_final >= ceiling_final - CR_EPS && ceiling_final>CR_EPS){
            if (g_debt[c].insolvent_streak < 30000) g_debt[c].insolvent_streak++;
        } else {
            g_debt[c].insolvent_streak = 0;
        }
        g_forced_pending[c] = (g_debt[c].insolvent_streak >= (int16_t)tune_f("BANKRUPTCY_GRACE_YEARS",2.f));
    }

    /* RACHAT DE CRÉDIT — le marché secondaire (les Fugger) : une cité-état/mercantile au
     * trésor OISIF rachète la dette-CLASSES d'un pays tiers à sa valeur faciale (v1
     * simple et déterministe, brief §5 — pas d'escompte spéculatif). Simplicité : un SEUL
     * créancier-cité-état par pays (comme l'incrément 1) — le créancier EXISTANT est
     * TOUJOURS prioritaire pour étendre sa position (même motif que
     * credit_borrow_citystate : sans cette priorité, `pick_lender` élit le PLUS RICHE
     * éligible CE tick, qui change d'un tick à l'autre — quasiment JAMAIS le créancier
     * déjà assigné, donc quasiment AUCUN rachat n'aboutissait, mesuré au calibrage : 0/9
     * sims sur le sweep {9,11,42}×3×250 avant ce fix). Un NOUVEAU créancier n'est élu
     * QUE si le pays n'en a encore AUCUN.
     * SEUIL D'OISIVETÉ = SINK_FLOOR (500), PAS COURT_FLOOR (4000, motif crédit_borrow_
     * citystate — le seuil de HOARDING) : mesuré au calibrage (SCPS_BUYBACKDIAG), un
     * créancier ACTIF (qui prête déjà à d'autres débiteurs chaque mois, credit_settle_
     * monthly) redescend rarement au-dessus de 4000 — son capital est CONTINUELLEMENT
     * redéployé, pas thésaurisé. Le rachat n'a jamais abouti (0/9 sims) avec COURT_FLOOR ;
     * SINK_FLOOR (même bar que le prêt normal) suffit à qualifier "un surplus disponible",
     * cohérent avec « racheter est un placement SÛR, pas moins attractif qu'un nouveau
     * prêt ». */
    float bthresh=tune_f("BUYBACK_DEBT_THRESHOLD",500.f);
    float ishare =tune_f("BUYBACK_IDLE_SHARE",0.30f);
    float ifloor=tune_f("SINK_FLOOR",500.f);
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        if (g_debt[c].to_class<=bthresh) continue;
        int L=-1;
        if (g_debt[c].cs_id>=0){
            int cand=g_debt[c].cs_id;
            bool lender=(cand>=0 && cand<w->n_countries && cand!=c && w->country[cand].role==POLITY_CITY_STATE);
            if (!lender && cand>=0 && cand<w->n_countries && cand!=c){
                Ethos et=country_ethos(e,w,cand); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE);
            }
            if (lender && country_surplus(e,cand,ifloor)>CR_EPS) L=cand;
        } else {
            L=pick_lender(e,w,c,ifloor);   /* aucun créancier encore assigné : le plus riche éligible */
        }
        if (getenv("SCPS_BUYBACKDIAG"))
            fprintf(stderr,"[BUYBACKDIAG] c=%d to_class=%.0f cs_id=%d L=%d surplus(L)=%.0f\n",
                    c, g_debt[c].to_class, g_debt[c].cs_id, L, L>=0?country_surplus(e,L,ifloor):-1.f);
        if (L<0) continue;
        float idle=country_surplus(e,L,ifloor)*ishare;
        float amount=fminf(g_debt[c].to_class, idle);
        if (amount<=CR_EPS) continue;
        debit_surplus_prorata(e,L,ifloor,amount);            /* le racheteur paie face value */
        float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
        float tot=ew+bw; if (tot<=CR_EPS) tot=1.f;
        credit_wealth_prorata(e,c,CLASS_ELITE,     amount*(ew/tot));   /* les classes créancières sont CASHÉES OUT */
        credit_wealth_prorata(e,c,CLASS_BOURGEOIS, amount*(bw/tot));
        g_debt[c].to_class -= amount;
        g_debt[c].to_cs    += amount;
        g_debt[c].cs_id     = (int16_t)L;                    /* le racheteur DEVIENT le créancier */
        g_buybacks++;
    }
}

/* M3d §5 — LA BANQUEROUTE : répudiation TOTALE. « Leur monnaie est déjà partie au prêt » —
 * pas de création/destruction, on RAYE juste l'ACTIF du créancier (les classes rentières ne
 * sont PAS remboursées, leur richesse ne bouge pas ICI — la répudiation, c'est précisément
 * qu'elles ne reverront pas cette créance). Retourne l'ex-créancier cité-état (-1 si aucun)
 * — l'appelant (scps_sim.c, CMD_BANKRUPTCY + le forcé après credit_year_tick) en tire
 * l'effet diplomatique (rancune, motif §6) : credit.c ne connaît pas DiploState. `forced`
 * pilote la télémétrie SEULE (n/sim forcée vs volontaire, brief gate 1). */
int credit_bankruptcy(WorldEconomy *e, int c, bool forced){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY) return -1;
    int L = g_debt[c].cs_id;
    g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
    g_forced_pending[c]=false;
    /* DÉBUFF −75 % (production/croissance/moral, brief « tape fort ») : la cicatrice
     * frappe TOUTES les provinces ACTIVES du pays (motif revolt_scar — econ_tick la
     * décroît sur BANKRUPTCY_SCAR_YEARS, scps_campaign.c lit econ_country_bankruptcy_scar
     * pour le moral d'armée). */
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    for (int p=0;p<n;p++) if (e->prov[p].owner==c && e->prov[p].active) e->prov[p].bankruptcy_scar=1.f;
    if (forced) g_bankrupt_forced++; else g_bankrupt_voluntary++;
    return L;
}
/* Flag TRANSIENT posé par credit_year_tick (streak au plafond ≥ BANKRUPTCY_GRACE_YEARS) —
 * scps_sim.c le lit juste après credit_year_tick et exécute credit_bankruptcy(e,c,true)
 * pour chaque pays flaggé (le flag redescend alors via credit_bankruptcy lui-même). */
bool credit_bankrupt_pending(int c){ return (c>=0 && c<SCPS_MAX_COUNTRY) && g_forced_pending[c]; }

bool credit_save(FILE *f){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (fwrite(&g_debt[c].to_class,        sizeof(float),  1,f)!=1) return false;
        if (fwrite(&g_debt[c].to_cs,           sizeof(float),  1,f)!=1) return false;
        if (fwrite(&g_debt[c].cs_id,           sizeof(int16_t),1,f)!=1) return false;
        if (fwrite(&g_debt[c].insolvent_streak,sizeof(int16_t),1,f)!=1) return false;   /* M3d (v90) */
    }
    return true;
}
bool credit_load(FILE *f){
    /* lecture BRUTE — la revalidation ("refus net") est le rôle de save_sane
     * (scps_save.c), même convention que credit_of/reserve_gold/va_country_prev :
     * cette fonction ne fait QUE désérialiser. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (fread(&g_debt[c].to_class,        sizeof(float),  1,f)!=1) return false;
        if (fread(&g_debt[c].to_cs,           sizeof(float),  1,f)!=1) return false;
        if (fread(&g_debt[c].cs_id,           sizeof(int16_t),1,f)!=1) return false;
        if (fread(&g_debt[c].insolvent_streak,sizeof(int16_t),1,f)!=1) return false;    /* M3d (v90) */
    }
    return true;
}
