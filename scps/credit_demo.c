/*
 * credit_demo.c — banc DETTE & PRÊTS (scps_credit, incrément 1).
 *
 *   make credit_demo && ./credit_demo
 *
 * Scénario à la main (pas de world_generate) : un empire PAUVRE + une cité-état
 * RICHE voisine. On prouve : la ligne de crédit ÉMERGE de la taille éco ; dépenser
 * au-delà du trésor creuse une dette ; un créancier solvable est assigné ; l'intérêt
 * annuel creuse le débiteur et crédite le prêteur ; sans créancier le plafond mord
 * quand même ; save/load préserve le créancier.
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

/* (ré)installe le scénario : empire 0 pauvre, cité-état 1 riche. */
static void setup(WorldEconomy *e, float emp_tres, float cs_tres){
    e->region[0].owner=0; e->region[0].treasury=emp_tres;
    e->region[1].owner=1; e->region[1].treasury=cs_tres;
    e->region[0].strata[CLASS_LABORER].pop=1000.f;   /* pop => ligne de crédit > 0 */
    e->region[1].strata[CLASS_LABORER].pop=300.f;
}

int main(void){
    World *w=calloc(1,sizeof(World));
    WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
    WorldLegitimacy *wl=calloc(1,sizeof(WorldLegitimacy));
    if(!w||!e||!wl){ fprintf(stderr,"OOM\n"); return 1; }

    printf("═══ DETTE & PRÊTS — le plafond émerge de la solvabilité ═══\n");

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
    ok("au-delà du trésor mais SOUS la ligne : autorisé (on s'endette)", credit_can_spend(e,w,0,400.f));
    ok("au-delà de la ligne : REFUSÉ (plafond émergent)", !credit_can_spend(e,w,0,700.f));

    /* — 2. dépenser au-delà creuse la dette + assigne un créancier — */
    printf("\n── 2. Dépenser au-delà du trésor ──\n");
    credit_spend(e,w,0,400.f);
    ok("le trésor net passe NÉGATIF (dette)", econ_country_gold(e,0) < 0.0);
    ok("un créancier solvable est assigné (la cité-état)", credit_of(0)==1);

    /* — 3. l'intérêt annuel creuse le débiteur, crédite le créancier — */
    printf("\n── 3. L'intérêt annuel ──\n");
    double emp_before = econ_country_gold(e,0);
    double cs_before  = e->region[1].treasury;
    credit_year_tick(e, wl, w);
    ok("l'intérêt CREUSE le débiteur", econ_country_gold(e,0) < emp_before);
    ok("l'intérêt CRÉDITE le créancier (cité-état)", e->region[1].treasury > cs_before);

    /* — 4. soldé => le lien se dénoue — */
    printf("\n── 4. Solder la dette dénoue le lien ──\n");
    e->region[0].treasury = 200.f;                 /* repasse en positif */
    credit_year_tick(e, wl, w);
    ok("or >= 0 => plus de créancier", credit_of(0) < 0);

    /* — 5. plafond sans créancier solvable — */
    printf("\n── 5. Plafond sans créancier solvable ──\n");
    credit_init(); setup(e, 100.f, 0.f);           /* cité-état INSOLVABLE (trésor 0) */
    ok("au-delà de la ligne : toujours REFUSÉ (le plafond émerge, sans prêteur)", !credit_can_spend(e,w,0,700.f));
    credit_spend(e,w,0,400.f);
    ok("aucun créancier solvable => aucun créancier assigné", credit_of(0) < 0);

    /* — 6. save/load préserve le créancier — */
    printf("\n── 6. Save/load ──\n");
    credit_init(); setup(e, 100.f, 5000.f); credit_spend(e,w,0,400.f);   /* créancier = 1 */
    FILE *tmp=tmpfile();
    bool sv = tmp && credit_save(tmp);
    credit_init();                                 /* efface la mémoire vive */
    if (tmp) rewind(tmp);
    bool ld = tmp && credit_load(tmp);
    if (tmp) fclose(tmp);
    ok("save/load roundtrip préserve le créancier", sv && ld && credit_of(0)==1);

    /* — 7. LE JOUEUR entre dans la dette par un chantier (incrément 2) —
     * L'incrément 1 ne prouvait que l'IA ; le joueur emprunte au MÊME livre. On choisit
     * P = le pays joueur, on lui passe une dépense de TAILLE CHANTIER au-delà de son or,
     * et la mécanique mord pareil : trésor net < 0, créancier assigné, l'intérêt creuse. */
    printf("\n── 7. Le JOUEUR entre dans la dette (chantier > trésor) ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    int P=0;   /* le pays joueur (POLITY_PLAYER) */
    float build_cost = (float)econ_country_gold(e,P) + 350.f;   /* > trésor, taille chantier, sous la ligne */
    credit_spend(e,w,P,build_cost);
    ok("le chantier pousse l'or du JOUEUR sous zéro (dette)", econ_country_gold(e,P) < 0.0);
    ok("un créancier (cité-état/mercantile) est assigné au joueur", credit_of(P) >= 0);
    double p_before  = econ_country_gold(e,P);
    double len_before= e->region[credit_of(P)].treasury;
    credit_year_tick(e, wl, w);
    ok("l'intérêt CREUSE encore le joueur (la dette grandit)", econ_country_gold(e,P) < p_before);
    ok("l'intérêt CRÉDITE le prêteur du joueur", e->region[credit_of(P)].treasury > len_before);

    printf("\n═══ BILAN : %d réussis, %d échoués ═══\n", g_pass, g_fail);
    free(w); free(e); free(wl);
    return g_fail?1:0;
}
