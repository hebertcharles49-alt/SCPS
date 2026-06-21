shader_type canvas_item;
// SOL = SPLAT par PIXEL, tracé en UN quad couvrant la carte iso. Pour chaque pixel :
//   1. inverse-iso → cellule monde (cx,cy) ;
//   2. on PERTURBE la position de lecture par un bruit continu → la frontière de biome devient
//      ONDULÉE (plus de losanges droits) ;
//   3. on lit le BIOME dans `biome_map` (R = biome/255) à cette position perturbée ;
//   4. on échantillonne la texture TUILÉE de ce biome (texture-array) en espace monde continu.
// → terrain CONTINU, frontières organiques, zéro grille. La PRIORITÉ : on lit AUSSI le biome voisin
//   dominant et on fond vers lui sur une bande (le dominant mord). SHORELINE : sable injecté à la
//   frontière terre/mer. Les ESCARPEMENTS (sprites) sont une COUCHE À PART (iso_cliffs), HORS de ce
//   shader : un canvas-shader avec un sampler2DArray ne lie pas la TEXTURE intégrée pour draw_*.
uniform sampler2DArray terrains : filter_linear_mipmap, repeat_enable;
uniform sampler2D biome_map : filter_nearest;
uniform sampler2D noise_tex : repeat_enable, filter_linear;
// FALAISE PLATE : autotile blob 47 états. `cliff_idx` (par cellule, R=layer+1 ; 0=pas de falaise) →
// `cliff_atlas` échantillonné en UV local à la cellule → bord de plateau (scree) auto-connecté.
// `cliff_dist` = distance au bord (cellules, /dist_max) → TERRASSES (riser de pente + tread plat).
uniform sampler2DArray cliff_atlas : filter_linear_mipmap;
uniform sampler2D cliff_idx : filter_nearest;
uniform sampler2D cliff_dist : filter_linear;
uniform vec2 cliff_grid = vec2(128.0, 64.0);   // (nx, ny) cellules
uniform float tile_k = 8.0;                     // cellules-monde par cellule d'autotile
uniform float dist_max = 24.0;                  // plafond du champ de distance (cellules)
uniform float terrace_period : hint_range(2.0, 6.0) = 3.0;   // cellules par MARCHE (riser + tread)
uniform float riser_frac : hint_range(0.15, 0.6) = 0.34;     // part de la marche en PENTE (reste = plat)
uniform float slope_shade : hint_range(0.0, 0.8) = 0.38;     // force de l'ombrage directionnel du flanc
uniform float border_feather : hint_range(0.0, 3.0) = 1.6;   // cellules de fondu vers le sol au bord
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
uniform float water_var : hint_range(0.0, 1.0) = 0.5;        // force de la variation de teinte (courants)
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
// distance au bord highland (en CELLULES) au point monde c.
float cdist(vec2 c) { return texture(cliff_dist, c / (cliff_grid * tile_k)).r * dist_max; }

void fragment() {
	vec2 cell = vec2(iso.y + iso.x * 0.5, iso.y - iso.x * 0.5);   // inverse-iso
	int b0 = biome_at(cell);                       // biome VRAI au pixel (NON ondulé)
	if (b0 < 0) { COLOR = vec4(0.0); }
	else {
		vec2 wv = vec2(texture(noise_tex, cell * warp_freq).r,
		               texture(noise_tex, cell * warp_freq + vec2(5.2, 1.3)).r) - 0.5;
		vec2 cw = cell + wv * (2.0 * warp);        // position de lecture ONDULÉE
		int b = biome_at(cw);
		// l'ondulation casse la grille SUR TERRE, mais NE FRANCHIT JAMAIS la côte : au passage
		// mer↔terre on RETOMBE sur le biome vrai → la mer ne "bave" plus sur la terre (ni l'inverse).
		if (b < 0 || (b <= 2) != (b0 <= 2)) b = b0;
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
		// FALAISE PLATE TERRASSÉE (autotile blob + champ de distance). Cellule HIGHLAND → le sol
		// DEVIENT la tuile scree (UV local NET), PUIS on la sculpte en TERRASSES concentriques :
		// chaque MARCHE = un RISER (pente 30-45°, ombré directionnellement) suivi d'un TREAD plat
		// (habillable). Le bord du plateau se FOND légèrement vers le sol dessous.
		vec2 cg = floor(cell / tile_k);
		float ci = texture(cliff_idx, (cg + vec2(0.5)) / cliff_grid).r * 255.0;
		if (ci >= 0.5) {
			vec3 ccol = texture(cliff_atlas, vec3(fract(cell / tile_k), ci - 1.0)).rgb;
			float d = cdist(cell);
			float e = tile_k * 0.75;
			vec2 g = vec2(cdist(cell + vec2(e, 0.0)) - cdist(cell - vec2(e, 0.0)),
			              cdist(cell + vec2(0.0, e)) - cdist(cell - vec2(0.0, e)));
			float gl = max(length(g), 1e-4);
			float lvl = floor(d / terrace_period);                     // NIVEAU de terrasse (0 = le plus bas)
			float f = fract(d / terrace_period);                       // position dans la marche
			float riser = 1.0 - smoothstep(riser_frac, riser_frac + 0.10, f);   // 1 = pente, 0 = tread plat
			vec2 Ld = normalize(vec2(-0.6, -0.8));                      // lumière haut-gauche
			float lit = dot(-g / gl, Ld);                              // la face regarde l'AVAL (-grad)
			ccol *= 1.0 + clamp(lvl * 0.075, 0.0, 0.45);               // pas tonal /niveau (plateau ensoleillé)
			ccol *= mix(1.0, 0.80, riser);                             // RISER toujours plus sombre (face de marche)
			ccol *= 1.0 + slope_shade * lit * riser;                   // + ombrage DIRECTIONNEL 30-45° (clair/sombre)
			ccol *= 1.0 - 0.26 * (1.0 - smoothstep(0.0, 0.12, f));     // OMBRE marquée au PIED de la marche
			ccol *= 1.0 + 0.18 * riser * smoothstep(riser_frac - 0.06, riser_frac, f);  // CRÊTE claire en haut du riser
			col = mix(col, ccol, smoothstep(0.0, border_feather, d));  // FONDU léger sur la frontière
		}
		if (sea) {
			// PROFONDEUR : deep un peu plus SOMBRE → shallow un peu plus CLAIR (doux, pas de blocs).
			float depth = (b == 0) ? 0.0 : ((b == 1) ? 0.55 : 1.0);
			col *= mix(0.82, 1.10, depth);
			// COURANTS : patches GRANDE ÉCHELLE + teinte, ATTÉNUÉS → nappe vivante mais non blocky.
			float cur = texture(noise_tex, cell * 0.012 + vec2(7.0, 2.0)).r;
			col *= mix(0.90, 1.12, cur);
			col = mix(col, col * vec3(0.86, 0.94, 1.12), (cur - 0.5) * water_var);   // bleuit/verdit par zones
			// MIROITEMENT : taches claires mouvantes.
			float shim = texture(noise_tex, cell * water_freq * 2.3
				+ vec2(TIME * water_speed * 2.0, -TIME * water_speed * 1.5)).r;
			col += vec3(0.10, 0.14, 0.18) * smoothstep(0.70, 0.95, shim) * water_shimmer;
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
