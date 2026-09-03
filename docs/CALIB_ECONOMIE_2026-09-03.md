# CALIBRAGE ÉCONOMIQUE — coûts, entretiens, prix, leviers (2026-09-03)

**Périmètre** : lecture seule. Aucun fichier moteur touché, aucun `make`, aucun sweep long.
**Sources** : `scps/scps_econ.c`, `scps/scps_agency.c`, `scps/scps_intertrade.c`,
`scps/scps_sim.c`, `scps/scps_decrees.c`, `scps/scps_tune_list.h`, `docs/LEVIERS.md`,
et les **20 journaux** de `sweep_doct_ai_10x200/` (10 graines × 200 ans × 2 bras,
protocole `manifest.txt`, binaire `8140fa92…`).
**Méthode de lecture des logs** : 5 journaux lus intégralement (`temoin_s7`, `essai_s7`,
`essai_s1009`, `essai_s11`, `essai_s2026`) ; pour les 20, lecture intégrale de tout le
registre économique (an 40/80/120/160, bloc monétaire, bloc par-empire, bloc « par âge »,
manufactures, friche, spéculation, détroits, trésor/flux décomposé, accession). Seules
les listes de worldgen (`PROV n ville=…`, dispatch des brutes, biomes) ont été écartées
après lecture d'un exemplaire complet — elles ne portent aucun chiffre économique.

---

## 0. Les 5 lois qu'il faut avoir en tête avant de lire les tables

| # | Loi | Site |
|---|---|---|
| L1 | **Un édifice ne coûte RIEN en or à un empire qui possède la matière.** `intertrade_buy_cost` facture uniquement la part IMPORTÉE ; le stock de l'empire est à marge 0. | `scps_intertrade.c:456-461` |
| L2 | **L'entretien d'un édifice = poids de son delta × 2,661 or/mois**, indépendant du prix de marché. `(W × 35 / 400) × 365/12` avec `W = K + 1,5·H + P + PE + food + port + faith + savoir`. | `scps_econ.c:2988-2994`, constantes `:2929-2930`, `:2955` |
| L3 | **L'entretien de base n'est prélevé que sur le surplus au-dessus de `SINK_FLOOR`=500 / province** ; **cour, admin et encadrement des manufactures ne mordent qu'au-dessus de `COURT_FLOOR`=4000 / province**. | `scps_econ.c:4560` (`paid_up`), `:4590`, `:4614`, `:4624` |
| L4 | **Les manufactures de l'IA sont GRATUITES** : `econ_build_tick` (§NF v2) sème un bâtiment de niveau 1 chaque tick dans toute province en pénurie — sauf celle du joueur (`pe->owner == g_econ_human`). Le joueur seul paie `MANUF_BUILD_COST`=50 × ipm. | `scps_econ.c:2819-2853` (gate joueur `:2834`), `scps_sim.c:712` et `:733` |
| L5 | **Tous les prix flottent contre `price_level[pays] = Σ(trésor−500) / VA_pays`**, plancher `0,15×base×pl`, plafond `8×base×pl`, cible `base×pl×clamp(demande/offre, 0,2, 6)`. | `scps_econ.c:3933`, `:5390-5398`, lecteur `scps_econ.h:889` |

Conséquence immédiate : **le prix affiché au menu construction est un prix que presque
aucun empire ne paie**, et **la charge récurrente d'un bâtiment est le seul coût réel**.

---

## 1. Table des ÉDIFICES — coût, entretien, retour

`W` = poids d'entretien. `entretien/mois = W × 2,661`. Matière = trio bois/pierre/argile,
multipliée par `agency_extent_mult = 1 + 0,15 × n_régions` (`scps_agency.c:296-303`).
« or nominal » = Σqty × plancher de prix `BUILD_MIN_PRICE`=0,20 (`scps_agency.c:283`),
étendue 1 — **c'est le prix d'un empire NU** ; « or réel » = 0 dès qu'une tuile bois +
une tuile argile sont dans l'empire (L1). « seuil d'amortissement » = habitants
supplémentaires nécessaires pour couvrir l'entretien au **plancher fiscal**
(`TAX_BASE_LABORER` 0,06 × `TAX_FLOOR_FRAC` 0,5 = 0,03 or/mois/tête,
`scps_tune_list.h:177`, `:685`).

| Édifice | j | recette b/p/a | Σqty | W | entretien/mois | or nominal | or réel | amort. (hab.) |
|---|---|---|---|---|---|---|---|---|
| Tribunal (`:46`) | 180 | 20/–/10 | 30 | 1,0 | **2,66** | 6,0 | 0 | 89 |
| Chancellerie (`:47`) | 360 | 20/10/5 | 35 | 2,5 | **6,65** | 7,0 | 0 | 222 |
| Académie (`:48`) | 960 | 10/20/10 | 40 | 4,5 | **11,98** | 8,0 | 0 | 399 |
| Garnison (`:51`) | 360 | 20/10/5 | 35 | 1,5 | **3,99** | 7,0 | 0 | 133 |
| Forteresse (`:52`) | 540 | 5/20/10 | 35 | 4,5 | **11,98** | 7,0 | 0 | 399 |
| Citadelle (`:53`) | 960 | 5/20/15 | 40 | 9,0 | **23,95** | 8,0 | 0 | 798 |
| Port (`:56`) | 360 | 25/10/10 | 45 | 2,0 | **5,32** | 9,0 | 0 | 177 |
| Caravansérail (`:57`) | 180 | 20/–/10 | 30 | 0,7 | **1,86** | 6,0 | 0 | 62 |
| Marché (`:59`) | 180 | 20/–/10 | 30 | 1,0 | **2,66** | 6,0 | 0 | 89 |
| Entrepôt (`:60`) | 180 | 20/–/10 | 30 | 0,7 | **1,86** | 6,0 | 0 | 62 |
| Grenier (`:62`) | 180 | 20/5/10 | 35 | 1,0 | **2,66** | 7,0 | 0 | 89 |
| Irrigation (`:63`) | 360 | 25/10/5 | 40 | 1,5 | **3,99** | 8,0 | 0 | 133 |
| Aqueduc (`:64`) | 540 | 5/25/10 | 40 | 1,2 | **3,19** | 8,0 | 0 | 106 |
| Sanctuaire (`:72`) | 180 | 10/–/– | 10 | 1,0 | **2,66** | 2,0 | 0 | 89 |
| Temple (`:73`) | 540 | 5/20/10 | 35 | 3,0 | **7,98** | 7,0 | 0 | 266 |
| Cathédrale (`:74`) | 960 | 10/40/20 | 70 | 6,5 | **17,30** | 14,0 | 0 | 577 |
| Bibliothèque (`:77`) | 360 | 20/10/5 | 35 | 1,5 | **3,99** | 7,0 | 0 | 133 |
| Monastère (`:78`) | 540 | 5/15/10 | 30 | 3,5 | **9,32** | 6,0 | 0 | 311 |
| Comptoir (`:80`) | 180 | 20/–/10 | 30 | 0,8 | **2,13** | 6,0 | 0 | 71 |
| Banque (`:81`) | 540 | 5/20/10 | 35 | 1,4 | **3,73** | 7,0 | 0 | 124 |
| Arsenal (`:87`) | 540 | 5/20/15 | 40 | 2,8 | **7,45** | 8,0 | 0 | 249 |
| Amirauté (`:89`) | 540 | 5/20/10 | 35 | 2,4 | **6,39** | 7,0 | 0 | 213 |
| Port marchand (`:91`) | 540 | 5/15/10 | 30 | 2,5 | **6,65** | 6,0 | 0 | 222 |
| Bibliothèque militaire (`:93`) | 360 | 20/10/5 | 35 | 1,8 | **4,79** | 7,0 | 0 | 160 |
| Observatoire (`:95`) | 360 | 20/10/5 | 35 | 1,8 | **4,79** | 7,0 | 0 | 160 |
| Centre commercial (`:100`) | 540 | 10/30/15 | 55 | 2,5 | **6,65** | 11,0 | 0 | 222 |

**Lecture.** La province médiane du monde à l'an 200 pèse 800-2 000 habitants
(échantillon `PROV n … pop=` de `temoin_s7`). **Tous** les édifices sauf la Citadelle
(798 hab.) et la Cathédrale (577) sont amortis d'office par la seule population qui est
déjà là. Aucun édifice n'a de « temps de retour » au sens strict : le coût en or est nul,
l'entretien est marginal, l'effet (K/H/P/PE/food/faith/savoir) est permanent.
**Aucun édifice « mort »** : les 26 sont bâtis dans les sweeps (télémétrie
`agency_edi_notech_count` + « 245 à 492 édifice(s) refusé(s) faute de tech de palier/sim »,
donc le gate qui mord est la TECH de palier, jamais l'or).

**Preuve dans les logs** : la ligne `chantiers` du flux décomposé (dernière année,
or/mois/empire) vaut **−0,0 à −5,5** sur 20/20 sims, contre **+356 à +2 658** de taxes.
(`temoin_s7:974` `chantiers -0.0` ; `essai_s3333` `chantiers -5.5` ; `essai_s1009`
`chantiers -3.5`.)

---

## 2. Table des MANUFACTURES — coût, entretien, VA, retour

Coût joueur : `MANUF_BUILD_COST` 50 × ipm × décret ATELIERS × doctrine « Gages »
(`scps_sim.c:712`). La pose crée un bâtiment **niveau 5** (`scps_econ.c:3357`) ;
`CMD_MANUF_LEVEL +1` ajoute 5 niveaux pour le même prix (`MANUF_LEVEL_STEP`=5,
`scps_econ.c:3376`). ipm mesuré fin de partie : **0,86-0,88** ⇒ coût réel ≈ **43-44 or**.

Entretien : `econ_job_upkeep_month = 0,60 × ouvriers × 0,06 × ipm / max(prix, 0,5×base)`
(`scps_econ.c:3006-3016`) — **prélevé uniquement si le trésor de la province > 4 000**
(L3). Mesuré : ligne `encadr.` = **0,0 à −7,8** or/mois/empire sur 20/20 sims.

VA/lot et VA/ouvrier calculées **aux prix de base** (`BASE_PRICE`, `scps_econ.c:303-360`),
à `market_effort` = 1,10 (prix = base, `scps_econ.c:863-866`). Un « lot » = 1 unité de
`cap` ; un niveau 5 tourne à `cap` = 5,5 lots/mois.

| Manufacture (`scps_econ.c`) | recette | VA/lot | labor/lot | **VA/ouvrier** | ouvriers @ niv.5 | VA/mois @ niv.5 | retour (mois) |
|---|---|---|---|---|---|---|---|
| Atelier d'outillage `:465` | fer 1 + bois 1 → 3 outils | **22,10** | 0,9 | **24,56** | 5 | 121,6 | **< 1** |
| Manufacture textile `:427` | laine 1,5 → 2,8 étoffe | 9,90 | 1,0 | 9,90 | 6 | 54,5 | **1-2** |
| Atelier de mage `:459` | cristal 1 → 1 essence | 18,00 | 1,3 | 13,85 | 7 | 99,0 | **< 1** |
| Alambic `:488` | salpêtre 1,2 → 1 flux | 8,16 | 0,9 | 9,07 | 5 | 44,9 | **1-2** |
| Joaillerie `:447` | or 0,2 → 0,5 orfèvrerie | 9,40 | 1,2 | 7,83 | 7 | 51,7 | **1-2** |
| Atelier d'arc `:497` | fer 1 + bois 1 → 1 trait | 6,60 | 0,9 | 7,33 | 5 | 36,3 | **2** |
| Atelier de sculpture `:501` | pierre 2 → 1 statue | 7,40 | 1,1 | 6,73 | 6 | 40,7 | **2** |
| Chancellerie de luxe `:512` | bois 1 + argile 1 → 1 registre | 7,40 | 1,1 | 6,73 | 6 | 40,7 | **2** |
| Poudrière `:502` | salpêtre 1 + charbon 0,8 → 1 poudre | 6,36 | 1,0 | 6,36 | 6 | 35,0 | **2** |
| Armurerie légère `:494` | fer 1,2 → 1 arme (×10 au stock) | 6,12 | 1,0 | 6,12 | 6 | 33,7 | **2** |
| Armurerie lourde `:496` | fer 3 → 1 lourde | 6,80 | 1,1 | 6,18 | 6 | 37,4 | **2** |
| Atelier serein `:514` | bois 1 + laine 1 → 1 ouvrage | 6,20 | 1,1 | 5,64 | 6 | 34,1 | **2** |
| Heaumerie `:509` | fer 1 + charbon 1 → 1 heaume | 4,80 | 1,1 | 4,36 | 6 | 26,4 | **3** |
| Comptoir d'artisan `:513` | cuivre 1 + sel 1 → 1 colifichet | 4,20 | 1,1 | 3,82 | 6 | 23,1 | **3** |
| Parurier `:510` | or 0,25 + fourrure 1 → 1 parure | 4,00 | 1,1 | 3,64 | 6 | 22,0 | **3** |
| Horloger `:511` | fer 1 + cuivre 1 → 1 horloge | 4,00 | 1,1 | 3,64 | 6 | 22,0 | **3** |
| Forge céleste `:462` | fer cél. 2 + charbon 1 → 1 arme ench. | 4,20 | 1,4 | 3,00 | 8 | 23,1 | **3** |
| Scierie navale `:428` | bois 2 + cuivre 0,2 → 1 fourniture | 1,48 | 0,8 | 1,85 | 4 | 8,1 | **10** |
| Réplicateur `:491` (×2 faust.) | flux 0,5 → 8 bois | 2,00 (10,0) | 1,4 | 1,43 (7,1) | 8 | 11,0 (55) | 7 (**2**) |
| Distillerie `:437` | sucre 1,6 → 1,4 eau-de-vie | 3,80 | 38 | **0,100** | 209 | 20,9 | 4 |
| Poterie `:500` | argile 1,5 → 1,4 poterie | 3,30 | 46 | **0,072** | 253 | 18,2 | 5 |
| Brasserie `:438` | grain 1,2 → 1 bière | 1,80 | 27 | **0,067** | 149 | 9,9 | 9 |
| Tunique `:456` | étoffe 1 → 1 tunique | 1,00 | 28 | **0,036** | 154 | 5,5 | 16 |
| Papeterie `:433` | bois 1,5 → 1 papier | 4,00 | 209 | **0,019** | 1 150 | 22,0 | 4 |
| Apothicaire `:505` | herbes 1 → 1 remède | 3,50 | 404 | **0,0087** | 2 222 | 19,3 | 5 |
| **Étoffe précieuse `:453`** | murex 0,1 + **4 étoffes** → 1 précieuse | **−1,10 → 0** | 1,1 | **0** | 6 | 0 | **jamais** |
| **Charbonnière `:469`** | 2 bois → 1 charbon | **−0,20 → 0** | 0,8 | **0** | 4 | 0 | **jamais** |
| **Corne divine `:492`** (×2 faust.) | fer cél. 0,5 → 8 grain | **−2,00 → 0** (6,0) | 1,4 | **0** (4,3) | 8 | 0 (33) | **jamais** (2) |
| **Arquebuserie `:498`** | fer 1 + **poudre 2** → 1 arme à feu | **−8,40 → 0** | 1,1 | **0** | 6 | 0 | **jamais** |
| **Foreuse arcanique `:483`** (×2 faust.) | **essence 0,7** → 2 fer | **−19,0 → 0** (−14,2) | 1,4 | **0** | 8 | 0 | **jamais** |

Note : la VA est clampée à 0 (`va = fmaxf(0, val_out − val_in)`, `scps_econ.c:4383`) —
une recette négative ne détruit pas de richesse comptable, elle **brûle son intrant pour
rien**.

### 2.1 Les bâtiments OP (retour < 2 ans) — c'est-à-dire TOUS sauf 5

Le prix de 50 or est **décoratif**. La part de la VA qui remonte au trésor
(`buy_pay`, `scps_econ.c:2518-2531`, clé 42/20/38 puis impôt sur le revenu
0,40/0,55/0,75 — `scps_tune_list.h:1633-1635`) vaut ≈ **0,57 × VA** avant évasion et
exonération vitale. Même à 30 % net, l'atelier d'outillage rembourse **43 or en moins
d'un mois** et le pire des rentables (la tunique) en **16 mois**.

**Le vrai déséquilibre n'est pas le prix : c'est `labor`.** L'écart de VA par ouvrier va
de **0,0087** (apothicaire) à **24,56** (outillage) — un facteur **2 800×**. Le levier
`labor` a été recalé (E3 2026-07-05, commentaires `scps_econ.c:429-435`, `:454-455`,
`:499`, `:504`) sur les **6 biens du panier** (papier, eau-de-vie, bière, tunique,
poterie, remède) selon `labor = 1200 × qout / demande_1000hab`. Les **24 autres**
recettes sont restées à `labor` 0,8-1,4, jamais recalées.

### 2.2 Les bâtiments MORTS

| Bâtiment | Diagnostic | Preuve |
|---|---|---|
| **Scierie navale** | Son bien n'a **aucun acheteur** : `RES_NAVAL_SUPPLIES` n'est consommé que par `scps_navy.c` et `scps_campaign.c:509/577`. `NAVY_COMBAT_ON`=0 (décision joueur 2026-08-16). | **20/20 sims** : « 0 coque(s) bâtie(s) · **0 fournitures navales consommées** (NE doit plus être zéro) ». Pourtant **22 à 35 scieries** par sim brûlent bois + cuivre. |
| **Foreuse arcanique** | VA −19/lot ; l'essence (34) coûte 7× le fer qu'elle produit. | Absente de **17/20 sims** ; présente à 1 ou 7 exemplaires dans 3 sims (`essai_s2026` 1, `essai_s777` 7, `temoin_s90` 1). « conso foreuse 0 » dans 17/20. |
| **Arquebuserie** | VA −8,4/lot : 2 poudres (22) pour une arme à feu (16). | 1 à 36 par sim, médiane ~7 ; « armes produites » ne distingue pas, mais le gate `TECH_POUDRIERE` + la VA nulle l'expliquent. |
| **Étoffe précieuse** | VA −1,1/lot : 4 étoffes (18,0) = exactement le prix de la précieuse (18,0), plus la teinture. | 32 à 117 par sim — **bâtie mais structurellement non-rentable** ; ne tourne que quand l'étoffe est au plancher et la précieuse au plafond. |
| **Réplicateur / Atelier de mage / Forge céleste** | Chaîne arcane : 0-17, 1-3 (sauf `essai_s2026` 77), 1-5 par sim. | Voir tables « manufactures » des 20 logs. |

---

## 3. Revenus et charges d'État par taille (an 200, 20 sims, 350 lignes-empire lues)

### 3.1 Trésor et solde par taille (an 200)

| Taille | Obs. | Trésor (or) | Solde (or/mois) |
|---|---|---|---|
| 1 région, pop < 5 k | 18 | **0 – 467** | 0,0 à +3,6 |
| 1-2 régions, pop 2-25 k | 21 | **1 – 3 061** | −3,6 à +24,5 |
| 3-6 régions | 34 | **293 – 6 749** | −43,1 à +34,5 |
| 7-16 régions | 22 | **979 – 17 807** | −86,4 à +78,4 |
| 17-30 régions | 16 | **3 061 – 29 509** | −290,2 à +75 |
| 31-49 régions | 17 | **9 293 – 59 383** | −290,2 à +399,7 |
| 50-78 régions (hégémon) | 21 | **20 149 – 95 122** | −317,5 à **+1 779,5** |

**Trésor moyen par empire, fin de sim** : 4 521 (`essai_s512`) à **40 003**
(`essai_s777`), médiane ≈ 19 000. **Flux moyen** : −74,9 à +315,4 or/mois — jamais un
effondrement.

### 3.2 Le budget d'État décomposé (dernière année, or/mois/empire — 20 sims)

| Poste | Min | Max | Médiane approx. |
|---|---|---|---|
| taxes | +356,3 | **+2 658,1** | ≈ +1 300 |
| frappe (faucet) | +51,1 | **+807,5** | ≈ +240 |
| export | +3,5 | +26,1 | +10 |
| péages+ | +0,6 | +2,7 | +1,0 |
| **redépense** | −66,6 | **−344,5** | ≈ −150 |
| **soldes (armée)** | −20,1 | **−274,7** | ≈ −70 |
| **entretien (bâti)** | −16,2 | **−131,1** | ≈ −50 |
| conseil | −4,5 | −95,1 | −35 |
| cour | −3,0 | −59,3 | −22 |
| import | −4,3 | −29,8 | −12 |
| intérêts | −0,1 | −6,3 | −1,5 |
| encadrement (manufactures) | 0,0 | **−7,8** | −2,5 |
| **chantiers** | 0,0 | **−5,5** | ≈ −0,2 |
| **admin** | **−0,1** | **−0,5** | −0,3 |
| audits · marine · invest. · routes · intrigue | 0,0 | −5,9 | 0,0 |

**Anomalie majeure : `admin` est mort.** `ADMIN_BASE` 0,4 × n^(1,3−1) × ipm par province
(`scps_econ.c:4626`) donnerait, pour un empire de 60 régions, **≈ 80 or/mois** au total.
Mesuré : **−0,1 à −0,5 or/mois/empire**, soit **99 % de manque**. Cause : le gate
`re->treasury > COURT_FLOOR (4000)` est **PAR PROVINCE** (`:4624`) alors que le trésor
d'un hégémon à 95 122 or réparti sur 70 provinces fait **1 359 or/province** — sous le
seuil. Même cause pour `cour` (`:4614`) et `encadr.` (`:4590`). **Trois des quatre
freins anti-thésaurisation ne se déclenchent presque jamais.**

### 3.3 Assiette fiscale mondiale et dette dans le temps

| Année | Revenu fiscal Σ monde (or/an) | dette/revenu moyen |
|---|---|---|
| 40 | 1 413 – 17 738 (méd. ≈ 8 000) | **7 % – 245 %** (méd. ≈ 130 %) |
| 80 | 10 737 – 53 524 (méd. ≈ 26 000) | 4 % – 206 % (méd. ≈ 60 %) |
| 120 | 23 943 – 138 191 (méd. ≈ 85 000) | 0 % – 71 % (méd. ≈ 13 %) |
| 160 | 20 030 – **370 225** (méd. ≈ 180 000) | 0 % – 73 % (méd. ≈ **7 %**) |

**Où le trésor part en spirale** : nulle part à l'an 200. Sur **350 lignes-empire**,
exactement **deux** trésors négatifs (`or -58` `essai_s3333` Ordre Dornwica ;
`or -42` `essai_s512` Ordre Caelwic). La dette est un phénomène de **premier siècle**
qui se résout seul (130 % → 7 %). La vraie pathologie de pauvreté est la **friche** :
`16 à 47 régions impayées` par sim, soit **5-13 %** des provinces colonisées, production
× `FRICHE_FACTOR` 0,6 (`scps_econ.c:2931`).

**Où l'or déborde** : chez l'hégémon. 20 149 à 95 122 or, croissance +58 à +1 780/mois.
Une part **structurellement immobilisée** : `SINK_FLOOR` 500 × n_provinces —
soit ≈ **35 000 or gelés** dans un empire de 70 provinces (37 % du trésor de
`temoin_s1009`). Le reste échappe aux sinks par §3.2.

**Masse monétaire** : `M(0)` 58-60 k → `M(fin)` **2,11 M à 15,49 M** (×36 à ×258),
dérive **+10 263 à +77 176 or/an**. La frappe seule vaut 9 269 à 30 907 or/an/monde,
dont **1 412 à 42 460 or/an de sur-frappe (débase)** — 6 à 24 pays débasent en fin de
partie.

**La saisie de banqueroute est le plus gros transfert du modèle après la frappe.**
`BANKRUPTCY_GARNISH` 0,75 confisque **75 % de la valeur de TOUTE la production
manufacturière** du failli pendant `BANKRUPTCY_SCAR_YEARS` = 10 ans décroissants
(`scps_econ.c:4277-4288`). Mesuré : **104 484 à 1 934 548 or/sim** confisqués
(27 939 à 147 676 or par banqueroute), pour **3 à 19 banqueroutes/sim** — alors que
la **dette mondiale totale** en fin de sim ne vaut que **430 à 109 116 or**. La saisie
déplace donc **10 à 200× l'encours qui l'a déclenchée**.

---

## 4. Prix : fourchettes réelles, cycles, effondrements

### 4.1 Le mécanisme (à lire avant les chiffres)

```
target = BASE_PRICE[r] × price_level[pays] × clamp(demande_nat / offre_nat, 0,2 ; 6)
prix   = prix_t-1 × 0,65 + target × 0,35                    (PRICE_INERTIA)
prix   = clamp(prix, BASE × 0,15 × pl , BASE × 8 × pl)
price_level[c] = clamp( Σ(trésor_province − 500) / VA_pays_t-1 , 0 , INFLATION_CAP=2 )
```
`scps_econ.c:3933`, `:5390-5398`, `:4836-4837`. Or et cuivre sont **exemptés** de
`price_level` (numéraire, `:5393`).

Conséquence : **le niveau général des prix est le ratio caisse d'État / valeur ajoutée.**
Comme la VA croît beaucoup plus vite que les trésors, **le monde est structurellement
déflationniste** : indice 1,00 à la genèse → 0,143 à 0,702 en moyenne de partie.

### 4.2 Fourchettes mesurées (20 sims, blocs « par âge »)

| Bien (base) | Âge des Découvertes (an 3-13) | Âge des Empires (an 54-68) | Âge de la Brèche (an 181) |
|---|---|---|---|
| grain (1,0) | 0,15 – **3,06** | 0,04 – 0,77 | **0,08 – 0,63** (méd. 0,16) |
| étoffe (4,5) | 0,10 – 1,23 | 0,45 – 1,56 | **1,05 – 2,74** (méd. 2,30) |
| orfèvrerie (22,0) | 0,20 – 4,53 | 2,00 – 27,96 | **9,34 – 39,96** (méd. 25) |
| outils (8,5) | 1,02 – **31,71** | 1,52 – 13,18 | **3,73 – 9,76** (méd. 5,4) |
| fer (2,4) | — | — | moy **0,2 – 1,3** · max 0,6 – 27,9 |
| or (parité 16,0) | — | — | **3,15 – 10,40** |
| cuivre (parité 5,2) | — | — | **0,52 – 6,09** |

**Effondrements récurrents.** Le grain, le fer et l'étoffe vivent en permanence contre
leur **plancher** `0,15 × base × pl` : leur ratio demande/offre est collé au clamp bas
0,2, et `pl` ≈ 0,3-0,5 ⇒ prix d'équilibre ≈ 6-10 % de la base. À l'inverse orfèvrerie et
outils sont **gatés par la demande** (`GATE_DEMAND_BUFFER` 1,25, `scps_econ.c:684` ;
`TOOLS_PER_LABORER` 0,05, `:687`) donc leur ratio sature au clamp haut 6 ⇒ prix
= 1,8-3,0 × base × pl. **Il n'y a pas de cycle : il y a deux régimes fixes**, plancher
ou plafond, selon que le bien a ou non une demande bornée.

Un cas limite lu tel quel : `temoin_s7` et `essai_s7`, âge des Soulèvements
(an 8) — `marché : grain 0.00 · étoffe 0.00 · orfèvr. 0.00 · outils 0.00`. Prix **tous
nuls** l'année de l'avènement de l'âge, puis retour à 0,27/0,04/0,20/2,28 deux ans plus
tard. `econ_avg_price` ne moyenne que sur les régions colonisées **actives** ; un instant
d'agrégation avant reconstruction de `region[]` rend 0. À vérifier (voir §6).

### 4.3 « prix 0,377 vs 0,443 » du `resume.txt` : ce n'est PAS un prix

`tools/sweep_doct_ai.sh:106` étiquette `prix %5.3f` un champ alimenté par
`/inflation \(M7-I1\)/ { … if($i=="moy"){ p+=$(i+1) } }` — c'est-à-dire
`econ_world_price_index` (`scps_econ.h:889`), **la moyenne pondérée-VA des
`price_level` par pays**, donc `caisse/VA`. Recoupé à la main sur les 20 lignes
`inflation (M7-I1) : indice moy …` :

* essai : 0,548 + 0,391 + 0,465 + 0,143 + 0,325 + 0,360 + 0,336 + 0,386 + 0,514 + 0,297 = 3,765 → **/10 = 0,3765 ≈ 0,377** ✔
* témoin : 0,457 + 0,430 + 0,410 + 0,247 + 0,390 + 0,702 + 0,404 + 0,496 + 0,599 + 0,297 = 4,432 → **/10 = 0,4432 ≈ 0,443** ✔

**Pourquoi l'essai est 15 % plus bas** : le bras `AI_DOCT=1` fait adopter en moyenne
33,1 doctrines/monde, dont **Production 62 fois** (la plus prise) — qui multiplie
`MANUF_QOUT_MULT` et `RAW_BOOST_PER_TIER` (`scps_doctrines.c:99`). Plus de VA, même
caisse ⇒ `caisse/VA` baisse. **Les doctrines sont déflationnistes par construction.**
Ce n'est pas un effondrement de prix de marché — le grain médian à la Brèche est
0,16 (essai) contre 0,15 (témoin) : identique.

### 4.4 Portée du marché et péages

| Levier | Valeur | Site |
|---|---|---|
| `MARKET_DIST_FALLOFF` | 0,12 / saut | `scps_tune_list.h:783` |
| `IMPORT_MARGIN_OWN/THIRD/NONE` | 1,3 / 1,8 / 2,0 | `:181-183` |
| `TRADE_LEVY` (importateur) | 0,10 | `:186`, site `scps_intertrade.c:1020` |
| `IT_CHOKE_TOLL` (détroit) | 0,12 × (0,4+0,6·étroitesse) = **4,8-12 %** | `scps_intertrade.c:28`, `:1061` |
| péage de chantier | **toute** la marge (`gold − base_gold`), split `TOLL_STATE_SHARE` 0,5 | `scps_agency.c:390-406` |

**Mesure** : péage de détroit **cumulé sur 200 ans** = **0 à 5 166 or par SIM ENTIÈRE**
(médiane ≈ 1 100 ; 3/20 sims à 0). À comparer aux 20 000-95 000 or du trésor d'un seul
hégémon. **Les péages de détroit sont négligeables à l'échelle du siècle** — le meilleur
tenant encaisse 33 à 4 259 or en 200 ans.

**Les hubs restent aux cités-états** : 48 % à 100 % du commerce mondial passe par leurs
Centres (100 % dans **14/20** sims) ; les empires tiennent 0 à 2 hubs sur 4-23.
`puissance commerciale : pool moy 254,9 à 1 756,3 /mois · 6 à 33 achats BORNÉS` — le
plafond du pool commercial mord peu.

---

## 5. Récurrences et trucs « OP »

| # | Truc | Chiffre | Site |
|---|---|---|---|
| OP1 | **Bâtir est gratuit** dès qu'on possède la matière (trio bois/pierre/argile). | ligne `chantiers` = −0,0 à −5,5 or/mois/empire vs +356 à +2 658 de taxes, 20/20 sims | `scps_intertrade.c:456-461` |
| OP2 | **Rénover est gratuit** : `RENOV_COST_FRAC` 0,5 × un coût qui vaut 0. Vétusté 2 %/an, plancher 50 % ⇒ aucune raison de ne pas rénover en boucle. Pas de dilemme. | — | `scps_agency.c:762-765`, `:720-739` |
| OP3 | **L'IA reçoit ses manufactures gratuitement** (§NF v2, niveau 1, chaque tick, toute province en pénurie) ; **seul le joueur paie 50 or**. | 39 à 222 exemplaires par type et par sim | `scps_econ.c:2819` (gate joueur `:2834`) |
| OP4 | **Trois sinks anti-hoarding morts** (cour/admin/encadrement) : gate `trésor > 4 000` **par province**, trésor réel ≈ 1 359/province chez l'hégémon. | admin mesuré −0,3 vs ~80 nominal (−99 %) | `scps_econ.c:4590`, `:4614`, `:4624` |
| OP5 | **`labor` non recalé** hors panier : VA/ouvrier de 0,0087 à 24,56 (×2 800). | table §2 | `scps_econ.c:427-514` |
| OP6 | **Coloniser ne coûte pas d'or** : 150 âmes (≤ 25 % de la classe libre), aucun débit. | 78 à 358 fondations/sim ; ligne `colonisation +0/an` du résidu de périmètre | `scps_econ.c:718`, `:6034` |
| OP7 | **La saisie de banqueroute** déplace 10 à 200× l'encours qui l'a déclenchée. | 104 k à 1,93 M or/sim confisqués vs 430 à 109 k de dette mondiale | `scps_econ.c:4277` |
| OP8 | **La débase est systématique** : l'IA la prend dès `DEBASE_AI_ONSET_YEARS`=2 au plafond de dette. | 1 412 à 42 460 or/an créés ; 6 à 24 pays débasent en fin de partie | `scps_tune_list.h:938-943` |
| OP9 | **Le crédit ne mord qu'au premier siècle** : taux moyen 2,2-21,8 % (méd. 3,3 %), 0 à 5 dettes structurelles ≥3× par sim, dette/revenu 130 % (an 40) → 7 % (an 160). | §3.3 | `scps_tune_list.h:868-873` |
| OP10 | **Allocation** : `alloc_bld` à 0 ferme un bâtiment sans coût ni contrepartie (0 ouvrier, 0 intrant, 0 sortie) ; l'override joueur est un interrupteur gratuit. | — | `scps_econ.c:4142-4144` |

**Anomalie de télémétrie** : la mesure causale du lissage par entrepôt se contredit.
« σ centres À entrepôt vs centres SANS » : **6/20 sims montrent les centres AVEC entrepôt
plus volatils** (`essai_s7` 0,732 vs 0,666 ; `essai_s2026` 1,198 vs 0,978 ;
`temoin_s2026` idem). Le titre de la ligne annonce « les stocks doivent LISSER » ; la
mesure ne le confirme pas dans un tiers des sims.

**Anomalie de trésor gelé** : plusieurs polities mineures affichent `or 2000
(+0.0/mois)` exactement — la valeur de `GENESIS_TREASURY_EMPIRE`
(`scps_econ.c:1780`) — à l'an 200 (`essai_s7` Ligue Dornredor, `essai_s512` Couronne
Morgoryn). Équilibre taxes/redépense ou trésor réellement figé : à départager.

---

## 6. Propositions chiffrées, classées par impact

> Aucune n'est appliquée. Chacune porte son site, son chiffre et son risque.
> Toutes celles qui touchent le moteur exigent un **re-baseline golden** documenté.

### P1 — Rendre les freins anti-thésaurisation opérants (impact MAX, risque MOYEN)

**Constat** : §3.2 — `admin` −99 % de son nominal, `cour` et `encadr.` du même ordre,
parce que `COURT_FLOOR` = 4 000 est comparé au trésor **d'une province** quand la
richesse est **d'un pays**.

**Proposition A — minimale, zéro nouvelle structure** :
`COURT_FLOOR` **4 000 → 1 200** (`scps_tune_list.h:159`). 1 200 est juste sous le trésor
médian par province d'un hégémon mesuré (1 359 = 95 122 / 70, `temoin_s1009`).
Attendu : `cour` de −22 à ≈ −150 or/mois/empire ; `admin` de −0,3 à ≈ −45 ;
`encadr.` de −2,5 à ≈ −25. Le trésor moyen d'empire passerait de ≈ 19 000 à ≈ 12 000.
**Risque** : `COURT_FLOOR` sert AUSSI de seuil à la surcharge IPM de l'entretien
(`scps_econ.c:4590`) — l'abaisser frappe les petites polities les premières.
**Kill-switch** : `SCPS_TUNE=COURT_FLOOR=4000` (byte-identique).
**Gate** : sweep apparié 3 graines × 3, lire `trésor moy`, `friche (E1bis.10)` (aujourd'hui
16-47 rég), satisfaction Laborer (aujourd'hui 38-61 %).

**Proposition B — juste mais plus de code** : comparer au trésor **national/n_provinces**
au lieu du trésor local. Hors budget d'une passe de calibrage ; à consigner pour une
vague dédiée.

### P2 — Recaler `labor` sur les 7 luxes hors panier (impact FORT, risque FAIBLE)

**Constat** : §2.1 — VA/ouvrier ×2 800 ; les 7 luxes de niche gagnent 3,6 à 6,7 or par
ouvrier quand la bière en gagne 0,067.

**Chiffres proposés** (`scps_econ.c`, colonne `labor` de `RECIPE[]`) :

| Bâtiment | `labor` actuel | proposé | VA/ouvrier après |
|---|---|---|---|
| `BLD_SCULPTURE` `:501` | 1,1 | **6,0** | 6,73 → **1,23** |
| `BLD_HEAUMERIE` `:509` | 1,1 | **6,0** | 4,36 → **0,80** |
| `BLD_PARURIER` `:510` | 1,1 | **6,0** | 3,64 → **0,67** |
| `BLD_HORLOGER` `:511` | 1,1 | **6,0** | 3,64 → **0,67** |
| `BLD_CHANCELLERIE_LUX` `:512` | 1,1 | **6,0** | 6,73 → **1,23** |
| `BLD_COMPTOIR_ARTISAN` `:513` | 1,1 | **6,0** | 3,82 → **0,70** |
| `BLD_ATELIER_SEREIN` `:514` | 1,1 | **6,0** | 5,64 → **1,03** |

**Pourquoi ces 7 et pas les autres** : ils sont **hors `NEED_ORDER`** de base, servis en
« désir croisé » d'éthos (`scps_econ.c:4818-4823`) — aucun banc de satisfaction ne les
gate. Toucher papeterie/apothicaire/poterie recalibrerait le panier de besoins et donc la
démographie : **à ne PAS faire dans la même vague**.
**Risque** : `labor` est surchargeable par `SCPS_MODS` (`scps_econ.c:7010`, `:7027`) —
vérifier que le dump/load de modtools reste cohérent. **Golden : re-baseline obligatoire.**

### P3 — Faire payer la matière maison au chantier (impact FORT, risque MOYEN)

**Constat** : OP1 + doctrine UI « le MENU CONSTRUCTION = la vérité de TOUT ». Le menu
annonce un prix (`agency_build_gold`) qu'aucun empire autosuffisant ne paie.

**Site** : `scps_intertrade.c:461` — `(void)p_emp; /* l'empire est GRATUIT (marge 0) */`.
**Proposition** : nouveau tunable `BUILD_OWN_MATERIAL_PRICE` (défaut **0 = legacy exact,
golden byte-identique** ; 1,0 = prix de revient plein), et
`return unit_price*(p_emp*own + p_near*base + p_dist*base*2.f)`.
**Chiffrage à 1,0** : Tribunal = 30 unités × prix bois/argile (0,15-1,0 mesuré) ×
`ext` (1 + 0,15 × 50 régions = 8,5) ≈ **40 à 250 or** au lieu de 0. Sur ~300 édifices
bâtis en 200 ans par le monde entier : ordre de **40 k or**, soit ≈ 1,5 trésor d'hégémon
— mordant, pas bloquant.
**Risque** : le gate de matière (`scps_agency.c:359-370`) est inchangé ; seul le gate
d'or (`credit_can_spend`, `:377`) mord davantage → l'**accession 960 j** (mesurée
**an 29 à 51**, moyenne 41) peut reculer. **Gate** : re-mesurer la ligne
`accession (E1 §9)` sur 3 graines ; refuser si le 960 j dépasse l'an 120.

### P4 — Purger 5 entrées fantômes de `docs/LEVIERS.md` (impact MOYEN, risque NUL)

`MANUF_UPKEEP_DAY` (0,05), `CREDIT_LINE_BASE` (0,5), `CREDIT_RATE_BASE` (0,05),
`CREDIT_RATIO_CAP` (8,0), `ARB_CAPTURE` (0,35) — lignes **89, 96, 97, 98, 113** de
`docs/LEVIERS.md`. Vérifié : **0** occurrence dans `scps/scps_tune_list.h`, **0** appel
`tune_f("…")` dans tout `scps/*.c`. Exactement le motif `IMPORT_TOLL_FRAC` purgé le
2026-09-01 (TROUVAILLES). `MANUF_UPKEEP_DAY` en particulier a été **remplacé** par
`econ_job_upkeep_month` (M3d, `scps_econ.c:3006`) et la doc annonce encore l'ancien.
**Docs seules, moteur intact, golden-neutre.**

### P5 — Exposer 6 constantes économiques structurantes au registre J (impact MOYEN, risque FAIBLE)

Aujourd'hui **aucun sweep ne peut les sonder** (ni `SCPS_TUNE`, ni panneau F10) :

| Constante | Valeur | Site | Ce qu'elle pilote |
|---|---|---|---|
| `BUILD_GOLD_PER_DELTA` | 35 | `scps_econ.c:2930` | **tout** l'entretien du bâti (table §1) |
| `DEF_UPKEEP_MULT` | 1,5 | `scps_econ.c:2955` | surcoût de la famille H (Garnison→Citadelle) |
| `STATE_SPEND_RATE` | 0,30/an | `scps_econ.c:2787` | la redépense — 2e poste de charge (−66 à −345/mois) |
| `NF_SHORTAGE` | 1,8 | `scps_econ.c:674` | le seuil qui sème gratuitement les manufactures IA |
| `PRICE_INERTIA` | 0,65 | `scps_econ.c:706` | la vitesse des prix (cycles §4) |
| `FRICHE_FACTOR` | 0,6 | `scps_econ.c:2931` | la punition de la friche (16-47 rég/sim) |

Motif : `tune_f("X", X)` au site + `X(X, défaut)` au registre ⇒ **golden byte-identique
par construction** (le défaut ne change pas). 12 autres `#define` économiques sont dans
le même cas (`PAYROLL_FRACTION`, `STATE_TAX_AMBITION`, `TAX_RATE`, `WAGE_SHARE`,
`GATE_DEMAND_BUFFER`, `DEMAND_TENSION`, `BUILD_MIN_PRICE`, `MANUF_LEVEL_STEP`,
`IT_CHOKE_TOLL`, `COLONY_SEED_POP`, `RENOV_DAYS`, `NF_SEED_LEVEL`) — les 6 ci-dessus
sont ceux dont l'effet est mesurable au sweep.

### P6 — Corriger l'étiquette « prix » du sweep (impact MOYEN, risque NUL)

`tools/sweep_doct_ai.sh:106` affiche `prix %5.3f` pour une quantité qui est
`econ_world_price_index` = caisse/VA (§4.3). **Renommer `indice`**, et ajouter un vrai
champ de prix (par ex. le grain du dernier âge, déjà présent dans chaque log :
`marché : grain X`). Outillage seul, aucun impact moteur.

### P7 — Rendre 4 recettes structurellement rentables (impact FAIBLE-MOYEN, risque MOYEN)

Les 5 recettes à VA négative (§2, dernières lignes) ne produisent que lorsque le ratio de
prix bascule complètement (intrant au plancher 0,15×pl, sortie au plafond 8×pl — un
écart de 53×). Corrections **au choix, une seule à la fois** :

| Recette | Aujourd'hui | Proposition | VA/lot après |
|---|---|---|---|
| `BLD_WEAVER_LUX` `:453` | murex 0,1 + **étoffe 4,0** → 1 précieuse | étoffe **4,0 → 2,5** | −1,10 → **+6,40** |
| `BLD_ARQUEBUS` `:498` | fer 1 + **poudre 2,0** → 1 arme à feu | poudre **2,0 → 1,0** | −8,40 → **+2,60** |
| `BLD_CHARCOAL` `:469` | **2,0 bois** → 1 charbon | bois **2,0 → 1,5** | −0,20 → **+0,30** |
| `BLD_CORNE` `:492` | fer cél. 0,5 → 8 grain | *ne rien faire* — sa raison d'être est la **nourriture**, pas l'or (1 à 118 exemplaires/sim, « corne 306 à 16 089 » de conso) | — |

**Risque** : la charbonnière est le déblocage de la fonderie (commentaire
`scps_econ.c:466-468`) — toucher son ratio touche toute la chaîne fer. Gate : lire
`FER prix moy` (aujourd'hui **0,2 à 1,3** pour une base 2,4) et le compte
`Atelier d'outillage` (63-190/sim).

### P8 — Chaîne navale : ne rien changer, mais ne plus semer (impact FAIBLE, risque FAIBLE)

**Décision joueur actée** : `NAVY_COMBAT_ON` = 0, coques OFF (TROUVAILLES 2026-08-16,
« non c'est pas high ou 2, c'est OFF »). **Je ne re-litige pas.** Constat de coût
seulement : **22 à 35 scieries navales par sim** (20/20) consomment bois + cuivre pour un
bien dont la télémétrie dit elle-même « 0 fournitures navales consommées (NE doit plus
être zéro) ». Proposition minimale : ajouter au §NF v2 (`scps_econ.c:2836-2842`, la série
de gates `if (b==BLD_… && !pe->tech_…) continue;`) une ligne
`if (b==BLD_SAWMILL && tune_f("NAVY_COMBAT_ON",0.f)<=0.f) continue;` — le bois et le
cuivre retournent à l'outillage et à l'horlogerie. **Golden : re-baseline.**

---

## 7. Sondes exécutées — 3 runs `chronicle 7 1 120 6 12`

Trois runs **séquentiels**, même graine, même monde, un seul tunable de différence.
**1 sim, 1 graine : indicatif, PAS un gate.** Toute décision exige le sweep apparié 3×3.

| Mesure (an 120) | baseline | `ENTRETIEN_DIV=100` (entretien ×4) | `MANUF_BUILD_COST=250` (×5) |
|---|---|---|---|
| **cour** or/mois/empire | **+0,0** | **+0,0** | **+0,0** |
| **admin** or/mois/empire | **+0,0** | **+0,0** | **+0,0** |
| **encadrement** or/mois/empire | **+0,0** | **+0,0** | **+0,0** |
| taxes or/mois/empire | +232,2 | **+133,3** (−43 %) | +180,0 (−22 %) |
| entretien or/mois/empire | −29,2 | **−65,8** (×2,25, pas ×4) | −30,3 |
| chantiers or/mois/empire | −1,2 | −0,4 | −0,2 |
| trésor moy/empire | 8 292 | 7 535 (−9 %) | 6 531 (−21 %) |
| flux moy or/mois | −6,8 | +7,1 | −2,3 |
| friche (rég impayées) | **75** | **54** | **46** |
| satisfaction Laborer | 48 % | 43 % | 41 % |
| indice de prix moy | 0,473 | **0,286** (−40 %) | 0,510 |
| M(fin) | 950 518 | 889 409 | 735 422 |
| accession 960 j | an 39 | an 51 | an 50 |
| colonisation (fondations) | 152 | 137 | **205** |
| Σ manufactures (ordre) | ≈ 2 000 | ≈ 2 000 | ≈ 2 050 |

**Trois enseignements fermes.**

1. **`cour`, `admin` et `encadr.` valent exactement `+0.0` dans les TROIS bras.** Le
   diagnostic §3.2 / P1 (« les trois freins anti-thésaurisation sont morts ») est confirmé
   sous trois calibrages différents, pas seulement dans le sweep 200 ans. C'est la
   trouvaille la plus solide du rapport.
2. **`ENTRETIEN_DIV` est un levier non-linéaire et cher en satisfaction.** Le diviseur ÷4
   ne multiplie l'entretien réellement PRÉLEVÉ que par **2,25** (il est borné au surplus
   au-dessus de `SINK_FLOOR`, `scps_econ.c:4560`) — mais il coûte **−43 % de taxes**,
   **−5 points de satisfaction Laborer** et **−40 % d'indice de prix** (déflation
   aggravée : moins de caisse pour la même VA). Contre-intuitif : la **friche BAISSE**
   (75 → 54) — le compte de friche n'est **pas monotone** en ce levier. Conclusion :
   `ENTRETIEN_DIV` n'est PAS le bon levier pour reprendre le trésor de l'hégémon ;
   c'est `COURT_FLOOR` (P1) qui vise la bonne cible.
3. **`MANUF_BUILD_COST` ×5 ne change PAS le nombre de manufactures** (≈ 2 000 dans les
   deux bras) — **preuve directe de OP3** : le §NF v2 les sème gratuitement pour l'IA
   (`scps_econ.c:2819`), le prix ne mord que sur le joueur et sur l'initiative privée
   (`econ_ip_invest_tick`, `:6770`). Le levier ne fait que **redistribuer les types**
   (papeterie 85 → 47, armurerie lourde 113 → 69, poudrière 31 → 1, arquebuserie 24 → 1
   contre textile 81 → 126, joaillerie 124 → 158, poterie 156 → 196).

## 8. Ce que je n'ai PAS pu mesurer

* Les chiffres attendus de **P1** (`COURT_FLOOR` 4 000 → 1 200) et **P3**
  (`BUILD_OWN_MATERIAL_PRICE`) restent des **estimations analytiques** : P3 exige une
  modification du moteur (interdite dans cette mission) et P1 mérite le sweep apparié
  3×3 plutôt qu'une sim unique.
* `econ_scan` n'existe pas en binaire (`scps/econ_scan.c` présent, `econ_scan.exe`
  absent) et `make` est interdit dans cette mission — la table « AUDIT BÂTIMENTS
  (supply/demand par bien) » qu'il produit manque donc au rapport.
* Le cas « prix tous nuls à l'an 8 » (§4.2) est **reproduit** dans la sonde baseline
  (`marché : grain 0.00 · étoffe 0.00 · orfèvr. 0.00 · outils 0.00` au 2e âge) mais n'a
  pas pu être instrumenté.
