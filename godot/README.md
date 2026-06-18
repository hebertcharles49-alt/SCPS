# SCPS × Godot — le spike

Le **front-end Godot** qui pilote le **moteur SCPS (C99) inchangé**. Le moteur
CALCULE (déterministe, byte-reproductible) ; Godot AFFICHE et SAISIT. La frontière
est la façade C `scps_api` — la membrane, version binding.

```
godot/
  src/                  binding C++ (godot-cpp) : la classe Godot `ScpsWorld`
    scps_sim_node.{h,cpp}  passe-plat vers ../scps/scps_api.{h,c}
    register_types.{h,cpp} point d'entrée GDExtension
  SConstruct            bâtit le MOTEUR C + la façade + le binding → libscps.<…>.so
  project/              le projet Godot 4
    project.godot · Main.tscn · main.gd · water.gdshader · scps.gdextension
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

Le spike : génère le monde (graine 9), affiche le terrain (`render_map` → texture),
applique le **shader d'eau** (`water.gdshader`, qui anime la mer depuis la couche
SEA — la continuité eau↔asset en shader, pas en blit), et avance d'un mois toutes
les 0,2 s (la pop monte dans le HUD).

## État (spike)

- ✅ façade C `scps_api` testée sans Godot : `make scps_api_demo` (9/9, REPRODUCTIBLE).
- ✅ binding C++ compile contre godot-cpp 4.3 ; `libscps.so` lie moteur+façade+binding.
- ⏳ `advance_days` roule pour l'instant la **colonne économique** (cf. la note
  fidélité dans `scps_api.h`). Le tick PLEIN (IA/guerre/diplo/endgame, fidèle au
  hash de chronicle) viendra de l'extraction `chronicle::sim_day → scps_sim` —
  **sans changer la surface de l'API**.

Prochaines marches : panneaux readout (province/pays) en nodes Godot · sprites
d'armées via `region_centroid` + état campagne · `TileMap` autotiling pour des
côtes douces · sauvegarde via le format C existant.
