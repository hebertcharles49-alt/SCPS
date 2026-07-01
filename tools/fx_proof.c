/* tools/fx_proof.c — PREUVE VISUELLE headless des animations FX (hors arbre de build).
 * Génère un monde réel, rend le terrain par render_map dans un renderer LOGICIEL,
 * composite les 4 planches FX (mer/côte/armée/vortex) aux VRAIES positions d'eau/
 * côte (+ quelques armées & un vortex de démo), et écrit un PNG. Aucun affichage.
 *
 *   cc tools/fx_proof.c <sources moteur> scps/scps_render.c -Ithird_party -Iscps \
 *      $(pkg-config --cflags --libs sdl2) -lm -o fx_proof && ./fx_proof [seed] [frame]
 */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <SDL.h>
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FX_SEA_CELL 128
#define FX_SEA_FRAMES 8
#define FX_COAST_CELL 128
#define FX_COAST_FRAMES 6
#define FX_ARMY_CELL 96
#define FX_ARMY_FRAMES 4
#define FX_VORTEX_CELL 256

static SDL_Texture *load_fx(SDL_Renderer *ren, const char *f){
    SDL_Surface *ns=SDL_LoadBMP(f); if(!ns){ printf("  [absent] %s\n",f); return NULL; }
    SDL_Surface *cv=SDL_ConvertSurfaceFormat(ns,SDL_PIXELFORMAT_ARGB8888,0); SDL_FreeSurface(ns); if(!cv)return NULL;
    Uint32 clear=SDL_MapRGBA(cv->format,0,0,0,0);
    for(int y=0;y<cv->h;y++){ Uint32*row=(Uint32*)((Uint8*)cv->pixels+(size_t)y*cv->pitch);
        for(int x=0;x<cv->w;x++){ Uint8 r,g,b,a; SDL_GetRGBA(row[x],cv->format,&r,&g,&b,&a);
            int mn=(r<b)?r:b,key=mn-(int)g; if(key<=2)continue;
            float m=(float)key/255.f,af=1.f-m; af*=af;
            if(af<0.03f){row[x]=clear;continue;}
            row[x]=SDL_MapRGBA(cv->format,r,g,b,(Uint8)(af*255.f+0.5f)); } }
    SDL_Texture*t=SDL_CreateTextureFromSurface(ren,cv); SDL_FreeSurface(cv);
    if(t)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND); return t;
}
static uint32_t mhash(int x,int y,uint32_t s){ uint32_t h=(uint32_t)x*0x8da6b343u^(uint32_t)y*0xd8163841u^s; h^=h>>15;h*=0x2c1b3c6du;h^=h>>12;h*=0x297a2d39u;h^=h>>15; return h; }
#define RAD2DEG 57.29577951308232
#define COAST_PUSH 0.85f
#define COAST_FACE_DEG 180.0f   /* mer en HAUT de la planche → base 180° */
/* normale de plage (réplique coast_sea_normal du viewer) */
static int csn(const World*w,int cx,int cy,float*nx,float*ny){
    float ax=0.f,ay=0.f; int n=0;
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){ if(!dx&&!dy)continue;
        int x=cx+dx,y=cy+dy; if(x<0||y<0||x>=SCPS_W||y>=SCPS_H)continue;
        const Cell*c=scps_cellc(w,x,y); if(c&&c->sea){ float il=1.f/(float)(abs(dx)+abs(dy)); ax+=(float)dx*il; ay+=(float)dy*il; n++; } }
    if(!n)return 0; float l=sqrtf(ax*ax+ay*ay); if(l<1e-4f)return 0; *nx=ax/l;*ny=ay/l; return 1;
}

/* atlas SETTLEMENTS (despill complet, réplique viewer.c) : 6 tiers × 6 groupes, 96 px. */
#define SETTLE_CELL 96
enum { SETTLE_MOUNTAIN=0, SETTLE_RIVER, SETTLE_ESTUARY, SETTLE_RURAL, SETTLE_MARKET, SETTLE_FORTIFIED };
static SDL_Texture *load_despilled(SDL_Renderer *ren, const char *file){
    SDL_Surface *ns=SDL_LoadBMP(file); if(!ns){ printf("  [absent] %s\n",file); return NULL; }
    SDL_Surface *cv=SDL_ConvertSurfaceFormat(ns,SDL_PIXELFORMAT_ARGB8888,0); SDL_FreeSurface(ns); if(!cv)return NULL;
    Uint32 clear=SDL_MapRGBA(cv->format,0,0,0,0);
    for(int y=0;y<cv->h;y++){ Uint32*row=(Uint32*)((Uint8*)cv->pixels+(size_t)y*cv->pitch);
        for(int x=0;x<cv->w;x++){ Uint8 r,g,b,a; SDL_GetRGBA(row[x],cv->format,&r,&g,&b,&a);
            int mn=(r<b)?r:b,key=mn-(int)g; if(key<=2)continue;
            float mness=(float)key/255.f,af=1.f-mness; af*=af;
            if(af<0.03f){row[x]=clear;continue;}
            int v=(int)((float)g*0.26f+0.5f);
            row[x]=SDL_MapRGBA(cv->format,(Uint8)v,(Uint8)v,(Uint8)v,(Uint8)(af*255.f+0.5f)); } }
    for(int y=0;y<cv->h;y++){ Uint32*row=(Uint32*)((Uint8*)cv->pixels+(size_t)y*cv->pitch);
        for(int x=0;x<cv->w;x++){ Uint8 r,g,b,a; SDL_GetRGBA(row[x],cv->format,&r,&g,&b,&a);
            if(a>0&&(int)r>(int)g+6&&(int)b>(int)g-6){ int v=((int)r*5+(int)g*9+(int)b*2)/16;
                row[x]=SDL_MapRGBA(cv->format,(Uint8)v,(Uint8)v,(Uint8)v,a); } } }
    SDL_Texture*t=SDL_CreateTextureFromSurface(ren,cv); SDL_FreeSurface(cv);
    if(t)SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND); return t;
}

int main(int argc,char**argv){
    uint32_t seed = argc>1?(uint32_t)strtoul(argv[1],0,10):9u;
    int frame = argc>2?atoi(argv[2]):2;
    float zoom = argc>3?(float)atof(argv[3]):5.0f;
    SDL_SetHint(SDL_HINT_VIDEODRIVER,"dummy");
    if(SDL_Init(SDL_INIT_VIDEO)!=0){ printf("SDL_Init: %s\n",SDL_GetError()); return 2; }

    World *w=malloc(sizeof(World)); WorldEconomy*econ=malloc(sizeof(WorldEconomy));
    if(!w||!econ){ printf("OOM\n"); return 2; }
    WorldParams p=worldparams_default(seed); world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);

    /* foyer caméra : une VILLE CÔTIÈRE — la région colonisée+côtière la plus peuplée ;
     * assise = une de SES cellules de côte (la plus proche de son centroïde). */
    int fx_c=SCPS_W/2, fy_c=SCPS_H/2, city_reg=-1, city_tier=4;
    { double bestpop=0;
      for(int r=0;r<econ->n_regions && r<w->n_regions;r++){
          const RegionEconomy*re=&econ->region[r];
          if(!re->colonized || !re->coastal) continue;
          double pop=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop;
          if(pop>bestpop){ bestpop=pop; city_reg=r; } }
      if(city_reg>=0){
          double cxs=0,cys=0; long n=0;
          for(int y=0;y<SCPS_H;y++)for(int x=0;x<SCPS_W;x++){ const Cell*c=scps_cellc(w,x,y); if(c&&c->region==(int16_t)city_reg){cxs+=x;cys+=y;n++;} }
          int ccx=n?(int)(cxs/n):SCPS_W/2, ccy=n?(int)(cys/n):SCPS_H/2, bd=1<<30, sx=ccx, sy=ccy;
          for(int y=0;y<SCPS_H;y++)for(int x=0;x<SCPS_W;x++){ const Cell*c=scps_cellc(w,x,y);
              if(c&&c->region==(int16_t)city_reg&&c->coast){ int d=abs(x-ccx)+abs(y-ccy); if(d<bd){bd=d;sx=x;sy=y;} } }
          fx_c=sx; fy_c=sy;
          city_tier = bestpop>=4000?5:bestpop>=1500?4:bestpop>=500?3:bestpop>=150?2:bestpop>=50?1:0;
      } else { /* pas de ville côtière : la côte la plus proche du centre */
          int best=1<<30; for(int y=60;y<SCPS_H-60;y++)for(int x=100;x<SCPS_W-100;x+=2){
              const Cell*c=scps_cellc(w,x,y); if(!c||!c->coast)continue;
              int d=abs(x-SCPS_W/2)+abs(y-SCPS_H/2); if(d<best){best=d;fx_c=x;fy_c=y;} } } }

    const int CW=1000,CH=760; const float scale=zoom;
    float ox=(float)fx_c-(float)CW/(2.f*scale), oy=(float)fy_c-(float)CH/(2.f*scale);

    SDL_Surface*canvas=SDL_CreateRGBSurfaceWithFormat(0,CW,CH,32,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer*ren=SDL_CreateSoftwareRenderer(canvas);
    if(!ren){ printf("software renderer: %s\n",SDL_GetError()); return 2; }

    uint32_t*px=malloc((size_t)CW*CH*4);
    RenderParams rp; memset(&rp,0,sizeof rp);
    rp.cam_ox=ox; rp.cam_oy=oy; rp.cam_scale=scale; rp.selected_prov=-1;
    rp.show_rivers=true; rp.show_borders=true; rp.iso=false; rp.screen_strokes=false;
    render_map(w,px,CW,CH,&rp,VIEW_TERRAIN);
    SDL_Texture*bg=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,CW,CH);
    SDL_UpdateTexture(bg,NULL,px,CW*4);
    SDL_RenderCopy(ren,bg,NULL,NULL);

    #define PROJ(WX,WY,SX,SY) do{ *(SX)=((WX)-ox)*scale; *(SY)=((WY)-oy)*scale; }while(0)
    SDL_Texture*sea=load_fx(ren,"scps_fx_sea.bmp");
    SDL_Texture*coast=load_fx(ren,"scps_fx_coast.bmp");
    SDL_Texture*army=load_fx(ren,"scps_fx_army.bmp");
    SDL_Texture*vortex=load_fx(ren,"scps_fx_vortex.bmp");
    SDL_Texture*settle=load_despilled(ren,"scps_map_settlements.bmp");
    int nsea=0,ncoast=0;

    if(sea){ int TILE=8,dpx=(int)((float)TILE*scale+1.5f); SDL_SetTextureAlphaMod(sea,90);
      for(int ty=0;ty<SCPS_H;ty+=TILE)for(int tx=0;tx<SCPS_W;tx+=TILE){
        int mx=tx+4,my=ty+4; if(mx>=SCPS_W||my>=SCPS_H)continue;
        const Cell*c=scps_cellc(w,mx,my); if(!c||c->sea==0)continue;
        float sx,sy; PROJ((float)tx+4.f,(float)ty+4.f,&sx,&sy);
        if(sx<-dpx||sx>CW+dpx||sy<-dpx||sy>CH+dpx)continue;
        uint32_t h=mhash(tx,ty,0x5EA50000u); int ph=(frame+(int)(h%FX_SEA_FRAMES))%FX_SEA_FRAMES;
        int en=(int)c->cur_vx*c->cur_vx+(int)c->cur_vy*c->cur_vy; int row=(en>1200)?1:0;
        SDL_Rect s={ph*FX_SEA_CELL,row*FX_SEA_CELL,FX_SEA_CELL,FX_SEA_CELL}, d={(int)sx-dpx/2,(int)sy-dpx/2,dpx,dpx};
        SDL_RenderCopy(ren,sea,&s,&d); nsea++; } SDL_SetTextureAlphaMod(sea,255); }

    if(coast){ int TILE=3,dpx=(int)(5.f*scale+1.5f); if(dpx<10)dpx=10; SDL_SetTextureAlphaMod(coast,150);
      for(int ty=0;ty<SCPS_H;ty+=TILE)for(int tx=0;tx<SCPS_W;tx+=TILE){
        const Cell*c=scps_cellc(w,tx,ty); if(!c||!c->coast)continue;
        float wnx,wny; if(!csn(w,tx,ty,&wnx,&wny))continue;
        float p0x=tx+0.5f,p0y=ty+0.5f, psx=p0x+wnx*COAST_PUSH, psy=p0y+wny*COAST_PUSH;
        float ax,ay,bx,by; PROJ(p0x,p0y,&ax,&ay); PROJ(psx,psy,&bx,&by);
        float sdx=bx-ax,sdy=by-ay,sl=sqrtf(sdx*sdx+sdy*sdy); if(sl<1e-3f)continue;
        if(bx<-dpx||bx>CW+dpx||by<-dpx||by>CH+dpx)continue;
        double rot=atan2((double)sdy,(double)sdx)*RAD2DEG-90.0+COAST_FACE_DEG;
        uint32_t h=mhash(tx,ty,0xC0A57001u); int ph=(frame+(int)(h%FX_COAST_FRAMES))%FX_COAST_FRAMES;
        SDL_Rect s={ph*FX_COAST_CELL,0,FX_COAST_CELL,FX_COAST_CELL}, d={(int)bx-dpx/2,(int)by-dpx/2,dpx,dpx};
        SDL_RenderCopyEx(ren,coast,&s,&d,rot,NULL,SDL_FLIP_NONE); ncoast++; } SDL_SetTextureAlphaMod(coast,255); }

    /* la VILLE CÔTIÈRE elle-même (sprite estuaire, ancré bas-centre) — SUR le terrain
     * et les FX mer/écume, comme dans le viewer (settlements après sea/coast). */
    if(settle && city_reg>=0){
        static const float dsc[6]={0.50f,0.66f,0.84f,1.05f,1.28f,1.55f};
        float sx,sy; PROJ((float)fx_c+0.5f,(float)fy_c+0.5f,&sx,&sy);
        int dpx=(int)(scale*18.f*dsc[city_tier]); if(dpx<22)dpx=22; if(dpx>680)dpx=680;
        SDL_Rect s={city_tier*SETTLE_CELL, SETTLE_ESTUARY*SETTLE_CELL, SETTLE_CELL, SETTLE_CELL};
        SDL_Rect d={(int)sx-dpx/2,(int)sy-(dpx*7)/10,dpx,dpx};
        SDL_RenderCopy(ren,settle,&s,&d);
    }

    int coastonly = getenv("FX_COASTONLY")!=NULL;   /* vérif d'orientation : écume seule */
    if(army && !coastonly){ int dpx=(int)(10.f*scale+0.5f); if(dpx<20)dpx=20;
      for(int k=0;k<3;k++){ float wx=(float)fx_c+(float)(k-1)*13.f, wy=(float)fy_c+(float)(k-1)*7.f-12.f;
        float sx,sy; PROJ(wx,wy,&sx,&sy);
        int row=(k==2)?1:0,col=(frame+k)%FX_ARMY_FRAMES;
        SDL_Rect s={col*FX_ARMY_CELL,row*FX_ARMY_CELL,FX_ARMY_CELL,FX_ARMY_CELL}, d={(int)sx-dpx/2,(int)sy-(dpx*3)/4,dpx,dpx};
        SDL_RenderCopy(ren,army,&s,&d);} }

    if(vortex && !coastonly){ int vx=fx_c,vy=fy_c,found=0;
      for(int rad=1;rad<80&&!found;rad++)for(int a=0;a<16&&!found;a++){
        int axc=fx_c+(int)(rad*cos(a*0.39)),ayc=fy_c+(int)(rad*sin(a*0.39));
        if(axc<0||ayc<0||axc>=SCPS_W||ayc>=SCPS_H)continue;
        const Cell*c=scps_cellc(w,axc,ayc); if(c&&c->sea){vx=axc;vy=ayc;found=1;} }
      float sx,sy; PROJ((float)vx,(float)vy,&sx,&sy);
      double ang=frame*40.0; int big=(int)(60.f*scale); int big2=(int)(big*0.66f);
      SDL_Rect s0={0,0,FX_VORTEX_CELL,FX_VORTEX_CELL}, s1={FX_VORTEX_CELL,0,FX_VORTEX_CELL,FX_VORTEX_CELL};
      SDL_Rect d0={(int)sx-big/2,(int)sy-big/2,big,big}, d1={(int)sx-big2/2,(int)sy-big2/2,big2,big2};
      SDL_SetTextureAlphaMod(vortex,205);
      SDL_RenderCopyEx(ren,vortex,&s0,&d0,ang,NULL,SDL_FLIP_NONE);
      SDL_RenderCopyEx(ren,vortex,&s1,&d1,-ang*1.4,NULL,SDL_FLIP_NONE);
      SDL_SetTextureAlphaMod(vortex,255); }

    SDL_RenderPresent(ren);

    /* canvas (ARGB en mémoire) → RGBA octets pour stb (format-agnostique via SDL_GetRGBA) */
    unsigned char*out=malloc((size_t)CW*CH*4);
    for(int y=0;y<CH;y++){ Uint32*row=(Uint32*)((Uint8*)canvas->pixels+(size_t)y*canvas->pitch);
      for(int x=0;x<CW;x++){ Uint8 r,g,b,a; SDL_GetRGBA(row[x],canvas->format,&r,&g,&b,&a);
        unsigned char*o=out+((size_t)y*CW+x)*4; o[0]=r;o[1]=g;o[2]=b;o[3]=255; } }
    stbi_write_png("fx_proof.png",CW,CH,4,out,CW*4);
    printf("fx_proof.png écrit (%dx%d) · seed %u frame %d · foyer (%d,%d) · ville côtière rég %d tier %d · tuiles mer=%d côte=%d\n",
           CW,CH,seed,frame,fx_c,fy_c,city_reg,city_tier,nsea,ncoast);
    return 0;
}
