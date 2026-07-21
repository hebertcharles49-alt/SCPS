# Plan vivant — profondeur et fluidité de l’interface

**Objectif produit** : conserver l’interface actuelle, déjà lisible, et lui donner la
profondeur d’un grand jeu de stratégie : toute information visible doit mener à son
détail utile, toute alerte à sa cause, et toute cause à l’action ou au lieu concerné.

**Direction** : moteur personnel façon EU4, accès à l’information et interaction
contextuelle façon RimWorld. Il ne s’agit pas d’un redesign graphique.

**Statut global** : EN COURS — démarré le 2026-07-12.

## Légende

- `À FAIRE` : pas commencé.
- `EN COURS` : tranche active, non encore validée.
- `FAIT` : implémenté et vérifié.
- `DIFFÉRÉ` : volontairement hors de la tranche actuelle, avec raison documentée.

## Règles de conception verrouillées

1. Aucun nouvel écran si une surface actuelle peut porter l’information proprement.
2. Un clic ouvre le bon niveau de détail ; un survol explique l’état et sa tendance.
3. Les liens contextuels utilisent des références stables, jamais du texte analysé.
4. Les raisons d’indisponibilité viennent du moteur ou de la façade, pas d’une
   reconstitution approximative dans l’UI.
5. L’historique de navigation restaure une vue, il ne rejoue jamais une action moteur.
6. Les nouveaux contrats restent optionnels pendant la migration afin de ne pas casser
   les panneaux existants.
7. Chaque tranche doit démarrer en headless sans erreur GDScript avant de passer à `FAIT`.

## Références visuelles — économie (2026-07-12)

Les trois captures CK3 fournies fixent la hiérarchie d'information, pas leur décor :

- **Topbar** : une ligne compacte d'icônes ; valeur principale et variation directement
  visibles, sans ouvrir un panneau.
- **Hover économique** : stock + évolution mensuelle, total des revenus, détail des
  revenus, total des dépenses, détail des dépenses, puis accès au panneau concerné.
- **Vue Économie** : revenus et dépenses séparés, complets et comparables ; solde,
  crédit, marché et commerce restent des angles d'un même domaine.
- Le chrome graphite/parchemin SCPS et la disposition actuelle sont conservés. Aucun
  portrait, blason ou onglet décoratif n'est importé seulement parce qu'il existe dans
  la référence.

## Parcours cible

```text
état visible → survol chiffré → clic vers le détail → cause localisée
             ↘ concept si nécessaire          ↘ action légale / raison du blocage

alerte → objet concerné → carte ou panneau → décision

recherche globale → pays / province / corps / ressource / technologie → même navigation
```

## Phases

### P0 — Contrat d’information et plan vivant — FAIT

- Ajouter ce document et le tenir à jour au fil des tranches.
- Définir `InfoRef`, référence légère vers un pays, une province, une région, un corps,
  une ressource, une technologie, un onglet ou un mode de carte.
- Définir le format d’une requête de navigation, sans dépendance aux panneaux concrets.
- Poser les invariants de validation et de déduplication.

**Critère de fin** : les contrats existent, sont documentés et parsables par Godot.

### P1 — Routeur central et historique — FAIT

- Ajouter un `NavigationHub` possédé par `Main`.
- Centraliser dans `Main` les ouvertures de surfaces aujourd’hui dispersées.
- Ajouter historique arrière/avant, limité et sans doublons consécutifs.
- Raccourcis : `Alt+Gauche` / `Alt+Droite` ; ignorer les champs de saisie.
- Conserver la règle actuelle de zone contextuelle unique.

**Critère de fin** : navigation pays, province, région, onglet et technologie ; retour et
avance restaurent les vues sans modifier la simulation.

### P2 — Premiers liens visibles — FAIT

- Rendre les blocs pertinents de la topbar ouvrables : royaume, trésor, matériaux,
  armes, nourriture, savoir, factions, loyauté/prospérité.
- Router les alertes existantes par le même contrat sans supprimer leurs signaux pendant
  la migration.
- Faire accepter au tiroir un petit contexte de focus (`resource`, `country`, etc.).
- Montrer un affordance discret au survol des cellules ouvrables.

**Critère de fin** : depuis la topbar et une alerte, le joueur atteint en un clic la vue
qui expose le détail déjà disponible.

### P3 — Cartes d’information structurées — FAIT

- Étendre `TooltipServer` avec un payload structuré optionnel : titre, état, tendance,
  lignes de décomposition, références liées et actions de navigation.
- Garder la compatibilité avec tous les `tooltip_text` actuels.
- Permettre d’épingler une carte puis de suivre ses liens sans course au survol.
- Harmoniser unités et horizons : `/jour`, `/mois`, `/an`, stock, couverture.

**Critère de fin** : une cellule migrée expose état + tendance + causes + destination,
sans régression des tooltips à concepts.

### P4 — Actions bloquées et conséquences — FAIT

- Standardiser le résultat des prédicats de légalité : `allowed`, code stable, libellé,
  données chiffrées et références concernées.
- Afficher la première raison bloquante sur le bouton et la décomposition complète au
  survol.
- Ajouter coût actuel, coût manquant, délai ou prérequis quand la façade les expose.
- Migrer d’abord construction, armée, diplomatie et technologie.

**Critère de fin** : aucun bouton important n’est seulement grisé ; le joueur sait quoi
changer et où regarder.

### P5 — Recherche universelle — FAIT

- Ajouter une palette `Ctrl+K` réutilisant la logique de recherche locale du Codex.
- Indexer au minimum : actions, concepts, pays, provinces, régions, corps, ressources,
  technologies et modes de carte.
- Résultats classés par pertinence et type, navigation clavier complète.
- Entrée = ouvrir ; variante de commande uniquement pour des actions sûres et explicites.

**Critère de fin** : une information nommable est atteignable en quelques frappes sans
connaître le menu qui la contient.

### P6 — Profondeur économie et territoire — EN COURS (fonctionnellement complet ; contrôle visuel release restant)

- Trésor : solde, postes, évolution, projection et accès aux sources territoriales.
- Stocks : stock, flux, couverture, producteurs, consommateurs et chaîne d’achat.
- Marché : origine locale/marché proche/mondial, distance, marges et taxes.
- Province : capacité, population, classes, satisfaction, agitation et modificateurs,
  tous reliés à leurs décompositions façade.

**Critère de fin** : expliquer une variation économique ou provinciale ne demande plus
de comparer manuellement plusieurs panneaux.

### P7 — Profondeur militaire — FAIT

- Corps : composition, force effective, position, destination, durée, ravitaillement,
  renfort et état de combat.
- Multi-sélection : synthèse agrégée puis détail par corps sans perdre la sélection.
- Déplacement : aperçu chemin/temps/terrain/mer/légalité avant confirmation.
- Stacking et fusion : participants, total, contraintes, résultat attendu.
- Combat : camps, modificateurs, phases, pertes, moral/ralliement et accès à la région.
- Après-combat : alerte localisée ouvrable, pertes conservées dans le journal permanent
  et retour en un clic au théâtre après disparition de la notification.

**Critère de fin** : sélectionner, déplacer, fusionner et comprendre un combat forme un
seul parcours continu, avec feedback immédiat.

### P8 — Profondeur politique, diplomatie et savoir — FAIT

- Factions — `FAIT` : soutien, tendance, griefs, coup, assise sociale, effet des
  politiques et capture de l'État.
- Conseil — `FAIT` : poste, titulaire, loyauté et cible, contribution décomposée,
  vacance et candidats prévisionnels.
- Diplomatie — `FAIT` : opinion et légalité décomposées, engagements, portée des pactes,
  créneaux, lieux et meilleure route partagée.
- Technologie — `FAIT` : coût, revenu, prérequis, accès culturel, effet concret et chemin
  de recherche complet avec prochaine étape.
- Culture/foi — `FAIT` : composition locale, dominante réelle, rapport à la Couronne,
  friction, foi vivante, contact commercial et projection de fusion issue du tick moteur.

**Critère de fin** : chaque score politique ou relationnel expose sa provenance et ses
conséquences concrètes.

### P9 — Mémoire de travail du joueur — FAIT

- `FAIT` — favoris/épingles sur références d’information.
- `FAIT` — historique récent consultable, distinct de l’arrière/avant immédiat.
- `FAIT` — comparaison live de deux pays, provinces, corps ou ressources sur un
  sous-ensemble commun.
- `FAIT` — conservation prudente par sidecar lié à l'emplacement de sauvegarde, sans
  modifier le format moteur.

**Critère de fin** : le joueur peut suivre une question stratégique dans le temps sans
refaire son parcours de menus.

### P10 — Vérification et audit d’usage — EN COURS (automatisation complète ; contrôle visuel release restant)

- `FAIT` — tests GDScript des références, routes, historique, mémoire et classement de
  recherche, plus bancs des cartes d'information par domaine.
- `FAIT` — audit headless Godot après chaque tranche et parse complet final.
- `FAIT` — parcours automatisés alerte→cause, topbar→détail, recherche→objet,
  armée→combat et mémoire→objet.
- `FAIT` — clavier, focus, résolution projet, textes longs et destinations des liens.
- `FAIT` — dix questions stratégiques mesurées à trois interactions maximum.
- `DIFFÉRÉ` — contrôle visuel manuel de la build release, commun au dernier verrou P6.

**Critère de fin** : aucune erreur GDScript, aucun lien mort, et chaque question testée
atteint sa réponse en trois interactions au plus hors saisie de recherche.

## Ordre de livraison

1. P0 + P1 : socle transversal.
2. P2 : bénéfice visible immédiat sur l’interface actuelle.
3. P3 + P4 : qualité des explications et des décisions.
4. P5 : accès universel.
5. P6 à P8 : migration domaine par domaine selon les données réellement exposées.
6. P9 + P10 : mémoire de travail et durcissement final.

## Journal d’implémentation

### 2026-07-12

- `FAIT` — création du plan vivant.
- `FAIT` — inventaire ciblé de `main.gd`, `topbar.gd`, `sidebar.gd`,
  `sidebar_drawer.gd`, `tooltip_server.gd`, `concepts.gd` et `codex.gd`.
- Constat : les données et les surfaces de lecture existent déjà en grande partie ; le
  premier manque transversal est le routage stable entre elles.
- `FAIT` — ajout de `InfoRef`, contrat commun pour pays, province, région, corps,
  ressource, technologie, onglet et mode de carte.
- `FAIT` — ajout de `NavigationHub` : historique de 48 vues, déduplication consécutive,
  arrière/avant et restauration sans rejouer d'action moteur.
- `FAIT` — branchement dans `Main` des routes pays, province, région, corps, ressource,
  technologie, onglet et mode de carte ; raccourcis `Alt+Gauche` / `Alt+Droite`.
- `FAIT` — clics de province, pays contextuel, alertes, popups et annales localisées
  raccordés au même historique.
- `FAIT` — contexte optionnel du tiroir et mise en évidence d'une ressource ciblée.
- `FAIT` — topbar ouvrable sans changement de mise en page : royaume, budget, stocks,
  armes, nourriture, savoir, factions, conseil et démographie. Curseur et tooltip
  annoncent les cellules ouvrables.
- `VÉRIFIÉ` — `tests/navigation_hub_test.gd` : routes invalides, déduplication,
  arrière et avant (`OK`, 5 routes observées).
- `VÉRIFIÉ` — démarrage Godot 4.6.3 headless sans erreur GDScript. Les avertissements
  RID à l'arrêt sont ceux déjà observés lors d'un `--quit` immédiat.
- `FAIT` — `TooltipServer` accepte désormais un payload structuré optionnel tout en
  conservant les tooltips texte et la cascade de concepts existants.
- `FAIT` — cartes structurées : titre, état, tendance, lignes chiffrées et actions
  deep-linkées. Les actions utilisent les mêmes requêtes que le reste de l'interface.
- `FAIT` — épinglage explicite d'une carte et fermeture volontaire ; une carte épinglée
  ne disparaît plus quand la souris quitte sa source.
- `FAIT` — première migration verticale : le Trésor expose stock d'or, tendance
  mensuelle, postes budgétaires mensuels et lien vers le budget.
- `VÉRIFIÉ` — `tests/info_card_test.gd` : contenu, décomposition, deep-link et commandes
  d'épinglage (`OK`).
- `EN COURS` — P4 : normalisation des raisons de légalité à partir des lecteurs moteur
  existants. Première cible : construction, qui expose déjà le refus exact du drain.
- `FAIT` — `build_legal` conserve ses champs historiques et expose en plus le contrat
  structuré `allowed`, `reason_code`, `reason_label`. Codes stables : `ok`,
  `structural`, `insufficient_gold`, `missing_material`, `missing_tier_tech`.
- `FAIT` — les lignes d'édifice consomment le libellé façade avec fallback compatible
  sur l'ancien entier ; le refus au clic utilise exactement la même source.
- `FAIT` — carte de décision Construction : état constructible/bloqué, premier verrou
  moteur, durée, effet, coût en or face au trésor et recette face au stock national.
  Les montants viennent uniquement de `building_roster`, `country_info` et
  `country_stocks` ; aucun « manque exact » n'est inventé car crédit et imports ne sont
  pas encore décomposés par le lecteur de légalité.
- `VÉRIFIÉ` — `tests/build_info_card_test.tscn` (`OK`) avec la DLL debug actuellement
  chargée ; compatibilité ascendante du fallback prouvée.
- `VÉRIFIÉ` — binding C++ compilé et DLL release liée sans symbole manquant ; présence
  des trois nouveaux codes de refus contrôlée dans le binaire release.
- `DIFFÉRÉ` — remplacement de la DLL debug : plusieurs instances Godot utilisateur la
  gardent chargée. L'UI reste fonctionnelle grâce au fallback `reason` historique ; la
  prochaine compilation debug activera automatiquement les nouveaux libellés façade.
- `À FAIRE` — terminer P4 sur armée, diplomatie et technologie après ajout de lecteurs
  de raison moteur dédiés ; aucune raison ne sera déduite silencieusement dans l'UI.
- `EN COURS` — P6 repriorisé par les références visuelles : topbar avec tendances
  visibles, hover Trésor hiérarchisé et budget complet séparé revenus/dépenses.
- `FAIT` — topbar économique : le Trésor, les matériaux, les armes, la nourriture et
  le savoir affichent désormais leur variation mensuelle directement sous la valeur.
- `FAIT` — hover Trésor rapproché de la référence : stock et solde mensuel, total des
  revenus, postes de revenus, total des dépenses, postes de dépenses, crédit, puis lien
  vers le panneau Économie. Les lignes portent un sens textuel et coloré.
- `FAIT` — tiroir Économie : abandon de la limite arbitraire à cinq postes ; revenus et
  dépenses sont séparés, totalisés et affichés intégralement en rythme mensuel. Le
  défilement existant absorbe la hauteur supplémentaire.
- `VÉRIFIÉ` — démarrage headless sans erreur après correction d'une inférence Variant
  traitée comme erreur par Godot.
- `DIFFÉRÉ` — contrôle visuel fenêtré de cette tranche : l'autorisation de lancer une
  nouvelle fenêtre Godot a expiré et les instances utilisateur ouvertes n'ont pas été
  manipulées. Ne pas marquer P6 `FAIT` avant cette vérification réelle.
- `FAIT` — lecteur de stock enrichi sans calcul UI : production brute mensuelle et
  consommation brute mensuelle traversent désormais `ScpsStock` puis le binding Godot,
  en plus du stock, flux net, prix, bande de marché et couverture existants.
- `FAIT` — chaque ligne de l'onglet Stocks ouvre une carte structurée : état du marché,
  variation mensuelle, production, consommation, couverture, prix moyen et deep-link
  vers le même bien dans l'onglet Marché.
- `VÉRIFIÉ` — `stock_info_card_test.tscn` (`OK`), compilation C complète du banc API,
  compilation ciblée du binding et lien de la DLL release réussis.
- `DIAGNOSTIQUÉ` — le segfault tardif de `scps_api_demo` venait de l'environnement de
  test : `tmpfile()` ciblait `D:\MSYS2\tmp`, interdit dans le sandbox. La sauvegarde
  échouait proprement (`scps_sim_load` retournait 1), puis le banc continuait malgré
  l'échec et appelait `strncmp` sur une sortie non initialisée. Ce n'était ni la genèse,
  ni le lecteur de stock, ni les changements moteur concurrents.
- `VÉRIFIÉ` — avec `TMP`, `TEMP` et `TMPDIR` dirigés vers `build/tmp`, reconstruction
  normale `-O2` puis exécution isolée de `scps_api_demo` : **179 réussis, 0 échoués**.
  Le build de diagnostic `-O0 -g3` a également franchi le save/load ; son exécution
  complète a seulement dépassé la limite temporelle de 180 s.
- `FAIT` — chaque ligne Marché fournit maintenant une carte structurée : catégorie
  moteur, prix, bande de marché, tendance mensuelle, stock national, production,
  consommation et couverture. Le deep-link inverse rouvre le même bien dans Stocks ;
  le focus existant conserve la ressource sélectionnée entre les deux onglets.
- `DIAGNOSTIQUÉ` — le crash natif `0xC0000005` du lancement Godot headless venait du
  confinement du processus, pas du projet ni de la DLL. Le même test hors sandbox et
  un mini-projet sans GDExtension terminent tous deux normalement.
- `VÉRIFIÉ` — `stock_info_card_test.tscn` étendu à la fiche Marché passe dans le projet
  réel ; `info_card_test.gd` et `navigation_hub_test.gd` passent aussi dans leur mode
  correct `--script`.
- `FAIT` — la ligne habitants/prospérité de Province ouvre une synthèse territoriale :
  population face au logement, capacité de services, prospérité, loyauté, agitation,
  impôt annuel et tenue de siège. Un deep-link mène à l'économie nationale.
- `FAIT` — la section Satisfaction ouvre une carte causale : score de chaque classe,
  classe absente explicitement nommée, loyauté locale, agitation et causes moteur avec
  apport signé et résorption annuelle lorsqu'elle existe.
- `VÉRIFIÉ` — `province_info_card_test.tscn`, `stock_info_card_test.tscn` et
  `build_info_card_test.tscn` passent dans le projet Godot réel. Les deux nouveaux
  lecteurs territoriaux sont testés sur surpopulation, fiscalité, défense, seuils de
  satisfaction, classes absentes et décomposition de l'agitation.
- `FAIT` — lecteur façade `scps_market_quote` : depuis la capitale, il expose sans
  mutation le Centre proche et son propriétaire, stock local accessible, marge de
  distance, accès au réseau mondial, profondeur mondiale, double marge, puissance
  commerciale restante, quantité livrable et coût estimé pour chaque étage.
- `FAIT` — la carte d'un bien au Marché affiche maintenant cette chaîne complète sous
  les flux nationaux : Centre proche → devis local → accès/réserve mondiale → devis
  mondial. Les états « aucun marché », « indisponible » et « accès fermé » sont nommés,
  jamais laissés à une couleur ou à un bouton muet.
- `VÉRIFIÉ` — `scps_api_demo` monte à **182 réussis, 0 échoué** avec les invariants du
  devis commercial ; binding release reconstruit, symbole `market_quote` présent dans
  la DLL, puis `stock_info_card_test.tscn` repasse avec le sourcing local/mondial.
- `FAIT` — le clic-destination n'efface plus la sélection militaire. Le panneau et
  l'anneau restent visibles après l'ordre ; clic droit demeure l'annulation explicite.
  Le joueur voit donc le même corps passer de son état courant à « En marche » au drain.
- `FAIT` — accusé immédiat d'ordre : destination nommée, nombre de corps transmis et
  refus explicite si la cible ou le corps est indisponible. L'ordre n'est plus un clic
  silencieux en attente d'un tick invisible.
- `FAIT` — `ScpsArmyInfo` expose maintenant le prochain saut, les noms de départ et de
  destination, jours restants/total et progression de l'étape, déroute, ralliement et
  effectif attendu, plus le journal étapes/batailles/régions réduites.
- `FAIT` — panneau multi-corps : total agrégé conservé, puis jusqu'à six lignes de corps
  individualisées avec position, phase, destination, progression et états critiques.
  Fusionner/Scinder sont désactivés avec une raison lorsque co-localisation, effectif ou
  phase de bataille/mer rendent l'action impossible.
- `VÉRIFIÉ` — binding release recompilé, `scps_api_demo` toujours **182/182** ;
  `army_panel_test.tscn`, `army_selection_flow_test.tscn` et `army_move_audit.tscn`
  passent dans le projet Godot réel. Le test de flux prouve ordre transmis, destination
  nommée, accusé positif et sélection conservée.
- `DIFFÉRÉ` — DLL debug toujours chargée par quatre instances Godot utilisateur ; elles
  n'ont pas été fermées. La DLL release contient le nouveau lecteur, le fallback de la
  carte garde la debug courante compatible en attendant son prochain rebuild.

### 2026-07-13

- `FAIT` — aperçu de marche en lecture pure dans le moteur : pour chaque corps, la
  façade expose route terrestre complète, nombre de sauts, durée estimée et issue
  d'arrivée (rester, repositionner ou assiéger), avec un motif précis si l'ordre serait
  refusé. Le calcul reprend le même chemin et les mêmes coûts que l'ordre réel.
- `FAIT` — survol d'une région en mode armée : la carte trace le chemin avant le clic,
  marque la cible en or ou rouge et le panneau affiche destination, durée, issue et
  portée d'un éventuel refus sur la multi-sélection.
- `VÉRIFIÉ` — la compilation C complète passe ; `scps_api_demo` reste à **182/182**
  et `campaign_demo` monte à **27/27**, dont route/durée/issue et absence de mutation.
  `army_panel_test.tscn`, `army_selection_flow_test.tscn` et
  `army_move_audit.tscn` passent dans le projet Godot réel ; le chargement headless de
  l'éditeur valide aussi l'overlay et la scène principale. La DLL release est reconstruite.
- `CORRIGÉ` — les lecteurs `ScpsArmyInfo` et `ScpsBattleInfo` annonçaient des
  « hommes » mais exposaient encore des paquets de 100. Total et quatre composantes
  sont maintenant convertis une seule fois dans la façade ; panneau, jeton de carte et
  scission utilisent la même unité. Un marqueur de schéma garde l'ancienne DLL debug
  compatible jusqu'au redémarrage des instances utilisateur.
- `CORRIGÉ` — le panneau de bataille additionnait auparavant tous les corps d'un pays,
  même distants. Il ne compte plus que les corps présents dans la région et réellement
  engagés dans la phase courante ; une place assiégée sans armée n'invente plus un
  défenseur national et nomme sa défense par ouvrages et vivres.
- `FAIT` — profondeur de stack/fusion : la multi-sélection affiche corps, effectif total,
  dispersion ou verrou bataille/mer, puis annonce quel identifiant survivra et quel
  effectif résultera de la fusion. Après l'ordre, la sélection bascule immédiatement sur
  ce corps survivant.
- `FAIT` — lecture live du combat enrichie : jour, chocs livrés, nombre de corps locaux,
  cohésion de chaque camp, renfort allié et score de guerre partagent désormais le
  même panneau. Les anciens `lossA/lossB` ont été identifiés comme simples reports
  fractionnaires, pas comme un cumul : ils ne sont plus affichés mensongèrement.
- `VÉRIFIÉ` — `scps_api_demo` monte à **185/185** (stacks locaux, hommes ×100 et
  cohérence total/composition), `campaign_demo` reste à **27/27** ; test ciblé du
  panneau d'armée et analyse headless complète Godot passent sans erreur. DLL release
  reconstruite, instances utilisateur laissées ouvertes.
- `DÉCISION` — les postures militaires sont abandonnées, pas seulement masquées :
  elles sortent du contrat d'interface et du plan P7. Le champ moteur historique reste
  uniquement un tombstone de sauvegarde inerte jusqu'à une future migration de format.
- `FAIT` — lecture tactique du prochain choc issue des fonctions moteur elles-mêmes :
  phase Choc/Accalmie, avantage du terrain, rivière et pont, contres de composition,
  rapport de puissance avant l'aléa quotidien ±15 % et seuil réel de rupture. Le panneau
  explique ainsi pourquoi un camp prend l'avantage sans promettre le résultat du jet.
- `NETTOYÉ` — posture retirée de `ScpsArmy`, du binding, du journal de commandes,
  des multiplicateurs de marche/siège, des fonctions publiques, des commentaires actifs
  et de `verbs_audit.gd`. Seul le champ tombstone demeure pour garder la disposition des
  sauvegardes existantes ; aucun comportement ne peut encore le lire ou le modifier.
- `VÉRIFIÉ` — `campaign_demo` monte à **29/29**, `scps_api_demo` à **186/186** ;
  `army_panel_test`, `battle_panel_test` et `army_selection_flow_test` passent, de même
  que l'analyse headless complète du projet. DLL release reconstruite.
- `VÉRIFIÉ PARTIEL` — `verbs_audit` confirme les **13 verbes restants** après retrait
  de la posture et leur drainage sans crash. Son ancien contrôle final de colonisation
  échoue ensuite (`1 → 1 province`) malgré une cible annoncée colonisable : anomalie
  territoriale distincte, consignée sans modification hors sujet.
- `VÉRIFIÉ` — après suppression du code de posture résiduel, `make determinism`
  confirme **5/5 hashes stables sur 12 ans** ; aucun comportement moteur n'a divergé.
- `FAIT` — pertes de bataille réellement cumulées : les champs sérialisés
  `FieldBattle.lossA/lossB` conservent désormais les morts confirmés du choc, du
  décrochage et de la poursuite. Le franchissement de chaque paquet de 100 reste
  strictement identique ; seule la partie entière n'est plus soustraite du lecteur.
- `FAIT` — le panneau live réaffiche un bilan honnête par camp. À la conclusion,
  le fil distingue victoire, défaite et bataille indécise, puis nomme les pertes
  confirmées du joueur et de l'adversaire dans la notification.
- `CORRIGÉ` — l'observation des fins de bataille ne surveille plus seulement le corps
  historique n°0 : elle suit chacun des huit champs et reconnaît donc les combats de
  n'importe quel corps, y compris ceux commencés et terminés dans le même pas mensuel.
- `VÉRIFIÉ` — `campaign_demo` monte à **30/30**, `scps_api_demo` reste à **186/186**,
  tests Godot `battle_panel`, `alerts_battle` et `army_panel` verts. Les hashes courts
  restent **strictement identiques 5/5** et `scps_viewer --savetest` passe **2/2** :
  aucun bump de sauvegarde n'est nécessaire. DLL release reconstruite.

- `FAIT` — profondeur de siège branchée sur les formules moteur : compte à rebours
  exact, progression estimée selon l'état courant, résistance de référence, niveau
  d'ouvrages, mois de vivres et multiplicateur de terrain apparaissent dans le panneau.
  L'estimation est explicitement nommée comme telle, car vivres et bâti peuvent évoluer.
- `CORRIGÉ` — le lecteur ne confond plus seulement « assiégeant étranger » et siège :
  il reconnaît aussi une armée qui reprend une région de son pays occupée par l'ennemi,
  nomme le véritable occupant comme défenseur et annonce « libération » plutôt
  qu'« occupation » à la chute.
- `VÉRIFIÉ` — `campaign_demo` monte à **31/31** et `scps_api_demo` à **187/187**
  avec les bornes du readout de siège ; aucun état moteur ni format de sauvegarde ajouté.

- `VÉRIFIÉ` — test Godot ciblé et analyse headless complète verts ; binding release
  reconstruit. Les cinq hashes de déterminisme restent strictement identiques et le
  `savetest` fraîchement relié passe **2/2**. Le premier lancement avait utilisé
  l'ancien exécutable car la cible correcte du Makefile est `scps`, pas le nom du
  fichier Windows `scps_viewer` ; après reliaison, aucune anomalie de sauvegarde.

- `FAIT` — aperçu de renfort par corps : le panneau annonce la vague visée (+100 hommes
  par type d'unité encore présent), les classes réellement mobilisables, les hommes
  garantis par l'arsenal national et le détail stock/requis de chaque catégorie d'arme.
  Si l'arsenal ne suffit pas, le recours possible au marché est nommé sans promettre
  un prix futur : achat, marge et trésor seront ceux du drain effectif.
- `FAIT` — la multi-sélection agrège les besoins sans compter plusieurs fois le même
  stock national ; seuls les corps effectivement ravitaillables entrent dans le total.
  Le bouton se grise avec une raison lisible et l'accusé d'ordre reprend volume visé
  et garantie avant imports. L'ancienne DLL debug conserve son fallback générique.
- `CORRIGÉ` — le prédicat moteur « renfort uniquement sur région nationale », jusque-là
  inutilisé, est maintenant revalidé au drain. Une pénurie de la bonne classe sociale
  est testée avant de pomper les armes, ce qui supprime une consommation sans recrue.
  Les milices peuvent enfin recevoir leur vague gratuite en armes manufacturées : leurs
  armes de fortune (`RES_NONE`) alimentent correctement le jeton interne de recrutement.
- `VÉRIFIÉ` — `campaign_demo` monte à **33/33** et `scps_api_demo` à **188/188** ;
  le test Godot `army_panel` et l'analyse headless complète passent. Binding release
  reconstruit, cinq hashes courts strictement inchangés et `savetest` **2/2** : le
  renfort n'ajoute aucun état sérialisé ni divergence hors des ordres joueur concernés.
- `FAIT` — les alertes transitoires de bataille, siège, libération et pillage ne sont
  plus des clics morts : clic gauche centre la carte sur la région de l'évènement ;
  clic droit demeure l'acquittement sans navigation. Les évènements non localisés ne
  reçoivent aucune destination inventée.
- `FAIT` — le journal permanent de la bande d'empire conserve la région des évènements
  localisés et les pertes confirmées des batailles. Ses lignes cliquables sont signalées
  par une teinte plus lisible ; le survol restitue le texte complet non tronqué et le
  clic revient au théâtre même après expiration de la notification immédiate.
- `VÉRIFIÉ` — analyse headless complète du projet et `alerts_battle_test.tscn` verts ;
  le test couvre le décodage des pertes, le routage d'une bataille localisée et
  l'absence volontaire de routage régional pour une paix.
- `FAIT` — l'aperçu de marche expose désormais la conséquence réelle du terrain :
  effectif au départ, pertes d'attrition projetées étape par étape avec la même
  fonction que le drain, effectif attendu à l'arrivée, pourcentage perdu et pire
  taux journalier rencontré. Aucune jauge de « ravitaillement » fictive n'est ajoutée :
  le moteur actuel modélise le renfort sur sol national et l'attrition de marche.
- `FAIT` — la projection multi-corps additionne départ, pertes et arrivée sans
  écraser les détails individuels ; l'aperçu passe de lisible à ambre puis rouge
  lorsque le coût humain devient notable. La route complète interne sert au calcul
  même si le tableau demandé par un appelant est volontairement borné.
- `VÉRIFIÉ` — `scps_api_demo` monte à **189/189**, `campaign_demo` reste à **33/33** ;
  tests Godot `army_panel`, `army_selection_flow` et analyse headless verts. DLL
  release reconstruite. Les cinq hashes courts restent strictement identiques :
  la projection est une lecture pure et ne modifie ni ordre ni simulation.

### 2026-07-13 — P4 à P6

- `FAIT` — P4 Diplomatie : nouveau lecteur `scps_diplo_action_legal` commun aux sept
  verbes. Il expose autorisation, premier verrou stable, libellé, coût, or disponible,
  manque, délai, consentement attendu et sens de l'embargo. Le panneau pays ne déduit
  plus la légalité depuis l'opinion ou des combinaisons de drapeaux.
- `CORRIGÉ` — le bouton d'embargo envoyait toujours l'ordre `ON`, même lorsque son
  libellé demandait de lever l'embargo. Il transmet maintenant le sens exact du lecteur.
- `FAIT` — P4 Technologie : `tech_research_block` factorise les portes réelles
  (acquise, héritage, ruines, prérequis), complétées par la porte d'âge dans la façade.
  Chaque nœud expose sa raison, la réserve de points et le manque ; le clic obéit au
  booléen moteur, sans reconstituer les règles dans GDScript.
- `FAIT` — P4 est clos : construction, armée, diplomatie et technologie possèdent
  désormais un retour explicite. L'armée a été couverte par P7 (marche, renfort,
  fusion/scission, bataille et sièges), sans postures.
- `FAIT` — P5 : palette universelle `Ctrl+K`, navigation clavier complète et classement
  partagé avec la recherche du Codex. L'index couvre panneaux, verbes réellement live,
  concepts, pays connus, provinces/régions de propriétaires connus, corps du joueur,
  ressources nationales, technologies et modes de carte.
- `SÛRETÉ` — une entrée « Action » n'exécute jamais un ordre moteur depuis la palette :
  elle ouvre le passage correspondant du Codex. Les objets de lecture naviguent via
  `InfoRef` et rejoignent le même historique arrière/avant. Pays et territoires inconnus
  restent hors index afin de ne pas percer le brouillard.
- `FAIT` — une technologie trouvée ouvre directement son dossier dans l'arbre, sans
  lancer la recherche ; seul un clic volontaire sur son atome reste actionneur.
- `VÉRIFIÉ` — `search_palette_test` construit l'index sur un monde réel, valide les
  types et toutes les routes, l'insensibilité aux accents et la priorité des résultats.
  Analyse headless complète sans erreur GDScript.
- `FAIT` — P6 Stocks/Marché : nouveau lecteur `scps_stock_regions`, trié par activité,
  qui nomme le principal territoire producteur et consommateur de chaque bien. Les
  cartes Stocks et Marché les affichent et proposent un retour direct à la carte.
- `FAIT` — P6 Trésor : le résumé budget expose maintenant les rythmes mensuels, le
  trésor projeté en fin d'année au rythme courant et l'autonomie en mois du couple
  trésor + ligne de crédit. Topbar et tiroir Économie partagent ces mêmes valeurs.
- `VÉRIFIÉ` — `scps_api_demo` atteint **197/197**, dont contrats P4, tri territorial et
  projections P6 ; `campaign_demo` reste **33/33**. `make determinism` confirme les
  hashes historiques **5/5 strictement identiques**. DLL release reconstruite et
  export release temporaire réussi.
- `VÉRIFIÉ` — analyse Godot headless complète, `search_palette_test` et
  `stock_info_card_test` verts avec la DLL debug historique et leurs fallbacks.
- `DIFFÉRÉ` — le contrôle visuel release reste la seule condition avant de marquer P6
  `FAIT`. Une build release dédiée a été exportée, mais le contrôle Windows a détecté
  une activité utilisateur dans une autre application et la capture de fenêtre est
  restée ambiguë ; automatisation arrêtée immédiatement, aucune fenêtre utilisateur
  manipulée. La DLL debug reste volontairement non remplacée tant que les instances
  Godot utilisateur la verrouillent.

### 2026-07-13 — P8 politique

- `FAIT` — Factions : la façade distingue maintenant la part effective de l'assise
  démographique et expose le déplacement signé produit par les politiques. Elle nomme
  aussi, pour chaque faction, rancœur, contribution exacte à la tension de coup,
  faction porteuse du risque et faction qui capture le plus l'État.
- `FAIT` — un troisième sous-onglet `Factions` rejoint Gouvernement et Politiques dans
  le tiroir Conseil. Il donne le rapport de forces complet sans charger la topbar ;
  chaque ligne ouvre une carte structurée. Un clic sur une faction de la topbar arrive
  directement sur cette vue.
- `FAIT` — Conseil : la cible réelle de loyauté est devenue un lecteur public du moteur.
  Les sièges et candidats exposent désormais base, Administration, Loyauté, Corruption,
  valeur avant plafond et éventuel clamp. Le GDScript ne reconstitue plus les coefficients
  et ne déduit plus la loyauté d'un candidat par différence.
- `FAIT` — cartes de survol des titulaires et candidats : loyauté actuelle/cible,
  efficacité livrée, contribution nette, traitement annuel et faction sont regroupés
  sans agrandir les cartes permanentes.
- `VÉRIFIÉ` — `scps_api_demo` monte à **199/199** avec les nouveaux contrats politiques ;
  `campaign_demo` reste **33/33**. Godot headless analyse le projet sans erreur et la
  DLL release est reconstruite. Les cinq hashes de déterminisme restent strictement
  identiques (`1fa06b60`, `fc9e670a`, `a398d0fa`, `ef7f249c`, `835f910f`).

### 2026-07-13 — P8 diplomatie, technologie et culture/foi

- `FAIT` — Diplomatie : `scps_diplo_context` réunit les engagements actifs, score de
  guerre, pactes, embargo, trêve, vassalité, créneaux d'alliance, valeur commerciale et
  meilleure route partagée. La fiche pays donne accès à la capitale adverse et aux deux
  extrémités de la relation sans reconstruire ces liens dans l'interface.
- `FAIT` — Technologie : chaque cible non acquise expose le premier nœud réellement
  recherchable, le nombre d'étapes restantes et la chaîne complète de prérequis. Le
  dossier persistant reste concis ; le survol conserve le chemin détaillé.
- `FAIT` — Culture/foi : les groupes exposent désormais leur foi vivante et l'unique
  dominante réelle. La fiche provinciale regroupe rapport à la Couronne, dérive d'éthos,
  friction moyenne/maximale, foi locale/foi d'État et partenaire commercial effectif.
- `PRÉCISÉ` — l'estimation de fusion ne reprend pas le temps théorique générique de
  `culture_can_syncretize` : elle projette la même réduction annuelle que
  `demography_contact_tick`, avec ouverture de la porte, mer/terre, tradition du pays et
  contrôle de cristallisation suivant. Seule la province-pivot réellement transformée
  par le moteur annonce ce contact.
- `VÉRIFIÉ` — `scps_api_demo` monte à **205/205** ; compilation C sans avertissement,
  binding Godot release reconstruit et analyse headless complète sans erreur. P8 est clos.

### 2026-07-13 — P9 mémoire de campagne

- `FAIT` — `NavigationHub` conserve maintenant 24 vues récentes distinctes et 16
  épingles, sans les confondre avec la pile immédiate arrière/avant. Ouvrir la mémoire
  ne remplace pas la vue courante : celle-ci reste donc épinglable après consultation.
- `FAIT` — panneau `Mémoire de campagne`, accessible par `Ctrl+M` ou par la recherche
  universelle. Une ligne récente ou épinglée revient directement à l'objet par la route
  `InfoRef` originale, contexte et surface compris.
- `FAIT` — comparaison homogène de deux pays, provinces, corps ou ressources. Les
  valeurs ne sont pas des captures : le panneau relit le moteur chaque mois et compare
  le sous-ensemble partagé (population, scores, composition, flux, couverture, etc.).
- `FAIT` — les récents, épingles et la paire comparée suivent chaque emplacement via
  `user://campaign_memory_<slot>.cfg`. Une nouvelle partie remet la mémoire à plat ; un
  chargement restaure uniquement des requêtes `InfoRef` encore valides. Le format de
  sauvegarde moteur et son déterminisme restent intacts.
- `VÉRIFIÉ` — analyse headless complète propre ; `navigation_hub_test` couvre
  déduplication, épinglage, homogénéité de comparaison et non-pollution de la vue
  courante. `memory_panel_test` exerce un monde réel, le rendu comparatif et un cycle
  sidecar temporaire sauvegarde/chargement. `search_palette_test` reste vert.

### 2026-07-13 — P10 audit d'usage automatisé

- `FAIT` — `ui_usage_audit_test` ouvre les vraies surfaces à la résolution projet
  1600×900, vérifie que le panneau Mémoire reste dans le viewport avec un texte de plus
  de 400 caractères et que la recherche donne effectivement le focus à son champ.
- `FAIT` — les **11 scènes** de `godot/project/tests/` passent individuellement : alertes,
  armée, flux de sélection, bataille, construction, mémoire, province, recherche,
  stocks, trésor et audit d'usage. Le parse headless final du projet est sans erreur.
- `FAIT` — mesure des dix questions fréquentes, hors saisie de recherche :

| Question stratégique | Porte la plus courte | Interactions |
|---|---|---:|
| Pourquoi mon trésor varie ? | Trésor → poste | 2 |
| Où mon bien déficitaire est-il produit ? | Recherche → ressource → territoire | 2 |
| Pourquoi cette province rapporte-t-elle ce montant ? | Province → résumé | 2 |
| Cette culture va-t-elle fusionner ? | Province → culture/foi | 2 |
| Quelle faction porte le coup ? | Faction topbar → dossier | 2 |
| Pourquoi cette action diplomatique est-elle bloquée ? | Pays → verbe | 2 |
| Quel lieu relie ces deux pays ? | Pays → portée diplomatique | 2 |
| Quel chemin mène à cette technologie ? | Technologie → dossier/chemin | 3 |
| Combien d'hommes arriveront après la marche ? | Corps → survol cible | 2 |
| Où et pourquoi ai-je perdu cette bataille ? | Alerte/journal → théâtre | 1 |

- `VÉRIFIÉ` — `scps_api_demo` **205/205**, `campaign_demo` **33/33** et déterminisme
  **5/5**, avec les hashes inchangés `1fa06b60`, `fc9e670a`, `a398d0fa`, `ef7f249c`,
  `835f910f`. `git diff --check` reste propre.
- `DIFFÉRÉ` — le contrôle visuel manuel release demeure volontairement non automatisé
  tant qu'une instance Godot utilisateur est active. Aucun processus utilisateur n'a
  été fermé ; seuls trois processus headless créés par un lanceur de test défectueux ont
  été identifiés par leur heure/PID puis arrêtés. Cette vérification est le verrou commun
  restant de P6 et P10, pas un manque fonctionnel.

### 2026-07-13 — Hiérarchie visuelle et accès immédiat

- `FAIT` — le centre redevient le théâtre de la carte : commandement d'armée ancré
  à gauche, diplomatie et bataille à droite devant le ledger. Le panneau de bataille
  ne duplique plus le score de guerre ; seules les modales importantes continuent de
  prendre le milieu et de mettre le monde en pause.
- `FAIT` — une section `GUERRES` ouvre le menu droit : un adversaire par ligne, icône
  agrandie, score brut signé du point de vue joueur et jauge divergente centrée sur
  zéro. Le clic rejoint directement la fiche diplomatique du pays concerné.
- `FAIT` — les notifications actives quittent la colonne flottante et deviennent une
  liste `NOTIFICATIONS` du menu droit. Aucun regroupement automatique ne les masque
  lorsqu'un panneau s'ouvre ; clic gauche et acquittement droit gardent leurs actions.
  Le ledger occupe la hauteur disponible et défile, journal compris, au lieu de couper
  silencieusement ses dernières lignes.
- `FAIT` — densité revue : sections et rangées communes resserrées, interlignes du
  ledger réduits. Les icônes de topbar, d'âge, d'émissaire, de guerre, de notification,
  d'édifice, de manufacture et de ressource sont agrandies et leurs textes réalignés.
- `FAIT` — les valeurs signées restent signées : un score négatif est affiché tel quel ;
  seul le dessin de sa jauge est borné. Aucun clamp ne transforme une mauvaise situation
  en zéro rassurant.
- `FAIT` — les hovers multilignes sont présentés comme des listes. L'épinglage, qui
  retenait une carte hors contexte, est supprimé ; la chaîne se ferme dès la sortie de
  sa hitbox élargie, avec seulement 120 ms pour franchir l'espace vers un sous-hover.
- `FAIT` — une languette `Construction`, visible sur le bord droit de toute province
  possédée, ouvre directement le panneau sur cette province. L'ancienne petite case `+`
  enfouie dans la liste des bâtiments disparaît. Les textes d'ambiance des édifices sont
  retirés du panneau comme du hover ; restent coûts, durée, recette, stocks et effets.
- `VÉRIFIÉ` — `git diff --check`, import/analyse Godot headless et `core_demo` **35/35**
  sont verts. Les scènes Godot exécutées en jeu restent momentanément invérifiables :
  une scène de référence non modifiée et le test hover terminent tous deux avec le même
  code d'accès natif pendant que l'instance Godot utilisateur verrouille l'extension.
  Aucun processus utilisateur n'a été fermé et aucun contrôle visuel automatisé lancé.

### 2026-07-13 — Opinions et pilotage budgétaire

- `FAIT` — toute opinion affichée est désormais une jauge divergente canonique
  `-100 ← 0 → +100`. La fiche diplomatique montre la valeur actuelle par remplissage,
  le point d'équilibre par un repère distinct et réserve le texte à la tendance ; la
  liste diplomatique du menu droit conserve le même langage visuel.
- `FAIT` — le panneau Économie possède trois curseurs fiscaux indépendants :
  **Laboureurs**, **Artisans** et **Noblesse**. Chacun couvre `×0,1…×2` par pas de `0,1`
  et modifie réellement l'ambition de prélèvement de la classe ; évasion, rendement
  net et grogne continuent de passer par sa tolérance et sa satisfaction.
- `FAIT` — quatre enveloppes de dépense sont pilotables sur la même plage :
  **Investissement public**, **Entretien des bâtiments**, **Armée** et **Flotte**.
  L'investissement règle la remise en circulation du trésor ; le sous-entretien met
  l'infrastructure en friche ; sous-payer les armées provoque des désertions et
  sous-payer les flottes accélère leur délabrement. Les valeurs négatives du bilan
  restent affichées comme telles : les multiplicateurs sont des décisions, pas des
  clamps cosmétiques sur les résultats.
- `FAIT` — chaque conseiller dispose maintenant d'un curseur continu de paie
  `×0,1…×2`, en remplacement des quatre paliers. La valeur commande toujours le coût
  réel et la cible de loyauté. Un tick moteur ne rompt plus un glisser en cours ; la
  capture prend fin au relâchement de la souris.
- `MOTEUR` — les politiques sont journalisées par `CMD_BUDGET_POLICY`, revalidées au
  drain, lues par une membrane publique et persistées dans `WorldEconomy`. Le format de
  sauvegarde passe à **v82** ; les anciennes saves sont donc refusées explicitement.
- `VÉRIFIÉ` — compilation C sans avertissement ; `scps_api_demo` **210/210** (neutralité
  ×1, round-trip des commandes, valeurs distinctes, clamps, save/load),
  `statecraft_demo` **74/74**, `econ_tax_demo` **8/8**, `campaign_demo` **33/33**,
  `navy_demo` **20/20** et `warhost_demo` **6/6**. Le parse Godot headless est propre et
  l'objet C++ des bindings est compilé. Le déterminisme reste **5/5** avec les hashes
  historiques inchangés (`1fa06b60`, `fc9e670a`, `a398d0fa`, `ef7f249c`, `835f910f`).
- `RÉSOLU` — après confirmation qu'aucune fenêtre utilisateur n'était ouverte, le vieux
  processus Godot sans fenêtre a été fermé et la DLL debug reconstruite. Les nouveaux
  curseurs et lecteurs sont désormais chargés par la variante de développement.

### 2026-07-13 — Évènements entièrement renseignés

- `FAIT` — chaque décision en attente reste branchée sur le verbe journalisé
  `CMD_EVENT_CHOICE`; le bouton ne simule aucun résultat côté Godot et le drain
  revalide toujours le slot et l'option avant d'appeler `pending_event_resolve`.
- `FAIT` — la membrane d'évènement expose désormais, pour chaque option, le libellé,
  le texte d'action (`blurb`), le flavor, les effets mécaniques chiffrés et la variation
  d'or physique signée. Une entrée historique sans flavor dédié retombe explicitement
  sur son blurb : aucune option affichée ne reste vide.
- `PRÉCISÉ` — le montant d'or n'est pas recalculé en GDScript. Le lecteur emploie la
  même assiette que `resolve_treasury_mois` — taxes de l'année / 12 × IPM courant — et
  reproduit aussi le clamp du vieux coût fixe sur le liquide de la province porteuse.
  L'UI affiche donc `Coût N or`, `Gain N or` ou `Coût 0 or`, au montant qui serait
  effectivement appliqué si le choix était drainé à cet instant.
- `FAIT` — les flèches vagues disparaissent : légitimité, agitation, institutions,
  défense bâtie, fertilité, coercition, influence, connectivité, Brèche, population et
  probabilité d'un pari portent leurs deltas numériques. Les cartes de choix affichent
  directement action, chiffres et flavor ; le survol n'est plus nécessaire pour
  comprendre la décision.
- `VÉRIFIÉ` — compilation C et binding C++ ciblé sans avertissement, parse Godot
  headless propre, `scps_api_demo` **211/211**. Le banc de dialogue exige maintenant
  autant de textes, flavors, effets et prix finis que d'options.
- `LIVE` — DLL debug reconstruite puis `event_dialog_audit.tscn` exécuté sur une vraie
  décision à trois choix : ouverture et pause, trois cartes complètes, choix drainé,
  fermeture et vitesse restaurée. Résultat **EVENT DIALOG AUDIT OK**.

### 2026-07-13 — Tiroir diplomatique et paix composée

- `FAIT` — la fiche pays est désormais un tiroir latéral à hiérarchie stable : résumé
  du pays (**habitants, éthos/régime effectif, statut politique, territoires**), opinion
  canonique `−100/+100`, statut diplomatique et engagements, puis les verbes primaires
  **Proposer une alliance** et **Déclarer la guerre**. Les lignes sont resserrées ; les
  conséquences et premiers verrous restent lisibles sans hover.
- `FAIT` — **Actions économiques** est un sous-tiroir contenant pacte migratoire et
  pacte commercial. **Actions antagonistes** contient embargo et revendication. Cette
  dernière nomme maintenant sa cible réelle (`Revendiquer X`) ; le territoire reste
  épinglé pendant la maturation et jusqu'à consommation/expiration du casus belli.
- `FAIT` — **Faire la paix** est toujours visible dans la fiche et grisé en temps de
  paix. En guerre, il ouvre un tiroir imbriqué : territoires nommés et occupés avec le
  vrai `diplo_province_price`, or, réparations, humiliation, pillage, libération,
  vassalisation et fragmentation. Le total courant est comparé en permanence au score
  positif disponible ; l'émissaire ne bloque que l'envoi, jamais la lecture des termes.
- `MOTEUR` — l'or demandé coûte `0…25` points ; chaque point représente exactement
  `3 % × revenu mensuel de la cible` (`revenu annuel / 12`), borné par son trésor réel.
  Les réparations prélèvent physiquement `10 %` du revenu, mensuellement pendant dix
  ans. Humilier vide les trois sièges du conseil ; piller transfère `5 %` de chaque
  stock ; libérer impose l'éthos du vainqueur à toutes les provinces et populations ;
  vassaliser coûte la somme des prix provinciaux ; fragmenter coûte `100` et crée un
  État vivant par région, dans les emplacements disponibles.
- `MOTEUR` — une cession de territoire exige l'occupation réelle et réutilise le corps
  historique du règlement (propriété provinciale, cicatrice, légitimité, rancune,
  saccage et captifs). Toute offre est revalidée au drain : cible, guerre, doublons,
  occupation, coûts et score. La paix blanche conserve le consentement de l'IA.
- `SAVE` — la revendication territoriale épinglée et les dix années de réparations sont
  persistées dans `DiploState`; le format passe à **v83** et `save_sane` borne les nouveaux
  champs.
- `VÉRIFIÉ` — compilation C sans avertissement, extension Godot debug reconstruite,
  `scps_api_demo` **216/216**, `diplo_demo` **85/85**, `diplo_audit.tscn` vert après instanciation réelle du
  tiroir, `--savetest` **2/2** (continuité byte-identique + corruption refusée), et
  déterminisme **5/5** avec les cinq hashes historiques inchangés.

### 2026-07-21 — Crédit rationné par les prêteurs

- `FAIT` — suppression des deux murs côté débiteur : ligne proportionnelle à la population
  et plafond dette/revenu à 300 %. Le ratio dette/revenu reste une information et devient
  l'assiette d'un taux convexe ; il n'interdit jamais l'emprunt.
- `FAIT` — chaque ordre et chaque État prêteur conserve une réserve liquide, une limite de
  portefeuille et une limite d'exposition au débiteur. Les créances existantes consomment
  réellement cette marge ; le rachat de dette respecte les mêmes bornes.
- `FAIT` — le service annuel emploie d'abord le surplus du débiteur, puis tente un
  refinancement physique. Le défaut ne commence que si l'échéance reste impayée après
  fermeture du marché ; le créancier étranger courant ne peut plus être remplacé
  silencieusement par un autre.
- `FAIT` — l'IA débitrice ne relâche plus sa fiscalité en fonction d'une prudence de dette :
  seul le garde bootstrap day-1 subsiste. Le rationnement vient des prêteurs.
- `FAIT UI` — l'onglet Monnaie affiche revenu annuel, dette/revenu, taux fixe proposé,
  crédit disponible, exposition et marge du créancier. Le tiroir diplomatique cote avant
  clic le montant, le taux, le surplus du prêteur, l'exposition et la cause de blocage.
- `VÉRIFIÉ CIBLÉ` — `credit_demo` **85/85**, `scps_api_demo` **226/226**, compilation de
  `chronicle` sans avertissement, extension Godot debug reconstruite et parse headless
  propre. Aucun sweep n'a été lancé sans accord.

## Risques suivis

- Les panneaux sont majoritairement construits en code et certains sont custom-drawn :
  les zones de clic doivent être reconstruites avec le dessin et rester exactes au resize.
- Une navigation naïve pourrait empiler des panneaux ; `Main` doit rester l’autorité sur
  les exclusivités et la fermeture.
- Les identifiants de certaines ressources/technologies ne sont pas encore garantis sur
  toutes les lectures façade ; les liens incomplets doivent rester absents, jamais devinés.
- Les actions moteur ne doivent jamais entrer dans l’historique comme opérations rejouables.
