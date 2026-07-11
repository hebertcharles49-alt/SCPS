/*
 * scps_missions.c — LES MISSIONS DÉCENNALES (factions §8)
 *
 * Contextuelles : le KIND tourne par décennie, la CIBLE vient de l'état du pays —
 * l'éthos dominant choisit l'institution à bâtir, la production choisit la chaîne à
 * renforcer, la recherche choisit le palier à percer. Récompense ∝ ampleur.
 */
#include "scps_missions.h"
#include "scps_factions.h"   /* la faction dominante oriente la mission de bâti */
#include "scps_tune.h"       /* P3 : COUNCIL_MISSION_* (registre J) */
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Les six coordonnées bâties adressables (MIS_COORD_*, cf. scps_missions.h) + le
 * mot de la cible. */
static float build_coord(const ProvBuild *b, int c){
    switch (c){ case MIS_COORD_K: return b->K_inst; case MIS_COORD_PE: return b->PE_infra;
                case MIS_COORD_FAITH: return b->faith; case MIS_COORD_SAVOIR: return b->savoir;
                case MIS_COORD_H: return b->H_coerc;  default: return b->food_cap; }
}
static const char *coord_word(int c){
    switch (c){ case MIS_COORD_K: return "les institutions"; case MIS_COORD_PE: return "le commerce";
                case MIS_COORD_FAITH: return "la foi"; case MIS_COORD_SAVOIR: return "le savoir";
                case MIS_COORD_H: return "la garde";  default: return "les vivres"; }
}
/* L'éthos dominant choisit la coordonnée à ériger (ce que la faction veut de l'État). */
static int coord_for_faction(EthosFaction f){
    switch (f){ case FAC_CONQUERANT: return MIS_COORD_H;   case FAC_MARCHAND: return MIS_COORD_PE;
                case FAC_LEGISTE:    return MIS_COORD_K;   case FAC_GARDIEN:  return MIS_COORD_FAITH;
                case FAC_TRANSGRESSEUR: return MIS_COORD_SAVOIR; default: return MIS_COORD_FOOD; }
}
/* P3 — le SIÈGE responsable, déduit du type (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) :
 * MIS_TECH→Savoir(0) · MIS_CHAIN→Ouvrages(2) · MIS_BUILD savoir/foi→Savoir(0) ·
 * institutions/garde/vivres→Royaume(1) · commerce→Ouvrages(2). */
int mission_responsible_seat(const Mission *m){
    if (!m) return -1;
    if (m->kind==MIS_TECH)  return 0;
    if (m->kind==MIS_CHAIN) return 2;
    if (m->kind==MIS_BUILD){
        switch (m->coord){
            case MIS_COORD_SAVOIR: case MIS_COORD_FAITH: return 0;   /* Savoir */
            case MIS_COORD_PE:                            return 2;   /* Ouvrages (commerce) */
            default:                                      return 1;   /* Royaume (institutions/garde/vivres) */
        }
    }
    return -1;
}
/* P3 — pénalise/récompense la LOYAUTÉ du titulaire du siège responsable (le
 * SUCCESSEUR reprend : on relit le siège pourvu MAINTENANT, aucun id figé). */
static void mission_seat_loyalty(Statecraft *sc, int cid, const Mission *m, float delta){
    if (!sc || !m) return;
    int seat = mission_responsible_seat(m);
    if (seat<0) return;
    statecraft_council_loyalty_add(sc, cid, seat, delta);   /* no-op si vacant */
}

static int capital_region(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0||cid>=w->n_countries) return -1;
    int cp=w->country[cid].capital_prov; if (cp<0||cp>=w->n_provinces) return -1;
    int cr=w->province[cp].region; return (cr>=0&&cr<econ->n_regions)?cr:-1;
}
static int has_regions(const WorldEconomy *econ, int cid){
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid) return 1;
    return 0;
}
static float country_stock(const WorldEconomy *econ, int cid, Resource g){
    float s=0.f; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid) s+=econ->region[r].stock[g];
    return s;
}

void missions_init(MissionsState *ms){ memset(ms,0,sizeof *ms); }

/* ---- Émission contextuelle ------------------------------------------- */
static Mission mission_roll(const World *w, WorldEconomy *econ, const TechState *ts,
                            int cid, int year){
    Mission m; memset(&m,0,sizeof m);
    m.issued_year=year; m.active=true; m.done=false;
    int cr=capital_region(w,econ,cid);
    int decade=year/10;
    MissionKind kind = (MissionKind)(MIS_BUILD + (decade%3));   /* tourne BÂTIR/CHAÎNE/TECH */
    m.kind=kind;
    if (kind==MIS_BUILD){
        float fw[FAC_COUNT]; EthosFaction dom=country_faction_weights(w,econ,cid,fw);
        m.coord=coord_for_faction(dom);
        float cur = (cr>=0)?build_coord(&econ->region[cr].build, m.coord):0.f;
        m.threshold = cur + 2.0f;                          /* deux paliers de plus */
        m.reward_gold = 320.f; m.reward_mat=RES_IRON; m.reward_qty=60.f;
        snprintf(m.text,sizeof m.text, "Ériger %s (atteindre %.0f)", coord_word(m.coord), m.threshold);
    } else if (kind==MIS_CHAIN){
        m.good = RES_TOOLS;                                /* la chaîne d'outils (multiplicateur) */
        float cur = country_stock(econ,cid,m.good);
        m.threshold = cur + 80.f;
        m.reward_gold = 280.f; m.reward_mat=RES_WOOD; m.reward_qty=120.f;
        snprintf(m.text,sizeof m.text, "Renforcer la chaîne d'outils (stock %.0f)", m.threshold);
    } else { /* MIS_TECH */
        float cur = (cid<SCPS_MAX_COUNTRY)?(float)ts[cid].n_unlocked:0.f;
        m.threshold = cur + 3.f;                           /* percer trois technologies de plus */
        m.reward_gold = 360.f; m.reward_mat=RES_IRON; m.reward_qty=40.f;
        snprintf(m.text,sizeof m.text, "Percer la connaissance (atteindre %.0f techs)", m.threshold);
    }
    return m;
}

static bool mission_check(const World *w, WorldEconomy *econ, const TechState *ts,
                          int cid, const Mission *m){
    if (m->kind==MIS_BUILD){
        int cr=capital_region(w,econ,cid);
        return (cr>=0) && build_coord(&econ->region[cr].build, m->coord) >= m->threshold;
    } else if (m->kind==MIS_CHAIN){
        return country_stock(econ,cid,m->good) >= m->threshold;
    } else {
        return (cid<SCPS_MAX_COUNTRY) && (float)ts[cid].n_unlocked >= m->threshold;
    }
}

/* P3 — le bonus de récompense du siège responsable : PER_RANK × (rang−1) × efficacité
 * (I×0 · II×1×PER_RANK · III×2×PER_RANK), 1.0 (aucun bonus) si le siège est VACANT —
 * pas de titulaire à créditer. `sc`/`wp` NULL ⇒ mult=1 (mission neutre). */
static float mission_reward_mult(const Statecraft *sc, const WorldProsperity *wp,
                                 uint32_t seed, int cid, const Mission *m){
    if (!sc) return 1.f;
    int seat = mission_responsible_seat(m);
    if (seat<0) return 1.f;
    int slot = statecraft_council_seated(sc,cid,seat);
    if (slot<0) return 1.f;
    int gen  = statecraft_council_seated_gen(sc,cid,seat);
    int tier = statecraft_council_cand_tier(seed,cid,seat,slot,gen);
    float eff = statecraft_council_efficiency(sc,wp,cid,seat);
    return 1.f + tune_f("COUNCIL_MISSION_REWARD_PER_RANK",0.05f) * (float)(tier-1) * eff;
}

static void mission_grant(const World *w, WorldEconomy *econ, Statecraft *sc, const WorldProsperity *wp,
                          uint32_t seed, int cid, const Mission *m, MissionsState *ms){
    /* récompense versée à la CAPITALE (le siège) — cohérent avec mission_check, qui VÉRIFIE le bâti sur
     * capital_region(). L'ancien « 1re région possédée » (plus bas index) coïncidait avec la capitale sur
     * les anciens mondes ; un monde re-baseliné peut les dissocier → la récompense tombait à côté. */
    int cr=capital_region(w,econ,cid);
    if (cr<0) return;
    float mult = mission_reward_mult(sc,wp,seed,cid,m);            /* P3 : bonus ∝ rang×efficacité du siège responsable */
    /* raccord 7 (Âge des Héros) — « Lui confier »/« Lui donner les clefs » posent un bonus
     * (mult>0) sur le SIÈGE responsable, identité (slot,gen) figée au moment du choix. Il ne
     * s'applique QUE si le titulaire n'a pas changé depuis (sinon le successeur ne le reçoit
     * pas) — CONSOMMÉ dans les deux cas (utilisé, ou perdu). */
    { int seat = mission_responsible_seat(m);
      if (ms && seat>=0 && seat<SC_COUNCIL_SEATS && cid>=0 && cid<SCPS_MISSIONS_MAX){
          HeroMissionBonus *hb = &ms->hero_bonus[cid][seat];
          if (hb->mult > 0.f){
              int slot = statecraft_council_seated(sc,cid,seat);
              int gen  = statecraft_council_seated_gen(sc,cid,seat);
              if (slot==hb->slot && gen==hb->gen) mult *= hb->mult;
              hb->mult = 0.f;   /* consommée (appliquée ou perdue) */
          }
      } }
    /* RE-KEY PROVINCE : treasury province-owned — route sur la représentative.
     * region[].stock[] est un REFLET reconstruit EN ENTIER depuis prov[] à chaque
     * econ_aggregate_regions — PAS « le marché, resté au grain région, intact » (le
     * faux mantra qui a produit ce trou, Lot B 2026-07-07) : une écriture directe s'y
     * évapore (≤ 30 j). Route par econ_region_stock_add (province représentative
     * d'abord, sœurs en débordement) comme le trésor ci-dessus. */
    int crp=econ_region_rep_province(econ,cr);
    if (crp>=0 && crp<econ->n_prov) econ->prov[crp].treasury += m->reward_gold * mult;   /* or au trésor */
    if (m->reward_mat>RES_NONE && m->reward_mat<RES_COUNT)
        econ_region_stock_add(econ, cr, m->reward_mat, m->reward_qty * mult);  /* matières au marché */
    mission_seat_loyalty(sc, cid, m, tune_f("COUNCIL_MISSION_SUCCESS_LOYALTY",5.f));   /* P3 : réussite → +loyauté */
}

void missions_tick(MissionsState *ms, const World *w, WorldEconomy *econ,
                   const TechState *ts, Statecraft *sc, const WorldProsperity *wp,
                   uint32_t seed, int year){
    for (int c=0;c<w->n_countries && c<SCPS_MISSIONS_MAX;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || !has_regions(econ,c)) continue;
        Mission *m=&ms->m[c];
        m->just_completed=false;   /* raccord 7 : RAZ à chaque appel — le signal ne vit qu'UN tour */
        if (year%10==0 && (!m->active || m->issued_year!=year)){  /* nouvelle décennie : mission fraîche */
            /* P3 — l'ÉCHEC est RÉSOLU ICI, AVANT l'émission de la suivante (fin du
             * remplacement silencieux) : une mission ACTIVE et INACHEVÉE au moment
             * de la décennie suivante est un échec pour le siège responsable. */
            if (m->active && !m->done)
                mission_seat_loyalty(sc, c, m, -tune_f("COUNCIL_MISSION_FAILURE_LOYALTY",10.f));
            *m = mission_roll(w,econ,ts,c,year);
        }
        if (m->active && !m->done && mission_check(w,econ,ts,c,m)){
            m->done=true; m->just_completed=true;
            mission_grant(w,econ,sc,wp,seed,c,m,ms);   /* accomplie → récompense (au siège) + loyauté */
        }
    }
}

const Mission *mission_of(const MissionsState *ms, int cid){
    if (cid<0||cid>=SCPS_MISSIONS_MAX) return NULL;
    return ms->m[cid].active ? &ms->m[cid] : NULL;
}
