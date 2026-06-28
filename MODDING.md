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

**En jeu (Godot), touche `F10`** : le panneau dev liste les ~168 tunables (filtrable) et les
édite **EN DIRECT** — l'effet apparaît au tick suivant, là où le moteur relit la valeur. Même
empreinte de surcharge dans le save. (`SCPS_TUNE` au lancement reste le canal pour un run
headless / reproductible.)

## 2. Valeurs en TABLE — `SCPS_MODS` (prix, recettes, tech, unités ; zéro recompile)

Les valeurs qui sont des TABLES (pas des tunables scalaires) s'éditent par un fichier
**TAG-keyed**, TAB-séparé (les noms ont des espaces : « Eau de vie »), **name-keyed** (robuste
au réordonnancement d'enum). Une ligne = un objet, préfixée par sa table :

| TAG | colonnes | ce que ça change |
|-----|----------|------------------|
| `price`     | `<ressource> <base_price> <extract_yield>` | prix de base + rendement d'extraction |
| `recipe`    | `<bâtiment> <labor> <qout>`                | main-d'œuvre/lot + sortie/lot (les leviers « 100 emplois → X ») |
| `basecost`  | `<tier 0-5> <coût>`                        | coût de recherche par tier (la courbe) |
| `techbonus` | `<tech> <prod_pct> <eff_pct>`              | bonus de production/efficacité d'une tech |
| `unit`      | `<unité> <discipline> <moral> <mvt> <cmd>` | stats d'unité (combat) |

```sh
./chronicle --dump-data > mods.tsv     # écrit le point de départ éditable (TOUTES les tables)
# éditer mods.tsv (séparateur = TABULATION ; '#'/vide ignoré ; nom/tag inconnu ignoré)
SCPS_MODS=mods.tsv ./chronicle 9 1 200 # charge (messages [mods] éco/tech/unités sur stderr)
```

Côté jeu (Godot / viewer) : poser `SCPS_MODS` dans l'environnement avant de lancer (chargé à la
génération de monde / au démarrage). Comme `SCPS_TUNE`, un monde modé n'est plus rejouable contre
le golden vanilla. *(Les types d'intrants/sorties d'une recette + la structure des nœuds restent
du domaine du contenu : cf. §Ajouter du contenu.)*

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

### Générateur de boilerplate — `tools/gen_content.py` (ressources)

Pour ne RIEN oublier (l'enum + toutes les tables), un manifeste TSV → les lignes EXACTES à coller :

```sh
python tools/gen_content.py tools/content_example.tsv   # imprime les blocs + la checklist
# format : resource <TAB> <Nom> <TAB> <RES_ENUM> <TAB> price=.. <TAB> yield=.. <TAB> food=0|1
```

L'outil ne modifie PAS le code (il imprime) ; tu colles aux endroits indiqués, tu bumps
`SAVE_VERSION`, puis `make golden-update && make test`. Les bâtiments/unités/techs suivent le même
motif (enum + table — cf. le tableau ci-dessus) ; non scaffoldés (ils touchent des recettes/prereqs
structurels — à faire à la main avec le tableau).

Les **4 paliers** sont en place : VALEURS (1 : éco · 2 : recettes/tech/unités, par fichier
`SCPS_MODS`), CONTENU (3 : `gen_content.py`), et l'ÉDITION LIVE (4 : **panneau dev Godot, `F10`** —
les ~168 tunables réglables en jeu, cf. §1).

## Disciplines (à respecter pour tout devtool)

- L'effet passe par les **entrées du moteur** (jamais un bonus plat) — `tune_f`/`SCPS_MODS` modifient
  des coefficients que le moteur LIT déjà.
- **Membrane** : un outil GUI appelle la **façade** `scps_api` (mots + nombres), jamais les structs
  moteur directement.
- **Déterminisme** : une partie modée n'est rejouable QUE sous les mêmes surcharges ; le vanilla
  (aucune surcharge) reste byte-identique (golden).
