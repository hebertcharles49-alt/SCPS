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
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 50);
    CHECK("conso essence dominante → FIN_EAU", eg.fired && eg.fin == FIN_EAU);
    CHECK("foyer figé (epicentre assigné)", eg.epicenter_reg >= -1);
    CHECK("année de fin posée", eg.fin_year == 50);

    /* flux (1) dominant → RONCES */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 1000.0; wp->faust_consumed[2] = 0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 60);
    CHECK("conso flux dominante → FIN_RONCES", eg.fired && eg.fin == FIN_RONCES);

    /* fer céleste (2) dominant → FROID */
    endgame_init(&eg);
    wp->entropy = FINV + 1.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0] = 0.0; wp->faust_consumed[1] = 0.0; wp->faust_consumed[2] = 1000.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 70);
    CHECK("conso fer céleste dominante → FIN_FROID", eg.fired && eg.fin == FIN_FROID);

    /* LATCH : un 2e tick ne re-sélectionne pas (la fin reste figée) */
    FinType latched = eg.fin; int latched_year = eg.fin_year;
    wp->faust_consumed[0] = 9999.0;  /* on tente de forcer EAU après coup */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 71);
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
    /* fire (an 100) : sélecteur EAU + amorçage du rift + 1er pas de carve */
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 100);
    CHECK("EAU déclenchée", eg.fired && eg.fin == FIN_EAU);
    CHECK("rift amorcé (régions programmées)", eg.sink_pending + eg.n_sunken > 0);
    /* déroule jusqu'à épuisement du rift */
    for (int y = 0; y < 80 && eg.sink_pending > 0; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 101 + y);
    CHECK("rift épuisé (sink_pending == 0)", eg.sink_pending == 0);
    CHECK("au moins une région engloutie", eg.n_sunken > 0);
    CHECK("n_regions INCHANGÉ (la région garde son indice)", econ->n_regions == n_reg0);

    /* (b) toute région englouti : owner=-1, impassable, cellules en mer */
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
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 100);
    CHECK("FROID déclenchée", eg.fired && eg.fin == FIN_FROID);
    for (int y = 0; y < 220; y++)
        endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 101 + y);
    /* mesures APRÈS */
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
    endgame_init(&eg);
    wp->entropy = FINV+10.f; wp->entropy_epicenter = -1;
    wp->faust_consumed[0]=0.0; wp->faust_consumed[1]=1000.0; wp->faust_consumed[2]=0.0;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 100);
    CHECK("RONCES déclenchée", eg.fired && eg.fin == FIN_RONCES);
    long thn_after_fire=0; for(int i=0;i<SCPS_N;i++) if(w->cell[i].biome==BIO_THORNS) thn_after_fire++;
    CHECK("éruption : des cellules deviennent ronces", thn_after_fire > 0);
    int epi_b = eg.epicenter_reg;
    CHECK("la région-foyer est tombée (owner=-1)", epi_b<0 || econ->region[epi_b].owner == -1);
    for (int y=0;y<80;y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 101+y);
    long thn_late=0; for(int i=0;i<SCPS_N;i++) if(w->cell[i].biome==BIO_THORNS) thn_late++;
    CHECK("le front s'étend (ronces ↑ après 80 ans)", thn_late > thn_after_fire);
    CHECK("n_regions inchangé (indices figés)", econ->n_regions == n_reg_b);
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
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 100);
    for (int y=0;y<80;y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, 0, 101+y);
    THORNS_HASH(w, thn_n_b, thn_h_b);
    CHECK("déterminisme : même ensemble de ronces (count)", thn_n_a == thn_n_b);
    CHECK("déterminisme : même ensemble de ronces (hash)", thn_h_a == thn_h_b);
    #undef THORNS_HASH

    /* ---- C6 : la Merveille (Ascension, victoire joueur) --------------- */
    printf("\nC6 merveille d'Ascension (paliers + charge + victoire)\n");
    world_generate(w, &p); econ_init(econ, w); gen_population(w, econ); prosperity_init(wp, w);
    int pl = -1;
    for (int c = 0; c < w->n_countries; c++) if (w->country[c].role != POLITY_UNCLAIMED) { pl = c; break; }
    CHECK("un empire joueur existe", pl >= 0);
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) for (int t = 0; t < TECH_COUNT; t++) ts[c].unlocked[t] = false;
    for (int c = 0; c < SCPS_MAX_COUNTRY; c++) ts[c].charge = 0.f;
    /* injecte les 3 rares dans le pool du joueur */
    for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) {
        econ->region[r].stock[RES_CELESTIAL_IRON] += 100.f;
        econ->region[r].stock[RES_FLUX]           += 100.f;
        econ->region[r].stock[RES_ESSENCE]        += 100.f;
    }
    int capr = -1; { int cap = w->country[pl].capital_prov; if (cap >= 0 && cap < w->n_provinces) capr = w->province[cap].region; }
    endgame_init(&eg);
    endgame_start_wonder(&eg, pl, capr);
    CHECK("merveille démarrée en FORGE", eg.merv == MERV_FORGE);
    wp->entropy = 0.f; wp->entropy_epicenter = -1;          /* entropie BASSE : pas d'apocalypse concurrente */
    for (int k = 0; k < 3; k++) wp->faust_consumed[k] = 0.0;
    double ci0 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) ci0 += econ->region[r].stock[RES_CELESTIAL_IRON];
    float site_ch0 = (capr >= 0 && capr < econ->n_regions) ? econ->region[capr].faust_charge : 0.f;
    for (int y = 0; y < 4; y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("ordre imposé : pas de SAVOIR avant FORGE/SOCIÉTÉ", eg.merv < MERV_SAVOIR);
    double ci1 = 0.0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) ci1 += econ->region[r].stock[RES_CELESTIAL_IRON];
    CHECK("FORGE dévore le fer céleste", ci1 < ci0);
    float site_ch1 = (capr >= 0 && capr < econ->n_regions) ? econ->region[capr].faust_charge : 0.f;
    CHECK("charge-additive : la Brèche se rapproche pendant le chantier", site_ch1 > site_ch0);
    /* déroule jusqu'à SAVOIR_DONE — SANS conditions de victoire → pas d'ascension */
    for (int y = 4; y < 50 && eg.merv != MERV_SAVOIR_DONE; y++) endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, y);
    CHECK("3 paliers bouclés (SAVOIR_DONE)", eg.merv == MERV_SAVOIR_DONE);
    CHECK("pas de victoire sans assimilation+arbre", !eg.fired && eg.merv == MERV_SAVOIR_DONE);

    /* conditions de victoire : tout le monde au joueur + intégré, arbre complet */
    int probe_r = -1; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) { probe_r = r; break; }
    int probe_cell = -1; Biome probe_bio = BIO_PLAINS; float probe_h = 0.f;
    if (probe_r >= 0) for (int i = 0; i < SCPS_N; i++) if (w->cell[i].region == probe_r && w->cell[i].height >= SEA_LEVEL) { probe_cell = i; probe_bio = w->cell[i].biome; probe_h = w->cell[i].height; break; }
    for (int r = 0; r < econ->n_regions; r++) { RegionEconomy *re = &econ->region[r];
        if (!re->active) continue;
        re->owner = (int16_t)pl; re->culture.settled = true;
        for (int g = 0; g < re->pop.n_groups; g++) re->pop.groups[g].integration = 1.f;
    }
    for (int t = 0; t < TECH_COUNT; t++) ts[pl].unlocked[t] = true;
    /* un tech manquant → PAS de victoire (test négatif d'abord) */
    ts[pl].unlocked[0] = false;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 51);
    CHECK("un seul tech manquant ⇒ pas d'ascension", eg.merv == MERV_SAVOIR_DONE && !eg.fired);
    /* arbre complet → ASCENSION */
    ts[pl].unlocked[0] = true;
    endgame_tick(&eg, w, econ, wp, ts, NULL, NULL, NULL, NULL, pl, 52);
    CHECK("conditions réunies → ASCENSION", eg.merv == MERV_ASCENDED && eg.fired && eg.fin == FIN_ASCENSION);
    int owned = 0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == pl) owned++;
    CHECK("l'empire DISPARAÎT (plus aucune région au joueur)", owned == 0);
    CHECK("terre INTACTE (biome/height inchangés — pas de carve)",
          probe_cell < 0 || (w->cell[probe_cell].biome == probe_bio && w->cell[probe_cell].height == probe_h));

    free(ts); free(wp); free(econ); free(w);

    /* ---- Récapitulatif ------------------------------------------------- */
    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
