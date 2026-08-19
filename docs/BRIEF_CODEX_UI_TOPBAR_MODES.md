# BRIEF CODEX — Refonte TOPBAR · rail gauche/icônes · MODES CARTE (2026-08-19)

Décision joueur : « On va rework la topbar/sidebar, très moche. Les icônes en général.
Une passe maintenant que tous les designs sont fixés. »

## 0. Contrats NON NÉGOCIABLES (relire CLAUDE.md avant toute ligne)

- **Membrane stricte** : la façade Godot ne lit que des MOTS (readout) et des nombres
  tangibles via `scps_api.h` / binding `godot/src/scps_sim_node.cpp`. JAMAIS un
  flottant moteur brut face joueur, jamais un calcul inventé côté .gd — si une
  donnée manque, on AJOUTE un reader façade PUR (dérivé, sans état).
- **Doctrine UI** : TOPBAR = NATIONALE · rail GAUCHE = menus (contextuel) · barre
  DROITE = monde/raccourcis. Toute valeur : PAR MOIS, réelle, jamais le calcul,
  jamais d'annuel. L'info générale à l'œil, le DÉTAIL en hover. Info à ≤ 3 clics.
- **STR_*** : tout texte face-joueur naît en `scps/strings_ids.h` (FR) +
  `scps/strings_en.h` (EN). `make lang-check` doit rester vert (baseline
  `scps/lang_baseline.txt` — le cliquet INTERDIT de nouveaux littéraux).
- **Déterminisme / golden** : les readers ajoutés sont PURS (aucun état, aucune
  écriture) ⇒ golden-neutres. Aucun bump SAVE_VERSION attendu dans ce lot.
- **Ne pas toucher** : `godot/project/map/overlay.gd` rendu routes/cuisson,
  `iso_antique.gdshader`, worldgen — chantiers calibrés récents, HORS périmètre.
- **Gates avant de rendre** : `make full-test` (40 bancs) · `scps_viewer --savetest`
  · `make determinism` · `make lang-check` · parse-check Godot
  (`Godot --headless --path godot/project --check-only --quit`).
- **TROUVAILLES.md** : APPEND en fin de mission (Découvertes · Pièges · Restes).
- Fin de mission : probes visuels (voir §5) — on REGARDE, on ne suppose pas.

## 1. Chantier A — TOPBAR (refonte de composition)

Fichier : `godot/project/ui/topbar.gd` (792 l., Control custom-draw, 4 blocs
actuels ROYAUME·ÉCONOMIE·POLITIQUE·TEMPS). Hauteur : `Frame.TOPBAR_H = 48.0`
(cellules « CK3 » : valeur EMPILÉE sur delta). Kits : `VKit` (typo/couleurs),
`UIKit` (chips parchemin + `icon()` → planches série-2 `sheet11_system_icons_*`),
`InfoRef`/`hover_zones.gd` (tooltips détaillés), `heraldry.gd` (blasons),
`date_chip.gd`. Le patron DELTA MENSUEL (photo à `month_ticked`, `_d_gold` etc.)
et le MODE OBSERVATEUR (`_observing()`) EXISTENT — les préserver.

### Nouvelle composition, de gauche à droite (l'ordre EST la spec) :

1. **NOM DU PAYS + BLASON** — blason via `heraldry.gd` (chip du pays joué), nom à
   côté ; clic = ouvrir le panneau pays (signal `navigate_requested` existant).
2. **OR** — trésor + delta mensuel (pattern `_d_gold` existant). Icône or série-2.
3. **POPULATION TOTALE** — pop empire + delta (`_d_pop` existant).
4. **MATÉRIAUX DE CONSTRUCTION** — UNE cellule agrégée : somme des stocks
   `RES_STONE + RES_CLAY + RES_WOOD` (via `country_stocks()` — entrées par nom,
   champ `stock` + `net_day`). Valeur = somme des stocks ; delta = somme des
   `net_day × 30`. **Hover = le détail par ressource** (Pierre X +a/mois ·
   Argile Y +b/mois · Bois Z +c/mois) — même mécanique de tooltip que les
   cellules actuelles (`_nav_zones`/hint), jamais un panneau.
5. **NOURRITURE** — même idée : somme `RES_GRAIN + RES_LIVESTOCK + RES_FISH +
   RES_FRUIT` ; hover détaillé par ressource.
6. **ARMES** — même idée : `RES_ARMS + RES_ARMS_HEAVY + RES_GUNPOWDER +
   RES_ENCHANTED_ARMS` ; hover détaillé.
7. **PRODUITS MANUFACTURÉS** — même idée : famille de production (`RES_TOOLS,
   RES_CLOTH, RES_PAPER, RES_REMEDE, RES_TUNIQUE, RES_BEER, RES_EAU_DE_VIE,
   RES_PRECIOUS_WARE, RES_PRECIOUS_CLOTH`) ; hover détaillé (lister seulement
   les ressources à stock ou flux non nul — pas 9 lignes de zéros).
8. **SATISFACTION GLOBALE** — la MOYENNE pays : reader `scps_country_demo`
   (champ `satisfaction`, 0..100, agrégat pondéré pop — existe déjà). **Hover =
   détail PAR CLASSE** (`cls_sat[3]` du même reader : Journaliers · Bourgeois ·
   Élite ; ajouter la ligne serviles si le reader l'expose, sinon 3 classes).
   Le ±X « Votre politique » existe (`scps_api.h:1696`) — l'inclure au hover.
9. **VITESSE + DATE** — reprendre les contrôles existants (boutons vitesse
   RimWorld + capsule de date) tels quels, à droite.

### Ce qui SORT de la topbar (l'actuel à retirer proprement)
Prix national, savoir, pénurie-alerte, factions/loyauté, spectres (les 4-5 % à
boussoles) : ces cellules DISPARAISSENT de la topbar. Vérifier que chaque info
retirée reste accessible ailleurs (tiroir Stocks `sidebar_drawer.gd`, panneau
pays, etc.) — si une info n'existe NULLE PART ailleurs, la déplacer, jamais la
perdre. Documenter le nouvel emplacement dans le commit.

### Style
Cellules : icône gravée série-2 (24 px) + valeur `VKit.FS` + delta dessous
(vert/rouge, `_delta_txt`). Séparateurs de blocs (`_block_sep`) conservés mais
réduits à 3 groupes : IDENTITÉ (1) · RESSOURCES (2-7) · ÉTAT (8) · TEMPS (9).
AUCUNE nouvelle couleur hors palette VKit. Mode observateur : les cellules 1-8
remplacées par le mot neutre existant, le bloc TEMPS intact.

## 2. Chantier B — Rail gauche & passe ICÔNES générale

- Rail : `godot/project/ui/sidebar.gd` (`Frame.SIDEBAR_W = 64`, boutons
  `BTN = 52`, 8 onglets `menu_*`, états normal/hover/sélectionné/indispo via
  `icon_button.gd`). Le rail RESTE (structure validée) — c'est le DESSIN des
  icônes qui change : toutes les icônes du rail ET de la topbar passent sur les
  planches série-2 (`UIKit.icon()` + remap `sheet11_system_icons_*` —
  compléter le remap si un id manque, les planches sont dans
  `res://assets/scps/ui/icons/`). Supprimer les derniers PNG hors-planche.
- Cohérence : même traitement gravure/encre partout (pas un mélange emoji-style
  / gravure). Si une icône n'a AUCUN équivalent de planche, la lister dans
  TROUVAILLES (Restes) plutôt que d'en dessiner une à la main.

## 3. Chantier C — MODES CARTE (4 modes, remplacent la rangée actuelle)

État actuel : `map_view.gd` → `var mode := 0` (0 terrain · 1 politique · 2
régions · 3 pays · 9 ressources) + `overlay.nature_mode` (bool séparé, touche N).
La rangée de boutons en bas à gauche (5 médaillons) est le switcheur — la
trouver par grep (`mode`, boutons bas-gauche, probablement `main.gd` ou un
panneau filtres). REMPLACER par 4 modes exclusifs :

1. **DÉFAUT** — l'actuel mode 0 (terrain + lavis politique léger + tout le
   chrome). Rien à changer au rendu.
2. **POLITIQUE** — pays par couleur pleine : réutiliser `political_image()`
   (C++, existe) + le mode 1 existant ; frontières + noms, terrain estompé.
3. **NATURE** — `nature_mode` existant : terrain + dressing SEULS, rien d'autre
   (pas de frontières, pas de villes, pas de routes vectorielles, pas de noms).
   Devient un mode de la rangée au lieu d'une touche cachée (garder N en
   raccourci).
4. **MARCHÉ** (NOUVEAU) — chaque province teintée par le MARCHÉ dont elle
   dépend (système de PROXIMITÉ) :
   - **Moteur** (nouveau reader PUR, `scps_econ.c` ou `scps_api.c`) :
     `scps_market_catchment(sim, pid)` → pid du CENTRE de marché dont dépend la
     province (province possédant un édifice Marché/Comptoir/Centre de commerce
     — masque `edi_built`, cf. `EDI_MARCHE|EDI_COMPTOIR|EDI_TRADE_CENTER`), par
     distance de graphe la plus courte (réutiliser l'infra de pathing commerce
     existante — grep `route`, `lane`, distances intertrade — NE PAS réinventer
     un Dijkstra si un champ de distance existe). Sans centre atteignable → -1.
     Reader DÉRIVÉ pur + cache invalidé à la clôture mensuelle (motif des vues) ;
     AUCUN nouvel état sérialisé, AUCUN bump save, golden-neutre.
   - **Binding** : exposer `market_catchment_image(palette)` sur le motif EXACT
     de `political_image()` (une Image teintée bâtie en C++ — jamais une boucle
     GDScript par cellule).
   - **Façade** : teinte par centre = famille de couleur stable (hash du pid du
     centre → palette parchemin désaturée, motif `_entity_pigment`) ; hover
     d'une province en ce mode = « Marché de {nom de la ville centre} » (nom
     via `region_city_name` existant). Les provinces à -1 : gris neutre.
- Boutons : 4 médaillons série-2 (Défaut/Politique/Nature/Marché), état
  sélectionné visible (motif `icon_button.gd`), tooltips STR_*.

## 4. Fichiers du périmètre (et EUX SEULS)

- `godot/project/ui/topbar.gd` (refonte), `sidebar.gd`/`icon_button.gd`/
  `uikit.gd` (remap icônes), `heraldry.gd` (lecture seule), le fichier du
  switcheur de modes (à localiser), `map_view.gd` (mode MARCHÉ).
- `scps/scps_econ.c` ou `scps_api.c` + `scps_api.h` (reader catchment),
  `godot/src/scps_sim_node.cpp` (binding image), `scps/strings_ids.h` +
  `strings_en.h` (nouveaux STR_*), `godot/project/i18n/ui.csv` si clés Godot.
- INTERDITS : overlay.gd (rendu carte), shaders, worldgen, tout module sim.

## 5. Vérification (avant de rendre)

1. Gates : full-test 40/40 · savetest · determinism · lang-check · parse-check.
2. Probes visuels (fenêtré, `SCPS_MUTE=1 Godot --audio-driver Dummy --path
   godot/project res://<probe>.tscn -- seed=205`) : shot topbar (probe existante
   `series2_audit.gd` ou équivalent), shot des 4 modes carte (créer une probe
   `map_modes_shot.tscn` sur le motif de `map_art_shot.gd` : un PNG par mode).
   REGARDER les PNG : cellules alignées, hover-zones aux bonnes coordonnées
   (`_nav_zones` = mêmes rects que le dessin), mode Marché lisible (pas un
   patchwork bruité — si toutes les provinces ont chacune leur couleur, le
   catchment est cassé).
3. TROUVAILLES.md : APPEND (Découvertes · Pièges · Restes).

## 6. Pièges connus (payés cher, ne pas les repayer)

- `country_stocks()` renvoie des entrées PAR NOM (String) — matcher par nom
  (`_res_pair` existant), jamais par index. Une ressource absente = pas de ligne.
- Les hover-zones de la topbar (`_nav_zones`) doivent être REMPLIES avec les
  MÊMES coordonnées que le dessin (elles divergent facilement au refactor).
- `_w()`-like : tout ce qui est dessiné custom-draw dans la topbar est en px
  écran directs (pas de zoom) — ne pas importer les patterns de l'overlay.
- Mode observateur : `_observing()` court-circuite le bloc ROYAUME — le nouveau
  layout doit le préserver (le probe observateur existe : `observer_shot.gd`).
- `political_image(palette)` : la palette est passée DEPUIS le .gd (familles de
  couleur par entité) — suivre ce motif pour le catchment, pas de couleurs C++.
- Godot 4 : pas de `Dictionary.has` sur null ; `draw_string` ancre à la BASELINE
  (`VKit.text_map` gère l'ascent — passer par VKit, jamais draw_string nu).
