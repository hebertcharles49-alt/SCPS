# RAPPORT — L'économie des biens et de la satisfaction (lecture M10)

**Méthode** : lecture pure de code, aucun build/run (une mesure diagnostique tournait en
parallèle sur `chronicle.exe` pendant cette mission — voir `TROUVAILLES.md`, entrée
« DIAG-BANQUEROUTES »). Dépôt `C:\Users\Charl\Desktop\SCPS-main`, HEAD `42373dc` (M9 livré,
SAVE_VERSION 94). Cœur : `scps/scps_econ.c` (5834 lignes) ; lectures croisées dans
`scps_labor.c`, `scps_demography.c`, `scps_prosperity.c`, `scps_legitimacy.c`, `scps_revolt.c`,
`scps_ai.c`, `scps_events.c`, `scps_api.c`, `scps_types.h`, `scps_econ.h`.

Chaque affirmation est sourcée `fichier:ligne`. Là où le code seul ne permet pas de trancher
un chiffre exact (ça arrive une fois, §A.2), c'est dit explicitement — cette mission n'a lancé
aucun binaire pour vérifier.

---

## A. LA SATISFACTION

### A.1 — La formule complète, terme par terme

`PopStratum.satisfaction` (`scps_econ.h:86-90`, commentaire : « [0..1] : fraction des besoins
couverts au dernier tick ») est recalculée **par classe, par province, chaque tick**, à la fin
de la boucle marché/consommation (`econ_tick`, `scps_econ.c:4277-4278`) :

```
satisfaction = clampf( basket + comfort_joy − over_tax[c]·K_TAX_AGIT − annex_scar·ANNEX_SAT_W,
                        0.f, 1.f )
```

- **`basket`** (`scps_econ.c:4273`) = `met_w/need_w` si `need_w>0`, sinon `0.5` par défaut.
  `need_w`/`met_w` sont des sommes pondérées par **valeur** (`BASE_PRICE[r]*need`) accumulées
  UNIQUEMENT sur les besoins **actifs** de la classe (`need_rank(c,r) < active_needs`,
  `scps_econ.c:4134` — un besoin verrouillé ne pèse ni au numérateur ni au dénominateur). C'est
  une fraction **continue** (pas un seuil binaire).
- **`comfort_joy`** (`scps_econ.c:4164`) = `+COMFORT_JOY(0.08 défaut)` par bien de confort
  (poterie/statuaire) SERVI — un bonus **hors panier** : aucune pénalité s'il est absent
  (`scps_econ.c:4154-4166`).
- **`over_tax[c]`** (`scps_econ.c:3882`, agrégé Laborer à `re->over_tax` ligne `3884`) = la
  **grogne fiscale** : `max(0, ambition − seuil)` où `ambition = STATE_TAX_AMBITION(0.42)
  × tax_mult[cid][c]` et `seuil = tolerance(éthos,classe) × (0.40+0.60·satisfaction_du_tick_
  PRÉCÉDENT) × debase_factor × satisfaction_tax_factor` (`scps_econ.c:3846-3852`). Poids
  `K_TAX_AGIT = 0.85` (`scps_econ.c:2321`).
- **`annex_scar`** (cicatrice d'annexion, étage 3d) × `ANNEX_SAT_W` (0.5 défaut) — pénalité
  décroissante après une conquête (`scps_econ.c:4278`, décroît `scps_econ.c:4364`).

**Ordre d'application dans `econ_tick`** : §1 extraction → §2 manufacture → §3 « l'État
ACHÈTE » la VA produite (salaire/profit/rente crédités, `scps_econ.c:3771-3822`) → §3b impôt
d'État + calcul d'`over_tax` (`scps_econ.c:3824-3884`, donc **avant** la satisfaction du même
tick, mais lisant la satisfaction du tick **précédent**) → §besoins progressifs (`active_needs`,
`scps_econ.c:4032-4038`) → §4 demande → §5 marché (prix) puis **satisfaction par strate**
(`scps_econ.c:4091-4282`, « l'État REVEND », crédite le trésor) → §6 mise à jour (démographie,
tech, agrégat région).

**Il existe une SECONDE quantité, distincte, qu'il ne faut pas confondre avec la
satisfaction** : `re->needs_met` (« besoins comblés », `scps_econ.c:4283`) = moyenne pop-
pondérée sur les classes de `nsat/nbasket`, où :
- `nbasket` (`scps_econ.c:4129-4130`) = nombre d'entrées `NEED[c][*]>0` de la classe, **hors**
  poterie/statuaire, **indépendamment du tier débloqué** (le panier COMPLET, mature).
- `nsat` (`scps_econ.c:4145,4188,4211,4220,4254`) = nombre de ces entrées **actives**
  (`need_rank<active_needs`) dont la couverture `got≥τ` (`NEEDS_MET_TAU=0.5`, `scps_econ.c:
  4100`) — un seuil **binaire**, pas continu.

`needs_met` n'entre **jamais** dans la formule de satisfaction — il pilote la fertilité
(`scps_econ.c:4283` commentaire, lu `scps_econ.c:4347`). Les deux métriques répondent à des
questions différentes (« combien du panier MATURE est-il couvert » vs « à quel point cette
classe est-elle contente CE tick, biens + fisc + guerre »directamente) et n'ont **structurellement
aucune raison de bouger ensemble** — c'est le cœur de la réponse à A.2.

Enfin, l'agrégat région `re->satisfaction` (lu par la croissance, la légitimité, la pression
fiscale — cf. A.3) est une **troisième** quantité : moyenne pop-pondérée des `satisfaction`
par classe (`scps_econ.c:4412-4414`), puis réduite par la pénalité off-culture
(`×(1−0.45·fraction_hors_culture)`, `scps_econ.c:4417`) et par la dissidence religieuse
minoritaire (`−RELIG_MINORITY_SAT=0.15`, `scps_econ.c:4419-4427`).

### A.2 — Le cas early (an 0, trace M8DIAG) : besoins comblés 15 % / satisfaction Laborer 0 %

Le diagnostic cité dans le brief (`chronicle.c:919-932`, `SCPS_M8DIAG`) imprime, pour le pays
le plus peuplé, `pe->needs_met` de la **capitale** et `econ_country_class_satisfaction(...,
CLASS_LABORER)` — c'est-à-dire deux métriques qui ne sont **ni la même formule ni forcément le
même périmètre** (needs_met = province capitale seule ; satisfaction Laborer = moyenne
pop-pondérée pays entier — mais à l'an 0, une seule province est colonisée par pays, cf.
plus bas, donc le périmètre coïncide en pratique).

**Ce qui est prouvé par le code (constantes de genèse)** :
- Un empire jouable/IA naît avec `EMPIRE_SEED = 4000` habitants **sur une seule province**
  (`scps_econ.c:1807-1818` — « TOUTES les autres provinces… restent owner=-1/non-colonisées »,
  ligne 1813).
- `capitale_max_tier(4000)` = **4** (`scps_labor.c:34-51` : seuils T2=2000, T3=3000, T4=4000,
  T5=5000, T6=8000, T7=10000) → `active_needs = 1+4 = 5` **dès la genèse**
  (`scps_econ.c:4038`).
- Pour le Laborer (`NEED_ORDER[LABORER] = {GRAIN, EAU_DE_VIE, FISH, WOOD, TUNIQUE, POTTERY}`,
  `scps_econ.c:582`), `active_needs=5` débloque déjà **les 5 rangs 0-4** — soit la TOTALITÉ du
  `nbasket` Laborer (5, poterie exclue). Même chose pour l'Élite (5/5). Le Bourgeois est à 5/6
  (papier, rang 5, encore verrouillé).

**Conclusion n°1, contre-intuitive** : à la genèse d'un empire joueur/IA, le verrouillage par
tier (`active_needs`) n'est **PAS** le facteur limitant — la quasi-totalité du panier mature
est déjà débloquée dès le premier tick (parce qu'`EMPIRE_SEED=4000` franchit déjà le seuil
T4). Le verrouillage par tier ne mord fort que pour une cité-état (`CITY_SEED=2000`, pile T2,
`active_needs=3`) ou une colonie fraîchement fondée à plus petite population.

**Ce qui explique le « 15 % »** : à la genèse, une seule province est colonisée, le tirage
brut d'une tuile est **borné à ≤2 ressources** (« RÈGLE ≤2 RAWS STRICTE », `scps_events.c:
909-920`), et **aucune manufacture n'existe encore** (le parc de bâtiments se construit via
`econ_build_tick`, dans le temps). Sur les 5 besoins actifs du Laborer, seul RES_GRAIN
bénéficie d'un canal garanti indépendant du stock/richesse construits : la « ration vitale »
(M5, `scps_econ.c:4141-4152`) le sert via `can_stock` SEUL (jamais gaté par le budget) — mais
même lui exige `S[RES_GRAIN]>0`. Les 4 autres (EAU_DE_VIE, FISH, WOOD, TUNIQUE) dépendent soit
d'un tirage brut absent (FISH/WOOD hors du tirage ≤2 de la capitale), soit d'une manufacture
pas encore bâtie (EAU_DE_VIE via `BLD_DISTILLERY`/`BLD_BREWERY`, TUNIQUE via une chaîne à DEUX
étages `BLD_TEXTILE`→`BLD_TUNIC`). Si seul GRAIN atteint `got≥τ`, `nsat=1` sur `nbasket=5` →
`needs_met≈20%` — de l'ordre du 15 % observé (l'écart tient à la pondération pop entre
classes, Bourgeois/Élite ayant leurs propres nbasket/nsat).

**Ce qui explique le « 0 % » de satisfaction (reconstruction PLAUSIBLE, pas confirmée par un
run)** : contrairement à `needs_met`, `satisfaction` **soustrait** deux termes sans plancher :
- `basket` lui-même est probablement bas mais pas nécessairement nul : c'est une fraction
  **pondérée par valeur**, pas un compte d'items — si GRAIN (poids `BASE_PRICE[GRAIN]×3.5`)
  ne représente qu'une fraction du `need_w` total des 5 items actifs, un service parfait du
  seul GRAIN peut donner un `basket` petit (quelques % à ~20 %) sans être strictement 0.
- `over_tax` est **déjà vivant au tick 1** : `§3b` lit la satisfaction de fin de tick
  PRÉCÉDENT — qui vaut `0.5` par construction (`econ_seed_population`, `scps_econ.c:1037` :
  `satisfaction=0.5f` à la genèse, PAS 0). `seuil = tolerance(éthos,Laborer)×0.7` (avec
  `sat=0.5`) : la table `tolerance` (`scps_econ.c:2035-2043`) vaut 0.38-0.60 selon l'éthos,
  et `STATE_TAX_AMBITION=0.42` (`scps_econ.c:2320`) — pour la plupart des éthos (hors
  Dominateur), `ambition(0.42) > seuil`, donc `over_tax` est déjà **positif dès le premier
  tick** (de l'ordre de 0.03 à 0.15 selon éthos), soustrayant `×K_TAX_AGIT(0.85)` jusqu'à
  ~0.13 point à un `basket` déjà mince.
- Le `clampf(...,0,1)` n'a pas de plancher protégé pour ces deux termes soustractifs : un
  `basket` faible + une grogne fiscale déjà active suffit à toucher **0 littéral**, alors que
  `needs_met` (aucun terme soustractif, purement `nsat/nbasket≥0`) ne peut structurellement
  pas descendre sous ce que la couverture brute permet.

**Honnêteté requise par le brief** : le partage exact du delta (combien vient du `basket`
faible vs de `over_tax` vs du plancher `clampf`) **ne peut pas être fixé au chiffre près par
la seule lecture du code** — cela demanderait un run instrumenté (type `SCPS_M8DIAG` étendu à
imprimer `basket`/`comfort_joy`/`over_tax` séparément), explicitement hors du périmètre
« lecture seule » de cette mission. Ce qui EST prouvé par le code, et suffit à répondre à la
question posée (« si l'early est dominé par autre chose que les besoins, tiérer les besoins ne
suffira pas ») : **la satisfaction et needs_met à l'an 0 sont dominées par des mécanismes
DIFFÉRENTS** — needs_met par la capacité de production non encore montée (manufactures
inexistantes, tirage brut borné, colonie unique), satisfaction en PLUS par un terme fiscal
(`over_tax`) qui n'a rien à voir avec les biens et qui est actif dès le tick 1 sur la base
d'un défaut de genèse (`satisfaction=0.5`), pas sur un vrai historique de contentement.

### A.3 — Qui LIT (consomme) la satisfaction : liste exhaustive

| # | Lecteur | Fichier:ligne | Ce qu'il en fait |
|---|---|---|---|
| 1 | Seuil de tolérance fiscale (§3b + 3 autres sites de calcul du même seuil) | `scps_econ.c:2079-2085` (`econ_satisfaction_tax_factor`), `2358-2361`, `2394-2397`, `2430-2433`, `3846-3849` | Module le seuil d'évasion fiscale — un peuple content absorbe plus d'impôt |
| 2 | `econ_ai_fiscal_tick` (M8 C3, contrôleur IA) | `scps_econ.c:2148-2226` | Ajuste `tax_mult[cid][c]` pour viser `AI_FISCAL_TARGET=0.60` |
| 3 | `econ_country_class_satisfaction` (lecteur agrégé pays) | `scps_econ.c:2133-2146` | Sert le C3 ci-dessus ET la façade UI-monnaie `scps_country_fiscal_orders` (`scps_api.c`) |
| 4 | Tech des élites | `scps_econ.c:4435` | `tech += wealth×TECH_RATE×satisfaction×...` — une élite mécontente ne convertit pas sa richesse en savoir |
| 5a | Fertilité / croissance (terme DOMINANT) | `scps_econ.c:4347` | `bonus += POP_NEEDS_W(0.85)×needs_met` — **pas** `satisfaction` |
| 5b | Fertilité / croissance (terme MINEUR, asymétrique) | `scps_econ.c:4351` | `bonus += POP_SAT_W(0.20)×max(0, satisfaction−0.5)` — un peuple mécontent (< 50 %) ne perd RIEN, seul le surplus au-dessus de 50 % prime la croissance |
| 6 | Promotion de classe | `scps_econ.c:3072-3074` (`PROMOTE_SAT_GATE=0.50`), `3182` | Aucune promotion VERS une strate dont la satisfaction < 50 % |
| 7 | Démotion de classe | `scps_econ.c:3076` (`g_lowsat_streak`), `3197` | Satisfaction < 30 % DEUX mois consécutifs → démotion |
| 8 | Légitimité par groupe démographique | `scps_demography.c:139` (`aisance=satisfaction×10`), `145-150`, `877` | Entrée de `group_L_target` (légitimité L d'un groupe pop) |
| 9 | Pression fiscale/politique (modèle Prospérité/PE) | `scps_prosperity.c:133-147` (`econ_fiscal_pressure`) | `unmet=(1−satisfaction)` → charge politique I |
| 10 | Légitimité (module séparé) | `scps_legitimacy.c:64-66` | Réimplémente le MÊME idiome `aisance=satisfaction×10` — deux modules distincts, même formule, non factorisée |
| 11 | IA — priorisation d'allocation | `scps_ai.c:922` | Cible la province à pire satisfaction (hors forte coercition) pour ses décisions |
| 12 | Révolte — **ÉCRITURE**, pas lecture | `scps_revolt.c:917` (+0.15, concession religieuse), `950` (+0.20, concession de classe) | Une concession aux rebelles **bombe directement** la satisfaction — court-circuite entièrement la chaîne biens→satisfaction |

**Deux non-lecteurs notables** (le brief en évoquait la possibilité — infirmé par le code) :
- **Migration interne** (`econ_migrate_tick`, `scps_econ.c:5591-5616`) lit `re->prosperity`
  (différentiel `pros_dst/pros_src`), **PAS** `satisfaction`.
- **Factions** : règle de conception explicite, « AUCUN âge ne donne de satisfaction »
  (`scps_factions.c:318-329`, commentaire de suppression de l'ancien mécanisme).
- **Révolte — l'IGNITION** (pas la résolution) : `revolt_ignite` est appelé avec `re->over_tax`
  (`scps_revolt.c:566`), pas `re->satisfaction` directement — la grogne fiscale, un terme
  INTERNE à la formule de satisfaction, sert de déclencheur, mais la satisfaction globale non.

---

## B. LES BIENS

### B.4 — Le catalogue complet

`Resource` (`scps_types.h:148-218`) compte **55 entrées** : `RES_NONE` + **25 brutes** + **29
manufacturées**.

**Brutes (25)** — rendement `EXTRACT_YIELD[r]` en unités/ouvrier/an (`scps_econ.c:384-406`) :

| Ressource | Rendement | Ressource | Rendement | Ressource | Rendement |
|---|---|---|---|---|---|
| GRAIN | 5.333 | WOOD | 1.00 | SALT | 0.30 |
| FISH | 2.667 | STONE | 0.25 | MED_HERBS | 0.30 |
| LIVESTOCK | 3.00 | CLAY | 0.25 | SALTPETER | 0.30 |
| WOOL | 0.60 | IRON | 0.40 | SULFUR | 0.30 |
| SUGAR | 0.60 | COAL | 0.40 | FUR | 0.40 |
| COTTON | 0.60 | COPPER | 0.25 | GOLD | 0.08 |
| FRUIT | 2.667 | PRECIOUS_METAL | 0.05 | PEARL | 0.05 |
| MUREX | 0.05 | INDIGO | 0.06 | ARCANE_CRYSTAL | 0.04 |
| CELESTIAL_IRON | 0.03 | | | | |

**Manufacturées (29)** — recette `RECIPE[BLD]` (`scps_econ.c:422-513`), format intrant(s)→sortie :

| Bâtiment | Recette | Bâtiment | Recette |
|---|---|---|---|
| BLD_TEXTILE | laine(1.5) ou coton(1.5) → tissu(2.8) | BLD_APOTHECARY | herbes(1.0) → remède(1.0) |
| BLD_SAWMILL | bois(2.0)+cuivre(0.2) → fourn. navales(1.0) | BLD_HEAUMERIE | fer(1.0)+charbon(1.0) → heaumes |
| BLD_PAPERMILL | bois(1.5) → papier(1.0) | BLD_PARURIER | or(0.25)+fourrure(1.0) → parures |
| BLD_DISTILLERY | sucre(1.6) ou fruit(4.0) → eau-de-vie(1.4) | BLD_HORLOGER | fer(1.0)+cuivre(1.0) → horloges |
| BLD_BREWERY | grain(1.2) → bière(1.0) | BLD_CHANCELLERIE_LUX | bois(1.0)+argile(1.0) → registres |
| BLD_JEWELER | or(0.2) ou perle(1.6) → orfèvrerie(0.5) | BLD_COMPTOIR_ARTISAN | cuivre(1.0)+sel(1.0) → colifichets |
| BLD_WEAVER_LUX | murex(0.1) ou indigo(0.1)+tissu(4.0) → étoffe précieuse(1.0) | BLD_ATELIER_SEREIN | bois(1.0)+laine(1.0) → ouvrages |
| BLD_TUNIC | tissu(1.0) → tunique(1.0) | BLD_TOOLWORKS | fer(1.0)+bois(1.0) → outils(3.0) |
| BLD_MAGE_WORKSHOP | cristal arcanique(1.0) → essence(1.0) [+bâton de mage] | BLD_CHARCOAL | bois(2.0) → charbon(1.0) |
| BLD_CELESTIAL_FORGE | fer céleste(2.0)+charbon(1.0) → armes enchantées(1.0) | BLD_FOREUSE | essence(0.7) → fer(2.0) [+panier minéraux] |
| BLD_ALAMBIC | salpêtre(1.2) → flux(1.0) [+nécessaire alchimiste] | BLD_ARMORY / HEAVY / BOWYER / ARQUEBUS | fer(+bois/poudre/cuivre) → armes légères/lourdes/de trait/à feu |
| BLD_POTTERY | argile(1.5) → poterie(1.4) | BLD_POWDERMILL | salpêtre(1.0)+charbon(0.8) → poudre(1.0) |
| BLD_SCULPTURE | pierre(2.0) → statuaire(1.0) | BLD_REPLICATEUR / BLD_CORNE (faustien) | flux→bois / fer céleste→grain |

**Biens spéciaux** :
- **Étalon monétaire** — or/cuivre : exemptés de `price_level` (`pl=1` toujours,
  `scps_econ.c:4543`), une part `MINT_ROYALTY` du tirage brut est détournée AVANT le marché
  vers la réserve d'État (`scps_econ.c:3554-3566`, jamais marchandise).
- **Rares** — PEARL, ARCANE_CRYSTAL, CELESTIAL_IRON, PRECIOUS_METAL : rendement 0.03-0.05,
  intrants de biens de très haut statut ou d'armement faustien.
- **Faustiens** — ESSENCE, FLUX, ALCHEMIST_KIT, ENCHANTED_ARMS, MAGE_STAFF : liés à
  `faust_charge`/la Brèche (§27 endgame), leur combustion charge l'entropie
  (`scps_econ.c:3576-3581, 3750-3756`).

### B.5 — Le panier de besoins actuel

`NEED[CLASS_COUNT][RES_COUNT]` (`scps_econ.c:549-575`) — table **statique, par classe**,
quantité/100hab/tick (annuelle pour les vivres via `×food_need`) :

| Classe | Panier (NEED_ORDER, rang 0→n) |
|---|---|
| Laborer | GRAIN(3.50), EAU_DE_VIE(0.35, variante bière), FISH(1.00), WOOD(1.00), TUNIQUE(0.40), POTTERY(0.30, hors-panier) |
| Bourgeois | GRAIN(4.00), SALT(0.20), CLOTH(0.34), REMEDE(0.15), EAU_DE_VIE(0.30), PAPER(0.25), POTTERY(0.25, hors-panier), STATUE(0.12, hors-panier) |
| Élite | GRAIN(4.00), FUR(0.12), PAPER(0.12), EAU_DE_VIE(0.28), PRECIOUS_WARE(0.13, variante orfèvrerie/étoffe), STATUE(0.18, hors-panier) |
| Esclave | GRAIN(3.50) **seul** |

Ordre de priorité : `NEED_ORDER[CLASS][9]` (`scps_econ.c:581-586`), rang via `need_rank()`
(`scps_econ.c:588-592`). Nombre de rangs débloqués : `active_needs = 1+capitale_max_tier(pop
Laborer+Bourgeois+Élite)` (`scps_econ.c:4036-4038`), table de tiers `scps_labor.c:34-51`
(T1 minimum absolu, dès la fondation — jamais 0).

« Besoin comblé » se calcule de **deux façons distinctes** selon le consommateur (cf. A.1) :
- `needs_met` : seuil binaire `got≥τ=0.5` par bien, compté sur `nbasket` = panier COMPLET
  (indépendant du tier) — `scps_econ.c:4097-4131, 4283`.
- `satisfaction`/`basket` : fraction continue pondérée par valeur, sur les rangs ACTIFS
  seulement — `scps_econ.c:4126-4278`.

**Variantes culturelles** (routage, pas un choix joueur) : EAU_DE_VIE→bière/eau-de-vie selon
`culture.subsistance` (`preferred_drink`, `scps_econ.c:608-610`), PRECIOUS_WARE→orfèvrerie/
étoffe précieuse (`preferred_luxe`, `scps_econ.c:618-620`) — la mauvaise variante ne comble
qu'à moitié (`DRINK_OFFCULT`/`LUXE_OFFCULT=0.5`, `scps_econ.c:607,617`).

**Confort-bonus (POTTERY/STATUE)** : hors panier — n'entrent JAMAIS dans `need_w`/`nbasket` ;
consommés, ils ajoutent un bonus PLAT `+COMFORT_JOY(0.08)` à la satisfaction, sans pénalité si
absents (`scps_econ.c:4154-4166`). Design radicalement différent du reste du panier — un
précédent utile pour un « palier bonus » (jamais punitif) si M10 en veut un.

**Désir croisé d'éthos (6 biens, `RES_HEAUMES…RES_OUVRAGES`)** : EUX sont DANS le panier
(pèsent `need_w`/`met_w`), gatés à `active_needs≥ETHOS_LUXURY_MIN_TIER(4)`, Laborer+Élite
seulement (`scps_econ.c:4242-4257, 632-654`).

### B.6 — La consommation post-M5

**Ration vitale garantie** (M5 R3, `scps_econ.c:4141-4152`) : RES_GRAIN (`need_rank==0`,
universel, y compris pour l'Esclave) servi via `got=can_stock` **SEUL** — jamais gaté par
`can_buy`/le budget (le garde-fou anti-collapse post-M3b v1). Payé « au mieux »
(`paid=min(cost,budget)`), manquant toléré SANS dette. Kill-switch `ASSIETTE_ON`
(`scps_econ.c:4116-4118`).

**Élasticité ±20 % à la richesse** (au-dessus du plancher vital, calibrage final post-M5
« Découvertes ») : `elastic_mult = clampf(1+K·(wealth_ratio−1), MIN, MAX)` avec
`K=CONSUME_ELASTIC_K=0.3`, `MIN=CONSUME_ELASTIC_MIN=0.8`, `MAX=CONSUME_ELASTIC_MAX=1.2`
(`scps_econ.c:4117-4125`) — `wealth_ratio = (richesse/tête) / g_basket_pc[pid][c]`, le panier/
tête du **tick précédent** (accumulateur inter-ticks lagué et **sérialisé**, cf. C.11). Neutre
(`×1`) au 1er tick (pas de référence, `g_basket_pc=0`) et à `ASSIETTE_ON=0`.

**Composition** : `need *= elastic_mult` (`scps_econ.c:4153`) s'applique **uniquement** au bloc
générique du panier (donc TUNIQUE, SALT, CLOTH, PAPER, REMEDE, FUR, POTTERY/STATUE via leur
propre calcul, EAU_DE_VIE/PRECIOUS_WARE via leurs blocs variante) — **jamais** à GRAIN (ration
garantie, bloc séparé plus haut dans la boucle) ni au désir croisé d'éthos (bloc tardif séparé,
`scps_econ.c:4242-4257`, portée délibérément restreinte selon M5 « Découvertes »).

### B.7 — Disponibilité : le chemin d'une tonne de bière

```
BLD_BREWERY (grain 1.2 → bière 1.0, labor 27, scps_econ.c:436)
        │  production §2, scps_econ.c:3728
        ▼
S[RES_BEER] += out      ← S = pool[owner_] = LE POOL NATIONAL DE L'EMPIRE
        │  (scps_econ.c:3445 : "float *S = pool[owner_]" — PAS re->stock local)
        ▼
Pool agrégé en tête de tick : pool[o][g] = Σ re->stock[g] sur TOUTES les provinces
colonisées de l'empire (scps_econ.c:3323-3340)
        │
        ▼
Prix soldé UNE FOIS par empire (offre/demande NATIONALES vs pool, price_level
appliqué sauf or/cuivre) puis PROJETÉ sur pe->price de CHAQUE province
(scps_econ.c:4520-4552 — "empire mono-province ⇒ national=local, IDENTIQUE")
        │
        ▼
Boucle consommation §5, PAR PROVINCE dans l'ordre pid=0..n : chaque province
lit/débite le MÊME S[]=pool[owner_] partagé (scps_econ.c:4131-4232) — une
bière brassée en province X peut nourrir un Laborer de la province Y DANS LE
MÊME TICK (premier arrivé dans l'ordre pid, premier servi)
        │
        ▼
Fin de boucle : pool plafonné (Σ caps Entrepôts) + décru (×0.85/mois,
×0.99 arsenal) (scps_econ.c:4557-4573)
        │
        ▼
REDISTRIBUTION pro-rata population → re->stock[] de chaque province
(scps_econ.c:4574-4593) — ce que lisent intertrade/Centres/viewer/save/
butin de guerre jusqu'au tick suivant
```

Il n'existe **pas** de site séparé « circuit d'État M3b achat/revente » pour la consommation
citoyenne au-delà de ce qui est déjà dans §3/§5 : §3 est l'État qui **ACHÈTE** la valeur
ajoutée produite (crédite salaire/profit/rente aux classes, décoté par `price_level`,
`scps_econ.c:3771-3822`) ; §5 est l'État qui **REVEND** (la dépense du citoyen crédite le
trésor de la province au prix courant, `scps_econ.c:4269-4272`). Les Centres/intertrade ont
LEURS PROPRES marges (`IMPORT_TOLL_FRAC`, `IMPORT_MARGIN_*`, cf. TROUVAILLES M5) — un circuit
séparé, pas croisé avec la boucle de consommation des pops.

---

## C. CE QUE M10 A BESOIN DE SAVOIR

### C.8 — Combien de biens distincts un empire consomme-t-il

Sur les **55** biens catalogués, seuls **~21** apparaissent JAMAIS dans un `NEED[]` de pop :
- **15 biens « cœur »** (union des 4 paniers, variantes dépliées) : GRAIN, FISH, WOOD,
  EAU_DE_VIE, BEER, TUNIQUE, POTTERY, CLOTH, PAPER, SALT, REMEDE, STATUE, FUR, PRECIOUS_WARE,
  PRECIOUS_CLOTH.
- **+6 biens de luxe d'éthos**, tardifs (`active_needs≥4`) : HEAUMES, PARURES, HORLOGES,
  REGISTRES, COLIFICHETS, OUVRAGES.

Le reste (34 biens : métaux bruts non-alimentaires, outils, armes, biens arcanes/faustiens,
fournitures navales…) ne comble **jamais** un besoin de pop — ce sont des intrants de
production/institution/guerre.

À la genèse (`EMPIRE_SEED=4000`, `active_needs=5`), le Laborer voit déjà 5/5 de son
`nbasket`, l'Élite 5/5, le Bourgeois 5/6 — **presque tout le panier mature est débloqué dès le
premier tick** pour un empire jouable/IA (une cité-état, `CITY_SEED=2000`, pile T2, ne débloque
que 3 rangs). La lenteur observée n'est donc pas un verrouillage par palier de pop, mais une
lenteur de **montée en capacité de production** (cf. C.9). Aucune mesure en jeu réel du nombre
de biens EFFECTIVEMENT servis en mi/fin de partie n'existe dans TROUVAILLES ou dans le code —
ce point n'est **pas mesuré**, seulement la structure du panier l'est.

### C.9 — La frontière raws/manufacturés pour les premiers paliers

Pour le Laborer (**80 % de la population**, `CLASS_SHARE[LABORER]=0.80`, `scps_econ.c:600`),
le rang 1 (le tout premier besoin après le grain, débloqué dès `active_needs≥2`, donc dès la
fondation) est **EAU_DE_VIE** — un bien **manufacturé** (`BLD_DISTILLERY`/`BLD_BREWERY`). Seul
le rang 0 (GRAIN) est garanti-brut. Pour le Bourgeois et l'Élite, le rang 1 EST un brut (SALT,
FUR) — la règle n'est **pas uniforme entre classes**.

La règle « ≤2 raws/tuile » (`scps_events.c:909-920`, doctrine CLAUDE.md) borne à 2 le nombre de
bruts qu'**une seule province** peut extraire — au-delà, il faut soit la colonisation
(davantage de provinces alimentant le pool national), soit la manufacture (qui elle-même
consomme des bruts, parfois importés d'une autre province via le pool).

**Conclusion** : il n'existe **pas**, dans le code actuel, de « palier 100 % brut » propre sur
lequel accrocher un premier tier M10 pour la classe majoritaire — le second besoin de la classe
Laborer implique déjà une chaîne manufacturière. Un design M10 « raws d'abord, manufacturés
ensuite » devra soit re-classer `NEED_ORDER`, soit définir le déblocage des paliers
indépendamment du rang par-classe existant — c'est une divergence délibérée à trancher, pas un
prolongement naturel du système actuel.

### C.10 — La population d'empire : lecteurs et ordres de grandeur

Lecteurs façade : `scps_world_pop`/`scps_country_pop` (`scps_api.c:327-337`), qui somment
`region_pop_i` par région — un lecteur **grain-région** légitime (vue agrégée, pas un nouveau
verbe, conforme à la doctrine province).

Ordres de grandeur code-sourcés :
- Genèse : `EMPIRE_SEED=4000`/empire jouable-IA, `CITY_SEED=2000`/cité-état
  (`scps_econ.c:1818-1819`), hameaux sauvages `WILD_POP≈750`×2/empire (mentionné en commentaire
  `scps_econ.c:1812`).
- Seuils de tier : T2=2000 … T7=10000 (`scps_labor.c:34-39`) — un empire mi-partie (plusieurs
  provinces colonisées, chacune pouvant dépasser 2-4k) a depuis longtemps franchi tous les
  tiers ; le verrouillage `active_needs` ne mord donc significativement QUE dans les tout
  premiers mois d'une entité neuve (colonie, cité-état).
- Fin de partie (an 250, TROUVAILLES §« POPULATION FINALE », lignes 1646-1658) : population
  **MONDIALE** totale (tous pays confondus, mondes-test à ~9-11 pays) entre **94k et 367k**
  selon graine/run — un ordre de grandeur **mondial**, pas par-pays ; aucune mesure par-pays
  isolée n'a été trouvée dans TROUVAILLES ou le code pour cette mission (à diviser
  approximativement par le nombre de pays actifs si une borne par-empire est nécessaire — non
  fait ici, ce serait une extrapolation, pas une mesure).

### C.11 — Les pièges de sérialisation pour l'hystérésis

Le code documente déjà, à l'identique de ce que M10 devra faire pour des paliers avec
hystérésis, **le patron exact et le bug qu'il évite** :

- `g_basket_pc[SCPS_MAX_PROV][CLASS_COUNT]` (panier/tête, lagué d'1 tick, `scps_econ.c:3075`)
  et `g_lowsat_streak[SCPS_MAX_PROV][CLASS_COUNT]` (mois consécutifs de satisfaction < 30 %,
  `scps_econ.c:3076`) sont des **accumulateurs statiques de MODULE** (hors `ProvinceEconomy`),
  donc invisibles au save automatique de la struct — ils ont dû être câblés à la main :
  `econ_mobility_save`/`econ_mobility_load` (`scps_econ.c:3110-3121`), remis à zéro
  explicitement sur démarrage frais par `econ_mobility_reset` (`scps_econ.c:3086-3089`, appelé
  par `econ_init`).
- Le commentaire `scps_econ.c:3096-3109` nomme le bug évité : avant leur câblage save/load,
  un reload gardait la valeur laissée par la FIN du run précédent (potentiellement des
  centaines de jours après le point de sauvegarde) au lieu de celle du jour sauvegardé — un
  `--savetest` divergeait (Σtrésor dérivant de quelques centièmes à ~15 sur 400 jours).

**Pour un système de paliers à hystérésis (M10)** : si le déblocage/dé-déblocage d'un palier
doit résister à un creux d'un seul tick (le motif exact de `g_lowsat_streak` — 2 mois
consécutifs requis avant démotion), le même patron à QUATRE étapes sera nécessaire : (1) tableau
statique par province/pays, (2) fonction `*_save`/`*_load` dédiée intégrée au blob de save
existant, (3) `save_sane` (bornes vérifiées au chargement, doctrine CLAUDE.md), (4) RAZ
explicite dans `econ_init`/`*_reset` sur démarrage frais — et un bump de `SAVE_VERSION`
(actuellement 94) si la taille sérialisée change. `re->needs_met` lui-même, champ ordinaire de
`ProvinceEconomy`, est DÉJÀ sérialisé avec la struct — ce sont les accumulateurs SATELLITES
(hors struct) que les agents précédents ont dû se souvenir de câbler à la main, et qu'il est
facile d'oublier pour un nouveau système M10.

---

## DIAGRAMME — flux complet

```
┌─────────────┐   §1 extraction    ┌──────────────┐   §2 manufacture   ┌───────────────┐
│  Géographie  │ ───(≤2 raws/tuile)→│ POOL NATIONAL│ ──(RECIPE[BLD])──→ │ POOL NATIONAL │
│  (tirage)    │   EXTRACT_YIELD    │ S[r] (bruts) │                    │ S[r] (+manuf) │
└─────────────┘                    └──────┬───────┘                    └───────┬───────┘
                                            │ §3 État ACHÈTE la VA               │
                                            │ (wage/profit/rente → richesse)     │
                                            ▼                                    │
                                   §5 prix national soldé 1×/empire ◄────────────┘
                                   (price_level, or/cuivre exemptés)
                                            │
                                            ▼
                          §5 CONSOMMATION par province/classe (ordre pid)
                     ┌── GRAIN : ration garantie (can_stock seul) ──────┐
                     ├── générique : can_stock × can_buy × elastic_mult ┤
                     ├── EAU_DE_VIE/PRECIOUS_WARE : variante culturelle ┤
                     └── POTTERY/STATUE : bonus hors-panier (+0.08) ────┘
                                            │  État REVEND (dépense → trésor)
                     ┌──────────────────────┴───────────────────────────┐
                     ▼                                                  ▼
        needs_met = nsat/nbasket                        satisfaction = basket+comfort_joy
        (seuil binaire τ=0.5,                              −over_tax·K_TAX_AGIT
         panier COMPLET, tier-indép.)                       −annex_scar·W   (clamp 0..1)
                     │                                                  │
                     ▼                                                  ▼
          FERTILITÉ (poids 0.85, dominant)          ┌── seuil fiscal (évasion/tolérance)
                                                     ├── contrôleur IA fiscal (cible 60%)
                                                     ├── tech élite (wealth×TECH_RATE×sat)
                                                     ├── légitimité groupe (aisance=sat×10)
                                                     ├── pression fiscale/PE (unmet=1−sat)
                                                     ├── promotion (gate ≥50%) / démotion (<30%, 2 mois)
                                                     ├── croissance (bonus asymétrique >50%)
                                                     └── IA (cible province pire-sat)
                     over_tax (interne à sat.) ──→ ignition de révolte
                     concession de révolte ────→ ÉCRIT +0.15/+0.20 sur satisfaction (retour direct)
```

---

## IMPLICATIONS POUR M10

1. **`needs_met` a un dénominateur tier-indépendant — le réutiliser tel quel comme jauge de
   palier serait trompeur.** Son numérateur (`nsat`) ne compte que les besoins débloqués, mais
   son dénominateur (`nbasket`) compte TOUJOURS le panier mature complet
   (`scps_econ.c:4129-4130`). Un empire qui sert PARFAITEMENT ses 2-3 premiers paliers
   affichera quand même un `needs_met` bas tant que le panier entier n'est pas débloqué — si
   M10 veut une jauge « palier N/N atteint », il lui faut un nouveau ratio scopé aux paliers
   RÉELLEMENT actifs, pas needs_met en l'état.

2. **Satisfaction et besoins comblés sont deux axes différents, et satisfaction porte des
   termes SANS RAPPORT avec les biens** (`comfort_joy`, `over_tax`, `annex_scar`,
   `scps_econ.c:4277-4278`). Si les paliers M10 doivent se lire comme « satisfaction », ces
   termes continueront de diluer/masquer le signal des biens, exactement comme dans la trace
   M8 (15 % vs 0 %, §A.2). Décision à trancher : les paliers pilotent-ils `needs_met` (biens
   purs), `satisfaction` (biens + politique + fisc), ou une TROISIÈME jauge dédiée ?

3. **Le panier actuel est PAR CLASSE et PAR BIEN SPÉCIFIQUE, pas générique.** Le seul
   mécanisme de substitution existant est le routage culturel à DEUX variantes fixes
   (bière/eau-de-vie, orfèvrerie/étoffe précieuse — `preferred_drink`/`preferred_luxe`,
   `scps_econ.c:608-620`), jamais « n'importe quel bien comble le rang ». « Bière = papier »
   est un changement d'architecture réel : soit une structure parallèle au-dessus de
   `NEED[]`/`NEED_ORDER`, soit leur remplacement — pas une extension de données.

4. **La frontière raw/manufacturé n'est PAS nette au rang 1 pour la classe majoritaire.** Le
   Laborer (80 % de la pop) a son 2e besoin déjà manufacturé (EAU_DE_VIE) — Bourgeois/Élite ont
   un brut en rang 1. Un premier palier « 100 % brut » universel devra RÉ-ORDONNER
   `NEED_ORDER`, pas juste ajouter des rangs au-dessus.

5. **Toute hystérésis de palier suit un patron déjà 4 fois éprouvé (et 1 fois cassé puis
   réparé) dans ce fichier** : tableau statique + save/load dédié + `save_sane` + RAZ
   `econ_init` — cf. `g_basket_pc`/`g_lowsat_streak` (`scps_econ.c:3075-3121`). Sauter une
   étape reproduit le bug de divergence `--savetest` déjà documenté.

6. **Le partage exact du delta 15 %→0 % (§A.2) reste une reconstruction plausible, pas un
   chiffre confirmé.** Les constantes de genèse (EMPIRE_SEED=4000, table de tiers, table de
   tolérance fiscale, STATE_TAX_AMBITION) rendent l'explication « verrouillage par tier »
   FAUSSE (tout est quasi débloqué dès le tick 1) et pointent plutôt vers « capacité de
   production pas encore montée » + « grogne fiscale déjà active sur un défaut de genèse à
   0.5 » — mais un run instrumenté (hors périmètre de cette mission) serait nécessaire pour
   fixer les proportions exactes si M10 en a besoin au chiffre près.

---

*Rapport produit en lecture seule (aucune compilation, aucune exécution) — HEAD `42373dc`,
`scps_econ.c` 5834 lignes, SAVE_VERSION 94.*
