shader_type spatial;
// FALAISE peinte : la géométrie low-poly seule fait « CG ». On REPEINT la roche par-dessus le toon —
// STRATES horizontales (bandes sédimentaires ∝ hauteur monde) + MOTTLE de bruit (taches de couleur) +
// AÉRIEN (le pied s'assombrit, le sommet s'éclaire) → texture organique, raccord au terrain peint.
// Display-only. La couleur d'INSTANCE (COLOR) porte la teinte de biome ; on la module ici.
render_mode diffuse_toon, specular_disabled, cull_back;

uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform float strata_freq = 1.7;     // ~3 bandes sur la hauteur du mur
uniform float strata_amt = 0.16;     // profondeur des lignes de strate
uniform float mottle_amt = 0.26;     // force des taches de bruit

varying vec3 wpos;
varying float yloc;

void vertex() {
	wpos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	yloc = VERTEX.y;                  // 0 = pied de la strate, 1 = haut (avant échelle)
}

void fragment() {
	vec3 base = COLOR.rgb;
	// STRATES : lignes sombres régulières en hauteur monde (le banc sédimentaire), brouillées par le bruit
	float ny = texture(noise_tex, wpos.xz * 0.08).r - 0.5;
	float band = sin((wpos.y + ny * 1.2) * strata_freq * 6.2831853);
	float strata = 1.0 - strata_amt * smoothstep(0.55, 1.0, band);   // creux fins, pas un dégradé plat
	// MOTTLE : deux octaves → taches de couleur (peinte, pas uni)
	float n1 = texture(noise_tex, wpos.xz * 0.13).r;
	float n2 = texture(noise_tex, wpos.xz * 0.41 + vec2(0.3, 0.7)).r;
	float mot = 1.0 + mottle_amt * (n1 * 0.6 + n2 * 0.4 - 0.5);
	ALBEDO = base * strata * mot;
	ROUGHNESS = 1.0;
}
