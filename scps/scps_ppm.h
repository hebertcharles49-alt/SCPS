#ifndef SCPS_PPM_H
#define SCPS_PPM_H
/*
 * scps_ppm.h — écriture PPM partagée (outillage headless : dump.c, mapshot.c).
 * Dédoublonnage assumé de longue date (« doublons d'outillage assumés » —
 * synthèse mission éco) : DEUX copies statiques identiques vivaient dans les
 * deux outils. Une seule définition, static inline (le motif scps_math.h).
 */
#include <stdio.h>
#include <stdint.h>

static inline void write_ppm(const char *path, const uint32_t *px, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "écriture %s impossible\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w*h; i++) {
        uint32_t c = px[i];
        unsigned char rgb[3] = {
            (unsigned char)((c >> 16) & 0xFF),
            (unsigned char)((c >>  8) & 0xFF),
            (unsigned char)((c      ) & 0xFF)
        };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

#endif /* SCPS_PPM_H */
