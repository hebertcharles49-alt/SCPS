extends Node

const Palette = preload("res://ui/search_palette.gd")
const Rank = preload("res://ui/search_rank.gd")
const InfoRef = preload("res://ui/info_ref.gd")

func _ready() -> void:
	var palette = Palette.new()
	var entries: Array = palette.build_entries(Sim.world)
	assert(entries.size() > 30)
	var kinds := {}
	for entry in entries:
		kinds[String(entry.get("kind", ""))] = true
		var request: Dictionary = entry.get("request", {})
		assert(InfoRef.request_key(request) != "")
	assert(kinds.has("Action") and kinds.has("Concept") and kinds.has("Carte") and kinds.has("Panneau"))
	assert(kinds.has("Pays") and kinds.has("Province") and kinds.has("Région") and kinds.has("Technologie"))
	assert(Rank.normalize("Économie — Marché") == "economie marche")
	var ranked := palette.rank_entries("economie", entries)
	assert(not ranked.is_empty())
	assert(Rank.normalize(String(ranked[0].get("title", ""))).contains("economie"))
	print("SEARCH_PALETTE_TEST_OK · %d entrées indexées · %d résultats économie" % [entries.size(), ranked.size()])
	palette.free()
	get_tree().quit(0)
