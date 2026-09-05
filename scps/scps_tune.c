/*
 * scps_tune.c — le registre des tunables (Arc J1, voir scps_tune.h).
 *
 * Registre BÂTI de scps_tune_list.h (X-macro) → tous les noms valides sont connus
 * d'emblée, donc un nom inconnu dans SCPS_TUNE est rejeté AVANT le run (exit 2 —
 * la faute de frappe ne calibre pas dans le vide). Lecture de l'env une seule fois.
 */
#include "scps_tune.h"
#include "scps_tune_list.h"
#include <math.h>
#include <stdint.h>
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
static char      *g_active = NULL;  /* chaîne canonique complète (pour CSV / en-tête) */
static size_t     g_active_cap = 0;
static unsigned   g_revision = 0;

/* Clés historiques gardées dans le registre pour que les anciennes interfaces
 * puissent encore les afficher, mais sans effet moteur. Les réactiver demande
 * d'abord de raccorder un vrai call-site ; tune_set_checked/SCPS_TUNE les refuse. */
static const char *const g_inactive[] = {
    "REGION_RAW_KEEP", "SPAWN_FOOD_RAW", "NAVY_BUILD_SUPPLY_FLOOR",
    "AI_COMPLEMENT_W", "WILD_REGIMENTS"
};
static const char *const g_diagnostic[] = {
    "INVARIANT_DRIFT_FRAC", "INVARIANT_SCALE_FLOOR"
};
static const char *const g_genesis[] = {
    "RES_MAT_SPREAD", "RES_GUARANTEE_ALL", "WORLD_PROV_BASE",
    "WORLD_PROV_PER_EMPIRE", "WORLD_PROV_PER_CITY", "WORLD_EMP_COMFORT_LOW",
    "WORLD_EMP_COMFORT_FULL", "WORLD_PROV_SAT_K", "RIVER_FILL",
    "RIVER_ARID_NIL", "RIVER_NILE_KEEP", "SPAWN_SAFE_HOPS",
    "SPAWN_SAFE_HOPS_MIN", "SPAWN_CONT_QUOTA", "SPAWN_CONT_MIN_HAB",
    "WILD_CULTURE_DISTINCT", "CHOKE_MIN_ROUTES", "CHOKE_MIN_FRAC"
};
static int is_inactive(const char *name){
    for (size_t i=0;i<sizeof g_inactive/sizeof g_inactive[0];i++)
        if (strcmp(g_inactive[i], name)==0) return 1;
    return 0;
}
static int in_names(const char *name, const char *const *list, size_t n){
    for (size_t i=0;i<n;i++) if (strcmp(list[i], name)==0) return 1;
    return 0;
}
static int value_sane(const char *name, float val){
    if (!isfinite(val)) return 0;
    /* Les réglages de durée/coût ne peuvent pas être négatifs : les bornes
     * spécifiques religion restent ici afin que l'UI et le moteur partagent
     * le même contrat avant d'atteindre les helpers de domaine. */
    if (val < 0.f && (strstr(name,"COST") || strstr(name,"PRICE") ||
                      strstr(name,"DAYS") || strstr(name,"DURATION"))) return 0;
    if (strcmp(name,"RELIG_SCHISM_FLIP_D")==0 || strcmp(name,"RELIG_SCHISM_FLIP_L")==0)
        return val>=0.f && val<=10.f;
    if (strcmp(name,"RELIG_SCHOLAR_DAYS")==0) return val>=1.f && val<=365000.f;
    if (strcmp(name,"RELIG_SCHISM_MAX")==0) return val>=0.f && val<=64.f;
    if (strncmp(name,"TIER",4)==0 && strstr(name,"_POP")!=NULL)
        return val>=1.f && val<=1000000000.f; /* sûr avant conversion en long */
    return 1;
}

static Tunable *find(const char *name){
    if (!name) return NULL;
    for (int i=0;i<g_n;i++) if (strcmp(g_reg[i].name, name)==0) return &g_reg[i];
    return NULL;
}

static void rebuild_active(void){
    size_t need = 0;
    for (int i=0;i<g_n;i++) if (g_reg[i].overridden){
        int n = snprintf(NULL, 0, "%s%s=%g", need ? "," : "", g_reg[i].name, g_reg[i].val);
        if (n < 0 || (size_t)n > SIZE_MAX - need - 1){
            fprintf(stderr, "scps_tune: chaîne canonique trop longue\n");
            exit(2);
        }
        need += (size_t)n;
    }
    if (need + 1 > g_active_cap){
        char *p = (char*)realloc(g_active, need + 1);
        if (!p){
            fprintf(stderr, "scps_tune: mémoire insuffisante pour la chaîne canonique\n");
            exit(2);
        }
        g_active = p;
        g_active_cap = need + 1;
    }
    if (!g_active){
        g_active = (char*)malloc(1);
        if (!g_active){ fprintf(stderr, "scps_tune: mémoire insuffisante\n"); exit(2); }
        g_active_cap = 1;
    }
    size_t o = 0;
    for (int i=0;i<g_n;i++) if (g_reg[i].overridden){
        int n = snprintf(g_active + o, g_active_cap - o, "%s%s=%g",
                         o ? "," : "", g_reg[i].name, g_reg[i].val);
        if (n < 0 || (size_t)n >= g_active_cap - o){
            fprintf(stderr, "scps_tune: erreur de construction de la chaîne canonique\n");
            exit(2);
        }
        o += (size_t)n;
    }
    g_active[o] = '\0';
}

void tune_init(void){
    if (g_inited) return;
    g_inited = 1;
    rebuild_active();
    const char *env = getenv("SCPS_TUNE");
    if (!env || !*env) return;
    /* Copie modifiable à taille exacte : SCPS_TUNE ne doit jamais être
     * tronquée, même lorsque le fingerprint/CSV dépasse 1024 octets. */
    size_t env_len=strlen(env);
    char *buf=(char*)malloc(env_len+1);
    if (!buf){ fprintf(stderr, "scps_tune: mémoire insuffisante pour SCPS_TUNE\n"); exit(2); }
    memcpy(buf, env, env_len + 1);
    int changed = 0;
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
        if (is_inactive(name)){
            fprintf(stderr, "scps_tune: tunable INACTIF « %s » — aucun call-site moteur\n", name);
            exit(2);
        }
        char *end = NULL;
        float v = strtof(vstr, &end);
        if (end == vstr || (end && *end != '\0') || !value_sane(name,v)){
            fprintf(stderr, "scps_tune: valeur invalide « %s » pour %s\n", vstr, name);
            exit(2);
        }
        t->val = v;
        t->overridden = 1;
        changed = 1;
    }
    free(buf);
    rebuild_active();
    if (changed) g_revision = 1;
}

float tune_f(const char *name, float def){
    if (!g_inited) tune_init();
    Tunable *t = find(name);
    return t ? t->val : def;   /* t->def doit égaler def (source unique = la X-macro) */
}

int tune_set_checked(const char *name, float val){
    if (!g_inited) tune_init();        /* l'env est parsé d'abord ; tune_set surcharge ensuite */
    Tunable *t = find(name);
    if (!t || is_inactive(name) || !value_sane(name,val)) return 0;
    t->val = val; t->overridden = 1;
    rebuild_active();
    ++g_revision;
    return 1;
}

void tune_set(const char *name, float val){
    (void)tune_set_checked(name, val);
}

int tune_reset(const char *name){
    if (!g_inited) tune_init();
    Tunable *t=find(name);
    if (!t || is_inactive(name)) return 0;
    if (!t->overridden && t->val==t->def) return 1;
    t->val=t->def; t->overridden=0;
    rebuild_active();
    ++g_revision;
    return 1;
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

static uint32_t fnv32(const char *s, size_t n){
    uint64_t h=0xcbf29ce484222325ull;
    for (size_t i=0;i<n;i++){ h^=(unsigned char)s[i]; h*=0x100000001b3ull; }
    return (uint32_t)h;
}

uint32_t tune_fingerprint32(void){
    if (!g_inited) tune_init();
    /* L'empreinte de contrat ne dépend pas du format court (%g) de l'en-tête
     * CSV : elle couvre chaque valeur effective avec ses bits IEEE exacts,
     * dans l'ordre stable du registre. */
    uint64_t h=0xcbf29ce484222325ull;
    for (int i=0;i<g_n;i++){
        const unsigned char *p=(const unsigned char*)g_reg[i].name;
        for (;*p;p++){ h^=*p; h*=0x100000001b3ull; }
        h^='='; h*=0x100000001b3ull;
        uint32_t bits=0; memcpy(&bits, &g_reg[i].val, sizeof bits);
        for (unsigned b=0;b<4;b++){ h^=(unsigned char)(bits>>(8*b)); h*=0x100000001b3ull; }
        h^=';'; h*=0x100000001b3ull;
    }
    return (uint32_t)h;
}

uint32_t tune_legacy_fingerprint32(void){
    if (!g_inited) tune_init();
    size_t n = strlen(g_active);
    if (n > 1023) n = 1023;
    return fnv32(g_active, n);
}

unsigned tune_revision(void){
    if (!g_inited) tune_init();
    return g_revision;
}

int tune_n_active(void){
    if (!g_inited) tune_init();
    int n=0; for (int i=0;i<g_n;i++) if (g_reg[i].overridden) n++;
    return n;
}

/* ── Énumération du registre (MODTOOLS — panneau dev : lister + éditer en direct) ── */
int         tune_count(void){ return g_n; }
const char *tune_name_at(int i){ return (i>=0&&i<g_n)?g_reg[i].name:NULL; }
float       tune_value_at(int i){ if(!g_inited)tune_init(); return (i>=0&&i<g_n)?g_reg[i].val:0.f; }
float       tune_default_at(int i){ return (i>=0&&i<g_n)?g_reg[i].def:0.f; }
int         tune_overridden_at(int i){ if(!g_inited)tune_init(); return (i>=0&&i<g_n)?g_reg[i].overridden:0; }
int         tune_is_active(const char *name){ return name && find(name) && !is_inactive(name); }
const char *tune_phase(const char *name){
    if (!g_inited) tune_init();
    Tunable *t=find(name);
    if (!t) return NULL;
    if (is_inactive(name)) return "inactive";
    if (in_names(name,g_diagnostic,sizeof g_diagnostic/sizeof g_diagnostic[0])) return "diagnostic";
    if (in_names(name,g_genesis,sizeof g_genesis/sizeof g_genesis[0]) ||
        strncmp(name, "TOPONYM_", 8)==0) return "new_world";
    if (strcmp(name, "RELIG_SCHOLAR_DAYS")==0) return "next_action";
    if (strncmp(name, "RELIG_", 6)==0) return "rule_read";
    return "rule_read";
}

const char *tune_active_string(void){
    if (!g_inited) tune_init();
    return g_active;
}
