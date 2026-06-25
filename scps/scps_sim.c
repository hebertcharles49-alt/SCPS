#define _POSIX_C_SOURCE 199309L   /* PROF : clock_gettime/CLOCK_MONOTONIC visibles sous -std=c99 strict */
/*
 * scps_sim.c — le TICK DE JEU PARTAGÉ (voir scps_sim.h).
 *
 * Code DÉPLACÉ VERBATIM depuis chronicle.c (PROF, regions_of, sim_campaign_*,
 * sim_day, sim_init) : la chronique inclut désormais scps_sim.h et appelle ce
 * cœur ; son hash de déterminisme est INCHANGÉ (`make determinism`). La façade
 * scps_api roule le MÊME sim_day → le monde Godot vit pleinement (IA, guerre,
 * diplo, prospérité, endgame), plus seulement la colonne économique.
 */
#include "scps_sim.h"
#include "scps_readout.h"   /* RECHERCHE JOUEUR : la cloche de prospérité (country_readout), FIDÈLE au viewer */
#include "scps_provlog.h"   /* journal d'évènements provincial (display ; runtime, hors déterminisme) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ── PROFILER DE BOUCLE (OFF par défaut ; SCPS_PROF=1) ─────────────────────────
 * Classe les blocs de sim_day par temps CPU. Zéro coût éteint : PROF se réduit à
 * `stmt` quand g_prof_on=0 → le hash de déterminisme reste INCHANGÉ sans la var. */
typedef enum { PB_AGENCY,PB_AI,PB_EVENTS,PB_NAVY_J,PB_ECON,PB_DEMO,PB_NAVY_M,PB_BUILD,
               PB_REVOLT,PB_LEGIT,PB_INTERTRADE,PB_CONTACT,PB_PROSP,PB_WARHOST,PB_CAMPAGNE,
               PB_COUNT } ProfBlock;
static const char *PB_NAME[PB_COUNT]={"agency","ai","events","navy_j","econ","demo","navy_m",
    "build","revolt","legit","intertrade","contact","prosp","warhost","campagne"};
static double g_prof[PB_COUNT], g_prof_prev[PB_COUNT];
static int g_prof_on=-1;
static inline double prof_now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec*1000.0 + t.tv_nsec/1e6; }
#define PROF(blk, stmt) do{ if(g_prof_on<0) g_prof_on=getenv("SCPS_PROF")?1:0; \
    if(g_prof_on){ double _t0=prof_now(); stmt; g_prof[blk]+=prof_now()-_t0; } else { stmt; } }while(0)
static void prof_flush(int year){ if(g_prof_on<=0) return;
    double dt=0,ct=0; for(int i=0;i<PB_COUNT;i++){ ct+=g_prof[i]; dt+=g_prof[i]-g_prof_prev[i]; }
    fprintf(stderr,"[PROF an %d] annee %.0f ms (cumul %.0f) |",year,dt,ct);
    for(int i=0;i<PB_COUNT;i++){ double d=g_prof[i]-g_prof_prev[i];
        if(d>0.5) fprintf(stderr," %s %.0f",PB_NAME[i],d); }
    fprintf(stderr,"\n"); for(int i=0;i<PB_COUNT;i++) g_prof_prev[i]=g_prof[i]; }

/* télémétrie partagée (la chronique les lit ; déclarées extern dans le header) */
long g_tot_occ_posed=0, g_tot_occ_lifted=0;
long g_peak_u[U_COUNT];   /* FORGEDIAG : pic d'effectif debout par type d'unité (sur tout le siècle, pas le seul snapshot) */

int regions_of(const WorldEconomy *e, int c){
    int n=0; for (int r=0;r<e->n_regions;r++) if (e->region[r].owner==c) n++; return n;
}

static void sim_campaign_defense(Sim *s, World *w) {
    (void)w;
    for (int k=0; k<SCPS_MAX_COUNTRY; k++) {
        const FieldArmy *en=&s->camp->army[k];
        if (!en->active || en->phase!=FA_SIEGE) continue;
        if (en->loc<0 || en->loc>=s->econ->n_regions) continue;
        int def=s->econ->region[en->loc].owner;
        if (def<0 || def>=SCPS_MAX_COUNTRY || def==en->owner) continue;
        if (def==s->human_player) continue;   /* la sortie défensive du JOUEUR est SA décision (gate IA-off ; human=-1 ⇒ no-op chronique) */
        if (diplo_status(s->dp,def,en->owner)!=DIPLO_WAR) continue;
        if (campaign_active(s->camp,def) && campaign_phase(s->camp,def)!=FA_IDLE){
            campaign_redirect(s->camp, s->econ, s->dp, def, en->loc);     /* on déroute l'armée en route */
        } else if (warhost_units(s->host,def)>0){
            campaign_order(s->camp, s->econ, def, en->loc, en->loc, &s->host->army[def]);  /* la sortie */
        }
    }
}

static void sim_campaign_orders(Sim *s, World *w) {
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++) {
        if (c==s->human_player) continue;   /* le JOUEUR projette son armée À LA MAIN (gate IA-off ; human=-1 ⇒ no-op chronique) */
        if (campaign_active(s->camp,c) && campaign_phase(s->camp,c)!=FA_IDLE) continue; /* déjà en route */
        if (warhost_units(s->host, c) <= 0) continue;                                   /* rien à projeter */
        int frontier=-1, target=-1;
        /* B5 — PRIORITÉ DE LIBÉRATION : une de mes régions tenue par un occupant
         * ENNEMI (occupier[r] hostile) est la cible n°1. J'y marche depuis une
         * région voisine que je tiens ENCORE (et qui n'est pas elle-même occupée) :
         * le siège mené à terme y LÈVE l'occupation (récolte plus bas → diplo_liberate).
         * Sans ça, les armées ne ciblaient que l'offensive → 1100 occupations posées
         * pour 1-4 levées : le sol repris ne l'était jamais par les armes. */
        for (int r=0; r<s->econ->n_regions && frontier<0; r++) {
            if (s->econ->region[r].owner!=c) continue;
            int occ=s->dp->occupier[r];
            if (occ<0 || occ==c || diplo_status(s->dp,c,occ)!=DIPLO_WAR) continue;
            for (int sn=0; sn<s->econ->n_regions; sn++) {
                if (!s->econ->adj[r][sn]) continue;
                if (s->econ->region[sn].owner!=c || s->dp->occupier[sn]>=0) continue;
                frontier=sn; target=r; break;                       /* libérer MA région */
            }
        }
        /* sinon : une frontière chaude (région ENNEMIE adjacente — l'offensive). */
        for (int r=0; r<s->econ->n_regions && frontier<0; r++) {
            if (s->econ->region[r].owner!=c) continue;
            for (int sn=0; sn<s->econ->n_regions; sn++) {
                if (!s->econ->adj[r][sn]) continue;
                int ob=s->econ->region[sn].owner;
                if (ob<0 || ob==c || diplo_status(s->dp,c,ob)!=DIPLO_WAR) continue;
                /* P3/doctrine — on n'attaque qu'avec un AVANTAGE DE FORCE (≥1.2× le
                 * défenseur) : sinon l'assaut s'use sur le relief et la guerre tourne à
                 * vide. (La LIBÉRATION de NOTRE sol, plus haut, n'est PAS soumise au seuil.) */
                if ((float)warhost_units(s->host,c) < tune_f("BT_ATK_RATIO",1.2f)*(float)warhost_units(s->host,ob)) continue;
                frontier=r; target=sn; break;                       /* une frontière chaude OÙ l'on PÈSE */
            }
        }
        if (frontier>=0){
            campaign_order(s->camp, s->econ, c, frontier, target, &s->host->army[c]);
        } else {
            /* pas de frontière TERRESTRE : la guerre passe la mer si un port, des
             * transports et un chemin existent (mer §6/§8 — contraint par le champ). */
            int port=navy_best_port(w,s->econ,c);
            if (port>=0 && navy_transport_packets_free(s->navy,c)>0){
                int tgt=-1;
                for (int r2=0;r2<s->econ->n_regions && tgt<0;r2++){
                    int ob=s->econ->region[r2].owner;
                    if (ob<0||ob==c||diplo_status(s->dp,c,ob)!=DIPLO_WAR) continue;
                    if (!s->econ->region[r2].coastal) continue;
                    tgt=r2;
                }
                if (tgt>=0)
                    campaign_order_sea(s->camp, w, s->econ, s->navy, c, port, tgt, &s->host->army[c]);
            }
        }
    }
}

static void sim_campaign_year(Sim *s, World *w) {
    /* L1 — la campagne RESPIRE AU MOIS : la défense intercepte en route, la récolte
     * tombe au fil de l'an et l'attaquant re-cible sans attendre janvier. (Le test
     * de paires de campaign_tick s'évalue désormais 12×/an — deux armées qui se
     * croisent se TROUVENT ; l'ordre frais de projection reste annuel.) */
    for (int month=0; month<12; month++){
        if (month==0) sim_campaign_orders(s, w);            /* les ordres frais : annuels (inchangé) */
        sim_campaign_defense(s, w);                          /* L1 : la défense marche À LA RENCONTRE */
        campaign_tick(s->camp, w, s->econ, s->dp, &s->camp_rng, 365.f/12.f);
        campaign_release_transports(s->camp, s->navy);       /* les transports rentrent à la rade */
        /* RÉCOLTE (couche sim) : chaque siège mené à terme (taken_region) pose une
         * OCCUPATION réelle (région ennemie tenue) ou LIBÈRE (notre région reprise). La
         * propriété ne bascule qu'à la paix (diplo_settle) ; la campagne est restée lectrice. */
        for (int i=0; i<w->n_countries && i<SCPS_MAX_COUNTRY; i++){
            FieldArmy *a=&s->camp->army[i];
            if (a->taken_region<0) continue;
            int reg=a->taken_region; a->taken_region=-1;
            if (reg<0 || reg>=s->econ->n_regions) continue;
            if (s->econ->region[reg].owner==a->owner){
                if (s->dp->occupier[reg]>=0) g_tot_occ_lifted++;   /* une occupation réellement levée */
                diplo_liberate(s->dp, s->econ, reg);
            } else {
                if (diplo_occupy(s->dp, s->econ, a->owner, reg)) g_tot_occ_posed++;
            }
            /* L1 — L'ATTAQUANT NE DORT PAS : après la prise, re-cibler — l'armée
             * ennemie qui assiège NOTRE sol d'abord, sinon la frontière suivante. */
            int ntgt=-1;
            for (int k=0;k<SCPS_MAX_COUNTRY && ntgt<0;k++){
                const FieldArmy *en=&s->camp->army[k];
                if (!en->active || en->phase!=FA_SIEGE || en->owner==a->owner) continue;
                if (en->loc<0 || en->loc>=s->econ->n_regions) continue;
                if (s->econ->region[en->loc].owner!=a->owner) continue;
                if (diplo_status(s->dp,a->owner,en->owner)!=DIPLO_WAR) continue;
                ntgt=en->loc;
            }
            for (int sn=0; sn<s->econ->n_regions && ntgt<0; sn++){
                if (!s->econ->adj[reg][sn]) continue;
                int ob=s->econ->region[sn].owner;
                if (ob<0||ob==a->owner||diplo_status(s->dp,a->owner,ob)!=DIPLO_WAR) continue;
                if (s->dp->occupier[sn]==a->owner) continue;        /* déjà tenue : au suivant */
                ntgt=sn;
            }
            if (ntgt>=0 && a->owner!=s->human_player) campaign_redirect(s->camp, s->econ, s->dp, a->owner, ntgt);   /* le JOUEUR re-cible à la main (gate IA-off ; human=-1 ⇒ no-op chronique) */
        }
    }
}

/* RECHERCHE JOUEUR — revenu SAVOIR mensuel de la capitale (réplique le viewer :
 * player_savoir_income_month). Le LaborEcon est calé sur s->player (== s->human_player
 * dans la façade) → prov[0] EST la capitale du joueur. Lecture PURE du LaborEcon. */
static float sim_player_savoir_month(const LaborEcon *lab){
    if (!lab || lab->n_prov<1) return 0.f;
    const LProvince *cap=&lab->prov[0];
    int ct=cap->cap_tier; if(ct<1)ct=1; if(ct>4)ct=4;
    float m = 0.5f*(float)ct;                                   /* la capitale : ses nobles/lettrés */
    for (int b=0;b<cap->n_bld;b++){
        const LBuilding *bd=&cap->bld[b];
        if (bd->type==LB_NONE) continue;
        int slots=building_job_slots(bd->level); if(slots<1)slots=1;
        float staffing=(float)bd->jobs_filled/(float)slots;
        staffing = staffing<0.f ? 0.f : (staffing>1.f ? 1.f : staffing);
        int tier=bd->level+1; if(tier<1)tier=1; if(tier>4)tier=4;
        m += 0.5f*(float)tier*staffing;                         /* 0.5·tier /mois, au prorata */
    }
    return m;
}

/* enfile un ordre joueur (façade) — FIFO bornée ; false si pleine (jamais d'écrasement). */
bool sim_cmd_push(Sim *s, PlayerCmd c){
    if (!s || s->cmd_n >= SCPS_CMDQ_MAX) return false;
    if (c.verb==CMD_NONE || c.verb>CMD_RESEARCH) return false;   /* verbe hors domaine : refus net */
    s->cmdq[s->cmd_n++] = c;
    return true;
}

/* VIDE le journal de commandes du JOUEUR au point FIXE du tick. Chaque ordre est
 * REVALIDÉ contre l'état COURANT avant dispatch (miroir save_sane : un index périmé
 * — région perdue, edifice/unité hors domaine — est ignoré, jamais déréférencé) puis
 * appliqué par le MÊME actionneur que l'IA (agency/warhost). Drain → file remise à 0.
 * cmd_n=0 (chronique) ⇒ no-op total : aucun état touché, aucun RNG, hash INCHANGÉ. */
static void sim_cmd_drain(Sim *s, World *w){
    int p = s->human_player;
    if (p < 0 || p >= w->n_countries){ s->cmd_n = 0; return; }   /* pas d'humain : on jette (sécurité) */
    for (int i=0; i<s->cmd_n; i++){
        const PlayerCmd *c = &s->cmdq[i];
        switch (c->verb){
          case CMD_BUILD: {
            int e = c->a[0];
            if (e<0 || e>=EDIFICE_COUNT) break;
            int cp  = w->country[p].capital_prov;
            int cap = (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;
            int reg = (c->a[1] >= 0) ? c->a[1] : cap;            /* a[1]<0 ⇒ capitale par défaut */
            if (reg<0 || reg>=s->econ->n_regions) break;
            if (s->econ->region[reg].owner != p) break;          /* REVALIDE : la région est-elle encore au joueur ? */
            agency_build_acct(s->ag, s->econ, w, reg, (Edifice)e, p);
            break; }
          case CMD_RECRUIT: {
            int u = c->a[0]; long n = (c->a[1] > 0) ? c->a[1] : 1;
            if (u<0 || u>=U_COUNT) break;
            warhost_player_recruit(s->host, w, s->econ, &s->ts[p], p, (UnitType)u, n);
            break; }
          case CMD_SET_LEVY:
            warhost_set_levy(s->host, p, c->a[0]);
            break;
          case CMD_RESEARCH: {
            int t = c->a[0];
            if (t<0 || t>=TECH_COUNT){ s->research_target = -1; break; }   /* a[0]<0 ⇒ annuler la cible */
            s->research_target = t;   /* file de 1 : la progression/déblocage se fait au tick (bloc sim_day) */
            break; }
        }
    }
    s->cmd_n = 0;
}

void sim_day(Sim *s, World *w) {
    provlog_set_year(s->year);   /* l'an courant pour les pushs d'évènements du directeur (display) */
    PROF(PB_AGENCY, agency_advance(s->ag, w, s->econ, s->wl, s->drift, 1));
    sim_cmd_drain(s, w);   /* JOUEUR : ses ordres s'appliquent ICI, après agency_advance, AVANT l'IA (point fixe) */
    /* leviers intérieurs : draine les coûts SCPS différés (purge/mater) vers TechState */
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float ch,fr,hh;
        if (agency_drain_levier_costs(c,&ch,&fr,&hh)){
            s->ts[c].charge+=ch; s->ts[c].fracture+=fr; s->ts[c].H+=hh;
        }
    }
    routes_advance(s->rn, w, s->econ, 1);
    PROF(PB_AI, { for (int c=0;c<w->n_countries;c++) if (s->ai_on[c]){
        ai_step(&s->ai[c], w, s->econ, s->wp, s->wl, s->ag, s->rn, s->dp, s->day);
        ai_research_step(&s->ai[c], &s->ts[c], w, s->econ, s->rn, s->wp, s->day);  /* l'arbre vivant (S1 : + le commerce) */
    } });
    /* RECHERCHE DU JOUEUR (gate IA-off : l'humain ne reçoit PAS ai_research_step ci-dessus).
     * La cible (CMD_RESEARCH) progresse, payée par l'INCOME SAVOIR de la capitale × rendement
     * des INSTITUTIONS Savoir × CLOCHE DE PROSPÉRITÉ — modèle FIDÈLE au viewer (file de 1 ;
     * coût plein ; jamais un bonus plat). human=-1 ⇒ research_target reste -1 ⇒ NO-OP chronique. */
    if (s->research_target>=0 && s->human_player>=0 && s->human_player<w->n_countries){
        int pl=s->human_player;
        unsigned access = ai_race_access(w, s->econ, s->rn, pl);
        if (!tech_can_research(&s->ts[pl], (TechId)s->research_target, access)){
            s->research_target=-1;                              /* plus accessible (acquise / prérequis manquant) */
        } else {
            float pop   = ai_country_population(w, s->econ, pl);
            float month = sim_player_savoir_month(s->labor);    /* capital par tier × staffing */
            float yield = tech_research_yield(&s->ts[pl]);       /* institutions Savoir : ×1..2.5 */
            CountryReadout cr = country_readout(s->wp, s->ts, w, pl);
            float prosp = 0.4f + (float)cr.m_prosperite.value/100.f*1.2f;   /* ×[0.4..1.6] selon la prospérité */
            s->ts[pl].research_points += (month/30.4f) * yield * prosp;     /* /mois → /jour */
            if (s->ts[pl].research_points >= tech_cost((TechId)s->research_target, pop)){
                tech_research(&s->ts[pl], (TechId)s->research_target, access);   /* DÉBLOQUÉ */
                s->ts[pl].research_points = 0.f; s->research_target=-1;          /* file de 1 : terminé */
            }
        }
    }
    PROF(PB_EVENTS, world_events_tick(s->ev, w, s->econ, s->wl, s->wp, s->sc, s->rn, s->ts, s->dp, 1));
    labor_tick(s->labor);
    /* navy_tick (chantier + entretien) est passé MENSUEL (bloc plus bas) : il pesait ~½ du coût/an
     * en quotidien, et il est pleinement dt-scalé (rien ne le veut au jour). */
    /* — mensuel : économie + réputation diplomatique (O(n²)) + démographie — */
    if (s->day % 30 == 29) {
        econ_apply_country_tech(s->econ, s->ts, SCPS_MAX_COUNTRY);  /* §B1 : techs de prod du pays → prod_mult région */
        statecraft_council_apply(s->sc, w, s->econ, w->seed, 1.f/12.f);  /* Q1 : le Conseil pousse ses ×, paie son or */
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
            if (s->ai_on[c]) statecraft_council_ai(s->sc, w, s->econ, w->seed, c);   /* Q1 : l'IA pourvoit son siège d'éthos */
        PROF(PB_ECON, econ_tick(s->econ, 1.f/12.f));
        statecraft_tick(s->sc, w, s->econ, s->wp, s->wl, s->dp, s->rn, 30);
        PROF(PB_DEMO, demography_tick(w, s->econ, s->wl, s->drift, 5.f, 5.f, 1.f/12.f));
        labor_resync_pop(s->labor, s->econ);   /* E0.1 : labor RELIT la pop (le monde la possède) */
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)   /* E3 : l'IA stockeuse (mensuel) */
            if (s->ai_on[c]) ai_speculate_tick(&s->ai[c], s->econ);
        /* — conquête du mois : un peuple passé sous une couronne ÉTRANGÈRE devient
         *   restif (intégration à zéro, L au plancher) → terreau de sécession. */
        for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
            int16_t no=s->econ->region[r].owner, po=s->prev_owner_mo[r];
            if (po>=0 && no>=0 && no!=po){
                demography_on_conquest(w, s->econ, s->drift, r, no);
                revolt_on_conquest(s->rs, r);    /* subir la conquête arme le séparatisme (≈10 ans) */
            }
            s->prev_owner_mo[r]=no;
        }
        /* — la révolte INCARNÉE : la misère SOUTENUE d'une région (le pire déficit
         *   de groupe : faim, sur-taxe, aliénation, non-intégration) allume un
         *   soulèvement, puis on tranche (sécession, coup, jacquerie, écrasement).
         *   Un pays NÉ d'une sécession prend vie. */
        PROF(PB_NAVY_M, { navy_tick(s->navy, w, s->econ, s->dp, 30.f);   /* chantier + entretien : MENSUEL (ex-quotidien) */
        navy_colonize_tick(s->navy, w, s->econ, 30.f, s->human_player);   /* mer §8 : on découvre ce que la volta touche (le JOUEUR colonise à la main) */
        navy_course_tick(s->navy, w, s->econ, s->dp, s->rn, &s->camp_rng,
                         -1, 30.f);   /* coques : la course (raids - saignee - blocus - verdicts) */
        navy_interception_tick(s->navy, s->camp, w, s->econ, s->dp, &s->camp_rng); });   /* les convois se chassent */
        /* IA navale FRUGALE (mer §5/§8) : un pays côtier prospère bâtit son port,
         * puis un transport, puis tente la route MARITIME — décision par éthos
         * (les poids fins viendront avec la passe course). */
        PROF(PB_BUILD, { for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
            if (!s->ai_on[c]) continue;
            int hr=s->ai[c].home_region;
            if (hr<0||hr>=s->econ->n_regions) continue;
            RegionEconomy *re=&s->econ->region[hr];
            if (re->owner!=c) continue;
            /* V3/WG — la rade s'ouvre sur la MEILLEURE CÔTE : navy_best_coast LIT l'aptitude
             * portuaire (Region.harbor, la FORME du littoral) + un appoint de pop + l'avantage
             * de siège — une baie franche peut l'emporter sur un cap capital exposé. */
            int pr=navy_best_coast(w,s->econ,c);
            if (pr>=0 && s->econ->region[pr].build.port<=0.f && s->econ->region[pr].treasury>400.f){
                if (getenv("SCPS_HARBORDIAG")){   /* WG : la rade choisie par aptitude portuaire (vs la région de la capitale) */
                    int cp=w->country[c].capital_prov;
                    int capr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
                    float cap_h=(capr>=0&&capr<w->n_regions)?w->region[capr].harbor:-1.f;
                    bool cap_coast=(capr>=0&&capr<s->econ->n_regions)?s->econ->region[capr].coastal:false;
                    const char *why = (pr==capr)?"" :
                                      (!cap_coast)?"  <- la capitale est enclavee : rade sur la meilleure cote" :
                                      "  <- la FORME l'emporte sur le siege cotier expose";
                    printf("      [HARBOR] pays %d : rade region %d (harbor %.2f) ; region-capitale %d (harbor %.2f, cote=%d)%s\n",
                           c, pr, w->region[pr].harbor, capr, cap_h, cap_coast?1:0, why);
                }
                agency_build(s->ag, s->econ, w, pr, EDI_PORT);
            } else if (navy_best_port(w,s->econ,c)>=0 && s->navy->n[c].build_hull<0){
                if (s->navy->n[c].hull[HULL_TRANSPORT]<2 && re->treasury>500.f)
                    navy_order_build(s->navy, w, s->econ, c, HULL_TRANSPORT);
                else if (s->navy->n[c].hull[HULL_MERCHANT]<1 && re->treasury>700.f)
                    navy_order_build(s->navy, w, s->econ, c, HULL_MERCHANT);
            }
            /* la route maritime : depuis la RADE (le meilleur port, pas forcément la
             * capitale) vers un partenaire PORTÉ d'un autre pays — et la SOBRIÉTÉ :
             * trois liens maritimes au plus par rade. */
            int hp=navy_best_port(w,s->econ,c);
            if (s->day%180==29 && hp>=0){
                int mine=0;
                for (int i=0;i<s->rn->n;i++){
                    const TradeRoute *t=&s->rn->route[i];
                    if (t->maritime && (t->ra==hp||t->rb==hp)) mine++;
                }
                for (int r2=0;r2<s->econ->n_regions && mine<3;r2++){
                    if (s->econ->region[r2].owner==c||s->econ->region[r2].owner<0) continue;
                    if (!navy_region_is_port(w,s->econ,r2)) continue;
                    if (routes_order(s->rn, w, s->econ, hp, r2, true)){ mine++; break; }
                }
            }
        } });
        PROF(PB_REVOLT, { revolt_scan(s->rs, w, s->econ, s->drift, 30);
        revolt_tick(s->rs, w, s->econ, s->drift, s->wl, s->wp, 30); });
        if (s->rs->last_spawned>=0){
            /* un pays vient de naître : on donne vie (IA) à tout sécessionniste
             * vivant pas encore piloté (plusieurs peuvent éclore le même mois). */
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (s->ai_on[c]) continue;
                if (w->country[c].role==POLITY_ANTAGONIST && w->country[c].capital_prov>=0
                    && regions_of(s->econ,c)>0){
                    s->ai_on[c]=true;
                    ai_actor_init(&s->ai[c], w, s->econ, c, w->seed ^ (uint32_t)(c*2654435761u));
                }
            }
            /* une SÉCESSION a changé des propriétaires CE mois : resynchroniser, sinon
             * la détection de conquête du mois prochain prendrait l'indépendance pour
             * une invasion (le peuple libéré deviendrait restif envers SON propre État). */
            for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)
                s->prev_owner_mo[r]=s->econ->region[r].owner;
        }
    }
    if (s->day % 365 == 364) {
        econ_colonize_tick(s->econ, w, s->human_player); econ_migrate_tick(s->econ, w);   /* le JOUEUR essaime à la main (gate IA-off ; human=-1 ⇒ no-op chronique) */
        world_tick(w, s->econ, 1.0f);
        PROF(PB_LEGIT, legitimacy_tick(s->wl, w, s->econ, s->ts));
        trade_network_build(s->net, w, s->econ); trade_tick(s->econ, s->net);
        PROF(PB_INTERTRADE, intertrade_tick(s->econ, s->rn, s->dp));   /* grandes routes marchandes (goods inter-pays + embargo) */
        PROF(PB_CONTACT, demography_contact_tick(s->econ, s->drift, s->rn, s->dp, 5.f, 5.f, 1.f));   /* S2 : la cristallisation suit le contact (annuel) */
        PROF(PB_PROSP, prosperity_tick(s->wp, w, s->econ, s->net, s->ts, s->wl));
        if (s->eg) endgame_tick(s->eg, w, s->econ, s->wp, s->ts, s->rn, s->navy, s->dp, s->camp, s->player, s->year);
        /* DIPLOMATIE annuelle : usure de guerre, FONTE des trêves & du momentum
         * (la guerre peut reprendre après le répit), et le SCORE DE GUERRE (bras-de-fer
         * + attrition qui saigne les armes). */
        PROF(PB_WARHOST, warhost_tick(s->host, w, s->econ, s->dp, s->ts, 1.0f));   /* la mobilisation : les armées vivent */
        PROF(PB_CAMPAGNE, sim_campaign_year(s, w));                           /* … et MARCHENT : campagne sur la carte */
        if (getenv("SCPS_FORGEDIAG")){   /* pic d'effectif par type sur tout le siècle (démasque la démob) */
            long yu[U_COUNT]; memset(yu,0,sizeof yu);
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
                for (int i=0;i<s->host->army[c].n_units;i++){ int ty=s->host->army[c].units[i].type;
                    if (ty>=0&&ty<U_COUNT) yu[ty]+=s->host->army[c].units[i].count; }
            for (int t=0;t<U_COUNT;t++) if (yu[t]>g_peak_u[t]) g_peak_u[t]=yu[t];
        }
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
            diplo_set_faustian(s->dp, c, s->ts[c].charge);  /* souillure faustienne → croisades */
        diplo_tick(s->dp, 365.f);
        credit_year_tick(s->econ, s->wl, w);               /* dette : intérêt annuel (creuse le débiteur, crédite le prêteur) */
        diplo_suzerainty_tick(s->dp, w, s->econ, s->wp);   /* suzeraineté + FRONDE : tributs, ligues, défections */
        diplo_war_tick(s->dp, w, s->econ, s->wp, 1.0f);
        missions_tick(s->missions, w, s->econ, s->ts, s->year);  /* missions décennales : rythme + récompense */
        faction_levers_decay(0.07f);   /* §4 : une stance non entretenue s'efface (~15 ans) */
        if (s->ev->ages.last_dawned != s->prev_dawned){          /* §7 : un âge se lève → engagement */
            int age=s->ev->ages.last_dawned;
            if (age>=0) for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
                if (w->country[c].role!=POLITY_UNCLAIMED && regions_of(s->econ,c)>0
                    && c!=s->human_player)                        /* le JOUEUR choisit lui-même : pas d'engagement auto (human=-1 ⇒ no-op chronique) */
                    faction_age_engage(w, s->econ, c, age);       /* la faction-patronne s'avance (l'IA accepte) */
            s->prev_dawned = s->ev->ages.last_dawned;
        }
        /* JOURNAL PROVINCIAL : diff annuel des modificateurs DYNAMIQUES par région
         * (apparition → entrée). Lecture pure (provmod_collect + years_held) → aucun
         * état sim touché, déterminisme intact. Le 1er passage amorce sans logguer. */
        for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
            const RegionEconomy *re=&s->econ->region[r];
            if (re->owner<0) continue;
            unsigned mask=0;
            if (s->wl->years_held[r] < 5.f) mask |= 1u<<JMOD_CONQUEST;
            ProvModHit ph[8]; int nh=provmod_collect(re, ph, 8);
            for (int h=0;h<nh;h++) switch(ph[h].kind){
                case PMOD_CICATRICE:      mask |= 1u<<JMOD_SCAR;        break;
                case PMOD_ABONDANCE:      mask |= 1u<<JMOD_ABONDANCE;   break;
                case PMOD_FERVEUR:        mask |= 1u<<JMOD_FERVEUR;     break;
                case PMOD_RECONSTRUCTION: mask |= 1u<<JMOD_RECONSTRUCT; break;
                default: break;
            }
            provlog_modifier_diff(r, mask);
        }
        prof_flush(s->year);   /* PROF : classement de l'année (no-op si SCPS_PROF non posé) */
    }
    if (++s->day % 365 == 0) s->year++;
}

void sim_init(Sim *s, World *w) {
    econ_init(s->econ, w); gen_population(w, s->econ);
    worldgen_seed_peoples(w, s->econ, RACE_HUMAIN);
    legitimacy_init(s->wl, w, s->econ); prosperity_init(s->wp, w);
    trade_network_build(s->net, w, s->econ);
    statecraft_init(s->sc, w); agency_init(s->ag); diplo_init(s->dp); routes_init(s->rn);
    diplo_seed_rng(s->dp, w->seed);   /* la fronde tire sa graine du monde (séquence par sim) */
    intertrade_reset();   /* embargos décrétés + flux inter-pays : RAZ par sim */
    provlog_reset();      /* journal provincial : RAZ par sim (runtime, hors save) */
    demography_contact_reset();   /* S2 : compteur de cristallisations culturelles par contact */
    intertrade_seed_centres(w, s->econ);   /* P3.20 : les Centres commerciaux (hubs) — géographiques */
    intertrade_seed_citystate_arms(w, s->econ);   /* F-arc : chaque cité-état naît armurier (manufacture d'armes aléatoire sur son Centre) */
    agency_seed_capital_markets(w, s->econ);   /* DÉPART : chaque empire naît avec un Marché sur sa capitale (carte nue) */
    econ_set_arms_pump(intertrade_market_pull);   /* F-arc : la levée s'arme au marché (propre→Centre cité-état→mondial) */
    /* RAZ PLEINE PLAGE (SCPS_MAX_COUNTRY, pas n_countries) : n_countries GRANDIT par
     * sécession en cours de sim — la sim suivante repart plus bas. Sans ça, les slots
     * hauts gardent ai_on=true + un acteur/TechState PÉRIMÉS d'un autre monde : un pays
     * sécessionniste né à cet index sautait son init (« if (ai_on) continue ») → piloté
     * par un fantôme (home_region d'un ancien monde, cadences mortes, arbre de tech
     * hérité) et sa télémétrie polluait les totaux par sim. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ s->ai_on[c]=false; tech_state_init(&s->ts[c], false); }
    s->player = 0;
    for (int c=0;c<w->n_countries;c++) if (w->country[c].role==POLITY_PLAYER){ s->player=c; break; }
    s->human_player = -1;   /* aucun humain par DÉFAUT (la chronique reste 100 % IA) ; la façade débraye après coup */
    s->cmd_n = 0;           /* journal de commandes joueur : vide (la chronique n'enfile jamais) */
    s->research_target = -1;   /* aucune cible de recherche joueur (la chronique n'en pose jamais ⇒ bloc no-op) */
    /* PAS DE JOUEUR HUMAIN dans la chronique : TOUT pays habitable est piloté par
     * l'IA — y compris l'ex-emplacement « joueur ». Sinon ce pays restait inerte
     * (il ne bâtissait rien, ne se défendait pas) et FAUSSAIT le balayage (un trou
     * mort sur la carte). Le LaborEcon reste calé sur s->player (modèle isolé : il
     * ne nourrit pas l'éco partagée, les capitales agissent via capitale_* en direct). */
    for (int c=0;c<w->n_countries;c++){
        s->ai_on[c] = (w->country[c].role!=POLITY_UNCLAIMED
                       && w->country[c].capital_prov>=0);
        if (s->ai_on[c]) ai_actor_init(&s->ai[c], w, s->econ, c, w->seed ^ (uint32_t)(c*2654435761u));
    }
    ai_ensure_dominator(s->ai, s->ai_on, w->n_countries);   /* §war : un monde tout en alliances reste atone */
    demography_attach(w, s->econ, s->drift);
    demography_dyn_id_rebase(s->econ);   /* compteur de drift_id : repart au socle par sim */
    /* P1 : LIBÉRER le scratch warhost du run précédent AVANT la RAZ — warhost_init re-calloc le scratch ;
     * sans ce free, chaque ré-génère (façade) ou sim enchaîné (chronique) fuyait ~15 Ko. host est calloc'd
     * (scratch NULL au 1er passage → free(NULL) sûr). */
    revolt_init(s->rs); warhost_free(s->host); warhost_init(s->host); missions_init(s->missions);
    credit_init();
    navy_init(s->navy);
    if (s->eg) endgame_init(s->eg);                      /* capstone §27 : RAZ du cataclysme */
    campaign_init(s->camp, w, s->econ);                  /* armées de campagne : table de terrain + RAZ */
    s->camp_rng = w->seed ^ 0xCA117A11u;                 /* graine de campagne, propre à la sim */
    faction_levers_reset();   /* §4 : stances de factions remises à zéro pour cette sim */
    s->prev_dawned=-1;        /* §7 : aucun âge encore traité */
    for (int r=0;r<SCPS_MAX_REG;r++)
        s->prev_owner_mo[r] = (r<s->econ->n_regions)? s->econ->region[r].owner : -1;
    events_init(s->ev, w, w->seed);
    labor_init(s->labor, w); labor_seed_from_world(s->labor, w, s->econ, s->player);
    s->day=0; s->year=0;
}

/* ── allocation des MEMBRES (la façade scps_api en a besoin ; la chronique alloue
 *    inline, à l'identique). Même séquence de malloc que chronicle::main. ─────── */
bool sim_alloc(Sim *s) {
    s->econ=malloc(sizeof(WorldEconomy)); s->wp=malloc(sizeof(WorldProsperity));
    s->wl=malloc(sizeof(WorldLegitimacy)); s->net=malloc(sizeof(TradeNetwork));
    s->ts=calloc(SCPS_MAX_COUNTRY,sizeof(TechState)); s->sc=malloc(sizeof(Statecraft));
    s->ag=malloc(sizeof(AgencyState)); s->ev=malloc(sizeof(EventsState));
    s->drift=malloc(sizeof(ModifierStack)); s->labor=malloc(sizeof(LaborEcon));
    s->dp=malloc(sizeof(DiploState)); s->rn=malloc(sizeof(RouteNetwork));
    s->ai=calloc(SCPS_MAX_COUNTRY,sizeof(AiActor)); s->ai_on=calloc(SCPS_MAX_COUNTRY,sizeof(bool));
    s->rs=malloc(sizeof(RevoltState)); s->host=calloc(1,sizeof(WarHost));   /* P1 : calloc → scratch NULL d'emblée (free sûr) */
    s->missions=malloc(sizeof(MissionsState)); s->camp=malloc(sizeof(Campaign));
    s->navy=malloc(sizeof(NavyState)); s->eg=calloc(1,sizeof(EndgameState));
    return s->econ&&s->wp&&s->wl&&s->net&&s->ts&&s->sc&&s->ag&&s->ev&&s->drift
        &&s->labor&&s->dp&&s->rn&&s->ai&&s->ai_on&&s->rs&&s->host&&s->missions&&s->camp&&s->navy&&s->eg;
}

void sim_free_members(Sim *s) {
    free(s->econ); free(s->wp); free(s->wl); free(s->net); free(s->ts); free(s->sc);
    free(s->ag); free(s->ev); free(s->drift); free(s->labor); free(s->dp); free(s->rn);
    free(s->ai); free(s->ai_on); free(s->rs); warhost_free(s->host); free(s->host); free(s->missions);   /* P1 : scratch warhost */
    free(s->camp); free(s->navy); free(s->eg);
}
