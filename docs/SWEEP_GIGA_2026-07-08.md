# GIGA SWEEP — 20 graines × 10 sims × 250 ans (2026-07-08, moteur GELÉ à `8644065`)

**Protocole** : 200 mondes (graines 3·5·7·9·10·11·19·23·42·55·77·99·123·145·200·310·411·777·1000·2026,
10 sous-graines chacune, +101), 250 ans, binaire post-vague (lots A/B/E/M/P/T/U/V/W + calibrage
religion). Analyse : `tools/sweep_analyze.py` → `sweep_giga/sims.tsv` (matrice 200 × 40 métriques).
**200/200 EXIT 0 · zéro mécanisme à somme nulle · déterminisme/golden tenus.**

## 1. Distributions (200 sims — min · p25 · méd · moy · p75 · max)

| Métrique | Valeurs |
|---|---|
| Population finale | 92k · 320k · **446k** · 505k · 692k · 1 177k |
| Empires (genèse) | 2 · 4 · 7 · 6.5 · 9 · 11 |
| Guerres territoriales | 0 · 5 · **11** · 15.9 · 21 · 70 |
| Guerres de subjugation | 0 · 5 · **10** · 11.4 · 15 · 43 |
| Guerres religieuses | 0 · 0 · **1** · 3.5 · 5 · 26 |
| Guerres économiques | 0 · 0 · 1 · 1.6 · 2 · 25 |
| Batailles livrées | 7 · 148 · **233** · 283 · 405 · 799 |
| Soulèvements allumés | 0 · 1 · 4 · 8.7 · 13 · 51 (morts méd 227, max 5 174) |
| Sécessions / coups / renversements | méd 0 partout (max 5 / 6 / 1) — la révolte échoue « comme dans l'Histoire » |
| **Pillage réel (lot P)** | 3 · 32 · **57** · 85.5 · 107 · 408 pillages/sim |
| — prise or-équiv. | 60 · 111k · **324k** · 516k · 713k · 3.66M |
| — % de la cible 20 %·revenu | 4 · 35 · **47** · 48.7 · 59 · 100 (le « si possible » borne) |
| **Esclavage** | âmes serviles 0 · 0 · **22** · 117 · 188 · 816 ; déportées méd 81 (max 2 373) |
| Course pirate | bimodale : méd 0, moy 32 raids (max 322 · 2.77M or) — des mondes à pirates, des mondes sans |
| Marine | coques méd 656 · routes maritimes méd 23 (max 104) |
| Réfugiés | fuites méd 28 → retours méd 224 (la pop RESPIRE, ratio ~8:1) |
| Hégémon (taille) | méd 31 rég · stab plancher méd 4 |
| IPM final | 1.01 · 1.16 · **1.21** · 1.20 · 1.25 · 1.32 — l'inflation bornée |
| Arbre tech | 16 · 23 · **26 %** · 26.7 · 30 · 57 % /empire à 250 ans |
| Métabolisation max | 0.9 · 4.6 · **10 %** · 16.4 · 28.3 · 80 % |
| Recherche (nœuds monde) | méd 192 (38–481) |

## 2. Fins §27 — LE constat d'équilibre

| Fin | Sims | Fenêtre |
|---|---|---|
| **GRAND HIVER** | **97** (67 % des fins) | an 180–248 |
| (aucune en 250 ans) | 56 (28 %) | — |
| RONCES | 29 | 180–246 |
| ENGLOUTISSEMENT | 17 | 180–248 |
| SANG | **1** | 204 |

- ⚠ **Le FROID écrase 4:1** : le sélecteur de fin (compteur de rare dominant, signature du fauteur
  par défaut) penche structurellement HIVER. Si on veut des fins variées : curseur à toucher
  (post-gel, décision joueur).
- Le gate an-180 TIENT (aucune fin < 180). 28 % de mondes sans fin à 250 ans — plutôt les mondes
  calmes ; à trancher : feature (la fin n'est pas garantie) ou pas.
- **SANG 1/200** : seed 5 sim 2 (3 empires, 23 guerres, 561 batailles) — la fin-carnage exige un
  monde exceptionnellement sanglant. En partie JOUEUR (guerres concentrées sur lui) elle sortira
  plus souvent — c'est sa conception (« la fin signe TON règne »).
- Curiosité clinique : l'archétype **monde froid** a le plus de sans-fin (10/24) — un monde déjà
  froid nourrit moins le compteur de refroidissement.

## 3. Hégémon — la fragilité mord

**CRAQUÉ 181/200 (90.5 %)**. Les 19 sims où l'hégémon TIENT : petits mondes (2 empires méd,
hégémon 17 rég méd — des duels sans pression périphérique). Aucun archétype n'immunise.

## 4. Religion (post-calibrage, cap ⌈N/2⌉, fondation au Temple T2)

Par graine (fois fondées/sim · schismes/sim) : 2026 **2.0**·0.8 → 3 2.0 · 19 1.9 · 145/55/23 1.4 ·
11/10/9/5 1.3 · 777 1.2 · 1000/42 1.1 · 99/7 0.9 · 200/77 0.8 · 123 0.7 · **411/310 0.6**.
**Moyenne ≈ 1.2 foi/sim** (contre 0.3 avant calibrage — ×4) ; pays fidèles jusqu'à 2.4/sim ;
guerres religieuses méd 1 (max 26) — la foi est redevenue une force géopolitique. Sous le cap (~4) :
la rareté restante = nombre de prosélytes par monde + Temple 540 j + tier-2 en milieu de partie —
la conception choisie (fondation TARDIVE et signifiante), pas un verrou.

## 5. Archétypes — 8 profils de partie mesurables

| Archétype | n | pop méd | guerres méd | craqué | fins |
|---|---|---|---|---|---|
| jeune-montagneux | 39 | 425k | 13 | 90 % | 28 (dont LA sim SANG) |
| pangée | 26 | 360k | 10 | 81 % | 19 |
| vieux-érodé | 25 | **752k** | 7 | 100 % | 19 |
| monde aride | 24 | 471k | 11 | 96 % | 21 |
| monde froid | 24 | 492k | **15** | 92 % | 14 |
| mer intérieure | 23 | 444k | 9 | 87 % | 17 |
| continents | 21 | 406k | 9 | 81 % | 14 |
| archipel | 18 | 584k | 8 | 100 % | 12 |

Riche = vieux-érodé/archipel · guerrier = froid/montagneux/aride · calme = continents/pangée.
La variété demandée est MESURABLE, pas seulement visuelle.

## 6. Verdicts & curseurs post-gel (données en main, décisions joueur)

1. **Équilibre des fins** : HIVER 4:1 — rééquilibrer le sélecteur si variété voulue.
2. **56 sans-fin** : assumer ou pousser (entropie de base ↑ sur mondes calmes).
3. **Religion** : vivante (×4) ; pour viser le cap, élargir les prosélytes ou dialer
   `AI_FAITH_ZEAL` — optionnel.
4. **Course pirate bimodale** : méd 0 — si on veut des pirates partout, le nid d'eaux mortes est
   le levier ; sinon c'est une saveur de monde.
5. **Sécessions quasi nulles** (méd 0) : Phase 3a assumée (« la plupart des révoltes échouent ») —
   mais 0.14/sim est PLUS rare que l'intention « rare mais réel » (~1/15 victoires rebelles) ;
   à re-mesurer côté victoires rebelles si souhaité.
6. **Arbre 26 % à 250 ans** : cohérent avec √N + gate de palier — le tier-2 est un événement de
   milieu de partie, le tier-4/5 une fin de partie. Conforme à la vision.
