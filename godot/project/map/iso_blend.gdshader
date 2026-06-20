shader_type canvas_item;
// Applique un MASQUE DE FONDU varié par cellule (banc blend.dat, atlas vertical de N variantes)
// à la tuile de terrain brute → les bords se fondent SANS répétition évidente (rotations/variance
// bakées hors-ligne ; la cellule choisit la variante via COLOR.r, l'assombrissement via COLOR.g).
uniform sampler2D mask_atlas : filter_linear, repeat_disable;
uniform int n_variants = 16;

void fragment() {
	vec4 tex = texture(TEXTURE, UV);                 // texture du biome (brute, losange)
	int v = clamp(int(COLOR.r * float(n_variants)), 0, n_variants - 1);
	vec2 muv = vec2(UV.x, (float(v) + UV.y) / float(n_variants));
	float m = texture(mask_atlas, muv).r;            // luminance du masque = fondu
	COLOR = vec4(tex.rgb * COLOR.g, tex.a * m);      // COLOR.g = assombrissement (falaise)
}
