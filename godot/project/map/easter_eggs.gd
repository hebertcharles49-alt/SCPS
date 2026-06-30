class_name MapEasterEggCatalog
extends RefCounted

const BASE_PATH := "res://art/map_stamps/lot4_easter_eggs/assets_alpha/"

const STAMPS := {
    "sea_serpent_01": {"category": "sea_serpent", "layer": "sea_easter_egg", "path": BASE_PATH + "sea_serpent_01.png", "size": Vector2i(1024, 512)},
    "sea_serpent_02": {"category": "sea_serpent", "layer": "sea_easter_egg", "path": BASE_PATH + "sea_serpent_02.png", "size": Vector2i(1024, 512)},
    "shipwreck_hull_01": {"category": "shipwreck", "layer": "wreck", "path": BASE_PATH + "shipwreck_hull_01.png", "size": Vector2i(512, 512)},
    "broken_mast_01": {"category": "shipwreck", "layer": "wreck", "path": BASE_PATH + "broken_mast_01.png", "size": Vector2i(512, 512)},
    "floating_debris_01": {"category": "shipwreck", "layer": "wreck", "path": BASE_PATH + "floating_debris_01.png", "size": Vector2i(512, 512)},
    "half_sunk_wreck_01": {"category": "shipwreck", "layer": "wreck", "path": BASE_PATH + "half_sunk_wreck_01.png", "size": Vector2i(512, 512)},
    "jagged_reef_01": {"category": "reef_stones", "layer": "hazard", "path": BASE_PATH + "jagged_reef_01.png", "size": Vector2i(512, 512)},
    "low_rocks_01": {"category": "reef_stones", "layer": "hazard", "path": BASE_PATH + "low_rocks_01.png", "size": Vector2i(512, 512)},
    "sea_stacks_01": {"category": "reef_stones", "layer": "hazard", "path": BASE_PATH + "sea_stacks_01.png", "size": Vector2i(512, 512)},
    "shoal_stones_01": {"category": "reef_stones", "layer": "hazard", "path": BASE_PATH + "shoal_stones_01.png", "size": Vector2i(512, 512)},
    "apoc_rabbit_banner_01": {"category": "apocalypse_rabbit", "layer": "map_easter_egg", "path": BASE_PATH + "apoc_rabbit_banner_01.png", "size": Vector2i(512, 512)},
    "apoc_rabbit_horn_01": {"category": "apocalypse_rabbit", "layer": "map_easter_egg", "path": BASE_PATH + "apoc_rabbit_horn_01.png", "size": Vector2i(512, 512)},
    "apoc_rabbit_spear_01": {"category": "apocalypse_rabbit", "layer": "map_easter_egg", "path": BASE_PATH + "apoc_rabbit_spear_01.png", "size": Vector2i(512, 512)},
    "apoc_rabbit_crown_01": {"category": "apocalypse_rabbit", "layer": "map_easter_egg", "path": BASE_PATH + "apoc_rabbit_crown_01.png", "size": Vector2i(512, 512)},
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
