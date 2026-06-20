shader_type canvas_item;
// Fondu DIRECTIONNEL cohérent : la tuile de terre ne se fond QUE sur les arêtes du losange qui
// font face à une cellule d'EAU (bits passés par COLOR.r : NE=1 SE=2 SW=4 NW=8) → le rivage suit
// la côte réelle et raccorde avec la voisine (pas de stamp par cellule). COLOR.g = assombrissement.
uniform float feather : hint_range(0.1, 0.5) = 0.40;

void fragment() {
	vec4 tex = texture(TEXTURE, UV);
	int wm = int(COLOR.r * 15.0 + 0.5);
	float p = UV.x - 0.5;
	float q = UV.y - 0.5;
	float a = 1.0;
	if ((wm & 1) != 0) a = min(a, clamp((0.5 - (p - q)) / feather, 0.0, 1.0));   // arête NE = eau
	if ((wm & 2) != 0) a = min(a, clamp((0.5 - (p + q)) / feather, 0.0, 1.0));   // arête SE = eau
	if ((wm & 4) != 0) a = min(a, clamp((0.5 - (q - p)) / feather, 0.0, 1.0));   // arête SW = eau
	if ((wm & 8) != 0) a = min(a, clamp((0.5 - (-p - q)) / feather, 0.0, 1.0));  // arête NW = eau
	COLOR = vec4(tex.rgb * COLOR.g, tex.a * a);
}
