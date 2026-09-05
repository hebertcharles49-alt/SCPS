# Rapport — corrections de la chaîne verbes, actions et réglages

5 septembre 2026 · SCPS · Agents Luna, intégration et revue par l’orchestrateur.

## Résultat livré

Les ruptures prioritaires de l’audit sont corrigées. Les **59 commandes CMD ont désormais un chemin d’interface identifié**, y compris la levée. Les taux de rachat ont leur contrôle économique ; les mutations religieuses directes ont des validations de propriété et d’éligibilité côté moteur.

Le registre passe de **618 à 627 clés**, sans modifier les valeurs d’équilibrage existantes : cinq paramètres de toponymie et quatre paramètres religieux sont ajoutés avec leurs anciennes valeurs de code. Les cinq clés historiques sans actionneur sont explicitement désactivées et refusées à l’écriture. Elles ne sont plus présentées comme des leviers utilisables.

**Version jouable :** `packaging/windows/dist_godot/session-20260905-verbes/scps.exe`, à conserver avec la DLL du même dossier. L’ancienne version exportée reste disponible. La bibliothèque de `godot/project/bin` a également été actualisée ; ses deux anciennes DLL sont conservées sous `runs/corrections-verbes-2026-09-05/previous-project-dll`.

Aucune recalibration de satisfaction ou de croissance n’a été imposée. La limite militaire reste fondée sur la réserve, avec réserve, campagne, total et surcoût distincts. Aucun commit ni publication externe.

## Gameplay : ce qui change pour le joueur

| Domaine | Avant | Après |
|---|---|---|
| Levée | Commande C/Godot présente, sans contrôle actif retrouvé | Quatre niveaux 0–3 dans Armée ; ordre en attente puis état confirmé au tick |
| Rachats | Taux C existants mais inaccessibles dans Godot | Trois curseurs : vivrier, brutes, manufacturés ; relecture du taux accepté ; mention inactive si BUY_RATE_ON est désactivé |
| Fondation religieuse | Appel direct pouvant agir sur un autre pays ou refonder | Pays humain uniquement, foi existante protégée ; validation du crédo et des traditions lors d’une fondation |
| Ralliement | UI autorisait l’adoption au plafond sans traditions valides, façade les exigeait | Adoption cohérente avec l’UI : les choix de création ne bloquent pas le ralliement |
| Schisme | Précondition du lecteur non reprise dans la mutation | Même éligibilité et même plafond côté mutation ; refus sans création de religion |
| Lettré | Région d’action non contrôlée par la façade | Région valide appartenant au pays humain ; mode observateur interdit |
| Raid côtier | Exécution positive sans quantité explicite dans le journal | Butin affiché, y compris zéro, et délai de récupération lorsqu’il est disponible |
| Rénovation | Paiement possible avant refus d’une file pleine | Capacité de file validée avant paiement |
| Embarquement | Fournitures consommables avant refus géographique | Géographie, port, transport et stocks validés avant mutation ; débit après le dernier point de refus |
| Construction navale | Kit partiel accepté car le débit borné était ignoré | Kit national complet exigé : fournitures navales, bois et cuivre si requis |

Les corrections navales peuvent modifier les longues parties : un chantier auparavant accepté en pénurie est désormais réellement refusé. C’est une correction de règle, même si les coefficients restent identiques. Les cinq graines de référence sur douze ans conservent leurs empreintes.

Les actions religieuses et le taux de rachat restent des **mutations directes** avec retour/relecture immédiate. Elles n’ont pas été artificiellement ajoutées à la file CMD, ce qui aurait changé leurs signatures et leur temporalité. Leur validation est désormais côté moteur ; l’interface explique les refus religieux.

## Réglages : ce qui est présent, défini et modifiable

### Contrat de modification

`tune_set_checked` retourne accepté/refusé. Les noms inconnus, les valeurs non finies et les clés inactives sont refusés sans mutation. Les coûts/prix/durées négatifs sont refusés selon les familles nommées du registre ; les paramètres religieux et les seuils TIER ont aussi des bornes spécifiques.

La façade vérifie également qu’un nombre Godot peut être représenté par le flottant du moteur. L’ancienne fonction `tune_set` reste disponible pour les appelants existants et passe par la validation commune.

F10 refuse le texte invalide au lieu de le convertir en zéro. Il affiche la valeur réellement relue, la phase d’application et un bouton **Défaut** pour retirer une surcharge. **Copier les réglages** produit une ligne SCPS_TUNE contenant les surcharges actives. Le panneau est centré après calcul des dimensions du texte et garde le registre dans une zone défilante.

### Phases exposées

| Phase du registre | Nombre | Sens |
|---|---:|---|
| rule_read | 596 | Application lorsque la règle consommatrice relit le paramètre ; pas une promesse universelle au prochain tick |
| new_world | 23 | Paramètres de genèse/toponymie identifiés : nouveau monde nécessaire |
| next_action | 1 | Durée du prochain recrutement de lettré |
| diagnostic | 2 | Télémétrie chronicle, sans effet de gameplay |
| inactive | 5 | Anciennes clés conservées pour identification, sans actionneur ; modification refusée |

Ces phases classent les comportements vérifiés. Elles ne remplacent pas une spécification individuelle des unités et des plages pertinentes de tous les coefficients du moteur. Les bornes de domaine restent également dans leurs helpers ; « nombre fini accepté » ne garantit pas un équilibrage raisonnable pour n’importe quelle valeur extrême.

Les six seuils `TIER2_POP` à `TIER7_POP` ne restent plus figés après leur première lecture : le cache suit une révision du registre. L’effet d’une modification est vérifié par le banc C.

### Clés ajoutées, à valeurs inchangées

| Clé | Défaut | Application |
|---|---:|---|
| TOPONYM_RIVER_MAJOR | 160 | Genèse des noms |
| TOPONYM_HARBOR_HIGH | 0,5 | Genèse des noms |
| TOPONYM_ISLAND_MAX_AREA | 700 | Genèse des noms |
| TOPONYM_ETHOS_REINFORCED | 0,8 | Genèse des noms |
| TOPONYM_ETHOS_BASE | 0,25 | Genèse des noms |
| RELIG_SCHISM_FLIP_D | 5 | Seuil de distance culturelle, borné 0–10 |
| RELIG_SCHISM_FLIP_L | 4 | Seuil de légitimité, borné 0–10 |
| RELIG_SCHOLAR_DAYS | 1825 | Durée du prochain mandat ; 1–365000 jours |
| RELIG_SCHISM_MAX | 5 | Plafond de schismes par racine ; 0–64 |

Une baisse de durée des lettrés ne supprime pas un mandat déjà enregistré dans une sauvegarde : le timer existant reste chargé.

Clés désormais explicitement inactives : `REGION_RAW_KEEP`, `SPAWN_FOOD_RAW`, `NAVY_BUILD_SUPPLY_FLOOR`, `AI_COMPLEMENT_W`, `WILD_REGIMENTS`. Les réactiver demanderait de définir puis raccorder leur règle ; nous n’avons pas inventé un nouvel effet pour faire disparaître une alerte d’audit.

`INVARIANT_DRIFT_FRAC` et `INVARIANT_SCALE_FLOOR` sont identifiés comme diagnostics. Le registre corrigé et ses références sont livrés dans `docs/TUNABLES_CORRIGES_2026-09-05.csv`.

### Définitions harmonisées

Les 27 clés dont les valeurs de secours divergeaient du registre ont été harmonisées sur les valeurs déjà actives. Le nouvel inventaire prétraite les 44 modules : **aucune lecture de clé absente du registre et aucune divergence numérique de fallback détectée**, contre cinq clés absentes et 37 occurrences divergentes avant correction.

Les commentaires des commandes de marché servile et de transfert de population indiquent désormais leur vrai grain : province PID, et non région.

## Sauvegardes et reproductibilité

L’empreinte ne repose plus sur les 1023 premiers caractères de la description des surcharges. Elle intègre tous les noms et les bits exacts des valeurs effectives. Deux flottants voisins et une clé placée à la fin du registre produisent des empreintes différentes dans les tests.

Le texte SCPS_TUNE n’est plus tronqué silencieusement à l’entrée. Un profil de plus de 6000 caractères est relu jusqu’à sa dernière valeur dans le test d’environnement. La chaîne descriptive des surcharges est allouée à sa taille réelle.

Le format de sauvegarde reste **111** : aucune structure sérialisée n’a été agrandie pour ces corrections. Une sauvegarde 111 portant l’ancienne empreinte reste lisible ; un avertissement indique que cette ancienne représentation ne permet pas de certifier toute la configuration. Les réglages ne sont pas restaurés depuis la sauvegarde : l’empreinte détecte une différence, elle n’est pas un profil de configuration embarqué.

L’empreinte concerne le registre SCPS_TUNE. Elle ne constitue pas un contrôle de tout fichier externe SCPS_MODS ni de tout contenu compilé.

## Vérifications réalisées

| Vérification | Résultat | Preuve sous runs/corrections-verbes-2026-09-05 |
|---|---|---|
| Suite générale | 48 bancs verts, API interrompue à 240 s ; relance API seule réussie, soit 49 bancs validés | full-tests.log, api-final.log |
| Façade API | 261 réussis, 0 échoué | api-final.log |
| Atomicité des actions | 22/22 | atomicity-review.log |
| Propriété/éligibilité religieuse et rachats | 19/19 | player-contract.log |
| Registre, cache, reset, empreinte | 650/650 | tests/tune_contract_demo.run.log |
| Entrée SCPS_TUNE | 7/7 : NaN, infini, coût négatif, inconnu, inactif, profil long, toponymie | environment-contract.json |
| ASan + UBSan | Atomicité 22/22 et contrat joueur 19/19, sans diagnostic | atomicity-asan.log, player-asan.log |
| Déterminisme de référence | 5 graines × 12 ans identiques au golden existant | golden.log |
| Interface Godot | Levée, F10, reset, phases, rachats, refus religieux, navigation ; sortie 0 | ui-geometry-final.log |
| Géométrie F10 | Cadre centré et contenu dans le viewport de test | ui-geometry-final.log |
| Compilation GDExtension | Réussie, runtimes GCC liés statiquement | gdextension-package-build.log |
| Export Windows et démarrage | Export réussi ; démarrage isolé, sortie 0 | export.log, package-smoke.log |
| Analyse statique finale | 627 clés, 59 traitements CMD, 0 clé lue absente, 0 fallback divergent | static-summary.json, inventory-final.log |

Les tests Godot ouvrent les panneaux et appellent les handlers reliés aux contrôles, puis vérifient leurs effets ; ils ne constituent pas une exploration manuelle de toutes les branches du jeu. Aucun screenshot de validation visuelle n’est revendiqué.

Les outils mémoire ont été exécutés avec arrêt sur erreur ; la détection des fuites ASan était désactivée dans cet environnement Windows. Un avertissement de magasin de certificats Windows reste présent au lancement Godot. L’importeur Mono signale également l’absence du SDK .NET ; le projet GDScript/C++ testé et exporté fonctionne sans ce SDK. Aucun échec de script ni de chargement de DLL dans le lancement validé.

## Revue et limites conservées

Trois agents Luna ont traité respectivement l’atomicité, le registre/sauvegarde et l’interface. La revue a demandé puis vérifié plusieurs corrections supplémentaires : empreinte sur bits exacts plutôt que texte arrondi, cache révisable, distinction diagnostic/genèse, contrôle de schisme côté façade, relecture des setters sans retour, centrage F10 et nettoyage audio du banc headless.

Les grosses fixtures du banc d’atomicité sont sorties de la pile pour fonctionner avec la pile Windows normale. Les trois nouveaux bancs sont intégrés au Makefile et au runner général.

Il reste volontairement plusieurs modes d’édition : paramètres joueur, registre F10/SCPS_TUNE, fichiers SCPS_MODS pour les champs déjà pris en charge, et code pour les tables/règles structurelles. Les recettes, identifiants et toutes les combinaisons de contenu n’ont pas été transformés en centaines de nouveaux curseurs. Le présent lot corrige les ruptures identifiées ; il ne prétend pas que toute constante de tout module est devenue un réglage live.

Les fichiers utilisateur préexistants (deux traductions binaires et deux captures) sont contrôlés par empreinte et préservés. Les imports/tests se sont déroulés dans `build/session-review`, les sauvegardes de tests dans ses dossiers isolés. Les anciens exports restent intacts.

## Fichiers de référence

- Rapport d’audit avant correction : `docs/AUDIT_CHAINE_VERBES_2026-09-05.md` (matrices détaillées des 59 verbes).
- Inventaire corrigé : `docs/TUNABLES_CORRIGES_2026-09-05.csv`.
- Diff de revue du lot : `runs/corrections-verbes-2026-09-05/review.diff` ; il complète les nouveaux bancs et les mises à jour de traduction/test.
- Rapports des agents : `actions.md`, `tuning.md`, `ui.md` dans le dossier de preuves.
- Paquet jouable et manifeste : `packaging/windows/dist_godot/session-20260905-verbes`.
