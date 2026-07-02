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
#include <godot_cpp/variant/packed_color_array.hpp>   /* political_image(pal) */

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
    Ref<Image> political_image(PackedColorArray pal);   /* LAVIS : owner/cellule teinté par pal[pays] (RGBA, transparent hors territoire) */

    /* nombres tangibles (membrane) */
    int     year() const;
    int     player() const;
    int     country_count() const;
    int     country_province_count(int country) const;   /* provinces possédées (topbar EU4) */
    int     region_count() const;
    int64_t world_pop() const;
    int64_t country_pop(int country) const;
    double  country_gold(int country) const;
    int     country_role(int country) const;          /* 0 joueur · 1 IA · 2 cité-état · 3 vierge · 4 libre · -1 */

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
    Dictionary province_agitation(int province);      /* MODIFICATEURS : { value:int, causes:[{cause,delta,decay}] } */
    Array      province_buildings(int province);      /* MANUFACTURES : [{nom, niveau, ouvriers}] */
    Array      province_log(int province);            /* JOURNAL : [{year, label, sign}] (récent en tête) */
    Dictionary province_classes(int province);        /* barre empilée des classes */
    Dictionary province_capitale(int province);       /* ossature de capitale */

    /* SIDEBAR : agrégats PAYS (read-only) */
    Dictionary country_demo(int country);             /* classes + satisfaction */
    Array      country_stocks(int country);           /* biens : stock · net · couverture · marché */
    Array      country_relations(int country);        /* diplomatie : statut + opinion #26 par pays */
    Dictionary diplo_options(int target);             /* §3 : légalité des verbes diplo contre `target` (boutons grisés) */
    Dictionary country_army(int country);             /* mobilisation + flotte */
    Dictionary country_trade(int country);            /* commerce : routes · or · partenaires */
    Array      country_council(int country);          /* conseil : 3 sièges */

    /* CONSTRUCTION : roster militaire (22 unités) + édifices (boutons + survol) */
    Array      unit_roster(int country);              /* unités : nom·classe·coût·éthos·contres·recrutable */
    Array      building_roster(int country);          /* édifices : nom·coût matériaux·or·jours·débloqué */
    Dictionary tech_info();                           /* arbre du joueur : points · thèmes/fonctions · présage · crise% · métab% */
    Array      tech_nodes();                           /* arbre du joueur : nœuds (quadrant·tier·état·coût·effet) */
    Array      heritage_access();                      /* barre de métabolisation : par héritage tier 0-3 + part digérée */
    Array      tunables();                             /* MODTOOLS : registre des tunables (nom·valeur·défaut·surchargé) */
    void       tune_set(const godot::String &nom, double value);  /* MODTOOLS : surcharge LIVE d'un tunable */
    Array      country_budget(int country);            /* budget : postes de flux de l'année (signés) */
    Dictionary budget_summary(int country);            /* budget : or · revenus · dépenses · net · crédit · prêteur */
    Dictionary mission_info(int country);              /* mission décennale : texte · récompense · année */

    /* ACTIONS du joueur (la main humaine — mêmes actionneurs que l'IA) */
    bool       player_build(int edifice, int region); /* enfile un chantier (region<0 ⇒ capitale) ; false = file pleine/hors-domaine */
    int        player_recruit(int unit);              /* enfile 1 paquet à lever ; 1 = mis en file, 0 = refus d'enfilement */
    void       player_set_levy(int level);            /* enfile le réglage de la jauge de levée 0-3 */
    int        player_research(int tech);             /* fixe la cible de tech (file de 1) ; tech<0 ⇒ annule ; 1 = mis en file */
    Dictionary research_status();                     /* { target:int(-1=aucune), progress:float[0..1] } */
    Dictionary age_state();                           /* §7 : { age:int(-1=aucun), engaged:bool, name:String } */
    bool       player_age_engage();                   /* §7 : engager l'âge courant (une fois par âge) */

    /* §3 — VERBES DIPLO du joueur (proposer → ai_consider_offer évalue) ; true = ordre enfilé */
    bool       player_declare_war(int target);        /* déclarer la guerre */
    bool       player_make_peace(int target);         /* offre de paix blanche */
    bool       player_offer_alliance(int target);     /* proposer une alliance */
    bool       player_offer_pact(int target);         /* proposer un pacte de commerce */
    bool       player_embargo(int target, bool on);   /* poser/lever un embargo */

    /* ALLOCATION DE MAIN-D'ŒUVRE (onglet province) — lire les puits + régler les poids.
     * region_alloc renvoie { region, on:bool, pool:float, sinks:[{kind,id,name,output,in_name,
     * alt_name,weight,pct,workers,closed,input}] }. Les verbes ENFILENT (revalidé au drain). */
    Dictionary region_alloc(int region);
    bool       player_alloc_raw(int region, int resource, int weight);
    bool       player_alloc_bld(int region, int bld_type, int weight);  /* weight 0 = fermé */
    bool       player_alloc_input(int region, int bld_type, int input);
    bool       player_alloc_auto(int region);          /* retour au split AUTO */

    /* CRÉATEUR DE CULTURE (façon Stellaris) — listes + validation + aperçu + composition.
     * Membrane : des MOTS et des SIGNES (pas de levier brut). Pur (aucun sim) → utilisable
     * AVANT generate() ; set_player_culture grave la compo À la prochaine generate(). */
    Array      heritage_list();                        /* [{id,nom,sphere,exemple}] */
    Array      ethos_list();                           /* [{id,nom,epithete,hint}] */
    Array      tradition_list();                       /* [{id,nom,axe,axe_nom,rang,hover}] */
    bool       culture_validate(int t0, int t1, int t2);    /* 1maj+1min+1déf, une/axe, pas d'antonyme */
    Array      culture_preview(int t0, int t1, int t2);     /* [{nom,signe}] — aperçu des leviers */
    String     culture_name(int heritage, int seed);        /* ethnonyme façon Stellaris (aperçu live) */
    bool       set_empire_culture(int slot, int heritage, int ethos, int t0, int t1, int t2); /* slot 0=joueur, 1..N IA */
    bool       set_player_culture(int heritage, int ethos, int t0, int t1, int t2);  /* raccourci slot 0 */
    void       clear_player_culture();                      /* efface TOUS les slots */

    /* PARAMÈTRES DE GÉNÉRATION (sliders « Nouvelle partie ») — POD WorldParams en Dictionary. */
    Dictionary worldparams_default(int seed);               /* défauts pour pré-remplir les sliders */
    void       worldgen_set(Dictionary p);                  /* override la prochaine generate() */
    void       worldgen_clear();

    /* RELIGION (P5) — listes, fondation, schisme, lecture (membrane). */
    Array      religion_pole_list();                        /* [{id,nom,axe,axe_nom,tip}] (16) */
    Array      credo_list();                                /* [{id,nom}] (3) */
    bool       religion_picks_valid(int p0, int p1, int p2);
    int        religion_found(int cid, int credo, int t0, int t1, int t2);   /* id religion / -1 */
    int        religion_eligible(int cid);                  /* 0 aucune · 1 RUPTURE · 2 DERIVE */
    Dictionary religion_schism(int cid, int slot_a, int pole_a, int slot_b, int pole_b, int new_credo); /* {child,flipped} */
    int        religion_of_country(int cid);
    int        religion_of_region(int region);
    int        religion_recruit_scholar(int cid, int region);   /* ScholarRole / -1 */
    int        religion_scholar_role(int cid);
    String     religion_name(int cid);
    int        religion_founding_ready(int cid);   /* 1 = édifice religieux bâti + pas de foi → créateur */
    int        religion_cap();                      /* ⌈n_empires/3⌉ */
    int        religion_can_found();                /* 1 = sous le plafond (créer) ; 0 = rallier une foi */

    /* SAUVEGARDE (« Charger ») — 3 emplacements (1..3) */
    bool       save_game(int slot);                         /* true = écrit */
    int        load_game(int slot);                         /* 0 ok · 1 absent/corrompu · 2 ère antérieure */
    Array      save_slots();                                /* [{used:bool, year:int, line:String}] (slots 1..3) */

    /* TRACÉS DE CARTE : rivières (Vector3 par point : x · y · angle rad) */
    Array      river_points();
    /* rivières STRUCTURÉES par fleuve : Array[Dictionary{points:PackedVector2Array, flow:float}]
     * — pour un rendu STRATÉGIQUE (l'hôte trie par débit et ne trace que les majeurs). */
    Array      river_paths();

    /* FRONTIÈRES : segments d'arête (PackedVector2Array, 2 points/segment) au niveau
     * 0=province · 1=région · 2=pays. Port du balayage bseg de viewer.c. */
    PackedVector2Array border_segments(int level);
    Dictionary         border_segments_col(int level);   /* segments + owner + normale (dégradé int.→ext.) */
    int                country_ethos(int c) const;        /* éthos dominant 0..5 (-1) — inline ordre↔chaos */
    int                country_heritage(int c) const;      /* héritage/culture 0..5 (-1) — outline par culture */
    int                country_capital_region(int c) const; /* région-capitale (-1) */
    Dictionary         region_border_segments(int region);  /* contour d'une région {pts,nrm} — liseré capitale */
    Dictionary         province_border_segments(int prov);   /* contour d'une PROVINCE {pts,nrm} — surbrillance de sélection */

    /* ROUTES : réseau à jonctions (A* « routes attirent routes »). Array de
     * Dictionary { points: PackedVector2Array (centres de cellule) · level: int
     * 0=artère/1=desserte/2=mineure }. Port de roads_ensure_cache de viewer.c. */
    Array road_paths();
};

} // namespace godot
#endif // SCPS_SIM_NODE_H
