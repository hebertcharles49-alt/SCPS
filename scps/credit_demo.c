/*
 * credit_demo.c — banc DETTE & PRÊTS (scps_credit, M3c : LE CRÉDIT RÉEL).
 *
 *   make credit_demo && ./credit_demo
 *
 * Scénario à la main (pas de world_generate) : un empire PAUVRE + une cité-état
 * RICHE voisine. On prouve : la ligne de crédit ÉMERGE de la taille éco ; dépenser
 * au-delà du trésor local NE LAISSE PLUS le trésor net passer négatif — la chaîne
 * d'emprunt (péréquation → classes → cité-état) avance de VRAIES pièces et enregistre
 * un passif RÉEL ; le prêteur voit RÉELLEMENT son trésor baisser ; l'intérêt annuel
 * creuse le débiteur et crédite le créancier ; un surplus amortit le principal ;
 * save/load préserve la dette.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_legitimacy.h"
#include "scps_credit.h"
#include "scps_types.h"
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

/* (ré)installe le scénario : empire 0 pauvre, cité-état 1 riche.
 * RE-KEY PROVINCE (PROVINCE_MODEL.md) : treasury/pop sont PROVINCE-OWNED — credit_spend/
 * credit_year_tick/credit_borrow* routent sur prov[] (region[] est un DÉRIVÉ Σ, écrasé par
 * econ_aggregate_regions). Ce banc À LA MAIN pose donc la vérité sur prov[] ;
 * econ_aggregate_regions() est appelée après pour que les lecteurs region[]-grain
 * (econ_country_gold, ce banc) restent à jour — même idiome que forks_demo.c/social_demo.c. */
static void setup(WorldEconomy *e, float emp_tres, float cs_tres){
    e->n_prov=2;
    e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
    e->prov[0].treasury=emp_tres;
    e->prov[1].owner=1; e->prov[1].region=1; e->prov[1].active=true; e->prov[1].colonized=true;
    e->prov[1].treasury=cs_tres;
    e->prov[0].strata[CLASS_LABORER].pop=1000.f;   /* pop => ligne de crédit > 0 */
    e->prov[1].strata[CLASS_LABORER].pop=300.f;
    e->region_rep_prov[0]=0; e->region_rep_prov[1]=1;   /* 1 province/région : la représentative est directe */
    econ_aggregate_regions(e);   /* region[] à jour pour econ_country_gold et les lectures du banc */
    /* M3d — LE PLAFOND + LA TRANCHE (credit_borrow_local/citystate) mordent désormais sur
     * econ_country_tax_year (le REVENU ANNUEL) : ce banc À LA MAIN ne tourne jamais econ_tick
     * (aucun tax_year capté) — sans un revenu seedé, la ligne serait TOUJOURS 0 et la chaîne
     * d'emprunt refuserait tout. On CAPTE un revenu annuel plausible (3000, ceiling=9000,
     * tranche=600 — au-dessus des besoins testés ~300-350) via le canal officiel
     * (econ_flux_add+econ_flux_year_capture), pas une valeur seedée à la main. */
    econ_flux_add(0, FX_TAX, 3000.f);
    econ_flux_add(1, FX_TAX, 3000.f);
    econ_flux_year_capture();
}

int main(void){
    World *w=calloc(1,sizeof(World));
    WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
    WorldLegitimacy *wl=calloc(1,sizeof(WorldLegitimacy));
    if(!w||!e||!wl){ fprintf(stderr,"OOM\n"); return 1; }

    printf("═══ DETTE & PRÊTS — M3c : le crédit RÉEL ═══\n");

    /* monde minimal : 2 pays (empire 0, cité-état 1), 2 provinces/régions. */
    w->n_countries=2; w->n_provinces=2;
    w->country[0].role=POLITY_PLAYER;      w->country[0].capital_prov=0;
    w->country[1].role=POLITY_CITY_STATE;  w->country[1].capital_prov=1;
    w->province[0].region=0; w->province[1].region=1;
    e->n_regions=2;
    setup(e, 100.f, 5000.f);
    credit_init();

    /* — 1. la ligne de crédit émerge de la taille éco — */
    float line0 = credit_line(w,e,0);
    printf("\n── 1. Ligne de crédit (∝ pop) = %.0f ──\n", line0);
    ok("la ligne de crédit ÉMERGE de la pop (> 0)", line0 > 0.f);
    ok("dans le trésor : autorisé", credit_can_spend(e,w,0,50.f));
    ok("au-delà du trésor mais SOUS la ligne : autorisé (la chaîne d'emprunt avancera)", credit_can_spend(e,w,0,400.f));
    ok("au-delà de la ligne : REFUSÉ (plafond émergent)", !credit_can_spend(e,w,0,700.f));

    /* — 2. dépenser au-delà du trésor local : la chaîne d'emprunt avance de VRAIES
     * pièces (péréquation vide ici — 1 seule province/pays — puis classes SANS
     * richesse ici — puis la cité-état, seule solvable) — le trésor NET ne passe PLUS
     * négatif, un passif RÉEL est enregistré, et le PRÊTEUR voit son trésor baisser. */
    printf("\n── 2. Dépenser au-delà du trésor local ──\n");
    double cs_gold_before = e->prov[1].treasury;
    credit_spend(e,w,0,400.f);
    econ_aggregate_regions(e);
    ok("le trésor net NE passe PLUS négatif (dette RÉELLE, pas imprimée)", econ_country_gold(e,0) >= -1e-3);
    ok("un créancier solvable est assigné (la cité-état)", credit_of(0)==1);
    ok("un passif RÉEL est enregistré (dette-cité-état > 0)", credit_debt_citystate(0) > 0.f);
    ok("le PRÊTEUR a RÉELLEMENT avancé les fonds (son trésor baisse)", e->prov[1].treasury < cs_gold_before);

    /* — 3. l'intérêt annuel creuse le débiteur (via son surplus, pas une dette
     * fabriquée), crédite le créancier — */
    printf("\n── 3. L'intérêt annuel ──\n");
    /* on redote le débiteur d'un surplus pour que l'intérêt ait de quoi se payer
     * (sinon il est auto-limité à 0 — comportement voulu, testé au §5). */
    e->prov[0].treasury = 600.f; econ_aggregate_regions(e);
    double emp_before = econ_country_gold(e,0);
    double cs_before  = e->prov[1].treasury;
    double debt_before= credit_debt_total(0);
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("l'intérêt CREUSE le débiteur (son surplus baisse)", econ_country_gold(e,0) < emp_before);
    ok("l'intérêt CRÉDITE le créancier (cité-état)", e->prov[1].treasury > cs_before);
    ok("le PRINCIPAL de la dette n'a PAS grossi rien qu'à payer l'intérêt", credit_debt_total(0) <= debt_before + 1e-3);

    /* — 4. un surplus SUBSTANTIEL amortit le principal — */
    printf("\n── 4. Amortissement du principal ──\n");
    e->prov[0].treasury = 20000.f;   /* trésor GRAS : au-dessus de COURT_FLOOR */
    econ_aggregate_regions(e);
    double debt_before2 = credit_debt_total(0);
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("le principal DIMINUE depuis un trésor gras", credit_debt_total(0) < debt_before2);

    /* — 5. plafond sans prêteur solvable : la chaîne échoue, le trésor reste ce qu'il
     * peut payer localement (jamais négatif au-delà de ce qu'aucune source ne couvre) — */
    printf("\n── 5. Aucun prêteur solvable ──\n");
    credit_init(); setup(e, 100.f, 0.f);           /* cité-état INSOLVABLE (trésor 0) */
    ok("au-delà de la ligne : toujours REFUSÉ (le plafond émerge, sans prêteur)", !credit_can_spend(e,w,0,700.f));
    credit_spend(e,w,0,400.f);
    ok("aucun prêteur solvable => aucun créancier assigné", credit_of(0) < 0);

    /* — 6. save/load préserve la dette — */
    printf("\n── 6. Save/load ──\n");
    credit_init(); setup(e, 100.f, 5000.f); credit_spend(e,w,0,400.f);   /* créancier = 1, dette > 0 */
    float debt_ref = credit_debt_total(0);
    FILE *tmp=tmpfile();
    bool sv = tmp && credit_save(tmp);
    credit_init();                                 /* efface la mémoire vive */
    if (tmp) rewind(tmp);
    bool ld = tmp && credit_load(tmp);
    if (tmp) fclose(tmp);
    ok("save/load roundtrip préserve le créancier ET la dette", sv && ld && credit_of(0)==1 && credit_debt_total(0)==debt_ref);

    /* — 7. LE JOUEUR entre dans la dette par un chantier (incrément 2) —
     * une dépense de TAILLE CHANTIER au-delà de son or : la mécanique mord pareil,
     * un créancier est assigné, l'intérêt creuse — SANS trésor négatif. */
    printf("\n── 7. Le JOUEUR entre dans la dette (chantier > trésor) ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    int P=0;   /* le pays joueur (POLITY_PLAYER) */
    float build_cost = (float)econ_country_gold(e,P) + 350.f;   /* > trésor, taille chantier, sous la ligne */
    credit_spend(e,w,P,build_cost);
    econ_aggregate_regions(e);
    ok("le chantier NE pousse PLUS l'or du JOUEUR sous zéro", econ_country_gold(e,P) >= -1e-3);
    ok("un créancier (cité-état/mercantile) est assigné au joueur", credit_of(P) >= 0);
    ok("un passif RÉEL couvre le chantier", credit_debt_total(P) > 0.f);
    e->prov[P].treasury += 600.f; econ_aggregate_regions(e);   /* redote le joueur (l'intérêt a de quoi se payer) */
    double p_before  = econ_country_gold(e,P);
    double len_before= e->prov[credit_of(P)].treasury;
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("l'intérêt CREUSE encore le joueur", econ_country_gold(e,P) < p_before);
    ok("l'intérêt CRÉDITE le prêteur du joueur", e->prov[credit_of(P)].treasury > len_before);

    /* ═══════════════════════════════════════════════════════════════════════════════
     * MONNAIE M11 — LA VAGUE AUDIT-SOL : A4, les 5 contrôles qui manquaient. Chaque
     * nouveau contrôle est écrit pour ÉCHOUER sur pre-m11 et PASSER sur HEAD (prouvé
     * séparément par le rapport de mission, pas ici). */

    /* — 8. A2 : cohérence immédiate prov[cap].treasury == region[].treasury SANS
     * ré-agrégation manuelle. Les tests 1-7 ci-dessus ré-agrègent à la main après CHAQUE
     * mutation (setup(), motif « même idiome que forks_demo.c/social_demo.c ») — ce
     * contrôle ne le fait PAS après credit_year_tick : la vérité doit tenir SEULE. — */
    printf("\n── 8. A2 : cohérence immédiate SANS ré-agrégation manuelle ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    credit_spend(e,w,0,400.f); econ_aggregate_regions(e);    /* SETUP (motif test 2) : établit un créancier */
    e->prov[0].treasury = 600.f; econ_aggregate_regions(e);  /* SETUP (motif test 3) : redote le débiteur */
    credit_year_tick(e, wl, w);   /* AUCUNE ré-agrégation manuelle après CET appel */
    ok("le trésor du DÉBITEUR (empire, intérêt payé) reste cohérent prov==region SANS ré-agrégation manuelle",
       fabsf(e->prov[0].treasury - e->region[0].treasury) < 1e-2f);
    ok("le trésor du CRÉANCIER (cité-état, intérêt reçu) reste cohérent prov==region SANS ré-agrégation manuelle",
       fabsf(e->prov[1].treasury - e->region[1].treasury) < 1e-2f);

    /* — 9. A3 v2 : L'INTÉRÊT FIXE À L'ORIGINATION (décision joueur, en cours de mission —
     * remplace l'arriéré-qui-capitalise v1 : « 1000 à 5 % ⇒ tu rembourses 1050, pas +5 %/an »).
     * DEBT_FIXED=0 (legacy) : la dette inscrite == le montant RÉEL emprunté, aucun markup ;
     * DEBT_FIXED=1 : le markup (taux courant, credit_current_rate) est figé À L'EMPRUNT — et
     * ne grossit JAMAIS ensuite, même après des ANNÉES d'échéances totalement impayées
     * (« fixe veut dire fixe », pas de capitalisation). — */
    printf("\n── 9. A3 v2 : l'intérêt FIXE à l'origination (DEBT_FIXED) ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 0.f);
    float borrowed_legacy9 = credit_borrow_citystate(e,w,0,400.f); econ_aggregate_regions(e);
    float debt_legacy9 = credit_debt_total(0);
    printf("   emprunté=%.1f · dette inscrite (DEBT_FIXED=0)=%.1f\n", borrowed_legacy9, debt_legacy9);
    ok("DEBT_FIXED=0 (legacy) : la dette inscrite == le montant RÉEL emprunté (aucun markup)",
       fabsf(debt_legacy9 - borrowed_legacy9) < 1.f);

    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 1.f);
    float borrowed_fixed9 = credit_borrow_citystate(e,w,0,400.f); econ_aggregate_regions(e);
    float debt_fixed9 = credit_debt_total(0);
    printf("   emprunté=%.1f · dette inscrite (DEBT_FIXED=1)=%.1f (markup à l'origination=%.1f, taux=%.1f%%)\n",
           borrowed_fixed9, debt_fixed9, debt_fixed9-borrowed_fixed9,
           100.0*(debt_fixed9/borrowed_fixed9-1.0));
    ok("DEBT_FIXED=1 : la dette inscrite INCLUT le markup à l'origination (> montant RÉELLEMENT emprunté)",
       debt_fixed9 > borrowed_fixed9 + 1.f);

    e->prov[0].treasury = 0.f; econ_aggregate_regions(e);   /* AUCUN surplus : l'échéance sera TOTALEMENT impayée */
    float debt_before9c = credit_debt_total(0);
    for (int yr9=0; yr9<10; yr9++) credit_year_tick(e, wl, w);
    ok("DEBT_FIXED=1 : 10 ans d'échéances TOTALEMENT impayées NE FONT PAS grossir la dette (fixe veut dire fixe)",
       credit_debt_total(0) <= debt_before9c + 1e-3f);

    /* — 10. A3 v2 : des ÉCHÉANCES impayées SUR UNE DETTE SUBSTANTIELLE ⇒ streak d'impayés ⇒
     * banqueroute FORCÉE — SANS jamais approcher le plafond (reproduit puis corrige le
     * scénario exact de l'audit : « un pays à 200 % sans trésor qui ne dépense plus ne fait
     * JAMAIS faillite »). Revenu SAIN (tax_year=3000 ⇒ plafond=9000, large marge) — aucun
     * artifice d'effondrement du plafond requis ici (contrairement à v1) : le streak
     * d'impayés réagit désormais SEUL. Dette empruntée en 6 tranches (credit_borrow_citystate,
     * 600 chacune — la tranche/tick M3d — pour dépasser DEBT_DEFAULT_THRESHOLD=3000, le
     * plancher « dette qui compte », calibrage sweep, cf. scps_credit.c) : le trivial ne fait
     * PAS faillite, le substantiel SI. — */
    printf("\n── 10. A3 v2 : des échéances impayées ⇒ streak ⇒ banqueroute forcée ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 0.f);
    for (int b10=0;b10<6;b10++) credit_borrow_citystate(e,w,0,600.f);
    econ_aggregate_regions(e);
    e->prov[0].treasury = 0.f; econ_aggregate_regions(e);   /* aucun surplus : impayé TOTAL chaque année */
    float ceiling10 = credit_debt_ceiling(0);
    printf("   dette=%.0f · plafond=%.0f (large marge — motif audit « 200%% sans trésor »)\n",
           credit_debt_total(0), ceiling10);
    ok("la dette est LARGEMENT sous le plafond (le test isole le défaut d'échéance, pas le plafond)",
       credit_debt_total(0) < ceiling10 * 0.5f);
    bool forced_legacy=false;
    for (int yr10=0; yr10<20 && !forced_legacy; yr10++){
        credit_year_tick(e, wl, w);
        if (credit_bankrupt_pending(0)) forced_legacy=true;
    }
    ok("DEBT_FIXED=0 (legacy) : 20 ans d'impayés TOTAUX, dette SOUS le plafond ⇒ JAMAIS de "
       "banqueroute forcée — le bug de l'audit reproduit", !forced_legacy);

    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 1.f);
    for (int b10=0;b10<6;b10++) credit_borrow_citystate(e,w,0,600.f);
    econ_aggregate_regions(e);
    e->prov[0].treasury = 0.f; econ_aggregate_regions(e);
    ok("la dette SUBSTANTIELLE dépasse le plancher « dette qui compte » (DEBT_DEFAULT_THRESHOLD)",
       credit_debt_total(0) > 3000.f);
    bool forced_fixed=false; int yr_forced=-1;
    for (int yr10=0; yr10<20 && !forced_fixed; yr10++){
        credit_year_tick(e, wl, w);
        if (credit_bankrupt_pending(0)){ forced_fixed=true; yr_forced=yr10+1; }
    }
    printf("   DEBT_FIXED=1 : banqueroute forcée %s (streak=%d, dette finale=%.0f, plafond=%.0f)\n",
           forced_fixed?"DÉCLENCHÉE":"jamais déclenchée", credit_insolvent_streak(0),
           credit_debt_total(0), credit_debt_ceiling(0));
    if (forced_fixed) printf("      → à l'an %d (dette TOUJOURS sous le plafond)\n", yr_forced);
    ok("DEBT_FIXED=1 (A3 v2) : des échéances impayées déclenchent NATURELLEMENT la banqueroute "
       "forcée SANS jamais atteindre le plafond (le défaut réel)",
       forced_fixed && credit_debt_total(0) < credit_debt_ceiling(0));

    /* — le PLANCHER « dette qui compte » (DEBT_DEFAULT_THRESHOLD) : un résidu TRIVIAL, jamais
     * remboursable (même trésor 0 à vie), NE fait PAS faillite — seule une dette substantielle
     * l'engage. Calibrage (sweep {9,11,42}×3×250, 4 points) : sans ce plancher, Σ banqueroutes
     * explosait de 583 à ~1950 QUELLE QUE SOIT DEBT_DUE_FRAC (n'importe quel résidu comptait) —
     * à 3000 (retenu), Σ 795 (+36 %, sous le doublement) ET invariant 0/9. — */
    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 1.f);
    credit_borrow_citystate(e,w,0,50.f); econ_aggregate_regions(e);   /* trivial : bien SOUS le plancher */
    e->prov[0].treasury = 0.f; econ_aggregate_regions(e);
    bool forced_trivial=false;
    for (int yr10=0; yr10<20 && !forced_trivial; yr10++){
        credit_year_tick(e, wl, w);
        if (credit_bankrupt_pending(0)) forced_trivial=true;
    }
    printf("   dette TRIVIALE=%.0f (< plancher 3000) · 20 ans d'impayés : banqueroute forcée %s\n",
           credit_debt_total(0), forced_trivial?"DÉCLENCHÉE (ANOMALIE)":"jamais déclenchée (attendu)");
    ok("le PLANCHER « dette qui compte » protège un résidu TRIVIAL de la banqueroute forcée",
       !forced_trivial);

    /* — 11. Banqueroute FORCÉE effective + LA SAISIE post-faillite (M3g), sur l'état
     * laissé par le test 10 (g_forced_pending[0] vrai, créancier cs=1). — */
    printf("\n── 11. Banqueroute forcée + saisie post-faillite ──\n");
    long forced_before11, vol_before11; credit_bankruptcy_stats(&forced_before11,&vol_before11);
    int L11 = credit_bankruptcy(e, 0, true /* forcée */);
    ok("la banqueroute FORCÉE répudie la dette (RAZ)", credit_debt_total(0)==0.f);
    ok("le créancier répudié est bien la cité-état identifiée", L11==1);
    long forced_after11, vol_after11; credit_bankruptcy_stats(&forced_after11,&vol_after11);
    ok("la télémétrie « forcée » s'incrémente (pas « volontaire »)",
       forced_after11==forced_before11+1 && vol_after11==vol_before11);
    ok("la cicatrice de banqueroute est posée sur la province", e->prov[0].bankruptcy_scar > 0.f);
    ok("le créancier D'AVANT-répudiation est figé pour la saisie (M3g)",
       credit_garnish_cs_id(0)==1 && credit_garnish_cs_share(0) > 0.f);
    credit_garnish_note(0, 50.f, 30.f);   /* motif econ_tick §confiscation : 30 or de part cité-état */
    ok("la part cité-état de la saisie s'accumule (en attente du règlement annuel)",
       credit_garnish_cs_pending(0) > 29.f);
    double cs_treas_before11 = (double)e->prov[1].treasury;
    credit_year_tick(e, wl, w);
    ok("LA SAISIE post-faillite règle la part cité-état au créancier figé (motif M3g)",
       (double)e->prov[1].treasury > cs_treas_before11 + 29.0);
    ok("le règlement de la saisie ne laisse AUCUN reliquat (pending RAZ)", credit_garnish_cs_pending(0)==0.f);

    /* — 12. Banqueroute VOLONTAIRE (CMD_BANKRUPTCY, joueur) — repartie à zéro. — */
    printf("\n── 12. Banqueroute volontaire ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    credit_spend(e,w,0,400.f); econ_aggregate_regions(e);
    ok("une dette réelle existe avant la répudiation volontaire", credit_debt_total(0) > 0.f);
    long fb12, vb12; credit_bankruptcy_stats(&fb12,&vb12);
    credit_bankruptcy(e, 0, false /* volontaire */);
    ok("la banqueroute VOLONTAIRE répudie aussi la dette (RAZ)", credit_debt_total(0)==0.f);
    long fa12, va12; credit_bankruptcy_stats(&fa12,&va12);
    ok("la télémétrie « volontaire » s'incrémente (pas « forcée »)", va12==vb12+1 && fa12==fb12);

    /* — 13. A1 : LA FRAPPE À PARITÉ PLEINE + CONSERVATION (frappe royale+libre,
     * MINT_FULL_PARITY). Fixture directe à un pays/une province (econ_tick ne prend pas
     * de World* — motif econ_tax_demo.c, ici allégé sans world_generate). Trésor posé
     * EXACTEMENT à SINK_FLOOR (500) : au-dessus, la redépense publique I3bis
     * (scps_econ.c §I3bis, « le trou DÉJÀ documenté de l'instrument » — sans pop/impôt
     * ici, coll_tot=0, la part payroll de la redépense ne revient à AUCUNE classe) est un
     * SITE DE DESTRUCTION distinct, connu, HORS SCOPE M11 (M0/M3c l'ont déjà classé) —
     * l'annuler ici isole PROPREMENT la conservation à CE que A1 change. */
    printf("\n── 13. A1 : la frappe à parité pleine + conservation ──\n");
    tune_set("DEBT_FIXED", 1.f);   /* redéfinit le défaut pour la suite (motif tests 9/10) */
    memset(e, 0, sizeof(WorldEconomy));
    credit_init();
    e->n_prov=1; e->n_regions=1;
    e->prov[0].owner=0; e->prov[0].region=0;
    e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
    e->region_rep_prov[0]=0;
    e->prov[0].treasury=500.f;            /* == SINK_FLOOR : la redépense I3bis (hors scope) reste nulle */
    e->reserve_gold[0]=120.f;             /* réserve d'État (royalty en nature) : frappe ROYALE, DÉJÀ à parité pleine */
    e->prov[0].stock[RES_GOLD]=1000.f;    /* marché privé : frappe LIBRE (A1) */
    e->prov[0].price[RES_GOLD]=8.f;       /* < MINT_PARITY_GOLD (16) : arbitrage positif */
    econ_aggregate_regions(e);
    econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();   /* revenu annuel plausible (motif setup()) */

    double m_before13 = (double)e->prov[0].treasury;
    for (int k13=0;k13<CLASS_COUNT;k13++) m_before13 += (double)e->prov[0].strata[k13].wealth;
    float reserve_before13 = e->reserve_gold[0];
    float stock_before13   = e->prov[0].stock[RES_GOLD];

    tune_set("MINT_FULL_PARITY", 1.f);
    econ_tick(e, 1.f/12.f);

    ok("prov[cap].treasury == region[].treasury SANS ré-agrégation manuelle APRÈS la frappe (A2)",
       fabsf(e->prov[0].treasury - e->region[0].treasury) < 1e-2f);
    ok("la réserve d'État a été prélevée (frappe royale, § M2)", e->reserve_gold[0] < reserve_before13);
    ok("le stock de marché a RÉELLEMENT diminué (le métal quitte le marché, frappe libre A1)",
       e->prov[0].stock[RES_GOLD] < stock_before13);
    double wealth_after13=0.0; for (int k13=0;k13<CLASS_COUNT;k13++) wealth_after13 += (double)e->prov[0].strata[k13].wealth;
    ok("A1 : un vendeur RÉEL a été payé pour son métal (richesse des classes > 0)", wealth_after13 > 1.0);

    double m_after13 = (double)e->prov[0].treasury + wealth_after13;
    double frappe13  = (double)econ_flux_get(0, FX_MINT);
    printf("   M avant=%.1f · M après=%.1f (Δ=%.1f) · FX_MINT (vraie création, royale+libre)=%.1f\n",
           m_before13, m_after13, m_after13-m_before13, frappe13);
    ok("CONSERVATION : ΔM == la VRAIE création (FX_MINT), à l'arrondi près (l'invariant sous A1)",
       fabs((m_after13 - m_before13) - frappe13) < 1.0);

    /* kill-switch : sans A1, la frappe libre ne paie JAMAIS le vendeur — reproduit le
     * bug de l'audit (richesse des classes reste nulle, seul le gain net était crédité). */
    memset(e, 0, sizeof(WorldEconomy));
    e->n_prov=1; e->n_regions=1;
    e->prov[0].owner=0; e->prov[0].region=0;
    e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
    e->region_rep_prov[0]=0;
    e->prov[0].treasury=500.f;
    e->prov[0].stock[RES_GOLD]=1000.f;
    e->prov[0].price[RES_GOLD]=8.f;
    econ_aggregate_regions(e);
    econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
    float stock_legacy_before13 = e->prov[0].stock[RES_GOLD];
    tune_set("MINT_FULL_PARITY", 0.f);
    econ_tick(e, 1.f/12.f);
    double wealth_legacy13=0.0; for (int k13=0;k13<CLASS_COUNT;k13++) wealth_legacy13 += (double)e->prov[0].strata[k13].wealth;
    printf("   MINT_FULL_PARITY=0 (legacy) : stock %.1f→%.1f (métal disparu) · richesse classes=%.2f (jamais payé) · FX_MINT=%.1f\n",
           stock_legacy_before13, e->prov[0].stock[RES_GOLD], wealth_legacy13, (double)econ_flux_get(0, FX_MINT));
    ok("MINT_FULL_PARITY=0 (legacy pré-M11) : reproduit le bug de l'audit — AUCUN vendeur payé",
       wealth_legacy13 < 1e-3);
    ok("MINT_FULL_PARITY=0 (legacy pré-M11) : le métal disparaît quand même du marché (le trou de l'audit)",
       e->prov[0].stock[RES_GOLD] < stock_legacy_before13);
    tune_set("MINT_FULL_PARITY", 1.f);   /* redéfinit le défaut */

    printf("\n═══ BILAN : %d réussis, %d échoués ═══\n", g_pass, g_fail);
    free(w); free(e); free(wl);
    return g_fail?1:0;
}
