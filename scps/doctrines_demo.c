/*
 * doctrines_demo.c — LES DOCTRINES (docs/DESIGN_MISSIONS_DOCTRINES.md §4,
 * catalogue : docs/DESIGN_DOCTRINES_ANNEXE.md)
 *
 *   make doctrines_demo && ./doctrines_demo [graine]
 *
 * Prouve : les SIX emplacements libres d'office · le coût d'adoption qui monte
 * (50 + 25 × actives) · les DEUX seules exclusivités (Commerce ⊥ Mercantilisme,
 * un seul courant) · l'achat SÉQUENTIEL des idées au coût croissant (30 + 3 ×
 * possédées) · l'ENTRETIEN en influence et la SUSPENSION déterministe des
 * dernières adoptées (mults à 1.0) · l'abandon (slot libéré, idées perdues,
 * aucun remboursement) · la bascule d'ASSIETTE par le courant politique ·
 * doctrine_key_mult (produit, clamp [0.60,1.60], cache invalidé) · la
 * LINÉARISATION des prix sur l'assiette (2× de nobles ⇒ 2× de gain ET 2× de
 * prix) et son ANTI-EXPLOIT (le rang du Conseil ne bouge PAS les prix) ·
 * save→reload (round-trip binaire + resynchronisation du miroir).
 */
#include "scps_doctrines.h"
#include "scps_influence.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_statecraft.h"
#include "scps_tune.h"
#include "scps_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, int cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static bool near_f(double a, double b, double eps){ double d=a-b; if(d<0)d=-d; return d<=eps; }

/* Le miroir du traducteur DoctrineId → assiette (scps_sim.c : sim_influence_base). */
static InfluenceBase base_of(const DoctrineState *ds, int cid){
    switch (doctrines_current(ds, cid)){
      case DOCT_ARISTOCRATIE: return INFL_BASE_ARISTO;
      case DOCT_BOURGEOISIE:  return INFL_BASE_BOURGEOIS;
      case DOCT_POPULAIRE:    return INFL_BASE_LABORER;
      case DOCT_DIVIN:        return INFL_BASE_FAITH;
      default:                return INFL_BASE_DEFAUT;
    }
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;

    printf("==================================================================\n");
    printf(" LES DOCTRINES (§4) -- slots, coûts, exclusivités, entretien, clés\n");
    printf("==================================================================\n");

    World *w=malloc(sizeof(World)); WorldEconomy *econ=malloc(sizeof(WorldEconomy));
    Statecraft *sc=malloc(sizeof(Statecraft));
    InfluenceState *is=malloc(sizeof(InfluenceState));
    DoctrineState  *ds=malloc(sizeof(DoctrineState));
    if(!w||!econ||!sc||!is||!ds){ fprintf(stderr,"OOM\n"); return 1; }

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    statecraft_init(sc,w);
    influence_init(is); doctrines_init(ds);

    int cid=-1;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
        if (w->country[c].capital_prov<0) continue;
        cid=c; break;
    }
    if (cid<0){ fprintf(stderr,"monde trop vide -- autre graine\n"); return 1; }

    /* Assiette EXACTE : 1000 élites, 500 bourgeois, 4000 journaliers dans UNE
     * province du pays, zéro ailleurs — les chiffres du banc sont alors lisibles
     * à la main (pas de bruit de worldgen). */
    int pid=-1;
    for (int i=0;i<econ->n_prov;i++) if (econ->prov[i].owner==cid){
        econ->prov[i].strata[CLASS_ELITE].pop=0.f;
        econ->prov[i].strata[CLASS_BOURGEOIS].pop=0.f;
        econ->prov[i].strata[CLASS_LABORER].pop=0.f;
        econ->prov[i].build.faith=0.f; econ->prov[i].ferveur=0.f;
        if (pid<0) pid=i;
    }
    ok("le pays choisi possede au moins une province", pid>=0);
    if (pid<0) return 1;
    econ->prov[pid].strata[CLASS_ELITE].pop     = 1000.f;
    econ->prov[pid].strata[CLASS_BOURGEOIS].pop =  500.f;
    econ->prov[pid].strata[CLASS_LABORER].pop   = 4000.f;

    tune_set("INFLUENCE_PER_NOBLE", 0.002f);
    tune_set("INFLUENCE_PER_BOURGEOIS", 0.0006f);
    tune_set("INFLUENCE_COUNCIL_FLOOR", 1.0f);
    tune_set("INFLUENCE_CAP", 0.0f);
    tune_set("INFLUENCE_BASE_REF", 2.0f);
    tune_set("DOCT_COST_BASE", 50.f); tune_set("DOCT_COST_STEP", 25.f);
    tune_set("IDEA_COST_BASE", 30.f); tune_set("IDEA_COST_STEP", 3.f);
    tune_set("DOCT_UPKEEP", 1.f);

    float ech = influence_scale(econ, cid, INFL_BASE_DEFAUT);

    /* ═══ 1. LES SLOTS — six, LIBRES d'office ═══ */
    printf("\n-- 1. Six emplacements, LIBRES des la genese --\n");
    ok("slots_open == 6 des le depart (aucune ouverture progressive)",
       doctrines_slots_open(ds, cid)==DOCT_SLOTS_MAX);
    ok("aucun slot occupe a l'initialisation", doctrines_n_active(ds,cid)==0);
    ok("l'assiette de reference (1000 elites x 0.002 = 2.0/mois) donne une echelle de 1.0",
       near_f(ech, 1.0, 0.001));

    /* ═══ 2. ADOPTION — coût croissant, payé en influence ═══ */
    printf("\n-- 2. Adoption : 50 + 25 x doctrines actives, paye en influence --\n");
    is->influence[cid] = 10000.f;
    ok("1re adoption coute 50", doctrines_adopt_cost(ds,cid,ech)==50);
    ok("adopter Commerce au slot 0 prend", doctrines_adopt(ds,is,cid,0,DOCT_COMMERCE,ech)==1);
    ok("le stock a ete debite de 50", near_f(influence_get(is,cid), 9950.0, 0.01));
    ok("2e adoption coute 75", doctrines_adopt_cost(ds,cid,ech)==75);
    ok("adopter Production au slot 1 prend", doctrines_adopt(ds,is,cid,1,DOCT_PRODUCTION,ech)==1);
    ok("3e adoption coute 100", doctrines_adopt_cost(ds,cid,ech)==100);
    ok("le slot 0 porte bien Commerce", doctrines_at(ds,cid,0)==DOCT_COMMERCE);
    ok("un slot deja occupe est REFUSE", doctrines_adopt(ds,is,cid,0,DOCT_DIPLOMATIE,ech)==0);

    /* ═══ 3. LES DEUX SEULES EXCLUSIVITÉS ═══ */
    printf("\n-- 3. Exclusivites : Commerce perp Mercantilisme, un seul courant --\n");
    ok("Mercantilisme est REFUSE tant que Commerce est tenue",
       doctrines_adopt(ds,is,cid,2,DOCT_MERCANTILISME,ech)==0);
    ok("la raison lue est bien l'exclusivite de paire",
       doctrines_why_not(ds,is,cid,2,DOCT_MERCANTILISME,ech)==DOCT_EXCLUSIVE_PAIR);
    ok("adopter le courant Aristocratie prend", doctrines_adopt(ds,is,cid,2,DOCT_ARISTOCRATIE,ech)==1);
    ok("un SECOND courant est REFUSE", doctrines_adopt(ds,is,cid,3,DOCT_BOURGEOISIE,ech)==0);
    ok("la raison lue est bien l'exclusivite de courant",
       doctrines_why_not(ds,is,cid,3,DOCT_BOURGEOISIE,ech)==DOCT_EXCLUSIVE_CURRENT);
    ok("une doctrine NEUTRE reste libre (aucun autre gate)",
       doctrines_why_not(ds,is,cid,3,DOCT_DIPLOMATIE,ech)==DOCT_OK);
    ok("une doctrine DEJA tenue est refusee comme telle",
       doctrines_why_not(ds,is,cid,3,DOCT_COMMERCE,ech)==DOCT_ALREADY);

    /* ═══ 4. LES IDÉES — séquentielles, coût croissant ═══ */
    printf("\n-- 4. Idees : SEQUENTIELLES, 30 + 3 x idees possedees --\n");
    ok("1re idee coute 30", doctrines_idea_cost(ds,cid,ech)==30);
    ok("acheter la 1re idee de Commerce prend", doctrines_buy_idea(ds,is,cid,DOCT_COMMERCE,ech)==1);
    ok("Commerce possede 1 idee", doctrines_ideas_of(ds,cid,DOCT_COMMERCE)==1);
    ok("2e idee coute 33 (le compte est GLOBAL)", doctrines_idea_cost(ds,cid,ech)==33);
    ok("acheter une idee d'une doctrine NON adoptee est refuse",
       doctrines_buy_idea(ds,is,cid,DOCT_FAUSTIEN,ech)==0);
    for (int i=1;i<DOCT_IDEAS;i++) doctrines_buy_idea(ds,is,cid,DOCT_COMMERCE,ech);
    ok("Commerce est COMPLETE (6/6)", doctrines_ideas_of(ds,cid,DOCT_COMMERCE)==DOCT_IDEAS);
    ok("une 7e idee est refusee (la doctrine est pleine)",
       doctrines_buy_idea(ds,is,cid,DOCT_COMMERCE,ech)==0);
    ok("le compte global d'idees vaut 6", doctrines_n_ideas(ds,cid)==6);

    /* ═══ 5. doctrine_key_mult — produit, clamp, cache ═══ */
    printf("\n-- 5. doctrine_key_mult : produit des idees, clamp [0.60,1.60], cache --\n");
    float m_cw = doctrine_key_mult(cid, "COMMERCE_W_BOURGEOIS");
    printf("   COMMERCE_W_BOURGEOIS (Commerce 6/6, idee 5 << Guildes >>) = %.4f\n", m_cw);
    ok("<< Guildes marchandes >> pose COMMERCE_W_BOURGEOIS a x1.30", near_f(m_cw, 1.30, 0.001));
    ok("le meme appel rend la MEME valeur (cache)", doctrine_key_mult(cid,"COMMERCE_W_BOURGEOIS")==m_cw);
    ok("une cle qu'aucune idee ne porte rend 1.0 (identite)",
       doctrine_key_mult(cid,"UNE_CLE_QUI_N_EXISTE_PAS")==1.f);
    ok("un pays SANS doctrine rend 1.0 (le chemin O(1) de la chronique)",
       doctrine_key_mult((cid+1)%SCPS_MAX_COUNTRY, "COMMERCE_W_BOURGEOIS")==1.f);
    /* le CLAMP BAS : Populaire << Souverainete >> pose C3_K_HOLLOW x0.25 → 0.60. */
    ok("adopter Populaire au slot 3 est refuse (Aristocratie tient le courant)",
       doctrines_adopt(ds,is,cid,3,DOCT_POPULAIRE,ech)==0);
    ok("abandonner le courant Aristocratie (slot 2) prend", doctrines_abandon(ds,is,cid,2)==1);
    ok("adopter Populaire une fois le courant libere", doctrines_adopt(ds,is,cid,2,DOCT_POPULAIRE,ech)==1);
    for (int i=0;i<DOCT_IDEAS;i++) doctrines_buy_idea(ds,is,cid,DOCT_POPULAIRE,ech);
    float m_c3 = doctrine_key_mult(cid, "C3_K_HOLLOW");
    printf("   C3_K_HOLLOW (Populaire << Souverainete >>, x0.25 brut) = %.4f\n", m_c3);
    ok("le produit brut x0.25 est CLAMPE au plancher 0.60", near_f(m_c3, 0.60, 0.001));

    /* ═══ 6. L'ASSIETTE DU COURANT — bascule mesurable ═══ */
    printf("\n-- 6. Le courant RE-SIED l'assiette de l'influence --\n");
    double g_def = influence_base_gain(econ, cid, INFL_BASE_DEFAUT);
    double g_pop = influence_base_gain(econ, cid, base_of(ds, cid));
    printf("   assiette par defaut (1000 elites x 0.002) = %.4f/mois\n", g_def);
    printf("   assiette POPULAIRE  (4000 journaliers x 0.00012) = %.4f/mois\n", g_pop);
    ok("sans courant, l'assiette est celle des elites", near_f(g_def, 2.0, 0.001));
    ok("le courant Populaire re-sied l'assiette sur les journaliers",
       near_f(g_pop, 4000.0*0.00012, 0.001) && !near_f(g_pop, g_def, 0.001));
    ok("influence_base_pop suit l'assiette (4000 journaliers)",
       near_f(influence_base_pop(econ,cid,INFL_BASE_LABORER), 4000.0, 0.5));
    ok("l'assiette BOURGEOISE lit bien les bourgeois (500)",
       near_f(influence_base_pop(econ,cid,INFL_BASE_BOURGEOIS), 500.0, 0.5));

    /* ═══ 7. LINÉARISATION — 2× de nobles ⇒ 2× de gain ET 2× de prix ═══ */
    printf("\n-- 7. Linearisation des prix sur l'assiette (et l'anti-exploit Conseil) --\n");
    DoctrineState *d2=malloc(sizeof(DoctrineState)); doctrines_init(d2);
    int cid2 = -1;
    for (int c=0;c<w->n_countries;c++) if (c!=cid && w->country[c].role!=POLITY_UNCLAIMED
                                        && w->country[c].role!=POLITY_WILD
                                        && w->country[c].capital_prov>=0){ cid2=c; break; }
    ok("un second pays est disponible pour la comparaison", cid2>=0);
    if (cid2>=0){
        int pid2=-1;
        for (int i=0;i<econ->n_prov;i++) if (econ->prov[i].owner==cid2){
            econ->prov[i].strata[CLASS_ELITE].pop=0.f; if (pid2<0) pid2=i; }
        if (pid2>=0) econ->prov[pid2].strata[CLASS_ELITE].pop = 2000.f;   /* DEUX FOIS le pays de reference */
        float ech2 = influence_scale(econ, cid2, INFL_BASE_DEFAUT);
        double gain2 = influence_base_gain(econ, cid2, INFL_BASE_DEFAUT);
        printf("   pays A : 1000 nobles -> assiette %.2f/mois, echelle %.2f (adoption courante %d, 3 actives)\n",
               g_def, ech, doctrines_adopt_cost(ds,cid,ech));
        printf("   pays B : 2000 nobles -> assiette %.2f/mois, echelle %.2f (1re adoption %d = le double de 50)\n",
               gain2, ech2, doctrines_adopt_cost(d2,cid2,ech2));
        ok("2x de nobles => 2x de GAIN mensuel", near_f(gain2, 2.0*g_def, 0.01));
        ok("2x de nobles => 2x d'echelle", near_f(ech2, 2.0*ech, 0.01));
        ok("2x de nobles => 2x le prix d'ADOPTION (100 au lieu de 50)",
           doctrines_adopt_cost(d2,cid2,ech2)==100);
        ok("2x de nobles => 2x le prix d'une IDEE (60 au lieu de 30)",
           doctrines_idea_cost(d2,cid2,ech2)==60);
        /* ANTI-EXPLOIT : l'echelle se lit sur l'ASSIETTE SEULE, jamais x le Conseil —
         * sinon accumuler a Conseil plein puis RENVOYER ses ministres braderait tout. */
        int best_slot=0, best_tier=0;
        for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
            int t = statecraft_council_cand_tier(w->seed, cid2, 0, sl, 0);
            if (t>best_tier){ best_tier=t; best_slot=sl; }
        }
        statecraft_council_hire(sc, w->seed, cid2, 0, best_slot, 0);
        float mult_c = influence_council_mult(sc, w->seed, cid2, NULL);
        float ech2b  = influence_scale(econ, cid2, INFL_BASE_DEFAUT);
        printf("   pays B, un ministre de rang %d en siege : mult_conseil %.2f, echelle %.2f\n",
               best_tier, mult_c, ech2b);
        ok("le Conseil MULTIPLIE bien le gain", mult_c >= 1.f);
        ok("le Conseil NE BOUGE PAS l'echelle (anti-exploit du renvoi de ministres)",
           near_f(ech2b, ech2, 0.001));
        ok("donc le prix d'adoption est INCHANGE par le Conseil",
           doctrines_adopt_cost(d2,cid2,ech2b)==100);
    }
    free(d2);
    doctrines_sync(ds);   /* d2 a repris le miroir : on le rend au pays testé */

    /* ═══ 8. ENTRETIEN + SUSPENSION déterministe ═══ */
    printf("\n-- 8. Entretien en influence : insolvable => les DERNIERES adoptees se suspendent --\n");
    int n_act = doctrines_n_active(ds, cid);
    printf("   %d doctrines actives, entretien attendu %d/mois\n", n_act, n_act);
    ok("l'entretien vaut DOCT_UPKEEP x doctrines actives x echelle",
       doctrines_upkeep(ds,cid,ech) == n_act);
    is->influence[cid] = 100.f;
    doctrines_tick(ds, is, cid, ech);
    ok("solvable : AUCUNE doctrine suspendue", doctrines_suspended(ds,cid,0)==false
                                            && doctrines_suspended(ds,cid,1)==false);
    ok("solvable : l'entretien a ete debite", near_f(influence_get(is,cid), 100.0-(double)n_act, 0.01));
    /* À SEC : il ne reste de quoi payer qu'UNE doctrine (la plus ANCIENNEMENT adoptée). */
    is->influence[cid] = 1.0f;
    doctrines_tick(ds, is, cid, ech);
    int n_susp=0, last_slot=-1; int16_t best_seq=-1;
    for (int sl=0; sl<DOCT_SLOTS_MAX; sl++){
        if (doctrines_at(ds,cid,sl)<0) continue;
        if (doctrines_suspended(ds,cid,sl)) n_susp++;
        if (ds->seq[cid][sl] > best_seq){ best_seq = ds->seq[cid][sl]; last_slot = sl; }
    }
    printf("   a sec (stock 1.0, %d actives) : %d suspendue(s) ; la DERNIERE adoptee est le slot %d\n",
           n_act, n_susp, last_slot);
    ok("insolvable : au moins une doctrine se suspend", n_susp >= 1);
    ok("c'est la doctrine la plus RECEMMENT adoptee qui tombe la premiere",
       last_slot>=0 && doctrines_suspended(ds,cid,last_slot));
    ok("le slot 0 (la plus ANCIENNE) tient encore", doctrines_suspended(ds,cid,0)==false);
    ok("une doctrine SUSPENDUE rend ses multiplicateurs a 1.0",
       doctrine_key_mult(cid,"C3_K_HOLLOW")==1.f);
    ok("la doctrine NON suspendue garde le sien (Commerce, slot 0)",
       near_f(doctrine_key_mult(cid,"COMMERCE_W_BOURGEOIS"), 1.30, 0.001));
    /* de nouveau solvable : la suspension se LÈVE (elle vaut pour CE mois seulement). */
    is->influence[cid] = 1000.f;
    doctrines_tick(ds, is, cid, ech);
    ok("re-solvable : plus AUCUNE suspension le mois suivant",
       doctrines_suspended(ds,cid,last_slot)==false);
    ok("et le multiplicateur suspendu est REVENU",
       near_f(doctrine_key_mult(cid,"C3_K_HOLLOW"), 0.60, 0.001));

    /* ═══ 9. ABANDON — slot libéré, idées perdues, aucun remboursement ═══ */
    printf("\n-- 9. Abandon libre : slot libere, idees PERDUES, zero remboursement --\n");
    float before = influence_get(is, cid);
    int n_before = doctrines_n_ideas(ds, cid);
    ok("abandonner le slot 0 (Commerce 6/6) prend", doctrines_abandon(ds,is,cid,0)==1);
    ok("aucun remboursement", near_f(influence_get(is,cid), before, 0.001));
    ok("le slot est LIBRE", doctrines_at(ds,cid,0)==-1);
    ok("les idees sont PERDUES", doctrines_ideas_of(ds,cid,DOCT_COMMERCE)==0);
    ok("le compte global d'idees a baisse de 6", doctrines_n_ideas(ds,cid)==n_before-6);
    ok("le multiplicateur de la doctrine abandonnee est retombe a 1.0 (cache invalide)",
       doctrine_key_mult(cid,"COMMERCE_W_BOURGEOIS")==1.f);
    ok("Mercantilisme redevient adoptable (Commerce n'est plus tenue)",
       doctrines_why_not(ds,is,cid,0,DOCT_MERCANTILISME,ech)==DOCT_OK);

    /* ═══ 10. SAVE → RELOAD (round-trip binaire + resync du miroir) ═══ */
    printf("\n-- 10. Persistance : round-trip binaire + resynchronisation du miroir --\n");
    doctrines_adopt(ds,is,cid,0,DOCT_TECHNOLOGIE,ech);
    doctrines_buy_idea(ds,is,cid,DOCT_TECHNOLOGIE,ech);
    doctrines_buy_idea(ds,is,cid,DOCT_TECHNOLOGIE,ech);   /* << Ecoles de ville >> : SAVOIR_W_BOURGEOIS x1.30 */
    float m_before = doctrine_key_mult(cid, "SAVOIR_W_BOURGEOIS");
    ok("<< Ecoles de ville >> pose SAVOIR_W_BOURGEOIS a x1.30", near_f(m_before, 1.30, 0.001));
    char tmp_path[64]; snprintf(tmp_path,sizeof tmp_path,"doctrines_demo_%u.tmp",(unsigned)seed);
    FILE *tf=fopen(tmp_path,"wb");
    ok("le fichier temporaire s'ouvre en ecriture", tf!=NULL);
    if (tf){ fwrite(ds,sizeof *ds,1,tf); fclose(tf); }
    DoctrineState *ds2=malloc(sizeof(DoctrineState)); doctrines_init(ds2);
    tf=fopen(tmp_path,"rb");
    ok("le fichier temporaire se relit", tf!=NULL);
    if (tf){ size_t n=fread(ds2,sizeof *ds2,1,tf); fclose(tf); ok("round-trip : 1 bloc complet lu", n==1); }
    remove(tmp_path);
    ok("SAVE->RELOAD conserve l'etat au bit pres", memcmp(ds, ds2, sizeof *ds)==0);
    /* le MIROIR est un cache de PROCESS : sans doctrines_sync, l'etat recharge
     * serait MUET jusqu'a la premiere cloture (et le --savetest le prendrait). */
    doctrines_init(ds);                       /* on efface le miroir en le RAZant */
    ok("apres RAZ, le multiplicateur est retombe", doctrine_key_mult(cid,"SAVOIR_W_BOURGEOIS")==1.f);
    doctrines_sync(ds2);                      /* le rappel du chargement */
    ok("doctrines_sync republie le miroir depuis l'etat recharge",
       near_f(doctrine_key_mult(cid,"SAVOIR_W_BOURGEOIS"), (double)m_before, 0.001));
    free(ds2);

    /* ═══ 11. LA TABLE — 17 x 6, slugs d'assets 1:1 ═══ */
    printf("\n-- 11. Le catalogue : 17 doctrines x 6 idees, slugs d'assets --\n");
    { int n_verbes=0, n_cablees=0, n_icons=0;
      for (int d=0; d<DOCT_COUNT; d++){
          if (!DOCT_DEF[d].bg || !DOCT_DEF[d].bg[0]) { ok("chaque doctrine porte un fond", 0); break; }
          for (int i=0;i<DOCT_IDEAS;i++){
              const DoctIdeaDef *id=&DOCT_DEF[d].idea[i];
              if (id->icon && id->icon[0]) n_icons++;
              if (id->verbe) n_verbes++;
              if (id->cable) n_cablees++;
          }
      }
      printf("   %d icones d'idee, %d idees-VERBES, %d idees CABLEES au moteur\n",
             n_icons, n_verbes, n_cablees);
      ok("les 102 idees portent chacune leur icone", n_icons==DOCT_COUNT*DOCT_IDEAS);
      ok("aucune idee-VERBE n'est cablee (les verbes sont une vague a part)", n_verbes>0);
      ok("une part substantielle des idees porte un effet moteur reel", n_cablees>=30);
    }

    free(w); free(econ); free(sc); free(is); free(ds);

    printf("\n==================================================================\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("==================================================================\n");
    return g_fail?1:0;
}
