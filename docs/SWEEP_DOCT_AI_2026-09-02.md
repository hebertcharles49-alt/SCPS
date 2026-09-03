# DÉPOUILLEMENT — sweep apparié « l'IA joue les doctrines » (10 × 200 ans × 2 bras)

Dossier : `sweep_doct_ai_10x200/` · protocole `sweep-doct-ai-paired-3x3-v1` ·
binaire `8140fa92…` · lancé 2026-09-02T17:56Z, fini 18:54Z · `nonzero_runs=0`.
Graines 7 · 1009 · 4243 · 11 · 2026 · 512 · 3333 · 777 · 90 · 60.
Bras : `temoin` = `AI_DOCT=0` (kill-switch, l'IA n'adopte jamais) · `essai` = défaut.

---

## 1. Méthode

**Les 20 journaux ont été lus INTÉGRALEMENT, ligne à ligne, de la première à la
dernière** (16 707 lignes au total : en-têtes worldgen, colonnes périodiques an
40/80/120/160, bilans monétaires, BILAN an 200, bilan DOCTRINES, le dump PROV
complet de chaque monde — 500 à 870 lignes par journal —, les listes d'empires
vivants, les instantanés par âge et la SYNTHÈSE finale). Aucun `grep`, `awk`,
`sed`, ni filtre d'aucune sorte n'a servi à *découvrir* quoi que ce soit : les
anomalies du §5 ont toutes été vues en lecture. Les seules commandes exécutées
hors lecture sont (a) `wc -l` + `cat *.rc` pour dimensionner le corpus et
vérifier les codes de retour, (b) après la lecture, la relecture du code moteur
(`scps_ai.c`, `scps_doctrines.c`, `scps_influence.c`) pour ancrer les
propositions du §6 sur les formules réelles.

Contrôle de sanité : les 20 `*.rc` valent `0`. Le bras témoin est **muet**
(0 doctrine, 0 idée, corrélations 0/0 dans les 10 journaux) — le kill-switch
tient. Aucun `ASSERT`, aucun `NaN`, aucun `inf`, aucun avertissement moteur,
aucune fin prématurée : les 20 sims vont bien à l'an 200 avec 6 âges chacune.
L'invariant M3f culmine à 130 % pour un seuil courant de 370 %.

---

## 2. Équilibre général apparié, PAR GRAINE

### 2.1 Le tableau

| graine | bras | pays (BILAN) | pop | guerres | M(fin) | indice prix moy | fin §27 (an) | âges |
|---|---|---:|---:|---:|---:|---:|---|---:|
| **7** | témoin | 27 | 1 052 k | 65 | 4 404 722 | 0,599 | GRAND HIVER (180) | 6 |
| | essai | **37** | 929 k | **47** | 3 984 966 | 0,514 | **ENGLOUTISSEMENT (180)** | 6 |
| **1009** | témoin | 25 | 604 k | 65 | 3 855 245 | 0,457 | GRAND HIVER (180) | 6 |
| | essai | 28 | 608 k | 63 | **6 055 077** | 0,548 | GRAND HIVER (180) | 6 |
| **4243** | témoin | 36 | 523 k | 53 | 3 275 173 | 0,390 | ENGLOUTISSEMENT (180) | 6 |
| | essai | **27** | **674 k** | 53 | **6 175 469** | 0,325 | **RONCES (180)** | 6 |
| **11** | témoin | 28 | 957 k | 114 | 5 838 148 | 0,430 | GRAND HIVER (180) | 6 |
| | essai | 30 | 939 k | **90** | 6 708 167 | 0,391 | GRAND HIVER (180) | 6 |
| **2026** | témoin | 29 | 583 k | 32 | 3 668 719 | 0,410 | RONCES (180) | 6 |
| | essai | 29 | 603 k | 33 | **5 835 038** | 0,465 | **GRAND HIVER (180)** | 6 |
| **512** | témoin | 30 | 798 k | 64 | 10 997 403 | 0,702 | RONCES (180) | 6 |
| | essai | **44** | 681 k | **87** | 9 349 161 | **0,360** | **ENGLOUTISSEMENT (180)** | 6 |
| **3333** | témoin | 28 | 795 k | 86 | 3 037 884 | 0,247 | RONCES (180) | 6 |
| | essai | 28 | 812 k | 87 | **2 110 599** | **0,143** | RONCES (180) | 6 |
| **777** | témoin | 25 | 1 248 k | 13 | 10 885 690 | 0,496 | RONCES (180) | 6 |
| | essai | 25 | 1 242 k | **32** | **15 493 253** | 0,386 | RONCES (180) | 6 |
| **90** | témoin | 22 | 959 k | 70 | 7 692 062 | 0,297 | GRAND HIVER (180) | 6 |
| | essai | 23 | 918 k | 57 | 6 857 962 | 0,297 | GRAND HIVER (180) | 6 |
| **60** | témoin | 27 | 893 k | 71 | 7 382 937 | 0,404 | GRAND HIVER (180) | 6 |
| | essai | 25 | 914 k | 62 | 5 808 192 | 0,336 | GRAND HIVER (180) | 6 |

### 2.2 Les moyennes (10 graines)

| mesure | témoin | essai | Δ |
|---|---:|---:|---:|
| pays subsistants (an 200) | 27,7 | 29,6 | **+6,9 %** |
| population mondiale | 841 k | 832 k | −1,1 % |
| guerres déclenchées | 63,3 | 61,1 | −3,5 % |
| M(fin) | 6 103 798 | 6 837 788 | +12,0 % |
| indice de prix moyen | 0,443 | 0,377 | **−14,9 %** |
| trésor moyen / empire | 20 692 or | 18 343 or | −11,4 % |
| revenu d'État « assiette » | 5 497 or/an | 5 015 or/an | −8,8 % |
| commerce inter-pays / an | 3 348 | 3 612 | +7,9 % |
| satisfaction Laboureur | 47,6 % | **49,3 %** | +1,7 pt |
| richesse/tête Laboureur | 1,80 | **2,51** | **+39 %** |
| richesse/tête Élite | 77,8 | 71,9 | −7,6 % |
| pays émergés (sécession) | 6,0 | 7,9 | +32 % |
| pays absorbés | 2,3 | 2,3 | 0 |
| entropie faustienne monde | 64 029 | 63 193 | −1,3 % |
| âges advenus | 6,0 | 6,0 | 0 |

Les âges : 6/6 partout, dans les deux bras. Seule la **date** des Lumières
bouge, de −11 à +9 ans selon la graine (moyenne −1,3 an) — bruit, pas signal.

### 2.3 VERDICT

**L'arbre ne déplace PAS l'équilibre en moyenne, mais il augmente FORTEMENT la
variance par graine, et le sens du déplacement est le bon.**

Trois choses sont vraies simultanément :

1. **Les agrégats mondiaux tiennent.** Population −1 %, guerres −3,5 %, âges
   identiques, entropie faustienne identique, aucune fin §27 supplémentaire ni
   manquante (10 fins dans chaque bras). L'arbre n'a rien cassé.

2. **La répartition, elle, s'améliore nettement — au bénéfice du bas.**
   Richesse/tête du Laboureur +39 % (1,80 → 2,51), satisfaction +1,7 pt, indice
   de prix −15 %, tandis que la richesse d'Élite recule de 7,6 % et le trésor
   d'État de 11 %. Autrement dit : les doctrines déplacent de la valeur du
   trésor et de l'élite vers la production réelle et le prix payé par le peuple.
   C'est cohérent avec la distribution adoptée (Production 62 · Colonisation 47
   · Infrastructure 38 · Commerce 37 = 55 % des adoptions). **C'est SAIN.**

3. **La dispersion par graine est le vrai coût.** M(fin) bouge de −31 % (s3333)
   à +89 % (s4243) selon le monde ; le nombre de pays de −25 % (s4243) à +47 %
   (s512) ; les guerres de −24 (s11) à +23 (s512). Et surtout, **la fin §27
   change dans 4 mondes sur 10** (s7 HIVER→EAU, s4243 EAU→RONCES, s2026
   RONCES→HIVER, s512 RONCES→EAU) — toujours à l'an 180, jamais avec un
   déclencheur nouveau. C'est l'effet papillon attendu d'un moteur déterministe
   qu'on perturbe dès l'an 2, pas un déséquilibre.

Le seul point qui mérite une décision : **s512 fragmente** (30 → 44 pays,
23 sécessions contre 7, pic de révolte 29 contre 20, influence médiane
exactement 0,0). Ce monde-là dégénère en poussière de micro-États sous l'arbre.
Voir §5 anomalie A1.

---

## 3. La vie de l'arbre

### 3.1 Premières adoptions et rythme

La cadence moteur est `AI_DOCT_CHECK_MONTHS=12`, **arrondie en ANNÉES**
(`scps_sim.c:1621`), et `ai_doctrines_year` fait **au plus UN acte par
passage** : adopter *ou* acheter une idée, jamais les deux
(`scps_ai.c:3343-3364`). Le plafond théorique est donc de **1 acte/an/pays**,
soit 6 slots + 36 idées = **42 ans minimum** pour remplir un arbre complet.

Les journaux le confirment exactement — dès la **première colonne périodique
(an 40)**, l'arbre est déjà aux trois quarts construit :

| état du monde | an 40 | an 80 | an 120 | an 160 | an 200 |
|---|---:|---:|---:|---:|---:|
| doctrines actives (moy. 10 graines) | 22,1 | 28,3 | 30,7 | 32,0 | 33,1 |
| idées possédées (moy.) | 98,3 | 148,2 | 168,6 | 176,7 | 179,8 |
| influence médiane (moy.) | **22,5** | 51,2 | 1 067 | 3 944 | — |
| *idem, bras témoin* | *307,9* | *780,4* | *1 622* | *3 395* | — |

Autrement dit : **l'adoption est finie avant l'an 40**, l'achat d'idées avant
l'an 100-120, et l'arbre est **GELÉ** pendant la seconde moitié de la partie
(+8 % de doctrines et +7 % d'idées entre l'an 120 et l'an 200 — et cette
poussée-là vient de pays neufs, pas d'arbres qui grandissent).

Premières adoptions : dès l'an 1-10 (aucune colonne avant l'an 40, mais 21 à 29
doctrines actives à l'an 40 pour 6 empires × 6 slots = 36 possibles, c'est-à-dire
**58 à 81 % de saturation à l'an 40**, ce qui exige que les slots aient été pris
dans les toutes premières années).

### 3.2 L'influence dans le temps — le stock est consommé, puis inutile

| an | influence méd. témoin | influence méd. essai | ratio |
|---:|---:|---:|---:|
| 40 | 307,9 | 22,5 | **÷ 13,7** |
| 80 | 780,4 | 51,2 | ÷ 15,2 |
| 120 | 1 622 | 1 067 | ÷ 1,5 |
| 160 | 3 395 | 3 944 | **× 1,16** |

C'est la courbe la plus parlante du sweep : **pendant 80 ans l'IA vit à découvert
(elle dépense 93 % de ce qu'elle génère), puis l'arbre est plein et le stock
s'accumule sans emploi** — et finit par DÉPASSER le bras témoin (influence
générée cumulée : 102 190 en essai contre 79 577 en témoin, +28 %, parce que les
doctrines de Production/Infrastructure font grossir les assiettes qui la
génèrent). Après l'an 120, l'influence est une monnaie morte.

Le maximum, lui, explose : jusqu'à **70 354** (essai s777, Couronne Thrumdinyn)
et 38 110 (essai s512, Clans Tikexis). L'écart médiane/max atteint **∞** en
essai s512 (médiane 0,0 · max 38 110).

### 3.3 Distribution finale (bras ESSAI, 331 adoptions, 66 pays adoptants)

| doctrine | n | part | | doctrine | n | part |
|---|---:|---:|---|---|---:|---:|
| **Production** | 62 | 18,7 % | | Mercantilisme | 12 | 3,6 % |
| **Colonisation** | 47 | 14,2 % | | Diplomatie | 4 | 1,2 % |
| **Aristocratie** *(courant)* | 43 | 13,0 % | | Technologie | 2 | 0,6 % |
| Infrastructure | 38 | 11,5 % | | Peuple | 2 | 0,6 % |
| Commerce | 37 | 11,2 % | | Connaissances | 2 | 0,6 % |
| Vassaux | 24 | 7,3 % | | Bourgeoisie *(courant)* | 1 | 0,3 % |
| Offense | 23 | 6,9 % | | **Faustien** | **0** | **0 %** |
| Défense | 22 | 6,6 % | | **Divin** *(courant)* | **0** | **0 %** |
| Populaire *(courant)* | 12 | 3,6 % | | | | |

**Les COURANTS** : 56 adoptions sur 331 (17 %), soit 85 % des pays adoptants
(l'exclusivité §4.1 en autorise un seul). Aristocratie rafle **77 %** des
courants, Populaire 21 %, Bourgeoisie 2 %, Divin **0 %**.

### 3.4 TOP-3 par pays — ce que ça donne concrètement

Sur les 66 lignes de pays lues, le motif est écrasant :

- **Production apparaît dans le TOP-3 de 55 des 66 pays adoptants** et dans les
  10 graines sans exception. C'est la doctrine par défaut de tout le monde.
- Les trois grands empires typiques d'une graine ont presque toujours le même
  triptyque : `Production + Colonisation + (Aristocratie | Vassaux | Commerce)`.
  Exemples littéraux : `pays 55 Ligue Pyxexis : Production(6) Colonisation(6)
  Vassaux(6)` (essai s7:124) · `pays 3 Ligue Aldyana : Vassaux(6) Production(6)
  Colonisation(6)` (essai s1009:116) · `pays 36 Havre Mithwena : Production(6)
  Colonisation(6) Commerce(6)` (essai s4243:117) · `pays 97 Couronne Thrumdinyn :
  Production(6) Colonisation(6) Commerce(6)` (essai s777:119).
- Les doctrines rares n'apparaissent **QUE chez des micro-États** : `pays 8
  Mécaniste libre … Bourgeoisie(6 idées)` et `pays 19 Métallurgiste libre …
  Connaissances(2 idées)` (essai s11:116-118), `pays 38 Havre Gualred … Peuple(1
  idée)` (essai s2026:120), `pays 49 Havre Dornwica` / `pays 123 Ordre Brakrak`
  (essai s777:117,121). Ce sont des polities de 1 région et 0-2 k habitants.

### 3.5 POURQUOI cette distribution — remontée aux signaux

La table des signaux (`scps_ai.c:3140-3300`) et la boucle d'adoption
(`scps_ai.c:3336-3349`) expliquent **tout**, sans reste.

**(a) Le verrou du calendrier.** L'adoption est tentée **avant** l'achat d'idée,
et elle réussit tant qu'un slot est libre. Les 6 slots sont donc pris dans les
6 à 12 premières années réussies — **puis `ai_doctrines_year` ne teste PLUS
JAMAIS l'adoption** (les slots sont pleins, `doctrines_why_not` refuse). Et
comme la v107 a supprimé entretien et suspension, **rien ne se libère jamais**.
Conclusion : *les six doctrines d'un pays sont déterminées par ses scores de
l'an 1-10 et ne bougent plus pendant 190 ans.*

**(b) Qui a un score non nul à l'an 1 ?**

| doctrine | terme d'amorce | valeur an ~2 | plafond |
|---|---|---:|---:|
| **Production** | `rawcap/nprov × 0,03` — les brutes existent **dès la genèse** | **> 0 toujours** | 2,0 |
| **Colonisation** | capitale côtière (+1,0) · chantier actif (+1,0) · fronts vierges (+0,6) | 1,0 à 2,6 | **2,6** |
| **Aristocratie** | `1,0 + part du courant` | ≈ 1,35 | 2,0 |
| Mercantilisme | `nroutes ≤ 1 → +0,6` — vrai au démarrage ! | ≈ 0,6-1,0 | 2,4 |
| Infrastructure | `dens×0,8 + vétusté×4` — bâti neuf ⇒ vétusté nulle | ≈ 0 | 2,4 |
| Commerce | `nroutes×0,25` — 0 route à l'an 1 | 0 | 2,5 |
| Offense / Défense | `nwar×2,0` — pas de guerre à l'an 1 | 0 | 3,0+ |
| Vassaux | exige `nvassal > 0` | 0 | 2,6 |

Colonisation est **la seule doctrine dont le plafond (2,6) dépasse celui de
Production (2,0) et d'Aristocratie (2,0)** : tant qu'un chantier tourne et que la
capitale est côtière, elle est mécaniquement servie la première. Production est
**la seule strictement positive pour tout le monde à l'an 1**. Aristocratie est
servie d'office parce que le courant est toujours attribué. **Ces trois-là
prennent les trois premiers slots dans presque tous les mondes** — 62+47+43 = 152
adoptions, soit 46 % du total. Infrastructure et Commerce prennent les slots 4-6
un peu plus tard (vers l'an 15-40, quand la vétusté et les routes existent) :
38+37 = 75, soit 23 % de plus. **69 % de l'arbre est expliqué par l'ordre
chronologique d'apparition des signaux, pas par la situation du pays.**

**(c) Pourquoi Offense/Défense/Vassaux malgré tout (23+22+24 = 69) ?** Parce
que leurs signaux sont les plus FORTS quand ils se déclenchent (2,0 par guerre,
plafond 3,0 ; +0,4 durable via la trêve) et qu'ils tombent souvent avant que les
6 slots soient pleins — mais seulement si la première guerre arrive tôt. C'est
exactement ce que montrent les corrélations-juges (§4) : elles sont excellentes
là où les guerres commencent tôt (s3333 : 75 %) et médiocres là où le monde est
pacifique 60 ans (s777 : **17 %**, 13 guerres seulement en témoin).

**(d) Pourquoi Technologie (2), Connaissances (2), Peuple (2) sont quasi
absentes.** Leurs termes sont **structurellement faibles ET tardifs** :
- **Technologie** = `nlib×0,6 + savoir/pop`. Les bibliothèques/monastères sont
  des édifices de palier ; il n'y en a aucun à l'an 5. Le terme `savoir/pop`
  vaut ~10⁻² . Score ≈ 0 pendant 30 ans.
- **Connaissances** = `métabolisation×3 + héritages digérés×0,4`. La
  métabolisation moyenne du monde à l'an 200 vaut **1,3 à 11,8 %** (lu dans les
  20 journaux) ; à l'an 5 elle est nulle. Terme max réel ≈ 0,35.
- **Peuple** = `diaspora/pop ×3 + pactes migratoires×0,6`. Le brassage compte
  0 à 507 flux **sur 200 ans** ; à l'an 5 il n'y a ni diaspora ni pacte.
  Terme ≈ 0. (Et 2 journaux ont **0 flux migratoire de toute la partie** — cf.
  A11.)

Ces trois-là ne peuvent gagner un slot **qu'après** l'an 30-50 — quand il n'y en
a plus. Les 6 adoptions observées viennent toutes de micro-États nés tard, qui
disposent de slots vierges dans un monde déjà mûr.

**(e) Pourquoi Faustien est du CODE MORT (0 sur 331, 0 sur 10 mondes).**
Le score exige `nfaust > 0` — au moins un nœud faustien déjà déverrouillé. Les
journaux donnent 14 à 51 nœuds faustiens par sim, mais ils tombent **après l'an
100** (l'Âge de la Brèche est l'an 181). Les 6 slots sont pris depuis 60 ans.
**Faustien ne peut PAS être adopté dans l'état actuel du moteur.**

**(f) Pourquoi Divin est mort et Bourgeoisie quasi morte.** Le courant se décide
par `aid_best_current`, qui compare les **gains** (taux × effectif), pas les
effectifs. Avec la répartition de départ **80/15/5** et les taux du registre :

| assiette | taux | part départ | gain relatif | part régime (89/8/2) | gain relatif |
|---|---:|---:|---:|---:|---:|
| **Aristo** | 0,0025 | 5 % | **1,25 · 10⁻⁴** ← gagne | 2 % | 0,50 · 10⁻⁴ |
| Laborer | 0,00012 | 80 % | 0,96 · 10⁻⁴ | 89 % | **1,07 · 10⁻⁴** ← gagnerait |
| Bourgeois | 0,0006 | 15 % | 0,90 · 10⁻⁴ | 8 % | 0,48 · 10⁻⁴ |
| **Faith** | 0,08 | **foi bâtie = 0** (monde ATHÉE au départ, fondation au Temple T2) | **0** | | |

Aristocratie gagne au démarrage **de 30 % seulement** — mais elle gagne au seul
moment où la décision se prend, et le slot est verrouillé pour 200 ans. En
régime, c'est Populaire qui devrait l'emporter (+114 %) : elle n'y arrive que
12 fois, chez des pays nés tard. Bourgeoisie ne gagne **jamais** (3ᵉ au départ
et 3ᵉ en régime). Divin ne peut **jamais** gagner : sa base vaut 0 au moment du
choix, par construction du monde athée.

---

## 4. Les corrélations-juges — ce qu'elles prouvent

Deux assiettes sont imprimées : **tous les pays** (dénominateur = tous les
côtiers / suzerains / belligérants vivants) et **les ADOPTANTS seuls**
(dénominateur = ceux qui tiennent au moins une doctrine).

| graine | côtiers→Colonisation | suzerains→Vassaux | belligérants→Off/Déf |
|---|---|---|---|
| | *tous* → **adoptants** | *tous* → **adoptants** | *tous* → **adoptants** |
| 7 | 29 % → **44 %** (4/9) | 100 % → **100 %** (3/3) | 62 % → **83 %** (5/6) |
| 1009 | 100 % → **100 %** (5/5) | 100 % → **100 %** (1/1) | 67 % → **67 %** (4/6) |
| 4243 | 83 % → **83 %** (5/6) | 33 % → **33 %** (2/6) | 71 % → **83 %** (5/6) |
| 11 | 100 % → **100 %** (4/4) | 100 % → **100 %** (2/2) | 43 % → **50 %** (3/6) |
| 2026 | 33 % → **33 %** (2/6) | 75 % → **75 %** (3/4) | 50 % → **50 %** (3/6) |
| 512 | 14 % → **75 %** (3/4) | 50 % → **67 %** (2/3) | 33 % → **50 %** (3/6) |
| 3333 | 100 % → **100 %** (8/8) | 71 % → **71 %** (5/7) | 75 % → **75 %** (6/8) |
| 777 | 100 % → **100 %** (4/4) | 50 % → **50 %** (2/4) | 17 % → **17 %** (1/6) |
| 90 | 80 % → **80 %** (4/5) | 100 % → **100 %** (2/2) | 75 % → **75 %** (3/4) |
| 60 | 83 % → **83 %** (5/6) | 67 % → **67 %** (2/3) | 67 % → **67 %** (4/6) |
| **Σ pondéré** | **44/57 = 77 %** | **24/35 = 69 %** | **37/60 = 62 %** |
| *(moy. des ratios, `resume.txt`)* | *76 %* | *75 %* | *59 %* |
| *témoin (contrôle)* | *0 %* | *0 %* | *0 %* |

**Ce que ça prouve :**

1. **L'IA choisit bien sur son ÉTAT, pas au hasard.** Une adoption aléatoire
   parmi 17 doctrines donnerait ~6 % par doctrine et ~35 % pour la paire
   Offense/Défense sur 6 slots. On mesure 77 / 69 / 62 %. Le départage est réel
   et franc.

2. **L'écart entre les deux assiettes MESURE le silence des micro-États, pas
   une erreur de l'IA.** Il n'existe que là où beaucoup de pays n'adoptent rien :
   s512 (14 % → 75 %, avec 23 « empires vivants » dont 17 sans une seule
   doctrine) et s7 (29 % → 44 %, 10 adoptants sur 16 polities). Dans 7 graines
   sur 10, les deux chiffres sont **identiques**. C'est l'assiette ADOPTANTS
   qu'il faut lire.

3. **Le juge le plus faible est le juge martial (62 %), et sa faiblesse est
   datée, pas structurelle.** Les deux mauvais scores sont s777 (17 %,
   **13 guerres** en 200 ans dans le bras témoin) et s11/s512/s2026 (50 %). Là
   où le monde se bat tôt et souvent — s3333 (86 guerres, 75 %), s90 (70
   guerres, 75 %), s4243 (83 %) — la corrélation est franche. **Ce n'est pas le
   poids du signal martial qui est mauvais, c'est le fait qu'il arrive trop tard
   dans un monde pacifique** : les 6 slots sont pris avant la première guerre.
   C'est la même cause que le §3.5(a).

4. **Le juge Vassaux (69 %) a un seul point noir, s4243 (2/6).** Ce monde compte
   14 à 18 protectorats mais seulement 7 empires vivants ; la suzeraineté y
   arrive après la saturation des slots. Même diagnostic.

**Conclusion du §4 : les corrélations valident le SCORE (l'IA lit bien son
état) et invalident la CADENCE (l'état lu est celui de l'an 5, pas celui de
l'an 100).**

---

## 5. Anomalies

Tout ce qui sort de l'ordinaire, dans n'importe quel journal, bras confondus.
Rien de fatal : **aucun ASSERT, aucun NaN, aucune fin prématurée, aucun rc≠0.**

### A1 — Influence médiane exactement 0,0 · `essai_s512_y200.log:113`
`influence : médiane 0.0 · max 38109.6` — avec `6 pays sur 23` adoptants et
23 « empires vivants ». Plus de la moitié des polities du monde ne génère
**rien** politiquement. Ce monde a fragmenté en 44 pays (contre 30 en témoin),
23 sécessions (contre 7), pic de révolte 29 à l'an 181 (contre 20 à l'an 27).
**C'est l'écart témoin/essai le plus violent du sweep.**

### A2 — Armées à ZÉRO régiment, en masse · `essai_s512_y200.log:645-706`
16 des 23 empires portent `armée N (0 rgt)`, dont `Ordre Pyxil 12 rég · pop 56k
· 64 tech · armée 11 (0 rgt)` (l.645) et `Clans Fizzexa 5 rég · armée 4 (0 rgt)`
(l.654). **Aucun cas comparable dans `temoin_s512`** (tous ses empires ont
4-42 régiments). Une armée existante sans un seul régiment est une contradiction
avec le modèle « renfort = déficit » de la vague v97.

### A3 — Régiments hypertrophiés (présent dans les DEUX bras, **amplifié** en essai)
- `temoin_s11:668` : `Clans Hobwickis 12 rég · pop 63k · armée 13 (165 rgt)`
- `essai_s11:675` : `Clans Hobwickis 10 rég · pop 57k · armée 12 (**317 rgt**)`
- `temoin_s3333:645` : `Mécaniste libre 7 rég · armée 5 (167 rgt)`
- `essai_s3333:657` : `Adaptatif libre 1 rég · pop 37k · armée 15 (77 rgt)`
- `essai_s777:781` : `Couronne Falwick 6 rég · pop 79k · armée 14 (**342 rgt**)`
  — contre `temoin_s777:772` : `Couronne Falwick 15 rég · armée 14 (50 rgt)`.
Le ratio normal est de 1 à 3 régiments par armée. 342/14 = **24**. Le phénomène
préexiste au sweep mais les doctrines le multiplient par 2 à 7.

### A4 — Trésors NÉGATIFS (essai seulement)
`essai_s512:705` : `Ordre Caelwic 1 rég · pop 15k · or **-42**`.
`essai_s3333:654` : `Ordre Dornwica 2 rég · or **-58**`.
Aucun trésor négatif dans les 10 témoins. À rapprocher de A2 (mêmes journaux).

### A5 — Doctrine adoptée avec ZÉRO idée · `essai_s777_y200.log:121`
`pays 123 Ordre Brakrak influence 11.2 : Aristocratie(6 idées) **Infrastructure(0 idée)**`
Un slot consommé pour rien : l'IA a payé l'adoption puis n'a plus jamais eu de
quoi acheter la première idée. Le slot est perdu à vie (aucune suspension en
v107). Symptôme direct de « adopter d'abord, payer ensuite ».

### A6 — Le compteur de doctrines RECULE (adoptions puis pertes)
`essai_s90` : an 80 → **28** doctrines / 149 idées ; an 120 → **24** / 133 ;
an 160 → 25 / 135. Quatre doctrines et **seize idées disparues**.
`essai_s512` : idées 192 (an 160) → 158 (fin) — **34 idées perdues en 40 ans**.
Ce ne sont pas des abandons (v107 : aucun entretien, « les doctrines adoptées
restent ALLUMÉES »), ce sont des **pays qui meurent**. Le compteur mondial est
donc un instantané de pays vivants, pas un stock cumulé — la ligne de télémétrie
devrait le dire, sinon elle se lit à l'envers.

### A7 — Micro-État à structure de classe inversée · `temoin_s512_y200.log:687`
`Métallurgiste libre 1 rég · pop 0k · classes : J 0.0k (**0 %**) · B 0.0k (11 %)
· É 0.0k (**89 %**)` — 89 % d'élites, zéro laboureur — et marqué `hub OUI`.

### A8 — Entité « libre » promue empire avec métriques nulles · `essai_s4243:628`
`Ésotérique libre 1 rég · pop 0k · **Stab 0 Prosp 0** Légit 48 Cohés 100 Corr 39
· 0 tech`. Stabilité ET prospérité à zéro pile, dans la liste des empires
vivants. Les entités « X libre » (hameaux WILD promus) polluent les
dénominateurs des juges et la liste des empires dans 6 journaux.

### A9 — Provinces hypertrophiées (le plus gros écart individuel du sweep)
- `temoin_s90:458` : `PROV 409 Granmetfurt pays=70 pop=**110**`
  → `essai_s90:463` : `PROV 409 Granmetfurt pays=70 pop=**30 174**` — **× 274**.
- `temoin_s3333:617` : `PROV 553 Grandaber pop=6 896`
  → `essai_s3333:623` : `PROV 553 Grandaber pop=**27 828**` — × 4.
  Et l'empire correspondant : `essai_s3333:656` `Adaptatif libre **1 rég · pop
  37 k**` — 37 000 habitants dans une seule province.
- `essai_s60:160` : `PROV 40 Granbrive pays=70 pop=**22 697**`
  contre `temoin_s60:155` `PROV 40 Granbrive pays=33 pop=**47**`.

### A10 — Bâtiments faustiens qui apparaissent sous l'arbre
`essai_s2026:668` : `**77×Atelier de mage**` (témoin : 3) et `1×Foreuse
arcanique` avec `conso foreuse 301` (témoin : 0 / 0).
`essai_s777:798` : `**7×Foreuse arcanique**`, `conso foreuse 2527`, contre 0/0
au témoin. Les doctrines ouvrent des chaînes faustiennes que le témoin
n'atteint pas — à surveiller vis-à-vis du garde-fou `FAUST_BRECHE_CAUTION`
(alors même que la doctrine Faustien, elle, n'est jamais adoptée : cf. §3.5(e)).

### A11 — Canal migratoire à ZÉRO DUR
`essai_s1009:564` : `brassage : **0 flux** de pacte migratoire (0 âmes)` —
alors que `temoin_s1009:569` en compte 93. Également `temoin_s2026:697` : 0 flux.
Un canal qui tombe à zéro absolu sur 200 ans, dans un bras et pas l'autre.

### A12 — Compteur `remplacement(s) IA` à ZÉRO DUR
`temoin_s2026:749`, `essai_s2026:759`, `temoin_s777:880`, `essai_s777:889` :
`0.0 remplacement(s) IA/sim`. Ailleurs ce compteur vaut 48 à **438**. Quatre
journaux à zéro pile sur une métrique de Conseil qui devrait toujours bouger.

### A13 — Ratios choc/poursuite hors gabarit
`temoin_s2026:743` : `morts choc **300** vs POURSUITE 10 300 (ratio **34,3×**)`,
avec `0 décrochage · 0 renfort`.
`essai_s3333:753` : `700 vs 16 200 (**23,1×**)`.
`temoin_s90:880` : `1 700 vs 20 300 (11,9×)`.
La cible implicite du commentaire moteur est ~2-5×.

### A14 — Explosion des batailles et des renforts sur un monde identique
`temoin_s60:877` : `332 livrées · 13 renforts`
→ `essai_s60:888` : `**496 livrées** · **256 renforts**` (morts choc 7 300 vs
poursuite 45 600). +49 % de batailles et **× 20 de renforts** pour la même
graine. Cohérent avec l'adoption d'Offense par 3 des 6 empires de ce monde.

### A15 — Crédit : ratio dette/revenu × 160 · `temoin_s512_y200.log:106`
`taux moyen **21,81 %** · **5** dette(s) structurelle(s) ≥3× · max **159,97×**
· 9 marchés étrangers fermés`. Le maximum suivant sur tout le corpus est 16,98×
(`temoin_s4243:106`). Un pays porte 160 années de revenu en dette.

### A16 — « 100 % du commerce mondial » sur un marché mort
14 journaux sur 20 affichent `hubs : 100 % du commerce mondial passe par les
Centres des cités-états`, mais avec des volumes qui s'effondrent :
`temoin_s4243:605` `(76 / 76)` et `essai_s4243:632` `(**33 / 33**)`. Un ratio
de 100 % sur 33 unités de volume n'est pas la preuve d'un hub, c'est la preuve
qu'il n'y a plus de commerce hors-Centres. La métrique devrait être gardée par
un plancher de volume.

### A17 — L'entrepôt DÉSTABILISE le prix (lissage CENTRES inversé)
`essai_s2026:673` : `σ centres À entrepôt **1,198** vs centres SANS 0,978`.
`essai_s3333:673` : `**1,155** vs 0,607`. `temoin_s3333:667` : `0,951 vs 0,591`.
`essai_s60:859` : `0,843 vs 0,763`. Quatre journaux où la mesure causale part
dans le mauvais sens (ailleurs le rapport est franchement favorable, ex.
`temoin_s4243:618` `0,000 vs 1,606`).

### A18 — Provinces sans propriétaire (`pays=-1`) et comptage divergent
`pays=-1` apparaît dans les deux bras (`temoin_s4243:189,198,202,216,238,244,
301,374,387,446,489,544` — 12 cas ; `essai_s7:171,175,205,216,222,259,278,283,
290,314,318,360,374,413,417,423,426,427,455,468,486,505,511,514,566,584,631,
663,672,689,724,740,758,781,799,820` — une trentaine ; `essai_s512` idem). Or
`essai_s7:840` annonce `PROV libres **16**` pour ~30 lignes `pays=-1`, et
`essai_s512:628` annonce `PROV libres 19` pour ~24 lignes. **Le dump PROV et le
compteur « PROV libres » ne comptent pas la même chose.** À trancher : l'un des
deux ment.

### A19 — Ordre des âges inversé (les deux bras)
`temoin_s3333:71-73` et `essai_s3333:70-72` : `an 2 L'Ère des Échanges` **avant**
`an 6 L'Âge des Découvertes`. Idem `s2026` (an 4 Découvertes, an 5 Échanges,
an 9 Soulèvements). Ce n'est pas un écart témoin/essai, mais une bizarrerie
narrative : les Échanges avant les Découvertes.

### A20 — Écarts témoin/essai difficiles à imputer aux doctrines
- **Ligue Karggoris (graine 7)** : `temoin_s7:884` `29 rég · pop 105 k · 63 tech`
  → `essai_s7:895` `**2 rég · pop 0 k · 1 tech**`. Un empire de premier plan
  réduit à néant. La graine 7 passe aussi de 6 à **16** « empires vivants ».
- **Havre Gualyan (graine 7)** : corruption 29 → **85** (`essai_s7:856`).
- **Entropie faustienne** : neutre en moyenne (−1,3 %) mais ré-battue par graine :
  s7 73 300 → 8 223 (÷ 8,9), s512 44 486 → 7 098 (÷ 6,3), s1009 8 635 → 35 632
  (× 4,1), s90 90 552 → **211 172** (× 2,3, avec `réplicateur 9 370` contre 0).
- **Manufactures privées** : s3333 480 → **114** (÷ 4,2) ; s777 5 021 → 2 719 ;
  s11 1 742 → 158 (÷ 11). Cette métrique est la plus volatile du corpus.

---

## 6. Propositions de calibrage — CHIFFRÉES

Rappel des formules réelles (lues dans le code, pas dans le design) :
`coût adoption = (DOCT_COST_BASE 50 + DOCT_COST_STEP 25 × n_actives) × ech` ·
`coût idée = (IDEA_COST_BASE 30 + IDEA_COST_STEP 3 × n_idées) × ech` ·
`ech = influence_base_gain / INFLUENCE_BASE_REF (2.0)`, planchée à **0,25** ·
gate d'achat : `influence ≥ coût × AI_DOCT_RESERVE (1.5)` ·
**un seul acte par passage**, cadence `AI_DOCT_CHECK_MONTHS (12)` arrondie en
années.

### À FAIRE

**F1 — Plusieurs actes par passage, plafonnés.**
*Constat* : 1 acte/an ⇒ 42 ans pour un arbre complet ⇒ les 6 slots sont pris à
l'an 1-10 et gelés 190 ans (§3.1, §3.5a). C'est la cause racine de 4 constats
sur 5 de ce rapport.
*Proposition* : boucler l'acte tant que la réserve tient, avec un plafond neuf
`AI_DOCT_ACTS_MAX = 3` (défaut 1 aujourd'hui, implicite).
*Effet attendu* : l'arbre se remplit au rythme du **budget** et non du
calendrier ; les grands empires vont plus vite, les petits restent bloqués — ce
qui est le comportement voulu. Coût CPU nul (le site est déjà annuel).
*Note* : F1 seul vide l'arbre encore plus vite ⇒ **F1 s'applique avec F4.**

**F2 — Ne pas ouvrir les 6 slots d'office ; les échelonner.**
*Constat* : `doctrines_slots_open` rend toujours `DOCT_SLOTS_MAX`. Conséquence
mesurée : Faustien **0/331** (code mort — les nœuds faustiens arrivent an 100+),
Technologie **2/331**, Connaissances **2/331**, Peuple **2/331**, Divin
**0/331** ; et les juges martiaux plafonnent à 62 % parce que la guerre arrive
après la saturation.
*Proposition* : `AI_DOCT_SLOTS_EARLY = 3` (3 slots ouverts d'emblée, les 3
autres à l'avènement des âges 4, 5 et 6 — an ~55, ~100 et ~181 dans ce corpus).
*Effet attendu* : ~3 slots par pays restent libres au moment où apparaissent les
guerres (Offense/Défense), les vassaux, les bibliothèques (Technologie), la
métabolisation (Connaissances) et les nœuds faustiens. Prédiction chiffrée :
Offense+Défense passeraient de 45 à ~75 adoptions, le juge martial de 62 % à
~80 %, et Faustien cesserait d'être du code mort.
*Réserve* : c'est un retour partiel sur la décision « les 6 slots ouverts
d'office » (brief §1.2) ⇒ à valider par le joueur, mais l'effet est démontré.

**F3 — Faustien : décider.**
0 adoption sur 331, sur 10 mondes, avec 14 à 51 nœuds faustiens par sim. Soit
on applique F2 (et il devient atteignable), soit on retire Faustien de la table
des scores IA. **Le garder tel quel, c'est garder du code que rien n'exerce.**

### À MESURER D'ABORD

**F4 — Remonter les marches de prix, en même temps que F1.**
*Constat* : `DOCT_COST_STEP 25` et `IDEA_COST_STEP 3` sont des marches douces —
le 6ᵉ slot coûte 175×ech, la 36ᵉ idée 135×ech. Aujourd'hui ce n'est pas le prix
qui freine, c'est la cadence (l'influence médiane à l'an 40 est **22,5**, soit
juste au-dessus du seuil : l'IA vit en flux tendu et dépense tout).
*Proposition* : `DOCT_COST_STEP 25 → 60` et `IDEA_COST_STEP 3 → 8`, à appliquer
**seulement avec F1**. Le 6ᵉ slot passe à 350×ech, la 36ᵉ idée à 310×ech.
*Effet attendu* : le budget redevient le frein ; les grands empires gardent un
arbre complet, les moyens s'arrêtent à 3-4 doctrines, les micro-États à 1.
*À mesurer* : que la saturation an 40 (aujourd'hui 58-81 %) descende vers
25-40 %, et que la médiane d'influence à l'an 160 cesse de dépasser le témoin.

**F5 — Le plancher d'échelle : 0,25 → 0,50.**
*Constat* : `influence_scale` plancher 0,25 ⇒ un micro-État paie l'arbre au
quart du tarif. Résultat : les 12 Populaire, les 2 Technologie, les 2
Connaissances, le 1 Bourgeoisie viennent **tous** de polities d'une région et
de 0-2 k habitants, et on voit `Ordre Brakrak influence 11.2 : Aristocratie(6
idées)` (essai s777:121) — un arbre entier pour un pays qui n'existe pas.
*Proposition* : plancher 0,25 → 0,50 (le prix double pour les nains).
*Effet attendu* : élimine la queue de distribution parasite ; les corrélations
« tous pays » se rapprochent des corrélations « adoptants ».
*À mesurer* : risque d'assécher totalement la queue (Populaire/Bourgeoisie
tomberaient à 0 comme Divin). Ne pas appliquer avant F2, qui les rendrait
atteignables par les vrais empires.

**F6 — `AI_DOCT_RESERVE` 1.5 → 2.5, APRÈS F1/F4.**
*Constat* : la réserve est aujourd'hui **inopérante** — l'IA est en permanence à
la limite (méd. an 40 : 22,5 pour un coût nominal de 50-175×ech), donc le
coussin ne protège rien. Le seul effet observable est A5 (adoption payée, idée
impayable).
*Effet attendu une fois F4 posé* : l'IA garde de quoi payer un envoyé
diplomatique (`INFLUENCE_COST_ENVOY 12`) au lieu de tout mettre dans l'arbre.

**F7 — Aplatir Production et Colonisation (le levier le plus risqué).**
*Constat chiffré* : Colonisation plafonne à **2,6** contre 2,0 pour Production
et Aristocratie — elle est mécaniquement servie la première. Production est la
seule strictement positive à l'an 1 (`rawcap/nprov`), d'où 62 adoptions et une
présence dans le TOP-3 de 55 pays sur 66.
*Proposition* : Production `aid_clamp(nbld×0,10, 0, 1.2) + aid_clamp(rawcap…,
0, 0.8)` → caps **0,8 et 0,5** (max 1,3) ; Colonisation « chantier en cours »
`+1.0` → **+0,6** (max 2,2).
*Effet attendu* : libère 1 à 2 slots par pays au profit de Commerce,
Mercantilisme et Vassaux.
*À mesurer impérativement* : ce levier touche le juge du design lui-même. Le
brief §1.3 dit que ces poids « ne sont pas des tunables » ; s'ils le deviennent,
il faut les inscrire au registre J avant de les bouger.

### DÉCISION JOUEUR

**J1 — Accepte-t-on la variance par graine ?**
L'arbre est neutre en moyenne (pop −1 %, guerres −3,5 %, âges identiques) mais
change la fin §27 dans **4 mondes sur 10** et fait varier M(fin) de −31 % à
+89 %. Le brief §3 dit « l'adoption IA change les trajectoires — c'est le but ».
Ma lecture : **oui, accepter** — les moyennes sont saines et la dispersion vient
de l'endgame §27 (toujours an 180, jamais un déclencheur nouveau), pas des
doctrines elles-mêmes.

**J2 — Veut-on que Bourgeoisie / Populaire / Divin existent chez l'IA ?**
Aujourd'hui : 1, 12, 0 adoptions sur 331. La cause est arithmétique et exacte
(§3.5f) : au départ 80/15/5, Aristo gagne de 30 % ; en régime 89/8/2, Populaire
gagnerait de 114 % — mais le slot est verrouillé. Trois voies, au choix :
(a) **ne rien faire** — assumer qu'un monde qui commence noble reste noble ;
(b) `INFLUENCE_PER_BOURGEOIS 0.0006 → 0.0010` — Bourgeois passerait à
1,50 · 10⁻⁴ au départ et gagnerait le courant ; effet secondaire : +67 % de
génération d'influence pour tout empire bourgeois, à re-mesurer ;
(c) **ne décider le courant qu'après l'an 40**, quand la structure de classe
s'est stabilisée — c'est la voie propre, mais elle demande un slot réservé
(donc F2).
Divin restera à 0 dans tous les cas tant que le monde démarre athée : sa base
`foi bâtie` vaut zéro au moment du choix. **C'est un fait de design, pas un bug.**

**J3 — Les anomalies A2/A4 (armées à 0 régiment, trésors négatifs) sont-elles
imputables aux doctrines ?** Elles n'apparaissent que dans le bras essai, et
uniquement dans les mondes qui fragmentent (s512, s3333). Il faut décider si on
les traite comme un bug de sécession (indépendant de la vague) ou comme un effet
de bord à corriger ici.

---

## 7. Verdict en 5 lignes

1. **Le kill-switch tient, les 20 sims sont propres** : rc=0, aucun ASSERT,
   aucun NaN, 6 âges partout, invariant à 130 % pour un seuil de 370 %.
2. **L'arbre est neutre sur les agrégats** (pop −1 %, guerres −3,5 %, âges
   inchangés) **et positif sur la répartition** : richesse du Laboureur +39 %,
   satisfaction +1,7 pt, prix −15 %, au détriment du trésor (−11 %) et de
   l'Élite (−8 %). C'est sain.
3. **Le vrai défaut n'est pas le score, c'est la cadence** : un acte par an et
   six slots ouverts d'office verrouillent l'arbre de chaque pays sur ses
   signaux de l'an 1-10 ; Production+Colonisation+Aristocratie raflent 46 % des
   adoptions, Faustien et Divin sont du code mort (0/331), Technologie,
   Connaissances et Peuple font 2 chacune.
4. **Les juges valident le choix par score** (côtiers 77 %, suzerains 69 %,
   belligérants 62 % chez les adoptants, contre 0 % au témoin) ; leur seule
   faiblesse — le juge martial — a la même cause : la guerre arrive après la
   saturation des slots.
5. **Deux corrections suffisent à tout débloquer** : échelonner les slots sur
   les âges (F2) et découpler le rythme du calendrier en remontant les prix
   (F1+F4). Le reste est de la mesure. Anomalies à trier hors vague : armées à
   0 régiment et trésors négatifs dans les mondes qui fragmentent (A2, A4),
   régiments hypertrophiés jusqu'à 342 (A3), et le désaccord entre le dump PROV
   et le compteur « PROV libres » (A18).

---

## Annexe 2026-09-03 (W2-4) — LE PROTOCOLE A CHANGÉ POUR LE SWEEP DE VALIDATION

Ce rapport lit le format de résumé **v1**. `tools/sweep_doct_ai.sh` en produit
désormais un **v2** — même protocole apparié (deux bras, mêmes graines, un seul
tunable de différence : `SCPS_TUNE=AI_DOCT=0` contre le défaut), même règle de
lecture (**le résumé est un index, l'analyste lit les `.log`**), mais deux lignes
par bras au lieu d'une :

1. `pays · guerres · M(fin) · indice · grain · doctrines · juges (ADOPTANTS)`
2. `rgt/limite · solde/revenu · désert · sur-budget · décroch · LEDGERS figées ·
   fidèles (porte)`

Trois changements de lecture, à ne pas confondre avec une dérive du monde :

- **La colonne « prix » de la v1 était l'`indice` (caisse/VA, M7-I1), pas un
  prix.** Elle garde son nom honnête (`indice`) et une VRAIE colonne de prix
  apparaît à côté : `grain`, la médiane du prix du grain sur les provinces
  tenues (base 1,00). Les valeurs « indice prix moy » du §2.1 ci-dessus sont donc
  des indices — les relire comme tels.
- **Les juges viennent maintenant de la ligne ADOPTANTS SEULS** (la vraie mesure
  du départage, 2026-09-02). La v1 mélangeait les deux lignes de corrélation.
- **Les colonnes W1 sont neuves** : elles mesurent le frein économique de la
  levée (désertions `WH_DESERT_RATE`, mois-pays sur-budget
  `WH_PAY_REVENUE_FRAC`), l'invariant des sièges (LEDGERS P11) et la porte du
  courant Divin (religion d'État fondée). Sans elles, le sweep de validation ne
  peut pas dire si la vague W1 a tenu.

Le chronicle gagne aussi une ligne `recoupement I0` : la Σ des postes du flux
décomposé contre le flux RÉELLEMENT mesuré au trésor national, sur le même
dénominateur — c'est elle qui dit si la télémétrie monétaire se recoupe.

Les anomalies A2/A3/A4 et A18 de ce rapport ont été traitées depuis (voir
TROUVAILLES.md, missions 2026-09-03) : le sweep de validation est ce qui doit le
confirmer sur 10 graines.
