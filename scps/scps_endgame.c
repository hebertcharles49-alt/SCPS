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
#include "scps_factions.h"   /* V1a — réactions des factions au tir (lot C) */
#include "scps_math.h"       /* clampf partagé (endgame_heritage_metabolized) */
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

/* ── C1 — élargir la source d'entropie (V1a : UNE BARRE, QUATRE NOURRITURES) ──
 * prosperity_tick a posé wp->entropy = Σ faust_charge régional (la transgression
 * ACTIVE des transmuteurs ; plate à 0 en monde stable). On AJOUTE :
 *   (1) la « pression du savoir » : la charge de TECH faustienne accumulée par
 *       empire (celle qui ramène déjà l'Âge de la Brèche) — EXISTANT ;
 *   (2) la pression de l'Âge de la Brèche (wp->age_breach_flux, posée par
 *       scps_events.c — un flux faustien mondial DÉJÀ une entrée moteur, jamais
 *       lu par l'endgame jusqu'ici) — décision #1, l'unification ;
 *   (3) LES MORTS DE GUERRE (neuf) : Campaign.dead_choc + dead_pursuit, cumul
 *       SIM (RAZ par campaign_init), rapporté à pop_ref (échelle cohérente —
 *       un ratio, pas un compte brut). war_dead est mis à jour ICI (une seule
 *       lecture par tick, sérialisé dans eg pour que le ratio survive au reload
 *       et que la sélection au fire relise le même chiffre qu'au tick).
 * Recalculé à neuf chaque tick (prosperity_tick réassigne wp->entropy = esum)
 * → l'ajout est NON cumulatif inter-ticks. Ce sont de vraies ENTRÉES moteur
 * (charge/flux/morts), jamais un bonus plat. */
/* Σ pop VIVANTE monde (toutes strates, régions possédées) — le dénominateur du
 * ratio de sang. CALIBRAGE (2e passe, mesuré 5 graines × 250 ans) : rapporté à
 * pop_ref (l'an-0), le ratio explosait (la pop TRIPLE en 250 ans) et écrasait
 * toute discrimination ; rapporté à la pop ACTUELLE, les graines s'étalent
 * (0.05 → 1.5) et le seuil 0.20 sépare NATURELLEMENT les mondes sanglants
 * (Sang) des mondes calmes (les autres visages). */
static double endgame_world_pop(const WorldEconomy *econ) {
    if (!econ) return 0.0;
    double tot = 0.0;
    for (int r = 0; r < econ->n_regions && r < SCPS_MAX_REG; r++) {
        const RegionEconomy *re = &econ->region[r];
        if (re->owner < 0) continue;
        for (int cl = 0; cl < CLASS_COUNT; cl++) tot += (double)re->strata[cl].pop;
    }
    return tot;
}

/* Le ratio de sang CANONIQUE (mémoire décrue / pop actuelle, repli pop_ref) —
 * LU par l'entrée d'entropie ET par la sélection au fire (le même chiffre). */
double endgame_blood_ratio(const EndgameState *eg, const WorldEconomy *econ) {
    if (!eg) return 0.0;
    double base = endgame_world_pop(econ);
    if (base <= 0.0) base = eg->pop_ref;
    return (base > 0.0) ? eg->war_dead / base : 0.0;
}

static void endgame_entropy_widen(EndgameState *eg, WorldProsperity *wp,
                                  const TechState ts[], const Campaign *camp,
                                  const WorldEconomy *econ, int nc) {
    if (!wp) return;
    if (ts) {
        float tech_ent = 0.f;
        for (int c = 0; c < nc && c < SCPS_MAX_COUNTRY; c++) tech_ent += ts[c].charge;
        wp->entropy += tune_f("ENTROPY_TECH_W", 1.0f) * tech_ent;
    }
    wp->entropy += tune_f("ENTROPY_BREACH_W", 0.3f) * wp->age_breach_flux;
    if (eg && camp) {
        /* MÉMOIRE À DÉCRUE (calibrage post-sweep : le miroir cumulatif atteignait
         * 40-961 % de pop_ref — toute partie longue devenait SANG). On accumule le
         * DELTA de morts depuis la dernière lecture, puis la mémoire DÉCROÎT d'une
         * demi-vie SANG_MEMORY_HL ans par appel (endgame_tick est ANNUEL) : le seuil
         * ENDGAME_BLOOD_FRAC mesure « une génération qui a perdu un cinquième du
         * monde », pas l'addition de tous les siècles. */
        double cum = (double)camp->dead_choc + (double)camp->dead_pursuit;
        double delta = cum - eg->war_dead_seen;
        if (delta < 0.0) delta = 0.0;             /* RAZ de campagne (nouvelle sim) */
        eg->war_dead_seen = cum;
        eg->war_dead = eg->war_dead * pow(0.5, 1.0 / (double)tune_f("SANG_MEMORY_HL", 40.f))
                     + delta;
        wp->entropy += tune_f("ENTROPY_BLOOD_W", 8.0f)
                     * (float)endgame_blood_ratio(eg, econ);
    }
}

/* Posé UNE fois (sim_init, après gen_population) : la population de référence
 * an-0 — désormais le REPLI du dénominateur (bancs sans econ vivant) ; le ratio
 * courant se calcule sur la pop ACTUELLE (endgame_blood_ratio). No-op si déjà
 * posé (>0) : un reload ne le ré-amorce jamais. */
void endgame_set_pop_ref(EndgameState *eg, const WorldEconomy *econ) {
    if (!eg || !econ || eg->pop_ref > 0.0) return;
    eg->pop_ref = endgame_world_pop(econ);
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
/* STRIP ÉCO d'une région MORTE (eau OU ronces) : dépeuplée, sans maître,
 * infranchissable ; armées échouées ; détachée du pays World. La région GARDE son
 * indice (rien n'est réordonné → save_sane satisfait). PARTAGÉ C3 (eau) / C5 (ronces). */
static void cataclysm_strip_region_econ(World *w, WorldEconomy *econ, Campaign *camp, int r) {
    /* RE-KEY PROVINCE : la région ENGLOUTIE perd TOUTES ses provinces (le sol lui-même
     * disparaît sous les flots) — un strip sur econ->region[r] seul serait à la fois
     * ÉCRASÉ au prochain econ_tick (les champs Σ/miroir province-owned) ET incomplet
     * (les provinces-sœurs de la région resteraient vivantes, actives). On efface
     * CHAQUE province membre ; econ_region_set_owner pose owner=-1 partout. */
    if (r >= 0 && r < w->n_regions) {
        const Region *rg = &w->region[r];
        for (int k = 0; k < rg->n_provinces; k++) {
            int pid = rg->province_ids[k];
            if (pid < 0 || pid >= econ->n_prov) continue;
            ProvinceEconomy *pe = &econ->prov[pid];
            for (int cl = 0; cl < CLASS_COUNT; cl++) pe->strata[cl].pop = 0.f;
            pe->pop.n_groups = 0;
            pe->owner = -1; pe->active = false; pe->colonized = false; pe->impassable = true;
            pe->coastal = false; pe->n_bld = 0; pe->cap_pop = 0.f; pe->diaspora_pop = 0.f;
            for (int g = 0; g < RES_COUNT; g++) { pe->stock[g]=0.f; pe->supply[g]=0.f; pe->demand[g]=0.f; pe->raw_cap[g]=0.f; }
        }
    }
    if (r < w->n_regions) w->region[r].country = -1;   /* province.country reste ≥0 (save_sane) */
    if (camp) for (int c = 0; c < SCPS_MAX_COUNTRY; c++) {
        FieldArmy *a = &camp->army[c];
        if (a->active && (a->loc == r || a->dest == r || a->next == r)) {
            a->active = false; a->dest = -1; a->next = -1;
        }
    }
}

static void cataclysm_sink_region(World *w, WorldEconomy *econ, Campaign *camp, int r) {
    if (r < 0 || r >= econ->n_regions) return;
    /* cellules : terre → mer, hiérarchie cell strippée. */
    for (int i = 0; i < SCPS_N; i++) {
        Cell *c = &w->cell[i];
        if (c->region != r) continue;
        world_sink_cell(c, SEA_LEVEL - 0.03f);
    }
    cataclysm_strip_region_econ(w, econ, camp, r);     /* l'éco morte (partagé eau/ronces) */
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
                        Heritage heritage = econ->region[firstreg].culture.heritage;
                        Ethos ethos = econ->region[firstreg].culture.ethos;
                        Country *nc = &w->country[newc]; memset(nc, 0, sizeof *nc);
                        nc->role = POLITY_ANTAGONIST;
                        nc->continent = (firstreg < w->n_regions) ? w->region[firstreg].continent : 0;
                        nc->color = country_heritage_color(heritage, newc);
                        country_make_name(nc->name, sizeof nc->name, heritage, ethos, newc);
                        /* RE-KEY PROVINCE : owner sur TOUTES les provinces de la région
                         * (econ_region_set_owner) — region[r].owner est un DÉRIVÉ. */
                        for (int i = 0; i < nr; i++) if (comp[i] == k) {
                            econ_region_set_owner(econ, w, regs[i], newc);
                            region_set_country(w, regs[i], newc);
                        }
                        born++;
                    } else {                               /* table pleine : perdu */
                        for (int i = 0; i < nr; i++) if (comp[i] == k) {
                            econ_region_set_owner(econ, w, regs[i], -1); region_set_country(w, regs[i], -1);
                        }
                    }
                } else {                                   /* trop petit : perdu */
                    for (int i = 0; i < nr; i++) if (comp[i] == k) {
                        econ_region_set_owner(econ, w, regs[i], -1); region_set_country(w, regs[i], -1);
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

/* ─────────────────────────────────────────────────────────────────────────────
 * C4 — APOCALYPSE DE FROID (fer céleste) : effet GRADUEL, non géométrique
 * ──────────────────────────────────────────────────────────────────────────── */
/* Pas de modificateur plat : on MUTE la température des cellules par le DELTA annuel
 * (cell.temperature persistée dans WRLD → appliquer le delta, pas l'offset total, évite
 * la double application au reload), on rebiome (forêt→steppe→glacier via assign_biome),
 * puis econ_cold_refresh fait ÉMERGER la famine (grain f(habitabilité refroidie)). */
static void cold_step(EndgameState *eg, World *w, WorldEconomy *econ) {
    float ramp = tune_f("COLD_RAMP_PER_YEAR", 0.005f);
    float prev = eg->cold_offset;
    eg->cold_offset += ramp;
    if (eg->cold_offset > 1.0f) eg->cold_offset = 1.0f;   /* plafond : monde figé sous la glace */
    float delta = eg->cold_offset - prev;                 /* delta RÉELLEMENT appliqué (borné) */
    if (delta <= 0.f) return;
    for (int i = 0; i < SCPS_N; i++) {
        Cell *c = &w->cell[i];
        if (c->height < SEA_LEVEL) continue;              /* la mer reste mer */
        c->temperature -= delta;
        if (c->temperature < 0.f) c->temperature = 0.f;
        world_rebiome_cell(c);                            /* les biomes blanchissent */
    }
    econ_cold_refresh(econ, w);                           /* la famine émerge de la chaîne */
}

/* ─────────────────────────────────────────────────────────────────────────────
 * C5 — APOCALYPSE DES RONCES (flux) : BFS-CELLULES erratique
 * ──────────────────────────────────────────────────────────────────────────── */
#define THORN_FLIP_FRAC 0.5f    /* ≥ 50 % des cellules de terre en ronces ⇒ la région tombe */

/* ÉRUPTION (1er pas) : la bramble jaillit du foyer — corrompt les cellules de la
 * région-épicentre et les pose comme front initial. Mute w (≠ water_seed, read-only). */
static void cataclysm_thorns_seed(EndgameState *eg, World *w, const WorldEconomy *econ) {
    eg->thorn_rng = (uint32_t)((eg->epicenter_reg + 1) * 2654435761u)
                  ^ (uint32_t)((eg->fauteur_country + 1) * 40503u) ^ 0x7140C5u;
    eg->thorn_front_n = 0;
    int epi = eg->epicenter_reg;
    if (epi < 0 || epi >= econ->n_regions) return;
    for (int i = 0; i < SCPS_N; i++) {
        Cell *c = &w->cell[i];
        if (c->region == epi && c->height >= SEA_LEVEL) {
            c->biome = BIO_THORNS;
            if (eg->thorn_front_n < SCPS_THORN_FRONT_MAX) eg->thorn_front[eg->thorn_front_n++] = i;
        }
    }
}

/* DÉROULEMENT (par an) : le front de ronces propage de façon ERRATIQUE (fraction
 * aléatoire des voisins de terre, rng dédié → déterministe & frontière non circulaire).
 * Une région majoritairement ronces TOMBE (strip partagé C3) → recalcul + refragmentation. */
static void thorns_step(EndgameState *eg, World *w, WorldEconomy *econ, Campaign *camp) {
    if (eg->thorn_front_n == 0) cataclysm_thorns_seed(eg, w, econ);   /* éruption au 1er pas */
    int budget = (int)(tune_f("THORN_CELLS_PER_YEAR", 200.f) + 0.5f); if (budget < 1) budget = 1;
    float frac = tune_f("THORN_RANDOM_FRAC", 0.35f);
    static const int NDX[8] = {1,-1,0,0,1,1,-1,-1}, NDY[8] = {0,0,1,-1,1,-1,1,-1};
    static int next[SCPS_THORN_FRONT_MAX]; int nn = 0;
    int corrupted = 0;
    for (int fi = 0; fi < eg->thorn_front_n; fi++) {
        int ci = eg->thorn_front[fi];
        if (corrupted >= budget) { if (nn < SCPS_THORN_FRONT_MAX) next[nn++] = ci; continue; }  /* reporte le reste */
        int x = ci % SCPS_W, y = ci / SCPS_W;
        bool still = false;
        for (int d = 0; d < 8; d++) {
            int nx = x + NDX[d], ny = y + NDY[d];
            if (nx < 0 || ny < 0 || nx >= SCPS_W || ny >= SCPS_H) continue;
            int nidx = scps_idx(nx, ny); Cell *nc = &w->cell[nidx];
            if (nc->height < SEA_LEVEL || nc->biome == BIO_THORNS) continue;   /* mer / déjà ronces */
            eg->thorn_rng = eg->thorn_rng * 1664525u + 1013904223u;
            float roll = (float)((eg->thorn_rng >> 8) & 0xFFFFFFu) / (float)0x1000000;
            if (roll < frac) { nc->biome = BIO_THORNS; corrupted++; if (nn < SCPS_THORN_FRONT_MAX) next[nn++] = nidx; }
            else still = true;                                                 /* un voisin sain reste → tip actif */
        }
        if (still && nn < SCPS_THORN_FRONT_MAX) next[nn++] = ci;
    }
    memcpy(eg->thorn_front, next, sizeof(int) * (size_t)nn);
    eg->thorn_front_n = nn;
    if (corrupted == 0) return;                                                /* rien corrompu : carte stable */

    /* régions majoritairement ronces → TOMBENT (convertit + détache + strip éco partagé). */
    static int land[SCPS_MAX_REG], thn[SCPS_MAX_REG];
    memset(land, 0, sizeof land); memset(thn, 0, sizeof thn);
    for (int i = 0; i < SCPS_N; i++) {
        Cell *c = &w->cell[i];
        if (c->height < SEA_LEVEL) continue;
        int r = c->region; if (r < 0 || r >= SCPS_MAX_REG) continue;
        land[r]++; if (c->biome == BIO_THORNS) thn[r]++;
    }
    int flipped = 0;
    for (int r = 0; r < econ->n_regions && r < SCPS_MAX_REG; r++) {
        if (land[r] == 0 || econ->region[r].owner < 0) continue;
        if ((float)thn[r] >= THORN_FLIP_FRAC * (float)land[r]) {
            for (int i = 0; i < SCPS_N; i++) { Cell *c = &w->cell[i];
                if (c->region == r) { c->biome = BIO_THORNS; c->province = c->region = c->country = c->continent = -1; c->coast = false; } }
            cataclysm_strip_region_econ(w, econ, camp, r);
            flipped++;
        }
    }
    if (flipped == 0) return;
    world_recompute_adjacency(w); econ_build_adjacency(econ, w);
    int nc0 = w->n_countries, born = 0;
    for (int c = 0; c < nc0; c++) born += cataclysm_resplit_empire(w, econ, c);
    if (born > 0) world_recompute_adjacency(w);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * C4bis — APOCALYPSE DE SANG (guerre) : dépeuplement progressif, cicatrice
 * PERMANENTE (ne guérit plus — contrairement à revolt_scar qui décroît).
 * ──────────────────────────────────────────────────────────────────────────── */
#define SANG_SCAR_MIN 0.15f   /* une région n'entre dans la marque qu'au-delà (guerre RÉELLE, pas du bruit) */

/* AMORÇAGE (au fire) : la marque SANG naît d'un instantané de revolt_scar (le
 * signal « région ravagée » déjà porté par l'éco — sac de guerre/révolte) sur
 * TOUTES les régions vivantes qui le dépassent. Une fois posée, sang_scar ne
 * décroît JAMAIS (contrairement à revolt_scar) — la guerre laisse une plaie
 * qui ne se referme plus, c'est la thèse de cette fin. */
static void sang_seed(EndgameState *eg, const WorldEconomy *econ) {
    memset(eg->sang_scar, 0, sizeof eg->sang_scar);
    for (int r = 0; r < econ->n_regions && r < SCPS_MAX_REG; r++) {
        const RegionEconomy *re = &econ->region[r];
        if (re->owner < 0) continue;
        float s = re->revolt_scar;
        if (s > SANG_SCAR_MIN) eg->sang_scar[r] = (s > 1.f) ? 1.f : s;
    }
}

/* DÉROULEMENT (par an) : draine une fraction de pop ∝ la marque, dans CHAQUE
 * région marquée, PLAFONNÉE (un plancher de pop empêche la spirale vers zéro —
 * "un monde fini", pas un memset brutal). Réutilise econ_region_pop_add (le
 * même idiome que le reste de l'éco — jamais un accès direct à prov[]). */
#define SANG_DRAIN_PER_YEAR 0.03f  /* fraction de pop drainée/an dans une région à marque=1 */
#define SANG_POP_FLOOR      50.f   /* plancher : une région marquée ne descend jamais sous ça */
static void sang_step(EndgameState *eg, WorldEconomy *econ) {
    float rate = tune_f("SANG_DRAIN_PER_YEAR", SANG_DRAIN_PER_YEAR);
    for (int r = 0; r < econ->n_regions && r < SCPS_MAX_REG; r++) {
        float scar = eg->sang_scar[r];
        if (scar <= 0.f) continue;
        RegionEconomy *re = &econ->region[r];
        if (re->owner < 0) continue;
        float tot = 0.f; for (int cl = 0; cl < CLASS_COUNT; cl++) tot += re->strata[cl].pop;
        if (tot <= SANG_POP_FLOOR) continue;                 /* plancher atteint : la région respire */
        float drain = tot * rate * scar;
        if (tot - drain < SANG_POP_FLOOR) drain = tot - SANG_POP_FLOOR;
        if (drain <= 0.f) continue;
        for (int cl = 0; cl < CLASS_COUNT; cl++) {
            float share = (tot > 0.f) ? re->strata[cl].pop / tot : 0.f;
            float take = drain * share;
            if (take > 0.f) econ_region_pop_add(econ, r, cl, -take);
        }
    }
}

/* ── LOT C — RÉACTIONS DES FACTIONS AU TIR (one-shot, au moment où la fin latche) ──
 * UN SEUL site par fin (jamais par tick) : faction_lever_apply est le SEUL point
 * d'entrée public des factions — la grief se propage par la table d'OPPOSITION
 * (scps_factions.c : faction_opposition) déjà existante, jamais un setter direct.
 * Mapping (design, forces modestes 0.10-0.20) :
 *   TOUTE fin (apocalypse ou Merveille) → Transgresseur AVANCE (« ils l'ont voulue » —
 *     la puissance au-delà des limites, qu'elle brûle le monde ou l'élève) ; Gardien
 *     s'aigrit EN RETOUR (opposition G↔T = 1.0, la table la porte automatiquement).
 *   FIN_SANG    → EN PLUS, Conquérant AVANCE (la guerre) ; Communautaire s'aigrit
 *     (opposition C↔U = 1.0, « le peuple paie le prix »).
 *   FIN_RONCES  → Communautaire s'aigrit déjà via l'opposition T↔U = 0.9 (l'avancée
 *     Transgresseur universelle suffit ; pas de lever supplémentaire nécessaire).
 *   FIN_EAU     → EN PLUS, Gardien AVANCE (le déluge comme jugement) ; Marchand
 *     s'aigrit (opposition M↔G = 0.9, « les routes noyées »).
 * `faction_levers_reset` (sim_init) tient le déterminisme : hors ce site, aucun
 * autre code de l'endgame ne touche les factions. */
#define FAC_REACT_UNIVERSAL 0.15f   /* Transgresseur, toute fin */
#define FAC_REACT_SPECIFIC  0.15f   /* le lever additionnel propre à la fin */
static void endgame_faction_react(FinType fin, int fauteur) {
    if (fauteur < 0 || fauteur >= SCPS_MAX_COUNTRY) return;
    faction_lever_apply(fauteur, FAC_TRANSGRESSEUR, FAC_REACT_UNIVERSAL);   /* toute fin : ils l'ont voulue */
    switch (fin) {
        case FIN_SANG: faction_lever_apply(fauteur, FAC_CONQUERANT, FAC_REACT_SPECIFIC); break;   /* → grief Communautaire */
        case FIN_EAU:  faction_lever_apply(fauteur, FAC_GARDIEN,    FAC_REACT_SPECIFIC); break;   /* → grief Marchand */
        default: break;   /* RONCES/FROID/ASCENSION : le lever universel suffit (T↔U=0.9) */
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * C6 — LA MERVEILLE (Ascension : la SEULE victoire, réservée au JOUEUR)
 * ──────────────────────────────────────────────────────────────────────────── */
#define MERV_RARE_PER_YEAR 2.0f    /* rare dévorée/an par le chantier (TRÈS rare) */
/* FORGE ← fer céleste · SOCIÉTÉ ← flux · SAVOIR ← essence */
static const Resource MERV_RARE[3] = { RES_CELESTIAL_IRON, RES_FLUX, RES_ESSENCE };

/* Consomme jusqu'à `amount` de `good` dans le POOL de l'empire (P1) ; renvoie le pris. */
static float endgame_empire_consume(WorldEconomy *econ, int owner, Resource good, float amount) {
    float got = 0.f;
    for (int r = 0; r < econ->n_regions && got < amount; r++) {
        if (econ->region[r].owner != owner) continue;
        float take = fminf(amount - got, econ->region[r].stock[good]);
        if (take > 0.f) { econ->region[r].stock[good] -= take; got += take; }
    }
    return got;
}

/* ── CORRECTIF (relu par le joueur) — DÉNOMINATEUR PAR-HÉRITAGE, pas la pop totale ──
 * PIÈGE ÉVITÉ : `econ_country_heritage_digested` (Temps 2 tech) normalise CHAQUE
 * dig[h] sur la POP TOTALE de l'empire (scps_econ.c:568, `out[r]=dig[r]/tot`) — un
 * choix voulu pour l'accès TECH (« X% de l'empire est digéré de cet héritage »),
 * mais qui rend « 6 héritages ≥ METAB_TIER3=0.35 simultanément » MATHÉMATIQUEMENT
 * IMPOSSIBLE (6×0.35 = 210 % de la pop totale — les héritages PARTITIONNENT le
 * même total, au plus 2 peuvent franchir 0.35 en même temps). La Merveille a
 * besoin d'une question DIFFÉRENTE : « CE peuple-là, LUI, est-il digéré ? » — le
 * dénominateur est la taille de SA PROPRE diaspora, pas celle de tout l'empire.
 * `endgame_heritage_metabolized` réplique le motif de scan de
 * econ_country_heritage_digested (prov[] entier, charte règle 1) mais somme
 * dig_X/tot_X SUR LES SEULS groupes de l'héritage X — chaque culture jugée sur
 * SON PROPRE poids, pas celui des 5 autres. metab_diffuse_coeff est `static` dans
 * scps_econ.c (non exporté) : répliqué ici À L'IDENTIQUE (même switch, même
 * tunable METAB_DIFFUSE_SLAVE) plutôt que dupliqué sous un autre nom. */
static float endgame_diffuse_coeff(uint8_t arrival) {
    switch (arrival) {
        case ARR_MIGRANT: return 1.f;
        case ARR_SOUMIS:  return 1.f;
        case ARR_REFUGIE: return 1.f;
        case ARR_DEPORTE: return tune_f("METAB_DIFFUSE_SLAVE", 0.3f);
        default:          return 0.f;   /* ARR_NATIF */
    }
}

/* Un héritage h (≠ natif) est-il « métabolisé » pour la Merveille ? Σ sur les
 * SEULS groupes diaspora d'héritage h : dig_h = Σ count×integration×coeff(arrival),
 * tot_h = Σ count (même héritage, TOUT groupe — diaspora ou non, au dénominateur :
 * une diaspora encore mal intégrée doit peser aussi, sinon dig_h/tot_h flambe à 1.0
 * dès qu'UN groupe bien intégré existe alors que 10 autres du même héritage restent
 * à 0). Deux gardes : un RATIO (bien digéré RELATIVEMENT à sa propre communauté) ET
 * un PLANCHER D'ÂMES (pas de « culture métabolisée » à 30 personnes noyées dans un
 * gros pays — dig_h doit peser un minimum en absolu). */
static bool endgame_heritage_metabolized(const World *w, const WorldEconomy *econ, int cid, int h) {
    if (!w || !econ || cid < 0 || cid >= w->n_countries) return false;
    double dig = 0.0, tot = 0.0;
    int nprov = econ->n_prov; if (nprov > SCPS_MAX_PROV) nprov = SCPS_MAX_PROV;
    for (int p = 0; p < nprov; p++) {
        const ProvinceEconomy *pe = &econ->prov[p];
        if (pe->owner != cid) continue;
        const ProvincePop *pp = &pe->pop;
        for (int i = 0; i < pp->n_groups; i++) {
            const PopGroup *g = &pp->groups[i];
            if ((int)g->heritage != h) continue;
            tot += (double)g->count;
            float df = endgame_diffuse_coeff(g->arrival);
            if (df > 0.f) dig += (double)g->count * (double)clampf(g->integration, 0.f, 1.f) * (double)df;
        }
    }
    if (tot <= 0.0) return false;
    float ratio = (float)(dig / tot);
    return ratio >= tune_f("METAB_MERV_RATIO", 0.60f) && dig >= (double)tune_f("METAB_MERV_MIN", 500.f);
}

/* MÉTABOLISATION (décision #2) — nb d'héritages « métabolisés » par cid : le
 * NATIF (l'héritage de la capitale) compte TOUJOURS + tout héritage ÉTRANGER
 * INDIVIDUALISÉ (endgame_heritage_metabolized, ci-dessus) OU à profondeur de
 * CONTACT PROFONDE (ts[cid].arch_depth[], cache DÉJÀ posé par ai_sync_refresh —
 * commerce/gouvernance, orthogonal à la population : n'est PAS sujet au piège du
 * dénominateur partagé). Sans ts (bancs C0-C2 qui passent NULL) : seule la
 * métabolisation individualisée compte (repli, arch_depth absent). */
static int endgame_metab_count_ts(const World *w, const WorldEconomy *econ,
                                  const TechState ts[], int cid) {
    if (!w || !econ || cid < 0 || cid >= w->n_countries) return 0;
    int cp = w->country[cid].capital_prov;
    int cr = (cp >= 0 && cp < w->n_provinces) ? w->province[cp].region : -1;
    Heritage native = (cr >= 0 && cr < econ->n_regions) ? econ->region[cr].culture.heritage
                                                        : HERITAGE_ADAPTATIF;
    const unsigned char *depth = (ts && cid < SCPS_MAX_COUNTRY) ? ts[cid].arch_depth : NULL;
    int n = 0;
    for (int h = 0; h < HERITAGE_COUNT; h++) {
        if (h == (int)native) { n++; continue; }        /* natif : toujours compté (accès plein) */
        bool contact_deep = (depth && h < ARCH_COUNT && depth[h] >= (unsigned char)PROF_PROFOND);
        if (contact_deep || endgame_heritage_metabolized(w, econ, cid, h)) n++;
    }
    return n;
}

/* API publique (header) : sans TechState — repli metab-seule (arch_depth absent).
 * Le tick interne (wonder_tick) utilise endgame_metab_count_ts (avec ts) pour la
 * barre COMPLÈTE (contact OU métabolisation individualisée). */
int endgame_metab_count(const World *w, const WorldEconomy *econ, int cid) {
    return endgame_metab_count_ts(w, econ, NULL, cid);
}

/* Requis de métabolisation du palier COURANT (décision #2 : FORGE≥3, SOCIÉTÉ≥4,
 * SAVOIR≥6). 0 si aucun palier actif (MERV_NONE/ASCENDED) — rien à gater. */
int endgame_metab_required(MervPhase merv) {
    switch (merv) {
        case MERV_FORGE: case MERV_FORGE_DONE:     return 3;
        case MERV_SOCIETE: case MERV_SOCIETE_DONE: return 4;
        case MERV_SAVOIR: case MERV_SAVOIR_DONE:   return 6;
        default: return 0;
    }
}

/* L'empire ASCENDANT disparaît à la Dwemer : régions vidées (owner=-1, pop=0), terre
 * INTACTE (biome/height inchangés, passable, recolonisable — PAS de carve). */
static void endgame_empire_vanish(World *w, WorldEconomy *econ, int player) {
    for (int r = 0; r < econ->n_regions; r++) {
        if (econ->region[r].owner != player) continue;
        /* RE-KEY PROVINCE : strata/pop/owner/colonized sont PROVINCE-OWNED — la région
         * ENTIÈRE se vide (l'empire disparaît) donc CHAQUE province membre est vidée
         * (region[r].* est un DÉRIVÉ, écrasé au prochain econ_tick sinon). */
        if (r < w->n_regions) {
            const Region *rg = &w->region[r];
            for (int k = 0; k < rg->n_provinces; k++) {
                int pid = rg->province_ids[k];
                if (pid < 0 || pid >= econ->n_prov) continue;
                ProvinceEconomy *pe = &econ->prov[pid];
                for (int cl = 0; cl < CLASS_COUNT; cl++) pe->strata[cl].pop = 0.f;
                pe->pop.n_groups = 0; pe->owner = -1; pe->colonized = false;   /* active/passable INTACT */
            }
        }
        if (r < w->n_regions) w->region[r].country = -1;
    }
    for (int i = 0; i < SCPS_N; i++) if (w->cell[i].country == player) w->cell[i].country = -1;
    world_recompute_adjacency(w);                                             /* les frontières s'effacent */
}

/* Tick de la Merveille : ordre IMPOSÉ FORGE→SOCIÉTÉ→SAVOIR, chaque palier GATÉ par
 * la métabolisation (décision #2 : FORGE exige ≥3 héritages métabolisés, SOCIÉTÉ
 * ≥4, SAVOIR ≥6 — sous son compte, le palier NI NE DÉMARRE NI NE PROGRESSE, même
 * si la rare alimente) et dévore SA rare tant qu'il alimente ; charge-additive
 * (ascension ET apocalypse sur la MÊME course) ; VICTOIRE = palier 3 (SAVOIR)
 * COMPLÉTÉ — les anciennes conditions (arbre complet + assimilation totale du
 * monde) SONT RETIRÉES du verdict (la thèse du contact métabolisé remplace la
 * conquête totale). `ts` sert au gate de palier (endgame_metab_count_ts lit
 * ts[].arch_depth — la profondeur de contact, MAX-ée avec la métabolisation). */
static void wonder_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                        const TechState ts[], int player, int year) {
    if (eg->merv == MERV_NONE || eg->merv == MERV_ASCENDED) return;
    if (eg->merv_country < 0) return;
    int site = eg->merv_site_reg;
    bool done_state = (eg->merv==MERV_FORGE_DONE || eg->merv==MERV_SOCIETE_DONE || eg->merv==MERV_SAVOIR_DONE);
    if (!done_state) {
        int req = endgame_metab_required(eg->merv);
        if (req > 0 && endgame_metab_count_ts(w, econ, ts, eg->merv_country) < req) return;  /* sous son compte : gelé */
        int phase = (eg->merv==MERV_FORGE)?0 : (eg->merv==MERV_SOCIETE)?1 : 2;
        float feed = endgame_empire_consume(econ, eg->merv_country, MERV_RARE[phase], MERV_RARE_PER_YEAR);
        if (feed > 0.f) {                                       /* la rare alimente → on bâtit */
            eg->merv_progress += 365.f / tune_f("MERV_PHASE_DAYS", 3650.f);
            if (site >= 0 && site < econ->n_regions) {          /* charge-additive (la Brèche se rapproche) */
                /* RE-KEY PROVINCE : arcane_charge/faust_charge sont PROVINCE-OWNED (Σ-agrégés) —
                 * route sur la représentative (équivalent inline de faust_charge_add(RegionEconomy*)
                 * pour une ProvinceEconomy, même idiome que scps_econ.c:1915). */
                float ch = tune_f("MERV_CHARGE_PER_TICK", 0.5f);
                int sitep = econ_region_rep_province(econ, site);
                if (sitep >= 0 && sitep < econ->n_prov) {
                    econ->prov[sitep].arcane_charge += ch;
                    econ->prov[sitep].faust_charge += ch;
                }
            }
            if (eg->merv_progress >= 1.0f) { eg->merv_progress = 0.f; eg->merv = (MervPhase)(eg->merv + 1); }
        }
        return;
    }
    /* palier bouclé : enchaîne, ou VICTOIRE dès SAVOIR bouclé (décision #2). */
    if (eg->merv == MERV_FORGE_DONE)        eg->merv = MERV_SOCIETE;
    else if (eg->merv == MERV_SOCIETE_DONE) eg->merv = MERV_SAVOIR;
    else if (eg->merv == MERV_SAVOIR_DONE) {
        eg->merv = MERV_ASCENDED;
        if (!eg->fired) {
            eg->fired = true; eg->fin = FIN_ASCENSION; eg->fin_year = year;
            endgame_faction_react(FIN_ASCENSION, player);
        }
        endgame_empire_vanish(w, econ, player);
    }
}

/* Démarrage de la Merveille (ordre agency AGENCY_BUILD_WONDER — JOUEUR uniquement).
 * LOT C : les Transgresseurs avancent DÈS le démarrage du chantier (pas d'attente
 * de la victoire — la seule DÉCISION de bâtir la Merveille les avance ; un lever
 * PLUS LÉGER que le tir final, cf. FAC_REACT_START < FAC_REACT_UNIVERSAL). */
#define FAC_REACT_START 0.10f
void endgame_start_wonder(EndgameState *eg, int player, int capital_region) {
    if (!eg || eg->merv != MERV_NONE) return;
    eg->merv = MERV_FORGE;
    eg->merv_country = player;
    eg->merv_site_reg = capital_region;
    eg->merv_progress = 0.f;
    if (player >= 0 && player < SCPS_MAX_COUNTRY)
        faction_lever_apply(player, FAC_TRANSGRESSEUR, FAC_REACT_START);
}

/* ── C2 — sélecteur + déclencheur (latch : un seul déclenchement) ──────────── */
static void endgame_select_and_fire(EndgameState *eg, const World *w,
                                     WorldEconomy *econ, const WorldProsperity *wp,
                                     const TechState ts[], int year) {
    if (eg->fired) return;

    /* Override MERVEILLE EN PREMIER (exempt du gate temporel et d'entropie) :
     * la victoire du joueur ne dépend pas du seuil mondial — si l'Ascension
     * est menée à bout, c'est elle qui l'emporte. */
    if (eg->merv == MERV_ASCENDED) {
        eg->fired = true; eg->fin = FIN_ASCENSION; eg->fin_year = year;
        endgame_faction_react(FIN_ASCENSION, eg->merv_country);
        return;
    }

    /* Gate temporel : les 3 apocalypses ne peuvent éclore avant ENDGAME_YEAR_OPEN. */
    if (year < (int)tune_f("ENDGAME_YEAR_OPEN", 180.f)) return;

    if (wp->entropy < tune_f("ENTROPY_FIN", 50.f)) return;

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

    /* SÉLECTION SANG (décision #1) : quelle que soit la nature qui a franchi la
     * barre — une barre, plusieurs nourritures — si le ratio morts-de-guerre/
     * pop_ref franchit ENDGAME_BLOOD_FRAC, LE SANG L'EMPORTE (visage dominant,
     * PAS un second seuil parallèle : le gate temporel + ENTROPY_FIN restent
     * les MÊMES conditions d'ouverture testées plus haut). */
    if (endgame_blood_ratio(eg, econ) >= (double)tune_f("ENDGAME_BLOOD_FRAC", 0.20f)) {
        /* Foyer SANG : la région vivante la plus ravagée (max revolt_scar), pas
         * forcément le foyer d'entropie (le sang a SA propre géographie). */
        int worst = -1; float worst_scar = -1.f;
        for (int r = 0; r < econ->n_regions && r < SCPS_MAX_REG; r++) {
            if (econ->region[r].owner < 0) continue;
            if (econ->region[r].revolt_scar > worst_scar) { worst_scar = econ->region[r].revolt_scar; worst = r; }
        }
        if (worst >= 0) {
            eg->epicenter_reg = worst;
            eg->fauteur_country = econ->region[worst].owner;
        }
        eg->fin   = FIN_SANG;
        eg->fired = true;
        sang_seed(eg, econ);
        endgame_faction_react(FIN_SANG, eg->fauteur_country);
        return;
    }

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
    endgame_faction_react(eg->fin, eg->fauteur_country);
}

void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  Campaign *camp, int player, int year) {
    (void)rn; (void)navy; (void)dp; (void)player;
    if (!eg || !wp) return;

    /* C1 — élargir l'entropie (savoir faustien + Âge de la Brèche + morts de guerre). */
    endgame_entropy_widen(eg, wp, ts, camp, econ, w ? w->n_countries : 0);

    /* Diag de CALIBRATION (SCPS_ENTDIAG=1) : la courbe d'entropie année par année.
     * OFF par défaut, stderr → déterminisme intact. */
    if (getenv("SCPS_ENTDIAG")) {
        static const char *FN[] = { "—", "EAU", "FROID", "RONCES", "ASCENSION", "SANG" };
        int fn_i = (int)eg->fin; if (fn_i < 0 || fn_i > 5) fn_i = 0;
        fprintf(stderr, "[ENTDIAG] an %d : entropie %.1f / fin %.0f%s%s%s\n",
                year, (double)wp->entropy, (double)tune_f("ENTROPY_FIN", 50.f),
                eg->fired ? " [" : "", eg->fired ? FN[fn_i] : "",
                eg->fired ? "]" : "");
    }

    /* C2 — pas encore déclenché : tester le seuil combiné et latcher (+ amorcer). */
    if (!eg->fired) {
        if (w && econ && ts) endgame_select_and_fire(eg, w, econ, wp, ts, year);
    }

    /* C3-C5 — dérouler la fin latchée (la carve voit la même année que le fire). */
    if (eg->fired && w && econ) {
        switch (eg->fin) {
            case FIN_EAU:    cataclysm_water_step(eg, w, econ, camp); break;
            case FIN_FROID:  cold_step(eg, w, econ); break;
            case FIN_RONCES: thorns_step(eg, w, econ, camp); break;
            case FIN_SANG:   sang_step(eg, econ); break;
            default: break;   /* ASCENSION : pas de carve (terre intacte) */
        }
    }

    /* C6 — la Merveille avance TOUJOURS (course ascension vs apocalypse) ; elle ne
     * peut vaincre que si une apocalypse n'a pas DÉJÀ latché (gate !fired interne). */
    if (w && econ) wonder_tick(eg, w, econ, ts, player, year);
}

/* ── LOT D — READER D'INTENSITÉ PAR RÉGION (0..1, PUR, pour le lavis V3) ────────
 * Aucun état muté : dérivé de l'EndgameState + du monde courant. viewer.c ne
 * l'appelle JAMAIS directement (passe par une façade, hors ce lot). */
float endgame_region_intensity(const EndgameState *eg, const World *w,
                               const WorldEconomy *econ, int region) {
    if (!eg || !w || !econ || region < 0 || region >= econ->n_regions) return 0.f;
    switch (eg->fin) {
        case FIN_EAU: {
            if (region < SCPS_MAX_REG) {
                if (eg->sunken[region] == 2) return 1.0f;     /* engloutie */
                if (eg->sunken[region] == 1) return 0.6f;     /* programmée (le rift la traverse) */
            }
            /* adjacente à une région engloutie : lueur d'alerte (≈0.3). */
            if (region < SCPS_MAX_REG)
                for (int o = 0; o < econ->n_regions && o < SCPS_MAX_REG; o++)
                    if (eg->sunken[o] == 2 && econ->adj[region][o]) return 0.3f;
            return 0.f;
        }
        case FIN_FROID:
            /* rampe globale (cold_offset [0..1]), un rien modulée par la température
             * locale (plus froid déjà ⇒ frappe plus fort en proportion — la région
             * polaire visualise le grand hiver avant la tropicale). Dérivé, aucun état. */
            return eg->cold_offset;
        case FIN_RONCES: {
            /* fraction de cellules BIO_THORNS de la région (scan direct — la même
             * vérité que le rendu de carte). */
            int land = 0, thn = 0;
            for (int i = 0; i < SCPS_N; i++) {
                const Cell *c = &w->cell[i];
                if (c->region != region) continue;
                if (c->height < SEA_LEVEL) continue;
                land++; if (c->biome == BIO_THORNS) thn++;
            }
            return (land > 0) ? (float)thn / (float)land : 0.f;
        }
        case FIN_SANG:
            return (region < SCPS_MAX_REG) ? eg->sang_scar[region] : 0.f;
        default:
            return 0.f;   /* AUCUNE / ASCENSION : rien à teindre */
    }
}
