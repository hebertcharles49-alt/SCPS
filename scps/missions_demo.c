/*
 * missions_demo.c — LES DESSEINS (docs/DESIGN_MISSIONS_DOCTRINES.md §2)
 *
 *   make missions_demo && ./missions_demo [graine]
 *
 * Prouve : la branche du SOL se GÉNÈRE pour le joueur seul (et pas pour l'IA) ;
 * ses cibles se résolvent DÉTERMINISTIQUEMENT sur la géographie ; un échelon
 * rempli devient PRÊT et se SCELLE ; le scellage verse une récompense qui
 * n'invente pas d'or ; une remise DATÉE vit puis EXPIRE (le latch d'année) ;
 * une cible DÉTRUITE se re-résout, une cible passée à un tiers ne bouge PAS ;
 * le pivot exige sa preuve d'usage ; l'état survit à un aller-retour de save.
 */
#include "scps_missions.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_tune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, int cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World)); WorldEconomy *econ=malloc(sizeof(WorldEconomy));
    MissionsState *ms=malloc(sizeof(MissionsState));
    MissionsState *ms2=malloc(sizeof(MissionsState));
    DiploState *dp=malloc(sizeof(DiploState));
    Statecraft *sc=malloc(sizeof(Statecraft));
    if(!w||!econ||!ms||!ms2||!dp||!sc){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LES DESSEINS — branche du SOL : cibles, échelons, remises datées\n");
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    diplo_init(dp);
    statecraft_init(sc,w);
    missions_init(ms);

    /* Un pays réel avec une capitale ET des voisins (POLITY_WILD exclu : un hameau
     * n'a ni conseil ni diplomatie — la branche s'y résoudrait à vide). */
    int cid=-1;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
        if (w->country[c].capital_prov<0) continue;
        cid=c; break;
    }
    if (cid<0){ fprintf(stderr,"monde trop vide — autre graine\n"); return 1; }
    int cap = w->country[cid].capital_prov;

    /* ═══ 1. GÉNÉRATION : le Sol est TOUJOURS attribué… au JOUEUR SEUL ═══ */
    printf("\n── 1. La branche du Sol se génère — pour le joueur, et pour lui seul ──\n");
    ok("avant toute clôture, aucune branche n'existe", dessein_of(ms,cid,DESS_SOL)==NULL);
    missions_tick(ms, w, econ, dp, 0, -1);                 /* chronique : human = -1 */
    ok("human_player = -1 (chronique) → RIEN n'est généré (golden intact par construction)",
       dessein_of(ms,cid,DESS_SOL)==NULL);
    missions_tick(ms, w, econ, dp, 0, cid);
    const Dessein *d = dessein_of(ms,cid,DESS_SOL);
    ok("le joueur reçoit la branche du Sol (éligibilité : TOUJOURS)", d!=NULL);
    if (!d){ printf("branche absente — arrêt\n"); return 1; }
    ok("elle démarre à l'échelon Unification, sans voie", d->rung==DESS_RUNG_UNIFICATION
                                                       && d->voie==DESS_VOIE_AUCUNE);
    ok("sa première cible est la province-CAPITALE (grain province)",
       d->tpid[DESS_RUNG_UNIFICATION]==cap);

    /* la génération est IDEMPOTENTE : re-clôturer ne re-tire rien. */
    { int8_t r0=d->rung; int16_t t0=d->tpid[DESS_RUNG_UNIFICATION];
      missions_tick(ms, w, econ, dp, 1, cid);
      ok("une seconde clôture ne re-génère ni ne re-tire (idempotent)",
         d->rung==r0 && d->tpid[DESS_RUNG_UNIFICATION]==t0); }

    /* ═══ 2. LA CONDITION EST UN PRÉDICAT D'ÉTAT RÉEL ══════════════════ */
    printf("\n── 2. Unification : toute la vallée possédée ET colonisée ──\n");
    int reg = w->province[cap].region;
    const Region *rg = &w->region[reg];
    /* on DÉPOSSÈDE une province de la vallée : la condition doit tomber. */
    int victim=-1;
    for (int k=0;k<rg->n_provinces;k++){
        int pid=rg->province_ids[k];
        if (pid<0||pid>=econ->n_prov||!econ->prov[pid].active) continue;
        if (pid==cap) continue;
        victim=pid; break;
    }
    if (victim>=0){
        int16_t save_owner=econ->prov[victim].owner;
        econ->prov[victim].owner=-1;
        missions_tick(ms, w, econ, dp, 1, cid);
        ok("une province de la vallée hors de mes mains ⇒ l'échelon n'est PAS prêt", d->ready==0);
        econ->prov[victim].owner=save_owner;
    }
    /* on SATISFAIT : tout le monde à moi et colonisé. */
    for (int k=0;k<rg->n_provinces;k++){
        int pid=rg->province_ids[k];
        if (pid<0||pid>=econ->n_prov||!econ->prov[pid].active) continue;
        econ->prov[pid].owner=(int16_t)cid; econ->prov[pid].colonized=true;
    }
    missions_tick(ms, w, econ, dp, 1, cid);
    ok("toute la vallée tenue ⇒ l'échelon devient PRÊT (le sceau reste au joueur)", d->ready==1);

    /* ═══ 3. LE SCEAU : récompense en COORDONNÉE BÂTIE, jamais en or ═══ */
    printf("\n── 3. Sceller verse une coordonnée BÂTIE (aucun lingot créé) ──\n");
    float k0 = econ->prov[cap].build.K_inst;
    double tre0 = econ_country_gold(econ, cid);   /* TRÉSOR NATIONAL (2026-09-03) : l'or est au grain PAYS */
    ok("sceller un échelon qui n'est PAS le courant est refusé",
       missions_seal(ms,w,econ,dp,sc,seed,1,cid,DESS_SOL,DESS_RUNG_RIVAL,0,0.f)==0);
    ok("sceller l'échelon courant rempli est ACCEPTÉ",
       missions_seal(ms,w,econ,dp,sc,seed,1,cid,DESS_SOL,DESS_RUNG_UNIFICATION,0,0.f)==1);
    ok("K_inst de la capitale a monté de 0,6 (densité institutionnelle RÉALISÉE)",
       econ->prov[cap].build.K_inst > k0 + 0.55f && econ->prov[cap].build.K_inst < k0 + 0.65f);
    ok("le trésor n'a PAS bougé — la règle d'or §2.4 (jamais d'or créé)",
       econ_country_gold(econ, cid) == tre0);
    ok("l'échelon suivant est armé (Expansion), l'an du sceau est latché",
       d->rung==DESS_RUNG_EXPANSION && d->sealed_year[DESS_RUNG_UNIFICATION]==1);
    ok("re-sceller le même échelon est refusé (il n'est plus le courant)",
       missions_seal(ms,w,econ,dp,sc,seed,1,cid,DESS_SOL,DESS_RUNG_UNIFICATION,0,0.f)==0);

    /* ═══ 4. LA CIBLE : résolution, re-résolution, et ce qui ne bouge PAS ═ */
    printf("\n── 4. La cible : la DESTRUCTION la re-résout, l'inconvénient ne la touche pas ──\n");
    int t2 = d->tpid[DESS_RUNG_EXPANSION];
    ok("Expansion a résolu une marche étrangère/vierge (BFS pur, zéro rand)",
       t2>=0 && t2<econ->n_prov && econ->prov[t2].owner!=cid);
    if (t2>=0){
        /* (a) la cible passe à un TIERS : elle NE bouge PAS (la cible est la terre). */
        int other=-1;
        for (int c=0;c<w->n_countries;c++)
            if (c!=cid && w->country[c].role!=POLITY_UNCLAIMED){ other=c; break; }
        if (other>=0){
            econ->prov[t2].owner=(int16_t)other;
            missions_tick(ms, w, econ, dp, 2, cid);
            ok("cible passée à un tiers ⇒ la cible RESTE la même (il faudra la prendre)",
               d->tpid[DESS_RUNG_EXPANSION]==t2);
        }
        /* (b) la cible est ENGLOUTIE : re-résolution sur le monde courant. */
        econ->prov[t2].active=false;
        missions_tick(ms, w, econ, dp, 2, cid);
        ok("cible ENGLOUTIE (!active) ⇒ re-résolution déterministe sur une autre terre",
           d->tpid[DESS_RUNG_EXPANSION]!=t2);
        econ->prov[t2].active=true;
    }
    /* (c) l'objectif atteint par une AUTRE voie : l'échelon devient prêt. */
    { int t=d->tpid[DESS_RUNG_EXPANSION];
      if (t>=0){
          econ->prov[t].owner=(int16_t)cid; econ->prov[t].colonized=true;
          missions_tick(ms, w, econ, dp, 2, cid);
          ok("objectif atteint par N'IMPORTE QUELLE voie ⇒ l'échelon est prêt", d->ready==1);
          ok("… et sa récompense amorce la RECONSTRUCTION de la marche prise",
             missions_seal(ms,w,econ,dp,sc,seed,2,cid,DESS_SOL,DESS_RUNG_EXPANSION,0,0.f)==1
             && econ->prov[t].reconstruction >= 0.99f);
      } }
    ok("l'échelon « Le rival » a nommé un rival", d->rung==DESS_RUNG_RIVAL);

    /* ═══ 5. LA REMISE DATÉE : elle vit, puis elle EXPIRE ═══════════════ */
    printf("\n── 5. Le canal DATÉ : un latch d'année, zéro accumulateur ──\n");
    ok("avant tout scellage porteur, la remise vaut 1.0",
       dessein_mult(cid, DBOON_FAB_VALID_DAYS)==1.0f);
    /* on force le scellage de « Le rival » : la condition est un prédicat d'état,
     * on la satisfait en donnant la province visée au joueur. */
    { int t3=d->tpid[DESS_RUNG_RIVAL];
      if (t3>=0 && t3<econ->n_prov){ econ->prov[t3].owner=(int16_t)cid; econ->prov[t3].colonized=true; }
      missions_tick(ms, w, econ, dp, 10, cid);
      int sealed = missions_seal(ms,w,econ,dp,sc,seed,10,cid,DESS_SOL,DESS_RUNG_RIVAL,0,0.f);
      ok("« Le rival » scellé (l'épée OU le serment)", sealed==1);
      if (sealed){
          ok("la remise DATÉE est ACTIVE l'année du sceau",
             dessein_mult(cid, DBOON_FAB_VALID_DAYS)==1.60f);
          missions_boons_sync(ms, 10+19);
          ok("… encore active à 19 ans (fenêtre DESSEIN_BOON_YEARS = 20)",
             dessein_mult(cid, DBOON_FAB_VALID_DAYS)==1.60f);
          missions_boons_sync(ms, 10+20);
          ok("… ÉTEINTE à 20 ans — sans qu'aucun compteur n'ait été décrémenté",
             dessein_mult(cid, DBOON_FAB_VALID_DAYS)==1.0f);
          missions_boons_sync(ms, 10);
          ok("KILL-SWITCH DESSEIN_BOON_YEARS=0 : toutes les remises retombent à 1.0",
             (tune_set("DESSEIN_BOON_YEARS",0.f), dessein_mult(cid, DBOON_FAB_VALID_DAYS)==1.0f));
          tune_set("DESSEIN_BOON_YEARS",20.f);
      } }

    /* ═══ 6. LE PIVOT : une preuve d'usage, pas un achat au guichet ═════ */
    printf("\n── 6. Le pivot : irréversible, et il exige une PREUVE d'usage ──\n");
    ok("l'échelon courant est le PIVOT", d->rung==DESS_RUNG_PIVOT && dessein_is_pivot(d->rung));
    missions_tick(ms, w, econ, dp, 11, cid);
    ok("le pivot est toujours « prêt » — c'est le SCEAU qui exige la preuve", d->ready==1);
    ok("une voie hors domaine est refusée",
       missions_seal(ms,w,econ,dp,sc,seed,11,cid,DESS_SOL,DESS_RUNG_PIVOT,7,0.f)==0);
    /* aucun vassal, aucune terre arrachée par traité : les deux voies sont fermées. */
    ok("sans preuve d'usage, la voie Vassalisation est refusée",
       d->proof_b!=0 || missions_seal(ms,w,econ,dp,sc,seed,11,cid,DESS_SOL,DESS_RUNG_PIVOT,
                                      DESS_VOIE_VASSALISATION,0.f)==0);
    /* on FABRIQUE la preuve de la voie Conquête : la trace durable d'une province
     * arrachée par traité (rancor du dépossédé — cf. le commentaire de
     * missions_tick : conq_value est soldé À L'INTÉRIEUR de diplo_settle). */
    { int other=-1;
      for (int c=0;c<w->n_countries;c++)
          if (c!=cid && w->country[c].role!=POLITY_UNCLAIMED){ other=c; break; }
      if (other>=0) dp->rancor[other][cid] += 1.0f;
      missions_tick(ms, w, econ, dp, 11, cid);
      ok("une province arrachée par traité LATCHE la preuve de la voie Conquête", d->proof_a==1);
      ok("le pivot se scelle sur la voie Conquête",
         missions_seal(ms,w,econ,dp,sc,seed,11,cid,DESS_SOL,DESS_RUNG_PIVOT,DESS_VOIE_CONQUETE,0.f)==1);
      ok("la voie est posée, IRRÉVERSIBLE, et l'échelon suivant est armé",
         d->voie==DESS_VOIE_CONQUETE && d->rung==DESS_RUNG_4);
      ok("les noms d'échelon suivent la voie (slot d'affichage 4-7 = Conquête)",
         dessein_display_slot(DESS_RUNG_4, DESS_VOIE_CONQUETE)==4
      && dessein_display_slot(DESS_RUNG_4, DESS_VOIE_VASSALISATION)==8); }

    /* ═══ 7. SAVE → RELOAD : l'état REJOUE À L'IDENTIQUE ════════════════ */
    printf("\n── 7. Aller-retour de sauvegarde : le blob et le miroir des remises ──\n");
    memcpy(ms2, ms, sizeof *ms);                 /* le blob BRUT, comme la section MISS */
    ok("le blob sérialisé restitue l'échelon, la voie et les latches d'année",
       memcmp(&ms2->d[cid][DESS_SOL], &ms->d[cid][DESS_SOL], sizeof(Dessein))==0);
    { /* le miroir de dessein_mult est un cache de PROCESS : missions_boons_sync le
       * reconstruit intégralement depuis l'état rechargé (c'est ce que scps_save.c
       * appelle après un load — sans quoi les remises seraient muettes jusqu'à la
       * première clôture, et le savetest A==B le prendrait). */
      float before = dessein_mult(cid, DBOON_FAB_VALID_DAYS);
      missions_boons_sync(ms2, 11);
      ok("après re-synchronisation depuis la save, la remise est IDENTIQUE",
         dessein_mult(cid, DBOON_FAB_VALID_DAYS)==before); }

    /* ═══ 8. LES LECTEURS PURS (la membrane) ════════════════════════════ */
    printf("\n── 8. Les lecteurs : un échelon, une cible, un parachèvement ──\n");
    ok("dessein_of refuse une branche hors domaine", dessein_of(ms,cid,99)==NULL);
    ok("dessein_of refuse un pays hors domaine", dessein_of(ms,-1,DESS_SOL)==NULL);
    ok("le PARACHÈVEMENT est le dernier échelon, et lui seul",
       dessein_is_finale(DESS_RUNG_7) && !dessein_is_finale(DESS_RUNG_6));
    ok("la branche du Sol est portée par le siège Royaume (le Conseil n'a pas de siège de la Guerre)",
       dessein_seat_of(DESS_SOL)==1 && dessein_seat_of(99)==-1);
    ok("la cible courante est lisible (province ou couronne, jamais les deux)",
       (dessein_target_pid(d)>=0) != (dessein_target_cid(d,dp,cid)>=0)
       || (dessein_target_pid(d)<0 && dessein_target_cid(d,dp,cid)<0));

    /* ═══ 9. L'AUTRE VOIE : le serment (branche neuve, fixture forcée) ══ */
    printf("\n── 9. La voie Vassalisation : la cible est une COURONNE, pas une terre ──\n");
    { /* Le pivot est irréversible : pour éprouver l'autre voie on repart d'une
       * branche NEUVE et on l'amène au pivot par la fixture (le tronc a déjà été
       * prouvé plus haut — on ne le rejoue pas). */
      missions_init(ms2);
      missions_tick(ms2, w, econ, dp, 20, cid);
      Dessein *db = &ms2->d[cid][DESS_SOL];
      db->rung=DESS_RUNG_PIVOT; db->ready=1;
      ok("sans vassal, la preuve de la voie Vassalisation manque", db->proof_b==0);
      /* on donne un vassal : la preuve se latche à la clôture. */
      int v=-1;
      for (int c=0;c<w->n_countries;c++)
          if (c!=cid && w->country[c].role!=POLITY_UNCLAIMED
              && diplo_suzerain(dp,c)<0){ v=c; break; }
      if (v>=0) diplo_set_vassal(dp, cid, v, CONTRAT_PROTECTORAT);
      missions_tick(ms2, w, econ, dp, 20, cid);
      ok("un vassal LATCHE la preuve de la voie Vassalisation", db->proof_b==1);
      ok("le pivot se scelle sur la voie Vassalisation",
         missions_seal(ms2,w,econ,dp,sc,seed,20,cid,DESS_SOL,DESS_RUNG_PIVOT,
                       DESS_VOIE_VASSALISATION,0.f)==1);
      int v1 = dessein_target_cid(db,dp,cid);
      ok("« Premier vassal » vise une COURONNE (jamais une province)",
         db->rung==DESS_RUNG_4 && v1>=0 && dessein_target_pid(db)<0);
      missions_tick(ms2, w, econ, dp, 20, cid);
      ok("tant que CETTE couronne n'a pas juré, l'échelon n'est pas prêt",
         db->ready==0 || diplo_suzerain(dp,v1)==cid);
      if (v1>=0 && diplo_suzerain(dp,v1)!=cid) diplo_set_vassal(dp, cid, v1, CONTRAT_PROTECTORAT);
      missions_tick(ms2, w, econ, dp, 20, cid);
      ok("la couronne nommée ayant juré, l'échelon est PRÊT", db->ready==1);
      ok("le sceau verse le crédit du serment (OPINION_VASSAL daté)",
         missions_seal(ms2,w,econ,dp,sc,seed,20,cid,DESS_SOL,DESS_RUNG_4,0,0.f)==1
         && (missions_boons_sync(ms2,20), dessein_mult(cid, DBOON_OPINION_VASSAL)==1.30f));
      ok("les deux voies portent des clés DISTINCTES (aucun double-dip)",
         dessein_mult(cid, DBOON_ANNEX_SOFT_SCAR)==1.0f);
      ok("« Trois vassaux » est armé et attend TROIS serments", db->rung==DESS_RUNG_5);
      missions_boons_sync(ms, 11);   /* on rend le miroir à la branche de la voie A */
    }

    /* ═══ 6. RÉGRESSION : ready est un cache, le sceau relit l'état réel ═══ */
    printf("\n── 6. Le sceau revalide la condition après un changement d'état ──\n");
    static MissionsState regression;
    missions_init(&regression);
    int rreg=w->province[cap].region, reg_victim=-1;
    if (rreg>=0 && rreg<w->n_regions){
        const Region *rr=&w->region[rreg];
        for (int k=0;k<rr->n_provinces;k++){
            int pid=rr->province_ids[k];
            if (pid>=0 && pid<econ->n_prov && econ->prov[pid].active){
                econ->prov[pid].owner=(int16_t)cid;
                econ->prov[pid].colonized=true;
                if (pid!=cap) reg_victim=pid;
            }
        }
    }
    ok("la fixture contient une province-cible distincte de la capitale", reg_victim>=0);
    if (reg_victim>=0){
        missions_tick(&regression,w,econ,dp,30,cid);
        Dessein *rd=&regression.d[cid][DESS_SOL];
        ok("la clôture marque l'échelon prêt", rd->ready==1 && rd->rung==DESS_RUNG_UNIFICATION);
        Dessein before=*rd;
        float k_before=econ->prov[cap].build.K_inst;
        econ->prov[reg_victim].owner=-1;         /* changement après la clôture : cache volontairement périmé */
        int stale=missions_seal(&regression,w,econ,dp,sc,seed,30,cid,DESS_SOL,
                                DESS_RUNG_UNIFICATION,0,0.f);
        ok("sceau refusé si la province n'est plus tenue (revalidation réelle)", stale==0);
        ok("refus sans mutation de l'échelon ni récompense", stale==0 &&
           memcmp(rd,&before,sizeof before)==0 && econ->prov[reg_victim].owner==-1 &&
           econ->prov[cap].build.K_inst==k_before);
        econ->prov[reg_victim].owner=(int16_t)cid;
        missions_tick(&regression,w,econ,dp,31,cid);
        float k_ready=econ->prov[cap].build.K_inst;
        int accepted=missions_seal(&regression,w,econ,dp,sc,seed,31,cid,DESS_SOL,
                                   DESS_RUNG_UNIFICATION,0,0.f);
        ok("après rétablissement et clôture, le sceau réussit", accepted==1 &&
           rd->rung==DESS_RUNG_EXPANSION && econ->prov[cap].build.K_inst>k_ready+0.55f);
        float k_sealed=econ->prov[cap].build.K_inst;
        ok("un second sceau du même échelon est refusé sans double récompense",
           missions_seal(&regression,w,econ,dp,sc,seed,31,cid,DESS_SOL,
                         DESS_RUNG_UNIFICATION,0,0.f)==0 &&
           econ->prov[cap].build.K_inst==k_sealed);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w);free(econ);free(ms);free(ms2);free(dp);free(sc);
    return g_fail?1:0;
}
