shader_type canvas_item;
// CARTE ANCIENNE — le sol rendu comme une carte de cartographe : fond PARCHEMIN, aplats SÉPIA par biome
// (LISSÉS → pas de grille de cellules), ENCRE de côte + écume + bave, RIVIÈRES à la plume, HACHURES de
// relief par DENSITÉ (massifs clusterisés), ROSE DES VENTS, BORDS BRÛLÉS, vignette & grade chaud. Lit les
// MÊMES données que iso_blend (biome_map, river_map, noise) — même projection iso. Display-only, opt-in.
uniform sampler2D biome_map : filter_nearest;
uniform sampler2D river_map : filter_linear;
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform vec2 map_size = vec2(1024.0, 512.0);
uniform float ink : hint_range(0.0, 1.0) = 0.55;        // noirceur du trait d'encre (côtes)
uniform float paper_grain : hint_range(0.0, 0.3) = 0.09;
uniform float hatch_str : hint_range(0.0, 0.6) = 0.26;  // force des hachures de relief
uniform float hatch_freq : hint_range(0.3, 3.0) = 1.6;  // FINESSE des hachures (haut = serré)
uniform float vignette : hint_range(0.0, 0.6) = 0.20;
uniform float burn : hint_range(0.0, 0.8) = 0.42;       // bords BRÛLÉS (brunissement irrégulier)
uniform float compass_on = 1.0;
uniform vec2 compass_uv = vec2(0.13, 0.27);             // position (fraction de la carte) — un coin de mer
uniform float compass_r : hint_range(8.0, 80.0) = 30.0; // rayon MONDE de la rose
// (PI, TAU : constantes intégrées au langage de shader Godot — ne pas redéfinir)

varying vec2 iso;
void vertex() { iso = VERTEX; }

int biome_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return -1;
	return int(texture(biome_map, c / map_size).r * 255.0 + 0.5);
}
bool is_water(int b) { return b >= 0 && b <= 2; }
bool is_forest(int b) { return b >= 12 && b <= 14; }
bool is_mountain(int b) { return b == 18 || b == 21 || b == 22 || b == 23; }
bool is_snow(int b) { return b == 19 || b == 20; }
bool is_relief(int b) { return b == 15 || b == 16 || b == 18 || b == 21 || b == 22 || b == 23; }  // collines + montagnes

// palette SÉPIA par biome (parchemin) — biomes plus DISTINCTS (steppe ≠ désert, forêt olive)
vec3 antique_color(int b) {
	if (is_water(b))           return vec3(0.58, 0.67, 0.70);   // mer bleu-gris pâle
	if (b == 3)                return vec3(0.88, 0.82, 0.62);   // plage / sable clair
	if (is_forest(b))          return vec3(0.57, 0.62, 0.40);   // forêt vert OLIVE
	if (b == 9)                return vec3(0.79, 0.76, 0.54);   // steppe (herbe sèche)
	if (b >= 10 && b <= 11)    return vec3(0.86, 0.77, 0.53);   // désert ocre
	if (is_snow(b))            return vec3(0.92, 0.91, 0.86);   // neige / glace crème
	if (is_mountain(b))        return vec3(0.76, 0.67, 0.51);   // montagne tan
	return vec3(0.81, 0.79, 0.60);                              // plaines / herbe (parchemin verdâtre)
}

// couleur de base LISSÉE sur un petit voisinage → dissout le SEMIS de cellules en aplats régionaux.
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
// colline ISOLÉE non (sinon : semis de hachures = grille). Pilote teinte + ombre + hachures.
float relief_density(vec2 c) {
	float n = 0.0;
	if (is_relief(biome_at(c)))                   n += 1.0;
	if (is_relief(biome_at(c + vec2(3.0, 1.0))))   n += 1.0;
	if (is_relief(biome_at(c + vec2(-2.2, 2.6))))  n += 1.0;
	if (is_relief(biome_at(c + vec2(1.0, -3.1))))  n += 1.0;
	if (is_relief(biome_at(c + vec2(-3.0, -1.4)))) n += 1.0;
	if (is_relief(biome_at(c + vec2(2.6, 2.4))))   n += 1.0;
	return n / 6.0;
}

bool near_water(vec2 cell, float r) {
	for (float a = 0.0; a < 6.2831; a += 1.0472) {
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

// ROSE DES VENTS procédurale, ancrée à un point CARTE mais dessinée en espace ÉCRAN (iso) → CERCLES (pas
// d'ellipses) et branches DROITES à l'écran. 8 branches (cardinaux longs, diagonales courtes) + 2 anneaux.
float compass(vec2 ip, vec2 cworld, float R) {
	vec2 ci = vec2(cworld.x - cworld.y, (cworld.x + cworld.y) * 0.5);   // centre projeté en iso
	vec2 d = ip - ci;
	float cd = length(d);
	if (cd > R * 1.25) return 0.0;
	float a = atan(d.y, d.x);
	float v = 0.0;
	for (int k = 0; k < 8; k++) {
		float ka = float(k) * 0.78539816;                 // pas de 45°
		float da = abs(mod(a - ka + PI, TAU) - PI);        // écart angulaire au rayon k
		float len = (k % 2 == 0) ? R : R * 0.52;           // cardinaux longs, diagonales courtes
		if (cd < len && da < 0.12 * (1.0 - cd / max(len, 0.001))) v = 1.0;   // branche triangulaire
	}
	v = max(v, smoothstep(0.8, 0.0, abs(cd - R)) * 0.75);          // anneau externe
	v = max(v, smoothstep(0.6, 0.0, abs(cd - R * 0.60)) * 0.6);    // anneau interne
	if (cd < R * 0.06) v = 1.0;                                    // moyeu
	return v;
}

void fragment() {
	vec2 cell = vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);   // inverse-iso (identique à iso_blend)
	int b = biome_at(cell);
	vec3 col;

	if (b < 0) {
		// HORS CARTE : le PAPIER nu (la carte est posée sur une feuille)
		float pg = texture(noise_tex, cell * 0.5).r;
		col = vec3(0.80, 0.72, 0.54) * mix(0.90, 1.07, pg);
	} else {
		// WARP de la position de lecture → frontières de biome ONDULÉES (dissout la grille de losanges)
		vec2 bw = (vec2(texture(noise_tex, cell * 0.05).r,
		                texture(noise_tex, cell * 0.05 + vec2(2.0, 5.0)).r) - 0.5) * 5.0;
		vec2 cw = cell + bw;
		int bb = biome_at(cw);
		if (bb >= 0) b = bb;
		col = antique_base(cw, b);                 // aplat LISSÉ (dissout le semis de cellules)
		// grain de PAPIER WARPÉ, plus subtil en mer
		vec2 wv = vec2(texture(noise_tex, cell * 0.02).r,
		               texture(noise_tex, cell * 0.02 + vec2(4.0, 1.0)).r) - 0.5;
		float g = texture(noise_tex, (cell + wv * 9.0) * 0.18).r;
		float g2 = texture(noise_tex, cell * 0.012 + vec2(3.0, 7.0)).r;
		float grain_s = is_water(b) ? paper_grain * 0.5 : paper_grain;
		col *= mix(1.0 - grain_s, 1.0 + grain_s, g) * mix(0.93, 1.05, g2);

		if (is_water(b)) {
			if (near_land(cw, 2.2)) col *= 0.86;
			if (near_land(cw, 1.2)) col *= 0.90;
			if (near_land(cw, 1.8) && !near_land(cw, 1.2)) col = mix(col, vec3(0.78, 0.82, 0.80), 0.40);  // écume
			float ln = sin(iso.y * 1.6);                          // fines lignes de gravure (mer engravée)
			col *= mix(1.0, 0.96, smoothstep(0.6, 1.0, ln));
		} else {
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
		float plume = smoothstep(0.33, 0.46, rv) * (0.7 + 0.3 * rv_noise) + smoothstep(0.6, 0.9, rv) * 0.15;
		// TRANSITION RIVIÈRE→MER : sur l'EAU la rivière S'ÉVASE et se FOND dans la mer (estuaire, couleur
		// tirée vers le bleu marin, trait atténué) → pas de langue d'encre franche ; sur TERRE, trait net.
		vec3 river_col = is_water(b) ? mix(vec3(0.35, 0.45, 0.55), antique_color(0), 0.55) : vec3(0.35, 0.45, 0.55);
		col = mix(col, river_col, is_water(b) ? plume * 0.40 : plume);

		// RELIEF (massifs clusterisés, piloté par DENSITÉ → pas de hachures éparses en plaine)
		float rd = relief_density(cw);
		if (rd > 0.25) {
			float m = smoothstep(0.25, 0.65, rd);
			col = mix(col, vec3(0.76, 0.67, 0.51), m * 0.65);     // teinte massif tan cohérente
			if (relief_density(cw + vec2(2.6, 2.6)) < 0.2 && !is_water(biome_at(cw + vec2(2.0, 2.0)))) col *= 0.86;  // ombre SE
			// HACHURES FINES : direction variée par massif + jitter bruit ; force ∝ densité
			float dir = texture(noise_tex, cell * 0.02 + vec2(8.0, 3.0)).r;
			float coord = (iso.x - iso.y) * mix(0.7, 1.1, dir) + (iso.x + iso.y) * (dir - 0.5) * 0.35;
			float hatch = sin(coord * hatch_freq + texture(noise_tex, cell * 0.12).r * 4.0);
			float hn = texture(noise_tex, cell * 0.18 + vec2(5.0, 2.0)).r;
			col *= mix(1.0, 1.0 - hatch_str * m, smoothstep(0.45, 0.95, hatch) * (0.45 + 0.55 * hn));
			// CROISÉE sur les cœurs denses (relief fort) → hachures croisées, plus sombre
			if (m > 0.7) {
				float hatch2 = sin(coord * hatch_freq * 0.9 - (iso.x + iso.y) * 0.8 + 2.0);
				col *= mix(1.0, 1.0 - hatch_str * 0.6, smoothstep(0.55, 0.95, hatch2) * hn);
			}
		}
	}

	// ── COMMUN (carte ET papier) : rose des vents, bords brûlés, vignette, grade chaud ──
	if (compass_on > 0.5) {
		float rose = compass(iso, map_size * compass_uv, compass_r);
		col = mix(col, vec3(0.20, 0.14, 0.09), rose * 0.72);
	}
	// BORDS BRÛLÉS : brunissement IRRÉGULIER (bruit) qui monte vers les bords d'écran + vignette douce
	float edge = pow(clamp(length(SCREEN_UV - 0.5) * 1.42, 0.0, 1.0), 2.4);
	float bn = texture(noise_tex, SCREEN_UV * 3.3 + vec2(0.5, 0.2)).r;
	col = mix(col, vec3(0.26, 0.18, 0.10), clamp(edge * (0.55 + 0.6 * bn) * burn, 0.0, 0.72));   // brûlure brune
	col *= 1.0 - vignette * edge;                                                                // assombrissement doux
	col = mix(col, col * vec3(1.09, 1.0, 0.84), 0.45);                                           // grade chaud sépia
	COLOR = vec4(col, 1.0);
}
