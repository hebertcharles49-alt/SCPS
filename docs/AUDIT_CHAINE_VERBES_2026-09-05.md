# Audit de la chaîne verbes → actions joueur → réglages

Date : 5 septembre 2026. Revue de l’état local du dépôt, comprenant les modifications de la session précédente. Audit par trois agents Luna, vérification et synthèse par l’orchestrateur.

## Verdict

**La chaîne est largement présente, mais elle n’est pas intégralement accessible, définie sans ambiguïté et réglable.** Les 59 commandes ont une définition, une façade, un binding et un traitement moteur. Une entrée UI active a été retracée pour 58 d’entre elles. La levée (`CMD_SET_LEVY`) reste sans contrôle joueur actif identifié. Trois actions religieuses passent directement par l’API, hors de cette chaîne de commandes.

Les 618 entrées du registre sont lisibles et inscriptibles. Cela ne prouve pas leur effet : cinq n’ont aucune lecture dans les 44 modules du moteur compilé étudiés, deux servent à la télémétrie seulement, six seuils de population restent en cache après modification, et cinq paramètres de toponymie lus par le moteur manquent au registre. Plusieurs coûts, recettes, seuils et durées restent des constantes ou tables de code.

Aucun réglage d’équilibrage ni fichier de production n’a été modifié pendant cet audit. Les choix précédents restent applicables : limite militaire fondée sur la réserve avec affichage réserve/campagne/total ; priorité aux stratégies efficaces et diverses, sans cible artificielle de satisfaction.

## Ce que signifie « complet » ici

Pour chaque commande : identifier le verbe, ses arguments et leur grain (pays/province/région/corps), le chemin UI, la façade, le traitement au tick, les conditions, le coût, le délai, l’effet et le retour d’exécution. Pour les réglages : distinguer inscription au registre, lecture réelle, effet immédiat, effet différé et valeur figée au démarrage.

La matrice couvre exhaustivement les **59 familles CMD**. Elle ne constitue pas un essai interactif de chaque combinaison de bâtiment, unité, événement ou doctrine. Les tables de contenu et les règles codées sont modifiables dans le code, mais ne sont pas pour autant des tunables F10. Les références de l’annexe UI servent à la traçabilité ; ses pistes de réglage sont subordonnées aux annexes moteur détaillées.

## Résultats mesurés

| Contrôle | Résultat | Portée |
|---|---:|---|
| Commandes réelles, hors CMD_NONE | 59 | Inventaire enum et traitement moteur |
| Commandes avec chemin UI actif retracé | 58/59 | Lecture des panneaux et de leur branchement depuis Main |
| Entrées du registre, sans doublon | 618 | Registre compilé |
| Valeurs inscriptibles puis restaurées | 618/618 | Sonde C isolée ; pas une preuve de sens gameplay |
| Modules moteur prétraités | 44/44 | Aucune erreur de prétraitement |
| Clés enregistrées sans lecture dans le moteur du jeu | 7 | Dont 2 lues dans chronicle |
| Clés lues mais absentes du registre | 5 | Toponymie |
| Clés avec défaut local différent du registre | 27 | 37 occurrences dans les unités prétraitées |
| Tunables exposés au binding Godot | 618 | Scène headless, sortie 0 |

## Ruptures à traiter en priorité

### 1. Valider les valeurs avant toute écriture de réglage

**Confirmé à l’exécution.** Le registre accepte NaN, l’infini et les valeurs négatives sans validation générique. F10 transforme `texte_invalide` en `0.0` puis écrit cette valeur. Références : `scps/scps_tune.c:30`, `:77`, `godot/project/ui/devpanel.gd:72`.

Correction proposée : refuser les valeurs non numériques et non finies, retourner un résultat explicite pour une clé inconnue, puis définir les bornes propres aux paramètres sensibles. Toutes les valeurs négatives ne sont pas nécessairement illégales : les bornes doivent être établies par famille. Afficher la valeur réellement acceptée et sa phase d’application.

### 2. Garantir qu’un refus ne prélève rien

**Vérifié par lecture des actionneurs, sans fixture dynamique de ces échecs dans cet audit.**

- Rénovation : `agency_renover_acct` débite et distribue le paiement avant l’appel final à la file. Une file pleine peut refuser après paiement (`scps/scps_agency.c:794`).
- Embarquement de réserve : les provisions peuvent être consommées avant validation de la côte, du chemin maritime ou des transports (`scps/scps_campaign.c:503`). Cela concerne le chemin de levée/embarquement, pas toute redirection d’un corps existant.
- Construction navale : le chantier prélève les stocks sans vérifier que les quantités demandées ont effectivement été obtenues ; le chantier peut démarrer en pénurie (`scps/scps_navy.c:137`). `NAVY_BUILD_SUPPLY_FLOOR` n’est pas lu.

Correction proposée : effectuer toutes les validations avant les débits, réserver la place nécessaire, puis appliquer les mutations ensemble. Tests ciblés : file pleine, côte invalide, transports insuffisants, manque de chaque ressource ; vérifier les stocks et la trésorerie avant/après.

### 3. Faire correspondre le réglage affiché à l’effet réel

Sans lecture trouvée dans le moteur : `REGION_RAW_KEEP`, `SPAWN_FOOD_RAW`, `NAVY_BUILD_SUPPLY_FLOOR`, `AI_COMPLEMENT_W`, `WILD_REGIMENTS`. Décider pour chacun si la règle reste voulue : brancher son actionneur ou retirer le réglage trompeur.

`INVARIANT_DRIFT_FRAC` et `INVARIANT_SCALE_FLOOR` servent à chronicle ; ils doivent être présentés comme réglages de diagnostic, pas comme leviers de gameplay.

Les six seuils `TIER2_POP` à `TIER7_POP` sont mémorisés au premier appel (`scps/scps_labor.c:19`). La sonde change TIER2 à 999999 : la valeur du registre change, mais une population de 2500 reste au tier 2. Un nouveau monde dans le même processus ne réinitialise pas ce cache. À l’inverse, le test de `SOLDE_OVER_K` fait bien passer le multiplicateur de solde de 4 à 6 : certains leviers fonctionnent en direct.

Les paramètres de genèse demandent également une qualification de phase : être lu par le moteur ne signifie pas agir rétroactivement sur le monde. Le CSV n’invente pas une phase quand elle n’a pas été vérifiée individuellement.

### 4. Compléter les surfaces et les contrats joueur

- `CMD_SET_LEVY` : ajouter un contrôle si la levée manuelle fait partie du jeu voulu, ou expliciter son retrait. Les scènes de tests ne prouvent pas une accessibilité dans le jeu.
- `scps_player_set_buy_rate` : API C sans binding ni UI trouvés. C’est un levier économique potentiel inaccessible, pas une commande CMD existante.
- Fondation, schisme et recrutement religieux : UI active mais mutations directes, sans journal CMD. Définir le même contrat de validation/retour que les autres actions, sans supposer que l’absence de file interdit leur fonctionnement.
- Certains retours utilisent des identifiants au lieu de noms. Un raid à butin nul peut recevoir un retour positif : préciser le résultat obtenu et le coût/cooldown, plutôt qu’assimiler automatiquement cela à une absence totale d’effet.

### 5. Rendre les profils de réglages traçables

**Collision reproduite.** La chaîne des surcharges actives est limitée à 1024 octets. Après surcharge des 618 valeurs, elle mesure 1023 caractères ; changer la dernière clé ne change plus cette chaîne. Le contrôle de configuration des sauvegardes qui dépend de cette représentation peut donc manquer une différence. Référence : `scps/scps_tune.c:23` et reconstruction dans `tune_set` ; usages dans `scps/scps_save.c`.

Correction proposée : calculer l’empreinte sur toutes les paires clé/valeur de manière canonique, indépendamment de l’affichage. Ne pas tronquer silencieusement l’entrée SCPS_TUNE. Prévoir un profil exportable et un retour aux défauts ; distinguer les réglages globaux de ceux réellement portés par la sauvegarde.

## Complément : les trois actions religieuses

| Action | Chaîne réelle | Leviers et limites |
|---|---|---|
| Fonder | religion_panel → scps_religion_found → religion_spawn ou adoption d’une foi existante si plafond atteint | Crédo et trois traditions choisis ; plafond et règles de définition dans le code ; une adoption peut remplacer la fondation demandée |
| Schismer | religion_panel → scps_religion_schism → religion_schism puis religion_fracture | Slots/pôles et crédo ; seuils SCHISM_FLIP_D=5 et SCHISM_FLIP_L=4 codés dans scps_religion.c:165 ; plafond RELIG_SCHISM_MAX de code |
| Recruter un érudit | religion_panel → scps_religion_recruit_scholar → religion_scholar_recruit | Rôle dérivé du crédo ; durée SCHOLAR_DURATION=1825 jours codée dans scps_religion.c:203 |

Les façades sont à `scps/scps_api.c:5795`, `:5819`, `:5835`. Elles valident l’existence du monde et le pays, mais ne reprennent pas le contrôle de propriété joueur d’une commande standard. La façade de schisme vérifie le plafond et le parent, sans appeler la fonction d’éligibilité utilisée par le lecteur. Ce sont des écarts de contrat à vérifier par des tests d’appels directs ; le parcours UI ne suffit pas à garantir ces préconditions.

## Défauts de registre et valeurs de secours

Les cinq lectures non enregistrées sont `TOPONYM_ETHOS_BASE`, `TOPONYM_ETHOS_REINFORCED`, `TOPONYM_HARBOR_HIGH`, `TOPONYM_ISLAND_MAX_AREA`, `TOPONYM_RIVER_MAJOR`. Elles utilisent leur valeur de secours ; SCPS_TUNE refuse ces noms et tune_set les ignore. Références : `scps/scps_toponym.c:218`, `:220`, `:222`, `:301`, `:305`.

Les différences ci-dessous ne font pas alterner deux calibrations : pour une clé connue, **le registre gagne toujours**. Elles rendent les commentaires et valeurs locales trompeurs et fragilisent une extraction ultérieure.

| Clé | Défaut du registre | Valeur(s) de secours différente(s) |
|---|---:|---|
| `SAVOIR_W_ELITE` | 0.0099999998 | 0.02 |
| `SAVOIR_W_BOURGEOIS` | 0.0049999999 | 0.01 |
| `SAVOIR_W_LABORER` | 0.0010000001 | 0.002 |
| `SPAWN_KIT_WOOD` | 250 | 50 |
| `SPAWN_KIT_FOOD` | 2000 | 100 |
| `CONSUME_ELASTIC_MAX` | 3 | 1.2 |
| `POP_R_BASE` | 0.0198 | 0.01733 |
| `IP_COLON_WPC` | 4 | 8 |
| `AGE_EXCHANGE_NODE_VALUE` | 2.5 | 1 |
| `AGE_EXCHANGE_NODE_MIN` | 10 | 4 |
| `AGE_EXCHANGE_NODE_SHARE` | 0.2 | 0.08 |
| `AGE_DISCOVERY_COUNTRY_MIN` | 8 | 6 |
| `AGE_DISCOVERY_KNOWN_PAIR_SHARE` | 0.22 | 0.12 |
| `AGE_EMPIRES_REGIONS_WORLD` | 20 | 8 |
| `AGE_EMPIRES_REGIONS_ONE_COUNTRY` | 8 | 4 |
| `AGE_SOULEVEMENTS_MIN_COUNTRIES` | 8 | 2 |
| `AGE_TYRANS_FRACTURE` | 0.30000001 | 3 |
| `AGE_TYRANS_SI` | 8.5 | 5 |
| `REFUGEE_FLEE_SCAR` | 0.40000001 | 0.5 |
| `REFUGEE_FLEE_FRAC` | 0.12 | 0.1 |
| `MANUMIT_INTEG` | 0.94999999 | 0.85 |
| `REGIMENT_PAY` | 90 | 1.5 |
| `NAVY_UPKEEP_GOLD` | 90 | 1.5 |
| `ENTROPY_TECH_W` | 0.2 | 1 |
| `HEAT_RAMP_PER_YEAR` | 0.0099999998 | 0.006 |
| `ENDGAME_BLOOD_FRAC` | 0.090000004 | 0.2 |
| `FUEL_FALLBACK_MIN` | 2 | 4 |

## Vérifications et limites

Les sondes C et Godot tournent dans le dossier de revue isolé. Elles ne modifient pas la partie de l’utilisateur. Le fichier `registry-runtime-final.log` prouve l’inscription/restauration, le cache, le levier de solde, les valeurs invalides et la collision ; `godot-chain-console-final.log` prouve l’exposition Godot et la conversion du texte invalide. Les 59 verbes ont été relus statiquement, pas tous rejoués avec toutes leurs branches de refus. Les suites complètes de la session précédente n’ont pas été relancées pour un audit sans modification de production.

L’absence de variation de `unit_roster.cout` quand REGIMENT_PRICE change n’est pas une panne : ce champ est un coût matériel. Le recrutement manuel prélève armes/population ; REGIMENT_PRICE intervient notamment dans la solde et la levée automatique. Il faut documenter les unités et le moment du prélèvement pour éviter de confondre ces prix.

L’erreur Windows `cc1.exe / libisl-23.dll` venait du chemin de recherche du compilateur lancé par notre inventaire. Les processus concernés ont été arrêtés. La DLL était déjà installée. L’inventaire utilise maintenant le chemin MSYS explicite, un précontrôle et un délai maximal ; le prétraitement des 44 modules a ensuite terminé sans popup. Cela ne nécessitait pas de réinstaller le jeu.

## Livrables et suite proposée

- `docs/AUDIT_TUNABLES_2026-09-05.csv` : les 618 clés, leurs défauts, lectures, références et phases prouvées.
- `runs/audit-verbes-2026-09-05/registry-inventory.json` : détail exploitable des lectures prétraitées et mentions sources.
- `runs/audit-verbes-2026-09-05/static-summary.json` : inventaire et anomalies.
- Les annexes suivantes contiennent les 59 chemins UI et les 59 fiches moteur, puis les mutations hors CMD regroupées par usage.

Ordre proposé pour les corrections : (1) débits et validations, (2) sûreté et traçabilité des réglages, (3) leviers inaccessibles ou inactifs, (4) métadonnées unités/bornes/phase et harmonisation des retours. Extraire les constantes d’équilibrage utiles, sans transformer les identifiants ou limites structurelles en curseurs. Chaque extraction doit conserver la valeur actuelle et être validée par un test d’effet ciblé avant toute recalibration de gameplay.

Vérification des empreintes des sources précédemment revues : **0 différence(s)**.


---

# Annexe A — Chemins UI et mutations hors CMD

# Audit chaîne commandes joueur — 2026-09-05

Périmètre : chaque `CMD_*` moteur **1 à 59** ( `CMD_NONE=0` est le sentinel), de la définition enum au drain, à la façade C, au binding Godot et à un appel UI joueur actif. Les tests, captures, scripts d’audit et scripts de session sont exclus des preuves UI. Les lignes sont des points d’ancrage ciblés, pas une affirmation basée sur un simple `has_method`.

## Contrat de lecture

- Enum et numéro : scps/scps_sim.h:68-180; exécution différée : scps/scps_sim.c dans la colonne drain.
- API C : scps/scps_api.c, avec déclarations correspondantes dans scps/scps_api.h.
- Binding : ClassDB::bind_method dans godot/src/scps_sim_node.cpp.
- Preuve UI active : les chemins sont suivis jusqu’à un panneau chargé par godot/project/main/main.gd; sidebar_drawer.gd est réellement instancié par sidebar.gd:86-92 et main.gd:144. Les tests, captures, scripts d’audit et scripts de session sont exclus.
- Doctrine 58/59 : main.gd:314-317 instancie le panneau, topbar.gd:636 émet au clic Influence, main.gd:122-130 route, puis doctrine_panel.gd:543/559 câble les boutons.
- Tunables : la colonne conserve les clés exactes lorsqu’elles sont identifiées. Les libellés génériques (module/gates/coûts) sont des points de recherche actionneur, pas des assertions de clé; l’autorité reste le moteur et scps/scps_tune_list.h. L’UI joueur n’appelle pas tune_set; devpanel.gd:72 est une surface développeur F10.

## Matrice exhaustive



| # | CMD | API C | Binding Godot | Appel UI joueur vérifié | Granularité | Accès/conditions | Drain | Tunables/actionneur |
|---:|---|---|---|---|---|---|---|---|
| 1 | CMD_BUILD | scps_player_build@scps/scps_api.c:3814 | player_build@godot/src/scps_sim_node.cpp:177 | godot/project/ui/construction_panel.gd:649; godot/project/ui/province_panel_v2.gd:735 | edifice, province PID | Province possédée; bouton Construction/fiche province, coût et gates au drain | scps_sim.c:658; agency build gates | BUILD_OWN_MATERIAL_PRICE; CLEAR_GOLD_MIN; coûts agency |
| 2 | CMD_RECRUIT | scps_player_recruit@scps/scps_api.c:3844 | player_recruit@godot/src/scps_sim_node.cpp:178 | godot/project/ui/sidebar_drawer.gd:1753 | unit type; packs de 100 hommes | Roster Armée; disponibilité tech/éthos/armes/pop et réserve | scps_sim.c:678; warhost_player_recruit | REGIMENT_PRICE; WH_ARSENAL_GATE; WH_MILICE_FLOOR |
| 3 | CMD_SET_LEVY | scps_player_set_levy@scps/scps_api.c:3850 (void) | player_set_levy@godot/src/scps_sim_node.cpp:179 | Aucun appel UI joueur actif (seulement player_session.gd:482/army_panel_shot.gd:80, exclus) | level 0..3 | Binding disponible; aucun bouton actif identifié | scps_sim.c:684; warhost_set_levy | aucun coût d'enfilage; cadence warhost WH_* au tick |
| 4 | CMD_RESEARCH | scps_player_research@scps/scps_api.c:3858 | player_research@godot/src/scps_sim_node.cpp:180 | godot/project/ui/tech_panel.gd:582 | tech index; -1 annule la cible | Arbre Tech; nœud autorisé, prérequis/points et bouton annulation | scps_sim.c:692 | TECHPOP; AI_RESEARCH_INCOME_W; coûts tech |
| 5 | CMD_DECLARE_WAR | scps_player_declare_war@scps/scps_api.c:3868 | player_declare_war@godot/src/scps_sim_node.cpp:245 | godot/project/ui/country_actions.gd:808 | target country CID | Fenêtre pays; cible connue/valide, CB et influence au lecteur | scps_sim.c:718; diplo war | INFLUENCE_COST_ENVOY; gates diplo |
| 6 | CMD_MAKE_PEACE | scps_player_make_peace@scps/scps_api.c:3873 | player_make_peace@godot/src/scps_sim_node.cpp:246 | godot/project/ui/country_actions.gd:809 | target country CID | Fenêtre pays; guerre/trêve et consentement revalidés | scps_sim.c:752; diplo peace | INFLUENCE_COST_ENVOY; DIPLO_ENVOY_FLOOR_DAYS |
| 7 | CMD_OFFER_ALLIANCE | scps_player_offer_alliance@scps/scps_api.c:3887 | player_offer_alliance@godot/src/scps_sim_node.cpp:248 | godot/project/ui/country_actions.gd:810 | target country CID | Fenêtre pays; pays connu et acceptation du vis-à-vis | scps_sim.c:804 | INFLUENCE_COST_ENVOY; DIPLO_ENVOY_FLOOR_DAYS |
| 8 | CMD_OFFER_PACT | scps_player_offer_pact@scps/scps_api.c:3892 | player_offer_pact@godot/src/scps_sim_node.cpp:249 | godot/project/ui/country_actions.gd:811 | target country CID | Fenêtre pays; cible connue et décision du vis-à-vis | scps_sim.c:814 | INFLUENCE_COST_ENVOY; DIPLO_ENVOY_FLOOR_DAYS |
| 9 | CMD_EMBARGO | scps_player_embargo@scps/scps_api.c:4007 | player_embargo@godot/src/scps_sim_node.cpp:251 | godot/project/ui/country_actions.gd:815 | target CID, on/off | Fenêtre pays; option embargo légale et cible connue | scps_sim.c:917; diplo/intertrade | INFLUENCE_COST_ENVOY; commerce |
| 10 | CMD_REPRESS | scps_player_repress@scps/scps_api.c:4171 | player_repress@godot/src/scps_sim_node.cpp:196 | godot/project/ui/province_panel_v2.gd:407; godot/project/ui/event_popup.gd:198 | province PID | Fiche province/évènement; province à soi et preview action | scps_sim.c:930 | coûts/gates agency/statecraft |
| 11 | CMD_ASSIMILATE | scps_player_assimilate@scps/scps_api.c:4176 | player_assimilate@godot/src/scps_sim_node.cpp:197 | godot/project/ui/province_panel_v2.gd:409 | province PID, creuset | Fiche province; province à soi, action et creuset disponibles | scps_sim.c:937 | coûts/gates assimilation; tech si creuset |
| 12 | CMD_PURGE | scps_player_purge@scps/scps_api.c:4181 | player_purge@godot/src/scps_sim_node.cpp:198 | godot/project/ui/province_panel_v2.gd:411 | province PID | Fiche province; province à soi et action légale | scps_sim.c:945 | coûts/gates purge |
| 13 | CMD_COUNCIL_HIRE | scps_player_council_hire@scps/scps_api.c:4291 | player_council_hire@godot/src/scps_sim_node.cpp:200 | godot/project/ui/sidebar_drawer.gd:1638 | seat, candidate slot | Conseil; siège vide/candidat disponible et or | scps_sim.c:952 | COUNCIL_*; paie/cour |
| 14 | CMD_COUNCIL_DISMISS | scps_player_council_dismiss@scps/scps_api.c:4296 | player_council_dismiss@godot/src/scps_sim_node.cpp:201 | godot/project/ui/sidebar_drawer.gd:1643 | seat | Conseil; siège occupé | scps_sim.c:960 | COUNCIL_* |
| 15 | CMD_ROUTE | scps_player_route@scps/scps_api.c:4337 | player_route@godot/src/scps_sim_node.cpp:207 | godot/project/ui/province_panel_v2.gd:420 | source/target region, maritime | Fiche province; ports/market et pacte ou même empire, route | scps_sim.c:999; routes_order | MARKET_DIST_FALLOFF; maritime route gates |
| 16 | CMD_MARKET_BUY | scps_player_market_buy@scps/scps_api.c:4342 | player_market_buy@godot/src/scps_sim_node.cpp:208 | godot/project/ui/sidebar_drawer.gd:1620 | province PID, resource, qty, tier | Stocks/Marché; capitale du joueur, stock/commerce/prix et or | scps_sim.c:1006 | IMPORT_MARGIN_*; BUILD_OWN_MATERIAL_PRICE; TRADE_LEVY |
| 17 | CMD_MARKET_SELL | scps_player_market_sell@scps/scps_api.c:4347 | player_market_sell@godot/src/scps_sim_node.cpp:209 | godot/project/ui/sidebar_drawer.gd:1622 | province PID, resource, qty, tier | Stocks/Marché; capitale, bien détenu et marché | scps_sim.c:1014 | IMPORT_MARGIN_*; TRADE_LEVY |
| 18 | CMD_CAMPAIGN | scps_player_campaign@scps/scps_api.c:4352 | player_campaign@godot/src/scps_sim_node.cpp:210 | godot/project/ui/province_panel_v2.gd:415 | from/target region | Fiche province; réserve/armée et cible régionale légale | scps_sim.c:1023; campaign_order | EMBARK_NAVAL_COST; campaign combat tunables |
| 19 | CMD_REFILL | scps_player_refill@scps/scps_api.c:4365 | player_refill@godot/src/scps_sim_node.cpp:212 | godot/project/ui/sidebar_drawer.gd:1591; godot/project/ui/empire_sidebar.gd:125 | réserve | Sidebar Armée; déficit de réserve/armes et trésor | scps_sim.c:1111; warhost | WH_ARSENAL_GATE; REGIMENT_PRICE; solde |
| 20 | CMD_NAVY_BUILD | scps_player_navy_build@scps/scps_api.c:4370 | player_navy_build@godot/src/scps_sim_node.cpp:213 | godot/project/ui/sidebar_drawer.gd:1599 | hull type | Sidebar Armée/Marché; coût, disponibilité et chantier naval | scps_sim.c:1115; navy | NAVY_UPKEEP_GOLD; NAVY_TRANSPORT_MIN |
| 21 | CMD_DISBAND | scps_player_disband@scps/scps_api.c:4375 | player_disband@godot/src/scps_sim_node.cpp:214 | godot/project/ui/sidebar_drawer.gd:1595 | réserve | Sidebar Armée; confirmation/armée présente | scps_sim.c:1121; warhost_disband | aucun tunable direct; démobilisation warhost |
| 22 | CMD_ALLOC_RAW | scps_player_alloc_raw@scps/scps_api.c:4467 | player_alloc_raw@godot/src/scps_sim_node.cpp:256 | godot/project/ui/province_detail.gd:662; godot/project/ui/province_panel_v2.gd:603 | province PID, resource, weight | Détail province; puits matière sélectionné, poids borné | scps_sim.c:1162 | allocation sans tuning direct; econ inputs |
| 23 | CMD_ALLOC_BLD | scps_player_alloc_bld@scps/scps_api.c:4472 | player_alloc_bld@godot/src/scps_sim_node.cpp:257 | godot/project/ui/province_detail.gd:664; godot/project/ui/province_panel_v2.gd:605 | province PID, building, weight | Détail province; bâtiment et poids disponibles | scps_sim.c:1172 | allocation sans tuning direct; econ inputs |
| 24 | CMD_ALLOC_INPUT | scps_player_alloc_input@scps/scps_api.c:4477 | player_alloc_input@godot/src/scps_sim_node.cpp:258 | godot/project/ui/province_detail.gd:801 | province PID, building, input on/off | Détail province; bâtiment consommateur présent | scps_sim.c:1182 | allocation sans tuning direct; econ inputs |
| 25 | CMD_ALLOC_AUTO | scps_player_alloc_auto@scps/scps_api.c:4482 | player_alloc_auto@godot/src/scps_sim_node.cpp:259 | godot/project/ui/province_detail.gd:788; godot/project/ui/province_panel_v2.gd:556 | province PID | Détail province; retour explicite au split automatique | scps_sim.c:1189 | allocation sans tuning direct |
| 26 | CMD_AGE_ENGAGE | scps_player_age_engage@scps/scps_api.c:4489 | player_age_engage@godot/src/scps_sim_node.cpp:183 | godot/project/ui/age_recap.gd:191; godot/project/ui/empire_sidebar.gd:117 | aucun (âge courant) | Récap âge/encart; âge dawned et engagement non déjà choisi | scps_sim.c:1202 | AGE_HERO_EFFICIENCY_MIN; AGE_HERO_LOYALTY_MIN |
| 27 | CMD_COLONIZE | scps_player_colonize@scps/scps_api.c:4503 | player_colonize@godot/src/scps_sim_node.cpp:190 | godot/project/ui/province_panel_v2.gd:355 | target province PID | Fiche province; province libre/frontière, flotte/nourriture/POP et cooldown | scps_sim.c:1279; navy colony | COLONY_FOOD_GATE; COLONY_MIN_POP; NAVY_COLONY_CD_DAYS |
| 28 | CMD_OFFER_MIGRATION | scps_player_offer_migration@scps/scps_api.c:3897 | player_offer_migration@godot/src/scps_sim_node.cpp:250 | godot/project/ui/country_actions.gd:812 | target country CID | Fenêtre pays; migration permise et consentement | scps_sim.c:824; diplo | INFLUENCE_COST_ENVOY; migration gates |
| 29 | CMD_BUILD_MANUF | scps_player_build_manuf@scps/scps_api.c:3909 | player_build_manuf@godot/src/scps_sim_node.cpp:224 | godot/project/ui/construction_panel.gd:653; godot/project/ui/province_detail.gd:770 | province PID, building type | Construction/province; slot, ressources, personnel et coût | scps_sim.c:845; agency_build_manuf | MANUF_BUILD_COST; BUILD_OWN_MATERIAL_PRICE |
| 30 | CMD_EVENT_CHOICE | scps_player_event_choice@scps/scps_api.c:4793 | player_event_choice@godot/src/scps_sim_node.cpp:187 | godot/project/ui/event_dialog.gd:224 | pending slot, option | Dialogue évènement; slot actif, option valide et sujet joueur | scps_sim.c:1296; events | évènement/tunables propres aux évènements |
| 31 | CMD_DECREE | scps_player_decree@scps/scps_api.c:4332 | player_decree@godot/src/scps_sim_node.cpp:206 | godot/project/ui/sidebar_drawer.gd:1151 | decree id, on/off | Conseil/Sidebar; décret connu, conditions d’entrée et réforme active | scps_sim.c:1318 | decree-specific tunables; MANUF_BUILD_COST indirect |
| 32 | CMD_MANUMIT | scps_player_manumit@scps/scps_api.c:4014 | player_manumit@godot/src/scps_sim_node.cpp:264 | godot/project/ui/sidebar_drawer.gd:1267 | aucun (pays joueur) | Sidebar; preview/âmes esclaves et décision pays | scps_sim.c:1337 | DECISION_MANUMIT_COMMUNAUTAIRE_BIAS |
| 33 | CMD_SLAVE_BUY | scps_player_slave_buy@scps/scps_api.c:4036 | player_slave_buy@godot/src/scps_sim_node.cpp:265 | godot/project/ui/sidebar_drawer.gd:1281 | province PID, count | Sidebar Marché; province à soi, stock global, gate ethos/tech | scps_sim.c:1381; intertrade | SLAVE_PRICE; SLAVE_POOL_REF; SLAVE_MARKET_CONSERVED |
| 34 | CMD_SLAVE_SELL | scps_player_slave_sell@scps/scps_api.c:4031 | player_slave_sell@godot/src/scps_sim_node.cpp:266 | godot/project/ui/sidebar_drawer.gd:1284 | province PID, count | Sidebar Marché; esclaves présents et marché | scps_sim.c:1373; intertrade | SLAVE_PRICE; SLAVE_POOL_REF |
| 35 | CMD_POP_TRANSFER | scps_player_pop_transfer@scps/scps_api.c:4082 | player_pop_transfer@godot/src/scps_sim_node.cpp:261 | godot/project/ui/province_detail.gd:823 | source/destination province PID, class, count | Détail province; deux provinces à soi, classe et quantité | scps_sim.c:1394 | migration/pop transfer gates |
| 36 | CMD_FABRICATE_CB | scps_player_fabricate_cb@scps/scps_api.c:3904 | player_fabricate_cb@godot/src/scps_sim_node.cpp:252 | godot/project/ui/country_actions.gd:816 | target country CID | Fenêtre pays; cible connue et or/influence | scps_sim.c:741; diplo fabricate | FAB_CB_COST_YEARS; FAB_MATURE_DAYS; FAB_VALID_DAYS; INFLUENCE_COST_FAB |
| 37 | CMD_COUNCIL_PAY | scps_player_council_pay@scps/scps_api.c:4304 | player_council_pay@godot/src/scps_sim_node.cpp:202 | godot/project/ui/sidebar_drawer.gd:1640; godot/project/ui/sidebar_drawer.gd:1667 | seat, pay multiplier | Conseil; siège pourvu, curseur paie borné | scps_sim.c:968 | council pay; budget army/treasury |
| 38 | CMD_RAID_COAST | scps_player_raid_coast@scps/scps_api.c:4451 | player_raid_coast@godot/src/scps_sim_node.cpp:222 | godot/project/ui/province_panel_v2.gd:422 | target province PID | Fiche province; côte ennemie, flotte pirate, pas allié/pacte, cooldown | scps_sim.c:1128; diplo pillage | PILLAGE_INCOME_FRAC; SIEGE_LOOT_FRAC |
| 39 | CMD_MOVE_ARMY | scps_player_move_army@scps/scps_api.c:4359 | player_move_army@godot/src/scps_sim_node.cpp:211 | godot/project/map/map_view.gd:346 | target region | Carte; armée sélectionnée/réserve, cible régionale | scps_sim.c:1030; campaign redirect | EMBARK_NAVAL_COST; SEA_TRAVEL |
| 40 | CMD_CORPS_RAISE | scps_player_raise_corps@scps/scps_api.c:4380 | player_raise_corps@godot/src/scps_sim_node.cpp:215 | godot/project/ui/army_panel.gd:845 | packets, target region | Panneau Armée; réserve et cible régionale | scps_sim.c:1065; campaign | campaign/army tunables |
| 41 | CMD_CORPS_SPLIT | scps_player_split_corps@scps/scps_api.c:4385 | player_split_corps@godot/src/scps_sim_node.cpp:216 | godot/project/ui/army_panel.gd:871 | corps id, packets | Panneau Armée; corps joueur actif et fraction positive | scps_sim.c:1074 | campaign composition/army tunables |
| 42 | CMD_CORPS_MERGE | scps_player_merge_corps@scps/scps_api.c:4396 | player_merge_corps@godot/src/scps_sim_node.cpp:218 | godot/project/ui/army_panel.gd:879 | destination/source corps IDs | Panneau Armée; >=2 corps sélectionnés au même lieu | scps_sim.c:1085 | campaign merge gates |
| 43 | CMD_CORPS_MOVE | scps_player_move_corps@scps/scps_api.c:4400 | player_move_corps@godot/src/scps_sim_node.cpp:219 | godot/project/map/map_view.gd:341 | corps id, target region | Carte; corps sélectionné, destination légale | scps_sim.c:1091 | EMBARK_NAVAL_COST; SEA_TRAVEL |
| 44 | CMD_CORPS_REFILL | scps_player_refill_corps@scps/scps_api.c:4404 | player_refill_corps@scps/scps_api.c:220 | godot/project/ui/army_panel.gd:831 | corps id | Panneau Armée; preview autorise renfort et ressources | scps_sim.c:1100 | campaign refill/army supply tunables |
| 45 | CMD_CORPS_DISBAND | scps_player_disband_corps@scps/scps_api.c:4408 | player_disband_corps@godot/src/scps_sim_node.cpp:221 | godot/project/ui/army_panel.gd:857 | corps id | Panneau Armée; confirmation et corps sélectionné | scps_sim.c:1106 | campaign disband gates |
| 46 | CMD_BUDGET_POLICY | scps_player_budget_policy@scps/scps_api.c:4318 | player_budget_policy@godot/src/scps_sim_node.cpp:203 | godot/project/ui/budget_panel_v2.gd:330; godot/project/ui/economy_page.gd:108; godot/project/ui/sidebar_drawer.gd:1669 | family (tax/spend), line, multiplier | BudgetV2/Économie; curseurs famille-ligne, bornes et drain | scps_sim.c:977 | INCOME_TAX_RATE_*; budget_mult; fiscal/econ tunables |
| 47 | CMD_PEACE_OFFER | scps_player_peace_offer@scps/scps_api.c:3878 | player_peace_offer@godot/src/scps_sim_node.cpp:247 | godot/project/ui/country_actions.gd:482 | target CID, regions, gold, flags | Fenêtre pays; guerre, score, régions et consentement | scps_sim.c:762 | influence/peace/diplo tunables |
| 48 | CMD_MANUF_LEVEL | scps_player_manuf_level@scps/scps_api.c:3916 | player_manuf_level@godot/src/scps_sim_node.cpp:225 | godot/project/ui/province_panel_v2.gd:713; godot/project/ui/province_panel_v2.gd:717 | province PID, building, direction | Fiche province; manufacture bâtie, coût/borne de niveau | scps_sim.c:883 | MANUF_BUILD_COST; VETUSTE_* |
| 49 | CMD_DEMOLISH_EDI | scps_player_demolish_edifice@scps/scps_api.c:3922 | player_demolish_edifice@godot/src/scps_sim_node.cpp:226 | godot/project/ui/province_panel_v2.gd:729 | province PID, edifice | Fiche province; édifice présent et confirmation | scps_sim.c:906 | construction/demolition costs and gates |
| 50 | CMD_BANKRUPTCY | scps_player_bankruptcy@scps/scps_api.c:4020 | player_bankruptcy@godot/src/scps_sim_node.cpp:278 | godot/project/ui/budget_panel_v2.gd:696 | aucun (pays joueur) | BudgetV2; bouton confirmation et dette éligible | scps_sim.c:1349; credit | BANKRUPTCY_GRACE_YEARS; BANKRUPTCY_RANCOR |
| 51 | CMD_REPAY | scps_player_repay@scps/scps_api.c:4026 | player_repay@godot/src/scps_sim_node.cpp:279 | godot/project/ui/budget_panel_v2.gd:348 | amount <=0 = maximum | BudgetV2; dette et surplus, bouton remboursement | scps_sim.c:1362; credit | PRINCIPAL_REPAY_RATE; DEBT_* |
| 52 | CMD_BORROW_CLASS | scps_player_borrow_class@scps/scps_api.c:2445 | player_borrow_class@godot/src/scps_sim_node.cpp:274 | godot/project/ui/budget_panel_v2.gd:680 | class, amount <=0 max | BudgetV2/Monnaie; classe prêteuse et capacité | scps_sim.c:991; credit | CLASS_LEND_SHARE; CLASS_EXPOSURE_SHARE; DEBT_* |
| 53 | CMD_REQUEST_LOAN | scps_player_request_loan@scps/scps_api.c:2463 | player_request_loan@godot/src/scps_sim_node.cpp:277 | godot/project/ui/country_actions.gd:847 | target country, amount <=0 max | Fenêtre pays; cible connue et capacité/consentement | scps_sim.c:834; credit/diplo | CITYSTATE_LEND_SHARE; LENDER_*; DEBT_* |
| 54 | CMD_RENOVER | scps_player_renover@scps/scps_api.c:3838 | player_renover@godot/src/scps_sim_node.cpp:239 | godot/project/ui/construction_panel.gd:397 | province PID | Construction; vétusté et province à soi | scps_sim.c:669; agency renovate | RENOV_COST_FRAC; VETUSTE_FLOOR; VETUSTE_RATE |
| 55 | CMD_SPLIT_COMP | scps_player_split_comp@scps/scps_api.c:4389 | player_split_comp@godot/src/scps_sim_node.cpp:217 | godot/project/ui/troop_select.gd:127 | corps id, inf/arch/cav/mage packets | Sélecteur de troupes; composition positive et sous-effectif | scps_sim.c:1079; campaign | campaign composition tunables |
| 56 | CMD_SEAL_DESSEIN | scps_player_seal_dessein@scps/scps_api.c:4496 | seal_dessein@godot/src/scps_sim_node.cpp:169 | godot/project/ui/desseins_panel.gd:220 | branch, rung, path | Panneau Desseins; échelon courant prêt et pivot/voie légal | scps_sim.c:1213; missions | mission pacing/reward tunables |
| 57 | CMD_DOCT_ADOPT | scps_doctrine_adopt@scps/scps_api.c:3791 | doctrine_adopt@godot/src/scps_sim_node.cpp:173 | godot/project/ui/doctrine_panel.gd:400 | slot, doctrine id | Panneau Doctrines; slot ouvert, doctrine disponible/influence | scps_sim.c:1262; doctrines | DOCT_COST_BASE; DOCT_COST_STEP; influence |
| 58 | CMD_DOCT_IDEA | scps_doctrine_buy_idea@scps/scps_api.c:3796 | doctrine_buy_idea@godot/src/scps_sim_node.cpp:174 | godot/project/ui/doctrine_panel.gd:543 | doctrine id | Panneau Doctrines; doctrine adoptée, prochaine idée et influence | scps_sim.c:1267; doctrines | IDEA_COST_BASE; IDEA_COST_STEP; influence |
| 59 | CMD_DOCT_ABANDON | scps_doctrine_abandon@scps/scps_api.c:3801 | doctrine_abandon@godot/src/scps_sim_node.cpp:175 | godot/project/ui/doctrine_panel.gd:559 | slot | Panneau Doctrines; slot occupé et abandon confirmé | scps_sim.c:1272; doctrines | aucun coût; slot/doctrine gates |

## Mutations ClassDB hors CMD

Cette annexe recense les méthodes ClassDB qui mutent un état ou pilotent le monde sans être une des commandes 1..59. Les catégories séparent gameplay joueur, préparation de partie, contrôle moteur, configuration, outils développeur et persistance.

| Action | Catégorie | API C | Binding/implémentation | UI active ou preuve d’absence | Granularité | Statut |
|---|---|---|---|---|---|---|
| religion_found | player_gameplay | scps_religion_found@scps/scps_api.c:5795 | religion_found@godot/src/scps_sim_node.cpp:306, implementation:2731 | main.gd:281-284,873-878; religion_panel.gd:140-141,278-284; reachable by first Temple, alert, Empire/Foi or R | country player, credo + 3 traditions | direct gameplay mutation, no CMD; active root proven |
| religion_schism | player_gameplay | scps_religion_schism@scps/scps_api.c:5819 | religion_schism@godot/src/scps_sim_node.cpp:308, implementation:2737 | main.gd:281-284; religion_panel.gd:136-138,286-293; panel reachable by same routes | country player, two pole slots + new credo | direct gameplay mutation, no CMD; active root proven |
| religion_recruit_scholar | player_gameplay | scps_religion_recruit_scholar@scps/scps_api.c:5835 | religion_recruit_scholar@godot/src/scps_sim_node.cpp:311, implementation:2747 | main.gd:281-284; religion_panel.gd:123-124,265-275; panel reachable by same routes | country player, capital region scholar role | direct gameplay mutation, no CMD; active root proven |
| culture_composition | new_game_setup | scps_set_empire_culture/scps_set_player_culture@scps/scps_api.c:5693/5703 | set_empire_culture/set_player_culture@godot/src/scps_sim_node.cpp:293-294,2646-2650 | main.gd:268; menu_root.gd:58; new_game_panel.gd:368; culture_creator.gd:721 | empire slot or player slot, heritage/ethos/traditions | direct setup mutation before generation; excluded from in-game CMD claims |
| climate_choice | new_game_setup | scps_set_player_climat@scps/scps_api.c:5714 | set_player_climat@godot/src/scps_sim_node.cpp:295, implementation:2654 | culture_creator.gd:720; active through NewGamePanel route | player setup climate enum | direct setup mutation before generation; no CMD |
| clear_player_culture | new_game_setup | scps_clear_player_culture@scps/scps_api.c:5707 | clear_player_culture@godot/src/scps_sim_node.cpp:296, implementation:2658 | culture_creator.gd:742; new_game_panel.gd:364 | reset setup culture | direct setup reset; no CMD |
| country_name | new_game_setup | scps_set_country_name@scps/scps_api.c:5764 | set_country_name@godot/src/scps_sim_node.cpp:297, implementation:2663 | culture_creator.gd:726-727; new_game_panel.gd:376-377 | player country display name | direct setup mutation, serialized; no CMD |
| worldgen_parameters | new_game_setup | scps_worldgen_set/clear@scps/scps_api.c:5913/5927 | worldgen_set/worldgen_clear@godot/src/scps_sim_node.cpp:299-300, implementation:2682-2697 | main.gd:268; menu_root.gd:58; new_game_panel.gd:365 | next-world generation parameters | direct setup mutation before generate; no CMD |
| advance_days | engine_control | scps_sim_advance_days@scps/scps_api.c:203 | advance_days@godot/src/scps_sim_node.cpp:48, implementation:358 | autoload/sim.gd:143; vitesse/contrôle moteur main.gd, appels actifs hors actions joueur | nombre de jours simulés | contrôle moteur; pas une mutation joueur ni une commande CMD |
| reset_command_feedback | transient_ui | scps_command_feedback_reset@scps/scps_api.c:226 | reset_command_feedback@godot/src/scps_sim_node.cpp:50, implementation:375 | autoload/sim.gd:197-200; utilisé par journal/feedback, aucun bouton joueur direct | file historique feedback transitoire | purge d affichage; pas une mutation gameplay ni une commande CMD |
| generate_world | engine_control | scps_sim_generate@scps/scps_api.c:150 | generate@godot/src/scps_sim_node.cpp:47, implementation:357 | autoload/sim.gd:60-64; called by active NewGamePanel via Sim.regenerate | whole world, seed | engine lifecycle mutation; not a player command |
| observer_mode | engine_control | scps_set_observer@scps/scps_api.c:336 | set_observer@godot/src/scps_sim_node.cpp:64, implementation:582 | new_game_panel.gd:221-224,372-373 | world human-control mode | setup mode mutation; no CMD |
| language | configuration | scps_lang_set@scps/scps_api.c:3459 | lang_set@godot/src/scps_sim_node.cpp:161, implementation:1687 | options_panel.gd:71-72 | global language 0/1 | configuration mutation; no gameplay CMD |
| tunable_override | developer_tool | scps_tune_set_val@scps/scps_api.c:3454 | tune_set@godot/src/scps_sim_node.cpp:160, implementation:1681 | devpanel.gd:72, reachable through F10 DevPanel only | named engine tunable/value | developer mutation; excluded from player UI claims |
| save_load | persistence | scps_save_game/load_game via scps_sim_save/load@scps/scps_api.c:5935/5941 | save_game/load_game@godot/src/scps_sim_node.cpp:320-321, implementation:2763-2766 | menu_root.gd:253-259 via Sim.save_game/load_game | save slot/world snapshot | persistence mutation; no gameplay CMD |
| buy_rate | unbound_api | scps_player_set_buy_rate@scps/scps_api.c:5727 | aucun ClassDB binding | aucun appel UI joueur actif trouvé | player economy category percentage | direct C API mutation; no CMD, no binding, no UI; aucun CMD correspondant |

Inventaire de contrôle : generate, advance_days, set_observer, reset_command_feedback; gameplay hors CMD religion_found, religion_schism, religion_recruit_scholar; préparation set_empire_culture, set_player_culture, set_player_climat, clear_player_culture, set_country_name, worldgen_set, worldgen_clear; développeur/configuration tune_set, lang_set; persistance save_game, load_game. Les lecteurs et méthodes *_info/*_list sont read-only et ne sont pas des mutations.
## Constats et trous

- Les lignes sidebar_drawer.gd sont des appels UI actifs : main.gd:144 charge Sidebar, puis sidebar.gd:86-92 instancie SidebarDrawer; les onglets F1-F8 le rendent atteignable.
- CMD_DOCT_IDEA (58) et CMD_DOCT_ABANDON (59) sont atteignables : main.gd:314-317 instancie DoctrinePanel; topbar.gd:636 émet doctrine_requested au clic Influence; main.gd:122-130 route le signal; doctrine_panel.gd:543 et :559 câblent les boutons.
- Religion est une famille de mutations de gameplay hors CMD, active et instanciée par main.gd:281-284. Les trois actuateurs sont religion_found, religion_schism et religion_recruit_scholar.
- tune_set est une écriture de registre réservée à DevPanel/F10; elle ne doit pas être confondue avec les actions UI joueur. Les tunables de la matrice sont à lire comme pistes moteur : les mentions génériques ne sont pas des clés exactes garanties.
- Les APIs retournant bool/enfilé ne prouvent pas l’exécution : le drain scps_sim.c et le feedback transient sont la preuve du résultat.

### Trous explicitement vérifiés

- CMD_SET_LEVY est défini/enfilable/bindé mais aucun bouton UI joueur actif identifié; les preuves player_session/army_panel_shot restent exclues.
- scps_player_set_buy_rate reste une mutation directe C non journalisée, non bindée et sans UI; aucun CMD ne lui correspond.
- Les noms exacts de certaines unités/édifices/ressources et sièges restent des IDs dans le journal faute de lecteur d’identité dédié.
- Les clés de tunables génériques ou non retrouvées littéralement dans le registre ne sont pas présentées comme des clés exactes; leur autorité reste l’actionneur moteur et scps_tune_list.h.

---

# Annexe B — Moteur, verbes 1 à 29

# Audit moteur des verbes joueur CMD 1–29

Date : 2026-09-05. Périmètre : `CMD_BUILD` à `CMD_BUILD_MANUF` dans `scps/scps_sim.h:68-106`, et leur chaîne `sim_cmd_drain` dans `scps/scps_sim.c:595-1406`. Audit statique en lecture seule ; aucun fichier de production ni tunable n’a été modifié.

## Contrat de lecture

Un appel de façade ne signifie pas une exécution : les arguments sont copiés dans `PlayerCmd`, puis revalidés au drain du tick (`scps/scps_sim.c:595`). `PENDING` signifie seulement « en file », `EXECUTED` signifie que l’actuateur a confirmé la mutation/commande, et `REFUSED` signifie validation échouée, consentement refusé, indisponibilité ou absence d’effet. Le fallback du drain (`scps/scps_sim.c:1406`) transforme tout ordre resté sans preuve en `REFUSED/NO_EFFECT`; le résultat ne dépend pas de la présence d’un pointeur feedback.

« Modifiable » sépare (J) les paramètres envoyés par le joueur, bornés par l’API/drain, et (D) les clés `SCPS_TUNE` effectivement lues sur le chemin. Les constantes de code, tables de recettes, définitions d’unités et règles diplomatiques sont listées comme fixes : elles ne sont pas réglables par le registre. Le registre accepte un flottant sans borne générique (`scps_tune.c:tune_init`), mais les helpers peuvent le borner explicitement ; les bornes indiquées ci-dessous sont donc celles du chemin réel.

## Tableau exhaustif

| # / verbe | Paramètres, unités, grain | Drain → actuateur réel | Coût, gate, délai, effet | Tunables réellement lus (D) et bornes | Feedback moteur / statut |
|---|---|---|---|---|---|
| 1 BUILD | `a0=Edifice`, `a1=pid` (`<0` capitale), province, instant de décision | `scps_sim.c:658` → `agency_build_acct` (`scps_agency.c:343`) | Province possédée, région valide, doublon/chantier/tech/marché/matière/or ; chantier `EDIFICES[e].days`, effet à l’achèvement | `BUILD_OWN_MATERIAL_PRICE=1.0` (`scps_tune_list.h:993`, plancher 0 dans agency), `TOLL_STATE_SHARE=0.5` (`:1828`, clampé 0..1). Coûts, jours, recipes et staff gates sont tables/constantes de code | `EXECUTED/STARTED` seulement après acceptation de `agency_build_acct`; sinon `REFUSED/NO_EFFECT` |
| 2 RECRUIT | `a0=UnitType`, `a1=paquets` (défaut 1), unités internes | `scps_sim.c:678` → `warhost_player_recruit` (`scps_warhost.c:443`) | Unité valide, population libre, armes/tech selon `UnitDef`; ponction d’un paquet et ajout à la réserve, immédiat | `ARMY_POOL_FRAC=0.20` (`tune_list.h:319`) est actif dans `army_class_free_ex` et clampé 0..1; `WH_POOL_CLAMP=1.0` (`:340`) active la borne de demande. `REGIMENT_PRICE=12`, `REGIMENT_PAY=90` (`:275-276`) sont solde/tick, pas le recrutement. Coûts d’armes et gates `UnitDef` sont fixes. La preview `scps_unit_roster` (`scps_api.c:3018-3050`) expose seulement `100` + catégorie d’arme dans `cout` : aucun prix-or de recrutement n’est affiché ; ici le coût réel est matériel/pop | `COMPLETED`, `value=got` uniquement si `got>0`; l’enqueue seul reste pending |
| 3 SET_LEVY | `a0=WH_LEVY_*`, posture sans unité | `scps_sim.c:684` → `warhost_set_levy` (`scps_warhost.c:215`) | Valeur bornée `WH_LEVY_BASSE..WH_LEVY_MASSE`; mutation immédiate, sans coût ni délai | Aucun D sur l’action ; les coûts/upkeep des postures sont consommés par `warhost_tick`, pas par le setter | `MUTATED(old,new)` seulement si la posture change ; l’API historique est `void`, donc le feedback est la preuve du drain |
| 4 RESEARCH | `a0=TechId`, `-1` annule, grain pays/cible unique | `scps_sim.c:692` : accès/prérequis/âge puis `research_target`; progression `sim_day` → `tech_research` (`scps_tech.c:650+`) | Pas de coût au choix ; cible revalidée, progression journalière et dépense de recherche au tick | `TECH_COST_MULT=0.70` (`tune_list.h:384`), `TECH_COST_N_EXP=0.65` (`:393`) actifs dans `tech_cost`; `TECH_COST_N_K`, floor et coûts de base sont codés. Aucun délai de commande | `MUTATED(target)` au changement/annulation ; cible indisponible `REFUSED/UNAVAILABLE`, âge non ouvert `NOT_READY` |
| 5 DECLARE_WAR | `a0=pays cible`, pays | `scps_sim.c:718` → `diplo_declare_war_cb` (`scps_diplo.c:806`) | Cible valide, pas soi/mort/guerre/trêve ; CB gratuit ou fabrication mûre. Aucun émissaire, or ou influence ; effet bilatéral immédiat | Aucun D lu par la déclaration elle-même. Les clés de fabrication (`FAB_CB_COST_YEARS=2`, `FAB_MATURE_DAYS=365`, `FAB_VALID_DAYS=1825`, `tune_list.h:1516-1518`) appartiennent au verbe de fabrication antérieur, pas à ce drain. CB/règles sont fixes | `MUTATED(DIPLO_WAR,CB)` seulement après vérification du nouveau statut; sans CB/trêve = refus sans faux succès |
| 6 MAKE_PEACE | `a0=pays cible`, pays | `scps_sim.c:752` → `ai_consider_offer(OFFER_PEACE)`, puis `diplo_make_peace` | Doit être en guerre ; consentement du vis-à-vis ; plancher d’émissaire avant le switch ; blanc immédiat si accepté, aucun coût d’influence direct | `DIPLO_ENVOY_FLOOR_DAYS=30` (`tune_list.h:2100`) est lu avant le case. Le score de consentement est une règle AI/diplomatie et n’est pas un tunable de ce verbe | `MUTATED` si le statut quitte WAR, sinon `REFUSED/NO_CONSENT` |
| 7 OFFER_ALLIANCE | `a0=pays cible`, pays | `scps_sim.c:804` → `ai_consider_offer`, `diplo_form_alliance` | Cible valide, consentement ; avant le switch, émissaire libre + influence | `INFLUENCE_COST_ENVOY=6.0` (`tune_list.h:2098`) × `influence_scale` × doctrine; `DIPLO_ENVOY_FLOOR_DAYS=30` (`:2100`), cast en jours. `influence_scale`/coût sont bornés par leurs helpers, registre sans borne générique | `MUTATED/DIPLO_ALLIED` seulement après transition; sinon `REFUSED/NO_CONSENT`, influence/cooldown explicites |
| 8 OFFER_PACT | `a0=pays cible`, pays | `scps_sim.c:814` → `ai_consider_offer`, `diplo_set_trade_pact` (`scps_diplo.c:154`) | Cible valide, consentement ; même gate émissaire/influence ; pacte bilatéral immédiat | Même D : `INFLUENCE_COST_ENVOY`, `DIPLO_ENVOY_FLOOR_DAYS`; relation/consentement reste code AI | `MUTATED(1)` uniquement si pacte passe de faux à vrai; sinon refus |
| 9 EMBARGO | `a0=pays cible`, `a1=on/off`, pays | `scps_sim.c:917` → `intertrade_order_embargo` (`scps_intertrade.c:957`) | Cible valide ; émissaire/influence ; mutation de l’embargo, flux commerciaux affectés au tick | `INFLUENCE_COST_ENVOY=6.0`, `DIPLO_ENVOY_FLOOR_DAYS=30`; aucune marge de marché n’est lue par l’actuateur d’embargo | `MUTATED` seulement si l’état change; demande déjà satisfaite = refus/no-effect |
| 10 REPRESS | `a0=pid`, province possédée | `scps_sim.c:930` → `agency_order_repress` (`scps_agency.c:630`) | Enfile l’action intérieure ; aucun coût or ; délai codé `REPRESS_DAYS=30`; effet coercition au traitement | Aucun D. `REPRESS_DAYS=30` et constantes de hit sont codés (`scps_agency.c:626+`), donc non réglables par `SCPS_TUNE` | `STARTED` à l’enfilage confirmée ; l’effet final est différé, distinct de l’acceptation |
| 11 ASSIMILATE | `a0=pid`, `a1=creuset!=0`, province | `scps_sim.c:937` → `agency_order_assimilate` (`scps_agency.c:634`) | Province possédée ; action enfilée, sans or ; `ASSIM_DAYS=365`; dérive/assimilation à l’avance de l’agence | Aucun D. `ASSIM_DAYS=365`, +0.25/+0.50 de minorité, `L=-0.5`, coercition `+0.10` sont constants de code | `STARTED` confirme le chantier, pas l’assimilation terminée |
| 12 PURGE | `a0=pid`, province | `scps_sim.c:945` → `agency_order_purge` (`scps_agency.c:636`) | Province possédée ; action enfilée sans coût ; durée `AGY_PURGE_YEARS*365`; tranches annuelles | Aucun D. `PURGE_FRAC_AN=0.12` (`scps_agency.c:628`) et durée `AGY_PURGE_YEARS` sont codées/exportées, non tunables | `STARTED` à la mise en chantier ; décès et réduction démographique sont différés |
| 13 COUNCIL_HIRE | `a0=seat`, `a1=candidate slot`, pays | `scps_sim.c:952` → `statecraft_council_hire` | Siège `0..SC_COUNCIL_SEATS-1`, candidat `0..SC_COUNCIL_CANDS-1`, siège vacant ; mutation immédiate, sans coût/délai | `COUNCIL_HIRE_LEVER=0.10` (`tune_list.h:1524`) est lu pour le levier de faction ; tiers, génération, identité et candidat sont tables/code | `MUTATED` si le siège devient effectivement pourvu; siège déjà occupé = refus/no-effect |
| 14 COUNCIL_DISMISS | `a0=seat`, pays | `scps_sim.c:960` → `statecraft_council_dismiss` | Siège valide et pourvu ; vacance immédiate, sans coût/délai | `COUNCIL_DISMISS_GRIEF=0.10` (`tune_list.h:1525`) est lu pour le grief de la faction, multiplié par le modificateur doctrine; titulaire et règles de faction sont code | `MUTATED` seulement si un conseiller était assis |
| 15 ROUTE | `a0=region A`, `a1=region B`, `a2=maritime!=0`, paire de régions | `scps_sim.c:999` → `routes_order` (`scps_routes.c:25`) | Régions distinctes/valides, source au joueur et cultures settled ; terrestre ou maritime. Mer : deux ports/côtes ; distance module le rendement, ne ferme plus la route. Le case ne vérifie pas la paix diplomatique | Aucun D de légalité. `CHOKE_EMERGENT=1.0` (`tune_list.h:2043`) et `CHOKE_REAL_PATH=0.0` (`:2028`) ne règlent que le recalcul/assignation des chokes (`routes.c:66-76,173-177`), pas l’acceptation. Navigation BFS/adjacence gratuite, sans tunable légitime | `COMPLETED` avec la route créée; échec de paire/gate = refus |
| 16 MARKET_BUY | `a0=pid`, `a1=Resource`, `a2=quantité`, `a3=tier` (0..2), province | `scps_sim.c:1006` → `intertrade_market_buy_pid` (`scps_intertrade.c:629`) | Quantité positive, ressource/tier valides, stock disponible et trésor ; achat immédiat, stock joueur crédité et marché débité | `IMPORT_MARGIN_OWN=1.3`, `IMPORT_MARGIN_THIRD=1.8`, `IMPORT_MARGIN_NONE=2.0` (`tune_list.h:226-228`), `MARKET_DIST_FALLOFF=0.12` (`:933`) sont actifs dans le prix/accès; `MARKET_MIN_PRICE=0.2` est codé (`intertrade.c:628`) | `COMPLETED(value=got,value2=spent)` uniquement si unités obtenues; `got=0` ne devient jamais succès |
| 17 MARKET_SELL | `a0=pid`, `a1=Resource`, `a2=quantité`, `a3=tier` (0..2), province | `scps_sim.c:1014` → `intertrade_market_sell_pid` (`scps_intertrade.c:636`) | Quantité positive, possession du bien/étage et marché ; débit du stock joueur et crédit d’or immédiats | Le helper de vente applique le prix courant `re->price[good]` (plancher codé `MARKET_MIN_PRICE=0.2`) sans marge d’import : `IMPORT_MARGIN_*`/`MARKET_DIST_FALLOFF` façonnent le prix courant en amont (`intertrade.c:1043-1054`), mais ne sont pas relus par `market_sell` lui-même | `COMPLETED(value=sold,value2=gained)` seulement si vente effective |
| 18 CAMPAIGN | `a0=region source`, `a1=region cible`, déplacement de la réserve, région | `scps_sim.c:1023` → `campaign_order` (le repli maritime `campaign_order_sea` est dans `CMD_MOVE_ARMY`, pas ce case) | Force au départ et chemin terrestre ; enfile une campagne, mouvement différé. `CMD_CAMPAIGN` est terrestre uniquement ; la traversée joueur est le verbe de mouvement, avec port/côte/transports revalidés | Aucun D dans ce case. `SEA_TRAVEL=1.0` (`tune_list.h:1849`) est lu seulement par le repli maritime de `CMD_MOVE_ARMY`; navigation terrestre/BFS gratuite et constante | `STARTED(from,target)` si ordre accepté ; sinon refus, jamais completed prématuré |
| 19 REFILL | aucun argument, campagne du joueur | `scps_sim.c:1111` → `campaign_can_refill` puis `campaign_refill` | Corps actif en région amie, sous nominal ; consomme population libre et armes du stock national, immédiat | `ARMY_POOL_FRAC=0.20` (`tune_list.h:319`, clampé 0..1 dans `army_class_free_ex`) est relu par `army_recruit`; `POP_PER_UNIT`, paquet matériel `2` et ressources d’armes sont codés dans `campaign.c:1362+`; upkeep n’est pas ce verbe | `COMPLETED` seulement si au moins un paquet ajouté |
| 20 NAVY_BUILD | `a0=HullType`, pays/rade choisie par moteur | `scps_sim.c:1115` → `navy_order_build` (`scps_navy.c:137`) | Un seul chantier, meilleur port, or suffisant, ouvriers suffisants ; paie trésor national, tente de débiter supplies/bois/cuivre (le débit est borné au disponible mais son retour est ignoré), équipage, puis délai `HULLS[t].days` | Aucun D de construction. `NAVY_UPKEEP_GOLD=90` est seulement entretien (`tune_list.h:370`); `NAVY_MIN_PRICE=0.5` est codé (`navy.c:19`), coûts/crew/days `HULLS` codés. Il n’existe pas de gate de stock explicite dans `navy_order_build` | `STARTED(hull)` après paiement et chantier établi; absence de port/or/bras = refus; manque de stock ne refuse pas explicitement et peut produire un débit partiel |
| 21 DISBAND | aucun argument ; campagne active sinon réserve militaire | `scps_sim.c:1121` → `campaign_disband` ou `warhost_disband` | Dissout immédiatement, restitue la force/population selon le module ; aucun coût ni délai | Aucun D direct ; règles de restitution sont code warhost/campaign | `COMPLETED` seulement si des unités ont effectivement été dissoutes |
| 22 ALLOC_RAW | `a0=pid`, `a1=Resource brute`, `a2=poids`, province | `scps_sim.c:1162` → écriture `prov[pid].alloc_raw`, `alloc_on=1` | Province possédée ; poids clampé 0..255 ; mutation immédiate, allocation normalisée au tick | Aucun D. Normalisation par somme des poids (`scps_econ.c:4278+`) et rendement sont moteurs aval, pas des paramètres de l’ordre | `MUTATED(pid,resource,weight)` uniquement si la valeur change |
| 23 ALLOC_BLD | `a0=pid`, `a1=BuildingType`, `a2=poids`, province | `scps_sim.c:1172` → `prov[pid].alloc_bld`, `alloc_on=1` | Province possédée ; bld valide ; poids clampé 0..255 ; mutation immédiate | Aucun D ; partage de main-d’œuvre au tick est aval | `MUTATED` seulement si poids différent |
| 24 ALLOC_INPUT | `a0=pid`, `a1=BuildingType`, `a2=intrant 0/1`, province | `scps_sim.c:1182` → `prov[pid].bld_input[b]` | Province/bâtiment valides ; booléisation `a2!=0`; mutation immédiate | Aucun D | `MUTATED` seulement si le bit change |
| 25 ALLOC_AUTO | `a0=pid`, province | `scps_sim.c:1189` → `prov[pid].alloc_on=0` | Province possédée ; retire l’override et revient à l’allocation automatique, immédiat | Aucun D | `MUTATED` seulement si l’override était actif |
| 26 AGE_ENGAGE | aucun argument ; pays du joueur et dernier âge levé | `scps_sim.c:1202` → `player_age_engaged=last_dawned` | Âge existant, pas déjà accusé réception, pays vivant ; aucun coût/délai. Les effets ont déjà été appliqués par `age_dawn` | Aucun D sur le chemin de l’ordre ; les tunables de l’avènement sont consommés lors de l’évènement, indépendamment de ce verbe | `MUTATED(age)` : accusé de réception pur, pas une promesse d’effet moteur supplémentaire |
| 27 COLONIZE | `a0=pid cible`, province vierge | `scps_sim.c:1279` → `econ_colonize_province` (`scps_econ.c:6163`) | Source = province colonisée du joueur la plus peuplée ; cible active/vierge, source `COLONY_MIN_POP`, vivres, chantier libre/cooldown ; ponction pop/wealth immédiate, arrivée différée | `COLONY_MIN_POP=300`, `COLONY_COST_POP=150`, `COLONY_FOOD_GATE=0.25`, `COLONY_WEALTH_SHARE=1.0` (`tune_list.h:1784-1786,726`); `COLONY_BASE_DAYS=360`, max 1080, CD 360, seed 100 et `COLONY_YIELD_HREF=4` sont codés (`econ.c:6155-6158`). Le ratio pop prélevé est clampé 0..1 avant multiplication par `COLONY_WEALTH_SHARE`; la clé elle-même n’a pas de borne générique | `STARTED(src,dst)` après ouverture du convoi ; refus si aucune source/gate/cible |
| 28 OFFER_MIGRATION | `a0=pays cible`, pays | `scps_sim.c:824` → `ai_consider_offer(OFFER_MIGRATION)`, `diplo_set_migration_pact` | Cible/consentement ; même émissaire, cooldown et coût d’influence que les autres offres | `INFLUENCE_COST_ENVOY=6.0`, `DIPLO_ENVOY_FLOOR_DAYS=30`; pacte migratoire et consentement sont règles diplomatiques | `MUTATED(1)` après passage faux→vrai; sinon `NO_CONSENT`/gate refus |
| 29 BUILD_MANUF | `a0=pid`, `a1=BuildingType`, province | `scps_sim.c:845` → `econ_build_manufacture` ; coût au drain `scps_sim.c:870-893` | Province à soi et colonisée, type civil/recipe valide, slot libre, pop `250*(n_bld+1)`, tier, intrant, or ; effet et niveau immédiats, pas de chantier différé | `MANUF_BUILD_COST=50` (`tune_list.h:970`) actif × IPM × multiplicateurs décret/doctrine. `250` staff, liste de sorties interdites (armes/arcane), recipes et niveau sont codés. Aucun coût de matériau séparé : le prix est le salaire/or | `COMPLETED(pid,bld,cost)` seulement après pose effective et débit ; toute gate donne refus |

## Constat de couverture

Les 29 verbes ont chacun un `case` de drain et un actuateur ou une mutation moteur identifiable. Les verbes différés sont explicitement `STARTED` (BUILD, REPRESS, ASSIMILATE, PURGE, CAMPAIGN, NAVY_BUILD, COLONIZE) ; les verbes immédiats emploient `MUTATED` ou `COMPLETED` seulement après preuve de mutation/quantité. Les refus de validation ne sont pas rapportés comme succès. Les offres diplomatiques sont des propositions : le consentement IA peut produire un refus réel après une mise en file valide.

Pour les offres 7–9 et 28, le coût d’influence et le plancher d’émissaire sont appliqués dans le pré-switch avant l’évaluation du consentement ; une offre refusée peut donc consommer l’influence, tandis qu’un ordre invalide ou bloqué par cooldown ne la consomme pas.

Les entrées de navigation (`ROUTE`, `CAMPAIGN`, et le repli maritime du mouvement de campagne) n’ont pas de tunable de « gratuité » : la gratuité est la règle économique du déplacement, tandis que `SEA_TRAVEL` ne fait qu’activer le chemin joueur maritime et `CHOKE_*` ne fait que calculer des chokes. Les dépenses de recherche, d’entretien, d’armes et de commerce apparaissent dans les helpers/ticks aval ; elles ne doivent pas être attribuées à tort au setter qui choisit une cible ou une posture.

Preuve de vérification : recherches `rg` ciblées et lecture des branches `scps_sim.c:595-1310`, `scps_agency.c`, `scps_intertrade.c`, `scps_campaign.c`, `scps_navy.c`, `scps_econ.c` et des entrées correspondantes de `scps_tune_list.h`. Ce livrable n’est pas une preuve comportementale exhaustive par scénario ; il documente la chaîne de code et les conditions observables du moteur au snapshot audité.

---

# Annexe C — Moteur, verbes 30 à 59

# Audit moteur des verbes joueur CMD30..CMD59

Date : 2026-09-05  
Périmètre : lecture seule de `scps/scps_sim.c` et des actionneurs appelés par le drain. Aucun réglage ni code de production n’a été modifié pour cet audit.

## Méthode et conventions

Les numéros sont ceux de `enum PlayerVerb` dans `scps/scps_sim.h`. Le drain est le point de vérité : l’API ne fait qu’enfiler un `PlayerCmd`, et le verdict arrive au tick (`scps_sim.c:1403-1408`). Sauf mention contraire, un ordre sans mutation explicite reçoit `SCPS_CMD_REASON_NO_EFFECT` par le refus de fin de boucle (`scps/scps_sim.c:1403-1406`). Les refus explicites sont indiqués quand le code les produit.

`tune_f(name, default)` lit le registre X-macro ou `SCPS_TUNE`; le registre ne fournit pas de min/max générique (`scps/scps_tune.c:16-80`). Les bornes ci-dessous sont donc celles des setters/gates du chemin, ou « aucune borne de registre » si le code ne clamp pas la surcharge. Les nombres codés sont signalés séparément : ils ne sont pas équilibrables par `SCPS_TUNE`.

## Verbes

### CMD30 — `CMD_EVENT_CHOICE`

- **Paramètres / grain.** `a[0]=slot` dans `EventsState.pending[]`, `a[1]=option`; choix discret au grain événement, sujet pays ou région (`scps_sim.c:1296-1306`). Le joueur choisit uniquement le slot et l’option.
- **Actionneur / gate / effet.** `pending_event_resolve(...)` est l’actionneur (`scps_sim.c:1307-1309`). Le slot doit être occupé, l’option dans `[0,n_options)`, et le sujet doit appartenir encore au joueur. L’effet, le coût, le délai et les changements d’état sont propres à l’`EventDef` et à son résolveur; il n’existe pas de coût/tunable unique attaché à CMD30.
- **Réglages / nombres.** Aucun `tune_f` direct dans le drain; aucun réglage déclaré n’est consommé uniformément par ce verbe. Les bornes d’option sont des données d’événement, pas des réglages.
- **Retour.** `COMPLETED(slot, option)` si le résolveur réussit; sinon refus final `NO_EFFECT`.

### CMD31 — `CMD_DECREE`

- **Paramètres / grain.** `a[0]=DecreeId`, `a[1]!=0` active ou désactive, au grain pays (`scps_sim.c:1318-1329`). C’est un choix de politique persistant.
- **Actionneur / gates / effet.** `decree_legal` puis `decree_toggle` pour les orientations; `decree_fire_decision` pour `DECISION_AUDIT_OFFICES` (`scps_sim.c:1321-1329`, `scps/scps_decrees.c:125-151`). Les orientations ont deux exclusions codées (Rations/Foyers et Circulation/Frontières), une réforme active ne peut pas être désactivée (`scps/scps_decrees.c:111-123`). La décision Audit exige corruption et cooldown, paie ponctuellement puis modifie corruption/L; les orientations sont financées chaque mois tout-ou-rien (`scps/scps_decrees.c:200-231`, `270-299`).
- **Réglages actifs.** Audit : `DECISION_AUDIT_CORRUPTION_MIN=20`, `DECISION_AUDIT_REVENUE_RATE=.25`, `DECISION_AUDIT_COOLDOWN_YEARS=5`, `DECISION_AUDIT_L_DELTA=.3` (`scps/scps_tune_list.h:1658-1662`); cooldown sérialisé borné au load à `[-31,40*365]` (`scps/scps_decrees.c:166-170`). Pour les orientations, les multiplicateurs et coûts `DECREE_*` sont lus dans les sites aval (`scps/scps_decrees.c:151-194` et `scps/scps_tune_list.h:1626-1655`), sans borne de registre; les états sont bornés par le masque et les clamps des lecteurs. La réforme Levée exige `TECH_CONSCRIPTION` (`scps/scps_decrees.c:15-23`) et utilise `DECREE_LEVEE_MIN_LEVEL=2` (`scps/scps_decrees.c:228-232`, registre `scps/scps_tune_list.h:1650`).
- **Nombres codés / retour.** Les paires exclusives sont codées. Succès `MUTATED(id,on)` pour une orientation, `COMPLETED(id,1)` pour Audit; sinon `NO_EFFECT`.

### CMD32 — `CMD_MANUMIT`

- **Paramètres / grain.** Aucun paramètre; affranchissement global du pays joueur (`scps_sim.c:1337-1343`). Le choix joueur est l’action elle-même.
- **Actionneur / effet.** `demography_manumit_country(econ,p)` transforme les groupes serviles du pays; si au moins une âme est libérée, `faction_lever_apply` déplace la faction communautaire (`scps_sim.c:1337-1342`). Aucun délai ni coût monétaire.
- **Réglage actif.** `DECISION_MANUMIT_COMMUNAUTAIRE_BIAS=.10`, sans borne de registre, lu uniquement si `freed>0` (`scps/scps_tune_list.h:1658`). La fraction de population est une règle du démographe, pas un tunable de CMD32.
- **Retour.** `COMPLETED(freed)` si `freed>0`; sinon refus `NO_EFFECT`.

### CMD33 — `CMD_SLAVE_BUY`

- **Paramètres / grain.** `a[0]=PID province de dépôt`, `a[1]=nombre demandé`, achat au grain province avec marché/prix au grain région (`scps_sim.c:1381-1388`). Le joueur choisit cible et quantité.
- **Actionneur / gates / effet.** La province doit être valide, colonisée et au joueur; le pays doit passer `econ_country_can_enslave` (pacifiste refusé), puis `intertrade_slave_buy(...,pid)` (`scps_sim.c:1382-1387`, `scps/scps_econ.c:853-867`, `scps/scps_intertrade.c:889-940`). Le prix est `SLAVE_PRICE * IPM * 2 * slave_pool_price_mult`; la quantité est limitée par le pool, le trésor et les groupes disponibles. L’achat crée/renforce un groupe `CLASS_SLAVE` sur le PID. Pas de délai.
- **Réglages actifs.** `SLAVE_PRICE=40` (`scps/scps_tune_list.h:537`, borne effective : prix protégé par `fmaxf(price,1e-4)`); `SLAVE_MARKET_CONSERVED=1` (`scps/scps_tune_list.h:543`, interrupteur `>0`); `SLAVE_POOL_REF=600` (`scps/scps_tune_list.h:1497`, référence de courbe, pas de clamp de registre); `SLAVE_FRACTION=.05` et `SLAVE_FRACTION_TECH=.15` ne sont pas le prix d’achat mais la fraction utilisée par conquête/pillage, donc réglages aval pertinents seulement (`scps/scps_econ.c:865-867`).
- **Nombres codés / retour.** Le multiplicateur mondial `*2`, la sélection de l’héritage dominant et `count<=0` sont codés (`scps/scps_intertrade.c:889-922`). `COMPLETED(got)` si une quantité est réellement achetée; sinon `NO_EFFECT`.

### CMD34 — `CMD_SLAVE_SELL`

- **Paramètres / grain.** `a[0]=PID province appartenant au joueur`, `a[1]=nombre demandé`; l’action est saisie sur une province mais le vendeur scanne toutes les provinces du pays (`scps_sim.c:1373-1379`, `scps/scps_intertrade.c:812-829`).
- **Actionneur / gates / effet.** `intertrade_slave_sell(econ,region,n)` exige une quantité positive et des groupes serviles existants; il retire les âmes, rescale leurs sièges, verse le pool par héritage puis paie le vendeur via Centre/wealth (`scps/scps_intertrade.c:812-887`). Pas de gate éthos/tech et pas de délai.
- **Réglages actifs.** `SLAVE_PRICE=40`, `SLAVE_MARKET_CONSERVED=1`, `SLAVE_POOL_REF=600` (`scps/scps_intertrade.c:800-816`, `scps/scps_tune_list.h:537,543,1497`). Prix et pool n’ont pas de borne de registre; le code borne naturellement la vente au stock détenu. `SLAVE_AI_KEEP_FRAC/.SELL_FRAC` sont des réglages IA, non consommés par ce verbe joueur (`scps/scps_tune_list.h:1499-1506`).
- **Nombres codés / retour.** La vente prend d’abord les plus gros groupes; les parts de secours du paiement sont `.42/.20/.38` (`scps/scps_intertrade.c:844-863`), non réglables. `COMPLETED(sold)` si vendu, sinon `NO_EFFECT`.

### CMD35 — `CMD_POP_TRANSFER`

- **Paramètres / grain.** `a={PID source, PID destination, SocialClass, count}`; transfert province→province, classe et nombre choisis par le joueur (`scps_sim.c:1394-1400`). Source/destination distinctes, toutes deux au joueur, classe valide, `count>0`.
- **Actionneur / effet.** `demography_pop_transfer` (`scps/scps_sim.c:1399`) déplace jusqu’au disponible; aucun coût, gate économique ou délai dans ce chemin. Les règles de groupe/classe sont internes au démographe.
- **Réglages / nombres.** Aucun `tune_f` consommé par CMD35; aucune déclaration de registre à rattacher. Le test `pa==pb` et les bornes d’index sont codés.
- **Retour.** `COMPLETED(moved,klass)` si mouvement réel; sinon `NO_EFFECT`.

### CMD36 — `CMD_FABRICATE_CB`

- **Paramètres / grain.** `a[0]=pays cible`; le joueur choisit la cible. Le drain applique d’abord le plancher d’émissaire et le coût d’influence pour le joueur (`scps_sim.c:620-655`), puis lance l’intrigue (`scps_sim.c:741-750`).
- **Actionneur / gates / effet.** `diplo_can_fabricate` exige deux pays vivants, capitales/régions valides, aucune intrigue sur la paire et l’or disponible; `diplo_fabricate_cb` débite le trésor national, crédite les élites de la cible et pose `FAB_MATURING` (`scps/scps_diplo.c:695-733`). L’intrigue mûrit au tick puis devient `FAB_READY`; elle expire ensuite.
- **Réglages actifs.** `INFLUENCE_COST_FAB=12` et `DIPLO_ENVOY_FLOOR_DAYS=30` (`scps/scps_sim.c:641-655`, registre `scps/scps_tune_list.h:2098-2100`); `FAB_CB_COST_YEARS=2` (`scps/scps_diplo.c:695`, registre `scps/scps_tune_list.h:1516`); `FAB_MATURE_DAYS=365`, `FAB_VALID_DAYS=1825` (`scps/scps_diplo.c:750,785`, registre `scps/scps_tune_list.h:1517-1518`). Aucun min/max de registre; l’or disponible, état `FAB_NONE`, et jours `<=0` sont les bornes effectives. La doctrine Diplomatie peut modifier l’influence via sa clé mais pas le coût d’or.
- **Nombres codés / retour.** Repli du CB vers `CB_TERRITORIAL` (`scps/scps_sim.c:745-746`). `STARTED(state,cb)` au succès; sinon `NO_EFFECT` ou refus global d’influence/cooldown.

### CMD37 — `CMD_COUNCIL_PAY`

- **Paramètres / grain.** `a[0]=siège 0..SC_COUNCIL_SEATS-1`, `a[1]=paie×100`; choix de paie d’un ministre, grain pays/siège (`scps_sim.c:968-976`).
- **Actionneur / gates / effet.** Siège pourvu obligatoire; `statecraft_council_set_pay` clamp le ratio à `[0.1,2.0]` et le tick fait converger la loyauté (`scps/scps_statecraft.c:358-363`, `395-438`). Pas de coût immédiat ni délai; effet différé sur loyauté, efficacité et risque de trahison.
- **Réglages actifs.** `COUNCIL_PAY_ADJ=30` et `COUNCIL_CLASS_SAT_W=40` modulent respectivement la cible de loyauté et la pénalité de classe; `COUNCIL_LOYAL_RATE=.05`, `COUNCIL_ROT_BOOST=1.5`, `COUNCIL_BETRAYAL_THRESHOLD=15` pilotent l’aval (`scps/scps_statecraft.c:405-438`, registre `scps/scps_tune_list.h:852-855,1528-1529`). Bornes de paie `[.1,2]` codées dans le setter; les tunables n’ont pas de borne de registre.
- **Retour.** `MUTATED(paiex100,0,pay)` si changement; siège vacant ou valeur déjà identique ⇒ `NO_EFFECT`.

### CMD38 — `CMD_RAID_COAST`

- **Paramètres / grain.** `a[0]=PID province côtière ennemie`; raid au grain province, pillage au grain région, joueur choisit la cible (`scps_sim.c:1128-1149`).
- **Actionneur / gates / effet.** Province colonisée et côtière, autre pays, ni allié ni pacte; région sans `raid_cd_days`; au moins une coque pirate. `diplo_pillage_value` transfère une valeur réelle puis `navy_mark_raided` pose la balafre/CD; la capture servile utilise la fraction du pays attaquant (`scps/scps_sim.c:1142-1158`, `scps/scps_navy.c:397-403`). Pas de délai de file; le cooldown empêche le raid suivant.
- **Réglages actifs.** `PILLAGE_INCOME_FRAC=.20` est la valeur cible du revenu annuel victime (`scps/scps_diplo.c:1458`, registre `scps/scps_tune_list.h:368`); `SLAVE_FRACTION=.05` ou `SLAVE_FRACTION_TECH=.15` est choisi par l’état technique/éthos (`scps/scps_econ.c:865-867`). `NAVY_COMBAT_ON=0` ne gate pas ce raid direct : il concerne les combats/réservations navales dans les chemins campagne; aucun tune de cooldown n’existe.
- **Nombres codés / retour.** `PILLAGE_COOLDOWN_Y=5` ans est un `#define` hors registre (`scps/scps_diplo.c:1324-1329`); `navy_mark_raided` pose `COURSE_IMMUNITE_J` codé (`scps/scps_navy.c:397-403`). Une fois les gates franchies, le drain marque la province et renvoie toujours `COMPLETED(prov,region,loot)`, y compris si `loot==0`; seuls les refus de gate donnent `NO_EFFECT`.

### CMD39 — `CMD_MOVE_ARMY`

- **Paramètres / grain.** `a[0]=région cible`; déplacement de l’armée de réserve ou redirection du corps pays (`scps/scps_sim.c:1030-1062`). Le joueur choisit seulement la cible.
- **Actionneur / gates / effet.** Corps actif : `campaign_redirect`, puis repli maritime si activé; réserve : levée depuis la capitale via `campaign_order` ou `campaign_order_sea`. Terre exige un chemin; mer exige ports/côtes, ancre maritime, supplies et transports. L’ordre démarre `FA_MARCH`/`FA_EMBARK`; le mouvement et l’attrition se résolvent dans `campaign_tick`.
- **Réglages actifs.** `SEA_TRAVEL=1` est le kill-switch du repli maritime (`scps/scps_sim.c:1035-1040,1053`, registre `scps/scps_tune_list.h:1849`); `EMBARK_NAVAL_COST=10` supplies par stack et `NAVY_COMBAT_ON=0` réservation de transports (`scps/scps_campaign.c:503-520`, registre `scps/scps_tune_list.h:853-854`). Les bornes de supplies sont `>=needsup`, transports `>=ceil(packets/10)` si combat actif; aucune borne de registre.
- **Nombres codés / délai / retour.** Chargement `4 + packets/15` jours, débarquement `3 + units/20` et facteur hors-port `1.6` (`scps/scps_campaign.c:529-530,582-583`); `1 transport=10 paquets`. Point de sémantique à surveiller : `campaign_order_sea` débite `EMBARK_NAVAL_COST` avant de vérifier côte cible/ancre/bassin (`scps/scps_campaign.c:503-516`), donc un échec géographique postérieur peut consommer des supplies. `STARTED(target)` si l’ordre est créé, sinon `NO_EFFECT`.

### CMD40 — `CMD_CORPS_RAISE`

- **Paramètres / grain.** `a[0]=packets`, `a[1]=région cible`; levée depuis la capitale dans un nouveau corps (`scps/scps_sim.c:1065-1072`). Quantité et cible sont choix joueur.
- **Actionneur / gates / effet.** `campaign_raise` exige régions valides, `packets>0`, au plus l’effectif de réserve, un slot de corps libre et un prochain saut terrestre (`scps/scps_campaign.c:311-333`). Le corps est créé, réserve débitée, marche démarrée; délai = `army_step_days` au terrain suivant.
- **Réglages / nombres.** Aucun tunable direct. Bornes codées : `packets<=force_units(src_force)`, `CAMPAIGN_MAX_CORPS`, `next_hop>=0`; le corps reçoit `nominal=force_units` (`scps/scps_campaign.c:316-327`).
- **Retour.** `STARTED(packets,target)` seulement si `campaign_raise>0`, sinon `NO_EFFECT`.

### CMD41 — `CMD_CORPS_SPLIT`

- **Paramètres / grain.** `a[0]=id corps`, `a[1]=packets`; séparation d’un corps actif joueur (`scps/scps_sim.c:1074-1077`).
- **Actionneur / gates / effet.** `campaign_split` refuse bataille/mer, `packets<=0` ou `>=effectif`, et exige un slot libre; il détache la force et partage le nominal au prorata (`scps/scps_campaign.c:336-358`). Pas de coût ni délai.
- **Réglages / nombres.** Aucun `tune_f`; limites codées par `CAMPAIGN_MAX_CORPS` et les états `FA_BATTLE/FA_EMBARK`.
- **Retour.** `COMPLETED(id,packets)` si nouveau corps, sinon `NO_EFFECT`.

### CMD42 — `CMD_CORPS_MERGE`

- **Paramètres / grain.** `a[0]=dst_id`, `a[1]=src_id`; fusion de deux corps au même lieu (`scps/scps_sim.c:1085-1089`).
- **Actionneur / gates / effet.** `campaign_merge` exige corps distincts, actifs, même propriétaire et emplacement, hors bataille/mer; additionne forces et nominal puis désactive la source (`scps/scps_campaign.c:394-407`). Aucun coût ni délai.
- **Réglages / nombres.** Aucun tune; états de phase et égalité de lieu sont codés.
- **Retour.** `COMPLETED(dst,src)` si fusion, sinon `NO_EFFECT`.

### CMD43 — `CMD_CORPS_MOVE`

- **Paramètres / grain.** `a[0]=id`, `a[1]=région cible`; redirection d’un corps joueur (`scps/scps_sim.c:1091-1098`).
- **Actionneur / gates / effet.** `campaign_redirect_corps` revalide corps actif, non brisé, non bataille/mer, force positive et chemin terrestre; à défaut le chemin maritime est tenté si `SEA_TRAVEL>0`, avec les mêmes ports/côtes/supplies/transports que CMD39 (`scps/scps_campaign.c:409-433,552-584`). Le corps passe en marche ou embarquement; délai `army_step_days`/embarquement.
- **Réglages / nombres.** `SEA_TRAVEL=1`, `EMBARK_NAVAL_COST=10`, `NAVY_COMBAT_ON=0`; aucune borne de registre. Les facteurs de chargement codés sont ceux de `scps/scps_campaign.c:582-583`.
- **Retour.** `STARTED(id,target)` si redirection, sinon `NO_EFFECT`.

### CMD44 — `CMD_CORPS_REFILL`

- **Paramètres / grain.** `a[0]=id`; renfort du corps au grain corps/unité (`scps/scps_sim.c:1100-1104`).
- **Actionneur / gates / effet.** `campaign_can_refill_corps` exige corps actif sur une région appartenant au propriétaire; `campaign_refill_corps` ne fait rien au nominal, puis par ligne d’unité vérifie population de classe et armes macro (`scps/scps_campaign.c:1341-1389`). Chaque paquet ajouté lève `POP_PER_UNIT` hommes; l’aperçu `campaign_refill_corps_cost` annonce `2` matériaux par ligne, tandis que l’exécution prend `POP_PER_UNIT` de la ressource d’arme au stock national (`scps/scps_campaign.c:1352-1381`).
- **Réglages / nombres.** Aucun tune direct. `POP_PER_UNIT`, l’aperçu `2` matériaux/ligne, une vague par ligne et le nominal ratchet sont codés; armes `RES_NONE` n’ôtent aucun stock macro.
- **Retour.** `COMPLETED(id)` si au moins un paquet; sinon `NO_EFFECT`.

### CMD45 — `CMD_CORPS_DISBAND`

- **Paramètres / grain.** `a[0]=id`; dissolution d’un corps actif du joueur (`scps/scps_sim.c:1106-1110`).
- **Actionneur / gates / effet.** `campaign_disband_corps` restitue les survivants à l’armée hôte et désactive le corps (`scps/scps_campaign.c:1307-1319`). Aucun coût ni délai; la destination est le host du joueur.
- **Réglages / nombres.** Aucun `tune_f`; seules les bornes d’id/état actif et `packets>0` codées.
- **Retour.** `COMPLETED(id)` si des unités sont restituées, sinon `NO_EFFECT`.

### CMD46 — `CMD_BUDGET_POLICY`

- **Paramètres / grain.** `a[0]=family` (0 fiscalité par classe, 1 enveloppe), `a[1]=index`, `a[2]=multiplicateur×100`; choix pays/classe ou pays/politique (`scps/scps_sim.c:977-990`).
- **Actionneur / gates / effet.** `econ_country_tax_set` ou `econ_country_budget_set`; les setters portent les bornes du multiplicateur et l’effet est lu par l’économie au tick (`scps/scps_sim.c:979-988`). Aucun coût, gate ou délai.
- **Réglages / nombres.** Aucun tunable de registre : ce sont des réglages joueur sérialisés, distincts de `SCPS_TUNE`. Les familles/indices sont bornés par `CLASS_COUNT` et `BUDGET_POLICY_COUNT`; la conversion ×100 est codée. Les limites exactes du setter sont donc la source de vérité, pas le registre.
- **Retour.** `MUTATED(index, family, mult)` si la valeur change, sinon `NO_EFFECT`.

### CMD47 — `CMD_PEACE_OFFER`

- **Paramètres / grain.** `a={target, flags, gold_score, n_regions, regions[]}`; négociation au grain pays, transfert au grain région (`scps/scps_sim.c:762-797`). Le joueur choisit territoires, drapeaux et score-or.
- **Actionneur / gates / effet.** Cible en guerre, `n_regions` et score valides, flags cohérents, régions ennemies occupées par le joueur et distinctes; coût comparé au `diplo_war_score`. Le vis-à-vis doit consentir si l’offre est vide. Succès transfère régions, prend l’or, puis applique réparations, humiliation, pillage, libération, vassalisation ou fragmentation (`scps/scps_sim.c:768-805`).
- **Réglages actifs.** Le drain joueur consomme `INFLUENCE_COST_ENVOY=6` × échelle × doctrine Diplomatie, puis `DIPLO_ENVOY_FLOOR_DAYS=30` (`scps/scps_sim.c:641-655`, registre `scps/scps_tune_list.h:2098-2100`). La fraction d’esclaves de transfert lit `SLAVE_FRACTION/.TECH`. Aucun autre tune ne tarife directement l’offre.
- **Nombres codés / bornes.** `SCPS_PEACE_MAX_TERRITORIES=32`, `gold_score<=25`; coûts de guerre `+10/+20/+10/+50/+100` par drapeau, or `score*0.03*revenu mensuel`, pillage stock `.05` (`scps/scps_sim.c:765-782,789-795`). Ces chiffres ne sont pas réglables. `MUTATED(status,0,cost)` si paix effective; refus global `NO_EFFECT`, ou `NO_CONSENT` pour l’offre vide refusée.

### CMD48 — `CMD_MANUF_LEVEL`

- **Paramètres / grain.** `a={PID province, BuildingType, dir}`; `dir>=0` monte d’un niveau, sinon descend, au grain province (`scps/scps_sim.c:883-905`).
- **Actionneur / gates / effet.** Province colonisée appartenant au joueur, type valide. Monter vérifie crédit puis `econ_manuf_level_delta(+1)`; descendre appelle la même primitive avec `-1` gratuitement. Le niveau et la capacité économique changent immédiatement.
- **Réglages actifs.** Montée : `MANUF_BUILD_COST=50` × IPM × `decree_manuf_cost_mult` × doctrine `MANUF_BUILD_COST` (`scps/scps_sim.c:890-899`, registre `scps/scps_tune_list.h:970`); l’orientation Ateliers lit `DECREE_ATELIERS_MANUF_COST_MULT=.95` (`scps/scps_decrees.c:157-159`, registre `scps/scps_tune_list.h:1639`). Aucun min/max de registre; la borne de niveau est dans `econ_manuf_level_delta`.
- **Nombres / retour.** `dir` est réduit à ±1 par le drain; montée payante, descente libre. `COMPLETED(1,0,cost)` en montée ou `MUTATED(-1)` en descente; sinon `NO_EFFECT`.

### CMD49 — `CMD_DEMOLISH_EDI`

- **Paramètres / grain.** `a={PID province, Edifice}`; démolition d’un édifice provincial du joueur (`scps/scps_sim.c:906-916`).
- **Actionneur / gates / effet.** Province et édifice valides, province au joueur; `agency_demolish_edifice` retire un palier/bit selon la famille (`scps/scps_sim.c:908-915`). Aucun coût ni délai.
- **Réglages / nombres.** Aucun `tune_f` dans l’actionneur ou le drain; aucune déclaration de registre directement consommée. Les familles et bits (`EDIFICE_COUNT`, masque 32 bits) sont codés.
- **Retour.** `MUTATED(bit)` si le masque change, sinon `NO_EFFECT`.

### CMD50 — `CMD_BANKRUPTCY`

- **Paramètres / grain.** Aucun; répudiation volontaire de la dette du pays joueur (`scps/scps_sim.c:1349-1358`).
- **Actionneur / gates / effet.** Dette strictement positive requise; `credit_bankruptcy(...,false)` efface les trois dettes, pose la cicatrice sur toutes les provinces actives et mémorise le créancier. Le drain ajoute la rancune au créancier cité-état (`scps/scps_credit.c:942-986`, `scps/scps_sim.c:1350-1357`). Pas de délai d’action; la cicatrice décroît au fil des années.
- **Réglages actifs.** `BANKRUPTCY_RANCOR=2` au drain; `BANKRUPTCY_SCAR_YEARS=10` pour la décroissance aval; `LENDER_RUIN_SHARE=.5` et `SINK_FLOOR=500` pour la ruine éventuelle du prêteur (`scps/scps_credit.c:966-984`, registre `scps/scps_tune_list.h:169,1022,1080-1081`). Les bornes de dette sont l’existence et `>0`; les réglages n’ont pas de min/max générique.
- **Nombres codés / retour.** Le wipe est total et `forced=false` distingue la télémétrie. `COMPLETED(L,0,debt_before-debt_after)` si dette réduite; sans dette, refus explicite `NO_EFFECT`.

### CMD51 — `CMD_REPAY`

- **Paramètres / grain.** `a[0]=montant`; montant `<=0` signifie tout le surplus disponible, grain pays (`scps/scps_sim.c:1362-1365`).
- **Actionneur / gates / effet.** `credit_repay_principal` exige dette et surplus positifs; rembourse le minimum dette/surplus, ou le montant demandé, débite le surplus et ventile la somme aux créanciers (`scps/scps_credit.c:999-1026`). Pas de délai.
- **Réglage actif.** `COURT_FLOOR=4000` est le plancher de surplus (`scps/scps_credit.c:1003-1007`, registre `scps/scps_tune_list.h:159`); aucune borne de registre. `PRINCIPAL_REPAY_RATE=.10` est utilisé par l’amortissement annuel, pas par ce verbe manuel (`scps/scps_credit.c:783-787`, registre `scps/scps_tune_list.h:1025`).
- **Nombres / retour.** La ventilation `share_e` et le seuil `CR_EPS` sont codés; `COMPLETED(paid)` si paiement, sinon `NO_EFFECT`.

### CMD52 — `CMD_BORROW_CLASS`

- **Paramètres / grain.** `a[0]=SocialClass`, `a[1]=montant`; montant `<=0` demande le maximum disponible, grain pays/classe (`scps/scps_sim.c:991-997`).
- **Actionneur / gates / effet.** Classe bornée par `CLASS_COUNT`; `credit_borrow_class` prête ce que la classe peut financer, sans refus de consentement mais avec capacité/surplus/plafond d’exposition (`scps/scps_credit.c:430-522`). Dette et trésor augmentent immédiatement.
- **Réglages actifs.** `SINK_FLOOR=500`, `CLASS_LEND_SHARE=.05`, `CLASS_EXPOSURE_SHARE=.50`, `ELITE_LEND_WEIGHT=1`, `BOURGEOIS_LEND_WEIGHT=.5`, `DEBT_FIXED=1`, et la courbe `DEBT_RATE_BASE=.02`, `DEBT_RATE_LINEAR=.015`, `DEBT_RATE_QUAD=.0075`, `DEBT_RATE_MIN=.02`, `DEBT_RATE_MAX=.50` (`scps/scps_credit.c:121-157,430-509`, registre `scps/scps_tune_list.h:169,997-1000,1033-1055`). Aucun min/max de registre; les plafonds calculés sont les bornes effectives.
- **Nombres / retour.** `<=0` maximum, pondération des classes et choix d’unité de dette sont codés. `COMPLETED(cls,0,got)` si prêt réel, sinon `NO_EFFECT`.

### CMD53 — `CMD_REQUEST_LOAN`

- **Paramètres / grain.** `a[0]=État prêteur`, `a[1]=montant` (`<=0` = maximum structurel); offre au grain paire de pays (`scps/scps_sim.c:834-843`).
- **Actionneur / gates / effet.** Cible valide et non soi; `ai_consider_offer(...OFFER_LOAN)` décide du consentement puis `credit_borrow_state` calcule/exécute le prêt (`scps/scps_sim.c:835-842`). L’offre passe aussi par le plancher d’émissaire `DIPLO_ENVOY_FLOOR_DAYS=30` mais ne paie pas `INFLUENCE_COST_ENVOY` dans le branchement actuel (`scps/scps_sim.c:620-655`). Dette, trésor et note transitoire de feedback changent immédiatement.
- **Réglages actifs.** `LENDER_PORTFOLIO_SHARE=.75`, `LENDER_DEBTOR_SHARE=.35`, `LENDER_MEMORY=1`, `SINK_FLOOR=500`, `DEBT_FIXED=1` et la même courbe `DEBT_RATE_*` (`scps/scps_credit.c:294-380,620-626`, registre `scps/scps_tune_list.h:1004-1013,1033,1055`). `CITYSTATE_LEND_SHARE=.5` participe au prêteur cité-état (`scps/scps_credit.c:320`, registre `scps/scps_tune_list.h:1003`). Aucun min/max de registre.
- **Nombres / retour.** Les parts d’exposition et `amount<=0` sont codés par les helpers; l’acceptation est subjective et différée dans le même drain, sans délai de maturation. `COMPLETED(got)` si accepté, sinon refus explicite `NO_CONSENT`.

### CMD54 — `CMD_RENOVER`

- **Paramètres / grain.** `a[0]=PID province`; rénovation du bâti provincial (`scps/scps_sim.c:669-676`).
- **Actionneur / gates / effet.** Province au joueur, avec région valide; `agency_renover_acct` exige au moins un édifice, usure `<=.995`, aucune rénovation déjà en file et crédit disponible (`scps/scps_agency.c:794-816`). Le coût est débité avant l’appel à `enqueue`; si la file est pleine, `enqueue` peut échouer après le débit (aucun rollback local), point à traiter côté UI/capacité de file. Quand l’ordre est accepté, il est payé aux classes puis une commande de `180` jours remet au moins les valeurs nominales au terme (`scps/scps_agency.c:845-856`).
- **Réglages actifs.** `RENOV_COST_FRAC=.50` calcule le coût relatif aux édifices; `RENOV_SHARE_LAB=.50` partage le paiement entre laboureurs et bourgeois (`scps/scps_agency.c:785-816`, registre `scps/scps_tune_list.h:1174-1176`). Aucun min/max de registre; les deux valeurs sont clampées à `[0,1]` pour le partage et l’usure est bornée par le test.
- **Nombres / retour.** `RENOV_DAYS=180` est un `#define` hors registre (`scps/scps_agency.c:793`); seuil d’usure `.995` et absence de doublon sont codés. `STARTED` si l’ordre est en file, sinon `NO_EFFECT`.

### CMD55 — `CMD_SPLIT_COMP`

- **Paramètres / grain.** `a[0]=id`, `a[1..4]={infanterie, archers, cavalerie, mages}` en paquets; composition choisie exactement par le joueur (`scps/scps_sim.c:1079-1083`).
- **Actionneur / gates / effet.** `campaign_split_comp` exige corps actif joueur, hors bataille/mer, chaque paquet `>=0`, total strictement entre 0 et effectif, composition disponible et slot libre (`scps/scps_campaign.c:360-391`). La force et le nominal sont détachés au prorata; aucun coût ni délai.
- **Réglages / nombres.** Aucun tune; `total_req<total`, `CAMPAIGN_MAX_CORPS`, catégories et quatre paquets sont codés.
- **Retour.** `COMPLETED(id)` si séparation, sinon `NO_EFFECT`.

### CMD56 — `CMD_SEAL_DESSEIN`

- **Paramètres / grain.** `a={branch, rung, voie}`; scellement d’un échelon de mission au grain pays/branche; la voie n’est un choix effectif qu’au pivot (`scps/scps_sim.c:1213-1224`).
- **Actionneur / gates / effet.** `missions_seal` exige branche générée, échelon courant prêt, pays vivant; le pivot exige voie Conquête/Vassalisation, preuve correspondante et paiement d’influence (`scps/scps_missions.c:640-676`). Le sceau avance le rung, pose récompense/revendication, puis peut déclencher l’âge des héros si l’échelon final et le Conseil satisfont les seuils (`scps/scps_sim.c:1222-1241`).
- **Réglages actifs.** `DESSEIN_SOL_HEGEMON_FRAC=.40` conditionne l’hégémonie, `DESSEIN_BOON_YEARS=20` la durée des remises, `DESSEIN_PIVOT_INFLUENCE=10` le coût de pivot (`scps/scps_missions.c:79,98,463`, registre `scps/scps_tune_list.h:1608-1618`); `AGE_HERO_EFFICIENCY_MIN=1.0` et `AGE_HERO_LOYALTY_MIN=75` gardent la gate finale (`scps/scps_sim.c:1233-1241`, registre `scps/scps_tune_list.h:1722-1723`). Le coût de pivot est multiplié par l’échelle d’influence; aucun min/max de registre.
- **Nombres codés / délai / retour.** Les récompenses `K_inst +.6`, reconstruction `1.0`, `+2.0/+1.0` et annale `100/45` sont codées (`scps/scps_missions.c:684-734`, `scps/scps_sim.c:1226-1232`). `COMPLETED(branch,rung)` si sceau, sinon `NO_EFFECT`.

### CMD57 — `CMD_DOCT_ADOPT`

- **Paramètres / grain.** `a[0]=slot`, `a[1]=DoctrineId`; choix d’un courant dans un slot du pays (`scps/scps_sim.c:1262-1266`).
- **Actionneur / gates / effet.** `doctrines_adopt` exige slot ouvert/libre, doctrine non déjà présente, exclusivités respectées, foi fondée pour Divin, et influence suffisante; il paie, installe la doctrine et remet ses idées à zéro (`scps/scps_doctrines.c:345-402`). Pas de délai ni entretien.
- **Réglages actifs.** Coût `DOCT_COST_BASE=50 + DOCT_COST_STEP=25 × doctrines actives`, multiplié par échelle `ech` (`scps/scps_doctrines.c:302-311`, registre `scps/scps_tune_list.h:2112-2115`). `ech` est bornée défensivement à `[1,1000]` dans le helper (`scps/scps_doctrines.c:294-300`), avec plancher d’échelle côté appelant. Les doctrines aval lisent leurs clés d’effet, mais ces clés ne sont pas des coûts du verbe.
- **Nombres / retour.** `DOCT_SLOTS_MAX`, exclusivités Commerce/Mercantilisme et courant unique sont codés. `MUTATED(slot,doctrine)` si adoption; sinon `NO_EFFECT`.

### CMD58 — `CMD_DOCT_IDEA`

- **Paramètres / grain.** `a[0]=DoctrineId`; achat de la prochaine idée séquentielle du pays (`scps/scps_sim.c:1267-1271`).
- **Actionneur / gates / effet.** `doctrines_buy_idea` exige doctrine adoptée, moins de `DOCT_IDEAS`, influence suffisante; il débite et incrémente l’idée suivante (`scps/scps_doctrines.c:399-411`). Aucun délai/entretien.
- **Réglages actifs.** `IDEA_COST_BASE=30 + IDEA_COST_STEP=3 × idées déjà possédées`, multiplié par `ech`; même clamp `ech∈[1,1000]` (`scps/scps_doctrines.c:302-311`, registre `scps/scps_tune_list.h:2112-2115`).
- **Nombres / retour.** Séquence imposée et `DOCT_IDEAS` sont codés; `COMPLETED(doctrine)` si achat, sinon `NO_EFFECT`.

### CMD59 — `CMD_DOCT_ABANDON`

- **Paramètres / grain.** `a[0]=slot`; abandon d’une doctrine au grain slot/pays (`scps/scps_sim.c:1272-1275`).
- **Actionneur / gates / effet.** `doctrines_abandon` exige slot valide occupé, efface toutes les idées de la doctrine puis libère le slot (`scps/scps_doctrines.c:413-423`). Abandon libre, sans remboursement, coût ni délai.
- **Réglages / nombres.** Aucun `tune_f`; `DOCT_SLOTS_MAX` et la perte totale des idées sont codés. `MUTATED(slot)` si abandon, sinon `NO_EFFECT`.

## Réglages déclarés mais morts ou hors chemin CMD30..59

- Le registre ne pose aucun min/max universel : une surcharge `SCPS_TUNE` est acceptée telle quelle si le nom existe (`scps/scps_tune.c:33-80`). Les clamps observés sont donc locaux et doivent rester associés au site de lecture.
- `RENOV_AI_TRIG` est déclaré pour le déclenchement IA; il n’est pas lu par `CMD_RENOVER` (`scps/scps_tune_list.h:1174-1176`, `scps/scps_agency.c:794-816`).
- `NAVY_COLONY_MAX_DAYS`, `NAVY_COLONY_CD_DAYS`, `COLONY_MIN_POP`, `COLONY_FOOD_GATE`, `NAVY_TRANSPORT_MIN` concernent colonisation/course IA et ne pilotent pas directement CMD38–43; CMD39/43 ne lisent ici que `SEA_TRAVEL`, `EMBARK_NAVAL_COST`, `NAVY_COMBAT_ON` (`scps/scps_navy.c:27-30,287-300,491`, `scps/scps_campaign.c:503-584`).
- `PRINCIPAL_REPAY_RATE` et `BANKRUPTCY_GRACE_YEARS` sont actifs dans l’amortissement/défaut annuel, mais pas dans le remboursement manuel CMD51 ni la banqueroute volontaire CMD50; `BANKRUPTCY_SCAR_YEARS` reste actif dans la cicatrice après CMD50.
- `SLAVE_AI_KEEP_FRAC`, `SLAVE_AI_SELL_FRAC`, `SLAVE_AI_BUY_FRAC` ne sont pas des réglages des ventes/achats manuels CMD33–34; ils alimentent les producteurs IA du pool.
- `FAB_MATURE_DAYS` et `FAB_VALID_DAYS` sont actifs pour le cycle persistant lancé par CMD36; `FAB_VALID_DAYS` est aussi multiplié par le Dessein « Rival ». Aucun réglage ancien de cooldown d’émissaire ne subsiste : le plancher actif est `DIPLO_ENVOY_FLOOR_DAYS`.

## Limites de design observées

Les limites les plus sensibles pour le designer sont codées hors registre : coûts des drapeaux de paix et facteur d’or/pillage, cooldown de pillage, chargement/débarquement naval, `250` hommes de personnel par manufacture (hors verbes 48–49 mais voisin du même panneau), `POP_PER_UNIT` et le `2` de l’aperçu de coût du refill, nombre maximal de régions de paix `32`, et pertes d’idées à l’abandon. Deux risques de contrat ressortent de la lecture : le raid côtier signale `COMPLETED` même avec butin nul une fois ses gates franchies, et la rénovation débite avant de vérifier la capacité de file; le mouvement maritime de réserve peut aussi débiter les supplies avant un refus géographique. Les autres seuils présentés comme réglages sont réellement raccordés au chemin indiqué et disposent d’un défaut X-macro vérifiable.
