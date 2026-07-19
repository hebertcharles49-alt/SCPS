# CARTOGRAPHIE UI — la carte complète des surfaces, de l'information et des interactions

> **RÈGLE DE MAINTENANCE : toute vague UI met à jour cette cartographie dans le même
> commit que ses changements.** Un panneau ajouté/retiré, un raccourci recâblé, un
> onglet déplacé : cette page suit, ou elle ment.
>
> Référence : HEAD `296a8c2` · rédigée le 2026-07-17 · périmètre `godot/project/**`
> (lecture seule, aucune ligne de moteur C). ⚠ Un agent **UI-POLISH** modifiait
> `godot/project/ui/**` en parallèle de cette mission — les faits ci-dessous sont
> une PHOTO à HEAD `296a8c2` ; les fichiers marqués « en évolution » peuvent avoir
> bougé depuis. Toute ambiguïté non tranchable à la lecture du code est notée
> « à vérifier en jeu », jamais inventée.
>
> **MISE À JOUR UI-DOCTRINE (D1-D3, 2026-07-18, HEAD `d557685`)** : `province_panel.gd`
> (legacy) est SUPPRIMÉ — `province_panel_v2.gd` est désormais LA SEULE fiche province
> (clic-carte + touche V), avec le pied d'actions gouvernemental/diplomatique/colonisation
> porté depuis le legacy. Le Créateur de Foi est rebranché (touche R + lien dans la
> Fenêtre Empire → Population). Les curseurs fiscaux de la Fenêtre Empire → Économie
> sont passés en LECTURE SEULE (lien vers le Trésor). Tous les détails ci-dessous sont
> corrigés en place (pas une note à part) ; §D.1 garde l'historique barré pour mémoire.
>
> **CLÔTURE UI-DOCTRINE (D1-D7 complets, 2026-07-18, HEAD `49dcacb`+)** : la vague
> entière est livrée — D4 glossaire hover (§C.4), D5 coûts matières exacts (§B ligne
> « Coût matières »), D6 tuning sonore (§A.1 Options), D7 tailles d'icônes (verdict
> rail-médaillons : FAUX POSITIF, l'art des PNG `menu_*` est lui-même un médaillon —
> aucun doublon de code, cf. TROUVAILLES D7). Audit 3-clics final : cf. fin de §B.
>
> **MISE À JOUR CARTE PARCHEMIN (2026-07-19)** : les bourgs portent désormais une
> ombre douce réellement projetée depuis leur silhouette et mise en cache ; les hameaux
> libres sont plus discrets. Le fog possède son propre cycle d'invalidation annuel et
> territorial, reste opaque au cœur et reçoit un grain sépia. Le sol suit une saison
> `day_of_year/365` très légère, appliquée avant le lavis d'endgame ; le lavis politique
> varie à basse fréquence par pays et les falaises cassent la régularité de leurs ticks.
> Les routes et montagnes sont volontairement inchangées : les premières rejoignent déjà
> les bourgs sous les vignettes, les secondes ont déjà leurs deux échelles de détail.

---

## A. L'INVENTAIRE DES SURFACES

Convention d'ancrage : *topbar* = bandeau haut pleine largeur · *rail G* = bande
gauche 64 px (`Frame.SIDEBAR_W`) · *rail D* = bande droite 288 px (`Frame.LEDGER_W`,
`empire_sidebar.gd`) · *flottant* = `Control`/`PanelContainer` positionné en dur,
déplaçable par bandeau-titre (`main.gd:436-440`, groupe `draggable`) · *plein écran* =
`CenterContainer` + voile.

Trois mécanismes de rendu cohabitent, unifiés dans la même palette parchemin
(« elle est passée où la DA juste avant ? », 2026-07-14) mais PAS dans le même code :
**VKit** (`vkit.gd`, dessin immédiat `_draw()`), **ParchTheme** (`parch_theme.gd`,
`Theme` Godot natif consommé par les panneaux à conteneurs), **UiTheme** (`ui_theme.gd`,
le `Theme` de fenêtre global posé une fois par `main.gd:61`, filet pour tout Control
sans thème propre). Les écrans de menu/shell (menu principal, Options, Nouvelle
Partie, Codex, Annales, Mémoire, Recherche, DevPanel, Feedback, Épilogue, Récap
d'âge, boîtes d'évènement) redéfinissent chacun leur PROPRE jeu de constantes
`C_BG/C_PANEL/C_EDGE/C_TEXT/C_DIM/C_TITLE` en `Color()` littéraux (ex.
`menu_root.gd:14-20`, `options_panel.gd:19-23`, `new_game_panel.gd:18-23`) plutôt que
de référencer `VKit`/`ParchTheme` — les valeurs sont proches (cohérence par
coïncidence), pas la même source. Aucun hérétique de PALETTE trouvé (rien en dehors
de la famille ivoire/brun/or) — seulement cette duplication de constantes.

### A.1 — Écrans plein écran (shell, avant/hors partie)

| Surface | Fichier | Ouverture | Ancrage | Thème | Structure | Fermeture |
|---|---|---|---|---|---|---|
| Menu principal (Jouer/Charger/Options/Codex/Quitter) | `main/main.gd` instancie `ui/menu_root.gd` | au boot ; réouvert par Échap en jeu (`main.gd:531,536`) | plein écran, par-dessus la carte | palette locale (`menu_root.gd:14-20`) | plat (5 boutons) | bouton propre à chaque sous-écran (`_show(_main)`) |
| Nouvelle Partie | `ui/new_game_panel.gd` | bouton « Jouer » du menu | plein écran (fils de `menu_root`) | palette locale | plat : slider taille + liste d'empires (slot 0=joueur, 1..N=IA) | bouton « Retour » (`signal back`) |
| Créateur d'empire/culture | `ui/culture_creator.gd` | depuis un slot de Nouvelle Partie **ou** touche **C** en jeu (mode « autonome », commentaire `culture_creator.gd:29`) | plein écran | palette locale | onglets natifs (`TabContainer`, seul autre du projet avec `memory_panel.gd`) : Héritage · Éthos · Traditions · Identité | bouton propre (`cancelled`/`started`/`composed`) |
| Options (langue, plein écran, échelle UI, son : 4 curseurs Général/Musique/Effets/UI) | `ui/options_panel.gd` | bouton « Options » du menu | plein écran | palette locale | plat (section « Son » séparée par un `HSeparator`) | bouton « Retour » |
| Charger / Sauvegarder | `menu_root.gd::_build_load()` | bouton « Charger » du menu | plein écran | palette locale | liste de slots (Sauver/Charger par ligne) | bouton « Retour » |

### A.2 — Chrome permanent (visible en jeu en continu)

| Surface | Fichier | Ancrage | Thème | Structure | Notes |
|---|---|---|---|---|---|
| **Topbar** — 4 blocs ROYAUME·ÉCONOMIE·POLITIQUE·TEMPS | `ui/topbar.gd` | topbar, pleine largeur | VKit | plat, cellules façon CK3 (icône+valeur+delta), séparateurs `_block_sep` | chaque cellule est cliquable → `navigate_requested` (routage `InfoRef`, `main.gd:630-704`) ; hover = carte détaillée (`get_info_card`) |
| **Rail gauche** — 8 icônes menu | `ui/sidebar.gd` | rail G, pleine hauteur | VKit (`IconButton`) | 8 boutons verticaux, un seul actif à la fois | **les 8, dans l'ordre F1→F8** : Économie · Démographie · Stocks · Marché · Armée · Filtres · Diplomatie · Conseil (`sidebar.gd:18-27`) |
| **Tiroir du rail gauche** | `ui/sidebar_drawer.gd` | s'ouvre à droite du rail G (même bande que la fiche province — mutuellement exclusifs, `main.gd:129`) | VKit (dessin immédiat, `_draw_eco/_draw_demo/_draw_stocks/_draw_marche/_draw_armee/_draw_filtres/_draw_diplo/_draw_conseil`) | 8 panneaux plats (PAS de `TabContainer` — un `match _tab` qui appelle une fonction `_draw_*` par onglet), scroll générique par onglet | 1897 lignes, le plus gros fichier UI du projet |
| **Rail droit / Empire Sidebar** — résumé + journal | `ui/empire_sidebar.gd` | rail D, pleine hauteur, repliable (languette) | VKit | 9 sections empilées, CHACUNE repliable indépendamment (`_fold`, clic sur bandeau) : ÂGE · ÉMISSAIRE · GUERRES · NOTIFICATIONS · VILLES · ARMÉES · COLONISATION · MISSION · JOURNAL | le JOURNAL est un ring persistant (200 entrées, `alerts.gd::JOURNAL_MAX`) — seule trace DURABLE des notifications |
| Barres de carte (bas) : mode + zoom | `ui/controls.gd` | flottant bas-gauche (5 icônes mode) + bas-droite (3 icônes zoom) | VKit / `IconButton` | plat | **5 icônes du bas** = Terrain · Politique · Régions · Pays · Nature (toggle, `controls.gd:14-19,60-63`) ; **3 icônes en bas à droite** = zoom + / zoom − / cadrer (`controls.gd:68`) |
| Bandeau d'entropie / destin du monde (§27) | `ui/endgame_banner.gd` | flottant haut-centre | VKit/`PanelContainer` | plat, barre + curseur teinté | masqué tant que l'entropie < 25 % (`SHOW_FROM`) |
| Tooltip universel (survol à concepts) | `ui/tooltip_server.gd` | `CanvasLayer` layer 120, suit la souris | — | cascade récursive (verrouillage 1 s, mots turquoise cliquables) | remplace le tooltip natif Godot (délai natif désactivé, `main.gd:66`) |
| Bouton « Signaler un bug » | `ui/feedback.gd` | `CanvasLayer` layer 80, coin flottant, TOUJOURS visible | palette locale | plat (formulaire) | détecte aussi les crashs au redémarrage |

### A.3 — Fenêtres/panneaux contextuels (ouverts par une action joueur)

| Surface | Fichier | Ouverture | Ancrage | Thème | Structure | Fermeture |
|---|---|---|---|---|---|---|
| **Fiche province — LA SEULE** (D1-UNIFICATION, ex-legacy+V2) | `ui/province_panel_v2.gd` | clic sur une province (`main.gd::_on_province_picked`, AUTOMATIQUE) OU touche **V** (bascule visibilité) | flottant, `(Frame.SIDEBAR_W+14, Frame.TOPBAR_H+12)` | ParchTheme (conteneurs natifs) | 3 onglets (Infrastructure · **Région**, vue agrégée nommée · Militaire) + PIED D'ACTIONS fixe hors-onglets (Réprimer/Assimiler/Purger 2-clics/Détail si mienne · Coloniser si vierge légale · Attaquer/Route/Piller si étrangère) | ✕ (nouveau) ou Échap (`_clear_selection`, UN seul appui — retiré de la liste générique `_close_topmost`/`major_open`, cf. §D.1.2 note) |
| Détail de province (sous-onglets) | `ui/province_detail.gd` | bouton « Détail… » du pied d'actions (`province_panel_v2.gd` → `detail_requested`, `main.gd`) | flottant, même ancre que la fiche | VKit (dessin immédiat) | 6 sous-onglets : Peuples · Production · Constructions · Journal · Main-d'œuvre · Contexte | ✕ ou Échap ; REMPLACE la fiche province tant qu'il est ouvert (zone contextuelle unique, `main.gd`) |
| **Menu Construction** | `ui/construction_panel.gd` | bouton « Construire… » (fiche V2, détail, ou détail-onglet Constructions) — **aucun raccourci clavier direct** | flottant, se colle au bord droit du panneau appelant (`main.gd:182-200`) | ParchTheme | 2 onglets : Édifices · Manufactures ; une CARTE par bâtiment (icône+prix+effet+entretien+ressources+« Prochain palier ») | ✕ |
| Panneau pays (étranger) | `ui/country_panel.gd` | clic sur une province d'un pays ≠ joueur | flottant, coin haut-droit (à gauche du rail D) | VKit | plat, 5 jauges + mission | ✕ ou Échap |
| **Fenêtre diplomatique par pays** | `ui/country_actions.gd` | clic droit sur la carte, ou liste Diplomatie (tiroir F7) | flottant, tiroir collé au rail G (28 % largeur viewport, 420-540 px) | palette locale (`VKit.COL_PANEL`/`COL_EDGE` directement) | plat + 2 tiroirs repliables (Actions économiques / Actions antagonistes) + tiroir « Conditions de paix » | ✕ ou Échap |
| **Fenêtre Empire** (gestion) | `ui/empire_window.gd` | touche **E** (`main.gd:570-578`) | flottant, `(150,80)` | ParchTheme | 4 onglets : **Économie** (réutilise `economy_page.gd`, LECTURE SEULE depuis D1.2 — lien « Régler… → Trésor (B) ») · **Population** (section Foi/Religion porte un lien « Foi d'État : X (R) », D2) · Diplomatie · Conseil | Échap (pile `_panel_stack`, corrigé UI-POLISH) |
| **Trésor / Budget V2** | `ui/budget_panel_v2.gd` | touche **B** (`main.gd:556-561`) | flottant, `(120,90)` | ParchTheme | 4 onglets : Balance · Monnaie · Marché · **Commerce (vide, hors périmètre assumé)** | Échap (pile `_panel_stack`, corrigé UI-POLISH) |
| Économie dans le temps (graphes) | `ui/economy_panel.gd` | tiroir Économie (F1) → bouton « Courbes dans le temps » (`sidebar.gd::charts_requested`) | flottant, centré, adaptatif | VKit + Easy Charts (`LineChart`) | plat, menu déroulant de métrique (Population/Trésor/Prospérité) | ✕ |
| **Arbre de technologie** | `ui/tech_panel.gd` | touche **T** (`main.gd:546-555`), ou clic Savoir topbar (legacy), ou navigation (`InfoRef.TECH`) | flottant, quasi plein-cadre entre rail G et rail D | ParchTheme (chrome) + dessin immédiat (grille) | 3 couloirs horizontaux (Savoir/Forge/Société) × colonnes de paliers défilantes ; pied = dossier du nœud sélectionné + bande de métabolisation | ✕ ou Échap |
| Popup de découverte technologique | `ui/tech_popup.gd` | automatique, enfant de `tech_panel.gd`, à la complétion d'une recherche (≥85 % puis cible change) | flottant, centré sur `tech_panel` | ParchTheme | plat (effets + flavor) | bouton propre |
| **Panneau Armée** | `ui/army_panel.gd` | sélection d'un pion armée sur la carte (`map.army_selection_changed`) | flottant bas-gauche, au-dessus de la barre basse | ParchTheme | 2 onglets : Composition · Combat (bascule AUTOMATIQUE sur Combat si un engagement s'allume) | désélection sur la carte (pas dans la pile Échap) |
| Panneau de combat (siège/bataille) | `ui/battle_panel.gd` | clic sur une région en guerre/un jeton (`main.gd:813-816`) | flottant, coin haut-droit (avant le rail D) | VKit (dessin immédiat) | plat | ✕ ou Échap |
| **Créateur de foi / Religion** | `ui/religion_panel.gd` | **automatique** au 1ᵉʳ édifice religieux (`main.gd`, une fois) OU clic sur l'alerte « fondation prête » (`alerts.gd`) OU **touche R** (D2, rebranchée) OU lien « Foi d'État : X (R) » (Fenêtre Empire → Population, D2) | plein écran (voile + panneau centré) | palette locale (référence `VKit.COL_PANEL`) | plat : Crédo + 3 traditions + section Lettré + bouton Schisme | bouton « Fermer », touche **R**, ou Échap (`visibility_changed`, D2 — le jeu reprend quel que soit le chemin, corrigé un blocage en pause via Échap) |
| Codex des verbes | `ui/codex.gd` | menu Échap → bouton « Codex » (`menu_root.gd:156` → `main.gd:262-265`) ; ou recherche universelle (`InfoRef.CODEX`) | plein écran centré | palette locale | 6 catégories repliables + recherche + sommaire cliquable | bouton « Fermer » ou Échap |
| Les Annales du Règne (chronique) | `ui/chronique.gd` | touche **H** (`main.gd:579-585`) | plein écran centré | palette locale (panneau référence `VKit.COL_PANEL`) | frise chronologique, hauteur adaptative au nombre de faits | ✕/Échap |
| Écran de chapitre (récap d'âge) | `ui/age_recap.gd` | clic sur le chip « Engager : <âge> » (rail D) → `alerts.age_recap_requested` | plein écran, PRÉCÉDÉ d'une transition (`page_turn.gd`) | palette locale | plat : bilan + tranche d'annales de l'âge + bouton « Engager l'âge suivant »/« Plus tard » | Échap ou bouton |
| La page qui se tourne (transition) | `ui/page_turn.gd` | déclenchée par `age_recap.gd` (monte/tourne) | `CanvasLayer` layer 60, plein écran, INDÉPENDANT de la couche `ui` | — | animation pure | automatique |
| Épilogue (fin de partie) | `ui/epilogue.gd` | automatique à la 1ʳᵉ fin détectée (`endgame_info().fin > 0`, `main.gd:721-728`, une fois/partie) | plein écran centré | palette locale | « règne en une phrase » + frise complète des annales | bouton « Contempler » |
| Popup « OYEZ OYEZ » (évènement majeur) | `ui/event_popup.gd` | fil moteur, kinds majeurs (`alerts.gd::POPUP_KINDS = [1,2,6,7,10]` guerre/paix/révolte/sécession/directeur) | flottant, centré haut | palette locale | plat, boutons ADAPTATIFS à la situation | bouton (dont toujours « Vu ») ; PAUSE à l'ouverture, vitesse restaurée à la fermeture du dernier |
| Boîte de dialogue de décision (membrane) | `ui/event_dialog.gd` | évènement à vraie décision (dilemmes, `scps_player_event_choice`) | flottant, centré haut | palette locale | cartes de choix (action+flavor+deltas chiffrés) | choix obligatoire (PAUSE, enchaîne les décisions en attente) |
| Recherche universelle | `ui/search_palette.gd` | **Ctrl+K** (`main.gd:489-496`) | plein écran centré (voile) | palette locale | champ + liste de résultats classés (`search_rank.gd`) | Échap, Entrée (ouvre le résultat) |
| Mémoire de campagne | `ui/memory_panel.gd` | **Ctrl+M** (`main.gd:481-488`) | plein écran centré (voile) | palette locale | onglets natifs (`TabContainer`) : Récents · Épingles · Comparaison | Échap/✕ |
| DevPanel (MODTOOLS) | `ui/devpanel.gd` | **F10** (`main.gd:537-539`) | flottant centré | palette locale | liste filtrable de ~168 tunables (champ texte par tunable, PAS de slider) | F10 à nouveau, ou Échap |

**Surfaces NON instanciées / mortes ou à vérifier en jeu :**
- `alerts.gd` ne dessine plus rien à l'écran : `set_ledger_mode(true)` est posé en dur
  par `main.gd:337`, donc `visible` reste toujours `false` (`alerts.gd:143,146`) — tout
  son rendu de « chip empilé » (`_draw`, `_draw_chip`, `_draw_compact`, `CHIP`/`LABELW`)
  est du code MORT dans l'état actuel : la collecte et le routage restent la
  RÉSOLUTION UNIQUE, consommée exclusivement par `empire_sidebar.gd` via
  `ledger_rows()`/`journal_rows()`. À vérifier en jeu si un mode « chips flottants sur
  la carte » est encore accessible autrement (aucun site d'appel `set_ledger_mode(false)`
  trouvé).
- ~~`province_panel.gd` (legacy) et `province_panel_v2.gd` peuvent être visibles
  simultanément, à la MÊME ancre~~ **CORRIGÉ (UI-DOCTRINE D1, 2026-07-18)** :
  `province_panel.gd` est supprimé, `province_panel_v2.gd` est LA seule fiche province.

---

## B. LA CARTE DE L'INFORMATION (l'outil du 3-clics)

Profondeur mesurée depuis l'écran de jeu NU (carte + chrome permanent visible),
1 « clic » = 1 clic souris OU 1 pression de touche.

| Catégorie | Où (surface · onglet) | Profondeur | Notes |
|---|---|---|---|
| Trésor (or, solde net/mois) | Topbar, cellule Or | **0** (+ survol = décomposition `country_budget`) | `topbar.gd:556-566` |
| Réserve métallique / Frappe | Panneau B → onglet **Monnaie** | **2** (touche B, puis onglet) | `budget_panel_v2.gd:451-456` |
| Débase (état + curseur) | Panneau B → onglet Monnaie | **2** | `budget_panel_v2.gd:458-467` |
| Dette (total/classe/créancier/échéance) | Panneau B → onglet Monnaie | **2** | `budget_panel_v2.gd:469-474` |
| Emprunter à un ordre | Panneau B → onglet Monnaie | **2** (+1 pour confirmer, geste 2-clics) | `budget_panel_v2.gd:476-484` |
| Emprunt d'État (à un pays) | Fenêtre diplomatique par pays → « Demander un emprunt » | **2** (clic droit carte ou liste diplo, puis bouton) | `country_actions.gd:229` |
| Banqueroute volontaire | Panneau B → onglet Monnaie | **2** (+1 confirmation) | `budget_panel_v2.gd:486-497` |
| Indice des prix national | Topbar, cellule Prix | **0** (survol = tendance /mois) | `topbar.gd:571-577` |
| Prix courant par ressource | Panneau B → onglet Marché | **2** | `budget_panel_v2.gd:656-724` |
| Prix d'une ressource dans UNE province | Fiche province (clic/V) → survol d'une ligne d'allocation | **1** (+ survol) | `province_panel_v2.gd` (`province_res_price`) |
| Satisfaction/bonheur agrégé pays | Topbar, cellule Population (survol) | **0** (survol seulement) | `topbar.gd:510-529` |
| Satisfaction par classe (pays) | Fenêtre Empire (E) → onglet Population, section CLASSE | **2** | `empire_window.gd:243-252` |
| Satisfaction par classe (province) | Fiche province (clic/V) → onglet Infrastructure, ligne de classe | **1** | `province_panel_v2.gd:777-799` |
| Pop / composition par classe (pays) | Rail G, tiroir Démographie (F2) | **1** | `sidebar_drawer.gd::_draw_demo` |
| Pop / classes + culture + foi (pays, barres) | Fenêtre Empire (E) → onglet Population | **2** | `empire_window.gd:181-273` |
| Production/ressources (pays) | Rail G, tiroir Stocks (F3) | **1** | `sidebar_drawer.gd` |
| Production (province, brute+manufacturée) | Fiche province (clic/V) → onglet Région (agrégat) | **1** | `province_panel_v2.gd:511-535` |
| Entretien d'un édifice/manufacture | Fiche province (clic/V), hover sur le chip bâti | **1** (+ survol) | `province_panel_v2.gd:441-444,491-495` |
| Entretien avant construction (devis) | Menu Construction, carte du bâtiment | **2** (province puis Construire…) | `construction_panel.gd:372-383,510-517` |
| Coût matières avant construction (devis, quantités RÉELLES débitées) | Menu Construction, carte du bâtiment (puces ×qty) | **2** (province puis Construire…) | `construction_panel.gd:291-299,400` — AUDIT D5 (2026-07-18) : `scps_building_roster` (scps_api.c) renvoie la recette nue, sans le multiplicateur d'ÉTENDUE que le drain applique réellement (`agency_build_acct`, scps_agency.c:411-416) ; `_extent_mult()`/`_cost_qty_real()` le rejouent côté façade depuis `country_info().regions` pour que la puce affiche la quantité vraiment consommée |
| Arbre de tech / recherche en cours | Touche T | **1** | `tech_panel.gd` |
| Revenu de recherche décomposé | Topbar, cellule Savoir (survol) | **0** (survol) | `topbar.gd:282-301` |
| Diplo/relations (liste, opinion, guerres) | Rail G, tiroir Diplomatie (F7) | **1** | `sidebar_drawer.gd::_draw_diplo` |
| Diplo détail par pays (mémoire, engagements, verbes) | Clic droit carte / liste diplo → fenêtre pays | **1-2** | `country_actions.gd` |
| Armée sélectionnée (composition, combat) | Clic sur un pion sur la carte | **1** | `army_panel.gd` |
| Levée / réserve nationale | Rail G, tiroir Armée (F5) | **1** | `sidebar_drawer.gd::_draw_armee` |
| Religion d'État (nom, éligibilité schisme) | Touche **R** (D2, rebranchée) | **0** | `religion_panel.gd` (+ lien Fenêtre Empire → Population) |
| Culture/religion de la province (barres) | Fiche province (clic/V) → onglet Infrastructure, section PEUPLES | **1** | `province_panel_v2.gd` |
| Culture/foi agrégées du pays | Fenêtre Empire (E) → onglet Population | **2** | `empire_window.gd:181-217` |
| Entropie / destin du monde | Bandeau haut-centre | **0** (si entropie ≥ 25 %) | `endgame_banner.gd` |
| Compte pour l'Ascension (Merveille) | Touche T, pied du panneau (bande métabolisation) | **1** | `tech_panel.gd:676-709` |
| Conseil (sièges, factions) | Rail G, tiroir Conseil (F8) | **1** | `sidebar_drawer.gd::_draw_conseil` |
| Faction — tendance /mois | Topbar (survol) | **0** | `topbar.gd:625-654` |

⚠ **Au-delà de 3 clics** : aucune donnée identifiée n'exige plus de 2 clics/touches
pour être atteinte (le pire cas mesuré est 2). ~~Le seul dépassement réel était
QUALITATIF : la religion d'État redevenait introuvable (profondeur infinie) une
fois fondée~~ — **CORRIGÉ (UI-DOCTRINE D2, 2026-07-18)** : touche R (profondeur 0)
+ lien Fenêtre Empire → Population (profondeur 2).

**AUDIT 3-CLICS FINAL (clôture UI-DOCTRINE D1-D7, 2026-07-18)** — re-vérifié après
la vague complète : 0 dépassement, qualitatif ou quantitatif. Les SURFACES AJOUTÉES
par la vague respectent toutes le principe : religion d'État **0** (touche R) ·
pied d'actions de la fiche province **1** (clic province — les verbes Réprimer/
Assimiler/Purger/Coloniser/Routes y sont désormais, ex-legacy) · volumes sonores
**2** (Échap → Options, écran de menu) · coût matières RÉEL d'un bâtiment **2**
(inchangé en profondeur, désormais EXACT — audit D5) · définitions de glossaire
**+survol** sur des labels déjà à ≤ 2 (D4, jamais un clic de plus). Les tailles
d'icônes (D7) ne changent aucune profondeur.

---

## C. L'INVENTAIRE DES INTERACTIONS

### C.1 — Raccourcis clavier câblés

| Touche | Action | Source |
|---|---|---|
| Échap | pile de fermeture (panneau flottant visible → sélection → menu) | `main.gd:526-536` |
| F1 | Rail G, tiroir Économie | `main.gd:543-545` |
| F2 | Rail G, tiroir Démographie | idem |
| F3 | Rail G, tiroir Stocks | idem |
| F4 | Rail G, tiroir Marché | idem |
| F5 | Rail G, tiroir Armée | idem |
| F6 | Rail G, tiroir Filtres | idem |
| F7 | Rail G, tiroir Diplomatie | idem |
| F8 | Rail G, tiroir Conseil | idem |
| F10 | DevPanel (MODTOOLS, tunables live) | `main.gd:537-539` |
| T | Arbre de technologie | `main.gd:546-555` |
| B | Trésor / Budget V2 | `main.gd:556-561` |
| V | Fiche province (LA seule, D1) — bascule visibilité | `main.gd` |
| E | Fenêtre Empire | `main.gd:570-578` |
| H | Les Annales du Règne | `main.gd:579-585` |
| R | Créateur de Foi (D2, rebranchée) — bascule visibilité | `main.gd` |
| C | Créateur d'empire (mode autonome, en jeu) | `culture_creator.gd:29` |
| `+`/Pavé+ | Vitesse plus rapide | `main.gd:586-587` |
| `-`/Pavé− | Vitesse plus lente | `main.gd:588-589` |
| Espace | Pause/reprise (sauf si un champ de texte a le focus) | `main.gd:444,497-503` |
| Ctrl+M | Mémoire de campagne | `main.gd:481-488` |
| Ctrl+K | Recherche universelle | `main.gd:489-496` |
| Alt+← | Navigation : retour (historique de vues) | `main.gd:518-521` |
| Alt+→ | Navigation : avance | `main.gd:522-525` |
| ↑/↓/Entrée/Échap | navigation interne à la Recherche universelle (une fois ouverte) | `search_palette.gd:219-227` |

**~~⚠ Absent malgré la documentation en commentaire : `KEY_R`~~ CORRIGÉ (UI-DOCTRINE
D2, 2026-07-18)** : `KEY_R` rebranché dans `main.gd::_unhandled_input`, bascule
`religion_panel.gd` — les commentaires `religion_panel.gd:6` et `main.gd` qui le
documentaient sont désormais exacts.

**Pas de raccourci direct** (ouverture uniquement par clic/bouton) : Menu
Construction, panneau pays étranger, fenêtre diplomatique, panneau de combat,
panneau Armée, Codex (déplacé au menu Échap depuis le 2026-07-10), courbes
Économie (derrière le tiroir Économie).

### C.2 — Verbes joueur (CMD_\* / `player_*`) et leur surface

Le Codex (`ui/codex.gd:16-61`) est la table de référence FACE JOUEUR, tenue à la
main par les agents UI ; elle documente 34 verbes/décisions. Reproduite ici,
regroupée par domaine (« bientôt » = câblé façade mais sans UI, aucun trouvé
dans ce fichier actuellement) :

**Empire & Économie** (12) — bâtir un édifice · bâtir une manufacture · recruter
une unité · régler la levée · rechercher une technologie · allouer la main-d'œuvre ·
réincorporer de la population · acheter/vendre sur le marché · ouvrir/router une
route commerciale · recompléter/dissoudre l'armée · mettre une coque en chantier ·
lancer une campagne. **Peuples** (8) — réprimer · assimiler · purger · coloniser ·
proposer un pacte migratoire · affranchir les esclaves · marché servile · choisir
dans un évènement. **Diplomatie & Guerre** (11) — déclarer la guerre · fabriquer
une revendication · proposer la paix/alliance/pacte · embargo · engager/démettre un
conseiller · payer un siège du Conseil · décret · engager l'âge. **Foi & Savoir** (3)
— fonder/rallier une religion · schisme · composer sa culture. **Fin de partie** (2,
lecture seule) — consulter l'Entropie/la Merveille · consulter un combat.

Verbes NOUVEAUX (UI-MONNAIE, 2026-07-16) absents du Codex mais présents dans le
code : `player_request_loan` (emprunt d'État, `country_actions.gd:229,730-741`),
`player_borrow_class`/`player_bankruptcy` (panneau B, onglet Monnaie). **⚠ Le Codex
n'a pas été mis à jour depuis leur ajout** — écart de synchronisation entre la
table déclarative et le code réel (aucune section « Monnaie » dans `codex.gd`).

### C.3 — Curseurs (sliders)

| Curseur | Où | Plage | Verbe/tunable derrière |
|---|---|---|---|
| Taux d'imposition par classe (×3) | Panneau B → Balance **seul** (UI-DOCTRINE D1.2 : Fenêtre Empire → Économie est passée lecture seule) | 2-100 % | `player_budget_policy(0, classe, mult)` |
| Enveloppes de dépense (×6, dont Frappe) | Panneau B → Balance **seul** (idem D1.2) | 2-100 % | `player_budget_policy(1, poste, mult)` |
| Part de la réserve frappée | Panneau B → Monnaie | 2-100 % | `player_budget_policy(1, 5, mult)` |
| Sur-frappe (débase) au-delà de la parité | Panneau B → Monnaie | 2-100 % | `player_budget_policy(1, 6, mult)` |
| Fiscalité par ordre (×3, Monnaie) | Panneau B → Monnaie | 2-100 % | `player_budget_policy(0, classe, mult)` — **même curseur que « Balance », dict séparé** (`_m_sliders` vs `_sliders`) |
| Or à prendre (conditions de paix) | Fenêtre diplomatique → tiroir Paix | 0-25 (score) | `player_peace_offer(...)` |
| Taille du monde (Tiny→Huge) | Écran Nouvelle Partie | 6 paliers | `worldgen_set` (setup, pas un verbe en jeu) |

Le DevPanel (F10) n'utilise **aucun** `HSlider` — chaque tunable est un champ
texte (`LineEdit`) validé à l'Entrée (`devpanel.gd:58-67`).

### C.4 — Hovers (survol-détail)

Quasi toutes les surfaces portent du hover via le `TooltipServer` universel
(`tooltip_server.gd`, remplace le tooltip natif) : topbar (chaque cellule),
rail G/tiroir, rail D (chaque section + ligne), fiches province (les deux),
Menu Construction (chaque carte), arbre de tech (chaque nœud), fenêtre
diplomatique (chaque verbe grisé nomme sa raison), Codex (mots-concepts
turquoise cliquables, cascade récursive).

**MISE À JOUR UI-DOCTRINE D4 (glossaire hover, 2026-07-18)** : `ui/concepts.gd`
(le registre `DEFS`, déjà la source unique consommée par `TooltipServer`/Codex)
est passé de 66 à 68 entrées (+ « Frappe », + « Dette » — monnaie, jamais
définis alors que centraux au Trésor) et une CORRECTION (la clé « Credo » sans
accent ne matchait JAMAIS le mot réellement affiché « Crédo » — corrigée,
c'était une définition morte depuis sa création). Un nouveau lecteur PUBLIC,
`Concepts.def_of_label(label)`, généralise `def_of()` (correspondance EXACTE)
à un label qui CONTIENT un concept plutôt que de l'être (casse/pluriel
tolérés, réutilise le moteur de `decorate()`) — au passage, corrige un piège
vérifié du moteur RegEx de Godot : `(?i)` NE replie PAS la casse des
majuscules ACCENTUÉES (« DÉBASE » ne matchait pas la clé « Débase » malgré le
flag case-insensitive) — contourné en abaissant la casse via `String.to_lower()`
(qui, lui, replie correctement les accents français) avant l'appariement.
Câblage effectif (motif `province_panel_v2.gd::_kv` : le label porte
`tooltip_text = Concepts.def_of(...)`, jamais la valeur affichée) : le Trésor
(`budget_panel_v2.gd`, `_row`/`_m_row`/`_section` génériques — Débase, Parité,
Péages, Dette, Frappe, Entretien… tout libellé de ligne/section qui nomme un
concept) ; la Fenêtre Empire (`empire_window.gd`, `_pop_section`/`_kv_row`
génériques + Vassal/Suzerain → Vassalité et la colonne Opinion, onglet
Diplomatie, + le domaine de siège de Conseil) ; la fenêtre diplomatique par
pays (`country_actions.gd` : en-tête Opinion, statut Vassal/Suzerain, case
Vassaliser) ; le panneau pays étranger (`country_panel.gd` : Éthos) ;
l'arbre de technologie (`tech_panel.gd` : Ascension, Métabolisation, le
couloir Savoir — nouveau mécanisme `_tips`/`_get_tooltip`, panneau en dessin
immédiat pur) ; le panneau de combat (`battle_panel.gd` : Siège — même
mécanisme `_tips`, zéro hover existant avant) ; le panneau Armée
(`army_panel.gd` : Siège, section « LECTURE DU SIÈGE ») ; le Créateur de Foi
(`religion_panel.gd` : Crédo, Tradition, Schisme, Fonder/Rallier) ; les
courbes d'économie (`economy_panel.gd` : le sélecteur de métrique porte la
définition de la métrique choisie). **Collision de sens ÉVITÉE
délibérément** : « Cohésion » désigne le MORAL de bataille dans
`army_panel.gd`/`battle_panel.gd` (pas la Cohésion nationale de
`concepts.gd`) — ces libellés-là restent volontairement SANS hover généré
(un câblage aveugle aurait affiché la MAUVAISE définition). `construction_panel.gd`
et `memory_panel.gd` sont restés INTACTS : le premier route déjà tout son
survol via `get_info_card`/`Concepts.decorate()` (Or/Effet/Recette/Entretien
manufacture déjà décorés automatiquement) et toucher ses lignes « Entretien »/
« Palier » édifice exigerait de modifier `_build_info_card()`, le territoire
CONCURRENT de l'audit coûts D5 sur ce même fichier ; le second n'a aucun
Label par ligne (un seul `RichTextLabel` en tableau BBCode) — le motif
`_kv`/`tooltip_text` ne s'y applique pas sans réinventer un système.

**Sans hover riche identifié** (texte simple ou aucun) : Menu principal, Options,
Nouvelle Partie, écran Charger, DevPanel (hors `tooltip_text` natif ponctuel),
popups d'évènement (`event_popup.gd`/`event_dialog.gd` — l'effet mécanique est
DANS la carte, pas au survol, par construction — « le hover donne l'effet AVANT
le flavor » y est déjà la règle donc rien à ajouter), Épilogue, Récap d'âge,
Mémoire de campagne (`memory_panel.gd`, tableau de comparaison en BBCode — cf.
D4 ci-dessus). Ce sont majoritairement des écrans de LECTURE SEULE / one-shot
où le hover n'ajoute rien.

---

## D. LES ÉCARTS À LA DOCTRINE (le carnet de chasse)

Contre CLAUDE.md §UI (topbar=national · droite=monde · gauche=contextuel
menus→sous-onglets→détails · ≤3 clics · hover=détail · /mois réels ·
membrane MOTS). Constat froid, sourcé fichier:ligne — pas un procès.

### D.1 — Les 5 plus graves

1. ~~**Échap ne ferme pas trois fenêtres majeures.**~~ **CORRIGÉ (UI-POLISH,
   2026-07-18, item 13, TROUVAILLES.md).** `province_panel_v2.gd` (V),
   `budget_panel_v2.gd` (B) et `empire_window.gd` (E) sont désormais dans
   `_close_topmost()`/`major_open()` (`main.gd`) — Échap ferme le panneau au
   PREMIER PLAN (pile `_panel_stack`, un point d'écoute `visibility_changed` par
   panneau) ; ouvrir un panneau MAJEUR (Trésor/Diplomatie) referme aussi
   Construction (popup flottant non ancré) — la fiche province coexiste
   toujours. Vérifié en probe réelle (`uipolish_shot.gd`).

2. ~~**Trois fiches province coexistent, avec des noms de classe INCOHÉRENTS.**~~
   **CORRIGÉ (UI-DOCTRINE D1, 2026-07-18).** `province_panel.gd` (legacy) est
   SUPPRIMÉ ; `province_panel_v2.gd` est LA seule fiche province, ouverte au clic
   ET à la touche V, avec le pied d'actions (Réprimer/Assimiler/Purger/Détail/
   Coloniser/diplomatie) porté depuis le legacy. Nomenclature CANONIQUE
   (Journaliers/Bourgeois/Élites) alignée partout (`province_detail.gd`,
   `topbar.gd`). `province_detail.gd` (le « détail », sous-onglets) reste une
   surface distincte ASSUMÉE — elle REMPLACE la fiche au lieu de coexister avec
   elle (zone contextuelle unique), pas un doublon au même sens.

3. ~~**Les curseurs fiscaux/budgétaires sont réimplémentés (pas partagés) dans deux
   fenêtres.**~~ **CORRIGÉ (UI-DOCTRINE D1.2, 2026-07-18).** `economy_page.gd`
   (onglet Économie de la Fenêtre Empire) est passé LECTURE SEULE
   (`interactive = false`) — le Trésor (`budget_panel_v2.gd`, onglet Balance)
   reste la SEULE surface de réglage ; un lien explicite (« Régler… → Trésor (B) »)
   fait la jonction. Le doublon INTERNE à `budget_panel_v2.gd` (onglet Balance vs
   Monnaie, `_sliders` vs `_m_sliders`, même fenêtre) n'est PAS touché — hors
   mandat D1 (nommait explicitement `economy_page.gd` vs `budget_panel_v2.gd`).

4. ~~**Le Créateur de Foi devient introuvable après la fondation.**~~ **CORRIGÉ
   (UI-DOCTRINE D2, 2026-07-18).** `KEY_R` rebranché (`main.gd::_unhandled_input`,
   bascule visibilité) + un lien « Foi d'État : X (R) » dans la Fenêtre Empire →
   Population (section FOI/RELIGION). Corrigé en passant : fermer via Échap
   laissait le jeu en PAUSE indéfiniment (le signal `closed` ne tirait que depuis
   le bouton « Fermer ») — remplacé par `visibility_changed`, couvre tout chemin.

5. ~~**Un flux mensualisable encore affiché « or/an ».**~~ **CORRIGÉ (UI-DOCTRINE
   D1, 2026-07-18 — par suppression du fichier).** Les deux lignes
   (`province_panel.gd:317,883`) ont disparu avec le fichier legacy.
   `province_panel_v2.gd` affichait déjà « Impôts ~%s or/mois » (conforme). Un
   balayage D3 a trouvé 2 AUTRES résidus (hors des 5 graves d'origine) :
   `budget_panel_v2.gd` (« Échéance » de dette, reformulée en cadence explicite
   plutôt qu'un calcul mensuel fictif — le prélèvement est RÉELLEMENT annuel,
   `credit_year_tick`) et `province_detail.gd` (en-tête « Production en direct
   (par an) », reliquat du retrait du ×365 — corrigé en « (par jour) », l'unité
   réellement affichée par les barres).

### D.2 — Autres écarts constatés

- **Doublon de VUE budgétaire à QUATRE endroits distincts** (INFORMATION seule,
  distinct de l'INTERACTION corrigée en D.1.3) : le tiroir Économie du rail G
  (`sidebar_drawer.gd::_draw_eco`), le panneau B (Balance), la Fenêtre Empire →
  Économie (`economy_page.gd`, lecture seule depuis D1.2) et les courbes
  (`economy_panel.gd`) montrent chacun une vue partielle et redondante du même
  trésor/revenu nationaux, avec des agencements et parfois des chiffres
  légèrement différents (mensualisation recalculée séparément dans chaque
  fichier — quatre implémentations de `_grp()`/normalisation /mois trouvées,
  une par fichier). NON touché par D1 (mandat scopé à `economy_page.gd` vs
  `budget_panel_v2.gd`, l'interaction, pas les 4 vues) — reste un doublon de
  LECTURE assumé (chacune sert un contexte différent : tiroir permanent,
  panneau dédié, fenêtre de gestion, historique).
- **`alerts.gd` : une pile d'alertes entièrement câblée mais qui ne se dessine
  jamais.** `set_ledger_mode(true)` est posé sans condition (`main.gd:337`) — le
  chemin `_draw()`/`_draw_chip()`/`_draw_compact()` de ce fichier (476-502) est
  mort dans l'état actuel du jeu ; seule sa collecte (`_collect()`, `_poll_feed()`)
  sert, consommée par `empire_sidebar.gd`. Rien d'illégitime en soi, mais ~150
  lignes de rendu jamais atteintes méritent un commentaire au fichier (absent
  aujourd'hui) plutôt qu'une découverte au prochain audit.
- **Codex désynchronisé du code réel.** Les verbes de l'onglet Monnaie
  (emprunt d'État, banqueroute volontaire, emprunt à un ordre — tous
  UI-MONNAIE 2026-07-16) n'ont pas d'entrée dans `codex.gd::DOMAINS` — le
  Codex prétend documenter « tout ce que le joueur peut faire », il en a
  raté trois verbes récents.
- **Menu Construction sans raccourci clavier ni entrée de menu directe** — la
  SEULE information de « ce qu'un bâtiment coûte AVANT de posséder une
  province » (l'entretien, les recettes) exige : sélectionner une province à
  soi (1) → bouton Construire (2). Conforme à la doctrine des 3 clics, mais
  c'est la seule fenêtre « majeure » (au sens `major_open()`) sans TOUCHE
  dédiée alors que Tech/Budget/Province/Empire/Annales en ont toutes une.
- **Panneaux shell : palette dupliquée, pas de source unique.** Cf. remarque
  d'ouverture de la section A — 13 fichiers redéfinissent `C_BG/C_PANEL/
  C_EDGE/C_TEXT/C_DIM/C_TITLE` en littéraux plutôt que de référencer
  `VKit`/`ParchTheme`. Aucune dérive visuelle constatée aujourd'hui (les
  valeurs sont proches), mais un futur changement de palette centrale ne les
  atteindra pas automatiquement — dette silencieuse, pas un bug visible.
- **`army_panel.gd` hors de la pile Échap** (`main.gd:745` ne le liste pas) —
  moins grave que D.1.1 car il se ferme par désélection sur la carte (clic
  droit / re-sélection), mais reste une exception non documentée à la règle
  « tout panneau affiché doit pouvoir être dismiss [par Échap] ».
- ~~**Trois variantes de « fiche province » au total**~~ **RÉDUIT À DEUX
  (UI-DOCTRINE D1, 2026-07-18)** : `province_panel.gd` (legacy, 944 lignes) est
  supprimé. Restent `province_panel_v2.gd` (LA fiche, ~1090 lignes) et
  `province_detail.gd` (le détail, sous-onglets, ~840 lignes, REMPLACE la fiche
  plutôt que d'y coexister — zone contextuelle unique) — un jeu de vocabulaire
  UNIQUE (Journaliers/Bourgeois/Élites) partout désormais.

---

## Sommaire chiffré

> **Mis à jour UI-DOCTRINE D1-D3 (2026-07-18)** — les comptes ci-dessous reflètent
> la purge D1 (fiche province : 3→2 fichiers) ; le reste de la photo §A/B/C est
> inchangé sauf mention contraire dans le corps du texte ci-dessus.

- **36 surfaces** inventoriées en §A (5 écrans plein-écran shell, 8 éléments de
  chrome permanent, 23 fenêtres/panneaux contextuels — `province_panel.gd`
  legacy retiré), plus les 8 sous-onglets du tiroir gauche et les 6 sous-onglets
  du détail province comptés à part.
- **23 raccourcis clavier** globaux câblés (`main.gd` — `R` ajouté, D2) + 4
  raccourcis internes à la Recherche universelle.
- **34 verbes joueur** catalogués par le Codex lui-même (+ 3 verbes Monnaie
  toujours non catalogués — D.2, non touché par cette vague).
- **6 points d'implémentation de curseur** (`HSlider`), dont 1 dupliquait
  strictement le même réglage moteur dans DEUX FENÊTRES différentes — **corrigé
  (D1.2)** : `economy_page.gd` est lecture seule. Le doublon INTERNE à
  `budget_panel_v2.gd` (Balance vs Monnaie, même fenêtre, `_sliders` vs
  `_m_sliders`) reste, non mandaté par D1.
- **0 information** mesurée à plus de 2 clics de profondeur — le seul
  dépassement qualitatif du principe des 3 clics (la religion d'État
  injoignable après fondation) est **corrigé (D2)**.
