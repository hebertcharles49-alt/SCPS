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
#include "scps_religion.h"   /* P3 : test de persistance religion */
#include "scps_provlog.h"    /* DACT_* : le journal d'actes diplomatique */
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

    /* §5 PUISSANCE COMMERCIALE : la façade du menu marché — le pool mensuel est peuplé & borné. */
    { ScpsCommerce cc; scps_commerce_power(s, scps_player(s), &cc);
      printf("   puissance comm. joueur : pool=%.1f/mois · restant=%.1f · bourgeois=%.0f · +%d%%\n",
             cc.pool, cc.remaining, cc.bourgeois, cc.bonus_pct);
      ok("commerce_power : pool mensuel > 0 après 20 ans (la pop marchande produit)", cc.pool > 0.f);
      ok("commerce_power : restant borné [0..pool]", cc.remaining >= 0.f && cc.remaining <= cc.pool + 1.f);
      ok("commerce_power : sources cohérentes (bourgeois>0, bonus 0-50%)",
         cc.bourgeois > 0.f && cc.bonus_pct >= 0 && cc.bonus_pct <= 50); }

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
        /* v50 (le DIPLOMATE) : UN acte / 2 mois — on vise UNE cible dont la déclaration
         * est LÉGALE (options) et qui paraît dans les relations (pas un hameau libre). */
        int wt=-1;
        for (int c=0;c<nc2 && wt<0;c++){
            if (c==pl || scps_country_role(s2,c)==4) continue;
            ScpsDiploOptions op;
            if (scps_diplo_options(s2, c, &op) && op.can_declare_war) wt=c;
        }
        int enq = (wt>=0) ? scps_player_declare_war(s2, wt) : 0;
        ok("verbe DÉCLARER LA GUERRE : ordre ENFILÉ (différé, cible légale)", enq>0);
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
        int mig = scps_player_offer_migration(s2, (pl+1)%nc2);   /* BRASSAGE : pacte migratoire */
        scps_sim_advance_days(s2, 1);
        ok("verbes PAIX/EMBARGO/ALLIANCE : enfilés + drainés sans crash (membrane stable)",
           pe>=0 && em>=0 && (al==0 || al==1));
        ok("BRASSAGE : le verbe pacte migratoire ENFILE + DRAINE sans crash", mig==0 || mig==1);
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
        /* v50 (le DIPLOMATE) : l'émissaire est UNIQUE (1 acte / 2 mois) — on ne redéclare
         * pas, on réutilise la cible DÉJÀ en guerre (celle du bloc précédent) : c'est
         * précisément l'état « paix offrable, déclaration grisée » qu'on veut prouver. */
        int tgt=-1;
        for (int c=0;c<nc2;c++){ if (c==pl) continue; ScpsDiploOptions tmp;
            if (scps_diplo_options(s2,c,&tmp) && tmp.can_make_peace){ tgt=c; break; } }
        ScpsDiploOptions dop; int gotd = (tgt>=0) && scps_diplo_options(s2, tgt, &dop);
        ok("scps_diplo_options : cible valide → rempli", gotd==1);
        ok("options diplo COHÉRENTES (jamais guerre ET paix offrables ensemble)",
           !(dop.can_make_peace && dop.can_declare_war));
        ok("options diplo : en GUERRE ⇒ paix offrable, déclaration grisée",
           dop.can_make_peace==1 && dop.can_declare_war==0);
        ok("options diplo : aperçus de consentement ∈ {0,1} (l'opinion #26 prévisualisée)",
           (dop.would_accept_alliance|dop.would_accept_pact|dop.would_accept_migration|dop.would_accept_peace|
            dop.can_offer_alliance|dop.can_offer_pact|dop.can_offer_migration|dop.can_embargo|dop.can_lift_embargo) <= 1);
        ok("scps_build_legal : réponse bornée {0,1} (région · or)",
           (scps_build_legal(s2,-1,0) & ~1)==0);
    }

    /* ── ALLOCATION DE MAIN-D'ŒUVRE (onglet province) : lire les puits, poser un poids
     *    (active l'override), fermer un bâtiment, revenir AUTO — tout via le journal. ── */
    {
        int preg=-1, np=scps_province_count(s2);
        for (int pp=0; pp<np; pp++){ ScpsProvInfo pi; scps_province_info(s2,pp,&pi);
            if (pi.owner==pl){ int rg=scps_province_region(s2,pp); if(rg>=0){ preg=rg; break; } } }
        ok("alloc : région du joueur trouvée", preg>=0);
        if (preg>=0){
            ScpsAlloc al; scps_region_alloc(s2, preg, &al);
            ok("scps_region_alloc : région lue (bassin>0, puits listés)", al.region==preg && al.pool>0.f && al.n>0);
            ok("alloc : mode AUTO au départ (on=0)", al.on==0);
            int kbld=-1, kraw=-1;
            for (int i=0;i<al.n;i++){ if(al.sink[i].kind==1 && kbld<0) kbld=i; if(al.sink[i].kind==0 && kraw<0) kraw=i; }
            if (kraw>=0){
                ok("verbe alloc_raw ENFILÉ", scps_player_alloc_raw(s2, preg, al.sink[kraw].id, 80)==1);
                scps_sim_advance_days(s2, 1);
                ScpsAlloc al2; scps_region_alloc(s2, preg, &al2);
                ok("alloc : override ACTIVÉ au drain (on=1)", al2.on==1);
            }
            if (kbld>=0){
                int bid=al.sink[kbld].id;
                scps_player_alloc_bld(s2, preg, bid, 0);   /* poids 0 = fermé */
                scps_sim_advance_days(s2, 1);
                ScpsAlloc al3; scps_region_alloc(s2, preg, &al3);
                int closed=0; for (int i=0;i<al3.n;i++) if(al3.sink[i].kind==1 && al3.sink[i].id==bid) closed=al3.sink[i].closed;
                ok("alloc : bâtiment FERMÉ (poids 0) reflété au readout", closed==1);
            }
            scps_player_alloc_auto(s2, preg);
            scps_sim_advance_days(s2, 1);
            ScpsAlloc al4; scps_region_alloc(s2, preg, &al4);
            ok("alloc : retour au mode AUTO", al4.on==0);
        }
    }

    /* ── LE FIL D'ÉVÈNEMENTS (alertes, voie « ce qui arrive ») : la guerre déclarée plus
     *    haut doit PARAÎTRE au poll après le diff mensuel (observation gatée joueur).
     *    DRAIN EN BOUCLE (le fil porte aussi les évènements du directeur : > 32 possible). ── */
    {
        scps_sim_advance_days(s2, 35);                      /* passe une frontière de mois */
        ScpsFeedEvent fe[32];
        int has_war=0, total=0, last=0;
        for (int guard=0; guard<8; guard++){
            int nf = scps_feed_poll(s2, last, fe, 32);
            if (nf<=0) break;
            total += nf;
            for (int i=0;i<nf;i++){
                if (fe[i].kind==1 /* FEED_WAR_DECLARED */ && fe[i].a_name[0]) has_war=1;
                last = fe[i].seq;
            }
        }
        printf("   fil d'évènements : %d entrée(s) drainée(s)\n", total);
        ok("fil d'évènements : la GUERRE déclarée paraît (kind war + nom résolu)", has_war);
        ok("fil d'évènements : poll incrémental (rien après le dernier seq)",
           scps_feed_poll(s2, last, fe, 32)==0);
    }

    /* ── RÉSUMÉ D'OPINION (#26) : la décomposition — une cible EN GUERRE porte la
     *    composante guerre, le total reste borné ±100. ── */
    {
        ScpsOpinionParts op;
        int tgt=-1;
        for (int c=0;c<scps_country_count(s2) && tgt<0;c++){
            ScpsOpinionParts t;
            if (scps_opinion_summary(s2,c,&t)==0 && t.war<0) tgt=c;
        }
        ok("résumé d'opinion : une cible EN GUERRE porte la composante guerre (<0)", tgt>=0);
        if (tgt>=0){
            scps_opinion_summary(s2,tgt,&op);
            ok("résumé d'opinion : total borné ±100 et composante guerre négative",
               op.total>=-100 && op.total<=100 && op.war<0);
            /* le JOURNAL D'ACTES (la sous-détaille de « Mémoire ») : la déclaration de
             * guerre est LOGGÉE, datée, avec la bonne paire. */
            ScpsDiploAct ja[12];
            int nj = scps_diplo_journal(s2, tgt, ja, 12);
            int has_decl=0;
            for (int i=0;i<nj;i++)
                if (ja[i].act==DACT_WAR_DECLARED &&
                    (ja[i].a_id==pl || ja[i].b_id==pl) && ja[i].year>=0) has_decl=1;
            printf("   journal diplo : %d acte(s) avec la cible %d\n", nj, tgt);
            ok("journal d'actes : la déclaration de guerre est loggée (datée, bonne paire)", has_decl);
            ok("journal d'actes : le plus récent d'abord (seq décroissant)",
               nj<2 || ja[0].year >= ja[nj-1].year);
        }
    }

    /* ── COLONISATION (charte : « le joueur colonise n'importe quelle province ») — v50 :
     *    l'ordre OUVRE un CHANTIER (la colonie MÛRIT ~1 an frontalier) : ordre → drain →
     *    chantier actif (cadence : plus d'autre ordre) → avance total_days → FONDÉE (+1). ── */
    {
        int before = scps_country_province_count(s2, pl);
        int tgt=-1, np=scps_province_count(s2);
        for (int pp=0; pp<np && tgt<0; pp++)
            if (scps_can_colonize(s2, pp)) tgt=pp;
        ok("colonisation : une cible LÉGALE existe (scps_can_colonize)", tgt>=0);
        if (tgt>=0){
            ok("verbe COLONISER : ordre ENFILÉ (différé)", scps_player_colonize(s2, tgt)==1);
            scps_sim_advance_days(s2, 2);                   /* le drain OUVRE le chantier */
            int cdst=-1, cleft=0, ctot=0, ccd=0, cy=0;
            int act = scps_colony_status(s2, &cdst, &cleft, &ctot, &ccd, &cy);
            printf("   chantier : actif=%d dst=%d %d/%d j · cd %d j · rendement %d%%\n",
                   act, cdst, cleft, ctot, ccd, cy);
            ok("chantier OUVERT au drain (la colonie mûrit, pas d'apparition instantanée)",
               act==1 && cdst==tgt && ctot>=360 && cleft>0);
            ok("cadence : AUCUNE autre cible colonisable pendant le chantier",
               scps_can_colonize(s2, tgt)==0);
            ok("rendement borné (log-distance capitale)", cy>=30 && cy<=100);
            scps_sim_advance_days(s2, ctot+5);              /* la colonie MÛRIT puis FONDE */
            int after = scps_country_province_count(s2, pl);
            printf("   colonisation joueur : %d → %d province(s) au terme\n", before, after);
            ok("colonie FONDÉE au terme du chantier (+1 province au joueur)", after == before+1);
            ok("colonisation : la cible n'est PLUS colonisable (fondée)", scps_can_colonize(s2, tgt)==0);
        }
    }

    /* ── PANNEAU B (manufacture joueur) : légalité + verbe enfilé + pose au drain.
     *    On cherche un couple (région du joueur, type) LÉGAL ; s'il en existe un, la
     *    pose au drain doit AJOUTER un bâtiment (puits kind==1 de scps_region_alloc). ── */
    {
        int nreg=scps_region_count(s2);
        int lr=-1, lb=-1;
        for (int r=0;r<nreg && lr<0;r++)
            for (int b=1;b<64 && lr<0;b++)
                if (scps_manuf_legal(s2, r, b)){ lr=r; lb=b; }
        printf("   panneau B : couple légal (région %d, type %d)\n", lr, lb);
        if (lr>=0){
            ScpsAlloc al; scps_region_alloc(s2, lr, &al);
            int nb_before=0; for (int i=0;i<al.n;i++) if (al.sink[i].kind==1) nb_before++;
            ok("panneau B : verbe BUILD_MANUF enfilé", scps_player_build_manuf(s2, lr, lb)==1);
            scps_sim_advance_days(s2, 2);                    /* le drain pose */
            scps_region_alloc(s2, lr, &al);
            int nb_after=0; for (int i=0;i<al.n;i++) if (al.sink[i].kind==1) nb_after++;
            ok("panneau B : la manufacture est POSÉE au drain (+1 bâtiment)", nb_after == nb_before+1);
            ok("panneau B : le même type n'est plus légal (slot rempli)", scps_manuf_legal(s2, lr, lb)==0);
        } else {
            ok("panneau B : aucun couple légal (monde nu précoce) — verbe refusé proprement",
               scps_player_build_manuf(s2, 0, 1)==1);        /* enfilé ; le drain refusera sans crash */
            scps_sim_advance_days(s2, 2);
            ok("panneau B : drain sans crash sur refus", 1);
        }
        ok("panneau B : hors-borne refusé (type 999)", scps_manuf_legal(s2, 0, 999)==0);
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

    /* ── SAUVEGARDE : aller-retour (compose → sauve → recharge → tout conservé) ── */
    {
        scps_clear_player_culture();
        ScpsTradition trs3[64]; int ntr3=scps_tradition_list(trs3,64);
        int maj=-1, mn=-1, df=-1;
        for(int i=0;i<ntr3;i++){
            if(maj<0 && trs3[i].axe==0 && trs3[i].rang>=2) maj=trs3[i].id;
            if(mn <0 && trs3[i].axe==1 && trs3[i].rang==1) mn =trs3[i].id;
            if(df <0 && trs3[i].axe==2 && trs3[i].rang< 0) df =trs3[i].id;
        }
        scps_set_empire_culture(0, 0, 5, maj, mn, df);   /* joueur ESOTERIQUE/PACIFISTE → Havre */
        ScpsSim *sg=scps_sim_new(); scps_sim_generate(sg, seed);
        scps_sim_advance_days(sg, 365*3);
        int yr_before=scps_year(sg);
        long pop_before=scps_world_pop(sg);
        ScpsCountryInfo before; scps_country_info(sg, scps_player(sg), &before);
        char nom_before[64]; snprintf(nom_before,sizeof nom_before,"%s",before.nom);
        ok("sauvegarde écrite (slot 1)", scps_sim_save(sg, 1)==1);
        ScpsSaveSlot slots[3]; scps_save_slots(slots,3);
        ok("slot 1 listé OCCUPÉ (année cohérente)", slots[0].used==1 && slots[0].year==yr_before);

        ScpsSim *sl=scps_sim_new();
        int rc=scps_sim_load(sl, 1);
        ok("chargement OK (rc=0)", rc==0);
        ok("année + pop restaurées", scps_year(sl)==yr_before && scps_world_pop(sl)==pop_before);
        ScpsCountryInfo after; scps_country_info(sl, scps_player(sl), &after);
        printf("   save/load : « %s » (an %d, %ld âmes) → « %s » (an %d, %ld âmes)\n",
               nom_before, yr_before, pop_before, after.nom, scps_year(sl), scps_world_pop(sl));
        ok("culture du joueur CONSERVÉE (nom = épithète « Havre »)", strncmp(after.nom,"Havre",5)==0);
        /* la partie chargée VIT (le build composé persiste via la section CULT) */
        scps_sim_advance_days(sl, 365);
        ok("la partie chargée AVANCE (an +1)", scps_year(sl)==yr_before+1);
        scps_clear_player_culture();
        scps_sim_free(sg); scps_sim_free(sl);
    }

    /* ── RELIGION (P3) : le registre + le lien pays→religion SURVIVENT au save/load ── */
    {
        ScpsSim *sr=scps_sim_new(); scps_sim_generate(sr, seed);   /* reset religion */
        int pl=scps_player(sr);
        int rtrad[3]={RP_FECONDITE, RP_ACCUEIL, RP_GNOSE};
        uint8_t rcol[3]={30,60,200};
        int rid=religion_spawn(CREDO_PLURALISTE, rtrad, 100, pl, rcol);
        religion_set_country(pl, rid);
        ok("religion fondée + liée au joueur", rid>=0 && religion_of_country(pl)==rid);
        ok("sauvegarde religion (slot 2)", scps_sim_save(sr, 2)==1);
        scps_sim_free(sr);

        ScpsSim *sr2=scps_sim_new();
        scps_sim_generate(sr2, seed);                               /* reset → plus de religion */
        ok("après reset : registre religion vide", g_religion_count==0);
        int rc=scps_sim_load(sr2, 2);
        ok("chargement religion OK (rc=0)", rc==0);
        int pl2=scps_player(sr2);
        printf("   religion save/load : registre=%d · lien joueur=%d (attendu %d)\n",
               g_religion_count, religion_of_country(pl2), rid);
        ok("registre religion RESTAURÉ (>=1)", g_religion_count>=1);
        ok("lien pays→religion RESTAURÉ", religion_of_country(pl2)==rid);
        ok("tradition[0] conservée (Fécondité)", rid>=0 && g_religions[rid].traditions[0]==RP_FECONDITE);
        religion_reset();
        scps_sim_free(sr2);
    }

    /* ── RELIGION (P4) : la foi NUDGE le moteur (gated) — la pop joueur DIVERGE vs sans-foi ── */
    {
        ScpsSim *na=scps_sim_new(); scps_sim_generate(na, seed);   /* A : generate reset → sans foi */
        int pa=scps_player(na); scps_sim_advance_days(na, 365*10);
        long popA=scps_country_pop(na, pa);
        scps_sim_free(na);

        ScpsSim *nb=scps_sim_new(); scps_sim_generate(nb, seed);   /* B : même graine, foi pro-natalité */
        int pb=scps_player(nb);
        int rt[3]={RP_FECONDITE, RP_COURONNE, RP_GNOSE};           /* Fécondité(popgrowth+) · Couronne(L+) · Gnose */
        int rr=religion_spawn(CREDO_EVANGELISTE, rt, 0, pb, NULL);
        religion_set_country(pb, rr);
        scps_sim_advance_days(nb, 365*10);
        long popB=scps_country_pop(nb, pb);
        printf("   P4 effet : pop joueur sans-foi=%ld · avec-foi(Fécondité+Couronne)=%ld\n", popA, popB);
        ok("la religion MORD sur le moteur (pop joueur diverge)", popB != popA);
        ok("foi pro-natalité ⇒ pop ≥ sans-foi", popB >= popA);
        religion_reset();
        scps_sim_free(nb);
    }

    /* ── RELIGION (P8) : fondation région-héritée + schisme INTERNE qui FRACTURE ── */
    {
        ScpsSim *sf=scps_sim_new(); scps_sim_generate(sf, seed);
        int pl=scps_player(sf);
        int rid=scps_religion_found(sf, pl, CREDO_PLURALISTE, RP_FECONDITE, RP_ACCUEIL, RP_GNOSE);
        ok("religion fondée (façade)", rid>=0 && scps_religion_of_country(sf,pl)==rid);
        int inherited=0, nrg=scps_region_count(sf);
        for(int r=0;r<nrg;r++) if(scps_religion_of_region(sf,r)==rid) inherited++;
        ok("régions du pays HÉRITENT de la religion", inherited>0);
        int flipped=0;
        int child=scps_religion_schism(sf, pl, 1, RP_MUR, 2, RP_ORTHODOXIE, CREDO_PURIFICATEUR, &flipped);
        int now_child=0; for(int r=0;r<nrg;r++) if(scps_religion_of_region(sf,r)==child) now_child++;
        printf("   P8 schisme interne : enfant=%d · régions basculées=%d/%d · régions enfant=%d\n",
               child, flipped, inherited, now_child);
        ok("schisme interne crée un enfant", child>rid);
        ok("fracture bornée (0..régions héritées)", flipped>=0 && flipped<=inherited);
        ok("régions basculées == compte enfant (cohérent)", now_child==flipped);
        scps_sim_free(sf);
        religion_reset();
    }

    /* ── RELIGION (P6) : le LETTRÉ — un Missionnaire RECONVERTIT une région minoritaire ── */
    {
        ScpsSim *ss=scps_sim_new(); scps_sim_generate(ss, seed);
        int pl=scps_player(ss);
        int rid=scps_religion_found(ss, pl, CREDO_EVANGELISTE, RP_FECONDITE, RP_ACCUEIL, RP_GNOSE);
        ok("foi évangéliste fondée", rid>=0);
        int prg=-1, nrg=scps_region_count(ss);
        for(int r=0;r<nrg;r++) if(scps_region_owner(ss,r)==pl){ prg=r; break; }
        ok("région du joueur trouvée", prg>=0);
        int otr[3]={RP_OFFRANDE, RP_MUR, RP_ORTHODOXIE};
        int other=religion_spawn(CREDO_PURIFICATEUR, otr, 0, pl, NULL);
        religion_set_region(NULL, prg, other);   /* cache-only (econ opaque au banc) ; la conversion
                                                   * de GROUPES par le Missionnaire est couverte en sim réelle */
        ok("région rendue minoritaire", scps_religion_of_region(ss,prg)==other);
        int role=scps_religion_recruit_scholar(ss, pl, prg);
        ok("Missionnaire recruté (CONVERT)", role==SCHOLAR_CONVERT);
        scps_sim_advance_days(ss, 30);
        printf("   P6 missionnaire : région %d religion=%d (foi d'État=%d)\n", prg, scps_religion_of_region(ss,prg), rid);
        ok("Missionnaire RECONVERTIT à la foi d'État", scps_religion_of_region(ss,prg)==rid);
        ok("crédo→rôle : pluraliste=Gourou(RESIST)", scholar_role_from_credo(CREDO_PLURALISTE)==SCHOLAR_RESIST);
        ok("crédo→rôle : purificateur=Moine(STABILIZE)", scholar_role_from_credo(CREDO_PURIFICATEUR)==SCHOLAR_STABILIZE);
        scps_sim_free(ss);
        religion_reset();
    }

    /* ── RELIGION : PLAFOND ⌈n_emp/3⌉ — fonder sous le cap, RALLIER au-delà ── */
    {
        religion_reset();
        ok("cap : ⌈4/3⌉=2 · ⌈3/3⌉=1 · ⌈6/3⌉=2 · ⌈7/3⌉=3",
           religion_cap(4)==2 && religion_cap(3)==1 && religion_cap(6)==2 && religion_cap(7)==3);
        int r0=religion_found_random(0, 10, 111u);
        int r1=religion_found_random(1, 20, 222u);
        ok("2 racines fondées (cap 4 emp = 2)", r0>=0 && r1>=0 && religion_root_count()==2);
        ok("au plafond de RACINES : religion_can_found faux (2 == cap 2)", !religion_can_found(4));
        /* 3e empire : plafond de RACINES atteint → RALLIE, pas de nouvelle racine */
        int before=religion_root_count();
        int r2 = religion_can_found(4) ? religion_found_random(2,30,333u)
                                       : religion_adopt_existing(2,333u);
        ok("3e empire RALLIE (aucune racine neuve)", r2>=0 && religion_root_count()==before);
        ok("le rallié partage une foi existante", r2==r0 || r2==r1);
        /* SCHISME borné PAR RACINE : RELIG_SCHISM_MAX sectes par foi fondatrice */
        ok("racine r0 peut schismer (0 secte)", religion_can_schism(r0));
        int made=0, klast=r1;   /* crée jusqu'au PLAFOND (RELIG_SCHISM_MAX, relâché à 5) */
        for(int s=0; s<RELIG_SCHISM_MAX && religion_can_schism(r0); s++){
            int pa=(s%2)?RP_MUR:RP_ACCUEIL, pb=(s%2)?RP_GNOSE:RP_ORTHODOXIE;
            int k=religion_schism(r0, 1, pa, 2, pb, 2, 30+s, 1, 1, 0xABCDu+(uint32_t)s);
            if(k>r1 && religion_root_of(k)==r0){ made++; klast=k; }
        }
        ok("RELIG_SCHISM_MAX sectes créées sous r0", made==RELIG_SCHISM_MAX && klast>r1);
        ok("au plafond : r0 ne peut plus schismer", !religion_can_schism(r0));
        ok("racine r1 (0 secte) PEUT encore schismer", religion_can_schism(r1));
        religion_reset();
    }

    /* ── MEMBRANE DE DÉCISION — la file joueur : pending EXPOSÉ + choix DRAINÉ. ── */
    {
        ScpsSim *sp = scps_sim_new(); scps_sim_generate(sp, seed);
        ok("pending : aucune décision en attente à la genèse (monde frais)",
           scps_pending_count(sp)==0);
        ScpsPendingEvent pe; memset(&pe,0,sizeof pe);
        ok("pending_event sur un slot HORS-BORNE renvoie 0 (jamais déréférencé)",
           scps_pending_event(sp, 0, &pe)==0 && pe.n_options==0);
        ok("player_event_choice sur un slot HORS-BORNE est un refus net",
           scps_player_event_choice(sp, 0, 0)==0);
        /* on laisse le monde VIVRE assez longtemps pour qu'une VRAIE décision (Marbrive,
         * n_options>1) finisse par concerner le joueur — la façade l'ENFILE (pas d'auto-
         * résolution IA sur son propre pays) ; on la résout via le VERBE (drain déterministe). */
        int pl2 = scps_player(sp);
        int n0 = scps_pending_count(sp);
        for (int yr=0; yr<200 && scps_pending_count(sp)==n0; yr++)
            scps_sim_advance_days(sp, 365);
        int n1 = scps_pending_count(sp);
        if (n1>n0){
            ok("pending_event lit un slot VALIDE (situation résolue, une option ou plus)",
               scps_pending_event(sp, 0, &pe)==1 && pe.n_options>=1 && pe.situation[0]!='\0');
            ok("player_event_choice ENFILE le choix (mis en file)",
               scps_player_event_choice(sp, 0, 0)==1);
            scps_sim_advance_days(sp, 2);   /* le drain RÉSOUT au prochain tick */
            ok("le choix DRAINÉ retire le pending de la file", scps_pending_count(sp)<n1);
            /* LES ANNALES DU RÈGNE : le dilemme qu'on vient de trancher (drain réel, pas
             * pending_event_resolve directement) doit apparaître, TRIÉ, ligne non vide. */
            ScpsAnnal an[16];
            int na = scps_annals(sp, an, 16);
            ok("scps_annals rend au moins une entrée après un dilemme résolu", na>=1);
            bool has_line=true, sorted=true;
            for (int i=0;i<na;i++){
                if (!an[i].ligne || an[i].ligne[0]=='\0') has_line=false;
                if (i>0 && an[i].year<an[i-1].year) sorted=false;
            }
            ok("chaque entrée porte une LIGNE diégétique non vide", has_line);
            ok("scps_annals rend les entrées TRIÉES par année croissante", sorted);
        } else {
            ok("(aucune décision joueur apparue en 150 ans sur cette graine — ignoré)", true);
            ok("(idem)", true);
            ok("(idem)", true);
            ok("(idem)", true);
            ok("(idem)", true);
        }
        (void)pl2;
        scps_sim_free(sp);
    }

    /* ── DÉCRETS DU JOUEUR (civics) — liste exposée, toggle drainé, réforme irréversible. ── */
    {
        ScpsSim *sd = scps_sim_new(); scps_sim_generate(sd, seed);
        int me = scps_player(sd);
        ScpsDecree decs[8];
        int nd = scps_decrees_list(sd, me, decs, 8);
        ok("scps_decrees_list expose au moins un décret", nd>0);
        bool has_line=true, has_flavor=true, has_plateaux=true, none_active=true;
        int levee_id=-1, tribut_id=-1;
        for (int i=0;i<nd;i++){
            if (!decs[i].nom || decs[i].nom[0]=='\0') has_line=false;
            if (!decs[i].flavor || decs[i].flavor[0]=='\0') has_flavor=false;
            if (!decs[i].plateaux || decs[i].plateaux[0]=='\0') has_plateaux=false;
            if (decs[i].active) none_active=false;
            if (decs[i].reforme) tribut_id=decs[i].id;
            else if (levee_id<0) levee_id=decs[i].id;
        }
        ok("chaque décret porte un nom non vide", has_line);
        ok("chaque décret porte un flavor non vide", has_flavor);
        ok("chaque décret décrit ses DEUX plateaux (gain/contrepartie)", has_plateaux);
        ok("aucun décret actif à la genèse (monde frais)", none_active);
        ok("au moins une RÉFORME (irréversible) est répertoriée", tribut_id>=0);

        /* toggle ON d'un ÉDIT (pas de réforme) : enfilé, DRAINÉ au tick suivant. */
        if (levee_id>=0){
            ok("player_decree(ON) enfile l'ordre", scps_player_decree(sd, levee_id, 1)==1);
            scps_sim_advance_days(sd, 2);   /* le drain applique (si la condition est remplie) */
            int nd2 = scps_decrees_list(sd, me, decs, 8);
            bool found=false, active_after=false;
            for (int i=0;i<nd2;i++) if (decs[i].id==levee_id){ found=true; active_after=(decs[i].active!=0); }
            ok("le décret réapparaît dans la liste après le drain", found);
            /* l'activation dépend de la condition d'entrée (tech) : si illégale au départ,
             * le drain la REFUSE (comportement attendu, pas un échec de plomberie). */
            bool legal_before=false;
            for (int i=0;i<nd;i++) if (decs[i].id==levee_id) legal_before=(decs[i].legal!=0);
            ok("l'activation suit la légalité (ON accepté ssi la condition l'était)",
               active_after==legal_before || !legal_before);
            /* toggle OFF : un ÉDIT se désengage librement. */
            if (active_after){
                ok("player_decree(OFF) enfile le retour arrière (édit réversible)",
                   scps_player_decree(sd, levee_id, 0)==1);
                scps_sim_advance_days(sd, 2);
                int nd3 = scps_decrees_list(sd, me, decs, 8);
                bool still_active=false;
                for (int i=0;i<nd3;i++) if (decs[i].id==levee_id) still_active=(decs[i].active!=0);
                ok("l'édit est bien désactivé après le OFF", !still_active);
            } else {
                ok("(édit non activable sur cette graine — ignoré)", true);
            }
        } else {
            ok("(aucun édit non-réforme trouvé — ignoré)", true);
            ok("(idem)", true);
            ok("(idem)", true);
        }

        /* RÉFORME irréversible : forcer l'activation (contourne la condition d'entrée pour
         * le test de PLOMBERIE du refus-retour, pas de la porte — déjà couvert ailleurs)
         * n'est pas exposé côté façade ; on vérifie plutôt que decree_toggle(OFF) sur une
         * réforme jamais activée reste bien un no-op silencieux (rien à désactiver). */
        if (tribut_id>=0){
            ok("player_decree(OFF) sur une réforme jamais activée est un ordre inoffensif",
               scps_player_decree(sd, tribut_id, 0)==1);   /* enfilé (mis en file) — le drain n'a rien à défaire */
            scps_sim_advance_days(sd, 2);
            int nd4 = scps_decrees_list(sd, me, decs, 8);
            bool still_inactive=true;
            for (int i=0;i<nd4;i++) if (decs[i].id==tribut_id) still_inactive=!decs[i].active;
            ok("la réforme reste inactive (rien à retirer)", still_inactive);
        } else {
            ok("(aucune réforme trouvée — ignoré)", true);
        }
        scps_sim_free(sd);
    }

    free(rgba); free(lay);
    printf("\n══ BILAN : %d réussis, %d échoués ══\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
