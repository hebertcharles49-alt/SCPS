/*
 * endgame_demo.c — banc auto-vérifiant du capstone §27.
 *
 * Contrôles :
 *   C0 : init zéroïse · tick sans entropie ne déclenche rien · struct plate
 *   C1 : band_entropie franchit les 4 bandes aux bornes ; widen ajoute la tech
 *   C2 : sélecteur par compteur dominant (eau/ronces/froid) · latch · override
 */
#include "scps_endgame.h"
#include "scps_world.h"
#include "scps_prosperity.h"
#include "scps_tune.h"
#include "scps_readout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    WorldProsperity wp0; memset(&wp0, 0, sizeof wp0);
    wp0.entropy = 0.0f;
    wp0.entropy_epicenter = -1;

    /* N ticks : fired doit rester false */
    for (int y = 0; y < 300; y++)
        endgame_tick(&eg, NULL, NULL, &wp0, NULL, NULL, NULL, NULL, 0, y);

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

    /* ---- C1 : la membrane band_entropie aux 4 bornes ------------------- */
    printf("\nC1 band_entropie (ratio entropy/fin)\n");
    float FIN = 1000.f;   /* seuil arbitraire : on teste le RATIO */
    CHECK("ratio 0.10 → ENT_STABLE",      band_entropie(100.f,  FIN) == ENT_STABLE);
    CHECK("ratio 0.24 → ENT_STABLE",      band_entropie(240.f,  FIN) == ENT_STABLE);
    CHECK("ratio 0.30 → ENT_FREMISSANTE", band_entropie(300.f,  FIN) == ENT_FREMISSANTE);
    CHECK("ratio 0.54 → ENT_FREMISSANTE", band_entropie(540.f,  FIN) == ENT_FREMISSANTE);
    CHECK("ratio 0.60 → ENT_INSTABLE",    band_entropie(600.f,  FIN) == ENT_INSTABLE);
    CHECK("ratio 0.84 → ENT_INSTABLE",    band_entropie(840.f,  FIN) == ENT_INSTABLE);
    CHECK("ratio 0.90 → ENT_AU_BORD",     band_entropie(900.f,  FIN) == ENT_AU_BORD);
    CHECK("ratio 1.50 → ENT_AU_BORD",     band_entropie(1500.f, FIN) == ENT_AU_BORD);
    CHECK("fin<=0 → ENT_STABLE (garde)",  band_entropie(500.f,  0.f) == ENT_STABLE);

    /* ---- monde réel pour C1-widen + C2 -------------------------------- */
    World *w = (World*)malloc(sizeof(World));
    WorldEconomy *econ = (WorldEconomy*)malloc(sizeof(WorldEconomy));
    WorldProsperity *wp = (WorldProsperity*)malloc(sizeof(WorldProsperity));
    if (!w || !econ || !wp) { fprintf(stderr,"OOM\n"); return 1; }
    WorldParams p = worldparams_default(9);
    p.n_empires = 4; p.n_city_states = 6;
    world_generate(w, &p);
    econ_init(econ, w);
    gen_population(w, econ);
    prosperity_init(wp, w);

    printf("\nC1 widen : la charge de tech faustienne abonde l'entropie\n");
    float TECH_W = tune_f("ENTROPY_TECH_W", 1.0f);
    float FIN0   = tune_f("ENTROPY_FIN", 50.f);
    endgame_init(&eg);
    wp->entropy = 0.f; wp->entropy_epicenter = -1;
    for (int k = 0; k < 3; k++) wp->faust_consumed[k] = 0.0;
    int victim = -1;
    for (int c = 0; c < w->n_countries; c++)
        if (w->country[c].role != POLITY_UNCLAIMED) { victim = c; break; }
    CHECK("un empire existe", victim >= 0);
    /* charge nulle → widen n'ajoute rien */
    wp->entropy = 0.f;
    endgame_tick(&eg, w, econ, wp, /*ts (toutes charges 0 via calloc)*/ NULL, NULL, NULL, NULL, 0, 0);
    /* (ts NULL → widen ne fait rien, et select_and_fire est sauté : entropy reste 0) */
    CHECK("ts NULL : entropie inchangée (0)", wp->entropy == 0.f);

    /* avec un vrai tableau ts : charge injectée (visée 0.4×FIN → SOUS le seuil) →
     * entropie montée de TECH_W×charge, et PAS de déclenchement. */
    TechState *ts = (TechState*)calloc(SCPS_MAX_COUNTRY, sizeof(TechState));
    if (!ts) { fprintf(stderr,"OOM\n"); return 1; }
    float charge_inj = (FIN0 * 0.4f) / (TECH_W > 0.f ? TECH_W : 1.f);
    ts[victim].charge = charge_inj;
    wp->entropy = 0.f;
    endgame_init(&eg);
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 0);
    float expect = TECH_W * charge_inj;
    CHECK("widen : entropie == TECH_W × charge", fabsf(wp->entropy - expect) < 0.01f);
    CHECK("sous le seuil : pas de fire", eg.fired == false);

    /* ---- C2 : sélecteur par compteur dominant -------------------------- */
    printf("\nC2 sélecteur + latch + override\n");
    float FINV = tune_f("ENTROPY_FIN", 50.f);

    /* essence (0) dominant → EAU */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 1000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) ts[c].charge = 0.f;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 50);
    CHECK("conso essence dominante → FIN_EAU", eg.fired && eg.fin == FIN_EAU);
    CHECK("foyer figé (epicentre assigné)", eg.epicenter_reg >= -1);
    CHECK("année de fin posée", eg.fin_year == 50);

    /* flux (1) dominant → RONCES */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 1000.0; wp->faust_consumed[2] = 0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 60);
    CHECK("conso flux dominante → FIN_RONCES", eg.fired && eg.fin == FIN_RONCES);

    /* fer céleste (2) dominant → FROID */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 70);
    CHECK("conso fer céleste dominante → FIN_FROID", eg.fired && eg.fin == FIN_FROID);

    /* LATCH : un 2e tick ne re-sélectionne pas (la fin reste figée) */
    FinType latched = eg.fin; int latched_year = eg.fin_year;
    wp->faust_consumed[0] = 9999.0;  /* on tente de forcer EAU après coup */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 71);
    CHECK("latch : la fin reste figée malgré un nouveau dominant", eg.fin == latched);
    CHECK("latch : l'année de fin ne bouge pas", eg.fin_year == latched_year);

    /* OVERRIDE merveille : MERV_ASCENDED force ASCENSION malgré un compteur dominant */
    endgame_init(&eg);
    eg.merv = MERV_ASCENDED;
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 1000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 80);
    CHECK("MERV_ASCENDED → FIN_ASCENSION (override)", eg.fired && eg.fin == FIN_ASCENSION);

    /* sous le seuil : aucune fin même avec un dominant */
    endgame_init(&eg);
    wp->entropy = FINV - 1.f;
    wp->faust_consumed[0] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, 0, 90);
    CHECK("sous ENTROPY_FIN : pas de fire", eg.fired == false && eg.fin == FIN_AUCUNE);

    free(ts); free(wp); free(econ); free(w);

    /* ---- Récapitulatif ------------------------------------------------- */
    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
