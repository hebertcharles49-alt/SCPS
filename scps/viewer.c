/*
 * viewer.c — visualiseur SDL2 du moteur SCPS
 *
 * Contrôles :
 *   Clic gauche      — sélectionne la province sous la souris
 *   Drag bouton mil. — pan de la caméra
 *   Molette          — zoom centré sur le curseur
 *   TAB / 1-5        — modes de vue
 *   R                — nouvelle graine aléatoire
 *   ESC / Q          — quitte
 *
 * Architecture :
 *   viewer.c  = shell applicatif fin
 *   scps_world  = génération  (indépendant du rendu)
 *   scps_render = rendu       (indépendant de SDL)
 *   → scps_diplo, scps_economy... viendront se brancher sur World
 */
#include <SDL.h>
#include <SDL_ttf.h>
#include "scps_world.h"
#include "scps_render.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"
#include "scps_crypt.h"   /* sauvegardes chiffrées (ChaCha20 + empreinte du clair) */   /* la membrane : viewer ne voit QUE des bandes + mots.
                             * (n'inclut PAS scps_core.h — cloison vérifiée par grep) */
#include "scps_statecraft.h"/* Influence/Opinion/Diplomates : API en ENTIERS de jeu */
#include "scps_agency.h"    /* actions du joueur (file de construction, en JOURS) */
#include "scps_routes.h"
#include "scps_diplo.h"
#include "scps_events.h"    /* chocs, évènements culturels, ÂGES */
#include "scps_modifier.h"  /* pile de dérive démographique */
#include "scps_demography.h"/* GROUPES par province (composition) + H/intégration */
#include "scps_labor.h"     /* topbar : Or / Nourriture / Matériaux */
#include "scps_ai.h"        /* les voisins VIVENT : lecteurs de coordonnées, mêmes leviers */
#include "scps_revolt.h"    /* la révolte INCARNÉE : sécessions/coups dans le jeu vivant */
#include "scps_intertrade.h"/* commerce inter-pays : grandes routes marchandes + embargo */
#include "scps_credit.h"    /* dette & prêts (incrément 1) : créancier par pays, save/load */
#include "scps_warhost.h"   /* les armées VIVENT : mobilisation par pays */
#include "scps_campaign.h"  /* … et MARCHENT : campagne sur la carte (marche/siège/bataille) */
#include "scps_missions.h"  /* missions décennales : rythme + injection de ressources */
#include "scps_navy.h"     /* la flotte (mer §5) : coques, chantier, entretien, outre-mer */
#include "scps_tune.h"     /* P0-3 : empreinte des surcharges SCPS_TUNE dans la save (tune_active_string) */
#include "scps_endgame.h"  /* capstone §27 : entropie + 4 fins + merveille (moteur, pas scps_core) */
#include "scps_save_io.h"  /* écriture ATOMIQUE du slot (write-then-rename) : un crash ne corrompt pas la sauvegarde existante */
#include "scps_lang.h"     /* la table de chaînes : tout mot face-joueur vient des tables */
#include "scps_map_dressing.h"  /* pack MAP DRESSING : décors de carte (champs, bâtiments, arbres, roches, buissons, rivières, routes) — display-only */
/* L'atlas chargé (NULL = absent → carte lisse). Display-only, même régime éditable que scps_lang.txt. */
static SDL_Texture *g_dress_tex = NULL;  /* atlas de dressing (décors de carte) — NULL si le .bmp est absent */
static SDL_Texture *g_settle_tex = NULL; /* atlas SETTLEMENTS (villes par tier × terrain) — NULL si absent */
#define SCPS_SETTLE_FILE "scps_map_settlements.bmp"
#define SCPS_SETTLE_CELL 96               /* atlas 6 tiers × 6 groupes, cellule 96 px */
enum { SETTLE_MOUNTAIN=0, SETTLE_RIVER, SETTLE_ESTUARY, SETTLE_RURAL, SETTLE_MARKET, SETTLE_FORTIFIED };
/* Charge un atlas BMP à fond MAGENTA et le DESPILLE (magenta → alpha, frange → bronze
 * neutre). Mutualisé décors & settlements. NULL si le fichier est absent. */
static SDL_Texture *load_despilled_bmp(SDL_Renderer *ren, const char *file){
    SDL_Surface *ns = SDL_LoadBMP(file);
    if (!ns) return NULL;
    SDL_Surface *cv = SDL_ConvertSurfaceFormat(ns, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(ns);
    if (!cv) return NULL;
    Uint32 clear = SDL_MapRGBA(cv->format, 0,0,0,0);
    for (int y=0; y<cv->h; y++){
        Uint32 *row = (Uint32*)((Uint8*)cv->pixels + (size_t)y*cv->pitch);
        for (int x=0; x<cv->w; x++){
            Uint8 r,g,b,a; SDL_GetRGBA(row[x], cv->format, &r,&g,&b,&a);
            int mn=(r<b)?r:b; int key=mn-(int)g;
            if (key<=2) continue;                            /* couvre PLUS LARGE (frange ≥3, ex-≥5) */
            float mness=(float)key/255.0f; float af=1.0f-mness; af*=af;
            if (af<0.03f){ row[x]=clear; continue; }
            /* frange magenta → NEUTRE SOMBRE (gris→noir, AUCUNE chaleur) : le liseré de CHAQUE
             * sprite ne tire plus au ROSÉ. Base = le VERT (seul canal NON contaminé par le
             * magenta) ⇒ un contour d'encre sombre, jamais un halo chaud. */
            int v=(int)((float)g*0.26f+0.5f);
            row[x]=SDL_MapRGBA(cv->format,(Uint8)v,(Uint8)v,(Uint8)v,(Uint8)(af*255.0f+0.5f));
        }
    }
    /* POST-TRAITEMENT anti-ROSE — le résidu SALMON (corps du sprite). Spectre ÉTROIT (ne pas
     * abîmer les autres assets) : R nettement au-dessus du VERT (chaud) ET B encore PRÈS du
     * vert (B>V−6) — signature du résidu de magenta (le bleu du fond a contaminé le pixel).
     * L'orange/brun/sable LÉGITIME a B BIEN sous V (non touché) ; la MER a R bas (non touchée).
     * DÉSATURÉ en GRIS à luminance ÉGALE (aucune chaleur résiduelle, pas de rosé). */
    for (int y=0; y<cv->h; y++){
        Uint32 *row = (Uint32*)((Uint8*)cv->pixels + (size_t)y*cv->pitch);
        for (int x=0; x<cv->w; x++){
            Uint8 r,g,b,a; SDL_GetRGBA(row[x], cv->format, &r,&g,&b,&a);
            if (a>0 && (int)r>(int)g+6 && (int)b>(int)g-6){
                int v=((int)r*5 + (int)g*9 + (int)b*2)/16;   /* luminance perçue → GRIS neutre de même éclat */
                row[x]=SDL_MapRGBA(cv->format,(Uint8)v,(Uint8)v,(Uint8)v, a);
            }
        }
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, cv);
    SDL_FreeSurface(cv);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}
static SDL_Texture *g_cover_tex = NULL;   /* route-cover : MOBILIER de route (bornes, haies, murets, bottes, rochers…) */
static SDL_Texture *g_softblob_tex = NULL;/* glow radial DOUX (blanc, alpha en cloche) — teinté à la volée pour le raccord mer */
/* Construit une fois un disque à alpha en CLOCHE (1 au centre → 0 au bord, sans bord franc).
 * Teinté/écrasé à la volée (color/alpha-mod), il sert d'apron de mer FONDU sous les villes
 * côtières — un quad à dégradé linéaire faisait un « carré d'eau » ; ce disque n'a aucun bord. */
static void softblob_build(SDL_Renderer *ren){
    const int S=96; const float c=(float)(S-1)*0.5f;
    SDL_Surface *sf=SDL_CreateRGBSurfaceWithFormat(0,S,S,32,SDL_PIXELFORMAT_ARGB8888);
    if(!sf) return;
    for(int y=0;y<S;y++){
        Uint32 *row=(Uint32*)((Uint8*)sf->pixels+(size_t)y*sf->pitch);
        for(int x=0;x<S;x++){
            float dx=((float)x-c)/c, dy=((float)y-c)/c; float d=sqrtf(dx*dx+dy*dy);
            float a=1.0f-d; if(a<0.f)a=0.f; a*=a;                 /* cloche : 0 au bord, douce */
            row[x]=SDL_MapRGBA(sf->format,255,255,255,(Uint8)(a*255.0f));
        }
    }
    g_softblob_tex=SDL_CreateTextureFromSurface(ren,sf);
    SDL_FreeSurface(sf);
    if(g_softblob_tex) SDL_SetTextureBlendMode(g_softblob_tex,SDL_BLENDMODE_BLEND);
}
static SDL_Texture *g_port_tex = NULL;    /* ports ORIENTÉS : ville côtière dont les quais SUIVENT la mer */
#define SCPS_PORT_FILE "scps_port_orientation.bmp"
#define SCPS_PORT_CELL 96                 /* atlas 8 col × 16 lignes (0-7 front · 8-15 vue de dos), 96 px */
#define SCPS_PORT_COLS 8
/* type de port par LIGNE (front_side 0-7 ; +8 = la même vue de DOS) */
enum { PORT_FISHING=0, PORT_TRADE_QUAY=1, PORT_TOWN_HARBOR=2, PORT_FORTIFIED=3, PORT_ESTUARY_DOCK=4 };
#define SCPS_COVER_FILE "scps_route_cover.bmp"
#define SCPS_COVER_CELL 128               /* atlas 8 col × 8 lignes, cellule 128 px */
#define SCPS_COVER_COLS 8
enum { COVER_HEDGE_CLUMP=31, COVER_BOULDER=32, COVER_ROCKPILE=33, COVER_BUSH=34, COVER_REED=35,
       COVER_HAYSTACK=37, COVER_CRATE=38, COVER_MILESTONE=48, COVER_SIGNPOST=49, COVER_WAYSTONE=50 };
static void cover_blit(SDL_Renderer *ren, int id, int x, int y, int px){
    if (!g_cover_tex || id<0) return;
    SDL_Rect src={ (id%SCPS_COVER_COLS)*SCPS_COVER_CELL, (id/SCPS_COVER_COLS)*SCPS_COVER_CELL, SCPS_COVER_CELL, SCPS_COVER_CELL };
    SDL_Rect dst={ x, y, px, px };
    SDL_RenderCopy(ren, g_cover_tex, &src, &dst);
}
/* ═══ ANIMATIONS FX (display-only) ══════════════════════════════════════════
 * Quatre planches à fond MAGENTA (FF00FF → alpha), cadencées par SDL_GetTicks
 * (horloge MUR, JAMAIS le moteur → le déterminisme reste intact, comme le dressing).
 * Absentes ⇒ texture NULL ⇒ no-op (la carte reste statique). Même régime éditable
 * que les autres .bmp : on les dépose à côté du binaire. */
static SDL_Texture *g_fx_sea_tex    = NULL;  /* houle : 8 frames × 2 lignes (calme/vive), 128 px */
static SDL_Texture *g_fx_coast_tex  = NULL;  /* écume de rive : 6 frames, 128 px */
static SDL_Texture *g_fx_army_tex   = NULL;  /* armée en campagne : 4 frames × 2 lignes (marche/assaut), 96 px */
static SDL_Texture *g_fx_vortex_tex = NULL;  /* vortex de la fin EAU : 2 calques contrarotatifs, 256 px */
#define SCPS_FX_SEA_FILE     "scps_fx_sea.bmp"
#define SCPS_FX_COAST_FILE   "scps_fx_coast.bmp"
#define SCPS_FX_ARMY_FILE    "scps_fx_army.bmp"
#define SCPS_FX_VORTEX_FILE  "scps_fx_vortex.bmp"
#define SCPS_FX_SEA_CELL     128
#define SCPS_FX_SEA_FRAMES   8
#define SCPS_FX_COAST_CELL   128
#define SCPS_FX_COAST_FRAMES 6
#define SCPS_FX_ARMY_CELL    96
#define SCPS_FX_ARMY_FRAMES  4
#define SCPS_FX_VORTEX_CELL  256
/* écume : POUSSÉE vers la mer (en cellules-monde) pour quitter la plage — le « radius
 * neutre » qui pose l'ourlet sur la LIGNE D'EAU, pas sur le sable ; et l'orientation de
 * BASE de la planche (la crête d'écume regarde le BAS = sud = 180° boussole). */
#define SCPS_FX_COAST_PUSH   0.85f
#define SCPS_FX_COAST_FACE_DEG 180.0f   /* la planche a la MER en HAUT (eau vide au-dessus de la crête) → base 180° */
/* Indice de frame d'une animation à n images, period_ms par image (horloge mur). */
static int fx_frame(int nframes, int period_ms){
    if (nframes<1) nframes=1;
    if (period_ms<1) period_ms=1;
    return (int)((SDL_GetTicks()/(Uint32)period_ms) % (Uint32)nframes);
}
/* Charge un BMP FX à fond MAGENTA → alpha SANS le despill agressif des atlas de
 * carte (qui désaturerait un vortex violet ou une écume bleutée) : on ne touche
 * QUE le magenta, le reste passe VERBATIM (la teinte FX est préservée). */
static SDL_Texture *load_fx_bmp(SDL_Renderer *ren, const char *file){
    SDL_Surface *ns = SDL_LoadBMP(file);
    if (!ns) return NULL;
    SDL_Surface *cv = SDL_ConvertSurfaceFormat(ns, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(ns);
    if (!cv) return NULL;
    Uint32 clear = SDL_MapRGBA(cv->format, 0,0,0,0);
    for (int y=0; y<cv->h; y++){
        Uint32 *row=(Uint32*)((Uint8*)cv->pixels+(size_t)y*cv->pitch);
        for (int x=0; x<cv->w; x++){
            Uint8 r,g,b,a; SDL_GetRGBA(row[x],cv->format,&r,&g,&b,&a);
            int mn=(r<b)?r:b; int key=mn-(int)g;             /* magenta-ness : R&B hauts, G bas */
            if (key<=2) continue;                             /* pas magenta → VERBATIM */
            float mness=(float)key/255.0f; float af=1.0f-mness; af*=af;
            if (af<0.03f){ row[x]=clear; continue; }           /* cœur magenta → transparent */
            row[x]=SDL_MapRGBA(cv->format,r,g,b,(Uint8)(af*255.0f+0.5f));  /* frange : alpha partiel, teinte gardée */
        }
    }
    SDL_Texture *tex=SDL_CreateTextureFromSurface(ren,cv);
    SDL_FreeSurface(cv);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}
#include "stb_image_write.h"  /* F12 : capture d'écran PNG (vendoré) */
#include "scps_audio.h"       /* la prise audio (miniaudio) — preuve de vie sur alerte */
#ifdef SCPS_DEV
#include "dev_overlay.h"      /* §6 : l'inspecteur de coordonnées brutes (F3) — JAMAIS en release */
static bool g_dev_overlay = false;
#endif
#include "scps_factions.h"  /* §4 : leviers de factions (reset/decay par sim) */
#include <stdlib.h>
/* mkdir portable (la sauvegarde crée saves/ sans passer par system(), qui
 * échouait sous Windows : cmd.exe ne connaît pas `mkdir -p`). */
#ifdef _WIN32
# include <direct.h>
# define scps_mkdir(p) _mkdir(p)
#else
# include <sys/stat.h>
# define scps_mkdir(p) mkdir((p), 0755)
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ---- Configuration fenêtre ------------------------------------------- */
#define WIN_W 1200
#define WIN_H 1080

/* ---- Temps de jeu : du snapshot au JEU VIVANT ------------------------ */
typedef enum { SPEED_PAUSE=0, SPEED_1, SPEED_2, SPEED_3, SPEED_5, SPEED_COUNT } GameSpeed;
static const double DAYS_PER_SEC[SPEED_COUNT] = { 0.0, 3.0, 8.0, 14.0, 24.0 };
static const char  *SPEED_LABEL[SPEED_COUNT]  = { "❙❙", "▸ ×1", "▸▸ ×2", "▸▸ ×3", "▸▸▸ ×5" };
static SDL_Rect g_speed_zone;   /* le cran est CLIQUABLE (clic = cran suivant, repasse à ×1) */
static GameSpeed g_last_speed = SPEED_1;   /* Espace repart sur le DERNIER cran choisi */
static SpeciesArchetype g_player_race = RACE_HUMAIN;   /* le choix de l'écran de création (shell) */
#define GAME_YEARS 250

/* ---- État caméra ----------------------------------------------------- */
typedef struct {
    float ox, oy;    /* offset en cellules */
    float scale;     /* pixels par cellule */
} Cam;

/* ---- VUE ISOMÉTRIQUE (display-only) ---------------------------------- *
 * Toggle (touche I). Le RENDERER incline le terrain (render_map, p->iso) ; ici on
 * projette les SURCOUCHES (décors, bordures, labels, marqueurs) avec les MÊMES
 * facteurs ISO_KX/KY autour du centre fenêtre, et on inverse pour le picking. Les
 * dims fenêtre du pivot sont rafraîchies par frame (g_iso_w/h). */
static int g_iso=0, g_iso_w=0, g_iso_h=0;
static void cam_project(const Cam *cam, float wx, float wy, float *osx, float *osy){
    float fx=(wx-cam->ox)*cam->scale, fy=(wy-cam->oy)*cam->scale;   /* écran « plat » */
    if (!g_iso){ *osx=fx; *osy=fy; return; }
    float px=g_iso_w*0.5f, py=g_iso_h*0.5f, dx=fx-px, dy=fy-py;
    *osx = px + (dx - dy)*ISO_KX;          /* rotation 45° */
    *osy = py + (dx + dy)*ISO_KY;          /* + écrasement vertical (2:1) */
}
static void cam_unproject(const Cam *cam, float sx, float sy, float *owx, float *owy){
    float fx=sx, fy=sy;
    if (g_iso){
        float px=g_iso_w*0.5f, py=g_iso_h*0.5f, a=(sx-px)/ISO_KX, b=(sy-py)/ISO_KY;
        fx = px + (a+b)*0.5f; fy = py + (b-a)*0.5f;
    }
    *owx = fx/cam->scale + cam->ox; *owy = fy/cam->scale + cam->oy;
}

static void cam_zoom(Cam *c, float factor, float screen_x, float screen_y) {
    /* Zoom centré sur le point écran (screen_x, screen_y) */
    float wx = screen_x / c->scale + c->ox;
    float wy = screen_y / c->scale + c->oy;
    c->scale *= factor;
    if (c->scale < 0.20f) c->scale = 0.20f;
    if (c->scale > 16.0f) c->scale = 16.0f;
    c->ox = wx - screen_x / c->scale;
    c->oy = wy - screen_y / c->scale;
}

static void cam_pan(Cam *c, float dpx, float dpy) {
    c->ox -= dpx / c->scale;
    c->oy -= dpy / c->scale;
}

static void cam_fit(Cam *c, int win_w, int win_h) {
    /* Ajuste pour montrer toute la carte dans la fenêtre */
    float sx = (float)win_w / SCPS_W;
    float sy = (float)win_h / SCPS_H;
    c->scale = (sx < sy) ? sx : sy;
    c->ox = (SCPS_W - win_w / c->scale) * 0.5f;
    c->oy = (SCPS_H - win_h / c->scale) * 0.5f;
}

/* ---- Pixel buffer ---------------------------------------------------- */
typedef struct {
    SDL_Texture *tex;
    uint32_t    *pixels;
    int          w, h;
} PixBuf;

static PixBuf pixbuf_create(SDL_Renderer *ren, int w, int h) {
    PixBuf pb;
    pb.w = w; pb.h = h;
    pb.tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING, w, h);
    pb.pixels = (uint32_t*)malloc((size_t)(w*h)*4);
    return pb;
}
static void pixbuf_destroy(PixBuf *pb) {
    SDL_DestroyTexture(pb->tex);
    free(pb->pixels);
    pb->tex = NULL; pb->pixels = NULL;
}
static void pixbuf_upload(PixBuf *pb) {
    SDL_UpdateTexture(pb->tex, NULL, pb->pixels, pb->w * 4);
}

/* ═══ DÉCORS DE CARTE (pack MAP DRESSING — display-only) ════════════════════
 * Une couche de détail pixel-art (112 sprites, 7 familles : champs · bâtiments ·
 * arbres · roches · buissons · rivières · routes), semée DÉTERMINISTE par
 * coordonnée MONDE (jamais de hasard runtime → stable au pan/zoom/save), posée
 * SOUS les frontières/labels/marqueurs. Gate au zoom : rien au dézoom
 * (sous-pixel). No-op si l'atlas est absent (g_dress_tex NULL). */
static inline uint32_t map_hash(int x, int y, uint32_t salt){
    uint32_t h=(uint32_t)x*0x8da6b343u ^ (uint32_t)y*0xd8163841u ^ salt;
    h^=h>>15; h*=0x2c1b3c6du; h^=h>>12; h*=0x297a2d39u; h^=h>>15;
    return h;
}
static inline void dress_blit(SDL_Renderer *ren, int id, int x, int y, int px){
    if (!g_dress_tex || id<0) return;
    SDL_Rect src = { MAP_DRESSING_X(id), MAP_DRESSING_Y(id), SCPS_MAP_DRESSING_CELL, SCPS_MAP_DRESSING_CELL };
    SDL_Rect dst = { x, y, px, px };
    SDL_RenderCopy(ren, g_dress_tex, &src, &dst);
}
/* biome (+ température pour la variante froide, + hash pour la diversité)
 * → id de sprite de dressing, ou -1 si la cellule ne porte pas de décor. */
static int dress_pick(const Cell *c, uint32_t h){
    int v = (int)(h & 3u);
    switch (c->biome){
        case BIO_FOREST:
            if (c->temperature < 0.30f) return (h&1u)?MAPD_TREE_SNOW_CONIFER:MAPD_TREES_CONIFER_CLUSTER;
            return v==0?MAPD_TREE_OAK_LARGE:v==1?MAPD_TREES_BROADLEAF_CLUSTER
                  :v==2?MAPD_FOREST_CLUMP_DARK:MAPD_TREE_BROADLEAF;
        case BIO_WOODS:
            return v==0?MAPD_TREE_BROADLEAF:v==1?MAPD_GROVE_THIN
                  :v==2?MAPD_TREES_BIRCH:MAPD_BUSH_GREEN;
        case BIO_JUNGLE:
            return v<2?MAPD_GROVE_MIXED:v==2?MAPD_FOREST_CLUMP_DARK:MAPD_TREE_POPLAR;
        case BIO_FARMLAND:
            return v==0?MAPD_FIELD_WHEAT:v==1?MAPD_FIELD_TILLED
                  :v==2?MAPD_FIELD_ORCHARD:MAPD_FIELD_VEG_GARDEN;
        case BIO_PLAINS: case BIO_GRASSLAND:
            return v==0?MAPD_FIELD_HAY:v==1?MAPD_FIELD_PASTURE:v==2?MAPD_BUSH_GREEN:-1;
        case BIO_STEPPE: case BIO_SAVANNA:
            return v==0?MAPD_SCRUB_DRY:v==1?MAPD_HEATHER:-1;   /* clairsemé */
        case BIO_DRYLANDS: case BIO_COASTAL_DESERT:
            return v==0?MAPD_DRY_THORN:v==1?MAPD_TREE_DRY_SCRUB:-1;
        case BIO_DESERT:     return v==0?MAPD_ROCKS_RED_SANDSTONE:-1;
        case BIO_MARSH:
            return v==0?MAPD_REEDS_CLUMP:v==1?MAPD_YELLOW_REEDS
                  :v==2?MAPD_MARSH_GRASSES:MAPD_LILYPAD;
        case BIO_BOG:
            return v==0?MAPD_WETLAND_SEDGE:v==1?MAPD_REEDS_CLUMP:MAPD_THICKET_LOW;
        case BIO_MANGROVE:   return v<2?MAPD_TREE_POPLAR:MAPD_WETLAND_SEDGE;
        case BIO_HILLS:      return v==0?MAPD_ROCK_BOULDER:v==1?MAPD_ROCKS_PAIR:v==2?MAPD_HEATHER:-1;
        case BIO_HIGHLANDS:
            return v==0?MAPD_ROCKS_RIDGE:v==1?MAPD_ROCKS_MOSSY:v==2?MAPD_HEATHER:-1;
        case BIO_MOUNTAINS:
            return v<2?MAPD_ROCKS_RIDGE:v==2?MAPD_ROCK_CLIFF_NUB:MAPD_ROCKS_SCREE;
        case BIO_PEAK:       return (h&1u)?MAPD_ROCKS_SNOW_CAPPED:MAPD_ROCK_CLIFF_NUB;
        case BIO_GLACIER:    return (h&1u)?MAPD_ROCKS_SNOW_CAPPED:MAPD_TREE_SNOW_CONIFER;
        case BIO_VOLCANO:    return (h&1u)?MAPD_ROCKS_BASALT:MAPD_ROCKS_RED_SANDSTONE;
        default:             return -1;
    }
}
/* taille du décor en pixels ~ FAMILLE × zoom (arbres GRANDS — canopée qui se
 * chevauche ; buissons petits). famille = id<56 ? id/8 : (id-56)/8. */
static int dress_size(int id, float sc){
    int fam = (id < 56) ? id/8 : (id-56)/8;   /* 0 champs · 1 bâti · 2 arbres · 3 roches · 4 buissons · 5 rivières · 6 routes */
    float k;
    switch (fam){
        case 2:  k = 3.6f; break;   /* arbres */
        case 0:  k = 3.0f; break;   /* champs */
        case 3:  k = 3.0f; break;   /* roches */
        case 1:  k = 2.8f; break;   /* bâtiments isolés */
        case 5:  k = 2.6f; break;   /* rivières */
        case 6:  k = 2.6f; break;   /* routes */
        default: k = 2.4f; break;   /* buissons / végétation basse */
    }
    int px=(int)(sc*k);
    if (px < 10) px = 10;
    if (px > 110) px = 110;
    return px;
}
/* densité par biome : combien de cellules-candidates sur 16 portent un décor
 * (forêts/montagnes/marais ≈ couverture pleine ; plaines clairsemées). */
static int dress_density(Biome b){
    switch (b){
        case BIO_FOREST: case BIO_JUNGLE:                      return 8;
        case BIO_MOUNTAINS: case BIO_PEAK: case BIO_GLACIER:   return 8;
        case BIO_MARSH: case BIO_BOG:                          return 7;
        case BIO_WOODS: case BIO_MANGROVE:                     return 6;
        case BIO_VOLCANO:                                      return 6;
        case BIO_HILLS: case BIO_HIGHLANDS:                    return 5;
        case BIO_FARMLAND:                                     return 2;   /* champ PONCTUEL (mais habillé en ferme) */
        case BIO_PLAINS: case BIO_GRASSLAND:
        case BIO_STEPPE: case BIO_SAVANNA:                     return 3;
        case BIO_DRYLANDS: case BIO_COASTAL_DESERT:            return 2;
        case BIO_DESERT:                                       return 1;
        default:                                               return 0;
    }
}
/* La passe : iter cellules visibles (lattice monde, pas adaptatif → densité
 * écran ~constante et compte borné), décide par biome via hash. Rangées
 * top→bottom = dessin ARRIÈRE→AVANT (les décors bas-ancrés se recouvrent en
 * canopée). Appelée APRÈS le terrain, AVANT les frontières/labels. La MER reste
 * NUE : le pack est terrestre (plus de couche d'écume). */
/* bâtiment isolé (« ville ») selon le CONTEXTE — côte, relief, forêt, fleuve, tier de
 * pop. Les hameaux suivent ainsi la géographie ET le peuplement (membrane : on lit
 * colonized + pop, des nombres tangibles). */
static int dress_building(const Cell *c, int tier, uint32_t h){
    int v=(int)((h>>5)&3u);
    if (c->coast) return (v&1)?MAPD_BLD_FISHING_HUT:MAPD_BLD_BOATHOUSE;
    switch(c->biome){
        case BIO_MOUNTAINS: case BIO_HILLS: case BIO_HIGHLANDS: case BIO_PEAK:
            return (v&1)?MAPD_BLD_MINE_SHACK:MAPD_BLD_QUARRY_SHED;
        case BIO_FOREST: case BIO_WOODS: case BIO_JUNGLE:
            return (v&1)?MAPD_BLD_LUMBER_CAMP:MAPD_BLD_CHARCOAL_KILN;
        default: break;
    }
    if (c->river>60) return MAPD_BLD_WATERMILL;
    if (tier>=2) return (v==0)?MAPD_BLD_CHAPEL:(v==1)?MAPD_BLD_STOREHOUSE:(v==2)?MAPD_BLD_WINDMILL:MAPD_BLD_BARN;
    return (v==0)?MAPD_BLD_ROUND_HUT:(v==1)?MAPD_BLD_COTTAGE:(v==2)?MAPD_BLD_BARN:MAPD_BLD_SHEPHERD;
}
/* L'OUTIL DE ROTATION : blit d'un sprite TOURNÉ de `ang` degrés autour de son centre.
 * Les décals PLATS (rivières, routes) suivent leur direction par rotation ; les sprites
 * DEBOUT (arbres, bâtiments) ne tournent pas. */
static void dress_blit_rot(SDL_Renderer *ren, int id, float scx, float scy, int px, double ang){
    if (!g_dress_tex || id<0) return;
    SDL_Rect src = { MAP_DRESSING_X(id), MAP_DRESSING_Y(id), SCPS_MAP_DRESSING_CELL, SCPS_MAP_DRESSING_CELL };
    SDL_Rect dst = { (int)(scx-px*0.5f), (int)(scy-px*0.5f), px, px };
    SDL_RenderCopyEx(ren, g_dress_tex, &src, &dst, ang, NULL, SDL_FLIP_NONE);
}
#define RAD2DEG 57.29577951308232
/* angle ÉCRAN (deg) du segment monde (ax,ay)->(bx,by) sous la projection courante. */
static double seg_screen_ang(const Cam *cam, float ax, float ay, float bx, float by){
    float p0x,p0y,p1x,p1y; cam_project(cam,ax,ay,&p0x,&p0y); cam_project(cam,bx,by,&p1x,&p1y);
    return atan2((double)(p1y-p0y),(double)(p1x-p0x))*RAD2DEG;
}
/* angle (deg) de l'axe « route/rivière » DESSINÉ dans le sprite iso droit. MESURÉ par
 * PCA sur l'atlas : ≈ −26° en espace écran (le sprite va de bas-gauche vers haut-droite).
 * On retire cet angle et on ajoute la direction écran courante du segment. */
#define SEG_SPRITE_ANG0 (-26.0)
/* CHAÎNAGE des rivières : on suit les VRAIS tracés (w->river[].x/y, polylignes) — pas
 * le champ de débit (trop large). À chaque cellule, l'orientation vient des pas
 * AMONT→AVAL (voisin précédent + suivant) → sprite iso droit/coude, flip H/V. SOUS les décors. */
static void draw_map_rivers(SDL_Renderer *ren, const World *w, const Cam *cam, int win_w, int win_h){
    if (!g_dress_tex) return;
    float sc=cam->scale; if (sc<3.0f) return;
    g_iso_w=win_w; g_iso_h=win_h;
    int px=(int)(sc*2.6f);
    if(px<10)px=10;
    if(px>130)px=130;
    SDL_SetTextureAlphaMod(g_dress_tex, 230);
    for (int ri=0; ri<w->n_rivers && ri<SCPS_MAX_RIVERS; ri++){
        const River *rv=&w->river[ri];
        for (int k=0; k<rv->len; k++){
            int cx=rv->x[k], cy=rv->y[k];
            if (cx<0||cy<0||cx>=SCPS_W||cy>=SCPS_H) continue;
            /* direction SORTANTE (on re-pick à chaque changement d'orientation, pas de lissage) */
            int ax,ay,bx,by;
            if      (k<rv->len-1){ ax=cx; ay=cy; bx=rv->x[k+1]; by=rv->y[k+1]; }
            else if (k>0)        { ax=rv->x[k-1]; ay=rv->y[k-1]; bx=cx; by=cy; }
            else                 { ax=cx; ay=cy; bx=cx+1; by=cy; }
            float fsx,fsy; cam_project(cam,(float)cx+0.5f,(float)cy+0.5f,&fsx,&fsy);
            if (fsx<-px||fsx>win_w+px||fsy<-px||fsy>win_h+px) continue;   /* hors champ */
            double theta = seg_screen_ang(cam,(float)ax+0.5f,(float)ay+0.5f,(float)bx+0.5f,(float)by+0.5f);
            double ang = theta - SEG_SPRITE_ANG0;   /* axe sprite mesuré (−26°) → suit la direction */
            dress_blit_rot(ren,MAPD_RIVER_STRAIGHT,fsx,fsy,px,ang);   /* TOURNÉ pour suivre le fil */
        }
    }
    SDL_SetTextureAlphaMod(g_dress_tex, 255);
}
static bool region_world_pos(const World *w, int reg, float *wx, float *wy){
    if (reg<0||reg>=w->n_regions) return false;
    const Region *R=&w->region[reg];
    long ax=0,ay=0; int n=0;
    for (int k=0;k<R->n_provinces && k<12;k++){
        int pid=R->province_ids[k];
        if (pid<0||pid>=w->n_provinces) continue;
        ax+=w->province[pid].seed_x; ay+=w->province[pid].seed_y; n++;
    }
    if (n==0) return false;
    *wx=(float)ax/n; *wy=(float)ay/n; return true;
}
/* ── ROUTES TERRAIN-AWARE : A* sur la grille, coût = RELIEF (height + biome) ; mer/lac
 * infranchissable, fleuve franchi par un PONT. Chemins MIS EN CACHE (recalcul si le réseau
 * change). Rendu LISSÉ (tangente fenêtrée) → plus de double-variation aux diagonales. ── */
#define ROAD_PATH_MAX 1400
typedef struct { int16_t x[ROAD_PATH_MAX], y[ROAD_PATH_MAX]; int len; int ra, rb; } RoadPath;
static RoadPath *g_rd_paths=NULL; static int g_rd_npaths=0; static uint64_t g_rd_sig=0;
static uint8_t *g_road_mask=NULL;   /* 1 = cellule sur/à côté d'une route (les champs l'évitent) */
static uint8_t *g_clear_mask=NULL;  /* 1 = cellule DÉBOISÉE (rayon autour d'une ville) → pas de canopée */
static float *g_rd_g=NULL, *g_heapf=NULL; static int *g_rd_from=NULL,*g_rd_gen=NULL,*g_rd_closed=NULL,*g_heapi=NULL;
static int g_rd_curgen=0, g_heap_n=0;
static void rheap_push(float f,int idx){ int i=g_heap_n++; g_heapf[i]=f; g_heapi[i]=idx;
    while(i>0){ int p=(i-1)/2; if(g_heapf[p]<=g_heapf[i])break;
        float tf=g_heapf[p];g_heapf[p]=g_heapf[i];g_heapf[i]=tf; int ti=g_heapi[p];g_heapi[p]=g_heapi[i];g_heapi[i]=ti; i=p; } }
static int rheap_pop(void){ int r=g_heapi[0],n=--g_heap_n; g_heapf[0]=g_heapf[n]; g_heapi[0]=g_heapi[n]; int i=0;
    for(;;){ int l=2*i+1,rr=2*i+2,s=i; if(l<n&&g_heapf[l]<g_heapf[s])s=l; if(rr<n&&g_heapf[rr]<g_heapf[s])s=rr; if(s==i)break;
        float tf=g_heapf[s];g_heapf[s]=g_heapf[i];g_heapf[i]=tf; int ti=g_heapi[s];g_heapi[s]=g_heapi[i];g_heapi[i]=ti; i=s; } return r; }
static float road_cell_cost(const World *w,int x,int y){
    const Cell *c=scps_cellc(w,x,y);
    if (c->sea || c->lake) return -1.f;            /* mer/lac : on contourne (sauf pont au fleuve) */
    float cost=1.f + c->height*7.f;                /* plat facile, on longe le BAS (height ∈ [0..1]) */
    if (c->biome==BIO_FOREST||c->biome==BIO_WOODS) cost+=2.5f;   /* forêt : plus cher */
    if (c->biome==BIO_JUNGLE)    cost+=5.f;
    if (c->biome==BIO_HILLS||c->biome==BIO_HIGHLANDS) cost+=5.f;
    if (c->biome==BIO_MOUNTAINS) cost+=16.f;       /* montagne : très cher */
    if (c->biome==BIO_PEAK)      cost+=45.f;
    if (c->river>40)             cost+=7.f;        /* franchir un fleuve = un PONT (cher mais permis) */
    return cost;
}
static bool road_astar(const World *w,int ax,int ay,int bx,int by,RoadPath *out){
    if(ax<0||ay<0||ax>=SCPS_W||ay>=SCPS_H||bx<0||by<0||bx>=SCPS_W||by>=SCPS_H) return false;
    if(!g_rd_g){ g_rd_g=(float*)malloc(sizeof(float)*SCPS_N); g_rd_from=(int*)malloc(sizeof(int)*SCPS_N);
        g_rd_gen=(int*)calloc(SCPS_N,sizeof(int)); g_rd_closed=(int*)calloc(SCPS_N,sizeof(int));
        g_heapf=(float*)malloc(sizeof(float)*SCPS_N); g_heapi=(int*)malloc(sizeof(int)*SCPS_N);
        if(!g_rd_g||!g_rd_from||!g_rd_gen||!g_rd_closed||!g_heapf||!g_heapi) return false; }
    int minx=(ax<bx?ax:bx)-48, maxx=(ax>bx?ax:bx)+48, miny=(ay<by?ay:by)-48, maxy=(ay>by?ay:by)+48;
    if(minx<0)minx=0;
    if(miny<0)miny=0;
    if(maxx>=SCPS_W)maxx=SCPS_W-1;
    if(maxy>=SCPS_H)maxy=SCPS_H-1;
    g_rd_curgen++; g_heap_n=0;
    int s=scps_idx(ax,ay), goal=scps_idx(bx,by);
    g_rd_g[s]=0.f; g_rd_from[s]=-1; g_rd_gen[s]=g_rd_curgen;
    rheap_push(hypotf((float)(bx-ax),(float)(by-ay)), s);
    static const int dx8[8]={1,-1,0,0,1,1,-1,-1}, dy8[8]={0,0,1,-1,1,-1,1,-1};
    bool found=false; int guard=0;
    while(g_heap_n>0 && guard++<900000){
        int cur=rheap_pop();
        if(g_rd_closed[cur]==g_rd_curgen) continue;
        g_rd_closed[cur]=g_rd_curgen;
        if(cur==goal){ found=true; break; }
        int cxx=cur%SCPS_W, cyy=cur/SCPS_W;
        for(int d=0;d<8;d++){
            int nx=cxx+dx8[d], ny=cyy+dy8[d];
            if(nx<minx||nx>maxx||ny<miny||ny>maxy) continue;
            int ni=scps_idx(nx,ny);
            if(g_rd_closed[ni]==g_rd_curgen) continue;
            float cc=road_cell_cost(w,nx,ny); if(cc<0.f) continue;
            float ng=g_rd_g[cur]+(d<4?1.f:1.41421f)*cc;
            if(g_rd_gen[ni]==g_rd_curgen && g_rd_g[ni]<=ng) continue;
            g_rd_g[ni]=ng; g_rd_from[ni]=cur; g_rd_gen[ni]=g_rd_curgen;
            rheap_push(ng+hypotf((float)(bx-nx),(float)(by-ny)), ni);
        }
    }
    if(!found) return false;
    static int tmp[4096]; int tn=0;
    for(int cur=goal; cur!=-1 && tn<4096; cur=g_rd_from[cur]) tmp[tn++]=cur;
    int stepd=(tn>ROAD_PATH_MAX)?(tn/ROAD_PATH_MAX+1):1;
    out->len=0;
    for(int k=tn-1;k>=0 && out->len<ROAD_PATH_MAX;k-=stepd){
        out->x[out->len]=(int16_t)(tmp[k]%SCPS_W); out->y[out->len]=(int16_t)(tmp[k]/SCPS_W); out->len++;
    }
    return out->len>=2;
}
/* LISSAGE++ : moyenne mobile (extrémités fixes) — arrondit l'escalier 8-connexe de l'A*. */
static void road_path_smooth(RoadPath *p){
    if (p->len<3) return;
    static int16_t nx[ROAD_PATH_MAX], ny[ROAD_PATH_MAX];
    for (int pass=0; pass<3; pass++){
        nx[0]=p->x[0]; ny[0]=p->y[0]; nx[p->len-1]=p->x[p->len-1]; ny[p->len-1]=p->y[p->len-1];
        for (int k=1;k<p->len-1;k++){
            nx[k]=(int16_t)((p->x[k-1]+2*p->x[k]+p->x[k+1])/4);
            ny[k]=(int16_t)((p->y[k-1]+2*p->y[k]+p->y[k+1])/4);
        }
        memcpy(p->x,nx,sizeof(int16_t)*p->len); memcpy(p->y,ny,sizeof(int16_t)*p->len);
    }
}
static void roads_ensure_cache(const World *w,const RouteNetwork *rn){
    uint64_t sig=1469598103934665603ull;
    for(int i=0;i<rn->n && i<SCPS_MAX_ROUTES;i++){ const TradeRoute *tr=&rn->route[i];
        if(tr->open && !tr->maritime) sig=(sig^(uint64_t)(tr->ra*100003+tr->rb))*1099511628211ull; }
    if(sig==g_rd_sig && g_rd_paths) return;        /* réseau inchangé → cache valide */
    g_rd_sig=sig;
    if(!g_rd_paths){ g_rd_paths=(RoadPath*)malloc(sizeof(RoadPath)*SCPS_MAX_ROUTES); if(!g_rd_paths){g_rd_npaths=0;return;} }
    g_rd_npaths=0;
    /* NIVEAUX : le commerce existe VIRTUELLEMENT ; on ne TRACE que les routes MAJEURES
     * (les plus fortes CAPACITÉS — top K). Trop de traits = réseau abstrait. */
    int idx[SCPS_MAX_ROUTES], ni=0;
    for(int i=0;i<rn->n && i<SCPS_MAX_ROUTES;i++){ const TradeRoute *tr=&rn->route[i];
        if(tr->open && !tr->maritime) idx[ni++]=i; }
    for(int a=1;a<ni;a++){ int v=idx[a]; float vc=rn->route[v].capacity; int b=a-1;   /* tri ↓ capacité */
        while(b>=0 && rn->route[idx[b]].capacity<vc){ idx[b+1]=idx[b]; b--; } idx[b+1]=v; }
    int K = ni<14?ni:14;                                  /* majeures seulement (au zoom courant) */
    for(int r=0;r<K && g_rd_npaths<SCPS_MAX_ROUTES;r++){
        const TradeRoute *tr=&rn->route[idx[r]];
        float fax,fay,fbx,fby;
        if(!region_world_pos(w,tr->ra,&fax,&fay)||!region_world_pos(w,tr->rb,&fbx,&fby)) continue;
        if(hypotf(fbx-fax,fby-fay) > 360.f) continue;     /* trop loin (probable saut de mer) */
        if(road_astar(w,(int)fax,(int)fay,(int)fbx,(int)fby,&g_rd_paths[g_rd_npaths])){
            g_rd_paths[g_rd_npaths].ra=tr->ra; g_rd_paths[g_rd_npaths].rb=tr->rb;
            road_path_smooth(&g_rd_paths[g_rd_npaths]);    /* lissage++ */
            g_rd_npaths++;
        }
    }
    /* MASQUE des cellules-ROUTE (+1 de marge) : pour ne PAS poser de champ sur une route. */
    if(!g_road_mask) g_road_mask=(uint8_t*)calloc(SCPS_N,1);
    if(g_road_mask){
        memset(g_road_mask,0,SCPS_N);
        for(int pi=0;pi<g_rd_npaths;pi++){ const RoadPath *p=&g_rd_paths[pi];
            for(int k=0;k<p->len;k++){ int rx=p->x[k],ry=p->y[k];
                for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
                    int nx=rx+dx,ny=ry+dy; if(nx>=0&&ny>=0&&nx<SCPS_W&&ny<SCPS_H) g_road_mask[ny*SCPS_W+nx]=1; } } }
    }
}
/* TRAIT ÉPAIS continu (RenderGeometry, quad par segment + losange aux points) — grammaire
 * HOMOGÈNE : largeur & couleur CONSTANTES, raccords par recouvrement. Remplace le tuilage
 * de sprites qui « clôturait ». */
static void road_thick_polyline(SDL_Renderer *ren, const float *sx, const float *sy, int n, float wid, SDL_Color col){
    if (n<2 || wid<1.f) return;
    static SDL_Vertex vb[ROAD_PATH_MAX*8]; static int ib[ROAD_PATH_MAX*12];
    int nv=0,nidx=0; const int CAP=ROAD_PATH_MAX*8;
    for (int i=0;i+1<n && nv+4<=CAP;i++){
        float dx=sx[i+1]-sx[i], dy=sy[i+1]-sy[i]; float L=sqrtf(dx*dx+dy*dy); if(L<0.01f) continue;
        float nx=-dy/L*wid*0.5f, ny=dx/L*wid*0.5f; int b=nv;
        vb[nv].position.x=sx[i]+nx;   vb[nv].position.y=sy[i]+ny;   vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i]-nx;   vb[nv].position.y=sy[i]-ny;   vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i+1]+nx; vb[nv].position.y=sy[i+1]+ny; vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i+1]-nx; vb[nv].position.y=sy[i+1]-ny; vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        ib[nidx++]=b; ib[nidx++]=b+1; ib[nidx++]=b+2; ib[nidx++]=b+2; ib[nidx++]=b+1; ib[nidx++]=b+3;
    }
    for (int i=1;i+1<n && nv+4<=CAP;i++){            /* losange à chaque coude → raccord sans trou */
        float h=wid*0.5f; int b=nv;
        vb[nv].position.x=sx[i];   vb[nv].position.y=sy[i]-h; vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i]+h; vb[nv].position.y=sy[i];   vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i];   vb[nv].position.y=sy[i]+h; vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        vb[nv].position.x=sx[i]-h; vb[nv].position.y=sy[i];   vb[nv].color=col; vb[nv].tex_coord.x=0; vb[nv].tex_coord.y=0; nv++;
        ib[nidx++]=b; ib[nidx++]=b+1; ib[nidx++]=b+2; ib[nidx++]=b+2; ib[nidx++]=b+3; ib[nidx++]=b;
    }
    if (nv>=3) SDL_RenderGeometry(ren, NULL, vb, nv, ib, nidx);
}
static void draw_map_roads(SDL_Renderer *ren, const World *w, const RouteNetwork *rn, const Cam *cam, int win_w, int win_h){
    if (!g_dress_tex || !rn) return;
    float sc=cam->scale; if (sc<3.0f) return;
    g_iso_w=win_w; g_iso_h=win_h;
    roads_ensure_cache(w,rn);
    static float rsx[ROAD_PATH_MAX], rsy[ROAD_PATH_MAX];
    SDL_Color casing={58,42,28,255}, fill={196,164,110,255}, wood={122,84,48,255};
    for(int pi=0; pi<g_rd_npaths; pi++){
        const RoadPath *p=&g_rd_paths[pi];
        int n=p->len; if(n>ROAD_PATH_MAX)n=ROAD_PATH_MAX;
        for(int k=0;k<n;k++){ float fx,fy; cam_project(cam,(float)p->x[k]+0.5f,(float)p->y[k]+0.5f,&fx,&fy); rsx[k]=fx; rsy[k]=fy; }
        float maj=(pi<3)?1.0f:(pi<7?0.78f:0.6f);                  /* NIVEAU → largeur (artère/desserte) */
        float wfill=sc*0.55f*maj; if(wfill<2.2f)wfill=2.2f;
        float wcas=wfill + sc*0.16f + 2.f;
        road_thick_polyline(ren, rsx,rsy,n, wcas, casing);        /* bord sombre (casing) */
        road_thick_polyline(ren, rsx,rsy,n, wfill, fill);         /* surface (constante) */
        for(int k=0;k<n;k++){ const Cell *c=scps_cellc(w,p->x[k],p->y[k]);
            if(c->river>40 && !c->lake){                           /* PONT (bois) au franchissement */
                int bs=(int)(wcas*1.3f); if(bs<4)bs=4;
                SDL_Rect br={(int)(rsx[k]-bs*0.5f),(int)(rsy[k]-bs*0.5f),bs,bs};
                SDL_SetRenderDrawColor(ren,wood.r,wood.g,wood.b,255); SDL_RenderFillRect(ren,&br); } }
        /* HABILLAGE : MOBILIER de route (pack route-cover) en BORDURE — bornes, haies, bottes,
         * rochers, caisses… varié ; DEUX côtés AUX COUDES (masquent l'angle), un côté ailleurs. */
        static const int rcov[8]={COVER_HEDGE_CLUMP,COVER_BUSH,COVER_BOULDER,COVER_HAYSTACK,
                                  COVER_MILESTONE,COVER_CRATE,COVER_BUSH,COVER_HEDGE_CLUMP};
        int dsz=(int)(sc*1.7f); if(dsz<10)dsz=10; if(dsz>96)dsz=96;
        int rstep=(int)(11.0f/sc)+3;
        for(int k=rstep;k<n-rstep;k+=rstep){
            int kp=k-rstep, kn=k+rstep; if(kn>=n)kn=n-1;
            float ax=rsx[k]-rsx[kp], ay=rsy[k]-rsy[kp], bx=rsx[kn]-rsx[k], by=rsy[kn]-rsy[k];
            float la=sqrtf(ax*ax+ay*ay)+1e-3f, lb=sqrtf(bx*bx+by*by)+1e-3f;
            float pxp=-(ay+by), pyp=(ax+bx); float pl=sqrtf(pxp*pxp+pyp*pyp)+1e-3f; pxp/=pl; pyp/=pl;  /* perp moyenne */
            bool bend=((ax*bx+ay*by)/(la*lb))<0.86f;        /* la route TOURNE ici */
            float off=wcas*0.5f + dsz*0.32f;
            for(int sgn=-1;sgn<=1;sgn+=2){
                if(sgn>0 && !bend && ((k/rstep)&1)) continue;            /* tout-droit : un seul côté, alterné */
                uint32_t hh=map_hash(p->x[k]+sgn, p->y[k], 0xD2E5CAFEu);
                int id=rcov[hh&7];                                       /* mobilier varié */
                int ssx=(int)(rsx[k]+sgn*pxp*off), ssy=(int)(rsy[k]+sgn*pyp*off);
                if(ssx<-dsz||ssx>win_w+dsz||ssy<-dsz||ssy>win_h+dsz) continue;
                cover_blit(ren, id, ssx-dsz/2, ssy-(dsz*3)/4, dsz);     /* ancré au sol */
            }
        }
    }
}
/* SETTLEMENTS : une VILLE par région COLONISÉE, au centre, TAILLE = tier de population,
 * SILHOUETTE = terrain (priorité estuaire > rivière > montagne > capitale=fortifiée >
 * rural). Display-only : lit colonized + pop + géographie (tangibles). Dessinée APRÈS le
 * décor → focale bien lisible. */
/* cellule de MER la plus proche d'une ville côtière, dans un PETIT rayon (maxrad). Le
 * traitement « côtier » (port + raccord d'eau) ne s'applique que si la mer est VRAIMENT
 * proche — sinon une ville au centre d'une région côtière (mer à 10+ cellules) héritait d'un
 * faux halo d'eau (« ville dans un lac »). */
static bool settle_nearest_sea(const World *w, int cx, int cy, int maxrad, int *swx, int *swy){
    for (int rad=1; rad<=maxrad; rad++)
        for (int dy=-rad; dy<=rad; dy++)
            for (int dx=-rad; dx<=rad; dx++){
                if (abs(dx)!=rad && abs(dy)!=rad) continue;        /* bord de l'anneau seulement */
                int nx=cx+dx, ny=cy+dy;
                if (nx<0||ny<0||nx>=SCPS_W||ny>=SCPS_H) continue;
                if (scps_cellc(w,nx,ny)->sea){ *swx=nx; *swy=ny; return true; }
            }
    return false;
}
/* BLEND mer : un APRON d'eau au glow DOUX (disque à alpha en cloche, sans bord franc), posé
 * SOUS l'asset côtier et poussé vers la mer → l'eau de la rade « remonte » dans la carte (raccord
 * naturel, fini le carré d'eau rapporté). (bx,by) = assise écran de la ville ; (ux,uy) = direction
 * écran NORMALISÉE vers la mer. Écrasé en ellipse (respire l'iso 2:1), teinté à la volée. */
static void sea_blend(SDL_Renderer *ren, float bx, float by, float dpx, float ux, float uy){
    if(!g_softblob_tex) return;
    float cx=bx+ux*dpx*0.24f, cy=(by-dpx*0.06f)+uy*dpx*0.24f;     /* centre : assise, poussé un peu vers la mer */
    float bw=dpx*1.34f, bh=dpx*0.82f;                              /* ellipse plus large que haute (iso) */
    SDL_Rect dst={(int)(cx-bw*0.5f),(int)(cy-bh*0.5f),(int)bw,(int)bh};
    SDL_SetTextureColorMod(g_softblob_tex,52,110,154);
    SDL_SetTextureAlphaMod(g_softblob_tex,118);
    SDL_RenderCopy(ren,g_softblob_tex,NULL,&dst);
    SDL_SetTextureColorMod(g_softblob_tex,255,255,255);
    SDL_SetTextureAlphaMod(g_softblob_tex,255);
}
/* Cale une ville sur la TERRE en s'écartant des LACS : rend la cellule de terre la plus proche
 * (anneaux croissants) SANS aucun lac à ≤ lakeclr cellules. La MER reste permise (les côtières
 * sont des ports) — seuls les LACS sont fuis. false si rien à portée (l'appelant retombe alors
 * sur lakeclr=0 = simple terre la plus proche). */
static bool settle_land_spot(const World *w, int cx, int cy, int lakeclr, int *ox, int *oy){
    for (int R=0; R<=14; R++)
        for (int dy=-R; dy<=R; dy++)
            for (int dx=-R; dx<=R; dx++){
                if (R>0 && abs(dx)!=R && abs(dy)!=R) continue;       /* bord d'anneau (R=0 = le centre) */
                int nx=cx+dx, ny=cy+dy;
                if (nx<0||ny<0||nx>=SCPS_W||ny>=SCPS_H) continue;
                const Cell *nc=scps_cellc(w,nx,ny);
                if (nc->sea||nc->lake) continue;                     /* la ville est sur la TERRE */
                bool nearlake=false;
                for (int yy=-lakeclr; yy<=lakeclr && !nearlake; yy++)
                    for (int xx=-lakeclr; xx<=lakeclr; xx++){
                        int mx=nx+xx,my=ny+yy;
                        if (mx<0||my<0||mx>=SCPS_W||my>=SCPS_H) continue;
                        if (scps_cellc(w,mx,my)->lake){ nearlake=true; break; }
                    }
                if (nearlake) continue;                              /* écarte les bords de lac */
                *ox=nx; *oy=ny; return true;
            }
    return false;
}
static void draw_map_settlements(SDL_Renderer *ren, const World *w, const WorldEconomy *econ, const Cam *cam, int win_w, int win_h){
    if (!g_settle_tex || !econ) return;
    float sc=cam->scale; if (sc<2.0f) return;
    g_iso_w=win_w; g_iso_h=win_h;
    static const float dscale[6]={0.50f,0.66f,0.84f,1.05f,1.28f,1.55f};   /* outpost…metropolis (échelonné) */
    /* MASQUE de DÉBOISEMENT : remis à plat ici (les villes le tamponnent ci-dessous, AVANT la
     * passe canopée qui le lit) → un rayon dégagé autour de chaque ville. */
    if(!g_clear_mask) g_clear_mask=(uint8_t*)calloc(SCPS_N,1);
    if(g_clear_mask) memset(g_clear_mask,0,SCPS_N);
    for (int r=0; r<econ->n_regions && r<w->n_regions; r++){
        const RegionEconomy *re=&econ->region[r];
        if (!re->colonized) continue;
        float pop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        if (pop<40.f) continue;
        int tier = pop>=4000?5 : pop>=1500?4 : pop>=500?3 : pop>=150?2 : pop>=50?1 : 0;
        float wx,wy; if(!region_world_pos(w,r,&wx,&wy)) continue;
        int cx=(int)wx, cy=(int)wy; if(cx<0||cy<0||cx>=SCPS_W||cy>=SCPS_H) continue;
        /* PAS DE VILLE DANS / AU BORD D'UN LAC : le centroïde des provinces peut tomber sur l'eau
         * (région enroulée autour d'un lac) ; on cale sur la terre la plus proche À L'ÉCART des lacs
         * (marge 2). À défaut (région très lacustre) → terre nue la plus proche ; sinon aucune ville. */
        { int bx,by;
          if (settle_land_spot(w,cx,cy,2,&bx,&by) || settle_land_spot(w,cx,cy,0,&bx,&by)){
              cx=bx; cy=by; wx=(float)cx; wy=(float)cy;
          } else continue;
        }
        const Cell *c=scps_cellc(w,cx,cy);
        int owner=re->owner;
        bool cap=(owner>=0 && owner<w->n_countries && w->country[owner].capital_prov>=0
                  && w->country[owner].capital_prov<w->n_provinces
                  && w->province[w->country[owner].capital_prov].region==r);
        /* mer la plus proche : (a) RABAT un PEU vers l'intérieur (assise sur terre SANS se couper
         * des routes), (b) ORIENTE la cité portuaire. Sprite estuaire (plage au SUD) si la mer est
         * au sud d'écran ; mer au NORD → PORT vue de DOS (estuary_dock 96-103) — une vraie cité
         * PORTUAIRE orientée (pas une cité fortifiée sans quais). */
        int seawx=0,seawy=0; float wx2=wx, wy2=wy, sdx=0.f, sdy=1.f;
        bool hassea = (re->coastal && settle_nearest_sea(w,cx,cy,3,&seawx,&seawy));   /* mer à ≤3 cellules : vraie côtière */
        if (hassea){
            float ix=(float)cx-seawx, iy=(float)cy-seawy; float il=sqrtf(ix*ix+iy*iy)+1e-3f;
            wx2 += ix/il*2.5f; wy2 += iy/il*2.5f;                       /* léger rabat → la route ATTEINT encore la ville */
            float ax,ay,bx,by; cam_project(cam,(float)cx,(float)cy,&ax,&ay);
            cam_project(cam,(float)seawx+0.5f,(float)seawy+0.5f,&bx,&by); sdx=bx-ax; sdy=by-ay;
        }
        int group; bool port_back=false;
        if (re->coastal && sdy>0.f)                group=SETTLE_ESTUARY;        /* plage au sud → sprite estuaire */
        else if (re->coastal && g_port_tex)      { group=-1; port_back=true; }  /* mer au NORD → port de DOS */
        else if (re->coastal)                      group=(cap?SETTLE_FORTIFIED:SETTLE_RURAL);
        else if (c->river>40 && !c->lake)          group=SETTLE_RIVER;
        else if (c->biome==BIO_MOUNTAINS||c->biome==BIO_PEAK||c->biome==BIO_HILLS||c->biome==BIO_HIGHLANDS) group=SETTLE_MOUNTAIN;
        else if (cap)                              group=SETTLE_FORTIFIED;     /* capitale = remparts */
        else                                       group=SETTLE_RURAL;
        if (cap && tier<4) tier=4;                                             /* la CAPITALE domine : cité a minima */
        if (g_clear_mask){                                                     /* DÉBOISE un disque ∝ taille autour de la ville */
            int rad=(int)(9.0f*dscale[tier]+0.5f); if(rad<4)rad=4; if(rad>18)rad=18;
            for(int dy=-rad;dy<=rad;dy++) for(int dx=-rad;dx<=rad;dx++){
                if(dx*dx+dy*dy>rad*rad) continue;
                int nx=cx+dx,ny=cy+dy; if(nx>=0&&ny>=0&&nx<SCPS_W&&ny<SCPS_H) g_clear_mask[ny*SCPS_W+nx]=1;
            }
        }
        float fsx,fsy; cam_project(cam,wx2,wy2,&fsx,&fsy);
        int dpx=(int)(sc*18.0f*dscale[tier]); if(dpx<22)dpx=22; if(dpx>680)dpx=680;
        if(fsx<-dpx||fsx>win_w+dpx||fsy<-dpx||fsy>win_h+dpx) continue;
        /* HABILLAGE LÉGER : quelques arbres en bordure ARRIÈRE seulement → adoucit sans BLOQUER
         * les routes qui sortent (l'anneau plein cachait tout le réseau). */
        if (g_dress_tex){
            static const int rgt[3]={MAPD_TREE_BROADLEAF,MAPD_TREE_CONIFER,MAPD_HEDGE_PATCH};
            int rr=(int)(dpx*0.34f), tsz=(int)(dpx*0.20f); if(tsz<8)tsz=8;
            for (int a=0;a<5;a++){
                float ang=4.19f + (float)a*0.50f;                              /* arc ARRIÈRE (haut d'écran) */
                int tx=(int)(fsx+cosf(ang)*rr), ty=(int)(fsy+sinf(ang)*rr*0.55f - dpx*0.12f);
                uint32_t hh=map_hash(cx+a*7, cy, 0x9A17C0DEu);
                dress_blit(ren, rgt[hh%3], tx-tsz/2, ty-(tsz*3)/4, tsz);
            }
        }
        if (port_back){                              /* PORT vue de DOS — estuary_dock (12) INTERDIT ;
                                                      * on prend pêche(8)/quai(9)/port(10) selon tier, miroir selon mer G/D. */
            int prow=(tier>=4)?10:(tier>=2)?9:8; int pcol=(sdx<0.f)?1:0; int pid=prow*SCPS_PORT_COLS+pcol;
            SDL_Rect psrc={(pid%SCPS_PORT_COLS)*SCPS_PORT_CELL,(pid/SCPS_PORT_COLS)*SCPS_PORT_CELL,SCPS_PORT_CELL,SCPS_PORT_CELL};
            SDL_Rect pdst={(int)fsx-dpx/2,(int)fsy-(dpx*7)/10,dpx,dpx};
            { float dl=sqrtf(sdx*sdx+sdy*sdy)+1e-3f; sea_blend(ren,fsx,fsy,(float)dpx,sdx/dl,sdy/dl); }  /* mer DERRIÈRE (avant l'asset) */
            SDL_RenderCopy(ren,g_port_tex,&psrc,&pdst);
            continue;
        }
        if (hassea){ float dl=sqrtf(sdx*sdx+sdy*sdy)+1e-3f; sea_blend(ren,fsx,fsy,(float)dpx,sdx/dl,sdy/dl); }  /* raccord mer (derrière) */
        SDL_Rect src={tier*SCPS_SETTLE_CELL, group*SCPS_SETTLE_CELL, SCPS_SETTLE_CELL, SCPS_SETTLE_CELL};
        SDL_Rect dst={(int)fsx-dpx/2, (int)fsy-(dpx*7)/10, dpx, dpx};          /* ancré bas-centre */
        SDL_RenderCopy(ren, g_settle_tex, &src, &dst);
    }
}
/* DÉCORS (habillage). Dessiné en DEUX passes pour le bon ordre de calques (z-order
 * demandé : routes → villes → habillage → CANOPÉES) : canopy_pass=0 pose tout le décor
 * de SOL (champs, bâti, roches, buissons, fermes) ; canopy_pass=1 pose UNIQUEMENT les
 * ARBRES (famille 2), qui coiffent donc tout le reste. La canopée se chevauche en dernier. */
static void draw_map_dressing(SDL_Renderer *ren, const World *w, const WorldEconomy *econ, const Cam *cam, int win_w, int win_h, int canopy_pass){
    if (!g_dress_tex) return;
    g_iso_w=win_w; g_iso_h=win_h;                 /* pivot iso de la frame */
    float sc = cam->scale;                        /* pixels par cellule */
    if (sc < 3.0f) return;                         /* trop dézoomé → pas de décor (sous-pixel) */
    /* pas monde tel que l'espacement ÉCRAN reste ~constant (≈ 14 px) : dense de
     * près, jamais en bouillie ni explosif en compte. */
    int step = (int)(14.0f/sc + 0.5f); if (step < 1) step = 1; if (step > 4) step = 4;
    int alpha = (sc < 5.0f) ? 200 : (sc < 9.0f ? 230 : 255);
    /* plage MONDE couvrant la fenêtre (iso : on inverse les 4 coins → boîte englobante). */
    float wx0,wy0,wx1,wy1,twx,twy; cam_unproject(cam,0,0,&wx0,&wy0); wx1=wx0; wy1=wy0;
    float corn[3][2]={{(float)win_w,0},{0,(float)win_h},{(float)win_w,(float)win_h}};
    for (int k=0;k<3;k++){
        cam_unproject(cam,corn[k][0],corn[k][1],&twx,&twy);
        if(twx<wx0)wx0=twx;
        if(twx>wx1)wx1=twx;
        if(twy<wy0)wy0=twy;
        if(twy>wy1)wy1=twy;
    }
    int cx0=(int)wx0-2, cy0=(int)wy0-2, cx1=(int)wx1+2, cy1=(int)wy1+2;
    if (cx0<0) cx0=0;
    if (cy0<0) cy0=0;
    if (cx1>SCPS_W) cx1=SCPS_W;
    if (cy1>SCPS_H) cy1=SCPS_H;
    cx0 -= cx0%step; cy0 -= cy0%step;             /* lattice ancré monde → stable au pan */
    SDL_SetTextureAlphaMod(g_dress_tex, (Uint8)alpha);
    for (int cy=cy0; cy<cy1; cy+=step){           /* rangées : arrière → avant (ordre de profondeur iso) */
        for (int cx=cx0; cx<cx1; cx+=step){
            const Cell *c = scps_cellc(w, cx, cy);
            if (c->sea || c->lake) continue;       /* mer ET LACS restent nus (plus d'arbres dans l'eau : le bleu du lac perçait sous la canopée) */
            float fsx,fsy; cam_project(cam,(float)cx,(float)cy,&fsx,&fsy);
            int sx=(int)fsx, sy=(int)fsy;
            uint32_t h=map_hash(cx,cy,0x5EED01u);
            /* ── peuplement RÉEL : la région est-elle colonisée, et à quel point ? ── */
            int rg = c->region;
            bool settled = (econ && rg>=0 && rg<econ->n_regions && econ->region[rg].colonized);
            int tier=0;
            if (settled){ const RegionEconomy *re=&econ->region[rg];
                float pp=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
                tier = (pp>4000.f)?2:(pp>800.f)?1:0; }
            uint32_t hb = map_hash(cx,cy,0xB17DCAFEu);   /* hash indépendant pour le bâti */
            /* (les RIVIÈRES sont chaînées dans une passe à part, draw_map_rivers, SOUS ici.) */
            /* ── VILLES : bâtiments isolés sur la terre COLONISÉE, densité ∝ tier de pop.
             *    Le bâti OCCUPE la cellule (pas de nature en plus) + un chemin proche. ── */
            int bgate = (tier>=2)?1:0;                 /* fermes OUTLYING rares : la VILLE est le sprite settlement */
            if (settled && (int)(hb&15u) < bgate){
                if (canopy_pass) continue;             /* le bâti isolé est de l'HABILLAGE (sol), pas la canopée */
                int bid = dress_building(c, tier, hb);
                int bpx = dress_size(bid,sc);
                int bjx=(int)((hb>>8)&7u)-4, bjy=(int)((hb>>11)&7u)-4;
                if ((hb&0x30u)==0u){ int rid=MAPD_FOOTPATH_STRAIGHT; int rpx=dress_size(rid,sc);   /* ROUTE : un chemin au pied du bâti */
                    dress_blit(ren, rid, sx-rpx/2, sy-rpx/2, rpx); }
                dress_blit(ren, bid, sx-bpx/2+bjx, sy-(bpx*3)/4+bjy, bpx);
                continue;
            }
            if ((int)(h&15u) >= dress_density(c->biome)) continue;   /* cellule sans décor naturel */
            int id=dress_pick(c,h);
            if (id<0) continue;
            { int fam=(id<56)?id/8:(id-56)/8; if ((fam==2) != (canopy_pass!=0)) continue; }  /* famille 2 = ARBRES → passe canopée ; le reste → passe sol */
            if (canopy_pass && g_clear_mask && g_clear_mask[cy*SCPS_W+cx]) continue;          /* forêt DÉBOISÉE autour des villes */
            if (c->biome==BIO_FARMLAND){                              /* CHAMP : très ponctuel (~95 % retirés) + JAMAIS sur une route */
                if (g_road_mask && g_road_mask[cy*SCPS_W+cx]) continue;
                if ((map_hash(cx,cy,0xFA12BEEFu) & 15u) != 0u) continue;  /* ne garde qu'1 sur 16 */
            }
            int px=dress_size(id,sc);
            int jx=(int)((h>>8)&7u)-4, jy=(int)((h>>11)&7u)-4;   /* jitter sous-cellule (anti-grille) */
            dress_blit(ren, id, sx-px/2+jx, sy-(px*3)/4+jy, px);  /* ancrage bas-centre (sprite debout) */
            /* FERME : le champ (RARE) est HABILLÉ — un petit MOULIN + une grappe d'assets
             * (bottes, caisses, haies) en fait une ferme, pas un carré répété. */
            if (c->biome==BIO_FARMLAND){
                int wsz=(int)((float)px*0.95f); if(wsz<10)wsz=10;
                dress_blit(ren, MAPD_BLD_WINDMILL, sx-wsz/2+jx+(int)((h>>20)&3u)-2, sy-(wsz*3)/4+jy, wsz);
                if (g_cover_tex){
                    static const int fcl[5]={COVER_HAYSTACK,COVER_CRATE,COVER_HEDGE_CLUMP,COVER_HAYSTACK,COVER_REED};
                    int fn=2+(int)((h>>17)&1u);
                    for (int q=0;q<fn;q++){
                        uint32_t hq=map_hash(cx*3+q, cy*5+q, 0xFA12BEEFu);
                        int fsz=(int)((float)px*0.5f); if(fsz<7)fsz=7;
                        int ox=(int)((hq>>3)&31u)-16, oy=(int)((hq>>9)&15u)-8;
                        cover_blit(ren, fcl[hq%5], sx-fsz/2+ox+jx, sy-(fsz*3)/4+oy+jy, fsz);
                    }
                }
            }
        }
    }
    SDL_SetTextureAlphaMod(g_dress_tex, 255);
}

/* ═══ RENDU DES ANIMATIONS FX ═══════════════════════════════════════════════
 * Surcouches display-only, projetées par cam_project (iso compris), cadencées mur.
 * Toutes no-op si leur texture est absente. */

/* Boîte-monde VISIBLE (AABB), bornée par les 4 coins écran dé-projetés. En iso la
 * fenêtre est un losange → on élargit. Sert à ne balayer que le viewport. */
static void fx_visible_aabb(const Cam *cam, int win_w, int win_h, int *x0,int *y0,int *x1,int *y1){
    int xmn=SCPS_W, ymn=SCPS_H, xmx=0, ymx=0;
    const int cxs[4]={0,win_w,0,win_w}, cys[4]={0,0,win_h,win_h};
    for (int i=0;i<4;i++){
        float wx,wy; cam_unproject(cam,(float)cxs[i],(float)cys[i],&wx,&wy);
        int ix=(int)wx, iy=(int)wy;
        if (ix<xmn) xmn=ix;
        if (iy<ymn) ymn=iy;
        if (ix>xmx) xmx=ix;
        if (iy>ymx) ymx=iy;
    }
    int padx=(xmx-xmn)/4+4, pady=(ymx-ymn)/4+4;             /* couvre les coins du losange iso */
    xmn-=padx; xmx+=padx; ymn-=pady; ymx+=pady;
    if (xmn<0) xmn=0;
    if (ymn<0) ymn=0;
    if (xmx>SCPS_W-1) xmx=SCPS_W-1;
    if (ymx>SCPS_H-1) ymx=SCPS_H-1;
    *x0=xmn;*y0=ymn;*x1=xmx;*y1=ymx;
}

/* HOULE : un voile de vagues sur la mer ouverte (c->sea != 0), tuilé en cellules-
 * monde, déphasé par tuile (pas de répétition franche), la ligne d'atlas suivant
 * l'énergie du courant (eaux vives → houle ample). Léger : la couleur d'eau du
 * terrain transparaît dessous. Gate au zoom (sous-pixel au dézoom). */
static void draw_sea_fx(SDL_Renderer *ren, const World *w, const Cam *cam, int win_w, int win_h){
    if (!g_fx_sea_tex || !w) return;
    float sc=cam->scale; if (sc<1.0f) return;            /* ~zoom ajusté : la houle vit dès la vue d'ensemble */
    g_iso_w=win_w; g_iso_h=win_h;
    const int TILE=8;
    int dpx=(int)((float)TILE*sc+1.5f); if (dpx<6) return;
    int frame=fx_frame(SCPS_FX_SEA_FRAMES, 150);
    int x0,y0,x1,y1; fx_visible_aabb(cam,win_w,win_h,&x0,&y0,&x1,&y1);
    SDL_SetTextureAlphaMod(g_fx_sea_tex, 90);              /* voile léger : la couleur d'eau transparaît */
    for (int ty=y0; ty<=y1; ty+=TILE) for (int tx=x0; tx<=x1; tx+=TILE){
        int mx=tx+TILE/2, my=ty+TILE/2;
        if (mx>=SCPS_W||my>=SCPS_H) continue;
        const Cell *c=scps_cellc(w,mx,my);
        if (!c || c->sea==0) continue;                       /* mer ouverte seulement */
        float fsx,fsy; cam_project(cam,(float)tx+(float)TILE*0.5f,(float)ty+(float)TILE*0.5f,&fsx,&fsy);
        if (fsx<-dpx||fsx>win_w+dpx||fsy<-dpx||fsy>win_h+dpx) continue;
        uint32_t h=map_hash(tx,ty,0x5EA50000u);
        int ph=(frame+(int)(h%(uint32_t)SCPS_FX_SEA_FRAMES))%SCPS_FX_SEA_FRAMES;
        int en=(int)c->cur_vx*c->cur_vx+(int)c->cur_vy*c->cur_vy;   /* énergie du courant */
        int rowi=(en>1200)?1:0;
        SDL_Rect src={ ph*SCPS_FX_SEA_CELL, rowi*SCPS_FX_SEA_CELL, SCPS_FX_SEA_CELL, SCPS_FX_SEA_CELL };
        SDL_Rect dst={ (int)fsx-dpx/2, (int)fsy-dpx/2, dpx, dpx };
        SDL_RenderCopy(ren, g_fx_sea_tex, &src, &dst);
    }
    SDL_SetTextureAlphaMod(g_fx_sea_tex, 255);
}

/* NORMALE DE PLAGE : la direction MOYENNE vers la mer autour d'une cellule de côte
 * (somme pondérée des offsets vers les voisins de mer, rayon 2). Unitaire, (0,0) si
 * aucun voisin de mer. C'est elle qui POUSSE l'écume vers l'eau et qui l'ORIENTE. */
static bool coast_sea_normal(const World *w, int cx, int cy, float *nx, float *ny){
    float ax=0.f, ay=0.f; int n=0;
    for (int dy=-2; dy<=2; dy++) for (int dx=-2; dx<=2; dx++){
        if (!dx && !dy) continue;
        int x=cx+dx, y=cy+dy;
        if (x<0||y<0||x>=SCPS_W||y>=SCPS_H) continue;
        const Cell *c=scps_cellc(w,x,y);
        if (c && c->sea){ float il=1.f/(float)(abs(dx)+abs(dy)); ax+=(float)dx*il; ay+=(float)dy*il; n++; }
    }
    if (!n) return false;
    float l=sqrtf(ax*ax+ay*ay); if (l<1e-4f) return false;
    *nx=ax/l; *ny=ay/l; return true;
}

/* ÉCUME DE RIVE : un ourlet animé à la LIGNE D'EAU des cellules de côte (c->coast).
 * POUSSÉE vers la mer (normale de plage × SCPS_FX_COAST_PUSH) pour quitter le sable —
 * le « radius neutre » demandé — et ORIENTÉE f(direction plage) : la crête regarde la
 * mer (rotation déduite de la normale projetée, iso compris). Détail de zoom franc. */
static void draw_coast_fx(SDL_Renderer *ren, const World *w, const Cam *cam, int win_w, int win_h){
    if (!g_fx_coast_tex || !w) return;
    float sc=cam->scale; if (sc<2.0f) return;
    g_iso_w=win_w; g_iso_h=win_h;
    int x0,y0,x1,y1; fx_visible_aabb(cam,win_w,win_h,&x0,&y0,&x1,&y1);
    int frame=fx_frame(SCPS_FX_COAST_FRAMES, 130);
    int dpx=(int)(5.0f*sc+1.5f); if (dpx<10) dpx=10;
    const int TILE=3;
    SDL_SetTextureAlphaMod(g_fx_coast_tex, 150);
    for (int ty=y0; ty<=y1; ty+=TILE) for (int tx=x0; tx<=x1; tx+=TILE){
        const Cell *c=scps_cellc(w,tx,ty);
        if (!c || !c->coast) continue;                       /* terre au bord de mer */
        float wnx,wny; if (!coast_sea_normal(w,tx,ty,&wnx,&wny)) continue;   /* pas de mer voisine : on s'abstient */
        /* point POUSSÉ vers la mer (la ligne d'eau) + la direction ÉCRAN de la normale
         * (projection des DEUX points → iso-correct). */
        float p0x=(float)tx+0.5f, p0y=(float)ty+0.5f;
        float psx=p0x+wnx*SCPS_FX_COAST_PUSH, psy=p0y+wny*SCPS_FX_COAST_PUSH;
        float ax,ay,bx,by; cam_project(cam,p0x,p0y,&ax,&ay); cam_project(cam,psx,psy,&bx,&by);
        float sdx=bx-ax, sdy=by-ay; float sl=sqrtf(sdx*sdx+sdy*sdy);
        if (sl<1e-3f) continue;
        if (bx<-dpx||bx>win_w+dpx||by<-dpx||by>win_h+dpx) continue;
        double rot=atan2((double)sdy,(double)sdx)*RAD2DEG - 90.0 + (double)SCPS_FX_COAST_FACE_DEG;
        uint32_t h=map_hash(tx,ty,0xC0A57001u);
        int ph=(frame+(int)(h%(uint32_t)SCPS_FX_COAST_FRAMES))%SCPS_FX_COAST_FRAMES;
        SDL_Rect src={ ph*SCPS_FX_COAST_CELL, 0, SCPS_FX_COAST_CELL, SCPS_FX_COAST_CELL };
        SDL_Rect dst={ (int)bx-dpx/2, (int)by-dpx/2, dpx, dpx };
        SDL_RenderCopyEx(ren, g_fx_coast_tex, &src, &dst, rot, NULL, SDL_FLIP_NONE);
    }
    SDL_SetTextureAlphaMod(g_fx_coast_tex, 255);
}

/* ARMÉES EN CAMPAGNE : la force de chaque pays anime la case de sa région — ligne
 * d'atlas 0 (marche) ou 1 (assaut : siège/bataille). Une seule force par pays
 * (campaign), projetée au centroïde de sa région courante, ancrée bas-centre. */
static void draw_army_fx(SDL_Renderer *ren, const World *w, const Campaign *camp, const Cam *cam, int win_w, int win_h){
    if (!g_fx_army_tex || !w || !camp) return;
    float sc=cam->scale; if (sc<1.0f) return;
    g_iso_w=win_w; g_iso_h=win_h;
    int frame=fx_frame(SCPS_FX_ARMY_FRAMES, 180);
    int dpx=(int)(10.0f*sc+0.5f); if (dpx<20) dpx=20; if (dpx>240) dpx=240;
    for (int cid=0; cid<w->n_countries; cid++){
        if (!campaign_active(camp,cid)) continue;
        int reg=campaign_location(camp,cid);
        if (reg<0) continue;
        float wx,wy; if (!region_world_pos(w,reg,&wx,&wy)) continue;
        float fsx,fsy; cam_project(cam,wx,wy,&fsx,&fsy);
        if (fsx<-dpx||fsx>win_w+dpx||fsy<-dpx||fsy>win_h+dpx) continue;
        FieldPhase ph=campaign_phase(camp,cid);
        int rowi=(ph==FA_SIEGE||ph==FA_BATTLE)?1:0;          /* assaut : la ligne de combat */
        int col=(frame+cid)%SCPS_FX_ARMY_FRAMES;             /* déphasage par armée */
        SDL_Rect src={ col*SCPS_FX_ARMY_CELL, rowi*SCPS_FX_ARMY_CELL, SCPS_FX_ARMY_CELL, SCPS_FX_ARMY_CELL };
        SDL_Rect dst={ (int)fsx-dpx/2, (int)fsy-(dpx*3)/4, dpx, dpx };   /* ancré bas-centre */
        SDL_RenderCopy(ren, g_fx_army_tex, &src, &dst);
    }
}

/* VORTEX (fin EAU §27) : un maelström à DEUX calques contrarotatifs au foyer du
 * cataclysme (epicenter_reg), plus un tourbillon plus modeste sur chaque région
 * engloutie (sunken[]) — le flot qui s'étend. Rotation continue (horloge mur). */
static void draw_vortex_fx(SDL_Renderer *ren, const World *w, const EndgameState *eg, const Cam *cam, int win_w, int win_h){
    if (!g_fx_vortex_tex || !w || !eg) return;
    if (eg->fin != FIN_EAU) return;                          /* le vortex = la fin EAU (le rift) */
    g_iso_w=win_w; g_iso_h=win_h;
    float sc=cam->scale;
    float ms=(float)(SDL_GetTicks()%36000u);
    double ang0=(double)(ms*0.010f), ang1=(double)(-ms*0.014f);   /* deux sens, vitesses distinctes */
    SDL_Rect s0={0,0,SCPS_FX_VORTEX_CELL,SCPS_FX_VORTEX_CELL};
    SDL_Rect s1={SCPS_FX_VORTEX_CELL,0,SCPS_FX_VORTEX_CELL,SCPS_FX_VORTEX_CELL};
    if (eg->epicenter_reg>=0){
        float wx,wy;
        if (region_world_pos(w,eg->epicenter_reg,&wx,&wy)){
            float fsx,fsy; cam_project(cam,wx,wy,&fsx,&fsy);
            int big=(int)(60.0f*sc+0.5f); if (big<48) big=48;
            int big2=(int)(big*0.66f);
            SDL_Rect d0={ (int)fsx-big/2, (int)fsy-big/2, big, big };
            SDL_Rect d1={ (int)fsx-big2/2, (int)fsy-big2/2, big2, big2 };
            SDL_SetTextureAlphaMod(g_fx_vortex_tex, 205);
            SDL_RenderCopyEx(ren,g_fx_vortex_tex,&s0,&d0,ang0,NULL,SDL_FLIP_NONE);
            SDL_RenderCopyEx(ren,g_fx_vortex_tex,&s1,&d1,ang1,NULL,SDL_FLIP_NONE);
        }
    }
    int drawn=0;
    for (int r=0; r<w->n_regions && r<SCPS_MAX_REG && drawn<48; r++){
        if (!eg->sunken[r] || r==eg->epicenter_reg) continue;
        float wx,wy; if (!region_world_pos(w,r,&wx,&wy)) continue;
        float fsx,fsy; cam_project(cam,wx,wy,&fsx,&fsy);
        int sz=(int)(28.0f*sc+0.5f); if (sz<22) sz=22;
        if (fsx<-sz||fsx>win_w+sz||fsy<-sz||fsy>win_h+sz) continue;
        SDL_Rect d0={ (int)fsx-sz/2, (int)fsy-sz/2, sz, sz };
        double a=ang0+(double)(map_hash(r,r,0x901D0FF5u)%360u);    /* phase de rotation par région */
        SDL_SetTextureAlphaMod(g_fx_vortex_tex, 150);
        SDL_RenderCopyEx(ren,g_fx_vortex_tex,&s0,&d0,a,NULL,SDL_FLIP_NONE);
        drawn++;
    }
    SDL_SetTextureAlphaMod(g_fx_vortex_tex, 255);
}

/* ---- Info province (console) ----------------------------------------- */
static void print_province_info(const World *w, int prov_id) {
    if (prov_id < 0 || prov_id >= w->n_provinces) return;
    const Province *p = &w->province[prov_id];
    printf("\n┌─ Province #%d ─────────────────────────────────\n", prov_id);
    printf("│  Biome dominant  : %s\n", biome_name(p->biome_dominant));
    printf("│  Surface         : %d cellules%s\n", p->area, p->coastal?" (côtière)":"");
    printf("│  Altitude moy.   : %.2f\n", p->height_avg);
    printf("│  Latitude        : %.2f\n", p->lat);
    printf("│  Ressource       : %s\n", resource_name(p->resource));
    printf("│  Hiérarchie      : région %d · pays %d · continent %d\n",
           (int)p->region, (int)p->country, (int)p->continent);
    /* La culture (langue/valeurs/subsistance/parenté/religion) n'appartient
     * plus à la Province : c'est un attribut de la population régionale
     * (RegionEconomy.culture), absente de ce visualiseur purement géographique. */
    printf("└────────────────────────────────────────────────\n");
    fflush(stdout);
}

/* ---- Barre de status (console, une ligne) ---------------------------- */
static void status_line(const World *w, ViewMode mode, uint32_t seed,
                        int cx, int cy, int selected) {
    printf("\r[%s] graine=%u  ", VIEW_NAMES[mode], seed);
    if (cx >= 0 && cx < SCPS_W && cy >= 0 && cy < SCPS_H) {
        const Cell *c = scps_cellc(w, cx, cy);
        printf("(%3d,%3d) %-18s h=%.2f m=%.2f t=%.2f  prov=%d",
               cx, cy, biome_name(c->biome),
               c->height, c->moisture, c->temperature,
               (int)c->province);
    }
    if (selected >= 0) printf("  [sél: #%d]", selected);
    fflush(stdout);
}

/* ====================================================================== *
 * UI DIÉGÉTIQUE — bandeau royaume + panneau de province (via la membrane)
 *
 * Le viewer ne lit QUE des Readout (bandes + mots) : aucun flottant SCPS,
 * aucun appel à scps_core. Tout passe par country_readout / province_readout
 * + label_X / hover_X.
 * ====================================================================== */

/* ---- Palette (bleu nuit & cuivre, §5.4) — affinée pour la profondeur --- */
static const SDL_Color COL_PANEL    = { 0x0d,0x14,0x20,0xf6 };  /* navy profond (corps de panneau) */
static const SDL_Color COL_PANEL2   = { 0x17,0x23,0x35,0xf6 };  /* navy clair (champs, pastilles) */
static const SDL_Color COL_PANEL_HI = { 0x26,0x36,0x4c,0x4d };  /* voile clair translucide (sheen) */
static const SDL_Color COL_COPPER   = { 0xc8,0x82,0x3e,0xff };  /* cuivre, plus chaud & vif */
static const SDL_Color COL_PARCH    = { 0xed,0xe3,0xcd,0xff };  /* parchemin, un brin plus clair */
static const SDL_Color COL_DIM      = { 0x96,0x8d,0x79,0xff };  /* texte secondaire */
static const SDL_Color COL_EDGE     = { 0x34,0x42,0x57,0xff };  /* bord DOUX (au lieu d'un trait dur) */
static const SDL_Color COL_SHADOW   = { 0x00,0x02,0x05,0x6e };  /* ombre portée (relief) */

/* Bande → couleur de sens (vert favorable → ambre → rouge défavorable). */
static SDL_Color sense_color(float good) {
    if (good < 0.f) good = 0.f;
    if (good > 1.f) good = 1.f;
    SDL_Color c; c.a = 0xff;
    if (good >= 0.5f) { float t=(good-0.5f)*2.f;            /* ambre → vert */
        c.r=(Uint8)(0xc8+(0x6a-0xc8)*t); c.g=(Uint8)(0xa0+(0x9a-0xa0)*t); c.b=(Uint8)(0x4a+(0x5b-0x4a)*t);
    } else { float t=good*2.f;                              /* rouge → ambre */
        c.r=(Uint8)(0xb1+(0xc8-0xb1)*t); c.g=(Uint8)(0x50+(0xa0-0x50)*t); c.b=(Uint8)(0x3c+(0x4a-0x3c)*t);
    }
    return c;
}
static SDL_Color band_good(int idx, int n, bool higher_better) {
    float g = (n>1) ? (float)idx/(float)(n-1) : 0.5f;
    return sense_color(higher_better ? g : 1.f-g);
}

/* ---- Texte (SDL_ttf) -------------------------------------------------- */
static TTF_Font *g_font = NULL, *g_font_big = NULL, *g_font_small = NULL;

/* §BALISES INLINE `#tag …#!` (brief loc §2) — un balisage LÉGER, display-only,
 * qui COLORE un segment sans jamais afficher les marqueurs. La membrane est
 * respectée : ce sont des MOTS habillés, aucun flottant SCPS ne transite ici.
 * Forme : `#tag contenu#!` — `#`, un nom de balise en lettres, UN espace
 * séparateur (consommé), le contenu, puis `#!` qui ferme. La couleur de base
 * (passée par l'appelant) habille tout le texte HORS balise. Vocabulaire fermé
 * (un mot inconnu retombe sur la base) — chaud/froid/faste/alarme/sourd/cuivre. */
static SDL_Color markup_color(const char *tag, size_t len, SDL_Color base){
    struct { const char *name; SDL_Color col; } M[] = {
        { "hot",   sense_color(0.12f) },              /* chaud / défavorable (rouge) */
        { "cold",  (SDL_Color){0x6f,0x9f,0xd8,0xff} },/* froid (bleu glace)          */
        { "good",  sense_color(0.85f) },              /* faste / favorable (vert)    */
        { "bad",   sense_color(0.12f) },              /* alias de hot                */
        { "warn",  COL_COPPER },                      /* alerte (cuivre)             */
        { "dim",   COL_DIM },                         /* sourd                       */
        { "gold",  COL_COPPER },                      /* or / cuivre                 */
    };
    for (size_t i=0;i<sizeof M/sizeof *M;i++)
        if (strlen(M[i].name)==len && strncmp(tag,M[i].name,len)==0) return M[i].col;
    return base;                                       /* balise inconnue : pas d'effet */
}
/* Rend un run [b,e[ dans la couleur c à (x,y) ; renvoie la largeur avancée. */
static int draw_run(SDL_Renderer *ren, TTF_Font *f, int x, int y, SDL_Color c,
                    const char *b, const char *e){
    if (e<=b) return 0;
    int len=(int)(e-b);
    char tmp[512];
    if (len>(int)sizeof tmp-1) len=(int)sizeof tmp-1;
    memcpy(tmp,b,(size_t)len); tmp[len]='\0';
    if (!tmp[0]) return 0;
    SDL_Surface *su = TTF_RenderUTF8_Blended(f, tmp, c);
    if (!su) return 0;
    SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, su);
    SDL_Rect d = { x, y, su->w, su->h };
    int w = su->w;
    SDL_FreeSurface(su);
    if (tx){ SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); }
    return w;
}
static void draw_text(SDL_Renderer *ren, TTF_Font *f, int x, int y, SDL_Color col, const char *s) {
    if (!f || !s || !s[0]) return;
    /* Chemin RAPIDE — aucune balise : rendu d'UNE texture, octet-pour-octet comme
     * avant (la quasi-totalité des appels ; aucune régression de mise en page). */
    if (!strchr(s,'#')){
        SDL_Surface *su = TTF_RenderUTF8_Blended(f, s, col);
        if (!su) return;
        SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, su);
        SDL_Rect d = { x, y, su->w, su->h };
        SDL_FreeSurface(su);
        if (tx) { SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); }
        return;
    }
    /* Chemin BALISÉ — on découpe en runs (base / coloré) et on les pose côte à côte. */
    int cx=x;
    const char *p=s, *run=s;
    while (*p){
        if (p[0]=='#' && p[1]=='!'){            /* fermeture orpheline → on l'avale */
            cx += draw_run(ren,f,cx,y,col,run,p);
            p+=2; run=p; continue;
        }
        if (p[0]=='#' && ((p[1]>='a'&&p[1]<='z')||(p[1]>='A'&&p[1]<='Z'))){
            const char *t=p+1; while ((*t>='a'&&*t<='z')||(*t>='A'&&*t<='Z')) t++;
            if (*t==' '){                       /* balise valide : #tag<espace>contenu#! */
                /* poser d'abord le run de base accumulé */
                cx += draw_run(ren,f,cx,y,col,run,p);
                size_t tlen=(size_t)(t-(p+1));
                SDL_Color tc=markup_color(p+1,tlen,col);
                const char *cont=t+1;           /* après l'espace séparateur */
                const char *end=cont;
                while (*end && !(end[0]=='#'&&end[1]=='!')) end++;
                cx += draw_run(ren,f,cx,y,tc,cont,end);
                p = (*end) ? end+2 : end;        /* saute le contenu + `#!` */
                run=p; continue;
            }
        }
        p++;                                     /* `#` littéral (hors motif) : run continue */
    }
    draw_run(ren,f,cx,y,col,run,p);              /* le reste */
}
static int text_w(TTF_Font *f, const char *s){ int w=0; if (f&&s) TTF_SizeUTF8(f,s,&w,NULL); return w; }
/* P1.8 — texte avec ALPHA (fondu) : TTF_Blended ne lit pas col.a, on module la texture. */
static void draw_text_a(SDL_Renderer *ren, TTF_Font *f, int x, int y, SDL_Color col, Uint8 a, const char *s){
    if (!f || !s || !s[0]) return;
    SDL_Surface *su = TTF_RenderUTF8_Blended(f, s, col);
    if (!su) return;
    SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, su);
    SDL_Rect d = { x, y, su->w, su->h };
    SDL_FreeSurface(su);
    if (tx){ SDL_SetTextureAlphaMod(tx, a); SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); }
}
static void fill_rect(SDL_Renderer *ren, int x,int y,int w,int h, SDL_Color c) {
    SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
    SDL_Rect r={x,y,w,h}; SDL_RenderFillRect(ren,&r);
}
/* Rectangle à COINS ARRONDIS (douceur) — corps en 3 bandes + 4 quarts de disque. */
static void fill_round(SDL_Renderer *ren, int x,int y,int w,int h, SDL_Color c, int r){
    if (r<1){ fill_rect(ren,x,y,w,h,c); return; }
    if (r*2>w) r=w/2;
    if (r*2>h) r=h/2;
    fill_rect(ren, x+r, y, w-2*r, h, c);
    fill_rect(ren, x, y+r, r, h-2*r, c);
    fill_rect(ren, x+w-r, y+r, r, h-2*r, c);
    SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
    for (int dy=0; dy<r; dy++) for (int dx=0; dx<r; dx++){
        if (dx*dx+dy*dy <= r*r){
            SDL_RenderDrawPoint(ren, x+r-1-dx,   y+r-1-dy);
            SDL_RenderDrawPoint(ren, x+w-r+dx,   y+r-1-dy);
            SDL_RenderDrawPoint(ren, x+r-1-dx,   y+h-r+dy);
            SDL_RenderDrawPoint(ren, x+w-r+dx,   y+h-r+dy);
        }
    }
}
/* Contour à coins arrondis (anneau d'un fill_round). */
static void round_box(SDL_Renderer *ren, int x,int y,int w,int h, SDL_Color c, int r){
    if (r<1){ SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
              SDL_Rect rc={x,y,w,h}; SDL_RenderDrawRect(ren,&rc); return; }
    if (r*2>w) r=w/2;
    if (r*2>h) r=h/2;
    SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
    SDL_RenderDrawLine(ren, x+r,y, x+w-r,y);
    SDL_RenderDrawLine(ren, x+r,y+h-1, x+w-r,y+h-1);
    SDL_RenderDrawLine(ren, x,y+r, x,y+h-r);
    SDL_RenderDrawLine(ren, x+w-1,y+r, x+w-1,y+h-r);
    for (int a=0;a<=90;a+=6){
        float rad=a*0.0174533f; int dx=(int)(r*cosf(rad)), dy=(int)(r*sinf(rad));
        SDL_RenderDrawPoint(ren, x+r-dx,   y+r-dy);
        SDL_RenderDrawPoint(ren, x+w-r+dx, y+r-dy);
        SDL_RenderDrawPoint(ren, x+r-dx,   y+h-r+dy);
        SDL_RenderDrawPoint(ren, x+w-r+dx, y+h-r+dy);
    }
}
/* Fond de PANNEAU « smooth » : ombre portée + corps navy arrondi + voile clair en
 * haut (sheen) + une BORDURE ÉPAISSE qui déborde vers l'EXTÉRIEUR (relief, cadre). */
static void panel_bg(SDL_Renderer *ren, int x,int y,int w,int h){
    fill_round(ren, x+4, y+6, w, h, COL_SHADOW, 10);    /* ombre portée, décalée */
    fill_round(ren, x, y, w, h, COL_PANEL, 8);          /* corps arrondi */
    fill_round(ren, x+2, y+2, w-4, h/5, COL_PANEL_HI, 7);/* voile clair en haut */
    round_box(ren, x,   y,   w,   h,   COL_EDGE,   8);  /* liseré intérieur doux */
    round_box(ren, x-1, y-1, w+2, h+2, COL_COPPER, 9);  /* bordure cuivre, débordante… */
    round_box(ren, x-2, y-2, w+4, h+4, COL_COPPER, 10); /* …épaissie vers l'extérieur */
}

/* ===================================================================== */
/* ARBRE DE TECH CONCENTRIQUE — la membrane (TechTreeReadout) → des anneaux */
/* angle = quartier (thème×fonction) · rayon = tier. Aucun flottant de tech : */
/* on ne lit QUE le readout (mots + nombres tangibles).                    */
/* ===================================================================== */
static void draw_ring(SDL_Renderer *ren, int cx, int cy, float r, SDL_Color c){
    SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
    int seg=120; float px=0,py=0;
    for (int i=0;i<=seg;i++){
        float a=(float)i/seg*6.2831853f, x=cx+cosf(a)*r, y=cy+sinf(a)*r;
        if (i>0) SDL_RenderDrawLine(ren,(int)px,(int)py,(int)x,(int)y);
        px=x; py=y;
    }
}
static void draw_box(SDL_Renderer *ren, int x,int y,int w,int h, SDL_Color c){
    fill_rect(ren,x,y,w,1,c); fill_rect(ren,x,y+h-1,w,1,c);
    fill_rect(ren,x,y,1,h,c); fill_rect(ren,x+w-1,y,1,h,c);
}
/* Jauge 0-100 : un dégradé ROUGE(bas)→VERT(haut) avec un marqueur clair à la
 * valeur. SDL n'a pas de gradient → on peint colonne par colonne (sense_color). */
static void draw_gauge(SDL_Renderer *ren, int x,int y,int gw,int gh,int value){
    if (value<0) value=0;
    if (value>100) value=100;
    for (int i=0;i<gw;i++){
        float t = (gw>1)? (float)i/(gw-1) : 0.f;     /* 0=rouge … 1=vert */
        fill_rect(ren, x+i, y, 1, gh, sense_color(t));
    }
    round_box(ren, x-1, y-1, gw+2, gh+2, COL_EDGE, 3);
    int mx = x + (int)(value/100.f*(gw-1));
    fill_round(ren, mx-1, y-2, 3, gh+4, COL_PARCH, 1);   /* le curseur à la valeur */
}
/* Camembert : des PARTS (percent[]) en couleurs (cols[]), peint disque par
 * pixel (SDL n'a pas de remplissage d'arc) — 0 en haut, sens horaire. */
static void draw_pie(SDL_Renderer *ren, int cx,int cy,int r,
                     const int *percent, const SDL_Color *cols, int n){
    for (int dy=-r; dy<=r; dy++) for (int dx=-r; dx<=r; dx++){
        if (dx*dx+dy*dy > r*r) continue;
        float a = atan2f((float)dx, (float)-dy);      /* 0 en haut */
        if (a<0) a += 6.2831853f;
        float frac100 = a/6.2831853f*100.f;           /* 0..100 horaire */
        SDL_Color c = COL_PANEL2; int acc=0;
        for (int i=0;i<n;i++){ acc+=percent[i]; if (frac100 < acc){ c=cols[i]; break; } }
        SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
        SDL_RenderDrawPoint(ren, cx+dx, cy+dy);
    }
    draw_ring(ren, cx, cy, (float)r, COL_DIM);
}
/* Palette de parts (camemberts, barres empilées) — cuivre, teals, parchemin… */
static const SDL_Color SLICE_PAL[8] = {
    {0xb8,0x73,0x33,0xff}, {0x4e,0x8d,0x8a,0xff}, {0xc9,0xa2,0x4b,0xff}, {0x7a,0x5c,0x99,0xff},
    {0x9a,0x8f,0x78,0xff}, {0x5f,0x8a,0xb0,0xff}, {0xa8,0x5a,0x5a,0xff}, {0x6f,0x9a,0x5a,0xff},
};
/* Un VISAGE : cercle + yeux + bouche parabolique dont la courbure suit l'humeur
 * (0 = triste/∩, 1 = content/∪). Allumé = en couleur ; éteint = gris muet. */
static void draw_face(SDL_Renderer *ren, int cx,int cy,int r, float mood, bool lit){
    SDL_Color c = lit ? sense_color(mood) : (SDL_Color){0x4a,0x52,0x5e,0xff};
    draw_ring(ren, cx, cy, (float)r, c);
    fill_rect(ren, cx-r/2,   cy-r/4, 2,2, c);     /* œil gauche */
    fill_rect(ren, cx+r/2-1, cy-r/4, 2,2, c);     /* œil droit */
    float curve=(mood-0.5f)*2.f;                  /* -1 = grimace … +1 = sourire */
    int span=r/2, my=cy+r/4, prevx=0, prevy=0;
    SDL_SetRenderDrawColor(ren, c.r,c.g,c.b,c.a);
    for (int k=0;k<=8;k++){
        float t=(float)k/8.f*2.f-1.f;             /* -1..1 */
        int mxk=cx+(int)(t*span);
        int myk=my+(int)(curve*(r/3.f)*(1.f-t*t));
        if (k>0) SDL_RenderDrawLine(ren, prevx,prevy, mxk,myk);
        prevx=mxk; prevy=myk;
    }
}
static void zone_add(SDL_Rect r, const char *def);   /* (défini plus bas — survol) */
/* survol : nom + EFFET de chaque nœud (mots de jeu) ; positions pour la capture. */
static char g_tree_hov[TECH_COUNT][240];
static int  g_tree_x[TECH_COUNT], g_tree_y[TECH_COUNT], g_tree_demo;
static int  g_tree_open = -1;                 /* tech dont l'anneau de SOUS-TECHS est ouvert (clic) ; -1 = aucun */
/* P5.26 — RECHERCHE DU JOUEUR : la TechId visée (clic sur l'arbre), -1 = aucune.
 * File de 1 : une seule recherche à la fois, comme l'IA. Déclarée ICI car l'arbre
 * (draw_tech_tree) ET la boucle (sim_day) la lisent — l'en-tête montre la cible. */
static int  g_research_target=-1;
#define SYNC_HOV_SZ 200
static char g_sync_hov[SYNC_COUNT][SYNC_HOV_SZ];   /* survol 2-colonnes des sous-techs */
static int  sync_children(int techid, int *out){   /* indices des nœuds syncrétiques pendant de `techid` */
    int n=0; for (int k=0;k<SYNC_COUNT;k++) if ((int)tech_sync_node(k)->parent==techid) out[n++]=k; return n;
}
static void draw_tech_tree(SDL_Renderer *ren, int win_w, int win_h,
                           WorldEconomy *econ, TechState *ts, World *w,
                           const RouteNetwork *rn, int cid){
    fill_rect(ren, 0,0, win_w, win_h, (SDL_Color){0x0a,0x0e,0x16,0xff});
    if (cid<0 || cid>=w->n_countries) return;
    TechTreeReadout tr;
    ai_sync_refresh(w, econ, rn, &ts[cid], cid);   /* §syncrétique : cercle à jour à l'image (hors cadence IA) */
    unsigned acc = ai_race_access(w, econ, rn, cid);
    float    pop = ai_country_population(w, econ, cid);
    tech_tree_readout(&ts[cid], acc, pop, &tr);

    int cx=win_w/2, cy=win_h/2 - 4;
    float ring = (float)win_h * 0.082f;            /* 4 anneaux (tiers 1..4) tiennent dans la hauteur */
    float GAP  = 0.45f*ring;                        /* écart du POINT central au 1er anneau */
    const float D2R=0.01745329f, TOP=-1.5707963f;  /* quartier 0 au sommet */
    SDL_Color tcol[3] = { {0x5a,0x86,0xd8,0xff}, {0xd8,0x86,0x42,0xff}, {0x5c,0xb8,0x6e,0xff} };

    for (int t=1;t<=4;t++) draw_ring(ren,cx,cy, GAP+t*ring, COL_PANEL2); /* anneaux = tiers (1..4) */
    for (int q=0;q<=9;q++){                                              /* rayons : thèmes (cuivre) & quartiers */
        float a=(q*40.f)*D2R + TOP; SDL_Color c=(q%3==0)?COL_COPPER:COL_PANEL2;
        SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
        SDL_RenderDrawLine(ren, cx+(int)(cosf(a)*ring*0.30f),     cy+(int)(sinf(a)*ring*0.30f),
                                cx+(int)(cosf(a)*(GAP+4.4f*ring)),cy+(int)(sinf(a)*(GAP+4.4f*ring)));
    }
    /* LE CENTRE (anneau 0) = un POINT. Les 6 bâtiments de base s'y logent ; le
     * survol les décrit (ils sont le départ, acquis d'emblée). */
    static char center_hov[300];
    { int p=snprintf(center_hov,sizeof center_hov,
        "Le centre (anneau 0) — les 6 bâtiments de base, acquis au départ : ");
      bool first=true;
      for (int i=0;i<tr.n && p<(int)sizeof center_hov-2;i++) if (tr.node[i].is_base){
          p += snprintf(center_hov+p, sizeof center_hov-p, "%s%s", first?"":" · ", tr.node[i].name);
          first=false;
      } }
    fill_rect(ren, cx-4, cy-4, 8, 8, COL_COPPER);                        /* le point central */
    draw_box(ren, cx-6, cy-6, 12, 12, COL_PARCH);
    zone_add((SDL_Rect){cx-9,cy-9,18,18}, center_hov);

    int cnt[9][8]={{0}}, seen[9][8]={{0}};
    for (int i=0;i<tr.n;i++){ int q=tr.node[i].quarter,t=tr.node[i].tier; if(q>=0&&q<9&&t>=1&&t<8)cnt[q][t]++; }
    g_tree_demo=-1;
    for (int i=0;i<tr.n;i++){
        const TreeNodeReadout *nd=&tr.node[i];
        if (nd->is_base) continue;                                       /* les bases SONT le centre */
        int q=nd->quarter,t=nd->tier; if(q<0||q>=9||t<1||t>=8) continue;
        int k=cnt[q][t], j=seen[q][t]++;
        float off=(k>1)? ((float)j-(k-1)/2.f)*(40.f/(k+1.f)) : 0.f;
        float ang=(q*40.f+20.f+off)*D2R + TOP, rad=GAP+t*ring;
        int x=cx+(int)(cosf(ang)*rad), y=cy+(int)(sinf(ang)*rad), theme=q/3;
        g_tree_x[i]=x; g_tree_y[i]=y;
        SDL_Color c=tcol[theme];
        if (nd->state==TREE_LOCKED){ c.r/=3;c.g/=3;c.b/=3; }
        else if (nd->state==TREE_OPEN){ c.r=(uint8_t)((c.r+255)/2);c.g=(uint8_t)((c.g+255)/2);c.b=(uint8_t)((c.b+255)/2); }
        int sz=5;
        fill_rect(ren,x-sz,y-sz,sz*2,sz*2,c);
        if (nd->faustian) draw_box(ren,x-sz-2,y-sz-2,sz*2+4,sz*2+4,(SDL_Color){0xe0,0x44,0x30,0xff});
        else if (nd->orphan) draw_box(ren,x-sz-2,y-sz-2,sz*2+4,sz*2+4,(SDL_Color){0x80,0x80,0x80,0xff});
        if (sync_children(i,(int[SYNC_COUNT]){0})>0)                      /* a des SOUS-TECHS : cliquable (anneau) */
            draw_ring(ren, x, y, (float)(sz+5), COL_COPPER);
        if (g_font_small){                                               /* NOM compact sous la bulle ; le détail au survol */
            SDL_Color lc = (nd->state==TREE_DONE)?COL_PARCH
                         : (nd->state==TREE_OPEN)?COL_DIM : (SDL_Color){0x5b,0x56,0x4b,0xff};
            int lw=text_w(g_font_small,nd->name);
            draw_text(ren,g_font_small, x-lw/2, y+sz+1, lc, nd->name);
        }
        /* SURVOL 2 COLONNES : titre = le bâtiment ; à GAUCHE le PRIX (+état), à DROITE l'EFFET. */
        snprintf(g_tree_hov[i],sizeof g_tree_hov[i], "%s%s%s\x1f" "coût %d pts · %s%s\x1f%s",
                 nd->name,
                 strcmp(nd->name,nd->unlocks)? " · bâtit " : "",
                 strcmp(nd->name,nd->unlocks)? nd->unlocks : "",
                 nd->cost, label_tree_state(nd->state),
                 nd->orphan? " · orpheline" : "",
                 nd->effet);
        zone_add((SDL_Rect){x-sz-3,y-sz-3,sz*2+6,sz*2+6}, g_tree_hov[i]);
        if (nd->faustian && (g_tree_demo<0 || nd->state==TREE_DONE)) g_tree_demo=i;  /* un faustien pour la démo */
    }
    for (int th=0;th<3;th++){                                            /* étiquettes de thème EXCENTRÉES (hors des nœuds) */
        float a=(th*120.f+60.f)*D2R + TOP, rl=GAP+4.75f*ring;
        int lx=cx+(int)(cosf(a)*rl), ly=cy+(int)(sinf(a)*rl);
        draw_text(ren,g_font_big,lx-text_w(g_font_big,tr.theme[th])/2,ly-9,tcol[th],tr.theme[th]);
    }
    /* ── ANNEAU SYNCRÉTIQUE (§11/§12) — au CLIC sur une tech, ses SOUS-TECHS (diffusion par
     *    contact) s'ouvrent en anneau autour d'elle ; disponibles seulement si la parente est
     *    ACQUISE. Pastille = état d'accès ; survol = hover 2-colonnes (profondeur · capacité). ── */
    if (g_tree_open>=0 && g_tree_open<TECH_COUNT &&
        (g_tree_x[g_tree_open] || g_tree_y[g_tree_open])){
        int kids[SYNC_COUNT], nk = sync_children(g_tree_open, kids);
        if (nk>0){
            int ox=g_tree_x[g_tree_open], oy=g_tree_y[g_tree_open];
            bool parent_done = ts[cid].unlocked[g_tree_open];
            float rr = ring*0.66f;
            draw_ring(ren, ox,oy, rr, COL_COPPER);                       /* l'anneau des sous-techs */
            for (int j=0;j<nk;j++){
                float a = TOP + (nk>1 ? (j-(nk-1)/2.f)*0.85f : 0.f);     /* en éventail au-dessus du parent */
                int x=ox+(int)(cosf(a)*rr), y=oy+(int)(sinf(a)*rr);
                SDL_RenderDrawLine(ren, ox,oy, x,y);                      /* trait parent → sous-tech */
                SyncReadout sr = sync_node_readout(&ts[cid], kids[j]);
                SDL_Color c = parent_done ? band_good((int)sr.acces,4,true) : (SDL_Color){0x44,0x40,0x38,0xff};
                int sz=6; fill_rect(ren,x-sz,y-sz,sz*2,sz*2,c);
                draw_box(ren,x-sz-1,y-sz-1,sz*2+2,sz*2+2,COL_PARCH);
                if (g_font_small){ int lw=text_w(g_font_small,sr.nom);
                    draw_text(ren,g_font_small,x-lw/2,y+sz+1,parent_done?COL_PARCH:COL_DIM,sr.nom); }
                /* hover 2 colonnes : GAUCHE = accès · profondeur requise ; DROITE = la capacité diffusée. */
                snprintf(g_sync_hov[kids[j]],SYNC_HOV_SZ,"%s\x1f%s · %s\x1f%s",
                         sr.nom,
                         parent_done? label_acces(sr.acces) : "parente requise",
                         label_profondeur(sr.requise),
                         tech_sync_node(kids[j])->unlocks);
                zone_add((SDL_Rect){x-sz-3,y-sz-3,sz*2+6,sz*2+6}, g_sync_hov[kids[j]]);
            }
        }
    }
    /* P5.30 — l'en-tête porte le STRICT NÉCESSAIRE (pays · points · cible en cours) ;
     * les LÉGENDES de prose (anneaux, couleurs, cadres) sont PURGÉES du bas — tout
     * vit désormais au survol (nœud → effet & prix ; secteurs déjà labellisés). */
    char hdr[260];
    if (g_research_target>=0 && g_research_target<TECH_COUNT && tr.points>=0)
        snprintf(hdr,sizeof hdr,"ARBRE DE TECH — %s   ·   %d points de recherche   ·   en cours : %s",
                 w->country[cid].name, tr.points, tech_node((TechId)g_research_target)?tech_node((TechId)g_research_target)->name:"—");
    else
        snprintf(hdr,sizeof hdr,"ARBRE DE TECH — %s   ·   %d points de recherche   ·   clic = lancer · survol = effet & prix",
                 w->country[cid].name, tr.points);
    draw_text(ren,g_font_big,18,12,COL_COPPER,hdr);
}

/* ---- Zones de survol → « un mot, une définition » --------------------- */
typedef struct { SDL_Rect r; const char *def; } HoverZone;
static HoverZone g_zones[160]; static int g_nzones;
static void zone_reset(void){ g_nzones=0; }
static void zone_add(SDL_Rect r, const char *def){
    if (g_nzones<160 && def){ g_zones[g_nzones].r=r; g_zones[g_nzones].def=def; g_nzones++; }
}
static const char *zone_hit(int mx,int my){
    for (int i=0;i<g_nzones;i++){ SDL_Rect *r=&g_zones[i].r;
        if (mx>=r->x && mx<r->x+r->w && my>=r->y && my<r->y+r->h) return g_zones[i].def; }
    return NULL;
}
/* SLOTS DE BÂTIMENT cliquables (§4 panneau) : le panneau les pose chaque frame,
 * la boucle d'évènements les teste au clic (vide → bâtir l'édifice). */
typedef struct { SDL_Rect r; int reg; int edifice; } BuildSlot;
static BuildSlot g_bslots[8]; static int g_nbslots;
static void bslot_reset(void){ g_nbslots=0; }
static void bslot_add(SDL_Rect r, int reg, int edifice){
    if (g_nbslots<8){ g_bslots[g_nbslots].r=r; g_bslots[g_nbslots].reg=reg; g_bslots[g_nbslots].edifice=edifice; g_nbslots++; }
}
/* LIGNES de l'OUTLINER cliquables : clic sur la ligne → saute à la région ; clic
 * sur le marteau → bâtir. `prov` = province à sélectionner ; `hammer_reg` = région
 * où bâtir (-1 = pas de marteau). */
typedef struct { SDL_Rect row; SDL_Rect ham; int prov; int hammer_reg; } OutRow;
static OutRow g_orows[80]; static int g_norows;
static SDL_Rect g_refill_btn; static int g_refill_owner = -1;   /* bouton « remplir » l'armée */
static void orow_reset(void){ g_norows=0; g_refill_owner=-1; }
static void orow_add(SDL_Rect row, SDL_Rect ham, int prov, int hammer_reg){
    if (g_norows<80){ g_orows[g_norows].row=row; g_orows[g_norows].ham=ham;
                      g_orows[g_norows].prov=prov; g_orows[g_norows].hammer_reg=hammer_reg; g_norows++; }
}
/* BOUTONS de mode de carte (§5) : posés chaque frame, testés au clic. */
typedef struct { SDL_Rect r; int mode; } ModeBtn;
static ModeBtn g_modebtns[6]; static int g_nmodebtns;
static void modebtn_reset(void){ g_nmodebtns=0; }
static void modebtn_add(SDL_Rect r, int mode){
    if (g_nmodebtns<6){ g_modebtns[g_nmodebtns].r=r; g_modebtns[g_nmodebtns].mode=mode; g_nmodebtns++; }
}
/* §1 — Le bandeau est un SOMMAIRE : chaque ressource OUVRE son système (clic). */
enum { SYS_FINANCES=0, SYS_SUBSISTANCE, SYS_CHAINES, SYS_TECH, SYS_DIPLO };
typedef struct { SDL_Rect r; int sys; } TopBtn;
static TopBtn g_topbtns[8]; static int g_ntopbtns;
static void topbtn_reset(void){ g_ntopbtns=0; }
static void topbtn_add(SDL_Rect r, int sys){
    if (g_ntopbtns<8){ g_topbtns[g_ntopbtns].r=r; g_topbtns[g_ntopbtns].sys=sys; g_ntopbtns++; }
}
/* Teintes diégétiques par culture (éthos) et par foi (branche) — le viewer les
 * calcule, le renderer les blende (membrane : pas de flottant SCPS au rendu). */
static uint32_t ethos_tint(int e){
    static const uint32_t P[6]={0xFFb0413a,0xFFc06a2e,0xFFc9a24b,0xFF4e8d8a,0xFF5f8ab0,0xFF6f9a5a};
    return P[(e>=0&&e<6)?e:0];
}
static uint32_t faith_tint(int b){
    static const uint32_t P[4]={0xFF6f9a5a,0xFFc9a24b,0xFF7a5c99,0xFF5f8ab0};
    return P[(b>=0&&b<4)?b:0];
}

/* ---- Sim branchée (snapshot de N ticks, déterministe par graine) ------ */
typedef struct {
    WorldEconomy    *econ;
    WorldProsperity *wp;
    WorldLegitimacy *wl;
    TradeNetwork    *net;
    TechState       *ts;
    Statecraft      *sc;
    AgencyState     *ag;       /* file d'actions (joueur ET IA) — en jours */
    EventsState     *ev;       /* chocs / évènements / âges */
    ModifierStack   *drift;    /* pile de dérive démographique */
    LaborEcon       *labor;    /* économie de pop du joueur (topbar) */
    DiploState      *dp;       /* relations / guerres */
    RouteNetwork    *rn;       /* routes commerciales */
    RevoltState     *rs;       /* soulèvements incarnés (sécessions, coups) */
    WarHost         *host;     /* armées levées par pays (mobilisation) */
    Campaign        *camp;     /* armées de campagne : marche/siège/bataille sur la carte (non-invasif) */
    uint32_t         camp_rng;
    MissionsState   *missions; /* missions décennales (rythme + injection de ressources) */
    NavyState       *navy;     /* la flotte (mer §5) : coques, chantier, entretien */
    EndgameState    *eg;       /* capstone §27 : état cataclysme (entropie + fin + merveille) */
    int              prev_dawned; /* dernier âge avéné traité (engagement d'âge §7) */
    AiActor         *ai;       /* un acteur IA par pays voisin (cadence étalée) */
    bool            *ai_on;    /* ce pays est-il piloté par l'IA ? */
    int16_t          prev_owner_mo[SCPS_MAX_REG];  /* propriétaires du mois (détection de conquête) */
    int              day;      /* jour de jeu (1 tick = 1 jour) */
    int              year;
    int              player;   /* pays du joueur */
    bool             ready;
} Sim;

/* ═══ N3.1 — FRONTIÈRES en espace écran ════════════════════════════════════
 * Les flags par cellule ne savent faire qu'UNE cellule de large (la hiérarchie
 * pays/province n'y était qu'un fondu). Ici : les frontières sont des LISTES DE
 * SEGMENTS en coordonnées de COIN de cellule, extraites au BAKE (rebâties au
 * changement de propriétaire, jamais par frame), puis STROKÉES par-dessus le
 * terrain en LARGEUR ÉCRAN CONSTANTE — pays 5 px, région 3 px, province 2 px,
 * à TOUT zoom. Recalculable au chargement → rien en sauvegarde. Membrane : on
 * ne lit que des IDS (région/province/owner), jamais un flottant SCPS. */
typedef struct { uint16_t x0,y0,x1,y1; int16_t ra,rb; } BSeg;  /* arête (coins, unités-cellule) + régions riveraines (-1 = mer/vierge) */
enum { BL_PROV=0, BL_REG=1, BL_CTY=2, BL_COUNT=3 };            /* niveau le plus FORT du joint : pays > région > province */
static BSeg   *g_bseg[BL_COUNT];
static int     g_bseg_n[BL_COUNT], g_bseg_cap[BL_COUNT];
static int16_t g_bseg_own[SCPS_MAX_REG+1];   /* photo owner-effectif + n_regions : l'invalidation */
static uint32_t g_bseg_seed = 0;             /* régénération (R) → monde neuf → rebâtir */
static bool    g_bseg_built = false;

static void bseg_push(int lvl, int x0,int y0,int x1,int y1, int ra,int rb){
    if (g_bseg_n[lvl] >= g_bseg_cap[lvl]){
        int nc = g_bseg_cap[lvl] ? g_bseg_cap[lvl]*2 : 4096;
        BSeg *nb = (BSeg*)realloc(g_bseg[lvl], (size_t)nc*sizeof(BSeg));
        if (!nb) return;                                /* OOM : frontière incomplète, pas de crash */
        g_bseg[lvl]=nb; g_bseg_cap[lvl]=nc;
    }
    BSeg *sg=&g_bseg[lvl][g_bseg_n[lvl]++];
    sg->x0=(uint16_t)x0; sg->y0=(uint16_t)y0; sg->x1=(uint16_t)x1; sg->y1=(uint16_t)y1;
    sg->ra=(int16_t)ra;  sg->rb=(int16_t)rb;
}
/* L'owner EFFECTIF d'une cellule — la MÊME lecture que la teinte politique
 * (owner valide d'une région colonisée), -1 sinon (mer, terre vierge). */
static int bseg_owner_of(const Sim *s, const World *w, const Cell *c){
    if (!c || c->region<0 || c->region>=s->econ->n_regions || c->region>=SCPS_MAX_REG) return -1;
    int ow=s->econ->region[c->region].owner;
    return (ow>=0 && ow<w->n_countries && s->econ->region[c->region].colonized) ? ow : -1;
}
static void bseg_build(const World *w, const Sim *s){
    for (int l=0;l<BL_COUNT;l++) g_bseg_n[l]=0;
    /* Balayage des joints E et S : chaque arête interne vue UNE fois (dédup par
     * construction). Hors-grille = voisin owner -1 → le contour d'un pays se FERME
     * aussi au bord de carte. Le joint est classé à son niveau le plus FORT
     * (country > region > prov) : un joint partagé n'est jamais tracé deux fois. */
    for (int y=0;y<SCPS_H;y++) for (int x=0;x<SCPS_W;x++){
        const Cell *a=scps_cellc(w,x,y);
        int own_a=bseg_owner_of(s,w,a);
        for (int d=0;d<2;d++){                          /* d : 0 = arête EST, 1 = arête SUD */
            int nx2=x+(d==0), ny2=y+(d==1);
            const Cell *b=(nx2<SCPS_W && ny2<SCPS_H) ? scps_cellc(w,nx2,ny2) : NULL;
            int own_b = b ? bseg_owner_of(s,w,b) : -1;
            int rga=a->region,   rgb=b?b->region:-1;
            int pva=a->province, pvb=b?b->province:-1;
            int lvl=-1;
            if (own_a!=own_b && (own_a>=0||own_b>=0)) lvl=BL_CTY;  /* pays↔pays OU contour externe (vierge/mer = côte politique) */
            else if (rga!=rgb && rga>=0 && rgb>=0)    lvl=BL_REG;
            else if (pva!=pvb && pva>=0 && pvb>=0)    lvl=BL_PROV;
            if (lvl<0) continue;
            if (d==0) bseg_push(lvl, x+1,y,   x+1,y+1, rga,rgb);   /* arête verticale partagée */
            else      bseg_push(lvl, x,  y+1, x+1,y+1, rga,rgb);   /* arête horizontale partagée */
        }
        /* bords OUEST/NORD de la grille : jamais vus par le balayage E/S */
        if (x==0 && own_a>=0) bseg_push(BL_CTY, 0,y, 0,y+1, a->region,-1);
        if (y==0 && own_a>=0) bseg_push(BL_CTY, x,0, x+1,0, a->region,-1);
    }
    g_bseg_built=true;
}
/* Rebâtit SEULEMENT si la photo de souveraineté a changé (colonisation, conquête à
 * la paix, sécession) ou si le monde a été régénéré — jamais par frame. */
static void bseg_check(const World *w, const Sim *s){
    int16_t now[SCPS_MAX_REG+1];
    int nr=s->econ->n_regions; if (nr>SCPS_MAX_REG) nr=SCPS_MAX_REG;
    for (int r=0;r<SCPS_MAX_REG;r++){
        int ow=-1;
        if (r<nr){ ow=s->econ->region[r].owner;
                   if (!(ow>=0 && ow<w->n_countries && s->econ->region[r].colonized)) ow=-1; }
        now[r]=(int16_t)ow;
    }
    now[SCPS_MAX_REG]=(int16_t)nr;
    if (g_bseg_built && g_bseg_seed==w->seed && memcmp(now,g_bseg_own,sizeof now)==0) return;
    memcpy(g_bseg_own,now,sizeof now); g_bseg_seed=w->seed;
    bseg_build(w,s);
}

/* ── Tracé : quads batchés (UN SDL_RenderGeometry par niveau — AA correct,
 *    largeur écran exacte). Repli < SDL 2.0.18 : 3 lignes parallèles. ── */
#if SDL_VERSION_ATLEAST(2,0,18)
static SDL_Vertex *g_bv;  static int g_bv_n,  g_bv_cap;
static int        *g_bvi; static int g_bvi_n, g_bvi_cap;
static bool bv_reserve(int nv,int ni){
    if (g_bv_n+nv>g_bv_cap){ int nc=g_bv_cap?g_bv_cap*2:8192; while(nc<g_bv_n+nv)nc*=2;
        SDL_Vertex *p=(SDL_Vertex*)realloc(g_bv,(size_t)nc*sizeof(SDL_Vertex)); if(!p)return false; g_bv=p; g_bv_cap=nc; }
    if (g_bvi_n+ni>g_bvi_cap){ int nc=g_bvi_cap?g_bvi_cap*2:16384; while(nc<g_bvi_n+ni)nc*=2;
        int *p=(int*)realloc(g_bvi,(size_t)nc*sizeof(int)); if(!p)return false; g_bvi=p; g_bvi_cap=nc; }
    return true;
}
static void bv_quad(float ax,float ay,float bx,float by,float hw,SDL_Color col){
    float dx=bx-ax, dy=by-ay, L=sqrtf(dx*dx+dy*dy); if (L<0.001f) return;
    float nx=-dy/L*hw, ny=dx/L*hw;
    if (!bv_reserve(4,6)) return;
    int b0=g_bv_n;
    SDL_Vertex *v=&g_bv[g_bv_n]; g_bv_n+=4;
    v[0].position.x=ax+nx; v[0].position.y=ay+ny; v[1].position.x=ax-nx; v[1].position.y=ay-ny;
    v[2].position.x=bx+nx; v[2].position.y=by+ny; v[3].position.x=bx-nx; v[3].position.y=by-ny;
    for (int i=0;i<4;i++){ v[i].color=col; v[i].tex_coord.x=0; v[i].tex_coord.y=0; }
    int *ix=&g_bvi[g_bvi_n]; g_bvi_n+=6;
    ix[0]=b0; ix[1]=b0+1; ix[2]=b0+2; ix[3]=b0+2; ix[4]=b0+1; ix[5]=b0+3;
}
/* Bout ROND (hexagone plein, rayon ½ largeur) : posé aux extrémités de segment —
 * aux jonctions ≥3 frontières il bouche le trou de coin, en ligne droite il
 * disparaît SOUS le trait. La couleur du niveau le plus fort domine (tracé après). */
static void bv_disc(float cx,float cy,float r,SDL_Color col){
    if (!bv_reserve(7,18)) return;
    int b0=g_bv_n;
    SDL_Vertex *v=&g_bv[g_bv_n]; g_bv_n+=7;
    v[0].position.x=cx; v[0].position.y=cy;
    for (int i=0;i<6;i++){
        float a=(float)i*1.04719755f;   /* π/3 (M_PI n'est pas C99 strict) */
        v[1+i].position.x=cx+cosf(a)*r; v[1+i].position.y=cy+sinf(a)*r;
    }
    for (int i=0;i<7;i++){ v[i].color=col; v[i].tex_coord.x=0; v[i].tex_coord.y=0; }
    int *ix=&g_bvi[g_bvi_n]; g_bvi_n+=18;
    for (int i=0;i<6;i++){ ix[i*3]=b0; ix[i*3+1]=b0+1+i; ix[i*3+2]=b0+1+((i+1)%6); }
}
static void bv_flush(SDL_Renderer *ren){
    if (g_bv_n) SDL_RenderGeometry(ren,NULL,g_bv,g_bv_n,g_bvi,g_bvi_n);
    g_bv_n=0; g_bvi_n=0;
}
#endif
typedef struct { const Cam *cam; int win_w,win_h; } BView;
static void bseg_stroke_one(SDL_Renderer *ren, const BView *bv, const BSeg *sg, float wpx, SDL_Color col){
    float X0,Y0,X1,Y1;
    cam_project(bv->cam,(float)sg->x0,(float)sg->y0,&X0,&Y0);
    cam_project(bv->cam,(float)sg->x1,(float)sg->y1,&X1,&Y1);
    if ((X0<-wpx&&X1<-wpx)||(X0>bv->win_w+wpx&&X1>bv->win_w+wpx)||
        (Y0<-wpx&&Y1<-wpx)||(Y0>bv->win_h+wpx&&Y1>bv->win_h+wpx)) return;   /* cull viewport */
#if SDL_VERSION_ATLEAST(2,0,18)
    bv_quad(X0,Y0,X1,Y1,wpx*0.5f,col);
    if (wpx>=3.f){ bv_disc(X0,Y0,wpx*0.5f,col); bv_disc(X1,Y1,wpx*0.5f,col); }  /* bouts ronds (jonctions) */
    (void)ren;
#else
    SDL_SetRenderDrawColor(ren,col.r,col.g,col.b,col.a);
    float dx=X1-X0, dy=Y1-Y0, L=sqrtf(dx*dx+dy*dy); if (L<0.001f) return;
    float nx=-dy/L, ny=dx/L;
    int n=(int)wpx; if (n<1) n=1;
    for (int i=0;i<n;i++){
        float off=((float)i-(float)(n-1)*0.5f);
        SDL_RenderDrawLine(ren,(int)(X0+nx*off),(int)(Y0+ny*off),(int)(X1+nx*off),(int)(Y1+ny*off));
    }
#endif
}
static void bseg_draw_level(SDL_Renderer *ren, const BView *bv, int lvl, float wpx, uint32_t argb){
    SDL_Color col={ (Uint8)(argb>>16),(Uint8)(argb>>8),(Uint8)argb,(Uint8)(argb>>24) };
    for (int i=0;i<g_bseg_n[lvl];i++) bseg_stroke_one(ren,bv,&g_bseg[lvl][i],wpx,col);
#if SDL_VERSION_ATLEAST(2,0,18)
    bv_flush(ren);
#endif
}
/* La SÉLECTION (z-order §2c : AU-DESSUS des strokes politiques) : le contour de la
 * région choisie, doré — tous niveaux confondus (une seule rive est la région). */
static void bseg_draw_selection(SDL_Renderer *ren, const BView *bv, int selreg){
    SDL_Color gold={0xFF,0xDD,0x00,0xFF};
    for (int lvl=0;lvl<BL_COUNT;lvl++)
        for (int i=0;i<g_bseg_n[lvl];i++){
            const BSeg *sg=&g_bseg[lvl][i];
            if ((sg->ra==selreg) == (sg->rb==selreg)) continue;   /* XOR : exactement une rive */
            bseg_stroke_one(ren,bv,sg,3.f,gold);
        }
#if SDL_VERSION_ATLEAST(2,0,18)
    bv_flush(ren);
#endif
}
/* Le point d'entrée par frame : transforme + stroke (quelques milliers de segments,
 * négligeable). Z-ORDER STRICT : province (2px) DESSOUS → région (3px) → pays (5px)
 * DOMINE (couvre les joints coïncidents) → sélection dorée. Strokes JAMAIS sur la
 * mer libre : un segment n'existe qu'au joint d'une terre administrée. */
static void borders_draw(SDL_Renderer *ren, const Cam *cam, const World *w, const Sim *s,
                         ViewMode mode, int selected_prov, int win_w, int win_h){
    if (!s->ready) return;
    if (mode!=VIEW_POLITICAL && mode!=VIEW_REGIONS && mode!=VIEW_COUNTRIES) return;
    bseg_check(w,s);
    BView bv={ cam, win_w, win_h };
    if (mode==VIEW_POLITICAL){
        bseg_draw_level(ren,&bv,BL_PROV,2.f,0xFF1A2230u);
        bseg_draw_level(ren,&bv,BL_REG, 2.f,0xFF1A2230u);   /* en Politique, une limite de région EST une limite de province (pas de 3px ici — il ne vit qu'en vue Régions) */
    } else if (mode==VIEW_REGIONS){
        bseg_draw_level(ren,&bv,BL_REG,3.f,0xFF141A26u);
    }
    bseg_draw_level(ren,&bv,BL_CTY,4.f,0xFF0A0E16u);        /* en DERNIER : domine (4px) */
    if (selected_prov>=0 && selected_prov<w->n_provinces){
        int selreg=w->province[selected_prov].region;
        if (selreg>=0) bseg_draw_selection(ren,&bv,selreg);
    }
}

/* UN JOUR de jeu vivant (§1). Chaque sous-système avance à SA cadence calibrée :
 * agency/évènements/statecraft/labor en JOURS ; l'économie/légitimité/prospérité/
 * démographie à l'ANNÉE (comme les bancs d'essai — on ne dérègle pas le pacing).
 * Le joueur n'est qu'un acteur : ses actions sont déjà en file dans s->ag. */
/* Les armées de CAMPAGNE : chaque pays mobilisé ET en guerre projette sa force vers
 * le front ennemi adjacent (marche §1 → siège → bataille §2/§3). NON-INVASIF : lit
 * econ, ne change JAMAIS la propriété des régions — les armées VIVENT sur la carte
 * (la fondation que l'UI §4 dessine). */
/* L1 — PRIORITÉ DÉFENSE (jumelle de chronicle) : un siège ennemi sur MON sol →
 * mon armée marche À LA RENCONTRE (redirection en route ; une armée fraîche fait
 * SORTIR la garnison de la place assiégée elle-même). */
static void sim_campaign_defense(Sim *s, World *w) {
    (void)w;
    for (int k=0; k<SCPS_MAX_COUNTRY; k++) {
        const FieldArmy *en=&s->camp->army[k];
        if (!en->active || en->phase!=FA_SIEGE) continue;
        if (en->loc<0 || en->loc>=s->econ->n_regions) continue;
        int def=s->econ->region[en->loc].owner;
        if (def<0 || def>=SCPS_MAX_COUNTRY || def==en->owner) continue;
        if (diplo_status(s->dp,def,en->owner)!=DIPLO_WAR) continue;
        if (campaign_active(s->camp,def) && campaign_phase(s->camp,def)!=FA_IDLE){
            campaign_redirect(s->camp, s->econ, s->dp, def, en->loc);
        } else if (warhost_units(s->host,def)>0){
            campaign_order(s->camp, s->econ, def, en->loc, en->loc, &s->host->army[def]);
        }
    }
}

static void sim_campaign_orders(Sim *s, World *w) {
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++) {
        if (campaign_active(s->camp,c) && campaign_phase(s->camp,c)!=FA_IDLE) continue;
        if (warhost_units(s->host, c) <= 0) continue;
        int frontier=-1, target=-1;
        /* B5 — PRIORITÉ DE LIBÉRATION : reprendre par les armes une de mes régions
         * tenue par un occupant ennemi (le siège mené à terme y lève l'occupation). */
        for (int r=0; r<s->econ->n_regions && frontier<0; r++) {
            if (s->econ->region[r].owner!=c) continue;
            int occ=s->dp->occupier[r];
            if (occ<0 || occ==c || diplo_status(s->dp,c,occ)!=DIPLO_WAR) continue;
            for (int sn=0; sn<s->econ->n_regions; sn++) {
                if (!s->econ->adj[r][sn]) continue;
                if (s->econ->region[sn].owner!=c || s->dp->occupier[sn]>=0) continue;
                frontier=sn; target=r; break;
            }
        }
        /* sinon : une frontière chaude (région ennemie adjacente — l'offensive). */
        for (int r=0; r<s->econ->n_regions && frontier<0; r++) {
            if (s->econ->region[r].owner!=c) continue;
            for (int sn=0; sn<s->econ->n_regions; sn++) {
                if (!s->econ->adj[r][sn]) continue;
                int ob=s->econ->region[sn].owner;
                if (ob<0 || ob==c || diplo_status(s->dp,c,ob)!=DIPLO_WAR) continue;
                /* P3/doctrine — on n'attaque qu'avec un AVANTAGE DE FORCE (≥1.2× le
                 * défenseur) : sinon l'assaut s'use sur le relief (la LIBÉRATION de
                 * notre sol, plus haut, n'est pas soumise au seuil). 1.2 = défaut du
                 * tunable BT_ATK_RATIO côté chronique (mêmes décisions en partie). */
                if ((float)warhost_units(s->host,c) < 1.2f*(float)warhost_units(s->host,ob)) continue;
                frontier=r; target=sn; break;
            }
        }
        if (frontier>=0){
            campaign_order(s->camp, s->econ, c, frontier, target, &s->host->army[c]);
        } else if (c!=s->player){
            /* pas de frontière TERRESTRE : la guerre passe la mer si un port, des
             * transports et un chemin existent (mer §6 — contraint par le champ). */
            int port=navy_best_port(w,s->econ,c);
            if (port>=0 && navy_transport_packets_free(s->navy,c)>0){
                int tgt=-1;
                for (int r2=0;r2<s->econ->n_regions && tgt<0;r2++){
                    int ob=s->econ->region[r2].owner;
                    if (ob<0||ob==c||diplo_status(s->dp,c,ob)!=DIPLO_WAR) continue;
                    if (!s->econ->region[r2].coastal) continue;
                    tgt=r2;
                }
                if (tgt>=0)
                    campaign_order_sea(s->camp, w, s->econ, s->navy, c, port, tgt, &s->host->army[c]);
            }
        }
    }
}

static void sim_campaign_year(Sim *s, World *w) {
    /* L1 — la campagne RESPIRE AU MOIS (jumelle de chronicle) : la défense
     * intercepte en route, la récolte tombe au fil de l'an, l'attaquant re-cible. */
    for (int month=0; month<12; month++){
        if (month==0) sim_campaign_orders(s, w);
        sim_campaign_defense(s, w);
        campaign_tick(s->camp, w, s->econ, s->dp, &s->camp_rng, 365.f/12.f);
        campaign_release_transports(s->camp, s->navy);   /* les transports rentrent à la rade */
        /* RÉCOLTE (couche sim, jumelle de chronicle) : un siège mené à terme pose une
         * OCCUPATION réelle (région ennemie tenue) ou LIBÈRE (notre région reprise). La
         * propriété ne bascule qu'à la paix (diplo_settle) ; la campagne reste lectrice. */
        for (int i=0; i<w->n_countries && i<SCPS_MAX_COUNTRY; i++){
            FieldArmy *a=&s->camp->army[i];
            if (a->taken_region<0) continue;
            int reg=a->taken_region; a->taken_region=-1;
            if (reg<0 || reg>=s->econ->n_regions) continue;
            if (s->econ->region[reg].owner==a->owner) diplo_liberate(s->dp, s->econ, reg);
            else                                      diplo_occupy  (s->dp, s->econ, a->owner, reg);
            /* L1 — l'attaquant ne dort pas : l'assiégeant de NOTRE sol d'abord,
             * sinon la frontière suivante. */
            int ntgt=-1;
            for (int k=0;k<SCPS_MAX_COUNTRY && ntgt<0;k++){
                const FieldArmy *en=&s->camp->army[k];
                if (!en->active || en->phase!=FA_SIEGE || en->owner==a->owner) continue;
                if (en->loc<0 || en->loc>=s->econ->n_regions) continue;
                if (s->econ->region[en->loc].owner!=a->owner) continue;
                if (diplo_status(s->dp,a->owner,en->owner)!=DIPLO_WAR) continue;
                ntgt=en->loc;
            }
            for (int sn=0; sn<s->econ->n_regions && ntgt<0; sn++){
                if (!s->econ->adj[reg][sn]) continue;
                int ob=s->econ->region[sn].owner;
                if (ob<0||ob==a->owner||diplo_status(s->dp,a->owner,ob)!=DIPLO_WAR) continue;
                if (s->dp->occupier[sn]==a->owner) continue;
                ntgt=sn;
            }
            if (ntgt>=0) campaign_redirect(s->camp, s->econ, s->dp, a->owner, ntgt);
        }
    }
}

/* État de l'écran (déclaré tôt : sim_day gate l'alerte audio sur GS_PLAYING). */
typedef enum { GS_MENU=0, GS_SETUP, GS_OPENING, GS_PLAYING } GameState;
static GameState g_gs = GS_MENU;
/* §terrain — LA DÉFAITE DU JOUEUR : son royaume réduit à 0 région (polity_death).
 * g_defeat fige les entrées (écran de fin) ; deux sorties : OBSERVER (le monde
 * continue, sidebar en lecture seule) ou MENU. g_observer survit à « Observer ». */
static bool g_defeat=false, g_observer=false;
static int  g_defeat_year=0;
/* P5.27 — INCOME SAVOIR passif (points/MOIS) : lit les bâtiments STAFFÉS de la
 * capitale (province-siège = prov[0] de l'éco du travail), pondérés PAR TIER —
 * T1 0.5 · T2 1 · T3 1.5 · T4 2 /mois, bâtiment plein, PRORATISÉ au staffing
 * (jobs_filled / slots du niveau). La capitale elle-même (structure tierée :
 * ses lettrés) compte comme une source, plafond T4. Lecteur de l'éco du
 * travail — jamais un bonus plat ; pop n'entre PAS (elle ne sert qu'au coût). */
static float player_savoir_income_month(const LaborEcon *lab){
    if (!lab || lab->n_prov<1) return 0.f;
    const LProvince *cap=&lab->prov[0];
    int ct=cap->cap_tier; if(ct<1)ct=1; if(ct>4)ct=4;
    float m = 0.5f*(float)ct;                                   /* la capitale : ses nobles/lettrés */
    for (int b=0;b<cap->n_bld;b++){
        const LBuilding *bd=&cap->bld[b];
        if (bd->type==LB_NONE) continue;
        int slots=building_job_slots(bd->level); if(slots<1)slots=1;
        float staffing=(float)bd->jobs_filled/(float)slots;
        staffing = staffing<0.f ? 0.f : (staffing>1.f ? 1.f : staffing);
        int tier=bd->level+1; if(tier<1)tier=1; if(tier>4)tier=4;
        m += 0.5f*(float)tier*staffing;                         /* 0.5·tier /mois, au prorata */
    }
    return m;
}

static void sim_day(Sim *s, World *w) {
    /* — quotidien — */
    agency_advance(s->ag, w, s->econ, s->wl, s->drift, 1);     /* les actions progressent */
    /* leviers intérieurs : draine les coûts SCPS différés (purge/mater) vers TechState */
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        float ch,fr,hh;
        if (agency_drain_levier_costs(c,&ch,&fr,&hh)){
            s->ts[c].charge+=ch; s->ts[c].fracture+=fr; s->ts[c].H+=hh;
        }
    }
    routes_advance(s->rn, w, s->econ, 1);
    for (int c=0;c<w->n_countries;c++) if (s->ai_on[c]){    /* les voisins VIVENT (cadence étalée) */
        ai_step(&s->ai[c], w, s->econ, s->wp, s->wl, s->ag, s->rn, s->dp, s->sc, s->day);
        ai_research_step(&s->ai[c], &s->ts[c], w, s->econ, s->rn, s->wp, s->day);  /* l'arbre vivant (S1 : + le commerce) */
    }
    /* P5.26/27/28 — RECHERCHE DU JOUEUR : la cible (clic sur l'arbre) progresse,
     * payée par l'INCOME SAVOIR (bâtiments staffés de la capitale, par tier — P5.27)
     * × rendement des INSTITUTIONS Savoir (Scriptorium/Académie/Université) × CLOCHE
     * DE PROSPÉRITÉ (P5.28, lecteur core — jamais un bonus plat) ; coût ×3 (P5.29). File de 1. */
    if (g_research_target>=0 && s->player>=0 && s->player<w->n_countries){
        int pl=s->player;
        float pop = ai_country_population(w, s->econ, pl);
        unsigned access = ai_race_access(w, s->econ, s->rn, pl);
        if (!tech_can_research(&s->ts[pl], (TechId)g_research_target, access)){
            g_research_target=-1;                              /* plus accessible (acquise / prérequis manquant) */
        } else {
            float month   = player_savoir_income_month(s->labor);          /* P5.27 : capital par tier × staffing */
            float yield   = tech_research_yield(&s->ts[pl]);               /* institutions Savoir : ×1..2.5 */
            CountryReadout cr = country_readout(s->wp, s->ts, w, pl);
            float prosp = 0.4f + (float)cr.m_prosperite.value/100.f*1.2f;   /* P5.28 : ×[0.4..1.6] selon la prospérité */
            s->ts[pl].research_points += (month/30.4f) * yield * prosp;     /* /mois → /jour */
            if (s->ts[pl].research_points >= tech_cost((TechId)g_research_target, pop)){
                tech_research(&s->ts[pl], (TechId)g_research_target, access);   /* DÉBLOQUÉ */
                s->ts[pl].research_points = 0.f; g_research_target=-1;          /* file de 1 : terminé */
            }
        }
    }
    world_events_tick(s->ev, w, s->econ, s->wl, s->wp, s->sc, s->rn, s->ts, s->dp, 1);
    labor_tick(s->labor);
    /* navy_tick (chantier + entretien) est passé MENSUEL (bloc plus bas) : pleinement dt-scalé,
     * rien ne le veut au jour — il pesait ~½ du coût/an de la boucle headless. */
    /* — mensuel : ÉCONOMIE + réputation diplomatique (O(n²)) + démographie, tous
     * au pas dt=1/12 → même rythme annuel, mais plus fluide qu'un saut yearly — */
    if (s->day % 30 == 29) {
        econ_apply_country_tech(s->econ, s->ts, SCPS_MAX_COUNTRY);  /* §B1 : techs de prod du pays → prod_mult région */
        statecraft_council_apply(s->sc, w, s->econ, w->seed, 1.f/12.f);  /* Q1 : le Conseil pousse ses ×, paie son or */
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
            if (s->ai_on[c]) statecraft_council_ai(s->sc, w, s->econ, w->seed, c);   /* Q1 : l'IA pourvoit son siège d'éthos */
        econ_tick(s->econ, 1.f/12.f);
        navy_tick(s->navy, w, s->econ, s->dp, 30.f);   /* chantier + entretien : MENSUEL (ex-quotidien) */
        navy_colonize_tick(s->navy, w, s->econ, 30.f, s->player);   /* mer §8 : on découvre ce que la volta touche (le JOUEUR colonise à la main) */
        navy_course_tick(s->navy, w, s->econ, s->dp, s->rn, &s->camp_rng,
                         s->player, 30.f);   /* coques : la course (raids - saignee - blocus - verdicts) */
        navy_interception_tick(s->navy, s->camp, w, s->econ, s->dp, &s->camp_rng);   /* les convois se chassent */
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){   /* IA navale frugale (mer §5) */
            if (!s->ai_on[c]) continue;
            int hr=s->ai[c].home_region;
            if (hr<0||hr>=s->econ->n_regions) continue;
            RegionEconomy *re=&s->econ->region[hr];
            if (re->owner!=c) continue;
            /* V3 — la rade s'ouvre sur la MEILLEURE CÔTE (capitale côtière, sinon la côte
             * la plus peuplée) : un empire à capitale enclavée participe enfin à la mer. */
            int pr=navy_best_coast(w,s->econ,c);
            if (pr>=0 && s->econ->region[pr].build.port<=0.f && s->econ->region[pr].treasury>400.f){
                agency_build(s->ag, s->econ, w, pr, EDI_PORT);
            } else if (navy_best_port(w,s->econ,c)>=0 && s->navy->n[c].build_hull<0){
                if (s->navy->n[c].hull[HULL_TRANSPORT]<2 && re->treasury>500.f)
                    navy_order_build(s->navy, w, s->econ, c, HULL_TRANSPORT);
                else if (s->navy->n[c].hull[HULL_MERCHANT]<1 && re->treasury>700.f)
                    navy_order_build(s->navy, w, s->econ, c, HULL_MERCHANT);
            }
            /* la route maritime DEPUIS LA RADE (le meilleur port, pas forcément la capitale) */
            int hp=navy_best_port(w,s->econ,c);
            if (s->day%180==29 && hp>=0){
                int mine=0;
                for (int i=0;i<s->rn->n;i++){
                    const TradeRoute *t=&s->rn->route[i];
                    if (t->maritime && (t->ra==hp||t->rb==hp)) mine++;
                }
                for (int r2=0;r2<s->econ->n_regions && mine<3;r2++){
                    if (s->econ->region[r2].owner==c||s->econ->region[r2].owner<0) continue;
                    if (!navy_region_is_port(w,s->econ,r2)) continue;
                    if (routes_order(s->rn, w, s->econ, hp, r2, true)){ mine++; break; }
                }
            }
        }
        statecraft_tick(s->sc, w, s->econ, s->wp, s->wl, s->dp, s->rn, 30);
        demography_tick(w, s->econ, s->wl, s->drift, 5.f, 5.f, 1.f/12.f);
        /* E0.1 — la pop a UN propriétaire (la démographie monde) : labor RELIT les
         * strates chaque mois. E0.4 — il PUBLIE ses capitales bâties : le moteur de
         * révolte lit le tier PAYÉ (croître sans bâtir devient un grief réel). */
        labor_resync_pop(s->labor, s->econ);
        labor_publish_capitals(s->labor);
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)   /* E3 : l'IA stockeuse (mensuel) */
            if (s->ai_on[c]) ai_speculate_tick(&s->ai[c], s->econ);
        /* — conquête du mois : un peuple passé sous une couronne ÉTRANGÈRE devient
         *   restif (intégration à zéro, L au plancher) → terreau de sécession. */
        for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
            int16_t no=s->econ->region[r].owner, po=s->prev_owner_mo[r];
            if (po>=0 && no>=0 && no!=po){
                demography_on_conquest(w, s->econ, s->drift, r, no);
                revolt_on_conquest(s->rs, r);
            }
            s->prev_owner_mo[r]=no;
        }
        /* — la révolte INCARNÉE : misère soutenue → soulèvement → sécession/coup/
         *   jacquerie/écrasement ; un pays NÉ d'une sécession prend vie (IA). */
        revolt_scan(s->rs, w, s->econ, s->drift, 30);
        revolt_tick(s->rs, w, s->econ, s->drift, s->wl, s->wp, 30);
        if (s->rs->last_spawned>=0){
            for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (c==s->player || s->ai_on[c]) continue;
                int nreg=0; for (int r=0;r<s->econ->n_regions;r++) if (s->econ->region[r].owner==c) nreg++;
                if (w->country[c].role==POLITY_ANTAGONIST && w->country[c].capital_prov>=0 && nreg>0){
                    s->ai_on[c]=true;
                    ai_actor_init(&s->ai[c], w, s->econ, c, w->seed ^ (uint32_t)(c*2654435761u));
                }
            }
            /* une sécession a changé des propriétaires CE mois : resynchroniser, sinon
             * la détection du mois prochain prendrait l'indépendance pour une invasion. */
            for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)
                s->prev_owner_mo[r]=s->econ->region[r].owner;
        }
    }
    /* — annuel (le tour stratégique) — */
    if (s->day % 365 == 364) {
        econ_colonize_tick(s->econ, w, s->player);
        econ_migrate_tick(s->econ, w);
        world_tick(w, s->econ, 1.0f);
        legitimacy_tick(s->wl, w, s->econ, s->ts);
        trade_network_build(s->net, w, s->econ);
        trade_tick(s->econ, s->net);
        intertrade_tick(s->econ, s->rn, s->dp);   /* grandes routes marchandes (goods inter-pays + embargo) */
        demography_contact_tick(s->econ, s->drift, s->rn, s->dp, 5.f, 5.f, 1.f);   /* S2 : la cristallisation suit le contact (annuel) */
        prosperity_tick(s->wp, w, s->econ, s->net, s->ts, s->wl);
        if (s->eg) endgame_tick(s->eg, w, s->econ, s->wp, s->ts, s->rn, s->navy, s->dp, s->camp, s->player, s->year);
        /* Diplomatie annuelle : usure de guerre, fonte des trêves/momentum, score de guerre. */
        warhost_tick(s->host, w, s->econ, s->dp, s->ts, 1.0f);   /* la mobilisation : les armées vivent */
        sim_campaign_year(s, w);                           /* … et MARCHENT : campagne sur la carte */
        for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
            diplo_set_faustian(s->dp, c, s->ts[c].charge);  /* souillure faustienne → croisades */
        diplo_tick(s->dp, 365.f);
        credit_year_tick(s->econ, s->wl, w);               /* dette : intérêt annuel (creuse le débiteur, crédite le prêteur) */
        diplo_suzerainty_tick(s->dp, w, s->econ, s->wp);   /* suzeraineté + FRONDE : tributs, ligues, défections */
        diplo_war_tick(s->dp, w, s->econ, s->wp, 1.0f);
        missions_tick(s->missions, w, s->econ, s->ts, s->year);  /* missions décennales */
        faction_levers_decay(0.07f);   /* §4 : une stance non entretenue s'efface */
        if (s->ev->ages.last_dawned != s->prev_dawned){          /* §7 : un âge se lève → engagement */
            int age=s->ev->ages.last_dawned;
            if (age>=0) for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
                if (w->country[c].role==POLITY_UNCLAIMED) continue;
                int nr=0; for (int r=0;r<s->econ->n_regions;r++) if (s->econ->region[r].owner==c) nr++;
                if (nr>0) faction_age_engage(w, s->econ, c, age);
            }
            s->prev_dawned = s->ev->ages.last_dawned;
            if (g_gs==GS_PLAYING) audio_alert();    /* §5 preuve de vie : un âge se lève → l'alerte discrète */
        }
    }
    if (++s->day % 365 == 0) s->year++;
}

/* (Ré)initialise la partie VIVANTE : monde déjà généré, on installe TOUS les
 * sous-systèmes, on attache les GROUPES démographiques, on amorce ~3 ans, puis
 * la partie avance par sim_day (plus de snapshot figé). */
/* — Progression de l'amorce, lue par l'écran de chargement (thread principal) pendant
 *   que le worker simule. volatile : écrit par le worker, lu par l'UI — monotone, course
 *   bénigne (seul l'affichage en dépend, pas le moteur). */
#define GEN_BOOT_DAYS (3*365)
static volatile int g_gen_phase = 0;   /* 0 = façonnage du monde · 1 = amorce · 2 = fini */
static volatile int g_gen_day   = 0;   /* jour d'amorce courant (0..GEN_BOOT_DAYS) */
static void sim_rebuild(Sim *s, World *w) {
    if (!s->econ || !s->wp || !s->wl || !s->net || !s->ts || !s->sc
        || !s->ag || !s->ev || !s->drift || !s->labor || !s->rs || !s->host || !s->camp || !s->navy || !s->eg) return;
    econ_init(s->econ, w);
    gen_population(w, s->econ);
    worldgen_seed_peoples(w, s->econ, g_player_race);   /* la race CHOISIE ancre le gradient */
    legitimacy_init(s->wl, w, s->econ);
    prosperity_init(s->wp, w);
    trade_network_build(s->net, w, s->econ);
    statecraft_init(s->sc, w);
    agency_init(s->ag);
    diplo_init(s->dp);
    diplo_seed_rng(s->dp, w->seed);   /* la fronde tire sa graine du MONDE (sinon même séquence à chaque partie) */
    routes_init(s->rn);
    intertrade_reset();   /* embargos décrétés + flux : RAZ par partie */
    intertrade_seed_centres(w, s->econ);   /* P3.20 : les Centres commerciaux (hubs du réseau) */
    intertrade_seed_citystate_arms(w, s->econ);   /* F-arc : chaque cité-état naît armurier (les empires y pompent leurs armes) */
    agency_seed_capital_markets(w, s->econ);   /* DÉPART : chaque empire naît avec un Marché sur sa capitale (carte nue) */
    econ_set_arms_pump(intertrade_market_pull);   /* F-arc : la levée s'arme au marché (propre→Centre cité-état→mondial) */
    /* RAZ PLEINE PLAGE : n_countries grandit par sécession ; à la RÉGÉNÉRATION (touche R)
     * les slots hauts gardaient ai_on/TechState périmés d'un monde précédent (cf. chronicle). */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ s->ai_on[c]=false; tech_state_init(&s->ts[c], false); }
    s->player = 0;
    for (int c=0;c<w->n_countries;c++) if (w->country[c].role==POLITY_PLAYER){ s->player=c; break; }
    /* Chaque voisin (non-vierge, non-joueur) reçoit un acteur IA — sa personnalité
     * sort de sa fiche, ses actions empruntent la MÊME couche d'agency que le joueur. */
    for (int c=0;c<w->n_countries;c++){
        s->ai_on[c] = (c!=s->player && w->country[c].role!=POLITY_UNCLAIMED
                       && w->country[c].capital_prov>=0);
        if (s->ai_on[c]) ai_actor_init(&s->ai[c], w, s->econ, c, w->seed ^ (uint32_t)(c*2654435761u));
    }
    ai_ensure_dominator(s->ai, s->ai_on, w->n_countries);   /* §war : un monde tout en alliances reste atone */
    demography_attach(w, s->econ, s->drift);             /* 1 groupe substrat/région (non-régression) */
    demography_dyn_id_rebase(s->econ);                   /* compteur de drift_id au-dessus de l'existant */
    events_init(s->ev, w, w->seed);
    labor_init(s->labor, w);
    labor_seed_from_world(s->labor, w, s->econ, s->player);
    revolt_init(s->rs);                                  /* les soulèvements incarnés */
    credit_init();                                       /* dette : aucun créancier au départ */
    warhost_init(s->host);                               /* les armées levées par pays */
    campaign_init(s->camp, w, s->econ);                  /* … qui marcheront sur la carte (terrain + RAZ) */
    s->camp_rng = w->seed ^ 0xCA117A11u;                 /* graine de campagne propre à la partie */
    missions_init(s->missions);                          /* missions décennales */
    navy_init(s->navy);                                  /* la flotte : chantiers vides, rades à trouver */
    if (s->eg) endgame_init(s->eg);                     /* capstone §27 : RAZ du cataclysme */
    faction_levers_reset();                              /* §4 : stances de factions à zéro */
    s->prev_dawned=-1;                                   /* §7 : aucun âge encore traité */
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)   /* photo des propriétaires (conquête) */
        s->prev_owner_mo[r]=s->econ->region[r].owner;
    s->day=0; s->year=0;
    for (int t=0; t<GEN_BOOT_DAYS; t++){ sim_day(s, w); g_gen_day=t+1; }   /* amorce : une carte déjà vivante */
    s->ready = true;
}

static int country_for_panel(const World *w, int selected) {
    if (selected>=0 && selected<w->n_provinces) {
        int c = w->province[selected].country;
        if (c>=0 && c<w->n_countries) return c;
    }
    for (int c=0;c<w->n_countries;c++) if (w->country[c].role==POLITY_PLAYER) return c;
    return 0;
}

/* Une lecture « Catégorie NN » : la catégorie en cuivre sourd, puis le NOMBRE de
 * jeu (0-100, sur 100 — le joueur le sait) coloré par le sens ; toute la pastille
 * est survolable (la définition au survol). Une métrique chiffrée se lit au seul
 * nombre : pas de mot redondant derrière. value < 0 ⇒ un MOT seul (signature sans
 * échelle : Assise, Présage). */
static int draw_reading(SDL_Renderer *ren, int x, int y, const char *cat,
                        int value, const char *word, SDL_Color wc, const char *def) {
    draw_text(ren, g_font, x, y, COL_DIM, cat);
    int xx = x + text_w(g_font, cat) + 6;
    char num[16];
    const char *shown = word;
    if (value >= 0) { snprintf(num, sizeof num, "%d", value); shown = num; }
    draw_text(ren, g_font, xx, y, wc, shown);
    int total = (xx - x) + text_w(g_font, shown);
    SDL_Rect z = { x-3, y-2, total+6, 21 };
    zone_add(z, def);
    return x + total + 18;
}

/* Une ressource « Nom stock +flux » (flux rouge si négatif). Survolable. */
static int draw_res(SDL_Renderer *ren, int x, int y, const char *name, long stock, float flow,
                    const char *def){
    char buf[48]; snprintf(buf,sizeof buf, "%s %ld", name, stock);
    draw_text(ren, g_font, x, y, COL_PARCH, buf);
    int w1 = text_w(g_font, buf);
    char fb[24];                                  /* le flux, TOUJOURS en +N/j */
    if (flow>=10.f || flow<=-10.f) snprintf(fb,sizeof fb, " %+d/j", (int)flow);
    else                            snprintf(fb,sizeof fb, " %+.1f/j", flow);
    SDL_Color fc = (flow<0) ? sense_color(0.12f) : sense_color(0.82f);
    draw_text(ren, g_font, x+w1, y, fc, fb);
    int total = w1 + text_w(g_font, fb);
    zone_add((SDL_Rect){x-3,y-2,total+6,19}, def);
    return x + total + 20;
}
/* Séparateur vertical fin entre clusters du bandeau. */
static int topbar_sep(SDL_Renderer *ren, int x, int y){
    fill_rect(ren, x+2, y-1, 1, 16, COL_DIM);
    return x + 9;
}

/* LA TOPBAR (§2) : ressources · métriques 0-100 · temps/âge/vitesse. Deux rangs,
 * cuivre sur bleu nuit. Aucun flottant SCPS — tout par la membrane (mots + 0-100). */
/* Une pastille d'ALERTE : un accent de couleur (nature) + le texte diégétique.
 * ambre = opportunité/mission · rouge = menace · bleu = information. Extensible :
 * les pastilles s'empilent horizontalement. Renvoie le x suivant. */
static int draw_alert(SDL_Renderer *ren, int x, int y, SDL_Color accent,
                      const char *text, const char *hov){
    TTF_Font *fs = g_font_small ? g_font_small : g_font;
    int tw = text_w(fs, text), w = tw + 18;
    fill_round(ren, x, y, w, 18, COL_PANEL2, 5);     /* corps arrondi */
    fill_round(ren, x+2, y+2, 4, 14, accent, 2);     /* la barre d'accent (nature) */
    round_box (ren, x-1, y-1, w+2, 20, accent, 6);   /* bordure colorée, débordante */
    draw_text (ren, fs, x+11, y+1, COL_PARCH, text);
    if (hov) zone_add((SDL_Rect){x,y,w,18}, hov);
    return x + w + 7;
}

static void draw_topbar(SDL_Renderer *ren, int win_w, const Sim *s, const World *w, int cid,
                        GameSpeed sp) {
    CountryReadout r = country_readout(s->wp, s->ts, w, cid);
    r.influence = statecraft_influence(s->sc, cid);
    int bh = 74;
    fill_rect(ren, 0,0, win_w, bh, COL_PANEL);
    fill_rect(ren, 0,0, win_w, 10, COL_PANEL_HI);          /* voile clair en haut (relief) */
    fill_rect(ren, 0,bh,   win_w, 3, COL_COPPER);          /* bordure cuivre ÉPAISSE */
    fill_rect(ren, 0,bh+3, win_w, 5, COL_SHADOW);          /* ombre portée sous le bandeau */

    /* — Rang A : DÉPENSABLE | ACCUMULABLE (clusters séparés) · temps/âge/vitesse (droite).
     *   Le bandeau est un SOMMAIRE : chaque ressource ouvre son système (clic). — */
    int x=12, yA=6, x0;
    /* Dépensable : Or · Nourriture · Matériaux (stock + flux +N/j) — chaque ressource
     * est une PORTE : un clic ouvre le menu de son système. */
    x0=x; x = draw_res(ren,x,yA, "Or",      (long)econ_country_gold(s->econ, cid),      0.f,
                 "Or en caisse, dette comprise (clic → Finances : revenus, commerce, taxation, prêts). Taxes + surplus vendu au marché.");
    topbtn_add((SDL_Rect){x0-3,yA-2,x-x0,19}, SYS_FINANCES);
    x0=x; x = draw_res(ren,x,yA, lres_name(LR_FOOD),      s->labor->stock[LR_FOOD],      (float)s->labor->flow[LR_FOOD],
                 "Vivres (clic → Subsistance & démographie). La famine stoppe la croissance de la population.");
    topbtn_add((SDL_Rect){x0-3,yA-2,x-x0,19}, SYS_SUBSISTANCE);
    x0=x; x = draw_res(ren,x,yA, tr(STR_TOPBAR_MATERIAUX), econ_empire_stock(s->econ, cid, RES_TOOLS), 0.f,
                 "Matériaux (clic → Chaînes de production & stock du marché). Bâtir, coloniser et armer en consomment.");
    topbtn_add((SDL_Rect){x0-3,yA-2,x-x0,19}, SYS_CHAINES);
    x = topbar_sep(ren, x, yA);
    /* Accumulable : Savoir (clic → l'Arbre de tech, qui EXISTE) · Influence. */
    float ppop = ai_country_population(w, s->econ, cid);
    x0=x; x = draw_res(ren,x,yA, "Savoir",    (long)r.m_savoir.value, ai_research_income(&s->ts[cid], ppop),
                 "Savoir — le niveau de lumière du royaume 0-100 (clic → Arbre de tech) ; le flux est la recherche que la population produit par jour.");
    topbtn_add((SDL_Rect){x0-3,yA-2,x-x0,19}, SYS_TECH);
    x0=x; x = draw_res(ren,x,yA, "Influence", (long)statecraft_influence(s->sc,cid), statecraft_influence_flux(s->sc,s->econ,s->wp,cid),
                 "Influence diplomatique (clic → Diplomatie). Prospérité + taille + accords tenus la nourrissent ; elle plafonne les diplomates.");
    topbtn_add((SDL_Rect){x0-3,yA-2,x-x0,19}, SYS_DIPLO);
    /* droite : date · barre 250 ans · âge · VITESSE (Espace = pause, +/-) */
    char date[48]; snprintf(date,sizeof date, "An %d / %d", s->year, GAME_YEARS);
    const char *age = (s->ev->ages.last_dawned>=0) ? age_name((AgeId)s->ev->ages.last_dawned) : "Aube du monde";
    const char *spl = SPEED_LABEL[sp];
    int wdate=text_w(g_font,date), wage=text_w(g_font,age), wspeed=text_w(g_font,spl);
    int bar_w=110, gap=14;
    int speedx = win_w - 12 - wspeed;
    int agex   = speedx - gap - wage;
    int barx   = agex - gap - bar_w;
    int datex  = barx - gap - wdate;
    if (datex < x+10) datex = x+10;
    draw_text(ren, g_font, datex, yA, COL_PARCH, date);
    zone_add((SDL_Rect){datex-3,yA-2,wdate+6,19}, "L'an de la partie (borne de fin : 250). Les âges montent la pression de la fin.");
    fill_rect(ren, barx, yA+4, bar_w, 8, COL_PANEL2);
    int filled = (int)((double)bar_w * (s->year<GAME_YEARS?s->year:GAME_YEARS) / GAME_YEARS);
    fill_rect(ren, barx, yA+4, filled, 8, COL_COPPER);
    draw_text(ren, g_font, agex, yA, sense_color(0.5f), age);
    zone_add((SDL_Rect){agex-3,yA-2,wage+6,19}, "L'âge courant du monde — reconnu quand le monde a atteint son état.");
    draw_text(ren, g_font, speedx, yA, (sp==SPEED_PAUSE)?sense_color(0.5f):COL_COPPER, spl);
    g_speed_zone=(SDL_Rect){speedx-3,yA-2,wspeed+6,19};   /* CLIQUABLE : cran suivant */
    zone_add(g_speed_zone, "Vitesse du temps — CLIC = cran suivant ; Espace = pause/reprise (dernier cran) ; +/− = accélérer/ralentir.");

    /* — Rang B : pays + INDICES 0-100 (colorés par valeur, rouge bas → vert haut).
     *   Pas d'« Assise » (on ne nomme pas le type de légitimité). Savoir/Influence
     *   sont passés en ACCUMULABLE (rang A) ; le présage part en alertes. — */
    int yB=28;
    const char *name = (cid>=0 && cid<w->n_countries) ? w->country[cid].name : "—";
    int xb=12;
    draw_text(ren, g_font, xb, yB, COL_COPPER, name); xb += text_w(g_font,name) + 18;
    xb = draw_reading(ren,xb,yB,"Stabilité", r.m_stabilite.value, label_stab(r.stabilite),   band_good(r.stabilite,5,true),  hover_stab());
    xb = draw_reading(ren,xb,yB,"Légitimité",r.m_legitimite.value,label_legit(r.legitimite), band_good(r.legitimite,5,true), hover_legit());
    xb = draw_reading(ren,xb,yB,"Cohésion",  r.m_cohesion.value,  label_concorde(r.concorde),band_good(r.concorde,4,false),  hover_concorde());
    xb = draw_reading(ren,xb,yB,"Prospérité",r.m_prosperite.value,label_prosp(r.prosperite), band_good(r.prosperite,5,true), hover_prosp());
    /* §27 — ENTROPIE : le destin PARTAGÉ du monde (≠ par-pays). Masqué tant que le monde
     * est STABLE (comme le présage calme) ; il SURGIT à mesure que la Brèche approche.
     * Couleur band_good(…,false) : haut = MAUVAIS. Tout via la membrane (endgame_readout). */
    { EndgameReadout er = endgame_readout(s->wp, s->eg);
      if (er.entropie > ENT_STABLE)
          xb = draw_reading(ren,xb,yB,"Entropie", er.entropie_pct, label_entropie(er.entropie),
                            band_good((int)er.entropie,4,false), hover_entropie()); }
    (void)xb;

    /* — Rang C : les FACTIONS-ÉTHOS par leur SATISFACTION (la signature, §9). Pastille
     *   d'identité + % coloré (vert content → ambre tiède → rouge aliéné). PAS de mot
     *   (ni « mène », ni « Sédition ») : le coup se lit d'une faction ROUGE & PUISSANTE.
     *   Triées par satisfaction décroissante : l'aliéné saute à droite. */
    {
        FactionsReadout fc = faction_readout(w, s->econ, cid);
        TTF_Font *fs = g_font_small ? g_font_small : g_font;
        int yC=52, xc=12;
        static const char *AB[6]   = {"Conquérants","Marchands","Légistes","Gardiens","Transgresseurs","Communs"};
        static const SDL_Color FCOL[6] = {
            {0xc0,0x55,0x4a,0xff}, {0xc9,0xa2,0x4b,0xff}, {0x5f,0x8a,0xb0,0xff},
            {0x7a,0x5c,0x99,0xff}, {0x8a,0x3a,0x6a,0xff}, {0x6f,0x9a,0x5a,0xff},
        };
        draw_text(ren, fs, xc, yC+2, COL_DIM, "Factions · satisfaction"); xc += 150;
        int ord[6]={0,1,2,3,4,5};
        for (int i=0;i<6;i++) for (int j=i+1;j<6;j++)
            if (fc.faction[ord[j]].satisfaction > fc.faction[ord[i]].satisfaction){ int t=ord[i];ord[i]=ord[j];ord[j]=t; }
        static char fhov[6][256];
        for (int k=0;k<6;k++){
            int f=ord[k];
            fill_round(ren, xc, yC+2, 10, 10, FCOL[f], 3);        /* pastille d'identité arrondie */
            round_box (ren, xc, yC+2, 10, 10, COL_EDGE, 3);
            draw_text(ren, fs, xc+13, yC, COL_DIM, AB[f]);
            int aw = text_w(fs, AB[f]);
            char pz[8]; snprintf(pz,sizeof pz, "%d%%", fc.faction[f].satisfaction);
            draw_text(ren, fs, xc+13+aw+4, yC, sense_color(fc.faction[f].satisfaction/100.f), pz);
            int total = 13+aw+4+text_w(fs,pz);
            bool coup = (!fc.faction[f].aligned && fc.faction[f].part >= 25);
            /* P6.32 — QUI ils sont (l'éthos) + ce que MESURE le % (adhésion · poids). */
            char sz[8], pp[8]; snprintf(sz,sizeof sz,"%d",fc.faction[f].satisfaction);
            snprintf(pp,sizeof pp,"%d",fc.faction[f].part);
            char body[200]; tr_fmt(body,sizeof body, STR_FACTION_HOV_FMT,
                                   fc.faction[f].name, tr_band(STR_FACTION_ETHOS_0,f,6), sz, pp);
            snprintf(fhov[k],sizeof fhov[k], "%s%s", body, coup?tr(STR_FACTION_HOV_COUP):"");
            zone_add((SDL_Rect){xc-2,yC-2, total+6, 20}, fhov[k]);
            xc += total + 14;
        }
    }

    /* — La RANGÉE D'ALERTES : tout ce qui réclame une décision, en pastilles
     *   diégétiques empilables, accent par nature (ambre opportunité · rouge menace ·
     *   bleu information). Scalable : elles s'ajoutent à mesure qu'elles surgissent. — */
    {
        SDL_Color AMBER = sense_color(0.62f), RED = sense_color(0.10f), BLUE = (SDL_Color){0x5f,0x8a,0xb0,0xff};
        int ay=bh+4, ax=8, used=0;
        fill_rect(ren, 0, bh+2, win_w, 24, COL_PANEL2);
        /* 1. mission décennale (opportunité) */
        const Mission *mis = mission_of(s->missions, cid);
        if (mis && !mis->done){
            char mz[120]; snprintf(mz,sizeof mz, "Mission · %s", mis->text);
            ax = draw_alert(ren, ax, ay, AMBER, mz,
                            "Mission décennale : un but tiré de l'état du pays ; l'accomplir verse or + matières."); used++;
        }
        /* 2. faction aliénée & puissante (le coup qui couve — menace) */
        {
            FactionsReadout fc = faction_readout(w, s->econ, cid);
            for (int f=0; f<6; f++)
                if (!fc.faction[f].aligned && fc.faction[f].part >= 25){
                    char cz[120]; snprintf(cz,sizeof cz, "Cour · les %ss s'aliènent", fc.faction[f].name);
                    static char chov[140]; snprintf(chov,sizeof chov,
                        "Une faction PUISSANTE (%d%% du pouvoir) et ALIÉNÉE (satisfaction %d%%) : le coup couve.",
                        fc.faction[f].part, fc.faction[f].satisfaction);
                    ax = draw_alert(ren, ax, ay, RED, cz, chov); used++;
                    break;   /* une seule pastille de cour suffit */
                }
        }
        /* 3. l'augure (menace lue de l'état : sécession, révolte, coercition fragile) */
        if (r.augure) ax = draw_alert(ren, ax, ay, RED, r.augure,
                          "Un péril lu de l'état du royaume : sécession qui gronde, révolte, ou poigne qui s'effrite."), used++;
        /* 4. présage (information) */
        if (r.presage != PG_CALME)
            ax = draw_alert(ren, ax, ay, BLUE, label_presage(r.presage), hover_presage()), used++;
        if (!used) draw_text(ren, g_font_small?g_font_small:g_font, 10, bh+5, COL_DIM, "Rien ne presse.");
    }
}

static void ui_section(SDL_Renderer *ren, int x, int *y, const char *title){
    *y += 9;
    draw_text(ren, g_font, x, *y, COL_COPPER, title);
    *y += 21;
}
static void ui_row(SDL_Renderer *ren, int x, int *y, int pw, const char *cat,
                   const char *word, SDL_Color wc, const char *def){
    draw_text(ren, g_font, x,     *y, COL_DIM,  cat);
    draw_text(ren, g_font, x+104, *y, wc,       word);
    zone_add((SDL_Rect){x-2, *y-2, pw, 19}, def);
    *y += 20;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SIDEBAR GAUCHE (§sidebar) — l'interface d'EMPIRE + la couche de DÉCISION.
 * Rail fixe (icônes-onglets) + tiroir glissant. Deux lois : (1) ne lit QUE la
 * membrane (bandes + nombres de jeu) ; (2) toute décision passe par les
 * ACTIONNEURS (agency / intertrade / warhost / campaign) — les MÊMES leviers
 * que l'IA, en JOURS, jamais une écriture directe ni un effet instantané.
 * ═══════════════════════════════════════════════════════════════════════════ */
#define SB_RAIL_W   46
#define SB_DRAWER_W 380
typedef enum { SBT_ECO=0, SBT_DEMO, SBT_STOCKS, SBT_MARCHE, SBT_ARMEE, SBT_FILTRES, SBT_DIPLO, SBT_ETAT, SBT_COUNT } SbTab;
typedef struct {
    int     tab;                  /* -1 = replié */
    int     eco_sub;              /* 0 Commerce · 1 Marché · 2 Import/Export */
    int     marche_sub;           /* #5 : 0 Stock local · 1 Marché régional · 2 Marché global */
    int     scroll[SBT_COUNT];
    float   anim;                 /* 0..1 — glissement du tiroir (~150 ms) */
    int     reloc_src;            /* étape 1 de la relocalisation (-1 = pas choisie) */
    bool    show_currents;        /* filtre COURANTS : le champ marin (physique → continu légitime) */
    MapLens lens;                 /* lentille readout (LENS_NONE = vues classiques) */
    bool    purge_arm;            /* purge : 1er clic ARME, 2e confirme (acte irréversible) */
} Sidebar;
static Sidebar g_sb = { -1, 1, 1, {0,0,0,0,0,0,0,0}, 0.f, -1, false, LENS_NONE, false };

/* cibles cliquables du tiroir (reconstruites chaque frame, comme zone_add) */
enum { SBH_TAB=1, SBH_ECOSUB, SBH_EMBARGO, SBH_RELOC_SRC, SBH_RELOC_DST, SBH_RELOC_CLR,
       SBH_EXPLOIT, SBH_LEVY, SBH_POSTURE, SBH_CHIP_MODE, SBH_CHIP_LENS, SBH_REFILL,
       SBH_MARCH, SBH_MUSTER, SBH_CANCEL, SBH_DEMOB /* P4.24 : démobiliser */,
       SBH_CHIP_CUR,
       SBH_NAVY_BUILD /* a=HullType */, SBH_SAIL /* a=région-cible */, SBH_NAVY_CONV /* a=1 vers pirate */,
       SBH_LEV_REPRESS, SBH_LEV_ASSIM, SBH_LEV_PURGE, SBH_LEV_EMBARGO,
       SBH_LEV_CONTRAT /* a=SuzContrat, b=pays cible */,
       SBH_DECLARE /* a=pays cible (déclarer la guerre, CB inhérent) */,
       SBH_PEACE   /* a=pays cible (négocier la paix : arbitrage IA exposé) */,
       SBH_COUNCIL /* Q1 : a=siège(0..2), b=slot candidat à NOMMER (≥0) ou -1 = RENVOYER */,
       SBH_MARCHE_SUB /* #5 : a=sous-onglet marché (0 local · 1 régio · 2 global) */,
       SBH_BUY  /* #5 : a=bien, b=tier (0 régio · 1 global) — pompe au marché */,
       SBH_SELL /* #5 : a=bien, b=tier — vend au marché */,
       SBH_PACT /* M3 : a=pays partenaire, b=1 signer / 0 rompre un pacte commercial */,
      SBH_PROD_CAP /* limiteur : a=good ; clic gauche/molette+ → +step, clic droit/molette− → −step (Maj = ×10) */ };
typedef struct { SDL_Rect r; int kind, a, b; } SbHit;
static SbHit g_sbhits[256]; static int g_nsbhits;
static void sbhit_reset(void){ g_nsbhits=0; }
static void sbhit_add(SDL_Rect r, int kind, int a, int b){
    if (g_nsbhits<256){ g_sbhits[g_nsbhits].r=r; g_sbhits[g_nsbhits].kind=kind;
                        g_sbhits[g_nsbhits].a=a; g_sbhits[g_nsbhits].b=b; g_nsbhits++; }
}

/* Cache d'agrégats PAYS — recalculé au PAS DE JOUR (jamais par frame, §8). */
typedef struct {
    int   day;                    /* jour du dernier calcul (-1 = jamais) */
    float dem[RES_COUNT], sup[RES_COUNT], stk[RES_COUNT], prix[RES_COUNT];
    float prix_prev[RES_COUNT];   /* échantillon précédent (tendance ▲▼) */
    float cls_pop[CLASS_COUNT], cls_sat[CLASS_COUNT];
    long  pop_total; int n_reg;
} SbCache;
static SbCache g_sbc = { -1, {0},{0},{0},{0},{0},{0},{0}, 0, 0 };
static void sb_cache_refresh(const Sim *s, int day){
    if (g_sbc.day==day) return;
    memcpy(g_sbc.prix_prev, g_sbc.prix, sizeof g_sbc.prix);
    memset(g_sbc.dem,0,sizeof g_sbc.dem); memset(g_sbc.sup,0,sizeof g_sbc.sup);
    memset(g_sbc.stk,0,sizeof g_sbc.stk); memset(g_sbc.prix,0,sizeof g_sbc.prix);
    memset(g_sbc.cls_pop,0,sizeof g_sbc.cls_pop); memset(g_sbc.cls_sat,0,sizeof g_sbc.cls_sat);
    g_sbc.pop_total=0; g_sbc.n_reg=0;
    int me=s->player;
    for (int r=0;r<s->econ->n_regions;r++){
        const RegionEconomy *re=&s->econ->region[r];
        if (re->owner!=me || !re->colonized) continue;
        g_sbc.n_reg++;
        for (int g=1;g<RES_COUNT;g++){
            g_sbc.dem[g]+=re->demand[g]; g_sbc.sup[g]+=re->supply[g];
            g_sbc.stk[g]+=re->stock[g];  g_sbc.prix[g]+=re->price[g];
        }
        for (int c=0;c<CLASS_COUNT;c++){
            g_sbc.cls_pop[c]+=re->strata[c].pop;
            g_sbc.cls_sat[c]+=re->strata[c].satisfaction*re->strata[c].pop;
        }
    }
    for (int g=1;g<RES_COUNT;g++) if (g_sbc.n_reg>0) g_sbc.prix[g]/=(float)g_sbc.n_reg;
    for (int c=0;c<CLASS_COUNT;c++){
        g_sbc.pop_total+=(long)g_sbc.cls_pop[c];
        g_sbc.cls_sat[c]=(g_sbc.cls_pop[c]>0.f)?g_sbc.cls_sat[c]/g_sbc.cls_pop[c]:0.f;
    }
    if (g_sbc.day<0) memcpy(g_sbc.prix_prev,g_sbc.prix,sizeof g_sbc.prix);
    g_sbc.day=day;
}

/* petits pinceaux */
static SDL_Color sb_marche_col(BandMarche m){
    static const SDL_Color C[5]={ {0x60,0x60,0x60,0xff},{0xd0,0x50,0x40,0xff},{0xc8,0x90,0x40,0xff},
                                  {0x7a,0xa8,0x78,0xff},{0x8a,0x9a,0xb8,0xff} };
    return C[(m>=0&&m<=MARCHE_ENGORGE)?(int)m:0];
}
static int sb_chip(SDL_Renderer *ren, int x, int y, const char *txt, bool actif,
                   int kind, int a, int b, const char *hov){
    int w=text_w(g_font_small,txt)+14;
    fill_rect(ren,x,y,w,17, actif?(SDL_Color){0x3a,0x2c,0x1a,0xff}:(SDL_Color){0x16,0x1e,0x2c,0xff});
    draw_box (ren,x,y,w,17, actif?COL_COPPER:COL_PANEL2);
    draw_text(ren,g_font_small,x+7,y+2, actif?COL_COPPER:COL_DIM, txt);
    sbhit_add((SDL_Rect){x,y,w,17}, kind, a, b);
    if (hov) zone_add((SDL_Rect){x,y,w,17}, hov);
    return x+w+5;
}
/* chip VERROUILLÉ : grisé, AUCUN hit (non cliquable) ; le hover dit POURQUOI. */
static int sb_chip_locked(SDL_Renderer *ren, int x, int y, const char *txt, const char *why){
    int w=text_w(g_font_small,txt)+14;
    fill_rect(ren,x,y,w,17,(SDL_Color){0x10,0x12,0x16,0xff});
    draw_box (ren,x,y,w,17,(SDL_Color){0x33,0x30,0x2a,0xff});
    draw_text(ren,g_font_small,x+7,y+2,(SDL_Color){0x55,0x52,0x4a,0xff}, txt);
    if (why) zone_add((SDL_Rect){x,y,w,17}, why);
    return x+w+5;
}
static const char *sb_country_name(const World *w, int cid){
    return (cid>=0 && cid<w->n_countries) ? w->country[cid].name : "—";
}
/* région-capitale du joueur (point de ralliement par défaut) */
static int sb_capital_region(const Sim *s, const World *w){
    int cp=(s->player>=0&&s->player<w->n_countries)?w->country[s->player].capital_prov:-1;
    return (cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
}

/* P3.20 — NOM d'un Centre commercial : on RÉCUPÈRE le pattern de nommage existant
 * (place_make_name, le syllabaire par RACE de scps_world) flavoré par la culture
 * DOMINANTE de la région ; déterministe par région, décorrélé des noms d'empire. */
static void region_make_name(char *out, int n, const WorldEconomy *e, int region){
    SpeciesArchetype race = (e && region>=0 && region<e->n_regions)
                          ? e->region[region].culture.race : RACE_HUMAIN;
    place_make_name(out, n, race, (uint32_t)region ^ 0x5EEDu);
}

/* ── panneau ÉCONOMIE : Commerce · Marché · Import/Export ─────────────────── */
static void sb_panel_eco(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, const World *world){
    static char buf[64][120]; int nb=0;
    int me=s->player;
    int cx=x+10;
    cx=sb_chip(ren,cx,y,"Commerce",   g_sb.eco_sub==0, SBH_ECOSUB,0,0,"Les routes marchandes vivantes — et l'EMBARGO, qui se décrète ici.");
    cx=sb_chip(ren,cx,y,"Marché",     g_sb.eco_sub==1, SBH_ECOSUB,1,0,"La table des biens : prix en or, tendance, état du marché en mots.");
    cx=sb_chip(ren,cx,y,"Imp/Exp", g_sb.eco_sub==2, SBH_ECOSUB,2,0,"Ce que le pays importe et exporte (volumes, partenaires, or encaissé).");
    (void)sb_chip(ren,cx,y,"Réseau", g_sb.eco_sub==3, SBH_ECOSUB,3,0,"Tous les Centres commerciaux du monde, nommés — le tenir = commercer (l'utilité des cités-états).");
    y+=24;
    if (g_sb.eco_sub==0){
        int routes=intertrade_active_routes(s->econ, s->rn, s->dp, me);
        float gold=intertrade_export_gold(me);
        snprintf(buf[nb],120,"%d route(s) vivante(s) · export %.0f or/an", routes, gold);
        draw_text(ren,g_font_small,x+10,y,COL_PARCH,buf[nb]); nb++; y+=20;
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"partenaires :"); y+=16;
        int shown=0;
        for (int c=0;c<world->n_countries && shown<10 && y<h-22;c++){
            if (c==me) continue;
            float v=intertrade_pair_value(me,c);
            bool emb=intertrade_embargoed(me,c);
            bool war=diplo_status(s->dp,me,c)==DIPLO_WAR;
            if (v<=0.5f && !emb && !war) continue;
            const char *etat = war?"GUERRE":emb?"EMBARGO":(v>200.f)?"florissant":"modeste";
            snprintf(buf[nb],120,"%-18.18s %7.0f or/an  %s", sb_country_name(world,c), v, etat);
            SDL_Color col= war?(SDL_Color){0xd0,0x50,0x40,0xff}: emb?(SDL_Color){0xc8,0x90,0x40,0xff}:COL_PARCH;
            draw_text(ren,g_font_small,x+12,y,col,buf[nb]);
            sbhit_add((SDL_Rect){x+8,y-2,w-16,16}, SBH_EMBARGO, c, emb?0:1);
            static char hv[64][140]; snprintf(hv[shown],140, emb?
                "Lever l'embargo contre %s — le négoce (~%.0f or/an) pourra reprendre.":
                "Décréter l'EMBARGO contre %s — coupe ~%.0f or/an de négoce (le coût, avant de signer).",
                sb_country_name(world,c), v);
            zone_add((SDL_Rect){x+8,y-2,w-16,16}, hv[shown]);
            nb++; shown++; y+=16;
        }
    } else if (g_sb.eco_sub==1){
        /* P3.21 — la COULEUR porte l'état ; le mot va au survol. P3.20 — la colonne
         * « échange » : net du RÉSEAU vs notre marché de référence (↓ on importe /
         * achète · ↑ on exporte / vend). */
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"bien          prix(or) tend.  échange"); y+=16;
        int idx[RES_COUNT], n=0;
        for (int g=1;g<RES_COUNT;g++) if (g_sbc.dem[g]>0.05f||g_sbc.sup[g]>0.05f) idx[n++]=g;
        for (int i=1;i<n;i++){ int k=idx[i],j=i;        /* tri : tension d'abord */
            while(j>0 && (int)band_marche(g_sbc.dem[idx[j-1]],g_sbc.sup[idx[j-1]]+g_sbc.stk[idx[j-1]])
                       > (int)band_marche(g_sbc.dem[k],g_sbc.sup[k]+g_sbc.stk[k])){ idx[j]=idx[j-1]; j--; }
            idx[j]=k; }
        static char mhov[RES_COUNT][120];
        int off=g_sb.scroll[SBT_ECO]; if(off>n-1)off=n>0?n-1:0; if(off<0)off=0;
        for (int i=off;i<n && y<h-20;i++){
            int g=idx[i];
            BandMarche m=band_marche(g_sbc.dem[g], g_sbc.sup[g]+g_sbc.stk[g]);
            float d=g_sbc.prix[g]-g_sbc.prix_prev[g];
            const char *tend=(d>0.02f)?"\xe2\x96\xb2":(d<-0.02f)?"\xe2\x96\xbc":"\xc2\xb7";
            float net=intertrade_import_vol(me,g)-intertrade_export_vol(me,g);  /* +import / -export */
            char ie[16];
            if (net> 0.5f) snprintf(ie,sizeof ie,"\xe2\x86\x93%.0f", net);       /* ↓ on importe */
            else if (net<-0.5f) snprintf(ie,sizeof ie,"\xe2\x86\x91%.0f", -net); /* ↑ on exporte */
            else snprintf(ie,sizeof ie,"\xc2\xb7");
            snprintf(buf[nb],120,"%-12.12s %7.2f  %s  %s",
                     resource_name((Resource)g), g_sbc.prix[g], tend, ie);
            draw_text(ren,g_font_small,x+12,y,sb_marche_col(m),buf[nb]); nb++;
            snprintf(mhov[g],sizeof mhov[g],"%s — marché %s · réseau : %s",
                     resource_name((Resource)g), label_marche(m),
                     net>0.5f?"on IMPORTE (on achète)":net<-0.5f?"on EXPORTE (on vend)":"équilibré");
            zone_add((SDL_Rect){x+8,y-1,w-16,15}, mhov[g]);
            y+=15;
        }
    } else if (g_sb.eco_sub==2){
        /* P3.20 — accès au RÉSEAU : tient-on un Centre commercial ? (sinon, coupé) */
        bool net = intertrade_country_has_centre(s->econ, me);
        draw_text(ren,g_font_small,x+10,y, net?(SDL_Color){0x8c,0xd0,0x9c,0xff}:sense_color(0.12f),
                  net?tr(STR_CENTRE_RESEAU_OUVERT):tr(STR_CENTRE_RESEAU_FERME)); y+=16;
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"bien            import (de)        export (vers)"); y+=16;
        int shown=0, off=g_sb.scroll[SBT_ECO];
        for (int g=1, seen=0; g<RES_COUNT && y<h-20; g++){
            float iv=intertrade_import_vol(me,g), ev=intertrade_export_vol(me,g);
            if (iv<0.05f && ev<0.05f) continue;
            if (seen++ < off) continue;
            int from=intertrade_import_from(me,g), to=intertrade_export_to(me,g);
            snprintf(buf[nb],120,"%-12.12s %5.0f %-10.10s   %5.0f %-10.10s",
                     resource_name((Resource)g),
                     iv, iv>0.05f?sb_country_name(world,from):"—",
                     ev, ev>0.05f?sb_country_name(world,to):"—");
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++; y+=15; shown++;
        }
    } else {
        /* P3.20 — TOUS les Centres commerciaux, nommés par lieu : on voit le RÉSEAU
         * entier et QUI tient chaque hub (une petite cité-état qui en tient un est
         * un partenaire précieux : l'utilité des cités-états). */
        bool net=intertrade_country_has_centre(s->econ, me);
        draw_text(ren,g_font_small,x+10,y, net?(SDL_Color){0x8c,0xd0,0x9c,0xff}:sense_color(0.12f),
                  net?tr(STR_CENTRE_RESEAU_OUVERT):tr(STR_CENTRE_RESEAU_FERME)); y+=18;
        static char chov[SCPS_MAX_REG][112]; int seen=0, off=g_sb.scroll[SBT_ECO], shown=0;
        for (int r=0;r<s->econ->n_regions && y<h-18 && nb<64;r++){
            if (!intertrade_has_centre(r)) continue;
            if (seen++ < off) continue;
            int o=s->econ->region[r].owner;
            char nm[24]; region_make_name(nm,sizeof nm,s->econ,r);
            const char *own=(o>=0&&o<world->n_countries)?world->country[o].name:"libre";
            bool mine=(o==me), war=(o>=0&&o!=me)&&diplo_status(s->dp,me,o)==DIPLO_WAR;
            snprintf(buf[nb],120,"Marché de %-10.10s  %-13.13s", nm, own);
            SDL_Color col = mine?COL_COPPER : war?(SDL_Color){0xd0,0x50,0x40,0xff}:COL_PARCH;
            draw_text(ren,g_font_small,x+12,y,col,buf[nb]); nb++;
            snprintf(chov[r],sizeof chov[r],"Centre commercial tenu par %s%s — un hub du réseau.",
                     own, mine?" (vous)":war?" — EN GUERRE":"");
            zone_add((SDL_Rect){x+8,y-1,w-16,15}, chov[r]);
            y+=15; shown++;
        }
        if (shown==0) draw_text(ren,g_font_small,x+12,y,COL_DIM,"(aucun Centre commercial connu)");
    }
}

/* ── panneau MARCHÉ (#5) : Stock local · Marché régional · Marché global ──────
 * La chaîne joueur → cité-état → mondial, RENDUE. Trois étages : ce qu'on DÉTIENT,
 * le marché de la cité-état la plus proche (régional), le réseau mondial (grisé sans
 * Centre direct). Sur les deux marchés : ACHAT (▲) / VENTE (▼) en direct — par
 * paliers de 10, Maj = 100 — via l'actionneur intertrade_market_buy/sell (jamais une
 * écriture directe). L'ancre des transactions = la région-CAPITALE du joueur. */
static void sb_panel_marche(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, const World *world){
    static char buf[64][120]; int nb=0;
    int me=s->player;
    int cap=sb_capital_region(s, world);
    bool hasglobal=intertrade_has_global_access(me);   /* M3 : Centre propre OU pacte commercial */
    int hub=(cap>=0)?intertrade_region_hub(cap):-1;
    /* les trois sous-onglets (le global GRISÉ si pas d'accès DIRECT) */
    int cx=x+10;
    cx=sb_chip(ren,cx,y,"Stock local", g_sb.marche_sub==0, SBH_MARCHE_SUB,0,0,
               "Ce que VOTRE pays détient (stock cumulé).");
    cx=sb_chip(ren,cx,y,"Régional",    g_sb.marche_sub==1, SBH_MARCHE_SUB,1,0,
               "Le marché de la cité-état la plus proche — on y achète à la marge de proximité.");
    if (hasglobal)
        (void)sb_chip(ren,cx,y,"Global", g_sb.marche_sub==2, SBH_MARCHE_SUB,2,0,
                      "Le marché MONDIAL (réseau des Centres) — la double taxe.");
    else { (void)sb_chip_locked(ren,cx,y,"Global",
               "Sans Centre commercial, pas d'accès DIRECT au marché mondial — il en faut un (conquis ou bâti).");
           if (g_sb.marche_sub==2) g_sb.marche_sub=1; }
    y+=24;
    int sub=g_sb.marche_sub;
    if (cap<0){ draw_text(ren,g_font_small,x+10,y,COL_DIM,"(pas de capitale — aucun marché)"); return; }
    if (sub>=1 && hub<0){ draw_text(ren,g_font_small,x+10,y,COL_DIM,
            "(aucun marché de cité-état atteignable — enclavé)"); return; }
    const RegionEconomy *cre=&s->econ->region[cap];
    float marge=cre->import_margin; if (marge<1.f) marge=1.f;
    if (sub==0){
        /* ce que le pays DÉTIENT (agrégat) — informatif, sans bouton */
        snprintf(buf[nb],120,"stock du pays · %d région(s)", g_sbc.n_reg);
        draw_text(ren,g_font_small,x+10,y,COL_COPPER,buf[nb]); nb++; y+=18;
        draw_text(ren,g_font_small,x+10,y,COL_DIM,tr(STR_MARCHE_HDR_LOCAL)); y+=16;
        for (int g=1; g<RES_COUNT && y<h-16; g++){
            if (g_sbc.stk[g]<0.5f) continue;
            snprintf(buf[nb],120,"%-12.12s %9.0f   %7.2f", resource_name((Resource)g),
                     g_sbc.stk[g], g_sbc.prix[g]);
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++; y+=15;
        }
        return;
    }
    /* marché RÉGIONAL (tier 0) ou MONDIAL (tier 1) — prix vif + achat/vente */
    int tier=(sub==2)?1:0;
    float pxmult=(sub==2)?marge*2.f:marge;   /* mondial = double taxe */
    snprintf(buf[nb],120, sub==2 ? "marché mondial · marge ×%.2f (double taxe)"
                                 : "via cité-état · marge ×%.2f (distance incluse)", pxmult);
    draw_text(ren,g_font_small,x+10,y,COL_COPPER,buf[nb]); nb++; y+=16;
    draw_text(ren,g_font_small,x+10,y,COL_DIM,"\xe2\x96\xb2 achat  \xe2\x96\xbc vente   ( Maj = \xc3\x97" "100 )"); y+=15;
    draw_text(ren,g_font_small,x+10,y,COL_DIM,tr(STR_MARCHE_HDR_MARCHE)); y+=15;
    int off=g_sb.scroll[SBT_MARCHE]; if(off<0)off=0;
    int seen=0;
    for (int g=1; g<RES_COUNT && y<h-16; g++){
        float avail=(sub==2)?intertrade_global_stock(s->econ,g):s->econ->region[hub].stock[g];
        if (avail<0.5f) continue;                       /* on ne liste que le DISPONIBLE */
        if (seen++ < off) continue;
        float price=cre->price[g]; if (price<0.2f) price=0.2f;
        float unit=price*pxmult;
        SDL_Color col=(s->econ->region[cap].treasury>=unit)?COL_PARCH:(SDL_Color){0x9a,0x6a,0x55,0xff};
        snprintf(buf[nb],120,"%-11.11s %6.2f %7.0f", resource_name((Resource)g), unit, avail);
        draw_text(ren,g_font_small,x+12,y,col,buf[nb]); nb++;
        /* boutons ACHAT (▲) / VENTE (▼) — actionneurs, hit par bien+tier */
        int bx=x+w-58;
        fill_rect(ren,bx,y-1,22,15,(SDL_Color){0x1c,0x2a,0x1c,0xff}); draw_box(ren,bx,y-1,22,15,(SDL_Color){0x4a,0x7a,0x4a,0xff});
        draw_text(ren,g_font_small,bx+6,y,(SDL_Color){0x9a,0xd0,0x9a,0xff},"\xe2\x96\xb2");
        sbhit_add((SDL_Rect){bx,y-1,22,15}, SBH_BUY, g, tier);
        zone_add((SDL_Rect){bx,y-1,22,15},tr(STR_MARCHE_BUY_HOV));
        int sx=x+w-32;
        fill_rect(ren,sx,y-1,22,15,(SDL_Color){0x2a,0x1c,0x1c,0xff}); draw_box(ren,sx,y-1,22,15,(SDL_Color){0x7a,0x4a,0x4a,0xff});
        draw_text(ren,g_font_small,sx+6,y,(SDL_Color){0xd0,0x9a,0x9a,0xff},"\xe2\x96\xbc");
        sbhit_add((SDL_Rect){sx,y-1,22,15}, SBH_SELL, g, tier);
        zone_add((SDL_Rect){sx,y-1,22,15},tr(STR_MARCHE_SELL_HOV));
        (void)nb; y+=16;
    }
    if (seen==0) draw_text(ren,g_font_small,x+12,y,COL_DIM,"(marché vide — rien à acheter ici)");
}

/* ── panneau DÉMOGRAPHIE : classes, peuples, croissance + RELOCALISATION ───── */
static void sb_panel_demo(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, const World *world){
    static char buf[48][130]; int nb=0;
    (void)world;
    static const char *CN[CLASS_COUNT]={"Journaliers","Bourgeois","Nobles"};
    snprintf(buf[nb],130,"population : %ldk  ·  %d région(s)", g_sbc.pop_total/1000, g_sbc.n_reg);
    draw_text(ren,g_font,x+10,y,COL_PARCH,buf[nb]); nb++; y+=22;
    for (int c=0;c<CLASS_COUNT;c++){
        float part=(g_sbc.pop_total>0)?100.f*g_sbc.cls_pop[c]/(float)g_sbc.pop_total:0.f;
        snprintf(buf[nb],130,"%-12s %6.0f  (%2.0f%%)   satisfaction %2.0f%%",
                 CN[c], g_sbc.cls_pop[c], part, 100.f*g_sbc.cls_sat[c]);
        draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++; y+=15;
    }
    y+=8;
    draw_text(ren,g_font_small,x+10,y,COL_COPPER,"Relocalisation"); y+=16;
    /* file d'ordres en cours (annulables) */
    for (int i=0;i<s->ag->n && y<h-120;i++){
        const BuildOrder *o=&s->ag->order[i];
        if (!o->active || o->kind!=AGY_RELOCATE) continue;
        if (o->region<0||o->region>=s->econ->n_regions) continue;
        if (s->econ->region[o->region].owner!=s->player) continue;
        snprintf(buf[nb],130,"convoi %d→%d · %d j restants   [annuler]",
                 o->region, o->param, o->days_total-o->days_done);
        draw_text(ren,g_font_small,x+12,y,COL_DIM,buf[nb]); nb++;
        sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_CANCEL, i, 0);
        zone_add((SDL_Rect){x+8,y-2,w-16,15},"Révoquer ce convoi (rien n'est appliqué tant qu'il n'est pas arrivé).");
        y+=15;
    }
    if (g_sb.reloc_src<0){
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"source :"); y+=15;
        int picked[6], np=0;
        for (int k=0;k<6;k++){ int best=-1; float bp=0.f;
            for (int r=0;r<s->econ->n_regions;r++){
                const RegionEconomy *re=&s->econ->region[r];
                if (re->owner!=s->player||!re->colonized) continue;
                bool used=false; for(int q=0;q<np;q++) if(picked[q]==r) used=true;
                if (used) continue;
                float lp=re->strata[CLASS_LABORER].pop;
                if (lp>bp){ bp=lp; best=r; }
            }
            if (best<0) break;
            picked[np++]=best;
        }
        for (int q=0;q<np && y<h-20;q++){
            int r=picked[q];
            snprintf(buf[nb],130,"région %-3d  ·  %5.0f journaliers", r, s->econ->region[r].strata[CLASS_LABORER].pop);
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
            sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_RELOC_SRC, r, 0);
            zone_add((SDL_Rect){x+8,y-2,w-16,15},"Prendre les familles ICI — la coercition montera à la source (le coût du déplacement forcé).");
            y+=15;
        }
    } else {
        snprintf(buf[nb],130,"source : région %d   [changer]", g_sb.reloc_src);
        draw_text(ren,g_font_small,x+10,y,COL_COPPER,buf[nb]); nb++;
        sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_RELOC_CLR, 0,0); y+=16;
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"cible :"); y+=15;
        int shown=0;
        for (int r=0;r<s->econ->n_regions && shown<6 && y<h-20;r++){
            const RegionEconomy *re=&s->econ->region[r];
            if (re->owner!=s->player||!re->colonized||r==g_sb.reloc_src) continue;
            if (re->habitability<0.20f) continue;
            if (re->strata[CLASS_LABORER].pop > 600.f) continue;     /* déjà peuplée */
            int gbest=-1; float sbest=0.f;
            for (int g=1;g<RES_COUNT;g++){
                if (re->raw_cap[g]<=0.f) continue;
                float shortfall=g_sbc.dem[g]-(g_sbc.sup[g]+g_sbc.stk[g]);
                if (shortfall>sbest){ sbest=shortfall; gbest=g; }
            }
            if (gbest<0) continue;
            snprintf(buf[nb],130,"région %-3d  ·  %-12.12s  ·  %4.0f bras",
                     r, resource_name((Resource)gbest), re->strata[CLASS_LABORER].pop);
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
            sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_RELOC_DST, r, 0);
            static char hv2[8][150];
            snprintf(hv2[shown],150,"ORDONNER le convoi : ~%d familles, 90 jours de route — la coercition montera dans la région %d.",
                     AGY_RELOC_POP, g_sb.reloc_src);
            zone_add((SDL_Rect){x+8,y-2,w-16,15},hv2[shown]);
            y+=15; shown++;
        }
    }
}

/* ── panneau STOCKS — sobre : l'état par la COULEUR et l'ORDRE, jamais par les mots.
 * Un bien mort n'existe pas à l'écran. L'exploitation est un chip de LIGNE (un ordre
 * disponible = un état), pas une suggestion. ───────────────────────────────── */
static bool sb_good_alive(int g){
    return g_sbc.stk[g]>0.5f || g_sbc.dem[g]>0.05f || g_sbc.sup[g]>0.05f;
}
static void sb_panel_stocks(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, const World *world){
    static char buf[56][130]; int nb=0;
    static char capl[56][16]; int nbc=0;  /* libellés limiteur de production */
    (void)world;
    static const Resource STRAT[3]={RES_SALTPETER,RES_CELESTIAL_IRON,RES_ARCANE_CRYSTAL};
    draw_text(ren,g_font_small,x+10,y,COL_DIM,"bien            stock   net/j   couv."); y+=16;
    int idx[RES_COUNT], n=0;
    for (int k=0;k<3;k++) if (sb_good_alive((int)STRAT[k])) idx[n++]=(int)STRAT[k];   /* ★ vivants seulement */
    int base=n;
    for (int g=1;g<RES_COUNT;g++){
        bool strat=false; for(int k2=0;k2<3;k2++) if((int)STRAT[k2]==g) strat=true;
        if (strat || !sb_good_alive(g)) continue;
        idx[n++]=g;
    }
    for (int i=base+1;i<n;i++){ int k=idx[i],j=i;                  /* tensions en tête */
        while(j>base && (int)band_marche(g_sbc.dem[idx[j-1]],g_sbc.sup[idx[j-1]]+g_sbc.stk[idx[j-1]])
                      > (int)band_marche(g_sbc.dem[k],g_sbc.sup[k]+g_sbc.stk[k])){ idx[j]=idx[j-1]; j--; }
        idx[j]=k; }
    int off=g_sb.scroll[SBT_STOCKS]; if(off<0)off=0; if(off>n-1&&n>0)off=n-1;
    int chips=0;
    for (int i=off;i<n && y<h-18;i++){
        int g=idx[i];
        BandMarche m=band_marche(g_sbc.dem[g], g_sbc.sup[g]+g_sbc.stk[g]);
        float net=(g_sbc.sup[g]-g_sbc.dem[g])/30.f;                /* le tick éco est mensuel */
        if (net>-0.05f && net<0.05f) net=0.f;                       /* fini les −0.0 */
        char couv[12]="";
        if (net<-0.05f){
            float dj=g_sbc.stk[g]/(-net);
            if (dj>365.f) snprintf(couv,12,">1 an"); else snprintf(couv,12,"%.0f j",dj);
        }
        char netl[12];
        if (net==0.f) snprintf(netl,12,"  0.0"); else snprintf(netl,12,"%+5.1f",net);
        snprintf(buf[nb],130,"%s%-12.12s %6.0f  %s  %6s",
                 (i<base)?"\xe2\x98\x85 ":"  ", resource_name((Resource)g),
                 g_sbc.stk[g], netl, couv);
        draw_text(ren,g_font_small,x+8,y,sb_marche_col(m),buf[nb]); nb++;
        /* LIMITEUR DE PRODUCTION — contrôle [−] cap [+] à droite de la ligne.
         * b=+1 → augmenter ; b=-1 → diminuer. La molette est aussi interceptée. */
        { float capv=econ_prod_cap(s->player, g);
          if (capv<0.f) snprintf(capl[nbc],16,"\xe2\x88\x9e"); /* ∞ UTF-8 */
          else          snprintf(capl[nbc],16,"%d",(int)capv);
          int cx2=x+w-90;
          draw_text(ren,g_font_small,cx2,y,COL_DIM,capl[nbc]); nbc++;
          /* [-] à gauche, [+] à droite du libellé */
          sbhit_add((SDL_Rect){cx2-14,y-1,12,14}, SBH_PROD_CAP, g, -1);
          draw_text(ren,g_font_small,cx2-14,y,COL_DIM,"-");
          sbhit_add((SDL_Rect){cx2+36,y-1,12,14}, SBH_PROD_CAP, g, +1);
          draw_text(ren,g_font_small,cx2+36,y,COL_DIM,"+");
        }
        /* un ordre DISPONIBLE est un état : le chip [exploiter] sur la ligne en tension,
         * si une terre à toi porte ce bien (sinon : rien — le silence informe). */
        if (chips<4 && (m==MARCHE_PENURIE||m==MARCHE_TENDU)){
            for (int r=0;r<s->econ->n_regions;r++){
                const RegionEconomy *re=&s->econ->region[r];
                if (re->owner!=s->player||!re->colonized||re->raw_cap[g]<=0.f) continue;
                sbhit_add((SDL_Rect){x+w-92,y-1,44,14}, SBH_EXPLOIT, r, g);
                draw_text(ren,g_font_small,x+w-92,y,COL_COPPER,"[exploiter]");
                zone_add((SDL_Rect){x+w-92,y-1,44,14},
                    "Aménager l'extraction (mine/carrière) sur ta terre qui porte ce bien — en file, en jours.");
                chips++; break;
            }
        }
        y+=15;
    }
}

/* P4.23 — la CASERNE GÂTE la levée : sans elle, plafond GARDE ; la Caserne ouvre
 * le PIED DE GUERRE ; la Conscription, la LEVÉE EN MASSE. (L'IA reste à GARDE —
 * le palier supérieur est l'avantage que le joueur PAIE en techs.) */
static int player_max_levy(const TechState *ts){
    if (ts && ts->unlocked[TECH_CONSCRIPTION]) return WH_LEVY_MASSE;
    if (ts && ts->unlocked[TECH_CASERNE])      return WH_LEVY_GUERRE;
    return WH_LEVY_GARDE;
}

/* ── panneau ARMÉE : jauge de levée + armée de campagne (posture, ordres) ──── */
static void sb_panel_armee(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, const World *world, int selected){
    static char buf[32][140]; int nb=0;
    int me=s->player;
    long units=warhost_units(s->host, me);
    int lv=warhost_levy(s->host, me);
    snprintf(buf[nb],140,"force mobilisée : %ld régiments", units);
    draw_text(ren,g_font,x+10,y,COL_PARCH,buf[nb]); nb++; y+=22;
    draw_text(ren,g_font_small,x+10,y,COL_DIM,"jauge de levée :"); y+=16;
    { int cx=x+12; int maxl=player_max_levy(&s->ts[me]);   /* P4.23 : la caserne ouvre les crans */
      static const char *HV[4]={
        "Levée basse : on rend les bras à l'économie (mobilisation 0.4×).",
        "Garde : l'entretien normal du temps de paix (1×).",
        "Pied de guerre : la mobilisation presse (1.6×).",
        "LEVÉE EN MASSE (2.6×) : on force la main des familles — la coercition montera à la capitale." };
      const char *LK[4]={ 0,0, tr(STR_ARMEE_LEVY_LOCK_GUERRE), tr(STR_ARMEE_LEVY_LOCK_MASSE) };
      for (int l=0;l<4;l++)
          cx = (l<=maxl) ? sb_chip(ren,cx,y,warhost_levy_name(l), lv==l, SBH_LEVY,l,0,HV[l])
                         : sb_chip_locked(ren,cx,y,warhost_levy_name(l), LK[l]);
    } y+=24;
    if (campaign_active(s->camp, me)){
        int loc=campaign_location(s->camp, me);
        FieldPhase ph=campaign_phase(s->camp, me);
        ArmyComposition comp=campaign_composition(s->camp, me);
        snprintf(buf[nb],140,"armée de campagne — région %d · %s", loc, campaign_phase_name(ph));
        draw_text(ren,g_font_small,x+10,y,COL_COPPER,buf[nb]); nb++; y+=15;
        snprintf(buf[nb],140,"  inf %ld · arch %ld · cav %ld · mages %ld  (Σ %ld régiments)",
                 comp.infanterie, comp.archers, comp.cavalerie, comp.mages, comp.total);
        draw_text(ren,g_font_small,x+10,y,COL_PARCH,buf[nb]); nb++; y+=16;
        draw_text(ren,g_font_small,x+10,y,COL_DIM,"posture :"); y+=15;
        { int cx=x+12; int po=campaign_posture(s->camp, me);
          static const char *HV[3]={
            "Prudente : marche et siège LENTS — on préserve la troupe.",
            "Standard : l'allure du manuel.",
            "Agressive : marche vive, siège tambour battant — on presse." };
          for (int p=0;p<3;p++) cx=sb_chip(ren,cx,y,campaign_posture_name(p), po==p, SBH_POSTURE,p,0,HV[p]);
        } y+=24;
        if (campaign_can_refill(s->camp, s->econ, me)){
            long men=0, mat=0; campaign_refill_cost(s->camp, me, &men, &mat);
            snprintf(buf[nb],140,"[renforcer]  +1 régiment/type — %ld hommes · %ld matériaux", men, mat);
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
            sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_REFILL, 0,0);
            zone_add((SDL_Rect){x+8,y-2,w-16,15},"Recompléter en territoire AMI : on lève des hommes et l'on fabrique les armes (payé sur l'économie).");
            y+=17;
        }
        if (selected>=0 && selected<world->n_provinces){
            int tr=world->province[selected].region;
            if (tr>=0 && tr<s->econ->n_regions && tr!=loc){
                snprintf(buf[nb],140,"[marcher] sur la province sélectionnée (région %d)", tr);
                draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
                sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_MARCH, tr, 0);
                zone_add((SDL_Rect){x+8,y-2,w-16,15},"ORDONNER la marche (itinéraire en jours, terrain décidant) ; une terre ennemie sera ASSIÉGÉE à l'arrivée.");
                y+=17;
            }
        }
    } else {
        if (units>0){
            int cr=sb_capital_region(s,world);
            snprintf(buf[nb],140,"[lever l'ost] rassembler à la capitale (région %d)", cr);
            draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
            sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_MUSTER, cr, 0);
            zone_add((SDL_Rect){x+8,y-2,w-16,15},"Déployer la force mobilisée en ARMÉE DE CAMPAGNE au camp de la capitale — elle marchera sur ordre.");
        }
    }
    /* P4.24 — DÉMOBILISER : l'armée se dissout. Les ARMES sont CONSOMMÉES (aucun
     * matériau rendu) ; les hommes RENTRENT à leur point d'origine (la capitale) et
     * redeviennent main-d'œuvre (topbar : « en armée » → « libre »). */
    if ((campaign_active(s->camp, me) || units>0) && nb<32){
        long camp=campaign_units(s->camp,me);
        long tot = (units>camp)?units:camp;
        y+=18;
        char tnum[16]; snprintf(tnum,sizeof tnum,"%ld",tot);
        tr_fmt(buf[nb],140, STR_ARMEE_DEMOB, tnum);
        draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
        sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_DEMOB, 0,0);
        zone_add((SDL_Rect){x+8,y-2,w-16,15}, tr(STR_ARMEE_DEMOB_HOV));
        y+=17;
    }
    /* ── LA FLOTTE (mer §5) : coques · chantier · rade — et l'embarquement ── */
    y+=8;
    { const Navy *nv=&s->navy->n[me];
      int port=navy_best_port(world, s->econ, me);
      draw_text(ren,g_font_small,x+10,y,COL_DIM,"Flotte"); y+=15;
      if (port<0){
          /* P4.25 — donnée seule (« Flotte 0 ») ; l'explication vit en hover. */
          draw_text(ren,g_font_small,x+12,y,COL_DIM,"0");
          zone_add((SDL_Rect){x+8,y-2,300,15},
                   s->econ->region[sb_capital_region(s,world)].coastal
                   ? "Aucune rade : bâtir un Port (panneau Bâtir) ouvre la mer."
                   : "Pays sans accès à la mer d'ici : la flotte n'a pas de rade.");
          y+=17;
      } else {
          snprintf(buf[nb],140,"%d combat · %d transport(s) (%d en mer) · %d marchand(s) · %d pirate(s)",
                   nv->hull[HULL_WAR], nv->hull[HULL_TRANSPORT], nv->at_sea, nv->hull[HULL_MERCHANT], nv->hull[HULL_PIRATE]);
          draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++; y+=17;
          { float worst=0.f;   /* coques §7 : l'état des routes, en mots */
            const TradeRoute *asym=NULL; bool asym_ab_down=true;
            for (int i=0;i<s->rn->n;i++){ const TradeRoute *t=&s->rn->route[i];
                if (!t->open) continue;
                bool mine=(s->econ->region[t->ra].owner==me || s->econ->region[t->rb].owner==me);
                if (!mine) continue;
                if (t->maritime && t->pirate_press>worst) worst=t->pirate_press;
                /* commerce asym. §5 : la première route DIRECTIONNELLE se détaille en mots */
                if (!asym && (t->fluvial || (t->maritime && t->days_ab>0.f && t->days_ba>0.f
                                             && fabsf(t->days_ab-t->days_ba)>2.f))){
                    asym=t;
                    asym_ab_down = t->fluvial ? (t->fluvial==1) : (t->days_ab<t->days_ba);
                } }
            if (worst>0.f){
                snprintf(buf[nb],140,"routes : %s", worst>=90.f?"BLOQUÉES":worst>2.f?"infestées":"harcelées");
                draw_text(ren,g_font_small,x+12,y,worst>2.f?COL_COPPER:COL_DIM,buf[nb]); nb++; y+=17;
            }
            if (asym){
                int down_end = asym_ab_down ? asym->rb : asym->ra;
                snprintf(buf[nb],140,"%s rég. %d : en aval — abondant · retour : maigre et précieux",
                         asym->fluvial?"fleuve vers":"couloir vers", down_end);
                draw_text(ren,g_font_small,x+12,y,COL_DIM,buf[nb]); nb++;
                zone_add((SDL_Rect){x+8,y-2,w-16,15},"Le coût de transport a un SENS : le vrac (grain, bois, charbon) ne paie que la descente ; seul le précieux (étoffe, orfèvrerie, remèdes, armes) paie la remontée. Le volume suit la facilité — divisé à contre-courant, jamais nul.");
                y+=17;
            } }
          if (nv->hull[HULL_MERCHANT]>0 || nv->hull[HULL_PIRATE]>0){
              bool top=(nv->hull[HULL_MERCHANT]>0);
              snprintf(buf[nb],140,"[convertir : %s]  la course %s",
                       top?"marchand → pirate":"pirate → marchand",
                       top?"(revenu déniable — la rancune s'armera)":"(désarmer apaise)");
              draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
              sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_NAVY_CONV, top?1:0, 0);
              zone_add((SDL_Rect){x+8,y-2,w-16,15},"Conversion AU CHANTIER, peu coûteuse, réversible : c'est sa nature. Le pirate niche en eaux mortes, pille les côtes (1/10), saigne les routes — et désigne son commanditaire s'il est pris.");
              y+=17;
          }
          if (nv->build_hull>=0){
              snprintf(buf[nb],140,"chantier : %s — %d j", navy_hull_name((HullType)nv->build_hull),(int)nv->build_days);
              draw_text(ren,g_font_small,x+12,y,COL_DIM,buf[nb]); nb++; y+=17;
          } else {
              static const HullType BB[3]={HULL_TRANSPORT,HULL_MERCHANT,HULL_WAR};
              for (int k=0;k<3;k++){
                  snprintf(buf[nb],140,"[chantier : %s]  %.0f or — fournitures navales + bois%s",
                           navy_hull_name(BB[k]), navy_build_gold(s->econ,port,BB[k]),
                           BB[k]==HULL_WAR?" + métal":"");
                  draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
                  sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_NAVY_BUILD, (int)BB[k], 0);
                  zone_add((SDL_Rect){x+8,y-2,w-16,15},"Commander une coque au chantier de la rade : la recette s'achète AU MARCHÉ (la Scierie navale a un débouché).");
                  y+=17;
              }
          }
          /* l'embarquement : armée au port + capacité + province visée côtière */
          const FieldArmy *fa2=&s->camp->army[me];
          if (fa2->active && fa2->phase==FA_IDLE && fa2->loc==port
              && selected>=0 && selected<world->n_provinces){
              int tr=world->province[selected].region;
              if (tr>=0 && tr<s->econ->n_regions && tr!=port && s->econ->region[tr].coastal){
                  float aller=navy_sea_days_regions(world,port,tr);
                  float retour=navy_sea_days_regions(world,tr,port);
                  long pk=campaign_units(s->camp,me);
                  int need=(int)((pk+9)/10);
                  if (aller>=0.f && navy_transport_packets_free(s->navy,me)>=(int)pk){
                      snprintf(buf[nb],140,"[embarquer] vers la région %d — %.0f j (retour %.0f j) · %d transport(s)",
                               tr, aller, retour, need);
                      draw_text(ren,g_font_small,x+12,y,COL_PARCH,buf[nb]); nb++;
                      sbhit_add((SDL_Rect){x+8,y-2,w-16,15}, SBH_SAIL, tr, 0);
                      zone_add((SDL_Rect){x+8,y-2,w-16,15},"Embarquer l'armée : charger (jours), traverser (l'ALLER ne vaut pas le RETOUR — la volta), débarquer (hors port : plus lent, exposé).");
                      y+=17;
                  } else if (aller>=0.f){
                      snprintf(buf[nb],140,"traversée %.0f j — transports insuffisants (%d requis)", aller, need);
                      draw_text(ren,g_font_small,x+12,y,COL_DIM,buf[nb]); nb++; y+=17;
                  }
              }
          }
      }
    }
    (void)h;
}

/* ── panneau FILTRES : chips → ViewMode existants + lentilles readout ──────── */
/* §terrain — mot FACE-JOUEUR du casus belli (jamais diplo_cb_name, qui est télémétrie). */
static StrId cb_str(CasusBelli cb){
    switch(cb){ case CB_TERRITORIAL:   return STR_CB_TERRITORIAL;
                case CB_RELIGIOUS:     return STR_CB_RELIGIOUS;
                case CB_ECONOMIC:      return STR_CB_ECONOMIC;
                case CB_SUBJUGATION:   return STR_CB_SUBJUGATION;
                case CB_ANTIPIRATERIE: return STR_CB_ANTIPIRATERIE;
                default:               return STR_DIPLO_SANS_CB; }
}
/* §terrain — DIPLOMATIE (le joueur belligérant) : les pays VIVANTS, leur statut, et
 * les verbes : DÉCLARER (si un casus belli inhérent tient) · NÉGOCIER (en guerre).
 * Membrane : on LIT diplo (statut, score, CB, rancune) et on pose des ACTIONS ;
 * l'arbitrage de la paix est l'IA exposée — aucune logique diplomatique nouvelle. */
static void sb_panel_diplo(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, World *world){
    (void)w;
    int me=s->player, bottom=y+h;
    int off=g_sb.scroll[SBT_DIPLO], idx=0;
    for (int c=0; c<world->n_countries && y<bottom-30; c++){
        if (c==me || world->country[c].role==POLITY_UNCLAIMED) continue;
        if (idx++ < off) continue;
        DiploStatus st=diplo_status(s->dp,me,c);
        const char *sw = (st==DIPLO_WAR)            ? tr(STR_DIPLO_GUERRE)
                       : (st==DIPLO_ALLIED)         ? tr(STR_DIPLO_ALLIE)
                       : (diplo_suzerain(s->dp,c)==me) ? tr(STR_DIPLO_VASSAL)
                       : (diplo_suzerain(s->dp,me)==c) ? tr(STR_DIPLO_SUZERAIN)
                       :                              tr(STR_DIPLO_NEUTRE);
        SDL_Color nc = (st==DIPLO_WAR)   ? (SDL_Color){0xe0,0x6a,0x4a,0xff}
                     : (st==DIPLO_ALLIED)? (SDL_Color){0x6a,0xc0,0x8a,0xff} : COL_PARCH;
        draw_text(ren,g_font_small,x+10, y, nc, sb_country_name(world,c));
        draw_text(ren,g_font_small,x+150,y, COL_DIM, sw);
        if (diplo_rancor(s->dp,c,me) > 1.f)
            draw_text(ren,g_font_small,x+250,y, (SDL_Color){0xc0,0x80,0x50,0xff}, tr(STR_DIPLO_RANCUNE));
        y+=15;
        /* M3 — LE PACTE COMMERCIAL (hors guerre) : statut RÉCIPROQUE + signer/rompre.
         * Il ouvre l'accès au marché GLOBAL du partenaire si l'un tient un Centre. */
        if (st!=DIPLO_WAR){
            bool pact=diplo_trade_pact(s->dp,me,c);
            bool phub=intertrade_country_has_centre(s->econ,c);
            draw_text(ren,g_font_small,x+12,y+2, pact?(SDL_Color){0x8c,0xc0,0x9c,0xff}:COL_PANEL2,
                      pact?(phub?tr(STR_PACT_GLOBAL):tr(STR_PACT_ACTIF)):tr(STR_PACT_AUCUN));
            (void)sb_chip(ren,x+210,y, pact?tr(STR_PACT_BREAK):tr(STR_PACT_SIGN), pact,
                          SBH_PACT, c, pact?0:1, tr(STR_PACT_HOV));
            y+=18;
        }
        if (st==DIPLO_WAR){
            char sz[48], num[16];
            snprintf(num,sizeof num,"%+d",(int)diplo_war_score(s->dp,me,c));
            tr_fmt(sz,sizeof sz,STR_DIPLO_SCORE_FMT,num);
            draw_text(ren,g_font_small,x+12,y+2,COL_DIM,sz);
            (void)sb_chip(ren,x+150,y, tr(STR_DIPLO_NEGOCIER), false, SBH_PEACE, c, 0, NULL);
        } else if (st==DIPLO_NEUTRAL && diplo_can_declare(s->dp,me,c)){
            CasusBelli cb=diplo_casus_belli(world,s->econ,s->wp,s->dp,me,c,RES_NONE);
            if (cb!=CB_NONE){
                char mz[64]; tr_fmt(mz,sizeof mz,STR_DIPLO_MOTIF_FMT, tr(cb_str(cb)));
                draw_text(ren,g_font_small,x+12,y+2,COL_DIM,mz);
                (void)sb_chip(ren,x+150,y, tr(STR_DIPLO_DECLARER), false, SBH_DECLARE, c, (int)cb, NULL);
            } else {
                draw_text(ren,g_font_small,x+12,y+2,COL_PANEL2,tr(STR_DIPLO_SANS_CB));
            }
        }
        y+=18;
    }
}

static void sb_panel_filtres(SDL_Renderer *ren, int x, int y, int w, int h, ViewMode mode){
    (void)w; (void)h;
    struct { const char *grp; struct { const char *lbl; int mode; } it[7]; int n; } G[3]={
        { "Souveraineté", { {"Politique",VIEW_POLITICAL},{"Pays",VIEW_COUNTRIES},{"Régions",VIEW_REGIONS},{"Continents",VIEW_CONTINENTS} }, 4 },
        { "Peuples",      { {"Culture",VIEW_CULTURE},{"Foi",VIEW_FAITH} }, 2 },
        { "Terre",        { {"Relief",VIEW_TERRAIN},{"Altitude",VIEW_HEIGHT},{"Fertilité",VIEW_FERTILITY},{"Humidité",VIEW_MOISTURE},{"Température",VIEW_TEMPERATURE},{"Ressources",VIEW_RESOURCES},{"Habitabilité",VIEW_HABITABILITY} }, 7 },
    };
    for (int g=0;g<3;g++){
        draw_text(ren,g_font_small,x+10,y,COL_DIM,G[g].grp); y+=15;
        int cx=x+12;
        for (int i=0;i<G[g].n;i++){
            if (cx>x+SB_DRAWER_W-90){ cx=x+12; y+=21; }
            bool actif=(g_sb.lens==LENS_NONE && mode==(ViewMode)G[g].it[i].mode);
            cx=sb_chip(ren,cx,y,G[g].it[i].lbl,actif,SBH_CHIP_MODE,G[g].it[i].mode,0,VIEW_NAMES[G[g].it[i].mode]);
        }
        y+=24;
    }
    { int cx=x+12;
      (void)sb_chip(ren,cx,y,"Courants", g_sb.show_currents, SBH_CHIP_CUR,0,0,
          "Le champ des COURANTS marins (physique du monde — flux continus) : couloirs, eaux vives ; les eaux MORTES se lisent en creux.");
    } y+=24;
    draw_text(ren,g_font_small,x+10,y,COL_DIM,"Empire (lentilles — par bande, 5 teintes)"); y+=15;
    { int cx=x+12;
      static const char *HV[3]={
        "Chaque province colorée par sa bande de PROSPÉRITÉ (misère → opulence).",
        "Où ça gronde : la bande d'HUMEUR locale (révoltée → dévouée).",
        "Où sont les PÉNURIES : la pire tension du panier (grain/outils/fer/étoffe/vin)." };
      cx=sb_chip(ren,cx,y,"Prospérité", g_sb.lens==LENS_PROSP,  SBH_CHIP_LENS, LENS_PROSP,0, HV[0]);
      cx=sb_chip(ren,cx,y,"Humeur",     g_sb.lens==LENS_HUMEUR, SBH_CHIP_LENS, LENS_HUMEUR,0,HV[1]);
      (void)sb_chip(ren,cx,y,"Marché",  g_sb.lens==LENS_MARCHE, SBH_CHIP_LENS, LENS_MARCHE,0,HV[2]);
    }
}

/* ── zone basse : LES LEVIERS (brief leviers §1) — gouverner d'où l'on observe ──
 * Toujours la même, quel que soit l'onglet. Intérieur (mater·former·purger — cible :
 * la province SÉLECTIONNÉE, à toi) + Couronne (embargo · contrats — cible : le pays
 * de la sélection). Chaque bouton : coût AVANT (survol), ordre en file/en jours.
 * La PURGE exige DEUX clics — un acte irréversible mérite deux temps. */
/* ── Q1 — LE CONSEIL (I7) : la section ÉTAT. Trois sièges (Savoir/Société/Industrie) ;
 *   un siège vacant propose ses candidats (nom·tier·coût) en chips « Nommer », un siège
 *   pourvu montre son conseiller + « Renvoyer ». Le clic appelle l'API EXISTANTE
 *   (statecraft_council_hire/_dismiss) — celle que l'IA utilise déjà. Pur câblage UI. ── */
static void sb_panel_conseil(SDL_Renderer *ren, int x, int y, int w, int h, Sim *s, World *world){
    (void)h;
    int cid=s->player; uint32_t seed=world->seed;
    float ipm=econ_world_ipm(s->econ);
    static const int seat_pct[SC_COUNCIL_SEATS]={20,12,15};   /* effets de base (affichage) */
    int ry=y+2;
    for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
        char pc[8]; snprintf(pc,sizeof pc,"%d",seat_pct[seat]);
        char hd[96]; tr_fmt(hd,sizeof hd,STR_COUNCIL_SEAT_FMT,
                            tr((StrId)(STR_COUNCIL_SEAT_0+seat)), pc, tr((StrId)(STR_COUNCIL_EFF_0+seat)));
        draw_text(ren,g_font,x+10,ry,COL_COPPER,hd); ry+=18;
        int slot=statecraft_council_seated(s->sc,cid,seat);
        if (slot>=0){
            int tier=statecraft_council_cand_tier(seed,cid,seat,slot);
            int nm  =statecraft_council_cand_name(seed,cid,seat,slot);
            char ts[8];  snprintf(ts,sizeof ts,"%d",tier);
            char cs[12]; snprintf(cs,sizeof cs,"%.0f",statecraft_council_cand_cost(seed,cid,seat,slot,ipm));
            char line[96]; tr_fmt(line,sizeof line,STR_COUNCIL_SEATED_FMT, tr((StrId)nm), ts, cs);
            draw_text(ren,g_font_small,x+16,ry+2,COL_PARCH,line);
            sb_chip(ren,x+w-80,ry,tr(STR_COUNCIL_RENVOYER),false,SBH_COUNCIL,seat,-1,
                    "Renvoyer ce conseiller : le siège redevient vacant, le coût mensuel cesse.");
            ry+=22;
        } else {
            draw_text(ren,g_font_small,x+16,ry+2,COL_DIM,tr(STR_COUNCIL_VACANT)); ry+=18;
            for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
                int tier=statecraft_council_cand_tier(seed,cid,seat,sl);
                int nm  =statecraft_council_cand_name(seed,cid,seat,sl);
                char ts[8];  snprintf(ts,sizeof ts,"%d",tier);
                char cs[12]; snprintf(cs,sizeof cs,"%.0f",statecraft_council_cand_cost(seed,cid,seat,sl,ipm));
                char lab[64]; tr_fmt(lab,sizeof lab,STR_COUNCIL_CAND_FMT, tr((StrId)nm), ts, cs);
                sb_chip(ren,x+16,ry,lab,false,SBH_COUNCIL,seat,sl,
                        "Nommer ce conseiller (tier = effet ×1 / ×1.5 / ×2 ; coût mensuel ×IPM).");
                ry+=19;
            }
        }
        ry+=6;
    }
}
static void sb_panel_leviers(SDL_Renderer *ren, int x, int y, int w, Sim *s,
                             const World *world, int selected){
    fill_rect(ren,x+6,y-4,w-12,1,COL_PANEL2);
    draw_text(ren,g_font_small,x+10,y,COL_COPPER,"LEVIERS");
    int selreg=(selected>=0&&selected<world->n_provinces)?world->province[selected].region:-1;
    bool mine=(selreg>=0&&selreg<s->econ->n_regions&&s->econ->region[selreg].owner==s->player);
    int tgt=(selreg>=0&&selreg<s->econ->n_regions)?s->econ->region[selreg].owner:-1;
    bool foreign=(tgt>=0&&tgt!=s->player);
    /* rangée INTÉRIEUR */
    { int cx=x+10, ry=y+14;
      draw_text(ren,g_font_small,cx,ry,COL_DIM,"Intérieur:"); cx+=62;
      if (mine){
          cx=sb_chip(ren,cx,ry,"Mater",false,SBH_LEV_REPRESS,selreg,0,
              "MATER (30 j) : la botte — l'agitation se TAIT (le grief est masqué, il ressortira amplifié) ; l'Assise glisse vers Contrainte.");
          bool creuset = s->ts[s->player].unlocked[TECH_INTEGRATION];
          cx=sb_chip(ren,cx,ry,creuset?"Former (Creuset)":"Former",false,SBH_LEV_ASSIM,selreg,creuset?1:0,
              "FORMER (1 an) : écoles & magistrats — la minorité s'assimile plus vite. AVERTISSEMENT : cette culture est une source de savoir — la former, c'est TARIR le canal.");
          (void)sb_chip(ren,cx,ry, g_sb.purge_arm?"CONFIRMER LA PURGE ?":"Purger…",
              g_sb.purge_arm,SBH_LEV_PURGE,selreg,0,
              g_sb.purge_arm? "SECOND CLIC = la purge commence : ~12 % du groupe périra PAR AN pendant 4 ans · la stabilité vacillera des années · la Brèche se RAPPROCHERA."
                            : "PROCLAMER LA PURGE (4 ans, par tranches, arrêtable au prix du gâchis) : des familles périront · stabilité plombée · CHARGE faustienne. Deux clics.");
      }
    }
    /* rangée COURONNE */
    { int cx=x+10, ry=y+38;
      draw_text(ren,g_font_small,cx,ry,COL_DIM,"Couronne:"); cx+=62;
      cx=sb_chip(ren,cx,ry,"Embargo…",false,SBH_LEV_EMBARGO,0,0,
          "Ouvre Économie·Commerce : décréter/lever l'embargo par partenaire (le commerce perdu se lit AVANT).");
      if (foreign){
          float ws=diplo_war_score(s->dp, s->player, tgt);
          bool at_war=(diplo_status(s->dp,s->player,tgt)==DIPLO_WAR);
          bool winning=(at_war && ws>=15.f);
          bool free_v=(diplo_suzerain(s->dp,tgt)<0);
          if (winning && free_v){
              cx=sb_chip(ren,cx,ry,"Imposer: servage",false,SBH_LEV_CONTRAT,CONTRAT_SERVAGE,tgt,
                  "IMPOSER LE SERVAGE au vaincu : tribut LOURD + levées — mais la coercition et la fracture montent CHEZ TOI aussi (parenté servile) ; un serf complote.");
              cx=sb_chip(ren,cx,ry,"protectorat",false,SBH_LEV_CONTRAT,CONTRAT_PROTECTORAT,tgt,
                  "IMPOSER LE PROTECTORAT : tribut léger, un glacis — ses guerres t'APPELLERONT (un protecteur qui ne vient pas perd le contrat ET la face).");
          } else if (free_v && !at_war){
              if (world->country[tgt].role==POLITY_CITY_STATE)
                  cx=sb_chip(ren,cx,ry,"Lier la cité",false,SBH_LEV_CONTRAT,CONTRAT_CITE,tgt,
                      "CONTRAT DE CITÉ : route marchande GARANTIE (ni guerre ni embargo) — la cité reste LIBRE et inannexable tant que le contrat tient.");
              else {
                  int cr=sb_capital_region(s,world);
                  int tr=(world->country[tgt].capital_prov>=0)?world->province[world->country[tgt].capital_prov].region:-1;
                  bool same_credo=(cr>=0&&tr>=0&&tr<s->econ->n_regions&&cr<s->econ->n_regions
                                   && s->econ->region[cr].culture.credo==s->econ->region[tr].culture.credo);
                  if (same_credo)
                      cx=sb_chip(ren,cx,ry,"Concordat",false,SBH_LEV_CONTRAT,CONTRAT_CONCORDAT,tgt,
                          "CONCORDAT (co-religionnaires) : légitimité partagée, canal de foi — pas d'or ; l'hérésie de l'un devient le souci de l'autre.");
              }
          }
          (void)cx;
      }
    }
}

/* ── le RAIL + le TIROIR ───────────────────────────────────────────────────── */
static void draw_sidebar(SDL_Renderer *ren, int win_w, int win_h, Sim *s, World *world,
                         ViewMode mode, int selected){
    (void)win_w;
    sbhit_reset();
    if (s->ready) sb_cache_refresh(s, s->day);                /* agrégats : au pas de jour (§8) */
    /* glissement (~150 ms, dt interne — l'overlay est redessiné chaque frame) */
    { static Uint32 sb_last=0; Uint32 now=SDL_GetTicks();
      float dtf = sb_last ? (float)(now-sb_last)/1000.f : 0.016f;
      if (dtf>0.1f) dtf=0.1f;
      sb_last=now;
      float target=(g_sb.tab>=0)?1.f:0.f;
      if (g_sb.anim<target) { g_sb.anim+=dtf/0.15f; if(g_sb.anim>1.f)g_sb.anim=1.f; }
      if (g_sb.anim>target) { g_sb.anim-=dtf/0.15f; if(g_sb.anim<0.f)g_sb.anim=0.f; }
    }
    int dw=(int)(SB_DRAWER_W*g_sb.anim);
    /* tiroir — il ÉPOUSE son contenu (borné), commence SOUS la topbar et s'arrête
     * AU-DESSUS de la rangée de chips : aucun calque ne recouvre l'autre. */
    if (dw>2 && s->ready){
        int dx=SB_RAIL_W, dy=70;
        int content=120;                              /* mesure grossière par onglet */
        switch(g_sb.tab){
            case SBT_STOCKS:{ int nn=0; for(int g=1;g<RES_COUNT;g++) if (sb_good_alive(g)) nn++;
                              content=20+nn*15+8; } break;
            case SBT_MARCHE:{ int nn=0;
                if (g_sb.marche_sub==0){ for(int g=1;g<RES_COUNT;g++) if (g_sbc.stk[g]>=0.5f) nn++; }
                else { int cap=sb_capital_region(s,world); int hub=(cap>=0)?intertrade_region_hub(cap):-1;
                       for(int g=1;g<RES_COUNT;g++){ float a=(g_sb.marche_sub==2)?intertrade_global_stock(s->econ,g)
                                                          :((hub>=0)?s->econ->region[hub].stock[g]:0.f);
                                                     if(a>=0.5f) nn++; } }
                content=24+18+16+15+nn*16+8; } break;
            case SBT_ECO:{ int nn=0;
                if (g_sb.eco_sub==1){ for(int g=1;g<RES_COUNT;g++) if (g_sbc.dem[g]>0.05f||g_sbc.sup[g]>0.05f) nn++; }
                else if (g_sb.eco_sub==2){ for(int g=1;g<RES_COUNT;g++) if (intertrade_import_vol(s->player,g)>0.05f||intertrade_export_vol(s->player,g)>0.05f) nn++; }
                else { for(int c=0;c<world->n_countries;c++){ if(c==s->player) continue;
                        if (intertrade_pair_value(s->player,c)>0.5f||intertrade_embargoed(s->player,c)
                            ||diplo_status(s->dp,s->player,c)==DIPLO_WAR) nn++; } if(nn>10)nn=10; nn+=2; }
                content=24+20+nn*16+8; } break;
            case SBT_DEMO: content=22+3*15+8+16+15+6*15+24; break;
            case SBT_ARMEE: content=22+16+24+6*16+3*17+8; break;
            case SBT_FILTRES: content=3*40+15+24+30; break;
            case SBT_DIPLO:{ int nn=0;
                             for(int c=0;c<world->n_countries;c++){
                                 if(c!=s->player && world->country[c].role!=POLITY_UNCLAIMED) nn++; }
                             content=22+(nn>16?16:nn)*30+8; } break;
            case SBT_ETAT: content=3*(18+18+3*19+6); break;   /* Q1 : 3 sièges, pire cas (vacants) */
            default: break;
        }
        int dh=32+content+96;                          /* titre + contenu + zone Leviers */
        int dhmax=win_h-dy-52;                         /* au-dessus des chips de mode */
        if (dh>dhmax) dh=dhmax;
        if (dh<200)   dh=200;
        panel_bg(ren,dx,dy,dw,dh);
        if (g_sb.anim>0.95f){
            static const char *TITRE[SBT_COUNT]={"ÉCONOMIE","DÉMOGRAPHIE","STOCKS","MARCHÉ","ARMÉE","FILTRES DE CARTE",NULL,NULL};
            if (g_sb.tab>=0&&g_sb.tab<SBT_COUNT)
                draw_text(ren,g_font,dx+10,dy+8,COL_COPPER,
                          (g_sb.tab==SBT_DIPLO)? tr(STR_DIPLO_TITRE) :
                          (g_sb.tab==SBT_ETAT) ? tr(STR_COUNCIL_TITRE) : TITRE[g_sb.tab]);
            int py=dy+32, ph=dy+dh-96;          /* on réserve la zone basse aux LEVIERS */
            switch(g_sb.tab){
                case SBT_ECO:     sb_panel_eco   (ren,dx,py,dw,ph,s,world); break;
                case SBT_DEMO:    sb_panel_demo  (ren,dx,py,dw,ph,s,world); break;
                case SBT_STOCKS:  sb_panel_stocks(ren,dx,py,dw,ph,s,world); break;
                case SBT_MARCHE:  sb_panel_marche(ren,dx,py,dw,ph,s,world); break;
                case SBT_ARMEE:   sb_panel_armee (ren,dx,py,dw,ph,s,world,selected); break;
                case SBT_FILTRES: sb_panel_filtres(ren,dx,py,dw,ph,mode); break;
                case SBT_DIPLO:   sb_panel_diplo (ren,dx,py,dw,ph,s,world); break;
                case SBT_ETAT:    sb_panel_conseil(ren,dx,py,dw,ph,s,world); break;   /* Q1 */
                default: break;
            }
            sb_panel_leviers(ren, dx, dy+dh-92, dw, s, world, selected);   /* zone basse : toujours là */
        }
    }
    /* rail (toujours visible, par-dessus) */
    fill_rect(ren,0,0,SB_RAIL_W,win_h,(SDL_Color){0x0c,0x12,0x1d,0xfa});
    fill_rect(ren,SB_RAIL_W-1,0,1,win_h,COL_PANEL2);
    static const char *IC[SBT_COUNT] ={"E","D","S","M","A","F","G","C"};   /* M = Marché (#5) · C = Conseil (Q1) */
    static const char *NM[SBT_COUNT] ={"Économie (E) — commerce, marché, import/export",
                                       "Démographie (D) — classes, peuples, relocalisation",
                                       "Stocks (S) — tensions, couverture, exploiter",
                                       "Marché (M) — stock local · régional · global ; achat/vente direct",
                                       "Armée (A) — levée, campagne, posture",
                                       "Filtres (F) — vues de carte & lentilles d'empire",
                                       NULL /* DIPLO : hover en STR_* (migré) */,
                                       NULL /* ÉTAT : hover en STR_* (Q1) */};
    for (int t=0;t<SBT_COUNT;t++){
        int by=52+t*44;
        bool actif=(g_sb.tab==t);
        if (actif) fill_rect(ren,0,by-6,SB_RAIL_W,32,(SDL_Color){0x2a,0x20,0x14,0xff});
        draw_text(ren,g_font_big,14,by,actif?COL_COPPER:COL_DIM,IC[t]);
        sbhit_add((SDL_Rect){0,by-6,SB_RAIL_W,32}, SBH_TAB, t, 0);
        zone_add((SDL_Rect){0,by-6,SB_RAIL_W,32}, (t==SBT_DIPLO)? tr(STR_RAIL_DIPLO) :
                 (t==SBT_ETAT)? tr(STR_COUNCIL_TITRE) : NM[t]);
    }
}

/* clic dans la sidebar — exécute la DÉCISION par l'actionneur ; true si consommé */
static bool sidebar_click(Sim *s, World *world, int mx, int my, ViewMode *mode, bool *dirty){
    int hit=-1;
    for (int i=0;i<g_nsbhits;i++){ SDL_Rect *r=&g_sbhits[i].r;
        if (mx>=r->x&&mx<r->x+r->w&&my>=r->y&&my<r->y+r->h){ hit=i; break; } }
    if (hit<0){
        int dw=(int)(SB_DRAWER_W*g_sb.anim);
        return (mx < SB_RAIL_W + ((g_sb.tab>=0)?dw:0));   /* le tiroir absorbe le clic perdu */
    }
    SbHit *hh=&g_sbhits[hit]; *dirty=true;
    /* §terrain — OBSERVATEUR (joueur vaincu) : LECTURE SEULE. On laisse la NAVIGATION et
     * les VUES (onglets, sous-onglets, modes/lentilles de carte), on bloque tout ACTE. */
    if (g_observer && hh->kind!=SBH_TAB && hh->kind!=SBH_ECOSUB && hh->kind!=SBH_MARCHE_SUB
        && hh->kind!=SBH_CHIP_MODE && hh->kind!=SBH_CHIP_LENS && hh->kind!=SBH_CHIP_CUR)
        return true;
    if (hh->kind!=SBH_LEV_PURGE) g_sb.purge_arm=false;   /* tout autre clic DÉSARME la purge */
    switch(hh->kind){
        case SBH_TAB:      g_sb.tab=(g_sb.tab==hh->a)?-1:hh->a; break;
        case SBH_ECOSUB:   g_sb.eco_sub=hh->a; break;
        case SBH_MARCHE_SUB: g_sb.marche_sub=hh->a; break;       /* #5 : navigation des étages */
        case SBH_BUY: case SBH_SELL: {                           /* #5 : achat/vente DIRECT (actionneur) */
            int cap=sb_capital_region(s, world);
            int step=(SDL_GetModState()&KMOD_SHIFT)?100:10;      /* Maj = palier de 100 */
            long org=0;
            if (hh->kind==SBH_BUY){ long got=intertrade_market_buy(s->econ, cap, hh->a, step, hh->b, &org);
                if (got>0) printf("\n[scps] Achat : %ld %s au marché %s (−%ld or).\n",
                                  got, resource_name((Resource)hh->a), hh->b?"mondial":"régional", org);
                else printf("\n[scps] Achat refusé (%s indisponible ou trésor insuffisant).\n",
                            resource_name((Resource)hh->a));
            } else { long sold=intertrade_market_sell(s->econ, cap, hh->a, step, hh->b, &org);
                if (sold>0) printf("\n[scps] Vente : %ld %s au marché %s (+%ld or).\n",
                                   sold, resource_name((Resource)hh->a), hh->b?"mondial":"régional", org);
            }
            break;
        }
        case SBH_PACT:                                           /* M3 : signer/rompre un pacte commercial (réciproque) */
            diplo_set_trade_pact(s->dp, s->player, hh->a, hh->b!=0);
            printf("\n[scps] Pacte commercial %s avec %s — accès RÉCIPROQUE au marché global (si l'un tient un Centre).\n",
                   hh->b? "SIGNÉ":"ROMPU", sb_country_name(world, hh->a));
            break;
        case SBH_EMBARGO:
            intertrade_order_embargo(s->player, hh->a, hh->b!=0);
            printf("\n[scps] %s contre %s — le négoce %s.\n",
                   hh->b? "EMBARGO décrété":"Embargo LEVÉ", sb_country_name(world,hh->a),
                   hh->b? "se ferme":"peut reprendre");
            break;
        case SBH_RELOC_SRC: g_sb.reloc_src=hh->a; break;
        case SBH_RELOC_CLR: g_sb.reloc_src=-1; break;
        case SBH_RELOC_DST:
            if (g_sb.reloc_src>=0 && agency_order_relocate(s->ag, g_sb.reloc_src, hh->a))
                printf("\n[scps] Convoi ordonné : ~%d familles, région %d → %d (90 j) — la coercition montera à la source.\n",
                       AGY_RELOC_POP, g_sb.reloc_src, hh->a);
            g_sb.reloc_src=-1;
            break;
        case SBH_EXPLOIT:
            if (agency_order_exploit(s->ag, hh->a, (Resource)hh->b))
                printf("\n[scps] Exploitation en file : %s, région %d (en jours).\n",
                       resource_name((Resource)hh->b), hh->a);
            break;
        case SBH_LEVY:
            if (hh->a <= player_max_levy(&s->ts[s->player])){       /* P4.23 : garde-fou (le palier verrouillé n'a pas de hit) */
                warhost_set_levy(s->host, s->player, hh->a);
                printf("\n[scps] Levée : %s.\n", warhost_levy_name(hh->a));
            }
            break;
        case SBH_POSTURE:  campaign_set_posture(s->camp, s->player, hh->a); break;
        case SBH_REFILL: {
            int got=campaign_refill(s->camp, s->player, s->econ, s->labor);
            printf("\n[scps] Renfort : %d paquet(s) levés en territoire ami.\n", got);
        } break;
        case SBH_MARCH: {
            const FieldArmy *fa=&s->camp->army[s->player];
            if (campaign_order(s->camp, s->econ, s->player, fa->loc, hh->a, &fa->force))
                printf("\n[scps] L'armée marche sur la région %d (itinéraire en jours).\n", hh->a);
            else if (campaign_order_sea(s->camp, world, s->econ, s->navy, s->player, fa->loc, hh->a, &fa->force))
                printf("\n[scps] La terre ne mène pas là : l'armée EMBARQUE pour la région %d.\n", hh->a);
        } break;
        case SBH_SAIL: {
            const FieldArmy *fa=&s->camp->army[s->player];
            if (campaign_order_sea(s->camp, world, s->econ, s->navy, s->player, fa->loc, hh->a, &fa->force))
                printf("\n[scps] L'armée embarque pour la région %d (la volta décidera des jours).\n", hh->a);
        } break;
        case SBH_NAVY_BUILD:
            if (navy_order_build(s->navy, world, s->econ, s->player, (HullType)hh->a))
                printf("\n[scps] Chantier naval : %s en construction.\n", navy_hull_name((HullType)hh->a));
            break;
        case SBH_NAVY_CONV:
            if (navy_convert(s->navy, world, s->econ, s->player, hh->a!=0))
                printf("\n[scps] Chantier : %s.\n", hh->a?"un marchand passe à la COURSE":"un pirate rentre dans le rang");
            break;
        case SBH_MUSTER:
            if (hh->a>=0 && campaign_order(s->camp, s->econ, s->player, hh->a, hh->a, &s->host->army[s->player]))
                printf("\n[scps] L'ost se rassemble au camp de la capitale (région %d).\n", hh->a);
            break;
        case SBH_DEMOB: {                                  /* P4.24 — DÉMOBILISER */
            long camp = campaign_disband(s->camp, s->player);     /* l'armée de campagne quitte la carte */
            long res  = warhost_disband(s->host, s->player);      /* la réserve levée se dissout (jauge → garde) */
            long packets = (res>camp)?res:camp;                   /* (la campagne est une COPIE : ne pas doubler) */
            /* POPULATIONS → POINT D'ORIGINE : la main-d'œuvre réservée repasse en
             * civils (la capitale = le foyer de levée). Les ARMES sont CONSOMMÉES :
             * aucun matériau/or n'est rendu (coût irrécupérable). */
            long home = packets*POP_PER_SLOT;
            for (int p=0; p<s->labor->n_prov; p++){
                long *pia=&s->labor->prov[p].pop_in_army;
                long back=(*pia<home)?*pia:home; *pia-=back; home-=back;
                if (home<=0) break;
            }
            printf("\n[scps] Démobilisation : %ld régiments rentrent au foyer ; les armes sont consommées.\n", packets);
        } break;
        case SBH_CANCEL:   agency_cancel(s->ag, hh->a); break;
        case SBH_LEV_REPRESS:
            g_sb.purge_arm=false;
            if (agency_order_repress(s->ag, hh->a))
                printf("\n[scps] MATER : la garnison marche sur la région %d (30 j) — l'agitation se taira ; le grief, lui, attendra.\n", hh->a);
            break;
        case SBH_LEV_ASSIM:
            g_sb.purge_arm=false;
            if (agency_order_assimilate(s->ag, hh->a, hh->b!=0))
                printf("\n[scps] FORMER : écoles et magistrats en région %d (1 an%s) — une source de savoir va se tarir.\n",
                       hh->a, hh->b?", Creuset":"");
            break;
        case SBH_LEV_PURGE:
            if (!g_sb.purge_arm){ g_sb.purge_arm=true; }       /* 1er clic : ARME */
            else {
                g_sb.purge_arm=false;
                if (agency_order_purge(s->ag, hh->a))
                    printf("\n[scps] LA PURGE EST PROCLAMÉE (région %d) : 4 ans de tranches — des familles périront, la Brèche se rapproche.\n", hh->a);
            }
            break;
        case SBH_LEV_EMBARGO: g_sb.purge_arm=false; g_sb.tab=SBT_ECO; g_sb.eco_sub=0; break;
        case SBH_LEV_CONTRAT:
            g_sb.purge_arm=false;
            diplo_set_vassal(s->dp, s->player, hh->b, (SuzContrat)hh->a);
            printf("\n[scps] CONTRAT : %s — %s passe sous ta couronne (le lien tiendra tant que ta force le tient).\n",
                   diplo_contrat_name((SuzContrat)hh->a), sb_country_name(world,hh->b));
            break;
        case SBH_CHIP_MODE: *mode=(ViewMode)hh->a; g_sb.lens=LENS_NONE; break;
        case SBH_CHIP_LENS: g_sb.lens=(g_sb.lens==(MapLens)hh->a)?LENS_NONE:(MapLens)hh->a; break;
        case SBH_CHIP_CUR:  g_sb.show_currents=!g_sb.show_currents; break;
        /* §terrain — LE JOUEUR BELLIGÉRANT : déclarer (CB inhérent) · négocier (arbitrage IA exposé).
         * Retour console (outillage) ; le joueur LIT l'effet sur le panneau Diplomatie + la carte
         * (statut, score, occupations). Aucune logique diplomatique nouvelle : settle/reparations. */
        case SBH_DECLARE:
            diplo_declare_war_cb(s->dp, s->player, hh->a, (CasusBelli)hh->b);
            printf("\n[scps] GUERRE déclarée à %s (motif : %s) — porte tes armées sur sa frontière.\n",
                   sb_country_name(world,hh->a), diplo_cb_name((CasusBelli)hh->b));
            break;
        case SBH_PEACE: {
            int me=s->player, en=hh->a;
            float ms=diplo_war_score(s->dp, me, en);                 /* score du point de vue du joueur */
            bool en_ensl=(en>=0&&en<SCPS_MAX_COUNTRY)? s->ts[en].unlocked[TECH_ESCLAVAGE] : false;
            bool me_ensl=s->ts[me].unlocked[TECH_ESCLAVAGE];
            if (ms >= 20.f){                                         /* le joueur DOMINE : il dicte (peut TUER l'ennemi) */
                int got=diplo_settle(s->dp, world, s->econ, s->wl, me, en, me_ensl);
                printf("\n[scps] PAIX imposée à %s : %d région(s) cédée(s).\n", sb_country_name(world,en), got);
            } else if (ms <= -20.f){                                 /* le joueur est SANS ESPOIR : capitulation */
                diplo_reparations(s->dp, world, s->econ, me, en);
                diplo_settle(s->dp, world, s->econ, s->wl, en, me, en_ensl);
                printf("\n[scps] CAPITULATION devant %s (réparations versées).\n", sb_country_name(world,en));
            } else if (s->dp->war_years[me][en] >= 2.f){             /* match nul ASSEZ saigné → paix blanche */
                diplo_settle(s->dp, world, s->econ, s->wl, me, en, false);
                printf("\n[scps] PAIX BLANCHE avec %s.\n", sb_country_name(world,en));
            } else {                                                 /* trop tôt : l'ennemi refuse */
                printf("\n[scps] %s REFUSE : la guerre n'a pas assez saigné.\n", sb_country_name(world,en));
            }
        } break;
        case SBH_COUNCIL:                                    /* Q1 : NOMMER (b≥0) ou RENVOYER (b=-1) un conseiller */
            if (hh->b>=0) statecraft_council_hire(s->sc, s->player, hh->a, hh->b);
            else          statecraft_council_dismiss(s->sc, s->player, hh->a);
            break;
        case SBH_PROD_CAP: {                                 /* limiteur de production (onglet Stocks) */
            int step=(SDL_GetModState()&KMOD_SHIFT)?100:10;
            float cur=econ_prod_cap(s->player, hh->a);
            float nxt;
            if (hh->b>0){                                    /* augmenter */
                nxt=(cur<0.f)?step:(cur+step);
            } else {                                          /* diminuer */
                if (cur<0.f) nxt=0.f;
                else { nxt=cur-step; if (nxt<0.f) nxt=-1.f; } /* sous 0 → ∞ (désactivé) */
            }
            econ_set_prod_cap(s->player, hh->a, nxt);
            printf("\n[scps] limiteur %s : %g\n", resource_name((Resource)hh->a), (double)nxt);
        } break;
        default: break;
    }
    return true;
}
/* molette sur le tiroir → défilement du panneau (sinon : zoom carte) */
static bool sidebar_wheel(int mx, int my, int wheel_y){
    (void)my;
    if (g_sb.tab<0) return false;
    int dw=(int)(SB_DRAWER_W*g_sb.anim);
    if (mx >= SB_RAIL_W+dw) return false;
    int t=g_sb.tab; g_sb.scroll[t]-= wheel_y*3;
    if (g_sb.scroll[t]<0) g_sb.scroll[t]=0;
    if (g_sb.scroll[t]>RES_COUNT) g_sb.scroll[t]=RES_COUNT;
    return true;
}

/* P2.14 — COLONISER : bouton + cibles (la région vierge sélectionnée, adjacente au
 * territoire du joueur). g_colonize_src = la région possédée la plus peuplée d'où
 * partent les colons (réutilise econ_colonize_from : 100 colons s'installent). */
#define COLONIZE_GOLD_COST 40.f
static SDL_Rect g_colonize_btn; static int g_colonize_dst=-1, g_colonize_src=-1;
#define CENTRE_RELOC_COST 250.f   /* P3.20 : relocaliser un Centre commercial (balance later) */
static SDL_Rect g_reloc_btn; static int g_reloc_dst=-1;
/* E2 §10 — bâtir un COMPTOIR ici (branche la province au réseau marchand). */
static SDL_Rect g_comptoir_btn; static int g_comptoir_reg=-1;
/* M2 — bâtir un CENTRE COMMERCIAL ici (devient un hub du réseau global). */
static SDL_Rect g_center_btn; static int g_center_reg=-1;
static int player_colonize_src(const Sim *s, int dst_reg){
    if (dst_reg<0 || dst_reg>=s->econ->n_regions) return -1;
    const RegionEconomy *d=&s->econ->region[dst_reg];
    if (d->colonized || !d->active) return -1;                 /* déjà possédée / morte */
    int best=-1; float bestpop=0.f;
    for (int r=0;r<s->econ->n_regions;r++){
        if (!s->econ->adj[r][dst_reg]) continue;
        const RegionEconomy *re=&s->econ->region[r];
        if (re->owner!=s->player || !re->colonized) continue;
        float pop=0.f; for(int c=0;c<CLASS_COUNT;c++) pop+=re->strata[c].pop;
        if (pop>bestpop){ bestpop=pop; best=r; }
    }
    return best;
}
static void draw_province_panel(SDL_Renderer *ren, int win_w, int win_h,
                                const World *w, const WorldEconomy *econ,
                                const WorldProsperity *wp, const WorldLegitimacy *wl,
                                const ModifierStack *drift, int pid, const Sim *s) {
    ProvinceReadout p = province_readout(w, econ, wp, wl, pid);
    (void)win_w;
    int pw=312, px=0, py=102, ph=win_h-py-26;   /* à GAUCHE, sous le bandeau + alertes (§7) */
    panel_bg(ren, px,py, pw,ph);
    fill_rect(ren, px+pw-2,py, 2,ph, COL_COPPER);   /* liseré cuivre sur le bord intérieur (droite) */
    int x=px+16, y=py+14, rw=pw-30;
    char line[192];
    bool restive=false;     /* une minorité frondeuse présente → chemins H / Intégrer */

    /* EN-TÊTE : place d'héraldique réservée · nom · climat·relief·taille · jauge
     * de prospérité (rouge→vert, chiffrée, calée en haut à droite). */
    int hsz=30;
    draw_box(ren, x, y+2, hsz, hsz, COL_COPPER);
    fill_rect(ren, x+1, y+3, hsz-2, hsz-2, COL_PANEL2);
    zone_add((SDL_Rect){x,y+2,hsz,hsz}, "Place réservée à l'héraldique du royaume (à venir).");
    draw_text(ren, g_font_big, x+hsz+8, y, COL_COPPER, p.nom);
    {   /* la jauge de prospérité, en haut à droite, avec son chiffre. */
        int gw=64, gh=10, gx=px+pw-16-gw, gy=y+4;
        draw_gauge(ren, gx, gy, gw, gh, p.m_aisance.value);
        char nb[8]; snprintf(nb,sizeof nb,"%d", p.m_aisance.value);
        int nbw=text_w(g_font, nb);
        draw_text(ren, g_font, gx-nbw-6, y, COL_PARCH, nb);
        zone_add((SDL_Rect){gx-nbw-8, y-2, gw+nbw+12, gh+8},
                 "Prospérité de la province (0-100) : l'aisance matérielle, du dénuement au faste — "
                 "tirée par la production, le commerce et la paix.");
    }
    snprintf(line,sizeof line, "%s · %s · %s", p.climat, p.relief, capitale_status(capitale_max_tier(p.ames)));
    draw_text(ren, g_font, x+hsz+8, y+18, COL_PARCH, line);
    zone_add((SDL_Rect){x+hsz+6, y+16, rw-hsz-6, 18},
             "Climat · relief · TAILLE (le statut vient du tier de la capitale : Hameau → Métropole).");
    y += hsz + 8;
    /* HABITANTS — un nombre, rien de plus (le détail va aux camemberts). */
    snprintf(line,sizeof line, "%ld habitants", p.ames);
    draw_text(ren, g_font, x, y, COL_PARCH, line);
    zone_add((SDL_Rect){x-2,y-2,rw,19}, "Le nombre total d'habitants de la province."); y += 22;
    { int creg=(pid>=0&&pid<w->n_provinces)?w->province[pid].region:-1;   /* P3.20 : badge Centre commercial */
      if (creg>=0 && intertrade_has_centre(creg)){
          draw_text(ren, g_font, x, y, COL_COPPER, "◆ Centre commercial");
          zone_add((SDL_Rect){x-2,y-2,rw,19}, tr(STR_CENTRE_COMMERCIAL)); y += 20;
      } }
    { int breg=(pid>=0&&pid<w->n_provinces)?w->province[pid].region:-1;   /* coques §7 : la balafre SE VOIT */
      if (breg>=0 && breg<econ->n_regions && econ->region[breg].balafre_days>0.f){
          char bal[64];
          snprintf(bal,sizeof bal,"côte balafrée — pillée il y a %d mois",
                   (int)((365.f-econ->region[breg].balafre_days)/30.f));
          draw_text(ren, g_font_small, x, y, COL_COPPER, bal);
          zone_add((SDL_Rect){x-2,y-2,rw,17}, "Des pirates ont pillé cette côte (1/10 des stocks) : production entaillée ~1 an ; la province est immunisée ~5 ans (la course ne trait pas deux fois la même vache).");
          y += 18;
      } }

    /* CAMEMBERTS — Culture + Religion côte à côte (la race SUIT la culture :
     * pas de 3ᵉ disque). Surface sobre ; le détail vit dans le survol. */
    {
        int reg = (pid>=0 && pid<w->n_provinces) ? w->province[pid].region : -1;
        if (reg>=0 && reg<econ->n_regions && econ->region[reg].pop.n_groups>0) {
            int owner = econ->region[reg].owner;
            const PopCulture *crown = &econ->region[reg].culture;     /* repli */
            if (owner>=0 && owner<w->n_countries) {
                int cp=w->country[owner].capital_prov;
                if (cp>=0 && cp<w->n_provinces) { int cr=w->province[cp].region;
                    if (cr>=0 && cr<econ->n_regions) crown=&econ->region[cr].culture; }
            }
            GroupReadout gr[SCPS_MAX_GROUPS];
            int ng = province_composition(&econ->region[reg].pop, drift, crown, 5.f, 5.f,
                                          gr, SCPS_MAX_GROUPS);
            /* parts de CULTURE : un secteur par groupe (la race le suit, en survol). */
            int cper[SCPS_MAX_GROUPS]={0}; SDL_Color ccol[SCPS_MAX_GROUPS]={{0}};
            for (int i=0;i<ng;i++){ cper[i]=gr[i].percent; ccol[i]=SLICE_PAL[i&7];
                if (i>0 && gr[i].loyaute<=HU_FRONDEUSE) restive=true; }
            /* parts de RELIGION : agrégées par confession. */
            const char *rnm[SCPS_MAX_GROUPS]={0}; int rper[SCPS_MAX_GROUPS]={0}; SDL_Color rcol[SCPS_MAX_GROUPS]={{0}}; int nr=0;
            for (int i=0;i<ng;i++){
                int f=-1; for (int j=0;j<nr;j++) if (!strcmp(rnm[j],gr[i].religion)){ f=j; break; }
                if (f<0){ rnm[nr]=gr[i].religion; rper[nr]=gr[i].percent; rcol[nr]=SLICE_PAL[nr&7]; nr++; }
                else rper[f]+=gr[i].percent;
            }
            int pr_=22, cyc=y+pr_+4, cx1=x+pr_+6, cx2=x+rw/2+pr_+2;
            draw_pie(ren, cx1, cyc, pr_, cper, ccol, ng);
            draw_pie(ren, cx2, cyc, pr_, rper, rcol, nr);
            draw_text(ren, g_font_small, cx1-pr_, cyc+pr_+3, COL_DIM, "Culture");
            draw_text(ren, g_font_small, cx2-pr_, cyc+pr_+3, COL_DIM, "Idéologie");   /* GR2 : ex-Religion */
            /* survols : les compositions détaillées (mots, pas un flottant SCPS). */
            static char chov[320], rhov[320]; int cn=0, rn2=0;
            cn += snprintf(chov+cn, sizeof chov-cn, "Culture : ");
            for (int i=0;i<ng && cn<(int)sizeof chov-48;i++)
                cn += snprintf(chov+cn, sizeof chov-cn, "%s%d%% %s (%s — %s)",
                               i?" · ":"", gr[i].percent, gr[i].culture, gr[i].race, label_humeur(gr[i].loyaute));
            rn2 += snprintf(rhov+rn2, sizeof rhov-rn2, "Idéologie · doctrine du trône : %s %s — ",
                            credo_name(crown->credo), religion_branch_name(crown->rel_branch));  /* GR2 : credo × vision CUMULÉS */
            for (int i=0;i<nr && rn2<(int)sizeof rhov-40;i++)
                rn2 += snprintf(rhov+rn2, sizeof rhov-rn2, "%s%d%% %s", i?" · ":"", rper[i], rnm[i]);
            zone_add((SDL_Rect){cx1-pr_,cyc-pr_,2*pr_+4,2*pr_+14}, chov);
            zone_add((SDL_Rect){cx2-pr_,cyc-pr_,2*pr_+4,2*pr_+14}, rhov);
            y = cyc + pr_ + 16;
        } else {
            ui_section(ren, x, &y, "PEUPLE");
            ui_row(ren,x,&y,rw,"Race", p.race, COL_PARCH,
                   "L'espèce de la population.");
        }
    }

    /* HUMEUR — une rangée de VISAGES (triste→content), le courant allumé, + le chiffre. */
    {
        ui_section(ren, x, &y, "HUMEUR");
        int nf=5, fr=9, gap=8, fy=y+fr;
        float moodv = p.m_humeur.value/100.f;
        int lit = (int)(moodv*(nf-1)+0.5f);
        for (int i=0;i<nf;i++)
            draw_face(ren, x+fr + i*(2*fr+gap), fy, fr, (float)i/(nf-1), i==lit);
        char nb[16]; snprintf(nb,sizeof nb,"%d", p.m_humeur.value);
        draw_text(ren, g_font, x + nf*(2*fr+gap) + 6, y, sense_color(moodv), nb);
        static char hh[200];
        snprintf(hh,sizeof hh,
                 "Humeur %d/100 — l'allégeance ressentie (légitimité). Agitation %d/100 : ce qui la mine "
                 "(légitimité basse, coercition, tension de diversité).", p.m_humeur.value, p.agitation.value);
        zone_add((SDL_Rect){x-2,y-2,rw,2*fr+4}, hh);
        y = fy + fr + 8;
    }

    /* POPULATION — barre EMPILÉE des classes + nom + chiffre ; le qui-fait-quoi et
     * le penchant de faction vont au SURVOL (rien de plus en surface). */
    {
        int reg = (pid>=0 && pid<w->n_provinces) ? w->province[pid].region : -1;
        if (reg>=0 && reg<econ->n_regions) {
            ui_section(ren, x, &y, "POPULATION");
            const RegionEconomy *re2=&econ->region[reg];
            /* la composition de classe ÉMERGE des groupes (§pop précise) : Σ des
             * pop_by_class de chaque groupe race×culture×foi. Repli sur les strates. */
            long cp[3] = {0,0,0};
            const ProvincePop *pp2=&re2->pop;
            if (pp2->n_groups>0){
                for (int gi=0; gi<pp2->n_groups; gi++)
                    for (int cc=0; cc<3; cc++) cp[cc]+=pp2->groups[gi].pop_by_class[cc];
            } else {
                cp[0]=(long)re2->strata[CLASS_LABORER].pop;
                cp[1]=(long)re2->strata[CLASS_BOURGEOIS].pop;
                cp[2]=(long)re2->strata[CLASS_ELITE].pop;
            }
            long tot=cp[0]+cp[1]+cp[2]; if(tot<1) tot=1;
            const SDL_Color cc[3]={ SLICE_PAL[0], SLICE_PAL[1], SLICE_PAL[3] };
            const char *chov[3]={
                "Laboureurs — aux terres et à l'armée. Penchant de faction : Communautaire.",
                "Artisans (bourgeois) — aux ateliers. Penchant de faction : Marchand.",
                "Noblesse — officiels et armée. Penchant de faction : Conquérant · Légiste." };
            /* la barre empilée */
            int bh=12, acc=0;
            for (int i=0;i<3;i++){
                int segw = (i==2) ? (rw-acc) : (int)((float)cp[i]/tot*rw);
                if (segw<0) segw=0;
                fill_rect(ren, x+acc, y, segw, bh, cc[i]);
                zone_add((SDL_Rect){x+acc,y,segw,bh}, chov[i]);
                acc += segw;
            }
            draw_box(ren, x, y, rw, bh, COL_DIM);
            y += bh+5;
            /* la légende : pastille + nom + chiffre, par classe. */
            for (int i=0;i<3;i++){
                fill_rect(ren, x, y+3, 9,9, cc[i]);
                char l[64]; snprintf(l,sizeof l, "%s %ld", labor_class_word((SocialClass)i), cp[i]);
                draw_text(ren, g_font, x+16, y, COL_PARCH, l);
                zone_add((SDL_Rect){x-2,y-2,rw,18}, chov[i]);
                y += 18;
            }
        }
    }

    /* RESSOURCES + REVENUS — la province produit, en flux JOURNALIER (+N/j). */
    IncomeReadout inc = province_income(econ, (pid>=0&&pid<w->n_provinces)?w->province[pid].region:-1);
    {
        ui_section(ren, x, &y, "RESSOURCES");
        char res[96]; int rn=0, shown=0; res[0]=0;
        for (int i=0;i<inc.n && shown<2;i++){
            if (inc.line[i].manufactured) continue;                    /* les brutes locales en tête */
            rn += snprintf(res+rn, sizeof res-rn, "%s%s", shown?" · ":"", inc.line[i].source);
            shown++;
        }
        if (shown==0) snprintf(res,sizeof res, "%s", p.ressource);      /* repli : la ressource géo */
        draw_text(ren, g_font, x, y, COL_PARCH, res);
        char rhov[160]; snprintf(rhov,sizeof rhov,
                 "Les biens extraits sur place (vocation : %s). Les quantités produites sont dans Production.", p.vocation);
        zone_add((SDL_Rect){x-2,y-2,rw,19}, rhov); y += 22;
    }
    {
        ui_section(ren, x, &y, "PRODUCTION");
        for (int i=0;i<inc.n;i++){
            char l[24]; snprintf(l,sizeof l, "+%.1f/j", inc.line[i].per_day);
            draw_text(ren, g_font, x, y, sense_color(0.62f), l);
            draw_text(ren, g_font, x+74, y, COL_DIM, inc.line[i].source);
            char hv[176]; snprintf(hv,sizeof hv,
                     "%s · %s : +%.1f unité(s)/jour. C'est l'income en RESSOURCE (en or si c'est de l'or) ; "
                     "la VENTE (→ or par le commerce) est une autre histoire.",
                     inc.line[i].manufactured?"Sortie d'atelier (bourgeois)":"Collecte (laboureurs)",
                     inc.line[i].source, inc.line[i].per_day);
            zone_add((SDL_Rect){x-2,y-2,rw,18}, hv); y += 18;
        }
        if (inc.n==0){ draw_text(ren, g_font, x, y, COL_DIM, "rien de notable"); y += 18; }
        y += 4;
    }

    /* (Le MARCHÉ — prix, entrepôt, matériaux, vivres — vit dans l'onglet Marché ;
     * pas de doublon sur le panneau régional.) */

    /* Le seuil de révolte reste signalé (gameplay) ; lignée/foi vivent dans les
     * camemberts, l'agitation dans le survol de l'humeur — surface non dense. */
    if (p.seuil_revolte) {
        snprintf(line,sizeof line, "⚑ Au bord de la révolte (agitation %d)", p.agitation.value);
        draw_text(ren, g_font, x, y, sense_color(0.06f), line);
        zone_add((SDL_Rect){x-2,y-2,rw,19},
                 "L'agitation a franchi le seuil : maintenue, elle vire à la révolte ouverte. "
                 "Lignée, foi et moteurs d'humeur : voir les camemberts et le survol des visages.");
        y += 22;
    }

    /* CAPITALE — l'ossature administrative (§capitale) : son TIER (que la pop débloque)
     * donne le statut ; les Nobles en poste (paquets de 100) délivrent logement +
     * services + productivité. Calculée de l'éco (pop + Nobles). */
    {
        int reg = (pid>=0 && pid<w->n_provinces) ? w->province[pid].region : -1;
        if (reg>=0 && reg<econ->n_regions) {
            long pop = p.ames;
            int  tier  = capitale_max_tier(pop);
            /* Nobles à l'administration : la mobilité ÉMERGENTE en promeut autant que
             * l'admin en réclame (tier·100), borné par la pop disponible (paquets de 100). */
            long admin = capitale_admin_pop(tier); if (admin > pop) admin = (pop/100)*100;
            long house = capitale_housing(tier, admin);
            long serv  = capitale_housing(tier, admin);
            int  prodp = (int)((capitale_prodmult(tier, admin)-1.f)*100.f + 0.5f);
            ui_section(ren, x, &y, "CAPITALE");
            char l[96];
            snprintf(l,sizeof l, "%s · tier %d", capitale_status(tier), tier);
            ui_row(ren,x,&y,rw,"Statut", l, COL_COPPER,
                   "L'ossature administrative. La pop DÉBLOQUE le tier, une recette de plus en plus précieuse le PAIE ; le tier nomme la taille.");
            long libres = house - pop;
            snprintf(l,sizeof l, "%ld/%ld", pop, house);   /* P6.34 : occupés/capacité compact ; la COULEUR porte l'état */
            ui_row(ren,x,&y,rw,"Logement", l, libres>=0?COL_PARCH:sense_color(0.12f),
                   "Logement délivré par la capitale (au prorata des Nobles en poste) : occupés / capacité. Surpeuplé (rouge) = grogne.");
            snprintf(l,sizeof l, "%ld", serv);
            ui_row(ren,x,&y,rw,"Services", l, serv>=pop?COL_PARCH:sense_color(0.30f),
                   "Les SERVICES délivrés ; sous-équipé, le contentement baisse.");
            snprintf(l,sizeof l, "+%d %%", prodp);
            ui_row(ren,x,&y,rw,"Productivité", l, prodp>0?sense_color(0.75f):COL_DIM,
                   "La PRODUCTIVITÉ que la capitale ajoute à la collecte (+5 % par tier servi par un paquet de Nobles).");
        }
    }

    /* MODIFICATEURS PROVINCIAUX (slot réservé, multiple) — les effets diégétiques actifs
     * ici : fléau (cicatrice de révolte) ou faveur (terre d'abondance…). Lus de la membrane
     * (mots + signe) ; la couleur porte le sens (vert = faveur, rouge = fléau). */
    if (p.n_mods > 0) {
        ui_section(ren, x, &y, tr(STR_PMOD_SECTION));
        for (int i = 0; i < p.n_mods; i++)
            ui_row(ren, x, &y, rw,
                   tr(p.mods[i].faveur ? STR_PMOD_FAVEUR : STR_PMOD_FLEAU),
                   p.mods[i].nom,
                   p.mods[i].faveur ? sense_color(0.80f) : sense_color(0.15f),
                   p.mods[i].effet);
    }

    /* BÂTIMENTS — une grille 6 + 2 : 6 emplacements ordinaires + 2 SPÉCIAUX
     * (optimisation · défense), visuellement distincts (liseré cuivre). Survol =
     * l'effet (si bâti) ou ce qu'on peut y bâtir. PAS de bouton « Bâtir » : on
     * clique un slot (vide → bâtir ; plein → améliorer/remplacer). */
    ui_section(ren, x, &y, "BÂTIMENTS");
    {
        int reg = (pid>=0 && pid<w->n_provinces) ? w->province[pid].region : -1;
        ProvBuild b; memset(&b,0,sizeof b);
        int nbld=0;
        if (reg>=0 && reg<econ->n_regions){ b=econ->region[reg].build; nbld=econ->region[reg].n_bld; }
        bool opt_built = (nbld>0);
        struct { const char *name,*abbr,*eff,*todo; float lvl; bool special; int edi; } S[8] = {
            {"Ordre","Or",  "monte la capacité du royaume (K)",       "Tribunal : ordre & administration",            b.K_inst,  false, EDI_TRIBUNAL},
            {"Vivres","Vi", "loge et nourrit la croissance",         "Grenier : nourrir & loger",                    b.food_cap,false, EDI_GRENIER},
            {"Foi","Fo",    "apaise l'agitation, soutient la légitimité","Sanctuaire : apaiser & légitimer",          b.faith,   false, EDI_SANCTUAIRE},
            {"Savoir","Sa", "accélère la recherche locale",          "Bibliothèque : hâter le savoir",               b.savoir,  false, EDI_BIBLIOTHEQUE},
            {"Marché","Ma", "capte la prospérité locale (PE)",       "Marché : capter la prospérité",                b.PE_infra,false, EDI_MARCHE},
            {"Ouvert.","Ov","perméabilité aux échanges",             "Port : perméer aux échanges",                  b.P_open,  false, EDI_PORT},
            {"Optim.","Op", p.specialisation,                        "Entrepôt : optimiser la production locale",    opt_built?1.f:0.f, true, EDI_ENTREPOT},
            {"Défense","Df",p.defense,                               "fortifier : garnison → forteresse → citadelle",b.H_coerc, true, EDI_GARNISON},
        };
        int sw=30, sg=(rw-4*sw)/3, sy0=y;
        static char bhov[8][192];
        for (int i=0;i<8;i++){
            int col=i%4, row=i/4, sx=x+col*(sw+sg), sy=sy0+row*(sw+8);
            bool built = S[i].lvl > 0.3f;
            SDL_Color fc = S[i].special ? COL_COPPER : SLICE_PAL[i%6];
            fill_round(ren, sx, sy, sw, sw, built ? fc : COL_PANEL2, 5);
            round_box (ren, sx, sy, sw, sw, COL_EDGE, 5);
            if (S[i].special){ round_box(ren, sx-1, sy-1, sw+2, sw+2, COL_COPPER, 6);  /* liseré cuivre… */
                               round_box(ren, sx-2, sy-2, sw+4, sw+4, COL_COPPER, 7); } /* …épaissi vers l'extérieur */
            int aw=text_w(g_font_small, S[i].abbr);
            draw_text(ren, g_font_small, sx+(sw-aw)/2, sy+sw/2-7, built?COL_PANEL:COL_DIM, S[i].abbr);
            /* E1bis.11 — FAMILLE ↑ : le slot offre le PALIER BÂTISSABLE (base si rien,
             * sinon le ↑ du palier en place ; « complet » au sommet). Singleton = tel quel. */
            Edifice base=(Edifice)S[i].edi;
            bool family=(base==EDI_TRIBUNAL||base==EDI_SANCTUAIRE||base==EDI_BIBLIOTHEQUE||base==EDI_GARNISON);
            Edifice tgt = family && reg>=0 ? edifice_next_buildable(econ,reg,base) : base;
            bool maxed = (tgt>=EDIFICE_COUNT);
            bool up = family && !maxed && tgt!=base;     /* un palier est déjà en place → c'est un ↑ */
            /* E2 §13 — édifice GATÉ par l'arbre : le slot se VERROUILLE tant que la
             * tech n'est pas acquise (le hover nomme la porte, pas la recette). */
            bool locked = s && !maxed && !edifice_unlocked(&s->ts[s->player], tgt);
            /* P6.33 — format STRICT : « nom ↑ (édifice) », puis les lignes de coût, rien d'autre. */
            if (locked){
                TechId gt=edifice_gate_tech(tgt);
                tr_fmt(bhov[i],sizeof bhov[i], STR_SLOT_VERROU_FMT, S[i].name,
                       gt<TECH_COUNT?tech_name(gt):"?");
                zone_add((SDL_Rect){sx,sy,sw,sw}, bhov[i]);
                continue;                                    /* pas cliquable tant que verrouillé */
            }
            if (maxed){
                snprintf(bhov[i],sizeof bhov[i], "%s — au sommet (rien à bâtir)\x1f\x1f", S[i].name);
            } else {
                const EdificeDef *ed = edifice_def(tgt);
                int hn = snprintf(bhov[i],sizeof bhov[i], "%s %s (%s)\x1f",
                                  S[i].name, up?"↑":"→", edifice_name(tgt));
                bool anyc=false;
                if (ed) for (int c=0;c<BUILD_RES_MAX && hn<(int)sizeof bhov[i]-2;c++){
                    if (ed->cost.qty[c] <= 0.f) continue;
                    hn += snprintf(bhov[i]+hn,sizeof bhov[i]-hn, "%s%-7s %.0f",
                                   anyc?"\n":"", resource_name(ed->cost.res[c]), ed->cost.qty[c]);
                    anyc=true;
                }
                if (!anyc) hn += snprintf(bhov[i]+hn,sizeof bhov[i]-hn, "—");
                snprintf(bhov[i]+hn,sizeof bhov[i]-hn, "\x1f");
            }
            zone_add((SDL_Rect){sx,sy,sw,sw}, bhov[i]);
            if (reg>=0 && !maxed) bslot_add((SDL_Rect){sx,sy,sw,sw}, reg, tgt);   /* cliquable → pose le ↑ */
        }
        y = sy0 + 2*(sw+8) + 4;
    }

    if (restive) {
        ui_section(ren, x, &y, "ACTIONS");
        draw_text(ren, g_font, x, y, sense_color(0.30f), "▸ Réprimer (la poigne)");
        zone_add((SDL_Rect){x-2,y-2,rw,19},
                 "Réprimer : calme immédiat de l'agitation — MAIS la légitimité du groupe est rongée "
                 "et la fragilité monte. Réversible : la révolte resurgit si la botte se lève (rien n'est métabolisé).");
        y += 20;
        draw_text(ren, g_font, x, y, sense_color(0.75f), "▸ Intégrer (la patience)");
        zone_add((SDL_Rect){x-2,y-2,rw,19},
                 "Intégrer : métabolise lentement (capacité + ouverture + légitimité + temps) — durable et vrai, "
                 "mais long (∝ la distance culturelle : le gouffre prend des générations).");
        y += 20;
    }
    /* P2.14 — COLONISER : région VIERGE sélectionnée, adjacente au territoire du
     * joueur → bouton (coût en or + 100 colons de la région la plus peuplée). */
    if (s && pid>=0 && pid<w->n_provinces){
        int dreg=w->province[pid].region;
        int src=player_colonize_src(s, dreg);
        if (src>=0){
            TTF_Font *fb=g_font_small?g_font_small:g_font;
            int by=py+ph-28; char cb[72];
            snprintf(cb,sizeof cb,"Coloniser  (%.0f or · 100 colons)", COLONIZE_GOLD_COST);
            int bw=text_w(fb,cb)+18;
            fill_rect(ren, x, by, bw, 22, (SDL_Color){0x16,0x1e,0x2c,0xff});
            draw_box (ren, x, by, bw, 22, COL_COPPER);
            draw_text(ren, fb, x+9, by+4, COL_COPPER, cb);
            g_colonize_btn=(SDL_Rect){x,by,bw,22}; g_colonize_dst=dreg; g_colonize_src=src;
        }
    }
    /* E2 §10 — BÂTIR UN COMPTOIR ici : région possédée, non-hub, sans comptoir,
     * tech « Comptoirs marchands » acquise. Il branche la province au réseau. */
    if (s && pid>=0 && pid<w->n_provinces){
        int dreg=w->province[pid].region;
        if (dreg>=0 && dreg<econ->n_regions && econ->region[dreg].owner==s->player
            && !intertrade_has_centre(dreg)
            && !(econ->region[dreg].edi_built & (1u<<EDI_COMPTOIR))
            && edifice_unlocked(&s->ts[s->player], EDI_COMPTOIR)){
            TTF_Font *fb=g_font_small?g_font_small:g_font;
            int by=py+ph-54; char c0[16], cb[96];
            snprintf(c0,sizeof c0,"%.0f", agency_build_gold(econ,dreg,EDI_COMPTOIR));
            tr_fmt(cb,sizeof cb, STR_BTN_COMPTOIR_FMT, c0);
            int bw=text_w(fb,cb)+18;
            fill_rect(ren, x, by, bw, 22, (SDL_Color){0x16,0x1e,0x2c,0xff});
            draw_box (ren, x, by, bw, 22, COL_COPPER);
            draw_text(ren, fb, x+9, by+4, COL_COPPER, cb);
            zone_add((SDL_Rect){x,by,bw,22}, tr(STR_COMPTOIR_HOV));
            g_comptoir_btn=(SDL_Rect){x,by,bw,22}; g_comptoir_reg=dreg;
        }
    }
    /* M2 — BÂTIR UN CENTRE COMMERCIAL ici : région possédée, CÔTIÈRE/ESTUAIRE, à vocation
     * MARCHANDE, sans Centre encore → l'empire se fait hub du réseau GLOBAL (causalité). */
    if (s && pid>=0 && pid<w->n_provinces){
        int dreg=w->province[pid].region;
        if (dreg>=0 && dreg<econ->n_regions && econ->region[dreg].owner==s->player
            && !intertrade_has_centre(dreg)
            && (econ->region[dreg].coastal || econ->region[dreg].estuary)
            && econ->region[dreg].culture.ethos==ETHOS_MERCANTILE){
            TTF_Font *fb=g_font_small?g_font_small:g_font;
            int by=py+ph-80; char c0[16], cb[96];
            snprintf(c0,sizeof c0,"%.0f", agency_build_gold(econ,dreg,EDI_TRADE_CENTER));
            tr_fmt(cb,sizeof cb, STR_BTN_CENTER_FMT, c0);
            int bw=text_w(fb,cb)+18;
            fill_rect(ren, x, by, bw, 22, (SDL_Color){0x16,0x1e,0x2c,0xff});
            draw_box (ren, x, by, bw, 22, COL_COPPER);
            draw_text(ren, fb, x+9, by+4, COL_COPPER, cb);
            zone_add((SDL_Rect){x,by,bw,22}, tr(STR_CENTER_HOV));
            g_center_btn=(SDL_Rect){x,by,bw,22}; g_center_reg=dreg;
        }
    }
    /* P3.20 — RELOCALISER un Centre commercial vers CETTE région (possédée, non-hub),
     * si le joueur en tient un ailleurs : le hub se DÉPLACE (il ne meurt jamais). */
    if (s && pid>=0 && pid<w->n_provinces){
        int dreg=w->province[pid].region;
        if (dreg>=0 && dreg<econ->n_regions && econ->region[dreg].owner==s->player
            && !intertrade_has_centre(dreg) && intertrade_country_centre(econ, s->player)>=0){
            TTF_Font *fb=g_font_small?g_font_small:g_font;
            int by=py+ph-28; char cb[80];
            snprintf(cb,sizeof cb,"Relocaliser un Centre commercial ici  (%.0f or)", CENTRE_RELOC_COST);
            int bw=text_w(fb,cb)+18;
            fill_rect(ren, x, by, bw, 22, (SDL_Color){0x16,0x1e,0x2c,0xff});
            draw_box (ren, x, by, bw, 22, COL_COPPER);
            draw_text(ren, fb, x+9, by+4, COL_COPPER, cb);
            g_reloc_btn=(SDL_Rect){x,by,bw,22}; g_reloc_dst=dreg;
        }
    }
}

/* ===================================================================== */
/* L'OUTLINER — « ce que je possède », groupé par URBANISATION (§6)        */
/* ===================================================================== */
/* L'urbanisation vient du TIER de la capitale (§capitale) : on groupe l'outliner
 * par tier (statut), pas par terrain ni par pop brute. */
static long region_pop_of(const RegionEconomy *re){
    return (long)(re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop + re->strata[CLASS_ELITE].pop);
}
/* Un emplacement institutionnel encore LIBRE (un axe ProvBuild vide) ? Avec un
 * bâtiment de base toujours posable, c'est la condition du MARTEAU (§6b). */
static bool region_free_slot(const RegionEconomy *re){
    const ProvBuild *b=&re->build;
    return b->K_inst<0.3f||b->food_cap<0.3f||b->faith<0.3f||b->savoir<0.3f||b->PE_infra<0.3f||b->P_open<0.3f;
}
static char g_ohov[80][200];
static bool g_right_hidden = false;          /* P0.4 : Domaine (sidebar droite) replié (mémorisé en session) */
static SDL_Rect g_right_chevron;             /* P0.4 : bouton de repli (cliquable) */
static int  g_diplo_target = -1;             /* P0.3 : pays cible du panneau diplo (right-clic), -1 = fermé */
static void draw_outliner(SDL_Renderer *ren, int win_w, int win_h, const Sim *s, const World *w, int selected){
    int player=s->player; if (player<0) return;
    int selreg=(selected>=0 && selected<w->n_provinces)? w->province[selected].region : -1;  /* P1.11 */
    /* P0.4 — chevron de repli, TOUJOURS présent (coin haut-droit) : « » replie, « replie « déplie. */
    g_right_chevron = (SDL_Rect){win_w-22, 86, 20, 22};
    fill_rect(ren, g_right_chevron.x, g_right_chevron.y, 20,22, COL_PANEL2);
    draw_box (ren, g_right_chevron.x, g_right_chevron.y, 20,22, COL_COPPER);
    draw_text(ren, g_font, g_right_chevron.x+5, g_right_chevron.y+2, COL_COPPER, g_right_hidden?"\xC2\xAB":"\xC2\xBB");
    if (g_right_hidden) return;               /* replié : on n'affiche que le chevron */
    int pw=234, px=win_w-pw, py=102, ph=win_h-py-26;
    panel_bg(ren, px,py, pw,ph);
    fill_rect(ren, px,py, 2,ph, COL_COPPER);          /* liseré cuivre sur le bord intérieur (gauche) */
    TTF_Font *fs = g_font_small?g_font_small:g_font;
    int x=px+12, y=py+10, rw=pw-22, bottom=py+ph-4, oi=0;
    draw_text(ren, g_font, x, y, COL_COPPER, "Domaine"); y+=20;
    /* régions groupées par URBANISATION = le TIER de la capitale (grand → petit). */
    for (int tier=7; tier>=1 && y<bottom-16; tier--){
        int cnt=0;
        for (int r=0;r<s->econ->n_regions;r++){
            const RegionEconomy *re=&s->econ->region[r];
            if (re->owner!=player) continue;
            if (capitale_max_tier(region_pop_of(re))==tier) cnt++;
        }
        if (!cnt) continue;
        const char *st=capitale_status(tier); size_t sl=strlen(st);   /* pluriel FR : -eau → -eaux */
        const char *plur=(sl>=3 && !strcmp(st+sl-3,"eau"))?"x":"s";
        char gh[44]; snprintf(gh,sizeof gh,"%s%s (%d)", st, plur, cnt);
        draw_text(ren, fs, x, y, COL_DIM, gh); y+=15;
        for (int r=0;r<s->econ->n_regions && y<bottom-15;r++){
            const RegionEconomy *re=&s->econ->region[r];
            if (re->owner!=player) continue;
            long pop=region_pop_of(re);
            if (capitale_max_tier(pop)!=tier) continue;
            if (r==selreg) fill_rect(ren, x-2, y-1, rw+2, 15, (SDL_Color){0x3a,0x2c,0x1a,0xff});  /* P1.11 : ligne sélectionnée */
            int pid=(r<w->n_regions && w->region[r].n_provinces>0)? w->region[r].province_ids[0]:-1;
            Resource res=(pid>=0 && pid<w->n_provinces)? w->province[pid].resource:RES_NONE;
            fill_rect(ren, x+2, y+2, 9,9, SLICE_PAL[((int)res)&7]); draw_box(ren,x+2,y+2,9,9,COL_DIM);  /* icône-ressource */
            const char *nm=(r<w->n_regions && w->region[r].name[0])? w->region[r].name:"—";
            draw_text(ren, fs, x+16, y, COL_PARCH, nm);
            char pz[24]; snprintf(pz,sizeof pz,"%ld",pop); int pzw=text_w(fs,pz);
            draw_text(ren, fs, x+rw-pzw, y, COL_DIM, pz);                 /* population : la stat-clé */
            bool ham=region_free_slot(re); SDL_Rect hr={0,0,0,0};
            if (ham){ hr=(SDL_Rect){x+rw-pzw-15, y, 11,12};               /* le MARTEAU (slot libre × tech) */
                fill_rect(ren,hr.x,hr.y+1,10,10,COL_COPPER); draw_box(ren,hr.x,hr.y+1,10,10,COL_DIM);
                draw_text(ren, fs, hr.x+2, hr.y, COL_PANEL, "+"); }
            if (oi<80){                                                   /* survol : détail rapide */
                IncomeReadout inc=province_income(s->econ,r); int n=0;
                n+=snprintf(g_ohov[oi]+n,(size_t)(sizeof g_ohov[oi])-n,"%s — Laboureurs %ld · Artisans %ld · Nobles %ld",
                            nm,(long)re->strata[0].pop,(long)re->strata[1].pop,(long)re->strata[2].pop);
                if (inc.n>0) n+=snprintf(g_ohov[oi]+n,(size_t)(sizeof g_ohov[oi])-n," · produit +%.1f %s/j",inc.line[0].per_day,inc.line[0].source);
                n+=snprintf(g_ohov[oi]+n,(size_t)(sizeof g_ohov[oi])-n, ham?" · slot libre (marteau : bâtir)":" · complet");
                int occ=s->dp->occupier[r];   /* §terrain : une de NOS régions TENUE par l'ennemi (à libérer) */
                if (occ>=0 && occ!=player && occ<w->n_countries){
                    char occn[80]; tr_fmt(occn,sizeof occn,STR_OCCUPEE_PAR,w->country[occ].name);
                    n+=snprintf(g_ohov[oi]+n,(size_t)(sizeof g_ohov[oi])-n," · %s",occn);
                }
                zone_add((SDL_Rect){x,y-1,rw,14}, g_ohov[oi]);
                orow_add((SDL_Rect){x,y-1,rw-pzw-17,14}, hr, pid, ham?r:-1);
                oi++;
            }
            y+=15;
        }
        y+=3;
    }
    /* ARMÉES — l'effectif en HOMMES (×100) ; survol = composition. */
    if (s->camp && campaign_active(s->camp,player) && y<bottom-15 && oi<80){
        draw_text(ren, fs, x, y, COL_DIM, "Armées (1)"); y+=15;
        long men=campaign_units(s->camp,player)*100;
        fill_rect(ren, x+2,y+2,9,9, COL_COPPER); draw_box(ren,x+2,y+2,9,9,COL_DIM);
        draw_text(ren, fs, x+16, y, COL_PARCH, "Armée de campagne");
        char pz[24]; snprintf(pz,sizeof pz,"%ld",men); int pzw=text_w(fs,pz);
        draw_text(ren, fs, x+rw-pzw, y, COL_DIM, pz);
        ArmyComposition cp=campaign_composition(s->camp,player);
        snprintf(g_ohov[oi],sizeof g_ohov[oi],"Armée de campagne — %ld hommes : inf %ld · arch %ld · cav %ld · %s",
                 men, cp.infanterie*100, cp.archers*100, cp.cavalerie*100, campaign_phase_name(campaign_phase(s->camp,player)));
        zone_add((SDL_Rect){x,y-1,rw,14}, g_ohov[oi]);
        int loc=campaign_location(s->camp,player);
        int lp=(loc>=0&&loc<w->n_regions&&w->region[loc].n_provinces>0)?w->region[loc].province_ids[0]:-1;
        orow_add((SDL_Rect){x,y-1,rw,14}, (SDL_Rect){0,0,0,0}, lp, -1);
        y+=15;
        /* « REMPLIR » : recompléter l'armée en TERRITOIRE AMI (note). */
        if (campaign_can_refill(s->camp, s->econ, player) && y<bottom-16){
            long rm, mat; campaign_refill_cost(s->camp, player, &rm, &mat);
            int bw=text_w(fs,"Remplir")+16;
            fill_round(ren, x+16, y, bw, 15, COL_PANEL2, 4);
            round_box (ren, x+16, y, bw, 15, COL_COPPER, 4);
            draw_text (ren, fs, x+24, y+1, COL_COPPER, "Remplir");
            static char rh[180];
            snprintf(rh,sizeof rh,
                     "Remplir l'armée (en territoire ami) : +%ld hommes levés · %ld matériaux pour les armes "
                     "(achetés au marché, en or si le stock manque).", rm, mat);
            zone_add((SDL_Rect){x+16,y,bw,15}, rh);
            g_refill_btn=(SDL_Rect){x+16,y,bw,15}; g_refill_owner=player;
            y+=17;
        }
    }
}

/* §5 — la MINICARTE (coin bas-droit) : le monde en petit + le cadre de vue. */
#define MM_W 212
#define MM_H 100
static void minimap_fit(float *scale, float *ox, float *oy){
    float sx=(float)MM_W/SCPS_W, sy=(float)MM_H/SCPS_H;
    float sc = sx<sy?sx:sy;
    *scale=sc; *ox=(SCPS_W - MM_W/sc)*0.5f; *oy=(SCPS_H - MM_H/sc)*0.5f;
}
static void minimap_rect(int win_w, int win_h, SDL_Rect *r){
    r->x=win_w-MM_W-12; r->y=win_h-MM_H-36; r->w=MM_W; r->h=MM_H;
}
/* P0.2 — PRIORITÉ D'ENTRÉE : panneau > carte. Le curseur est-il au-dessus d'un
 * panneau persistant ? (topbar · rail+tiroir gauche · Domaine droit · panneau de
 * province · minicarte · boutons de mode). Si oui : aucun hover ni clic CARTE.
 * g_right_hidden replie le Domaine (P0.4, déclaré plus haut). */
static bool over_panel(int mx, int my, int win_w, int win_h, int selected){
    if (my < 78) return true;                                              /* topbar */
    int drawer = (g_sb.tab>=0)? (int)(SB_DRAWER_W*g_sb.anim) : 0;
    if (mx < SB_RAIL_W + drawer) return true;                             /* rail + tiroir gauche */
    if (!g_right_hidden && mx >= win_w-234 && my >= 102 && my < win_h-26) return true;  /* Domaine (droite) */
    if (mx >= win_w-22 && my >= 86 && my < 110) return true;             /* chevron du Domaine */
    if (selected>=0 && mx < 312 && my >= 102 && my < win_h-26) return true;             /* panneau province (gauche) */
    if (mx >= win_w-MM_W-12 && my >= win_h-MM_H-36) return true;         /* minicarte (coin bas-droit) */
    if (my >= win_h-62 && my < win_h-30 && mx >= 318 && mx < 640) return true;          /* boutons de mode (bas) */
    if (g_diplo_target>=0 && mx>=win_w-272 && my>=86) return true;                       /* popup diplo (P0.3) */
    return false;
}
/* P0.3 — PANNEAU DIPLOMATIQUE rapide (clic-droit sur une région étrangère) : le
 * SQUELETTE — nom · race · statut · menace (+ score si en guerre). Les ACTIONS
 * viendront avec l'arc guerre (emplacement réservé) ; ici, lecture seule. */
static void draw_diplo_popup(SDL_Renderer *ren, int win_w, const World *w, const Sim *s){
    int cid=g_diplo_target; if (cid<0||cid>=w->n_countries) return;
    int pw=260, ph=170, px=win_w-pw-12, py=92;
    panel_bg(ren, px,py, pw,ph);
    fill_rect(ren, px,py, pw,3, COL_COPPER);
    TTF_Font *fs=g_font_small?g_font_small:g_font;
    int x=px+14, y=py+12; char buf[96];
    draw_text(ren, g_font, x, y, COL_COPPER, sb_country_name(w,cid)); y+=24;
    int cr=(w->country[cid].capital_prov>=0)? w->province[w->country[cid].capital_prov].region : -1;
    const char *race=(cr>=0 && cr<s->econ->n_regions)? species_name(s->econ->region[cr].culture.race) : "—";
    tr_fmt(buf,sizeof buf,STR_DIPLO_RACE_FMT,race);                draw_text(ren,fs,x,y,COL_PARCH,buf); y+=17;
    DiploStatus st=diplo_status(s->dp,s->player,cid);
    const char *sw=(st==DIPLO_WAR)?tr(STR_DIPLO_GUERRE):(st==DIPLO_ALLIED)?tr(STR_DIPLO_ALLIE):
                   (diplo_suzerain(s->dp,cid)==s->player)?tr(STR_DIPLO_VASSAL):
                   (diplo_suzerain(s->dp,s->player)==cid)?tr(STR_DIPLO_SUZERAIN):tr(STR_DIPLO_NEUTRE);
    tr_fmt(buf,sizeof buf,STR_DIPLO_STATUT_FMT,sw);               draw_text(ren,fs,x,y,COL_PARCH,buf); y+=17;
    Relation rel=diplo_relation(w,s->econ,s->wp,s->dp,s->player,cid);
    float th=rel.threat; if(th<0.f)th=0.f; if(th>1.f)th=1.f;
    char nb[16]; snprintf(nb,sizeof nb,"%.0f",th*100.f);
    tr_fmt(buf,sizeof buf,STR_DIPLO_MENACE_FMT,nb);               draw_text(ren,fs,x,y,COL_DIM,buf); y+=17;
    if (st==DIPLO_WAR){ char ns[16]; snprintf(ns,sizeof ns,"%+d",(int)diplo_war_score(s->dp,s->player,cid));
        tr_fmt(buf,sizeof buf,STR_DIPLO_SCORE_FMT,ns);            draw_text(ren,fs,x,y,COL_DIM,buf); y+=17;
        /* P-bis — la PAIX SE DÉCLARE : score ≥ 50 (décisive : on encaisse l'occupé) OU 10 ans
         * (paix blanche). Le joueur voit où en est la guerre. */
        char sc2[16], yr2[16];
        snprintf(sc2,sizeof sc2,"%d",(int)diplo_war_score(s->dp,s->player,cid));
        snprintf(yr2,sizeof yr2,"%.0f",s->dp->war_years[s->player][cid]);
        tr_fmt(buf,sizeof buf,STR_DIPLO_PAIX_FMT,sc2,yr2);        draw_text(ren,fs,x,y,COL_COPPER,buf); y+=17; }
    draw_text(ren,fs,x,py+ph-22,COL_PANEL2,tr(STR_DIPLO_ACTIONS));
}
static void draw_minimap(SDL_Renderer *ren, PixBuf *mm, int win_w, int win_h, const Cam *cam){
    SDL_Rect m; minimap_rect(win_w,win_h,&m);
    panel_bg(ren, m.x-5, m.y-5, MM_W+10, MM_H+10);
    if (mm->tex){ SDL_RenderCopy(ren, mm->tex, NULL, &m); }
    round_box(ren, m.x-1,m.y-1,MM_W+2,MM_H+2, COL_COPPER, 3);
    /* le cadre de VUE (où regarde la caméra). */
    float sc,ox,oy; minimap_fit(&sc,&ox,&oy);
    int vx=m.x+(int)((cam->ox-ox)*sc), vy=m.y+(int)((cam->oy-oy)*sc);
    int vw=(int)((win_w/cam->scale)*sc), vh=(int)((win_h/cam->scale)*sc);
    if (vx<m.x){ vw-=(m.x-vx); vx=m.x; } if (vy<m.y){ vh-=(m.y-vy); vy=m.y; }
    if (vx+vw>m.x+MM_W) vw=m.x+MM_W-vx;
    if (vy+vh>m.y+MM_H) vh=m.y+MM_H-vy;
    if (vw>2 && vh>2) draw_box(ren, vx, vy, vw, vh, (SDL_Color){0xff,0xf4,0xe0,0xff});
}

/* §5 — BOUTONS de mode de carte (Politique · Culture · Foi · Relief), le COURANT
 * en cuivre. Posés en bas (zone carte), testés au clic. */
static void draw_mode_buttons(SDL_Renderer *ren, int win_h, ViewMode cur){
    struct { const char *name; ViewMode m; } B[4] = {
        {"Politique", VIEW_COUNTRIES}, {"Culture", VIEW_CULTURE},
        {"Foi", VIEW_FAITH}, {"Relief", VIEW_TERRAIN} };
    TTF_Font *fs=g_font_small?g_font_small:g_font;
    int x=322, y=win_h-26-32, h=24;
    for (int i=0;i<4;i++){
        int tw=text_w(fs,B[i].name), w=tw+18;
        bool on=(cur==B[i].m);
        fill_round(ren, x, y, w, h, on?COL_PANEL2:COL_PANEL, 6);
        round_box (ren, x, y, w, h, on?COL_COPPER:COL_EDGE, 6);
        if (on) round_box(ren, x-1,y-1,w+2,h+2, COL_COPPER, 7);   /* le courant : rim cuivre */
        draw_text(ren, fs, x+9, y+5, on?COL_COPPER:COL_DIM, B[i].name);
        modebtn_add((SDL_Rect){x,y,w,h}, (int)B[i].m);
        x += w+7;
    }
}

/* P6.33 — découpe d'une recette MULTILIGNE pour la boîte de survol : copie la ligne
 * courante (jusqu'à '\n' ou la fin) dans `out` (borné), renvoie le début de la
 * suivante (ou NULL). Évite toute ambiguïté d'indentation dans les boucles. */
static const char *hov_next_line(const char *ls, char *out, int cap){
    const char *nl=strchr(ls,'\n');
    int len = nl ? (int)(nl-ls) : (int)strlen(ls);
    if (len>cap-1) len=cap-1;
    memcpy(out,ls,(size_t)len); out[len]='\0';
    return nl ? nl+1 : NULL;
}

/* Survol = définition : le hover de la zone sous le curseur, en pied d'écran. */
static void draw_hover_footer(SDL_Renderer *ren, int win_w, int win_h, int mx, int my){
    const char *def = zone_hit(mx,my);
    if (!def) return;
    const char *s1 = strchr(def, '\x1f');
    if (!s1){                                                   /* survol SIMPLE : bandeau en bas */
        int fh=22;
        fill_rect(ren, 0, win_h-fh, win_w, fh, COL_PANEL2);
        fill_rect(ren, 0, win_h-fh-1, win_w, 1, COL_COPPER);
        draw_text(ren, g_font, 12, win_h-fh+3, COL_PARCH, def);
        return;
    }
    /* survol DEUX COLONNES : « titre \x1f gauche(prix) \x1f droite(effet) » → une boîte près
     * du curseur (comme l'exemple : nom en titre, input/prix à gauche, effet à droite). */
    char title[120]={0}, left[120]={0}, right[160]={0};
    int n1=(int)(s1-def); if(n1>119)n1=119; memcpy(title,def,(size_t)n1);
    const char *s2=strchr(s1+1,'\x1f');
    if (s2){
        int nl=(int)(s2-(s1+1)); if(nl>119)nl=119; memcpy(left,s1+1,(size_t)nl);
        snprintf(right,sizeof right,"%s",s2+1);
    } else {
        snprintf(left,sizeof left,"%s",s1+1);
    }
    /* le PRIX (gauche) peut être MULTILIGNE : une ligne ressource/quantité par '\n'
     * (P6.33 — recette de bâtiment). On mesure la plus large, on empile, la boîte suit. */
    int wt=text_w(g_font_big,title), wr=text_w(g_font,right);
    int wl=0, nlines=0;
    { const char *ls=left; char line[120];
      while (ls && *ls){ const char *next=hov_next_line(ls,line,120);
          int wln=text_w(g_font,line);
          if (wln>wl) wl=wln;
          nlines++; ls=next; } }
    if (nlines<1) nlines=1;
    int gap=46, inner=wl+(right[0]?gap+wr:0); if (wt>inner) inner=wt;
    int rowh=18, bw=inner+24, bh=33+nlines*rowh+5, bx=mx+16, by=my+10;
    if (bx+bw>win_w-4) bx=win_w-bw-4;
    if (bx<4) bx=4;
    if (by+bh>win_h-4) by=win_h-bh-4;
    if (by<4) by=4;
    panel_bg(ren, bx,by, bw,bh);
    draw_text(ren, g_font_big, bx+12, by+6,  COL_COPPER, title);
    fill_rect(ren, bx+10, by+27, bw-20, 1, COL_PANEL2);         /* la ligne de séparation */
    { const char *ls=left; int ry=by+33; char line[120];        /* PRIX à gauche (empilé) */
      while (ls && *ls){ const char *next=hov_next_line(ls,line,120);
          draw_text(ren, g_font, bx+12, ry, COL_PARCH, line);
          ry+=rowh; ls=next; } }
    if (right[0]) draw_text(ren, g_font, bx+bw-12-wr, by+33,
                            (SDL_Color){0x8c,0xd0,0x9c,0xff}, right);          /* EFFET à droite (vert doux) */
}

/* Le PEUPLE dominant d'un empire (via la membrane : groupe majoritaire de sa
 * capitale). Mot diégétique, jamais un nom SCPS. */
static const char *army_people(const World *w, const WorldEconomy *econ,
                               const ModifierStack *drift, int cid){
    if (cid<0 || cid>=w->n_countries) return "—";
    int cp=w->country[cid].capital_prov; if (cp<0 || cp>=w->n_provinces) return "—";
    int reg=w->province[cp].region;      if (reg<0 || reg>=econ->n_regions) return "—";
    if (econ->region[reg].pop.n_groups<=0) return "—";
    GroupReadout gr[SCPS_MAX_GROUPS];
    int ng=province_composition(&econ->region[reg].pop, drift, &econ->region[reg].culture,
                                5.f, 5.f, gr, SCPS_MAX_GROUPS);
    return (ng>0) ? gr[0].race : "—";
}

/* Centre-écran d'une région = barycentre des graines de ses PROVINCES (coords de
 * grille géographiques ; Region.seed_x/y n'est pas peuplé). */
static bool region_screen_pos(const World *w, const Cam *cam, int reg, int *osx, int *osy){
    if (reg<0 || reg>=w->n_regions) return false;
    const Region *R=&w->region[reg];
    long ax=0, ay=0; int n=0;
    for (int k=0;k<R->n_provinces && k<12;k++){
        int pid=R->province_ids[k];
        if (pid<0 || pid>=w->n_provinces) continue;
        ax += w->province[pid].seed_x; ay += w->province[pid].seed_y; n++;
    }
    if (n==0) return false;
    float _px,_py; cam_project(cam,(float)ax/n,(float)ay/n,&_px,&_py);
    *osx=(int)_px; *osy=(int)_py;
    return true;
}

/* §4 — LE MARQUEUR D'ARMÉE : une armée de campagne posée sur la carte, en COULEUR
 * D'EMPIRE, dimensionnée par l'effectif ; le survol détaille (peuple, inf/arch/cav,
 * phase). Pour une armée ENNEMIE : ASYMÉTRIE d'information — on montre la TAILLE
 * (un mot), pas le décompte. Membrane : ne lit que des nombres tangibles (paquets),
 * des mots et la GÉOGRAPHIE (position de région) ; aucun flottant SCPS. */
static char g_army_tip[SCPS_MAX_COUNTRY][192];
static void draw_army_markers(SDL_Renderer *ren, const Cam *cam, const Sim *s,
                              const World *w, int win_w, int win_h){
    if (!s->camp) return;
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++){
        if (!campaign_active(s->camp, c)) continue;
        int reg = campaign_location(s->camp, c);
        int sx, sy;
        if (!region_screen_pos(w, cam, reg, &sx, &sy)) continue;
        if (sx < -24 || sy < -24 || sx > win_w+24 || sy > win_h+24) continue;   /* hors champ */
        long paquets   = campaign_units(s->camp, c);
        FieldPhase ph  = campaign_phase(s->camp, c);
        bool mine      = (c == s->player);
        uint32_t col   = w->country[c].color;
        SDL_Color ec = { (uint8_t)((col>>16)&0xFF), (uint8_t)((col>>8)&0xFF), (uint8_t)(col&0xFF), 0xFF };
        SDL_Color dk = { 0x10,0x12,0x16,0xFF };

        /* trait de marche : une fine ligne vers la région-but (ses propres armées). */
        if (mine && ph==FA_MARCH){
            int dx, dy;
            if (region_screen_pos(w, cam, s->camp->army[c].dest, &dx, &dy)){
                SDL_SetRenderDrawColor(ren, ec.r, ec.g, ec.b, 0x99);
                SDL_RenderDrawLine(ren, sx, sy, dx, dy);
            }
        }
        /* jeton : carré en couleur d'empire (taille ∝ effectif, 5..11), liseré clair. */
        int rr = 5 + (int)(paquets/18); if (rr>11) rr=11;
        fill_rect(ren, sx-rr, sy-rr, 2*rr, 2*rr, ec);
        draw_box (ren, sx-rr, sy-rr, 2*rr, 2*rr, dk);
        draw_box (ren, sx-rr-1, sy-rr-1, 2*rr+2, 2*rr+2, COL_PARCH);
        if (ph==FA_SIEGE) draw_ring(ren, sx, sy, (float)(rr+3), COL_COPPER);   /* halo de siège */

        /* P1.10 — effectif EXACT sur TOUTE armée visible (amie comme ennemie) : le
         * nombre d'hommes, jamais un mot de brouillard. */
        char lab[24];
        snprintf(lab, sizeof lab, "%ld", paquets*100);
        if (g_font_small){
            int lw=text_w(g_font_small, lab);
            SDL_Color labbg = { 0x0f,0x16,0x22,0xcc };
            fill_rect(ren, sx-lw/2-2, sy+rr+1, lw+4, 13, labbg);
            draw_text(ren, g_font_small, sx-lw/2, sy+rr+1, COL_PARCH, lab);
        }

        /* survol : effectif exact + composition (P1.10 — plus de mot de taille). */
        const char *people = army_people(w, s->econ, s->drift, c);
        ArmyComposition cp = campaign_composition(s->camp, c);
        if (mine)
            snprintf(g_army_tip[c], sizeof g_army_tip[c],
                     "%s (%s) — %ld hommes : inf %ld · arch %ld · cav %ld%s · %s",
                     w->country[c].name, people, paquets*100,
                     cp.infanterie*100, cp.archers*100, cp.cavalerie*100,
                     cp.mages? " · mages":"", campaign_phase_name(ph));
        else
            snprintf(g_army_tip[c], sizeof g_army_tip[c],
                     "%s (%s) — %ld hommes · %s",
                     w->country[c].name, people, paquets*100, campaign_phase_name(ph));
        zone_add((SDL_Rect){sx-rr-2, sy-rr-2, 2*rr+4, 2*rr+16}, g_army_tip[c]);
    }
}

/* P3.20 — LES CENTRES COMMERCIAUX sur la carte (NOTIFIER le carrefour) : un losange
 * cuivré au carrefour, « ◆ Marché de <lieu> » ; le liseré dit qui le TIENT (couleur
 * d'empire) — un point stratégique à voir et à disputer. Survol = le rôle + l'accès. */
static void draw_centre_markers(SDL_Renderer *ren, const Cam *cam, const Sim *s,
                                const World *w, int win_w, int win_h){
    static char tip[SCPS_MAX_REG][128];
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++){
        if (!intertrade_has_centre(r)) continue;
        int sx, sy;
        if (!region_screen_pos(w, cam, r, &sx, &sy)) continue;
        if (sx<-20||sy<-20||sx>win_w+20||sy>win_h+20) continue;
        int owner=s->econ->region[r].owner;
        SDL_Color oc = (owner>=0 && owner<w->n_countries)
            ? (SDL_Color){(uint8_t)((w->country[owner].color>>16)&0xFF),
                          (uint8_t)((w->country[owner].color>>8)&0xFF),
                          (uint8_t)(w->country[owner].color&0xFF),0xFF}
            : (SDL_Color){0x70,0x70,0x70,0xFF};
        /* le losange : carré tourné 45° (4 triangles autour du centre) — cuivre + liseré d'empire. */
        int rad=6;
        for (int dy=-rad;dy<=rad;dy++){ int wdt=rad-(dy<0?-dy:dy);
            SDL_SetRenderDrawColor(ren,COL_COPPER.r,COL_COPPER.g,COL_COPPER.b,0xFF);
            SDL_RenderDrawLine(ren, sx-wdt, sy+dy, sx+wdt, sy+dy); }
        SDL_SetRenderDrawColor(ren,oc.r,oc.g,oc.b,0xFF);                 /* liseré : qui le tient */
        SDL_RenderDrawLine(ren,sx,sy-rad-2, sx+rad+2,sy); SDL_RenderDrawLine(ren,sx+rad+2,sy, sx,sy+rad+2);
        SDL_RenderDrawLine(ren,sx,sy+rad+2, sx-rad-2,sy); SDL_RenderDrawLine(ren,sx-rad-2,sy, sx,sy-rad-2);
        char nm[32]; region_make_name(nm,sizeof nm,s->econ,r);
        if (g_font_small && cam->scale>1.6f){                            /* nom au zoom (sinon trop dense) */
            char lab[48]; snprintf(lab,sizeof lab,"\xe2\x97\x86 Marché de %s", nm);
            int lw=text_w(g_font_small,lab);
            fill_rect(ren, sx-lw/2-2, sy+rad+2, lw+4, 13, (SDL_Color){0x0f,0x16,0x22,0xcc});
            draw_text(ren, g_font_small, sx-lw/2, sy+rad+2, COL_COPPER, lab);
        }
        snprintf(tip[r],sizeof tip[r],"Marché de %s\x1f%s\x1f" "Centre commercial : le tenir = commercer.",
                 nm, (owner>=0&&owner<w->n_countries)?w->country[owner].name:"sans maitre");
        zone_add((SDL_Rect){sx-rad-2,sy-rad-2,2*rad+4,2*rad+4}, tip[r]);
    }
}

/* P1.8 — ÉTIQUETTE de NOM D'EMPIRE au DÉZOOM, centrée sur le territoire (centroïde
 * des régions possédées), en FONDU selon le zoom (pleine au dézoom, s'efface en
 * zoomant pour ne pas encombrer). Le nom est procédural (P1.9). */
static void draw_empire_labels(SDL_Renderer *ren, const Cam *cam, const Sim *s,
                               const World *w, int win_w, int win_h){
    if (!g_font_small) return;
    float fade=(6.5f - cam->scale)/3.5f; if (fade<=0.f) return; if (fade>1.f) fade=1.f;
    Uint8 a=(Uint8)(fade*235.f);
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++){
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        long ax=0, ay=0; int n=0;                            /* centroïde des régions POSSÉDÉES */
        for (int r=0;r<s->econ->n_regions;r++){
            if (s->econ->region[r].owner!=c) continue;
            const Region *R=&w->region[r];
            for (int k=0;k<R->n_provinces && k<12;k++){
                int pid=R->province_ids[k]; if (pid<0||pid>=w->n_provinces) continue;
                ax+=w->province[pid].seed_x; ay+=w->province[pid].seed_y; n++;
            }
        }
        if (n<2) continue;
        float _px,_py; cam_project(cam,(float)ax/n,(float)ay/n,&_px,&_py);
        int sx=(int)_px, sy=(int)_py;
        if (sx<60||sy<86||sx>win_w-60||sy>win_h-70) continue;   /* hors champ / sous les panneaux */
        const char *nm=w->country[c].name; int lw=text_w(g_font_small,nm);
        fill_rect(ren, sx-lw/2-3, sy-7, lw+6, 14, (SDL_Color){0x08,0x0c,0x14,(Uint8)(a*0.66f)});
        draw_text_a(ren, g_font_small, sx-lw/2, sy-6, (SDL_Color){0xf2,0xe8,0xd0,0xff}, a, nm);
    }
}

/* Une armée de campagne EST-ELLE en mouvement quelque part ? (sert au mode --war
 * pour laisser une guerre mûrir avant la capture.) */
static bool any_field_army(const Sim *s, const World *w){
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        if (campaign_active(s->camp,c) && campaign_phase(s->camp,c)!=FA_IDLE) return true;
    return false;
}

/* Capture hors-écran (mode --shot) : sérialise le rendu courant en PPM, pour
 * vérifier l'UI sans display interactif. */
static void save_ppm(const char *path, const uint32_t *px, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i=0;i<w*h;i++) {
        uint32_t c = px[i];
        unsigned char rgb[3] = { (unsigned char)((c>>16)&0xFF),
                                 (unsigned char)((c>>8)&0xFF),
                                 (unsigned char)(c&0xFF) };
        fwrite(rgb,1,3,f);
    }
    fclose(f);
    printf("[scps] capture écrite : %s\n", path);
}

/* ======================================================================= */

/* ═══════════════════════════════════════════════════════════════════════════
 * LE SHELL DU JEU (brief shell) — menu → création → litanie → OUVERTURE EN PAUSE
 * → partie. Discipline ESC : fermer la couche du dessus, JAMAIS quitter l'appli
 * (quitter = bouton + confirmation, seul chemin). Sauvegarde : chantier suivant
 * (le menu vit avec « Charger » grisé — l'ordre du brief lui-même).
 * ═══════════════════════════════════════════════════════════════════════════ */
static bool g_pause_menu=false, g_quit_confirm=false, g_show_tuto=false;
static int  g_tuto_page=0;
static int  g_setup_ethos=5, g_setup_race=(int)RACE_HUMAIN, g_setup_terre=5;  /* défauts : Pacifiste? non → voir tables */
static char g_open_terre_line[120]="";
static WorldParams g_stage;            /* l'écran de création édite une COPIE */
static bool g_pending_open=false;      /* après la forge : entrer en OUVERTURE */
static int  g_save_pick=0, g_load_pick=0;   /* surcouches : choisir un slot (1=ouvert) */
static int  g_load_confirm=-1;               /* slot à charger après confirmation (partie en cours) */
static bool g_game_started=false;            /* une partie a commencé (→ confirmation au chargement) */
/* cibles cliquables du shell */
enum { SH_SLOT_SAVE=100, SH_SLOT_LOAD, SH_PICK_CLOSE, SH_LOADC_YES, SH_LOADC_NO,
       SH_MENU_ITEM=1, SH_SLIDER_DN, SH_SLIDER_UP, SH_SEED_DICE, SH_PICK_ETHOS,
       SH_PICK_RACE, SH_PICK_TERRE, SH_FORGER, SH_BACK, SH_OPEN_GO, SH_OPEN_REROLL,
       SH_PM_ITEM, SH_QC_YES, SH_QC_NO, SH_TUTO_PREV, SH_TUTO_NEXT,
       SH_DEFEAT /* a=0 observer · a=1 menu */ };
typedef struct { SDL_Rect r; int kind, a; } ShellHit;
static ShellHit g_shhits[80]; static int g_nshhits;
static void shhit_reset(void){ g_nshhits=0; }
static void shhit_add(SDL_Rect r,int k,int a){ if(g_nshhits<80){ g_shhits[g_nshhits].r=r; g_shhits[g_nshhits].kind=k; g_shhits[g_nshhits].a=a; g_nshhits++; } }

/* ── LE TUTORIEL — texte embarqué tel quel (brief §7), pages courtes ── */
static const char *SH_ETHOS_N[6]={"Dominateur","Honneur","Mercantile","Bureaucrate","Ordre","Pacifiste"};
static const char *SH_ETHOS_L[6]={
    "la conquête est un droit","la parole vaut le sang","tout s'achète, surtout la paix",
    "l'archive gouverne mieux que l'épée","la discipline tient le monde","rien ne vaut le sang"};
static const char *SH_TERRE_N[6]={"Plaines","Côte","Estuaire","Collines","Forêt","Au hasard"};

static void sh_button(SDL_Renderer *ren,int x,int y,int w,const char *txt,bool on,bool grise,int kind,int a){
    fill_rect(ren,x,y,w,26, grise?(SDL_Color){0x12,0x16,0x20,0xff}: on?(SDL_Color){0x3a,0x2c,0x1a,0xff}:(SDL_Color){0x18,0x20,0x30,0xff});
    draw_box(ren,x,y,w,26, grise?COL_PANEL2: on?COL_COPPER:COL_PANEL2);
    int tw=text_w(g_font,txt);
    draw_text(ren,g_font,x+(w-tw)/2,y+5, grise?(SDL_Color){0x55,0x55,0x55,0xff}: on?COL_COPPER:COL_PARCH, txt);
    if (!grise) shhit_add((SDL_Rect){x,y,w,26},kind,a);
}
/* un slider du monde : étiquette, valeur, [−][+] */
static int sh_slider(SDL_Renderer *ren,int x,int y,const char *lbl,const char *val,int idx,const char *hov){
    draw_text(ren,g_font,x,y,COL_DIM,lbl);
    draw_text(ren,g_font,x+150,y,COL_PARCH,val);
    fill_rect(ren,x+230,y+1,18,16,(SDL_Color){0x18,0x20,0x30,0xff}); draw_box(ren,x+230,y+1,18,16,COL_PANEL2);
    draw_text(ren,g_font,x+235,y,COL_PARCH,"-"); shhit_add((SDL_Rect){x+230,y+1,18,16},SH_SLIDER_DN,idx);
    fill_rect(ren,x+252,y+1,18,16,(SDL_Color){0x18,0x20,0x30,0xff}); draw_box(ren,x+252,y+1,18,16,COL_PANEL2);
    draw_text(ren,g_font,x+257,y,COL_PARCH,"+"); shhit_add((SDL_Rect){x+252,y+1,18,16},SH_SLIDER_UP,idx);
    if (hov) zone_add((SDL_Rect){x,y-2,270,20},hov);
    return y+24;
}
static void sh_apply_slider(WorldParams *p,int idx,int dir){
    float st=0.05f*dir;
    switch(idx){
        case 0: p->n_continents+=dir; if(p->n_continents<1)p->n_continents=1; if(p->n_continents>8)p->n_continents=8; break;
        case 1: p->land_amount+=st;  if(p->land_amount<0)p->land_amount=0; if(p->land_amount>1)p->land_amount=1; break;
        case 2: p->world_age+=st;    if(p->world_age<0)p->world_age=0; if(p->world_age>1)p->world_age=1; break;
        case 3: p->erosion+=st;      if(p->erosion<0)p->erosion=0; if(p->erosion>1)p->erosion=1; break;
        case 4: p->mountains+=st;    if(p->mountains<0)p->mountains=0; if(p->mountains>1)p->mountains=1; break;
        case 5: p->temperature+=st;  if(p->temperature<0)p->temperature=0; if(p->temperature>1)p->temperature=1; break;
        case 6: p->humidity+=st;     if(p->humidity<0)p->humidity=0; if(p->humidity>1)p->humidity=1; break;
        case 7: p->n_empires+=dir;   if(p->n_empires<2)p->n_empires=2; if(p->n_empires>15)p->n_empires=15; break;
        case 8: p->n_city_states+=dir; if(p->n_city_states<0)p->n_city_states=0; if(p->n_city_states>20)p->n_city_states=20; break;
    }
}
/* le VŒU DE TERRE — best-effort, jamais un mur : on déplace la capitale du joueur
 * vers une de SES provinces qui honore le vœu ; sinon ligne honnête. */
static void sh_apply_terre(World *w, Sim *s){
    g_open_terre_line[0]=0;
    if (g_setup_terre>=5) return;                          /* au hasard : on prend ce que la gen a donné */
    int me=s->player;
    Biome want1=BIO_PLAINS, want2=BIO_FARMLAND;
    switch(g_setup_terre){
        case 0: want1=BIO_PLAINS;  want2=BIO_FARMLAND; break;
        case 1: want1=BIO_COAST;   want2=BIO_MANGROVE; break;
        case 2: want1=BIO_COAST;   want2=BIO_MARSH;    break;   /* estuaire ≈ côte d'embouchure */
        case 3: want1=BIO_HILLS;   want2=BIO_HIGHLANDS;break;
        case 4: want1=BIO_FOREST;  want2=BIO_WOODS;    break;
    }
    int best=-1;
    for (int p=0;p<w->n_provinces;p++){
        if (w->province[p].country!=me) continue;
        Biome b=w->province[p].biome_dominant;
        bool estu = (g_setup_terre==2) ? (w->province[p].coastal && b!=BIO_DESERT) : false;
        if (b==want1 || b==want2 || estu){ best=p; break; }
    }
    if (best>=0) w->country[me].capital_prov=best;
    else snprintf(g_open_terre_line,sizeof g_open_terre_line,
                  "Nulle %s libre sur ce monde — vous voilà où la terre a voulu.",
                  SH_TERRE_N[g_setup_terre]);
}
static void sh_center_capital(const World *w, const Sim *s, Cam *cam, int win_w, int win_h){
    int cp=(s->player>=0&&s->player<w->n_countries)?w->country[s->player].capital_prov:-1;
    if (cp<0||cp>=w->n_provinces) return;
    cam->scale=3.0f;
    cam->ox=(float)w->province[cp].seed_x - (float)win_w/(2.f*cam->scale);
    cam->oy=(float)w->province[cp].seed_y - (float)win_h/(2.f*cam->scale);
}
/* l'ouverture applique les CHOIX du joueur sur le monde fraîchement forgé */
static void sh_enter_opening(World *w, Sim *s, Cam *cam, int win_w, int win_h, GameSpeed *speed){
    sh_apply_terre(w,s);
    { int cp=w->country[s->player].capital_prov;                    /* l'ÉTHOS choisi s'ancre au trône */
      int cr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
      if (cr>=0&&cr<s->econ->n_regions) s->econ->region[cr].culture.ethos=(Ethos)g_setup_ethos; }
    sh_center_capital(w,s,cam,win_w,win_h);
    *speed=SPEED_PAUSE;                                              /* PAUSE FORCÉE : on ne vole pas le premier regard */
    g_gs=GS_OPENING;
}
/* la LITANIE de forge (une frame d'ambiance avant la gen synchrone) */
static void sh_draw_litanie(SDL_Renderer *ren,int win_w,int win_h,uint32_t seedv){
    fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x0a,0x0e,0x16,0xff});
    draw_text(ren,g_font_big,win_w/2-90,win_h/2-110,COL_COPPER,"LE MONDE SE FORGE");
    static const char *L[]={ "géologie…","architecture…","altération…","érosion…","côtes…",
                             "climat (vents)…","vallées…","biomes…","fertilité…","territoires…",
                             "hiérarchie…","peuples…","ressources…","sites de départ…","rivières…" };
    for (int i=0;i<15;i++) draw_text(ren,g_font,win_w/2-70,win_h/2-70+i*16,COL_DIM,L[i]);
    char sd[48]; snprintf(sd,sizeof sd,"graine %u",seedv);
    draw_text(ren,g_font,win_w/2-40,win_h/2+180,COL_PARCH,sd);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * LA SAUVEGARDE (brief shell §6) — en-tête versionné + sections TAGUÉES.
 * Chaque struct de système est PLATE (audité) ; les modules à états statiques
 * (intertrade, agency, diplo, factions) possèdent leur sérialisation. Version
 * qui ne matche pas = refus poli (« sauvegarde d'une ère antérieure »).
 * ═══════════════════════════════════════════════════════════════════════════ */
#define SAVE_MAGIC   0x53504353u   /* "SCPS" */
#define SAVE_VERSION 33u           /* v33 : OPINION À MÉMOIRE (#26) — Statecraft gagne opinion_mem[][] (ledger
                                    * durable des actes : la trahison qui survit au statut) ⇒ sizeof(Statecraft)
                                    * change → <v33 refusé. opinion_mem est un float à effet BORNÉ (il ne fait
                                    * que nourrir opinion, clampé ±100 au tick) — aucun index → pas de clause
                                    * save_sane neuve (le blob SVT_STAT le persiste).
                                    * v32 : VASSALITÉ SUR LA DURÉE (pipeline diplo étage 3) — DiploState gagne
                                    * v_integration[]/v_annex[] (+ n_annex) et RegionEconomy gagne annex_scar ⇒
                                    * sizeof(DiploState) & sizeof(WorldEconomy) changent → <v32 refusé. save_sane
                                    * borne annex_scar/v_integration/v_annex ∈ [0,1] et suzerain ∈ [-1,n).
                                    * v31 : EMPREINTE TUNABLES — SaveHeader gagne `tune_ck` (FNV des surcharges
                                    * SCPS_TUNE résolues, tune_active_string) ⇒ sizeof(SaveHeader) change → <v31
                                    * refusé. Au chargement, un tune_ck DIFFÉRENT AVERTIT (la partie évoluera sous
                                    * d'AUTRES règles ⇒ replays/graines-partagées invalides) sans bloquer le load.
                                    * v30 : ROSTER 22 — le roster militaire passe de 12 à 22 unités (spec
                                    * design). ArmyState.weapons[W_COUNT] croît (12→22 slots de tampon de
                                    * combat) ⇒ sizeof(ArmyState) ⇒ sizeof(WarHost)/Campaign change → blob
                                    * sv_w plus large → ère antérieure (<v30 refusé). Indices des 12 unités
                                    * d'origine PRÉSERVÉS (Unit.type sérialisé reste valide), seuls 10 types
                                    * neufs s'ajoutent au-delà.
                                    * v29 : MERGE des deux lignées — le format COMBINE les deux jeux de
                                    * changements de struct (donc <v29 refusé) :
                                    *   · (assets/worldgen-graphe) Region GAGNE `harbor` (float) ⇒ sizeof(World) ;
                                    *     TradeRoute GAGNE `choke_region`/`choke_block` ⇒ sizeof(RouteNetwork) ;
                                    *     Director (blob EVNT) gagne adapt_days/budget/amplitude/mem[]/omens ⇒ sizeof(EventsState).
                                    *   · (vitalité) RegionEconomy GAGNE ferveur+reconstruction (v27) + prov_geo (v28) ⇒
                                    *     sizeof(WorldEconomy) ; EndgameState §27 (v26, section EGAM).
                                    * save_sane borne harbor∈[0,1], choke_region∈[-1,n_regions), mem_head/mem.* (désérialisés).
                                    * v28 : DONS GÉO — RegionEconomy.prov_geo (gibier/halieutique).
                                    * v27 : MODIFICATEURS lot 2 — RegionEconomy.ferveur + reconstruction.
                                    * v26 : CAPSTONE §27 — EndgameState (entropie + fins + merveille) section EGAM.
                                    * v25 : UN SEUL LIVRE D'OR — LR_GOLD éradiqué (l'or vit dans econ country_gold,
                                    * dette via scps_credit). LaborEcon perd treasury + stock/flow[LR_GOLD] (LRes 2→1,
                                    * LR_FOOD seul) ⇒ sizeof(LaborEcon) rétrécit (blob sv_w) → ère antérieure (<v25 refusé).
                                    * v24 : LIMITEUR — section prod_cap appendue après CRDT (econ_prodcap_save/load). <v24 refusé.
                                    * v23 : FERTILITÉ — RegionEconomy.needs_met (float) ⇒ sizeof(WorldEconomy)
                                    * change → ère antérieure (<v23 refusé).
                                    * v22 : DETTE — section CRDT (scps_credit g_creditor[]) appendue
                                    * au save (après FACT). save_sane borne g_creditor. <v22 refusé.
                                    * v21 : P-arc — la couche MATÉRIAU labor ÉRADIQUÉE (le matériau
                                    * vit dans le pool éco). LRes 7→2 (LR_FOOD/LR_GOLD) ⇒ LaborEcon.stock/
                                    * flow[LR_COUNT] et g_pres[][LR_COUNT] rétrécissent : sizeof(LaborEcon)
                                    * change → ère antérieure (les saves <v21 sont refusés au chargement).
                                    * v20 : F-arc (alchimie & FAUSTIEN) — RES_FLUX + RES_ALCHEMIST_KIT
                                    * appendus (RES_COUNT change) + TECH_ALCHIMIE (unlocked[TECH_COUNT]).
                                    * v19 : M6 — LR_CALCAIRE coupé (LaborEcon.stock[LR_COUNT] 8→7).
                                    * v18 : M3 — DiploState.trade_pact[pays][pays] (le pacte commercial réciproque).
                                    * v17 : Q1 — Statecraft.council[pays][3] (état conseil persistant).
                                    * v16 : Country.region_ids[12→32] (mondes fragmentés dépassaient 12).
                                    * v15 : arc M — les fourches (RegionEconomy.last_pole/pole_since_day,
                                    * +5 édifices, BLD_ALAMBIC + RES_ESSENCE_PURIFIEE : RES_COUNT change).
                                    * v14 : arc L — le ralliement (FieldArmy.rally_*, Campaign.n_rallies).
                                    * v13 : ère « les puits d'or / le joueur en scène » (arcs I/H) —
                                    * AiActor.next_audit_day (I5) + RegionEconomy.import_margin/
                                    * import_toll_region (I6) ; ride les futurs champs I7/I8/H5/H6.
                                    * v12 : G0.1 anti-acharnement multi-échelle (Director.cont_cd_until[]).
                                    * v11 = EventsState.director (§F) ; v10 = AiActor.spec_cd[] (B1).
                                    * Ère antérieure à chaque bump (struct sérialisée plus large). */
#define SAVE_F_CRYPT 1u
typedef struct {
    uint32_t magic, version;
    uint32_t seed;
    int32_t  day, year, player;
    WorldParams params;
    int64_t  stamp;            /* horodatage (time) */
    char     line[96];         /* « An 87 — Empire de X, 12 régions » (écran Charger) */
    uint32_t payload;          /* taille attendue après l'en-tête (intégrité) */
    uint32_t flags;            /* SAVE_F_CRYPT : sections chiffrées (l'en-tête reste en CLAIR) */
    uint64_t nonce;            /* nonce ChaCha20 — unique par sauvegarde */
    uint64_t plain_ck;         /* empreinte FNV-1a du CLAIR : un octet altéré = refus net */
    uint32_t tune_ck;          /* P0-3 : FNV des surcharges SCPS_TUNE résolues (tune_active_string) — une save
                                * faite sous d'autres tunables, rechargée, évolue sous d'AUTRES règles : on le DÉTECTE */
} SaveHeader;
typedef struct { int32_t day, year, player, prev_dawned; uint32_t camp_rng;
                 int32_t race, ethos; int16_t prev_owner[SCPS_MAX_REG]; } SaveMisc;
static const char *save_slot_path(int slot){
    static char p[64]; snprintf(p,sizeof p,"saves/slot_%d.scps",slot); return p;
}
static bool save_slot_info(int slot, SaveHeader *out){
    FILE *f=fopen(save_slot_path(slot),"rb");
    if (!f) return false;
    bool ok = fread(out,sizeof *out,1,f)==1 && out->magic==SAVE_MAGIC;
    fclose(f); return ok;
}
#define SV_TAG(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
/* REGISTRE DES SECTIONS (X-macro) — l'ORDRE et les tags vivent ICI, en un seul
 * endroit lisible. game_save écrit cette suite, game_load la relit dans LE MÊME
 * ordre : la table évite qu'une moitié dérive de l'autre (un tag changé d'un
 * côté seul = format cassé). Chaque ligne : SYMBOLE, 4 octets ASCII du tag. */
#define SV_SECTIONS(X) \
    X(WRLD,'W','R','L','D')  /* World                       */ \
    X(ECON,'E','C','O','N')  /* WorldEconomy                */ \
    X(PROS,'P','R','O','S')  /* WorldProsperity             */ \
    X(LEGI,'L','E','G','I')  /* WorldLegitimacy             */ \
    X(NETW,'N','E','T','W')  /* InfluenceNet                */ \
    X(TECH,'T','E','C','H')  /* TechState[pays]             */ \
    X(STAT,'S','T','A','T')  /* Statecraft                  */ \
    X(AGCY,'A','G','C','Y')  /* AgencyState                 */ \
    X(EVNT,'E','V','N','T')  /* EventsState                 */ \
    X(DRFT,'D','R','F','T')  /* DriftState                  */ \
    X(LABO,'L','A','B','O')  /* LaborEcon                   */ \
    X(DIPL,'D','I','P','L')  /* DiploState                  */ \
    X(RTES,'R','T','E','S')  /* RouteNetwork                */ \
    X(RVLT,'R','V','L','T')  /* RevoltState                 */ \
    X(MISS,'M','I','S','S')  /* MissionState                */ \
    X(CAMP,'C','A','M','P')  /* Campaign                    */ \
    X(NAVY,'N','A','V','Y')  /* NavyState                   */ \
    X(HARM,'H','A','R','M')  /* WarHost.army (sans scratch) */ \
    X(HLVY,'H','L','V','Y')  /* WarHost.levy                */ \
    X(AIAC,'A','I','A','C')  /* AiActor[pays]               */ \
    X(AION,'A','I','O','N')  /* ai_on[pays]                 */ \
    X(MISC,'M','I','S','C')  /* SaveMisc                    */ \
    X(ITRD,'I','T','R','D')  /* intertrade (états statiques) */ \
    X(AGYS,'A','G','Y','S')  /* agency (états statiques)    */ \
    X(DPLS,'D','P','L','S')  /* diplo (statiques)           */ \
    X(FACT,'F','A','C','T')  /* factions (statiques)        */ \
    X(CRDT,'C','R','D','T')  /* dette : g_creditor[]        */ \
    X(PCAP,'P','C','A','P')  /* limiteur de production (v24) */
/* Chaque symbole devient une constante de tag : SVT_WRLD, SVT_ECON, … */
#define SV_DECL_TAG(name,a,b,c,d) enum { SVT_##name = SV_TAG(a,b,c,d) };
SV_SECTIONS(SV_DECL_TAG)
#undef SV_DECL_TAG
static bool sv_w(FILE *f, uint32_t tag, const void *p, size_t sz){
    uint32_t z=(uint32_t)sz;
    return fwrite(&tag,4,1,f)==1 && fwrite(&z,4,1,f)==1 && (sz==0 || fwrite(p,sz,1,f)==1);
}
static bool sv_r(FILE *f, uint32_t tag, void *p, size_t sz){
    uint32_t t,z;
    if (fread(&t,4,1,f)!=1 || fread(&z,4,1,f)!=1) return false;
    if (t!=tag || z!=(uint32_t)sz) return false;
    return sz==0 || fread(p,sz,1,f)==1;
}
/* sauve la partie ENTIÈRE dans un slot ; renvoie false si l'écriture échoue.
 *
 * Le format est ASSEMBLÉ EN MÉMOIRE (en-tête + payload taggé), puis posé d'un
 * seul geste ATOMIQUE (write-then-rename via save_write_atomic) : un crash, un
 * disque plein, une coupure EN COURS d'écriture ne corrompt JAMAIS le slot
 * existant — l'ancienne sauvegarde reste chargeable jusqu'à ce que la neuve soit
 * intégralement écrite et flushée. Le payload est d'abord composé via les
 * sérialiseurs FILE* (intertrade_save, agency_save, …) dans un tmpfile() — relu
 * pour l'empreinte FNV puis le chiffrement —, ce qui laisse ces modules
 * INCHANGÉS. */
static bool game_save(int slot, World *w, Sim *s, const WorldParams *params){
    if (slot<1 || slot>3) return false;
    scps_mkdir("saves");                         /* EEXIST inoffensif ; l'échec réel tombe au write */
    /* Étape 1 : composer le PAYLOAD CLAIR dans un fichier temporaire anonyme
     * (auto-effacé). Les modules à états statiques sérialisent vers ce FILE*. */
    FILE *f=tmpfile();
    if (!f) return false;
    SaveHeader h; memset(&h,0,sizeof h);
    h.magic=SAVE_MAGIC; h.version=SAVE_VERSION; h.seed=params->seed;
    h.day=s->day; h.year=s->year; h.player=s->player; h.params=*params;
    h.stamp=(int64_t)time(NULL);
    { const char *tstr=tune_active_string();    /* P0-3 : empreinte des surcharges SCPS_TUNE résolues */
      h.tune_ck=(uint32_t)scrypt_fnv1a(tstr, strlen(tstr)); }
    { int nreg=0; for (int r=0;r<s->econ->n_regions;r++) if (s->econ->region[r].owner==s->player) nreg++;
      snprintf(h.line,sizeof h.line,"An %d — %s, %d région(s)",
               s->year, (s->player>=0&&s->player<w->n_countries)?w->country[s->player].name:"?", nreg); }
    bool ok = true;
    ok&=sv_w(f,SVT_WRLD, w,        sizeof *w);
    ok&=sv_w(f,SVT_ECON, s->econ,  sizeof *s->econ);
    ok&=sv_w(f,SVT_PROS, s->wp,    sizeof *s->wp);
    ok&=sv_w(f,SVT_LEGI, s->wl,    sizeof *s->wl);
    ok&=sv_w(f,SVT_NETW, s->net,   sizeof *s->net);
    ok&=sv_w(f,SVT_TECH, s->ts,    sizeof(TechState)*SCPS_MAX_COUNTRY);
    ok&=sv_w(f,SVT_STAT, s->sc,    sizeof *s->sc);
    ok&=sv_w(f,SVT_AGCY, s->ag,    sizeof *s->ag);
    ok&=sv_w(f,SVT_EVNT, s->ev,    sizeof *s->ev);
    ok&=sv_w(f,SVT_DRFT, s->drift, sizeof *s->drift);
    ok&=sv_w(f,SVT_LABO, s->labor, sizeof *s->labor);
    ok&=sv_w(f,SVT_DIPL, s->dp,    sizeof *s->dp);
    ok&=sv_w(f,SVT_RTES, s->rn,    sizeof *s->rn);
    ok&=sv_w(f,SVT_RVLT, s->rs,    sizeof *s->rs);
    ok&=sv_w(f,SVT_MISS, s->missions, sizeof *s->missions);
    ok&=sv_w(f,SVT_CAMP, s->camp,  sizeof *s->camp);
    ok&=sv_w(f,SVT_NAVY, s->navy,  sizeof *s->navy);
    ok&=sv_w(f,SVT_HARM, s->host->army, sizeof s->host->army);   /* WarHost SANS scratch */
    ok&=sv_w(f,SVT_HLVY, s->host->levy, sizeof s->host->levy);
    ok&=sv_w(f,SVT_AIAC, s->ai,    sizeof(AiActor)*SCPS_MAX_COUNTRY);
    ok&=sv_w(f,SVT_AION, s->ai_on, sizeof(bool)*SCPS_MAX_COUNTRY);
    { SaveMisc m; memset(&m,0,sizeof m);
      m.day=s->day; m.year=s->year; m.player=s->player; m.prev_dawned=s->prev_dawned;
      m.camp_rng=s->camp_rng; m.race=(int32_t)g_player_race; m.ethos=g_setup_ethos;
      memcpy(m.prev_owner,s->prev_owner_mo,sizeof m.prev_owner);
      ok&=sv_w(f,SVT_MISC, &m, sizeof m); }
    /* les modules à ÉTATS STATIQUES possèdent leur sérialisation */
    ok&=sv_w(f,SVT_ITRD, NULL,0); intertrade_save(f);
    ok&=sv_w(f,SVT_AGYS, NULL,0); agency_save(f);
    ok&=sv_w(f,SVT_DPLS, NULL,0); diplo_save_statics(f);
    ok&=sv_w(f,SVT_FACT, NULL,0); faction_save(f);
    ok&=sv_w(f,SVT_CRDT, NULL,0); credit_save(f);   /* dette : g_creditor[] */
    ok&=sv_w(f,SVT_PCAP, NULL,0); econ_prodcap_save(f);   /* v24 : limiteur de production */
    if (s->eg) ok&=sv_w(f,SV_TAG('E','G','A','M'), s->eg, sizeof *s->eg);  /* v26 : EndgameState (capstone §27) */
    /* Étape 2 : aspirer le payload clair en mémoire (empreinte FNV + chiffrement).
     * Le fichier final = en-tête CLAIR (l'écran Charger lit la ligne sans
     * déchiffrer) suivi du payload chiffré. */
    if (ok && fflush(f)!=0) ok=false;
    long psz = ok ? ftell(f) : -1;
    if (!ok || psz<0){ fclose(f); return false; }
    h.payload=(uint32_t)psz;
    uint8_t *img=(uint8_t*)malloc(sizeof h + (size_t)h.payload);   /* en-tête + payload, d'un bloc */
    if (!img){ fclose(f); return false; }
    uint8_t *pay = img + sizeof h;
    rewind(f);
    if (fread(pay,1,h.payload,f)!=h.payload){ free(img); fclose(f); return false; }
    fclose(f);
    h.plain_ck = scrypt_fnv1a(pay,h.payload);
    /* Nonce : modèle « obfuscation, pas secret » (la clé vit dans le binaire) —
     * l'unicité n'est requise que pour éviter un keystream identique entre deux
     * sauvegardes ; le compteur monotone couvre les sauvegardes rapprochées. */
    { static uint64_t seq=0; ++seq;
      h.nonce = ((uint64_t)time(NULL)<<32) ^ (uint64_t)SDL_GetTicks()
              ^ ((uint64_t)params->seed<<13) ^ (uint64_t)(uintptr_t)pay
              ^ (seq<<48) ^ (uint64_t)clock(); }
    h.flags = SAVE_F_CRYPT;
    scrypt_stream(h.nonce, pay, h.payload);
    memcpy(img,&h,sizeof h);                      /* en-tête FINAL (payload/nonce/empreinte) en tête de bloc */
    /* Étape 3 : poser le bloc d'un seul geste ATOMIQUE. */
    ok = save_write_atomic(save_slot_path(slot), img, sizeof h + (size_t)h.payload);
    free(img);
    return ok;
}
/* Garde-fou post-chargement : le moteur entier boucle sur ces comptes et indices
 * en LEUR FAISANT CONFIANCE (n_countries ≤ 56, cell.province < n_provinces…).
 * L'empreinte FNV-1a n'est pas un MAC (la clé vit dans le binaire) : un fichier
 * FORGÉ passe l'intégrité — on revalide donc les invariants avant de déclarer
 * la partie prête. Refus net, comme le reste du chargeur. */
static bool save_sane(const World *w, const Sim *s, int player){
    if (w->n_provinces <0 || w->n_provinces >SCPS_MAX_PROV)      return false;
    if (w->n_regions   <0 || w->n_regions   >SCPS_MAX_REG)       return false;
    if (w->n_countries <0 || w->n_countries >SCPS_MAX_COUNTRY)   return false;
    if (w->n_continents<0 || w->n_continents>SCPS_MAX_CONTINENT) return false;
    for (int c=0;c<w->n_countries;c++)                          /* dette : créancier désérialisé borné */
        if (credit_of(c) < -1 || credit_of(c) >= w->n_countries) return false;
    if (w->n_rivers    <0 || w->n_rivers    >SCPS_MAX_RIVERS)    return false;
    for (int i=0;i<w->n_rivers;i++)
        if (w->river[i].len<0 || w->river[i].len>SCPS_RIVER_MAXLEN) return false;
    if (player<0 || player>=w->n_countries) return false;
    /* Bornes BASSES autant que hautes : -1 est le seul négatif légitime (sentinelle
     * « mer / non-possédé / aucun ») — on le tolère là où il a un sens, on rejette
     * tout index plus négatif (un save forgé ne peut alors sortir du domaine [-1,n)
     * que le moteur sait déjà traiter). province.region/country, eux, sont TOUJOURS
     * assignés : on y rejette dès < 0. */
    for (int i=0;i<SCPS_N;i++){ const Cell *c=&w->cell[i];
        if (c->province < -1 || c->region    < -1 ||
            c->country  < -1 || c->continent < -1) return false;
        if (c->province >= w->n_provinces || c->region    >= w->n_regions ||
            c->country  >= w->n_countries || c->continent >= w->n_continents) return false; }
    for (int p=0;p<w->n_provinces;p++){ const Province *pr=&w->province[p];
        if (pr->region < 0 || pr->country < 0) return false;
        if (pr->region >= w->n_regions || pr->country >= w->n_countries) return false; }
    for (int r=0;r<w->n_regions;r++){ const Region *rg=&w->region[r];
        if (rg->n_provinces<0 || rg->n_provinces>12 || rg->country< -1 || rg->country>=w->n_countries) return false;
        for (int k=0;k<rg->n_provinces;k++)
            if (rg->province_ids[k]<0 || rg->province_ids[k]>=w->n_provinces) return false;
        /* WG : l'aptitude portuaire désérialisée doit rester une coordonnée FINIE de [0,1]
         * (navy_best_coast la lit pour SCORER la rade — un NaN/hors-borne forgé fausserait
         * le choix de port ; on refuse net, comme tout le chargeur). */
        if (!(rg->harbor>=0.f && rg->harbor<=1.f)) return false; }
    for (int c=0;c<w->n_countries;c++){ const Country *ct=&w->country[c];
        if (ct->n_regions<0 || ct->n_regions>32 || ct->capital_prov< -1 || ct->capital_prov>=w->n_provinces) return false;
        for (int k=0;k<ct->n_regions;k++)
            if (ct->region_ids[k]<0 || ct->region_ids[k]>=w->n_regions) return false; }
    if (s->econ->n_regions<0 || s->econ->n_regions>SCPS_MAX_REG) return false;
    for (int r=0;r<s->econ->n_regions;r++){ const RegionEconomy *re=&s->econ->region[r];
        if (re->owner < -1 || re->owner >= w->n_countries) return false;
        if (re->pop.n_groups<0 || re->pop.n_groups>SCPS_MAX_GROUPS) return false;
        /* étage 3 (v32) : la cicatrice d'annexion FRAPPE la satisfaction — un forgé hors-[0,1]
         * fausserait l'humeur ; on la borne comme toute coordonnée désérialisée. */
        if (!(re->annex_scar>=0.f && re->annex_scar<=1.f)) return false; }
    if (s->rn->n<0 || s->rn->n>SCPS_MAX_ROUTES) return false;
    for (int i=0;i<s->rn->n;i++){ const TradeRoute *rt=&s->rn->route[i];
        if (rt->ra<0 || rt->ra>=s->econ->n_regions || rt->rb<0 || rt->rb>=s->econ->n_regions) return false;
        /* WG : la région-flanc du détroit (lue par intertrade pour trouver le tenant) —
         * -1 (aucun) ou un index de région valide ; un forgé indexerait econ->region. */
        if (rt->choke_region< -1 || rt->choke_region>=s->econ->n_regions) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){ const FieldArmy *a=&s->camp->army[i];
        if (!a->active) continue;
        if (a->owner<0 || a->owner>=w->n_countries) return false;
        if (a->loc <0 || a->loc >=s->econ->n_regions) return false;
        if (a->dest< -1 || a->dest>=s->econ->n_regions || a->next< -1 || a->next>=s->econ->n_regions) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){ const Navy *nv=&s->navy->n[i];
        for (int t=0;t<HULL_COUNT;t++) if (nv->hull[t]<0 || nv->hull[t]>100000) return false;
        if (nv->at_sea<0 || nv->build_hull<-1 || nv->build_hull>=HULL_COUNT) return false;
        if (nv->home_port< -1 || nv->home_port>=s->econ->n_regions) return false; }
    /* §terrain (v5) : l'OCCUPATION et la dernière réduction non récoltée se revalident —
     * un occupier forgé indexerait w->country, un taken_region forgé econ->region. */
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)
        if (s->dp->occupier[r] < -1 || s->dp->occupier[r] >= w->n_countries) return false;
    /* étage 3 (v32) : le lien suzerain (indexé par l'annexion-processus) + les jauges
     * d'intégration/d'annexion désérialisées — un forgé hors-domaine fausserait la digestion
     * (transfert de régions) ou ferait une boucle d'indexation hors-borne. Refus net. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (s->dp->suzerain[c] < -1 || s->dp->suzerain[c] >= w->n_countries) return false;
        if (!(s->dp->v_integration[c]>=0.f && s->dp->v_integration[c]<=1.f)) return false;
        if (!(s->dp->v_annex[c]      >=0.f && s->dp->v_annex[c]      <=1.f)) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++)
        if (s->camp->army[i].taken_region < -1 || s->camp->army[i].taken_region >= s->econ->n_regions) return false;
    /* P0 : COMPTEURS désérialisés qui BORNENT des boucles / indexent des tableaux, jusqu'ici NON revalidés
     * (agency_advance, revolt_tick et les boucles d'unités leur font CONFIANCE — un compte forgé = lecture/
     * écriture hors-bornes). On les borne comme tout le reste du chargeur ; + chaque région d'ordre indexe
     * econ->region (purge_slice / apply_action / relocate / colonize). */
    if (s->ag){
        if (s->ag->n < 0 || s->ag->n > SCPS_MAX_BUILDS) return false;
        for (int i=0;i<s->ag->n;i++){ const BuildOrder *o=&s->ag->order[i];
            if (o->region < -1 || o->region >= s->econ->n_regions) return false;
            if ((o->kind==AGY_RELOCATE || o->kind==AGY_COLONIZE) &&
                (o->param < -1 || o->param >= s->econ->n_regions)) return false; }
    }
    if (s->rs){
        if (s->rs->count < 0 || s->rs->count > REVOLT_MAX) return false;
        for (int i=0;i<s->rs->count;i++)
            if (s->rs->list[i].region < -1 || s->rs->list[i].region >= s->econ->n_regions) return false;
    }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        if (s->camp->army[i].force.n_units < 0 || s->camp->army[i].force.n_units > ARMY_MAX_UNITS) return false;
        if (s->host && (s->host->army[i].n_units < 0 || s->host->army[i].n_units > ARMY_MAX_UNITS)) return false;
    }
    /* capstone §27 (v26) : EndgameState — bornes sur tous les champs-indices */
    if (s->eg) {
        const EndgameState *eg = s->eg;
        if ((int)eg->fin < 0 || (int)eg->fin > (int)FIN_ASCENSION) return false;
        if ((int)eg->merv < 0 || (int)eg->merv > (int)MERV_ASCENDED) return false;
        if (eg->epicenter_reg < -1 || eg->epicenter_reg >= s->econ->n_regions) return false;
        if (eg->fauteur_country < -1 || eg->fauteur_country >= w->n_countries) return false;
        if (eg->merv_country    < -1 || eg->merv_country    >= w->n_countries) return false;
        if (eg->merv_site_reg   < -1 || eg->merv_site_reg   >= s->econ->n_regions) return false;
        if (eg->n_sunken < 0 || eg->n_sunken > SCPS_MAX_REG) return false;
        if (eg->sink_pending < 0) return false;
        if (eg->thorn_front_n < 0 || eg->thorn_front_n > SCPS_THORN_FRONT_MAX) return false;
        for (int i = 0; i < eg->thorn_front_n; i++)
            if (eg->thorn_front[i] < 0 || eg->thorn_front[i] >= SCPS_N) return false;
        if (eg->cold_offset < 0.0f || eg->cold_offset > 1.0f) return false;
        if (eg->merv_progress < 0.0f || eg->merv_progress > 1.0f) return false;
    }
    /* §G2 (v26) : le DIRECTEUR-AMPLITUDE désérialisé se revalide — mem_head BORNE l'écriture
     * dans l'anneau mem[DIR_MEM_CAP] (un index forgé déborderait la prochaine inscription) ;
     * chaque mem[i].kind est une étiquette [0..DMEM_KIND_COUNT) et mem[i].subject un index
     * que le présage relit (pays·MAX+pays pour l'Amnistie ⇒ < SCPS_MAX_COUNTRY²) — refus net.
     * La garde vit AVEC la struct (scps_events) et est testée headless (events_demo). */
    if (!director_save_sane(s->ev, SCPS_MAX_COUNTRY*SCPS_MAX_COUNTRY)) return false;
    return true;
}
/* charge un slot. 0 = ok ; 1 = absent/corrompu ; 2 = « ère antérieure » (version). */
#define SAVE_MAX_PAYLOAD (256u<<20)   /* plafond de vraisemblance : pas de malloc(4 Go) sur en-tête forgé */
static int game_load(int slot, World *w, Sim *s, WorldParams *params){
    if (slot<1 || slot>3) return 1;
    FILE *f=fopen(save_slot_path(slot),"rb");
    if (!f) return 1;
    SaveHeader h;
    if (fread(&h,sizeof h,1,f)!=1 || h.magic!=SAVE_MAGIC){ fclose(f); return 1; }
    if (h.version!=SAVE_VERSION){ fclose(f); return 2; }
    if (h.payload==0 || h.payload>SAVE_MAX_PAYLOAD){ fclose(f); return 1; }
    /* payload → mémoire ; déchiffré si marqué ; l'EMPREINTE du clair fait foi
     * (un octet altéré — chiffré ou non — et le chargement REFUSE net). */
    { uint8_t *buf=(uint8_t*)malloc(h.payload?h.payload:1);
      if (!buf){ fclose(f); return 1; }
      if (fread(buf,1,h.payload,f)!=h.payload){ free(buf); fclose(f); return 1; }
      fclose(f);
      if (h.flags & SAVE_F_CRYPT) scrypt_stream(h.nonce, buf, h.payload);
      if (scrypt_fnv1a(buf,h.payload)!=h.plain_ck){ free(buf); return 1; }   /* altéré → refus */
      f=tmpfile();
      if (!f){ free(buf); return 1; }
      if (fwrite(buf,1,h.payload,f)!=h.payload){ free(buf); fclose(f); return 1; }
      free(buf); rewind(f); }
    long p0=ftell(f);
    bool ok=true;
    ok&=sv_r(f,SVT_WRLD, w,        sizeof *w);
    ok&=sv_r(f,SVT_ECON, s->econ,  sizeof *s->econ);
    ok&=sv_r(f,SVT_PROS, s->wp,    sizeof *s->wp);
    ok&=sv_r(f,SVT_LEGI, s->wl,    sizeof *s->wl);
    ok&=sv_r(f,SVT_NETW, s->net,   sizeof *s->net);
    ok&=sv_r(f,SVT_TECH, s->ts,    sizeof(TechState)*SCPS_MAX_COUNTRY);
    ok&=sv_r(f,SVT_STAT, s->sc,    sizeof *s->sc);
    ok&=sv_r(f,SVT_AGCY, s->ag,    sizeof *s->ag);
    ok&=sv_r(f,SVT_EVNT, s->ev,    sizeof *s->ev);
    ok&=sv_r(f,SVT_DRFT, s->drift, sizeof *s->drift);
    ok&=sv_r(f,SVT_LABO, s->labor, sizeof *s->labor);
    ok&=sv_r(f,SVT_DIPL, s->dp,    sizeof *s->dp);
    ok&=sv_r(f,SVT_RTES, s->rn,    sizeof *s->rn);
    ok&=sv_r(f,SVT_RVLT, s->rs,    sizeof *s->rs);
    ok&=sv_r(f,SVT_MISS, s->missions, sizeof *s->missions);
    ok&=sv_r(f,SVT_CAMP, s->camp,  sizeof *s->camp);
    ok&=sv_r(f,SVT_NAVY, s->navy,  sizeof *s->navy);
    ok&=sv_r(f,SVT_HARM, s->host->army, sizeof s->host->army);
    ok&=sv_r(f,SVT_HLVY, s->host->levy, sizeof s->host->levy);
    ok&=sv_r(f,SVT_AIAC, s->ai,    sizeof(AiActor)*SCPS_MAX_COUNTRY);
    ok&=sv_r(f,SVT_AION, s->ai_on, sizeof(bool)*SCPS_MAX_COUNTRY);
    { SaveMisc m;
      ok&=sv_r(f,SVT_MISC, &m, sizeof m);
      if (ok){ s->day=m.day; s->year=m.year; s->player=m.player; s->prev_dawned=m.prev_dawned;
               s->camp_rng=m.camp_rng;
               if (m.race <0 || m.race >=(int32_t)RACE_COUNT)  m.race =(int32_t)RACE_HUMAIN;
               if (m.ethos<0 || m.ethos>=(int32_t)ETHOS_COUNT) m.ethos=0;
               g_player_race=(SpeciesArchetype)m.race; g_setup_ethos=m.ethos;
               memcpy(s->prev_owner_mo,m.prev_owner,sizeof m.prev_owner); } }
    ok&=sv_r(f,SVT_ITRD, NULL,0); ok&=intertrade_load(f);
    ok&=sv_r(f,SVT_AGYS, NULL,0); ok&=agency_load(f);
    ok&=sv_r(f,SVT_DPLS, NULL,0); ok&=diplo_load_statics(f);
    ok&=sv_r(f,SVT_FACT, NULL,0); ok&=faction_load(f);
    ok&=sv_r(f,SVT_CRDT, NULL,0); ok&=credit_load(f);   /* dette : g_creditor[] */
    ok&=sv_r(f,SVT_PCAP, NULL,0); ok&=econ_prodcap_load(f);   /* v24 : limiteur de production */
    if (s->eg) ok&=sv_r(f,SV_TAG('E','G','A','M'), s->eg, sizeof *s->eg);   /* v26 : EndgameState (capstone §27) */
    long p1=ftell(f); fclose(f);
    if (!ok || (uint32_t)(p1-p0)!=h.payload) return 1;     /* taille/section : refus net */
    if (!save_sane(w, s, s->player)) return 1;             /* invariants du moteur : refus net */
    demography_dyn_id_rebase(s->econ);                     /* drift_id dynamiques au-dessus du chargé */
    *params=h.params;
    warhost_set_human(s->player);                          /* P0 : RÉTABLIR la main humaine — warhost_init l'a remise
                                                            * à -1 ; sans ça, warhost_tick re-mobiliserait l'armée du
                                                            * joueur tout seul après un chargement (le generate, lui,
                                                            * le pose déjà — le load l'avait OUBLIÉ). */
    { const char *tstr=tune_active_string();               /* P0-3 : les tunables actifs ≠ ceux de la save ? */
      if ((uint32_t)scrypt_fnv1a(tstr, strlen(tstr)) != h.tune_ck)
          fprintf(stderr, "[save] AVERTISSEMENT : SCPS_TUNE actif ≠ celui de la sauvegarde — la partie évoluera "
                          "sous d'AUTRES règles (replays / graines partagées invalides).\n"); }
    s->ready=true;
    return 0;
}

/* F12 — capture d'écran (brief build §3) : on relit le framebuffer du renderer
 * en RGB top-down (l'ordre que stb attend) et on écrit un PNG horodaté dans
 * screenshots/. Utile au joueur, et à la boucle où une session de code regarde
 * ses propres rendus. */
static void viewer_screenshot(SDL_Renderer *ren){
    int w=0,h=0; SDL_GetRendererOutputSize(ren,&w,&h);
    if (w<=0||h<=0) return;
    unsigned char *px=(unsigned char*)malloc((size_t)w*h*3);
    if (!px) return;
    if (SDL_RenderReadPixels(ren,NULL,SDL_PIXELFORMAT_RGB24,px,w*3)==0){
        scps_mkdir("screenshots");
        char name[96]; time_t t=time(NULL);
        struct tm *lt=localtime(&t);
        if (lt) strftime(name,sizeof name,"screenshots/scps_%Y%m%d_%H%M%S.png",lt);
        else    snprintf(name,sizeof name,"screenshots/scps_%ld.png",(long)t);
        if (stbi_write_png(name,w,h,3,px,w*3)) printf("\n[scps] capture : %s\n",name);
    }
    free(px);
}

/* ── rendu du shell : écrans pleins + surcouches (pause · tuto · confirmation) ── */
static void shell_draw(SDL_Renderer *ren,int win_w,int win_h,World *w,Sim *s,
                       WorldParams *stage){
    shhit_reset();
    if (g_gs==GS_MENU){
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x07,0x0b,0x12,0xb8});   /* le monde respire derrière */
        draw_text(ren,g_font_big,win_w/2-44,win_h/4,COL_COPPER,"S C P S");
        draw_text(ren,g_font,win_w/2-150,win_h/4+26,COL_DIM,tr(STR_MENU_SOUS_TITRE));
        int bx=win_w/2-90, by=win_h/4+70;
        sh_button(ren,bx,by,180,tr(STR_MENU_JOUER),false,false,SH_MENU_ITEM,0); by+=34;
        { SaveHeader hh; bool any=false;
          for (int sl=1;sl<=3 && !any;sl++) any=save_slot_info(sl,&hh);
          sh_button(ren,bx,by,180,tr(STR_MENU_CHARGER),false,!any,SH_MENU_ITEM,1); by+=34; }
        sh_button(ren,bx,by,180,tr(STR_MENU_TUTORIEL),false,false,SH_MENU_ITEM,2); by+=34;
        sh_button(ren,bx,by,180,tr(STR_MENU_QUITTER),false,false,SH_MENU_ITEM,3); by+=34;
        { char lng[48]; tr_fmt(lng,sizeof lng,STR_MENU_LANGUE,lang_name(lang_get()));
          sh_button(ren,bx,by,180,lng,false,false,SH_MENU_ITEM,4); }   /* Options : FR/EN à chaud */
    }
    else if (g_gs==GS_SETUP){
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x0a,0x0e,0x16,0xf2});
        draw_text(ren,g_font_big,40,24,COL_COPPER,tr(STR_SETUP_TITRE));
        /* colonne MONDE */
        int x=60,y=70; char v[24];
        draw_text(ren,g_font,x,y,COL_COPPER,"Le monde"); y+=24;
        snprintf(v,24,"%d",stage->n_continents); y=sh_slider(ren,x,y,"Continents",v,0,"Le nombre de masses (1-8).");
        snprintf(v,24,"%.2f",stage->land_amount); y=sh_slider(ren,x,y,"Terres",v,1,"La part de terre émergée.");
        snprintf(v,24,"%.2f",stage->world_age);   y=sh_slider(ren,x,y,"Âge du monde",v,2,"Un monde vieux a le relief usé.");
        snprintf(v,24,"%.2f",stage->erosion);     y=sh_slider(ren,x,y,"Érosion",v,3,"L'eau qui ronge la pierre.");
        snprintf(v,24,"%.2f",stage->mountains);   y=sh_slider(ren,x,y,"Montagnes",v,4,"Le relief qui sépare et protège.");
        snprintf(v,24,"%.2f",stage->temperature); y=sh_slider(ren,x,y,"Température",v,5,"Des glaces aux fournaises.");
        snprintf(v,24,"%.2f",stage->humidity);    y=sh_slider(ren,x,y,"Humidité",v,6,"La pluie fait les forêts.");
        snprintf(v,24,"%d",stage->n_empires);     y=sh_slider(ren,x,y,"Empires",v,7,"Les couronnes rivales (2-15).");
        snprintf(v,24,"%d",stage->n_city_states); y=sh_slider(ren,x,y,"Cités-états",v,8,"Les cités libres (0-20).");
        y+=6; char sd[40]; snprintf(sd,40,"graine  %u",stage->seed);
        draw_text(ren,g_font,x,y,COL_DIM,sd);
        sh_button(ren,x+170,y-4,60,"dé",false,false,SH_SEED_DICE,0);
        /* colonne JOUEUR */
        int px=win_w/2+30, py=70;
        draw_text(ren,g_font,px,py,COL_COPPER,"Le joueur"); py+=24;
        draw_text(ren,g_font_small,px,py,COL_DIM,"Éthos"); py+=18;
        for (int e=0;e<6;e++){
            char lab[80]; snprintf(lab,80,"%s — %s",SH_ETHOS_N[e],SH_ETHOS_L[e]);
            bool on=(g_setup_ethos==e);
            fill_rect(ren,px,py,330,20,on?(SDL_Color){0x3a,0x2c,0x1a,0xff}:(SDL_Color){0x12,0x18,0x24,0xff});
            draw_text(ren,g_font_small,px+8,py+3,on?COL_COPPER:COL_PARCH,lab);
            shhit_add((SDL_Rect){px,py,330,20},SH_PICK_ETHOS,e); py+=22;
        }
        py+=8; draw_text(ren,g_font_small,px,py,COL_DIM,"Race"); py+=18;
        for (int r=0;r<(int)RACE_COUNT;r++){
            bool on=(g_setup_race==r);
            int cx2=px+(r%3)*112, cy2=py+(r/3)*24;
            fill_rect(ren,cx2,cy2,104,20,on?(SDL_Color){0x3a,0x2c,0x1a,0xff}:(SDL_Color){0x12,0x18,0x24,0xff});
            draw_text(ren,g_font_small,cx2+8,cy2+3,on?COL_COPPER:COL_PARCH,species_name((SpeciesArchetype)r));
            shhit_add((SDL_Rect){cx2,cy2,104,20},SH_PICK_RACE,r);
        }
        py+=56; draw_text(ren,g_font_small,px,py,COL_DIM,"Terre de départ (un vœu — jamais un mur)"); py+=18;
        for (int t=0;t<6;t++){
            bool on=(g_setup_terre==t);
            int cx2=px+(t%3)*112, cy2=py+(t/3)*24;
            fill_rect(ren,cx2,cy2,104,20,on?(SDL_Color){0x3a,0x2c,0x1a,0xff}:(SDL_Color){0x12,0x18,0x24,0xff});
            draw_text(ren,g_font_small,cx2+8,cy2+3,on?COL_COPPER:COL_PARCH,SH_TERRE_N[t]);
            shhit_add((SDL_Rect){cx2,cy2,104,20},SH_PICK_TERRE,t);
        }
        /* récapitulatif diégétique + Forger */
        char rec[200];
        snprintf(rec,200,"Un monde %s%s · %d empires · un peuple %s %s cherchant %s.",
                 stage->world_age<0.4f?"jeune":"vieux",
                 stage->mountains>0.6f?" et montagneux":"",
                 stage->n_empires, species_name((SpeciesArchetype)g_setup_race),
                 SH_ETHOS_N[g_setup_ethos],
                 g_setup_terre<5?SH_TERRE_N[g_setup_terre]:"sa chance");
        draw_text(ren,g_font,60,win_h-86,COL_DIM,rec);
        sh_button(ren,60,win_h-52,200,"[ Forger le monde ]",true,false,SH_FORGER,0);
        sh_button(ren,280,win_h-52,120,"Retour",false,false,SH_BACK,0);
    }
    else if (g_gs==GS_OPENING){
        int pw=560, px=(win_w-pw)/2, py=win_h/2-120;
        panel_bg(ren,px,py,pw,236);
        draw_text(ren,g_font_big,px+24,py+18,COL_COPPER,"Vous voilà. Une région. 4 000 âmes.");
        char l1[160];
        int nemp=0,ncit=0;
        for (int c2=0;c2<w->n_countries;c2++){
            if (c2==s->player) continue;
            if (w->country[c2].role==POLITY_CITY_STATE) ncit++;
            else if (w->country[c2].role!=POLITY_UNCLAIMED) nemp++;
        }
        snprintf(l1,160,"Autour de vous : %d empires, %d cités libres, et un monde qui ne vous attend pas.",
                 nemp, ncit);
        draw_text(ren,g_font,px+24,py+52,COL_PARCH,l1);
        if (g_open_terre_line[0]) draw_text(ren,g_font_small,px+24,py+76,COL_DIM,g_open_terre_line);
        draw_text(ren,g_font_big,px+24,py+108,COL_PARCH,"« Voici comment votre empire tombera. »");
        sh_button(ren,px+24, py+170,220,"[ Reroll le monde ]",false,false,SH_OPEN_REROLL,0);
        sh_button(ren,px+pw-24-180,py+170,180,"[ Commencer ]",true,false,SH_OPEN_GO,0);
        draw_text(ren,g_font_small,px+24,py+206,COL_DIM,"La partie s'ouvre EN PAUSE — votre premier geste sera Espace.");
    }
    /* surcouches (jouables aussi en partie) */
    if (g_pause_menu && g_gs==GS_PLAYING){
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x08,0x0e,0x99});
        int bx=win_w/2-100, by=win_h/2-80;
        panel_bg(ren,bx-20,by-20,240,228);
        draw_text(ren,g_font_big,bx,by-6,COL_COPPER,tr(STR_PAUSE_TITRE)); by+=30;
        sh_button(ren,bx,by,200,tr(STR_PM_REPRENDRE),false,false,SH_PM_ITEM,0); by+=32;
        sh_button(ren,bx,by,200,tr(STR_PM_SAUVER),false,false,SH_PM_ITEM,4); by+=32;
        sh_button(ren,bx,by,200,tr(STR_PM_TUTORIEL),false,false,SH_PM_ITEM,1); by+=32;
        sh_button(ren,bx,by,200,tr(STR_PM_MENU),false,false,SH_PM_ITEM,2); by+=32;
        sh_button(ren,bx,by,200,tr(STR_PM_QUITTER),false,false,SH_PM_ITEM,3);
    }
    /* §terrain — ÉCRAN DE DÉFAITE : sobre, deux sorties (Observer · Menu). */
    if (g_defeat){
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x06,0x09,0xcc});
        int bx=win_w/2-110, by=win_h/2-70;
        panel_bg(ren,bx-24,by-24,268,178);
        draw_text(ren,g_font_big,bx,by-8,(SDL_Color){0xe0,0x6a,0x4a,0xff},tr(STR_DEFAITE_TITRE)); by+=34;
        { char yz[16], ln[96]; snprintf(yz,sizeof yz,"%d",g_defeat_year);
          tr_fmt(ln,sizeof ln,STR_DEFAITE_LIGNE,yz);
          draw_text(ren,g_font,bx,by,COL_DIM,ln); } by+=36;
        sh_button(ren,bx,by,220,tr(STR_DEFAITE_OBSERVER),false,false,SH_DEFEAT,0); by+=34;
        sh_button(ren,bx,by,220,tr(STR_DEFAITE_MENU),false,false,SH_DEFEAT,1);
    }
    if (g_save_pick||g_load_pick){
        int pw=460, px=(win_w-pw)/2, py=win_h/2-90;
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x08,0x0e,0x99});
        panel_bg(ren,px,py,pw,180);
        draw_text(ren,g_font_big,px+20,py+12,COL_COPPER, g_save_pick?tr(STR_PICK_SAUVER):tr(STR_PICK_CHARGER));
        for (int sl=1;sl<=3;sl++){
            SaveHeader hh; bool has=save_slot_info(sl,&hh);
            char lab[140];
            { char num[16]; snprintf(num,sizeof num,"%d",sl);
              if (has && hh.version==SAVE_VERSION) tr_fmt(lab,sizeof lab,STR_SLOT_LINE,num,hh.line);
              else if (has)                        tr_fmt(lab,sizeof lab,STR_SLOT_ANCIEN,num);
              else                                 tr_fmt(lab,sizeof lab,STR_SLOT_VIDE,num); }
            bool grise = g_load_pick && (!has || hh.version!=SAVE_VERSION);
            sh_button(ren,px+20,py+46+(sl-1)*34,pw-40,lab,false,grise,
                      g_save_pick?SH_SLOT_SAVE:SH_SLOT_LOAD, sl);
        }
        sh_button(ren,px+pw-130,py+150,110,"Retour",false,false,SH_PICK_CLOSE,0);
    }
    if (g_load_confirm>=0){
        int pw=440, px=(win_w-pw)/2, py=win_h/2-50;
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x08,0x0e,0x99});
        panel_bg(ren,px,py,pw,110);
        draw_text(ren,g_font,px+20,py+16,COL_PARCH,"Charger ? La partie en cours sera perdue.");
        sh_button(ren,px+30,py+58,150,"Charger",false,false,SH_LOADC_YES,g_load_confirm);
        sh_button(ren,px+pw-30-150,py+58,150,"Rester",true,false,SH_LOADC_NO,0);
    }
    if (g_show_tuto){
        int pw=620, ph=240, px=(win_w-pw)/2, py=(win_h-ph)/2;
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x08,0x0e,0x88});
        panel_bg(ren,px,py,pw,ph);
        draw_text(ren,g_font_big,px+20,py+14,COL_COPPER,tr_band(STR_TUTO_TITLE_0,g_tuto_page,7));
        { const char *t=tr_band(STR_TUTO_PAGE_0,g_tuto_page,7); int ly=py+48; char line[200]; int li=0;
          for (const char *c2=t;;c2++){
              if (*c2=='\n'||*c2==0){ line[li]=0; draw_text(ren,g_font,px+20,ly,COL_PARCH,line); ly+=20; li=0; if(!*c2)break; }
              else if (li<198) line[li++]=*c2;
          } }
        char pg[24]; { char num[16]; snprintf(num,sizeof num,"%d",g_tuto_page+1);
                       tr_fmt(pg,sizeof pg,STR_TUTO_PAGEFMT,num); }
        draw_text(ren,g_font_small,px+pw/2-12,py+ph-26,COL_DIM,pg);
        if (g_tuto_page>0) sh_button(ren,px+16,py+ph-34,90,tr(STR_TUTO_PREC),false,false,SH_TUTO_PREV,0);
        if (g_tuto_page<6) sh_button(ren,px+pw-106,py+ph-34,90,tr(STR_TUTO_SUIV),false,false,SH_TUTO_NEXT,0);
        draw_text(ren,g_font_small,px+16,py+ph-12,COL_DIM,"ESC ferme");
    }
    if (g_quit_confirm){
        int pw=430, px=(win_w-pw)/2, py=win_h/2-60;
        fill_rect(ren,0,0,win_w,win_h,(SDL_Color){0x05,0x08,0x0e,0x99});
        panel_bg(ren,px,py,pw,120);
        draw_text(ren,g_font,px+20,py+16,COL_PARCH,"Quitter ? Toute progression non sauvée sera perdue.");
        sh_button(ren,px+30,py+62,150,"Quitter",false,false,SH_QC_YES,0);
        sh_button(ren,px+pw-30-150,py+62,150,"Rester",true,false,SH_QC_NO,0);
    }
}

/* Échec fatal au démarrage : une BOÎTE native. L'exe est -mwindows (aucune
 * console) — un « return 1 » muet ressemble à « ça ne se lance pas ». La boîte
 * SDL est best-effort (elle marche même si l'init vidéo a échoué). {0}=détail SDL. */
static void fatal_box(const char *detail) {
    char m[512];
    tr_fmt(m, sizeof m, STR_FATAL_SDL, detail ? detail : "");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, tr(STR_FATAL_TITRE), m, NULL);
    fprintf(stderr, "[scps] démarrage : %s\n", detail ? detail : "(inconnu)");
}

/* — Le worker de génération : façonne le monde puis l'amorce (3 ans). AUCUNE primitive
 *   SDL ici. Le thread principal ne TOUCHE pas world/sim tant que la phase n'est pas 2
 *   (SDL_WaitThread fait la barrière mémoire). Déterminisme inchangé : même calcul,
 *   simplement hors du thread d'affichage. */
typedef struct { World *w; WorldParams *p; Sim *s; } GenCtx;
static int SDLCALL gen_worker(void *data){
    GenCtx *g=(GenCtx*)data;
    g_gen_phase=0; world_generate(g->w, g->p);
    g_gen_phase=1; sim_rebuild(g->s, g->w);
    g_gen_phase=2;
    return 0;
}
/* L'écran de chargement : fond sombre, libellé de phase, barre de progression — réelle
 * pour l'amorce, balayage indéterminé pour le façonnage (qui n'a pas de jauge fine). */
static void loading_paint(SDL_Renderer *ren, int W, int H){
    fill_rect(ren, 0,0, W,H, (SDL_Color){0x0a,0x0d,0x14,0xff});
    int phase=g_gen_phase;
    const char *label=tr(phase==0 ? STR_LOADING_MONDE : STR_LOADING_EVEIL);
    draw_text(ren, g_font_big, (W-text_w(g_font_big,label))/2, H/2-48, COL_PARCH, label);
    int bw=W/3; if (bw<240) bw=240; int bh=18, bx=(W-bw)/2, by=H/2;
    fill_rect(ren, bx-2,by-2, bw+4, bh+4, COL_PANEL2);          /* cadre */
    fill_rect(ren, bx,by, bw,bh, COL_PANEL);                    /* fond de barre */
    if (phase==0){                                             /* indéterminé : un bloc qui va-et-vient */
        int seg=bw/5, span=bw-seg, pos=(int)((SDL_GetTicks()/6)%(unsigned)(2*span));
        if (pos>span) pos=2*span-pos;
        fill_rect(ren, bx+pos,by, seg,bh, COL_COPPER);
    } else {                                                    /* amorce : progression réelle (jours) */
        float frac=(float)g_gen_day/(float)GEN_BOOT_DAYS;
        if (frac<0.f) frac=0.f; else if (frac>1.f) frac=1.f;
        fill_rect(ren, bx,by, (int)(bw*frac),bh, COL_COPPER);
    }
    SDL_RenderPresent(ren);
}
int main(int argc, char **argv) {
    bool shot = false, shot_tree = false, shot_war = false, shot_culture = false, shot_sidebar = false, shot_council = false;
    bool shot_market = false;   /* #5 : capture du tiroir MARCHÉ (achat/vente direct) */
    bool shot_diplo = false;    /* M3 : capture du tiroir DIPLO (pacte commercial) */
    bool shot_political = false; float shot_zoom = 1.f;   /* N3.1 : capture vue Politique ± zoom */
    int  shot_shell = 0;
    bool savetest = false;
    bool fuzztest = false;      /* P0-1 bonus : forge des compteurs (save_sane les rejette) + fuzz d'octets (jamais de crash) */
    bool shot_cur = false;
    bool langshot = false;      /* loc §2 : preuve PPM du balisage inline (#tag…#!) + nombre groupé */
    uint32_t shot_seed = 0; bool have_shot_seed = false;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i], "--shot")) shot = true;
        else if (!strcmp(argv[i], "--tree")) { shot = true; shot_tree = true; }
        else if (!strcmp(argv[i], "--sidebar")) { shot = true; shot_sidebar = true; }   /* tiroir Stocks + lentille Marché */
        else if (!strcmp(argv[i], "--market")) { shot = true; shot_market = true; }      /* #5 : tiroir MARCHÉ (achat/vente) */
        else if (!strcmp(argv[i], "--diplo")) { shot = true; shot_diplo = true; }        /* M3 : tiroir DIPLO (pacte commercial) */
        else if (!strcmp(argv[i], "--council")) { shot = true; shot_council = true; }   /* Q1 : section ÉTAT (Conseil) */
        else if (!strcmp(argv[i], "--shellshot") && i+1<argc) { shot=true; shot_shell=1+atoi(argv[++i]); }  /* 1=menu 2=setup 3=ouverture */
        else if (!strcmp(argv[i], "--savetest")) savetest=true;   /* vérif sauvegarde : sauver-recharger = continuation identique */
        else if (!strcmp(argv[i], "--fuzztest")) fuzztest=true;   /* P0-1 : forge de compteurs + fuzz d'octets du save */
        else if (!strcmp(argv[i], "--curshot")) { shot=true; shot_cur=true; }   /* carte + champ des courants */
        else if (!strcmp(argv[i], "--war"))  { shot = true; shot_war  = true; }  /* §4 : capturer les armées sur la carte */
        else if (!strcmp(argv[i], "--culture")) { shot = true; shot_culture = true; }  /* §5 : vue culture */
        else if (!strcmp(argv[i], "--political")) { shot = true; shot_political = true; } /* N3.1 : hiérarchie 5/2 px mesurable */
        else if (!strcmp(argv[i], "--zoom") && i+1<argc) { shot_zoom = (float)atof(argv[++i]); } /* N3.1 : preuve zoom-stable */
        else if (!strcmp(argv[i], "--dump-lang")) {   /* écrit scps_lang.txt (tout le texte joueur, éditable) puis sort */
            int nw = lang_dump_file("scps_lang.txt");
            printf("[scps] scps_lang.txt écrit (%d entrées) — édite le texte, relance le jeu.\n", nw);
            return nw>0 ? 0 : 1;
        }
        else if (!strcmp(argv[i], "--dump-readout")) {   /* écrit scps_readout.txt (manifeste de la membrane : bandes/labels/hovers) puis sort */
            int nb = readout_dump_file("scps_readout.txt");
            printf("[scps] scps_readout.txt écrit (%d bandes) — manifeste de la membrane (outillage).\n", nb);
            return nb>0 ? 0 : 1;
        }
        else if (!strcmp(argv[i], "--lang-audit")) {  /* loc §3 : confronte un scps_lang.txt au set COMPILÉ (IDs périmés/manquants) */
            const char *path = (i+1<argc && argv[i+1][0]!='-') ? argv[++i] : "scps_lang.txt";
            int an = lang_audit_file(path, stdout);
            return an==0 ? 0 : 1;                      /* 0 = sain, sinon (anomalies ou absent) ≠ 0 */
        }
        else if (!strcmp(argv[i], "--dump-fnv")) {    /* loc §3 : manifeste d'empreintes (ID<TAB>hash<TAB>texte) puis sort */
            int nw = lang_dump_fingerprints("scps_lang.fnv");
            printf("[scps] scps_lang.fnv écrit (%d entrées).\n", nw);
            return nw>0 ? 0 : 1;
        }
        else if (!strcmp(argv[i], "--langshot")) { shot = true; langshot = true; }  /* loc §2 : preuve PPM balises + nombre groupé */
        else { shot_seed = (uint32_t)strtoul(argv[i], NULL, 10); have_shot_seed = true; }
    }
    g_iso = getenv("SCPS_ISO") ? 1 : 0;   /* vue isométrique : défaut OFF (toggle en jeu) */

    /* §SURCHARGE TEXTE : si scps_lang.txt est présent, il REMPLACE le texte joueur
     * (par ID). Absent → défauts compilés. Rupture assumée de zéro-asset, display-only. */
    { int nov = lang_load_file("scps_lang.txt");
      if (nov>0) printf("[scps] scps_lang.txt chargé : %d libellé(s) surchargé(s).\n", nov); }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fatal_box(SDL_GetError());
        return 1;
    }

    /* P0.1 — fenêtre à la taille de l'ÉCRAN au lancement (plein écran fenêtré,
     * respecte la barre des tâches) ; le layout suit la taille réelle (resize).
     * En mode capture (--shot), on garde WIN_W×WIN_H pour des images stables. */
    int initw = WIN_W, inith = WIN_H;
    /* Override de résolution (capture) : SCPS_WIN_W/H — utile pour des screens calibrés. */
    { const char *ew=getenv("SCPS_WIN_W"), *eh=getenv("SCPS_WIN_H");
      if (ew){ int v=atoi(ew); if (v>=640 && v<=4096) initw=v; }
      if (eh){ int v=atoi(eh); if (v>=480 && v<=4096) inith=v; } }
    Uint32 winflags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (!shot){
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm)==0){ initw=dm.w; inith=dm.h; }
        winflags |= SDL_WINDOW_MAXIMIZED;
    }
    SDL_Window *win = SDL_CreateWindow(
        "SCPS — Moteur de carte",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        initw, inith, winflags);
    if (!win) { fatal_box(SDL_GetError()); return 1; }
    /* Rendu : on PRÉFÈRE l'accéléré + vsync, mais on RETOMBE sur le logiciel.
     * Sans ce repli, une machine sans accélération GPU (machine virtuelle,
     * bureau distant, pilote graphique absent/obsolète) renvoyait ren=NULL →
     * sortie muette = le « ça ne se lance pas » signalé. Le logiciel marche
     * partout (CPU) : mieux vaut lent que rien. */
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);                     /* sans vsync */
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE); /* repli ultime */
    if (!ren) { fatal_box(SDL_GetError()); return 1; }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* La prise audio (§5) : ouvre un device si présent ; sinon MUET, sans erreur
     * (conteneur/serveur sans carte → le release tourne quand même). */
    if (!audio_init()) fprintf(stderr,"[scps] audio : aucun device — silence.\n");

    /* Pack MAP DRESSING (display-only, même régime éditable que scps_lang.txt) :
     * la planche scps_map_dressing.bmp à côté du binaire, fond MAGENTA FF00FF →
     * transparent par DESPILL (alpha + dé-teinte, voir plus bas). ABSENTE →
     * g_dress_tex NULL → carte lisse (le moteur/déterminisme n'y touchent jamais). */
    g_dress_tex = load_despilled_bmp(ren, SCPS_MAP_DRESSING_FILE);
    if (g_dress_tex) printf("[scps] %s chargé (décors de carte).\n", SCPS_MAP_DRESSING_FILE);
    else fprintf(stderr,"[scps] décors de carte : %s absent — carte lisse.\n", SCPS_MAP_DRESSING_FILE);
    g_settle_tex = load_despilled_bmp(ren, SCPS_SETTLE_FILE);
    if (g_settle_tex) printf("[scps] %s chargé (settlements).\n", SCPS_SETTLE_FILE);
    else fprintf(stderr,"[scps] settlements : %s absent.\n", SCPS_SETTLE_FILE);
    g_cover_tex = load_despilled_bmp(ren, SCPS_COVER_FILE);
    if (g_cover_tex) printf("[scps] %s chargé (route-cover).\n", SCPS_COVER_FILE);
    g_port_tex = load_despilled_bmp(ren, SCPS_PORT_FILE);
    if (g_port_tex) printf("[scps] %s chargé (ports orientés).\n", SCPS_PORT_FILE);
    /* Planches d'ANIMATION FX (display-only) — absentes ⇒ NULL ⇒ no-op (carte statique). */
    g_fx_sea_tex    = load_fx_bmp(ren, SCPS_FX_SEA_FILE);
    g_fx_coast_tex  = load_fx_bmp(ren, SCPS_FX_COAST_FILE);
    g_fx_army_tex   = load_fx_bmp(ren, SCPS_FX_ARMY_FILE);
    g_fx_vortex_tex = load_fx_bmp(ren, SCPS_FX_VORTEX_FILE);
    if (g_fx_sea_tex||g_fx_coast_tex||g_fx_army_tex||g_fx_vortex_tex)
        printf("[scps] FX animés : mer %s · côte %s · armée %s · vortex %s\n",
               g_fx_sea_tex?"on":"—", g_fx_coast_tex?"on":"—",
               g_fx_army_tex?"on":"—", g_fx_vortex_tex?"on":"—");
    softblob_build(ren);                  /* glow doux pour le raccord mer des villes côtières */
#ifdef SCPS_DEV
    dev_overlay_init(win, ren);   /* §6 : l'overlay de dev (F3) — build -DSCPS_DEV seul */
#endif

    /* Police diégétique (SDL_ttf) — DejaVu couvre les accents français. */
    if (TTF_Init() != 0) fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    static const char *font_paths[] = {
        "DejaVuSans.ttf",                /* police BUNDLÉE à côté de l'exe (Windows/portable) */
        "fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/Library/Fonts/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf",    /* repli système Windows (couvre les accents FR) */
    };
    for (size_t i=0; i<sizeof(font_paths)/sizeof(font_paths[0]) && !g_font; i++) {
        g_font     = TTF_OpenFont(font_paths[i], 14);
        g_font_big = TTF_OpenFont(font_paths[i], 18);
        g_font_small = TTF_OpenFont(font_paths[i], 10);
    }
    if (!g_font) fprintf(stderr, "[scps] police introuvable — panneau sans texte\n");

    /* §LANGSHOT (loc §2/§1) — preuve autonome, headless : on peint sur fond noir
     * (1) un nombre GROUPÉ via tr_fmt {0|n}, (2) le MÊME mot en clair puis balisé
     * `#hot …#!`, et on VÉRIFIE par les pixels que (a) les deux versions ont la
     * MÊME largeur (les marqueurs ne s'affichent pas) et (b) la version balisée
     * porte la teinte « hot » (rouge) là où la claire est en cuivre. Écrit
     * scps_langshot.ppm + un verdict, puis sort — aucun monde généré. */
    if (langshot){
        int LW=560, LH=160;
        SDL_Texture *rt = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_TARGET, LW, LH);
        if (!rt){ fprintf(stderr,"[langshot] pas de cible de rendu\n"); return 1; }
        SDL_SetRenderTarget(ren, rt);
        SDL_SetRenderDrawColor(ren, 0x0a,0x0e,0x16,0xff);
        SDL_RenderClear(ren);

        /* (1) nombre groupé — via une surcharge runtime à spec, sur une clé porteuse. */
        char gbuf[64];
        lang_dump_file(".langshot_lang.txt");                  /* base éditable */
        { FILE *f=fopen(".langshot_lang.txt","ab");            /* on AJOUTE l'override à spec */
          if (f){ fputs("STR_DIPLO_SCORE_FMT\tan-0 : {0|n} habitants\n", f); fclose(f); } }
        lang_clear_overrides(); lang_load_file(".langshot_lang.txt");
        tr_fmt(gbuf,sizeof gbuf, STR_DIPLO_SCORE_FMT, "48000");
        draw_text(ren, g_font, 16, 14, COL_COPPER, gbuf);      /* doit lire « an-0 : 48 000 habitants » */
        lang_clear_overrides();                                /* on revient aux défauts compilés */

        /* (2) plain vs balisé — même contenu, pour la mesure de largeur. Littéraux
         * HORS du site draw_text (variables) : c'est de l'outillage, pas de l'UI
         * livrée — le cliquet lang-check n'a pas à les compter. */
        const char *plain  = "Marche engorge";
        const char *tagged = "#hot Marche engorge#!";
        const char *coldln = "flux #cold -9 500#! ce trimestre";
        int wp = text_w(g_font, plain);                        /* largeur de référence (sans balise) */
        draw_text(ren, g_font, 16, 56, COL_COPPER, plain);
        draw_text(ren, g_font, 16, 92, COL_COPPER, tagged);    /* le segment doit virer au rouge, sans marqueurs */
        draw_text(ren, g_font, 16, 122, COL_PARCH, coldln);    /* un froid (bleu) à côté, pour l'image */

        /* lecture des pixels : largeur réelle de la ligne balisée + présence du rouge. */
        uint32_t *px = (uint32_t*)malloc((size_t)LW*LH*4);
        int verdict_w=0, verdict_red=0;
        if (px && SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, px, LW*4)==0){
            /* largeur de l'encre sur la rangée balisée (y≈92..104) : dernière colonne non-fond */
            int last_plain=0, last_tag=0;
            for (int y=58; y<74; y++) for (int x2=16; x2<LW; x2++){
                uint32_t c=px[y*LW+x2]; if ((c&0xffffff)!=0x0a0e16 && (c&0xffffff)!=0) if (x2>last_plain) last_plain=x2;
            }
            for (int y=94; y<110; y++) for (int x2=16; x2<LW; x2++){
                uint32_t c=px[y*LW+x2]; if ((c&0xffffff)!=0x0a0e16 && (c&0xffffff)!=0) if (x2>last_tag) last_tag=x2;
            }
            /* les deux fins d'encre doivent coïncider (à 3 px près) : marqueurs invisibles */
            verdict_w = (last_plain>0 && abs(last_tag-last_plain)<=3);
            /* présence d'un pixel franchement ROUGE (R≫G,B) sur la rangée balisée */
            for (int y=94; y<110 && !verdict_red; y++) for (int x2=16; x2<LW; x2++){
                uint32_t c=px[y*LW+x2];
                int r=(c>>16)&0xff, g=(c>>8)&0xff, b=c&0xff;
                if (r>120 && r> g+40 && r> b+40){ verdict_red=1; break; }
            }
            /* PPM (RGB) pour l'œil */
            FILE *f=fopen("scps_langshot.ppm","wb");
            if (f){ fprintf(f,"P6\n%d %d\n255\n",LW,LH);
                for (int i2=0;i2<LW*LH;i2++){ uint32_t c=px[i2];
                    unsigned char rgb[3]={(unsigned char)((c>>16)&0xff),(unsigned char)((c>>8)&0xff),(unsigned char)(c&0xff)};
                    fwrite(rgb,1,3,f); }
                fclose(f); }
        }
        free(px);
        SDL_SetRenderTarget(ren, NULL);
        remove(".langshot_lang.txt");
        printf("[langshot] nombre groupe   : \"%s\"\n", gbuf);
        printf("[langshot] largeur plain=%d (reference text_w=%d)\n", verdict_w, wp);
        printf("[langshot] marqueurs invisibles (largeur claire==balisee) : %s\n", verdict_w?"OUI":"NON");
        printf("[langshot] segment colore en rouge (#hot) detecte         : %s\n", verdict_red?"OUI":"NON");
        printf("[langshot] image : scps_langshot.ppm\n");
        return (verdict_w && verdict_red) ? 0 : 1;
    }

    /* Simulation branchée sous la carte (alimente bandeau + panneau). */
    Sim sim = {0};
    sim.econ = (WorldEconomy*)    malloc(sizeof(WorldEconomy));
    sim.wp   = (WorldProsperity*) malloc(sizeof(WorldProsperity));
    sim.wl   = (WorldLegitimacy*) malloc(sizeof(WorldLegitimacy));
    sim.net  = (TradeNetwork*)    malloc(sizeof(TradeNetwork));
    sim.ts   = (TechState*)       calloc(SCPS_MAX_COUNTRY, sizeof(TechState));
    sim.sc   = (Statecraft*)      malloc(sizeof(Statecraft));
    sim.ag   = (AgencyState*)     malloc(sizeof(AgencyState));
    sim.ev   = (EventsState*)     malloc(sizeof(EventsState));
    sim.drift= (ModifierStack*)   malloc(sizeof(ModifierStack));
    sim.labor= (LaborEcon*)       malloc(sizeof(LaborEcon));
    sim.dp   = (DiploState*)      malloc(sizeof(DiploState));
    sim.rn   = (RouteNetwork*)    malloc(sizeof(RouteNetwork));
    sim.rs   = (RevoltState*)     malloc(sizeof(RevoltState));
    sim.host = (WarHost*)         malloc(sizeof(WarHost));
    sim.camp = (Campaign*)        malloc(sizeof(Campaign));
    sim.missions = (MissionsState*) malloc(sizeof(MissionsState));
    sim.navy = (NavyState*)       malloc(sizeof(NavyState));
    sim.ai   = (AiActor*)         calloc(SCPS_MAX_COUNTRY, sizeof(AiActor));
    sim.ai_on= (bool*)            calloc(SCPS_MAX_COUNTRY, sizeof(bool));
    sim.eg   = (EndgameState*)    calloc(1, sizeof(EndgameState));

    int win_w = WIN_W, win_h = WIN_H;
    SDL_GetWindowSize(win, &win_w, &win_h);            /* P0.1 : taille RÉELLE (maximisée) */
    if (win_w < 640) win_w = WIN_W;                    /* garde-fous */
    if (win_h < 480) win_h = WIN_H;
    PixBuf pb = pixbuf_create(ren, win_w, win_h);
    PixBuf mm_pb = pixbuf_create(ren, MM_W, MM_H);   /* §5 : la minicarte (taille fixe) */

    World *world = (World*)malloc(sizeof(World));
    if (!world) { fprintf(stderr,"OOM\n"); return 1; }

    uint32_t  seed     = have_shot_seed ? shot_seed : (uint32_t)time(NULL);
    WorldParams params = worldparams_default(seed);
    ViewMode  mode     = VIEW_TERRAIN;
    GameSpeed  speed   = SPEED_1;        /* le temps coule (Espace = pause) */
    double     day_accum = 0.0;
    uint32_t   last_ticks = SDL_GetTicks();
    int       selected = -1;
    bool      show_tree = false;     /* superposition : l'arbre de tech (Tab) */
    bool      dirty    = true;
    bool      running  = true;
    bool      regen    = false;   /* demande de régénération du monde */

    /* Caméra : ajuste pour montrer toute la carte */
    Cam cam;
    cam_fit(&cam, win_w, win_h);

    /* Paramètres de rendu */
    RenderParams rp = {
        .cam_ox = cam.ox, .cam_oy = cam.oy, .cam_scale = cam.scale,
        .selected_prov = -1,
        .show_rivers = true, .show_borders = true, .show_grid = false
    };

    /* Pan à la souris */
    bool  panning = false;
    int   rdown_x = 0, rdown_y = 0;       /* P0.3 : position du clic-droit (clic vs glissé) */
    int   pan_sx = 0, pan_sy = 0;

    printf("[scps] Génération (graine %u)…\n", seed);
    if (shot){
        world_generate(world, &params);
        sim_rebuild(&sim, world);   /* capture headless : synchrone, pas d'écran de chargement */
    } else {
        /* Genèse + amorce sur un THREAD : le thread principal reste réactif — il pompe
         * les évènements (plus de « ne répond pas ») et peint la progression. */
        GenCtx gctx = { world, &params, &sim };
        SDL_Thread *gth = SDL_CreateThread(gen_worker, "scps-genese", &gctx);
        if (!gth){ world_generate(world,&params); sim_rebuild(&sim,world); }   /* repli : pas de thread → synchrone */
        else {
            bool gen_quit=false;
            while (g_gen_phase < 2 && !gen_quit){
                SDL_Event e;
                while (SDL_PollEvent(&e))
                    if (e.type==SDL_QUIT) gen_quit=true;
                int ow=win_w, oh=win_h; SDL_GetRendererOutputSize(ren,&ow,&oh);
                loading_paint(ren, ow, oh);
                SDL_Delay(16);
            }
            if (gen_quit) return 0;          /* fermé pendant la genèse : le process sort (worker tué proprement à l'exit) */
            SDL_WaitThread(gth, NULL);        /* fini : on récupère le worker (barrière → world/sim visibles) */
        }
    }
    g_gs = shot ? GS_PLAYING : GS_MENU;      /* le jeu COMMENCE au menu (le monde respire derrière) */
    g_stage = params;

    /* ── --savetest : LA VÉRIF du brief (4) — sauver puis recharger restitue la
     * partie AU JOUR PRÈS : on avance N jours, on sauve, on avance M jours (digest A) ;
     * on recharge, on ré-avance M jours (digest B) ; A doit ÉGALER B. ── */
    if (savetest){
        #define DIGEST(tag) do{ double dpop=0,dgld=0; long dtech=0; unsigned long downer=5381; \
            for (int r=0;r<sim.econ->n_regions;r++){ const RegionEconomy *re=&sim.econ->region[r]; \
                for (int c2=0;c2<CLASS_COUNT;c2++) dpop+=re->strata[c2].pop; \
                dgld+=re->treasury; downer=downer*33+(unsigned long)(re->owner+2); } \
            for (int c2=0;c2<SCPS_MAX_COUNTRY;c2++) dtech+=sim.ts[c2].n_unlocked; \
            snprintf(tag,sizeof tag,"day=%d pop=%.1f or=%.1f tech=%ld own=%lu pays=%d frondes=%d", \
                     sim.day,dpop,dgld,dtech,downer,world->n_countries,sim.dp->n_frondes); }while(0)
        char dA[200], dB[200];
        for (int d2=0;d2<600;d2++) sim_day(&sim, world);
        if (!game_save(3, world, &sim, &params)){ printf("savetest: ÉCHEC d'écriture\n"); return 1; }
        for (int d2=0;d2<400;d2++) sim_day(&sim, world);
        DIGEST(dA);
        int rc=game_load(3, world, &sim, &params);
        if (rc!=0){ printf("savetest: ÉCHEC de lecture (%d)\n", rc); return 1; }
        for (int d2=0;d2<400;d2++) sim_day(&sim, world);
        DIGEST(dB);
        bool same = (strcmp(dA,dB)==0);
        /* 2e contrôle : un octet ALTÉRÉ au milieu du payload chiffré → REFUS net. */
        bool tamper_ok=false;
        { FILE *tf=fopen(save_slot_path(3),"r+b");
          if (tf){ SaveHeader th;
            if (fread(&th,sizeof th,1,tf)==1){
                long mid=(long)sizeof th + (long)th.payload/2;
                fseek(tf,mid,SEEK_SET); int c2=fgetc(tf);
                fseek(tf,mid,SEEK_SET); fputc(c2^0x5A,tf);
            }
            fclose(tf);
            tamper_ok = (game_load(3, world, &sim, &params)!=0);   /* doit ÉCHOUER */
          } }
        printf("A: %s\nB: %s\n  altération d'un octet → %s\n"
               "══════════════════════════════════════\n BILAN : %d réussis, %d échoués\n",
               dA, dB, tamper_ok?"REFUSÉE (empreinte)":"ACCEPTÉE (BUG)",
               (same?1:0)+(tamper_ok?1:0), (same?0:1)+(tamper_ok?0:1));
        return (same&&tamper_ok)?0:1;
    }

    /* ── --fuzztest : LE DURCISSEMENT « contrat public » du save (audit P0-1, bonus). (1) chaque COMPTEUR/
     * INDEX désérialisé, forgé HORS-BORNE, doit être REJETÉ par save_sane (le vecteur d'écriture hors-bornes) ;
     * (2) un FUZZ d'octets du fichier (en-tête + payload) : game_load doit TOUJOURS rendre proprement — jamais
     * planter (un OOB serait attrapé sous ASan). Headless : SDL_VIDEODRIVER=dummy ./scps_viewer --fuzztest 9. ── */
    if (fuzztest){
        for (int d2=0; d2<365*5; d2++) sim_day(&sim, world);   /* de l'état RÉEL : ordres, armées, révoltes */
        int ok=0, ko=0;
        #define FZ(cond,msg) do{ if (cond) ok++; else { ko++; printf("  ✗ %s\n",(msg)); } }while(0)
        FZ(save_sane(world,&sim,sim.player), "sim valide accepté par save_sane");
        { int v=sim.ag->n; sim.ag->n=SCPS_MAX_BUILDS+1; FZ(!save_sane(world,&sim,sim.player), "agency.n hors-borne REJETÉ"); sim.ag->n=v; }
        { int v=sim.ag->n; sim.ag->n=-1;                FZ(!save_sane(world,&sim,sim.player), "agency.n négatif REJETÉ");   sim.ag->n=v; }
        if (sim.ag->n>0){ int v=sim.ag->order[0].region; sim.ag->order[0].region=sim.econ->n_regions+9;
            FZ(!save_sane(world,&sim,sim.player), "ordre.region OOB REJETÉ (le vecteur purge_slice)"); sim.ag->order[0].region=v; }
        { int v=sim.rs->count; sim.rs->count=REVOLT_MAX+5; FZ(!save_sane(world,&sim,sim.player), "revolt.count hors-borne REJETÉ"); sim.rs->count=v; }
        { int v=sim.camp->army[0].force.n_units; sim.camp->army[0].force.n_units=ARMY_MAX_UNITS+1;
            FZ(!save_sane(world,&sim,sim.player), "camp army.n_units hors-borne REJETÉ"); sim.camp->army[0].force.n_units=v; }
        { int v=sim.host->army[0].n_units; sim.host->army[0].n_units=ARMY_MAX_UNITS+13;
            FZ(!save_sane(world,&sim,sim.player), "host army.n_units hors-borne REJETÉ"); sim.host->army[0].n_units=v; }
        long flips=0;
        if (game_save(3, world, &sim, &params)){
            const char *fp=save_slot_path(3);
            long fsz=0; { FILE *g=fopen(fp,"rb"); if(g){ fseek(g,0,SEEK_END); fsz=ftell(g); fclose(g); } }
            long hdr=(long)sizeof(SaveHeader);
            /* tout l'EN-TÊTE octet par octet (le parsing brut — magic/version/payload/nonce/ck) + un
             * petit échantillon du payload (lequel est de toute façon protégé par l'empreinte FNV). */
            long limit = (fsz < hdr+2048) ? fsz : hdr+2048;
            for (long b=0; b<limit; b += (b<hdr?1:128)){
                FILE *g=fopen(fp,"r+b"); if(!g) break;
                fseek(g,b,SEEK_SET); int c=fgetc(g);
                fseek(g,b,SEEK_SET); fputc(c^0xFF,g); fclose(g);
                (void)game_load(3, world, &sim, &params);   /* doit RENDRE (0/1/2) — jamais planter */
                flips++;
                FILE *g2=fopen(fp,"r+b"); if(g2){ fseek(g2,b,SEEK_SET); fputc(c,g2); fclose(g2); }   /* restaure l'octet */
            }
            FZ(flips>0, "fuzz d'octets exécuté (game_load a toujours rendu — aucun crash)");
        } else FZ(0, "game_save a écrit le fichier de fuzz");
        #undef FZ
        printf("  (%ld octets flippés ; save_sane a rejeté chaque forge ; aucun crash)\n", flips);
        printf("══════════════════════════════════════\n BILAN : %d réussis, %d échoués\n", ok, ko);
        return ko?1:0;
    }
    printf("[scps] Prêt. TAB/1-0=vues  T=arbre de tech  E/D/S/A/F=sidebar (éco·démo·stocks·armée·filtres)  Z=cadrer  R=regénère  clic=territoire\n");
    printf("[scps] Réglages (régénèrent) : c=continents g=âge e=érosion\n");
    printf("       l=terres m=montagnes t=température h=humidité (Maj=baisse)\n");

    /* Mode capture (--shot) : une frame (carte + bandeau + panneau sur une
     * province peuplée), sérialisée en PPM, puis sortie — vérifie l'UI sans écran. */
    if (shot) {
        if (shot_tree || shot_sidebar) for (int d=0; d<60*365; d++) sim_day(&sim, world);  /* laisse le monde POUSSER */
        int cid = country_for_panel(world, -1);
        int pcap = (cid>=0 && cid<world->n_countries) ? world->country[cid].capital_prov : -1;
        selected = (pcap>=0) ? pcap : 0;
        /* en capture zoomée : on CENTRE sur la capitale (terre garantie, biomes variés)
         * avant la photo caméra → le zoom reste centré là (sinon : centre géographique,
         * souvent l'océan). Pan only — display-only, hors zoom le plein-cadre est intact. */
        if (shot_zoom > 1.f && selected>=0 && selected<world->n_provinces){
            cam.ox = (float)world->province[selected].seed_x - win_w/(2.f*cam.scale);
            cam.oy = (float)world->province[selected].seed_y - win_h/(2.f*cam.scale);
        }
        if (shot_zoom > 1.f) cam_zoom(&cam, shot_zoom, win_w*0.5f, win_h*0.5f);  /* N3.1 : zoom AVANT la photo caméra */
        rp.cam_ox=cam.ox; rp.cam_oy=cam.oy; rp.cam_scale=cam.scale; rp.selected_prov=selected;
        g_iso_w=win_w; g_iso_h=win_h; rp.iso=g_iso;   /* vue iso (SCPS_ISO en capture) */
        SDL_RenderClear(ren);
        if (shot_cur) {
            rp.region_tint=NULL;
            render_map(world, pb.pixels, pb.w, pb.h, &rp, VIEW_TERRAIN); pixbuf_upload(&pb);
            if (pb.tex) SDL_RenderCopy(ren, pb.tex, NULL, NULL);
            for (int sy=8; sy<win_h-8; sy+=12) for (int sx=8; sx<win_w-8; sx+=12){
                float _wx,_wy; cam_unproject(&cam,(float)sx,(float)sy,&_wx,&_wy);
                int cx2=(int)_wx, cy2=(int)_wy;
                if (cx2<0||cy2<0||cx2>=SCPS_W||cy2>=SCPS_H) continue;
                const Cell *cc=scps_cellc(world,cx2,cy2);
                if (cc->sea<=SEA_CABOTAGE) continue;
                float vx2=cc->cur_vx/100.f, vy2=cc->cur_vy/100.f;
                float m=sqrtf(vx2*vx2+vy2*vy2); if (m<0.10f) continue;
                float l=(cc->sea==SEA_COURANT)?9.f:5.f;
                SDL_Color cl=(cc->sea==SEA_COURANT)?COL_COPPER:(SDL_Color){0x6a,0x8a,0xb0,0xff};
                SDL_SetRenderDrawColor(ren,cl.r,cl.g,cl.b,(cc->sea==SEA_COURANT)?0xE0:0x90);
                SDL_RenderDrawLine(ren,sx,sy,sx+(int)(vx2/m*l),sy+(int)(vy2/m*l));
            }
        } else if (shot_shell && sim.ready && g_font) {
            ViewMode smode0=VIEW_TERRAIN; rp.region_tint=NULL;
            render_map(world, pb.pixels, pb.w, pb.h, &rp, smode0); pixbuf_upload(&pb);
            if (pb.tex) SDL_RenderCopy(ren, pb.tex, NULL, NULL);
            zone_reset();
            g_gs = (shot_shell==1)?GS_MENU:(shot_shell==2)?GS_SETUP:GS_OPENING;
            if (g_gs==GS_OPENING){ snprintf(g_open_terre_line,sizeof g_open_terre_line,
                "Nulle plaine libre — vous voilà sur la côte."); }
            shell_draw(ren,win_w,win_h,world,&sim,&g_stage);
        } else if (shot_tree && sim.ready && g_font) {
            g_tree_open = TECH_CONSCRIPTION;                                  /* démo : un anneau de sous-techs ouvert (à gauche, loin du survol) */
            draw_tech_tree(ren, win_w, win_h, sim.econ, sim.ts, world, sim.rn, cid);   /* l'arbre concentrique du pays */
            if (g_tree_demo>=0){                                              /* démo : un survol (boîte 2 colonnes) */
                draw_box(ren, g_tree_x[g_tree_demo]-9, g_tree_y[g_tree_demo]-9, 18,18, COL_PARCH);
                draw_hover_footer(ren, win_w, win_h, g_tree_x[g_tree_demo], g_tree_y[g_tree_demo]);
            }
        } else {
            if (shot_war)                    /* §4 : laisse une guerre mûrir → des armées sur la carte */
                for (int y=0; y<120 && !any_field_army(&sim, world); y++)
                    for (int d=0; d<365; d++) sim_day(&sim, world);
            ViewMode smode = shot_culture ? VIEW_CULTURE
                           : shot_political ? VIEW_POLITICAL : VIEW_COUNTRIES;
            rp.region_tint = NULL;
            if (shot_sidebar){                       /* démo sidebar : tiroir Stocks + lentille Marché */
                g_sb.tab=SBT_STOCKS; g_sb.anim=1.f; g_sb.lens=LENS_MARCHE;
                static uint32_t lt[SCPS_MAX_REG];
                map_lens_tints(sim.econ, sim.wl, g_sb.lens, lt);
                rp.region_tint = lt; smode = VIEW_CULTURE;
            } else if (shot_market){                 /* #5 : démo MARCHÉ — tiroir ouvert (étage via SCPS_MARCHE_SUB, défaut régional) */
                g_sb.tab=SBT_MARCHE; g_sb.anim=1.f; g_sb.marche_sub=1;
                const char *ms=getenv("SCPS_MARCHE_SUB"); if (ms){ int v=atoi(ms); if(v>=0&&v<=2) g_sb.marche_sub=v; }
                if (g_sb.marche_sub==2 && !intertrade_country_has_centre(sim.econ, sim.player)){
                    for (int r=0;r<sim.econ->n_regions;r++) if (intertrade_has_centre(r)){ sim.player=sim.econ->region[r].owner; break; }
                    g_sbc.day=-1;   /* démo : joueur tenant un Centre → l'étage GLOBAL s'ouvre */
                }
            } else if (shot_diplo){                  /* M3 : démo DIPLO — un pacte SIGNÉ pour montrer l'affichage */
                g_sb.tab=SBT_DIPLO; g_sb.anim=1.f;
                for (int c=0;c<world->n_countries;c++)
                    if (c!=sim.player && world->country[c].role!=POLITY_UNCLAIMED
                        && diplo_status(sim.dp,sim.player,c)!=DIPLO_WAR){
                        diplo_set_trade_pact(sim.dp, sim.player, c, true); break;   /* 1 pacte actif (le reste « pas de pacte ») */
                    }
            } else if (shot_council){                /* Q1 : démo Conseil — un siège POURVU, deux vacants */
                int best=0,bt=0; for(int sl=0;sl<SC_COUNCIL_CANDS;sl++){ int t=statecraft_council_cand_tier(world->seed,sim.player,0,sl); if(t>bt){bt=t;best=sl;} }
                statecraft_council_hire(sim.sc, sim.player, 0, best);   /* siège Savoir pourvu (montre « Renvoyer ») */
                g_sb.tab=SBT_ETAT; g_sb.anim=1.f;
            } else if (smode==VIEW_CULTURE){
                static uint32_t tnt[SCPS_MAX_REG];
                for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++)
                    tnt[r]=ethos_tint((int)sim.econ->region[r].culture.ethos);
                rp.region_tint = tnt;
            } else if (smode==VIEW_COUNTRIES && sim.ready){    /* P1.6 : owner courant, non-colonisé = terrain */
                static uint32_t ot[SCPS_MAX_REG];
                for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++){
                    int ow=sim.econ->region[r].owner;
                    ot[r]=(ow>=0 && ow<world->n_countries && sim.econ->region[r].colonized)? world->country[ow].color : 0u;
                }
                rp.region_tint = ot;
            }
            /* §terrain : la capture montre aussi l'OCCUPATION (hachure de l'occupant). */
            rp.occupier_tint = NULL;
            { static uint32_t g_shot_occ[SCPS_MAX_REG]; bool anyo=false;
              for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++){
                  int occ=sim.dp->occupier[r];
                  if (occ>=0 && occ<world->n_countries){ g_shot_occ[r]=world->country[occ].color; anyo=true; }
                  else g_shot_occ[r]=0u; }
              if (anyo) rp.occupier_tint = g_shot_occ; }
            rp.screen_strokes = (smode==VIEW_POLITICAL||smode==VIEW_REGIONS||smode==VIEW_COUNTRIES);  /* N3.1 */
            render_map(world, pb.pixels, pb.w, pb.h, &rp, smode);
            pixbuf_upload(&pb);
            if (pb.tex) SDL_RenderCopy(ren, pb.tex, NULL, NULL);
            if (sim.ready) { draw_map_rivers(ren, world, &cam, win_w, win_h); draw_map_roads(ren, world, sim.rn, &cam, win_w, win_h); draw_map_settlements(ren, world, sim.econ, &cam, win_w, win_h); draw_map_dressing(ren, world, sim.econ, &cam, win_w, win_h, 0); draw_map_dressing(ren, world, sim.econ, &cam, win_w, win_h, 1); }   /* calques : routes → villes → habillage → canopées, SOUS les frontières */
            if (sim.ready) borders_draw(ren, &cam, world, &sim, smode, selected, win_w, win_h);  /* N3.1 : la preuve par capture */
            if (mm_pb.pixels){ RenderParams mmp=rp; mmp.selected_prov=-1; mmp.screen_strokes=false; mmp.iso=false;  /* minicarte : top-down */
                minimap_fit(&mmp.cam_scale,&mmp.cam_ox,&mmp.cam_oy);
                render_map(world, mm_pb.pixels, mm_pb.w, mm_pb.h, &mmp, smode); pixbuf_upload(&mm_pb); }
            if (sim.ready && g_font) {
                zone_reset(); bslot_reset(); orow_reset(); modebtn_reset(); topbtn_reset(); g_colonize_dst=-1; g_reloc_dst=-1; g_comptoir_reg=-1; g_center_reg=-1;
                draw_empire_labels(ren, &cam, &sim, world, win_w, win_h);  /* P1.8 : noms d'empire au dézoom */
                draw_army_markers(ren, &cam, &sim, world, win_w, win_h);   /* §4 : les armées sur la carte */
                draw_centre_markers(ren, &cam, &sim, world, win_w, win_h);  /* P3.20 : les Centres commerciaux */
                draw_topbar(ren, win_w, &sim, world, cid, speed);
                draw_outliner(ren, win_w, win_h, &sim, world, selected);            /* §6 : l'outliner */
                draw_mode_buttons(ren, win_h, smode);                     /* §5 : modes de carte */
                draw_minimap(ren, &mm_pb, win_w, win_h, &cam);            /* §5 : la minicarte */
                draw_province_panel(ren, win_w, win_h, world, sim.econ, sim.wp, sim.wl, sim.drift, selected, &sim);
                if (shot_sidebar || shot_council || shot_market || shot_diplo) draw_sidebar(ren, win_w, win_h, &sim, world, smode, selected);
            }
        }
        SDL_RenderPresent(ren);
        uint32_t *cap = (uint32_t*)malloc((size_t)win_w*win_h*4);
        if (cap && SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, cap, win_w*4)==0)
            save_ppm("scps_ui.ppm", cap, win_w, win_h);
        else fprintf(stderr, "[scps] capture impossible\n");
        free(cap);
        running = false;
    }

    while (running) {
        SDL_Event ev;
#ifdef SCPS_DEV
        dev_overlay_input_begin();
#endif
        while (SDL_PollEvent(&ev)) {
#ifdef SCPS_DEV
            if (g_dev_overlay && ev.key.keysym.sym!=SDLK_F3
                && dev_overlay_handle_event(&ev)) continue;   /* Nuklear a la main (sauf le toggle F3) */
#endif
            switch (ev.type) {

            case SDL_QUIT: g_quit_confirm=true; dirty=true; break;   /* la croix passe par la CONFIRMATION */

            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    win_w = ev.window.data1;
                    win_h = ev.window.data2;
                    pixbuf_destroy(&pb);
                    pb = pixbuf_create(ren, win_w, win_h);
                    dirty = true;
                }
                break;

            case SDL_MOUSEWHEEL: {
                int mx, my; SDL_GetMouseState(&mx, &my);
                /* molette sur un contrôle SBH_PROD_CAP → ajuste le limiteur (pas de scroll) */
                bool pcap_handled=false;
                if (sim.ready && g_sb.tab==SBT_STOCKS){
                    for (int i=0;i<g_nsbhits&&!pcap_handled;i++){
                        SbHit *hh=&g_sbhits[i]; if (hh->kind!=SBH_PROD_CAP) continue;
                        SDL_Rect *r=&hh->r;
                        if (mx>=r->x&&mx<r->x+r->w&&my>=r->y&&my<r->y+r->h){
                            int step=(SDL_GetModState()&KMOD_SHIFT)?100:10;
                            float cur=econ_prod_cap(sim.player, hh->a);
                            float nxt;
                            if (ev.wheel.y>0){ nxt=(cur<0.f)?step:(cur+step); }
                            else { if (cur<0.f) nxt=0.f; else { nxt=cur-step; if(nxt<0.f) nxt=-1.f; } }
                            econ_set_prod_cap(sim.player, hh->a, nxt);
                            pcap_handled=true; dirty=true;
                        }
                    }
                }
                if (pcap_handled) break;
                if (sidebar_wheel(mx, my, ev.wheel.y)) { dirty=true; break; }   /* le tiroir défile (hit-test d'abord) */
                float factor = (ev.wheel.y > 0) ? 1.25f : 0.80f;
                cam_zoom(&cam, factor, (float)mx, (float)my);
                dirty = true;
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_MIDDLE ||
                    ev.button.button == SDL_BUTTON_RIGHT) {
                    panning = true;
                    pan_sx = ev.button.x;
                    pan_sy = ev.button.y;
                    rdown_x = ev.button.x; rdown_y = ev.button.y;   /* P0.3 : départ (clic vs glissé) */
                } else if (ev.button.button == SDL_BUTTON_LEFT) {
                    /* LE SHELL capte d'abord (menu/création/ouverture + surcouches). */
                    if (g_quit_confirm||g_show_tuto||g_pause_menu||g_save_pick||g_load_pick||g_load_confirm>=0||g_defeat||g_gs!=GS_PLAYING){
                        int hit=-1;
                        for (int i2=0;i2<g_nshhits;i2++){ SDL_Rect *r2=&g_shhits[i2].r;
                            if (ev.button.x>=r2->x&&ev.button.x<r2->x+r2->w&&ev.button.y>=r2->y&&ev.button.y<r2->y+r2->h){ hit=i2; break; } }
                        if (hit>=0){
                            ShellHit *sh2=&g_shhits[hit];
                            switch(sh2->kind){
                                case SH_QC_YES: running=false; break;
                                case SH_PICK_CLOSE: g_save_pick=g_load_pick=0; break;
                                case SH_SLOT_SAVE:
                                    if (game_save(sh2->a, world, &sim, &params))
                                        printf("\n[scps] Sauvé — slot %d.\n", sh2->a);
                                    else printf("\n[scps] Échec d'écriture (slot %d).\n", sh2->a);
                                    g_save_pick=0;
                                    break;
                                case SH_SLOT_LOAD:
                                    if (g_game_started){ g_load_confirm=sh2->a; g_load_pick=0; }
                                    else {
                                        int rc=game_load(sh2->a, world, &sim, &params);
                                        if (rc==0){ seed=params.seed; g_load_pick=0; g_pause_menu=false;
                                            g_game_started=true; speed=SPEED_PAUSE; g_sbc.day=-1; selected=-1;
                                            sh_center_capital(world,&sim,&cam,win_w,win_h);
                                            g_gs=GS_PLAYING;
                                            g_defeat=false; g_observer=false;          /* §terrain : save d'un joueur péri → défaite */
                                            if (sim.player>=0 && sim.player<world->n_countries
                                                && world->country[sim.player].role==POLITY_UNCLAIMED){
                                                g_defeat=true; g_defeat_year=sim.year; } }
                                        else printf("\n[scps] %s\n", rc==2?"Sauvegarde d'une ère antérieure — refusée poliment.":"Slot illisible.");
                                    }
                                    break;
                                case SH_LOADC_YES: {
                                    int rc=game_load(sh2->a, world, &sim, &params);
                                    if (rc==0){ seed=params.seed; g_pause_menu=false; speed=SPEED_PAUSE;
                                        g_sbc.day=-1; selected=-1; g_game_started=true;
                                        sh_center_capital(world,&sim,&cam,win_w,win_h); g_gs=GS_PLAYING;
                                        /* §terrain : une save où le joueur a péri → ré-poser la défaite. */
                                        g_defeat=false; g_observer=false;
                                        if (sim.player>=0 && sim.player<world->n_countries
                                            && world->country[sim.player].role==POLITY_UNCLAIMED){
                                            g_defeat=true; g_defeat_year=sim.year; } }
                                    else printf("\n[scps] %s\n", rc==2?"Sauvegarde d'une ère antérieure — refusée poliment.":"Slot illisible.");
                                    g_load_confirm=-1;
                                } break;
                                case SH_LOADC_NO: g_load_confirm=-1; break;
                                case SH_QC_NO:  g_quit_confirm=false; break;
                                case SH_TUTO_PREV: if(g_tuto_page>0)g_tuto_page--; break;
                                case SH_TUTO_NEXT: if(g_tuto_page<6)g_tuto_page++; break;
                                case SH_DEFEAT:
                                    if (sh2->a==0){ g_defeat=false; g_observer=true; } /* OBSERVER : le monde continue, lecture seule */
                                    else { g_defeat=false; g_observer=false; g_gs=GS_MENU; speed=SPEED_PAUSE; }  /* MENU */
                                    break;
                                case SH_MENU_ITEM:
                                    if (sh2->a==0){ g_stage=params; g_gs=GS_SETUP; }
                                    else if (sh2->a==1){ g_load_pick=1; }
                                    else if (sh2->a==2){ g_show_tuto=true; g_tuto_page=0; }
                                    else if (sh2->a==3) g_quit_confirm=true;
                                    else if (sh2->a==4) lang_set(lang_get()==LANG_FR?LANG_EN:LANG_FR);
                                    break;
                                case SH_SLIDER_DN: sh_apply_slider(&g_stage,sh2->a,-1); break;
                                case SH_SLIDER_UP: sh_apply_slider(&g_stage,sh2->a,+1); break;
                                case SH_SEED_DICE: g_stage.seed ^= (uint32_t)SDL_GetTicks()*2654435761u; if(!g_stage.seed)g_stage.seed=1u; break;
                                case SH_PICK_ETHOS: g_setup_ethos=sh2->a; break;
                                case SH_PICK_RACE:  g_setup_race=sh2->a; g_player_race=(SpeciesArchetype)sh2->a; break;
                                case SH_PICK_TERRE: g_setup_terre=sh2->a; break;
                                case SH_BACK: g_gs=GS_MENU; break;
                                case SH_FORGER:
                                    params=g_stage; seed=params.seed;
                                    sh_draw_litanie(ren,win_w,win_h,params.seed); SDL_RenderPresent(ren);
                                    regen=true; g_pending_open=true;
                                    break;
                                case SH_OPEN_REROLL:
                                    params.seed ^= (uint32_t)SDL_GetTicks()*2654435761u; if(!params.seed)params.seed=1u;
                                    seed=params.seed;
                                    sh_draw_litanie(ren,win_w,win_h,params.seed); SDL_RenderPresent(ren);
                                    regen=true; g_pending_open=true;
                                    break;
                                case SH_OPEN_GO: g_gs=GS_PLAYING; g_game_started=true; g_defeat=false; g_observer=false; break;   /* la pause TIENT : Espace sera le premier geste */
                                case SH_PM_ITEM:
                                    if (sh2->a==0) g_pause_menu=false;
                                    else if (sh2->a==4){ g_save_pick=1; }
                                    else if (sh2->a==1){ g_show_tuto=true; g_tuto_page=0; }
                                    else if (sh2->a==2){ g_pause_menu=false; g_gs=GS_MENU; speed=SPEED_PAUSE; }
                                    else g_quit_confirm=true;
                                    break;
                            }
                        }
                        dirty=true; break;
                    }
                    if (g_diplo_target>=0){ g_diplo_target=-1; dirty=true; }   /* P0.3 : un clic-gauche ferme le popup diplo */
                    /* cran de VITESSE cliquable (topbar) */
                    if (g_gs==GS_PLAYING && ev.button.x>=g_speed_zone.x && ev.button.x<g_speed_zone.x+g_speed_zone.w
                        && ev.button.y>=g_speed_zone.y && ev.button.y<g_speed_zone.y+g_speed_zone.h){
                        speed=(GameSpeed)((speed+1)%SPEED_COUNT);
                        if (speed!=SPEED_PAUSE) g_last_speed=speed;
                        dirty=true; break;
                    }
                    /* P0.4 — chevron : replier / déplier le Domaine (droite) */
                    if (g_gs==GS_PLAYING && sim.ready && !show_tree &&
                        ev.button.x>=g_right_chevron.x && ev.button.x<g_right_chevron.x+g_right_chevron.w &&
                        ev.button.y>=g_right_chevron.y && ev.button.y<g_right_chevron.y+g_right_chevron.h){
                        g_right_hidden = !g_right_hidden; dirty=true; break;
                    }
                    /* SIDEBAR d'abord (rail + tiroir) : la décision capte le clic. */
                    if (sim.ready && !show_tree) {
                        bool sbd=false;
                        if (sidebar_click(&sim, world, ev.button.x, ev.button.y, &mode, &sbd)){
                            if (sbd) g_sbc.day=-1;           /* décision → recalcul des agrégats */
                            dirty=true; break;
                        }
                    }
                    /* ARBRE OUVERT : un clic sur une tech ouvre/ferme l'anneau de ses
                     * SOUS-TECHS (clic ailleurs = ferme). L'écran de l'arbre capte le clic. */
                    if (show_tree) {
                        int hit=-1;
                        for (int i=0;i<TECH_COUNT;i++){
                            if (!g_tree_x[i] && !g_tree_y[i]) continue;        /* nœud non positionné (base) */
                            int dx=ev.button.x-g_tree_x[i], dy=ev.button.y-g_tree_y[i];
                            if (dx*dx+dy*dy <= 11*11){ hit=i; break; }
                        }
                        /* P5.26 — clic sur une tech ACCESSIBLE → la recherche SE LANCE (file de 1). */
                        if (hit>=0 && sim.ready && sim.player>=0 && sim.player<world->n_countries){
                            unsigned acc = ai_race_access(world, sim.econ, sim.rn, sim.player);
                            if (tech_can_research(&sim.ts[sim.player], (TechId)hit, acc)){
                                g_research_target=hit; sim.ts[sim.player].research_points=0.f;
                            }
                        }
                        g_tree_open = (hit>=0 && hit!=g_tree_open) ? hit : -1;  /* bascule l'anneau de sous-techs */
                        dirty=true; break;
                    }
                    /* §1 : le bandeau est un SOMMAIRE — un clic sur une RESSOURCE ouvre
                     * son système. Le Savoir ouvre l'ARBRE DE TECH (qui existe) ; les
                     * autres écrans (finances, subsistance, chaînes, diplomatie) sont à
                     * venir — le clic les annonce. */
                    int tb=-1;
                    for (int i=0;i<g_ntopbtns;i++){ SDL_Rect *r=&g_topbtns[i].r;
                        if (ev.button.x>=r->x && ev.button.x<r->x+r->w &&
                            ev.button.y>=r->y && ev.button.y<r->y+r->h){ tb=i; break; } }
                    if (tb>=0){
                        /* le bandeau est un SOMMAIRE : chaque ressource ouvre SON panneau de la sidebar */
                        switch (g_topbtns[tb].sys){
                            case SYS_TECH:        show_tree = !show_tree; g_tree_open = -1; break;
                            case SYS_FINANCES:    g_sb.tab=SBT_ECO;    g_sb.eco_sub=1; break;  /* Or → Marché */
                            case SYS_SUBSISTANCE: g_sb.tab=SBT_DEMO;   break;                   /* Nourriture → Démographie */
                            case SYS_CHAINES:     g_sb.tab=SBT_STOCKS; break;                   /* Matériaux → Stocks */
                            default:              g_sb.tab=SBT_ECO;    g_sb.eco_sub=0; break;  /* Diplomatie → Commerce/embargo */
                        }
                        dirty=true; break;
                    }
                    /* §5 : un clic sur un BOUTON DE MODE change la vue de carte. */
                    int mb=-1;
                    for (int i=0;i<g_nmodebtns;i++){ SDL_Rect *r=&g_modebtns[i].r;
                        if (ev.button.x>=r->x && ev.button.x<r->x+r->w &&
                            ev.button.y>=r->y && ev.button.y<r->y+r->h){ mb=i; break; } }
                    if (mb>=0){ mode=(ViewMode)g_modebtns[mb].mode; dirty=true; break; }
                    /* §5 : un clic sur la MINICARTE recentre la caméra. */
                    { SDL_Rect m; minimap_rect(win_w,win_h,&m);
                      if (ev.button.x>=m.x && ev.button.x<m.x+m.w && ev.button.y>=m.y && ev.button.y<m.y+m.h){
                          float sc,ox,oy; minimap_fit(&sc,&ox,&oy);
                          float wx=(ev.button.x-m.x)/sc+ox, wy=(ev.button.y-m.y)/sc+oy;
                          cam.ox = wx - win_w/(2.f*cam.scale); cam.oy = wy - win_h/(2.f*cam.scale);
                          dirty=true; break;
                      } }
                    /* P2.14 — clic « Coloniser » : prélève l'or sur la capitale, 100 colons
                     * de la région la plus peuplée (econ_colonize_from) ; la région devient nue. */
                    if (g_colonize_dst>=0 && g_colonize_src>=0 && sim.ready &&
                        ev.button.x>=g_colonize_btn.x && ev.button.x<g_colonize_btn.x+g_colonize_btn.w &&
                        ev.button.y>=g_colonize_btn.y && ev.button.y<g_colonize_btn.y+g_colonize_btn.h){
                        /* E0.3 : l'or sort du TRÉSOR UNIQUE (topbar). E1 : le convoi
                         * MARCHE 180 jours — la région ne se peuple qu'à l'arrivée. */
                        if (credit_can_spend(sim.econ, world, sim.player, (float)COLONIZE_GOLD_COST)
                            && agency_order_colonize(sim.ag, g_colonize_dst, g_colonize_src)){
                            credit_spend(sim.econ, world, sim.player, (float)COLONIZE_GOLD_COST);
                            printf("\n[scps] Coloniser : convoi parti vers la région %d (100 colons, %.0f or, 180 j).\n", g_colonize_dst, COLONIZE_GOLD_COST);
                        } else printf("\n[scps] Coloniser : trésor insuffisant (%.0f or requis).\n", COLONIZE_GOLD_COST);
                        dirty=true; break;
                    }
                    /* P3.20 — RELOCALISER un Centre commercial vers la région sélectionnée :
                     * le hub le plus proche du joueur S'Y DÉPLACE (il ne meurt pas), payé en or. */
                    if (g_reloc_dst>=0 && sim.ready &&
                        ev.button.x>=g_reloc_btn.x && ev.button.x<g_reloc_btn.x+g_reloc_btn.w &&
                        ev.button.y>=g_reloc_btn.y && ev.button.y<g_reloc_btn.y+g_reloc_btn.h){
                        int from=intertrade_country_centre(sim.econ, sim.player);
                        /* E0.3 : payé du TRÉSOR UNIQUE (topbar). */
                        if (from>=0 && credit_can_spend(sim.econ, world, sim.player, (float)CENTRE_RELOC_COST)
                            && intertrade_relocate_centre(sim.econ, from, g_reloc_dst)){
                            credit_spend(sim.econ, world, sim.player, (float)CENTRE_RELOC_COST);
                            printf("\n[scps] Centre commercial relocalisé : région %d → %d (%.0f or).\n", from, g_reloc_dst, CENTRE_RELOC_COST);
                        } else printf("\n[scps] Relocalisation refusée (trésor insuffisant ou pas de hub).\n");
                        dirty=true; break;
                    }
                    /* E2 §10 — clic « Bâtir un Comptoir » : payé du trésor unique. */
                    if (g_comptoir_reg>=0 && sim.ready &&
                        ev.button.x>=g_comptoir_btn.x && ev.button.x<g_comptoir_btn.x+g_comptoir_btn.w &&
                        ev.button.y>=g_comptoir_btn.y && ev.button.y<g_comptoir_btn.y+g_comptoir_btn.h){
                        if (agency_build_acct(sim.ag, sim.econ, world, g_comptoir_reg, EDI_COMPTOIR,
                                              sim.player))
                            printf("\n[scps] Comptoir mis en chantier (région %d, 180 j) — la province se branche au réseau.\n", g_comptoir_reg);
                        else
                            printf("\n[scps] Comptoir : trésor insuffisant.\n");
                        dirty=true; break;
                    }
                    /* M2 — clic « Bâtir un Centre commercial » : payé du trésor unique. */
                    if (g_center_reg>=0 && sim.ready &&
                        ev.button.x>=g_center_btn.x && ev.button.x<g_center_btn.x+g_center_btn.w &&
                        ev.button.y>=g_center_btn.y && ev.button.y<g_center_btn.y+g_center_btn.h){
                        if (agency_build_acct(sim.ag, sim.econ, world, g_center_reg, EDI_TRADE_CENTER,
                                              sim.player))
                            printf("\n[scps] Centre commercial mis en chantier (région %d, 540 j) — l'empire se fait hub du réseau global.\n", g_center_reg);
                        else
                            printf("\n[scps] Centre commercial : trésor insuffisant ou recette manquante.\n");
                        dirty=true; break;
                    }
                    /* §4 panneau : un clic sur un SLOT de bâtiment bâtit l'édifice
                     * (payé au marché, en jours) — pas de bouton « Bâtir ». */
                    int hit=-1;
                    for (int i=0;i<g_nbslots;i++){ SDL_Rect *r=&g_bslots[i].r;
                        if (ev.button.x>=r->x && ev.button.x<r->x+r->w &&
                            ev.button.y>=r->y && ev.button.y<r->y+r->h){ hit=i; break; } }
                    if (hit>=0){
                        /* E0.3 : l'or du chantier sort du TRÉSOR UNIQUE (la topbar dit vrai). */
                        if (sim.ready && agency_build_acct(sim.ag, sim.econ, world, g_bslots[hit].reg,
                                                           g_bslots[hit].edifice, sim.player))
                            printf("\n[scps] Bâtir %s (région %d) — payé au marché, construit en jours.\n",
                                   edifice_name(g_bslots[hit].edifice), g_bslots[hit].reg);
                        else
                            printf("\n[scps] Bâtir : trésor insuffisant pour les matériaux.\n");
                        dirty = true;
                        break;
                    }
                    /* Note armée : clic sur « Remplir » → recomplète l'armée (territoire ami). */
                    if (g_refill_owner>=0 &&
                        ev.button.x>=g_refill_btn.x && ev.button.x<g_refill_btn.x+g_refill_btn.w &&
                        ev.button.y>=g_refill_btn.y && ev.button.y<g_refill_btn.y+g_refill_btn.h){
                        int added = campaign_refill(sim.camp, g_refill_owner, sim.econ, sim.labor);
                        printf("\n[scps] Remplir : +%d paquet(s) levé(s) (territoire ami, payé au marché).\n", added);
                        dirty=true; break;
                    }
                    /* §6 OUTLINER : clic sur le marteau → bâtir ; clic sur la ligne → saut. */
                    int orhit=-1;
                    for (int i=0;i<g_norows;i++){
                        SDL_Rect *hm=&g_orows[i].ham;
                        if (g_orows[i].hammer_reg>=0 && hm->w>0 &&
                            ev.button.x>=hm->x && ev.button.x<hm->x+hm->w &&
                            ev.button.y>=hm->y && ev.button.y<hm->y+hm->h){
                            if (sim.ready) agency_build_acct(sim.ag, sim.econ, world, g_orows[i].hammer_reg,
                                                             EDI_TRIBUNAL, sim.player);
                            dirty=true; orhit=-2; break;
                        }
                        SDL_Rect *rw=&g_orows[i].row;
                        if (ev.button.x>=rw->x && ev.button.x<rw->x+rw->w &&
                            ev.button.y>=rw->y && ev.button.y<rw->y+rw->h){ orhit=i; break; }
                    }
                    if (orhit==-2) break;                       /* marteau cliqué */
                    if (orhit>=0){ selected = g_orows[orhit].prov;   /* P1.11 : sélectionne ET CENTRE la région */
                        if (selected>=0 && selected<world->n_provinces){
                            cam.ox=(float)world->province[selected].seed_x - (float)win_w/(2.f*cam.scale);
                            cam.oy=(float)world->province[selected].seed_y - (float)win_h/(2.f*cam.scale);
                        }
                        dirty=true; break; }
                    /* P0.2 — clic CARTE seulement HORS panneau (priorité panneau > carte) */
                    if (over_panel(ev.button.x, ev.button.y, win_w, win_h, selected)) break;
                    /* Sinon : sélectionner la province au clic (détail province) */
                    float _wx,_wy; cam_unproject(&cam,(float)ev.button.x,(float)ev.button.y,&_wx,&_wy);
                    int cx=(int)_wx, cy=(int)_wy;
                    if (cx>=0&&cx<SCPS_W&&cy>=0&&cy<SCPS_H) {
                        int p = (int)scps_cellc(world, cx, cy)->province;
                        if (p != selected) {
                            selected = p;
                            if (p >= 0) print_province_info(world, p);
                        } else {
                            selected = -1;
                        }
                        dirty = true;
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_MIDDLE ||
                    ev.button.button == SDL_BUTTON_RIGHT)
                    panning = false;
                /* P0.3 — CLIC-DROIT (sans glissé) sur une région ÉTRANGÈRE, hors panneau →
                 * panneau diplomatique (squelette : nom, race, relation, statut). */
                if (ev.button.button == SDL_BUTTON_RIGHT && g_gs==GS_PLAYING && sim.ready
                    && abs(ev.button.x-rdown_x)<5 && abs(ev.button.y-rdown_y)<5
                    && !over_panel(ev.button.x,ev.button.y,win_w,win_h,selected)){
                    float _wx,_wy; cam_unproject(&cam,(float)ev.button.x,(float)ev.button.y,&_wx,&_wy);
                    int cx=(int)_wx, cy=(int)_wy;
                    g_diplo_target=-1;
                    if (cx>=0&&cy>=0&&cx<SCPS_W&&cy<SCPS_H){
                        int rg=(int)scps_cellc(world,cx,cy)->region;
                        if (rg>=0 && rg<sim.econ->n_regions){
                            int ow=sim.econ->region[rg].owner;
                            if (ow>=0 && ow!=sim.player && ow<world->n_countries) g_diplo_target=ow;
                        }
                    }
                    dirty=true;
                }
                break;

            case SDL_MOUSEMOTION:
                if (panning) {
                    cam_pan(&cam, (float)(ev.motion.x - pan_sx),
                                  (float)(ev.motion.y - pan_sy));
                    pan_sx = ev.motion.x;
                    pan_sy = ev.motion.y;
                    dirty = true;
                }
                break;

            case SDL_KEYDOWN:
                /* réglage worldgen ±0.25 (Maj = baisser) — utilisé par g/l/m/h et Ctrl+e/t */
                #define ADJ(field) { bool dn=(ev.key.keysym.mod&KMOD_SHIFT); \
                    params.field += dn?-0.25f:0.25f; \
                    if(params.field<-0.001f)params.field=1.f; \
                    else if(params.field>1.001f)params.field=0.f; \
                    regen=true; }
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                    /* L'ÉCHELLE ESC : fermer la couche du dessus — JAMAIS quitter l'appli. */
                    if      (g_diplo_target>=0){ g_diplo_target=-1; }   /* P0.3 : ferme le popup diplo */
                    else if (g_quit_confirm){ g_quit_confirm=false; }
                    else if (g_load_confirm>=0){ g_load_confirm=-1; }
                    else if (g_save_pick||g_load_pick){ g_save_pick=g_load_pick=0; }
                    else if (g_show_tuto)   { g_show_tuto=false; }
                    else if (g_pause_menu)  { g_pause_menu=false; }
                    else if (g_gs==GS_SETUP){ g_gs=GS_MENU; }
                    else if (g_gs==GS_OPENING){ g_gs=GS_MENU; }
                    else if (g_gs==GS_MENU) { /* rien : on ne quitte que par le bouton */ }
                    else if (show_tree)     { show_tree=false; g_tree_open=-1; }
                    else if (g_sb.tab>=0)   { g_sb.tab=-1; }
                    else { g_pause_menu=true; g_last_speed=(speed==SPEED_PAUSE)?g_last_speed:speed; speed=SPEED_PAUSE; }
                    dirty=true; break;
                /* --- Contrôle du TEMPS (§1) : Espace = pause ; +/- = vitesse --- */
                case SDLK_SPACE:
                    if (g_gs!=GS_PLAYING||g_pause_menu||g_show_tuto||g_quit_confirm) break;
                    if (speed==SPEED_PAUSE) speed=g_last_speed;            /* on repart sur SON cran */
                    else { g_last_speed=speed; speed=SPEED_PAUSE; }
                    break;
                case SDLK_t:     /* T = l'Arbre de Tech (Ctrl+T = réglage température) */
                    if (SDL_GetModState() & KMOD_CTRL) { ADJ(temperature) }
                    else { show_tree = !show_tree; g_tree_open = -1; }
                    break;
                /* --- SIDEBAR (§sidebar) : E/D/S/A/F = onglets d'empire --- */
                case SDLK_e:
                    if (SDL_GetModState() & KMOD_CTRL) { ADJ(erosion) }
                    else { g_sb.tab=(g_sb.tab==SBT_ECO)?-1:SBT_ECO; dirty=true; }
                    break;
                case SDLK_d:     g_sb.tab=(g_sb.tab==SBT_DEMO)?-1:SBT_DEMO;     dirty=true; break;
                case SDLK_s:     g_sb.tab=(g_sb.tab==SBT_STOCKS)?-1:SBT_STOCKS; dirty=true; break;
                case SDLK_a:     g_sb.tab=(g_sb.tab==SBT_ARMEE)?-1:SBT_ARMEE;   dirty=true; break;
                case SDLK_PLUS: case SDLK_EQUALS: case SDLK_KP_PLUS:
                    if (speed<SPEED_5) speed++;
                    if (speed==SPEED_PAUSE) speed=SPEED_1;
                    if (speed!=SPEED_PAUSE) g_last_speed=speed;
                    break;
                case SDLK_MINUS: case SDLK_KP_MINUS:
                    if (speed>SPEED_1) speed--;
                    if (speed!=SPEED_PAUSE) g_last_speed=speed;
                    break;
                /* --- ACTION (§4) : bâtir, via la couche d'agency, en JOURS --- */
                case SDLK_b:
                    if (sim.ready && selected>=0 && selected<world->n_provinces) {
                        int reg = world->province[selected].region;
                        if (reg>=0 && agency_build_acct(sim.ag, sim.econ, world, reg, EDI_TRIBUNAL, sim.player))
                            printf("\n[scps] Action : Tribunal mis en file (région %d) — payé au marché, construit en jours.\n", reg);
                        else
                            printf("\n[scps] Tribunal : trésor insuffisant pour acheter les matériaux.\n");
                    }
                    break;
                case SDLK_F4: {  /* §SURCHARGE : recharge scps_lang.txt À CHAUD (édition temps-réel du texte) */
                    lang_clear_overrides();
                    int nov = lang_load_file("scps_lang.txt");
                    printf("[scps] scps_lang.txt rechargé : %d libellé(s) surchargé(s).\n", nov>0?nov:0);
                    dirty=true; break;
                }
                case SDLK_F12:   viewer_screenshot(ren); break;   /* capture PNG horodatée */
#ifdef SCPS_DEV
                case SDLK_F3:    g_dev_overlay=!g_dev_overlay; break;   /* §6 : l'inspecteur brut */
#endif
                case SDLK_TAB:   mode=(ViewMode)((mode+1)%VIEW_COUNT); dirty=true; printf("\n"); break;
                case SDLK_1:     mode=VIEW_TERRAIN;     dirty=true; break;
                case SDLK_2:     mode=VIEW_POLITICAL;   dirty=true; break;
                case SDLK_3:     mode=VIEW_REGIONS;     dirty=true; break;
                case SDLK_4:     mode=VIEW_COUNTRIES;   dirty=true; break;
                case SDLK_5:     mode=VIEW_CONTINENTS;  dirty=true; break;
                case SDLK_6:     mode=VIEW_HEIGHT;      dirty=true; break;
                case SDLK_7:     mode=VIEW_FERTILITY;   dirty=true; break;
                case SDLK_8:     mode=VIEW_MOISTURE;    dirty=true; break;
                case SDLK_9:     mode=VIEW_TEMPERATURE; dirty=true; break;
                case SDLK_0:     mode=VIEW_RESOURCES;    dirty=true; break;
                case SDLK_i:     mode=VIEW_HABITABILITY; dirty=true; break;
                case SDLK_f:     g_sb.tab=(g_sb.tab==SBT_FILTRES)?-1:SBT_FILTRES; dirty=true; break;
                case SDLK_z:     cam_fit(&cam,win_w,win_h); dirty=true; break;   /* Z = cadrer la carte */
                case SDLK_o:     g_iso=!g_iso; dirty=true; break;   /* O = bascule vue ISOMÉTRIQUE (oblique) */
                case SDLK_r:
                    seed ^= (uint32_t)time(NULL) * 2654435761u;
                    params.seed = seed;
                    regen = true;
                    break;

                /* --- Réglages de génération (Maj = diminuer) — e/t passés sous Ctrl (sidebar/arbre) --- */
                case SDLK_c: {  /* nombre de continents 1..6 */
                    bool dn = (ev.key.keysym.mod & KMOD_SHIFT);
                    params.n_continents += dn?-1:1;
                    if (params.n_continents<1) params.n_continents=6;
                    if (params.n_continents>6) params.n_continents=1;
                    regen=true; break;
                }
                case SDLK_g:  ADJ(world_age)   break;  /* âge du monde   */
                case SDLK_l:  ADJ(land_amount) break;  /* quantité terre */
                case SDLK_m:  ADJ(mountains)   break;  /* relief         */
                case SDLK_h:  ADJ(humidity)    break;  /* humidité       */
                #undef ADJ
                default: break;
                }
                break;
            }
        }
#ifdef SCPS_DEV
        dev_overlay_input_end();
#endif

        if (regen) {
            printf("\n[scps] Génération — graine %u · continents %d · âge %.2f"
                   " · érosion %.2f · terres %.2f · relief %.2f · temp %.2f · humid %.2f\n",
                   params.seed, params.n_continents, params.world_age, params.erosion,
                   params.land_amount, params.mountains, params.temperature, params.humidity);
            world_generate(world, &params);
            sim_rebuild(&sim, world);
            selected = -1; dirty = true; regen = false;
            if (g_pending_open){ g_pending_open=false;
                sh_enter_opening(world,&sim,&cam,win_w,win_h,&speed); }
        }

        /* --- LE TEMPS COULE : la partie avance selon la vitesse (§1) --- */
        {
            uint32_t now = SDL_GetTicks();
            double frame_dt = (now - last_ticks) / 1000.0; last_ticks = now;
            if (frame_dt > 0.25) frame_dt = 0.25;               /* anti spirale de la mort */
            if (sim.ready && speed != SPEED_PAUSE && sim.year < GAME_YEARS) {
                day_accum += frame_dt * DAYS_PER_SEC[speed];
                int steps=0;
                while (day_accum >= 1.0 && steps < 40) { sim_day(&sim, world); day_accum -= 1.0; steps++; }
                if (steps>0) dirty = true;                      /* propriété/overlays peuvent changer */
                /* §terrain — LA DÉFAITE : le royaume du joueur réduit à 0 région (polity_death
                 * a posé role=UNCLAIMED). On fige une fois ; « Observer » lève le verrou. */
                if (!g_defeat && !g_observer && sim.player>=0 && sim.player<world->n_countries
                    && world->country[sim.player].role==POLITY_UNCLAIMED){
                    g_defeat=true; g_defeat_year=sim.year; speed=SPEED_PAUSE; dirty=true;
                }
            } else day_accum = 0.0;
        }

        if (dirty && pb.pixels) {
            rp.cam_ox = cam.ox; rp.cam_oy = cam.oy; rp.cam_scale = cam.scale;
            g_iso_w=win_w; g_iso_h=win_h; rp.iso=g_iso;   /* vue iso (toggle) */
            rp.selected_prov = selected;
            rp.region_tint = NULL;
            ViewMode rmode = mode;
            if (g_sb.lens!=LENS_NONE && sim.ready) {
                /* LENTILLE readout (§6) : la membrane calcule les teintes (bandes
                 * discrètes) ; le viewer ne fait que les poser sur la carte. */
                static uint32_t g_lens_tint[SCPS_MAX_REG];
                map_lens_tints(sim.econ, sim.wl, g_sb.lens, g_lens_tint);
                rp.region_tint = g_lens_tint;
                rmode = VIEW_CULTURE;                       /* le chemin de rendu teinté par région */
            } else if ((mode==VIEW_CULTURE || mode==VIEW_FAITH) && sim.ready) {
                static uint32_t g_region_tint[SCPS_MAX_REG];
                for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++){
                    const PopCulture *cu=&sim.econ->region[r].culture;
                    g_region_tint[r] = (mode==VIEW_CULTURE) ? ethos_tint((int)cu->ethos)
                                                            : faith_tint((int)cu->rel_branch);
                }
                rp.region_tint = g_region_tint;
            } else if ((mode==VIEW_COUNTRIES || mode==VIEW_TERRAIN) && sim.ready) {
                /* P1.6 — carte politique : teinte par OWNER COURANT (race-famille),
                 * non-colonisé = 0 → terrain nu. P1.7 — en vue RELIEF, render_map n'en
                 * garde QUE le liseré de frontière (la souveraineté se lit sur le relief). */
                static uint32_t g_owner_tint[SCPS_MAX_REG];
                for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++){
                    int ow=sim.econ->region[r].owner;
                    g_owner_tint[r] = (ow>=0 && ow<world->n_countries && sim.econ->region[r].colonized)
                                    ? world->country[ow].color : 0u;
                }
                rp.region_tint = g_owner_tint;
            }
            /* OCCUPATION (brief terrain §membrane) : la carte politique HACHURE les
             * régions TENUES par les armes de la couleur de l'occupant — la propriété,
             * elle, ne bascule qu'à la paix. On lit dp->occupier, on passe des couleurs
             * de PAYS (aucun flottant SCPS). cell_color ne l'applique qu'en vue politique. */
            rp.occupier_tint = NULL;
            if (sim.ready){
                static uint32_t g_occ_tint[SCPS_MAX_REG];
                bool any=false;
                for (int r=0;r<sim.econ->n_regions && r<SCPS_MAX_REG;r++){
                    int occ=sim.dp->occupier[r];
                    if (occ>=0 && occ<world->n_countries){ g_occ_tint[r]=world->country[occ].color; any=true; }
                    else g_occ_tint[r]=0u;
                }
                if (any) rp.occupier_tint = g_occ_tint;
            }
            /* N3.1 : la carte principale trace ses frontières politiques en STROKES
             * écran (après le blit) → le bake n'en peint plus pour ces modes. */
            rp.screen_strokes = (rmode==VIEW_POLITICAL||rmode==VIEW_REGIONS||rmode==VIEW_COUNTRIES);
            render_map(world, pb.pixels, pb.w, pb.h, &rp, rmode);
            pixbuf_upload(&pb);
            /* §5 : la minicarte — le monde entier en petit (même mode + teinte). */
            if (mm_pb.pixels){
                RenderParams mmp = rp; mmp.selected_prov = -1;
                mmp.screen_strokes = false;        /* la minicarte garde le bake 1 cellule (pas de strokes dessus) */
                minimap_fit(&mmp.cam_scale, &mmp.cam_ox, &mmp.cam_oy);
                render_map(world, mm_pb.pixels, mm_pb.w, mm_pb.h, &mmp, rmode);
                pixbuf_upload(&mm_pb);
            }
            dirty = false;
        }

        SDL_RenderClear(ren);
        if (pb.tex) SDL_RenderCopy(ren, pb.tex, NULL, NULL);
        /* FX MER (display-only, cadence mur) : houle sur la mer ouverte + écume de rive,
         * SOUS les features de terre (rivières/routes/villes). No-op sans .bmp. */
        if (sim.ready && g_gs==GS_PLAYING){ draw_sea_fx(ren, world, &cam, win_w, win_h); draw_coast_fx(ren, world, &cam, win_w, win_h); }
        /* décors NATURE (pack display-only) : sur le terrain bléité, SOUS les frontières/labels. */
        if (sim.ready && g_gs==GS_PLAYING) { draw_map_rivers(ren, world, &cam, win_w, win_h); draw_map_roads(ren, world, sim.rn, &cam, win_w, win_h); draw_map_settlements(ren, world, sim.econ, &cam, win_w, win_h); draw_map_dressing(ren, world, sim.econ, &cam, win_w, win_h, 0); draw_map_dressing(ren, world, sim.econ, &cam, win_w, win_h, 1); }   /* calques : routes → villes → habillage → canopées */
        /* N3.1 — strokes de frontière en espace écran : province 2px / région 3px /
         * pays 5px, constants à TOUT zoom, posés PAR-DESSUS le terrain bléité et
         * SOUS la sélection/glyphes/étiquettes (z-order §2c). */
        if (sim.ready && g_gs==GS_PLAYING){
            ViewMode smode2 = (g_sb.lens!=LENS_NONE) ? VIEW_CULTURE : mode;
            borders_draw(ren, &cam, world, &sim, smode2, selected, win_w, win_h);
        }
        /* FX ARMÉES (la force de campagne anime sa case, PAR-DESSUS frontières & terrain)
         * + FX VORTEX (le maelström de la fin EAU §27, au foyer du cataclysme). */
        if (sim.ready && g_gs==GS_PLAYING){ draw_army_fx(ren, world, sim.camp, &cam, win_w, win_h); draw_vortex_fx(ren, world, sim.eg, &cam, win_w, win_h); }
        /* ── filtre COURANTS : lignes de flux sur la mer (les MORTES = le creux) ── */
        if (g_sb.show_currents && g_gs==GS_PLAYING){
            for (int sy=8; sy<win_h-8; sy+=14) for (int sx=8; sx<win_w-8; sx+=14){
                float _wx,_wy; cam_unproject(&cam,(float)sx,(float)sy,&_wx,&_wy);
                int cx2=(int)_wx, cy2=(int)_wy;
                if (cx2<0||cy2<0||cx2>=SCPS_W||cy2>=SCPS_H) continue;
                const Cell *cc=scps_cellc(world,cx2,cy2);
                if (cc->sea<=SEA_CABOTAGE) continue;          /* terre/côte : rien ; MORTE : creux */
                float vx2=cc->cur_vx/100.f, vy2=cc->cur_vy/100.f;
                float m=sqrtf(vx2*vx2+vy2*vy2);
                if (m<0.10f) continue;
                float l=(cc->sea==SEA_COURANT)?9.f:5.f;
                SDL_Color cl=(cc->sea==SEA_COURANT)?COL_COPPER:(SDL_Color){0x6a,0x8a,0xb0,0xff};
                SDL_SetRenderDrawColor(ren,cl.r,cl.g,cl.b,(cc->sea==SEA_COURANT)?0xE0:0x90);
                SDL_RenderDrawLine(ren,sx,sy,sx+(int)(vx2/m*l),sy+(int)(vy2/m*l));
                SDL_RenderDrawPoint(ren,sx+(int)(vx2/m*l),sy+(int)(vy2/m*l));
            }
        }
        /* mer §9 : au survol d'une tuile de mer, LE MOT — cabotage · eaux mortes ·
         * eaux vives · courant (et son sens). Des mots, jamais un vecteur nu. */
        if (g_gs==GS_PLAYING && g_font_small && sim.ready){
            int hmx,hmy; SDL_GetMouseState(&hmx,&hmy);
            float _hwx,_hwy; cam_unproject(&cam,(float)hmx,(float)hmy,&_hwx,&_hwy); int hcx=(int)_hwx, hcy=(int)_hwy;
            if (!over_panel(hmx,hmy,win_w,win_h,selected) && hcx>=0&&hcy>=0&&hcx<SCPS_W&&hcy<SCPS_H){  /* P0.2 : pas de hover carte sous un panneau */
                const Cell *hc=scps_cellc(world,hcx,hcy);
                if (hc->sea){
                    /* §E — la mer LISIBLE : le MOT de la classe (table FR/EN), et pour
                     * les eaux qui portent, le SENS du courant — jamais un vecteur nu. */
                    const char *cls = hc->sea==SEA_CABOTAGE?tr(STR_MER_CABOTAGE):
                                      hc->sea==SEA_MORTE   ?tr(STR_MER_MORTE):
                                      hc->sea==SEA_VIVE    ?tr(STR_MER_VIVE):
                                                            tr(STR_MER_COURANT);
                    char sw[96];
                    if (hc->sea==SEA_COURANT||hc->sea==SEA_VIVE){
                        int ax=hc->cur_vx, ay=hc->cur_vy;
                        const char *dir = (abs(ax)>=abs(ay)) ? (ax>=0?tr(STR_MER_DIR_EST):tr(STR_MER_DIR_OUEST))
                                                             : (ay>=0?tr(STR_MER_DIR_SUD):tr(STR_MER_DIR_NORD));
                        tr_fmt(sw,sizeof sw, STR_MER_DIR_FMT, cls, dir);
                    } else {
                        snprintf(sw,sizeof sw,"%s",cls);
                    }
                    draw_text(ren,g_font_small,hmx+14,hmy+10,COL_PARCH,sw);
                }
            }
        }
        /* Overlay diégétique : bandeau royaume + panneau de province, via la
         * membrane (bandes + mots). Le viewer ne touche aucun flottant SCPS. */
        if (sim.ready && g_font) {
            int mx2,my2; SDL_GetMouseState(&mx2,&my2);
            zone_reset(); bslot_reset(); orow_reset(); modebtn_reset(); topbtn_reset(); g_colonize_dst=-1; g_reloc_dst=-1; g_comptoir_reg=-1; g_center_reg=-1;
            int cid = country_for_panel(world, selected);
            if (g_gs!=GS_PLAYING) {                              /* le SHELL tient l'écran */
                shell_draw(ren,win_w,win_h,world,&sim,&g_stage);
                draw_hover_footer(ren, win_w, win_h, mx2, my2);
                SDL_RenderPresent(ren);
                int cx0,cy0; SDL_GetMouseState(&cx0,&cy0); (void)cx0;(void)cy0;
                SDL_Delay(8);
                continue;                                        /* pas de HUD hors partie */
            }
            if (show_tree) {                                    /* superposition de l'arbre (Tab) */
                draw_tech_tree(ren, win_w, win_h, sim.econ, sim.ts, world, sim.rn, cid);
            } else {
                draw_empire_labels(ren, &cam, &sim, world, win_w, win_h);  /* P1.8 : noms d'empire au dézoom */
                draw_army_markers(ren, &cam, &sim, world, win_w, win_h);   /* §4 : les armées sur la carte */
                draw_centre_markers(ren, &cam, &sim, world, win_w, win_h);  /* P3.20 : les Centres commerciaux */
                draw_topbar(ren, win_w, &sim, world, cid, speed);
                draw_outliner(ren, win_w, win_h, &sim, world, selected);            /* §6 : « ce que je possède » */
                draw_mode_buttons(ren, win_h, mode);                      /* §5 : modes de carte */
                draw_minimap(ren, &mm_pb, win_w, win_h, &cam);            /* §5 : la minicarte */
                if (selected >= 0)
                    draw_province_panel(ren, win_w, win_h, world, sim.econ, sim.wp, sim.wl, sim.drift, selected, &sim);
                draw_sidebar(ren, win_w, win_h, &sim, world, mode, selected);   /* §sidebar : rail + tiroir d'empire */
                if (g_diplo_target>=0) draw_diplo_popup(ren, win_w, world, &sim);  /* P0.3 */
            }
            shell_draw(ren,win_w,win_h,world,&sim,&g_stage);     /* surcouches : pause · tuto · confirmation */
            /* mer au survol : le MOT (repli de plus basse priorité — ajouté en dernier) */
            { float _wx,_wy; cam_unproject(&cam,(float)mx2,(float)my2,&_wx,&_wy); int cx3=(int)_wx, cy3=(int)_wy;
              if (!over_panel(mx2,my2,win_w,win_h,selected) && cx3>=0&&cy3>=0&&cx3<SCPS_W&&cy3<SCPS_H){  /* P0.2 */
                  const Cell *cc=scps_cellc(world,cx3,cy3);
                  if (cc->sea!=SEA_NONE){
                      static char seahov[96];
                      if (cc->sea==SEA_CABOTAGE) snprintf(seahov,sizeof seahov,"Cabotage — lent mais sûr, de port en port.");
                      else if (cc->sea==SEA_MORTE) snprintf(seahov,sizeof seahov,"Eaux mortes — rien ne pousse un navire ici : on contourne.");
                      else {
                          const char *d = (abs(cc->cur_vx)>=abs(cc->cur_vy))
                                        ? (cc->cur_vx>0?"l'est":"l'ouest") : (cc->cur_vy>0?"le sud":"le nord");
                          snprintf(seahov,sizeof seahov,"%s — courant favorable vers %s.",
                                   cc->sea==SEA_COURANT?"Couloir":"Eaux vives", d);
                      }
                      zone_add((SDL_Rect){0,0,win_w,win_h}, seahov);
                  } } }
            draw_hover_footer(ren, win_w, win_h, mx2, my2);     /* survol : nom + EFFET du nœud */
        }
#ifdef SCPS_DEV
        if (g_dev_overlay)   /* §6 : l'inspecteur brut PAR-DESSUS le jeu (dev seul) */
            dev_overlay_draw(world, sim.econ, sim.ts, sim.dp, country_for_panel(world, selected), selected);
#endif
        SDL_RenderPresent(ren);

        /* Status console */
        int mx, my; SDL_GetMouseState(&mx, &my);
        float _wx,_wy; cam_unproject(&cam,(float)mx,(float)my,&_wx,&_wy);
        int cx=(int)_wx, cy=(int)_wy;
        status_line(world, mode, seed, cx, cy, selected);

        SDL_Delay(8); /* ~120fps max */
    }

    printf("\n");
    pixbuf_destroy(&pb);
    pixbuf_destroy(&mm_pb);
    free(world);
    free(sim.econ); free(sim.wp); free(sim.wl); free(sim.net); free(sim.ts); free(sim.sc);
    free(sim.ag); free(sim.ev); free(sim.drift); free(sim.labor);
    warhost_free(sim.host);
    free(sim.dp); free(sim.rn); free(sim.rs); free(sim.host); free(sim.camp); free(sim.ai); free(sim.ai_on);
    free(sim.navy); free(sim.eg);
    if (g_font)     TTF_CloseFont(g_font);
    if (g_font_big) TTF_CloseFont(g_font_big);
    if (g_font_small) TTF_CloseFont(g_font_small);
    TTF_Quit();
#ifdef SCPS_DEV
    dev_overlay_shutdown();
#endif
    audio_shutdown();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
