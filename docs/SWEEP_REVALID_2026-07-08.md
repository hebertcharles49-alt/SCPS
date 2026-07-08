# SWEEP DE RE-VALIDATION — 20 graines × 10 sims × 250 ans (2026-07-08, post-lots G/F/I)

**Protocole** : mêmes 200 mondes que le giga sweep de référence (`SWEEP_GIGA_2026-07-08.md`),
binaire post-vague calibrage (lots G flux humains · F fins/catastrophes/exode · I moteurs
d'innovation). **200/200 EXIT 0.** ⚠ Ce sweep précède la vague VOLUMES DE POP (`cad6775`,
télémétrie en âmes + magnitudes ×3-5) — ses chiffres migration sont donc DÉJÀ périmés à la
baisse ; la mesure appariée du commit fait foi pour ce lot (pacte 8-29k âmes/sim, fuites
1.4-19k). Analyse : `tools/sweep_analyze.py sweep_revalid`.

## 1. Fins §27 — le rééquilibrage F a mordu, HIVER reste en tête

| Fin | Giga (avant) | Re-valid (après) | Δ |
|---|---|---|---|
| GRAND HIVER | 97 (67 % des fins) | **90 (58 % des fins)** | −7 |
| RONCES | 29 | **41** | +41 % |
| ENGLOUTISSEMENT | 17 | **25** | +47 % |
| SANG | 1 | 0 | — |
| (aucune) | 56 | **44** | −21 % |

- Le dispatch par défaut (hash avalanche + poids diégétiques) et les catastrophes des mondes
  calmes fonctionnent : RONCES/EAU montent, les sans-fin baissent.
- **HIVER domine toujours par le chemin LÉGITIME** : la corne est le seul transmuteur vivant
  (`conso_corne` méd 7 · moy 634 · max 9631) et consomme du **fer céleste** ⇒ compteur FROID.
  La **foreuse est morte partout** (`conso_foreuse` = 0 sur 200 sims) ⇒ le compteur essence
  (EAU) ne se nourrit jamais par transmuteur. Curseur restant (décision joueur) : diversifier
  la conso de la corne ou la lecture des compteurs par la fin.
- SANG 0/200 : conforme — fin joueur-centrique, la chronique sans joueur ne la produit
  qu'exceptionnellement (1/200 au giga).

## 2. Moteurs d'innovation (lot I) — mieux, cible non atteinte

| Métrique | Giga | Re-valid |
|---|---|---|
| Arbre tech méd/moy/max | 26 % / 26.7 / 57 | **28 % / 28.4 / 61** |
| Recherche (nœuds monde) méd | 192 | **216** |
| Métabolisation max méd | 10 % | **28.7 %** |

L'arbre gagne ~2 pts (ethos-gating + grain + SOIF DE SAVOIR) — la cible 35-45 % n'est pas
atteinte. Le levier identifié et non tiré : prioriser la chaîne Scriptorium→Académie→Université
plus TÔT/plus FORT (le yield compose sur 250 ans — chaque décennie d'avance paie). La
métabolisation ×3 est l'effet croisé G (plus de diaspora à digérer).

## 3. Flux humains (lot G — chiffres PRÉ-volume, périmés à la baisse)

| Métrique (méd) | Giga | Re-valid |
|---|---|---|
| Fuites de réfugiés | 28 | **51** |
| Retours au foyer | 224 | **929** |
| Flux de pacte migratoire | 17 | **161** |
| Âmes déportées | 81 | **130** |
| Âmes serviles | 22 | **46** |

Tout monte ×2-8. La vague VOLUMES (`cad6775`) multiplie encore par ~3-5 les âmes par
mouvement — mesure appariée : 5-15 % de la pop mondiale bouge en cumul, l'afflux déstabilise
(soulèvements chez l'hôte) sans effondrement.

## 4. Effets systémiques du monde plus vif

- **Guerres** : territoriales méd 11→19, religieuses méd 1→**4** (la foi ×4 du calibrage
  religion se paie en conflits — voulu). Batailles méd 233→288.
- **Soulèvements** : méd 4→13, morts méd 227→812 — plus turbulent, borné (pas de spirale ;
  max 5109 ≈ max giga). Sécessions moy 0.41/sim (giga ~0.14) — « rare mais réel » respire.
- **Hégémon craqué** : 181/200 → 173/200 (86.5 %) — stable.
- **IPM** 1.20 méd (inchangé) · pop méd 446k→468k · colonisation méd 186 fondations/sim.
- **Commerce** : volume marché méd 1884 · hubs 82 % du commerce mondial · pool moy 144/mois —
  cohérent avec le giga (le calibrage n'a pas déstabilisé l'éco).

## 5. Restes ouverts (décisions joueur)

1. **HIVER structurel** : foreuse morte (0/200) + corne=fer céleste ⇒ le seul flux de rare
   vivant pointe FROID. Diversifier (conso corne mixte, ou réveiller la foreuse) si l'on veut
   ≈33/33/33 aussi sur le chemin transmuteur.
2. **Arbre 28 % < 35-45 %** : pousser la chaîne savoir plus tôt (levier identifié, lot I).
3. **SANG** : n'existe qu'en partie joueur — assumé.
