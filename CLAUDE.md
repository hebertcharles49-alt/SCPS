# SCPS — conventions du dépôt

## Build & vérification

- `make` → `core_demo` (35 contrôles auto-vérifiés, sortie ≠ 0 si échec).
- `make <x>_demo` : chaque module a son banc auto-vérifiant — **tout doit rester vert**. `ai_demo` : 23/23 depuis la graine par défaut 9 (L4-2026-06 ; le test « Bâtisseur +K » reste sensible au monde, documenté en AUDIT.md).
- `make chronicle && ./chronicle <graine> <sims> <années> [empires] [cités] [continents]` : le balayage headless — la télémétrie est la preuve d'équilibre.
- `make asan && ./chronicle_asan …` : ASan+UBSan doivent rester muets.
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
  par leurs Centres. `intertrade_seed_centres(w,e)`. Cascade ASSUMÉE : marchés rares →
  accès marchand plus dur → développement plus lent → `EMPIRE_CAP` recalé 10300→**13000**
  (le doublement ~96k tient). À VENIR (#5 suite) : le **pump à 2 étages** (marché LOCAL
  cité-état à rendement dégressif + marché MONDIAL via Centre, double taxe : `stock_market`
  local & global) ; et la nourriture ∝ fertilité (régions stériles-riches ⇒ import).
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
