extends RefCounted
## InfoRef — vocabulaire commun des liens contextuels de l'interface.
##
## Une référence décrit un OBJET, jamais le panneau qui l'affiche. Les dictionnaires
## restent volontairement légers pour traverser les signaux GDScript sans coupler les
## contrôles entre eux.

const COUNTRY := "country"
const PROVINCE := "province"
const REGION := "region"
const CORPS := "corps"
const RESOURCE := "resource"
const TECH := "tech"
const SIDEBAR_TAB := "sidebar_tab"
const MAP_MODE := "map_mode"
const CODEX := "codex"
const MEMORY := "memory"

const KINDS := [COUNTRY, PROVINCE, REGION, CORPS, RESOURCE, TECH, SIDEBAR_TAB, MAP_MODE, CODEX, MEMORY]

static func make(kind: String, id = -1, label: String = "", data: Dictionary = {}) -> Dictionary:
	var ref := {"kind": kind, "id": id}
	if label != "":
		ref["label"] = label
	if not data.is_empty():
		ref["data"] = data.duplicate(true)
	return ref

static func is_valid(ref: Dictionary) -> bool:
	if not KINDS.has(String(ref.get("kind", ""))):
		return false
	return ref.has("id") and typeof(ref["id"]) in [TYPE_INT, TYPE_STRING]

static func key(ref: Dictionary) -> String:
	if not is_valid(ref):
		return ""
	return "%s:%s" % [String(ref["kind"]), str(ref["id"])]

## Une requête ajoute seulement une préférence de surface et un contexte de lecture.
## `surface` reste un indice ("detail", "map", "sidebar"), Main décide du contrôle réel.
static func request(ref: Dictionary, surface: String = "", context: Dictionary = {}) -> Dictionary:
	var out := {"ref": ref.duplicate(true)}
	if surface != "":
		out["surface"] = surface
	if not context.is_empty():
		out["context"] = context.duplicate(true)
	return out

static func request_key(req: Dictionary) -> String:
	var ref = req.get("ref", {})
	if not (ref is Dictionary):
		return ""
	var base := key(ref)
	if base == "":
		return ""
	# Le contexte fait partie de la vue restaurable. `var_to_str` est déterministe pour
	# nos petits dictionnaires construits dans le code et évite un parseur parallèle.
	return "%s|%s|%s" % [base, String(req.get("surface", "")), var_to_str(req.get("context", {}))]
