# SCPS — conventions du dépôt

## Build & vérification

- `make` → `core_demo` (35 contrôles auto-vérifiés, sortie ≠ 0 si échec).
- `make <x>_demo` : chaque module a son banc auto-vérifiant — **tout doit rester vert**. `ai_demo` : 23/23 depuis la graine par défaut 9 (L4-2026-06 ; le test « Bâtisseur +K » reste sensible au monde, documenté en AUDIT.md).
- `make chronicle && ./chronicle <graine> <sims> <années> [empires] [cités] [continents]` : le balayage headless — la télémétrie est la preuve d'équilibre.
- `make asan && ./chronicle_asan …` : ASan+UBSan doivent rester muets.
- **Re-baseline GR4 (2026-06)** : l'HÉRITAGE (ex-race) est DÉBRAYÉ de la géo — assigné
  par hash déterministe (graine × index pays), décorrélé du biome. Le biome décide
  toujours TERRAIN & RESSOURCES, plus QUEL héritage s'installe (« orques des forêts »
  possibles). Les noms SUIVENT l'héritage (country/region_make_name lisent culture.race).
  ⚠ Les mondes des graines de référence ONT CHANGÉ (qui porte quel héritage change ;
  chronique re-baselinée). GR1-GR3 (reskin race→héritage, religion→idéologie) sont PURS
  (hash chronique IDENTIQUE) ; SEUL GR4 bouge le monde.
- **Re-baseline L4 (2026-06)** : la genèse distribue désormais les empires entre
  continents (≥15 % habitable ⇒ ≥1 empire) — les mondes des graines de référence ONT
  CHANGÉ (qui est empire change). Mondes fendus (p.ex. graines 1/42) : H3 active la
  guerre trans-mer (2026-06) — calibrer la guerre sur une graine mono-continent (7/99).
- **Re-baseline Q6 (2026-06-14) — la capacité VIENT DU DÉVELOPPEMENT (48k→96k)** : monde
  âgé (`world_age=0.7` : la Pangée FEND, la mer s'éveille), genèse à **6 empires + 12
  cités-états**. An-0 = **48 000** hab, distribution **UNIFORME** (même pop par région,
  sous le plancher) ; à l'an-100 ça DIVERGE (le bâti). La pop ne croît plus vers un apex
  figé mais vers ce que le **BÂTI porte** : `eff_cap = ½·cap_pop` (la terre nue) **+
  LOGEMENTS bâtis** (manufactures UNIQUEMENT, `+HOUSE_MANUF`/niveau, plafond ½·cap_pop ≈
  25 ateliers·100) **+ grenier** (qui garde son rôle NOURRITURE). `cap_pop` = la taille
  PLEINE nourrie (le socle vivrier suit `cap_pop`). **Bâtir double la région ½→plein** ⇒
  le monde passe de 48k à ~96k au siècle par la seule CONSTRUCTION (la pop SUIT le bâti ;
  aucun taux de croissance touché ; la guerre redistribue). Tunables (registre J) :
  `EMPIRE_CAP` 10300 / `CITY_CAP` 5150 (taille nourrie) · `HOUSE_MANUF` 100 · `SEED_POP`
  48000. `cap_pop_sum` exact (zones mortes tranchées en Passe 1, RÉUTILISÉES en Passe 3).
  Le **readout** (viewer) reflète l'eff_cap moteur : les logements MONTENT quand on bâtit.
  ⚠ Pop & taille des pays ≫ qu'avant ; `ai_demo` a deux contrôles SENSIBLES AU MONDE
  (aggression/routes réalisées) rendus ROBUSTES. Diag : `SCPS_CAPDIAG` (`Σmanuf_lvl`).
  **#5 (étape 1) — les cités-états TIENNENT le marché mondial** : les Centres (hubs
  du commerce mondial) sont RARES, **N marchés = N(cités-états)/2**, plantés sur les
  meilleures cités-états (carrefour), espacés → **100 % du commerce mondial** passe
  par leurs Centres. `intertrade_seed_centres(w,e)`.
  **#5 (étape 2) — le PUMP À 2 ÉTAGES** (la chaîne joueur → cité-état → mondial) : chaque
  région se branche au marché de la cité-état (Centre) la **plus proche** (BFS multi-source
  sur l'adjacence, `hub_map_build`), à **rendement dégressif** (`MARKET_DIST_FALLOFF`/saut).
  Un achat de chantier est SOURCÉ pour de vrai (`intertrade_buy_cost`/`_market_consume`,
  lus par `agency_build_*`) : **stock PROPRE** de la région (×1, production sur place) →
  **marché LOCAL** = le stock du Centre le plus proche (×marge, péage à la cité-état hôte)
  → **marché MONDIAL** = le réseau des autres Centres (×marge×2, la **double taxe** quand
  le bien manque au local). Les achats **DÉPLÉTENT** ces stocks (pas dans le vide) ; la
  production y converge ⇒ elle a une destination. Chaque entité jouable garde SON stock
  par région (jamais drainé par autrui — seuls les Centres, le marché, sont partagés).
  Effet net POP-NEUTRE : le doublement tient (~100-102k year-100, seed 9) à `EMPIRE_CAP`
  **INCHANGÉ (13000)** — l'achat sur stock PROPRE reste ×1, ce qui compense le surcoût
  distance/double-taxe (le surcoût frappe l'import, pas la production locale). À VENIR
  (#5 suite) : la nourriture ∝ fertilité (régions stériles-riches ⇒ import).
- **V3 (2026-06-14) — LIBÉRER LE MARITIME RÉGIONAL (l'interaction est VIRTUELLE)** : le monde
  re-baseliné (Q6/L4 : Pangée fendue, empires écartés) avait mis les côtes étrangères les plus
  proches **hors** du vieux plafond de mer (72 j sur seed 7 > 60 j) ⇒ **0 route maritime**. La
  règle joueur : **deux ports + deux marchés + (pacte OU même empire) = commerce** ; la distance
  ne **FERME** plus la route — elle ne fait que **MODULER le rendement** (`routes_advance` :
  yield ∝ `1/(1+sea_days/40)`, loin = ténu, jamais nul ; hors-portée du calcul ⇒ distance
  VIRTUELLE = la borne). `routes_order` maritime ne garde donc que **DEUX PORTS** comme
  condition réelle. **`navy_best_coast`** ouvre une rade sur la meilleure côte (capitale côtière,
  sinon la côte la + peuplée) ⇒ un empire à **capitale enclavée** participe enfin à la mer ;
  l'IA navale (chronicle + viewer) bâtit le port là et trace ses routes depuis CE port. La
  **passerelle de mer** de `hub_map_build` est ROBUSTIFIÉE : toute côte déjà branchée à un marché
  par terre (Centre OU relais) fait porte — plus de dépendance à UN seul Centre côtier.
  ⚠ **RE-BASELINE** (le monde change : routes maritimes seed 7/11/19 = **6/18/20**, ex-0/3/0 ;
  commerce terrestre seed 7 **monte** 4493→4866). Sobriété : **3 routes/pays** ⇒ seed 7 (2 empires)
  plafonne à 6. SAVE **non bumpé**. Colonies outre-mer = f(géographie : côte vierge cross-continent)
  + paix (transports libres) — émergent, non forcé.
- `make scps` : le visualiseur (SDL2) — **0 warning** (`-Wall -Wextra`), toujours.

## Disciplines non négociables

- **La membrane** : `viewer.c` n'inclut jamais `scps_core.h` et ne lit aucun flottant SCPS — des MOTS (readout) et des nombres tangibles seulement.
- **On lit des coordonnées, on n'assigne jamais de modificateur** : un effet passe par les entrées du moteur (K, P, H…), jamais par un bonus plat.

## Langue (brief table de chaînes)

- **Aucune chaîne littérale face-joueur hors des tables.** Tout panneau, journal, preview, bande, tooltip à venir naît directement en `STR_*` :
  - la liste maîtresse vit dans `scps/strings_ids.h` (X-macro, texte FR de référence inline) ;
  - l'anglais est la liste jumelle `scps/strings_en.h` (même ordre — la complétude est vérifiée à la **compilation**, retirer une ligne casse le build) ;
  - appels : `tr(STR_X)`, plages `tr_band(STR_X_0, idx, n)`, paramètres **positionnels** `tr_fmt(buf, n, STR_X, a0, a1)` — `{0}..{9}`, l'ordre des mots n'est pas universel ;
  - pluriels : deux clés (`STR_X_UN` / `STR_X_PLUSIEURS`) là où c'est nécessaire, et seulement là.
- **Clôture** : `chronicle.c`, `econ_scan.c`, `batch.c`, `dump.c`, tout `printf` de télémétrie/journal console et les commentaires restent en **français, définitivement** (l'outillage de l'ingénieur, pas le jeu).
- **Surcharge runtime éditable** (rupture ASSUMÉE de « zéro asset ») : `scps_lang.txt` à côté du binaire **remplace** n'importe quel `STR_*` par son ID. Les défauts compilés restent (le binaire tourne sans le fichier) ; c'est **display-only** (le moteur/déterminisme n'y touchent pas) et c'est le mécanisme de **traduction** (un fichier = une langue surchargée). `scps_viewer --dump-lang` écrit le fichier éditable complet ; **F4** le recharge à chaud. Tout texte face-joueur naît donc en `STR_*` ⇒ devient éditable sans recompiler.
- `make lang-check` : le **cliquet** — échoue si le nombre de littéraux face-joueur dépasse la base (`scps/lang_baseline.txt`). Le reflux est attendu à mesure que la migration avance (abaisser la base à chaque extraction).
- État de migration : lexique readout (bandes, labels, hovers) + shell (menu, pause, slots, tutoriel) **migrés** ; le reste de `viewer.c` (panneaux, chips, zone_add) part de la base 64 et descend.

## Sauvegarde

- Format versionné (`SAVE_VERSION`), sections taguées, ChaCha20 (clé = obfuscation assumée, pas un secret) + empreinte FNV du clair.
- Toute valeur désérialisée qui borne une boucle ou indexe un tableau **se revalide** au chargement (`save_sane`) — refus net.
- Changer la taille d'une struct sérialisée ⇒ bump `SAVE_VERSION` (« ère antérieure »).
