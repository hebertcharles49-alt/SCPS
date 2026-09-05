/* Banc de robustesse du chargement : checksum valide, payload sémantiquement
 * forgé, puis vérification que le rollback laisse la partie et sa file intactes.
 * Le fichier est volontairement isolé de `saves/` et n'exerce aucun hook de
 * production : on injecte uniquement une valeur dans la section WRLD. */
#include "scps_api.h"
#include "scps_save.h"
#include "scps_crypt.h"
#include "scps_save_io.h"
#include "scps_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define TAG(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))

/* Ces hooks ne sont actifs que par les options --wrap du target dédié :
 * le moteur de production ne compile ni macro ni branche de panne. */
static int g_tmp_calls=0, g_tmp_fail_at=0, g_capture_tmp_at=0;
static int g_atomic_fail=0, g_snapshot_write_fail=0, g_snapshot_write_hit=0;
static FILE *g_snapshot_file=NULL;
extern FILE *__real_tmpfile(void);
FILE *__wrap_tmpfile(void){
    g_tmp_calls++;
    if(g_tmp_fail_at>0 && g_tmp_calls>=g_tmp_fail_at) return NULL;
    FILE *f=__real_tmpfile();
    if(f && g_capture_tmp_at>0 && g_tmp_calls==g_capture_tmp_at) g_snapshot_file=f;
    return f;
}
extern size_t __real_fwrite(const void*, size_t, size_t, FILE*);
size_t __wrap_fwrite(const void *p, size_t size, size_t nmemb, FILE *f){
    if(g_snapshot_write_fail && f==g_snapshot_file){ g_snapshot_write_hit=1; return 0; }
    return __real_fwrite(p,size,nmemb,f);
}
extern bool __real_save_write_atomic(const char*, const void*, size_t);
bool __wrap_save_write_atomic(const char *path, const void *buf, size_t len){
    return g_atomic_fail ? false : __real_save_write_atomic(path,buf,len);
}

static int file_digest(const char *path, uint64_t *digest, size_t *length){
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    if(fseek(f,0,SEEK_END)!=0){ fclose(f); return 0; }
    long n=ftell(f); if(n<0){ fclose(f); return 0; }
    rewind(f);
    uint8_t *buf=(uint8_t*)malloc((size_t)n ? (size_t)n : 1);
    if(!buf || fread(buf,1,(size_t)n,f)!=(size_t)n){ free(buf); fclose(f); return 0; }
    fclose(f); *digest=scrypt_fnv1a(buf,(size_t)n); *length=(size_t)n; free(buf); return 1;
}

static int check(int ok, const char *what){
    if (!ok) fprintf(stderr, "save_failure: ECHEC: %s\n", what);
    return ok;
}

static int forge_payload(const char *path, int geo){
    FILE *f=fopen(path,"rb");
    if(!f) return 0;
    SaveHeader h; int ok=fread(&h,sizeof h,1,f)==1 && h.magic==SAVE_MAGIC &&
        h.version==SAVE_VERSION && h.payload>0 && h.payload<(256u<<20);
    uint8_t *p=ok?(uint8_t*)malloc(h.payload):NULL;
    if(!p || fread(p,1,h.payload,f)!=h.payload) ok=0;
    fclose(f);
    if(!ok){ free(p); return 0; }
    if(h.flags & SAVE_F_CRYPT) scrypt_stream(h.nonce,p,h.payload);
    size_t at=0; int found=0;
    while(at+8u<=h.payload){
        uint32_t tag=0, sz=0;
        memcpy(&tag,p+at,4); memcpy(&sz,p+at+4,4);
        if(sz > h.payload-at-8u) break;
        if(tag==TAG('W','R','L','D') && sz==sizeof(World)){
            if (geo) {
                int16_t forged=(int16_t)(SCPS_MAX_PROV+1);
                memcpy(p+at+8u+offsetof(World,cell)+offsetof(Cell,province),&forged,sizeof forged);
            } else {
                int forged=SCPS_MAX_COUNTRY+1;
                memcpy(p+at+8u+offsetof(World,n_countries),&forged,sizeof forged);
            }
            found=1; break;
        }
        at += 8u+(size_t)sz;
    }
    if(!found){ free(p); return 0; }
    h.plain_ck=scrypt_fnv1a(p,h.payload);
    if(h.flags & SAVE_F_CRYPT) scrypt_stream(h.nonce,p,h.payload);
    size_t image_n=sizeof h+(size_t)h.payload;
    uint8_t *image=(uint8_t*)malloc(image_n);
    if(!image){ free(p); return 0; }
    memcpy(image,&h,sizeof h); memcpy(image+sizeof h,p,h.payload);
    ok=save_write_atomic(path,image,image_n);
    free(image); free(p);
    return ok;
}

int main(void){
    int ok=1;
    ok &= check(scps_set_save_directory("build/save-failure-demo"),"chemin de test isolé");
    ScpsSim *s=scps_sim_new(); if(!s) return 1;
    scps_sim_generate(s,9u); scps_sim_advance_days(s,1);
    ScpsTechNode nodes[256]; int nn=scps_tech_nodes(s,nodes,256), target=-1;
    for(int i=0;i<nn;i++) if(nodes[i].allowed && nodes[i].state!=2){ target=i; break; }
    ok &= check(target>=0,"cible de recherche ouverte");
    ok &= check(scps_sim_save(s,1)==1,"save de référence");
    char path[2048]; snprintf(path,sizeof path,"%s",save_slot_path(1));
    float progress=0.f; int research_saved=scps_research_target(s,&progress);
    ok &= check(scps_player_research(s,target)==1,"ordre de recherche différé");
    int seed_before=scps_world_seed(s), countries_before=scps_country_count(s);
    ok &= check(forge_payload(path,0),"injection checksum-correcte (compteur pays)");
    int rc=scps_sim_load(s,1);
    ok &= check(rc==1,"payload sémantiquement invalide refusé");
    ok &= check(scps_world_seed(s)==seed_before && scps_country_count(s)==countries_before,
                "état vivant restauré");
    ok &= check(scps_research_target(s,&progress)==research_saved,"recherche inchangée après refus");
    scps_sim_advance_days(s,1);
    ok &= check(scps_research_target(s,&progress)==target,"file de recherche conservée puis drainée");

    /* Lien géographique forgé : prov_adj doit rester neutre avant rollback. */
    ok &= check(scps_sim_save(s,1)==1,"save valide avant corruption géographique");
    ok &= check(forge_payload(path,1),"injection géographique checksum-correcte");
    int research_before=scps_research_target(s,&progress);
    rc=scps_sim_load(s,1);
    ok &= check(rc==1,"lien géographique invalide refusé");
    ok &= check(scps_world_seed(s)==seed_before && scps_country_count(s)==countries_before &&
                scps_research_target(s,&progress)==research_before,
                "état et recherche restaurés après géo");

    /* L'écriture atomique doit laisser le slot existant bit-identique. */
    ok &= check(scps_sim_save(s,1)==1,"save valide avant panne d'écriture");
    uint64_t digest_before=0,digest_after=0; size_t len_before=0,len_after=0;
    ok &= check(file_digest(path,&digest_before,&len_before),"empreinte avant panne d'écriture");
    g_atomic_fail=1; ok &= check(scps_sim_save(s,1)==0,"écriture atomique en panne refusée"); g_atomic_fail=0;
    ok &= check(file_digest(path,&digest_after,&len_after) && len_after==len_before && digest_after==digest_before,
                "slot existant préservé après panne d'écriture");

    /* Reforger le slot valide pour atteindre le chemin de rollback. */
    ok &= check(forge_payload(path,1),"seconde injection géographique");
    int seed_snap=seed_before, countries_snap=countries_before;
    research_before=scps_research_target(s,&progress); int fail_at=g_tmp_calls+2;
    g_tmp_fail_at=fail_at;
    rc=scps_sim_load(s,1);
    g_tmp_fail_at=0;
    ok &= check(rc==1 && g_tmp_calls>=fail_at,"snapshot tmpfile en panne atteint");
    ok &= check(scps_world_seed(s)==seed_snap && scps_country_count(s)==countries_snap &&
                scps_research_target(s,&progress)==research_before,
                "état inchangé après panne snapshot");

    /* Même preuve avec une écriture snapshot qui échoue après création du FILE. */
    g_snapshot_file=NULL; g_snapshot_write_hit=0; g_snapshot_write_fail=1;
    int capture_at=g_tmp_calls+2; g_capture_tmp_at=capture_at;
    rc=scps_sim_load(s,1);
    g_snapshot_write_fail=0; g_capture_tmp_at=0; g_snapshot_file=NULL;
    ok &= check(rc==1 && g_tmp_calls>=capture_at && g_snapshot_write_hit,
                "écriture snapshot en panne atteinte");
    ok &= check(scps_world_seed(s)==seed_snap && scps_country_count(s)==countries_snap &&
                scps_research_target(s,&progress)==research_before,
                "état inchangé après panne d'écriture snapshot");
    scps_sim_free(s);
    printf("save_failure: %s (rc=%d, checksum valide + rollback + E/S injectées)\n",ok?"OK":"ECHEC",rc);
    return ok?0:1;
}
