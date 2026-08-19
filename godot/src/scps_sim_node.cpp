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
#include <cstdint>

using namespace godot;

/* Grain de lavis politique, stable et a basse frequence. Les quatre valeurs
 * aux coins d'une maille de 24 cellules sont interpolees : le pigment varie
 * doucement, sans bruit pixel ni trame visible. */
static float political_wash_hash(int x, int y, int owner) {
    uint32_t v = (uint32_t)x * 0x9e3779b9u;
    v ^= (uint32_t)y * 0x85ebca6bu;
    v ^= (uint32_t)(owner + 1) * 0xc2b2ae35u;
    v ^= v >> 16;
    v *= 0x7feb352du;
    v ^= v >> 15;
    return (float)(v & 0xffffu) / 65535.0f;
}

static float political_wash_grain(int x, int y, int owner) {
    const int step = 24;
    int gx = x / step, gy = y / step;
    float fx = (float)(x % step) / (float)step;
    float fy = (float)(y % step) / (float)step;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = political_wash_hash(gx,     gy,     owner);
    float b = political_wash_hash(gx + 1, gy,     owner);
    float c = political_wash_hash(gx,     gy + 1, owner);
    float d = political_wash_hash(gx + 1, gy + 1, owner);
    float ab = a + (b - a) * fx;
    float cd = c + (d - c) * fx;
    return 0.95f + 0.10f * (ab + (cd - ab) * fy); /* pigment +/-5 % */
}

void ScpsWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "seed"),        &ScpsWorld::generate);
    ClassDB::bind_method(D_METHOD("advance_days", "days"),    &ScpsWorld::advance_days);
    ClassDB::bind_method(D_METHOD("map_w"),                   &ScpsWorld::map_w);
    ClassDB::bind_method(D_METHOD("map_h"),                   &ScpsWorld::map_h);
    ClassDB::bind_method(D_METHOD("map_image", "mode", "selected_prov"), &ScpsWorld::map_image, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("layer_image", "layer"),    &ScpsWorld::layer_image);
    ClassDB::bind_method(D_METHOD("political_image", "pal"),  &ScpsWorld::political_image);
    ClassDB::bind_method(D_METHOD("market_catchment_image", "pal"), &ScpsWorld::market_catchment_image);
    ClassDB::bind_method(D_METHOD("religion_image", "pal"), &ScpsWorld::religion_image);
    ClassDB::bind_method(D_METHOD("culture_image", "pal"), &ScpsWorld::culture_image);
    ClassDB::bind_method(D_METHOD("fog_image"),                &ScpsWorld::fog_image);
    ClassDB::bind_method(D_METHOD("fog_region_mask"),          &ScpsWorld::fog_region_mask);
    ClassDB::bind_method(D_METHOD("year"),                    &ScpsWorld::year);
    ClassDB::bind_method(D_METHOD("player"),                  &ScpsWorld::player);
    ClassDB::bind_method(D_METHOD("set_observer", "on"),      &ScpsWorld::set_observer);
    ClassDB::bind_method(D_METHOD("is_observer"),             &ScpsWorld::is_observer);
    ClassDB::bind_method(D_METHOD("country_count"),           &ScpsWorld::country_count);
    ClassDB::bind_method(D_METHOD("country_province_count", "country"), &ScpsWorld::country_province_count);
    ClassDB::bind_method(D_METHOD("region_count"),            &ScpsWorld::region_count);
    ClassDB::bind_method(D_METHOD("province_count"),          &ScpsWorld::province_count);
    ClassDB::bind_method(D_METHOD("world_pop"),               &ScpsWorld::world_pop);
    ClassDB::bind_method(D_METHOD("country_pop", "country"),  &ScpsWorld::country_pop);
    ClassDB::bind_method(D_METHOD("country_gold", "country"), &ScpsWorld::country_gold);
    ClassDB::bind_method(D_METHOD("country_reserve", "country"),    &ScpsWorld::country_reserve);
    ClassDB::bind_method(D_METHOD("country_mint_month", "country"), &ScpsWorld::country_mint_month);
    ClassDB::bind_method(D_METHOD("country_mint_detail", "country"), &ScpsWorld::country_mint_detail);
    ClassDB::bind_method(D_METHOD("country_role", "country"), &ScpsWorld::country_role);
    ClassDB::bind_method(D_METHOD("region_owner", "region"),     &ScpsWorld::region_owner);
    ClassDB::bind_method(D_METHOD("region_pop", "region"),       &ScpsWorld::region_pop);
    ClassDB::bind_method(D_METHOD("region_colonized", "region"), &ScpsWorld::region_colonized);
    ClassDB::bind_method(D_METHOD("region_centroid", "region"),  &ScpsWorld::region_centroid);
    ClassDB::bind_method(D_METHOD("region_seat", "region"),      &ScpsWorld::region_seat);
    ClassDB::bind_method(D_METHOD("region_city_name", "region"), &ScpsWorld::region_city_name);

    ClassDB::bind_method(D_METHOD("province_at", "x", "y"),          &ScpsWorld::province_at);
    ClassDB::bind_method(D_METHOD("province_region", "province"),    &ScpsWorld::province_region);
    ClassDB::bind_method(D_METHOD("province_info", "province"),      &ScpsWorld::province_info);
    ClassDB::bind_method(D_METHOD("country_info", "country"),        &ScpsWorld::country_info);
    ClassDB::bind_method(D_METHOD("country_research_income", "country"), &ScpsWorld::country_research_income);
    ClassDB::bind_method(D_METHOD("army_info", "country"),           &ScpsWorld::army_info);
    ClassDB::bind_method(D_METHOD("corps_ids", "country"),            &ScpsWorld::corps_ids);
    ClassDB::bind_method(D_METHOD("corps_info", "id"),                &ScpsWorld::corps_info);
    ClassDB::bind_method(D_METHOD("corps_move_preview", "id", "target_region"), &ScpsWorld::corps_move_preview);
    ClassDB::bind_method(D_METHOD("corps_refill_preview", "id"), &ScpsWorld::corps_refill_preview);
    ClassDB::bind_method(D_METHOD("region_tier", "region"),          &ScpsWorld::region_tier);
    ClassDB::bind_method(D_METHOD("region_settle_group", "region"),  &ScpsWorld::region_settle_group);
    ClassDB::bind_method(D_METHOD("region_war_state", "region"),     &ScpsWorld::region_war_state);
    ClassDB::bind_method(D_METHOD("battle_info", "region"),          &ScpsWorld::battle_info);
    ClassDB::bind_method(D_METHOD("endgame_info"),                   &ScpsWorld::endgame_info);
    ClassDB::bind_method(D_METHOD("region_sunken", "region"),        &ScpsWorld::region_sunken);
    ClassDB::bind_method(D_METHOD("endgame_region_intensity", "region"), &ScpsWorld::endgame_region_intensity);
    ClassDB::bind_method(D_METHOD("variant_map_image"),              &ScpsWorld::variant_map_image);
    ClassDB::bind_method(D_METHOD("province_groups", "province"),    &ScpsWorld::province_groups);
    ClassDB::bind_method(D_METHOD("province_culture_context", "province"), &ScpsWorld::province_culture_context);
    ClassDB::bind_method(D_METHOD("province_income", "province"),    &ScpsWorld::province_income);
    ClassDB::bind_method(D_METHOD("province_agitation", "province"), &ScpsWorld::province_agitation);
    ClassDB::bind_method(D_METHOD("province_developpement", "province"), &ScpsWorld::province_developpement);
    ClassDB::bind_method(D_METHOD("province_capadmin", "province"), &ScpsWorld::province_capadmin);
    ClassDB::bind_method(D_METHOD("province_services_why", "province"), &ScpsWorld::province_services_why);
    ClassDB::bind_method(D_METHOD("province_buildings", "province"), &ScpsWorld::province_buildings);
    ClassDB::bind_method(D_METHOD("province_edifices", "province"),  &ScpsWorld::province_edifices);
    ClassDB::bind_method(D_METHOD("province_friche", "province"),    &ScpsWorld::province_friche);
    ClassDB::bind_method(D_METHOD("day_of_year"),                    &ScpsWorld::day_of_year);
    ClassDB::bind_method(D_METHOD("country_known", "country"),       &ScpsWorld::country_known);
    ClassDB::bind_method(D_METHOD("province_log", "province"),       &ScpsWorld::province_log);
    ClassDB::bind_method(D_METHOD("province_classes", "province"),   &ScpsWorld::province_classes);
    ClassDB::bind_method(D_METHOD("province_class_sat", "province"), &ScpsWorld::province_class_sat);
    ClassDB::bind_method(D_METHOD("province_capitale", "province"),  &ScpsWorld::province_capitale);
    ClassDB::bind_method(D_METHOD("province_slave_count", "province"),   &ScpsWorld::province_slave_count);
    ClassDB::bind_method(D_METHOD("province_tax", "province"),           &ScpsWorld::province_tax);
    ClassDB::bind_method(D_METHOD("province_defense_pct", "province"),   &ScpsWorld::province_defense_pct);
    ClassDB::bind_method(D_METHOD("province_seed", "province"),          &ScpsWorld::province_seed);
    ClassDB::bind_method(D_METHOD("province_market", "province"),        &ScpsWorld::province_market);
    ClassDB::bind_method(D_METHOD("market_catchment", "province"),       &ScpsWorld::market_catchment);
    ClassDB::bind_method(D_METHOD("province_centroid", "province"),      &ScpsWorld::province_centroid);
    ClassDB::bind_method(D_METHOD("province_raws", "province"),          &ScpsWorld::province_raws);
    ClassDB::bind_method(D_METHOD("market_hover", "province"),           &ScpsWorld::market_hover);
    ClassDB::bind_method(D_METHOD("map_mode_label", "i"),                 &ScpsWorld::map_mode_label);
    ClassDB::bind_method(D_METHOD("province_religion", "province"),      &ScpsWorld::province_religion);
    ClassDB::bind_method(D_METHOD("province_religion_hover", "province"), &ScpsWorld::province_religion_hover);
    ClassDB::bind_method(D_METHOD("province_culture_id", "province"),    &ScpsWorld::province_culture_id);
    ClassDB::bind_method(D_METHOD("province_culture_hover", "province"), &ScpsWorld::province_culture_hover);
    ClassDB::bind_method(D_METHOD("country_demo", "country"),        &ScpsWorld::country_demo);
    ClassDB::bind_method(D_METHOD("country_class_policy_sat", "country", "classe"), &ScpsWorld::country_class_policy_sat);
    ClassDB::bind_method(D_METHOD("country_stocks", "country"),      &ScpsWorld::country_stocks);
    ClassDB::bind_method(D_METHOD("stock_regions", "country", "good"), &ScpsWorld::stock_regions);
    ClassDB::bind_method(D_METHOD("market_quote", "country", "good", "qty"), &ScpsWorld::market_quote);
    ClassDB::bind_method(D_METHOD("country_relations", "country"),   &ScpsWorld::country_relations);
    ClassDB::bind_method(D_METHOD("diplo_options", "target"),        &ScpsWorld::diplo_options);
    ClassDB::bind_method(D_METHOD("diplo_action_legal", "target", "action"), &ScpsWorld::diplo_action_legal);
    ClassDB::bind_method(D_METHOD("diplo_context", "target"), &ScpsWorld::diplo_context);
    ClassDB::bind_method(D_METHOD("peace_terms", "target"), &ScpsWorld::peace_terms);
    ClassDB::bind_method(D_METHOD("opinion_summary", "country"),     &ScpsWorld::opinion_summary);
    ClassDB::bind_method(D_METHOD("diplo_journal", "country"),       &ScpsWorld::diplo_journal);
    ClassDB::bind_method(D_METHOD("country_army", "country"),        &ScpsWorld::country_army);
    ClassDB::bind_method(D_METHOD("country_trade", "country"),       &ScpsWorld::country_trade);
    ClassDB::bind_method(D_METHOD("commerce_power", "country"),      &ScpsWorld::commerce_power);
    ClassDB::bind_method(D_METHOD("country_council", "country"),     &ScpsWorld::country_council);
    ClassDB::bind_method(D_METHOD("decrees_list", "country"),        &ScpsWorld::decrees_list);
    ClassDB::bind_method(D_METHOD("country_revenue_year", "country"), &ScpsWorld::country_revenue_year);
    ClassDB::bind_method(D_METHOD("tax_class_month", "cls"),          &ScpsWorld::tax_class_month);
    ClassDB::bind_method(D_METHOD("world_ipm"),                       &ScpsWorld::world_ipm);
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
    ClassDB::bind_method(D_METHOD("budget_controls", "country"),     &ScpsWorld::budget_controls);
    ClassDB::bind_method(D_METHOD("mission_info", "country"),        &ScpsWorld::mission_info);
    ClassDB::bind_method(D_METHOD("country_factions", "country"),    &ScpsWorld::country_factions);
    ClassDB::bind_method(D_METHOD("player_build", "edifice", "province"), &ScpsWorld::player_build, DEFVAL(-1));
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
    /* RE-KEY PROVINCE (2026-07-14) : repress/assimilate/purge/action_preview prennent
     * un PID direct ("prov") — plus d'indirection région. */
    ClassDB::bind_method(D_METHOD("player_repress", "prov"),          &ScpsWorld::player_repress);
    ClassDB::bind_method(D_METHOD("player_assimilate", "prov", "creuset"), &ScpsWorld::player_assimilate);
    ClassDB::bind_method(D_METHOD("player_purge", "prov"),            &ScpsWorld::player_purge);
    ClassDB::bind_method(D_METHOD("action_preview", "prov", "verb"),  &ScpsWorld::action_preview);
    ClassDB::bind_method(D_METHOD("player_council_hire", "seat", "slot"), &ScpsWorld::player_council_hire);
    ClassDB::bind_method(D_METHOD("player_council_dismiss", "seat"),    &ScpsWorld::player_council_dismiss);
    ClassDB::bind_method(D_METHOD("player_council_pay", "seat", "pay"), &ScpsWorld::player_council_pay);
    ClassDB::bind_method(D_METHOD("player_budget_policy", "family", "index", "mult"), &ScpsWorld::player_budget_policy);
    ClassDB::bind_method(D_METHOD("council_pair_state", "seat_a", "seat_b"), &ScpsWorld::council_pair_state);
    ClassDB::bind_method(D_METHOD("council_candidates", "seat"),        &ScpsWorld::council_candidates);
    ClassDB::bind_method(D_METHOD("player_decree", "id", "on"),         &ScpsWorld::player_decree);
    ClassDB::bind_method(D_METHOD("player_route", "ra", "rb", "maritime"), &ScpsWorld::player_route);
    ClassDB::bind_method(D_METHOD("player_market_buy", "province", "good", "qty", "tier"),  &ScpsWorld::player_market_buy);
    ClassDB::bind_method(D_METHOD("player_market_sell", "province", "good", "qty", "tier"), &ScpsWorld::player_market_sell);
    ClassDB::bind_method(D_METHOD("player_campaign", "from_region", "target_region"), &ScpsWorld::player_campaign);
    ClassDB::bind_method(D_METHOD("player_move_army", "target_region"), &ScpsWorld::player_move_army);
    ClassDB::bind_method(D_METHOD("player_refill"),                     &ScpsWorld::player_refill);
    ClassDB::bind_method(D_METHOD("player_navy_build", "hull"),         &ScpsWorld::player_navy_build);
    ClassDB::bind_method(D_METHOD("player_disband"),                    &ScpsWorld::player_disband);
    ClassDB::bind_method(D_METHOD("player_raise_corps", "packets", "target_region"), &ScpsWorld::player_raise_corps);
    ClassDB::bind_method(D_METHOD("player_split_corps", "id", "packets"), &ScpsWorld::player_split_corps);
    ClassDB::bind_method(D_METHOD("player_split_comp", "id", "inf_p", "arch_p", "cav_p", "mages_p"), &ScpsWorld::player_split_comp);
    ClassDB::bind_method(D_METHOD("player_merge_corps", "dst_id", "src_id"), &ScpsWorld::player_merge_corps);
    ClassDB::bind_method(D_METHOD("player_move_corps", "id", "target_region"), &ScpsWorld::player_move_corps);
    ClassDB::bind_method(D_METHOD("player_refill_corps", "id"), &ScpsWorld::player_refill_corps);
    ClassDB::bind_method(D_METHOD("player_disband_corps", "id"), &ScpsWorld::player_disband_corps);
    ClassDB::bind_method(D_METHOD("player_raid_coast", "prov"),         &ScpsWorld::player_raid_coast);
    ClassDB::bind_method(D_METHOD("can_raid_coast", "prov"),            &ScpsWorld::can_raid_coast);
    ClassDB::bind_method(D_METHOD("player_build_manuf", "province", "bld"), &ScpsWorld::player_build_manuf);
    ClassDB::bind_method(D_METHOD("player_manuf_level", "province", "bld", "dir"), &ScpsWorld::player_manuf_level);
    ClassDB::bind_method(D_METHOD("player_demolish_edifice", "province", "edifice"), &ScpsWorld::player_demolish_edifice);
    ClassDB::bind_method(D_METHOD("manuf_legal", "province", "bld"),        &ScpsWorld::manuf_legal);
    ClassDB::bind_method(D_METHOD("manuf_cost"),                          &ScpsWorld::manuf_cost);
    ClassDB::bind_method(D_METHOD("manuf_recipe", "bld"),                 &ScpsWorld::manuf_recipe);
    ClassDB::bind_method(D_METHOD("manuf_upkeep_month", "province", "bld"), &ScpsWorld::manuf_upkeep_month);
    ClassDB::bind_method(D_METHOD("manuf_name", "bld"),                   &ScpsWorld::manuf_name);
    ClassDB::bind_method(D_METHOD("edifice_name", "edifice"),             &ScpsWorld::edifice_name);
    ClassDB::bind_method(D_METHOD("edifice_succ", "edifice"),             &ScpsWorld::edifice_succ);
    ClassDB::bind_method(D_METHOD("edifice_upkeep_month", "edifice"),     &ScpsWorld::edifice_upkeep_month);
    ClassDB::bind_method(D_METHOD("build_legal", "province", "edifice"),    &ScpsWorld::build_legal);
    ClassDB::bind_method(D_METHOD("renover_state", "province"),             &ScpsWorld::renover_state);
    ClassDB::bind_method(D_METHOD("player_renover", "province"),            &ScpsWorld::player_renover);
    ClassDB::bind_method(D_METHOD("colonized_total"),               &ScpsWorld::colonized_total);
    ClassDB::bind_method(D_METHOD("colony_status"),                 &ScpsWorld::colony_status);
    ClassDB::bind_method(D_METHOD("country_food", "c"),             &ScpsWorld::country_food);
    ClassDB::bind_method(D_METHOD("diplo_cd"),                      &ScpsWorld::diplo_cd);
    ClassDB::bind_method(D_METHOD("country_capital_province", "c"), &ScpsWorld::country_capital_province);
    ClassDB::bind_method(D_METHOD("player_declare_war", "target"),    &ScpsWorld::player_declare_war);
    ClassDB::bind_method(D_METHOD("player_make_peace", "target"),     &ScpsWorld::player_make_peace);
    ClassDB::bind_method(D_METHOD("player_peace_offer", "target", "regions", "gold_score", "flags"), &ScpsWorld::player_peace_offer);
    ClassDB::bind_method(D_METHOD("player_offer_alliance", "target"), &ScpsWorld::player_offer_alliance);
    ClassDB::bind_method(D_METHOD("player_offer_pact", "target"),     &ScpsWorld::player_offer_pact);
    ClassDB::bind_method(D_METHOD("player_offer_migration", "target"),&ScpsWorld::player_offer_migration);
    ClassDB::bind_method(D_METHOD("player_embargo", "target", "on"),  &ScpsWorld::player_embargo);
    ClassDB::bind_method(D_METHOD("player_fabricate_cb", "target"),   &ScpsWorld::player_fabricate_cb);

    /* ALLOCATION DE MAIN-D'ŒUVRE (onglet province) — RE-KEY : pid direct, jamais une région */
    ClassDB::bind_method(D_METHOD("province_alloc", "province"),              &ScpsWorld::province_alloc);
    ClassDB::bind_method(D_METHOD("player_alloc_raw", "province", "resource", "weight"), &ScpsWorld::player_alloc_raw);
    ClassDB::bind_method(D_METHOD("player_alloc_bld", "province", "bld_type", "weight"), &ScpsWorld::player_alloc_bld);
    ClassDB::bind_method(D_METHOD("player_alloc_input", "province", "bld_type", "input"), &ScpsWorld::player_alloc_input);
    ClassDB::bind_method(D_METHOD("player_alloc_auto", "province"),         &ScpsWorld::player_alloc_auto);
    /* RE-KEY PROVINCE : src_prov/dst_prov et prov sont des PID directs (plus de région). */
    ClassDB::bind_method(D_METHOD("player_pop_transfer", "src_prov", "dst_prov", "klass", "count"), &ScpsWorld::player_pop_transfer);
    ClassDB::bind_method(D_METHOD("manumit_preview"),                     &ScpsWorld::manumit_preview);
    ClassDB::bind_method(D_METHOD("country_shortages", "country"),        &ScpsWorld::country_shortages);
    ClassDB::bind_method(D_METHOD("player_manumit"),                      &ScpsWorld::player_manumit);
    ClassDB::bind_method(D_METHOD("player_slave_buy", "prov", "count"), &ScpsWorld::player_slave_buy);
    ClassDB::bind_method(D_METHOD("player_slave_sell", "prov", "count"), &ScpsWorld::player_slave_sell);
    ClassDB::bind_method(D_METHOD("slave_market"),                        &ScpsWorld::slave_market);

    /* UI-MONNAIE (2026-07-16) — dette/emprunt/banqueroute/fiscalité/prix, voir scps_sim_node.h. */
    ClassDB::bind_method(D_METHOD("country_debt", "country"),             &ScpsWorld::country_debt);
    ClassDB::bind_method(D_METHOD("country_fiscal_orders", "country"),    &ScpsWorld::country_fiscal_orders);
    ClassDB::bind_method(D_METHOD("country_loan_capacity", "country"),    &ScpsWorld::country_loan_capacity);
    ClassDB::bind_method(D_METHOD("country_loan_quote", "debtor", "lender"), &ScpsWorld::country_loan_quote);
    ClassDB::bind_method(D_METHOD("player_borrow_class", "cls", "amount"), &ScpsWorld::player_borrow_class);
    ClassDB::bind_method(D_METHOD("country_loan_request_target", "country"), &ScpsWorld::country_loan_request_target);
    ClassDB::bind_method(D_METHOD("country_loan_status", "country"),      &ScpsWorld::country_loan_status);
    ClassDB::bind_method(D_METHOD("player_request_loan", "target", "amount"), &ScpsWorld::player_request_loan);
    ClassDB::bind_method(D_METHOD("player_bankruptcy"),                   &ScpsWorld::player_bankruptcy);
    ClassDB::bind_method(D_METHOD("player_repay", "amount"),              &ScpsWorld::player_repay);
    ClassDB::bind_method(D_METHOD("country_price_level", "country"),      &ScpsWorld::country_price_level);
    ClassDB::bind_method(D_METHOD("world_price_index"),                   &ScpsWorld::world_price_index);
    ClassDB::bind_method(D_METHOD("country_debase_frac", "country"),      &ScpsWorld::country_debase_frac);
    ClassDB::bind_method(D_METHOD("country_bankruptcy_scar", "country"),  &ScpsWorld::country_bankruptcy_scar);
    ClassDB::bind_method(D_METHOD("province_res_price", "province", "res_id"), &ScpsWorld::province_res_price);

    /* CRÉATEUR DE CULTURE */
    ClassDB::bind_method(D_METHOD("heritage_list"),                  &ScpsWorld::heritage_list);
    ClassDB::bind_method(D_METHOD("ethos_list"),                     &ScpsWorld::ethos_list);
    ClassDB::bind_method(D_METHOD("tradition_list"),                 &ScpsWorld::tradition_list);
    ClassDB::bind_method(D_METHOD("culture_validate", "t0", "t1", "t2"), &ScpsWorld::culture_validate);
    ClassDB::bind_method(D_METHOD("culture_preview", "t0", "t1", "t2"),  &ScpsWorld::culture_preview);
    ClassDB::bind_method(D_METHOD("culture_name", "heritage", "seed"),   &ScpsWorld::culture_name);
    ClassDB::bind_method(D_METHOD("set_empire_culture", "slot", "heritage", "ethos", "t0", "t1", "t2"), &ScpsWorld::set_empire_culture);
    ClassDB::bind_method(D_METHOD("set_player_culture", "heritage", "ethos", "t0", "t1", "t2"), &ScpsWorld::set_player_culture);
    ClassDB::bind_method(D_METHOD("set_player_climat", "climat"), &ScpsWorld::set_player_climat);
    ClassDB::bind_method(D_METHOD("clear_player_culture"),          &ScpsWorld::clear_player_culture);
    ClassDB::bind_method(D_METHOD("set_country_name", "cid", "name"), &ScpsWorld::set_country_name);
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
    ClassDB::bind_method(D_METHOD("religion_scholar_expected", "cid"), &ScpsWorld::religion_scholar_expected);
    ClassDB::bind_method(D_METHOD("scholar_role_name", "role"),     &ScpsWorld::scholar_role_name);
    ClassDB::bind_method(D_METHOD("scholar_role_ability", "role"),  &ScpsWorld::scholar_role_ability);
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
    ClassDB::bind_method(D_METHOD("sea_paths"),                      &ScpsWorld::sea_paths);
    ClassDB::bind_method(D_METHOD("sea_travel", "target_region"),    &ScpsWorld::sea_travel);

    /* couches brutes (scps_map_layer) — int en clair côté GDScript :
     * 0 = HEIGHT · 1 = SEA · 2 = BIOME · 3 = COAST · 4 = WATER · 5 = RIVER · 6 = CLIFF */
    BIND_CONSTANT(SCPS_LAYER_HEIGHT);
    BIND_CONSTANT(SCPS_LAYER_SEA);
    BIND_CONSTANT(SCPS_LAYER_BIOME);
    BIND_CONSTANT(SCPS_LAYER_COAST);
    BIND_CONSTANT(SCPS_LAYER_WATER);
    BIND_CONSTANT(SCPS_LAYER_RIVER);
    BIND_CONSTANT(SCPS_LAYER_CLIFF);
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
            int x = (int)(i % w), y = (int)(i / w);
            float grain = political_wash_grain(x, y, o);
            dst[i*4+0] = (uint8_t)CLAMP((int)(c.r * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(c.g * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(c.b * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+3] = (uint8_t)CLAMP((int)(c.a * 255.0f + 0.5f), 0, 255);
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

/* MODE CARTE MARCHÉ (Chantier C) : bassin/cellule teinté par pal[pid du CENTRE] — motif
 * EXACT de political_image() ci-dessus (grain de wash identique), sauf que l'entier lu
 * par cellule est un pid de PROVINCE (scps_map_catchment), pas un pid de pays. `pal`
 * DOIT couvrir 0..province_count()-1 (les seuls pid pouvant apparaître comme centre) —
 * bâtie côté .gd (motif _entity_wash : hash du pid → famille de couleur stable). */
Ref<Image> ScpsWorld::market_catchment_image(PackedColorArray pal) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    uint8_t *dst = buf.ptrw();
    memset(dst, 0, (size_t)w * h * 4);
    if (sim) {
        std::vector<int16_t> ctc((size_t)w * h);
        scps_map_catchment(sim, ctc.data());
        const int np = (int)pal.size();
        for (int64_t i = 0; i < (int64_t)w * h; i++) {
            int o = ctc[(size_t)i];
            if (o < 0 || o >= np) continue;
            Color c = pal[o];
            int x = (int)(i % w), y = (int)(i / w);
            float grain = political_wash_grain(x, y, o);
            dst[i*4+0] = (uint8_t)CLAMP((int)(c.r * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(c.g * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(c.b * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+3] = (uint8_t)CLAMP((int)(c.a * 255.0f + 0.5f), 0, 255);
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

/* MODE CARTE RELIGION (extension 2026-08-19) — MOTIF EXACT de market_catchment_image
 * ci-dessus, l'entier par cellule est un rid de religion (scps_map_religion), `pal`
 * couvre 0..RELIG_MAX-1 (bâtie côté .gd). */
Ref<Image> ScpsWorld::religion_image(PackedColorArray pal) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    uint8_t *dst = buf.ptrw();
    memset(dst, 0, (size_t)w * h * 4);
    if (sim) {
        std::vector<int16_t> rel((size_t)w * h);
        scps_map_religion(sim, rel.data());
        const int np = (int)pal.size();
        for (int64_t i = 0; i < (int64_t)w * h; i++) {
            int o = rel[(size_t)i];
            if (o < 0 || o >= np) continue;
            Color c = pal[o];
            int x = (int)(i % w), y = (int)(i / w);
            float grain = political_wash_grain(x, y, o);
            dst[i*4+0] = (uint8_t)CLAMP((int)(c.r * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(c.g * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(c.b * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+3] = (uint8_t)CLAMP((int)(c.a * 255.0f + 0.5f), 0, 255);
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

/* MODE CARTE CULTURE (extension) — MÊME motif, l'entier par cellule est un culture_id
 * (int32, espace vivant plus large — scps_map_culture) ; `pal` couvre 0..N-1 (bâtie
 * côté .gd, taillée large — l'id est un uint16_t moteur). */
Ref<Image> ScpsWorld::culture_image(PackedColorArray pal) {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    uint8_t *dst = buf.ptrw();
    memset(dst, 0, (size_t)w * h * 4);
    if (sim) {
        std::vector<int32_t> cul((size_t)w * h);
        scps_map_culture(sim, cul.data());
        const int np = (int)pal.size();
        for (int64_t i = 0; i < (int64_t)w * h; i++) {
            int o = cul[(size_t)i];
            if (o < 0 || o >= np) continue;
            Color c = pal[o];
            int x = (int)(i % w), y = (int)(i / w);
            float grain = political_wash_grain(x, y, o);
            dst[i*4+0] = (uint8_t)CLAMP((int)(c.r * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(c.g * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(c.b * grain * 255.0f + 0.5f), 0, 255);
            dst[i*4+3] = (uint8_t)CLAMP((int)(c.a * 255.0f + 0.5f), 0, 255);
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}

/* BROUILLARD DE GUERRE (étape 1/2 — le VOILE visuel, cf. scps_fog.h/scps_api.h) : la
 * façade rend le masque par-CELLULE (motif political_image) — ici on le teinte d'une
 * encre sépia sombre ESTOMPÉE (esprit parchemin, jamais du noir pur) sur les cellules
 * voilées, transparent sur les visibles. Une simple Sprite plein-écran sans shader
 * (composée directement, comme le lavis politique). */
Ref<Image> ScpsWorld::fog_image() {
    int w = scps_map_w(), h = scps_map_h();
    PackedByteArray buf; buf.resize((int64_t)w * h * 4);
    uint8_t *dst = buf.ptrw();
    memset(dst, 0, (size_t)w * h * 4);
    if (sim) {
        std::vector<uint8_t> vis((size_t)w * h);
        scps_fog_visible(sim, vis.data());
        /* ADOUCISSEMENT DU FRONT (display-only) : le masque binaire donne un saut 0→opaque
         * qui suit les fronts carrés de la BFS (blocs crénelés sur la mer, capture joueur
         * 2026-07-09). BFS bornée depuis le visible → l'alpha monte en RAMPE sur FOG_RAMP
         * cellules (halo doux, esprit parchemin). La CONNAISSANCE reste celle du moteur. */
        const int FOG_RAMP = 4;   /* front RESSERRÉ (6→4) : bord doux plus étroit (2026-07-11) */
        std::vector<uint8_t> dist((size_t)w * h, 255);   /* 255 = loin (opaque plein) */
        std::vector<int32_t> q((size_t)w * h);
        int64_t qh = 0, qt = 0;
        for (int64_t i = 0; i < (int64_t)w * h; i++)
            if (vis[(size_t)i]) { dist[(size_t)i] = 0; q[(size_t)qt++] = (int32_t)i; }
        while (qh < qt) {
            int32_t i = q[(size_t)qh++];
            int d = dist[(size_t)i];
            if (d >= FOG_RAMP) continue;
            int x = i % w, y = i / w;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                if (!dx && !dy) continue;
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                int32_t j = ny * w + nx;
                if (dist[(size_t)j] != 255) continue;
                dist[(size_t)j] = (uint8_t)(d + 1);
                q[(size_t)qt++] = j;
            }
        }
        for (int64_t i = 0; i < (int64_t)w * h; i++) {
            if (vis[(size_t)i]) continue;              /* visible : transparent, rien à peindre */
            int d = dist[(size_t)i];
            /* cœur PLEINEMENT OPAQUE 255 (retour joueur 2026-07-11 : « dès qu'on voit à
             * travers, ce n'est pas un brouillard de guerre — fais DISPARAÎTRE ce qu'il y a
             * dessous ! ») : à 252 le lavis politique/les bandes transparaissaient ENCORE
             * (~1 %). Le voile est dessiné EN DERNIER (overlay:2028), donc à 255 il masque
             * TOUT (terrain, lavis, frontières) hors du connu. La rampe de front (bord doux
             * anti-crénelage BFS) est RESSERRÉE (plancher relevé 0.62, RAMP 4) → transition
             * fine et déjà très opaque, plus de « fenêtre » sur la carte au front. */
            uint8_t a = (d >= FOG_RAMP || d == 255) ? 255
                      : (uint8_t)(255.0f * (0.62f + 0.38f * (float)d / (float)FOG_RAMP));
            int x = (int)(i % w), y = (int)(i / w);
            float parchment = political_wash_grain(x, y, -1);
            int hatch = ((x + y) % 14 < 2) ? 1 : 0; /* hachure diagonale large et discrète */
            dst[i*4+0] = (uint8_t)CLAMP((int)(24.0f * parchment + hatch + 0.5f), 0, 255);
            dst[i*4+1] = (uint8_t)CLAMP((int)(19.0f * parchment + hatch + 0.5f), 0, 255);
            dst[i*4+2] = (uint8_t)CLAMP((int)(14.0f * parchment + hatch + 0.5f), 0, 255);
            dst[i*4+3] = a;                                      /* opaque au cœur, rampe au front */
        }
    }
    return Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, buf);
}
/* même connaissance, grain RÉGION (motif map_state_tint : UNE passe, consommée en
 * boucle) — pour que l'overlay grise/cache villes, armées et noms ennemis tombant
 * dans le voile SANS ressonder l'image cellule par cellule pour chaque acteur.
 * Dimensionné sur region_count() (PAS un cap moteur interne : SCPS_MAX_REG n'est
 * jamais exposé à ce binding, motif region_owner/region_tier). */
PackedByteArray ScpsWorld::fog_region_mask() {
    PackedByteArray out;
    int nr = sim ? scps_region_count(sim) : 0;
    if (nr < 0) nr = 0;
    out.resize(nr);
    if (sim && nr > 0) scps_fog_region_mask(sim, out.ptrw());
    return out;
}

int     ScpsWorld::year()          const { return scps_year(sim); }
int     ScpsWorld::player()        const { return scps_player(sim); }
void    ScpsWorld::set_observer(bool on) { if (sim) scps_set_observer(sim, on ? 1 : 0); }
bool    ScpsWorld::is_observer() const   { return sim ? scps_is_observer(sim) != 0 : false; }
int     ScpsWorld::country_count() const { return scps_country_count(sim); }
int     ScpsWorld::country_province_count(int c) const { return scps_country_province_count(sim, c); }
int     ScpsWorld::region_count()  const { return scps_region_count(sim); }
int     ScpsWorld::province_count() const { return scps_province_count(sim); }
int64_t ScpsWorld::world_pop()     const { return (int64_t)scps_world_pop(sim); }
int64_t ScpsWorld::country_pop(int c)  const { return (int64_t)scps_country_pop(sim, c); }
double  ScpsWorld::country_gold(int c) const { return scps_country_gold(sim, c); }
Dictionary ScpsWorld::country_reserve(int c) const {
    Dictionary d; float g=0.f, cp=0.f;
    scps_country_reserve(sim, c, &g, &cp);
    d["gold"]=g; d["copper"]=cp;
    return d;
}
double  ScpsWorld::country_mint_month(int c) const { return scps_country_mint_month(sim, c); }
Dictionary ScpsWorld::country_mint_detail(int c) const {
    /* UI-ALLIAGE : la frappe PHYSIQUE du mois (paires, billon or/cuivre, valeur, débase). */
    Dictionary d; float pair=0.f, bg=0.f, bc=0.f, v=0.f, dbg=0.f;
    scps_country_mint_detail(sim, c, &pair, &bg, &bc, &v, &dbg);
    d["pair"]=pair; d["billon_gold"]=bg; d["billon_copper"]=bc; d["value"]=v; d["debase"]=dbg;
    return d;
}
int     ScpsWorld::country_role(int c) const { return scps_country_role(sim, c); }

int     ScpsWorld::region_owner(int r)     const { return scps_region_owner(sim, r); }
int64_t ScpsWorld::region_pop(int r)       const { return (int64_t)scps_region_pop(sim, r); }
bool    ScpsWorld::region_colonized(int r) const { return scps_region_colonized(sim, r); }

Vector2 ScpsWorld::region_centroid(int r) const {
    float x = -1.f, y = -1.f;
    scps_region_centroid(sim, r, &x, &y);
    return Vector2(x, y);
}

Vector2 ScpsWorld::region_seat(int r) const {
    float x = -1.f, y = -1.f;
    scps_region_seat(sim, r, &x, &y);
    return Vector2(x, y);
}

/* TOPONYMIE (scps_toponym.c) : nom de bourg au grain RÉGION — "" tant que la région n'est
 * pas colonisée (aucune ville). Distinct de province_info(pid)["nom"] (nom de RÉGION,
 * scps_readout.c, non touché ici) : c'est l'IDENTITÉ DE LA VILLE, pas l'ancrage régional. */
String ScpsWorld::region_city_name(int r) const {
    return String::utf8(scps_region_city_name(sim, r));
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
    d["developpement"] = p.developpement;
    d["capadmin"] = p.capadmin;
    d["logements_libres"] = (int64_t)p.logements_libres;
    d["logements_cap"]    = (int64_t)p.logements_cap;
    d["services_libres"]  = (int64_t)p.services_libres;
    d["services_cap"]     = (int64_t)p.services_cap;
    d["habitabilite_pct"] = p.habitabilite_pct;
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
    /* MEMBRANE HONNÊTE (topbar « quoi + combien ») : la façade calcule DÉJÀ
     * c.metab_pct (scps_api.c : econ_country_metabolized × AI_METAB_RES_W) mais
     * le binding ne le copiait jamais dans le Dictionary — champ mort côté GD.
     * Nécessaire pour le hover Savoir (source de bonus chiffrée : « métabolisation
     * +X% recherche »), cf. TROUVAILLES.md. */
    d["metab_pct"]      = c.metab_pct;
    return d;
}

Dictionary ScpsWorld::country_research_income(int country) {
    Dictionary d;
    ScpsResearchIncome ri;
    scps_country_research_income(sim, country, &ri);
    d["per_day"]    = ri.per_day;
    d["pop_daily"]  = ri.pop_daily;
    d["yield_mult"] = ri.yield_mult;
    d["age_mult"]   = ri.age_mult;
    d["metab_pct"]  = ri.metab_pct;
    return d;
}

Dictionary ScpsWorld::army_info(int country) {
    Dictionary d;
    ScpsArmyInfo a;
    scps_army_info(sim, country, &a);
    d["active"] = (bool)a.active;
    if (!a.active) return d;
    d["id"]       = a.id;
    d["region"]   = a.region;
    d["dest"]     = a.dest;
    d["owner"]    = a.owner;
    d["phase_id"] = a.phase_id;
    d["phase"]    = String::utf8(a.phase);
    d["units_are_humans"] = true;
    d["units"]    = (int64_t)a.units;
    d["inf"]      = (int64_t)a.inf;
    d["arch"]     = (int64_t)a.arch;
    d["cav"]      = (int64_t)a.cav;
    d["mages"]    = (int64_t)a.mages;
    return d;
}
Array ScpsWorld::corps_ids(int country) {
    Array out; if (!sim) return out;
    int n=scps_country_corps_count(sim,country);
    for(int i=0;i<n;i++){ int id=scps_country_corps_id(sim,country,i); if(id>=0) out.push_back(id); }
    return out;
}
Dictionary ScpsWorld::corps_info(int id) {
    Dictionary d; ScpsArmyInfo a; scps_corps_info(sim,id,&a);
    d["active"]=(bool)a.active; d["id"]=id; if(!a.active) return d;
    d["region"]=a.region; d["dest"]=a.dest; d["next"]=a.next; d["owner"]=a.owner; d["phase_id"]=a.phase_id;
    d["phase"]=String::utf8(a.phase); d["units"]=(int64_t)a.units; d["inf"]=(int64_t)a.inf;
    d["units_are_humans"]=true;
    d["arch"]=(int64_t)a.arch; d["cav"]=(int64_t)a.cav; d["mages"]=(int64_t)a.mages;
    d["location"]=String::utf8(a.location); d["destination"]=String::utf8(a.destination);
    d["days_left"]=a.days_left; d["leg_days"]=a.leg_days; d["progress_pct"]=a.progress_pct;
    d["broken_days"]=a.broken_days; d["rally_days"]=a.rally_days; d["rally_units"]=(int64_t)a.rally_units;
    d["taken"]=a.taken; d["legs"]=a.legs; d["battles"]=a.battles;
    return d;
}

Dictionary ScpsWorld::corps_move_preview(int id, int target_region) {
    Dictionary d; ScpsMovePreview p{}; int route[128];
    int n=sim?scps_corps_move_preview(sim,id,target_region,&p,route,128):0;
    d["valid"]=(bool)p.valid; d["corps_id"]=p.corps_id;
    d["from_region"]=p.from_region; d["target_region"]=p.target_region;
    d["from_name"]=String::utf8(p.from_name); d["target_name"]=String::utf8(p.target_name);
    d["travel_days"]=p.travel_days; d["hops"]=p.hops;
    d["units_start"]=(int64_t)p.units_start;
    d["attrition_loss"]=(int64_t)p.attrition_loss;
    d["units_arrival"]=(int64_t)p.units_arrival;
    d["attrition_pct"]=p.attrition_pct;
    d["worst_daily_pct10"]=p.worst_daily_pct10;
    d["reason_code"]=p.reason_code; d["reason"]=String::utf8(p.reason);
    d["arrival_code"]=p.arrival_code; d["arrival"]=String::utf8(p.arrival);
    Array path; for(int i=0;i<n;i++)path.push_back(route[i]); d["path"]=path;
    return d;
}

Dictionary ScpsWorld::corps_refill_preview(int id) {
    Dictionary d; ScpsRefillPreview p{};
    scps_corps_refill_preview(sim,id,&p);
    d["valid"]=(bool)p.valid; d["allowed"]=(bool)p.allowed;
    d["corps_id"]=p.corps_id; d["region"]=p.region;
    d["reason_code"]=p.reason_code; d["reason"]=String::utf8(p.reason?p.reason:"");
    d["requested_humans"]=(int64_t)p.requested_humans;
    d["population_ready_humans"]=(int64_t)p.population_ready_humans;
    d["guaranteed_humans"]=(int64_t)p.guaranteed_humans;
    d["weapons_needed"]=(int64_t)p.weapons_needed;
    d["weapons_owned"]=(int64_t)p.weapons_owned;
    Array needs;
    for(int i=0;i<p.n_needs;i++){
        Dictionary n; n["resource"]=p.need[i].resource;
        n["name"]=String::utf8(p.need[i].name?p.need[i].name:"");
        n["needed"]=(int64_t)p.need[i].needed; n["owned"]=(int64_t)p.need[i].owned;
        needs.push_back(n);
    }
    d["needs"]=needs;
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
    d["units_are_humans"] = true;
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
    d["atk_corps"] = b.atk_corps;
    d["def_corps"] = b.def_corps;
    d["atk_helper"] = b.atk_helper;
    d["def_helper"] = b.def_helper;
    d["in_battle"] = (bool)b.in_battle;
    d["days"]       = b.days;
    d["chocs"]      = b.chocs;
    d["atk_morale_pct"] = b.atk_morale_pct;
    d["def_morale_pct"] = b.def_morale_pct;
    d["stage_id"] = b.stage_id;
    d["stage"] = String::utf8(b.stage);
    d["terrain_holder"] = b.terrain_holder;
    d["river"] = (bool)b.river;
    d["bridged"] = (bool)b.bridged;
    d["atk_terrain_pct"] = b.atk_terrain_pct;
    d["def_terrain_pct"] = b.def_terrain_pct;
    d["atk_counter_pct"] = b.atk_counter_pct;
    d["def_counter_pct"] = b.def_counter_pct;
    d["balance_atk_pct"] = b.balance_atk_pct;
    d["rupture_pct"] = b.rupture_pct;
    d["loss_atk"]  = (double)b.loss_atk;
    d["loss_def"]  = (double)b.loss_def;
    d["siege_days_left"] = (double)b.siege_days_left;
    d["siege_full_days"] = (double)b.siege_full_days;
    d["siege_progress_pct"] = b.siege_progress_pct;
    d["siege_defense"] = (double)b.siege_defense;
    d["siege_food_months"] = (double)b.siege_food_months;
    d["siege_terrain_pct"] = b.siege_terrain_pct;
    d["siege_outcome"] = b.siege_outcome;
    d["war_score"] = (double)b.war_score;
    d["climat"] = String::utf8(b.climat ? b.climat : "");
    d["relief"] = String::utf8(b.relief ? b.relief : "");
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
        d["lineage"]  = String::utf8(g[i].lineage);
        d["religion"] = String::utf8(g[i].religion);
        d["faith"]     = String::utf8(g[i].faith);
        d["faith_id"]  = g[i].faith_id;
        d["dominant"]  = (bool)g[i].dominant;
        d["klass"]    = String::utf8(g[i].klass);
        d["etat"]     = String::utf8(g[i].etat);
        d["loyaute"]  = String::utf8(g[i].loyaute);
        d["percent"]  = g[i].percent;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::province_culture_context(int province) {
    Dictionary d; ScpsCultureContext c{};
    scps_province_culture_context(sim,province,&c);
    d["valid"]=(bool)c.valid; d["province"]=c.province; d["region"]=c.region; d["owner"]=c.owner;
    d["groups"]=c.groups; d["dominant_culture"]=String::utf8(c.dominant_culture?c.dominant_culture:"");
    d["dominant_lineage"]=String::utf8(c.dominant_lineage?c.dominant_lineage:"");
    d["dominant_percent"]=c.dominant_percent;
    d["local_ethos"]=String::utf8(c.local_ethos?c.local_ethos:"");
    d["ruling_ethos"]=String::utf8(c.ruling_ethos?c.ruling_ethos:"");
    d["relation_to_crown"]=String::utf8(c.relation_to_crown?c.relation_to_crown:"");
    d["ethos_drift_pct"]=c.ethos_drift_pct; d["friction_avg_pct"]=c.friction_avg_pct;
    d["friction_max_pct"]=c.friction_max_pct;
    d["local_faith_id"]=c.local_faith_id; d["state_faith_id"]=c.state_faith_id;
    d["faith_mismatch"]=(bool)c.faith_mismatch;
    d["local_faith"]=String::utf8(c.local_faith?c.local_faith:"");
    d["state_faith"]=String::utf8(c.state_faith?c.state_faith:"");
    d["contact"]=(bool)c.contact; d["contact_region"]=c.contact_region;
    d["contact_country"]=c.contact_country; d["contact_maritime"]=(bool)c.contact_maritime;
    d["contact_country_name"]=String::utf8(c.contact_country_name?c.contact_country_name:"");
    d["contact_region_name"]=String::utf8(c.contact_region_name?c.contact_region_name:"");
    d["contact_culture"]=String::utf8(c.contact_culture?c.contact_culture:"");
    d["contact_distance_pct"]=c.contact_distance_pct; d["fusion_open_pct"]=c.fusion_open_pct;
    d["fusion_feasible"]=(bool)c.fusion_feasible; d["fusion_years"]=c.fusion_years;
    d["fusion_reason"]=String::utf8(c.fusion_reason?c.fusion_reason:"");
    return d;
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

/* MÉTRIQUES PROVINCE — même forme que province_agitation ({value, causes}). */
static Dictionary _breakdown_dict(::ScpsSim *sim, int province,
        int (*fn)(::ScpsSim*, int, int*, ScpsBreakdownLine*, int)) {
    Dictionary out;
    Array causes;
    int value = 0;
    ScpsBreakdownLine bl[6];
    int n = sim ? fn(sim, province, &value, bl, 6) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["cause"] = String::utf8(bl[i].cause);
        d["delta"] = bl[i].delta;
        causes.push_back(d);
    }
    out["value"] = value;
    out["causes"] = causes;
    return out;
}
Dictionary ScpsWorld::province_developpement(int province) { return _breakdown_dict(sim, province, scps_province_developpement); }
Dictionary ScpsWorld::province_capadmin(int province)      { return _breakdown_dict(sim, province, scps_province_capadmin); }
Dictionary ScpsWorld::province_services_why(int province)  { return _breakdown_dict(sim, province, scps_province_services_why); }

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

/* les ÉDIFICES de BASE bâtis (masque edi_built — grenier, marché, temple…). */
Array ScpsWorld::province_edifices(int province) {
    Array a;
    ScpsProvBld b[32];
    int n = sim ? scps_province_edifices(sim, province, b, 32) : 0;
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"]  = String::utf8(b[i].nom);
        d["type"] = b[i].niveau;   /* le TYPE Edifice (vignette UIKit.building_sprite) */
        a.push_back(d);
    }
    return a;
}
int ScpsWorld::province_friche(int province) const {
    return sim ? scps_province_friche(sim, province) : 0;
}

int ScpsWorld::day_of_year() const { return scps_day_of_year(sim); }
int ScpsWorld::country_known(int country) const { return scps_country_known(sim, country); }

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

/* SATISFACTION 0-100 par classe (−1 = classe vide), grain province — incl. serviles. */
Dictionary ScpsWorld::province_class_sat(int province) {
    int lab = -1, art = -1, nob = -1, slv = -1;
    scps_province_class_sat(sim, province, &lab, &art, &nob, &slv);
    Dictionary d;
    d["laboureurs"] = lab;
    d["artisans"]   = art;
    d["noblesse"]   = nob;
    d["esclaves"]   = slv;
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

/* MODE CARTE MARCHÉ (Chantier C/E) : pid du CENTRE dont dépend `province` (-1 : aucun),
 * centroïde d'ancrage (icône de tuile), ≤2 brutes du tirage — passe-plats purs. */
int ScpsWorld::market_catchment(int province) const {
    return sim ? scps_market_catchment(sim, province) : -1;
}
Vector2 ScpsWorld::province_centroid(int province) const {
    float x = -1.f, y = -1.f;
    scps_province_centroid(sim, province, &x, &y);
    return Vector2(x, y);
}
PackedInt32Array ScpsWorld::province_raws(int province) const {
    PackedInt32Array out;
    int ids[2];
    int n = sim ? scps_province_raws(sim, province, ids, 2) : 0;
    for (int i = 0; i < n; i++) out.push_back(ids[i]);
    return out;
}
String ScpsWorld::market_hover(int province) const {
    return sim ? String::utf8(scps_market_hover(sim, province)) : String();
}
String ScpsWorld::map_mode_label(int i) const {
    return String::utf8(scps_map_mode_label(i));
}
int ScpsWorld::province_religion(int province) const {
    return sim ? scps_province_religion(sim, province) : -1;
}
String ScpsWorld::province_religion_hover(int province) const {
    return sim ? String::utf8(scps_province_religion_hover(sim, province)) : String();
}
int ScpsWorld::province_culture_id(int province) const {
    return sim ? scps_province_culture_id(sim, province) : -1;
}
String ScpsWorld::province_culture_hover(int province) const {
    return sim ? String::utf8(scps_province_culture_hover(sim, province)) : String();
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

int ScpsWorld::country_class_policy_sat(int country, int classe) const {
    return sim ? scps_country_class_policy_sat(sim, country, classe) : 0;
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
        d["supply_month"]  = st[i].supply_month;
        d["demand_month"]  = st[i].demand_month;
        d["coverage_days"] = st[i].coverage_days;
        d["market_band"]   = st[i].market_band;
        d["price"]         = st[i].price;
        d["res_id"]        = st[i].res_id;
        a.push_back(d);
    }
    return a;
}

Array ScpsWorld::stock_regions(int country, int good) {
    Array a;
    ScpsStockRegion rows[64];
    int n=scps_stock_regions(sim,country,good,rows,64);
    for(int i=0;i<n;i++){
        Dictionary d;
        d["region"]=rows[i].region; d["province"]=rows[i].province;
        d["name"]=String::utf8(rows[i].name);
        d["stock"]=(int64_t)rows[i].stock;
        d["supply_month"]=rows[i].supply_month;
        d["demand_month"]=rows[i].demand_month;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::market_quote(int country, int good, int qty) {
    Dictionary d;
    ScpsMarketQuote q{};
    if (!sim || !scps_market_quote(sim, country, good, (long)qty, &q)) {
        d["valid"] = false;
        return d;
    }
    d["valid"] = q.valid != 0;
    d["region"] = q.region;
    d["hub_region"] = q.hub_region;
    d["hub_owner"] = q.hub_owner;
    d["hub_name"] = String::utf8(q.hub_name);
    d["global_access"] = q.global_access != 0;
    d["price"] = q.price;
    d["margin"] = q.margin;
    d["local_available"] = q.local_available;
    d["global_available"] = q.global_available;
    d["commerce_remaining"] = q.commerce_remaining;
    d["request_qty"] = (int64_t)q.request_qty;
    d["local_qty"] = (int64_t)q.local_qty;
    d["global_qty"] = (int64_t)q.global_qty;
    d["local_cost"] = q.local_cost;
    d["global_cost"] = q.global_cost;
    return d;
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
    d["truce_days"]           = ok ? (double)o.truce_days : 0.0;
    /* W-GUERRE-3 — l'état de l'intrigue fabriquée (CB payant) contre `target`. */
    d["can_fabricate"]          = (bool)(ok && o.can_fabricate);
    d["fabricate_cost"]         = ok ? (double)o.fabricate_cost : 0.0;
    d["fabricating"]            = (bool)(ok && o.fabricating);
    d["fabricating_days_left"]  = ok ? (double)o.fabricating_days_left : 0.0;
    d["cb_ready"]                = (bool)(ok && o.cb_ready);
    d["cb_ready_years_left"]    = ok ? (double)o.cb_ready_years_left : 0.0;
    d["claim_region"]            = ok ? o.claim_region : -1;
    d["claim_province"]          = ok ? o.claim_province : -1;
    d["claim_name"]              = String::utf8(ok&&o.claim_name?o.claim_name:"");
    return d;
}

Dictionary ScpsWorld::diplo_action_legal(int target, int action) {
    Dictionary d; ScpsActionLegal a{};
    scps_diplo_action_legal(sim,target,action,&a);
    d["valid"]=(bool)a.valid; d["allowed"]=(bool)a.allowed;
    d["would_accept"]=(bool)a.would_accept; d["unilateral"]=(bool)a.unilateral;
    d["target"]=a.target; d["action"]=a.action; d["toggle_on"]=(bool)a.toggle_on;
    d["reason_code"]=String::utf8(a.reason_code?a.reason_code:"");
    d["reason_label"]=String::utf8(a.reason_label?a.reason_label:"");
    d["cost_gold"]=a.cost_gold; d["gold_have"]=a.gold_have;
    d["gold_missing"]=a.gold_missing; d["duration_days"]=a.duration_days;
    /* LA CHECKLIST DE REFUS (2026-07-21) : [{label, ok}, …] — l'UI la rend en hover. */
    Array conds;
    for (int i=0;i<a.n_conds;i++){
        Dictionary c; c["label"]=String::utf8(a.conds[i].label); c["ok"]=(bool)a.conds[i].ok;
        conds.push_back(c);
    }
    d["conds"]=conds;
    return d;
}

Dictionary ScpsWorld::diplo_context(int target) {
    Dictionary d; ScpsDiploContext c{};
    scps_diplo_context(sim,target,&c);
    d["valid"]=(bool)c.valid; d["player"]=c.player; d["target"]=c.target;
    d["at_war"]=(bool)c.at_war; d["allied"]=(bool)c.allied;
    d["trade_pact"]=(bool)c.trade_pact; d["migration_pact"]=(bool)c.migration_pact;
    d["embargo"]=(bool)c.embargo; d["truce_days"]=c.truce_days; d["war_score"]=c.war_score;
    d["ally_slots_player"]=c.ally_slots_player; d["ally_slots_target"]=c.ally_slots_target;
    d["ally_slots_max"]=c.ally_slots_max; d["vassal_direction"]=c.vassal_direction;
    d["contract"]=String::utf8(c.contract?c.contract:""); d["trade_value"]=c.trade_value;
    d["shared_routes"]=c.shared_routes; d["open_routes"]=c.open_routes;
    d["route_a"]=c.route_a; d["route_b"]=c.route_b;
    d["route_maritime"]=(bool)c.route_maritime; d["route_open"]=(bool)c.route_open;
    d["route_sea_days"]=c.route_sea_days; d["route_yield"]=c.route_yield;
    d["route_a_name"]=String::utf8(c.route_a_name?c.route_a_name:"");
    d["route_b_name"]=String::utf8(c.route_b_name?c.route_b_name:"");
    d["target_capital_province"]=c.target_capital_province;
    d["target_capital_region"]=c.target_capital_region;
    d["target_capital_name"]=String::utf8(c.target_capital_name?c.target_capital_name:"");
    return d;
}

Dictionary ScpsWorld::peace_terms(int target) {
    Dictionary d; ScpsPeacePreview p{};
    scps_peace_preview(sim,target,&p);
    d["valid"]=(bool)p.valid;d["at_war"]=(bool)p.at_war;d["target"]=p.target;
    d["war_score"]=p.war_score;d["revenue_year"]=p.revenue_year;
    d["revenue_month"]=p.revenue_month;d["gold_per_score"]=p.gold_per_score;
    d["gold_available"]=p.gold_available;d["gold_max"]=p.gold_max;
    d["vassal_score"]=p.vassal_score;d["target_regions"]=p.target_regions;
    d["fragment_possible"]=(bool)p.fragment_possible;
    d["reparations_cost"]=p.reparations_cost;d["humiliate_cost"]=p.humiliate_cost;
    d["pillage_cost"]=p.pillage_cost;d["liberate_cost"]=p.liberate_cost;
    d["fragment_cost"]=p.fragment_cost;
    Array rows;
    if(sim){
        ScpsPeaceTerritory tr[SCPS_PEACE_TERRITORY_MAX];
        int n=scps_peace_territories(sim,target,tr,SCPS_PEACE_TERRITORY_MAX);
        for(int i=0;i<n;i++){
            Dictionary x;x["region"]=tr[i].region;x["province"]=tr[i].province;
            x["name"]=String::utf8(tr[i].name?tr[i].name:"");
            x["score_cost"]=tr[i].score_cost;x["occupied"]=(bool)tr[i].occupied;
            rows.push_back(x);
        }
    }
    d["territories"]=rows;
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
        /* CADRES DU CONSEIL (2026-07-10) : identité déterministe PUREMENT NARRATIVE
         * (effet mécanique 0 — cf. scps_api.h ScpsCouncilSeat + la spec). */
        d["identite"]    = String::utf8(seats[i].identite);
        d["portrait_id"] = seats[i].portrait_id;
        d["id_flavor"]   = String::utf8(seats[i].id_flavor);
        /* CARTE CONSEIL (2026-07-10, § « Interface (cartes) ») — personne+maison,
         * bonus de rang/efficacité/final, coût {taux,montant courant}, retraite,
         * décomposition (K/Corruption) pour le hover. "" / 0 si vacant. */
        d["firstname"]       = String::utf8(seats[i].firstname);
        d["house"]           = String::utf8(seats[i].house);
        d["domain"]          = String::utf8(seats[i].domain);
        d["rank_bonus_pct"]  = seats[i].rank_bonus_pct;
        d["efficiency_pct"]  = seats[i].efficiency_pct;
        d["final_bonus_pct"] = seats[i].final_bonus_pct;
        d["cost_rate_pct"]   = seats[i].cost_rate_pct;
        d["cost_year"]       = seats[i].cost_year;
        d["retire_lo"]       = seats[i].retire_lo;
        d["retire_hi"]       = seats[i].retire_hi;
        d["k_admin"]         = seats[i].k_admin;
        d["corruption_pct"]  = seats[i].corruption_pct;
        d["eff_base_pct"]          = seats[i].eff_base_pct;
        d["eff_admin_points"]      = seats[i].eff_admin_points;
        d["eff_loyalty_points"]    = seats[i].eff_loyalty_points;
        d["eff_corruption_points"] = seats[i].eff_corruption_points;
        d["eff_preclamp_pct"]      = seats[i].eff_preclamp_pct;
        d["eff_clamped"]           = (bool)seats[i].eff_clamped;
        d["loyalty_target"]        = seats[i].loyalty_target;
        a.push_back(d);
    }
    return a;
}

/* CANDIDATS d'un siège (pool de la génération courante — toujours pleine) :
 * nom · tier · ÂGE · coût/mois — l'embauche ÉCLAIRÉE du joueur, + la CARTE
 * complète (personne+maison, bonus/efficacité PRÉVUE/coût annuel/retraite). */
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
        d["identite"]    = String::utf8(c[i].identite);
        d["portrait_id"] = c[i].portrait_id;
        d["id_flavor"]   = String::utf8(c[i].id_flavor);
        d["firstname"]       = String::utf8(c[i].firstname);
        d["house"]           = String::utf8(c[i].house);
        d["faction"]         = String::utf8(c[i].faction);
        d["domain"]          = String::utf8(c[i].domain);
        d["rank_bonus_pct"]  = c[i].rank_bonus_pct;
        d["efficiency_pct"]  = c[i].efficiency_pct;
        d["final_bonus_pct"] = c[i].final_bonus_pct;
        d["cost_rate_pct"]   = c[i].cost_rate_pct;
        d["cost_year"]       = c[i].cost_year;
        d["retire_lo"]       = c[i].retire_lo;
        d["retire_hi"]       = c[i].retire_hi;
        d["predicted_loyalty"]     = c[i].predicted_loyalty;
        d["eff_base_pct"]          = c[i].eff_base_pct;
        d["eff_admin_points"]      = c[i].eff_admin_points;
        d["eff_loyalty_points"]    = c[i].eff_loyalty_points;
        d["eff_corruption_points"] = c[i].eff_corruption_points;
        d["eff_preclamp_pct"]      = c[i].eff_preclamp_pct;
        d["eff_clamped"]           = (bool)c[i].eff_clamped;
        a.push_back(d);
    }
    return a;
}

/* DÉCRETS DU JOUEUR (civics) : nom · flavor · les DEUX plateaux · réforme (irréversible) ·
 * actif · légal (condition d'entrée remplie MAINTENANT — pour griser le bouton) ·
 * type/exclusive_id/cost_rate_pct/cost_year/cond_met/cooldown_active (2026-07-10,
 * recâblage Politiques — carte orientation, cf. scps_api.h ScpsDecree). ⚠ le tampon
 * DOIT couvrir DECREE_COUNT (11 aujourd'hui : 10 orientations + 1 décision) — un
 * tampon de 8 tronquait silencieusement les 3 derniers décrets (Légations, Levée
 * entretenue, ET la décision Audit des offices elle-même : la carte décision était
 * invisible). 16 = headroom, comme les autres tampons de ce fichier (unit_roster
 * u[64], etc.). */
Array ScpsWorld::decrees_list(int country) {
    Array a;
    if (!sim) return a;
    ScpsDecree d[16];
    int n = scps_decrees_list(sim, country, d, 16);
    for (int i = 0; i < n; i++) {
        Dictionary dd;
        dd["id"]              = d[i].id;
        dd["nom"]             = String::utf8(d[i].nom);
        dd["flavor"]          = String::utf8(d[i].flavor);
        dd["plateaux"]        = String::utf8(d[i].plateaux);
        dd["reforme"]         = (bool)d[i].reforme;
        dd["active"]          = (bool)d[i].active;
        dd["legal"]           = (bool)d[i].legal;
        dd["type"]            = d[i].type;
        dd["exclusive_id"]    = d[i].exclusive_id;
        dd["cost_rate_pct"]   = (double)d[i].cost_rate_pct;
        dd["cost_year"]       = d[i].cost_year;
        dd["cond_met"]        = (bool)d[i].cond_met;
        dd["cooldown_active"] = (bool)d[i].cooldown_active;
        a.push_back(dd);
    }
    return a;
}

/* Assiette des coûts % — hovers quantitatifs (« 3 % du revenu (2033 or) × IPM 1,12 = 68 or/an »). */
double ScpsWorld::country_revenue_year(int country) {
    return sim ? scps_country_revenue_year(sim, country) : 0.0;
}
double ScpsWorld::tax_class_month(int cls) {
    return sim ? scps_tax_class_month(sim, cls) : 0.0;
}
double ScpsWorld::world_ipm() {
    return sim ? scps_world_ipm_now(sim) : 1.0;
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
        d["categorie"]       = String::utf8(u[i].categorie);
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
        d["entretien"] = b[i].entretien;   /* or/mois une fois bâti (miroir E1bis.10) */
        d["days"]     = b[i].days;
        d["debloque"] = (bool)b[i].debloque;
        d["tier"]       = b[i].tier;
        d["prev"]       = b[i].prev;
        d["prev_built"] = (bool)b[i].prev_built;
        d["effet"]      = String::utf8(b[i].effet);
        d["flavor"]     = String::utf8(b[i].flavor);
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
        d["allowed"]  = (bool)nd[i].allowed;
        d["reason_code"] = String::utf8(nd[i].reason_code);
        d["reason_label"] = String::utf8(nd[i].reason_label);
        d["points_have"] = nd[i].points_have;
        d["points_missing"] = nd[i].points_missing;
        d["next_step"] = nd[i].next_step;
        d["steps_remaining"] = nd[i].steps_remaining;
        d["path_label"] = String::utf8(nd[i].path_label);
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
    d["monthly_income"] = b.monthly_income;
    d["monthly_expense"] = b.monthly_expense;
    d["monthly_net"] = b.monthly_net;
    d["projected_year_end"] = b.projected_year_end;
    d["runway_months"] = b.runway_months;
    d["creditor"]      = b.creditor;
    d["creditor_name"] = String::utf8(b.creditor_name);
    return d;
}

Dictionary ScpsWorld::budget_controls(int country) {
    Dictionary d;
    Array taxes, spending;
    static const char *tax_names[3] = {"Laboureurs", "Artisans", "Noblesse"};
    static const char *spend_names[7] = {"Investissement public", "Entretien des bâtiments", "Armée", "Flotte", "Entretien des routes", "Frappe", "Débase"};
    for (int i=0;i<3;i++){
        Dictionary row; row["id"]=i; row["name"]=String::utf8(tax_names[i]);
        row["mult"]=scps_country_budget_policy(sim,country,0,i); taxes.push_back(row);
    }
    for (int i=0;i<7;i++){
        Dictionary row; row["id"]=i; row["name"]=String::utf8(spend_names[i]);
        row["mult"]=scps_country_budget_policy(sim,country,1,i); spending.push_back(row);
    }
    d["taxes"]=taxes; d["spending"]=spending;
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
    /* CARTE MISSION (2026-07-10) : le siège RESPONSABLE + son bonus + la récompense
     * PRÉVUE (base × bonus, or ET matière) — § « Interface (cartes) ». */
    d["resp_seat"]       = String::utf8(m.resp_seat);
    d["resp_name"]       = String::utf8(m.resp_name);
    d["resp_tier"]       = m.resp_tier;
    d["resp_bonus_pct"]  = m.resp_bonus_pct;
    d["reward_gold_adj"] = m.reward_gold_adj;
    d["reward_qty_adj"]  = m.reward_qty_adj;
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
        e["base_part"] = f[i].base_part;
        e["policy_delta"] = f[i].policy_delta;
        e["grief"]    = f[i].grief;
        e["dominant"] = (bool)f[i].dominant;
        e["coup_pressure"] = f[i].coup_pressure;
        e["coup_driver"] = (bool)f[i].coup_driver;
        e["captor"] = (bool)f[i].captor;
        list.push_back(e);
    }
    d["list"] = list;
    d["coup"] = coup;
    d["corruption"] = cor;
    return d;
}

bool ScpsWorld::player_build(int edifice, int province) {
    return sim ? scps_player_build(sim, edifice, province) != 0 : false;
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
    d["effects"] = String::utf8(sim ? scps_age_effects(sim) : "");
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
    Array labels, blurbs, flavors, advisors, effets, gold_delta;
    for (int i = 0; i < (ok ? pe.n_options : 0); i++) {
        labels.push_back(String::utf8(pe.labels[i]));
        blurbs.push_back(String::utf8(pe.blurbs[i]));
        flavors.push_back(String::utf8(pe.flavors[i]));
        advisors.push_back(String::utf8(pe.advisors[i]));
        effets.push_back(String::utf8(pe.effets[i]));
        gold_delta.push_back(pe.gold_delta[i]);
    }
    d["labels"]   = labels;
    d["blurbs"]   = blurbs;
    d["flavors"]  = flavors;
    d["advisors"] = advisors;   /* le VISAGE de chaque choix (mot de faction, "" si aucun) */
    d["effets"]   = effets;     /* l'EFFET MÉCANIQUE en clair (« Ça veut dire quoi ? ») */
    d["gold_delta"] = gold_delta; /* même montant signé que le drain : <0 coût, >0 gain */
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
/* §3 — le RESTE de la surface (wiring UI complet) : intérieur · conseil · commerce · guerre.
 * RE-KEY PROVINCE : repress/assimilate/purge/action_preview prennent un PID direct. */
bool ScpsWorld::player_repress(int prov)               { return sim ? scps_player_repress(sim, prov) != 0 : false; }
bool ScpsWorld::player_assimilate(int prov, bool creuset) { return sim ? scps_player_assimilate(sim, prov, creuset ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_purge(int prov)                 { return sim ? scps_player_purge(sim, prov) != 0 : false; }
/* APERÇU D'ACTION (UI-4) : { cost_gold, duration_days, pop_delta, satisfaction_delta,
 * agitation_delta, coercition_delta, risque } — lecture pure, aucune mutation. */
Dictionary ScpsWorld::action_preview(int prov, int verb) {
    Dictionary out;
    out["cost_gold"] = 0.0; out["duration_days"] = 0; out["pop_delta"] = 0;
    out["satisfaction_delta"] = 0; out["agitation_delta"] = 0; out["coercition_delta"] = 0;
    out["risque"] = String();
    if (!sim) return out;
    ScpsActionPreview p;
    if (scps_action_preview(sim, prov, verb, &p)) {
        out["cost_gold"]          = (double)p.cost_gold;
        out["duration_days"]      = p.duration_days;
        out["pop_delta"]          = p.pop_delta;
        out["satisfaction_delta"] = p.satisfaction_delta;
        out["agitation_delta"]    = p.agitation_delta;
        out["coercition_delta"]   = p.coercition_delta;
        out["risque"]             = String::utf8(p.risque);
    }
    return out;
}
bool ScpsWorld::player_council_hire(int seat, int slot)  { return sim ? scps_player_council_hire(sim, seat, slot) != 0 : false; }
bool ScpsWorld::player_council_dismiss(int seat)         { return sim ? scps_player_council_dismiss(sim, seat) != 0 : false; }
bool ScpsWorld::player_council_pay(int seat, float pay)  { return sim ? scps_player_council_pay(sim, seat, pay) != 0 : false; }
bool ScpsWorld::player_budget_policy(int family, int index, float mult) { return sim ? scps_player_budget_policy(sim, family, index, mult) != 0 : false; }
int  ScpsWorld::council_pair_state(int seat_a, int seat_b) { return sim ? scps_council_pair_state(sim, seat_a, seat_b) : 0; }
bool ScpsWorld::player_decree(int id, bool on)            { return sim ? scps_player_decree(sim, id, on ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_route(int ra, int rb, bool maritime) { return sim ? scps_player_route(sim, ra, rb, maritime ? 1 : 0) != 0 : false; }
bool ScpsWorld::player_market_buy(int region, int good, int qty, int tier)  { return sim ? scps_player_market_buy(sim, region, good, (long)qty, tier) != 0 : false; }
bool ScpsWorld::player_market_sell(int region, int good, int qty, int tier) { return sim ? scps_player_market_sell(sim, region, good, (long)qty, tier) != 0 : false; }
bool ScpsWorld::player_campaign(int from_region, int target_region) { return sim ? scps_player_campaign(sim, from_region, target_region) != 0 : false; }
bool ScpsWorld::player_move_army(int target_region) { return sim ? scps_player_move_army(sim, target_region) != 0 : false; }
bool ScpsWorld::player_refill()                          { return sim ? scps_player_refill(sim) != 0 : false; }
bool ScpsWorld::player_navy_build(int hull)              { return sim ? scps_player_navy_build(sim, hull) != 0 : false; }
bool ScpsWorld::player_disband()                         { return sim ? scps_player_disband(sim) != 0 : false; }
bool ScpsWorld::player_raise_corps(int packets,int target_region){ return sim?scps_player_raise_corps(sim,packets,target_region)!=0:false; }
bool ScpsWorld::player_split_corps(int id,int packets){ return sim?scps_player_split_corps(sim,id,packets)!=0:false; }
bool ScpsWorld::player_split_comp(int id,int inf_p,int arch_p,int cav_p,int mages_p){
    return sim?scps_player_split_comp(sim,id,inf_p,arch_p,cav_p,mages_p)!=0:false;
}
bool ScpsWorld::player_merge_corps(int dst_id,int src_id){ return sim?scps_player_merge_corps(sim,dst_id,src_id)!=0:false; }
bool ScpsWorld::player_move_corps(int id,int target_region){ return sim?scps_player_move_corps(sim,id,target_region)!=0:false; }
bool ScpsWorld::player_refill_corps(int id){ return sim?scps_player_refill_corps(sim,id)!=0:false; }
bool ScpsWorld::player_disband_corps(int id){ return sim?scps_player_disband_corps(sim,id)!=0:false; }

/* PANNEAU B — manufacture civile par PROVINCE (RE-KEY : pid direct) : le verbe (enfile)
 * + la légalité (read-only). */
bool ScpsWorld::player_build_manuf(int province, int bld) {
    return sim ? scps_player_build_manuf(sim, province, bld) != 0 : false;
}
bool ScpsWorld::player_manuf_level(int province, int bld, int dir) {
    return sim ? scps_player_manuf_level(sim, province, bld, dir) != 0 : false;
}
bool ScpsWorld::player_demolish_edifice(int province, int edifice) {
    return sim ? scps_player_demolish_edifice(sim, province, edifice) != 0 : false;
}
int ScpsWorld::manuf_legal(int province, int bld) {
    return sim ? scps_manuf_legal(sim, province, bld) : 0;
}
int ScpsWorld::manuf_cost() const {
    return sim ? scps_manuf_cost(sim) : 0;
}
/* ENTRETIEN/mois d'une manufacture — miroir E1bis.10 (au niveau bâti, ou naissance si
 * pas encore posée là : le picker prévisualise). */
int ScpsWorld::manuf_upkeep_month(int province, int bld) const {
    return sim ? scps_manuf_upkeep_month(sim, province, bld) : 0;
}
/* LA RECETTE réelle d'une manufacture (menu construction, « vérité absolue ») : noms
 * résolus + quantités RÉELLES (RECIPE). Ne dépend pas du sim (table de design) mais on
 * garde la signature membrane-cohérente (const, pas de sim requis). */
Dictionary ScpsWorld::manuf_recipe(int bld) const {
    Dictionary d;
    ScpsManufRecipe r;
    scps_manuf_recipe(bld, &r);
    d["in1"] = String::utf8(r.in1);   d["q1"] = r.q1;
    d["in2"] = String::utf8(r.in2);   d["q2"] = r.q2;
    d["out"] = String::utf8(r.out);   d["qout"] = r.qout;
    d["alt1"] = String::utf8(r.alt1); d["alt1_q"] = r.alt1_q;
    return d;
}
/* LÉGALITÉ de construction (lot M, membrane honnête) : miroir read-only des gates du
 * drain CMD_BUILD — legal + la RAISON du refus (0 OK · 1 structurel · 2 or · 3 matière).
 * RE-KEY : `province` est un PID DIRECT (jamais une région). */
Dictionary ScpsWorld::build_legal(int province, int edifice) {
    Dictionary d;
    int reason = 1;
    int legal = sim ? scps_build_legal_ex(sim, province, edifice, &reason) : 0;
    d["legal"]  = (legal != 0);
    d["reason"] = reason;
    d["allowed"] = (legal != 0); /* contrat UI structuré — alias de migration */
    const char *code = "structural";
    const char *label = "indisponible ici (palier, file ou bâtiment existant)";
    switch (reason) {
        case 0: code = "ok";                label = "disponible"; break;
        case 2: code = "insufficient_gold"; label = "or insuffisant"; break;
        case 3: code = "missing_material";  label = "matière manquante"; break;
        case 4: code = "missing_tier_tech"; label = "technologie de palier manquante"; break;
        default: break;
    }
    d["reason_code"]  = String::utf8(code);
    d["reason_label"] = String::utf8(label);
    return d;
}

Dictionary ScpsWorld::renover_state(int province) {
    Dictionary d;
    ScpsRenoverState rs;
    if (!sim || !scps_renover_state(sim, province, &rs)) return d;
    d["wear_pct"] = rs.wear_pct;
    d["gold"]     = rs.gold;
    d["allowed"]  = (rs.allowed != 0);
    d["reason"]   = rs.reason;   /* 1 rien à rénover · 2 or */
    return d;
}

bool ScpsWorld::player_renover(int province) {
    return sim && scps_player_renover(sim, province) != 0;
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
String ScpsWorld::edifice_name(int edifice) {
    return String::utf8(scps_edifice_name(edifice));
}
int ScpsWorld::edifice_succ(int edifice) {
    return scps_edifice_succ(edifice);
}
int ScpsWorld::edifice_upkeep_month(int edifice) const {
    return scps_edifice_upkeep_month(edifice);
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
bool ScpsWorld::player_peace_offer(int target,const PackedInt32Array &regions,int gold_score,int flags){
    if(!sim||regions.size()>SCPS_PEACE_TERRITORY_MAX)return false;
    int rr[SCPS_PEACE_TERRITORY_MAX];
    for(int i=0;i<regions.size();i++)rr[i]=regions[i];
    return scps_player_peace_offer(sim,target,rr,regions.size(),gold_score,flags)!=0;
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

/* LOT P — PILLER LA CÔTE : verbe (enfile) + légalité/CD (griser + « côte balafrée — X j »). */
bool ScpsWorld::player_raid_coast(int prov) {
    return sim ? scps_player_raid_coast(sim, prov) != 0 : false;
}
Dictionary ScpsWorld::can_raid_coast(int prov) {
    Dictionary d;
    int reason = 1, legal = 0, cd = 0;
    if (sim) {
        legal = scps_can_raid_coast(sim, prov, &reason);
        cd    = scps_raid_cd_days(sim, prov);
    }
    d["legal"]   = (bool)legal;
    d["reason"]  = reason;
    d["cd_days"] = cd;
    return d;
}

/* ── ALLOCATION DE MAIN-D'ŒUVRE — la membrane traverse en Dictionary (mots + poids).
 * RE-KEY PROVINCE : `province` est un PID DIRECT (jamais une région). Le champ Dictionary
 * "region" garde son nom (compat binding) mais porte désormais un PID. ── */
Dictionary ScpsWorld::province_alloc(int province) {
    Dictionary out;
    ScpsAlloc al;
    if (sim) scps_province_alloc(sim, province, &al); else al.region = -1;
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
bool ScpsWorld::player_alloc_raw(int province, int resource, int weight) {
    return sim ? scps_player_alloc_raw(sim, province, resource, weight) != 0 : false;
}
bool ScpsWorld::player_alloc_bld(int province, int bld_type, int weight) {
    return sim ? scps_player_alloc_bld(sim, province, bld_type, weight) != 0 : false;
}
bool ScpsWorld::player_alloc_input(int province, int bld_type, int input) {
    return sim ? scps_player_alloc_input(sim, province, bld_type, input) != 0 : false;
}
bool ScpsWorld::player_alloc_auto(int province) {
    return sim ? scps_player_alloc_auto(sim, province) != 0 : false;
}

/* ── LOT G — RÉINCORPORATION DE POP — RE-KEY PROVINCE : src_prov/dst_prov PID directs ── */
bool ScpsWorld::player_pop_transfer(int src_prov, int dst_prov, int klass, int count) {
    return sim ? scps_player_pop_transfer(sim, src_prov, dst_prov, klass, (long)count) != 0 : false;
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
/* PÉNURIES (UI-2, topbar) — [{nom, res_id, runway_days, structurel}], trié urgence croissante. */
Array ScpsWorld::country_shortages(int country) {
    Array a;
    if (!sim) return a;
    ScpsShortage sh[64];   /* RES_COUNT ~57 aujourd'hui ; scps_api.h reste opaque (pas de RES_COUNT côté hôte) */
    int n = scps_country_shortages(sim, country, sh, 64);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["nom"]         = String::utf8(sh[i].nom);
        d["res_id"]       = sh[i].res_id;
        d["runway_days"] = (double)sh[i].runway_days;
        d["structurel"]  = (bool)sh[i].structurel;
        a.push_back(d);
    }
    return a;
}

/* ── ESCLAVAGE — les 3 verbes orphelins + le lecteur de marché — RE-KEY PROVINCE :
 * slave_buy/slave_sell prennent un PID direct ── */
bool ScpsWorld::player_manumit() {
    return sim ? scps_player_manumit(sim) != 0 : false;
}
bool ScpsWorld::player_slave_buy(int prov, int count) {
    return sim ? scps_player_slave_buy(sim, prov, (long)count) != 0 : false;
}
bool ScpsWorld::player_slave_sell(int prov, int count) {
    return sim ? scps_player_slave_sell(sim, prov, (long)count) != 0 : false;
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
    /* lot M — le SPREAD affiché (achat ×2 / vente ×1, or par âme) : la membrane cesse
     * de taire le prix que le drain débite. */
    int pb = 0, ps = 0;
    scps_slave_prices(sim, &pb, &ps);
    out["price_buy"]  = pb;
    out["price_sell"] = ps;
    return out;
}

/* ── UI-MONNAIE (2026-07-16) — dette/emprunt/banqueroute/fiscalité/prix : chaque verbe/
 * lecteur moteur EXISTAIT DÉJÀ (M8/M9) — seule cette façade manquait (TROUVAILLES.md). ── */
Dictionary ScpsWorld::country_debt(int country) {
    Dictionary d;
    d["to_class"] = 0.0; d["to_cs"] = 0.0; d["total"] = 0.0; d["taux"] = 0.0; d["due"] = 0.0;
    d["annual_revenue"] = 0.0; d["leverage"] = 0.0; d["available"] = 0.0;
    d["foreign_exposure"] = 0.0; d["foreign_room"] = 0.0;
    d["creditor"] = (int64_t)(-1); d["creditor_name"] = String();
    if (!sim) return d;
    ScpsDebt deb;
    scps_country_debt(sim, country, &deb);
    d["to_class"] = (double)deb.to_class;
    d["to_cs"] = (double)deb.to_cs;
    d["total"] = (double)deb.total;
    d["taux"] = (double)deb.taux;
    d["annual_revenue"] = (double)deb.annual_revenue;
    d["leverage"] = (double)deb.leverage;
    d["available"] = (double)deb.available;
    d["foreign_exposure"] = (double)deb.foreign_exposure;
    d["foreign_room"] = (double)deb.foreign_room;
    /* MONNAIE M14 — B7 : l'échéance RÉELLE (10 %/an du stock sous DEBT_FIXED, PAS total×taux). */
    d["due"] = (double)deb.due;
    d["creditor"] = (int64_t)deb.creditor;
    d["creditor_name"] = String::utf8(deb.creditor_name);
    return d;
}
Dictionary ScpsWorld::country_loan_quote(int debtor, int lender) {
    Dictionary d;
    d["montant_max"]=0.0; d["taux"]=0.0; d["lender_surplus"]=0.0;
    d["exposure"]=0.0; d["exposure_limit"]=0.0; d["portfolio_exposure"]=0.0;
    d["blocked_by_other_creditor"]=false;
    if (!sim) return d;
    ScpsStateLoanQuote q; scps_country_loan_quote(sim,debtor,lender,&q);
    d["montant_max"]=(double)q.montant_max;
    d["taux"]=(double)q.taux;
    d["lender_surplus"]=(double)q.lender_surplus;
    d["exposure"]=(double)q.exposure;
    d["exposure_limit"]=(double)q.exposure_limit;
    d["portfolio_exposure"]=(double)q.portfolio_exposure;
    d["blocked_by_other_creditor"]=(bool)(q.blocked_by_other_creditor!=0);
    return d;
}
Array ScpsWorld::country_fiscal_orders(int country) {
    Array a;
    if (!sim) return a;
    ScpsFiscalOrder fo[3];
    int n = scps_country_fiscal_orders(sim, country, fo, 3);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["taux"] = (double)fo[i].taux;
        d["satisfaction"] = fo[i].satisfaction;
        d["revenu_mois"] = fo[i].revenu_mois;
        a.push_back(d);
    }
    return a;
}
Array ScpsWorld::country_loan_capacity(int country) {
    Array a;
    if (!sim) return a;
    ScpsLoanCapacity lc[3];
    int n = scps_country_loan_capacity(sim, country, lc, 3);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["montant_max"] = (double)lc[i].montant_max;
        d["taux"] = (double)lc[i].taux;
        a.push_back(d);
    }
    return a;
}
bool ScpsWorld::player_borrow_class(int cls, float amount) {
    return sim ? scps_player_borrow_class(sim, cls, amount) != 0 : false;
}
int ScpsWorld::country_loan_request_target(int country) const {
    return sim ? scps_country_loan_request_target(sim, country) : -1;
}
String ScpsWorld::country_loan_status(int country) {
    if (!sim) return String();
    return String::utf8(scps_country_loan_status(sim, country));
}
bool ScpsWorld::player_request_loan(int target, float amount) {
    return sim ? scps_player_request_loan(sim, target, amount) != 0 : false;
}
bool ScpsWorld::player_repay(int amount) {
    return sim ? scps_player_repay(sim, amount) != 0 : false;
}
bool ScpsWorld::player_bankruptcy() {
    return sim ? scps_player_bankruptcy(sim) != 0 : false;
}
double ScpsWorld::country_price_level(int country) const {
    return sim ? scps_country_price_level(sim, country) : 1.0;
}
double ScpsWorld::world_price_index() const {
    return sim ? scps_world_price_index(sim) : 1.0;
}
float ScpsWorld::country_debase_frac(int country) const {
    return sim ? scps_country_debase_frac(sim, country) : 0.f;
}
float ScpsWorld::country_bankruptcy_scar(int country) const {
    return sim ? scps_country_bankruptcy_scar(sim, country) : 0.f;
}
float ScpsWorld::province_res_price(int province, int res_id) const {
    return sim ? scps_province_res_price(sim, province, res_id) : 0.f;
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

/* CLIMAT DU PEUPLE (createur d'empire) - entree de genese, meme statut que la culture. */
void ScpsWorld::set_player_climat(int climat) {
    scps_set_player_climat(climat);
}

void ScpsWorld::clear_player_culture() {
    scps_clear_player_culture();
}

void ScpsWorld::set_country_name(int cid, const String &name) {
    if (sim) scps_set_country_name(sim, cid, name.utf8().get_data());
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
int ScpsWorld::religion_scholar_expected(int cid){ return sim ? scps_religion_scholar_expected(sim, cid) : -1; }
String ScpsWorld::scholar_role_name(int role) const { return String::utf8(scps_scholar_role_name(role)); }
String ScpsWorld::scholar_role_ability(int role) const { return String::utf8(scps_scholar_role_ability(role)); }
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

Array ScpsWorld::sea_paths() {
    Array a;
    if (!sim) return a;
    int np = scps_sea_lanes_build(sim);
    static const int MAXPT = 1400;
    static ScpsRoadPt pts[MAXPT];
    for (int i = 0; i < np; i++) {
        int open = 0, choke = -1, ra = -1, rb = -1;
        int n = scps_sea_lane_path(sim, i, pts, MAXPT, &open, &choke, &ra, &rb);
        if (n < 2) continue;
        PackedVector2Array pv;
        pv.resize(n);
        for (int k = 0; k < n; k++) pv.set(k, Vector2(pts[k].x, pts[k].y));
        Dictionary d;
        d["points"] = pv;
        d["open"]   = open;
        d["choke"]  = choke;
        d["ra"]     = ra;
        d["rb"]     = rb;
        a.push_back(d);
    }
    return a;
}

Dictionary ScpsWorld::sea_travel(int target_region) {
    Dictionary d;
    if (!sim) return d;
    ScpsSeaTravel st;
    if (!scps_sea_travel(sim, target_region, &st)) return d;
    d["possible"]        = st.possible;
    d["days"]            = st.days;
    d["port_region"]     = st.port_region;
    d["transports_need"] = st.transports_need;
    d["transports_free"] = st.transports_free;
    d["blocked"]         = st.blocked;
    return d;
}
