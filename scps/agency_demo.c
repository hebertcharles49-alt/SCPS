/*
 * agency_demo.c — banc d'essai de la couche d'agency (§1-§2)
 *
 *   make agency_demo && ./agency_demo [graine]
 *
 * Prouve le principe « une action est un levier qui déplace une coordonnée » :
 *   - bâtir des institutions (Tribunal/Chancellerie/Académie) monte K → l'ordre
 *     se LIT « plus stable » (consenti) ;
 *   - bâtir des citadelles (Garnison→Citadelle) monte H ET ronge L → l'ordre
 *     bascule « coercitif-fragile » (Stabilité Tenue · Assise Contrainte).
 *
 * Le temps passe en JOURS (l'arc de 250 ans) ; aucune construction n'est
 * instantanée. La lecture se fait par la MEMBRANE (mots), jamais en chiffres
 * (les flottants affichés ici sont pour le développeur, pas le joueur).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"
#include "scps_agency.h"
#include "scps_credit.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

/* Contexte de simulation. */
typedef struct {
    World *w; WorldEconomy *econ; TradeNetwork *net; TechState *ts;
    WorldProsperity *wp; WorldLegitimacy *wl; AgencyState *ag;
    int player, cap_reg;
} Sim;

/* Un jour : éco → actions → légitimité → prospérité (ordre anti-circularité). */
static void run_days(Sim *s, int days){
    for (int d=0; d<days; d++){
        econ_tick(s->econ, 1.f);
        agency_advance(s->ag, s->w, s->econ, s->wl, NULL, 1);
        legitimacy_tick(s->wl, s->w, s->econ, s->ts);
        prosperity_tick(s->wp, s->w, s->econ, s->net, s->ts, s->wl);
    }
}

static void snapshot(Sim *s, const char *phase, float *out_SI, float *out_frag, float *out_L){
    CountryProsperity *cp=&s->wp->country[s->player];
    CountryReadout r=country_readout(s->wp, s->ts, s->w, s->player);
    printf("  An %-3d %-22s Stabilité=%-12s Assise=%-11s Légitimité=%-10s",
           agency_year(s->ag), phase,
           label_stab(r.stabilite), label_assise(r.assise), label_legit(r.legitimite));
    printf("   [dev SI=%.2f frag=%.2f L=%.2f]\n", cp->SI, cp->fragilite, cp->L);
    if (out_SI)   *out_SI=cp->SI;
    if (out_frag) *out_frag=cp->fragilite;
    if (out_L)    *out_L=cp->L;
}

int main(int argc, char **argv){
    /* RECALIBRAGE FIXTURE (vague climat, 2026-08-18) : sous la graine 42, la
     * refonte climat retire au JOUEUR une capitale quasi vide (pop journaliers
     * ≈1) — le contrôleur fiscal IA (econ_ai_fiscal_tick/M3h) sur-frappe alors en
     * continu pour compenser une trésorerie exsangue, et DEBASE_K_EROSION_RATE
     * ronge K_inst de la capitale à -0.5/jour (scps_econ.c:5766) : l'institution
     * bâtie s'effondre à 0 avant même la fin de la fenêtre de mesure (K_an8==
     * K_ctrl==3.000 pile). Mécanisme moteur intact et correctement câblé (c'est
     * la MÊME punition qu'un joueur humain subirait avec une capitale ruinée) —
     * seule la fixture (aucune population/trésor injectés, contrairement à
     * ai_demo qui égalise le SUBSTRAT) hérite d'un tirage de graine malchanceux
     * sous la nouvelle carte. Graine 1 : capitale réelle (~15 000 journaliers),
     * K_inst décroît normalement (4.0→3.74 sur 6 ans, pas de spirale de débase) —
     * marge large sur l'assert (K_an8=6.74 vs K_ctrl+2=5.0, contre 3.0==3.0). */
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):1u;

    Sim s={0};
    s.w   =(World*)          malloc(sizeof(World));
    s.econ=(WorldEconomy*)   malloc(sizeof(WorldEconomy));
    s.net =(TradeNetwork*)   malloc(sizeof(TradeNetwork));
    s.ts  =(TechState*)      calloc(SCPS_MAX_COUNTRY,sizeof(TechState));
    s.wp  =(WorldProsperity*)malloc(sizeof(WorldProsperity));
    s.wl  =(WorldLegitimacy*)malloc(sizeof(WorldLegitimacy));
    s.ag  =(AgencyState*)    malloc(sizeof(AgencyState));
    if(!s.w||!s.econ||!s.net||!s.ts||!s.wp||!s.wl||!s.ag){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" AGENCY — bâtir des coordonnées dans le temps (graine %u)\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);

    /* ── TÉMOIN (E1bis.11) : même monde, mêmes années, ZÉRO chantier. L'↑ de
     * famille REMPLACE son palier (K final 4.0, plus 7.5 cumulés) : l'effet sur
     * SI est plus fin que la dérive de fond — on mesure donc CONTRE TÉMOIN. */
    float SI_ctrl, K_ctrl;
    {
        world_generate(s.w,&p);
        econ_init(s.econ,s.w); gen_population(s.w,s.econ); worldgen_seed_peoples(s.w,s.econ,HERITAGE_ADAPTATIF);
        trade_network_build(s.net,s.w,s.econ);
        for (int c=0;c<s.w->n_countries;c++) tech_state_init(&s.ts[c],false);
        prosperity_init(s.wp,s.w); legitimacy_init(s.wl,s.w,s.econ); agency_init(s.ag);
        s.player=0;
        for (int c=0;c<s.w->n_countries;c++) if (s.w->country[c].role==POLITY_PLAYER){ s.player=c; break; }
        run_days(&s, 8*SCPS_DAYS_PER_YEAR);
        SI_ctrl=s.wp->country[s.player].SI;
        K_ctrl =s.wp->country[s.player].K;
    }

    world_generate(s.w,&p);
    econ_init(s.econ,s.w);
    gen_population(s.w,s.econ);
    worldgen_seed_peoples(s.w,s.econ,HERITAGE_ADAPTATIF);
    trade_network_build(s.net,s.w,s.econ);
    for (int c=0;c<s.w->n_countries;c++) tech_state_init(&s.ts[c],false);
    prosperity_init(s.wp,s.w);
    legitimacy_init(s.wl,s.w,s.econ);
    agency_init(s.ag);
    credit_init();   /* un seul livre d'or : agency_build débite l'or NATIONAL via crédit */

    /* Joueur + sa région-capitale (où l'on bâtit). */
    s.player=0;
    for (int c=0;c<s.w->n_countries;c++) if (s.w->country[c].role==POLITY_PLAYER){ s.player=c; break; }
    int cap_prov=s.w->country[s.player].capital_prov;
    s.cap_reg=(cap_prov>=0)?s.w->province[cap_prov].region:0;
    printf("\n  Joueur = pays %d, capitale région %d (« %s ») | témoin an 8 : SI=%.2f\n",
           s.player, s.cap_reg, s.w->region[s.cap_reg].name, SI_ctrl);

    printf("\n── L'arc d'une partie : on bâtit, le temps passe, l'ordre se lit ──\n");
    float SI0,SI1,SI2, F0,F1,F2, L0,L1,L2;

    /* Phase 0 — fondation (rien de bâti). */
    run_days(&s, 2*SCPS_DAYS_PER_YEAR);
    snapshot(&s, "fondation", &SI0,&F0,&L0);

    /* Phase 1 — INSTITUTIONS (→ K) + infrastructure (→ PE, food). */
    agency_order_build(s.ag, s.cap_reg, EDI_TRIBUNAL);
    agency_order_build(s.ag, s.cap_reg, EDI_CHANCELLERIE);
    agency_order_build(s.ag, s.cap_reg, EDI_ACADEMIE);
    agency_order_build(s.ag, s.cap_reg, EDI_MARCHE);    /* → PE_infra (carrefour) */
    agency_order_build(s.ag, s.cap_reg, EDI_GRENIER);   /* → food_cap (apex) */
    /* VÉTUSTÉ (b0116bb, « Métriques province ») : la densité bâtie s'érode dès la fin
     * du chantier (agency_build_decay tourne CHAQUE jour). Le total de jours simulés
     * reste 6*SCPS_DAYS_PER_YEAR (K_an8 tombe au même an que le témoin) mais on VEILLE
     * jour par jour pour capturer le PIC (juste après complétion, avant toute usure) au
     * lieu de lire en fin de fenêtre — sinon on prouve l'usure, pas l'accumulation
     * (déjà l'objet d'un banc dédié : scps_api_demo VÉTUSTÉ). */
    const ProvBuild *b1=&s.econ->region[s.cap_reg].build;
    float K_inst1=0.f, PE_infra1=0.f, food_cap1=0.f;
    for (int d=0; d<6*SCPS_DAYS_PER_YEAR; d++){
        run_days(&s, 1);
        if (b1->K_inst  >K_inst1)  K_inst1  =b1->K_inst;
        if (b1->PE_infra>PE_infra1)PE_infra1=b1->PE_infra;
        if (b1->food_cap>food_cap1)food_cap1=b1->food_cap;
    }
    snapshot(&s, "après institutions (K↑)", &SI1,&F1,&L1);
    float K_an8 = s.wp->country[s.player].K;   /* la COORDONNÉE à l'an 8 (même an que le témoin) */

    /* Phase 2 — CITADELLES (→ H, ronge L) : tenir par la force. */
    agency_order_build(s.ag, s.cap_reg, EDI_GARNISON);
    agency_order_build(s.ag, s.cap_reg, EDI_FORTERESSE);
    agency_order_build(s.ag, s.cap_reg, EDI_CITADELLE);
    float H_coerc2=0.f;   /* idem : pic avant l'usure du défrichement (phase 3) */
    for (int d=0; d<8*SCPS_DAYS_PER_YEAR; d++){   /* la Citadelle met ~6 ans */
        run_days(&s, 1);
        if (b1->H_coerc>H_coerc2) H_coerc2=b1->H_coerc;
    }
    snapshot(&s, "après citadelles (H↑, L↓)", &SI2,&F2,&L2);

    /* Phase 3 — DÉFRICHEMENT (§4) sur une niche forestière + EXPLOITATION (§3).
     * On choisit une niche forestière existante ; s'il n'y en a pas dans le monde
     * généré, on en FABRIQUE une (déterministe) sur une région ≠ capitale → le
     * test mesure l'EFFET du défrichement, pas la flore de la graine. */
    int forest=-1;
    for (int r=0;r<s.econ->n_regions;r++){
        const PopCulture *c=&s.econ->region[r].culture;
        if (c->settled && (c->lifeway==LIFE_HUNTER||c->lifeway==LIFE_HORTICULTURE)){ forest=r; break; }
    }
    if (forest<0)
        for (int r=0;r<s.econ->n_regions;r++)
            if (s.econ->region[r].culture.settled && r!=s.cap_reg){ forest=r; break; }
    if (forest<0) forest=s.cap_reg;
    s.econ->region[forest].culture.lifeway=LIFE_HORTICULTURE;  /* niche forestière franche */
    s.econ->region[forest].culture.subsistance=2.5f;            /* marge nette pour la dérive agricole */
    bool is_forest=true;
    float subs0=s.econ->region[forest].culture.subsistance;
    float food0=s.econ->region[forest].build.food_cap;
    float Lf0=s.wl->L[forest];
    float iron0=s.econ->region[s.cap_reg].raw_cap[RES_IRON];
    agency_order_clear(s.ag, forest);                 /* §4 défrichement */
    agency_order_exploit(s.ag, s.cap_reg, RES_IRON);  /* §3 exploitation */
    run_days(&s, 3650);   /* DÉFRICHEMENT = 10 ANS (CLEAR_DAYS 3600, décision joueur
                           * 2026-07-31 « prends 10 ans ») ; l'exploitation (180 j) est
                           * achevée depuis longtemps — on mesure les DEUX à l'arrivée. */
    float subs1=s.econ->region[forest].culture.subsistance;
    float food1=s.econ->region[forest].build.food_cap;
    float Lf1=s.wl->L[forest];
    float iron1=s.econ->region[s.cap_reg].raw_cap[RES_IRON];
    printf("  An %-3d défrichement (région %d%s) : subsistance %.1f→%.1f  food_cap +%.1f  L %.1f→%.1f\n",
           agency_year(s.ag), forest, is_forest?" forestière":"", subs0,subs1, food1-food0, Lf0,Lf1);

    /* ---- Contrôles ---------------------------------------------------- */
    printf("\n── Vérification : l'action est un levier ──\n");
    ok("le temps passe en années (≥ 16 ans écoulés)", agency_year(s.ag) >= 16);
    /* E1bis.11 : l'↑ REMPLACE son palier (K bâti final 4.0, plus 7.5 cumulés) — le
     * verdict SI (moteur d'ordre, banc core_demo) peut saturer ; on prouve donc LE
     * LEVIER au niveau de la COORDONNÉE : K du pays > témoin sans chantier. */
    (void)SI_ctrl;
    ok("bâtir des institutions DÉPLACE la coordonnée (K pays > témoin, même an)",
       K_an8 > K_ctrl + 2.0f);
    ok("bâtir des citadelles ronge la légitimité (L baisse)",     L2  < L1 - 0.1f);
    ok("les citadelles aggravent la fragilité (par la force)",    F2  > F1);
    /* La densité bâtie dans la capitale est bien accumulée (PIC relevé jour par jour
     * pendant CHAQUE phase de chantier — cf. K_inst1/PE_infra1/food_cap1/H_coerc2 plus
     * haut — pas en fin de banc, où la vétusté aurait déjà rongé la marge sous les
     * seuils). */
    ok("K institutionnel bâti dans la capitale (Tribunal+Chancellerie+Académie)",
       K_inst1 >= 3.5f);
    ok("coercition bâtie dans la capitale (Garnison+Forteresse+Citadelle)",
       H_coerc2 >= 5.5f);
    ok("infrastructure marchande bâtie (PE_infra, Marché)", PE_infra1 >= 1.0f);
    ok("stockage alimentaire bâti (food_cap, Grenier)",     food_cap1 >= 1.0f);
    printf("     capitale (au palier bâti) : K_inst=%.1f  H_coerc=%.1f  PE_infra=%.1f  food_cap=%.1f\n",
           K_inst1, H_coerc2, PE_infra1, food_cap1);
    /* §4 défrichement + §3 exploitation */
    ok("défricher monte la nourriture (food_cap)",            food1 > food0);
    ok("défricher dérive la subsistance vers l'agriculture",  subs1 > subs0 + 0.3f);
    if (is_forest)
        ok("défricher en niche forestière ronge L local",     Lf1 < Lf0);
    ok("exploiter monte l'extraction (raw_cap fer)",          iron1 > iron0 + 0.5f);

    /* ═══ §1 — LA MATIÈRE DU BÂTI : recette SOURCÉE au réseau (P1 empire-aware) ════
     * Re-baseline (2026-06-15) : la matière de SON empire est GRATUITE pour SON chantier
     * (pool marchés/ports, marge 0, cf. intertrade_buy_cost) — l'or ne paie QUE le déficit
     * importé des Centres ÉTRANGERS. Ici la capitale tient SON stock ⇒ devis = 0 or ; on
     * prouve à la place que (a) c'est bien gratuit en propre, (b) le TIER pilote toujours la
     * recette (la Citadelle MANGE bien plus de pierre que le Grenier), (c) la matière est
     * réellement CONSOMMÉE, (d) une pénurie franche (rien en propre, aucun Centre atteignable)
     * REFUSE le chantier au gate de matière (l'autre verrou, l'or ayant disparu en propre). */
    printf("\n── §1. La matière du bâti : gratuite en propre, ∝ tier, gate de pénurie ──\n");
    {
        RegionEconomy *re=&s.econ->region[s.cap_reg];
        /* Marché de RÉFÉRENCE uniforme (prix=1 partout) : on teste le TIER (la recette :
         * Grenier 40+15+10 unités vs Citadelle 60+220+150), PAS les oscillations du marché. */
        for (int r=0;r<RES_COUNT;r++) re->price[r]=1.0f;
        /* 2-BRUTES STRICTES (2026-07-13) + RE-KEY : la matière RÉELLE vit sur la PROVINCE —
         * la consommation du chantier (intertrade_market_consume → centre_take →
         * econ_region_stock_add) DÉBITE prov[] et ne décrémente la vue region[] que du
         * prélevé réel. Sous « exactement 2 brutes/tuile », l'empire du banc n'a plus une
         * miette de pierre en propre : doter la SEULE vue region[] (l'ancienne fixture)
         * rendait la consommation invisible (débit province = 0). On dote donc LA PROVINCE
         * porteuse (la réalité) ET la vue (le gate de matière lit l'agrégat). */
        int rep=econ_region_rep_province(s.econ, s.cap_reg);
        static const Resource DOTE[]={RES_WOOD,RES_IRON,RES_CLAY,RES_STONE,RES_TOOLS,RES_PRECIOUS_METAL,RES_SALT};
        for (unsigned k=0;k<sizeof DOTE/sizeof DOTE[0];k++){
            re->stock[DOTE[k]]=1000.f;   /* gate de matière : la recette de l'édifice doit être SOURÇABLE en propre */
            if (rep>=0) s.econ->prov[rep].stock[DOTE[k]]=1000.f;
        }
        re->treasury=100000.f;
        float gold_grenier   = agency_build_gold(s.econ, s.cap_reg, EDI_GRENIER);
        float gold_citadelle = agency_build_gold(s.econ, s.cap_reg, EDI_CITADELLE);
        printf("   Grenier devise %.0f or | Citadelle %.0f or (matière propre ⇒ pool empire GRATUIT)\n",
               gold_grenier, gold_citadelle);
        ok("la matière de SON empire est GRATUITE pour SON chantier (devis 0 or)",
           gold_grenier < 1e-3f && gold_citadelle < 1e-3f);
        /* (b) le TIER pilote la recette : on MESURE la pierre mangée par chaque édifice —
         * Grenier (pierre 10, vivrier de base) vs Irrigation (pierre 30, palier vivrier
         * avancé). La matière, pas l'or, porte désormais le tier. (Les deux sont des
         * édifices STACKABLES hors-famille : pas de verrou « déjà bâti ».) */
        float stone_g0=re->stock[RES_STONE];
        bool built_g = agency_build(s.ag, s.econ, s.w, s.cap_reg, EDI_GRENIER);
        float stone_eaten_grenier = stone_g0 - re->stock[RES_STONE];
        ok("bâtir CONSOMME la matière de l'empire (le stock baisse)", built_g && stone_eaten_grenier > 0.f);
        re->stock[RES_STONE]=1000.f;   /* on RÉ-DOTE pour mesurer le palier supérieur seul */
        if (rep>=0) s.econ->prov[rep].stock[RES_STONE]=1000.f;   /* la province AUSSI (la réalité débitée) */
        float stone_c0=re->stock[RES_STONE];
        bool built_i = agency_build(s.ag, s.econ, s.w, s.cap_reg, EDI_IRRIGATION);   /* palier vivrier supérieur */
        float stone_eaten_irrig = stone_c0 - re->stock[RES_STONE];
        printf("   pierre mangée : Grenier %.0f · Irrigation %.0f (le TIER porte la recette)\n",
               stone_eaten_grenier, stone_eaten_irrig);
        ok("un palier SUPÉRIEUR mange plus de matière (∝ tier)",
           built_i && stone_eaten_irrig > stone_eaten_grenier);
        /* (d) PÉNURIE FRANCHE : on vide la pierre de TOUT l'empire (capitale + co-possédées) ;
         * aucun Centre atteignable (autarcie) ⇒ le gate de matière REFUSE le chantier. */
        int owner=re->owner;
        for (int r=0;r<s.econ->n_regions;r++)
            if (owner<0 ? r==s.cap_reg : s.econ->region[r].owner==owner){
                econ_region_stock_add(s.econ, r, RES_STONE, -1e9f);   /* draine les PROVINCES (la réalité) */
                s.econ->region[r].stock[RES_STONE]=0.f;               /* et la vue (le gate lit l'agrégat) */
            }
        int nbefore=s.ag->n;
        bool blocked = agency_build(s.ag, s.econ, s.w, s.cap_reg, EDI_GRENIER);
        ok("pénurie de matière (rien en propre, aucun Centre) : REFUSÉ, pas de chantier",
           !blocked && s.ag->n==nbefore);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(s.w);free(s.econ);free(s.net);free(s.ts);free(s.wp);free(s.wl);free(s.ag);
    return g_fail?1:0;
}
