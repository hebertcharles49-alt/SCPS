/*
 * scps_demography.c — la clé de voûte : la province contient des groupes
 *
 * D interne PAR province (entre groupes), H jouable (suppression réversible),
 * assimilation incarnée (dérive durable via scps_modifier), L/fracture vécues.
 * On RÉUTILISE la formule de légitimité existante (mêmes constantes) → une
 * province mono-groupe reproduit les nombres d'aujourd'hui (non-régression).
 */
#include "scps_demography.h"
#include "scps_culture.h"   /* ethos_name */
#include "scps_labor.h"     /* capitale_max_tier : les emplois nobles dont la classe émerge (§pop précise) */
#include "scps_routes.h"    /* S2 : le contact COMMERCIAL porte la cristallisation culturelle */
#include "scps_diplo.h"     /* S2 : la guerre coupe le contact */
#include "scps_tune.h"      /* S2 : le rythme de fusion calibrable */
#include <string.h>
#include <math.h>
#include <stdio.h>

/* drift_id dynamiques (migration · conquête) : compteur unique, rebasé après
 * chargement (cf. demography_dyn_id_rebase). Le socle laisse la place aux ids
 * d'attache (1..n_groupes) loin en dessous. */
#define DYN_DRIFT_BASE 1000000
static int g_dyn_drift_id = DYN_DRIFT_BASE;

/* ---- Constantes de légitimité (miroir de scps_legitimacy : non-régression) */
#define K_ALIGN          1.0f
#define W_ALIGN          0.55f
#define W_AISANCE        0.45f
#define K_COERC          3.0f
#define K_H              0.4f
#define K_BUILD_H        0.6f
#define RELAX_RATE       0.04f
/* ---- Coercition (§3) — surface d'équilibrage -------------------------- */
#define COERCE_L_THRESH  4.0f   /* on ne réprime que les groupes restifs (L basse) */
#define SUPPRESS_PER_H   8.0f   /* baisse d'agitation par point de H (réversible) */
#define L_DROP_PER_H     0.06f  /* régner par la force ronge la L du groupe */
#define FRAG_PER_H       0.5f
/* ---- Assimilation (§5) ------------------------------------------------ */
#define YEARS_PER_DINF   10.0f  /* timer ∝ D∞ (gouffre) */
#define ASSIM_MIN_YEARS  12.0f
#define ASSIM_MAX_YEARS  200.0f
#define FUSE_EPS         0.30f   /* distance de contenu sous laquelle on fusionne */
/* ---- Conversion religieuse (§2) : la FOI converge vers le TRÔNE ------- *
 * L'assimilation tire la culture vers la dominante LOCALE ; la CONVERSION
 * tire la foi — l'axe doctrinal puis la BRANCHE sacrée — vers la COURONNE,
 * mais seulement sous un trône PROSÉLYTE. La bascule de branche exige que la
 * foi ait pris RACINE (années de règne) ET que l'axe ait convergé. */
#define FAITH_CONV_SOFT    0.015f /* évangélisme : fraction/an du gap doctrinal comblée */
#define FAITH_CONV_HARD    0.045f /* purification : conversion forcée, plus vive */
#define CONVERT_YEARS      60.0f  /* évangéliste : la branche bascule après deux générations */
#define CONVERT_YEARS_HARD 20.0f  /* purificateur : une génération suffit */
#define CONVERT_AXIS       1.5f   /* évangéliste : bascule quand l'axe a quasi convergé */
#define CONVERT_AXIS_HARD  4.0f   /* purificateur : bascule de force, axe encore distant */

static inline float clampf(float v,float lo,float hi){ return v!=v?lo:(v<lo?lo:(v>hi?hi:v)); }
static inline float absf(float v){ return v<0?-v:v; }

/* Distance de CONTENU (L∞, langue exclue) — la friction. */
/* La FOI est un axe ACTIF (§1/§3) : une autre BRANCHE de foi est une vraie
 * fracture, au-delà de l'axe religion (même foi → cohésion & assimilation vite ;
 * autre foi → résistance, fracture). */
#define FAITH_BRANCH_PEN 3.5f
static float content_dist(const PopCulture *a, const PopCulture *b){
    float dv=absf(a->valeurs-b->valeurs), ds=absf(a->subsistance-b->subsistance);
    float dp=absf(a->parente-b->parente), dr=absf(a->religion-b->religion);
    if (a->rel_branch!=b->rel_branch && dr<FAITH_BRANCH_PEN) dr=FAITH_BRANCH_PEN;
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}
static float agit_from_L(float L){ return clampf((6.f - L)*15.f, 0.f, 100.f); }

/* ===================================================================== */
/* FICHE EFFECTIVE = origine + dérive (recalcul, jamais mutation)         */
/* ===================================================================== */
static const PopCulture *dom_ruling_culture(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0||cid>=w->n_countries) return NULL;
    int cp=w->country[cid].capital_prov; if(cp<0||cp>=w->n_provinces) return NULL;
    int cr=w->province[cp].region; if(cr<0||cr>=econ->n_regions) return NULL;
    return &econ->region[cr].culture;
}

PopCulture group_culture_effective(const PopGroup *g, const ModifierStack *drift){
    if (!g){ PopCulture z; memset(&z,0,sizeof z); return z; }  /* NULL-safe : cohérent avec le module défensif */
    PopCulture c = g->origin;
    if (drift){
        GroupDrift d = modstack_group_drift(drift, g->drift_id);
        c.valeurs     = clampf(c.valeurs     + d.dCv, 0.f, 10.f);
        c.subsistance = clampf(c.subsistance + d.dCs, 0.f, 10.f);
        c.parente     = clampf(c.parente     + d.dCp, 0.f, 10.f);
        c.religion    = clampf(c.religion    + d.dCr, 0.f, 10.f);
    }
    return c;
}
float group_agitation_effective(const PopGroup *g, const ModifierStack *drift){
    float suppress = drift ? modstack_group_drift(drift, g->drift_id).dAgit : 0.f;
    return clampf(g->agit_base - suppress, 0.f, 100.f);   /* la suppression MASQUE l'agitation vraie */
}

/* ===================================================================== */
/* LECTURES DE PROVINCE (§2)                                              */
/* ===================================================================== */
const PopGroup *province_dominant(const ProvincePop *pp){
    if (pp->n_groups<=0) return NULL;
    int best=0; for (int i=1;i<pp->n_groups;i++) if (pp->groups[i].count>pp->groups[best].count) best=i;
    return &pp->groups[best];
}
long province_total_pop(const ProvincePop *pp){
    long t=0; for (int i=0;i<pp->n_groups;i++) t+=pp->groups[i].count; return t;
}
/* D̄ interne = moyenne PONDÉRÉE des distances inter-groupes. */
float province_Dbar(const ProvincePop *pp, const ModifierStack *drift){
    double sw=0, sd=0;
    for (int i=0;i<pp->n_groups;i++){
        PopCulture ci=group_culture_effective(&pp->groups[i],drift);
        for (int j=i+1;j<pp->n_groups;j++){
            PopCulture cj=group_culture_effective(&pp->groups[j],drift);
            double w=(double)pp->groups[i].count*pp->groups[j].count;
            sd += w*content_dist(&ci,&cj); sw += w;
        }
    }
    return sw>0 ? (float)(sd/sw) : 0.f;
}
/* D∞ interne = MAILLON FAIBLE (max distance inter-groupes). */
float province_Dinf(const ProvincePop *pp, const ModifierStack *drift){
    float m=0;
    for (int i=0;i<pp->n_groups;i++){
        PopCulture ci=group_culture_effective(&pp->groups[i],drift);
        for (int j=i+1;j<pp->n_groups;j++){
            PopCulture cj=group_culture_effective(&pp->groups[j],drift);
            float d=content_dist(&ci,&cj); if(d>m)m=d;
        }
    }
    return m;
}
float province_L(const ProvincePop *pp){
    double num=0, den=0;
    for (int i=0;i<pp->n_groups;i++){ num+=(double)pp->groups[i].count*pp->groups[i].L; den+=pp->groups[i].count; }
    return den>0 ? (float)(num/den) : 5.f;
}
float province_agitation(const ProvincePop *pp, const ModifierStack *drift){
    double num=0, den=0;
    for (int i=0;i<pp->n_groups;i++){
        num+=(double)pp->groups[i].count*group_agitation_effective(&pp->groups[i],drift);
        den+=pp->groups[i].count;
    }
    return den>0 ? (float)(num/den) : 0.f;
}

/* ===================================================================== */
/* LÉGITIMITÉ PAR GROUPE (formule existante, clé sur culture vs couronne) */
/* ===================================================================== */
float group_L_target(const PopGroup *g, const ModifierStack *drift, const PopCulture *crown,
                     float satisfaction, float integ, float country_H, float coercion, float build_H){
    PopCulture eff = group_culture_effective(g, drift);
    float align   = crown ? (10.f - K_ALIGN*content_dist(&eff, crown)) : 5.f;
    float aisance = clampf(satisfaction*10.f, 0.f, 10.f);
    float ombre   = K_COERC*coercion + K_H*country_H + K_BUILD_H*build_H;
    float Lstar   = (W_ALIGN*align + W_AISANCE*aisance)*integ - ombre;
    return clampf(Lstar, 0.f, 10.f);
}
void group_L_tick(PopGroup *g, const ModifierStack *drift, const PopCulture *crown,
                  float satisfaction, float country_H, float coercion, float build_H){
    float Lstar = group_L_target(g, drift, crown, satisfaction, g->integration, country_H, coercion, build_H);
    g->L += (Lstar - g->L)*RELAX_RATE;
    g->L = clampf(g->L, 0.f, 10.f);
    g->agit_base = agit_from_L(g->L);
}

/* ===================================================================== */
/* H JOUABLE — SUPPRIME (réversible), n'assimile pas (§3)                  */
/* ===================================================================== */
CoercionEffect province_apply_coercion(ProvincePop *pp, ModifierStack *drift, float H){
    CoercionEffect e={0,0,0};
    const PopGroup *dom=province_dominant(pp);
    for (int i=0;i<pp->n_groups;i++){
        PopGroup *g=&pp->groups[i];
        if (g==dom || g->L>=COERCE_L_THRESH) continue;     /* on ne réprime que le restif */
        GroupDrift s={0,0,0,0, H*SUPPRESS_PER_H};
        modstack_accumulate_drift(drift, g->drift_id, s, true);   /* RÉVERSIBLE (saute si H tombe) */
        g->L = clampf(g->L - H*L_DROP_PER_H, 0.f, 10.f);          /* régner par la force ronge L */
        g->agit_base = agit_from_L(g->L);                         /* l'agitation VRAIE monte (masquée) */
        e.agitation_drop += H*SUPPRESS_PER_H; e.L_drop += H*L_DROP_PER_H; e.fragility_rise += H*FRAG_PER_H;
    }
    return e;
}
void province_lift_coercion(ProvincePop *pp, ModifierStack *drift){
    for (int i=0;i<pp->n_groups;i++) modstack_drop_reversible(drift, pp->groups[i].drift_id);
}

/* ===================================================================== */
/* ASSIMILATION — dérive DURABLE, timer ∝ D∞ (gouffre §5)                 */
/* ===================================================================== */
float assimilation_years(float Dinf, float P, float K){
    float metab = 0.5f + (P+K)/20.f;          /* P+K accélèrent (la capacité métabolise) */
    return clampf(Dinf*YEARS_PER_DINF/metab, ASSIM_MIN_YEARS, ASSIM_MAX_YEARS);
}
int assimilation_tick(ProvincePop *pp, ModifierStack *drift, float P, float K, float ypt){
    if (pp->n_groups<2) return 0;
    const PopGroup *dom=province_dominant(pp);
    int dom_idx=(int)(dom-pp->groups);
    PopCulture target=group_culture_effective(dom, drift);
    int fused=0;
    for (int i=pp->n_groups-1;i>=0;i--){
        if (i==dom_idx) continue;
        PopGroup *g=&pp->groups[i];
        PopCulture eff=group_culture_effective(g, drift);
        float d=content_dist(&eff,&target);
        if (d<FUSE_EPS){                                   /* fusion dans le dominant */
            pp->groups[dom_idx].count += g->count;
            int last=pp->n_groups-1;
            pp->groups[i]=pp->groups[last]; pp->n_groups--;
            if (dom_idx==last) dom_idx=i;
            fused++; continue;
        }
        float years=assimilation_years(d,P,K);
        float rate=ypt/fmaxf(years,1.f);
        GroupDrift step={ (target.valeurs    -eff.valeurs)    *rate,
                          (target.subsistance-eff.subsistance)*rate,
                          (target.parente    -eff.parente)    *rate,
                          (target.religion   -eff.religion)   *rate, 0.f };
        modstack_accumulate_drift(drift, g->drift_id, step, false);   /* DURABLE (métabolisé) */
        g->integration = clampf(g->integration + ypt/years, 0.f, 1.f);
    }
    return fused;
}

/* CONVERSION RELIGIEUSE (§2) — la foi des provinces converge vers le TRÔNE.
 * Distincte de l'assimilation (qui tire vers la dominante LOCALE) : ici l'axe
 * doctrinal de CHAQUE groupe dérive vers la couronne, et la branche sacrée
 * BASCULE une fois la foi enracinée (années de règne) et l'axe convergé. Un
 * trône pluraliste ne convertit personne — l'empire reste multi-confessionnel
 * (la tolérance est la non-conversion). Évangélisme : lent, n'achève que les
 * proches ; Purification : vif, bascule de force les confessions distantes. */
void faith_convert_tick(ProvincePop *pp, const PopCulture *crown,
                        float years_held, float ypt){
    if (!crown || crown->credo==CREDO_PLURALISTE) return;   /* tolérance : nulle conversion */
    bool  hard       = (crown->credo==CREDO_PURIFICATEUR);
    float rate       = (hard?FAITH_CONV_HARD   :FAITH_CONV_SOFT )*ypt;
    float need_years = (hard?CONVERT_YEARS_HARD :CONVERT_YEARS  );
    float need_axis  = (hard?CONVERT_AXIS_HARD  :CONVERT_AXIS   );
    for (int i=0;i<pp->n_groups;i++){
        PopGroup *g=&pp->groups[i];
        /* L'axe doctrinal de l'ORIGINE dérive vers le trône — mutation DIRECTE
         * (la foi convertie EST qui ils sont), pas la pile réversible : O(1) par
         * groupe, et cohérent avec la bascule de branche ci-dessous. */
        float gap = crown->religion - g->origin.religion;
        g->origin.religion = clampf(g->origin.religion + gap*rate, 0.f, 10.f);
        if (g->origin.rel_branch != crown->rel_branch
            && years_held >= need_years && absf(gap) <= need_axis){
            g->origin.rel_branch = crown->rel_branch;       /* la BRANCHE bascule (durable) */
            g->origin.credo      = crown->credo;            /* et embrasse le credo du trône */
        }
    }
}

/* ===================================================================== */
/* MIGRATION PASSIVE — emporte heritage + culture (§4)                        */
/* ===================================================================== */
bool migration_move(ProvincePop *from, ProvincePop *to, int gi, long amount, int new_drift_id, int mode){
    if (gi<0||gi>=from->n_groups) return false;
    PopGroup *src=&from->groups[gi];
    if (amount>src->count) amount=src->count;
    if (amount<=0) return false;
    /* groupe d'accueil = même heritage ET même culture d'origine ? sinon DIASPORA. */
    int dst=-1;
    for (int i=0;i<to->n_groups;i++)
        if (to->groups[i].heritage==src->heritage && content_dist(&to->groups[i].origin,&src->origin)<FUSE_EPS){ dst=i; break; }
    if (dst<0){
        if (to->n_groups>=DEMO_MAX_GROUPS) return false;
        PopGroup ng=*src;                    /* garde species/culture → minorité à l'arrivée */
        ng.count=amount; ng.diaspora=true; ng.integration=0.f; ng.drift_id=new_drift_id;
        ng.arrival=(uint8_t)mode;            /* le MODE d'arrivée (migrant/déporté) → coeff de diffusion */
        to->groups[to->n_groups++]=ng;       /* crée du D interne dans la cible */
    } else {
        to->groups[dst].count += amount;
    }
    src->count -= amount;
    if (src->count<=0){ from->groups[gi]=from->groups[from->n_groups-1]; from->n_groups--; }
    return true;
}

/* ===================================================================== */
/* AGRÉGATION PAYS (§2, §6) — alimente scps_order (inchangé)              */
/* ===================================================================== */
/* On rassemble toutes les fiches effectives du pays, puis maillon faible. */
static int collect(const ProvincePop *provs, int n, const ModifierStack *drift,
                   PopCulture out[], long w[], int max){
    int k=0;
    for (int p=0;p<n && k<max;p++) for (int i=0;i<provs[p].n_groups && k<max;i++){
        out[k]=group_culture_effective(&provs[p].groups[i],drift); w[k]=provs[p].groups[i].count; k++;
    }
    return k;
}
float country_Dbar(const ProvincePop *provs, int n, const ModifierStack *drift){
    PopCulture c[DEMO_MAX_GROUPS*16]; long w[DEMO_MAX_GROUPS*16];
    int k=collect(provs,n,drift,c,w,DEMO_MAX_GROUPS*16);
    double sw=0,sd=0;
    for (int i=0;i<k;i++) for (int j=i+1;j<k;j++){ double ww=(double)w[i]*w[j]; sd+=ww*content_dist(&c[i],&c[j]); sw+=ww; }
    return sw>0?(float)(sd/sw):0.f;
}
float country_Dinf(const ProvincePop *provs, int n, const ModifierStack *drift){
    PopCulture c[DEMO_MAX_GROUPS*16]; long w[DEMO_MAX_GROUPS*16];
    int k=collect(provs,n,drift,c,w,DEMO_MAX_GROUPS*16);
    float m=0;
    for (int i=0;i<k;i++) for (int j=i+1;j<k;j++){ float d=content_dist(&c[i],&c[j]); if(d>m)m=d; }
    return m;
}
float country_L(const ProvincePop *provs, int n){
    double num=0,den=0;
    for (int p=0;p<n;p++) for (int i=0;i<provs[p].n_groups;i++){
        num+=(double)provs[p].groups[i].count*provs[p].groups[i].L; den+=provs[p].groups[i].count;
    }
    return den>0?(float)(num/den):5.f;
}

/* ===================================================================== */
/* COMPOSITION (§6) — la membrane : des mots, jamais de SCPS brut         */
/* ===================================================================== */
const char *labor_class_word(SocialClass k){
    switch(k){ case CLASS_ELITE: return "Noblesse"; case CLASS_BOURGEOIS: return "Artisans";
               default: return "Laboureurs"; }
}
int province_composition(const ProvincePop *pp, const ModifierStack *drift,
                         const PopCulture *crown, float P, float K,
                         GroupReadout out[], int max){
    static char etat_buf[DEMO_MAX_GROUPS][48];
    if (pp->n_groups<=0) return 0;                  /* province vide : nulle composition (province_dominant→NULL) */
    long total=province_total_pop(pp); if(total<1)total=1;
    const PopGroup *dom=province_dominant(pp);
    PopCulture domc=group_culture_effective(dom,drift);
    int n=0;
    for (int i=0;i<pp->n_groups && n<max;i++){
        const PopGroup *g=&pp->groups[i];
        PopCulture eff=group_culture_effective(g,drift);
        GroupReadout *r=&out[n];
        r->heritage    = heritage_name(g->heritage);
        r->culture = ethos_name(eff.ethos);
        r->religion= religion_branch_name(eff.rel_branch);
        r->klass   = labor_class_word(g->klass);
        r->percent = (int)(100*g->count/total);
        r->loyaute = band_humeur(g->L);
        /* BRASSAGE — le MODE D'ARRIVÉE nomme l'état : la diaspora libre reste « diaspora »,
         * mais le conquis allophone est « soumis » et le captif « déporté ». Pour ces deux
         * voies coercitives, on SURFACE l'intégration (0-100 %) : c'est la métabolisation en
         * cours — le savoir qu'on digère, le nombre tangible qui compte pour le joueur. */
        if (g->arrival==ARR_SOUMIS){
            snprintf(etat_buf[n],sizeof etat_buf[n], "soumis · %d%% intégré", (int)(100.f*g->integration+0.5f));
            r->etat=etat_buf[n];
        } else if (g->arrival==ARR_DEPORTE){
            snprintf(etat_buf[n],sizeof etat_buf[n], "déporté · %d%% intégré", (int)(100.f*g->integration+0.5f));
            r->etat=etat_buf[n];
        } else if (g->diaspora){ r->etat="diaspora"; }
        else if (g==dom){ r->etat="natif"; }
        else {
            float d=content_dist(&eff, crown?crown:&domc);
            if (d<0.6f){ r->etat="natif"; }
            else {
                int yrs=(int)(assimilation_years(d,P,K)+0.5f);
                snprintf(etat_buf[n],sizeof etat_buf[n], "en assimilation (%d ans)", yrs);
                r->etat=etat_buf[n];
            }
        }
        n++;
    }
    return n;
}

/* ===================================================================== */
/* INTÉGRATION AU MOTEUR VIVANT (§7)                                      */
/* ===================================================================== */
/* ---- Migration vivante (surface d'équilibrage) ------------------------ */
#define MIG_GRADIENT   1.5f    /* il faut une cible NOTABLEMENT plus prospère */
#define MIG_FRACTION   200     /* 1/200 du groupe dominant par an (0.5 %) */
#define MIG_MIN        20

void demography_attach(World *w, WorldEconomy *econ, ModifierStack *drift){
    (void)w;   /* signature publique conservée (appelants historiques) ; RE-KEY : plus besoin du World* */
    if (drift) memset(drift, 0, sizeof(*drift));     /* la pile de dérive du monde, à neuf */
    int id=1;
    /* RE-KEY PROVINCE (T1) : le groupe-substrat VIT sur la province représentative
     * (econ_region_rep_province, ancrage figé) — écrire econ->region[r].pop serait
     * effacé au 1er econ_tick (miroir). À la genèse, region[r].strata/culture sont
     * encore le miroir tout frais d'econ_init (aucun econ_tick entre les deux) : lire
     * la province directement est équivalent et plus robuste. */
    for (int r=0; r<econ->n_regions; r++){
        int rp=econ_region_rep_province(econ, r);
        if (rp<0 || rp>=econ->n_prov) continue;
        ProvinceEconomy *re=&econ->prov[rp];
        ProvincePop *pp=&re->pop;
        memset(pp, 0, sizeof(*pp));
        pp->prov = rp;
        pp->prosperity = re->prosperity;
        if (!re->culture.settled) continue;
        long total = (long)(re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop
                          + re->strata[CLASS_ELITE].pop);
        if (total<1) total=1;
        PopGroup *g=&pp->groups[0]; memset(g,0,sizeof(*g));
        g->heritage=re->culture.heritage; g->origin_sphere=heritage_sphere(re->culture.heritage);
        g->origin=re->culture; g->culture=re->culture;     /* substrat = effective au départ */
        g->klass=CLASS_LABORER; g->count=total;
        g->pop_by_class[CLASS_LABORER]=total;        /* repli : tout Journalier avant la 1re émergence */
        g->pop_by_class[CLASS_BOURGEOIS]=0; g->pop_by_class[CLASS_ELITE]=0;
        g->L=7.f; g->agit_base=agit_from_L(7.f); g->integration=1.f;   /* natifs intégrés */
        g->diaspora=false; g->drift_id=id++;
        pp->n_groups=1;     /* MONO-GROUPE → non-régression (les nombres d'hier) */
    }
    g_dyn_drift_id=DYN_DRIFT_BASE;   /* nouvelle partie : le compteur dynamique repart au socle */
}

/* ---- drift_id DYNAMIQUES (migration · conquête) ------------------------- *
 * Un compteur UNIQUE et monotone : l'ancien schéma donnait 900000+région aux
 * colons de conquête → deux vagues sur la MÊME région partageaient un drift_id
 * (la dérive de l'une lue par l'autre). Le compteur n'étant pas sérialisé, on
 * le REBASE au-dessus du maximum chargé après game_load (sinon collision avec
 * les groupes d'une partie sauvegardée). */
int demography_dyn_id_next(void){ return g_dyn_drift_id++; }
void demography_dyn_id_rebase(const WorldEconomy *econ){
    int hi=DYN_DRIFT_BASE-1;
    /* RE-KEY PROVINCE (T1) : les groupes vivent sur prov[] — scanner region[].pop (miroir,
     * potentiellement vide) sous-estimerait hi et rouvrirait des collisions de drift_id. */
    int nprov=econ->n_prov; if (nprov>SCPS_MAX_PROV) nprov=SCPS_MAX_PROV;
    for (int p=0;p<nprov;p++){
        const ProvincePop *pp=&econ->prov[p].pop;
        for (int i=0;i<pp->n_groups;i++)
            if (pp->groups[i].drift_id>hi) hi=pp->groups[i].drift_id;
    }
    g_dyn_drift_id=hi+1;
}

/* ÉMERGENCE DE CLASSE (§pop précise) : la classe de CHAQUE groupe (heritage×culture×foi)
 * sort des EMPLOIS de la province (RE-KEY T1 : re->bld[]/re->n_bld sont désormais ceux de
 * la province-hôte, plus précis que l'ex-miroir région) — la capitale (tier·100 emplois
 * NOBLES, le tier que la pop débloque) + les ateliers (emploi BOURGEOIS ≈ ouvriers des
 * manufactures), répartis sur les groupes AU PRORATA, par paquets de 100. Σ pop_by_class
 * = count. Rien n'est posé : la structure d'emplois sculpte le tissu social, groupe par
 * groupe. */
static void demography_emerge_classes(ProvinceEconomy *re){
    ProvincePop *pp=&re->pop;
    long total=0; for (int i=0;i<pp->n_groups;i++) total+=pp->groups[i].count;
    if (total<1){
        for (int i=0;i<pp->n_groups;i++){
            pp->groups[i].pop_by_class[CLASS_LABORER]=pp->groups[i].count;
            pp->groups[i].pop_by_class[CLASS_BOURGEOIS]=0;
            pp->groups[i].pop_by_class[CLASS_ELITE]=0;
        }
        return;
    }
    long elite_jobs   = (long)capitale_max_tier(total)*100;                  /* capitale : tier·100 */
    long artisan_jobs = 0;
    for (int b=0;b<re->n_bld;b++) artisan_jobs += (long)re->bld[b].workers;  /* ateliers : ouvriers */
    artisan_jobs = (artisan_jobs/100)*100;
    if (elite_jobs > total) elite_jobs=(total/100)*100;
    if (elite_jobs+artisan_jobs > total) artisan_jobs=((total-elite_jobs)/100)*100;
    for (int i=0;i<pp->n_groups;i++){
        PopGroup *g=&pp->groups[i];
        long e=((g->count*elite_jobs/total)/100)*100;       /* part noble de la bande */
        long a=((g->count*artisan_jobs/total)/100)*100;     /* part bourgeoise */
        if (e>g->count) e=(g->count/100)*100;
        if (e+a>g->count) a=((g->count-e)/100)*100;
        g->pop_by_class[CLASS_ELITE]=e;
        g->pop_by_class[CLASS_BOURGEOIS]=a;
        g->pop_by_class[CLASS_LABORER]=g->count-e-a;
    }
}

/* S2 — LA CRISTALLISATION CULTURELLE SUIT LE CONTACT (pas que la cohabitation). Le moteur
 * `culture_syncretize` (le mutant hybride : substrat A sous élite B) était DORMANT — seul le
 * banc l'appelait. On le RÉVEILLE : une région en CONTACT COMMERCIAL soutenu (route OUVERTE,
 * à la paix) avec un partenaire d'un AUTRE pays voit sa culture dominante dériver vers la
 * sienne — la MER porte plus loin (poids ×2) —, jugée par la MÊME porte métabolique que la
 * prospérité de contact (`culture_can_syncretize` : σ(0.8(P−D∞)+0.35(K−5)), INCHANGÉE). Quand
 * la distance franchit le seuil de fusion, l'hybride CRISTALLISE (le banc de la chronique le
 * compte). Pas de mur de distance — il faut juste plus de P+K et du TEMPS (générations). On
 * NE crée PAS d'état sérialisé : la dérive s'écrit dans les axes de culture existants. */
static void pc_to_culture(const PopCulture *p, Culture *c){
    memset(c,0,sizeof *c);
    c->langue=p->langue; c->valeurs=p->valeurs; c->subsistance=p->subsistance;
    c->parente=p->parente; c->religion=p->religion; c->ethos=p->ethos; c->lifeway=p->lifeway;
    c->structure=p->structure; c->credo=p->credo; c->rel_branch=p->rel_branch;
    c->martial=p->martial; c->econ=p->econ; c->age=p->age; c->is_hybrid=false;
}
static void culture_to_pc(const Culture *c, PopCulture *p){
    p->langue=c->langue; p->valeurs=c->valeurs; p->subsistance=c->subsistance;
    p->parente=c->parente; p->religion=c->religion; p->ethos=c->ethos; p->lifeway=c->lifeway;
    p->structure=c->structure; p->credo=c->credo; p->rel_branch=c->rel_branch;
    p->martial=c->martial; p->econ=c->econ; p->age=c->age;   /* settled/heritage PRÉSERVÉS */
}
static long g_contact_cryst = 0;   /* cristallisations par contact, cumul de la sim (télémétrie) */
void demography_contact_reset(void){ g_contact_cryst = 0; }
long demography_contact_count(void){ return g_contact_cryst; }
static long g_migration_pact_flows = 0;   /* flux de pacte migratoire, cumul de la sim (télémétrie) */
void demography_migration_pact_reset(void){ g_migration_pact_flows = 0; }
long demography_migration_pact_count(void){ return g_migration_pact_flows; }
int demography_contact_tick(WorldEconomy *e, ModifierStack *drift,
                            const RouteNetwork *rn, const DiploState *dp,
                            float P, float K, float ypt){
    if (!e || !rn) return 0;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    float fuse_rate=tune_f("SYNC_FUSE_RATE",0.10f);            /* fraction du fossé comblée/an (mer soutenue) */
    float inner=FUSE_EPS*0.5f;                                  /* la bande de FRANCHISSEMENT (compté une fois) */
    int cryst=0;
    /* RE-KEY PROVINCE (T1) : e->region[r].pop est un miroir — les groupes vivent sur la
     * province représentative (econ_region_rep_province). L'adjacence commerciale (routes)
     * reste au grain RÉGION (charte règle 4) : seul l'accès aux groupes est redirigé. */
    for (int r=0;r<n;r++){
        RegionEconomy *re=&e->region[r];
        int rp=econ_region_rep_province(e, r);
        if (rp<0 || rp>=e->n_prov) continue;
        ProvinceEconomy *pe=&e->prov[rp];
        if (pe->pop.n_groups<=0 || !pe->culture.settled || re->owner<0) continue;
        int partner=-1; bool sea=false;                        /* le MEILLEUR partenaire-route étranger */
        for (int i=0;i<rn->n;i++){
            const TradeRoute *t=&rn->route[i];
            if (!t->open || t->ra<0||t->rb<0||t->ra>=n||t->rb>=n) continue;
            int far;
            if      (t->ra==r) far=t->rb;
            else if (t->rb==r) far=t->ra;
            else continue;
            int fo=e->region[far].owner;
            if (fo<0 || fo==re->owner) continue;               /* un AUTRE pays */
            if (dp && diplo_status(dp,re->owner,fo)==DIPLO_WAR) continue;   /* la guerre coupe le contact */
            int rpf=econ_region_rep_province(e, far);
            if (rpf<0 || rpf>=e->n_prov) continue;
            if (e->prov[rpf].pop.n_groups<=0 || !e->prov[rpf].culture.settled) continue;
            if (t->maritime){ partner=far; sea=true; break; }  /* la MER d'abord (porte plus loin) */
            if (partner<0) partner=far;
        }
        if (partner<0) continue;
        int rpp=econ_region_rep_province(e, partner);
        if (rpp<0 || rpp>=e->n_prov) continue;
        PopGroup *dom=(PopGroup*)province_dominant(&pe->pop); if(!dom) continue;
        const PopGroup *pd=province_dominant(&e->prov[rpp].pop); if(!pd) continue;
        PopCulture a=group_culture_effective(dom, drift);      /* EFFECTIVE = origine + dérive DURABLE */
        PopCulture b=group_culture_effective(pd, drift);
        Culture ca,cb; pc_to_culture(&a,&ca); pc_to_culture(&b,&cb);
        SyncFeasibility f=culture_can_syncretize(&ca,&cb,P,K);
        if (!f.feasible) continue;                             /* porte fermée : plus de P+K requis */
        float d=content_dist(&a,&b);
        if (d>=inner && d<FUSE_EPS){                           /* FRANCHISSEMENT → l'hybride cristallise */
            Culture h;
            if (culture_syncretize(&ca,&cb,&h)){               /* l'ORIGINE porte la fusion (durable) */
                PopCulture hpc=dom->origin; culture_to_pc(&h,&hpc); dom->origin=hpc;
                if (drift){ GroupDrift cur=modstack_group_drift(drift,dom->drift_id);
                    GroupDrift neg={-cur.dCv,-cur.dCs,-cur.dCp,-cur.dCr,0.f};   /* la dérive culture revient à plat (l'origine EST l'hybride) */
                    modstack_accumulate_drift(drift, dom->drift_id, neg, false); }
                dom->culture=group_culture_effective(dom, drift);   /* rafraîchit le cache (= l'hybride) */
                cryst++;
            }
        } else if (d>=FUSE_EPS && drift){                      /* encore loin : la dérive DURABLE vers le partenaire (générations) */
            float rate=fuse_rate*f.openness*(sea?1.f:0.5f)*ypt; if(rate>0.5f)rate=0.5f;
            GroupDrift step={ (b.valeurs-a.valeurs)*rate, (b.subsistance-a.subsistance)*rate,
                              (b.parente-a.parente)*rate, (b.religion-a.religion)*rate, 0.f };
            modstack_accumulate_drift(drift, dom->drift_id, step, false);   /* DURABLE (métabolisé), comme l'assimilation */
        }
    }
    g_contact_cryst += cryst;
    return cryst;
}

/* ===================================================================== */
/* PACTE MIGRATOIRE (BRASSAGE) — l'échange passif de population entre alliés */
/* ===================================================================== */
/* Entre deux pays liés par un pacte migratoire et à la PAIX, chacun envoie un
 * petit flux depuis sa province la plus peuplée vers celle de l'autre : le plus
 * ATTRACTIF (prospère) reçoit NET (« à l'avantage de celui qui a le plus à
 * offrir »). Les migrants portent leur héritage → diaspora ARR_MIGRANT chez
 * l'hôte → métabolisation (le savoir DIFFUSE — l'inverse de l'isolement de Song). */
int demography_migration_pact_tick(WorldEconomy *e, const DiploState *dp){
    if (!e || !dp) return 0;
    float frac = tune_f("MIG_PACT_FRAC", 0.006f);       /* ~0.6 %/an du dominant, modulé par l'attractivité */
    long  fmin = (long)tune_f("MIG_PACT_MIN", 30.f);
    int np = e->n_prov; if (np>SCPS_MAX_PROV) np=SCPS_MAX_PROV;
    /* province la plus peuplée + sa prospérité (proxy d'attractivité), par pays. */
    static int   top_prov[SCPS_MAX_COUNTRY];
    static long  top_pop [SCPS_MAX_COUNTRY];
    static float attract [SCPS_MAX_COUNTRY];
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ top_prov[c]=-1; top_pop[c]=0; attract[c]=0.f; }
    for (int p=0;p<np;p++){
        ProvinceEconomy *pe=&e->prov[p];
        int o=pe->owner; if (o<0||o>=SCPS_MAX_COUNTRY) continue;
        if (!pe->culture.settled || pe->pop.n_groups<=0) continue;
        long tot=province_total_pop(&pe->pop);
        if (tot>top_pop[o]){ top_pop[o]=tot; top_prov[o]=p; attract[o]=pe->pop.prosperity; }
    }
    int flows=0;
    for (int a=0;a<SCPS_MAX_COUNTRY;a++){
        if (top_prov[a]<0) continue;
        for (int b=a+1;b<SCPS_MAX_COUNTRY;b++){
            if (!dp->migration_pact[a][b] || top_prov[b]<0) continue;
            if (diplo_status(dp,a,b)==DIPLO_WAR) continue;   /* la guerre suspend l'échange */
            float sa=attract[a], sb=attract[b], tot_attr=sa+sb+1e-3f;
            ProvincePop *pa=&e->prov[top_prov[a]].pop, *pb=&e->prov[top_prov[b]].pop;
            /* a→b : ∝ attractivité de b (b, s'il est plus prospère, reçoit plus). */
            PopGroup *da=(PopGroup*)province_dominant(pa);
            if (da){
                long amt=(long)((float)da->count*frac*(2.f*sb/tot_attr));
                if (amt>=fmin && migration_move(pa,pb,(int)(da-pa->groups),amt,demography_dyn_id_next(),ARR_MIGRANT)) flows++;
            }
            /* b→a : ∝ attractivité de a (re-résoudre le dominant : le move a pu compacter). */
            PopGroup *db=(PopGroup*)province_dominant(pb);
            if (db){
                long amt=(long)((float)db->count*frac*(2.f*sa/tot_attr));
                if (amt>=fmin && migration_move(pb,pa,(int)(db-pb->groups),amt,demography_dyn_id_next(),ARR_MIGRANT)) flows++;
            }
        }
    }
    g_migration_pact_flows += flows;
    return flows;
}

void demography_tick(World *w, WorldEconomy *econ, WorldLegitimacy *wl,
                     ModifierStack *drift, float P, float K, float dt){
    if (dt<=0.f) dt=1.f;
    /* RE-KEY PROVINCE (T1) : econ->region[r].pop n'est qu'un MIROIR de la province
     * représentative (copié par econ_aggregate_regions) — y écrire des groupes est perdu
     * au prochain econ_tick. Les groupes VIVENT sur econ->prov[rep_pid].pop (la vérité) ;
     * on route dessus via econ_region_rep_province (ancrage FIGÉ, jamais recalculé à
     * l'état courant — cf. scps_econ.h). L'itération reste PAR RÉGION (adjacence de
     * migration, satisfaction/coercition lues en agrégat) : même comportement, juste le
     * bon grain d'écriture. Une région sans province représentative valide (jamais
     * colonisée) est ignorée, comme avant (pp->n_groups restait à 0). */
    /* 1. Par région : L par groupe, assimilation, rafraîchir le cache, sync dominante. */
    for (int r=0; r<econ->n_regions; r++){
        RegionEconomy *re=&econ->region[r];
        int rp=econ_region_rep_province(econ, r);
        if (rp<0 || rp>=econ->n_prov) continue;
        ProvinceEconomy *pe=&econ->prov[rp];
        ProvincePop *pp=&pe->pop;
        if (pp->n_groups<=0 || !pe->culture.settled) continue;
        pp->prosperity = re->prosperity;
        const PopCulture *crown = (re->owner>=0) ? dom_ruling_culture(w,econ,re->owner) : &pe->culture;
        for (int i=0;i<pp->n_groups;i++){
            group_L_tick(&pp->groups[i], drift, crown, re->satisfaction, 0.f, re->coercion, re->build.H_coerc);
            pp->groups[i].culture = group_culture_effective(&pp->groups[i], drift);
        }
        assimilation_tick(pp, drift, P, K, dt);                  /* dérive durable (∝ D∞), au pas dt */
        float yh = (wl && r < SCPS_MAX_REG) ? wl->years_held[r] : 100.f;
        faith_convert_tick(pp, crown, yh, dt);                  /* la FOI converge vers le trône (§2) */
        for (int i=0;i<pp->n_groups;i++)
            pp->groups[i].culture = group_culture_effective(&pp->groups[i], drift);
        const PopGroup *dom=province_dominant(pp);
        if (dom){ pe->culture = dom->culture; re->culture = dom->culture; } /* la dominante mène ; miroir région immédiat (lecteurs de la même frame) */
    }
    /* 2. Migration : les groupes affluent vers la prospérité voisine (round-robin),
     *    toujours via la province représentative source/destination. */
    for (int r=0; r<econ->n_regions; r++){
        RegionEconomy *re=&econ->region[r];
        int rp=econ_region_rep_province(econ, r);
        if (rp<0 || rp>=econ->n_prov) continue;
        ProvinceEconomy *pe=&econ->prov[rp];
        if (pe->pop.n_groups<=0 || !pe->culture.settled) continue;
        int best=-1; float bestp=re->prosperity + MIG_GRADIENT;
        for (int s=0;s<econ->n_regions;s++){
            int rps=econ_region_rep_province(econ, s);
            if (rps<0 || rps>=econ->n_prov) continue;
            const ProvinceEconomy *pes=&econ->prov[rps];
            if (econ->adj[r][s] && pes->culture.settled && pes->pop.n_groups>0
                && econ->region[s].prosperity>bestp){ bestp=econ->region[s].prosperity; best=s; }
        }
        if (best<0) continue;
        int rpb=econ_region_rep_province(econ, best);
        if (rpb<0 || rpb>=econ->n_prov) continue;
        PopGroup *dom=(PopGroup*)province_dominant(&pe->pop);
        long amount=dom->count/MIG_FRACTION;
        if (amount<MIG_MIN) continue;
        int gi=(int)(dom-pe->pop.groups);
        migration_move(&pe->pop, &econ->prov[rpb].pop, gi, amount, demography_dyn_id_next(), ARR_MIGRANT);
    }
    /* 3. ÉMERGENCE DE CLASSE : la classe de chaque groupe sort des emplois (capitale
     *    + ateliers). Après migration/assimilation, le tissu social se recompose. */
    for (int r=0; r<econ->n_regions; r++){
        int rp=econ_region_rep_province(econ, r);
        if (rp<0 || rp>=econ->n_prov) continue;
        ProvinceEconomy *pe=&econ->prov[rp];
        if (pe->pop.n_groups>0 && pe->culture.settled) demography_emerge_classes(pe);
    }
}

void demography_on_conquest(World *w, WorldEconomy *econ, ModifierStack *drift, int region, int conqueror){
    (void)drift;
    if (region<0||region>=econ->n_regions) return;
    /* RE-KEY PROVINCE (T1) : les groupes vivent sur la province représentative,
     * comme demography_tick/_attach (même ancrage econ_region_rep_province). */
    int rp=econ_region_rep_province(econ, region);
    if (rp<0 || rp>=econ->n_prov) return;
    ProvincePop *pp=&econ->prov[rp].pop;
    if (pp->n_groups<=0) return;
    const PopCulture *crown = dom_ruling_culture(w,econ,conqueror);
    Heritage ch = crown ? crown->heritage : HERITAGE_ADAPTATIF;
    /* les conquis deviennent une minorité restive : l'intégration repart de zéro.
     * BRASSAGE — une nation native d'un AUTRE héritage vient d'être ABSORBÉE : elle
     * n'est plus « de naissance » sous ce maître, c'est un peuple SOUMIS à digérer
     * (Gaule romaine). Son savoir DIFFUSE (métabolisation), à côté de l'irrédentisme
     * que porte déjà son L bas. Les conquis du MÊME héritage restent natifs (rien à
     * digérer) mais aussi restifs. On ne re-soumet PAS les diaspora/déportés déjà là. */
    for (int i=0;i<pp->n_groups;i++){
        PopGroup *g=&pp->groups[i];
        g->integration=0.1f; g->L=2.0f; g->agit_base=agit_from_L(2.0f);
        if (g->arrival==ARR_NATIF && g->heritage!=ch){ g->arrival=ARR_SOUMIS; g->diaspora=true; }
    }
    /* on dépose des colons de la couronne (culture du conquérant) → D INTERNE vécu.
     * Ce sont des gens du MAÎTRE (heritage=conquérant) → ne se métabolisent pas (rien
     * d'allophone à digérer) : arrival MIGRANT sans effet de diffusion (heritage==natif). */
    if (crown && pp->n_groups<SCPS_MAX_GROUPS){
        long total=province_total_pop(pp);
        PopGroup g; memset(&g,0,sizeof g);
        g.heritage=crown->heritage; g.origin_sphere=heritage_sphere(crown->heritage);
        g.origin=*crown; g.culture=*crown; g.klass=CLASS_ELITE;
        g.count=total/5+50; g.L=7.f; g.agit_base=agit_from_L(7.f); g.integration=1.f;
        g.diaspora=true; g.arrival=ARR_MIGRANT; g.drift_id=demography_dyn_id_next();
        pp->groups[pp->n_groups++]=g;
    }
}
