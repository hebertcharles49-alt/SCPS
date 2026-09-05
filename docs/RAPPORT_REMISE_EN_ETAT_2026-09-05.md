# Remise en état SCPS — code et gameplay — 5 septembre 2026

**Livraison terminée.** Corrections code et gameplay intégrées, revues et testées ; export Windows construit et lancé. Les réglages économiques et la règle de limite militaire sont conservés. L'équilibre d'une campagne complète reste à démontrer, comme détaillé en fin de rapport.

Jeu livré : [scps.exe](C:/Users/Charl/Desktop/SCPS-main/packaging/windows/dist_godot/session-20260905-validated/scps.exe). Garder l'exécutable et sa DLL dans le même dossier. **Sauvegardes v111 : les anciennes sauvegardes v110 ne se chargent pas dans cette version.** Elles sont préservées.

## Mandat et décisions

L'utilisateur a autorisé les modifications du code et du gameplay proposées après l'audit, exécutées par des agents **gpt-5.6-luna**, avec revue et validation par l'orchestrateur et livraison de ce rapport Markdown.

- Révision de départ : `e55e0b1`. Format de sauvegarde de départ : v110.
- Limite militaire : conserver la règle actuelle (réserve), rendre explicites réserve, corps en campagne et total.
- Économie : conserver les réglages tant que les mesures ne justifient pas un changement ; privilégier l'efficacité des choix et la diversité des stratégies.
- Préserver les fichiers utilisateur préexistants, les sauvegardes et les travaux interrompus dans d'autres worktrees.
- Aucun commit, push, publication ou installation système ne fait partie de cette livraison.

## Suivi de couverture

| Domaine | Travail prévu | État |
|---|---|---|
| Sauvegardes | Validation sémantique, refus transactionnel, ordres transitoires, recherche persistée, chemin utilisateur | Livré ; injections d'E/S et sauvegarde Godot validées |
| Robustesse | Interception navale, coercition provinciale, initialisation atomique des caches | Livré ; bancs et injections d'allocations verts |
| Objectifs joueur | Desseins accessibles, progression, conditions, lieux, choix et scellement | Livré pour Le Sol, branche disponible dans le moteur |
| Découverte | Conseils contextuels et accès aux panneaux sur l'état réel | Livré ; navigation testée |
| Contrat des actions | Attente, exécution, refus ; effets et délais compréhensibles | Livré ; journal C/API/Godot et banc dédié |
| Effectifs | Réserve, corps et total sans changer la règle de limite | Livré ; calcul multi-corps et surcoût testés |
| Validation | Codes retour, journalisation des échecs, complétude des sweeps, parcours UI | Livré ; cas positifs et négatifs vérifiés |
| Distribution | Build identifié, export propre, instructions actuelles, dossier utilisateur | Livré ; lancement headless et graphique |
| Performances | Mesurer le moteur et les pauses de l'interface | Profil moteur réalisé ; cadence graphique non mesurée |
| Équilibrage | Vérifier les leviers sans cible arbitraire de satisfaction | 9 scénarios fiscaux mesurés ; viabilité des stratégies non démontrée |

## Référence avant modifications

L'audit précédent sur cette révision a recompilé les 42 bancs (42 verts ; banc API 258/258), vérifié déterminisme et références à cinq graines sur douze ans, ainsi que membrane/langue/écritures régionales. Des probes supplémentaires ont néanmoins reproduit l'acceptation de données invalides, la perte de cible de recherche et la conservation d'un ordre au chargement.

Ces résultats sont une **référence avant corrections**, pas la validation des changements de cette session.

## Revue, essais et résultats

### Vérifications intermédiaires (avant intégration finale)

- Compilation GDExtension debug réussie dans `build/session-review`, avec une copie locale de godot-cpp : le dépôt de dépendance d'origine est une jonction vers un autre disque.
- Premier lancement du parcours UI : erreur de typage GDScript dans le panneau Desseins, corrigée après revue. Deuxième lancement : code retour 0 et parcours terminé. Une fuite à la sortie restait à revérifier ; les connexions capturantes des nouveaux panneaux ont ensuite été remplacées par des méthodes nommées.
- Revue des nouvelles traductions : champs CSV contenant des virgules non protégées détectés puis corrigés. Le simple import dans un dictionnaire ne suffisait pas à détecter les colonnes excédentaires.
- Mesure de référence moteur, graine 9 sur 20 ans : les blocs instrumentés cumulent environ 10,8 secondes jusqu'à l'an 19. Cette somme n'inclut pas toute la génération ni le rendu et ne constitue pas une mesure du temps de frame. Les événements et l'économie dominent les blocs mesurés.
- Comparaison fiscale via les commandes publiques, sans modification de tunables : réalisée sur les graines 9, 11 et 145. Les scénarios ne contiennent pas d'autres décisions du joueur ; ils mesurent un levier isolé, pas une stratégie optimale.
- Incident de lancement des probes : deux exécutions natives sans l'environnement MinGW ont provoqué des fenêtres Windows pour `libwinpthread-1.dll` absente. Elles ont été arrêtées. L'outil de mesure a été recompilé statiquement ; ses imports ont été vérifiés (`KERNEL32.dll` et `msvcrt.dll` uniquement).

Les journaux sont conservés dans `runs/remise_en_etat_2026-09-05/`. Ces étapes intermédiaires ne remplacent pas les tests de l'arbre intégré final.

### Contre-vérification indépendante des sauvegardes

La probe de revue reproduit les manipulations de l'audit sur la graine 7 : état normal accepté ; nombre de manufactures hors tableau, type d'unité 122 et meneur de fronde hors pays tous refusés. Après sauvegarde puis changement de recherche et mise en file d'un ordre, le chargement rend `rc=0`, restaure la cible 34 et laisse zéro ordre. Résultat : zéro échec (`review-save-probe.log`).

### Mesure du levier fiscal, sans recalibrage

Neuf scénarios, trois graines × curseurs 20/60/100 %, cinq années chacun. Les trois classes imposables reçoivent le même curseur par les commandes publiques. Aucun autre choix joueur n'est effectué. Les pourcentages sont les positions du curseur, pas un taux effectif garanti sur tous les revenus. Données complètes : `economic-measures.csv` (45 relevés annuels).

| Graine | Curseur | Trésor an 5 | Population du joueur | Satisfaction journaliers / bourgeois / nobles |
|---|---:|---:|---:|---|
| 9 | 20 % | 315,5 | 3 828 | 1 / 51 / 0 |
| 9 | 60 % | 661,7 | 3 835 | 1 / 51 / 0 |
| 9 | 100 % | 840,6 | 3 839 | 1 / 33 / 0 |
| 11 | 20 % | 208,7 | 3 838 | 0 / 0 / 0 |
| 11 | 60 % | 418,4 | 3 839 | 0 / 0 / 0 |
| 11 | 100 % | 415,2 | 3 843 | 0 / 0 / 0 |
| 145 | 20 % | 123,2 | 5 031 | 49 / 0 / 0 |
| 145 | 60 % | 132,4 | 5 025 | 49 / 0 / 0 |
| 145 | 100 % | 127,9 | 5 022 | 38 / 0 / 0 |

Le curseur produit bien des effets, variables selon le monde : davantage de trésor sur la graine 9, avec une baisse de satisfaction bourgeoise au maximum ; rendement non monotone sur les graines 11 et 145. Une baisse d'impôt seule ne remplit pas les besoins. Les satisfactions très faibles de plusieurs classes, dans ces scénarios passifs, restent un signal de gameplay à investiguer par des parcours de production et d'approvisionnement. Ces essais ne démontrent ni l'équilibre séculaire ni la viabilité de toutes les stratégies. Conformément à la décision utilisateur, aucun tunable économique n'a été changé.

## Validation finale et preuves

La validation a été faite dans **`build/session-review`**, copie locale du moteur et du projet Godot, avec données utilisateur isolées. Les sources moteur et le binding ont été comparés à l'arbre de travail avant la livraison. Les fichiers utilisateur préexistants ont conservé leurs quatre empreintes SHA-256.

Le premier passage complet a donné 42 bancs existants verts et un échec du nouveau `save_contract_demo` : son dossier parent dépendait d'une préparation manuelle. Ce défaut du banc a été corrigé. Les bancs concernés ont ensuite été recompilés et relancés ; les résultats ci-dessous sont les derniers résultats, **pas l'affirmation d'un unique passage de 46 bancs après la toute dernière retouche**. Le runner complet référence désormais ces 46 bancs.

| Vérification | Résultat final | Preuve sous `runs/remise_en_etat_2026-09-05/` |
|---|---|---|
| 42 bancs existants | Tous verts dans le passage complet | `full-tests.log` ; détails dans la copie de revue |
| API, après les derniers tests militaires | 261/261 | `api-final-tests.log` |
| Commandes | 21/21, dont 320 ordres et dette de classe | `failure-final-tests.log` + journal `command_feedback_demo.run.log` de la copie |
| Caches | 54/54, chaque panne atteinte, six buffers vides, reprise, vrais chemins | `failure-final-tests.log` + `api_cache_demo.run.log` |
| Contrat sauvegarde | Succès : cible, purge, douze ans, chargement dans instance fraîche | `save-final-tests.log` |
| Sauvegardes corrompues et E/S | Succès ; déclenchement prouvé des pannes, état et ordre préservés | `failure-final-tests.log` + `save_failure_demo.run.log` |
| ASan + UBSan moteur | Graine 7, 20 ans, rc0, aucun diagnostic sanitizer | `asan-run.log` |
| ASan + UBSan chargement | Même banc d'injections, rc0, aucun diagnostic sanitizer | `save-asan-run.log` |
| Déterminisme | Deux passages identiques, 5 graines × 12 ans | `determinism-final.log` |
| Golden court | Identique au fichier commité ; aucune re-baseline | `determinism-final.log` |
| Membrane, langue, écritures régionales | Verts | `gates-final.log` |
| Traductions CSV | 243 lignes, 3 colonnes chacune, aucun doublon | Contrôle CSV strict et import Godot |
| Parcours UI | rc0 ; fuite du test corrigée, plus d'erreur de script ni fuite signalée | `ui-final2-console.log` |
| Sauvegarde via Godot | Dossier utilisateur, chargement, graine, remise à zéro UI, slot invalide | `godot-save-final-console.log` |
| Captures UI | 18 captures : 3 panneaux × 2 langues × 100/125/150 %, rc0 | `visual-validated-console.log`, dossier `visual/` |
| Sweep | Succès réel 1×1 an ; rc7 et fin absente refusés ; doublon/paramètres invalides refusés | `sweep-validation.txt` |
| Export Windows | Compilation release et export réussis ; démarrage graphique rc0 | `package-validated2.log`, `package-graphics.log` |
| Contenu exporté | Panneaux présents ; tests, captures ciblées et archives absents | `package-inspect-editor-console.log` |
| Fichiers utilisateur | 4/4 empreintes préservées | `preexisting-final-check.json` |

Empreintes courtes conservées : graine 7 `78f8b0f8`, 108 `3dabef2b`, 209 `a37f6931`, 310 `9d4381aa`, 411 `82ee100c`.

Le banc d'E/S utilise des interceptions de lien réservées au test ; aucun bouton de panne ou branche de test n'est ajouté au moteur de production. Il vérifie notamment le refus d'un snapshot impossible à créer **et** d'un snapshot impossible à écrire, la conservation du fichier existant lors d'une panne d'écriture atomique, puis le traitement de l'ordre resté en attente après un refus.

Les compilations conservent des avertissements préexistants ; il n'est pas revendiqué une compilation sans avertissement. L'éditeur Godot **mono** signale un SDK .NET absent lors des imports, alors que ce projet utilise GDScript et C++. Le jeu exporté utilise le template standard et n'exige pas ce SDK. Le moteur Godot signale aussi un magasin de certificats inaccessible dans cet environnement de test ; ce message subsiste au lancement graphique et n'est pas une erreur de script du jeu. L'inspection du PCK avec l'éditeur seul signale l'absence de sa DLL debug voisine : cette étape vérifie le contenu du pack ; le lancement du paquet release vérifie séparément la DLL livrée.

## Livraison Windows

Utiliser uniquement le dossier **`packaging/windows/dist_godot/session-20260905-validated/`** pour cette version. Les dossiers d'essais précédents de cette session ne sont pas des livraisons validées.

- `scps.exe` : jeu avec ressources embarquées, 258 847 592 octets.
- `libscps.windows.template_release.x86_64.dll` : moteur, 2 848 256 octets.
- `LISEZMOI.txt` : lancement et sauvegardes.
- `MANIFEST-SHA256.txt` : date UTC, Godot 4.6.3, révision `e55e0b1` et état modifié, empreintes des trois fichiers livrés.

SHA-256 du jeu : `6afdfb2394b2838034dd66fea55871c0f6e152b863845ea3d94e9cb17f240f6d`.

Le manifeste indique volontairement **dirty=true** : les corrections ne sont pas commitées. Les empreintes des sources de la session sont aussi conservées dans `source-manifest.json`. L'export a utilisé les templates 4.6.3 présents localement, configurés dans la copie de revue ; aucun chemin machine de template n'a été inscrit dans le preset source partagé.

La DLL livrée importe uniquement `KERNEL32.dll` et `msvcrt.dll` : aucune copie manuelle de `libwinpthread-1.dll` n'est nécessaire. Le script de fabrication a également été corrigé après essai réel pour convertir la destination en chemin absolu et utiliser un dossier temporaire Windows accessible. Aucun ancien export ni aucune sauvegarde utilisateur n'a été supprimé.

## Ce qui change pour le joueur

### Objectifs et découverte

Le panneau **Desseins**, accessible depuis la partie et avec D, expose la branche moteur actuellement disponible, **Le Sol** : échelon, condition, récompense, cible géographique et scellement. Les cibles passent par des identifiants de province, région ou pays et ouvrent la navigation existante. Le bouton distingue une condition non remplie d'un ordre de scellement envoyé. Le panneau s'adapte à la hauteur disponible.

Le panneau **Découvertes** (F) propose des cartes calculées depuis la partie : capitale, recherche disponible, construction, pays connu, Dessein actif, pénurie prévue et budget déficitaire. Les liens vont vers les écrans existants, notamment Stocks avec la ressource sélectionnée et Budget. Trois cartes par page évitent de dépasser la fenêtre. Masquer une carte est un choix d'interface, pas une récompense ou une progression simulée.

### Ordres et armée

Le **Journal des ordres** (O) distingue une commande en attente à la pause, son exécution effective et son refus. Le moteur enregistre un verdict lors du traitement, avec une raison codée, au lieu de confondre l'entrée dans la file et le succès de l'action. Les 60 valeurs du catalogue sont nommées ; les lieux sont résolus via la façade et les postes budgétaires ont des noms lisibles. Une action sans effet, une recherche inaccessible ou une dépense impossible ne doivent plus être annoncées comme réussies.

Le journal moteur est borné à 128 entrées ; le panneau montre les huit dernières. Les ordres encore en attente sont protégés lors de l'éviction de l'historique. Ce journal est transitoire et se vide après un chargement réussi.

La section militaire affiche **réserve, corps en campagne et total**, ainsi que la limite et le surcoût de réserve. Tous les corps actifs sont additionnés, sans compter deux fois le corps principal. La règle choisie est conservée : le dépassement est calculé sur les régiments de réserve ; les corps paient déjà leur entretien. Le calcul du surcoût partage désormais la formule de solde du moteur : à deux fois la limite, le réglage courant donne un multiplicateur ×4, donc +300 %.

## Ce qui change dans le code

### Sauvegardes

Le format passe de **v110 à v111** pour conserver la cible de recherche. Les anciennes versions sont refusées explicitement ; aucune migration silencieuse et aucune modification des anciennes sauvegardes n'ont été effectuées.

La validation rejette davantage de données impossibles avant leur utilisation : compteurs de pays, liens géographiques, types et quantités d'unités, bâtiments, états de campagne et relations diplomatiques. Les liens de cellules sont vérifiés avant de reconstruire l'adjacence. Les bornes ont été confrontées aux valeurs réellement produites par le moteur : une rancune ou un score de fronde ne sont pas arbitrairement ramenés à une valeur normalisée.

Le chargement exige un snapshot de secours complet avant de toucher à la partie vivante. Un refus restaure l'état précédent et garde ses ordres ; un succès purge les ordres de l'ancienne session et restaure la cible de recherche et la graine. Un échec de restauration est distingué d'un simple fichier refusé, pour empêcher la reprise d'une instance incertaine.

Le chemin de sauvegarde est configurable par la façade. Godot utilise son dossier utilisateur `user://saves`, au lieu de dépendre du répertoire d'installation. Les délais et informations temporaires de l'interface sont remis en cohérence après un chargement réussi.

### Robustesse et contrôles

- Interception navale : les valeurs de probabilité et de pertes sont initialisées, y compris pour un transport sans escorte. La règle d'interception n'est pas recalibrée.
- Coercition : l'effet de vassalisation atteint les provinces actives de la région capitale, qui portent l'état économique réel.
- Caches : les centroïdes et les six buffers A* sont publiés seulement lorsque toutes leurs allocations ont réussi. Un échec laisse un état inutilisable mais cohérent, avec possibilité de réessayer.
- Runner : un bilan textuel vert ne masque plus un code retour non nul ; les journaux de compilation et d'exécution sont distincts.
- Gates : une erreur du préprocesseur ou du moteur de recherche est bloquante. Le contrôle d'écriture régionale reconnaît les index imbriqués.
- Sweep : suivi des processus, codes retour, marqueurs de fin et empreintes des journaux. Les tests bloquent la publication ; les diagnostics restent récupérables en cas d'échec. Aucun workflow distant n'a été lancé.
- Distribution : moteur Godot attendu documenté, chemin configurable, destination neuve, exclusion des tests/captures/archives et manifeste de fichiers. L'ancien dossier de distribution n'est pas supprimé.

Les points fiscaux et certains effets visuels cités dans l'ancien audit étaient déjà corrigés dans la révision de départ. Ils n'ont pas fait l'objet d'un second correctif artificiel. Les travaux partiels d'équilibrage et de performance présents dans d'autres worktrees ont été préservés.

## Ce qui manque encore pour un jeu potable

**Cette livraison rend les décisions plus lisibles et la base plus fiable ; elle ne démontre pas encore qu'une campagne entière est intéressante et équilibrée.** Les priorités restantes sont concrètes :

1. **Prouver plusieurs stratégies viables.** Jouer et mesurer au moins un parcours de production, un parcours commercial et un parcours militaire, sur plusieurs graines. Les scénarios fiscaux montrent que les besoins restent très mal satisfaits dans certains départs passifs. Il faut identifier les choix qui corrigent réellement ces situations avant de modifier les réglages.
2. **Éprouver la progression sur une campagne.** L'interface expose Le Sol ; les autres branches envisagées ne deviennent pas implémentées du seul fait d'avoir un écran. Tester l'enchaînement des objectifs, leurs récompenses et l'accès aux fins dans des parties longues reste nécessaire.
3. **Faire une passe de confort en situation réelle.** Vérifier les nouveaux conseils et le journal avec des ordres militaires, achats, diplomatie et refus successifs, ainsi que l'accessibilité des textes et des raccourcis dans les écrans principaux. Les captures et tests automatisés couvrent des parcours ciblés, pas toute l'expérience utilisateur.
4. **Mesurer les pauses perceptibles.** Le profil moteur désigne les événements et l'économie comme gros postes ; il ne mesure pas les images par seconde ni les à-coups de la carte. Optimiser seulement après une mesure en partie, avec maintien des empreintes de simulation.
5. **Reprendre la validation séculaire existante.** Les références à douze ans restent le contrat court. Le travail historique sur les trajectoires longues et la cadence S2 n'est pas remplacé par un nouveau golden choisi pour faire passer un test.

Les décisions économiques et militaires confirmées priment sur les anciennes propositions de recalibrage. Aucun changement de satisfaction cible, de taux démographique ou de limite de force totale n'est introduit ici.
