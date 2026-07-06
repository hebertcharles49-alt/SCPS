extends CanvasLayer
## PageTurn — la PAGE QUI SE TOURNE : la transition d'âge devient un vrai feuillet de codex.
## `rise(bilan_text)` : la page MONTE depuis le bas et couvre l'écran (~0.8 s), puis le texte
## de bilan s'affiche DESSUS. `turn()` : le coin se soulève en balayage diagonal (~1.2 s) et
## révèle la carte ; le nœud se cache. Tweens en HORLOGE MUR (Tween par défaut = process,
## jamais le tick moteur) — display-only, le déterminisme ne bouge pas.
##
## Robustesse : si le shader ne charge pas, `rise`/`turn` retournent immédiatement (repli sur
## l'ancien modal côté age_recap.gd — aucun crash).

signal risen         ## la page a fini de monter (le contenu peut s'afficher dessus)
signal turned        ## la page a fini de tourner (révélée, le nœud est caché)

var _rect: ColorRect
var _mat: ShaderMaterial
var _text_label: Label
var _tween: Tween
var _shader_ok := false

func _ready() -> void:
	layer = 60   # au-dessus de TOUTE l'UI (topbar/panneaux/popups) — le codex couvre tout
	visible = false

	_rect = ColorRect.new()
	_rect.color = Color(1, 1, 1, 1)   # neutre : le shader peint tout (COLOR =)
	_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	_rect.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(_rect)

	var sh := load("res://map/page_turn.gdshader")
	if sh != null:
		_mat = ShaderMaterial.new()
		_mat.shader = sh
		_mat.set_shader_parameter("noise_tex", _make_noise())
		_mat.set_shader_parameter("turn_progress", 0.0)
		_rect.material = _mat
		_shader_ok = true

	_text_label = Label.new()
	_text_label.set_anchors_and_offsets_preset(Control.PRESET_CENTER, Control.PRESET_MODE_MINSIZE)
	_text_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_text_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_text_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_text_label.custom_minimum_size = Vector2(560, 0)
	_text_label.add_theme_font_size_override("font_size", 20)
	_text_label.add_theme_color_override("font_color", Color(0.20, 0.14, 0.08))
	_text_label.visible = false
	add_child(_text_label)

func is_shader_ok() -> bool:
	return _shader_ok

## bruit fbm SEAMLESS (même famille que le parchemin de carte — grain de papier).
func _make_noise() -> NoiseTexture2D:
	var fnl := FastNoiseLite.new()
	fnl.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	fnl.frequency = 0.02
	fnl.fractal_type = FastNoiseLite.FRACTAL_FBM
	fnl.fractal_octaves = 4
	fnl.seed = 4242
	var nt := NoiseTexture2D.new()
	nt.width = 256
	nt.height = 256
	nt.seamless = true
	nt.noise = fnl
	return nt

## MONTÉE : la page couvre l'écran depuis le bas (~0.8s), puis affiche `bilan_text` dessus.
func rise(bilan_text: String) -> void:
	if not _shader_ok:
		risen.emit()   # repli : le contenu (modal legacy) s'affiche sans page
		return
	visible = true
	_text_label.visible = false
	_text_label.text = bilan_text
	if _tween != null and _tween.is_valid():
		_tween.kill()
	_mat.set_shader_parameter("turn_progress", 0.0)
	_tween = create_tween()
	_tween.tween_method(_set_progress, 0.0, 0.5, 0.8)
	_tween.tween_callback(func():
		_text_label.visible = true
		risen.emit())

## TOURNE : le coin se soulève en balayage diagonal (~1.2s), révèle le monde, se cache.
func turn() -> void:
	if not _shader_ok:
		visible = false
		turned.emit()
		return
	_text_label.visible = false
	if _tween != null and _tween.is_valid():
		_tween.kill()
	_tween = create_tween()
	_tween.tween_method(_set_progress, 0.5, 1.0, 1.2)
	_tween.tween_callback(func():
		visible = false
		turned.emit())

## redescend (repli « Plus tard ») : la page reprend sa hauteur puis se cache — sans tourner.
func lower() -> void:
	if not _shader_ok:
		visible = false
		return
	_text_label.visible = false
	if _tween != null and _tween.is_valid():
		_tween.kill()
	_tween = create_tween()
	_tween.tween_method(_set_progress, 0.5, 0.0, 0.5)
	_tween.tween_callback(func(): visible = false)

func _set_progress(v: float) -> void:
	if _mat != null:
		_mat.set_shader_parameter("turn_progress", v)
