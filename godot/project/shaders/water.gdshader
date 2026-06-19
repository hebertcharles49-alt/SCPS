shader_type canvas_item;
//
// water.gdshader — appliqué au MAILLAGE ISO À HAUTEUR (MeshInstance2D). Le VOLUME vient
// de la GÉOMÉTRIE (sommets soulevés par l'altitude) ; ce shader ne fait que TEINTER :
//   · ÉCLAIRAGE directionnel depuis le gradient de hauteur (soleil au NO) → versants
//     éclairés/ombrés, pics rehaussés, vallées creusées (renforce le volume géométrique).
//   · EAU : houle croisée + écume de crête (couche SEA). TIME = horloge mur de Godot.
//
uniform sampler2D sea_tex : source_color, hint_default_black;     // L8 : 0 = terre, >0 = mer
uniform sampler2D height_tex : source_color, hint_default_black;  // L8 : altitude 0..1
uniform vec2  tex_px = vec2(0.0009765625, 0.001953125);          // 1 / taille de carte (1024×512)
uniform float relief_strength : hint_range(0.0, 40.0) = 16.0;    // contraste de l'éclairage de relief
uniform float relief_span     : hint_range(1.0, 8.0) = 3.0;      // largeur d'échantillon du gradient

void fragment() {
    vec4  base = texture(TEXTURE, UV);
    float sea  = texture(sea_tex, UV).r;

    if (sea < 0.003) {
        vec2  e  = tex_px * relief_span;
        float hL = texture(height_tex, UV - vec2(e.x, 0.0)).r;
        float hR = texture(height_tex, UV + vec2(e.x, 0.0)).r;
        float hU = texture(height_tex, UV - vec2(0.0, e.y)).r;
        float hD = texture(height_tex, UV + vec2(0.0, e.y)).r;
        vec3  n  = normalize(vec3((hL - hR) * relief_strength, (hU - hD) * relief_strength, 1.0));
        vec3  L  = normalize(vec3(-0.55, -0.62, 0.55));          // soleil au NORD-OUEST
        float diff  = clamp(dot(n, L), 0.0, 1.0);
        float shade = 0.62 + 0.74 * diff;                        // ombre 0.62 → lumière 1.36
        base.rgb *= shade;
        float h = texture(height_tex, UV).r;
        base.rgb += vec3(0.09, 0.08, 0.06) * smoothstep(0.58, 1.0, h) * diff;     // PICS éclairés
        base.rgb -= vec3(0.04, 0.05, 0.06) * (1.0 - smoothstep(0.0, 0.34, h));    // VALLÉES creusées
    } else {
        float w = sin(UV.x * 380.0 + TIME * 1.4)
                * sin(UV.y * 240.0 - TIME * 1.1);
        float crest = smoothstep(0.55, 1.0, w);
        base.rgb += vec3(0.05, 0.08, 0.10) * crest;                 // écume claire
        base.rgb -= vec3(0.02, 0.03, 0.04) * (1.0 - crest) * 0.5;   // creux
    }
    COLOR = base;
}
