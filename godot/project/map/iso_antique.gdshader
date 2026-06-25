shader_type canvas_item;
// CARTE ANCIENNE — le sol rendu comme une carte de cartographe. TOP-DOWN (flat_map=1 : plus de skew iso) :
// fond PARCHEMIN, aplats SÉPIA par biome (LISSÉS → pas de grille), ENCRE de côte + écume, MARAIS en zone
// humide (n'obstrue PAS les estuaires), RIVIÈRES à la plume + estuaires fondus, HACHURES de relief par
// DENSITÉ, ROSE DES VENTS, BORDS BRÛLÉS. Lit biome_map + river_map (mêmes données qu'iso_blend). Opt-in.
//
// Enum Biome RÉEL (scps_types.h) : 0-2 mer · 3 côte · 4-6 plaine/agricole/prairie · 7-8 steppe/savane ·
// 9-11 drylands/désert · 12-14 forêt/bois/jungle · 15 MARAIS · 16 highlands · 17 collines · 18 montagnes ·
// 19 pic · 20 glacier · 21 MANGROVE · 22 TOURBIÈRE · 23 volcan · 24 ronces.
uniform sampler2D biome_map : filter_nearest;
uniform sampler2D river_map : filter_linear;
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform vec2 map_size = vec2(1024.0, 512.0);
uniform float flat_map = 1.0;                           // 1 = TOP-DOWN (carte) ; 0 = projection iso 2:1
uniform float ink : hint_range(0.0, 1.0) = 0.55;
uniform float paper_grain : hint_range(0.0, 0.3) = 0.09;
uniform float hatch_str : hint_range(0.0, 0.6) = 0.26;
uniform float hatch_freq : hint_range(0.3, 3.0) = 1.6;
uniform float vignette : hint_range(0.0, 0.6) = 0.20;
uniform float burn : hint_range(0.0, 0.8) = 0.42;
uniform float compass_on = 1.0;
uniform vec2 compass_uv = vec2(0.13, 0.27);
uniform float compass_r : hint_range(8.0, 80.0) = 30.0;
uniform float rgb_jitter : hint_range(0.0, 16.0) = 3.0;   // variation PERLIN intra-biome : ±N/255 par canal
uniform float jitter_freq : hint_range(0.02, 0.5) = 0.12; // échelle des taches de couleur

varying vec2 iso;
void vertex() { iso = VERTEX; }

int biome_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return -1;
	return int(texture(biome_map, c / map_size).r * 255.0 + 0.5);
}
bool is_water(int b)   { return b >= 0 && b <= 2; }
bool is_wetland(int b) { return b == 15 || b == 21 || b == 22; }            // MARAIS, mangrove, tourbière
bool is_wet(int b)     { return is_water(b) || is_wetland(b); }
bool is_solid(int b)   { return b >= 3 && !is_wetland(b); }                 // terre FERME (hors marais)
bool is_forest(int b)  { return b >= 12 && b <= 14; }
bool is_relief(int b)  { return b == 17 || b == 18 || b == 19 || b == 23; } // collines + montagnes + pic + volcan

// palette SÉPIA par biome (enum RÉEL)
vec3 antique_color(int b) {
	if (b <= 2)                        return vec3(0.47, 0.57, 0.63);   // MER : bleu plus PROFOND (sombre)
	if (b == 3)                        return vec3(0.88, 0.82, 0.62);   // côte / plage
	if (b == 15 || b == 21 || b == 22) return vec3(0.63, 0.70, 0.60);   // MARAIS / mangrove / tourbière (humide)
	if (b >= 12 && b <= 14)            return vec3(0.57, 0.62, 0.40);   // forêt OLIVE
	if (b == 7 || b == 8)              return vec3(0.80, 0.76, 0.55);   // steppe / savane (herbe sèche)
	if (b >= 9 && b <= 11)             return vec3(0.86, 0.77, 0.53);   // drylands / désert ocre
	if (b == 20)                       return vec3(0.92, 0.91, 0.86);   // glacier crème
	if (b == 17)                       return vec3(0.79, 0.72, 0.55);   // collines tan clair
	if (b == 18 || b == 19)            return vec3(0.76, 0.67, 0.51);   // montagnes / pic tan
	if (b == 23)                       return vec3(0.55, 0.49, 0.46);   // volcan basalte
	if (b == 24)                       return vec3(0.47, 0.36, 0.33);   // ronces (corruption)
	return vec3(0.81, 0.79, 0.60);                                      // plaine / prairie / highlands gentil
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

// DENSITÉ de relief (massif clusterisé hachure, colline isolée non → pas de grille de hachures).
float relief_density(vec2 c) {
	float n = 0.0;
	if (is_relief(biome_at(c)))                    n += 1.0;
	if (is_relief(biome_at(c + vec2(3.0, 1.0))))    n += 1.0;
	if (is_relief(biome_at(c + vec2(-2.2, 2.6))))   n += 1.0;
	if (is_relief(biome_at(c + vec2(1.0, -3.1))))   n += 1.0;
	if (is_relief(biome_at(c + vec2(-3.0, -1.4))))  n += 1.0;
	if (is_relief(biome_at(c + vec2(2.6, 2.4))))    n += 1.0;
	return n / 6.0;
}

bool near_solid(vec2 cell, float r) {
	for (float a = 0.0; a < 6.2831; a += 1.0472) {
		if (is_solid(biome_at(cell + vec2(cos(a), sin(a)) * r))) return true;
	}
	return false;
}
bool near_wet(vec2 cell, float r) {
	for (float a = 0.0; a < 6.2831; a += 1.0472) {
		int b = biome_at(cell + vec2(cos(a), sin(a)) * r);
		if (b >= 0 && is_wet(b)) return true;
	}
	return false;
}

// ROSE DES VENTS — ancrée à un point carte mais dessinée en espace ÉCRAN (`ci` = centre projeté) → cercles.
float compass(vec2 ip, vec2 ci, float R) {
	vec2 d = ip - ci;
	float cd = length(d);
	if (cd > R * 1.25) return 0.0;
	float a = atan(d.y, d.x);
	float v = 0.0;
	for (int k = 0; k < 8; k++) {
		float ka = float(k) * 0.78539816;
		float da = abs(mod(a - ka + PI, TAU) - PI);
		float len = (k % 2 == 0) ? R : R * 0.52;
		if (cd < len && da < 0.12 * (1.0 - cd / max(len, 0.001))) v = 1.0;
	}
	v = max(v, smoothstep(0.8, 0.0, abs(cd - R)) * 0.75);
	v = max(v, smoothstep(0.6, 0.0, abs(cd - R * 0.60)) * 0.6);
	if (cd < R * 0.06) v = 1.0;
	return v;
}

// bruit de VALEUR lissé (perlin-like) : hash de coin + interpolation smoothstep → taches DOUCES.
float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
	vec2 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = hash2(i), b = hash2(i + vec2(1.0, 0.0)), c = hash2(i + vec2(0.0, 1.0)), d = hash2(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// hash entier → [0,1) (déterministe, display-only) pour varier les symboles par cellule de semis.
float hash(uint n) {
	n = (n << 13u) ^ n;
	n = n * (n * n * 15731u + 789221u) + 1376312589u;
	return float(n & 0x7fffffffu) / 2147483648.0;
}

// ARBRE STYLISÉ à l'encre : conifère (3 triangles empilés) OU feuillu (couronne ronde) + tronc. uv local.
float draw_tree(vec2 uv, uint seed) {
	float h = hash(seed);
	vec2 p = uv;
	float trunk = step(abs(p.x), 0.025) * step(p.y, 0.35) * step(-0.45, p.y);
	float foliage = 0.0;
	for (int i = 0; i < 3; i++) {
		float y_off = -0.15 - float(i) * 0.20;
		float tri_w = 0.22 - float(i) * 0.04;
		float tri_h = 0.22;
		foliage += step(abs(p.x) / tri_w + (p.y - y_off) / tri_h, 1.0) * step(p.y, y_off) * step(y_off - tri_h, p.y);
	}
	foliage = min(foliage, 1.0);
	float crown = 0.0;
	if (h < 0.5) {
		crown = step(length(p - vec2(0.0, -0.25)), 0.18)
		      + step(length(p - vec2(-0.08, -0.32)), 0.12)
		      + step(length(p - vec2(0.07, -0.30)), 0.10);
		crown = min(crown, 1.0) * 0.9;
	}
	return max(max(trunk, foliage), crown);
}

// MONTAGNE STYLISÉE à l'encre : pic principal + pic secondaire + hachures internes + ombre portée SE.
float draw_mountain(vec2 uv, uint seed) {
	vec2 p = uv;
	float h1 = hash(seed);
	float h2 = hash(seed + 1u);
	float peak_h = 0.5 + h1 * 0.3;
	float peak_w = 0.25 + h2 * 0.1;
	float mountain = step(abs(p.x) / peak_w + p.y / peak_h, 1.0) * step(p.y, 0.0) * step(-peak_h, p.y);
	mountain += step(abs(p.x - 0.18) / 0.12 + (p.y + 0.08) / 0.30, 1.0) * step(p.y, -0.08) * step(-0.38, p.y) * 0.8;
	float hatch = 0.0;
	for (int i = 0; i < 5; i++) {
		float y_line = -0.05 - float(i) * 0.09;
		hatch += step(abs(p.x + (p.y - y_line) * 0.15), 0.015) * step(p.y, y_line + 0.01) * step(y_line - 0.02, p.y);
	}
	hatch = min(hatch, 1.0) * 0.6;
	float shadow = step(p.x, 0.0) * step(-p.x * 0.4 + p.y + 0.15, 0.0) * mountain * 0.35;
	return min(max(max(mountain, shadow), hatch * mountain), 1.0);
}

void fragment() {
	// TOP-DOWN : cell = position monde directe (iso=VERTEX du quad plat) ; sinon inverse-iso 2:1.
	vec2 cell = (flat_map > 0.5) ? iso : vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);
	int b = biome_at(cell);
	vec3 col;

	if (b < 0) {
		// HORS CARTE : marge de PAPIER nu (la carte est posée sur une feuille)
		float pg = texture(noise_tex, cell * 0.5).r;
		col = vec3(0.80, 0.72, 0.54) * mix(0.90, 1.07, pg);
	} else {
		// WARP de lecture → frontières de biome ONDULÉES (dissout la grille de cellules)
		vec2 bw = (vec2(texture(noise_tex, cell * 0.05).r,
		                texture(noise_tex, cell * 0.05 + vec2(2.0, 5.0)).r) - 0.5) * 5.0;
		vec2 cw = cell + bw;
		int bb = biome_at(cw);
		if (bb >= 0) b = bb;
		col = antique_base(cw, b);
		// grain de PAPIER
		vec2 wv = vec2(texture(noise_tex, cell * 0.02).r,
		               texture(noise_tex, cell * 0.02 + vec2(4.0, 1.0)).r) - 0.5;
		float g = texture(noise_tex, (cell + wv * 9.0) * 0.18).r;
		float g2 = texture(noise_tex, cell * 0.012 + vec2(3.0, 7.0)).r;
		float grain_s = is_wet(b) ? paper_grain * 0.5 : paper_grain;
		col *= mix(1.0 - grain_s, 1.0 + grain_s, g) * mix(0.93, 1.05, g2);
		// VARIATION PERLIN intra-biome : jitter RGB ±rgb_jitter/255 PAR CANAL (3 bruits décorrélés → micro
		// dérive de teinte, pas juste de luminance) → les aplats RESPIRENT au lieu d'être morts.
		vec3 jit3 = vec3(vnoise(cell * jitter_freq),
		                 vnoise(cell * jitter_freq + vec2(37.2, 11.5)),
		                 vnoise(cell * jitter_freq + vec2(91.7, 53.1))) - 0.5;
		col += jit3 * (2.0 * rgb_jitter / 255.0);

		if (is_water(b)) {
			// MER : assombrie + écume près de la TERRE FERME (PAS le marais → estuaire OUVERT)
			if (near_solid(cw, 2.2)) col *= 0.86;
			if (near_solid(cw, 1.2)) col *= 0.90;
			if (near_solid(cw, 1.8) && !near_solid(cw, 1.2)) col = mix(col, vec3(0.78, 0.82, 0.80), 0.40);
			col *= mix(1.0, 0.96, smoothstep(0.6, 1.0, sin(iso.y * 1.6)));   // gravure
		} else if (is_wetland(b)) {
			// MARAIS : SYMBOLE de tirets horizontaux (cartographie) — wetland traité comme « humide »,
			// donc AUCUN trait de côte qui obstrue l'estuaire : la rivière/mer y passent librement.
			float rows = sin(iso.y * 0.9 + texture(noise_tex, cell * 0.1).r * 2.0);
			float rowOn = smoothstep(0.35, 0.7, rows);
			float dash = sin(iso.x * 1.7 + step(0.0, rows) * 1.6);     // tirets décalés rangée/rangée
			col = mix(col, col * 0.78, rowOn * smoothstep(0.2, 0.7, dash) * 0.7);
		} else {
			// TERRE FERME : trait d'ENCRE de côte contre l'eau ET le marais (la rive solide) + bave
			if (near_wet(cw, 2.4)) col = mix(col, vec3(0.30, 0.24, 0.17), ink * 0.35);
			if (near_wet(cw, 1.1)) col = mix(col, vec3(0.20, 0.15, 0.10), ink);
			if (near_wet(cw, 2.2) && !near_wet(cw, 1.2)) {
				float bleed = texture(noise_tex, cell * 0.2 + vec2(9.0, 3.0)).r;
				col = mix(col, vec3(0.30, 0.24, 0.18), bleed * 0.15);
			}
		}

		// RIVIÈRES à la PLUME + ESTUAIRE : sur l'eau OUVERTE la rivière s'évase/se fond (couleur marine),
		// dans le MARAIS elle reste un chenal lisible, sur terre un trait net.
		float rv = texture(river_map, (cell + vec2(0.5)) / map_size).r;
		float rv_noise = texture(noise_tex, cell * 0.35 + vec2(1.3, 4.7)).r;
		float plume = smoothstep(0.33, 0.46, rv) * (0.7 + 0.3 * rv_noise) + smoothstep(0.6, 0.9, rv) * 0.15;
		// EAUX TERRESTRES plus CLAIRES que la mer (inversion demandée) : rivière = bleu PÂLE lumineux.
		vec3 river_col = is_water(b) ? mix(vec3(0.64, 0.74, 0.77), antique_color(0), 0.55) : vec3(0.64, 0.74, 0.77);
		float river_amt = is_water(b) ? plume * 0.40 : (is_wetland(b) ? plume * 0.7 : plume);
		col = mix(col, river_col, river_amt);

		// RELIEF (massifs clusterisés ∝ densité → pas de hachures éparses)
		float rd = relief_density(cw);
		if (rd > 0.25) {
			float m = smoothstep(0.25, 0.65, rd);
			col = mix(col, vec3(0.76, 0.67, 0.51), m * 0.5);   // teinte massif tan (les PICS dessinent le relief)
			if (relief_density(cw + vec2(2.6, 2.6)) < 0.2 && is_solid(biome_at(cw + vec2(2.0, 2.0)))) col *= 0.90;
		}

		// ── SYMBOLES DESSINÉS à l'ENCRE (carte ancienne) : conifères/feuillus en FORÊT, pics hachurés en
		//    MONTAGNE — semis sur grille jittered, clippés à leur région (le biome du PIXEL). ──
		if (is_forest(b)) {
			vec2 grd = floor(cell * 0.30);                 // ~1 arbre / 3.3 unités monde
			uint sd = uint(max(grd.x * 7919.0 + grd.y * 6271.0 + 1048576.0, 0.0));
			if (hash(sd + 5u) > 0.16) {                    // ~16 % de cellules VIDES → casse la grille
				vec2 luv = fract(cell * 0.30) - 0.5;
				luv -= vec2(hash(sd) - 0.5, hash(sd + 1u) - 0.5) * 0.55;             // gros jitter
				col = mix(col, vec3(0.22, 0.16, 0.09), draw_tree(luv * (1.0 + hash(sd + 2u) * 0.6), sd) * 0.8);  // taille variée, glyphe REMPLIT la cellule
			}
		} else if (is_relief(b) && rd > 0.3) {
			vec2 grd = floor(cell * 0.22);                 // pics plus GROS et espacés
			uint sd = uint(max(grd.x * 8191.0 + grd.y * 5113.0 + 1048576.0, 0.0));
			if (hash(sd + 5u) > 0.12) {
				vec2 luv = fract(cell * 0.22) - 0.5;
				luv -= vec2(hash(sd) - 0.5, hash(sd + 1u) - 0.5) * 0.5;
				col = mix(col, vec3(0.20, 0.14, 0.07), draw_mountain(luv * (1.0 + hash(sd + 2u) * 0.5), sd) * 0.82);
			}
		}
	}

	// ── COMMUN : rose des vents, bords brûlés, vignette, grade chaud ──
	if (compass_on > 0.5) {
		vec2 cwld = map_size * compass_uv;
		vec2 ci = (flat_map > 0.5) ? cwld : vec2(cwld.x - cwld.y, (cwld.x + cwld.y) * 0.5);
		col = mix(col, vec3(0.20, 0.14, 0.09), compass(iso, ci, compass_r) * 0.72);
	}
	float edge = pow(clamp(length(SCREEN_UV - 0.5) * 1.42, 0.0, 1.0), 2.4);
	float bn = texture(noise_tex, SCREEN_UV * 3.3 + vec2(0.5, 0.2)).r;
	col = mix(col, vec3(0.26, 0.18, 0.10), clamp(edge * (0.55 + 0.6 * bn) * burn, 0.0, 0.72));
	col *= 1.0 - vignette * edge;
	col = mix(col, col * vec3(1.09, 1.0, 0.84), 0.45);
	COLOR = vec4(col, 1.0);
}
