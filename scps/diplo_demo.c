/*
 * diplo_demo.c — banc d'essai diplomatie & guerre (§5-§6)
 *
 *   make diplo_demo && ./diplo_demo [graine]
 *
 * Prouve :
 *   1. Les relations se LISENT : menace, parenté (sphère), schisme, alliance —
 *      calculées sur des coordonnées existantes, pas posées à la main.
 *   2. La guerre = territoire CONTRE diversité : conquérir une culture lointaine
 *      monte le D̄ interne du conquérant → la fracture monte (gated par K).
 *      Concorde « Unie » → « Murmurante/Fracturée ».
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"
#include "scps_diplo.h"
#include "scps_tune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0,g_fail=0;
static void ok(const char*w,bool c){ printf("   %s %s\n",c?"✓":"✗",w); if(c)g_pass++; else g_fail++; }

static float content_dist(const PopCulture*a,const PopCulture*b){
    float dv=a->valeurs-b->valeurs;       if(dv<0)dv=-dv;
    float ds=a->subsistance-b->subsistance; if(ds<0)ds=-ds;
    float dp=a->parente-b->parente;       if(dp<0)dp=-dp;
    float dr=a->religion-b->religion;     if(dr<0)dr=-dr;
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}

int main(int argc,char**argv){
    /* Fixture STABLE : monde pinné à ~320 territoires (le banc teste la diplomatie/valeur de
     * province, pas le scaling f(empires) ; un monde géant change les compteurs et fausse les seuils). */
    if (!getenv("SCPS_TUNE")){
        tune_set("WORLD_PROV_BASE",320.f);
        tune_set("WORLD_PROV_PER_EMPIRE",0.f);
        tune_set("WORLD_PROV_PER_CITY",0.f);
    }
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World*w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    TradeNetwork*net=malloc(sizeof(TradeNetwork)); TechState*ts=calloc(SCPS_MAX_COUNTRY,sizeof(TechState));
    WorldProsperity*wp=malloc(sizeof(WorldProsperity)); WorldLegitimacy*wl=malloc(sizeof(WorldLegitimacy));
    DiploState*dp=malloc(sizeof(DiploState));
    if(!w||!econ||!net||!ts||!wp||!wl||!dp){fprintf(stderr,"OOM\n");return 1;}

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" DIPLOMATIE & GUERRE — lire les relations, payer la conquête (graine %u)\n",seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    trade_network_build(net,w,econ);
    for(int c=0;c<w->n_countries;c++) tech_state_init(&ts[c],false);
    prosperity_init(wp,w); legitimacy_init(wl,w,econ); diplo_init(dp);

    int player=0; for(int c=0;c<w->n_countries;c++) if(w->country[c].role==POLITY_PLAYER){player=c;break;}

    /* L'économie se met en route, SANS colonisation : le joueur reste sur sa
     * seule capitale (homogène, D̄≈0) — pour isoler l'effet de la conquête. */
    for(int t=0;t<8;t++){
        econ_tick(econ, 1.f);
        legitimacy_tick(wl,w,econ,ts); prosperity_tick(wp,w,econ,net,ts,wl);
    }

    /* ---- 1. Les relations se lisent ---------------------------------- */
    printf("\n── 1. Relations du joueur (pays %d) — tout est LU ──\n",player);
    printf("   %-10s %-8s %-8s %-8s %-8s %-9s\n","pays","menace","parenté","schisme","compl.","alliance");
    int shown=0;
    for(int c=0;c<w->n_countries && shown<6;c++){
        if(c==player) continue;
        Relation r=diplo_relation(w,econ,wp,dp,player,c);
        printf("   #%-9d %-8.2f %-8.0f %-8.2f %-8.2f %-+9.2f\n",
               c,r.threat,r.kinship,r.schism,r.complement,r.alliance);
        shown++;
    }

    /* ---- 2. La guerre monte la diversité ----------------------------- */
    printf("\n── 2. Territoire CONTRE diversité ──\n");
    int cap_reg=-1;
    { int cp=w->country[player].capital_prov;
      if(cp>=0) cap_reg=w->province[cp].region; }
    /* RE-BASELINE : la genèse distribue désormais les empires sur PLUSIEURS régions
     * (≥1 empire par continent habitable), donc le joueur naît déjà DIVERS (D̄≈4-5).
     * Or ce banc isole l'effet de LA conquête : il veut un conquérant homogène (D̄≈0)
     * qui, en avalant UNE culture lointaine, voit son D̄ BONDIR. On restaure donc la
     * prémisse — on rabat le joueur sur sa SEULE capitale, culturellement UNIE : chaque
     * autre région possédée reçoit la culture de la capitale (mono-groupe pur). La
     * lecture redevient nette : 0 → un saut franc, pas une dilution dans le bruit. */
    if(cap_reg>=0){
        PopCulture capc=econ->region[cap_reg].culture;
        for(int r=0;r<econ->n_regions;r++){
            RegionEconomy*re=&econ->region[r];
            if(re->owner!=player || r==cap_reg) continue;
            re->culture=capc;                 /* même culture que le cœur → D̄=0 entre régions */
            memset(&re->pop,0,sizeof re->pop);  /* pas de groupes hérités divergents */
        }
    }
    const PopCulture *pcap = (cap_reg>=0)? &econ->region[cap_reg].culture : NULL;

    /* cible : la région PEUPLÉE la plus LOINTAINE culturellement, hors joueur. */
    int target=-1; float best=-1.f; int tgt_owner=-1;
    for(int r=0;r<econ->n_regions;r++){
        RegionEconomy*re=&econ->region[r];
        if(!re->culture.settled || re->owner==player || re->owner<0) continue;
        float d = pcap?content_dist(&re->culture,pcap):0.f;
        if(d>best){best=d; target=r; tgt_owner=re->owner;}
    }
    if(target<0){ printf("   (pas de cible peuplée ennemie — monde trop vide)\n"); }
    else {
        CountryProsperity*cp=&wp->country[player];
        /* re-mesurer APRÈS l'homogénéisation : le profil (D̄_int) ne bouge qu'au tick. */
        for(int t=0;t<3;t++){ legitimacy_tick(wl,w,econ,ts); prosperity_tick(wp,w,econ,net,ts,wl); }
        float Dbar0=cp->profile.D_bar_int, frac0=cp->fracture;
        CountryReadout r0=country_readout(wp,ts,w,player);
        printf("   AVANT : Concorde=%-12s  [dev D̄_int=%.2f fracture=%.2f]\n",
               label_concorde(r0.concorde), Dbar0, frac0);

        printf("   → guerre au pays #%d, conquête de la région %d (« %s », D∞=%.1f de nous)\n",
               tgt_owner, target, w->region[target].name, best);
        diplo_declare_war(dp,player,tgt_owner);
        /* §terrain : l'armée INVESTIT la région (occupation), la PAIX la transfère —
         * bornée par le budget §5. On assure la domination (cible désarmée, joueur
         * SURARMÉ) pour que la prise ait lieu, puis on RÈGLE : la région passe au joueur.
         * À la nouvelle échelle, les deux pays pèsent des MILLIERS d'âmes : le mil_power
         * est dominé par la population (parité ⇒ ratio≈0.5 ⇒ budget nul). On sépare donc
         * franchement la force par l'ARMEMENT — le terme qui survit à la masse : on désarme
         * la cible (stocks → 0) et on surarme le joueur (armes enchantées + de base, près
         * de leur plafond) → ratio≈0.7 ⇒ budget ≫ prix. On ne touche QUE les stocks (pas
         * le H_coerc), pour ne rien laisser fuir dans les scénarios de §5/§8 plus bas. */
        for(int r=0;r<econ->n_regions;r++){
            if(econ->region[r].owner==tgt_owner){ econ->region[r].stock[RES_ARMS]=0.f;
                econ->region[r].stock[RES_GUNPOWDER]=0.f; econ->region[r].stock[RES_ENCHANTED_ARMS]=0.f; }
            else if(econ->region[r].owner==player){
                econ->region[r].stock[RES_ENCHANTED_ARMS]=8000.f; econ->region[r].stock[RES_ARMS]=8000.f; }
        }
        diplo_occupy(dp,econ,player,target);
        int got=diplo_settle(dp,w,econ,wl,player,tgt_owner,false);
        /* RE-KEY PROVINCE : settle_transfer route owner sur les PROVINCES membres —
         * econ->region[target].owner ne se re-dérive qu'au prochain econ_tick (aggregate).
         * On vérifie directement au grain province (econ_region_rep_province). */
        int target_pid=econ_region_rep_province(econ,target);
        bool took=(got>0 && target_pid>=0 && econ->prov[target_pid].owner==player);

        /* recalcul : la région conquise est désormais à nous → diversité (econ_tick
         * ré-agrège region[] depuis prov[] — ici via legitimacy/prosperity ticks qui ne
         * ré-agrègent PAS ; on force un econ_tick pour que la lecture region[] post-conquête
         * (Dbar/fracture plus bas, via country_readout) reflète le nouveau territoire). */
        econ_tick(econ,1.f/12.f);
        for(int t=0;t<3;t++){ legitimacy_tick(wl,w,econ,ts); prosperity_tick(wp,w,econ,net,ts,wl); }
        float Dbar1=cp->profile.D_bar_int, frac1=cp->fracture;
        CountryReadout r1=country_readout(wp,ts,w,player);
        printf("   APRÈS : Concorde=%-12s  [dev D̄_int=%.2f fracture=%.2f]\n",
               label_concorde(r1.concorde), Dbar1, frac1);

        printf("\n── Vérification ──\n");
        ok("la conquête a eu lieu (owner transféré)", took && econ->region[target].owner==player);
        ok("la région conquise démarre à légitimité effondrée",
           wl->L[target] <= 2.0f);
        ok("conquérir une culture lointaine monte la diversité interne (D̄↑)",
           Dbar1 > Dbar0 + 0.3f);
        ok("la diversité non métabolisée monte la fracture", frac1 > frac0);
    }

    /* ---- 3. Diplomatie d'ÉQUILIBRE : trêve · momentum · friction · coalition ---- */
    printf("\n── 3. Diplomatie d'équilibre (rétroaction négative, pas d'interdit) ──\n");
    {
        /* B = un voisin ; C = un pays avec une FORCE réelle (l'allié dont l'entrée
         * pèse) — sinon le coût d'élargissement serait nul (pays vide). */
        int A=player, B=-1, C=-1;
        for(int c=0;c<w->n_countries;c++){ if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
            if(B<0){B=c;continue;}
            if(diplo_mil_power(w,econ,c)>0.01f){ C=c; break; } }
        if(B>=0 && C>=0){
            /* TRÊVE : une longue guerre → une longue trêve ; on n'enchaîne plus. */
            diplo_init(dp); dp->war_years[A][B]=dp->war_years[B][A]=4.f;
            diplo_make_peace(dp,A,B);
            float tr4=diplo_truce_days(dp,A,B);
            ok("après la paix, on ne peut PAS redéclarer (trêve)", !diplo_can_declare(dp,A,B));
            diplo_init(dp); dp->war_years[A][B]=dp->war_years[B][A]=8.f;
            diplo_make_peace(dp,A,B);
            ok("une plus LONGUE guerre → une plus longue trêve", diplo_truce_days(dp,A,B) > tr4);
            diplo_tick(dp, 365.f*15.f);
            ok("la trêve FOND : la guerre redevient possible après le répit", diplo_can_declare(dp,A,B));

            /* MOMENTUM : la fulgurance effraie plus que la masse statique. */
            diplo_init(dp);
            float th0=diplo_relation(w,econ,wp,dp,A,B).threat;
            dp->momentum[B]=6.f;
            float th1=diplo_relation(w,econ,wp,dp,A,B).threat;
            ok("un conquérant FULGURANT menace plus qu'à puissance statique égale", th1 > th0+0.01f);

            /* FRICTION : un protégé d'un allié puissant renchérit (coût d'élargissement). */
            diplo_init(dp);
            float wno=diplo_war_widening_cost(w,econ,dp,A,B);
            diplo_form_alliance(dp,B,C);
            ok("frapper un protégé d'allié ÉLARGIT la guerre (coût ↑)",
               diplo_war_widening_cost(w,econ,dp,A,B) > wno);

            /* COALITION : un hégémon (fulgurance extrême) est perçu comme menace dominante. */
            diplo_init(dp); dp->momentum[B]=40.f;
            ok("un hégémon fulgurant est PERÇU (posture de coalition, sans script)",
               diplo_perceived_hegemon(w,econ,wp,dp,A)==B);
        } else {
            ok("(monde trop petit pour le test d'équilibre)", true);
        }
    }

    /* ---- 4. Casus belli : la guerre a une RAISON (lue) ; son type gate la paix ---- */
    printf("\n── 4. Casus belli (la guerre a une raison ; son type fixe le but) ──\n");
    {
        int A=player, B=-1, Badj=-1;
        for(int c=0;c<w->n_countries;c++){
            if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
            if(B<0)B=c;
            bool adj=false;
            for(int r=0;r<econ->n_regions&&!adj;r++) if(econ->region[r].owner==A)
                for(int s=0;s<econ->n_regions;s++) if(econ->region[s].owner==c&&econ->adj[r][s]){adj=true;break;}
            if(adj&&Badj<0)Badj=c;
        }
        if(B>=0){
            diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_ECONOMIC);
            ok("déclarer avec un casus belli MÉMORISE le but de guerre", diplo_war_goal(dp,A,B)==CB_ECONOMIC);
            diplo_make_peace(dp,A,B);
            ok("la paix ÉTEINT le but de guerre", diplo_war_goal(dp,A,B)==CB_NONE);
            int Br=-1; for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==B){Br=r;break;}
            if(Br>=0){
                for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==A) econ->region[r].raw_cap[RES_SALTPETER]=0.f;
                econ->region[Br].raw_cap[RES_SALTPETER]=2.f;   /* B monopolise un bien stratégique */
                ok("un bien aigu MONOPOLISÉ par la cible → casus belli ÉCONOMIQUE",
                   diplo_casus_belli(w,econ,wp,dp,A,B,RES_SALTPETER)==CB_ECONOMIC);
            }
        }
        if(Badj>=0)
            ok("un voisin (adjacence) fournit un casus belli — une raison existe (≠ aucun)",
               diplo_casus_belli(w,econ,wp,dp,A,Badj,RES_NONE)!=CB_NONE);
    }

    /* ---- 4b. CROISADE faustienne : Gardiens orthodoxes vs Transgresseurs ---- */
    printf("\n── 4b. La croisade faustienne (l'orthodoxe frappe le développeur de l'interdit) ──\n");
    {
        int crusader=player, heretic=-1;
        for(int c=0;c<w->n_countries;c++) if(c!=crusader && w->country[c].role!=POLITY_UNCLAIMED){ heretic=c; break; }
        if(heretic>=0){
            diplo_init(dp);
            int capR=w->province[w->country[crusader].capital_prov].region;
            econ->region[capR].culture.ethos=ETHOS_ORDRE;          /* le croisé est ORTHODOXE */
            diplo_set_faustian(dp,heretic,6.0f);                    /* l'hérétique développe l'interdit */
            ok("la souillure faustienne se LIT", diplo_faustian(dp,heretic)==6.0f);
            ok("un orthodoxe a un CASUS BELLI (croisade) contre un développeur faustien",
               diplo_faustian_cb(w,econ,dp,crusader,heretic) &&
               diplo_casus_belli(w,econ,wp,dp,crusader,heretic,RES_NONE)==CB_RELIGIOUS);
            diplo_set_faustian(dp,heretic,0.f);
            ok("sans souillure faustienne, PAS de croisade", !diplo_faustian_cb(w,econ,dp,crusader,heretic));
            econ->region[capR].culture.ethos=ETHOS_DOMINATEUR;     /* un empire permissif */
            diplo_set_faustian(dp,heretic,6.0f);
            ok("un empire PERMISSIF ne croise pas (il cède lui-même à l'interdit)",
               !diplo_faustian_cb(w,econ,dp,crusader,heretic));
        } else ok("(monde trop petit pour la croisade)", true);
    }

    /* ---- 5. Score de guerre (§2) : le bras-de-fer (batailles, occupation, attrition) ---- */
    printf("\n── 5. Score de guerre (batailles plafonnées +50 · occupation · attrition) ──\n");
    {
        int A=player, B=-1, Ar=-1;
        for(int c=0;c<w->n_countries && B<0;c++){
            if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==c){ B=c; break; }
        }
        for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==A){ Ar=r; break; }
        if(B>=0 && Ar>=0){
            diplo_init(dp);
            diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);   /* A = attaquant */
            float armsA0=0.f;
            /* RE-KEY : le surarmement se sème PROVINCE-persistant (helper : porteuse+vue) —
             * écrit sur la seule vue, il serait invisible à l'attrition réelle (deplete_arms
             * mord désormais les provinces). armsA0 = la vue APRÈS semis (miroir exact). */
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==A)
                econ_region_stock_add(econ, r, RES_ENCHANTED_ARMS, 1000.f);   /* A surarme */
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==A)
                armsA0+=econ->region[r].stock[RES_ENCHANTED_ARMS];
            for(int y=0;y<15;y++) diplo_war_tick(dp,w,econ,wp,1.f);
            ok("l'avantage militaire POUSSE le battle_score (l'attaquant gagne les batailles)",
               dp->battle_score[A][B] > 5.f);
            ok("le battle_score est PLAFONNÉ à +50 (les batailles seules ne gagnent pas)",
               dp->battle_score[A][B] <= 50.01f);
            dp->conquered[A][B]=5;
            ok("l'occupation porte le score AU-DELÀ de +50 (l'autre moitié)",
               diplo_war_score(dp,A,B) > 50.5f);
            float armsA1=0.f;
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==A) armsA1+=econ->region[r].stock[RES_ENCHANTED_ARMS];
            ok("l'attrition SAIGNE les armes pendant la guerre (épuisement)", armsA1 < armsA0);
        }
    }

    /* ---- 6. Saccage (§4) : la prise DÉPOUILLE la province (1×/5 ans) ---- */
    printf("\n── 6. Saccage (or + production → trésor de l'occupant · 1×/5 ans) ──\n");
    if(econ->n_regions>=2){
        int vic=0, occ=1;
        /* RE-KEY PROVINCE : treasury/revolt_scar/pillage_cd sont PROVINCE-OWNED (routés par
         * diplo_pillage_region sur econ_region_rep_province) — stock[]/price[] restent au
         * grain RÉGION (le marché, charte). On pose/lit donc sur la province représentative
         * pour treasury/scar/cd, et sur la région pour stock/price. */
        int vic_pid=econ_region_rep_province(econ,vic), occ_pid=econ_region_rep_province(econ,occ);
        RegionEconomy *rvr=&econ->region[vic];
        ProvinceEconomy *rv=&econ->prov[vic_pid], *ro=&econ->prov[occ_pid];
        rv->pillage_cd=0.f; rv->revolt_scar=0.f; rv->treasury=1000.f;
        for(int g=1;g<RES_COUNT;g++){ rvr->stock[g]=10.f; rvr->price[g]=1.f; }
        float occ0=ro->treasury, vic0=rv->treasury;
        float loot=diplo_pillage_region(econ,vic,occ);
        printf("   sac de la région %d → %d : butin = %.0f or-équiv.\n",vic,occ,loot);
        ok("le saccage rapporte un butin (or des coffres + entrepôt valorisé)", loot>600.f);
        ok("le trésor de l'OCCUPANT enfle du butin entier",
           ro->treasury > occ0+loot-1.f && ro->treasury < occ0+loot+1.f);
        ok("la province pillée est VIDÉE de son or", rv->treasury < vic0);
        ok("le sac CONVULSE la province (cicatrice au plancher → gel)", rv->revolt_scar > 0.99f);
        float loot2=diplo_pillage_region(econ,vic,occ);
        ok("on ne RE-saccage pas avant 5 ans (plus rien à prendre)", loot2==0.f);
        ok("le compteur anti-saccage est armé (~5 ans)", rv->pillage_cd > 4.5f);
        for(int t=0;t<6;t++) econ_tick(econ,1.f);   /* 6 ans s'écoulent (cd décroît par an) */
        ok("après ~5 ans, la province REDEVIENT saccageable", econ->prov[vic_pid].pillage_cd <= 0.f);
    } else ok("(monde trop petit pour le test de saccage)", true);

    /* ---- 6b. LOT 4 — LE PILLAGE DE SIÈGE (mensuel, ∝ production, PENDANT le siège) ---- */
    printf("\n── 6b. LOT 4 : le siège détourne CHAQUE MOIS une fraction de la PRODUCTION (matière RÉELLEMENT prise) ──\n");
    if(econ->n_regions>=2){
        int vic=0, occ=1;
        int vic_pid=econ_region_rep_province(econ,vic), occ_pid=econ_region_rep_province(econ,occ);
        RegionEconomy *rvr=&econ->region[vic];
        ProvinceEconomy *rv=&econ->prov[vic_pid], *ro=&econ->prov[occ_pid];
        rv->pillage_cd=0.f;                                  /* province FRAÎCHE (pas déjà sac(c)agée) */
        for(int g=1;g<RES_COUNT;g++){ rvr->stock[g]=100.f; rvr->supply[g]=40.f; rvr->price[g]=1.f; }
        float vic_stock_before=0.f, occ_treas_before=ro->treasury;
        for(int g=1;g<RES_COUNT;g++) vic_stock_before += rvr->stock[g];
        float loot=diplo_siege_loot(econ,vic,occ);
        printf("   siège de la région %d → capitale de %d : détourné ce mois = %.0f or-équiv.\n",vic,occ,loot);
        ok("le siège détourne ∝ la PRODUCTION du mois (supply·SIEGE_LOOT_FRAC), pas un or plat",
           loot > 0.f && loot < 40.f*(float)(RES_COUNT-1));   /* borné par 25% de Σ supply valorisée à prix 1 */
        ok("le trésor du BESIÉGEUR enfle du détourné", ro->treasury > occ_treas_before);
        float vic_stock_after=0.f; for(int g=1;g<RES_COUNT;g++) vic_stock_after+=rvr->stock[g];
        ok("la MATIÈRE est RÉELLEMENT PRISE (le stock régional DIMINUE d'autant, pas dupliquée)",
           vic_stock_after < vic_stock_before && (vic_stock_before-vic_stock_after) > 0.f);
        ok("Σ CONSERVÉE : ce que le besiégé perd égale (or-équivalent) ce que le besiégeur gagne",
           fabsf((vic_stock_before-vic_stock_after) - loot) < 0.01f);   /* prix=1 partout : stock perdu == or gagné */
        /* le MÊME cooldown que le butin final (anti-double-sac, LOT 4) : une province
         * déjà pillée/capturée récemment ne se fait plus détourner sa production. */
        rv->pillage_cd = 3.0f;
        ok("sous le cooldown anti-re-saccage (partagé avec le butin final), le siège NE détourne PLUS",
           diplo_siege_loot(econ,vic,occ)==0.f);
        rv->pillage_cd = 0.f;   /* on remet comme trouvé pour la suite du banc */
    } else ok("(monde trop petit pour le test de pillage de siège)", true);

    /* ---- 7. Paix proportionnelle (§5) : revendication ∝ domination · surexpansion → coalition ---- */
    printf("\n── 7. Paix proportionnelle (revendication ∝ domination · indemnité · surexpansion) ──\n");
    {
        /* B = le pays NON-joueur le plus ÉTOFFÉ (≥2 régions) : prendre UNE province ne
         * l'anéantit pas → sa puissance reste lisible (claim stable) et il reste un
         * trésor à ponctionner après. */
        int A=player, B=-1, Bn=0;
        for(int c=0;c<w->n_countries;c++){
            if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
            int n=0; for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==c) n++;
            if(n>Bn){ Bn=n; B=c; }
        }
        if(B>=0 && Bn>=2){
            /* REVENDICATION ∝ domination militaire : un dominant annexe LÉGITIMEMENT plus. */
            for(int r=0;r<econ->n_regions;r++){
                econ->region[r].stock[RES_ARMS]=econ->region[r].stock[RES_GUNPOWDER]=econ->region[r].stock[RES_ENCHANTED_ARMS]=0.f;
                if(econ->region[r].owner==B) econ->region[r].stock[RES_ENCHANTED_ARMS]=4000.f; /* B surarme → A marginal */
            }
            diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
            int claim_marg=diplo_war_claim(dp,w,econ,A,B);
            for(int r=0;r<econ->n_regions;r++){
                econ->region[r].stock[RES_ENCHANTED_ARMS]=0.f;
                if(econ->region[r].owner==A) econ->region[r].stock[RES_ENCHANTED_ARMS]=4000.f; /* A surarme → A dominant */
            }
            int claim_dom=diplo_war_claim(dp,w,econ,A,B);
            printf("   revendication territoriale : marginal=%d prov · dominant=%d prov\n",claim_marg,claim_dom);
            ok("un attaquant DOMINANT a une revendication légitime plus large", claim_dom > claim_marg);
            ok("un attaquant marginal n'a droit qu'à la province-frontière (≥1)", claim_marg >= 1);

            /* un casus belli NON-territorial ne donne droit qu'à UNE prise (humiliation/source). */
            diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_RELIGIOUS);
            ok("un casus belli non-territorial ne vaut qu'UNE prise (pas d'annexion étendue)",
               diplo_war_claim(dp,w,econ,A,B)==1);

            /* BORNAGE PAR LE BUDGET (§5, prix log-compressé P-bis) : on OCCUPE DEUX régions
             * de B, mais B reste FORT (surarmé) → budget marginal → la PAIX ne transfère que
             * ce que le budget couvre, PAS tout l'occupé (pas d'annexion gratuite). */
            for(int r=0;r<econ->n_regions;r++){
                econ->region[r].stock[RES_ARMS]=econ->region[r].stock[RES_GUNPOWDER]=0.f;
                econ->region[r].stock[RES_ENCHANTED_ARMS]=(econ->region[r].owner==B)?6000.f:0.f;
            }
            diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
            int Br1=-1,Br2=-1;
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==B && econ->region[r].culture.settled){ if(Br1<0)Br1=r; else if(Br2<0){Br2=r;break;} }
            if(Br1>=0 && Br2>=0){
                diplo_occupy(dp,econ,A,Br1); diplo_occupy(dp,econ,A,Br2);   /* deux régions INVESTIES */
                int got=diplo_settle(dp,w,econ,wl,A,B,false);               /* B surarmé → budget marginal */
                ok("le budget BORNE la prise : B surarmé → la paix ne transfère pas tout l'occupé",
                   got < 2);
            } else ok("(pas de cible nette pour le test de bornage)", true);

            /* RÉPARATIONS : le vaincu net indemnise le vainqueur ∝ score. */
            diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
            dp->battle_score[A][B]=50.f; dp->conquered[A][B]=5;          /* score de A ≈ +100 */
            for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==B) econ->region[r].treasury=1000.f;
            int capA=w->province[w->country[A].capital_prov].region;
            float capA0=(capA>=0)?econ->region[capA].treasury:0.f;
            float rep=diplo_reparations(dp,w,econ,A,B);
            printf("   indemnité de guerre extorquée : %.0f or\n",rep);
            ok("le VAINCU net paie une indemnité au vainqueur (∝ score)", rep>0.f);
            ok("le trésor du VAINQUEUR enfle de l'indemnité", capA>=0 && econ->region[capA].treasury > capA0);

            /* match nul : aucune indemnité (pas de vainqueur net). */
            diplo_init(dp);
            ok("un MATCH NUL (score sous le seuil) n'extorque aucune indemnité",
               diplo_reparations(dp,w,econ,A,B)==0.f);
        } else ok("(monde trop petit pour le test de paix proportionnelle)", true);
    }

    /* ---- 8. Rancune nationale (§6) : irrédentisme · ralliement · l'oubli ---- */
    printf("\n── 8. Rancune nationale (irrédentisme · ralliement · l'oubli) ──\n");
    {
        int A=player, P=-1, B=-1, Bn=0;
        for(int c=0;c<w->n_countries;c++){
            if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
            if(P<0) P=c;
            int n=0; for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==c && econ->region[r].culture.settled) n++;
            if(n>Bn){ Bn=n; B=c; }
        }
        if(P<0){ ok("(monde trop petit pour le test de rancune)", true); }
        else {
            /* RALLIEMENT (avant toute conquête : B intact) — à rancune ÉGALE de
             * domination, le lésé qui reprend ses terres accumule le score plus vite. */
            if(B>=0){
                for(int r=0;r<econ->n_regions;r++){
                    econ->region[r].stock[RES_ARMS]=econ->region[r].stock[RES_GUNPOWDER]=0.f;
                    econ->region[r].stock[RES_ENCHANTED_ARMS]=(econ->region[r].owner==A)?2000.f:0.f;
                }
                diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
                for(int y=0;y<3;y++) diplo_war_tick(dp,w,econ,wp,1.f);
                float noRally=dp->battle_score[A][B];
                diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
                dp->rancor[A][B]=3.0f;                       /* A reprend SES terres */
                for(int y=0;y<3;y++) diplo_war_tick(dp,w,econ,wp,1.f);
                ok("le RALLIEMENT galvanise la reconquête (bras-de-fer plus rapide à domination égale)",
                   dp->battle_score[A][B] > noRally + 0.5f);
            } else ok("(pas de cible pour le ralliement)", true);

            /* IRRÉDENTISME : la rancune seule donne un CB territorial SANS adjacence. */
            int Bfar=-1;
            for(int c=0;c<w->n_countries;c++){
                if(c==A||w->country[c].role==POLITY_UNCLAIMED) continue;
                bool adj=false;
                for(int r=0;r<econ->n_regions&&!adj;r++) if(econ->region[r].owner==A)
                    for(int s=0;s<econ->n_regions;s++) if(econ->region[s].owner==c&&econ->adj[r][s]){adj=true;break;}
                if(!adj){ Bfar=c; break; }
            }
            if(Bfar>=0){
                diplo_init(dp);
                CasusBelli cb0=diplo_casus_belli(w,econ,wp,dp,A,Bfar,RES_NONE);
                dp->rancor[A][Bfar]=2.0f;
                CasusBelli cb1=diplo_casus_belli(w,econ,wp,dp,A,Bfar,RES_NONE);
                ok("sans rancune NI adjacence : pas de casus belli territorial", cb0!=CB_TERRITORIAL);
                ok("la RANCUNE seule donne un CB territorial (irrédentisme, sans adjacence)", cb1==CB_TERRITORIAL);
            } else ok("(pas de pays non-adjacent pour le test d'irrédentisme)", true);

            /* La rancune SURVIT à la paix, puis S'OUBLIE sur une génération. */
            diplo_init(dp); dp->rancor[A][P]=2.0f;
            diplo_make_peace(dp,A,P);
            ok("la rancune SURVIT à la paix (le grief ne s'éteint pas avec la guerre)", diplo_rancor(dp,A,P) > 1.9f);
            diplo_tick(dp, 365.f*5.f);
            float r5=diplo_rancor(dp,A,P);
            ok("la rancune S'ESTOMPE avec le temps (5 ans → elle fond)", r5 < 1.9f && r5 > 0.f);
            diplo_tick(dp, 365.f*30.f);
            ok("sur une génération, la rancune est OUBLIÉE (→ 0)", diplo_rancor(dp,A,P)==0.f);

            /* CONQUÊTE (en dernier : mute B) — la prise (au RÈGLEMENT) POSE la rancune
             * sur le dépossédé, ∝ ce qui lui est ARRACHÉ. A dominant → la paix transfère. */
            if(B>=0 && Bn>=2){
                int Br1=-1,Br2=-1;
                for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==B && econ->region[r].culture.settled){ if(Br1<0)Br1=r; else {Br2=r;break;} }
                for(int r=0;r<econ->n_regions;r++){          /* A dominant (B désarmé) → la paix transfère */
                    if(econ->region[r].owner==B){ econ->region[r].stock[RES_ARMS]=0.f;
                        econ->region[r].stock[RES_ENCHANTED_ARMS]=0.f; econ->region[r].stock[RES_GUNPOWDER]=0.f; }
                    else if(econ->region[r].owner==A) econ->region[r].stock[RES_ARMS]=3000.f;
                }
                /* UNE perte (occuper puis régler) : la rancune se POSE (B garde ≥1 région). */
                diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
                diplo_occupy(dp,econ,A,Br1); diplo_settle(dp,w,econ,wl,A,B,false);
                float rancor_one=diplo_rancor(dp,B,A);
                econ->region[Br1].owner=(int16_t)B;          /* on rend Br1 à B */
                ok("perdre une province POSE la rancune sur le dépossédé", rancor_one >= 0.9f);
                /* DEUX pertes → rancune plus PROFONDE — seulement si B SURVIT (Bn≥3) : un B
                 * ANÉANTI verrait sa rancune éteinte par sa MORT (on teste le dépossédé vivant). */
                if(Bn>=3 && Br2>=0){
                    diplo_init(dp); diplo_declare_war_cb(dp,A,B,CB_TERRITORIAL);
                    diplo_occupy(dp,econ,A,Br1); diplo_occupy(dp,econ,A,Br2); diplo_settle(dp,w,econ,wl,A,B,false);
                    ok("perdre PLUS de terres CREUSE la rancune (∝ ce qui est arraché)",
                       diplo_rancor(dp,B,A) >= rancor_one + 0.9f);
                } else ok("(monde trop petit pour la rancune ∝ pertes : B survivrait à peine)", true);
            } else ok("(pas assez de provinces pour la rancune de conquête)", true);
        }
    }

    /* ---- 9. Esclavage (§4c) : déportation → groupe restif au cœur (D̄↑) ---- */
    printf("\n── 9. Esclavage (déportation → groupe restif au cœur · D̄↑) ──\n");
    {
        int A=player;
        int capR=(w->country[A].capital_prov>=0)? w->province[w->country[A].capital_prov].region : -1;
        int srcR=-1; for(int r=0;r<econ->n_regions;r++) if(r!=capR){ srcR=r; break; }
        /* RE-KEY PROVINCE : .pop est PROVINCE-OWNED (miroir, pas Σ) — diplo_enslave_capture
         * route désormais sur econ_region_rep_province. Setup ET vérifications au même grain. */
        int capP=(capR>=0)?econ_region_rep_province(econ,capR):-1;
        int srcP=(srcR>=0)?econ_region_rep_province(econ,srcR):-1;
        if(capP>=0 && srcP>=0){
            /* Le banc tourne en mono-groupe : on PEUPLE explicitement le cœur (natifs)
             * et une province source (un groupe ÉTRANGER — des claniques). */
            PopGroup nat; memset(&nat,0,sizeof nat);
            nat.heritage=HERITAGE_ADAPTATIF; nat.klass=CLASS_BOURGEOIS; nat.count=8000;
            nat.integration=1.f; nat.L=6.f; nat.drift_id=111; nat.origin_sphere=heritage_sphere(HERITAGE_ADAPTATIF);
            econ->prov[capP].pop.n_groups=1; econ->prov[capP].pop.groups[0]=nat;
            PopGroup foe; memset(&foe,0,sizeof foe);
            foe.heritage=HERITAGE_CLANIQUE; foe.klass=CLASS_LABORER; foe.count=4000;
            foe.integration=1.f; foe.L=5.f; foe.drift_id=222; foe.origin_sphere=heritage_sphere(HERITAGE_CLANIQUE);
            econ->prov[srcP].pop.n_groups=1; econ->prov[srcP].pop.groups[0]=foe;

            /* GATE = esclavagiste (TECH_ESCLAVAGE OU éthos conquérant, résolu par l'appelant) : booléen.
             * BRASSAGE : SLAVE_FRACTION calé BAS (0.08 « taux très faible ») — la déportation apporte
             * (savoir arraché, diffusion faible) sans jamais dominer. */
            long captives=diplo_enslave_capture(w,econ,A,srcR,/*enslaves*/true);
            printf("   captifs déportés au cœur : %ld (sur 4000, ~8%%)\n",captives);
            ok("un esclavagiste DÉPORTE une FRACTION FAIBLE (~8 %) de la population prise",
               captives>0 && captives<=500);
            ok("la capitale gagne un GROUPE de plus — les captifs au cœur",
               econ->prov[capP].pop.n_groups==2);
            PopGroup *g=&econ->prov[capP].pop.groups[econ->prov[capP].pop.n_groups-1];
            ok("le groupe d'esclaves est RESTIF (non-intégré + diaspora → D̄↑ au centre)",
               g->integration<0.01f && g->diaspora && g->heritage==HERITAGE_CLANIQUE);
            ok("BRASSAGE : le captif est flaggé DÉPORTÉ (diffuse FAIBLE, voie coercitive)",
               g->arrival==ARR_DEPORTE);
            ok("la province prise PERD la population déportée",
               econ->prov[srcP].pop.groups[0].count < 4000);
            /* GATE : sans la TECH d'asservissement (enslaves=false), personne n'est asservi. */
            ok("sans la TECH d'asservissement (TECH_ESCLAVAGE), personne n'est capturé",
               diplo_enslave_capture(w,econ,A,srcR,/*enslaves*/false)==0);
        } else ok("(monde trop petit pour le test d'esclavage)", true);
    }

    /* ---- 10. BRASSAGE : le PACTE MIGRATOIRE (voie pacifique, RÉCIPROQUE) ---- */
    printf("\n── 10. Pacte migratoire (brassage pacifique · réciproque) ──\n");
    if (w->n_countries>=2){
        int A=0, B=1;
        ok("au départ, aucun pacte migratoire entre A et B",
           !diplo_migration_pact(dp,A,B) && !diplo_migration_pact(dp,B,A));
        diplo_set_migration_pact(dp,A,B,true);
        ok("poser le pacte est RÉCIPROQUE (l'échange va dans les deux sens)",
           diplo_migration_pact(dp,A,B) && diplo_migration_pact(dp,B,A));
        ok("le pacte migratoire est DISTINCT du pacte commercial (canal séparé)",
           !diplo_trade_pact(dp,A,B));
        diplo_set_migration_pact(dp,A,B,false);
        ok("le pacte se ROMPT des deux côtés",
           !diplo_migration_pact(dp,A,B) && !diplo_migration_pact(dp,B,A));
    } else ok("(monde trop petit pour le test de pacte migratoire)", true);

    /* ── INVARIANT ANTI-MODIFICATEUR (pipeline diplo, valeur subjective) ────────────
     * Deux empires regardent le MÊME grenier : l'AFFAMÉ (runway food court → stress haut) le
     * valorise HAUT, le REPU (runway long → stress nul) s'en tient au prix OBJECTIF. Mêmes
     * valeurs = échec (cela voudrait dire une hiérarchie de criticité CODÉE, pas émergente). */
    printf("\n── Valeur SUBJECTIVE de province (besoin ⇒ valeur, pas de hiérarchie codée) ──\n");
    {
        int fr=-1;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].raw_cap[RES_GRAIN]>0.5f){ fr=r; break; }
        if (fr<0){ fr=0; econ->region[0].raw_cap[RES_GRAIN]=8.f; }
        econ->region[fr].price[RES_GRAIN]=econ_base_price(RES_GRAIN)*2.f;   /* grain RARE (cher) */
        EconForecast hungry, sated;
        memset(&hungry,0,sizeof hungry); memset(&sated,0,sizeof sated);
        for (int g=0;g<RES_COUNT;g++){ hungry.runway[g]=100.f; sated.runway[g]=100.f; }
        hungry.runway[RES_GRAIN]=2.f;     /* AFFAMÉ : le grain manque dans 2 ans */
        float v_hungry=ai_province_value(econ, player, fr, &hungry);
        float v_sated =ai_province_value(econ, player, fr, &sated);
        float objective=diplo_province_price(econ, fr);
        printf("   grenier (rég %d) : affamé=%.1f vs repu=%.1f (objectif=%.1f)\n", fr, v_hungry, v_sated, objective);
        ok("l'AFFAMÉ valorise le grenier PLUS HAUT que le REPU (valeur subjective émergente)",
           v_hungry > v_sated + 1.f);
        ok("le covet du REPU est BIEN MOINDRE que celui de l'affamé (il s'éteint avec le runway long)",
           (v_sated - objective) < 0.3f*(v_hungry - objective));
    }

    /* ── VASSALITÉ SUR LA DURÉE (pipeline diplo étage 3) ──────────────────────────────
     * La VALEUR cible, l'ÉTHOS décide la MÉTHODE. On vérifie : (a) la cicatrice d'annexion est
     * une COORDONNÉE surfacée (fléau, demo_bonus=0 — pas un modificateur de croissance plat) ;
     * (b) l'intégration MONTE à la paix mais LENTEMENT (sur la durée) ; (c) un maître ANNEXEUR
     * DIGÈRE le vassal intégré (processus → transfert + cicatrice douce + mort du vassal). */
    printf("\n── Vassalité sur la durée : intégration · contribution · digestion (étage 3) ──\n");
    {
        /* (a) la cicatrice d'annexion = COORDONNÉE de STABILITÉ surfacée, PAS un bonus de natalité. */
        int r0 = (cap_reg>=0)?cap_reg:0;
        econ->region[r0].annex_scar = 0.5f;
        ProvModHit hits[PMOD_COUNT]; int nh = provmod_collect(&econ->region[r0], hits, PMOD_COUNT);
        bool found=false; float db=-1.f, inten=-1.f;
        for(int i=0;i<nh;i++) if(hits[i].kind==PMOD_ANNEX_SCAR){ found=true; db=hits[i].demo_bonus; inten=hits[i].intensity; }
        ok("la cicatrice d'annexion est SURFACÉE dans le slot MODIFICATEURS (fléau visible)", found);
        ok("elle ne touche PAS la natalité (demo_bonus=0 — c'est la STABILITÉ, lue ailleurs, pas un bonus plat)",
           found && db==0.f);
        ok("son intensité SUIT la plaie (la bande reflète la coordonnée [0..1])", found && inten>0.4f && inten<=1.f);
        econ->region[r0].annex_scar = 0.f;

        /* un vassal V : un pays VIVANT ≠ joueur tenant ≥1 région. */
        int S=player, V=-1;
        for(int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
            if(c==S || w->country[c].role==POLITY_UNCLAIMED) continue;
            int rc=0; for(int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==c) rc++;
            if(rc>0){ V=c; break; }
        }
        if(V<0){ ok("(monde trop petit : pas de vassal disponible)", true); }
        else {
            int capS=cap_reg, cpV=w->country[V].capital_prov, capV=(cpV>=0)?w->province[cpV].region:-1;
            if(capS>=0 && capV>=0){                              /* capitales RAPPROCHÉES (prox≈1) → test net */
                PopCulture cs=econ->region[capS].culture;
                econ->region[capV].culture.valeurs=cs.valeurs;   econ->region[capV].culture.subsistance=cs.subsistance;
                econ->region[capV].culture.parente=cs.parente;   econ->region[capV].culture.religion=cs.religion;
                econ->region[capS].culture.ethos=ETHOS_MERCANTILE;  /* tient-et-traire, NE digère pas */
            }
            /* (b) INTÉGRATION : V vassal de S, à la paix → le lien MÛRIT, lentement. */
            diplo_set_vassal(dp,S,V,CONTRAT_PROTECTORAT);
            dp->v_integration[V]=0.f; dp->v_grief[V]=0.f;
            diplo_suzerainty_tick(dp,w,econ,wp);
            float i1=dp->v_integration[V];
            ok("l'intégration MONTE dès la 1re année de paix", i1>0.f);
            ok("mais LENTEMENT (une année ne mûrit pas le lien : sous le seuil de contribution 0.65)", i1<0.65f);
            for(int t=0;t<25;t++){ dp->v_grief[V]=0.05f; diplo_suzerainty_tick(dp,w,econ,wp); }
            ok("après ~25 ans de paix, le lien a MÛRI (intégration ≥0.6) et reste BORNÉE ≤1",
               dp->v_integration[V]>=0.6f && dp->v_integration[V]<=1.f);
            /* (c) DIGESTION : maître ANNEXEUR + vassal bien intégré (amorcé près du terme) → absorption. */
            if(capS>=0) econ->region[capS].culture.ethos=ETHOS_DOMINATEUR;
            dp->v_integration[V]=0.9f; dp->v_annex[V]=0.95f;
            int annex0=dp->n_annex;
            /* RE-KEY PROVINCE : la digestion paie le trésor de la province représentative de
             * la capitale (econ_region_rep_province, comme diplo_suzerainty_tick le lit). */
            int capSp=(capS>=0)?econ_region_rep_province(econ,capS):-1;
            for(int t=0;t<12 && w->country[V].role!=POLITY_UNCLAIMED;t++){
                dp->v_grief[V]=0.05f;
                if(capSp>=0) econ->prov[capSp].treasury=1.0e9f;   /* de quoi payer la digestion */
                diplo_suzerainty_tick(dp,w,econ,wp);
            }
            /* owner/annex_scar sont désormais posés au grain PROVINCE (econ_region_set_owner) —
             * region[r].owner/.annex_scar ne se re-dérivent qu'au prochain econ_tick (aggregate). */
            int vleft=0; bool scar=false;
            for(int p=0;p<econ->n_prov;p++){
                if(econ->prov[p].owner==V) vleft++;
                if(econ->prov[p].owner==S && econ->prov[p].annex_scar>0.f) scar=true;
            }
            ok("un maître ANNEXEUR DIGÈRE le vassal intégré (annexion-processus aboutie)", dp->n_annex>annex0);
            ok("le vassal digéré DISPARAÎT (role=UNCLAIMED)", w->country[V].role==POLITY_UNCLAIMED);
            ok("les régions de l'ex-vassal passent au maître (plus aucune à V)", vleft==0);
            ok("au moins une région annexée porte une cicatrice DOUCE (la plaie de fierté)", scar);
        }
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n",g_pass,g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w);free(econ);free(net);free(ts);free(wp);free(wl);free(dp);
    return g_fail?1:0;
}
