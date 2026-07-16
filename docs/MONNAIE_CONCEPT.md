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

### M8 — LE CERCLE VERTUEUX DE L'IMPÔT (LIVRÉ, 2026-07-16)

**Statut : CALIBRÉ-LIVRÉ.** Décision joueur : « Les biens manufacturés doivent nourrir les
besoins ET les impôts, cercle vertueux de l'impôt. Plus satisfait = paye plus. […] tu peux
largement booster leur fiscalité pour atteindre l'équilibre, ça permet de renflouer les
caisses simplement, mais du coup plus sensibles aux chocs exogènes. L'IA doit jouer avec la
fiscalité pour atteindre les 60 % de satisfaction (marge de sécurité). »
- [x] **C1 — SATISFACTION → CAPACITÉ FISCALE** : `econ_satisfaction_tax_factor` (scps_econ.c)
      ajoute une SECONDE modulation du seuil de tolérance fiscale (§7/§3b), par-dessus la
      modulation plate déjà existante (0.40+0.60·sat) — au-dessus de 60 % (TAX_SAT_REF) la
      tolérance s'élargit, en dessous elle se resserre (`TAX_SAT_COUPLING`, calibré 0.35).
      0 = kill-switch exact. La sensibilité aux chocs exogènes voulue par le joueur est
      ÉMERGENTE de cette même formule (satisfaction plonge → seuil plonge → évasion/grogne
      montent → collecte baisse) — rien de codé à part.
- [x] **C2 — LE CURSEUR FISCAL PAR ORDRE** : AUDIT AVANT câblage (demandé par le brief) — le
      curseur par ordre (Laborer/Bourgeois/Élite) existait DÉJÀ de bout en bout depuis AVANT
      le chantier MONNAIE (`tax_mult[cid][c]`, commit pré-MONNAIE 92efb58 ; verbe
      `CMD_BUDGET_POLICY(family=0,…)` ; slider GDScript déjà par classe). Aucun « curseur
      global » n'a jamais existé à remplacer — aucun nouveau verbe créé. Ce qui manquait :
      un reader façade COMBINÉ (`scps_country_fiscal_orders`, taux+satisfaction+revenu en un
      appel) pour la future UI-MONNAIE, assemblé depuis 3 lectures pures déjà existantes +
      `econ_country_class_satisfaction` (nouveau, agrégat pop-pondéré province-grain).
- [x] **C3 — L'IA FISCALE À 60 %** : `econ_ai_fiscal_tick` (scps_econ.c, appelé mensuellement
      dans la boucle frappe d'econ_tick) ajuste `tax_mult[cid][c]` par classe pour viser
      `AI_FISCAL_TARGET=0.60` — au-dessus, serre la vis ; en dessous, relâche. Zone morte
      `AI_FISCAL_DEADBAND` (hystérésis anti-oscillation, calibré 0.05), pas borné
      `AI_FISCAL_STEP`/mois (calibré 0.012). Jamais le joueur ni les esclaves. Découverte au
      gate 1 : `culture_player_cid()` ne peut PAS servir de signal « joueur » ici (reste -1
      tant qu'aucune culture n'a été composée à la main, gated par `culture_any_active()`,
      donc faux dans tout monde vanilla) — `econ_is_human_country` (g_econ_human, posé SANS
      condition à la genèse, même mécanisme qu'`econ_build_tick` §NF) est le signal fiable.
- **La chaîne manufacture→satisfaction→fisc, tracée et VIVANTE** (SCPS_M8DIAG, chronicle) :
  aucun maillon mort (M5 avait déjà câblé manufacture→besoin comblé→satisfaction ; M8 ferme
  la boucle satisfaction→capacité→collecte) — un pays type passe de 15 % de besoins comblés/
  0 % satisfaction/92 or-mois à l'an 0 à 73-78 % de besoins comblés/83-87 % satisfaction/
  7500-8200 or-mois vers l'an 200-250, tax_mult Laborer montant de 0.86 à 1.00 (plafond) en
  cours de route — le développement manufacturier PAYE littéralement l'impôt.
- **Calibrage** (sweep {9,11,42}×3×250, recherche manuelle 7 points, PAS un optimum global) :
  0.8/0.02/0.03 initial cassait la bande Laborer (seed 11 : 59→47 %) ; 0.25/0.006/0.07
  ramenait Laborer en bande mais régressait l'invariant M3c (0/9→2/9 breach, seed 11) —
  sensibilité NON monotone (bifurcation, pas un gradient, motif M7). Verrouillé à
  `TAX_SAT_COUPLING=0.35 / AI_FISCAL_STEP=0.012 / AI_FISCAL_DEADBAND=0.05` : invariant
  0/9 breach restauré (pic max 246 %), Laborer 55-66 % (seed 9 marginal +2pts, documenté).
- Gate : kill-switches prouvés (`TAX_SAT_COUPLING=0,AI_FISCAL_TARGET=0` → golden pré-M8
  byte-identique) · sweep apparié · `make test` 38/38 (intertrade_demo seul pré-existant,
  ai_demo réparé — fixture, moteur intact) · golden RE-BASELINÉ · determinism/deep/savetest/
  fuzztest verts. Détail complet (mesures, le recalibrage, les découvertes) :
  TROUVAILLES.md « CHANTIER MONNAIE — M8 ».
- **Restes** : distribution de satisfaction inter-pays PAS resserrée autour de 60 % à
  l'échelle du sweep headless (34→43 % moy., σ~37pts quasi inchangé) — le lien manufacture→
  satisfaction→fisc est prouvé sur UN pays développé (ci-dessus), mais dans un monde IA-only
  avec beaucoup de petits pays/hameaux fragiles (guerres, pauvreté chronique), la fiscalité
  reste un levier BORNÉ face à des chocs bien plus lourds — cohérent avec la doctrine
  (jamais un bonus/malus plat forçant la satisfaction), mais le joueur devra le voir en jeu
  réel pour juger si c'est assez visible · banqueroutes Σ EN HAUSSE sur les 3 graines
  (+81/+96/+161 %, CONTRAIREMENT à l'attente « premier levier devrait absorber ») — hypothèse
  mesurée, pas confirmée en détail : relâcher la fiscalité d'un pays déjà en difficulté (sous
  60 % de satisfaction) réduit SON revenu au moment où il en a le plus besoin pour honorer sa
  dette, accélérant potentiellement la bascule vers l'échelle du désespoir M3h/M3g plutôt que
  de la retarder — un futur calibrage pourrait border le relâchement (ex. ne jamais couper
  sous un plancher de revenu vital) si cette tension est jugée trop forte · colonisation
  bidirectionnelle mais dominée par deux fortes baisses (-9/-57/+7 %, vs ±10 % en M3i/M5) —
  la fiscalité qui monte sur les riches concurrence directement l'initiative privée M4-IP
  (le même surplus finance les deux) · UI-MONNAIE dédiée non câblée (readers C2 prêts,
  aucune demande GDScript cette vague).

### M9 — L'EMPRUNT DEMANDÉ + LA COHÉRENCE FISCALE-DETTE DE L'IA (LIVRÉ, 2026-07-16)

**Statut : LIVRÉ (C0 mesuré MIXTE, voir Restes).** Décision joueur : « Emprunts demandé oui,
à faire. Verbe à produire, en diplomatie et en panneau éco […] L'état emprunte d'abord aux
classes. » + sur le contrôleur fiscal M8 : « Les banqueroutes ne sont pas émergentes, elles
sont MAL RÉGLÉES. […] Faut viser 60 % ET du pognon. Si il vise 60 day1 ça marche pas : il cut
ses impôts. »
- [x] **C0 — LA COHÉRENCE FISCALE-DETTE** : `econ_ai_fiscal_slack` (scps_econ.c) borne le
      levier RELÂCHER (jamais DURCIR) du contrôleur C3/M8 par la marge de revenu/solvabilité
      — `AI_FISCAL_REVENUE_FLOOR` (assiette fiscale prouvée, le piège day-1) × la pression de
      dette (`1 - debt/ceiling`) × l'absence de streak d'insolvabilité chronique. Sans marge :
      pas=0, le contrôleur TIENT (jamais négatif). `AI_DEBT_FISCAL_COHERENCE=0` : kill-switch
      EXACT (relax_factor toujours 1.0, golden pré-M9 byte-identique).
- [x] **V1 — EMPRUNTER À UN ORDRE** (`CMD_BORROW_CLASS`, panneau éco) : l'État emprunte à UNE
      classe (Élite/Bourgeois — Laborer/Esclave n'ont pas d'épargne, motif M3c) de son propre
      empire ; la classe NE REFUSE JAMAIS (capacité épuisée ≠ refus). Réutilise le socle M3c
      (`credit_class_borrow_capacity`/`credit_borrow_class`, plafond+tranche M3d). Reader
      façade `scps_country_loan_capacity` (montant max + taux, PAR ordre).
- [x] **V2 — DEMANDER UN EMPRUNT À UN ÉTAT** (`CMD_REQUEST_LOAN`, diplomatie) : le joueur
      sollicite un État étranger DE SON CHOIX (pas l'auto-sélection `pick_lender` de M3c) ;
      celui-ci PEUT REFUSER — value SUBJECTIVE (`ai_consider_offer`/`OFFER_LOAN` : jamais en
      guerre, relation nette positive, liquidité propre du prêteur, confiance — seuil plus bas
      pour un éthos prêteur naturel mercantile/pacifiste). Reader façade en MOTS
      (`scps_country_loan_status` : Aucune demande/Accordé/Refusé — STR_LOAN_*, jamais un
      flottant ; la résolution est SYNCHRONE au drain, aucun état « en cours » persistant).
- [x] **V3 — LES RACHATS À MÉTABOLISATION DISTINCTE** : le rachat M3c (« les Fugger ») reste
      INCHANGÉ ; ce qu'il RAPPORTE au racheteur diffère désormais par archétype — cité-état →
      rancor ALLÉGÉE (influence/vassalité douce) · pacifiste → `faction_lever_apply`/
      FAC_COMMUNAUTAIRE (stabilité) · mercantile → RIEN de plus (son profit PUR est déjà
      l'intérêt annuel uniforme). `RRACHAT_META=0` : kill-switch exact.
- **Piège corrigé (scps_credit.c)** : V1/V2 créditaient `prov[cap_pid].treasury` directement —
  invisible d'`econ_country_gold`/`credit_can_spend`/`credit_line`/`audit_eco`, qui lisent
  TOUS `region[].treasury`, jamais ré-agrégé depuis `prov[]` ailleurs dans le moteur. Corrigé
  via `econ_region_treasury_add` (le SEUL chemin qui tient les deux en phase). V2 utilisait en
  outre `econ_region_rep_province` — l'indirection région EXPLICITEMENT interdite dans un
  chemin joueur (doctrine province, CLAUDE.md) — remplacée par `econ_country_capital_prov` +
  le miroir `ProvinceEconomy.region` (aucun `World*` requis). Détail : TROUVAILLES.md
  « CHANTIER MONNAIE — M9 ».
- Gate : kill-switch prouvé (`AI_DEBT_FISCAL_COHERENCE=0,RRACHAT_META=0` → golden pré-M9
  byte-identique) · sweep apparié {9,11,42}×3×250 · `make test` 38/39 (intertrade_demo seul
  pré-existant) · golden RE-BASELINÉ · determinism/deep/savetest/fuzztest verts.
- **Restes (C0, mesuré MIXTE — STOP PROPRE plutôt que forcer)** : banqueroutes Σ M8→M9
  {341→365, 507→506, 353→312} (seed 9 EMPIRE, 11 quasi inchangé, 42 amélioré ~12 %) —
  l'objectif « effacer la hausse M8 » n'est PAS clairement atteint · colonisation M8→M9
  {112→67, 37→73, 163→147} (seed 9 et 42 DAVANTAGE supprimées, seed 11 très amélioré) — gate
  « pas davantage supprimée » ÉCHOUE sur 2/3 graines. En contrepartie : revenu fiscal Σ an-150
  maintenu/amélioré sur les 3 graines (+0.6 / +47 / +3 %), bande Laborer 50-64 % RESPECTÉE sur
  les 3 (corrige même le dépassement M8 seed 9), invariant 0/9 breach maintenu. Hypothèse non
  confirmée (hors budget, sweep dédié requis) : tenir la fiscalité plutôt que la relâcher
  prolonge une satisfaction/richesse basse qui, via un canal DIFFÉRENT (révoltes, initiative
  privée M4-IP), pourrait desservir exactement ce que C0 cherche à protéger — la tension que
  M8 avait déjà anticipée sans la confirmer.

### M10 — LA CENTRALISATION FISCALE + LE TRANSPORT (v3 : reformulé ; ex-M7, renuméroté une
seconde fois pour M8 LIVRÉ, une troisième fois pour M9 LIVRÉ ci-dessus)
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
