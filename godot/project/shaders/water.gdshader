shader_type canvas_item;
//
// Terrain ISO en TUILES — la tuile fixe la GÉOMÉTRIE (et les bandes côtières) mais n'est PAS
// l'élément graphique dominant : la couleur de terrain est LISSÉE par pixel (contraste de tuile
// presque invisible, plus de damier), texturée (herbe/terre/roche en micro-dither), ombrée par un
// hillshade PAR PIXEL (doux). Côte en DEUX couronnes (terre → sable → eau peu profonde → profond).
// Mer en bleu PROFOND maîtrisé (pas cyan), variations de valeur, RARES reflets animés (TIME = mur).
//
uniform sampler2D height_tex : hint_default_black;   // altitude 0..1 (ombrage par pixel)
uniform vec2 map_size   = vec2(1024.0, 512.0);
uniform vec2 tile_count = vec2(204.0, 102.0);        // (réservé)

varying vec2 v_pos;     // position ISO du fragment (stable en monde)

const vec3 SAND     = vec3(0.76, 0.69, 0.50);
const vec3 SEA_DEEP = vec3(0.04, 0.10, 0.19);
const vec3 SEA_SHAL = vec3(0.12, 0.26, 0.37);

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
    v_pos = VERTEX;     // le pan bouge la caméra, pas le maillage → domaine de texture stable
}

const float SEA_LEVEL = 0.43;   // == SEA_LEVEL moteur : la limite terre/eau

void fragment() {
    // position MONDE par pixel = inverse iso PLAT (h=0) : wx = iy + ix/2 · wy = iy − ix/2
    vec2  world = vec2(v_pos.y + v_pos.x * 0.5, v_pos.y - v_pos.x * 0.5);
    vec2  uvpp  = clamp(world / map_size, vec2(0.0), vec2(1.0));
    float h     = texture(height_tex, uvpp).r;     // altitude LISSÉE (linéaire) → tout est doux

    vec3 col;
    if (h >= SEA_LEVEL) {
        // TERRE : albédo LISSÉ par pixel + couronne de sable IRRÉGULIÈRE au rivage
        col = texture(TEXTURE, uvpp).rgb;
        float edge  = (vnoise(v_pos * 1.1) - 0.5) * 0.018;             // seuil perturbé (bord dentelé doux)
        float beach = 1.0 - smoothstep(SEA_LEVEL + edge, SEA_LEVEL + 0.038 + edge, h);
        beach *= 0.55 + 0.70 * vnoise(v_pos * 1.7 + 13.0);            // densité IRRÉGULIÈRE (pas un liseré)
        col = mix(col, SAND, clamp(beach, 0.0, 1.0) * 0.5);
        // micro-texture (herbe/terre/roche/humidité) — petites variations, faible contraste
        float n1 = vnoise(v_pos * 0.7);
        float n2 = vnoise(v_pos * 2.3);
        col *= 0.97 + 0.06 * n1;
        col += (n2 - 0.5) * 0.028;
        col += (hash21(floor(v_pos)) - 0.5) * 0.010;
        // OMBRAGE par pixel (gradient d'altitude, soleil NO) — doux, lissé, pas de damier
        vec2  e  = 1.5 / map_size;
        float hl = texture(height_tex, uvpp - vec2(e.x, 0.0)).r;
        float hr = texture(height_tex, uvpp + vec2(e.x, 0.0)).r;
        float hu = texture(height_tex, uvpp - vec2(0.0, e.y)).r;
        float hd = texture(height_tex, uvpp + vec2(0.0, e.y)).r;
        vec3  n  = normalize(vec3((hl - hr) * 7.0, (hu - hd) * 7.0, 1.0));
        float diff = clamp(dot(n, normalize(vec3(-0.5, -0.6, 0.62))), 0.0, 1.0);
        col *= 0.93 + 0.12 * diff;
    } else {
        // EAU : PROFONDEUR lissée par pixel → dégradé bleu profond CONTINU (ni cyan, ni marches)
        float depth = clamp((SEA_LEVEL - h) / 0.28, 0.0, 1.0);
        col = mix(SEA_SHAL, SEA_DEEP, depth);
        // eau PEU PROFONDE plus large & VARIÉE près du rivage (autour des îlots)
        float shoal = 1.0 - smoothstep(0.0, 0.22, depth);
        col += vec3(0.03, 0.06, 0.05) * shoal * (0.5 + 0.8 * vnoise(v_pos * 1.2));
        col += (vnoise(v_pos * 0.45) - 0.5) * 0.012;        // micro-variation de valeur
        float sp = vnoise(v_pos * 0.85 + vec2(TIME * 0.10, -TIME * 0.08));
        sp = smoothstep(0.88, 0.98, sp);                    // seuil haut → reflets CLAIRSEMÉS
        col += vec3(0.10, 0.13, 0.15) * sp;
        // ÉCUME IRRÉGULIÈRE au rivage : quelques pixels (bruit), pas un liseré uniforme
        float fn   = vnoise(v_pos * 2.2 + vec2(TIME * 0.06, -TIME * 0.04));
        float foam = (1.0 - smoothstep(0.0, 0.06, depth)) * smoothstep(0.45, 0.75, fn);
        col += vec3(0.12, 0.15, 0.15) * foam;
    }
    COLOR = vec4(col, 1.0);
}
