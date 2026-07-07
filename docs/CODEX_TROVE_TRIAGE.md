# Triage du trove Codex — `C:\Users\Charl\Documents\Codex\` (2026-07-07)

77 zips sur 2026-06-16 → 06-29. Trié par ACTION. Chemins relatifs à
`C:/Users/Charl/Documents/Codex/`.

## ✅ DÉJÀ INTÉGRÉ (ne rien faire)
- `2026-06-29/…/scps_resources_atlas_ancien_v3` → **48 icônes de ressources** (commit
  « ASSETS — les 48 icônes »). Le re-découpage PROPRE qui a remplacé les cellules foirées.
- `2026-06-29/…/scps_event_illustrations_v2_integration` → les **8 bannières d'évènement**
  (commit « Illustrations d'évènements sur les popups »).
- Les planches `scps_ui_*sheetNN` sont DÉJÀ dans le repo sous `assets/scps/ui/parch/`
  (sheet01-32, ≈500 cellules) — mais **découpées de façon foirée** (voir § RE-CUT).

## 🔧 À INTÉGRER MAINTENANT (assets prêts, docs fournis)
- `scps_endgame_screens_atlas_v1_2026-07-06` (48 Mo) → **5 fonds de fin** 1920×1080 +
  barre d'entropie. Doc : `docs/IMPLEMENTATION_GODOT.md`. → épilogue + endgame_banner.
- `scps_illuminated_borders_ai_v1_2026-07-06` (86 Mo) → **8 bordures d'âge + 5 de fin**
  2048² NinePatch, centre transparent. Doc fournie. (Ignorer `scps_illuminated_borders_v1`
  23 Mo = version NON-IA antérieure, le joueur a désigné la _ai_.)

## ✂️ LE GROS CHANTIER — RE-DÉCOUPE des planches (« la plupart des découpages sont foirés »)
Les cellules actuelles (`assets/scps/ui/parch/*.png`) ont été mal coupées de planches
MASTER magenta. Les MASTERS sont dans ces zips (à re-couper : auto-détection de grille +
key magenta + despill + trim/re-center par cellule) :
- `scps_ui_parchment_12_sheets` (89 Mo) — les 12 planches de base (sheet01-12 : chrome,
  boutons, jauges, tech, édifices ×2, manufactures, unités ×2, système, cartouches).
- `scps_ui_parchment_series2_full` (111 Mo) — série 2 (sheet13 conseillers, 14 factions,
  15-18 tech, 19 ressources, 20-24 biomes/topbar, chrome restant).
- `scps_ui_parchment_series3_final` (32 Mo) — série 3 (sheet25-28 : tech complément,
  manufactures complément, rituels de fin/curseurs).
- `scps_ui_parchment_series4_plateau` (22 Mo) — série 4 (sheet29-32 : héraldique boucliers/
  charges, pions d'armée iso étain).
- (les zips `scps_ui_sheetNN` / `scps_ui_series2/3/4_sheetNN` individuels sont les mêmes
  planches en granularité fine — sources redondantes.)
→ Chaque delivery a `sources/raw_magenta_sheets/` + un `manifest.json` (mapping cellule→id).
  Le re-cut vise `assets/scps/ui/parch/sheetNN_<name>_MM.png` (les mêmes noms que lisent
  PARCH_UNIT/PARCH_BLD/… dans uikit.gd — remplacement en place, zéro changement de code).

## 🗺️ ORPHELINS — la carte est 100 % PROCÉDURALE (ne PAS intégrer sauf revirement DA)
Tampons/tuiles de carte, inutilisés depuis le passage au shader parchemin :
`lot1_stamps`, `lot2_painted_marks`, `lot3_biome_marks`, `lot4_sea_serpent…`, `lot5_kcd`,
`lot6_*` (broadleaf/conifer/ground/houses/landmarks/settlements + micro + da_stamps + front32),
`scps_map_overlays_32`, `scps_road_tiles_32`, `scps_oriented_assets_16`, et (2026-06-23)
`props`, `props (2)`, `buildings`, `muddy-track-seamless-512`. → archive/ignore.

## 🎨 ÉVÈNEMENTS — upgrade possible (déjà 8 bannières en place)
- `scps_event_backgrounds` (+ sheet_a governance/famine/court/roads, sheet_b entropy/schism/
  tech/consequences) et `scps_event_illustrations_wide` (+ a/b) → fonds/illustrations
  d'évènement plus larges/variés que les 8 bannières actuelles. **Optionnel** : élargir
  event_art.gd si on veut plus de variété visuelle par thème. Basse priorité.

## 📦 DIVERS
- `scps_ui_series2_menu_background` (5 Mo) → fond de menu principal (déjà un
  `menu_main_background.png` dans le repo ? à vérifier — sinon brancher au menu).
- `scps_da_register_test_v2`, `scps_missing_assets` → planches de TEST DA / lot de
  bouche-trous ; vérifier s'ils apportent des pièces neuves vs déjà couvert.
- `2026-06-16/Jeu1-SCPS.zip`, `SCPS-claude-…-2.zip` → **snapshots du dépôt** (code), pas
  des assets. Ignorer.

## 🧠 NON-ASSETS — analyses & propositions de code (dossiers 07-01 → 07-05)
Ces dossiers ne sont PAS des packs d'assets — ce sont des sorties de travail Codex :
- `2026-07-01/tu/outputs/scps_code*.txt` → dump du code source (avec/sans commentaires).
  Redondant avec le repo. Ignorer.
- `2026-07-02`, `2026-07-03`, `2026-07-04` → dossiers `work/` (llama.cpp, outillage Codex).
  Rien pour le jeu. Ignorer.
- `2026-07-05/ana/outputs/audit-scps-lecture-seule.md` (221 lignes) → **AUDIT read-only
  utile**. P1 : `research_target` + la file `cmd_n` ne sont PAS dans le save → au chargement
  d'une partie joueur, la cible de recherche courante retombe à -1 et une commande en attente
  pourrait s'appliquer au monde chargé. ⚠ Les classes d'ACCUMULATEURS qu'il craint (caches
  globaux, routes) ont DÉJÀ été traitées (EMOB v57 · COLC v61 · TXYR v65 · hub intertrade) —
  reste ce trou `research_target`/`cmd_n` (QoL joueur mineur, pas une corruption). À trancher
  à part (sérialiser research_target, décider du sort de cmd_n au load).
- `2026-07-05/ana/outputs/scps_event_batch_pattern_code.md` → **proposition de contenu** :
  un lot d'évènements au motif « situation → 3 choix imparfaits → rareté → conséquence ». À
  évaluer vs le pipeline events existant si on veut plus de contenu.
- `2026-07-05/…/scps_propositions_release_design.xlsx` → tableur de design de release. À lire.

## Ordre d'action recommandé
1. **MAINTENANT** : écrans de fin + bordures (fait/en cours ce tour).
2. **QUAND LES AGENTS REPASSENT** (limite de session ~3h40) : la RE-DÉCOUPE des 4 masters
   (workflow : 1 agent par série, auto-grid + key + trim, remplace les cellules foirées) —
   c'est le gros lot qui répare « la plupart des découpages foirés ».
3. **OPTIONNEL** : menu background, upgrade des fonds d'évènement.
4. **IGNORER** : orphelins de carte, snapshots de code.
