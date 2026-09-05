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

    /* On cherche une frontière : région A adjacente à une région B d'un autre pays,
     * ET RÉELLEMENT MARCHABLE. L'adjacence éco (econ->adj) seule ne suffit plus : la
     * carte de CAMPAGNE (biome majoritaire par région, terrain_impassable) est PLUS
     * STRICTE — une paire adjacente au sens éco peut être injoignable pour une armée
     * (roche/glace/eau dominante) sous une graine donnée. On sonde donc la route RÉELLE
     * (campaign_order) sur `camp2`, un scratch jamais lu ici (re-initialisé avant son
     * usage réel en LOT 3 plus bas) — jamais `camp`, qui doit rester vierge pour la
     * vraie force de §1 (un campaign_order de sonde y fusionnerait un reliquat parasite
     * dans le compte de troupes). Même jurisprudence que LOT 3 (§3d, region_ok/next_hop
     * plus strict que le flag `impassable`), étendue ici à la sélection INITIALE. */
    campaign_init(camp2, w, econ);
    int frontier=-1, target=-1, A=-1, B=-1;
    for(int r=0;r<econ->n_regions && frontier<0;r++){
        int oa=econ->region[r].owner;
        if(oa<0 || !econ->region[r].colonized) continue;
        for(int s=0;s<econ->n_regions;s++){
            if(!econ->adj[r][s]) continue;
            int ob=econ->region[s].owner;
            if(ob<0 || ob==oa || !econ->region[s].colonized) continue;
            ArmyState probe=make_force(1,0,0);
            if(!campaign_order(camp2, econ, oa, r, s, &probe)) continue;   /* injoignable : paire suivante */
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

    /* ═══ 0. ARRIVÉE NEUTRE & FIN DE GUERRE ============================= */
    printf("\n── 0. Une région neutre n'est pas assiégée ; la paix arrête un siège ──\n");
    /* La frontière A/B éprouvée ci-dessus fournit déjà une cible marchable :
     * on la garde, mais sans déclarer la guerre, pour tester l'arrivée diplomatiquement
     * neutre. Il ne faut pas transformer l'absence d'une autre région neutre en succès. */
    static DiploState neutral_dp; diplo_init(&neutral_dp);
    campaign_init(camp2,w,econ);
    ArmyState neutral_force=make_force(3,0,0);
    bool neutral_order=campaign_order(camp2,econ,A,frontier,target,&neutral_force);
    int neutral_arrived=0;
    for(int it=0; neutral_order && it<800 && !neutral_arrived; it++){
        uint32_t rng=0xD00u + (uint32_t)it*2654435761u;
        campaign_tick(camp2,w,econ,&neutral_dp,&rng,5.f);
        if(campaign_location(camp2,A)==target) neutral_arrived=1;
    }
    ok("l'arrivée diplomatiquement neutre reste au repos, sans siège ni prise",
       neutral_order && neutral_arrived && campaign_phase(camp2,A)==FA_IDLE
       && campaign_taken(camp2,A)==0);

    campaign_init(camp2,w,econ);
    ArmyState neutral_home=make_force(1,0,0);
    bool neutral_same=campaign_order(camp2,econ,A,target,target,&neutral_home);
    uint32_t neutral_rng=0x4E55u;
    campaign_tick(camp2,w,econ,&neutral_dp,&neutral_rng,1.f);
    ok("un ordre neutre sur place reste immédiatement au repos",
       neutral_same && campaign_phase(camp2,A)==FA_IDLE
       && campaign_location(camp2,A)==target && campaign_taken(camp2,A)==0);

    /* Une cible ennemie déjà tenue par A ne doit pas être re-sieégée :
     * l'occupation est une sortie de siège, même si la guerre continue. */
    static DiploState held_dp; diplo_init(&held_dp); diplo_declare_war(&held_dp,A,B);
    bool held= diplo_occupy(&held_dp,econ,A,target);
    campaign_init(camp2,w,econ);
    ArmyState held_force=make_force(8,5,2);
    bool held_order=campaign_order(camp2,econ,A,frontier,target,&held_force);
    int held_arrived=0;
    for(int it=0; held_order && it<800 && !held_arrived; it++){
        uint32_t rng=0x0CCu + (uint32_t)it*65537u;
        campaign_tick(camp2,w,econ,&held_dp,&rng,5.f);
        held_arrived=(campaign_location(camp2,A)==target);
    }
    bool held_redirect=held_arrived && campaign_redirect_corps(camp2,econ,&held_dp,A,target);
    ok("une région ennemie déjà occupée par nous reste au repos à l'arrivée",
       held && held_order && held_arrived && campaign_phase(camp2,A)==FA_IDLE
       && campaign_taken(camp2,A)==0);
    ok("une redirection sur notre occupation ne recrée pas de siège",
       held_redirect && campaign_phase(camp2,A)==FA_IDLE
       && campaign_taken(camp2,A)==0);

    static DiploState peace_dp; diplo_init(&peace_dp); diplo_declare_war(&peace_dp,A,B);
    campaign_init(camp2,w,econ);
    ArmyState peace_force=make_force(22,16,9);
    bool peace_order=campaign_order(camp2,econ,A,frontier,target,&peace_force);
    int peace_siege=0;
    for(int it=0; peace_order && it<800 && !peace_siege; it++){
        uint32_t rng=0xBEEFu + (uint32_t)it*40503u;
        campaign_tick(camp2,w,econ,&peace_dp,&rng,5.f);
        peace_siege=campaign_phase(camp2,A)==FA_SIEGE;
    }
    diplo_make_peace(&peace_dp,A,B);
    uint32_t peace_rng=0xA11u;
    campaign_tick(camp2,w,econ,&peace_dp,&peace_rng,1.f);
    ok("la paix en plein siège annule la phase sans prise",
       peace_siege && diplo_status(&peace_dp,A,B)!=DIPLO_WAR
       && campaign_phase(camp2,A)==FA_IDLE && campaign_taken(camp2,A)==0);

    static DiploState war_dp; diplo_init(&war_dp); diplo_declare_war(&war_dp,A,B);
    campaign_init(camp2,w,econ);
    ArmyState war_force=make_force(22,16,9);
    bool war_order=campaign_order(camp2,econ,A,frontier,target,&war_force);
    int war_siege=0;
    for(int it=0; war_order && it<800 && !war_siege; it++){
        uint32_t rng=0xCAFEu + (uint32_t)it*40503u;
        campaign_tick(camp2,w,econ,&war_dp,&rng,5.f);
        war_siege=campaign_phase(camp2,A)==FA_SIEGE;
    }
    float war_left=camp2->army[A].days_left;
    uint32_t war_rng=0xA12u;
    campaign_tick(camp2,w,econ,&war_dp,&war_rng,1.f);
    ok("la guerre maintenue laisse le siège actif",
       war_siege && diplo_status(&war_dp,A,B)==DIPLO_WAR
       && campaign_phase(camp2,A)==FA_SIEGE && camp2->army[A].days_left<war_left);

    /* Fixture de libération réelle : la frontière appartient à A, mais B l'occupe.
     * Le corps A doit d'abord reprendre la place en guerre ; le règlement de
     * l'occupation reste ensuite une opération diplomatique publique. */
    static DiploState liberate_dp; diplo_init(&liberate_dp); diplo_declare_war(&liberate_dp,A,B);
    bool occupied=diplo_occupy(&liberate_dp,econ,B,frontier);
    campaign_init(camp2,w,econ);
    ArmyState liberate_force=make_force(20,15,8);
    bool liberate_order=campaign_order(camp2,econ,A,frontier,frontier,&liberate_force);
    bool liberate_siege=liberate_order
        && campaign_redirect_corps(camp2,econ,&liberate_dp,A,frontier)
        && campaign_phase(camp2,A)==FA_SIEGE;
    int liberate_taken=0;
    for(int it=0; liberate_siege && it<800 && !liberate_taken; it++){
        uint32_t rng=0x1BEEFu + (uint32_t)it*40503u;
        campaign_tick(camp2,w,econ,&liberate_dp,&rng,5.f);
        liberate_taken=campaign_taken(camp2,A)>0 && camp2->army[A].taken_region==frontier;
    }
    ok("en guerre, le corps arrivé sur notre terre occupée assiège l'occupant",
       occupied && liberate_siege);
    ok("le siège abouti marque la région à libérer",
       liberate_taken && camp2->army[A].taken_region==frontier);
    diplo_liberate(&liberate_dp,econ,frontier);
    ok("la libération nettoie l'occupation et conserve la guerre",
       liberate_taken && liberate_dp.occupier[frontier]<0
       && econ->region[frontier].owner==A && diplo_status(&liberate_dp,A,B)==DIPLO_WAR);

    static DiploState peace_occupied_dp; diplo_init(&peace_occupied_dp);
    diplo_declare_war(&peace_occupied_dp,A,B);
    bool peace_occupied=diplo_occupy(&peace_occupied_dp,econ,B,frontier);
    diplo_make_peace(&peace_occupied_dp,A,B);
    campaign_init(camp2,w,econ);
    ArmyState peace_home=make_force(20,15,8);
    bool peace_home_order=campaign_order(camp2,econ,A,frontier,frontier,&peace_home);
    bool peace_home_idle=peace_home_order
        && campaign_redirect_corps(camp2,econ,&peace_occupied_dp,A,frontier)
        && campaign_phase(camp2,A)==FA_IDLE;
    ok("en paix, la même terre reste au repos sans réouvrir de siège",
       peace_occupied && peace_home_idle && campaign_taken(camp2,A)==0);

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
    int route[SCPS_MAX_REG], preview_reason=-1, preview_arrival=-1;
    float preview_days=-1.f;
    int next_before=camp->army[A].next, dest_before=camp->army[A].dest;
    float left_before=camp->army[A].days_left;
    int route_n=campaign_preview_corps(camp,econ,&dp,A,target,route,SCPS_MAX_REG,
                                        &preview_days,&preview_reason,&preview_arrival);
    ok("l'aperçu expose la route complète, sa durée et l'issue SIÈGE avant le clic",
       route_n>=2 && route[0]==frontier && route[route_n-1]==target
       && preview_days>0.f && preview_reason==0 && preview_arrival==2);
    ok("l'aperçu est une LECTURE PURE : il ne modifie pas l'ordre en cours",
       camp->army[A].next==next_before && camp->army[A].dest==dest_before
       && camp->army[A].days_left==left_before);

    /* Le code d'issue est public : 1 = stationner en paix, 0 = déjà sur place,
     * 2 = siège réellement autorisé par la guerre. */
    static Campaign preview_camp; static DiploState preview_peace, preview_war;
    int preview_route[SCPS_MAX_REG], preview_n=0, preview_reason2=-1, preview_arrival2=-1;
    float preview_days2=0.f;
    diplo_init(&preview_peace);
    campaign_init(&preview_camp,w,econ);
    ArmyState preview_force=make_force(4,3,1);
    bool preview_order=campaign_order(&preview_camp,econ,A,frontier,target,&preview_force);
    preview_n=campaign_preview_corps(&preview_camp,econ,&preview_peace,A,target,
                                     preview_route,SCPS_MAX_REG,&preview_days2,
                                     &preview_reason2,&preview_arrival2);
    ok("l'aperçu d'une cible ennemie en paix annonce le stationnement (1)",
       preview_order && preview_n>=2 && preview_reason2==0 && preview_arrival2==1);

    campaign_init(&preview_camp,w,econ);
    ArmyState preview_home=make_force(4,3,1);
    bool preview_same=campaign_order(&preview_camp,econ,A,target,target,&preview_home);
    preview_reason2=-1; preview_arrival2=-1;
    preview_n=campaign_preview_corps(&preview_camp,econ,&preview_peace,A,target,
                                     preview_route,SCPS_MAX_REG,&preview_days2,
                                     &preview_reason2,&preview_arrival2);
    ok("l'aperçu sur la région courante annonce le stationnement (0)",
       preview_same && preview_n==1 && preview_reason2==0 && preview_arrival2==0);

    diplo_init(&preview_war); diplo_declare_war(&preview_war,A,B);
    campaign_init(&preview_camp,w,econ);
    ArmyState preview_attack=make_force(4,3,1);
    bool preview_war_order=campaign_order(&preview_camp,econ,A,frontier,target,&preview_attack);
    preview_reason2=-1; preview_arrival2=-1;
    preview_n=campaign_preview_corps(&preview_camp,econ,&preview_war,A,target,
                                     preview_route,SCPS_MAX_REG,&preview_days2,
                                     &preview_reason2,&preview_arrival2);
    ok("l'aperçu d'une cible ennemie en guerre annonce le siège (2)",
       preview_war_order && preview_n>=2 && preview_reason2==0 && preview_arrival2==2);
    long u0 = campaign_units(camp,A);

    /* ═══ 2. ARRIVÉE → SIÈGE → RÉDUCTION ══════════════════════════════ */
    printf("\n── 2. Arrivée en terre ennemie → SIÈGE (dans le temps) → région réduite ──\n");
    int arrived=0, besieging=0, siege_read_ok=0; float siege_len=0.f;
    CampaignSiegeFactors siege_read={0};
    for(int it=0; it<800 && campaign_taken(camp,A)==0; it++){
        uint32_t rng = 0x51u + (uint32_t)it*2654435761u;
        campaign_tick(camp, w, econ, &dp, &rng, 5.f);
        if(campaign_location(camp,A)==target) arrived=1;
        if(campaign_phase(camp,A)==FA_SIEGE){
            besieging=1; if(siege_len<=0.f) siege_len=camp->army[A].days_left;
            if(!siege_read_ok) siege_read_ok=campaign_siege_factors(camp,econ,target,&siege_read)?1:0;
        }
    }
    printf("   étapes franchies %d · arrivée %s · assiégé %s · siège initial ≈ %.0f j · réduite %d\n",
           camp->army[A].legs, arrived?"oui":"non", besieging?"oui":"non", siege_len, campaign_taken(camp,A));
    ok("la force a MARCHÉ jusqu'à la région-but (terrain → jours, §1)", arrived && camp->army[A].legs>=1);
    ok("arrivée en terre ennemie colonisée, elle ASSIÈGE plus longtemps que 14 j", besieging && siege_len>14.f);
    ok("le siège abouti RÉDUIT la région (taken=1, puis au repos)",
       campaign_taken(camp,A)==1 && campaign_phase(camp,A)==FA_IDLE);
    ok("le readout de siege expose restant, resistance, ouvrages, vivres et terrain",
       siege_read_ok && siege_read.valid && siege_read.owner==A
       && siege_read.days_left>0.f && siege_read.full_days>=siege_read.days_left
       && siege_read.defense_level>0.f && siege_read.food_months>=0.f
       && siege_read.terrain_pct>0 && siege_read.progress_pct>=0 && siege_read.progress_pct<=100);
    ok("la marche n'AUGMENTE jamais les effectifs (l'attrition peut mordre)", campaign_units(camp,A) <= u0);
    ok("NON-INVASIF : econ inchangé — la propriété de la région-but n'a pas bougé",
       econ->region[target].owner == owner_before);
    ok("le renfort est refusé hors du territoire national, sans consommer d'arsenal",
       !campaign_can_refill_corps(camp,econ,A) && campaign_refill_corps(camp,A,econ)==0);

    campaign_init(camp2,w,econ);
    ArmyState militia; army_init(&militia); militia.n_units=1;
    militia.units[0].type=U_MILICE; militia.units[0].count=1;
    bool militia_home=campaign_order(camp2,econ,A,frontier,frontier,&militia);
    long militia_before=campaign_corps_units(camp2,A);
    /* RENFORCER = COMBLER LE DÉFICIT : un corps frais est déjà À SON NOMINAL (posé à la
     * levée) — sans perte, le déficit est nul et le renfort est un no-op légitime. On
     * simule ici une perte passée (nominal > courant) pour éprouver le comblement. */
    ok("un corps FRAIS (nominal=courant) refuse le renfort : déficit nul",
       campaign_refill_corps(camp2,A,econ)==0 && campaign_corps_units(camp2,A)==militia_before);
    camp2->army[A].nominal += 1;
    int militia_added=campaign_refill_corps(camp2,A,econ);
    ok("sur sol national, la milice COMBLE son déficit (1 paquet) sans exiger d'armes manufacturées",
       militia_home && militia_added==1 && campaign_corps_units(camp2,A)==militia_before+1);

    /* ═══ 3. BATAILLE DE RENCONTRE : un défenseur conteste la place ════ */
    printf("\n── 3. Quand une armée hostile défend la place, il y a BATAILLE (§2/§3) ──\n");
    campaign_init(camp2, w, econ);
    ArmyState defender = make_force(10,8,3);          /* la garnison de campagne de B, sur sa région */
    ArmyState invader  = make_force(22,16,9);         /* l'assaillant de A, un peu plus fort */
    bool dord = campaign_order(camp2, econ, B, target, target, &defender);  /* B se tient sur target */
    bool aord = campaign_order(camp2, econ, A, frontier, target, &invader);
    ok("le défenseur se tient sur sa région ; l'assaillant marche dessus", dord && aord);
    int fought=0, tactical=0; CampaignBattleFactors factors={0};
    for(int it=0; it<800 && !tactical; it++){
        uint32_t rng = 0x9e3779b9u + (uint32_t)it*40503u;
        campaign_tick(camp2, w, econ, &dp, &rng, 1.f);
        if(camp2->army[A].battles>0 || camp2->army[B].battles>0) fought=1;
        tactical=campaign_battle_factors(camp2,econ,target,&factors)?1:0;
    }
    printf("   batailles livrées : assaillant %d · défenseur %d\n",
           camp2->army[A].battles, camp2->army[B].battles);
    ok("les deux forces se sont CROISÉES sur la région et ont LIVRÉ BATAILLE", fought);
    ok("la lecture tactique reprend le champ ACTIF sans le muter",
       tactical && factors.valid && factors.region==target
       && factors.owner_a>=0 && factors.owner_b>=0);
    ok("terrain, contres, rapport et rupture sont FINIS et bornés",
       factors.terrain_a>0.f && factors.counter_a>0.f && factors.counter_b>0.f
       && factors.power_a>0.f && factors.power_b>0.f
       && factors.balance_a_pct>=0 && factors.balance_a_pct<=100
       && factors.rupture_pct>=0 && factors.rupture_pct<=100);
    float loss_seen=0.f; int loss_monotonic=1;
    for(int it=0;it<20 && camp2->battle[0].active;it++){
        float before=camp2->battle[0].lossA+camp2->battle[0].lossB;
        uint32_t rng=0xA11CEu+(uint32_t)it*40503u;
        campaign_tick(camp2,w,econ,&dp,&rng,1.f);
        float after=camp2->battle[0].lossA+camp2->battle[0].lossB;
        if(after+1e-6f<before)loss_monotonic=0;
        if(after>loss_seen)loss_seen=after;
    }
    ok("les pertes de bataille sont maintenant CUMULÉES sans retomber au reliquat fractionnaire",
       loss_monotonic && loss_seen>=1.f);

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
        /* SONDE LA ROUTE RÉELLE (campaign_order) plutôt que le seul flag `impassable`
         * éco : la carte de CAMPAGNE (biome majoritaire par région, terrain_impassable)
         * est PLUS STRICTE — une région « libre » côté éco peut être un biome de combat
         * impraticable (roche/glace/eau) sous une graine donnée. Sans ce sondage, le
         * premier candidat économiquement libre peut être géographiquement injoignable
         * (campaign_order échoue en silence, next_hop<0) et le siège ne démarre jamais. */
        int rfree=-1;
        for (int r=0;r<econ->n_regions && rfree<0;r++){
            if (econ->region[r].owner>=0 || econ->region[r].impassable || !econ->adj[frontier][r]) continue;
            campaign_init(camp2, w, econ);
            ArmyState probe=make_force(1,0,0);
            if (campaign_order(camp2, econ, A, frontier, r, &probe)) rfree=r;
        }
        if (rfree<0)
            for (int r=0;r<econ->n_regions && rfree<0;r++){
                if (econ->region[r].impassable || r==frontier) continue;
                campaign_init(camp2, w, econ);
                ArmyState probe=make_force(1,0,0);
                if (campaign_order(camp2, econ, A, frontier, r, &probe)) rfree=r;
            }
        if (rfree<0){
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

    /* ═══ 5. CORPS MULTIPLES : identité, scission/fusion et front B2 ═══ */
    printf("\n── 5. Corps multiples : scinder · fusionner · combattre en stack ──\n");
    int second=campaign_split(camp,A,16);
    ok("SCINDER crée un second corps stable sans créer de régiments",
       second>=0 && campaign_corps_count(camp,A)==2
       && campaign_corps_units(camp,A)+campaign_corps_units(camp,second)==48);
    ok("FUSIONNER deux corps co-localisés rend le slot et conserve l'effectif",
       campaign_merge(camp,A,second) && campaign_corps_count(camp,A)==1
       && campaign_corps_units(camp,A)==48);

    campaign_init(camp2,w,econ);
    ArmyState poolA=make_force(20,0,0), poolB=make_force(15,0,0);
    int ca0=campaign_raise(camp2,econ,A,target,target,&poolA,10);
    int ca1=campaign_raise(camp2,econ,A,target,target,&poolA,10);
    int cb0=campaign_raise(camp2,econ,B,target,target,&poolB,15);
    uint32_t stack_rng=0xB2u;
    campaign_tick(camp2,w,econ,&dp,&stack_rng,1.f);
    ok("B2 engage UNE bataille et épingle TOUS les corps amis présents",
       ca0>=0 && ca1>=0 && cb0>=0 && camp2->n_battles==1
       && camp2->army[ca0].phase==FA_BATTLE && camp2->army[ca1].phase==FA_BATTLE);
    long stack_before=campaign_corps_units(camp2,ca0)+campaign_corps_units(camp2,ca1);
    for(int d=0;d<80 && camp2->battle[0].active;d++) campaign_tick(camp2,w,econ,&dp,&stack_rng,1.f);
    long stack_after=campaign_corps_units(camp2,ca0)+campaign_corps_units(camp2,ca1);
    ok("les pertes du front sont imputées aux corps réels (jamais à une copie agrégée)",
       stack_after<=stack_before && camp2->dead_choc+camp2->dead_pursuit>0);

done:
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(econ); free(camp); free(camp2);
    return g_fail?1:0;
}
