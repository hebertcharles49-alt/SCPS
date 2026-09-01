/*
 * scps_influence.c — implémentation de l'Influence politique (voir scps_influence.h).
 */
#include "scps_influence.h"
#include "scps_tune.h"   /* tune_f : INFLUENCE_PER_NOBLE/COUNCIL_FLOOR/CAP (registre J) */
#include <string.h>

void influence_init(InfluenceState *is){
    if (!is) return;
    memset(is->influence, 0, sizeof is->influence);
}

double influence_elites(const WorldEconomy *econ, int cid){
    if (!econ || cid<0) return 0.0;
    double e = 0.0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++)
        if (econ->prov[p].owner == cid) e += econ->prov[p].strata[CLASS_ELITE].pop;
    return e;
}

float influence_council_mult(const Statecraft *sc, uint32_t seed, int cid, int *out_n_seated){
    float floor_mult = tune_f("INFLUENCE_COUNCIL_FLOOR", 1.0f);
    if (out_n_seated) *out_n_seated = 0;
    if (!sc || cid<0 || cid>=SCPS_MAX_COUNTRY) return floor_mult;
    int n = 0; float sum = 0.f;
    for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
        int slot = statecraft_council_seated(sc, cid, seat);
        if (slot < 0) continue;   /* siège vacant : ne compte pas dans la moyenne */
        int gen = statecraft_council_seated_gen(sc, cid, seat);
        int tier = statecraft_council_cand_tier(seed, cid, seat, slot, gen);   /* I=1..III=3 */
        sum += (float)tier; n++;
    }
    if (out_n_seated) *out_n_seated = n;
    if (n == 0) return floor_mult;   /* AUCUN siège pourvu : plancher (sinon un Conseil vide
                                      * rend le joueur muet en diplomatie — décision signalée) */
    return sum / (float)n;
}

void influence_tick(InfluenceState *is, const World *w, const WorldEconomy *econ,
                     const Statecraft *sc, uint32_t seed, int cid){
    (void)w;
    if (!is || cid<0 || cid>=SCPS_MAX_COUNTRY) return;
    double elites = influence_elites(econ, cid);
    float  mult   = influence_council_mult(sc, seed, cid, NULL);
    double gain   = (double)tune_f("INFLUENCE_PER_NOBLE", 0.002f) * elites * (double)mult;
    float v = is->influence[cid] + (float)gain;
    float cap = tune_f("INFLUENCE_CAP", 0.0f);   /* 0 = sans plafond (décision joueur 2026-09-01) */
    if (cap > 0.f && v > cap) v = cap;
    if (v < 0.f) v = 0.f;
    is->influence[cid] = v;
}

float influence_get(const InfluenceState *is, int cid){
    if (!is || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0.f;
    return is->influence[cid];
}

int influence_can_spend(const InfluenceState *is, int cid, float cost){
    if (!is || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    if (cost <= 0.f) return 1;
    return is->influence[cid] >= cost - 0.001f;
}

void influence_spend(InfluenceState *is, int cid, float cost){
    if (!is || cid<0 || cid>=SCPS_MAX_COUNTRY || cost<=0.f) return;
    float v = is->influence[cid] - cost;
    is->influence[cid] = (v > 0.f) ? v : 0.f;
}
