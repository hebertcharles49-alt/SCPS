# CALIBRAGE DE L'INFLUENCE POLITIQUE — 2026-09-03

Rapport d'analyse. **Aucune modification de code.** Toute proposition est
chiffrée, datée d'un site `fichier:ligne`, et laissée à la décision joueur.

---

## 0. Méthode et corpus

**Corpus lu** (intégralement, aucun filtre statistique) :

- `scps/scps_influence.{c,h}`, `scps/scps_doctrines.{c,h}`,
  `scps/scps_ai.c` (§ doctrines, l. 3106-3380), `scps/scps_sim.c`
  (l. 495-560, 1240-1290, 1620-1650), `scps/scps_statecraft.c`
  (l. 106-145, 245-335, 509-535), `scps/scps_religion.{c,h}`,
  `scps/scps_tune_list.h` (l. 1430-1490, 1870-1975).
- `docs/DESIGN_MISSIONS_DOCTRINES.md` §3-§4, `docs/SWEEP_DOCT_AI_2026-09-02.md`.
- `sweep_doct_ai_10x200/` : les 10 logs du bras ESSAI + `resume.txt` +
  `manifest.txt`.

**Trois mesures neuves** (binaire `chronicle.exe` du 03-09 10:53, postérieur à
tous les sources — donc **assiette SIÈGES corrigée**, contrairement au sweep du
02-09 qui s'est terminé à 18:54 UTC alors que `ddd1b1c` date du 03-09 09:47) :

| # | commande | rôle |
|---|---|---|
| M1 | `./chronicle.exe 7 1 120` | 2 empires, référence |
| M2 | `./chronicle.exe 7 1 120 6 12` | **monde du sweep**, clé neuve |
| M3 | `SCPS_TUNE=INFLUENCE_PER_NOBLE_ARISTO=0.002,INFLUENCE_PER_BOURGEOIS=0.0011,INFLUENCE_PER_LABORER=0.00011 ./chronicle.exe 7 1 120 6 12` | sonde Divin : les 3 boosts de classe annulés ⇒ Divin gagne dès **un** fidèle |

**⚠ Ce qui du sweep 02-09 reste valide et ce qui ne l'est plus.** Les juges, les
plafonds de score et la distribution *relative* des 13 orientations tiennent (le
score ne dépend pas de l'assiette). En revanche **tout le §3.5f et la décision
J2 du rapport précédent sont caducs** : sous la clé neuve le courant gagnant
bascule d'Aristocratie vers Bourgeoisie/Populaire (§4.3 ci-dessous), et le
plancher `é=0.25` ne mord plus pour presque personne (§2.3).

---

## 1. L'ASSIETTE MESURÉE — le nombre dont tout découle

### 1.1 La part des sièges, mesurée sur 10 mondes

Ligne « SIÈGES (édifices, pop_by_class) » des 10 logs ESSAI, an 200 :

| graine | 7 | 1009 | 4243 | 11 | 2026 | 512 | 3333 | 777 | 90 | 60 | **moy.** |
|---|---|---|---|---|---|---|---|---|---|---|---|
| élites | 15 | 14 | 16 | 15 | 14 | 19 | 17 | 17 | 16 | 15 | **15,8 %** |
| bourgeois | 7 | 12 | 8 | 14 | 7 | 7 | 6 | 8 | 10 | 13 | **9,2 %** |
| journaliers | 76 | 73 | 74 | 69 | 78 | 73 | 75 | 74 | 73 | 71 | **73,6 %** |

(M2, an 120, donne 11/7/80 : la part d'élites monte avec l'âge du monde.)

### 1.2 Le taux par habitant

`influence_base_gain`, `scps/scps_influence.c:128-149`, taux
`scps/scps_tune_list.h:1899,1930,1932` :

```
a_def/hab/mois = 0,158×0,002 + 0,092×0,0011 + 0,736×0,00011
               = 3,160e-4  + 1,012e-4     + 0,810e-4
               = 4,98e-4  /habitant/mois        (avant le Conseil)
```

Parts de gain : **élites 63,4 % · bourgeois 20,3 % · journaliers 16,3 %** — le
design vise 60/20/20 (`docs/DESIGN_MISSIONS_DOCTRINES.md:209-213`) : **conforme**,
les journaliers un peu sous la cible.

```
assiette/mois = pop × 4,98e-4
é             = assiette / INFLUENCE_BASE_REF (2,0)   = pop × 2,49e-4   [plancher 0,25]
gain/mois     = assiette × mult_conseil
```

**Le plancher `é=0,25` mord sous pop = 1 004 habitants.** (`scps_influence.c:155`)

### 1.3 Le multiplicateur du Conseil, mesuré

`influence_council_mult`, `scps/scps_influence.c:45-61` : moyenne des rangs
(I=1 · II=2 · III=3) des **sièges POURVUS seulement** ; plancher
`INFLUENCE_COUNCIL_FLOOR` = 1,0 si aucun.

- Tirage des rangs : 55 % I · 30 % II · 15 % III (`scps_statecraft.c:137-140`),
  3 candidats par siège (`scps_statecraft.h:53`).
- **L'IA ne pourvoit QU'UN siège** — celui de son éthos
  (`scps_statecraft.c:514`, `sc_ethos_seat`) — et y prend le meilleur rang
  abordable (`:528-534`). Espérance du meilleur de 3 : **2,22**.
- Mesuré : 16 à 26 sièges pourvus pour ~28 pays vivants (ligne « conseil (V2a) »
  des 10 logs ; moy. 18,6) ⇒ **≈ 0,66 siège/pays** ⇒ ~1/3 des pays sont au
  plancher 1,0, ~2/3 à ~2,2.

**Fourchette retenue : `mult_conseil` ∈ [1,0 ; 3,0], moyenne monde ≈ 1,7.**

### 1.4 FOURCHETTES du gain mensuel par taille

Tailles tirées des logs (`an 40/80/120/160` + blocs « empires vivants ») :

| Taille | pop | assiette | é | **gain/mois, Conseil vide (×1)** | **gain/mois, Conseil III plein (×3)** |
|---|---|---|---|---|---|
| Cité-état 1 prov. (pire) | 300 | 0,15 | 0,25 *(plancher)* | **0,15** | 0,45 |
| Cité-état 1 prov. (bon) | 3 000 | 1,49 | 0,75 | **1,49** | 4,48 |
| Pays de départ, an ~20 (pire/bon) | 2 000 / 6 000 | 1,00 / 2,99 | 0,50 / 1,49 | **1,00 / 2,99** | 2,99 / 8,97 |
| Pays moyen, an ~80 | 8 000 / 25 000 | 3,99 / 12,5 | 1,99 / 6,2 | **3,99 / 12,5** | 12,0 / 37,4 |
| Hégémon 30+ rég., an ~180 | 90 000 / 130 000 | 44,8 / 64,8 | 22,4 / 32,4 | **44,8 / 64,8** | 134 / 194 |

**Amplitude totale : ×1 300 entre la pire cité-état et le meilleur hégémon
à Conseil plein** (0,15 → 194 /mois).

Contrôle sur mesure réelle (M2, an 120, 6 empires) : Ligue Pyxexis, 84 k hab,
sièges 11/7/80 ⇒ a = 84 000 × 3,85e-4 = **32,3/mois** ; é = 16,2 ; stock 9 127.

**⚠ L'échelle de référence du design est périmée.** `DESIGN_MISSIONS_DOCTRINES.md:224-227`
dit « empire mûr (~13 000 hab) ≈ 5,7/mois (é≈2,8) ». Le vrai empire mûr du
moteur fait **90-130 k habitants** : é ≈ **22 à 32**, soit **10× la référence
écrite**. Tout raisonnement de design fait sur « é≈2,8 » est faux d'un ordre de
grandeur.

### 1.5 Par COURANT (l'assiette, hors Conseil), sur la moyenne mesurée 15,8/9,2/73,6

Le courant **ajoute** un terme, il ne remplace rien (`scps_influence.c:135-147`) :

| Courant | assiette/hab/mois | gain vs défaut | terme ajouté |
|---|---|---|---|
| aucun (défaut) | 4,98e-4 | — | — |
| **Aristocratie** | 5,77e-4 | **+15,9 %** | élites × 0,0005 |
| **Bourgeoisie** | 5,99e-4 | **+20,3 %** | bourgeois × 0,0011 |
| **Populaire** | 5,79e-4 | **+16,3 %** | journaliers × 0,00011 |
| **Divin**, f = 100 % de croyants | 6,65e-4 | **+33,5 %** | croyants × 1/6000 |
| Divin, f = 50 % | 5,81e-4 | +16,7 % | idem |

**Les trois courants de classe sont à ±4 points l'un de l'autre.** Sur les
extrêmes mesurés le classement bascule sans qu'aucun choix politique ne soit en
jeu :

| monde | sièges é/b/l | vainqueur | marge sur le 2ᵉ |
|---|---|---|---|
| M2 an 120 | 11/7/80 | **Populaire** (+22,9 %) | +2,9 pt sur Bourgeoisie |
| s11 | 15/14/69 | **Bourgeoisie** (+29,1 %) | +14,8 pt sur Populaire |
| s512 | 19/7/73 | **Aristocratie** (+17,7 %) | +2,8 pt sur Populaire |

**Seuil Divin :** Divin dépasse le meilleur courant de classe dès
**f > 61 %** de la population professant la foi d'État (monde moyen) ; f > 92 %
dans un monde bourgeois (s11).

---

## 2. TEMPS D'ACQUISITION

### 2.1 Les coûts (`scps/scps_doctrines.c:301-313`, `scps_tune_list.h:1916-1919`)

```
adoption n° k (k=0..5) = (50 + 25k) × é      Σ = 675 × é
idée     n° n (n=0..35)= (30 +  3n) × é      Σ = 2 970 × é
ARBRE COMPLET                                Σ = 3 645 × é
1re adoption seule                           =    50 × é
1re doctrine COMPLÈTE (adoption + 6 idées)   =   275 × é
```

### 2.2 Le temps est INVARIANT D'ÉCHELLE — c'est le but, et c'est atteint

Avec `assiette = 2é` (par construction de `INFLUENCE_BASE_REF = 2,0`) :

```
t = (C × é) / (2é × mult) = C / (2·mult)  MOIS      ← é s'annule
```

| jalon | Conseil ×1 | ×1,7 (monde) | ×2 | ×3 |
|---|---|---|---|---|
| 1re adoption | 25 mois = **2,1 an** | 1,2 an | 1,0 an | **0,7 an** |
| 1re doctrine complète (6 idées) | 137,5 mois = **11,5 an** | 6,7 an | 5,7 an | **3,8 an** |
| **ARBRE SATURÉ (6 slots × 6 idées)** | 1 822 mois = **152 an** | **89 an** | 76 an | **51 an** |

**Vérification empirique (M2, 6 empires, clé neuve) :**

```
an 24 : 25 doctrines · 95 idées      an 72 : 36 doctrines · 206 idées
an 48 : 30 doctrines · 162 idées     an 96 : 36 doctrines · 216 idées  ← SATURÉ
```

36/36 slots et 216/216 idées à **l'an 96**. Le modèle prédit 89 ans à mult 1,7 :
**écart 8 %**. Le modèle est bon.

Idem M1 (2 empires) : 12 doctrines · 72 idées à l'an 120 = **arbre du monde
entier épuisé**.

### 2.3 Où le plancher é = 0,25 mord encore, où é explose

**Il ne mord presque plus.** Sous l'ancienne clé (strates, ~2 % d'élites)
l'assiette valait ~1/10 ⇒ **tout le monde** était planché ; c'est ce qui rend
la cadence du sweep 02-09 non transposable. Sous la clé neuve, seuil = **pop
1 004**. Pour un pays planché, le temps redevient dépendant de la taille :

| pop | assiette | é | 1re adoption (×1) | arbre complet (×1) |
|---|---|---|---|---|
| 1 004 | 0,50 | 0,25 | 25 mois | 152 an |
| 500 | 0,249 | 0,25 | **50 mois** | **305 an** |
| 300 | 0,149 | 0,25 | **84 mois** | **508 an** |

Le plancher pénalise donc bien les nains, dans le bon sens. **La proposition F5
du rapport 02-09 (plancher 0,25 → 0,50) est LARGEMENT CADUQUE** : elle visait un
symptôme de l'ancienne assiette.

**La queue parasite subsiste malgré tout, pour une AUTRE raison.** M2 :
`Ligue Karggoris, 4 rég, pop 0,4 k, influence 1 644,8 : Production(6) Colonisation(6) Commerce(6)`
— 400 âmes, **trois doctrines complètes**. Ce pays n'a pas acheté à 400 âmes :
il a acheté **quand il était grand**, et rien n'est jamais perdu (aucun
entretien, v107 ; aucun remboursement ; l'IA n'appelle jamais
`doctrines_abandon`). C'est un **effet d'héritage de taille**, pas un effet de
plancher. Décision joueur ferme (« flat, sans entretien ») ⇒ **non re-litigé
ici**, seulement signalé.

**é explose sans borne.** Hégémon an 180 : é = 22-32. La 6ᵉ adoption y coûte
175 × 30 = **5 250** et la 36ᵉ idée 135 × 30 = **4 050** — chiffres qui
paraîtront absurdes à l'UI, mais qui sont exacts et neutres (le revenu est
60-190/mois). `ech_ok` clampe à 1 000 (`scps_doctrines.c:298`) : jamais atteint
(il faudrait pop = 4 M).

### 2.4 Le vrai frein de l'IA n'est pas le prix, c'est la cadence

`ai_doctrines_year` fait **UN acte par passage annuel**
(`scps_ai.c:3315-3369`, `return 1` après chaque acte ; cadence
`AI_DOCT_CHECK_MONTHS = 12`, `scps_tune_list.h:1970`).

```
6 adoptions + 36 idées = 42 actes  ⇒  42 ANS minimum, quel que soit le trésor
```

À mult 1,7 le budget demande 89 ans, la cadence 42 : **le budget est le frein
jusqu'à ~l'an 90, la cadence prend le relais au-delà.** Les deux se rejoignent :
saturation an 96 mesurée.

---

## 3. LES COÛTS PLATS FACE AU GAIN

`INFLUENCE_COST_ENVOY = 12` · `INFLUENCE_COST_FAB = 25`
(`scps_tune_list.h:1902-1903`, débités `scps_sim.c:541,543`) ·
`DESSEIN_PIVOT_INFLUENCE = 20` (`scps_tune_list.h:1448`, débité
`scps_missions.c:95`). **Aucun des trois ne passe par é.**

### 3.1 Combien de mois de revenu politique coûte un émissaire ?

`mois = C / (pop × 4,98e-4 × mult)`

| pays | pop | mult | gain/mois | **émissaire (12)** | **fabrication (25)** | **pivot (20)** | émissaires/mois payables |
|---|---|---|---|---|---|---|---|
| cité-état | 1 000 | 1 | 0,50 | **24,1 mois** | 50,2 mois | 40,2 mois | 0,04 |
| départ | 4 000 | 1 | 1,99 | **6,0 mois** | 12,5 mois | 10,0 mois | 0,17 |
| moyen | 20 000 | 2 | 19,9 | **0,60 mois** | 1,26 mois | 1,00 mois | 1,7 |
| **hégémon** | 120 000 | 2 | 119,6 | **0,10 mois** | 0,21 mois | 0,17 mois | **10,0** |
| hégémon | 120 000 | 3 | 179,4 | 0,07 mois | 0,14 mois | 0,11 mois | **15,0** |

**Écart pire/meilleur cas : ×241.** Un hégémon peut se payer **10 à 15
émissaires par mois** ; le plancher anti-spam `DIPLO_ENVOY_FLOOR_DAYS = 30`
(`scps_tune_list.h:1905`) n'en laisse passer **qu'un**. Le coût est donc
**totalement inerte au sommet** (10 % du revenu mensuel, 0,8 % du revenu annuel)
et **prohibitif en bas** (2 ans de revenu pour une cité-état).

Le design voulait exactement l'inverse : « le coût REMPLACE le cooldown — on
enchaîne si on a économisé, on est muet à sec »
(`DESIGN_MISSIONS_DOCTRINES.md:245`). **Aujourd'hui le cooldown est revenu par
la fenêtre pour les riches, et le coût seul mord pour les pauvres.**

### 3.2 Chiffrage d'un passage par é

Si `coût = C × é`, alors `mois = C×é / (2é×mult) = C/(2·mult)` — **invariant
d'échelle**, comme les doctrines :

| C | mois de revenu à ×1 | à ×2 | à ×3 | lecture |
|---|---|---|---|---|
| 12 (valeur actuelle) | 6,0 | 3,0 | 2,0 | trop cher pour un verbe courant |
| **6** | 3,0 | **1,5** | 1,0 | l'émissaire redevient un arbitrage |
| 3 | 1,5 | 0,75 | 0,5 | le plancher 30 j redevient le frein |

**Proposition chiffrée** (détail en §6-P2) :
`INFLUENCE_COST_ENVOY 12 → 6 × é` · `INFLUENCE_COST_FAB 25 → 12 × é` ·
`DESSEIN_PIVOT_INFLUENCE 20 → 10 × é`. Effet : l'hégémon paie 180 par émissaire
(1,5 mois de revenu à mult 2) au lieu de 12 (0,10 mois) — **facteur 15 de
rééquilibrage** ; la cité-état planchée (é = 0,25) paie 1,5 au lieu de 12, soit
6 mois au lieu de 24 — **facteur 4 de soulagement**.

### 3.3 Le puits face au robinet — le chiffre qui condamne

M2, 6 empires, an 120 :

```
influence GÉNÉRÉE Σ ..................... 108 035
stock non dépensé en fin de partie ......  30 989   (4 048+2 219+12 265+1 685+9 127+1 645)
dépensé (arbre) .........................  77 046   (71 %)
```

Mais **l'arbre est saturé dès l'an 96** : à partir de là, **100 % du robinet est
de la thésaurisation**. Mesuré sur les 24 dernières années :

```
médiane :  972 (an 96) →  3 134 (an 120)   = +90/an, sans emploi
maximum : 6 520 (an 96) → 12 265 (an 120)   = +239/an, sans emploi
```

Sur l'ancienne clé, an 200, même verdict et pire encore (les stocks explosent à
20 000 - 70 000 : s777 max 70 354, s512 max 38 109, s3333 max 37 094).

**Le seul puits qu'un joueur peut ouvrir aujourd'hui** : 12 émissaires/an
(plancher 30 j) × 12 = **144/an**, contre 1 435/an de revenu pour un hégémon à
mult 2. **Le puits maximal absorbe 10 % du robinet.** Pour l'IA il absorbe
**0 %** : `sim_cmd_drain` est gaté joueur (`scps_sim.c:511`).

---

## 4. RÉCURRENCES ET « OP »

### 4.1 OP nº1 — LE CONSEIL DÉCAPITÉ (le code contredit le design écrit)

`scps/scps_influence.c:50-60` :

```c
for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
    int slot = statecraft_council_seated(sc, cid, seat);
    if (slot < 0) continue;   /* siège vacant : ne compte pas dans la moyenne */
    ... sum += tier; n++;
}
return sum / (float)n;
```

Le design écrit dit l'inverse : « rang moyen des ministres en siège (I → ×1 …) ;
**siège vide compte 0 dans la moyenne** »
(`docs/DESIGN_MISSIONS_DOCTRINES.md:219-222`).

**Conséquence chiffrée**, avec les salaires
(`COUNCIL_TIER{1,2,3}_REVENUE_RATE` = 1,5 / 3,0 / 5,0 % du revenu fiscal annuel,
`scps_tune_list.h:1421-1423`, `scps_statecraft.c:106-118`) :

| stratégie du joueur | `mult_conseil` | masse salariale | verdict |
|---|---|---|---|
| 3 sièges pourvus (III, I, I) | (3+1+1)/3 = **1,67** | 5,0+1,5+1,5 = **8,0 %** | ce que l'UI encourage |
| **seul le III assis, 2 sièges vides** | 3/1 = **3,00** | **5,0 %** | **+80 % d'influence, −37 % de salaire** |
| Conseil VIDE | 1,00 (plancher) | 0 % | strictement égal à un Conseil tout-rang-I |

**Un ministre médiocre DILUE l'influence.** Le jeu optimal est de décapiter son
propre Conseil. Le seul prix payé est la perte de deux bonus de siège
(+12 % savoir / +15 % promo / +20 % manuf, `scps_statecraft.c:88-93`).

Espérance sur 3 candidats/siège (55/30/15) :
- meilleur d'un siège : **2,22** ;
- meilleur des 3 sièges (stratégie décapitée) : P(au moins un III) = 1 − 0,614³ = **76,9 %**,
  espérance **2,76** — soit **+24 % de revenu politique** pour 3 clics.

La note du header (`scps_influence.h:76-78`) verrouille bien l'exploit
symétrique (« accumuler à Conseil plein puis RENVOYER pour brader les prix » :
é ne lit jamais le Conseil — **correct, aucune faille de ce côté**). C'est
l'exploit **inverse**, sur le revenu, qui est ouvert.

**Bonus d'asymétrie :** l'IA ne pourvoit **qu'un seul siège**
(`scps_statecraft.c:514`) — elle bénéficie donc gratuitement de la règle
« moyenne des pourvus ». Un joueur qui joue « proprement » (3 sièges) est
**pénalisé** face à l'IA.

### 4.2 OP nº2 — LE ROBINET SANS PUITS

Voir §3.3. Après l'an ~96, l'influence est une monnaie **morte** : un
accumulateur sérialisé (section INFL, save v104) qui ne fait plus que croître.
La spec le savait — « les sinks (synergies fibonacciennes) font le travail »
(`DESIGN_MISSIONS_DOCTRINES.md:229-231`) — mais les synergies **ne sont pas
implémentées**, et leur barème écrit (0·2·3·5·8 /mois × é,
`DESIGN_MISSIONS_DOCTRINES.md:318-330`) est calibré sur la référence périmée
« ~4/mois de génération » (§1.4) : 4 synergies = 18×é/mois contre 2é×mult de
revenu, soit **9/mult mois de revenu par mois** — mathématiquement intenable.
Le barème doit être re-chiffré **en fraction du revenu**, pas en points.

### 4.3 Le courant : un tirage au sort déguisé, et un basculement de la clé

`aid_best_current`, `scps_ai.c:3123-3135` : compare les 4 assiettes,
`if (g[i] > g[best])` **strict** ⇒ **toute égalité va à l'id le plus petit**,
c'est-à-dire **`DOCT_ARISTOCRATIE = 13`** (`scps_doctrines.h:78`).

Score du courant = `1,0 + part` (`scps_ai.c:3312`). Les 4 assiettes étant à
±8 % l'une de l'autre, la part vaut ~0,27 ⇒ **score ≈ 1,27, quasi constant**.
Le courant est donc toujours un remplissage de 4ᵉ-6ᵉ slot, jamais un choix
disputé.

**Le basculement mesuré :**

| | Aristocratie | Bourgeoisie | Populaire | Divin |
|---|---|---|---|---|
| sweep 02-09 (clé ANCIENNE, 10 sims) | **43** | 1 | 12 | **0** |
| M2 (clé NEUVE, 6 empires) | 2 | 1 | **3** | **0** |

Cause exacte (ancienne clé, strates de genèse 80/15/5,
`chronicle` « classes (E0.7, départ 80/15/5) ») :
aristo = 0,05×0,0025 = 12,5e-5 · bourgeois = 0,15×0,0006 = 9,0e-5 ·
journaliers = 0,80×0,00012 = 9,6e-5 ⇒ **Aristocratie gagnait à la genèse**, et
l'exclusivité gelait le choix pour 200 ans.
Sous la clé neuve (sièges 15,8/9,2/73,6), c'est **Bourgeoisie** qui gagne le
monde moyen (§1.5) — mais avec 3 points de marge, donc **par graine**.

### 4.4 Les doctrines dominantes : c'est le SCORE, jamais le coût

**Le coût ne départage rien** : `doctrines_adopt_cost_f` ne dépend que de
`n_active`, `doctrines_idea_cost_f` que de `n_ideas`
(`scps_doctrines.c:301-307`) — **identique pour les 17 doctrines**. La
distribution est donc **100 % un produit de `ai_doct_scores`**.

Plafonds réels (`scps_ai.c:3237-3312`) et valeur observée à l'an 1-10, quand les
6 slots se prennent :

| doctrine | plafond | valeur an 1-10 | adoptions (sweep, /331) | cause |
|---|---|---|---|---|
| **Production** | 2,0 (`:3292`) | **2,0 dès l'an ~15** | **62** | `nbld×0,10` sature à 12 manufactures — le monde en pose **250/sim** (log s7 an 120) ; `rawcap/nprov×0,03` sature à 26,7. **Les deux plafonds sont atteints avant l'an 20 et ne bougent plus.** |
| **Colonisation** | **2,6** (`:3245`) | 1,0-2,6 | **47** | plus haut plafond permanent du catalogue (capitale côtière 1,0 + chantier 1,0 + vierges 0,6) |
| **Courants** | ~1,30 (`:3312`) | 1,27 | **56** (43+12+1) | toujours disponible, jamais nul |
| **Infrastructure** | 2,4 (`:3298`) | ~0,5 | **38** | monte avec la vétusté (signal tardif mais durable) |
| **Commerce** | 2,5 (`:3248`) | ~0 | **37** | nul à l'an 1 (0 route), fort après |
| Vassaux | 2,6 (`:3270`) | 0 | 24 | **0 sans vassal** — signal binaire |
| Offense / Défense | 3,7 / 4,1 (`:3258,3267`) | 0 | 23 / 22 | **transitoires** : la guerre arrive après la saturation |
| Mercantilisme | 2,4 (`:3255`) | ~1,4 | 12 | +0,6 si ≤1 route ⇒ fort à l'an 1, s'éteint ensuite |

**Les mortes, et pourquoi :**

| doctrine | adoptions | valeur mesurée | cause exacte |
|---|---|---|---|
| **Diplomatie** (`:3277`) | 4 | 0,3-0,9 | `nally×0,6` : le monde tient **1 à 4 pactes d'alliance au total** (logs) ; opinion moyenne ≈ 0 |
| **Technologie** (`:3303`) | 2 | 0-0,6 | `nlib×0,6` : bibliothèque/monastère quasi absents — **407 édifices refusés faute de tech de palier** (log s7) ; `savoir/pop` est un ratio ~0 |
| **Peuple** (`:3280`) | 2 | 0,1-0,3 | `diaspora/pop×3` : la diaspora fait 2-5 % ⇒ 0,06-0,15 ; pactes migratoires ≈ 0 |
| **Connaissances** (`:3288`) | 2 | 0,9-1,3 | plafonnée à 2,0 mais la métabolisation reste basse (« métabolisation MAX 2/6 ») |
| **Faustien** (`:3306-3308`) | **0** | 0 | double gate : `nfaust>0` **ET** `crisis < FAUST_BRECHE_CAUTION 0,55`. Les nœuds faustiens arrivent an 100+, après saturation. **Code mort.** |
| **Divin** | **0** | — | §5 |

**Verdict : Production est un plancher permanent à 2,0 que rien ne peut battre
sauf une guerre en cours.** C'est la cause racine unique de la monotonie.

### 4.5 Hygiène — les cités-états génèrent une monnaie qu'elles ne peuvent pas dépenser

`scps_sim.c:1270-1274` fait tourner `influence_tick` pour **tout pays vivant**
avec `ai_on`, cités-états comprises ; mais `ai_doctrines_year` les exclut
explicitement (`scps_ai.c:3323`) et `sim_cmd_drain` est gaté joueur. Elles
accumulent donc un stock que **rien** ne dépense, sérialisé pour rien, et qui
écrase la médiane du chronicle (s512 : « médiane 0,0 · max 38 109,6 » sur 23
pays vivants — la médiane ne parle de personne).

---

## 5. DIVIN — 0 adoption sur 331, et TROIS causes indépendantes

### 5.1 La preuve par la sonde (M3)

Boosts de classe annulés ⇒ les 4 assiettes de classe sont **exactement égales**
⇒ Divin gagne **strictement** dès qu'un seul fidèle existe
(`influence_base_gain`, `scps_influence.c:146-147`). Résultat :

```
distribution : … Aristocratie 5 ·          (Divin : 0)
```

Donc **au moment où chacun des 7 adoptants a choisi son courant,
`infl_believers` valait 0.** L'égalité a été tranchée par l'id ascendant
(`scps_ai.c:3130`), qui donne Aristocratie.

### 5.2 Cause A — LE CALENDRIER (le courant est pris avant la première foi)

Le courant occupe un slot rempli entre l'an 5 et l'an 40 (saturation an 72-96,
§2.2). L'IA ne fonde une religion qu'**au TEMPLE T2 bâti**
(`scps_ai.c:1770-1786`), or **T2 = 5 % des provinces à l'an 200**
(ligne « tiers de province (LOT T) », log s7). Et l'IA **n'abandonne jamais** :
`doctrines_abandon` n'est appelé nulle part dans `scps_ai.c`.

Mesuré : les foi(s) existent (3,0 fondée(s)/sim, 5 à 10 pays fidèles à l'an 200)
— **trop tard**.

### 5.3 Cause B — LE GRAIN (le bug de fond : écriture RÉGION, lecture PROVINCE)

`infl_believers` (`scps_influence.c:90-102`) somme `PopGroup.count` sur
**TOUTES les provinces** du pays. Or **le seul écrivain** de `PopGroup.faith`
dans tout le moteur est `region_set_native_faith`
(`scps_religion.c:99-106`) :

```c
static void region_set_native_faith(WorldEconomy *econ, int r, int rid){
  int rpid = econ_region_rep_province(econ, r);      /* ← LA province représentative */
  ProvincePop *pp = &econ->prov[rpid].pop;
  for(int i=0;i<pp->n_groups;i++)
    if(!pp->groups[i].diaspora) pp->groups[i].faith = rid;
}
```

**Une seule province par région reçoit la foi ; toutes les autres restent
athées à jamais** (`scps_econ.c:6156` pose `faith=-1` à la colonisation).

C'est exactement le motif « write-side région / read-side province » de la
re-key 2026-07, et c'est une violation directe de la doctrine CLAUDE.md
(« JAMAIS l'indirection `econ_region_rep_province` dans un chemin joueur »).

**Chiffrage :** graine 7 = 804 provinces / 297 régions = **2,71 prov/région**
⇒ au mieux **37 %** des âmes d'un pays sont marquées croyantes. Plafond effectif
du terme Divin :

```
0,37 × 1,667e-4 = 0,617e-4   <   Aristocratie 0,790e-4
```

**Divin ne peut PAS gagner, même à 100 % de conversion réelle.** La cause B
seule suffit à condamner la doctrine.

### 5.4 Cause C — LE PLAFOND DE FONDATION, et l'impact de la gate « a FONDÉ »

`religion_can_found` = racines < ⌈empires_de_genèse / 2⌉
(`scps_religion.h:154-155`). Avec 6 empires ⇒ **cap = 3**. Mesuré :
**3,0 foi(s) fondée(s)/sim dans 10 sims sur 10** — le cap est saturé partout.
Au-delà, les pays **RALLIENT** (`religion_adopt_existing`, `scps_ai.c:1785`) :
ils ne fondent jamais.

Schismes mesurés : 3, 2, 2, 7, 1, 10, 11, 2, 7, 6 (moy. **5,1/sim**) — mais
seul le mode **RUPTURE** fait de la couronne le fondateur de SA foi
(`religion_set_country(a->cid, child)`, `scps_ai.c:1822`) ; le mode **DÉRIVE**
garde le parent (`religion_fracture`, `:1824`) et **ne franchirait donc PAS la
gate**. Les logs confirment que la dérive aboutit rarement (« same-root/hérésie
0,0 à 1,0 » contre « foreign/zélote 3,0 à 9,0 »).

**Impact chiffré de la gate « a FONDÉ une religion » :**

| population éligible | aujourd'hui (`religion_of_country ≥ 0`) | avec la gate |
|---|---|---|
| pays fidèles / sim | **6,9** (6,5,5,8,4,9,8,5,10,9) | ~3 racines + les RUPTURE ⇒ **3 à 4** |
| en % des ~28 pays vivants | 25 % | **~12 %** |
| en % des 6 empires de genèse | ~100 % à terme | **50-65 %** |
| adoptions Divin observées | **0** | **0** |

**La gate ne coûte rien aujourd'hui** (Divin est déjà à 0) **et elle est
cohérente avec le design** (« fonder » est un acte, « rallier » n'en est pas
un). Mais appliquée seule, elle **grave dans le marbre une doctrine morte**.

### 5.5 Équilibrage chiffré de Divin, si on veut qu'il existe

Cible naturelle : Divin doit être **le plus fort des quatre**, parce qu'il exige
une religion FONDÉE + un Temple T2 + une conversion — trois conditions que les
courants de classe n'ont pas.

| `INFLUENCE_PER_BELIEVER` | gain à f = 100 % | seuil f où Divin passe Bourgeoisie | lecture |
|---|---|---|---|
| **1/6000 (actuel)** | **+33,5 %** | **61 %** | Divin = le plus fort, +13 pt sur Bourgeoisie |
| 1/9000 | +22,3 % | 91 % | Divin ≈ Bourgeoisie, seulement à conversion quasi totale |
| 1/4500 | +44,7 % | 46 % | Divin domine largement — déséquilibré |

**Le taux 1/6000 est BON.** Le problème n'est **pas** le taux : ce sont les
causes A et B. Ne pas toucher `INFLUENCE_PER_BELIEVER` avant de les corriger,
sinon on compense un bug par un tunable.

---

## 6. PROPOSITIONS CHIFFRÉES, CLASSÉES PAR IMPACT

### P1 — LE CONSEIL DÉCAPITÉ (impact : **×1,8 sur le revenu politique du joueur**)

**Site** : `scps/scps_influence.c:50-60` (`influence_council_mult`).
**Constat** : `if (slot < 0) continue;` ⇒ un ministre de rang I **dilue**.
Décapiter son Conseil vaut **+80 % d'influence et −37 % de salaire** (§4.1).
Le design écrit dit « siège vide compte 0 »
(`DESIGN_MISSIONS_DOCTRINES.md:219-222`) — **le code contredit la spec**.

**Trois variantes chiffrées :**

| variante | formule | 1 seul III | III+I+I | III+III+III | vide | effet sur l'IA (1 siège pourvu) |
|---|---|---|---|---|---|---|
| **actuelle** | Σt / n_pourvus | **3,00** | 1,67 | 3,00 | 1,00 | 2,22 |
| A — la spec littérale | Σt / SC_COUNCIL_SEATS | 1,00 | 1,67 | 3,00 | 0 → plancher 1,0 | **0,74** (−67 %) |
| **B — siège vide = plancher** *(recommandée)* | (Σt + 1,0×n_vides) / 3 | **1,67** | 1,67 | 3,00 | 1,00 | **1,41** (−36 %) |

**Recommandation : variante B.** Monotone (ajouter un ministre ne peut jamais
nuire), aucun tunable neuf (`INFLUENCE_COUNCIL_FLOOR` = 1,0 devient la valeur
d'un siège vide), l'incitation à pourvoir les 3 sièges revient.
**Risque** : divise le revenu de l'IA par 1,57 (elle ne pourvoit qu'un siège,
`scps_statecraft.c:514`) ⇒ la saturation glisse de l'an 96 à l'an ~150.
Golden à re-baseliner ; sweep apparié obligatoire. **À appliquer avant P3**
(elle fait déjà une partie du travail de P3).

### P2 — LES COÛTS PLATS PASSENT PAR é (impact : **×15 sur ce que paie un hégémon**)

**Sites** : `scps_sim.c:541` (envoi) · `scps_sim.c:543` (fabrication) ·
`scps_missions.c:95` (pivot). **Tunables** : `scps_tune_list.h:1902,1903,1448`.

| clé | actuel | **proposé** | hégémon é=30, mult 2 : mois de revenu | cité-état é=0,25 : mois de revenu |
|---|---|---|---|---|
| `INFLUENCE_COST_ENVOY` | 12 plat | **6 × é** | 0,10 → **1,50** | 24,1 → **6,0** |
| `INFLUENCE_COST_FAB` | 25 plat | **12 × é** | 0,21 → **3,00** | 50,2 → **12,0** |
| `DESSEIN_PIVOT_INFLUENCE` | 20 plat | **10 × é** | 0,17 → **2,50** | 40,2 → **10,0** |

**Effet** : le coût redevient invariant d'échelle (`C/(2·mult)` mois) — la
promesse du design (`DESIGN_MISSIONS_DOCTRINES.md:245`) est enfin tenue :
« on enchaîne si on a économisé, on est muet à sec », à toutes les tailles.
**Risque** : `é` doit être calculé au drain avec le courant actif — même appel
que `scps_sim.c:1053` (`influence_scale(econ, p, sim_influence_base(s,p))`) ;
aucun état neuf. Golden **inchangé** (chemin joueur seul).
**Réserve** : le pivot de Dessein est marqué « PLAT — aucune modulation d'éthos »
(`scps_tune_list.h:1445`) : passer par é n'est pas une modulation d'éthos, mais
c'est une décision joueur à acter.

### P3 — REMONTER LES MARCHES DE PRIX (impact : saturation an 96 → an ~150)

**Sites** : `scps_tune_list.h:1917` (`DOCT_COST_STEP`) · `:1919` (`IDEA_COST_STEP`).

| | actuel | proposé | Σ arbre | saturation à mult 1,7 |
|---|---|---|---|---|
| `DOCT_COST_STEP` | 25 (50→175) | **60** (50→350) | 675 → **1 200** | |
| `IDEA_COST_STEP` | 3 (30→135) | **8** (30→310) | 2 970 → **6 120** | |
| **total** | | | **3 645 → 7 320 (×2,01)** | **89 an → 179 an** |

C'est le F4 du rapport 02-09 : **il reste juste sous la clé neuve**, et c'est
désormais le **seul** frein de prix (le plancher é ne mord plus, §2.3).
**Risque** : couplé à la cadence d'un acte/an (42 ans plancher) et à
`AI_DOCT_RESERVE = 1,5`, un doublement des prix peut bloquer l'IA moyenne à
3-4 doctrines. **Ne pas appliquer seul** : soit avec le F1 du rapport précédent
(plusieurs actes par passage), soit **à la place de P1** — jamais les deux
(P1 × P3 = saturation repoussée à l'an ~280, l'arbre ne se finit plus jamais).

### P4 — DIVIN : LE GRAIN D'ABORD, LA GATE ENSUITE (impact : rend une doctrine possible)

**Ordre impératif** :

1. **Écrire la foi au grain PROVINCE.** Site : `scps_religion.c:99-106`
   (`region_set_native_faith`) — boucler sur **toutes** les provinces de la
   région, jamais `econ_region_rep_province`. Sans ça, plafond Divin
   = 0,617e-4 < Aristocratie 0,790e-4 : **la doctrine est arithmétiquement
   impossible** (§5.3). *Risque* : touche la religion (17 lecteurs « dominant »),
   golden + golden-deep à re-baseliner ; vague dédiée, pas un correctif d'un
   patch d'influence.
2. **Décider le courant après l'an 40** (ou permettre l'abandon IA). Sans ça, le
   slot est pris entre l'an 5 et 40 alors que le Temple T2 arrive après (§5.2).
3. **Puis** la gate « a FONDÉ ». Impact mesuré : éligibilité 25 % → **~12 %** des
   pays vivants, 50-65 % des empires de genèse (§5.4). *Coût aujourd'hui : nul*
   (0 adoption avec ou sans). *Risque si appliquée seule* : verrouille
   définitivement une doctrine morte.
4. **Ne PAS toucher `INFLUENCE_PER_BELIEVER` (1/6000)** : le taux est juste
   (+33,5 % à conversion totale, seuil de bascule f = 61 %, §5.5). Le corriger
   maintenant reviendrait à compenser le bug du §5.3 par un tunable.

### P5 — UN PUITS À L'ÉCHELLE DU ROBINET (impact : supprime 100 % du hoard post-saturation)

**Constat** : après l'an ~96, **tout** le robinet est thésaurisé (§3.3) ; le
puits maximal du joueur absorbe **10 %**, celui de l'IA **0 %**.

Le puits prévu au design (synergies fibonacci `0·2·3·5·8` /mois ×é,
`DESIGN_MISSIONS_DOCTRINES.md:318-330`) est **calibré sur la référence périmée
« ~4/mois »** (§1.4) : 4 synergies = 18×é/mois contre 2é×mult de revenu, soit
**4,5 mois de revenu par mois à mult 2** — intenable.

**Proposition : re-chiffrer le barème en fraction du revenu politique**, pas en
points :

| rang de synergie | barème écrit | **proposé** | hégémon é=30, mult 2 (revenu 120/mois) |
|---|---|---|---|
| 1re | 0 | **0** | gratuite |
| 2e | 2 × é = 60 | **0,10 × 2é×mult** | 12/mois |
| 3e | 3 × é = 90 | **0,15 ×** | 18/mois |
| 4e | 5 × é = 150 | **0,25 ×** | 30/mois |
| 5e | 8 × é = 240 | **0,40 ×** | 48/mois |

Cinq synergies = **90 % du revenu politique**. Le hoard disparaît, l'arbitrage
naît. *Risque* : les synergies **n'existent pas encore** — c'est une entrée de
spec pour la vague suivante, pas un tunable actionnable aujourd'hui.
*Palliatif refusé* : `INFLUENCE_CAP` (`scps_tune_list.h:1901`) est un **scalaire
plat** — un plafond qui écraserait l'hégémon libérerait le nain. À laisser à 0.

### P6 — APLATIR PRODUCTION (impact : libère 1 slot par pays)

**Site** : `scps_ai.c:3292-3293`.
**Constat** : `nbld×0,10` sature à **12 manufactures** alors que le monde en pose
**250/sim** ; `rawcap/nprov×0,03` sature à 26,7. **Production vaut 2,0 pour tout
pays développé, dès l'an ~20, et jusqu'à la fin** — 62 adoptions sur 331, et
présente chez **10 des 10** meilleurs adoptants du sweep.

**Proposition** : caps `1,2 → 0,8` et `0,8 → 0,5` (max 2,0 → **1,3**).
Production passe alors **sous** le courant (1,27) et sous Mercantilisme (1,4 à
l'an 1) ⇒ 1 slot se libère par pays au profit de Commerce/Vassaux/Défense.
**Risque élevé** : le brief §1.3 dit que ces poids « ne sont pas des tunables ».
Les bouger demande de **les inscrire au registre J d'abord**. C'est le levier
qui touche le juge du design lui-même — à mesurer en apparié, jamais en aveugle.

### P7 — HYGIÈNE : les cités-états ne devraient pas générer (impact : télémétrie honnête)

**Site** : `scps_sim.c:1270-1274`. Miroir de l'exclusion déjà présente dans
`ai_doctrines_year` (`scps_ai.c:3323`). Effet : la médiane d'influence du
chronicle cesse d'être écrasée par des pays qui ne dépenseront jamais (s512 :
« médiane 0,0 · max 38 109,6 »). *Risque* : nul côté équilibre ; change le hash
(un accumulateur cesse d'être écrit) ⇒ re-baseline.

### P8 — À NE PAS FAIRE

- **Plancher `é` 0,25 → 0,50** (F5 du rapport 02-09) : **caduque**. Il ne mord
  plus que sous 1 004 habitants (§2.3), et le nain de 500 âmes met déjà **305
  ans** à finir l'arbre. La queue parasite observée (`Ligue Karggoris`, 400 âmes,
  3 doctrines complètes) vient d'un **héritage de taille**, pas du plancher —
  et le « flat sans entretien » est une décision joueur ferme.
- **Revoir la formule de `é`** : elle fait exactement ce qu'elle promet. Vérifié :
  le temps d'acquisition est invariant d'échelle à 8 % près sur mesure réelle
  (§2.2), et l'exploit « renvoyer les ministres pour brader les prix » est bien
  fermé (`scps_influence.c:154`, l'échelle ne lit jamais le Conseil).

---

## 7. Verdict en 6 lignes

1. **L'assiette est juste** : 4,98e-4 /hab/mois, parts 63/20/16 contre 60/20/20
   visés. La re-key SIÈGES du 03-09 a réparé le facteur ~10.
2. **La linéarisation par é marche** : temps d'acquisition invariant d'échelle,
   prédit à 8 % près (152/mult ans pour l'arbre entier).
3. **Le Conseil est cassé dans le sens inverse de celui qu'on surveillait** :
   un ministre médiocre DILUE ; décapiter son Conseil vaut +80 % d'influence
   et −37 % de salaire. Le code contredit le design écrit.
4. **Les coûts plats sont inertes au sommet (×241 d'écart)** : un hégémon paie
   un émissaire 0,10 mois de revenu, une cité-état 24 mois.
5. **Le robinet n'a pas de puits** : l'arbre sature à l'an 96, après quoi 100 %
   de la génération est thésaurisée (max 12 265 à l'an 120, 70 354 à l'an 200).
6. **Divin est mort de trois causes indépendantes** — le calendrier (courant pris
   avant la première foi), **le grain (la foi n'est écrite que sur la province
   représentative : plafond 0,617e-4 < Aristocratie 0,790e-4)**, et le plafond de
   fondation ⌈N/2⌉ saturé dans 10 sims sur 10. Le taux 1/6000 n'y est pour rien.
