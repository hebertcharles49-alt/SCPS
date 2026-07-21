# Crédit rationné par les prêteurs

**Date** : 2026-07-21  
**Statut** : implémenté et vérifié par les bancs ciblés  
**Périmètre** : dette publique, prêts physiques, taux, exposition des créanciers,
refinancement, défaut, télémétrie et interface économique/diplomatique.

## Intention

Remplacer le plafond administratif de dette à `300 %` du revenu par un marché du crédit
borné du côté des prêteurs.

L'État débiteur ne devient pas artificiellement prudent. Il peut continuer à emprunter
tant qu'il trouve un créancier disposant à la fois :

- de liquidités réellement présentes ;
- d'une réserve qu'il accepte de ne pas entamer ;
- d'une marge dans son portefeuille de prêts ;
- d'une exposition encore acceptable envers ce débiteur précis.

Le ratio dette/revenu ne constitue plus une autorisation ou une interdiction. Il sert à
fixer le prix du risque.

## Questions de design tranchées

### Qui prête ?

- Les ordres nationaux prêtent en premier : Élites et Bourgeois.
- Les Laboureurs et les Esclaves ne disposent pas d'une capacité de prêt public.
- La chaîne automatique sollicite ensuite les cités-États et les puissances
  mercantiles ou pacifistes.
- Un prêt diplomatique explicite peut toujours être demandé à un autre État ; son
  consentement diplomatique reste distinct de sa capacité financière physique.

### Quelle assiette mesure la solvabilité ?

Le revenu fiscal annuel capturé par `econ_country_tax_year`.

Un plancher technique `DEBT_REVENUE_FLOOR` évite une singularité pendant les premiers
mois, avant que l'assiette fiscale annuelle soit connue. Ce plancher ne crée aucune
capacité de prêt : il sert uniquement au calcul du taux.

### Comment le taux évolue-t-il ?

Pour `x = dette / revenu fiscal annuel` :

```text
taux = BASE + LINEAR × x + QUAD × x²
```

Valeurs initiales :

- `DEBT_RATE_BASE = 0.02`
- `DEBT_RATE_LINEAR = 0.015`
- `DEBT_RATE_QUAD = 0.0075`
- `DEBT_RATE_MIN = 0.02`
- `DEBT_RATE_MAX = 0.50`

Le maximum est une borne numérique de contrat, pas un plafond de dette. Sous le régime
`DEBT_FIXED`, le taux est un coût forfaitaire figé à l'origination : emprunter `100` à
`5 %` inscrit une créance de `105`, sans intérêt composé annuel sur cette tranche.

### Quelle exposition est acceptable ?

Ordres nationaux :

- capacité par tirage : `CLASS_LEND_SHARE = 5 %` du patrimoine liquide pondéré ;
- exposition maximale à l'État : `CLASS_EXPOSURE_SHARE = 50 %` de leur capital,
  défini comme patrimoine liquide pondéré + créance existante.

États prêteurs :

- réserve incompressible : le surplus au-dessus de `SINK_FLOOR` seulement ;
- capacité par tirage : `CITYSTATE_LEND_SHARE = 50 %` du surplus disponible ;
- portefeuille maximal : `LENDER_PORTFOLIO_SHARE = 75 %` du capital prêtable ;
- exposition maximale à un débiteur : `LENDER_DEBTOR_SHARE = 35 %` du capital prêtable.

Le capital prêtable d'un État est la somme de son surplus liquide et de ses créances
encore vivantes. Le markup forfaitaire appartient à la créance et consomme donc lui aussi
la limite d'exposition.

### Plusieurs créanciers étrangers ?

Le modèle existant à un seul créancier étranger par débiteur est conservé.

Un créancier illiquide ne peut plus être remplacé silencieusement par un autre tandis que
sa créance reste inscrite. Le changement de créancier passe obligatoirement par :

- le remboursement ;
- le rachat de la dette ;
- ou la banqueroute.

Ce choix permet de dériver les expositions depuis le livre existant, sans nouvel état
persistant ni changement du format de sauvegarde.

### Une échéance peut-elle être refinancée ?

Oui.

Le service annuel suit désormais cet ordre :

1. paiement depuis le surplus du débiteur ;
2. refinancement auprès des ordres nationaux ;
3. refinancement auprès du créancier étranger courant ou d'un premier créancier
   automatique si aucune dette étrangère n'existe encore ;
4. constat d'un impayé seulement sur le reliquat non couvert.

Un rollover par le créancier actuel fait circuler physiquement les pièces : il avance le
montant de l'échéance, puis le reçoit en remboursement de l'ancienne tranche. Sa trésorerie
retrouve donc le principal, mais son exposition augmente du coût du nouveau contrat.

### Quand survient le défaut ?

Le simple franchissement d'un ratio dette/revenu ne déclenche plus rien.

Le streak d'insolvabilité augmente seulement lorsqu'une échéance :

- reste impayée après les tentatives de refinancement ;
- et porte sur une dette supérieure à `DEBT_DEFAULT_THRESHOLD`.

Après `BANKRUPTCY_GRACE_YEARS` années consécutives, la banqueroute forcée existante est
déclenchée. La répudiation, la cicatrice, la saisie et les conséquences diplomatiques
préexistantes sont conservées.

## Comportement de l'IA débitrice

La prudence fiscale liée à la proximité de l'ancien plafond a été retirée.

Le contrôleur fiscal ne réduit plus ses décisions en fonction de la dette ou du streak
d'insolvabilité. Seule la protection du bootstrap day-1 subsiste : avant qu'une assiette
fiscale réelle soit mesurée, une satisfaction initialement vide ne provoque pas une baisse
mécanique des impôts.

Le système est donc borné par les créanciers, pas par une anticipation rationnelle de
l'État emprunteur.

## Interface

### Panneau Monnaie

La section Dette affiche désormais :

- dette totale ;
- dette due aux ordres ;
- dette due au créancier étranger ;
- revenu fiscal annuel ;
- ratio dette/revenu en années de revenu ;
- crédit physique disponible immédiatement ;
- exposition du créancier et marge restante ;
- taux forfaitaire proposé pour une nouvelle tranche ;
- échéance annuelle réelle.

### Tiroir diplomatique

Avant l'envoi d'une demande de prêt, le joueur voit :

- le principal maximal disponible ;
- le taux fixe proposé ;
- le surplus liquide du prêteur ;
- son exposition actuelle envers le demandeur ;
- sa limite d'exposition ;
- la raison physique d'une indisponibilité ;
- l'existence éventuelle d'un autre créancier étranger bloquant la demande.

La capacité physique n'anticipe pas le consentement diplomatique : un État disposant de
fonds peut encore refuser pour des raisons d'opinion ou de relation.

## Télémétrie Chronicle

Les anciennes mentions de pays « au plafond » ont été remplacées par :

- taux moyen observé ;
- nombre de pays endettés ;
- nombre de dettes structurelles supérieures ou égales à `3×` le revenu ;
- ratio dette/revenu maximal ;
- nombre de marchés étrangers effectivement fermés par manque de capacité ;
- revenu, dette et ratio dans les diagnostics d'invariant et de débasage.

## Fichiers modifiés

Moteur :

- `scps/scps_credit.c`
- `scps/scps_credit.h`
- `scps/scps_tune_list.h`
- `scps/scps_econ.c`
- `scps/chronicle.c`

Façade C et bancs :

- `scps/scps_api.c`
- `scps/scps_api.h`
- `scps/credit_demo.c`
- `scps/scps_api_demo.c`

Façade Godot et interface :

- `godot/src/scps_sim_node.cpp`
- `godot/src/scps_sim_node.h`
- `godot/project/ui/budget_panel_v2.gd`
- `godot/project/ui/country_actions.gd`

Documentation générale également mise à jour :

- `docs/MONNAIE_CONCEPT.md`
- `docs/PLAN_PROFONDEUR_INTERFACE.md`

## Validation effectuée

- `credit_demo` : **85/85**
- `scps_api_demo` : **226/226**
- `core_demo` : **35/35**
- compilation de `chronicle` sans avertissement ;
- compilation de l'extension Godot debug sans avertissement ;
- parse Godot headless propre ;
- `git diff --check` propre.

Aucun sweep ni gigasweep Chronicle n'a été lancé.

## Points restant à calibrer

La mécanique et ses invariants sont vérifiés, mais les paramètres de distribution doivent
encore être observés sur des simulations autorisées :

- `CLASS_EXPOSURE_SHARE = 50 %` ;
- `LENDER_PORTFOLIO_SHARE = 75 %` ;
- `LENDER_DEBTOR_SHARE = 35 %` ;
- coefficients linéaire et quadratique du taux ;
- `DEBT_DEFAULT_THRESHOLD` ;
- durée de grâce avant banqueroute.

Les résultats recherchés sont :

- une poignée de gros débiteurs structurels ;
- une majorité de pays peu ou pas endettés ;
- des créanciers concentrés mais non universels ;
- des rollovers durables pour les États aux fondamentaux solides ;
- des défauts provoqués par la fermeture du crédit, pas par un mur administratif.

