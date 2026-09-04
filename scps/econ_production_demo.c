/*
 * econ_production_demo.c — l'épine dorsale : fer+charbon → métal → outils → productivité
 *
 *   make econ_production_demo && ./econ_production_demo [graine]
 *
 * Les biens de PRODUCTION sont des intrants, pas des paniers. La chaîne centrale :
 *   Fer + Charbon → (Fonderie) Métal → (Atelier) Outils.
 * Et les OUTILS sont le MULTIPLICATEUR de productivité : leur stock booste
 * l'extraction et la manufacture. On vérifie :
 *   1. La fonderie produit du MÉTAL (fer + charbon).
 *   2. L'atelier d'outillage produit des OUTILS (métal + bois).
 *   3. Les outils MONTENT la production (même région, plus d'outils → plus de PIB).
 *   4. Les outils s'USENT (sans entretien, le stock décroît).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_tune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

/* Province REPRÉSENTATIVE d'une région (charte PROVINCE_MODEL.md : l'économie
 * vit à la province, la région n'est qu'un agrégat) — repli : scan direct. */
static int rep_prov(WorldEconomy *e, int r){
    if (r>=0 && r<SCPS_MAX_REG && e->region_rep_prov[r]>=0) return e->region_rep_prov[r];
    for (int p=0;p<e->n_prov;p++) if (e->prov[p].region==r) return p;
    return -1;
}

/* ÉTEINT toute province SŒUR (même région, ≠ pid) : la région AGRÈGE (règle 2) —
 * sans ça les autres provinces (semées par world_generate/gen_population)
 * contaminent la supply agrégée que le banc lit sur e->region[r]. */
static void mute_siblings(WorldEconomy *e, int r, int pid){
    for (int p=0;p<e->n_prov;p++){
        if (p==pid || e->prov[p].region!=r) continue;
        ProvinceEconomy *pe=&e->prov[p];
        pe->active=false; pe->colonized=false;
        memset(pe->strata,0,sizeof pe->strata);
        for (int k=0;k<RES_COUNT;k++){ pe->raw_cap[k]=0.f; pe->supply[k]=0.f; }
    }
}

/* STOCK NATIONAL (2026-09-03) : l'entrepôt vit au grain PAYS — une province SANS maître
 * (l'ancienne isolation `owner=-1`) ne stocke PLUS rien du tout, le banc ne pourrait plus
 * ni doter ni mesurer les outils. Chaque rig reçoit donc un pays SYNTHÉTIQUE d'UNE
 * province, pris en haut de la plage (jamais attribué par le worldgen). */
static int rig_cid(int r){ return SCPS_MAX_COUNTRY-1-r; }

/* Fige la PROVINCE représentative de la région r en banc d'essai de production :
 * fer+charbon+bois+grain, plus un atelier d'outillage (fer + bois → outils,
 * DIRECT), sur une pop de travail donnée. */
static void rig(WorldEconomy *e, int r, float tools){
    int pid=rep_prov(e,r);
    mute_siblings(e,r,pid);
    ProvinceEconomy *re=&e->prov[pid];
    int cid=rig_cid(r);
    re->active=true; re->colonized=true; re->culture.settled=true;
    re->owner=(int16_t)cid;   /* un empire d'UNE province : le seul qui puise dans SON entrepôt */
    econ_set_human(cid);      /* ISOLATION : §NF ne bâtit JAMAIS chez la main humaine — même effet
                               * que l'ancien owner=-1 (pas d'atelier qui mange le métal accumulé),
                               * mais la province garde un entrepôt national LISIBLE. */
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; e->nat_stock[cid][k]=0.f; }
    re->raw_cap[RES_IRON]=4.f; re->raw_cap[RES_COAL]=4.f;
    re->raw_cap[RES_WOOD]=4.f; re->raw_cap[RES_GRAIN]=8.f;
    re->n_bld=0;
    re->bld[re->n_bld].type=BLD_TOOLWORKS; re->bld[re->n_bld].level=3.f; re->n_bld++;
    re->strata[CLASS_LABORER].pop=600.f;  re->strata[CLASS_LABORER].wealth=400.f;
    re->strata[CLASS_BOURGEOIS].pop=100.f;re->strata[CLASS_BOURGEOIS].wealth=200.f;
    re->strata[CLASS_ELITE].pop=50.f;     re->strata[CLASS_ELITE].wealth=300.f;
    e->nat_stock[cid][RES_TOOLS]=tools;
}

int main(int argc, char **argv){
    /* Fixture STABLE : monde pinné à ~320 territoires (le banc teste l'usure/chaîne d'outils, pas
     * le scaling f(empires) ; un monde géant dilue la pop/labor par région et fausse les seuils). */
    if (!getenv("SCPS_TUNE")){
        tune_set("WORLD_PROV_BASE",320.f);
        tune_set("WORLD_PROV_PER_EMPIRE",0.f);
        tune_set("WORLD_PROV_PER_CITY",0.f);
    }
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World)); WorldEconomy *e=malloc(sizeof(WorldEconomy));
    if(!w||!e){ fprintf(stderr,"OOM\n"); return 1; }
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" ÉPINE DORSALE — fer+bois→outils→productivité — graine %u\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p); econ_init(e,w); gen_population(w,e);
    if (e->n_regions<3){ fprintf(stderr,"monde trop petit\n"); return 1; }

    /* Régression province → agrégat : raw_boost vit sur ProvinceEconomy ;
     * RegionEconomy n'en est que le max calculé. L'ancien écrivain IA posait le
     * palier sur le miroir, donc l'agrégation suivante le faisait disparaître. */
    printf("\n── 0. Le palier d'exploitation provincial survit à l'agrégation ──\n");
    {
        int probe_r=0, probe_p=-1;
        uint8_t before=0;
        for (int p0=0;p0<e->n_prov && p0<SCPS_MAX_PROV;p0++) if (e->prov[p0].region==probe_r){
            if (probe_p<0 || e->prov[p0].raw_boost[RES_WOOD]>=before){
                probe_p=p0; before=e->prov[p0].raw_boost[RES_WOOD];
            }
        }
        bool room=(probe_p>=0 && before<UINT8_MAX);
        uint8_t raised=room?(uint8_t)(before+1u):before;
        if (room) e->prov[probe_p].raw_boost[RES_WOOD]=raised;
        econ_aggregate_regions(e);
        bool once=room && e->region[probe_r].raw_boost[RES_WOOD]==raised;
        econ_aggregate_regions(e);
        bool twice=room && e->region[probe_r].raw_boost[RES_WOOD]==raised;
        ok("le palier provincial est projeté dans la région", once);
        ok("une seconde agrégation ne fait pas disparaître le palier", twice);
        if (room) e->prov[probe_p].raw_boost[RES_WOOD]=before;
        econ_aggregate_regions(e);   /* restitue la fixture avant les mesures de production */
    }

    /* ═══ 1. La chaîne réelle (DIRECTE) ═════════════════════════════════ */
    printf("\n── 1. Atelier d'outillage (fer + bois → outils, DIRECT — plus de métal) ──\n");
    /* Région 1 : l'atelier sort des OUTILS directement du fer + bois. */
    rig(e, 1, 0.f);
    for (int t=0;t<6;t++) econ_tick(e,1.f);
    float tools=econ_country_stock_sum(e, rig_cid(1), RES_TOOLS);
    printf("   chaîne directe : outils=%.1f\n", tools);
    ok("l'Atelier produit des OUTILS (fer + bois, DIRECT)", tools > 0.5f);

    /* ═══ 3. Les outils MULTIPLIENT la productivité ═════════════════════ */
    printf("\n── 3. Les outils montent la production (le multiplicateur) ──\n");
    /* REFONTE A0 : on mesure l'EXTRACTION BRUTE (que les outils multiplient via prod_mult),
     * pas le PIB (valeur ajoutée) — désormais confondu par le marché des outils (le stock
     * d'outils déprime leur prix → la manufacture d'outils, donc la VA, varie en sens inverse). */
    rig(e, 1, 0.f);    /* site sans outils */
    econ_tick(e,1.f);
    float out_none=0.f; for(int g=1;g<RES_PROD_FIRST;g++) out_none+=e->region[1].supply[g];
    rig(e, 1, 600.f);  /* région IDENTIQUE mais bien outillée */
    econ_tick(e,1.f);
    float out_tools=0.f; for(int g=1;g<RES_PROD_FIRST;g++) out_tools+=e->region[1].supply[g];
    printf("   extraction brute du même site : sans outils=%.1f vs bien outillé=%.1f\n", out_none, out_tools);
    ok("un site BIEN OUTILLÉ extrait plus qu'un site sans outils (productivité)",
       out_tools > out_none + 0.5f);

    /* ═══ 4. Les outils s'usent ═════════════════════════════════════════ */
    printf("\n── 4. Les outils s'usent (il faut les entretenir) ──\n");
    int pid2=rep_prov(e,2);
    mute_siblings(e,2,pid2);
    ProvinceEconomy *re2=&e->prov[pid2];
    re2->active=true; re2->colonized=true; re2->culture.settled=true;
    /* EMPIRE ISOLÉ (slot pays INUTILISÉ) : l'usure du PARC NATIONAL (×0.97/tick) est
     * hoistée par EMPIRE → une province SANS owner valide n'entre dans aucun pool et
     * n'use JAMAIS ses outils. On en fait un empire mono-province (pool = cette seule
     * province, pshare=1 ⇒ usure NETTE) ; sans ressources ⇒ pas de §NF qui rebâtirait
     * l'atelier. (La re-baseline worldgen #3 a fait passer region[2] en non-possédée.) */
    int own2 = (w->n_countries < SCPS_MAX_COUNTRY) ? w->n_countries : SCPS_MAX_COUNTRY-1;
    re2->owner = (int16_t)own2;
    for (int k=0;k<RES_COUNT;k++){ re2->raw_cap[k]=0.f; e->nat_stock[own2][k]=0.f; }
    re2->n_bld=0;   /* AUCUN atelier → pas d'entretien */
    re2->strata[CLASS_LABORER].pop=600.f;
    e->nat_stock[own2][RES_TOOLS]=1000.f;   /* le PARC est national : c'est lui qui s'use */
    float tw0=econ_country_stock_sum(e, own2, RES_TOOLS);
    for (int t=0;t<10;t++) econ_tick(e,1.f);
    float tw1=econ_country_stock_sum(e, own2, RES_TOOLS);
    printf("   stock d'outils sans entretien : %.0f → %.0f (usure)\n", tw0, tw1);
    ok("sans atelier pour les entretenir, le stock d'outils DÉCROÎT", tw1 < tw0 - 1.f);

    /* ═══ 5. LA CADENCE DE L'INITIATIVE PRIVÉE ══════════════════════════
     * Décision joueur 2026-09-04 (« une initiative privée par mois ») : le semis privé
     * (econ_ip_invest_tick) allait jusqu'à 6 manufactures par MOIS et par pays sur 250 ans.
     * On monte quatre provinces JUMELLES d'un même empire synthétique, toutes riches et
     * toutes en pénurie — seule l'INTENSITÉ de la pénurie les distingue — et on vérifie :
     *   PRIV_SEED_PER_MONTH=0 (kill-switch) : les quatre sèment le même mois (illimité) ;
     *   PRIV_SEED_PER_MONTH=1 (défaut)      : UNE seule sème, et c'est LA PLUS RENTABLE. */
    printf("\n── 5. Initiative privée : au plus UNE par mois et par pays ──\n");
    {
        const int NP=4;
        int cid = SCPS_MAX_COUNTRY-9;          /* empire synthétique, jamais attribué par le worldgen */
        int pids[4]; int npid=0;
        for (int p=0;p<e->n_prov && npid<NP;p++) pids[npid++]=p;
        /* Quatre jumelles : même pop, même richesse, même dotation — SEUL le prix diffère
         * (×2, ×3, ×5, ×4 de la base) : la 3e est la plus rentable, et ce n'est PAS la
         * plus petite pid, donc le banc distingue « la plus rentable » de « la première ». */
        const float PMULT[4]={2.f,3.f,5.f,4.f};
        int winner=2;
        /* Le palier de besoins d'un pays SYNTHÉTIQUE n'a jamais tourné (g_needs_tier_held=0
         * ⇒ le panier s'arrête au grain, qu'aucune recette ne fabrique) : on repasse par le
         * chemin LEGACY (NEEDS_TIER_POP=0 ⇒ palier déduit de la pop locale), où une pop de
         * 5 000 ouvre les paliers manufacturés (sel/étoffe). */
        float need_tier_save=tune_f("NEEDS_TIER_POP",3000.f);
        tune_set("NEEDS_TIER_POP",0.f);
        for (int i=0;i<npid;i++){
            ProvinceEconomy *pe=&e->prov[pids[i]];
            memset(pe->strata,0,sizeof pe->strata);
            pe->active=true; pe->colonized=true; pe->culture.settled=true;
            pe->owner=(int16_t)cid; pe->n_bld=0;
            pe->strata[CLASS_LABORER].pop  =4000.f; pe->strata[CLASS_LABORER].wealth  =40000.f;
            pe->strata[CLASS_BOURGEOIS].pop=1000.f; pe->strata[CLASS_BOURGEOIS].wealth=1000000.f;
            for (int k=0;k<RES_COUNT;k++){
                pe->raw_cap[k]=8.f;                                  /* toute recette est nourrissable ICI */
                pe->price[k]=econ_base_price((Resource)k)*PMULT[i];                 /* pénurie franche, d'intensité DIFFÉRENTE */
                e->nat_stock[cid][k]=100000.f;                       /* le chantier ne manque de rien */
            }
        }
        int before[4]; for (int i=0;i<npid;i++) before[i]=e->prov[pids[i]].n_bld;
        tune_set("PRIV_SEED_PER_MONTH",0.f);   /* kill-switch : illimité (le comportement d'avant) */
        econ_ip_invest_tick(e);
        int free_seeds=0; for (int i=0;i<npid;i++) free_seeds += e->prov[pids[i]].n_bld-before[i];
        { const long *ipr=NULL; econ_ip_reason_stats(&ipr);   /* P2 : la table des raisons, lisible ici aussi */
          printf("   raisons (province-mois) :");
          for (int k=0;k<IPR_COUNT;k++) printf(" %s %ld ·", econ_ip_reason_name(k), ipr[k]);
          printf("\n"); }
        printf("   cadence ÉTEINTE (0) : %d semis le même mois sur %d provinces\n", free_seeds, npid);
        ok("sans cadence, plusieurs manufactures privées naissent le même mois", free_seeds>1);

        for (int i=0;i<npid;i++){          /* on remet les quatre jumelles à zéro */
            ProvinceEconomy *pe=&e->prov[pids[i]];
            pe->n_bld=0;
            pe->strata[CLASS_BOURGEOIS].wealth=1000000.f;
            for (int k=0;k<RES_COUNT;k++) pe->price[k]=econ_base_price((Resource)k)*PMULT[i];
        }
        e->ip_seed_credit[cid]=0.f;
        tune_set("PRIV_SEED_PER_MONTH",1.f);   /* le DÉFAUT : une par mois et par pays */
        econ_ip_invest_tick(e);
        int capped=0, who=-1;
        for (int i=0;i<npid;i++) if (e->prov[pids[i]].n_bld>0){ capped++; who=i; }
        printf("   cadence à 1/mois : %d semis (province gagnante %d, pénurie ×%.0f)\n",
               capped, who, who>=0?PMULT[who]:0.f);
        ok("à cadence 1, UNE seule manufacture privée naît par mois et par pays", capped==1);
        ok("la gagnante est la province la PLUS RENTABLE (pénurie la plus intense), pas la plus petite pid",
           who==winner);
        tune_set("NEEDS_TIER_POP",need_tier_save);   /* la fixture repart telle qu'elle était */
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(e);
    return g_fail?1:0;
}
