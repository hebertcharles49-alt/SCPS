class_name SettlementStampCatalog
extends RefCounted

const BASE_PATH := "res://art/map_stamps/lot1/assets_alpha/"

const STAMPS := {
    "city_t1": {"category": "city", "tier": 1, "path": BASE_PATH + "city_t1.png"},
    "city_t2": {"category": "city", "tier": 2, "path": BASE_PATH + "city_t2.png"},
    "city_t3": {"category": "city", "tier": 3, "path": BASE_PATH + "city_t3.png"},
    "city_t4": {"category": "city", "tier": 4, "path": BASE_PATH + "city_t4.png"},
    "city_t5": {"category": "city", "tier": 5, "path": BASE_PATH + "city_t5.png"},
    "city_t6": {"category": "city", "tier": 6, "path": BASE_PATH + "city_t6.png"},
    "city_t7": {"category": "city", "tier": 7, "path": BASE_PATH + "city_t7.png"},
    "port_city_t1": {"category": "port_city", "tier": 1, "path": BASE_PATH + "port_city_t1.png"},
    "port_city_t2": {"category": "port_city", "tier": 2, "path": BASE_PATH + "port_city_t2.png"},
    "port_city_t3": {"category": "port_city", "tier": 3, "path": BASE_PATH + "port_city_t3.png"},
    "port_city_t4": {"category": "port_city", "tier": 4, "path": BASE_PATH + "port_city_t4.png"},
    "port_city_t5": {"category": "port_city", "tier": 5, "path": BASE_PATH + "port_city_t5.png"},
    "port_city_t6": {"category": "port_city", "tier": 6, "path": BASE_PATH + "port_city_t6.png"},
    "port_city_t7": {"category": "port_city", "tier": 7, "path": BASE_PATH + "port_city_t7.png"},
    "wild_hamlet": {"category": "wild_hamlet", "tier": 0, "path": BASE_PATH + "wild_hamlet.png"},
    "city_state": {"category": "city_state", "tier": 0, "path": BASE_PATH + "city_state.png"},
}

static func id_for_settlement(tier: int, has_port: bool, is_wild: bool = false, is_city_state: bool = false) -> String:
    if is_city_state:
        return "city_state"
    if is_wild:
        return "wild_hamlet"

    var clamped_tier := clampi(tier, 1, 7)
    if has_port:
        return "port_city_t%d" % clamped_tier
    return "city_t%d" % clamped_tier


static func texture_path(stamp_id: String) -> String:
    if not STAMPS.has(stamp_id):
        return ""
    return STAMPS[stamp_id]["path"]


static func load_texture(stamp_id: String) -> Texture2D:
    var path := texture_path(stamp_id)
    if path.is_empty():
        return null
    return load(path)


static func draw_size_for_zoom(zoom: float, tier: int = 1, selected: bool = false) -> float:
    var base_size := lerpf(28.0, 52.0, clampf(float(tier - 1) / 6.0, 0.0, 1.0))
    var zoom_factor := clampf(zoom, 0.65, 1.35)
    if selected:
        base_size += 8.0
    return base_size * zoom_factor
