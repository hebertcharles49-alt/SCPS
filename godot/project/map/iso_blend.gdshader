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
uniform float highland_lift : hint_range(0.0, 40.0) = 14.0;    // hauteur de falaise ABAISSÉE (~1 tuile) → relief plus DOUX, habillé de cailloux/buissons
uniform float face_dark : hint_range(0.0, 1.0) = 0.80;       // pied de face (HAUT = clair, anti near-black)
uniform float highland_warp : hint_range(0.0, 16.0) = 1.8;   // bord PEU ondulé → arêtes DROITES (anguleux, pas curvy)
uniform float ramp_cells : hint_range(1.0, 8.0) = 4.0;       // largeur (cellules) des slopes d'ACCÈS
uniform float ramp_amount : hint_range(0.0, 1.0) = 0.6;      // fréquence des accès (0 = aucun, 1 = beaucoup)
uniform float outline_w : hint_range(0.0, 4.0) = 1.4;        // demi-largeur du détourage (px écran)
uniform float outline_str : hint_range(0.0, 1.0) = 0.40;      // silhouette painterly (pas un cerne)
uniform float ridge_density : hint_range(0.1, 1.0) = 0.16;   // PEU de panneaux → grands facets, arêtes ESPACÉES (pas des stries)
uniform float fold_skew : hint_range(0.0, 0.4) = 0.05;       // cisaillement quasi-NUL → facets DROITES (pas ondulées)
uniform float fold_shade : hint_range(0.0, 0.6) = 0.16;      // contraste DOUX des grands plans (facets lisibles)
uniform float arete_w : hint_range(0.005, 0.2) = 0.06;       // largeur de l'arête (FINE)
uniform float arete_dark : hint_range(0.0, 0.8) = 0.55;      // noirceur de l'arête (OPAQUE, nette)
// ÉCLAIRAGE GLOBAL DIRECTIONNEL : UNE seule lumière (haut-gauche écran) éclaire sol/falaise/assets.
// `light_world` = direction MONDE vers la source (un pas `cell + light_world*k` va vers la lumière à
// l'écran). Les falaises projettent une OMBRE PORTÉE douce sur le sol côté OPPOSÉ à la lumière.
uniform vec2 light_world = vec2(-0.95, -0.32);
uniform float shadow_str : hint_range(0.0, 0.7) = 0.48;      // force de l'ombre portée des falaises (franche au pied)
uniform float shadow_reach : hint_range(2.0, 20.0) = 14.0;   // portée de l'ombre (unités monde)
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

// ── RIVIÈRES CARVÉES (worldgen) : `river_map` = DÉBIT accumulé par cellule (c->river/255), un champ
//    LISSE (filtre linéaire). On rend l'eau DANS le relief : cœur d'eau PROPRE là où le débit ≥ core ;
//    bords FONDUS entre edge et core. Le débit est FORT le long du fil (continuité = cœur continu) et
//    TOMBE en travers des berges (le fondu n'est qu'aux bords). Un fleuve plus FIN (débit moindre) reste
//    tout en zone de fondu → plus mélangé (le « blend supplémentaire » des petits cours). ─────────────
uniform sampler2D river_map : filter_linear;
// La RIVIÈRE est de l'EAU pour la SHORELINE (unifiée mer+rivière) : le SABLE de berge et le confluent
// rivière↔mer sont gérés par la MÊME pipeline que la côte (terr(3)) → continuité, pas de barrière de
// sable au débouché. Ici on ne règle que le seuil d'EAU et le méandrage des berges.
uniform float river_water : hint_range(0.0, 1.0) = 0.40;     // champ de débit ≥ ça = EAU (abaissé : cœur plus large, moins de sable)
uniform float river_band  : hint_range(0.02, 0.5) = 0.14;    // rampe du cœur d'eau (berge→eau)
uniform float river_warp  : hint_range(0.0, 6.0) = 0.0;      // 0 = berge NETTE et CONTINUE (un warp brise le fil fin) ; la berge de sable de la shoreline texture déjà le bord

// ── ROUTES en CHAMP LISSE (modèle de la RIVIÈRE) : `road_cov` = couverture scalaire par cellule (filtre
//    linéaire), rasterisée depuis les polylignes. On lit le champ → BORD terreux (road_worn) + CENTRE pavé
//    (tuile seamless `road_pave` tuilée sur le plan du sol), avec WARP de bruit sur le bord. Largeur
//    variable = l'amplitude du champ (plus large près des bourgs, plus étroit en forêt — encodé au carve). ─
uniform sampler2D road_cov : filter_linear;                  // couverture de route 0..1 (champ lisse)
uniform sampler2D city_wear : filter_linear;                 // SOL URBAIN 0..1 (terre battue sous les bourgs, champ lisse)
uniform sampler2D road_pave : filter_linear, repeat_enable;  // tuile pavé seamless (centre de chaussée)
uniform float road_on = 0.0;
uniform float road_dirt_edge = 0.30;                         // seuil champ : début du bord terreux (HAUSSÉ → sente étroite)
uniform float road_pave_edge = 0.62;                         // seuil champ : début du centre pavé
uniform float road_warp = 0.08;                              // amplitude du warp de bruit sur le bord (resserré)
uniform float road_period = 5.0;                             // période monde de la tuile pavé (petit = pavés gros)
uniform vec3 road_worn = vec3(0.60, 0.53, 0.44);             // TEINTE de la sente : MULTIPLIE le terrain (sol usé/tassé)

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
// 1.0 si le pixel (cellW = cwp) est COUVERT par la falaise (sommet OU face) — sert au DÉTOURAGE.
float cliff_cov(vec2 cwp) {
	for (int i = 0; i <= 6; i++) {
		if (highland_at(cwp + vec2(highland_lift * float(i) / 6.0)) > 0.5) return 1.0;
	}
	return 0.0;
}
// SOMMET du plateau : le TERRAIN ALENTOUR (biome non-highland le plus proche : terreux/herbe/neige),
// RELEVÉ — la roche n'est QUE sur la face (comme AoE). VARIANCE : taches claires/sombres (pas un aplat).
vec3 cliff_top(vec2 c) {
	// SOMMET du massif = le BIOME voisin (herbe/sable/neige) assigné à cliff_top_biome, propre (le fix
	// du blend inter-biome empêche la « soupe » de couleurs voisines de contaminer le highland).
	int bb = int(texture(cliff_top_biome, (floor(c / tile_k) + vec2(0.5)) / cliff_grid).r * 255.0 + 0.5);
	if (!((bb >= 3 && bb <= 11) || bb == 20)) bb = 6; // hors terrain ouvert/neige → herbe (jamais roche/forêt sombre sur le sommet)
	vec3 r = terr(bb, fract(c / period), c);
	float n = texture(noise_tex, c * 0.03 + vec2(2.0, 6.0)).r;
	float n2 = texture(noise_tex, c * 0.09 + vec2(7.0, 1.0)).r;
	r *= mix(0.90, 1.12, n) * mix(0.95, 1.05, n2);   // variance APAISÉE (ex-0.80/1.26 : le sommet marbrait)
	return r * 1.08;
}
// FACE de roche (paroi) — base CLAIRE GARANTIE (couleur gris-brun, jamais la texture sombre brute).
// ARÊTES = la paroi est PLIÉE comme un ÉVENTAIL : des panneaux verticaux le long de la falaise, chacun
// un FACET dont l'angle ALTERNE (~30°/60°) → la texture est CISAILLÉE (pliée) et chaque flanc est
// OMBRÉ selon son angle (un côté capte la lumière, l'autre est dans l'ombre) → le PLI fait l'arête.
vec3 cliff_face(vec2 c, float t) {
	float ci = max(texture(cliff_idx, (floor(c / tile_k) + vec2(0.5)) / cliff_grid).r * 255.0, 1.0);
	float hx = c.x - c.y;                       // le long de la paroi (≈ const en colonne d'écran)
	float p = hx * ridge_density;
	float fp = fract(p);                        // position dans le panneau (0..1)
	float ang = (mod(floor(p), 2.0) < 0.5) ? 0.52 : 1.05;   // ~30° puis ~60° (l'éventail)
	float side = fp - 0.5;                      // <0 flanc gauche du pli, >0 flanc droit
	float skew = side * tan(ang) * fold_skew;   // la texture PLIÉE (cisaillée verticalement ∝ angle)
	float tex = texture(cliff_atlas, vec3(fract(vec2(c.x / tile_k, c.y / tile_k + skew)), ci - 1.0)).g;
	// COULEUR DE BASE : grès chaud, PLAT — le grain d'atlas (qui DÉFILE verticalement quand on remonte la
	// paroi → les « stries » horizontales) est QUASI-ÉTEINT (0.03, ex-0.38). La structure vient désormais
	// UNIQUEMENT des PLANS (facets) + des ARÊTES fines — pas d'une texture striée.
	vec3 base = vec3(0.74, 0.67, 0.57) * (0.97 + 0.03 * tex);
	// GRADIENT VERTICAL : sombre au pied, chaud au sommet → VOLUME (pas de bande).
	base *= mix(vec3(0.85, 0.84, 0.82), vec3(1.08, 1.05, 0.98), t);
	// FACETS ANGULAIRES : flanc gauche du pli CLAIR, flanc droit OMBRÉ (l'écart ∝ angle) → des PLANS.
	base *= clamp(1.0 - sin(ang) * sign(side) * fold_shade, 0.74, 1.26);
	// ARÊTES : lignes FINES OPAQUES aux angles — l'arête entre deux facettes (fp≈0/1) + le pli (fp≈0.5).
	// L'ossature ANGULEUSE du rocher (AUCUNE strate, aucune bande horizontale).
	float arete_edge = smoothstep(arete_w, 0.0, min(fp, 1.0 - fp));   // bord entre deux facettes
	float arete_fold = smoothstep(arete_w * 0.7, 0.0, abs(fp - 0.5)); // pli central
	float arete = max(arete_edge, arete_fold * 0.65);
	base *= 1.0 - arete * arete_dark;                                 // trait NET, sombre (opaque & fin)
	// LÈVRE SOMMITALE : lumière au sommet, ombre au pied (jamais noir).
	vec3 col = base;
	col += vec3(0.10, 0.09, 0.07) * smoothstep(0.88, 0.98, t);
	return col;
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
		// MONTAGNE (falaise OU poche carvée sous une ville OU pied de paroi) : le SOL DE BASE prend le
		// terrain OUVERT du sommet (herbe/désert/neige), JAMAIS la roche sombre du biome montagne → fini
		// le "sol noir". On vise le BIOME (18/19/23), pas le masque falaise (la carve l'a vidé sous la ville).
		if (highland_at(cell) > 0.5 || b0 == 18 || b0 == 19 || b0 == 23) {
			int bs = int(texture(cliff_top_biome, (floor(cell / tile_k) + vec2(0.5)) / cliff_grid).r * 255.0 + 0.5);
			b = ((bs >= 3 && bs <= 11) || bs == 20) ? bs : 6;
		}
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
		// FONDU inter-biome : SEULEMENT sur le SOL PLAT. Sur un highland, le `col` ici est RÉUTILISÉ par la
		// face/le sommet (cliff_face/cliff_top) ; le laisser se mélanger aux biomes voisins le transformait
		// en SOUPE → la paroi héritait d'un gris sale. On garde donc le biome PROPRE sous les falaises.
		bool is_highland = highland_at(cell) > 0.5 || b0 == 18 || b0 == 19 || b0 == 23;
		if (!is_highland) {
			vec2 dir = normalize(wv + vec2(1e-4));
			int bn = biome_at(cw + dir * blendband);
			if (bn >= 0 && bn != b && (bn > 2) == (b > 2)) {
				float edge = texture(noise_tex, cell * warp_freq * 1.7 + vec2(2.1, 8.4)).r;
				col = mix(col, terr(bn, uv, cell), clamp(0.5 + (edge - 0.5) * 1.3, 0.0, 1.0) * 0.8);
			}
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
		// SHORELINE UNIFIÉE (mer OU rivière) : le SABLE (terr(3)) se forme autour de TOUTE eau — côte
		// ET berges de rivière — et au CONFLUENT rivière↔mer il n'y a PAS de barrière (même eau) → le
		// fleuve REJOINT la mer. ÉCUME : ourlet blanc animé, côté EAU, MER seulement.
		vec2 rwv = wv + (vec2(texture(noise_tex, cell * 0.11 + vec2(1.7, 4.3)).r,
			texture(noise_tex, cell * 0.11 + vec2(6.1, 0.9)).r) - 0.5) * 0.6;   // ondulation → berges ORGANIQUES
		float rv = texture(river_map, (cell + rwv * river_warp + vec2(0.5)) / map_size).r;
		bool water_here = sea || (rv >= river_water);
		float shore = 0.0;
		float foam = 0.0;
		for (int i = 0; i < 8; i++) {
			float a = float(i) * 0.7854;
			vec2 d2 = vec2(cos(a), sin(a));
			vec2 np = cw + d2 * shore_w;
			int bs = biome_at(np);
			float rvn = texture(river_map, (np + rwv * river_warp + vec2(0.5)) / map_size).r;
			bool nbr_water = (bs >= 0 && bs <= 2) || (rvn >= river_water);
			if (nbr_water != water_here) shore = 1.0;                 // sable de part et d'autre de TOUTE rive
			if (sea && biome_at(cw + d2 * foam_w) > 2) foam = 1.0;    // écume = MER seulement
		}
		if (shore > 0.5) col = mix(col, terr(3, uv, cell), shore_str);
		if (foam > 0.5) {
			float fp = 0.55 + 0.45 * sin(TIME * 2.2 + cell.x * 0.6 + cell.y * 0.4);
			col = mix(col, vec3(0.86, 0.93, 0.98), foam_str * fp);
		}
		// RIVIÈRE CARVÉE (worldgen) : seul le CŒUR d'EAU ici — la berge de SABLE est faite par la
		// SHORELINE ci-dessus (unifiée). Jamais sur la mer (déjà eau).
		if (!sea) {
			float wamt = smoothstep(river_water, river_water + river_band, rv);
			if (wamt > 0.003) {
				vec2 ruv = fract(cell / period + (vec2(
					texture(noise_tex, cell * water_freq + vec2(TIME * water_speed, 0.0)).r,
					texture(noise_tex, cell * water_freq + vec2(0.0, -TIME * water_speed)).r) - 0.5) * water_wave);
				vec3 wcol = terr(2, ruv, cell) * vec3(1.06, 1.18, 1.30);   // eau DOUCE CLAIRE (texture PEU PROFONDE) → tranche NET sur le marais sombre, lit comme une vraie rivière
				float shim = texture(noise_tex, cell * water_freq * 2.3 + vec2(TIME * water_speed * 2.0, 0.0)).r;
				wcol += vec3(0.14, 0.18, 0.22) * smoothstep(0.62, 0.92, shim) * water_shimmer;
				col = mix(col, wcol, wamt);
			}
		}
		float vb = mix(1.0 - var_amp, 1.0 + var_amp,
			texture(noise_tex, cell * 0.02 + vec2(3.1, 7.3)).r);
		col *= vb;
		// ── SOL URBAIN (city_wear) : terre battue GRADUELLE sous les bourgs (champ lisse, blob organique).
		//    Le biome transparaît (terre battue EN FORÊT ≠ EN PLAINE) ; pas de disque, pas de frontière nette. ──
		if (!water_here) {
			float cw = texture(city_wear, cell / map_size).r;
			if (cw > 0.005) {
				// terre battue DÉRIVÉE de col (le biome déjà rendu) : forêt → brun-vert, plaine → brun-jaune,
				// neige → gris-brun. Désaturé (luminance) + bruni légèrement. PAS un brun générique.
				float lum = dot(col, vec3(0.30, 0.59, 0.11));
				vec3 dirt = mix(col, vec3(lum) * vec3(1.05, 0.95, 0.82), 0.5);
				float grit = texture(noise_tex, cell * 0.12 + vec2(8.1, 3.7)).r;   // variation basse-freq (ornières)
				float grit2 = texture(noise_tex, cell * 0.35).r;                   // grain fin (piétinement)
				dirt *= mix(0.93, 1.07, grit);
				dirt *= mix(0.97, 1.03, grit2);
				col = mix(col, dirt, cw * 0.60);
			}
		}
		// ── ROUTES en CHAMP LISSE (modèle rivière) : on lit `road_cov` (couverture scalaire, filtre
		//    linéaire) → BORD terreux (assombri road_worn) + CENTRE pavé (tuile seamless sur le plan du sol).
		//    Un WARP de bruit casse la régularité du bord. La route NE COURT PAS sur l'eau (water_here). ──
		if (road_on > 0.5 && !water_here) {
			float rc = texture(road_cov, cell / map_size).r;
			if (rc > 0.01) {
				float wn = (texture(noise_tex, cell * 0.10).r - 0.5) * road_warp;   // warp de bord (bruit)
				float rcw = rc + wn;
				float dirt = smoothstep(road_dirt_edge, road_dirt_edge + 0.14, rcw);   // bord terreux (sente usée)
				float pave = smoothstep(road_pave_edge, road_pave_edge + 0.14, rcw);   // centre pavé
				col *= mix(vec3(1.0), road_worn, dirt);                                 // sente = terrain assombri/tassé
				vec3 pav = texture(road_pave, fract(cell / road_period)).rgb;           // tuile pavé seamless (plan du sol)
				col = mix(col, pav, pave);                                              // centre PAVÉ
			}
		}
		// OMBRE PORTÉE des falaises sur le SOL : un pixel de sol est dans l'ombre s'il y a une falaise
		// EN AMONT DE LA LUMIÈRE (on marche vers `light_world`). Plus la falaise est proche, plus c'est
		// sombre (chute douce) → le plateau « décolle » du sol comme dans AoE. (Le relief écrase ensuite
		// les pixels de falaise ; l'ombre ne reste donc que sur le SOL, côté opposé à la lumière.)
		float shp = 0.0;
		for (int s = 1; s <= 6; s++) {
			if (highland_at(cell + light_world * (float(s) * shadow_reach / 6.0)) > 0.5) {
				shp = 1.0 - (float(s) - 1.0) / 6.0;
				break;
			}
		}
		col *= 1.0 - shadow_str * shp;
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
			if (fk > 0.0) {
				float tface = fk / eff_lift;
				float face_alpha = smoothstep(0.0, 0.03, tface);   // OPAQUE dès 3 % (ex-16 % : la paroi était
				col = mix(col, cliff_face(fc, tface), face_alpha);  // semi-TRANSPARENTE en bas → le terrain transparaissait)
			}
		}
		// DÉTOURAGE : trait sombre LÉGER sur la SILHOUETTE (le pixel est falaise, un voisin écran
		// VERS L'EXTÉRIEUR ne l'est pas) → contour lisible du plateau, sans cerner tout le massif.
		if (cliff_cov(cellW) > 0.5) {
			float o = outline_w;
			float edge = (1.0 - cliff_cov(cellW + vec2(o, o)))                 // bas écran
			           + (1.0 - cliff_cov(cellW + vec2(o * 0.5, -o * 0.5)))    // droite écran
			           + (1.0 - cliff_cov(cellW + vec2(-o * 0.5, o * 0.5)));   // gauche écran
			col = mix(col, col * 0.62, clamp(edge, 0.0, 1.0) * outline_str);
		}
		COLOR = vec4(col, 1.0);
	}
}
