/*
 * save_io_demo.c — banc auto-vérifiant : compresse/décompresse un bloc test et
 * vérifie le CRC32 round-trip (brief build §9.5) + l'ÉCRITURE ATOMIQUE (un échec
 * d'écriture ne corrompt pas le fichier existant). Sortie ≠ 0 si échec.
 */
#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L   /* mkdir/rmdir visibles sous -std=c99 strict */
#endif
#include "scps_save_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
# include <unistd.h>
#endif

/* lit tout un fichier en mémoire ; renvoie la longueur (-1 si absent). */
static long slurp(const char *path, unsigned char *out, long cap){
    FILE *f=fopen(path,"rb"); if (!f) return -1;
    long n=(long)fread(out,1,(size_t)cap,f); fclose(f); return n;
}

static int pass=0, fail=0;
static void check(const char *what, int ok){
    printf("   %s %s\n", ok?"✓":"✗", what);
    if (ok) pass++; else fail++;
}

int main(void){
    printf("══ save_io : compression de bloc + CRC32 round-trip ══\n");

    /* Un bloc « état » plausible : motif répétitif (comme une struct pleine de
     * zéros et de petits entiers) → la compression doit MORDRE. */
    enum { N = 64*1024 };
    unsigned char *src = (unsigned char*)malloc(N);
    if (!src){ fprintf(stderr,"OOM\n"); return 2; }
    for (int i=0;i<N;i++) src[i] = (unsigned char)((i*7+ (i/137)) & 0x1f);   /* peu d'entropie */

    uint32_t crc_src = scps_crc32(src, N);

    void *zip=NULL; size_t zn=0;
    int zok = scps_zip(src, N, &zip, &zn);
    check("la compression réussit", zok && zip && zn>0);
    check("le bloc compressé est PLUS PETIT que le clair", zok && zn < (size_t)N);

    unsigned char *dst = (unsigned char*)malloc(N);
    if (!dst){ free(src); free(zip); return 2; }
    size_t dn = zok ? scps_unzip(zip, zn, dst, N) : 0;
    check("la décompression rend la taille d'origine", dn == (size_t)N);
    check("le clair est IDENTIQUE après round-trip", dn==(size_t)N && !memcmp(src,dst,N));
    check("le CRC32 du round-trip ÉGALE celui du clair", dn==(size_t)N && scps_crc32(dst,dn)==crc_src);

    /* Garde-fou : une capacité de destination INSUFFISANTE → refus net (0). */
    unsigned char tiny[16];
    size_t too = zok ? scps_unzip(zip, zn, tiny, sizeof tiny) : 1;
    check("une destination trop petite est REFUSÉE (0)", too==0);

    /* CRC32 : déterministe et SENSIBLE — un bit retourné change l'empreinte. */
    { uint32_t a=scps_crc32("SCPS",4), a2=scps_crc32("SCPS",4), b=scps_crc32("SCQS",4);
      check("crc32 est déterministe (même entrée → même empreinte)", a==a2 && a!=0);
      check("crc32 est sensible (un octet diffère → empreinte différente)", a!=b); }

    /* ── ÉCRITURE ATOMIQUE : le slot existant SURVIT à un échec d'écriture ──
     * On écrit une 1re version (l'« ancienne sauvegarde »), puis on FORCE l'échec
     * de la passe atomique (le .tmp ne peut pas naître car un RÉPERTOIRE squatte
     * son chemin) et on vérifie que l'ancien fichier reste INTACT, octet pour
     * octet. Enfin, une écriture saine REMPLACE bien le contenu. */
    printf("── écriture atomique (write-then-rename) ──\n");
    { const char *path   = "build/sio_atomic.bin";
      const char *blocker= "build/sio_atomic.bin.tmp";   /* doit matcher <path>.tmp */
      const unsigned char v1[] = "ANCIENNE SAUVEGARDE — ne doit PAS être corrompue";
      const unsigned char v2[] = "NOUVELLE SAUVEGARDE";
      remove(path); rmdir(blocker); remove(blocker);

      bool w1 = save_write_atomic(path, v1, sizeof v1);
      check("1re écriture atomique réussit", w1);
      unsigned char rb[256]; long rn = slurp(path, rb, sizeof rb);
      check("le fichier porte la 1re version", rn==(long)sizeof v1 && !memcmp(rb,v1,sizeof v1));
      check("aucun .tmp résiduel après succès", slurp(blocker, rb, sizeof rb) < 0);

      /* Sabotage : un RÉPERTOIRE au chemin du .tmp ⇒ fopen(.tmp,"wb") échoue ⇒
       * la passe atomique renonce SANS toucher `path`. */
#ifdef _WIN32
      int md = _mkdir(blocker);
#else
      int md = mkdir(blocker, 0755);
#endif
      bool w2 = (md==0) ? save_write_atomic(path, v2, sizeof v2) : true;
      check("2e écriture atomique ÉCHOUE (le .tmp est bloqué)", md==0 && !w2);
      rn = slurp(path, rb, sizeof rb);
      check("l'ANCIEN fichier a SURVÉCU intact (échec non destructif)",
            rn==(long)sizeof v1 && !memcmp(rb,v1,sizeof v1));
      rmdir(blocker);

      /* Le sabotage levé, une écriture saine remplace bien le contenu. */
      bool w3 = save_write_atomic(path, v2, sizeof v2);
      rn = slurp(path, rb, sizeof rb);
      check("une écriture saine REMPLACE le contenu (atomicité OK)",
            w3 && rn==(long)sizeof v2 && !memcmp(rb,v2,sizeof v2));
      remove(path); }

    free(src); free(dst); free(zip);
    printf("══ BILAN : %d réussis, %d échoués ══\n", pass, fail);
    return fail ? 1 : 0;
}
