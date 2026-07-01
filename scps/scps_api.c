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
#include "scps_lang.h"      /* tr() : noms de conseillers (StrId) → mot */
#include "scps_provlog.h"   /* journal d'évènements provincial */
#include "scps_heritage.h"   /* CRÉATEUR DE CULTURE : héritages, traditions, override joueur */
#include "scps_culture.h"   /* ethos_name, enum Ethos */
#include "scps_world.h"     /* culture_make_name (ethnonyme façon Stellaris) */
#include "scps_save.h"      /* SAUVEGARDE partagée : scps_save_game/load/slot_info */
#include "scps_religion.h"  /* religion_reset (nouvelle partie) */
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct ScpsSim {
    World      *w;
    Sim         sim;       /* l'état PLEIN : sim_init/sim_day (IA, guerre, diplo, prospérité…) */
    WorldParams params;    /* les paramètres de la genèse courante (pour la sauvegarde) */
    uint32_t   *px;        /* SCPS_N : tampon de travail pour render_map */
    float      *cx, *cy;   /* centroïdes région (figés par worldgen) */
    int         n_cent;
    bool        ready;
};

/* OVERRIDE des paramètres de genèse (l'écran « Nouvelle partie »). Inactif par défaut
 * ⇒ scps_sim_generate utilise worldparams_default (comportement historique). */
static struct { bool active; ScpsWorldParams p; } g_wg = { false, {0,0,0,0,0,0,0,0,0} };

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

/* centroïdes région (la géo est figée par worldgen/chargement ; seul l'OWNER changera). */
static void api_centroids(ScpsSim *s){
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

void scps_sim_generate(ScpsSim *s, uint32_t seed){
    if(!s) return;
    tune_once();
    { const char *m=getenv("SCPS_MODS");   /* MODTOOLS : surcharge des valeurs si défini (sinon vanilla) */
      if (m && *m){ econ_moddata_load(m); tech_moddata_load(m); army_moddata_load(m); } }
    religion_reset();   /* nouvelle partie : registre religion + liens pays remis à plat */
    WorldParams p = worldparams_default(seed);
    /* NOUVELLE PARTIE : applique l'override de sliders s'il est posé (sinon défaut). */
    if(g_wg.active){
        p.n_empires     = g_wg.p.n_empires;
        p.n_city_states = g_wg.p.n_city_states;
        p.n_continents  = g_wg.p.n_continents;
        p.world_age     = g_wg.p.world_age;
        p.land_amount   = g_wg.p.land_amount;
        p.mountains     = g_wg.p.mountains;
        p.erosion       = g_wg.p.erosion;
        p.temperature   = g_wg.p.temperature;
        p.humidity      = g_wg.p.humidity;
    }
    s->params = p;   /* mémorisé pour la sauvegarde (l'en-tête sérialise WorldParams) */
    world_generate(s->w, &p);
    /* CRÉATEUR DE CULTURE : lier chaque empire à son SLOT par ordinal (slot 0 = joueur,
     * 1..N = empires IA dans l'ordre des cid) AVANT sim_init, pour que les leviers/héritage/
     * éthos choisis imprègnent la genèse. Aucun slot posé (défaut) ⇒ no-op total. */
    if (culture_any_active()){
        culture_reset_cid_map();
        int ord=1;
        for (int c=0;c<s->w->n_countries;c++){
            int role = s->w->country[c].role;
            if (role==POLITY_PLAYER)          culture_bind_cid(c, 0);
            else if (role==POLITY_ANTAGONIST) culture_bind_cid(c, ord++);
        }
    }
    sim_init(&s->sim, s->w);   /* RAZ pleine + seed : econ, peuples, IA, diplo, prospérité, légitimité… */
    /* DÉBRAYAGE DE L'IA : le pays « joueur » passe sous la MAIN HUMAINE. ai_on=false le
     * retire de TOUTES les boucles de DÉCISION de sim_day (ai_step, conseil-IA, spéculation,
     * la doctrine bâti/navale — toutes gardées par ai_on) ; human_player le retire de
     * l'engagement d'âge auto (la seule fuite non gardée par ai_on). Les systèmes PASSIFS
     * (éco, démographie, prospérité, usure de guerre…) tournent pour lui comme pour tous. */
    s->sim.human_player = s->sim.player;
    s->sim.ai_on[s->sim.player] = false;
    warhost_set_human(s->sim.player);   /* la main humaine : l'armée du joueur ne s'auto-mobilise plus */
    econ_flux_reset();   /* budget façade : repart d'une ardoise propre (le flux est un état GLOBAL) */
    s->ready = true;
    api_centroids(s);   /* centroïdes région (géo figée par worldgen) */
}

/* Le TICK PLEIN : exactement le sim_day de la chronique (déterministe). */
void scps_sim_advance_days(ScpsSim *s, int ndays){
    if(!s || !s->ready) return;
    for(int i=0; i<ndays; i++){
        if(s->sim.day % 365 == 0) econ_flux_reset();   /* budget façade : le flux porte sur l'ANNÉE courante */
        sim_day(&s->sim, s->w);
    }
}

int scps_map_w(void){ return SCPS_W; }
int scps_map_h(void){ return SCPS_H; }

/* MODES D'ÉTAT (stabilité · commerce · guerre · diplo) : la façade calcule une
 * teinte PAR RÉGION depuis l'état sim et la passe à render_map (comme culture/foi).
 * tint[r]=0 ⇒ ignoré ; on évite donc le noir pur. */
static void map_state_tint(ScpsSim *s, int mode, uint32_t *tint){
    int nreg = s->sim.econ->n_regions; if(nreg>SCPS_MAX_REG) nreg=SCPS_MAX_REG;
    int me = s->sim.player;
    int rcount[SCPS_MAX_REG];
    if(mode==VIEW_TRADE){
        memset(rcount, 0, sizeof rcount);
        for(int i=0;i<s->sim.rn->n;i++){ const TradeRoute *t=&s->sim.rn->route[i];
            if(!t->open) continue;
            if(t->ra>=0&&t->ra<SCPS_MAX_REG) rcount[t->ra]++;
            if(t->rb>=0&&t->rb<SCPS_MAX_REG) rcount[t->rb]++; }
    }
    for(int r=0;r<nreg;r++){
        int owner = s->sim.econ->region[r].owner;
        uint32_t col = 0x303038u;   /* neutre par défaut (gris-bleu sombre) */
        if(mode==VIEW_STABILITY){
            float L = s->sim.wl->L[r]; if(L<0.f)L=0.f; if(L>10.f)L=10.f;
            float t = L/10.f;        /* rouge (instable) → vert (stable) */
            col = ((unsigned)((1.f-t)*215.f+25.f)<<16) | ((unsigned)(t*195.f+30.f)<<8) | 40u;
        } else if(mode==VIEW_TRADE){
            int n=rcount[r]; float t = n>0 ? (n>4?1.f:n/4.f) : 0.f;
            col = n>0 ? (((unsigned)(70.f+t*175.f)<<16)|((unsigned)(50.f+t*115.f)<<8)|((unsigned)(30.f+t*25.f)))
                      : 0x2c2c34u;
        } else if(mode==VIEW_WAR){
            int occ = s->sim.dp->occupier[r];
            int at_war=0;
            if(owner>=0) for(int b=0;b<s->w->n_countries;b++)
                if(b!=owner && diplo_status(s->sim.dp,owner,b)==DIPLO_WAR){ at_war=1; break; }
            if(occ>=0 && occ!=owner) col = 0xC02820u;        /* occupé : rouge vif   */
            else if(at_war)          col = 0xC07820u;        /* belligérant : orange */
            else if(owner>=0)        col = 0x35502fu;        /* en paix : vert sombre */
        } else { /* VIEW_DIPLO : relation au JOUEUR */
            if(owner<0)        col = 0x303038u;
            else if(owner==me) col = 0x2f63c0u;              /* soi : bleu */
            else { DiploStatus st = diplo_status(s->sim.dp, me, owner);
                   col = (st==DIPLO_WAR)?0xC02828u : (st==DIPLO_ALLIED)?0x2fa050u : 0x6f6f78u; }
        }
        tint[r] = col ? col : 0x010101u;
    }
}

void scps_map_rgba(ScpsSim *s, uint8_t *dst, int mode, int selected_prov){
    if(!s || !s->ready || !dst) return;
    RenderParams rp; memset(&rp, 0, sizeof rp);
    rp.cam_ox=0.f; rp.cam_oy=0.f; rp.cam_scale=1.f; rp.selected_prov=selected_prov;
    rp.show_rivers=true; rp.show_borders=true; rp.iso=false; rp.screen_strokes=false;
    uint32_t tint[SCPS_MAX_REG];
    if(mode==VIEW_STABILITY || mode==VIEW_TRADE || mode==VIEW_WAR || mode==VIEW_DIPLO){
        memset(tint, 0, sizeof tint);
        map_state_tint(s, mode, tint);
        rp.region_tint = tint;
    }
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
            /* EAU = exactement ce que le RENDU peint en bleu : height < SEA_LEVEL (mer ouverte,
             * MAIS AUSSI bassins endoréiques sous le niveau que c->sea n'attrape pas) OU c->lake.
             * La couche SEA (c->sea) seule laissait des bourgs sur des cellules peintes en eau. */
            case SCPS_LAYER_WATER:  v = (c->height < SEA_LEVEL || c->lake) ? 255 : 0; break;
            case SCPS_LAYER_RIVER:  v = c->river; break;   /* débit accumulé (worldgen) → carve par l'hôte */
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
int  scps_province_count(const ScpsSim *s){ return (s && s->ready) ? s->w->n_provinces : 0; }

/* nombre de provinces possédées par `country` : compte direct sur prov[] (la vérité de
 * propriété, charte PROVINCE_MODEL.md) — alimente la topbar "provinces". */
int scps_country_province_count(const ScpsSim *s, int country){
    if(!s || !s->ready || country<0 || country>=s->w->n_countries) return 0;
    int n = 0;
    for(int pid=0; pid<s->sim.econ->n_prov; pid++)
        if(s->sim.econ->prov[pid].owner==country) n++;
    return n;
}

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
int scps_country_role(const ScpsSim *s, int c){
    if(!s || !s->ready || c<0 || c>=s->w->n_countries) return -1;
    return (int)s->w->country[c].role;
}

int scps_country_ethos(const ScpsSim *s, int c){
    if(!s || !s->ready || c<0 || c>=s->w->n_countries) return -1;
    int cp=s->w->country[c].capital_prov;
    int cr=(cp>=0 && cp<s->w->n_provinces)? s->w->province[cp].region : -1;
    if (cr>=0 && cr<s->sim.econ->n_regions) return (int)s->sim.econ->region[cr].culture.ethos;
    /* repli (slot sans capitale, ex. WILD) : éthos de la 1re région possédée */
    for (int r=0;r<s->sim.econ->n_regions;r++)
        if (s->sim.econ->region[r].owner==c) return (int)s->sim.econ->region[r].culture.ethos;
    return -1;
}

int scps_country_heritage(const ScpsSim *s, int c){
    if(!s || !s->ready || c<0 || c>=s->w->n_countries) return -1;
    int cp=s->w->country[c].capital_prov;
    int cr=(cp>=0 && cp<s->w->n_provinces)? s->w->province[cp].region : -1;
    if (cr>=0 && cr<s->sim.econ->n_regions) return (int)s->sim.econ->region[cr].culture.heritage;
    for (int r=0;r<s->sim.econ->n_regions;r++)
        if (s->sim.econ->region[r].owner==c) return (int)s->sim.econ->region[r].culture.heritage;
    return -1;
}

int scps_country_capital_region(const ScpsSim *s, int c){
    if(!s || !s->ready || c<0 || c>=s->w->n_countries) return -1;
    int cp=s->w->country[c].capital_prov;
    return (cp>=0 && cp<s->w->n_provinces)? s->w->province[cp].region : -1;
}

/* CONTOUR d'UNE région (arêtes où le voisin change de région) — normale vers l'EXTÉRIEUR. Sert au
 * liseré POURPRE de la capitale (un anneau autour de la province-capitale). */
int scps_region_border_segments(ScpsSim *s, int region, ScpsSegC *out, int max){
    if(!s || !s->ready || !out || max<=0 || region<0) return 0;
    int n=0;
    for (int y=0; y<SCPS_H && n<max; y++) for (int x=0; x<SCPS_W && n<max; x++){
        const Cell *a=scps_cellc(s->w,x,y);
        if (a->region!=region) continue;
        const Cell *e=(x+1<SCPS_W)?scps_cellc(s->w,x+1,y):NULL;
        if ((!e||e->region!=region) && n<max){ out[n].x0=(float)(x+1);out[n].y0=(float)y;out[n].x1=(float)(x+1);out[n].y1=(float)(y+1);out[n].nx=1;out[n].ny=0;out[n].owner=region;out[n].other=-1;n++; }
        const Cell *we=(x>0)?scps_cellc(s->w,x-1,y):NULL;
        if ((!we||we->region!=region) && n<max){ out[n].x0=(float)x;out[n].y0=(float)y;out[n].x1=(float)x;out[n].y1=(float)(y+1);out[n].nx=-1;out[n].ny=0;out[n].owner=region;out[n].other=-1;n++; }
        const Cell *so=(y+1<SCPS_H)?scps_cellc(s->w,x,y+1):NULL;
        if ((!so||so->region!=region) && n<max){ out[n].x0=(float)x;out[n].y0=(float)(y+1);out[n].x1=(float)(x+1);out[n].y1=(float)(y+1);out[n].nx=0;out[n].ny=1;out[n].owner=region;out[n].other=-1;n++; }
        const Cell *no=(y>0)?scps_cellc(s->w,x,y-1):NULL;
        if ((!no||no->region!=region) && n<max){ out[n].x0=(float)x;out[n].y0=(float)y;out[n].x1=(float)(x+1);out[n].y1=(float)y;out[n].nx=0;out[n].ny=-1;out[n].owner=region;out[n].other=-1;n++; }
    }
    return n;
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

    out->valid          = 1;
    out->nom            = sz(pr.nom);
    out->terrain        = sz(pr.terrain);
    out->climat         = sz(pr.climat);
    out->relief         = sz(pr.relief);
    out->heritage           = sz(pr.heritage);
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
    /* PROPRIÉTAIRE : la vérité vit sur la province elle-même (charte PROVINCE_MODEL.md) —
     * econ->region[reg].owner n'est qu'un REPRÉSENTANT (capitale, sinon la plus peuplée de
     * la région) et mentirait pour toute province non-dominante de sa région. */
    out->owner          = (pid>=0 && pid<s->sim.econ->n_prov) ? s->sim.econ->prov[pid].owner : -1;
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
    /* MÉTABOLISATION (Temps 1) : le +% de recherche que vaut le creuset digéré (W·part). */
    out->metab_pct    = (int)(tune_f("AI_METAB_RES_W",AI_METAB_RES_W)
                              * econ_country_metabolized(s->w, s->sim.econ, cid) * 100.f + 0.5f);
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

int scps_region_settle_group(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return -1;
    const RegionEconomy *re = &s->sim.econ->region[r];
    if(!re->colonized) return -1;
    /* capitale ? */
    int cap = 0;
    for(int c=0;c<s->w->n_countries;c++){
        int cp = s->w->country[c].capital_prov;
        if(cp>=0 && cp<s->w->n_provinces && s->w->province[cp].region==r){ cap=1; break; }
    }
    /* cellule du centroïde (biome / rivière) */
    const Cell *cell = NULL;
    if(r<s->n_cent && s->cx[r]>=0.f){
        int cx = (int)s->cx[r], cy = (int)s->cy[r];
        if(cx>=0 && cy>=0 && cx<SCPS_W && cy<SCPS_H) cell = scps_cellc(s->w, cx, cy);
    }
    if(re->coastal)               return cap ? 5 : 3;           /* côtier : fortifié si capitale, sinon rural */
    if(cell && cell->river>40 && !cell->lake) return 1;         /* rivière */
    if(cell){ Biome b=cell->biome;
        if(b==BIO_HIGHLANDS||b==BIO_HILLS||b==BIO_MOUNTAINS||b==BIO_PEAK) return 0; }  /* montagne */
    if(cap)                       return 5;                     /* capitale fortifiée */
    return 3;                                                   /* rural */
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

/* ---- DÉTAIL DE PROVINCE (port fidèle de viewer.c) --------------------- */

int scps_province_groups(ScpsSim *s, int pid, ScpsGroup *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    int reg = s->w->province[pid].region;
    if(reg<0 || reg>=s->sim.econ->n_regions) return 0;
    RegionEconomy *re = &s->sim.econ->region[reg];
    if(re->pop.n_groups<=0) return 0;
    /* la doctrine du TRÔNE (crown) : culture de la région-capitale du propriétaire. */
    int owner = re->owner;
    const PopCulture *crown = &re->culture;
    if(owner>=0 && owner<s->w->n_countries){
        int cp = s->w->country[owner].capital_prov;
        if(cp>=0 && cp<s->w->n_provinces){
            int cr = s->w->province[cp].region;
            if(cr>=0 && cr<s->sim.econ->n_regions) crown = &s->sim.econ->region[cr].culture;
        }
    }
    GroupReadout gr[SCPS_MAX_GROUPS];
    int ng = province_composition(&re->pop, s->sim.drift, crown, 5.f, 5.f, gr, SCPS_MAX_GROUPS);
    if(ng>max) ng=max;
    for(int i=0;i<ng;i++){
        out[i].heritage     = sz(gr[i].heritage);
        out[i].culture  = sz(gr[i].culture);
        out[i].religion = sz(gr[i].religion);
        out[i].klass    = sz(gr[i].klass);
        out[i].etat     = sz(gr[i].etat);
        out[i].loyaute  = sz(label_humeur(gr[i].loyaute));
        out[i].percent  = gr[i].percent;
    }
    return ng;
}

int scps_province_income(ScpsSim *s, int pid, ScpsIncome *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    int reg = s->w->province[pid].region;
    IncomeReadout inc = province_income(s->sim.econ, reg);
    int n = inc.n; if(n>max) n=max;
    for(int i=0;i<n;i++){
        out[i].source       = sz(inc.line[i].source);
        out[i].per_day      = inc.line[i].per_day;
        out[i].manufactured = inc.line[i].manufactured ? 1 : 0;
        out[i].res_id       = inc.line[i].good;
    }
    return n;
}

/* le POURQUOI de l'AGITATION d'une province : *out_value ← le nombre 0-100 ; out[]
 * ← les causes signées (mots résolus + points), triées par poids. Retourne n lignes. */
int scps_province_agitation(ScpsSim *s, int pid, int *out_value, ScpsBreakdownLine *out, int max){
    if(out_value) *out_value = 0;
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    ProvinceReadout pr = province_readout(s->w, s->sim.econ, s->sim.wp, s->sim.wl, pid);
    if(out_value) *out_value = pr.agitation.value;
    int n = pr.agitation_why.n; if(n>max) n=max;
    for(int i=0;i<n;i++){
        out[i].cause = sz(pr.agitation_why.line[i].cause);
        out[i].delta = pr.agitation_why.line[i].delta;
        out[i].decay = pr.agitation_why.line[i].decay;
    }
    return n;
}

/* les MANUFACTURES bâties dans la province : nom + niveau (capacité) + ouvriers
 * (emploi effectif). Lues de ProvinceEconomy.bld[] DIRECT (charte PROVINCE_MODEL.md :
 * le bâti est une vérité par-province ; RegionEconomy.bld[] n'est qu'un miroir de la
 * province REPRÉSENTATIVE de la région, pas une somme — lire la province évite de
 * montrer les bâtiments d'une voisine). Retourne n (trié par niveau desc). */
int scps_province_buildings(ScpsSim *s, int pid, ScpsProvBld *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    if(pid>=s->sim.econ->n_prov) return 0;
    const ProvinceEconomy *re = &s->sim.econ->prov[pid];
    /* indices triés par niveau décroissant (le bâti le plus gros en tête) */
    int idx[ECON_MAX_BLD], k=0;
    for(int i=0;i<re->n_bld && i<ECON_MAX_BLD;i++) if(re->bld[i].level > 0.05f) idx[k++]=i;
    for(int i=0;i<k;i++) for(int j=i+1;j<k;j++)
        if(re->bld[idx[j]].level > re->bld[idx[i]].level){ int t=idx[i]; idx[i]=idx[j]; idx[j]=t; }
    int n = (k>max)?max:k;
    for(int i=0;i<n;i++){
        const Building *b = &re->bld[idx[i]];
        out[i].nom     = sz(building_name(b->type));
        out[i].niveau  = (int)(b->level + 0.5f);
        out[i].ouvriers= (int)(b->workers + 0.5f);
    }
    return n;
}

/* le JOURNAL d'évènements de la province : les dernières entrées (an + libellé +
 * signe), la PLUS RÉCENTE en tête. Lecture pure du tampon provlog. Retourne n. */
/* le mot de chaque stat touchée par un évènement (cf. JEFF_*) */
static const StrId JEFF_WORD[JEFF_N] = {
    STR_JLOG_POP, STR_JLOG_PROD, STR_GLOSS_AGITATION, STR_GLOSS_LEGIT, STR_JLOG_TRESOR
};
int scps_province_log(ScpsSim *s, int pid, ScpsLogEntry *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    int reg = s->w->province[pid].region;
    int n = provlog_count(reg); if(n>max) n=max;
    for(int i=0;i<n;i++){
        const ProvLogEntry *e = provlog_at(reg, i);
        if(!e){ n=i; break; }
        out[i].year  = e->year;
        out[i].label = (e->str_id>=0) ? sz(tr((StrId)e->str_id)) : sz(e->lit);
        out[i].sign  = e->sign;
        out[i].hover[0] = '\0';
        if(e->eff_str>=0){                                  /* MODIFICATEUR : sa ligne d'effet */
            snprintf(out[i].hover, sizeof out[i].hover, "%s", sz(tr((StrId)e->eff_str)));
        } else {                                            /* ÉVÈNEMENT : les stats touchées, ↑/↓ */
            int pos=0;
            for(int st=0; st<JEFF_N; st++){
                int dir = (int)((e->eff_dir >> (2*st)) & 3u);
                if(!dir) continue;
                pos += snprintf(out[i].hover+pos, (pos<(int)sizeof out[i].hover)?sizeof out[i].hover-pos:0,
                                "%s%s %s", pos?"  ·  ":"", sz(tr(JEFF_WORD[st])), dir==1?"↑":"↓");
            }
        }
    }
    return n;
}

void scps_province_classes(ScpsSim *s, int pid, long *lab, long *bourg, long *elite){
    long cp[3] = {0,0,0};
    if(s && s->ready && pid>=0 && pid<s->w->n_provinces && pid<s->sim.econ->n_prov){
        /* la province est la vérité (charte PROVINCE_MODEL.md) : strata[] y est TENU à
         * jour par econ_tick ; pop.n_groups n'y est pas encore peuplé (câblage moteur à
         * venir) → le repli sur strata[] fait le bon calcul dès aujourd'hui. */
        const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
        const ProvincePop *pp = &pe->pop;
        if(pp->n_groups>0){
            for(int gi=0; gi<pp->n_groups; gi++)
                for(int cc=0; cc<3; cc++) cp[cc] += pp->groups[gi].pop_by_class[cc];
        } else {
            cp[0]=(long)pe->strata[CLASS_LABORER].pop;
            cp[1]=(long)pe->strata[CLASS_BOURGEOIS].pop;
            cp[2]=(long)pe->strata[CLASS_ELITE].pop;
        }
    }
    if(lab)   *lab   = cp[0];
    if(bourg) *bourg = cp[1];
    if(elite) *elite = cp[2];
}

void scps_province_capitale(ScpsSim *s, int pid, ScpsCapitale *out){
    if(!out) return;
    memset(out, 0, sizeof *out); out->statut = "";
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces || pid>=s->sim.econ->n_prov) return;
    /* la population de LA province-siège (pas l'agrégat de sa région entière — une
     * région peut porter d'autres provinces que la capitale, charte PROVINCE_MODEL.md). */
    const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
    long pop = (long)(pe->strata[CLASS_LABORER].pop + pe->strata[CLASS_BOURGEOIS].pop
                     + pe->strata[CLASS_ELITE].pop);
    int tier = capitale_max_tier(pop);
    long admin = capitale_admin_pop(tier); if(admin>pop) admin = (pop/100)*100;
    out->statut       = sz(capitale_status(tier));
    out->tier         = tier;
    out->pop          = pop;
    out->logement_cap = capitale_housing(tier, admin);
    out->service_cap  = capitale_housing(tier, admin);
    out->prod_pct     = (int)((capitale_prodmult(tier, admin) - 1.f) * 100.f + 0.5f);
}

/* ---- SIDEBAR : agrégats PAYS (read-only) ------------------------------ */

void scps_country_demo(ScpsSim *s, int cid, ScpsCountryDemo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    double clp[3]={0,0,0}, cls[3]={0,0,0};
    for(int r=0; r<s->sim.econ->n_regions; r++){
        RegionEconomy *re = &s->sim.econ->region[r];
        if(re->owner!=cid || !re->colonized) continue;
        out->n_regions++;
        for(int c=0;c<3;c++){
            clp[c] += re->strata[c].pop;
            cls[c] += re->strata[c].satisfaction * re->strata[c].pop;
        }
    }
    for(int c=0;c<3;c++){
        out->cls_pop[c] = (long)clp[c];
        out->pop_total += (long)clp[c];
        out->cls_sat[c] = (clp[c]>0.0) ? (int)(100.0*cls[c]/clp[c] + 0.5) : 0;
    }
}

int scps_country_stocks(ScpsSim *s, int cid, ScpsStock *out, int max){
    if(!out || max<=0 || !s || !s->ready || cid<0 || cid>=s->w->n_countries) return 0;
    double dem[RES_COUNT]={0}, sup[RES_COUNT]={0}, stk[RES_COUNT]={0}, pri[RES_COUNT]={0};
    int nreg=0;
    for(int r=0; r<s->sim.econ->n_regions; r++){
        RegionEconomy *re = &s->sim.econ->region[r];
        if(re->owner!=cid || !re->colonized) continue;
        nreg++;
        for(int g=1; g<RES_COUNT; g++){
            dem[g] += re->demand[g]; sup[g] += re->supply[g];
            stk[g] += re->stock[g];  pri[g] += re->price[g];
        }
    }
    int n=0;
    for(int g=1; g<RES_COUNT && n<max; g++){
        if(!(stk[g]>0.5 || dem[g]>0.05 || sup[g]>0.05)) continue;   /* bien VIVANT seulement */
        float net = (float)(sup[g]-dem[g]) / 30.f;                  /* le tick éco est mensuel */
        if(net>-0.05f && net<0.05f) net = 0.f;
        out[n].name        = sz(resource_name((Resource)g));
        out[n].market_band = (int)band_marche((float)dem[g], (float)(sup[g]+stk[g]));
        out[n].marche      = sz(label_marche((BandMarche)out[n].market_band));
        out[n].stock       = (long)stk[g];
        out[n].net_day     = net;
        out[n].price       = (nreg>0) ? (float)(pri[g]/nreg) : 0.f;
        out[n].res_id      = g;
        if(net < -0.05f){ float dj = (float)stk[g]/(-net); out[n].coverage_days = (dj>365.f)?366:(int)dj; }
        else out[n].coverage_days = -1;
        n++;
    }
    return n;
}

int scps_country_trade(ScpsSim *s, int me, int *routes, double *export_gold,
                       int *has_centre, ScpsTradePartner *out, int max){
    if(routes)      *routes = 0;
    if(export_gold) *export_gold = 0.0;
    if(has_centre)  *has_centre = 0;
    if(!s || !s->ready || me<0 || me>=s->w->n_countries) return 0;
    if(routes)      *routes = intertrade_active_routes(s->sim.econ, s->sim.rn, s->sim.dp, me);
    if(export_gold) *export_gold = intertrade_export_gold(me);
    if(has_centre)  *has_centre = intertrade_country_has_centre(s->sim.econ, me) ? 1 : 0;
    int n=0;
    if(!out || max<=0) return 0;
    for(int c=0; c<s->w->n_countries && n<max; c++){
        if(c==me) continue;
        float v = intertrade_pair_value(me, c);
        int emb = intertrade_embargoed(me, c) ? 1 : 0;
        int war = (diplo_status(s->sim.dp, me, c)==DIPLO_WAR) ? 1 : 0;
        if(v<=0.5f && !emb && !war) continue;
        out[n].name    = sz(s->w->country[c].name);
        out[n].value   = v;
        out[n].at_war  = war;
        out[n].embargo = emb;
        out[n].status  = war ? "guerre" : emb ? "embargo" : (v>200.f) ? "florissant" : "modeste";
        n++;
    }
    return n;
}

int scps_country_council(ScpsSim *s, int me, ScpsCouncilSeat *out, int max){
    if(!out || max<=0 || !s || !s->ready || me<0 || me>=s->w->n_countries) return 0;
    uint32_t seed = s->w->seed;
    int n=0;
    for(int seat=0; seat<SC_COUNCIL_SEATS && n<max; seat++){
        out[n].seat = sz(tr((StrId)(STR_COUNCIL_SEAT_0+seat)));
        int slot = statecraft_council_seated(s->sim.sc, me, seat);
        if(slot>=0){
            out[n].filled    = 1;
            out[n].councilor = sz(tr((StrId)statecraft_council_cand_name(seed, me, seat, slot)));
            out[n].tier      = statecraft_council_cand_tier(seed, me, seat, slot);
        } else {
            out[n].filled = 0; out[n].councilor = ""; out[n].tier = 0;
        }
        n++;
    }
    return n;
}

int scps_country_relations(ScpsSim *s, int me, ScpsRelation *out, int max){
    if(!out || max<=0 || !s || !s->ready || me<0 || me>=s->w->n_countries) return 0;
    int n=0;
    for(int c=0; c<s->w->n_countries && n<max; c++){
        if(c==me || s->w->country[c].role==POLITY_UNCLAIMED) continue;
        if(regions_of(s->sim.econ, c) <= 0) continue;   /* pays VIVANT seulement */
        DiploStatus st = diplo_status(s->sim.dp, me, c);
        const char *sw;
        if      (st==DIPLO_WAR)                       sw = "Guerre";
        else if (st==DIPLO_ALLIED)                    sw = "Allié";
        else if (diplo_suzerain(s->sim.dp, c)==me)    sw = "Vassal";
        else if (diplo_suzerain(s->sim.dp, me)==c)    sw = "Suzerain";
        else                                          sw = "Neutre";
        out[n].name   = sz(s->w->country[c].name);
        out[n].status = sw;
        out[n].at_war = (st==DIPLO_WAR) ? 1 : 0;
        out[n].allied = (st==DIPLO_ALLIED) ? 1 : 0;
        out[n].opinion= statecraft_opinion(s->sim.sc, c, me);   /* #26 : ce que `c` pense de NOUS (mémoire) */
        out[n].country= c;                                      /* §3 : cible des verbes/options diplo */
        n++;
    }
    return n;
}

/* §3 — OPTIONS DIPLO : la légalité des verbes du joueur contre `target` + l'aperçu du consentement
 * (ai_consider_offer, l'opinion #26). Pour GRISER les boutons et montrer « il refusera » avant l'offre. */
int scps_diplo_options(ScpsSim *s, int target, ScpsDiploOptions *out){
    if (!out) return 0;
    memset(out, 0, sizeof *out);
    if (!s || !s->ready) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    int t = target;
    if (p<0 || p>=s->w->n_countries || t<0 || t>=s->w->n_countries || t==p) return 0;
    if (s->w->country[t].role==POLITY_UNCLAIMED || regions_of(s->sim.econ, t)<=0) return 0;
    DiploState *d = s->sim.dp;
    DiploStatus st = diplo_status(d, p, t);
    int at_war = (st==DIPLO_WAR);
    int slot   = (diplo_ally_count(d,p) < DIPLO_ALLY_SLOTS) && (diplo_ally_count(d,t) < DIPLO_ALLY_SLOTS);
    int emb    = intertrade_embargoed(p, t) ? 1:0;
    out->can_declare_war    = (!at_war && diplo_truce_days(d,p,t)<=0.f) ? 1:0;
    out->can_make_peace     = at_war ? 1:0;
    out->can_offer_alliance = (!at_war && st!=DIPLO_ALLIED && slot) ? 1:0;
    out->can_offer_pact     = (!at_war && !diplo_trade_pact(d,p,t)) ? 1:0;
    out->can_embargo        = emb ? 0:1;
    out->can_lift_embargo   = emb ? 1:0;
    out->would_accept_alliance = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_ALLIANCE) ? 1:0;
    out->would_accept_pact     = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_TRADE_PACT) ? 1:0;
    out->would_accept_peace    = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_PEACE) ? 1:0;
    return 1;
}

/* §3 — LÉGALITÉ de construction PAR RÉGION (le roster `debloque` gate la TECH ; ici la RÉGION+l'OR). */
int scps_build_legal(ScpsSim *s, int region, int edifice){
    if (!s || !s->ready || edifice<0 || edifice>=EDIFICE_COUNT) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries) return 0;
    int reg = region;
    if (reg<0){ int cp=s->w->country[p].capital_prov; reg = (cp>=0&&cp<s->w->n_provinces)? s->w->province[cp].region : -1; }
    if (reg<0 || reg>=s->sim.econ->n_regions || s->sim.econ->region[reg].owner != p) return 0;
    if (edifice_build_blocked(s->sim.econ, reg, (Edifice)edifice)) return 0;
    return credit_can_spend(s->sim.econ, s->w, p, agency_build_gold(s->sim.econ, reg, (Edifice)edifice)) ? 1:0;
}

void scps_country_army(ScpsSim *s, int cid, ScpsArmy *out){
    if(!out) return;
    memset(out, 0, sizeof *out); out->levy_name = "";
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    out->regiments = warhost_units(s->sim.host, cid);
    out->levy      = warhost_levy(s->sim.host, cid);
    out->levy_name = sz(warhost_levy_name(out->levy));
    int f=0; for(int t=0; t<HULL_COUNT; t++) f += s->sim.navy->n[cid].hull[t];
    out->fleet = f;
}

/* ====================================================================== */
/* CONSTRUCTION — roster militaire & édifices (les boutons + le survol)    */
/* ---------------------------------------------------------------------- *
 * Les chaînes COMPOSÉES (coût/éthos/contres) vivent dans des tampons static
 * par ligne, valides jusqu'au prochain appel — le binding les copie aussitôt
 * en String Godot. Les noms simples pointent dans les tables compilées (tr). */
static char g_ucost  [U_COUNT][48];
static char g_uethos [U_COUNT][72];
static char g_ufort  [U_COUNT][112];
static char g_ufaible[U_COUNT][112];

static void sj(char *buf, size_t cap, const char *add, int *n){
    size_t l = strlen(buf);
    if (*n && l+2 < cap){ buf[l]=','; buf[l+1]=' '; buf[l+2]=0; l+=2; }
    if (l < cap-1) strncat(buf, add, cap-l-1);
    (*n)++;
}

int scps_unit_roster(ScpsSim *s, int country, ScpsUnitDef *out, int max){
    if (!s || !s->ready || !out || max<=0) return 0;
    const TechState *ts = (country>=0 && country<SCPS_MAX_COUNTRY) ? &s->sim.ts[country] : NULL;
    int g10=5, fd=1; labor_upkeep_per100(&g10,&fd);
    int n=0;
    for (int t=0; t<U_COUNT && n<max; t++){
        const UnitDef *d = unit_def((UnitType)t);
        ScpsUnitDef *o = &out[n];
        o->type   = t;
        o->nom    = sz(unit_name((UnitType)t));
        o->classe = (d->from==LAB_ELITE) ? "Élite" : "Journalier";
        o->arme   = sz(weapon_name(d->weapon));
        Resource arm = unit_res_arm((UnitType)t);
        if (arm==RES_NONE) snprintf(g_ucost[t], sizeof g_ucost[t], "%d (fortune)", POP_PER_UNIT);
        else               snprintf(g_ucost[t], sizeof g_ucost[t], "%d %s", POP_PER_UNIT, resource_name(arm));
        o->cout = g_ucost[t];
        g_uethos[t][0]=0; { int ne=0; for (int f=0; f<6; f++)        /* éthos : affinité ≥ 2 */
            if (warhost_unit_affinity(f,t) >= 2.f) sj(g_uethos[t], sizeof g_uethos[t], faction_name(f), &ne); }
        o->ethos = g_uethos[t][0] ? g_uethos[t] : "—";
        g_ufort[t][0]=0; g_ufaible[t][0]=0; { int nf=0, nw=0;       /* contres : >1.5 bat · <0.75 battu */
            for (int j=0; j<U_COUNT; j++){ if (j==t) continue; float m=matchup((UnitType)t,(UnitType)j);
                if      (m>1.5f  && nf<3) sj(g_ufort[t],   sizeof g_ufort[t],   unit_name((UnitType)j), &nf);
                else if (m<0.75f && nw<3) sj(g_ufaible[t], sizeof g_ufaible[t], unit_name((UnitType)j), &nw); } }
        o->fort   = g_ufort[t][0]   ? g_ufort[t]   : "—";
        o->faible = g_ufaible[t][0] ? g_ufaible[t] : "—";
        o->entretien_or10   = g10;
        o->entretien_vivre  = fd;
        o->recrutable = unit_recruitable(ts,(UnitType)t) ? 1 : 0;
        n++;
    }
    return n;
}

int scps_building_roster(ScpsSim *s, int country, ScpsEdificeDef *out, int max){
    if (!s || !s->ready || !out || max<=0) return 0;
    const TechState *ts = (country>=0 && country<SCPS_MAX_COUNTRY) ? &s->sim.ts[country] : NULL;
    int cap_reg = -1;                              /* la capitale fixe le prix OR du chantier */
    if (country>=0 && country<s->w->n_countries){
        int cp = s->w->country[country].capital_prov;
        if (cp>=0 && cp<s->w->n_provinces) cap_reg = s->w->province[cp].region;
    }
    int n=0;
    for (int e=0; e<EDIFICE_COUNT && n<max; e++){
        const EdificeDef *d = edifice_def((Edifice)e);
        if (!d) continue;
        ScpsEdificeDef *o = &out[n];
        o->type = e;
        o->nom  = sz(edifice_name(e));
        o->days = d->days;
        int nc=0;
        for (int k=0; k<BUILD_RES_MAX && nc<SCPS_BUILD_COSTS; k++){
            if (d->cost.res[k]==RES_NONE || d->cost.qty[k]<=0.f) continue;
            o->cost[nc].res = resource_name(d->cost.res[k]);
            o->cost[nc].qty = (int)(d->cost.qty[k]+0.5f);
            nc++;
        }
        o->n_cost   = nc;
        o->gold     = (cap_reg>=0) ? (int)(agency_build_gold(s->sim.econ, cap_reg, (Edifice)e)+0.5f) : 0;
        o->debloque = edifice_unlocked(ts,(Edifice)e) ? 1 : 0;
        n++;
    }
    return n;
}

/* ALLOCATION DE MAIN-D'ŒUVRE — lit les PUITS d'une région (brutes extraites + manufactures)
 * avec leur poids et leur emploi. En mode AUTO (alloc_on=0), `weight` reflète une estimation
 * de la part de bras ACTUELLE (manufactures : bras réels ; brutes : part ∝ raw_cap) → l'UI
 * part de la distribution réelle. PUR read (aucun tick, aucun RNG). */
void scps_region_alloc(ScpsSim *s, int region, ScpsAlloc *out){
    if (!out) return;
    memset(out, 0, sizeof *out);
    out->region = -1;
    if (!s || !s->ready) return;
    WorldEconomy *e = s->sim.econ;
    if (region<0 || region>=e->n_regions) return;
    RegionEconomy *re = &e->region[region];
    out->region = region;
    out->on     = re->alloc_on;
    out->pool   = re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop;
    int n=0;
    float estw[SCPS_ALLOC_MAX];   /* estimation de bras par puits (mode AUTO) */
    /* — extraction : part du bassin EXTRACTION (~0.65) répartie ∝ raw_cap — */
    float sumrc=0.f; for (int g=1;g<RES_PROD_FIRST;g++) if (re->raw_cap[g]>0.f) sumrc+=re->raw_cap[g];
    float L_ext = out->pool * 0.65f;
    for (int g=1; g<RES_PROD_FIRST && n<SCPS_ALLOC_MAX; g++){
        if (re->raw_cap[g] <= 0.f) continue;
        ScpsAllocSink *k = &out->sink[n];
        k->kind=0; k->id=g; k->name=sz(resource_name((Resource)g));
        k->output=NULL; k->closed=0; k->input=-1; k->alt_name=NULL; k->in_name=NULL;
        estw[n] = (sumrc>1e-6f)? L_ext*re->raw_cap[g]/sumrc : 0.f;
        k->workers = re->alloc_on ? 0.f : estw[n];
        k->weight  = re->alloc_on ? re->alloc_raw[g] : 0;
        n++;
    }
    /* — manufactures bâties : bras RÉELS (b->workers) — */
    for (int i=0; i<re->n_bld && n<SCPS_ALLOC_MAX; i++){
        int b = re->bld[i].type;
        if (b<0 || b>=BLD_TYPE_COUNT) continue;
        Resource in1,in2,o; building_recipe((BuildingType)b,&in1,&in2,&o);
        Resource alt = building_alt_input((BuildingType)b);
        ScpsAllocSink *k = &out->sink[n];
        k->kind=1; k->id=b; k->name=sz(building_name((BuildingType)b));
        k->output  = (o>RES_NONE)? sz(resource_name(o)) : NULL;
        k->in_name = (in1>RES_NONE)? sz(resource_name(in1)) : NULL;
        k->alt_name= (alt!=RES_NONE)? sz(resource_name(alt)) : NULL;
        k->input   = (alt!=RES_NONE)? (int)re->bld_input[b] : -1;
        k->closed  = (re->alloc_on && re->alloc_bld[b]==0)?1:0;
        k->workers = re->bld[i].workers;
        estw[n]    = re->bld[i].workers;
        k->weight  = re->alloc_on ? re->alloc_bld[b] : 0;
        n++;
    }
    out->n = n;
    /* mode AUTO : poids = estimation de bras normalisée 0-100 (l'UI part du réel) */
    if (!re->alloc_on){
        float sw=0.f; for (int i=0;i<n;i++) sw+=estw[i];
        for (int i=0;i<n;i++) out->sink[i].weight = (sw>1e-6f)? (int)(100.f*estw[i]/sw+0.5f) : 0;
    }
    /* pct = part normalisée des poids courants */
    { float tw=0.f; for (int i=0;i<n;i++) tw+=(float)out->sink[i].weight;
      for (int i=0;i<n;i++) out->sink[i].pct = (tw>1e-6f)? (int)(100.f*out->sink[i].weight/tw+0.5f) : 0; }
}

/* ====================================================================== */
/* LECTURES DE FENÊTRES : arbre de tech · budget · missions (read-only)     */
/* Const, sans tick ni RNG (golden-safe) ; membrane (mots résolus + nombres */
/* tangibles, bandes pour le risque — jamais un flottant moteur brut).      */
/* ====================================================================== */

int scps_tech_nodes(ScpsSim *s, ScpsTechNode *out, int max){
    if(!out || max<=0 || !s || !s->ready) return 0;
    int p = s->sim.player;
    if(p<0 || p>=s->w->n_countries) return 0;
    unsigned acc = ai_heritage_access(s->w, s->sim.econ, s->sim.rn, p);
    float nprov = (float)s->w->country[p].n_regions;
    TechTreeReadout tt; tech_tree_readout(&s->sim.ts[p], acc, nprov, &tt);
    int n = (tt.n < max) ? tt.n : max;
    for(int i=0;i<n;i++){
        const TreeNodeReadout *nd = &tt.node[i];
        out[i].quarter = nd->quarter;  out[i].tier = nd->tier;
        out[i].state   = (int)nd->state;
        out[i].faustian= nd->faustian?1:0; out[i].orphan = nd->orphan?1:0;
        out[i].is_base = nd->is_base?1:0;
        out[i].name    = sz(nd->name);  out[i].unlocks = sz(nd->unlocks);
        out[i].effet   = sz(nd->effet);
        /* coût AFFICHÉ = coût de base × remise de diffusion (ce que le joueur paiera vraiment). */
        out[i].cost    = (int)(nd->cost * tech_diffusion_mult((TechId)i) + 0.5f);
        /* prérequis : node[i] ↔ TechId i, donc prereq (un TechId, TECH_COUNT=aucun)
         * est directement l'INDICE du nœud parent dans CE tableau → arête de l'arbre. */
        const TechNode *tn = tech_node((TechId)i);
        int pr = tn ? (int)tn->prereq : (int)TECH_COUNT;
        out[i].prereq = (pr < (int)TECH_COUNT && pr < n) ? pr : -1;
    }
    return n;
}

void scps_tech_info(ScpsSim *s, ScpsTechInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    for(int i=0;i<3;i++){ out->theme[i]=""; out->function[i]=""; }
    out->presage="";
    if(!s || !s->ready) return;
    int p = s->sim.player;
    if(p<0 || p>=s->w->n_countries) return;
    unsigned acc = ai_heritage_access(s->w, s->sim.econ, s->sim.rn, p);
    float nprov = (float)s->w->country[p].n_regions;
    TechTreeReadout tt; tech_tree_readout(&s->sim.ts[p], acc, nprov, &tt);
    out->points = tt.points;
    for(int i=0;i<3;i++){ out->theme[i]=sz(tt.theme[i]); out->function[i]=sz(tt.function[i]); }
    const TechState *ts = &s->sim.ts[p];
    float ch = ts->charge; if(ch<0.f)ch=0.f; if(ch>10.f)ch=10.f;
    out->presage = sz(label_presage(band_presage(ch)));
    float cp = tech_crisis_proximity(ts); if(cp<0.f)cp=0.f; if(cp>1.f)cp=1.f;
    out->crise_pct = (int)(cp*100.f + 0.5f);
    /* MÉTABOLISATION : le +% de recherche que vaut le creuset digéré (W·part) — pour le hover. */
    out->metab_pct = (int)(tune_f("AI_METAB_RES_W",AI_METAB_RES_W)
                           * econ_country_metabolized(s->w, s->sim.econ, p) * 100.f + 0.5f);
}

/* ACCÈS D'HÉRITAGE du joueur — la « barre de métabolisation » par héritage (tier 0..3 + part
 * digérée). Décode le masque d'accès gradué (Temps 2a) + la métabolisation ventilée. */
int scps_player_heritage_access(ScpsSim *s, ScpsHeritageAccess *out, int max){
    if(!out || max<=0 || !s || !s->ready) return 0;
    int p = s->sim.player;
    if(p<0 || p>=s->w->n_countries) return 0;
    unsigned acc = ai_heritage_access(s->w, s->sim.econ, s->sim.rn, p);
    float metab[HERITAGE_COUNT]; econ_country_heritage_digested(s->w, s->sim.econ, p, metab);
    int cp2 = s->w->country[p].capital_prov;
    int cr  = (cp2>=0 && cp2<s->w->n_provinces) ? s->w->province[cp2].region : -1;
    int nativ = (cr>=0 && cr<s->sim.econ->n_regions) ? (int)s->sim.econ->region[cr].culture.heritage : -1;
    int n = (HERITAGE_COUNT < max) ? HERITAGE_COUNT : max;
    for(int r=0;r<n;r++){
        out[r].nom          = sz(heritage_name((Heritage)r));
        out[r].tier         = tech_heritage_access_tier(acc, (Heritage)r);
        out[r].digested_pct = (int)(metab[r]*100.f + 0.5f);
        out[r].native       = (r==nativ) ? 1 : 0;
    }
    return n;
}

/* MODTOOLS — registre des tunables (panneau dev). GLOBAL : pas de s->sim. */
int  scps_tune_count(void){ return tune_count(); }
void scps_tune_at(int i, ScpsTunable *out){
    if(!out) return;
    out->nom        = sz(tune_name_at(i));
    out->value      = (double)tune_value_at(i);
    out->def_value  = (double)tune_default_at(i);
    out->overridden = tune_overridden_at(i);
}
void scps_tune_set_val(const char *nom, double value){ tune_set(nom, (float)value); }

int scps_country_budget(ScpsSim *s, int cid, ScpsFluxLine *out, int max){
    if(!out || max<=0 || !s || !s->ready || cid<0 || cid>=s->w->n_countries) return 0;
    int n=0;
    for(int comp=0; comp<FX_COUNT && n<max; comp++){
        double amt = econ_flux_get(cid, (FluxComp)comp);
        if(amt > -0.5 && amt < 0.5) continue;          /* poste vide → omis */
        out[n].name   = sz(econ_flux_name((FluxComp)comp));
        out[n].amount = amt;
        n++;
    }
    return n;
}

void scps_budget_summary(ScpsSim *s, int cid, ScpsBudget *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    out->creditor = -1; out->creditor_name = "";
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    double inc=0, exp=0;
    for(int comp=0; comp<FX_COUNT; comp++){
        double a = econ_flux_get(cid, (FluxComp)comp);
        if(a>0) inc+=a; else exp+= -a;                 /* dépenses stockées NÉGATIVES → |a| */
    }
    out->gold        = econ_country_gold(s->sim.econ, cid);
    out->income      = inc;
    out->expense     = exp;
    out->net         = inc - exp;
    out->credit_line = credit_line(s->w, s->sim.econ, cid);
    int cr = credit_of(cid);
    out->creditor = cr;
    if(cr>=0 && cr<s->w->n_countries) out->creditor_name = sz(s->w->country[cr].name);
}

void scps_mission_info(ScpsSim *s, int cid, ScpsMission *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    out->text=""; out->reward_mat="";
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    const Mission *m = mission_of(s->sim.missions, cid);
    if(!m) return;
    out->active      = 1;
    out->text        = sz(m->text);
    out->reward_gold = m->reward_gold;
    out->reward_qty  = m->reward_qty;
    if(m->reward_qty > 0.f) out->reward_mat = sz(resource_name(m->reward_mat));
    out->issued_year = m->issued_year;
    out->done        = m->done?1:0;
}

/* ====================================================================== */
/* ACTIONS DU JOUEUR — ENFILÉES dans le journal (déterministe), pas appliquées  */
/* ici : le drain de sim_day les passe aux MÊMES actionneurs que l'IA (agency/   */
/* warhost) à un point FIXE du tick. Retour = ACCEPTÉ-DANS-LA-FILE (1) / refus    */
/* d'enfilement (0, file pleine ou argument hors domaine) — PAS le verdict        */
/* d'application (trésor/matière), qui tombe au tick. cf. scps_sim.h.            */
/* ====================================================================== */
int scps_player_build(ScpsSim *s, int edifice, int region){
    if (!s || !s->ready || edifice<0 || edifice>=EDIFICE_COUNT) return 0;
    PlayerCmd c = { CMD_BUILD, { edifice, region, 0, 0 } };   /* region<0 ⇒ capitale (résolu au drain) */
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

long scps_player_recruit(ScpsSim *s, int unit){
    if (!s || !s->ready || unit<0 || unit>=U_COUNT) return 0;
    PlayerCmd c = { CMD_RECRUIT, { unit, 1, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

void scps_player_set_levy(ScpsSim *s, int level){
    if (!s || !s->ready) return;
    PlayerCmd c = { CMD_SET_LEVY, { level, 0, 0, 0 } };
    sim_cmd_push(&s->sim, c);
}

/* RECHERCHE : fixe la CIBLE de tech du joueur (file de 1). tech<0 ⇒ annule. La
 * progression/déblocage tombe au tick (income SAVOIR × prospérité, cf. sim_day). */
int scps_player_research(ScpsSim *s, int tech){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_RESEARCH, { tech, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* ── §3 — VERBES DIPLO (capstone #26) : le joueur PROPOSE, le vis-à-vis ÉVALUE au drain via
 * ai_consider_offer (alliance/paix/pacte) → l'offre n'aboutit que si l'OPINION+relation CONSENTENT.
 * declare_war / embargo sont UNILATÉRAUX. Retour = ACCEPTÉ-DANS-LA-FILE (1) ; le VERDICT
 * (l'autre a-t-il accepté ?) tombe au tick → se lit ensuite dans country_relations. */
int scps_player_declare_war(ScpsSim *s, int target){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_DECLARE_WAR, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_make_peace(ScpsSim *s, int target){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_MAKE_PEACE, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_offer_alliance(ScpsSim *s, int target){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_OFFER_ALLIANCE, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_offer_pact(ScpsSim *s, int target){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_OFFER_PACT, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_embargo(ScpsSim *s, int target, int on){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_EMBARGO, { target, on?1:0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* ── §3 — INTÉRIEUR · COMMERCE · GUERRE : plomberie additive (même motif que ci-dessus).
 * Tous ENFILENT (différé) ; chaque verbe est REVALIDÉ au drain (région à soi, indices bornés)
 * puis passé au MÊME actionneur que l'IA (agency/statecraft/intertrade/routes/campaign/navy). */
int scps_player_repress(ScpsSim *s, int region){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_REPRESS, { region, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_assimilate(ScpsSim *s, int region, int creuset){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ASSIMILATE, { region, creuset?1:0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_purge(ScpsSim *s, int region){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_PURGE, { region, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_council_hire(ScpsSim *s, int seat, int slot){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_COUNCIL_HIRE, { seat, slot, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_council_dismiss(ScpsSim *s, int seat){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_COUNCIL_DISMISS, { seat, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_route(ScpsSim *s, int ra, int rb, int maritime){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ROUTE, { ra, rb, maritime?1:0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_market_buy(ScpsSim *s, int region, int good, long qty, int tier){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_MARKET_BUY, { region, good, (int32_t)qty, tier } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_market_sell(ScpsSim *s, int region, int good, long qty, int tier){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_MARKET_SELL, { region, good, (int32_t)qty, tier } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_campaign(ScpsSim *s, int from_region, int target_region){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_CAMPAIGN, { from_region, target_region, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_posture(ScpsSim *s, int posture){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_POSTURE, { posture, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_refill(ScpsSim *s){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_REFILL, { 0, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_navy_build(ScpsSim *s, int hull){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_NAVY_BUILD, { hull, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_disband(ScpsSim *s){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_DISBAND, { 0, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_alloc_raw(ScpsSim *s, int region, int resource, int weight){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ALLOC_RAW, { region, resource, weight, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_alloc_bld(ScpsSim *s, int region, int bld_type, int weight){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ALLOC_BLD, { region, bld_type, weight, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_alloc_input(ScpsSim *s, int region, int bld_type, int input){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ALLOC_INPUT, { region, bld_type, input, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_alloc_auto(ScpsSim *s, int region){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_ALLOC_AUTO, { region, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* LECTURE : la cible de recherche COURANTE (-1 = aucune) ; *progress01 ← fraction
 * acquise [0..1] (points / coût plein) pour la jauge UI. Lecture pure. */
int scps_research_target(ScpsSim *s, float *progress01){
    if (progress01) *progress01 = 0.f;
    if (!s || !s->ready) return -1;
    int pl = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    int t  = s->sim.research_target;
    if (t<0 || pl<0 || pl>=s->w->n_countries) return -1;
    if (progress01){
        float cost = tech_cost((TechId)t, (float)s->w->country[pl].n_regions);
        float f = (cost>0.f) ? (s->sim.ts[pl].research_points / cost) : 0.f;
        *progress01 = f<0.f ? 0.f : (f>1.f ? 1.f : f);
    }
    return t;
}

int scps_river_points(ScpsSim *s, ScpsRiverPt *out, int max){
    if (!s || !s->ready || !out || max<=0) return 0;
    int n=0;
    for (int ri=0; ri<s->w->n_rivers && n<max; ri++){
        const River *rv = &s->w->river[ri];
        for (int k=0; k<rv->len && n<max; k++){
            float ax, ay, bx, by;                       /* fil sortant (direction locale) */
            if (k < rv->len-1){ ax=rv->x[k];   ay=rv->y[k];   bx=rv->x[k+1]; by=rv->y[k+1]; }
            else if (k > 0)   { ax=rv->x[k-1]; ay=rv->y[k-1]; bx=rv->x[k];   by=rv->y[k];   }
            else              { ax=rv->x[k];   ay=rv->y[k];   bx=ax+1.f;     by=ay;          }
            out[n].x   = (float)rv->x[k] + 0.5f;
            out[n].y   = (float)rv->y[k] + 0.5f;
            out[n].ang = atan2f(by-ay, bx-ax);
            n++;
        }
    }
    return n;
}

int scps_river_count(ScpsSim *s){
    if (!s || !s->ready) return 0;
    return s->w->n_rivers;
}

/* Le i-ème fleuve en FIL ORDONNÉ (points = centres de cellule), *flow = débit max. */
int scps_river_path(ScpsSim *s, int i, ScpsRiverPt *out, int max, float *flow){
    if (!s || !s->ready || !out || max<=0 || i<0 || i>=s->w->n_rivers) return 0;
    const River *rv = &s->w->river[i];
    if (flow) *flow = rv->flow_max;
    int n=0;
    for (int k=0; k<rv->len && n<max; k++){
        float ax, ay, bx, by;                           /* fil sortant (direction locale) */
        if (k < rv->len-1){ ax=rv->x[k];   ay=rv->y[k];   bx=rv->x[k+1]; by=rv->y[k+1]; }
        else if (k > 0)   { ax=rv->x[k-1]; ay=rv->y[k-1]; bx=rv->x[k];   by=rv->y[k];   }
        else              { ax=rv->x[k];   ay=rv->y[k];   bx=ax+1.f;     by=ay;          }
        out[n].x   = (float)rv->x[k] + 0.5f;
        out[n].y   = (float)rv->y[k] + 0.5f;
        out[n].ang = atan2f(by-ay, bx-ax);
        n++;
    }
    return n;
}

/* owner EFFECTIF d'une cellule (la lecture de la teinte politique) — -1 si mer,
 * terre vierge ou région non colonisée. Miroir de bseg_owner_of (viewer.c). */
static int border_owner_of(const ScpsSim *s, const Cell *c){
    if (!c || c->region<0 || c->region>=s->sim.econ->n_regions || c->region>=SCPS_MAX_REG) return -1;
    int ow = s->sim.econ->region[c->region].owner;
    return (ow>=0 && ow<s->w->n_countries && s->sim.econ->region[c->region].colonized) ? ow : -1;
}

int scps_border_segments(ScpsSim *s, int level, ScpsSeg *out, int max){
    if (!s || !s->ready || !out || max<=0 || level<0 || level>2) return 0;
    int n=0;
    /* balayage des arêtes EST & SUD : chaque joint interne vu UNE fois ; classé à son
     * niveau le plus FORT (pays > région > province) → on n'émet QUE le niveau demandé. */
    for (int y=0; y<SCPS_H && n<max; y++) for (int x=0; x<SCPS_W && n<max; x++){
        const Cell *a = scps_cellc(s->w, x, y);
        int own_a = border_owner_of(s, a);
        for (int d=0; d<2 && n<max; d++){               /* d : 0 = arête EST, 1 = arête SUD */
            int nx2=x+(d==0), ny2=y+(d==1);
            const Cell *b = (nx2<SCPS_W && ny2<SCPS_H) ? scps_cellc(s->w, nx2, ny2) : NULL;
            int own_b = b ? border_owner_of(s, b) : -1;
            int rga=a->region,   rgb=b?b->region:-1;
            int pva=a->province, pvb=b?b->province:-1;
            int lvl=-1;
            if (own_a!=own_b && (own_a>=0||own_b>=0)) lvl=2;        /* PAYS (ou contour externe) */
            else if (rga!=rgb && rga>=0 && rgb>=0)    lvl=1;        /* RÉGION */
            else if (pva!=pvb && pva>=0 && pvb>=0)    lvl=0;        /* PROVINCE */
            if (lvl!=level) continue;
            if (d==0){ out[n].x0=(float)(x+1); out[n].y0=(float)y;     out[n].x1=(float)(x+1); out[n].y1=(float)(y+1); }
            else     { out[n].x0=(float)x;     out[n].y0=(float)(y+1); out[n].x1=(float)(x+1); out[n].y1=(float)(y+1); }
            n++;
        }
        /* bords OUEST/NORD de la grille : le contour d'un PAYS s'y ferme aussi */
        if (level==2){
            if (x==0 && own_a>=0 && n<max){ out[n].x0=0;          out[n].y0=(float)y; out[n].x1=0;            out[n].y1=(float)(y+1); n++; }
            if (y==0 && own_a>=0 && n<max){ out[n].x0=(float)x;   out[n].y0=0;        out[n].x1=(float)(x+1); out[n].y1=0;            n++; }
        }
    }
    return n;
}

/* idem scps_border_segments mais TAGGÉ owner+other → outline par empire, hachures inter-empire,
 * et CÔTES (joint touchant la mer) NON émises au niveau pays (le rivage du shader suffit). */
int scps_border_segments_col(ScpsSim *s, int level, ScpsSegC *out, int max){
    if (!s || !s->ready || !out || max<=0 || level<0 || level>2) return 0;
    int n=0;
    for (int y=0; y<SCPS_H && n<max; y++) for (int x=0; x<SCPS_W && n<max; x++){
        const Cell *a = scps_cellc(s->w, x, y);
        int own_a = border_owner_of(s, a);
        int sea_a = (a->sea || a->lake);
        for (int d=0; d<2 && n<max; d++){
            int nx2=x+(d==0), ny2=y+(d==1);
            const Cell *b = (nx2<SCPS_W && ny2<SCPS_H) ? scps_cellc(s->w, nx2, ny2) : NULL;
            int own_b = b ? border_owner_of(s, b) : -1;
            int sea_b = b ? (b->sea || b->lake) : 1;     /* hors-grille = mer */
            int rga=a->region,   rgb=b?b->region:-1;
            int pva=a->province, pvb=b?b->province:-1;
            int lvl=-1, owner=-1, other=-1;
            if (own_a!=own_b && (own_a>=0||own_b>=0)){
                int land_own  = (own_a>=0?own_a:own_b);
                int land_other= (own_a>=0?own_b:own_a);
                int is_cs = (land_own>=0 && land_own<s->w->n_countries
                             && s->w->country[land_own].role==POLITY_CITY_STATE);
                /* CÔTE : invisible pour un EMPIRE (le rivage suffit) ; GARDÉE pour une CITÉ-ÉTAT (son
                 * contour or-argent doit se voir, et les marchés sont côtiers). */
                if ((sea_a || sea_b) && !is_cs) continue;
                lvl=2; owner=land_own; other=(sea_a||sea_b)?-2:land_other;
            }
            else if (rga!=rgb && rga>=0 && rgb>=0){    lvl=1; owner=own_a; }
            else if (pva!=pvb && pva>=0 && pvb>=0){    lvl=0; owner=own_a; }
            if (lvl!=level) continue;
            if (d==0){ out[n].x0=(float)(x+1); out[n].y0=(float)y;     out[n].x1=(float)(x+1); out[n].y1=(float)(y+1); }
            else     { out[n].x0=(float)x;     out[n].y0=(float)(y+1); out[n].x1=(float)(x+1); out[n].y1=(float)(y+1); }
            /* normale vers l'EXTÉRIEUR : owner côté A ⇒ l'extérieur est vers B (+) ; sinon vers A (−).
             * arête EST → normale en X ; arête SUD → normale en Y. (Sert au dégradé int.→ext.) */
            float sgn=(own_a>=0)?1.f:-1.f;
            out[n].nx=(d==0)?sgn:0.f; out[n].ny=(d==1)?sgn:0.f;
            out[n].owner=owner; out[n].other=other; n++;
        }
    }
    return n;
}

/* ============ ROUTES TERRAIN-AWARE → RÉSEAU À JONCTIONS (port de viewer.c) ========= *
 * A* sur la grille (coût relief+biome ; mer/lac contournés, fleuve = pont), reliant les
 * régions des routes commerciales TERRESTRES majeures. « Les routes attirent les routes »
 * : une cellule déjà routée (corridor lissé + marge) est fortement rabattue → les tracés
 * FUSIONNENT en jonctions Y/T/X au lieu de se croiser en désordre. Lissé (moyenne mobile).
 * Caché par signature du réseau (display-only, hors tick → déterminisme intact). */
#define API_ROAD_PATH_MAX 1400
typedef struct { int16_t x[API_ROAD_PATH_MAX], y[API_ROAD_PATH_MAX]; int len; int level; } ApiRoad;
static ApiRoad  *g_roads = NULL;
static int       g_nroads = 0;
static uint64_t  g_road_sig = 0;
static uint8_t  *g_roadmask = NULL;   /* 1 = cellule sur un corridor déjà tracé (attire) */
static float *g_ag = NULL, *g_aheapf = NULL;
static int   *g_afrom = NULL, *g_agen = NULL, *g_aclosed = NULL, *g_aheapi = NULL;
static int    g_acurgen = 0, g_aheap_n = 0;

static void aheap_push(float f, int idx){
    int i=g_aheap_n++; g_aheapf[i]=f; g_aheapi[i]=idx;
    while(i>0){ int p=(i-1)/2; if(g_aheapf[p]<=g_aheapf[i]) break;
        float tf=g_aheapf[p]; g_aheapf[p]=g_aheapf[i]; g_aheapf[i]=tf;
        int ti=g_aheapi[p]; g_aheapi[p]=g_aheapi[i]; g_aheapi[i]=ti; i=p; }
}
static int aheap_pop(void){
    int r=g_aheapi[0], n=--g_aheap_n; g_aheapf[0]=g_aheapf[n]; g_aheapi[0]=g_aheapi[n]; int i=0;
    for(;;){ int l=2*i+1, rr=2*i+2, sm=i;
        if(l<n && g_aheapf[l]<g_aheapf[sm]) sm=l;
        if(rr<n && g_aheapf[rr]<g_aheapf[sm]) sm=rr;
        if(sm==i) break;
        float tf=g_aheapf[sm]; g_aheapf[sm]=g_aheapf[i]; g_aheapf[i]=tf;
        int ti=g_aheapi[sm]; g_aheapi[sm]=g_aheapi[i]; g_aheapi[i]=ti; i=sm; }
    return r;
}
static float api_road_cost(const World *w, int x, int y){
    const Cell *c = scps_cellc(w, x, y);
    if (c->sea || c->lake) return -1.f;            /* mer/lac : on contourne */
    float cost = 1.f + c->height*7.f;              /* plat facile, on longe le BAS */
    if (c->biome==BIO_FOREST || c->biome==BIO_WOODS) cost += 2.5f;
    if (c->biome==BIO_JUNGLE)    cost += 5.f;
    if (c->biome==BIO_HILLS || c->biome==BIO_HIGHLANDS) cost += 5.f;
    if (c->biome==BIO_MOUNTAINS) cost += 16.f;
    if (c->biome==BIO_PEAK)      cost += 45.f;
    if (c->river>40)             cost += 7.f;      /* franchir un fleuve = un PONT */
    if (g_roadmask && g_roadmask[scps_idx(x,y)]) cost *= 0.30f;   /* les routes attirent les routes */
    return cost;
}
static bool api_road_astar(const World *w, int ax, int ay, int bx, int by, ApiRoad *out){
    if(ax<0||ay<0||ax>=SCPS_W||ay>=SCPS_H||bx<0||by<0||bx>=SCPS_W||by>=SCPS_H) return false;
    if(!g_ag){
        g_ag=(float*)malloc(sizeof(float)*SCPS_N); g_afrom=(int*)malloc(sizeof(int)*SCPS_N);
        g_agen=(int*)calloc(SCPS_N,sizeof(int)); g_aclosed=(int*)calloc(SCPS_N,sizeof(int));
        g_aheapf=(float*)malloc(sizeof(float)*SCPS_N); g_aheapi=(int*)malloc(sizeof(int)*SCPS_N);
        if(!g_ag||!g_afrom||!g_agen||!g_aclosed||!g_aheapf||!g_aheapi) return false;
    }
    int minx=(ax<bx?ax:bx)-48, maxx=(ax>bx?ax:bx)+48, miny=(ay<by?ay:by)-48, maxy=(ay>by?ay:by)+48;
    if(minx<0) minx=0;
    if(miny<0) miny=0;
    if(maxx>=SCPS_W) maxx=SCPS_W-1;
    if(maxy>=SCPS_H) maxy=SCPS_H-1;
    g_acurgen++; g_aheap_n=0;
    int s=scps_idx(ax,ay), goal=scps_idx(bx,by);
    g_ag[s]=0.f; g_afrom[s]=-1; g_agen[s]=g_acurgen;
    aheap_push(hypotf((float)(bx-ax),(float)(by-ay)), s);
    static const int dx8[8]={1,-1,0,0,1,1,-1,-1}, dy8[8]={0,0,1,-1,1,-1,1,-1};
    bool found=false; int guard=0;
    while(g_aheap_n>0 && guard++<900000){
        int cur=aheap_pop();
        if(g_aclosed[cur]==g_acurgen) continue;
        g_aclosed[cur]=g_acurgen;
        if(cur==goal){ found=true; break; }
        int cxx=cur%SCPS_W, cyy=cur/SCPS_W;
        for(int d=0;d<8;d++){
            int nx=cxx+dx8[d], ny=cyy+dy8[d];
            if(nx<minx||nx>maxx||ny<miny||ny>maxy) continue;
            int ni=scps_idx(nx,ny);
            if(g_aclosed[ni]==g_acurgen) continue;
            float cc=api_road_cost(w,nx,ny); if(cc<0.f) continue;
            float ng=g_ag[cur]+(d<4?1.f:1.41421f)*cc;
            if(g_agen[ni]==g_acurgen && g_ag[ni]<=ng) continue;
            g_ag[ni]=ng; g_afrom[ni]=cur; g_agen[ni]=g_acurgen;
            aheap_push(ng+hypotf((float)(bx-nx),(float)(by-ny)), ni);
        }
    }
    if(!found) return false;
    static int tmp[4096]; int tn=0;
    for(int cur=goal; cur!=-1 && tn<4096; cur=g_afrom[cur]) tmp[tn++]=cur;
    int stepd=(tn>API_ROAD_PATH_MAX)?(tn/API_ROAD_PATH_MAX+1):1;
    out->len=0;
    for(int k=tn-1;k>=0 && out->len<API_ROAD_PATH_MAX;k-=stepd){
        out->x[out->len]=(int16_t)(tmp[k]%SCPS_W); out->y[out->len]=(int16_t)(tmp[k]/SCPS_W); out->len++;
    }
    return out->len>=2;
}
static void api_road_smooth(ApiRoad *p){            /* moyenne mobile, extrémités fixes */
    if(p->len<3) return;
    static int16_t nx[API_ROAD_PATH_MAX], ny[API_ROAD_PATH_MAX];
    for(int pass=0; pass<3; pass++){
        nx[0]=p->x[0]; ny[0]=p->y[0]; nx[p->len-1]=p->x[p->len-1]; ny[p->len-1]=p->y[p->len-1];
        for(int k=1;k<p->len-1;k++){
            nx[k]=(int16_t)((p->x[k-1]+2*p->x[k]+p->x[k+1])/4);
            ny[k]=(int16_t)((p->y[k-1]+2*p->y[k]+p->y[k+1])/4);
        }
        memcpy(p->x,nx,sizeof(int16_t)*p->len); memcpy(p->y,ny,sizeof(int16_t)*p->len);
    }
}
/* cale un point sur la TERRE la plus proche (le centre d'une région côtière peut tomber
 * en mer → l'A* démarrerait dans l'eau et échouerait, la ville resterait sans route). */
static void api_snap_land(const World *w, int *px, int *py){
    int cx=*px, cy=*py;
    if(cx<0||cy<0||cx>=SCPS_W||cy>=SCPS_H) return;
    const Cell *c = scps_cellc(w,cx,cy);
    if(!c->sea && !c->lake) return;
    for(int R=1;R<=20;R++)
        for(int dy=-R;dy<=R;dy++) for(int dx=-R;dx<=R;dx++){
            if(abs(dx)!=R && abs(dy)!=R) continue;
            int nx=cx+dx, ny=cy+dy;
            if(nx<0||ny<0||nx>=SCPS_W||ny>=SCPS_H) continue;
            const Cell *nc=scps_cellc(w,nx,ny);
            if(!nc->sea && !nc->lake){ *px=nx; *py=ny; return; }
        }
}
static bool api_region_wpos(const World *w, int reg, float *wx, float *wy){
    if(reg<0||reg>=w->n_regions) return false;
    const Region *R=&w->region[reg];
    long ax=0,ay=0; int n=0;
    for(int k=0;k<R->n_provinces && k<12;k++){
        int pid=R->province_ids[k];
        if(pid<0||pid>=w->n_provinces) continue;
        ax+=w->province[pid].seed_x; ay+=w->province[pid].seed_y; n++;
    }
    if(n==0) return false;
    *wx=(float)ax/n; *wy=(float)ay/n; return true;
}
/* stampe le corridor LISSÉ (+ marge 1) dans le masque → bassin d'attraction pour les
 * routes suivantes (elles s'y rabattent : jonctions). */
static void api_road_stamp(const ApiRoad *p){
    if(!g_roadmask) return;
    for(int k=0;k<p->len;k++){ int rx=p->x[k], ry=p->y[k];
        for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
            int nx=rx+dx, ny=ry+dy;
            if(nx>=0&&ny>=0&&nx<SCPS_W&&ny<SCPS_H) g_roadmask[ny*SCPS_W+nx]=1; } }
}
/* tente A* entre deux villes ; succès → range la route (niveau ∝ tier max), lisse, stampe
 * le corridor (attraction), marque les deux villes connectées. */
static bool api_road_link(const World *w, const float *cx, const float *cy, const int *ctier,
                          int ia, int ib, int *connected){
    if(g_nroads>=SCPS_MAX_ROUTES) return false;
    ApiRoad *rd=&g_roads[g_nroads];
    if(!api_road_astar(w,(int)cx[ia],(int)cy[ia],(int)cx[ib],(int)cy[ib],rd)) return false;
    int t = ctier[ia]>ctier[ib]?ctier[ia]:ctier[ib];
    rd->level = (t>=4)?0 : (t>=2?1:2);
    api_road_smooth(rd);
    api_road_stamp(rd);
    connected[ia]=1; connected[ib]=1;
    g_nroads++;
    return true;
}
/* RÉSEAU DE VILLES : chaque ville colonisée est reliée à ses ~2 plus proches voisines par
 * une route A* — un réseau LOGIQUE où AUCUNE ville n'est isolée (garantie : toute ville
 * joignable par terre obtient ≥1 route). Les arêtes courtes d'abord → les longues se
 * rabattent dessus (jonctions Y/T/X via le masque d'attraction). */
static void api_roads_build(ScpsSim *s){
    const World *w = s->w;
    if(!s->sim.econ){ g_nroads=0; return; }
    int nreg = s->sim.econ->n_regions;
    /* signature : photo des villes (région colonisée + propriétaire) — change à colo/conquête */
    uint64_t sig=1469598103934665603ull;
    for(int r=0;r<nreg && r<SCPS_MAX_REG;r++){
        const RegionEconomy *re=&s->sim.econ->region[r];
        if(re->colonized && re->owner>=0) sig=(sig^(uint64_t)(r*100003+(re->owner+1)))*1099511628211ull;
    }
    if(sig==g_road_sig && g_roads) return;          /* villes inchangées → cache valide */
    g_road_sig=sig;
    if(!g_roads){ g_roads=(ApiRoad*)malloc(sizeof(ApiRoad)*SCPS_MAX_ROUTES); if(!g_roads){g_nroads=0;return;} }
    if(!g_roadmask){ g_roadmask=(uint8_t*)calloc(SCPS_N,1); }
    if(g_roadmask) memset(g_roadmask,0,SCPS_N);
    g_nroads=0;

    /* villes = régions colonisées (centre monde + tier de pop) */
    static float cx[SCPS_MAX_REG], cy[SCPS_MAX_REG];
    static int   creg[SCPS_MAX_REG], ctier[SCPS_MAX_REG], connected[SCPS_MAX_REG];
    int nc=0;
    for(int r=0;r<nreg && r<SCPS_MAX_REG && nc<SCPS_MAX_REG;r++){
        const RegionEconomy *re=&s->sim.econ->region[r];
        if(!re->colonized || re->owner<0) continue;
        float wx,wy; if(!api_region_wpos(w,r,&wx,&wy)) continue;
        int ix=(int)wx, iy=(int)wy; api_snap_land(w,&ix,&iy);   /* assise sur TERRE → A* démarre */
        long pop=region_pop_i(s,r);
        cx[nc]=(float)ix; cy[nc]=(float)iy; creg[nc]=r;
        ctier[nc]= pop>=4000?5:pop>=1500?4:pop>=500?3:pop>=150?2:pop>=50?1:0;
        connected[nc]=0; nc++;
    }
    (void)creg;
    if(nc<2) return;

    /* arêtes candidates : les 2 plus proches voisines de chaque ville (dédup (min,max)). */
    typedef struct { int a,b; float d; } Edge;
    static Edge edges[SCPS_MAX_REG*2]; int ne=0;
    for(int i=0;i<nc;i++){
        int n1=-1,n2=-1; float d1=1e18f,d2=1e18f;
        for(int j=0;j<nc;j++){ if(j==i) continue;
            float dd=(cx[i]-cx[j])*(cx[i]-cx[j])+(cy[i]-cy[j])*(cy[i]-cy[j]);
            if(dd<d1){ d2=d1;n2=n1; d1=dd;n1=j; } else if(dd<d2){ d2=dd;n2=j; } }
        if(n1>=0 && ne<SCPS_MAX_REG*2){ int a=i<n1?i:n1,b=i<n1?n1:i; edges[ne].a=a;edges[ne].b=b;edges[ne].d=d1; ne++; }
        if(n2>=0 && ne<SCPS_MAX_REG*2){ int a=i<n2?i:n2,b=i<n2?n2:i; edges[ne].a=a;edges[ne].b=b;edges[ne].d=d2; ne++; }
    }
    for(int e1=0;e1<ne;e1++) for(int e2=e1+1;e2<ne;e2++)        /* dédup (ne petit) */
        if(edges[e2].a==edges[e1].a && edges[e2].b==edges[e1].b){ edges[e2]=edges[--ne]; e2--; }
    for(int a=1;a<ne;a++){ Edge v=edges[a]; int b=a-1;          /* tri ↑ distance → courtes d'abord */
        while(b>=0 && edges[b].d>v.d){ edges[b+1]=edges[b]; b--; } edges[b+1]=v; }

    for(int e=0;e<ne && g_nroads<SCPS_MAX_ROUTES;e++)
        api_road_link(w,cx,cy,ctier,edges[e].a,edges[e].b,connected);

    /* GARANTIE « chaque ville a une route » : toute ville encore sans route tente ses
     * voisines de plus en plus loin jusqu'à un A* qui passe (île isolée d'1 ville : rien). */
    static int order[SCPS_MAX_REG];
    for(int i=0;i<nc && g_nroads<SCPS_MAX_ROUTES;i++){
        if(connected[i]) continue;
        int m=0; for(int j=0;j<nc;j++) if(j!=i) order[m++]=j;
        for(int a=1;a<m;a++){
            int v=order[a];
            float vd=(cx[i]-cx[v])*(cx[i]-cx[v])+(cy[i]-cy[v])*(cy[i]-cy[v]); int b=a-1;
            while(b>=0){
                int u=order[b];
                float ud=(cx[i]-cx[u])*(cx[i]-cx[u])+(cy[i]-cy[u])*(cy[i]-cy[u]);
                if(ud<=vd) break;
                order[b+1]=order[b]; b--;
            }
            order[b+1]=v;
        }
        for(int a=0;a<m && !connected[i] && g_nroads<SCPS_MAX_ROUTES;a++)
            api_road_link(w,cx,cy,ctier,i,order[a],connected);
    }
}
int scps_roads_build(ScpsSim *s){
    if(!s || !s->ready) return 0;
    api_roads_build(s);
    return g_nroads;
}
int scps_road_path(ScpsSim *s, int i, ScpsRoadPt *out, int max, int *level){
    if(!s || !s->ready || !out || max<=0 || i<0 || i>=g_nroads) return 0;
    const ApiRoad *p=&g_roads[i];
    int n=p->len; if(n>max) n=max;
    for(int k=0;k<n;k++){ out[k].x=(float)p->x[k]+0.5f; out[k].y=(float)p->y[k]+0.5f; }   /* centre cellule */
    if(level) *level=p->level;
    return n;
}

/* ====================================================================== */
/* CRÉATEUR DE CULTURE — listes, validation, aperçu, composition (voir .h)  */
/* La membrane : des MOTS (noms, axes, épithètes) et des SIGNES. Pur (aucun  */
/* sim) → utilisable AVANT scps_sim_generate.                               */
/* ====================================================================== */

int scps_heritage_list(ScpsHeritage *out, int max){
    static char ex[HERITAGE_COUNT][32];   /* ethnonymes-exemples (persistent le temps que l'hôte copie) */
    int n=0;
    for(int h=0; h<HERITAGE_COUNT && n<max; h++){
        culture_make_name(ex[h], (int)sizeof ex[h], (Heritage)h, 1u);
        out[n].id      = h;
        out[n].nom     = heritage_name((Heritage)h);
        out[n].sphere  = sphere_name(heritage_sphere((Heritage)h));
        out[n].exemple = ex[h];
        n++;
    }
    return n;
}

int scps_ethos_list(ScpsEthosDef *out, int max){
    /* épithètes = celles de country_make_name (DOMINATEUR..PACIFISTE) ; lignes courtes. */
    static const char *EPI[ETHOS_COUNT]  = { "Horde","Clans","Ordre","Couronne","Ligue","Havre" };
    static const char *HINT[ETHOS_COUNT] = {
        "Conquête : pousse la coercition, mauvais intégrateur.",
        "Gloire & razzia : honneur martial, digère mal.",
        "Hiérarchie & discipline : l'État qui tient l'ordre.",
        "Bâtisseur d'institutions : tient la diversité.",
        "Profit & carrefours : prospère par le commerce.",
        "Consentement seul : ne fracture jamais, pacifique.",
    };
    int n=0;
    for(int e=0; e<ETHOS_COUNT && n<max; e++){
        out[n].id       = e;
        out[n].nom      = ethos_name((Ethos)e);
        out[n].epithete = EPI[e];
        out[n].hint     = HINT[e];
        n++;
    }
    return n;
}

int scps_tradition_list(ScpsTradition *out, int max){
    int n=0;
    for(int t=0; t<TRAIT_COUNT && n<max; t++){
        const TraitDef *d = trait_def((TraitId)t);
        if(!d) continue;
        out[n].id      = t;
        out[n].nom     = d->name;
        out[n].axe     = (int)d->cat;
        out[n].axe_nom = category_name(d->cat);
        out[n].rang    = d->pts;
        out[n].hover   = d->hover;
        n++;
    }
    return n;
}

/* construit un build depuis 3 ids (le slot = l'axe, comme build_is_valid l'exige). */
static HeritageBuild api_build_of(int t0, int t1, int t2){
    HeritageBuild b;
    b.trait[0]=(TraitId)t0; b.trait[1]=(TraitId)t1; b.trait[2]=(TraitId)t2;
    return b;
}

int scps_culture_validate(int t0, int t1, int t2){
    if(t0<0||t0>=TRAIT_COUNT||t1<0||t1>=TRAIT_COUNT||t2<0||t2>=TRAIT_COUNT) return 0;
    HeritageBuild b = api_build_of(t0,t1,t2);
    return build_is_valid(&b) ? 1 : 0;
}

int scps_culture_preview(int t0, int t1, int t2, ScpsLevierLine *out, int max){
    if(!out || max<=0) return 0;
    if(t0<0||t0>=TRAIT_COUNT||t1<0||t1>=TRAIT_COUNT||t2<0||t2>=TRAIT_COUNT) return 0;
    HeritageBuild b = api_build_of(t0,t1,t2);
    HeritageLeviers L = build_leviers(&b);
    /* les 9 leviers du moteur, dans l'ordre de la struct → un MOT par axe touché. */
    float v[9] = { L.demographie, L.productivite, L.influence, L.coercition,
                   L.capacite, L.permeabilite, L.arcane, L.derive, L.fracture };
    static const char *NM[9] = {
        "Démographie", "Productivité", "Influence", "Coercition (militaire)",
        "Capacité (diversité)", "Perméabilité (assimilation)", "Affinité arcane",
        "Dérive culturelle", "Fracture" };
    /* RELATIFS (1+x → affichés en %) : demographie(0), productivite(1), derive(7).
     * Les autres sont ABSOLUS (additifs échelle 0..10). Cf. HeritageLeviers (scps_heritage.h). */
    int n=0;
    for(int i=0;i<9 && n<max;i++){
        if(v[i] > 0.0001f || v[i] < -0.0001f){
            out[n].nom    = NM[i];
            out[n].signe  = (v[i] > 0.f) ? +1 : -1;
            out[n].value  = v[i];
            out[n].is_pct = (i==0 || i==1 || i==7) ? 1 : 0;
            n++;
        }
    }
    return n;
}

const char *scps_culture_name(int heritage, uint32_t seed){
    static char buf[40];
    int h = (heritage>=0 && heritage<HERITAGE_COUNT) ? heritage : HERITAGE_ADAPTATIF;
    culture_make_name(buf, (int)sizeof buf, (Heritage)h, seed);
    return buf;
}

int scps_set_empire_culture(int slot, int heritage, int ethos, int t0, int t1, int t2){
    if(slot<0 || slot>=CULTURE_SLOTS) return 0;
    if(!scps_culture_validate(t0,t1,t2)) return 0;
    if(heritage<0||heritage>=HERITAGE_COUNT) return 0;
    if(ethos<0||ethos>=ETHOS_COUNT) return 0;
    HeritageBuild b = api_build_of(t0,t1,t2);
    culture_slot_set(slot, (Heritage)heritage, ethos, b);
    return 1;
}

int scps_set_player_culture(int heritage, int ethos, int t0, int t1, int t2){
    return scps_set_empire_culture(0, heritage, ethos, t0, t1, t2);
}

void scps_clear_player_culture(void){ culture_player_clear(); }

/* ====================================================================== */
/* RELIGION (façade) — voir scps_api.h                                      */
/* ====================================================================== */
static int api_capital_cell(ScpsSim *s, int cid){
    int cp=s->w->country[cid].capital_prov, centre=0;
    if(cp>=0 && cp<s->w->n_provinces){
        int sx=s->w->province[cp].seed_x, sy=s->w->province[cp].seed_y;
        if(sx>=0 && sy>=0) centre=sy*SCPS_W+sx;
    }
    return centre;
}
static int api_count_empires(ScpsSim *s){
    /* Le plafond religion s'ancre au compte d'empires de GENÈSE (religion_empire_ref, posé par
     * sim_init) — stable face aux sécessions. Repli : compte courant si le ref n'est pas semé. */
    int ref = religion_empire_ref();
    if (ref > 0) return ref;
    int n=0;
    for(int c=0;c<s->w->n_countries;c++){ int rl=s->w->country[c].role;
        if(rl==POLITY_PLAYER||rl==POLITY_ANTAGONIST) n++; }
    return n;
}
int scps_religion_cap(ScpsSim *s){ return s ? religion_cap(api_count_empires(s)) : 1; }
int scps_religion_can_found(ScpsSim *s){
    return (s && s->ready && religion_can_found(api_count_empires(s))) ? 1 : 0;
}
int scps_religion_found(ScpsSim *s, int cid, int credo, int t0, int t1, int t2){
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return -1;
    if(!religion_picks_valid(t0,t1,t2)) return -1;
    /* PLAFOND ⌈n_emp/3⌉ sur les RACINES : au-delà, le joueur RALLIE une foi existante (les empires
     * se PARTAGENT les religions) au lieu d'en fonder une nouvelle. */
    if(!religion_can_found(api_count_empires(s))){
        int rid=religion_adopt_existing(cid, (uint32_t)(cid+1));
        if(rid>=0) religion_inherit_regions(s->w, cid);
        return rid;
    }
    int trad[3]={t0,t1,t2};
    int rid=religion_spawn(credo, trad, api_capital_cell(s,cid), cid, NULL);
    if(rid<0) return -1;
    religion_set_country(cid, rid);
    religion_inherit_regions(s->w, cid);
    return rid;
}
int scps_religion_eligible(ScpsSim *s, int cid){
    if(!s || !s->ready) return 0;
    /* PLAFOND PAR RACINE : pas de schisme si la foi a déjà ses RELIG_SCHISM_MAX sectes (bouton grisé) —
     * la foi en exil PERSISTE alors au lieu d'essaimer une secte de plus. */
    if(!religion_can_schism(religion_of_country(cid))) return 0;
    return (int)religion_schism_eligible(s->w, cid);
}
int scps_religion_schism(ScpsSim *s, int cid, int slot_a, int pole_a, int slot_b, int pole_b,
                         int new_credo, int *out_flipped){
    if(out_flipped) *out_flipped=0;
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return -1;
    if(!religion_can_schism(religion_of_country(cid))) return -1;   /* plafond schismes/racine atteint */
    int parent=religion_of_country(cid);
    if(parent<0) return -1;
    int child=religion_schism(parent, slot_a, pole_a, slot_b, pole_b, new_credo,
                              api_capital_cell(s,cid), cid, 0, 0);
    if(child<0) return -1;
    int fl=religion_fracture(s->w, s->sim.econ, s->sim.wl, cid, child);
    if(out_flipped) *out_flipped=fl;
    return child;
}
int scps_religion_of_country(ScpsSim *s, int cid){ (void)s; return religion_of_country(cid); }
int scps_religion_of_region (ScpsSim *s, int region){ (void)s; return religion_of_region(region); }
int scps_religion_recruit_scholar(ScpsSim *s, int cid, int region){
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return -1;
    return religion_scholar_recruit(cid, region);
}
int scps_religion_scholar_role(ScpsSim *s, int cid){ (void)s; return religion_scholar_role(cid); }

int scps_religion_pole_list(ScpsReligPole *out, int max){
    int n=0;
    for(int p=0;p<RP_COUNT && n<max;p++){
        const ReligPoleDef *d=&RELIG_POLES[p];
        out[n].id=p; out[n].nom=relig_pole_name((ReligPole)p);
        out[n].axe=(int)d->axis; out[n].axe_nom=relig_axis_name(d->axis);
        out[n].tip=relig_pole_tip((ReligPole)p); n++;
    }
    return n;
}
int scps_credo_list(ScpsCredoDef *out, int max){
    int n=0;
    for(int c=0;c<CREDO_COUNT && n<max;c++){ out[n].id=c; out[n].nom=credo_name((Credo)c); n++; }
    return n;
}
int scps_religion_picks_valid(int p0, int p1, int p2){ return religion_picks_valid(p0,p1,p2); }
int scps_religion_founding_ready(ScpsSim *s, int cid){
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return 0;
    if(religion_of_country(cid) >= 0) return 0;   /* a déjà une foi */
    uint32_t mask = (1u<<EDI_SANCTUAIRE)|(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE)|(1u<<EDI_MONASTERE);
    for(int r=0;r<s->sim.econ->n_regions;r++)
        if(s->sim.econ->region[r].owner==cid && (s->sim.econ->region[r].edi_built & mask)) return 1;
    return 0;
}
const char *scps_religion_name(ScpsSim *s, int cid){
    static char buf[96];
    (void)s;
    int rid=religion_of_country(cid);
    if(rid<0 || rid>=g_religion_count){ buf[0]='\0'; return buf; }
    const Religion *r=&g_religions[rid];
    snprintf(buf,sizeof buf,"%s \xc2\xb7 %s/%s/%s", credo_name((Credo)r->credo),
             relig_pole_name((ReligPole)r->traditions[0]),
             relig_pole_name((ReligPole)r->traditions[1]),
             relig_pole_name((ReligPole)r->traditions[2]));
    return buf;
}

/* ====================================================================== */
/* PARAMÈTRES DE GÉNÉRATION (sliders de nouvelle partie)                    */
/* ====================================================================== */
void scps_worldparams_default(uint32_t seed, ScpsWorldParams *out){
    if(!out) return;
    WorldParams d = worldparams_default(seed);
    out->n_empires     = d.n_empires;
    out->n_city_states = d.n_city_states;
    out->n_continents  = d.n_continents;
    out->world_age     = d.world_age;
    out->land_amount   = d.land_amount;
    out->mountains     = d.mountains;
    out->erosion       = d.erosion;
    out->temperature   = d.temperature;
    out->humidity      = d.humidity;
}

static int   clampi_api(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }
static float clamp01_api(float v){ return v<0.f?0.f:(v>1.f?1.f:v); }

void scps_worldgen_set(const ScpsWorldParams *p){
    if(!p){ g_wg.active=false; return; }
    g_wg.active = true;
    g_wg.p.n_empires     = clampi_api(p->n_empires,     1, 60);
    g_wg.p.n_city_states = clampi_api(p->n_city_states, 0, 120);
    g_wg.p.n_continents  = clampi_api(p->n_continents,  1, 8);
    g_wg.p.world_age     = clamp01_api(p->world_age);
    g_wg.p.land_amount   = clamp01_api(p->land_amount);
    g_wg.p.mountains     = clamp01_api(p->mountains);
    g_wg.p.erosion       = clamp01_api(p->erosion);
    g_wg.p.temperature   = clamp01_api(p->temperature);
    g_wg.p.humidity      = clamp01_api(p->humidity);
}

void scps_worldgen_clear(void){ g_wg.active = false; }

/* ====================================================================== */
/* SAUVEGARDE (l'écran « Charger ») — réutilise le format PARTAGÉ scps_save */
/* ====================================================================== */
int scps_sim_save(ScpsSim *s, int slot){
    if(!s || !s->ready) return 0;
    /* identité de culture = le slot 0 (joueur) ; la section CULT persiste TOUS les slots. */
    return scps_save_game(slot, s->w, &s->sim, &s->params,
                          (int)culture_player_heritage(), culture_player_ethos()) ? 1 : 0;
}
int scps_sim_load(ScpsSim *s, int slot){
    if(!s) return 1;
    int r=0, e=0;
    int rc = scps_load_game(slot, s->w, &s->sim, &s->params, &r, &e);
    if(rc!=0) return rc;
    /* MAIN HUMAINE : comme à la genèse (le load restaure ai_on du save, mais on garantit
     * que le joueur reste débrayé de l'IA côté façade). */
    s->sim.human_player = s->sim.player;
    s->sim.ai_on[s->sim.player] = false;
    warhost_set_human(s->sim.player);
    econ_flux_reset();
    s->ready = true;
    api_centroids(s);   /* la carte chargée : recalcule les centroïdes */
    return 0;
}
void scps_save_slots(ScpsSaveSlot *out, int max){
    if(!out) return;
    for(int i=0;i<max;i++){
        out[i].used=0; out[i].year=0; out[i].line[0]='\0';
        SaveHeader h;
        if(scps_save_slot_info(i+1, &h)){
            out[i].used = 1;
            out[i].year = (int)h.year;
            snprintf(out[i].line, sizeof out[i].line, "%s", h.line);
        }
    }
}
