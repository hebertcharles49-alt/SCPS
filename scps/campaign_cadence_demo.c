/* Cadence joueur et reprise en marche : façade publique, monde neuf. */
#include "scps_api.h"
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
static int pass,fail;
static void check(const char *name,int ok){printf("%s %s\n",ok?"OK":"FAIL",name);if(ok)pass++;else fail++;}
static void day(ScpsSim *s){
    ScpsPendingEvent ev;
    if(scps_pending_event(s,0,&ev)&&ev.n_options>0)scps_player_event_choice(s,0,0);
    scps_sim_advance_days(s,1);
}
static int same(const ScpsArmyInfo *a,const ScpsArmyInfo *b){
    return a->active==b->active && a->region==b->region && a->dest==b->dest
        && a->phase_id==b->phase_id && a->units==b->units && a->taken==b->taken
        && a->battles==b->battles && a->broken_days==b->broken_days
        && fabsf(a->days_left-b->days_left)<1e-5f && fabsf(a->rally_days-b->rally_days)<1e-5f;
}
int main(void){
    ScpsSim *s=scps_sim_new();
    check("simulation allocated",s!=NULL); if(!s)return 1;
    scps_sim_generate(s,9);
    int p=scps_player(s),cap=scps_country_capital_region(s,p);
    ScpsArmy army;
    scps_player_recruit(s,0);day(s);scps_country_army(s,p,&army);
    check("public recruitment supplies a packet",army.regiments>0);
    check("raise order enqueued",scps_player_raise_corps(s,1,cap));day(s);
    int id=scps_country_corps_id(s,p,0);
    ScpsArmyInfo a,b;
    scps_corps_info(s,id,&a);check("corps deployed",id>=0&&a.active&&a.units>0);
    int idle_route[2]={-1,-1};
    int idle_route_n=scps_corps_route(s,id,idle_route,2);
    check("route reader exposes idle origin",idle_route_n==1&&idle_route[0]==a.region);
    check("route reader rejects invalid corps",scps_corps_route(s,-1,idle_route,2)==0);
    check("route reader rejects empty capacity",scps_corps_route(s,id,idle_route,0)==0);
    int target=-1;
    for(int r=0;r<scps_region_count(s);r++){
        int owner=scps_region_owner(s,r);
        if(owner<0||owner==p||!scps_country_known(s,owner))continue;
        ScpsMovePreview v;scps_corps_move_preview(s,id,r,&v,NULL,0);
        if(v.valid && v.travel_days>=5.f && v.travel_days<60.f){target=r;break;}
    }
    check("reachable public march target",target>=0);
    if(target<0)goto done;
    check("march order enqueued",scps_player_move_corps(s,id,target));day(s);
    scps_corps_info(s,id,&a);
    int route_before[3]={-1,-1,-1};
    int route_n=scps_corps_route(s,id,route_before,3);
    check("route reader exposes current segment",route_n>=1&&route_n<=2&&route_before[0]==a.region);
    check("route reader exposes stored next step",a.next<0||(route_n==2&&route_before[1]==a.next));
    int tiny_route[1]={-777};
    check("route reader truncates without inventing a step",
          scps_corps_route(s,id,tiny_route,1)==1&&tiny_route[0]==a.region);
    int route_region=a.region, route_next=a.next, route_dest=a.dest;
    scps_corps_route(s,id,route_before,3);
    scps_corps_info(s,id,&b);
    check("route reader is pure",a.region==b.region&&a.next==b.next&&a.dest==b.dest
          &&route_region==b.region&&route_next==b.next&&route_dest==b.dest);
    day(s);scps_corps_info(s,id,&b);
    check("march progresses the next calendar day",a.active&&b.active
        && (b.region!=a.region || b.phase_id!=a.phase_id || b.days_left<a.days_left-0.5f));
    check("test remains before annual closure",scps_year(s)==0);
#ifdef _WIN32
    _mkdir("cadence_test_saves");
#else
    mkdir("cadence_test_saves",0700);
#endif
    check("isolated save directory",scps_set_save_directory("cadence_test_saves"));
    int saved=scps_sim_save(s,1);check("save while marching",saved);
    ScpsArmyInfo expected[20];
    for(int d=0;d<20;d++){day(s);scps_corps_info(s,id,&expected[d]);}
    scps_sim_free(s);s=scps_sim_new();
    int loaded=saved&&s&&scps_sim_load(s,1)==0;check("reload fresh simulation",loaded);
    if(loaded){
        int equal=1;
        for(int d=0;d<20;d++){day(s);scps_corps_info(s,id,&b);if(!same(&expected[d],&b))equal=0;}
        check("twenty daily corps states match after reload",equal);
    }
done:
    scps_sim_free(s);
    printf("campaign_cadence_demo: %d/%d\n",pass,pass+fail);
    return fail?1:0;
}
