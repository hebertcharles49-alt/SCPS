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

## Phase 3 — Les acteurs sur la carte **[+façade]** ← *débloquée*

Le tick plein roule DÉJÀ campagnes & guerres (les armées marchent, les sièges
tombent) — il ne reste qu'à les EXPOSER et les dessiner.
- *Façade* : getters campagne (`campaign_location/phase/units`) + tiers de ville.
- **Sprites d'armées** au `region_centroid`, animés selon la phase (marche/assaut).
- **Villes** par tier au centroïde ; **frontières** (les modes politiques existent).

## Phase 4 — Le spectacle (shaders & particules) **[+façade]**

- *Façade* : `endgame_readout` + accès `sunken[]` / epicentre.
- **Les fins §27** (EAU/FROID/RONCES) en **shaders** (rift, gel, ronces).
- **Particules GPU** : écume, neige du Grand Hiver, fumée de siège, sillages.
- **`TileMap` autotiling** (option) : côtes/routes/rivières lisses — dissout le
  problème de « continuité » à la racine.

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
