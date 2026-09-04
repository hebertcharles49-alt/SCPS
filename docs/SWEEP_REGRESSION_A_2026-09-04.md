# SWEEP DE NON-RÉGRESSION — VAGUE A (6 graines × 250 ans, apparié)

Question du joueur : « **vérifie que les corrections n'ont pas ouvert de nouveaux trous** ».

Corpus APRÈS : `sweep_valid_A_6x250/` — 12 journaux (graines 7 · 11 · 512 · 777 · 3 · 60 ×
témoin `AI_DOCT=0` / essai), 250 ans, binaire `128df3a0…`, **12/12 rendent 0**.
Corpus AVANT : les **mêmes 6 graines** dans `sweep_valid_W1W2_50x250/` (binaire `2bb5301c…`),
12 journaux. Total 24 journaux lus.

**Méthode.** Les 24 blocs analytiques ont été ouverts et lus intégralement (en-tête worldgen,
colonnes an 50/100/150/200, bloc monétaire complet, `BILAN`, `DOCTRINES`, `LEDGERS`, liste des
empires vivants ligne par ligne, bloc « par âge », toute la SYNTHÈSE). Les dumps `PROV` ont été
lus en entier pour `temoin_s7`, `temoin_s11`, `essai_s11` et `essai_s7` (corpus APRÈS) ; pour les
huit autres journaux APRÈS et pour le corpus AVANT, le dump `PROV` n'a pas été relu ligne à ligne
(cf. §6 Limites). `grep -n` n'a servi qu'à retrouver une ligne déjà lue et à afficher côte à côte
les 24 exemplaires d'une même ligne pour dresser les tables. **Aucun compteur, aucun filtre
découvrant.**

**Rappel du brief.** L'« indice » (M7-I1) des journaux AVANT lisait un miroir FAUX
(`econ_country_price_level` resté à `SINK_FLOOR × n_prov`, corrigé par A3) : il n'est **jamais**
comparé ici. Les comparaisons de prix passent par la ligne dédiée `prix du grain` et par la ligne
`marché :` du bloc par âge.

**Sanité générale, 12/12 APRÈS** : 6 âges, aucun ASSERT, aucun NaN, aucune fin prématurée,
`0 groupe(s) hors invariant` et `écart +0 = 0.0 %` sur le ledger P11, `0 nul(s)` de bataille,
`acharnement 0`, `top ≤ 30 %` (14-16 %). Invariant M3f : pic **95 à 110 %** pour un seuil de 370 %
(AVANT : 93 à **172 %**, `sweep_valid_W1W2_50x250/essai_s3:110`).

---

## 1. LES CINQ ANOMALIES DU SWEEP PRÉCÉDENT, AVANT → APRÈS

### A1 — Le recoupement I0 ne se recoupe jamais · **FERMÉ**

`hors registre`, or/mois/empire (la ligne `recoupement I0` de la SYNTHÈSE) :

| graine | témoin AVANT → APRÈS | essai AVANT → APRÈS |
|---|---:|---:|
| 7 | **−4 077,8** → **+0,5** | −1 287,6 → +2,0 |
| 11 | −505,6 → +3,3 | −2 245,5 → +0,9 |
| 512 | −3 388,7 → +5,6 | −1 908,5 → +0,8 |
| 777 | −1 658,8 → +0,1 | −3 783,2 → +0,7 |
| 3 | −299,6 → +7,1 | −567,4 → +4,0 |
| 60 | −1 993,2 → **+37,9** | −2 139,7 → +19,6 |
| **médiane** | **−1 826 → +2,7** | |

Citations APRÈS : `temoin_s7:990` `Σ postes −109.1 · flux mesuré −108.6 · hors registre +0.5` ·
`temoin_s60:941` (+37,9, le pire du corpus) · `essai_s512:826` (+0,8).

Le signe s'est **inversé** (12/12 positifs, contre 27/27 négatifs avant), et l'amplitude est
divisée par 50 à 8 000. Le critère d'acceptation posé par l'analyste précédent
(`|hors registre| < 0,10 × taxes`) est **tenu 12/12** : le pire cas, `temoin_s60`, vaut
37,9 sur des taxes de 1 058,5 (3,6 %). La ligne `flux décomposé` compte désormais 30 postes,
dont `achat d'État` (−1 095 à −10 303) et `assiette` (+761 à +6 147), qui sont exactement ce que
A1 avait nommé.

**Reste** : le résidu de PORTE, que A1 avait déclaré non localisé, est **plus gros que le
recoupement lui-même** et de signe variable — voir N13.

### A2 — `armée / limite de force` jusqu'à 432 % · **ATTÉNUÉ**

| graine | témoin méd·max AVANT → APRÈS | essai méd·max AVANT → APRÈS |
|---|---|---|
| 7 | 30 % · 224 % → 74 % · **202 %** | 68 % · 224 % → 45 % · **169 %** |
| 11 | 60 % · 268 % → 15 % · **225 %** | 30 % · 331 % → 7 % · **205 %** |
| 512 | 30 % · 200 % → 53 % · **187 %** | 103 % · 194 % → 23 % · **319 %** |
| 777 | 113 % · 245 % → 15 % · **257 %** | 81 % · 265 % → 22 % · **306 %** |
| 3 | 74 % · **432 %** → 15 % · **134 %** | 83 % · 258 % → 15 % · **158 %** |
| 60 | 83 % · 218 % → 31 % · **151 %** | 45 % · 246 % → 22 % · **148 %** |

Médiane des maxima **245,5 % → 194,5 %** ; pire du corpus **432 % → 319 %** ; nombre de journaux
au-dessus de 250 % : **5/12 → 3/12**. Le pire offender nommé par le sweep précédent
(`temoin_s3`, 432 %) tombe à **134 %** (`temoin_s3:986`) — sur la même graine, le même bras.

Atténué, pas fermé : `essai_s512:847` **319 %** et `essai_s777:954` **306 %** sont *au-dessus* de
leurs homologues AVANT (194 % et 265 %). Et la **médiane s'effondre de 71 % à 22 %**, ce qui n'est
pas un gain mais une dégradation de la mesure — voir N5.

### A3 — Empires riches, armés en stock, à 0 régiment · **FERMÉ**

Le cas nominatif du sweep précédent, `Ligue Dhûrganyn` :

- AVANT `sweep_valid_W1W2_50x250/essai_s11:697-699` : `64 rég · pop 378 k · **or 184 577** ·
  armée 33 (**0 rgt** / limite 51) · solde/revenu 0 %`, stock `Armes lourdes 127 329 ·
  Armes de trait 108 922`.
- APRÈS `sweep_valid_A_6x250/essai_s11:716-717` : `40 rég · pop 159 k · or 9 337 ·
  armée 19 (**56 rgt** / limite 34) · solde/revenu 67 % · **corps 22 rgt** ·
  solde 16 521/an dont corps 4 964 (30 %)`.

`arsenal vide` vaut **0 pays-an dans 12/12** (contre 3 063 mesurés par A2 sur le bras AVANT de s3).
`sans capitale` vaut 0 dans 11/12 (146 sur `essai_s7`). Les `0 rgt` restants sont soit des hameaux
d'1-2 régions dont toute l'armée est au front (`essai_s11:729` `0 rgt / limite 7 · **corps 44 rgt**
· solde 5 580/an dont corps 5 580 (100 %)`), soit des micro-États sans population mobilisable
(`temoin_s512:697` `Clans Zenilel 2 rég · pop 3 k · levée : plus d'hommes`). **Aucun empire ≥ 20
régions n'est plus désarmé** ; le critère d'acceptation de P3 est tenu.

### A4 — Le prix du grain à 0,000 · **FERMÉ**

| graine | témoin AVANT → APRÈS | essai AVANT → APRÈS |
|---|---:|---:|
| 7 | 0,212 → 0,154 | 0,168 → 0,152 |
| 11 | **0,048** → **0,812** | 0,233 → **1,432** |
| 512 | 0,199 → **2,253** | 1,154 → 0,200 |
| 777 | **0,000** → **0,194** | 0,228 → 0,171 |
| 3 | **0,000** → **0,154** | **0,000** → **0,408** |
| 60 | 0,232 → **0,709** | 0,233 → 0,435 |

`médiane 0.000` exact : **3/12 → 0/12**. Citations : `temoin_s3:102` `médiane 0.154` (AVANT
`0.000`, `sweep_valid_W1W2_50x250/temoin_s3:102`) · `temoin_s777:102` `0.194` (AVANT `0.000`) ·
`essai_s3:101` `0.408` (AVANT `0.000`).

La ligne `marché :` du bloc par âge confirme, et c'est la mesure la plus nette : graine 7 témoin,
les six âges passent de **1,06 / 0,94 / 0,81 / 0,07 / 0,15 / 0,26** à
**1,71 / 1,69 / 1,95 / 0,71 / 0,47 / 0,34**. Les âges tardifs ne tombent plus à 1 % de la base.
Le zéro absorbant est mort. La médiane des médianes bouge peu (0,206 → 0,222) mais **la queue a
changé de côté** : elle était basse (0,000), elle est haute (2,253 sur `temoin_s512:102`) — voir N15.

### A5 — `solde / revenu fiscal` sans borne supérieure · **ATTÉNUÉ**

| graine | témoin méd·max AVANT → APRÈS | essai méd·max AVANT → APRÈS |
|---|---|---|
| 7 | 27 % · 121 % → 30 % · **63 %** | 21 % · 1 412 % → 48 % · **1 962 %** |
| 11 | 16 % · 878 % → 12 % · **79 %** | 10 % · 437 % → 23 % · **867 %** |
| 512 | 3 % · 29 % → 56 % · **197 %** | 41 % · **3 656 %** → 32 % · **268 %** |
| 777 | 33 % · 293 % → 9 % · **65 %** | 22 % · 133 % → 36 % · **129 %** |
| 3 | 34 % · **3 981 %** → 13 % · **137 %** | 15 % · 477 % → 27 % · **4 706 %** |
| 60 | 38 % · **19 066 %** → 11 % · **120 %** | 13 % · 389 % → 28 % · **201 %** |

Médiane des maxima **457 % → 167 %** ; pire du corpus **19 066 % → 4 706 %** ; journaux au-dessus
de 1 000 % : **4/12 → 2/12**. Les deux pires cas de W2 (`temoin_s60` 19 066 % et `temoin_s3`
3 981 %) tombent à **120 %** et **137 %** (`temoin_s60:963`, `temoin_s3:987`).

**Mais la métrique reste inexploitable comme gate** : le nouveau pire cas, `essai_s3:921`, est
`Ligue … 6 k hab · 2 rgt / limite 7 · corps 1 rgt · **solde 535 or/an · revenu 11,4** ·
solde/revenu 4 706 %`. C'est une division par un dénominateur famélique, pas un doomstack — le
diagnostic est *identique* à celui posé en 2026-09-04 §A5, et la proposition P7 (plancher de
revenu à l'affichage) **n'a jamais été appliquée**. Voir N6.

### Anomalie 5 (Marbrive) — sans objet

Le rapport §1.5 l'avait déjà réfutée. Confirmé : `1 Marbrive` sur `temoin_s512`, `essai_s512`,
`essai_s777`, `temoin_s3` ; `2` sur `temoin_s60` ; `3` sur `temoin_s7`, `essai_s7`, `essai_s60` ;
`4` sur `essai_s3` ; `0` sur les deux s11 et `temoin_s777`. **9 journaux sur 12 non nuls, 21
déclenchements** — le trigger vit. Rien à corriger.

**Bilan §1 : 3 FERMÉES (A1, A3, A4) · 2 ATTÉNUÉES (A2, A5) · 0 OUVERTE.**

---

## 2. LES NOUVEAUX TROUS

### N1 — La satisfaction des journaliers perd 20 points, dans 11 journaux sur 12 · **CRITIQUE**

| graine | témoin AVANT → APRÈS | essai AVANT → APRÈS |
|---|---:|---:|
| 7 | 63 % → **39 %** | 64 % → **42 %** |
| 11 | 49 % → 40 % | 59 % → **41 %** |
| 512 | 66 % → **42 %** | 59 % → 50 % |
| 777 | 62 % → 64 % *(seul en hausse)* | 73 % → **56 %** |
| 3 | 61 % → **45 %** | 58 % → **43 %** |
| 60 | 56 % → **28 %** | 60 % → **32 %** |
| **médiane** | **60,5 % → 42 %** | |

Citations : `temoin_s60:936` `Laborer **28 %**` (le plancher du corpus) · `temoin_s7:985` `39 %` ·
`essai_s11:823` `41 %`. Le Bourgeois perd 8,5 points de médiane (73,5 → 65), l'Élite 3,5 (62,5 → 59) :
**la baisse est spécifique aux journaliers**, elle n'est pas une déflation uniforme.

La richesse/tête suit : Laborer médiane **5,29 → 2,85**, avec deux effondrements
(`temoin_s7` 4,73 → **0,41** ; `temoin_s3` 2,40 → **0,16**). Et à mi-parcours c'est pire :
`temoin_s60` an 100 `Laborer **0,18**` (AVANT 1,32) · `essai_s60` an 100 `Laborer **0,10**`
(AVANT 0,89) · `temoin_s7` an 100 `0,07` (AVANT 0,77).

*Cause pressentie* : A3. TROUVAILLES §A3 l'avait vue venir et actée comme un coût honnête
(« des prix réels exonèrent réellement les pauvres… calibrage de `TAX_EXEMPT_BASKET_MULT` —
décision joueur, hors périmètre A3 ») avec une mesure de −15 points à 120 ans sur une graine.
**À 250 ans sur 12 journaux, le coût est de −20 points de médiane et il touche 11 journaux sur
12.** Ce n'est plus « un cheveu ». C'est le trou le plus large ouvert par la vague, et il porte
sur 84-91 % de la population mondiale.

*Sévérité* : CRITIQUE. C'est la seule ligne de télémétrie qui dise si le monde est vivable.

### N2 — Le semis privé de manufactures est devenu bimodal (141 à 18 060) · **HAUTE**

| graine | témoin AVANT → APRÈS | essai AVANT → APRÈS |
|---|---:|---:|
| 7 | 2 839 → 2 075 | 2 781 → **683** |
| 11 | 2 378 → **141** | 2 427 → **189** |
| 512 | 2 774 → 3 113 | 2 779 → **450** |
| 777 | 2 674 → **18 060** | 5 100 → **9 023** |
| 3 | 1 407 → 3 046 | 1 053 → 1 278 |
| 60 | 3 385 → **878** | 12 886 → 1 805 |

Étendue AVANT **1 053 → 12 886** (facteur 12) ; APRÈS **141 → 18 060** (facteur **128**).
Citations : `temoin_s11:813` `141 manufacture(s) privée(s)` · `temoin_s777:873` `18 060`.

La médiane bouge peu (2 776 → 1 541), mais **la dispersion a gagné un ordre de grandeur** : le
canal du semis privé est devenu tout-ou-rien. TROUVAILLES §A3 avait mesuré, à 120 ans,
`61 → 6 → 69` sur s512 et concluait qu'`exp=0,5` « restaure le semis là où `exp=1` le casse » :
à 250 ans sur 6 graines, ce n'est **pas confirmé** — le semis s'éteint (141, 189, 450, 683) autant
qu'il explose (9 023, 18 060). Les colonies du peuple suivent : `9 → 2` (s512 témoin),
`7 → 2` (s512 essai), `5 → 0` (s3 témoin), `7 → 0` (s3 essai).

*Cause pressentie* : le semis privé est financé par la richesse des classes, qui est libellée en
`price_level` ; A3 en a changé la dynamique sans changer le seuil de semis. Aucune ligne du
chronicle ne dit **pourquoi** un semis est refusé (c'est exactement le trou que A2 a comblé pour
la levée).

### N3 — La friche remonte (8 journaux sur 12) · **MOYENNE**

| graine | témoin | essai |
|---|---|---|
| 7 | 47 → **32** ✅ | 44 → **69** |
| 11 | 34 → 39 | 29 → **40** |
| 512 | 22 → 30 | 26 → 31 |
| 777 | 23 → 28 | 24 → **36** |
| 3 | 45 → **35** ✅ | 35 → 40 |
| 60 | 31 → 29 ✅ | 20 → 20 = |
| **médiane** | **30 → 33,5** | |

Citations : `essai_s7:967` `friche … 69 rég impayée(s)` (le pire du corpus) · `essai_s11:757` `40`.
TROUVAILLES §A3 annonçait « **la friche est le gain le plus régulier** : `exp=0,5` bat les DEUX
autres régimes sur les trois graines (41/16/24) ». **À 250 ans sur 12 journaux, la friche monte
dans 8, descend dans 3, stagne dans 1.** La promesse d'A3 sur la friche ne tient pas à l'horizon
de la partie.

### N4 — Les provinces FIGÉES remontent (9 journaux sur 12) · **MOYENNE**

| graine | témoin | essai |
|---|---|---|
| 7 | 8 % → 8 % = | 5 % → 7 % |
| 11 | 12 % → 13 % | 9 % → 11 % |
| 512 | 9 % → 9 % = | 7 % → 10 % |
| 777 | 5 % → **9 %** | 10 % → **14 %** |
| 3 | 7 % → **17 %** | 6 % → **12 %** |
| 60 | 10 % → 11 % | **36 % → 10 %** ✅ |
| **médiane** | **8,5 % → 10,5 %** | |

Citations : `temoin_s3` `118 FIGÉES … (17 %)` · `essai_s777` `90 FIGÉES … (14 %)`.
C'est modeste en médiane (+2 points), mais **monotone** : 9 hausses, 1 baisse, 2 égalités.
Le reste W1-B (« les mondes figés sont ceux à faible population par province ») n'est pas
refermé, et l'appauvrissement des journaliers (N1) le nourrit mécaniquement.

### N5 — `armée / limite` ne mesure plus rien : la médiane s'effondre de 71 % à 22 % · **HAUTE**

Médiane des médianes **71 % → 22 %** (table §A2). Ce n'est pas une armée qui fond : c'est le
NUMÉRATEUR qui se vide. A4 fait payer les corps de campagne, donc les pays transfèrent au front —
et `warhost_units` comme la limite de force restent **host-seuls** (reste acté par TROUVAILLES §A4,
« décision joueur : la limite de force doit-elle compter l'armée de campagne ? »).

Les deux cas qui le disent nommément :

- `temoin_s512:675-676` : `Clans Tikexis 56 rég · pop 193 k · armée 21 (**5 rgt / limite 45**) ·
  **corps 79 rgt** · solde 6 705/an dont corps 6 705 (**100 %**)` — l'empire n° 1 du monde entretient
  **79 régiments** et compte pour **11 %** de sa limite de force.
- `essai_s11:719-720` : `Ligue Mertonis 37 rég · armée 25 (**0 rgt / limite 32**) · **corps 61 rgt**`
  — 0 % de sa limite, 61 régiments au front.

Conséquence directe : ces empires ne paient **aucune intendance de dépassement** (`over`/`sizemult`
se calculent sur le host), et la ligne du chronicle sur laquelle le joueur juge l'armée est
devenue fausse dans les deux sens. La solde, elle, est bien facturée — A4 tient sur son périmètre.

*Sévérité* : HAUTE, mais c'est un reste **acté**, pas une surprise.

### N6 — `solde / revenu` et `dette / revenu` : la queue est un dénominateur, jamais bornée · **MOYENNE**

`essai_s3:921` `solde 535 or/an · revenu 11,4 · solde/revenu **4 706 %**`.
`essai_s7:1060` max **1 962 %** · `essai_s11:850` max **867 %**.
Même maladie sur la dette : `dette structurelle max` passe de 115,17× à 115,56× (s60 témoin,
`temoin_s60:107`) et **empire sur 6/12** (`temoin_s512` 10,88× → **77,15×** ; `temoin_s777`
12,57× → **49,11×** ; `essai_s60` 8,73× → **94,37×**) alors même que le stock de dette s'effondre
(N7). Les deux ratios divisent par un revenu fiscal qui a rétréci avec les prix.

P7 de l'analyste précédent (n'agréger que les pays au revenu > seuil, et imprimer le nombre
d'exclus) reste **non appliquée**. Print-only, risque nul.

### N7 — Le crédit s'est éteint, et son indicateur de risque a explosé · **HAUTE**

| mesure (médiane 12 journaux) | AVANT | APRÈS |
|---|---:|---:|
| dette totale (or) | 285 000 | **16 400** |
| banqueroutes forcées | 31,5 | **3,5** |
| prêteurs ruinés | 9,5 | **0** |
| poste `intérêts` (I0, or/mois/empire) | −93,6 à −770,0 | **−0,1 à −37,9** |
| saisie (M3g, or) | 8,0 M | **0,26 M** |
| **dette/revenu MAX** | 15,98× | **36,74×** (6/12 en hausse) |

Citations : `temoin_s7:107` `**2** banqueroute(s) … taux moyen **2,73 %** … 0 dette(s)
structurelle(s) ≥3x · max **2,19x** · **0** prêteur(s) ruiné(s)` contre
`sweep_valid_W1W2_50x250/temoin_s7:—` `31 banqueroute(s) … max 57,31x · 14 prêteur(s) ruiné(s)` ·
`essai_s3:106` `**1** banqueroute` (AVANT 34) · `essai_s3` saisie `**863 or**` sur toute la sim
(AVANT 8 498 225).

Le module crédit/banqueroute/saisie, qui était un des moteurs de la vie politique du monde
(M3g, M9-V3, prêteurs ruinés, marchés étrangers fermés), **ne mord plus**. Ce n'est pas
« l'assainissement » : `0 prêteur ruiné` dans 8/12 et `1 banqueroute` sur 250 ans sont des
zéros de système mort, pas des zéros de santé. Simultanément le ratio max dette/revenu monte —
c'est-à-dire que les rares endettés le sont **plus** relativement, mais que rien ne les punit.

*Cause pressentie* : A3 (numérateur nominal divisé par ~2 sur la masse monétaire, ÷1,9 médiane)
combiné à A1 (la comptabilité qui ferme les fuites) : les États n'ont plus de raison d'emprunter.

### N8 — La dérive séculaire est saine en signe mais TOMBE SOUS la cible joueur · **MOYENNE**

`dérive annualisée (OLS log)`, cible `docs/MONNAIE_CONCEPT.md` **0,5-1,5 %/an** :

| graine | témoin AVANT → APRÈS | essai AVANT → APRÈS |
|---|---:|---:|
| 7 | +1,46 → **+0,33** | +0,79 → **+0,08** |
| 11 | **−1,36** → +0,37 | −0,09 → **+0,51** |
| 512 | +0,83 → +0,33 | +0,29 → +0,38 |
| 777 | +0,12 → +0,37 | **−0,45** → +0,35 |
| 3 | −0,07 → +0,37 | **−0,48** → +0,37 |
| 60 | **−0,90** → +0,30 | −0,26 → +0,39 |

**Le signe est réparé** : 6/12 dérives négatives AVANT, **0/12 APRÈS**, bande resserrée de
[−1,36 ; +1,46] à [+0,08 ; +0,51]. C'est le gain le plus propre du chantier A3.
**Mais 11 valeurs sur 12 sont SOUS le plancher de 0,5 %/an.** A3 avait tranché `PL_EXPONENT=0,5`
sur trois graines à 120 ans qui donnaient +1,28 / +0,99 / +0,50 — **à 250 ans la dérive médiane
est +0,365**, soit la moitié basse de la cible manquée. Le critère qui a tranché l'exposant ne
tient pas à l'horizon de la partie.

### N9 — Les fins §27 s'uniformisent : RONCES passe de 7/12 à 11/12 · **MOYENNE**

| | AVANT | APRÈS |
|---|---:|---:|
| RONCES | 7 | **11** |
| ENGLOUTISSEMENT (EAU) | 4 | **1** |
| GRAND HIVER | 1 | **0** |
| RÉCHAUFFEMENT / ASCENSION / SANG / aucune | 0 | 0 |

Bascules : `temoin_s777:761` EAU → **RONCES** · `temoin_s3:847` EAU → **RONCES** ·
`essai_s3:875` EAU → **RONCES** · `essai_s60:837` GRAND HIVER → **RONCES**. Seule
`essai_s7:878` reste ENGLOUTISSEMENT.

*Cause pressentie* : l'entropie faustienne s'effondre avec les prix — `temoin_s7` 856 130 → 414 487,
`essai_s7` 3 691 → **66**, `temoin_s3` 245 363 → **5 762**, `temoin_s60` 321 891 → 65 041 — et avec
elle les fins qui dépendent de la charge faustienne, laissant RONCES dominer. La ligne du chronicle
se dénonce déjà elle-même (`ratio max/min dispatch 99.9:1, cible ≤2:1`) ; la vague A a **aggravé**
ce ratio au lieu de le laisser tel quel.

*Conséquence collatérale expliquée* : les provinces `sans propriétaire (owner=-1)` sont exactement
les régions ENGLOUTIES. La corrélation est parfaite sur les 24 journaux — `essai_s7` ENGLOUTISSEMENT
12 régions → 21 orphelines ; `temoin_s3` AVANT ENGLOUTISSEMENT 12 régions → 25 orphelines ;
tous les journaux RONCES → 0 orpheline. Le mécanisme est cohérent ; ce n'est pas une anomalie
(mais `essai_s7` conserve deux provinces orphelines de **12 915** et **13 529** habitants qui ne
paient d'impôt à personne).

### N10 — 100 % du commerce mondial par les Centres des cités-états, deux fois · **MOYENNE**

`temoin_s11:822` et `essai_s11:834` : `hubs des cités-états ........ **100 %** du commerce mondial
passe par leurs Centres` — et sur `temoin_s11` le détail donne `1910 / 1910`, donc **au-dessus du
plancher de volume** : ce n'est pas l'artefact A6 du sweep précédent, c'est une mesure réelle.
AVANT, les mêmes graines donnaient 84 % et 31 %. Douze cités-états captent l'intégralité des
échanges d'un monde de 36 pays et 1,2 M d'habitants.

À noter aussi : le plancher de volume n'est **toujours pas** posé sur la ligne de la SYNTHÈSE
(proposition P6 du sweep précédent, jamais appliquée).

### N11 — L'arbre HÉRITÉ (§27) redevient muet · **MOYENNE**

Journaux à `arbre HÉRITÉ` non nul : **3 AVANT → 1 APRÈS**.
`sweep_valid_W1W2_50x250` : `temoin_s777` 2 empires 77 % · `temoin_s3` **6 empires 91 %** ·
`essai_s3` 1 empire 90 %. `sweep_valid_A_6x250` : `essai_s7:1006` `**4 empire(s) · 79 %** (max 93 %)`
seul non nul ; `temoin_s777:844`, `temoin_s3:933`, `essai_s3:985` sont tous retombés à
`0 empire(s) · 0 %`. Les trois qui s'éteignent sont exactement les trois qui ont perdu leur fin
ENGLOUTISSEMENT (N9) : l'héritage §27 est gaté par la fin tirée.

Le sweep précédent classait cette ligne parmi les **correctifs TENUS** de W1-C (« l'arbre HÉRITÉ
enfin visible »). Elle redevient conditionnelle.

### N12 — `brassage : 0 flux` passe de 3/12 à 5/12 · **BASSE**

`temoin_s777:848` **286 → 0** · `essai_s777:905` **105 → 0** ; s512 reste à 0 sur les deux bras
avant et après ; `temoin_s60:913` 1 → 20 (en hausse) ; `essai_s3:989` 156 → 59.
Le pacte migratoire tombe à zéro absolu sur 250 ans dans **5 mondes sur 12** (A9 du sweep
précédent, aggravé).

### N13 — Le résidu de PORTE est plus gros que le recoupement, et de signe variable · **MOYENNE**

`contrôle des portes (A1)`, colonne `porte hors poste`, or/mois/empire, APRÈS :

| graine | témoin | essai |
|---|---:|---:|
| 7 | −5,6 | −32,9 |
| 11 | −49,0 | **−196,7** |
| 512 | −61,5 | +120,4 |
| 777 | +95,2 | **+444,4** |
| 3 | −134,2 | −3,9 |
| 60 | +75,7 | +146,5 |

Citations : `essai_s777:934` `Σ portes +456.4 · **porte hors poste +444.4** · écriture directe
−443.7` — soit **22 % de la ligne `taxes`** (+2 040,7) de ce journal ; `essai_s11:829` −196,7.
Le recoupement I0 ferme (§A1) parce que porte et écriture directe se compensent au centième ; mais
l'instrument que A1 a posé pour **nommer** le résidu montre qu'il est de l'ordre de 5 à 22 % des
taxes et qu'il change de signe d'un monde à l'autre. Reste A1 acté (« ventiler PAR SITE »),
mécanique, print-only.

### N14 — Le conseil s'emballe sur deux mondes · **BASSE**

`remplacement(s) IA/sim` : `temoin_s3` 431 → **1 132** (×2,6) · `essai_s777` 38 → **857** (×22) ·
`essai_s60` 578 → 890. Médiane 395,5 → 423 : c'est une queue, pas un régime. À lire avec
`ministre(s) au bord` : `essai_s777` 1 → **11**.

### N15 — Le prix du grain a une queue HAUTE là où il avait une queue basse · **BASSE**

`temoin_s512:102` `médiane **2,253** · moyenne 2,436` et `essai_s11:101` `médiane **1,432**` :
deux mondes où le pain vaut plus du double de sa base. C'est le symétrique du 0,000 fermé par A3,
et c'est **moins grave** (pas de point absorbant : 2,253 est une médiane saine, la moyenne suit).
À surveiller, pas à corriger.

---

## 2 bis. LA GRAINE 60, TRAITÉE À PART (divergence signalée par A4)

TROUVAILLES §A4 signalait, sur une seule run s60 non appariée : `intérêts −8,9 → −344,8`,
`grain médian 0,23 → 1,51`, `satisfaction Laborer 56 % → 36 %`, `provinces figées 10 % → 38 %`,
et concluait « trajectoire chaotique probable, PAS un verdict — à re-juger sur un apparié 3×3 ».
Voici le jugement, avec les deux bras :

| mesure | témoin AVANT → APRÈS | essai AVANT → APRÈS | verdict |
|---|---|---|---|
| provinces FIGÉES | 10 % → **11 %** | **36 % → 10 %** | **NON REPRODUIT** (l'essai s'améliore de 26 points) |
| poste `intérêts` (I0) | −8,9 → **−37,9** | **−149,8 → −19,6** | **NON REPRODUIT** (sens opposé sur les deux bras) |
| grain médian | 0,232 → 0,709 | 0,233 → 0,435 | sain, pas 1,51 |
| satisfaction Laborer | 56 % → **28 %** | 60 % → **32 %** | **REPRODUIT — et généralisé** |
| `armée/limite` max | 218 % → **151 %** | 246 % → **148 %** | amélioré, 2/2 |
| `solde/revenu` max | **19 066 % → 120 %** | 389 % → 201 % | amélioré, 2/2 |
| dette totale | 28 405 → **132 364** | 388 448 → 41 193 | sens opposé sur les deux bras : bruit |

**Verdict s60** : deux des trois signaux d'alarme d'A4 (figées, intérêts) sont du **bruit de
trajectoire** — ils changent de sens selon le bras. Le troisième, la satisfaction Laborer, est
**réel mais n'est pas propre à s60** : c'est N1, qui frappe 11 journaux sur 12. La graine 60 n'est
pas un cas particulier, c'est simplement le monde où N1 est le plus violent (28 %). **A4 n'est pas
en cause** : le poste `soldes` de s60 tombe de −562,3 à −192,6 or/mois/empire, exactement ce que
A4 promettait (« l'État finit par payer moins parce qu'il entretient moins »).

---

## 3. CE QUI S'EST AMÉLIORÉ SANS QU'ON L'AIT VISÉ

1. **Le trésor et le flux des empires se redressent.** Trésor moyen/empire, médiane
   **10 448 → 12 965 or** ; flux moyen/empire, médiane **−10,6 → +19,1 or/mois** (7/12 passent en
   positif). `temoin_s3` +22,9 → +90,3 · `essai_s3` −29,2 → +52,9 · `essai_s11` −0,6 → +236,0.
   Aucune de ces lignes n'était visée par A1-A4.
2. **L'écart âmes/strates PAR PROVINCE s'effondre** (anomalie A7 du sweep précédent, non traitée
   par la vague) : `temoin_s3` **130 941 % → 227,8 %** · `essai_s11` **85 814 % → 617,2 %** ·
   `temoin_s7` 51 459 % → 9 756 % · `essai_s777` 49 123 % → 27 478 %. Une seule régression
   (`temoin_s11` 2 449 % → 13 867 %).
3. **L'invariant monétaire M3f respire** : pic annuel `temoin_s3` **155 % → 100 %**,
   `essai_s3` **172 % → 99 %** ; l'ensemble du corpus tient dans 95-110 % pour un seuil de 370 %.
4. **Les provinces sans propriétaire disparaissent des 3/4 des mondes** : 4 journaux concernés
   AVANT (18, 18, 25, 37 orphelines) → **1 seul** APRÈS (21, `essai_s7`), et l'explication est
   trouvée (§N9 : ce sont les régions englouties).
5. **`édifices refusés faute de palier`** : médiane **374,5 → 273**. `temoin_s11` 543 → 348 ·
   `essai_s11` 518 → 283 · `essai_s7` 444 → 336. Le reste W1-C n'a pas été touché et la ligne
   s'améliore quand même.
6. **Divin sort du zéro absolu** : 0 adoption sur 1 209 doctrines AVANT → **2** APRÈS
   (`essai_s7:—` `Divin 1`, `essai_s512:—` `Divin 1`), sans que la porte (foi d'État) ait bougé.
7. **La fronde vassale produit enfin ses trois fins** : `temoin_s11` `3 fronde(s) → 1 indépendance
   · 2 RENVERSEMENT(s)` (AVANT 1/0/1) · `essai_s7` `1 fronde → 1 RENVERSEMENT` (AVANT 0/0/0).
   Le `concordat` de suzeraineté aussi : `essai_s7` **17** (AVANT 0), `temoin_s11` 16, `temoin_s512` 4.
8. **Le décrochage (W2-3) tient à 250 ans dans le nouveau régime** : 16,8 % à 21,4 % sur 12/12,
   `0 nul` sur 12/12, durée 16-17 j. La médiane bouge de 19,9 % à 19,6 %.
9. **`arsenal vide` 0 pays-an et `sans capitale` 0 dans 11/12** : les deux gates que A2 et 38523b6
   visaient sont éteints partout.
10. **Le juge côtier remonte** (35 % contre 30 %, `resume.txt`) alors que W1-E l'avait cassé —
    et les doctrines actives montent de 82 à 90,5 en médiane.

---

## 4. VERDICT EN 5 LIGNES

1. **Les cinq anomalies sont traitées** : A1 (recoupement), A3 (riche désarmé) et A4 (grain 0,000)
   sont **FERMÉES** sur 12/12 ; A2 (armée/limite) et A5 (solde/revenu) sont **ATTÉNUÉES** — pire
   cas 432 % → 319 % et 19 066 % → 4 706 %, mais leurs queues restent ouvertes.
2. **Oui, la vague a ouvert un trou de premier ordre** : la **satisfaction des journaliers perd 20
   points de médiane dans 11 journaux sur 12** (60,5 % → 42 %, plancher 28 %), avec la richesse/tête
   Laborer divisée par deux. C'est le prix d'A3, assumé à 120 ans, hors de proportion à 250.
3. **Trois autres trous sont à corriger avant de continuer** : le semis privé devenu tout-ou-rien
   (141 à 18 060, dispersion ×128), le crédit éteint (banqueroutes ÷9, prêteurs ruinés → 0) pendant
   que son ratio de risque explose, et `armée/limite` rendue illisible parce que A4 fait payer les
   corps que ni le numérateur ni la limite ne comptent.
4. **Le reste est du bruit de trajectoire** (6 graines, une sim par cellule) : friche +3,5,
   FIGÉES +2 points, conseil qui s'emballe sur deux mondes, brassage à 0 sur deux graines de plus,
   grain à 2,25 sur un monde. Aucun de ces écarts n'a la même signature que N1/N2/N7.
5. **La divergence s60 signalée par A4 est levée** : figées et intérêts changent de sens selon le
   bras (bruit), et le seul signal réel — la satisfaction — n'est pas propre à s60 : c'est N1.
   **A4 est innocenté**, son poste `soldes` fait exactement ce qu'il promettait.

---

## 5. PROPOSITIONS, CLASSÉES PAR IMPACT

### P1 — Ventiler la satisfaction du journalier avant toute autre vague · **impact fort, risque nul (étape 1)**
*Constat* : N1, −20 points de médiane, 11/12, plancher 28 % (`temoin_s60:936`).
*Geste, étape 1 (print-only)* : décomposer la ligne `satisfaction` en ses termes moteur (panier
vital servi / non servi · gages perçus · exonération fiscale mordue ou non) et l'imprimer par
classe, comme A2 l'a fait pour la raison du refus de levée. Sans cette ligne on ne saura pas si la
chute vient du prix du panier, du gage, ou de l'assiette.
*Site pressenti* : `scps/scps_econ.c` (calcul de satisfaction, voisinage du panier vital
`g_basket_pc` et de `TAX_EXEMPT_BASKET_MULT`) + impression dans `scps/chronicle.c`.
*Étape 2, après lecture* : calibrer `TAX_EXEMPT_BASKET_MULT` (explicitement laissé « décision
joueur, hors périmètre A3 » par TROUVAILLES §A3).
*Mesure d'acceptation* : satisfaction Laborer médiane ≥ 55 % sur 6 graines × 250 ans, apparié
`PL_LEGACY=1` vs défaut ; aucune graine sous 40 %.

### P2 — Imprimer la RAISON du refus de semis privé · **impact fort, risque nul**
*Constat* : N2, dispersion ×128, `temoin_s11:813` 141 contre `temoin_s777:873` 18 060.
*Geste* : compteurs print-only par pays-an sur le semis §NF (capital insuffisant · demande nulle ·
intrant absent · palier de province · main-d'œuvre), exactement le motif `warhost_levy_reason*`.
*Site pressenti* : `scps/scps_econ.c` (semis privé §NF) + ligne SYNTHÈSE.
*Mesure* : la raison lue explique ≥ 80 % des refus sur les deux extrêmes (s11 et s777).

### P3 — Faire compter les corps dans `warhost_units` et dans la limite de force · **impact fort, risque moyen — DÉCISION JOUEUR**
*Constat* : N5. `temoin_s512:675-676` `Clans Tikexis 5 rgt / limite 45 · corps 79 rgt` ;
`essai_s11:719-720` `Ligue Mertonis 0 rgt / limite 32 · corps 61 rgt`.
*Geste* : `over`/`sizemult` et `warhost_units` somment host + corps (les trois `static inline`
d'en-tête posées par A4 existent déjà : `campaign_deployed_units`, `campaign_deployed_by_type`).
**Aucun plafond** : l'intendance de dépassement est un COÛT, la limite reste indicative.
*Risque* : renchérit d'un coup tout pays en guerre longue — à mesurer en apparié avant d'acter,
comme A4 l'a fait.
*Kill-switch* : une clé au registre J à 0 = comportement d'aujourd'hui.
*Mesure* : `armée/limite` médiane revient dans 40-110 % (au lieu de 22 %) et aucun empire ne porte
> 10 régiments de corps pour une limite host de 7.

### P4 — Garder `solde/revenu` et `dette/revenu` par un plancher de revenu · **impact moyen, risque nul**
*Constat* : N6. `essai_s3:921` `solde 535 or/an · revenu 11,4 · 4 706 %`.
*Geste* : P7 de l'analyste précédent, jamais appliquée — n'agréger dans médiane/max que les pays
dont le revenu fiscal annuel dépasse un seuil (ex. 100 or/an), et imprimer le nombre d'exclus.
Print-only.
*Site pressenti* : `scps/chronicle.c` (lignes `solde / revenu fiscal` et `dette (M3c)`).
*Mesure* : le max tombe sous 400 % et le nombre d'exclus est imprimé.

### P5 — Décider du sort du crédit · **impact moyen, risque moyen — DÉCISION JOUEUR**
*Constat* : N7. `essai_s3:106` `1 banqueroute` sur 250 ans, `essai_s3` saisie **863 or** au total,
`0 prêteur ruiné` dans 8/12.
*Geste* : soit re-calibrer les seuils d'emprunt sur la nouvelle échelle de prix (l'IA n'emprunte
plus parce que la caisse suffit), soit acter que le crédit est un levier de fin de partie et
retirer les lignes de télémétrie qui l'attendent. **Ne pas toucher avant P1** : si la satisfaction
remonte via les gages, l'assiette fiscale remonte avec elle et le crédit peut se réveiller seul.
*Site pressenti* : `scps/scps_credit.c` (seuils d'emprunt) — à ne rouvrir qu'après mesure.
*Mesure* : banqueroutes médiane ≥ 10/sim et `dette/revenu max` ≤ 30×.

### P6 — Ventiler le résidu de PORTE par site · **impact moyen, risque nul**
*Constat* : N13. `essai_s777:934` `porte hors poste **+444,4**` = 22 % des taxes.
*Geste* : reste A1, déclaré mécanique par TROUVAILLES — passer un `FluxComp` aux deux portes en
mode diag et imprimer la ventilation par site d'appel. Print-only.
*Site pressenti* : `econ_flux_door_note` / `econ_nation_gold_add` / `econ_nation_gold_force`.
*Mesure* : `|porte hors poste| < 0,03 × taxes` sur 6 graines.

### P7 — Re-trancher `PL_EXPONENT` sur la dérive à 250 ans · **impact moyen, risque moyen — DÉCISION JOUEUR**
*Constat* : N8. Le critère qui a tranché 0,5 (dérive dans 0,5-1,5 %/an) était mesuré à 120 ans ;
à 250 ans **11 valeurs sur 12 sont sous le plancher** (médiane +0,365 %/an).
*Geste* : sonde appariée `PL_EXPONENT ∈ {0,5 ; 0,6}` sur 3 graines × 250 ans, en lisant **d'abord**
la satisfaction Laborer et les manufactures privées (N1, N2), pas seulement la dérive — le piège
documenté par A3 (« toute correction DOIT être mesurée sur la masse monétaire ET le semis privé »).
*Mesure* : dérive dans 0,5-1,5 %/an **et** satisfaction Laborer non dégradée.

### P8 — Le plancher de volume sur la ligne hubs de la SYNTHÈSE, et le 100 % · **impact faible, risque nul**
*Constat* : N10. `temoin_s11:822` et `essai_s11:834` à **100 %** (volume 1910/1910, donc réel).
*Geste* : (a) poser la garde de `scps/chronicle.c:2234` sur l'agrégat `:3045` (P6 du sweep
précédent, toujours pas faite, 3 lignes) ; (b) instrumenter le routage du commerce sur s11 pour
comprendre pourquoi 12 cités-états captent tout.

### P9 — Systèmes morts : rien de nouveau, rien de changé · **décision joueur**
Marine (`0 coque · 0 fourniture · 0 raid · 0 interception` dans **12/12** APRÈS comme AVANT, avec
30-34 Scieries navales bâties par sim), `0 ralliés culturellement` (12/12), fins
RÉCHAUFFEMENT / ASCENSION / SANG (0/12), `Pont effondré` (0/12), guerres anti-piraterie (0/12).
La vague A ne les a ni ouverts ni fermés. Soit on les branche, soit on retire les lignes qui
s'accusent elles-mêmes.

---

## 6. LIMITES

1. **6 graines, une sim par cellule.** Aucune répétition intra-graine : les Δ par graine ne sont
   pas des signaux, seules les médianes sur 12 journaux le sont. Un écart présent dans 11/12
   (N1) ou 9/12 (N4) est un régime ; un écart sur 2 journaux (N14) est du bruit.
2. **Deux binaires différents.** Les mondes AVANT et APRÈS divergent dès l'an 2 ; l'appariement
   est fait graine à graine et bras à bras, ce qui est la seule façon honnête sans re-run, mais
   il n'isole pas la vague A du hasard de trajectoire. Les quatre bascules de fin §27 (N9) en sont
   la démonstration : elles peuvent être un effet A3 **ou** une divergence de trajectoire.
3. **Horizon unique de 250 ans face à des cibles calibrées à 120.** C'est la limite la plus lourde,
   et elle joue ici dans les deux sens : N8 (dérive sous la cible) et N3 (friche) sont exactement
   des cibles héritées d'une mesure à 120 ans qui ne tiennent pas à 250. Sans un point à 120 ans
   dans le MÊME binaire, on ne peut pas séparer l'effet d'horizon de l'effet de vague.
4. **Les dumps `PROV` du corpus AVANT n'ont pas été relus ligne à ligne**, ni ceux de 8 des 12
   journaux APRÈS. Ce que ces dumps portent — la population par province — a été lu intégralement
   sur 4 journaux APRÈS : pop maximale 20 076 (`temoin_s7`, 1,8 % du monde), 28 825 (`temoin_s11`,
   2,4 %), 36 432 (`essai_s11`, 3,3 %), 17 045 (`essai_s7`, 1,6 %). L'anomalie A8 du sweep
   précédent (une tuile à **8 %** du monde) n'est retrouvée dans aucun des quatre. Le constat est
   donc « pas d'aggravation visible sur 4 mondes », pas « A8 est fermée ».
5. **Ce que le corpus ne permet pas de conclure** : qu'un système est mort (Divin passe de 0 à 2
   adoptions : c'est « rare », pas « impossible ») ; qu'une fin §27 est sur-représentée
   (11 RONCES sur 12 tirages est frappant mais le χ² n'a pas de puissance sur 12 sims) ; qu'un Δ
   de médiane inférieur à 15 % sur 12 journaux est un signal.
