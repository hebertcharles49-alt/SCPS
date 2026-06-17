/*
 * endgame_demo.c — banc auto-vérifiant du capstone §27.
 *
 * Contrôles C0 :
 *   0 : endgame_init zéroïse toutes les valeurs sensibles
 *   1 : endgame_tick sans entropie ne déclenche PAS fired
 *   2 : EndgameState est une struct plate (taille cohérente avec ses membres)
 */
#include "scps_endgame.h"
#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;

#define CHECK(msg, cond) do { \
    if (cond) { g_pass++; printf("  OK  " msg "\n"); } \
    else      { g_fail++; printf("  KO  " msg "\n"); } \
} while(0)

int main(void) {
    printf("=== endgame_demo — capstone §27 ===\n\n");

    /* ---- C0-0 : init zéroïse ------------------------------------------ */
    printf("C0-0 init zéroïse\n");
    EndgameState eg;
    /* on remplit de bruit pour s'assurer que init l'efface */
    memset(&eg, 0xFF, sizeof eg);
    endgame_init(&eg);

    CHECK("fin == FIN_AUCUNE",     eg.fin == FIN_AUCUNE);
    CHECK("fired == false",        eg.fired == false);
    CHECK("epicenter_reg == -1",   eg.epicenter_reg == -1);
    CHECK("fauteur_country == -1", eg.fauteur_country == -1);
    CHECK("fin_year == -1",        eg.fin_year == -1);
    CHECK("n_sunken == 0",         eg.n_sunken == 0);
    CHECK("sink_pending == 0",     eg.sink_pending == 0);
    CHECK("cold_offset == 0",      eg.cold_offset == 0.0f);
    CHECK("thorn_front_n == 0",    eg.thorn_front_n == 0);
    CHECK("thorn_rng == 0",        eg.thorn_rng == 0u);
    CHECK("merv == MERV_NONE",     eg.merv == MERV_NONE);
    CHECK("merv_country == -1",    eg.merv_country == -1);
    CHECK("merv_site_reg == -1",   eg.merv_site_reg == -1);
    CHECK("merv_progress == 0",    eg.merv_progress == 0.0f);

    /* sunken[] doit être entièrement zéro */
    bool sunken_ok = true;
    for (int r = 0; r < SCPS_MAX_REG; r++)
        if (eg.sunken[r] != 0) { sunken_ok = false; break; }
    CHECK("sunken[] entièrement zéro", sunken_ok);

    /* ---- C0-1 : tick sans entropie → pas de fire ----------------------- */
    printf("\nC0-1 tick sans entropie\n");
    WorldProsperity wp; memset(&wp, 0, sizeof wp);
    wp.entropy = 0.0f;
    wp.entropy_epicenter = -1;

    /* N ticks : fired doit rester false */
    for (int y = 0; y < 300; y++)
        endgame_tick(&eg, NULL, NULL, &wp, NULL, NULL, NULL, NULL, 0, y);

    CHECK("300 ticks entropie=0 : fired reste false",  eg.fired == false);
    CHECK("300 ticks entropie=0 : fin reste FIN_AUCUNE", eg.fin == FIN_AUCUNE);

    /* ---- C0-2 : struct plate — taille minimale cohérente --------------- */
    printf("\nC0-2 struct plate\n");
    /* Vérification grossière : la taille doit être au moins la somme des membres
     * connus (le padding peut l'agrandir, jamais la rétrécir). */
    size_t min_sz = sizeof(FinType) + sizeof(bool)
                  + 3 * sizeof(int)
                  + SCPS_MAX_REG * sizeof(uint8_t) + 2 * sizeof(int)
                  + sizeof(float)
                  + SCPS_THORN_FRONT_MAX * sizeof(int) + sizeof(int) + sizeof(uint32_t)
                  + sizeof(MervPhase) + 2 * sizeof(int) + sizeof(float);
    CHECK("sizeof(EndgameState) >= somme membres", sizeof(EndgameState) >= min_sz);

    /* La struct ne doit pas grossir de façon absurde (plafond 2× min_sz) */
    CHECK("sizeof(EndgameState) < 2× somme membres", sizeof(EndgameState) < 2 * min_sz);

    /* ---- Récapitulatif ------------------------------------------------- */
    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
