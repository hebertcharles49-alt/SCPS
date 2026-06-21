shader_type canvas_item;
// Sol CONTINU façon splat (AoM) : la texture PLATE du biome est échantillonnée en ESPACE MONDE
// CONTINU (inverse-iso de la position iso → cellule monde → UV tuilée, repeat) → un même biome est
// SANS COUTURE sur toute la carte (zéro grille de losanges). Le losange DÉCOUPE la cellule ; la
// texture COULE à travers les tuiles.
//   · COLOR.b ∈ ]0.25,0.75[ → SPRITE (falaise) : UV LOCAL, hors splat (sinon le sprite serait tuilé).
//   · COLOR.b < 0.25 → BASE : tout le losange, biome de la cellule.
//   · COLOR.b > 0.75 → DÉBORDEMENT : texture du voisin (COLOR.r = arêtes) fondue depuis l'arête.
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform float period : hint_range(2.0, 24.0) = 8.0;       // cellules monde par répétition de texture
uniform float feather : hint_range(0.05, 0.5) = 0.32;
uniform float spill : hint_range(0.1, 0.6) = 0.42;        // profondeur du débordement
uniform float noise_amp : hint_range(0.0, 0.5) = 0.30;
uniform float noise_freq : hint_range(0.01, 0.4) = 0.05;
uniform float var_freq : hint_range(0.005, 0.1) = 0.02;
uniform float var_amp : hint_range(0.0, 0.3) = 0.12;

varying vec2 iso;
void vertex() { iso = VERTEX; }     // position iso continue (espace cellule)

void fragment() {
	if (COLOR.b > 0.25 && COLOR.b < 0.75) {            // SPRITE (falaise) : UV local, pas de splat/clip
		vec4 s = texture(TEXTURE, UV);
		COLOR = vec4(s.rgb * COLOR.g, s.a);
	} else {
		// inverse-iso : iso = (cx - cy, (cx + cy)/2) → cellule monde par PIXEL → UV tuilée continue
		vec2 wcell = vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);
		vec4 tex = texture(TEXTURE, wcell / period);   // texture PLATE, repeat → coule entre tuiles
		float p = UV.x - 0.5;
		float q = UV.y - 0.5;
		float dwob = (texture(noise_tex, iso * noise_freq * 2.3).r - 0.5) * 0.11;
		float dia = 0.515 - (abs(p) + abs(q)) + dwob;  // >0 dans le losange (clip ondulé = arête organique)
		int cfg = int(COLOR.r * 15.0 + 0.5);
		float thr = 0.5 + (texture(noise_tex, iso * noise_freq).r - 0.5) * noise_amp;
		float ev[4];
		ev[0] = p - q;   // NE
		ev[1] = p + q;   // SE
		ev[2] = q - p;   // SW
		ev[3] = -p - q;  // NW
		float a;
		if (COLOR.b < 0.5) {
			a = 1.0;                                    // BASE : tout le losange
		} else {
			float thr_in = thr - spill;                // DÉBORDEMENT : fond le voisin depuis l'arête
			a = 0.0;
			for (int k = 0; k < 4; k++)
				if ((cfg & (1 << k)) != 0)
					a = max(a, clamp((ev[k] - thr_in) / feather, 0.0, 1.0));
		}
		a *= clamp(dia / 0.03, 0.0, 1.0);              // découpe au losange (AA léger)
		float vb = mix(1.0 - var_amp, 1.0 + var_amp,
			texture(noise_tex, iso * var_freq + vec2(3.1, 7.3)).r);   // variété continue (sans grille)
		COLOR = vec4(tex.rgb * COLOR.g * vb, tex.a * a);
	}
}
