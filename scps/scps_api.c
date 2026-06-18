/*
 * scps_api.c — implémentation de la façade C (voir scps_api.h).
 * Réutilise telles quelles les entrées du moteur ; aucune logique de sim neuve.
 */
#include "scps_api.h"
#include "scps_world.h"     /* world_generate, world_tick, worldparams_default, Cell, scps_cellc */
#include "scps_econ.h"      /* econ_init, econ_tick, colonize/migrate, econ_country_gold */
#include "scps_labor.h"     /* labor_init/tick/resync/seed */
#include "scps_agency.h"    /* agency_init, agency_advance */
#include "scps_credit.h"    /* credit_init (un seul livre d'or) */
#include "scps_render.h"    /* render_map, RenderParams, ViewMode */
#include "scps_tune.h"      /* tune_init (lit SCPS_TUNE une fois) */
#include "scps_tech.h"      /* TechState, tech_state_init (savoir/forge des readouts) */
#include "scps_prosperity.h"/* WorldProsperity, prosperity_init/tick */
#include "scps_legitimacy.h"/* WorldLegitimacy, legitimacy_init/tick (l'humeur) */
#include "scps_readout.h"   /* LA MEMBRANE : province_readout/country_readout → mots */
#include <stdlib.h>
#include <string.h>

struct ScpsSim {
    World           *w;
    WorldEconomy    *econ;
    LaborEcon       *lab;
    AgencyState     *ag;
    WorldProsperity *wp;     /* PE/SI — la prospérité (readouts pays) */
    WorldLegitimacy *wl;     /* L par région — l'humeur des provinces */
    TechState       *ts;     /* SCPS_MAX_COUNTRY : savoir/forge des readouts */
    uint32_t        *px;     /* SCPS_N : tampon de travail pour render_map */
    float           *cx, *cy;/* centroïdes région (figés par worldgen) */
    int              n_cent;
    int              day, year, player;
    bool             ready;
};

static void tune_once(void){ static bool done=false; if(!done){ tune_init(); done=true; } }

ScpsSim *scps_sim_new(void){
    ScpsSim *s = (ScpsSim*)calloc(1, sizeof *s);
    if(!s) return NULL;
    s->w    = (World*)          malloc(sizeof(World));
    s->econ = (WorldEconomy*)   malloc(sizeof(WorldEconomy));
    s->lab  = (LaborEcon*)      malloc(sizeof(LaborEcon));
    s->ag   = (AgencyState*)    malloc(sizeof(AgencyState));
    s->wp   = (WorldProsperity*)malloc(sizeof(WorldProsperity));
    s->wl   = (WorldLegitimacy*)malloc(sizeof(WorldLegitimacy));
    s->ts   = (TechState*)      calloc((size_t)SCPS_MAX_COUNTRY, sizeof(TechState));
    s->px   = (uint32_t*)       malloc((size_t)SCPS_N * sizeof(uint32_t));
    if(!s->w || !s->econ || !s->lab || !s->ag || !s->wp || !s->wl || !s->ts || !s->px){
        scps_sim_free(s); return NULL;
    }
    return s;
}

void scps_sim_free(ScpsSim *s){
    if(!s) return;
    free(s->w); free(s->econ); free(s->lab); free(s->ag);
    free(s->wp); free(s->wl); free(s->ts);
    free(s->px); free(s->cx); free(s->cy); free(s);
}

void scps_sim_generate(ScpsSim *s, uint32_t seed){
    if(!s) return;
    tune_once();
    WorldParams p = worldparams_default(seed);
    world_generate(s->w, &p);
    econ_init(s->econ, s->w);
    gen_population(s->w, s->econ);
    worldgen_seed_peoples(s->w, s->econ, RACE_HUMAIN);
    agency_init(s->ag);
    credit_init();
    labor_init(s->lab, s->w);
    s->player = 0;
    for(int c=0; c<s->w->n_countries; c++)
        if(s->w->country[c].role == POLITY_PLAYER){ s->player = c; break; }
    labor_seed_from_world(s->lab, s->w, s->econ, s->player);

    /* PROSPÉRITÉ + LÉGITIMITÉ + TECH (les readouts des panneaux). Lisent econ/world
     * en CONST (n'écrivent QUE wp/wl) → la colonne économique reste byte-identique.
     * Tech au socle (la colonne éco ne fait pas de recherche : savoir/forge de base). */
    for(int c=0; c<s->w->n_countries; c++) tech_state_init(&s->ts[c], false);
    prosperity_init(s->wp, s->w);
    legitimacy_init(s->wl, s->w, s->econ);
    /* un premier pas L→P : un panneau ouvert à l'an-0 (avant tout avancement) lit
     * des valeurs vivantes, pas le seul état d'amorce. */
    legitimacy_tick(s->wl, s->w, s->econ, s->ts);
    prosperity_tick(s->wp, s->w, s->econ, NULL, s->ts, s->wl);

    s->day = 0; s->year = 0; s->ready = true;

    /* centroïdes région (la géo est figée par worldgen ; seul l'OWNER changera) */
    int nr = s->econ->n_regions; s->n_cent = nr;
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

/* La COLONNE économique : strictement la boucle du banc audit_eco (déterministe,
 * auto-suffisante : ni IA, ni guerre, ni diplo — ceux-là exigent le Sim PLEIN). */
void scps_sim_advance_days(ScpsSim *s, int ndays){
    if(!s || !s->ready) return;
    for(int i=0; i<ndays; i++){
        agency_advance(s->ag, s->w, s->econ, NULL, NULL, 1);
        labor_tick(s->lab);
        if(s->day % 30 == 29){ econ_tick(s->econ, 1.f/12.f); labor_resync_pop(s->lab, s->econ); }
        if(s->day % 365 == 364){
            econ_colonize_tick(s->econ, s->w, -1);
            econ_migrate_tick(s->econ, s->w);
            world_tick(s->w, s->econ, 1.0f);
            /* L émerge (lit l'éco fraîche) AVANT la prospérité — l'ordre canonique.
             * Tous deux lisent econ/world en CONST → la pop ne bouge pas. */
            legitimacy_tick(s->wl, s->w, s->econ, s->ts);
            prosperity_tick(s->wp, s->w, s->econ, NULL, s->ts, s->wl);
            s->year++;
        }
        s->day++;
    }
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
    const RegionEconomy *re = &s->econ->region[r];
    return (long)(re->strata[0].pop + re->strata[1].pop + re->strata[2].pop);
}

int  scps_year         (const ScpsSim *s){ return s ? s->year : 0; }
int  scps_player       (const ScpsSim *s){ return s ? s->player : 0; }
int  scps_country_count(const ScpsSim *s){ return (s && s->ready) ? s->w->n_countries : 0; }
int  scps_region_count (const ScpsSim *s){ return (s && s->ready) ? s->econ->n_regions : 0; }

long scps_region_pop(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->econ->n_regions) return 0;
    return region_pop_i(s, r);
}
int scps_region_owner(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->econ->n_regions) return -1;
    return s->econ->region[r].owner;
}
bool scps_region_colonized(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->econ->n_regions) return false;
    return s->econ->region[r].colonized;
}
bool scps_region_centroid(const ScpsSim *s, int r, float *x, float *y){
    if(!s || !s->ready || r<0 || r>=s->n_cent || s->cx[r]<0.f) return false;
    if(x) *x = s->cx[r];
    if(y) *y = s->cy[r];
    return true;
}

long scps_world_pop(const ScpsSim *s){
    if(!s || !s->ready) return 0;
    long t=0; for(int r=0; r<s->econ->n_regions; r++) t += region_pop_i(s, r);
    return t;
}
long scps_country_pop(const ScpsSim *s, int c){
    if(!s || !s->ready) return 0;
    long t=0; for(int r=0; r<s->econ->n_regions; r++) if(s->econ->region[r].owner==c) t += region_pop_i(s, r);
    return t;
}
double scps_country_gold(const ScpsSim *s, int c){
    if(!s || !s->ready) return 0.0;
    return econ_country_gold(s->econ, c);
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

    ProvinceReadout pr = province_readout(s->w, s->econ, s->wp, s->wl, pid);
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
    out->owner          = (reg>=0 && reg<s->econ->n_regions) ? s->econ->region[reg].owner : -1;
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

    CountryReadout  cr = country_readout(s->wp, s->ts, s->w, cid);
    FactionsReadout fr = faction_readout(s->w, s->econ, cid);

    out->valid        = 1;
    out->nom          = sz(s->w->country[cid].name);
    out->ethos        = sz(fr.dominant);
    out->pop          = scps_country_pop(s, cid);
    out->gold         = econ_country_gold(s->econ, cid);
    out->n_regions    = s->w->country[cid].n_regions;
    out->stabilite    = cr.m_stabilite.value;  out->stabilite_mot  = sz(cr.m_stabilite.word);
    out->prosperite   = cr.m_prosperite.value; out->prosperite_mot = sz(cr.m_prosperite.word);
    out->legitimite   = cr.m_legitimite.value; out->legitimite_mot = sz(cr.m_legitimite.word);
    out->cohesion     = cr.m_cohesion.value;   out->cohesion_mot   = sz(cr.m_cohesion.word);
    out->savoir       = cr.m_savoir.value;     out->savoir_mot     = sz(cr.m_savoir.word);
    out->influence    = cr.influence;
    out->corruption   = cr.corruption;
}
