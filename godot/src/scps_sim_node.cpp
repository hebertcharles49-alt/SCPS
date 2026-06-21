/*
 * scps_sim_node.cpp — implémentation du node Godot (voir .h).
 * Chaque méthode est un mince passe-plat vers scps_api (la façade C).
 */
#include "scps_sim_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

void ScpsWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "seed"),        &ScpsWorld::generate);
    ClassDB::bind_method(D_METHOD("advance_days", "days"),    &ScpsWorld::advance_days);
    ClassDB::bind_method(D_METHOD("map_w"),                   &ScpsWorld::map_w);
    ClassDB::bind_method(D_METHOD("map_h"),                   &ScpsWorld::map_h);
    ClassDB::bind_method(D_METHOD("map_image", "mode", "selected_prov"), &ScpsWorld::map_image, DEFVAL(-1));
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

    ClassDB::bind_method(D_METHOD("province_at", "x", "y"),          &ScpsWorld::province_at);
    ClassDB::bind_method(D_METHOD("province_region", "province"),    &ScpsWorld::province_region);
    ClassDB::bind_method(D_METHOD("province_info", "province"),      &ScpsWorld::province_info);
    ClassDB::bind_method(D_METHOD("country_info", "country"),        &ScpsWorld::country_info);
    ClassDB::bind_method(D_METHOD("army_info", "country"),           &ScpsWorld::army_info);
    ClassDB::bind_method(D_METHOD("region_tier", "region"),          &ScpsWorld::region_tier);
    ClassDB::bind_method(D_METHOD("region_settle_group", "region"),  &ScpsWorld::region_settle_group);
    ClassDB::bind_method(D_METHOD("endgame_info"),                   &ScpsWorld::endgame_info);
    ClassDB::bind_method(D_METHOD("region_sunken", "region"),        &ScpsWorld::region_sunken);
    ClassDB::bind_method(D_METHOD("province_groups", "province"),    &ScpsWorld::province_groups);
    ClassDB::bind_method(D_METHOD("province_income", "province"),    &ScpsWorld::province_income);
    ClassDB::bind_method(D_METHOD("province_classes", "province"),   &ScpsWorld::province_classes);
    ClassDB::bind_method(D_METHOD("province_capitale", "province"),  &ScpsWorld::province_capitale);
    ClassDB::bind_method(D_METHOD("country_demo", "country"),        &ScpsWorld::country_demo);
    ClassDB::bind_method(D_METHOD("country_stocks", "country"),      &ScpsWorld::country_stocks);
    ClassDB::bind_method(D_METHOD("country_relations", "country"),   &ScpsWorld::country_relations);
    ClassDB::bind_method(D_METHOD("country_army", "country"),        &ScpsWorld::country_army);
    ClassDB::bind_method(D_METHOD("country_trade", "country"),       &ScpsWorld::country_trade);
    ClassDB::bind_method(D_METHOD("country_council", "country"),     &ScpsWorld::country_council);
    ClassDB::bind_method(D_METHOD("unit_roster", "country"),         &ScpsWorld::unit_roster);
    ClassDB::bind_method(D_METHOD("building_roster", "country"),     &ScpsWorld::building_roster);
    ClassDB::bind_method(D_METHOD("player_build", "edifice"),        &ScpsWorld::player_build);
    ClassDB::bind_method(D_METHOD("player_recruit", "unit"),         &ScpsWorld::player_recruit);
    ClassDB::bind_method(D_METHOD("player_set_levy", "level"),       &ScpsWorld::player_set_levy);
    ClassDB::bind_method(D_METHOD("river_points"),                   &ScpsWorld::river_points);
    ClassDB::bind_method(D_METHOD("river_paths"),                    &ScpsWorld::river_paths);
    ClassDB::bind_method(D_METHOD("border_segments", "level"),       &ScpsWorld::border_segments);
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
    d["race"]           = String::utf8(p.race);
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
    return d;
}

int ScpsWorld::region_sunken(int region) const { return scps_region_sunken(sim, region); }

Array ScpsWorld::province_groups(int province) {
    Array a;
    ScpsGroup g[8];
    int n = scps_province_groups(sim, province, g, 8);
    for (int i = 0; i < n; i++) {
        Dictionary d;
        d["race"]     = String::utf8(g[i].race);
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
        a.push_back(d);
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
        a.push_back(d);
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

bool ScpsWorld::player_build(int edifice) {
    return sim ? scps_player_build(sim, edifice) != 0 : false;
}

int ScpsWorld::player_recruit(int unit) {
    return sim ? (int)scps_player_recruit(sim, unit) : 0;
}

void ScpsWorld::player_set_levy(int level) {
    if (sim) scps_player_set_levy(sim, level);
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
