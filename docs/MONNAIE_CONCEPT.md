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
4. **ÉTALON BIMÉTALLIQUE — parité FIXE à la ressource** (v5, décision joueur
   2026-07-14 — remplace la frappe au prix courant de v4) : la monnaie est liée au
   MÉTAL, pas à sa cote. 1 tonne d'or frappe **MINT_PARITY_GOLD = 8** monnaies
   (« l'étalon, à calibrer — part sur 8 » = le prix de base, rien ne se recale) ;
   le cuivre **MINT_PARITY_COPPER = 2.6**. Registre J. Le MARCHÉ de l'or/cuivre
   continue de flotter AUTOUR de la parité → l'arbitrage devient un CHOIX émergent :
   vendre son métal quand la joaillerie/l'industrie paie au-dessus de la parité,
   le frapper quand elle paie en dessous. Aucun métal privilégié par construction.
   Conséquence d'identité : la monnaie EST du métal frappé — le trésor devient
   littéralement du métal (M6 en découle), l'invariant M(t)=M(0)+frappe devient
   physique.
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
**Cœur A — QUI VEND ? — TRANCHÉ (joueur, go M3 2026-07-14)** : le **COMPTE DE MARCHÉ**
— l'acheteur PAIE ; le produit des ventes est reversé aux provinces PRODUCTRICES
∝ leur contribution (les stocks étant un pool national, la traçabilité par unité
n'existe pas — la répartition ∝ valeur produite du tick est la vérité disponible),
puis aux classes au split existant 42/20/38 (les parts forfaitaires deviennent la CLÉ
DE RÉPARTITION du produit des ventes, plus une création). L'État ne vend rien ; il
taxe et péage. (Alternative « propriété par classe » écartée : plus lourde.)
⚠ RÉVERSIBILITÉ M3 : PAS de kill-switch runtime (deux économies parallèles =
ingérable) — la réversibilité est PAR COMMIT (vagues séparées + tag `pre-m3`).
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

### M5 — LE REVENU PROPRE + L'ASSIETTE (LIVRÉ, 2026-07-15)
**Statut : CALIBRÉ-LIVRÉ.** Décision joueur : « Le toll, 50/50 état-bourgeois. Réserve d'or et
de cuivre au début (100/100). La gabelle... mauvaise idée pour l'instant. […] Moi je pars sur
le toll, la réserve initiale, "paie ton assiette". » Contexte : les États empruntaient AVANT
d'avoir un fisc (dette mondiale early élevée, sweep graine 9). La gabelle et la régale élargie
sont REJETÉES (non implémentées).
- [x] **R1 — LE TOLL 50/50** : les 3 sites de péage (échange inter-empire TRADE_LEVY, détroit,
      marge d'import chantier) versaient 100 % aux BOURGEOIS (item 5, M3b-v2.1) — l'État y
      perdait un revenu. `TOLL_STATE_SHARE` (défaut 0.5, registre J) partage désormais entre
      le trésor de la province-hôte et les bourgeois. Diagnostic : le flux « péages+ » n'était
      PAS un site mort (il s'alimente, croît avec l'activité commerciale) — il est
      structurellement PETIT car TRADE_LEVY (10 %) ne taxe que le canal route bilatérale
      inter-empire (pas le commerce intra-empire scps_trade, pas les Centres) ; calibrage à
      trancher par le joueur si un revenu plus visible est désiré (cf. TROUVAILLES M5).
- [x] **R2 — LA RÉSERVE DE GENÈSE 100/100** : `GENESIS_RESERVE_GOLD_EMPIRE`/
      `GENESIS_RESERVE_COPPER_EMPIRE` (défaut 100/100, registre J) — un empire jouable/IA naît
      désormais avec une réserve métallique (le champ M1 `reserve_gold`/`reserve_copper`,
      jusqu'ici réservé aux cités-états à 200/500, INTACT) → seigneuriage early même sans mine.
      Se frappe par le MÊME canal que la redevance royale (aucune voie neuve).
- [x] **R3 — « PAIE TON ASSIETTE »** : audit d'abord (TROUVAILLES) — la consommation
      créditait DÉJÀ le trésor depuis M3b-v2 (« l'État revend »), le « trou » réel était
      ailleurs : (a) AUCUNE ration n'était GARANTIE (le grain, vital, subissait le même gate
      d'affordabilité que le confort — le risque de collapse M3b-v1 restait ouvert) et
      (b) la demande était STRICTEMENT LINÉAIRE à la pop, jamais sensible à la richesse.
      Câblage : `ASSIETTE_ON` (kill-switch, défaut 1) sépare désormais RES_GRAIN (need_rank==0,
      universel — « le seigneur garant du stock de grain ») en ration VITALE GARANTIE (servie
      au stock physique disponible, jamais gatée par l'affordabilité ; payée au mieux, le
      manquant TOLÉRÉ sans dette) du reste du panier, qui devient ÉLASTIQUE à la richesse
      (`CONSUME_ELASTIC_K/MIN/MAX`, calibré 0.3/0.8/1.2 — une classe riche consomme jusqu'à
      +20 % de confort, une pauvre se serre jusqu'à −20 %, référencé au panier/tête du tick
      précédent `g_basket_pc`). Calibrage plus large (K=0.5, bande 0.5-2.0) cassait la bande
      Laborer (43-51 % vs 50-64 requis) — resserré après sweep, voir TROUVAILLES.
- Gate : kill-switches prouvés (golden pre-m5 byte-identique) · sweep apparié {9,11,42}×3×250 ·
  bandes M3g/h/i tenues · `make test`/determinism/golden/savetest/fuzztest verts. Détail complet
  (mesures, pièges, découvertes) : TROUVAILLES.md « CHANTIER MONNAIE — M5 ».
- **Restes** : TRADE_LEVY calibrage (proposé au joueur, non tranché) · gabelle/régale élargie
  (rejetées pour l'instant) · l'ethos-luxury cross-desire n'est PAS élastique (scope, cf.
  TROUVAILLES) · UI (part du revenu par source) non câblée (aucun reader façade demandé).

### M6 — LES RÉFLEXES MONÉTAIRES IA
- [ ] Fuite vers le métal (prix locaux hauts → thésauriser, frapper moins).
- [ ] Débase de guerre (trésor vide + guerre → frapper fort, assumer l'inflation).
- [ ] Arbitrage (acheter le métal bon marché du voisin pour le frapper chez soi —
      germe : le spéculateur intertrade).
- Gate : coordonnées réelles, télémétrie par réflexe, sweep.

### M7 — L'INFLATION SÉCULAIRE + LA DÉCOUVERTE D'OR (LIVRÉ, 2026-07-16)
**Statut : CALIBRÉ-LIVRÉ.** Décision joueur : « Inflation séculaire (1% par an ?),
découverte d'or sur certaine tile par évent (0,5N(empire) par game). » Contexte :
`price_level[c]` (le facteur monétaire M3b-v2) était plafonné à 1.0 en dur — le système
savait déflater, jamais inflater ; c'était la spec non tenue depuis le début du chantier
(« pas de perte de monnaie… inflation séculaire = trait historique »).
- [x] **I1 — LE DÉPLAFONNAGE** : `INFLATION_CAP` (registre J, défaut 1.6) remplace le
      `1.f` codé en dur de `price_level`. Quand la caisse d'État déborde la VA
      nationale, les prix montent au-dessus du pair — le MÊME circuit qui revend/paie
      (M3b-v2) fait toute la transmission, aucun taux codé en dur. Calibré (MINT_ROYALTY/
      MINT_AI_SHARE montés 0.35→0.6, INFLATION_CAP=1.6) pour une dérive mondiale
      moyenne dans la cible 0.5-1.5 %/an (mesuré : +0.51 %/an sur le sweep officiel
      {9,11,42}×250 ; +0.90 %/an à 10 empires fixes — cf. TROUVAILLES M7 pour la
      variance inter-graines assumée). L'étalon or/cuivre reste EXEMPTÉ (pl=1, la
      parité fixe est le numéraire) et le déclampage reste PAR PAYS (jamais l'IPM
      mondial `e->ipm` — la contrainte « pas de déclampage MONDIAL » ci-dessous tient).
- [x] **I2 — LA DÉCOUVERTE D'OR** : évènement `EVID_GOLD_DISCOVERY` (pays, budget
      mondial ≈0.5×N(empires)/partie posé à `events_init`). **Conception revue en cours
      de mission** (décision coordinateur) : la version initiale « slot raw libre »
      (`resource2==RES_NONE`) s'est mesurée à 0 % d'éligibilité (le worldgen pose une
      « pincée partout » qui remplit quasi toujours le 2e slot) — remplacée par un
      **remplacement 1-pour-1 de la ressource COMMUNE mondiale dominante** (tally
      déterministe à la genèse, hors rares/faustiens/or/cuivre) : la tile éligible porte
      cette ressource dans un de ses ≤2 slots, l'évent le convertit en RES_GOLD et
      transfère son `raw_cap` tel quel (≤2 raws respecté PAR CONSTRUCTION). AUCUN
      modificateur direct — le circuit royalty→réserve→frappe existant (M1/M2) porte
      tout le choc, émergent (mesuré : le pays découvreur peut voir son indice de prix
      rester DURABLEMENT au-dessus du monde plusieurs décennies, cf. TROUVAILLES M7).
- Gate : kill-switch prouvé (`INFLATION_CAP=1.0,GOLD_DISCOVERY_RATE=0,MINT_ROYALTY=0.35,
  MINT_AI_SHARE=0.35` → golden pré-M7 byte-identique) · `make test` 38/38 (1 build
  échec pré-existant Windows) · golden re-baseliné · determinism/deep/savetest/fuzztest
  verts. Détail complet (mesures, pièges, découvertes, le virage de conception I2) :
  TROUVAILLES.md « CHANTIER MONNAIE — M7 ».
- **Restes** : variance inter-graines de la dérive I1 non lissée (un effondrement
  ponctuel possible, assumé comme épisode déflationniste légitime) · sous-réalisation
  des découvertes d'or (≈52 % du plafond théorique, non recreusé) · effet vivrier local
  d'une découverte non chiffré en télémétrie dédiée (aucune famine observée en
  calibrage, mais pas mesuré systématiquement) · SAVE_VERSION 94.

### M8 — LA CENTRALISATION FISCALE + LE TRANSPORT (v2 : reformulé ; ex-M7, renuméroté
pour laisser place au chantier LIVRÉ ci-dessus)
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
