#ifndef SCPS_MODPARSE_H
#define SCPS_MODPARSE_H

/* Strict, shared parsing for SCPS_MODS.  Moddata is optional input: a malformed
 * field must never become zero through atoi/atof, and a record is committed only
 * after every value on that record has passed its domain check. */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define SCPS_MOD_UNUSED __attribute__((unused))
#else
#define SCPS_MOD_UNUSED
#endif

static int scps_mod_float(const char *text, float *out){
    if (!text || !out) return 0;
    while (isspace((unsigned char)*text)) text++;
    if (!*text) return 0;
    errno = 0;
    char *end = NULL;
    float value = strtof(text, &end);
    if (text == end || errno == ERANGE || !isfinite(value)) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;
    *out = value;
    return 1;
}

static int scps_mod_int(const char *text, int *out) SCPS_MOD_UNUSED;
static int scps_mod_int(const char *text, int *out){
    if (!text || !out) return 0;
    while (isspace((unsigned char)*text)) text++;
    if (!*text) return 0;
    errno = 0;
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (text == end || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0' || value < 0 || value > 2147483647L) return 0;
    *out = (int)value;
    return 1;
}

static char *scps_mod_first_nonspace(char *line){
    while (line && isspace((unsigned char)*line)) line++;
    return line;
}

/* fgets() may return only the first fragment of a physical line.  Consume the
 * remainder so a long record can never be accepted as a shortened record. */
static int scps_mod_line_complete(FILE *f, const char *line){
    if (!line) return 0;
    size_t n = strlen(line);
    /* All current loaders use char line[256].  A shorter fragment without a
     * newline is a complete final line; only a full buffer needs draining. */
    if (n < 255 || (n && line[n-1] == '\n')) return 1;
    if (!f) return 0;
    int ch;
    while ((ch=fgetc(f)) != '\n' && ch != EOF) {}
    return 0;
}

static void scps_mod_invalid(const char *path, int line, const char *why){
    fprintf(stderr, "[mods] %s:%d : ligne invalide (%s)\n",
            path ? path : "<mods>", line, why ? why : "champ invalide");
}

#endif
