# Progression joueur — état du 5 septembre 2026

Cette passe poursuit l'objectif de finition après les corrections des verbes, du Temple et des réglages. Elle distingue le contenu réellement présent des ambitions de conception et vérifie les passages entre condition, action et récompense.

## Contenu présent et contenu proposé

`scps/scps_missions.h` déclare uniquement `DESS_SOL` puis `DESS_BRANCH_COUNT`. Le panneau Godot expose cette branche. Elle comporte un tronc, un pivot irréversible, puis deux voies : conquête et vassalisation. Huit échelons sont parcourus dans une partie ; les douze slots d'affichage couvrent les deux voies alternatives, pas douze échelons successifs.

Le document `docs/DESIGN_MISSIONS_DOCTRINES.md:71` propose trois branches parallèles par pays : Sol, ouverture géographique et esprit culturel. Le même document se présente comme une proposition à discuter avant code. Le header du moteur précise explicitement que P1 ne livre que le Sol et reporte les six autres branches : Mer & Comptoirs, Routes & Caravanes, Foi, Savoir, Creuset, Horde.

**Conséquence pour la finition :** le Sol implémenté ne prouve pas la progression de toutes les stratégies. Les six branches proposées sont absentes du moteur ; elles ne doivent pas être présentées comme jouables. Leur périmètre final doit suivre les décisions de conception, sans transformer silencieusement une proposition en exigence déjà approuvée.

## Revalidation des conditions au sceau

Constat avant correction : `missions_tick` recalcule `ready` depuis un prédicat d'état réel. Ce statut n'est donc pas un accomplissement acquis à vie. Pourtant `missions_seal` vérifie seulement le cache `ready`, alors que le drain de `CMD_SEAL_DESSEIN` affirme que la revalidation vit dans cette fonction. Une cible peut changer de propriétaire entre la clôture et le clic, et recevoir quand même la récompense.

Reproduction avant correction : `runs/progression-2026-09-05/before.log`, sortie 1, 53 réussis et 3 échecs. Le scénario retire une province entre clôture et sceau, constate la récompense indue, rétablit la possession et vérifie aussi l'absence de double récompense. Après relecture du prédicat avant tout effet : 56/56. La contre-vérification de l'orchestrateur confirme ce résultat, influence 49/49 et banc de retours d'ordres vert ; ASan/UBSan muets sur le banc Desseins (détection des fuites désactivée).

Les preuves historiques d'usage du pivot, explicitement acquises une fois pour toutes, gardent leur sémantique distincte. Aucune cible n'est retouchée, aucune récompense ni durée n'est recalibrée.

## Merveille : entrée dans la course et verdict final

L'audit `runs/progression-2026-09-05/endgame-audit.md` démontre deux chemins à corriger :

1. Le compteur affiché et le tick de chantier reconnaissent les contacts profonds ; l'événement de fondation utilisait un compteur sans `TechState`. Le banc antérieur démarrait directement le chantier et ne prouvait donc pas son accessibilité par cet événement. La correction raccorde le déclencheur au compteur complet ; le test doit choisir la fondation via la file d'événements réelle.
2. La branche finale passait à `MERV_ASCENDED` et effaçait l'empire même si une apocalypse empêchait de publier `FIN_ASCENSION`. Reproduction avec une apocalypse SANG déjà latchée : 120 contrôles verts, deux échecs exactement (`endgame-apocalypse.log`). Un garde avant le verdict final empêche cette ascension silencieuse. La fixture est maintenant exécutée par défaut, sans variable d'environnement.

L'aide `concepts.gd` a également été alignée sur la décision actuelle du moteur : Forge, Société, Savoir, seuils de 3/4/6 héritages intégrés ou en contact profond et ressources rares. Les anciennes exigences arbre complet/assimilation de tout le monde ne sont plus annoncées.

Validation intégrée des raccords Merveille et livraison : en cours. Le fond artistique dédié à la fin chaude reste absent ; l'épilogue possède un voile de secours et son texte. Ce manque d'illustration n'est pas présenté comme un défaut de déclenchement.

## Critères restant à satisfaire

- Parcours normal des huit échelons du Sol sur chaque voie, avec acquisitions réelles et retours compréhensibles ; les fixtures de possession ne prouvent pas leur accessibilité en campagne.
- Objectifs et moyens adaptés aux stratégies marchandes, productives, religieuses et savantes ; périmètre des branches proposées à clarifier au regard des décisions existantes.
- Progression jusqu'aux fins : actions disponibles, conditions effectives, retour joueur, état final et sauvegarde cohérents.
- Mesures de diversité sur plusieurs graines, sans retouche arbitraire des réglages économiques pour faire passer un résultat global.

Les autres besoins restent dans `BESOINS_FINITION_2026-09-05.md`. Le jeu n'est pas déclaré terminé par cette passe.
