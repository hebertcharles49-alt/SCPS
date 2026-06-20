shader_type canvas_item;
// Fondu de terrain MAISON (procédural) : PRIORITÉ + bord organique.
//   · COLOR.r = config d'arêtes (NE=1 SE=2 SW=4 NW=8) · COLOR.g = assombrissement (falaise)
//   · COLOR.b < 0.5 → mode BASE : la tuile se fond VERS la mer (fade-out) sur ses arêtes d'eau.
//   · COLOR.b ≥ 0.5 → mode DÉBORDEMENT : la tuile (texture du VOISIN plus prioritaire) déborde
//     (fade-in) DEPUIS ses arêtes → le terrain dominant mord sur le voisin.
// Le SEUIL du bord est perturbé par un bruit fbm en ESPACE ISO CONTINU → bords irréguliers MAIS
// raccordés d'une tuile à l'autre (cohérent, zéro motif par cellule).
uniform float feather : hint_range(0.05, 0.5) = 0.30;
uniform float spill : hint_range(0.1, 0.6) = 0.42;      // profondeur du débordement
uniform float noise_amp : hint_range(0.0, 0.4) = 0.24;
uniform float noise_freq : hint_range(0.02, 1.0) = 0.13;

varying vec2 iso;
void vertex() { iso = VERTEX; }     // position iso continue à travers les tuiles

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
	vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x),
	           mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) { return 0.62 * vnoise(p) + 0.38 * vnoise(p * 2.3 + 7.0); }

void fragment() {
	vec4 tex = texture(TEXTURE, UV);
	int cfg = int(COLOR.r * 15.0 + 0.5);
	float p = UV.x - 0.5;
	float q = UV.y - 0.5;
	float thr = 0.5 + (fbm(iso * noise_freq) - 0.5) * noise_amp;
	float ev[4];
	ev[0] = p - q;   // NE
	ev[1] = p + q;   // SE
	ev[2] = q - p;   // SW
	ev[3] = -p - q;  // NW
	float a;
	if (COLOR.b < 0.5) {                                   // BASE : fade-out vers la mer
		a = 1.0;
		for (int k = 0; k < 4; k++)
			if ((cfg & (1 << k)) != 0)
				a = min(a, clamp((thr - ev[k]) / feather, 0.0, 1.0));
	} else {                                               // DÉBORDEMENT : fade-in depuis l'arête
		float thr_in = thr - spill;
		a = 0.0;
		for (int k = 0; k < 4; k++)
			if ((cfg & (1 << k)) != 0)
				a = max(a, clamp((ev[k] - thr_in) / feather, 0.0, 1.0));
	}
	COLOR = vec4(tex.rgb * COLOR.g, tex.a * a);
}
