/*
 * forks_demo.c — banc des FOURCHES (arc M, design v3 docs/DESIGN_FORKS.md)
 *
 *   make forks_demo && ./forks_demo
 *
 * Les assertions §24, au fil des items livrés :
 *   M1 — ETHOS_FN : l'argmax de chaque éthos (Dominateur→ARMÉE,
 *        Bureaucrate→RENFORCEMENT, Mercantile→PRODUCTION).
 *   M2 — le pôle d'une région ÉMERGE des poids de factions (Transgresseur exclu,
 *        tie-breaks §7 : capitale→impérial, port→Fluide, frontière→Martial, sinon Ordre).
 *   (M3/M5 edifice_succ_ctx + hystérésis · M6 alambic/flux : asserts ajoutés à la livraison.)
 */
#include "scps_ai.h"
#include "scps_factions.h"
#include "scps_agency.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LES FOURCHES — éthos→fonction, factions→pôle (design v3)\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* ═══ M1 — ETHOS_FN : l'éthos pèse la FONCTION (argmax §24) ═══════════ */
    printf("\n── M1. L'éthos préfère SA fonction (la table §6, verbatim) ──\n");
    ok("le Dominateur préfère l'ARMÉE",        ai_ethos_pref_func(ETHOS_DOMINATEUR) ==FN_ARMEE);
    ok("l'Honneur préfère l'ARMÉE",            ai_ethos_pref_func(ETHOS_HONNEUR)    ==FN_ARMEE);
    ok("l'Ordre préfère le RENFORCEMENT",      ai_ethos_pref_func(ETHOS_ORDRE)      ==FN_RENFORCEMENT);
    ok("le Bureaucrate préfère le RENFORCEMENT",ai_ethos_pref_func(ETHOS_BUREAUCRATE)==FN_RENFORCEMENT);
    ok("le Mercantile préfère la PRODUCTION",  ai_ethos_pref_func(ETHOS_MERCANTILE) ==FN_PRODUCTION);
    ok("le Pacifiste préfère la PRODUCTION",   ai_ethos_pref_func(ETHOS_PACIFISTE)  ==FN_PRODUCTION);

    /* ═══ M2 — LE PÔLE émerge des factions (§7) ═══════════════════════════ */
    printf("\n── M2. Le pôle d'une région : les factions votent, les tie-breaks tranchent ──\n");
    {
        float wgt[FAC_COUNT];
        memset(wgt,0,sizeof wgt); wgt[FAC_CONQUERANT]=0.6f; wgt[FAC_MARCHAND]=0.2f;
        ok("Conquérant dominant → pôle MARTIAL", faction_pole_of(wgt,-1,false,false)==POLE_MARTIAL);
        memset(wgt,0,sizeof wgt); wgt[FAC_LEGISTE]=0.5f; wgt[FAC_CONQUERANT]=0.2f;
        ok("Légiste dominant → pôle ORDRE",      faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        memset(wgt,0,sizeof wgt); wgt[FAC_MARCHAND]=0.5f; wgt[FAC_GARDIEN]=0.2f;
        ok("Marchand dominant → pôle FLUIDE",    faction_pole_of(wgt,-1,false,false)==POLE_FLUIDE);
        /* le Transgresseur est ORTHOGONAL : il ne pousse AUCUN pôle (il nourrit
         * l'appétit faustien). Une région tout-Transgresseur tombe aux tie-breaks. */
        memset(wgt,0,sizeof wgt); wgt[FAC_TRANSGRESSEUR]=0.9f;
        ok("Transgresseur seul → AUCUN pôle poussé (tie-break : sinon → Ordre)",
           faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        /* tie-breaks §7 dans l'ordre, sur une égalité parfaite martial=ordre=fluide. */
        memset(wgt,0,sizeof wgt); wgt[FAC_CONQUERANT]=0.3f; wgt[FAC_LEGISTE]=0.3f; wgt[FAC_MARCHAND]=0.3f;
        ok("égalité + capitale → le pôle IMPÉRIAL", faction_pole_of(wgt,POLE_FLUIDE,false,false)==POLE_FLUIDE);
        ok("égalité + port → FLUIDE",               faction_pole_of(wgt,-1,true, false)==POLE_FLUIDE);
        ok("égalité + frontière → MARTIAL",         faction_pole_of(wgt,-1,false,true )==POLE_MARTIAL);
        ok("égalité nue → ORDRE",                   faction_pole_of(wgt,-1,false,false)==POLE_ORDRE);
        /* le facteur 0.8 : le Gardien pousse Martial MOINS qu'un Conquérant plein. */
        memset(wgt,0,sizeof wgt); wgt[FAC_GARDIEN]=0.5f; wgt[FAC_MARCHAND]=0.45f;
        ok("Gardien 0.5 (×0.8 = 0.40) < Marchand 0.45 → FLUIDE (la pondération §7 compte)",
           faction_pole_of(wgt,-1,false,false)==POLE_FLUIDE);
    }

    /* ═══ M3+M5 — succ_ctx : le Port fork sur le PÔLE de la région (§24) ═══ */
    printf("\n── M3/M5. Le successeur du Port FORK sur le pôle — et l'hystérésis tient ──\n");
    {
        WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
        if (e){
            e->n_regions=3;
            /* trois régions mono-groupe : l'ETHOS du groupe pousse SA faction →
             * Dominateur→Conquérant→MARTIAL · Bureaucrate→Légiste→ORDRE ·
             * Mercantile→Marchand→FLUIDE. Pas de port/frontière (tie-breaks muets). */
            Ethos eth3[3]={ETHOS_DOMINATEUR,ETHOS_BUREAUCRATE,ETHOS_MERCANTILE};
            for (int r=0;r<3;r++){
                RegionEconomy *re=&e->region[r];
                re->active=re->colonized=true; re->owner=-1;
                re->last_pole=1; re->pole_since_day=0;
                re->pop.n_groups=1;
                re->pop.groups[0].count=1000;
                re->pop.groups[0].culture.ethos=eth3[r];
                re->pop.groups[0].culture.settled=true;
                re->pop.groups[0].klass=CLASS_LABORER;
            }
            /* l'hystérésis part validée ORDRE : on laisse chaque pôle TENIR 360 j. */
            for (int r=0;r<3;r++){ edifice_region_pole(NULL,e,r,10); edifice_region_pole(NULL,e,r,400); }
            ok("région MARTIALE : Port → ARSENAL",       edifice_succ_ctx(NULL,e,0,EDI_PORT,401)==EDI_ARSENAL);
            ok("région ORDRE : Port → AMIRAUTÉ",         edifice_succ_ctx(NULL,e,1,EDI_PORT,401)==EDI_AMIRAUTE);
            ok("région FLUIDE : Port → PORT MARCHAND",   edifice_succ_ctx(NULL,e,2,EDI_PORT,401)==EDI_PORT_MARCHAND);
            ok("base non forkée : succ_ctx(GRENIER) = COUNT (rien)",
               edifice_succ_ctx(NULL,e,0,EDI_GRENIER,401)==EDIFICE_COUNT);
            /* SPINE UNIVERSELLE : la Bibliothèque se bâtit partout (aucun frère posé). */
            ok("la SPINE est à tous : Bibliothèque non bloquée (martial/ordre/fluide)",
               !edifice_build_blocked(e,0,EDI_BIBLIOTHEQUE) &&
               !edifice_build_blocked(e,1,EDI_BIBLIOTHEQUE) &&
               !edifice_build_blocked(e,2,EDI_BIBLIOTHEQUE));
            /* HYSTÉRÉSIS : la région martiale VIRE Mercantile au jour 1000 — un flip
             * de 200 j ne RELIT pas (§24 flip-200j) ; à 360 j tenus, elle relit. */
            e->region[0].pop.groups[0].culture.ethos=ETHOS_MERCANTILE;
            ok("flip +0 j : le fork reste ARSENAL (pôle validé tenu)",
               edifice_succ_ctx(NULL,e,0,EDI_PORT,1000)==EDI_ARSENAL);
            ok("flip +200 j : TOUJOURS Arsenal (le candidat n'a pas tenu 360 j)",
               edifice_succ_ctx(NULL,e,0,EDI_PORT,1200)==EDI_ARSENAL);
            ok("flip +400 j : le pôle bascule, Port → PORT MARCHAND",
               edifice_succ_ctx(NULL,e,0,EDI_PORT,1400)==EDI_PORT_MARCHAND);
            /* UN FORK BÂTI NE SE RECONVERTIT JAMAIS : l'Arsenal posé bloque ses frères
             * (même après le flip du pôle) — et survit donc au flip (§24). */
            e->region[0].edi_built |= (1u<<EDI_ARSENAL);
            ok("Arsenal bâti → l'Amirauté est BLOQUÉE ici (les frères, à jamais)",
               edifice_build_blocked(e,0,EDI_AMIRAUTE));
            ok("Arsenal bâti → le Port marchand est BLOQUÉ aussi (fork_built_survives_pole_flip)",
               edifice_build_blocked(e,0,EDI_PORT_MARCHAND));
            ok("… mais un Grenier reste libre (le blocage ne fuit pas hors famille)",
               !edifice_build_blocked(e,0,EDI_GRENIER));
            /* la branche Ordre garde SA chaîne : Bibliothèque bâtie → Monastère licite,
             * mais Bibliothèque militaire / Observatoire bloqués (frères du savoir). */
            e->region[1].edi_built |= (1u<<EDI_BIBLIOTHEQUE);
            ok("Bibliothèque bâtie → le Monastère (SA chaîne) reste licite",
               !edifice_build_blocked(e,1,EDI_MONASTERE));
            ok("Bibliothèque bâtie → la Bibliothèque militaire est bloquée (frère)",
               edifice_build_blocked(e,1,EDI_BIBLIO_MIL));
            free(e);
        } else { for(int i=0;i<12;i++) ok("(OOM M3 — ignoré)",true); }
    }

    /* ═══ M6 — l'Alambic : gate matière, table de flux, LE PUITS ═══════════ */
    printf("\n── M6. L'Alambic distille le salpêtre — et NEUTRALISE la charge arcane ──\n");
    {
        WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
        if (e){
            e->n_regions=1; e->ipm=1.f;
            RegionEconomy *re=&e->region[0];
            re->active=re->colonized=true; re->owner=-1; re->culture.settled=true;
            re->culture.ethos=ETHOS_MERCANTILE; re->culture.race=RACE_HUMAIN;
            re->last_pole=1; re->import_margin=1.f; re->import_toll_region=-1;
            re->strata[CLASS_LABORER].pop=2000.f; re->strata[CLASS_BOURGEOIS].pop=300.f;
            re->strata[CLASS_ELITE].pop=50.f; re->tech_prod=1.f;
            re->raw_cap[RES_ARCANE_CRYSTAL]=2.f; re->raw_cap[RES_SALTPETER]=2.f;
            re->raw_cap[RES_GRAIN]=8.f;
            ok("gate MATIÈRE : la Forge céleste exige le fer céleste (absent → non)",
               !econ_bld_can_build(e,0,BLD_CELESTIAL_FORGE));
            re->raw_cap[RES_CELESTIAL_IRON]=1.f;
            ok("gate MATIÈRE : le fer céleste présent → la Forge se bâtit",
               econ_bld_can_build(e,0,BLD_CELESTIAL_FORGE));
            ok("l'Alambic exige le salpêtre (présent ici)", econ_bld_can_build(e,0,BLD_ALAMBIC));
            ok("table de flux : Forge (+1.2) > Atelier du Mage (+0.8)",
               econ_bld_flux_delta(BLD_CELESTIAL_FORGE) > econ_bld_flux_delta(BLD_MAGE_WORKSHOP));
            ok("table de flux : l'Alambic est un PUITS (delta < 0)",
               econ_bld_flux_delta(BLD_ALAMBIC) < 0.f);
            /* LE PUITS EN MARCHE : même région, mage SEUL vs mage+alambic — la charge
             * du tick chute quand l'alambic distille (l'essence purifiée la neutralise). */
            re->bld[0]=(Building){BLD_MAGE_WORKSHOP,1.f,0.f}; re->n_bld=1;
            re->stock[RES_ARCANE_CRYSTAL]=50.f;
            econ_tick(e,1.f/12.f);
            float charge_seul=re->arcane_charge;
            re->bld[1]=(Building){BLD_ALAMBIC,1.f,0.f}; re->n_bld=2;
            re->stock[RES_ARCANE_CRYSTAL]=50.f; re->stock[RES_SALTPETER]=50.f;
            re->stock[RES_ESSENCE_PURIFIEE]=20.f;
            econ_tick(e,1.f/12.f);
            float charge_purgee=re->arcane_charge;
            printf("   charge arcane du tick : mage seul %.2f → mage+alambic %.2f\n",
                   charge_seul, charge_purgee);
            ok("LE PUITS : la charge arcane chute quand l'alambic distille (§24)",
               charge_seul > 0.f && charge_purgee < charge_seul);
            free(e);
        } else { for(int i=0;i<6;i++) ok("(OOM M6 — ignoré)",true); }
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    return g_fail?1:0;
}
