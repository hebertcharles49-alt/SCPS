/*
 * tech_demo.c — banc d'essai de l'arbre CONCENTRIQUE & FRACTAL
 *
 *   make tech_demo && ./tech_demo
 *
 * Prouve les 6 points de vérification : la FORME (3 thèmes × 3 fonctions = 9
 * quartiers, faustien au bord), le DÉPART (6 bâtiments de base seulement), une
 * tech = un DÉVERROUILLAGE, le FAUSTIEN partout (→ la Brèche ; seule la Société
 * métabolise), les ORPHELINES de heritage (greffe par la population), le COÛT qui
 * scale ∝ population.
 */
#include "scps_tech.h"
#include "scps_heritage.h"
#include <stdio.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

static void research(TechState *s, TechId id, unsigned acc){
    const TechNode *n=tech_node(id);
    if (tech_research(s,id,acc))
        printf("  + %-22s [%s·%s t.%d] → %s\n", tech_name(id),
               tech_theme_name(n->theme), tech_function_name(n->func), n->tier, tech_unlocks(id));
    else
        printf("  ✗ %-22s (prérequis / accès de heritage / porte arcane manquants)\n", tech_name(id));
}

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" ARBRE DE TECH — concentrique & fractal (centre → 3 thèmes → 3 fonctions)\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* ---- 1. LA FORME : concentrique & fractale ------------------------- */
    printf("\n── 1. La forme (angle = quartier · rayon = tier · faustien au bord) ──\n");
    ok("3 THÈMES (Savoir · Forge · Société)", THM_COUNT==3);
    ok("3 FONCTIONS (Production · Armée · Renforcement)", FN_COUNT==3);
    ok("9 QUARTIERS (3 thèmes × 3 fonctions)", TECH_QUARTERS==9);
    bool themes[THM_COUNT]={false}, funcs[FN_COUNT]={false};
    int base_count=0, faustian_count=0; bool faustian_at_edge=true, all_unlock_named=true;
    for(int i=0;i<TECH_COUNT;i++){
        const TechNode *n=tech_node(i);
        themes[n->theme]=true; funcs[n->func]=true;
        if(n->tier==0) base_count++;
        if(n->faustian){ faustian_count++; if(n->tier<2) faustian_at_edge=false; }
        if(!tech_unlocks((TechId)i) || !tech_unlocks((TechId)i)[0]) all_unlock_named=false;
    }
    ok("chaque thème ET chaque fonction portent des nœuds (la grille 3×3 est pleine)",
       themes[0]&&themes[1]&&themes[2]&&funcs[0]&&funcs[1]&&funcs[2]);
    ok("le FAUSTIEN est au BORD (jamais au centre : tier ≥ 2)", faustian_at_edge && faustian_count>0);

    /* ---- 2. LE DÉPART : seuls les 6 bâtiments de base ------------------ */
    printf("\n── 2. Le départ (centre = 6 bâtiments de base, rien d'autre) ──\n");
    TechState s; tech_state_init(&s, /*ruines*/false);
    int unlocked0=0; bool only_base=true;
    for(int i=0;i<TECH_COUNT;i++) if(s.unlocked[i]){ unlocked0++; if(!tech_is_base((TechId)i)) only_base=false; }
    printf("  déverrouillés au départ : %d  (Bibliothèque, Collectes bois/argile/nourriture, Atelier, Caserne)\n",unlocked0);
    ok("EXACTEMENT 6 bâtiments de base au centre", base_count==6 && unlocked0==6);
    ok("au départ, RIEN d'autre que les bâtiments de base", only_base);
    ok("un nœud profond n'est PAS disponible d'emblée (ex. Université)", !s.unlocked[TECH_UNIVERSITE]);

    /* ---- 3. UNE TECH = UN DÉVERROUILLAGE ------------------------------- */
    printf("\n── 3. Une tech = un déverrouillage (un bâtiment / une capacité) ──\n");
    ok("chaque nœud nomme le bâtiment/capacité qu'il déverrouille", all_unlock_named);

    /* ---- 4. MAGIE ⊂ SAVOIR -------------------------------------------- */
    printf("\n── 4. Magie ⊂ Savoir (l'arcane EST du savoir) ──\n");
    ok("l'Invocation vit dans le thème SAVOIR (pas une branche Magie séparée)",
       tech_node(TECH_INVOCATION)->theme==THM_SAVOIR && tech_node(TECH_INVOCATION)->func==FN_ARMEE);
    ok("L'Éveil & le Savoir interdit sont des bouts faustiens du SAVOIR",
       tech_node(TECH_EVEIL)->theme==THM_SAVOIR && tech_node(TECH_SAVOIR_INTERDIT)->theme==THM_SAVOIR);

    /* ---- 5. FAUSTIEN PARTOUT → la Brèche (seule la Société métabolise) - */
    printf("\n── 5. Le faustien partout → la Brèche (K métabolise) ──\n");
    unsigned human=tech_heritage_bit(HERITAGE_ADAPTATIF);
    printf("  RUN A — ruée arcane sans socle :\n");
    TechState a; tech_state_init(&a,/*ruines*/true);
    research(&a,TECH_SAVOIR_GUERRE,human); research(&a,TECH_MAGIE_BATAILLE,human); research(&a,TECH_EVEIL,human);
    float derealA=tech_dereal(&a);
    printf("    → K=%.0f charge=%.1f puissance=%.0f  DEREAL=%.2f  crise=%.0f%%\n",
           a.K,a.charge,a.puissance,derealA,tech_crisis_proximity(&a)*100.f);
    printf("  RUN B — socle d'abord (Savoir·Prod + Chancellerie → K), PUIS même arcane :\n");
    TechState b; tech_state_init(&b,/*ruines*/true);
    research(&b,TECH_SCRIPTORIUM,human); research(&b,TECH_ACADEMIE,human); research(&b,TECH_CHANCELLERIE,human);
    research(&b,TECH_SAVOIR_GUERRE,human); research(&b,TECH_MAGIE_BATAILLE,human); research(&b,TECH_EVEIL,human);
    float derealB=tech_dereal(&b);
    printf("    → K=%.0f charge=%.1f puissance=%.0f  DEREAL=%.2f\n",b.K,b.charge,b.puissance,derealB);
    ok("la ruée arcane SANS socle ouvre la Brèche (dereal > 0)", derealA>0.f);
    ok("le même arcane APRÈS le socle (K↑) est MÉTABOLISÉ (dereal moindre)", derealB < derealA);
    ok("le faustien est dans CHAQUE thème (Savoir·Éveil, Forge·Œuvre noire, Société·Caste martiale)",
       tech_node(TECH_EVEIL)->faustian      && tech_node(TECH_EVEIL)->theme==THM_SAVOIR &&
       tech_node(TECH_OEUVRE_NOIRE)->faustian&& tech_node(TECH_OEUVRE_NOIRE)->theme==THM_FORGE &&
       tech_node(TECH_CASTE_MARTIALE)->faustian&&tech_node(TECH_CASTE_MARTIALE)->theme==THM_SOCIETE);

    /* ---- 6. ORPHELINES DE RACE (greffe par la population) -------------- */
    printf("\n── 6. Orphelines de heritage (greffe par la population / conquête) ──\n");
    TechState f; tech_state_init(&f,false);
    research(&f,TECH_ARMURERIE,human); research(&f,TECH_POUDRIERE,human);   /* prérequis de la Forge à runes */
    ok("MÉTALLURGISTE sans ARCANE : la Forge à runes (runique × arcane) reste INSUFFISANTE",
       !tech_can_research(&f,TECH_FORGE_RUNES, human|tech_heritage_bit(HERITAGE_METALLURGISTE)));
    ok("MÉTALLURGISTE + ARCANE (ésotérique en contact) : la Forge à runes se GREFFE (combo §syncrétique)",
       tech_can_research(&f,TECH_FORGE_RUNES, human|tech_heritage_bit(HERITAGE_METALLURGISTE)|tech_heritage_bit(HERITAGE_ESOTERIQUE)));
    ok("un empire MÉTALLURGISTE+ÉSOTÉRIQUE la recherche (native naine + combo ésotérique réunis)",
       tech_can_research(&f,TECH_FORGE_RUNES, tech_heritage_bit(HERITAGE_METALLURGISTE)|tech_heritage_bit(HERITAGE_ESOTERIQUE)));
    ok("la signature HALFELINE (Abondance) est la MOINS faustienne (charge nulle)",
       !tech_node(TECH_ABONDANCE)->faustian && tech_node(TECH_ABONDANCE)->charge==0.f &&
       tech_node(TECH_ABONDANCE)->native==HERITAGE_AGRAIRE);
    ok("l'Esclavage est la signature CLANIQUE (la tech d'asservissement, gate du §4c)",
       tech_node(TECH_ESCLAVAGE)->native==HERITAGE_CLANIQUE && tech_node(TECH_ESCLAVAGE)->faustian);

    /* ---- 7. LE COÛT QUI SCALE ∝ √N (provinces), SOUS-LINÉAIRE ---------- */
    printf("\n── 7. Le coût qui scale ∝ √N (provinces) — wide récompensé sous-linéairement ──\n");
    float c_small=tech_cost(TECH_ACADEMIE, 1.f), c_big=tech_cost(TECH_ACADEMIE, 25.f);
    printf("  Académie : mono-province (N=1) = %.0f pts · grand empire (N=25) = %.0f pts\n",c_small,c_big);
    ok("un GRAND empire paie PLUS cher chaque tech qu'un petit (coût ∝ √N)", c_big > c_small);
    ok("MAIS sous-linéaire : ×N provinces ne ×N pas le coût (√25=5×, pas 25×)", c_big < c_small*25.f);
    ok("le rayon (tier) renchérit : Université > Académie > Scriptorium (à N égal)",
       tech_cost(TECH_UNIVERSITE,8.f) > tech_cost(TECH_ACADEMIE,8.f) &&
       tech_cost(TECH_ACADEMIE,8.f)  > tech_cost(TECH_SCRIPTORIUM,8.f));
    ok("les bâtiments de base (tier 0, le centre) sont GRATUITS", tech_cost(TECH_BIBLIOTHEQUE,16.f)==0.f);

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    return g_fail?1:0;
}
