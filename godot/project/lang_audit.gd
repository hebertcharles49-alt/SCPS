extends Node
## lang_audit — pendant headless du lot E « version English ». Prouve, à travers la
## GDExtension LIVE : (1) le binding lang_set/lang_get bascule la table MOTEUR (0=FR
## 1=EN, round-trip) ; (2) le CSV i18n/ui.csv est chargé et tr() rend l'anglais sous
## locale « en » ; (3) le SHELL rend l'anglais de bout en bout (options.cfg « en » →
## MenuRoot instancié → un bouton du menu affiche « Play »).
## Lancer : godot --headless res://lang_audit.tscn

const CFG_PATH := "user://options.cfg"

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("lang_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2); return
	var w = Sim.world
	print("=== LANG AUDIT — switch FR/EN (moteur + chrome + shell) ===")
	var viol := 0

	# INVARIANT 0 : les scripts du shell compilent
	for s in ["res://ui/menu_root.gd", "res://ui/new_game_panel.gd", "res://ui/options_panel.gd"]:
		if load(s) == null:
			push_error("lang_audit: ne compile pas : " + s); viol += 1

	# INVARIANT 1 : le binding moteur existe et fait le round-trip 0→1→0
	for m in ["lang_set", "lang_get"]:
		if not w.has_method(m):
			push_error("lang_audit: méthode absente du binding : " + m); viol += 1
	if viol > 0:
		print("LANG AUDIT : ", viol, " VIOLATION(S)"); get_tree().quit(1); return
	w.lang_set(1)
	if int(w.lang_get()) != 1: viol += 1; push_error("lang_set(1) → lang_get() != 1")
	w.lang_set(0)
	if int(w.lang_get()) != 0: viol += 1; push_error("lang_set(0) → lang_get() != 0")
	print("  moteur : round-trip lang_set 0→1→0 OK")

	# INVARIANT 2 : le chrome — tr() rend l'anglais sous locale « en »
	TranslationServer.set_locale("en")
	var play_en := tr("T_MENU_PLAY")
	var ng_en := tr("T_NG_TITLE")
	TranslationServer.set_locale("fr")
	var play_fr := tr("T_MENU_PLAY")
	print("  chrome : T_MENU_PLAY en=« ", play_en, " » fr=« ", play_fr, " » · T_NG_TITLE en=« ", ng_en, " »")
	if play_en != "Play": viol += 1; push_error("tr(T_MENU_PLAY) en ≠ Play : " + play_en)
	if ng_en != "New game": viol += 1; push_error("tr(T_NG_TITLE) en ≠ New game : " + ng_en)
	if play_fr != "Jouer": viol += 1; push_error("tr(T_MENU_PLAY) fr ≠ Jouer : " + play_fr)

	# INVARIANT 3 : le SHELL de bout en bout — options.cfg « en » → MenuRoot
	# instancié → un bouton du menu rend « Play » (la persistance gouverne les textes)
	var cfg := ConfigFile.new()
	var had_cfg := cfg.load(CFG_PATH) == OK
	var old_lang = cfg.get_value("options", "lang", null) if had_cfg else null
	cfg.set_value("options", "lang", "en")
	cfg.save(CFG_PATH)
	var menu: Control = load("res://ui/menu_root.gd").new()
	add_child(menu)
	await get_tree().process_frame
	var found := _find_button_text(menu, "Play")
	print("  shell : bouton « Play » ", "TROUVÉ" if found else "ABSENT", " (menu bâti sous options.cfg lang=en)")
	if not found: viol += 1; push_error("le menu ne rend pas l'anglais (bouton Play absent)")
	menu.queue_free()
	# remise en état : la config d'avant (ou lang=fr si aucune), moteur en FR
	if old_lang != null: cfg.set_value("options", "lang", old_lang)
	else: cfg.set_value("options", "lang", "fr")
	cfg.save(CFG_PATH)
	TranslationServer.set_locale("fr")
	w.lang_set(0)

	print("")
	print("LANG AUDIT OK" if viol == 0 else ("LANG AUDIT : " + str(viol) + " VIOLATION(S)"))
	get_tree().quit(0 if viol == 0 else 1)

func _find_button_text(root: Node, txt: String) -> bool:
	if root is Button and String(root.text) == txt:
		return true
	for c in root.get_children():
		if _find_button_text(c, txt):
			return true
	return false
