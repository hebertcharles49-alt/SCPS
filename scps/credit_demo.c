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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    printf("\n═══ BILAN : %d réussis, %d échoués ═══\n", g_pass, g_fail);
    free(w); free(e); free(wl);
    return g_fail?1:0;
}
