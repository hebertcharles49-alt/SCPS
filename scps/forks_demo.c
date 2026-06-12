/*
 * forks_demo.c — banc des FOURCHES (arc M, design v3 docs/DESIGN_FORKS.md)
 *
 *   make forks_demo && ./forks_demo
 *
 * Les assertions §24, au fil des items livrés :
 *   M1 — ETHOS_FN : l'argmax de chaque éthos (Dominateur→ARMÉE,
 *        Bureaucrate→RENFORCEMENT, Mercantile→PRODUCTION).
 *   M2 — le pôle d'une région ÉMERGE des poids de factions (Transgresseur exclu,
 *        tie-breaks §7 : capitale→impérial, port→Fluide, frontière→Martial, sinon Ordre).
 *   (M3/M5 edifice_succ_ctx + hystérésis · M6 alambic/flux : asserts ajoutés à la livraison.)
 */
#include "scps_ai.h"
#include "scps_factions.h"
#include <stdio.h>
#include <string.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LES FOURCHES — éthos→fonction, factions→pôle (design v3)\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* ═══ M1 — ETHOS_FN : l'éthos pèse la FONCTION (argmax §24) ═══════════ */
    printf("\n── M1. L'éthos préfère SA fonction (la table §6, verbatim) ──\n");
    ok("le Dominateur préfère l'ARMÉE",        ai_ethos_pref_func(ETHOS_DOMINATEUR) ==FN_ARMEE);
    ok("l'Honneur préfère l'ARMÉE",            ai_ethos_pref_func(ETHOS_HONNEUR)    ==FN_ARMEE);
    ok("l'Ordre préfère le RENFORCEMENT",      ai_ethos_pref_func(ETHOS_ORDRE)      ==FN_RENFORCEMENT);
    ok("le Bureaucrate préfère le RENFORCEMENT",ai_ethos_pref_func(ETHOS_BUREAUCRATE)==FN_RENFORCEMENT);
    ok("le Mercantile préfère la PRODUCTION",  ai_ethos_pref_func(ETHOS_MERCANTILE) ==FN_PRODUCTION);
    ok("le Pacifiste préfère la PRODUCTION",   ai_ethos_pref_func(ETHOS_PACIFISTE)  ==FN_PRODUCTION);

    /* ═══ M2 — LE PÔLE émerge des factions (§7) ═══════════════════════════ */
    printf("\n── M2. Le pôle d'une région : les factions votent, les tie-breaks tranchent ──\n");
    {
        float wgt[FAC_COUNT];
        memset(wgt,0,sizeof wgt); wgt[FAC_CONQUERANT]=0.6f; wgt[FAC_MARCHAND]=0.2f;
        ok("Conquérant dominant → pôle MARTIAL", faction_pole_of(wgt,-1,false,false)==POLE_MARTIAL);
        memset(wgt,0,sizeof wgt); wgt[FAC_LEGISTE]=0.5f; wgt[FAC_CONQUERANT]=0.2f;
        ok("Légiste dominant → pôle ORDRE",      faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        memset(wgt,0,sizeof wgt); wgt[FAC_MARCHAND]=0.5f; wgt[FAC_GARDIEN]=0.2f;
        ok("Marchand dominant → pôle FLUIDE",    faction_pole_of(wgt,-1,false,false)==POLE_FLUIDE);
        /* le Transgresseur est ORTHOGONAL : il ne pousse AUCUN pôle (il nourrit
         * l'appétit faustien). Une région tout-Transgresseur tombe aux tie-breaks. */
        memset(wgt,0,sizeof wgt); wgt[FAC_TRANSGRESSEUR]=0.9f;
        ok("Transgresseur seul → AUCUN pôle poussé (tie-break : sinon → Ordre)",
           faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        /* tie-breaks §7 dans l'ordre, sur une égalité parfaite martial=ordre=fluide. */
        memset(wgt,0,sizeof wgt); wgt[FAC_CONQUERANT]=0.3f; wgt[FAC_LEGISTE]=0.3f; wgt[FAC_MARCHAND]=0.3f;
        ok("égalité + capitale → le pôle IMPÉRIAL", faction_pole_of(wgt,POLE_FLUIDE,false,false)==POLE_FLUIDE);
        ok("égalité + port → FLUIDE",               faction_pole_of(wgt,-1,true, false)==POLE_FLUIDE);
        ok("égalité + frontière → MARTIAL",         faction_pole_of(wgt,-1,false,true )==POLE_MARTIAL);
        ok("égalité nue → ORDRE",                   faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        /* le facteur 0.8 : le Gardien pousse Martial MOINS qu'un Conquérant plein. */
        memset(wgt,0,sizeof wgt); wgt[FAC_GARDIEN]=0.5f; wgt[FAC_MARCHAND]=0.45f;
        ok("Gardien 0.5 (×0.8 = 0.40) < Marchand 0.45 → FLUIDE (la pondération §7 compte)",
           faction_pole_of(wgt,-1,false,false)==POLE_FLUIDE);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    return g_fail?1:0;
}
