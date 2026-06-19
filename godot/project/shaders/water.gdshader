shader_type canvas_item;
//
// water.gdshader — l'eau (couche SEA) ANIMÉE + le RELIEF VOLUMÉTRIQUE (couche HEIGHT).
// Tout en shader, par-dessus le terrain render_map. TIME = horloge mur de Godot (le
// moteur reste déterministe : il n'affiche pas).
//
//  · VOLUMÉTRIE : le gradient de hauteur donne une NORMALE → éclairage directionnel
//    (soleil au NO) → les reliefs prennent du VOLUME (versants éclairés/ombrés, pics
//    rehaussés, vallées creusées). Un léger DÉCALAGE vertical ∝ hauteur (« parallaxe »)
//    pousse les hauteurs vers le haut → un soupçon d'ANGLE DE VUE (le relief « monte »).
//  · EAU : houle croisée + écume de crête (inchangé).
//
uniform sampler2D sea_tex : source_color, hint_default_black;     // L8 : 0 = terre, >0 = mer
uniform sampler2D height_tex : source_color, hint_default_black;  // L8 : altitude 0..1
uniform vec2  tex_px = vec2(0.0009765625, 0.001953125);         // 1 / taille de carte (1024×512), réglé par GDScript
uniform float relief_strength : hint_range(0.0, 40.0) = 22.0;   // force du volume
uniform float relief_lift     : hint_range(0.0, 24.0) = 4.0;    // décalage vertical ∝ hauteur (angle de vue — léger, pour ne pas désaxer l'overlay)
uniform float relief_span     : hint_range(1.0, 8.0) = 4.0;     // largeur d'échantillon du gradient (capte la PENTE)

void fragment() {
    // PARALLAXE D'ALTITUDE : on échantillonne plus HAUT là où c'est élevé → le relief
    // « se dresse » vers l'observateur (un soupçon d'iso sans casser la carte plate).
    float h0   = texture(height_tex, UV).r;
    vec2  liftv = vec2(0.0, h0 * relief_lift * tex_px.y);
    vec2  uv    = UV + liftv;

    vec4  base = texture(TEXTURE, uv);
    float sea  = texture(sea_tex, uv).r;

    if (sea < 0.003) {
        // NORMALE depuis le gradient de hauteur (échantillon ÉLARGI → la PENTE du relief,
        // pas le bruit pixel) → ÉCLAIRAGE directionnel = VOLUME.
        vec2  e  = tex_px * relief_span;
        float hL = texture(height_tex, uv - vec2(e.x, 0.0)).r;
        float hR = texture(height_tex, uv + vec2(e.x, 0.0)).r;
        float hU = texture(height_tex, uv - vec2(0.0, e.y)).r;
        float hD = texture(height_tex, uv + vec2(0.0, e.y)).r;
        vec3  n  = normalize(vec3((hL - hR) * relief_strength, (hU - hD) * relief_strength, 1.0));
        vec3  L  = normalize(vec3(-0.55, -0.62, 0.55));          // soleil au NORD-OUEST
        float diff  = clamp(dot(n, L), 0.0, 1.0);
        float shade = 0.58 + 0.82 * diff;                        // versants : ombre 0.58 → lumière 1.40
        base.rgb *= shade;
        float h = texture(height_tex, uv).r;
        base.rgb += vec3(0.10, 0.09, 0.07) * smoothstep(0.58, 1.0, h) * diff;     // PICS rehaussés
        base.rgb -= vec3(0.04, 0.05, 0.06) * (1.0 - smoothstep(0.0, 0.34, h));    // VALLÉES creusées
    } else {
        // houle : deux ondes croisées + scintillement de crête
        float w = sin(uv.x * 380.0 + TIME * 1.4)
                * sin(uv.y * 240.0 - TIME * 1.1);
        float crest = smoothstep(0.55, 1.0, w);
        base.rgb += vec3(0.05, 0.08, 0.10) * crest;                 // écume claire
        base.rgb -= vec3(0.02, 0.03, 0.04) * (1.0 - crest) * 0.5;   // creux
    }
    COLOR = base;
}
