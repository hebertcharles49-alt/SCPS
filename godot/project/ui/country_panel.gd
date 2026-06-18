extends PanelContainer
## CountryPanel — la membrane d'un PAYS, en MOTS + métriques 0-100. Lit
## Sim.world.country_info (Dictionary). Comme la province : aucun flottant SCPS,
## seulement des mots résolus et des nombres tangibles (pop, or) ou des
## projections de jeu (les jauges 0-100, posées par la membrane C).
##
## Bâti en code. Caché tant qu'aucun pays n'est désigné.

const COL_TITLE := Color(0.86, 0.78, 0.52)   ## or pâle (titre pays)
const COL_KEY   := Color(0.60, 0.60, 0.62)
const COL_VAL   := Color(0.90, 0.90, 0.88)

const MARGIN := 8.0

var _vbox: VBoxContainer

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # ne pas bloquer le clic carte dessous
	_vbox = VBoxContainer.new()
	_vbox.add_theme_constant_override("separation", 2)
	add_child(_vbox)
	hide()

## colle le panneau en HAUT-DROITE, taille = contenu (recalculée à chaque ouverture).
func _reposition() -> void:
	var sz := get_combined_minimum_size()
	var vp := get_viewport_rect().size
	reset_size()
	position = Vector2(max(MARGIN, vp.x - sz.x - MARGIN), MARGIN)

func show_country(info: Dictionary) -> void:
	if info.is_empty() or not bool(info.get("valide", false)):
		hide()
		return
	_clear()
	_title(String(info["nom"]))
	_sub("%s · %d régions" % [info["ethos"], info["regions"]])
	_row("Population", _grouped(int(info["pop"])))
	_row("Trésor", "%s or" % _grouped(int(info["or"])))
	_sep()
	_gauge("Stabilité",  int(info["stabilite"]),  String(info["stabilite_mot"]))
	_gauge("Prospérité", int(info["prosperite"]), String(info["prosperite_mot"]))
	_gauge("Légitimité", int(info["legitimite"]), String(info["legitimite_mot"]))
	_gauge("Cohésion",   int(info["cohesion"]),   String(info["cohesion_mot"]))
	_gauge("Savoir",     int(info["savoir"]),     String(info["savoir_mot"]))
	_sep()
	_row("Influence",  str(info["influence"]))
	if int(info["corruption"]) > 0:
		_row("Corruption", str(info["corruption"]))
	show()
	_reposition.call_deferred()   # après que le layout ait calculé la taille

# ── construction ───────────────────────────────────────────────────────────
func _clear() -> void:
	for c in _vbox.get_children():
		c.queue_free()

func _title(text: String) -> void:
	var l := Label.new()
	l.text = text
	l.add_theme_color_override("font_color", COL_TITLE)
	l.add_theme_font_size_override("font_size", 18)
	_vbox.add_child(l)

func _sub(text: String) -> void:
	var l := Label.new()
	l.text = text
	l.add_theme_color_override("font_color", COL_KEY)
	_vbox.add_child(l)

func _row(key: String, val: String) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	var k := Label.new()
	k.text = key
	k.custom_minimum_size.x = 96
	k.add_theme_color_override("font_color", COL_KEY)
	var v := Label.new()
	v.text = val
	v.add_theme_color_override("font_color", COL_VAL)
	hb.add_child(k)
	hb.add_child(v)
	_vbox.add_child(hb)

## une jauge 0-100 : le mot de bande + le nombre + une barre teintée rouge→vert.
func _gauge(key: String, value: int, word: String) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	var k := Label.new()
	k.text = key
	k.custom_minimum_size.x = 96
	k.add_theme_color_override("font_color", COL_KEY)
	hb.add_child(k)
	var bar := ProgressBar.new()
	bar.min_value = 0
	bar.max_value = 100
	bar.value = value
	bar.show_percentage = false
	bar.custom_minimum_size = Vector2(90, 14)
	var fill := StyleBoxFlat.new()
	fill.bg_color = Color.from_hsv(lerpf(0.0, 0.33, clampf(value / 100.0, 0.0, 1.0)), 0.55, 0.70)
	bar.add_theme_stylebox_override("fill", fill)
	hb.add_child(bar)
	var v := Label.new()
	v.text = "%d · %s" % [value, word]
	v.add_theme_color_override("font_color", COL_VAL)
	hb.add_child(v)
	_vbox.add_child(hb)

func _sep() -> void:
	_vbox.add_child(HSeparator.new())

func _grouped(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("-" if n < 0 else "") + out
