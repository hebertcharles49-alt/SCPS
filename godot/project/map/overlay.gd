extends Node2D
## Overlay — les ACTEURS sur la carte, en espace MONDE (enfant de MapView → suit
## la caméra zoom/pan automatiquement). DISPLAY-ONLY : lit la façade (region_tier,
## army_info, centroïdes), ne calcule rien. Redessine au tick (les données bougent).
##
## Villes : un disque au centroïde, dimensionné au tier (0-5), teinté au pays.
## Armées : un losange au centroïde de leur région + une ligne vers leur but
## (marche), un anneau coloré par phase (marche/siège/bataille).

const PHASE_MARCH := 1
const PHASE_SIEGE := 2
const PHASE_BATTLE := 3

var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre

func _ready() -> void:
	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(queue_redraw)
	queue_redraw()

func _on_tick(_year: int) -> void:
	queue_redraw()

func _process(_dt: float) -> void:
	# pendant un cataclysme, on redessine en continu pour PULSER l'épicentre
	# (horloge MUR, hors déterminisme). Sinon : aucun coût (le tick suffit).
	if _cataclysm:
		queue_redraw()

## couleur stable par pays (display-only : un hash d'indice → teinte ; -1 = neutre)
func _country_color(c: int) -> Color:
	if c < 0:
		return Color(0.7, 0.7, 0.72)
	return Color.from_hsv(fmod(c * 0.137, 1.0), 0.72, 0.90)

func _phase_color(phase: int) -> Color:
	match phase:
		PHASE_SIEGE:  return Color(0.95, 0.6, 0.2)   # orange : siège
		PHASE_BATTLE: return Color(0.95, 0.25, 0.2)  # rouge : bataille
		PHASE_MARCH:  return Color(0.95, 0.95, 0.95) # blanc : marche
		_:            return Color(0.8, 0.8, 0.85)   # gris : au repos
	return Color.WHITE

func _draw() -> void:
	var w = Sim.world
	if w == null:
		return

	# ── VILLES (dessous) : un disque par région colonisée, taille ∝ tier ──────
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		if t < 0:
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var col := _country_color(w.region_owner(r))
		var radius := 0.8 + t * 0.55
		draw_circle(ctr, radius, col)
		draw_arc(ctr, radius, 0.0, TAU, 16, Color(0, 0, 0, 0.6), 0.3, true)
		draw_circle(ctr, radius * 0.35, Color(1, 1, 1, 0.55))   # cœur clair (lisibilité)

	# ── ARMÉES (dessus) : losange + ligne de marche + anneau de phase ─────────
	for c in range(w.country_count()):
		var a: Dictionary = w.army_info(c)
		if not bool(a.get("active", false)):
			continue
		var reg: int = a.get("region", -1)
		if reg < 0:
			continue
		var ctr: Vector2 = w.region_centroid(reg)
		if ctr.x < 0:
			continue
		var col := _country_color(c)
		var phase: int = a.get("phase_id", 0)

		# ligne de marche vers le but
		var dest: int = a.get("dest", -1)
		if dest >= 0 and dest != reg:
			var dctr: Vector2 = w.region_centroid(dest)
			if dctr.x >= 0:
				draw_line(ctr, dctr, Color(_phase_color(phase), 0.7), 0.5)

		# losange d'armée, posé un peu AU-DESSUS de la ville (pour ne pas la masquer)
		var ac := ctr + Vector2(0, -3.0)
		var s := 4.5
		var diamond := PackedVector2Array([
			ac + Vector2(0, -s), ac + Vector2(s, 0),
			ac + Vector2(0, s), ac + Vector2(-s, 0)])
		# halo de phase (bataille/siège ressortent), puis contour noir, puis losange
		draw_circle(ac, s + 1.6, Color(_phase_color(phase), 0.85))
		var border := PackedVector2Array([
			ac + Vector2(0, -s), ac + Vector2(s, 0),
			ac + Vector2(0, s), ac + Vector2(-s, 0), ac + Vector2(0, -s)])
		draw_polyline(border, Color(0, 0, 0, 0.9), 1.4, true)
		draw_colored_polygon(diamond, col)

	# ── ÉPICENTRE du cataclysme §27 : anneaux pulsants (le foyer de la fin) ────
	var eg: Dictionary = w.endgame_info()
	var epi: int = eg.get("epicenter_reg", -1)
	var fin: int = eg.get("fin", 0)
	_cataclysm = (fin > 0 and epi >= 0)
	if epi >= 0:
		var ec: Vector2 = w.region_centroid(epi)
		if ec.x >= 0:
			var col := _fin_color(fin)
			var t := Time.get_ticks_msec() / 1000.0
			for k in range(3):
				var rad := 7.0 + k * 6.0 + fmod(t * 5.0, 6.0)
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), 1.0, true)

func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.30, 0.55, 0.95)   # EAU : bleu
		2: return Color(0.80, 0.92, 1.00)   # FROID : blanc glacé
		3: return Color(0.35, 0.70, 0.30)   # RONCES : vert
		_: return Color(0.70, 0.35, 0.85)   # Brèche / indéterminé : violet
