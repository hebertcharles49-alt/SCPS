extends RefCounted
## event_art.gd — MAPPING illustration d'évènement (réutilisation PAR THÈME).
##
## 8 bannières 4:1 (512×128, res://art/events/) couvrent des FAMILLES d'évènements,
## pas un évènement chacune : plusieurs EVID pointent le même thème (troubles civils,
## famine, cour, commerce, entropie, schisme, machine, conséquences). Display-only.
##
## Clé = EVID moteur (l'enum EvId de scps_events.h, exposé par le Dict pending_event
## via "evid"). Un EVID absent de la table retombe sur DEFAULT_SLUG (conséquences —
## un parchemin neutre plutôt qu'un trou). La table couvre les évènements ACTUELS
## + les slugs pour les familles à venir (registres v4/lot2) : ajouter un évènement
## = une ligne ici, aucun asset neuf tant qu'il rentre dans un thème existant.

const DIR := "res://art/events/"
const DEFAULT_SLUG := "consequences"

## Slug → chemin de texture (les 8 fichiers livrés).
const SLUG_FILE := {
	"revolte_silencieuse": "event_revolte_silencieuse_512x128.png",
	"greniers_scelles":    "event_greniers_scelles_512x128.png",
	"cour_qui_murmure":    "event_cour_qui_murmure_512x128.png",
	"routes_marchandes":   "event_routes_marchandes_512x128.png",
	"eau_noire":           "event_eau_noire_512x128.png",
	"schisme":             "event_schisme_512x128.png",
	"grand_engrenage":     "event_grand_engrenage_512x128.png",
	"consequences":        "event_consequences_512x128.png",
}

## EVID (valeur de l'enum EvId) → slug de thème. L'ORDRE de l'enum (scps_events.h) :
## 0 QUAKE · 1 FLOOD · 2 DROUGHT · 3 FIRE · 4 PLAGUE · 5-8 INTEG_* · 9 SUCCESSION ·
## 10 SCHISM · 11 XENOPHILE · 12 XENOPHOBE · 13 MARBRIVE · 14 PONT_EFFONDRE.
## Réutilisation assumée : les chocs géo partagent « eau_noire » (le désastre du monde),
## l'intégration/succession/xénophobie partagent « cour_qui_murmure » (la politique de cour).
const EVID_SLUG := {
	0:  "eau_noire",           # QUAKE — désastre géo
	1:  "eau_noire",           # FLOOD
	2:  "greniers_scelles",    # DROUGHT — la sécheresse frappe les vivres
	3:  "eau_noire",           # FIRE
	4:  "consequences",        # PLAGUE — l'après
	5:  "cour_qui_murmure",    # INTEG_DOMINATEUR
	6:  "routes_marchandes",   # INTEG_MERCANTILE — l'intégration marchande
	7:  "cour_qui_murmure",    # INTEG_BUREAUCRATE
	8:  "cour_qui_murmure",    # INTEG_ANCIEN
	9:  "cour_qui_murmure",    # SUCCESSION — l'intrigue de cour
	10: "schisme",             # SCHISM
	11: "cour_qui_murmure",    # XENOPHILE — le creuset au conseil
	12: "cour_qui_murmure",    # XENOPHOBE
	13: "revolte_silencieuse", # MARBRIVE — les hommes refusent le chantier
	14: "consequences",        # PONT_EFFONDRE — la cicatrice mûrie
}

## Cache des textures chargées (une seule fois par slug — réutilisation mémoire).
static var _cache := {}

## Texture d'illustration pour un EVID (ou null si le fichier manque). Réutilise le
## cache : la même bannière sert tous les évènements de son thème.
static func texture_for(evid: int) -> Texture2D:
	var slug: String = EVID_SLUG.get(evid, DEFAULT_SLUG)
	return _texture(slug)

static func _texture(slug: String) -> Texture2D:
	if _cache.has(slug):
		return _cache[slug]
	var file: String = SLUG_FILE.get(slug, SLUG_FILE[DEFAULT_SLUG])
	var tex: Texture2D = load(DIR + file)
	_cache[slug] = tex
	return tex
