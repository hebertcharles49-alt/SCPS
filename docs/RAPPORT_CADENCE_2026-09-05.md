# Rapport cadence militaire — 2026-09-05

## Portée

Cette passe raccorde l'interception navale au temps écoulé et décrit la
transition de la campagne militaire annuelle vers une résolution quotidienne.
Les pertes et le butin économiques restent réglés à la clôture mensuelle ; la
résolution des corps, des marches et des batailles est traitée au jour réel.

## Diff appliqué

Dans `scps/scps_sim.c:387-400`, `sim_campaign_day` appelle désormais
`navy_interception_tick(..., 1.f, ...)` à chaque journée, avant
`campaign_tick`. L'ancien garde-fou `day % 30 == 29`, qui limitait
l'interception à un appel mensuel, a été retiré.

Dans `scps/scps_navy.h:138-148` et `scps/scps_navy.c:673-707`,
`navy_interception_tick` reçoit `dt_days`. Le risque
historique de 45 % par mois est converti par :

`p(dt) = 1 - powf(1 - 0.45, dt / (365/12))`.

Ainsi `dt=365/12` conserve 45 %, tandis que `dt=1` vaut environ 1,946 %. Les
durées non finies ou positives sont protégées (`:674`, `:681-682`). Le garde
`NAVY_COMBAT_ON` ajouté ensuite (`scps/scps_navy.c:683`) conserve la marine
désactivée même si une mission ancienne est présente. Le helper pur
`navy_interception_probability` est partagé avec le banc ; les scénarios navals
historiques lui passent explicitement `365.f/12.f`.

`scps/navy_demo.c:55-88` contient un scénario déterministe : avec le premier tirage
connu 0,06446, un appel à un jour ne déclenche pas la capture, le même état
avec le pas mensuel la déclenche. `dt=0` et `NaN` ne consomment ni RNG ni état.

## Effets et limites de cadence

`campaign_tick` rejoue déjà les jours de bataille dans sa boucle interne, et
l'attrition d'une marche est calculée sur `leg_days`. La conversion ne rend pas
encore les traces groupées et quotidiennes identiques : le paramètre est tronqué
à 30 jours pour 365/12, les compteurs `broken_days` et `rally_days` sont
décrémentés après le lot, et les contacts sont rescannés seulement au début de
l'appel groupé. Ces écarts sont décrits précisément dans
`runs/cadence-2026-09-05/tick-audit.md`.

Le raccord quotidien augmente la fréquence des scans de corps et de paires
hostiles d'environ 30,4 fois. Les tirages navals et le jour d'une interception
peuvent changer ; c'est la conséquence attendue de la conversion du risque,
pas une preuve de rejeu identique.

## Preuves conservées

* `runs/cadence-2026-09-05/full-tests.log` : **51 bancs verts**, constaté avant
  l'ajout du dernier garde `NAVY_COMBAT_ON`.
* `runs/cadence-2026-09-05/navy-off.log` : marine désactivée, **40/40**.
* `runs/cadence-2026-09-05/cadence-final.log` : parcours de cadence,
  **campaign_cadence_demo 12/12**.
* `runs/cadence-2026-09-05/golden-before-update.log` : le golden échoue après
  le changement de cadence avec les hashes modifiés attendus sur les graines
  concernées. Aucune re-baseline n'est déclarée dans ce rapport.

Ces journaux sont des preuves des binaires et passes indiqués.

## Revue finale et soldat sur la carte

La marine reste désactivée (`NAVY_COMBAT_ON=0`). Le garde d'interception respecte
aussi ce réglage pour une mission issue d'une ancienne sauvegarde. Les réglages
économiques et la distinction réserve/campagne ne sont pas modifiés.

Chaque corps actif possède désormais une figurine individuelle 2D. Son pied
avance entre région et prochaine étape selon `progress_pct` du moteur ; sa
direction horizontale suit le déplacement. Sélection et compteur suivent ce point.
Au siège ou au repos, la figurine reste à son emplacement. Il s'agit d'un sprite
déplacé, sans animation articulée des jambes ni modèle 3D.

L'aperçu avant ordre conserve chemin, durée et attrition. Après ordre, la carte
montre le segment engagé et signale la destination finale. **Limite explicite :**
le moteur recalcule le prochain saut à chaque étape et ne mémorise pas une route
complète. La revue a rejeté `[loc,next,dest]`, qui aurait inventé une liaison
directe entre prochaine étape et destination ; `scps_corps_route` renvoie
uniquement `[loc,next]` (ou la position seule au repos). Aucun état sauvegardé ajouté.

Vérifications supplémentaires de l'orchestrateur :

- `route-test.log` : cadence et lecteur de segment **19/19**, dont troncature,
  invalides, lecture pure et sauvegarde/reprise en marche.
- `siege-final.log`, `battle-final.log` : **20/20 états quotidiens concordants**
  après sauvegarde puis rechargement dans une nouvelle simulation, pour chaque phase.
- `determinism-final.log` : deux séries identiques, cinq graines × douze ans.
  Les nouvelles empreintes concordent avec celles du golden divergent ; le golden
  historique est conservé. Les trajectoires anciennes ne sont donc pas garanties.
- `asan-run.log` : graine 9, douze ans, sortie 0, aucun diagnostic ASan/UBSan.
  Cela ne remplace pas une campagne séculaire ni une mesure de performance isolée.
- `army_visual_motion_test-final.log` : **8/8**, progression 0/50/100 %, repli
  temporel, immobilité au siège, publication et purge du segment.
- `army_selection_flow_test-final.log`, `army_panel_test-final.log`,
  `ui_usage_audit_test-final.log` : exécutions réussies sans erreur de script.
- `army-soldier-render.png` : rendu réel Godot inspecté à trois tailles sur fond clair.
  Cette planche vérifie le sprite ; elle ne constitue pas une capture de partie complète.

Les premiers essais Godot ont détecté trois inférences de type invalides dans le
dessin du trajet ; elles ont été corrigées puis les scènes relancées. L'avertissement
Windows de magasin de certificats reste présent, distinct des erreurs de script.

Asset : `godot/project/assets/scps/ui/parch/army_soldier_map_v1.png`, ImageGen intégré.
Prompt de production conservé dans `runs/cadence-2026-09-05/soldier-asset.md`.
Les limites encore ouvertes sont l'animation articulée, la validation visuelle
en partie complète et le coût de la cadence quotidienne sur campagne longue.

## Livrable

Exécutable : `packaging/windows/dist_godot/session-20260905-soldat/scps.exe`.
Export terminé, démarrage headless isolé avec attente de fin : sortie 0
(`export-runtime.log`). DLL du projet mises à jour après copie des versions
précédentes dans `runs/cadence-2026-09-05/before-project-*.dll`.
Les anciens exports restent disponibles. Aucun commit ni push effectué.
