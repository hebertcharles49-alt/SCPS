/*
 * scps_endgame.c — Capstone §27 : Entropie mondiale + 4 fins + Merveille.
 *
 * Lit des structs moteur (econ/prosperity/tech/world) mais reste autonome.
 * N'EST JAMAIS inclus par viewer.c (la membrane passe par EndgameReadout).
 *
 * Séquence du tick orchestrateur :
 *   C1 : élargir l'entropie (charge de tech faustienne — le SAVOIR)
 *   C2 : select_and_fire si !fired (seuil ENTROPY_FIN → latch d'une fin)
 *   C3 : EAU — carve radiale (rift) → recalcul ciblé → refragmentation géo
 *   C4-C5 : froid / ronces — modules suivants
 *   C6 : wonder_tick (FIN_ASCENSION) — module suivant
 */
#include "scps_endgame.h"
#include "scps_world.h"   /* world_sink_cell, world_recompute_adjacency, country_make_name */
#include "scps_tune.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>   /* getenv : diag de calibration (SCPS_ENTDIAG), OFF par défaut */
#include <math.h>     /* cosf/sinf : les bras radiaux du rift */

/* ── Formes du cataclysme (capacités/géométrie — PAS le registre J) ──────── */
#define RIFT_ARMS         5     /* bras radiaux du rift d'eau */
#define RIFT_ARM_LEN     96     /* portée d'un bras (cellules) depuis l'épicentre */
#define RIFT_ARM_STEP     3     /* pas d'échantillonnage le long d'un bras */
#define SPLIT_VIABLE_MIN  2     /* un fragment de ≥ N régions sécède ; sinon il est perdu */

void endgame_init(EndgameState *eg) {
    memset(eg, 0, sizeof *eg);
    eg->epicenter_reg   = -1;
    eg->fauteur_country = -1;
    eg->fin_year        = -1;
    eg->merv_country    = -1;
    eg->merv_site_reg   = -1;
}

/* ── C1 — élargir la source d'entropie ────────────────────────────────────────
 * prosperity_tick a posé wp->entropy = Σ faust_charge régional (la transgression
 * ACTIVE des transmuteurs ; plate à 0 en monde stable). On AJOUTE la composante
 * « pression du savoir » : la charge de TECH faustienne accumulée par empire
 * (celle qui ramène déjà l'Âge de la Brèche). Recalculé à neuf chaque tick
 * (prosperity_tick réassigne wp->entropy = esum) → l'ajout est NON cumulatif
 * inter-ticks. C'est une vraie ENTRÉE moteur (la charge), jamais un bonus plat. */
static void endgame_entropy_widen(WorldProsperity *wp, const TechState ts[], int nc) {
    if (!wp || !ts) return;
    float tech_ent = 0.f;
    for (int c = 0; c < nc && c < SCPS_MAX_COUNTRY; c++) tech_ent += ts[c].charge;
    wp->entropy += tune_f("ENTROPY_TECH_W", 1.0f) * tech_ent;
}

/* Foyer de REPLI : si aucune région ne porte de faust_charge (entropie TECH-driven
 * pure → entropy_epicenter == -1), le foyer suit l'empire le PLUS faustien (max
 * ts[].charge) et sa capitale. Renvoie le pays fauteur ; pose *out_reg. */
static int endgame_pick_fauteur(const World *w, const TechState ts[], int *out_reg) {
    int best = -1; float bestc = -1.f;
    for (int c = 0; c < w->n_countries && c < SCPS_MAX_COUNTRY; c++) {
        if (w->country[c].role == POLITY_UNCLAIMED) continue;
        if (ts[c].charge > bestc) { bestc = ts[c].charge; best = c; }
    }
    *out_reg = -1;
    if (best >= 0) {
        int cap = w->country[best].capital_prov;
        if (cap >= 0 && cap < w->n_provinces) *out_reg = w->province[cap].region;
    }
    return best;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * C3 — APOCALYPSE D'EAU (essence) : carve radiale → recalcul → fragmentation
 * ──────────────────────────────────────────────────────────────────────────── */

/* Barycentre (cellule centrale) des cellules d'une région. */
static void region_centroid(const World *w, int r, int *ox, int *oy) {
    long sx = 0, sy = 0, n = 0;
    for (int y = 0; y < SCPS_H; y++) for (int x = 0; x < SCPS_W; x++)
        if (w->cell[scps_idx(x,y)].region == r) { sx += x; sy += y; n++; }
    if (n > 0) { *ox = (int)(sx / n); *oy = (int)(sy / n); }
    else       { *ox = SCPS_W / 2;    *oy = SCPS_H / 2;    }
}

/* Un slot de pays RÉUTILISABLE (UNCLAIMED, ne tient aucune région), sinon agrandit
 * la table — copie de l'idiome de la sécession (scps_revolt). */
static int endgame_free_slot(World *w, const WorldEconomy *econ, int avoid) {
    for (int c = 0; c < w->n_countries; c++) {
        if (c == avoid || w->country[c].role != POLITY_UNCLAIMED) continue;
        int held = 0; for (int r = 0; r < econ->n_regions; r++) if (econ->region[r].owner == c) { held = 1; break; }
        if (!held) return c;
    }
    if (w->n_countries >= SCPS_MAX_COUNTRY) return -1;
    return w->n_countries++;
}

/* Pose le pays `newc` sur TOUTES les cellules d'une région + la hiérarchie World.
 * newc<0 (région perdue) : cells/region → -1, mais province.country INTACTE (save_sane
 * exige province.country ≥ 0 ; l'autorité d'occupation est econ->region.owner). */
static void region_set_country(World *w, int r, int newc) {
    for (int y = 0; y < SCPS_H; y++) for (int x = 0; x < SCPS_W; x++) {
        Cell *c = &w->cell[scps_idx(x,y)];
        if (c->region == r) c->country = (int16_t)newc;
    }
    if (r >= 0 && r < w->n_regions) w->region[r].country = (int16_t)newc;
    if (newc >= 0)
        for (int p = 0; p < w->n_provinces; p++)
            if (w->province[p].region == r) w->province[p].country = (int16_t)newc;
}

/* LA CARVE : engloutit une région — cellules → mer (hiérarchie strippée), éco
 * dépeuplée/sans maître/infranchissable, armées échouées. La région GARDE son
 * indice (on ne réordonne rien : save_sane reste satisfait). */
static void cataclysm_sink_region(World *w, WorldEconomy *econ, Campaign *camp, int r) {
    if (r < 0 || r >= econ->n_regions) return;
    /* 1. cellules : terre → mer, hiérarchie strippée. */
    for (int y = 0; y < SCPS_H; y++) for (int x = 0; x < SCPS_W; x++) {
        Cell *c = &w->cell[scps_idx(x,y)];
        if (c->region != r) continue;
        world_sink_cell(c, SEA_LEVEL - 0.03f);
    }
    /* 2. éco région : dépeuplée, sans maître, infranchissable. */
    RegionEconomy *re = &econ->region[r];
    for (int cl = 0; cl < CLASS_COUNT; cl++) re->strata[cl].pop = 0.f;
    re->pop.n_groups = 0;
    re->owner = -1; re->active = false; re->colonized = false; re->impassable = true;
    re->coastal = false; re->n_bld = 0; re->cap_pop = 0.f; re->diaspora_pop = 0.f;
    for (int g = 0; g < RES_COUNT; g++) { re->stock[g]=0.f; re->supply[g]=0.f; re->demand[g]=0.f; re->raw_cap[g]=0.f; }
    /* 3. hiérarchie World : la région perd son pays (l'indice reste ; province.country
     *    reste ≥ 0 pour save_sane — labels orphelins inoffensifs, l'éco fait autorité). */
    if (r < w->n_regions) w->region[r].country = -1;
    /* 4. armées échouées : toute force dont la position/cible est cette région tombe. */
    if (camp) for (int c = 0; c < SCPS_MAX_COUNTRY; c++) {
        FieldArmy *a = &camp->army[c];
        if (a->active && (a->loc == r || a->dest == r || a->next == r)) {
            a->active = false; a->dest = -1; a->next = -1;
        }
    }
}

/* REFRAGMENTATION (BFS géo) : un empire scindé par la mer se brise en composantes
 * connexes (sur econ->adj recalculée). Le plus GROS fragment garde l'identité ; les
 * fragments VIABLES (≥ SPLIT_VIABLE_MIN régions) sécèdent dans un slot libre ; les
 * trop petits sont PERDUS (owner=-1, terre vierge). Reconstruit region_ids + capitale.
 * Renvoie le nb de NOUVEAUX pays nés (pour savoir s'il faut redessiner les frontières). */
static int cataclysm_resplit_empire(World *w, WorldEconomy *econ, int country) {
    if (country < 0 || country >= w->n_countries) return 0;
    if (w->country[country].role == POLITY_UNCLAIMED) return 0;
    int regs[SCPS_MAX_REG], nr = 0;
    for (int r = 0; r < econ->n_regions && nr < SCPS_MAX_REG; r++)
        if (econ->region[r].owner == country) regs[nr++] = r;

    /* Reconstruit toujours region_ids depuis l'éco (les régions englouties sont
     * déjà owner=-1) + recale la capitale si elle a sombré. */
    Country *ct = &w->country[country];
    int born = 0;

    if (nr >= 2) {
        /* composantes connexes (BFS sur l'adjacence des régions de CE pays). */
        int comp[SCPS_MAX_REG]; for (int i = 0; i < nr; i++) comp[i] = -1;
        int queue[SCPS_MAX_REG], ncomp = 0;
        for (int i = 0; i < nr; i++) {
            if (comp[i] >= 0) continue;
            int qh = 0, qt = 0; comp[i] = ncomp; queue[qt++] = i;
            while (qh < qt) {
                int ra = regs[queue[qh++]];
                for (int j = 0; j < nr; j++)
                    if (comp[j] < 0 && econ->adj[ra][regs[j]]) { comp[j] = ncomp; queue[qt++] = j; }
            }
            ncomp++;
        }
        if (ncomp >= 2) {
            int sz[SCPS_MAX_REG] = {0};
            for (int i = 0; i < nr; i++) sz[comp[i]]++;
            int big = 0; for (int k = 1; k < ncomp; k++) if (sz[k] > sz[big]) big = k;
            for (int k = 0; k < ncomp; k++) {
                if (k == big) continue;
                if (sz[k] >= SPLIT_VIABLE_MIN) {
                    int firstreg = -1; for (int i = 0; i < nr; i++) if (comp[i] == k) { firstreg = regs[i]; break; }
                    int newc = endgame_free_slot(w, econ, country);
                    if (newc >= 0) {                       /* un successeur naît */
                        SpeciesArchetype race = econ->region[firstreg].culture.race;
                        Ethos ethos = econ->region[firstreg].culture.ethos;
                        Country *nc = &w->country[newc]; memset(nc, 0, sizeof *nc);
                        nc->role = POLITY_ANTAGONIST;
                        nc->continent = (firstreg < w->n_regions) ? w->region[firstreg].continent : 0;
                        nc->color = country_race_color(race, newc);
                        country_make_name(nc->name, sizeof nc->name, race, ethos, newc);
                        for (int i = 0; i < nr; i++) if (comp[i] == k) {
                            econ->region[regs[i]].owner = (int16_t)newc;
                            region_set_country(w, regs[i], newc);
                        }
                        born++;
                    } else {                               /* table pleine : perdu */
                        for (int i = 0; i < nr; i++) if (comp[i] == k) {
                            econ->region[regs[i]].owner = -1; region_set_country(w, regs[i], -1);
                        }
                    }
                } else {                                   /* trop petit : perdu */
                    for (int i = 0; i < nr; i++) if (comp[i] == k) {
                        econ->region[regs[i]].owner = -1; region_set_country(w, regs[i], -1);
                    }
                }
            }
        }
    }

    /* region_ids du pays = ses régions éco RESTANTES (post-split). */
    ct->n_regions = 0;
    for (int r = 0; r < econ->n_regions && ct->n_regions < 32; r++)
        if (econ->region[r].owner == country) ct->region_ids[ct->n_regions++] = (int16_t)r;
    /* capitale : si la province-capitale n'est plus dans une région du pays, recale. */
    int cap_ok = 0;
    if (ct->capital_prov >= 0 && ct->capital_prov < w->n_provinces) {
        int crg = w->province[ct->capital_prov].region;
        if (crg >= 0 && crg < econ->n_regions && econ->region[crg].owner == country) cap_ok = 1;
    }
    if (!cap_ok) {
        ct->capital_prov = -1;
        if (ct->n_regions > 0) {
            int r0 = ct->region_ids[0];
            if (r0 >= 0 && r0 < w->n_regions && w->region[r0].n_provinces > 0)
                ct->capital_prov = w->region[r0].province_ids[0];
        }
    }
    return born;
}

/* AMORÇAGE (au fire) : trace K bras radiaux depuis l'épicentre, programme les
 * régions traversées (sunken[r]=1). Le résultat (le bitmap) EST la donnée persistée. */
static void cataclysm_water_seed(EndgameState *eg, const World *w, const WorldEconomy *econ) {
    int cx, cy; region_centroid(w, eg->epicenter_reg, &cx, &cy);
    uint32_t rng = (uint32_t)((eg->epicenter_reg + 1) * 2654435761u)
                 ^ (uint32_t)((eg->fauteur_country + 1) * 40503u) ^ 0x5EA00Du;
    memset(eg->sunken, 0, sizeof eg->sunken);
    if (eg->epicenter_reg >= 0 && eg->epicenter_reg < econ->n_regions)
        eg->sunken[eg->epicenter_reg] = 1;
    for (int a = 0; a < RIFT_ARMS; a++) {
        float ang = 6.2831853f * ((float)a / RIFT_ARMS) + (float)(rng % 628) / 100.f;
        rng = rng * 1664525u + 1013904223u;
        float dx = cosf(ang), dy = sinf(ang);
        for (int s = RIFT_ARM_STEP; s <= RIFT_ARM_LEN; s += RIFT_ARM_STEP) {
            int x = cx + (int)(dx * s), y = cy + (int)(dy * s);
            if (x < 0 || y < 0 || x >= SCPS_W || y >= SCPS_H) break;
            int r = w->cell[scps_idx(x,y)].region;
            if (r >= 0 && r < econ->n_regions && econ->region[r].owner >= 0)
                eg->sunken[r] = 1;
        }
    }
    int pend = 0; for (int r = 0; r < econ->n_regions; r++) if (eg->sunken[r] == 1) pend++;
    eg->sink_pending = pend; eg->n_sunken = 0;
}

/* DÉROULEMENT (par an) : engloutit jusqu'à SINK_RIFTS_PER_YEAR régions programmées,
 * recalcule l'adjacence, refragmente les empires scindés. */
static void cataclysm_water_step(EndgameState *eg, World *w, WorldEconomy *econ, Campaign *camp) {
    int budget = (int)(tune_f("SINK_RIFTS_PER_YEAR", 3.f) + 0.5f);
    if (budget < 1) budget = 1;
    int sunk_now = 0;
    for (int r = 0; r < econ->n_regions && sunk_now < budget; r++) {
        if (eg->sunken[r] != 1) continue;       /* 1 = programmé, pas encore sombré */
        cataclysm_sink_region(w, econ, camp, r);
        eg->sunken[r] = 2;                       /* 2 = englouti */
        eg->n_sunken++; if (eg->sink_pending > 0) eg->sink_pending--;
        sunk_now++;
    }
    if (sunk_now == 0) return;                   /* rien englouti ce tick : carte stable */
    world_recompute_adjacency(w);                /* côtes/frontières neuves (world-side) */
    econ_build_adjacency(econ, w);               /* adjacence éco : barrières mer neuves */
    int nc0 = w->n_countries, born = 0;          /* fige le compte (les successeurs ne se resplittent pas) */
    for (int c = 0; c < nc0; c++) born += cataclysm_resplit_empire(w, econ, c);
    if (born > 0) world_recompute_adjacency(w);  /* redessine les frontières de pays */
}

/* ── C2 — sélecteur + déclencheur (latch : un seul déclenchement) ──────────── */
static void endgame_select_and_fire(EndgameState *eg, const World *w,
                                     WorldEconomy *econ, const WorldProsperity *wp,
                                     const TechState ts[], int year) {
    if (eg->fired) return;
    if (wp->entropy < tune_f("ENTROPY_FIN", 50.f)) return;

    /* Override MERVEILLE (priorité) : l'Ascension a été menée à bout → c'est ELLE. */
    if (eg->merv == MERV_ASCENDED) {
        eg->fired = true; eg->fin = FIN_ASCENSION; eg->fin_year = year;
        return;
    }

    /* Foyer FIGÉ : l'épicentre régional (région la plus saturée), sinon — entropie
     * tech-driven pure — l'empire le plus faustien et sa capitale. */
    int epi = wp->entropy_epicenter;
    int fauteur = (epi >= 0 && epi < econ->n_regions) ? econ->region[epi].owner : -1;
    if (epi < 0 || fauteur < 0) {
        int fr = -1; int f = endgame_pick_fauteur(w, ts, &fr);
        if (epi < 0)     epi = fr;
        if (fauteur < 0) fauteur = f;
    }
    eg->epicenter_reg   = epi;
    eg->fauteur_country = fauteur;
    eg->fin_year        = year;

    /* Sélecteur par compteur DOMINANT de conso de rare (les transmuteurs) :
     * 0 (essence) → EAU · 1 (flux) → RONCES · 2 (fer céleste) → FROID. */
    int k = 0; double mx = wp->faust_consumed[0];
    for (int i = 1; i < 3; i++) if (wp->faust_consumed[i] > mx) { mx = wp->faust_consumed[i]; k = i; }
    if (mx < 1.0) {
        /* Aucun transmuteur : le SAVOIR faustien seul a mené à la Brèche → la FORME
         * de l'apocalypse suit la signature (déterministe) de l'empire fauteur, pour
         * ne pas figer le monde sur une seule fin. */
        uint32_t h = (uint32_t)((eg->fauteur_country + 1) * 2654435761u)
                   ^ (uint32_t)((eg->epicenter_reg + 1) * 40503u);
        k = (int)(h % 3u);
    }
    static const FinType MAP[3] = { FIN_EAU, FIN_RONCES, FIN_FROID };
    eg->fin   = MAP[k];
    eg->fired = true;
    /* Amorçage de l'eau : trace le rift (le front de ronces C5 viendra ici aussi). */
    if (eg->fin == FIN_EAU && epi >= 0) cataclysm_water_seed(eg, w, econ);
}

void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  Campaign *camp, int player, int year) {
    (void)rn; (void)navy; (void)dp; (void)player;
    if (!eg || !wp) return;

    /* C1 — élargir l'entropie (le savoir faustien). */
    endgame_entropy_widen(wp, ts, w ? w->n_countries : 0);

    /* Diag de CALIBRATION (SCPS_ENTDIAG=1) : la courbe d'entropie année par année.
     * OFF par défaut, stderr → déterminisme intact. */
    if (getenv("SCPS_ENTDIAG")) {
        static const char *FN[] = { "—", "EAU", "FROID", "RONCES", "ASCENSION" };
        fprintf(stderr, "[ENTDIAG] an %d : entropie %.1f / fin %.0f%s%s%s\n",
                year, (double)wp->entropy, (double)tune_f("ENTROPY_FIN", 50.f),
                eg->fired ? " [" : "", eg->fired ? FN[eg->fin] : "",
                eg->fired ? "]" : "");
    }

    /* C2 — pas encore déclenché : tester le seuil combiné et latcher (+ amorcer). */
    if (!eg->fired) {
        if (w && econ && ts) endgame_select_and_fire(eg, w, econ, wp, ts, year);
    }

    /* C3-C6 — dérouler la fin latchée (la carve voit la même année que le fire). */
    if (eg->fired && w && econ) {
        switch (eg->fin) {
            case FIN_EAU:    cataclysm_water_step(eg, w, econ, camp); break;
            case FIN_FROID:  /* C4 */ break;
            case FIN_RONCES: /* C5 */ break;
            default: break;
        }
    }
}
