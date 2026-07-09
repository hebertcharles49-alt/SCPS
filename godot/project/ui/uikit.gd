extends RefCounted
## UIKit — HABILLAGE : le pack d'assets (chrome + icônes) appliqué à l'UI Godot.
## Charge les PNG via load_img() — EXPORT-SAFE : la RESSOURCE importée (lisible dans le PCK
## du .exe) puis get_image(), avec un fallback Image.load_from_file en dev. Cache par chemin.
## (Image.load_from_file seul lit le disque réel → INVISIBLE en export : le bug corrigé.) Display-only.
##
## Consommé via `const UIKit = preload("res://ui/uikit.gd")`.

const CHROME := "res://assets/scps/ui/chrome/"
const ICONS  := "res://assets/scps/ui/icons/"
const RESOURCES := "res://assets/scps/pack/resources/"
const MAP := "res://assets/scps/pack/map/"
const PARCH := "res://assets/scps/ui/parch/"   ## les 12 planches PARCHEMIN (cellules 256², alpha)

## LE RESKIN PARCHEMIN passe par ICI : nom d'icône historique → pièce des nouvelles
## planches. Le remap est consulté en PREMIER par icon() — aucun panneau à retoucher ;
## pièce absente → l'ancienne icône (le jeu tourne à moitié reskiné sans casse).
const PARCH_ICON := {
	"gold_coin":          "sheet11_system_icons_01",
	"population_group":   "sheet11_system_icons_02",
	"grain_bundle":       "sheet11_system_icons_03",
	"knowledge_book":     "sheet11_system_icons_04",
	"action_build":       "sheet11_system_icons_05",
	"build_hammer":       "sheet11_system_icons_05",
	"action_recruit":     "sheet11_system_icons_06",
	"action_research":    "sheet11_system_icons_07",
	"action_trade":       "sheet11_system_icons_08",
	"action_treaty":      "sheet11_system_icons_09",
	"action_decree":      "sheet11_system_icons_10",
	"dipl_rivalry":       "sheet11_system_icons_11",
	"alert_revolt":       "sheet11_system_icons_12",
	"alert_famine":       "sheet11_system_icons_13",
	"alert_shortage":     "sheet11_system_icons_13",
	"alert_siege":        "sheet11_system_icons_14",
	"alert_event_bell":   "sheet11_system_icons_15",
	"alert_warning":      "sheet11_system_icons_15",
	"politics_crown":     "sheet11_system_icons_16",
	# série 2 — TOPBAR fine (planche 24 : médaillons sobres pensés pour 16 px)
	"tool_speed":         "sheet24_topbar_boats_menu_05",
	"tool_pause":         "sheet24_topbar_boats_menu_06",
	"settlement_cluster": "sheet24_topbar_boats_menu_08",
	"fine_date":          "sheet24_topbar_boats_menu_01",
	"fine_coin":          "sheet24_topbar_boats_menu_02",
	"fine_grain":         "sheet24_topbar_boats_menu_03",
	"fine_knowledge":     "sheet24_topbar_boats_menu_04",
	"fine_age":           "sheet24_topbar_boats_menu_07",
	# SIDEBAR GAUCHE (planche 23) + divers
	"menu_economy":       "sheet23_remaining_chrome_sidebar_05",
	"menu_demography":    "sheet23_remaining_chrome_sidebar_06",
	"menu_stocks":        "sheet23_remaining_chrome_sidebar_07",
	"menu_market":        "sheet23_remaining_chrome_sidebar_08",
	"menu_army":          "sheet23_remaining_chrome_sidebar_09",
	"menu_filters":       "sheet23_remaining_chrome_sidebar_10",
	"menu_diplomacy":     "sheet23_remaining_chrome_sidebar_11",
	"menu_council":       "sheet23_remaining_chrome_sidebar_12",
	"menu_kingdom":       "sheet23_remaining_chrome_sidebar_13",
	"menu_religion":      "sheet23_remaining_chrome_sidebar_14",
	"laurel_success":     "sheet23_remaining_chrome_sidebar_15",
	"mourning_veil":      "sheet23_remaining_chrome_sidebar_16",
}

## une PIÈCE parchemin par nom nu (« sheet24_topbar_boats_menu_11 ») — l'accès public.
static func parch_tex(piece: String) -> Texture2D:
	return _tex(PARCH + piece + ".png")

## FACTIONS (planche 14) : nom → blason (base + variante EN COLÈRE quand la rancœur couve)
const PARCH_FACTION := {
	"Conquérants":     ["sheet14_factions_01", "sheet14_factions_07"],
	"Marchands":       ["sheet14_factions_02", "sheet14_factions_08"],
	"Légistes":        ["sheet14_factions_03", "sheet14_factions_09"],
	"Gardiens":        ["sheet14_factions_04", "sheet14_factions_10"],
	"Transgresseurs":  ["sheet14_factions_05", "sheet14_factions_11"],
	"Communautaires":  ["sheet14_factions_06", "sheet14_factions_12"],
}
static func faction_blason(nom: String, angry: bool) -> Texture2D:
	if not PARCH_FACTION.has(nom):
		return null
	return _tex(PARCH + PARCH_FACTION[nom][1 if angry else 0] + ".png")

## CONSEILLERS (planche 13) : siège 0-7 → buste (variante féminine = +8, par graine)
static func advisor_portrait(seat: int, fem: bool = false) -> Texture2D:
	if seat < 0 or seat > 7:
		return null
	return _tex(PARCH + "sheet13_advisors_%02d.png" % (seat + 1 + (8 if fem else 0)))

## UNITÉS (planches 9-10) : aligné sur UNIT_FILE (l'ordre U_* du moteur)
const PARCH_UNIT := [
	"sheet09_unit_icons_01", "sheet09_unit_icons_02", "sheet09_unit_icons_03",
	"sheet09_unit_icons_05", "sheet09_unit_icons_06", "sheet09_unit_icons_09",
	"sheet09_unit_icons_10", "sheet10_special_units_letters_01", "sheet09_unit_icons_04",
	"sheet09_unit_icons_08", "sheet10_special_units_letters_02", "sheet10_special_units_letters_03",
	"sheet09_unit_icons_07", "sheet09_unit_icons_13", "sheet09_unit_icons_14",
	"sheet09_unit_icons_15", "sheet10_special_units_letters_04", "sheet10_special_units_letters_05",
	"sheet10_special_units_letters_06", "sheet09_unit_icons_16",
	"sheet09_unit_icons_11", "sheet09_unit_icons_12",
]
## ÉDIFICES (planches 6-7) : aligné sur BLD_FILE (l'ordre EDI_* du moteur)
const PARCH_BLD := [
	"sheet06_buildings_state_01", "sheet06_buildings_state_02", "sheet06_buildings_state_03",
	"sheet06_buildings_state_04", "sheet06_buildings_state_05", "sheet06_buildings_state_06",
	"sheet06_buildings_state_07", "sheet06_buildings_state_08", "sheet06_buildings_state_09",
	"sheet06_buildings_state_10", "sheet06_buildings_state_11", "sheet06_buildings_state_12",
	"sheet06_buildings_state_13", "sheet06_buildings_state_14", "sheet06_buildings_state_15",
	"sheet06_buildings_state_16", "sheet07_buildings_works_01", "sheet07_buildings_works_02",
	"sheet07_buildings_works_03", "sheet07_buildings_works_04", "sheet07_buildings_works_05",
	"sheet07_buildings_works_06", "sheet07_buildings_works_07", "sheet07_buildings_works_08",
	"sheet07_buildings_works_09", "sheet07_buildings_works_10",
]

## MANUFACTURES (planche 8) : nom d'affichage moteur → vignette gravée (partiel —
## les types sans pièce retombent sur le marteau générique de l'appelant).
const PARCH_MANUF := {
	"Brasserie": "sheet08_buildings_manufactures_01",
	"Distillerie": "sheet08_buildings_manufactures_02",
	"Manufacture textile": "sheet08_buildings_manufactures_03",
	"Atelier d'outillage": "sheet08_buildings_manufactures_04",
	"Armurerie lourde": "sheet08_buildings_manufactures_05",
	"Arquebuserie": "sheet08_buildings_manufactures_06",
	"Poudrière": "sheet08_buildings_manufactures_07",
	"Alambic": "sheet08_buildings_manufactures_08",
	"Foreuse arcanique": "sheet08_buildings_manufactures_09",
	"Réplicateur ligneux": "sheet08_buildings_manufactures_10",
	"Corne divine": "sheet08_buildings_manufactures_11",
	"Forge céleste": "sheet08_buildings_manufactures_12",
	# COMPLÉMENT série 3 (planche 27) — le roster manufactures est COUVERT
	"Armurerie légère": "sheet27_manufactures_complement_01",
	"Atelier d'arc": "sheet27_manufactures_complement_02",
	"Charbonnière": "sheet27_manufactures_complement_03",
	"Papeterie": "sheet27_manufactures_complement_04",
	"Scierie navale": "sheet27_manufactures_complement_05",
	"Joaillerie": "sheet27_manufactures_complement_06",
	"Atelier d'étoffe précieuse": "sheet27_manufactures_complement_07",
	"Apothicaire": "sheet27_manufactures_complement_08",
	"Atelier de tunique": "sheet27_manufactures_complement_09",
	"Atelier de mage": "sheet27_manufactures_complement_10",
	"Poterie": "sheet27_manufactures_complement_11",           # le four voûté à briques = le four du potier
	"Atelier de sculpture": "sheet27_manufactures_complement_14",
}
static func manuf_sprite(nom: String) -> Texture2D:
	if not PARCH_MANUF.has(nom):
		return null
	return _tex(PARCH + PARCH_MANUF[nom] + ".png")

## RESSOURCES (planche 19) : clé normalisée (resource_key) → chip gravée
const PARCH_RES := {
	"cereales": "sheet19_resources_01", "poisson": "sheet19_resources_02",
	"betail": "sheet19_resources_03", "fruits": "sheet19_resources_04",
	"bois": "sheet19_resources_05", "pierre": "sheet19_resources_06",
	"argile": "sheet19_resources_07", "fer": "sheet19_resources_08",
	"cuivre": "sheet19_resources_09", "or": "sheet19_resources_10",
	"sel": "sheet19_resources_11", "laine": "sheet19_resources_12",
	"fourrure": "sheet19_resources_13", "charbon": "sheet19_resources_14",
	"cristal_arcanique": "sheet19_resources_15", "fer_celeste": "sheet19_resources_16",
}

## FONDS DE PROVINCE (planches 20-22) : mot de terrain/climat → paysage enluminé
static func biome_painting(terrain: String, climat: String) -> Texture2D:
	var t := (terrain + " " + climat).to_lower()
	var piece := "sheet20_province_biomes_01_04_01"          # défaut : plaines
	if t.contains("glac") or t.contains("toundra") or t.contains("polaire"):
		piece = "sheet22_province_biomes_09_12_02"
	elif t.contains("taïga") or t.contains("taiga") or (t.contains("froid") and (t.contains("forêt") or t.contains("bois"))):
		piece = "sheet22_province_biomes_09_12_01"
	elif t.contains("jungle"):
		piece = "sheet21_province_biomes_05_08_04"
	elif t.contains("marais") or t.contains("tourb") or t.contains("mangrove"):
		piece = "sheet21_province_biomes_05_08_03"
	elif t.contains("désert") or t.contains("desert") or t.contains("dune") or t.contains("aride"):
		piece = "sheet21_province_biomes_05_08_01"
	elif t.contains("steppe") or t.contains("savane"):
		piece = "sheet21_province_biomes_05_08_02"
	elif t.contains("mont") or t.contains("pic") or t.contains("volcan"):
		piece = "sheet20_province_biomes_01_04_03"
	elif t.contains("colline") or t.contains("haut"):
		piece = "sheet22_province_biomes_09_12_03"
	elif t.contains("fleuve") or t.contains("delta") or t.contains("estuaire"):
		piece = "sheet22_province_biomes_09_12_04"
	elif t.contains("côt") or t.contains("cot") or t.contains("littoral") or t.contains("mer"):
		piece = "sheet20_province_biomes_01_04_04"
	elif t.contains("forêt") or t.contains("foret") or t.contains("bois"):
		piece = "sheet20_province_biomes_01_04_02"
	return _tex(PARCH + piece + ".png")

## TECHS : nom connu (planches 15-18) > apex/combo/faustien > FONCTION du quartier
## (planche 5 : quartier = thème×3+fonction ; fonction 0 Prod → 04, 1 Armée → 06,
## 2 Renfort → 05).
const PARCH_TECH_FN_BY_FUNC := ["sheet05_tech_icons_04", "sheet05_tech_icons_06", "sheet05_tech_icons_05"]
const PARCH_TECH_NAME := {
	"Rouages de précision": "sheet15_tech_foundation_01",
	"Mécanisme d'horlogerie": "sheet15_tech_foundation_02",
	"Alliages des profondeurs": "sheet15_tech_foundation_03",
	"Gravure runique": "sheet15_tech_foundation_04",
	"Glyphes éthérés": "sheet15_tech_foundation_05",
	"Communion éthérée": "sheet15_tech_foundation_06",
	"Droit coutumier": "sheet15_tech_foundation_07",
	"Langue franque": "sheet15_tech_foundation_08",
	"Vergers étagés": "sheet15_tech_foundation_09",
	"Pâturages intégrés": "sheet15_tech_foundation_10",
	"Rites guerriers": "sheet15_tech_foundation_11",
	"Hordes conquérantes": "sheet15_tech_foundation_12",
	# l'ESSOR (planche 16) — matchs sûrs nom moteur ↔ sujet gravé
	"Poudrière": "sheet16_tech_rise_01",
	"Armurerie": "sheet16_tech_rise_02",
	"Fonderie": "sheet16_tech_rise_04",
	"Comptoirs marchands": "sheet16_tech_rise_06",
	"Commerce": "sheet16_tech_rise_07",
	"Manufacture": "sheet16_tech_rise_08",
	"Scriptorium": "sheet16_tech_rise_09",
	"Université": "sheet16_tech_rise_10",
	"Fortifications": "sheet16_tech_rise_12",
	"Atelier de construction": "sheet16_tech_rise_14",
	"Abondance": "sheet16_tech_rise_16",
	# les SIGNATURES & alentours (planche 17)
	"Forge à runes": "sheet17_tech_signatures_01",
	"Communion": "sheet17_tech_signatures_02",
	"Automates": "sheet17_tech_signatures_03",
	"Droit d'intégration": "sheet17_tech_signatures_04",
	"Irrigation & greniers": "sheet17_tech_signatures_05",
	"Conscription": "sheet17_tech_signatures_06",
	"Alchimie": "sheet17_tech_signatures_07",
	"Scrying": "sheet17_tech_signatures_08",
	"Savoir de guerre": "sheet17_tech_signatures_09",
	"Organisation militaire": "sheet17_tech_signatures_10",
	"Cadastre": "sheet17_tech_signatures_11",
	"Halles & entrepôts": "sheet17_tech_signatures_15",
	"Collecte de nourriture": "sheet17_tech_signatures_16",
	# COMBOS tier-4 (planche 18, anneaux d'or)
	"Arquebuserie de précision": "sheet18_tech_combos_apex_01",
	"Poliorcétique": "sheet18_tech_combos_apex_02",
	"Engins de siège": "sheet18_tech_combos_apex_03",
	"Chamanisme de guerre": "sheet18_tech_combos_apex_04",
	"Foederati": "sheet18_tech_combos_apex_05",
	"Économie de horde": "sheet18_tech_combos_apex_06",
	"Automates arcanes": "sheet18_tech_combos_apex_07",
	"Abondance druidique": "sheet18_tech_combos_apex_08",
	"Guildes maîtresses": "sheet18_tech_combos_apex_09",
	"Charrues lourdes": "sheet18_tech_combos_apex_10",
	"Machines agricoles": "sheet18_tech_combos_apex_11",
	"Horlogerie marchande": "sheet18_tech_combos_apex_12",
	"Académie cosmopolite": "sheet18_tech_combos_apex_13",
	"Grenier colonial": "sheet18_tech_combos_apex_14",
	# APEX tier-5
	"Arquebuse runique": "sheet18_tech_combos_apex_15",
	"Concile des savants": "sheet18_tech_combos_apex_16",
	"Légion universelle": "sheet26_tech_complement_apex_reserves_06",
	# COMPLÉMENT série 3 (planches 25-26) — plus AUCUN médaillon générique nommable
	"Bibliothèque": "sheet25_tech_complement_knowledge_arcane_01",
	"Académie": "sheet25_tech_complement_knowledge_arcane_02",
	"Magie de bataille": "sheet25_tech_complement_knowledge_arcane_03",
	"Invocation": "sheet25_tech_complement_knowledge_arcane_04",
	"L'Éveil": "sheet25_tech_complement_knowledge_arcane_05",
	"Gardes runiques": "sheet25_tech_complement_knowledge_arcane_06",
	"Savoir interdit": "sheet25_tech_complement_knowledge_arcane_07",
	"Collecte de bois": "sheet25_tech_complement_knowledge_arcane_08",
	"Collecte d'argile": "sheet25_tech_complement_knowledge_arcane_09",
	"Outillage": "sheet25_tech_complement_knowledge_arcane_10",
	"Industrie de masse": "sheet25_tech_complement_knowledge_arcane_11",
	"Foreuse arcanique": "sheet25_tech_complement_knowledge_arcane_12",
	"L'Œuvre noire": "sheet25_tech_complement_knowledge_arcane_13",
	"Qualité des matériaux": "sheet25_tech_complement_knowledge_arcane_14",
	"Caserne": "sheet25_tech_complement_knowledge_arcane_15",
	"Économie servile": "sheet25_tech_complement_knowledge_arcane_16",
	"Caste martiale": "sheet26_tech_complement_apex_reserves_01",
	"Chancellerie": "sheet26_tech_complement_apex_reserves_02",
	"Foi": "sheet26_tech_complement_apex_reserves_03",
	"Culte impérial": "sheet26_tech_complement_apex_reserves_04",
	"Transmutation": "sheet26_tech_complement_apex_reserves_05",
}
static func tech_medallion(nom: String, faustian: bool, tier: int, quarter: int) -> Texture2D:
	if PARCH_TECH_NAME.has(nom):
		return _tex(PARCH + PARCH_TECH_NAME[nom] + ".png")
	if tier >= 5:
		return _tex(PARCH + "sheet05_tech_icons_10.png")
	if tier >= 4:
		return _tex(PARCH + "sheet05_tech_icons_09.png")
	if faustian:
		return _tex(PARCH + "sheet05_tech_icons_07.png")
	var fn := quarter % 3
	if fn >= 0 and fn < PARCH_TECH_FN_BY_FUNC.size():
		return _tex(PARCH + PARCH_TECH_FN_BY_FUNC[fn] + ".png")
	return null

## pièce parch : texture + BBOX opaque (les cellules 256² ont de larges marges vides —
## on ne dessine que la matière). Cache {tex, rect}.
## Un ASSET existe-t-il ? — en RESSOURCE importée (lisible dans le PCK du .exe) OU en fichier
## réel sur disque (dev). FileAccess.file_exists SEUL est FAUX en export (le .png source n'est
## PAS dans le PCK, seule sa ressource importée l'est) — la cause des planches invisibles.
static func has(path: String) -> bool:
	return ResourceLoader.exists(path) or FileAccess.file_exists(path)

## EXPORT-SAFE : charge un PNG comme IMAGE exploitable. En EXPORT, res:// vit dans le PCK →
## Image.load_from_file ÉCHOUE (elle lit le disque réel) ; on charge la RESSOURCE importée
## (load) puis get_image() — les planches sont importées lossless RGBA8, donc get_used_rect /
## convert / resize marchent. Fallback DEV (PNG présent, .import pas encore généré) :
## Image.load_from_file. NULL si l'asset est absent.
static func load_img(path: String) -> Image:
	if ResourceLoader.exists(path):
		var t = load(path)
		if t is Texture2D:
			var im: Image = t.get_image()
			if im != null:
				return im
	if has(path):
		return Image.load_from_file(path)
	return null

static var _parch_cache := {}
static func parch_piece(piece: String) -> Dictionary:
	if _parch_cache.has(piece):
		return _parch_cache[piece]
	var out := {}
	var path := PARCH + piece + ".png"
	if has(path):
		var img := load_img(path)
		if img != null:
			var used := img.get_used_rect()
			img.generate_mipmaps()
			out = {"tex": ImageTexture.create_from_image(img), "rect": used}
	_parch_cache[piece] = out
	return out

## STYLEBOX 9-slice sur la BANDE opaque d'une pièce (le CORPS d'un bouton, hors fleuron
## décoratif) : on mesure la largeur opaque de chaque ligne, la bande = les lignes à
## ≥85 % du max (le crest étroit au-dessus est exclu) → le 9-slice ne s'écrase plus.
static var _parch_band := {}
static func parch_band_box(piece: String, tm: int, cmh: float = -1.0, cmv: float = -1.0,
		scale: float = 1.0) -> StyleBox:
	var key := "%s@%d@%.0f@%.0f@%.2f" % [piece, tm, cmh, cmv, scale]
	if _parch_band.has(key):
		return _parch_band[key]
	var sb: StyleBox = null
	var path := PARCH + piece + ".png"
	if has(path):
		var img := load_img(path)
		if img != null:
			if img.get_format() != Image.FORMAT_RGBA8:
				img.convert(Image.FORMAT_RGBA8)
			if scale != 1.0:
				# les marges 9-slice sont en px SOURCE : une bordure de 14 px écraserait un
				# bouton de 38 px — on RÉDUIT la source pour ramener la bordure à l'échelle UI
				img.resize(int(img.get_width() * scale), int(img.get_height() * scale),
					Image.INTERPOLATE_LANCZOS)
			var w := img.get_width()
			var h := img.get_height()
			var data := img.get_data()
			var x0s := PackedInt32Array(); x0s.resize(h)
			var x1s := PackedInt32Array(); x1s.resize(h)
			var wid := PackedInt32Array(); wid.resize(h)
			var maxw := 0
			for y in range(h):
				var a0 := -1
				var a1 := -1
				var row := y * w * 4
				for x in range(w):
					if data[row + x * 4 + 3] > 24:
						if a0 < 0:
							a0 = x
						a1 = x
				x0s[y] = a0; x1s[y] = a1
				wid[y] = (a1 - a0 + 1) if a0 >= 0 else 0
				maxw = maxi(maxw, wid[y])
			if maxw > 8:
				var thr := int(maxw * 0.85)
				# la PLUS LONGUE plage CONTIGUË ≥ seuil : le corps du bouton — une 2e ligne
				# pleine largeur ailleurs (ombre sous la pièce) ne s'invite plus dans la région
				var y0 := -1
				var y1 := -1
				var run0 := -1
				for y in range(h + 1):
					var on: bool = (y < h) and wid[y] >= thr
					if on and run0 < 0:
						run0 = y
					elif not on and run0 >= 0:
						if y0 < 0 or (y - run0) > (y1 - y0 + 1):
							y0 = run0
							y1 = y - 1
						run0 = -1
				var rx0 := w
				var rx1 := 0
				if y0 >= 0:
					for y in range(y0, y1 + 1):
						if wid[y] > 0:
							rx0 = mini(rx0, x0s[y])
							rx1 = maxi(rx1, x1s[y])
				if y0 >= 0 and y1 > y0:
					img.generate_mipmaps()
					var st := StyleBoxTexture.new()
					st.texture = ImageTexture.create_from_image(img)
					st.region_rect = Rect2(rx0, y0, rx1 - rx0 + 1, y1 - y0 + 1)
					st.texture_margin_left = tm
					st.texture_margin_right = tm
					st.texture_margin_top = tm
					st.texture_margin_bottom = tm
					if cmh >= 0.0:
						st.content_margin_left = cmh
						st.content_margin_right = cmh
					if cmv >= 0.0:
						st.content_margin_top = cmv
						st.content_margin_bottom = cmv
					sb = st
	_parch_band[key] = sb
	return sb

## STYLEBOX 9-slice depuis une pièce parch (bbox → region_rect, marges de texture tm,
## marges de contenu cmh/cmv). null si la pièce manque → l'appelant garde son repli.
static var _parch_sb := {}
static func parch_box(piece: String, tm: int, cmh: float = -1.0, cmv: float = -1.0) -> StyleBox:
	var key := "%s@%d@%.0f@%.0f" % [piece, tm, cmh, cmv]
	if _parch_sb.has(key):
		return _parch_sb[key]
	var sb: StyleBox = null
	var p := parch_piece(piece)
	if not p.is_empty():
		var st := StyleBoxTexture.new()
		st.texture = p["tex"]
		st.region_rect = p["rect"]
		st.texture_margin_left = tm
		st.texture_margin_right = tm
		st.texture_margin_top = tm
		st.texture_margin_bottom = tm
		if cmh >= 0.0:
			st.content_margin_left = cmh
			st.content_margin_right = cmh
		if cmv >= 0.0:
			st.content_margin_top = cmv
			st.content_margin_bottom = cmv
		sb = st
	_parch_sb[key] = sb
	return sb

const SETTLE_CELL := 96   # atlas settlements : 6 tiers (col) × 6 groupes (ligne), 96 px

## ÔTE LE MAGENTA d'un atlas BMP. Pas un simple seuil (le seuil laissait des FRANGES
## roses sur les bords anti-aliasés) : on mesure la « magenta-ité » m = min(R,B) − G
## (à quel point R et B dominent le vert). m franc ⇒ transparent ; frange ⇒ alpha
## dégressif + DESPILL (on rabat R et B vers G pour tuer le rose résiduel). Une seule
## passe, partagée par tous les atlas (settlements, dressing, …).
static func _key_magenta(img: Image) -> Texture2D:
	if img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	var data := img.get_data()
	for i in range(0, data.size(), 4):
		var r := data[i]
		var g := data[i + 1]
		var b := data[i + 2]
		var lo := mini(r, b)
		var m := lo - g                       # > 0 ⇒ le pixel penche vers le magenta
		if m > 100:
			data[i + 3] = 0                   # magenta franc → transparent
		elif m > 20:                          # frange : alpha dégressif + despill du rose
			data[i + 3] = int(data[i + 3] * float(120 - m) / 100.0)
			data[i]     = g + (r - lo)        # rebascule R/B sur G (ôte l'excès magenta,
			data[i + 2] = g + (b - lo)        # garde la teinte propre du sprite)
	var keyed := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, data)
	return ImageTexture.create_from_image(keyed)

static var _settle_tex: Texture2D = null
static var _settle_tried := false

## charge l'atlas SETTLEMENTS (magenta ôté → alpha), une fois (caché).
static func _settlements() -> Texture2D:
	if _settle_tried:
		return _settle_tex
	_settle_tried = true
	# La PLACE est gardée : on charge en PRIORITÉ un PNG à alpha (settlements.png →
	# aucun keying) ; à défaut, repli LEGACY sur un BMP magenta-keyé. Tant qu'aucun
	# des deux n'est présent → null → l'overlay retombe sur ses marqueurs (pas de bave).
	var img: Image = null
	if has(MAP +"settlements.png"):
		img = load_img(MAP +"settlements.png")
		if img != null:
			_settle_tex = ImageTexture.create_from_image(img)
	elif has(MAP +"settlements.bmp"):
		img = load_img(MAP +"settlements.bmp")
		if img != null:
			_settle_tex = _key_magenta(img)
	return _settle_tex

const DRESS_CELL := 32   # atlas dressing : 16 colonnes, 32 px, magenta-keyé
static var _dress_tex: Texture2D = null
static var _dress_tried := false

static func _dressing() -> Texture2D:
	if _dress_tried:
		return _dress_tex
	_dress_tried = true
	# Même règle : PNG à alpha d'abord (dressing.png), BMP magenta-keyé en repli legacy.
	var img: Image = null
	if has(MAP +"dressing.png"):
		img = load_img(MAP +"dressing.png")
		if img != null:
			_dress_tex = ImageTexture.create_from_image(img)
	elif has(MAP +"dressing.bmp"):
		img = load_img(MAP +"dressing.bmp")
		if img != null:
			_dress_tex = _key_magenta(img)
	return _dress_tex

## sprite de dressing par id MAPD (0-111). null si l'atlas est absent.
static func dressing_sprite(id: int) -> Texture2D:
	var t := _dressing()
	if t == null or id < 0:
		return null
	var at := AtlasTexture.new()
	at.atlas = t
	at.region = Rect2((id % 16) * DRESS_CELL, (id / 16) * DRESS_CELL, DRESS_CELL, DRESS_CELL)
	return at

## sprite de settlement : colonne = tier (0-5), ligne = groupe (0-5). null si absent.
static func settlement_sprite(tier: int, group: int) -> Texture2D:
	var t := _settlements()
	if t == null:
		return null
	tier = clampi(tier, 0, 5)
	group = clampi(group, 0, 5)
	var at := AtlasTexture.new()
	at.atlas = t
	at.region = Rect2(tier * SETTLE_CELL, group * SETTLE_CELL, SETTLE_CELL, SETTLE_CELL)
	return at

static var _cache := {}

# ── TUILES de CONSTRUCTION (unités · édifices) : des PNG auto-encadrés (fond sombre
#    arrondi + liseré doré + art). On ôte seulement le NOIR des coins → alpha, la
#    tuile survit. Mappage indice d'enum moteur → nom de fichier (certaines clés
#    d'asset diffèrent de l'identifiant C : U_FOUDRIER←U_ARQUEBUSIER, etc.). ────────
const UNITS_DIR := "res://assets/scps/pack/units/"
const BUILDINGS_DIR := "res://assets/scps/pack/buildings/"

# indexé par UnitType (0-21), ordre de scps_army.h
const UNIT_FILE := [
	"U_PIQUIER", "U_LANCIER", "U_EPEISTE", "U_ARCHER", "U_ARBALETRIER",
	"U_CAVALERIE_LEGERE", "U_CAVALERIE_LOURDE", "U_SORCIER", "U_HALLEBARDIER", "U_FOUDRIER",
	"U_ALCHIMISTE", "U_CHAMAN", "U_ARBALETRIER_LOURD", "U_BERSERKER", "U_LANCIER_DE_CHOC",
	"U_MILICE", "U_HARCELEUR", "U_TRAQUEUR", "U_LAME_FRANCHE", "U_GARDE_ESCORTE",
	"U_CAVALERIE_CUIRASSEE", "U_CAVALERIE_DE_RAID",
]
# indexé par Edifice (0-25), ordre de scps_agency.h (noms = ceux des fichiers)
const BLD_FILE := [
	"EDI_TRIBUNAL", "EDI_CHANCELLERIE", "EDI_ACADEMIE", "EDI_GARNISON", "EDI_FORTERESSE", "EDI_CITADELLE",
	"EDI_PORT", "EDI_CARAVANSERAIL", "EDI_MARCHE", "EDI_ENTREPOT", "EDI_GRENIER", "EDI_IRRIGATION", "EDI_AQUEDUC",
	"EDI_SANCTUAIRE", "EDI_TEMPLE", "EDI_CATHEDRALE", "EDI_BIBLIOTHEQUE", "EDI_MONASTERE", "EDI_COMPTOIR", "EDI_BANQUE",
	"EDI_ARSENAL", "EDI_AMIRAUTE", "EDI_PORT_MARCHAND", "EDI_BIBLIO_MIL", "EDI_OBSERVATOIRE", "EDI_TRADE_CENTER",
]
static var _tile_cache := {}

## ôte le NOIR du fond (coins de la tuile, somme RGB < 24) → alpha ; fond/liseré/art survivent.
static func _key_black(img: Image) -> Texture2D:
	if img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	var d := img.get_data()
	for i in range(0, d.size(), 4):
		if int(d[i]) + int(d[i + 1]) + int(d[i + 2]) < 24:
			d[i + 3] = 0
	var keyed := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, d)
	return ImageTexture.create_from_image(keyed)

static func _tile(dir: String, key: String) -> Texture2D:
	var path := dir + key + ".png"
	if _tile_cache.has(path):
		return _tile_cache[path]
	var tex: Texture2D = null
	if has(path):
		var img := load_img(path)
		if img != null:
			tex = _key_black(img)
	_tile_cache[path] = tex
	return tex

## tuile d'UNITÉ par UnitType (0-21) ; parchemin (planches 9-10) d'abord, sinon
## l'ancienne tuile ; null si tout manque (l'appelant retombe sur le texte).
static func unit_sprite(unit_type: int) -> Texture2D:
	if unit_type < 0 or unit_type >= UNIT_FILE.size():
		return null
	if unit_type < PARCH_UNIT.size():
		var t := _tex(PARCH + PARCH_UNIT[unit_type] + ".png")
		if t != null:
			return t
	return _tile(UNITS_DIR, UNIT_FILE[unit_type])

## tuile d'ÉDIFICE par Edifice (0-25) ; parchemin (planches 6-7) d'abord, sinon
## l'ancienne tuile ; null si tout manque.
static func building_sprite(edi_type: int) -> Texture2D:
	if edi_type < 0 or edi_type >= BLD_FILE.size():
		return null
	if edi_type < PARCH_BLD.size():
		var t := _tex(PARCH + PARCH_BLD[edi_type] + ".png")
		if t != null:
			return t
	return _tile(BUILDINGS_DIR, BLD_FILE[edi_type])

# ── ASSETS MONDE à ALPHA (cités par bande de pop · dressing nature) — RGBA DIRECT
#    (vrai alpha, AUCUN keying) : on réutilise `_tex` (chargement simple + cache). ──
const CITIES_DIR := "res://assets/scps/pack/cities/"
const DRESSING_DIR := "res://assets/scps/pack/dressing/"

## sprite de CITÉ par bande de pop (1-8) × variante (0-7 → A-H) ; null si absente.
const CITY_LIFT := 2.1   ## relèvement de luminance des sprites de ville/centre/structure (sources ~lum 50)
static func city_sprite(band: int, variant: int) -> Texture2D:
	var v := "ABCDEFGH"
	return _tex_lift(CITIES_DIR + "CITY_POP_BAND_%02d_%s.png" % [clampi(band, 1, 8), v[clampi(variant, 0, 7)]], CITY_LIFT)

## variante de ville TERRAIN par NOM (CITY_BIOME_*) ; null si absente.
static func city_biome(nm: String) -> Texture2D:
	return _tex_lift(CITIES_DIR + nm + ".png", CITY_LIFT)

## CENTRE de ville par TIER (1-7) : nouveau lot power-progression (hutte→palais royal), AGNOSTIQUE au
## biome (un seul jeu de 7). `kind` conservé pour compat d'appel mais IGNORÉ. RGBA direct, lift DOUX
## (l'art painterly est déjà clair, lum ~85 → le lift ×2.1 le délaverait).
const CENTRES_DIR := "res://assets/scps/pack/centres/"
const NEW_ART_LIFT := 1.25   ## éclaircissement doux des nouveaux sprites (centres/bâtiments)
static func city_centre(kind: String, tier: int) -> Texture2D:
	return _tex_lift(CENTRES_DIR + "CITY_CENTRE_T%d.png" % clampi(tier, 1, 7), NEW_ART_LIFT)

## sprite de DRESSING par NOM (DRESS_TREE_*, DRESS_GROVE_*…) ; null si absent.
static func dressing_named(nm: String) -> Texture2D:
	return _tex(DRESSING_DIR + nm + ".png")

## JETON d'armée de campagne par NOM (ARMY_TOKEN_*) ; null si absent. RGBA direct.
const CAMPAIGN_DIR := "res://assets/scps/pack/campaign/"
static func army_token(nm: String) -> Texture2D:
	return _tex(CAMPAIGN_DIR + nm + ".png")

## sprite de RIVIÈRE droit (RIVER_HORIZONTAL, coule en X → tourné le long du fil par
## l'angle de la façade) ; null si absent.
const RIVERS_DIR := "res://assets/scps/pack/rivers/"
## segment de rivière DROIT (tourné le long du fil) — l'eau traverse le losange W↔E, centrée.
static func river_sprite() -> Texture2D:
	return _tex(RIVERS_DIR + "RIVER_EW.png")
## TUILE ISO directionnelle de rivière (pré-orientée) par nom — sélectionnée selon l'axe du flux.
static func river_named(nm: String) -> Texture2D:
	return _tex(RIVERS_DIR + nm + ".png")
## ÉLARGISSEMENT (delta) posé aux EMBOUCHURES — là où le fil rejoint la mer.
static func river_mouth_sprite() -> Texture2D:
	return _tex(RIVERS_DIR + "RIVER_WIDENING.png")

# ── SOL ISO : VARIANTES de texture par biome (256×128, eau comprise) chargées des sources.
#    Le renderer peint la variante (par cellule → variété) et le shader fait le bord. Index = enum Biome.
const ISO_TILES_DIR := "res://assets/scps/pack/iso_tiles/"
const ISO_TILE_W := 256
const ISO_TILE_H := 128
const BIOME_TEX_DIR := "res://assets/scps/pack/iso_tiles/flat/"   # tuiles PLATES tuilables, lum. NORMALISÉE
## plusieurs textures par biome → variété (anti-répétition) + bons mappings (aucun tile noir) +
## EAU tuilée (deep/ocean/shallow) pour un rivage propre DEPUIS LA TERRE (la terre déborde sur l'eau).
const BIOME_TEX := {
	0: ["wtr"], 1: ["wtr"], 2: ["wtr"], 3: ["snd", "bch"],   # eau UNIFIÉE (1 texture → mer continue)
	4: ["grs", "gr2", "gr3"], 5: ["fc1", "fc2", "fc3"], 6: ["gr3", "grs", "gr6"], 7: ["gr5", "gr4"],
	8: ["gr6", "gr5"], 9: ["ds2", "ds3"], 10: ["des", "ds2", "ds4"], 11: ["ds3", "snd"],
	12: ["for", "fo2"], 13: ["fo2", "for"], 14: ["gr2", "for"], 15: ["rm1", "rm2"],
	16: ["rc1", "rc2"], 17: ["gr2", "gr3"], 18: ["rc1", "rc2", "rc3"], 19: ["sno", "snf"],
	20: ["ice", "sno"], 21: ["rm1", "for"], 22: ["rm2", "rm1"], 23: ["rc3", "rc1"], 24: ["for", "fo2"],
}

static func _src_tex(name: String) -> Texture2D:
	return _tex(BIOME_TEX_DIR + name + ".png")

## texture du biome, variante `idx` (variété par cellule) → Texture2D 256×128 ; null si biome inconnu.
static func biome_variant(biome: int, idx: int) -> Texture2D:
	var lst: Array = BIOME_TEX.get(biome, [])
	if lst.is_empty():
		return null
	return _src_tex(String(lst[abs(idx) % lst.size()]))

static func biome_tile(biome: int) -> Texture2D:
	return biome_variant(biome, 0)

## VRAI si les textures de biome sont présentes → sol en tuiles ; sinon repli procédural.
static func has_iso_tiles() -> bool:
	return biome_variant(4, 0) != null

## stamp de bruit fbm SEAMLESS (blend_noise.png) — ondule le bord du fondu (shader iso_blend).
static func blend_noise() -> Texture2D:
	return _tex(ISO_TILES_DIR + "blend_noise.png")

# ── (SECONDAIRE) palette super_biomes — grandes planches de VARIATION, pioche par cellule. ───────
const SUPER_GRID := 10                       ## super_biomes_01 = PALETTE de 100 tuiles découpées
## cellules EAU de la palette (à exclure du tirage TERRE) — repérées sur la planche (r*10+c).
const SUPER_WATER := [0, 8, 9, 10, 11, 12, 19, 20, 21, 22, 30, 40, 49, 50, 60, 70, 80, 90]
static var _iso_region_cache := {}
static var _land_pal: Array = []             ## indices de tuiles TERRE (palette de variation)

## l'atlas super_biomes (Texture2D N·256 × N·128) ; null si absent. Caché par chemin (un seul essai).
static func super_atlas() -> Texture2D:
	return _tex(ISO_TILES_DIR + "super_biomes_01.png")

## tuile (col,row) du champ → région 256×128 (AtlasTexture caché) ; null si l'atlas manque.
static func super_tile(col: int, row: int) -> Texture2D:
	var atl := super_atlas()
	if atl == null:
		return null
	var c := col % SUPER_GRID
	var r := row % SUPER_GRID
	var ck := r * SUPER_GRID + c
	if _iso_region_cache.has(ck):
		return _iso_region_cache[ck]
	var at := AtlasTexture.new()
	at.atlas = atl
	at.region = Rect2(c * ISO_TILE_W, r * ISO_TILE_H, ISO_TILE_W, ISO_TILE_H)
	_iso_region_cache[ck] = at
	return at

## palette des tuiles de TERRE (toutes sauf l'eau) — construite une fois.
static func _land_palette() -> Array:
	if _land_pal.is_empty():
		for i in range(SUPER_GRID * SUPER_GRID):
			if not SUPER_WATER.has(i):
				_land_pal.append(i)
	return _land_pal

## tuile de TERRE pour la cellule monde (tc,tr) : on PIOCHE dans la palette (variation, pas la
## disposition de la planche-exemple) ; le blend derrière fond les tuiles voisines.
static func super_land(tc: int, tr: int) -> Texture2D:
	var pal := _land_palette()
	var idx := absi((tc * 73856093) ^ (tr * 19349663)) % pal.size()
	var t: int = pal[idx]
	return super_tile(t % SUPER_GRID, t / SUPER_GRID)

# ── STRUCTURES de terrain (maisons · ateliers · champs · édifices civiques) : un
#    POOL parsemé autour des villes. On énumère le dossier au 1er appel (pas de
#    const de 96 noms) ; RGBA direct via _tex. ──────────────────────────────────
const STRUCTURES_DIR := "res://assets/scps/pack/structures/"
static var _struct_names: PackedStringArray = []
static var _struct_listed := false

static func structure_names() -> PackedStringArray:
	if _struct_listed:
		return _struct_names
	_struct_listed = true
	var d := DirAccess.open(STRUCTURES_DIR)
	if d != null:
		for f in d.get_files():
			if f.ends_with(".png"):
				_struct_names.append(f.get_basename())
	return _struct_names

static func structure_sprite(nm: String) -> Texture2D:
	return _tex_lift(STRUCTURES_DIR + nm + ".png", NEW_ART_LIFT)

# ── CLUTTER (props de vie : barils, bûches, charrettes, puits…) — découpés de la planche, RGBA direct.
const CLUTTER_DIR := "res://assets/scps/pack/clutter/"
static var _clutter_names: PackedStringArray = []
static var _clutter_listed := false
static func clutter_names() -> PackedStringArray:
	if _clutter_listed:
		return _clutter_names
	_clutter_listed = true
	var d := DirAccess.open(CLUTTER_DIR)
	if d != null:
		for f in d.get_files():
			if f.ends_with(".png"):
				_clutter_names.append(f.get_basename())
	return _clutter_names
static func clutter_sprite(nm: String) -> Texture2D:
	return _tex_lift(CLUTTER_DIR + nm + ".png", NEW_ART_LIFT)


# ressources couvertes par le pack UI (repli tant que le sprite dédié n'est pas posé)
const RES_FALLBACK := {
	"grain": "grain_bundle", "ble": "grain_bundle", "betail": "grain_bundle",
	"cereales": "grain_bundle",
	"poisson": "health_food_bowl", "nourriture": "health_food_bowl", "vivres": "health_food_bowl",
	"pierre": "materials_stone", "argile": "materials_stone",
	"bois": "layer_forest",
	"or": "gold_coin", "metal_precieux": "gold_coin", "perle": "gold_coin",
	"outils": "development_tools", "metal": "development_tools",
	"fer": "development_tools", "cuivre": "development_tools", "etain": "development_tools",
	"charbon": "materials_stone", "sel": "materials_stone", "salpetre": "materials_stone",
	"vin": "health_food_bowl", "epices": "health_food_bowl", "laine": "layer_forest",
}

## clé de fichier normalisée d'un nom de ressource : minuscules, accents ôtés,
## espaces→_ . Ex. « Fer »→"fer", « Cristal arcanique »→"cristal_arcanique".
static func resource_key(res_name: String) -> String:
	var s := res_name.to_lower()
	var acc := {"é":"e","è":"e","ê":"e","ë":"e","à":"a","â":"a","ä":"a","î":"i","ï":"i",
		"ô":"o","ö":"o","û":"u","ù":"u","ü":"u","ç":"c","'":"","’":"","-":" "}
	for k in acc:
		s = s.replace(k, acc[k])
	return s.strip_edges().replace(" ", "_")

## le SPRITE d'une ressource (assets/scps/pack/resources/). On essaie, dans l'ordre :
## par INDEX d'enum (<id>.png puis <id zero-paddé 3>.png — l'ordre du jeu de sprites
## fourni), par CLÉ de nom (<clé>.png), puis repli sur une icône du pack, sinon null
## (l'appelant retombe sur le texte). Le NOM va au survol.
const RES_ATLAS_COLS := 16   # feuille « sheet.png » : grille de 16 colonnes (ordre enum)

static func resource_sprite(res_id: int, res_name: String) -> Texture2D:
	var pkey := resource_key(res_name)
	if PARCH_RES.has(pkey):
		var pt := _tex(PARCH + PARCH_RES[pkey] + ".png")         # chip PARCHEMIN (planche 19)
		if pt != null: return pt
	if res_id >= 0:
		var t := _tex(RESOURCES + str(res_id) + ".png")          # fichier par indice
		if t != null: return t
		t = _tex(RESOURCES + "%03d.png" % res_id)                # variante zéro-paddée
		if t != null: return t
		var sheet := _tex(RESOURCES + "sheet.png")               # OU une feuille 16-col
		if sheet != null:
			var cell := int(sheet.get_width() / RES_ATLAS_COLS)
			var at := AtlasTexture.new()
			at.atlas = sheet
			at.region = Rect2((res_id % RES_ATLAS_COLS) * cell, (res_id / RES_ATLAS_COLS) * cell, cell, cell)
			return at
	return resource_icon(res_name)

## variante par NOM seul (income/province, où l'on n'a pas l'id).
static func resource_icon(res_name: String) -> Texture2D:
	var key := resource_key(res_name)
	if PARCH_RES.has(key):
		var pt := _tex(PARCH + PARCH_RES[key] + ".png")          # chip PARCHEMIN (planche 19)
		if pt != null:
			return pt
	var t := _tex(RESOURCES + key + ".png")
	if t != null:
		return t
	if RES_FALLBACK.has(key):
		return icon(RES_FALLBACK[key])
	return null

static func _tex(path: String) -> Texture2D:
	if _cache.has(path):
		return _cache[path]
	var tex: Texture2D = null
	if has(path):        # garde : pas d'erreur console si l'asset manque
		var img := load_img(path)
		if img != null:
			img.generate_mipmaps()          # anti-aliasing au DÉZOOM (sol échantillonné en monde continu)
			tex = ImageTexture.create_from_image(img)
	_cache[path] = tex            # met aussi en cache les ratés (null) → un seul essai
	return tex

static func icon(name: String) -> Texture2D:
	if PARCH_ICON.has(name):
		var t := _tex(PARCH + PARCH_ICON[name] + ".png")
		if t != null:
			return t
	return _tex(ICONS + name + ".png")

static func chrome(name: String) -> Texture2D:
	return _tex(CHROME + name + ".png")

## charge un PNG en RELEVANT sa luminance (×f sur les pixels opaques) — pour les sprites SOMBRES
## (villes/centres/structures ~lum 50) que le modulate canvas ne peut PAS éclaircir (clamp à 1.0).
## Fait au CHARGEMENT (CPU, hors clamp) puis caché → coût une seule fois par sprite.
static var _lift_cache := {}
static func _tex_lift(path: String, f: float) -> Texture2D:
	var key := path + "@" + str(f)
	if _lift_cache.has(key):
		return _lift_cache[key]
	var tex: Texture2D = null
	if has(path):
		var img := load_img(path)
		if img != null:
			if img.get_format() != Image.FORMAT_RGBA8:
				img.convert(Image.FORMAT_RGBA8)
			var d := img.get_data()
			for i in range(0, d.size(), 4):
				if d[i + 3] > 0:
					d[i] = mini(255, int(d[i] * f))
					d[i + 1] = mini(255, int(d[i + 1] * f))
					d[i + 2] = mini(255, int(d[i + 2] * f))
			var lifted := Image.create_from_data(img.get_width(), img.get_height(), false, Image.FORMAT_RGBA8, d)
			lifted.generate_mipmaps()    # sinon INVISIBLE sous un filtre mipmap (ex. falaises sur iso_ground)
			tex = ImageTexture.create_from_image(lifted)
	_lift_cache[key] = tex
	return tex

## dessine une icône (carrée) à `pos`, côté `sizepx`. No-op si absente.
static func draw_icon(ci: CanvasItem, name: String, pos: Vector2, sizepx: float, mod: Color = Color.WHITE) -> void:
	var t := icon(name)
	if t != null:
		ci.draw_texture_rect(t, Rect2(pos, Vector2(sizepx, sizepx)), false, mod)

## dessine une pièce de chrome étirée dans `rect`. No-op si absente.
static func draw_chrome(ci: CanvasItem, name: String, rect: Rect2, mod: Color = Color.WHITE) -> void:
	var t := chrome(name)
	if t != null:
		ci.draw_texture_rect(t, rect, false, mod)

## JAUGE TEXTURÉE : cadre vide + remplissage (région clippée à `value` 0-100).
## La couleur du remplissage suit le sens : vert (haut) · or (moyen) · rouge (bas).
## PARCHEMIN d'abord (planche 4 : gouttière 01 + remplissages olive 03 / or 02 /
## terre cuite 04, dessinés par BBOX — les cellules ont de larges marges vides).
static func bar(ci: CanvasItem, rect: Rect2, value: int) -> void:
	value = clampi(value, 0, 100)
	var trough := parch_piece("sheet04_gauges_bars_01")
	if not trough.is_empty():
		ci.draw_texture_rect_region(trough["tex"], rect, trough["rect"])
		var fname := "sheet04_gauges_bars_03" if value >= 60 else \
			("sheet04_gauges_bars_02" if value >= 35 else "sheet04_gauges_bars_04")
		var fp := parch_piece(fname)
		if not fp.is_empty() and value > 0:
			var fr: Rect2 = fp["rect"]
			var inset := Vector2(rect.size.y * 0.18, rect.size.y * 0.18)
			var area := Rect2(rect.position + inset, rect.size - inset * 2.0)
			var fw := area.size.x * value / 100.0
			ci.draw_texture_rect_region(fp["tex"],
				Rect2(area.position, Vector2(fw, area.size.y)),
				Rect2(fr.position, Vector2(fr.size.x * value / 100.0, fr.size.y)))
		return
	var empty := chrome("bar_empty_prosperity")
	if empty != null:
		ci.draw_texture_rect(empty, rect, false)
	else:
		ci.draw_rect(rect, Color(0.11, 0.09, 0.06), true)
		ci.draw_rect(rect, Color(0.78, 0.51, 0.24), false, 1.0)
	var kind := "bar_fill_green" if value >= 60 else ("bar_fill_gold" if value >= 35 else "bar_fill_red")
	var fill := chrome(kind)
	# repli en aplat si la texture manque
	var inset := Vector2(3, 3)
	var area := Rect2(rect.position + inset, rect.size - inset * 2.0)
	var fw := area.size.x * value / 100.0
	if fill != null:
		var ts := fill.get_size()
		ci.draw_texture_rect_region(fill,
			Rect2(area.position, Vector2(fw, area.size.y)),
			Rect2(0, 0, ts.x * value / 100.0, ts.y))
	elif fw > 0:
		var col := Color(0.42,0.66,0.36) if value >= 60 else (Color(0.79,0.62,0.29) if value >= 35 else Color(0.78,0.30,0.27))
		ci.draw_rect(Rect2(area.position, Vector2(fw, area.size.y)), col, true)
