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

/* ── L'ASSIETTE DU COURANT (§4.3bis) ─────────────────────────────────────
 * Le courant politique adopté DÉPLACE la base de génération sur SA classe.
 * Tout se somme sur les PROVINCES (prov[], jamais region[].pop — doctrine
 * « la province est la seule réalité économique »). L'assiette Divine est la
 * FOI BÂTIE (build.faith) relevée par la ferveur moyenne : volatile par nature. */
static double infl_class_pop(const WorldEconomy *econ, int cid, int klass){
    if (!econ || cid<0) return 0.0;
    double s = 0.0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++)
        if (econ->prov[p].owner == cid) s += econ->prov[p].strata[klass].pop;
    return s;
}
/* Σ foi bâtie × (1 + ferveur MOYENNE des provinces du pays). */
static double infl_faith_base(const WorldEconomy *econ, int cid){
    if (!econ || cid<0) return 0.0;
    double faith = 0.0, ferv = 0.0; int n = 0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++){
        if (econ->prov[p].owner != cid) continue;
        faith += econ->prov[p].build.faith;
        ferv  += econ->prov[p].ferveur;
        n++;
    }
    double favg = (n>0) ? (ferv/(double)n) : 0.0;
    if (favg < 0.0) favg = 0.0; else if (favg > 1.0) favg = 1.0;
    return faith * (1.0 + favg);
}

double influence_base_pop(const WorldEconomy *econ, int cid, InfluenceBase base){
    switch (base){
      case INFL_BASE_BOURGEOIS: return infl_class_pop(econ, cid, CLASS_BOURGEOIS);
      case INFL_BASE_LABORER:   return infl_class_pop(econ, cid, CLASS_LABORER);
      case INFL_BASE_FAITH:     return infl_faith_base(econ, cid);
      case INFL_BASE_ARISTO:
      case INFL_BASE_DEFAUT:
      default:                  return influence_elites(econ, cid);
    }
}

double influence_base_gain(const WorldEconomy *econ, int cid, InfluenceBase base){
    double pop = influence_base_pop(econ, cid, base);
    float rate;
    switch (base){
      case INFL_BASE_ARISTO:    rate = tune_f("INFLUENCE_PER_NOBLE_ARISTO", 0.0025f); break;
      case INFL_BASE_BOURGEOIS: rate = tune_f("INFLUENCE_PER_BOURGEOIS",    0.0006f); break;
      case INFL_BASE_LABORER:   rate = tune_f("INFLUENCE_PER_LABORER",      0.00012f); break;
      case INFL_BASE_FAITH:     rate = tune_f("INFLUENCE_PER_FAITH",        0.08f); break;
      case INFL_BASE_DEFAUT:
      default:                  rate = tune_f("INFLUENCE_PER_NOBLE",        0.002f); break;
    }
    return (double)rate * pop;
}

float influence_scale(const WorldEconomy *econ, int cid, InfluenceBase base){
    float ref = tune_f("INFLUENCE_BASE_REF", 2.0f);
    if (ref <= 0.f) return 1.f;                     /* kill-switch : prix PLATS (d'avant la linéarisation) */
    double e = influence_base_gain(econ, cid, base) / (double)ref;   /* l'assiette SEULE — jamais × le Conseil */
    if (e < 0.25) e = 0.25;                         /* plancher : un micro-pays n'a pas des doctrines gratuites */
    return (float)e;
}

void influence_tick(InfluenceState *is, const World *w, const WorldEconomy *econ,
                     const Statecraft *sc, uint32_t seed, int cid, InfluenceBase base){
    (void)w;
    if (!is || cid<0 || cid>=SCPS_MAX_COUNTRY) return;
    float  mult   = influence_council_mult(sc, seed, cid, NULL);
    double gain   = influence_base_gain(econ, cid, base) * (double)mult;   /* TOUJOURS × le rang du Conseil */
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
