#include "scps_provlog.h"
#include "scps_types.h"     /* SCPS_MAX_REG */
#include "scps_lang.h"      /* StrId : les libellés de modificateurs */
#include <string.h>

/* l'anneau par région + le compteur TOTAL poussé (la tête = total-1) */
static ProvLogEntry g_log[SCPS_MAX_REG][PROVLOG_CAP];
static int          g_total[SCPS_MAX_REG];
static unsigned     g_prevmask[SCPS_MAX_REG];
static unsigned char g_primed[SCPS_MAX_REG];
static int          g_year;

/* libellé STR_* + signe par bit de modificateur DYNAMIQUE (cf. JMOD_*) */
static const StrId JMOD_STR[JMOD_COUNT] = {
    STR_AGIT_CAUSE_CHOC,          /* JMOD_CONQUEST    → « Conquête récente » */
    STR_PMOD_CICATRICE_NOM,       /* JMOD_SCAR        → cicatrice de révolte  */
    STR_PMOD_ABONDANCE_NOM,       /* JMOD_ABONDANCE                            */
    STR_PMOD_FERVEUR_NOM,         /* JMOD_FERVEUR                              */
    STR_PMOD_RECONSTRUCTION_NOM,  /* JMOD_RECONSTRUCT                          */
};
static const signed char JMOD_SIGN[JMOD_COUNT] = { +1, +1, -1, -1, -1 };

void provlog_reset(void){
    memset(g_log, 0, sizeof g_log);
    memset(g_total, 0, sizeof g_total);
    memset(g_prevmask, 0, sizeof g_prevmask);
    memset(g_primed, 0, sizeof g_primed);
    g_year = 0;
}

void provlog_set_year(int year){ g_year = year; }

static void push(int region, int str_id, const char *lit, int sign){
    if (region < 0 || region >= SCPS_MAX_REG) return;
    ProvLogEntry *e = &g_log[region][g_total[region] % PROVLOG_CAP];
    e->year = g_year; e->str_id = str_id; e->lit = lit;
    e->sign = (signed char)(sign < 0 ? -1 : (sign > 0 ? 1 : 0));
    g_total[region]++;
}

void provlog_push_str(int region, int str_id, int sign){ push(region, str_id, 0, sign); }
void provlog_push_lit(int region, const char *lit, int sign){ if (lit) push(region, -1, lit, sign); }

void provlog_modifier_diff(int region, unsigned mask){
    if (region < 0 || region >= SCPS_MAX_REG) return;
    if (!g_primed[region]){              /* premier passage : pose la base, ne logue rien (anti-spam an-0) */
        g_prevmask[region] = mask;
        g_primed[region] = 1;
        return;
    }
    unsigned appeared = mask & ~g_prevmask[region];   /* bits NOUVELLEMENT actifs */
    for (int b = 0; b < JMOD_COUNT; b++)
        if (appeared & (1u << b))
            provlog_push_str(region, (int)JMOD_STR[b], JMOD_SIGN[b]);
    g_prevmask[region] = mask;
}

int provlog_count(int region){
    if (region < 0 || region >= SCPS_MAX_REG) return 0;
    int t = g_total[region];
    return t < PROVLOG_CAP ? t : PROVLOG_CAP;
}

const ProvLogEntry *provlog_at(int region, int i){
    if (region < 0 || region >= SCPS_MAX_REG) return 0;
    int n = provlog_count(region);
    if (i < 0 || i >= n) return 0;
    /* i=0 = la plus récente : tête = (total-1) ; on remonte de i */
    int idx = (g_total[region] - 1 - i) % PROVLOG_CAP;
    if (idx < 0) idx += PROVLOG_CAP;
    return &g_log[region][idx];
}
