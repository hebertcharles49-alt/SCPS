/*
 * scps_influence.c — implémentation de l'Influence politique (voir scps_influence.h).
 */
#include "scps_influence.h"
#include "scps_tune.h"      /* tune_f : INFLUENCE_PER_NOBLE/COUNCIL_FLOOR/CAP (registre J) */
#include "scps_religion.h"  /* religion_of_country : les FIDÈLES de Divin (grain GROUPE, jamais region[]) */
#include <string.h>

/* TÉLÉMÉTRIE (print-only, chronicle) — Σ influence GÉNÉRÉE depuis la genèse de
 * CETTE sim. Statique de module, JAMAIS sérialisée (motif econ_colony_stats) :
 * RAZ à influence_init, donc à chaque sim_init. N'entre dans aucun calcul. */
static double g_infl_generated = 0.0;

void influence_init(InfluenceState *is){
    g_infl_generated = 0.0;
    if (!is) return;
    memset(is->influence, 0, sizeof is->influence);
}

void influence_stats_get(double *generated){
    if (generated) *generated = g_infl_generated;
}

/* LA DOUBLE RÉALITÉ DE CLASSE (piège 2026-09-02) : `prov[].strata[k].pop` est la
 * strate MOBILE par richesse (~1-3 % d'élites) — la classe qui SE PROMEUT/RÉTROGRADE.
 * `prov[].pop.groups[i].pop_by_class[k]` est le SIÈGE d'emploi (édifices, capitale),
 * recalculé au tick par demography_emerge_classes — ~13 % d'élites, 8 % bourgeois,
 * 78 % journaliers (chronicle « SIÈGES »). La NOBLESSE du design politique, c'est
 * LA SECONDE (des offices tenus, pas une aptitude à s'enrichir) : l'assiette lit les
 * SIÈGES, jamais les strates. Σ sur les PROVINCES du pays (prov[], jamais region[].pop
 * stale — doctrine « la province est la seule réalité économique »), Σ sur les
 * GROUPES de chaque province (exactement la somme du chronicle, chronicle.c:1762-1768). */
double influence_elites(const WorldEconomy *econ, int cid){
    if (!econ || cid<0) return 0.0;
    double e = 0.0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++){
        if (econ->prov[p].owner != cid) continue;
        const ProvincePop *pp = &econ->prov[p].pop;
        for (int gi=0; gi<pp->n_groups && gi<SCPS_MAX_GROUPS; gi++)
            e += (double)pp->groups[gi].pop_by_class[CLASS_ELITE];
    }
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

/* ── L'ASSIETTE — les TROIS classes, TOUJOURS (§3.1bis, §4.3bis) ─────────
 * L'assiette par défaut SOMME les trois classes (sièges — pop_by_class, jamais
 * les strates par richesse, cf. influence_elites ci-dessus) : élites × NOBLE +
 * bourgeois × BOURGEOIS_BASE + journaliers × LABORER_BASE. Sur une pop assise
 * ~13/8/78 %, ça pose ~60/20/20 % de parts de gain (les taux SONT la pondération
 * politique : un noble « pèse » ~18× un journalier).
 * Le COURANT adopté ne REMPLACE plus l'assiette (design pré-2026-09-02) : il
 * RELÈVE le taux de SA seule classe (les deux autres restent au taux _BASE) —
 * jamais un malus, toujours ≥ l'assiette par défaut, puisqu'un seul terme monte.
 * Divin ne relève aucun taux de classe : il AJOUTE le terme des FIDÈLES (la
 * religion vit au grain GROUPE — PopGroup.faith, jamais region[]/province
 * représentative) à l'assiette par défaut. Sans religion fondée (religion_of_
 * country<0) : terme nul, jamais un malus. */
static double infl_class_pop(const WorldEconomy *econ, int cid, int klass){
    if (!econ || cid<0) return 0.0;
    double s = 0.0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++){
        if (econ->prov[p].owner != cid) continue;
        const ProvincePop *pp = &econ->prov[p].pop;
        for (int gi=0; gi<pp->n_groups && gi<SCPS_MAX_GROUPS; gi++)
            s += (double)pp->groups[gi].pop_by_class[klass];
    }
    return s;
}
/* Σ des ÂMES (PopGroup.count) des groupes qui professent la religion D'ÉTAT du
 * pays (religion_of_country) — grain GROUPE, jamais la province représentative
 * ni region[].culture. Pays athée (rid<0) ⇒ 0. */
static double infl_believers(const WorldEconomy *econ, int cid){
    if (!econ || cid<0) return 0.0;
    int rid = religion_of_country(cid);
    if (rid < 0) return 0.0;
    double s = 0.0;
    for (int p=0; p<econ->n_prov && p<SCPS_MAX_PROV; p++){
        if (econ->prov[p].owner != cid) continue;
        const ProvincePop *pp = &econ->prov[p].pop;
        for (int gi=0; gi<pp->n_groups && gi<SCPS_MAX_GROUPS; gi++)
            if (pp->groups[gi].faith == rid) s += (double)pp->groups[gi].count;
    }
    return s;
}

/* Les TROIS effectifs de l'assiette (sièges), pour le hover en MOTS —
 * INDÉPENDANTS du courant actif (qui ne fait qu'ÉLEVER le taux de SA classe,
 * jamais remplacer les deux autres). */
void influence_seats(const WorldEconomy *econ, int cid,
                      double *elites, double *bourgeois, double *laborers){
    if (elites)    *elites    = infl_class_pop(econ, cid, CLASS_ELITE);
    if (bourgeois) *bourgeois = infl_class_pop(econ, cid, CLASS_BOURGEOIS);
    if (laborers)  *laborers  = infl_class_pop(econ, cid, CLASS_LABORER);
}

/* L'effectif SIMPLE associé à un courant (pour les bancs / lecteurs qui ne
 * veulent qu'UN nombre) : nobles pour DEFAUT/ARISTO, bourgeois/journaliers pour
 * leur courant, fidèles pour Divin. */
double influence_base_pop(const WorldEconomy *econ, int cid, InfluenceBase base){
    switch (base){
      case INFL_BASE_BOURGEOIS: return infl_class_pop(econ, cid, CLASS_BOURGEOIS);
      case INFL_BASE_LABORER:   return infl_class_pop(econ, cid, CLASS_LABORER);
      case INFL_BASE_FAITH:     return infl_believers(econ, cid);
      case INFL_BASE_ARISTO:
      case INFL_BASE_DEFAUT:
      default:                  return infl_class_pop(econ, cid, CLASS_ELITE);
    }
}

double influence_base_gain(const WorldEconomy *econ, int cid, InfluenceBase base){
    double elites    = infl_class_pop(econ, cid, CLASS_ELITE);
    double bourgeois = infl_class_pop(econ, cid, CLASS_BOURGEOIS);
    double laborers  = infl_class_pop(econ, cid, CLASS_LABORER);

    /* seul le taux de LA classe du courant actif est relevé ; les deux autres
     * restent au taux _BASE — la somme ne peut donc que MONTER (jamais un malus). */
    float rate_e = (base==INFL_BASE_ARISTO)
                 ? tune_f("INFLUENCE_PER_NOBLE_ARISTO", 0.0025f)
                 : tune_f("INFLUENCE_PER_NOBLE",         0.002f);
    float rate_b = (base==INFL_BASE_BOURGEOIS)
                 ? tune_f("INFLUENCE_PER_BOURGEOIS",      0.0022f)
                 : tune_f("INFLUENCE_PER_BOURGEOIS_BASE",  0.0011f);
    float rate_l = (base==INFL_BASE_LABORER)
                 ? tune_f("INFLUENCE_PER_LABORER",         0.00022f)
                 : tune_f("INFLUENCE_PER_LABORER_BASE",     0.00011f);

    double gain = elites*(double)rate_e + bourgeois*(double)rate_b + laborers*(double)rate_l;
    if (base == INFL_BASE_FAITH)
        gain += infl_believers(econ, cid) * (double)tune_f("INFLUENCE_PER_BELIEVER", 0.00016667f);
    return gain;
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
    g_infl_generated += (double)(v - is->influence[cid]);   /* télémétrie : le gain RÉELLEMENT crédité (post-plafond) */
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
