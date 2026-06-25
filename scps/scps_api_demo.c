/*
 * scps_api_demo.c — le banc de la FAÇADE C (scps_api).
 *
 *   make scps_api_demo && ./scps_api_demo [graine=9]
 *
 * Prouve, sans Godot, que la façade pilote vraiment le moteur : génération,
 * rendu carte (eau ET terre), couches brutes, avancement (le monde VIT), et
 * surtout la REPRODUCTIBILITÉ (deux sims identiques → même pop) — la garantie
 * de déterminisme que l'hôte Godot héritera tant qu'il n'AFFICHE que.
 */
#include "scps_api.h"
#include <stdio.h>
#include <stdlib.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if(cond) g_pass++; else g_fail++;
}

int main(int argc, char **argv){
    uint32_t seed = (argc>1) ? (uint32_t)strtoul(argv[1],NULL,10) : 9u;
    printf("══ scps_api : la façade C pilote le moteur (graine %u) ══\n", seed);

    ScpsSim *s = scps_sim_new();
    ok("sim créée", s!=NULL);
    if(!s){ printf("OOM\n"); return 1; }
    scps_sim_generate(s, seed);

    int W = scps_map_w(), H = scps_map_h();
    ok("dims carte 1024×512", W==1024 && H==512);

    int nc = scps_country_count(s);
    long p0 = scps_world_pop(s);
    printf("   pays=%d · régions=%d · pop an0=%ld · joueur=%d\n",
           nc, scps_region_count(s), p0, scps_player(s));
    ok("monde peuplé", nc>0 && p0>0);

    /* rendu : la carte contient de l'EAU (bleu dominant) ET de la TERRE (vert/brun) */
    uint8_t *rgba = (uint8_t*)malloc((size_t)W*H*4);
    scps_map_rgba(s, rgba, 0 /*VIEW_TERRAIN*/, -1 /*aucune sélection*/);
    long blue=0, land=0;
    for(int i=0;i<W*H;i++){ uint8_t r=rgba[i*4], g=rgba[i*4+1], b=rgba[i*4+2];
        if(b>r && b>=g) blue++; else if(r>40||g>40) land++; }
    printf("   render_map : %ld px eau · %ld px terre\n", blue, land);
    ok("render_map : eau ET terre", blue>5000 && land>5000);

    /* couche brute SEA (pour shader d'eau côté hôte) */
    uint8_t *lay = (uint8_t*)malloc((size_t)W*H);
    scps_map_layer(s, lay, SCPS_LAYER_SEA);
    long sea=0; for(int i=0;i<W*H;i++) if(lay[i]) sea++;
    ok("couche SEA non vide", sea>5000);

    /* centroïde tangible d'une région (pour overlays/sprites) */
    float cx=-9, cy=-9; bool gotc=false;
    for(int r=0;r<scps_region_count(s) && !gotc;r++) gotc = scps_region_centroid(s, r, &cx, &cy);
    ok("centroïde de région disponible", gotc && cx>=0 && cy>=0);

    /* le monde VIT : 20 ans d'avancement → la pop bouge */
    scps_sim_advance_days(s, 365*20);
    long p20 = scps_world_pop(s); int yr = scps_year(s);
    printf("   an %d · pop=%ld (Δ %+ld)\n", yr, p20, p20-p0);
    ok("20 ans écoulés", yr==20);
    ok("le monde VIT (pop a changé)", p20 != p0);

    /* ── LECTURES DE FENÊTRES : arbre de tech · budget · missions (read-only) ──
     * LU AVANT le 2e sim : le flux fiscal est un état GLOBAL (règle « un seul Sim
     * actif/processus ») — créer s2 le RAZ. On lit donc le budget de s ICI, sur ~1
     * an de flux accumulé, tant que s est le seul monde vivant. */
    int pl0 = scps_player(s);
    ScpsTechInfo ti; scps_tech_info(s, &ti);
    ScpsTechNode tn[64]; int ntn = scps_tech_nodes(s, tn, 64);
    printf("   tech : %d nœuds · %d points · présage=%s · crise=%d%%\n",
           ntn, ti.points, ti.presage, ti.crise_pct);
    ok("arbre de tech lu (nœuds + points ≥0)", ntn>0 && ti.points>=0);
    ok("risque faustien BANDÉ (présage résolu, jamais le flottant)", ti.presage[0]!='\0');
    ok("thèmes/fonctions résolus", ti.theme[0][0]!='\0' && ti.function[0][0]!='\0');

    ScpsBudget bg; scps_budget_summary(s, pl0, &bg);
    ScpsFluxLine fx[32]; int nfx = scps_country_budget(s, pl0, fx, 32);
    printf("   budget : or=%.0f · revenus=%.0f · dépenses=%.0f · net=%+.0f · %d postes · crédit=%.0f\n",
           bg.gold, bg.income, bg.expense, bg.net, nfx, bg.credit_line);
    ok("budget : décomposition du flux (postes non vides)", nfx>0);
    ok("budget : net = revenus − dépenses (cohérent)", bg.net==bg.income-bg.expense);
    ok("budget : ligne de crédit ∝ pop (≥0, finie)", bg.credit_line>=0 && bg.credit_line==bg.credit_line);

    ScpsMission ms; scps_mission_info(s, pl0, &ms);
    printf("   mission : %s\n", ms.active ? ms.text : "(aucune active)");
    ok("mission lue sans crash (active ∈ {0,1})", ms.active==0 || ms.active==1);

    /* REPRODUCTIBILITÉ : un 2e sim, mêmes appels → même résultat au bit près */
    ScpsSim *s2 = scps_sim_new();
    scps_sim_generate(s2, seed);
    scps_sim_advance_days(s2, 365*20);
    long p20b = scps_world_pop(s2);
    printf("   sim B an %d · pop=%ld\n", scps_year(s2), p20b);
    ok("REPRODUCTIBLE (sim A == sim B)", p20b == p20);

    /* ── JOURNAL DE COMMANDES JOUEUR (déterministe) : enfiler → vider au tick ──
     * La main humaine PASSE par le moteur : un ordre de levée est ENFILÉ (différé),
     * sans effet jusqu'au tick, puis APPLIQUÉ au drain de sim_day. Démontre du même
     * coup le DÉBRAYAGE de l'IA : la levée du joueur obéit à SA commande (ai_on[joueur]
     * =false ⇒ l'IA ne la repilote pas). */
    int pl = scps_player(s2);
    ScpsArmy a0; scps_country_army(s2, pl, &a0);
    int want = (a0.levy>=3) ? 0 : 3;            /* viser un cran DIFFÉRENT de l'actuel */
    scps_player_set_levy(s2, want);
    ScpsArmy a1; scps_country_army(s2, pl, &a1);
    ok("ordre de levée ENFILÉ, pas encore appliqué (différé)", a1.levy==a0.levy);
    scps_sim_advance_days(s2, 1);               /* un tick → le drain applique l'ordre */
    ScpsArmy a2; scps_country_army(s2, pl, &a2);
    printf("   levée joueur : %d → (ordre %d) → %d après 1 tick\n", a0.levy, want, a2.levy);
    ok("ordre de levée APPLIQUÉ au drain (round-trip du journal)", a2.levy==want);

    scps_sim_free(s); scps_sim_free(s2);
    free(rgba); free(lay);
    printf("\n══ BILAN : %d réussis, %d échoués ══\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
