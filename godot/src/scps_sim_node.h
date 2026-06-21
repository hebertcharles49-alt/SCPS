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
    int        region_settle_group(int region) const; /* groupe de sprite settlement 0-5 (-1) */

    /* ENDGAME §27 (Phase 4) */
    Dictionary endgame_info();                        /* entropie · augure · fin · épicentre */
    int        region_sunken(int region) const;       /* 0 non · 1 programmée · 2 engloutie */

    /* DÉTAIL DE PROVINCE (port fidèle viewer.c) */
    Array      province_groups(int province);         /* camemberts culture/idéologie */
    Array      province_income(int province);         /* RESSOURCES / PRODUCTION */
    Dictionary province_classes(int province);        /* barre empilée des classes */
    Dictionary province_capitale(int province);       /* ossature de capitale */

    /* SIDEBAR : agrégats PAYS (read-only) */
    Dictionary country_demo(int country);             /* classes + satisfaction */
    Array      country_stocks(int country);           /* biens : stock · net · couverture · marché */
    Array      country_relations(int country);        /* diplomatie : statut par pays */
    Dictionary country_army(int country);             /* mobilisation + flotte */
    Dictionary country_trade(int country);            /* commerce : routes · or · partenaires */
    Array      country_council(int country);          /* conseil : 3 sièges */

    /* CONSTRUCTION : roster militaire (22 unités) + édifices (boutons + survol) */
    Array      unit_roster(int country);              /* unités : nom·classe·coût·éthos·contres·recrutable */
    Array      building_roster(int country);          /* édifices : nom·coût matériaux·or·jours·débloqué */

    /* ACTIONS du joueur (la main humaine — mêmes actionneurs que l'IA) */
    bool       player_build(int edifice);             /* met un chantier en file (payé) ; false = refus */
    int        player_recruit(int unit);              /* lève 1 paquet ; renvoie les paquets levés (0 = gate) */
    void       player_set_levy(int level);            /* jauge de levée 0-3 */

    /* TRACÉS DE CARTE : rivières (Vector3 par point : x · y · angle rad) */
    Array      river_points();
    /* rivières STRUCTURÉES par fleuve : Array[Dictionary{points:PackedVector2Array, flow:float}]
     * — pour un rendu STRATÉGIQUE (l'hôte trie par débit et ne trace que les majeurs). */
    Array      river_paths();

    /* FRONTIÈRES : segments d'arête (PackedVector2Array, 2 points/segment) au niveau
     * 0=province · 1=région · 2=pays. Port du balayage bseg de viewer.c. */
    PackedVector2Array border_segments(int level);

    /* ROUTES : réseau à jonctions (A* « routes attirent routes »). Array de
     * Dictionary { points: PackedVector2Array (centres de cellule) · level: int
     * 0=artère/1=desserte/2=mineure }. Port de roads_ensure_cache de viewer.c. */
    Array road_paths();
};

} // namespace godot
#endif // SCPS_SIM_NODE_H
