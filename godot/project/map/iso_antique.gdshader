shader_type canvas_item;
// CARTE ANCIENNE (essai) — le sol rendu comme une carte de cartographe : fond PARCHEMIN, aplats SÉPIA
// par biome, ENCRE de côte, RIVIÈRES à la plume, HACHURES de relief (montagnes) & SEMIS (forêts),
// vignette & grade chaud. Lit les MÊMES données que iso_blend (biome_map, river_map, noise) — la
// projection iso est identique (inverse-iso sur `iso`=VERTEX). Display-only, opt-in. Pas de bâti/asset.
uniform sampler2D biome_map : filter_nearest;
uniform sampler2D river_map : filter_linear;
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform vec2 map_size = vec2(1024.0, 512.0);
uniform float ink : hint_range(0.0, 1.0) = 0.55;        // noirceur du trait d'encre (côtes)
uniform float paper_grain : hint_range(0.0, 0.3) = 0.10;
uniform float hatch_str : hint_range(0.0, 0.6) = 0.30;  // force des hachures de relief
uniform float vignette : hint_range(0.0, 0.6) = 0.24;

varying vec2 iso;
void vertex() { iso = VERTEX; }

int biome_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return -1;
	return int(texture(biome_map, c / map_size).r * 255.0 + 0.5);
}
bool is_water(int b) { return b >= 0 && b <= 2; }
bool is_forest(int b) { return b >= 12 && b <= 14; }
bool is_mountain(int b) { return b == 15 || b == 16 || b == 18 || b == 21 || b == 22 || b == 23; }
bool is_snow(int b) { return b == 19 || b == 20; }

// palette SÉPIA par biome (parchemin)
vec3 antique_color(int b) {
	if (is_water(b))           return vec3(0.60, 0.69, 0.71);   // mer bleu-gris pâle
	if (b == 3)                return vec3(0.87, 0.81, 0.61);   // plage / sable
	if (is_forest(b))          return vec3(0.62, 0.67, 0.46);   // forêt vert sourd
	if (b >= 9 && b <= 11)     return vec3(0.84, 0.74, 0.51);   // désert / steppe ocre
	if (is_snow(b))            return vec3(0.91, 0.90, 0.85);   // neige / glace crème
	if (is_mountain(b))        return vec3(0.77, 0.68, 0.52);   // montagne tan
	return vec3(0.83, 0.78, 0.59);                              // plaines / herbe (parchemin)
}

// VRAI si une cellule d'eau jouxte (rayon r) → sert au trait de côte / aux hachures côtières
bool near_water(vec2 cell, float r) {
	for (float a = 0.0; a < 6.2831; a += 1.0472) {   // 6 directions
		if (is_water(biome_at(cell + vec2(cos(a), sin(a)) * r))) return true;
	}
	return false;
}
bool near_land(vec2 cell, float r) {
	for (float a = 0.0; a < 6.2831; a += 1.0472) {
		int b = biome_at(cell + vec2(cos(a), sin(a)) * r);
		if (b >= 0 && !is_water(b)) return true;
	}
	return false;
}

void fragment() {
	vec2 cell = vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);   // inverse-iso (identique à iso_blend)
	int b = biome_at(cell);
	if (b < 0) {
		// HORS CARTE : le PAPIER nu (la carte est posée sur une feuille) → pas de trou gris au bord
		float pg = texture(noise_tex, cell * 0.5).r;
		vec3 paper = vec3(0.80, 0.72, 0.54) * mix(0.90, 1.07, pg);
		paper *= 1.0 - vignette * pow(clamp(length(SCREEN_UV - 0.5) * 1.35, 0.0, 1.0), 2.2);
		COLOR = vec4(mix(paper, paper * vec3(1.09, 1.0, 0.84), 0.45), 1.0);
	} else {
		vec3 col = antique_color(b);
		// grain de PAPIER (deux échelles) + taches
		float g = texture(noise_tex, cell * 0.5).r;
		float g2 = texture(noise_tex, cell * 0.03 + vec2(3.0, 7.0)).r;
		col *= mix(1.0 - paper_grain, 1.0 + paper_grain, g) * mix(0.93, 1.05, g2);

		if (is_water(b)) {
			// HACHURES CÔTIÈRES : la mer s'assombrit près du rivage (lignes parallèles à la côte estompées au large)
			if (near_land(cell, 2.2)) col *= 0.86;
			if (near_land(cell, 1.2)) col *= 0.90;
			// fines lignes de gravure horizontales (mer engravée)
			float ln = sin(iso.y * 1.6);
			col *= mix(1.0, 0.96, smoothstep(0.6, 1.0, ln));
		} else {
			// TRAIT D'ENCRE DE CÔTE : un cerne NET côté terre (proche) + un halo doux (un peu plus loin)
			if (near_water(cell, 2.4)) col = mix(col, vec3(0.30, 0.24, 0.17), ink * 0.35);
			if (near_water(cell, 1.1)) col = mix(col, vec3(0.20, 0.15, 0.10), ink);
		}

		// RIVIÈRES : trait bleu-encre (depuis le champ de débit)
		float rv = texture(river_map, (cell + vec2(0.5)) / map_size).r;
		col = mix(col, vec3(0.40, 0.50, 0.58), smoothstep(0.33, 0.46, rv) * 0.85);

		// HACHURES DE RELIEF (montagnes) : lignes diagonales écran, densité modulée bruit → relief « dessiné »
		if (is_mountain(b)) {
			float hatch = sin((iso.x - iso.y) * 0.9 + texture(noise_tex, cell * 0.12).r * 4.0);
			float hn = texture(noise_tex, cell * 0.18 + vec2(5.0, 2.0)).r;
			col *= mix(1.0, 1.0 - hatch_str, smoothstep(0.25, 0.95, hatch) * (0.45 + 0.55 * hn));
		}
		// FORÊTS : SEMIS de points (stipple) → symbole de bois
		if (is_forest(b)) {
			float d = texture(noise_tex, cell * 0.55 + vec2(2.0, 9.0)).r;
			col *= mix(1.0, 0.78, step(0.74, d));
		}

		// VIGNETTE sépia + grade chaud (atmosphère vieux papier)
		col *= 1.0 - vignette * pow(clamp(length(SCREEN_UV - 0.5) * 1.35, 0.0, 1.0), 2.2);
		col = mix(col, col * vec3(1.09, 1.0, 0.84), 0.45);
		COLOR = vec4(col, 1.0);
	}
}
