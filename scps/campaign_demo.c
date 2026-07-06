/*
 * campaign_demo.c — les armées SUR LA CARTE : marche, siège, bataille de rencontre
 *
 *   make campaign_demo && ./campaign_demo [graine]
 *
 * Prouve que les primitives combat-dans-le-temps VIVENT sur une vraie carte :
 *   1. Une force expéditionnaire part de la frontière et MARCHE vers une région
 *      ennemie (le terrain décide des jours — §1) ; la marche use (attrition).
 *   2. En arrivant en terre ennemie, elle ASSIÈGE (≤ 2 ans selon fortif/vivres/
 *      terrain) ; le siège abouti RÉDUIT la région (enregistré).
 *   3. Quand une armée hostile défend la place, il y a BATAILLE (§2/§3) avant.
 *   4. NON-INVASIF : la propriété econ de la région ne bouge pas (la conquête
 *      abstraite — prix/volume — reste intacte).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_diplo.h"
#include "scps_army.h"
#include "scps_labor.h"
#include "scps_campaign.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

/* Un détachement de campagne, monté à la main (déterministe). */
static ArmyState make_force(long pik,long epe,long cav){
    ArmyState a; army_init(&a); a.n_units=0;
    if(pik>0){ a.units[a.n_units].type=U_PIQUIER;    a.units[a.n_units].count=pik; a.n_units++; }
    if(epe>0){ a.units[a.n_units].type=U_EPEISTE;    a.units[a.n_units].count=epe; a.n_units++; }
    if(cav>0){ a.units[a.n_units].type=U_CAV_LEGERE; a.units[a.n_units].count=cav; a.n_units++; }
    return a;
}

int main(int argc,char**argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    Campaign*camp=malloc(sizeof(Campaign)), *camp2=malloc(sizeof(Campaign));
    if(!w||!econ||!camp||!camp2){ fprintf(stderr,"OOM\n"); free(w);free(econ);free(camp);free(camp2); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LA CAMPAGNE — les armées marchent, assiègent, livrent bataille (graine %u)\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    for(int t=0;t<12;t++) econ_tick(econ,1.f);    /* peuple les régions, les fait croître/se toucher */

    /* On cherche une frontière : région A adjacente à une région B d'un autre pays. */
    int frontier=-1, target=-1, A=-1, B=-1;
    for(int r=0;r<econ->n_regions && frontier<0;r++){
        int oa=econ->region[r].owner;
        if(oa<0 || !econ->region[r].colonized) continue;
        for(int s=0;s<econ->n_regions;s++){
            if(!econ->adj[r][s]) continue;
            int ob=econ->region[s].owner;
            if(ob<0 || ob==oa || !econ->region[s].colonized) continue;
            frontier=r; target=s; A=oa; B=ob; break;
        }
    }
    if(frontier<0){
        printf("\n (pas de frontière terrestre entre deux pays sur cette graine — démo neutre)\n");
        ok("(monde trop fragmenté : pas de front à éprouver)", true);
        goto done;
    }
    printf("   front : pays %d (région %d) ⇆ pays %d (région %d, biome représentatif éprouvé)\n",
           A, frontier, B, target);

    DiploState dp; diplo_init(&dp);
    diplo_declare_war(&dp, A, B);
    int owner_before = econ->region[target].owner;

    /* ═══ 1. ORDRE & MARCHE ═══════════════════════════════════════════ */
    printf("\n── 1. La force part de la frontière et MARCHE vers la région ennemie ──\n");
    campaign_init(camp, w, econ);
    ArmyState force = make_force(20,15,8);            /* 43 paquets : piquiers, épéistes, cavalerie */
    ArmyState empty; army_init(&empty); empty.n_units=0;
    ok("une force VIDE ne peut être ordonnée (ce n'est pas un clic)",
       !campaign_order(camp, econ, A, frontier, target, &empty));
    bool ordered = campaign_order(camp, econ, A, frontier, target, &force);
    ok("l'ordre est accepté : la force part vers la région ennemie (itinéraire trouvé)", ordered);
    ok("la force est ACTIVE, posée sur la frontière, EN MARCHE",
       campaign_active(camp,A) && campaign_location(camp,A)==frontier && campaign_phase(camp,A)==FA_MARCH);
    long u0 = campaign_units(camp,A);

    /* ═══ 2. ARRIVÉE → SIÈGE → RÉDUCTION ══════════════════════════════ */
    printf("\n── 2. Arrivée en terre ennemie → SIÈGE (dans le temps) → région réduite ──\n");
    int arrived=0, besieging=0; float siege_len=0.f;
    for(int it=0; it<800 && campaign_taken(camp,A)==0; it++){
        uint32_t rng = 0x51u + (uint32_t)it*2654435761u;
        campaign_tick(camp, w, econ, &dp, &rng, 5.f);
        if(campaign_location(camp,A)==target) arrived=1;
        if(campaign_phase(camp,A)==FA_SIEGE){ besieging=1; if(siege_len<=0.f) siege_len=camp->army[A].days_left; }
    }
    printf("   étapes franchies %d · arrivée %s · assiégé %s · siège initial ≈ %.0f j · réduite %d\n",
           camp->army[A].legs, arrived?"oui":"non", besieging?"oui":"non", siege_len, campaign_taken(camp,A));
    ok("la force a MARCHÉ jusqu'à la région-but (terrain → jours, §1)", arrived && camp->army[A].legs>=1);
    ok("arrivée en terre ennemie colonisée, elle ASSIÈGE plus longtemps que 14 j", besieging && siege_len>14.f);
    ok("le siège abouti RÉDUIT la région (taken=1, puis au repos)",
       campaign_taken(camp,A)==1 && campaign_phase(camp,A)==FA_IDLE);
    ok("la marche n'AUGMENTE jamais les effectifs (l'attrition peut mordre)", campaign_units(camp,A) <= u0);
    ok("NON-INVASIF : econ inchangé — la propriété de la région-but n'a pas bougé",
       econ->region[target].owner == owner_before);

    /* ═══ 3. BATAILLE DE RENCONTRE : un défenseur conteste la place ════ */
    printf("\n── 3. Quand une armée hostile défend la place, il y a BATAILLE (§2/§3) ──\n");
    campaign_init(camp2, w, econ);
    ArmyState defender = make_force(10,8,3);          /* la garnison de campagne de B, sur sa région */
    ArmyState invader  = make_force(22,16,9);         /* l'assaillant de A, un peu plus fort */
    bool dord = campaign_order(camp2, econ, B, target, target, &defender);  /* B se tient sur target */
    bool aord = campaign_order(camp2, econ, A, frontier, target, &invader);
    ok("le défenseur se tient sur sa région ; l'assaillant marche dessus", dord && aord);
    int fought=0;
    for(int it=0; it<800 && !fought; it++){
        uint32_t rng = 0x9e3779b9u + (uint32_t)it*40503u;
        campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
        if(camp2->army[A].battles>0 || camp2->army[B].battles>0) fought=1;
    }
    printf("   batailles livrées : assaillant %d · défenseur %d\n",
           camp2->army[A].battles, camp2->army[B].battles);
    ok("les deux forces se sont CROISÉES sur la région et ont LIVRÉ BATAILLE", fought);

    /* ═══ 3b. L1 — L'INTERCEPTION : l'assiégeant se fait surprendre ═════ */
    printf("\n── 3b. L1 : un assiégeant se fait INTERCEPTER par le défenseur ──\n");
    campaign_init(camp2, w, econ);
    /* LOT 1 — campaign_order TRANSFÈRE désormais `src_force` (vidé au succès) : on
     * refait des forces fraîches pour cette 2e manche (invader/defender ont déjà été
     * consommées §3 ci-dessus). */
    ArmyState invader2  = make_force(22,16,9);
    ArmyState defender2 = make_force(10,8,3);
    campaign_order(camp2, econ, A, frontier, target, &invader2);     /* A marche, puis ASSIÈGE */
    { int guard=0;
      while (campaign_phase(camp2,A)!=FA_SIEGE && ++guard<800){
          uint32_t rng=0xC0FFEEu+(uint32_t)guard*2654435761u;
          campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
      } }
    ok("l'assaillant est EN SIÈGE sur la région du défenseur (interceptable)",
       campaign_phase(camp2,A)==FA_SIEGE && camp2->army[A].loc==target);
    /* la garnison de B fait une SORTIE sur sa propre place assiégée (le verbe L1). */
    bool sortie = campaign_order(camp2, econ, B, target, target, &defender2);
    int intercepted=0;
    for (int it=0; it<200 && !intercepted; it++){
        uint32_t rng=0xBADBEEFu+(uint32_t)it*40503u;
        campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
        if (camp2->army[B].battles>0) intercepted=1;
    }
    ok("le défenseur INTERCEPTE l'assiégeant : la bataille s'engage sous les murs",
       sortie && intercepted && camp2->n_battles>0);

    /* ═══ 3c. L2 — LE RALLIEMENT : l'armée brisée se reforme ════════════ */
    printf("\n── 3c. L2 : une armée en déroute se REFORME (40-60 %%, 30-60 j, une fois/guerre) ──\n");
    campaign_init(camp2, w, econ);
    { ArmyState david   = make_force(6,4,1);          /* le petit qui va rompre */
      ArmyState goliath = make_force(40,25,12);       /* l'écrasant */
      long u_pre_A=40+25+12, u_pre_B=6+4+1;
      campaign_order(camp2, econ, B, target, target, &david);
      campaign_order(camp2, econ, A, frontier, target, &goliath);
      /* le banc prouve LE RALLIEMENT, pas le vainqueur (l'équilibre du choc = L3) :
       * on suit CELUI QUI ROMPT, quel que soit le camp. */
      long after_rout=-1; int RT=-1, guard=0;
      while (camp2->n_rallies==0 && ++guard<600){      /* déroute PUIS ralliement (30-60 j après) */
          uint32_t rng=0xFEEDFACEu+(uint32_t)guard*40503u;
          campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
          if (camp2->n_routs>0){
              if (RT<0) RT = camp2->army[A].rally_used ? A : B;
              long now = campaign_units(camp2,RT);   /* le NOYAU = le CREUX réel (curée finie),
                                                        pas le 1er tick : la curée court encore
                                                        quelques pas avant que le ralliement parte. */
              if (after_rout<0 || now<after_rout) after_rout = now;
          }
      }
      long u_pre = (RT==A)? u_pre_A : u_pre_B;
      ok("une DÉROUTE survient, et le ralliement se CONSOMME (une fois/guerre)",
         camp2->n_routs>0 && camp2->n_rallies>0 && RT>=0 && camp2->army[RT].rally_used);
      ok("le NOYAU survit à la curée (l'armée ne s'évapore plus)", after_rout>=1);
      long reformed=(RT>=0)?campaign_units(camp2,RT):0;
      printf("   le rompu : avant %ld → après curée %ld → reformé %ld (cap 60 %% de l'avant-déroute)\n",
             u_pre, after_rout, reformed);
      ok("la re-formation TOMBE (ralliement compté, brisure levée)",
         camp2->n_rallies>0 && RT>=0 && camp2->army[RT].broken_days==0);
      /* P3 — curée ALLÉGÉE : le noyau survivant peut DÉPASSER 60 % de l'avant-déroute.
       * Le ralliement ne RÉDUIT jamais (≥ noyau) et ne POUSSE pas au-dessus de 60 % ;
       * quand le noyau prime déjà, c'est lui le plancher effectif. */
      long cap60=(long)(0.6f*(float)u_pre)+1, cap_eff=(after_rout>cap60)?after_rout:cap60;
      ok("le CAP est respecté : reformé ≥ le noyau, et le ralliement ne pousse pas au-dessus de 60 %",
         RT>=0 && reformed>=after_rout && reformed<=cap_eff);
    }

    /* ═══ 3d. LOT 3 — LE SIÈGE LIT LA GARNISON (H_coerc) ═══════════════ */
    printf("\n── 3d. LOT 3 : une garnison BÂTIE (H_coerc) allonge le siège (modéré, jamais l'immortalité) ──\n");
    {
        /* region_defense() est static (scps_campaign.c) : on l'éprouve PAR LE SIÈGE
         * réel, sur une cible qu'on CONTRÔLE entièrement (pop nulle → pas de cap_def
         * de capitale qui sature le plafond 2 ans, cf. le piège mesuré sur `target`
         * du monde généré : cap_def d'une grosse capitale y sature souvent 727-730j
         * à lui seul, masquant tout delta H_coerc). On choisit une région LIBRE
         * (non colonisée) du monde, on la peuple/colonise à la main avec UN SEUL
         * bâtiment (n_bld=1, pop modeste), pour isoler H_coerc. */
        int rfree=-1;
        for (int r=0;r<econ->n_regions;r++)
            if (econ->region[r].owner<0 && !econ->region[r].impassable && econ->adj[frontier][r]){ rfree=r; break; }
        if (rfree<0)
            for (int r=0;r<econ->n_regions;r++)
                if (!econ->region[r].impassable && r!=frontier){ rfree=r; break; }
        if (rfree<0 || !econ->adj[frontier][rfree]){
            ok("(aucune région voisine de la frontière disponible pour ce test isolé sur cette graine)", true);
        } else {
            econ->region[rfree].owner=B; econ->region[rfree].colonized=true;
            econ->region[rfree].n_bld=1;
            econ->region[rfree].strata[CLASS_LABORER].pop=50.f;
            econ->region[rfree].strata[CLASS_BOURGEOIS].pop=0.f;
            econ->region[rfree].strata[CLASS_ELITE].pop=0.f;
            econ->region[rfree].food_sat=0.5f;
            econ->region[rfree].build.H_coerc=0.f;

            campaign_init(camp2, w, econ);
            ArmyState atk1 = make_force(22,16,9);
            campaign_order(camp2, econ, A, frontier, rfree, &atk1);
            float siege0=0.f;
            { int guard=0;
              while (campaign_phase(camp2,A)!=FA_SIEGE && ++guard<800){
                  uint32_t rng=0x1234u+(uint32_t)guard*2654435761u;
                  campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
              }
              if (campaign_phase(camp2,A)==FA_SIEGE) siege0=camp2->army[A].days_left;
            }
            /* MÊME cible, GARNISON BÂTIE en plus (H_coerc, comme une Forteresse) —
             * rien d'autre ne bouge (même n_bld, même pop, même terrain). */
            econ->region[rfree].build.H_coerc=6.f;
            campaign_init(camp2, w, econ);
            ArmyState atk2 = make_force(22,16,9);
            campaign_order(camp2, econ, A, frontier, rfree, &atk2);
            float siege1=0.f;
            { int guard=0;
              while (campaign_phase(camp2,A)!=FA_SIEGE && ++guard<800){
                  uint32_t rng=0x1234u+(uint32_t)guard*2654435761u;
                  campaign_tick(camp2, w, econ, &dp, &rng, 5.f);
              }
              if (campaign_phase(camp2,A)==FA_SIEGE) siege1=camp2->army[A].days_left;
            }
            printf("   région témoin %d : H_coerc 0→6 · siège initial %.0f j → %.0f j (+%.0f%%)\n",
                   rfree, siege0, siege1, (siege0>0.f)?100.f*(siege1-siege0)/siege0:0.f);
            ok("une garnison BÂTIE (H_coerc) allonge RÉELLEMENT le siège (entrée moteur, pas un bonus plat)",
               siege0>0.f && siege1>siege0);
            ok("l'allongement reste MODÉRÉ (~20-40 %%), jamais l'immortalité (toujours < 2 ans plafond)",
               siege1 < 730.f && siege1 <= siege0*1.6f);
            /* on remet la région comme trouvée (les tests suivants ne doivent pas hériter d'un monde altéré) */
            econ->region[rfree].owner=-1; econ->region[rfree].colonized=false; econ->region[rfree].n_bld=0;
            econ->region[rfree].build.H_coerc=0.f;
            econ->region[rfree].strata[CLASS_LABORER].pop=0.f;
        }
    }

    /* ═══ 4. LECTEURS (membrane : tangibles) ═══════════════════════════ */
    printf("\n── 4. Lecteurs de campagne (mots & nombres tangibles, pour l'UI §4) ──\n");
    ok("les phases ont des noms diégétiques distincts (repos / marche / siège)",
       campaign_phase_name(FA_MARCH)[0] && campaign_phase_name(FA_SIEGE)[0] &&
       campaign_phase_name(FA_IDLE)[0]);
    /* composition par grand type d'arme (survol UI §4) + mot de taille (asymétrie). */
    campaign_init(camp, w, econ);
    ArmyState mix = make_force(20,15,8);              /* 20 inf, 8 cav ; +archers à la main */
    mix.units[mix.n_units].type=U_ARCHER; mix.units[mix.n_units].count=5; mix.n_units++;
    campaign_order(camp, econ, A, frontier, target, &mix);
    ArmyComposition cp = campaign_composition(camp, A);
    printf("   composition : %ld inf · %ld arch · %ld cav · %ld mages (total %ld régiments)\n",
           cp.infanterie, cp.archers, cp.cavalerie, cp.mages, cp.total);
    ok("la composition se range par grand type d'arme (inf 35, arch 5, cav 8)",
       cp.infanterie==35 && cp.archers==5 && cp.cavalerie==8 && cp.total==48);
    ok("l'effectif EXACT se lit (P1.10 : la somme des armes = le total, 4800 hommes)",
       cp.infanterie+cp.archers+cp.cavalerie+cp.mages==cp.total && cp.total*100==4800);

done:
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ); free(camp); free(camp2);
    return g_fail?1:0;
}
