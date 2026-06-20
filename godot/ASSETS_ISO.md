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

L'index/clé est **autoritatif** (vient de `scps/scps_types.h` `enum Biome`). Le moteur expose le
biome par cellule via la couche `SCPS_LAYER_BIOME`. *(La colonne `prio` servait au dual-grid
abandonné — gardée comme repère de « qui domine » pour le choix de canevas/feature ; non
contraignante.)*

---

## 3. Le sol = **UNE tuile propre par biome** + blend (l'approche RETENUE)

> **DÉCISION FINALE (2026-06-20).** Après essais, les **canevas continus** (super_biomes /
> palettes) **ne tuilent PAS** sur une carte procédurale par cellule : leurs tuiles sont
> *transitionnelles*, le placement par cellule = bruit. La solution **propre & qui MARCHE** :
> **une tuile NETTE par biome** (`pack/iso_tiles/biomes/<clé>.png`, 256×128), peinte par cellule
> **par-dessus le blend procédural** (qui adoucit les bords + fait l'eau/les côtes). L'art se
> **remplace au slot** : tuiles générées (présentes), **Kenney CC0** « Isometric Landscape », ou
> tes propres tuiles IA **par biome** (PAS des canevas continus). Falaises §3b inchangé.
>
> Godot ne FOURNIT pas d'art ; il fournit le **système** (`TileMapLayer`+`TileSet` iso, autotiling
> par *terrains*). Les sections ci-dessous (canevas/palette) restent comme **exploration** ; le
> moteur du jeu lit `pack/iso_tiles/biomes/`.

<details><summary>Historique : modèle canevas continus (exploratoire, non retenu)</summary>

### Règle d'or (la continuité vient de la SOURCE, pas de pièces modulaires)
- Une tuile **garde ses voisins** d'origine (même `row`/`col`, même canevas). On NE mélange PAS
  des tuiles isolées de canevas différents : on stampe un **champ 5×5 entier**, ou on **crope un
  sous-rectangle contigu** dedans.
- **Modèle de pose retenu : « monde = sol général, features superposées » (100 % procédural)** :
  | famille (dossier)   | rôle moteur | posée par le renderer… |
  |---------------------|-------------|------------------------|
  | `canevas_monde`     | côte/sol multi-biome général | **SOL GÉNÉRAL** : stampé partout (pavage de blocs 5×5 cropés à la région) |
  | `cote_inversee`     | côte, orientation opposée | superposée aux **littoraux** d'orientation inverse |
  | `estuaire`          | embouchure / delta | superposée aux **embouchures de rivière** |
  | `canevas_falaises`  | rupture géologique (plateau · face · talus · bas) | superposée aux **ruptures de relief** (cf. §3b) |
- Le moteur n'a **pas d'éditeur** : la famille est choisie **automatiquement** d'après le terrain
  local (la carte naît de la génération par cellule — `SCPS_LAYER_BIOME`/`_HEIGHT`/`_COAST`).

### Livrable par famille
- Dossier `pack/iso_tiles/<famille>/` (ou conserve `ISO_<CANVAS>_VNN.png`), **25 PNG 256×128**.
- Plusieurs **variantes** de champ par famille = plus de variété (sinon le 5×5 se répète) ; nomme
  les variantes `…_setB_VNN.png`, etc. — dis-moi le nommage final, je l'indexe.
- **Invariants qualité** (repris de `CANEVAS_INDEX.json`) : RGBA, transparent hors du losange
  canonique, **pas de magenta**, **pas de frange rose**, **tout en 256×128**.

> Le pipeline dual-grid que j'avais posé (atlas 4×4 par biome, `iso_ground.gd`) est **remplacé**
> par ce modèle ; j'adapte le renderer au stamping de champs quand les canevas sont figés.

### `super_biomes_NN` — la source PRIMAIRE (grand champ continu, par ZONE CLIMATIQUE)
`super_biomes_01` = champ **10×10** (100 tuiles) « haute-variété snapable » : neige/froid · forêt ·
plaine · roche plate · marais/tourbe · plage · crique · presqu'île · estuaire · bas-fonds · mer.
- **Constat asset (vérifié, seed 9)** — deux choses cadrent le renderer :
  1. **Les tuiles sont TRANSITIONNELLES** (eau+sable, plaine/forêt mêlées) : on **ne peut PAS**
     découper le canevas en 25 tuiles-par-biome propres (la classif couleur s'effondre). En
     revanche **terre / eau / neige se séparent** nettement. Le placement **aléatoire par tuile =
     BRUIT** (prouvé) ; il **FAUT** du contigu.
  2. `super_biomes_01` couvre **UNE zone climatique** (tempéré-froid côtier ; ni désert, ni
     jungle…). Le suffixe `_NN` ⇒ d'autres jeux par climat (désert, tropical, aride…). Le monde
     est rendu en mappant **biome → zone climatique → jeu `super_biomes_NN`**.
- ⚠ **Les `super_biomes` sont des palettes-EXEMPLE** : on n'en stampe **JAMAIS** la disposition.
  On se sert des **tuiles découpées** comme une **PALETTE de variation** (pas de « système
  climatique » — juste des lots de couleurs à fondre). Recette RETENUE :
  1. **blend DERRIÈRE** = le sol procédural (mélange biome-couleur lissé) — il fait la mer, les
     côtes et la teinte ;
  2. **terre** : on **PIOCHE** une tuile de terre de la palette **par cellule** (variation),
     **fusionnée (alpha) sur le blend** → le blend dissout le clash entre voisines (sans lui, le
     placement par tuile = BRUIT, prouvé) ;
  3. **falaises** (§3b) → assombries = **barrière inhabitable**.
- **Anti-répétition / climats** : fournis **plusieurs lots** (`super_biomes_02…`, autres
  couleurs/terrains) — je les ajoute à la palette ; option **rotation/miroir/jitter** au tirage.
  Côtes *speckle* ⇒ à lisser via overlay côtier (`canevas_monde`/`cote_inversee`).

---

</details>

---

## 3b. Falaises = barrière **inhabitable** (façon Age of Empires)

La face rocheuse qui **grimpe** (`canevas_falaises`) est une **barrière infranchissable**, pas du
sol bâtissable. Décidé : **auto depuis la rupture de relief, classe PEAK, zéro bump SAVE**.

- **Détection** (display-derivée, « on lit des coordonnées ») : une cellule de **terre** dont le
  **gradient de hauteur** vers un 4-voisin dépasse le seuil (≈ **40/255**, calibré : le monde est
  bimodal — sol lisse Δ<20 vs falaise Δ≥30) est une **cellule de falaise**. ~0.3 % de la terre,
  groupée le long des dorsales (vérifié seed 9). Source : couche `SCPS_LAYER_HEIGHT` (déjà exposée).
- **Conséquences (AoE)** : sur une cellule de falaise →
  1. **rendu** : la face de `canevas_falaises` (pas le sol général) ;
  2. **inhabitable** : aucun bâti, aucune colonisation, aucun décor posé dessus ;
  3. **barrière** : l'armée ne traverse pas (passage nul) — on contourne.
  Le **plateau au-dessus** et la **terre basse en dessous** restent **habitables** : seule la FACE
  est morte. Réutilise le mécanisme `terrain_impassable`/`region.impassable` existant (PEAK/glacier
  /volcan sont déjà ainsi : `habitability 0`, sautés par toutes les boucles éco/colonisation).
- L'enforcement **gameplay** (passage armée nul, décor sauté) passe par les **entrées moteur**
  (la membrane) → commit dédié, déterminisme re-baseliné, `make test` au vert (pas un hack viewer).

---

## 4. Eau (biomes 0-3) + houle animée

- L'eau (`deep_ocean / ocean / shallow / coast`) est **incluse dans les canevas côtiers**
  (`canevas_monde` / `cote_inversee` / `estuaire` portent déjà mer profonde → bas-fonds → plage).
  Au large (pleine mer hors canevas côtier), je tuile la portion eau du canevas le plus proche.
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
| **sol (canevas)** | **`pack/iso_tiles/<famille>/`** *(nouveau)* | `ISO_<CANVAS>_VNN.png` (champ 5×5 de 256×128) — §3 |

- **Ancre des bâtiments / sprites posés** : **pied au centre-bas** (le sommet BAS du losange de leur
  tuile). Les routes **entrent par ce même sommet** (orientation officielle déjà verrouillée).
- Sprites posés (bâti/dressing) : PNG RGBA, transparent autour ; dessinés à taille MONDE, ancrés au
  pied — calibre tes nouveaux assets pour qu'un bâtiment « 1 tuile » ait sa base ≈ 256 px de large.

---

## 6. Intégration côté code (état)

- **Chargement runtime** des PNG (pas d'import éditeur) : acquis (`ui/uikit.gd`). À recâbler du
  modèle dual-grid (par biome) vers le **stamping de champs 5×5** (par famille) — fait quand les
  canevas sont figés.
- `map/iso_ground.gd` : renderer de sol — **repli** intact (tant que `pack/iso_tiles/` est vide, le
  sol procédural actuel reste, rien ne casse). Sa logique passe de l'atlas-par-biome au **stamping
  de canevas + overlays de feature** (côte inversée / estuaire / falaise).
- **Falaises** (§3b) : signal de rupture de relief depuis `SCPS_LAYER_HEIGHT` — **détection
  prouvée** (seed 9). Reste : rendu de la face + enforcement inhabitable (commit moteur dédié).
- **Houle** : shader display-only sur les portions eau.

Ordre de marche : (1) tu figes un jeu de canevas par famille → (2) je câble le stamping + overlays
+ la houle → (3) commit moteur falaise-inhabitable (déterminisme re-baseliné, tests verts).
