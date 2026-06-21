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
uniform sampler2D cliff_top_biome : filter_nearest;   // biome du SOMMET (terrain alentour) par cellule
uniform vec2 cliff_grid = vec2(128.0, 64.0);   // (nx, ny) cellules
uniform float tile_k = 8.0;                     // cellules-monde par cellule d'autotile
uniform float dist_max = 24.0;                  // plafond du champ de distance (cellules)
uniform float slope_shade : hint_range(0.0, 0.8) = 0.38;     // force de l'ombrage de la terrasse (sommet)
// RELIEF « AoE » : chaque cellule highland est RELEVÉE de `highland_lift` (unités iso-écran) — son
// sommet monte, une FACE de roche le relie au pied → vrai escarpement en volume (pas un sol plat).
uniform float highland_lift : hint_range(0.0, 40.0) = 12.0;   // hauteur de falaise (~1.5 tuile = 12 px iso)
uniform float face_dark : hint_range(0.0, 1.0) = 0.55;       // assombrissement du PIED de la face
uniform float highland_warp : hint_range(0.0, 16.0) = 2.5;   // ondulation du bord (DOUX : angle droit à peine cassé)
uniform float ramp_cells : hint_range(1.0, 8.0) = 4.0;       // largeur (cellules) des slopes d'ACCÈS
uniform float ramp_amount : hint_range(0.0, 1.0) = 0.6;      // fréquence des accès (0 = aucun, 1 = beaucoup)
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
// highland à la cellule contenant le point monde c → 1.0 si oui (cliff_idx = (layer+1)/255 > 0), sinon 0.
float highland_at(vec2 c) {
	if (c.x < 0.0 || c.y < 0.0 || c.x >= map_size.x || c.y >= map_size.y) return 0.0;
	return texture(cliff_idx, (floor(c / tile_k) + vec2(0.5)) / cliff_grid).r > 0.0015 ? 1.0 : 0.0;
}
// SOMMET du plateau : le TERRAIN ALENTOUR (biome non-highland le plus proche : terreux/herbe/neige),
// RELEVÉ — la roche n'est QUE sur la face (comme AoE). VARIANCE : taches claires/sombres (pas un aplat).
vec3 cliff_top(vec2 c) {
	int bb = int(texture(cliff_top_biome, (floor(c / tile_k) + vec2(0.5)) / cliff_grid).r * 255.0 + 0.5);
	if (bb < 3) bb = 6;                              // repli : herbe (île de roche cernée de mer)
	vec3 r = terr(bb, fract(c / period), c);
	float n = texture(noise_tex, c * 0.03 + vec2(2.0, 6.0)).r;
	float n2 = texture(noise_tex, c * 0.09 + vec2(7.0, 1.0)).r;
	r *= mix(0.80, 1.26, n) * mix(0.92, 1.08, n2);   // variance grande + petite échelle
	return r * 1.08;
}
// FACE de roche (verticale) : scree de la cellule (UV LOCAL stable → PAS de stries), ombré en bas.
vec3 cliff_face(vec2 c, float t) {
	float ci = max(texture(cliff_idx, (floor(c / tile_k) + vec2(0.5)) / cliff_grid).r * 255.0, 1.0);
	vec3 r = texture(cliff_atlas, vec3(fract(c / tile_k), ci - 1.0)).rgb * 1.4;
	return r * mix(face_dark, 0.95, t);              // pied plus sombre (ombre portée), haut clair
}

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
		col *= vb;
		// ───── RELIEF « AoE » : highland RELEVÉ par-dessus le sol. (1) bord ONDULÉ par un bruit
		// (cellW) → contour ORGANIQUE, moins carré / arrondi. (2) SLOPES D'ACCÈS : par zones (bruit),
		// le relèvement RAMPE avec la distance au bord → un talus praticable au lieu d'une paroi.
		// (3) Le PIED de la face se FOND dans le sol → côté arrondi, moins brut.
		vec2 hw = (vec2(texture(noise_tex, cell * 0.05).r,
			texture(noise_tex, cell * 0.05 + vec2(3.7, 1.9)).r) - 0.5) * highland_warp;
		vec2 cellW = cell + hw;
		float rmask = ramp_amount * smoothstep(0.56, 0.66,
			texture(noise_tex, cellW * 0.016 + vec2(5.0, 9.0)).r);
		float eff_lift = highland_lift * (1.0 - rmask * (1.0 - clamp(cdist(cellW) / ramp_cells, 0.0, 1.0)));
		if (highland_at(cellW + vec2(eff_lift)) > 0.5) {
			col = cliff_top(cellW + vec2(eff_lift));
		} else if (eff_lift > 0.5) {
			float fk = -1.0;
			vec2 fc = cellW;
			for (int i = 1; i <= 12; i++) {
				float k = eff_lift * float(i) / 12.0;
				if (highland_at(cellW + vec2(k)) > 0.5) { fk = k; fc = cellW + vec2(k); break; }
			}
			if (fk > 0.0) col = mix(col, cliff_face(fc, fk / eff_lift),
				smoothstep(0.0, 0.16, fk / eff_lift));   // pied FONDU dans le sol (talus arrondi)
		}
		COLOR = vec4(col, 1.0);
	}
}
