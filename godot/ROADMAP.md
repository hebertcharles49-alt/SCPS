# SCPS × Godot — feuille de route du front-end

> Le moteur C99 (la sim) **ne bouge pas**. On bâtit l'ŒIL et les MAINS. Chaque
> phase est marquée **[Godot]** (aucune touche moteur — faisable tout de suite) ou
> **[+façade]** (demande d'ajouter quelques getters à `scps/scps_api.*` — différé
> tant qu'« on laisse le moteur de côté »). La règle d'or partout : **zéro logique
> de simulation côté GDScript**.

---

## Phase 0 — Spike ✅ (fait)

Façade C `scps_api`, binding GDExtension (`ScpsWorld`), `scons` lie `libscps.so`,
la carte se rend (`render_map`→texture), le shader d'eau anime la mer, ça tique.
Banc `scps_api_demo` 9/9 (reproductible).

## Phase 1 — La carte vivante **[Godot]** ← *on est là*

Le socle de présentation, **sans toucher au moteur** (n'utilise que la façade
actuelle).
- **`Sim` (autoload)** : détient `ScpsWorld`, cadence le temps (vitesse pause/
  ×1/×2/×3), émet `generated` / `ticked(year)`. Le point d'accès unique au monde.
- **`MapView`** : terrain (`map_image(mode)`) en texture + `water.gdshader` +
  `Camera2D` (zoom molette, pan clic-droit) + bascule des **modes de carte**
  (terrain / politique / régions / pays — déjà dans `map_image`).
- **`Topbar`** : année · pop · pays · contrôle de vitesse.

Livrable : on génère, on regarde le monde vivre, on navigue. **Aucune dépendance
moteur nouvelle.**

## Phase 2 — Lire le monde (la membrane → panneaux) **[+façade]** ✅ (fait)

- ✅ *Façade ajoutée* : `scps_province_info` / `scps_country_info` (POD de MOTS
  résolus + nombres → `Dictionary` côté binding) et `scps_province_at(x,y)`
  (picking cellule→province). La façade alloue+ticke `WorldProsperity` /
  `WorldLegitimacy` / `TechState` (lus en CONST → la colonne éco reste
  byte-identique ; `scps_api_demo` 9/9). `scps_map_rgba` prend la province
  surlignée.
- ✅ **Sélection** : clic gauche sur la carte → province surlignée (`render_map`
  `selected_prov`), clic-glissé distingué du clic (slop), clic en mer referme.
- ✅ **`ProvincePanel` / `CountryPanel`** : bandes + mots + jauges 0-100 (la
  membrane), en `Control` bâtis en code. Aucun flottant SCPS lu côté Godot.
- ✅ **TICK PLEIN** : `advance_days` roule désormais `sim_day` (cœur partagé
  `scps_sim.c`, extrait de chronicle À L'IDENTIQUE — hash inchangé). Fini les
  zéros : le monde VIT (guerres, sécessions, or & prospérité réels). La surface
  façade n'a PAS bougé.

## Phase 3 — Les acteurs sur la carte **[+façade]** ✅ (fait)

Le tick plein roule DÉJÀ campagnes & guerres ; on les EXPOSE et on les dessine.
- ✅ *Façade* : `scps_army_info` (loc · dest · phase · effectif · composition, lu
  de `s->sim.camp`) + `scps_region_tier` (0-5 selon pop, capitale ≥4).
- ✅ **`Overlay`** (Node2D enfant de MapView, espace monde → suit la caméra) :
  **villes** par tier (disque teinté au pays, cœur clair) + **armées** (losange
  teinté au pays, halo de phase marche/siège/bataille, ligne vers le but).
  Redessine au tick. **Vérifié** (capture xvfb : 7 armées, 73 villes à l'an-110).
- ⏳ *Reste (optionnel)* : panneau d'armée au clic, frontières fines, anim de marche.

## Phase 4 — Le spectacle : l'endgame §27 **[+façade]** ✅ (cœur fait)

Le moteur MUTE déjà le monde quand une fin éclôt (régions englouties → mer, biomes
blanchis au froid, ronces qui gagnent) → `render_map` montre l'apocalypse PHYSIQUE
SANS shader. On a ajouté la LECTURE et la mise en scène.
- ✅ *Façade* : `scps_endgame_info` (entropie 0-100 + bande · augure · fin · merveille
  · intensités · épicentre) + `scps_region_sunken`.
- ✅ **`EndgameBanner`** (haut-centre) : la jauge d'entropie monte par bandes (Stable
  → Frémissante → Instable → Au bord) avec des **augures** qui escaladent ; au
  déclenchement, un **bandeau rouge** nomme la fin. **Vérifié** (seed 9 : RONCES an-181).
- ✅ **Épicentre** : anneaux pulsants (Overlay, horloge mur) teintés par type de fin.
- ⏳ *Reste (optionnel)* : **particules GPU** (neige du Grand Hiver, écume), shader de
  rift sur l'eau engloutie, `TileMap` autotiling pour les côtes.

## Phase 5 — Le shell de jeu **[+façade]**

- *Façade* : `save(path)` / `load(path)` (le format C existe déjà).
- Menu, sauvegarde/chargement, options, **i18n** (tes `STR_*` → traduction Godot).
- **Export** : desktop + **Web (WASM)** + mobile.

---

## Transversal

- **Thème** Godot cohérent (un `.tres`) — quand l'UI prend forme (Phase 2+).
- **Déterminisme** : la sim reste 100 % C ; `make scps_api_demo` (banc façade)
  est le garde-fou — il échoue si la reproductibilité casse.
- **Le viewer SDL reste** le front de référence/debug tant que Godot n'a pas
  rattrapé ; les deux lisent le même moteur via la façade. On ne supprime SDL
  que quand Godot fait tout ce qu'il fait.

## Le prochain pas concret

Phase 1 est **entièrement [Godot]** → on la finit sans rouvrir le moteur. Puis,
quand tu rallumes le moteur, Phase 2 commence par **3-4 getters façade** et les
panneaux readout décollent.
