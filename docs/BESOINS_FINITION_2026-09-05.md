# Besoins de finition — état vérifié le 5 septembre 2026

Objectif conservé : terminer le jeu, boucher les trous, tester les résultats, énumérer les besoins et démontrer les contradictions. Ce document ne réduit pas cet objectif aux derniers correctifs. **Achèvement global non prouvé.**

## État des exigences

| Besoin | État actuel / preuve | Preuve nécessaire pour fermer |
|---|---|---|
| Chaîne des 59 commandes joueur | Définition, façades, drains et chemins UI audités ; levée ajoutée. Rapport corrections verbes, tests ciblés C/Godot | Scénarios de succès et refus représentatifs de chaque famille, avec coûts, effets et retours cohérents ; la présence des symboles ne suffit pas |
| Règles de fondation religieuse | Corrigé : prérequis provincial contrôlé par la mutation et le panneau ; contrat joueur 26/26, test Godot vert | Contrat ciblé fermé ; parcours d'acquisition du bâtiment durant une campagne normale inclus dans la progression restant à éprouver |
| Réglages | F10/SCPS_TUNE sécurisés, 627 clés classées ; SCPS_MODS strict et atomique, contrat 16/16 et dump complet relu | Chargement fermé sur les cas testés ; mesurer les effets des profils et garder les valeurs extrêmes hors de toute promesse d'équilibre |
| Plusieurs stratégies viables | Anciennes mesures fiscales et une partie graine 7 existent ; elles ne démontrent pas trois voies robustes | Parcours production, commerce et militaire joués sur plusieurs graines, actions/effets mesurés ; échecs expliqués, pas corrigés par une cible mondiale arbitraire |
| Progression complète | Desseins et fins ont des modules/UI/tests ; cela ne prouve pas une campagne complète | Parcours début→objectifs→récompenses→fin, accès et obstacles observés ; expliciter branches effectivement implémentées et absentes |
| Rejeu d'une partie joueur | Journal CMD transient et mutations directes : promesse de replay intégral non prouvée | Journal persistant complet, graine/paramètres/versions, dates d'application, toutes mutations enregistrées, replay indépendant identique ; ou décision de produit explicite sur cette fonctionnalité |
| Sauvegardes | Contrats v111, restauration atomique et injections testés ; empreinte des tunables renforcée | Campagne longue sauvée/rechargée aux étapes sensibles ; vérifier aussi les contenus SCPS_MODS, qui ne sont pas couverts par l'empreinte SCPS_TUNE |
| Confort de jeu | Tests de handlers et géométrie F10, export démarré | Parcours réel des écrans, clavier/souris, états vides, refus successifs, textes FR/EN, tailles de fenêtre et absence d'impasse |
| Performances perçues | Les profils moteur ne mesurent pas le rendu réel | Mesures des temps d'image et pauses sur carte à différents âges ; conserver les empreintes après optimisation |
| Validation séculaire | Golden 5×12 vert ; référence courte uniquement | Reprendre le contrat long et les runs S2 historiques sans nouveau golden opportuniste ; seeds/paramètres/logs complets |
| Documentation | Mise à jour CLAUDE/TROUVAILLES demandée explicitement | Instructions cohérentes avec code et résultats, contradictions exposées, besoins non fermés visibles |

## Contradictions démontrées

### C1 — Nombre et nature des bancs

Avant cette passe, CLAUDE.md indiquait 40 bancs et 8 smoke. `tools/run_tests.sh:BENCHES_FULL` contient 49 cibles à la livraison verbes ; `BENCHES_SMOKE` en contient 7. Le résultat conservé `runs/corrections-verbes-2026-09-05/full-tests.log` compte 48 verts et un timeout, suivi d'`api-final.log` à 261/261. La documentation est rectifiée sans transformer le premier timeout en réussite.

`make determinism` compare deux exécutions ; `make golden` compare à une référence enregistrée. Les appeler tous deux « vs golden » masquait une différence de preuve. Les gates membrane/langue/écritures régionales ont été exécutés séparément dans cette passe : `runs/finition-2026-09-05/gates.log`, sortie 0.

### C2 — Façade religieuse prétendument en lecture seule

`scps/scps_api.c:scps_religion_found`, `scps_religion_schism` et `scps_religion_recruit_scholar` modifient l'état religieux. `godot/project/ui/religion_panel.gd` les appelle. L'ancienne mention « façade read-only » est fausse et a été corrigée. Ces mutations directes ne sont pas magiquement couvertes par le journal CMD.

### C3 — Temple exigé, mais contournable

`scps/scps_api.c:scps_religion_founding_ready` documente explicitement la chaîne tech T2 → Temple bâti → fondation et vérifie le masque Temple/Cathédrale. `main.gd:_on_tick_faith` utilise ce lecteur pour une invite automatique. Cependant `scps_religion_found` ne l'appelle pas et le panneau peut être ouvert par R. Le banc `player_contract_demo.c` fonde actuellement sur son monde initial sans construire de Temple, ce qui fournit également une preuve exécutable du contournement.

Au constat initial, le lecteur lit lui-même `region[].edi_built`, une vue agrégée, alors que la règle du dépôt exige la province comme source du bâti. Correction réalisée : lecture des provinces possédées, garde dans la fondation et explication dans le panneau. Tests de refus sans Temple/Sanctuaire seul/Temple étranger/faux agrégat, puis succès provincial : contrat joueur 26/26. Le constat ci-dessus décrit le défaut avant correction.

### C4 — Registre validé, fichier de contenu permissif

Au début de cette passe, les trois fonctions `econ_moddata_load`, `army_moddata_load`, `tech_moddata_load` utilisent `atof` ; les tiers technologiques utilisent `atoi`. Le chargeur économique écrit le prix avant de traiter le rendement de la même ligne. L'acceptation de F10 ne décrit donc pas la sûreté de tous les chemins d'édition. Correction réalisée : parsing strict et validation complète avant écriture. Contrat 16/16, ASan/UBSan sans diagnostic, tous les enregistrements du dump combiné relus ; formats courts et zéros admissibles conservés.

### C5 — Journal d'ordres et replay

`scps/scps_sim.h` décrit explicitement le feedback comme transient et non sauvegardé. L'API de remise à zéro le purge. Le principe « graine + journal = replay byte-identique » est une exigence possible, pas une fonctionnalité que ces structures démontrent. La preuve de déterminisme headless ne couvre pas automatiquement la saisie/relecture de toutes les actions d'un joueur.

L'audit `runs/finition-2026-09-05/replay-needs.md` précise que `viewer.c` compare un digest partiel après continuation ; il ne compare ni l'état complet, ni des commandes joueur rejouées. Les fichiers de sauvegarde contiennent aussi des métadonnées time/nonce : leur comparaison brute ne constitue pas le bon critère d'équivalence de simulation.

## Incertitudes à garder visibles

- Une stratégie encore peu jouée ne doit pas être déclarée viable sur la seule croissance d'un monde IA.
- Une fin déclenchée par fixture prouve son actionneur, pas son accessibilité ni son intérêt durant une campagne normale.
- Le prérequis Temple est documenté comme une règle réelle : il n'est pas supprimé pour rendre le code existant conforme sur le papier.
- La phase `rule_read` des tunables signifie « à la lecture de la règle », pas « effet garanti au prochain tick » ; les valeurs extrêmes restent à juger par domaine.
- L'objectif « jeu terminé » n'est pas fermé par ce fichier : chaque ligne doit recevoir une preuve à sa propre échelle.
