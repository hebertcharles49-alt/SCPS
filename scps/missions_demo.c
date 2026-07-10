/*
 * missions_demo.c — LES MISSIONS DÉCENNALES (factions §8)
 *
 *   make missions_demo && ./missions_demo [graine]
 *
 * Prouve : une mission CONTEXTUELLE est émise à la décennie ; l'accomplir VERSE
 * or + matières ; le kind tourne (bâtir / chaîne / tech) ; texte diégétique.
 */
#include "scps_missions.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_tune.h"      /* P3 : COUNCIL_MISSION_* (registre J) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, int cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static bool near_f(float a, float b, float eps){ float d=a-b; if(d<0)d=-d; return d<=eps; }

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World)); WorldEconomy *econ=malloc(sizeof(WorldEconomy));
    TechState *ts=calloc(SCPS_MAX_COUNTRY,sizeof(TechState));
    MissionsState *ms=malloc(sizeof(MissionsState));
    Statecraft *sc=malloc(sizeof(Statecraft));   /* P3 : le siège responsable */
    if(!w||!econ||!ts||!ms||!sc){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" MISSIONS DÉCENNALES (factions §8) — contexte, récompense, rythme\n");
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) tech_state_init(&ts[c],false);
    missions_init(ms);
    statecraft_init(sc,w);   /* P3 : les sièges du Conseil (tous vacants au départ) */

    /* Un pays réel avec une capitale. */
    int cid=-1;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        int cp=w->country[c].capital_prov; if (cp<0) continue;
        int cr=w->province[cp].region;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c && cr>=0){ cid=c; break; }
        if (cid>=0) break;
    }
    if (cid<0){ fprintf(stderr,"monde trop vide — autre graine\n"); return 1; }
    int cr=w->province[w->country[cid].capital_prov].region;

    /* ═══ P3 — LE SIÈGE RESPONSABLE, DÉDUIT DU TYPE (aucun état neuf) ═══ */
    printf("\n── P3. Le siège responsable se déduit du type ──\n");
    { Mission mm={0};
      mm.kind=MIS_TECH;  ok("MIS_TECH → siège Savoir (0)", mission_responsible_seat(&mm)==0);
      mm.kind=MIS_CHAIN; ok("MIS_CHAIN → siège Ouvrages (2)", mission_responsible_seat(&mm)==2);
      mm.kind=MIS_BUILD; mm.coord=MIS_COORD_SAVOIR; ok("MIS_BUILD savoir → Savoir (0)", mission_responsible_seat(&mm)==0);
      mm.coord=MIS_COORD_FAITH;  ok("MIS_BUILD foi → Savoir (0)",      mission_responsible_seat(&mm)==0);
      mm.coord=MIS_COORD_K;      ok("MIS_BUILD institutions → Royaume (1)", mission_responsible_seat(&mm)==1);
      mm.coord=MIS_COORD_H;      ok("MIS_BUILD garde → Royaume (1)",   mission_responsible_seat(&mm)==1);
      mm.coord=MIS_COORD_FOOD;   ok("MIS_BUILD vivres → Royaume (1)",  mission_responsible_seat(&mm)==1);
      mm.coord=MIS_COORD_PE;     ok("MIS_BUILD commerce → Ouvrages (2)", mission_responsible_seat(&mm)==2);
      mm.kind=MIS_NONE; ok("aucune mission → -1", mission_responsible_seat(&mm)==-1);
      ok("mission_responsible_seat(NULL) → -1", mission_responsible_seat(NULL)==-1);
    }

    /* ═══ 1. ÉMISSION DÉCENNALE ════════════════════════════════════════ */
    printf("\n── 1. À la décennie, une mission contextuelle est émise ──\n");
    missions_tick(ms, w, econ, ts, NULL, NULL, seed, 0);   /* an 0 : décennie → émission (sc=NULL : pas de Conseil ici) */
    const Mission *m = mission_of(ms, cid);
    ok("une mission est émise au pays (an 0, décennie)", m!=NULL && m->active);
    ok("la mission porte un texte diégétique (membrane)", m && m->text[0]!=0);
    if (m) printf("     « %s »  (récompense : %.0f or + %.0f matières)\n",
                  m->text, m->reward_gold, m->reward_qty);
    ok("an 0 (décennie 0) → mission de BÂTI (le kind tourne)", m && m->kind==MIS_BUILD);

    /* ═══ 2. ACCOMPLISSEMENT → RÉCOMPENSE OR + MATIÈRES (+ BONUS/LOYAUTÉ P3) ══ */
    printf("\n── 2. Accomplir la mission verse or + matières (bonus + loyauté du siège, P3) ──\n");
    int seatB = mission_responsible_seat(m);
    ok("P3 : le siège responsable de la mission an-0 est un siège valide (0..2)", seatB>=0 && seatB<SC_COUNCIL_SEATS);
    /* pourvoit ce siège du MEILLEUR candidat (tier le plus haut) — le bonus se voit. */
    int bestB=0, btB=0;
    for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatB,sl,0); if(t>btB){btB=t;bestB=sl;} }
    statecraft_council_hire(sc, seed, cid, seatB, bestB, 0);
    int loyB_before = statecraft_council_loyalty(sc,cid,seatB);
    /* RE-KEY PROVINCE : mission_grant route le trésor sur la province représentative
     * (treasury province-owned) — on lit/pose donc au même grain (comme econ_tax_demo.c). */
    int crp=econ_region_rep_province(econ,cr);
    float tre0=econ->prov[crp].treasury;
    Resource rmat = m?m->reward_mat:RES_NONE;
    float mat0 = (rmat>RES_NONE)?econ->region[cr].stock[rmat]:0.f;
    float gold_base = m?m->reward_gold:0.f;
    /* on SATISFAIT la mission de bâti : toutes les coordonnées bâties au-delà du seuil. */
    ProvBuild *b=&econ->region[cr].build;
    b->K_inst=b->PE_infra=b->faith=b->savoir=b->H_coerc=b->food_cap=50.f;
    missions_tick(ms, w, econ, ts, sc, NULL, seed, 1);     /* an 1 : vérifie → accomplie (siège pourvu → bonus + loyauté) */
    const Mission *m2=mission_of(ms,cid);
    ok("la mission est marquée ACCOMPLIE une fois la cible atteinte", m2 && m2->done);
    ok("la récompense en OR a abondé le trésor de la capitale", econ->prov[crp].treasury > tre0 + 1.f);
    ok("la récompense en MATIÈRES a abondé le marché",
       rmat<=RES_NONE || econ->region[cr].stock[rmat] > mat0 + 1.f);
    int loyB_after = statecraft_council_loyalty(sc,cid,seatB);
    printf("   Siège responsable (%d) : loyauté %d → %d (+%.0f attendu, réussite)\n",
           seatB, loyB_before, loyB_after, tune_f("COUNCIL_MISSION_SUCCESS_LOYALTY",5.f));
    ok("P3 : la réussite verse +COUNCIL_MISSION_SUCCESS_LOYALTY au siège responsable",
       loyB_after == loyB_before + (int)tune_f("COUNCIL_MISSION_SUCCESS_LOYALTY",5.f));
    if (btB>1)
        ok("P3 : un siège pourvu (rang>I) verse un BONUS — la récompense DÉPASSE la base",
           econ->prov[crp].treasury > tre0 + gold_base + 1.f);
    else
        ok("P3 : rang I → bonus nul (PER_RANK×(rang−1)=0), la récompense reste la base",
           near_f(econ->prov[crp].treasury, tre0 + gold_base, 2.f));

    /* ═══ 3. LE KIND TOURNE PAR DÉCENNIE (chaîne, puis tech) + ÉCHEC RÉSOLU (P3) ═ */
    printf("\n── 3. Le kind tourne : chaîne (décennie 1), tech (décennie 2) — l'échec est RÉSOLU avant le remplacement ──\n");
    missions_tick(ms, w, econ, ts, sc, NULL, seed, 10);    /* décennie 1 → CHAÎNE */
    const Mission *mc=mission_of(ms,cid);
    ok("décennie 1 (an 10) → mission de CHAÎNE de production", mc && mc->kind==MIS_CHAIN);
    if (mc) printf("     chaîne : « %s »\n", mc->text);
    int seatC = mission_responsible_seat(mc);
    ok("P3 : MIS_CHAIN engage TOUJOURS le siège des Ouvrages (2)", seatC==2);
    int bestC=0, btC=0;
    for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatC,sl,statecraft_council_gen(10)); if(t>btC){btC=t;bestC=sl;} }
    statecraft_council_hire(sc, seed, cid, seatC, bestC, statecraft_council_gen(10));
    int loyC_before = statecraft_council_loyalty(sc,cid,seatC);
    /* aucune matière RES_TOOLS n'est ajoutée entre-temps : la mission de CHAÎNE
     * reste INACHEVÉE — un échec au remplacement décennal suivant, GARANTI. */
    missions_tick(ms, w, econ, ts, sc, NULL, seed, 20);    /* décennie 2 : l'échec est résolu ICI, avant le remplacement */
    const Mission *mt=mission_of(ms,cid);
    ok("décennie 2 (an 20) → mission de TECHNOLOGIE", mt && mt->kind==MIS_TECH);
    if (mt) printf("     tech   : « %s »\n", mt->text);
    int loyC_after = statecraft_council_loyalty(sc,cid,seatC);
    printf("   Siège Ouvrages : loyauté %d → %d (échec de la mission de CHAÎNE, décennie 1)\n", loyC_before, loyC_after);
    ok("P3 : une mission décennale INACHEVÉE pénalise le siège responsable (−COUNCIL_MISSION_FAILURE_LOYALTY, "
       "résolu AVANT le remplacement — fin du remplacement silencieux)",
       loyC_after == loyC_before - (int)tune_f("COUNCIL_MISSION_FAILURE_LOYALTY",10.f));

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w);free(econ);free(ts);free(ms);free(sc);
    return g_fail?1:0;
}
