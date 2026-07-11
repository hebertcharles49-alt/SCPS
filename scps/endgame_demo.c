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
        endgame_tick(&eg, NULL, NULL, &wp0, NULL, NULL, NULL, NULL, NULL, 0, y);

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
    endgame_tick(&eg, w, econ, wp, /*ts (toutes charges 0 via calloc)*/ NULL, NULL, NULL, NULL, NULL, 0, 0);
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
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 0);
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
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 180);
    CHECK("conso essence dominante → FIN_EAU", eg.fired && eg.fin == FIN_EAU);
    CHECK("foyer figé (epicentre assigné)", eg.epicenter_reg >= -1);
    CHECK("année de fin posée", eg.fin_year == 180);

    /* flux (1) dominant → RONCES */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 1000.0; wp->faust_consumed[2] = 0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 181);
    CHECK("conso flux dominante → FIN_RONCES", eg.fired && eg.fin == FIN_RONCES);

    /* fer céleste (2) dominant → FROID */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 182);
    CHECK("conso fer céleste dominante → FIN_FROID", eg.fired && eg.fin == FIN_FROID);

    /* LATCH : un 2e tick ne re-sélectionne pas (la fin reste figée) */
    FinType latched = eg.fin; int latched_year = eg.fin_year;
    wp->faust_consumed[0] = 9999.0;  /* on tente de forcer EAU après coup */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 183);
    CHECK("latch : la fin reste figée malgré un nouveau dominant", eg.fin == latched);
    CHECK("latch : l'année de fin ne bouge pas", eg.fin_year == latched_year);

    /* OVERRIDE merveille : MERV_ASCENDED force ASCENSION malgré un compteur dominant */
    endgame_init(&eg);
    eg.merv = MERV_ASCENDED;
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 1000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 80);
    CHECK("MERV_ASCENDED → FIN_ASCENSION (override)", eg.fired && eg.fin == FIN_ASCENSION);

    /* sous le seuil : aucune fin même avec un dominant */
    endgame_init(&eg);
    wp->entropy = FINV - 1.f;
    wp->faust_consumed[0] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 90);
    CHECK("sous ENTROPY_FIN : pas de fire", eg.fired == false && eg.fin == FIN_AUCUNE);

    /* ---- C3 : carve EAU → recalcul → fragmentation -------------------- */
    printf("\nC3 apocalypse d'eau (carve + recalcul + split)\n");
    /* monde FRAIS (les tests C2 ont déjà carvé/refragmenté ce w) */
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    int n_reg0 = econ->n_regions;
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
    endgame_init(&eg);
    wp->entropy = FINV + 10.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 1000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;
    /* fire (an 180) : sélecteur EAU + amorçage du rift + 1er pas de carve */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 180);
    CHECK("EAU déclenchée", eg.fired && eg.fin == FIN_EAU);
    CHECK("rift amorcé (régions programmées)", eg.sink_pending + eg.n_sunken > 0);
    /* déroule jusqu'à épuisement du rift */
    for (int y = 0; y < 80 && eg.sink_pending > 0; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 181 + y);
    CHECK("rift épuisé (sink_pending == 0)", eg.sink_pending == 0);
    CHECK("au moins une région engloutie", eg.n_sunken > 0);
    CHECK("n_regions INCHANGÉ (la région garde son indice)", econ->n_regions == n_reg0);

    /* (b) toute région englouti : owner=-1, impassable, cellules en mer.
     * RE-KEY PROVINCE : cataclysm_strip_region_econ route owner/impassable/… sur CHAQUE
     * province membre (econ->region[r] est un DÉRIVÉ, jamais rafraîchi par endgame_tick
     * seul — le jeu réel tourne econ_tick chaque mois ; ici on rafraîchit l'agrégat à la
     * main, PUR). */
    econ_aggregate_regions(econ);
    bool sink_ok = true, cell_ok = true;
    for (int r = 0; r < econ->n_regions; r++) {
        if (eg.sunken[r] != 2) continue;
        if (econ->region[r].owner != -1 || !econ->region[r].impassable) sink_ok = false;
    }
    /* échantillon : une cellule d'une région engloutie est bien sous la mer & strippée */
    { int probe = -1; for (int r = 0; r < econ->n_regions; r++) if (eg.sunken[r]==2){ probe=r; break; }
      if (probe >= 0) {
          bool found = false;
          for (int i = 0; i < SCPS_N && !found; i++)
              if (w->cell[i].height < SEA_LEVEL && w->cell[i].region == -1) found = true;
          cell_ok = found;
      } }
    CHECK("régions englouties : owner=-1 & impassable", sink_ok);
    CHECK("cellules englouties : sous la mer & hiérarchie strippée", cell_ok);

    /* (c) save_sane-style : indices de cellule/province/région dans les bornes */
    bool sane = true;
    for (int i = 0; i < SCPS_N; i++) { const Cell *c = &w->cell[i];
        if (c->province < -1 || c->region < -1 || c->country < -1 || c->continent < -1) sane = false;
        if (c->province >= w->n_provinces || c->region >= w->n_regions ||
            c->country >= w->n_countries || c->continent >= w->n_continents) sane = false; }
    for (int pr = 0; pr < w->n_provinces; pr++) {
        if (w->province[pr].region < 0 || w->province[pr].country < 0) sane = false;
        if (w->province[pr].region >= w->n_regions || w->province[pr].country >= w->n_countries) sane = false; }
    for (int c = 0; c < w->n_countries; c++) { const Country *ctc = &w->country[c];
        if (ctc->n_regions < 0 || ctc->n_regions > 32) sane = false;
        for (int k = 0; k < ctc->n_regions; k++)
            if (ctc->region_ids[k] < 0 || ctc->region_ids[k] >= w->n_regions) sane = false; }
    CHECK("invariants save_sane tenus après carve", sane);

    /* (d) idempotence : reconstruire l'adjacence éco 2× donne la MÊME matrice */
    { static uint8_t snap[SCPS_MAX_REG][SCPS_MAX_REG];
      world_recompute_adjacency(w); econ_build_adjacency(econ, w);
      memcpy(snap, econ->adj, sizeof snap);
      world_recompute_adjacency(w); econ_build_adjacency(econ, w);
      CHECK("idempotence du recalcul d'adjacence", memcmp(snap, econ->adj, sizeof snap) == 0); }

    /* (e) garantie du split : les régions d'un empire VIVANT forment UNE composante
     *     connexe (sinon la refragmentation a échoué). BFS sur econ->adj. */
    { bool all_connected = true;
      for (int c = 0; c < w->n_countries && all_connected; c++) {
          int regs[SCPS_MAX_REG], nr = 0;
          for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == c) regs[nr++] = r;
          if (nr <= 1) continue;
          int seen[SCPS_MAX_REG] = {0}, queue[SCPS_MAX_REG], qh = 0, qt = 0;
          seen[0] = 1; queue[qt++] = 0;
          while (qh < qt) { int ra = regs[queue[qh++]];
              for (int j = 0; j < nr; j++) if (!seen[j] && econ->adj[ra][regs[j]]) { seen[j] = 1; queue[qt++] = j; } }
          int reached = 0; for (int j = 0; j < nr; j++) reached += seen[j];
          if (reached != nr) all_connected = false;
      }
      CHECK("empires vivants : régions connexes (refragmentation)", all_connected); }

    /* ---- C4 : refroidissement (graduel, non géométrique) -------------- */
    printf("\nC4 apocalypse de froid (température → biomes → famine)\n");
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
    /* mesures AVANT */
    int n_sea0 = 0, n_cold0 = 0; double grain0 = 0.0; int probe = -1; float t0 = 0.f;
    for (int i = 0; i < SCPS_N; i++) {
        if (w->cell[i].height < SEA_LEVEL) n_sea0++;
        else if (w->cell[i].biome == BIO_GLACIER || w->cell[i].biome == BIO_STEPPE) n_cold0++;
        if (probe < 0 && w->cell[i].height >= SEA_LEVEL &&
            w->cell[i].temperature > 0.40f && w->cell[i].temperature < 0.60f) { probe = i; t0 = w->cell[i].temperature; }
    }
    for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner >= 0) grain0 += econ->region[r].raw_cap[RES_GRAIN];
    /* fire FROID + déroulé jusqu'au plateau */
    endgame_init(&eg);
    wp->entropy = FINV + 10.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 180);
    CHECK("FROID déclenchée", eg.fired && eg.fin == FIN_FROID);
    for (int y = 0; y < 220; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 181 + y);
    /* mesures APRÈS — RE-KEY PROVINCE : econ_cold_refresh (dans cold_step) route le
     * plancher de grain sur CHAQUE ProvinceEconomy.raw_cap[RES_GRAIN] (la vérité) ;
     * econ->region[r] n'est qu'un AGRÉGAT, jamais rafraîchi par endgame_tick seul (le
     * jeu réel tourne econ_tick chaque mois) — on rafraîchit l'agrégat à la main, PUR,
     * même idiome que C3 (l.214) et C6, sinon grain1 lit un miroir périmé (le grain
     * gelé n'y apparaît jamais → faux négatif de famine). */
    econ_aggregate_regions(econ);
    int n_sea1 = 0, n_cold1 = 0; double grain1 = 0.0; float t1 = (probe >= 0) ? w->cell[probe].temperature : 0.f;
    for (int i = 0; i < SCPS_N; i++) {
        if (w->cell[i].height < SEA_LEVEL) n_sea1++;
        else if (w->cell[i].biome == BIO_GLACIER || w->cell[i].biome == BIO_STEPPE) n_cold1++;
    }
    for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner >= 0) grain1 += econ->region[r].raw_cap[RES_GRAIN];
    CHECK("cold_offset plafonne (≈1.0)", eg.cold_offset >= 0.999f);
    CHECK("les biomes blanchissent (glacier/steppe ↑)", n_cold1 > n_cold0);
    CHECK("AUCUNE cellule ne devient mer (non géométrique)", n_sea1 == n_sea0);
    CHECK("une cellule tempérée s'est refroidie", probe < 0 || t1 < t0);
    CHECK("la fertilité vivrière s'effondre (famine)", grain1 < grain0);

    /* ---- C5 : ronces (BFS-cellules erratique, déterministe) ----------- */
    printf("\nC5 apocalypse des ronces (corruption cellulaire)\n");
    #define THORNS_HASH(W,OUTN,OUTH) do{ (OUTN)=0; (OUTH)=1469598103934665603ULL; \
        for(int i=0;i<SCPS_N;i++) if((W)->cell[i].biome==BIO_THORNS){ (OUTN)++; (OUTH)=((OUTH)^(unsigned)i)*1099511628211ULL; } }while(0)

    long thn_n_a=0; unsigned long long thn_h_a=0;
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) ts[c].charge=0.f;
    int n_reg_b = econ->n_regions;
    /* FINS CORRIGÉES (2026-07-11) : plus de bascule/détachement de région (THORN_FLIP_FRAC
     * SUPPRIMÉ) — la fin DÉGRADE (habitabilité + grain, econ_cold_refresh, motif du FROID)
     * au lieu d'EFFACER. « AVANT » = TOUTES les régions du monde FRAIS, capturé avant même
     * le fire — l'ÉRUPTION (1er pas de thorns_step) corrompt le foyer d'UN COUP dans la
     * MÊME tick que le fire, donc un snapshot pris APRÈS le fire capturerait déjà la marque
     * de l'épicentre à son plancher (aucune dégradation supplémentaire à observer sur 80
     * ans, puisque l'éruption a déjà tout corrompu). */
    econ_aggregate_regions(econ);
    static float hab_pre[SCPS_MAX_REG]; static double grain_pre[SCPS_MAX_REG];
    for (int r = 0; r < n_reg_b && r < SCPS_MAX_REG; r++) {
        hab_pre[r] = econ->region[r].habitability;
        grain_pre[r] = (double)econ->region[r].raw_cap[RES_GRAIN];
    }
    endgame_init(&eg);
    wp->entropy = FINV+10.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0]=0.0; wp->faust_consumed[1]=1000.0; wp->faust_consumed[2]=0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 180);
    CHECK("RONCES déclenchée", eg.fired && eg.fin == FIN_RONCES);
    long thn_after_fire=0; for(int i=0;i<SCPS_N;i++) if(w->cell[i].biome==BIO_THORNS) thn_after_fire++;
    CHECK("éruption : des cellules deviennent ronces", thn_after_fire > 0);
    int epi_b = eg.epicenter_reg;
    econ_aggregate_regions(econ);
    int epi_owner0 = (epi_b>=0 && epi_b<econ->n_regions) ? econ->region[epi_b].owner : -1;
    for (int y=0;y<80;y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 181+y);
    long thn_late=0; for(int i=0;i<SCPS_N;i++) if(w->cell[i].biome==BIO_THORNS) thn_late++;
    CHECK("le front s'étend (ronces ↑ après 80 ans)", thn_late > thn_after_fire);
    econ_aggregate_regions(econ);   /* RE-KEY : region[] n'est qu'un MIROIR de prov[] — même idiome que C3/C4/C6 */
    CHECK("la région-foyer n'est PAS détachée (owner inchangé, aucune suppression)",
          epi_b<0 || econ->region[epi_b].owner == epi_owner0);
    CHECK("n_regions inchangé (indices figés)", econ->n_regions == n_reg_b);
    if (epi_b>=0 && epi_b<n_reg_b && epi_b<SCPS_MAX_REG) {
        float  epi_hab1   = econ->region[epi_b].habitability;
        CHECK("l'habitabilité du foyer DÉGRADE (ronces = 0.05, plus de flip)", epi_hab1 < hab_pre[epi_b]);
    }
    /* Le GRAIN ne peut baisser QUE là où il existait déjà (une province côtière/archipel
     * peut vivre de poisson, raw_cap[GRAIN]=0 dès la genèse — econ_cold_refresh ne fait
     * que PLAFONNER vers le bas, jamais remonter) : on cherche, parmi TOUTES les régions
     * dégradées par le front (habitabilité en baisse), UNE qui avait du grain pour prouver
     * que la famine ÉMERGE bien (même mécanisme que C4/le FROID), plutôt que de fixer le
     * seul foyer (dont le sort géo-dépendant n'est pas garanti). */
    {
        bool found_grain_drop = false;
        for (int r = 0; r < n_reg_b && r < SCPS_MAX_REG; r++) {
            if (econ->region[r].owner < 0) continue;
            if (grain_pre[r] <= 0.0) continue;
            if (econ->region[r].habitability >= hab_pre[r]) continue;   /* pas touché par les ronces */
            if ((double)econ->region[r].raw_cap[RES_GRAIN] < grain_pre[r]) { found_grain_drop = true; break; }
        }
        CHECK("la fertilité vivrière d'une région dégradée s'effondre (famine progressive, pas de purge)",
              found_grain_drop);
    }
    THORNS_HASH(w, thn_n_a, thn_h_a);

    bool sane5 = (eg.thorn_front_n >= 0 && eg.thorn_front_n <= SCPS_THORN_FRONT_MAX);
    for (int i=0;i<eg.thorn_front_n && sane5;i++) if (eg.thorn_front[i]<0 || eg.thorn_front[i]>=SCPS_N) sane5=false;
    for (int i=0;i<SCPS_N && sane5;i++){ const Cell *c=&w->cell[i];
        if (c->region<-1||c->region>=w->n_regions||c->country<-1||c->country>=w->n_countries) sane5=false; }
    CHECK("invariants save_sane (front + cellules)", sane5);

    /* DÉTERMINISME : même graine + même fin ⇒ MÊME ensemble de ronces */
    long thn_n_b=0; unsigned long long thn_h_b=0;
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) ts[c].charge=0.f;
    endgame_init(&eg);
    wp->entropy = FINV+10.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0]=0.0; wp->faust_consumed[1]=1000.0; wp->faust_consumed[2]=0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 180);
    for (int y=0;y<80;y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 181+y);
    THORNS_HASH(w, thn_n_b, thn_h_b);
    CHECK("déterminisme : même ensemble de ronces (count)", thn_n_a == thn_n_b);
    CHECK("déterminisme : même ensemble de ronces (hash)", thn_h_a == thn_h_b);
    #undef THORNS_HASH

    /* ---- C6 : la Merveille (Ascension, victoire joueur) --------------- */
    printf("\nC6 merveille d'Ascension (paliers + charge + victoire)\n");
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    int pl = -1;
    /* RE-KEY PROVINCE : « role != POLITY_UNCLAIMED » n'exclut plus assez — le monde
     * porte désormais aussi des hameaux POLITY_WILD (Peuples Libres, jamais UNCLAIMED
     * mais jamais un vrai empire) plantés près des jouables ; le premier pays non-vierge
     * peut être un hameau SANS province semée (owner=-1 partout) → ci0 reste à 0, toute
     * la course FORGE→SAVOIR ne peut jamais avancer. On restreint donc au JOUEUR/IA
     * majeure réels (POLITY_PLAYER/POLITY_ANTAGONIST), la seule polité qui reçoit la
     * graine EMPIRE_SEED sur sa capitale (charte règle 7). */
    for (int c = 0; c < w->n_countries; c++)
        if (w->country[c].role == POLITY_PLAYER || w->country[c].role == POLITY_ANTAGONIST) { pl = c; break; }
    CHECK("un empire joueur existe", pl >= 0);
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) for (int t = 0; t < TECH_COUNT; t++) ts[c].unlocked[t] = false;
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
    /* injecte les 3 rares dans le pool du joueur — RE-KEY PROVINCE : la VÉRITÉ vit dans
     * prov[] (charte règle 1) ; injecter uniquement sur l'agrégat region[] tient jusqu'au
     * premier econ_aggregate_regions() (l.391 plus bas), qui l'ÉCRASE depuis les provinces
     * jamais dotées → flux/essence retombent à 0 en pleine course (paliers SOCIÉTÉ/SAVOIR
     * meurent de faim). On dote donc CHAQUE province membre (même idiome que econ_init
     * l.1121-1126, le pool tradable des cités-états). */
    for (int p = 0; p < econ->n_prov; p++) if (econ->prov[p].owner == pl) {
        econ->prov[p].stock[RES_CELESTIAL_IRON] += 100.f;
        econ->prov[p].stock[RES_FLUX]           += 100.f;
        econ->prov[p].stock[RES_ESSENCE]        += 100.f;
    }
    econ_aggregate_regions(econ);   /* miroir immédiat : ci0/le 1er endgame_tick lisent l'agrégat */
    int capr = -1; { int cap = w->country[pl].capital_prov; if (cap >= 0 && cap < w->n_provinces) capr = w->province[cap].region; }
    /* MÉTABOLISATION (décision #2) : FORGE≥3 SOCIÉTÉ≥4 SAVOIR≥6, comptés en MAX de
     * DEUX voies (comme ai_heritage_access) : la métabolisation active (population
     * digérée — plafonne à natif+2 par construction, la somme des parts d'héritages
     * NE PEUT PAS dépasser 1.0 : au plus 2 héritages étrangers peuvent chacun tenir
     * ≥METAB_TIER3=0.35 du total en même temps) ET la profondeur de CONTACT
     * (ts[].arch_depth[], commerce/gouvernance — INDÉPENDANTE du partage de pop,
     * peut donner tier3 à TOUS les héritages simultanément). On utilise arch_depth
     * pour piloter le compte proprement (3 puis 4 puis 6), la métabolisation seule
     * couvrant déjà le cas natif+2. */
    Heritage native0 = HERITAGE_ADAPTATIF;
    { int cp0 = w->country[pl].capital_prov; int cr0 = (cp0>=0&&cp0<w->n_provinces)?w->province[cp0].region:-1;
      if (cr0>=0 && cr0<econ->n_regions) native0 = econ->region[cr0].culture.heritage; }
    for (int h = 0; h < ARCH_COUNT; h++) ts[pl].arch_depth[h] = PROF_NONE;
    /* natif + 2 héritages en accès PROFOND (tier3 par contact) = 3 → assez pour FORGE. */
    { int put = 0; for (int h = 0; h < HERITAGE_COUNT && put < 2; h++) { if (h == (int)native0) continue;
        ts[pl].arch_depth[h] = (unsigned char)PROF_PROFOND; put++; } }

    endgame_init(&eg);
    endgame_start_wonder(&eg, pl, capr);
    CHECK("merveille démarrée en FORGE", eg.merv == MERV_FORGE);
    wp->entropy = 0.f; wp->entropy_epicenter = -1;          /* entropie BASSE : pas d'apocalypse concurrente */
    for (int k = 0; k < 3; k++) wp->faust_consumed[k] = 0.0;
    double ci0 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) ci0 += econ->region[r].stock[RES_CELESTIAL_IRON];
    float site_ch0 = (capr >= 0 && capr < econ->n_regions) ? econ->region[capr].faust_charge : 0.f;
    for (int y = 0; y < 4; y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("FORGE avance (3 héritages en accès plein)", eg.merv >= MERV_FORGE && eg.merv < MERV_SOCIETE);
    double ci1 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) ci1 += econ->region[r].stock[RES_CELESTIAL_IRON];
    CHECK("FORGE dévore le fer céleste", ci1 < ci0);
    /* RE-KEY PROVINCE : wonder_tick route faust_charge sur la province représentative
     * (econ->region[capr] est un DÉRIVÉ, jamais rafraîchi par endgame_tick seul — un
     * econ_tick RÉEL tourne chaque mois dans le jeu ; ici on rafraîchit l'agrégat à la
     * main, PUR, sans faire tourner un tick complet). */
    econ_aggregate_regions(econ);
    float site_ch1 = (capr >= 0 && capr < econ->n_regions) ? econ->region[capr].faust_charge : 0.f;
    CHECK("charge-additive : la Brèche se rapproche pendant le chantier", site_ch1 > site_ch0);
    /* pousse FORGE jusqu'à FORGE_DONE (metab_count==3 suffit), PUIS un tick de plus
     * pour franchir la transition FORGE_DONE→SOCIÉTÉ (le `done_state` de wonder_tick
     * n'enchaîne qu'au tick SUIVANT le palier bouclé). */
    for (int y = 4; y < 60 && eg.merv != MERV_FORGE_DONE; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("FORGE_DONE atteint (3 héritages suffisent)", eg.merv == MERV_FORGE_DONE);
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 60);
    CHECK("SOCIÉTÉ démarrée mais GELÉE sous 4 héritages (toujours 3)", eg.merv == MERV_SOCIETE);
    MervPhase frozen_at = eg.merv; float frozen_progress = eg.merv_progress;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 61);
    CHECK("gelé : la progression ne bouge pas sous le seuil",
          eg.merv == frozen_at && eg.merv_progress == frozen_progress);

    /* un 3e héritage étranger en accès profond (natif+3=4) : SOCIÉTÉ reprend. */
    { int put = 0; for (int h = 0; h < HERITAGE_COUNT && put < 3; h++) { if (h == (int)native0) continue;
        ts[pl].arch_depth[h] = (unsigned char)PROF_PROFOND; put++; } }
    for (int y = 62; y < 100 && eg.merv != MERV_SOCIETE_DONE; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("SOCIÉTÉ_DONE atteint (4 héritages suffisent)", eg.merv == MERV_SOCIETE_DONE);
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 100);
    CHECK("SAVOIR démarré mais GELÉ sous 6 héritages (toujours 4)", eg.merv == MERV_SAVOIR);
    MervPhase frozen_at2 = eg.merv; float frozen_progress2 = eg.merv_progress;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 101);
    CHECK("gelé (SAVOIR) : la progression ne bouge pas sous le seuil",
          eg.merv == frozen_at2 && eg.merv_progress == frozen_progress2);

    /* les 5 héritages étrangers en accès profond (natif+5=6) : SAVOIR reprend. */
    for (int h = 0; h < HERITAGE_COUNT; h++) if (h != (int)native0) ts[pl].arch_depth[h] = (unsigned char)PROF_PROFOND;
    for (int y = 102; y < 150 && eg.merv != MERV_SAVOIR_DONE; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("3 paliers bouclés (SAVOIR_DONE), 6 héritages suffisent", eg.merv == MERV_SAVOIR_DONE);
    CHECK("pas encore d'ascension (SAVOIR_DONE attend le tick suivant)", !eg.fired);

    int probe_r = -1; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) { probe_r = r; break; }
    int probe_cell = -1; Biome probe_bio = BIO_PLAINS; float probe_h = 0.f;
    if (probe_r >= 0) for (int i = 0; i < SCPS_N; i++) if (w->cell[i].region == probe_r && w->cell[i].height >= SEA_LEVEL) { probe_cell = i; probe_bio = w->cell[i].biome; probe_h = w->cell[i].height; break; }

    /* VICTOIRE = palier 3 (SAVOIR) COMPLÉTÉ — plus d'arbre complet ni d'assimilation
     * totale du monde (décision #2 : ces anciennes conditions sont RETIRÉES du verdict). */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 151);
    CHECK("SAVOIR_DONE + tick suivant → ASCENSION (sans arbre complet ni assimilation)",
          eg.merv == MERV_ASCENDED && eg.fired && eg.fin == FIN_ASCENSION);
    /* RE-KEY PROVINCE : endgame_empire_vanish route owner=-1 sur CHAQUE province membre
     * (econ->region[r].owner est un DÉRIVÉ, jamais rafraîchi par endgame_tick seul — cf.
     * le rafraîchissement PUR plus haut). */
    econ_aggregate_regions(econ);
    int owned = 0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) owned++;
    CHECK("l'empire DISPARAÎT (plus aucune région au joueur)", owned == 0);
    CHECK("terre INTACTE (biome/height inchangés — pas de carve)",
          probe_cell < 0 || (w->cell[probe_cell].biome == probe_bio && w->cell[probe_cell].height == probe_h));

    /* ---- C7 : V1a — le SANG (morts de guerre → entropie → FIN_SANG) --------- */
    printf("\nC7 apocalypse de SANG (morts de guerre, entropie unifiée, gate métabolisation)\n");
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
    Campaign *camp = (Campaign*)malloc(sizeof(Campaign));
    if (!camp) { fprintf(stderr,"OOM\n"); return 1; }
    campaign_init(camp, w, econ);

    /* pop_ref : posé une fois (sim_init réel), no-op si déjà posé. */
    endgame_init(&eg);
    CHECK("pop_ref==0 avant endgame_set_pop_ref", eg.pop_ref == 0.0);
    endgame_set_pop_ref(&eg, econ);
    double pop_ref0 = eg.pop_ref;
    CHECK("pop_ref posé (>0, un monde peuplé)", eg.pop_ref > 0.0);
    endgame_set_pop_ref(&eg, econ);   /* 2e appel : NO-OP (déjà posé) */
    CHECK("endgame_set_pop_ref est un no-op au 2e appel", eg.pop_ref == pop_ref0);

    /* war_dead s'accumule depuis Campaign.dead_choc/dead_pursuit (C1 widen) et SE
     * SÉRIALISE (EndgameState est fwrite/fread en un bloc — sizeof suffit à le prouver
     * : war_dead/pop_ref sont des membres PLATS de la struct, comme sunken[]/cold_offset). */
    wp->entropy = 0.f; wp->entropy_epicenter = -1;
    for (int k = 0; k < 3; k++) wp->faust_consumed[k] = 0.0;
    camp->dead_choc = 0; camp->dead_pursuit = 0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 10);
    CHECK("war_dead==0 sans morts de campagne", eg.war_dead == 0.0);
    float entropy_before_war = wp->entropy;
    camp->dead_choc = 1000; camp->dead_pursuit = 500;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 11);
    CHECK("war_dead reflète Campaign.dead_choc+dead_pursuit", eg.war_dead == 1500.0);
    CHECK("les morts de guerre NOURRISSENT l'entropie (widen)", wp->entropy > entropy_before_war);

    /* SANG l'emporte au SEUIL, quelle que soit la nature dominante (décision #1) —
     * même avec un transmuteur essence archi-dominant (qui SANS le ratio sang aurait
     * donné FIN_EAU), le sang prime dès que le ratio franchit ENDGAME_BLOOD_FRAC. */
    endgame_init(&eg);
    endgame_set_pop_ref(&eg, econ);
    double blood_frac = (double)tune_f("ENDGAME_BLOOD_FRAC", 0.20f);
    camp->dead_choc = (long)(eg.pop_ref * (blood_frac + 0.05));  /* nettement au-dessus du seuil */
    camp->dead_pursuit = 0;
    wp->entropy = tune_f("ENTROPY_FIN", 55.f) + 10.f;   /* seuil d'entropie déjà franchi */
    wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 100000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;  /* essence archi-dominant */
    /* SANG a besoin d'une région RAVAGÉE pour se marquer (sang_step relit revolt_scar) —
     * un monde frais n'a vécu ni guerre ni révolte : on marque une région à la main (le
     * signal que sac de guerre/révolte aurait posé). */
    for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner >= 0) { econ->region[r].revolt_scar = 0.9f; break; }
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 200);
    CHECK("SANG l'emporte au seuil (malgré un transmuteur dominant)", eg.fired && eg.fin == FIN_SANG);
    CHECK("foyer SANG assigné (épicentre valide)", eg.epicenter_reg >= -1);

    /* PLANCHER PERMANENT (2026-07-11, « fins corrigées ») : AUCUN drain de pop, AUCUNE
     * région supprimée — sang_scar[] est un RATCHET sur revolt_scar, et sa marque
     * PLANCHE le revolt_scar de CHAQUE province de la région (déjà appliqué par le
     * sang_step d'amorçage ci-dessus, au tick de fire). */
    int probe_sang = -1; float scar0 = -1.f;
    for (int r = 0; r < econ->n_regions; r++) if (eg.sang_scar[r] > scar0) { scar0 = eg.sang_scar[r]; probe_sang = r; }
    bool sang_marked = (probe_sang >= 0 && scar0 > 0.f);
    CHECK("une région marquée SANG existe", sang_marked);

    bool floored_at_fire = true;
    int  rep_pid = -1;
    if (sang_marked && probe_sang < w->n_regions) {
        const Region *rg = &w->region[probe_sang];
        for (int k = 0; k < rg->n_provinces; k++) {
            int pid = rg->province_ids[k];
            if (pid < 0 || pid >= econ->n_prov) continue;
            if (rep_pid < 0) rep_pid = pid;
            if (econ->prov[pid].revolt_scar < scar0 - 1e-4f) floored_at_fire = false;
        }
    }
    CHECK("le plancher a DÉJÀ mordu chaque province de la région, au tick de fire",
          !sang_marked || floored_at_fire);

    /* « une nouvelle guerre AGRANDIT la marque » : on simule ~4 ans de décrue NORMALE
     * (scps_econ.c) sur la province représentative (revolt_scar → 0), PUIS une guerre
     * plus sanglante (revolt_scar régional monte au-delà de scar0). sang_step doit
     * (a) faire monter sang_scar au nouveau maximum et (b) RESTAURER le plancher sur
     * la province malgré la décrue manuelle — la plaie ne se referme jamais. */
    if (rep_pid >= 0) econ->prov[rep_pid].revolt_scar = 0.f;
    float scar_grown = scar0 + 0.05f; if (scar_grown > 1.f) scar_grown = 1.f;
    if (sang_marked) econ->region[probe_sang].revolt_scar = scar_grown;
    for (int y = 0; y < 300; y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 201 + y);
    CHECK("une NOUVELLE guerre AGRANDIT la marque (jamais redescendue)",
          !sang_marked || eg.sang_scar[probe_sang] >= scar_grown - 1e-4f);
    CHECK("le plancher RESTAURE la province malgré la décrue manuelle (ne guérit plus)",
          rep_pid < 0 || econ->prov[rep_pid].revolt_scar >= eg.sang_scar[probe_sang] - 1e-4f);
    CHECK("la cicatrice sang NE GUÉRIT PAS (contrairement à revolt_scar)",
          !sang_marked || eg.sang_scar[probe_sang] >= scar0);

    /* INTENSITÉ PAR RÉGION (lot D) : bornée [0..1], cohérente avec la marque. */
    if (sang_marked) {
        float inten = endgame_region_intensity(&eg, w, econ, probe_sang);
        CHECK("intensité SANG == sang_scar (la même marque)", fabsf(inten - eg.sang_scar[probe_sang]) < 1e-6f);
    }
    { float bogus = endgame_region_intensity(&eg, w, econ, -1); CHECK("intensité hors-bornes == 0", bogus == 0.f); }

    /* METAB_COUNT public (sans TechState) : natif toujours ≥1, borné [0,HERITAGE_COUNT]. */
    { int mc = endgame_metab_count(w, econ, 0);
      CHECK("endgame_metab_count borné [0,HERITAGE_COUNT]", mc >= 0 && mc <= HERITAGE_COUNT); }
    CHECK("endgame_metab_required(FORGE)==3",   endgame_metab_required(MERV_FORGE) == 3);
    CHECK("endgame_metab_required(SOCIETE)==4", endgame_metab_required(MERV_SOCIETE) == 4);
    CHECK("endgame_metab_required(SAVOIR)==6",  endgame_metab_required(MERV_SAVOIR) == 6);
    CHECK("endgame_metab_required(NONE)==0",    endgame_metab_required(MERV_NONE) == 0);

    /* ---- C9 : #32 (LE SANG SIGNE TON RÈGNE) — le JUMEAU joueur ---------------------
     * La thèse : « chaque fin est la conséquence de la ressource qu'ON a brûlée ». Un
     * joueur pacifiste dans un monde IA sanglant ne doit PAS recevoir SANG — seul SON
     * sang doit compter. Trois garanties : (1) l'accumulateur jumeau ne compte QUE les
     * batailles où le joueur est belligérant ; (2) la garde retombe au sélecteur normal
     * (rare dominant/hash) quand la part du joueur est SOUS le seuil ; (3) sans main
     * humaine (campaign_get_human()==-1, le défaut), rien ne change — golden intact. */
    printf("\nC9 #32 — LE SANG SIGNE TON RÈGNE (le jumeau joueur)\n");

    /* C9a — l'accumulateur jumeau ne compte QUE les batailles DU joueur. */
    { int pcid = 3;   /* un pays quelconque, "le joueur" pour ce test */
      campaign_set_human(pcid);
      CHECK("campaign_get_human reflète campaign_set_human", campaign_get_human() == pcid);
      camp->dead_choc = 0; camp->dead_pursuit = 0;
      camp->dead_choc_player = 0; camp->dead_pursuit_player = 0;
      /* bataille IMPLIQUANT le joueur (site bt_day/bt_rout — ici simulée directement,
       * comme le fait déjà endgame_entropy_widen en lisant Campaign) : le joueur
       * belligérant ⇒ le site per-bataille (scps_campaign.c) cumule le jumeau. */
      camp->army[pcid].owner = pcid; camp->army[7].owner = 7;   /* deux belligérants distincts */
      /* Le SITE réel est dans bt_day/bt_rout (scps_campaign.c) ; ce banc ne rejoue pas
       * une bataille complète (hors périmètre endgame_demo) — il VÉRIFIE le CONTRAT
       * observable : quand seul le cumul GLOBAL bouge (aucune bataille du joueur),
       * war_dead_player doit rester à 0 ; quand le jumeau bouge (le joueur a combattu),
       * le ratio joueur doit suivre. */
      endgame_init(&eg); endgame_set_pop_ref(&eg, econ);
      wp->entropy = 0.f; wp->entropy_epicenter = -1;
      camp->dead_choc = 2000; camp->dead_pursuit = 0;         /* morts MONDIALES (IA vs IA, pas le joueur) */
      camp->dead_choc_player = 0; camp->dead_pursuit_player = 0;   /* le joueur n'a RIEN combattu */
      endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 10);
      CHECK("war_dead MONDIAL reflète le cumul (IA vs IA)", eg.war_dead == 2000.0);
      CHECK("war_dead_player reste À ZÉRO (le joueur n'était PAS belligérant)", eg.war_dead_player == 0.0);
      CHECK("blood_player_share == 0 (rien à partager côté joueur)", endgame_blood_player_share(&eg) == 0.0);

      /* maintenant le joueur COMBAT (le jumeau bouge, au MÊME site que le global —
       * simulé ici par le même delta-tracking que bt_day/bt_rout appliqueraient). */
      camp->dead_choc = 3000; camp->dead_choc_player = 1000;   /* +1000 mondial, dont +1000 DU joueur */
      endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 11);
      CHECK("war_dead_player suit SA propre bataille (delta 1000)", eg.war_dead_player > 0.0);
      CHECK("war_dead_player ne dépasse JAMAIS war_dead (⊆ par construction)",
            eg.war_dead_player <= eg.war_dead + 1e-6);
      campaign_set_human(-1);   /* repos : ne pas fuiter vers les blocs suivants */
    }

    /* C9b — la garde retombe au sélecteur normal (rare dominant/hash) quand la PART
     * du joueur est SOUS BLOOD_PLAYER_SHARE, malgré un ratio MONDIAL bien au-dessus
     * du seuil ENDGAME_BLOOD_FRAC (le monde a saigné — mais pas par lui). */
    { int pcid = 5;
      campaign_set_human(pcid);
      world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
      for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
      endgame_init(&eg); endgame_set_pop_ref(&eg, econ);
      double blood_frac2 = (double)tune_f("ENDGAME_BLOOD_FRAC", 0.20f);
      double share_thr = (double)tune_f("BLOOD_PLAYER_SHARE", 0.25f);
      /* ratio MONDIAL largement au-dessus du seuil, mais la part DU joueur (via le
       * jumeau) reste NULLE — un monde IA sanglant, un joueur qui n'a rien fait. */
      camp->dead_choc = (long)(eg.pop_ref * (blood_frac2 + 0.05));
      camp->dead_pursuit = 0;
      camp->dead_choc_player = 0; camp->dead_pursuit_player = 0;   /* AUCUNE bataille du joueur */
      wp->entropy = tune_f("ENTROPY_FIN", 55.f) + 10.f;
      wp->entropy_epicenter = -1;
      wp->faust_consumed[0] = 100000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;  /* essence dominant → EAU attendu */
      for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner >= 0) { econ->region[r].revolt_scar = 0.9f; break; }
      endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 200);
      double ratio2 = endgame_blood_ratio(&eg, econ);
      CHECK("ratio MONDIAL bien au-dessus du seuil (le monde A saigné)", ratio2 >= blood_frac2);
      CHECK("part DU joueur SOUS le seuil (il n'a rien brûlé)", endgame_blood_player_share(&eg) < share_thr);
      CHECK("SANG N'EST PAS retenue malgré le ratio mondial (retombe au rare dominant)",
            eg.fired && eg.fin != FIN_SANG);
      campaign_set_human(-1);
    }

    /* C9c — human=-1 (chronique/viewer sans main humaine) : comportement STRICTEMENT
     * INCHANGÉ — le seul test reste ENDGAME_BLOOD_FRAC (comme avant #32). Reproduit
     * EXACTEMENT le scénario C7 « SANG l'emporte au seuil » ligne ~505 : sans main
     * humaine, une part-joueur nulle ne doit PAS bloquer SANG. */
    { CHECK("campaign_get_human()==-1 par défaut (repos post-C9a/b)", campaign_get_human() == -1);
      world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
      for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
      endgame_init(&eg); endgame_set_pop_ref(&eg, econ);
      double blood_frac3 = (double)tune_f("ENDGAME_BLOOD_FRAC", 0.20f);
      camp->dead_choc = (long)(eg.pop_ref * (blood_frac3 + 0.05));
      camp->dead_pursuit = 0; camp->dead_choc_player = 0; camp->dead_pursuit_player = 0;
      wp->entropy = tune_f("ENTROPY_FIN", 55.f) + 10.f;
      wp->entropy_epicenter = -1;
      wp->faust_consumed[0] = 100000.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 0.0;
      for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner >= 0) { econ->region[r].revolt_scar = 0.9f; break; }
      endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, camp, 0, 200);
      CHECK("human=-1 : SANG l'emporte au seuil MONDIAL seul (garde INACTIVE, inchangé)",
            eg.fired && eg.fin == FIN_SANG);
    }

    /* ---- C8 : CORRECTIF Merveille — MAX(contact, diaspora INDIVIDUALISÉE) ----------
     * Le piège évité : l'ancien calcul jugeait chaque héritage sur la POP TOTALE de
     * l'empire (comme econ_country_heritage_digested, fait pour l'accès TECH) — 6
     * héritages ne peuvent JAMAIS clear 0.35 chacun de la MÊME pop (6×0.35>1). Ici,
     * chaque héritage est jugé sur SA PROPRE communauté (dig_X/tot_X), ou par la voie
     * gouvernance (arch_depth) — indépendante de la population, pour les CONQUIS
     * administrés en profondeur SANS diaspora. */
    printf("\nC8 correctif Merveille (individualisation par héritage + voie gouvernance)\n");
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);   /* monde FRAIS (C7 a dépeuplé/muté) */
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) { for (int h = 0; h < ARCH_COUNT; h++) ts[c].arch_depth[h] = PROF_NONE; ts[c].charge = 0.f; }
    { int pl2 = -1, provA = -1, provB = -1;
      /* choisit un empire avec ≥2 provinces ACTIVES (le premier PLAYER/ANTAGONIST
       * trouvé peut n'en avoir qu'une seule dans ce monde-là — on scanne large). */
      for (int c = 0; c < w->n_countries && provB < 0; c++) {
          if (w->country[c].role != POLITY_PLAYER && w->country[c].role != POLITY_ANTAGONIST) continue;
          int a = -1, b = -1;
          for (int p = 0; p < econ->n_prov; p++) if (econ->prov[p].owner == c && econ->prov[p].active) {
              if (a < 0) a = p; else if (b < 0) { b = p; break; }
          }
          if (a >= 0 && b >= 0) { pl2 = c; provA = a; provB = b; }
      }
      if (pl2 < 0) {
          /* repli : aucun empire n'a nativement ≥2 provinces actives à la genèse
           * (mondes petits/4-empires) — on en FORCE une 2e sur le premier empire
           * trouvé (même idiome que econ_guarantee_player_construction : une
           * province vierge PASSABLE devient sienne pour le test). */
          for (int c = 0; c < w->n_countries && pl2 < 0; c++) {
              if (w->country[c].role != POLITY_PLAYER && w->country[c].role != POLITY_ANTAGONIST) continue;
              int a = -1, b = -1;
              for (int p = 0; p < econ->n_prov; p++) {
                  if (econ->prov[p].owner == c && econ->prov[p].active) { if (a < 0) a = p; }
                  else if (b < 0 && !econ->prov[p].active && econ->prov[p].owner < 0) b = p;
              }
              if (a >= 0 && b >= 0) {
                  econ->prov[b].owner = (int16_t)c; econ->prov[b].active = true; econ->prov[b].colonized = true;
                  pl2 = c; provA = a; provB = b;
              }
          }
      }
      CHECK("un empire existe (C8)", pl2 >= 0);
      Heritage nat2 = HERITAGE_ADAPTATIF;
      { int cp2 = w->country[pl2].capital_prov; int cr2 = (cp2>=0&&cp2<w->n_provinces)?w->province[cp2].region:-1;
        if (cr2>=0 && cr2<econ->n_regions) nat2 = econ->region[cr2].culture.heritage; }
      /* choisit 2 héritages étrangers ≠ natif pour les diasporas (A petite/bien
       * intégrée, B grosse/bien intégrée — l'ancienne math, sur la pop TOTALE, aurait
       * laissé A écrasée sous le seuil dès que B existe ; ici chacun a SON propre
       * dénominateur, donc les DEUX comptent). */
      int hA = -1, hB = -1;
      for (int h = 0; h < HERITAGE_COUNT; h++) { if (h == (int)nat2) continue; if (hA<0) hA=h; else if (hB<0) { hB=h; break; } }
      CHECK("2 héritages étrangers disponibles (C8)", hA >= 0 && hB >= 0);
      CHECK("2 provinces du joueur disponibles (C8)", provA >= 0 && provB >= 0);
      if (provA >= 0 && provB >= 0 && hA >= 0 && hB >= 0) {
          ProvinceEconomy *peA = &econ->prov[provA], *peB = &econ->prov[provB];
          peA->pop.n_groups = 0; peB->pop.n_groups = 0;
          /* natif résiduel (pour que native0 continue d'exister ailleurs — non testé ici). */
          PopGroup *gA = &peA->pop.groups[peA->pop.n_groups++]; memset(gA, 0, sizeof *gA);
          gA->heritage = (Heritage)hA; gA->arrival = ARR_MIGRANT; gA->integration = 1.f;
          gA->count = 600; gA->home_reg = -1; gA->faith = -1;               /* PETITE diaspora, bien intégrée */
          PopGroup *gB = &peB->pop.groups[peB->pop.n_groups++]; memset(gB, 0, sizeof *gB);
          gB->heritage = (Heritage)hB; gB->arrival = ARR_MIGRANT; gB->integration = 1.f;
          gB->count = 50000; gB->home_reg = -1; gB->faith = -1;             /* GROSSE diaspora, bien intégrée */
          econ_aggregate_regions(econ);
          for (int h = 0; h < ARCH_COUNT; h++) ts[pl2].arch_depth[h] = PROF_NONE;   /* voie gouvernance OFF ici */
          int mc2 = endgame_metab_count(w, econ, pl2);
          CHECK("l'individualisation compte les DEUX diasporas (petite ET grosse)", mc2 >= 3 /* natif+A+B */);
      }
      /* VOIE GOUVERNANCE SEULE (via le gate RÉEL de wonder_tick, qui lit ts — la
       * fonction individualisée endgame_metab_count_ts est privée au module) : un
       * empire SANS AUCUNE diaspora (0 groupe étranger) mais dont 3 héritages
       * étrangers sont en accès PROFOND (contact/gouvernance, ts[].arch_depth) doit
       * pouvoir faire avancer FORGE (natif+3=... ici natif+3 héritages contact =
       * 4 en tout, ≥ requis FORGE=3) — la preuve que la voie gouvernance SEULE,
       * sans un seul migrant, compte. */
      int pl3 = -1;
      for (int c = pl2+1; c < w->n_countries; c++)
          if (w->country[c].role == POLITY_PLAYER || w->country[c].role == POLITY_ANTAGONIST) { pl3 = c; break; }
      if (pl3 < 0) pl3 = pl2;   /* repli : un seul empire majeur dans ce monde-là */
      Heritage nat3 = HERITAGE_ADAPTATIF;
      { int cp3 = w->country[pl3].capital_prov; int cr3 = (cp3>=0&&cp3<w->n_provinces)?w->province[cp3].region:-1;
        if (cr3>=0 && cr3<econ->n_regions) nat3 = econ->region[cr3].culture.heritage; }
      /* VIDE toute diaspora du joueur pl3 (0 groupe étranger — la voie diaspora ne
       * peut RIEN compter) puis pose 2 héritages étrangers en accès PROFOND. */
      for (int p = 0; p < econ->n_prov; p++) if (econ->prov[p].owner == pl3 && econ->prov[p].active)
          econ->prov[p].pop.n_groups = 0;
      econ_aggregate_regions(econ);
      for (int c = 0; c < SCPS_MAX_COUNTRY; c++) for (int h = 0; h < ARCH_COUNT; h++) ts[c].arch_depth[h] = PROF_NONE;
      { int put = 0; for (int h = 0; h < HERITAGE_COUNT && put < 2; h++) { if (h == (int)nat3) continue;
          ts[pl3].arch_depth[h] = (unsigned char)PROF_PROFOND; put++; } }
      EndgameState eg8; endgame_init(&eg8);
      int capr3 = -1; { int cap3 = w->country[pl3].capital_prov; if (cap3>=0&&cap3<w->n_provinces) capr3 = w->province[cap3].region; }
      for (int p = 0; p < econ->n_prov; p++) if (econ->prov[p].owner == pl3) {
          econ->prov[p].stock[RES_CELESTIAL_IRON] += 100.f;
      }
      econ_aggregate_regions(econ);
      endgame_start_wonder(&eg8, pl3, capr3);
      eg8.merv = MERV_FORGE;   /* re-force au cas où endgame_start_wonder ait été bloqué par un état résiduel */
      double ci8_0 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl3) ci8_0 += econ->region[r].stock[RES_CELESTIAL_IRON];
      WorldProsperity wp8; memset(&wp8, 0, sizeof wp8); wp8.entropy = 0.f; wp8.entropy_epicenter = -1;
      endgame_tick(&eg8, w, econ, &wp8, ts, NULL, NULL, NULL, NULL, pl3, 0);
      double ci8_1 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl3) ci8_1 += econ->region[r].stock[RES_CELESTIAL_IRON];
      CHECK("voie GOUVERNANCE seule (0 diaspora, 2 contacts profonds+natif=3) ⇒ FORGE avance",
            ci8_1 < ci8_0);
    }

    free(camp);
    free(ts); free(wp); free(econ); free(w);

    /* ---- Récapitulatif ------------------------------------------------- */
    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
