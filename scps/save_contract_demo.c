/* Contrat de sauvegarde façade : chemin isolé, cible de recherche et purge des
 * ordres de l'ancienne session après un chargement réussi. Le Makefile peut
 * l'ajouter comme banc dédié sans toucher aux slots du projet. */
#include "scps_api.h"
#include "scps_save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check(int ok, const char *what){
    if (!ok) fprintf(stderr,"save_contract: ECHEC: %s\n",what);
    return ok;
}

int main(int argc, char **argv){
    int ok=1;
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):9u;
    /* `scps_save_game` crée le dernier dossier, pas toute une arborescence :
     * rester directement sous build rend le banc autonome après un clean. */
    const char *dir="build/save-contract-demo";
    ok &= check(scps_save_set_dir(dir),"configuration du chemin");
    ScpsWorldParams wp;
    scps_worldparams_default(seed,&wp);
    /* Le banc par défaut couvre le monde standard multi-pays. La fixture
     * réduite reste disponible pour les itérations locales rapides. */
    if (getenv("SCPS_SAVE_CONTRACT_SMALL")) {
        wp.n_empires=1; wp.n_city_states=0; wp.n_continents=1;
        scps_worldgen_set(&wp);
    }
    ScpsSim *s=scps_sim_new();
    if (!s) return 1;
    scps_sim_generate(s,seed);
    scps_sim_advance_days(s,1);
    /* Choisir une cible réellement ouverte par ce monde : le dernier nœud de
     * l'arbre est souvent tier-3 et le moteur annule légitimement cette cible
     * au premier tick (ce qui rendrait le banc dépendant de la graine). */
    ScpsTechNode nodes[256];
    int nn=scps_tech_nodes(s,nodes,256), target=-1;
    for (int i=0;i<nn;i++)
        if (nodes[i].allowed && nodes[i].state!=2){ target=i; break; }
    ok &= check(target>=0 && target<TECH_COUNT,"cible de recherche ouverte");
    ok &= check(scps_player_research(s,target)==1,"mise en file de recherche");
    scps_sim_advance_days(s,1); /* l'ordre devient l'état cible sérialisé */
    float progress=0.f;
    target=scps_research_target(s,&progress);
    ok &= check(target>=0 && target<TECH_COUNT,"cible posée");
    ok &= check(scps_sim_save(s,1)==1,"écriture du slot");
    ok &= check(strstr(save_slot_path(1),"save-contract")!=NULL,"chemin de slot isolé");

    /* Une commande de la partie courante doit disparaître au chargement ; on
     * utilise une seconde cible pour rendre sa présence observable au tick. */
    ok &= check(scps_player_research(s,-1)==1,"ordre différé");
    ok &= check(scps_sim_load(s,1)==0,"relecture du slot");
    scps_sim_advance_days(s,1);
    int after=scps_research_target(s,&progress);
    ok &= check(after==target,"purge des ordres différés");

    /* Les compteurs de guerre, fronde, usure et économie évoluent réellement :
     * une garde de save doit accepter un état de partie ordinaire après vingt ans. */
    scps_sim_advance_days(s,365*12);
    ok &= check(scps_sim_save(s,2)==1,"écriture après douze ans");
    ok &= check(scps_sim_load(s,2)==0,"relecture après douze ans");

    scps_sim_free(s);
    ScpsSim *fresh=scps_sim_new();
    ok &= check(fresh!=NULL,"instance fraîche créée");
    if (fresh){
        ok &= check(scps_sim_load(fresh,2)==0,"chargement dans une instance fraîche");
        ok &= check(scps_world_seed(fresh)==(int)seed,"graine canonique restaurée");
        scps_sim_free(fresh);
    }
    printf("save_contract: %s\n",ok?"OK":"ECHEC");
    return ok?0:1;
}
