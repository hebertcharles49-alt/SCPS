# SCPS × Godot — le front-end

Le **front-end Godot** qui pilote le **moteur SCPS (C99) inchangé**. Le moteur
CALCULE (déterministe, byte-reproductible) ; Godot AFFICHE et SAISIT. La frontière
est la façade C `scps_api` — la membrane, version binding.

**Plan d'ensemble : [`ROADMAP.md`](ROADMAP.md)** (les phases du front ; le moteur
reste de côté tant qu'on bâtit la présentation).

```
godot/
  ROADMAP.md            les phases du front-end (ce qui demande ou non le moteur)
  src/                  binding C++ (godot-cpp) : la classe Godot `ScpsWorld`
    scps_sim_node.{h,cpp}  passe-plat vers ../scps/scps_api.{h,c}
    register_types.{h,cpp} point d'entrée GDExtension
  SConstruct            bâtit le MOTEUR C + la façade + le binding → libscps.<…>.so
  project/              le projet Godot 4 (architecture, pas un spike à plat)
    project.godot          autoload `Sim` + scène principale
    scps.gdextension       déclare la lib native
    autoload/sim.gd        SINGLETON : détient ScpsWorld, cadence le temps, signaux
    main/                  Main.tscn + main.gd : compose carte + UI
    map/map_view.gd        terrain (render_map) + shader d'eau + Camera2D (zoom/pan)
    ui/topbar.gd           année · pop · vitesse
    shaders/water.gdshader la continuité eau↔asset, en shader
```

## Ce que le moteur expose (façade `../scps/scps_api.h`)

`ScpsWorld` (GDScript) → `scps_api` (C) :
`generate(seed)` · `advance_days(n)` · `map_image(mode)` (render_map → `Image`) ·
`layer_image(layer)` (height/sea/biome/coast → `Image` L8, pour shaders) ·
`year/player/country_count/region_count/world_pop/country_pop/country_gold` ·
`region_owner/pop/colonized/centroid`.

> **Règle d'or :** pas une ligne de simulation côté Godot/GDScript. Tout calcul
> reste en C → le déterminisme survit. Godot lit des octets et des nombres
> tangibles (la membrane), envoie des verbes.

## Bâtir

Prérequis : `scons`, un compilo C/C++, et **godot-cpp 4.x** à côté :

```bash
cd godot
git clone --depth 1 -b 4.3 https://github.com/godotengine/godot-cpp
scons -j$(nproc)                  # debug   → project/bin/libscps.<plateforme>.template_debug.<arch>.so
scons target=template_release     # release
```

Le `SConstruct` compile les **mêmes** sources `../scps/scps_*.c` que le `make` du
moteur (la liste `CHRONICLE_OBJS` sans chronicle/viewer, + `scps_render` +
`scps_api`) et les lie au binding godot-cpp en UNE bibliothèque partagée.

## Lancer

```bash
godot --path project          # ou ouvrir project/ dans l'éditeur Godot 4.3+
```

Au lancement (`autoload/sim.gd` → `main/main.gd`) : `Sim` génère le monde
(graine 9), `MapView` affiche le terrain (`render_map` → texture) sous le
**shader d'eau** (anime la mer depuis la couche SEA — continuité eau↔asset en
shader, pas en blit) avec caméra zoom/pan, et `Topbar` montre année · pop · pays.
Le temps avance selon la vitesse (clic sur le bouton vitesse). Si `libscps` n'est
pas bâtie, `Sim` le dit dans la console (pas de crash).

## État

- ✅ **façade C `scps_api`** testée sans Godot : `make scps_api_demo` (9/9, REPRODUCTIBLE).
- ✅ **binding** C++ compile contre godot-cpp 4.3 ; `libscps.so` lie moteur+façade+binding.
- ✅ **Phase 1 (carte vivante)** échafaudée : autoload `Sim`, `MapView` (terrain +
  shader + caméra + modes de carte), `Topbar`. **N'utilise que la façade actuelle —
  aucune touche moteur.** ⚠ écrit sans runtime Godot ici : à vérifier à l'ouverture.
- ⏳ `advance_days` roule pour l'instant la **colonne économique** (cf. note fidélité
  dans `scps_api.h`) ; le tick PLEIN viendra de `chronicle::sim_day → scps_sim`,
  **sans changer la surface de l'API**.

**La suite est dans [`ROADMAP.md`](ROADMAP.md)** (Phase 2 : panneaux readout, dès
qu'on rouvre le moteur pour 3-4 getters).
