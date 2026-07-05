#ifndef SCPS_MATH_H
#define SCPS_MATH_H
/*
 * scps_math.h — helpers numériques PARTAGÉS du moteur.
 *
 * Consolidation des copies locales qui vivaient dans ~20 modules (clampf ×15,
 * absf ×9, iclamp ×3, xs32 ×3, frand ×2) — UNE sémantique, plus de dérive :
 *   - clampf n'avale ni ne propage le NaN : v!=v ⇒ lo (audit P1-2) ;
 *   - xs32 ne rend JAMAIS 0 (l'état xorshift resterait coincé à zéro) ;
 *   - header autonome, aucun <math.h> requis (abs manuel — cf. le piège des
 *     déclarations implicites de scps_econ.h/provmod_collect).
 */
#include <stdint.h>

static inline float clampf(float v, float lo, float hi){ return v!=v?lo:(v<lo?lo:(v>hi?hi:v)); }
static inline float absf(float v){ return v<0.f?-v:v; }
static inline int   iclamp(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }

/* xorshift32 déterministe (Marsaglia) — le générateur de TOUS les tirages sim. */
static inline uint32_t xs32(uint32_t *s){ uint32_t x=*s; x^=x<<13; x^=x>>17; x^=x<<5; return *s=x?x:1u; }
static inline float    frand(uint32_t *s){ return (float)(xs32(s)&0xffffffu)/(float)0x1000000u; }

#endif /* SCPS_MATH_H */
