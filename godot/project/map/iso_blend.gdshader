shader_type canvas_item;
// SOL = SPLAT par PIXEL, tracé en UN quad couvrant la carte iso. Pour chaque pixel :
//   1. inverse-iso → cellule monde (cx,cy) ;
//   2. on PERTURBE la position de lecture par un bruit continu → la frontière de biome devient
//      ONDULÉE (plus de losanges droits) ;
//   3. on lit le BIOME dans `biome_map` (R = biome/255) à cette position perturbée ;
//   4. on échantillonne la texture TUILÉE de ce biome (texture-array) en espace monde continu.
// → terrain CONTINU, frontières organiques, zéro grille. La PRIORITÉ : on lit AUSSI le biome voisin
//   dominant et on fond vers lui sur une bande (le dominant mord). SHORELINE : sable injecté à la
//   frontière terre/mer. Falaises = passe SPRITE (COLOR.b≈0.5, UV local).
uniform sampler2DArray terrains : filter_linear_mipmap, repeat_enable;
uniform sampler2D biome_map : filter_nearest;
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform vec2 map_size = vec2(1024.0, 512.0);
uniform float period : hint_range(2.0, 24.0) = 8.0;       // cellules par répétition de texture
uniform float warp : hint_range(0.0, 12.0) = 5.0;         // amplitude d'ondulation des frontières (cellules)
uniform float warp_freq : hint_range(0.01, 0.2) = 0.05;
uniform float blendband : hint_range(0.5, 6.0) = 2.2;     // largeur de fondu inter-biome (cellules)
uniform float uv_warp : hint_range(0.0, 1.5) = 0.7;       // ondule l'UV de texture → casse la répétition
uniform float var_amp : hint_range(0.0, 0.3) = 0.12;

varying vec2 iso;
void vertex() { iso = VERTEX; }

int biome_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return -1;
	return int(texture(biome_map, c / map_size).r * 255.0 + 0.5);
}
vec3 terr(int b, vec2 uv) { return texture(terrains, vec3(uv, float(b))).rgb; }

void fragment() {
	if (COLOR.b > 0.25 && COLOR.b < 0.75) {            // SPRITE (falaise) : UV local
		vec4 s = texture(TEXTURE, UV);
		COLOR = vec4(s.rgb * COLOR.g, s.a);
	} else {
		vec2 cell = vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);   // inverse-iso
		vec2 wv = vec2(texture(noise_tex, cell * warp_freq).r,
		               texture(noise_tex, cell * warp_freq + vec2(5.2, 1.3)).r) - 0.5;
		vec2 cw = cell + wv * (2.0 * warp);            // position de lecture ONDULÉE
		int b = biome_at(cw);
		if (b < 0) { COLOR = vec4(0.0); }
		else {
			// UV de texture ONDULÉE par un bruit basse-fréquence → la tuile ne se répète plus en grille
			vec2 uvw = cell / period + (vec2(texture(noise_tex, cell * 0.012).r,
				texture(noise_tex, cell * 0.012 + vec2(4.0, 2.0)).r) - 0.5) * uv_warp;
			vec2 uv = fract(uvw);
			vec3 col = terr(b, uv);
			// PRIORITÉ + SHORELINE : on regarde un cran plus loin dans le sens du gradient de bruit ;
			// si le voisin DOMINE (ou frontière terre/mer → sable), on fond vers lui sur une bande.
			vec2 dir = normalize(wv + vec2(1e-4));
			int bn = biome_at(cw + dir * blendband);
			if (bn >= 0 && bn != b) {
				bool sea_a = b <= 2; bool sea_b = bn <= 2;
				int top = bn; bool shore = (sea_a != sea_b);
				if (shore) top = 3;                    // terre↔mer → SABLE (coast)
				// fondu : distance signée à la frontière, adoucie par le bruit
				float edge = texture(noise_tex, cell * warp_freq * 1.7 + vec2(2.1, 8.4)).r;
				float t = clamp(0.5 + (edge - 0.5) * 1.3, 0.0, 1.0);
				if (shore || top != b) col = mix(col, terr(top, uv), t * 0.85);
			}
			float vb = mix(1.0 - var_amp, 1.0 + var_amp,
				texture(noise_tex, cell * 0.02 + vec2(3.1, 7.3)).r);
			COLOR = vec4(col * vb, 1.0);
		}
	}
}
