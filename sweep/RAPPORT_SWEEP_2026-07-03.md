# RAPPORT — GIGA SWEEP CHRONICLE (2026-07-03)

## Protocole

- **14 runs · 66 sims · rc = 0 partout** (aucun crash, aucune assertion, aucun NaN visible).
- 12 graines (3, 5, 7, 9, 11, 19, 42, 77, 99, 123, 145, 777) × 5 sims × 250 ans,
  tailles de monde VARIÉES par sim (2→6 empires — le défaut chronicle `2+(k%10)`),
  + 2 runs « grand monde » (graines 9 et 42, 10 empires / 16 cités-états, 3 sims × 250 ans).
- Données : `sweep/logs/*.log` (télémétrie brute), `sweep/sims.csv` (66 lignes),
  `sweep/seeds.csv` (14 lignes), parseur `sweep/parse_sweep.ps1`.
- Durées : ~45-55 s par run standard, ~150 s par grand monde (scaling ≈ linéaire en régions — sain).

## Ce qui TIENT (le socle est bon)

| Axe | Constat |
|---|---|
| Robustesse | 66/66 sims finissent, exit 0, pas de dérive numérique visible (trésors bornés, pas de -1e31) |
| Population | croissance saine partout, aucun effondrement ; grands mondes 338-553k an 200 |
| §27 endgame | **le cliquet an-180 tient STRICTEMENT** (min observé = 180) ; les 3 fins vivent (Ronces 18 · Engloutissement 20 · Grand Hiver 21) ; 59/66 sims concluent |
| Satisfaction | L 70-85 · B 78-88 · É 75-86 — pas un seul monde malheureux chronique |
| Guerre & politique | 13-33 guerres/sim, coups 0-49/graine, soulèvements vivants, sécessions 0-5, absorptions 10-20 — la carte respire |
| Combos tier-4 | vivants partout (1-98/sim) ; remise diffusion active (max −40 %) |
| Annexion par digestion | rare et vivante (0-8/graine) — conforme au design |
| Grands mondes | stables, hégémon mortel 3/3 et 3/3, pas d'explosion de coût |

## LES TROUS (par gravité)

### T1 — La MÉTABOLISATION est MORTE : 0/66 sims
`métabolisation : 0/N empire(s) creuset (>1 % digéré) · moyenne 0.0 % · max 0.0 %` sur les
**66 sims**. Avant la re-key province : moyenne 4-8 %, max **+48 %** de recherche.
Le canal ACTIF de la triade tech (déverrouille/accélère/escompte par digestion des
nouveaux venus) est éteint — l'accès aux signatures d'héritage ne passe plus que par
le commerce. **Piste** : `econ_country_metabolized` somme la diaspora (`g->integration`
des groupes migrants/captifs) — la re-key province a probablement cassé la POSE des
groupes diaspora (migrants posés à integration=0 qui ne montent plus, ou plus posés
du tout au grain province). À vérifier : `demography` → création de groupes DIASPORA
et leur tick d'intégration post-re-key.

### T2 — L'IPM est FIGÉ à 1.34 : 66/66 sims identiques
`IPM final 1.34 · pic 1.34` À LA DÉCIMALE PRÈS sur des mondes de 2 à 10 empires,
toutes graines. Un indicateur qui ne varie plus ne mesure plus rien : l'inflation
« sature » à une constante structurelle très tôt et n'évolue plus (avant : 1.21-1.31,
variable par monde). **Piste** : le panier/le numéraire de l'IPM lit des prix
région-keyés devenus uniformes (prix national P1 projeté ?) ou un dénominateur figé
à la genèse. Voir `credit`/`prosperity` là où l'IPM est calculé.

### T3 — La RELIGION ne naît presque plus : 0.2 foi/sim (calibré : 2.0)
0 à 0.6 fondée/sim, schismes 0-0.4 (14 graines). Le monde est athée pour de bon.
La chaîne « zèle proactif → PREMIER sanctuaire → fondation » ne s'amorce plus.
**Piste** : le scan `edi_built` (union région) est toujours là — c'est l'AMONT qui
ne bâtit plus le sanctuaire : la voie `agency_build`/§NF re-keyée province ne pose
plus d'édifice religieux pour les empires IA (gate matière ? file ? le `w_faith`
n'atteint plus le seuil ?). Reproduire avec `SCPS_GATEDIAG` sur un empire évangéliste.

### T4 — Les HUBS de cités-états sont à 0 % : 14/14 runs
`hubs des cités-états : 0 % du commerce mondial` partout, et le bloc « commerce
asym. » est à 0.00 partout (aval/amont/vrac/précieux). Le pilier #5 (« les
cités-états TIENNENT le marché mondial ») ne se mesure plus — soit les Centres
intertrade ne sont plus semés/alimentés post-re-key, soit `intertrade_centre_value(r)`
lit un index région qui ne correspond plus au store réel. Le commerce lui-même VIT
(`commerce/an moy 388`, routes terrestres ouvertes) : c'est l'étage CENTRES/MARCHÉ
MONDIAL qui est déconnecté. **Piste** : `intertrade_seed_centres` + le remplissage
des stocks de Centre au grain province.

### T5 — Des BATAILLES FANTÔMES : 22/66 sims à 0 mort malgré des centaines de batailles
Extrêmes : 714 batailles → 0 mort (77-5), 622 → 100 (777-1), 204/237 → 0 (9-1, 9-2).
Des armées de campagne (quasi) VIDES se battent en boucle : le choc calcule des
pertes ∝ `force_units` ≈ 0, la déroute poursuit 8 % de ~0 paquet. À l'inverse la
variance est folle : 0 à **1454** batailles/sim, et quand ça mord, le CHOC domine
la poursuite (137k vs 20k — la télémétrie elle-même dit que la poursuite DEVRAIT
dominer). **Piste** : la mobilisation warhost → `campaign_order` au grain province
staffe mal les FieldArmy (compositions vides), et le seuil d'engagement laisse des
armées squelettes s'accrocher sans fin.

### T6 — Les hameaux libres se rallient EN BOUCLE : ~23× les semés
`4.8 semés/sim · 111 ralliés/sim (pop moy 2816)` (seed 11 ; même ordre partout —
256 à 607 ralliés par graine). Avant : ~4 ralliés/sim pour ~4 semés. Un hameau ne
peut se rallier qu'UNE fois : soit le compteur compte des provinces/régions au lieu
d'entités, soit les hameaux re-spawnent/re-rallient après sécession. **Piste** :
le tick de ralliement WILD post-re-key (compte-t-il par région membre ?) et la
télémétrie `wild` de chronicle.

### T7 — Le MARITIME est retombé : routes > 0 dans 13/66 sims seulement
567 coques bâties, 39 traversées… et **2 routes maritimes** (seed 7, qui en tenait
6-16 après V3). La marine navigue, le commerce de mer ne S'OUVRE plus. **Piste** :
`routes_order` maritime (deux ports + deux marchés + pacte) — la condition
« marché » dépend des Centres (T4) : T7 est probablement un SYMPTÔME de T4.

### T8 — Les MICRO-MONDES gèlent : duopoles sans histoire
Sims à 2 empires : top empire à 3-17 régions après 200 ans, hégémon jamais craqué
(seed 3 : 0/5), 7/66 sims sans fin §27 à 250 ans (toutes des mondes 2-3 empires —
l'entropie ne charge pas). Un monde à 2 empires peut rester duopole figé 250 ans.
C'est le défaut chronicle (2+(k%10)) qui échantillonne ces tailles ; en jeu, le
slider « tiny » produira ces parties plates. **Piste** : soit assumer (tiny =
bac à sable), soit un plancher d'agressivité/entropie pour N<4 empires.

### Notes mineures
- `provinces transférées à la paix` : 285-477 à l'an 200 — au grain PROVINCE
  (≈4 provinces/région), c'est ~70-120 équivalents-régions : élevé mais plausible
  avec 20-30 guerres/sim ; à surveiller, pas un trou avéré.
- `1er empire 0 rég` dans l'affichage A5 sous 10 régions : artefact de seuil
  (le compteur ne s'arme qu'à ≥10) — cosmétique télémétrie.

## Recommandation d'ordre (quand tu t'y remettras)

1. **T4 Centres/intertrade** (racine probable de T7, et le pilier économique #5) ;
2. **T1 métabolisation** (toute la triade tech-héritage repose dessus) ;
3. **T5 batailles fantômes** (la guerre est le cœur visible du plateau) ;
4. **T3 religion** (système entier en sommeil) ;
5. **T6 WILD** (compteur ou boucle — diagnostic rapide) ;
6. **T2 IPM** (indicateur mort — recâbler la mesure) ;
7. **T8 micro-mondes** (décision de design plus que bug).

Tous les trous T1-T7 pointent vers la même époque : la **re-key province-economy**
(les systèmes qui lisaient le grain RÉGION n'ont pas tous suivi). Le socle
(déterminisme, pop, endgame, guerre vivante, satisfaction) a survécu — ce sont les
étages TRANSVERSAUX (intertrade, diaspora, religion, staffing de campagne) qui ont
perdu leur branchement.
