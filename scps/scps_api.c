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
#include "scps_factions.h"  /* le SPECTRE de factions (sidebar : parts/griefs/coup/corruption) */
#include "scps_lang.h"      /* tr() : noms de conseillers (StrId) → mot */
#include "scps_provlog.h"   /* journal d'évènements provincial */
#include "scps_heritage.h"   /* CRÉATEUR DE CULTURE : héritages, traditions, override joueur */
#include "scps_culture.h"   /* ethos_name, enum Ethos */
#include "scps_world.h"     /* culture_make_name (ethnonyme façon Stellaris) */
#include "scps_save.h"      /* SAUVEGARDE partagée : scps_save_game/load/slot_info */
#include "scps_religion.h"  /* religion_reset (nouvelle partie) */
#include "scps_fog.h"       /* BROUILLARD DE GUERRE : fog_visible_regions (le voile — étape 1/2) */
#include "scps_math.h"      /* clampf (LOT J : scps_manumit_preview) */
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
    float      *ppx, *ppy; /* centroïdes PROVINCE (siège de ville : la province rep, pas la région) */
    int         n_pcent;
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
    free(s->w); free(s->px); free(s->cx); free(s->cy); free(s->ppx); free(s->ppy); free(s);
}

/* centroïdes région + PROVINCE (la géo est figée par worldgen/chargement ; seul l'OWNER
 * changera). Les centroïdes de province servent au SIÈGE de ville (scps_region_seat) :
 * le bourg vit dans la province REPRÉSENTATIVE de sa région, pas au barycentre de la
 * région entière (qui peut tomber à son bord, voire hors d'elle sur une forme concave). */
static void api_centroids(ScpsSim *s){
    int nr = s->sim.econ->n_regions; s->n_cent = nr;
    int np = s->w->n_provinces;      s->n_pcent = np;
    s->cx  = (float*)realloc(s->cx,  (size_t)nr*sizeof(float));
    s->cy  = (float*)realloc(s->cy,  (size_t)nr*sizeof(float));
    s->ppx = (float*)realloc(s->ppx, (size_t)np*sizeof(float));
    s->ppy = (float*)realloc(s->ppy, (size_t)np*sizeof(float));
    double *ax = (double*)calloc((size_t)nr, sizeof(double));
    double *ay = (double*)calloc((size_t)nr, sizeof(double));
    long   *cn = (long*)  calloc((size_t)nr, sizeof(long));
    double *pax = (double*)calloc((size_t)np, sizeof(double));
    double *pay = (double*)calloc((size_t)np, sizeof(double));
    long   *pcn = (long*)  calloc((size_t)np, sizeof(long));
    if(s->cx && s->cy && s->ppx && s->ppy && ax && ay && cn && pax && pay && pcn){
        for(int y=0; y<SCPS_H; y++) for(int x=0; x<SCPS_W; x++){
            const Cell *c = scps_cellc(s->w, x, y);
            int r = c->region;
            if(r>=0 && r<nr){ ax[r]+=x; ay[r]+=y; cn[r]++; }
            int p = c->province;
            if(p>=0 && p<np){ pax[p]+=x; pay[p]+=y; pcn[p]++; }
        }
        for(int r=0; r<nr; r++){
            if(cn[r]){ s->cx[r]=(float)(ax[r]/(double)cn[r]); s->cy[r]=(float)(ay[r]/(double)cn[r]); }
            else     { s->cx[r]=-1.f; s->cy[r]=-1.f; }
        }
        for(int p=0; p<np; p++){
            if(pcn[p]){ s->ppx[p]=(float)(pax[p]/(double)pcn[p]); s->ppy[p]=(float)(pay[p]/(double)pcn[p]); }
            else      { s->ppx[p]=-1.f; s->ppy[p]=-1.f; }
        }
    }
    free(ax); free(ay); free(cn); free(pax); free(pay); free(pcn);
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
    econ_set_human(s->sim.player);     /* §NF : la construction autonome SKIPPE le joueur (il construit via le panneau B) */
    campaign_set_human(s->sim.player); /* #32 : n'isoler QUE les morts de guerre du joueur (endgame_blood_ratio) */
    feed_set_focus(s->sim.player);     /* le fil d'évènements ne garde que ce qui LE concerne */
    econ_flux_reset();   /* budget façade : repart d'une ardoise propre (le flux est un état GLOBAL ;
                          * econ_init, déjà appelé par sim_init, a RAZ g_tax_lastyear — pas de capture ici) */
    s->ready = true;
    api_centroids(s);   /* centroïdes région (géo figée par worldgen) */
}

/* Le TICK PLEIN : exactement le sim_day de la chronique (déterministe). */
void scps_sim_advance_days(ScpsSim *s, int ndays){
    if(!s || !s->ready) return;
    for(int i=0; i<ndays; i++){
        if(s->sim.day % 365 == 0) econ_flux_year_capture();   /* budget façade : le flux ET le revenu annuel (d_treasury_mois) */
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
            case SCPS_LAYER_CLIFF:  v = world_cliff_intensity(s->w, x, y); break;   /* falaises (dérivé) */
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
int  scps_day_of_year  (const ScpsSim *s){ int d = s ? s->sim.day : 0; return ((d % 365) + 365) % 365; }
int  scps_country_known(const ScpsSim *s, int c){
    if(!s || !s->ready || c<0 || c>=s->w->n_countries) return 0;
    if(s->sim.human_player < 0) return 1;          /* pas de joueur : rien n'est voilé */
    if(c == s->sim.human_player) return 1;
    return country_knows(s->sim.human_player, c) ? 1 : 0;
}
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
/* SIÈGE DE VILLE d'une région : le centroïde de sa PROVINCE REPRÉSENTATIVE (celle qui
 * porte le bourg — econ_region_rep_province, sérialisée depuis la genèse), PAS celui de
 * la région entière (retour joueur : « les villes ne sont pas centrées sur leurs
 * provinces » — sur une région concave le barycentre tombe au bord). Repli : centroïde
 * région. Coordonnées MONDE (cellules) ; false si vide. */
bool scps_region_seat(const ScpsSim *s, int r, float *x, float *y){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return false;
    int rp = econ_region_rep_province(s->sim.econ, r);
    if(rp>=0 && rp<s->n_pcent && s->ppx[rp]>=0.f){
        if(x) *x = s->ppx[rp];
        if(y) *y = s->ppy[rp];
        return true;
    }
    return scps_region_centroid(s, r, x, y);
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

/* CONTOUR d'une PROVINCE (miroir du contour de région, comparaison cell.province) —
 * la SURBRILLANCE DE SÉLECTION du front (le grain de panneau, charte EU4). */
int scps_province_border_segments(ScpsSim *s, int prov, ScpsSegC *out, int max){
    if(!s || !s->ready || !out || max<=0 || prov<0) return 0;
    int n=0;
    for (int y=0; y<SCPS_H && n<max; y++) for (int x=0; x<SCPS_W && n<max; x++){
        const Cell *a=scps_cellc(s->w,x,y);
        if (a->province!=prov) continue;
        const Cell *e=(x+1<SCPS_W)?scps_cellc(s->w,x+1,y):NULL;
        if ((!e||e->province!=prov) && n<max){ out[n].x0=(float)(x+1);out[n].y0=(float)y;out[n].x1=(float)(x+1);out[n].y1=(float)(y+1);out[n].nx=1;out[n].ny=0;out[n].owner=prov;out[n].other=-1;n++; }
        const Cell *we=(x>0)?scps_cellc(s->w,x-1,y):NULL;
        if ((!we||we->province!=prov) && n<max){ out[n].x0=(float)x;out[n].y0=(float)y;out[n].x1=(float)x;out[n].y1=(float)(y+1);out[n].nx=-1;out[n].ny=0;out[n].owner=prov;out[n].other=-1;n++; }
        const Cell *so=(y+1<SCPS_H)?scps_cellc(s->w,x,y+1):NULL;
        if ((!so||so->province!=prov) && n<max){ out[n].x0=(float)x;out[n].y0=(float)(y+1);out[n].x1=(float)(x+1);out[n].y1=(float)(y+1);out[n].nx=0;out[n].ny=1;out[n].owner=prov;out[n].other=-1;n++; }
        const Cell *no=(y>0)?scps_cellc(s->w,x,y-1):NULL;
        if ((!no||no->province!=prov) && n<max){ out[n].x0=(float)x;out[n].y0=(float)y;out[n].x1=(float)(x+1);out[n].y1=(float)y;out[n].nx=0;out[n].ny=-1;out[n].owner=prov;out[n].other=-1;n++; }
    }
    return n;
}

/* LAVIS POLITIQUE — l'owner effectif par cellule (même lecture que les frontières :
 * border_owner_of = pays VALIDE d'une région colonisée), -1 = mer/terre vierge. */
static int border_owner_of(const ScpsSim *s, const Cell *c);   /* défini plus bas (frontières) */
void scps_map_owner(ScpsSim *s, int16_t *out){
    if (!out) return;
    if (!s || !s->ready){ for (int i=0;i<SCPS_W*SCPS_H;i++) out[i]=-1; return; }
    for (int y=0;y<SCPS_H;y++) for (int x=0;x<SCPS_W;x++){
        const Cell *c = scps_cellc(s->w, x, y);
        out[y*SCPS_W+x] = (int16_t)((c->sea||c->lake) ? -1 : border_owner_of(s, c));
    }
}

/* BROUILLARD DE GUERRE (étape 1/2, VISUEL SEULEMENT — cf. scps_fog.h) : le masque par
 * région est calculé UNE fois (fog_visible_regions, motif map_state_tint) puis recopié
 * par cellule via c->region (motif scps_map_owner). Une cellule SANS région (mer, lac,
 * basse-terre non revendiquée) part VOILÉE (fail-closed) — sinon on « voit à travers »
 * la mer/les lacs (plainte joueur). Une DILATATION bornée révèle ensuite l'eau qui
 * TOUCHE le territoire découvert (lacs enclavés, frange côtière du joueur) tout en
 * gardant l'océan ouvert et les côtes étrangères sous le voile — on n'étale JAMAIS
 * dans une terre étrangère (r>=0 voilée reste voilée). human_player<0 (chronique/
 * viewer/vitrine du menu sans joueur engagé) : aucun voile, toute la carte est nue. */
#define FOG_SEA_HALO 8   /* cellules d'eau/basse-terre révélées autour du connu (dialable) */
void scps_fog_visible(ScpsSim *s, uint8_t *out){
    if (!out) return;
    if (!s || !s->ready || s->sim.human_player < 0){
        memset(out, 1, (size_t)SCPS_W*SCPS_H); return;   /* pas de joueur : rien à cacher */
    }
    uint8_t rv[SCPS_MAX_REG];
    fog_visible_regions(s->w, s->sim.econ, s->sim.human_player,
                        ages_fog_radius_add(s->sim.ev), rv);
    const int W = SCPS_W, H = SCPS_H;
    /* base : visible ⇔ sa région est vue ; eau/sans-région VOILÉE. */
    for (int y=0;y<H;y++) for (int x=0;x<W;x++){
        int r = scps_cellc(s->w, x, y)->region;
        out[y*W+x] = (r>=0 && r<SCPS_MAX_REG) ? rv[r] : 0;
    }
    /* dilatation multi-source (statique, motif fog_visible_regions : pas de malloc
     * répété) — n'étale QUE dans les cellules sans région (r<0). */
    static int32_t q[SCPS_W*SCPS_H];
    static uint8_t dist[SCPS_W*SCPS_H];
    int qh=0, qt=0;
    memset(dist, 0, (size_t)W*H);
    for (int i=0;i<W*H;i++) if (out[i]) q[qt++]=i;   /* sources = tout le visible (dist 0) */
    while (qh<qt){
        int i=q[qh++]; int d=dist[i];
        if (d>=FOG_SEA_HALO) continue;
        int x=i%W, y=i/W;
        for (int dy=-1;dy<=1;dy++) for (int dx=-1;dx<=1;dx++){
            if (!dx && !dy) continue;
            int nx=x+dx, ny=y+dy;
            if (nx<0||nx>=W||ny<0||ny>=H) continue;       /* pas de wrap (KISS : la couture est rare) */
            int j=ny*W+nx;
            if (out[j]) continue;                          /* déjà visible */
            if (scps_cellc(s->w,nx,ny)->region>=0) continue;/* terre étrangère : jamais révélée par dilatation */
            out[j]=1; dist[j]=(uint8_t)(d+1); q[qt++]=j;
        }
    }
}
/* ⚠ `out` doit pointer ≥ scps_region_count(s) octets — PAS SCPS_MAX_REG, ce
 * plafond interne n'est jamais exposé à l'hôte (motif region_owner/region_tier :
 * l'hôte dimensionne SES tampons sur scps_region_count, jamais sur un cap moteur). */
void scps_fog_region_mask(ScpsSim *s, uint8_t *out){
    if (!out) return;
    if (!s || !s->ready) return;   /* rien à écrire : scps_region_count(s) vaut déjà 0 */
    uint8_t rv[SCPS_MAX_REG];
    fog_visible_regions(s->w, s->sim.econ, s->sim.human_player,
                        ages_fog_radius_add(s->sim.ev), rv);
    int n = s->sim.econ->n_regions; if (n<0) n=0; if (n>SCPS_MAX_REG) n=SCPS_MAX_REG;
    memcpy(out, rv, (size_t)n);
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

/* REVENU DE RECHERCHE (hover Savoir) — décompose ai_research_income comme le moteur :
 *   income/jour = (savoir_pop / 365) × yield(institutions) × income_W × age_mult × (1+métab).
 * Chaque facteur est un NOMBRE TANGIBLE lu de l'état courant, jamais recalculé à part. */
void scps_country_research_income(ScpsSim *s, int cid, ScpsResearchIncome *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return;
    const TechState *ts = &s->sim.ts[cid];
    double pop   = econ_country_savoir(s->sim.econ, cid);       /* annuel, la POP produit */
    double yield = tech_research_yield(ts);                     /* institutions & bibliothèques */
    double agem  = s->sim.wp ? s->sim.wp->age_research_mult : 1.0; /* LUMIÈRE de l'âge (transitoire) */
    double metabf = econ_country_metabolized(s->w, s->sim.econ, cid);
    double income_w = tune_f("AI_RESEARCH_INCOME_W", AI_RESEARCH_INCOME_W);
    double metab_w  = tune_f("AI_METAB_RES_W", AI_METAB_RES_W);
    /* la base POP par jour, le poids global (tune) DÉJÀ plié dedans (jamais surfacé) :
     * le joueur voit « Pops +N », puis les modificateurs ×/+ qui l'amènent au total. */
    out->pop_daily  = (pop / 365.0) * income_w;
    out->yield_mult = yield;
    out->age_mult   = agem;
    out->metab_pct  = (int)(metab_w * metabf * 100.0 + 0.5);
    out->per_day    = out->pop_daily * yield * agem * (1.0 + metab_w * metabf);
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

/* W-GUERRE UI (lot A) — HACHURES de siège/occupation : cf. header. */
int scps_region_war_state(ScpsSim *s, int r, int *belligerent_out){
    if(belligerent_out) *belligerent_out = -1;
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return 0;
    int owner = s->sim.econ->region[r].owner;
    /* OCCUPÉE domine : le siège a déjà abouti (occupier tient militairement). */
    int occ = s->sim.dp->occupier[r];
    if(occ>=0 && occ!=owner){ if(belligerent_out) *belligerent_out = occ; return 2; }
    /* ASSIÉGÉE : une armée de campagne D'UN AUTRE pays s'y tient en phase FA_SIEGE. */
    for(int k=0;k<s->w->n_countries && k<SCPS_MAX_COUNTRY;k++){
        const FieldArmy *a = &s->sim.camp->army[k];
        if(!a->active || a->phase!=FA_SIEGE || a->loc!=r) continue;
        if(owner>=0 && k==owner) continue;   /* on ne s'assiège pas soi-même */
        if(belligerent_out) *belligerent_out = k;
        return 1;
    }
    return 0;
}

/* W-GUERRE UI (lot B) — LE PANNEAU DE COMBAT : cf. header. */
void scps_battle_info(ScpsSim *s, int r, ScpsBattleInfo *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    out->region=-1; out->attacker=-1; out->defender=-1; out->phase="";
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return;
    int owner = s->sim.econ->region[r].owner;

    /* 1) une FieldBattle ACTIVE sur cette région ? (chocs/accalmies en cours) */
    const FieldBattle *bt = NULL;
    for(int k=0;k<CAMPAIGN_MAX_BATTLES;k++){
        const FieldBattle *b = &s->sim.camp->battle[k];
        if(b->active && b->loc==r){ bt = b; break; }
    }
    if(bt){
        out->valid=1; out->region=r;
        /* a = celui qui a marché jusqu'ici (l'attaquant présumé si ≠ owner du sol) */
        int A=bt->a, B=bt->b;
        if(owner>=0 && B==owner){ out->attacker=A; out->defender=B; }
        else if(owner>=0 && A==owner){ out->attacker=B; out->defender=A; }
        else { out->attacker=A; out->defender=B; }
        out->phase_id=(int)FA_BATTLE; out->phase=sz(campaign_phase_name(FA_BATTLE));
        out->in_battle=1;
        out->loss_atk = (out->attacker==A)? bt->lossA : bt->lossB;
        out->loss_def = (out->defender==B)? bt->lossB : bt->lossA;
    } else {
        /* 2) sinon, un SIÈGE en cours (une armée ennemie FA_SIEGE ici) */
        int besieger=-1;
        for(int k=0;k<s->w->n_countries && k<SCPS_MAX_COUNTRY;k++){
            const FieldArmy *a=&s->sim.camp->army[k];
            if(a->active && a->phase==FA_SIEGE && a->loc==r && !(owner>=0 && k==owner)){ besieger=k; break; }
        }
        if(besieger<0) return;   /* rien à montrer : out reste invalide */
        out->valid=1; out->region=r;
        out->attacker=besieger; out->defender=owner;
        out->phase_id=(int)FA_SIEGE; out->phase=sz(campaign_phase_name(FA_SIEGE));
    }

    if(out->attacker>=0 && out->attacker<SCPS_MAX_COUNTRY){
        ArmyComposition c = campaign_composition(s->sim.camp, out->attacker);
        out->atk_units=c.total; out->atk_inf=c.infanterie; out->atk_arch=c.archers;
        out->atk_cav=c.cavalerie; out->atk_mages=c.mages;
    }
    if(out->defender>=0 && out->defender<SCPS_MAX_COUNTRY){
        ArmyComposition c = campaign_composition(s->sim.camp, out->defender);
        out->def_units=c.total; out->def_inf=c.infanterie; out->def_arch=c.archers;
        out->def_cav=c.cavalerie; out->def_mages=c.mages;
    }
    if(out->attacker>=0 && out->defender>=0)
        out->war_score = diplo_war_score(s->sim.dp, out->attacker, out->defender);
}

/* LOT T (2026-07-07) — UNIFIÉ sur capitale_max_tier (scps_labor.c : T2 2000 · T3 3000 ·
 * T4 4000 · T5 5000…), au lieu d'un barème ad hoc DISTINCT (4000/1500/500/150/50) qui
 * divergeait silencieusement du reste du moteur (readout, T-gate ai.c). Décalé -1 pour
 * garder le contrat d'affichage 0-5 existant (Godot : ce n'est qu'un GATE d'affichage +
 * un multiplicateur de taille grossier — le VRAI étalement de sprites t1..t7 vient de
 * `_pop_tier()` côté GDScript, indépendant, cf. overlay.gd). Grain RÉGION conservé (lecteur
 * historique façade/viewer) — la province, elle, est gatée à son propre grain ailleurs
 * (scps_province_capitale, scps_manuf_legal via host_province_tier côté ai.c). */
int scps_region_tier(const ScpsSim *s, int r){
    if(!s || !s->ready || r<0 || r>=s->sim.econ->n_regions) return -1;
    if(!s->sim.econ->region[r].colonized) return -1;
    long pop = region_pop_i(s, r);
    int tier = capitale_max_tier(pop) - 1;
    if (tier > 5) tier = 5;
    /* la capitale domine : plancher modeste (EMPIRE_SEED=4000 aligne déjà nativement sur
     * T4→tier 3 ; le plancher couvre les captures/dérives post-guerre où la pop retombe). */
    for(int c=0;c<s->w->n_countries;c++){
        int cp = s->w->country[c].capital_prov;
        if(cp>=0 && cp<s->w->n_provinces && s->w->province[cp].region==r){
            if(tier<3) tier=3;
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
    out->fin_raw       = (s->sim.eg) ? (int)s->sim.eg->fin : 0;   /* brut, SANG compris (5) */
    /* #32 — bande de sang : « le monde a saigné, et TOI ? ». Nombres tangibles,
     * jamais de flottant moteur qui franchit la membrane. */
    if (s->sim.eg) {
        double ratio = endgame_blood_ratio(s->sim.eg, s->sim.econ);
        float frac = tune_f("ENDGAME_BLOOD_FRAC", 0.20f);
        int bp = (frac > 0.f) ? (int)(100.0 * ratio / (double)frac + 0.5) : 0;
        out->blood_pct = (bp < 0) ? 0 : (bp > 100 ? 100 : bp);
        double share = endgame_blood_player_share(s->sim.eg);
        int bpp = (int)(100.0 * share + 0.5);
        out->blood_player_pct = (bpp < 0) ? 0 : (bpp > 100 ? 100 : bpp);
    }
}

int scps_region_sunken(const ScpsSim *s, int r){
    if(!s || !s->ready || !s->sim.eg || r<0 || r>=SCPS_MAX_REG) return 0;
    return (int)s->sim.eg->sunken[r];
}

/* LOT V3 — LAVIS PAR VARIANTE : pur wrapper, aucune mutation. */
float scps_endgame_region_intensity(const ScpsSim *s, int region){
    if(!s || !s->ready || !s->sim.eg || !s->w || !s->sim.econ) return 0.f;
    return endgame_region_intensity(s->sim.eg, s->w, s->sim.econ, region);
}

/* CARTE DE VARIANTE : intensité précalculée PAR RÉGION (≤ SCPS_MAX_REG, cher pour
 * RONCES qui scanne toutes les cellules — donc UNE fois, jamais par cellule) puis
 * recopiée par cellule via c->region (motif scps_map_owner). Repli tout-0 si pas
 * de fin (fin_raw==FIN_AUCUNE ou moteur pas prêt) — coût nul dans le cas commun. */
void scps_map_endgame_variant(ScpsSim *s, uint8_t *dst){
    if(!dst) return;
    int w = SCPS_W, h = SCPS_H;
    memset(dst, 0, (size_t)w*h);
    if(!s || !s->ready || !s->sim.eg || !s->w || !s->sim.econ) return;
    if(s->sim.eg->fin == FIN_AUCUNE) return;
    int n_regions = s->sim.econ->n_regions;
    if(n_regions <= 0 || n_regions > SCPS_MAX_REG) n_regions = SCPS_MAX_REG;
    static float inten[SCPS_MAX_REG];
    for(int r=0; r<n_regions; r++)
        inten[r] = endgame_region_intensity(s->sim.eg, s->w, s->sim.econ, r);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        const Cell *c = scps_cellc(s->w, x, y);
        int r = c->region;
        float v = (r>=0 && r<n_regions) ? inten[r] : 0.f;
        if(v < 0.f) v = 0.f;
        if(v > 1.f) v = 1.f;
        dst[y*w+x] = (uint8_t)(v * 255.f + 0.5f);
    }
}

/* ---- DÉTAIL DE PROVINCE (port fidèle de viewer.c) --------------------- */

int scps_province_groups(ScpsSim *s, int pid, ScpsGroup *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    int reg = s->w->province[pid].region;
    if(reg<0 || reg>=s->sim.econ->n_regions) return 0;
    RegionEconomy *re = &s->sim.econ->region[reg];
    ProvinceEconomy *pe = &s->sim.econ->prov[pid];
    if(pe->pop.n_groups<=0) return 0;
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
    int ng = province_composition(&pe->pop, s->sim.drift, crown, 5.f, 5.f, gr, SCPS_MAX_GROUPS);
    if(ng>max) ng=max;
    for(int i=0;i<ng;i++){
        out[i].heritage     = sz(gr[i].heritage);
        out[i].culture  = sz(gr[i].culture);
        out[i].lineage  = sz(gr[i].lineage);
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
    /* v50 — au grain PROVINCE (l'UI montrait l'agrégat de toute la RÉGION) */
    IncomeReadout inc = province_income_prov(s->sim.econ, pid);
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

/* les ÉDIFICES de BASE bâtis (grenier, marché, temple, remparts…) — le masque
 * edi_built de la province (retour joueur 2026-07-09 : « on ne voit pas les
 * bâtiments de base » — province_buildings ne rend que les MANUFACTURES). */
int scps_province_edifices(ScpsSim *s, int pid, ScpsProvBld *out, int max){
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 0;
    if(pid>=s->sim.econ->n_prov) return 0;
    uint32_t m = s->sim.econ->prov[pid].edi_built;
    int n=0;
    for(int e=0; e<EDIFICE_COUNT && e<32 && n<max; e++){
        if(m & (1u<<e)){
            out[n].nom = sz(edifice_name(e));
            out[n].niveau = e;    /* ⚠ porte le TYPE Edifice (pour la vignette côté hôte) */
            out[n].ouvriers = 0;
            n++;
        }
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

/* SATISFACTION par CLASSE au grain PROVINCE (0-100 ; -1 si la classe est VIDE) — les
 * barres du panneau (retour joueur 2026-07-09 : « satisfaction » par pop + la strate
 * servile visible). ADDITIF (province_classes garde sa signature figée). Lecture
 * directe de strata[c].satisfaction — la vérité province, tenue par econ_tick. */
void scps_province_class_sat(ScpsSim *s, int pid, int *lab, int *bourg, int *elite, int *slave){
    int v[4] = {-1,-1,-1,-1};
    if(s && s->ready && pid>=0 && pid<s->w->n_provinces && pid<s->sim.econ->n_prov){
        const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
        const int cls[4] = { CLASS_LABORER, CLASS_BOURGEOIS, CLASS_ELITE, CLASS_SLAVE };
        for(int i=0;i<4;i++){
            const PopStratum *st = &pe->strata[cls[i]];
            if(st->pop >= 1.f) v[i] = (int)(clampf(st->satisfaction, 0.f, 1.f) * 100.f + 0.5f);
        }
    }
    if(lab)   *lab   = v[0];
    if(bourg) *bourg = v[1];
    if(elite) *elite = v[2];
    if(slave) *slave = v[3];
}

/* UI PROVINCE LOT 1 — le 4e segment (ESCLAVES) : Σ groupes CLASS_SLAVE de la
 * province. ADDITIF (scps_province_classes garde sa signature 3-classes figée —
 * cf. piège documenté dans scps_api.h). */
long scps_province_slave_count(ScpsSim *s, int pid){
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces || pid>=s->sim.econ->n_prov) return 0;
    const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
    const ProvincePop *pp = &pe->pop;
    long souls = 0;
    for(int i=0;i<pp->n_groups;i++) if(pp->groups[i].klass==CLASS_SLAVE) souls += pp->groups[i].count;
    return souls;
}

/* UI PROVINCE LOT 3 — IMPÔTS DE LA PROVINCE, en or/AN. Rejoue EXACTEMENT la
 * formule de collecte fiscale d'econ_tick (scps_econ.c ~L2390-2406, §6-7) sur les
 * strates DE LA PROVINCE — PUR read, aucune écriture (contrairement au tick réel
 * qui débite st->wealth et crédite re->treasury). STATE_TAX_AMBITION est un
 * #define LOCAL à scps_econ.c (pas dans le .h) : mirorré ici à sa valeur actuelle
 * (0.42f) — si econ_tick change ce taux, ce reader DOIT suivre (les deux portent
 * le même commentaire de couplage). */
double scps_province_tax(ScpsSim *s, int pid){
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces || pid>=s->sim.econ->n_prov) return 0.0;
    const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
    if(!pe->colonized) return 0.0;
    const float STATE_TAX_AMBITION_MIRROR = 0.42f;   /* cf. scps_econ.c:1624 (le taux visé, non exposé au .h) */
    double coll_tot = 0.0;
    for(int c=0;c<CLASS_COUNT;c++){
        const PopStratum *st = &pe->strata[c];
        float sat   = clampf(st->satisfaction, 0.f, 1.f);
        float seuil = econ_tax_tolerance(pe->culture.ethos, (SocialClass)c) * (0.40f + 0.60f*sat);
        float evasion   = clampf(STATE_TAX_AMBITION_MIRROR - seuil, 0.f, 1.f);
        /* Miroir de scps_econ.c:2621 : collected = 0.42·wealth·(1−évasion)·dt où dt est en
         * ANNÉES par tick (cf. « ×365×dt » ligne 2652) — le taux est donc déjà ANNUEL.
         * (L'ancien miroir le prenait pour un mensuel et ×12 ⇒ affichait 12× le réel —
         * les « 6300 or/an » d'un hameau de 750 âmes.) Borné à la richesse de la classe. */
        float collected_yearly = STATE_TAX_AMBITION_MIRROR * st->wealth * (1.f - evasion);
        if(collected_yearly > st->wealth) collected_yearly = st->wealth;
        coll_tot += (double)collected_yearly;
    }
    return coll_tot;   /* or/an */
}

/* UI PROVINCE LOT 4 — le TERRAIN comme % de tenue de siège. Réplique
 * terrain_defense_mult(Biome,float) — scps_army.c:528-540 (fichier EN COURS
 * D'ÉDITION PARALLÈLE, cf. scps_api.h : coefficients COPIÉS ici, pas partagés,
 * pour ne dépendre d'aucun symbole en flux). 100 = neutre (plaine, alt. 0) ;
 * >100 = le siège dure d'autant plus longtemps (multiplicateur de DURÉE — pas un
 * bonus de combat, cf. terrain_combat_bonus, DISTINCT et non répliqué ici). */
static float ui_terrain_defense_mult(Biome b, float height){
    float base;
    switch (b){
        case BIO_MOUNTAINS:                              base = 1.8f; break;
        case BIO_HILLS:  case BIO_HIGHLANDS:             base = 1.4f; break;
        case BIO_FOREST: case BIO_WOODS: case BIO_JUNGLE:base = 1.3f; break;
        case BIO_MARSH:  case BIO_BOG: case BIO_MANGROVE:base = 1.3f; break;
        case BIO_DESERT: case BIO_COASTAL_DESERT:        base = 1.1f; break;
        default:                                         base = 1.0f; break;
    }
    float h = height < 0.f ? 0.f : (height > 1.f ? 1.f : height);
    return base * (1.f + 0.5f * h);   /* RELIEF_DEFENSE = 0.5f, scps_army.c */
}
int scps_province_defense_pct(ScpsSim *s, int pid){
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces) return 100;
    const Province *p = &s->w->province[pid];
    float mult = ui_terrain_defense_mult(p->biome_dominant, p->height_avg);
    return (int)(mult * 100.f + 0.5f);
}

/* UI PROVINCE LOT 5 — SEED déterministe par province (hash figé worldgen :
 * seed_x/seed_y, jamais aléatoire ; miroir de compose_arms(cid) côté pays, qui
 * hash cid — ici on hash la position de germe de la province). -1 hors-borne. */
int scps_province_seed(const ScpsSim *s, int pid){
    if(!s || !s->ready || pid<0 || pid>=s->w->n_provinces) return -1;
    const Province *p = &s->w->province[pid];
    uint32_t h = (uint32_t)((int)p->seed_x * 92821 + (int)p->seed_y * 68917 + pid * 2654435761u);
    return (int)(h & 0x7fffffffu);
}

/* UI PROVINCE LOT 6 — le marché LOCAL : prix/stock/bande des biens les plus
 * significatifs de la province (même tri que province_income : brute d'abord,
 * puis manufacturé), + le mot de PORT (EDI_PORT bâti ⇒ "Port", sinon ""). Dérivé
 * pur (ProvinceEconomy.price/stock, band_marche — même primitive que
 * scps_country_stocks, au grain PROVINCE au lieu du pays agrégé). */
int scps_province_market(ScpsSim *s, int pid, ScpsMarketLine *out, int max, const char **port_out){
    if(port_out) *port_out = "";
    if(!out || max<=0 || !s || !s->ready || pid<0 || pid>=s->w->n_provinces || pid>=s->sim.econ->n_prov) return 0;
    const ProvinceEconomy *pe = &s->sim.econ->prov[pid];
    if(port_out) *port_out = (pe->edi_built & (1u<<EDI_PORT)) ? "Port" : "";
    /* trie : d'abord la ressource brute principale de la province (info.ressource),
     * puis jusqu'à 2 lignes de plus (les biens VIVANTS — stock ou flux non nuls),
     * même filtre que scps_country_stocks. */
    int n=0;
    int g = (int)s->w->province[pid].resource;
    for(int pass=0; pass<2 && n<max; pass++){
        for(int gg=(pass==0?g:1); gg<RES_COUNT && n<max; gg++){
            if(pass==0 && gg!=g) break;                 /* pass 0 : uniquement la ressource dominante */
            if(pass==1 && gg==g) continue;               /* pass 1 : le reste, pas de doublon */
            float stk = pe->stock[gg], dem = pe->demand[gg], sup = pe->supply[gg];
            if(!(stk>0.5f || dem>0.05f || sup>0.05f)) continue;
            out[n].name   = sz(resource_name((Resource)gg));
            /* MEMBRANE : un déficit transitoire (ProvinceEconomy.stock<0, non clampé
             * côté moteur — cf. TROUVAILLES 2026-07-08) reste un signal VALIDE pour la
             * bande (band_marche lit le stk BRUT : très négatif ⇒ PENURIE, cohérent) mais
             * ne doit JAMAIS être publié tel quel — un stock/prix négatif est un nombre
             * tangible FAUX pour le joueur. On clampe seulement à la SORTIE. */
            out[n].price  = pe->price[gg] > 0.f ? pe->price[gg] : 0.f;
            out[n].stock  = stk > 0.f ? stk : 0.f;
            out[n].marche = sz(label_marche(band_marche(dem, sup+stk)));
            n++;
        }
        if(pass==0 && g==RES_NONE) continue;   /* pas de brute dominante : direct pass 1 */
    }
    return n;
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
    /* MERGÉ : logement = tier_housing + manuf + grenier (miroir eff_cap moteur) ;
     * services = tier_housing + institutions. UN SEUL pool chacun.
     * E4 (2026-07-05) — logement_cap DÉLÈGUE désormais à econ_prov_effcap() (scps_econ.h) au
     * lieu de recalculer à la main : l'ancien recalcul codait tier_h/manuf_h en dur SANS le
     * bonus CONFORT (poterie/statuaire → COMFORT_HOUSE_RELIEF, cf. ECON_EFFCAP_BODY) — le
     * joueur voyait une capacité FAUSSE (trop basse) dès que le confort était actif. Même
     * grain (province, pe) que le helper — aucune sommation nécessaire. tier_h reste utile
     * pour service_cap (qui n'a pas d'équivalent moteur unifié). */
    float tier_h = (float)capitale_housing(tier, admin);
    out->logement_cap = (long)econ_prov_effcap(pe);
    out->service_cap  = (long)(tier_h + (pe->build.K_inst + pe->build.savoir + pe->build.faith) * 700.f);
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

/* §5 PUISSANCE COMMERCIALE — la membrane du menu marché : le pool mensuel + le restant + les
 * sources (pop marchande, chaîne commerciale). Nombres tangibles, aucun flottant moteur. */
void scps_commerce_power(ScpsSim *s, int me, ScpsCommerce *out){
    if(!out) return;
    memset(out, 0, sizeof *out);
    if(!s || !s->ready || me<0 || me>=s->w->n_countries) return;
    out->pool      = intertrade_commerce_pool(me);
    out->remaining = intertrade_commerce_remaining(me);
    double bourg=0.0, elite=0.0, infra=0.0;
    for(int r=0;r<s->sim.econ->n_regions;r++){
        const RegionEconomy *re=&s->sim.econ->region[r];
        if(re->owner!=me) continue;
        bourg += re->strata[CLASS_BOURGEOIS].pop;
        elite += re->strata[CLASS_ELITE].pop;
        infra += re->build.PE_infra;
    }
    out->bourgeois=(float)bourg; out->elite=(float)elite;
    float pct=(float)(infra*tune_f("COMMERCE_BLD_PER",COMMERCE_BLD_PER)), mx=tune_f("COMMERCE_BLD_MAX",COMMERCE_BLD_MAX);
    if(pct>mx) pct=mx;
    if(pct<0.f) pct=0.f;
    out->bonus_pct=(int)(pct*100.f+0.5f);
}

/* ── CADRES DU CONSEIL — IDENTITÉ DE MINISTRE (2026-07-10, spec docs/CONSEIL_
 * ORIENTATIONS_2026-07-10.md § « Noms, maisons, identités ») ────────────────────────
 * Un ministre (candidat OU assis) porte une IDENTITÉ DÉTERMINISTE dérivée des MÊMES
 * clés que son nom/tier/âge/faction (seed,cid,seat,slot,gen — statecraft_council_cand_*,
 * scps_statecraft.c:105-131) : même candidat ⇒ même identité, RIEN À SÉRIALISER (dérivée,
 * comme tier/âge/faction le sont déjà — recalculée au vol). L'algorithme de hash MIROIRE
 * sc_hash (scps_statecraft.c:88-92, `static` donc non exportée — réimplémenté ici à
 * l'identique) avec un salt DISTINCT (0x1DE47170) des salts déjà pris par ce module (tier
 * 0xC0FFEE · nom 0x5EAB011 · âge 0xA6E11 · faction 0xFAC7104 · retraite 0x0DDA6E) — identité
 * ⊥ faction ⊥ siège ⊥ rang (tirages indépendants, comme la spec l'exige des maisons).
 *
 * ⚠ IDENTITÉS = PUREMENT NARRATIVES, EFFET MÉCANIQUE 0 (décision joueur, spec : « ne pas
 * afficher de faux modificateurs », pas de boucle d'équilibrage relancée). AUCUN
 * coefficient, AUCUN site de lecture moteur — le mot + la flavor sont du CHROME que l'UI
 * affiche tel quel. Golden intact par construction (scps_api.c n'est pas dans chronicle ;
 * rien ne mord le tick). `portrait_id` 0..7 = index stable dans UIKit.advisor_portrait. */
#define CONS_ID_N 8
typedef struct { const char *nom; int portrait_id; const char *flavor; } ConsIdentity;
static const ConsIdentity CONS_IDENTITES[CONS_ID_N] = {   /* les 8 de la spec, flavors VERBATIM */
    { "Rigoriste",   0, "Chaque exception lui paraît être la première pierre d'une ruine." },
    { "Courtisan",   1, "Il sait qui doit être salué, qui doit être payé et qui doit croire que les deux gestes se valent." },
    { "Austère",     2, "Son train de maison tient dans deux coffres. Sa reconnaissance aussi." },
    { "Réformateur", 3, "Aucune institution ne lui semble achevée tant qu'il reste possible de la démonter." },
    { "Vétéran",     4, "Il a servi trois règnes et appris à ne confondre aucun d'eux avec l'État." },
    { "Ambitieux",   5, "Il appelle service la distance qui le sépare encore du pouvoir." },
    { "Loyaliste",   6, "Il sert la couronne avec assez de ferveur pour inquiéter celui qui la porte." },
    { "Vénal",       7, "Il connaît le prix de chaque secret, sauf celui du dernier." },
};
static uint32_t cons_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d){
    uint32_t h = a*2654435761u ^ b*40503u ^ c*2246822519u ^ d*3266489917u;
    h ^= h>>16; h *= 2246822519u; h ^= h>>13; h *= 3266489917u; h ^= h>>16;
    return h;
}
static int cons_identity_index(uint32_t seed, int cid, int seat, int slot, int gen){
    uint32_t genseed = seed ^ ((uint32_t)(gen>0?gen:0) * 0x9E3779B9u);   /* miroir sc_genseed, scps_statecraft.c:99 */
    uint32_t h = cons_hash(genseed ^ 0x1DE47170u, (uint32_t)cid, (uint32_t)(seat*7+slot), 0x9E37u);
    return (int)(h % (uint32_t)CONS_ID_N);
}

/* ── CARTE CONSEIL — bonus/coût/retraite (2026-07-10, § « Rangs et coûts »,
 * « Efficacité politique », « Interface (cartes) ») ──────────────────────────
 * Les lecteurs statecraft_council_seat_mult/_efficiency/_cand_cost sont déjà
 * exportés et lus TELS QUELS pour un siège POURVU (aucune réimplémentation :
 * l'état réel — loyauté vécue, K du pays — existe déjà). Pour un CANDIDAT
 * (pas encore embauché), deux formules sont mirées à l'identique depuis leurs
 * `static` d'origine (même discipline que cons_hash ci-dessus) car elles
 * calculent une PRÉVISION que rien d'exporté ne donne :
 *  - cons_predicted_loyalty MIROIRE la loyauté de DÉPART que statecraft_
 *    council_hire assignerait RÉELLEMENT (scps_statecraft.c:225-226, salt
 *    0x10AD17Bu) — la prévision EST donc exacte, pas une estimation.
 *  - cons_rank_mult/cons_tier_revenue_rate MIRORENT sc_seat_base/sc_tier_mult/
 *    sc_tier_revenue_rate (scps_statecraft.c:87-116, `static`) : simples
 *    lectures de tune_f sur les clés EXACTES de la spec — aucune valeur inventée.
 *  - cons_efficiency_calc MIROIRE le corps de statecraft_council_efficiency
 *    (scps_statecraft.c:326-336) pour l'appliquer à la loyauté PRÉVUE plutôt
 *    qu'à un siège réellement pourvu (l'exportée renverrait 1.0, vacant). */
static float cons_predicted_loyalty(uint32_t seed, int cid, int seat, int slot, int gen){
    uint32_t h = cons_hash(seed^0x10AD17Bu, (uint32_t)cid, (uint32_t)seat, (uint32_t)(slot*13+gen));
    return 45.f + (float)(h % 21u);
}
static float cons_seat_base(int seat){
    switch (seat){
        case 0:  return tune_f("COUNCIL_SAVOIR_BASE",   0.12f);
        case 1:  return tune_f("COUNCIL_ROYAUME_BASE",  0.15f);
        default: return tune_f("COUNCIL_OUVRAGES_BASE", 0.20f);
    }
}
static float cons_tier_mult_of(int tier){
    switch (tier){
        case 1:  return 1.f;
        case 2:  return tune_f("COUNCIL_TIER2_MULT", 1.50f);
        case 3:  return tune_f("COUNCIL_TIER3_MULT", 2.00f);
        default: return 0.f;
    }
}
static float cons_rank_mult(int seat, int tier){ return 1.f + cons_seat_base(seat)*cons_tier_mult_of(tier); }
static float cons_tier_revenue_rate(int tier){
    switch (tier){
        case 1:  return tune_f("COUNCIL_TIER1_REVENUE_RATE", 0.015f);
        case 2:  return tune_f("COUNCIL_TIER2_REVENUE_RATE", 0.030f);
        case 3:  return tune_f("COUNCIL_TIER3_REVENUE_RATE", 0.050f);
        default: return 0.f;
    }
}
static float cons_efficiency_calc(float K, float loy, float corr){
    float eff = tune_f("COUNCIL_EFF_BASE", 0.70f)
              + tune_f("COUNCIL_EFF_K_PER", 0.03f) * K
              + tune_f("COUNCIL_EFF_LOY_W", 0.15f) * (loy/100.f)
              - tune_f("COUNCIL_EFF_CORRUPTION_PER_POINT", 0.0035f) * corr;
    return clampf(eff, tune_f("COUNCIL_EFF_MIN",0.50f), tune_f("COUNCIL_EFF_MAX",1.15f));
}
static float cons_pct100(float x){ return x*100.f; }   /* fraction → pourcentage (décimales gardées, l'UI arrondit à l'affichage) */
/* Mission décennale — le bonus du siège responsable (miroir de mission_reward_mult,
 * scps_missions.c:120-131, `static` — même formule, aucune valeur inventée). */
static float cons_mission_reward_mult(const Statecraft *sc, const WorldProsperity *wp,
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

int scps_country_council(ScpsSim *s, int me, ScpsCouncilSeat *out, int max){
    if(!out || max<=0 || !s || !s->ready || me<0 || me>=s->w->n_countries) return 0;
    uint32_t seed = s->w->seed;
    float ipm = econ_world_ipm(s->sim.econ);
    const WorldProsperity *wp = s->sim.wp;
    float K = (wp && me<wp->n_countries) ? wp->country[me].K : 0.f;
    int corr = faction_corruption_0_100(me);
    int n=0;
    for(int seat=0; seat<SC_COUNCIL_SEATS && n<max; seat++){
        out[n].seat   = sz(tr((StrId)(STR_COUNCIL_SEAT_0+seat)));
        out[n].domain = sz(tr((StrId)(STR_COUNCIL_EFF_0+seat)));
        out[n].k_admin = K; out[n].corruption_pct = corr;
        int slot = statecraft_council_seated(s->sim.sc, me, seat);
        if(slot>=0){
            int sgen = statecraft_council_seated_gen(s->sim.sc, me, seat);   /* identité ÉPINGLÉE à l'embauche */
            int loy  = statecraft_council_loyalty(s->sim.sc, me, seat);
            int tier = statecraft_council_cand_tier(seed, me, seat, slot, sgen);
            out[n].filled    = 1;
            out[n].councilor = sz(tr((StrId)statecraft_council_cand_name(seed, me, seat, slot, sgen)));
            out[n].tier      = tier;
            out[n].age       = statecraft_council_seated_age(s->sim.sc, seed, me, seat, s->sim.year);
            out[n].faction   = sz(faction_name((int)statecraft_council_faction(seed, me, seat, slot, sgen)));
            out[n].loyalty   = loy;
            out[n].pay       = statecraft_council_pay(s->sim.sc, me, seat);
            out[n].mood      = sz(council_mood_word(loy));
            out[n].firstname = sz(statecraft_council_cand_firstname(seed, me, seat, slot, sgen));
            out[n].house     = sz(statecraft_council_cand_house(seed, me, seat, slot, sgen));
            /* CARTE — bonus de rang (lecteur réel) / efficacité (lecteur réel, loyauté vécue)
             * / bonus final = rang×efficacité (statecraft_council_apply, même composition). */
            float rank_mult = statecraft_council_seat_mult(s->sim.sc, seed, me, seat);
            float eff        = statecraft_council_efficiency(s->sim.sc, wp, me, seat);
            out[n].rank_bonus_pct  = cons_pct100(rank_mult - 1.f);
            out[n].efficiency_pct  = cons_pct100(eff);
            out[n].final_bonus_pct = cons_pct100((rank_mult-1.f)*eff);
            out[n].cost_rate_pct   = cons_tier_revenue_rate(tier) * 100.f;
            out[n].cost_year       = (double)statecraft_council_cand_cost(seed, me, seat, slot, sgen, ipm) * 12.0;
            int rlo = 66 - out[n].age, rhi = 73 - out[n].age;
            out[n].retire_lo = (rlo>0)?rlo:0; out[n].retire_hi = (rhi>0)?rhi:0;
            { int ci = cons_identity_index(seed, me, seat, slot, sgen);
              out[n].identite    = CONS_IDENTITES[ci].nom;
              out[n].portrait_id = CONS_IDENTITES[ci].portrait_id;
              out[n].id_flavor   = CONS_IDENTITES[ci].flavor; }
        } else {
            out[n].filled = 0; out[n].councilor = ""; out[n].tier = 0; out[n].age = 0;
            out[n].faction = ""; out[n].loyalty = 0; out[n].pay = 1.f; out[n].mood = "";
            out[n].identite = ""; out[n].portrait_id = -1; out[n].id_flavor = "";
            out[n].firstname = ""; out[n].house = "";
            out[n].rank_bonus_pct = 0; out[n].efficiency_pct = 0; out[n].final_bonus_pct = 0;
            out[n].cost_rate_pct = 0.f; out[n].cost_year = 0.0;
            out[n].retire_lo = -1; out[n].retire_hi = -1;
        }
        n++;
    }
    return n;
}

/* CANDIDATS du siège (pool de la génération COURANTE — toujours pleine) : l'embauche
 * ÉCLAIRÉE — nom · tier · ÂGE (grandit avec l'année) · coût/mois (×IPM) · la CARTE
 * complète (personne+maison, bonus de rang, efficacité PRÉVUE, bonus final, coût
 * annuel, retraite estimée) — § « Interface (cartes) ». */
int scps_council_candidates(ScpsSim *s, int seat, ScpsCouncilCand *out, int max){
    if(!out || max<=0 || !s || !s->ready || seat<0 || seat>=SC_COUNCIL_SEATS) return 0;
    int me = s->sim.player;
    if (me<0 || me>=s->w->n_countries) return 0;
    uint32_t seed = s->w->seed;
    int gen = statecraft_council_gen(s->sim.year);
    float ipm = econ_world_ipm(s->sim.econ);
    const WorldProsperity *wp = s->sim.wp;
    float K = (wp && me<wp->n_countries) ? wp->country[me].K : 0.f;
    int corr = faction_corruption_0_100(me);
    const char *domain = sz(tr((StrId)(STR_COUNCIL_EFF_0+seat)));
    int n=0;
    for (int slot=0; slot<SC_COUNCIL_CANDS && n<max; slot++){
        int tier = statecraft_council_cand_tier(seed, me, seat, slot, gen);
        out[n].slot = slot;
        out[n].nom  = sz(tr((StrId)statecraft_council_cand_name(seed, me, seat, slot, gen)));
        out[n].tier = tier;
        out[n].age  = statecraft_council_cand_age(seed, me, seat, slot, gen, s->sim.year);
        out[n].cost = statecraft_council_cand_cost(seed, me, seat, slot, gen, ipm);
        out[n].firstname = sz(statecraft_council_cand_firstname(seed, me, seat, slot, gen));
        out[n].house     = sz(statecraft_council_cand_house(seed, me, seat, slot, gen));
        out[n].faction   = sz(faction_name((int)statecraft_council_faction(seed, me, seat, slot, gen)));
        out[n].domain    = domain;
        float rank_mult = cons_rank_mult(seat, tier);
        float pred_loy  = cons_predicted_loyalty(seed, me, seat, slot, gen);
        float pred_eff  = cons_efficiency_calc(K, pred_loy, (float)corr);
        out[n].rank_bonus_pct  = cons_pct100(rank_mult - 1.f);
        out[n].efficiency_pct  = cons_pct100(pred_eff);
        out[n].final_bonus_pct = cons_pct100((rank_mult-1.f)*pred_eff);
        out[n].cost_rate_pct   = cons_tier_revenue_rate(tier) * 100.f;
        out[n].cost_year       = (double)out[n].cost * 12.0;
        int rlo = 66 - out[n].age, rhi = 73 - out[n].age;
        out[n].retire_lo = (rlo>0)?rlo:0; out[n].retire_hi = (rhi>0)?rhi:0;
        { int ci = cons_identity_index(seed, me, seat, slot, gen);
          out[n].identite    = CONS_IDENTITES[ci].nom;
          out[n].portrait_id = CONS_IDENTITES[ci].portrait_id;
          out[n].id_flavor   = CONS_IDENTITES[ci].flavor; }
        n++;
    }
    return n;
}

/* V2a — l'état de la PAIRE (a,b) de sièges du pays du JOUEUR (signal pour V2b :
 * rivalité/alliance/conspiration entre deux ministres). */
int scps_council_pair_state(ScpsSim *s, int seat_a, int seat_b){
    if (!s || !s->ready) return 0;
    int me = s->sim.player;
    if (me<0 || me>=s->w->n_countries) return 0;
    return (int)statecraft_council_pair_state(s->sim.sc, s->w, s->sim.econ, s->w->seed, me, seat_a, seat_b, s->sim.year);
}

/* ── Recâblage Politiques 2026-07-10 : miroirs READ-ONLY du catalogue de
 * scps_decrees.c (fichier possédé par un autre lot — HORS scope de cette
 * façade) — même discipline que cons_tier_revenue_rate/cons_rank_mult
 * ci-dessus (LOT Conseil) et scps_slave_prices (LOT M) : mêmes clés tune_f,
 * mêmes valeurs par défaut, aucun chiffre inventé, à re-synchroniser si
 * scps_decrees.c change ses clés/défauts. ────────────────────────────────── */
/* Taux annuel (fraction, PAS %) de chaque décret — miroir de decree_revenue_rate
 * (scps_decrees.c, `static`). Pour DECISION_AUDIT_OFFICES : le taux du coût
 * PONCTUEL (DECISION_AUDIT_REVENUE_RATE), pas une mensualisation. */
static float decree_revenue_rate_mirror(int id){
    switch ((DecreeId)id){
        case DECREE_RATIONS:          return tune_f("DECREE_RATIONS_REVENUE_RATE",      0.005f);
        case DECREE_FOYERS:           return tune_f("DECREE_FOYERS_REVENUE_RATE",       0.015f);
        case DECREE_ECOLES:           return tune_f("DECREE_ECOLES_REVENUE_RATE",       0.02f);
        case DECREE_ATELIERS:         return tune_f("DECREE_ATELIERS_REVENUE_RATE",     0.02f);
        case DECREE_COMPTOIRS:        return tune_f("DECREE_COMPTOIRS_REVENUE_RATE",    0.015f);
        case DECREE_CIRCULATION:      return tune_f("DECREE_CIRCULATION_REVENUE_RATE",  0.0075f);
        case DECREE_FRONTIERES:       return tune_f("DECREE_FRONTIERES_REVENUE_RATE",   0.0f);
        case DECREE_MECENAT:          return tune_f("DECREE_MECENAT_REVENUE_RATE",      0.015f);
        case DECREE_LEGATIONS:        return tune_f("DECREE_LEGATIONS_REVENUE_RATE",    0.015f);
        case DECREE_LEVEE_ENTRETENUE: return tune_f("DECREE_LEVEE_REVENUE_RATE",        0.0f);
        case DECISION_AUDIT_OFFICES:  return tune_f("DECISION_AUDIT_REVENUE_RATE",      0.25f);
        default: return 0.f;
    }
}
/* La paire mutuellement exclusive (radio-boutons) — miroir de decree_exclusive_of
 * (scps_decrees.c, `static`) : RATIONS⊥FOYERS · CIRCULATION⊥FRONTIÈRES, spec
 * « Orientations politiques LÉGÈRES ». -1 = aucune paire. */
static int decree_exclusive_id_mirror(int id){
    switch ((DecreeId)id){
        case DECREE_RATIONS:     return (int)DECREE_FOYERS;
        case DECREE_FOYERS:      return (int)DECREE_RATIONS;
        case DECREE_CIRCULATION: return (int)DECREE_FRONTIERES;
        case DECREE_FRONTIERES:  return (int)DECREE_CIRCULATION;
        default:                 return -1;
    }
}
/* AUDIT DES OFFICES : condition d'entrée SEULE (hors cooldown) — miroir de
 * audit_ready (scps_decrees.c, `static`), moitié corruption de la condition
 * combinée. `faction_corruption_0_100` est l'API PUBLIQUE déjà lue par le
 * Conseil (scps_factions.h) ; DECISION_AUDIT_CORRUPTION_MIN est la MÊME clé. */
static int decree_cond_met_mirror(int id, int cid){
    if ((DecreeId)id==DECISION_AUDIT_OFFICES)
        return faction_corruption_0_100(cid) >= (int)tune_f("DECISION_AUDIT_CORRUPTION_MIN", 20.f);
    return 1;   /* ÉDITs : la condition d'entrée EST `legal` (pas de cooldown séparé) */
}

/* DÉCRETS DU JOUEUR (civics) — état + légalité de TOUS les décrets, pour `country`. */
int scps_decrees_list(ScpsSim *s, int country, ScpsDecree *out, int max){
    if(!out || max<=0 || !s || !s->ready || country<0 || country>=s->w->n_countries) return 0;
    float ipm = econ_world_ipm(s->sim.econ);
    float revenue = econ_country_tax_year(country);
    int n=0;
    for (int id=0; id<DECREE_COUNT && n<max; id++){
        const DecreeDef *d = &DECREES[id];
        out[n].id       = id;
        out[n].nom      = sz(d->nom);
        out[n].flavor   = sz(d->flavor);
        out[n].plateaux = sz(d->plateaux);
        out[n].reforme  = (d->type==DCR_REFORME) ? 1 : 0;
        out[n].active   = decree_active(country, (DecreeId)id) ? 1 : 0;
        out[n].legal    = decree_legal(s->w, s->sim.econ, s->sim.ts, s->sim.wl, s->sim.sc,
                                       s->sim.dp, country, (DecreeId)id) ? 1 : 0;
        out[n].type          = (int)d->type;
        out[n].exclusive_id  = decree_exclusive_id_mirror(id);
        float rate            = decree_revenue_rate_mirror(id);
        out[n].cost_rate_pct = rate * 100.f;
        out[n].cost_year     = (double)(revenue * rate * ipm);
        out[n].cond_met       = decree_cond_met_mirror(id, country);
        /* DÉCISION seulement : `legal` == (cond_met ET cooldown==0) par construction
         * (decree_legal appelle EXACTEMENT audit_ready = cooldown<=0 && corruption>=min,
         * cf. scps_decrees.c cond_audit) — donc cond_met&&!legal ⟺ cooldown>0, une
         * DÉDUCTION, pas une invention (aucun accès direct à g_audit_cd, privé au module). */
        out[n].cooldown_active = (d->type==DCR_DECISION && out[n].cond_met && !out[n].legal) ? 1 : 0;
        n++;
    }
    return n;
}

/* L'ASSIETTE des coûts en % (cf. .h) — lectures PURES pour les hovers quantitatifs. */
double scps_country_revenue_year(ScpsSim *s, int country){
    if (!s || !s->ready || country<0 || country>=s->w->n_countries) return 0.0;
    return (double)econ_country_tax_year(country);
}
double scps_world_ipm_now(ScpsSim *s){
    if (!s || !s->ready) return 1.0;
    return (double)econ_world_ipm(s->sim.econ);
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
    /* BROUILLARD DE GUERRE (étape 2) : un empire NON DÉCOUVERT n'existe pas pour le joueur —
     * aucune option diplo (guerre/paix/alliance/pacte/embargo) tant qu'il n'est pas rencontré
     * (radius 2). L'UI grise tout ; retour 0 = « cible inconnue ». */
    if (!country_knows(p, t)) return 0;
    DiploState *d = s->sim.dp;
    DiploStatus st = diplo_status(d, p, t);
    int at_war = (st==DIPLO_WAR);
    int slot   = (diplo_ally_count(d,p) < DIPLO_ALLY_SLOTS) && (diplo_ally_count(d,t) < DIPLO_ALLY_SLOTS);
    int emb    = intertrade_embargoed(p, t) ? 1:0;
    /* W-GUERRE-3 — can_declare_war exige désormais un CB RÉELLEMENT UTILISABLE (miroir exact
     * du gate au drain, CMD_DECLARE_WAR) : un motif GRATUIT (subjugation/anti-piraterie, lu
     * de diplo_casus_belli) OU une intrigue FABRIQUÉE et MÛRE contre CETTE cible. Sans ça le
     * bouton serait actif mais la déclaration un no-op silencieux — griser franchement. */
    { CasusBelli cbn = diplo_casus_belli(s->w, s->sim.econ, s->sim.wp, d, p, t, RES_NONE);
      bool has_cb = (cbn!=CB_NONE && !diplo_cb_needs_fabrication(cbn)) || diplo_fab_ready_cb(d,p,t)!=CB_NONE;
      out->can_declare_war = (!at_war && diplo_truce_days(d,p,t)<=0.f && has_cb) ? 1:0; }
    out->truce_days         = diplo_truce_days(d,p,t);   /* pour distinguer trêve vs no-CB à l'UI */
    out->can_make_peace     = at_war ? 1:0;
    out->can_offer_alliance = (!at_war && st!=DIPLO_ALLIED && slot) ? 1:0;
    out->can_offer_pact     = (!at_war && !diplo_trade_pact(d,p,t)) ? 1:0;
    out->can_offer_migration = (!at_war && !diplo_migration_pact(d,p,t)) ? 1:0;  /* BRASSAGE */
    out->can_embargo        = emb ? 0:1;
    out->can_lift_embargo   = emb ? 1:0;
    out->would_accept_alliance = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_ALLIANCE) ? 1:0;
    out->would_accept_pact     = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_TRADE_PACT) ? 1:0;
    out->would_accept_migration = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_MIGRATION) ? 1:0;  /* BRASSAGE */
    out->would_accept_peace    = ai_consider_offer(s->w, s->sim.econ, s->sim.wp, d, s->sim.sc, p, t, OFFER_PEACE) ? 1:0;
    /* W-GUERRE-3 — l'état de l'intrigue (fabrication payante du CB offensif). */
    out->can_fabricate         = diplo_can_fabricate(s->w, s->sim.econ, d, p, t) ? 1:0;
    out->fabricate_cost        = diplo_fabricate_cost(s->sim.econ, t);
    FabState fst = diplo_fab_state(d, p, t);
    out->fabricating           = (fst==FAB_MATURING) ? 1:0;
    out->fabricating_days_left = (fst==FAB_MATURING) ? diplo_fab_days_left(d, p, t) : 0.f;
    out->cb_ready              = (fst==FAB_READY) ? 1:0;
    out->cb_ready_years_left   = (fst==FAB_READY) ? diplo_fab_days_left(d, p, t)/365.f : 0.f;
    return 1;
}

/* #26 — le RÉSUMÉ D'OPINION : ce que `country` pense du JOUEUR, décomposé (l'opinion
 * courante converge vers la somme des composantes — statecraft_opinion_parts). */
int scps_opinion_summary(ScpsSim *s, int country, ScpsOpinionParts *out){
    if (!out) return -1;
    memset(out, 0, sizeof *out);
    if (!s || !s->ready) return -1;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries || country<0 || country>=s->w->n_countries || country==p) return -1;
    OpinionParts pp;
    statecraft_opinion_parts(s->sim.sc, s->sim.dp, country, p, &pp);
    out->total   = statecraft_opinion(s->sim.sc, country, p);
    out->memory  = (int)(pp.mem  + (pp.mem<0.f ? -0.5f : 0.5f));
    out->ally    = (int)(pp.ally    + 0.5f);
    out->war     = (int)(pp.war     - 0.5f);
    out->vassal  = (int)(pp.vassal  + 0.5f);
    out->pact    = (int)(pp.pact    + 0.5f);
    out->embargo = (int)(pp.embargo - 0.5f);
    out->rancor  = (int)(pp.rancor  - 0.5f);
    return 0;
}

/* le JOURNAL D'ACTES : l'histoire datée joueur↔country (diplog), le poids des actes
 * de MÉMOIRE recalculé au taux de decay du moteur (le chiffre montré = ce qui reste). */
int scps_diplo_journal(ScpsSim *s, int country, ScpsDiploAct *out, int max){
    if (!s || !s->ready || !out || max<=0) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || country<0 || country>=s->w->n_countries || country==p) return 0;
    DiplogEntry fe[24];
    int n = diplog_pair(country, p, fe, (max<24)?max:24);
    int ynow = scps_year(s);
    float decay = tune_f("OPINION_MEM_DECAY", 0.0003f);
    for (int i=0;i<n;i++){
        out[i].year = fe[i].year; out[i].act = fe[i].act;
        out[i].a_id = fe[i].a;    out[i].b_id = fe[i].b;
        float dn = 0.f;
        if (fe[i].delta != 0.f){                    /* acte de MÉMOIRE : le restant décayé */
            float days = (float)(ynow - fe[i].year) * 365.f;
            if (days < 0.f) days = 0.f;
            dn = fe[i].delta * powf(1.f - decay, days);
        }
        out[i].delta_now = (int)(dn + (dn<0.f ? -0.5f : 0.5f));
    }
    return n;
}

/* §3 — LÉGALITÉ de construction PAR RÉGION : MIROIR EN LECTURE des gates du drain
 * CMD_BUILD → agency_build_acct (scps_agency.c), dans le MÊME ORDRE — le refus
 * rapporté = le premier refus que le drain opposerait. AUCUNE mutation (le drain
 * refuse en silence : ce reader est ce qui rend le bouton honnête).
 * `reason_out` (option) : 0 OK · 1 structurel (région/palier/file/côte) ·
 * 2 or insuffisant · 3 matière manquante (marché atteignable à sec) ·
 * 4 tech de palier manquante (LOT T : edifice_tier ⇐ econ_country_has_tier). */
int scps_build_legal_ex(ScpsSim *s, int region, int edifice, int *reason_out){
    if (reason_out) *reason_out = 1;
    if (!s || !s->ready || edifice<0 || edifice>=EDIFICE_COUNT) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries) return 0;
    int reg = region;
    if (reg<0){ int cp=s->w->country[p].capital_prov; reg = (cp>=0&&cp<s->w->n_provinces)? s->w->province[cp].region : -1; }
    WorldEconomy *e = s->sim.econ;
    if (reg<0 || reg>=e->n_regions || e->region[reg].owner != p) return 0;
    /* gates GÉO (miroir agency_build_acct) : port SUR la côte, Centre = débouché */
    if (edifice==EDI_PORT && !e->region[reg].coastal) return 0;
    if (edifice==EDI_TRADE_CENTER && !e->region[reg].coastal && !e->region[reg].estuary) return 0;
    if (edifice_build_blocked(e, reg, (Edifice)edifice)) return 0;
    if (agency_pending_build(s->sim.ag, reg, (Edifice)edifice)) return 0;   /* F5 : déjà en file */
    /* LOT T — même ordre que agency_build_acct : la tech de palier avant la matière/l'or. */
    { int et = edifice_tier((Edifice)edifice);
      if (et>1 && !econ_country_has_tier(p, et)){ if (reason_out) *reason_out=4; return 0; } }
    /* gate MATIÈRE (miroir) : chaque composante × étendue doit être trouvable au marché
     * atteignable (propre + Centre + réseau). L'étendue ×(1+0.15·n_régions) recompose
     * agency_extent_mult (static côté agency — même formule §7, documentée là-bas). */
    { const EdificeDef *d = edifice_def((Edifice)edifice);
      int nreg=0; for (int r2=0;r2<e->n_regions;r2++) if (e->region[r2].owner==p) nreg++;
      float ext = 1.f + 0.15f*(float)nreg;
      for (int k=0; d && k<BUILD_RES_MAX; k++){
          Resource r = d->cost.res[k];
          if (r<=RES_NONE || r>=RES_COUNT || d->cost.qty[k]<=0.f) continue;
          float av = intertrade_market_avail_ex(e, reg, r, NULL);
          if (d->cost.qty[k]*ext > av+1e-3f){ if (reason_out) *reason_out=3; return 0; }
      } }
    if (!credit_can_spend(e, s->w, p, agency_build_gold(e, reg, (Edifice)edifice))){
        if (reason_out) *reason_out=2;
        return 0;
    }
    if (reason_out) *reason_out = 0;
    return 1;
}
int scps_build_legal(ScpsSim *s, int region, int edifice){
    return scps_build_legal_ex(s, region, edifice, NULL);
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
    out->posture      = campaign_posture(s->sim.camp, cid);      /* lue du MOTEUR (fin de l'état local UI) */
    out->posture_name = sz(campaign_posture_name(out->posture));
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

/* CATÉGORIE tactique d'une unité (le hover joueur : « cavalerie lourde », « distance »…).
 * Dérivée de l'arme macro (unit_res_arm) + de la monture — 6 catégories seulement. */
static const char *unit_category_word(UnitType t){
    Resource arm = unit_res_arm(t);
    if (arm==RES_MAGE_STAFF || arm==RES_ALCHEMIST_KIT) return "Mage";
    if (arm==RES_ARMS_RANGED || arm==RES_FIREARM)       return "Distance";
    const UnitDef *d = unit_def(t);
    int w = d ? d->weapon : -1;
    int cav = (w==W_MONTURE_L || w==W_MONTURE_H || w==W_MONTURE_CUIRASSEE || w==W_MONTURE_RAID);
    int heavy = (arm==RES_ARMS_HEAVY || arm==RES_ENCHANTED_ARMS);
    if (cav)   return heavy ? "Cavalerie lourde" : "Cavalerie légère";
    return heavy ? "Infanterie lourde" : "Infanterie légère";
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
        o->categorie = unit_category_word((UnitType)t);
        o->arme   = sz(weapon_name(d->weapon));
        Resource arm = unit_res_arm((UnitType)t);
        if (arm==RES_NONE) snprintf(g_ucost[t], sizeof g_ucost[t], "%d (fortune)", POP_PER_UNIT);
        else               snprintf(g_ucost[t], sizeof g_ucost[t], "%d %s", POP_PER_UNIT, resource_name(arm));
        o->cout = g_ucost[t];
        g_uethos[t][0]=0; { int ne=0; for (int f=0; f<6; f++)        /* éthos : affinité ≥ 2 */
            if (warhost_unit_affinity(f,t) >= 2.f) sj(g_uethos[t], sizeof g_uethos[t], faction_name(f), &ne); }
        o->ethos = g_uethos[t][0] ? g_uethos[t] : "—";
        g_ufort[t][0]=0; g_ufaible[t][0]=0; { int nf=0, nw=0;       /* contres en CATÉGORIES (dédup) : >1.5 bat · <0.75 battu */
            for (int j=0; j<U_COUNT; j++){ if (j==t) continue; float m=matchup((UnitType)t,(UnitType)j);
                const char *cj = unit_category_word((UnitType)j);
                if      (m>1.5f  && nf<3 && !strstr(g_ufort[t],   cj)) sj(g_ufort[t],   sizeof g_ufort[t],   cj, &nf);
                else if (m<0.75f && nw<3 && !strstr(g_ufaible[t], cj)) sj(g_ufaible[t], sizeof g_ufaible[t], cj, &nw); } }
        o->fort   = g_ufort[t][0]   ? g_ufort[t]   : "—";
        o->faible = g_ufaible[t][0] ? g_ufaible[t] : "—";
        o->entretien_or10   = g10;
        o->entretien_vivre  = fd;
        o->recrutable = unit_recruitable(ts,(UnitType)t) ? 1 : 0;
        n++;
    }
    return n;
}

/* FLAVOR CYNIQUE par édifice (display-only — le mot du conseiller, jamais un levier).
 * Indexé par l'enum Edifice (scps_agency.h). */
static const char *const EDI_FLAVOR[EDIFICE_COUNT] = {
    [EDI_TRIBUNAL]      = "La justice est aveugle ; ses greffiers, eux, voient très bien qui paie.",
    [EDI_CHANCELLERIE]  = "Cent scribes pour écrire ce que le roi a dit, mille pour expliquer ce qu'il a voulu dire.",
    [EDI_ACADEMIE]      = "On y apprend tout, sauf d'où vient l'argent qui la finance.",
    [EDI_GARNISON]      = "Des murs pour protéger le peuple. Les portes ferment de l'intérieur, remarquez.",
    [EDI_FORTERESSE]    = "Assez solide pour tenir un siège ; assez visible pour en attirer.",
    [EDI_CITADELLE]     = "Quand la citadelle domine la ville, on ne sait plus qui surveille qui.",
    [EDI_PORT]          = "La mer rapporte tout : marchandises, nouvelles, et les ennuis des autres.",
    [EDI_CARAVANSERAIL] = "L'étranger y dort une nuit et y laisse dix ans de rumeurs.",
    [EDI_MARCHE]        = "Tout s'y achète, surtout ce qui n'a pas de prix.",
    [EDI_ENTREPOT]      = "La richesse d'un royaume se mesure à ce qu'il peut se permettre de laisser dormir.",
    [EDI_GRENIER]       = "Le grain d'hiver achète plus de loyauté que dix discours d'été.",
    [EDI_IRRIGATION]    = "L'eau va où on la mène. Les paysans aussi, en général.",
    [EDI_AQUEDUC]       = "Des arches jusqu'à l'horizon, pour que la ville oublie qu'elle boit une rivière.",
    [EDI_SANCTUAIRE]    = "Quatre murs de bois entre l'homme et l'infini. Ça suffit, curieusement.",
    [EDI_TEMPLE]        = "Plus le plafond est haut, plus les fidèles se sentent petits. C'est le but.",
    [EDI_CATHEDRALE]    = "Trois générations de tailleurs de pierre pour prouver que la foi dure plus qu'eux.",
    [EDI_BIBLIOTHEQUE]  = "Mille livres, dont neuf cents recopient les cent premiers.",
    [EDI_MONASTERE]     = "Ils ont fait vœu de silence ; leurs registres, eux, disent tout.",
    [EDI_COMPTOIR]      = "L'or n'a pas d'odeur, mais le comptoir tient les registres de qui a senti quoi.",
    [EDI_BANQUE]        = "Elle prête un parapluie quand il fait beau et le réclame quand il pleut.",
    [EDI_ARSENAL]       = "La paix se négocie toujours mieux avec un arsenal plein derrière soi.",
    [EDI_AMIRAUTE]      = "Des amiraux à terre qui décident où les marins mourront en mer.",
    [EDI_PORT_MARCHAND] = "Chaque quai supplémentaire raccourcit la distance entre votre or et celui des autres.",
    [EDI_BIBLIO_MIL]    = "On y étudie les défaites des autres, avec l'espoir d'être étudié plus tard.",
    [EDI_OBSERVATOIRE]  = "Les étoiles ne mentent jamais ; les astronomes compensent.",
    [EDI_TRADE_CENTER]  = "Le centre du monde, d'après les registres du centre du monde.",
};

/* l'EFFET RÉEL d'un édifice, composé de son delta ProvBuild (membrane : les MOTS
 * du jeu + les chiffres du moteur, jamais une promesse). Buffer statique par type. */
static const char *api_edifice_effet(Edifice e){
    static char eb[EDIFICE_COUNT][176];
    const EdificeDef *d = edifice_def(e);
    if (!d) return "";
    char *b = eb[e]; b[0]=0;
    int len=0, first=1;
    #define EF_ADD(fmt, val, label) do{ if((val)>0.001f){ \
        len += snprintf(b+len, sizeof eb[e]-(size_t)len, "%s" fmt, first?"":" · ", (double)(val)); \
        len += snprintf(b+len, sizeof eb[e]-(size_t)len, "%s", (label)); first=0; } }while(0)
    EF_ADD("institutions +%.1f", d->delta.K_inst,  " (stabilité, capacité)");
    EF_ADD("coercition +%.1f",   d->delta.H_coerc, " (tient la province, ronge la loyauté)");
    EF_ADD("ouverture +%.1f",    d->delta.P_open,  " (perméabilité, routes)");
    EF_ADD("prospérité +%.1f",   d->delta.PE_infra," (capte l'échange local)");
    EF_ADD("vivres +%.1f",       d->delta.food_cap," (rendement & réserve, démographie)");
    EF_ADD("foi +%.1f",          d->delta.faith,   " (apaise l'agitation, soutient la loyauté)");
    EF_ADD("savoir +%.1f",       d->delta.savoir,  " (recherche locale)");
    #undef EF_ADD
    if (d->delta.port > 0.001f){
        len += snprintf(b+len, sizeof eb[e]-(size_t)len, "%sport (rade réelle : routes de mer, flotte)", first?"":" · ");
        first=0;
    }
    if (first) snprintf(b, sizeof eb[e], "structurel (voir sa famille)");
    return b;
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
        /* PALIER FAMILIAL (2026-07-10, « une ligne, un bâtiment ») : l'UI masque un
         * tier tant que le PRÉCÉDENT de la famille n'est bâti nulle part chez nous. */
        o->tier = edifice_tier((Edifice)e);
        Edifice pv = edifice_prev((Edifice)e);   /* EDIFICE_COUNT = base/singleton */
        o->prev = (pv>=0 && pv<EDIFICE_COUNT && pv!=(Edifice)e) ? (int)pv : -1;
        o->effet  = sz(api_edifice_effet((Edifice)e));
        o->flavor = sz(EDI_FLAVOR[e]);
        o->prev_built = 0;
        if (o->prev >= 0 && country>=0){
            for (int pid=0; pid<s->sim.econ->n_prov; pid++){
                if (s->sim.econ->prov[pid].owner==country
                    && (s->sim.econ->prov[pid].edi_built & (1u<<o->prev))){
                    o->prev_built = 1;
                    break;
                }
            }
        }
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
    /* RE-KEY PROVINCE : alloc_on/alloc_raw/alloc_bld/bld_input sont PROVINCE-OWNED
     * (miroir, cf. econ_aggregate_regions) — un verbe joueur (scps_player_alloc_*) les
     * pose sur la province représentative au drain du MÊME jour (scps_sim_advance_days),
     * mais econ_tick (qui re-dérive region[].alloc_* depuis prov[]) ne tourne QUE
     * mensuellement — sans lire ICI la province, le panneau verrait l'override 1 mois
     * en retard. On lit donc alloc_* sur la province (fraîcheur immédiate) ; le reste
     * (pool/raw_cap/bâtiments, agrégats stables) continue de lire l'agrégat région. */
    int pid = econ_region_rep_province(e, region);
    const ProvinceEconomy *pe = (pid>=0 && pid<e->n_prov) ? &e->prov[pid] : NULL;
    out->region = region;
    out->on     = pe ? pe->alloc_on : re->alloc_on;
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
        k->workers = out->on ? 0.f : estw[n];
        k->weight  = out->on ? (pe ? pe->alloc_raw[g] : re->alloc_raw[g]) : 0;
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
        k->input   = (alt!=RES_NONE)? (int)(pe ? pe->bld_input[b] : re->bld_input[b]) : -1;
        k->closed  = (out->on && (pe ? pe->alloc_bld[b] : re->alloc_bld[b])==0)?1:0;
        k->workers = re->bld[i].workers;
        estw[n]    = re->bld[i].workers;
        k->weight  = out->on ? (pe ? pe->alloc_bld[b] : re->alloc_bld[b]) : 0;
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
        /* raccord 3 (Âges sans ordre imposé) — un nœud RÉELLEMENT verrouillé par un âge
         * pas encore avenu (Société3/Savoir4/Société5/Savoir5) ne s'affiche jamais OPEN,
         * même si tech_tree_readout (pur, sans wp) l'a jugé accessible. */
        { TreeState st = nd->state;
          const TechNode *gn = tech_node((TechId)i);
          if (st==TREE_OPEN && !ages_tech_researchable(s->sim.wp, gn->theme, gn->tier)) st=TREE_LOCKED;
          out[i].state = (int)st; }
        out[i].faustian= nd->faustian?1:0; out[i].orphan = nd->orphan?1:0;
        out[i].is_base = nd->is_base?1:0;
        out[i].name    = sz(nd->name);  out[i].unlocks = sz(nd->unlocks);
        out[i].effet   = sz(nd->effet);
        /* coût AFFICHÉ = coût de base × remise de diffusion × traditions (arcane, nœuds
         * faustiens) — ce que le joueur paiera vraiment (miroir de la voie scps_sim.c). */
        out[i].cost    = (int)(nd->cost * tech_diffusion_mult((TechId)i)
                             * ai_tech_tradition_mult(p, (TechId)i) + 0.5f);
        /* prérequis : node[i] ↔ TechId i, donc prereq (un TechId, TECH_COUNT=aucun)
         * est directement l'INDICE du nœud parent dans CE tableau → arête de l'arbre. */
        const TechNode *tn = tech_node((TechId)i);
        int pr = tn ? (int)tn->prereq : (int)TECH_COUNT;
        out[i].prereq = (pr < (int)TECH_COUNT && pr < n) ? pr : -1;
        /* HOVER CHIFFRÉ (retour joueur : « il faut spécifier COMBIEN ») : le mot mécanique
         * + les NOMBRES réels du nœud — les seuls leviers VIVANTS (dK/dL/dH/dPuissance/
         * fracture/charge/flux + prod%/eff% ; dEco/dMil/dF sont MORTS, jamais affichés).
         * Buffers statiques : la membrane rend des mots+nombres, l'hôte copie. */
        {
            static char hb[TECH_COUNT][176];
            int pos = snprintf(hb[i], sizeof hb[i], "%s", sz(tech_hover((TechId)i)));
            #define HB_ADD(...) do{ if(pos < (int)sizeof hb[i]) \
                pos += snprintf(hb[i]+pos, sizeof hb[i]-pos, __VA_ARGS__); }while(0)
            if(tn){
                const char *sep = (pos>0) ? " — " : "";
                int first = 1;
                #define HB_NUM(v, lbl) do{ if((v)!=0.f){ \
                    HB_ADD("%s%s %+g", first?sep:" · ", lbl, (double)(v)); first=0; } }while(0)
                HB_NUM(tn->dK,         "prospérité");
                HB_NUM(tn->dL,         "stabilité");
                HB_NUM(tn->dH,         "coercition");
                HB_NUM(tn->dPuissance, "puissance");
                HB_NUM(tn->dFracture,  "fracture");
                float pp = tech_node_prod_pct((TechId)i);
                float ep = tech_node_eff_pct((TechId)i);
                if(pp!=0.f){ HB_ADD("%sproduction %+d %%", first?sep:" · ", (int)(pp*100.f+0.5f)); first=0; }
                if(ep!=0.f){ HB_ADD("%sefficacité %+d %%", first?sep:" · ", (int)(ep*100.f+0.5f)); first=0; }
                HB_NUM(tn->charge,     "charge faustienne");
                HB_NUM(tn->flux,       "flux");
                #undef HB_NUM
            }
            #undef HB_ADD
            out[i].hover = hb[i];
        }
        out[i].flavor  = sz(tech_flavor((TechId)i));
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

/* MÉTABOLISATION POUR LA VICTOIRE (P5) — la seconde lecture, distincte de
 * scps_player_heritage_access ci-dessus (accès TECH, pop-share). Ici : ce que
 * `endgame_metab_count`/wonder_tick comptent RÉELLEMENT pour faire progresser un
 * palier de la Merveille (endgame_heritage_detail, scps_endgame.c). */
int scps_merv_metab(ScpsSim *s, ScpsMervHeritage *out, int max, int *count, int *required){
    if (count) *count = 0;
    if (required) *required = 0;
    if(!out || max<=0 || !s || !s->ready) return 0;
    int p = s->sim.player;
    if(p<0 || p>=s->w->n_countries) return 0;
    EndgameHeritageDetail det[HERITAGE_COUNT];
    endgame_heritage_detail(s->w, s->sim.econ, s->sim.ts, p, det);
    int n = (HERITAGE_COUNT < max) ? HERITAGE_COUNT : max;
    int cnt = 0;
    for(int h=0; h<HERITAGE_COUNT; h++) if(det[h].metabolized) cnt++;
    for(int r=0;r<n;r++){
        out[r].nom          = sz(heritage_name((Heritage)r));
        out[r].metabolized  = det[r].metabolized ? 1 : 0;
        out[r].voie         = sz(det[r].voie);
        out[r].progress_pct = det[r].progress_pct;
        out[r].native       = (det[r].voie && strcmp(det[r].voie,"natif")==0) ? 1 : 0;
    }
    if (count) *count = cnt;
    if (required && s->sim.eg) *required = endgame_metab_required(s->sim.eg->merv);
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

/* LANGUE — bascule la table compilée que tr() résout (GLOBAL, display-only).
 * Hors bornes : ignoré (la langue courante reste). La surcharge scps_lang.txt
 * reste au-dessus — tr() lit g_override avant la table. */
void scps_lang_set(int lang){
    if (lang == 0) lang_set(LANG_FR);
    else if (lang == 1) lang_set(LANG_EN);
}
int scps_lang_get(void){ return (lang_get() == LANG_EN) ? 1 : 0; }

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
    out->text=""; out->reward_mat=""; out->resp_seat=""; out->resp_name="";
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
    /* CARTE MISSION — le siège RESPONSABLE (mission_responsible_seat, déduit du type,
     * aucun état neuf) + son bonus (mult ∝ rang×efficacité, miroir de mission_reward_mult)
     * + la récompense PRÉVUE (base × mult). */
    int seat = mission_responsible_seat(m);
    if (seat>=0){
        out->resp_seat = sz(tr((StrId)(STR_COUNCIL_SEAT_0+seat)));
        int slot = statecraft_council_seated(s->sim.sc, cid, seat);
        if (slot>=0){
            int gen = statecraft_council_seated_gen(s->sim.sc, cid, seat);
            out->resp_tier = statecraft_council_cand_tier(s->w->seed, cid, seat, slot, gen);
            out->resp_name = sz(tr((StrId)statecraft_council_cand_name(s->w->seed, cid, seat, slot, gen)));
        }
    }
    float mult = cons_mission_reward_mult(s->sim.sc, s->sim.wp, s->w->seed, cid, m);
    out->resp_bonus_pct  = cons_pct100(mult - 1.f);
    out->reward_gold_adj = (double)m->reward_gold * (double)mult;
    out->reward_qty_adj  = (double)m->reward_qty  * (double)mult;
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
int scps_player_offer_migration(ScpsSim *s, int target){   /* BRASSAGE : pacte migratoire */
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_OFFER_MIGRATION, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
/* W-GUERRE-3 — FABRIQUER une revendication (payante) contre `target` : enfile l'ordre,
 * revalidé au drain (diplo_can_fabricate : cible valide, or suffisant, pas déjà en cours). */
int scps_player_fabricate_cb(ScpsSim *s, int target){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_FABRICATE_CB, { target, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_build_manuf(ScpsSim *s, int region, int bld){   /* PANNEAU B : poser une manufacture */
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_BUILD_MANUF, { region, bld, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
/* LÉGALITÉ manufacture (miroir READ-ONLY des gates du drain CMD_BUILD_MANUF) :
 * région au joueur+peuplée · type civil non-faustien à intrant · slot libre ·
 * staffage (250/manuf) · tier · intrant nourrissable (ici OU l'empire) · or. */
int scps_manuf_legal(ScpsSim *s, int region, int bld){
    if (!s || !s->ready || region<0 || bld<0 || bld>=BLD_TYPE_COUNT) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries) return 0;
    const WorldEconomy *e=s->sim.econ;
    if (region>=e->n_regions) return 0;
    const RegionEconomy *re=&e->region[region];
    if (re->owner != p || !re->colonized) return 0;
    if (bld_is_faustian((BuildingType)bld)) return 0;
    Resource in1,in2,out; building_recipe((BuildingType)bld,&in1,&in2,&out); (void)in2;
    /* in1==RES_NONE ≠ dégénéré : les RAW-WORKS (four à brique·carrière·scierie) sont
     * HORS-SOL par conception (N1) — elles boostent l'output de brut. Retour joueur
     * 2026-07-09 : elles étaient verrouillées hors du panneau. Seul out==NONE rejette. */
    if (out==RES_NONE) return 0;
    if (out==RES_ARMS || out==RES_ARMS_HEAVY || out==RES_ARMS_RANGED || out==RES_FIREARM
        || out==RES_GUNPOWDER || out==RES_ENCHANTED_ARMS || out==RES_ESSENCE || out==RES_FLUX) return 0;
    for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==(BuildingType)bld) return 0;
    float rpop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
    if (rpop < 250.f*(float)(re->n_bld+1)) return 0;
    if (capitale_max_tier((long)rpop) < bld_min_tier((BuildingType)bld)) return 0;
    bool feed = (in1==RES_NONE) || (re->raw_cap[in1] > 0.f);   /* hors-sol ⇒ rien à nourrir */
    for (int r2=0;r2<e->n_regions && !feed;r2++)
        if (e->region[r2].owner==p && e->region[r2].raw_cap[in1]>0.f) feed=true;
    if (!feed) return 0;
    float cost=tune_f("MANUF_BUILD_COST",50.f)*econ_world_ipm(e);
    return credit_can_spend(e, s->w, p, cost) ? 1 : 0;
}
/* le PRIX affiché = le prix payé — MÊME formule EXACTE que le drain CMD_BUILD_MANUF
 * (scps_sim.c: cost=tune_f("MANUF_BUILD_COST",50.f)*econ_world_ipm(s->econ)*
 * decree_manuf_cost_mult(p)), y compris la remise ATELIERS (2026-07-10 : le prix
 * montré ignorait la remise, cf. TROUVAILLES.md « remise ATELIERS non reflétée »). */
int scps_manuf_cost(ScpsSim *s){
    if (!s || !s->ready) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    float mult = (p>=0 && p<s->w->n_countries) ? decree_manuf_cost_mult(p) : 1.f;
    float cost = tune_f("MANUF_BUILD_COST",50.f)*econ_world_ipm(s->sim.econ)*mult;
    return (int)(cost + 0.5f);
}
int scps_player_embargo(ScpsSim *s, int target, int on){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_EMBARGO, { target, on?1:0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* ── ESCLAVAGE — garder/affranchir/vendre ──────────────────────────────────── */
int scps_player_manumit(ScpsSim *s){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_MANUMIT, { 0,0,0,0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_slave_sell(ScpsSim *s, int region, long count){
    if (!s || !s->ready || count<=0) return 0;
    PlayerCmd c = { CMD_SLAVE_SELL, { region, (int32_t)count, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_slave_buy(ScpsSim *s, int region, long count){
    if (!s || !s->ready || count<=0) return 0;
    PlayerCmd c = { CMD_SLAVE_BUY, { region, (int32_t)count, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_slave_market(ScpsSim *s, ScpsSlavePoolLine *out, int max, long *total_out, int *can_buy_out){
    if (total_out) *total_out=0;
    if (can_buy_out) *can_buy_out=0;
    if (!s || !s->ready || !out || max<=0) return 0;
    float pool[HERITAGE_COUNT];
    intertrade_slave_pool(pool);
    int n=0;
    for (int h=0;h<HERITAGE_COUNT && n<max;h++){
        if (pool[h]<1.f) continue;
        out[n].heritage = sz(heritage_name((Heritage)h));
        out[n].count = (long)pool[h];
        n++;
    }
    if (total_out) *total_out = intertrade_slave_pool_count();
    if (can_buy_out){
        int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
        *can_buy_out = (p>=0 && p<s->w->n_countries
                        && econ_country_can_enslave(s->w, s->sim.econ, &s->sim.ts[p], p)) ? 1 : 0;
    }
    return n;
}
/* PRIX du marché servile — miroir READ-ONLY d'intertrade_slave_buy/_sell (le SPREAD
 * ×2 achat / ×1 vente débité au drain, jamais montré jusqu'ici). slave_pool_price_mult
 * est static côté intertrade (module possédé par un autre lot) : la respiration du pool
 * (ref/(pool+ref·0.10), bornée [0.5,2.5]) est RECOMPOSÉE depuis les lecteurs PUBLICS
 * pool_count + tune_f — même formule, à re-synchroniser si intertrade la change. */
void scps_slave_prices(ScpsSim *s, int *buy_out, int *sell_out){
    if (buy_out) *buy_out = 0;
    if (sell_out) *sell_out = 0;
    if (!s || !s->ready) return;
    float ref = tune_f("SLAVE_POOL_REF", 600.f); if (ref < 1.f) ref = 1.f;
    float pool = (float)intertrade_slave_pool_count();
    float mult = ref/(pool + ref*0.10f);
    if (mult < 0.5f) mult = 0.5f; else if (mult > 2.5f) mult = 2.5f;
    float base = tune_f("SLAVE_PRICE",40.f)*econ_world_ipm(s->sim.econ)*mult;
    if (sell_out) *sell_out = (int)(base + 0.5f);
    if (buy_out)  *buy_out  = (int)(base*2.f + 0.5f);
}

/* ── LOT G — RÉINCORPORATION DE POP ──────────────────────────────────────── */
int scps_player_pop_transfer(ScpsSim *s, int src_region, int dst_region, int klass, long count){
    if (!s || !s->ready || count<=0) return 0;
    PlayerCmd c = { CMD_POP_TRANSFER, { src_region, dst_region, klass, (int32_t)count } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* ── LOT J — L'APERÇU DE MANUMISSION (lecture PURE, aucune mutation) ────────
 * Balaie les provinces du joueur, compte les âmes/groupes CLASS_SLAVE, et projette
 * la friction off-culture (économique) qu'elles porteraient une fois LIBRES — le
 * même calcul qu'econ_off_culture_fraction, mais en incluant les groupes esclaves
 * COMME S'ILS étaient déjà affranchis (motif des options-readers : un aperçu,
 * jamais une mutation). Pondéré par province (Σ off × pop) / Σ pop du pays. */
int scps_manumit_preview(ScpsSim *s, ScpsManumitPreview *out){
    if (!out) return 0;
    memset(out, 0, sizeof *out);
    if (!s || !s->ready) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries) return 0;
    WorldEconomy *e = s->sim.econ;
    long souls=0; int n_groups=0; long total_pop=0;
    double off_num=0.0, off_den=0.0;
    for (int q=0;q<e->n_prov;q++){
        const ProvinceEconomy *pe=&e->prov[q];
        if (pe->owner!=p || !pe->colonized) continue;
        const ProvincePop *pp=&pe->pop;
        long ptot=0;
        for (int i=0;i<pp->n_groups;i++) ptot += pp->groups[i].count;
        total_pop += ptot;
        for (int i=0;i<pp->n_groups;i++)
            if (pp->groups[i].klass==CLASS_SLAVE){ souls += pp->groups[i].count; n_groups++; }
        if (pp->n_groups<=1 || ptot<=0) continue;
        /* projection : le dominant s'élit COMME SI aucun groupe n'était plus esclave
         * (miroir econ_off_culture_fraction, sans le filtre group_is_slave). */
        int dom=-1; long best=-1;
        for (int i=0;i<pp->n_groups;i++) if (pp->groups[i].count>best){ best=pp->groups[i].count; dom=i; }
        if (dom<0) continue;
        Sphere doms = pp->groups[dom].origin_sphere;
        float poff=0.f;
        for (int i=0;i<pp->n_groups;i++){
            float sd = sphere_distance(doms, pp->groups[i].origin_sphere)/7.f;
            float mism = sd * (1.f - clampf(pp->groups[i].integration,0.f,1.f));
            poff += mism * (float)pp->groups[i].count;
        }
        off_num += (double)poff; off_den += (double)ptot;
    }
    out->souls = souls;
    out->n_groups = n_groups;
    out->pct_of_country = (total_pop>0) ? (float)(100.0*(double)souls/(double)total_pop) : 0.f;
    out->friction_after = (off_den>0.0) ? (float)(off_num/off_den) : 0.f;
    return 1;
}

/* PÉNURIES (UI-2) — voir scps_api.h. Miroir DIRECT d'econ_country_forecast (lecture
 * pure) : le seuil « runway court » est AI_SAFETY_HORIZON — le MÊME que l'IA lit pour
 * son propre food_alert (scps_ai.c:319/scps_ai.h:62) : ce n'est pas un chiffre inventé
 * pour l'UI, c'est le seuil réel de « le mur approche » côté moteur. */
int scps_country_shortages(ScpsSim *s, int country, ScpsShortage *out, int max){
    if (!out || max<=0) return 0;
    if (!s || !s->ready || country<0 || country>=s->w->n_countries) return 0;
    EconForecast fc;
    econ_country_forecast(s->sim.econ, country, tune_f("AI_PROJ_HORIZON",25.f), &fc);
    float safety = tune_f("AI_SAFETY_HORIZON",12.f);
    int idx[RES_COUNT]; int ni=0;
    for (int g=1; g<RES_COUNT; g++){
        int structurel = fc.struct_deficit[g] ? 1 : 0;
        if (!structurel && fc.runway[g]>=safety) continue;   /* sain : ni structurel ni court */
        idx[ni++]=g;
    }
    /* tri à bulles (ni ≤ RES_COUNT, petit) par runway croissant — le plus urgent d'abord */
    for (int i=0;i<ni;i++)
        for (int j=i+1;j<ni;j++)
            if (fc.runway[idx[j]] < fc.runway[idx[i]]){ int t=idx[i]; idx[i]=idx[j]; idx[j]=t; }
    int n=0;
    for (int i=0;i<ni && n<max;i++){
        int g = idx[i];
        out[n].nom = sz(resource_name((Resource)g));
        out[n].res_id = g;
        out[n].runway_days = (fc.runway[g]>=1.0e8f) ? -1.f : fc.runway[g]*365.f;
        out[n].structurel = fc.struct_deficit[g] ? 1 : 0;
        n++;
    }
    return n;
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
/* miroir de biggest_minority (scps_agency.c:560-573, `static` non-exportée) : le plus
 * gros groupe MINORITAIRE d'une province (cible d'AGY_ASSIMILATE/AGY_PURGE), hors
 * dominant et hors CLASS_SLAVE (l'esclavage n'est ni une cible d'assimilation civique
 * ni de purge politique, §II.6) ; -1 si homogène/entièrement servile. */
static int ap_biggest_minority(const ProvincePop *pp){
    if (!pp || pp->n_groups<2) return -1;
    int dom=-1;
    for (int g=0; g<pp->n_groups; g++){
        if (pp->groups[g].klass==CLASS_SLAVE) continue;
        if (dom<0 || pp->groups[g].count>pp->groups[dom].count) dom=g;
    }
    if (dom<0) return -1;
    int best=-1; long bc=0;
    for (int g=0; g<pp->n_groups; g++){
        if (g==dom || pp->groups[g].klass==CLASS_SLAVE) continue;
        if (pp->groups[g].count>bc){ bc=pp->groups[g].count; best=g; }
    }
    return best;
}
/* APERÇU D'ACTION (UI-4) — voir scps_api.h. Miroir EXACT des trois leviers intérieurs
 * (scps_agency.c) : jamais de mutation du monde réel (copie jetable de ProvincePop +
 * ModifierStack scratch pour REPRESS, motif malloc de demography_demo.c/revolt_demo.c —
 * ModifierStack pèse ~112 Ko, sur le TAS, pas la pile — cf. TROUVAILLES stack-overflow
 * Windows). */
int scps_action_preview(ScpsSim *s, int region, int verb, ScpsActionPreview *out){
    if (!out) return 0;
    memset(out, 0, sizeof *out);
    if (!s || !s->ready || verb<0 || verb>2) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=s->w->n_countries) return 0;
    WorldEconomy *e = s->sim.econ;
    if (region<0 || region>=e->n_regions || e->region[region].owner!=p) return 0;
    int pid = econ_region_rep_province(e, region);
    if (pid<0 || pid>=e->n_prov) return 0;
    ProvinceEconomy *re = &e->prov[pid];
    switch (verb){
      case 0: { /* MATER — AGY_REPRESS, scps_agency.c:670-677 */
        out->cost_gold = 0.f;
        out->duration_days = 30;   /* miroir REPRESS_DAYS, scps_agency.c:523 */
        float H = 4.f + re->build.H_coerc;   /* miroir scps_agency.c:673 */
        ProvincePop tmp = re->pop;           /* copie jetable : AUCUNE mutation du monde réel */
        ModifierStack *scratch = (ModifierStack*)calloc(1, sizeof(ModifierStack));
        if (!scratch) return 0;
        CoercionEffect ce = province_apply_coercion(&tmp, scratch, H);   /* la SORTIE réelle de la formule */
        free(scratch);
        out->pop_delta = 0;                  /* on mate, on ne tue pas */
        out->satisfaction_delta = 0;         /* aucune formule de MATER ne touche `satisfaction` */
        out->agitation_delta = -(int)lroundf(ce.agitation_drop);
        { float dcoerc = fminf(1.f, re->coercion+0.5f) - re->coercion;   /* miroir scps_agency.c:674 */
          out->coercition_delta = (int)lroundf(dcoerc*100.f); }
        snprintf(out->risque, sizeof out->risque, "%s",
            "Masque l'agitation, ne la résout pas : elle resurgira amplifiée à la levée de la coercition.");
      } break;
      case 1: { /* FORMER — AGY_ASSIMILATE, scps_agency.c:678-690 */
        out->cost_gold = 0.f;
        out->duration_days = 365;   /* miroir ASSIM_DAYS, scps_agency.c:524 */
        int gi = ap_biggest_minority(&re->pop);
        if (gi>=0){
            const PopGroup *pg = &re->pop.groups[gi];
            /* creuset = TECH_INTEGRATION débloquée pour ce pays (ce qu'une UI éclairée
             * passerait en a[1] de CMD_ASSIMILATE, scps_sim.c:469) */
            int creuset = (s->sim.ts && s->sim.ts[p].unlocked[TECH_INTEGRATION]) ? 1 : 0;
            float Lafter = fmaxf(0.f, pg->L-0.5f);   /* miroir scps_agency.c:686 */
            /* agit_from_L (scps_demography.c:59, formule PUBLIQUE) mirée à l'identique :
             * clampf((6-L)*15,0,100). */
            float agit_now   = clampf((6.f-pg->L)*15.f,  0.f, 100.f);
            float agit_after = clampf((6.f-Lafter)*15.f, 0.f, 100.f);
            out->pop_delta = 0;                    /* aucune mort : une conversion */
            out->satisfaction_delta = 0;            /* aucune formule de FORMER ne touche `satisfaction` */
            out->agitation_delta = (int)lroundf(agit_after-agit_now);   /* le frottement : monte */
            { float dcoerc = fminf(1.f, re->coercion+0.10f) - re->coercion;   /* miroir scps_agency.c:687 */
              out->coercition_delta = (int)lroundf(dcoerc*100.f); }
            snprintf(out->risque, sizeof out->risque,
                creuset ? "Le Creuset accélère l'intégration (+50%%) au prix d'un frottement immédiat (la légitimité du groupe chute)."
                        : "Intégration lente (+25%% sans Creuset) : le groupe reste restif le temps de la conversion.");
        } else {
            snprintf(out->risque, sizeof out->risque, "%s",
                "Province homogène ou entièrement servile : aucune minorité à former.");
        }
      } break;
      case 2: { /* PURGER — AGY_PURGE, scps_agency.c:691-696 + purge_slice:575-608 */
        out->cost_gold = 0.f;
        out->duration_days = AGY_PURGE_YEARS*365;   /* scps_agency.h:169, exportée */
        int gi = ap_biggest_minority(&re->pop);
        if (gi>=0){
            const PopGroup *pg = &re->pop.groups[gi];
            long dead = (long)((float)pg->count*0.12f);   /* miroir PURGE_FRAC_AN, scps_agency.c:525 */
            if (dead<1) dead = pg->count;
            out->pop_delta = -(int)dead;   /* la 1re tranche annuelle — les suivantes dépendent d'un monde futur */
        } else {
            out->pop_delta = 0;
        }
        out->satisfaction_delta = 0;   /* purge_slice ne touche aucun champ satisfaction */
        out->agitation_delta = 0;      /* purge_slice ne mate aucune agitation — elle tue */
        { float dcoerc = 1.f - re->coercion;   /* miroir scps_agency.c:604 : coercion → PLAFOND 1.0 */
          out->coercition_delta = (int)lroundf(dcoerc*100.f); }
        snprintf(out->risque, sizeof out->risque,
            "Tranches ANNUELLES sur %d ans (annulable en cours, le gâchis reste) : légitimité au plancher, stabilité plombée pour des années.",
            AGY_PURGE_YEARS);
      } break;
      default: return 0;
    }
    return 1;
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
/* V2a — le curseur de PAIE (0..2) : encodé ×100 pour tenir dans l'entier du journal. */
int scps_player_council_pay(ScpsSim *s, int seat, float pay){
    if (!s || !s->ready) return 0;
    if (pay<0.f) pay=0.f; else if (pay>2.f) pay=2.f;
    PlayerCmd c = { CMD_COUNCIL_PAY, { seat, (int32_t)(pay*100.f+0.5f), 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
int scps_player_decree(ScpsSim *s, int id, int on){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_DECREE, { id, on?1:0, 0, 0 } };
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
/* LOT P (2026-07-07) — PILLER LA CÔTE : lecture PURE, miroir EXACT du gate au drain
 * (CMD_RAID_COAST, scps_sim.c) — la piraterie reste un acte GRIS (pas de guerre
 * exigée, comme la course pirate IA existante ; « pas d'allié/pacte » est le garde-
 * fou explicite du joueur). */
int scps_can_raid_coast(ScpsSim *s, int prov, int *reason_out){
    if (reason_out) *reason_out = 1;
    if (!s || !s->ready) return 0;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    WorldEconomy *econ = s->sim.econ;
    if (prov<0 || prov>=econ->n_prov) return 0;
    const ProvinceEconomy *pv = &econ->prov[prov];
    if (!pv->colonized || !pv->coastal) { if (reason_out) *reason_out=1; return 0; }
    int victim = pv->owner;
    if (victim<0 || victim==p) { if (reason_out) *reason_out=2; return 0; }
    if (diplo_status(s->sim.dp, p, victim)==DIPLO_ALLIED) { if (reason_out) *reason_out=2; return 0; }
    if (diplo_trade_pact(s->sim.dp, p, victim)) { if (reason_out) *reason_out=2; return 0; }
    int region = (prov<s->w->n_provinces) ? s->w->province[prov].region : -1;
    if (region<0 || region>=econ->n_regions) { if (reason_out) *reason_out=1; return 0; }
    /* CD lu sur la PROVINCE REPRÉSENTATIVE (celle que navy_mark_raided écrit) — la vue
     * region[] n'est rafraîchie qu'au MOIS (piège de re-lecture périmée, cf. drain). */
    { int rp=econ_region_rep_province(econ, region);
      if (rp>=0 && rp<econ->n_prov && econ->prov[rp].raid_cd_days > 0.f) { if (reason_out) *reason_out=3; return 0; } }
    if (p<0 || p>=SCPS_MAX_COUNTRY || s->sim.navy->n[p].hull[HULL_PIRATE]<=0) { if (reason_out) *reason_out=4; return 0; }
    if (reason_out) *reason_out = 0;
    return 1;
}
/* le CD RESTANT (jours, arrondi) de la province — pour afficher « côte balafrée — X j »
 * (le rappel de la scar) sur le bouton grisé. 0 si raidable/hors-borne. */
int scps_raid_cd_days(ScpsSim *s, int prov){
    if (!s || !s->ready) return 0;
    WorldEconomy *econ = s->sim.econ;
    if (prov<0 || prov>=econ->n_prov) return 0;
    int region = (prov<s->w->n_provinces) ? s->w->province[prov].region : -1;
    if (region<0 || region>=econ->n_regions) return 0;
    int rp=econ_region_rep_province(econ, region);
    if (rp<0 || rp>=econ->n_prov) return 0;
    float d = econ->prov[rp].raid_cd_days;
    return (d>0.f) ? (int)(d+0.5f) : 0;
}
int scps_player_raid_coast(ScpsSim *s, int prov){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_RAID_COAST, { prov, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
/* BANC SEULEMENT (motif intertrade_debug_set_hub_of, lot A) : accorde des coques
 * PIRATE au joueur sans passer par le chantier — le round-trip scps_api_demo
 * (légal → enfilé → drainé → CD posé) exige une coque, or le monde de test n'a pas
 * toujours un port assez riche pour en bâtir une dans la fenêtre du banc. Jamais
 * appelé par le jeu/le binding. */
void scps_debug_set_pirate_hulls(ScpsSim *s, int n){
    if (!s || !s->ready) return;
    int p = (s->sim.human_player>=0) ? s->sim.human_player : s->sim.player;
    if (p<0 || p>=SCPS_MAX_COUNTRY) return;
    s->sim.navy->n[p].hull[HULL_PIRATE] = (n<0)?0:n;
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

/* §7 — ENGAGEMENT D'ÂGE (le verbe du joueur ; l'IA s'engage auto au lever). */
int scps_player_age_engage(ScpsSim *s){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_AGE_ENGAGE, { 0, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
/* COLONISATION (charte : « le joueur colonise n'importe quelle province ») — ENFILE ;
 * la source (province la plus peuplée du joueur) et les portes vivent au drain. */
int scps_player_colonize(ScpsSim *s, int prov){
    if (!s || !s->ready) return 0;
    PlayerCmd c = { CMD_COLONIZE, { prov, 0, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}
/* READ de légalité (griser le bouton) : la cible est-elle colonisable ET le joueur
 * a-t-il une source aux portes (pop ≥ min, vivres) ? Miroir CONST du drain. */
int scps_can_colonize(ScpsSim *s, int prov){
    if (!s || !s->ready || prov<0 || prov>=s->sim.econ->n_prov) return 0;
    const ProvinceEconomy *dst = &s->sim.econ->prov[prov];
    if (!dst->active || dst->colonized) return 0;
    int p = s->sim.player;
    /* CADENCE (v50) : un chantier à la fois, 1 ordre/an — le bouton se grise pendant */
    if (p>=0 && p<SCPS_MAX_COUNTRY){
        const struct ColonyWork *cw=&s->sim.econ->colony[p];
        if (cw->dst>=0 || cw->cd_days>0) return 0;
    }
    for (int q=0;q<s->sim.econ->n_prov;q++){
        const ProvinceEconomy *pe=&s->sim.econ->prov[q];
        if (pe->owner!=p || !pe->colonized) continue;
        float pp=0.f; for (int k=0;k<CLASS_COUNT;k++) pp+=pe->strata[k].pop;
        if (pp>=800.f && pe->food_sat>=0.5f) return 1;   /* approximation UI des portes (le drain revalide) */
    }
    return 0;
}
/* LE CHANTIER DE COLONISATION du joueur (readout sidebar/topbar) : active=1 s'il mûrit.
 * days_left/total_days = la progression ; cd_days = cadence avant le prochain ordre ;
 * yield_pct = le rendement attendu à l'arrivée (log-distance capitale). Lecture pure. */
int scps_colony_status(ScpsSim *s, int *dst_prov, int *days_left, int *total_days,
                       int *cd_days, int *yield_pct){
    if (dst_prov) *dst_prov=-1;
    if (days_left) *days_left=0;
    if (total_days) *total_days=0;
    if (cd_days) *cd_days=0;
    if (yield_pct) *yield_pct=0;
    if (!s || !s->ready) return 0;
    int p=s->sim.player;
    if (p<0 || p>=SCPS_MAX_COUNTRY) return 0;
    const struct ColonyWork *cw=&s->sim.econ->colony[p];
    if (cd_days) *cd_days=cw->cd_days;
    if (cw->dst<0) return 0;
    if (dst_prov) *dst_prov=cw->dst;
    if (days_left) *days_left=cw->days_left;
    if (total_days) *total_days=cw->total_days;
    if (yield_pct) *yield_pct=(int)(cw->yield*100.f+0.5f);
    return 1;
}
/* NOURRITURE DISPONIBLE du pays (topbar) : Σ stock vivrier (grain·poisson·bétail·fruit)
 * sur ses provinces — le nombre TANGIBLE (rations en réserve). Lecture pure. */
double scps_country_food(const ScpsSim *s, int c){
    if (!s || !s->ready || c<0) return 0.0;
    double tot=0.0;
    for (int q=0;q<s->sim.econ->n_prov;q++){
        const ProvinceEconomy *pe=&s->sim.econ->prov[q];
        if (pe->owner!=c || !pe->colonized) continue;
        tot += pe->stock[RES_GRAIN]+pe->stock[RES_FISH]+pe->stock[RES_LIVESTOCK]+pe->stock[RES_FRUIT];
    }
    return tot;
}
/* LE DIPLOMATE (v50) : jours avant que l'émissaire du joueur soit à nouveau disponible
 * (0 = prêt). L'UI grise les verbes diplo et affiche « émissaire en tournée (N j) ». */
int scps_diplo_cd(const ScpsSim *s){
    if (!s || !s->ready) return 0;
    int left = s->sim.diplo_ready_day - s->sim.day;
    return left>0 ? left : 0;
}
/* LES FACTIONS d'un pays (le spectre d'éthos interne, §9 UI) : la distribution
 * EFFECTIVE (groupes + stance des leviers), la RANCŒUR par faction, la faction
 * DOMINANTE, la TENSION DE COUP et la CORRUPTION (capture de l'État). Mots résolus
 * (faction_name — membrane), parts en 0-100. Lecture pure. Renvoie n (≤ max). */
int scps_country_factions(ScpsSim *s, int cid, ScpsFaction *out, int max,
                          int *coup, int *corruption){
    if (coup) *coup = 0;
    if (corruption) *corruption = 0;
    if (!s || !s->ready || !out || max<=0 || cid<0 || cid>=s->w->n_countries) return 0;
    float wgt[FAC_COUNT];
    EthosFaction dom = faction_effective_distribution(s->w, s->sim.econ, cid, wgt);
    int n = FAC_COUNT < max ? FAC_COUNT : max;
    for (int f=0; f<n; f++){
        out[f].name     = faction_name((EthosFaction)f);
        out[f].part     = (int)(wgt[f]*100.f + 0.5f);
        out[f].grief    = (int)(faction_grievance(cid, (EthosFaction)f)*100.f + 0.5f);
        out[f].dominant = (f == (int)dom) ? 1 : 0;
    }
    if (coup){
        EthosFaction al;
        *coup = (int)(faction_coup_tension_c(s->w, s->sim.econ, cid, &al)*100.f + 0.5f);
    }
    if (corruption) *corruption = faction_corruption_0_100(cid);
    return n;
}
/* total de provinces COLONISÉES (toutes entités) — la SIGNATURE de souveraineté du front
 * (une colonisation intra-région ne bouge pas l'owner agrégé de région : sans ce compte,
 * le lavis/frontières au grain province ne se rebâtiraient pas). */
int scps_colonized_total(const ScpsSim *s){
    if (!s || !s->ready) return 0;
    int n=0;
    for (int q=0;q<s->sim.econ->n_prov;q++)
        if (s->sim.econ->prov[q].colonized && s->sim.econ->prov[q].owner>=0) n++;
    return n;
}
/* province-CAPITALE d'un pays (-1 si aucune) — le liseré pourpre au grain de la charte. */
int scps_country_capital_province(const ScpsSim *s, int c){
    if (!s || !s->ready || c<0 || c>=s->w->n_countries) return -1;
    int cp = s->w->country[c].capital_prov;
    return (cp>=0 && cp<s->w->n_provinces) ? cp : -1;
}
/* l'âge COURANT (dernier levé, -1 = aucun) + le joueur l'a-t-il engagé + son NOM
 * (mot résolu — membrane). Lecture pure. */
int scps_age_state(ScpsSim *s, int *engaged, char *name, int cap){
    if (engaged) *engaged = 0;
    if (name && cap>0) name[0]='\0';
    if (!s || !s->ready || !s->sim.ev) return -1;
    int age = s->sim.ev->ages.last_dawned;
    if (age < 0) return -1;
    if (engaged) *engaged = (s->sim.player_age_engaged==age) ? 1 : 0;
    if (name && cap>0) snprintf(name, (size_t)cap, "%s", age_name((AgeId)age));
    return age;
}
/* raccord « les 9 citations » — la citation de l'âge COURANT (membrane, mot résolu).
 * age=-1 (aucun âge levé, l'Aube) renvoie sa propre citation ; sinon celle de l'âge
 * (AgeId). Fonction ADDITIVE (n'a pas touché à scps_age_state pour ne pas casser
 * la signature déjà consommée côté binding Godot). */
const char *scps_age_citation(ScpsSim *s, int age){
    if (!s || !s->ready || !s->sim.ev) return age_citation(-1);
    return age_citation(age);
}
/* raccord 6 — le ratio de pays connus [0..1] (le déclencheur des Découvertes,
 * exposé pour un readout : « combien du monde se connaît »). */
float scps_known_pair_share(ScpsSim *s){
    if (!s || !s->ready) return 0.f;
    return ages_known_pair_share(s->w);
}

/* ── ALERTES (les deux voies) ─────────────────────────────────────────────── */
/* VOIE ÉVÈNEMENTS : poll incrémental du fil (membrane : noms résolus, kind brut). */
int scps_feed_poll(ScpsSim *s, int after_seq, ScpsFeedEvent *out, int max){
    if (!s || !s->ready || !out || max<=0) return 0;
    FeedEntry fe[FEED_CAP];
    int n = feed_poll(after_seq, fe, (max<FEED_CAP)?max:FEED_CAP);
    for (int i=0;i<n;i++){
        out[i].seq = fe[i].seq; out[i].year = fe[i].year;
        out[i].kind = fe[i].kind; out[i].region = fe[i].region; out[i].v = fe[i].v;
        out[i].a_id = fe[i].a; out[i].b_id = fe[i].b;
        out[i].a_name = (fe[i].a>=0 && fe[i].a<s->w->n_countries) ? sz(s->w->country[fe[i].a].name) : "";
        out[i].b_name = (fe[i].b>=0 && fe[i].b<s->w->n_countries) ? sz(s->w->country[fe[i].b].name) : "";
        out[i].label  = (fe[i].kind==FEED_DIRECTOR) ? sz(events_name_of(fe[i].v)) : "";
    }
    return n;
}

/* ── MEMBRANE DE DÉCISION — LA FILE JOUEUR ──────────────────────────────────────── */
int scps_pending_count(ScpsSim *s){
    return (s && s->ready) ? pending_event_count(s->sim.ev) : 0;
}
int scps_pending_event(ScpsSim *s, int slot, ScpsPendingEvent *out){
    if (!out) return 0;
    memset(out, 0, sizeof *out);
    out->situation=""; out->region=-1;
    if (!s || !s->ready) return 0;
    PendingEvent pe;
    if (!pending_event_at(s->sim.ev, slot, &pe)) return 0;
    const EventDef *d = event_def(pe.evid);
    if (!d) return 0;
    /* TITRE au NOM RÉEL du lieu, composé À LA LECTURE (un pending s'affiche parfois
     * des mois après son tir) — un buffer STABLE par slot (l'hôte copie aussitôt). */
    { static char title[8][96];
      int ti = (slot>=0 && slot<8) ? slot : 0;
      out->situation = sz(event_title(s->w, pe.evid, pe.subject, title[ti], 96)); }
    out->n_options = d->n_options;
    out->region = (d->scope==EV_PROVINCE) ? pe.subject : -1;
    out->evid = pe.evid;           /* clé d'illustration thématique côté hôte */
    int left = pe.expire_day - s->sim.ev->ages.days_elapsed;
    out->days_left = (left>0) ? left : 0;
    for (int i=0;i<d->n_options && i<4;i++){
        out->labels[i]  = sz(d->options[i].label);
        out->flavors[i] = sz(d->options[i].flavor);
        /* le VISAGE du choix : la faction du hook parle au conseil (membrane : un mot) */
        int fac = d->options[i].hook.faction;
        out->advisors[i] = (fac>=0) ? sz(faction_name(fac)) : "";
        /* l'EFFET MÉCANIQUE en clair (retour joueur : « Ça veut dire quoi ? ») — mots +
         * signes pour les leviers abstraits, CHIFFRES pour le tangible (trésor, pop).
         * Buffers statiques par slot×option (l'hôte copie aussitôt — membrane). */
        {
            static char eb[8][4][144];
            int si = (slot>=0 && slot<8) ? slot : 0;
            char *b = eb[si][i];
            const EvOption *o = &d->options[i];
            int pos = 0, first = 1;
            #define EB_ADD(...) do{ if(pos < 144) \
                pos += snprintf(b+pos, 144-pos, __VA_ARGS__); }while(0)
            #define EB_SEP (first ? (first=0, "") : " · ")
            if (o->eff.d_treasury_mois != 0.f)
                EB_ADD("%strésor %+d %% du revenu mensuel", EB_SEP, (int)(o->eff.d_treasury_mois*100.f + (o->eff.d_treasury_mois>0?0.5f:-0.5f)));
            if (o->eff.d_treasury != 0.f)
                EB_ADD("%strésor %+d or", EB_SEP, (int)o->eff.d_treasury);
            if (o->eff.pop_mult != 0.f && o->eff.pop_mult != 1.f)
                EB_ADD("%spopulation %+d %%", EB_SEP, (int)((o->eff.pop_mult-1.f)*100.f + (o->eff.pop_mult>1.f?0.5f:-0.5f)));
            if (o->eff.d_L > 0.f)         EB_ADD("%slégitimité ↑", EB_SEP);
            else if (o->eff.d_L < 0.f)    EB_ADD("%slégitimité ↓", EB_SEP);
            if (o->eff.d_agitation > 0.f)      EB_ADD("%sagitation ↑", EB_SEP);
            else if (o->eff.d_agitation < 0.f) EB_ADD("%sagitation ↓", EB_SEP);
            if (o->eff.d_K_inst > 0.f)    EB_ADD("%sinstitutions ↑", EB_SEP);
            if (o->eff.d_H_coerc > 0.f)   EB_ADD("%scoercition bâtie ↑", EB_SEP);
            if (o->eff.d_food_cap > 0.f)       EB_ADD("%sfertilité ↑", EB_SEP);
            else if (o->eff.d_food_cap < 0.f)  EB_ADD("%sfertilité ↓", EB_SEP);
            if (o->eff.d_coercion > 0.f)  EB_ADD("%scoercition ↑", EB_SEP);
            if (o->eff.d_influence > 0.f)      EB_ADD("%sinfluence ↑", EB_SEP);
            else if (o->eff.d_influence < 0.f) EB_ADD("%sinfluence ↓", EB_SEP);
            if (o->hook.scar_kind != 0)   EB_ADD("%scicatrice durable", EB_SEP);
            if (o->gamble_p > 0.f)        EB_ADD("%spari (%d %%)", EB_SEP, (int)(o->gamble_p*100.f+0.5f));
            #undef EB_SEP
            #undef EB_ADD
            if (pos == 0) b[0] = '\0';
            out->effets[i] = b;
        }
    }
    return 1;
}
int scps_player_event_choice(ScpsSim *s, int slot, int option){
    if (!s || !s->ready) return 0;
    PendingEvent pe;
    if (!pending_event_at(s->sim.ev, slot, &pe)) return 0;
    const EventDef *d = event_def(pe.evid);
    if (!d || option<0 || option>=d->n_options) return 0;
    PlayerCmd c = { CMD_EVENT_CHOICE, { slot, option, 0, 0 } };
    return sim_cmd_push(&s->sim, c) ? 1 : 0;
}

/* ── LES ANNALES DU RÈGNE — la phrase diégétique par kind ──────────────────────── */
/* Nom d'un pays (membrane) ; "?" hors-borne (comme event_title). */
static const char *annal_country_name(const World *w, int cid){
    return (w && cid>=0 && cid<w->n_countries) ? w->country[cid].name : "?";
}
/* Compose la LIGNE d'une entrée dans un buffer STABLE (motif event_title_ring) —
 * `origin_year` = l'année de l'ANNAL_DILEMME d'origine si connue (-1 sinon), pour
 * la référence causale d'une ANNAL_CICATRICE ("les registres rappellent…"). */
static const char *annal_line(const World *w, const AnnalEntry *e, int origin_year, char *buf, int n){
    char title[96];
    switch ((AnnalKind)e->kind){
        case ANNAL_DILEMME: {
            const EventDef *d = event_def(e->a);
            const char *choix = (d && e->option>=0 && e->option<d->n_options) ? d->options[e->option].label : "?";
            event_title(w, e->a, e->region, title, sizeof title);
            snprintf(buf, (size_t)n, "An %d — %s : vous avez %s.", e->year, title, choix);
            break; }
        case ANNAL_CICATRICE: {
            event_title(w, e->a, e->region, title, sizeof title);
            if (origin_year>=0)
                snprintf(buf, (size_t)n, "An %d — %s ; les registres rappellent votre décision de l'an %d.",
                         e->year, title, origin_year);
            else
                snprintf(buf, (size_t)n, "An %d — %s.", e->year, title);
            break; }
        case ANNAL_AGE:
            snprintf(buf, (size_t)n, "An %d — %s a commencé.", e->year, age_name((AgeId)e->a));
            break;
        case ANNAL_GUERRE_GAGNEE:
            snprintf(buf, (size_t)n, "An %d — La paix est signée avec %s : la victoire est vôtre (score %d).",
                     e->year, annal_country_name(w, e->a), (int)e->b);
            break;
        case ANNAL_GUERRE_PERDUE:
            snprintf(buf, (size_t)n, "An %d — La paix est signée avec %s : la défaite se referme sur vous (score %d).",
                     e->year, annal_country_name(w, e->a), (int)e->b);
            break;
        case ANNAL_SECESSION:
            snprintf(buf, (size_t)n, "An %d — %s est né d'une sécession.", e->year, annal_country_name(w, e->a));
            break;
        case ANNAL_HEGEMON_BRISE:
            snprintf(buf, (size_t)n, "An %d — Un hégémon s'est effondré.", e->year);
            break;
        case ANNAL_MONUMENT:
            snprintf(buf, (size_t)n, "An %d — Le premier grand édifice du règne s'achève.", e->year);
            break;
        case ANNAL_FIN: {
            static const char *FIN_N[]={"","les eaux montantes","le grand hiver","les ronces","l'Ascension"};
            int idx = (e->a>=0 && e->a<5) ? e->a : 0;
            if (e->b==1) snprintf(buf, (size_t)n, "An %d — La Merveille s'achève : votre peuple ASCENSIONNE.", e->year);
            else snprintf(buf, (size_t)n, "An %d — Le monde bascule : %s.", e->year, FIN_N[idx]);
            break; }
        default:
            snprintf(buf, (size_t)n, "An %d — .", e->year);
            break;
    }
    return buf;
}
int scps_annals(ScpsSim *s, ScpsAnnal *out, int max){
    if (!out || max<=0) return 0;
    if (!s || !s->ready || !s->sim.ev) return 0;
    int n = annals_count(s->sim.ev);
    if (n<=0) return 0;
    /* copie + tri par année croissante (insertion — n≤ANNALS_CAP=96, coût négligeable
     * pour une lecture UI occasionnelle ; PAS de qsort à comparateur flottant). */
    AnnalEntry tmp[ANNALS_CAP];
    for (int i=0;i<n && i<ANNALS_CAP;i++) annals_at(s->sim.ev, i, &tmp[i]);
    for (int i=1;i<n;i++){
        AnnalEntry key=tmp[i]; int j=i-1;
        while (j>=0 && tmp[j].year>key.year){ tmp[j+1]=tmp[j]; j--; }
        tmp[j+1]=key;
    }
    static char ring[32][160]; static int head=0;
    int m = (n<max)?n:max; if (m>ANNALS_CAP) m=ANNALS_CAP;
    for (int i=0;i<m;i++){
        const AnnalEntry *e=&tmp[i];
        int origin_year=-1;
        if (e->kind==ANNAL_CICATRICE && e->origin>=0 && e->origin<ANNALS_CAP){
            /* l'origin index vise l'ANNEAU BRUT (pas la copie triée) — on relit
             * directement (le tri de tmp[] ne change pas les index de l'anneau). */
            AnnalEntry o;
            if (annals_at(s->sim.ev, e->origin, &o) && o.kind==ANNAL_DILEMME) origin_year=o.year;
        }
        char *b = ring[head]; head=(head+1)&31;
        out[i].year=e->year; out[i].kind=(int)e->kind; out[i].region=e->region;
        out[i].ligne = sz(annal_line(s->w, e, origin_year, b, 160));
    }
    return m;
}

/* VOIE CONDITIONS : l'état du joueur scanné EN C (le front n'itère pas 800 régions). */
void scps_player_alerts(ScpsSim *s, ScpsPlayerAlerts *out){
    if (!out) return;
    memset(out, 0, sizeof *out);
    out->revolt_region=-1; out->famine_region=-1; out->siege_region=-1;
    out->price_good=-1; out->conso_good=-1;
    out->siege_by=""; out->price_name=""; out->conso_name="";
    if (!s || !s->ready) return;
    int me = s->sim.player;
    if (me<0 || me>=s->w->n_countries) return;
    const WorldEconomy *e = s->sim.econ;
    /* RÉVOLTE qui gronde + FAMINE : pire région à MOI (colonisée) */
    float worst_agit=60.f, worst_food=0.60f;
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++){
        const RegionEconomy *re=&e->region[r];
        if (re->owner!=me || !re->colonized) continue;
        if (s->sim.sc->agitation[r] >= worst_agit){ worst_agit=s->sim.sc->agitation[r]; out->revolt_region=r; }
        if (re->food_sat < worst_food){ worst_food=re->food_sat; out->famine_region=r; }
    }
    if (out->revolt_region>=0) out->revolt_agit=(int)(worst_agit+0.5f);
    if (out->famine_region>=0) out->famine_pct=(int)(worst_food*100.f+0.5f);
    /* SIÈGE ennemi sur MON sol (armée de campagne adverse en phase siège) */
    for (int k=0;k<SCPS_MAX_COUNTRY;k++){
        const FieldArmy *en=&s->sim.camp->army[k];
        if (!en->active || en->phase!=FA_SIEGE || en->owner==me) continue;
        if (en->loc<0 || en->loc>=e->n_regions || e->region[en->loc].owner!=me) continue;
        out->siege_region=en->loc;
        out->siege_by=(en->owner>=0 && en->owner<s->w->n_countries)? sz(s->w->country[en->owner].name):"";
        break;
    }
    /* PRIX EXORBITANT + CONSO INTROUVABLE : lus au marché de la CAPITALE (prix national projeté) */
    int cp = s->w->country[me].capital_prov;
    int capr = (cp>=0 && cp<s->w->n_provinces) ? s->w->province[cp].region : -1;
    if (capr>=0 && capr<e->n_regions){
        const RegionEconomy *re=&e->region[capr];
        float worst_ratio=3.f, worst_lack=1.f;
        for (int g=1; g<RES_COUNT; g++){
            float base=econ_base_price((Resource)g);
            if (base>0.01f && re->price[g] >= base*worst_ratio){
                worst_ratio = re->price[g]/base;
                out->price_good=g; out->price_x10=(int)(worst_ratio*10.f+0.5f);
                out->price_name=sz(resource_name((Resource)g));
            }
            /* un bien DEMANDÉ dont le stock ET l'offre sont ~nuls : le manque vécu */
            if (re->demand[g] > worst_lack && re->stock[g] < re->demand[g]*0.05f
                && re->supply[g] < re->demand[g]*0.10f){
                worst_lack = re->demand[g];
                out->conso_good=g; out->conso_name=sz(resource_name((Resource)g));
            }
        }
    }
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
    /* GRAIN PROVINCE (charte EU4) : la carte montre la VÉRITÉ de propriété — la province
     * COLONISÉE — pas l'agrégat région, qui peignait toute la région-capitale à l'an-0
     * alors qu'UNE seule province est fondée (« on commence avec plusieurs provinces »
     * n'était qu'un artefact d'affichage : le moteur sème bien 1 province). */
    if (!c || c->province<0 || c->province>=s->sim.econ->n_prov || c->province>=SCPS_MAX_PROV) return -1;
    const ProvinceEconomy *pe = &s->sim.econ->prov[c->province];
    int ow = pe->owner;
    return (ow>=0 && ow<s->w->n_countries && pe->colonized) ? ow : -1;
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
            else if (rga!=rgb && rga>=0 && rgb>=0){    lvl=1; owner=own_a; other=own_b; }
            else if (pva!=pvb && pva>=0 && pvb>=0){    lvl=0; owner=own_a; other=own_b; }
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

/* FLAVOR — phrases d'ambiance des 6 héritages (ordre ESOTERIQUE..CLANIQUE, docs/
 * EQUILIBRAGE_CULTURE_FOI_2026-07-10.md §HÉRITAGES, verbatim joueur). */
static const char *HERITAGE_FLAVOR[HERITAGE_COUNT] = {
    "Leurs généalogies commencent avant les premiers calendriers, dans des siècles "
      "dont les ruines seules se souviennent.",
    "Ils disent que tout serment ressemble à un métal : il révèle sa valeur seulement "
      "lorsqu'on le chauffe assez pour le briser.",
    "Leur première horloge mesurait les saisons. La seconde mesura le travail. La "
      "troisième apprit aux deux à rapporter de l'or.",
    "Ils ont porté tant de lois, de langues et de couronnes qu'ils appellent désormais "
      "tradition l'art de changer sans disparaître.",
    "Leurs frontières suivent les canaux, leurs fêtes les moissons et leurs souvenirs "
      "les champs que leurs ancêtres ont refusé d'abandonner.",
    "Un étranger leur demanda où finissait la famille. On lui montra les tombes, les "
      "troupeaux, les guerriers et enfin l'horizon.",
};

int scps_heritage_list(ScpsHeritage *out, int max){
    static char ex[HERITAGE_COUNT][32];   /* ethnonymes-exemples (persistent le temps que l'hôte copie) */
    int n=0;
    for(int h=0; h<HERITAGE_COUNT && n<max; h++){
        culture_make_name(ex[h], (int)sizeof ex[h], (Heritage)h, 1u);
        out[n].id      = h;
        out[n].nom     = heritage_name((Heritage)h);
        out[n].sphere  = sphere_name(heritage_sphere((Heritage)h));
        out[n].exemple = ex[h];
        out[n].flavor  = HERITAGE_FLAVOR[h];
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
    /* FLAVOR — phrases d'ambiance (ordre DOMINATEUR..PACIFISTE, même doc que HERITAGE_FLAVOR). */
    static const char *FLAVOR[ETHOS_COUNT] = {
        "Ils ne demandent pas si la frontière peut être franchie, seulement combien "
          "d'hommes il faudra pour qu'elle cesse d'exister.",
        "Une dette peut être oubliée, une défaite réparée. Une honte, elle, attend "
          "patiemment les petits-fils.",
        "Chaque personne connaît sa place, chaque place son devoir et chaque devoir "
          "le sceau qui le rend incontestable.",
        "Le royaume ne repose pas sur la volonté d'un seul homme, mais sur mille "
          "registres qui refusent obstinément de se contredire.",
        "Ils ne conquièrent pas les ports. Ils y prêtent de l'or jusqu'à ce que les "
          "clés deviennent une modalité de remboursement.",
        "Ils ont juré de ne prendre aucune vie. Leurs voisins débattent encore pour "
          "savoir si cette promesse est une vertu ou une invitation.",
    };
    int n=0;
    for(int e=0; e<ETHOS_COUNT && n<max; e++){
        out[n].id       = e;
        out[n].nom      = ethos_name((Ethos)e);
        out[n].epithete = EPI[e];
        out[n].hint     = HINT[e];
        out[n].flavor   = FLAVOR[e];
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
    float v[9] = { L.demographie, L.rendement, L.influence, L.coercition,
                   L.capacite, L.permeabilite, L.arcane, L.derive, L.fracture };
    /* MEMBRANE (retour joueur 2026-07-10 « je vois de la perméabilité, du K ») : les
     * noms rendus sont des MOTS DE JEU, jamais les leviers du modèle — le créateur
     * garde une table de retraduction locale pour les ANCIENS noms (compat). */
    static const char *NM[9] = {
        "Croissance de la population", "Production", "Rayonnement diplomatique",
        "Coercition", "Capacité de l'État", "Assimilation des minorités",
        "Magie faustienne", "Dérive culturelle", "Fracture" };
    /* RELATIFS (1+x → affichés en %) : demographie(0), rendement(1), derive(7).
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

/* NOM PERSONNALISÉ (créateur d'empire, 2026-07-10) : le joueur nomme son État.
 * Champ d'AFFICHAGE (les noms ne sont jamais hashés — même statut que les noms
 * tribaux WILD) ; Country.name est sérialisé avec le monde, donc le nom choisi
 * SURVIT aux saves. La chronique n'appelle jamais ceci : golden intact. */
void scps_set_country_name(ScpsSim *s, int cid, const char *name){
    if(!s || !s->w || !name || !name[0]) return;
    if(cid < 0 || cid >= s->w->n_countries) return;
    snprintf(s->w->country[cid].name, sizeof s->w->country[cid].name, "%s", name);
}

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
    /* PLAFOND ⌈n_emp/2⌉ sur les RACINES (LOT T) : au-delà, le joueur RALLIE une foi existante (les
     * empires se PARTAGENT les religions) au lieu d'en fonder une nouvelle. */
    if(!religion_can_found(api_count_empires(s))){
        int rid=religion_adopt_existing(cid, (uint32_t)(cid+1));
        if(rid>=0) religion_inherit_regions(s->w, s->sim.econ, cid);
        return rid;
    }
    int trad[3]={t0,t1,t2};
    int rid=religion_spawn(credo, trad, api_capital_cell(s,cid), cid, NULL);
    if(rid<0) return -1;
    religion_set_country(cid, rid);
    religion_inherit_regions(s->w, s->sim.econ, cid);
    return rid;
}
int scps_religion_eligible(ScpsSim *s, int cid){
    if(!s || !s->ready) return 0;
    /* PLAFOND PAR RACINE : pas de schisme si la foi a déjà ses RELIG_SCHISM_MAX sectes (bouton grisé) —
     * la foi en exil PERSISTE alors au lieu d'essaimer une secte de plus. */
    if(!religion_can_schism(religion_of_country(cid))) return 0;
    return (int)religion_schism_eligible(s->w, s->sim.econ, s->sim.wl, cid);
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
/* lot M — le rôle ATTENDU d'un recrutement (miroir READ-ONLY de religion_scholar_recruit :
 * foi d'État → crédo → scholar_role_from_credo), sans rien muter. -1 = pas de foi. */
int scps_religion_scholar_expected(ScpsSim *s, int cid){
    if (!s || !s->ready || cid<0 || cid>=s->w->n_countries) return -1;
    int rid = religion_of_country(cid);
    if (rid<0 || rid>=g_religion_count) return -1;
    return scholar_role_from_credo(g_religions[rid].credo);
}
const char *scps_scholar_role_name(int role){ return sz(scholar_role_name(role)); }
const char *scps_scholar_role_ability(int role){ return sz(scholar_role_ability(role)); }

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
/* LOT T (2026-07-07) — la fondation exigeait le 1er édifice religieux QUELCONQUE (Sanctuaire
 * T1 suffisait) ; elle exige désormais le TEMPLE (T2, cf. mission joueur) — le Sanctuaire
 * seul (T1) ne suffit plus, le Monastère (savoir, foi seulement en à-côté) n'a jamais été
 * le bon signal non plus. Chaîne complète : tech T2 → Temple bâtissable (edifice_tier,
 * agency_build_acct) → Temple BÂTI → fondation. */
int scps_religion_founding_ready(ScpsSim *s, int cid){
    if(!s || !s->ready || cid<0 || cid>=s->w->n_countries) return 0;
    if(religion_of_country(cid) >= 0) return 0;   /* a déjà une foi */
    uint32_t mask = (1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE);
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
    econ_set_human(s->sim.player);
    campaign_set_human(s->sim.player); /* #32 : miroir du load (cf. scps_sim_new) */
    feed_set_focus(s->sim.player);   /* le fil repart, focalisé joueur (RAZ par le load) */
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
