# LA MONNAIE MÉTALLIQUE — concept & feuille de route (v2)

> Statut : **CONCEPT ACTÉ, non implémenté** (discussion joueur 2026-07-14 ; v2 intègre
> la revue de code du joueur — 7 corrections). Le chantier ne démarre que sur ordre
> explicite ; ce document est le contrat.

## Vision

L'or-monnaie est le dernier grand compteur abstrait de SCPS. Ce chantier le rend
diégétique : la monnaie est du métal frappé, créée par la frappe SEULE, conservée,
transférée — jamais inventée. L'inflation devient un comportement émergent de la
géologie et de la politique monétaire — y compris l'inflation séculaire et les chocs
historiques (Mansa Musa, la révolution des prix espagnole), qui se PROPAGENT par le
commerce au lieu de frapper la planète uniformément.

## Fondations vérifiées dans le code (ce qui tient déjà)

- Le commerce international fait DÉJÀ de vrais transferts : achats/ventes/routes
  débitent l'acheteur et créditent le vendeur (scps_intertrade.c:650, :972).
- La production crée la richesse des classes depuis la VA (scps_econ.c:3078,
  split 42/20/38 salaires/profit/rente) — c'est la « planche à billets » à convertir.
- Le trésor est DÉJÀ stocké au grain provincial, le national est une somme
  (scps_econ.c:2675) — la « localisation » M6 est en fait une centralisation.

## Décisions actées

1. **Frappe = seule création monétaire.** Curseur « part de la réserve frappée »
   au MENU ÉCONOMIQUE (régalien) ; la monnaie entre au trésor, la redistribution
   passe par les canaux existants.
2. **Redevance minière EN NATURE** (v2 — remplace le « surplus confisqué ») : une
   fraction de l'extraction d'or/cuivre appartient IMMÉDIATEMENT à l'État (frappable) ;
   le reste demeure un bien marchand. Pas de confiscation implicite de l'invendu.
3. **Les usages physiques d'abord** (v2 — remplace « le luxe d'abord ») : le cuivre
   n'est pas qu'un luxe — fournitures navales, armes à feu, horloges, colifichets
   (scps_econ.c:425). Seul le surplus MARCHAND au-delà d'un stock de fonctionnement
   peut rejoindre la réserve monétaire (en sus de la redevance).
4. **Frappe NEUTRE EN VALEUR — 1:1, pas de Gresham** (v4, décision joueur
   2026-07-14 — remplace 10:1 puis 5:1) : le métal se convertit en monnaie À SON PRIX
   DE MARCHÉ courant (1 unité de VALEUR métal = 1 unité de monnaie). Aucun rapport
   fixe or/cuivre, donc AUCUN arbitrage, aucun métal à privilégier — l'or frappe plus
   par tonne simplement parce qu'il VAUT plus (prix de base 8 vs 2.6, scps_econ.c:315,
   et le prix courant flotte). La loi de Gresham est retirée du design.
   **Préparatif DÉJÀ implémenté (2026-07-14)** : « à la tonne » — 1 unité de ressource
   est un LOT MASSIF, une tonne d'or n'est pas une pièce → intrants d'or des recettes
   ÷4 (joaillerie 0.8→0.2, parurier 1.0→0.25) : la consommation physique d'or diminue,
   le surplus mintable grossit d'avance.
5. **L'impôt per-capita NE CHANGE PAS.**
6. **AUCUNE perte de monnaie** (métaux stables ; un drain serait inéquilibrable).
   L'inflation séculaire est un TRAIT — mais pas une garantie : cf. M4 (vitesse).
7. **Sans mine = troc pénalisant ÉMERGENT** — avec un garde-fou de DÉMARRAGE (v2) :
   la dotation de genèse M(0) existe (les strates naissent riches, scps_econ.c:1030)
   et le troc/la subsistance en nature restent possibles — sinon ce n'est pas une
   pénalité, c'est un marché qui ne démarre jamais.
8. **Pas de simulateur de trading IA** — des réflexes (M5).
9. **Monnaie-objet en deux étages** : conservée (M3) puis centralisée/transportable (M6).

## L'invariant (v2)

**M(t) = M(0) + frappe cumulée** — où M(0) est la dotation de genèse, assumée et
mesurée à l'an 0. Banc annuel sur 250 ans ; toute dérive = un site non converti.

## Points à suivre

### M0 — L'AUDIT : classer CHAQUE mutation monétaire (lecture seule, 0 risque)
Chaque `+=`/`-=` sur wealth/treasury/or est classé en **CRÉATION · DESTRUCTION ·
TRANSFERT · DETTE · INITIALISATION**. Sans les puits, on supprimerait les planches à
billets en GARDANT les trous noirs. Sites déjà identifiés :
- [ ] CRÉATION : VA → 3 pools (scps_econ.c:3078) ; récompenses/events/butins (à lister).
- [ ] **DESTRUCTION (les trous noirs)** : la consommation détruit la richesse sans
      créditer un vendeur (scps_econ.c:3353) ; l'entretien, la cour, l'administration
      et la fraction non salariale de la dépense publique disparaissent (scps_econ.c:3118).
- [ ] TRANSFERT : impôts, intertrade (:650/:972), routes, tributs, pillages (vérifier un à un).
- [ ] DETTE : credit_spend laisse le trésor passer négatif sans que le prêteur avance
      rien (scps_credit.c:67) — cf. M3.
- [ ] INITIALISATION : dotation de genèse des strates (scps_econ.c:1030) → mesurer M(0).
- [ ] Télémétrie « masse monétaire » : M(t), sa dérive/an, et la part par catégorie.
- Gate : golden IDENTIQUE ; le rapport chiffre créations ET destructions annuelles.

### M1 — LA REDEVANCE + LA RÉSERVE
- [ ] Redevance minière en nature : `MINT_ROYALTY` (part de l'extraction or/cuivre → 
      réserve d'État directement, ⚠ champ sérialisé → SAVE BUMP). Le reste = marchandise.
- [ ] Le surplus marchand au-delà du stock de fonctionnement (demande physique servie :
      navale, feu, horlogerie, luxe) peut REJOINDRE la réserve (vente à la Monnaie, payée
      au prix du marché — un transfert, pas une saisie).
- [ ] Menu éco : « Réserve : X or · Y cuivre ».
- Gate : re-baseline documentée ; sweep : les chaînes cuivre (navale/feu) vivent.

### M2 — LA FRAPPE
- [ ] Curseur « part de la réserve frappée » + frappe auto-arbitrée (or d'abord à 10:1,
      cf. décision 4) ; défaut IA ~10-25 %.
- [ ] Télémétrie « frappe : X or/an · métal choisi · N empires frappeurs ».
- Gate : sweep apparié OFF/ON ; IPM encore clampé à cette étape.

### M3 — LA CONSERVATION (le gros œuvre — et ses DEUX cœurs, v2)
**Cœur A — QUI VEND ?** Fermer la boucle exige un DESTINATAIRE pour chaque dépense de
consommation. Les stocks sont nationaux, les revenus forfaitaires — « l'acheteur paie
le producteur » n'a pas de destinataire défini aujourd'hui. **Proposition à discuter** :
le **COMPTE DE MARCHÉ provincial** — l'acheteur paie le compte de marché de la province
productrice, qui reverse aux classes au split existant 42/20/38 (les parts forfaitaires
deviennent la clé de répartition du produit des VENTES, plus une création). L'État ne
vend rien ; il taxe et péage. Alternatives : propriété par classe (les bourgeois vendent
les biens manufacturés, les journaliers le brut) — plus riche, plus lourd.
- [ ] Trancher le vendeur (compte de marché vs propriété par classe).
- [ ] Convertir les CRÉATIONS de M0 en ventes (payées par les acheteurs).
- [ ] Boucher les DESTRUCTIONS de M0 : la consommation crédite le vendeur (:3353) ;
      l'entretien/la cour/l'admin PAIENT quelqu'un (gages, fournisseurs) (:3118).
**Cœur B — LE CRÉDIT (v2 : dans le noyau, pas un site secondaire)** :
- [ ] Un prêt DÉPLACE des pièces réelles : le coffre du créancier se vide, le débiteur
      reçoit du positif, la dette devient un PASSIF séPARÉ.
- [ ] Une trésorerie négative n'existe plus comme « monnaie négative ».
- [ ] **Banc invariant** M(t) = M(0) + frappe, annuel, 250 ans × 5 graines.
- Gate : invariant vert ; satisfaction/pop dans les bandes ; avance au pas (une famille
  de sites à la fois, sweep entre chaque — le monde est bistable).

### M4 — LES PRIX LOCAUX + LE DÉCLAMPAGE (v2 : le local N'EST PAS optionnel)
- [ ] Niveaux de prix **PAR ÉCONOMIE** + **contagion par les échanges** (routes/Centres
      transmettent l'inflation) — le déclampage MONDIAL est retiré du plan : une
      découverte d'or lointaine ne doit PAS toucher la planète instantanément
      (l'IPM actuel : mondial, borné, mean-reverting — scps_econ.c:2689).
- [ ] L'inflation lit la **monnaie ACTIVE et sa VITESSE de circulation**, pas le stock
      total : les pièces thésaurisées dorment ; et la production peut croître plus vite
      que M — des épisodes DÉFLATIONNISTES sont possibles et voulus (l'inflation
      séculaire n'est pas garantie par la seule absence de perte, c'est un résultat).
- [ ] Retirer les bornes/mean-reversion une fois la conservation prouvée (M3).
- Gate : pas de runaway ; le sweep raconte Mansa Musa (l'or d'un conquérant déstabilise
  ses PARTENAIRES commerciaux, gradient visible par empire).

### M5 — LES RÉFLEXES MONÉTAIRES IA
- [ ] Fuite vers le métal (prix locaux hauts → thésauriser, frapper moins).
- [ ] Débase de guerre (trésor vide + guerre → frapper fort, assumer l'inflation).
- [ ] Arbitrage (acheter le métal bon marché du voisin pour le frapper chez soi —
      germe : le spéculateur intertrade).
- Gate : coordonnées réelles, télémétrie par réflexe, sweep.

### M6 — LA CENTRALISATION FISCALE + LE TRANSPORT (v2 : reformulé)
Le trésor est DÉJÀ provincial (:2675) — M6 n'est pas une « localisation » mais :
- [ ] La remontée fiscale devient un TRANSPORT physique vers la capitale (convois,
      délai, interceptables) ; le coffre de la capitale = la cible du sac.
- [ ] Le pillage prend de la monnaie RÉELLE (conservation tenue) ; trésor de guerre
      transportable/capturable en campagne.
- Gate : l'invariant survit au pillage ; sweep guerre (le sac plus rentable → mesurer).

## Ce qu'on NE fait PAS
- Pas de perte/usure de monnaie · pas de dîme proportionnelle · pas de simulateur de
  trading · pas de plancher monétaire pour l'empire sans mine (mais le troc/nature
  DOIT démarrer une économie) · pas de déclampage MONDIAL (contredit la vision).

## Risques nommés
- **M3 cœur A** : mal choisir le vendeur affame ou enrichit une classe entière —
  le sweep apparié tranche, pas l'intuition.
- **M3 vitesse** : la monnaie frappée doit ATTEINDRE les salaires assez vite ; une
  circulation trop lente = déflation d'étranglement au moment de la bascule.
- **M4 coût** : des prix par empire multiplient l'état (sim/save) — mesurer avant
  d'engager ; c'est le prix de la vision, pas une option.
- Bistabilité générale : chaque étape re-baseline ; sweep apparié obligatoire.
- Valeurs nominales croissantes sur 250 ans : l'UI doit raconter la hausse des prix,
  pas la subir.
