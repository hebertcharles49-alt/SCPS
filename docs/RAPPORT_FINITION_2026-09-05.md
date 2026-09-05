# Corrections complémentaires — gameplay et code — 5 septembre 2026

Travail réalisé avec des agents Luna, relu et intégré par l'orchestrateur. Ce rapport complète `RAPPORT_CORRECTIONS_VERBES_2026-09-05.md` ; il ne remplace pas ses preuves ni ne déclare le jeu terminé.

## Corrections

### Fichiers de réglages SCPS_MODS

Les chargeurs économiques, militaires et technologiques convertissaient des champs invalides avec `atof`/`atoi`. Une ligne économique pouvait modifier le prix avant de découvrir un rendement invalide. Ils valident désormais toute la ligne avant son application : nombres finis, champ intégralement reconnu, domaine non négatif et nombre exact de colonnes. Les lignes dépassant le tampon sont consommées et refusées en entier.

Le format historique `price` à trois colonnes reste accepté ; il conserve le rendement existant. Les valeurs nulles admises ne sont pas arbitrairement remplacées par les valeurs vanilla. Un fichier mixte est partagé entre les trois chargeurs : chacun ignore les sections destinées aux autres. Les diagnostics indiquent fichier et ligne.

La review a corrigé un rejet injustifié du format court, des avertissements erronés sur les sections étrangères et un test incohérent avec les valeurs nulles. Le nouveau banc vérifie aussi que toutes les lignes du dump combiné sont relues. Le runner contient désormais 50 bancs, dont ce nouveau contrat.

Limite : validation syntaxique et atomicité ne prouvent pas l'équilibre ou la stabilité numérique de toute combinaison de valeurs extrêmes. Le contenu de SCPS_MODS n'est toujours pas capturé dans l'empreinte des tunables d'une sauvegarde.

### Fondation religieuse

Le prérequis existant Temple/Cathédrale était contrôlé par l'invite automatique, mais contournable par l'action de fondation. La façade le vérifie maintenant avant de créer une foi. Le lecteur prend le bâti des provinces possédées comme référence, au lieu d'un agrégat régional potentiellement périmé.

Le panneau désactive Fonder et explique le bâtiment requis. Le ralliement à une foi existante lorsque le plafond de racines est atteint garde sa branche et sa règle. Aucun coefficient économique ni règle de limite militaire n'est modifié.

Les fixtures distinguent le refus sans Temple, le Sanctuaire seul, le Temple étranger, l'agrégat régional trompeur, le Temple/Cathédrale provincial réel et l'héritage des régions après fondation. Les tests API aval utilisent explicitement une foi de fixture ; ils ne sont pas présentés comme preuve d'une fondation légale.

## Preuves et limites

Les journaux sont conservés dans `runs/finition-2026-09-05/`. Compilation et imports réalisés dans `build/session-review`, avec PATH MSYS2 explicite pour éviter les erreurs de DLL répétées.

| Vérification de cette passe | Résultat | Journal |
|---|---|---|
| Chargement des réglages | 16/16 ; toutes les lignes du dump combiné relues | `moddata-final.log` |
| Production, armée, technologie | 8/8, 50/50, 23/23 | `targeted.log`, `tests/` |
| Contrat joueur, dont Temple | 26/26 | `tests/player_contract_demo.run.log` |
| Grande façade API | 263/263 | `tests/scps_api_demo.run.log` |
| Religion | 16/16 | `tests/religion_demo.run.log` |
| ASan + UBSan, réglages et contrat joueur | 16/16 et 26/26 ; aucun diagnostic des sanitizers | `moddata-san.log`, `player-san.log` |
| Membrane, langue, écritures régionales | Tous verts | `gates-golden.log` |
| Golden | 5 graines × 12 ans identiques à la référence existante | `gates-golden.log` |
| Parcours Godot, dont bouton Fonder et panneau F10 | Sortie 0, pas d'erreur de script | `ui-final.log` |
| Reconstruction, export, démarrage Windows | Sorties 0 | `gdextension-build.log`, `export.log`, `package-smoke.log` |

Le runner comprend 50 bancs ; **sept bancs ont été exécutés sur cette passe**, pas une nouvelle suite complète de 50. Le rapport précédent conserve la preuve de ses 49 bancs, dont la relance API après timeout. Les sanitizers ciblés utilisent `detect_leaks=0` : aucune promesse de couverture complète des fuites. Godot signale toujours le magasin de certificats inaccessible et, lors de l'import Mono, l'absence du SDK .NET 10.0.6 ; ces messages d'environnement ne sont pas des erreurs de script et n'ont pas empêché les tests C++/GDScript ni l'export.

## Livraison Windows

Exécutable : `packaging/windows/dist_godot/session-20260905-finition/scps.exe`, à conserver avec sa DLL voisine. SHA256SUMS.txt et LISEZMOI.txt sont fournis. Les anciens exports sont conservés. La DLL ne dépend que de KERNEL32.dll et msvcrt.dll, sans DLL MinGW externe.

Les DLL du projet ont également été actualisées ; les précédentes sont copiées dans `runs/finition-2026-09-05/previous-project-dll/`. Le manifeste `source-manifest.json` vérifie 229 fichiers source/contrat identiques à la copie utilisée pour construire. Les imports Godot ont été effectués dans cette copie, sans réécrire les traductions compilées ou captures utilisateur du projet d'origine. Format de sauvegarde inchangé : 111.

## Besoins toujours ouverts

L'audit lecture seule du replay montre que le journal d'ordres est transitoire. `--savetest` compare un digest partiel de continuation ; les chroniques golden n'enregistrent pas les actions joueur. Les anciennes formulations « replay byte-identique » ont été corrigées dans CLAUDE.md. Cela précise la preuve disponible, sans supprimer le besoin.

Les besoins ouverts restent détaillés dans `BESOINS_FINITION_2026-09-05.md` : stratégies production/commerce/militaire mesurées sur plusieurs graines, progression jusqu'aux fins, sauvegardes longues avec configuration, replay persistant si retenu comme fonctionnalité, confort réel et performances du rendu. Les changements utilisateur préexistants sont conservés ; aucun commit ni publication.
