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
#include "scps_religion.h"  /* P6 : religion_scholar_tick (quotidien dans sim_day) */
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
/* HAMEAUX LIBRES (POLITY_WILD) — télémétrie : hameaux semés · ralliés CULTURELLEMENT · pop
 * moyenne ralliée (l'absorption MILITAIRE passe par les mécanismes de conquête existants). */
long g_wild_spawned=0, g_wild_defected=0; double g_wild_absorb_pop=0.0;
static int  g_wild_contact[SCPS_MAX_REG];   /* années de contact PACIFIQUE par hameau (reset à sim_init) */

/* ── FIL D'ÉVÈNEMENTS (display) — trackers d'OBSERVATION, gatés human_player ≥ 0 (la
 * chronique ne pousse rien). RUNTIME purs (jamais sérialisés — un load re-amorce en
 * silence au 1er passage, comme provlog_modifier_diff) : un diff raté au chargement
 * coûte UNE notification, jamais un état de jeu. ── */
static uint8_t g_feed_war[SCPS_MAX_COUNTRY];      /* en guerre avec le joueur ? (diff → GUERRE/PAIX) */
static int8_t  g_feed_score[SCPS_MAX_COUNTRY];    /* DERNIER score de guerre connu vs c (le verdict de paix) */
static float   g_feed_balafre[SCPS_MAX_REG];      /* balafre_days précédent (montée → PILLAGE) */
static bool    g_feed_primed = false;             /* 1er passage = amorce muette */

/* section WILD du save partagé (v48) : les compteurs de contact SONT du gameplay
 * (le ralliement culturel se déclenche à WILD_DEFECT_YEARS) — sans sérialisation,
 * un chargement en processus frais retardait le ralliement de jusqu'à 8 ans. */
void sim_wild_save(FILE *f){ fwrite(g_wild_contact, sizeof g_wild_contact, 1, f); }
bool sim_wild_load(FILE *f){
    if (fread(g_wild_contact, sizeof g_wild_contact, 1, f)!=1) return false;
    for (int r=0;r<SCPS_MAX_REG;r++)                       /* forge → borne (pas d'index, simple vraisemblance) */
        if (g_wild_contact[r]<0 || g_wild_contact[r]>100000) g_wild_contact[r]=0;
    return true;
}

/* HAMEAUX LIBRES — ralliement CULTUREL (la règle neuve de B4). Un hameau WILD en contact
 * PACIFIQUE soutenu avec l'empire VOISIN s'y RALLIE : owner → empire, après WILD_DEFECT_YEARS
 * d'adjacence à la paix OU dès que sa culture a CONVERGÉ (heritage = celle du voisin, via
 * l'assimilation). Sa pop + sa culture DISTINCTE deviennent minorité dans l'empire →
 * assimilation_tick / off_culture_fraction / xénophile-xénophobe la digèrent (existant). */
static void wild_cultural_tick(Sim *s, World *w){
    WorldEconomy *e=s->econ;
    int defect_years=(int)tune_f("WILD_DEFECT_YEARS", 8.f);
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++){
        RegionEconomy *re=&e->region[r];
        int o=re->owner;
        if (o<0||o>=w->n_countries || w->country[o].role!=POLITY_WILD || !re->colonized){ g_wild_contact[r]=0; continue; }
        int adjc[SCPS_MAX_COUNTRY]; memset(adjc,0,sizeof adjc);
        int best_emp=-1, best_n=0;
        for (int t=0;t<e->n_regions;t++){
            if (!e->adj[r][t]) continue;
            int ot=e->region[t].owner;
            if (ot>=0 && ot<w->n_countries
                && (w->country[ot].role==POLITY_PLAYER || w->country[ot].role==POLITY_ANTAGONIST)
                && ++adjc[ot]>best_n){ best_n=adjc[ot]; best_emp=ot; }
        }
        if (best_emp<0){ g_wild_contact[r]=0; continue; }                       /* pas de voisin empire */
        if (diplo_status(s->dp, o, best_emp)==DIPLO_WAR){ g_wild_contact[r]=0; continue; }  /* la guerre RAZ le compteur */
        g_wild_contact[r]++;
        int cap=w->country[best_emp].capital_prov;
        int cr=(cap>=0&&cap<w->n_provinces)? w->province[cap].region : -1;
        bool converged=(cr>=0 && cr<e->n_regions && re->culture.heritage==e->region[cr].culture.heritage);
        if (g_wild_contact[r]>=defect_years || converged){
            double pop=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop;
            /* T6 — RE-KEY PROVINCE : re->owner est un DÉRIVÉ recalculé par
             * econ_aggregate_regions à CHAQUE tick (capitale, sinon meilleure pop de
             * PROVINCE) ; un simple re->owner=best_emp ici était écrasé au tick suivant
             * (les provinces membres restaient WILD) → le hameau revenait WILD, le
             * contact remûrissait ~WILD_DEFECT_YEARS plus tard, et se rallier À NOUVEAU
             * (boucle : ~23× plus de ralliements que de hameaux semés sur 200 ans, soit
             * ~200/8). Fix : transférer les PROVINCES membres (idiome de la conquête). */
            econ_region_set_owner(e, w, r, best_emp);
            g_wild_contact[r]=0; g_wild_defected++; g_wild_absorb_pop+=pop;
        }
    }
}

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
    /* FIL D'ÉVÈNEMENTS : observer la BATAILLE RANGÉE du joueur (transition HORS de
     * FA_BATTLE au fil des mois — gagnée si l'ost tient encore debout, perdue sinon). */
    int hp_fb = s->human_player;
    int fb_prev_ph  = (hp_fb>=0 && hp_fb<SCPS_MAX_COUNTRY) ? (int)campaign_phase(s->camp, hp_fb) : (int)FA_IDLE;
    int fb_prev_loc = (hp_fb>=0 && hp_fb<SCPS_MAX_COUNTRY && s->camp->army[hp_fb].active) ? s->camp->army[hp_fb].loc : -1;
    for (int month=0; month<12; month++){
        if (month==0) sim_campaign_orders(s, w);            /* les ordres frais : annuels (inchangé) */
        sim_campaign_defense(s, w);                          /* L1 : la défense marche À LA RENCONTRE */
        campaign_tick(s->camp, w, s->econ, s->dp, &s->camp_rng, 365.f/12.f);
        if (hp_fb>=0 && hp_fb<SCPS_MAX_COUNTRY){
            int ph = (int)campaign_phase(s->camp, hp_fb);
            if (fb_prev_ph==(int)FA_BATTLE && ph!=(int)FA_BATTLE){
                bool alive = s->camp->army[hp_fb].active;
                int foe=-1;                                  /* l'adversaire : une armée ENNEMIE au lieu de l'accrochage */
                for (int k=0;k<SCPS_MAX_COUNTRY && foe<0;k++){
                    if (k==hp_fb) continue;
                    const FieldArmy *en=&s->camp->army[k];
                    if (en->loc==fb_prev_loc && diplo_status(s->dp,hp_fb,k)==DIPLO_WAR) foe=k;
                }
                feed_push(alive?FEED_BATTLE_WON:FEED_BATTLE_LOST, hp_fb, foe, fb_prev_loc, 0);
            }
            fb_prev_ph = ph;
            if (s->camp->army[hp_fb].active) fb_prev_loc = s->camp->army[hp_fb].loc;
        }
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
                if (s->human_player>=0 && a->owner==s->human_player)
                    feed_push(FEED_LIBERATED, a->owner, -1, reg, 0);  /* fil display : MA place reprise */
            } else {
                int prev_owner=s->econ->region[reg].owner;
                if (diplo_occupy(s->dp, s->econ, a->owner, reg)){
                    g_tot_occ_posed++;
                    if (s->human_player>=0 && (a->owner==s->human_player || prev_owner==s->human_player))
                        feed_push(FEED_SIEGE_FALLEN, a->owner, prev_owner, reg, 0);  /* victoire de siège / place perdue */
                }
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

/* RECHERCHE — le revenu de SAVOIR est désormais UNIFIÉ (joueur ET IA) via econ_country_savoir :
 * la POP produit la recherche (strates pondérées × bonus bibliothèque). Fin du modèle isolé
 * LaborEcon/tier-de-capitale, clampé 4, joueur-only (bug-gabarit de l'audit éco). Cf. le bloc de
 * recherche dans sim_day. */

/* enfile un ordre joueur (façade) — FIFO bornée ; false si pleine (jamais d'écrasement). */
bool sim_cmd_push(Sim *s, PlayerCmd c){
    if (!s || s->cmd_n >= SCPS_CMDQ_MAX) return false;
    if (c.verb==CMD_NONE || c.verb>=CMD_COUNT) return false;   /* verbe hors domaine : refus net */
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
        /* le DIPLOMATE : les actes diplo passent par UN émissaire — 1 action / 2 mois.
         * Un ordre INVALIDE (cible hors-borne, soi-même, pays mort) ne le fait pas partir ;
         * un ordre arrivé pendant sa tournée est IGNORÉ (l'UI lit scps_diplo_cd et grise). */
        if (c->verb==CMD_DECLARE_WAR || c->verb==CMD_MAKE_PEACE || c->verb==CMD_OFFER_ALLIANCE
         || c->verb==CMD_OFFER_PACT  || c->verb==CMD_OFFER_MIGRATION || c->verb==CMD_EMBARGO){
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || regions_of(s->econ, t)<=0) continue;
            if (s->day < s->diplo_ready_day) continue;
            s->diplo_ready_day = s->day + 60;
        }
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
          /* ── §3 — DIPLO (capstone #26) : le joueur PROPOSE, le vis-à-vis ÉVALUE (ai_consider_offer).
           *    Tout est REVALIDÉ contre l'état courant ; une offre non consentie est silencieusement
           *    sans effet (la membrane lira le statut INCHANGÉ au tick suivant). */
          case CMD_DECLARE_WAR: {
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || w->country[t].role==POLITY_UNCLAIMED) break;
            if (diplo_status(s->dp,p,t)==DIPLO_WAR) break;             /* déjà en guerre */
            if (diplo_truce_days(s->dp,p,t) > 0.f) break;             /* trêve en cours → interdit */
            CasusBelli cb = diplo_casus_belli(w, s->econ, s->wp, s->dp, p, t, RES_NONE);
            if (cb==CB_NONE) cb = CB_TERRITORIAL;                     /* le joueur DÉCLARE : CB par défaut */
            diplo_declare_war_cb(s->dp, p, t, cb);
            break; }
          case CMD_MAKE_PEACE: {
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p) break;
            if (diplo_status(s->dp,p,t)!=DIPLO_WAR) break;
            if (ai_consider_offer(w, s->econ, s->wp, s->dp, s->sc, p, t, OFFER_PEACE))
                diplo_make_peace(s->dp, p, t);                        /* paix BLANCHE si l'autre cède */
            break; }
          case CMD_OFFER_ALLIANCE: {
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || w->country[t].role==POLITY_UNCLAIMED) break;
            if (ai_consider_offer(w, s->econ, s->wp, s->dp, s->sc, p, t, OFFER_ALLIANCE))
                diplo_form_alliance(s->dp, p, t);                     /* … BILATÉRAL : seulement si consenti (#26) */
            break; }
          case CMD_OFFER_PACT: {
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || w->country[t].role==POLITY_UNCLAIMED) break;
            if (ai_consider_offer(w, s->econ, s->wp, s->dp, s->sc, p, t, OFFER_TRADE_PACT))
                diplo_set_trade_pact(s->dp, p, t, true);
            break; }
          case CMD_OFFER_MIGRATION: {  /* BRASSAGE : le pacte migratoire (échange passif de population) */
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || w->country[t].role==POLITY_UNCLAIMED) break;
            if (ai_consider_offer(w, s->econ, s->wp, s->dp, s->sc, p, t, OFFER_MIGRATION))
                diplo_set_migration_pact(s->dp, p, t, true);
            break; }
          case CMD_EMBARGO: {
            int t = c->a[0];
            if (t<0 || t>=w->n_countries || t==p || w->country[t].role==POLITY_UNCLAIMED) break;
            intertrade_order_embargo(p, t, c->a[1]!=0);               /* décret unilatéral (pas d'évaluation) */
            break; }
          /* ── §3 — INTÉRIEUR : les leviers passent par les MÊMES actionneurs que l'IA
           *    (agency/statecraft) ; toute région est REVALIDÉE (∈ [0,n) ET au joueur). ── */
          case CMD_REPRESS: {
            int r=c->a[0];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            agency_order_repress(s->ag, r);
            break; }
          case CMD_ASSIMILATE: {
            int r=c->a[0];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            agency_order_assimilate(s->ag, r, c->a[1]!=0);            /* a[1] = creuset (TECH_INTEGRATION) */
            break; }
          case CMD_PURGE: {
            int r=c->a[0];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            agency_order_purge(s->ag, r);
            break; }
          case CMD_COUNCIL_HIRE: {
            int seat=c->a[0], slot=c->a[1];
            if (seat<0 || seat>=SC_COUNCIL_SEATS || slot<0 || slot>=SC_COUNCIL_CANDS) break;
            statecraft_council_hire(s->sc, p, seat, slot, statecraft_council_gen(s->year));
            break; }
          case CMD_COUNCIL_DISMISS: {
            int seat=c->a[0];
            if (seat<0 || seat>=SC_COUNCIL_SEATS) break;
            statecraft_council_dismiss(s->sc, p, seat);
            break; }
          /* ── §3 — COMMERCE ── */
          case CMD_ROUTE: {
            int ra=c->a[0], rb=c->a[1];
            if (ra<0 || ra>=s->econ->n_regions || rb<0 || rb>=s->econ->n_regions || ra==rb) break;
            if (s->econ->region[ra].owner!=p) break;                 /* on TRACE depuis une région à soi */
            routes_order(s->rn, w, s->econ, ra, rb, c->a[2]!=0);     /* a[2] = maritime */
            break; }
          case CMD_MARKET_BUY: {
            int r=c->a[0], g=c->a[1]; long q=c->a[2]; int tier=c->a[3];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            if (g<=RES_NONE || g>=RES_COUNT || q<=0) break;
            if (tier<0) tier=0; else if (tier>2) tier=2;
            long spent=0; intertrade_market_buy(s->econ, r, (Resource)g, q, tier, &spent);
            break; }
          case CMD_MARKET_SELL: {
            int r=c->a[0], g=c->a[1]; long q=c->a[2]; int tier=c->a[3];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            if (g<=RES_NONE || g>=RES_COUNT || q<=0) break;
            if (tier<0) tier=0; else if (tier>2) tier=2;
            long gained=0; intertrade_market_sell(s->econ, r, (Resource)g, q, tier, &gained);
            break; }
          /* ── §3 — GUERRE (campagne & flotte) : la force = l'ost MOBILISÉ du joueur (host) ── */
          case CMD_CAMPAIGN: {
            int from=c->a[0], tgt=c->a[1];
            if (from<0 || from>=s->econ->n_regions || tgt<0 || tgt>=s->econ->n_regions) break;
            if (s->econ->region[from].owner!=p) break;               /* on LANCE depuis une région à soi */
            campaign_order(s->camp, s->econ, p, from, tgt, &s->host->army[p]);
            break; }
          case CMD_POSTURE: {
            int po=c->a[0]; if (po<0) po=0; else if (po>2) po=2;     /* 0 prudente · 1 standard · 2 agressive */
            campaign_set_posture(s->camp, p, po);
            break; }
          case CMD_REFILL:
            campaign_refill(s->camp, p, s->econ, s->labor);          /* recomplète l'armée de campagne */
            break;
          case CMD_NAVY_BUILD: {
            int h=c->a[0];
            if (h<0 || h>=HULL_COUNT) break;
            navy_order_build(s->navy, w, s->econ, p, (HullType)h);
            break; }
          case CMD_DISBAND:
            warhost_disband(s->host, p);                             /* dissout la réserve levée */
            break;
          /* ── ALLOCATION de main-d'œuvre (onglet province). Tout REVALIDÉ : région ∈ [0,n) ET au
           *    joueur ; poids clampé ; res/bld bornés. Poser un poids ACTIVE l'override (alloc_on=1).
           *    RE-KEY PROVINCE : alloc_* sont PROVINCE-OWNED (miroir, cf. econ_aggregate_regions) —
           *    econ->region[r].alloc_* est un DÉRIVÉ écrasé au prochain econ_tick ; route sur la
           *    province représentative (le joueur cible une région à l'UI, l'écriture VIT à la
           *    province, exactement comme econ_tick la relit). ── */
          case CMD_ALLOC_RAW: {   /* a={region, resource, poids} */
            int r=c->a[0], g=c->a[1], wt=c->a[2];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            if (g<=RES_NONE || g>=RES_PROD_FIRST) break;
            if (wt<0) wt=0; else if (wt>255) wt=255;
            int rp=econ_region_rep_province(s->econ,r); if (rp<0||rp>=s->econ->n_prov) break;
            s->econ->prov[rp].alloc_raw[g]=(uint8_t)wt;
            s->econ->prov[rp].alloc_on=1;
            break; }
          case CMD_ALLOC_BLD: {   /* a={region, bld_type, poids (0=fermé)} */
            int r=c->a[0], b=c->a[1], wt=c->a[2];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            if (b<0 || b>=BLD_TYPE_COUNT) break;
            if (wt<0) wt=0; else if (wt>255) wt=255;
            int rp=econ_region_rep_province(s->econ,r); if (rp<0||rp>=s->econ->n_prov) break;
            s->econ->prov[rp].alloc_bld[b]=(uint8_t)wt;
            s->econ->prov[rp].alloc_on=1;
            break; }
          case CMD_ALLOC_INPUT: {   /* a={region, bld_type, intrant(0/1)} */
            int r=c->a[0], b=c->a[1];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            if (b<0 || b>=BLD_TYPE_COUNT) break;
            int rp=econ_region_rep_province(s->econ,r); if (rp<0||rp>=s->econ->n_prov) break;
            s->econ->prov[rp].bld_input[b]=(c->a[2]!=0)?1:0;
            break; }
          case CMD_ALLOC_AUTO: {   /* a={region} : retour au split AUTO */
            int r=c->a[0];
            if (r<0 || r>=s->econ->n_regions || s->econ->region[r].owner!=p) break;
            int rp=econ_region_rep_province(s->econ,r); if (rp<0||rp>=s->econ->n_prov) break;
            s->econ->prov[rp].alloc_on=0;
            break; }
          /* ── §7 — ENGAGEMENT D'ÂGE : l'IA s'engage auto au lever d'un âge ; le joueur
           *    CHOISIT (gate human_player dans le bloc annuel) — ce verbe est son choix.
           *    Une fois par âge (player_age_engaged retient le dernier engagé). ── */
          case CMD_AGE_ENGAGE: {
            int age = s->ev->ages.last_dawned;
            if (age<0 || s->player_age_engaged==age) break;   /* rien de levé / déjà engagé */
            if (regions_of(s->econ,p)<=0) break;               /* royaume mort : refus net */
            faction_age_engage(w, s->econ, p, age);
            s->player_age_engaged = age;
            break; }
          /* ── COLONISATION (charte) : a[0] = la province CIBLE (vierge). La SOURCE = la
           *    province colonisée du joueur la plus peuplée (miroir du modèle viewer) ;
           *    les PORTES (pop/vivres/cible) vivent dans econ_colonize_province. ── */
          case CMD_COLONIZE: {
            int dst = c->a[0];
            if (dst<0 || dst>=s->econ->n_prov) break;
            int src=-1; float best=-1.f;
            for (int q=0;q<s->econ->n_prov;q++){
                const ProvinceEconomy *pe=&s->econ->prov[q];
                if (pe->owner!=p || !pe->colonized) continue;
                float pp=0.f; for (int k=0;k<CLASS_COUNT;k++) pp+=pe->strata[k].pop;
                if (pp>best){ best=pp; src=q; }
            }
            if (src>=0) econ_colonize_province(s->econ, w, src, dst, p);
            break; }
        }
    }
    s->cmd_n = 0;
}

void sim_day(Sim *s, World *w) {
    provlog_set_year(s->year);   /* l'an courant pour les pushs d'évènements du directeur (display) */
    PROF(PB_AGENCY, agency_advance(s->ag, w, s->econ, s->wl, s->drift, 1));
    sim_cmd_drain(s, w);   /* JOUEUR : ses ordres s'appliquent ICI, après agency_advance, AVANT l'IA (point fixe) */
    econ_colony_day(s->econ, w);   /* chantiers de colonisation JOUEUR (no-op intégral sans chantier → golden) */
    religion_scholar_tick(w, s->econ);   /* P6 : lettrés (quotidien) — Missionnaire répand la foi ; gated (no-op sans foi) */
    /* leviers intérieurs : draine les coûts SCPS différés (purge/mater) vers TechState */
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float ch,fr,hh;
        if (agency_drain_levier_costs(c,&ch,&fr,&hh)){
            s->ts[c].charge+=ch; s->ts[c].fracture+=fr; s->ts[c].H+=hh;
        }
    }
    routes_advance(s->rn, w, s->econ, 1);
    tech_diffusion_refresh(w, s->ts, w->n_countries);   /* REMISE de prix : recompte qui possède quoi (savoir diffusé) */
    PROF(PB_AI, { for (int c=0;c<w->n_countries;c++) if (s->ai_on[c]){
        ai_step(&s->ai[c], w, s->econ, s->wp, s->wl, s->ag, s->rn, s->dp, s->sc, s->day);
        ai_research_step(&s->ai[c], &s->ts[c], w, s->econ, s->rn, s->wp, s->day);  /* l'arbre vivant (S1 : + le commerce) */
    } });
    /* RECHERCHE DU JOUEUR (gate IA-off : l'humain ne reçoit PAS ai_research_step ci-dessus).
     * La cible (CMD_RESEARCH) progresse, payée par l'INCOME SAVOIR de la capitale × rendement
     * des INSTITUTIONS Savoir × CLOCHE DE PROSPÉRITÉ — modèle FIDÈLE au viewer (file de 1 ;
     * coût plein ; jamais un bonus plat). human=-1 ⇒ research_target reste -1 ⇒ NO-OP chronique. */
    if (s->research_target>=0 && s->human_player>=0 && s->human_player<w->n_countries){
        int pl=s->human_player;
        unsigned access = ai_heritage_access(w, s->econ, s->rn, pl);
        if (!tech_can_research(&s->ts[pl], (TechId)s->research_target, access)){
            s->research_target=-1;                              /* plus accessible (acquise / prérequis manquant) */
        } else {
            float savoir = econ_country_savoir(s->econ, pl);     /* SAVOIR : la POP produit (strates × bibliothèque), annuel — la MÊME source que l'IA */
            float yield = tech_research_yield(&s->ts[pl]);       /* institutions Savoir (arbre tech) : ×1..2.5 */
            CountryReadout cr = country_readout(s->wp, s->ts, w, pl);
            float prosp = 0.4f + (float)cr.m_prosperite.value/100.f*1.2f;   /* ×[0.4..1.6] selon la prospérité */
            float metab = 1.f + tune_f("AI_METAB_RES_W",AI_METAB_RES_W)     /* MÉTABOLISATION (Temps 1) : creuset → +recherche */
                              * econ_country_metabolized(w, s->econ, pl);
            s->ts[pl].research_points += (savoir/365.f) * yield * prosp * metab; /* /an → /jour */
            if (s->ts[pl].research_points >= tech_cost((TechId)s->research_target, (float)w->country[pl].n_regions)
                                              * tech_diffusion_mult((TechId)s->research_target)){  /* remise de diffusion */
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
            if (s->ai_on[c]) statecraft_council_ai(s->sc, w, s->econ, w->seed, c, s->year);   /* Q1 : l'IA pourvoit son siège d'éthos (pool de la génération courante) */
        PROF(PB_ECON, econ_tick(s->econ, 1.f/12.f));
        statecraft_tick(s->sc, w, s->econ, s->wp, s->wl, s->dp, s->rn, 30);
        PROF(PB_DEMO, demography_tick(w, s->econ, s->wl, s->drift, 5.f, 5.f, 1.f/12.f));
        labor_resync_pop(s->labor, s->econ);   /* E0.1 : labor RELIT la pop (le monde la possède) */
        religion_refresh_all(s->econ);   /* FOI PAR GROUPE : le culte DOMINANT par région suit les groupes (post-démo) */
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
        PROF(PB_REVOLT, { revolt_scan(s->rs, w, s->econ, s->drift, s->sc, s->dp, s->camp, 30);
        revolt_tick(s->rs, w, s->econ, s->drift, s->wl, s->wp, s->dp, s->camp, s->sc, 30); });
        if (s->rs->last_spawned>=0){
            /* un pays vient de naître : on donne vie (IA) à tout sécessionniste
             * vivant pas encore piloté (plusieurs peuvent éclore le même mois). */
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (s->ai_on[c]) continue;
                if (w->country[c].role==POLITY_ANTAGONIST && w->country[c].capital_prov>=0
                    && regions_of(s->econ,c)>0){
                    s->ai_on[c]=true;
                    ai_actor_init(&s->ai[c], w, s->econ, c, w->seed ^ (uint32_t)(c*2654435761u));
                    /* #26bis — LA MÉMOIRE DU SÉCESSIONNISTE : né d'une guerre civile, le
                     * nouveau pays garde une dent DURABLE contre l'empire père (Flandre vs
                     * France), qui s'estompe sur des années (opinion_mem). Idempotent par
                     * ai_on (on ne passe ici qu'UNE fois par naissance) ; un pays né
                     * autrement (cataclysme §27) n'a pas de bande → pas de rancune. */
                    for (int i=0;i<s->rs->count;i++){
                        const Rebellion *rb=&s->rs->list[i];
                        if (rb->spawned==c && rb->owner!=c){
                            statecraft_on_secession(s->sc, c, rb->owner);
                            break;
                        }
                    }
                }
            }
            /* une SÉCESSION a changé des propriétaires CE mois : resynchroniser, sinon
             * la détection de conquête du mois prochain prendrait l'indépendance pour
             * une invasion (le peuple libéré deviendrait restif envers SON propre État). */
            for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)
                s->prev_owner_mo[r]=s->econ->region[r].owner;
        }
        /* ── LE FIL D'ÉVÈNEMENTS (display) : OBSERVATION d'état mensuelle, gatée JOUEUR —
         *    la chronique (human=-1) ne pousse RIEN (déterminisme/perf intacts par
         *    construction). Amorce MUETTE au 1er passage (pas de spam an-0/au load). ── */
        if (s->human_player>=0){
            int hp=s->human_player;
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){        /* GUERRE / PAIX (diff de statut) */
                uint8_t at=(c!=hp && diplo_status(s->dp,hp,c)==DIPLO_WAR)?1:0;
                if (at){                                                     /* en guerre : suivre le SCORE (le verdict) */
                    float sc=diplo_war_score(s->dp, hp, c);
                    g_feed_score[c]=(int8_t)((sc<-100.f)?-100:(sc>100.f)?100:(int)sc);
                }
                if (g_feed_primed && at!=g_feed_war[c])
                    feed_push(at?FEED_WAR_DECLARED:FEED_PEACE, c, hp, -1,
                              at?0:(int)g_feed_score[c]);                    /* la PAIX porte le score final */
                g_feed_war[c]=at;
            }
            /* RÉVOLTE INCARNÉE (Option B — scps_revolt.c est le SEUL acteur) : une guerre
             * civile a démarré ce scan (rebel_country≥0) → on NOMME le rebelle (a=rebel_
             * country, "Rebelles de X" déjà posé par spawn_rebel_polity). On ne notifie QUE
             * ce qui concerne le joueur (owner==hp) — feed_push re-filtre de toute façon sur
             * le focus (b=hp suffit à passer). La voie legacy (statecraft_revolt_fired,
             * FEED_REVOLT générique) est retirée : statecraft ne fire plus jamais, elle
             * n'aurait plus rien à notifier. */
            for (int i=0, ncw=revolt_new_civilwar_count(); i<ncw; i++){
                int owner=-1, region=-1;
                int rebel=revolt_new_civilwar_at(i, &owner, &region);
                if (rebel<0 || rebel>=w->n_countries) continue;   /* garde : index rebelle valide */
                if (owner!=hp) continue;                          /* ne concerne pas le suivi */
                if (g_feed_primed)
                    feed_push(FEED_REVOLT, rebel, hp, region, 0);  /* a=rebel_country → la façade résout son NOM */
            }
            for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
                const RegionEconomy *re=&s->econ->region[r];
                float b=re->balafre_days;                                    /* PILLAGE : la balafre MONTE */
                if (g_feed_primed && re->owner==hp && b>g_feed_balafre[r]+1.f)
                    feed_push(FEED_PILLAGE,-1,-1,r,0);
                g_feed_balafre[r]=b;
            }
            if (g_feed_primed && s->rs->last_spawned>=0)                     /* SÉCESSION : un pays est né */
                feed_push(FEED_SECESSION, s->rs->last_spawned, hp, -1, 0);  /* b=joueur : nouvelle du monde (passe le focus) */
            g_feed_primed=true;
        }
    }
    if (s->day % 365 == 364) {
        econ_colonize_tick(s->econ, w, s->human_player); econ_migrate_tick(s->econ, w);   /* le JOUEUR essaime à la main (gate IA-off ; human=-1 ⇒ no-op chronique) */
        world_tick(w, s->econ, 1.0f);
        PROF(PB_LEGIT, legitimacy_tick(s->wl, w, s->econ, s->ts));
        trade_network_build(s->net, w, s->econ);
        {   /* C5 — pré-calcul des liens bloqués par la GUERRE (la diplo vit ICI ; scps_trade reste
             * feuille). Un lien trans-frontière (owners ≠) en guerre sans pacte marchand est coupé. */
            static uint8_t tblk[TRADE_MAX_LINKS];
            int nl = s->net->n_links; if (nl>TRADE_MAX_LINKS) nl=TRADE_MAX_LINKS;
            for (int li=0; li<nl; li++){
                const TradeLink *lk=&s->net->link[li];
                int ca=(lk->ra>=0&&lk->ra<s->econ->n_regions)? s->econ->region[lk->ra].owner : -1;
                int cb=(lk->rb>=0&&lk->rb<s->econ->n_regions)? s->econ->region[lk->rb].owner : -1;
                tblk[li]=(ca>=0&&cb>=0&&ca!=cb&&diplo_status(s->dp,ca,cb)==DIPLO_WAR&&!diplo_trade_pact(s->dp,ca,cb))?1:0;
            }
            trade_tick(s->econ, s->net, tblk);
        }
        PROF(PB_INTERTRADE, intertrade_tick(s->econ, s->rn, s->dp));   /* grandes routes marchandes (goods inter-pays + embargo) */
        PROF(PB_CONTACT, demography_contact_tick(s->econ, s->drift, s->rn, s->dp, 5.f, 5.f, 1.f));   /* S2 : la cristallisation suit le contact (annuel) */
        demography_migration_pact_tick(s->econ, s->dp);   /* BRASSAGE : échange passif de population entre alliés (annuel) */
        demography_refugee_tick(w, s->econ, s->dp);        /* BRASSAGE : la guerre fait FUIR, l'apaisement fait RESPIRER (annuel) */
        wild_cultural_tick(s, w);   /* HAMEAUX LIBRES (B4) : ralliement culturel des hameaux WILD au voisin */
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
        statecraft_council_age_tick(s->sc, w->seed, s->year);    /* LES ANNÉES PASSENT : les conseillers vieillissent, la retraite vide le siège */
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
    /* CRÉATEUR DE CULTURE : l'héritage du JOUEUR (sa lignée → noms & couleur de son pays).
     * Inactif (chronique/bancs/déterminisme) ⇒ culture_player_heritage() ≡ HERITAGE_ADAPTATIF. */
    worldgen_seed_peoples(w, s->econ, culture_player_heritage());
    legitimacy_init(s->wl, w, s->econ); prosperity_init(s->wp, w);
    trade_network_build(s->net, w, s->econ);
    statecraft_init(s->sc, w); agency_init(s->ag); diplo_init(s->dp); routes_init(s->rn);
    diplo_seed_rng(s->dp, w->seed);   /* la fronde tire sa graine du monde (séquence par sim) */
    intertrade_reset();   /* embargos décrétés + flux inter-pays : RAZ par sim */
    provlog_reset();      /* journal provincial : RAZ par sim (runtime, hors save) */
    demography_contact_reset();   /* S2 : compteur de cristallisations culturelles par contact */
    demography_migration_pact_reset();   /* BRASSAGE : compteur de flux de pacte migratoire */
    demography_refugee_reset();   /* BRASSAGE : compteurs de fuite/retour de réfugiés */
    religion_reset();     /* RELIGION : monde ATHÉE à chaque sim (sinon les foi FUITENT entre sims) */
    { int ne=0; for (int c=0;c<w->n_countries;c++){ int rl=w->country[c].role;
          if (rl==POLITY_PLAYER||rl==POLITY_ANTAGONIST) ne++; }
      religion_set_empire_ref(ne); }   /* plafond ⌈N/3⌉ ANCRÉ au compte d'empires de genèse */
    /* HAMEAUX LIBRES : RAZ du compteur de contact (par sim) + recensement des hameaux semés. */
    memset(g_wild_contact, 0, sizeof g_wild_contact);
    /* FIL D'ÉVÈNEMENTS : trackers d'observation remis à plat (l'anneau lui-même est
     * RAZ par provlog_reset ci-dessus) — le 1er passage ré-amorce en silence. */
    memset(g_feed_war, 0, sizeof g_feed_war);
    memset(g_feed_score, 0, sizeof g_feed_score);
    memset(g_feed_balafre, 0, sizeof g_feed_balafre);
    g_feed_primed = false;
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
        int o=s->econ->region[r].owner;
        if (o>=0 && o<w->n_countries && w->country[o].role==POLITY_WILD && s->econ->region[r].colonized)
            g_wild_spawned++;
    }
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
    /* GAMEPLAY — argile + pierre GARANTIES dans le rayon 1-2 de la capitale joueur (force si le biome
     * n'en a pas donné) : la construction ne doit jamais être hors de portée. */
    econ_guarantee_player_construction(s->econ, w, s->player);
    s->human_player = -1;   /* aucun humain par DÉFAUT (la chronique reste 100 % IA) ; la façade débraye après coup */
    s->cmd_n = 0;           /* journal de commandes joueur : vide (la chronique n'enfile jamais) */
    s->research_target = -1;   /* aucune cible de recherche joueur (la chronique n'en pose jamais ⇒ bloc no-op) */
    s->player_age_engaged = -1;   /* §7 : aucun âge engagé par le joueur */
    s->diplo_ready_day = 0;       /* l'émissaire est disponible dès l'an 0 */
    /* PAS DE JOUEUR HUMAIN dans la chronique : TOUT pays habitable est piloté par
     * l'IA — y compris l'ex-emplacement « joueur ». Sinon ce pays restait inerte
     * (il ne bâtissait rien, ne se défendait pas) et FAUSSAIT le balayage (un trou
     * mort sur la carte). Le LaborEcon reste calé sur s->player (modèle isolé : il
     * ne nourrit pas l'éco partagée, les capitales agissent via capitale_* en direct). */
    for (int c=0;c<w->n_countries;c++){
        /* HAMEAUX LIBRES (POLITY_WILD) : PASSIFS — aucun acteur IA (ils ne conquièrent, ne
         * colonisent, ne recherchent JAMAIS ; ils défendent et se laissent rallier). */
        s->ai_on[c] = (w->country[c].role!=POLITY_UNCLAIMED
                       && w->country[c].role!=POLITY_WILD
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
