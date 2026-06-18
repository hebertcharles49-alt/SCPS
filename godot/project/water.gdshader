shader_type canvas_item;
//
// water.gdshader — LA PROMESSE tenue : la continuité eau↔asset en SHADER, pas en
// blit. Anime la mer (couche SEA du moteur) et fond les crêtes, par-dessus le
// terrain render_map. Aucun atlas, aucun despill, aucune horloge de frame — TIME
// est l'horloge mur de Godot (le moteur, lui, reste déterministe : il n'affiche pas).
//
uniform sampler2D sea_tex : source_color, hint_default_black;  // L8 : 0 = terre, >0 = mer

void fragment() {
    vec4 base = texture(TEXTURE, UV);
    float sea = texture(sea_tex, UV).r;
    if (sea > 0.003) {
        // houle : deux ondes croisées + scintillement de crête
        float w = sin(UV.x * 380.0 + TIME * 1.4)
                * sin(UV.y * 240.0 - TIME * 1.1);
        float crest = smoothstep(0.55, 1.0, w);
        base.rgb += vec3(0.05, 0.08, 0.10) * crest;                 // écume claire
        base.rgb -= vec3(0.02, 0.03, 0.04) * (1.0 - crest) * 0.5;   // creux
    }
    COLOR = base;
}
