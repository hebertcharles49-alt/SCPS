# SCPS — Modding / devtools

Le moteur SCPS est **statiquement dimensionné** (pour le déterminisme + un save byte-identique).
On ne peut donc PAS ajouter une ressource « à chaud » — mais on peut **éditer toutes les valeurs
sans recompiler**, et **ajouter du contenu** en éditant un fichier puis en recompilant une fois.

Trois mécanismes d'override, tous sur le même principe : **défaut compilé + surcharge fichier/env,
opt-in**. Sans surcharge ⇒ le jeu est *vanilla* (golden/déterminisme intacts).

---

## 1. Valeurs de calibrage — `SCPS_TUNE` (≈168 constantes, zéro recompile)

Caps éco, taux, poids IA, croissance pop, modificateurs provinciaux, endgame, échelle de coût
tech, etc. — tout le « registre J ».

```sh
./chronicle --tunables                 # liste : nom · défaut compilé · valeur active
SCPS_TUNE="POP_R_BASE=0.0173,AI_METAB_RES_W=1.5" ./chronicle 9 1 200
```

Le save enregistre une empreinte des surcharges actives : charger une partie sous d'AUTRES
règles **avertit** (replays / graines partagées invalides). C'est voulu : un monde modé n'est
plus « vanilla ».

## 2. Valeurs ÉCO en table — `SCPS_MODS` (prix & rendements, zéro recompile)

Les **prix de base** et les **rendements d'extraction** par ressource (qui ne sont pas des
tunables scalaires mais des TABLES) s'éditent par fichier TSV **name-keyed** (robuste au
réordonnancement d'enum).

```sh
./chronicle --dump-data > mods.tsv     # écrit le point de départ éditable (toutes les ressources)
# éditer mods.tsv : « ressource <TAB> base_price <TAB> extract_yield » (une ligne/ressource)
#   - séparateur = TABULATION (les noms ont des espaces : « Eau de vie »)
#   - extract_yield est optionnel ; '#' ou ligne vide = ignoré ; nom inconnu = ignoré
SCPS_MODS=mods.tsv ./chronicle 9 1 200 # charge les surcharges (message [mods] sur stderr)
```

Côté jeu (Godot / viewer) : poser `SCPS_MODS` dans l'environnement avant de lancer. Les valeurs
sont relues à chaque génération de monde. Comme `SCPS_TUNE`, un monde modé n'est plus rejouable
contre le golden vanilla.

## 3. Strings face-joueur — `scps_lang.txt` (≈358 textes, F4 à chaud)

Tout le texte UI (bandes, labels, hovers, tooltips, tutoriels). Display-only (aucun impact moteur).

```sh
./scps_viewer --dump-lang              # écrit scps_lang.txt (toutes les chaînes éditables)
# éditer : « STR_ID <TAB> texte » — un fichier = une langue
# en jeu : F4 recharge à chaud
```

## 4. Sprites / shaders / paramètres de monde — éditeur Godot

- **Sprites** : `godot/project/assets/scps/pack/**.png` — remplacer les PNG directement.
- **Shaders** : `godot/project/shaders/*.gdshader` (ex. l'eau procédurale) — édition visuelle.
- **Paramètres de génération** (taille, âge, température, humidité, nb d'empires…) : sliders de
  l'écran Nouvelle partie (façade `scps_worldgen_set`, régénère sans recompiler).

---

## Ajouter du CONTENU (ressource / bâtiment / tech / unité) — 1 recompile

Ces objets sont des `enum` + des tableaux `[*_COUNT]` sérialisés par `sizeof` ⇒ en ajouter un
**bump le `SAVE_VERSION` + re-baseline le golden + recompile** (le prix du déterminisme). Règle
d'or : **toujours APPENDRE en queue d'enum** (jamais insérer au milieu — ça décale les indices de
save). Emplacements :

| Contenu     | Enum                         | Table                                   |
|-------------|------------------------------|-----------------------------------------|
| Ressource   | `Resource` (scps_types.h)    | `BASE_PRICE`, `EXTRACT_YIELD`, recettes (scps_econ.c) |
| Bâtiment    | `BuildingType` (scps_econ.c) | `RECIPE[]` (scps_econ.c)                 |
| Tech        | `TechId` (scps_tech.h)       | `NODES[]` (scps_tech.c)                  |
| Unité       | `UnitType` (scps_army.h)     | `UNITS[]` (scps_army.c)                  |

> **À venir (devtools, par paliers)** : extension de `SCPS_MODS` aux **recettes / coûts-deltas tech /
> stats d'unité** ; un **codegen** (manifeste TSV → enums + tables générées) pour ajouter du contenu
> en éditant un fichier ; un **panneau dev Godot** (sliders live sur les tunables). Le palier 1
> (valeurs éco) est en place.

## Disciplines (à respecter pour tout devtool)

- L'effet passe par les **entrées du moteur** (jamais un bonus plat) — `tune_f`/`SCPS_MODS` modifient
  des coefficients que le moteur LIT déjà.
- **Membrane** : un outil GUI appelle la **façade** `scps_api` (mots + nombres), jamais les structs
  moteur directement.
- **Déterminisme** : une partie modée n'est rejouable QUE sous les mêmes surcharges ; le vanilla
  (aucune surcharge) reste byte-identique (golden).
