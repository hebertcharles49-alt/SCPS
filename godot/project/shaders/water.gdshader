shader_type canvas_item;
//
// Terrain ISO en TUILES (MeshInstance2D) — le sol parle iso, pas carte top-down :
//   · couleur PLATE par tuile (TEXTURE échantillonné au centre via UV) → les BORDS (côtes,
//     biomes) ÉPOUSENT la grille de losanges au lieu de la lisser ;
//   · TEXTURE procédurale par pixel (position iso via le VARYING v_pos) : variation de palette,
//     grain herbe/roche, dithering discret → riche, pas « pixel » plat ;
//   · OMBRAGE iso par tuile (COLOR.r = hillshade) → la lumière suit la géométrie iso ;
//   · EAU (COLOR.a : 0 terre · 0.5 berge · 1 profond) : houle à PHASE ALÉATOIRE PAR TUILE
//     (casse la périodicité cyan) + ourlet d'écume sur les berges. TIME = horloge mur.
//
uniform sampler2D sea_tex : source_color, hint_default_black;   // (réservé) 0 = terre, >0 = mer
uniform vec2 tile_count = vec2(204.0, 102.0);

varying vec2 v_pos;     // position ISO du fragment (stable en monde) — domaine de la texture

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}
float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void vertex() {
    v_pos = VERTEX;     // position iso locale (le pan bouge la caméra, pas le maillage → stable)
}

void fragment() {
    vec4  base  = texture(TEXTURE, UV);     // couleur PLATE de la tuile (albédo au centre)
    vec2  wp    = v_pos;                    // position iso par pixel (texture procédurale)
    float shade = COLOR.r;                  // hillshade iso de la tuile
    float water = COLOR.a;                  // 0 terre · 0.5 berge · 1 eau profonde
    vec2  tile  = floor(UV * tile_count);   // coord de tuile (graine de phase)

    if (water < 0.25) {
        // TERRE : ombrage iso × texture procédurale (palette + grain + dither)
        base.rgb *= shade;
        float n1 = vnoise(wp * 0.5);                    // grandes plaques (herbe/champ)
        float n2 = vnoise(wp * 1.7);                    // grain plus fin (roches/touffes)
        base.rgb *= 0.90 + 0.18 * n1;                   // variation de palette
        base.rgb += (n2 - 0.5) * 0.06;                  // grain
        base.rgb += (hash21(floor(wp)) - 0.5) * 0.025;  // dithering discret (casse le banding)
    } else {
        // EAU : houle à phase PAR TUILE (pas de périodicité globale) + berges écumeuses
        float ph = hash21(tile) * 6.2831;
        float w  = sin(wp.x * 0.30 + TIME * 1.25 + ph)
                 * sin(wp.y * 0.50 - TIME * 0.95 + ph * 1.7);
        float crest = smoothstep(0.40, 1.0, w);
        base.rgb += vec3(0.040, 0.070, 0.090) * crest;
        base.rgb -= vec3(0.015, 0.025, 0.030) * (1.0 - crest) * 0.5;
        if (water < 0.75) {                             // BERGE (eau peu profonde) : ourlet d'écume
            base.rgb += vec3(0.06, 0.09, 0.10) * (0.35 + 0.40 * crest);
        }
    }
    COLOR = vec4(base.rgb, 1.0);
}
