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

/* libellé NOM, ligne d'EFFET (hover) et signe par bit de modificateur DYNAMIQUE */
static const StrId JMOD_STR[JMOD_COUNT] = {
    STR_AGIT_CAUSE_CHOC,          /* JMOD_CONQUEST    → « Conquête récente » */
    STR_PMOD_CICATRICE_NOM,       /* JMOD_SCAR        → cicatrice de révolte  */
    STR_PMOD_ABONDANCE_NOM,       /* JMOD_ABONDANCE                            */
    STR_PMOD_FERVEUR_NOM,         /* JMOD_FERVEUR                              */
    STR_PMOD_RECONSTRUCTION_NOM,  /* JMOD_RECONSTRUCT                          */
};
static const StrId JMOD_EFF[JMOD_COUNT] = {
    STR_JLOG_CHOC_EFF,            /* conquête : agitation, se résorbe */
    STR_PMOD_CICATRICE_EFF,
    STR_PMOD_ABONDANCE_EFF,
    STR_PMOD_FERVEUR_EFF,
    STR_PMOD_RECONSTRUCTION_EFF,
};
static const signed char JMOD_SIGN[JMOD_COUNT] = { +1, +1, -1, -1, -1 };

/* ── LE FIL D'ÉVÈNEMENTS GLOBAL (voir header) : anneau write-only, jamais relu par le
 *    moteur — la RAZ vit dans provlog_reset (un seul point d'hygiène par sim). ── */
static FeedEntry g_feed[FEED_CAP];
static int g_feed_seq = 0;    /* seq du DERNIER poussé (0 = fil vide) */

void feed_push(int kind, int a, int b, int region){
    if (kind <= FEED_NONE || kind >= FEED_COUNT) return;
    FeedEntry *e = &g_feed[g_feed_seq % FEED_CAP];
    g_feed_seq++;
    e->seq = g_feed_seq; e->year = g_year; e->kind = kind;
    e->a = a; e->b = b; e->region = region;
}
int feed_poll(int after_seq, FeedEntry *out, int max){
    if (!out || max <= 0) return 0;
    int first = g_feed_seq - FEED_CAP + 1;                 /* plus ancien encore dans l'anneau */
    if (first < 1) first = 1;
    if (after_seq + 1 > first) first = after_seq + 1;
    int n = 0;
    for (int s = first; s <= g_feed_seq && n < max; s++)
        out[n++] = g_feed[(s - 1) % FEED_CAP];
    return n;
}

void provlog_reset(void){
    memset(g_log, 0, sizeof g_log);
    memset(g_total, 0, sizeof g_total);
    memset(g_prevmask, 0, sizeof g_prevmask);
    memset(g_primed, 0, sizeof g_primed);
    g_year = 0;
    memset(g_feed, 0, sizeof g_feed);                      /* le fil d'évènements repart à vide */
    g_feed_seq = 0;
}

void provlog_set_year(int year){ g_year = year; }

static void push(int region, int str_id, const char *lit, int sign, int eff_str, unsigned eff_dir){
    if (region < 0 || region >= SCPS_MAX_REG) return;
    ProvLogEntry *e = &g_log[region][g_total[region] % PROVLOG_CAP];
    e->year = g_year; e->str_id = str_id; e->lit = lit;
    e->sign = (signed char)(sign < 0 ? -1 : (sign > 0 ? 1 : 0));
    e->eff_str = eff_str; e->eff_dir = eff_dir;
    g_total[region]++;
}

void provlog_push_mod(int region, int str_id, int sign, int eff_str){ push(region, str_id, 0, sign, eff_str, 0u); }
void provlog_push_event(int region, const char *lit, int sign, unsigned eff_dir){ if (lit) push(region, -1, lit, sign, -1, eff_dir); }

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
            provlog_push_mod(region, (int)JMOD_STR[b], JMOD_SIGN[b], (int)JMOD_EFF[b]);
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
