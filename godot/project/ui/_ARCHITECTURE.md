# UI SCPS — le standard (revue 2026-07-21)

Doc court : les règles que TOUT nouveau panneau suit. L'historique vit dans git.

## 0. UNE fenêtre native, JAMAIS un système de plus (2026-07-21)

**Panneau = conteneurs NATIFS Godot + ParchTheme. Jamais de `_draw`/`VKit.text` pour un
panneau, et jamais un NOUVEAU composant-fenêtre : le chrome de référence existe déjà.**

- Le patron de fait : `empire_window` (page-stack) et `budget_panel_v2` (header +
  onglets + corps défilable). Un nouveau panneau REPREND ce patron — il ne réinvente
  pas un shell, et surtout on n'ajoute pas un 5e composant à côté.
- État : la MAJORITÉ est déjà native + sous-onglets (budget, empire_window,
  province_panel_v2, construction_panel, country_actions…). Ce qui reste VRAIMENT
  dessiné = **deux fichiers** : `sidebar_drawer` (219 draws) et `province_detail`
  (114), plus des petits (country_panel, opinion_bar, alerts).
- RATCHET : le jour où on touche un des 2 dessinés, on le ramène sur le chrome
  EXISTANT (le style empire_window/budget), pas sur un composant neuf. La dette réelle
  n'est pas « pas de fenêtre générique », c'est « 2 panneaux encore en _draw » + « le
  chrome dupliqué entre natifs » — à mutualiser SI on y retouche, pas en big-bang.

## 1. Rebuild vs widgets persistants — LA RÈGLE

**Rebuild du contenu à chaque refresh, SAUF pour les widgets à ÉTAT.**

- **Rebuild** (défaut) : lignes de valeurs, listes, cartes, sections. Simple, sûr,
  acceptable à cadence mensuelle/30fps. Exemples : sidebar_drawer, empire_window,
  province_panel_v2.
- **Persistant** (obligatoire) dès qu'un widget porte un état utilisateur :
  - `HSlider` (position en cours de drag — gate `has_focus()` avant tout
    `set_value_no_signal`, cf. budget_panel_v2) ;
  - conteneurs à **scroll** (la position se perd au rebuild — cf. tech_panel) ;
  - champs de saisie, boutons armés (confirmation 4 s), popups.
- Un panneau MIXTE construit son squelette persistant dans `_build_*()` (appelé
  une fois) et ne rafraîchit que les `Label.text`/couleurs dans `refresh()`
  (motif `_set_m`/`_val_lbls` de budget_panel_v2 — le dictionnaire clé→Label).
- INTERDIT : dupliquer un panneau parce que le refresh casse un widget — corriger
  le widget en persistant (le doublon EconomyPage est l'anti-exemple historique).

## 2. GDScript typé — LA RÈGLE

- **Toute signature PUBLIQUE (func sans `_`) est typée** : paramètres ET retour
  (`-> Dictionary`, `-> void`, `Array[int]`). Les internes suivent quand c'est
  gratuit.
- À la frontière C++ (ScpsWorld) : les retours sont des `Dictionary`/`Array`
  non typés par nature — les consommer via `.get("clé", défaut)` TOUJOURS avec
  défaut, et caster explicitement (`int(...)`, `float(...)`, `String(...)`).
  Jamais un accès `d["clé"]` nu sur un dict venu du binding.
- Nouveau fichier = typé dès la première ligne ; fichier existant = typer les
  signatures qu'on TOUCHE (ratchet opportuniste, pas de vague big-bang).

## 3. Tooltips — LA FORMULE ET LE SURVOL PARTAGÉS

- La formule valeur→icône→fiche vit dans `ui/tooltip_factory.gd` (static). Un
  panneau qui montre un bien/une ressource au survol l'appelle — il ne recompose
  JAMAIS sa propre fiche stock/prix.
- Les ZONES de survol d'un panneau dessiné vivent dans `ui/hover_zones.gd`
  (stockage + hit-test + filtre d'en-tête, un seul endroit) : `_hover.clear()`
  au début du draw, `_hover.add(...)`/`add_dict(...)` pendant, puis SOIT
  `to_tips(min_y)` vers le TooltipServer (sidebar_drawer), SOIT
  `hit_text(pos)` pour un tooltip local (province_detail — la fusion vers le
  serveur est le pas suivant).
- Le glossaire des CONCEPTS (mots du jeu) reste `ui/concepts.gd` (hover décoré
  par TooltipServer) : factory = les DONNÉES d'un bien, concepts = la
  DÉFINITION d'un mot.

## 3bis. Refus : la CHECKLIST, jamais une raison figée (2026-07-21)

Un bouton grisé n'affiche PAS une seule raison (le premier verrou) — il affiche au
hover la LISTE des conditions, chacune ✓ remplie / ✗ manquante (motif CK3/KoH2 « pourquoi
c'est grisé »). Le joueur voit TOUT ce qu'il faut, pas le premier blocage rencontré.

- **Le moteur expose les conditions** (l'UI ne reconstruit AUCUNE règle) : une struct
  `ScpsGateCond {label, ok}` remplie côté C, à côté du `allowed`/`reason` existants
  (additif, rétrocompatible). Cf. `ScpsActionLegal.conds[]` (scps_api.h) rempli par
  `da_fill_conds` (scps_api.c) — le patron pour les autres refus.
- **CONTRAT `ET(conds) == allowed`** — garanti au banc (scps_api_demo). Jamais « tout
  coché mais grisé ». Quand un préalable masque une condition (déjà-en-guerre masque
  « créneau libre »), la masquée est ✓, le préalable ✗ porte le refus.
- **Le rendu** : `TooltipFactory.gate_checklist(conds, header)` → ✓/✗ multi-lignes,
  servi par le TooltipServer. Repli legacy (`reason_label`) si le moteur n'a pas de conds
  (DLL antérieure).
- **RATCHET** : diplo (7 verbes) est câblé. Les autres refus (emprunt, esclavage
  achat/vente, construction, colonisation) suivent le MÊME patron quand on les touche —
  chacun expose sa gate-list, l'UI la rend avec `gate_checklist`.

## 4. Les gros fichiers (sidebar_drawer ~2600 · overlay ~3800)

Scission en sous-Controls = le prochain pas naturel, PAS urgent tant que ça
tourne (revue 2026-07-21). Ne pas y ajouter de nouveau panneau : un nouvel
onglet du tiroir naît dans SON fichier.

## 5. Les hot paths de draw — LA DISCIPLINE

Tout ce qui tourne dans `_draw()`/`_process()` à chaque frame est suspect :
- un calcul stable entre deux ticks se CACHE, invalidé par les signaux existants
  (`_owner_sig` poll, `_fog_dirty`, `_borders_dirty`, `generated`) — exemples :
  ACP des noms d'empire (`_name_anchor`/`_names_dirty`), hachures de guerre
  (précalculées au rebuild de `_war_regions`), vignettes de bourg (`_town_cache`
  par sid), stamps (`_dress_tex`).
- le CLIPPING/la géométrie coûteuse vit au REBUILD (cadence tick), le draw ne
  fait que projeter (`iso_pos`) et tracer.
- membrane inchangée : display-only, jamais un flottant moteur.

## Doctrine générale (rappel CLAUDE.md)

Topbar = national · droite = menu/monde · gauche = contextuel · ≤ 3 clics ·
hover = détail · /mois réels · une métrique, un mot, un chiffre — pas de phrase
dans le corps d'un panneau (le pourquoi vit au hover, concepts.gd).
