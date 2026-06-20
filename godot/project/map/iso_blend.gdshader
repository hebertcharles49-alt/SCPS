shader_type canvas_item;
// Fondu de terrain MAISON (procédural) : PRIORITÉ + bord organique.
//   · COLOR.r = config d'arêtes (NE=1 SE=2 SW=4 NW=8) · COLOR.g = assombrissement (falaise)
//   · COLOR.b < 0.5 → mode BASE : la tuile se fond VERS la mer (fade-out) sur ses arêtes d'eau.
//   · COLOR.b ≥ 0.5 → mode DÉBORDEMENT : la texture du VOISIN plus prioritaire déborde (fade-in)
//     DEPUIS ses arêtes → le terrain dominant mord sur le voisin.
// Le SEUIL du bord est ondulé par un STAMP de bruit fbm (blend_noise.png, seamless) échantillonné
// en ESPACE ISO CONTINU (repeat) → bords irréguliers MAIS raccordés d'une tuile à l'autre.
uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform float feather : hint_range(0.05, 0.5) = 0.30;
uniform float spill : hint_range(0.1, 0.6) = 0.42;       // profondeur du débordement
uniform float noise_amp : hint_range(0.0, 0.5) = 0.28;
uniform float noise_freq : hint_range(0.01, 0.4) = 0.045;

varying vec2 iso;
void vertex() { iso = VERTEX; }     // position iso continue à travers les tuiles

void fragment() {
	vec4 tex = texture(TEXTURE, UV);
	int cfg = int(COLOR.r * 15.0 + 0.5);
	float p = UV.x - 0.5;
	float q = UV.y - 0.5;
	float thr = 0.5 + (texture(noise_tex, iso * noise_freq).r - 0.5) * noise_amp;   // stamp fbm continu
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
