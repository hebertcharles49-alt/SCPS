/*
 * scps_credit.c — DETTE & PRÊTS — M3c : LE CRÉDIT RÉEL. Voir scps_credit.h.
 *
 * La dette n'a pas de plafond dette/revenu : le taux monte avec le levier et chaque
 * prêteur rationne selon ses réserves et son exposition. Un emprunt DÉPLACE des pièces qui
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
    /* MONNAIE M14 — B5 : VENTILÉE PAR ORDRE (SAVE_VERSION 96, ex-`to_class` agrégé) — un
     * emprunt 100 % bourgeois remboursait pourtant aux poids FIXES Élite/Bourgeois
     * (ELITE_LEND_WEIGHT/BOURGEOIS_LEND_WEIGHT) : les élites touchaient une créance
     * qu'elles n'avaient jamais avancée. `credit_debt_class(c)` reste l'AGRÉGAT
     * (to_elite+to_bourgeois) — contrat externe INCHANGÉ (save_sane, UI, chronicle). */
    float   to_elite;      /* dette due AUX ÉLITES du pays (ce qu'elles ont RÉELLEMENT prêté) */
    float   to_bourgeois;  /* dette due AUX BOURGEOIS du pays (ce qu'ils ont RÉELLEMENT prêté) */
    float   to_cs;        /* dette due à LA cité-état créancière (agrégé) */
    int16_t cs_id;         /* pays créancier cité-état, -1 = aucun */
    /* Années CONSÉCUTIVES d'échéance substantielle impayée APRÈS refinancement : la chronique
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
        g_debt[c].to_elite=0.f; g_debt[c].to_bourgeois=0.f; g_debt[c].to_cs=0.f;
        g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
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
/* B5 — l'AGRÉGAT (contrat externe INCHANGÉ : save_sane/UI/chronicle continuent de lire
 * "la dette aux classes", peu importe la ventilation interne par ordre). */
float credit_debt_class(int c)     { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_elite+g_debt[c].to_bourgeois : 0.f; }
/* B5 — LA VENTILATION PAR ORDRE (lecture pure, UI/télémétrie/save_sane) : ce que CHAQUE
 * ordre a RÉELLEMENT prêté à l'État — plus les poids fixes ELITE/BOURGEOIS_LEND_WEIGHT
 * (ceux-là restent la capacité de PRÊT, un concept différent de la créance DÉJÀ inscrite). */
float credit_debt_elite(int c)     { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_elite : 0.f; }
float credit_debt_bourgeois(int c) { return (c>=0&&c<SCPS_MAX_COUNTRY)? g_debt[c].to_bourgeois : 0.f; }
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
/* Le revenu fiscal annuel est la MESURE de solvabilité, jamais une autorisation. Il sert
 * au prix du risque ; aucun multiple de ce revenu ne ferme administrativement le marché. */
float credit_annual_revenue(int c){
    return fmaxf(0.f, econ_country_tax_year(c));
}
float credit_debt_ratio(int c){
    float revenue=fmaxf(credit_annual_revenue(c), tune_f("DEBT_REVENUE_FLOOR",200.f));
    return credit_debt_total(c)/fmaxf(revenue,1.f);
}
/* PRIME DE RISQUE CONVEXE : les premières années de revenu restent finançables ; au-delà,
 * chaque nouvelle tranche devient rapidement plus chère. Le plafond haut est purement
 * numérique (un contrat ne peut pas inscrire un float arbitraire), pas un plafond de dette. */
static float credit_rate_at(int c, float debt_total){
    float revenue=fmaxf(credit_annual_revenue(c), tune_f("DEBT_REVENUE_FLOOR",200.f));
    float lev=debt_total/fmaxf(revenue,1.f);
    float rate=tune_f("DEBT_RATE_BASE",0.02f)
              + tune_f("DEBT_RATE_LINEAR",0.015f)*lev
              + tune_f("DEBT_RATE_QUAD",0.0075f)*lev*lev;
    return clampf(rate, tune_f("DEBT_RATE_MIN",0.02f), tune_f("DEBT_RATE_MAX",0.50f));
}

/* V1 (MONNAIE M9) — le TAUX courant (lecture PURE, aucune mutation) : factorisé hors de
 * credit_year_tick (même formule EXACTE, M3d §3 — aucune constante changée) pour servir le
 * reader façade « capacité d'emprunt disponible par ordre » (taux proposé) sans dupliquer le
 * calcul. Pur refactor : credit_year_tick appelle désormais cette fonction au lieu d'inliner
 * la formule — comportement BIT-IDENTIQUE (même expression, mêmes tunables). */
float credit_current_rate(int c){
    return credit_rate_at(c, credit_debt_total(c));
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
static float credit_external_capacity(const WorldEconomy *e, const World *w, int c);
/* « Ligne de crédit » devient une lecture du marché : ce que les prêteurs physiques sont
 * prêts à avancer MAINTENANT. Elle ne dépend plus de la population du débiteur. */
float credit_line(const World *w, const WorldEconomy *e, int c){
    return credit_external_capacity(e,w,c);
}

/* éthos pays = éthos de la culture de sa région-capitale (convention scps_ai.c). */
static Ethos country_ethos(const WorldEconomy *e, const World *w, int c){
    int hr=home_reg(w,c);
    return (hr>=0&&hr<e->n_regions)? e->region[hr].culture.ethos : ETHOS_COUNT;
}
/* or NET d'un pays lu DIRECTEMENT sur prov[] (Σ) — contrepartie province-fraîche
 * d'econ_country_gold (qui lit region[], un DÉRIVÉ pas encore ré-agrégé juste après
 * une écriture prov[]). */
static double country_gold_prov(const WorldEconomy *e, int c){
    double g=0.0; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c) g+=e->prov[p].treasury;
    return g;
}

static float country_surplus(const WorldEconomy *e, int c, float floor_);
static void country_lendable(const WorldEconomy *e, int c, float ew, float bw,
                             float *cap_elite, float *cap_bourg);
static float state_lending_capacity_at(const WorldEconomy *e, int debtor, int lender,
                                       float debt_total_for_rate);
static float class_lending_capacity_at(const WorldEconomy *e, int c, SocialClass cls,
                                       float debt_total_for_rate);

/* Exposition inscrite au bilan d'un prêteur étranger. Les intérêts forfaitaires font
 * partie de la créance : ils consomment donc eux aussi la limite d'exposition. */
float credit_state_exposure(int debtor, int lender){
    if (debtor<0 || debtor>=SCPS_MAX_COUNTRY || lender<0 || lender>=SCPS_MAX_COUNTRY) return 0.f;
    return (g_debt[debtor].cs_id==lender) ? g_debt[debtor].to_cs : 0.f;
}
float credit_state_total_exposure(int lender){
    if (lender<0 || lender>=SCPS_MAX_COUNTRY) return 0.f;
    float total=0.f;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)
        if (g_debt[c].cs_id==lender) total+=g_debt[c].to_cs;
    return total;
}

/* PRÊTEUR AUTOMATIQUE : cité-État ou puissance mercantile/pacifiste. On choisit non plus
 * le plus gros trésor brut, mais la meilleure capacité APRÈS réserve, portefeuille total
 * et concentration sur ce débiteur. */
static int pick_lender_at(const WorldEconomy *e, const World *w, int c, float debt_for_rate){
    int best=-1; float best_room=0.f;
    for(int k=0;k<w->n_countries && k<SCPS_MAX_COUNTRY;k++){
        if(k==c) continue;
        bool lender=(w->country[k].role==POLITY_CITY_STATE);
        if(!lender){ Ethos et=country_ethos(e,w,k); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE); }
        if(!lender) continue;
        float room=state_lending_capacity_at(e,c,k,debt_for_rate);
        if (room>best_room){ best_room=room; best=k; }
    }
    return best;
}
static int pick_lender(const WorldEconomy *e, const World *w, int c){
    return pick_lender_at(e,w,c,credit_debt_total(c));
}

/* Capacité PHYSIQUE de la chaîne ad-hoc, sans mutation. La péréquation est absente :
 * déplacer des pièces entre provinces d'un même pays ne couvre jamais un déficit NET
 * national. Les deux sources de financement sont donc les classes, puis le prêteur
 * étranger, avec un taux recalculé après l'origination du premier étage. */
static int eligible_lender_at(const WorldEconomy *e, const World *w, int c, float debt_for_rate){
    int L=g_debt[c].cs_id;
    if (g_debt[c].to_cs>CR_EPS){
        /* Une créance étrangère existante ne change jamais silencieusement de propriétaire.
         * Si ce créancier ne refinance plus, le marché étranger est fermé jusqu'au rachat,
         * remboursement ou défaut. */
        if (L<0 || L>=w->n_countries || L==c) return -1;
        bool lender=(w->country[L].role==POLITY_CITY_STATE);
        if (!lender){ Ethos et=country_ethos(e,w,L); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE); }
        if (lender && state_lending_capacity_at(e,c,L,debt_for_rate)>CR_EPS) return L;
        return -1;
    }
    return pick_lender_at(e,w,c,debt_for_rate);
}
static float credit_external_capacity(const WorldEconomy *e, const World *w, int c){
    float debt0=credit_debt_total(c);
    float cap_e=class_lending_capacity_at(e,c,CLASS_ELITE,debt0);
    float cap_b=class_lending_capacity_at(e,c,CLASS_BOURGEOIS,debt0);
    float from_classes=cap_e+cap_b;
    float debt1=debt0;
    if (from_classes>CR_EPS){
        float factor=(tune_f("DEBT_FIXED",1.f)>0.f) ? 1.f+credit_rate_at(c,debt0) : 1.f;
        debt1 += from_classes*factor;
    }
    int L=eligible_lender_at(e,w,c,debt1);
    float from_state=(L>=0)?state_lending_capacity_at(e,c,L,debt1):0.f;
    return from_classes+from_state;
}
bool credit_can_spend(const WorldEconomy *e, const World *w, int c, float cost){
    if (!e || !w || c<0 || c>=w->n_countries || c>=SCPS_MAX_COUNTRY || cost<0.f) return false;
    int hr=home_reg(w,c);
    if (hr<0 || hr>=e->n_regions || econ_region_rep_province(e,hr)<0) return false;
    if (cost<=CR_EPS) return true;
    double gold=country_gold_prov(e,c);
    double need=(double)cost-gold;
    return need<=CR_EPS || need<=(double)credit_external_capacity(e,w,c)+CR_EPS;
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

/* Capacité d'un prêteur d'État. Le capital prêtable est son surplus liquide PLUS ses
 * créances encore vivantes. Deux bornes distinctes s'appliquent : taille du portefeuille
 * total, puis concentration sur le débiteur. `room_face` est une valeur de créance ; on
 * réserve le markup du nouveau contrat pour renvoyer des pièces réellement transférables. */
static float state_face_room(const WorldEconomy *e, int debtor, int lender){
    if (!e || debtor<0 || debtor>=SCPS_MAX_COUNTRY || lender<0 || lender>=SCPS_MAX_COUNTRY
        || debtor==lender) return 0.f;
    if (g_debt[debtor].to_cs>CR_EPS && g_debt[debtor].cs_id!=lender) return 0.f;
    float liquid=country_surplus(e,lender,tune_f("SINK_FLOOR",500.f));
    float total_exp=credit_state_total_exposure(lender);
    float debtor_exp=credit_state_exposure(debtor,lender);
    float capital=liquid+total_exp;
    float total_limit=capital*tune_f("LENDER_PORTFOLIO_SHARE",0.75f);
    float debtor_limit=capital*tune_f("LENDER_DEBTOR_SHARE",0.35f);
    float total_room=fmaxf(0.f,total_limit-total_exp);
    float debtor_room=fmaxf(0.f,debtor_limit-debtor_exp);
    return fminf(total_room,debtor_room);
}
static float state_lending_capacity_at(const WorldEconomy *e, int debtor, int lender,
                                       float debt_total_for_rate){
    float liquid=country_surplus(e,lender,tune_f("SINK_FLOOR",500.f));
    float draw=liquid*tune_f("CITYSTATE_LEND_SHARE",0.5f);
    float room_face=state_face_room(e,debtor,lender);
    float factor=(tune_f("DEBT_FIXED",1.f)>0.f)?1.f+credit_rate_at(debtor,debt_total_for_rate):1.f;
    return fmaxf(0.f,fminf(draw,room_face/fmaxf(factor,1.f)));
}
float credit_state_borrow_capacity(const WorldEconomy *e, int debtor, int lender){
    return state_lending_capacity_at(e,debtor,lender,credit_debt_total(debtor));
}
float credit_state_liquid_surplus(const WorldEconomy *e, int lender){
    if (!e || lender<0 || lender>=SCPS_MAX_COUNTRY) return 0.f;
    return country_surplus(e,lender,tune_f("SINK_FLOOR",500.f));
}
float credit_state_exposure_limit(const WorldEconomy *e, int debtor, int lender){
    return credit_state_exposure(debtor,lender)+state_face_room(e,debtor,lender);
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

/* Les ordres ont eux aussi une exposition finie. Sans cette borne, un refinancement par
 * la même classe rendrait les pièces puis les reprêterait indéfiniment, tandis que la
 * créance grossirait du markup. Le patrimoine liquide et la créance forment son capital ;
 * l'État ne peut en absorber qu'une fraction. */
static float class_lending_capacity_at(const WorldEconomy *e, int c, SocialClass cls,
                                       float debt_total_for_rate){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY || (cls!=CLASS_ELITE && cls!=CLASS_BOURGEOIS)) return 0.f;
    float ew=tune_f("ELITE_LEND_WEIGHT",1.f), bw=tune_f("BOURGEOIS_LEND_WEIGHT",0.5f);
    float cap_e=0.f, cap_b=0.f; country_lendable(e,c,ew,bw,&cap_e,&cap_b);
    float liquid=(cls==CLASS_ELITE)?cap_e:cap_b;
    float exposure=(cls==CLASS_ELITE)?g_debt[c].to_elite:g_debt[c].to_bourgeois;
    float capital=liquid+exposure;
    float limit=capital*tune_f("CLASS_EXPOSURE_SHARE",0.50f);
    float room_face=fmaxf(0.f,limit-exposure);
    float factor=(tune_f("DEBT_FIXED",1.f)>0.f)?1.f+credit_rate_at(c,debt_total_for_rate):1.f;
    float per_draw=liquid*tune_f("CLASS_LEND_SHARE",0.05f);
    return fmaxf(0.f,fminf(per_draw,room_face/fmaxf(factor,1.f)));
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
        float part=amount*(w_/tot);
        e->prov[p].strata[cls].wealth -= part;
        int r=e->prov[p].region;
        if (r>=0 && r<e->n_regions) e->region[r].strata[cls].wealth -= part;
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
            float part=amount*(w_/tot);
            e->prov[p].strata[cls].wealth += part;
            int r=e->prov[p].region;
            if (r>=0 && r<e->n_regions) e->region[r].strata[cls].wealth += part;
        }
    } else {
        float each=amount/(float)nactive;
        for(int p=0;p<n;p++) if(e->prov[p].owner==c && e->prov[p].active && e->prov[p].colonized){
            e->prov[p].strata[cls].wealth += each;
            int r=e->prov[p].region;
            if (r>=0 && r<e->n_regions) e->region[r].strata[cls].wealth += each;
        }
    }
}

static float credit_borrow_classes(WorldEconomy *e, int c, float need){
    if (!e || c<0 || c>=SCPS_MAX_COUNTRY || need<=CR_EPS) return 0.f;
    float debt0=credit_debt_total(c);
    float cap_e=class_lending_capacity_at(e,c,CLASS_ELITE,debt0);
    float cap_b=class_lending_capacity_at(e,c,CLASS_BOURGEOIS,debt0);
    float cap_tot=cap_e+cap_b;
    if (cap_tot<=CR_EPS) return 0.f;
    float borrow=fminf(need,cap_tot);
    float b_elite=borrow*(cap_e/cap_tot), b_bourg=borrow-b_elite;
    if (b_elite>CR_EPS) debit_wealth_prorata(e,c,CLASS_ELITE,b_elite);
    if (b_bourg>CR_EPS) debit_wealth_prorata(e,c,CLASS_BOURGEOIS,b_bourg);
    float origination=debt_origination(c,borrow);
    if (b_elite>CR_EPS) g_debt[c].to_elite += origination*(b_elite/borrow);
    if (b_bourg>CR_EPS) g_debt[c].to_bourgeois += origination*(b_bourg/borrow);
    return borrow;
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
    float rem=need-covered;
    if (rem>CR_EPS) covered += credit_borrow_classes(e,c,rem);
    return covered;
}

/* ---- V1 (MONNAIE M9) : LE VERBE « EMPRUNTER À UN ORDRE » (panneau éco) --------------- */

/* Capacité d'emprunt DISPONIBLE (lecture PURE, aucune mutation) pour la classe `cls` du pays
 * `c` — l'intersection de la capacité PAR CLASSE (même formule que credit_borrow_local §2 :
 * CLASS_LEND_SHARE × richesse pondérée ELITE/BOURGEOIS_LEND_WEIGHT) et de son exposition
 * de l'EXPOSITION de cet ordre (class_lending_capacity_at). CLASS_LABORER/
 * CLASS_SLAVE : toujours 0 (aucune épargne, motif M3c déjà établi "les laborers n'ont pas
 * d'épargne") — la fiche UI grise le bouton, aucun refus à coder côté verbe (décision joueur :
 * « l'ordre ne refuse pas », il n'y a simplement rien à prêter). Sert le reader façade
 * scps_country_loan_capacity (montant max + credit_current_rate, le taux proposé). */
float credit_class_borrow_capacity(const WorldEconomy *e, int c, SocialClass cls){
    return class_lending_capacity_at(e,c,cls,credit_debt_total(c));
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
    /* MONNAIE M14 — B5 : ce verbe cible UNE SEULE classe (cls, ELITE ou BOURGEOIS déjà
     * garanti par credit_class_borrow_capacity ci-dessus) — la créance va DIRECTEMENT à
     * son ordre réel, jamais l'agrégat. */
    if (cls==CLASS_ELITE) g_debt[c].to_elite     += debt_origination(c, borrow);
    else                  g_debt[c].to_bourgeois += debt_origination(c, borrow);   /* MONNAIE M11 — A3 v2 : forfait figé */
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
    int L=eligible_lender_at(e,w,c,credit_debt_total(c));
    if (L<0) return 0.f;
    float borrow=fminf(need,state_lending_capacity_at(e,c,L,credit_debt_total(c)));
    if (borrow<=CR_EPS) return 0.f;
    debit_surplus_prorata(e,L,tune_f("SINK_FLOOR",500.f),borrow);   /* DÉBIT seul (même raison que credit_borrow_local) */
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

/* Dépense ad-hoc TOUT OU RIEN. Le préflight lit la vérité province-grain, vérifie la
 * ligne de crédit ET les fonds physiques réellement mobilisables. Un déficit NET ne
 * peut être financé que par les classes puis un prêteur étranger : la péréquation entre
 * provinces du même pays n'ajoute aucune pièce au solde national. Toutes les écritures
 * tiennent les vues province/région en phase ; le journal ci-dessous garantit un rollback
 * exact si une future modification faisait diverger préflight et exécution. */
bool credit_spend(WorldEconomy *e, const World *w, int c, float cost){
    if (!credit_can_spend(e,w,c,cost)) return false;
    if (cost<=CR_EPS) return true;
    int hr=home_reg(w,c); if(hr<0||hr>=e->n_regions) return false;
    int pid=econ_region_rep_province(e, hr); if(pid<0||pid>=e->n_prov) return false;
    /* Journal minimal de transaction. Le préflight et l'exécution utilisent les mêmes
     * formules, donc le rollback ne doit jamais servir en régime normal ; il garantit
     * néanmoins le contrat tout-ou-rien si une future source de financement diverge. */
    float p_treas[SCPS_MAX_PROV], p_elite[SCPS_MAX_PROV], p_bourg[SCPS_MAX_PROV];
    float r_treas[SCPS_MAX_REG], r_elite[SCPS_MAX_REG], r_bourg[SCPS_MAX_REG];
    int np=e->n_prov; if (np>SCPS_MAX_PROV) np=SCPS_MAX_PROV;
    int nr=e->n_regions; if (nr>SCPS_MAX_REG) nr=SCPS_MAX_REG;
    for (int p=0;p<np;p++){
        p_treas[p]=e->prov[p].treasury;
        p_elite[p]=e->prov[p].strata[CLASS_ELITE].wealth;
        p_bourg[p]=e->prov[p].strata[CLASS_BOURGEOIS].wealth;
    }
    for (int r=0;r<nr;r++){
        r_treas[r]=e->region[r].treasury;
        r_elite[r]=e->region[r].strata[CLASS_ELITE].wealth;
        r_bourg[r]=e->region[r].strata[CLASS_BOURGEOIS].wealth;
    }
    CountryDebt debt_before=g_debt[c];
    econ_prov_treasury_credit(e, pid, -cost);
    float short_=(float)(-country_gold_prov(e,c));   /* découvert NET du pays, s'il y en a un */
    if (short_>CR_EPS){
        /* Déficit NATIONAL : uniquement des financements EXTERNES au trésor national.
         * La péréquation de credit_borrow_local serait un faux financement ici. */
        float covered=credit_borrow_classes(e,c,short_);
        float rem=short_-covered;
        if (rem>CR_EPS) covered += credit_borrow_citystate(e,w,c,rem);
        /* le trésor RÉEL couvert doit revenir dans la province représentative (c'est
         * elle qui a essuyé le débit ci-dessus) — les autres provinces/classes/prêteurs
         * ont, eux, été DÉBITÉS par la chaîne (péréquation/classes/cité-état). */
        econ_prov_treasury_credit(e, pid, covered);
        /* credit_can_spend a simulé exactement ces deux étages ; aucun état financier
         * n'a changé entre les deux appels. Cette garde détecte une régression plutôt que
         * de transformer à nouveau l'action en paiement partiel. */
        if (covered+CR_EPS < short_){
            for (int p=0;p<np;p++){
                e->prov[p].treasury=p_treas[p];
                e->prov[p].strata[CLASS_ELITE].wealth=p_elite[p];
                e->prov[p].strata[CLASS_BOURGEOIS].wealth=p_bourg[p];
            }
            for (int r=0;r<nr;r++){
                e->region[r].treasury=r_treas[r];
                e->region[r].strata[CLASS_ELITE].wealth=r_elite[r];
                e->region[r].strata[CLASS_BOURGEOIS].wealth=r_bourg[r];
            }
            g_debt[c]=debt_before;
            return false;
        }
    }
    return true;
}

/* ---- V2 (MONNAIE M9) : LA DEMANDE D'EMPRUNT DIPLOMATIQUE (diplomatie) ---------------- */

/* Le CONSENTEMENT est évalué par l'APPELANT (scps_sim.c a Statecraft/DiploState ; credit.c ne
 * les a pas, motif M3d §2 « le refus des cités-états, le motif existant ») via
 * ai_consider_offer/OFFER_LOAN (scps_ai.c) — cette fonction ASSUME le consentement DÉJÀ acquis
 * et ne fait QUE le transfert réel, au MÊME motif que credit_borrow_citystate (débit réel du
 * prêteur, réserve+exposition du créancier) mais LE CRÉANCIER EST CHOISI PAR LE JOUEUR (pas
 * pick_lender) — n'importe quel État étranger, pas seulement cité-état/mercantile/pacifiste
 * (réservés à V3, le RACHAT, ci-dessous). Un SEUL créancier étranger à la fois (motif « un
 * SEUL créancier-cité-état par pays », M3c) : refuse si le débiteur a déjà un AUTRE créancier
 * engagé (`to_cs`>0 sous un cs_id différent) — il doit d'abord s'acquitter/laisser s'éteindre
 * l'ancien avant d'en solliciter un nouveau. TRANSFERT COMPLET (motif credit_borrow_class, V1
 * ci-dessus) : DÉBITE le prêteur ET CRÉDITE le trésor du débiteur (contrairement à
 * credit_borrow_citystate qui ne fait que DÉBITER pour combler un besoin déjà tracé ailleurs
 * par l'appelant — ce verbe est autonome). Retourne le montant RÉELLEMENT prêté (0 =
     * impossible : déjà un autre créancier, réserve ou exposition du prêteur épuisée). */
float credit_borrow_state(WorldEconomy *e, const World *w, int debtor_c, int lender_c, float amount){
    if (!e || !w || debtor_c<0 || debtor_c>=SCPS_MAX_COUNTRY || lender_c<0 || lender_c>=SCPS_MAX_COUNTRY
        || debtor_c==lender_c) return 0.f;
    if (g_debt[debtor_c].to_cs>CR_EPS && g_debt[debtor_c].cs_id>=0 && g_debt[debtor_c].cs_id!=(int16_t)lender_c)
        return 0.f;   /* déjà engagé avec un AUTRE créancier étranger */
    /* amount<=0 ⇒ le MAXIMUM réellement disponible chez CE prêteur. */
    float avail=credit_state_borrow_capacity(e,debtor_c,lender_c);
    float borrow=(amount>CR_EPS)?fminf(amount,avail):avail;
    if (borrow<=CR_EPS) return 0.f;
    debit_surplus_prorata(e,lender_c,tune_f("SINK_FLOOR",500.f),borrow);
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
 * puis REFINANCÉE tant qu'un prêteur l'accepte ; le reliquat nourrit le streak
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
        /* Photo des créances D'AVANT service. Un refinancement crée une NOUVELLE créance
         * avant que l'ancienne échéance soit versée ; les bénéficiaires du versement restent
         * pourtant les anciens créanciers, dans leurs proportions d'avant refinancement. */
        float old_elite=g_debt[c].to_elite, old_bourg=g_debt[c].to_bourgeois, old_cs=g_debt[c].to_cs;
        int old_cs_id=g_debt[c].cs_id;
        float debt_total = old_elite + old_bourg + old_cs;
        if (debt_total<=CR_EPS){
            g_debt[c].to_elite=0.f; g_debt[c].to_bourgeois=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1;
            g_debt[c].insolvent_streak=0; g_forced_pending[c]=false;   /* M3d : pas de dette, pas de chronique */
            continue;
        }
        /* MONNAIE M11 — A3 v2 : L'INTÉRÊT FIXE + L'ÉCHÉANCE MINIMALE (voir DEBT_FIXED,
         * scps_tune_list.h — décision joueur, remplace le service d'intérêt ANNUEL composé
         * de M3d). `fixed` : chaque prêt porte déjà son markup (debt_origination, figé à
         * l'origination) — plus AUCUNE rente annuelle sur le stock ; à la place, une
         * ÉCHÉANCE MINIMALE (DEBT_DUE_FRAC × stock) est DUE chaque année, payée du surplus
         * puis refinancée, et RÉDUIT le stock payé (elle ÉTEINT une
         * part de la créance — motif amortissement). `!fixed` (kill-switch) : le service
         * d'intérêt ANNUEL M3d pré-M11 EXACT (le stock ne bouge JAMAIS, l'intérêt ne
         * rembourse rien). */
        bool fixed = tune_f("DEBT_FIXED", 1.0f) > 0.f;
        float rate_now = credit_current_rate(c);   /* V1 (M9) — factorisé, formule M3d INCHANGÉE */
        float charge = fixed ? (debt_total * tune_f("DEBT_DUE_FRAC", 0.10f)) : (debt_total * rate_now);

        /* Le service puise d'abord dans le surplus du débiteur. Le reliquat peut être
         * REFINANCÉ : les prêteurs avancent alors de vraies pièces, qui repartent aussitôt
         * vers les créanciers de l'ancienne tranche. Avec le même créancier, le mouvement
         * de trésor s'annule mais son exposition augmente du nouveau contrat — exactement
         * un rollover. Une exposition saturée ferme donc ce canal sans prudence artificielle
         * du débiteur. */
        float avail=country_surplus(e,c,floor_);
        float cash_paid=fminf(charge, avail);
        if (cash_paid>CR_EPS) debit_surplus_prorata(e,c,floor_,cash_paid);
        float refinanced=0.f, rem_due=charge-cash_paid;
        if (rem_due>CR_EPS){
            refinanced=credit_borrow_classes(e,c,rem_due);
            rem_due-=refinanced;
            if (rem_due>CR_EPS) refinanced+=credit_borrow_citystate(e,w,c,rem_due);
        }
        float covered=cash_paid+refinanced;
        bool underpaid = (covered+CR_EPS < charge);
        if (underpaid) g_defaults++;   /* échéance/intérêt de l'année sous-servi (auto-limité) */

        if (covered>CR_EPS){
            float old_class=old_elite+old_bourg;
            float i_class=covered*(old_class/debt_total);
            float i_cs   =covered-i_class;
            /* MONNAIE M14 — B5 : ventilé ∝ la ventilation RÉELLE de la créance (ce que
             * CHAQUE ordre a EFFECTIVEMENT prêté) — plus les poids fixes ELITE/BOURGEOIS_
             * LEND_WEIGHT (l'ancien bug : un emprunt 100 % bourgeois remboursait quand même
             * l'élite à son poids fixe, même en l'absence de toute créance élite). Calculé
             * ICI (portée du bloc `covered`) pour servir À LA FOIS le versement (ci-dessous)
             * ET la réduction du passif (plus bas, même ratio — sinon la créance se réduit
             * sur une clé différente de celle qui a été payée). */
            float share_e=(old_class>CR_EPS)?(old_elite/old_class):0.5f;
            if (i_class>CR_EPS){   /* l'ÉLITE RENTIÈRE (et le bourgeois-créancier) vivent du flux de remboursement */
                float amt_e=i_class*share_e, amt_b=i_class-amt_e;
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
            if (i_cs>CR_EPS && old_cs_id>=0){
                int hc=home_reg(w,old_cs_id);
                if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) econ_prov_treasury_credit(e, cp, i_cs); }   /* MONNAIE M11 — A2 */
            }
            /* `fixed` : le remboursement ÉTEINT une part du stock (principal+markup blended,
             * motif amortissement ci-dessous) — « rembourses 1050 » se solde en RÉDUISANT la
             * créance de ce qui a été payé. `!fixed` (legacy) : le stock NE bouge PAS ici
             * (l'intérêt pré-M11 ne remboursait rien — comportement EXACT, cf. le contrôle
             * historique « le principal n'a pas grossi rien qu'à payer l'intérêt »). */
            if (fixed){
                /* B5 : réduit CHAQUE créance de sa part RÉELLE de i_class (share_e, la MÊME
                 * clé que le versement ci-dessus) — jamais l'agrégat. */
                float i_e=i_class*share_e, i_b=i_class-i_e;
                g_debt[c].to_elite     -= i_e; if (g_debt[c].to_elite<0.f)     g_debt[c].to_elite=0.f;
                g_debt[c].to_bourgeois -= i_b; if (g_debt[c].to_bourgeois<0.f) g_debt[c].to_bourgeois=0.f;
                g_debt[c].to_cs        -= i_cs; if (g_debt[c].to_cs<0.f)       g_debt[c].to_cs=0.f;
            }
        }
        econ_flux_add(c, FX_CREDIT, -covered);   /* I0 : la ligne intérêts/échéance (montant RÉELLEMENT servi) */

        /* AMORTISSEMENT — un pays au trésor GRAS rembourse une part du PRINCIPAL depuis
         * son surplus (au-dessus de COURT_FLOOR, le seuil de hoarding — même réserve que
         * le faste de cour) : "la dette VIT" — elle ne fait pas que grossir.
         * MONNAIE M14 — B3 : `debt_total` ci-dessus est capturé AVANT l'échéance (ligne
         * 549) — l'échéance vient de RÉDUIRE g_debt[c].to_class/to_cs (ci-dessus, `fixed`)
         * mais PAS `debt_total` lui-même (variable locale, jamais réassignée). Répartir
         * `repay` avec `g_debt[c].to_class/debt_total` (numérateur POST-échéance, dénominateur
         * PRÉ-échéance) désaligne la fraction : dette 100 % classes → échéance 10 → to_class
         * passe à 90 → amortissement 10 réparti 10×90/100=9 aux classes + 1 à la branche
         * cité-état MÊME SANS dette étrangère (r_cs=1, jamais crédité : cs_id resterait -1
         * tant qu'aucun emprunt étranger n'a eu lieu) — le débiteur paie 10 (debit_surplus_
         * prorata sur `repay`=10 intégral), les créanciers ne reçoivent que 9 : 1 unité de
         * monnaie DÉTRUITE par tick, chaque année, pour tout pays qui amortit. Recapturer
         * `debt_total` ICI (après l'échéance, juste avant l'amortissement) referme l'écart :
         * numérateur et dénominateur redeviennent la MÊME photo. */
        float debt_total_amort = g_debt[c].to_elite + g_debt[c].to_bourgeois + g_debt[c].to_cs;
        float hof=tune_f("COURT_FLOOR",4000.f);
        float surplus=country_surplus(e,c,hof);
        if (surplus>CR_EPS && debt_total_amort>CR_EPS){
            float repay=fminf(debt_total_amort, fminf(surplus, debt_total_amort*tune_f("PRINCIPAL_REPAY_RATE",0.10f)));
            if (repay>CR_EPS){
                debit_surplus_prorata(e,c,hof,repay);
                float class_tot_a=g_debt[c].to_elite+g_debt[c].to_bourgeois;
                float r_class=repay*(class_tot_a/debt_total_amort), r_cs=repay-r_class;
                /* MONNAIE M14 — B5 : même correctif que l'échéance ci-dessus — ventilé ∝ la
                 * ventilation RÉELLE de la créance (share_a), plus les poids fixes. */
                float share_a=(class_tot_a>CR_EPS)?(g_debt[c].to_elite/class_tot_a):0.5f;
                float r_e=r_class*share_a, r_b=r_class-r_e;
                if (r_class>CR_EPS){
                    credit_wealth_prorata(e,c,CLASS_ELITE,     r_e);
                    credit_wealth_prorata(e,c,CLASS_BOURGEOIS, r_b);
                }
                if (r_cs>CR_EPS && g_debt[c].cs_id>=0){
                    int hc=home_reg(w,g_debt[c].cs_id);
                    if (hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if (cp>=0&&cp<e->n_prov) econ_prov_treasury_credit(e, cp, r_cs); }   /* MONNAIE M11 — A2 */
                }
                g_debt[c].to_elite     -= r_e; if (g_debt[c].to_elite<0.f)     g_debt[c].to_elite=0.f;
                g_debt[c].to_bourgeois -= r_b; if (g_debt[c].to_bourgeois<0.f) g_debt[c].to_bourgeois=0.f;
                g_debt[c].to_cs        -= r_cs; if (g_debt[c].to_cs<0.f)       g_debt[c].to_cs=0.f;
            }
        }
        if (g_debt[c].to_cs<=CR_EPS) g_debt[c].cs_id=-1;

        /* LA BANQUEROUTE FORCÉE : une échéance reste chroniquement IMPAYÉE SUR UNE DETTE
         * SUBSTANTIELLE APRÈS que les prêteurs ont refusé de la refinancer. Le streak
         * réagit à `underpaid` (échéance encore manquée CETTE année) — MAIS
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
         * Motif des grâces existantes (g_lowsat_streak/g_colony_cd, EMOB/COLC) : un
         * compteur d'années CONSÉCUTIVES en défaut (ré-évalué APRÈS échéance/intérêt+
         * amortissement de CETTE année) — BANKRUPTCY_GRACE_YEARS (registre J) de répit avant
         * le couperet, jamais un pic isolé. g_forced_pending est un flag TRANSIENT : scps_
         * sim.c l'exécute juste après credit_year_tick (RAZ dette + cicatrice + effet diplo,
         * motif CMD_MANUMIT — sim.c orchestre, credit.c fait le cœur). */
        float debt_final = g_debt[c].to_elite + g_debt[c].to_bourgeois + g_debt[c].to_cs;
        bool debt_meaningful = (debt_final > tune_f("DEBT_DEFAULT_THRESHOLD", 1500.f));
        bool in_default = underpaid && debt_meaningful;
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
        float to_class_tot=g_debt[c].to_elite+g_debt[c].to_bourgeois;
        if (to_class_tot<=bthresh) continue;
        int L=-1;
        if (g_debt[c].cs_id>=0){
            int cand=g_debt[c].cs_id;
            bool lender=(cand>=0 && cand<w->n_countries && cand!=c && w->country[cand].role==POLITY_CITY_STATE);
            if (!lender && cand>=0 && cand<w->n_countries && cand!=c){
                Ethos et=country_ethos(e,w,cand); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE);
            }
            if (lender && state_face_room(e,c,cand)>CR_EPS) L=cand;
        } else {
            L=pick_lender(e,w,c);   /* aucun créancier encore assigné : le plus de capacité nette */
        }
        if (getenv("SCPS_BUYBACKDIAG"))
            fprintf(stderr,"[BUYBACKDIAG] c=%d to_class=%.0f cs_id=%d L=%d surplus(L)=%.0f\n",
                    c, (double)to_class_tot, g_debt[c].cs_id, L, L>=0?country_surplus(e,L,ifloor):-1.f);
        if (L<0) continue;
        float idle=country_surplus(e,L,ifloor)*ishare;
        float amount=fminf(to_class_tot, fminf(idle,state_face_room(e,c,L)));
        if (amount<=CR_EPS) continue;
        debit_surplus_prorata(e,L,ifloor,amount);            /* le racheteur paie face value */
        /* MONNAIE M14 — B5 : les créanciers CASHÉS OUT sont les VRAIS prêteurs (∝ ce que
         * CHACUN a réellement avancé), plus les poids fixes ELITE/BOURGEOIS_LEND_WEIGHT. */
        float share_bb=(to_class_tot>CR_EPS)?(g_debt[c].to_elite/to_class_tot):0.5f;
        float amt_e_bb=amount*share_bb, amt_b_bb=amount-amt_e_bb;
        credit_wealth_prorata(e,c,CLASS_ELITE,     amt_e_bb);
        credit_wealth_prorata(e,c,CLASS_BOURGEOIS, amt_b_bb);
        g_debt[c].to_elite     -= amt_e_bb; if (g_debt[c].to_elite<0.f)     g_debt[c].to_elite=0.f;
        g_debt[c].to_bourgeois -= amt_b_bb; if (g_debt[c].to_bourgeois<0.f) g_debt[c].to_bourgeois=0.f;
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
    float debt_total_pre = g_debt[c].to_elite + g_debt[c].to_bourgeois + g_debt[c].to_cs;
    if (debt_total_pre > CR_EPS && g_debt[c].to_cs > CR_EPS && L>=0){
        g_garnish_cs_id[c]    = (int16_t)L;
        g_garnish_cs_share[c] = g_debt[c].to_cs / debt_total_pre;
    } else {
        g_garnish_cs_id[c]    = -1;
        g_garnish_cs_share[c] = 0.f;
    }
    g_garnish_cs_pending[c] = 0.f;   /* un reliquat non réglé d'un cycle précédent est déjà réglé par credit_year_tick avant que g_forced_pending ne déclenche CE tick (ordre garanti, cf. scps_credit.h) */
    g_debt[c].to_elite=0.f; g_debt[c].to_bourgeois=0.f; g_debt[c].to_cs=0.f; g_debt[c].cs_id=-1; g_debt[c].insolvent_streak=0;
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
/* Flag TRANSIENT posé par credit_year_tick (streak d'impayés ≥ BANKRUPTCY_GRACE_YEARS) —
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
        /* MONNAIE M14 — B5 (v96) : to_class agrégé → to_elite+to_bourgeois (ventilé PAR
         * ORDRE) — struct CountryDebt AGRANDIE, SAVE_VERSION bumpé. */
        if (fwrite(&g_debt[c].to_elite,        sizeof(float),  1,f)!=1) return false;
        if (fwrite(&g_debt[c].to_bourgeois,    sizeof(float),  1,f)!=1) return false;
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
        if (fread(&g_debt[c].to_elite,        sizeof(float),  1,f)!=1) return false;    /* B5 (v96) */
        if (fread(&g_debt[c].to_bourgeois,    sizeof(float),  1,f)!=1) return false;    /* B5 (v96) */
        if (fread(&g_debt[c].to_cs,           sizeof(float),  1,f)!=1) return false;
        if (fread(&g_debt[c].cs_id,           sizeof(int16_t),1,f)!=1) return false;
        if (fread(&g_debt[c].insolvent_streak,sizeof(int16_t),1,f)!=1) return false;    /* M3d (v90) */
    }
    if (fread(g_garnish_cs_id,    sizeof g_garnish_cs_id,    1,f)!=1) return false;      /* M3g (v92) */
    if (fread(g_garnish_cs_share, sizeof g_garnish_cs_share, 1,f)!=1) return false;
    if (fread(g_garnish_cs_pending,sizeof g_garnish_cs_pending,1,f)!=1) return false;
    return true;
}
