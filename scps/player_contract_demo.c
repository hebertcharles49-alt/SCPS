/* Fixtures internes pour les mutations directes : refus atomiques, propriété,
 * éligibilité religieuse et effet du réglage de durée. Pas de fichier utilisateur. */
#include "scps_api.c"

static int passed, failed;
static void check(const char *label, int yes){
    printf("%s %s\n",yes?"OK":"FAIL",label);
    if(yes) passed++; else failed++;
}
int main(void){
    ScpsSim *s=scps_sim_new();
    if(!s) return 1;
    scps_player_set_buy_rate(s,0,20); /* monde non prêt : doit rester sans effet */
    scps_sim_generate(s,9);
    int p=scps_player(s), other=(p+1)%s->w->n_countries;
    int own=-1, foreign=-1, foreign_cell=-1;
    for(int r=0;r<s->w->n_regions;r++){
        if(s->w->region[r].country==p) own=r;
        else if(s->w->region[r].country>=0) foreign=r;
    }
    for(int i=0;i<SCPS_N;i++)
        if(s->w->cell[i].country>=0 && s->w->cell[i].country!=p){ foreign_cell=i; break; }
    check("fixture régions possédée/étrangère",own>=0&&foreign>=0&&foreign_cell>=0);
    int n=g_religion_count;
    check("fondation étrangère refusée",scps_religion_found(s,other,0,0,4,10)<0&&g_religion_count==n);
    check("crédo invalide refusé",scps_religion_found(s,p,-1,0,4,10)<0&&g_religion_count==n);
    /* La preuve de fondation est PROVINCE, jamais l'union d'une région. Préparer
     * une province joueur nue et un faux bit agrégé pour vérifier les deux côtés. */
    const uint32_t temple_mask=(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE);
    int own_pid=-1;
    for(int i=0;i<s->sim.econ->n_prov;i++){
        if(s->sim.econ->prov[i].owner==p){
            own_pid=i;
            s->sim.econ->prov[i].edi_built &= ~temple_mask;
        }
    }
    for(int r=0;r<s->sim.econ->n_regions;r++) s->sim.econ->region[r].edi_built &= ~temple_mask;
    if(own_pid<0){
        check("province joueur disponible pour fixture Temple",0);
        scps_sim_free(s);
        return 1;
    }
    int foreign_pid=-1;
    for(int i=0;i<s->sim.econ->n_prov;i++)
        if(s->sim.econ->prov[i].owner>=0 && s->sim.econ->prov[i].owner!=p){ foreign_pid=i; break; }
    s->sim.econ->prov[own_pid].edi_built |= (1u<<EDI_SANCTUAIRE);
    check("Sanctuaire seul ne permet pas la fondation",scps_religion_founding_ready(s,p)==0);
    if(foreign_pid>=0) s->sim.econ->prov[foreign_pid].edi_built |= (1u<<EDI_TEMPLE);
    check("Temple étranger ignoré",scps_religion_founding_ready(s,p)==0);
    n=g_religion_count;
    check("fondation sans Temple refusée sans mutation",
          own_pid>=0 && scps_religion_founding_ready(s,p)==0
          && scps_religion_found(s,p,CREDO_PLURALISTE,RP_FECONDITE,RP_ACCUEIL,RP_GNOSE)<0
          && g_religion_count==n && scps_religion_of_country(s,p)<0);
    int own_reg=s->sim.econ->prov[own_pid].region;
    s->sim.econ->region[own_reg].edi_built |= (1u<<EDI_TEMPLE);
    check("bit Temple agrégé seul ignoré",
          scps_religion_founding_ready(s,p)==0);
    s->sim.econ->region[own_reg].edi_built &= ~temple_mask;
    s->sim.econ->prov[own_pid].edi_built |= (1u<<EDI_TEMPLE);
    check("Temple provincial reconnu",scps_religion_founding_ready(s,p)==1);
    s->sim.econ->prov[own_pid].edi_built &= ~(1u<<EDI_TEMPLE);
    s->sim.econ->prov[own_pid].edi_built |= (1u<<EDI_CATHEDRALE);
    check("Cathédrale provinciale reconnue",scps_religion_founding_ready(s,p)==1);
    int rid=scps_religion_found(s,p,CREDO_PLURALISTE,RP_FECONDITE,RP_ACCUEIL,RP_GNOSE);
    check("fondation joueur",rid>=0);
    int inherited=0;
    for(int r=0;r<s->w->n_regions;r++)
        if(religion_of_region(r)==rid) inherited++;
    check("fondation joueur hérite les régions",inherited>0);
    if(rid<0){ scps_sim_free(s); return 1; }
    n=g_religion_count;
    check("double fondation refusée",scps_religion_found(s,p,0,0,4,10)<0&&g_religion_count==n);
    int flipped=99;
    /* Centre possédé et monde initial homogène : aucune dérive. */
    check("schisme initial non éligible",scps_religion_eligible(s,p)==0);
    check("schisme inéligible sans mutation",
        scps_religion_schism(s,p,1,RP_MUR,2,RP_ORTHODOXIE,CREDO_PURIFICATEUR,&flipped)<0
        &&flipped==0&&g_religion_count==n);
    check("érudit hors région refusé",scps_religion_recruit_scholar(s,p,-1)<0);
    check("érudit région étrangère refusé",scps_religion_recruit_scholar(s,p,foreign)<0);
    check("érudit pays étranger refusé",scps_religion_recruit_scholar(s,other,own)<0);
    check("aucune mission créée après refus",!religion_scholar_active(p));
    scps_set_observer(s,1);
    check("observateur ne recrute pas",scps_religion_recruit_scholar(s,p,own)<0);
    int before=scps_country_buy_rate(s,p,0);
    scps_player_set_buy_rate(s,0,20);
    check("observateur ne modifie pas les rachats",scps_country_buy_rate(s,p,0)==before);
    scps_set_observer(s,0);
    scps_player_set_buy_rate(s,0,-1);
    scps_player_set_buy_rate(s,0,101);
    scps_player_set_buy_rate(s,3,20);
    check("rachats invalides sans effet",scps_country_buy_rate(s,p,0)==before);
    scps_player_set_buy_rate(s,0,20);
    check("rachat joueur appliqué",scps_country_buy_rate(s,p,0)==20);
    float duration=tune_f("RELIG_SCHOLAR_DAYS",1825.f);
    tune_set("RELIG_SCHOLAR_DAYS",1.f);
    check("érudit joueur recruté",scps_religion_recruit_scholar(s,p,own)>=0);
    religion_scholar_tick(s->w,s->sim.econ);
    check("durée du lettré réglable",!religion_scholar_active(p));
    tune_set("RELIG_SCHOLAR_DAYS",duration);
    /* Perte du centre : la rupture doit devenir possible puis exécutable. */
    g_religions[rid].centre_cell=foreign_cell;
    check("centre étranger ouvre la rupture",scps_religion_eligible(s,p)==RSE_RUPTURE);
    int child=scps_religion_schism(s,p,1,RP_MUR,2,RP_ORTHODOXIE,CREDO_PURIFICATEUR,&flipped);
    check("schisme éligible exécuté",child>=0&&g_religion_count==n+1);
    scps_sim_free(s);
    printf("BILAN player_contract_demo : %d/%d OK\n",passed,passed+failed);
    return failed?1:0;
}
