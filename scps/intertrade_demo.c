/*
 * intertrade_demo.c — banc d'essai du COMMERCE INTER-PAYS (scps_intertrade)
 *
 *   make intertrade_demo && ./intertrade_demo [graine]
 *
 * Prouve : une grande route marchande porte des GOODS entre pays (le bien remonte
 * la pente de prix, l'exportateur encaisse l'or) ; la GUERRE l'EMBARGO (guerre
 * commerciale) ; l'intra-pays n'est pas du commerce inter-pays.
 */
#define _POSIX_C_SOURCE 200809L   /* V1 : setenv visible sous -std=c99 strict (portable) */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_species.h"
#include "scps_routes.h"
#include "scps_diplo.h"
#include "scps_intertrade.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

int main(int argc,char**argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    /* M4 — ce banc ISOLE le commerce de ROUTES (inter-pays). On coupe la passe d'arbitrage
     * des cités-états (ARB_VOL_CAP=0), qui sinon importerait du RÉSEAU vers nos Centres de
     * test (comportement légitime, mais hors sujet ici ; sa preuve est la chronique). */
    setenv("SCPS_TUNE","ARB_VOL_CAP=0",1);
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    if(!w||!econ){fprintf(stderr,"OOM\n");return 1;}

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" COMMERCE INTER-PAYS — les grandes routes marchandes portent des goods (graine %u)\n",seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,RACE_HUMAIN);
    /* P3.20 — la GÂCHE du réseau : sans Centre commercial, pas de commerce
     * inter-pays. On sème les hubs (géographiques) et l'on teste ENTRE hubs. */
    intertrade_reset();
    intertrade_seed_centres(w, econ);

    /* deux régions-HUB que l'on attribue à DEUX pays distincts. */
    int ra=-1, rb=-1;
    for(int r=0;r<econ->n_regions;r++) if(intertrade_has_centre(r)){ if(ra<0)ra=r; else {rb=r;break;} }
    if(ra<0||rb<0){ printf(" (monde trop vide)\n"); return 0; }
    int g=8;   /* un bien quelconque : on pose nous-mêmes stock & prix */

    DiploState dp; diplo_init(&dp);
    RouteNetwork rn; routes_init(&rn);
    rn.route[0].ra=ra; rn.route[0].rb=rb; rn.route[0].maritime=true; rn.route[0].open=true;
    rn.route[0].capacity=120.f; rn.n=1;

    /* A bon marché + surplus ; B cher + pénurie → le bien doit remonter A→B. */
    #define SETUP() do{ econ->region[ra].owner=0; econ->region[rb].owner=1; \
        econ->region[ra].stock[g]=500.f; econ->region[ra].price[g]=1.0f; econ->region[ra].treasury=0.f; \
        econ->region[rb].stock[g]=0.f;   econ->region[rb].price[g]=6.0f; }while(0)

    /* ---- 1. Une route inter-pays PORTE des goods ---- */
    printf("\n── 1. La grande route porte des goods (arbitrage + or à l'exportateur) ──\n");
    SETUP();
    intertrade_tick(econ,&rn,&dp);
    printf("   après le tick : B reçoit %.0f unités · A encaisse %.0f or · valeur échangée %.0f\n",
           econ->region[rb].stock[g], econ->region[ra].treasury, intertrade_imports_value(econ));
    ok("le bien REMONTE la pente de prix (l'importateur B reçoit)", econ->region[rb].stock[g] > 0.f);
    ok("l'EXPORTATEUR A encaisse de l'or", econ->region[ra].treasury > 0.f);
    ok("un échange a eu lieu (valeur > 0)", intertrade_imports_value(econ) > 0.f);
    ok("la route marchande est ACTIVE pour les deux pays",
       intertrade_active_routes(econ,&rn,&dp,0)==1 && intertrade_active_routes(econ,&rn,&dp,1)==1);

    /* ---- 2. EMBARGO : la guerre coupe le commerce ---- */
    printf("\n── 2. Embargo : la guerre suspend le commerce inter-pays ──\n");
    SETUP();
    diplo_declare_war(&dp,0,1);
    intertrade_tick(econ,&rn,&dp);
    ok("EMBARGO : aucun bien ne passe entre pays EN GUERRE", econ->region[rb].stock[g]==0.f);
    ok("aucune valeur échangée sous l'embargo", intertrade_imports_value(econ)==0.f);
    ok("aucune route marchande active en guerre", intertrade_active_routes(econ,&rn,&dp,0)==0);

    /* ---- 3. La paix rouvre la route ---- */
    printf("\n── 3. La paix rouvre la route ──\n");
    SETUP();
    diplo_make_peace(&dp,0,1);
    intertrade_tick(econ,&rn,&dp);
    ok("après la paix, le commerce REPREND (B reçoit de nouveau)", econ->region[rb].stock[g] > 0.f);

    /* ---- 4. Intra-pays ≠ commerce inter-pays ---- */
    printf("\n── 4. Une route intra-pays n'est pas du commerce inter-pays ──\n");
    SETUP();
    econ->region[rb].owner=0;   /* même couronne aux deux bouts */
    intertrade_tick(econ,&rn,&dp);
    ok("même couronne aux deux bouts → pas de route INTER-pays (rien ne passe ici)",
       econ->region[rb].stock[g]==0.f);

    /* ---- 5. ACHAT DIRECT AU MARCHÉ (l'actionneur de l'UI) : le joueur À 0 achète son
     *         bois AU PRIX, et UNIQUEMENT s'il est dispo ---- */
    printf("\n── 5. Achat direct au marché : un joueur à 0 pompe son bois (au prix, si dispo) ──\n");
    /* pr = une région NON-Centre voisine de ra (son hub sera ra, distance 1) ; on la donne
     * au joueur (pays 0) qui tient AUSSI le Centre ra → marché RÉGIONAL de proximité. */
    econ->region[ra].owner=0;
    int pr=-1;
    for (int r=0;r<econ->n_regions;r++)
        if (r!=ra && !intertrade_has_centre(r) && econ->adj[ra][r]){ pr=r; break; }
    if (pr<0){ printf("   (pas de voisin non-Centre pour ra — test sauté)\n"); }
    else {
        econ->region[pr].owner=0;
        econ->region[pr].price[RES_WOOD]=1.0f;
        econ->region[pr].stock[RES_WOOD]=0.f;          /* le joueur n'a RIEN */
        econ->region[pr].treasury=100000.f;
        econ->region[ra].stock[RES_WOOD]=500.f;        /* le marché (Centre ra) EN a */
        intertrade_tick(econ,&rn,&dp);                 /* (re)bâtit la carte + écrit pr.import_margin */
        ok("pr est bien rattaché au Centre ra (son marché régional)", intertrade_region_hub(pr)==ra);
        float marge=econ->region[pr].import_margin; if(marge<1.f)marge=1.f;
        float tres0=econ->region[pr].treasury, hub0=econ->region[ra].stock[RES_WOOD];
        long spent=0; long got=intertrade_market_buy(econ,pr,RES_WOOD,50,0,&spent);
        printf("   achat 50 bois : reçu %ld · payé %ld or · marge ×%.2f · prix attendu %ld\n",
               got, spent, marge, (long)(50*1.0f*marge+0.5f));
        ok("le joueur REÇOIT son bois (50)", got==50 && econ->region[pr].stock[RES_WOOD]==50.f);
        ok("il a PAYÉ au prix courant×marge (le pump du trésor)",
           spent==(long)(50*1.0f*marge+0.5f) && econ->region[pr].treasury < tres0);
        ok("le bien VIENT du marché (le Centre ra se DÉPLÉTÉ de 50)",
           econ->region[ra].stock[RES_WOOD]==hub0-50.f);
        /* UNIQUEMENT s'il est dispo : marché vidé → achat nul, trésor intact */
        econ->region[ra].stock[RES_WOOD]=0.f;
        float tres1=econ->region[pr].treasury;
        long got2=intertrade_market_buy(econ,pr,RES_WOOD,50,0,&spent);
        ok("marché VIDE → aucun achat (« uniquement s'il est dispo »), trésor intact",
           got2==0 && spent==0 && econ->region[pr].treasury==tres1);
    }

    /* ---- 6. V2 — ANTI-EXPLOIT : capitale = son PROPRE Centre (hub==region) ----
     *         une transaction same-region ne crée NI stock NI or (tue l'or infini). */
    printf("\n── 6. Anti-exploit : capitale = hub → transaction same-region NULLE ──\n");
    /* ra EST un Centre → g_hub_of[ra]==ra. On le possède, on le dote, on tente buy+sell. */
    econ->region[ra].owner=0;
    econ->region[ra].stock[RES_WOOD]=200.f;
    econ->region[ra].price[RES_WOOD]=2.f;
    econ->region[ra].treasury=5000.f;
    intertrade_tick(econ,&rn,&dp);                 /* (re)bâtit la carte : ra est son propre hub */
    float s0=econ->region[ra].stock[RES_WOOD], t0=econ->region[ra].treasury;
    long xp=0;
    long xb=intertrade_market_buy (econ,ra,RES_WOOD,50,0,&xp);   /* tier 0, hub==ra */
    long xs=intertrade_market_sell(econ,ra,RES_WOOD,50,0,&xp);   /* tier 0, hub==ra */
    ok("V2 : buy/sell same-region REFUSÉS (0) et NI stock NI trésor ne bougent (or infini TUÉ)",
       xb==0 && xs==0 && econ->region[ra].stock[RES_WOOD]==s0 && econ->region[ra].treasury==t0);

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ);
    return g_fail?1:0;
}
