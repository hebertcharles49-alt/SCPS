/*
 * scps_credit.c — DETTE & PRÊTS (incrément 1). Voir scps_credit.h.
 *
 * Le plafond n'est pas une constante : il ÉMERGE de la capacité à rembourser (taille
 * éco). Dépenser au-delà du trésor creuse une dette ; l'intérêt annuel la fait grossir
 * (taux ∝ ratio de dette + chute de légitimité) et CRÉDITE le prêteur — la cité-état
 * vit enfin de tes intérêts. Soldé → le lien se dénoue.
 */
#include "scps_credit.h"
#include "scps_culture.h"
#include "scps_types.h"
#include "scps_tune.h"
#include <stdint.h>

static int16_t g_creditor[SCPS_MAX_COUNTRY];

void credit_init(void){ for(int c=0;c<SCPS_MAX_COUNTRY;c++) g_creditor[c]=-1; }
int  credit_of(int c){ return (c>=0&&c<SCPS_MAX_COUNTRY)?g_creditor[c]:-1; }

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
/* PLAFOND ÉMERGENT — capacité à rembourser ∝ taille éco (pop). La légitimité entre dans
 * le TAUX (credit_year_tick) ; le gate de dépense, lui, ne lit que la taille. */
float credit_line(const World *w, const WorldEconomy *e, int c){
    (void)w; return tune_f("CREDIT_LINE_BASE",0.5f) * (float)cpop(e,c);
}
bool credit_can_spend(const WorldEconomy *e, const World *w, int c, float cost){
    return econ_country_gold(e,c) - (double)cost >= -(double)credit_line(w,e,c);
}

/* éthos pays = éthos de la culture de sa région-capitale (convention scps_ai.c). */
static Ethos country_ethos(const WorldEconomy *e, const World *w, int c){
    int hr=home_reg(w,c);
    return (hr>=0&&hr<e->n_regions)? e->region[hr].culture.ethos : ETHOS_COUNT;
}
/* PRÊTEUR : cité-état OU mercantile/pacifiste, solvable (or>0), ≠ c. Le plus riche. */
static int pick_lender(const WorldEconomy *e, const World *w, int c){
    int best=-1; double bestg=0.0;
    for(int k=0;k<w->n_countries && k<SCPS_MAX_COUNTRY;k++){
        if(k==c) continue;
        bool lender=(w->country[k].role==POLITY_CITY_STATE);
        if(!lender){ Ethos et=country_ethos(e,w,k); lender=(et==ETHOS_MERCANTILE||et==ETHOS_PACIFISTE); }
        if(!lender) continue;
        double g=econ_country_gold(e,k); if(g<=0.0) continue;       /* pas de surplus → ne prête pas */
        if(g>bestg){ bestg=g; best=k; }
    }
    return best;
}
/* or NET d'un pays lu DIRECTEMENT sur prov[] (Σ) — contrepartie province-fraîche
 * d'econ_country_gold (qui lit region[], un DÉRIVÉ pas encore ré-agrégé juste après
 * une écriture prov[] ; cf. credit_spend ci-dessous, qui a besoin du total À JOUR
 * AU MÊME APPEL, avant qu'econ_tick ne ré-agrège). */
static double country_gold_prov(const WorldEconomy *e, int c){
    double g=0.0; int n=e->n_prov; if(n>SCPS_MAX_PROV)n=SCPS_MAX_PROV;
    for(int p=0;p<n;p++) if(e->prov[p].owner==c) g+=e->prov[p].treasury;
    return g;
}
void credit_spend(WorldEconomy *e, const World *w, int c, float cost){
    int hr=home_reg(w,c); if(hr<0||hr>=e->n_regions) return;
    /* RE-KEY PROVINCE : treasury est Σ-agrégé depuis prov[] à chaque econ_tick — écrire
     * region[hr] directement serait effacé au tick suivant. Route sur la province
     * représentative (capitale, charte). */
    int pid=econ_region_rep_province(e, hr); if(pid<0||pid>=e->n_prov) return;
    e->prov[pid].treasury-=cost;                                   /* peut passer NET négatif = dette */
    /* country_gold_prov (PAS econ_country_gold) : la dépense qu'on vient de faire n'a
     * pas encore traversé econ_aggregate_regions — lire region[] ici verrait l'ANCIEN
     * trésor (stale) et manquerait systématiquement le seuil de dette. */
    if(country_gold_prov(e,c)<0.0 && g_creditor[c]<0){
        int L=pick_lender(e,w,c); if(L>=0) g_creditor[c]=(int16_t)L;
    }
}

/* INTÉRÊT ANNUEL = la rétroaction. Taux ↑ avec le ratio de dette ET la chute de légitimité
 * (le créancier price le risque). L'intérêt CREUSE le débiteur et CRÉDITE le créancier.
 * Soldé (or ≥ 0) → le lien se dénoue. */
void credit_year_tick(WorldEconomy *e, const WorldLegitimacy *wl, const World *w){
    for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        double g=econ_country_gold(e,c);
        if(g>=0.0){ g_creditor[c]=-1; continue; }
        double debt=-g;
        float legit=legitimacy_country(wl,w,e,c);                   /* 0..10 */
        float line=credit_line(w,e,c); if(line<1.f) line=1.f;
        /* ANTI-EMBALLEMENT (bug pré-capstone) : sans plafond, rate∝ratio ET assiette=debt
         * → intérêt ∝ debt² → la dette spirale en GÉOMÉTRIQUE (treasury → -1e31 en ~105 ans,
         * NaN plus loin). Le prêteur n'étend pas le crédit À L'INFINI : au-delà de
         * CREDIT_RATIO_CAP × ligne, le taux PLAFONNE et l'assiette d'intérêt aussi → l'intérêt
         * devient CONSTANT (≈ cap·ligne·taux_max), la dette croît LINÉAIREMENT (bornée, finie).
         * 12 ans : dettes ≪ ligne ⇒ jamais plafonné ⇒ déterminisme INCHANGÉ. */
        float rcap=tune_f("CREDIT_RATIO_CAP",8.f);
        float ratio=(float)(debt/(double)line); if(ratio>rcap) ratio=rcap;
        float rate=tune_f("CREDIT_RATE_BASE",0.05f)*(1.f+ratio+(10.f-legit)/10.f);
        double idebt=debt; if(idebt>(double)rcap*(double)line) idebt=(double)rcap*(double)line;
        double interest=idebt*(double)rate;
        int hr=home_reg(w,c);
        if(hr>=0&&hr<e->n_regions){ int hp=econ_region_rep_province(e,hr); if(hp>=0&&hp<e->n_prov) e->prov[hp].treasury-=interest; }
        econ_flux_add(c, FX_CREDIT, -(float)interest);   /* I0 : la ligne intérêts */
        int Cr=g_creditor[c];
        if(Cr>=0&&Cr<w->n_countries){ int hc=home_reg(w,Cr);
            if(hc>=0&&hc<e->n_regions){ int cp=econ_region_rep_province(e,hc); if(cp>=0&&cp<e->n_prov) e->prov[cp].treasury+=interest; } }
    }
}
bool credit_save(FILE *f){ return fwrite(g_creditor,sizeof g_creditor,1,f)==1; }
bool credit_load(FILE *f){ return fread (g_creditor,sizeof g_creditor,1,f)==1; }
