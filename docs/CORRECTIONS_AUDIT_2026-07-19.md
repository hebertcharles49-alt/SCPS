# Corrections de l’audit complet — 2026-07-19

Statut : **CORRECTIONS MOTEUR TERMINÉES — VALIDATION CIBLÉE VERTE**.

Règle d'exécution actée : aucun gigasweep sans accord explicite. Un travail de fond
est plafonné à **10 simulations**. Le correctif naval final n'a donc pas été suivi
d'un nouveau balayage statistique ; sa preuve actuelle est un banc moteur ciblé.

## Plan suivi

- [x] Rendre les dépenses à crédit atomiques et physiquement finançables.
- [x] Supprimer les transferts diplomatiques partiels/gratuits.
- [x] Migrer le banc Brèche vers le déclencheur réellement en production.
- [x] Rendre les gardiens Windows reproductibles (`tmp`, Clang ASan/UBSan).
- [x] Exécuter bancs, déterminisme court/profond, golden, fuzz-save et Chronicle.
- [x] Corriger les défauts supplémentaires révélés par le sanitizer.
- [x] Exécuter un gigasweep frais de 100 mondes distincts sur 250 ans.
- [x] Corriger la création monétaire du commerce régional révélée par M3c.
- [x] Rejouer les graines 912 et 1114, puis le gigasweep complet autorisé à ce moment.
- [x] Rendre les blocus et interceptions navales physiquement actifs.
- [x] Repasser les 40 bancs après le dernier correctif.
- [ ] Mesurer la fréquence navale finale sur plus de 10 mondes — **attend un accord explicite**.

## Corrections moteur

### Crédit

- `credit_can_spend` vérifie maintenant la capacité **physique** séquentielle : trésor
  national, classes, puis prêteur étranger, avec plafond et markup recalculés entre les
  étages.
- `credit_spend` retourne un booléen et applique un contrat **tout ou rien**. Un appel
  non finançable ne prélève rien et ne produit aucun effet chez l’appelant.
- La péréquation entre provinces n’est plus comptée comme financement d’un déficit
  national : déplacer des pièces dans le même pays ne change pas son solde net.
- Un journal de transaction restaure trésors, richesses et dette si une divergence
  future apparaît entre préflight et exécution.
- Les écritures de richesse des créanciers tiennent immédiatement les vues province et
  région en phase.
- Tous les appelants gameplay vérifient le résultat de la dépense ; le préflight et
  l’exécution partagent les mêmes capacités, sans mutation financière intermédiaire.

### Diplomatie

- La fabrication de casus belli prélève le prix sur **toutes** les régions du pays,
  puisque son gate lit l’or national.
- Les élites adverses ne reçoivent que le montant intégral effectivement payé ; une
  incohérence éventuelle provoque une restitution atomique.
- Les dons de suzerain transfèrent désormais le débit réel, jamais un nominal supérieur
  aux fonds disponibles.

### Événements et Chronicle

- Le banc Brèche reflète la règle actuelle : la charge seule ne déclenche plus l’âge ;
  une transition vers une fin entropique appelle `ages_breach_fire`.
- ASan a révélé deux lectures `snap[-1]` dans Chronicle au premier instantané. Elles
  utilisent maintenant le snapshot courant.
- Le rapport monétaire ne nomme plus les flux `FX_*` « création/destruction » : ils
  incluent des transferts et restent un recoupement incomplet. Le véritable compteur est
  présenté séparément comme résidu de périmètre.

### Commerce régional

- Le règlement d'un flux calcule maintenant la capacité de paiement avec la population
  et la richesse de **l'importateur**, jamais celles de l'exportateur.
- Le volume est borné par ce que l'acheteur peut effectivement financer.
- Le débit de l'importateur précède le crédit de l'exportateur ; le même montant réel
  `paid` alimente le vendeur et `flow.value`. Il n'existe plus de branche où les biens et
  l'or sont crédités avant encaissement.
- `trade_demo` ajoute huit contrôles, notamment l'exportateur sans population et
  l'acheteur trop pauvre.

### Marine

- L'interception a été replacée dans la boucle mensuelle de campagne, au contact des
  phases de convois, au lieu d'être appelée avant leur création annuelle.
- Une flotte en `NAVY_BLOCUS` chasse bien les convois de **sa cible**, comme une flotte
  en `NAVY_INTERCEPTION` ; l'éthos Ordre arme désormais réellement la patrouille annoncée
  par sa doctrine.
- Le compteur de noyés lit le corps intercepté par son identifiant exact. Un corps
  secondaire ne réutilise plus par erreur la composition du corps principal.
- Le blocus ne transforme plus la mer en mur booléen : `campaign_order_sea` et
  `campaign_redirect_corps_sea` autorisent le départ si le port, la côte, le chemin et
  les transports existent. La flotte ennemie résout ensuite le risque physiquement.
- Les phases `FA_EMBARK` **et** `FA_SAIL` sont interceptables. C'est indispensable pour
  les traversées courtes, qui peuvent passer embarquement → mer → débarquement dans un
  seul tick mensuel.
- La façade conserve `blocked=1` comme information de danger, mais `possible` ne devient
  plus faux à cause du seul blocus.

## Gardes et portabilité

- Les logs de bancs et `fuzz-save` utilisent `build/tmp`, sans dépendre d’un `/tmp`
  inscriptible.
- Le target `asan` choisit la toolchain Clang64 quand elle existe, lie avec `-pthread`
  et le target `full-test` charge son runtime Windows.
- Toolchain installée localement dans MSYS2 pour cette machine : Clang 22 + compiler-rt.

## Preuves exécutées

- `make test` après le dernier correctif : **40/40 bancs verts**.
- `navy_demo` : **32/32**, dont départ sous blocus, interception pendant
  l'embarquement et réembarquement d'un corps secondaire.
- `trade_demo` : **8/8**.
- Validation antérieure de la même série : déterminisme court et profond stable,
  golden conforme, ASan+UBSan muets sur 20 ans.
- `credit_demo` : **82/82**.
- `diplo_demo` : **93/93**.
- `events_demo` : **119/119**.
- `make fuzz-save` : **8/8**, 216 octets mutés, aucun crash.
- `make determinism-deep` : graines 7 et 9 stables sur 200 ans, deux runs chacune.
- Chronicle réelle : `./chronicle 9 1 250 6 12`, terminée sans incident.

## Baseline

Les hashes à 12 ans rebaselinés après l'ensemble des corrections sont :

- graine 7 : `4eca2ab4`
- graine 108 : `12193194`
- graine 209 : `a104ee36`
- graine 310 : `7b245f79`
- graine 411 : `644ff959`

Deux exécutions par graine ont produit les mêmes valeurs avant mise à jour du golden.

## Limite explicitement conservée

L’invariant M3f est un détecteur annuel de régression sur un périmètre instrumenté, pas
une preuve de conservation exhaustive. Chronicle le disait déjà dans le code ; sa sortie
est désormais libellée sans ambiguïté. Étendre le registre à chaque transfert historique
est un chantier comptable distinct, pas une correction sûre à improviser en abaissant un
seuil.

## Gigasweep frais — 100 mondes, 2026-07-19

Protocole : 100 graines distinctes `3 + 101·n`, `n=0..99`, chacune sur 250 ans avec
6 empires et 12 cités-États. Exécution en 20 lots de 5, sans recouvrement, à partir du
binaire courant (`sha256 c5f3631e…`). Les 100 journaux sont complets.

Le premier passage a rendu **98 mondes verts, 2 rouges**. Les graines 912 et 1114 ont
reproduit sous `SCPS_INVDIAG=1` des pics M3c de respectivement 533 % et 658 %, contre un
seuil de 370 %. La dette était nulle ou minime : le crédit n'était pas la cause.

Cause lue dans le code : dans `scps_trade.c`, le règlement d'un flux crédite
l'exportateur, puis construit `need_tot` à partir de la richesse de l'importateur mais
conditionne chaque classe avec `exp->strata[c].pop`. Une région exportatrice sans
population peut donc recevoir le produit de la vente alors que `need_tot==0` et que
l'importateur ne débourse rien. Les diagnostics montrent précisément l'or apparaissant
en richesse bourgeoise dans des provinces actives, non colonisées et sans population.

Le seuil M3c n'a pas été relevé. Après correction atomique du commerce, les deux graines
fautives passent sur 250 ans (912 : pic ramené de 533 % à 84 % ; 1114 : plus de rouge),
puis le balayage complet `sweep_giga_2026-07-19_100_fixed` termine **100/100 vert**.

Tendances du passage corrigé : population finale médiane 321k ; 4 169 guerres toutes
catégories et 26 312 batailles ; 1 997 soulèvements et 159 sécessions ; routes maritimes
dans tous les mondes. Le mécanisme naval restait presque dormant (2 interceptions au
total), ce qui a conduit à l'audit de raccord détaillé ci-dessus.

## Contrôle plafonné après l'audit naval

Un contrôle de **10 simulations**, exécuté avant le dernier retrait du mur de blocus,
a terminé 10/10 sans anomalie économique. Il a produit zéro interception et a ainsi
permis d'isoler le dernier verrou : le blocus annulait l'ordre avant que le convoi
n'existe. Ce résultat ne doit pas être présenté comme une mesure du code naval final.

Conformément à la consigne, aucune nouvelle Chronicle n'a été lancée après ce correctif.
La prochaine mesure de fréquence en monde vivant nécessitera un accord explicite et sera
limitée à 10 simulations sauf autorisation spécifique pour davantage.

## Endgame explicitement préservé

Aucun fichier ni tunable d'endgame n'a été modifié. L'an **180** reste l'ouverture dure
de l'endgame, et le **réchauffement à l'an 240** reste le repli de sécurité demandé.

## Re-key province et matière cartographique — 2026-07-19

Statut : **IMPLÉMENTÉ — VALIDATION CIBLÉE VERTE**.

### Intégrité province → région

- `ai_build_raw_boost` choisit et améliore maintenant la province brute exacte. Le
  palier ne s'écrit plus dans `RegionEconomy`, agrégat qui l'effaçait au tick suivant.
- Le même garde a trouvé deux écritures événementielles de `build.food_cap` dans le
  miroir régional. Année sans été et Moissons modifient désormais les provinces, puis
  reprojettent immédiatement leur somme pour les lecteurs UI/directeur.
- `region-write-check`, intégré à `make smoke` et `make test`, bloque les nouveaux
  écrivains persistants de `region[]` hors allowlist explicite et marqueur
  `REGION_MIRROR_OK`.
- `econ_production_demo` vérifie la projection du palier puis sa persistance après deux
  agrégations successives.

### Carte parchemin

- Fog découplé des frontières : invalidation au monde neuf, changement de souveraineté
  et changement d'année. Son cœur reste alpha 255 ; seul son RGB reçoit un grain sépia
  et une hachure diagonale très discrète.
- Ombres des villes dérivées de l'alpha, adoucies à résolution réduite, projetées au sol
  et cachées avec la vignette. Les hameaux libres utilisent alpha 0,70 et ombre ×0,60.
- Troisième lavis intérieur des frontières reculé à 2,20 cellules et ramené à alpha 0,06.
- Lavis politique modulé par un grain déterministe basse fréquence de ±5 %, masque alpha
  inchangé.
- Saison shader indépendante (`day_of_year/365`), limitée à 4 % et appliquée avant
  `variant_map` afin que l'endgame garde la priorité.
- Hachures de falaises irrégularisées ; routes et montagnes laissées intactes après
  lecture du pipeline réel.
- `shot_parch` et `viewer_audit` utilisent maintenant 365 jours. Le premier possède un
  override saisonnier d'affichage qui ne fait pas évoluer le monde ; le second échoue
  désormais si le script Overlay n'a pas été chargé, au lieu d'afficher un faux vert.

### Preuves de ce lot

- `region-write-check` : vert.
- `econ_production_demo` : **5/5**.
- `ai_demo 9` : **26/26**.
- `events_demo` : **119/119**.
- GDExtension recompilée ; scan éditeur Godot final : sortie 0.
- `viewer_audit`, graine 9, an 1 : **OK** après correction d'un défaut de typage trouvé
  par le chargement réel de scène.
- Huit captures de contrôle ont couvert capitale, hameau, fog opaque et saisons. Elles
  ont aussi révélé que le premier override saisonnier était réécrit par `_draw()` ; le
  harnais conserve maintenant explicitement son jour de preview et son parsing est vert.
  Aucune onzième simulation n'a été lancée afin de respecter le plafond.
- Probe scène complète 1920×1080 : **15,6 FPS**, moteur 60 jours **18,2 ms**. Sans mesure
  avant identique, ce chiffre n'est pas attribuable à ce lot ; il révèle néanmoins une
  dette de rendu globale à profiler séparément.

Aucun Chronicle ni gigasweep n'a été exécuté dans ce lot. L'endgame an 180 et son repli
an 240 n'ont pas été modifiés.
