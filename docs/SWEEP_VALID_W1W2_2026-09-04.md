# DÉPOUILLEMENT — sweep de VALIDATION des vagues W1/W2 (50 graines × 250 ans, coupé)

Dossier `sweep_valid_W1W2_50x250/` · protocole `sweep-doct-ai-paired-3x3-v1`
(`reps_per_seed=1`, `horizons=250`, `empires=6`, `city_states=12`) · binaire
`2bb5301c…` · lancé 2026-09-04T02:43Z, **coupé par le joueur** vers 09:00Z.
Bras : `temoin` = `SCPS_TUNE=AI_DOCT=0` · `essai` = défaut.

---

## 0. LE CORPUS RÉEL — le brief dit 17 paires, il y en a 13

**Correction de la prémisse.** Le brief annonçait « 35 journaux complets, tous les
`.rc` à 0, 17 paires ». La lecture des `.rc` donne autre chose :

| état | journaux | graines |
|---|---|---|
| `.rc = 0`, journal complet (700-1091 lignes) | **27** | voir ci-dessous |
| `.rc = 127`, journal **tronqué à 67-68 lignes** (worldgen seul, sim jamais entrée) | 6 | `essai s5 s13 s17 s19` · `temoin s13 s17 s19 s23` |
| dossier `incomplets/` (8 journaux, pas de `.rc`) | 8 | s23 s29 s31 s37 s41 |

`essai_s13_y250.log` s'arrête ligne 67 sur le bloc `biomes (prov/cellules)` : c'est
la sortie du worldgen, la boucle de sim n'a jamais commencé. Ces six-là ne sont pas
des « journaux complets », ce sont des runs tués au même instant que ceux
d'`incomplets/` — seul le `.rc` a été écrit avant la coupure.

**Paires témoin/essai réellement complètes : 13** — graines
**1 · 2 · 3 · 7 · 11 · 60 · 90 · 512 · 777 · 1009 · 2026 · 3333 · 4243**.
**Orphelins : 2** — `temoin_s5` (complet, 982 lignes, `.rc`=0 ; son essai est un
tronc) et `temoin_s23` (tronc, à jeter).

**Méthode.** Les 27 journaux complets (25 274 lignes) ont été ouverts et lus :
en-têtes worldgen, colonnes an 50/100/150/200, blocs monétaires, `BILAN an 250`,
bloc DOCTRINES, dumps `PROV` (lus intégralement pour s7 ×2, s1009 ×2, s4243 ×2),
lignes `PROV total`/`CLASSES`/`SIÈGES`/`LEDGERS`, listes d'empires vivants, blocs
par âge, et la SYNTHÈSE finale de chaque sim. Le `resume.txt` du script **n'existe
pas** (le sweep a été coupé avant l'étape de dépouillement) : il n'y a donc pas de
table de départ, tout ce qui suit vient de la lecture. `grep -n` n'a servi qu'à
retrouver une ligne déjà lue et à afficher côte à côte 27 exemplaires d'une même
ligne pour dresser les tables ; aucun compteur, aucun filtre découvrant.

**Sanité générale.** 6 âges dans 27/27. Aucun ASSERT, aucun NaN, aucun `inf`,
aucune fin prématurée. Invariant M3f : pic 93 % à **201 %** (`temoin_s3333:111`)
pour un seuil de 370 % — tous passent. `acharnement 0` partout, `top ≤30 %`
partout (14-16 %).

---

## 1. VERDICT GLOBAL APPARIÉ (13 paires) ET COMPARAISON AU SWEEP PRÉCÉDENT

### 1.1 Les médianes appariées

| mesure (médiane sur 13 paires) | témoin | essai | Δ | sweep 10×200 (2026-09-02) |
|---|---:|---:|---:|---|
| pays subsistants an 250 | 38 | **33** | **−13 %** | 27,7 → 29,6 (+6,9 %) |
| guerres déclenchées | 245 | 219 | −11 % | 63,3 → 61,1 (−3,5 %) |
| M(fin) | 12,65 M | 12,97 M | +2,5 % | 6,10 M → 6,84 M (+12 %) |
| indice de prix (M7-I1) | 0,058 | 0,074 | +28 % | 0,443 → 0,377 (−15 %) |
| **prix du grain (médiane, base 1,00)** | **0,232** | **0,228** | ≈ 0 | *n'existait pas* |
| trésor moy / empire | 7 633 or | 6 612 or | −13 % | 20 692 → 18 343 (−11 %) |
| flux moy / empire | −20,6 or/mois | −29,2 or/mois | — | *n'existait pas* |
| doctrines actives (essai) | 0 | **82** | — | 33,1 (10×200) |
| idées possédées (essai) | 0 | **424** | — | 179,8 |
| adoptions cumulées (essai) | 0 | **96** | — | ~33 |
| juge côte (ADOPTANTS, pondéré) | 0 % | **30 %** | — | **77 %** |
| juge vassal (pondéré) | 0 % | **71 %** | — | 69 % |
| juge guerre (pondéré) | 0 % | **86 %** | — | **62 %** |
| satisfaction Laboureur | 49 % | 52 % | +3 pt | 47,6 → 49,3 |
| richesse/tête Laboureur | 5,18 | 4,78 | −8 % | 1,80 → 2,51 (+39 %) |
| âges advenus | 6,0 | 6,0 | 0 | 6,0 |
| fins §27 différentes entre bras | — | **5 / 13 (38 %)** | — | 4 / 10 (40 %) |

### 1.2 Ce qui a bougé PAR RAPPORT AU SWEEP PRÉCÉDENT

**Le kill-switch tient toujours** : `DOCTRINES (P3-IA) : 0 pays vivants … 0 doctrines
actives` et juges 0 %/0 %/0 % dans les 13 témoins et dans `temoin_s5`.

1. **L'arbre est passé de « verrouillé à l'an 10 » à « joué toute la partie ».**
   Adoptions cumulées 96 (méd.) pour 82 doctrines actives : l'écart (~15 %)
   mesure les pays morts en route, mais surtout l'arbre continue de bouger
   après l'an 150 (`essai_s512:81` 34 adoptions à l'an 100 → `:91` 62 à l'an 200 ;
   `essai_s2:82` 34 → `:91` 142). Au sweep précédent l'arbre était gelé passé
   l'an 120. **C'est le gain le plus net de W1-E.**
2. **La distribution s'est retournée** (1 209 doctrines actives sur 13 sims) :

| doctrine | n | part | 10×200 (part) | | doctrine | n | part | 10×200 |
|---|---:|---:|---:|---|---|---:|---:|---:|
| **Production** | 213 | 17,6 % | 18,7 % | | Vassaux | 59 | 4,9 % | 7,3 % |
| **Offense** | 158 | 13,1 % | *6,9 %* | | **Colonisation** | 53 | 4,4 % | ***14,2 %*** |
| **Défense** | 151 | 12,5 % | *6,6 %* | | Mercantilisme | 33 | 2,7 % | 3,6 % |
| Infrastructure | 146 | 12,1 % | 11,5 % | | Bourgeoisie | 27 | 2,2 % | *0,3 %* |
| **Technologie** | 116 | 9,6 % | ***0,6 %*** | | **Aristocratie** | 19 | 1,6 % | ***13,0 %*** |
| Commerce | 88 | 7,3 % | 11,2 % | | Peuple | 3 | 0,2 % | 0,6 % |
| **Connaissances** | 83 | 6,9 % | ***0,6 %*** | | Diplomatie | 2 | 0,2 % | 1,2 % |
| Populaire | 57 | 4,7 % | 3,6 % | | **Faustien** | **1** | 0,1 % | **0 %** |
| | | | | | **Divin** | **0** | **0 %** | **0 %** |

   - **Technologie ×16, Connaissances ×11,5** : les doctrines « tardives » de
     §3.5(d) du sweep précédent sont devenues des choix ordinaires. Les slots ne
     sont plus verrouillés à l'an 10.
   - **Faustien sort du code mort** : 1 adoption, `essai_s11:113`. Marginale, mais
     l'affirmation « Faustien ne peut PAS être adopté » est désormais fausse.
   - **Aristocratie s'effondre (13,0 % → 1,6 %)** et **Populaire prend la tête des
     courants** (57 sur 103 courants adoptés, soit 55 %, contre 21 % avant). Le
     courant IA différé à l'an 40 (W1-E) a fait exactement ce qu'il annonçait.
   - **Colonisation s'effondre aussi (14,2 % → 4,4 %)** — voir A12, c'est le prix payé.
3. **Le juge martial est réparé** (62 % → 86 % pondéré ; 100 % dans
   `essai_s512:115`, 96 % dans `essai_s2:115`, 93 % dans `essai_s777:115`). Le
   diagnostic « la guerre arrive après la saturation des slots » ne tient plus.
4. **Le juge côtier est cassé** (77 % → 30 %). Voir A12.
5. **Le monde est PLUS pauvre et PLUS déflaté qu'au sweep précédent** :
   indice 0,443/0,377 → 0,058/0,074 (÷6 à ÷5), trésor 20 692/18 343 → 7 633/6 612
   (÷2,7). Une partie vient du changement d'assiette (trésor national), une partie
   du prix du grain (§2.4).
6. **L'essai FRAGMENTE MOINS que le témoin** (33 pays contre 38) — l'inverse du
   sweep précédent. Deux mondes portent l'exception (`essai_s2` 61 pays contre 49,
   `essai_s7` 42 contre 30) ; ailleurs l'arbre stabilise (`essai_s777` 32 contre 45,
   `essai_s4243` 32 contre 41, `essai_s90` 33 contre 41).

### 1.3 Le tableau par graine (13 paires)

| graine | bras | pays | guerres | M(fin) | indice | **grain** | doct. | juges côte/vassal/guerre | fin §27 |
|---|---|---:|---:|---:|---:|---:|---:|---|---|
| **1** | t | 40 | 232 | 12,60 M | 0,015 | 0,217 | 0 | 0/0/0 | GRAND HIVER |
| | e | 38 | 219 | 13,25 M | 0,009 | **0,000** | 82 | 21/75/79 | GRAND HIVER |
| **2** | t | 49 | 284 | 19,02 M | 0,048 | 1,102 | 0 | 0/0/0 | RONCES |
| | e | **61** | **347** | 16,46 M | 0,060 | 0,143 | **233** | **13**/61/**96** | **EAU** |
| **3** | t | 48 | 284 | 10,61 M | 0,096 | **0,000** | 0 | 0/0/0 | EAU |
| | e | 47 | **380** | 11,94 M | 0,084 | **0,000** | 159 | 20/75/88 | EAU |
| **7** | t | 30 | 141 | 16,41 M | 0,117 | 0,212 | 0 | 0/0/0 | RONCES |
| | e | **42** | 200 | 22,96 M | 0,056 | 0,168 | 114 | 21/100/76 | **EAU** |
| **11** | t | 37 | 257 | 10,65 M | 0,080 | 0,048 | 0 | 0/0/0 | RONCES |
| | e | 37 | 286 | 12,96 M | 0,171 | 0,233 | 89 | 60/75/88 | RONCES |
| **60** | t | 30 | 279 | 18,21 M | 0,058 | 0,232 | 0 | 0/0/0 | RONCES |
| | e | 33 | 280 | 24,71 M | 0,134 | 0,233 | 79 | 31/50/76 | **GRAND HIVER** |
| **90** | t | 41 | 304 | 12,65 M | 0,095 | **2,985** | 0 | 0/0/0 | GRAND HIVER |
| | e | 33 | 264 | 18,14 M | 0,060 | 0,281 | 88 | 62/57/89 | GRAND HIVER |
| **512** | t | 29 | 245 | 19,24 M | 0,058 | 0,199 | 0 | 0/0/0 | RONCES |
| | e | 33 | 216 | 20,06 M | 0,078 | **1,154** | 65 | 38/60/**100** | RONCES |
| **777** | t | **45** | 271 | 27,18 M | 0,153 | **0,000** | 0 | 0/0/0 | EAU |
| | e | 32 | 250 | 37,34 M | 0,158 | 0,228 | 65 | 38/75/93 | **RONCES** |
| **1009** | t | 38 | 186 | 6,83 M | 0,003 | 1,583 | 0 | 0/0/0 | GRAND HIVER |
| | e | 41 | 205 | 9,69 M | 0,074 | 1,370 | 102 | 18/71/91 | GRAND HIVER |
| **2026** | t | 34 | 196 | 11,78 M | 0,001 | 0,932 | 0 | 0/0/0 | GRAND HIVER |
| | e | 30 | **114** | 8,66 M | 0,003 | 1,134 | 36 | 71/100/71 | **RONCES** |
| **3333** | t | 30 | 206 | 10,76 M | 0,019 | 0,317 | 0 | 0/0/0 | GRAND HIVER |
| | e | 31 | 184 | 12,64 M | 0,032 | 0,213 | 48 | 60/80/73 | GRAND HIVER |
| **4243** | t | 41 | 169 | 15,65 M | 0,252 | 1,208 | 0 | 0/0/0 | GRAND HIVER |
| | e | 32 | 160 | 12,97 M | 0,083 | **0,000** | 49 | 67/80/70 | **RONCES** |
| *(orphelin)* | **temoin s5** | 33 | 324 | 16,52 M | 0,039 | 0,857 | 0 | 0/0/0 | GRAND HIVER |

Fins §27 : 6 GRAND HIVER · 5 RONCES · 2 EAU (témoin) ; 5 · 5 · 3 (essai).
**RÉCHAUFFEMENT, ASCENSION, SANG et « aucune » ne sortent JAMAIS** (0 sur 27) — la
ligne se dénonce elle-même : `ratio max/min dispatch 99.9:1, cible ≤2:1`.

---

## 2. CHAQUE CORRECTIF, VÉRIFIÉ DANS LES JOURNAUX

### 2.1 TRÉSOR / STOCK NATIONAUX (W1-A) — **PARTIEL**

| mesure | attendu (TROUVAILLES) | lu (médiane 13 paires) | verdict |
|---|---|---|---|
| invariant M3c/M3f | sous le seuil 370 % | pic 93-**201 %** (`temoin_s3333:111`) | **TENU** |
| trésor moy/empire | ~17 k (mesure W1 à 120 ans) | t **7 633** · e **6 612** | PARTIEL |
| flux moy/empire | équilibré | t **−20,6** · e **−29,2** or/mois | PARTIEL |
| recoupement I0 « hors registre » | ~+150 or/mois/empire (an 120, s7) | t **−1 519** · e **−1 891** or/mois/empire | **RATÉ** |
| empires ruinés (or = 0) | rares | 3 à 8 lignes-pays à `or 0` par journal | à surveiller |

Le recoupement I0 est le point noir. **Les 27 journaux ont un « hors registre »
NÉGATIF**, c'est-à-dire que la Σ des postes surestime systématiquement le flux
réellement mesuré au trésor :

- `temoin_s7:993` : `Σ postes +4082.5 · flux mesuré +4.7 · hors registre **−4077.8**`
  (trésor moyen 26 671) — le registre annonce 4 082 or/mois d'entrées nettes, la
  caisse en encaisse 4,7.
- `essai_s777:899` : `Σ postes +3661.7 · mesuré −121.5 · hors registre −3783.2`.
- `temoin_s512:768` : `+3395.5 / +6.9 / −3388.7`. `essai_s4243:749` : `+2872.2 / +149.2 / −2723.0`.
- Le moins mauvais : `temoin_s3:997` `+322.5 / +22.9 / −299.6`.

Le sens constant (toujours négatif, jamais de compensation) exclut « quelques
buckets FX_* manquants » comme seule explication : un poste manquant serait
aléatoire en signe. **Ce qui manque est une SORTIE massive et permanente** — de
l'ordre de 20 à 40 % de la ligne `taxes` — que le registre ne voit pas. Le reste
W2-4 (« hors registre +149,9 ») était une mesure d'an 120 sur 2 empires ; à 250 ans
et sur 7-46 empires l'écart est d'un autre ordre.

**Cour / admin / encadrement** (le réveil W2-1, `COURT_MONTHS=60`) : à **+0,0
exactement** dans **16 journaux sur 27**, dont `temoin_s7:992`, `temoin_s1009:671`,
`essai_s1009:684`, `temoin_s4243:736`, `essai_s4243:748`, `essai_s2026:763`,
`temoin_s1:878`, `essai_s1:896`, `temoin_s2:959`. Là où il mord, c'est faible
(`essai_s7:1023` −1,3/−1,1/−1,1) sauf `temoin_s777:882` (**cour −45,4**, encadr.
−11,1) et `temoin_s3:996` (cour −6,4). **Le frein anti-thésaurisation ne mord que
dans 11 mondes sur 27.** PARTIEL.

### 2.2 FREIN DE LEVÉE (W1-A × W1-F, `WH_DESERT_RATE` 0,5 / `WH_PAY_REVENUE_FRAC` 0,35) — **RATÉ sur la queue, TENU sur la médiane**

| mesure | attendu | lu | verdict |
|---|---|---|---|
| **rgt / limite de force, médiane** | ~1,0× (W1-F : « 2,02 → 1,03 ») | t **30 %** · e **45 %** | TENU |
| **rgt / limite, MAX** | ≤ 1,03× | t **médiane des max 218 %**, pire **432 %** · e **253 %**, pire **331 %** | **RATÉ** |
| solde / revenu, médiane | ≤ 35 % | t **18 %** · e **18 %** | TENU |
| solde / revenu, MAX | ≤ 35 % (plafond de croissance) | t pire **19 066 %** · e pire **3 656 %** | **RATÉ** |
| rgt désertés / sim | 112 (mesure an 120) | t **1 918** · e **2 062** | fonctionne (fort) |
| mois-pays sur-budget | 73 % (an 120) → « régime voulu ? » | t **57 %** · e **54 %** | amélioré |
| empires à 0 rgt avec régions | doit disparaître | **présent, y compris chez le 1ᵉʳ empire** | **RATÉ** (A5) |

Citations : `temoin_s3:1017` `armée / limite de force … médiane 74 % · max **432 %**` ·
`essai_s11:846` `max 331 %` · `essai_s3333:780` `max 325 %` · `essai_s4243:769`
`max 313 %` · `temoin_s60:972` `solde / revenu … max **19 066 %**` ·
`temoin_s2:981` `max 4 836 %` · `temoin_s3:1018` `max 3 981 %` ·
`essai_s512:831` `max 3 656 %` (et le pays : `essai_s512:712` `Clans Zenilel
2 rég · or 275 · solde/revenu 3656 %`).

Le frein a **déplacé la médiane** (30-45 % de la limite, contre 2,02× avant W1) et
**déserte massivement** (2 000 rgt/sim) — mais il n'a pas de prise sur la queue :
un empire sur 13-46 finit régulièrement à 2-4× sa limite de force, et le rapport
solde/revenu n'a **aucune** borne supérieure observable.

### 2.3 POPULATION (W1-B + W2-2 : invariant, F2, LEDGERS) — **TENU, le plus net de la vague**

| mesure | attendu | lu (27 journaux) | verdict |
|---|---|---|---|
| Σ sièges = Σ âmes-groupes | écart 0 | **`écart +0 = 0.0 %` dans 27/27** | **TENU** |
| groupes hors invariant | 0 | **`0 groupe(s) hors invariant (Σ|écart| 0 âmes)` dans 27/27** | **TENU** |
| âmes / strates | ~100 % (23,6 % avant W2-2) | **97,8 % à 100,4 %** | **TENU** |
| provinces FIGÉES 100 % journaliers | 11 % (an 120, s7) | **médiane 12 % (t) / 11 % (e)** mais **5 % → 61 %** | PARTIEL |
| sièges d'élite (monde) | ~13 % (règle joueur) | **médiane 7 %** (min 3 %, max 8 %) | PARTIEL |
| assiette des classes | 56/22/22 visé | sièges **76 J / 13 B / 7 É** (médiane) | PARTIEL |
| écart âmes/strates PAR PROVINCE | « 27,7 % » (pic W2-2) | **max 130 941 %** (`temoin_s3:814`) | **anomalie A8** |

Le cœur du chantier — l'invariant et le ledger F2 — est **parfait sur les 27
journaux**, sans une exception. C'est le correctif le mieux tenu de la vague.

Ce qui reste : les **FIGÉES sont bimodales**. Un monde sur deux est à 5-12 %
(`essai_s7:870` 5 %, `temoin_s777:711` 5 %, `essai_s3:848` 6 %) et l'autre moitié
à 35-61 % (`temoin_s2026:641` **61 %**, `temoin_s1:728` **54 %**,
`temoin_s90:779` **51 %**, `temoin_s3333:629` **51 %**, `temoin_s4243:586` **50 %**).
Le détail par pays montre des empires entiers gelés : `temoin_s90:781`
`Clans Bramwickka 180 prov (**68 % figées**)`, `essai_s1009:530` `Ordre Khazdin
22 prov (**91 % figées**)`, `temoin_s1009:523` `Ordre Khazdin 86 % · Havre Nimtop 86 %`.
La corrélation lue : les mondes figés sont ceux à faible population par province
(l'arrondi aux paquets de 100 annule les sièges d'élite — diagnostic W1-B confirmé,
pas refermé).

### 2.4 ÉCONOMIE (W2-1 : `PL_SINK_MONTHS=3`, `COURT_MONTHS=60`, matière maison) — **PARTIEL, le prix du grain n'est pas réparé**

| mesure | attendu | lu | verdict |
|---|---|---|---|
| **prix du grain, médiane** | 0,00 → **1,06** (s7, an 120) | **0,232 (t) / 0,228 (e)** pour une base 1,00 | **RATÉ** |
| grain **exactement 0,000** | disparu | **5 sims sur 27** | **RATÉ** |
| cour/admin/encadrement | réveillés | à `+0,0` dans 16/27 | PARTIEL |
| chantiers (matière maison facturée) | −6,7 or/mois (s7 an 120) | −0,1 à **−10,7** (`essai_s1:896`) — non nul partout | TENU |
| manufactures privées/sim | 403 → 3 306 | médiane **3 611 (t) / 2 779 (e)**, étendue **268 → 12 886** | TENU (dispersion énorme) |
| friche (rég impayées) | 50 → 27 | **20 à 47** (`essai_s7:954` 44, `essai_s4243:679` 47, `temoin_s512:698` 22) | PARTIEL |
| édifices refusés faute de palier | 159-330 | médiane **362 (t) / 361 (e)**, max **748** (`essai_s1009:713`) | à trancher |
| satisfaction J/B/É | 50/84/64 (s7 an 120) | méd. **52/70/62** (e), **49/68/63** (t) | TENU |

Le grain : 5 mondes à **0,000** exactement (`temoin_s3:102`, `temoin_s777:102`,
`essai_s1:101`, `essai_s3:101`, `essai_s4243:101`), 13 mondes sous 0,25, et
5 mondes au-dessus de 0,9 dont un à **2,985** (`temoin_s90:102`). Le plancher
indexé sur `price_level` continue de s'effondrer là où le trésor national est bas :
`essai_s4243:101` `médiane 0.000 · moyenne 1.180` — la moyenne est saine, la
médiane est à zéro, donc **la majorité des provinces ont un prix nul pendant que
quelques-unes tiennent le prix**. Ce n'est pas une queue, c'est le régime.

À l'inverse la ligne `marché :` du bloc « par âge » se tient mieux
(`essai_s512:759` `grain 0.64 · étoffe 2.94 · orfèvr. 48.34 · outils 7.80` à
l'an 181) : le grain moribond est un fait **de province**, pas un fait de marché.

### 2.5 TECH & HÉRITAGE (W1-C : ruines, `TECH_COST_N_EXP` 0,65, barre d'héritage) — **PARTIEL**

| mesure | attendu | lu | verdict |
|---|---|---|---|
| arbre RECHERCHÉ moyen/empire | cible 40-60 % | **37 % (t) / 42 % (e)** | **TENU** |
| **arbre RECHERCHÉ, MAX (le leader)** | cible 40-60 %, prévu 78-80 % à l'an 200 | **100 % dans 23 journaux sur 27** | **RATÉ** |
| **arbre HÉRITÉ (§27)** | mesurable seulement après l'an 180 | **non nul dans 4 journaux** : `temoin_s777:851` 2 empires 77 % · `temoin_s3:965` 6 empires 91 % · `essai_s2:1009` **11 empires 92 %** · `essai_s3:1000` 1 empire 90 % | **TENU** (la ligne vit enfin) |
| combos tier-4 | 12 → 3 (an 120) | **37 à 319** à l'an 250 | non concluant |
| nœuds faustiens | 14-51 (10×200) | **22 à 81** | vivant |
| ruines / `TECH_EVEIL` | rebranchées sur la matière arcane | **aucune trace nommée dans la télémétrie** | non mesurable ici |
| archétypes distincts | 6/6 | 6/6 partout **sauf s3333 (5/6, les deux bras)** | à noter |

Le leader termine **l'arbre entier** (74/74) dans 23 mondes sur 27. L'exposant 0,65
freine bien l'empire moyen (37-42 %) mais pas le premier. Le reste W1-C
(« renchérir les tiers 4-5 en vague séparée ») reste ouvert et **la mesure à 250 ans
le confirme plus durement que l'extrapolation à 200 ans**.

### 2.6 INFLUENCE / FOI (W1-E : Conseil variante B, coûts ×é, foi au grain province, courant IA an 40) — **TENU sauf Divin**

| mesure | attendu | lu | verdict |
|---|---|---|---|
| influence médiane (essai) | 3 134 → 390 (s7 an 120) | méd. **250** sur 13 sims (min 51,8, max 20 855) | **TENU** |
| saturation de l'arbre avant l'an 40 | disparue | adoptions cumulées croissent jusqu'à l'an 200 partout | **TENU** |
| courant Aristocratie | ne doit plus rafler 77 % des courants | **19/103 courants (18 %)**, Populaire 57 (55 %) | **TENU** |
| foi d'État écrite sur toutes les provinces | plafond Divin levé | **porte OUVERTE : 33 % (t) / 35 % (e) des empires ont une religion d'État** | **TENU** |
| **Divin adopté** | reste 0 (reste connu) | **0 sur 1 209 doctrines actives** | **RATÉ (attendu)** |

La porte est ouverte dans presque tous les mondes (`essai_s11:134` **9/17 empires**,
`temoin_s3333:118` 8/12, `temoin_s90:118` **12/20**) avec des assiettes de fidèles
massives (`temoin_s2026:118` `fidèles Σ 48 878 · max 46 818`). **Le blocage n'est
plus la foi, c'est la compétition de slots** : Divin est un courant, et Populaire
gagne désormais le courant 55 fois sur 103.

*Note de télémétrie* : `scps/chronicle.c:1804` imprime « — Divin ADOPTABLE » en dur,
même quand `nfoi = 0`. `temoin_s7:118` dit donc `0/9 empire(s) ont une religion
d'État — Divin ADOPTABLE`, ce qui est faux. Étiquette à conditionner.

### 2.7 BATAILLE (W2-3 : `BT_DECROCHE` 0,35 → 0,26) — **TENU**

| mesure | cible | lu (13 paires) | verdict |
|---|---|---|---|
| **% décrochages** | 15-25 % | **méd. 19,7 % (t) · 18,5 % (e)** ; étendue **15,7 % → 26,0 %** | **TENU** |
| % déroutes | 75-85 % | 74-84 % | **TENU** |
| nuls | 0 | **0 sur 27** | TENU |
| durée moyenne | ~16-19 j | **15 à 18 j** | TENU |
| régions réduites | 41/37 (s7/s512 an 120) | **130 à 636** à 250 ans | à re-ancrer |
| guerres tranchées | — | 114 à 380/sim | — |

Le seul journal hors bande haute est `temoin_s7:958` (222/855 = **26,0 %**) et le
seul hors bande basse `temoin_s3333:772` (96/610 = 15,7 %). Sur 27 journaux la
constante `déroutes + décrochages = batailles` (aucun nul) est vérifiée partout.
**Le calibrage 0,26 tient à 250 ans, sur 13 mondes, dans les deux bras.**

Reste laid : le ratio choc/poursuite explose parfois — `essai_s2026:783`
`morts choc 2 800 vs POURSUITE 72 000 (**ratio 25,7×**)`, `essai_s7:1043` 6,2×,
`temoin_s4243:756` 6,6×, `temoin_s777:902` 7,5× — pour une cible implicite de 2-5×.

### 2.8 DÉCISIONS (W2-6 : membrane, Marbrive, dilemmes, directeur) — **TENU, et Marbrive n'est PAS mort**

| mesure | attendu (W2-6) | lu | verdict |
|---|---|---|---|
| **Marbrive** | « **structurellement MORT**, 0 sur 3,3 M région-jours » | **63 occurrences sur 27 sims (2,3/sim)**, non nul dans **20 journaux sur 27** | **le constat W2-6 est réfuté à 250 ans** |
| dilemmes W1 | vivants | **15 à 25 par sim** | TENU |
| latch tech / culturels / religieux | vivants | 17-26 / 4-7 / 7-15 | TENU |
| Pont(s) effondré(s) | — | **0 sur 27** | mort |
| directeur : acharnement | doit être 0 | **0 sur 27** | TENU |
| directeur : top ≤ 30 % | oui | **14-16 % partout** | TENU |

`temoin_s11:769` `membrane de décision : **5 Marbrive**` · `temoin_s2` 5 ·
`temoin_s1:—` 4 · `essai_s2` 4 · `essai_s3` 4 · `essai_s11` 4. Sept journaux sur 27
restent à 0 (dont les deux s3333). **Le diagnostic « les trois conditions sont
anti-corrélées » a été posé à 60 ans sur une seule graine : à 250 ans, sur 27
mondes, Marbrive se déclenche 63 fois.** La proposition W2-6 d'assouplir le
trigger doit être re-jugée sur cette mesure, pas sur celle de 60 ans.

Le « Pont effondré » et les 15 autres portes joueur restent, elles, à zéro dans
27/27 — la famine de contenu diagnostiquée par W2-6 est confirmée pour tout le
reste de la table.

---

## 3. ANOMALIES

Aucun run n'échoue par invariant ; les runs à `EXIT ≠ 0` sont **8 troncs `rc=127`**
(coupure du joueur, §0), pas des échecs moteur. Classées par sévérité.

### A1 — Le recoupement I0 ne se recoupe jamais (27/27), et toujours dans le même sens · **CRITIQUE**
`temoin_s7:993` `Σ postes +4082.5 · flux mesuré +4.7 · hors registre **−4077.8** or/mois/empire`.
Médiane du corpus : **−1 519 (témoin) / −1 891 (essai)**. Étendue −299,6 à −4 077,8.
*Hypothèse* : une sortie permanente proportionnelle à l'activité (achat d'État,
matière maison, semis IA, colonisation) hors des buckets FX_*, ou un double compte
sur `taxes`. Le signe constant exclut le bruit.
*Sévérité* : la télémétrie monétaire ne peut pas servir de gate tant que l'écart
vaut 20-40 % de la ligne `taxes`.

### A2 — `armée / limite de force` jusqu'à 432 % · **HAUTE**
`temoin_s3:1017` max **432 %** · `essai_s11:846` 331 % · `essai_s3333:780` 325 % ·
`essai_s4243:769` 313 % · `temoin_s11:832` 268 % · `essai_s777:919` 265 %.
Médiane des maxima : 218 % (t) / 253 % (e). W1-F annonçait « 2,02× → 1,03× ».
*Hypothèse* : le frein économique borne la CROISSANCE de la levée, pas le stock ;
un empire qui perd des régions garde son armée et voit sa limite fondre.
Exemple lisible : `essai_s3333:647` `Ligue Belilor 38 rég · armée 21 (**106 rgt /
limite 33**)`, solde/revenu 8 % — l'armée est payée, la limite est simplement dépassée.

### A3 — Empires riches, armés en stock, à **0 régiment** · **HAUTE**
- `essai_s11:698-699` : `Ligue Dhûrganyn 64 rég · pop 378 k · **or 184 577** ·
  armée 33 (**0 rgt** / limite 51) · solde/revenu 0 %` — et son stock contient
  `Armes lourdes 127 329 · Armes de trait 108 922`.
- `essai_s3333:650-651` : `Ordre Brenredel 36 rég · pop 183 k · or 59 502 ·
  armée 23 (**0 rgt** / limite 31)`, stock `Armes de trait 126 467 · Armes lourdes 119 324`.
- `essai_s4243:651` : `Ordre Belilyn 24 rég · pop 57 k · or 8 905 · **0 rgt** / limite 23`.
- `temoin_s90:790` : `Clans Bramwickka 73 rég · pop 199 k · or 40 014 · **5 rgt** / limite 57`.
*Hypothèse* : ce n'est **ni** l'or **ni** les armes (W2-3 avait conclu « soit sans
arme, soit sans pop, soit sans or » : les trois sont abondants ici). Reste la
main-d'œuvre (`ARMY_POOL_FRAC` 0,20 sur `prov[]`) ou un blocage de `wh_levy_batch`
sur un gate de tech d'unité. **À instrumenter en priorité** : c'est le premier
empire du monde qui se retrouve désarmé.

### A4 — Le prix du grain reste à zéro dans 5 mondes sur 27 · **HAUTE**
`temoin_s3:102` `médiane 0.000 · moyenne 0.123` · `temoin_s777:102` `0.000 / 0.109` ·
`essai_s1:101` `0.000 / 0.381` · `essai_s3:101` `0.000 / 0.378` ·
`essai_s4243:101` `0.000 / 1.180`. Médiane du corpus 0,23 pour une base 1,00.
*Hypothèse* : `PL_SINK_MONTHS=3` a relevé le plancher là où le trésor national est
gros ; là où il est bas (`temoin_s2026:769` trésor moyen **1 452 or**), le plancher
`BASE_PRICE×0,15×pl` retombe à zéro. Le correctif W2-1 est **conditionnel à la
richesse de l'État**, ce qui est exactement le couplage que le rapport W2-1 avait
identifié comme « propriété du modèle ».

### A5 — `solde / revenu fiscal` sans borne supérieure · **HAUTE**
`temoin_s60:972` **19 066 %** · `temoin_s2:981` 4 836 % · `temoin_s3:1018` 3 981 % ·
`essai_s512:831` 3 656 % · `temoin_s1:900` 1 869 % · `essai_s7:1045` 1 412 %.
Les pays concernés sont des micro-États (`temoin_s1:757` `Agraire libre 2 rég ·
pop 25 k · or 1 116 · solde/revenu **1 869 %**`). `WH_PAY_REVENUE_FRAC` ne les
touche pas parce que leur revenu fiscal est ~0 : le ratio explose par le
dénominateur. **La métrique est inexploitable comme gate tant qu'elle n'est pas
gardée par un plancher de revenu.**

### A6 — Incohérence de télémétrie sur les hubs (plancher de volume à moitié posé) · **MOYENNE**
`temoin_s2026:694` : `hubs : **marché atone** (V=424 < 500, % non significatif)`
puis `temoin_s2026:776` : `hubs des cités-états ........ **100 %** du commerce
mondial passe par leurs Centres`. Le plancher A16 est posé sur la ligne par-sim
(`scps/chronicle.c:2234`) mais **pas** sur l'agrégat de la SYNTHÈSE
(`scps/chronicle.c:3045`). Même motif dans `essai_s4243:672` (atone) → `:754` (26 %).

### A7 — `LEDGERS âmes/strates PAR PROVINCE : écart max 130 941 %` · **MOYENNE**
`temoin_s3:814` **130 941 %** · `essai_s11:688` 85 814 % · `temoin_s2:792` 62 162 % ·
`temoin_s1:729` 52 028 % · `temoin_s7:879` 51 459 % · `essai_s777:770` 49 123 %.
La médiane par province est excellente (0,1 %) et l'invariant global est parfait :
c'est **une province par monde** où les âmes valent 500 à 1 300 fois les strates.
*Hypothèse* : les colons de `demography_on_conquest` (`g.count = total/5 + 50`,
reste W2-2) déposés sur une tuile à strates quasi nulles.

### A8 — Provinces hypertrophiées jusqu'à 8 % du monde sur une tuile · **MOYENNE**
`temoin_s4243:415` `PROV 484 … pop=**59 317**` pour une population mondiale de
733 k · `temoin_s4243:343` `PROV 364 pop=46 750` · `:256` `PROV 224 pop=35 474` ·
`temoin_s3333:624` `PROV 553 pop=30 832` · `essai_s1:138` `PROV 1 pop=26 045`.
Le phénomène A9 du sweep précédent persiste, dans les deux bras.

### A9 — `brassage : 0 flux de pacte migratoire` sur 250 ans, dans 9 journaux · **MOYENNE**
`temoin_s512:740` · `temoin_s2026:743` · `essai_s2026:736` · `temoin_s3333:725` ·
`essai_s3333:732` · `essai_s1009:657` · `essai_s1:869` · `temoin_s2:932` ·
`essai_s60:946` (+ `temoin_s60:923` avec **1** flux, 33 âmes).
Ailleurs 105 à 644 flux. Un canal qui tombe à zéro absolu sur un quart du corpus.
Aggravation par rapport au sweep précédent (2 journaux sur 20).

### A10 — Micro-États à structure de classe inversée (J 0-1 %, B 90-99 %) · **BASSE, récurrente**
`temoin_s1009:592` `Ligue Elendoryn … J 0.0k (1 %) · B 1.3k (**98 %**)` ·
`essai_s1009:605` `J 0 % · B **99 %**` · `temoin_s11:723` `J 0 % · B 94 % · É 6 %` ·
`temoin_s90:823` `J 0 % · B 99 %` · `temoin_s90:811` `J 0 % · B 89 % · É 11 %` ·
`temoin_s1:754` `J 0 % · B 77 % · É 23 %` · `temoin_s2026:664` `J 29 % · B 5 % ·
**É 66 %**` · `temoin_s4243:651` `J 47 % · B 1 % · **É 53 %**`.
A7 du sweep précédent, inchangé.

### A11 — Hameaux à trésor absurde · **BASSE**
`essai_s90:846` : `Agraire libre 1 rég · pop 3 k · **Stab 0 Prosp 0** · or **29 855**
(+**2 487,9**/mois)` sans route ni export. `temoin_s90:820` `Mécaniste libre 1 rég ·
pop 2 k · or 4 027 (−139,4/mois)`. `essai_s4243:666` `Mécaniste libre 1 rég ·
or 7 024 · **21 rgt / limite 7**`.

### A12 — Le juge côtier s'effondre (77 % → 30 % pondéré) · **MOYENNE, régression**
`essai_s2:116` `côtiers→Colonisation **4/31 (13 %)**` · `essai_s1009:116` 3/17 (18 %) ·
`essai_s3:115` 5/25 (20 %) · `essai_s7:116` 4/19 (21 %) · `essai_s1:116` 4/19 (21 %).
Cause lue : Colonisation n'est plus adoptée (53 sur 1 209, 4,4 %, contre 14,2 %
avant) — le budget resserré et le courant différé ont réalloué ces slots vers
Offense/Défense/Technologie. **C'est le prix payé par W1-E** : le score
« côtier » est bon, il n'a simplement plus les moyens de se payer un slot.

### A13 — Dette structurelle jusqu'à 269× le revenu · **BASSE**
`temoin_s2:107` `max **269,01×**` · `essai_s11:106` 115,17× · `temoin_s7:107` 57,31× ·
`temoin_s3:107` 51,81× · `temoin_s3333:107` 47,31×. A15 du sweep précédent, aggravé.

### A14 — Systèmes entièrement morts dans 27/27 · **BASSE (design ou dette)**
- **Marine** : `0 coque(s) bâtie(s) · **0 fournitures navales consommées** · 0 raid ·
  0 bataille navale · 0 prise · 0 interception · 0 paquet noyé` — et la ligne se
  dénonce (`« NE doit plus être zéro »`). Pendant ce temps **25 à 36 Scieries navales
  sont bâties par sim** (`temoin_s7:924` `34×Scierie navale`).
- **Hameaux WILD** : `0 ralliés culturellement` dans 27/27.
- **Fins §27** : RÉCHAUFFEMENT / ASCENSION / SANG / « aucune » jamais tirées.
- **Guerres anti-piraterie** : 0 ; **guerres économiques** : 0 à 2 par sim.
- **Esclavage** : `temoin_s512:742` `**0 âme(s) servile(s) dans le monde**` ;
  ailleurs 98 à 8 858, soit ≤ 0,9 % de la population.
- **Nuls de bataille** : 0 sur 27.

### A15 — Étiquette « Divin ADOPTABLE » imprimée même quand aucune foi d'État n'existe · **COSMÉTIQUE**
`temoin_s7:118` `0/9 empire(s) ont une religion d'État — **Divin ADOPTABLE**`.
Chaîne en dur, `scps/chronicle.c:1804`.

### A16 — `L'Année Sans Été` absente de la ventilation du directeur · **COSMÉTIQUE**
`temoin_s1:925` et `essai_s1:943` listent les événements sans `L'Année Sans Été`,
présente dans les 25 autres journaux. À vérifier : tirage manqué ou ligne tronquée.

---

## 4. RÉCURRENCES ET SYSTÈMES OP / MORTS

### 4.1 Ce qui revient sur ≥ 5 graines

| motif | occurrences | portée |
|---|---:|---|
| marine / course / interception entièrement à zéro | **27 / 27** | universel |
| `0 ralliés culturellement` (hameaux WILD) | 27 / 27 | universel |
| `hors registre` négatif ≥ 300 or/mois/empire | 27 / 27 | universel |
| `armée/limite` max > 150 % | 27 / 27 | universel |
| prix du grain médiane < 0,30 | **18 / 27** | dominant |
| `solde/revenu` max > 100 % | 24 / 27 | dominant |
| Marbrive ≥ 1 | **20 / 27** | dominant |
| cour/admin/encadrement à `+0,0` exact | 16 / 27 | dominant |
| FIGÉES ≥ 35 % | 12 / 27 | moitié |
| micro-État à classe inversée (B > 70 %) | ≥ 11 journaux | fréquent |
| `brassage : 0 flux` | 9 / 27 | fréquent |
| arbre RECHERCHÉ max = 100 % | 23 / 27 | dominant |

### 4.2 « OP » — ce qui domine

- **Production** reste la doctrine n° 1 (213 / 1 209, 17,6 %) mais ne domine plus :
  le trio **Production + Offense + Défense = 43 %** (contre
  Production+Colonisation+Aristocratie = 46 % avant).
- **Le premier empire finit l'arbre technologique** (100 % dans 23/27) tout en
  restant à 60-80 régions : rien ne freine le leader.
- **Le pillage** rend 87-100 % de sa cible dans 27/27 (`temoin_s4243:769` 99 %),
  jusqu'à **2,58 M or-équiv./sim** (`temoin_s60:983`).
- **Populaire** a pris la place d'Aristocratie sur les courants (55 % contre 18 %).

### 4.3 Morts ou quasi morts

| chose | occurrences | commentaire |
|---|---:|---|
| doctrine **Divin** | **0 / 1 209** | la porte est ouverte (33-35 % d'empires à foi d'État), le slot ne l'est pas |
| doctrine **Faustien** | **1 / 1 209** (`essai_s11:113`) | sort du code mort, reste anecdotique |
| doctrine **Diplomatie** | 2 / 1 209 | `essai_s777:113`, `essai_s4243:113` |
| doctrine **Peuple** | 3 / 1 209 | `essai_s3:113` (2), `essai_s7:113` (1) |
| doctrine **Aristocratie** | 19 / 1 209 | passée de 13,0 % à 1,6 % |
| unités navales | 0 levée, 0 combat | 27/27 |
| fins RÉCHAUFFEMENT / ASCENSION / SANG | 0 | 27/27 |
| événement « Pont effondré » | 0 | 27/27 |
| classe servile | ≤ 0,9 % de la pop, **0 dans un monde** | `temoin_s512:742` |
| guerre anti-piraterie | 0 | 27/27 |
| bataille « nulle » | 0 | 27/27 |
| `concordat` de suzeraineté | 0 dans 12 journaux | inégal |

---

## 5. PROPOSITIONS, PAR IMPACT

### P1 — Fermer (ou nommer) le trou du recoupement I0 · **impact fort, risque nul**
*Constat* : 27/27 journaux, écart toujours négatif, médiane −1 519/−1 891
or/mois/empire, pire −4 077,8 (`temoin_s7:993`).
*Geste* : instrumenter `econ_flux_*` par un bucket **`FX_AUTRES`** calculé par
différence (`flux mesuré − Σ postes`) et l'imprimer comme un poste, PUIS le
ventiler poste par poste jusqu'à ce qu'il tombe sous 10 % de `taxes`. Candidats
lus dans les Restes W2-1/W2-4 : achat d'État, matière maison de chantier, semis
§NF payé, colonisation, tribut mûri, saisie, dons, butin.
*Site* : `scps/chronicle.c:2884-2913` (impression) + les sites d'écriture du trésor
dans `scps_econ.c`. *Risque* : nul (print-only pour la première étape).
*Mesure d'acceptation* : `|hors registre| < 0,10 × taxes` sur 6 graines.

### P2 — Donner un plancher de prix au grain qui ne dépende pas du trésor · **impact fort, risque moyen**
*Constat* : médiane 0,23 pour une base 1,00 ; **0,000 exact dans 5 sims** ;
corrélation lisible avec le trésor moyen (`temoin_s2026` trésor 1 452 → grain 0,932 ;
`essai_s4243` trésor 13 018 → grain 0,000 — donc la corrélation n'est pas simple, il
faut la mesurer).
*Geste* : découpler le plancher du panier vital de `price_level` — un plancher
absolu `BASE_PRICE × PL_FLOOR_ABS` (nouvelle clé, défaut 0,25) appliqué **après**
le clamp indexé, uniquement sur les biens du panier (grain, étoffe).
*Risque* : c'est un plancher, donc à la limite de la doctrine « pas de cap » —
mais un plancher de PRIX n'est pas un plafond de quantité ; à trancher par le joueur.
*Kill-switch* : `PL_FLOOR_ABS=0` redonne l'existant.
*Mesure* : grain médiane ≥ 0,60 sur 6 graines, `marché : grain` du bloc par âge inchangé.

### P3 — Instrumenter le « 0 régiment » du premier empire · **impact fort, risque nul**
*Constat* : A3 — quatre empires majeurs à 0-5 rgt avec or, armes en stock et
population (`essai_s11:698`, `essai_s3333:650`, `essai_s4243:651`, `temoin_s90:790`).
*Geste* : ajouter à la ligne `armée N (M rgt / limite L)` la **raison du refus de
levée** (pool de main-d'œuvre / armes prises / solde / gate d'unité), calculée par
`wh_levy_batch` et exposée par `warhost_braking_stats`. Print-only.
*Puis* : sonder `ARMY_POOL_FRAC` 0,20 → 0,30 seulement si la raison lue est le pool.
*Mesure* : plus aucun empire ≥ 20 régions à 0 rgt avec > 10 000 or et > 10 000 armes.

### P4 — Rendre le budget de doctrine sensible à la TAILLE, pas seulement au temps · **impact moyen, risque moyen**
*Constat* : le juge côtier tombe à 30 % parce que Colonisation ne trouve plus de
slot (4,4 % contre 14,2 %) pendant qu'Offense+Défense en prennent 25,6 %. Les
grands empires côtiers n'ont pas plus de slots que les hameaux.
*Geste* : `AI_DOCT_ACTS_MAX` (proposition F1 du sweep précédent, non appliquée)
**bornée par le budget**, pour que les grands puissent payer plusieurs actes par
an ; les petits restent à 1 par le prix.
*Risque* : re-sature l'arbre — à ne poser qu'avec la mesure d'influence médiane
(qui est aujourd'hui à 250, saine).
*Mesure* : juge côte ≥ 55 % pondéré **sans** que la médiane d'influence remonte
au-dessus de 1 000.

### P5 — Renchérir les tiers 4-5 de l'arbre technologique · **impact moyen, risque faible**
*Constat* : arbre RECHERCHÉ **max = 100 % dans 23/27**, moyenne 37-42 %. C'est
exactement le reste W1-C (« garder 0,65 et traiter le reste par P3 : `BASE_COST`
t4/t5 260→340, 400→560 »), et la mesure à 250 ans est pire que l'extrapolation.
*Risque* : `édifices refusés faute de palier` est déjà à 362/sim (médiane) et monte
à 748 (`essai_s1009:713`) — surveiller cette ligne dans la même sonde.
*Mesure* : arbre max ≤ 85 % à l'an 250, édifices refusés ≤ 400.

### P6 — Poser le plancher de volume des hubs sur la SYNTHÈSE aussi · **impact faible, risque nul**
*Constat* : A6. `scps/chronicle.c:3045` n'a pas la garde de `:2234`.
*Geste* : 3 lignes.

### P7 — Garder `solde / revenu fiscal` par un plancher de revenu · **impact faible, risque nul**
*Constat* : A5, max 19 066 % produit par un dénominateur ~0.
*Geste* : n'agréger dans la médiane/max que les pays dont le revenu fiscal annuel
dépasse un seuil (ex. 100 or/an), et imprimer à côté le nombre d'exclus. Print-only.

### P8 — Re-juger Marbrive sur la mesure à 250 ans, pas sur celle à 60 ans · **décision joueur**
*Constat* : 63 déclenchements sur 27 sims, non nul dans 20 journaux. Le trigger
n'est pas mort ; il est **lent**. Assouplir maintenant (OU sur `K_inst`, seuil 0,3)
le rendrait probablement fréquent. **Recommandation : ne rien toucher**, et fermer
le reste W2-6 par cette mesure.

### P9 — Trancher les systèmes morts · **décision joueur**
Marine (0 coque, 0 fourniture, 25-36 scieries navales bâties pour rien),
ralliement culturel des hameaux (0/27), fins RÉCHAUFFEMENT/ASCENSION/SANG (0/27),
classe servile (≤ 0,9 %, 0 dans un monde). Soit on les branche, soit on retire les
lignes de télémétrie qui les réclament — aujourd'hui le chronicle s'accuse
lui-même (`« NE doit plus être zéro »`) 27 fois sur 27.

---

## 6. LIMITES

1. **13 paires sur 50 (26 %).** Le sweep a été coupé à 5 h 15. Les graines
   arrivées ne sont pas un tirage aléatoire des 50 : ce sont les 13 premières
   à finir, donc **biaisées vers les mondes rapides** (moins de provinces, moins
   de pays, moins de batailles). Les graines lentes — potentiellement les plus
   grandes et les plus peuplées — manquent toutes.
2. **1 sim par cellule** (`reps_per_seed=1`). Il n'y a **aucune répétition** : tout
   écart témoin/essai d'une graine est un point unique, sans mesure de bruit
   intra-graine. Les médianes sur 13 paires sont robustes ; **les Δ par graine ne
   le sont pas** et ne doivent pas servir de gate.
3. **Horizon 250 ans, mono-horizon.** Toutes les cibles héritées (W1-B, W1-C, W2-1,
   W2-3) ont été calibrées à 120 ans. Le piège documenté par W2-3 (« toute cible
   chiffrée héritée d'avant le frein de levée doit être re-mesurée ») s'applique ici
   à l'envers : **la moitié des écarts de ce rapport peut être un effet d'horizon,
   pas un effet de vague.** Sans un point à 120 ans dans le MÊME binaire, on ne peut
   pas séparer les deux. C'est la limite la plus lourde.
4. **Ce que le corpus ne permet pas de conclure** :
   - qu'un correctif est « TENU » sur une mesure dont l'étendue couvre un ordre de
     grandeur (FIGÉES 5-61 %, manufactures 268-12 886, grain 0,000-2,985) ;
   - qu'un système est mort sur 13 graines (Divin, Faustien, Peuple, Diplomatie :
     1 209 adoptions, c'est assez pour dire « rare », pas pour dire « impossible ») ;
   - qu'une fin §27 est sur- ou sous-représentée : 1 fin par sim, 27 sims, trois
     modalités observées — le χ² n'a pas de puissance ;
   - qu'un Δ témoin/essai de moins de 15 % sur une médiane de 13 est un signal.
5. **Les 8 troncs `rc=127` n'ont pas été lus** (worldgen seul), conformément au
   brief. Le `resume.txt` du script n'existe pas : aucune table automatique n'a pu
   servir de point de départ ni de contre-vérification.

---

## 7. VERDICT EN 6 LIGNES

1. **Le corpus est de 13 paires, pas 17** ; 27 journaux complets, 8 troncs à
   `rc=127`, aucun échec moteur, invariant M3f max 201 % pour 370 %.
2. **Population (W1-B + W2-2) est le correctif le mieux tenu** : invariant Σ sièges
   = Σ âmes et 0 groupe hors invariant dans **27/27**, âmes/strates 97,8-100,4 %.
3. **Doctrines (W1-E) a fait ce qu'elle annonçait** : arbre joué jusqu'à l'an 250
   (96 adoptions cumulées médianes), influence médiane 250, Aristocratie 13 % → 1,6 %,
   Technologie ×16, Connaissances ×11,5, Faustien sort du code mort, **juge martial
   62 % → 86 %** — au prix du **juge côtier 77 % → 30 %**.
4. **Décrochage (W2-3) tient à 250 ans** : 18,5-19,7 % de décrochages, 0 nul, 27/27.
5. **Trois correctifs ne tiennent pas** : le recoupement monétaire (« hors registre »
   négatif de 300 à 4 078 or/mois/empire, 27/27), le frein de levée sur sa queue
   (`armée/limite` jusqu'à 432 %, empires majeurs à 0 régiment malgré or et armes),
   et le prix du grain (médiane 0,23 pour une base 1,00, **0,000 exact dans 5 sims**).
6. **Marbrive n'est pas mort** : 63 déclenchements sur 27 sims — le constat W2-6
   (« 0 sur 3,3 M région-jours ») était une mesure d'an 60 sur une graine.
