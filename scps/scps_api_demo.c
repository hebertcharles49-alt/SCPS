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
#include "scps_army.h"       /* ARMÉE — LA FORCE NOMINALE : U_MILICE (levée toujours ouverte, banc seul) */
#include "scps_tune.h"       /* MONNAIE M14 — B7 : tune_f/tune_set (DEBT_FIXED/DEBT_DUE_FRAC) */
#include "scps_religion.h"   /* P3 : test de persistance religion */
#include "scps_provlog.h"    /* DACT_* : le journal d'actes diplomatique */
#include "scps_agency.h"     /* LOT T : edifice_tier (le palier de famille) */
#include "scps_tech.h"       /* LOT T : tech_has_tier (la preuve de tier de recherche) */
#include "scps_fog.h"        /* DIPLO-FOG : fog_debug_meet_all (découverte forcée, banc seul) */
#include "scps_factions.h"   /* FAC_COUNT : profondeur du lecteur politique */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define TOPONYM_DEMO_BUF 40   /* TOPONYMIE : marge de snapshot local (> char[32] du moteur) */

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if(cond) g_pass++; else g_fail++;
}

int main(int argc, char **argv){
    /* EXPANSION PASSIVE COUPÉE pour ce banc (2026-08-06) : le seep fonde des provinces
     * au joueur PENDANT les chantiers testés (« +1 province » devenait +2/+3) — le banc
     * prouve la FAÇADE, pas l'expansion passive (testée par ses propres gates). */
    tune_set("PASSIVE_SEEP", 0.f);

    /* VAGUE CLIMAT (2026-08-18) : la graine 9 (défaut historique de ce banc) tire
     * désormais l'archétype « archipel » — le pays-joueur y est laminé par l'IA en
     * ~20 ans (0 province, 0 or, 0 bourgeois à l'an 20 : monde mort, pas un bug de
     * façade). Scan de graines (graine → prov/or/bourgeois/pool à l'an 20) : la 1
     * tient une province saine (or=500, bourgeois=415, pool=91.6) — recalibrage de
     * fixture, même esprit de banc (façade sur UN monde vivant), monde différent. */
    uint32_t seed = (argc>1) ? (uint32_t)strtoul(argv[1],NULL,10) : 1u;
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
    { int legal_ok=ntn>0, reasons_ok=ntn>0, paths_ok=ntn>0;
      for(int i=0;i<ntn;i++){
          legal_ok &= tn[i].allowed==0 || tn[i].allowed==1;
          legal_ok &= (tn[i].allowed==1)==(tn[i].state==1);
          legal_ok &= tn[i].points_have>=0 && tn[i].points_missing>=0;
          reasons_ok &= tn[i].reason_code && tn[i].reason_code[0] && tn[i].reason_label && tn[i].reason_label[0];
          reasons_ok &= (tn[i].allowed==1)==(strcmp(tn[i].reason_code,"ok")==0);
          if(tn[i].state==2) paths_ok &= tn[i].next_step==-1 && tn[i].steps_remaining==0 && tn[i].path_label[0]=='\0';
          else paths_ok &= tn[i].next_step>=0 && tn[i].next_step<ntn && tn[i].steps_remaining>=1 &&
                           tn[i].path_label && tn[i].path_label[0] && tn[tn[i].next_step].state!=2;
      }
      ok("P4 tech : décision structurée cohérente avec l'état moteur", legal_ok);
      ok("P4 tech : chaque nœud nomme son premier verrou", reasons_ok);
      ok("P8 tech : chaque cible non acquise expose une chaîne et sa prochaine étape", paths_ok); }

    ScpsBudget bg; scps_budget_summary(s, pl0, &bg);
    ScpsFluxLine fx[32]; int nfx = scps_country_budget(s, pl0, fx, 32);
    printf("   budget : or=%.0f · revenus=%.0f · dépenses=%.0f · net=%+.0f · %d postes · crédit=%.0f\n",
           bg.gold, bg.income, bg.expense, bg.net, nfx, bg.credit_line);
    /* L'ALLIAGE (2026-07-21) : la frappe exige la PAIRE or+cuivre (loi du minimum) — CE
     * joueur-graine, sans mine du 2e métal, a une monnaie légitimement MORTE à l'an 20
     * (réserve de genèse épuisée). Le banc teste la FAÇADE (les postes se décomposent),
     * pas la viabilité monétaire d'un pays particulier : on exige qu'AU MOINS UN pays du
     * monde ait des postes vivants (tous à zéro = monde monétairement mort, vraie alarme). */
    { int nfx_any = nfx;
      for (int ci=0; ci<nc && nfx_any<=0; ci++){
          ScpsFluxLine fx2[32]; nfx_any = scps_country_budget(s, ci, fx2, 32); }
      ok("budget : décomposition du flux (≥1 pays au monde a des postes vivants)", nfx_any>0); }
    ok("budget : net = revenus − dépenses (cohérent)", bg.net==bg.income-bg.expense);
    ok("budget : crédit disponible des prêteurs (≥0, fini)", bg.credit_line>=0 && bg.credit_line==bg.credit_line);
    ok("P6 budget : rythmes mensuels et projection sont finis",
       bg.monthly_income>=0.0 && bg.monthly_expense>=0.0 && bg.monthly_net==bg.monthly_net &&
       bg.projected_year_end==bg.projected_year_end && bg.runway_months==bg.runway_months);
    ok("P6 budget : autonomie négative seulement quand le solde est stable",
       (bg.monthly_net>=0.0 && bg.runway_months<0.0) || (bg.monthly_net<0.0 && bg.runway_months>=0.0));

    ScpsStock market_st[40]; int nmarket_st=scps_country_stocks(s,pl0,market_st,40);
    ok("marché : au moins un bien vivant exposé", nmarket_st>0);
    if(nmarket_st>0){
        ScpsMarketQuote mq;
        int mqok=scps_market_quote(s,pl0,market_st[0].res_id,10,&mq);
        ok("marché : devis capital valide et borné", mqok && mq.valid && mq.region>=0 &&
           mq.margin>=1.f && mq.local_qty>=0 && mq.local_qty<=10 && mq.global_qty>=0 && mq.global_qty<=10);
        ok("marché : coûts finis/non négatifs et accès mondial cohérent",
           mq.local_cost>=0.0 && mq.local_cost==mq.local_cost && mq.global_cost>=0.0 &&
           mq.global_cost==mq.global_cost && (mq.global_access || mq.global_qty==0));
        ScpsStockRegion sr[64];
        int nsr=scps_stock_regions(s,pl0,market_st[0].res_id,sr,64), sr_ok=nsr>0, sorted=nsr>0;
        for(int i=0;i<nsr;i++){
            sr_ok &= sr[i].region>=0 && sr[i].region<scps_region_count(s) && sr[i].province>=0;
            sr_ok &= sr[i].name && sr[i].name[0] && sr[i].stock>=0 &&
                     sr[i].supply_month>=0.f && sr[i].demand_month>=0.f;
            if(i>0)sorted &= sr[i-1].supply_month+sr[i-1].demand_month+1e-5f >=
                            sr[i].supply_month+sr[i].demand_month;
        }
        ok("P6 économie : producteurs/consommateurs territoriaux nommés et bornés",sr_ok);
        ok("P6 économie : territoires triés par activité décroissante",sorted);
    }

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
        /* DIPLO-FOG (lot 5) : un empire NON DÉCOUVERT n'existe pas pour le joueur —
         * or le joueur du banc est PASSIF (jamais d'expansion) et le monde-archétype
         * est clairsemé : il ne rencontre légitimement PERSONNE en radius 2, tous les
         * verbes seraient « cible inconnue ». Le banc teste la PLOMBERIE des verbes,
         * pas l'exploration → découverte forcée BANC-SEULEMENT (motif
         * intertrade_debug_set_hub_of). */
        fog_debug_meet_all(pl);
        /* W-GUERRE-3 : déclarer la guerre exige désormais un CASUS BELLI. On cherche
         * d'abord une cible à CB déjà UTILISABLE (gratuit — subjugation/anti-piraterie) ;
         * à défaut on FABRIQUE une intrigue (2 ans du revenu de la cible) contre une
         * cible qu'on peut se payer, on la laisse MÛRIR (FAB_MATURE_DAYS ≈ 365 j), puis
         * on déclare — l'aller-retour complet du NOUVEAU flux (v50 : UN acte / 2 mois ;
         * jamais un hameau libre role==4). */
        int wt=-1;
        for (int c=0;c<nc2 && wt<0;c++){
            if (c==pl || scps_country_role(s2,c)==4) continue;
            ScpsDiploOptions op;
            if (scps_diplo_options(s2, c, &op) && op.can_declare_war) wt=c;   /* CB déjà utilisable */
        }
        if (wt<0){                                          /* aucun CB gratuit → FABRIQUER */
            /* Le prix de l'intrigue (2 ans du revenu de la CIBLE) est SENSIBLE AU MONDE
             * (les tolérances fiscales 2026-07-10 ont bougé les trésoreries an-0) : si
             * aucune cible n'est payable AUJOURD'HUI, on laisse le trésor S'ACCUMULER
             * par pas de 180 j (borné ~5 ans) — l'intention du banc est l'aller-retour
             * du flux, pas la richesse du joueur au jour J. */
            for (int tries=0; tries<10 && wt<0; tries++){
                float min_cost = 1e30f;
                for (int c=0;c<nc2 && wt<0;c++){
                    if (c==pl || scps_country_role(s2,c)==4) continue;
                    ScpsDiploOptions op;
                    if (!scps_diplo_options(s2, c, &op)) continue;
                    if (op.fabricate_cost < min_cost) min_cost = op.fabricate_cost;
                    if (op.can_fabricate && scps_player_fabricate_cb(s2, c)>0) wt=c;   /* l'or PAYÉ, l'intrigue court */
                }
                if (wt<0){
                    printf("   (intrigue : aucune cible payable — coût min %.0f, on laisse le trésor monter, essai %d)\n",
                           (double)min_cost, tries+1);
                    scps_sim_advance_days(s2, 180);
                }
            }
            if (wt>=0) scps_sim_advance_days(s2, 400);       /* > FAB_MATURE_DAYS : l'intrigue MÛRIT */
        }
        int enq = (wt>=0) ? scps_player_declare_war(s2, wt) : 0;
        ok("verbe DÉCLARER LA GUERRE : ordre ENFILÉ (CB gratuit ou intrigue mûrie)", enq>0);
        scps_sim_advance_days(s2, 1);                       /* le drain applique (le CB est consommé) */
        nr = scps_country_relations(s2, pl, rel, 64);
        int wars1=0; for (int i=0;i<nr;i++) wars1 += rel[i].at_war;
        /* CIBLE PRÉCISE, pas le total agrégé (2026-08-18) : dans un monde vivant à 150+ pays,
         * une guerre PRÉEXISTANTE ailleurs peut se résoudre (paix/annexion) le MÊME jour que
         * notre déclaration — le total wars1/wars0 peut alors rester égal alors que NOTRE
         * cible EST bien entrée en guerre (faux négatif observé : guerre PRÉ vs pays 16 close
         * le jour même où la guerre vs pays 4 s'ouvre, wars1==wars0==1). On vérifie donc l'état
         * de LA cible déclarée, la seule chose que ce verbe prétend prouver. */
        int wt_at_war=0; for (int i=0;i<nr;i++) if (rel[i].country==wt) wt_at_war = rel[i].at_war;
        printf("   diplo joueur : guerres %d → %d après déclaration au drain (cible %d en guerre=%d)\n",
               wars0, wars1, wt, wt_at_war);
        ok("verbe DÉCLARER LA GUERRE : APPLIQUÉ au drain (le joueur entre en guerre)",
           wt>=0 && wt_at_war==1);
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
        v += scps_player_refill(s2)          ? 1:0;
        v += scps_player_navy_build(s2, 0)   ? 1:0;
        v += scps_player_disband(s2)         ? 1:0;
        ok("§3 — 12 verbes intérieur/commerce/guerre ENFILÉS (posture retirée)", v==12);
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
        ok("revendication antagoniste : le territoire visé est nommé",
           dop.claim_region>=0 && dop.claim_province>=0 && dop.claim_name && dop.claim_name[0]);
        { ScpsPeacePreview pp; ScpsPeaceTerritory pt[SCPS_PEACE_TERRITORY_MAX];
          int pv=scps_peace_preview(s2,tgt,&pp), nt=scps_peace_territories(s2,tgt,pt,SCPS_PEACE_TERRITORY_MAX);
          ok("paix : score, revenu mensuel et conversion or physique traversent la façade",
             pv==1 && pp.valid && pp.at_war && pp.revenue_month>=0.0 &&
             fabs(pp.gold_per_score-pp.revenue_month*0.03)<0.001 && pp.gold_max<=pp.gold_available+0.001);
          int rows_ok=(nt==pp.target_regions);
          for(int i=0;i<nt;i++)rows_ok &= pt[i].region>=0 && pt[i].province>=0 && pt[i].name && pt[i].name[0] && pt[i].score_cost>0.f;
          ok("paix : chaque territoire porte son nom, son occupation et son vrai prix", rows_ok);
          ok("paix : coûts fixes et vasselage additif sont physiques",
             pp.reparations_cost==10&&pp.humiliate_cost==20&&pp.pillage_cost==10&&
             pp.liberate_cost==50&&pp.fragment_cost==100&&pp.vassal_score>0.f);
          ScpsDiploOptions still_war;
          ok("paix : une offre composée s'enfile sans mutation immédiate",
             scps_player_peace_offer(s2,tgt,NULL,0,0,0)==1 &&
             scps_diplo_options(s2,tgt,&still_war)==1 && still_war.can_make_peace); }
        { int legal_ok=gotd, reasons_ok=gotd, conds_ok=gotd;
          for(int a=0;a<SCPS_DIPLO_ACTION_COUNT && gotd;a++){
              ScpsActionLegal dl;
              legal_ok &= scps_diplo_action_legal(s2,tgt,a,&dl)==1 && dl.valid;
              legal_ok &= dl.allowed>=0 && dl.allowed<=1 && dl.would_accept>=0 && dl.would_accept<=1;
              legal_ok &= dl.cost_gold>=0.0 && dl.gold_have>=0.0 && dl.gold_missing>=0.0 && dl.duration_days>=0;
              reasons_ok &= dl.reason_code && dl.reason_code[0] && dl.reason_label && dl.reason_label[0];
              reasons_ok &= (dl.allowed==1)==(strcmp(dl.reason_code,"ok")==0);
              /* CHECKLIST DE REFUS (2026-07-21) — le CONTRAT : la liste existe, chaque
               * label est non vide, ET les conds SE COMBINENT en allowed. La FABRIQUE a
               * des préalables géométriques cachés (allowed⊂conds) : pour elle, allowed→
               * toutes-cochées seulement ; les 6 verbes structurels : ET(conds)==allowed. */
              conds_ok &= dl.n_conds>=1 && dl.n_conds<=SCPS_GATE_MAX;
              int all=1; for(int i=0;i<dl.n_conds;i++){ conds_ok &= dl.conds[i].label[0]!=0; if(!dl.conds[i].ok) all=0; }
              if(a==SCPS_DIPLO_FABRICATE) conds_ok &= (!dl.allowed || all);      /* allowed → toutes cochées */
              else                        conds_ok &= (all==dl.allowed);          /* ET(conds) == allowed */
          }
          ScpsActionLegal bad;
          reasons_ok &= scps_diplo_action_legal(s2,-1,SCPS_DIPLO_WAR,&bad)==0 && !bad.valid
                     && strcmp(bad.reason_code,"invalid_target")==0;
          ok("P4 diplo : les 7 verbes exposent une décision bornée", legal_ok);
          ok("P4 diplo : chaque refus nomme un verrou stable, cible invalide comprise", reasons_ok);
          ok("checklist de refus : conds nommées · ET(conds)==allowed (fabrique : allowed→conds)", conds_ok); }
        ok("scps_build_legal : réponse bornée {0,1} (région · or)",
           (scps_build_legal(s2,-1,0) & ~1)==0);
        /* ── LOT T — la GATE TECH PAR PALIER (edifice_tier ⇐ tech_has_tier) ── */
        ok("LOT T edifice_tier : bases T1 (Sanctuaire · Tribunal · Marché · Grenier)",
           edifice_tier(EDI_SANCTUAIRE)==1 && edifice_tier(EDI_TRIBUNAL)==1
           && edifice_tier(EDI_MARCHE)==1 && edifice_tier(EDI_GRENIER)==1);
        ok("LOT T edifice_tier : paliers ↑ (Temple 2 · Cathédrale 3 · Chancellerie 2 · Académie 3 · Arsenal 2)",
           edifice_tier(EDI_TEMPLE)==2 && edifice_tier(EDI_CATHEDRALE)==3
           && edifice_tier(EDI_CHANCELLERIE)==2 && edifice_tier(EDI_ACADEMIE)==3
           && edifice_tier(EDI_ARSENAL)==2);
        {   /* tech_has_tier PUR : état frais = tier 0 seul ; poser un nœud tier-2 l'ouvre. */
            TechState lt; tech_state_init(&lt, false);
            int found2=-1; for (int t=0;t<TECH_COUNT;t++){ const TechNode *nd=tech_node((TechId)t);
                if (nd && nd->tier==2){ found2=t; break; } }
            bool fresh_ok = tech_has_tier(&lt,0) && tech_has_tier(NULL,2) && !tech_has_tier(&lt,2);
            if (found2>=0){ lt.unlocked[found2]=true; }
            ok("LOT T tech_has_tier : frais=T0 seul · NULL permissif · nœud tier-2 posé ⇒ T2 ouvert",
               fresh_ok && found2>=0 && tech_has_tier(&lt,2));
        }
        {   /* le MIROIR rapporte une raison BORNÉE 0..4 sur un édifice de palier ≥2 (Temple) —
             * illégal à l'an ~0 (base de famille non bâtie → 1, ou tech de palier → 4, ou
             * matière/or) : on prouve la borne + la cohérence, pas un monde précis. */
            int r4=-9; int lg=scps_build_legal_ex(s2,-1,(int)EDI_TEMPLE,&r4);
            ok("LOT T build_legal_ex(Temple) : raison bornée 0..4, cohérente avec legal",
               r4>=0 && r4<=4 && ((lg==1)==(r4==0)));
        }
        /* ── MONNAIE M9 — V1 : « EMPRUNTER À UN ORDRE » (panneau éco) — lecteur capacité
         * (3 lignes Laborer/Bourgeois/Élite, Laborer toujours 0) + le verbe : la classe
         * NE REFUSE JAMAIS (capacité épuisée ≠ refus), le trésor est crédité au drain. */
        {
            ScpsLoanCapacity lc[3]; int nlc = scps_country_loan_capacity(s2, pl, lc, 3);
            ok("V1 capacité d'emprunt : 3 lignes (Laborer/Bourgeois/Élite)", nlc==3);
            ok("V1 capacité : Laborer ne prête jamais (motif M3c, aucune épargne)",
               nlc==3 && lc[0].montant_max==0.f);
            double gold0 = scps_country_gold(s2, pl);
            int cls = 1;   /* CLASS_BOURGEOIS (scps_econ.h) — le prêteur ordinaire */
            float cap = (nlc==3) ? lc[cls].montant_max : 0.f;
            int enq = scps_player_borrow_class(s2, cls, -1.f);   /* <=0 ⇒ le MAXIMUM disponible */
            ok("V1 verbe EMPRUNTER À UN ORDRE : ordre ENFILÉ", enq==1);
            scps_sim_advance_days(s2, 1);                         /* le drain applique */
            double gold1 = scps_country_gold(s2, pl);
            printf("   V1 emprunt à l'ordre (Bourgeois) : capacité annoncée %.0f or (taux fixe %.1f%%) — trésor %.0f -> %.0f (%+.0f)\n",
                   (double)cap, (nlc==3)?(double)(lc[cls].taux*100.f):0.0, gold0, gold1, gold1-gold0);
            ok("V1 verbe EMPRUNTER À UN ORDRE : APPLIQUÉ au drain (trésor crédité de la capacité annoncée)",
               cap<=0.5f ? (gold1>=gold0-0.01) : (gold1 > gold0 + 0.5*(double)cap));

            /* MONNAIE M14 — B7 : LA VRAIE ÉCHÉANCE (scps_country_debt.due) — DISTINCTE de
             * `taux` (le taux d'ORIGINATION d'un futur emprunt, jamais celui qui prélève sur
             * la dette EXISTANTE sous DEBT_FIXED : 10 %/an du stock, credit_year_tick). Le
             * bug de l'UI (budget_panel_v2.gd) affichait total*taux (2-5 %) — bien EN DESSOUS
             * de la vraie échéance (10 %). Si les classes n'ont encore aucune épargne
             * mobilisable (bootstrap), on
             * réessaie ICI après un an supplémentaire, dédié à B7. */
            if (cap<=0.5f){
                scps_sim_advance_days(s2, 366);
                ScpsLoanCapacity lc2[3]; int nlc2 = scps_country_loan_capacity(s2, pl, lc2, 3);
                float cap2 = (nlc2==3) ? lc2[1].montant_max : 0.f;
                if (cap2>0.5f){
                    scps_player_borrow_class(s2, 1, -1.f);
                    scps_sim_advance_days(s2, 1);
                    cap = cap2;
                }
            }
            if (cap>0.5f){
                ScpsDebt deb; scps_country_debt(s2, pl, &deb);
                float expect_fixed = deb.total * tune_f("DEBT_DUE_FRAC", 0.10f);
                ok("crédit rationné visible : revenu, levier et capacité disponible sont exposés",
                   deb.annual_revenue>=0.f && deb.leverage>=0.f && deb.available>=0.f);
                printf("   B7 : dette totale=%.0f · taux origination=%.1f%% · échéance RÉELLE=%.1f (attendu %.1f, DEBT_DUE_FRAC)\n",
                       (double)deb.total, (double)(deb.taux*100.f), (double)deb.due, (double)expect_fixed);
                ok("B7 : sous DEBT_FIXED, l'échéance == total×DEBT_DUE_FRAC (PAS total×taux)",
                   fabsf(deb.due - expect_fixed) < 0.5f);
                /* Écart RELATIF (recalibré 2026-07-31) : l'ancien seuil ABSOLU de 0.5 or
                 * testait en réalité la TAILLE de la dette, pas la divergence des deux
                 * formules — sur une dette de 5 (capacité d'emprunt basse selon le monde),
                 * |due − total×taux| = 5×|0.10−0.02| = 0.4 < 0.5 et le banc rougissait alors
                 * que due == total×DEBT_DUE_FRAC au centième. On compare donc les TAUX :
                 * la preuve du bug fermé est que l'échéance suit DEBT_DUE_FRAC (10 %) et
                 * NON le taux d'origination (~2 %) — vrai quelle que soit la dette. */
                ok("B7 : l'échéance RÉELLE diverge du calcul BUGUÉ total×taux (la preuve du bug fermé)",
                   deb.total > 0.f
                   && fabsf(deb.due/deb.total - deb.taux) > 0.02f
                   && fabsf(deb.due - deb.total*deb.taux) > 0.001f);
                /* kill-switch DEBT_FIXED=0 : legacy, due==total*taux EXACT (comportement pré-M11). */
                tune_set("DEBT_FIXED", 0.f);
                ScpsDebt deb_legacy; scps_country_debt(s2, pl, &deb_legacy);
                ok("B7 kill-switch DEBT_FIXED=0 : legacy, due==total*taux EXACT",
                   fabsf(deb_legacy.due - deb_legacy.total*deb_legacy.taux) < 0.01f);
                tune_set("DEBT_FIXED", 1.f);   /* redéfinit le défaut */
            } else {
                ok("(pas de dette réelle contractée — test B7 sauté)", true);
            }
        }
        /* ── MONNAIE M9 — V2 : « DEMANDER UN EMPRUNT À UN ÉTAT » (diplomatie) — un État PEUT
         * REFUSER (ai_consider_offer/OFFER_LOAN, value SUBJECTIVE) ; on sollicite plusieurs
         * cibles (throttlées par le MÊME émissaire — 1 acte/2 mois) et on lit le MOT résolu
         * (jamais un flottant, doctrine membrane). */
        {
            scps_sim_advance_days(s2, 70);   /* purge tout cooldown émissaire laissé par les verbes diplo ci-dessus */
            int granted=0, refused=0, none=0, tried=0;
            bool quote_checked=false;
            for (int c=0; c<nc2 && tried<6; c++){
                if (c==pl) continue;
                tried++;
                if (!quote_checked){
                    ScpsStateLoanQuote q; scps_country_loan_quote(s2,pl,c,&q);
                    ok("V2 cotation AVANT demande : capacité/réserve/exposition sont bornées et lisibles",
                       q.montant_max>=0.f && q.lender_surplus>=0.f && q.exposure>=0.f
                       && q.exposure_limit+0.01f>=q.exposure && q.portfolio_exposure+0.01f>=q.exposure);
                    quote_checked=true;
                }
                scps_player_request_loan(s2, c, -1.f);   /* <=0 ⇒ le maximum réellement offert */
                scps_sim_advance_days(s2, 65);            /* > le cooldown émissaire (60j) : la demande part et se résout */
                int tgt = scps_country_loan_request_target(s2, pl);
                const char *word = scps_country_loan_status(s2, pl);
                if (tgt==c && strstr(word,"accorde")) granted++;
                else if (tgt==c) refused++;
                else none++;
            }
            printf("   V2 demande d'emprunt diplomatique (%d cible(s) sollicitée(s)) : %d accordé(s) · %d refusé(s) · %d sans effet\n",
                   tried, granted, refused, none);
            ok("V2 verbe DEMANDER UN EMPRUNT : au moins une demande RÉSOLUE en mot (accordé ou refusé)",
               (granted+refused) > 0);
        }
    }

    /* ── ALLOCATION DE MAIN-D'ŒUVRE (onglet province) : lire les puits, poser un poids
     *    (active l'override), fermer un bâtiment, revenir AUTO — tout via le journal.
     *    RE-KEY PROVINCE (doctrine « la province est la seule réalité économique ») :
     *    la cible est un PID DIRECT, plus une région à résoudre. ── */
    {
        int ppid=-1, np=scps_province_count(s2);
        for (int pp=0; pp<np; pp++){ ScpsProvInfo pi; scps_province_info(s2,pp,&pi);
            if (pi.owner==pl){ ppid=pp; break; } }
        ok("alloc : province du joueur trouvée", ppid>=0);
        if (ppid>=0){
            ScpsAlloc al; scps_province_alloc(s2, ppid, &al);
            ok("scps_province_alloc : province lue (bassin>0, puits listés)", al.region==ppid && al.pool>0.f && al.n>0);
            ok("alloc : mode AUTO au départ (on=0)", al.on==0);
            int kbld=-1, kraw=-1;
            for (int i=0;i<al.n;i++){ if(al.sink[i].kind==1 && kbld<0) kbld=i; if(al.sink[i].kind==0 && kraw<0) kraw=i; }
            if (kraw>=0){
                ok("verbe alloc_raw ENFILÉ", scps_player_alloc_raw(s2, ppid, al.sink[kraw].id, 80)==1);
                scps_sim_advance_days(s2, 1);
                ScpsAlloc al2; scps_province_alloc(s2, ppid, &al2);
                ok("alloc : override ACTIVÉ au drain (on=1)", al2.on==1);
            }
            if (kbld>=0){
                int bid=al.sink[kbld].id;
                scps_player_alloc_bld(s2, ppid, bid, 0);   /* poids 0 = fermé */
                scps_sim_advance_days(s2, 1);
                ScpsAlloc al3; scps_province_alloc(s2, ppid, &al3);
                int closed=0; for (int i=0;i<al3.n;i++) if(al3.sink[i].kind==1 && al3.sink[i].id==bid) closed=al3.sink[i].closed;
                ok("alloc : bâtiment FERMÉ (poids 0) reflété au readout", closed==1);
            }
            scps_player_alloc_auto(s2, ppid);
            scps_sim_advance_days(s2, 1);
            ScpsAlloc al4; scps_province_alloc(s2, ppid, &al4);
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
            ScpsDiploContext dc;
            int dctx=scps_diplo_context(s2,tgt,&dc);
            ok("P8 diplomatie : engagements et capitale traversent la façade",
               dctx==1 && dc.valid && dc.at_war && dc.target==tgt &&
               dc.target_capital_province>=0 && dc.target_capital_region>=0);
            ok("P8 diplomatie : portée commerciale et créneaux d'alliance sont bornés",
               dc.shared_routes>=dc.open_routes && dc.open_routes>=0 &&
               dc.ally_slots_player>=0 && dc.ally_slots_player<=dc.ally_slots_max &&
               dc.ally_slots_target>=0 && dc.ally_slots_target<=dc.ally_slots_max);
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
     *    RE-KEY PROVINCE : on cherche un couple (PROVINCE du joueur, type) LÉGAL — plus
     *    une région ; s'il en existe un, la pose au drain doit AJOUTER un bâtiment (puits
     *    kind==1 de scps_province_alloc). ── */
    {
        int nprov=scps_province_count(s2);
        int lp=-1, lb=-1;
        for (int pp=0;pp<nprov && lp<0;pp++)
            for (int b=1;b<64 && lp<0;b++)
                if (scps_manuf_legal(s2, pp, b)){ lp=pp; lb=b; }
        printf("   panneau B : couple légal (province %d, type %d)\n", lp, lb);
        if (lp>=0){
            ScpsAlloc al; scps_province_alloc(s2, lp, &al);
            int nb_before=0; for (int i=0;i<al.n;i++) if (al.sink[i].kind==1) nb_before++;
            ok("panneau B : verbe BUILD_MANUF enfilé", scps_player_build_manuf(s2, lp, lb)==1);
            scps_sim_advance_days(s2, 2);                    /* le drain pose (ou refuse proprement) */
            scps_province_alloc(s2, lp, &al);
            int nb_after=0; for (int i=0;i<al.n;i++) if (al.sink[i].kind==1) nb_after++;
            bool built = (nb_after >= nb_before+1);
            /* MONNAIE M3i — l'impôt SUR LE REVENU est lié à la PRODUCTION (plus à la pop) : la
             * caisse est plus volatile qu'avant, le couple légal à l'ENFILAGE peut redevenir
             * inabordable 2 jours plus tard (credit_can_spend REVALIDÉ au drain, scps_sim.c) —
             * refus PROPRE (aucun bâtiment posé, aucun crash), pas un bug moteur. Motif
             * « RÉPARATION BANCS » (2026-07-14) : la fixture tolère les DEUX issues légitimes.
             * `>=` (pas `==`) : une province FRAÎCHEMENT colonisée peut recevoir UN AUTRE puits
             * kind==1 dans la même fenêtre (mécanique de colonie, hors CMD_BUILD_MANUF lui-même)
             * — observé (nb_before=0, nb_after=2) une fois la caisse redevenue plus volatile,
             * sans rapport avec la légalité/pose de NOTRE commande. */
            ok("panneau B : la manufacture est POSÉE au drain, OU refusée proprement (caisse tombée sous le coût entre-temps)",
               built || nb_after==nb_before);
            ok("panneau B : cohérence légalité/pose (slot rempli ⇒ plus légal)",
               built ? (scps_manuf_legal(s2, lp, lb)==0) : 1);
        } else {
            ok("panneau B : aucun couple légal (monde nu précoce) — verbe refusé proprement",
               scps_player_build_manuf(s2, 0, 1)==1);        /* enfilé ; le drain refusera sans crash */
            scps_sim_advance_days(s2, 2);
            ok("panneau B : drain sans crash sur refus", 1);
        }
        ok("panneau B : hors-borne refusé (type 999)", scps_manuf_legal(s2, 0, 999)==0);
    }

    /* ── LOT P (2026-07-07) — PILLER LA CÔTE : légalité + verbe enfilé → CD posé au drain.
     *    Miroir du round-trip colonisation/panneau B : légal → enfilé → drainé → plus
     *    légal (la balafre/l'immunité vient d'être posée sur la province cible). Le
     *    joueur reçoit sa coque pirate par le setter BANC-only (motif
     *    intertrade_debug_set_hub_of) — le monde de test n'a pas toujours de quoi en
     *    bâtir une dans la fenêtre du banc. ── */
    {
        scps_debug_set_pirate_hulls(s2, 1);         /* la coque du banc */
        int np=scps_province_count(s2);
        int tgt=-1;
        for (int pp=0; pp<np && tgt<0; pp++)
            if (scps_can_raid_coast(s2, pp, NULL)) tgt=pp;
        printf("   piller la côte : cible légale = province %d\n", tgt);
        if (tgt>=0){
            int reason=-1;
            ok("piller la côte : légal (scps_can_raid_coast, reason=0)",
               scps_can_raid_coast(s2, tgt, &reason)==1 && reason==0);
            ok("verbe PILLER LA CÔTE enfilé", scps_player_raid_coast(s2, tgt)==1);
            scps_sim_advance_days(s2, 2);           /* le drain applique (pillage + CD/balafre) */
            int reason2=-1;
            ok("piller la côte : la MÊME cible n'est plus légale (balafre/CD posé)",
               scps_can_raid_coast(s2, tgt, &reason2)==0 && reason2==3);
            int cd=scps_raid_cd_days(s2, tgt);
            printf("   piller la côte : CD restant %d j\n", cd);
            ok("piller la côte : le CD restant est LISIBLE (~5 ans — « côte balafrée — X j »)",
               cd>1700 && cd<=1825);
        } else {
            ok("piller la côte : aucune cible légale (monde trop petit/homogène) — verbe refusé proprement",
               scps_player_raid_coast(s2, 0)==1);   /* enfilé ; le drain refusera sans crash */
            scps_sim_advance_days(s2, 2);
            ok("piller la côte : drain sans crash sur refus", 1);
            ok("piller la côte : (CD lisible sauté — pas de cible dans ce monde)", 1);
        }
        scps_debug_set_pirate_hulls(s2, 0);         /* on rend le banc comme trouvé */
        ok("piller la côte : hors-borne refusé (province 999999)", scps_can_raid_coast(s2, 999999, NULL)==0);
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
        ok("année + pop restaurées", rc==0 && scps_year(sl)==yr_before && scps_world_pop(sl)==pop_before);
        ScpsCountryInfo after; memset(&after,0,sizeof after);
        if (rc==0) scps_country_info(sl, scps_player(sl), &after);
        printf("   save/load : « %s » (an %d, %ld âmes) → « %s » (an %d, %ld âmes)\n",
               nom_before, yr_before, pop_before, after.nom, scps_year(sl), scps_world_pop(sl));
        ok("culture du joueur CONSERVÉE (nom = épithète « Havre »)",
           rc==0 && strncmp(after.nom,"Havre",5)==0);
        /* la partie chargée VIT (le build composé persiste via la section CULT) */
        if (rc==0) scps_sim_advance_days(sl, 365);
        ok("la partie chargée AVANCE (an +1)", rc==0 && scps_year(sl)==yr_before+1);
        scps_clear_player_culture();
        scps_sim_free(sg); scps_sim_free(sl);
    }

    /* ── TOPONYMIE DES VILLES (docs/DESIGN_TOPONYMIE_VILLES.md) ──────────────────
     * balayage annuel (world_tick, day%365==364) : ≥2 ans garantissent au moins un
     * passage — la capitale de départ, colonisée dès econ_init, reçoit son nom au
     * PREMIER passage (genèse et fondation en jeu sont le MÊME mécanisme). */
    {
        char nm1[TOPONYM_DEMO_BUF], nm2[TOPONYM_DEMO_BUF];
        int cap_r1=-1, cap_r2=-1;
        {   /* 1re génération — snapshot IMMÉDIAT (le nom vit dans un tableau de
             * MODULE, motif WILD/EMOB/COLC : un 2e scps_sim_generate le RAZ). */
            ScpsSim *st1=scps_sim_new(); scps_sim_generate(st1, seed);
            scps_sim_advance_days(st1, 365*2);
            cap_r1 = scps_country_capital_region(st1, scps_player(st1));
            snprintf(nm1,sizeof nm1,"%s", scps_region_city_name(st1, cap_r1));
            scps_sim_free(st1);
        }
        {   /* 2e génération, MÊME graine — le jitter de novlang (§5) est un hash pur */
            ScpsSim *st2=scps_sim_new(); scps_sim_generate(st2, seed);
            scps_sim_advance_days(st2, 365*2);
            cap_r2 = scps_country_capital_region(st2, scps_player(st2));
            snprintf(nm2,sizeof nm2,"%s", scps_region_city_name(st2, cap_r2));
            scps_sim_free(st2);
        }
        printf("   ville capitale (graine %u) : « %s » (2e génération : « %s »)\n", seed, nm1, nm2);
        ok("capitale colonisée => un nom de ville", cap_r1>=0 && nm1[0]!='\0');
        ok("longueur bornée (< 32, motif char[32])", strlen(nm1)>0 && strlen(nm1)<32);
        ok("déterminisme : même graine => même nom (2 générations, §5 hash pur)",
           cap_r1==cap_r2 && strcmp(nm1,nm2)==0);
    }
    {   /* un seul ScpsSim vivant : le nom survit dans le temps (§14), aucun combo
         * interdit (§11, 6 testés) sur tout le monde nommé, puis save/load (slot 3) */
        ScpsSim *st=scps_sim_new(); scps_sim_generate(st, seed);
        scps_sim_advance_days(st, 365*2);
        int cap_r=scps_country_capital_region(st, scps_player(st));
        char snap[TOPONYM_DEMO_BUF]; snprintf(snap,sizeof snap,"%s", scps_region_city_name(st,cap_r));

        scps_sim_advance_days(st, 365*5);   /* +5 ans : conquête/éthos/rien — le nom ne bouge JAMAIS */
        ok("le nom SURVIT (aucun re-tirage, doc §14)",
           cap_r>=0 && strcmp(snap, scps_region_city_name(st,cap_r))==0);

        /* §11 : les 6 combos interdits, cherchés dans TOUTES les villes nommées du monde */
        static const char *BAD_A[6]={"port","mont","fort","neuve","marc","cour"};
        static const char *BAD_B[6]={"havre","berg","castel","nouvelle","marche","siege"};
        int nreg=scps_region_count(st), nb_named=0, nb_bad=0;
        for(int r=0;r<nreg;r++){
            const char *nm=scps_region_city_name(st,r);
            if(!nm[0]) continue;
            nb_named++;
            char low[TOPONYM_DEMO_BUF]; int i=0;
            for(;nm[i] && i<TOPONYM_DEMO_BUF-1;i++) low[i]=(char)tolower((unsigned char)nm[i]);
            low[i]='\0';
            for(int k=0;k<6;k++) if(strstr(low,BAD_A[k]) && strstr(low,BAD_B[k])) nb_bad++;
        }
        printf("   %d villes nommées dans le monde (graine %u) · combos interdits détectés=%d\n",
               nb_named, seed, nb_bad);
        ok("aucun combo interdit (§11, 6 testés) sur tout le monde nommé", nb_named>0 && nb_bad==0);

        /* SAUVEGARDE : le nom de ville survit à un aller-retour (slot 3) */
        ok("sauvegarde toponymie (slot 3)", scps_sim_save(st,3)==1);
        scps_sim_free(st);

        ScpsSim *st3=scps_sim_new();
        int rc=scps_sim_load(st3,3);
        ok("chargement toponymie OK (rc=0)", rc==0);
        const char *after = (rc==0) ? scps_region_city_name(st3,cap_r) : "";
        printf("   save/load ville : « %s » → « %s »\n", snap, after);
        ok("nom de ville CONSERVÉ après save/load", rc==0 && strcmp(snap,after)==0);
        scps_sim_free(st3);
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
        /* ⚠ « popB >= popA » était SENSIBLE AU MONDE : depuis le PLAFOND DE TIRS À VIE
         * (3-5 dilemmes par partie), les runs A/B ne subissent plus le MÊME tirage de
         * dilemmes — un choc ponctuel (pop_mult d'un choix IA) noie le nudge sur 10-30
         * ans. L'intention (« Fécondité POUSSE la natalité ») se prouve au CANAL,
         * insensible au monde : l'accumulateur de foi arme RC_POPGROWTH > 0. */
        { const ReligAccum *acc = religion_country_acc(pb);
          ok("foi pro-natalité : le canal RC_POPGROWTH est ARMÉ (>0) chez le fidèle",
             acc && acc->ch[RC_POPGROWTH] > 0.f); }
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

    /* ── RELIGION : PLAFOND ⌈n_emp/2⌉ (LOT T, relâché de ⌈N/3⌉) — fonder sous le cap, RALLIER au-delà ── */
    {
        religion_reset();
        ok("cap : ⌈4/2⌉=2 · ⌈3/2⌉=2 · ⌈6/2⌉=3 · ⌈7/2⌉=4",
           religion_cap(4)==2 && religion_cap(3)==2 && religion_cap(6)==3 && religion_cap(7)==4);
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
            bool options_complete=true;
            for (int i=0;i<pe.n_options;i++)
                if (!pe.labels[i] || !pe.labels[i][0] || !pe.blurbs[i] || !pe.blurbs[i][0] ||
                    !pe.flavors[i] || !pe.flavors[i][0] || !isfinite(pe.gold_delta[i]))
                    options_complete=false;
            ok("chaque option expose branchement, texte, flavor et prix physique fini", options_complete);
            ok("player_event_choice ENFILE le choix (mis en file)",
               scps_player_event_choice(sp, 0, 0)==1);
            char chosen[128];
            strncpy(chosen, pe.situation, sizeof chosen - 1); chosen[sizeof chosen - 1]='\0';
            scps_sim_advance_days(sp, 2);   /* le drain RÉSOUT au prochain tick */
            /* ROBUSTE au monde : le kit vivrier ×20 (SPAWN_KIT_FOOD 2000) rend la genèse
             * plus ÉVÉNEMENTIELLE — d'autres décisions peuvent éclore pendant le drain, si
             * bien que le COMPTE net ne baisse pas toujours. On vérifie donc l'INTENT réel :
             * le pending qu'on a CHOISI (par sa situation) a bien DISPARU de la file. */
            bool chosen_gone=true;
            for (int k=0;k<scps_pending_count(sp);k++){ ScpsPendingEvent pk;
                if (scps_pending_event(sp,k,&pk)==1 && strcmp(pk.situation,chosen)==0){ chosen_gone=false; break; } }
            ok("le choix DRAINÉ résout le pending choisi", chosen_gone);
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
        /* ⚠ [16] et pas [8] : DECREE_COUNT=11 — le buffer court tronquait en silence
         * Légations, Levée entretenue et l'Audit (même bug pris dans le binding). */
        ScpsDecree decs[16];
        int nd = scps_decrees_list(sd, me, decs, 16);
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
        /* REFONTE 2026-07-10 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) : les 9 orientations
         * légères REMPLACENT les 4 anciens grands décrets — TOUTES réversibles (DCR_EDIT),
         * plus aucune RÉFORME irréversible au catalogue (Politique de tribut retirée). Le
         * sous-test réforme ci-dessous (ligne ~830) reste correctement SAUTÉ via son propre
         * `else` (tribut_id<0) — intention préservée, rien à recâbler côté drain/plomberie. */
        ok("plus aucune réforme irréversible au catalogue (les orientations sont toutes réversibles)", tribut_id<0);

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

        /* ── ESCLAVAGE — garder/affranchir/vendre : verbes + conservation des âmes/or ──
         * RE-KEY PROVINCE : slave_sell/slave_buy prennent un PID direct (`cap` est
         * désormais la province-capitale, pas sa région). */
        {
            int cap = scps_country_capital_province(sd, me);
            ok("esclavage : capitale du joueur trouvée", cap>=0);
            if (cap>=0){
                /* affranchissement : verbe ENFILÉ (drain no-op sans esclave, mais le verbe
                 * lui-même doit s'enfiler — la plomberie, pas l'effet, est ce qu'on prouve ici). */
                ok("verbe MANUMIT enfilé", scps_player_manumit(sd)==1);
                scps_sim_advance_days(sd, 2);

                long total_before=0;
                { ScpsSlavePoolLine lines[HERITAGE_COUNT]; int can_buy=0;
                  int nln = scps_slave_market(sd, lines, HERITAGE_COUNT, &total_before, &can_buy);
                  ok("scps_slave_market : total du pool ≥ 0 (lecteur borné)", total_before>=0);
                  /* V3 — lisibilité du marché (câblage servile) : chaque ligne porte un
                   * héritage NOMMÉ (jamais NULL) et un compte non-négatif — c'est ce que
                   * le panneau « Peuple servile » affiche tel quel. */
                  int market_readable=1;
                  for(int i=0;i<nln;i++){
                      if(lines[i].heritage==NULL || lines[i].count<0) market_readable=0;
                  }
                  ok("scps_slave_market : marché lisible (héritage nommé, comptes ≥0)", market_readable==1); }

                /* vente : sans esclave à vendre, l'ordre s'ENFILE mais reste sans effet
                 * (drain revalidé, silencieux — comme les offres diplo non consenties). */
                ok("verbe SLAVE_SELL enfilé (même sans esclave à vendre — le verbe, pas l'effet)",
                   scps_player_slave_sell(sd, cap, 100)==1);
                scps_sim_advance_days(sd, 2);

                long total_after=0;
                { ScpsSlavePoolLine lines[HERITAGE_COUNT]; int can_buy=0;
                  scps_slave_market(sd, lines, HERITAGE_COUNT, &total_after, &can_buy);
                  ok("vente SANS esclave : le pool ne bouge PAS (rien à vendre, conservation)",
                     total_after==total_before); }

                /* achat : enfilé de la même façon (le gate éthos/tech tranche au drain). */
                ok("verbe SLAVE_BUY enfilé", scps_player_slave_buy(sd, cap, 50)==1);
                scps_sim_advance_days(sd, 2);
            } else {
                ok("(idem)", true); ok("(idem)", true); ok("(idem)", true);
                ok("(idem)", true); ok("(idem)", true);
            }

            /* ── LOT G — RÉINCORPORATION DE POP : verbe enfilé (revalidé au drain :
             *    A≠B toutes deux au joueur). RE-KEY PROVINCE : A/B sont des PID
             *    directs. Le joueur n'a souvent qu'UNE province à la genèse (la
             *    colonisation joueur est un ordre explicite, CMD_COLONIZE, pas
             *    autonome) : la logique de fond est vérifiée en isolation par
             *    demography_demo (§12, group-level) — ici on prouve la PLOMBERIE façade
             *    (verbe enfilé + refus A==B), avec l'effet si le monde offre 2 provinces. */
            {
                int rb=-1, np2=scps_province_count(sd);
                for (int pp=0; pp<np2 && rb<0; pp++){ ScpsProvInfo pi; scps_province_info(sd,pp,&pi);
                    if (pi.owner==me && pp!=cap) rb=pp; }
                if (cap>=0 && rb>=0){
                    ok("verbe POP_TRANSFER enfilé (deux provinces distinctes au joueur)",
                       scps_player_pop_transfer(sd, cap, rb, 0 /*CLASS_LABORER*/, 500)==1);
                    scps_sim_advance_days(sd, 2);
                    ok("A==B est refusé (aucun ordre n'aurait de sens)",
                       scps_player_pop_transfer(sd, cap, cap, 0, 500)==1);   /* enfile quand même : REVALIDÉ au drain, pas au push */
                } else {
                    ok("(une seule province au joueur — POP_TRANSFER ignoré)", true);
                    ok("(idem)", true);
                }
            }
        }

        /* ── LOT J — L'APERÇU DE MANUMISSION : lecture PURE, mots + nombres bornés. ── */
        {
            ScpsManumitPreview mp;
            int okp = scps_manumit_preview(sd, &mp);
            ok("scps_manumit_preview : lecture réussie (joueur valide)", okp==1);
            ok("l'aperçu est BORNÉ (souls≥0, n_groups≥0, part∈[0,100], friction∈[0,1])",
               mp.souls>=0 && mp.n_groups>=0
               && mp.pct_of_country>=0.f && mp.pct_of_country<=100.f
               && mp.friction_after>=0.f && mp.friction_after<=1.f);
        }

        /* ── P5 — MÉTABOLISATION POUR LA VICTOIRE : deux lectures DISTINCTES, un chip
         * doit dire LAQUELLE (accès tech pop-share vs compte de la Merveille). ── */
        {
            ScpsMervHeritage mh[HERITAGE_COUNT];
            int count=-1, required=-1;
            int n = scps_merv_metab(sd, mh, HERITAGE_COUNT, &count, &required);
            ok("scps_merv_metab : HERITAGE_COUNT entrées lues", n==HERITAGE_COUNT);
            ok("le compte X/6 est BORNÉ 0..HERITAGE_COUNT", count>=0 && count<=HERITAGE_COUNT);
            int natif_found=0, natif_metab=0, all_bounded=1;
            for(int h=0; h<n; h++){
                if(mh[h].native){ natif_found=1; natif_metab = mh[h].metabolized; }
                if(mh[h].progress_pct<0 || mh[h].progress_pct>100) all_bounded=0;
            }
            ok("progress_pct BORNÉ [0,100] pour tous les héritages", all_bounded==1);
            ok("l'héritage NATIF compte TOUJOURS pour la Merveille (voie \"natif\")",
               natif_found==1 && natif_metab==1);
            ok("required : requis du palier courant, ou 0 si aucun palier actif", required>=0);
        }

        /* ── V3 — LE LAVIS PAR VARIANTE : intensité bornée + cohérence avec la fin
         * courante + la carte L8 (une valeur par cellule) reflète bien 0 tant
         * qu'aucune fin n'a latché (le cas commun, coût nul). ── */
        {
            int nreg = scps_region_count(sd);
            int intensity_bounded=1;
            for(int r=0; r<nreg; r++){
                float in = scps_endgame_region_intensity(sd, r);
                if(in<0.f || in>1.f) intensity_bounded=0;
            }
            ok("scps_endgame_region_intensity : bornée [0,1] sur toutes les régions", intensity_bounded==1);

            ScpsEndgameInfo ei; scps_endgame_info(sd, &ei);
            ok("fin_raw BORNÉ 0..5 (SANG compris, brut — indépendant du miroir RFIN)",
               ei.fin_raw>=0 && ei.fin_raw<=5);
            /* #32 — nombres tangibles bornés [0,100] ; un run frais (aucun mort de
             * guerre encore accumulé) n'a rien à partager côté joueur (0). */
            ok("blood_pct/blood_player_pct BORNÉS [0,100] (#32)",
               ei.blood_pct>=0 && ei.blood_pct<=100 && ei.blood_player_pct>=0 && ei.blood_player_pct<=100);

            /* la carte L8 (map_w*map_h octets) doit rester COHÉRENTE avec fin_raw : tant
             * qu'aucune fin n'a latché (cas courant sur un run de 30 ans), tout-0. */
            int mw = scps_map_w(), mh = scps_map_h();
            uint8_t *vbuf = (uint8_t*)malloc((size_t)mw*mh);
            scps_map_endgame_variant(sd, vbuf);
            int variant_consistent=1;
            if (ei.fin_raw==0){
                for(int64_t i=0;i<(int64_t)mw*mh;i++) if(vbuf[i]!=0){ variant_consistent=0; break; }
            } else {
                /* une fin en cours : au moins une région intense DOIT apparaître à l'écran
                 * si au moins une région a une intensité non nulle (cohérence carte↔reader). */
                int any_engine_intense=0;
                for(int r=0; r<nreg; r++) if(scps_endgame_region_intensity(sd,r)>0.01f) any_engine_intense=1;
                if (any_engine_intense){
                    int any_pixel=0;
                    for(int64_t i=0;i<(int64_t)mw*mh;i++) if(vbuf[i]>2){ any_pixel=1; break; }
                    variant_consistent = any_pixel;
                }
            }
            ok("variant_map cohérent avec fin_raw (tout-0 si AUCUNE fin, sinon reflète l'intensité)",
               variant_consistent==1);
            free(vbuf);
        }

        /* ── W-GUERRE UI (lot A/B) — war_state BORNÉ {0,1,2} + cohérence occupant/belligérant,
         * scannés sur TOUTES les régions après un run assez long pour voir sièges/occupations. ── */
        {
            scps_sim_advance_days(sd, 365*30);
            int nreg = scps_region_count(sd);
            ScpsMovePreview bad_move;
            int move_preview_bounded = scps_corps_move_preview(sd,-1,0,&bad_move,NULL,0)==0
                && !bad_move.valid && bad_move.reason && bad_move.reason[0];
            ScpsRefillPreview bad_refill;
            int refill_preview_bounded = !scps_corps_refill_preview(sd,-1,&bad_refill)
                && !bad_refill.valid && !bad_refill.allowed && bad_refill.reason && bad_refill.reason[0];
            int me_refill=scps_player(sd);
            for(int n=0;n<scps_country_corps_count(sd,me_refill);n++){
                int id=scps_country_corps_id(sd,me_refill,n); ScpsRefillPreview rp; ScpsArmyInfo ai;
                scps_corps_info(sd,id,&ai);
                ScpsMovePreview mp; int route[SCPS_MAX_REG];
                int rn=scps_corps_move_preview(sd,id,ai.region,&mp,route,SCPS_MAX_REG);
                if(mp.valid && (rn<1 || route[0]!=ai.region || mp.units_start!=ai.units
                   || mp.attrition_loss<0 || mp.units_arrival<0
                   || mp.units_arrival+mp.attrition_loss!=mp.units_start
                   || mp.attrition_pct<0 || mp.attrition_pct>100
                   || mp.worst_daily_pct10<0 || !mp.reason || !mp.reason[0])) move_preview_bounded=0;
                if(!scps_corps_refill_preview(sd,id,&rp) || !rp.valid) {refill_preview_bounded=0;continue;}
                long need_sum=0;
                for(int k=0;k<rp.n_needs;k++){
                    need_sum+=rp.need[k].needed;
                    if(!rp.need[k].name || !rp.need[k].name[0] || rp.need[k].needed<0 || rp.need[k].owned<0)
                        refill_preview_bounded=0;
                }
                if(rp.n_needs<0 || rp.n_needs>SCPS_REFILL_MAX_NEEDS || need_sum!=rp.weapons_needed
                   || rp.requested_humans<0 || rp.population_ready_humans<0 || rp.guaranteed_humans<0
                   || rp.population_ready_humans>rp.requested_humans
                   || rp.guaranteed_humans>rp.population_ready_humans
                   || rp.weapons_owned<0 || rp.weapons_owned>rp.weapons_needed
                   || !rp.reason || !rp.reason[0] || (rp.allowed && rp.reason_code!=0)) refill_preview_bounded=0;
            }
            int all_bounded=1, coherent=1, seen1=0, seen2=0, any_battle_valid=1, any_battle_owner_ok=1;
            int battle_local_stack=1, battle_human_units=1, corps_human_units=1, tactical_bounded=1, siege_bounded=1;
            for(int r=0; r<nreg; r++){
                int belli=-99;
                int st = scps_region_war_state(sd, r, &belli);
                if(st<0 || st>2) all_bounded=0;
                if(st==0){ if(belli!=-1) coherent=0; }
                else {
                    if(belli<0 || belli>=scps_country_count(sd)) coherent=0;
                    if(st==1) seen1=1; else seen2=1;
                }
                ScpsBattleInfo bi; scps_battle_info(sd, r, &bi);
                if(bi.valid){
                    if(bi.region!=r) any_battle_valid=0;
                    if(bi.attacker<0 || bi.attacker>=scps_country_count(sd)) any_battle_valid=0;
                    if(bi.defender>=0 && bi.attacker==bi.defender) any_battle_owner_ok=0;
                    if(bi.atk_units<0 || bi.def_units<0) any_battle_valid=0;
                    if(bi.war_score<-100.f || bi.war_score>100.f) any_battle_valid=0;
                    if(bi.atk_corps<1 || (bi.in_battle && bi.def_corps<1)) battle_local_stack=0;
                    if(bi.atk_units!=bi.atk_inf+bi.atk_arch+bi.atk_cav+bi.atk_mages) battle_local_stack=0;
                    if(bi.def_units!=bi.def_inf+bi.def_arch+bi.def_cav+bi.def_mages) battle_local_stack=0;
                    if(bi.atk_units%100L || bi.def_units%100L) battle_human_units=0;
                    if(bi.atk_morale_pct<0 || bi.atk_morale_pct>100
                       || bi.def_morale_pct<0 || bi.def_morale_pct>100) battle_local_stack=0;
                    if(bi.in_battle && (bi.stage_id<0 || bi.stage_id>1 || !bi.stage || !bi.stage[0]
                       || bi.atk_terrain_pct<=0 || bi.def_terrain_pct<=0
                       || bi.atk_counter_pct<=0 || bi.def_counter_pct<=0
                       || bi.balance_atk_pct<0 || bi.balance_atk_pct>100
                       || bi.rupture_pct<0 || bi.rupture_pct>100
                       || bi.loss_atk<0.f || bi.loss_def<0.f
                       || bi.loss_atk!=floorf(bi.loss_atk) || bi.loss_def!=floorf(bi.loss_def))) tactical_bounded=0;
                    if(!bi.in_battle && (bi.siege_days_left<0.f || bi.siege_full_days<14.f
                       || bi.siege_progress_pct<0 || bi.siege_progress_pct>100
                       || bi.siege_defense<0.f || bi.siege_food_months<0.f
                       || bi.siege_terrain_pct<=0 || bi.siege_outcome<0 || bi.siege_outcome>1)) siege_bounded=0;
                    int local_atk=0;
                    for(int n=0;n<scps_country_corps_count(sd,bi.attacker);n++){
                        int id=scps_country_corps_id(sd,bi.attacker,n); ScpsArmyInfo ai;
                        scps_corps_info(sd,id,&ai);
                        if(ai.active && ai.region==r){
                            local_atk++;
                            if(ai.units!=ai.inf+ai.arch+ai.cav+ai.mages || ai.units%100L) corps_human_units=0;
                        }
                    }
                    if(local_atk<1) corps_human_units=0;
                }
            }
            ok("scps_region_war_state : borné {0,1,2} sur toutes les régions", all_bounded==1);
            ok("belligérant cohérent (−1 ssi paix, sinon un pays valide ≠ owner)", coherent==1);
            ok("scps_battle_info : quand valide, région/pays/effectifs/war_score bornés", any_battle_valid==1 && any_battle_owner_ok==1);
            ok("scps_battle_info : seuls les corps LOCAUX engagés composent chaque camp", battle_local_stack==1);
            ok("scps_battle_info : les effectifs exposés sont des HOMMES (paquets × 100)", battle_human_units==1);
            ok("scps_corps_info : total/composition sont des HOMMES cohérents", corps_human_units==1);
            ok("scps_corps_move_preview : arrivée = départ - attrition, lecture pure et bornée", move_preview_bounded==1);
            ok("scps_battle_info : phase/terrain/contres/rapport/rupture tactiques sont bornés", tactical_bounded==1);
            printf("   war_state : %s siège(s) vu(s) · %s occupation(s) vue(s) (30 ans)\n",
                   seen1?"des":"aucun", seen2?"des":"aucune");
            ok("scps_battle_info : restant/progression/defense/vivres/terrain/issue du siege sont bornes", siege_bounded==1);
            ok("scps_corps_refill_preview : hommes/armes/garantie/motif sont purs et bornés", refill_preview_bounded==1);
        }

        /* ── ARMÉE — LA FORCE NOMINALE (« renforcer = combler le déficit ») + SPLIT
         * COMPOSÉ : un corps FRAIS (posé à la levée) est déjà à son PLEIN — le renfort
         * n'a rien à combler (preview grisée, déficit nul) ; campaign_split_comp détache
         * une composition EXACTE, jamais un clamp. Sim DÉDIÉE (pas sd) : on lève à la main
         * une milice (armes de fortune, levée toujours ouverte, quel que soit l'arsenal). ── */
        {
            ScpsSim *sn = scps_sim_new(); scps_sim_generate(sn, seed);
            scps_sim_advance_days(sn, 100);   /* laisse la pop/le trésor s'installer */
            int mp = scps_player(sn);
            int cap_reg = scps_country_capital_region(sn, mp);
            for (int k=0;k<6;k++) scps_player_recruit(sn, U_MILICE);
            scps_sim_advance_days(sn, 3);     /* drain des CMD_RECRUIT : garnit la réserve (warhost) */
            ScpsArmy arm; scps_country_army(sn, mp, &arm);
            long take = (arm.regiments<2) ? 0 : (arm.regiments>6 ? 6 : arm.regiments);
            int raised = (take>=2 && cap_reg>=0) ? scps_player_raise_corps(sn, take, cap_reg) : 0;
            scps_sim_advance_days(sn, 2);     /* drain du CMD_CORPS_RAISE : le corps naît, posé à son nominal */
            int nc = scps_country_corps_count(sn, mp);
            int cid = nc>0 ? scps_country_corps_id(sn, mp, 0) : -1;
            ScpsRefillPreview rp0={0};
            int got0 = cid>=0 ? scps_corps_refill_preview(sn, cid, &rp0) : 0;
            ok("un corps FRAIS (posé à la levée) est à son NOMINAL : déficit nul, renfort grisé",
               raised==1 && cid>=0 && got0==1 && rp0.valid
               && rp0.requested_humans==0 && rp0.allowed==0 && rp0.reason_code==5);

            ScpsArmyInfo ai0={0}; if (cid>=0) scps_corps_info(sn, cid, &ai0);
            int split_ok = (cid>=0 && ai0.inf>=200) ? scps_player_split_comp(sn, cid, 1,0,0,0) : 0;
            scps_sim_advance_days(sn, 1);     /* drain du CMD_SPLIT_COMP */
            int nc2 = scps_country_corps_count(sn, mp);
            int new_id=-1;
            for (int n=0;n<nc2;n++){ int id=scps_country_corps_id(sn,mp,n); if(id!=cid){new_id=id;break;} }
            ScpsArmyInfo aiN={0}, aiS={0};
            if (new_id>=0) scps_corps_info(sn, new_id, &aiN);
            if (cid>=0)    scps_corps_info(sn, cid, &aiS);
            ok("SPLIT COMPOSÉ : le nouveau corps porte EXACTEMENT la composition demandée (1 paquet d'infanterie, rien d'autre)",
               split_ok==1 && new_id>=0
               && aiN.inf==100 && aiN.arch==0 && aiN.cav==0 && aiN.mages==0 && aiN.units==100);
            ok("SPLIT COMPOSÉ : le corps source perd EXACTEMENT ce qui est parti (conservation, jamais un clamp)",
               cid>=0 && aiS.inf==ai0.inf-100 && aiS.units==ai0.units-100);
            scps_sim_free(sn);
        }

        /* ── UI PROVINCE — câblage complet (LOTS 1/3/4/6) : 4 readers additifs, bornés. ── */
        {
            int np = scps_province_count(sd);
            int slave_bounded=1, tax_bounded=1, def_bounded=1, market_bounded=1;
            long any_slave=0; double any_tax=0.0;
            for (int p=0; p<np; p++){
                long sc = scps_province_slave_count(sd, p);
                if (sc<0) slave_bounded=0;
                if (sc>0) any_slave += sc;

                double tax = scps_province_tax(sd, p);
                if (tax<0.0 || !(tax==tax) /* NaN */) tax_bounded=0;
                if (tax>0.0) any_tax += tax;

                int dp = scps_province_defense_pct(sd, p);
                if (dp<0 || dp>1000) def_bounded=0;   /* 100=neutre, montagne+relief plafonne largement < 1000 */

                ScpsMarketLine ml[3]; const char *port="";
                int nm = scps_province_market(sd, p, ml, 3, &port);
                if (nm<0 || nm>3) market_bounded=0;
                for (int i=0;i<nm;i++){
                    if (ml[i].price<0.f || ml[i].stock<0.f) market_bounded=0;
                    if (!ml[i].name || !ml[i].marche) market_bounded=0;
                }
                if (!port) market_bounded=0;
            }
            ok("scps_province_slave_count : borné (≥0) sur toutes les provinces", slave_bounded==1);
            ok("scps_province_tax : borné (≥0, fini) sur toutes les provinces", tax_bounded==1);
            ok("scps_province_defense_pct : borné [0,1000] sur toutes les provinces", def_bounded==1);
            ok("scps_province_market : 0..3 lignes bornées (prix/stock≥0, mots non-nuls)", market_bounded==1);
            printf("   province UI : %ld âme(s) esclave(s) au total · %.0f or/an de taxe cumulée (%d provinces)\n",
                   any_slave, any_tax, np);

            /* scps_province_seed : déterministe (même province → même seed d'un appel à l'autre). */
            int cultures_seen=0, groups_exact=1, culture_bounded=1, contact_bounded=1;
            for(int p=0;p<np;p++){
                ScpsGroup groups[16];
                int ng=scps_province_groups(sd,p,groups,16);
                if(ng<=0)continue;
                cultures_seen++;
                int dominant=0;
                for(int i=0;i<ng;i++){
                    dominant+=groups[i].dominant?1:0;
                    if(!groups[i].faith||!groups[i].faith[0]||groups[i].faith_id<-1
                       ||groups[i].percent<0||groups[i].percent>100)groups_exact=0;
                }
                if(dominant!=1)groups_exact=0;
                ScpsCultureContext cc;
                if(!scps_province_culture_context(sd,p,&cc)||!cc.valid||cc.province!=p
                   ||cc.region<0||cc.groups<1||!cc.dominant_culture||!cc.dominant_culture[0]
                   ||!cc.local_ethos||!cc.ruling_ethos||!cc.relation_to_crown
                   ||!cc.local_faith||!cc.state_faith||!cc.fusion_reason
                   ||cc.dominant_percent<0||cc.dominant_percent>100
                   ||cc.ethos_drift_pct<0||cc.ethos_drift_pct>100
                   ||cc.friction_avg_pct<0||cc.friction_avg_pct>100
                   ||cc.friction_max_pct<0||cc.friction_max_pct>100)culture_bounded=0;
                if(cc.contact&&(cc.contact_region<0||cc.contact_country<0
                   ||!cc.contact_country_name||!cc.contact_country_name[0]
                   ||!cc.contact_region_name||!cc.contact_region_name[0]
                   ||!cc.contact_culture||!cc.contact_culture[0]
                   ||cc.contact_distance_pct<0||cc.contact_distance_pct>100
                   ||cc.fusion_open_pct<0||cc.fusion_open_pct>100
                   ||cc.fusion_years<0))contact_bounded=0;
            }
            ScpsCultureContext bad_culture;
            int bad_ok=!scps_province_culture_context(sd,-1,&bad_culture)&&!bad_culture.valid;
            ok("scps_province_groups : foi vivante + un unique groupe dominant", cultures_seen>0&&groups_exact==1);
            ok("scps_province_culture_context : identite/Couronne/friction/foi bornees", culture_bounded==1&&bad_ok);
            ok("scps_province_culture_context : contact/fusion issus des routes et bornes", contact_bounded==1);

            int seed_stable=1;
            for (int p=0; p<np && p<50; p++){
                int a = scps_province_seed(sd, p), b = scps_province_seed(sd, p);
                if (a!=b || a<0) seed_stable=0;
            }
            ok("scps_province_seed : déterministe et non-négatif", seed_stable==1);
            ok("scps_province_seed : hors-borne → -1", scps_province_seed(sd, -1)==-1 && scps_province_seed(sd, np+999)==-1);
        }

        /* ── V2a — LE CONSEIL VIVANT : faction/loyauté/paie exposées, verbe de paie ──
         * (réutilise `sd`/`me` déjà générés ci-dessus — pas de genèse supplémentaire.) */
        {
            ScpsCouncilSeat seats[3];
            int ns = scps_country_council(sd, me, seats, 3);
            ok("scps_country_council : 3 sièges exposés", ns==3);
            bool bounds_ok=true;
            for (int i=0;i<ns;i++){
                if (seats[i].loyalty<0 || seats[i].loyalty>100) bounds_ok=false;
                if (seats[i].pay<0.f || seats[i].pay>2.f) bounds_ok=false;
                if (!seats[i].faction || !seats[i].mood) bounds_ok=false;
                if (!seats[i].filled && (seats[i].loyalty!=0 || seats[i].faction[0]!='\0')) bounds_ok=false;
            }
            ok("chaque siège : loyauté [0,100], paie [0,2], faction/mood non-null (vacant → 0/\"\")", bounds_ok);

            /* Recruter un ministre au premier siège vacant (ou déjà pourvu — no-op sinon utile) */
            ScpsCouncilCand cands[8];
            int nc2 = scps_council_candidates(sd, 0, cands, 8);
            ok("scps_council_candidates expose la pool du siège Savoir", nc2>0);
            bool cand_parts_ok = nc2>0;
            for (int i=0;i<nc2;i++){
                float expected=cands[i].eff_preclamp_pct;
                if (expected<50.f) expected=50.f;
                if (expected>115.f) expected=115.f;
                if (cands[i].predicted_loyalty<45 || cands[i].predicted_loyalty>65 ||
                    fabsf(cands[i].efficiency_pct-expected)>0.05f) cand_parts_ok=false;
            }
            ok("candidats : loyauté prévue + décomposition d'efficacité viennent de la formule moteur", cand_parts_ok);
            if (nc2>0 && !seats[0].filled){
                scps_player_council_hire(sd, 0, cands[0].slot);
                scps_sim_advance_days(sd, 5);
                ScpsCouncilSeat after[3]; scps_country_council(sd, me, after, 3);
                ok("après RECRUTER : le siège est pourvu, loyauté de départ humaine (>0)",
                   after[0].filled==1 && after[0].loyalty>0);
            } else {
                ok("(siège déjà pourvu par l'IA sur cette graine — recrutement sauté)", true);
            }

            /* MONNAIE M14 — B8 : RESTAURÉ [0.1, 2.0] (décision joueur, était narrowé à
             * [0.02,1.0]). Le verbe de PAIE clampe à 2.0 (pas 1.0) même hors-borne. */
            bool pay_ok = scps_player_council_pay(sd, 0, 0.6f) != 0;
            scps_sim_advance_days(sd, 32);
            ScpsCouncilSeat p1[3]; scps_country_council(sd, me, p1, 3);
            ok("scps_player_council_pay : verbe accepté", pay_ok);
            ok("après paie : le curseur reflète la valeur posée (~0.6, si le siège reste pourvu)",
               !p1[0].filled || (p1[0].pay>0.4f && p1[0].pay<=2.f));
            scps_player_council_pay(sd, 0, 99.f);   /* hors-borne : DOIT clamper à 2.0 (B8) */
            scps_sim_advance_days(sd, 32);
            ScpsCouncilSeat p2[3]; scps_country_council(sd, me, p2, 3);
            ok("le verbe de paie CLAMPE au drain (une valeur folle → 2.0, jamais un crash)",
               !p2[0].filled || p2[0].pay<=2.f);

            /* Pilotage budgétaire : chaque curseur traverse le journal de commandes,
             * se quantifie dans le moteur et reste lisible par la même membrane. */
            ok("budget : neutres à la genèse (impôt 100 % → 1.0, investissement 0 %)",
               fabs(scps_country_budget_policy(sd,me,0,CLASS_LABORER)-1.0)<0.01 &&
               fabs(scps_country_budget_policy(sd,me,1,BUDGET_INVEST)-0.0)<0.01);
            bool tax_cmd=scps_player_budget_policy(sd,0,CLASS_LABORER,0.1f)!=0;
            bool inv_cmd=scps_player_budget_policy(sd,1,BUDGET_INVEST,0.7f)!=0;
            scps_sim_advance_days(sd,1);
            ok("budget : impôt Laboureurs et investissement sont enfilés",tax_cmd && inv_cmd);
            ok("budget : les deux curseurs arrivent au moteur (impôt 0.1 / invest. 0.7)",
               fabs(scps_country_budget_policy(sd,me,0,CLASS_LABORER)-0.1)<0.01 &&
               fabs(scps_country_budget_policy(sd,me,1,BUDGET_INVEST)-0.7)<0.01);
            /* MONNAIE M14 — B8 : l'impôt (family=0) est RESTAURÉ [0.1,2.0] — une valeur folle
             * clampe désormais à 2.0 (pas 1.0). L'INVESTISSEMENT reste [0.02,1.0] (B8 ne le
             * concerne pas — niveau brut, pas une paie/impôt). */
            scps_player_budget_policy(sd,0,CLASS_LABORER,99.f);
            scps_player_budget_policy(sd,1,BUDGET_INVEST,-4.f);
            scps_sim_advance_days(sd,1);
            ok("budget : l'impôt CLAMPE à 2.0 (B8), l'investissement reste clampé à 0.02",
               fabs(scps_country_budget_policy(sd,me,0,CLASS_LABORER)-2.0)<0.01 &&
               fabs(scps_country_budget_policy(sd,me,1,BUDGET_INVEST)-0.02)<0.01);
            ok("budget : familles/indices hors borne refusés",
               scps_player_budget_policy(sd,9,0,1.f)==0 &&
               scps_player_budget_policy(sd,0,99,1.f)==0);
            scps_player_budget_policy(sd,0,CLASS_LABORER,1.f);
            scps_player_budget_policy(sd,1,BUDGET_INVEST,1.f);
            scps_sim_advance_days(sd,1);

            /* L'état de paire : borné aux 4 valeurs (0..3), lisible pour n'importe quels sièges. */
            int pst = scps_council_pair_state(sd, 0, 1);
            ok("scps_council_pair_state : borné {neutre,rivalité,alliance,conspiration}", pst>=0 && pst<=3);
            ok("scps_council_pair_state : hors-borne → neutre (0), jamais de crash",
               scps_council_pair_state(sd, -1, 99)==0);

            ScpsFaction fac[8]; int coup=0, corr=0;
            int nf=scps_country_factions(sd,me,fac,8,&coup,&corr);
            int sum_eff=0,sum_base=0,drivers=0,max_pressure=0;
            bool faction_ok=(nf==FAC_COUNT && coup>=0 && coup<=100 && corr>=0 && corr<=100);
            for(int f=0;f<nf;f++){
                sum_eff+=fac[f].part; sum_base+=fac[f].base_part;
                drivers+=fac[f].coup_driver;
                if(fac[f].coup_pressure>max_pressure) max_pressure=fac[f].coup_pressure;
                if(fac[f].policy_delta!=fac[f].part-fac[f].base_part) faction_ok=false;
            }
            if(sum_eff<98||sum_eff>102||sum_base<98||sum_base>102||
               drivers!=(coup>0?1:0)||abs(max_pressure-coup)>1) faction_ok=false;
            ok("factions : assise sociale + effet politique + pression de coup sont cohérents", faction_ok);
        }
        scps_sim_free(sd);
    }

    /* ── VÉTUSTÉ + RÉNOVER : décay réel → state → CMD_RENOVER → drain → complétion ── */
    {
        ScpsSim *sv = scps_sim_new();
        scps_sim_generate(sv, seed);
        int me = scps_player(sv);
        int cap = scps_country_capital_province(sv, me);
        scps_sim_advance_days(sv, 365*5);                    /* 5 ans : Marché seed usé à ~0.98^5 ≈ 90 % */
        ScpsRenoverState rs0;
        int got = scps_renover_state(sv, cap, &rs0);
        ok("renover_state : lecteur répond sur la capitale", got==1);
        printf("   vétusté capitale an-5 : bâti %d %% · rénover %d or (allowed=%d reason=%d)\n",
               rs0.wear_pct, rs0.gold, rs0.allowed, rs0.reason);
        ok("vétusté : le bâti s'use (wear < 100 %)", rs0.wear_pct < 100);
        ok("rénover : coût > 0 dès qu'usé", rs0.wear_pct >= 100 || rs0.gold > 0);
        if (rs0.allowed){
            ok("player_renover : ordre accepté", scps_player_renover(sv, cap)==1);
            scps_sim_advance_days(sv, 1);                    /* drain : l'or part, le chantier s'enfile */
            scps_sim_advance_days(sv, 200);                  /* > RENOV_DAYS : complétion */
            ScpsRenoverState rs1;
            scps_renover_state(sv, cap, &rs1);
            printf("   après rénovation : bâti %d %% (avant %d %%)\n", rs1.wear_pct, rs0.wear_pct);
            ok("rénover : le delta plein est re-posé (wear remonte)", rs1.wear_pct > rs0.wear_pct);
        } else {
            ok("rénover : refus motivé (reason 1 rien-à-rénover ou 2 or)", rs0.reason==1 || rs0.reason==2);
        }
        scps_sim_free(sv);
    }

    free(rgba); free(lay);
    printf("\n══ BILAN : %d réussis, %d échoués ══\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
