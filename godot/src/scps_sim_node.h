#ifndef SCPS_SIM_NODE_H
#define SCPS_SIM_NODE_H
/*
 * ScpsSim — le node Godot qui EXPOSE la façade C `scps_api` à GDScript.
 * Le moteur reste 100 % C (déterminisme). Ce node ne fait que TRADUIRE :
 * octets de carte → Image, nombres tangibles → int/float, verbes → appels C.
 * Aucune logique de simulation ici — c'est la règle d'or de la bascule.
 */
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/dictionary.hpp>

extern "C" {
#include "scps_api.h"
}

namespace godot {

// NB : la classe Godot s'appelle ScpsWorld (pas ScpsSim) — le type C opaque de la
// façade est, lui, `::ScpsSim`. Deux noms distincts pour éviter l'ambiguïté C/C++.
class ScpsWorld : public RefCounted {
    GDCLASS(ScpsWorld, RefCounted)

    ::ScpsSim *sim = nullptr;   /* le handle moteur (C, opaque) */

protected:
    static void _bind_methods();

public:
    ScpsWorld();
    ~ScpsWorld();

    /* cycle de vie */
    void  generate(int seed);
    void  advance_days(int days);

    /* carte */
    int   map_w() const;
    int   map_h() const;
    Ref<Image> map_image(int mode, int selected_prov);  /* render_map → Image RGBA8 (sel. surlignée) */
    Ref<Image> layer_image(int layer);  /* couche brute → Image L8 (shaders) */

    /* nombres tangibles (membrane) */
    int     year() const;
    int     player() const;
    int     country_count() const;
    int     region_count() const;
    int64_t world_pop() const;
    int64_t country_pop(int country) const;
    double  country_gold(int country) const;

    /* par région */
    int     region_owner(int region) const;
    int64_t region_pop(int region) const;
    bool    region_colonized(int region) const;
    Vector2 region_centroid(int region) const;   /* (-1,-1) si vide */

    /* PICKING & READOUTS (la membrane → panneaux) */
    int        province_at(int x, int y) const;       /* cellule monde → province (-1) */
    int        province_region(int province) const;   /* province → région (-1) */
    Dictionary province_info(int province);           /* mots + nombres (la membrane) */
    Dictionary country_info(int country);             /* mots + nombres (la membrane) */

    /* ACTEURS SUR LA CARTE (Phase 3) */
    Dictionary army_info(int country);                /* armée de campagne (vide si inactive) */
    int        region_tier(int region) const;         /* tier de ville 0-5 (-1 si non colonisée) */
};

} // namespace godot
#endif // SCPS_SIM_NODE_H
