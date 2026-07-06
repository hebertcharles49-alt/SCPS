/*
 * mapshot.c — rendu headless de N mondes (vue terrain) en PPM.
 *   ./mapshot <graine_base> <n> [vue]   (vue : 0=terrain[défaut] 1=politique)
 * Les PPM sont ensuite convertis en PNG (Pillow). Sert à juger d'un coup d'œil
 * le relief / la répartition des biomes (vallées vs montagnes).
 */
#include "scps_world.h"
#include "scps_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scps_ppm.h"   /* write_ppm partagé (dédoublonné avec dump.c) */

int main(int argc,char**argv){
    uint32_t base=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):7u;
    int n   =(argc>2)?atoi(argv[2]):10;
    int view=(argc>3)?atoi(argv[3]):0;
    int scale=2;                       /* 2× : 1 cellule → 2 px (carte plus nette) */
    int W=SCPS_W*scale, H=SCPS_H*scale;
    World *w=(World*)malloc(sizeof(World));
    uint32_t *buf=(uint32_t*)malloc((size_t)W*H*4);
    if(!w||!buf){ fprintf(stderr,"OOM\n"); return 1; }

    RenderParams rp = {
        .cam_ox=0.f, .cam_oy=0.f, .cam_scale=(float)scale,
        .selected_prov=-1,
        .show_rivers=true, .show_borders=true, .show_grid=false
    };
    /* view = index brut de ViewMode (0=terrain, 1=politique, 10=habitabilité…). */
    ViewMode vm = (view>=0 && view<VIEW_COUNT) ? (ViewMode)view : VIEW_TERRAIN;

    for (int k=0;k<n;k++){
        uint32_t seed=base+(uint32_t)k*101u;
        WorldParams p=worldparams_default(seed);
        world_generate(w,&p);
        render_map(w, buf, W, H, &rp, vm);
        char path[64]; snprintf(path,sizeof path,"seed_%u.ppm",seed);
        write_ppm(path, buf, W, H);
        printf("  graine %u → %s (%dx%d)\n", seed, path, W, H);
    }
    free(buf); free(w);
    return 0;
}
