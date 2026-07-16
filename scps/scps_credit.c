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
/* M3g — LA SAISIE : le créancier D'AVANT-répudiation figé au moment de la banqueroute
 * (credit_bankruptcy), valide toute la durée de la cicatrice. cs_id=-1 ⇒ tout domestique
 * (aucun créancier CS à la banqueroute, ou dette-CS nulle). SÉRIALISÉ (v92) — doit
 * survivre un save/reload pendant la cicatrice (~10 ans, BANKRUPTCY_SCAR_YEARS). */
static int16_t g_garnish_cs_id[SCPS_MAX_COUNTRY];
static float   g_garnish_cs_share[SCPS_MAX_COUNTRY];
/* Accumulateur INTER-TICK (motif EMOB/g_mint_demand_prev) : la part cité-état saisie
 * CE mois, en attente du règlement ANNUEL (credit_year_tick). SÉRIALISÉ (v92). */
static float   g_garnish_cs_pending[SCPS_MAX_COUNTRY];
/* Télémétrie MONDE cumulée CE run — RAZ par credit_init, NON sérialisée (motif
 * g_buybacks/g_defaults : un compteur de partie, pas un état de simulation). */
static double  g_garnish_total=0.0, g_garnish_domestic=0.0, g_garnish_cs_tel=0.0;
/* V2 (MONNAIE M9) — LA DEMANDE D'EMPRUNT DIPLOMATIQUE : état TRANSIENT de la DERNIÈRE demande
 * (façade/UI — « [État] accorde/refuse », jamais un flottant). target=-1 ⇒ aucune demande
 * cette partie. Non sérialisé (motif g_buybacks/g_defaults — un fanion d'UI, pas un état de
 * simulation : une demande faite juste avant une sauvegarde redevient « aucune » après
 * recharge, sans conséquence de gameplay — le PRÊT lui-même, s'il a été accordé, est réel et
 * DÉJÀ sérialisé via g_debt ci-dessus). */
static int16_t g_loan_req_target[SCPS_MAX_COUNTRY];
static bool    g_loan_req_granted[SCPS_MAX_COUNTRY];
/* V3 (MONNAIE M9) — LE RACHAT DE L'ANNÉE : archétype (LOAN_ARCHETYPE_*, scps_credit.h) du
 * racheteur qui vient d'acquérir la dette-classes du pays c CETTE année (credit_year_tick,
 * RACHAT DE CRÉDIT plus bas) — NONE si aucun rachat. TRANSIENT (RAZ en tête de CHAQUE
 * credit_year_tick), lu par scps_sim.c juste après (credit.c n'a pas DiploState/Statecraft)
 * pour appliquer la MÉTABOLISATION DISTINCTE par type de créancier. Non sérialisé (motif
 * g_forced_pending). */
static int8_t  g_buyback_archetype[SCPS_MAX_COUNTRY];
/* Télémétrie MONDE cumulée CE run (motif g_buybacks) : compte des rachats PAR ARCHÉTYPE. */
static long g_buyback_cs=0, g_buyback_mercantile=0, g_buyback_pacifist=0;

void credit_init(void){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
        g_forced_pending[c]=false;
        g_garnish_cs_id[c]=-1; g_garnish_cs_share[c]=0.f; g_garnish_cs_pending[c]=0.f;
        g_loan_req_target[c]=-1; g_loan_req_granted[c]=false;
        g_buyback_archetype[c]=LOAN_ARCHETYPE_NONE;
    }
    g_buybacks=0; g_defaults=0; g_bankrupt_forced=0; g_bankrupt_voluntary=0;
    g_garnish_total=0.0; g_garnish_domestic=0.0; g_garnish_cs_tel=0.0;
    g_buyback_cs=0; g_buyback_mercantile=0; g_buyback_pacifist=0;
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

/* V1 (MONNAIE M9) — le TAUX courant (lecture PURE, aucune mutation) : factorisé hors de
 * credit_year_tick (même formule EXACTE, M3d §3 — aucune constante changée) pour servir le
 * reader façade « capacité d'emprunt disponible par ordre » (taux proposé) sans dupliquer le
 * calcul. Pur refactor : credit_year_tick appelle désormais cette fonction au lieu d'inliner
 * la formule — comportement BIT-IDENTIQUE (même expression, mêmes tunables). */
float credit_current_rate(int c){
    float ceiling = credit_debt_ceiling(c); if (ceiling<1.f) ceiling=1.f;
    float lev = credit_debt_total(c)/ceiling;
    return clampf(tune_f("DEBT_RATE_BASE",0.02f) + tune_f("DEBT_RATE_SLOPE",0.03f)*lev,
                  tune_f("DEBT_RATE_MIN",0.02f), tune_f("DEBT_RATE_MAX",0.05f));
}

/* MONNAIE M11 — A3 v2 : L'INTÉRÊT FIXE (voir DEBT_FIXED, scps_tune_list.h — décision joueur
 * « si t'empruntes 1000 à 5 %, tu rembourses 1050, pas +5 % par an »). Le MONTANT DE DETTE
 * à inscrire pour un emprunt de `borrow` (le RÉEL transféré, INCHANGÉ — seul ce qui va au
 * PASSIF change) : le taux courant (credit_current_rate, formule M3d INCHANGÉE) est lu ICI,
 * AVANT toute mutation de g_debt par l'appelant (le levier reflète la situation PRÉ-prêt,
 * convention de cotation) et FIGÉ pour ce prêt — le forfait n'est JAMAIS recalculé ensuite.
 * DEBT_FIXED=0 : kill-switch — renvoie `borrow` nu (comportement pré-M11 exact, aucun
 * markup). Appelée aux 4 sites d'origination (credit_borrow_local/class/citystate/state) ;
 * jamais au rachat (V3/M9 : une créance qui change de mains n'est pas une NOUVELLE
 * origination, sa valeur reste le restant dû, INCHANGÉ). */
static float debt_origination(int c, float borrow){
    if (borrow<=CR_EPS) return borrow;
    if (tune_f("DEBT_FIXED", 1.0f) <= 0.f) return borrow;
    float rate = credit_current_rate(c);
    return borrow * (1.f + rate);
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
 * province — aucune ne descend sous floor_ par construction (amount borné par l'appelant).
 * MONNAIE M11 — A2 : tient region[].treasury EN PHASE à CHAQUE débit (motif
 * econ_prov_treasury_credit) — cette fonction est le SEUL débiteur partagé de TOUTE la
 * chaîne de crédit (emprunt local/cité-état/état, intérêt, amortissement, rachat) ;
 * corriger ICI ferme d'un coup tous les sites qui l'appellent, y compris ceux qui
 * s'exécutent APRÈS econ_aggregate_regions (credit_year_tick, credit_settle_monthly,
 * les verbes joueur) — motif signalé « Reste » par TROUVAILLES M9 (« côté PRÊTEUR, hors
 * scope M9, signalé pour un futur audit crédit »). Appelée AUSSI depuis l'intérieur
 * d'econ_tick (credit_borrow_local, AVANT agrégation) : la double écriture y est un
 * no-op inoffensif (region[] est de toute façon réécrit EN ENTIER par l'agrégation qui
 * suit). */
static void debit_surplus_prorata(WorldEconomy *e, int c, float floor_, float amount){
    if (amount<=CR_EPS) return;
    float tot=country_surplus(e,c,floor_); if (tot<=CR_EPS) return;
    int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++){
        if (e->prov[p].owner!=c || !e->prov[p].active || !e->prov[p].colonized) continue;
        float s=fmaxf(0.f, e->prov[p].treasury - floor_); if (s<=0.f) continue;
        float share=amount*(s/tot);
        e->prov[p].treasury -= share;
        int r=e->prov[p].region;
        if (r>=0 && r<e->n_regions) e->region[r].treasury -= share;
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
        g_debt[c].to_class += debt_origination(c, borrow);   /* MONNAIE M11 — A3 v2 : forfait figé */
        covered += borrow;
    }
    return covered;
}

/* ---- V1 (MONNAIE M9) : LE VERBE « EMPRUNTER À UN ORDRE » (panneau éco) --------------- */

/* Capacité d'emprunt DISPONIBLE (lecture PURE, aucune mutation) pour la classe `cls` du pays
 * `c` — l'intersection de la capacité PAR CLASSE (même formule que credit_borrow_local §2 :
 * CLASS_LEND_SHARE × richesse pondérée ELITE/BOURGEOIS_LEND_WEIGHT) et du plafond+tranche
 * STRUCTUREL du pays (debt_draw_cap, M3d — partagé avec la chaîne auto). CLASS_LABORER/
 * CLASS_SLAVE : toujours 0 (aucune épargne, motif M3c déjà établi "les laborers n'ont pas
 * d'épargne") — la fiche UI grise le bouton, aucun refus à coder côté verbe (décision joueur :
 * « l'ordre ne refuse pas », il n'y a simplement rien à prêter). Sert le reader façade
 * scps_country_loan_capacity (montant max + credit_current_rate, le taux proposé). */
float credit_class_borrow_capacity(const WorldEconomy *e, int c, SocialClass cls){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY) return 0.f;
    if (cls!=CLASS_ELITE && cls!=CLASS_BOURGEOIS) return 0.f;
    float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
    float share=tune_f("CLASS_LEND_SHARE",0.05f);
    float cap_e=0.f, cap_b=0.f; country_lendable(e,c,ew,bw,&cap_e,&cap_b);
    float cap = (cls==CLASS_ELITE) ? cap_e*share : cap_b*share;
    return fminf(cap, debt_draw_cap(c));
}
/* LE VERBE : emprunt EXPLICITE à UNE SEULE classe choisie par le joueur (décision 2026-07-16,
 * « panneau éco, c'est l'ordre qui prête… l'état emprunte d'abord aux classes ») — même étage/
 * mêmes capacités que credit_borrow_local §2 (AUCUNE voie neuve), mais isolé à `cls` (pas de
 * répartition auto ∝richesse entre élite/bourgeois) et un TRANSFERT COMPLET : contrairement à
 * credit_borrow_local (qui ne fait que DÉBITER pour combler un besoin déjà tracé ailleurs par
 * l'appelant), ce verbe est AUTONOME — il DÉPOSE lui-même le produit au trésor national (motif
 * credit_spend, capitale — AUCUN World* requis, econ_country_capital_prov scanne prov[]).
 * need<=0 ⇒ emprunte le MAXIMUM disponible (capacité ci-dessus). Retourne le montant RÉELLEMENT
 * prêté (0 si la classe n'a rien à prêter — jamais un refus, juste une capacité épuisée). */
float credit_borrow_class(WorldEconomy *e, int c, SocialClass cls, float need){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY) return 0.f;
    float cap = credit_class_borrow_capacity(e, c, cls);
    if (cap<=CR_EPS) return 0.f;
    float borrow = (need>CR_EPS) ? fminf(need, cap) : cap;
    if (borrow<=CR_EPS) return 0.f;
    debit_wealth_prorata(e, c, cls, borrow);
    g_debt[c].to_class += debt_origination(c, borrow);   /* MONNAIE M11 — A3 v2 : forfait figé */
    /* LE DÉPÔT : econ_region_treasury_add (PAS une écriture prov[].treasury directe) — c'est
     * le SEUL chemin qui tient region[].treasury EN PHASE avec prov[] (econ_country_gold,
     * credit_can_spend, credit_line, audit_eco lisent TOUS region[] ; region[].treasury n'est
     * JAMAIS ré-agrégé depuis prov[] ailleurs dans le moteur — un écrit prov[]-seul serait
     * invisible du trésor national jusqu'à la fin des temps). ProvinceEconomy.region est le
     * MIROIR province-grain de World.province[].region (aucun World* requis, motif ci-dessus). */
    int cap_pid = econ_country_capital_prov(e, c);
    if (cap_pid>=0 && cap_pid<e->n_prov){
        int reg = e->prov[cap_pid].region;
        if (reg>=0 && reg<e->n_regions) econ_region_treasury_add(e, reg, borrow);
    }
    return borrow;
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
    g_debt[c].to_cs += debt_origination(c, borrow);   /* MONNAIE M11 — A3 v2 : forfait figé */
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

/* ---- V2 (MONNAIE M9) : LA DEMANDE D'EMPRUNT DIPLOMATIQUE (diplomatie) ---------------- */

/* Le CONSENTEMENT est évalué par l'APPELANT (scps_sim.c a Statecraft/DiploState ; credit.c ne
 * les a pas, motif M3d §2 « le refus des cités-états, le motif existant ») via
 * ai_consider_offer/OFFER_LOAN (scps_ai.c) — cette fonction ASSUME le consentement DÉJÀ acquis
 * et ne fait QUE le transfert réel, au MÊME motif que credit_borrow_citystate (débit réel du
 * prêteur, plafond+tranche M3d du débiteur) mais LE CRÉANCIER EST CHOISI PAR LE JOUEUR (pas
 * pick_lender) — n'importe quel État étranger, pas seulement cité-état/mercantile/pacifiste
 * (réservés à V3, le RACHAT, ci-dessous). Un SEUL créancier étranger à la fois (motif « un
 * SEUL créancier-cité-état par pays », M3c) : refuse si le débiteur a déjà un AUTRE créancier
 * engagé (`to_cs`>0 sous un cs_id différent) — il doit d'abord s'acquitter/laisser s'éteindre
 * l'ancien avant d'en solliciter un nouveau. TRANSFERT COMPLET (motif credit_borrow_class, V1
 * ci-dessus) : DÉBITE le prêteur ET CRÉDITE le trésor du débiteur (contrairement à
 * credit_borrow_citystate qui ne fait que DÉBITER pour combler un besoin déjà tracé ailleurs
 * par l'appelant — ce verbe est autonome). Retourne le montant RÉELLEMENT prêté (0 =
 * impossible : déjà un autre créancier, plafond atteint, ou prêteur sans surplus). */
float credit_borrow_state(WorldEconomy *e, const World *w, int debtor_c, int lender_c, float amount){
    if (!e || !w || debtor_c<0 || debtor_c>=SCPS_MAX_COUNTRY || lender_c<0 || lender_c>=SCPS_MAX_COUNTRY
        || debtor_c==lender_c) return 0.f;
    if (g_debt[debtor_c].to_cs>CR_EPS && g_debt[debtor_c].cs_id>=0 && g_debt[debtor_c].cs_id!=(int16_t)lender_c)
        return 0.f;   /* déjà engagé avec un AUTRE créancier étranger */
    /* amount<=0 ⇒ le MAXIMUM disponible (motif credit_borrow_class, V1) — le joueur demande
     * « autant que possible » plutôt qu'un montant précis. */
    float need = (amount>CR_EPS) ? fminf(amount, debt_draw_cap(debtor_c)) : debt_draw_cap(debtor_c);
    if (need<=CR_EPS) return 0.f;
    float floor_=tune_f("SINK_FLOOR",500.f);
    float share=tune_f("CITYSTATE_LEND_SHARE",0.5f);   /* même capacité/tick que l'auto-emprunt cité-état */
    float avail=country_surplus(e,lender_c,floor_)*share;
    float borrow=fminf(need, avail);
    if (borrow<=CR_EPS) return 0.f;
    debit_surplus_prorata(e,lender_c,floor_,borrow);
    /* LE DÉPÔT chez le DÉBITEUR (motif credit_borrow_class, V1) : la CAPITALE-province
     * (econ_country_capital_prov, province-grain pur) + econ_region_treasury_add — JAMAIS
     * econ_region_rep_province dans un chemin joueur (doctrine province, CLAUDE.md) et
     * JAMAIS une écriture prov[].treasury nue (region[].treasury n'est ré-agrégé nulle part
     * ailleurs — invisible d'econ_country_gold/credit_can_spend sinon). `w` ne sert plus
     * qu'au garde-fou NULL ci-dessus (home_reg/econ_region_rep_province abandonnés, plus
     * besoin de World* pour résoudre la capitale — motif credit_borrow_class). */
    int cap_pid = econ_country_capital_prov(e, debtor_c);
    if (cap_pid>=0 && cap_pid<e->n_prov){
        int reg = e->prov[cap_pid].region;
        if (reg>=0 && reg<e->n_regions) econ_region_treasury_add(e, reg, borrow);
    }
    g_debt[debtor_c].to_cs += debt_origination(debtor_c, borrow);   /* MONNAIE M11 — A3 v2 : forfait figé */
    g_debt[debtor_c].cs_id = (int16_t)lender_c;
    return borrow;
}
/* État TRANSIENT de la DERNIÈRE demande (façade/UI, voir scps_credit.h — « [État] accorde/
 * refuse », jamais un flottant). Écrit par scps_sim.c juste après avoir évalué
 * ai_consider_offer/OFFER_LOAN et tenté credit_borrow_state ci-dessus. */
void credit_loan_request_note(int debtor_c, int lender_c, bool granted){
    if (debtor_c<0 || debtor_c>=SCPS_MAX_COUNTRY) return;
    g_loan_req_target[debtor_c]  = (int16_t)lender_c;
    g_loan_req_granted[debtor_c] = granted;
}
int credit_loan_request_target(int debtor_c){
    return (debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY) ? (int)g_loan_req_target[debtor_c] : -1;
}
bool credit_loan_request_granted(int debtor_c){
    return (debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY) && g_loan_req_granted[debtor_c];
}

/* MONNAIE M11 — A3 v2 : LE SERVICE ANNUEL = la rétroaction (rentier), désormais une
 * ÉCHÉANCE MINIMALE sur un stock dont le markup est FIGÉ à l'origination (DEBT_FIXED,
 * scps_tune_list.h — decision joueur « 1000 à 5 % ⇒ tu rembourses 1050, pas +5 %/an » ;
 * kill-switch=0 restaure l'intérêt ANNUEL composé M3d pré-M11 EXACT). Payé du SURPLUS
 * SEUL (jamais emprunté, jamais capitalisé si impayé — le manquant nourrit le streak
 * d'IMPAYÉS, cf. plus bas : LE défaut réel). Réparti aux DEUX créanciers ∝ leur part de
 * la dette. Puis AMORTISSEMENT (surplus substantiel → rembourse le stock plus vite) et
 * RACHAT DE CRÉDIT (le marché secondaire, INCHANGÉ — une créance qui change de mains
 * n'est pas une nouvelle origination). */
void credit_year_tick(WorldEconomy *e, const WorldLegitimacy *wl, const World *w){
    (void)wl;   /* M3d : le taux ne lit plus la légitimité (brief §3, remplace l'incrément 1) */
    float floor_=tune_f("SINK_FLOOR",500.f);
    /* V3 (MONNAIE M9) — RAZ le fanion TRANSIENT « rachat de l'année » (motif g_forced_pending :
     * un flag lu par scps_sim.c juste après CET appel, jamais à cheval sur deux années). */
    for(int c=0;c<SCPS_MAX_COUNTRY;c++) g_buyback_archetype[c]=LOAN_ARCHETYPE_NONE;
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float debt_total = g_debt[c].to_class + g_debt[c].to_cs;
        if (debt_total<=CR_EPS){
            g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1;
            g_debt[c].insolvent_streak=0; g_forced_pending[c]=false;   /* M3d : pas de dette, pas de chronique */
            continue;
        }
        /* MONNAIE M11 — A3 v2 : L'INTÉRÊT FIXE + L'ÉCHÉANCE MINIMALE (voir DEBT_FIXED,
         * scps_tune_list.h — décision joueur, remplace le service d'intérêt ANNUEL composé
         * de M3d). `fixed` : chaque prêt porte déjà son markup (debt_origination, figé à
         * l'origination) — plus AUCUNE rente annuelle sur le stock ; à la place, une
         * ÉCHÉANCE MINIMALE (DEBT_DUE_FRAC × stock) est DUE chaque année, payée du surplus
         * SEUL (jamais empruntée, motif M3d inchangé), et RÉDUIT le stock (elle ÉTEINT une
         * part de la créance — motif amortissement). `!fixed` (kill-switch) : le service
         * d'intérêt ANNUEL M3d pré-M11 EXACT (le stock ne bouge JAMAIS, l'intérêt ne
         * rembourse rien). */
        bool fixed = tune_f("DEBT_FIXED", 1.0f) > 0.f;
        float rate_now = credit_current_rate(c);   /* V1 (M9) — factorisé, formule M3d INCHANGÉE */
        float charge = fixed ? (debt_total * tune_f("DEBT_DUE_FRAC", 0.10f)) : (debt_total * rate_now);

        /* L'échéance/l'intérêt se paie du SURPLUS COURANT du pays SEUL (jamais via
         * credit_borrow*, qui EMPRUNTE et grossirait le stock du même montant qu'on vient de
         * "payer" — un double-compte qui fabriquerait de la dette sans contrepartie réelle).
         * Si le surplus ne suffit pas, le service de cette année-là est simplement PLUS PETIT
         * (auto-limité) — jamais capitalisé, jamais créé (« fixe veut dire fixe »). */
        float avail=country_surplus(e,c,floor_);
        float covered=fminf(charge, avail);
        if (covered>CR_EPS) debit_surplus_prorata(e,c,floor_,covered);
        bool underpaid = (covered+CR_EPS < charge);
        if (underpaid) g_defaults++;   /* échéance/intérêt de l'année sous-servi (auto-limité) */

        if (covered>CR_EPS){
            float i_class=covered*(g_debt[c].to_class/debt_total);
            float i_cs   =covered-i_class;
            if (i_class>CR_EPS){   /* l'ÉLITE RENTIÈRE (et le bourgeois-créancier) vivent du flux de remboursement */
                float ew=tune_f("ELITE_LEND_WEIGHT",1.0f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
                float tot=ew+bw; if (tot<=CR_EPS) tot=1.f;
                float amt_e=i_class*(ew/tot), amt_b=i_class*(bw/tot);
                /* MONNAIE M3i — RETENUE À LA SOURCE sur le REVENU des classes créancières
                 * (brief : « intérêts de la dette versés aux classes créancières » explicitement
                 * nommé). `fixed` : SEULE la part INTÉRÊT de CHAQUE remboursement est un revenu
                 * (taux/(1+taux) du flux, décision joueur A3 v2) — la part PRINCIPAL rembourse
                 * un capital déjà prêté, ce n'est pas un gain. `!fixed` (legacy) : TOUT le flux
                 * ÉTAIT de l'intérêt (comportement pré-M11 exact). Pas de province unique ici
                 * (le paiement est NATIONAL, prorata sur toutes les provinces du pays via
                 * credit_wealth_prorata) : la CAPITALE sert de référence fiscale
                 * (econ_income_tax_rate_capital, scps_econ.c) et reçoit la retenue au trésor —
                 * kill-switch INCOME_TAX=0 ⇒ taux 0 ⇒ comportement legacy EXACT. */
                float ifrac = fixed ? (rate_now/(1.f+rate_now)) : 1.f;
                float taxable_e=amt_e*ifrac, taxable_b=amt_b*ifrac;
                float rate_e=econ_income_tax_rate_capital(e,c,CLASS_ELITE);
                float rate_b=econ_income_tax_rate_capital(e,c,CLASS_BOURGEOIS);
                float tax_e=taxable_e*rate_e, tax_b=taxable_b*rate_b;
                if (tax_e+tax_b>CR_EPS){
                    int cap=econ_country_capital_prov(e,c);
                    if (cap>=0 && cap<e->n_prov) econ_prov_treasury_credit(e, cap, tax_e+tax_b);   /* MONNAIE M11 — A2 */
                    econ_flux_add(c, FX_TAX, tax_e+tax_b);
                    amt_e-=tax_e; amt_b-=tax_b;
                }
                credit_wealth_prorata(e,c,CLASS_ELITE,     amt_e);
                credit_wealth_prorata(e,c,CLASS_BOURGEOIS, amt_b);
            }
            if (i_cs>CR_EPS && g_debt[c].cs_id>=0){
                int hc=home_reg(w,g_debt[c].cs_id);
                if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) econ_prov_treasury_credit(e, cp, i_cs); }   /* MONNAIE M11 — A2 */
            }
            /* `fixed` : le remboursement ÉTEINT une part du stock (principal+markup blended,
             * motif amortissement ci-dessous) — « rembourses 1050 » se solde en RÉDUISANT la
             * créance de ce qui a été payé. `!fixed` (legacy) : le stock NE bouge PAS ici
             * (l'intérêt pré-M11 ne remboursait rien — comportement EXACT, cf. le contrôle
             * historique « le principal n'a pas grossi rien qu'à payer l'intérêt »). */
            if (fixed){
                g_debt[c].to_class -= i_class; if (g_debt[c].to_class<0.f) g_debt[c].to_class=0.f;
                g_debt[c].to_cs    -= i_cs;    if (g_debt[c].to_cs<0.f)    g_debt[c].to_cs=0.f;
            }
        }
        econ_flux_add(c, FX_CREDIT, -covered);   /* I0 : la ligne intérêts/échéance (montant RÉELLEMENT servi) */

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
                    if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) econ_prov_treasury_credit(e, cp, r_cs); }   /* MONNAIE M11 — A2 */
                }
                g_debt[c].to_class -= r_class; if (g_debt[c].to_class<0.f) g_debt[c].to_class=0.f;
                g_debt[c].to_cs    -= r_cs;    if (g_debt[c].to_cs<0.f)    g_debt[c].to_cs=0.f;
            }
        }
        if (g_debt[c].to_cs<=CR_EPS) g_debt[c].cs_id=-1;

        /* M3d §5 / MONNAIE M11 — A3 v2 — LA BANQUEROUTE FORCÉE : « plafond atteint OU
         * échéance/intérêt chroniquement IMPAYÉ SUR UNE DETTE SUBSTANTIELLE » (le OU est le
         * CŒUR de A3 v2 — l'audit : un pays sous le plafond qui ne paie PLUS JAMAIS RIEN
         * restait, pré-M11, hors de portée de la banqueroute forcée). `fixed` : le streak
         * réagit à `underpaid` (échéance manquée CETTE année) EN PLUS du plafond — MAIS
         * SEULEMENT si `debt_final` dépasse DEBT_DEFAULT_THRESHOLD (registre J, DÉLIBÉRÉMENT
         * distinct de BUYBACK_DEBT_THRESHOLD — le seuil du RACHAT DE CRÉDIT, un mécanisme
         * DIFFÉRENT, INCHANGÉ ici). CALIBRAGE (sweep {9,11,42}×3×250, mesuré) : sans ce
         * plancher, N'IMPORTE QUEL résidu de dette (même quelques or, jamais remboursable par
         * un pays qui ne repasse JAMAIS SINK_FLOOR de trésor) déclenchait la banqueroute
         * forcée après BANKRUPTCY_GRACE_YEARS années — Σ banqueroutes 583→~1950 QUELLE QUE
         * SOIT DEBT_DUE_FRAC (0.02 à 0.10, bifurcation pas un gradient, motif M7/M8) : la
         * FRACTION n'était PAS le vrai levier, la LARGEUR du déclencheur l'était. Avec le
         * plancher : seule une dette VRAIMENT substantielle, jamais servie pendant 5 ans,
         * fait faillite — le défaut RÉEL, pas le résidu trivial. `!fixed` (legacy) :
         * SEULEMENT le plafond (comportement pré-M11 exact, le bug de l'audit reproduit tel
         * quel). Motif des grâces existantes (g_lowsat_streak/g_colony_cd, EMOB/COLC) : un
         * compteur d'années CONSÉCUTIVES en défaut (ré-évalué APRÈS échéance/intérêt+
         * amortissement de CETTE année) — BANKRUPTCY_GRACE_YEARS (registre J) de répit avant
         * le couperet, jamais un pic isolé. g_forced_pending est un flag TRANSIENT : scps_
         * sim.c l'exécute juste après credit_year_tick (RAZ dette + cicatrice + effet diplo,
         * motif CMD_MANUMIT — sim.c orchestre, credit.c fait le cœur). */
        float debt_final = g_debt[c].to_class + g_debt[c].to_cs;
        float ceiling_final = credit_debt_ceiling(c);
        bool ceiling_hit = (debt_final >= ceiling_final - CR_EPS && ceiling_final>CR_EPS);
        bool debt_meaningful = (debt_final > tune_f("DEBT_DEFAULT_THRESHOLD", 1500.f));
        bool in_default = fixed ? (ceiling_hit || (underpaid && debt_meaningful)) : ceiling_hit;
        if (in_default){
            if (g_debt[c].insolvent_streak < 30000) g_debt[c].insolvent_streak++;
        } else {
            g_debt[c].insolvent_streak = 0;
        }
        g_forced_pending[c] = (g_debt[c].insolvent_streak >= (int16_t)tune_f("BANKRUPTCY_GRACE_YEARS",2.f));
    }

    /* M3g — RÈGLEMENT ANNUEL DE LA SAISIE (part cité-état) : même cadence/motif que le
     * paiement d'intérêt cs ci-dessus (home_reg + province représentative). Le cumul
     * mensuel (credit_garnish_note, appelé depuis econ_tick/scps_econ.c) attend ici,
     * PAS d'accès World* dans econ_tick — la même contrainte que credit_settle_monthly.
     * DOIT s'exécuter AVANT que g_forced_pending (ci-dessus) ne déclenche une NOUVELLE
     * banqueroute cette même année (scps_sim.c appelle credit_bankruptcy juste APRÈS ce
     * retour) — sinon un reliquat de l'ANCIEN créancier serait perdu silencieusement ;
     * l'ordre est garanti : cette boucle tourne AVANT le retour de credit_year_tick. */
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float pend=g_garnish_cs_pending[c]; if (pend<=CR_EPS) continue;
        int L=g_garnish_cs_id[c];
        if (L>=0 && L<w->n_countries && L!=c){
            int hc=home_reg(w,L);
            if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) econ_prov_treasury_credit(e, cp, pend); }   /* MONNAIE M11 — A2 */
        }
        g_garnish_cs_pending[c]=0.f;
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
        /* V3 (MONNAIE M9) — L'ARCHÉTYPE du racheteur, pour la MÉTABOLISATION DISTINCTE
         * (télémétrie ICI, l'EFFET diplomatique/politique — rancor/faction_lever — dans
         * scps_sim.c juste après credit_year_tick, credit.c n'a pas DiploState/Statecraft).
         * Même priorité de classification que pick_lender/l'éligibilité ci-dessus : rôle
         * CITÉ-ÉTAT d'abord, sinon l'éthos MERCANTILE/PACIFISTE (les deux seuls autres
         * éligibles). RRACHAT_META<=0 : kill-switch — aucune classification/télémétrie/
         * effet en aval (golden pré-M9 byte-identique ; le RACHAT lui-même, M3c, continue
         * de fonctionner à l'IDENTIQUE — seule la distinction est coupée). */
        if (tune_f("RRACHAT_META", 1.0f) > 0.f){
            int arche;
            if (w->country[L].role==POLITY_CITY_STATE) arche=LOAN_ARCHETYPE_CITYSTATE;
            else {
                Ethos et=country_ethos(e,w,L);
                arche = (et==ETHOS_PACIFISTE) ? LOAN_ARCHETYPE_PACIFIST : LOAN_ARCHETYPE_MERCANTILE;
            }
            g_buyback_archetype[c]=(int8_t)arche;
            if (arche==LOAN_ARCHETYPE_CITYSTATE)      g_buyback_cs++;
            else if (arche==LOAN_ARCHETYPE_PACIFIST)  g_buyback_pacifist++;
            else                                       g_buyback_mercantile++;
        }
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
    /* M3g — fige le créancier D'AVANT-répudiation (part CS de la dette totale, AVANT le
     * wipe ci-dessous) pour toute la saisie à venir (cf. scps_credit.h). Ne réinstaure
     * RIEN — la répudiation reste TOTALE (motif M3d inchangé) : ceci mémorise seulement
     * QUI recevra la part cité-état de la production confisquée pendant la cicatrice. */
    float debt_total_pre = g_debt[c].to_class + g_debt[c].to_cs;
    if (debt_total_pre > CR_EPS && g_debt[c].to_cs > CR_EPS && L>=0){
        g_garnish_cs_id[c]    = (int16_t)L;
        g_garnish_cs_share[c] = g_debt[c].to_cs / debt_total_pre;
    } else {
        g_garnish_cs_id[c]    = -1;
        g_garnish_cs_share[c] = 0.f;
    }
    g_garnish_cs_pending[c] = 0.f;   /* un reliquat non réglé d'un cycle précédent est déjà réglé par credit_year_tick avant que g_forced_pending ne déclenche CE tick (ordre garanti, cf. scps_credit.h) */
    g_debt[c].to_class=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
    g_forced_pending[c]=false;
    /* CICATRICE (moral d'armée EXPLICITE, brief « l'humiliation ne se calcule pas en
     * grain » — M3g conserve CE malus tel quel) + le GATE de la SAISIE (M3g remplace le
     * débuff plat −75 % production/croissance de M3d par une confiscation de VALEUR,
     * scps_econ.c/scps_credit.h) : la cicatrice frappe TOUTES les provinces ACTIVES du
     * pays (motif revolt_scar — econ_tick la décroît sur BANKRUPTCY_SCAR_YEARS,
     * scps_campaign.c lit econ_country_bankruptcy_scar pour le moral d'armée). */
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    for (int p=0;p<n;p++) if (e->prov[p].owner==c && e->prov[p].active) e->prov[p].bankruptcy_scar=1.f;
    if (forced) g_bankrupt_forced++; else g_bankrupt_voluntary++;
    return L;
}
/* Flag TRANSIENT posé par credit_year_tick (streak au plafond ≥ BANKRUPTCY_GRACE_YEARS) —
 * scps_sim.c le lit juste après credit_year_tick et exécute credit_bankruptcy(e,c,true)
 * pour chaque pays flaggé (le flag redescend alors via credit_bankruptcy lui-même). */
bool credit_bankrupt_pending(int c){ return (c>=0 && c<SCPS_MAX_COUNTRY) && g_forced_pending[c]; }

/* V3 (MONNAIE M9) — voir scps_credit.h. Flag TRANSIENT posé par credit_year_tick (le RACHAT
 * DE CRÉDIT, plus haut) : LOAN_ARCHETYPE_NONE si le pays n'a subi/bénéficié d'AUCUN rachat
 * cette année. scps_sim.c le lit juste après credit_year_tick pour appliquer la
 * MÉTABOLISATION DISTINCTE (rancor pour une cité-état, faction_lever_apply pour un
 * pacifiste — mercantile ne reçoit RIEN de plus que l'intérêt annuel déjà uniforme, son
 * « profit pur » du brief). */
int credit_buyback_archetype(int debtor_c){
    return (debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY) ? (int)g_buyback_archetype[debtor_c] : LOAN_ARCHETYPE_NONE;
}
/* Télémétrie MONDE cumulée depuis credit_init (motif g_buybacks/g_defaults) : rachats PAR
 * ARCHÉTYPE — la preuve chiffrée que les 3 métabolisations sont bien distinctes (chronicle). */
void credit_buyback_stats(long *cs, long *mercantile, long *pacifist){
    if (cs)         *cs=g_buyback_cs;
    if (mercantile) *mercantile=g_buyback_mercantile;
    if (pacifist)   *pacifist=g_buyback_pacifist;
}

/* M3g — voir scps_credit.h. Lecture pure (aucune mutation) : la part de la saisie qui
 * ira à la cité-état créancière figée à la dernière banqueroute de `debtor_c`. */
float credit_garnish_cs_share(int debtor_c){
    if (debtor_c<0 || debtor_c>=SCPS_MAX_COUNTRY) return 0.f;
    return (g_garnish_cs_id[debtor_c]>=0) ? g_garnish_cs_share[debtor_c] : 0.f;
}
int credit_garnish_cs_id(int debtor_c){
    return (debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY) ? (int)g_garnish_cs_id[debtor_c] : -1;
}
float credit_garnish_cs_pending(int debtor_c){
    return (debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY) ? g_garnish_cs_pending[debtor_c] : 0.f;
}
/* M3g — voir scps_credit.h. `domestic_value` est déjà crédité par l'appelant
 * (scps_econ.c, wealth province-grain) : ici, TÉLÉMÉTRIE seule pour cette part. La part
 * `cs_value` (si >0) s'accumule pour le règlement annuel (credit_year_tick, ci-dessus). */
void credit_garnish_note(int debtor_c, float domestic_value, float cs_value){
    if (domestic_value<0.f) domestic_value=0.f;
    if (cs_value<0.f) cs_value=0.f;
    g_garnish_total    += (double)(domestic_value+cs_value);
    g_garnish_domestic += (double)domestic_value;
    g_garnish_cs_tel    += (double)cs_value;
    if (cs_value>CR_EPS && debtor_c>=0 && debtor_c<SCPS_MAX_COUNTRY)
        g_garnish_cs_pending[debtor_c] += cs_value;
}
void credit_garnish_stats(double *total, double *domestic, double *citystate){
    if (total)     *total=g_garnish_total;
    if (domestic)  *domestic=g_garnish_domestic;
    if (citystate) *citystate=g_garnish_cs_tel;
}

bool credit_save(FILE *f){
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (fwrite(&g_debt[c].to_class,        sizeof(float),  1,f)!=1) return false;
        if (fwrite(&g_debt[c].to_cs,           sizeof(float),  1,f)!=1) return false;
        if (fwrite(&g_debt[c].cs_id,           sizeof(int16_t),1,f)!=1) return false;
        if (fwrite(&g_debt[c].insolvent_streak,sizeof(int16_t),1,f)!=1) return false;   /* M3d (v90) */
    }
    /* M3g (v92) — le créancier figé de la saisie + son cumul mensuel en attente
     * (inter-tick, motif EMOB : doit survivre un save/reload pendant la cicatrice). */
    if (fwrite(g_garnish_cs_id,    sizeof g_garnish_cs_id,    1,f)!=1) return false;
    if (fwrite(g_garnish_cs_share, sizeof g_garnish_cs_share, 1,f)!=1) return false;
    if (fwrite(g_garnish_cs_pending,sizeof g_garnish_cs_pending,1,f)!=1) return false;
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
    if (fread(g_garnish_cs_id,    sizeof g_garnish_cs_id,    1,f)!=1) return false;      /* M3g (v92) */
    if (fread(g_garnish_cs_share, sizeof g_garnish_cs_share, 1,f)!=1) return false;
    if (fread(g_garnish_cs_pending,sizeof g_garnish_cs_pending,1,f)!=1) return false;
    return true;
}
