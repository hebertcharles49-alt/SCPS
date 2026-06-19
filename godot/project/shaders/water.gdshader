shader_type canvas_item;
//
// Terrain ISO (MeshInstance2D) — applique l'albédo LISSÉ (filtre linéaire = blend à fond)
// × l'OMBRAGE par sommet (COLOR = hillshade cuit au maillage → relief sculpté), puis anime
// l'EAU (houle croisée + écume de crête) sur les cellules de mer. TIME = horloge MUR de Godot
// (display-only : ne touche ni la simulation ni le déterminisme).
//
uniform sampler2D sea_tex : source_color, hint_default_black;   // L8 : 0 = terre, >0 = mer

void fragment() {
    vec4  base = texture(TEXTURE, UV);     // albédo (terrain/biome/politique), lissé
    base.rgb  *= COLOR.rgb;                // ombrage de relief par sommet (hillshade interpolé)
    float sea  = texture(sea_tex, UV).r;
    if (sea > 0.003) {                     // EAU : houle animée
        float w = sin(UV.x * 420.0 + TIME * 1.30)
                * sin(UV.y * 260.0 - TIME * 1.00);
        float crest = smoothstep(0.45, 1.0, w);
        base.rgb += vec3(0.045, 0.075, 0.095) * crest;             // écume claire
        base.rgb -= vec3(0.02,  0.03,  0.04)  * (1.0 - crest) * 0.4; // creux
    }
    COLOR = base;
}
