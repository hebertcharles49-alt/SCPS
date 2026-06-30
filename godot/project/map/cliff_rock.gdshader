shader_type spatial;
// FALAISE peinte : la géométrie low-poly seule fait « CG ». On REPEINT la roche par-dessus le toon —
// STRATES sédimentaires (2 fréquences ∝ hauteur monde), COULURES verticales (érosion), MOTTLE de bruit
// (taches de couleur), VEINE chaude dans les creux, PIED dans l'ombre (raccord au sol) → texture
// organique, raccord au terrain peint. Display-only. COLOR = teinte de biome (instance) ; on la module.
// LAMBERT doux (pas toon) → dégradés peints sur les faces, moins de tranche « CG » dure.
render_mode diffuse_lambert_wrap, specular_disabled, cull_back;

uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform float strata_freq = 1.25;    // bancs sédimentaires sur la hauteur du mur
uniform float strata_amt = 0.25;     // profondeur des lignes de strate
uniform float mottle_amt = 0.32;     // force des taches de bruit
uniform float streak_amt = 0.17;     // force des coulures verticales

varying vec3 wpos;

void vertex() {
	wpos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	vec3 base = COLOR.rgb;
	// STRATES : 2 fréquences, ligne de banc brouillée par le bruit → bancs IRRÉGULIERS (pas un peigne)
	float ny = texture(noise_tex, wpos.xz * 0.07).r - 0.5;
	float yb = wpos.y + ny * 1.7;
	float band = 0.62 * sin(yb * strata_freq * 6.2831853)
	           + 0.38 * sin(yb * strata_freq * 2.3 * 6.2831853 + 1.7);
	float crev = strata_amt * smoothstep(0.30, 1.0, band);   // creux de strate (sombre)
	float strata = 1.0 - crev;
	// COULURES verticales (érosion / patine) : bruit ÉTIRÉ en hauteur
	float stk = texture(noise_tex, vec2(wpos.x * 0.5 + wpos.z * 0.37, wpos.y * 0.045)).r;
	float streak = 1.0 - streak_amt * smoothstep(0.5, 1.0, stk);
	// MOTTLE 2 octaves → taches de couleur (peint, pas uni)
	float n1 = texture(noise_tex, wpos.xz * 0.12).r;
	float n2 = texture(noise_tex, wpos.xz * 0.40 + vec2(0.3, 0.7)).r;
	float mot = 1.0 + mottle_amt * (n1 * 0.6 + n2 * 0.4 - 0.5);
	// PIED dans l'ombre (raccord au sol), faces hautes captent la lumière
	float foot = mix(0.70, 1.0, smoothstep(0.0, 2.6, wpos.y));
	vec3 col = base * strata * streak * mot * foot;
	// VEINE plus chaude dans les creux de strate (lecture « peinte »)
	col = mix(col, col * vec3(1.08, 1.0, 0.90), crev / max(strata_amt, 0.001) * 0.45);
	ALBEDO = col;
	ROUGHNESS = 1.0;
}
