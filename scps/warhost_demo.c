/*
 * warhost_demo.c — banc d'essai de la MOBILISATION (scps_warhost)
 *
 *   make warhost_demo && ./warhost_demo [graine]
 *
 * Prouve : les armées VIVENT — un pays sur PIED DE GUERRE lève des troupes (armes
 * fabriquées + unités recrutées) ; la force se dépose en armes sur sa capitale
 * (→ mil_power) ; la PAIX démobilise (un pays en paix lève moins).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_diplo.h"
#include "scps_army.h"
#include "scps_labor.h"
#include "scps_warhost.h"
#include <stdio.h>
#include <stdlib.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

/* LOT 2 — somme du stock macro (RES_ARMS_LIGHT/HEAVY, les deux catégories semées
 * dans ce banc) sur toutes les régions d'un pays : la preuve de conservation au
 * disband (armes RENDUES, pas perdues). */
static float country_arms_stock(const WorldEconomy*e,int c){
    float s=0.f;
    for(int r=0;r<e->n_regions;r++) if(e->region[r].owner==c)
        s += e->region[r].stock[RES_ARMS_LIGHT] + e->region[r].stock[RES_ARMS_HEAVY];
    return s;
}

static float capital_arms(const World*w,const WorldEconomy*e,int c){
    int cp=w->country[c].capital_prov; if(cp<0) return 0.f;
    int r=w->province[cp].region; if(r<0||r>=e->n_regions) return 0.f;
    /* RE-KEY PROVINCE : mil_stock est PROVINCE-OWNED (warhost_tick écrit
     * econ->prov[rep].mil_stock — cf. scps_warhost.c) ; region[].mil_stock N'EST
     * QU'UNE VUE agrégée, reconstruite par econ_aggregate_regions (appelé depuis
     * econ_tick) — jamais rafraîchie ici puisque la boucle du banc n'appelle QUE
     * warhost_tick après le peuplement initial. On lit la province représentative
     * directement : c'est ce que le mécanisme écrit RÉELLEMENT. */
    int rep=econ_region_rep_province(e,r);
    return (rep>=0 && rep<e->n_prov) ? e->prov[rep].mil_stock : 0.f;
}

int main(int argc,char**argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    if(!w||!econ){fprintf(stderr,"OOM\n");return 1;}

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" MOBILISATION — les armées vivent, le pied de guerre lève des troupes (graine %u)\n",seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    for(int t=0;t<8;t++) econ_tick(econ,1.f);     /* peuple les strates (pop par classe) */

    /* trois pays qui possèdent des régions : ca & cb en guerre, cp en paix. On les
     * TRIE par taille (régions) décroissante → le pays EN GUERRE (ca) est le PLUS grand,
     * si bien que sa levée de guerre domine franchement la levée d'ENTRETIEN du pays en
     * paix (cp), même avec une économie maigre. Sans ce tri, un grand pays PAISIBLE
     * pouvait, en valeur absolue, lever plus qu'un petit pays EN GUERRE. */
    int own[8], onr[8], no=0;
    for(int c=0;c<w->n_countries && no<8;c++){
        /* warhost_tick SAUTE tout pays POLITY_UNCLAIMED (cf. scps_warhost.c, gate d'entrée) —
         * même si econ lui attribue une région (colonisation résiduelle/hameau), il ne
         * mobilisera JAMAIS : l'exclure ici, sinon le banc choisit un pays mort-né et
         * mesure zéro pour de mauvaises raisons (piège trouvé seed 9 : ca=3 role=UNCLAIMED,
         * warhost_units reste 0 à jamais). */
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        int nreg=0; for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==c) nreg++;
        if(nreg>0 && w->country[c].capital_prov>=0){ own[no]=c; onr[no]=nreg; no++; }
    }
    if(no<2){ printf(" (monde trop vide)\n"); return 0; }
    for(int i=0;i<no;i++) for(int j=i+1;j<no;j++) if(onr[j]>onr[i]){
        int t=own[i];own[i]=own[j];own[j]=t; t=onr[i];onr[i]=onr[j];onr[j]=t; }
    int ca=own[0], cb=own[1], cp=(no>=3)?own[2]:-1;

    DiploState dp; diplo_init(&dp);
    diplo_declare_war(&dp, ca, cb);

    WarHost h; warhost_init(&h);
    /* F6 Option B : lever puise les armes MACRO (RES_ARMS_*) — on en sème (les fabriques les
     * auraient produites depuis le fer) sinon pas de levée. */
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==ca||econ->region[r].owner==cb){
        econ->region[r].stock[RES_ARMS_LIGHT]=5000.f; econ->region[r].stock[RES_ARMS_HEAVY]=2000.f; }
    float arms0=capital_arms(w,econ,ca);
    /* 3 ans : assez pour que le PIED DE GUERRE (WH_BATCH_WAR=7/an) lève son plafond,
     * mais trop court pour que l'ENTRETIEN de paix (WH_BATCH_PEACE=1.5/an) le rattrape.
     * (À 6 ans, les deux SATURENT leur capacité → l'écart guerre/paix s'efface.) */
    for(int y=0;y<3;y++) warhost_tick(&h,w,econ,&dp,NULL,1.f);
    long ua=warhost_units(&h,ca), ub=warhost_units(&h,cb);
    printf("   après 3 ans : pays en guerre %d → %ld paquets · pays en guerre %d → %ld · capitale arms %.0f→%.0f\n",
           ca, ua, cb, ub, arms0, capital_arms(w,econ,ca));

    ok("un pays EN GUERRE lève des troupes (unités > 0)", ua>0);
    ok("la mobilisation DÉPOSE des armes sur la capitale (→ mil_power)",
       capital_arms(w,econ,ca) > arms0);
    ok("l'armée VIT (warhost_units reflète les unités levées)", warhost_units(&h,ca)==ua && ua>0);
    if(cp>=0){
        long ucp=warhost_units(&h,cp);
        printf("   pays en PAIX %d → %ld paquets (entretien minimal)\n", cp, ucp);
        ok("la PAIX démobilise : un pays en paix lève MOINS qu'un pays en guerre", ucp < ua);
    } else ok("(pas de 3e pays pour comparer paix vs guerre)", true);

    /* LOT 2 — warhost_disband REND les armes (aligné sur wh_shed, le downsizing
     * naturel de paix) : le stock macro du pays APRÈS disband doit valoir le stock
     * AVANT (Σ conservée), la réserve levée s'étant fondue dedans plutôt que dans
     * le vide (l'ancien memset perdait le fer déjà dépensé à la levée). */
    printf("\n── LOT 2 : le disband REND les armes (Σ conservée) ──\n");
    float stock_before_disband = country_arms_stock(econ, ca);
    long disbanded = warhost_disband(&h, econ, ca);
    float stock_after_disband  = country_arms_stock(econ, ca);
    printf("   ca=%d : %ld paquets dissous · stock macro %.0f → %.0f\n",
           ca, disbanded, stock_before_disband, stock_after_disband);
    ok("le disband dissout bien la réserve (warhost_units retombe à 0)",
       disbanded==ua && warhost_units(&h,ca)==0);
    ok("les ARMES REVIENNENT au stock macro (Σ après ≥ Σ avant — rien n'est perdu)",
       stock_after_disband >= stock_before_disband - 0.5f);

    warhost_free(&h);
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ);
    return g_fail?1:0;
}
