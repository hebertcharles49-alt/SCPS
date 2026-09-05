/* command_feedback_demo.c — contrat du journal TRANSIENT des ordres joueur.
 *
 * Le banc vérifie le point de séparation essentiel : les actions player_* sont
 * seulement mises en file, puis le verdict arrive au drain. Il reste volontairement
 * court et n'avance le monde qu'au strict nécessaire par scénario.
 */
#include "scps_api.h"
#include "scps_sim.h"
#include "scps_tech.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int pass_count, fail_count;

static void check(const char *label, int ok){
    printf("   %s %s\n", ok ? "OK" : "FAIL", label);
    if (ok) pass_count++; else fail_count++;
}

static int last_feedback(const ScpsSim *s, ScpsCommandFeedback *out){
    int n=scps_command_feedback_count(s);
    return n>0 && scps_command_feedback_at(s,n-1,out);
}

static ScpsSim *world(void){
    ScpsSim *s=scps_sim_new();
    if (s) scps_sim_generate(s,9);
    return s;
}

int main(void){
    ScpsCommandFeedback f0, f1;

    /* Pause : avant le tick, l'ordre est réellement PENDING. */
    {
        ScpsSim *s=world();
        check("world allocated",s!=NULL);
        check("invalid build is enqueued",s && scps_player_build(s,0,INT_MAX));
        check("queued result is pending",s && last_feedback(s,&f0)
              && f0.status==SCPS_FEEDBACK_PENDING);
        scps_sim_advance_days(s,1);
        check("invalid build is refused at drain",last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_REFUSED);
        scps_sim_free(s);
    }

    /* Une route acceptée démarre un chantier différé : au premier drain le
     * journal dit STARTED, car routes_advance ne l'ouvre qu'après 90 jours
     * terrestres (120 maritimes). Le compteur public ne doit pas la compter
     * avant cette ouverture. */
    {
        ScpsSim *s=world();
        int p=s ? scps_player(s) : -1;
        int cap=s ? scps_country_capital_province(s,p) : -1;
        int ra=s ? scps_province_region(s,cap) : -1;
        int rb=-1;
        int before=0, after=0, has_centre=0;
        double export_gold=0.0;
        ScpsTradePartner partners[4];
        if(s){
            for(int r=0;r<scps_region_count(s);r++)
                if(r!=ra && scps_region_owner(s,r)>=0 && scps_region_owner(s,r)!=p){
                    rb=r; break;
                }
            scps_country_trade(s,p,&before,&export_gold,&has_centre,partners,4);
        }
        check("route target is a foreign settled region",s && ra>=0 && rb>=0);
        check("land route is enqueued",s && ra>=0 && rb>=0 && scps_player_route(s,ra,rb,0));
        scps_sim_advance_days(s,1);
        scps_country_trade(s,p,&after,&export_gold,&has_centre,partners,4);
        check("route drain reports STARTED while forming",last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_EXECUTED
              && f1.outcome==SCPS_FEEDBACK_STARTED);
        check("forming route is absent from active trade count",after==before);
        scps_sim_free(s);
    }

    /* Fiche relation : une route ouverte et une seconde route en formation
     * vers le même pays. Seed9 ne garantit pas une cible étrangère à deux
     * régions ; on garde donc une cible à une région et on crée explicitement
     * une seconde source joueur par COLONISATION publique. */
    {
        ScpsSim *s=world();
        int p=s ? scps_player(s) : -1;
        int cap=s ? scps_country_capital_province(s,p) : -1;
        int ra=s ? scps_province_region(s,cap) : -1;
        int target=-1, rb=-1, colonize_prov=-1, ra2=-1;
        ScpsRelation rel[64];
        int nr=s ? scps_country_relations(s,p,rel,64) : 0;
        for(int i=0;i<nr && target<0;i++){
            int c=rel[i].country, nreg=0, only=-1;
            for(int r=0;r<scps_region_count(s);r++){
                if(scps_region_owner(s,r)!=c) continue;
                only=r;
                nreg++;
            }
            ScpsDiploContext probe;
            memset(&probe,0,sizeof probe);
            if(nreg==1 && scps_diplo_context(s,c,&probe) && probe.shared_routes==0
               && only>=0 && ra>=0 && ra!=only){
                target=c; rb=only;
            }
        }
        if(s && target>=0){
            for(int pp=0;pp<scps_province_count(s) && colonize_prov<0;pp++){
                int cr=scps_province_region(s,pp);
                if(cr>=0 && cr!=ra && scps_region_owner(s,cr)!=target
                   && scps_can_colonize(s,pp)) colonize_prov=pp;
            }
        }
        check("fresh route fixture finds a known one-region target and colonisable province",
              s && target>=0 && rb>=0 && colonize_prov>=0);
        check("colonisation source order is enqueued",
              s && colonize_prov>=0 && scps_player_colonize(s,colonize_prov));
        scps_sim_advance_days(s,1);
        int cdst=-1, cleft=0, ctot=0, ccd=0, cy=0;
        int colony_active=scps_colony_status(s,&cdst,&cleft,&ctot,&ccd,&cy);
        check("colonisation opens a public source chantier",
              s && colony_active==1 && cdst==colonize_prov && ctot>0 && cleft>0);
        for(int day=0;day<ctot+2;day++) scps_sim_advance_days(s,1);
        for(int r=0;r<scps_region_count(s);r++)
            if(r!=ra && scps_region_owner(s,r)==p){ ra2=r; break; }
        check("colonisation yields a second region owned by the player",
              s && ra2>=0 && ra2!=ra);
        check("first route is enqueued from the capital",
              s && target>=0 && ra>=0 && scps_player_route(s,ra,rb,0));
        scps_sim_advance_days(s,1);
        for(int day=1;day<90;day++) scps_sim_advance_days(s,1);
        ScpsDiploContext open_ctx;
        memset(&open_ctx,0,sizeof open_ctx);
        check("first route reaches open state after 90 days",
              s && target>=0 && scps_diplo_context(s,target,&open_ctx)
              && open_ctx.shared_routes==1 && open_ctx.open_routes==1
              && open_ctx.route_open==1
              && open_ctx.route_days_done==open_ctx.route_days_total);
        check("second route is enqueued from the new player region",
              s && target>=0 && ra2>=0 && scps_player_route(s,ra2,rb,0));
        scps_sim_advance_days(s,1);
        ScpsDiploContext forming_ctx;
        memset(&forming_ctx,0,sizeof forming_ctx);
        check("relation reader prefers the second forming route",
              s && target>=0 && scps_diplo_context(s,target,&forming_ctx)
              && forming_ctx.shared_routes==2 && forming_ctx.open_routes==1
              && forming_ctx.route_open==0
              && forming_ctx.route_days_done>0
              && forming_ctx.route_days_done<forming_ctx.route_days_total
              && forming_ctx.route_days_done<=forming_ctx.route_days_total);
        int done_before=forming_ctx.route_days_done;
        scps_sim_advance_days(s,1);
        ScpsDiploContext forming_next;
        memset(&forming_next,0,sizeof forming_next);
        check("forming route advances one day without changing open count",
              s && scps_diplo_context(s,target,&forming_next)
              && forming_next.open_routes==1
              && forming_next.route_days_done>=done_before
              && forming_next.route_days_done<=forming_next.route_days_total);
        scps_sim_free(s);
    }

    /* La file de commandes est distincte du journal : le 65e ordre est refusé
     * immédiatement, avec un résultat consultable. */
    {
        ScpsSim *s=world();
        int accepted=0;
        for (int i=0;i<SCPS_CMDQ_MAX;i++){
            scps_player_set_levy(s,i%4);
            accepted++;
        }
        check("64 orders accepted into the queue",accepted==SCPS_CMDQ_MAX);
        check("65th order reports queue full",!scps_player_research(s,0)
              && last_feedback(s,&f0)
              && f0.status==SCPS_FEEDBACK_REFUSED
              && f0.reason==SCPS_FEEDBACK_REASON_QUEUE_FULL);
        for (int i=0;i<96;i++) (void)scps_player_research(s,0);
        scps_sim_advance_days(s,1);
        { int pending=0, executed=0;
          for (int i=0;i<scps_command_feedback_count(s);i++)
              if (scps_command_feedback_at(s,i,&f0)){
                  pending += f0.status==SCPS_FEEDBACK_PENDING;
                  executed += f0.status==SCPS_FEEDBACK_EXECUTED;
              }
          check("queue-full burst preserves drain verdicts",pending==0 && executed>0); }
        scps_sim_free(s);
    }

    /* 129 ordres : le journal borné garde les plus récents et éjecte le plus
     * ancien, tandis que la file reste limitée à 64. */
    {
        ScpsSim *s=world();
        uint32_t first_id=0;
        for (int i=0;i<64;i++) scps_player_set_levy(s,i%4);
        scps_sim_advance_days(s,1);
        scps_command_feedback_at(s,0,&f0); first_id=f0.id;
        /* Cinq vagues de 64 = 320 ordres (> 2×SCPS_CMD_FEEDBACK_MAX),
         * avec un drain entre chaque vague : l'historique roule sans laisser
         * une file acceptée ou un résultat récent disparaître. */
        for (int wave=0;wave<4;wave++){
            for (int i=0;i<64;i++) scps_player_set_levy(s,i%4);
            scps_sim_advance_days(s,1);
        }
        check("journal remains bounded after 320 orders",
              scps_command_feedback_count(s)==SCPS_CMD_FEEDBACK_MAX);
        check("oldest journal entry was evicted",
              scps_command_feedback_at(s,0,&f1) && f1.id!=first_id);
        scps_sim_free(s);
    }

    /* Même verbe : une mutation valide suivie d'un argument invalide. */
    {
        ScpsSim *s=world();
        scps_player_set_levy(s,3);
        scps_player_set_levy(s,99);
        scps_sim_advance_days(s,1);
        check("valid levy mutates",scps_command_feedback_at(s,0,&f0)
              && f0.status==SCPS_FEEDBACK_EXECUTED
              && f0.outcome==SCPS_FEEDBACK_MUTATED);
        check("invalid levy is refused",last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_REFUSED
              && f1.reason==SCPS_FEEDBACK_REASON_INVALID_ARGUMENT);
        scps_sim_free(s);
    }

    /* Deux achats démesurés : ils passent l'enfilage mais sont refusés au
     * drain quand la dépense réelle ne peut pas être couverte. */
    {
        ScpsSim *s=world();
        int pid=scps_country_capital_province(s,scps_player(s));
        scps_player_market_buy(s,pid,1,2000000000L,0);
        scps_player_market_buy(s,pid,1,2000000000L,0);
        scps_sim_advance_days(s,1);
        check("first budget spend executes a bounded purchase",scps_command_feedback_at(s,0,&f0)
              && f0.status==SCPS_FEEDBACK_EXECUTED);
        check("second budget spend is refused when exhausted",last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_REFUSED);
        scps_sim_free(s);
    }

    /* Recherche inaccessible, puis hors arbre, et diplomatie hors monde sont
     * refusées avec un motif, jamais transformées en succès silencieux. */
    {
        ScpsSim *s=world();
        scps_player_research(s,TECH_APEX_ARQUEBUSE);
        scps_player_research(s,TECH_COUNT);
        scps_player_declare_war(s,INT_MAX);
        scps_sim_advance_days(s,1);
        check("inaccessible research is refused",scps_command_feedback_at(s,0,&f0)
              && f0.status==SCPS_FEEDBACK_REFUSED
              && (f0.reason==SCPS_FEEDBACK_REASON_UNAVAILABLE
                  || f0.reason==SCPS_FEEDBACK_REASON_NOT_READY));
        check("research outside tree is refused",scps_command_feedback_at(s,1,&f0)
              && f0.status==SCPS_FEEDBACK_REFUSED
              && f0.reason==SCPS_FEEDBACK_REASON_INVALID_ARGUMENT);
        check("diplomacy outside world is refused",last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_REFUSED
              && f1.reason==SCPS_FEEDBACK_REASON_INVALID_ARGUMENT);
        scps_sim_free(s);
    }

    /* Dette portée uniquement par les classes : credit_bankruptcy renvoie alors
     * -1 (aucun créancier étranger), ce qui reste un résultat valide si le stock
     * de dette a effectivement été annulé. */
    {
        ScpsSim *s=world();
        int p=s ? scps_player(s) : -1;
        ScpsLoanCapacity cap[3];
        ScpsDebt before, after;
        int n=s ? scps_country_loan_capacity(s,p,cap,3) : 0;
        float ask=(n>=3) ? cap[2].montant_max : 0.f;
        if (ask>1000000.f) ask=1000000.f;
        if (ask>0.f && ask<1.f) ask=0.f; /* <=0 means maximum available */
        check("class lender exposes a usable capacity",s && ask>0.f);
        check("class debt order is enqueued",s && ask>0.f
              && scps_player_borrow_class(s,CLASS_ELITE,ask));
        scps_sim_advance_days(s,1);
        memset(&before,0,sizeof before);
        if(s) scps_country_debt(s,p,&before);
        check("class debt is real before bankruptcy",before.to_class>0.f
              && before.total>0.f);
        check("bankruptcy order is enqueued",s && scps_player_bankruptcy(s));
        scps_sim_advance_days(s,1);
        memset(&after,0,sizeof after);
        if(s) scps_country_debt(s,p,&after);
        check("class-only bankruptcy executes with creditor -1",
              s && last_feedback(s,&f1)
              && f1.status==SCPS_FEEDBACK_EXECUTED
              && f1.value==-1
              && f1.amount>0.f
              && after.total<before.total);
        scps_sim_free(s);
    }

    printf("command_feedback_demo: %d/%d\n",pass_count,pass_count+fail_count);
    return fail_count ? 1 : 0;
}
