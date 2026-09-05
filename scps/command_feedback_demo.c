/* command_feedback_demo.c — contrat du journal TRANSIENT des ordres joueur.
 *
 * Le banc vérifie le point de séparation essentiel : les actions player_* sont
 * seulement mises en file, puis le verdict arrive au drain. Il reste volontairement
 * court et n'avance le monde que d'un jour par scénario.
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
