/*
 * social_demo.c — le tissu social : brasserie, boisson culturelle, foi
 *
 *   make social_demo && ./social_demo [graine]
 *
 * Première passe du catalogue SOCIAL (au-delà des chaînes matérielles déjà
 * câblées). On vérifie :
 *   1. BRASSERIE — le grain devient de la BIÈRE (la chaîne vivrière du commun).
 *   2. VARIANTE CULTURELLE — le palier MORAL (boisson) est une variante : une
 *      culture de basse subsistance (clans/métallurgistes/claniques) est CONTENTE avec la
 *      bière et BOUDE le eau-de-vie ; une culture urbaine, l'inverse.
 *   3. FOI — un Temple bâti SOUTIENT la légitimité locale (sacraliser le trône
 *      apaise sans réprimer) — la coordonnée que la légitimité LIT.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_legitimacy.h"
#include "scps_agency.h"
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
 * vit à la province, la région n'est qu'un agrégat recalculé par econ_tick) —
 * repli : scan direct si le cache region_rep_prov n'a rien retenu. Même idiome
 * que econ_tax_demo.c. */
static int rep_prov(WorldEconomy *e, int r){
    if (r>=0 && r<SCPS_MAX_REG && e->region_rep_prov[r]>=0) return e->region_rep_prov[r];
    for (int p=0;p<e->n_prov;p++) if (e->prov[p].region==r) return p;
    return -1;
}

/* STOCK NATIONAL (2026-09-03) : l'entrepôt vit au grain PAYS — une province SANS maître
 * (l'ancienne isolation `owner=-1`) ne stocke PLUS rien, et les provinces sans maître
 * partagent toutes le même puits « no man's land ». L'isolation VRAIE se dit désormais
 * autrement : chaque fixture est un EMPIRE d'UNE province, sur un slot pays synthétique
 * (jamais attribué par le worldgen) — son entrepôt n'appartient qu'à elle. econ_set_human
 * l'exempte en plus du §NF (la construction demande-menée), comme le faisait owner=-1. */
static int rig_cid(int r){ return SCPS_MAX_COUNTRY-1-r; }
static void isolate(WorldEconomy *e, ProvinceEconomy *re, int r){
    int cid=rig_cid(r);
    re->owner=(int16_t)cid;
    econ_set_human(cid);   /* §NF ne bâtit jamais chez la main humaine : rien ne pousse dans le dos du banc */
    for (int k=0;k<RES_COUNT;k++) e->nat_stock[cid][k]=0.f;
    e->nat_treasury[cid]=0.f;
}

/* Société servie avec UNE boisson donnée, pour une culture de subsistance donnée.
 * Tous les AUTRES biens sociaux sont abondants → la BOISSON est la variable.
 * Rige la PROVINCE (la vérité) ; la région-r n'a qu'UNE province membre après
 * econ_init ⇒ l'agrégat post-tick e->region[r] reflète exactement cette province. */
static float society_with_drink(WorldEconomy *e, int r, float subsistance, Resource drink){
    int pid=rep_prov(e,r); if (pid<0) return 0.f;
    ProvinceEconomy *re=&e->prov[pid];
    re->active=true; re->colonized=true; re->culture.settled=true;
    re->culture.subsistance=subsistance;
    isolate(e, re, r);   /* polité ISOLÉE : son propre entrepôt national (le banc compare UNE région) */
    re->n_bld=0; re->coercion=0.f; re->over_tax=0.f;
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->price[k]=1.0f; }
    /* MONNAIE M10 — P1 : ce banc rend TOUS les autres biens sociaux du Laborer (FISH/WOOD/
     * TUNIQUE) triviallement abondants (1e5) POUR isoler le signal boisson — mais depuis P1,
     * les Kc=active_needs-1 biens les PLUS DISPONIBLES remplissent les paliers (n'importe quel
     * bien compte) : à pop=1250 (tier 1, Kc=1), les 3 autres biens abondants (score≈1.0)
     * évincent systématiquement la boisson hors-culture (score≈0.5, DRINK_OFFCULT) de l'UNIQUE
     * slot — le signal testé disparaît, pas un bug moteur. Il faut donc Kc ≥ 4 = n_cand
     * Laborer (EAU_DE_VIE/FISH/WOOD/TUNIQUE) ⇒ AUCUNE compétition de slot, la boisson
     * compte TOUJOURS, le signal hors-culture redevient visible.
     * ⚠ 2026-09-03 — CE QUI OUVRE LES PALIERS A CHANGÉ DE GRAIN. Ce rig s'isolait par
     * `owner = -1`, ce qui faisait retomber econ_needs_active_for_country sur le chemin
     * LEGACY (capitale_max_tier, pop LOCALE : 4000 ⇒ T4 ⇒ Kc=4). Le stock étant désormais
     * NATIONAL, le rig DOIT avoir un pays — et le palier se lit alors sur l'échelle M10-P1,
     * pilotée par la pop de L'EMPIRE : Kc = 1 + paliers franchis, seuils
     * NEEDS_TIER_POP × NEEDS_TIER_GROWTH^k = 3000 / 6000 / 12000… À 4000 hab, Kc valait 2 :
     * les biens abondants évinçaient la boisson et les quatre mesures saturaient à 1.00.
     * On donne donc au rig la pop qu'exige L'ÉCHELLE RÉELLE pour Kc=4 (≥ 12000), au lieu de
     * celle qu'exigeait l'ancien barreau local. Même intention, même invariant testé. */
    re->strata[CLASS_LABORER].pop=13000.f; re->strata[CLASS_LABORER].wealth=1e6f;
    re->strata[CLASS_BOURGEOIS].pop=200.f;re->strata[CLASS_BOURGEOIS].wealth=1e6f;
    re->strata[CLASS_ELITE].pop=50.f;     re->strata[CLASS_ELITE].wealth=1e6f;
    /* vivres + tous les biens sociaux NON-boisson, abondants */
    { float *ns=e->nat_stock[rig_cid(r)];
      ns[RES_GRAIN]=1e5f; ns[RES_FISH]=1e5f; ns[RES_WOOD]=1e5f;
      ns[RES_CLOTH]=1e5f; ns[RES_PAPER]=1e5f; ns[RES_SALT]=1e5f;
      ns[RES_FUR]=1e5f;   ns[RES_PRECIOUS_WARE]=1e5f; ns[RES_PRECIOUS_CLOTH]=1e5f;
      /* la SEULE boisson disponible = celle testée */
      ns[drink]=1e5f; }
    econ_tick(e, 1.f);
    return e->region[r].society_sat;
}

/* Satisfaction de l'ÉLITE servie d'UN luxe donné (orfèvrerie ou étoffe), pour une
 * culture de subsistance donnée. Les deux boissons sont servies (palier moral
 * neutralisé) → seul le LUXE varie. */
static float elite_sat_with_luxe(WorldEconomy *e, int r, float subsistance, Resource luxe){
    int pid=rep_prov(e,r); if (pid<0) return 0.f;
    ProvinceEconomy *re=&e->prov[pid];
    re->active=true; re->colonized=true; re->culture.settled=true;
    re->culture.subsistance=subsistance;
    isolate(e, re, r);   /* polité ISOLÉE : son propre entrepôt national (le banc compare UNE région) */
    re->n_bld=0; re->coercion=0.f; re->over_tax=0.f;
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->price[k]=1.0f; }
    /* §besoins progressifs : le palier STATUT (rang 4) demande plusieurs paliers ouverts.
     * ⚠ 2026-09-03 : même bascule de grain que society_with_drink ci-dessus — le rig ne
     * peut plus s'isoler par `owner = -1` (le stock est NATIONAL), donc le palier se lit sur
     * l'échelle M10-P1 pilotée par la pop d'EMPIRE (3000 × 2^k), et non plus sur le tier de
     * capitale LOCAL. Il faut ~25000 hab pour ouvrir le rang STATUT ; c'est le prix d'une
     * VRAIE métropole, ce que ce rig prétendait déjà être. */
    re->strata[CLASS_LABORER].pop=25000.f; re->strata[CLASS_LABORER].wealth=1e6f;
    re->strata[CLASS_BOURGEOIS].pop=400.f;re->strata[CLASS_BOURGEOIS].wealth=1e6f;
    re->strata[CLASS_ELITE].pop=200.f;    re->strata[CLASS_ELITE].wealth=1e6f;
    { float *ns=e->nat_stock[rig_cid(r)];
      ns[RES_GRAIN]=1e5f; ns[RES_FISH]=1e5f; ns[RES_WOOD]=1e5f;
      ns[RES_CLOTH]=1e5f; ns[RES_PAPER]=1e5f; ns[RES_SALT]=1e5f; ns[RES_FUR]=1e5f;
      ns[RES_EAU_DE_VIE]=1e5f; ns[RES_BEER]=1e5f;   /* boisson satisfaite quoi qu'il arrive */
      ns[luxe]=1e5f; }                              /* SEUL ce luxe est disponible */
    econ_tick(e, 1.f);
    return e->region[r].strata[CLASS_ELITE].satisfaction;
}

/* Recherche accumulée en un tick pour un niveau de SAVOIR bâti donné (toutes
 * choses égales par ailleurs : mêmes élites, même satisfaction). */
static float tech_with_savoir(WorldEconomy *e, int r, float savoir){
    int pid=rep_prov(e,r); if (pid<0) return 0.f;
    ProvinceEconomy *re=&e->prov[pid];
    re->active=true; re->colonized=true; re->culture.settled=true;
    isolate(e, re, r);   /* même isolation que les deux fixtures ci-dessus (entrepôt en propre) */
    re->culture.subsistance=8.f; re->coercion=0.f; re->over_tax=0.f;
    re->n_bld=0;
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->price[k]=1.0f; }
    re->strata[CLASS_LABORER].pop=500.f; re->strata[CLASS_LABORER].wealth=1e6f;
    re->strata[CLASS_BOURGEOIS].pop=100.f;re->strata[CLASS_BOURGEOIS].wealth=1e6f;
    re->strata[CLASS_ELITE].pop=100.f;    re->strata[CLASS_ELITE].wealth=1e5f;  /* les élites font le savoir */
    { float *ns=e->nat_stock[rig_cid(r)];
      ns[RES_GRAIN]=1e5f; ns[RES_FISH]=1e5f; ns[RES_WOOD]=1e5f;
      ns[RES_CLOTH]=1e5f; ns[RES_PAPER]=1e5f; ns[RES_SALT]=1e5f;
      ns[RES_FUR]=1e5f; ns[RES_PRECIOUS_WARE]=1e5f; ns[RES_PRECIOUS_CLOTH]=1e5f;
      ns[RES_EAU_DE_VIE]=1e5f; }
    memset(&re->build,0,sizeof re->build); re->build.savoir=savoir;
    re->tech=0.f;
    econ_tick(e, 1.f);
    return e->region[r].tech;
}

int main(int argc, char **argv){
    /* Fixture STABLE : monde pinné à ~320 territoires (le banc teste les chaînes/édifices, pas
     * le scaling f(empires) ; un monde géant dilue la pop/labor par région et fausse les seuils). */
    if (!getenv("SCPS_TUNE")){
        tune_set("WORLD_PROV_BASE",320.f);
        tune_set("WORLD_PROV_PER_EMPIRE",0.f);
        tune_set("WORLD_PROV_PER_CITY",0.f);
    }
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World));
    WorldEconomy *e=malloc(sizeof(WorldEconomy));
    WorldLegitimacy *wl=malloc(sizeof(WorldLegitimacy));
    if(!w||!e||!wl){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LE TISSU SOCIAL — brasserie · boisson culturelle · foi — graine %u\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p); econ_init(e,w);
    if (e->n_regions<4){ fprintf(stderr,"monde trop petit\n"); return 1; }

    /* ═══ 1. BRASSERIE — grain → bière ══════════════════════════════════ */
    printf("\n── 1. La brasserie : le grain devient de la bière ──\n");
    {
        int pid=rep_prov(e,0);
        if (pid<0){ fprintf(stderr,"région 0 sans province active\n"); return 1; }
        ProvinceEconomy *re=&e->prov[pid];
        /* polité ISOLÉE : un EMPIRE d'UNE province — son propre entrepôt national, sinon la
         * bière brassée ici se dilue sur les régions-sœurs NUES d'un vrai pays (worldgen ne
         * pose plus de brasserie : « carte nue », cités-états exceptées). Le banc compare UNE région. */
        re->active=true; re->colonized=true; re->culture.settled=true;
        isolate(e, re, 0);
        for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->price[k]=1.0f; }
        re->raw_cap[RES_GRAIN]=30.f;   /* grain ABONDANT : on ne brasse que le SURPLUS */
        re->n_bld=0;
        re->bld[re->n_bld].type=BLD_BREWERY; re->bld[re->n_bld].level=16.f; re->n_bld++;   /* M5 R3 : 3→8 quand la conso élastique est née ; 8→16 au déplafonnement du train de vie (CONSUME_ELASTIC_MAX 1.2→3.0, dépouillement 2026-08-11) — les riches boivent ×3, il faut brasser plus pour que du stock survive à l'assertion */
        re->strata[CLASS_LABORER].pop=400.f; re->strata[CLASS_LABORER].wealth=400.f;
        re->strata[CLASS_BOURGEOIS].pop=80.f; re->strata[CLASS_ELITE].pop=40.f;
        for (int t=0;t<6;t++) econ_tick(e,1.f);
        float beer=econ_country_stock_sum(e, rig_cid(0), RES_BEER);
        printf("   après 6 mois de brassage : bière en stock = %.1f\n", beer);
        ok("la Brasserie produit de la BIÈRE (grain → bière)", beer > 0.5f);
    }

    /* ═══ 1b. CHAÎNES MILITAIRES & SANTÉ — armurerie, poudrière, apothicaire ═ */
    printf("\n── 1b. Les chaînes complétées : armes, poudre, remèdes ──\n");
    {
        int pid=rep_prov(e,3);
        if (pid<0){ fprintf(stderr,"région 3 sans province active\n"); return 1; }
        ProvinceEconomy *re=&e->prov[pid];
        /* ISOLÉE comme la brasserie : un EMPIRE d'UNE province, son entrepôt PROPRE — sinon la
         * poudre/les remèdes se diluent avec les régions-sœurs d'un vrai pays, d'autant plus que
         * le monde est vaste. Le banc compare UNE région isolée → robuste à la taille du monde. */
        re->active=true; re->colonized=true; re->culture.settled=true;
        isolate(e, re, 3);
        for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->price[k]=1.0f; }
        re->raw_cap[RES_IRON]=4.f; re->raw_cap[RES_SALTPETER]=4.f; re->raw_cap[RES_COAL]=4.f;
        re->raw_cap[RES_MED_HERBS]=4.f;
        re->n_bld=0;
        re->bld[re->n_bld].type=BLD_ARMORY;     re->bld[re->n_bld].level=3.f; re->n_bld++;
        re->bld[re->n_bld].type=BLD_POWDERMILL; re->bld[re->n_bld].level=3.f; re->n_bld++;
        re->bld[re->n_bld].type=BLD_APOTHECARY; re->bld[re->n_bld].level=3.f; re->n_bld++;
        re->strata[CLASS_LABORER].pop=600.f; re->strata[CLASS_LABORER].wealth=400.f;
        re->strata[CLASS_BOURGEOIS].pop=100.f; re->strata[CLASS_ELITE].pop=50.f;
        for (int t=0;t<6;t++) econ_tick(e,1.f);
        float n_arms=econ_country_stock_sum(e, rig_cid(3), RES_ARMS);
        float n_powd=econ_country_stock_sum(e, rig_cid(3), RES_GUNPOWDER);
        float n_reme=econ_country_stock_sum(e, rig_cid(3), RES_REMEDE);
        printf("   après 6 mois : armes=%.1f · poudre=%.1f · remèdes=%.1f\n", n_arms, n_powd, n_reme);
        ok("l'Armurerie produit des ARMES (fer → armes)",            n_arms>0.5f);
        ok("la Poudrière produit de la POUDRE (salpêtre+charbon)",   n_powd>0.5f);
        ok("l'Apothicaire produit des REMÈDES (simples → remèdes)",  n_reme>0.5f);
    }

    /* ═══ 2. VARIANTE CULTURELLE — la bonne boisson contente ════════════ */
    printf("\n── 2. La variante culturelle : chacun sa boisson ──\n");
    float clan_beer = society_with_drink(e, 1, 2.0f, RES_BEER);   /* basse subsistance → bière */
    float clan_evdv = society_with_drink(e, 1, 2.0f, RES_EAU_DE_VIE);   /* … servi en eau-de-vie (off-culture) */
    float city_evdv = society_with_drink(e, 2, 8.0f, RES_EAU_DE_VIE);   /* haute subsistance → eau-de-vie */
    float city_beer = society_with_drink(e, 2, 8.0f, RES_BEER);   /* … servi en bière (off-culture) */
    printf("   clan (bière=%.2f vs eau-de-vie=%.2f) · cité (eau-de-vie=%.2f vs bière=%.2f)\n",
           clan_beer, clan_evdv, city_evdv, city_beer);
    ok("un peuple de clans est PLUS content avec sa bière qu'avec du eau-de-vie", clan_beer > clan_evdv + 0.02f);
    ok("un peuple des cités est PLUS content avec son eau-de-vie qu'avec de la bière", city_evdv > city_beer + 0.02f);

    /* ── 2b. STATUT (luxe) : orfèvrerie martiale vs étoffe raffinée ── */
    printf("\n── 2b. Le luxe d'élite : à chaque culture son statut ──\n");
    float clan_ware  = elite_sat_with_luxe(e, 1, 2.0f, RES_PRECIOUS_WARE);   /* martial → orfèvrerie */
    float clan_cloth = elite_sat_with_luxe(e, 1, 2.0f, RES_PRECIOUS_CLOTH);  /* … servi en étoffe (off) */
    float city_cloth = elite_sat_with_luxe(e, 2, 8.0f, RES_PRECIOUS_CLOTH);  /* raffiné → étoffe */
    float city_ware  = elite_sat_with_luxe(e, 2, 8.0f, RES_PRECIOUS_WARE);   /* … servi en orfèvrerie (off) */
    printf("   élite martiale (orfèvrerie=%.2f vs étoffe=%.2f) · raffinée (étoffe=%.2f vs orfèvrerie=%.2f)\n",
           clan_ware, clan_cloth, city_cloth, city_ware);
    ok("une élite martiale préfère l'ORFÈVRERIE à l'étoffe", clan_ware > clan_cloth + 0.02f);
    ok("une élite raffinée préfère l'ÉTOFFE à l'orfèvrerie", city_cloth > city_ware + 0.02f);

    /* ═══ 3. FOI — un Temple soutient la légitimité ═════════════════════ */
    printf("\n── 3. La foi : un Temple soutient le consentement ──\n");
    {
        /* Deux régions JUMELLES (même culture, propriétaire, satisfaction) ;
         * SEULE différence : l'une a la foi bâtie. */
        w->country[0].capital_prov = w->region[0].province_ids[0];
        w->country[0].role = POLITY_ANTAGONIST;
        for (int r=0;r<e->n_regions;r++){ e->region[r].owner=-1; }
        int RF=0, RN=3;   /* RF = avec foi, RN = sans */
        for (int two=0; two<2; two++){
            int r=(two==0)?RF:RN;
            RegionEconomy *re=&e->region[r];
            re->active=true; re->colonized=true; re->culture.settled=true; re->owner=0;
            re->culture.valeurs=5; re->culture.subsistance=5; re->culture.parente=5; re->culture.religion=5;
            re->satisfaction=0.5f; re->coercion=0.f;
            memset(&re->build,0,sizeof re->build);
        }
        e->region[RF].build.faith = 3.0f;          /* Temple bâti ici */
        memset(wl,0,sizeof *wl);
        legitimacy_init(wl, w, e);
        for (int t=0;t<60;t++) legitimacy_tick(wl, w, e, NULL);
        float Lf = wl->L[RF], Ln = wl->L[RN];
        printf("   légitimité : avec Temple = %.2f  vs  sans = %.2f\n", Lf, Ln);
        ok("un Temple bâti SOUTIENT la légitimité locale (L plus haute)", Lf > Ln + 0.5f);
    }

    /* ═══ 4. SAVOIR — la Bibliothèque accélère la recherche ═════════════ */
    printf("\n── 4. Le savoir : une Bibliothèque accélère la recherche ──\n");
    {
        float t_lib = tech_with_savoir(e, 1, 2.0f);   /* bibliothèque/monastère bâtis */
        float t_non = tech_with_savoir(e, 2, 0.0f);   /* sans savoir bâti */
        printf("   recherche en un tick : avec savoir bâti = %.3f  vs  sans = %.3f\n", t_lib, t_non);
        ok("une Bibliothèque ACCÉLÈRE la recherche (savoir bâti → +tech)", t_lib > t_non + 1e-4f);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(e); free(wl);
    return g_fail?1:0;
}
