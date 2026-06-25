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
bool is_mountain(int b) { return b == 18 || b == 21 || b == 22 || b == 23; }   // VRAIS reliefs (pas les collines éparses 15/16 → grille)
bool is_snow(int b) { return b == 19 || b == 20; }
bool is_relief(int b) { return b == 15 || b == 16 || b == 18 || b == 21 || b == 22 || b == 23; }  // collines + montagnes

// palette SÉPIA par biome (parchemin)
vec3 antique_color(int b) {
	if (is_water(b))           return vec3(0.60, 0.69, 0.71);   // mer bleu-gris pâle
	if (b == 3)                return vec3(0.87, 0.81, 0.61);   // plage / sable
	if (is_forest(b))          return vec3(0.58, 0.63, 0.42);   // forêt vert OLIVE (se détache des plaines)
	if (b >= 9 && b <= 11)     return vec3(0.84, 0.74, 0.51);   // désert / steppe ocre
	if (is_snow(b))            return vec3(0.91, 0.90, 0.85);   // neige / glace crème
	if (is_mountain(b))        return vec3(0.77, 0.68, 0.52);   // montagne tan
	return vec3(0.83, 0.78, 0.59);                              // plaines / herbe (parchemin)
}

// couleur de base LISSÉE sur un petit voisinage → dissout le SEMIS de cellules (biomes mêlés dans une
// région) en aplats régionaux propres, tout en gardant les grandes régions distinctes. Taps hors-carte ignorés.
vec3 antique_base(vec2 c, int bc) {
	vec3 a = antique_color(bc); float n = 1.0;
	int b1 = biome_at(c + vec2(2.3, 0.7));   if (b1 >= 0) { a += antique_color(b1); n += 1.0; }
	int b2 = biome_at(c + vec2(-1.8, 1.9));  if (b2 >= 0) { a += antique_color(b2); n += 1.0; }
	int b3 = biome_at(c + vec2(0.6, -2.4));  if (b3 >= 0) { a += antique_color(b3); n += 1.0; }
	int b4 = biome_at(c + vec2(-2.1, -1.3)); if (b4 >= 0) { a += antique_color(b4); n += 1.0; }
	int b5 = biome_at(c + vec2(1.5, 2.2));   if (b5 >= 0) { a += antique_color(b5); n += 1.0; }
	return a / n;
}

// DENSITÉ de relief (fraction du voisinage en colline/montagne) → un MASSIF clusterisé hachure, une
// colline ISOLÉE dans la plaine non (sinon : semis de hachures = grille). Pilote teinte + ombre + hachures.
float relief_density(vec2 c) {
	float n = 0.0;
	if (is_relief(biome_at(c)))                  n += 1.0;
	if (is_relief(biome_at(c + vec2(3.0, 1.0))))  n += 1.0;
	if (is_relief(biome_at(c + vec2(-2.2, 2.6)))) n += 1.0;
	if (is_relief(biome_at(c + vec2(1.0, -3.1)))) n += 1.0;
	if (is_relief(biome_at(c + vec2(-3.0, -1.4))))n += 1.0;
	if (is_relief(biome_at(c + vec2(2.6, 2.4))))  n += 1.0;
	return n / 6.0;
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
		// WARP de la position de lecture → frontières de biome ONDULÉES (DISSOUT la grille de losanges des
		// cellules) : on relit le biome à une position perturbée par un bruit continu (comme iso_blend).
		vec2 bw = (vec2(texture(noise_tex, cell * 0.05).r,
		                texture(noise_tex, cell * 0.05 + vec2(2.0, 5.0)).r) - 0.5) * 5.0;
		vec2 cw = cell + bw;
		int bb = biome_at(cw);
		if (bb >= 0) b = bb;                       // garde le biome ondulé (hors-carte → garde l'original)
		vec3 col = antique_base(cw, b);            // aplat LISSÉ (dissout le semis de cellules)
		// grain de PAPIER WARPÉ, PLUS SUBTIL en mer + taches lentes
		vec2 wv = vec2(texture(noise_tex, cell * 0.02).r,
		               texture(noise_tex, cell * 0.02 + vec2(4.0, 1.0)).r) - 0.5;
		float g = texture(noise_tex, (cell + wv * 9.0) * 0.18).r;
		float g2 = texture(noise_tex, cell * 0.012 + vec2(3.0, 7.0)).r;
		float grain_s = is_water(b) ? paper_grain * 0.5 : paper_grain;   // le papier vibre moins sous l'eau
		col *= mix(1.0 - grain_s, 1.0 + grain_s, g) * mix(0.93, 1.05, g2);

		if (is_water(b)) {
			// HACHURES CÔTIÈRES (mer assombrie près du rivage) + LISERÉ D'ÉCUME au ras de la côte
			if (near_land(cw, 2.2)) col *= 0.86;
			if (near_land(cw, 1.2)) col *= 0.90;
			if (near_land(cw, 1.8) && !near_land(cw, 1.2)) col = mix(col, vec3(0.78, 0.82, 0.80), 0.40);
			float ln = sin(iso.y * 1.6);                          // fines lignes de gravure (mer engravée)
			col *= mix(1.0, 0.96, smoothstep(0.6, 1.0, ln));
		} else {
			// TRAIT D'ENCRE DE CÔTE : cerne NET côté terre + halo doux + BAVE d'encre dans le papier
			if (near_water(cw, 2.4)) col = mix(col, vec3(0.30, 0.24, 0.17), ink * 0.35);
			if (near_water(cw, 1.1)) col = mix(col, vec3(0.20, 0.15, 0.10), ink);
			if (near_water(cw, 2.2) && !near_water(cw, 1.2)) {
				float bleed = texture(noise_tex, cell * 0.2 + vec2(9.0, 3.0)).r;
				col = mix(col, vec3(0.30, 0.24, 0.18), bleed * 0.15);
			}
		}

		// RIVIÈRES à la PLUME : trait vibré par bruit + ÉPAISSISSEMENT aux confluences (fort débit)
		float rv = texture(river_map, (cell + vec2(0.5)) / map_size).r;
		float rv_noise = texture(noise_tex, cell * 0.35 + vec2(1.3, 4.7)).r;
		float plume = smoothstep(0.33, 0.46, rv) * (0.7 + 0.3 * rv_noise);
		float width_var = smoothstep(0.6, 0.9, rv) * 0.15;
		col = mix(col, vec3(0.35, 0.45, 0.55), plume + width_var);

		// RELIEF (massifs clusterisés, piloté par la DENSITÉ → pas de hachures éparses en plaine)
		float rd = relief_density(cw);
		if (rd > 0.25) {
			float m = smoothstep(0.25, 0.65, rd);
			// teinte MASSIF tan (cohérente : collines+pics fondus en un relief), proportionnelle à la densité
			col = mix(col, vec3(0.76, 0.67, 0.51), m * 0.65);
			// OMBRE PORTÉE (lumière NW) : le pied SE, en contrebas du massif, s'assombrit → relief perçu
			if (relief_density(cw + vec2(2.6, 2.6)) < 0.2 && !is_water(biome_at(cw + vec2(2.0, 2.0)))) col *= 0.86;
			// HACHURES : direction VARIÉE par massif + jitter bruit ; force ∝ densité (cœur dense = dense)
			float dir = texture(noise_tex, cell * 0.02 + vec2(8.0, 3.0)).r;
			float coord = (iso.x - iso.y) * mix(0.7, 1.1, dir) + (iso.x + iso.y) * (dir - 0.5) * 0.35;
			float hatch = sin(coord * 0.9 + texture(noise_tex, cell * 0.12).r * 4.0);
			float hn = texture(noise_tex, cell * 0.18 + vec2(5.0, 2.0)).r;
			col *= mix(1.0, 1.0 - hatch_str * m, smoothstep(0.25, 0.95, hatch) * (0.45 + 0.55 * hn));
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
