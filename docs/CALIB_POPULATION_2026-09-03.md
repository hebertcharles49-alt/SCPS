# CALIBRAGE POPULATION — proportions, usages, interactions, frictions (2026-09-03)

**Périmètre** : lecture seule. Aucun fichier moteur touché, aucun `make`, aucun sweep neuf.
**Sources code** : `scps/scps_demography.c`, `scps_econ.c/.h`, `scps_influence.c`,
`scps_army.c`, `scps_warhost.c`, `scps_factions.c`, `scps_revolt.c`, `scps_religion.c`,
`scps_diplo.c`, `scps_api.c`, `scps_labor.h`, `scps_tune_list.h`, `chronicle.c`.
**Sources mesure** : les **20 journaux** de `sweep_doct_ai_10x200/` (10 graines × 200 ans ×
2 bras, `manifest.txt`, binaire `8140fa92…`) + **1 run de sonde** avec le binaire COURANT
(`SCPS_CAPDIAG=1 ./chronicle.exe 7 1 100 6 12`, mêmes arguments que le sweep).

**Méthode de lecture** : lecture intégrale des blocs démographiques des 20 journaux
(bilans an 40/80/120/160, bloc « empires vivants » complet — 152 lignes-pays —,
`CLASSES monde`, `SIÈGES`, satisfaction/richesse par classe, tiers de province, brassage,
réfugiés, esclavage, métabolisation, soulèvements, PROV total). Les dumps `PROV n ville=…`
et les listes de worldgen ont été lus sur un exemplaire complet (`temoin_s11`) puis écartés
pour les 19 autres : ils ne portent que `pid/ville/pays/pop` (strates), déjà résumé par les
lignes-bilan.

> **Avertissement de datation.** Le sweep est daté du **2026-09-02** ; l'assiette de
> l'influence a été re-keyée sur les SIÈGES le **2026-09-03** (`ddd1b1c`). **Toutes les
> lignes `influence :` des 20 journaux sont donc PRÉ-re-key** (assiette = strate élite
> seule). Le run de sonde est post-re-key.

---

## 0. Les deux réalités de classe — définitions exactes

| | STRATES par richesse | SIÈGES d'emploi |
|---|---|---|
| Champ | `prov[].strata[k].pop` (float) | `prov[].pop.groups[i].pop_by_class[k]` (long) |
| Déclaration | `scps_econ.h:274` (prov), `:405` (région) | `scps_econ.h:212` |
| Qui écrit | `econ_tick` : croissance `scps_econ.c:5261`, mobilité `mobility_tick_region` `:3653-3690` | `demography_emerge_classes` `scps_demography.c:642` |
| Cadence | mensuelle, **toutes** les provinces | mensuelle, **une province par région** (`scps_demography.c:1215-1220`) |
| Invariant | Σ strates = pop de la province | « Σ `pop_by_class` = `count` » (`scps_econ.h:211`) — **cassé, cf. §4.1** |
| Agrégat région | `region[].strata` = **vraie somme** (`scps_econ.c:1627`) | `region[].pop` = **COPIE de la province représentative** (`scps_econ.c:1649`) |
| Porte le wealth/satisfaction | oui | non |
| Sert à | impôt, croissance, savoir, commerce, levée, révolte, prospérité, légitimité, tiers, façade pays | influence, factions, `pol_sat`, façade province |

Les deux comptent des ÊTRES HUMAINS dans la même province et **ne sont jamais égales**.

---

## 1. PROPORTIONS

### 1.1 Monde, fin de sim (an 200) — 20 journaux, ligne `CLASSES monde` / `SIÈGES`

| Réalité | Journaliers | Bourgeois | Élites | Esclaves |
|---|---|---|---|---|
| **STRATES** (min / méd / max) | 85 / **89** / 91 % | 6 / **8** / 12 % | 1 / **2** / 4 % | 0 / **0** / 0 % |
| **SIÈGES** (min / méd / max) | 66 / **73,5** / 79 % | 3 / **8** / 15 % | 13 / **16** / 19 % | 0 / **0** / 0 % |

**L'écart élite est de ×8 (16 % contre 2 %)** — et il n'est pas stable : à l'an 100
(sonde graine 7) les sièges donnent **11 % / 4 % / 83 %** contre des strates à
**89 / 8 / 1** — ×11. L'écart CROÎT avec le bâti (chaque édifice d'élite ajoute
`tier×100` sièges, `scps_demography.c:600-621`) et DÉCROÎT avec la population (les
strates croissent, `count` ne croît quasiment pas — §4.1).

### 1.2 Trajectoire des strates (constante, presque inerte)

Semis : **80 / 15 / 5** (`CLASS_SHARE`, `scps_econ.c:605`, appliqué par
`econ_seed_population` `:1150`). Fin de sim : **89 / 8 / 2** (ligne « classes (E0.7,
départ 80/15/5) » des 20 journaux : Laborer 86-91 %, Bourgeois 6-12 %, Élite 1-5 %).

**Le monde se DÉ-stratifie** : bourgeois 15 → 8, élites 5 → 2. Les plafonds doux
(`SHARE_CAP_BOURGEOIS` 0,32 · `SHARE_CAP_ELITE` 0,11, `scps_econ.c:3650-3651`) ne
mordent **jamais** — ils sont 4× au-dessus de l'état atteint. Cause : §3.4.

### 1.3 Pyramide par pays (152 lignes-pays, an 200 — strates, `chronicle.c:2033`)

| Taille | J | B | É |
|---|---|---|---|
| ≥ 30 régions (32 pays) | 77-97 % (méd. **85**) | 2-18 % (méd. **11**) | 1-8 % (méd. **3**) |
| 5-29 régions | 71-97 % | 3-26 % | 0-11 % |
| 1-4 régions | 0-100 % | 0-33 % | 0-89 % |
| **pop < 1 000 (19 lignes, 12 %)** | 61-98 % | 1-19 % | 1-89 % | **pyramide FABRIQUÉE, cf. §4.4** |

Le pays le plus stratifié observé : `essai_s3333 Clans Dhûrdin` 31 rég, 146 k,
**78/15/7** (l.46 du relevé). Le moins : `temoin_s3333 Ordre Brenredel` 29 rég, 99 k,
**97/2/1**. À taille et époque égales, l'amplitude est de 20 points de bourgeois — la
diversité vient donc bien du bâti, pas d'un tirage.

### 1.4 Richesse par tête (an 200, ligne `richesse/tête (M4-IP)`)

| | min | médiane | max |
|---|---|---|---|
| Journalier | 0,32 | **1,68** | 5,19 |
| Bourgeois | 6,12 | **32,0** | 67,73 |
| Élite | 8,22 | **56,3** | 206,05 |
| ratio B/J | 5,0× | **22,1×** | 65,7× |
| ratio É/J | 8,2× | **39,4×** | 114,4× |

Trajectoire du journalier : an 40 **1,16-3,28** → an 80 **0,49-2,40** → an 120
**0,29-2,79** → an 200 **0,32-5,19**. **Le journalier ne s'enrichit pas sur 200 ans** ;
l'élite passe de 1,45-13,85 (an 40) à 8,2-206 (an 200). L'écart s'ouvre ×5 à ×10.

### 1.5 Satisfaction par classe (an 200, pop-pondérée)

| | min | médiane | max |
|---|---|---|---|
| Journalier (89 % de la pop) | 38 % | **48,5 %** | 61 % |
| Bourgeois (8 %) | 68 % | **75 %** | 86 % |
| Élite (2 %) | 54 % | **62 %** | 69 % |

### 1.6 Population mondiale et tiers de province

| an | 40 | 80 | 120 | 160 | 200 |
|---|---|---|---|---|---|
| pop monde (min-max) | 83-117 k | 170-302 k | 285-576 k | 429-904 k | 523-1 248 k |
| médiane | 114 k | 249 k | 428 k | 660 k | **812 k** |

Croissance géométrique implicite an 40 → an 200 : **1,24 %/an** (colonisation comprise) —
loin du plancher `POP_R_BASE` 0,01733 (`scps_econ.c:5165`) et du plafond 0,06
(`:5172`). **Tiers de province (an 200) : T1 77-93 % · T7 1-2 %** — la province médiane
du monde reste un hameau de < 2 000 habitants sur 200 ans.

### 1.7 Esclaves — la classe morte

`esclavage : N âme(s) servile(s) dans le monde`, an 200, 20 journaux :
**0 · 0 · 0 · 0 · 0 · 0 · 1 · 2 · 12 · 29 · 35 · 47 · 83 · 132 · 196 · 215 · 281**
(3 journaux à 0 dans les deux bras). Médiane **≈ 6 âmes** pour un monde de 812 000 →
**0,007 ‰**. En face : **150 à 2 929 affranchissements/sim** et **66 à 1 183 âmes
déportées/sim**. La strate est vidée plus vite qu'elle n'est remplie.

---

## 2. USAGES — qui lit quelle population

### 2.1 Lecteurs de STRATES (`strata[].pop`)

| Consommateur | Site | Grain | Unité lue |
|---|---|---|---|
| **Impôt** (assiette per-capita / plancher M3i) | `scps_econ.c:4500`, `:4507` ; barèmes `:2656-2658` (0,06 / 0,15 / 0,27 or/mois/tête), `TAX_FLOOR_FRAC` 0,5 `:4501` | province | têtes |
| **Impôt sur le revenu** (taux) | `scps_econ.c:2667-2675` (40 / 55 / 75 %) sur `income_gross` = gages/profit/rente `:4470-4472` | province | or/tick |
| **Croissance démographique** | `scps_econ.c:5203-5262` (`eff_cap`, `cap_factor`, `st->pop *= 1+net_growth`) | province | têtes |
| **Mobilité de classe E0.7** | `scps_econ.c:3653-3690` | province | têtes |
| **Savoir / recherche** | `scps_econ.c:999-1006` (`SAVOIR_W_ELITE/BOURGEOIS/LABORER`) | **région** (`region[]`) | têtes |
| **Puissance commerciale** | `scps_econ.c:1045-1046` | **région** | têtes |
| **Levée militaire — pool** | `scps_army.c:317-322` `army_class_free` | **région** | têtes, **100 % de la strate** |
| **Levée militaire — gate élite** | `scps_warhost.c:190-195` `wh_country_elite` (`elite<=200` ⇒ pas d'unité d'élite `:287`) | **région** | têtes |
| **Révolte — capitale sous-équipée** | `scps_revolt.c:504-517` | **région** | têtes |
| **Révolte — part servile** | `scps_revolt.c:533-541` (`SLAVE_REVOLT_SHARE` 0,20) | **région** | part |
| **Turchin — aspirants** | `scps_demography.c:638` `demography_elite_rival` : `strata[ELITE].pop` / sièges STRICTS | province (mixte !) | ratio |
| **Prospérité** | `scps_prosperity.c:75-76`, `:138-139` | prov + région | têtes |
| **Légitimité** | `scps_legitimacy.c:28-29` | région | têtes |
| **Religion — fracture** | `scps_religion.c:397` | région | têtes |
| **Marine — équipage** | `scps_navy.c:146` (`LABORER < crew+200` ⇒ pas de coque) | région | têtes |
| **Allocation main-d'œuvre (façade)** | `scps_api.c:3049` `pool = LABORER + BOURGEOIS` | province | têtes |
| **Fiche pays (sidebar)** | `scps_api.c:1755-1770` `scps_country_demo` | **région** | têtes + satisfaction |
| **Chronique — pyramide pays** | `chronicle.c:283-287` `country_class_pop` | **région** | têtes |
| **Chronique — tiers, dump PROV, pop monde** | `chronicle.c:2696`, `:1743`, `:190` | prov / région | têtes |

### 2.2 Lecteurs de SIÈGES (`pop_by_class[]`)

| Consommateur | Site | Grain | Unité |
|---|---|---|---|
| **Influence politique — assiette (verbe joueur)** | `scps_influence.c:74-86` `infl_class_pop` ; taux `:128-142` : élite 0,002 (0,0025 Aristo) · bourgeois 0,0011 (0,0022) · journalier 0,00011 (0,00022) | **province** (correct) | têtes → influence/mois |
| Influence — hover 3 nombres | `scps_influence.c:104-111` `influence_seats` | province | têtes |
| Influence — Divin | `scps_influence.c:88-101` `infl_believers` : Σ `count` des groupes dont `faith == religion_of_country` × 0,00016667 | province | **âmes (`count`), pas sièges** |
| **Factions / éthos IA** | `scps_factions.c:157-171` `accumulate` : Σ `pop_by_class[k] × class_clout(k)` (3,0 / 1,6 / 1,0, `:15-22`) | province (toutes) | têtes pondérées |
| **Satisfaction politique `pol_sat`** | `scps_econ.c:5081-5093` (`POL_SAT_W` 0,30, cap ±0,15) : grief pondéré par `pop_by_class[c]` | province | têtes |
| Idem, lecteur façade | `scps_api.c:5510-5533` | province | têtes |
| **Fiche PROVINCE (façade)** | `scps_api.c:1554-1573` `scps_province_classes` — sièges si `n_groups>0`, **repli strates sinon** | province | têtes |
| **Sièges d'élite (source)** | `scps_demography.c:574-621` `prov_elite_seats_ex` : `capitale_max_tier(count)×100` + `EDI_ELITE_JOBS` (100) × Σtiers d'édifices d'élite + `EDI_ELITE_POP_PCT` (0,004) × count × invest × (1+rot) | province | sièges |
| Chronique — ligne SIÈGES | `chronicle.c:1786-1822` | province | têtes |

### 2.3 Lecteurs de `count` (âmes du groupe, hors classe)

`econ_country_metabolized` `scps_econ.c:1063-1079` · `econ_country_heritage_digested`
`:1086-1105` · `econ_off_culture_fraction` `:911-930` · `religion_refresh_region`
`scps_religion.c:128-142` · `demography_country_L/agitation` `scps_demography.c:118-131`.

### 2.4 Incohérences de grain relevées

| # | Incohérence | Site | Effet |
|---|---|---|---|
| G1 | **Le pool de levée est région-grain ET strate-grain** alors que le verbe joueur est province-grain | `scps_army.c:320`, `scps_warhost.c:193` | une levée peut vider une province que le lecteur ne voit pas ; contredit la charte PROVINCE |
| G2 | **`region[].pop` n'est pas un agrégat, c'est une COPIE** de la province représentative | `scps_econ.c:1649` (vs `:1627` pour les strates, vraie somme) | tout lecteur `region[].pop` ne voit qu'1 province sur ~2,1 |
| G3 | **`religion_refresh_region` décide la foi d'une région entière sur la seule province représentative** | `scps_religion.c:131-141` | la foi dominante d'une région de 5 provinces est celle d'UNE tuile |
| G4 | **`diplo_enslave_capture` pioche et dépose sur les seules provinces représentatives** | `scps_diplo.c:1402-1404` | §5.1 |
| G5 | **`demography_tick` (L, assimilation, conversion de foi, migration, émergence) ne touche QUE la province représentative** | `scps_demography.c:1131-1220` | §4.1 — la friction majeure |
| G6 | **Deux « tiers de capitale » pour la même province** : `capitale_max_tier(Σcount)` pour les sièges (`scps_demography.c:576`) vs `capitale_max_tier(Σstrata)` partout ailleurs (`scps_ai.c:1056`, `scps_api.c:1730`, `chronicle.c:2697`, `scps_econ.c:4029`) | | le tier « politique » et le tier « de construction » divergent |
| G7 | **Turchin compare deux réalités** : aspirants = `strata[ELITE].pop`, positions = sièges stricts | `scps_demography.c:634-641` | le ratio mesure surtout l'écart entre les deux ledgers |

---

## 3. INTERACTIONS — les boucles, et lesquelles tournent

### 3.1 Édifices → sièges → influence → doctrines → édifices — **BOUCLE RAPIDE, mesurée**

`edi_built` → `prov_elite_seats_ex` (+100 par tier d'édifice d'élite bâti,
`scps_demography.c:600-616`) → `infl_class_pop(ELITE)` → `influence_base_gain`
(élite × 0,002, 18× le journalier) → adoption de doctrine → `doctrine_key_mult`
sur `EDI_ELITE_JOBS` (Aristocratie « Fiefs ») **qui augmente encore les sièges**.

Mesure (bras ESSAI, `resume.txt`) : **33,1 doctrines actives/sim** en moyenne, jusqu'à
**46** (`essai_s3333`), et l'influence médiane du monde passe de **29,5 (an 40) à
11 564 (an 200)** sur cette graine — ×392. `INFLUENCE_CAP` = 0 (aucun plafond, décision
joueur 2026-09-01, `scps_influence.c:166`). Répartition arithmétique du gain sur les
sièges médians mesurés (16/8/73,5) : **66,7 % élites · 17,3 % bourgeois · 16,0 %
journaliers** — le design vise 60/20/20 (`scps_influence.h:64-72`), l'assiette réelle est
donc déjà **7 points plus aristocratique que la cible**.

Sonde post-re-key (graine 7, 100 ans, binaire courant) : **Σ influence générée 83 117 en
100 ans**, contre **79 495 en 200 ans** au même bras témoin du sweep (code pré-re-key).
Ordre de grandeur : **la génération d'influence a doublé** avec le re-key sur les sièges.

### 3.2 Bâti → logement → croissance — **BOUCLE SATURÉE**

`eff_cap = cap_pop/2 + tier×1000 + Σniveaux×HOUSE_MANUF(100) + food_cap×250`
(`scps_econ.h:737-758`), puis `cap_factor = max(0, 1 − pop/(eff_cap×1,1))`
(`scps_econ.c:5212`).

**Mesure directe (sonde `SCPS_CAPDIAG`, graine 7, an 100)** :
`pop=335 381 | colonisées=250/289 | remplissage_col=113 % | pop/EFF_CAP=118 % |
food_sat=0,85 | needs_met=0,61 | Σmanuf_lvl=3021`.

**Le monde est à 118 % de sa capacité effective à l'an 100.** `cap_factor` est donc
clampé à 0 sur la majorité des provinces : **la fertilité ne pilote plus rien**, seuls
BÂTIR (manufactures = logement) et COLONISER font monter la population. C'est cohérent
avec T1 = 77-93 % des provinces à l'an 200 : elles ne franchissent jamais 2 000 habitants.

### 3.3 Colonisation → habitabilité → croissance — **BOUCLE ACTIVE, dominante**

`econ_passive_seep` (`scps_econ.c:6245-6310`) : **2,8 hab/mois** (`SEEP_POP_MONTH`,
`:6249`) vers toute province adjacente sous `SEEP_TARGET`=100, tant que la source
dépasse `COLONY_MIN_POP`=300. Malus d'habitabilité : `1 − (1−hab)×HAB_MALUS_K(0,20)`
(`:5197-5199`).

Mesure : `colonisation : 96 à 295 fondation(s) DIRIGÉES/sim` mais **374 à 752 provinces
colonisées** en fin de sim — **le passif fait la majorité de l'expansion**. Aucune
colonie « de survie » (grenier vide) dans les 20 journaux : **0 partout**.

### 3.4 Colonisation → composition sociale — **POMPE DE PROLÉTARISATION (non voulue)**

`econ_passive_seep` prélève à la source **au prorata des trois classes libres**
(`scps_econ.c:6296-6300`) mais dépose **100 % en journaliers** à destination
(`:6301` `dst->strata[CLASS_LABORER].pop += g`). Idem pour les relocalisations de
pénurie (`:6621-6637`, journaliers seuls) — **95 à 295 relocalisations/sim**.

C'est le mécanisme qui explique 15 % → 8 % de bourgeois et 5 % → 2 % d'élites (§1.2)
alors que les plafonds doux sont à 32 % et 11 %. La contre-pression (`mobility_tick_region`)
est bridée trois fois : (a) elle exige un atelier (`manuf = n_bld>0`, `scps_econ.c:3657`)
que 77-93 % des provinces T1 n'ont pas ; (b) le seuil J→B est 1,4× le panier
(`PROMOTE_BASKET_MULT`, `:3461`) alors que la richesse/tête du journalier tombe à
0,3-2,8 dès l'an 80 ; (c) la démotion tourne **2× plus vite** que la promotion
(`PROMOTE_RATE*2` `:3688` vs `PROMOTE_RATE` `:3456`).

### 3.5 Brassage → métabolisation → tech — **BOUCLE ACTIVE, à très forte variance**

`metab_diffuse_coeff` (`scps_econ.c:945-953`) : migrant/soumis/réfugié **1,0** ·
déporté **0,3** (`METAB_DIFFUSE_SLAVE`) · natif **0,0**, × `metab_eff_integration`
(plancher servile 0,15, `:964-968`).

| | min | médiane | max |
|---|---|---|---|
| empires « creuset » (> 1 % digéré) | 4/18 | ~10/19 | 19/35 |
| métabolisation moyenne | 1,3 % | **5,4 %** | 11,8 % |
| **max d'un empire → bonus recherche** | 10,9 % | **30,2 %** | **68,4 %** |
| flux de pacte migratoire (âmes) | **0** | 5 617 | **38 892** |

**Variance de 1 à ∞ sur le brassage** (0 à 507 flux/sim, 0 à 38 892 âmes) : quatre
journaux sur vingt à moins de 2 000 âmes, quatre au-dessus de 25 000. Le bonus de
recherche va de +11 % à **+68 %** — c'est le plus gros multiplicateur de tech du jeu, et
il est quasi-aléatoire du point de vue du joueur.

### 3.6 Conversion → fidèles → influence Divin — **BOUCLE INERTE**

`religion : 3 foi(s) fondée(s)/sim · 7 pays fidèle(s)/sim` ; adoption du courant Divin :
**0/331 dans tout le sweep** (`TROUVAILLES.md`, mission anomalies). Le terme fidèles
(`INFLUENCE_PER_BELIEVER` 1/6000, `scps_influence.c:146-147`) n'a jamais été exercé en
condition de sweep. La foi d'une région est par ailleurs décidée par une seule province
(§2.4 G3).

### 3.7 Guerre → morts par classe → strates — **BOUCLE PRESQUE MUETTE**

`sang : mémoire des morts` = **0,02 % à 1,65 % de la pop vivante** (seuil de bascule
« SANG » : **9 %**). Morts de révolte : 143 à 3 552/sim sur 500 k-1,2 M d'habitants.
Batailles : 274 livrées, 6 100 morts au choc vs 27 300 en poursuite (`temoin_s11`).
**La guerre ne fait pas de démographie** : à l'échelle du monde elle est un bruit de
fond de 1 %.

### 3.8 Sièges → factions → IA — **BOUCLE BIAISÉE** (cf. §4.1)

`country_faction_weights` scanne bien TOUTES les provinces (`scps_factions.c:222-226`),
mais lit `pop_by_class` — qui vaut « 100 % journaliers » sur au moins 53 % d'entre elles.
La direction politique de l'IA est donc structurellement tirée vers le penchant
journalier, sans rapport avec ce qui est bâti là-bas.

---

## 4. FRICTIONS

### 4.1 **LA FRICTION MAJEURE — l'émergence de classe ne tourne que sur 47 % du monde**

`demography_emerge_classes` est appelée **une fois par RÉGION, sur la province
représentative** (`scps_demography.c:1215-1220`), jamais sur les provinces sœurs. Toute
province non-représentative garde le `pop_by_class` posé à sa fondation :
**100 % CLASS_LABORER** (`scps_world.c:3377-3378` à la genèse,
`scps_econ.c:6161` à la colonisation) — **définitivement**, quels que soient sa
population, sa capitale ou ses édifices.

**Mesure (20 journaux, `PROV total N colonisée(s)` vs `n_régions`)** :

| graine | régions | prov. colonisées (témoin) | part au plus représentative |
|---|---|---|---|
| 7 | 297 | 752 | **39,5 %** |
| 3333 | 200 | 500 | 40,0 % |
| 60 | 304 | 687 | 44,3 % |
| 777 | 292 | 633 | 46,1 % |
| 90 | 300 | 645 | 46,5 % |
| 512 | 256 | 539 | 47,5 % |
| 2026 | 253 | 502 | 50,4 % |
| 1009 | 210 | 374 | 56,1 % |
| 11 | 313 | 532 | 58,8 % |
| 4243 | 312 | 427 | 73,1 % |

**Médiane : 47,0 % → au moins 53 % des provinces colonisées du monde (min 27 %, max
60,5 %) n'ont JAMAIS de bourgeois ni de nobles au sens des sièges.** (Borne basse :
`n_régions` compte aussi des régions jamais colonisées.)

Conséquences directes, toutes vérifiées au site :
- **Influence** (`scps_influence.c:74-86`, verbe joueur, landé `ddd1b1c`) : l'assiette d'un
  pays ne compte les élites/bourgeois que de ses provinces représentatives, et compte les
  journaliers des autres à leur taille de FONDATION.
- **`pol_sat`** (`scps_econ.c:5081-5093`) : dans ces provinces `gpop == 0` pour BOURGEOIS
  et ELITE ⇒ **le canal de satisfaction politique est mort pour 2 classes sur 3 dans la
  majorité du monde**.
- **Factions** (§3.8).
- **Fiche province de la façade** (`scps_api.c:1560-1567`) : affiche « 100 % journaliers »
  sur une province de 5 000 habitants qui a un Tribunal et une Garnison.

### 4.2 **L'invariant `Σ pop_by_class == count` est cassé par 7 sites d'écriture**

| # | Site | Ce qui se passe |
|---|---|---|
| 1 | `scps_demography.c:303-304` `migration_move` (nouveau groupe) | `ng = *src` **copie le `pop_by_class` du groupe SOURCE ENTIER**, puis `ng.count = amount` — un groupe de 200 âmes hérite des sièges d'un groupe de 5 000 |
| 2 | `scps_demography.c:320` (fusion) | `count += amount`, sièges inchangés |
| 3 | `scps_demography.c:322` (source) | `count -= amount`, sièges inchangés |
| 4 | `scps_econ.c:6304` `econ_passive_seep` | `dg->count += g`, sièges inchangés — **et c'est le canal d'expansion dominant** (§3.3) |
| 5 | `scps_revolt.c:412` `revolt_ignite` | `g->count -= mob`, sièges inchangés |
| 6 | `scps_revolt.c:597` `demobilize` | `count += survivors`, sièges inchangés |
| 7 | `scps_demography.c:241` assimilation | `winner->count += g->count` ; le commentaire `:228` reconnaît que « `klass`/`pop_by_class` du groupe disparu sont PERDUS » |

Sur une province représentative l'écart est réparé au tick suivant. Sur les autres
(≥ 53 %) il est **permanent et cumulatif**. Le site #1 est le plus grave : il **crée** des
sièges d'élite ex nihilo à chaque exode de guerre (`scps_endgame.c:628`, 523 à ~35 000
âmes évacuées/sim) et à chaque flux de pacte migratoire (jusqu'à 38 892 âmes/sim).

### 4.3 **La levée militaire ne coûte rien en population**

`army_class_free` = `Σ region[].strata[cl].pop − pop_by_class_in_army`
(`scps_army.c:317-322`). Les affectés **restent dans les strates** : ils continuent de
produire, de payer l'impôt, de faire de la recherche et de se reproduire
(`scps_army.c:337-340`, commentaire explicite). Il n'y a **aucune fraction d'âge**
mobilisable : **100 % de la population est levable**, un paquet = 100 âmes
(`POP_PER_UNIT`, `scps_army.h:22`).

Mesure (`armée N (M rgt)` × 100 / pop, 152 lignes-pays an 200) :

| | part de la pop sous les armes |
|---|---|
| hégémon (★, 20 journaux) — médiane | **2,4 %** (min 0,6 %, max 3,5 %) |
| `temoin_s11` Clans Hobwickis (63 k, 165 rgt) | **26 %** |
| `essai_s11` Clans Hobwickis (57 k, 317 rgt) | **56 %** |
| `essai_s777` Couronne Falwick (79 k, 342 rgt) | **43 %** |
| `temoin_s3333` Mécaniste libre (18 k, 167 rgt) | **93 %** |
| `essai_s7` Ligue Karggoris (0,3 k, 4 rgt) | **> 100 %** |

Les cas extrêmes ont été traités côté BUDGET par la vague `38523b6` (garde de budget
sous le feu), mais **le trou de conception demeure** : rien dans le moteur n'interdit de
lever la population entière, et le corps de campagne n'est pas compté dans le pool
(reste documenté du 2026-09-03).

### 4.4 **Le plancher `st->pop < 1 ⇒ 1` fabrique 12 % des pyramides affichées**

`scps_econ.c:5261` : `st->pop *= 1+net_growth; if (st->pop<1.f) st->pop=1.f;` — appliqué
**par strate**, pas par province. Une province vidée garde donc 1 bourgeois et 1 noble
pour toujours.

**19 lignes-pays sur 152 (12,5 %) affichent `pop 0k`** ; dans 8 d'entre elles
`B% == É%` **exactement** — signature arithmétique du plancher :

| journal | ligne affichée | reconstruction |
|---|---|---|
| `temoin_s11` Adaptatif libre | J 66 % · B 17 % · É 17 % | L=4, B=1, É=1 |
| `essai_s11` Mécaniste libre | J 61 % · B 19 % · É 19 % | L=3,2, B=1, É=1 |
| `essai_s1009` Ordre Brenred | J 77 % · B 12 % · É 12 % | L=6,4, B=1, É=1 |
| `temoin_s11` Adaptatif libre (2) | J 92 % · B 4 % · É 4 % | L=24, B=1, É=1 |
| `temoin_s1009` Ordre Brenred | J 89 % · B 6 % · É 6 % | L=15, B=1, É=1 |
| `essai_s2026` Ordre Faroror | J 83 % · B 8 % · É 8 % | L=10,4, B=1, É=1 |
| **`temoin_s512` Métallurgiste libre** | **J 0 % · B 11 % · É 89 %** | L=0, B=1, É=8 — **un État de 9 âmes dont 89 % de nobles** |

Et le symétrique : `essai_s11` / `temoin_s11` **Mécaniste libre, 18-23 k habitants,
J 100 % · B 0 % · É 0 %** — une pop de 23 000 sans un seul bourgeois (jamais d'atelier ⇒
`manuf == false` ⇒ la promotion J→B ne se déclenche jamais, `scps_econ.c:3657`).

### 4.5 Autres frictions relevées

| # | Friction | Site / mesure |
|---|---|---|
| F1 | **`SCPS_MAX_GROUPS` = 8** par province (`scps_econ.h:186`) alors que le monde nomme **98 identités culturelles** (`temoin_s11`) ; `diplo_enslave_capture` **renonce en silence** si la province représentative de la capitale est pleine (`scps_diplo.c:1413`) | plafond dur non instrumenté |
| F2 | **Aucune croissance organique des `count`** — aucune boucle `count *= 1+growth` n'existe (constat explicite `scps_econ.c:5216-5222`) ; seule la strate servile est synchronisée par delta (`:5253-5257`). Les âmes-groupes ne bougent que par migration/assimilation/capture/manumission | l'écart strates↔groupes ne peut que croître |
| F3 | **Diasporas** : `réfugiés : 6-202 fuites (1 186-38 892 âmes) → 217-1 558 retours (2 200-23 178 âmes)`. Les retours dépassent souvent les départs en NOMBRE mais pas en ÂMES — le solde net s'accumule chez les voisins | 20 journaux, ligne `réfugiés` |
| F4 | **Hameaux WILD stériles** : `5,0 à 8,0 semés/sim · 0 ralliés culturellement · pop moy. 0` dans **20/20 journaux** | ligne `hameaux libres (WILD)` |
| F5 | **Sécessions à pop=0k** : 7 à 12 pays émergés/sim, dont plusieurs nés avec `Stab 0 Prosp 0` (`essai_s4243 Ésotérique libre 1 rég, Stab 0 Prosp 0, 0,1 k`) | ligne `pays émergés` |
| F6 | **12/20 pays en déficit vivrier STRUCTUREL** (`temoin_s11`) — l'import vital est la norme, pas l'exception | ligne `prévision` |
| F7 | **`demography_elite_rival` compare deux ledgers** (§2.4 G7) : le déficit de rivalité mesure surtout la divergence strates/sièges, pas une surproduction d'élites | `scps_demography.c:634-641` |

---

## 5. TRUCS « OP » ET CLASSES MORTES

### 5.1 Morts

| Élément | Preuve |
|---|---|
| **CLASS_SLAVE** | 0-281 âmes/monde (méd. ~6) pour 812 k habitants = **0,007 ‰**. Cause : `diplo_enslave_capture` (`scps_diplo.c:1395-1450`) ne prend que `SLAVE_FRACTION` (5 %, 15 % avec tech, `scps_econ.c:848-851`) du **plus gros groupe de LA province représentative** de la région prise, et ne dépose que dans la province représentative de la capitale, refusée si elle a déjà 8 groupes. Face à cela, 150-2 929 affranchissements/sim. `SLAVE_REVOLT_SHARE`=0,20 (`scps_revolt.c:538`) n'a jamais pu être atteint. |
| **Courant Divin (influence)** | 0/331 adoptions dans tout le sweep |
| **Bourgeois dans ≥53 % des provinces** | sièges figés à 0 (§4.1) ⇒ `pol_sat` mort, poids de faction nul |
| **Ralliement culturel des WILD** | 0 dans 20/20 sims (F4) |
| **Plafonds doux de mobilité** | 0,32 / 0,11 jamais approchés (état atteint : 0,08 / 0,02) |
| **Colonies de survie** | 0 dans 20/20 sims |

### 5.2 OP

| Levier | Mesure |
|---|---|
| **Métabolisation par brassage** | jusqu'à **+68,4 % de recherche** (`essai_s4243`) pour un pays qui a signé des pactes migratoires ; le pacte est PACIFISTE et gratuit en population (échange). Meilleur rapport effort/tech du jeu. |
| **Bâtir des édifices d'élite** | +100 sièges par tier bâti, sans plafond de part (`prov_elite_seats_ex`), et l'élite pèse **18×** un journalier dans l'influence et **3×** dans les factions. Une Citadelle T3 = 300 sièges = 0,6 influence/mois à elle seule. |
| **La levée gratuite** | 100 % de la population est mobilisable et les mobilisés continuent de produire et de payer l'impôt (§4.3) — l'armée n'a qu'un coût en OR. |
| **Déportés pour la tech** | coefficient 0,3 × plancher d'intégration 0,15 (`METAB_DIFFUSE_SLAVE`, `SLAVE_METAB_FLOOR`) : théoriquement le levier « esclavage pour la tech » existe, **mais il est inatteignable en pratique** (§5.1) — c'est un OP de papier. |

---

## 6. PROPOSITIONS CHIFFRÉES — classées par impact

Aucune n'est appliquée. Chacune indique le site, le nombre, le risque.

| # | Proposition | Site | Nombre | Risque |
|---|---|---|---|---|
| **P1** | **Faire tourner l'émergence de classe sur TOUTES les provinces colonisées** : remplacer la boucle `for r < n_regions → rep_province` par `for p < n_prov` | `scps_demography.c:1215-1220` | O(804) au lieu de O(313) une fois/mois ; **passe de ≤47 % à 100 % du monde** | **Golden re-baseline certain.** Les sièges d'élite explosent dans les petites provinces (`capitale_max_tier(count)×100` = 100 sièges sur une tuile à 300 âmes = 33 %) — **P1 exige P2 ET P4 dans la même vague.** |
| **P2** | **Rétablir `Σ pop_by_class == count`** : une fonction `group_seats_rescale(g)` (prorata sur les 3 classes, arrondi à 100) appelée aux 7 sites de §4.2 ; à `migration_move` (site #1) poser les sièges au PRORATA de `amount/src->count` au lieu de copier | `scps_demography.c:303, 320, 322, 241` · `scps_econ.c:6304` · `scps_revolt.c:412, 597` | supprime une création nette de sièges qui suit aujourd'hui **jusqu'à 38 892 âmes/sim** de brassage + 523-35 000 d'exode | Golden bouge. Le correctif est CONSERVATIF (aucun nombre neuf), c'est le meilleur rapport impact/risque. |
| **P3** | **Plafonner l'assiette de levée** : `army_class_free` lit `prov[]` au lieu de `region[]` ET applique une fraction mobilisable `ARMY_POOL_FRAC` (registre J) | `scps_army.c:317-322` · `wh_country_elite` `scps_warhost.c:190-195` | **0,15-0,25** rend impossible par construction les 26 % / 43 % / 56 % / 93 % mesurés, en laissant intacte la médiane hégémon de **2,4 %** | Faible : la médiane observée (2,4 %) est 6× sous le plafond proposé. Golden bouge (levées bornées). Corrige aussi G1. |
| **P4** | **Le seep dépose au PRORATA des classes prélevées**, pas 100 % en journaliers | `scps_econ.c:6301-6302` (+ relocalisations `:6621-6637`) | supprime la pompe qui fait passer les bourgeois de **15 % → 8 %** et les élites de **5 % → 2 %** sur 200 ans | Modéré : la répartition à l'arrivée devient 80/15/5 de fait, plus proche du semis. Golden bouge. Aucun tunable neuf. |
| **P5** | **Le plancher de population devient PROVINCIAL, pas par strate** : `if (Σstrata < 1) province morte`, ou plancher sur `CLASS_LABORER` seul | `scps_econ.c:5261` | supprime les **12,5 % de lignes-pays** à pyramide fabriquée, dont le cas « 89 % de nobles pour 9 âmes » | Faible, mais touche une expression au cœur du tick ⇒ golden. Interaction à vérifier avec les ruines (`is_colonized && !colonized`). |
| **P6** | **Débrider la promotion J→B là où il n'y a pas d'atelier** : remplacer le gate binaire `manuf = n_bld>0` par un gate sur `build.PE_infra + build.K_inst` (marché/comptoir/tribunal comptent aussi comme débouché) | `scps_econ.c:3657` | débloque la classe moyenne dans les **77-93 % de provinces T1** ; cible : ramener la fin de sim de 89/8/2 vers **85/12/3** | Modéré. Le plafond doux 0,32 protège de l'emballement. |
| **P7** | **L'esclavage prend sur la RÉGION et dépose où il y a de la place** : boucler `diplo_enslave_capture` sur les provinces de la région prise, choisir la province d'accueil avec `n_groups` minimal (pas la seule représentative de la capitale) | `scps_diplo.c:1402-1413` | à `SLAVE_FRACTION`=0,05 inchangé, un sac de région à 5 provinces × 1 500 hab rapporte **~375 âmes** au lieu de ~75 ; le stock mondial passerait de ~6 à quelques milliers, soit **~0,5 %** de la pop — assez pour que `SLAVE_REVOLT_SHARE`=0,20 puisse mordre localement | Modéré. **À trancher par le joueur** : le brief 2026-07-21 dit explicitement « taux très faible » — P7 augmente le VOLUME sans toucher au TAUX. |
| **P8** | **Nommer les deux réalités dans l'UI** : la fiche province affiche « sièges d'emploi », la fiche pays « classes par richesse » (aujourd'hui les deux disent « classes ») | `scps_api.c:1554` (sièges) vs `:1751` (strates) — libellés `strings_ids.h` | 0 changement moteur, supprime la contradiction visible (province 100 % journaliers / pays 89/8/2) | Nul côté moteur. `lang-check` à repasser. |
| **P9** | **Un tier de capitale, pas deux** : `prov_elite_seats_ex` lit `capitale_max_tier(Σstrata)` comme tous les autres appelants | `scps_demography.c:576` | aligne G6 ; les sièges suivent alors la population RÉELLE de la province | Golden bouge. À faire avec P1/P2, pas avant. |
| **P10** | **`religion_refresh_region` somme les provinces de la région** au lieu de lire la représentative | `scps_religion.c:131-141` | supprime G3 ; coût O(n_prov) une fois/mois | Golden bouge ; la foi dominante de certaines régions changera. |
| **P11** | **Instrumenter `Σcount` vs `Σstrata`** dans `chronicle.c` (une ligne près de `SIÈGES`) | `chronicle.c:1786-1822` | mesure, aujourd'hui absente, de l'écart entre les deux ledgers — **prérequis de toute re-calibration** | Nul (print-only, golden intact). **À faire EN PREMIER.** |

### Ordre recommandé

**P11** (mesurer) → **P2** (réparer l'invariant, conservatif) → **P1 + P9 + P4** (une seule
vague, un seul re-baseline) → **P3** → **P5, P6** → **P7, P10** (décisions joueur) →
**P8** (UI).

---

## 7. Ce que ce rapport N'a PAS pu établir

- **Le rapport `Σ pop_by_class` / `Σ strata` du monde** : aucun journal ni aucun diag
  existant ne l'imprime (d'où P11). Toutes les conclusions de §4.1-4.2 sont établies par
  lecture de code + le comptage provinces/régions, pas par une mesure directe du ratio.
- **L'effet du re-key d'influence sur une trajectoire de 200 ans** : le sweep est
  antérieur. La seule comparaison possible (Σ générée 83 117 en 100 ans post-re-key vs
  79 495 en 200 ans pré-re-key, graine 7, mêmes arguments) est **indicative** : les deux
  mondes divergent dès l'an 2 et le binaire porte aussi les correctifs de guerre `38523b6`.
- **La part exacte de l'expansion due au seep passif** : le chronicle compte les
  fondations DIRIGÉES (96-295/sim) et le total colonisé (374-752), mais pas le seep.
