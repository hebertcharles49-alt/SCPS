# Campagnes joueur et retours d’action — 2026-09-05

## Résultat de cette passe

Trois comparaisons appariées de cinq ans ont été exécutées par l’API publique, sans modification des tunables ni injection de ressources. Les états initiaux sont identiques dans chaque paire. Construire n’est pas une stratégie gagnante partout : les résultats dépendent fortement du monde et de la politique choisie. Aucun rééquilibrage n’est justifié par ce seul pilote.

## Gameplay : protocole et mesures

Un processus indépendant par graine et politique ; graines 7, 9 et 11 ; 1 825 jours, avancés un par un. Le témoin répond aux événements avec la première option. La politique production fait de même et recherche chaque mois une construction légale, en privilégiant une sortie déficitaire puis sa couverture de stock. En absence de déficit, elle peut construire une manufacture non déficitaire. Ce repli est une limite importante : la politique n’optimise ni rentabilité, ni recherche, ni toute la chaîne d’intrants.

| Graine | Politique | Population finale | Trésorerie / IPM | Solde mensuel | Manufactures affichées | Ouvriers affichés |
|---|---|---:|---:|---:|---:|---:|
| 7 | Témoin | 5 159 | 669,47 | −5,60 | 0 | 0 |
| 7 | Production | 5 435 | 647,87 | −11,43 | 8 | 1 085 |
| 9 | Témoin | 3 839 | 798,08 | −9,68 | 0 | 0 |
| 9 | Production | 3 791 | 597,73 | −5,29 | 10 | 48 |
| 11 | Témoin | 3 843 | 415,16 | −17,17 | 0 | 0 |
| 11 | Production | 3 839 | 449,96 | −13,44 | 1 | 0 |

Les 19 ordres de construction recensés sont acceptés ; aucun refus de feedback dans ces six runs. IPM final = 1 dans les six cas. Sur 7, population +5,3 %, mais solde mensuel dégradé. Sur 9, solde moins négatif, mais trésorerie et population plus faibles. Sur 11, une manufacture sans ouvriers ne démontre aucun gain de production ; la différence de trésorerie ne suffit pas à établir sa rentabilité.

La graine 11 présente aussi du bétail abondant et des céréales presque absentes. Contradiction confirmée : `scps_country_food` (`scps_api.c:4561`) inclut le bétail comme rations disponibles ; la table `NEED` (`scps_econ.c:562`) ne le demande jamais. Le remplacement des rations manquantes (`scps_econ.c:5342`) utilise uniquement les fruits. Le bétail accumulé ne peut donc relever `food_sat` par cette consommation. `food_runway` l'agrège également. Cela prouve un décalage de contrat ; cela ne prouve pas que toutes les difficultés de la graine viennent de lui. Choix demandé : corriger les lecteurs selon la consommation actuelle, ou rendre le bétail consommable (changement de règle à mesurer). Les satisfactions de classe affichées ne distinguent pas à elles seules une classe absente d’une classe insatisfaite.

Preuves : `runs/campagnes-2026-09-05/seed*-*.log`, `comparison.csv`, `comparison.json` (empreintes des logs), `player_campaign_probe.c` et son protocole `.md`. Le mode commerce de la sonde refuse explicitement de tourner : il n’est pas une stratégie commerciale validée. Les revenus physiques de biens ne doivent pas être additionnés comme des recettes monétaires.

## Code : corrections revues

- `scps_sim.c`, traitement CMD_ROUTE : une route acceptée annonce STARTED, car son ouverture est différée. Le banc vérifie le résultat au premier drain et son absence du compteur de commerce actif. Une route créée sans les conditions commerciales n’est pas une preuve de rendement positif.
- `army_panel.gd` : la levée annonce un ordre transmis ; elle ne déclare plus un corps créé sur le seul booléen d’enfilement. Les préconditions locales et le refus d’enfilement sont explicités.
- `sidebar_drawer.gd` : le recrutement annonce la transmission. Un refus n’est plus attribué systématiquement à une file pleine sans preuve.

Les agents Luna ont livré ces corrections ciblées ; l’orchestrateur a revu les diffs, intégré et contre-vérifié. La sonde exploratoire a été reprise par l’orchestrateur pour remplacer une construction ponctuelle par une politique mensuelle mesurée. Aucun format de sauvegarde modifié (111).

## Validation réalisée

- `command_feedback_demo` : 25/25, dont quatre contrôles sur la route différée.
- Construction de l’extension Godot : sortie 0.
- `ui_usage_audit_test` : sortie 0 ; aucune erreur de script. Le parcours existant ne constitue pas un test interactif exhaustif des deux nouveaux messages militaires.
- Membrane, langue, écritures régionales : trois gates verts.
- Six campagnes : sortie 0 et année 5/jour 0 ; égalité des états initiaux appariés vérifiée automatiquement.
- Export Windows et démarrage headless : sortie 0. Avertissement environnemental conservé : lecture du magasin de certificats Windows impossible. Aucun nouveau passage global des 50 bancs ni nouvelle validation séculaire dans cette passe.

Export : `packaging/windows/dist_godot/session-20260905-campagnes/scps.exe`, avec sa DLL adjacente. Les anciennes DLL projet sont conservées dans `runs/campagnes-2026-09-05/previous-project-dll/`.

## Besoins encore ouverts

1. Expliquer les chaînes alimentaires et manufactures inactives avant d’ajuster l’économie.
2. Mesurer une politique production mieux informée et une vraie chaîne commerciale : marchés, accord effectif, ouverture, rendement positif et coût amorti.
3. Jouer le parcours militaire entier : recrutement, corps, motif de guerre, conflit, occupation et paix territoriale. Les fonctions présentes ne suffisent pas comme preuve.
4. Choisir le périmètre des Desseins : seul le Sol est livré ; six branches restent proposées. Question adressée à l’utilisateur, sans choix implicite.
5. Conserver les besoins de campagnes longues, progression vers les fins, sauvegardes/rejeu, confort et performances dans `docs/BESOINS_FINITION_2026-09-05.md`.

Le jeu n’est pas déclaré terminé. Les rapports précédents restent applicables : corrections des verbes, finition et progression, datés du même jour.
