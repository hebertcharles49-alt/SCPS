/*
 * scps_api.c — implémentation de la façade C (voir scps_api.h).
 *
 * Le monde Godot roule désormais le TICK PLEIN : scps_sim_advance_days appelle
 * `sim_day` (le cœur PARTAGÉ avec la chronique, scps_sim.c) — IA, guerre, diplo,
 * navy, révolte, prospérité, endgame… plus seulement la colonne économique. Les
 * readouts (or, prospérité, influence) montrent enfin du RÉEL. Aucune logique de
 * sim n'est écrite ici : on délègue à sim_init/sim_day, à l'identique de chronicle.
 *
 * ⚠ UN SEUL ScpsSim actif par PROCESSUS (cf. scps_sim.h : intertrade/factions
 * portent un état global remis à plat par sim_init). Le pays « joueur » est, pour
 * l'instant, piloté par l'IA comme les autres (le monde VIT et s'observe) ; la
 * main humaine sur ce pays viendra avec la phase d'agency joueur.
 */
#include "scps_api.h"
#include "scps_sim.h"       /* Sim, sim_alloc/sim_init/sim_day (+ tout le moteur) */
#include "scps_render.h"    /* render_map, RenderParams, ViewMode */
#include "scps_tune.h"      /* tune_init (lit SCPS_TUNE une fois) */
#include "scps_readout.h"   /* LA MEMBRANE : province_readout/country_readout → mots */
#include <stdlib.h>
#include <string.h>

struct ScpsSim {
    World    *w;
    Sim       sim;       /* l'état PLEIN : sim_init/sim_day (IA, guerre, diplo, prospérité…) */
    uint32_t *px;        /* SCPS_N : tampon de travail pour render_map */
    float    *cx, *cy;   /* centroïdes région (figés par worldgen) */
    int       n_cent;
    bool      ready;
};

static void tune_once(void){ static bool done=false; if(!done){ tune_init(); done=true; } }

ScpsSim *scps_sim_new(void){
    ScpsSim *s = (ScpsSim*)calloc(1, sizeof *s);   /* zéro → tous les pointeurs Sim NULL */
    if(!s) return NULL;
    s->w  = (World*)   malloc(sizeof(World));
    s->px = (uint32_t*)malloc((size_t)SCPS_N * sizeof(uint32_t));
    if(!s->w || !s->px || !sim_alloc(&s->sim)){ scps_sim_free(s); return NULL; }
    return s;
}

void scps_sim_free(ScpsSim *s){
    if(!s) return;
    sim_free_members(&s->sim);
    free(s->w); free(s->px); free(s->cx); free(s->cy); free(s);
}

void scps_sim_generate(ScpsSim *s, uint32_t seed){
    if(!s) return;
    tune_once();
    WorldParams p = worldparams_default(seed);
    world_generate(s->w, &p);
    sim_init(&s->sim, s->w);   /* RAZ pleine + seed : econ, peuples, IA, diplo, prospérité, légitimité… */
    s->ready = true;

    /* centroïdes région (la géo est figée par worldgen ; seul l'OWNER changera) */
    int nr = s->sim.econ->n_regions; s->n_cent = nr;
    s->cx = (float*)realloc(s->cx, (size_t)nr*sizeof(float));
    s->cy = (float*)realloc(s->cy, (size_t)nr*sizeof(float));
    double *ax = (double*)calloc((size_t)nr, sizeof(double));
    double *ay = (double*)calloc((size_t)nr, sizeof(double));
    long   *cn = (long*)  calloc((size_t)nr, sizeof(long));
    if(s->cx && s->cy && ax && ay && cn){
        for(int y=0; y<SCPS_H; y++) for(int x=0; x<SCPS_W; x++){
            int r = scps_cellc(s->w, x, y)->region;
            if(r>=0 && r<nr){ ax[r]+=x; ay[r]+=y; cn[r]++; }
        }
        for(int r=0; r<nr; r++){
            if(cn[r]){ s->cx[r]=(float)(ax[r]/(double)cn[r]); s->cy[r]=(float)(ay[r]/(double)cn[r]); }
            else     { s->cx[r]=-1.f; s->cy[r]=-1.f; }
        }
    }
    free(ax); free(ay); free(cn);
}

/* Le TICK PLEIN : exactement le sim_day de la chronique (déterministe). */
void scps_sim_advance_days(ScpsSim *s, int ndays){
    if(!s || !s->ready) return;
    for(int i=0; i<ndays; i++) sim_day(&s->sim, s->w);
}

int scps_map_w(void){ return SCPS_W; }
int scps_map_h(void){ return SCPS_H; }

void scps_map_rgba(ScpsSim *s, uint8_t *dst, int mode, int selected_prov){
    if(!s || !s->ready || !dst) return;
    RenderParams rp; memset(&rp, 0, sizeof rp);
    rp.cam_ox=0.f; rp.cam_oy=0.f; rp.cam_scale=1.f; rp.selected_prov=selected_prov;
    rp.show_rivers=true; rp.show_borders=true; rp.iso=false; rp.screen_strokes=false;
    render_map(s->w, s->px, SCPS_W, SCPS_H, &rp, (ViewMode)mode);
    /* render_map sort de l'ARGB8888 (uint32) ; on swizzle en RGBA octets (Godot). */
    for(int i=0; i<SCPS_N; i++){
        uint32_t c = s->px[i];
        dst[i*4+0] = (uint8_t)((c>>16)&0xFF);   /* R */
        dst[i*4+1] = (uint8_t)((c>>8 )&0xFF);   /* G */
        dst[i*4+2] = (uint8_t)( c     &0xFF);   /* B */
        dst[i*4+3] = 255;                        /* A */
    }
}

void scps_map_layer(ScpsSim *s, uint8_t *dst, int layer){
    if(!s || !s->ready || !dst) return;
    for(int y=0; y<SCPS_H; y++) for(int x=0; x<SCPS_W; x++){
        const Cell *c = scps_cellc(s->w, x, y);
        int i = y*SCPS_W + x; uint8_t v = 0;
        switch(layer){
            case SCPS_LAYER_HEIGHT: { float h=c->height; if(h<0.f)h=0.f; if(h>1.f)h=1.f; v=(uint8_t)(h*255.f+0.5f); } break;
            case SCPS_LAYER_SEA:    v = (uint8_t)c->sea; break;
            case SCPS_LAYER_BIOME:  v = (uint8_t)c->biome; break;
            case SCPS_LAYER_COAST:  v = c->coast ? 255 : 0; break;
            default: v = 0;
        }
        dst[i] = v;
    }
}

static long region_pop_i(const ScpsSim *s, int r){
    const RegionEconomy *re = &s->sim.econ->region[r];
    return (long)(re->strata[0].pop + re->strata[1].pop + re->strata[2].pop);
}

int  scps_year         (const ScpsSim *s){ return s ? s->sim.year : 0; }
int  scps_player       (const ScpsSim *s){ return s ? s->sim.player : 0; }
int  scps_country_count(const ScpsSim *s){ return (s && s->ready) ? s->w->n_countries : 0; }
int  scps_region_count (const ScpsSim *s){ return (s && s->ready) ? s->sim.econ->n_regions : 0; }

long scps_region_pop(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return 0;
    return region_pop_i(s, r);
}
int scps_region_owner(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return -1;
    return s->sim.econ->region[r].owner;
}
bool scps_region_colonized(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return false;
    return s->sim.econ->region[r].colonized;
}
bool scps_region_centroid(const ScpsSim *s, int r, float *x, float *y){
    if(!s || !s->ready || r<0 || r>=s->n_cent || s->cx[r]<0.f) return false;
    if(x) *x = s->cx[r];
    if(y) *y = s->cy[r];
    return true;
}

long scps_world_pop(const ScpsSim *s){
    if(!s || !s->ready) return 0;
    long t=0; for(int r=0; r<s->sim.econ->n_regions; r++) t += region_pop_i(s, r);
    return t;
}
long scps_country_pop(const ScpsSim *s, int c){
    if(!s || !s->ready) return 0;
    long t=0; for(int r=0; r<s->sim.econ->n_regions; r++)
        if(s->sim.econ->region[r].owner==c) t += region_pop_i(s, r);
    return t;
}
double scps_country_gold(const ScpsSim *s, int c){
    if(!s || !s->ready) return 0.0;
    return econ_country_gold(s->sim.econ, c);
}

/* ---- PICKING & READOUTS (la membrane traverse le binding) ------------- */

static const char *sz(const char *p){ return p ? p : ""; }   /* NULL → "" (Godot String) */

int scps_province_at(const ScpsSim *s, int x, int y){
    if(!s || !s->ready || x<0 || y<0 || x>=SCPS_W || y>=SCPS_H) return -1;
    return scps_cellc(s->w, x, y)->province;
}
int scps_province_region(const ScpsSim *s, int pid){
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces) return -1;
    return s->w->province[pid].region;
}

void scps_province_info(ScpsSim *s, int pid, ScpsProvInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces) return;

    ProvinceReadout pr = province_readout(s->w, s->sim.econ, s->sim.wp, s->sim.wl, pid);
    int reg = s->w->province[pid].region;

    out->valid          = 1;
    out->nom            = sz(pr.nom);
    out->terrain        = sz(pr.terrain);
    out->climat         = sz(pr.climat);
    out->relief         = sz(pr.relief);
    out->race           = sz(pr.race);
    out->stature        = sz(label_stature(pr.stature));
    out->flux           = sz(label_flux(pr.flux));
    out->lignee         = sz(label_lignee(pr.lignee));
    out->vocation       = sz(pr.vocation);
    out->ressource      = sz(pr.ressource);
    out->humeur         = sz(pr.m_humeur.word);
    out->aisance        = sz(pr.m_aisance.word);
    out->defense        = sz(pr.defense);
    out->specialisation = sz(pr.specialisation);
    out->ames           = pr.ames;
    out->owner          = (reg>=0 && reg<s->sim.econ->n_regions) ? s->sim.econ->region[reg].owner : -1;
    out->agitation      = pr.agitation.value;
    out->aisance_val    = pr.m_aisance.value;
    out->humeur_val     = pr.m_humeur.value;
    out->seuil_revolte  = pr.seuil_revolte ? 1 : 0;
    out->logements_libres = pr.logements_libres; out->logements_cap = pr.logements_cap;
    out->services_libres  = pr.services_libres;  out->services_cap  = pr.services_cap;

    int nm = pr.n_mods; if(nm > SCPS_PROV_MODS) nm = SCPS_PROV_MODS;
    out->n_mods = nm;
    for(int i=0;i<nm;i++){
        out->mods[i].nom    = sz(pr.mods[i].nom);
        out->mods[i].effet  = sz(pr.mods[i].effet);
        out->mods[i].faveur = pr.mods[i].faveur ? 1 : 0;
    }
}

void scps_country_info(ScpsSim *s, int cid, ScpsCountryInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;

    CountryReadout  cr = country_readout(s->sim.wp, s->sim.ts, s->w, cid);
    FactionsReadout fr = faction_readout(s->w, s->sim.econ, cid);

    out->valid        = 1;
    out->nom          = sz(s->w->country[cid].name);
    out->ethos        = sz(fr.dominant);
    out->pop          = scps_country_pop(s, cid);
    out->gold         = econ_country_gold(s->sim.econ, cid);
    out->n_regions    = s->w->country[cid].n_regions;
    out->stabilite    = cr.m_stabilite.value;  out->stabilite_mot  = sz(cr.m_stabilite.word);
    out->prosperite   = cr.m_prosperite.value; out->prosperite_mot = sz(cr.m_prosperite.word);
    out->legitimite   = cr.m_legitimite.value; out->legitimite_mot = sz(cr.m_legitimite.word);
    out->cohesion     = cr.m_cohesion.value;   out->cohesion_mot   = sz(cr.m_cohesion.word);
    out->savoir       = cr.m_savoir.value;     out->savoir_mot     = sz(cr.m_savoir.word);
    out->influence    = cr.influence;
    out->corruption   = cr.corruption;
}

/* ---- ACTEURS SUR LA CARTE (Phase 3) : armées de campagne + tiers de ville --- */

void scps_army_info(ScpsSim *s, int cid, ScpsArmyInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    out->region = -1; out->dest = -1; out->owner = cid; out->phase = "";
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    if(!campaign_active(s->sim.camp, cid)) return;
    const FieldArmy *a = &s->sim.camp->army[cid];
    FieldPhase ph = campaign_phase(s->sim.camp, cid);
    out->active   = 1;
    out->region   = campaign_location(s->sim.camp, cid);
    out->dest     = a->dest;
    out->phase_id = (int)ph;
    out->phase    = sz(campaign_phase_name(ph));
    out->units    = campaign_units(s->sim.camp, cid);
    ArmyComposition comp = campaign_composition(s->sim.camp, cid);
    out->inf = comp.infanterie; out->arch = comp.archers;
    out->cav = comp.cavalerie;  out->mages = comp.mages;
}

int scps_region_tier(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return -1;
    if(!s->sim.econ->region[r].colonized) return -1;
    long pop = region_pop_i(s, r);
    int tier = pop>=4000?5 : pop>=1500?4 : pop>=500?3 : pop>=150?2 : pop>=50?1 : 0;
    /* la capitale domine : cité a minima (miroir du viewer) */
    for(int c=0;c<s->w->n_countries;c++){
        int cp = s->w->country[c].capital_prov;
        if(cp>=0 && cp<s->w->n_provinces && s->w->province[cp].region==r){
            if(tier<4) tier=4;
            break;
        }
    }
    return tier;
}

/* ---- ENDGAME §27 (Phase 4) -------------------------------------------- */

void scps_endgame_info(ScpsSim *s, ScpsEndgameInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    out->entropie = ""; out->augure = ""; out->epicenter_reg = -1;
    if(!s || !s->ready) return;
    EndgameReadout er = endgame_readout(s->sim.wp, s->sim.eg);
    out->entropie_pct  = er.entropie_pct;
    out->entropie      = sz(label_entropie(er.entropie));
    out->augure        = sz(er.augure);
    out->fin           = (int)er.fin;
    out->merv          = (int)er.merv;
    out->merv_pct      = er.merv_progress_pct;
    out->cold_pct      = er.cold_pct;
    out->sink_pct      = er.sink_intensity;
    out->epicenter_reg = er.epicenter_reg;
}

int scps_region_sunken(const ScpsSim *s, int r){
    if(!s || !s->ready || !s->sim.eg || r<0 || r>=SCPS_MAX_REG) return 0;
    return (int)s->sim.eg->sunken[r];
}
