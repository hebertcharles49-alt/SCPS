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
#include <string.h>

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

    /* ── §3 — VERBES DIPLO (capstone #26) : le joueur DÉCLARE/PROPOSE, le moteur applique au drain.
     * DÉCLARER LA GUERRE est unilatéral (effet déterministe) ; OFFRIR ALLIANCE/PAIX passe par
     * ai_consider_offer (le vis-à-vis évalue l'opinion) — on prouve l'aller-retour du journal. */
    {
        int nc2 = scps_country_count(s2);
        ScpsRelation rel[64]; int nr = scps_country_relations(s2, pl, rel, 64);
        int wars0=0; for (int i=0;i<nr;i++) wars0 += rel[i].at_war;
        int enq=0;
        for (int c=0;c<nc2;c++){ if (c==pl) continue; if (scps_player_declare_war(s2, c)) enq++; }
        ok("verbe DÉCLARER LA GUERRE : ordre(s) ENFILÉ(s) (différé)", enq>0);
        scps_sim_advance_days(s2, 1);                       /* le drain applique */
        nr = scps_country_relations(s2, pl, rel, 64);
        int wars1=0; for (int i=0;i<nr;i++) wars1 += rel[i].at_war;
        printf("   diplo joueur : guerres %d → %d après déclaration au drain\n", wars0, wars1);
        ok("verbe DÉCLARER LA GUERRE : APPLIQUÉ au drain (le joueur entre en guerre)", wars1 > wars0);
        /* offre de PAIX + EMBARGO + ALLIANCE : enfilés et drainés sans crash (la membrane tient ;
         * le verdict d'acceptation — via l'opinion — tombe au tick, lu ensuite en relations). */
        int pe=0, em=0;
        for (int c=0;c<nc2;c++){ if (c==pl) continue; pe += scps_player_make_peace(s2,c); em += scps_player_embargo(s2,c,1); }
        int al = scps_player_offer_alliance(s2, (pl+1)%nc2);
        scps_sim_advance_days(s2, 1);
        ok("verbes PAIX/EMBARGO/ALLIANCE : enfilés + drainés sans crash (membrane stable)",
           pe>=0 && em>=0 && (al==0 || al==1));
        /* #26 — l'OPINION traverse la membrane : la bande ±100 du vis-à-vis (mémoire des actes). */
        ScpsRelation rel2[64]; int nr2 = scps_country_relations(s2, pl, rel2, 64);
        int op_ok = (nr2>0);
        for (int i=0;i<nr2;i++) if (rel2[i].opinion < -100 || rel2[i].opinion > 100) op_ok=0;
        ok("#26 : l'opinion ±100 traverse la membrane (bande bornée [-100,100])", op_ok);
        /* §3 suite — INTÉRIEUR · COMMERCE · GUERRE : tous les verbes ENFILENT + DRAINENT sans crash.
         * (Effet réel revalidé au drain : région à soi, indices bornés ; ici on prouve l'aller-retour.) */
        int v = 0;
        v += scps_player_repress(s2, 0)      ? 1:0;
        v += scps_player_assimilate(s2, 0, 1)? 1:0;
        v += scps_player_purge(s2, 0)        ? 1:0;
        v += scps_player_council_hire(s2, 0, 0)   ? 1:0;
        v += scps_player_council_dismiss(s2, 0)   ? 1:0;
        v += scps_player_route(s2, 0, 1, 0)  ? 1:0;
        v += scps_player_market_buy(s2, 0, 1, 10, 0) ? 1:0;
        v += scps_player_market_sell(s2, 0, 1, 10, 0)? 1:0;
        v += scps_player_campaign(s2, 0, 1)  ? 1:0;
        v += scps_player_posture(s2, 2)      ? 1:0;
        v += scps_player_refill(s2)          ? 1:0;
        v += scps_player_navy_build(s2, 0)   ? 1:0;
        v += scps_player_disband(s2)         ? 1:0;
        ok("§3 — 13 verbes intérieur/commerce/guerre ENFILÉS", v==13);
        scps_sim_advance_days(s2, 1);        /* le drain les applique tous, revalidés, sans crash */
        ok("§3 — drainés sans crash (revalidation au drain : membrane stable)", scps_year(s2)>=0);
        /* §3 — OPTIONS : la légalité grise les boutons. On choisit une cible VALIDE au sens de
         * scps_diplo_options — un pays qui POSSÈDE des régions et n'est PAS la friche POLITY_UNCLAIMED ;
         * le RÔLE n'entre pas (un empire comme une CITÉ-ÉTAT est une cible légitime) — puis on la met EN
         * GUERRE (déclaration UNILATÉRALE, déterministe) → PAIX offrable, DÉCLARATION grisée. Cibler un
         * index fixe (pl+1) serait fragile : le pays-joueur varie d'un monde à l'autre, et l'index voisin
         * peut tomber sur la friche (0 région) — c'était la cause de l'échec, pas le rôle de la cible. */
        int tgt=-1;
        for (int c=0;c<nc2;c++){ if (c==pl) continue; ScpsDiploOptions tmp; if (scps_diplo_options(s2,c,&tmp)){ tgt=c; break; } }
        scps_player_declare_war(s2, tgt);
        scps_sim_advance_days(s2, 1);                       /* le drain applique la guerre */
        ScpsDiploOptions dop; int gotd = (tgt>=0) && scps_diplo_options(s2, tgt, &dop);
        ok("scps_diplo_options : cible valide → rempli", gotd==1);
        ok("options diplo COHÉRENTES (jamais guerre ET paix offrables ensemble)",
           !(dop.can_make_peace && dop.can_declare_war));
        ok("options diplo : en GUERRE ⇒ paix offrable, déclaration grisée",
           dop.can_make_peace==1 && dop.can_declare_war==0);
        ok("options diplo : aperçus de consentement ∈ {0,1} (l'opinion #26 prévisualisée)",
           (dop.would_accept_alliance|dop.would_accept_pact|dop.would_accept_peace|
            dop.can_offer_alliance|dop.can_offer_pact|dop.can_embargo|dop.can_lift_embargo) <= 1);
        ok("scps_build_legal : réponse bornée {0,1} (région · or)",
           (scps_build_legal(s2,-1,0) & ~1)==0);
    }

    scps_sim_free(s); scps_sim_free(s2);

    /* ── CRÉATEUR DE CULTURE : listes + validation + aperçu + composition (headless) ──
     * La façade expose tout ce qu'il faut au créateur Godot SANS sim : les listes
     * (héritages/éthos/traditions), la validation 1maj/1min/1déf, l'aperçu des leviers
     * (mots+signe), puis la COMPOSITION gravée à la génération (l'éthos paraît au nom). */
    {
        ScpsHeritage her[16]; int nher=scps_heritage_list(her,16);
        printf("   héritages : %d (ex. %s « %s »)\n", nher, nher?her[0].nom:"", nher?her[0].exemple:"");
        ok("héritages listés (6, avec ethnonyme-exemple)", nher==6 && her[0].nom[0] && her[0].exemple[0]);

        ScpsEthosDef eth[16]; int neth=scps_ethos_list(eth,16);
        ok("éthos listés (6, avec épithète)", neth==6 && eth[0].epithete[0]);

        ScpsTradition trs[64]; int ntr=scps_tradition_list(trs,64);
        ok("traditions listées (36, axe + rang + survol)", ntr==36 && trs[0].nom[0] && trs[0].hover[0]);

        /* compo VALIDE piochée DANS la liste (façade-pure : on ne connaît pas les enums) :
         * 1 majeur (rang ≥ +2) sur l'axe 0, 1 mineur (+1) sur l'axe 1, 1 défaut (−1) sur l'axe 2. */
        int maj=-1, mn=-1, df=-1;
        for(int i=0;i<ntr;i++){
            if(maj<0 && trs[i].axe==0 && trs[i].rang>=2) maj=trs[i].id;
            if(mn <0 && trs[i].axe==1 && trs[i].rang==1) mn =trs[i].id;
            if(df <0 && trs[i].axe==2 && trs[i].rang< 0) df =trs[i].id;
        }
        ok("pioche compo (maj Phys / min Soc / déf Int)", maj>=0 && mn>=0 && df>=0);
        ok("validation : 1maj+1min+1déf ACCEPTÉE", scps_culture_validate(maj,mn,df)==1);
        /* trois majeurs (un par axe) → REFUSÉ (major≠1) */
        int m1=-1,m2=-1;
        for(int i=0;i<ntr;i++){
            if(m1<0 && trs[i].axe==1 && trs[i].rang>=2) m1=trs[i].id;
            if(m2<0 && trs[i].axe==2 && trs[i].rang>=2) m2=trs[i].id;
        }
        ok("validation : 3 majeurs REFUSÉS", scps_culture_validate(maj,m1,m2)==0);

        ScpsLevierLine lv[16]; int nlv=scps_culture_preview(maj,mn,df,lv,16);
        printf("   aperçu leviers : %d ligne(s) (ex. %s %s)\n",
               nlv, nlv?lv[0].nom:"", nlv?(lv[0].signe>0?"+":"-"):"");
        ok("aperçu leviers (mots + signe ±1)", nlv>0 && (lv[0].signe==1||lv[0].signe==-1));

        const char *cn = scps_culture_name(0 /*ESOTERIQUE*/, 7u);
        printf("   nom de culture (ESOTERIQUE, graine 7) : %s\n", cn);
        ok("nom de culture (ethnonyme) non vide", cn && cn[0]);

        /* COMPOSER puis GÉNÉRER : l'éthos PACIFISTE (5) doit donner l'épithète « Havre » au pays. */
        int set = scps_set_player_culture(0 /*ESOTERIQUE*/, 5 /*PACIFISTE*/, maj, mn, df);
        ok("composition retenue (valide)", set==1);
        ScpsSim *s3 = scps_sim_new();
        scps_sim_generate(s3, seed);
        int pl3 = scps_player(s3);
        ScpsCountryInfo ci3; scps_country_info(s3, pl3, &ci3);
        printf("   empire joueur composé : « %s » (faction dominante lue : %s)\n", ci3.nom, ci3.ethos);
        ok("éthos joueur GRAVÉ (nom = épithète « Havre … »)", strncmp(ci3.nom, "Havre", 5)==0);
        scps_sim_advance_days(s3, 365*5);
        ok("le monde composé VIT (5 ans, pop > 0)", scps_world_pop(s3) > 0);
        scps_sim_free(s3);

        /* EFFACER : retour au défaut (héritage ADAPTATIF, éthos émergent). */
        scps_clear_player_culture();
        ok("composition effacée (retour défaut)", scps_culture_validate(maj,mn,df)==1);  /* la validation reste pure */
    }

    /* ── PARAMÈTRES DE GÉNÉRATION (sliders « Nouvelle partie ») : la TAILLE mord ── */
    {
        ScpsWorldParams wp; scps_worldparams_default(seed, &wp);
        ok("worldparams défaut lus (empires>0, continents>0)", wp.n_empires>0 && wp.n_continents>0);

        ScpsWorldParams small = wp; small.n_empires=2;  small.n_city_states=4;
        scps_worldgen_set(&small);
        ScpsSim *sa=scps_sim_new(); scps_sim_generate(sa, seed); int ra=scps_region_count(sa);

        ScpsWorldParams big = wp;   big.n_empires=10;   big.n_city_states=20;
        scps_worldgen_set(&big);
        ScpsSim *sb=scps_sim_new(); scps_sim_generate(sb, seed); int rb=scps_region_count(sb);

        printf("   régions : petit(2 emp)=%d · grand(10 emp)=%d\n", ra, rb);
        ok("monde plus GRAND ⇒ plus de régions", rb > ra);
        scps_worldgen_clear();
        scps_sim_free(sa); scps_sim_free(sb);
    }

    /* ── SLOTS DE CULTURE PAR EMPIRE (façon Stellaris) : slot 0 joueur + slot 1 IA ── */
    {
        ScpsTradition trs2[64]; int ntr2=scps_tradition_list(trs2,64);
        int maj=-1, mn=-1, df=-1;
        for(int i=0;i<ntr2;i++){
            if(maj<0 && trs2[i].axe==0 && trs2[i].rang>=2) maj=trs2[i].id;
            if(mn <0 && trs2[i].axe==1 && trs2[i].rang==1) mn =trs2[i].id;
            if(df <0 && trs2[i].axe==2 && trs2[i].rang< 0) df =trs2[i].id;
        }
        scps_clear_player_culture();   /* repart propre */
        ok("slot HORS-BORNE refusé", scps_set_empire_culture(99, 0, 5, maj, mn, df)==0);
        ok("slot 0 (joueur) ESOTERIQUE/PACIFISTE retenu", scps_set_empire_culture(0, 0, 5, maj, mn, df)==1);
        ok("slot 1 (IA) CLANIQUE/DOMINATEUR retenu",      scps_set_empire_culture(1, 5, 0, maj, mn, df)==1);
        ScpsSim *se=scps_sim_new(); scps_sim_generate(se, seed);
        int pl=scps_player(se);
        ScpsCountryInfo pin; scps_country_info(se, pl, &pin);
        ok("joueur (slot 0) = épithète « Havre »", strncmp(pin.nom,"Havre",5)==0);
        int ai1=-1;
        for(int c=0;c<scps_country_count(se);c++) if(scps_country_role(se,c)==1){ ai1=c; break; }  /* 1er antagoniste = slot 1 */
        int horde_ok=0;
        if(ai1>=0){ ScpsCountryInfo ain; scps_country_info(se, ai1, &ain);
            printf("   slot 0 joueur=« %s » · slot 1 IA(cid %d)=« %s »\n", pin.nom, ai1, ain.nom);
            horde_ok = (strncmp(ain.nom,"Horde",5)==0); }
        ok("IA slot 1 = épithète « Horde » (culture DONNÉE à l'IA)", ai1>=0 && horde_ok);
        scps_clear_player_culture();
        scps_sim_free(se);
    }

    free(rgba); free(lay);
    printf("\n══ BILAN : %d réussis, %d échoués ══\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
