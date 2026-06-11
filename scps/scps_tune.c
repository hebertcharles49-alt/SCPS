/*
 * scps_tune.c — le registre des tunables (Arc J1, voir scps_tune.h).
 *
 * Registre BÂTI de scps_tune_list.h (X-macro) → tous les noms valides sont connus
 * d'emblée, donc un nom inconnu dans SCPS_TUNE est rejeté AVANT le run (exit 2 —
 * la faute de frappe ne calibre pas dans le vide). Lecture de l'env une seule fois.
 */
#include "scps_tune.h"
#include "scps_tune_list.h"
#include <stdlib.h>
#include <string.h>

typedef struct { const char *name; float def, val; int overridden; } Tunable;

static Tunable g_reg[] = {
#define X(n, d) { #n, (d), (d), 0 },
    SCPS_TUNABLES(X)
#undef X
};
static const int  g_n = (int)(sizeof g_reg / sizeof g_reg[0]);
static int        g_inited = 0;
static char       g_active[1024];   /* "N=V,N=V" des surcharges (pour le CSV / l'en-tête) */

static Tunable *find(const char *name){
    for (int i=0;i<g_n;i++) if (strcmp(g_reg[i].name, name)==0) return &g_reg[i];
    return NULL;
}

void tune_init(void){
    if (g_inited) return;
    g_inited = 1;
    g_active[0] = '\0';
    const char *env = getenv("SCPS_TUNE");
    if (!env || !*env) return;
    /* copie modifiable */
    char buf[1024];
    strncpy(buf, env, sizeof buf - 1); buf[sizeof buf - 1] = '\0';
    for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")){
        char *eq = strchr(tok, '=');
        if (!eq){
            fprintf(stderr, "scps_tune: entrée mal formée « %s » (attendu NOM=VALEUR)\n", tok);
            exit(2);
        }
        *eq = '\0';
        const char *name = tok;
        const char *vstr = eq + 1;
        Tunable *t = find(name);
        if (!t){
            fprintf(stderr, "scps_tune: tunable INCONNU « %s » — vois `chronicle --tunables`\n", name);
            exit(2);
        }
        char *end = NULL;
        float v = strtof(vstr, &end);
        if (end == vstr || (end && *end != '\0')){
            fprintf(stderr, "scps_tune: valeur invalide « %s » pour %s\n", vstr, name);
            exit(2);
        }
        t->val = v;
        t->overridden = 1;
    }
    /* construit la chaîne des surcharges actives (ordre du registre, stable) */
    size_t o = 0;
    for (int i=0;i<g_n && o < sizeof g_active - 1;i++){
        if (!g_reg[i].overridden) continue;
        o += (size_t)snprintf(g_active + o, sizeof g_active - o, "%s%s=%g",
                              o ? "," : "", g_reg[i].name, g_reg[i].val);
    }
}

float tune_f(const char *name, float def){
    if (!g_inited) tune_init();
    Tunable *t = find(name);
    return t ? t->val : def;   /* t->def doit égaler def (source unique = la X-macro) */
}

void tune_list(FILE *out){
    if (!g_inited) tune_init();
    fprintf(out, "tunables (%d) — SCPS_TUNE=\"NOM=VAL,…\" surcharge :\n", g_n);
    for (int i=0;i<g_n;i++)
        fprintf(out, "  %-22s défaut %-10g  actif %-10g%s\n",
                g_reg[i].name, g_reg[i].def, g_reg[i].val,
                g_reg[i].overridden ? "  (surchargé)" : "");
}

void tune_print_active(FILE *out){
    if (!g_inited) tune_init();
    if (g_active[0]) fprintf(out, "[tune] surcharges actives : %s\n", g_active);
}

int tune_n_active(void){
    if (!g_inited) tune_init();
    int n=0; for (int i=0;i<g_n;i++) if (g_reg[i].overridden) n++;
    return n;
}

const char *tune_active_string(void){
    if (!g_inited) tune_init();
    return g_active;
}
