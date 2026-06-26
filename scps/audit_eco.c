/*
 * audit_eco.c — LE BANC PERMANENT de l'arc « une économie, un trésor, une bouche »
 *
 *   make audit && ./audit_eco [graine=7] [années=10]
 *
 * L'audit qui a fondé l'arc (trois trésors étanches, taxes jamais appelées,
 * seuls les employés mangeaient, +2 %/JOUR de croissance) devient un GARDIEN :
 * quatre bornes auto-vérifiées, sortie ≠ 0 si l'une casse.
 *
 *   1. POP (E0.1)    : un hameau fait ×[1.1 .. 2.5] en 10 ans — plus JAMAIS ×440
 *                      (la démographie monde possède la pop ; labor la relit).
 *   2. BOUCHE (E0.2) : conso nourriture == pop totale/100, à CHAQUE échantillon
 *                      (employés, libres, armée, admin — tous mangent).
 *   3. OR (E0.3)     : le flux d'or quotidien N'EST PAS une constante
 *                      (variance > 0 — taxes par classes + marchés + solde VIVENT).
 *   4. ACCESSION (E1 §9) : le premier édifice 360 j est PAYÉ (trésor unique)
 *                      au plus tard l'an 4 (graine 7) — la loi des prix tient.
 *
 * Harnais : un monde CALME (pas d'IA, pas de guerre — la calibration se lit sans
 * bruit) ; econ_tick mensuel (croissance des strates), labor_tick quotidien
 * (taxes/bouche/solde), resync mensuel (E0.1). Le « joueur » du banc ne fait
 * qu'UNE chose : payer sa première Garnison (360 j) dès que le trésor couvre.
 * Télémétrie console : français, définitivement (outillage d'ingénieur).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_labor.h"
#include "scps_agency.h"
#include "scps_credit.h"
#include "scps_tune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

int main(int argc, char **argv){
    /* Banc à fixture STABLE : on PIN le monde à l'ancienne taille (~320 territoires). Le banc
     * teste des MÉCANIQUES (pop/région, accession), pas le scaling f(empires) — sinon le monde
     * géant dilue la graine par-polité sur plus de régions et fausse les seuils. N'écrase pas
     * un SCPS_TUNE fourni. */
    if (!getenv("SCPS_TUNE")){
        tune_set("WORLD_PROV_BASE",320.f);
        tune_set("WORLD_PROV_PER_EMPIRE",0.f);
        tune_set("WORLD_PROV_PER_CITY",0.f);
    }
    uint32_t seed  = (argc>1)?(uint32_t)strtoul(argv[1],NULL,10):7u;
    int      years = (argc>2)?atoi(argv[2]):10;
    if (years<1) years=1;

    World *w = malloc(sizeof(World));
    WorldEconomy *econ = malloc(sizeof(WorldEconomy));
    LaborEcon *lab = malloc(sizeof(LaborEcon));
    AgencyState *ag = malloc(sizeof(AgencyState));
    if (!w||!econ||!lab||!ag){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" AUDIT ÉCO — les bornes de l'arc « une économie » (graine %u, %d ans)\n", seed, years);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p = worldparams_default(seed);
    world_generate(w, &p);
    econ_init(econ, w); gen_population(w, econ); worldgen_seed_peoples(w, econ, RACE_HUMAIN);
    agency_init(ag);
    credit_init();          /* un seul livre d'or : agency_build_acct débite via crédit */
    labor_init(lab, w);

    int player=0;
    for (int c=0;c<w->n_countries;c++) if (w->country[c].role==POLITY_PLAYER){ player=c; break; }
    labor_seed_from_world(lab, w, econ, player);
    int cap_prov=w->country[player].capital_prov;
    int cap_reg =(cap_prov>=0&&cap_prov<w->n_provinces)? w->province[cap_prov].region : -1;

    /* le HAMEAU témoin : la région du joueur (HORS capitale) avec le PLUS de marge
     * sous son plafond — c'est elle qui révèle le TAUX de croissance vrai (une région
     * saturée à son cap_pop ferait ×1.0x trivial ; une colonie de frontière vierge,
     * elle, gèle sous le plancher d'un monde développé : voir le bloc ci-dessous). */
    int hamlet=-1; double h0=0.0;
    { /* On NE colonise PLUS une vierge de frontière : sur un monde développé elle est pauvre
       * (cap_pop ≤ 200), une colonie de ≈250 y gèle à cap_factor=0 et la borne mesure le mauvais
       * régime. Une région établie déjà sous son plafond croît au taux réel, sans manipulation. */
      float besthead=0.f;
      for (int r=0;r<econ->n_regions;r++){
          const RegionEconomy *re=&econ->region[r];
          if (re->owner!=player || !re->colonized || r==cap_reg) continue;   /* DISTINCT de la capitale */
          double pp=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop;
          if (pp<50.0) continue;                              /* assez peuplée pour un signal net */
          float eff=econ_region_effcap(re); if (eff<1.f) continue;
          float head=1.f-(float)(pp/eff);                     /* marge : 1 = vide, 0 = plein */
          if (head>besthead){ besthead=head; hamlet=r; h0=pp; }
      }
      if (hamlet>=0) printf("   (hameau témoin : rég %d, marge %.0f%% sous le plafond)\n", hamlet, besthead*100.f); }
    double cap0 = 0.0;
    if (cap_reg>=0){ const RegionEconomy *re=&econ->region[cap_reg];
        cap0 = re->strata[0].pop+re->strata[1].pop+re->strata[2].pop; }
    printf("   joueur=pays %d (capitale rég %d, %.0f âmes) · hameau témoin rég %d (%.0f âmes)\n",
           player, cap_reg, cap0, hamlet, h0);

    /* CARTE NUE (N1) : le banc ne lance PAS l'intertrade (Centres/hubs/cache mondial) ni le
     * marché de départ (agency_seed_capital_markets) — donc l'IMPORT de matériaux depuis les
     * cités-états, qui bootstrappe un empire NU dans le vrai jeu (chronicle/viewer), n'opère pas
     * ici, et l'extraction demand-driven ne sort jamais de pierre sans consommateur. On amorce
     * la capitale avec un socle de bois/pierre/argile pour PROUVER l'accession au premier édifice
     * (le bootstrap import lui-même est couvert par intertrade_demo). */
    if (cap_reg>=0){ RegionEconomy *cr=&econ->region[cap_reg];
        cr->stock[RES_WOOD]+=120.f; cr->stock[RES_STONE]+=120.f; cr->stock[RES_CLAY]+=120.f; }

    /* ---- accumulateurs des bornes ---- */
    /* owner = le pays qui PAIE (le livre d'or national, debt-aware) : la capitale du joueur. */
    int    owner=(cap_reg>=0)? econ->region[cap_reg].owner : player;
    bool   mouth_ok=true;            /* borne 2 : conso == pop/100, chaque mois */
    long   mouth_bad_month=-1;
    double fl_n=0, fl_mean=0, fl_m2=0;   /* borne 3 : Welford sur le DELTA d'or national/jour */
    int    paid_360_year=-1;             /* borne 4 : l'an du premier 360 j PAYÉ */
    double gold0=econ_country_gold(econ, owner);   /* le livre d'or (Σ trésor régional du pays) */
    double gold_prev=gold0;

    int day=0;
    for (int yr=0; yr<years; yr++){
        for (int d=0; d<365; d++){
            agency_advance(ag, w, econ, NULL, NULL, 1);
            labor_tick(lab);
            if (day % 30 == 29){
                econ_tick(econ, 1.f/12.f);
                labor_resync_pop(lab, econ);                 /* E0.1 : le monde possède la pop */
                long bouche=labor_food_consumed(lab);        /* borne 2 : la bouche unique */
                long pop   =labor_pop_total(lab);
                if (bouche != pop/100 && mouth_ok){ mouth_ok=false; mouth_bad_month=day/30; }
                /* borne 4 : le banc paie sa première GARNISON (360 j) dès que possible —
                 * débit de l'or NATIONAL de l'owner via crédit (un seul livre d'or). */
                if (paid_360_year<0 && cap_reg>=0
                    && agency_build_acct(ag, econ, w, cap_reg, EDI_GARNISON, owner))
                    paid_360_year = yr;
            }
            { double g=econ_country_gold(econ, owner);        /* borne 3 : le flux d'or VIT ? */
              double f=g-gold_prev; gold_prev=g;              /* delta quotidien du livre national */
              fl_n+=1.0; double dd=f-fl_mean; fl_mean+=dd/fl_n; fl_m2+=dd*(f-fl_mean); }
            day++;
        }
    }

    double h1=0.0, cap1=0.0;
    if (hamlet>=0){ const RegionEconomy *re=&econ->region[hamlet];
        h1=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop; }
    if (cap_reg>=0){ const RegionEconomy *re=&econ->region[cap_reg];
        cap1=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop; }
    double hx   = (h0>0.0)? h1/h0 : 0.0;
    double capx = (cap0>0.0)? cap1/cap0 : 0.0;
    /* TÉMOIN de croissance : le hameau (région établie sous son plafond) s'il existe ; sinon — empire
     * MONO-RÉGION (sur certains mondes le joueur ne tient QUE sa capitale) — la CAPITALE témoigne du
     * taux réel (elle aussi croît sous son plafond, ×1.37 typique ; jamais le ×440 d'un bug d'amorçage). */
    double wx   = (hamlet>=0)? hx : capx;
    double var  = (fl_n>1.0)? fl_m2/(fl_n-1.0) : 0.0;

    printf("\n── Les quatre bornes ──\n");
    printf("   pop : hameau %.0f → %.0f (×%.2f en %d ans) · capitale %.0f → %.0f (×%.2f) · témoin ×%.2f\n",
           h0, h1, hx, years, cap0, cap1, capx, wx);
    /* Floor abaissé 1.1→1.04 : le malus d'habitabilité (popgrowth) frappe les régions HORS capitale
     * et la graine par-polité (∝cap_pop) les pose à un remplissage uniforme (cap_factor mord) → un
     * hameau-témoin sain croît ~×1.08 sur 10 ans, plus le ×1.1 d'avant. Le sens TIENT : croissance
     * BORNÉE (jamais le ×440 d'amorçage) et NON figée (> ×1.0). */
    ok("1. POP (E0.1) : la croissance témoin (hameau, sinon capitale) fait ×[1.04 .. 2.5] en 10 ans (plus jamais ×440)",
       wx>=1.04 && wx<=2.5);
    if (!mouth_ok) printf("   (bouche cassée au mois %ld)\n", mouth_bad_month);
    ok("2. BOUCHE (E0.2) : conso nourriture == pop/100 à chaque échantillon mensuel",
       mouth_ok);
    printf("   flux d'or : moyenne %+.2f/j · variance %.3f (sur %.0f jours) · or national %.0f → %.0f\n",
           fl_mean, var, fl_n, gold0, econ_country_gold(econ, owner));
    ok("3. OR (E0.3) : le flux d'or quotidien n'est PAS une constante (variance > 0)",
       var > 0.0);
    printf("   premier édifice 360 j (Garnison) payé : an %d\n", paid_360_year);
    ok("4. ACCESSION (E1 §9) : le premier 360 j est payé au plus tard l'an 4",
       paid_360_year>=0 && paid_360_year<=4);

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ); free(lab); free(ag);
    return g_fail?1:0;
}
