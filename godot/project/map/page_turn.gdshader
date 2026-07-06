shader_type canvas_item;
// PAGE QUI SE TOURNE — le codex qui referme un âge. Quad plein écran, papier PROCÉDURAL
// (même motif de bruit FastNoiseLite que le parchemin de la carte : fond crème, bords
// brunis en vignette). Deux phases pilotées par turn_progress [0..1] :
//   0.0 → 0.5  MONTÉE : la page s'élève depuis le bord bas et couvre l'écran (le bilan
//              s'affiche DESSUS une fois montée — géré côté GDScript, ce shader ne fait
//              que peindre le papier + son bord d'ombre).
//   0.5 → 1.0  TOURNE : un balayage diagonal (façon coin de page qui se soulève) qui
//              révèle la carte dessous, avec une ombre portée qui suit le front.
// DISPLAY-ONLY : horloge murale (Tween côté script), aucune lecture moteur ici.

uniform sampler2D noise_tex : repeat_enable, filter_linear;
uniform float turn_progress : hint_range(0.0, 1.0) = 0.0;   // 0=caché · 0.5=page pleine · 1=révélée

void fragment() {
	vec2 uv = UV;   // 0..1 écran

	// ── phase MONTÉE (0..0.5→0..1 local) : la page glisse depuis le bas ──
	float rise = clamp(turn_progress / 0.5, 0.0, 1.0);
	// hauteur couverte par le papier (0 = rien, 1 = tout l'écran), easing doux
	float cover_y = 1.0 - smoothstep(0.0, 1.0, rise);   // en UV : y au-dessus duquel le papier commence

	// ── phase TOURNE (0.5..1→0..1 local) : le coin se soulève en balayage diagonal ──
	float turn = clamp((turn_progress - 0.5) / 0.5, 0.0, 1.0);
	// front diagonal qui parcourt l'écran du coin bas-droit vers le coin haut-gauche
	float diag = (uv.x + uv.y) * 0.5;
	float turn_edge = smoothstep(turn - 0.22, turn + 0.02, diag);   // 1 = encore couvert, 0 = révélé

	// couverture totale du papier à cet instant : plein pendant la montée, puis rongé par le turn
	float covered = (turn_progress < 0.5) ? step(uv.y, 1.0 - cover_y + 0.02) : turn_edge;

	if (covered < 0.02) {
		discard;   // rien à peindre ici : la carte transparaît (CanvasLayer transparent)
	}

	// ── papier PROCÉDURAL (même famille que iso_antique : crème + grain + bord bruni) ──
	vec2 pc = uv * vec2(14.0, 8.0);   // échelle de bruit en "cellules" virtuelles
	float g = texture(noise_tex, pc * 0.5).r;
	float g2 = texture(noise_tex, pc * 0.9 + vec2(5.0, 2.0)).r;
	vec3 col = vec3(0.80, 0.72, 0.54) * mix(0.92, 1.06, g) * mix(0.96, 1.03, g2);

	// vignette / bords brunis (le papier ancien)
	float edge = pow(clamp(length(uv - 0.5) * 1.35, 0.0, 1.0), 2.2);
	float bn = texture(noise_tex, uv * 3.1 + vec2(1.3, 0.4)).r;
	col = mix(col, vec3(0.30, 0.20, 0.12), clamp(edge * (0.5 + 0.6 * bn) * 0.55, 0.0, 0.75));

	// ombre portée au FRONT (bord montant en phase 1, bord du curl en phase 2) — lit la
	// page comme une vraie feuille qui se soulève, pas un cache plat.
	float shadow = 0.0;
	if (turn_progress < 0.5) {
		float dist_to_edge = abs(uv.y - (1.0 - cover_y));
		shadow = smoothstep(0.06, 0.0, dist_to_edge) * 0.35;
	} else {
		float dist_to_edge = abs(diag - turn);
		shadow = smoothstep(0.10, 0.0, dist_to_edge) * 0.45;
	}
	col = mix(col, col * 0.5, shadow);

	COLOR = vec4(col, covered);
}
