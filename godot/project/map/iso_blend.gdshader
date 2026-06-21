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
uniform float shore_w : hint_range(0.5, 5.0) = 1.8;       // demi-largeur de la bande de SABLE (cellules)
uniform float shore_str : hint_range(0.0, 1.0) = 0.85;    // opacité du sable de plage
uniform float uv_warp : hint_range(0.0, 1.5) = 0.7;       // ondule l'UV de texture → casse la répétition
uniform float var_amp : hint_range(0.0, 0.3) = 0.12;
uniform float var_sel_freq : hint_range(0.005, 0.1) = 0.028;   // échelle des RÉGIONS de variante
// EAU ANIMÉE (horloge MUR TIME → display-only, hors déterminisme) : ondulation d'UV + miroitement.
uniform float water_wave : hint_range(0.0, 0.3) = 0.07;
uniform float water_freq : hint_range(0.01, 0.2) = 0.045;
uniform float water_speed : hint_range(0.0, 0.5) = 0.06;
uniform float water_shimmer : hint_range(0.0, 1.0) = 0.5;
uniform float foam_w : hint_range(0.3, 3.0) = 1.0;            // largeur de l'ourlet d'écume (cellules)
uniform float foam_str : hint_range(0.0, 1.0) = 0.5;

varying vec2 iso;
void vertex() { iso = VERTEX; }

const int VAR = 4;   // variantes par biome (couche array = biome*VAR + variante)
int biome_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return -1;
	return int(texture(biome_map, c / map_size).r * 255.0 + 0.5);
}
// texture du biome, VARIANTE fondue par RÉGION (bruit basse-fréquence) → variété sans grille.
vec3 terr(int b, vec2 uv, vec2 cellG) {
	float vf = texture(noise_tex, cellG * var_sel_freq + vec2(11.0, 5.0)).r * float(VAR - 1);
	int v0 = int(vf);
	int v1 = min(v0 + 1, VAR - 1);
	int base = b * VAR;
	return mix(texture(terrains, vec3(uv, float(base + v0))).rgb,
	           texture(terrains, vec3(uv, float(base + v1))).rgb, fract(vf));
}

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
			bool sea = b <= 2;
			// UV ONDULÉE par un bruit basse-fréquence (casse la grille) ; pour l'EAU, ondulée EN PLUS par
			// le TEMPS → vagues qui COULENT (horloge mur, display-only) → fin de la répétition statique.
			vec2 uvw = cell / period + (vec2(texture(noise_tex, cell * 0.012).r,
				texture(noise_tex, cell * 0.012 + vec2(4.0, 2.0)).r) - 0.5) * uv_warp;
			if (sea) {
				vec2 flow = vec2(texture(noise_tex, cell * water_freq + vec2(TIME * water_speed, 0.0)).r,
					texture(noise_tex, cell * water_freq + vec2(0.0, -TIME * water_speed) + vec2(3.1, 1.7)).r) - 0.5;
				uvw += flow * water_wave;
			}
			vec2 uv = fract(uvw);
			vec3 col = terr(b, uv, cell);
			// FONDU inter-biome (terre↔terre OU profondeurs d'eau) : le voisin se fond ici.
			vec2 dir = normalize(wv + vec2(1e-4));
			int bn = biome_at(cw + dir * blendband);
			if (bn >= 0 && bn != b && (bn > 2) == (b > 2)) {
				float edge = texture(noise_tex, cell * warp_freq * 1.7 + vec2(2.1, 8.4)).r;
				col = mix(col, terr(bn, uv, cell), clamp(0.5 + (edge - 0.5) * 1.3, 0.0, 1.0) * 0.8);
			}
			if (sea) {                                     // MIROITEMENT : taches claires mouvantes
				float shim = texture(noise_tex, cell * water_freq * 2.3
					+ vec2(TIME * water_speed * 2.0, -TIME * water_speed * 1.5)).r;
				col += vec3(0.10, 0.14, 0.18) * smoothstep(0.74, 0.97, shim) * water_shimmer;
			}
			// SHORELINE : SABLE autour de toute côte (anneau) ; ÉCUME : ourlet blanc ANIMÉ au ras de la
			// terre, côté EAU (anneau plus serré). Détecté quelle que soit l'orientation de la côte.
			float shore = 0.0;
			float foam = 0.0;
			for (int i = 0; i < 8; i++) {
				float a = float(i) * 0.7854;
				vec2 d2 = vec2(cos(a), sin(a));
				int bs = biome_at(cw + d2 * shore_w);
				if (bs >= 0 && (bs <= 2) != sea) shore = 1.0;
				if (sea && biome_at(cw + d2 * foam_w) > 2) foam = 1.0;
			}
			if (shore > 0.5) col = mix(col, terr(3, uv, cell), shore_str);
			if (foam > 0.5) {
				float fp = 0.55 + 0.45 * sin(TIME * 2.2 + cell.x * 0.6 + cell.y * 0.4);
				col = mix(col, vec3(0.86, 0.93, 0.98), foam_str * fp);
			}
			float vb = mix(1.0 - var_amp, 1.0 + var_amp,
				texture(noise_tex, cell * 0.02 + vec2(3.1, 7.3)).r);
			COLOR = vec4(col * vb, 1.0);
		}
	}
}
