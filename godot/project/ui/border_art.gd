extends NinePatchRect
## BorderArt — la BORDURE ENLUMINÉE plein écran (cadre illustré, centre transparent).
## Change de motif selon l'ÂGE courant (age_state) ; à une FIN (endgame_info.fin>0),
## bascule sur la bordure de l'apocalypse/ascension. DISPLAY-ONLY (la membrane, lecture
## seule de la façade ; aucune incidence moteur/déterminisme). mouse_filter IGNORE — le
## cadre laisse passer tous les clics vers l'UI et la carte dessous. Un asset absent ⇒
## on garde le cadre courant (jamais de flash vide) ; tous absents ⇒ no-op silencieux.

const DIR := "res://art/borders/"
const MARGIN := 256

## age_state["age"] : -1 = l'aube du règne, 0..6 = AgeId (Commerce → Ordre de Fer)
const AGE_KEYS := ["commerce", "raison", "empires", "breche", "lumieres", "soulevements", "ordrefer"]
## endgame_info["fin"] : 1 EAU · 2 FROID · 3 RONCES · 4 ASCENSION · 5 SANG
const FIN_KEYS := { 1: "fin_eau", 2: "fin_froid", 3: "fin_ronces", 4: "fin_ascension", 5: "fin_sang" }

var _cache := {}          ## clé → Texture2D (chargée à la demande)
var _cur := ""            ## clé affichée (évite de recharger chaque tick)

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	patch_margin_left = MARGIN
	patch_margin_top = MARGIN
	patch_margin_right = MARGIN
	patch_margin_bottom = MARGIN
	Sim.ticked.connect(func(_y): _refresh())
	Sim.generated.connect(_refresh)
	_apply("age_aube")    # l'aube du règne, avant tout âge levé

func _tex(key: String) -> Texture2D:
	if _cache.has(key):
		return _cache[key]
	var path := DIR + key + ".png"
	var t: Texture2D = load(path) if ResourceLoader.exists(path) else null
	_cache[key] = t
	return t

## `key` = nom de fichier sans extension (age_aube, age_commerce, …, fin_eau, …)
func _apply(key: String) -> void:
	if key == _cur:
		return
	var t := _tex(key)
	if t == null:
		return            # asset absent → garder le cadre courant (pas de flash vide)
	texture = t
	_cur = key

func _refresh() -> void:
	var w = Sim.world
	if w == null:
		return
	# une FIN prime l'âge : l'apocalypse (ou l'ascension) encadre le monde
	var e: Dictionary = w.endgame_info() if w.has_method("endgame_info") else {}
	var fin: int = int(e.get("fin", 0))
	if fin > 0 and FIN_KEYS.has(fin):
		_apply(FIN_KEYS[fin])
		return
	var key := "age_aube"
	if w.has_method("age_state"):
		var a: int = int(w.age_state().get("age", -1))
		if a >= 0 and a < AGE_KEYS.size():
			key = "age_" + AGE_KEYS[a]
	_apply(key)
