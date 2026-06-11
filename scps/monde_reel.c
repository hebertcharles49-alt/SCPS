/*
 * monde_reel.c — BANC D'ÉTALONNAGE de la fragilité (arc « le monde se fracture », A3)
 *
 *   make monde_reel && ./monde_reel
 *
 * La forme A1 de la fragilité — 10·σ(0.9(H−L)+0.3(H−P)−1.5) — n'a de valeur que
 * si elle TRIE le réel. Ce banc pose une douzaine de régimes contemporains en
 * coordonnées SCPS (0..10) et VÉRIFIE le verdict du moteur :
 *   - les trois grandes autocraties (Chine, Russie, Iran) sortent COERCITIF_FRAGILE ;
 *   - les démocraties consensuelles (Scandinavie, UE-cœur, Japon) sortent CONSENTI,
 *     fragilité < 2 ;
 *   - l'Amérique latine populiste-écrasée bascule en SUBMERGÉ → RÉVOLUTION ;
 *   - l'Afrique sub-saharienne multi-ethnique en SUBMERGÉ → SÉCESSION ;
 *   - la Turquie (~5.4) et les pétromonarchies arabes (~5.0) restent au BORD du
 *     seuil — frontière assumée (on n'assert que la bande de fragilité).
 *
 * ⚠ Les coordonnées sont ILLUSTRATIVES : calibrées pour reproduire les cibles du
 * brief, pas tirées d'un jeu de données politiste. Ce qu'on teste, c'est que la
 * FORME du moteur ordonne ces régimes comme l'intuition l'exige — pas la valeur
 * exacte d'un pays. Outillage de l'ingénieur : commentaires en français (CLAUDE.md).
 */
#include "scps_core.h"
#include <stdio.h>
#include <math.h>

static int g_pass=0, g_fail=0;

/* un régime = une fiche d'État (les 8 coordonnées d'ordre interne + diversité). */
typedef struct {
    const char *nom;
    ScpsState   st;
    int         mode_attendu;   /* ScpsMode, ou -1 = bord de seuil (mode non asserté).
                                 * int et non ScpsMode : l'enum peut être NON SIGNÉ →
                                 * un sentinelle −1 y deviendrait positif (< 0 faux). */
    float       frg_lo, frg_hi; /* bande de fragilité attendue */
} Regime;

static void ligne(const Regime *rg){
    ScpsOrder o = scps_order(&rg->st);
    ScpsMode  m = scps_mode(&o);
    bool ok_frg  = (o.fragilite >= rg->frg_lo && o.fragilite <= rg->frg_hi);
    bool ok_mode = (rg->mode_attendu < 0) || (m == (ScpsMode)rg->mode_attendu);
    bool ok = ok_frg && ok_mode;
    printf("   %s %-22s  SI %5.2f  frg %5.2f  → %-22s",
           ok ? "✓" : "✗", rg->nom, o.SI, o.fragilite, scps_mode_name(m));
    if (!ok_frg)  printf("  [frg hors [%.1f,%.1f]]", rg->frg_lo, rg->frg_hi);
    if (!ok_mode) printf("  [mode ≠ %s]", scps_mode_name((ScpsMode)rg->mode_attendu));
    printf("\n");
    if (ok) g_pass++; else g_fail++;
}

int main(void){
    printf("══════════════════════════════════════════════════════════════════════\n");
    printf(" MONDE RÉEL — la fragilité A1 trie-t-elle les régimes ? (banc d'étalonnage)\n");
    printf("══════════════════════════════════════════════════════════════════════\n");
    printf("   coordonnées 0..10 — H poigne · L légitimité · P ouverture · K capacité\n\n");

    /* Champs : {D_bar, P, C, K, F, I, H, L, flux_faustien}. */
    Regime R[] = {
        /* ── Les autocraties capables : SI haut (l'ordre TIENT), mais la poigne
         *    dépasse le consentement → COERCITIF_FRAGILE (elles craquent au choc). */
        { "Chine",   {4.f, 3.f, 6.f, 7.f, 3.f, 4.f, 7.f, 6.f, 0.f}, SCPS_COERCITIF_FRAGILE, 5.5f, 7.5f }, /* ~6.5 : légitimité de performance amortit */
        { "Russie",  {5.f, 4.f, 5.f, 6.f, 2.f, 5.f, 7.f, 3.f, 0.f}, SCPS_COERCITIF_FRAGILE, 9.0f,10.0f }, /* ~9.6 : L effondrée sous la poigne */
        { "Iran",    {5.f, 3.f, 4.f, 5.f, 3.f, 5.f, 7.f, 4.f, 0.f}, SCPS_COERCITIF_FRAGILE, 8.5f, 9.7f }, /* ~9.2 : théocratie contestée */
        /* ── Les démocraties consensuelles : H bas, L et P hauts → CONSENTI, frg<2. */
        { "Scandinavie", {3.f, 8.f, 7.f, 8.f, 6.f, 3.f, 1.f, 9.f, 0.f}, SCPS_CONSENTI, 0.f, 2.f },
        { "UE-cœur",     {4.f, 8.f, 8.f, 7.f, 7.f, 4.f, 2.f, 8.f, 0.f}, SCPS_CONSENTI, 0.f, 2.f },
        { "Japon",       {2.f, 7.f, 7.f, 8.f, 4.f, 4.f, 2.f, 8.f, 0.f}, SCPS_CONSENTI, 0.f, 2.f },
        /* ── Les submergés : l'ordre NE TIENT PLUS (SI<5). Le mode dit la couture. */
        { "Amérique latine", {2.f, 6.f, 7.f, 3.f, 2.f, 9.f, 4.f, 2.f, 0.f}, SCPS_SUBMERGE_REVOLUTION, 0.f,10.f }, /* homogène + écrasé → pression domine */
        { "Afrique sub-sah.",{8.f, 5.f, 5.f, 3.f, 3.f, 4.f, 3.f, 2.f, 0.f}, SCPS_SUBMERGE_SECESSION,  0.f,10.f }, /* très divers + peu consenti → fracture domine */
        /* ── Les bords de seuil : frontière ASSUMÉE, on n'assert que la fragilité. */
        { "Turquie",       {5.f, 3.f, 6.f, 5.f, 3.f, 5.f, 6.f, 5.f, 0.f}, -1, 5.0f, 6.5f }, /* ~5.4 */
        { "Pétromonarchies",{4.f, 4.f, 5.f, 6.f, 2.f, 4.f, 6.f, 5.f, 0.f}, -1, 4.5f, 5.5f }, /* ~5.0 */
    };
    int n = (int)(sizeof(R)/sizeof(R[0]));

    printf("  ── Les autocraties capables (l'ordre tient, mais par la poigne) ──\n");
    for (int i=0;i<3;i++) ligne(&R[i]);
    printf("\n  ── Les démocraties consensuelles (consenti, fragilité < 2) ──\n");
    for (int i=3;i<6;i++) ligne(&R[i]);
    printf("\n  ── Les ordres submergés (SI < 5 : la couture cède) ──\n");
    for (int i=6;i<8;i++) ligne(&R[i]);
    printf("\n  ── Les bords de seuil (frontière assumée) ──\n");
    for (int i=8;i<n;i++) ligne(&R[i]);

    printf("\n══════════════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════════════\n");
    return g_fail ? 1 : 0;
}
