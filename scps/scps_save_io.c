/*
 * scps_save_io.c — crc32 + deflate sur miniz (voir scps_save_io.h)
 *                  + écriture ATOMIQUE de fichier (durabilité du slot).
 * Compilé avec la même config minimale que third_party/miniz.c.
 */
#ifndef _WIN32
/* fileno()/fsync() visibles sous -std=c99 strict (POSIX.1-2008). À déclarer
 * AVANT le moindre include système. rename() est, lui, du C standard. */
# define _POSIX_C_SOURCE 200809L
#endif
#include "scps_save_io.h"
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#include "miniz.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
#endif

uint32_t scps_crc32(const void *p, size_t n){
    return (uint32_t)mz_crc32(MZ_CRC32_INIT, (const unsigned char*)p, n);
}

bool scps_zip(const void *src, size_t n, void **out, size_t *outn){
    if (!out || !outn) return false;
    *out=NULL; *outn=0;
    mz_ulong cap = mz_compressBound((mz_ulong)n);
    unsigned char *buf = (unsigned char*)malloc(cap ? cap : 1);
    if (!buf) return false;
    mz_ulong clen = cap;
    int rc = mz_compress(buf, &clen, (const unsigned char*)src, (mz_ulong)n);
    if (rc != MZ_OK){ free(buf); return false; }
    *out = buf; *outn = (size_t)clen;
    return true;
}

size_t scps_unzip(const void *src, size_t n, void *dst, size_t dstn){
    mz_ulong dlen = (mz_ulong)dstn;
    int rc = mz_uncompress((unsigned char*)dst, &dlen, (const unsigned char*)src, (mz_ulong)n);
    return (rc == MZ_OK) ? (size_t)dlen : 0;
}

/* ── écriture ATOMIQUE (write-then-rename) ─────────────────────────────────
 * On écrit la TOTALITÉ dans <path>.tmp, on FLUSHE jusqu'au disque, on ferme,
 * puis on bascule par rename atomique. Aucune étape ne touche `path` avant que
 * le .tmp soit complet et durable ⇒ une coupure en cours laisse l'ancien slot
 * INTACT. La suite ".tmp" tient dans un tampon local borné. */
bool save_write_atomic(const char *path, const void *buf, size_t len){
    if (!path) return false;
    char tmp[1024];
    int np = snprintf(tmp, sizeof tmp, "%s.tmp", path);
    if (np < 0 || (size_t)np >= sizeof tmp) return false;   /* chemin trop long : refus net */

    FILE *f = fopen(tmp, "wb");
    if (!f) return false;

    bool ok = (len == 0) || (fwrite(buf, 1, len, f) == len);

    /* Durabilité : vider le tampon stdio PUIS forcer le noyau à écrire (fsync).
     * Sans le fsync, le rename pourrait précéder l'arrivée des octets sur le
     * média (un crash machine laisserait un slot neuf VIDE). */
    if (ok && fflush(f) != 0) ok = false;
#ifndef _WIN32
    if (ok){ int fd = fileno(f); if (fd < 0 || fsync(fd) != 0) ok = false; }
#endif
    if (fclose(f) != 0) ok = false;   /* le flush final peut révéler un disque plein */

    if (!ok){ remove(tmp); return false; }   /* .tmp partiel jeté ; `path` jamais touché */

#ifdef _WIN32
    /* rename() POSIX échoue si la cible existe ; sous Win32 c'est MoveFileEx
     * avec REPLACE_EXISTING (atomique sur le même volume). */
    if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)){
        remove(tmp); return false;
    }
#else
    if (rename(tmp, path) != 0){ remove(tmp); return false; }   /* échec → ancien slot préservé */
#endif
    return true;
}
