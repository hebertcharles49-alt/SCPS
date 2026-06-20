# Pipeline graphique ISO — manifeste des assets

> Cible : **remplacer tout le rendu du monde par des composantes iso**, à commencer par les
> **tuiles iso de terrain** qui *couvrent le monde* après le worldgen. Ce document est le
> **contrat** : produis les assets à cette spec, ils se branchent sans toucher au code.
>
> Philosophie (comme `ui/uikit.gd`) : **chargement RUNTIME des PNG** (pas de dépendance au
> système d'import de l'éditeur Godot) → vérifiable en headless. Pas de `.tres`/TileSet éditeur.

---

## 1. Tuile iso canonique

- **Empreinte : 256 × 128 px**, losange iso **2:1** (ratio largeur:hauteur = 2:1).
- **PNG RGBA**, transparent hors du losange. Le **« sol »** (la face du dessus) occupe le losange
  plein ; un éventuel **débord vertical** (falaise/épaisseur de tuile) tient dans le **haut** du
  cadre 256×128 (zone au-dessus du losange) — il recouvrira la tuile arrière au tri de profondeur.
- **Repère** : centre du losange = centre de l'image (128, 64). Les 4 **sommets** :
  - HAUT (nord) = (128, 0) · DROITE (est) = (256, 64) · BAS (sud) = (128, 128) · GAUCHE (ouest) = (0, 64).
- **Pas de magenta** (RGBA direct ; le keying magenta reste réservé aux vieux atlas BMP).

Le moteur place la tuile : `tile(col,row)` → écran `((col−row)·128, (col+row)·64)` (demi-empreinte),
**YSort** par profondeur `col+row` (arrière→avant). La caméra fait le zoom — ne te soucie pas de la
taille à l'écran, seulement du ratio 2:1 et du repère ci-dessus. Une tuile couvre `TILE_K` cellules
monde (réglable côté code) ; l'art reste indépendant de `TILE_K`.

---

## 2. Les 25 biomes (ordre moteur = index)

| idx | clé              | famille    | prio | idx | clé           | famille   | prio |
|----:|------------------|------------|-----:|----:|---------------|-----------|-----:|
| 0   | `deep_ocean`     | eau        | 0    | 13  | `woods`       | forêt     | 50   |
| 1   | `ocean`          | eau        | 1    | 14  | `jungle`      | forêt     | 52   |
| 2   | `shallow`        | eau        | 2    | 15  | `marsh`       | humide    | 35   |
| 3   | `coast`          | eau/rivage | 3    | 16  | `highlands`   | relief    | 60   |
| 4   | `plains`         | herbe      | 20   | 17  | `hills`       | relief    | 62   |
| 5   | `farmland`       | herbe      | 22   | 18  | `mountains`   | relief    | 70   |
| 6   | `grassland`      | herbe      | 21   | 19  | `peak`        | relief    | 72   |
| 7   | `steppe`         | herbe sèche| 18   | 20  | `glacier`     | froid     | 80   |
| 8   | `savanna`        | herbe sèche| 17   | 21  | `mangrove`    | humide    | 34   |
| 9   | `drylands`       | aride      | 12   | 22  | `bog`         | humide    | 33   |
| 10  | `desert`         | aride      | 10   | 23  | `volcano`     | roche nue | 90   |
| 11  | `coastal_desert` | aride      | 11   | 24  | `thorns`      | §27 fin   | 95   |
| 12  | `forest`         | forêt      | 51   |     |               |           |      |

**`prio`** = priorité de transition : à une frontière, le biome de **prio supérieure** « déborde »
sur l'autre (ses tuiles de bord recouvrent). Tu peux ajuster ces valeurs, dis-le moi.

L'index/clé est **autoritatif** (vient de `scps/scps_types.h` `enum Biome`). Le moteur expose le
biome par cellule via la couche `SCPS_LAYER_BIOME`.

---

## 3. Autotiling — schéma **dual-grid** (16 tuiles / biome)

Tu as choisi des **tuiles de transition dédiées**. On utilise le **dual-grid** (le plus économe :
16 tuiles par biome au lieu de 47) :

- La grille de RENDU est décalée d'une **demi-tuile** : chaque tuile de rendu chevauche **4 tuiles
  monde** (ses 4 coins = 4 centres de tuiles monde).
- Pour un biome donné, chacun des 4 coins est **PLEIN** (ce biome, ou un biome de prio ≥) ou **VIDE**.
  → masque 4 bits = **16 combinaisons**. Les transitions s'obtiennent en **empilant** les biomes du
  plus bas au plus haut `prio` (chaque couche dessine sa tuile-masque par-dessus la précédente).
- **Bits du masque** (coin → bit) : `N=1 · E=2 · S=4 · W=8` (N=haut, E=droite, S=bas, W=gauche).

### Livrable par biome : un **atlas 4×4** de 16 tuiles
- Fichier : `iso_tiles/<clé>.png`, taille **1024 × 512** (4 colonnes × 4 lignes de 256×128).
- **Indexation** : tuile du masque `m` (0..15) à la case `col = m % 4`, `row = m / 4`
  (origine haut-gauche ; `m=0` en (0,0), `m=15` en (3,3)).
- Sémantique du masque `m` = quels **coins** sont **PLEINS** (ce biome) :
  - `m=15` (NESW tous pleins) = **tuile pleine** du biome (cœur).
  - `m=0` (aucun) = **entièrement transparente** (ne dessine rien — le biome dessous reste).
  - les 14 autres = bords/coins (ce biome qui s'avance dans 1, 2 ou 3 coins).
- Référence visuelle du dual-grid : layout 4×4 standard (jess::codes / ThinMatrix). Si tu préfères
  un **47-blob** (8 voisins) au lieu du dual-grid, dis-le — je recâble (mais 16 suffisent et c'est
  beaucoup moins d'art).

> Démarrage minimal : si tu ne fournis QUE `m=15` (la tuile pleine), le monde se couvre déjà en
> tuiles iso (sans transitions douces) — les 15 autres masques sont un **raffinement** que tu peux
> livrer par vagues.

---

## 4. Eau (biomes 0-3) + houle animée

- `deep_ocean / ocean / shallow / coast` = tes **tuiles d'eau** (même schéma dual-grid 4×4, ou au
  minimum la tuile pleine `m=15`). La profondeur est déjà un dégradé de biomes (0→3).
- Par-dessus, le moteur applique un **shader de houle/écume animée léger** (display-only, horloge
  mur — n'altère pas le déterminisme) pour la vie. Tu n'as pas à animer l'eau.

---

## 5. Le reste des assets (remplacement iso progressif)

Tout ce qui est déjà intégré sera **remplacé par sa composante iso**. Conventions actuelles
(conservées — dépose les versions iso aux **mêmes chemins/noms**, je ne touche pas au code de chargement) :

| catégorie     | dossier                              | nommage / forme |
|---------------|--------------------------------------|-----------------|
| centres-ville | `pack/centres/<terrain>/`            | `CITY_CENTRE_<TERRAIN>_T1..T7.png` (6 terrains × 7 tiers) |
| structures    | `pack/structures/`                   | `*.png` (énumérées au runtime) — bâti de bourg |
| dressing      | `pack/dressing/`                     | `DRESS_*.png` (arbres, rochers, buissons, murets…) |
| édifices      | `pack/buildings/`                    | `EDI_*.png` |
| rivières      | `pack/rivers/`                       | `RIVER_HORIZONTAL.png` (segment, tourné par le moteur) |
| armées        | `pack/campaign/`                     | `ARMY_TOKEN_*.png` |
| **tuiles**    | **`pack/iso_tiles/`** *(nouveau)*    | `<clé_biome>.png` (atlas 4×4 256×128) — §3 |

- **Ancre des bâtiments / sprites posés** : **pied au centre-bas** (le sommet BAS du losange de leur
  tuile). Les routes **entrent par ce même sommet** (orientation officielle déjà verrouillée).
- Sprites posés (bâti/dressing) : PNG RGBA, transparent autour ; dessinés à taille MONDE, ancrés au
  pied — calibre tes nouveaux assets pour qu'un bâtiment « 1 tuile » ait sa base ≈ 256 px de large.

---

## 6. Intégration côté code (déjà préparé)

- `ui/uikit.gd` : `iso_tile_atlas(<clé>)` (charge l'atlas) + `iso_tile(<clé>, mask)` (extrait la
  région). Cache par chemin, chargement runtime.
- `map/iso_ground.gd` : le **renderer de sol iso** — couvre le monde en tuiles, dual-grid empilé par
  prio, **culling viewport** + **YSort**, **houle** sur l'eau. **Repli** : tant que `pack/iso_tiles/`
  est vide, le rendu procédural actuel (terre lissée + côte par pixel) reste — **rien ne casse**.
- Bascule **automatique** : dès qu'au moins la tuile pleine `m=15` d'un biome est présente, le sol
  passe en tuiles.

Dépose les assets, lance le jeu — pas de changement de code requis de ta part.
