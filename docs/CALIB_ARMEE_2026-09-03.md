# CALIBRAGE ARMÉE — rapport d'équilibrage (2026-09-03)

Lecture de code + dépouillement. **Aucune modification de source.**

## 0. Périmètre lu et mesuré

**Code lu intégralement** : `scps/scps_warhost.c` (435 l.), `scps/scps_army.c` (673 l.)
+ `scps_army.h`, `scps/scps_campaign.h` (328 l.) et le cœur de `scps/scps_campaign.c`
(outils 1-200, défense/siège 220-340, bataille 600-1110, tick 1110-1270),
`scps/scps_diplo.c` §mil_power/CB/trêve/rancune (37-77, 940-1100),
`scps/scps_ai.c` §cible & stratégie de guerre (35-69, 460-535, 1735-1920),
`scps/scps_sim.c` §ordres de campagne (100-260, 830-925, 1335-1360),
`scps/scps_tune_list.h` §J-armée (200-260), `scps/scps_econ.c` §prix de base (335-350)
et §dispatch d'État (4520-4640).

**Journaux lus** : `essai_s512_y200.log` et `temoin_s512_y200.log` **de bout en bout**
(dump PROV compris) ; pour les 18 autres, les blocs `empires vivants → SYNTHÈSE`
(l. ~630-1020 selon le fichier), soit toute la télémétrie militaire, économique
et diplomatique — le reste de ces fichiers est le dump PROV, déjà dépouillé
intégralement par `docs/SWEEP_DOCT_AI_2026-09-02.md`. Aucun filtre n'a servi à
*découvrir* : les nombres cités sont lus à la ligne indiquée.

**Mesures neuves** (binaire `chronicle.exe` du 2026-09-03 10:53, POST-38523b6, 3 runs) :

| run | commande | horizon |
|---|---|---|
| A | `./chronicle.exe 512 1 120 6 12` | 120 ans, défaut |
| B | `./chronicle.exe 512 1 30 6 12` | 30 ans, défaut |
| C | `SCPS_TUNE=REGIMENT_PAY=45 ./chronicle.exe 512 1 120 6 12` | 120 ans, solde ÷2 |

---

## 1. PAR UNITÉ — coût de levée, solde, force, cible de classe, efficacité

### 1.1 Les formules réelles

- **Levée** : 1 paquet = `POP_PER_UNIT` 100 hommes (`scps_army.h:22`) + **100 unités
  d'arme MACRO** de sa catégorie (`scps_warhost.c:106-113`) + un prix en or
  `REGIMENT_PRICE 12 × IPM` par paquet net levé (`scps_warhost.c:412`).
- **Solde mensuelle** (`scps_warhost.c:159-168`) :
  `pay_month(t) = REGIMENT_PRICE(12) × unit_pay_mult(t) × IPM / 13  +  100 × price[arm] / 26`
- **`unit_pay_mult`** (`scps_army.c:150-164`) = `1 + 1.3×tier(gate) + 1.4×élite + 1.0×spécialiste`,
  puis `×0.65` si l'arme est `RES_NONE`.
- **Force** (proxy) = `(1 + discipline) × moral` — la métrique du LOT 5,
  `scps_army.c:67-71` ; `side_power` en bataille lit `Σ count×(1+disc)` puis `^0.85`
  (`scps_campaign.c:639-647`).

### 1.2 Le tableau (22 unités)

`tier` = palier de la tech de gate (`scps_army.c:87-103` ↔ `scps_tech.c:69-224`).
`arm/mois` = `100×prix_base/26` avec les prix de base de `scps_econ.c:342-350`.
`contres` = nombre de `M_BEAT` propres dans `MATRIX` (`scps_army.c:184-237`).

| unité | classe | gate (tier) | pay_mult | or/mois | arme | arm/mois (base) | **solde tot.** | force | contres | **force/solde** |
|---|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| Milice | Lab | — (0) | **0.65** | 0.6 | AUCUNE | **0.0** | **0.6** | 67 | **0** | **112** |
| Piquier | Lab | — (0) | 1.00 | 0.9 | légère 9 | 34.6 | 35.5 | 150 | 2 | 4.2 |
| Lancier | Lab | — (0) | 1.00 | 0.9 | légère 9 | 34.6 | 35.5 | 125 | 2 | 3.5 |
| Épéiste | Lab | — (0) | 1.00 | 0.9 | légère 9 | 34.6 | 35.5 | 145 | 3 | 4.1 |
| **Hallebardier** | Lab | Caserne (**0**) | **1.00** | 0.9 | lourde 14 | 53.8 | 54.7 | **171** | **5** | 3.1 |
| Archer | Lab | — (0) | 1.00 | 0.9 | trait 10 | 38.5 | 39.4 | 84 | 4 | 2.1 |
| Arbalétrier | Lab | — (0) | 1.00 | 0.9 | trait 10 | 38.5 | 39.4 | 128 | 4 | 3.2 |
| Harceleur | Lab | — (0) | 1.00 | 0.9 | trait 10 | 38.5 | 39.4 | 71 | 3 | 1.8 |
| Traqueur | Lab | — (0) | 1.00 | 0.9 | trait 10 | 38.5 | 39.4 | 88 | 3 | 2.2 |
| **Cav. lourde** | **Élite** | — (**0**) | 2.40 | 2.2 | lourde 14 | 53.8 | 56.0 | **169** | **9** | 3.0 |
| Cav. légère | Élite | — (0) | 2.40 | 2.2 | légère 9 | 34.6 | 36.8 | 109 | 6 | 3.0 |
| Arbalétr. lourd | Lab | Taille préc. (1) | 2.30 | 2.1 | trait 10 | 38.5 | 40.6 | 141 | 5 | 3.5 |
| Berserker | Lab | Conscription (1) | 2.30 | 2.1 | lourde 14 | 53.8 | 55.9 | 73 | 4 | **1.3** |
| Lame franche | Lab | Comptoirs (1) | 2.30 | 2.1 | légère 9 | 34.6 | 36.7 | 132 | **0** | 3.6 |
| Cav. de raid | Élite | Comptoirs (1) | 3.70 | 3.4 | légère 9 | 34.6 | 38.0 | 99 | 4 | 2.6 |
| Lancier de choc | Lab | Organisation (2) | 3.60 | 3.3 | lourde 14 | 53.8 | 57.1 | 176 | 5 | 3.1 |
| Garde d'escorte | Lab | Organisation (2) | 3.60 | 3.3 | lourde 14 | 53.8 | 57.1 | **202** | **0** | 3.5 |
| Arquebusier | Lab | Poudrière (2) | 4.60 | 4.2 | feu 16 | 61.5 | 65.7 | 119 | 6 | 1.8 |
| Alchimiste | Lab | Alchimie (2) | 4.60 | 4.2 | nécessaire 34 | 130.8 | 135.0 | 94 | 5 | **0.7** |
| Sorcier (Mage) | Élite | Magie bat. (2) | 6.00 | 5.5 | bâton 30 | 115.4 | 120.9 | 87 | 4 | **0.7** |
| Chaman (G. run.) | Élite | Forge runes (3) | 7.30 | 6.7 | enchantées 46 | 176.9 | 183.6 | 206 | 4 | 1.1 |
| Cav. cuirassée | Élite | Caste mart. (4) | 7.60 | 7.0 | lourde 14 | 53.8 | 60.8 | 194 | 6 | 3.2 |

*(les prix de marché tournent à ~1/3 du prix de base — `essai_s512:631` « FER prix moy
0.8 (base 2.4) » —, donc la colonne « arm/mois » vaut en pratique 10-60 or/mois ; les
RAPPORTS entre unités, eux, ne bougent pas.)*

### 1.3 Ce que le tableau dit

**a) `unit_pay_mult` est arithmétiquement INERTE.** La part OR va de 0.6 à 7.0 or/mois
(facteur 11), la part ARMES de 0 à 177 (facteur ∞) — **97-99 % de la solde d'un
régiment est le prix des 100 armes**, qui ne dépend QUE de la catégorie d'arme
(7 valeurs), pas de la qualité de l'unité. Toute la mission « la solde par type
d'unité » (`scps_army.c:128-164`) déplace au mieux 6 or/mois sur un régiment qui en
coûte 35 à 184. Le seul vrai levier global est `REGIMENT_PAY` (registre J), qui
multiplie les DEUX termes (`scps_warhost.c:314-316`).

**b) MEILLEUR CAS — la MILICE, hors barème.** `RES_NONE` (`scps_army.c:119-121`) ⇒
**zéro arme consommée, zéro arme dans la solde, et zéro gate d'arsenal**. Solde
0.6 or/mois contre 35 pour un piquier : **59× moins cher**, pour une force 2.2× plus
faible. Efficacité 112 contre 4.2 — **27× le meilleur suivant**. Elle ne bat rien
(0 contre), mais elle ne PERD que contre le Berserker : son `side_counter`
(`scps_campaign.c:653-665`) reste ≈ 1.0 face à presque tout. **C'est l'unité la plus
rentable du jeu, et de très loin.** Elle n'est pourtant composée que par le Gardien
(poids 1) et le Communautaire (poids 3) dans `AFF` (`scps_warhost.c:78,80`).

**c) MEILLEUR CAS de combat — le HALLEBARDIER.** Force 171 (2ᵉ du roster derrière la
Garde d'escorte), **5 contres dont les QUATRE cavaleries + l'épéiste**, gate
`TECH_CASERNE` **tier 0** (`scps_tech.c:203`, parent `NONE`) ⇒ `pay_mult = 1.00`.
Il domine strictement le piquier (150) et le lancier (125) **au même prix or**.

**d) MEILLEUR CAS élite — la CAV. LOURDE.** **Aucun gate de tech** (`unit_tech_gate`
défaut `TECH_COUNT`, `scps_army.c:101`), seulement le gate d'élite `>200`
(`scps_warhost.c:213`) : force 169, `pay_mult` 2.40, **9 contres — le plus large
réseau du roster**. Et elle compte dans `army_cav_frac` (`scps_campaign.c:902-910`),
qui pilote la curée : voir §3.4.

**e) PIRES CAS.** Sorcier (0.7) et Alchimiste (0.7) : leur arme coûte 30-34 de base
soit 115-131 or/mois de solde, pour 87-94 de force — **50× le prix d'une milice pour
1.4× la force**. Berserker (1.3) : gate tier 1, arme LOURDE, force 73 — il paie une
arme de hallebardier pour la moitié de sa force. Le Chaman (1.1) est sauvé par sa
force 206 mais reste 5× moins rentable qu'un épéiste.

**f) UNITÉS INERTES par la matrice.** `Milice`, `Lame franche` et `Garde d'escorte`
ont **0 contre propre** (`scps_army.c:236` le dit pour la milice) : elles ne
déclenchent jamais `M_BEAT`, donc n'apportent rien à `side_counter` offensif. Pour
la Garde d'escorte (force 202, la plus haute, `pay_mult` 3.60) c'est un choix de
design assumé (« l'ancre ») ; pour la Lame franche (gate `TECH_COMPTOIRS`, 0 contre,
force 132) c'est une unité sans identité mécanique.

**g) UNITÉS JAMAIS LEVÉES : aucune, structurellement.** Aucune colonne de `AFF`
(`scps_warhost.c:73-81`) n'est nulle. Les plus rares sont Cav. cuirassée et Cav.
lourde (Conquérant seul), Lame franche (Marchand seul), Sorcier/Garde runique
(Transgresseur seul). **La chronique n'imprime aucune ventilation par type d'unité**
— impossible de le confirmer aux journaux : c'est un trou de télémétrie (cf. §5-P11).

---

## 2. TAILLE D'ARMÉE PAR TAILLE DE PAYS — limite de force, jauge, solde vs trésor

### 2.1 Les bornes du modèle

- **Limite de force** (`scps_warhost.c:171-173`) : `FL = 6.0 + 0.7 × n_régions`.
- **Garnison de paix** (`scps_warhost.c:391-392`) : `FL × PEACE_GAR_FRAC[jauge]`,
  `{basse 0.55 · garde 1.00 · guerre 1.20 · masse 1.40}`.
- **Cadence** : guerre `7 × LEVY_MULT` paquets/an, paix `3 × LEVY_MULT` **bornée au
  déficit** (`scps_warhost.c:11-12, 355, 378, 396-397`) ;
  `LEVY_MULT = {0.4, 1.0, 1.6, 2.6}`.
- **Surcharge d'intendance** (`scps_warhost.c:310-312`) :
  `sizemult = 1 + max(0, u/FL − 1) × SOLDE_OVER_K(3.0)` — au double de la limite chaque
  régiment coûte ×4, au triple ×7.
- **Guerre** : `×1.5` sur la solde (`:316`), jauge `×1.25/1.50` (`:304`).

⇒ **fourchettes THÉORIQUES de limite de force** :
cité-état 1-3 rég → **6.7-8.1** · petit pays 4-10 rég → **8.8-13.0** ·
moyen 20-35 rég → **20.0-30.5** · hégémon 50-60 rég → **41.0-48.0**.

### 2.2 Mesuré — AVANT la garde de budget sous le feu (sweep, an 200)

| journal:ligne | pays | rég | FL | rgt | **rgt/FL** | or | flux/mois |
|---|---|---:|---:|---:|---:|---:|---:|
| `essai_s777:781` | Couronne Falwick | 6 | 10.2 | **342** | **33.5×** | 3 670 | **−2.9** |
| `essai_s11:675` | Clans Hobwickis | 10 | 13.0 | **317** | **24.4×** | 4 583 | **−24.8** |
| `temoin_s3333` (§5 A3) | Mécaniste libre | 7 | 10.9 | 167 | 15.3× | — | — |
| `temoin_s11:668` | Clans Hobwickis | 12 | 14.4 | **165** | **11.5×** | 4 266 | **−17.1** |
| `essai_s3333:657` | Adaptatif libre | 1 | 6.7 | 77 | 11.5× | 487 | +11.6 |
| `temoin_s777:772` | Couronne Falwick | 15 | 16.5 | 50 | 3.0× | 10 553 | −2.3 |
| `essai_s512:645` | Ordre Pyxil | 12 | 14.4 | **0** | 0 | 17 807 | +30.3 |

**La preuve que l'armée était GRATUITE** : `Clans Hobwickis`, 317 régiments, ne coûte
que **−24.8 or/mois au flux national**. Même à 1 or/rgt/mois la solde seule devrait
faire −317. ≈ **92 % de l'armée n'était pas payée.** Cause double :
(i) la branche `at_war` ne connaissait aucun plafond, (ii) `paid = fmaxf(0, fminf(pay,
treasury))` (`scps_warhost.c:324`) rend **libre** tout régiment que le trésor ne couvre
pas. Et 16/23 empires à 0 régiment sur `essai_s512:645-706` (capitale orpheline).

### 2.3 Mesuré — APRÈS (run A, seed 512, an 120, binaire courant)

| pays (`/runA:562-586`) | rég | FL | rgt | **rgt/FL** | or | flux/mois |
|---|---:|---:|---:|---:|---:|---:|
| Clans Tikexis | 55 | 44.5 | 30 | 0.67 | 28 287 | +63.4 |
| Ligue Kargrakel | 32 | 28.4 | 19 | 0.67 | 10 515 | −22.6 |
| Ligue Thrumdinel | 13 | 15.1 | 8 | 0.53 | 1 796 | +5.5 |
| **Horde Brewick** | 7 | 10.9 | **22** | **2.02** | 7 347 | **−428.4** |
| Clans Zenilel | 3 | 8.1 | 8 | 0.99 | 169 | +1.9 |
| Ordre Caelwic | 1 | 6.7 | 1 | 0.15 | **−43** | 0.0 |
| Métallurgiste libre | 1 | 6.7 | 2 | 0.30 | 500 | 0.0 |

Et **an 30** (run B `/runB:343-357`) : 26 rég → 14 rgt (0.58×FL) · 20 rég → 13 (0.65) ·
5 rég → 2 (0.21) · **4 rég → 0 rgt** (Ligue Thrumdinel, or 156, −15.7/mois) ·
3 rég → 6 (0.74) · 1 rég → 1 (0.15).

**Fourchettes retenues (post-correctif)** :

| taille | rég | FL | **rgt observés** | rgt/FL |
|---|---|---:|---|---:|
| cité-état / micro-État | 1-3 | 6.7-8.1 | **0-8** | 0-1.0 |
| petit pays | 4-13 | 8.8-15.1 | **0-22** | 0-2.0 |
| moyen | 20-35 | 20-30.5 | **13-19** | 0.6-0.7 |
| hégémon | 50-60 | 41-48 | **30-48** | 0.67-1.0 |

**Le plafond résiduel est ×2 la limite de force, pas ×33.** L'anomalie A3 est éteinte.
Ce qui reste : rien n'INTERDIT le dépassement, seul le trésor de la capitale le paie
(Horde Brewick, 22 rgt pour FL 10.9, `sizemult = 1+1.02×3 = 4.06` → **−428 or/mois**,
soit 5,4× le flux moyen d'empire). Cf. §4.1.

### 2.4 Le budget militaire dans le temps (`flux décomposé`, or/mois/empire)

| horizon | source | taxes | soldes | **part** |
|---|---|---:|---:|---:|
| an 30 | `/runB:430` | +30.5 | **−14.6** | **48 %** |
| an 120 | `/runA:657` | +328.5 | −82.3 | **25 %** |
| an 120, `REGIMENT_PAY=45` | `/runC:702` | +281.5 | −72.5 | 26 % |
| an 200 essai s512 | `essai_s512:782` | +383.1 | −40.7 | **11 %** |
| an 200 témoin s512 | `temoin_s512:767` | +1 619.3 | −81.4 | **5 %** |
| an 200 essai s777 | `essai_s777:864` | +2 658.1 | −274.7 | 10 % |

**La cible W-GUERRE-3 (10-15 % des dépenses d'État, `scps_tune_list.h:225-229`) n'est
tenue qu'au milieu de la courbe.** À l'an 30 l'armée mange **la moitié** de la fiscalité
(c'est ce qui laisse un pays de 4 régions à 0 régiment) ; à l'an 200 elle tombe à
**5-11 %** — l'armée devient une ligne de dépense négligeable pour un hégémon.

### 2.5 Le probe REGIMENT_PAY (run C) — l'élasticité

`REGIMENT_PAY 90 → 45` (solde ÷2), même graine, 120 ans :

| mesure | run A (90) | run C (45) | Δ |
|---|---:|---:|---:|
| armée finale (Σ monde) | 244 | **257** | +5 % |
| rgt du 1er empire | 30 (55 rég) | 36 (50 rég) | **+20 %** |
| rgt du 2ᵉ | 19 (32 rég) | 31 (35 rég) | **+63 %** |
| **soldes or/mois/empire** | −82.3 | **−72.5** | **−12 % seulement** |
| guerres/120 ans | 43 | 32 | −26 % |
| batailles | 136 | 95 | −30 % |
| régions réduites | 34 | 16 | **−53 %** |
| pays absorbés | 4 | 1 | −75 % |

**Diviser le prix par 2 ne divise pas la dépense par 2 : elle ne baisse que de 12 %,
parce que les armées grossissent jusqu'à re-remplir le budget.** ⇒ **la contrainte
qui borne l'armée est le TRÉSOR, pas la limite de force.** `SOLDE_OVER_K` ne
« freine » pas le doomstack, il en fixe seulement le prix — et ce prix est payé par
une seule province (§4.1).

Effet de bord contre-intuitif : une armée moins chère produit **moins** de conquêtes
(−53 % de régions réduites). Les mondes divergent dès l'an 2, donc c'est indicatif,
pas causal ; l'hypothèse plausible est la symétrie (tout le monde grossit, personne
ne passe le seuil `BT_ATK_RATIO 1.2` de `scps_sim.c:222`, qui compare des RÉSERVES).

### 2.6 RISQUE — l'assiégé ruiné qui ne se défend plus : chiffré

`pay_starved = base_pay>0 && treasury_capitale < base_pay × 0.25` (`scps_warhost.c:340`)
coupe désormais la levée **aussi en guerre** (`:379`). Combien de pays en meurent ?

- an 120, run A : **0 pays sur 7** à 0 régiment (`/runA:562-586`).
- an 30, run B : **1 pays sur 6** (Ligue Thrumdinel, 4 rég, or 156, `/runB:353-354`).
- an 200, post-correctif (TROUVAILLES §Preuves appariées) : `essai_s512` **1/7**,
  `essai_s777` **0/6**, `essai_s90` **0/5**, `temoin_s90` **1/7**.

⇒ **le risque se matérialise sur ~1 pays sur 6-7, uniquement des micro-États pauvres**,
et l'unique cas résiduel de `essai_s512` est de toute façon **gaté par l'arsenal**
(Ligue Thrumdinel, stock Bétail/Laine/Poisson/Céréales, aucune arme —
`essai_s512:674-676`), pas par la garde de budget. **Le correctif ne désarme
personne qui pouvait s'armer.** Une seule contre-mesure manque : un pays sans armes
ne peut RIEN lever, alors que la Milice (`RES_NONE`) existe — cf. §5-P4.

**Guerres qui s'enlisent : non.** `essai_s512:110` 87 guerres/200 ans avec
`AI_WAR_CAP = 3` paires simultanées (`scps_ai.c:1881`) ⇒ **durée moyenne ≈ 3×200/87 =
6,9 ans**, sous le seuil d'épuisement `AI_WAR_EXHAUST = 10 ans`. Trêve d'après-guerre
`3 ans + 1 an/an de guerre` (`scps_diplo.c:37-39`) ⇒ ~10 ans de répit. Le cycle
guerre 7 ans / paix 10 ans est sain. Fourchette du corpus : **13 guerres** (`temoin_s777`)
à **114** (`temoin_s11`, §2.1 du dépouillement) sur 200 ans.

---

## 3. BATAILLE, ATTRITION, SIÈGE

### 3.1 L'issue selon le ratio de forces — la loi

`bt_day` (`scps_campaign.c:1004-1084`), cycle de 5 jours (3 choc + 2 accalmie) :

- `pA = side_power(A)^0.85 × terrain × contre^0.6`, aléa journalier `×[0.85,1.15]` (`:1019`).
- Perte de réserve de B par jour de choc : `resB0 × BT_DMG_K(0.057) × 2pA/(pA+pB)` (`:1023`).
- Récupération en accalmie : `1.5 %/j` (+0.7 % chez soi), 2 j/cycle (`:620, 1044-1047`).
- Rupture sous **20 %** de la réserve d'ouverture (`BT_RUPTURE`, `:619, 1040-1041`),
  jamais avant `CHOC_ROUNDS_BONUS = 2` chocs livrés (`:1039`).

**Bilan par cycle** (perte nette de la réserve de B) :

| pA/pB | perte B/cycle | perte A/cycle | cycles → rupture B | cycles → rupture A | **durée** |
|---:|---:|---:|---:|---:|---:|
| 1.0 | 14.1 % | 14.1 % | 5.7 | 5.7 | ~28 j (pile ou face) |
| 1.2 | 15.7 % | 12.6 % | 5.1 | 6.3 | ~25 j (A gagne, de peu) |
| 1.5 | 17.5 % | 10.7 % | 4.6 | 7.5 | ~23 j (A gagne) |
| 2.0 | 19.8 % | 8.0 % | 4.0 | 10.0 | ~20 j (A gagne net) |

`side_power` étant en `^0.85`, un avantage NUMÉRIQUE de 2:1 ne donne que
`2^0.85 = 1.79` de puissance. ⇒ **seuil pratique de victoire fiable : ~1,3:1 en
effectifs** (au-delà de l'aléa ±15 %). Cohérent avec la durée mesurée :
**16-20 jours par bataille** dans les 20 journaux + les 3 runs.

**Fait dominant : la bataille n'a qu'UNE issue.**

| journal | livrées | déroutes | **% déroute** | décrochages | nuls | renforts |
|---|---:|---:|---:|---:|---:|---:|
| `essai_s512:801` | 186 | 184 | **99 %** | 2 | 0 | 3 |
| `temoin_s512:786` | 143 | 140 | **98 %** | 3 | 0 | 27 |
| `essai_s11:735` | 264 | 258 | **98 %** | 6 | 0 | 21 |
| `temoin_s11:728` | 274 | 268 | **98 %** | 6 | 0 | 45 |
| `essai_s777:883` | 101 | 100 | **99 %** | 1 | 0 | 0 |
| `essai_s60:888` | 496 | 493 | **99 %** | 3 | 0 | **256** |
| `/runA:648` | 136 | 132 | **97 %** | 4 | 0 | 7 |
| `/runC:721` | 95 | 95 | **100 %** | 0 | 0 | 7 |

**0 nul sur ~1 700 batailles** (`BT_MAX_JOURS = 120` n'est jamais atteint : les batailles
durent 16-20 j) et **1-3 % de décrochages**. La raison est arithmétique :
`BT_DECROCHE = 0.22` (`scps_campaign.c:1054`) contre `BT_RUPTURE = 0.20` — la fenêtre
de décrochage fait **2 points de large**, et elle n'est testée **qu'un jour sur cinq**
(`if (ph==BT_CHOC_J)`, `:1048`). Une armée qui descend sous 0.22 passe sous 0.20 dans
le même cycle de choc et **rompt avant d'avoir pu décrocher**. Le « retrait en ordre »
est de fait mort.

### 3.2 Le terrain — écrasé par son propre clamp

`bt_terrainA` (`scps_campaign.c:753-771`) lit `terrain_combat_bonus` (`scps_army.c:613-622` :
montagne **1.20**, jungle 1.15, forêt 1.12, marais 1.10, collines 1.05, plaine 1.00),
puis applique `adv = fminf(adv, 1 + BT_DEF_EDGE(0.10))`.

⇒ **montagne = jungle = forêt = marais = 1.10.** Seules « collines 1.05 » et
« plaine 1.00 » survivent. **La table de terrain de combat est morte à 4/6.**
Elle n'est lue en entier que par `resolve_battle` (`scps_army.c:392-466`), que la
campagne n'utilise plus. La rivière non pontée (`RIVER_COMBAT_EDGE 1.25`,
`scps_campaign.c:61`) est écrasée par le même clamp.

Par comparaison le CONTRE vaut jusqu'à `2^0.6 = 1.52` (`CTR_BITE 0.6`, `:1015-1016`) —
**5× le terrain**. Le pierre-feuille-ciseaux prime largement, ce qui est le contrat,
mais le sol ne pèse plus rien.

### 3.3 Attrition de marche par biome — calculée

`army_step_days = 12 / (v × f)` (`scps_army.c:556`), `v` = mouvement de l'unité **la
plus lente** (`:538-547`) ; `f = base × (1 − 0.55×hauteur)` (`:505-523`) ;
`/1.6` sur route, `×1.8` au franchissement de rivière (`:492-493, 557-558`).
Perte = `1 − (1−taux)^jours` (`:567-568`), taux de `march_attrition_rate` (`:525-536`).

Vitesses du roster : **1** (Arbalétrier lourd) · **2** (Piquier, Arbalétrier,
Hallebardier, Arquebusier, Lancier de choc, Garde d'escorte) · 3-5 (le gros) ·
**8** (Cav. légère, Cav. de raid).

| biome (h≈0.2, sauf montagne h≈0.8) | taux/j | jours à v=2 | **perte v=2** | jours à v=8 | perte v=8 |
|---|---:|---:|---:|---:|---:|
| Plaine / prairie / steppe | 0.6 % | 5.6 | **3.3 %** | 1.4 | 0.8 % |
| Littoral / terres sèches | 0.6-1.0 % | 6.7 | 4.0-6.5 % | 1.7 | 1.0-1.7 % |
| Collines / hauts plateaux | 1.2 % | 9.6 | **10.9 %** | 2.4 | 2.8 % |
| Bois / forêt | 0.6 % | 13.4 | **7.7 %** | 3.4 | 2.0 % |
| Désert | 3.0 % | 9.5 | **25.1 %** | 2.4 | 7.0 % |
| Marais / tourbière / mangrove | 2.0 % | 16.7 | **28.5 %** | 4.2 | 8.2 % |
| Jungle | 2.5 % | 19.0 | **38.2 %** | 4.8 | 11.4 % |
| **Montagnes (h 0.8)** | 1.2 % | 32.4 | **32.4 %** | 8.1 | 9.3 % |

À `v = 1` (une seule ligne d'Arbalétriers lourds dans le corps) tout double :
**jungle 62 %, montagne 55 %, marais 50 % par case franchie.**

⇒ **L'attrition de marche est le vrai avantage de la cavalerie : 4× moins de pertes
sur tout terrain, jusqu'à 5× en jungle.** Une route la divise encore par ~1,6 en durée
(soit −35 à −40 % de pertes) — `ROUTE_SPEEDUP` (`scps_army.c:493`).

### 3.4 La poursuite — le multiplicateur cavalerie

`bt_rout` (`scps_campaign.c:926-943`) :
`P = 0.06 + 0.04×vfrac + CAV_PURSUIT(0.45)×cav + CTR_PURSUIT(0.30)×max(0,contre−1)`,
plafond `CUREE_CAP(0.22) + CAV_CUREE_CAP(0.40)×cav`, −0.04 si le sol couvre la fuite.

| composition du vainqueur | P typique | plafond | **% du vaincu tué** |
|---|---:|---:|---:|
| infanterie pure (cav = 0) | 0.08-0.12 | 0.22 | **8-12 %** |
| moitié montée (cav = 0.5) | 0.31-0.35 | 0.42 | **31-35 %** |
| cavalerie pure (cav = 1) | 0.55-0.62 | 0.62 | **55-62 %** |

⇒ **la cavalerie multiplie la curée par 5 à 6.** C'est le levier le plus violent du
module et il se cumule avec §3.3 (elle marche presque sans perte) et avec le fait que
la Cav. lourde n'a **aucun gate de tech** (§1.3-d).

### 3.5 Choc contre poursuite — la cible 2-5× n'est tenue qu'à mi-course

`BT_CHOC_MORTS = 0.006` (`scps_campaign.c:622, 1026-1027`) : par jour de choc,
`effectif × 0.006 × 2pA/tot` paquets. Sur ~12 jours de choc c'est **7,2 % de
l'effectif** — indépendant de la taille. Mais `dueB = (long)lossB − (long)prev_lossB`
(`:1029-1030`) ne tue que des paquets ENTIERS : à **5 paquets**, il faut
`1/(5×0.006) = 33 jours de choc` pour tuer 1 paquet, alors qu'une bataille en dure 12.

| source | morts CHOC | morts POURSUITE | **ratio** | cible |
|---|---:|---:|---:|---|
| `/runB:449` (an 30, petites armées) | **0** | 2 100 | **∞** | 2-5 |
| `/runC:721` (an 120, pay 45) | 400 | 9 600 | **24.0×** | 2-5 |
| `essai_s512:801` | 2 600 | 20 100 | 7.7× | 2-5 |
| `essai_s60:888` | 7 300 | 45 600 | 6.2× | 2-5 |
| `essai_s11:735` | 4 500 | 25 900 | 5.8× | 2-5 |
| `temoin_s512:786` | 3 400 | 16 800 | **4.9×** | ✔ |
| `temoin_s11:728` | 6 100 | 27 300 | **4.5×** | ✔ |
| `/runA:648` (an 120) | 3 800 | 14 200 | **3.7×** | ✔ |
| `essai_s777:883` | 5 300 | 11 600 | **2.2×** | ✔ |

⇒ **hors du régime « grandes armées d'empire », le choc ne tue personne.** Les
anomalies A13 (34,3× / 23,1×) et les 24× du run C sont le même artefact d'échelle,
pas une dérive de calibrage. En dessous de ~25 paquets, **toute** la mortalité vient
de la poursuite.

### 3.6 Le siège — tout est au plafond

`siege_days = (45 + 60×def + 30×food_months) × terrain`, borné `[14, 730]`
(`scps_army.c:592-596, 624-635`), avec
`def = 1 + 0.25×n_bld + tier_capitale + 0.05×H_coerc` (`scps_campaign.c:223-237`,
`capitale_defense(tier) = tier`, `scps_labor.c:60`) et
`food_months = food_sat × 12` (`scps_campaign.c:238-243`).

| place | def | food (mois) | brut | ×terrain | **jours réels** |
|---|---:|---:|---:|---|---:|
| province vacante | 0 | — | — | — | **14** |
| hameau T1, 2 bâtiments, food 0.5 | 2.5 | 6 | 375 | 1.0-1.5 | **375-562** |
| province T1, 4 bâtiments, food 0.7 | 3.0 | 8.4 | 477 | 1.0-2.7 | **477-730 (cap)** |
| capitale T4, 12 bâtiments, food 0.9, H 6 | 8.3 | 10.8 | 867 | ≥1.0 | **730 (cap)** |

⇒ **quasiment TOUTE place défendue et nourrie est au plafond des 2 ans.** Le terme
« vivres » à lui seul vaut jusqu'à **+360 jours** — plus que la fortification
(`60×def`) pour toute province sous 6 niveaux de défense. La conquête ne peut donc pas
passer par l'investissement : elle passe par `bt_press_siege`, qui **borne le compte à
`BT_RELIEF_FALL = 30 jours`** après une bataille gagnée (`scps_campaign.c:851-859`).

**Vérification empirique — le taux de conversion bataille → prise :**

| journal | batailles | régions réduites | **prises/bataille** |
|---|---:|---:|---:|
| `essai_s512:758,801` | 186 | 45 | 0.24 |
| `temoin_s512:743,786` | 143 | 39 | 0.27 |
| `temoin_s11:736,728` | 274 | 92 | 0.34 |
| `essai_s60:896,888` | 496 | 79 | 0.16 |
| `essai_s777:840,883` | 101 | 29 | 0.29 |
| `/runA:634,648` | 136 | 34 | 0.25 |

**≈ 1 prise pour 4 batailles, jamais 1 prise pour X jours de siège.** Le siège, dans
sa formule propre, est un décor : sa seule variable vivante est `BT_RELIEF_FALL`.
`DEF_PER_H` (registre J, 0.05) agit sur `def`, donc sur `full_days`, donc sur **rien**
tant que `fminf(full, 30)` est le chemin réel.

### 3.7 Qui gagne — récurrences du corpus

- Le 1er empire finit à **49-59 régions** (`temoin_s512:668` 49 · `essai_s60:827` 59 ·
  `essai_s777:771` 54) et **détient 18-21 % des terres**.
- Il est **TOUJOURS « CRAQUÉ »** : `hégémon (A5) ... Stabilité plancher 5/6/7/18 —
  CRAQUÉ` dans les 20 journaux et les 3 runs (`essai_s512:768`, `temoin_s512:753`,
  `essai_s777:850`, `temoin_s11:746`, `/runA:641`).
- **Provinces transférées à la paix : 113-335** contre **16-92 régions réduites par les
  armées**. Le sol change de main **3 à 7× plus par le RÈGLEMENT que par les armes**
  (`essai_s512:788-789` : 335 transférées vs 45 réduites).
- **Occupations levées : 1-8 pour 16-45 posées** (`essai_s512:789` 45/2 ·
  `/runA:709` 34/8). La libération par les armes existe mais reste marginale.
- **Ralliements : 22-166 pour 95-496 batailles** — soit **1 armée sur 3 déroutées se
  reforme** (`L2`, `scps_campaign.c:951-962`).
- **Renforts (marche au canon) : 0 à 256** pour un nombre de batailles comparable
  (`essai_s777:883` 0/101 vs `essai_s60:888` 256/496). Variance ×∞ : `bt_reinforce`
  (`scps_campaign.c:979-1001`) exige un allié/vassal **adjacent ET libre** — le monde
  qui a 4-5 pactes en tire 256, celui qui en a 1 en tire 0.

---

## 4. TRUCS « OP » ET BOUCLES

### 4.1 [MAJEUR] L'État paie sa solde depuis UNE SEULE province

`warhost_tick` débite `econ->prov[crpp].treasury` où `crpp = econ_region_rep_province(
econ, région-de-la-capitale)` (`scps_warhost.c:296-327`), et le prix de recrutement
au même endroit (`:408-420`). Or **le trésor est PROVINCE-OWNED** : `econ_country_gold`
= « Σ trésor des provinces » (`scps_econ.h:433`, `scps_econ.c:3722`), les taxes se
collectent province par province (`scps_econ.c:4521`), et **il n'existe aucune caisse
nationale** — seule la frappe est créditée à la capitale (`scps_econ.c:5628-5758`).

⇒ **la solde et le prix de recrutement sont bornés par la caisse d'UNE province, pas
par le trésor du pays.** `paid = fmaxf(0, fminf(pay, treasury))` (`:324`) : dès que
cette province est à sec, **tout le reste de l'armée est gratuit**.

C'est le mécanisme derrière `Clans Hobwickis` : **317 régiments pour −24,8 or/mois**
(`essai_s11:675`) et `Couronne Falwick` : **342 régiments pour −2,9 or/mois**
(`essai_s777:781`) — 92 à 99 % de la masse salariale jamais versée. La garde de budget
de 38523b6 empêche désormais d'EMPILER dans cet état, mais **le trou de paiement est
intact** : `sizemult` (le frein anti-doomstack, `:310-312`) ne peut mordre que sur
l'argent d'une seule tuile.

### 4.2 [MAJEUR] Le corps au front n'entre pas dans le pool de recrutement

`army_class_free(a, econ, cid, cl) = Σ strata[cl].pop(pays) − a->pop_by_class_in_army[cl]`
(`scps_army.c:317-323`) — `a` étant **le seul `ArmyState` passé**. Le warhost passe
`&h->army[cid]` ; or `campaign_order`/`campaign_raise` font
`army_merge_into(&a->force, src_force)`, un **TRANSFERT** qui vide le host
(`scps_campaign.c:295-297, 322`). Host à 0 affecté ⇒ **le pays peut relever
l'intégralité de sa population une seconde fois.**

**Quantification** : le gate mordant n'est pas le pool commun (Clans Tikexis :
77 600 laboureurs = 776 paquets, contre 30 régiments) mais **le gate d'ÉLITE**
(`scps_warhost.c:213` et `army_can_recruit`) : Clans Tikexis 3 100 élites =
**31 paquets de cavalerie** — exactement l'ordre de grandeur des 30 régiments observés.
Le doublement du pool d'élite par « corps parti » est donc **le canal actif de
sur-levée de cavalerie**, l'unité la plus OP du roster (§1.3-d, §3.4).
La cadence (`7×LEVY_MULT`/an, soit 2,8 à 18,2 paquets/an) est le seul autre frein.

### 4.3 [MAJEUR] Le doomstack n'est pas interdit, seulement facturé

La branche de guerre (`scps_warhost.c:377-382`) ne connaît AUCUN plafond de taille :
ni `warhost_force_limit`, ni la garnison, seulement `pay_starved`. Comparer à la paix
(`:391-402`), qui borne au déficit vers la garnison ET dégraisse la moitié de
l'excédent par an. Résultat mesuré post-correctif : **Horde Brewick, 22 rgt pour
FL 10.9 = 2,02×, −428,4 or/mois** (`/runA:568-569`), soit **5,4× le flux moyen
d'empire** de ce run. Le run C prouve que le frein-par-le-prix est une illusion : ÷2
sur le prix ⇒ +20 à +63 % de régiments pour −12 % de dépense (§2.5).

### 4.4 Le PILLAGE ne finance rien

`SIEGE_LOOT_FRAC = 0.25` de la production mensuelle en siège, `PILLAGE_INCOME_FRAC =
0.20` du revenu annuel de la victime au sac (`scps_tune_list.h:242, 249`), avec un
cooldown `pillage_cd` d'environ 5 ans.

| journal | pillages | or pris | sur cible | **or/an (monde)** | revenu d'État /an/empire |
|---|---:|---:|---:|---:|---:|
| `essai_s512:759-760` | 41 | 8 078 + 20 965 | 73 % | **145** | 2 890 (`:783`) |
| `temoin_s512:744-745` | 25 | 14 998 + 430 | 78 % | **77** | 6 085 (`:768`) |
| `essai_s777:841-842` | 28 | 10 080 + 1 596 | 43 % | **58** | 10 213 (`:865`) |
| `essai_s60:897-898` | 67 | 67 333 + 12 939 | 44 % | **401** | — |
| `/runA:637-638` | 27 | 22 948 + 3 077 | 88 % | **217** | 1 532 (`:659`) |

**Le pillage rapporte 0,3 à 5 % du revenu d'État d'UN empire, réparti sur tout le
monde.** Le taux de captation (43-88 %) montre que le mécanisme fonctionne : c'est
l'ASSIETTE (20 % d'un revenu annuel de micro-État) qui est dérisoire. La guerre ne se
finance pas ; elle ne paie que par le transfert de terres au règlement (§3.7).

### 4.5 Rancune / CB — la boucle de revanche perpétuelle

`rancor[perdant][vainqueur] += 1.0` par province perdue (+1.0 si la prise fut
illégitime), décroissance `1/(10×365)` par jour soit **−0,1/an**, seuil de casus belli
territorial **0,75** (`scps_diplo.c:72-77, 1067`). Poids dans le choix de cible :
`AI_RANCOR_W = 3.0` (`scps_ai.c:60, 514`), sur un score où `rel.threat` vaut typiquement
1-10.

⇒ **1 province perdue = 2,5 ans de CB ; 3 provinces = 22,5 ans ; 10 provinces = 92 ans.**
La trêve, elle, plafonne à 12 ans (`TRUCE_MAX`, `scps_diplo.c:39`). **Au-delà de
~2 provinces perdues, la rancune survit à toutes les trêves : la paire se rebat
indéfiniment.** C'est mécaniquement la source des 87-114 guerres/200 ans des graines
512 et 11, et de la stabilité « CRAQUÉE » systématique de l'hégémon (§3.7).

Le CB de **subjugation** est l'autre boucle : `mil_power(a) > 1.6×mil_power(b) + 1`
(`scps_diplo.c:1078`) — toujours vrai pour un hégémon face à un micro-État.
17-21 guerres de subjugation par sim (`essai_s512:749`, `temoin_s512:734`).

### 4.6 Vassaux — 10 à 22 protectorats, ~0 effet militaire

`suzeraineté : 0 servage · 22 protectorat · 10 concordat` (`temoin_s11:724`),
`13 protectorat` (`essai_s512:746`), `10 protectorat · 8 concordat` (`essai_s777:828`).
Le seul canal militaire du vassal est `bt_reinforce` : **UN corps allié/vassal adjacent
par camp et par bataille** (`scps_campaign.c:979-1001`, `if (*slot>=0) continue`).
Aucun tribut de troupes, aucun appel à la guerre. Avec 22 protectorats, l'apport
militaire est nul dans 0 à 3 batailles sur 101 (`essai_s777:883` : **0 renfort**).
Le vassal est un objet fiscal, pas militaire.

### 4.7 Mercenaires / levée en masse / Ban — ce qui existe et ce qui manque

- **Levée en masse** (`WH_LEVY_MASSE`) : cadence `×2.6`, solde `×1.5`, garnison de paix
  `×1.40`, et **coût politique réel** : `coercion += 0.08/an` à la capitale
  (`scps_warhost.c:357-363`). Le mécanisme du « Ban » est donc déjà là — mais c'est
  une jauge à 4 crans (`scps_warhost.c:134-146`), pas un verbe daté.
- **Mercenaires** : `U_LAME_FRANCHE` est décrit « soldé en OR » (`scps_army.h:42`) mais
  **consomme les mêmes 100 armes légères que le piquier** (`scps_army.c:109`) et suit
  exactement le même chemin de levée. **Il n'existe aucune mécanique de mercenariat**
  (pas de recrutement instantané, pas de solde majorée, pas de désertion à l'impayé).
- **Milice** : la seule unité `RES_NONE` du roster — le vrai « ban » potentiel (§1.3-b)
  — n'est jamais le plancher de secours : `wh_levy_batch` retombe sur
  Piquier/Épéiste/Archer (`scps_warhost.c:216`) puis Piquier seul (`:225`), **tous
  gatés sur l'arsenal**. Un pays sans armes ne lève **rien**.

### 4.8 Marine — éteinte par interrupteur, mais la télémétrie ment

`NAVY_COMBAT_ON = 0` par défaut (`scps_tune_list.h:703`) coupe la construction de
coques (`scps_sim.c:1348-1356`), la course et l'interception (`scps_navy.c:287, 417`,
`scps_campaign.c:519, 571`). C'est une **décision**, pas un bug. Conséquence :
`0 coque(s) · 0 bataille(s) navale(s) · 0 interception(s) · 0 paquet(s) noyés` dans
**les 20 journaux et les 3 runs**, alors que **37 à 79 traversées** ont lieu par sim
(`essai_s512:761`, `/runA:636`, `essai_s60:899`). La ligne de synthèse
`la mer ... 0 fournitures consommées (NE doit plus être zéro)` (`essai_s512:811`)
est une assertion **périmée** qui annonce un défaut inexistant.

### 4.9 Petits trucs

- **`WH_GARRISON_UNITS` (4.0, `scps_warhost.c:15`) est mort** — plus référencé que dans
  un commentaire (`:385`). Constante fantôme.
- **`resolve_battle` (`scps_army.c:392-466`) est mort** en jeu : la campagne utilise
  `bt_day`. Toute la table `terrain_combat_bonus` complète, le débordement de flanc
  (`ARM_FLANK_GAP`), le d20 (`ARM_THRESHOLD 12`) et `PURSUIT_KILL` ne servent plus
  qu'aux bancs.
- **`nreg == 0 → continue`** (`scps_warhost.c:275`) : un pays qui perd toutes ses
  régions garde son armée en réserve sans la payer.
- **`mil_stock = units × 8`** (`:432`), lu par `diplo_mil_power` via
  `gear = 1.8×(1 − 1/(1+kit×0.03))` (`scps_diplo.c:970`) : **saturé dès ~40 régiments**
  (gear 1.35 sur un max de 1.8). Au-delà, doubler l'armée n'augmente plus la puissance
  diplomatique perçue — d'où les « armée 14 (342 rgt) » de `essai_s777:781`.
- **`√pop × 0.04`** domine `diplo_mil_power` (`:971`) : la POPULATION, pas l'armée,
  décide qui l'IA ose attaquer (`AI_ARMY_MARGIN 0.75`, `scps_ai.c:35, 493`).

---

## 5. PROPOSITIONS CHIFFRÉES — classées par impact

Aucune n'est appliquée. `[T]` = tunable existant · `[C]` = changement de code.

### P1 — Plafonner la levée de GUERRE à un multiple de la limite de force `[C]`
**Site** : `scps/scps_warhost.c:377-382`.
**Constat** : la branche `at_war` n'a aucun plafond de taille ; il reste des pays à
2,02× leur limite (`/runA:568`, −428 or/mois) et le run C prouve que le prix ne freine
pas (§2.5).
**Proposition** : `if (cur >= warhost_force_limit(nreg) * WAR_GAR_FRAC) batch = 0;`
avec `WAR_GAR_FRAC = 2.0` (la guerre autorise le DOUBLE de la garnison de paix ;
`PEACE_GAR_FRAC[GUERRE] = 1.20` aujourd'hui, `:391`). Registre J.
**Effet attendu** : plafond dur 13-96 rgt selon la taille ; supprime en un geste
le canal §4.2 (le corps parti ne peut plus faire dépasser le host).
**Risque** : faible. Symétrique du garde-fou de paix déjà en place. Golden bouge.

### P2 — Migrer `SOLDE_*` et les 4 clés de bataille non enregistrées au registre J `[C]`
**Sites** : `scps/scps_warhost.c:33-42` (le TODO est déjà écrit ligne 33-34) —
`SOLDE_EU4_DIV 13` · `SOLDE_ARMS_DIV 26` · `SOLDE_FL_FLOOR 6.0` · `SOLDE_FL_PER_REG 0.7`
· `SOLDE_OVER_K 3.0`. Et **4 clés lues par `tune_f` mais ABSENTES de
`scps_tune_list.h`**, donc **non surchargeables** (`SCPS_TUNE=… → exit 2`) :
`BT_DEF_EDGE` (`scps_campaign.c:766`), `BT_DECROCHE` (`:1054`),
`BT_RELIEF_FALL` (`:858`), `BT_ATK_RATIO` (`scps_sim.c:222`).
**Vérifié** : `./chronicle.exe --tunables` ne les liste pas.
**Effet** : aucun en jeu (défauts identiques) ; **débloque tout sweep d'équilibrage
militaire**, aujourd'hui impossible sur 9 des valeurs les plus structurantes.
**Risque** : nul. Golden inchangé.

### P3 — Ouvrir la fenêtre de DÉCROCHAGE `[T:BT_DECROCHE, après P2]`
**Site** : `scps/scps_campaign.c:1048-1071`.
**Constat** : 97-100 % des batailles finissent en déroute, 0-3 % en décrochage, 0 nul
sur ~1 700 batailles (§3.1). Fenêtre `[BT_RUPTURE 0.20, BT_DECROCHE 0.22]` = **2 points**,
testée **1 jour sur 5**.
**Proposition** : `BT_DECROCHE 0.22 → 0.35` (fenêtre de 15 points) **et** tester le
décrochage sur **les deux** jours d'accalmie (`ph >= BT_CHOC_J` au lieu de
`ph == BT_CHOC_J`).
**Effet attendu** : décrochages de ~2 % à **15-25 %** des batailles ; les armées
survivent, la guerre s'inscrit dans la durée (le décrochage tue 8 % contre 8-62 % en
poursuite, `:1060`).
**Risque** : MOYEN — le décrochage n'appelle PAS `bt_press_siege` (`:1068-1071`),
donc moins de prises. Mitigation : faire presser le siège au vainqueur du décrochage
comme à celui de la déroute. **À mesurer en 3×3 apparié avant d'acter.**

### P4 — La MILICE comme plancher de levée (le « ban » du pays sans arsenal) `[C]`
**Sites** : `scps/scps_warhost.c:216` et `:225`.
**Constat** : le plancher retombe sur Piquier/Épéiste/Archer, **tous gatés sur
l'arsenal** ; un pays sans armes lève 0 régiment (`essai_s512:674-676`, Ligue
Thrumdinel : stock Bétail/Laine/Poisson, 2 rgt). C'est le seul « 0 rgt » résiduel du
post-correctif (§2.6).
**Proposition** : remplacer les deux planchers par `U_MILICE` (`RES_NONE`, aucun gate
d'arme, `pay_mult 0.65`).
**Effet** : plus jamais un pays absolument désarmé ; le pauvre a une armée de
fortune (force 67, 0 contre) qui perd — mais qui EXISTE. Répond directement au
risque « l'assiégé ruiné qui ne se défend plus ».
**Risque** : faible en jeu ; **mais** cette même Milice est l'unité la plus rentable
du roster (§1.3-b, efficacité 112 contre 4,2) — ne l'ouvrir QUE comme plancher, jamais
dans la composition normale, sinon l'IA n'a plus de raison de lever autre chose.

### P5 — Rééquilibrer la MILICE avant de l'ouvrir `[C]`
**Site** : `scps/scps_army.c:140` (`SOLDE_FORTUNE_DISC 0.35`) et `:119-121`.
**Constat** : `RES_NONE` supprime **97-99 % de la solde** (la part armes), pas 35 %.
Milice 0,6 or/mois contre 35 pour un piquier.
**Proposition** : donner à la Milice une catégorie d'arme (`RES_ARMS_LIGHT`) avec un
coefficient de consommation réduit — ou, plus simple et plus honnête, remplacer
`SOLDE_FORTUNE_DISC` par un **coût d'armes forfaitaire** :
`arms = POP_PER_UNIT × price[RES_ARMS] × 0.25` dans `warhost_unit_pay_month`
(`scps_warhost.c:162-167`) pour les unités `RES_NONE`.
**Effet chiffré** : solde milice 0,6 → **9,3 or/mois** ; efficacité 112 → **7,2**
(au-dessus du piquier 4,2, ce qui reste cohérent avec « masse paysanne bon marché »).
**Risque** : faible. Prérequis de P4.

### P6 — Le CHOC doit tuer au petit format `[T:BT_CHOC_MORTS]`
**Site** : `scps/scps_campaign.c:1026-1032`.
**Constat** : sous ~25 paquets, `dueB` reste à 0 sur toute la bataille
(`/runB:449` : **0 mort au choc** sur 23 batailles ; `/runC:721` : ratio 24×).
**Proposition A (tunable)** : `BT_CHOC_MORTS 0.006 → 0.012`. Effet : ratio
poursuite/choc de 7,7× à ~3,9× sur `essai_s512`, de 3,7× à ~1,9× sur le run A.
**Proposition B (code, préférable)** : appliquer au choc le **même plancher T5** que
la poursuite (`if (to_kill<1 && lp>=1) to_kill=1`, `:942-943`) — au moins 1 paquet
tué par cycle de choc décisif (`2pA/tot > 1.2`). Effet : la petite bataille cesse
d'être gratuite sans toucher le grand format.
**Risque** : faible pour B, MOYEN pour A (le grand format passerait sous la cible 2-5).

### P7 — Rendre le terrain de combat visible `[T:BT_DEF_EDGE, après P2]`
**Site** : `scps/scps_campaign.c:766`.
**Constat** : `fminf(adv, 1.10)` écrase montagne 1.20, jungle 1.15, forêt 1.12,
marais 1.10 sur une seule valeur (§3.2). 4 biomes sur 6 sont indiscernables.
**Proposition** : `BT_DEF_EDGE 0.10 → 0.20` — exactement le maximum de la table
`terrain_combat_bonus` (`scps_army.c:615`), donc **aucune valeur neuve** : le clamp
cesse d'écraser, il borne.
**Effet** : le gradient plaine 1.00 → collines 1.05 → marais 1.10 → forêt 1.12 →
jungle 1.15 → montagne 1.20 devient réel. Rivière non pontée : 1.25 → toujours
clampée à 1.20.
**Risque** : MOYEN — le commentaire P3 (`:763-766`) documente qu'un bonus défensif
trop haut a déjà gelé le front (217 batailles, 0 occupation). +10 points sur le seul
défenseur d'un FORT reste sous la barre historique, mais **doit être mesuré**.

### P8 — Modérer le levier cavalerie `[T:CAV_PURSUIT, CAV_CUREE_CAP]`
**Site** : `scps/scps_campaign.c:929-932`.
**Constat** : un vainqueur monté tue 55-62 % du vaincu contre 8-12 % pour l'infanterie
(§3.4), et il subit 4× moins d'attrition de marche (§3.3), et la Cav. lourde n'a
**aucun gate de tech** (§1.3-d).
**Proposition** : `CAV_PURSUIT 0.45 → 0.30` et `CAV_CUREE_CAP 0.40 → 0.25`.
**Effet chiffré** : cavalerie pure 0.40 (plafond 0.47) contre infanterie 0.10
(plafond 0.22) — ratio **4×** au lieu de 5-6×, la cavalerie reste l'arme de la curée
sans la monopoliser.
**Risque** : faible en calibrage, mais le ratio choc/poursuite baisserait aussi —
à combiner avec P6-B, pas avec P6-A.

### P9 — Gater la Cav. lourde `[C]`
**Site** : `scps/scps_army.c:87-102` (`unit_tech_gate`, défaut `TECH_COUNT`).
**Constat** : 9 contres (le plus large réseau), force 169, `pay_mult` 2.40, gate
d'élite seul. Le Hallebardier, son contre naturel, est au **tier 0** — l'équilibre
tient — mais la Cav. lourde est disponible dès le jour 1 à qui a 200 élites.
**Proposition** : `case U_CAV_LOURDE: return TECH_ORGANISATION;` (tier 2, comme le
Lancier de choc qui la contre) ⇒ `pay_mult 2.40 → 5.00`, solde 56,0 → 58,4 or/mois
(la part armes domine, l'effet de PRIX est faible), mais surtout **un palier de tech
à franchir**. La Cav. légère (6 contres, force 109) reste le jour-1.
**Risque** : MOYEN — la défense day-1 contre la cavalerie doit rester possible ;
Piquier/Lancier contrent déjà la Cav. légère (`scps_army.c:187`).

### P10 — La solde se paie de la caisse du PAYS `[C, chantier dédié]`
**Sites** : `scps/scps_warhost.c:324` et `:415`.
**Constat** : §4.1 — le trésor est province-owned, la solde ne peut mordre que sur
la province-capitale ; c'est ce qui a rendu 92-99 % de la masse salariale gratuite.
**Proposition** : après avoir épuisé la province-capitale, **répartir le solde impayé
au prorata sur les autres provinces du pays** (même idiome que le dispatch d'État,
`scps_econ.c:4560-4633`), en respectant la charte province-grain
(`econ_prov_treasury_credit` par province, jamais une caisse fictive).
**Effet attendu** : `sizemult` devient un vrai frein ; le budget militaire remonte de
5-11 % vers 25-48 % pour les empires au-dessus de leur limite (§2.4).
**Risque** : **ÉLEVÉ** — c'est la modification la plus lourde du lot : elle change la
trésorerie de tous les empires en guerre, donc les chantiers, le crédit et les
banqueroutes. **Doit être une vague à elle seule, avec kill-switch prouvé et sweep
apparié 3×3** (motif M-vague). À faire APRÈS P1, qui en absorbe déjà l'essentiel du
symptôme pour un centième du risque.

### P11 — Télémétrie : trois lignes qui manquent ou qui mentent `[C, chronicle.c seul]`
1. **Aucune ventilation par TYPE d'unité** dans la chronique ⇒ impossible de vérifier
   quelles unités sont levées (§1.3-g). Ajouter une ligne « armée du monde par type ».
2. `la mer ... 0 fournitures consommées (NE doit plus être zéro)` (`essai_s512:811`)
   annonce un défaut alors que `NAVY_COMBAT_ON=0` est la décision (§4.8) : la ligne
   doit dire « combat naval DÉSARMÉ (NAVY_COMBAT_ON=0) ».
3. **`n/limite de force` n'est jamais imprimé** : la ligne empire donne `armée N
   (M rgt)` (`chronicle.c:2033`) où `N` est `diplo_mil_power`, pas un compte d'armées.
   Ajouter `M rgt / FL` rendrait le dépassement lisible d'un coup d'œil (c'est ce qui
   a coûté deux missions à diagnostiquer A2/A3).
**Risque** : nul (sortie console seulement).

---

## 6. RÉCAPITULATIF DES FOURCHETTES

| grandeur | fourchette | source |
|---|---|---|
| Limite de force | 6,7 (1 rég) → 48,0 (60 rég) | `scps_warhost.c:171-173` |
| Régiments observés / limite (post-fix) | **0,15 → 2,02×** | `/runA:562-586`, `/runB:343-357` |
| Régiments observés / limite (pré-fix) | 0 → **33,5×** | `essai_s777:781` |
| Solde d'un régiment | 0,6 (Milice) → 184 or/mois (Chaman), prix de base | §1.2 |
| Part armes dans la solde | **97-99 %** | `scps_warhost.c:159-168` + `scps_econ.c:342-350` |
| Budget militaire / fiscalité | **48 % (an 30) → 5 % (an 200)** | §2.4 |
| Durée de bataille | **16-20 jours** | 20 journaux + 3 runs |
| % de batailles finies en déroute | **97-100 %** | §3.1 |
| Ratio morts poursuite/choc | **2,2× → ∞** (cible 2-5) | §3.5 |
| Part du vaincu tuée à la poursuite | **8-12 %** (inf) → **55-62 %** (cav) | §3.4 |
| Attrition de marche par case | **0,8 %** (cav/plaine) → **62 %** (v=1/jungle) | §3.3 |
| Siège d'une place tenue | **375 → 730 j** (plafond quasi systématique) | §3.6 |
| Chute réelle d'une place | **30 j** après bataille gagnée (`BT_RELIEF_FALL`) | `scps_campaign.c:858` |
| Prises par bataille | **0,16 → 0,34** | §3.6 |
| Guerres / 200 ans | **13 → 114** ; durée moyenne ~6,9 ans | `temoin_s777`, `temoin_s11`, §2.6 |
| Butin de guerre / an / monde | **58 → 401 or** (0,3-5 % d'un revenu d'État) | §4.4 |

---

*Rapport de lecture. Aucun fichier source, aucun banc, aucun golden touché.
Runs : `/runA_512_120.log`, `/runB_512_30.log`, `/runC_512_120_pay45.log`.*
