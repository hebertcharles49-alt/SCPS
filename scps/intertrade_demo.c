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

/* MONNAIE M14 — B9 : `setenv` n'existe PAS sous MinGW-w64 (msvcrt n'implémente pas
 * l'API POSIX, même avec _POSIX_C_SOURCE — un #define qui n'a jamais eu de prise ici :
 * ce banc ne BUILDAIT jamais sous Windows, motif du « 38/39 pré-existant » cité à
 * chaque vague). Shim portable : _putenv_s (CRT Windows, <stdlib.h>) sous _WIN32,
 * setenv POSIX partout ailleurs — comportement identique (variable posée, écrasée si
 * déjà présente). */
#ifdef _WIN32
static void scps_setenv(const char *name, const char *value){ _putenv_s(name, value); }
#else
static void scps_setenv(const char *name, const char *value){ setenv(name, value, 1); }
#endif

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }
/* B9 : LA MONNAIE totale du monde = trésor + richesse des 3 classes (le MÊME périmètre
 * que l'invariant M(t) ailleurs, ex. credit_demo.c « M avant/après ») — treasury SEUL
 * ignore le péage (TRADE_LEVY, M5-R1) qui verse la MOITIÉ de sa marge aux BOURGEOIS en
 * richesse, pas au trésor (item 5, scps_intertrade.c ~1010) : un échange RÉEL (zéro
 * faucet/sink) déplace bien treasury→wealth pour cette part, jamais treasury→treasury
 * seul — wsum() doit compter les DEUX pour mesurer une VRAIE conservation. */
static double wsum(const WorldEconomy *e){
    double s=0.0;
    for(int r=0;r<e->n_regions;r++){
        s += e->region[r].treasury;
        for (int c=0;c<CLASS_COUNT;c++) s += e->region[r].strata[c].wealth;
    }
    return s;
}
/* MONNAIE M14 — B9 : intertrade_tick/intertrade_market_* résolvent stock/prix/trésor au
 * grain PROVINCE (it_treasury, econ_region_stock_add — motif RE-KEY PROVINCE) ; ce banc,
 * jamais compilé sous Windows avant B9 (motif setenv), posait sa fixture UNIQUEMENT sur
 * region[] (la VUE) — invisible aux mutations RÉELLES (le GATE lit region[].stock, mais
 * le DÉBIT mord prov[], jamais seedé ⇒ moved≈0 ⇒ AUCUN échange, jamais démasqué faute de
 * build). `mirror_prov` pousse la fixture posée sur region[] vers la province
 * représentative (owner reste sur region[] SEUL, lu directement par intertrade_tick pour
 * l'appartenance) ; `econ_aggregate_regions` après CHAQUE tick tire les mutations RÉELLES
 * (prov[]) de retour vers region[] pour que les assertions (qui lisent region[]) voient
 * l'état RÉEL post-échange. */
static void mirror_prov(WorldEconomy *e, int region){
    if (region<0 || region>=e->n_regions) return;
    RegionEconomy *rv=&e->region[region];
    int rep = econ_region_rep_province(e, region);
    int n = e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    /* B9 : econ_aggregate_regions ÉLIT region[].owner depuis prov[] (capitale, sinon la
     * province la plus peuplée — scps_econ.c econ_aggregate_regions) et SOMME stock/
     * treasury sur TOUTES les provinces membres — un simple poke de la représentative
     * laisse les SŒURS polluer la Σ (et l'élection d'owner ignorer notre poke). On aligne
     * TOUTES les provinces de la région : owner partout, stock/trésor/prix sur la
     * REPRÉSENTATIVE seule, sœurs à ZÉRO (stock/trésor — jamais leur owner, motif ci-haut). */
    bool put=false;
    for (int p=0;p<n;p++){
        ProvinceEconomy *pv=&e->prov[p];
        if (pv->region!=region) continue;
        pv->owner = rv->owner;
        if (!put && (p==rep || rep<0)){
            for (int g=0; g<RES_COUNT; g++){ pv->stock[g]=rv->stock[g]; pv->price[g]=rv->price[g]; }
            pv->treasury = rv->treasury;
            put=true;
        } else {
            for (int g=0; g<RES_COUNT; g++) pv->stock[g]=0.f;
            pv->treasury = 0.f;
        }
    }
}

int main(int argc,char**argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    /* M4 — ce banc ISOLE le commerce de ROUTES (inter-pays). On coupe la passe d'arbitrage
     * des cités-états (ARB_VOL_CAP=0), qui sinon importerait du RÉSEAU vers nos Centres de
     * test (comportement légitime, mais hors sujet ici ; sa preuve est la chronique). */
    scps_setenv("SCPS_TUNE","ARB_VOL_CAP=0");
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    if(!w||!econ){fprintf(stderr,"OOM\n");return 1;}

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" COMMERCE INTER-PAYS — les grandes routes marchandes portent des goods (graine %u)\n",seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    /* B9 : region[] n'a JAMAIS été agrégé depuis prov[] à ce point (aucun econ_tick n'a
     * encore tourné) — wsum() (Σ region[].treasury, la conservation testée plus bas)
     * lirait sinon un état PÉRIMÉ/incohérent avec prov[] (la vérité, seedée par gen_
     * population) avant le 1er tick, faussant le AVANT/APRÈS de la conservation. */
    econ_aggregate_regions(econ);
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

    /* A bon marché + surplus ; B cher + pénurie → le bien doit remonter A→B.
     * B9 : mirror_prov pousse la fixture vers la province représentative (RE-KEY). */
    #define SETUP() do{ econ->region[ra].owner=0; econ->region[rb].owner=1; \
        econ->region[ra].stock[g]=500.f; econ->region[ra].price[g]=1.0f; econ->region[ra].treasury=0.f; \
        econ->region[rb].stock[g]=0.f;   econ->region[rb].price[g]=6.0f; econ->region[rb].treasury=100000.f; \
        mirror_prov(econ,ra); mirror_prov(econ,rb); }while(0)
    /* CONSERVATION : Σ monnaie du monde (wsum = trésor+richesse, B9 — le péage TRADE_LEVY
     * verse la moitié de sa marge aux BOURGEOIS en richesse, pas au trésor, item 5/M5-R1) —
     * invariant sur tout commerce/pump (zéro faucet, zéro sink). */
    /* ---- 1. Une route inter-pays PORTE des goods ---- */
    printf("\n── 1. La grande route porte des goods (arbitrage + or à l'exportateur) ──\n");
    SETUP();
    double w0=wsum(econ);
    intertrade_tick(econ,&rn,&dp);
    econ_aggregate_regions(econ);   /* B9 : tire les mutations RÉELLES (prov[]) vers region[] (lecture) */
    printf("   après le tick : B reçoit %.0f unités · A encaisse %.0f or · valeur échangée %.0f\n",
           econ->region[rb].stock[g], econ->region[ra].treasury, intertrade_imports_value(econ));
    ok("le bien REMONTE la pente de prix (l'importateur B reçoit)", econ->region[rb].stock[g] > 0.f);
    ok("l'EXPORTATEUR A encaisse de l'or", econ->region[ra].treasury > 0.f);
    ok("CONSERVATION : Σ monnaie du monde (trésor+richesse) INCHANGÉE par le commerce (zéro faucet/sink)", fabs(wsum(econ)-w0) < 1e-2);
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
    econ_aggregate_regions(econ);   /* B9 */
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
        mirror_prov(econ,pr); mirror_prov(econ,ra);     /* B9 */
        intertrade_tick(econ,&rn,&dp);                 /* (re)bâtit la carte + écrit pr.import_margin */
        econ_aggregate_regions(econ);                   /* B9 */
        ok("pr est bien rattaché au Centre ra (son marché régional)", intertrade_region_hub(pr)==ra);
        float marge=econ->region[pr].import_margin; if(marge<1.f)marge=1.f;
        float tres0=econ->region[pr].treasury, hub0=econ->region[ra].stock[RES_WOOD];
        double wb0=wsum(econ);
        long spent=0; long got=intertrade_market_buy(econ,pr,RES_WOOD,50,0,&spent);
        econ_aggregate_regions(econ);                   /* B9 : market_buy mord prov[] */
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
        mirror_prov(econ,ra);                           /* B9 */
        float tres1=econ->region[pr].treasury;
        long got2=intertrade_market_buy(econ,pr,RES_WOOD,50,0,&spent);
        econ_aggregate_regions(econ);                   /* B9 */
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
    mirror_prov(econ,ra);                           /* B9 : la fixture doit aussi VIVRE au grain province */
    intertrade_tick(econ,&rn,&dp);                 /* (re)bâtit la carte : ra est son propre hub */
    econ_aggregate_regions(econ);
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
        /* B9 : ZÉRO toutes les provinces de l'empire 0 (pas seulement region[]) — sinon un
         * reliquat de genèse dans une AUTRE province owner==0 fausserait la conso empire-aware
         * (elle puiserait ailleurs que Y, cassant l'égalité stricte Y==y0-100 testée plus bas). */
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==0){ econ->region[r].stock[RES_STONE]=0.f; mirror_prov(econ,r); }
        econ->region[Y].stock[RES_STONE]=300.f;            /* la SŒUR a la pierre ; X n'a RIEN */
        econ->region[X].price[RES_STONE]=2.f;
        mirror_prov(econ,X); mirror_prov(econ,Y);           /* B9 : la conso mord prov[], pas region[] */
        float av = intertrade_market_avail(econ, X, RES_STONE);
        float ib=0.f; float gold = intertrade_buy_cost(econ, X, RES_STONE, 100.f, 2.f, &ib);
        ok("le GATE voit la matière de la sœur (avail ≥ besoin)", av >= 100.f-1e-3f);
        ok("le DEVIS est GRATUIT (matière d'empire = 0 or, NU d'import = 0)", gold < 1e-3f && ib < 1e-3f);
        float y0=econ->region[Y].stock[RES_STONE];
        intertrade_market_consume(econ, X, RES_STONE, 100.f, econ->region[X].price[RES_STONE]);
        econ_aggregate_regions(econ);   /* B9 : la conso a mordu prov[] — tirer l'état RÉEL vers region[] */
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
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==0){ econ->region[r].stock[RES_STONE]=0.f; mirror_prov(econ,r); }
        for (int r=0;r<econ->n_regions;r++) if (intertrade_has_centre(r) && r!=ra){ econ->region[r].stock[RES_STONE]=0.f; mirror_prov(econ,r); }
        econ->region[ra].stock[RES_STONE]=200.f;           /* le SEUL Centre porteur — et il est ÉTRANGER */
        econ->region[pr].price[RES_STONE]=1.0f;
        mirror_prov(econ,ra); mirror_prov(econ,rb); mirror_prov(econ,pr);   /* B9 */
        intertrade_tick(econ,&rn,&dp);                     /* carte + cache (guerre ⇒ aucun trade ne bouge les stocks) */
        econ_aggregate_regions(econ);                       /* B9 */
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
            econ_aggregate_regions(econ);                    /* B9 */
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
        /* CONSERVATION (2026-07-21) : le marché servile a désormais un PAYEUR RÉEL (le
         * Centre, sinon les classes du marché régional — SLAVE_MARKET_CONSERVED). Cette
         * fixture sans Centre dote donc ses classes : sans elles le vendeur encaisse 0
         * (comportement conservé CORRECT) et la respiration du prix devient invisible
         * au trésor que le banc mesure. */
        pe->strata[CLASS_LABORER].wealth=1e7f;
        pe->strata[CLASS_BOURGEOIS].wealth=1e7f;
        pe->strata[CLASS_ELITE].wealth=1e7f;
        econ_aggregate_regions(e2);
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
        long bought_rare=intertrade_slave_buy(e2, 0, 10, true, -1);
        buy_empty_cost=(bought_rare>0)?(treas2-pe->treasury)/(float)bought_rare:0.f;
        intertrade_reset();
        pe->treasury=1e9f;
        pe->pop.groups[0].klass=CLASS_SLAVE; pe->pop.groups[0].count=5000;
        pe->pop.groups[0].heritage=HERITAGE_CLANIQUE; pe->pop.groups[0].origin_sphere=SPHERE_ETRANGERS;
        pe->pop.groups[0].arrival=ARR_DEPORTE; pe->pop.groups[0].diaspora=true;
        pe->pop.n_groups=1; pe->strata[CLASS_SLAVE].pop=5000.f;
        intertrade_slave_sell(e2, 0, 5000);   /* pool SURABONDANT (≫ RÉFÉRENCE) */
        float treas3=pe->treasury;
        long bought_full=intertrade_slave_buy(e2, 0, 10, true, -1);
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
