/*
 * scps_sim_node.cpp — implémentation du node Godot (voir .h).
 * Chaque méthode est un mince passe-plat vers scps_api (la façade C).
 */
#include "scps_sim_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <vector>    /* political_image : tampon owner int16 (boucle 512k cellules en C++) */
#include <cstring>

using namespace godot;

void ScpsWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "seed"),        &ScpsWorld::generate);
    ClassDB::bind_method(D_METHOD("advance_days", "days"),    &ScpsWorld::advance_days);
    ClassDB::bind_method(D_METHOD("map_w"),                   &ScpsWorld::map_w);
    ClassDB::bind_method(D_METHOD("map_h"),                   &ScpsWorld::map_h);
    ClassDB::bind_method(D_METHOD("map_image", "mode", "selected_prov"), &ScpsWorld::map_image, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("layer_image", "layer"),    &ScpsWorld::layer_image);
    ClassDB::bind_method(D_METHOD("political_image", "pal"),  &ScpsWorld::political_image);
    ClassDB::bind_method(D_METHOD("year"),                    &ScpsWorld::year);
    ClassDB::bind_method(D_METHOD("player"),                  &ScpsWorld::player);
    ClassDB::bind_method(D_METHOD("country_count"),           &ScpsWorld::country_count);
    ClassDB::bind_method(D_METHOD("country_province_count", "country"), &ScpsWorld::country_province_count);
    ClassDB::bind_method(D_METHOD("region_count"),            &ScpsWorld::region_count);
    ClassDB::bind_method(D_METHOD("province_count"),          &ScpsWorld::province_count);
    ClassDB::bind_method(D_METHOD("world_pop"),               &ScpsWorld::world_pop);
    ClassDB::bind_method(D_METHOD("country_pop", "country"),  &ScpsWorld::country_pop);
    ClassDB::bind_method(D_METHOD("country_gold", "country"), &ScpsWorld::country_gold);
    ClassDB::bind_method(D_METHOD("country_role", "country"), &ScpsWorld::country_role);
    ClassDB::bind_method(D_METHOD("region_owner", "region"),     &ScpsWorld::region_owner);
    ClassDB::bind_method(D_METHOD("region_pop", "region"),       &ScpsWorld::region_pop);
    ClassDB::bind_method(D_METHOD("region_colonized", "region"), &ScpsWorld::region_colonized);
    ClassDB::bind_method(D_METHOD("region_centroid", "region"),  &ScpsWorld::region_centroid);

    ClassDB::bind_method(D_METHOD("province_at", "x", "y"),          &ScpsWorld::province_at);
    ClassDB::bind_method(D_METHOD("province_region", "province"),    &ScpsWorld::province_region);
    ClassDB::bind_method(D_METHOD("province_info", "province"),      &ScpsWorld::province_info);
    ClassDB::bind_method(D_METHOD("country_info", "country"),        &ScpsWorld::country_info);
    ClassDB::bind_method(D_METHOD("army_info", "country"),           &ScpsWorld::army_info);
    ClassDB::bind_method(D_METHOD("region_tier", "region"),          &ScpsWorld::region_tier);
    ClassDB::bind_method(D_METHOD("region_settle_group", "region"),  &ScpsWorld::region_settle_group);
    ClassDB::bind_method(D_METHOD("region_war_state", "region"),     &ScpsWorld::region_war_state);
    ClassDB::bind_method(D_METHOD("battle_info", "region"),          &ScpsWorld::battle_info);
    ClassDB::bind_method(D_METHOD("endgame_info"),                   &ScpsWorld::endgame_info);
    ClassDB::bind_method(D_METHOD("region_sunken", "region"),        &ScpsWorld::region_sunken);
    ClassDB::bind_method(D_METHOD("endgame_region_intensity", "region"), &ScpsWorld::endgame_region_intensity);
    ClassDB::bind_method(D_METHOD("variant_map_image"),              &ScpsWorld::variant_map_image);
    ClassDB::bind_method(D_METHOD("province_groups", "province"),    &ScpsWorld::province_groups);
    ClassDB::bind_method(D_METHOD("province_income", "province"),    &ScpsWorld::province_income);
    ClassDB::bind_method(D_METHOD("province_agitation", "province"), &ScpsWorld::province_agitation);
    ClassDB::bind_method(D_METHOD("province_buildings", "province"), &ScpsWorld::province_buildings);
    ClassDB::bind_method(D_METHOD("province_log", "province"),       &ScpsWorld::province_log);
    ClassDB::bind_method(D_METHOD("province_classes", "province"),   &ScpsWorld::province_classes);
    ClassDB::bind_method(D_METHOD("province_capitale", "province"),  &ScpsWorld::province_capitale);
    ClassDB::bind_method(D_METHOD("province_slave_count", "province"),   &ScpsWorld::province_slave_count);
    ClassDB::bind_method(D_METHOD("province_tax", "province"),           &ScpsWorld::province_tax);
    ClassDB::bind_method(D_METHOD("province_defense_pct", "province"),   &ScpsWorld::province_defense_pct);
    ClassDB::bind_method(D_METHOD("province_seed", "province"),          &ScpsWorld::province_seed);
    ClassDB::bind_method(D_METHOD("province_market", "province"),        &ScpsWorld::province_market);
    ClassDB::bind_method(D_METHOD("country_demo", "country"),        &ScpsWorld::country_demo);
    ClassDB::bind_method(D_METHOD("country_stocks", "country"),      &ScpsWorld::country_stocks);
    ClassDB::bind_method(D_METHOD("country_relations", "country"),   &ScpsWorld::country_relations);
    ClassDB::bind_method(D_METHOD("diplo_options", "target"),        &ScpsWorld::diplo_options);
    ClassDB::bind_method(D_METHOD("opinion_summary", "country"),     &ScpsWorld::opinion_summary);
    ClassDB::bind_method(D_METHOD("diplo_journal", "country"),       &ScpsWorld::diplo_journal);
    ClassDB::bind_method(D_METHOD("country_army", "country"),        &ScpsWorld::country_army);
    ClassDB::bind_method(D_METHOD("country_trade", "country"),       &ScpsWorld::country_trade);
    ClassDB::bind_method(D_METHOD("commerce_power", "country"),      &ScpsWorld::commerce_power);
    ClassDB::bind_method(D_METHOD("country_council", "country"),     &ScpsWorld::country_council);
    ClassDB::bind_method(D_METHOD("decrees_list", "country"),        &ScpsWorld::decrees_list);
    ClassDB::bind_method(D_METHOD("unit_roster", "country"),         &ScpsWorld::unit_roster);
    ClassDB::bind_method(D_METHOD("building_roster", "country"),     &ScpsWorld::building_roster);
    ClassDB::bind_method(D_METHOD("tech_info"),                      &ScpsWorld::tech_info);
    ClassDB::bind_method(D_METHOD("tech_nodes"),                     &ScpsWorld::tech_nodes);
    ClassDB::bind_method(D_METHOD("heritage_access"),                &ScpsWorld::heritage_access);
    ClassDB::bind_method(D_METHOD("merv_metab"),                     &ScpsWorld::merv_metab);
    ClassDB::bind_method(D_METHOD("tunables"),                       &ScpsWorld::tunables);
    ClassDB::bind_method(D_METHOD("tune_set", "nom", "value"),       &ScpsWorld::tune_set);
    ClassDB::bind_method(D_METHOD("lang_set", "lang"),               &ScpsWorld::lang_set);
    ClassDB::bind_method(D_METHOD("lang_get"),                       &ScpsWorld::lang_get);
    ClassDB::bind_method(D_METHOD("country_budget", "country"),      &ScpsWorld::country_budget);
    ClassDB::bind_method(D_METHOD("budget_summary", "country"),      &ScpsWorld::budget_summary);
    ClassDB::bind_method(D_METHOD("mission_info", "country"),        &ScpsWorld::mission_info);
    ClassDB::bind_method(D_METHOD("country_factions", "country"),    &ScpsWorld::country_factions);
    ClassDB::bind_method(D_METHOD("player_build", "edifice", "region"), &ScpsWorld::player_build, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("player_recruit", "unit"),         &ScpsWorld::player_recruit);
    ClassDB::bind_method(D_METHOD("player_set_levy", "level"),       &ScpsWorld::player_set_levy);
    ClassDB::bind_method(D_METHOD("player_research", "tech"),        &ScpsWorld::player_research);
    ClassDB::bind_method(D_METHOD("research_status"),               &ScpsWorld::research_status);
    ClassDB::bind_method(D_METHOD("age_state"),                     &ScpsWorld::age_state);
    ClassDB::bind_method(D_METHOD("player_age_engage"),             &ScpsWorld::player_age_engage);
    ClassDB::bind_method(D_METHOD("feed_poll", "after_seq"),        &ScpsWorld::feed_poll);
    ClassDB::bind_method(D_METHOD("pending_count"),                 &ScpsWorld::pending_count);
    ClassDB::bind_method(D_METHOD("pending_event", "slot"),         &ScpsWorld::pending_event);
    ClassDB::bind_method(D_METHOD("player_event_choice", "slot", "option"), &ScpsWorld::player_event_choice);
    ClassDB::bind_method(D_METHOD("annals"),                         &ScpsWorld::annals);
    ClassDB::bind_method(D_METHOD("player_alerts"),                 &ScpsWorld::player_alerts);
    ClassDB::bind_method(D_METHOD("player_colonize", "prov"),       &ScpsWorld::player_colonize);
    ClassDB::bind_method(D_METHOD("can_colonize", "prov"),          &ScpsWorld::can_colonize);
    /* §3 — le RESTE de la surface de verbes (wiring UI complet) : intérieur · conseil ·
     * commerce · guerre. Passe-plats vers scps_player_* (journal déterministe, drain revalidé). */
    ClassDB::bind_method(D_METHOD("player_repress", "region"),          &ScpsWorld::player_repress);
    ClassDB::bind_method(D_METHOD("player_assimilate", "region", "creuset"), &ScpsWorld::player_assimilate);
    ClassDB::bind_method(D_METHOD("player_purge", "region"),            &ScpsWorld::player_purge);
    ClassDB::bind_method(D_METHOD("player_council_hire", "seat", "slot"), &ScpsWorld::player_council_hire);
    ClassDB::bind_method(D_METHOD("player_council_dismiss", "seat"),    &ScpsWorld::player_council_dismiss);
    ClassDB::bind_method(D_METHOD("player_council_pay", "seat", "pay"), &ScpsWorld::player_council_pay);
    ClassDB::bind_method(D_METHOD("council_pair_state", "seat_a", "seat_b"), &ScpsWorld::council_pair_state);
    ClassDB::bind_method(D_METHOD("council_candidates", "seat"),        &ScpsWorld::council_candidates);
    ClassDB::bind_method(D_METHOD("player_decree", "id", "on"),         &ScpsWorld::player_decree);
    ClassDB::bind_method(D_METHOD("player_route", "ra", "rb", "maritime"), &ScpsWorld::player_route);
    ClassDB::bind_method(D_METHOD("player_market_buy", "region", "good", "qty", "tier"),  &ScpsWorld::player_market_buy);
    ClassDB::bind_method(D_METHOD("player_market_sell", "region", "good", "qty", "tier"), &ScpsWorld::player_market_sell);
    ClassDB::bind_method(D_METHOD("player_campaign", "from_region", "target_region"), &ScpsWorld::player_campaign);
    ClassDB::bind_method(D_METHOD("player_posture", "posture"),         &ScpsWorld::player_posture);
    ClassDB::bind_method(D_METHOD("player_refill"),                     &ScpsWorld::player_refill);
    ClassDB::bind_method(D_METHOD("player_navy_build", "hull"),         &ScpsWorld::player_navy_build);
    ClassDB::bind_method(D_METHOD("player_disband"),                    &ScpsWorld::player_disband);
    ClassDB::bind_method(D_METHOD("player_build_manuf", "region", "bld"), &ScpsWorld::player_build_manuf);
    ClassDB::bind_method(D_METHOD("manuf_legal", "region", "bld"),        &ScpsWorld::manuf_legal);
    ClassDB::bind_method(D_METHOD("manuf_name", "bld"),                   &ScpsWorld::manuf_name);
    ClassDB::bind_method(D_METHOD("colonized_total"),               &ScpsWorld::colonized_total);
    ClassDB::bind_method(D_METHOD("colony_status"),                 &ScpsWorld::colony_status);
    ClassDB::bind_method(D_METHOD("country_food", "c"),             &ScpsWorld::country_food);
    ClassDB::bind_method(D_METHOD("diplo_cd"),                      &ScpsWorld::diplo_cd);
    ClassDB::bind_method(D_METHOD("country_capital_province", "c"), &ScpsWorld::country_capital_province);
    ClassDB::bind_method(D_METHOD("player_declare_war", "target"),    &ScpsWorld::player_declare_war);
    ClassDB::bind_method(D_METHOD("player_make_peace", "target"),     &ScpsWorld::player_make_peace);
    ClassDB::bind_method(D_METHOD("player_offer_alliance", "target"), &ScpsWorld::player_offer_alliance);
    ClassDB::bind_method(D_METHOD("player_offer_pact", "target"),     &ScpsWorld::player_offer_pact);
    ClassDB::bind_method(D_METHOD("player_offer_migration", "target"),&ScpsWorld::player_offer_migration);
    ClassDB::bind_method(D_METHOD("player_embargo", "target", "on"),  &ScpsWorld::player_embargo);
    ClassDB::bind_method(D_METHOD("player_fabricate_cb", "target"),   &ScpsWorld::player_fabricate_cb);

    /* ALLOCATION DE MAIN-D'ŒUVRE (onglet province) */
    ClassDB::bind_method(D_METHOD("region_alloc", "region"),              &ScpsWorld::region_alloc);
    ClassDB::bind_method(D_METHOD("player_alloc_raw", "region", "resource", "weight"), &ScpsWorld::player_alloc_raw);
    ClassDB::bind_method(D_METHOD("player_alloc_bld", "region", "bld_type", "weight"), &ScpsWorld::player_alloc_bld);
    ClassDB::bind_method(D_METHOD("player_alloc_input", "region", "bld_type", "input"), &ScpsWorld::player_alloc_input);
    ClassDB::bind_method(D_METHOD("player_alloc_auto", "region"),         &ScpsWorld::player_alloc_auto);
    ClassDB::bind_method(D_METHOD("player_pop_transfer", "src_region", "dst_region", "klass", "count"), &ScpsWorld::player_pop_transfer);
    ClassDB::bind_method(D_METHOD("manumit_preview"),                     &ScpsWorld::manumit_preview);
    ClassDB::bind_method(D_METHOD("player_manumit"),                      &ScpsWorld::player_manumit);
    ClassDB::bind_method(D_METHOD("player_slave_buy", "region", "count"), &ScpsWorld::player_slave_buy);
    ClassDB::bind_method(D_METHOD("player_slave_sell", "region", "count"), &ScpsWorld::player_slave_sell);
    ClassDB::bind_method(D_METHOD("slave_market"),                        &ScpsWorld::slave_market);

    /* CRÉATEUR DE CULTURE */
    ClassDB::bind_method(D_METHOD("heritage_list"),                  &ScpsWorld::heritage_list);
    ClassDB::bind_method(D_METHOD("ethos_list"),                     &ScpsWorld::ethos_list);
    ClassDB::bind_method(D_METHOD("tradition_list"),                 &ScpsWorld::tradition_list);
    ClassDB::bind_method(D_METHOD("culture_validate", "t0", "t1", "t2"), &ScpsWorld::culture_validate);
    ClassDB::bind_method(D_METHOD("culture_preview", "t0", "t1", "t2"),  &ScpsWorld::culture_preview);
    ClassDB::bind_method(D_METHOD("culture_name", "heritage", "seed"),   &ScpsWorld::culture_name);
    ClassDB::bind_method(D_METHOD("set_empire_culture", "slot", "heritage", "ethos", "t0", "t1", "t2"), &ScpsWorld::set_empire_culture);
    ClassDB::bind_method(D_METHOD("set_player_culture", "heritage", "ethos", "t0", "t1", "t2"), &ScpsWorld::set_player_culture);
    ClassDB::bind_method(D_METHOD("clear_player_culture"),          &ScpsWorld::clear_player_culture);
    ClassDB::bind_method(D_METHOD("worldparams_default", "seed"),   &ScpsWorld::worldparams_default);
    ClassDB::bind_method(D_METHOD("worldgen_set", "p"),             &ScpsWorld::worldgen_set);
    ClassDB::bind_method(D_METHOD("worldgen_clear"),                &ScpsWorld::worldgen_clear);

    /* RELIGION (P5) */
    ClassDB::bind_method(D_METHOD("religion_pole_list"),            &ScpsWorld::religion_pole_list);
    ClassDB::bind_method(D_METHOD("credo_list"),                    &ScpsWorld::credo_list);
    ClassDB::bind_method(D_METHOD("religion_picks_valid", "p0", "p1", "p2"), &ScpsWorld::religion_picks_valid);
    ClassDB::bind_method(D_METHOD("religion_found", "cid", "credo", "t0", "t1", "t2"), &ScpsWorld::religion_found);
    ClassDB::bind_method(D_METHOD("religion_eligible", "cid"),      &ScpsWorld::religion_eligible);
    ClassDB::bind_method(D_METHOD("religion_schism", "cid", "slot_a", "pole_a", "slot_b", "pole_b", "new_credo"), &ScpsWorld::religion_schism);
    ClassDB::bind_method(D_METHOD("religion_of_country", "cid"),    &ScpsWorld::religion_of_country);
    ClassDB::bind_method(D_METHOD("religion_of_region", "region"),  &ScpsWorld::religion_of_region);
    ClassDB::bind_method(D_METHOD("religion_recruit_scholar", "cid", "region"), &ScpsWorld::religion_recruit_scholar);
    ClassDB::bind_method(D_METHOD("religion_scholar_role", "cid"),  &ScpsWorld::religion_scholar_role);
    ClassDB::bind_method(D_METHOD("religion_name", "cid"),          &ScpsWorld::religion_name);
    ClassDB::bind_method(D_METHOD("religion_founding_ready", "cid"), &ScpsWorld::religion_founding_ready);
    ClassDB::bind_method(D_METHOD("religion_cap"),                  &ScpsWorld::religion_cap);
    ClassDB::bind_method(D_METHOD("religion_can_found"),            &ScpsWorld::religion_can_found);
    ClassDB::bind_method(D_METHOD("save_game", "slot"),             &ScpsWorld::save_game);
    ClassDB::bind_method(D_METHOD("load_game", "slot"),             &ScpsWorld::load_game);
    ClassDB::bind_method(D_METHOD("save_slots"),                    &ScpsWorld::save_slots);

    ClassDB::bind_method(D_METHOD("river_points"),                   &ScpsWorld::river_points);
    ClassDB::bind_method(D_METHOD("river_paths"),                    &ScpsWorld::river_paths);
    ClassDB::bind_method(D_METHOD("border_segments", "level"),       &ScpsWorld::border_segments);
    ClassDB::bind_method(D_METHOD("border_segments_col", "level"),   &ScpsWorld::border_segments_col);
    ClassDB::bind_method(D_METHOD("country_ethos", "c"),             &ScpsWorld::country_ethos);
    ClassDB::bind_method(D_METHOD("country_heritage", "c"),          &ScpsWorld::country_heritage);
    ClassDB::bind_method(D_METHOD("country_capital_region", "c"),    &ScpsWorld::country_capital_region);
    ClassDB::bind_method(D_METHOD("region_border_segments", "region"), &ScpsWorld::region_border_segments);
    ClassDB::bind_method(D_METHOD("province_border_segments", "prov"), &ScpsWorld::province_border_segments);
    ClassDB::bind_method(D_METHOD("road_paths"),                     &ScpsWorld::road_paths);

    /* couches brutes (scps_map_layer) — int en clair côté GDScript :
     * 0 = HEIGHT · 1 = SEA · 2 = BIOME · 3 = COAST · 4 = WATER · 5 = RIVER */
    BIND_CONSTANT(SCPS_LAYER_HEIGHT);
    BIND_CONSTANT(SCPS_LAYER_SEA);
    BIND_CONSTANT(SCPS_LAYER_BIOME);
    BIND_CONSTANT(SCPS_LAYER_COAST);
    BIND_CONSTANT(SCPS_LAYER_WATER);
    BIND_CONSTANT(SCPS_LAYER_RIVER);
}

ScpsWorld::ScpsWorld()  { sim = scps_sim_new(); }
ScpsWorld::~ScpsWorld() { if (sim) { scps_sim_free(sim); sim = nullptr; } }

void ScpsWorld::generate(int seed)      { if (sim) scps_sim_generate(sim, (uint32_t)seed); }
void ScpsWorld::advance_days(int days)  { if (sim) scps_sim_advance_days(sim, days); }

int ScpsWorld::map_w() const { return scps_map_w(); }
int ScpsWorld::map_h() const { return scps_map_h(); }

Ref<Image> ScpsWorld::map_image(int mode, int selected_prov) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    if (sim) scps_map_rgba(sim, buf.ptrw(), mode, selected_prov);
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

Ref<Image> ScpsWorld::layer_image(int layer) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h);
    if (sim) scps_map_layer(sim, buf.ptrw(), layer);
    return Image::create_from_data(w, h, false, Image::FORMAT_L8, buf);
}

/* LAVIS POLITIQUE : owner effectif par cellule (façade) teinté par la palette du front
 * (pal[pays] = pigment, alpha compris) → Image RGBA (transparent hors territoire). La
 * boucle 512k cellules vit ICI (C++) — en GDScript elle bloquerait la frame. */
Ref<Image> ScpsWorld::political_image(PackedColorArray pal) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    uint8_t *dst = buf.ptrw();
    memset(dst, 0, (size_t)w * h * 4);
    if (sim) {
        std::vector<int16_t> own((size_t)w * h);
        scps_map_owner(sim, own.data());
        const int np = (int)pal.size();
        for (int64_t i = 0; i < (int64_t)w * h; i++) {
            int o = own[(size_t)i];
            if (o < 0 || o >= np) continue;
            Color c = pal[o];
            dst[i*4+0] = (uint8_t)CLAMP((int)(c.r * 255.0f + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(c.g * 255.0f + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(c.b * 255.0f + 0.5f), 0, 255);
            dst[i*4+3] = (uint8_t)CLAMP((int)(c.a * 255.0f + 0.5f), 0, 255);
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

int     ScpsWorld::year()          const { return scps_year(sim); }
int     ScpsWorld::player()        const { return scps_player(sim); }
int     ScpsWorld::country_count() const { return scps_country_count(sim); }
int     ScpsWorld::country_province_count(int c) const { return scps_country_province_count(sim, c); }
int     ScpsWorld::region_count()  const { return scps_region_count(sim); }
int     ScpsWorld::province_count() const { return scps_province_count(sim); }
int64_t ScpsWorld::world_pop()     const { return (int64_t)scps_world_pop(sim); }
int64_t ScpsWorld::country_pop(int c)  const { return (int64_t)scps_country_pop(sim, c); }
double  ScpsWorld::country_gold(int c) const { return scps_country_gold(sim, c); }
int     ScpsWorld::country_role(int c) const { return scps_country_role(sim, c); }

int     ScpsWorld::region_owner(int r)     const { return scps_region_owner(sim, r); }
int64_t ScpsWorld::region_pop(int r)       const { return (int64_t)scps_region_pop(sim, r); }
bool    ScpsWorld::region_colonized(int r) const { return scps_region_colonized(sim, r); }

Vector2 ScpsWorld::region_centroid(int r) const {
    float x = -1.f, y = -1.f;
    scps_region_centroid(sim, r, &x, &y);
    return Vector2(x, y);
}

int ScpsWorld::province_at(int x, int y) const     { return scps_province_at(sim, x, y); }
int ScpsWorld::province_region(int p) const        { return scps_province_region(sim, p); }

/* province_info / country_info — la membrane assemblée en Dictionary : des MOTS
 * (String) et des nombres TANGIBLES. Le panneau GDScript ne lit que ces clés ;
 * aucun flottant moteur ne traverse. */
Dictionary ScpsWorld::province_info(int province) {
    Dictionary d;
    ScpsProvInfo p;
    scps_province_info(sim, province, &p);
    d["valide"] = (bool)p.valid;
    if (!p.valid) return d;
    d["nom"]            = String::utf8(p.nom);
    d["terrain"]        = String::utf8(p.terrain);
    d["climat"]         = String::utf8(p.climat);
    d["relief"]         = String::utf8(p.relief);
    d["heritage"]       = String::utf8(p.heritage);
    d["stature"]        = String::utf8(p.stature);
    d["flux"]           = String::utf8(p.flux);
    d["vocation"]       = String::utf8(p.vocation);
    d["ressource"]      = String::utf8(p.ressource);
    d["humeur"]         = String::utf8(p.humeur);
    d["lignee"]         = String::utf8(p.lignee);
    d["aisance"]        = String::utf8(p.aisance);
    d["defense"]        = String::utf8(p.defense);
    d["specialisation"] = String::utf8(p.specialisation);
    d["ames"]           = (int64_t)p.ames;
    d["owner"]          = p.owner;
    d["agitation"]      = p.agitation;
    d["aisance_val"]    = p.aisance_val;
    d["humeur_val"]     = p.humeur_val;
    d["seuil_revolte"]  = (bool)p.seuil_revolte;
    d["logements_libres"] = (int64_t)p.logements_libres;
    d["logements_cap"]    = (int64_t)p.logements_cap;
    d["services_libres"]  = (int64_t)p.services_libres;
    d["services_cap"]     = (int64_t)p.services_cap;
    Array mods;
    for (int i = 0; i < p.n_mods; i++) {
        Dictionary m;
        m["nom"]    = String::utf8(p.mods[i].nom);
        m["effet"]  = String::utf8(p.mods[i].effet);
        m["faveur"] = (bool)p.mods[i].faveur;
        mods.push_back(m);
    }
    d["mods"] = mods;
    return d;
}

Dictionary ScpsWorld::country_info(int country) {
    Dictionary d;
    ScpsCountryInfo c;
    scps_country_info(sim, country, &c);
    d["valide"] = (bool)c.valid;
    if (!c.valid) return d;
    d["nom"]            = String::utf8(c.nom);
    d["ethos"]          = String::utf8(c.ethos);
    d["pop"]            = (int64_t)c.pop;
    d["or"]             = c.gold;
    d["regions"]        = c.n_regions;
    d["stabilite"]      = c.stabilite;   d["stabilite_mot"]  = String::utf8(c.stabilite_mot);
    d["prosperite"]     = c.prosperite;  d["prosperite_mot"] = String::utf8(c.prosperite_mot);
    d["legitimite"]     = c.legitimite;  d["legitimite_mot"] = String::utf8(c.legitimite_mot);
    d["cohesion"]       = c.cohesion;    d["cohesion_mot"]   = String::utf8(c.cohesion_mot);
    d["savoir"]         = c.savoir;      d["savoir_mot"]     = String::utf8(c.savoir_mot);
    d["influence"]      = c.influence;
    d["corruption"]     = c.corruption;
    return d;
}

Dictionary ScpsWorld::army_info(int country) {
    Dictionary d;
    ScpsArmyInfo a;
    scps_army_info(sim, country, &a);
    d["active"] = (bool)a.active;
    if (!a.active) return d;
    d["region"]   = a.region;
    d["dest"]     = a.dest;
    d["owner"]    = a.owner;
    d["phase_id"] = a.phase_id;
    d["phase"]    = String::utf8(a.phase);
    d["units"]    = (int64_t)a.units;
    d["inf"]      = (int64_t)a.inf;
    d["arch"]     = (int64_t)a.arch;
    d["cav"]      = (int64_t)a.cav;
    d["mages"]    = (int64_t)a.mages;
    return d;
}

int ScpsWorld::region_tier(int region) const { return scps_region_tier(sim, region); }
int ScpsWorld::region_settle_group(int region) const { return scps_region_settle_group(sim, region); }

Dictionary ScpsWorld::region_war_state(int region) {
    Dictionary d;
    int belli = -1;
    int st = scps_region_war_state(sim, region, &belli);
    d["state"]       = st;           /* 0 paix · 1 assiégée · 2 occupée */
    d["belligerent"] = belli;        /* -1 si paix, sinon le pays qui assiège/occupe */
    return d;
}

Dictionary ScpsWorld::battle_info(int region) {
    Dictionary d;
    ScpsBattleInfo b;
    scps_battle_info(sim, region, &b);
    d["valid"] = (bool)b.valid;
    if (!b.valid) return d;
    d["region"]    = b.region;
    d["attacker"]  = b.attacker;
    d["defender"]  = b.defender;
    d["phase_id"]  = b.phase_id;
    d["phase"]     = String::utf8(b.phase);
    d["atk_units"] = (int64_t)b.atk_units;
    d["atk_inf"]   = (int64_t)b.atk_inf;
    d["atk_arch"]  = (int64_t)b.atk_arch;
    d["atk_cav"]   = (int64_t)b.atk_cav;
    d["atk_mages"] = (int64_t)b.atk_mages;
    d["def_units"] = (int64_t)b.def_units;
    d["def_inf"]   = (int64_t)b.def_inf;
    d["def_arch"]  = (int64_t)b.def_arch;
    d["def_cav"]   = (int64_t)b.def_cav;
    d["def_mages"] = (int64_t)b.def_mages;
    d["in_battle"] = (bool)b.in_battle;
    d["loss_atk"]  = (double)b.loss_atk;
    d["loss_def"]  = (double)b.loss_def;
    d["war_score"] = (double)b.war_score;
    return d;
}

Dictionary ScpsWorld::endgame_info() {
    Dictionary d;
    ScpsEndgameInfo e;
    scps_endgame_info(sim, &e);
    d["entropie_pct"]  = e.entropie_pct;
    d["entropie"]      = String::utf8(e.entropie);
    d["augure"]        = String::utf8(e.augure);
    d["fin"]           = e.fin;
    d["merv"]          = e.merv;
    d["merv_pct"]      = e.merv_pct;
    d["cold_pct"]      = e.cold_pct;
    d["sink_pct"]      = e.sink_pct;
    d["epicenter_reg"] = e.epicenter_reg;
    d["fin_raw"]       = e.fin_raw;
    return d;
}

int ScpsWorld::region_sunken(int region) const { return scps_region_sunken(sim, region); }

/* V3 — LAVIS PAR VARIANTE */
float ScpsWorld::endgame_region_intensity(int region) const {
    return sim ? scps_endgame_region_intensity(sim, region) : 0.0f;
}
/* Image L8 : une valeur d'intensité par CELLULE (la boucle 512k vit en C, via
 * scps_map_endgame_variant — jamais en GDScript). Motif layer_image. */
Ref<Image> ScpsWorld::variant_map_image() {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h);
    if (sim) scps_map_endgame_variant(sim, buf.ptrw());
    else memset(buf.ptrw(), 0, (size_t)w * h);
    return Image::create_from_data(w, h, false, Image::FORMAT_L8, buf);
}

Array ScpsWorld::province_groups(int province) {
    Array a;
    ScpsGroup g[8];
    int n = scps_province_groups(sim, province, g, 8);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["heritage"] = String::utf8(g[i].heritage);
        d["culture"]  = String::utf8(g[i].culture);
        d["religion"] = String::utf8(g[i].religion);
        d["klass"]    = String::utf8(g[i].klass);
        d["etat"]     = String::utf8(g[i].etat);
        d["loyaute"]  = String::utf8(g[i].loyaute);
        d["percent"]  = g[i].percent;
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::province_income(int province) {
    Array a;
    ScpsIncome inc[6];
    int n = scps_province_income(sim, province, inc, 6);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["source"]       = String::utf8(inc[i].source);
        d["per_day"]      = inc[i].per_day;
        d["manufactured"] = (bool)inc[i].manufactured;
        d["res_id"]       = inc[i].res_id;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::province_agitation(int province) {
    Dictionary out;
    Array causes;
    int value = 0;
    ScpsBreakdownLine bl[6];
    int n = sim ? scps_province_agitation(sim, province, &value, bl, 6) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["cause"] = String::utf8(bl[i].cause);
        d["delta"] = bl[i].delta;
        d["decay"] = bl[i].decay;
        causes.push_back(d);
    }
    out["value"] = value;
    out["causes"] = causes;
    return out;
}

Array ScpsWorld::province_buildings(int province) {
    Array a;
    ScpsProvBld b[16];
    int n = sim ? scps_province_buildings(sim, province, b, 16) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"] = String::utf8(b[i].nom);
        d["niveau"] = b[i].niveau;
        d["ouvriers"] = b[i].ouvriers;
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::province_log(int province) {
    Array a;
    ScpsLogEntry e[12];
    int n = sim ? scps_province_log(sim, province, e, 12) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["year"] = e[i].year;
        d["label"] = String::utf8(e[i].label);
        d["sign"] = e[i].sign;
        d["hover"] = String::utf8(e[i].hover);
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::province_classes(int province) {
    long lab = 0, bourg = 0, elite = 0;
    scps_province_classes(sim, province, &lab, &bourg, &elite);
    Dictionary d;
    d["laboureurs"] = (int64_t)lab;
    d["artisans"]   = (int64_t)bourg;
    d["noblesse"]   = (int64_t)elite;
    return d;
}

Dictionary ScpsWorld::province_capitale(int province) {
    ScpsCapitale c;
    scps_province_capitale(sim, province, &c);
    Dictionary d;
    d["statut"]       = String::utf8(c.statut);
    d["tier"]         = c.tier;
    d["pop"]          = (int64_t)c.pop;
    d["logement_cap"] = (int64_t)c.logement_cap;
    d["service_cap"]  = (int64_t)c.service_cap;
    d["prod_pct"]     = c.prod_pct;
    return d;
}

/* UI PROVINCE — câblage complet : 5 readers additifs (tous PURS, cf. scps_api.h). */
int64_t ScpsWorld::province_slave_count(int province) const {
    return sim ? (int64_t)scps_province_slave_count(sim, province) : 0;
}
double ScpsWorld::province_tax(int province) const {
    return sim ? scps_province_tax(sim, province) : 0.0;
}
int ScpsWorld::province_defense_pct(int province) const {
    return sim ? scps_province_defense_pct(sim, province) : 100;
}
int ScpsWorld::province_seed(int province) const {
    return sim ? scps_province_seed(sim, province) : -1;
}
Dictionary ScpsWorld::province_market(int province) {
    Dictionary d;
    ScpsMarketLine ml[3];
    const char *port = "";
    int n = sim ? scps_province_market(sim, province, ml, 3, &port) : 0;
    d["port"] = String::utf8(port ? port : "");
    Array lines;
    for (int i = 0; i < n; i++) {
        Dictionary l;
        l["name"]   = String::utf8(ml[i].name);
        l["price"]  = ml[i].price;
        l["stock"]  = ml[i].stock;
        l["marche"] = String::utf8(ml[i].marche);
        lines.push_back(l);
    }
    d["lines"] = lines;
    return d;
}

Dictionary ScpsWorld::country_demo(int country) {
    ScpsCountryDemo c;
    scps_country_demo(sim, country, &c);
    Dictionary d;
    d["pop_total"] = (int64_t)c.pop_total;
    d["n_regions"] = c.n_regions;
    Array classes;
    const char *NAMES[3] = {"Journaliers", "Bourgeois", "Nobles"};
    for (int i = 0; i < 3; i++) {
        Dictionary cl;
        cl["nom"]          = String::utf8(NAMES[i]);
        cl["pop"]          = (int64_t)c.cls_pop[i];
        cl["satisfaction"] = c.cls_sat[i];
        classes.push_back(cl);
    }
    d["classes"] = classes;
    return d;
}

Array ScpsWorld::country_stocks(int country) {
    Array a;
    ScpsStock st[40];
    int n = scps_country_stocks(sim, country, st, 40);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["name"]          = String::utf8(st[i].name);
        d["marche"]        = String::utf8(st[i].marche);
        d["stock"]         = (int64_t)st[i].stock;
        d["net_day"]       = st[i].net_day;
        d["coverage_days"] = st[i].coverage_days;
        d["market_band"]   = st[i].market_band;
        d["price"]         = st[i].price;
        d["res_id"]        = st[i].res_id;
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::country_relations(int country) {
    Array a;
    ScpsRelation rel[64];
    int n = scps_country_relations(sim, country, rel, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["name"]   = String::utf8(rel[i].name);
        d["status"] = String::utf8(rel[i].status);
        d["at_war"] = (bool)rel[i].at_war;
        d["allied"] = (bool)rel[i].allied;
        d["opinion"] = rel[i].opinion;   /* #26 : ±100, la mémoire des actes de l'AUTRE envers nous */
        d["country"] = rel[i].country;   /* §3 : index pays (cible des verbes/options diplo) */
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::diplo_options(int target) {
    Dictionary d;
    ScpsDiploOptions o;
    int ok = sim ? scps_diplo_options(sim, target, &o) : 0;
    d["valid"]                = (bool)ok;
    d["can_declare_war"]      = (bool)(ok && o.can_declare_war);
    d["can_make_peace"]       = (bool)(ok && o.can_make_peace);
    d["can_offer_alliance"]   = (bool)(ok && o.can_offer_alliance);
    d["can_offer_pact"]       = (bool)(ok && o.can_offer_pact);
    d["can_offer_migration"]  = (bool)(ok && o.can_offer_migration);
    d["can_embargo"]          = (bool)(ok && o.can_embargo);
    d["can_lift_embargo"]     = (bool)(ok && o.can_lift_embargo);
    d["would_accept_alliance"]= (bool)(ok && o.would_accept_alliance);
    d["would_accept_pact"]    = (bool)(ok && o.would_accept_pact);
    d["would_accept_migration"]= (bool)(ok && o.would_accept_migration);
    d["would_accept_peace"]   = (bool)(ok && o.would_accept_peace);
    /* W-GUERRE-3 — l'état de l'intrigue fabriquée (CB payant) contre `target`. */
    d["can_fabricate"]          = (bool)(ok && o.can_fabricate);
    d["fabricate_cost"]         = ok ? (double)o.fabricate_cost : 0.0;
    d["fabricating"]            = (bool)(ok && o.fabricating);
    d["fabricating_days_left"]  = ok ? (double)o.fabricating_days_left : 0.0;
    d["cb_ready"]                = (bool)(ok && o.cb_ready);
    d["cb_ready_years_left"]    = ok ? (double)o.cb_ready_years_left : 0.0;
    return d;
}

/* #26 — le RÉSUMÉ d'opinion : total courant + composantes (mémoire, statuts, rancune). */
Dictionary ScpsWorld::opinion_summary(int country) {
    Dictionary d;
    ScpsOpinionParts p;
    if (!sim || scps_opinion_summary(sim, country, &p) != 0) return d;
    d["total"]   = p.total;
    d["memory"]  = p.memory;
    d["ally"]    = p.ally;
    d["war"]     = p.war;
    d["vassal"]  = p.vassal;
    d["pact"]    = p.pact;
    d["embargo"] = p.embargo;
    d["rancor"]  = p.rancor;
    return d;
}

/* le JOURNAL D'ACTES joueur↔country : histoire datée (la sous-détaille de « Mémoire »). */
Array ScpsWorld::diplo_journal(int country) {
    Array a;
    if (!sim) return a;
    ScpsDiploAct acts[12];
    int n = scps_diplo_journal(sim, country, acts, 12);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["year"]  = acts[i].year;
        d["act"]   = acts[i].act;
        d["a"]     = acts[i].a_id;
        d["b"]     = acts[i].b_id;
        d["delta"] = acts[i].delta_now;
        a.append(d);
    }
    return a;
}

Dictionary ScpsWorld::country_army(int country) {
    ScpsArmy ar;
    scps_country_army(sim, country, &ar);
    Dictionary d;
    d["regiments"] = (int64_t)ar.regiments;
    d["levy"]      = ar.levy;
    d["levy_name"] = String::utf8(ar.levy_name);
    d["fleet"]     = ar.fleet;
    d["posture"]      = ar.posture;                     /* lue du MOTEUR (fin de l'état local UI) */
    d["posture_name"] = String::utf8(ar.posture_name);
    return d;
}

Dictionary ScpsWorld::country_trade(int country) {
    int routes = 0, has_centre = 0;
    double export_gold = 0.0;
    ScpsTradePartner pt[48];
    int n = scps_country_trade(sim, country, &routes, &export_gold, &has_centre, pt, 48);
    Dictionary d;
    d["routes"]      = routes;
    d["export_gold"] = export_gold;
    d["has_centre"]  = (bool)has_centre;
    Array partners;
    for (int i = 0; i < n; i++) {
        Dictionary p;
        p["name"]    = String::utf8(pt[i].name);
        p["value"]   = pt[i].value;
        p["status"]  = String::utf8(pt[i].status);
        p["at_war"]  = (bool)pt[i].at_war;
        p["embargo"] = (bool)pt[i].embargo;
        partners.push_back(p);
    }
    d["partners"] = partners;
    return d;
}

Dictionary ScpsWorld::commerce_power(int country) {
    ScpsCommerce cc;
    scps_commerce_power(sim, country, &cc);
    Dictionary d;
    d["pool"]      = cc.pool;        // §5 : le budget MENSUEL de volume échangeable
    d["remaining"] = cc.remaining;   // ce qu'il reste à acheter ce mois-ci
    d["bourgeois"] = cc.bourgeois;   // pop marchande (source ×0.04)
    d["elite"]     = cc.elite;       // élite (source ×0.01)
    d["bonus_pct"] = cc.bonus_pct;   // bonus de la chaîne commerciale (%)
    return d;
}

Array ScpsWorld::country_council(int country) {
    Array a;
    ScpsCouncilSeat seats[3];
    int n = scps_country_council(sim, country, seats, 3);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["seat"]      = String::utf8(seats[i].seat);
        d["filled"]    = (bool)seats[i].filled;
        d["councilor"] = String::utf8(seats[i].councilor);
        d["tier"]      = seats[i].tier;
        d["age"]       = seats[i].age;   /* v49 : le ministre VIEILLIT (retraite 66-73) */
        d["faction"]   = String::utf8(seats[i].faction);   /* V2a : sa faction-éthos (mot) */
        d["loyalty"]   = seats[i].loyalty;                 /* V2a : 0..100 */
        d["pay"]       = seats[i].pay;                     /* V2a : 0..2 (curseur) */
        d["mood"]      = String::utf8(seats[i].mood);      /* V2a : dévoué…AU BORD DE LA TRAHISON */
        a.push_back(d);
    }
    return a;
}

/* CANDIDATS d'un siège (pool de la génération courante — toujours pleine) :
 * nom · tier · ÂGE · coût/mois — l'embauche ÉCLAIRÉE du joueur. */
Array ScpsWorld::council_candidates(int seat) {
    Array a;
    if (!sim) return a;
    ScpsCouncilCand c[8];
    int n = scps_council_candidates(sim, seat, c, 8);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["slot"] = c[i].slot;
        d["nom"]  = String::utf8(c[i].nom);
        d["tier"] = c[i].tier;
        d["age"]  = c[i].age;
        d["cost"] = c[i].cost;
        a.push_back(d);
    }
    return a;
}

/* DÉCRETS DU JOUEUR (civics) : nom · flavor · les DEUX plateaux · réforme (irréversible) ·
 * actif · légal (condition d'entrée remplie MAINTENANT — pour griser le bouton). */
Array ScpsWorld::decrees_list(int country) {
    Array a;
    if (!sim) return a;
    ScpsDecree d[8];
    int n = scps_decrees_list(sim, country, d, 8);
    for (int i = 0; i < n; i++) {
        Dictionary dd;
        dd["id"]       = d[i].id;
        dd["nom"]      = String::utf8(d[i].nom);
        dd["flavor"]   = String::utf8(d[i].flavor);
        dd["plateaux"] = String::utf8(d[i].plateaux);
        dd["reforme"]  = (bool)d[i].reforme;
        dd["active"]   = (bool)d[i].active;
        dd["legal"]    = (bool)d[i].legal;
        a.push_back(dd);
    }
    return a;
}

Array ScpsWorld::unit_roster(int country) {
    Array a;
    if (!sim) return a;
    ScpsUnitDef u[64];
    int n = scps_unit_roster(sim, country, u, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["type"]            = u[i].type;
        d["nom"]             = String::utf8(u[i].nom);
        d["classe"]          = String::utf8(u[i].classe);
        d["arme"]            = String::utf8(u[i].arme);
        d["cout"]            = String::utf8(u[i].cout);
        d["ethos"]           = String::utf8(u[i].ethos);
        d["fort"]            = String::utf8(u[i].fort);
        d["faible"]          = String::utf8(u[i].faible);
        d["entretien_or10"]  = u[i].entretien_or10;
        d["entretien_vivre"] = u[i].entretien_vivre;
        d["recrutable"]      = (bool)u[i].recrutable;
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::building_roster(int country) {
    Array a;
    if (!sim) return a;
    ScpsEdificeDef b[64];
    int n = scps_building_roster(sim, country, b, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["type"]     = b[i].type;
        d["nom"]      = String::utf8(b[i].nom);
        d["gold"]     = b[i].gold;
        d["days"]     = b[i].days;
        d["debloque"] = (bool)b[i].debloque;
        Array costs;
        for (int k = 0; k < b[i].n_cost; k++) {
            Dictionary c;
            c["res"] = String::utf8(b[i].cost[k].res);
            c["qty"] = b[i].cost[k].qty;
            costs.push_back(c);
        }
        d["cost"] = costs;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::tech_info() {
    Dictionary d;
    ScpsTechInfo t;
    scps_tech_info(sim, &t);
    d["points"]    = t.points;
    d["crise_pct"] = t.crise_pct;
    d["presage"]   = String::utf8(t.presage);
    d["metab_pct"] = t.metab_pct;
    Array themes, funcs;
    for (int i = 0; i < 3; i++) { themes.push_back(String::utf8(t.theme[i])); funcs.push_back(String::utf8(t.function[i])); }
    d["themes"]    = themes;
    d["functions"] = funcs;
    return d;
}

/* MODTOOLS — registre des tunables (panneau dev) : lister + éditer en direct. GLOBAL. */
Array ScpsWorld::tunables() {
    Array a;
    int n = scps_tune_count();
    for (int i = 0; i < n; i++) {
        ScpsTunable t; scps_tune_at(i, &t);
        Dictionary d;
        d["nom"]        = String::utf8(t.nom ? t.nom : "");
        d["value"]      = t.value;
        d["def"]        = t.def_value;
        d["overridden"] = (bool)t.overridden;
        a.push_back(d);
    }
    return a;
}
void ScpsWorld::tune_set(const String &nom, double value) {
    scps_tune_set_val(nom.utf8().get_data(), value);
}

/* I18N — bascule la TABLE moteur (0=FR 1=EN). GLOBAL, à chaud, display-only :
 * les readouts traversants rendent la langue courante au prochain appel. */
void ScpsWorld::lang_set(int lang) { scps_lang_set(lang); }
int  ScpsWorld::lang_get() const { return scps_lang_get(); }

/* ACCÈS D'HÉRITAGE (barre de métabolisation) : par héritage, tier 0..3 + part digérée. */
Array ScpsWorld::heritage_access() {
    Array a;
    if (!sim) return a;
    ScpsHeritageAccess h[8];
    int n = scps_player_heritage_access(sim, h, 8);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"]          = String::utf8(h[i].nom);
        d["tier"]         = h[i].tier;
        d["digested_pct"] = h[i].digested_pct;
        d["native"]       = (bool)h[i].native;
        a.push_back(d);
    }
    return a;
}

/* MÉTABOLISATION POUR LA VICTOIRE (P5) — DISTINCTE de heritage_access() ci-dessus
 * (accès TECH, pop-share tier 0-3) : ce qui compte RÉELLEMENT pour un palier de
 * la Merveille (endgame_metab_count / wonder_tick). Ne PAS fusionner les deux
 * lectures côté UI — étiqueter clairement laquelle est affichée. */
Dictionary ScpsWorld::merv_metab() {
    Dictionary out;
    Array heritages;
    out["count"] = 0;
    out["required"] = 0;
    out["heritages"] = heritages;
    if (!sim) return out;
    ScpsMervHeritage h[8];
    int count = 0, required = 0;
    int n = scps_merv_metab(sim, h, 8, &count, &required);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"]          = String::utf8(h[i].nom);
        d["metabolized"]  = (bool)h[i].metabolized;
        d["voie"]         = String::utf8(h[i].voie);
        d["progress_pct"] = h[i].progress_pct;
        d["native"]       = (bool)h[i].native;
        heritages.push_back(d);
    }
    out["count"] = count;
    out["required"] = required;
    out["heritages"] = heritages;
    return out;
}

Array ScpsWorld::tech_nodes() {
    Array a;
    ScpsTechNode nd[96];
    int n = scps_tech_nodes(sim, nd, 96);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["quarter"]  = nd[i].quarter;
        d["tier"]     = nd[i].tier;
        d["state"]    = nd[i].state;
        d["faustian"] = (bool)nd[i].faustian;
        d["orphan"]   = (bool)nd[i].orphan;
        d["is_base"]  = (bool)nd[i].is_base;
        d["name"]     = String::utf8(nd[i].name);
        d["unlocks"]  = String::utf8(nd[i].unlocks);
        d["effet"]    = String::utf8(nd[i].effet);
        d["cost"]     = nd[i].cost;
        d["prereq"]   = nd[i].prereq;
        d["hover"]    = String::utf8(nd[i].hover);
        d["flavor"]   = String::utf8(nd[i].flavor);
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::country_budget(int country) {
    Array a;
    ScpsFluxLine fx[32];
    int n = scps_country_budget(sim, country, fx, 32);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["name"]   = String::utf8(fx[i].name);
        d["amount"] = fx[i].amount;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::budget_summary(int country) {
    Dictionary d;
    ScpsBudget b;
    scps_budget_summary(sim, country, &b);
    d["gold"]          = b.gold;
    d["income"]        = b.income;
    d["expense"]       = b.expense;
    d["net"]           = b.net;
    d["credit_line"]   = b.credit_line;
    d["creditor"]      = b.creditor;
    d["creditor_name"] = String::utf8(b.creditor_name);
    return d;
}

Dictionary ScpsWorld::mission_info(int country) {
    Dictionary d;
    ScpsMission m;
    scps_mission_info(sim, country, &m);
    d["active"]      = (bool)m.active;
    d["text"]        = String::utf8(m.text);
    d["reward_gold"] = m.reward_gold;
    d["reward_mat"]  = String::utf8(m.reward_mat);
    d["reward_qty"]  = m.reward_qty;
    d["issued_year"] = m.issued_year;
    d["done"]        = (bool)m.done;
    return d;
}

/* LES FACTIONS du pays (spectre d'éthos interne) : {list:[{name,part,grief,dominant}],
 * coup 0-100, corruption 0-100}. Lecture pure (sidebar). */
Dictionary ScpsWorld::country_factions(int country) {
    Dictionary d;
    Array list;
    ScpsFaction f[8];
    int coup = 0, cor = 0;
    int n = sim ? scps_country_factions(sim, country, f, 8, &coup, &cor) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary e;
        e["name"]     = String::utf8(f[i].name);
        e["part"]     = f[i].part;
        e["grief"]    = f[i].grief;
        e["dominant"] = (bool)f[i].dominant;
        list.push_back(e);
    }
    d["list"] = list;
    d["coup"] = coup;
    d["corruption"] = cor;
    return d;
}

bool ScpsWorld::player_build(int edifice, int region) {
    return sim ? scps_player_build(sim, edifice, region) != 0 : false;
}

int ScpsWorld::player_recruit(int unit) {
    return sim ? (int)scps_player_recruit(sim, unit) : 0;
}

void ScpsWorld::player_set_levy(int level) {
    if (sim) scps_player_set_levy(sim, level);
}

int ScpsWorld::player_research(int tech) {
    return sim ? scps_player_research(sim, tech) : 0;
}

Dictionary ScpsWorld::research_status() {
    Dictionary d;
    float prog = 0.0f;
    int t = sim ? scps_research_target(sim, &prog) : -1;
    d["target"] = t;
    d["progress"] = prog;
    return d;
}

/* §7 — l'âge courant (index -1 = aucun levé) + le joueur l'a-t-il engagé + son nom. */
Dictionary ScpsWorld::age_state() {
    Dictionary d;
    int engaged = 0; char name[64] = {0};
    int age = sim ? scps_age_state(sim, &engaged, name, (int)sizeof name) : -1;
    d["age"]     = age;
    d["engaged"] = engaged != 0;
    d["name"]    = String::utf8(name);
    return d;
}
bool ScpsWorld::player_age_engage() {
    return sim ? scps_player_age_engage(sim) != 0 : false;
}

/* ── ALERTES : le FIL d'évènements (poll incrémental) + les CONDITIONS en un appel ── */
Array ScpsWorld::feed_poll(int after_seq) {
    Array a;
    if (!sim) return a;
    ScpsFeedEvent ev[64];
    int n = scps_feed_poll(sim, after_seq, ev, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["seq"]    = ev[i].seq;
        d["year"]   = ev[i].year;
        d["kind"]   = ev[i].kind;
        d["region"] = ev[i].region;
        d["v"]      = ev[i].v;
        d["a_id"]   = ev[i].a_id;
        d["b_id"]   = ev[i].b_id;
        d["a"]      = String::utf8(ev[i].a_name);
        d["b"]      = String::utf8(ev[i].b_name);
        d["label"]  = String::utf8(ev[i].label);
        a.push_back(d);
    }
    return a;
}

/* MEMBRANE DE DÉCISION — la file joueur (3e voie des alertes). */
int ScpsWorld::pending_count() {
    return sim ? scps_pending_count(sim) : 0;
}
Dictionary ScpsWorld::pending_event(int slot) {
    Dictionary d;
    ScpsPendingEvent pe;
    int ok = sim ? scps_pending_event(sim, slot, &pe) : 0;
    d["valid"]     = (bool)ok;
    d["situation"] = ok ? String::utf8(pe.situation) : String();
    d["n_options"] = ok ? pe.n_options : 0;
    d["region"]    = ok ? pe.region : -1;
    d["days_left"] = ok ? pe.days_left : 0;
    d["evid"]      = ok ? pe.evid : -1;   /* clé d'illustration thématique (event_art.gd) */
    Array labels, flavors, advisors;
    for (int i = 0; i < (ok ? pe.n_options : 0); i++) {
        labels.push_back(String::utf8(pe.labels[i]));
        flavors.push_back(String::utf8(pe.flavors[i]));
        advisors.push_back(String::utf8(pe.advisors[i]));
    }
    d["labels"]   = labels;
    d["flavors"]  = flavors;
    d["advisors"] = advisors;   /* le VISAGE de chaque choix (mot de faction, "" si aucun) */
    return d;
}
bool ScpsWorld::player_event_choice(int slot, int option) {
    return sim ? scps_player_event_choice(sim, slot, option) != 0 : false;
}

/* LES ANNALES DU RÈGNE — récit SÉLECTIF, lecture seule (§ Annales). */
Array ScpsWorld::annals() {
    Array a;
    if (!sim) return a;
    ScpsAnnal an[96];
    int n = scps_annals(sim, an, 96);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["year"]   = an[i].year;
        d["kind"]   = an[i].kind;
        d["ligne"]  = String::utf8(an[i].ligne);
        d["region"] = an[i].region;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::player_alerts() {
    Dictionary d;
    ScpsPlayerAlerts al;
    scps_player_alerts(sim, &al);
    d["revolt_region"] = al.revolt_region; d["revolt_agit"] = al.revolt_agit;
    d["famine_region"] = al.famine_region; d["famine_pct"]  = al.famine_pct;
    d["siege_region"]  = al.siege_region;  d["siege_by"]    = String::utf8(al.siege_by);
    d["price_good"]    = al.price_good;    d["price_x10"]   = al.price_x10;
    d["price_name"]    = String::utf8(al.price_name);
    d["conso_good"]    = al.conso_good;    d["conso_name"]  = String::utf8(al.conso_name);
    return d;
}
/* COLONISATION (charte) : le verbe + son read de légalité + la signature de souveraineté. */
bool ScpsWorld::player_colonize(int prov) {
    return sim ? scps_player_colonize(sim, prov) != 0 : false;
}
/* §3 — le RESTE de la surface (wiring UI complet) : intérieur · conseil · commerce · guerre. */
bool ScpsWorld::player_repress(int region)               { return sim ? scps_player_repress(sim, region) != 0 : false; }
bool ScpsWorld::player_assimilate(int region, bool creuset) { return sim ? scps_player_assimilate(sim, region, creuset ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_purge(int region)                 { return sim ? scps_player_purge(sim, region) != 0 : false; }
bool ScpsWorld::player_council_hire(int seat, int slot)  { return sim ? scps_player_council_hire(sim, seat, slot) != 0 : false; }
bool ScpsWorld::player_council_dismiss(int seat)         { return sim ? scps_player_council_dismiss(sim, seat) != 0 : false; }
bool ScpsWorld::player_council_pay(int seat, float pay)  { return sim ? scps_player_council_pay(sim, seat, pay) != 0 : false; }
int  ScpsWorld::council_pair_state(int seat_a, int seat_b) { return sim ? scps_council_pair_state(sim, seat_a, seat_b) : 0; }
bool ScpsWorld::player_decree(int id, bool on)            { return sim ? scps_player_decree(sim, id, on ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_route(int ra, int rb, bool maritime) { return sim ? scps_player_route(sim, ra, rb, maritime ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_market_buy(int region, int good, int qty, int tier)  { return sim ? scps_player_market_buy(sim, region, good, (long)qty, tier) != 0 : false; }
bool ScpsWorld::player_market_sell(int region, int good, int qty, int tier) { return sim ? scps_player_market_sell(sim, region, good, (long)qty, tier) != 0 : false; }
bool ScpsWorld::player_campaign(int from_region, int target_region) { return sim ? scps_player_campaign(sim, from_region, target_region) != 0 : false; }
bool ScpsWorld::player_posture(int posture)              { return sim ? scps_player_posture(sim, posture) != 0 : false; }
bool ScpsWorld::player_refill()                          { return sim ? scps_player_refill(sim) != 0 : false; }
bool ScpsWorld::player_navy_build(int hull)              { return sim ? scps_player_navy_build(sim, hull) != 0 : false; }
bool ScpsWorld::player_disband()                         { return sim ? scps_player_disband(sim) != 0 : false; }

/* PANNEAU B — manufacture civile par région : le verbe (enfile) + la légalité (read-only). */
bool ScpsWorld::player_build_manuf(int region, int bld) {
    return sim ? scps_player_build_manuf(sim, region, bld) != 0 : false;
}
int ScpsWorld::manuf_legal(int region, int bld) {
    return sim ? scps_manuf_legal(sim, region, bld) : 0;
}
/* Nom d'affichage d'un BuildingType — miroir DISPLAY-ONLY de la table FR de
 * `building_name()` (scps_econ.c). La membrane interdit d'inclure scps_econ.h ici
 * (le binding ne voit QUE scps_api.h) ; aucun lecteur façade n'expose ce nom pour
 * un type PAS ENCORE posé (scps_region_alloc ne nomme que les puits EXISTANTS) —
 * cette petite copie statique est le compromis minimal accepté. Garder EN PHASE
 * avec l'enum BuildingType de scps_econ.h si de nouveaux types sont ajoutés. */
String ScpsWorld::manuf_name(int bld) {
    static const char *NAMES[] = {
        "Manufacture textile", "Scierie navale", "Papeterie", "Distillerie",
        "Brasserie", "Joaillerie", "Atelier d'étoffe précieuse", "Atelier de mage",
        "Forge céleste", "Atelier d'outillage", "Armurerie légère", "Poudrière",
        "Apothicaire", "Atelier de tunique", "Charbonnière", "Foreuse arcanique",
        "Alambic", "Réplicateur ligneux", "Corne divine", "Armurerie lourde",
        "Atelier d'arc", "Arquebuserie", "Poterie", "Atelier de sculpture",
    };
    static const int N_NAMES = (int)(sizeof(NAMES) / sizeof(NAMES[0]));
    if (bld < 0 || bld >= N_NAMES) return String("?");
    return String::utf8(NAMES[bld]);
}
bool ScpsWorld::can_colonize(int prov) {
    return sim ? scps_can_colonize(sim, prov) != 0 : false;
}
int ScpsWorld::colonized_total() const {
    return sim ? scps_colonized_total(sim) : 0;
}
/* v50 — le CHANTIER de colonisation du joueur (Dictionary : active/dst/days_left/total_days/cd_days/yield_pct). */
Dictionary ScpsWorld::colony_status() const {
    Dictionary d;
    int dst=-1, left=0, tot=0, cd=0, yp=0;
    int act = sim ? scps_colony_status(sim, &dst, &left, &tot, &cd, &yp) : 0;
    d["active"] = act != 0;
    d["dst"] = dst; d["days_left"] = left; d["total_days"] = tot;
    d["cd_days"] = cd; d["yield_pct"] = yp;
    return d;
}
double ScpsWorld::country_food(int c) const {
    return sim ? scps_country_food(sim, c) : 0.0;
}
int ScpsWorld::diplo_cd() const {
    return sim ? scps_diplo_cd(sim) : 0;
}
int ScpsWorld::country_capital_province(int c) const {
    return sim ? scps_country_capital_province(sim, c) : -1;
}

bool ScpsWorld::player_declare_war(int target) {
    return sim ? scps_player_declare_war(sim, target) != 0 : false;
}
bool ScpsWorld::player_make_peace(int target) {
    return sim ? scps_player_make_peace(sim, target) != 0 : false;
}
bool ScpsWorld::player_offer_alliance(int target) {
    return sim ? scps_player_offer_alliance(sim, target) != 0 : false;
}
bool ScpsWorld::player_offer_pact(int target) {
    return sim ? scps_player_offer_pact(sim, target) != 0 : false;
}
bool ScpsWorld::player_offer_migration(int target) {
    return sim ? scps_player_offer_migration(sim, target) != 0 : false;
}
bool ScpsWorld::player_embargo(int target, bool on) {
    return sim ? scps_player_embargo(sim, target, on ? 1 : 0) != 0 : false;
}
/* W-GUERRE-3 — FABRIQUER une revendication (payante) contre `target`. */
bool ScpsWorld::player_fabricate_cb(int target) {
    return sim ? scps_player_fabricate_cb(sim, target) != 0 : false;
}

/* ── ALLOCATION DE MAIN-D'ŒUVRE — la membrane traverse en Dictionary (mots + poids) ── */
Dictionary ScpsWorld::region_alloc(int region) {
    Dictionary out;
    ScpsAlloc al;
    if (sim) scps_region_alloc(sim, region, &al); else al.region = -1;
    out["region"] = al.region;
    if (al.region < 0) { out["on"] = false; out["pool"] = 0.0f; out["sinks"] = Array(); return out; }
    out["on"]   = al.on != 0;
    out["pool"] = al.pool;
    Array sinks;
    for (int i = 0; i < al.n; i++) {
        const ScpsAllocSink *k = &al.sink[i];
        Dictionary d;
        d["kind"]    = k->kind;
        d["id"]      = k->id;
        d["name"]    = String::utf8(k->name ? k->name : "");
        d["output"]  = String::utf8(k->output  ? k->output  : "");
        d["in_name"] = String::utf8(k->in_name ? k->in_name : "");
        d["alt_name"]= String::utf8(k->alt_name? k->alt_name : "");
        d["weight"]  = k->weight;
        d["pct"]     = k->pct;
        d["workers"] = k->workers;
        d["closed"]  = k->closed != 0;
        d["input"]   = k->input;
        sinks.push_back(d);
    }
    out["sinks"] = sinks;
    return out;
}
bool ScpsWorld::player_alloc_raw(int region, int resource, int weight) {
    return sim ? scps_player_alloc_raw(sim, region, resource, weight) != 0 : false;
}
bool ScpsWorld::player_alloc_bld(int region, int bld_type, int weight) {
    return sim ? scps_player_alloc_bld(sim, region, bld_type, weight) != 0 : false;
}
bool ScpsWorld::player_alloc_input(int region, int bld_type, int input) {
    return sim ? scps_player_alloc_input(sim, region, bld_type, input) != 0 : false;
}
bool ScpsWorld::player_alloc_auto(int region) {
    return sim ? scps_player_alloc_auto(sim, region) != 0 : false;
}

/* ── LOT G — RÉINCORPORATION DE POP ── */
bool ScpsWorld::player_pop_transfer(int src_region, int dst_region, int klass, int count) {
    return sim ? scps_player_pop_transfer(sim, src_region, dst_region, klass, (long)count) != 0 : false;
}
/* ── LOT J — L'APERÇU DE MANUMISSION ── */
Dictionary ScpsWorld::manumit_preview() {
    Dictionary out;
    out["souls"] = 0; out["n_groups"] = 0; out["pct_of_country"] = 0.0; out["friction_after"] = 0.0;
    if (!sim) return out;
    ScpsManumitPreview p;
    if (scps_manumit_preview(sim, &p)) {
        out["souls"] = (int64_t)p.souls;
        out["n_groups"] = p.n_groups;
        out["pct_of_country"] = (double)p.pct_of_country;
        out["friction_after"] = (double)p.friction_after;
    }
    return out;
}

/* ── ESCLAVAGE — les 3 verbes orphelins + le lecteur de marché ── */
bool ScpsWorld::player_manumit() {
    return sim ? scps_player_manumit(sim) != 0 : false;
}
bool ScpsWorld::player_slave_buy(int region, int count) {
    return sim ? scps_player_slave_buy(sim, region, (long)count) != 0 : false;
}
bool ScpsWorld::player_slave_sell(int region, int count) {
    return sim ? scps_player_slave_sell(sim, region, (long)count) != 0 : false;
}
Dictionary ScpsWorld::slave_market() {
    Dictionary out;
    Array lines;
    out["total"] = (int64_t)0; out["can_buy"] = false; out["lines"] = lines;
    if (!sim) return out;
    ScpsSlavePoolLine ln[16];
    long total = 0; int can_buy = 0;
    int n = scps_slave_market(sim, ln, 16, &total, &can_buy);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["heritage"] = String::utf8(ln[i].heritage);
        d["count"] = (int64_t)ln[i].count;
        lines.push_back(d);
    }
    out["total"] = (int64_t)total;
    out["can_buy"] = (can_buy != 0);
    out["lines"] = lines;
    return out;
}

/* ── CRÉATEUR DE CULTURE — la membrane traverse en Dictionary (mots + signes) ── */
Array ScpsWorld::heritage_list() {
    Array a;
    ScpsHeritage h[16];
    int n = scps_heritage_list(h, 16);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["id"]      = h[i].id;
        d["nom"]     = String::utf8(h[i].nom);
        d["sphere"]  = String::utf8(h[i].sphere);
        d["exemple"] = String::utf8(h[i].exemple);
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::ethos_list() {
    Array a;
    ScpsEthosDef e[16];
    int n = scps_ethos_list(e, 16);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["id"]       = e[i].id;
        d["nom"]      = String::utf8(e[i].nom);
        d["epithete"] = String::utf8(e[i].epithete);
        d["hint"]     = String::utf8(e[i].hint);
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::tradition_list() {
    Array a;
    ScpsTradition t[64];
    int n = scps_tradition_list(t, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["id"]      = t[i].id;
        d["nom"]     = String::utf8(t[i].nom);
        d["axe"]     = t[i].axe;
        d["axe_nom"] = String::utf8(t[i].axe_nom);
        d["rang"]    = t[i].rang;
        d["hover"]   = String::utf8(t[i].hover);
        a.push_back(d);
    }
    return a;
}

bool ScpsWorld::culture_validate(int t0, int t1, int t2) {
    return scps_culture_validate(t0, t1, t2) != 0;
}

Array ScpsWorld::culture_preview(int t0, int t1, int t2) {
    Array a;
    ScpsLevierLine lv[16];
    int n = scps_culture_preview(t0, t1, t2, lv, 16);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"]    = String::utf8(lv[i].nom);
        d["signe"]  = lv[i].signe;
        d["value"]  = lv[i].value;   // magnitude du delta (chiffre, plus la flèche seule)
        d["is_pct"] = lv[i].is_pct;  // 1 = relatif (+15 %) · 0 = absolu (+1.5)
        a.push_back(d);
    }
    return a;
}

String ScpsWorld::culture_name(int heritage, int seed) {
    return String::utf8(scps_culture_name(heritage, (uint32_t)seed));
}

bool ScpsWorld::set_empire_culture(int slot, int heritage, int ethos, int t0, int t1, int t2) {
    return scps_set_empire_culture(slot, heritage, ethos, t0, t1, t2) != 0;
}
bool ScpsWorld::set_player_culture(int heritage, int ethos, int t0, int t1, int t2) {
    return scps_set_player_culture(heritage, ethos, t0, t1, t2) != 0;
}

void ScpsWorld::clear_player_culture() {
    scps_clear_player_culture();
}

Dictionary ScpsWorld::worldparams_default(int seed) {
    ScpsWorldParams p;
    scps_worldparams_default((uint32_t)seed, &p);
    Dictionary d;
    d["n_empires"]     = p.n_empires;
    d["n_city_states"] = p.n_city_states;
    d["n_continents"]  = p.n_continents;
    d["world_age"]     = p.world_age;
    d["land_amount"]   = p.land_amount;
    d["mountains"]     = p.mountains;
    d["erosion"]       = p.erosion;
    d["temperature"]   = p.temperature;
    d["humidity"]      = p.humidity;
    return d;
}

void ScpsWorld::worldgen_set(Dictionary p) {
    ScpsWorldParams w;
    w.n_empires     = (int)p.get("n_empires", 6);
    w.n_city_states = (int)p.get("n_city_states", 12);
    w.n_continents  = (int)p.get("n_continents", 6);
    w.world_age     = (float)(double)p.get("world_age", 0.7);
    w.land_amount   = (float)(double)p.get("land_amount", 0.5);
    w.mountains     = (float)(double)p.get("mountains", 0.5);
    w.erosion       = (float)(double)p.get("erosion", 0.5);
    w.temperature   = (float)(double)p.get("temperature", 0.5);
    w.humidity      = (float)(double)p.get("humidity", 0.5);
    scps_worldgen_set(&w);
}

void ScpsWorld::worldgen_clear() {
    scps_worldgen_clear();
}

/* ── RELIGION (P5) — passe-plats vers la façade ── */
Array ScpsWorld::religion_pole_list() {
    Array a;
    ScpsReligPole p[32];
    int n = scps_religion_pole_list(p, 32);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["id"]      = p[i].id;
        d["nom"]     = String::utf8(p[i].nom);
        d["axe"]     = p[i].axe;
        d["axe_nom"] = String::utf8(p[i].axe_nom);
        d["tip"]     = String::utf8(p[i].tip);
        a.push_back(d);
    }
    return a;
}
Array ScpsWorld::credo_list() {
    Array a;
    ScpsCredoDef c[8];
    int n = scps_credo_list(c, 8);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["id"]  = c[i].id;
        d["nom"] = String::utf8(c[i].nom);
        a.push_back(d);
    }
    return a;
}
bool ScpsWorld::religion_picks_valid(int p0, int p1, int p2) {
    return scps_religion_picks_valid(p0, p1, p2) != 0;
}
int ScpsWorld::religion_found(int cid, int credo, int t0, int t1, int t2) {
    return sim ? scps_religion_found(sim, cid, credo, t0, t1, t2) : -1;
}
int ScpsWorld::religion_eligible(int cid) {
    return sim ? scps_religion_eligible(sim, cid) : 0;
}
Dictionary ScpsWorld::religion_schism(int cid, int slot_a, int pole_a, int slot_b, int pole_b, int new_credo) {
    Dictionary d;
    int flipped = 0;
    int child = sim ? scps_religion_schism(sim, cid, slot_a, pole_a, slot_b, pole_b, new_credo, &flipped) : -1;
    d["child"]   = child;
    d["flipped"] = flipped;
    return d;
}
int ScpsWorld::religion_of_country(int cid)  { return sim ? scps_religion_of_country(sim, cid) : -1; }
int ScpsWorld::religion_of_region(int region){ return sim ? scps_religion_of_region(sim, region) : -1; }
int ScpsWorld::religion_recruit_scholar(int cid, int region) {
    return sim ? scps_religion_recruit_scholar(sim, cid, region) : -1;
}
int ScpsWorld::religion_scholar_role(int cid){ return sim ? scps_religion_scholar_role(sim, cid) : -1; }
String ScpsWorld::religion_name(int cid) {
    return sim ? String::utf8(scps_religion_name(sim, cid)) : String();
}
int ScpsWorld::religion_founding_ready(int cid) {
    return sim ? scps_religion_founding_ready(sim, cid) : 0;
}
int ScpsWorld::religion_cap()       { return sim ? scps_religion_cap(sim) : 1; }
int ScpsWorld::religion_can_found() { return sim ? scps_religion_can_found(sim) : 1; }

bool ScpsWorld::save_game(int slot) {
    return sim ? scps_sim_save(sim, slot) != 0 : false;
}
int ScpsWorld::load_game(int slot) {
    return sim ? scps_sim_load(sim, slot) : 1;
}
Array ScpsWorld::save_slots() {
    Array a;
    ScpsSaveSlot sl[3];
    scps_save_slots(sl, 3);
    for (int i = 0; i < 3; i++) {
        Dictionary d;
        d["slot"] = i + 1;
        d["used"] = (bool)sl[i].used;
        d["year"] = sl[i].year;
        d["line"] = String::utf8(sl[i].line);
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::river_points() {
    Array a;
    if (!sim) return a;
    static ScpsRiverPt pts[6000];
    int n = scps_river_points(sim, pts, 6000);
    for (int i = 0; i < n; i++)
        a.push_back(Vector3(pts[i].x, pts[i].y, pts[i].ang));   /* x · y · angle */
    return a;
}

Array ScpsWorld::river_paths() {
    Array a;
    if (!sim) return a;
    int nr = scps_river_count(sim);
    static const int MAXPT = 4096;
    static ScpsRiverPt pts[MAXPT];
    for (int i = 0; i < nr; i++) {
        float flow = 0.0f;
        int n = scps_river_path(sim, i, pts, MAXPT, &flow);
        if (n < 2) continue;
        PackedVector2Array pv;
        pv.resize(n);
        for (int k = 0; k < n; k++) pv.set(k, Vector2(pts[k].x, pts[k].y));
        Dictionary d;
        d["points"] = pv;
        d["flow"]   = flow;
        a.push_back(d);
    }
    return a;
}

PackedVector2Array ScpsWorld::border_segments(int level) {
    PackedVector2Array a;
    if (!sim) return a;
    static const int MAXSEG = 50000;
    static ScpsSeg seg[MAXSEG];
    int n = scps_border_segments(sim, level, seg, MAXSEG);
    a.resize(n * 2);                                            /* 2 points par segment */
    for (int i = 0; i < n; i++) {
        a.set(i * 2,     Vector2(seg[i].x0, seg[i].y0));
        a.set(i * 2 + 1, Vector2(seg[i].x1, seg[i].y1));
    }
    return a;
}

/* frontières TAGGÉES par owner (pays) — { pts: PackedVector2Array (2/seg), owner: PackedInt32Array }
 * → l'overlay groupe par owner et colore l'outline par empire/entité. */
Dictionary ScpsWorld::border_segments_col(int level) {
    Dictionary d;
    PackedVector2Array pts;
    PackedVector2Array nrm;
    PackedInt32Array owners;
    PackedInt32Array others;
    if (sim) {
        static const int MAXSEG = 50000;
        static ScpsSegC seg[MAXSEG];
        int n = scps_border_segments_col(sim, level, seg, MAXSEG);
        pts.resize(n * 2);
        nrm.resize(n);
        owners.resize(n);
        others.resize(n);
        for (int i = 0; i < n; i++) {
            pts.set(i * 2,     Vector2(seg[i].x0, seg[i].y0));
            pts.set(i * 2 + 1, Vector2(seg[i].x1, seg[i].y1));
            nrm.set(i, Vector2(seg[i].nx, seg[i].ny));
            owners.set(i, seg[i].owner);
            others.set(i, seg[i].other);
        }
    }
    d["pts"] = pts;
    d["nrm"] = nrm;          /* normale vers l'extérieur (par segment) — dégradé int.→ext. */
    d["owner"] = owners;
    d["other"] = others;     /* >=0 autre empire · -1 terre libre · -2 mer */
    return d;
}

int ScpsWorld::country_ethos(int c) const {
    return sim ? scps_country_ethos(sim, c) : -1;
}

int ScpsWorld::country_heritage(int c) const {
    return sim ? scps_country_heritage(sim, c) : -1;
}

int ScpsWorld::country_capital_region(int c) const {
    return sim ? scps_country_capital_region(sim, c) : -1;
}

Dictionary ScpsWorld::region_border_segments(int region) {
    Dictionary d;
    PackedVector2Array pts, nrm;
    if (sim) {
        static const int MAXSEG = 20000;
        static ScpsSegC seg[MAXSEG];
        int n = scps_region_border_segments(sim, region, seg, MAXSEG);
        pts.resize(n * 2); nrm.resize(n);
        for (int i = 0; i < n; i++) {
            pts.set(i * 2,     Vector2(seg[i].x0, seg[i].y0));
            pts.set(i * 2 + 1, Vector2(seg[i].x1, seg[i].y1));
            nrm.set(i, Vector2(seg[i].nx, seg[i].ny));
        }
    }
    d["pts"] = pts; d["nrm"] = nrm;
    return d;
}

/* contour d'une PROVINCE (le grain de panneau) — la SURBRILLANCE de sélection. */
Dictionary ScpsWorld::province_border_segments(int prov) {
    Dictionary d;
    PackedVector2Array pts, nrm;
    if (sim) {
        static const int MAXSEG = 20000;
        static ScpsSegC seg[MAXSEG];
        int n = scps_province_border_segments(sim, prov, seg, MAXSEG);
        pts.resize(n * 2); nrm.resize(n);
        for (int i = 0; i < n; i++) {
            pts.set(i * 2,     Vector2(seg[i].x0, seg[i].y0));
            pts.set(i * 2 + 1, Vector2(seg[i].x1, seg[i].y1));
            nrm.set(i, Vector2(seg[i].nx, seg[i].ny));
        }
    }
    d["pts"] = pts; d["nrm"] = nrm;
    return d;
}

Array ScpsWorld::road_paths() {
    Array a;
    if (!sim) return a;
    int np = scps_roads_build(sim);
    static const int MAXPT = 1400;
    static ScpsRoadPt pts[MAXPT];
    for (int i = 0; i < np; i++) {
        int level = 0;
        int n = scps_road_path(sim, i, pts, MAXPT, &level);
        if (n < 2) continue;
        PackedVector2Array pv;
        pv.resize(n);
        for (int k = 0; k < n; k++) pv.set(k, Vector2(pts[k].x, pts[k].y));
        Dictionary d;
        d["points"] = pv;
        d["level"]  = level;
        a.push_back(d);
    }
    return a;
}
