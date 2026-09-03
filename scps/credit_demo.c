/*
 * credit_demo.c — banc DETTE & PRÊTS (scps_credit, M3c : LE CRÉDIT RÉEL).
 *
 *   make credit_demo && ./credit_demo
 *
 * Scénario à la main (pas de world_generate) : un empire PAUVRE + une cité-état
 * RICHE voisine. On prouve : la ligne de crédit ÉMERGE des réserves/expositions des prêteurs ; dépenser
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
 * TRÉSOR NATIONAL (2026-09-03, docs/DESIGN_TRESOR_NATIONAL.md) : l'or d'État est au grain
 * PAYS — UNE caisse par empire, ni province ni région n'en tient plus une part. La pop, elle,
 * reste PROVINCE-OWNED (region[] en est le Σ agrégé) : econ_aggregate_regions() reste appelée
 * pour les lecteurs pop/richesse région-grain, plus jamais pour l'or. */
static void setup(WorldEconomy *e, float emp_tres, float cs_tres){
    e->n_prov=2;
    e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
    e->prov[1].owner=1; e->prov[1].region=1; e->prov[1].active=true; e->prov[1].colonized=true;
    e->nat_treasury[0]=emp_tres;   /* LE trésor de l'empire 0 */
    e->nat_treasury[1]=cs_tres;    /* LE trésor de la cité-état 1 */
    e->prov[0].strata[CLASS_LABORER].pop=1000.f;
    e->prov[1].strata[CLASS_LABORER].pop=300.f;
    e->region_rep_prov[0]=0; e->region_rep_prov[1]=1;   /* 1 province/région : la représentative est directe */
    econ_aggregate_regions(e);   /* region[] à jour pour les lectures pop/richesse du banc */
    /* Le revenu annuel ne ferme plus le marché : il cote seulement le taux. On le sème par
     * le canal officiel pour que les assertions de ratio/taux aient une assiette connue. */
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

    /* — 1. la ligne de crédit émerge des prêteurs, pas de la population du débiteur — */
    float line0 = credit_line(w,e,0);
    printf("\n── 1. Crédit disponible (réserves + exposition) = %.0f ──\n", line0);
    ok("la ligne de crédit ÉMERGE des fonds physiques du prêteur (> 0)", line0 > 0.f);
    e->prov[0].strata[CLASS_LABORER].pop*=10.f; econ_aggregate_regions(e);
    ok("multiplier la population du débiteur ne fabrique AUCUNE capacité de prêt",
       fabsf(credit_line(w,e,0)-line0)<0.01f);
    e->prov[0].strata[CLASS_LABORER].pop/=10.f; econ_aggregate_regions(e);
    ok("dans le trésor : autorisé", credit_can_spend(e,w,0,50.f));
    ok("au-delà du trésor mais sous la liquidité offerte : autorisé", credit_can_spend(e,w,0,700.f));
    ok("au-delà des réserves/expositions des prêteurs : refusé", !credit_can_spend(e,w,0,100.f+line0+10.f));

    /* — 2. dépenser au-delà du trésor local : la chaîne d'emprunt avance de VRAIES
     * pièces (péréquation vide ici — 1 seule province/pays — puis classes SANS
     * richesse ici — puis la cité-état, seule solvable) — le trésor NET ne passe PLUS
     * négatif, un passif RÉEL est enregistré, et le PRÊTEUR voit son trésor baisser. */
    printf("\n── 2. Dépenser au-delà du trésor local ──\n");
    double cs_gold_before = econ_country_gold(e,1);
    bool spent2=credit_spend(e,w,0,400.f);
    econ_aggregate_regions(e);
    ok("la dépense est validée EN ENTIER", spent2);
    ok("le trésor net NE passe PLUS négatif (dette RÉELLE, pas imprimée)", econ_country_gold(e,0) >= -1e-3);
    ok("un créancier solvable est assigné (la cité-état)", credit_of(0)==1);
    ok("un passif RÉEL est enregistré (dette-cité-état > 0)", credit_debt_citystate(0) > 0.f);
    ok("le PRÊTEUR a RÉELLEMENT avancé les fonds (son trésor baisse)", econ_country_gold(e,1) < cs_gold_before);

    /* — 3. l'intérêt annuel creuse le débiteur (via son surplus, pas une dette
     * fabriquée), crédite le créancier — */
    printf("\n── 3. L'intérêt annuel ──\n");
    /* on redote le débiteur d'un surplus pour que l'intérêt ait de quoi se payer
     * (sinon il est auto-limité à 0 — comportement voulu, testé au §5). */
    e->nat_treasury[0] = 600.f;
    double emp_before = econ_country_gold(e,0);
    double cs_before  = econ_country_gold(e,1);
    double debt_before= credit_debt_total(0);
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("l'intérêt CREUSE le débiteur (son surplus baisse)", econ_country_gold(e,0) < emp_before);
    ok("l'intérêt CRÉDITE le créancier (cité-état)", econ_country_gold(e,1) > cs_before);
    ok("le PRINCIPAL de la dette n'a PAS grossi rien qu'à payer l'intérêt", credit_debt_total(0) <= debt_before + 1e-3);

    /* — 4. un surplus SUBSTANTIEL amortit le principal — */
    printf("\n── 4. Amortissement du principal ──\n");
    e->nat_treasury[0] = 20000.f;   /* trésor GRAS : au-dessus de COURT_FLOOR */
    double debt_before2 = credit_debt_total(0);
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("le principal DIMINUE depuis un trésor gras", credit_debt_total(0) < debt_before2);

    /* — 5. sans prêteur solvable : l'action entière est refusée, sans paiement
     * partiel ni effet gratuit chez l'appelant. — */
    printf("\n── 5. Aucun prêteur solvable ──\n");
    credit_init(); setup(e, 100.f, 0.f);           /* cité-état INSOLVABLE (trésor 0) */
    ok("sans réserve chez un prêteur : aucune ligne de crédit", credit_line(w,e,0)<=1e-4f);
    ok("une dépense sans fonds physiques est REFUSÉE au préflight",
       !credit_can_spend(e,w,0,400.f));
    double cash5=econ_country_gold(e,0);
    bool spent5=credit_spend(e,w,0,400.f);
    ok("la transaction non finançable retourne false", !spent5);
    ok("le refus est ATOMIQUE : le trésor reste inchangé", fabs(econ_country_gold(e,0)-cash5)<0.001);
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
    e->nat_treasury[P] += 600.f;   /* redote le joueur (l'intérêt a de quoi se payer) */
    double p_before  = econ_country_gold(e,P);
    double len_before= econ_country_gold(e,credit_of(P));
    credit_year_tick(e, wl, w);
    econ_aggregate_regions(e);
    ok("l'intérêt CREUSE encore le joueur", econ_country_gold(e,P) < p_before);
    ok("l'intérêt CRÉDITE le prêteur du joueur", econ_country_gold(e,credit_of(P)) > len_before);

    /* ═══════════════════════════════════════════════════════════════════════════════
     * MONNAIE M11 — LA VAGUE AUDIT-SOL : A4, les 5 contrôles qui manquaient. Chaque
     * nouveau contrôle est écrit pour ÉCHOUER sur pre-m11 et PASSER sur HEAD (prouvé
     * séparément par le rapport de mission, pas ici). */

    /* — 8. A2 (ré-écrit 2026-09-03) : l'ancien contrôle vérifiait que le MIROIR
     * prov[cap].treasury == region[].treasury tenait sans ré-agrégation manuelle. Le miroir
     * n'existe plus — il n'y a QU'UN livre d'or, celui du pays. L'intention survit telle
     * quelle : après credit_year_tick, la vérité doit tenir SEULE, sans qu'aucun appelant
     * n'ait à ré-agréger quoi que ce soit pour la voir. On le prouve en lisant l'or des DEUX
     * pays par l'unique porte publique (econ_country_gold), sans une seule ré-agrégation. — */
    printf("\n── 8. A2 : la caisse NATIONALE bouge SEULE (aucune ré-agrégation) ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    credit_spend(e,w,0,400.f); econ_aggregate_regions(e);    /* SETUP (motif test 2) : établit un créancier */
    e->nat_treasury[0] = 600.f;                              /* SETUP (motif test 3) : redote le débiteur */
    { double deb8=econ_country_gold(e,0), cre8=econ_country_gold(e,1);
      credit_year_tick(e, wl, w);   /* AUCUNE ré-agrégation manuelle après CET appel */
      ok("le trésor du DÉBITEUR baisse (intérêt payé) SANS ré-agrégation manuelle",
         econ_country_gold(e,0) < deb8);
      ok("le trésor du CRÉANCIER monte (intérêt reçu) SANS ré-agrégation manuelle",
         econ_country_gold(e,1) > cre8); }

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

    e->nat_treasury[0] = 0.f;
    e->nat_treasury[1] = 500.f;   /* prêteur AU PLANCHER : aucun refinancement */
    float debt_before9c = credit_debt_total(0);
    for (int yr9=0; yr9<10; yr9++) credit_year_tick(e, wl, w);
    ok("DEBT_FIXED=1 : 10 ans d'échéances TOTALEMENT impayées NE FONT PAS grossir la dette (fixe veut dire fixe)",
       credit_debt_total(0) <= debt_before9c + 1e-3f);

    /* — 10. Une échéance est roulée tant que le créancier accepte ; une fois son exposition
     * saturée, l'impayé devient un vrai défaut et nourrit la banqueroute. — */
    printf("\n── 10. Refinancement ouvert, puis rationnement et défaut ──\n");
    credit_init(); setup(e, 100.f, 40000.f); tune_set("DEBT_FIXED",1.f);
    credit_borrow_citystate(e,w,0,1000.f); econ_aggregate_regions(e);
    e->nat_treasury[0]=0.f;
    float debt_before_roll=credit_debt_total(0);
    credit_year_tick(e,wl,w);
    ok("une échéance sans trésor est REFINANCÉE tant que le créancier a de la marge",
       credit_insolvent_streak(0)==0);
    ok("le rollover a un coût : la créance augmente seulement du markup du nouveau contrat",
       credit_debt_total(0)>debt_before_roll);

    /* Remplit la concentration du créancier sur ce débiteur, sans plafond dette/revenu. */
    for (int b10=0;b10<30 && credit_state_borrow_capacity(e,0,1)>1.f;b10++)
        credit_borrow_citystate(e,w,0,1.0e9f);
    econ_aggregate_regions(e);
    e->nat_treasury[0]=0.f;
    printf("   dette=%.0f · revenu=%.0f · ratio=%.2fx · marge prêteur=%.2f\n",
           credit_debt_total(0),credit_annual_revenue(0),credit_debt_ratio(0),
           credit_state_borrow_capacity(e,0,1));
    ok("la dette peut dépasser 300%% du revenu : aucun mur côté débiteur",
       credit_debt_ratio(0)>3.f);
    ok("le créancier ferme le robinet lorsque SON exposition au débiteur est pleine",
       credit_state_borrow_capacity(e,0,1)<=1.f);
    ok("la dette SUBSTANTIELLE dépasse le plancher qui exclut les résidus triviaux",
       credit_debt_total(0)>tune_f("DEBT_DEFAULT_THRESHOLD",3000.f));
    bool forced_fixed=false; int yr_forced=-1;
    for (int yr10=0; yr10<20 && !forced_fixed; yr10++){
        credit_year_tick(e, wl, w);
        if (credit_bankrupt_pending(0)){ forced_fixed=true; yr_forced=yr10+1; }
    }
    printf("   banqueroute forcée %s (streak=%d, dette finale=%.0f, ratio=%.2fx)\n",
           forced_fixed?"DÉCLENCHÉE":"jamais déclenchée", credit_insolvent_streak(0),
           credit_debt_total(0), credit_debt_ratio(0));
    if (forced_fixed) printf("      → à l'an %d après fermeture du marché\n", yr_forced);
    ok("des échéances encore impayées APRÈS refinancement déclenchent la banqueroute forcée",
       forced_fixed);

    /* — le PLANCHER « dette qui compte » (DEBT_DEFAULT_THRESHOLD) : un résidu TRIVIAL, jamais
     * remboursable (même trésor 0 à vie), NE fait PAS faillite — seule une dette substantielle
     * l'engage. Calibrage (sweep {9,11,42}×3×250, 4 points) : sans ce plancher, Σ banqueroutes
     * explosait de 583 à ~1950 QUELLE QUE SOIT DEBT_DUE_FRAC (n'importe quel résidu comptait) —
     * à 3000 (retenu), Σ 795 (+36 %, sous le doublement) ET invariant 0/9. — */
    credit_init(); setup(e, 100.f, 5000.f);
    tune_set("DEBT_FIXED", 1.f);
    credit_borrow_citystate(e,w,0,50.f); econ_aggregate_regions(e);   /* trivial : bien SOUS le plancher */
    e->nat_treasury[0] = 0.f; e->nat_treasury[1]=500.f;
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
    double cs_treas_before11 = econ_country_gold(e,1);
    credit_year_tick(e, wl, w);
    ok("LA SAISIE post-faillite règle la part cité-état au créancier figé (motif M3g)",
       econ_country_gold(e,1) > cs_treas_before11 + 29.0);
    ok("le règlement de la saisie ne laisse AUCUN reliquat (pending RAZ)", credit_garnish_cs_pending(0)==0.f);

    /* — 12. Banqueroute VOLONTAIRE (CMD_BANKRUPTCY, joueur) — repartie à zéro. — */
    printf("\n── 12. Banqueroute volontaire ──\n");
    credit_init(); setup(e, 100.f, 5000.f);
    /* GUÉRIR la cicatrice du §11 : setup() ne l'efface pas, et LA MÉMOIRE DU PRÊTEUR (§20)
     * refuserait sinon l'emprunt qui fait naître la dette de CE banc. */
    e->prov[0].bankruptcy_scar=0.f; e->prov[1].bankruptcy_scar=0.f;
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
    tune_set("MINT_ALLOY", 0.f);   /* ce banc teste A1 PER-MÉTAL (or seul, aucun cuivre en fixture) —
                                    * l'alliage a son banc DÉDIÉ (§19) ; sous MINT_ALLOY=1 la frappe
                                    * min(or,cuivre)=0 rendrait ces assertions inertes. */
    memset(e, 0, sizeof(WorldEconomy));
    credit_init();
    e->n_prov=1; e->n_regions=1;
    e->prov[0].owner=0; e->prov[0].region=0;
    e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
    e->region_rep_prov[0]=0;
    e->nat_treasury[0]=500.f;             /* == SINK_FLOOR : la redépense I3bis (hors scope) reste nulle */
    e->reserve_gold[0]=120.f;             /* réserve d'État (royalty en nature) : frappe ROYALE, DÉJÀ à parité pleine */
    e->nat_stock[0][RES_GOLD]=1000.f;     /* marché privé : frappe LIBRE (A1) — l'entrepôt est NATIONAL */
    e->prov[0].price[RES_GOLD]=8.f;       /* < MINT_PARITY_GOLD (16) : arbitrage positif */
    econ_aggregate_regions(e);
    econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();   /* revenu annuel plausible (motif setup()) */

    double m_before13 = econ_country_gold(e,0);
    for (int k13=0;k13<CLASS_COUNT;k13++) m_before13 += (double)e->prov[0].strata[k13].wealth;
    float reserve_before13 = e->reserve_gold[0];
    float stock_before13   = econ_country_stock_sum(e,0,RES_GOLD);

    tune_set("MINT_FULL_PARITY", 1.f);
    econ_tick(e, 1.f/12.f);

    /* A2 ré-écrit (2026-09-03) : plus de miroir prov/région à comparer — la frappe doit
     * atterrir DIRECTEMENT dans LE livre d'or du pays, visible sans aucune ré-agrégation. */
    ok("la frappe crédite LE trésor national, visible SANS ré-agrégation manuelle (A2)",
       fabs(econ_country_gold(e,0) - 500.0) > 1e-2);
    ok("la réserve d'État a été prélevée (frappe royale, § M2)", e->reserve_gold[0] < reserve_before13);
    ok("le stock de marché a RÉELLEMENT diminué (le métal quitte le marché, frappe libre A1)",
       econ_country_stock_sum(e,0,RES_GOLD) < stock_before13);
    double wealth_after13=0.0; for (int k13=0;k13<CLASS_COUNT;k13++) wealth_after13 += (double)e->prov[0].strata[k13].wealth;
    ok("A1 : un vendeur RÉEL a été payé pour son métal (richesse des classes > 0)", wealth_after13 > 1.0);

    double m_after13 = econ_country_gold(e,0) + wealth_after13;
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
    e->nat_treasury[0]=500.f;
    e->nat_stock[0][RES_GOLD]=1000.f;
    e->prov[0].price[RES_GOLD]=8.f;
    econ_aggregate_regions(e);
    econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
    float stock_legacy_before13 = econ_country_stock_sum(e,0,RES_GOLD);
    tune_set("MINT_FULL_PARITY", 0.f);
    econ_tick(e, 1.f/12.f);
    double wealth_legacy13=0.0; for (int k13=0;k13<CLASS_COUNT;k13++) wealth_legacy13 += (double)e->prov[0].strata[k13].wealth;
    printf("   MINT_FULL_PARITY=0 (legacy) : stock %.1f→%.1f (métal disparu) · richesse classes=%.2f (jamais payé) · FX_MINT=%.1f\n",
           stock_legacy_before13, econ_country_stock_sum(e,0,RES_GOLD), wealth_legacy13, (double)econ_flux_get(0, FX_MINT));
    ok("MINT_FULL_PARITY=0 (legacy pré-M11) : reproduit le bug de l'audit — AUCUN vendeur payé",
       wealth_legacy13 < 1e-3);
    ok("MINT_FULL_PARITY=0 (legacy pré-M11) : le métal disparaît quand même du marché (le trou de l'audit)",
       econ_country_stock_sum(e,0,RES_GOLD) < stock_legacy_before13);
    tune_set("MINT_FULL_PARITY", 1.f);   /* redéfinit le défaut */
    tune_set("MINT_ALLOY", 1.f);         /* redéfinit le défaut (l'alliage, banc dédié §19) */

    /* ── 14. B2 : LE TRÉSOR FANTÔME (econ_nation_gold_add) + LE TOCTOU can_spend/spend ── */
    printf("\n── 14. B2 : le trésor fantôme + le TOCTOU can_spend/spend ──\n");
    {
        /* B2(a) — la porte d'or de l'État (econ_region_treasury_add jusqu'au 2026-09-03,
         * econ_nation_gold_add depuis — même contrat, une seule caisse) ne force plus un
         * résidu non couvert en dette FANTÔME (trésor négatif hors CountryDebt : sans
         * intérêt, créancier, plafond ni banqueroute) : le débit est CLAMPÉ au trésor
         * réellement disponible, et la fonction retourne le montant RÉELLEMENT pris (pas
         * la demande nominale). La dette RÉELLE, elle, a sa porte à part
         * (econ_nation_gold_force, réservée au crédit). */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
        e->nat_treasury[0]=0.f;
        e->region_rep_prov[0]=0;
        econ_aggregate_regions(e);
        float paidB2a = econ_nation_gold_add(e, 0, -300.f);
        ok("B2(a) : un empire SANS trésor ne peut RIEN payer (paid==0, pas de résidu fantôme)",
           paidB2a==0.f);
        ok("B2(a) : le trésor national ne descend JAMAIS sous zéro par ce chemin",
           econ_country_gold(e,0) >= 0.0);
        e->nat_treasury[0]=100.f;
        float paidB2a2 = econ_nation_gold_add(e, 0, -300.f);
        printf("   empire à 100 or, débit demandé 300 → payé RÉELLEMENT %.1f (clampé, pas 300)\n",
               (double)-paidB2a2);
        ok("B2(a) : un débit PARTIEL rend EXACTEMENT ce qui existait (100), jamais plus",
           fabs(-paidB2a2 - 100.f) < 0.01f);
        ok("B2(a) : le trésor est à ZÉRO, jamais négatif (aucune dette fantôme introduite)",
           econ_country_gold(e,0) >= -0.001 && econ_country_gold(e,0) < 0.01);

        /* B2(b) — LE TOCTOU can_spend/spend : can_spend lisait une VUE (l'or agrégé) que
         * credit_spend ne mettait pas à jour — deux dépenses successives dans le même mois
         * voyaient TOUTES DEUX l'état AVANT la première (stale) et pouvaient donc être
         * TOUTES DEUX autorisées. Le trésor national (2026-09-03) supprime la vue : il n'y
         * a plus qu'un livre. Le contrôle reste — c'est lui qui l'atteste. */
        setup(e, 100.f, 5000.f); credit_init();
        float room0 = credit_line(w,e,0);
        printf("   ligne de crédit = %.0f\n", (double)room0);
        ok("B2(b) setup : une grosse dépense (550, proche de la ligne) est autorisée",
           credit_can_spend(e,w,0,550.f));
        credit_spend(e,w,0,550.f);
        double gold_after1 = econ_country_gold(e,0);
        printf("   après la 1re dépense (550) : or nation = %.1f, dette = %.1f\n",
               gold_after1, (double)credit_debt_total(0));
        ok("B2(b) : le trésor NET ne passe jamais négatif (financé par la chaîne)",
           gold_after1 >= -0.5);
        float room_after=credit_line(w,e,0);
        float second_cost=room_after+100.f;
        bool second_ok = credit_can_spend(e,w,0,second_cost);
        printf("   une 2e dépense (%.0f, au-delà de la marge RESTANTE %.0f) : %s\n",
               (double)second_cost,(double)room_after,second_ok?"autorisée":"REFUSÉE");
        ok("B2(b) : la 2e dépense reflète le solde À JOUR (pas la vue périmée d'avant la 1re)",
           !second_ok);

        /* B2(b) — CHAÎNE D'EMPRUNT ÉPUISÉE : aucun paiement partiel. */
        setup(e, 50.f, 50.f);   /* cité-état PAUVRE aussi : la chaîne s'épuise tout de suite */
        credit_init();
        bool huge_spent=credit_spend(e,w,0,100000.f);   /* bien au-delà de TOUTE capacité de la chaîne */
        double gold_final = econ_country_gold(e,0);
        printf("   dépense DÉMESURÉE (100000) refusée : or national final = %.2f\n", gold_final);
        ok("B2(b) : chaîne épuisée ⇒ transaction REFUSÉE", !huge_spent);
        ok("B2(b) : le refus ne prélève PAS même les 50 pièces disponibles",
           fabs(gold_final-50.0)<0.01);

        /* B2(c) — L'OR NATIONAL EST LE SEUL PLAFOND. Le contrôle d'origine plantait deux
         * provinces SŒURS (100 et 1000) et prouvait que la péréquation interne ne fabriquait
         * pas de couverture : l'ancien credit_spend comptait la caisse de la sœur riche comme
         * une ressource NEUVE et laissait le total national négatif. Depuis le TRÉSOR NATIONAL
         * (2026-09-03) la péréquation n'existe plus — il n'y a qu'une caisse (1100, le même
         * total qu'avant) — mais l'invariant testé est LE MÊME et reste indispensable : une
         * dépense au-delà de l'or national + de la chaîne d'emprunt est REFUSÉE, atomiquement,
         * sans jamais laisser l'or du pays passer sous zéro. La fixture garde ses deux
         * provinces (l'empire est bien étendu) ; seule la caisse est unique. */
        memset(e,0,sizeof(*e)); credit_init();
        w->n_provinces=3; e->n_prov=3; e->n_regions=3;
        w->province[0].region=0; w->province[1].region=1; w->province[2].region=2;
        e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=e->prov[0].colonized=true;
        e->prov[1].owner=0; e->prov[1].region=1; e->prov[1].active=e->prov[1].colonized=true;
        e->prov[2].owner=1; e->prov[2].region=2; e->prov[2].active=e->prov[2].colonized=true;
        e->nat_treasury[0]=1100.f;   /* LE trésor de l'empire (l'ancien 100 + 1000 des deux sœurs) */
        e->nat_treasury[1]=0.f;      /* la cité-état est à sec : aucune chaîne d'emprunt */
        e->prov[0].strata[CLASS_LABORER].pop=4000.f;
        e->region_rep_prov[0]=0; e->region_rep_prov[1]=1; e->region_rep_prov[2]=2;
        econ_aggregate_regions(e);
        econ_flux_add(0,FX_TAX,3000.f); econ_flux_year_capture();
        ok("B2(c) : une dépense au-delà de l'or NATIONAL (et sans prêteur) n'est PAS finançable",
           !credit_can_spend(e,w,0,1400.f));
        bool perq_spent=credit_spend(e,w,0,1400.f);
        ok("B2(c) : la dépense non couverte est refusée", !perq_spent);
        ok("B2(c) : refus atomique, le trésor national est inchangé (1100)",
           fabs(econ_country_gold(e,0)-1100.0)<0.01);
        w->n_provinces=2;
    }

    /* ── 15. B3 : L'AMORTISSEMENT SUR DETTE PÉRIMÉE — conservation Σ reçus == Σ payés ── */
    printf("\n── 15. B3 : amortissement sur dette périmée — conservation Σ reçus == Σ payés ──\n");
    {
        /* CLAIM : `debt_total` (credit_year_tick) est capturé AVANT l'échéance ; l'échéance
         * réduit g_debt[c].to_class/to_cs MAIS PAS `debt_total` lui-même — l'amortissement,
         * juste après, répartissait `repay` avec ce dénominateur PÉRIMÉ : dette 100 → échéance
         * 10 → to_class=90 → amortissement 10 réparti 10×90/100=9 aux classes + 1 à la branche
         * cité-état MÊME SANS dette étrangère (jamais crédité, cs_id=-1) — le débiteur paie 10,
         * les créanciers reçoivent 9 : 1 détruit. Fixture : dette 100 % CLASSES (aucune
         * cité-état), trésor GRAS (surplus des DEUX seuils SINK_FLOOR/COURT_FLOOR) pour que
         * échéance ET amortissement mordent la MÊME année — INCOME_TAX=0 isole la conservation
         * treasury<->wealth (sans une 3e case fiscale à additionner). */
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=1; w->n_provinces=1;
        w->country[0].role=POLITY_PLAYER; w->country[0].capital_prov=0;
        w->province[0].region=0;
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
        e->prov[0].is_capital=true;
        e->prov[0].strata[CLASS_LABORER].pop=1000.f;
        e->prov[0].strata[CLASS_ELITE].wealth=100000.f;     /* capacité de prêt large */
        e->prov[0].strata[CLASS_BOURGEOIS].wealth=100000.f;
        e->nat_treasury[0]=60000.f;                          /* GRAS : surplus au-dessus de COURT_FLOOR aussi */
        e->region_rep_prov[0]=0;
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 30000.f); econ_flux_year_capture();   /* plafond de dette large */
        tune_set("INCOME_TAX", 0.f);
        credit_init();
        float borrowed = credit_borrow_class(e, 0, CLASS_ELITE, 1000.f);
        econ_aggregate_regions(e);
        printf("   emprunt accordé=%.1f · dette inscrite=%.1f (classes=%.1f, cité-état=%.1f)\n",
               (double)borrowed, (double)credit_debt_total(0), (double)credit_debt_class(0),
               (double)credit_debt_citystate(0));
        ok("setup B3 : l'emprunt est bien accordé (capacité large)", borrowed>900.f);
        ok("setup B3 : la dette est 100% CLASSES (aucune cité-état créancière)",
           credit_debt_citystate(0)==0.f && credit_debt_class(0)>0.f);

        double treas_before  = econ_country_gold(e,0);
        double wealth_before = (double)e->prov[0].strata[CLASS_ELITE].wealth
                              + (double)e->prov[0].strata[CLASS_BOURGEOIS].wealth;
        credit_year_tick(e, wl, w);
        double treas_after  = econ_country_gold(e,0);
        double wealth_after = (double)e->prov[0].strata[CLASS_ELITE].wealth
                             + (double)e->prov[0].strata[CLASS_BOURGEOIS].wealth;
        double treas_lost    = treas_before - treas_after;
        double wealth_gained = wealth_after - wealth_before;
        printf("   après échéance+amortissement : trésor perdu=%.3f, richesse classes gagnée=%.3f (écart=%.4f)\n",
               treas_lost, wealth_gained, treas_lost-wealth_gained);
        ok("B3 : le débiteur a RÉELLEMENT payé (échéance+amortissement ont mordu le trésor)",
           treas_lost > 1.0);
        ok("B3 : CONSERVATION EXACTE — Σ reçu par les créanciers == Σ payé par le débiteur (0 destruction)",
           fabs(treas_lost - wealth_gained) < 0.05);
        tune_set("INCOME_TAX", 1.f);   /* redéfinit le défaut */
    }

    /* ── 16. Aucun plafond débiteur : la dette va au-delà de 3x le revenu, puis l'ordre
     * ferme le marché sur SA limite d'exposition. ── */
    printf("\n── 16. Dette sans plafond, bornée par l'exposition du prêteur ──\n");
    {
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=1; w->n_provinces=1;
        w->country[0].role=POLITY_PLAYER; w->country[0].capital_prov=0;
        w->province[0].region=0;
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
        e->prov[0].is_capital=true;
        e->prov[0].strata[CLASS_LABORER].pop=1000.f;
        e->prov[0].strata[CLASS_ELITE].wealth=1.0e8f;
        e->nat_treasury[0]=100.f;
        e->region_rep_prov[0]=0;
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 3000.f); econ_flux_year_capture();
        credit_init();
        float rate_start=credit_current_rate(0);
        for (int i=0;i<60;i++){
            float got = credit_borrow_class(e, 0, CLASS_ELITE, -1.f);   /* -1 ⇒ le MAXIMUM disponible */
            if (got<=0.f) break;   /* capacité épuisée : plus rien à emprunter */
        }
        printf("   dette finale=%.0f · revenu=%.0f · ratio=%.1fx · taux %.1f%% → %.1f%% · marge ordre=%.2f\n",
               (double)credit_debt_total(0),(double)credit_annual_revenue(0),(double)credit_debt_ratio(0),
               (double)(rate_start*100.f),(double)(credit_current_rate(0)*100.f),
               (double)credit_class_borrow_capacity(e,0,CLASS_ELITE));
        ok("la dette franchit largement l'ancien mur de 300%%",credit_debt_ratio(0)>3.f);
        ok("le taux monte fortement avec le ratio dette/revenu",credit_current_rate(0)>rate_start+0.10f);
        ok("l'ordre rationne ensuite le débiteur parce que SON exposition est pleine",
           credit_class_borrow_capacity(e,0,CLASS_ELITE)<=1.f);
    }

    /* ── 17. B5 : LA VENTILATION PAR ORDRE — un emprunt 100% bourgeois REMBOURSE le bourgeois ── */
    printf("\n── 17. B5 : la ventilation par ordre — un emprunt 100%% bourgeois ne paie PLUS l'élite ──\n");
    {
        /* CLAIM : l'emprunt V1 (credit_borrow_class) agrégeait tout dans un SEUL to_class,
         * remboursé aux poids FIXES ELITE/BOURGEOIS_LEND_WEIGHT (1.0/0.5) — un emprunt
         * 100% BOURGEOIS (l'élite n'a RIEN prêté) remboursait quand même l'élite à hauteur
         * de son poids fixe (ew/(ew+bw)=67%) : une créance FANTÔME versée à un non-prêteur. */
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=1; w->n_provinces=1;
        w->country[0].role=POLITY_PLAYER; w->country[0].capital_prov=0;
        w->province[0].region=0;
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0; e->prov[0].active=true; e->prov[0].colonized=true;
        e->prov[0].is_capital=true;
        e->prov[0].strata[CLASS_LABORER].pop=1000.f;
        e->prov[0].strata[CLASS_ELITE].wealth=100000.f;      /* présente, mais ne PRÊTE JAMAIS */
        e->prov[0].strata[CLASS_BOURGEOIS].wealth=100000.f;  /* LA SEULE prêteuse */
        e->nat_treasury[0]=60000.f;
        e->region_rep_prov[0]=0;
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 30000.f); econ_flux_year_capture();
        tune_set("INCOME_TAX", 0.f);
        credit_init();
        float borrowed = credit_borrow_class(e, 0, CLASS_BOURGEOIS, 1000.f);   /* 100% BOURGEOIS */
        ok("B5/A2 : le débit de richesse reste cohérent prov==region SANS ré-agrégation",
           fabsf(e->prov[0].strata[CLASS_BOURGEOIS].wealth
                -e->region[0].strata[CLASS_BOURGEOIS].wealth)<0.01f);
        econ_aggregate_regions(e);
        printf("   emprunt 100%% bourgeois=%.1f → ventilation : élite=%.1f, bourgeois=%.1f\n",
               (double)borrowed, (double)credit_debt_elite(0), (double)credit_debt_bourgeois(0));
        ok("B5 setup : l'emprunt est accordé", borrowed>900.f);
        ok("B5 : la créance est ventilée SUR LE VRAI PRÊTEUR (élite=0, bourgeois>0)",
           credit_debt_elite(0)==0.f && credit_debt_bourgeois(0)>0.f);

        float elite_wealth_before = e->prov[0].strata[CLASS_ELITE].wealth;
        float bourg_wealth_before = e->prov[0].strata[CLASS_BOURGEOIS].wealth;
        credit_year_tick(e, wl, w);   /* échéance + amortissement de l'année */
        float elite_wealth_after = e->prov[0].strata[CLASS_ELITE].wealth;
        float bourg_wealth_after = e->prov[0].strata[CLASS_BOURGEOIS].wealth;
        ok("B5/A2 : les remboursements gardent aussi la vue régionale à jour immédiatement",
           fabsf(e->prov[0].strata[CLASS_BOURGEOIS].wealth
                -e->region[0].strata[CLASS_BOURGEOIS].wealth)<0.01f);
        printf("   après échéance+amortissement : élite %.2f→%.2f · bourgeois %.2f→%.2f\n",
               (double)elite_wealth_before, (double)elite_wealth_after,
               (double)bourg_wealth_before, (double)bourg_wealth_after);
        ok("B5 : l'ÉLITE (qui n'a RIEN prêté) ne reçoit AUCUN remboursement (créance fantôme fermée)",
           fabs(elite_wealth_after - elite_wealth_before) < 0.01f);
        ok("B5 : le BOURGEOIS (le VRAI prêteur) est RÉELLEMENT remboursé (richesse en hausse)",
           bourg_wealth_after > bourg_wealth_before + 1.0f);
        ok("B5 : la créance élite reste à ZÉRO après le service (jamais inscrite pour commencer)",
           credit_debt_elite(0)==0.f);
        tune_set("INCOME_TAX", 1.f);   /* redéfinit le défaut */
    }

    /* ── 18. B7 : LA VRAIE ÉCHÉANCE (façade scps_country_debt.due) — mêmes primitives
     * que scps_api.c (credit_debt_total/credit_current_rate/tune_f), sans passer par
     * ScpsSim (organique, sujet aux aléas d'une économie simulée — cf. scps_api_demo.c
     * où ce même calcul est aussi vérifié quand la fixture s'y prête). ── */
    printf("\n── 18. B7 : l'échéance affichée == total×DEBT_DUE_FRAC, PAS total×taux ──\n");
    {
        setup(e, 100.f, 5000.f); credit_init();
        credit_spend(e,w,0,550.f);   /* fait naître une dette réelle (motif §2) */
        float total = credit_debt_total(0), taux = credit_current_rate(0);
        ok("setup B7 : une dette réelle existe", total > 1.f);
        /* la MÊME formule que scps_country_debt (scps_api.c) : reproduite ici pour
         * vérifier la RELATION, jamais une constante dupliquée côté GDScript. */
        bool fixed = tune_f("DEBT_FIXED", 1.0f) > 0.f;
        float due = fixed ? (total * tune_f("DEBT_DUE_FRAC", 0.10f)) : (total * taux);
        float due_bugged = total * taux;   /* l'ANCIEN calcul de budget_panel_v2.gd, le bug */
        printf("   dette=%.1f · taux origination=%.1f%% · échéance RÉELLE=%.1f · ancien calcul buggé=%.1f\n",
               (double)total, (double)(taux*100.f), (double)due, (double)due_bugged);
        ok("B7 : sous DEBT_FIXED (défaut), l'échéance == 10%% du stock (DEBT_DUE_FRAC), pas le taux",
           fixed && fabsf(due - total*0.10f) < 0.5f);
        ok("B7 : l'échéance RÉELLE est SENSIBLEMENT PLUS GRANDE que l'ancien calcul buggé (taux 2-5%%)",
           due > due_bugged * 1.5f);
        tune_set("DEBT_FIXED", 0.f);
        float due_legacy = (tune_f("DEBT_FIXED",1.f)>0.f) ? (total*tune_f("DEBT_DUE_FRAC",0.10f)) : (total*taux);
        ok("B7 kill-switch DEBT_FIXED=0 : legacy, échéance==total×taux EXACT (comportement pré-M11)",
           fabsf(due_legacy - due_bugged) < 0.01f);
        tune_set("DEBT_FIXED", 1.f);   /* redéfinit le défaut */
    }

    /* ── 19. L'ALLIAGE (décision joueur 2026-07-21 : 1 or + 1 cuivre = 32 pièces) ──
     * Frappe ROYALE en paires min(or,cuivre) ; frappe LIBRE en paires (achat des DEUX
     * métaux, arbitrage sur le prix de la paire) ; kill-switch MINT_ALLOY=0 = per-métal. */
    printf("\n── 19. L'alliage : 1 or + 1 cuivre = 32 (loi du minimum) ──\n");
    {
        /* (a) ROYALE — réserve 120 or / 48 cuivre : la paire est bornée par le cuivre. */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f;             /* == SINK_FLOOR (motif §13 : redépense I3bis nulle) */
        e->reserve_gold[0]=120.f; e->reserve_copper[0]=48.f;
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        tune_set("MINT_ALLOY", 1.f);
        econ_tick(e, 1.f/12.f);
        float dg19 = 120.f - e->reserve_gold[0], dc19 = 48.f - e->reserve_copper[0];
        double mint19 = (double)econ_flux_get(0, FX_MINT);
        printf("   royale : or prélevé=%.2f · cuivre prélevé=%.2f · FX_MINT=%.2f (attendu paire×32=%.2f)\n",
               (double)dg19, (double)dc19, mint19, (double)dg19*32.0);
        ok("ALLIAGE royale : les DEUX métaux sont prélevés en PAIRES égales (1:1)",
           dg19 > 0.1f && fabsf(dg19-dc19) < 0.01f);
        ok("ALLIAGE royale : la valeur frappée == paire × MINT_ALLOY_VALUE (32), jamais les parités séparées",
           fabs(mint19 - (double)dg19*32.0) < 0.5);
        ok("ALLIAGE royale : l'excédent d'or (métal abondant) RESTE en réserve (loi du minimum)",
           e->reserve_gold[0] > e->reserve_copper[0] + 60.f);

        /* (b) LIBRE — marché avec les DEUX métaux : achat en paires, les DEUX stocks baissent. */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f;
        e->nat_stock[0][RES_GOLD]=1000.f;   e->prov[0].price[RES_GOLD]=8.f;
        e->nat_stock[0][RES_COPPER]=1000.f; e->prov[0].price[RES_COPPER]=2.f;    /* paire 10 < 32 : arbitrage */
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        econ_tick(e, 1.f/12.f);
        float sg19 = 1000.f - econ_country_stock_sum(e,0,RES_GOLD),
              sc19 = 1000.f - econ_country_stock_sum(e,0,RES_COPPER);
        printf("   libre : or acheté=%.2f · cuivre acheté=%.2f (paires)\n", (double)sg19, (double)sc19);
        ok("ALLIAGE libre : l'achat d'État tire les DEUX métaux du marché en paires égales",
           sg19 > 0.1f && fabsf(sg19-sc19) < 0.5f);

        /* (c) LIBRE, or SEUL au marché : AUCUN achat (pas de cuivre à apparier). */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f;
        e->nat_stock[0][RES_GOLD]=1000.f; e->prov[0].price[RES_GOLD]=8.f;   /* cuivre ABSENT */
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        econ_tick(e, 1.f/12.f);
        ok("ALLIAGE libre : l'or SEUL (sans cuivre à apparier) ne se frappe PAS (aucun achat)",
           econ_country_stock_sum(e,0,RES_GOLD) > 999.f);

        /* (d) kill-switch : MINT_ALLOY=0 reproduit le per-métal EXACT (l'or seul frappe). */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f;
        e->reserve_gold[0]=120.f;             /* aucun cuivre : l'alliage rendrait 0 */
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        tune_set("MINT_ALLOY", 0.f);
        econ_tick(e, 1.f/12.f);
        ok("MINT_ALLOY=0 (kill-switch) : le per-métal legacy frappe l'or SEUL (comportement pré-alliage)",
           e->reserve_gold[0] < 119.f);
        tune_set("MINT_ALLOY", 1.f);   /* redéfinit le défaut */

        /* (e) LE BILLON-SÉCHERESSE (décision joueur 2026-07-21) : réserve 120 or / 0 cuivre
         * — paires MORTES. La débase-sécheresse IA (déséquilibre (max−min)/max = 1) frappe
         * l'or CÉLIBATAIRE à sa vieille parité, et le coût M3h (érosion K_inst) mord. */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f;
        e->prov[0].build.K_inst=10.f;         /* pour MESURER l'érosion (le prix du billon) */
        e->reserve_gold[0]=120.f;             /* aucun cuivre : sécheresse totale */
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        econ_tick(e, 1.f/12.f);
        double bill19 = (double)econ_flux_get(0, FX_MINT);
        printf("   billon : or célibataire frappé=%.2f (réserve 120→%.1f) · K_inst 10→%.3f\n",
               bill19, (double)e->reserve_gold[0], (double)e->prov[0].build.K_inst);
        ok("BILLON : la sécheresse débase — l'or CÉLIBATAIRE frappe (la monnaie survit aux paires mortes)",
           bill19 > 1.0 && e->reserve_gold[0] < 119.f);
        ok("BILLON : le prix du désespoir mord (érosion K_inst, machinerie M3h telle quelle)",
           e->prov[0].build.K_inst < 10.f - 1e-4f);

        /* kill-switch DEBASE_DROUGHT=0 : sans le déclencheur sécheresse, paires mortes = rien. */
        memset(e, 0, sizeof(WorldEconomy));
        e->n_prov=1; e->n_regions=1;
        e->prov[0].owner=0; e->prov[0].region=0;
        e->prov[0].active=true; e->prov[0].colonized=true; e->prov[0].is_capital=true;
        e->region_rep_prov[0]=0;
        e->nat_treasury[0]=500.f; e->reserve_gold[0]=120.f;
        econ_aggregate_regions(e);
        econ_flux_add(0, FX_TAX, 12000.f); econ_flux_year_capture();
        tune_set("DEBASE_DROUGHT", 0.f);
        econ_tick(e, 1.f/12.f);
        ok("DEBASE_DROUGHT=0 (kill-switch) : paires mortes SANS sécheresse ⇒ aucune frappe (l'or attend)",
           econ_flux_get(0, FX_MINT) < 0.5f && e->reserve_gold[0] > 119.f);
        tune_set("DEBASE_DROUGHT", 1.f);   /* redéfinit le défaut */
    }

    /* ── 20. LA MÉMOIRE DU PRÊTEUR (décision joueur 2026-07-21, « Édouard III a tué les
     * Bardi ») : cicatrice de banqueroute VIVANTE ⇒ AUCUNE capacité nulle part (ordres,
     * cités-états, ligne de crédit) ; la décrue de la cicatrice rouvre le marché. ── */
    printf("\n── 20. La mémoire du prêteur : un répudiateur ne trouve plus personne ──\n");
    {
        /* fixture auto-suffisante : le §19 (memset) a laissé n_regions=1 — re-poser le
         * monde à 2 pays/2 régions AVANT setup() (motif §15/§16 qui reposent w->). */
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=2; w->n_provinces=2;
        w->country[0].role=POLITY_PLAYER;     w->country[0].capital_prov=0;
        w->country[1].role=POLITY_CITY_STATE; w->country[1].capital_prov=1;
        w->province[0].region=0; w->province[1].region=1;
        e->n_regions=2;
        setup(e, 100.f, 5000.f); credit_init();
        /* guérir la cicatrice héritée du §12 (setup() ne l'efface pas — même motif). */
        e->prov[0].bankruptcy_scar=0.f; e->prov[1].bankruptcy_scar=0.f;
        credit_spend(e,w,0,550.f); econ_aggregate_regions(e);   /* une dette réelle naît (motif §2) */
        ok("setup 20 : AVANT la banqueroute, le marché du crédit est ouvert",
           credit_line(w,e,0) > 1.f);
        credit_bankruptcy(e, 0, true /* forcée */);
        econ_aggregate_regions(e);
        ok("mémoire : la cicatrice posée, les ORDRES ne prêtent plus (capacité classes == 0)",
           credit_class_borrow_capacity(e,0,CLASS_ELITE)==0.f
           && credit_class_borrow_capacity(e,0,CLASS_BOURGEOIS)==0.f);
        ok("mémoire : les ÉTATS non plus (capacité cité-état == 0)",
           credit_state_borrow_capacity(e,0,1)==0.f);
        ok("mémoire : la ligne de crédit ENTIÈRE est fermée (credit_line == 0)",
           credit_line(w,e,0)==0.f);
        ok("mémoire : un emprunt forcé ne prête RIEN (0 or, pas un refus partiel)",
           credit_borrow_class(e,0,CLASS_ELITE,100.f)==0.f);
        /* Les assertions ci-dessous lisent credit_line (la cité-état riche de setup() — les
         * classes n'ont AUCUNE épargne dans cette fixture, leur capacité est 0 par nature). */
        tune_set("LENDER_MEMORY", 0.f);
        ok("LENDER_MEMORY=0 (kill-switch) : les prêteurs oublient instantanément (marché rouvert)",
           credit_line(w,e,0) > 1.f);
        tune_set("LENDER_MEMORY", 1.f);   /* redéfinit le défaut */
        /* LA DÉCRUE : la cicatrice guérie (~10 ans en jeu ; effacée à la main ici — le banc
         * teste le SEUIL, pas l'horloge), le marché rouvre — l'exclusion EST la cicatrice. */
        int np20 = e->n_prov; if (np20>SCPS_MAX_PROV) np20=SCPS_MAX_PROV;
        for (int p20=0;p20<np20;p20++) if (e->prov[p20].owner==0) e->prov[p20].bankruptcy_scar=0.f;
        ok("mémoire : cicatrice GUÉRIE ⇒ le marché rouvre (l'exclusion suit la cicatrice, aucun timer)",
           credit_line(w,e,0) > 1.f);
    }

    /* ── 21. LA RUINE DU CRÉANCIER (décision joueur 2026-07-21, « la banqueroute va tuer
     * des cités-états/empires prêteurs ») : la répudiation qui anéantit la MAJEURE part du
     * capital du prêteur externe l'effondre (cicatrice) ; un prêteur RICHE encaisse. ── */
    printf("\n── 21. La ruine du créancier : les Bardi meurent, les Fugger encaissent ──\n");
    {
        /* (a) prêteur RICHE : la créance perdue est une fraction mineure de son capital. */
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=2; w->n_provinces=2;
        w->country[0].role=POLITY_PLAYER;     w->country[0].capital_prov=0;
        w->country[1].role=POLITY_CITY_STATE; w->country[1].capital_prov=1;
        w->province[0].region=0; w->province[1].region=1;
        e->n_regions=2;
        setup(e, 100.f, 5000.f); credit_init();
        credit_spend(e,w,0,550.f); econ_aggregate_regions(e);
        ok("setup 21 : la cité-état est créancière (dette externe réelle)", credit_debt_citystate(0) > 100.f);
        long ruins_a = credit_lender_ruins();
        credit_bankruptcy(e, 0, true);
        ok("ruine : un prêteur RICHE encaisse la répudiation (aucune cicatrice — les Fugger)",
           e->prov[1].bankruptcy_scar == 0.f && credit_lender_ruins() == ruins_a);

        /* (b) prêteur APPAUVRI DEPUIS le prêt : son capital restant EST la créance → ruine. */
        e->prov[0].bankruptcy_scar=0.f; e->prov[1].bankruptcy_scar=0.f;   /* guérir (mémoire du prêteur) */
        credit_init(); setup(e, 100.f, 5000.f);
        credit_spend(e,w,0,550.f); econ_aggregate_regions(e);
        e->nat_treasury[1] = 400.f;   /* le prêteur a FONDU depuis (< SINK_FLOOR : surplus 0, capital ≈ la créance) */
        long ruins_b = credit_lender_ruins();
        credit_bankruptcy(e, 0, true);
        ok("ruine : la créance anéantie > moitié du capital ⇒ le prêteur S'EFFONDRE (cicatrice — les Bardi)",
           e->prov[1].bankruptcy_scar > 0.9f);
        ok("ruine : la télémétrie compte le prêteur ruiné", credit_lender_ruins() == ruins_b+1);
        ok("ruine : le ruiné ne peut plus RIEN emprunter lui-même (sa cicatrice le verrouille — lender_locked_out)",
           credit_state_borrow_capacity(e,1,0)==0.f && credit_class_borrow_capacity(e,1,CLASS_ELITE)==0.f);

        /* (c) kill-switch LENDER_RUIN_SHARE=0 : même appauvrissement, aucune ruine. */
        e->prov[0].bankruptcy_scar=0.f; e->prov[1].bankruptcy_scar=0.f;
        credit_init(); setup(e, 100.f, 5000.f);
        credit_spend(e,w,0,550.f); econ_aggregate_regions(e);
        e->nat_treasury[1] = 400.f;
        tune_set("LENDER_RUIN_SHARE", 0.f);
        credit_bankruptcy(e, 0, true);
        ok("LENDER_RUIN_SHARE=0 (kill-switch) : aucun effondrement de prêteur (comportement pré-ruine)",
           e->prov[1].bankruptcy_scar == 0.f);
        tune_set("LENDER_RUIN_SHARE", 0.5f);   /* redéfinit le défaut */
    }

    /* ── 22. LE VERBE « REMBOURSER » (2026-07-21, KoH2 « Repay All ») : le principal
     * fond depuis le surplus, les créanciers ENCAISSENT (conservation stricte),
     * miroir exact de l'amortissement annuel — mais à la main du joueur. ── */
    printf("\n── 22. Rembourser : se désendetter est une décision de joueur ──\n");
    {
        memset(e, 0, sizeof(WorldEconomy));
        w->n_countries=2; w->n_provinces=2;
        w->country[0].role=POLITY_PLAYER;     w->country[0].capital_prov=0;
        w->country[1].role=POLITY_CITY_STATE; w->country[1].capital_prov=1;
        w->province[0].region=0; w->province[1].region=1;
        e->n_regions=2;
        setup(e, 100.f, 5000.f); credit_init();
        e->prov[0].bankruptcy_scar=0.f; e->prov[1].bankruptcy_scar=0.f;   /* fixture neuve */
        credit_spend(e,w,0,550.f); econ_aggregate_regions(e);
        float debt0=credit_debt_total(0);
        ok("setup 22 : une dette réelle existe", debt0 > 100.f);
        /* sans surplus (trésor sous COURT_FLOOR) : rien ne se rembourse — jamais de dette forcée. */
        ok("sans surplus au-dessus du plancher de cour : le verbe rend 0 (jamais un découvert)",
           credit_repay_principal(e,w,0,-1.f)==0.f);
        /* on redote un trésor GRAS puis on rembourse TOUT (amount<=0). */
        e->nat_treasury[0] = 20000.f;
        double cs_before22 = econ_country_gold(e,1);
        float repaid = credit_repay_principal(e,w,0,-1.f);
        econ_aggregate_regions(e);
        printf("   dette %.1f → %.1f (remboursé %.1f) · trésor cité-état %.1f → %.1f\n",
               (double)debt0,(double)credit_debt_total(0),(double)repaid,
               cs_before22, econ_country_gold(e,1));
        ok("REMBOURSER éteint la dette entière quand le surplus le permet",
           repaid > debt0 - 0.5f && credit_debt_total(0) < 0.5f);
        ok("CONSERVATION : le créancier cité-état ENCAISSE réellement sa part",
           econ_country_gold(e,1) > cs_before22 + 1.0);
        ok("le créancier soldé est DÉLIÉ (credit_of == -1)", credit_of(0) < 0);
    }

    printf("\n═══ BILAN : %d réussis, %d échoués ═══\n", g_pass, g_fail);
    free(w); free(e); free(wl);
    return g_fail?1:0;
}
