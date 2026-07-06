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
#include "scps_heritage.h"
#include "scps_routes.h"
#include "scps_diplo.h"
#include "scps_intertrade.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }
static double wsum(const WorldEconomy *e){ double s=0.0; for(int r=0;r<e->n_regions;r++) s+=e->region[r].treasury; return s; }

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
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
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
        econ->region[rb].stock[g]=0.f;   econ->region[rb].price[g]=6.0f; econ->region[rb].treasury=100000.f; }while(0)
    /* CONSERVATION : Σ trésor du monde — invariant sur tout commerce/pump (zéro faucet, zéro sink). */
    /* ---- 1. Une route inter-pays PORTE des goods ---- */
    printf("\n── 1. La grande route porte des goods (arbitrage + or à l'exportateur) ──\n");
    SETUP();
    double w0=wsum(econ);
    intertrade_tick(econ,&rn,&dp);
    printf("   après le tick : B reçoit %.0f unités · A encaisse %.0f or · valeur échangée %.0f\n",
           econ->region[rb].stock[g], econ->region[ra].treasury, intertrade_imports_value(econ));
    ok("le bien REMONTE la pente de prix (l'importateur B reçoit)", econ->region[rb].stock[g] > 0.f);
    ok("l'EXPORTATEUR A encaisse de l'or", econ->region[ra].treasury > 0.f);
    ok("CONSERVATION : Σ trésor du monde INCHANGÉ par le commerce (zéro faucet/sink)", fabs(wsum(econ)-w0) < 1e-2);
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
        double wb0=wsum(econ);
        long spent=0; long got=intertrade_market_buy(econ,pr,RES_WOOD,50,0,&spent);
        printf("   achat 50 bois : reçu %ld · payé %ld or · marge ×%.2f · prix attendu %ld\n",
               got, spent, marge, (long)(50*1.0f*marge+0.5f));
        ok("le joueur REÇOIT son bois (50)", got==50 && econ->region[pr].stock[RES_WOOD]==50.f);
        ok("il a PAYÉ au prix courant×marge (le pump du trésor)",
           spent==(long)(50*1.0f*marge+0.5f) && econ->region[pr].treasury < tres0);
        ok("le bien VIENT du marché (le Centre ra se DÉPLÉTÉ de 50)",
           econ->region[ra].stock[RES_WOOD]==hub0-50.f);
        ok("CONSERVATION : l'achat ne crée ni ne détruit d'or (acheteur −cost == hub +cost)", fabs(wsum(econ)-wb0) < 1e-2);
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

    /* ---- 7. NON-RÉGRESSION : le TRIPTYQUE empire-aware (gate = devis = consume) ----
     *   La matière des AUTRES régions du même empire est mise en commun : le gate la VOIT,
     *   le devis ne la FACTURE PAS (gratuite, marge 0), la conso la PUISE. Les trois doivent
     *   s'accorder, sinon le joueur est sur/sous-facturé (le bug que ce banc verrouille). */
    printf("\n── 7. Empire-aware : la matière d'une région sœur est GRATUITE (gate=devis=consume) ──\n");
    {
        int X=-1, Y=-1;                                    /* X = chantier (vide), Y = sœur (riche), même empire 0 */
        for (int r=0;r<econ->n_regions;r++){ if (X<0) X=r; else { Y=r; break; } }
        econ->region[X].owner=0; econ->region[Y].owner=0;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==0) econ->region[r].stock[RES_STONE]=0.f;
        econ->region[Y].stock[RES_STONE]=300.f;            /* la SŒUR a la pierre ; X n'a RIEN */
        econ->region[X].price[RES_STONE]=2.f;
        float av = intertrade_market_avail(econ, X, RES_STONE);
        float ib=0.f; float gold = intertrade_buy_cost(econ, X, RES_STONE, 100.f, 2.f, &ib);
        ok("le GATE voit la matière de la sœur (avail ≥ besoin)", av >= 100.f-1e-3f);
        ok("le DEVIS est GRATUIT (matière d'empire = 0 or, NU d'import = 0)", gold < 1e-3f && ib < 1e-3f);
        float y0=econ->region[Y].stock[RES_STONE];
        intertrade_market_consume(econ, X, RES_STONE, 100.f, econ->region[X].price[RES_STONE]);
        ok("la CONSO puise la SŒUR Y (−100), X reste vide",
           fabsf(econ->region[Y].stock[RES_STONE]-(y0-100.f))<1e-2f && econ->region[X].stock[RES_STONE]<1e-3f);
    }

    /* ---- 8. NON-RÉGRESSION : le DÉFICIT importé est FACTURÉ (×marge), et le NU de l'import
     *   = la BASE DU PÉAGE. L'empire est gratuit → le nu de bâti n'est PAS la quantité totale
     *   mais la seule part importée ; (devis − NU) = la marge de transport routée à la cité-état. */
    printf("\n── 8. Déficit importé : devis = import×marge · NU de l'import = base du péage ──\n");
    if (pr>=0) {
        diplo_declare_war(&dp,0,1);                        /* guerre 0-1 : pas de trade au tick (stocks figés) */
        econ->region[pr].owner=0; econ->region[ra].owner=1; econ->region[rb].owner=1;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==0) econ->region[r].stock[RES_STONE]=0.f;
        for (int r=0;r<econ->n_regions;r++) if (intertrade_has_centre(r) && r!=ra) econ->region[r].stock[RES_STONE]=0.f;
        econ->region[ra].stock[RES_STONE]=200.f;           /* le SEUL Centre porteur — et il est ÉTRANGER */
        econ->region[pr].price[RES_STONE]=1.0f;
        intertrade_tick(econ,&rn,&dp);                     /* carte + cache (guerre ⇒ aucun trade ne bouge les stocks) */
        if (intertrade_region_hub(pr)==ra && econ->region[ra].owner!=econ->region[pr].owner){
            float marge=econ->region[pr].import_margin; if(marge<1.f)marge=1.f;
            float av=intertrade_market_avail(econ,pr,RES_STONE);
            float ib=0.f; float gold=intertrade_buy_cost(econ,pr,RES_STONE,100.f,1.0f,&ib);
            printf("   devis import 100 pierre : or %.1f · NU import %.1f · marge ×%.2f\n", gold, ib, marge);
            ok("le GATE voit l'import du Centre étranger (avail ≥ besoin)", av >= 100.f-1e-3f);
            ok("le NU de l'import = quantité importée × prix (100×1)", fabsf(ib-100.f) < 1e-2f);
            ok("le DEVIS facture l'import × marge (l'or paie le déficit étranger)", fabsf(gold-100.f*marge) < 1e-2f);
            ok("la BASE DU PÉAGE est positive (devis − NU = marge de transport > 0)", gold-ib > 1e-3f);
            float ra0=econ->region[ra].stock[RES_STONE];
            intertrade_market_consume(econ,pr,RES_STONE,100.f, econ->region[pr].price[RES_STONE]);
            ok("la CONSO importe bien du Centre étranger ra (−100)",
               fabsf(econ->region[ra].stock[RES_STONE]-(ra0-100.f))<1e-2f);
        } else {
            printf("   (topologie : pr non rattaché à un Centre étranger — test sauté)\n");
        }
    }

    /* ---- 9. LOT I — LE PRIX DU POOL RESPIRE : pool VIDE = cher, pool ABONDANT = bon marché */
    printf("\n── 9. Le prix du pool esclave respire avec sa profondeur ──\n");
    {
        WorldEconomy *e2=calloc(1,sizeof(WorldEconomy));
        e2->n_prov=1; e2->n_regions=1;
        e2->region_rep_prov[0]=0;
        ProvinceEconomy *pe=&e2->prov[0];
        pe->owner=0; pe->active=true; pe->colonized=true; pe->region=0;
        pe->treasury=1e9f;
        intertrade_reset();   /* pool à SEC (0 âme, toutes origines) */
        pe->pop.groups[0].klass=CLASS_SLAVE; pe->pop.groups[0].count=1000;
        pe->pop.groups[0].heritage=HERITAGE_CLANIQUE; pe->pop.groups[0].origin_sphere=SPHERE_ETRANGERS;
        pe->pop.groups[0].arrival=ARR_DEPORTE; pe->pop.groups[0].diaspora=true;
        pe->pop.n_groups=1;
        pe->strata[CLASS_SLAVE].pop=1000.f;
        float treas0=pe->treasury;
        long sold_empty=intertrade_slave_sell(e2, 0, 100);
        float gained_empty=(sold_empty>0)?(pe->treasury-treas0)/(float)sold_empty:0.f;
        ok("la VENTE au pool VIDE réussit (le pool démarre à sec)", sold_empty==100);

        /* on REMPLIT le pool loin au-dessus de SLAVE_POOL_REF (600) → surabondant. */
        pe->pop.groups[0].count=6000; pe->pop.n_groups=1; pe->strata[CLASS_SLAVE].pop=6000.f;
        intertrade_slave_sell(e2, 0, 6000);   /* déverse dans le pool : profondeur ≫ RÉFÉRENCE */
        pe->pop.groups[0].klass=CLASS_SLAVE; pe->pop.groups[0].count=100;
        pe->pop.groups[0].heritage=HERITAGE_CLANIQUE; pe->pop.groups[0].origin_sphere=SPHERE_ETRANGERS;
        pe->pop.groups[0].arrival=ARR_DEPORTE; pe->pop.groups[0].diaspora=true;
        pe->pop.n_groups=1; pe->strata[CLASS_SLAVE].pop=100.f;
        float treas1=pe->treasury;
        long sold_full=intertrade_slave_sell(e2, 0, 100);
        float gained_full=(sold_full>0)?(pe->treasury-treas1)/(float)sold_full:0.f;
        printf("   prix/âme : pool à SEC %.2f vs pool SURABONDANT %.2f\n", gained_empty, gained_full);
        ok("le pool VIDE (rare) VEND plus CHER que le pool SURABONDANT",
           sold_full==100 && gained_empty>gained_full);

        /* ACHAT : même sens — cher quand rare, moins cher quand abondant (marge Centres à part). */
        intertrade_reset();
        pe->treasury=1e9f;
        float buy_empty_cost=0.f;
        { long avail0=intertrade_slave_pool_count(); (void)avail0; }
        /* pool à sec ⇒ rien à acheter (avail=0) — on remplit d'abord un pool MODESTE (rare). */
        pe->pop.groups[0].klass=CLASS_SLAVE; pe->pop.groups[0].count=50;
        pe->pop.groups[0].heritage=HERITAGE_CLANIQUE; pe->pop.groups[0].origin_sphere=SPHERE_ETRANGERS;
        pe->pop.groups[0].arrival=ARR_DEPORTE; pe->pop.groups[0].diaspora=true;
        pe->pop.n_groups=1; pe->strata[CLASS_SLAVE].pop=50.f;
        intertrade_slave_sell(e2, 0, 50);   /* pool RARE (50 ≪ RÉFÉRENCE 600) */
        float treas2=pe->treasury;
        long bought_rare=intertrade_slave_buy(e2, 0, 10, true);
        buy_empty_cost=(bought_rare>0)?(treas2-pe->treasury)/(float)bought_rare:0.f;
        intertrade_reset();
        pe->treasury=1e9f;
        pe->pop.groups[0].klass=CLASS_SLAVE; pe->pop.groups[0].count=5000;
        pe->pop.groups[0].heritage=HERITAGE_CLANIQUE; pe->pop.groups[0].origin_sphere=SPHERE_ETRANGERS;
        pe->pop.groups[0].arrival=ARR_DEPORTE; pe->pop.groups[0].diaspora=true;
        pe->pop.n_groups=1; pe->strata[CLASS_SLAVE].pop=5000.f;
        intertrade_slave_sell(e2, 0, 5000);   /* pool SURABONDANT (≫ RÉFÉRENCE) */
        float treas3=pe->treasury;
        long bought_full=intertrade_slave_buy(e2, 0, 10, true);
        float buy_full_cost=(bought_full>0)?(treas3-pe->treasury)/(float)bought_full:0.f;
        printf("   coût/âme à l'achat : pool RARE %.2f vs pool SURABONDANT %.2f\n", buy_empty_cost, buy_full_cost);
        ok("l'ACHAT est plus CHER quand le pool est RARE qu'abondant",
           bought_rare>0 && bought_full>0 && buy_empty_cost>buy_full_cost);
        free(e2);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ);
    return g_fail?1:0;
}
