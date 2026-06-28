/*
 * cap_demo.c — banc LIMITEUR DE PRODUCTION (§11.4, econ_set_prod_cap).
 *
 *   make cap_demo && ./cap_demo [graine]
 *
 * Une région-brasserie (grain → bière). On prouve : le défaut du cap est ∞ (-1,
 * posé par econ_init — PAS « produire zéro ») ; sans cap la bière s'accumule ;
 * avec un cap, le STOCK plafonne (bien sous le cas non-bridé) et l'INTRANT (grain)
 * est LIBÉRÉ (plus brassé au plafond) ; save/load préserve le cap.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

/* On veut un STOCK de bière qui MONTE vers un plateau franc, pour y voir mordre
 * le cap. Deux obstacles du moteur, neutralisés à dessein (banc = on tient le
 * marché pour isoler le limiteur) :
 *   · la SOIF (un bien de marché consommé ne s'accumule pas) → culture AGRAIRE
 *     (subsistance ≥ 5 ⇒ boisson préférée = EAU-DE-VIE, pré-rempli) : la pop boit le eau-de-vie,
 *     la bière reste INTACTE ⇒ seul puits = la décrue ×0.85.
 *   · l'ANTI-GLUT (offre > demande ⇒ prix s'effondre ⇒ effort chute) → on ÉPINGLE
 *     le prix bière (tick_pin, avant chaque econ_tick) ⇒ effort de marché tenu
 *     au plafond (1.5×). Niveau 8 ⇒ cadence ~12/tick ⇒ plateau-décrue ~68, bien
 *     au-dessus du cap de 40 testé. */
#define BEER_PIN 6.0f   /* prix bière épinglé (> base 3 ⇒ market_effort saturé) */
static void brew_region(WorldEconomy *e){
    RegionEconomy *re=&e->region[0];
    re->active=true; re->colonized=true; re->culture.settled=true; re->owner=0;
    re->culture.subsistance=10.f;                 /* agraire ⇒ boit le EAU-DE-VIE, pas la bière */
    for (int k=0;k<RES_COUNT;k++){ re->raw_cap[k]=0.f; re->stock[k]=0.f; re->price[k]=1.0f; }
    re->raw_cap[RES_GRAIN]=60.f;                  /* grain ample pour une cadence haute */
    re->n_entrepot=20;                            /* gros plafond de stock (≈10k) : le grain n'est pas bridé à ~200 */
    re->stock[RES_GRAIN]=5000.f;                  /* SURPLUS vivrier large : la réserve (∝ pop) ne borne pas le brassage — le banc teste le CAP, pas la nourriture */
    re->stock[RES_EAU_DE_VIE]=1e5f;                     /* soif comblée au EAU-DE-VIE ⇒ bière intacte */
    re->n_bld=0; re->bld[0].type=BLD_BREWERY; re->bld[0].level=8.f; re->n_bld=1;
    /* Bassin AMPLE : ce banc teste le PLAFOND DE PRODUCTION, pas la main-d'œuvre. La refonte
     * labor-bound (brasserie labor=27, gourmande en bras) rendait une petite fixture labor-limitée
     * (la brasserie ne se staffait plus à plein) → on dote large pour que le CAP soit le seul goulot. */
    re->strata[CLASS_LABORER].pop=3000.f;  re->strata[CLASS_LABORER].wealth=1e5f;
    re->strata[CLASS_BOURGEOIS].pop=300.f; re->strata[CLASS_BOURGEOIS].wealth=1e5f;
    re->strata[CLASS_ELITE].pop=50.f;      re->strata[CLASS_ELITE].wealth=1e5f;
}
/* un tick en tenant le prix bière épinglé (effort de marché constant). */
static void tick_pin(WorldEconomy *e){ e->region[0].price[RES_BEER]=BEER_PIN; econ_tick(e,1.f); }

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):9u;
    World *w=malloc(sizeof(World)); WorldEconomy *e=malloc(sizeof(WorldEconomy));
    if(!w||!e){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══ LIMITEUR DE PRODUCTION — le cap joueur plafonne le stock ══\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p); econ_init(e,w); gen_population(w,e);   /* econ_init pose g_prod_cap = -1 */
    if (e->n_regions<2){ fprintf(stderr,"monde trop petit\n"); return 1; }
    for (int r=1;r<e->n_regions;r++) e->region[r].active=false;  /* isolation : seule la région 0 tourne */

    /* — 1. défaut = ∞ (econ_init a posé -1, PAS 0) — */
    printf("\n── 1. Le défaut du cap ──\n");
    ok("après econ_init, le cap par défaut est ∞ (-1, pas « produire zéro »)", econ_prod_cap(0,RES_BEER)<0.f);

    /* — 2. SANS cap : la bière s'accumule vers son plateau-décrue — */
    printf("\n── 2. Sans cap : la brasserie produit ──\n");
    brew_region(e);
    for (int t=0;t<24;t++) tick_pin(e);
    float beer_unc=e->region[0].stock[RES_BEER];
    float grain_unc=e->region[0].stock[RES_GRAIN];
    ok("sans cap : la brasserie PRODUIT (bière > 50)", beer_unc>50.f);

    /* — 3. AVEC cap : le stock plafonne bien sous le cas non-bridé — */
    printf("\n── 3. Avec cap = 40 : le stock plafonne ──\n");
    brew_region(e);
    econ_set_prod_cap(0,RES_BEER,40.f);
    for (int t=0;t<24;t++) tick_pin(e);
    float beer_cap=e->region[0].stock[RES_BEER];
    float grain_cap=e->region[0].stock[RES_GRAIN];
    printf("   bière : sans cap %.0f · cap=40 → %.0f | grain : %.0f vs %.0f\n",
           beer_unc, beer_cap, grain_unc, grain_cap);
    ok("avec cap : la bière PLAFONNE (≤ cap + marge d'un tick, « bien sous le non-bridé »)",
       beer_cap < beer_unc && beer_cap < 80.f);
    ok("au plafond, l'INTRANT grain est LIBÉRÉ (moins brassé → stock grain ≥ cas non-bridé)",
       grain_cap >= grain_unc - 1e-2f);

    /* — 4. save/load préserve le cap — */
    printf("\n── 4. Save/load ──\n");
    econ_set_prod_cap(0,RES_BEER,123.f);
    FILE *tmp=tmpfile();
    bool sv = tmp && (econ_prodcap_save(tmp),1);
    econ_set_prod_cap(0,RES_BEER,-1.f);                  /* on efface en vif */
    if (tmp) rewind(tmp);
    bool ld = tmp && econ_prodcap_load(tmp);
    if (tmp) fclose(tmp);
    ok("save/load round-trip préserve le cap (123)", sv && ld && fabsf(econ_prod_cap(0,RES_BEER)-123.f)<1e-3f);

    printf("\n══ BILAN : %d réussis, %d échoués ══\n", g_pass, g_fail);
    free(w); free(e);
    return g_fail?1:0;
}
