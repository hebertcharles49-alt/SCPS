# Soldat, lisibilité et siège en temps de paix — 2026-09-05

## Corrections réalisées

Le clic sur le soldat couvre maintenant le rectangle de son sprite et conserve
une tolérance autour des pieds. La sélection au rectangle détecte aussi le haut
de la silhouette. Les garnisons restent exclues ; les chevauchements choisissent
le corps le plus proche, puis le plus petit identifiant en cas d'égalité exacte.
Le calcul du rectangle est partagé avec le rendu.

Le contenu du panneau Armée reçoit un fond clair local, adapté à ses textes
sombres. Le thème général est conservé. La vignette de formation n'apparaît plus
pendant la marche, le repos ou un siège ; elle reste disponible en bataille.

## Contradiction découverte sur la vraie carte

La scène `army_live_map_test` instancie le vrai écran principal, recrute et lève
un corps par les commandes publiques, choisit une destination accessible puis
transmet l'ordre par MapView. Les captures montrent la progression et l'arrivée.
Ce parcours n'est pas une campagne stratégique complète : il pilote les handlers
de l'interface et contrôle le picking, sans reproduire toute la gestuelle souris.

Avant correction, au jour 19, le panneau affichait **En siège, 579 jours** tandis
que le compteur de guerres restait à zéro. L'assertion d'arrivée au repos échoue
dans `peace-before.log` : un échec. L'audit confirme deux causes distinctes :
arrivée étrangère assimilée à une arrivée ennemie ; butin mensuel sans garde de
guerre. Le prélèvement n'était donc pas seulement une erreur d'affichage.

`campaign_siege_allowed` est désormais partagé entre arrivée, redirection sur
place, siège en cours, débarquement et aperçu. En présence de la diplomatie,
il faut être en guerre contre le propriétaire étranger, ou contre l'occupant de
sa propre terre. Une région déjà tenue par nos forces n'est pas réassiégée.
La paix annule le siège, et le butin mensuel possède sa propre garde de guerre.
La marine reste désactivée.

Les appels bas niveau sans `DiploState` conservent leur compatibilité historique ;
ils ne permettent pas de distinguer paix et guerre. Les chemins joueur et Sim
fournissent cet état. Aucun format de sauvegarde ni réglage économique changé.

## Revue et preuves

**Suite complète : 50 bancs verts sur 51, un délai dépassé.**
`scps_api_demo` a atteint la limite existante de 420 secondes ; aucune erreur
de compilation. Le journal montre une progression dans ses scénarios, mais
ne prouve pas leur achèvement. Une relance isolée, avec la même limite, est
en cours dans `api-isolated.log`. Ce rapport ne déclare pas la suite verte.

- Campagnes : **47/47**, avec arrivée chez un pays neutre, redirection sur place,
  fin de guerre, maintien d'un siège légal, région étrangère déjà occupée et
  libération d'une région nationale occupée.
- Cadence et sauvegarde en marche : **19/19**.
- Mouvement et sélection Godot : **15/15**.
- Panneau Godot : succès, dont vignette masquée hors bataille et visible en bataille.
- Vraie carte après correction : `peace-after.log`, **zéro échec**, arrivée au
  repos et sélection de la silhouette pendant la marche. Captures inspectées.

La revue a rejeté une fixture qui transformait l'absence de cible en réussite,
ainsi qu'un contrôle de libération qui ne faisait pas intervenir la campagne.
Ils ont été remplacés par des scénarios exerçant effectivement le changement.
Les nouveaux gros états de test ont été déplacés hors de la pile après l'échec
initial sans sortie du banc. Le moteur n'a pas été modifié pour contourner ce test.

## Limites et besoins encore ouverts

- Le panneau reste large ; sa mise en page selon la taille de fenêtre n'est pas
  validée. Des libellés de repli comme `Prov.210` restent visibles dans cette fixture.
- Le rendu complet signale à la fermeture des ressources graphiques non libérées
  (CanvasItem, texture et police). Ces diagnostics sont conservés dans
  `peace-after-errors.log`, sans les confondre avec le magasin de certificats Windows.
- La campagne longue, les performances de la cadence quotidienne, l'animation
  articulée du soldat et la validation des stratégies restent ouvertes.
- Les empreintes historiques étaient déjà divergentes après la passe cadence.
  Aucun golden n'a été remplacé et aucun nouveau résultat séculaire n'est revendiqué.

Journaux et captures : `runs/selection-2026-09-05/`. Les rapports précédents
conservent leurs résultats de passe ; ce document décrit la correction suivante.

L'audit `render-lifetime-audit.md` propose un test comparatif scène vide / écran
principal / destruction après rendu. Les caches UIKit et Heraldry sont des
candidats, pas des causes encore établies. Un diagnostic à l'arrêt ne prouve
pas à lui seul une fuite croissante durant la partie.

## Livrable

`packaging/windows/dist_godot/session-20260905-paix/scps.exe` : export réussi,
démarrage headless isolé avec attente de fin, sortie 0. DLL projet, copie de
revue et export identiques ; 241 fichiers de sources vérifiés sans divergence
entre projet et copie de revue (`delivery-paix-manifest.json`). Les anciens
exports et les DLL précédentes sont conservés. Aucun commit ni push.
