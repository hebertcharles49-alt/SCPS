extends PanelContainer
## ProvincePanel — la membrane d'une PROVINCE, en MOTS. Lit Sim.world.province_info
## (Dictionary : des mots DÉJÀ résolus par le moteur + des nombres tangibles). Ne
## touche JAMAIS un flottant SCPS — la cloison membrane tient aussi côté Godot.
##
## Bâti en code (pas de .tscn). Caché tant qu'aucune province n'est sélectionnée.

const COL_TITLE  := Color(0.85, 0.62, 0.36)   ## cuivre (titre)
const COL_KEY    := Color(0.60, 0.60, 0.62)   ## clé grisée
const COL_VAL    := Color(0.90, 0.90, 0.88)   ## valeur claire
const COL_FAVEUR := Color(0.46, 0.78, 0.46)   ## vert (boon)
const COL_FLEAU  := Color(0.86, 0.42, 0.40)   ## rouge (malus)
const COL_ALERT  := Color(0.92, 0.50, 0.30)   ## seuil de révolte

const MARGIN := 8.0

var _vbox: VBoxContainer

func _ready() -> void:
	# ancrage par défaut (haut-gauche) ; on POSITIONNE explicitement après avoir
	# bâti le contenu (sinon la taille est nulle au _ready → panneau mal placé).
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # ne pas bloquer le clic carte dessous
	_vbox = VBoxContainer.new()
	_vbox.add_theme_constant_override("separation", 2)
	add_child(_vbox)
	hide()

## colle le panneau en BAS-GAUCHE, taille = contenu (recalculée à chaque ouverture).
func _reposition() -> void:
	var sz := get_combined_minimum_size()
	var vp := get_viewport_rect().size
	reset_size()
	position = Vector2(MARGIN, max(MARGIN, vp.y - sz.y - MARGIN))

func show_province(info: Dictionary) -> void:
	if info.is_empty() or not bool(info.get("valide", false)):
		hide()
		return
	_clear()
	_title(String(info["nom"]))
	_sub("%s · %s âmes" % [info["stature"], _grouped(int(info["ames"]))])
	_row("Terrain", "%s · %s · %s" % [info["terrain"], info["relief"], info["climat"]])
	_row("Peuple", String(info["race"]))
	if String(info["vocation"]) != "":
		_row("Vocation", String(info["vocation"]))
	if String(info["ressource"]) != "" and String(info["ressource"]) != "—":
		_row("Ressource", String(info["ressource"]))
	_row("Humeur", "%s · %d" % [info["humeur"], info["humeur_val"]])
	_row("Aisance", "%s · %d" % [info["aisance"], info["aisance_val"]])
	_row("Agitation", str(info["agitation"]), COL_ALERT if bool(info["seuil_revolte"]) else COL_VAL)
	_row("Lignée", String(info["lignee"]))
	_row("Logements", "%s / %s" % [_grouped(int(info["logements_libres"])), _grouped(int(info["logements_cap"]))])
	_row("Services", "%s / %s" % [_grouped(int(info["services_libres"])), _grouped(int(info["services_cap"]))])
	if String(info["defense"]) != "":
		_row("Défense", String(info["defense"]))
	var mods: Array = info.get("mods", [])
	if not mods.is_empty():
		_sep()
		for m in mods:
			_mod(m)
	show()
	_reposition.call_deferred()   # après que le layout ait calculé la taille

# ── construction de lignes ─────────────────────────────────────────────────
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

func _row(key: String, val: String, val_col: Color = COL_VAL) -> void:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 8)
	var k := Label.new()
	k.text = key
	k.custom_minimum_size.x = 92
	k.add_theme_color_override("font_color", COL_KEY)
	var v := Label.new()
	v.text = val
	v.add_theme_color_override("font_color", val_col)
	hb.add_child(k)
	hb.add_child(v)
	_vbox.add_child(hb)

func _mod(m: Dictionary) -> void:
	var l := Label.new()
	var fav := bool(m.get("faveur", true))
	l.text = "%s %s" % ["▲" if fav else "▼", m.get("nom", "")]
	l.tooltip_text = String(m.get("effet", ""))
	l.add_theme_color_override("font_color", COL_FAVEUR if fav else COL_FLEAU)
	_vbox.add_child(l)

func _sep() -> void:
	_vbox.add_child(HSeparator.new())

# milliers lisibles : 12345 → "12 345"
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
