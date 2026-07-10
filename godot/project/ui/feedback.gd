extends CanvasLayer
## Feedback — RETOUR JOUEUR. Un bouton « Signaler un bug » toujours visible en jeu ouvre un
## panneau (type + remarque + pièces jointes) qui EXPORTE EN LOCAL un dossier horodaté sous
## user://feedback/ : la remarque, le CONTEXTE (graine · année · OS · Godot · tunables
## surchargés), le LOG Godot, un SCREENSHOT, et la sauvegarde en cours (optionnel). Un bouton
## « Ouvrir le dossier » (OS.shell_open) le remet au joueur — libre à lui de l'envoyer.
##
## CRASH : à la fermeture PROPRE (croix / menu Quitter) on efface un flag ; s'il est encore là
## au démarrage suivant, la session précédente est morte anormalement (le moteur C peut
## crasher sans qu'aucun script ne tourne) → on propose de créer un rapport avec le log.
##
## DISPLAY-ONLY : lit la façade (contexte), écrit des fichiers user:// ; zéro logique de sim.

const LOG_PATH  := "user://logs/godot.log"
const FLAG_PATH := "user://session_running.flag"
const FB_ROOT   := "user://feedback"

# charte parchemin (miroir menu_root / options_panel)
const C_PANEL := Color(0.09, 0.08, 0.06, 0.97)
const C_EDGE  := Color(0.79, 0.64, 0.29)
const C_TEXT  := Color(0.88, 0.86, 0.82)
const C_DIM   := Color(0.66, 0.62, 0.56)
const C_TITLE := Color(0.90, 0.76, 0.48)

const TYPES := ["Bug", "Plantage", "Suggestion"]

var _root: Control          # conteneur plein écran du panneau (bloque les clics derrière)
var _type: OptionButton
var _text: TextEdit
var _shot_cb: CheckBox
var _save_cb: CheckBox
var _ctx: Label
var _msg: Label
var _open_btn: Button
var _btn: Button            # le bouton flottant « Signaler un bug »
var _shot: Image = null
var _last_dir := ""


func _ready() -> void:
	layer = 80               # au-dessus de l'UI de jeu et du menu
	process_mode = Node.PROCESS_MODE_ALWAYS
	_detect_crash()          # AVANT d'armer le nouveau flag
	_arm_flag()
	_build_button()
	_build_panel()
	# le CanvasLayer n'hérite pas du thème fenêtre → on le pose à la main (charte parchemin)
	var th = get_window().theme
	if th != null:
		_btn.theme = th
		_root.theme = th


## fermeture PROPRE (croix ou menu Quitter) → on désarme le flag. Un crash natif ne passe pas ici.
func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_EXIT_TREE:
		var da := DirAccess.open("user://")
		if da != null and da.file_exists("session_running.flag"):
			da.remove("session_running.flag")


# ── flag de session (détection de crash) ─────────────────────────────────────
func _arm_flag() -> void:
	DirAccess.make_dir_recursive_absolute(FB_ROOT)
	var f := FileAccess.open(FLAG_PATH, FileAccess.WRITE)
	if f:
		f.store_string(Time.get_datetime_string_from_system())
		f.close()


func _detect_crash() -> void:
	if not FileAccess.file_exists(FLAG_PATH):
		return
	# HEADLESS (probes shot_ui/audits) : jamais de dialogue modal — il attendrait un
	# clic qui ne viendra pas et GÈLE la probe (pris 2026-07-10 : instances tuées →
	# drapeau posé → la probe suivante pendait sur ce prompt). On efface le drapeau.
	if DisplayServer.get_name() == "headless":
		DirAccess.remove_absolute(FLAG_PATH)
		return
	var when := ""
	var f := FileAccess.open(FLAG_PATH, FileAccess.READ)
	if f:
		when = f.get_as_text().strip_edges()
		f.close()
	call_deferred("_crash_prompt", when)   # l'UI n'est pas prête au tout premier _ready


func _crash_prompt(when: String) -> void:
	var dlg := ConfirmationDialog.new()
	dlg.title = "Fermeture anormale détectée"
	dlg.dialog_text = "La session précédente (%s) s'est fermée sans quitter proprement — peut-être un plantage.\n\nCréer un rapport avec le journal du plantage ?" % (when if when != "" else "?")
	dlg.ok_button_text = "Créer un rapport"
	dlg.cancel_button_text = "Ignorer"
	add_child(dlg)
	dlg.confirmed.connect(func(): _open(true))
	dlg.close_requested.connect(func(): dlg.queue_free())
	dlg.confirmed.connect(func(): dlg.queue_free())
	dlg.popup_centered()


# ── le bouton flottant « Signaler un bug » ───────────────────────────────────
func _build_button() -> void:
	_btn = Button.new()
	_btn.text = "⚑ Signaler un bug"
	_btn.name = "ReportButton"
	_btn.add_theme_font_size_override("font_size", 13)
	_btn.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_RIGHT)
	# posé au-dessus des contrôles de zoom (bas-droite), discret
	_btn.offset_left = -168; _btn.offset_top = -78
	_btn.offset_right = -12; _btn.offset_bottom = -50
	_btn.modulate = Color(1, 1, 1, 0.72)
	_btn.mouse_entered.connect(func(): _btn.modulate = Color(1, 1, 1, 1.0))
	_btn.mouse_exited.connect(func(): _btn.modulate = Color(1, 1, 1, 0.72))
	_btn.pressed.connect(func(): _open(false))
	add_child(_btn)


# ── le panneau ───────────────────────────────────────────────────────────────
func _build_panel() -> void:
	_root = Control.new()
	_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_STOP    # bloque les clics sur le jeu derrière
	_root.visible = false
	add_child(_root)

	var veil := ColorRect.new()
	veil.color = Color(0.02, 0.02, 0.03, 0.55)
	veil.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	veil.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.add_child(veil)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.add_child(center)

	var box := PanelContainer.new()
	box.custom_minimum_size = Vector2(560, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = C_PANEL; sb.border_color = C_EDGE; sb.set_border_width_all(2)
	sb.set_corner_radius_all(6); sb.set_content_margin_all(18)
	box.add_theme_stylebox_override("panel", sb)
	center.add_child(box)

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 10)
	box.add_child(col)

	var title := Label.new()
	title.text = "Signaler un bug / un retour"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", C_TITLE)
	col.add_child(title)

	# type
	var trow := HBoxContainer.new(); trow.add_theme_constant_override("separation", 8)
	var tlab := Label.new(); tlab.text = "Type :"; tlab.custom_minimum_size = Vector2(90, 0)
	tlab.add_theme_color_override("font_color", C_TEXT); trow.add_child(tlab)
	_type = OptionButton.new()
	for t in TYPES: _type.add_item(t)
	_type.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	trow.add_child(_type); col.add_child(trow)

	# remarque
	var rlab := Label.new(); rlab.text = "Décris le problème (que faisais-tu ? qu'attendais-tu ?) :"
	rlab.add_theme_color_override("font_color", C_TEXT); col.add_child(rlab)
	_text = TextEdit.new()
	_text.custom_minimum_size = Vector2(0, 130)
	_text.placeholder_text = "Ta remarque…"
	col.add_child(_text)

	# pièces jointes
	_shot_cb = CheckBox.new(); _shot_cb.text = "Joindre une capture d'écran"
	_shot_cb.button_pressed = true
	_shot_cb.add_theme_color_override("font_color", C_TEXT); col.add_child(_shot_cb)
	_save_cb = CheckBox.new(); _save_cb.text = "Joindre la sauvegarde en cours (aide à reproduire)"
	_save_cb.button_pressed = false
	_save_cb.add_theme_color_override("font_color", C_TEXT); col.add_child(_save_cb)

	# contexte auto
	_ctx = Label.new(); _ctx.add_theme_color_override("font_color", C_DIM)
	_ctx.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_ctx.custom_minimum_size = Vector2(520, 0)
	col.add_child(_ctx)

	# message de résultat
	_msg = Label.new(); _msg.add_theme_color_override("font_color", C_TITLE)
	_msg.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_msg.custom_minimum_size = Vector2(520, 0)
	col.add_child(_msg)

	# boutons
	var brow := HBoxContainer.new(); brow.add_theme_constant_override("separation", 8)
	col.add_child(brow)
	var make := Button.new(); make.text = "Créer le rapport"
	make.pressed.connect(_create); brow.add_child(make)
	_open_btn = Button.new(); _open_btn.text = "Ouvrir le dossier"; _open_btn.disabled = true
	_open_btn.pressed.connect(func(): if _last_dir != "": OS.shell_open(_last_dir))
	brow.add_child(_open_btn)
	var spacer := Control.new(); spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	brow.add_child(spacer)
	var close := Button.new(); close.text = "Fermer"
	close.pressed.connect(_close); brow.add_child(close)


# ── ouverture / fermeture ────────────────────────────────────────────────────
func _open(is_crash: bool) -> void:
	await _capture()                       # screenshot AVANT d'afficher le panneau
	_type.selected = 1 if is_crash else 0  # « Plantage » pré-sélectionné si crash détecté
	_ctx.text = _context_line()
	_msg.text = ""
	_last_dir = ""
	_open_btn.disabled = true
	if _btn != null: _btn.visible = false
	_root.visible = true


func _close() -> void:
	_root.visible = false
	if _btn != null: _btn.visible = true


func _capture() -> void:
	if _btn != null: _btn.visible = false   # ne pas capturer le bouton lui-même
	await RenderingServer.frame_post_draw
	var vp := get_viewport()
	if vp != null:
		var tex := vp.get_texture()
		if tex != null:
			_shot = tex.get_image()


# ── contexte ─────────────────────────────────────────────────────────────────
func _context_line() -> String:
	var s := "OS %s · Godot %s · %s" % [OS.get_name(), Engine.get_version_info().get("string", "?"),
		str(DisplayServer.window_get_size())]
	if Sim != null and Sim.world != null:
		s += "\nGraine %d · An %d · %s" % [Sim.current_seed, int(Sim.world.year()),
			("en partie" if Sim.game_on else "au menu")]
	else:
		s += "\nMoteur ABSENT (libscps non chargée)"
	return s


func _compose() -> String:
	var s := "=== RAPPORT SCPS ===\n"
	s += "Date        : " + Time.get_datetime_string_from_system() + "\n"
	s += "Type        : " + TYPES[_type.selected] + "\n"
	s += "\n--- Remarque du joueur ---\n" + _text.text.strip_edges() + "\n"
	s += "\n--- Contexte ---\n"
	s += "OS          : " + OS.get_name() + "\n"
	s += "Godot       : " + str(Engine.get_version_info().get("string", "?")) + "\n"
	s += "Résolution  : " + str(DisplayServer.window_get_size()) + "\n"
	s += "Locale      : " + TranslationServer.get_locale() + "\n"
	if Sim != null and Sim.world != null:
		s += "Graine      : " + str(Sim.current_seed) + "\n"
		s += "Année       : " + str(int(Sim.world.year())) + "\n"
		s += "Partie      : " + ("commencée" if Sim.game_on else "menu (vitrine)") + "\n"
		var over := PackedStringArray()
		if Sim.world.has_method("tunables"):
			for t in Sim.world.tunables():
				if bool(t.get("overridden", false)):
					over.append("%s=%s" % [str(t.get("nom", "")), str(t.get("value", ""))])
		s += "Tunables    : " + (", ".join(over) if not over.is_empty() else "aucun (vanilla)") + "\n"
	else:
		s += "Moteur      : ABSENT (libscps non chargée)\n"
	return s


# ── génération du rapport ────────────────────────────────────────────────────
func _create() -> void:
	var ts := Time.get_datetime_string_from_system().replace(":", "-").replace("T", "_").replace(" ", "_")
	var dir := FB_ROOT + "/rapport_" + ts
	var err := DirAccess.make_dir_recursive_absolute(dir)
	if err != OK and not DirAccess.dir_exists_absolute(dir):
		_msg.text = "Échec : impossible de créer le dossier (%d)." % err
		return
	# remarque + contexte
	var f := FileAccess.open(dir + "/rapport.txt", FileAccess.WRITE)
	if f: f.store_string(_compose()); f.close()
	var attached := PackedStringArray(["rapport.txt"])
	# log Godot
	if _copy_file(LOG_PATH, dir + "/godot.log"): attached.append("godot.log")
	# screenshot
	if _shot_cb.button_pressed and _shot != null:
		if _shot.save_png(dir + "/screenshot.png") == OK: attached.append("screenshot.png")
	# sauvegarde (best-effort : on ignore le nom exact, on prend les fichiers de save de user://)
	if _save_cb.button_pressed:
		var n := _copy_saves(dir)
		if n > 0: attached.append("%d sauvegarde(s)" % n)
	_last_dir = ProjectSettings.globalize_path(dir)
	_open_btn.disabled = false
	_msg.text = "Rapport créé (%s). Clique « Ouvrir le dossier » et envoie-le-moi." % ", ".join(attached)


func _copy_file(src_path: String, dst_path: String) -> bool:
	if not FileAccess.file_exists(src_path):
		return false
	var src := FileAccess.open(src_path, FileAccess.READ)
	if src == null: return false
	var data := src.get_buffer(src.get_length()); src.close()
	var dst := FileAccess.open(dst_path, FileAccess.WRITE)
	if dst == null: return false
	dst.store_buffer(data); dst.close()
	return true


## copie les fichiers de sauvegarde de user:// (nom exact inconnu → heuristique large,
## on saute le log, le dossier feedback et les .cfg triviaux).
func _copy_saves(dir: String) -> int:
	var da := DirAccess.open("user://")
	if da == null: return 0
	var n := 0
	for fn in da.get_files():
		var low := fn.to_lower()
		if low == "session_running.flag": continue
		if low.ends_with(".cfg"): continue
		if low.ends_with(".sav") or low.ends_with(".save") or low.ends_with(".dat") \
		   or low.ends_with(".scps") or low.ends_with(".bin") or low.begins_with("save"):
			if _copy_file("user://" + fn, dir + "/" + fn): n += 1
	return n
