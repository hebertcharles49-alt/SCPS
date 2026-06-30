class_name BiomeDressingCatalog
extends RefCounted

const BASE_PATH := "res://art/map_stamps/lot3_biomes/assets_alpha/"

const STAMPS := {
    "savanna_grass_01": {"category": "savanna", "layer": "dry_vegetation", "path": BASE_PATH + "savanna_grass_01.png"},
    "savanna_grass_02": {"category": "savanna", "layer": "dry_vegetation", "path": BASE_PATH + "savanna_grass_02.png"},
    "acacia_mark_01": {"category": "savanna", "layer": "dry_vegetation", "path": BASE_PATH + "acacia_mark_01.png"},
    "savanna_sparse_tree_01": {"category": "savanna", "layer": "dry_vegetation", "path": BASE_PATH + "savanna_sparse_tree_01.png"},
    "marsh_reeds_01": {"category": "marsh", "layer": "wetland", "path": BASE_PATH + "marsh_reeds_01.png"},
    "marsh_reeds_02": {"category": "marsh", "layer": "wetland", "path": BASE_PATH + "marsh_reeds_02.png"},
    "marsh_ripple_reeds_01": {"category": "marsh", "layer": "wetland", "path": BASE_PATH + "marsh_ripple_reeds_01.png"},
    "marsh_tufts_01": {"category": "marsh", "layer": "wetland", "path": BASE_PATH + "marsh_tufts_01.png"},
    "sea_ripples_01": {"category": "sea_ocean", "layer": "water", "path": BASE_PATH + "sea_ripples_01.png"},
    "sea_ripples_02": {"category": "sea_ocean", "layer": "water", "path": BASE_PATH + "sea_ripples_02.png"},
    "ocean_swell_lines_01": {"category": "sea_ocean", "layer": "water", "path": BASE_PATH + "ocean_swell_lines_01.png"},
    "ocean_current_swirl_01": {"category": "sea_ocean", "layer": "water", "path": BASE_PATH + "ocean_current_swirl_01.png"},
    "plain_grass_01": {"category": "plain", "layer": "grassland", "path": BASE_PATH + "plain_grass_01.png"},
    "plain_grass_02": {"category": "plain", "layer": "grassland", "path": BASE_PATH + "plain_grass_02.png"},
    "plain_wind_strokes_01": {"category": "plain", "layer": "grassland", "path": BASE_PATH + "plain_wind_strokes_01.png"},
    "plain_sparse_tufts_01": {"category": "plain", "layer": "grassland", "path": BASE_PATH + "plain_sparse_tufts_01.png"},
    "steppe_grass_01": {"category": "steppe", "layer": "dry_grassland", "path": BASE_PATH + "steppe_grass_01.png"},
    "steppe_grass_02": {"category": "steppe", "layer": "dry_grassland", "path": BASE_PATH + "steppe_grass_02.png"},
    "steppe_dry_strokes_01": {"category": "steppe", "layer": "dry_grassland", "path": BASE_PATH + "steppe_dry_strokes_01.png"},
    "steppe_tufts_01": {"category": "steppe", "layer": "dry_grassland", "path": BASE_PATH + "steppe_tufts_01.png"},
}

static func texture_path(stamp_id: String) -> String:
    if not STAMPS.has(stamp_id):
        return ""
    return STAMPS[stamp_id]["path"]


static func load_texture(stamp_id: String) -> Texture2D:
    var path := texture_path(stamp_id)
    if path.is_empty():
        return null
    return load(path)


static func ids_for_category(category: String) -> Array[String]:
    var ids: Array[String] = []
    for stamp_id in STAMPS.keys():
        if STAMPS[stamp_id]["category"] == category:
            ids.append(stamp_id)
    return ids


static func draw_size_for_category(category: String, zoom: float) -> float:
    var base_size := 26.0
    if category == "sea_ocean":
        base_size = 34.0
    elif category in ["marsh", "savanna", "steppe"]:
        base_size = 28.0
    elif category == "plain":
        base_size = 22.0
    return base_size * clampf(zoom, 0.65, 1.35)
