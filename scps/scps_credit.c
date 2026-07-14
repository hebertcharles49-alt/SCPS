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
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define CR_EPS 1e-4f

typedef struct {
    float   to_class;    /* dette due aux PROPRES classes du pays (élites+bourgeois, agrégé) */
    float   to_cs;        /* dette due à LA cité-état créancière (agrégé) */
    int16_t cs_id;         /* pays créancier cité-état, -1 = aucun */
} CountryDebt;

static CountryDebt g_debt[SCPS_MAX_COUNTRY];
static long g_buybacks=0, g_defaults=0;   /* télémétrie MONDE, RAZ par credit_init (par partie/sim) */

void credit_init(void){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; }
    g_buybacks=0; g_defaults=0;
}
int  credit_of(int c){ return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].cs_id : -1; }
float credit_debt_class(int c)     { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_class : 0.f; }
float credit_debt_citystate(int c) { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_cs    : 0.f; }
float credit_debt_total(int c)     { return credit_debt_class(c)+credit_debt_citystate(c); }
void credit_stats_get(long *buybacks, long *defaults){
    if (buybacks) *buybacks=g_buybacks;
    if (defaults) *defaults=g_defaults;
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
        float borrow=fminf(rem, cap_tot);
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
    float floor_=tune_f("SINK_FLOOR",500.f);
    float share=tune_f("CITYSTATE_LEND_SHARE",0.5f);

    /* prêteur : le créancier EXISTANT s'il est encore éligible/solvable (on ne
     * fragmente pas la dette-cité-état — simplicité v1, brief §5), sinon le plus riche
     * prêteur éligible (pick_lender). */
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
    float floor_=tune_f("SINK_FLOOR",500.f);
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float debt_total = g_debt[c].to_class + g_debt[c].to_cs;
        if (debt_total<=CR_EPS){ g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; continue; }
        float legit=legitimacy_country(wl,w,e,c);                   /* 0..10 */
        float line=credit_line(w,e,c); if(line<1.f) line=1.f;
        /* ANTI-EMBALLEMENT (hérité de l'incrément 1) : taux ET assiette plafonnent
         * au-delà de CREDIT_RATIO_CAP×ligne — l'intérêt devient CONSTANT, la dette
         * croît LINÉAIREMENT (bornée), jamais géométrique. */
        float rcap=tune_f("CREDIT_RATIO_CAP",8.f);
        float ratio=debt_total/line; if (ratio>rcap) ratio=rcap;
        float rate=tune_f("CREDIT_RATE_BASE",0.05f)*(1.f+ratio+(10.f-legit)/10.f);
        float idebt=debt_total; if (idebt>rcap*line) idebt=rcap*line;
        float interest=idebt*rate;

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

bool credit_save(FILE *f){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (fwrite(&g_debt[c].to_class,sizeof(float),1,f)!=1) return false;
        if (fwrite(&g_debt[c].to_cs,   sizeof(float),1,f)!=1) return false;
        if (fwrite(&g_debt[c].cs_id,   sizeof(int16_t),1,f)!=1) return false;
    }
    return true;
}
bool credit_load(FILE *f){
    /* lecture BRUTE — la revalidation ("refus net") est le rôle de save_sane
     * (scps_save.c), même convention que credit_of/reserve_gold/va_country_prev :
     * cette fonction ne fait QUE désérialiser. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (fread(&g_debt[c].to_class,sizeof(float),  1,f)!=1) return false;
        if (fread(&g_debt[c].to_cs,   sizeof(float),  1,f)!=1) return false;
        if (fread(&g_debt[c].cs_id,   sizeof(int16_t),1,f)!=1) return false;
    }
    return true;
}
