# Parcours commerciaux et militaires — 2026-09-05

## Livré et vérifié

La fiche diplomatique montre désormais la formation d’une route en jours. Si une route est déjà ouverte et une autre en construction avec le même pays, le détail privilégie le chantier. Le nombre total de routes ouvertes reste inchangé. Les nouveaux champs `route_days_done` et `route_days_total` de `ScpsDiploContext` sont bornés et transmis à Godot ; aucun état de sauvegarde ajouté.

Le test C crée une colonie par les commandes publiques pour disposer d’une seconde région source, ouvre une première route, puis en démarre une seconde. Il vérifie la sélection de cette seconde route et sa progression : **34/34** pour `command_feedback_demo`. Le test Godot crée aussi une route par la commande publique et vérifie le libellé « formation » dans la fiche réelle. Sortie 0, aucune erreur de script ; avertissement habituel du magasin de certificats Windows conservé. Gates membrane/langue/écritures régionales verts.

La première fixture de test supposait une cible connue à deux régions et échouait : 25/31. Elle a été remplacée par la colonisation réelle, pas par la suppression des assertions. Les premiers prototypes des sondes ont également été corrigés en revue avant de retenir leurs résultats.

## Commerce : action et témoin appariés

Six simulations neuves, graines 7/9/11, un an chacune, commandes et lecteurs publics seulement. Un processus par exécution ; SCPS_TUNE et SCPS_MODS absents. Cible : premier pays connu en paix, sans embargo ni liaison existante, avec un endpoint possédé. La politique route crée une liaison terrestre ; le témoin conserve la même cible sans cet ordre. Événements : première option ; temps avancé un jour à la fois.

| Graine | Ouverture | Premier échange observé avec la cible | Valeur échangée avec cette cible au dernier relevé, route / témoin | Exportations joueur au dernier relevé, route / témoin |
|---|---:|---:|---:|---:|
| 7 | jour 90 | aucun sur l’année | 0 / 0 | 0 / 0 |
| 9 | jour 90 | jour 365 | 106,470 / 0 | 15,070 / 0 |
| 11 | jour 90 | jour 365 | 62,091 / 0 | 13,881 / 0 |

Il s’agit des lecteurs du dernier passage commercial, pas d’un profit cumulé ni d’une rentabilité garantie. Sur 7, le rendement de liaison affiché vaut 5,927 mais aucun échange avec la cible ne se réalise. Sur 9, il vaut zéro alors que des biens sont échangés. Le rendement culturel de `routes_advance` et la valeur commerciale d’`intertrade` sont distincts. La sonde retourne un échec de preuve pour la politique route sur 7 ; cela ne constitue pas à lui seul un bug moteur. Elle ne choisit pas encore le partenaire selon prix, surplus, capacité de paiement et distance.

**Contradiction documentaire démontrée** : la vieille règle « deux marchés + pacte » ne décrit plus le commerce régional. `scps_intertrade.c:1085` indique explicitement le commerce régional à la paix, sans Centre obligatoire ; Comptoir/Centre réduisent le coût de transport. `intertrade_active_routes` exclut les routes internes et ne vérifie pas les marges ni le volume effectivement échangé. Le pacte a un effet de garantie différent d’une condition générale d’existence. Les règles du réseau mondial ne doivent pas être appliquées au régional par analogie.

## Militaire : obstacle confirmé, parcours non fermé

La sonde a recruté un paquet de 100 hommes, levé un corps, fabriqué un motif de guerre, déclaré le conflit et envoyé le corps vers une cible terrestre (graine 9). Les autres tentatives de recrutement sont refusées ; le stock militaire initial ne fournit pas une armée de conquête. Ni occupation ni transfert de paix n’ont été obtenus en cinq ans : le scénario retourne 1, sans travestir une commande acceptée en victoire.

La première exécution avait aussi une erreur de sonde : `scps_corps_move_preview(..., NULL, 0)` retourne le nombre de points copiés, donc zéro, même lorsque `out.valid` est vrai. La revue a corrigé l’appelant ; le lecteur moteur n’était pas en faute. Le champ `ScpsDiploOptions.valid` cité par l’audit initial n’existe pas : le retour de fonction fait foi.

**Défaut de cadence démontré** : le corps est en marche au jour 393 avec 8,9 jours restants ; aucun changement de lieu n’est relevé avant le jour 730. À cette clôture, il est de nouveau au repos dans sa région initiale, avec une bataille enregistrée et aucune prise. `sim_campaign_year` (`scps_sim.c:268`) exécute douze pas de `365/12` jours, mais son unique appel est dans le bloc annuel (`scps_sim.c:1836`). Les douze étapes dites mensuelles se déroulent donc toutes à la clôture. Le texte de durée et la possibilité de réagir pendant la campagne ne correspondent pas à l’écoulement quotidien présenté au joueur.

Ce défaut n’est pas corrigé par la livraison actuelle. Il exige de revoir la cadence avec la défense, les interceptions, la récolte d’occupation, la mobilisation et le générateur aléatoire. La correction modifiera potentiellement les chroniques : conserver les différences mesurées, ne pas remplacer silencieusement une référence déterministe. Il reste à distinguer ensuite un revers militaire normal d’un problème de commande ou de recrutement.

## Preuves et limites

Sources et logs : `runs/parcours-2026-09-05/`. Les fichiers `commerce-{7,9,11}-{route,witness}.log` sont les comparaisons retenues. `commerce-seed9.log` est un essai préliminaire. `military-9.log` précède la correction de lecture du chemin ; `military-9-reviewed.log` observe par lots ; `military-9-daily.log` suit les changements quotidiens du corps. Les scripts de revue documentent les corrections apportées aux sondes ; ne pas les réappliquer sur les fichiers finaux.

L’extension a d’abord échoué sous le compilateur Windows choisi implicitement par SCons. Reconstruction réussie avec `use_mingw=yes` et PATH MSYS2 explicite ; aucun correctif moteur motivé par cette erreur d’environnement. Pas de nouvelle campagne séculaire, ni de passage intégral des 50 bancs dans cette passe. La validation porte sur le lecteur ajouté et les parcours décrits.

Export Windows : `packaging/windows/dist_godot/session-20260905-parcours/scps.exe`, garder la DLL adjacente. Save 111 inchangée. Les exports précédents sont conservés.

## Besoins ouverts

1. Corriger et éprouver la cadence militaire réelle ; rejouer recrutement→occupation→paix avec des forces préparées.
2. Mesurer une politique commerciale informée sur plusieurs graines et au-delà d’un an ; conserver un témoin et séparer recettes/coûts/effets culturels.
3. Trancher le contrat alimentaire bétail/compteur, déjà demandé à l’utilisateur.
4. Trancher les branches de Desseins proposées, puis éprouver début→fin.
5. Poursuivre sauvegardes/rejeu, confort et performances selon `docs/BESOINS_FINITION_2026-09-05.md`.

Ces progrès ne ferment pas l’objectif « jeu terminé ».
