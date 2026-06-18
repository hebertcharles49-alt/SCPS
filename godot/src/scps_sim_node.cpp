/*
 * scps_sim_node.cpp — implémentation du node Godot (voir .h).
 * Chaque méthode est un mince passe-plat vers scps_api (la façade C).
 */
#include "scps_sim_node.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ScpsWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "seed"),        &ScpsWorld::generate);
    ClassDB::bind_method(D_METHOD("advance_days", "days"),    &ScpsWorld::advance_days);
    ClassDB::bind_method(D_METHOD("map_w"),                   &ScpsWorld::map_w);
    ClassDB::bind_method(D_METHOD("map_h"),                   &ScpsWorld::map_h);
    ClassDB::bind_method(D_METHOD("map_image", "mode"),       &ScpsWorld::map_image);
    ClassDB::bind_method(D_METHOD("layer_image", "layer"),    &ScpsWorld::layer_image);
    ClassDB::bind_method(D_METHOD("year"),                    &ScpsWorld::year);
    ClassDB::bind_method(D_METHOD("player"),                  &ScpsWorld::player);
    ClassDB::bind_method(D_METHOD("country_count"),           &ScpsWorld::country_count);
    ClassDB::bind_method(D_METHOD("region_count"),            &ScpsWorld::region_count);
    ClassDB::bind_method(D_METHOD("world_pop"),               &ScpsWorld::world_pop);
    ClassDB::bind_method(D_METHOD("country_pop", "country"),  &ScpsWorld::country_pop);
    ClassDB::bind_method(D_METHOD("country_gold", "country"), &ScpsWorld::country_gold);
    ClassDB::bind_method(D_METHOD("region_owner", "region"),     &ScpsWorld::region_owner);
    ClassDB::bind_method(D_METHOD("region_pop", "region"),       &ScpsWorld::region_pop);
    ClassDB::bind_method(D_METHOD("region_colonized", "region"), &ScpsWorld::region_colonized);
    ClassDB::bind_method(D_METHOD("region_centroid", "region"),  &ScpsWorld::region_centroid);

    /* couches brutes (scps_map_layer) — int en clair côté GDScript :
     * 0 = HEIGHT · 1 = SEA · 2 = BIOME · 3 = COAST */
    BIND_CONSTANT(SCPS_LAYER_HEIGHT);
    BIND_CONSTANT(SCPS_LAYER_SEA);
    BIND_CONSTANT(SCPS_LAYER_BIOME);
    BIND_CONSTANT(SCPS_LAYER_COAST);
}

ScpsWorld::ScpsWorld()  { sim = scps_sim_new(); }
ScpsWorld::~ScpsWorld() { if (sim) { scps_sim_free(sim); sim = nullptr; } }

void ScpsWorld::generate(int seed)      { if (sim) scps_sim_generate(sim, (uint32_t)seed); }
void ScpsWorld::advance_days(int days)  { if (sim) scps_sim_advance_days(sim, days); }

int ScpsWorld::map_w() const { return scps_map_w(); }
int ScpsWorld::map_h() const { return scps_map_h(); }

Ref<Image> ScpsWorld::map_image(int mode) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    if (sim) scps_map_rgba(sim, buf.ptrw(), mode);
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

Ref<Image> ScpsWorld::layer_image(int layer) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h);
    if (sim) scps_map_layer(sim, buf.ptrw(), layer);
    return Image::create_from_data(w, h, false, Image::FORMAT_L8, buf);
}

int     ScpsWorld::year()          const { return scps_year(sim); }
int     ScpsWorld::player()        const { return scps_player(sim); }
int     ScpsWorld::country_count() const { return scps_country_count(sim); }
int     ScpsWorld::region_count()  const { return scps_region_count(sim); }
int64_t ScpsWorld::world_pop()     const { return (int64_t)scps_world_pop(sim); }
int64_t ScpsWorld::country_pop(int c)  const { return (int64_t)scps_country_pop(sim, c); }
double  ScpsWorld::country_gold(int c) const { return scps_country_gold(sim, c); }

int     ScpsWorld::region_owner(int r)     const { return scps_region_owner(sim, r); }
int64_t ScpsWorld::region_pop(int r)       const { return (int64_t)scps_region_pop(sim, r); }
bool    ScpsWorld::region_colonized(int r) const { return scps_region_colonized(sim, r); }

Vector2 ScpsWorld::region_centroid(int r) const {
    float x = -1.f, y = -1.f;
    scps_region_centroid(sim, r, &x, &y);
    return Vector2(x, y);
}
