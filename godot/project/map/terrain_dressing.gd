class_name TerrainDressingCatalog
extends RefCounted

const BASE_PATH := "res://art/map_stamps/lot2_painted/assets_alpha/"

const STAMPS := {
    "mountain_single_01": {"category": "mountain", "layer": "relief", "path": BASE_PATH + "mountain_single_01.png"},
    "mountain_single_02": {"category": "mountain", "layer": "relief", "path": BASE_PATH + "mountain_single_02.png"},
    "mountain_range_01": {"category": "mountain_range", "layer": "relief", "path": BASE_PATH + "mountain_range_01.png"},
    "mountain_range_02": {"category": "mountain_range", "layer": "relief", "path": BASE_PATH + "mountain_range_02.png"},
    "mountain_pass_01": {"category": "mountain_pass", "layer": "relief", "path": BASE_PATH + "mountain_pass_01.png"},
    "hill_mark_01": {"category": "hill", "layer": "relief", "path": BASE_PATH + "hill_mark_01.png"},
    "hill_cluster_01": {"category": "hill", "layer": "relief", "path": BASE_PATH + "hill_cluster_01.png"},
    "rocky_outcrop_01": {"category": "rocks", "layer": "relief", "path": BASE_PATH + "rocky_outcrop_01.png"},
    "tree_broadleaf_01": {"category": "tree", "layer": "vegetation", "path": BASE_PATH + "tree_broadleaf_01.png"},
    "tree_pine_01": {"category": "tree", "layer": "vegetation", "path": BASE_PATH + "tree_pine_01.png"},
    "forest_sparse_01": {"category": "forest", "layer": "vegetation", "path": BASE_PATH + "forest_sparse_01.png"},
    "forest_dense_01": {"category": "forest", "layer": "vegetation", "path": BASE_PATH + "forest_dense_01.png"},
    "reeds_01": {"category": "reeds", "layer": "wetland", "path": BASE_PATH + "reeds_01.png"},
    "water_ripples_01": {"category": "water_motion", "layer": "water", "path": BASE_PATH + "water_ripples_01.png"},
    "scrub_brush_01": {"category": "scrub", "layer": "dry_vegetation", "path": BASE_PATH + "scrub_brush_01.png"},
    "dune_wind_lines_01": {"category": "dune_wind", "layer": "dry", "path": BASE_PATH + "dune_wind_lines_01.png"},
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
    var base_size := 28.0
    if category in ["mountain", "mountain_range", "mountain_pass"]:
        base_size = 42.0
    elif category in ["forest", "tree", "rocks"]:
        base_size = 34.0
    elif category in ["water_motion", "reeds", "scrub", "dune_wind", "hill"]:
        base_size = 26.0
    return base_size * clampf(zoom, 0.65, 1.35)
