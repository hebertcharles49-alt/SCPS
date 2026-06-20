shader_type canvas_item;
// Fondu DIRECTIONNEL + bord ORGANIQUE (maison, inspiré du blendomatic AoE2 sans en lire les .dat).
// La terre se fond sur les arêtes face à l'EAU (bits COLOR.r : NE=1 SE=2 SW=4 NW=8) ; le SEUIL du
// feutre est perturbé par un BRUIT échantillonné en ESPACE ISO CONTINU (varying `iso` = VERTEX) →
// la côte ondule de façon irrégulière MAIS se raccorde d'une tuile à l'autre (cohérent, pas de
// motif par cellule). COLOR.g = assombrissement (falaise).
uniform float feather : hint_range(0.05, 0.5) = 0.30;
uniform float noise_amp : hint_range(0.0, 0.4) = 0.24;
uniform float noise_freq : hint_range(0.02, 1.0) = 0.13;

varying vec2 iso;
void vertex() { iso = VERTEX; }     // position en espace iso (continue à travers les tuiles)

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
	vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x),
	           mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) { return 0.62 * vnoise(p) + 0.38 * vnoise(p * 2.3 + 7.0); }

void fragment() {
	vec4 tex = texture(TEXTURE, UV);
	int wm = int(COLOR.r * 15.0 + 0.5);
	float p = UV.x - 0.5;
	float q = UV.y - 0.5;
	float thr = 0.5 + (fbm(iso * noise_freq) - 0.5) * noise_amp;   // seuil ondulé organique
	float a = 1.0;
	if ((wm & 1) != 0) a = min(a, clamp((thr - (p - q)) / feather, 0.0, 1.0));   // NE = eau
	if ((wm & 2) != 0) a = min(a, clamp((thr - (p + q)) / feather, 0.0, 1.0));   // SE = eau
	if ((wm & 4) != 0) a = min(a, clamp((thr - (q - p)) / feather, 0.0, 1.0));   // SW = eau
	if ((wm & 8) != 0) a = min(a, clamp((thr - (-p - q)) / feather, 0.0, 1.0));  // NW = eau
	COLOR = vec4(tex.rgb * COLOR.g, tex.a * a);
}
