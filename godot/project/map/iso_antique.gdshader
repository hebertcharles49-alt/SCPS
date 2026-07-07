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
uniform sampler2D cliff_map : filter_linear;   // FALAISES : intensité 0..1 (façade, dérivée du dénivelé côtier)
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

// ── V3 — LE LAVIS PAR VARIANTE : la carte SE COUVRE du visage de la fin. Une
// texture L8 (1 octet = l'intensité 0..1 de la RÉGION de cette cellule, bâtie
// côté C++/C — scps_map_endgame_variant — jamais une boucle GDScript). variant_on
// est un simple gate (0 = coût nul, la texture n'est même pas lue) posé par
// iso_ground.gd quand fin>0 ; variant_col est la teinte DA de la fin courante
// (ardoise profond EAU · ardoise pâle FROID · olive sombre RONCES · terre cuite
// SANG — ASCENSION reste hors gate, pas de lavis sombre). 1 seul tap, coût nul
// au repos (fin==0 ⇒ variant_on==0 ⇒ la branche entière est sautée).
uniform sampler2D variant_map : filter_linear;
uniform vec3 variant_col = vec3(0.35, 0.48, 0.62);
uniform float variant_on = 0.0;
uniform float variant_k : hint_range(0.0, 1.0) = 0.55;

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
// CONSCIENTE DE L'EAU : un voisin de l'autre côté de la ligne d'eau NE compte PAS — le bleu ne
// bave plus dans la plage ni la plage dans la mer (la « bouillie » de côte venait de là).
vec3 antique_base(vec2 c, int bc) {
	bool wc = is_water(bc);
	vec3 a = antique_color(bc); float n = 1.0;
	int b1 = biome_at(c + vec2(2.3, 0.7));   if (b1 >= 0 && is_water(b1) == wc) { a += antique_color(b1); n += 1.0; }
	int b2 = biome_at(c + vec2(-1.8, 1.9));  if (b2 >= 0 && is_water(b2) == wc) { a += antique_color(b2); n += 1.0; }
	int b3 = biome_at(c + vec2(0.6, -2.4));  if (b3 >= 0 && is_water(b3) == wc) { a += antique_color(b3); n += 1.0; }
	int b4 = biome_at(c + vec2(-2.1, -1.3)); if (b4 >= 0 && is_water(b4) == wc) { a += antique_color(b4); n += 1.0; }
	int b5 = biome_at(c + vec2(1.5, 2.2));   if (b5 >= 0 && is_water(b5) == wc) { a += antique_color(b5); n += 1.0; }
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
// fraction de TERRE FERME sur un anneau (CONTINU 0..1) → proximité de côte DOUCE (pas de bandes discrètes).
float ring_solid(vec2 cell, float r) {
	float n = 0.0;
	for (float a = 0.0; a < 6.2831; a += 1.0472)
		if (is_solid(biome_at(cell + vec2(cos(a), sin(a)) * r))) n += 1.0;
	return n / 6.0;
}
// fraction d'EAU SALÉE (mer, PAS marais) sur un anneau — proximité de mer CONTINUE pour la
// fusion fleuve→mer (l'ancien near_wet booléen à 3 paliers imprimait des BANDES d'estuaire).
float ring_sea(vec2 cell, float r) {
	float n = 0.0;
	for (float a = 0.0; a < 6.2831; a += 1.0472)
		if (is_water(biome_at(cell + vec2(cos(a), sin(a)) * r))) n += 1.0;
	return n / 6.0;
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

// (draw_tree / draw_mountain RETIRÉS : la carte n'imprime plus de glyphes arbre/pic — lavis seuls.)

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
		// ── LOD ANTI-BAVE (dérivées écran) : les bruits du lavis sont calés en MONDE pour le
		// plan moyen — au zoom PROFOND, magnifiés et interpolés, ils COULENT en taches molles
		// (« ça bave »). `deep` (px écran/cellule) ESTOMPE les taches larges au rapproché et
		// fait monter à la place une DENT DE PAPIER fine — le détail suit l'œil. ──
		float ppc = 1.0 / max(fwidth(cell.x), 1e-5);
		float zdeep = clamp((ppc - 5.0) / 9.0, 0.0, 1.0);
		// grain de PAPIER (marbrure warpée = les grandes volutes ; atténuée au zoom profond)
		vec2 wv = vec2(texture(noise_tex, cell * 0.02).r,
		               texture(noise_tex, cell * 0.02 + vec2(4.0, 1.0)).r) - 0.5;
		float g = texture(noise_tex, (cell + wv * 7.0) * 0.18).r;
		float g2 = texture(noise_tex, cell * 0.012 + vec2(3.0, 7.0)).r;
		// l'EAU reste PROPRE : quasi pas de grain/marbrure sur la mer (l'encre d'eau n'est
		// pas du papier) ; le marais garde un demi-grain (c'est de la terre humide).
		float grain_s = is_water(b) ? paper_grain * 0.12 : (is_wetland(b) ? paper_grain * 0.5 : paper_grain);
		grain_s *= mix(1.0, 0.40, zdeep);
		col *= mix(1.0 - grain_s, 1.0 + grain_s, g) * mix(0.93, 1.05, g2);
		float fine = texture(noise_tex, cell * 1.35 + vec2(9.0, 4.0)).r;   // la DENT fine (zoom profond)
		col *= mix(1.0, mix(0.958, 1.042, fine), zdeep * (is_water(b) ? 0.15 : 1.0));
		// VARIATION PERLIN intra-biome : jitter RGB ±rgb_jitter/255 PAR CANAL (3 bruits décorrélés → micro
		// dérive de teinte, pas juste de luminance) → les aplats RESPIRENT au lieu d'être morts.
		// Atténuée au zoom profond (les blobs de teinte magnifiés participaient au bavage).
		vec3 jit3 = vec3(vnoise(cell * jitter_freq),
		                 vnoise(cell * jitter_freq + vec2(37.2, 11.5)),
		                 vnoise(cell * jitter_freq + vec2(91.7, 53.1))) - 0.5;
		col += jit3 * (2.0 * rgb_jitter / 255.0) * mix(1.0, 0.45, zdeep) * (is_water(b) ? 0.25 : 1.0);

		if (is_water(b)) {
			// MER : assombrissement CÔTIER CONTINU (proximité de terre lissée → plus de bandes discrètes
			// « bancs de sable ») + écume FINE et douce au tout bord. Estuaire (marais) reste OUVERT.
			float coast = max(ring_solid(cw, 1.3), ring_solid(cw, 2.5) * 0.65);   // 0..1 doux
			col *= mix(1.0, 0.88, coast);                                        // bord mer ADOUCI (fade)
			float foam = clamp(ring_solid(cw, 1.1) - ring_solid(cw, 0.55), 0.0, 1.0);   // anneau IMMÉDIAT
			col = mix(col, vec3(0.74, 0.79, 0.78), foam * 0.10);                  // écume TRÈS subtile (bord fondu)
			col *= mix(1.0, 0.97, smoothstep(0.6, 1.0, sin(iso.y * 1.6)));        // gravure
		} else if (is_wetland(b)) {
			// MARAIS : SYMBOLE de tirets horizontaux (cartographie) — DISCRET (le marais se FOND dans le
			// papier, plus un semis de « trucs » noirs) ; wetland reste « humide » → estuaire ouvert.
			float rows = sin(iso.y * 0.9 + texture(noise_tex, cell * 0.1).r * 2.0);
			float rowOn = smoothstep(0.40, 0.75, rows);
			float dash = sin(iso.x * 1.7 + step(0.0, rows) * 1.6);     // tirets décalés rangée/rangée
			col = mix(col, col * 0.88, rowOn * smoothstep(0.2, 0.7, dash) * 0.40);
		} else {
			// TERRE FERME : la rive déteint en voile humide COURT — resserré (l'ancien dégradé
			// sur 4 cellules « bavait » en permanence autour de toute eau) ; la côte reste un
			// gradient peint, mais tenu contre le rivage.
			if (near_wet(cw, 2.6)) col = mix(col, vec3(0.51, 0.60, 0.60), 0.040);
			if (near_wet(cw, 1.6)) col = mix(col, vec3(0.47, 0.57, 0.59), 0.070);
			if (near_wet(cw, 1.0)) col = mix(col, vec3(0.41, 0.47, 0.47), 0.105);   // bord = voile humide (pas d'encre dure)
			// le FIL D'EAU : un liseré d'encre FANÉE au contact immédiat (la ligne que le
			// cartographe pose au pinceau fin) — la côte se LIT, les frontières y meurent
			// proprement. Inégal (bruit), doux (α faible) : un trait peint, pas un vecteur.
			float shoreink = clamp(1.0 - ring_solid(cw, 0.85), 0.0, 1.0) * float(near_wet(cw, 1.0));
			if (shoreink > 0.02) {
				float sg = texture(noise_tex, cell * 0.7 + vec2(3.3, 6.1)).r;
				col = mix(col, vec3(0.34, 0.32, 0.27), shoreink * (0.16 + 0.10 * sg));
			}

			// ── FALAISES MARITIMES : hachures d'atlas ancien — petits traits d'encre
			// PERPENDICULAIRES à la côte, espacés le long du rivage, sobres et
			// translucides (GLAZE : un lavis d'encre, jamais un aplat opaque). La
			// tangente vient du GRADIENT du champ falaise (bande fine ⊥ rivage) ;
			// les lignes constantes-t courent donc LE LONG de la normale. Intensité
			// ∝ dénivelé (cliff_map, façade) — la falaise ÉMERGE du moteur, ici on
			// ne fait que l'encrer.
			float cf = texture(cliff_map, cw / map_size).r;
			if (cf > 0.05) {
				vec2 ce1 = vec2(1.4, 0.0);
				vec2 ce2 = vec2(0.0, 1.4);
				vec2 cgr = vec2(texture(cliff_map, (cw + ce1) / map_size).r - texture(cliff_map, (cw - ce1) / map_size).r,
				                texture(cliff_map, (cw + ce2) / map_size).r - texture(cliff_map, (cw - ce2) / map_size).r);
				float cgl = length(cgr);
				if (cgl > 1e-4) {
					vec2 ctan = vec2(-cgr.y, cgr.x) / cgl;                 // tangente à la côte
					float ct = dot(cell, ctan);                            // abscisse LE LONG du rivage
					float cph = texture(noise_tex, cell * 0.07 + vec2(4.2, 8.8)).r * 5.0;   // phase jittée (casse la régularité)
					float tick = smoothstep(0.60, 0.92, sin(ct * 2.3 + cph));               // traits espacés ~2.7 cellules
					col = mix(col, vec3(0.32, 0.28, 0.22), tick * cf * 0.34);               // encre brune translucide
				}
			}
		}

		// RIVIÈRE = ENCRE FANÉE mais OPAQUE (pas un bleu uniforme « Google Maps », mais le sol ne TRANSPARAÎT
		// PAS — c'est moche). Couleur DÉSATURÉE avec PROFONDEUR : HAUT-FOND bleu-vert pâle à la berge → CHENAL
		// bleu-gris au cœur ; teinte variée par le grain du papier. Seul le BORD (la berge) se fond doucement.
		// ADOUCISSANT : champ débit moyenné sur 5 taps (centre + 4 voisins) → berges DOUCES, pas crénelées.
		float rv = texture(river_map, (cell + vec2(0.5)) / map_size).r * 0.36
		         + texture(river_map, (cell + vec2(1.4, 0.5)) / map_size).r * 0.16
		         + texture(river_map, (cell + vec2(-0.4, 0.5)) / map_size).r * 0.16
		         + texture(river_map, (cell + vec2(0.5, 1.4)) / map_size).r * 0.16
		         + texture(river_map, (cell + vec2(0.5, -0.4)) / map_size).r * 0.16;
		float rv_noise = texture(noise_tex, cell * 0.35 + vec2(1.3, 4.7)).r;
		float paper = texture(noise_tex, cell * 0.6 + vec2(7.0, 2.0)).r;   // grain du papier (teinte inégale)
		// SEUIL plus haut + transition plus douce : sous le seuil rien ne s'imprime (plus de « bancs de
		// sable » pâles dans les zones de faible débit). Le bord (berge) se fond.
		// SEUIL RELEVÉ : sous ~0.13 rien ne s'imprime → plus de lavis PÂLE de faible débit (les « artefacts »
		// que le seuil bas faisait ressortir). La berge reste douce par le `body` (opacité) + le haut du smoothstep.
		float edge  = smoothstep(0.13, 0.46, rv) * (0.92 + 0.08 * rv_noise);
		float depth = smoothstep(0.20, 0.66, rv);             // 0 = HAUT-FOND (berge) · 1 = CHENAL profond
		// l'ENCRE D'EAU reste PROPRE : dérive RGB réduite (±1.5 %) et grain de papier quasi nul
		// dans le lit — la texture appartient à la TERRE, pas à l'eau.
		vec3 water_ink = (vec3(vnoise(cell * 0.7 + vec2(11.0, 3.0)),
		                       vnoise(cell * 0.7 + vec2(5.0, 19.0)),
		                       vnoise(cell * 0.7 + vec2(23.0, 7.0))) - 0.5) * 0.03;
		vec3 shoal = vec3(0.55, 0.63, 0.60);                  // HAUT-FOND : bleu-vert PÂLE (eau peu profonde)
		vec3 deep  = vec3(0.34, 0.45, 0.54);                  // CHENAL : bleu-gris FANÉ
		vec3 riv_col = mix(shoal, deep, depth) + water_ink + (paper - 0.5) * 0.015;
		// l'encre RECOUVRE le sol (corps OPAQUE ~0.95) ; le bord fondu donne la berge — JAMAIS de terrain
		// qui transparaît à travers le lit (fade par la COULEUR désaturée, pas par la transparence).
		// AQUARELLE QUI BAVE : opacité DÉGRESSIVE — la berge est TRANSLUCIDE (le sol transparaît, l'eau y
		// « bave » comme une peinture), le CHENAL est plus plein. Plus d'aplat opaque dur. Et sur la MER,
		// l'embouchure se dissout (faible opacité). `body` rampe berge→chenal selon la profondeur.
		float body = mix(0.40, 0.93, depth);   // chenal plus PLEIN (le grain du sol ne remonte plus au milieu du lit)
		// FUSION AVEC LA MER (le fleuve côtier qui « ne merge pas ») : proximité d'eau salée sur PLUSIEURS
		// anneaux (jusqu'à 5 cellules — la bande de terre est souvent > 3). Près de la côte, l'eau du fleuve
		// (1) prend la TEINTE de la mer → jonction SANS couture, et (2) son opacité S'AMINCIT → il se dissout
		// dans la mer. Loin des côtes (to_sea=0), le fleuve garde sa couleur/pleine opacité.
		// CONTINU (fin des paliers) : fraction d'eau SALÉE sur 3 anneaux pondérés — le dégradé
		// d'estuaire est une rampe lisse, plus des bandes ; le marais ne tire plus la fusion.
		float to_sea = clamp(ring_sea(cw, 1.7) * 0.9 + ring_sea(cw, 3.2) * 0.5 + ring_sea(cw, 5.0) * 0.3, 0.0, 1.0);
		riv_col = mix(riv_col, antique_color(1), to_sea * 0.72);       // teinte → MER près de la côte
		float land_fac;
		if (is_water(b)) {
			// sur MER/LAC, l'encre de rivière ne fait qu'un SOUPÇON au rivage — l'eau garde SA
			// teinte (le double-rendu lac+rivière faisait les taches d'estuaire).
			float near_land = clamp(ring_solid(cw, 2.0), 0.0, 1.0);   // 1 au rivage → 0 au large
			land_fac = mix(0.04, 0.20, near_land);
		} else if (is_wetland(b)) {
			land_fac = 0.85;
		} else {
			land_fac = mix(1.0, 0.32, to_sea);                       // fleuve côtier : s'amincit fort près de la mer
		}
		float river_amt = edge * body * land_fac;
		col = mix(col, riv_col, clamp(river_amt, 0.0, 0.90));

		// ── la BERGE (shoreline de RIVIÈRE) : un liseré sur la TERRE qui borde le lit —
		//    PLAGE légère (sable) en plaine/pays sec, ROCAILLE grise en relief. Le rim vit
		//    juste SOUS le seuil d'impression de l'eau, et s'efface là où l'encre couvre. ──
		// le rim démarre à ras du seuil d'ENCRE (0.13) : la plage borde l'eau VISIBLE seulement
		// (plus bas, il peignait un lit sableux fantôme sur les tronçons à faible débit).
		float rim = smoothstep(0.095, 0.135, rv) * (1.0 - smoothstep(0.135, 0.24, rv)) * (1.0 - edge);
		if (rim > 0.003 && is_solid(b)) {
			bool rocky = (b >= 16 && b <= 19) || b == 23;                 // hauteurs/collines/montagnes/volcan
			vec3 shore = rocky ? vec3(0.63, 0.59, 0.52) : vec3(0.86, 0.79, 0.60);
			if (b >= 9 && b <= 11) shore = vec3(0.88, 0.81, 0.58);        // pays sec : sable plus franc
			float rimg = texture(noise_tex, cell * 0.9 + vec2(5.5, 8.5)).r;   // berge INÉGALE (pas un ourlet)
			col = mix(col, shore, rim * (0.30 + 0.24 * rimg));
		}

		// RELIEF (massifs clusterisés ∝ densité → pas de hachures éparses)
		float rd = relief_density(cw);
		if (rd > 0.25) {
			float m = smoothstep(0.25, 0.65, rd);
			col = mix(col, vec3(0.76, 0.67, 0.51), m * 0.5);   // teinte massif tan (les PICS dessinent le relief)
			if (relief_density(cw + vec2(2.6, 2.6)) < 0.2 && is_solid(biome_at(cw + vec2(2.0, 2.0)))) col *= 0.90;
		}

		// SYMBOLES DESSINÉS (arbres/pics à l'encre) : RETIRÉS — la carte garde les LAVIS de terrain
		// (vert forêt, tan massif) mais plus aucun glyphe semé ; lecture épurée, rivières en vedette.

		// ── V3 — LE LAVIS PAR VARIANTE : par-dessus le lavis de terrain, sous la rose des
		// vents/vignette (le drame de la fin teinte la carte, il ne l'efface pas). Gate à coût
		// nul (fin==0 ⇒ variant_on==0, le tap est sauté). RENFORCE la mutation physique déjà
		// peinte par le terrain (rebiome/sink/thorns) — il ne la remplace pas.
		if (variant_on > 0.5) {
			float vi = texture(variant_map, cw / map_size).r;
			col = mix(col, variant_col, clamp(vi, 0.0, 1.0) * variant_k);
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
