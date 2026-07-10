# SCPS : Conseil, factions, événements et orientations politiques (spec joueur, 2026-07-10)

> Spec de design/équilibrage, verbatim opérationnel. Tous les effets citent une coordonnée ou
> une clé EXISTANTE de scps_tune_list.h ; les coefficients neufs du Conseil y sont déclarés
> AVANT usage (F10/SCPS_TUNE). Sept règles : 3 sièges exactement · les 6 factions sur chaque
> siège · le SIÈGE = domaine mécanique, la FACTION = coût politique · la mission décennale =
> seul mandat temporaire · les événements Conseil existants = seule couche narrative ·
> efficacité = f(K, loyauté, Corruption) exactement · tout coût = part du revenu annuel × IPM
> (JAMAIS de prix nominal permanent). Pas de 4e ressource, pas d'arbre de carrière, pas de
> nouvelle famille d'événements.

## Membrane Corruption
« Corruption » PARTOUT côté joueur — le nom interne `rot` ne paraît jamais (2 fuites à corriger :
l'identité « Corrompu » dit « capté par le rot » ; l'infobulle Corruption parle d'une ponction du
trésor que le moteur n'applique pas). Infobulle recommandée : « Corruption : part de l'appareil
d'État capturée par les factions. Elle réduit la recherche, le rendement de la capitale et la
capacité des services, puis accélère les chutes de loyauté. » Jauge /100, plafond mécanique 85 ;
une concession = +4,5 points avant plafond.

## Les trois sièges (titres génériques, toute faction peut les occuper)
- **Conseiller du Savoir** — production de recherche, canal `council_m(owner,0)` dans la prod
  de tech. Rangs I/II/III : +12 %/+18 %/+24 %. Flavor : « Il ordonne les écoles, les archives et
  les hommes qui savent trop de choses pour rester sans surveillance. »
- **Conseiller du Royaume** — vitesse de promotion sociale, canal `council_m(owner,1)` dans
  `mobility_move`. Rangs : +15 %/+22,5 %/+30 %. Flavor : « Entre le trône et le dernier foyer
  s'étend une chaîne de charges, de faveurs et de refus. Il en tient le registre. »
- **Conseiller des Ouvrages** — vitesse d'expansion des manufactures, canal `council_m(owner,2)`
  dans `BASE_EXPANSION`. Rangs : +20 %/+30 %/+40 %. Flavor : « Il compte les ateliers, les
  routes, les bras disponibles et les raisons pour lesquelles aucun des trois ne suffit jamais. »

## Rangs et coûts (assiette : econ_country_tax_year(cid) × taux × IPM ; prélèvement mensuel = /12)
Rang I ×1,00 · 1,5 % du revenu annuel × IPM · II ×1,50 · 3 % · III ×2,00 · 5 %.
UI : « 3 % du revenu annuel × IPM, actuellement 45 or cette année » (montant courant informatif).
3 conseillers rang III = 15 % du revenu annuel à IPM 1.
Clés : COUNCIL_SAVOIR_BASE 0,12 · COUNCIL_ROYAUME_BASE 0,15 · COUNCIL_OUVRAGES_BASE 0,20 ·
COUNCIL_TIER2_MULT 1,50 · COUNCIL_TIER3_MULT 2,00 · COUNCIL_TIER1/2/3_REVENUE_RATE 0,015/0,030/0,050.

## Toutes les factions sur tous les sièges
Le code limite Savoir→Transgresseur/Légiste, Royaume→Conquérant/Communautaire, Ouvrages→Marchand
(Gardien nulle part) : CETTE RESTRICTION DISPARAÎT. Par siège et génération : mélange DÉTERMINISTE
des 6 factions (seed, pays, siège, génération) → 3 premières → 3 factions DISTINCTES → re-tirage
au renouvellement de génération. Lecture : le SIÈGE dit le +X %, le RANG la force, la FACTION qui
gagne du pouvoir, loyauté+Corruption combien est délivré. Nomination : COUNCIL_HIRE_LEVER 0,10 au
biais + rancœur 0,10×opposition aux opposants (inchangé). RENVOI : cesse de pousser la faction la
plus opposée — ajoute COUNCIL_DISMISS_GRIEF 0,10 à la rancœur de la faction CONGÉDIÉE (petit
lecteur-écrivain de rancœur, aucune coordonnée neuve).

## Efficacité politique (K = capacité administrative positive, Corruption = perte)
`efficacité = clamp(0,70 + 0,03×K + 0,15×loyauté/100 − 0,0035×Corruption, 0,50, 1,15)`
Clés : COUNCIL_EFF_BASE 0,70 · COUNCIL_EFF_K_PER 0,03 · COUNCIL_EFF_LOY_W 0,15 ·
COUNCIL_EFF_CORRUPTION_PER_POINT 0,0035 · COUNCIL_EFF_MIN 0,50 · COUNCIL_EFF_MAX 1,15.
`bonus final du siège = bonus de rang × efficacité`. Exemple : Savoir II, K6, loy 70, Corr 20 →
0,70+0,18+0,105−0,07 = 0,915 → 18 %×0,915 = 16,47 %. L'UI décompose (rang / K / loyauté /
Corruption / efficacité). ⚠ `statecraft_council_apply` doit LIRE `wp->country[cid].K`
(passer WorldProsperity au lieu de recalculer une approximation depuis les bâtiments).
L'efficacité multiplie SEULEMENT la part du conseiller (les effets directs de la Corruption
dans l'économie restent).

## Factions : biais (decay 7 %/an) · rancœur (7 %/an) · capture (~0,28 %/an) EN PARALLÈLE.
Corruption affichée = Σ captures ×100, cap 85 ; la faction affichée = la pire capture.
`faction_concede` : capture +0,045 · biais +0,06 · rancœur opposants 0,06×opposition — une
concession structure presque toute la campagne (97,2 % conservés à 10 ans) : réservée aux choix
où le texte dit que l'État ABANDONNE une prérogative. Une orientation légère n'appelle JAMAIS
faction_concede.

## Noms, maisons, identités
Candidat = PERSONNE + MAISON séparées (« Aveline Vœrn · Maison Vœrn · Conseillère du Savoir,
rang II · Faction : Marchand »). Prénoms : Aldren Corven Edras Isarn Maëlor Odran Orsan Séverac
Solvar Tévran Vaudric Ysarn Althéa Aveline Ilyne Isolde Maëra Mirenne Néris Oriane Téliane Ysilde
Zélie Vésane. Maisons (narratif PUR, zéro mécanique) : Vœrn (sceaux/registres) · Aldric
(comptoirs) · Harmel (ateliers/guildes) · Orlec (prêteurs) · Tessari (magistrats) · Velmor
(écoles/chapitres) · Brask (marches/armes) · Dovric (syndics/communes) · Sarnel (greniers/
domaines) · Corvane (ambassades) · Istrane (mines/forges) · Vaulserre (offices royaux).
Maison ⊥ faction ⊥ siège ⊥ rang ⊥ identité (tirages indépendants).
**IDENTITÉS = PUREMENT NARRATIVES, EFFET MÉCANIQUE 0** (ne pas afficher de faux modificateurs) :
Rigoriste « Chaque exception lui paraît être la première pierre d'une ruine. » · Courtisan « Il
sait qui doit être salué, qui doit être payé et qui doit croire que les deux gestes se valent. » ·
Austère « Son train de maison tient dans deux coffres. Sa reconnaissance aussi. » · Réformateur
« Aucune institution ne lui semble achevée tant qu'il reste possible de la démonter. » · Vétéran
« Il a servi trois règnes et appris à ne confondre aucun d'eux avec l'État. » · Ambitieux « Il
appelle service la distance qui le sépare encore du pouvoir. » · Loyaliste « Il sert la couronne
avec assez de ferveur pour inquiéter celui qui la porte. » · Vénal « Il connaît le prix de chaque
secret, sauf celui du dernier. »

## Mission décennale raccordée au siège responsable (aucun état neuf : déduit du type)
MIS_TECH→Savoir · MIS_CHAIN→Ouvrages · MIS_BUILD savoir/foi→Savoir · MIS_BUILD institutions/
garde/vivres→Royaume · MIS_BUILD commerce→Ouvrages. La responsabilité suit le SIÈGE (successeur
reprend). Bonus récompense = 5 %×(rang−1)×efficacité (III à 115 % = +11,5 %), sur or ET matières
(bases inchangées : 320+60fer Bâtir · 280+120bois Chaîne · 360+40fer Tech). Réussite : loyauté
responsable +5 · échec au remplacement décennal : −10 (l'échec est RÉSOLU avant l'émission de la
mission suivante — corriger le remplacement silencieux). Pas de nouvel événement (la loyauté basse
mène aux trahisons existantes, haute+ancienneté à la succession). Clés :
COUNCIL_MISSION_REWARD_PER_RANK 0,05 · COUNCIL_MISSION_SUCCESS_LOYALTY 5 · _FAILURE_LOYALTY 10.

## Événements Conseil : hooks DYNAMIQUES (deltas K/L/H/agitation/influence/trésor/cicatrices
INCHANGÉS ; l'UI convertit d_treasury_mois → « Paie 42 or » / « Reçoit 21 or », jamais l'assiette)
F = faction RÉELLE du titulaire en faute, Opp(F) = sa plus opposée.
- TRAHISONS : Savoir Taire → renvoi + Opp(F) +0,10 biais · Renvoyer sans bruit → renvoi, rien ·
  Exemple public → renvoi + Opp(F) +0,10. Royaume Purger les places → renvoi + Opp(F) +0,10 ·
  Composer → gardé, F +0,05 · Laisser faire → gardé, CONCESSION à F (+4,5 Corr). Ouvrages
  Poursuivre → renvoi + Opp(F) +0,10 · Négocier remboursement → gardé, F +0,05 · Fermer les
  yeux → gardé, CONCESSION à F. Fallback de nom : « Le conseiller du Savoir/du Royaume/des
  Ouvrages » (jamais « Le marchand »).
- RIVALITÉS R1/R2/R3 : trancher pour un siège = +0,10 biais à la faction RÉELLE de son titulaire
  (rancœur de l'autre par la matrice) ; renvoyer les deux = +0,10 rancœur directe à CHACUNE ;
  aucune capture.
- ALLIANCE A1 : Laisser faire = +0,05 biais aux deux alliées · Contrebalancer = +0,05 au 3e
  siège (vacant ⇒ aucun hook) · Séparer = renvoi du 2e + 0,10 rancœur.
- A2 (candidat du 3e siège) : Accepter leur candidat = choisir dans la liste RÉELLE celui qui
  minimise Σ oppositions aux deux alliées, puis le NOMMER RÉELLEMENT (corrige l'incohérence
  actuelle) · Imposer son choix = ouvrir la liste normale · Vacant = rien.
- CONSPIRATION C1 : Renvoyer les deux = +0,10 rancœur chacune, SANS capture · En sacrifier un =
  +0,10 rancœur à la sienne · CÉDER = double concession (+9 Corr, +0,06 biais chacune) — le SEUL
  choix Conseil qui abandonne une part durable de l'État.
- SUCCESSION : les deux options retirent IMMÉDIATEMENT le titulaire SANS grief de renvoi ;
  Remercier publiquement = faction +0,05 biais · Sans bruit = rien.

## Orientations politiques LÉGÈRES (réversibles, JAMAIS de Corruption ; coût = tax_year × taux ×
IPM, /12 mensuel ; trésor insuffisant le mois ⇒ désactivée et sans effet CE mois ; l'UI affiche
« taux % du revenu annuel × IPM, actuellement N or cette année »)
⚠ RÈGLE TECHNIQUE : jamais tune_set global — le lecteur applique
`valeur = tune_f("CLÉ") × multiplicateur de l'orientation DU PAYS` (le bit décrets existe ;
fournir le pays au lecteur). Toutes les clés DECREE_*/DECISION_* au registre J.
- RATIONS MESURÉES (0,5 %) : FOOD_NEED ×0,95 · POP_R_BASE ×0,97. « Chaque bouche recevra sa
  part. Le greffier précise seulement que la part sera plus petite. » ⊥ PRIMES AUX FOYERS
  (1,5 %) : POP_R_BASE ×1,05 · FOOD_NEED ×1,04. « La couronne célèbre les berceaux. Les
  greniers, eux, comptent déjà les années. » (mutuellement exclusives)
- ÉCOLES SOUTENUES (2 %) : SAVOIR_W_{ELITE,BOURGEOIS,LABORER} ×1,05. « Le maître reçoit une
  bourse, l'élève une place et le trésorier une nouvelle colonne de dépenses. »
- ATELIERS SOUTENUS (2 %) : MANUF_BUILD_COST ×0,95. « La couronne paie les premières pierres.
  Les guildes conserveront volontiers les murs. »
- COMPTOIRS SOUTENUS (1,5 %) : COMMERCE_W_{BOURGEOIS,ELITE} ×1,05. « Un sceau royal sur une
  lettre de change ne la rend pas plus honnête, seulement plus facile à encaisser. »
- CIRCULATION ENCOURAGÉE (0,75 %) : MIG_PACT_FRAC{,_LATE,_ALLY} ×1,10 (entrants ET sortants).
  « Les routes restent ouvertes… » ⊥ FRONTIÈRES FERMÉES (0 or ; contrepartie commerciale) :
  MIG_PACT_* ×0 ET COMMERCE_W_* ×0,95 (ne bloque PAS les réfugiés de guerre/sac). « Les portes
  se ferment aux familles… » — flux d'un pacte A↔B = base × mult(A) × mult(B).
- FÊTES PUBLIQUES (1,5 %) : RÉUTILISE le bit DECREE_MECENAT (aucun enum/état/save neuf) ;
  W_AGITATION_UNREST ×0,95. « La place reçoit des tables, des musiciens et assez de vin pour
  que les griefs parlent moins fort jusqu'au lendemain. »
- LÉGATIONS PERMANENTES (1,5 %) : Statecraft.influence +0,25/mois, plafond 100. « Une table
  bien servie ouvre parfois une frontière que trois armées n'auraient fait que fortifier. »
- LEVÉE ENTRETENUE (0 or ; contrepartie = main-d'œuvre) : plancher de levée
  DECREE_LEVEE_MIN_LEVEL = WH_LEVY_GUERRE ; coût réel affiché = perte de bras calculée moteur.
  « Les mêmes hommes montent la garde chaque saison. Leurs champs apprennent peu à peu à se
  passer d'eux. »
⚠ LA POLITIQUE DE TRIBUT SORT du catalogue (futur réglage de suzeraineté).
Taux-guides : 0,5-0,75 % si contrepartie directe · 1,5 % petit flux/bonus ciblé · 2 % bonus
national de 5 %. Total payant max 10,75 % + Conseil 3×III 15 % = plafond visible 25,75 %.

## Décisions ponctuelles
- AFFRANCHISSEMENT GÉNÉRAL (0 or) : CLASS_SLAVE→CLASS_LABORER · ARR_DEPORTE→ARR_MIGRANT ·
  intégration conservée · diffusion 0,30→1,00 · Communautaire +0,10 biais, aucune capture ·
  UI : âmes libérées + friction projetée (scps_manumit_preview). « Les chaînes tombent en un
  jour. Le pays mettra une génération à décider ce que signifie vivre ensemble après leur chute. »
- AUDIT DES OFFICES : condition Corruption ≥ 20 (DECISION_AUDIT_CORRUPTION_MIN) · coût ponctuel
  25 % du revenu annuel × IPM (DECISION_AUDIT_REVENUE_RATE 0,25) · cooldown 5 ans · effet
  faction_audit : Corruption −20 (DECISION_AUDIT_CORRUPTION_DELTA) · capitale L +0,3 si
  Corr>50, sinon L −0,3. « Les livres sont ouverts. Dans chaque marge, quelqu'un découvre que
  son nom possédait un prix. »

## Interface (cartes)
- CANDIDAT : « Aveline Vœrn / Maison Vœrn, Rigoriste / Conseillère du Savoir, rang II, 47 ans /
  Faction : Marchand / Bonus de rang : recherche +18 % / Efficacité politique prévue : 91,5 % /
  **Bonus final : recherche +16,5 %** / **Coût : 3 % du revenu annuel × IPM** / Actuellement :
  45 or cette année / Retraite estimée : 19 à 26 ans »
- ORIENTATION : « Écoles soutenues / Savoir produit par la population +5 % / {SAVOIR_W_*} ×1,05 /
  **Coût : 2 % du revenu annuel × IPM** / Actuellement : 30 or cette année / Activer »
- MISSION : « Responsable : X, siège, rang / Objectif 4/6 / Échéance an 40, 6 ans restants /
  Récompense de base / Bonus du responsable +4,6 % / Récompense prévue / Réussite loy +5 /
  Échec loy −10 »

## Priorités
P0 : 6 factions × 3 sièges · bonus final exact + faction + prix courant AVANT nomination ·
masquer les modificateurs d'identité non raccordés · personne + maison.
P1 : formule d'efficacité K/loyauté/Corruption (constantes au registre) · renvoi = rancœur à la
faction congédiée · une nomination n'écrase jamais un titulaire sans renvoi.
P2 : hooks de faction dynamiques sur les événements existants · A2 nomme réellement · Céder =
double concession +9 · succession retire immédiatement.
P3 : mission décennale au siège responsable · bonus de récompense rang×efficacité · échec résolu
avant remplacement · loyauté ±5/−10 sans nouvel événement.
P4 : remplacer les grands décrets par les orientations légères · clés SCPS tunes exactes ·
affichage taux + IPM + montant courant · Affranchissement + Audit en décisions ponctuelles.

## Télémétrie de validation (5 mesures)
bonus final moyen des sièges · Corruption moyenne à 100 ans · fréquence des 3 trahisons · taux
d'achèvement des missions décennales · nb moyen d'orientations actives.
