extends RefCounted
## HoverZones — LE petit objet de survol partagé (revue 2026-07-21, #3 volet « hover »).
## Un panneau dessiné (_draw) enregistre ses zones ici ; le hit-test et le filtre
## d'en-tête vivent en UN endroit. Deux consommations aujourd'hui : le TooltipServer
## (sidebar_drawer, via to_tips) et le tooltip local (province_detail, via hit_text) —
## la FUSION vers le serveur est le pas suivant, pas celui-ci. Typé (règle #2).

var _zones: Array[Dictionary] = []

func clear() -> void:
	_zones.clear()

## `card` : la fiche riche (tooltip_factory.stock_info_card…) — {} si simple texte.
func add(rect: Rect2, text: String, card: Dictionary = {}) -> void:
	_zones.append({"rect": rect, "text": text, "card": card})

## compat migration : accepte le dict inline {"rect", "text"[, "card"]} tel quel —
## les ~30 sites d'append multi-lignes migrent sans réécriture (même contrat).
func add_dict(z: Dictionary) -> void:
	_zones.append({"rect": z.get("rect", Rect2()), "text": String(z.get("text", "")),
		"card": z.get("card", {})})

func is_empty() -> bool:
	return _zones.is_empty()

## le TEXTE sous le point ("" si rien) — le hit-test maison de province_detail.
func hit_text(point: Vector2) -> String:
	for z in _zones:
		if (z["rect"] as Rect2).has_point(point):
			return String(z["text"])
	return ""

## la ZONE complète sous le point ({} si rien) — pour qui veut la fiche.
func hit(point: Vector2) -> Dictionary:
	for z in _zones:
		if (z["rect"] as Rect2).has_point(point):
			return z
	return {}

## exporte vers le TooltipServer (motif sidebar_drawer) : [rect, text, card] par zone,
## en ÉCARTANT celles défilées sous l'en-tête fixe (min_y) — sinon un fantôme invisible
## répondrait au survol dans le bandeau de titre.
func to_tips(min_y: float) -> Array:
	var out: Array = []
	for z in _zones:
		if (z["rect"] as Rect2).get_center().y < min_y:
			continue
		out.append([z["rect"], z["text"], z.get("card", {})])
	return out
