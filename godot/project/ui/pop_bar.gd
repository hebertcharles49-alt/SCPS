extends RefCounted
## PopBar — le widget FRISE de proportions PARTAGÉ (façon Victoria 3) : une barre
## horizontale pleine largeur segmentée (un ColorRect par membre, taille ∝ part) +
## sa légende à pastilles. DRY entre l'onglet Population de empire_window et l'onglet
## Démographie de province_panel_v2.
##
## Display-only, ZÉRO `_draw` : les segments sont des ColorRect NATIFS à
## `size_flags_stretch_ratio`. Les couleurs sont une PALETTE MUETTE parcheminée —
## aucun lecteur façade ne donne la couleur d'une culture/foi (seul l'owner de pays
## a un pigment, cf. border_segments_col), donc codage cohérent PAR RANG (le membre
## dominant garde l'ocre, etc.). Même couleur dans le segment ET sa pastille de légende.

const ParchTheme = preload("res://ui/parch_theme.gd")

## palette catégorielle SOURDE, accordée au parchemin (jamais criarde) : ocre · sienne ·
## olive · ardoise · prune · sarcelle · laiton · indigo doux. Boucle au-delà de 8.
const PALETTE := [
	Color("a8894e"),  # ocre
	Color("8c5a3c"),  # sienne
	Color("6f7a45"),  # olive
	Color("5a6b73"),  # ardoise
	Color("7a5568"),  # prune
	Color("4f7d78"),  # sarcelle
	Color("9a7b3e"),  # laiton
	Color("6b5b8a"),  # indigo doux
]

static func color_at(rank: int) -> Color:
	return PALETTE[rank % PALETTE.size()]

## construit la liste de membres TRIÉE décroissante depuis un map nom->valeur (âmes).
## Chaque membre : {name, value, pct(0-100 entier), color}. total<=0 ⇒ [].
static func members_from_map(m: Dictionary, total: float, cap := 8) -> Array:
	if total <= 0.0:
		return []
	var arr := []
	for k in m:
		var v := float(m[k])
		if v < 1.0:
			continue
		arr.append({"name": String(k), "value": v})
	arr.sort_custom(func(a, b): return float(a["value"]) > float(b["value"]))
	var out := []
	for i in range(arr.size()):
		if i >= cap:
			break
		var e: Dictionary = arr[i]
		e["pct"] = int(round(100.0 * float(e["value"]) / total))
		e["color"] = color_at(i)
		out.append(e)
	return out

## la BARRE seule : cadre parcheminé pleine largeur, ~height px, un ColorRect/membre
## (ratio = value). Membres vides ⇒ un segment neutre plein (honnête : monolithe).
## Le DÉTAIL est en HOVER : chaque segment porte « Nom · N% · effectif », et le cadre
## entier porte la ventilation COMPLÈTE (pour les tranches trop fines à survoler).
static func proportion_bar(members: Array, height := 14) -> Control:
	var frame := PanelContainer.new()
	frame.add_theme_stylebox_override("panel",
		ParchTheme.sb(ParchTheme.PANEL_BG, ParchTheme.BORDER, 1, 3, 0, 0, 0, 0))
	frame.custom_minimum_size = Vector2(0, height)
	frame.clip_contents = true
	var box := HBoxContainer.new()
	box.add_theme_constant_override("separation", 0)
	frame.add_child(box)
	if members.is_empty():
		var empty := ColorRect.new()
		empty.color = ParchTheme.DIVIDER
		empty.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		box.add_child(empty)
		return frame
	var full := ""
	for e in members:
		var seg := ColorRect.new()
		seg.color = e["color"]
		seg.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		seg.size_flags_stretch_ratio = maxf(float(e["value"]), 0.0001)
		seg.mouse_filter = Control.MOUSE_FILTER_STOP
		var lbl := "%s · %d %% · %s" % [String(e["name"]), int(e.get("pct", 0)), _grp(int(round(float(e["value"]))))]
		seg.tooltip_text = lbl
		box.add_child(seg)
		full += lbl + "\n"
	frame.tooltip_text = full.strip_edges()   # ventilation complète au survol du cadre
	frame.mouse_filter = Control.MOUSE_FILTER_PASS
	return frame

## GROUPE HOVER-ONLY : juste la barre (détail au survol), PAS de légende toujours affichée.
## C'est le mode des panneaux DENSES (fiche province) — le retour joueur « le détail en hover ».
static func build_bar_only(into: VBoxContainer, m: Dictionary, total: float) -> void:
	var members := members_from_map(m, total)
	if members.is_empty():
		var l := Label.new()
		l.theme_type_variation = "RowDim"
		l.text = "—"
		into.add_child(l)
		return
	into.add_child(proportion_bar(members))

## une ligne de légende par membre : pastille de couleur + nom + « N % · effectif ».
## `with_count` off ⇒ « N % » seul (quand la valeur n'est pas des âmes réelles).
static func legend_rows(members: Array, into: VBoxContainer, with_count := true) -> void:
	for e in members:
		var line := HBoxContainer.new()
		line.add_theme_constant_override("separation", 6)
		into.add_child(line)
		var sw := Panel.new()
		sw.custom_minimum_size = Vector2(12, 12)
		sw.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		sw.add_theme_stylebox_override("panel",
			ParchTheme.sb(e["color"], ParchTheme.BORDER, 1, 2, 0, 0, 0, 0))
		line.add_child(sw)
		var nm := Label.new()
		nm.theme_type_variation = "RowLabel"
		nm.text = String(e["name"])
		nm.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		nm.clip_text = true
		line.add_child(nm)
		var pc := Label.new()
		pc.theme_type_variation = "RowDim"
		if with_count:
			pc.text = "%d %% · %s" % [int(e.get("pct", 0)), _grp(int(round(float(e["value"]))))]
		else:
			pc.text = "%d %%" % int(e.get("pct", 0))
		pc.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		line.add_child(pc)

## GROUPE COMPLET (frise + légende), le point d'entrée DRY : ajoute au VBox `into`
## la barre de proportions puis sa légende, depuis un map nom->âmes.
static func build_group(into: VBoxContainer, m: Dictionary, total: float, with_count := true) -> void:
	var members := members_from_map(m, total)
	if members.is_empty():
		var l := Label.new()
		l.theme_type_variation = "RowDim"
		l.text = "—"
		into.add_child(l)
		return
	into.add_child(proportion_bar(members))
	legend_rows(members, into, with_count)

## séparateur de milliers (copie locale — le widget est autonome, aucun consommateur requis).
static func _grp(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var c := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		c += 1
		if c % 3 == 0 and i > 0:
			out = " " + out
	return ("−" if n < 0 else "") + out
