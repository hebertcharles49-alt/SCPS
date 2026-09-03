# CALIBRAGE — TECHNOLOGIE & ACCÈS D'HÉRITAGE (2026-09-03)

Rapport de LECTURE. Aucune modification de code. Toute valeur est citée `fichier:ligne`.
Données : `sweep_doct_ai_10x200/*.log` (20 sims = 10 graines × 2 bras, 200 ans, 6 empires
+ 12 cités) + mesures courtes `./chronicle.exe 7 1 120 6 12` (base · `AI_RESEARCH_INCOME_W=1`
· `TECH_COST_MULT=1`).

---

## 0. L'arbre en chiffres (le socle du reste)

`TECH_COUNT = 74` (`scps/scps_tech.h:110`) — la doc `docs/LEVIERS.md:461` annonce encore
« 85 nœuds » : **dérive documentaire**.

| tier | nœuds | `BASE_COST` (`scps_tech.c:35`) | Σ base |
|---|---|---|---|
| 0 (socle, gratuit) | 6 | 0 | 0 |
| 1 | 16 | 40 | 640 |
| 2 | 18 | 90 | 1 620 |
| 3 | 10 | 160 | 1 600 |
| 4 | 21 (7 + **14 combos**) | 260 | 5 460 |
| 5 | 3 (apex) | 400 | 1 200 |
| **Σ** | **74** | — | **10 520** |

11 nœuds faustiens · 3 nœuds `needs_ruins` · 26 nœuds à `native != UNIV` (signatures,
étoffe, combos, apex).

---

## 1. Coût réel d'un palier — la formule et ses fourchettes

### 1.1 Le prix nu

`tech_cost()` — `scps/scps_tech.c:667-680` :

```
cost = BASE_COST[tier] × COST_SCALE(14.4) × max(0.5, 0.90·N^0.5) × tune("TECH_COST_MULT", 0.70)
```

`COST_SCALE=14.4` (`scps_tech.c:31`) · `TECH_COST_N_K=0.90` (`:32`) · `TECH_COST_N_EXP=0.5`
(`:33`) · `TECH_COST_N_FLOOR=0.5` (`:34`) · `TECH_COST_MULT=0.70` (`scps_tune_list.h:265`).

Le plancher `0.5` est **mort** : `0.90·√N ≥ 0.90 > 0.50` dès `N=1`. La constante effective
est donc `14.4 × 0.70 × 0.90 = 9.072`, et **`cost = BASE × 9.072 × √N`**.

| tier | N=1 | N=4 | N=10 | N=20 | N=45 | N=70 |
|---|---|---|---|---|---|---|
| 1 |  363 |  726 | 1 148 | 1 623 | 2 435 | 3 037 |
| 2 |  817 | 1 633 | 2 582 | 3 652 | 5 478 | 6 832 |
| 3 | 1 452 | 2 903 | 4 590 | 6 491 | 9 739 | 12 146 |
| 4 | 2 359 | 4 717 | 7 459 | 10 548 | 15 825 | 19 737 |
| 5 | 3 629 | 7 258 | 11 475 | 16 227 | 24 345 | 30 364 |

### 1.2 Les multiplicateurs empilés (`ai_effective_cost`, `scps_ai.c:2415-2420`)

| facteur | site | plage | note |
|---|---|---|---|
| éthos × fonction | `ai_tech_cost_mult`, `scps_ai.c:2102-2129` | **0.60 – 1.60** (clamp `:2128`) | biais borné |
| remise de diffusion | `tech_diffusion_mult`, `scps_ai.c:2434-2439` | **0.60 – 1.00** | `AI_TECH_DIFFUSE_MAX=0.40` (`scps_tune_list.h:447`) |
| traditions (arcane) | `ai_tech_tradition_mult`, `scps_ai.c:2405-2410` | **0.50 – 2.00**, faustiens SEULS | `TRAD_ARCANE_W=0.25` |
| découplage faustien | `scps_ai.c:2419` | **×4.5** sur les faustiens | `AI_RESEARCH_INCOME_W` (`scps_tune_list.h:315`) |

⇒ enveloppe **non-faustien ×0.36 … ×1.60** (facteur 4,4 entre meilleur et pire cas),
**faustien ×0.81 … ×14.4** (facteur 17,8).

### 1.3 Le revenu

`ai_research_step`, `scps_ai.c:2586-2602` (miroir joueur `scps_sim.c:1226`) :

```
revenu/an = econ_country_savoir(cid) × tech_research_yield(ts) × AI_RESEARCH_INCOME_W(4.5)
            × wp->age_research_mult × (1 + AI_METAB_RES_W(1.0)·métabolisation)
```

`econ_country_savoir` (`scps_econ.c:989-1027`) :
`(0.01·É + 0.005·B + 0.001·J) × (1 + min(0.33, 0.067·Σbuild.savoir)) × (0.5 + 0.75·sat)`.
`tech_research_yield` = `1 + 0.5` par maillon Savoir·Production acquis, donc **×1.0 … ×2.5**
(`scps_tech.c:479-485`).

**Le facteur bibliothèque est inerte en pratique** : Σ build.savoir ≈ 0,0 par empire, même
chez les leaders à 65 techs (mesuré, `TROUVAILLES.md:4227` §« Bibliothèques : PAS le
discriminateur ») ⇒ `(1+pct) ≈ 1.00`, le plafond 1,33 n'est jamais approché.

**Cap dur oublié** : `AI_RESEARCH_CADENCE = 365` (`scps_ai.c:75`) — l'IA n'évalue et ne paie
qu'**UN nœud par an**, et ce n'est PAS un tunable du registre J.

### 1.4 Les trois fourchettes (an 200, valeurs lues aux logs)

Satisfaction pondérée mesurée : J 39-49 % · B 68-74 % · É 59-64 % ⇒ `f_sat ≈ 0.82-0.88`.

**A — LE LEADER** (`Ligue Pyxexis`, essai_s7 : 45 rég, J 104,6k / B 12,2k / É 2,5k —
`sweep_doct_ai_10x200/essai_s7_y200.log:671-672`) :
`base = 0.01·2500 + 0.005·12200 + 0.001·104600 = 190,6` → `savoir ≈ 168/an` →
`revenu ≈ 168 × 2,5 × 4,5 × 1,1 ≈ **2 080 pts/an**`.
Prix effectif à N=45, éthos 0,8 × diffusion 0,8 :

| palier | prix effectif | années |
|---|---|---|
| t2 | 3 506 | **1,7** |
| t3 | 6 233 | **3,0** |
| t4 (combo) | 10 128 | **4,9** |
| t5 (apex) | 15 581 | **7,5** |
| t4 faustien | 51 273 | **24,7** |

**B — L'EMPIRE MOYEN** (N≈12, pop ≈ 50k, yield 2,0, métabolisation ≈ 0 —
`Clans Caelwicyn`, 13 rég / 55k, 41 tech, `essai_s1009_y200.log`) :
`savoir ≈ 54/an` → `revenu ≈ 486 pts/an` ; t2 = 3,7 ans · t3 = 6,6 ans · t4 = 10,8 ans ·
t5 = 16,6 ans ⇒ **30-45 nœuds en 200 ans**. Conforme au relevé (41-48 tech).

**C — LE NAIN** (`Ligue Karggoris`, 2 rég, pop 0,3k, 1 tech —
`essai_s7_y200.log:716-718`) : `savoir ≈ 0,34/an` → `revenu ≈ 1,5 pts/an` ; un SEUL nœud
tier-1 à N=2 coûte ≈ 410 pts ⇒ **≈ 270 ans**. C'est la bande « min 8-9 % » de toutes les
sims : **stagnation totale**, jamais un rattrapage.

### 1.5 Coût de l'arbre COMPLET

Σ BASE = 10 520 ⇒ à N=45 : `10 520 × 9,072 × 6,708 ≈ **640 000 pts**` (à N=10 : 302 000).
Le leader gagne ~2 000/an en fin de partie et bien moins avant ⇒ le budget ne se boucle que
grâce aux remises (§3.5) et à la croissance de pop.

---

## 2. Cadence observée — l'arbre se BOUCLE, il ne force pas de choix

`« N tech »` en fiche empire = `n_unlocked − 6` (socle tier-0), `chronicle.c:2013-2014`.
`« arbre X% »` = `100·n_unlocked/74`, `chronicle.c:2432`.

### 2.1 Le leader de chaque sim, an 200

| graine | essai | témoin | graine | essai | témoin |
|---|---|---|---|---|---|
| 7    | 65 | 65 | 512  | 64 | 65 |
| 1009 | 65 | 65 | 3333 | 57 | 57 |
| 4243 | 65 | 65 | 777  | 65 | 65 |
| 11   | 65 | 65 | 90   | 64 | 65 |
| 2026 | 65 | 63 | 60   | 65 | 65 |

**Médiane = 65 · min = 57 · max = 65** sur 20 sims. `65 tech = 71/74 nœuds = 95 %` — la
ligne `arbre : … max 95%` apparaît dans **18 logs sur 20** (85 % pour s3333 seul).

**71 = 74 − 3.** Les 3 manquants sont EXACTEMENT les 3 nœuds `needs_ruins` (§3.1).
Autrement dit : **le leader prend TOUT ce qui est accessible.** L'intention écrite
(`scps_tech.c:30` : « un empire ne s'offre que ~40-60 % de l'arbre → il SPÉCIALISE »)
n'est pas tenue au sommet.

### 2.2 La médiane par empire est un ARTEFACT

| graine | essai | témoin | graine | essai | témoin |
|---|---|---|---|---|---|
| 7 | 55 % | 56 % | 512 | 39 % | 42 % |
| 1009 | 33 % | 31 % | 3333 | 44 % | 42 % |
| 4243 | 34 % | 46 % | 777 | 54 % | 53 % |
| 11 | 43 % | 45 % | 90 | 48 % | 54 % |
| 2026 | 35 % | 35 % | 60 | 54 % | 49 % |

Médiane essai 43,5 % · témoin 45,5 % · **combinée ≈ 44 %** — dans la cible 40-60 %.
Mais c'est la **moyenne d'un leader à 95 % et d'une queue à 8-9 %** : la colonne `min` vaut
8 % ou 9 % dans **20 logs sur 20**. La distribution est bimodale, jamais centrée.

### 2.3 La cadence dans le temps (mesure directe, graine 7)

| horizon | leader | arbre méd. | monde (nœuds) | source |
|---|---|---|---|---|
| an 54 | — | — | 38 | `essai_s7_y200.log` « par âge » |
| an 93 | — | — | 127 | idem |
| **an 120** | **49 tech** (55/74 = 74 %) | **26 %** (max 74 %) | **239** | mesure `chronicle 7 1 120 6 12` |
| an 181 | — | — | 454 | `essai_s7_y200.log` |
| an 200 | **65 tech** (71/74 = 95 %) | 55 % | 487 | idem |

⇒ le leader passe de 74 % à 95 % entre l'an 120 et l'an 200 : **l'arbre se referme vers
l'an 185-195**, juste avant l'horizon. Le plafond « 65 » est un ancrage connu et ancien
(`TROUVAILLES.md:4229` : « ancre leaders ±0 % (max 65 identique) »).

### 2.4 Sensibilité mesurée (3 runs, graine 7, 120 ans, `chronicle 7 1 120 6 12`)

| bras | leader | arbre méd. | max | monde | combos | remise (nœuds soldés) | édifices refusés (palier) | spécialisation |
|---|---|---|---|---|---|---|---|---|
| **base** | **49 tech** | 26 % | 74 % | **239** | 12 | 56 | 187 | 8 Sav · 0 Forge · 9 Soc |
| `AI_RESEARCH_INCOME_W=1` (×1/4,5) | **21 tech** | 11 % | 36 % | **48** | 1 | 27 | 448 | 17 Sav · 0 Forge · 1 Soc |
| `TECH_COST_MULT=1` (prix ×1,43) | **31 tech** | 18 % | 52 % | **178** | 3 | 37 | 323 | 16 Sav · 0 Forge · 6 Soc |

Trois lectures :

1. **`AI_RESEARCH_INCOME_W = 4.5` est LE levier** : le mettre à 1 divise le volume mondial
   par **5,0** (239 → 48 nœuds à l'an 120) et le leader par 2,3.
2. **Le prix mord fort et de façon PROGRESSIVE** : +43 % de prix (`TECH_COST_MULT` 0,70 → 1,00)
   coûte au leader **37 % de ses nœuds** (49 → 31 tech), fait tomber le max de 74 % à **52 %**,
   et les combos de 12 à **3**. L'élasticité leader/prix est donc voisine de **−0,85** dans
   cette plage : renchérir est un outil **fin et efficace** pour rouvrir la contrainte de choix.
3. **Dès que l'argent manque, la « soif de savoir » monopolise tout** : la spécialisation
   bascule en `16-17 Savoir · 0 Forge · 1-6 Société` — l'épargne ciblée
   Scriptorium→Université (`scps_ai.c:2648-2679`) rafle le budget et plus rien d'autre n'est
   jamais payé. C'est le corollaire de §3.2.

**Le thème FORGE n'est dominant chez PERSONNE** : la colonne `Forge` de la ligne
`spécialisation` vaut **0** dans **15 des 20 logs du sweep** (1 dans s90/s4243/s2026, seul
s3333 fait exception à 7-8). Sur 9 quartiers, un tiers de l'arbre n'est jamais l'identité
de personne.

### 2.5 Le rôle de l'héritage

- **Métabolisation → recherche** (`AI_METAB_RES_W=1.0`, `scps_ai.c:2600`) : sur 200 ans,
  moyenne monde 1,3 % (temoin_s777) à 11,8 % (temoin_s7) ; **max par empire 10,9 % → 68,4 %**
  (`essai_s4243_y200.log`). Le meilleur creuset gagne jusqu'à **+68 % de recherche**, soit
  l'équivalent de 1,7 maillon de la chaîne Savoir — significatif mais très dispersé.
- **Combos tier-4** : de 30 (temoin_s2026) à **219** (essai_s512) par sim ; de 4 à **26**
  empires en tiennent au moins un. Sur 18 possibles par empire (Forge runique + 14 combos +
  3 apex, `chronicle.c:2349-2353`), la moyenne des porteurs est de 8-9. **Ce n'est pas rare.**
- **Archétypes atteints** : `6/6` dans **18 logs sur 20** (5/6 pour s3333 seul).
- **Remise de diffusion** : `70 ou 71 tech(s) escomptée(s) (−5 %+) · remise max −40 %` dans
  **19 logs sur 20** (68 pour temoin_s11) — la quasi-totalité de l'arbre est soldée.
- **Récurrence** : témoin (AI_DOCT=0) et essai donnent le MÊME plafond dans 9 graines sur 10.
  **Les doctrines ne touchent pas la cadence tech.**

---

## 3. Ce qui est OP · ce qui est mort

### 3.1 MORT PAR CONSTRUCTION — les 3 nœuds `needs_ruins`

`scps_tech.c:639` refuse tout nœud `needs_ruins` si `!s->has_ruins_access`. Or
**`scps_sim.c:1726` initialise TOUS les pays avec `tech_state_init(&s->ts[c], false)`**, et
aucun site du moteur ne repasse jamais ce drapeau à `true` (recherche exhaustive : seuls
`army_demo.c:240-260` passent `true`, hors sim).

⇒ **`TECH_INVOCATION` (`scps_tech.c:73`), `TECH_EVEIL` (`:77`), `TECH_SAVOIR_INTERDIT`
(`:94`) sont inaccessibles à jamais** — 3/74 = 4 % de l'arbre. C'est la preuve arithmétique
du plafond `95 % = 71/74` observé dans 18 logs sur 20.

Conséquence lourde : **`TECH_EVEIL` est le SEUL nœud avec `triggers_crisis = true`**
(`scps_tech.c:59`, champ lu `scps_tech.c:661`). Le mécanisme « une tech convoque elle-même
la crise de fin » n'a **jamais pu se déclencher** dans aucune partie.

### 3.2 OP — la spine Savoir·Production, meilleur rapport du jeu

`Scriptorium + Académie + Université` = 40+90+160 = **290 pts de base**, soit **2,8 % de la
Σ BASE de l'arbre (10 520)**, pour `tech_research_yield` **×1 → ×2,5** permanent
(`scps_tech.c:479-485`), qui multiplie TOUTE recherche ultérieure.

À N=45, éthos 0,8, diffusion 0,8 : la chaîne coûte ≈ 14 120 pts et rend +1 250 pts/an ⇒
**amortie en ~11 ans**, bénéfice pur les 190 restants. Rien d'autre n'approche ce rendement.

### 3.3 OP — le bonus de production n'a AUCUN plafond

`econ_apply_country_tech` (`scps_econ.c`) :
`pe->tech_prod = 1.f + tech_prod_bonus(&ts[o]) + tech_eff_bonus(&ts[o])`, **sans clamp haut**
(seul un plancher `0.1` existe, posé après l'ajout du levier traditions).

Σ `NODE_PROD_PCT` (`scps_tech.c:490-505`) = **1,65** · Σ `NODE_EFF_PCT` (`:506-516`) = **0,38**.
⇒ **arbre complet = `tech_prod = 3,03`**, +0,30 de traditions ⇒ **jusqu'à ×3,33 de
production**. Le commentaire du code dit « multiplicateurs MODESTES » (`scps_tech.c:487`).
Et comme la production nourrit la pop et la richesse, qui nourrissent
`econ_country_savoir`, qui nourrit la recherche : **boucle de rétroaction positive non
bornée entre l'arbre et lui-même**. C'est le moteur mécanique du « le leader finit l'arbre ».

### 3.4 OP — l'accès d'héritage se prend par la GOUVERNANCE, pas par la métabolisation

`heritage_access_pack` (`scps_ai.c:2338-2354`) prend le **MAX** de deux voies :
- métabolisation : `METAB_TIER1/2/3 = 0.10 / 0.20 / 0.35` (`scps_tune_list.h:354-356`) ;
- profondeur de contact : `PROF_PROFOND ⇒ tier 3` (accès PLEIN).

Or dans `ai_archetype_depth` (`scps_ai.c:2270-2277`), **toute région que JE possède** donne
`PROF_SECRET` si cohésion ≥ 0,66, **`PROF_PROFOND` dès cohésion ≥ 0,33**, `PROF_METIER` sinon.
Et `region_bears_arch` (`scps_ai.c:2233-2245`) renvoie vrai dès qu'**UN SEUL groupe de pop
d'UNE province de la région** porte l'archétype — **aucun seuil d'effectif, aucun seuil
d'intégration sur ce groupe** (`region_cohesion`, `scps_ai.c:2213-2224`, ne pèse que la
moyenne de la région).

⇒ **une poignée d'âmes d'héritage X (migrants, soumis ou déportés) posées dans une région
par ailleurs bien intégrée ouvre l'accès tier-3 PLEIN à tout l'héritage X** : sa signature,
ses 2 branches d'étoffe, ses combos, ses apex. La barre 10/20/35 % est court-circuitée.

C'est la lecture chiffrée du « brassage exploitable » : `SLAVE_FRACTION=0.05` /
`SLAVE_FRACTION_TECH=0.15` (`scps_tune_list.h:376-377`) déportent bien plus que le
nécessaire, et le déporté compte PLEIN pour l'accès (`region_bears_arch`) alors que sa
diffusion de savoir est volontairement bridée à
`METAB_DIFFUSE_SLAVE=0.30 × SLAVE_METAB_FLOOR=0.15 ≈ 4,5 %` (`scps_tune_list.h:360-368`).
**Les deux voies se contredisent** : l'esclave ne transmet presque rien au creuset, mais il
ouvre 100 % de la porte technologique.

Preuve au relevé : `6/6 archétype(s)` dans 18/20 sims, combos jusqu'à 219/sim, `dispersion
0–34/empire` sur 34 nœuds à porte d'archétype possibles.

### 3.5 OP — la remise de diffusion solde l'arbre pour TOUT LE MONDE

`tech_diffusion_mult` (`scps_ai.c:2434-2439`) : `1 − 0.40 × (part des empires qui possèdent
déjà la tech)`. Elle est **symétrique** : le leader qui a débloqué un nœud le premier profite
ensuite de la remise sur tous ceux que les autres ont pris. Relevé : « **70 tech(s)
escomptée(s) (−5 %+) · remise max −40 %** » (`essai_s7_y200.log:757`) — 70 des 74 nœuds
soldés en fin de partie. Elle **cumule avec le biais d'éthos** (0,60) : plancher réel
`0,36 × prix nu` pour un nœud répandu et bien orienté.

### 3.6 BRUIT — l'héritage de l'arbre au cataclysme fausse la statistique

`scps_sim.c:1556` : `s->ts[ch] = s->ts[pa];` — un fragment né du resplit §27 (an 180) reçoit
**l'arbre COMPLET de son parent** (seule la banque `research_points` repart à 0, `:1557`).
Effet visible : `essai_s512_y200.log` liste 24 empires dont **`Clans Fizzexa` (5 rég, 8k hab)
à 64 tech**, `Ordre Tikexel` (4 rég), `Clans Nimyn` (2 rég), `Clans Nimexa` (3 rég)… tous
**exactement à 64 tech**, plus une grappe entière à **49** et une autre à **40**. Ces pays ont
un revenu de recherche de l'ordre de **150 pts/an** — il leur faudrait un siècle pour UN nœud
tier-4. Le §27 FIN de cette sim est bien un ENGLOUTISSEMENT an 180
(`essai_s512_y200.log:635`).

⇒ la médiane `arbre %` publiée par la chronique (`chronicle.c:2429-2444`) mesure en partie
des **copies**, pas de la recherche. Elle est simultanément gonflée (les héritiers) et tirée
vers le bas (les fragments neufs à 8 %).

### 3.7 MORTS — les libellés et les clés fantômes

- `dF`, `dEco`, `dMil` : **jamais lus par le moteur** (déclaré `scps_tech.h:126-131`, redit
  `docs/LEVIERS.md:469-472`) — présents dans 40+ nœuds, affichés en libellé.
- `TECH_COST_N_FLOOR` (`scps_tech.c:34`) : branche morte (§1.1).
- **`METAB_TIER12`** : clé portée par 2 idées de doctrine (`scps_doctrines.c:67` Métissage,
  `:121` Dictionnaires) qui **n'existe dans AUCUN registre et n'est lue à AUCUN site**.
  Clé purement fantôme.
- **`FOG_SEA_HALO`** (idée Portulans, `scps_doctrines.c:118`) : c'est un `#define` local à
  `scps_api.c:656`, pas un tunable ; aucun `doctrine_key_mult` ne le lit.
- **`AI_TECH_DIFFUSE_MAX`** (idée Copistes, `scps_doctrines.c:120`) : le site de lecture
  `scps_ai.c:2438` appelle `tune_f` **sans** `doctrine_key_mult` ⇒ l'idée est inerte.
  (Les masques `cable=0` le déclarent honnêtement, mais la clé promet un effet.)

### 3.8 Pourquoi Technologie 2/331 et Connaissances 2/331 (sweep `resume.txt`)

`ai_doct_scores`, `scps_ai.c:3302-3305` :

```c
out[DOCT_TECHNOLOGIE] = aid_clamp((float)nlib*0.6f, 0.f, 1.2f)
                      + aid_clamp((float)((double)sv/ppp), 0.f, 0.6f);
```

- `nlib` = provinces portant `EDI_BIBLIOTHEQUE|EDI_MONASTERE` (`scps_ai.c:3164`). Mesuré :
  Σ build.savoir ≈ **0,0 par empire, y compris chez les leaders à 65 techs**
  (`TROUVAILLES.md:4227`) ⇒ **premier terme ≈ 0**.
- `sv/ppp` = savoir par tête et par an : pour le leader, `168 / 119 000 ≈ 0,0014` ⇒ **second
  terme ≈ 0,001**. Le clamp `[0..0.6]` attend visiblement une grandeur d'ordre 1 :
  **erreur d'échelle d'un facteur ~400**.

⇒ score total **≈ 0,002**, contre Production ≤ 2,0 (`:3294-3295`), Infrastructure ≤ 2,4
(`:3298-3300`), Vassaux 1,4-2,6 (`:3268`), et le « courant » gagnant 1,0-2,0 (`:3312`). Elle
ne peut **mathématiquement jamais** gagner l'`argmax` de `ai_doctrines_year:3324-3330`. Les
2 adoptions observées sont des cas où presque tout le reste valait 0 — et un score exactement
nul est d'ailleurs exclu (`scps_ai.c:3327`).

```c
out[DOCT_CONNAISSANCES] = aid_clamp(met*3.f, 0.f, 1.2f) + aid_clamp((float)ndig*0.4f, 0.f, 0.8f);
```
`met` = métabolisation, moyenne monde **1,3 % à 11,8 %** ⇒ 0,04-0,35 ; `ndig` (héritages
digérés > 10 %) vaut 0 ou 1 chez la plupart ⇒ **score 0,04-0,75**. Toujours sous Production.

Câblage réel de ces deux doctrines : **2 idées vivantes sur 6, chacune**.
Technologie — Bibliothèques (`SAVOIR_LIB_PER`/`_MAX`, lues `scps_econ.c:1010-1011`) et Écoles
de ville (`SAVOIR_W_BOURGEOIS`/`_LABORER`, lues `scps_econ.c:996-997`).
Connaissances — Truchements (`SYNC_TRADE_METIER`/`_PROFOND`, lues `scps_ai.c:2316-2317`) et
Collèges (`AI_METAB_RES_W`, lue `scps_ai.c:2600`). Le reste : verbes ou clés fantômes (§3.7).
**Et Bibliothèques module un facteur dont §1.3 montre qu'il vaut ×1,00** : même adoptée,
l'idée-phare de la doctrine Technologie ne rend presque rien.

---

## 4. Propositions CHIFFRÉES — classées par impact

> Aucune n'est appliquée. Chacune indique le site, le chiffre et le risque.

### P1 — Plafonner `tech_prod` (impact MAJEUR · risque MOYEN-HAUT)
**Site** : `scps_econ.c`, `econ_apply_country_tech`, la ligne
`pe->tech_prod = 1.f + tech_prod_bonus(...) + tech_eff_bonus(...)` (et son miroir région).
**Geste** : `TECH_PROD_CAP` au registre J, défaut **3.10** (= le plafond actuel 3,03 ⇒
kill-switch neutre, golden intact), puis calibrer à **1.90**.
**Chiffre** : arbre complet aujourd'hui ×3,03 → ×1,90 (−37 % sur le rendement du leader
maxé ; **aucun effet sous 90 points de bonus cumulés**, donc rien ne change pour l'empire
moyen). Casse la boucle production → pop → savoir → arbre → production.
**Risque** : c'est le levier de production principal — sweep apparié 3×3 obligatoire, gate
sur le PIB monde et `richesse/tête`.

### P2 — Rendre le coût progressif avec la taille (impact MAJEUR · risque MOYEN)
**Site** : `scps_tech.c:33`, `TECH_COST_N_EXP 0.5f` (constante compilée — **à enregistrer**
au registre J pour être calibrable).
**Geste** : `0.50 → 0.65`.
**Chiffre** : coût ×`N^0.15` — N=2 : **×1,11** · N=10 : **×1,41** · N=20 : **×1,55** ·
N=45 : **×1,77** · N=70 : **×1,92**. Le nain ne paie rien de plus, le leader paie 77 % de plus.
Calage par la mesure §2.4 : un ×1,43 UNIFORME fait tomber le leader de 49 à 31 tech
(−37 %) et le max de 74 % à 52 % à l'an 120. Un ×1,77 **ciblé sur le seul leader** vise donc
un plafond an-200 autour de **50-58 nœuds (68-78 %)** au lieu de 71 (95 %), l'empire moyen
(N≈12, ×1,41) et le nain (N=2, ×1,11) restant à leur cadence actuelle. La spécialisation
redevient une contrainte réelle sans casser « wide récompensé » (l'exposant reste < 1).
**Risque** : re-baseline golden ; vérifier que les édifices T4/T5 restent atteignables
(`407 édifice(s) refusé(s) faute de tech de palier` est déjà haut, `essai_s7_y200.log:818`).

### P3 — Renchérir SEULEMENT les tiers 4-5 (impact MAJEUR · risque MOYEN — alternative à P2)
**Site** : `scps_tech.c:35`, `BASE_COST[] = {0,40,90,160,260,400}` (surchargeable par
`SCPS_MODS`, `tech_moddata_load`).
**Geste** : `260 → 340` (t4) et `400 → 560` (t5).
**Chiffre** : Σ BASE 10 520 → **12 800** (+22 %), concentré sur les 24 nœuds combos/apex.
À N=45 : t4 15 825 → **20 695** (4,9 → 6,4 ans), t5 24 345 → **34 083** (7,5 → 10,5 ans).
Sur 24 nœuds cela ajoute **~117 ans de recherche du leader** ⇒ l'arbre ne se referme plus
avant l'horizon. Cible : combos/sim de 30-219 → **20-80** (la mesure §2.4 montre que les
combos sont l'agrégat le plus sensible au prix : ×1,43 les fait passer de 12 à 3 à l'an 120).
**Risque** : plus chirurgical que P2 (n'affecte pas les petits, qui n'atteignent jamais t4).
**Attention** : ne PAS combiner P2 et P3 sans mesure — leurs effets se multiplient et le
run `TECH_COST_MULT=1` seul monte déjà les édifices refusés faute de palier de 187 à 323.

### P4 — Poser un seuil sur la porte d'archétype (impact FORT · risque MOYEN)
**Sites** : `scps_ai.c:2233-2245` (`region_bears_arch`, aucun seuil d'effectif) et
`scps_ai.c:2276` (`coh>=0.33f ⇒ PROF_PROFOND`).
**Geste** : (a) `ARCH_BEARER_MIN_FRAC`, défaut **0.00** (neutre) puis **0.05** — un groupe
minoritaire doit peser ≥ 5 % de la pop de la région pour « porter » l'archétype ;
(b) `ARCH_COH_PROFOND`, défaut **0.33** (neutre) puis **0.50**.
**Chiffre** : rend son sens à l'échelle `METAB_TIER1/2/3 = 0,10 / 0,20 / 0,35`. Cible :
`6/6 archétype(s)` dans 18/20 sims → **3-4/6 en médiane** ; combos/sim → 20-80. Ferme
l'exploit « une poignée de déportés = accès tier-3 plein » (§3.4).
**Risque** : `TECH_FORGE_RUNES` gate la Corne divine (22 bâties, `essai_s7_y200.log:728`) —
surveiller que la chaîne d'armes ne meure pas.

### P5 — Ouvrir (ou retirer) la porte des ruines (impact FORT · risque HAUT)
**Site** : `scps_sim.c:1726`, `tech_state_init(&s->ts[c], false)`.
**Geste** : `TECH_RUINS_ACCESS`, défaut **0** (= aujourd'hui, golden intact) ; valeur 1 =
`has_ruins_access` accordé au pays qui tient une province avec
`raw_cap[RES_ARCANE_CRYSTAL] > 0.1` ou `raw_cap[RES_CELESTIAL_IRON] > 0.1` — **le signal
existe déjà**, `ai_pick_tech` le lit à `scps_ai.c:2463-2468`. Alternative : retirer
`needs_ruins` des 3 nœuds et assumer 74/74 accessibles.
**Chiffre** : +3 nœuds (4 % de l'arbre) de charges 3,0 / 6,0 / 4,0 et flux 1,5 / 3,0 / 2,0
(`scps_tech.c:55,59,76`) ⇒ jusqu'à **+13 de charge faustienne** par empire arcanique.
Rend vivant l'unique `triggers_crisis` du jeu.
**Risque** : HAUT — `ENTROPY_TECH_W = 0.20` (`scps_tune_list.h:1057`) pèse la charge
faustienne dans l'entropie §27 ; la fenêtre des fins bougera. Défaut OFF impératif, sweep
apparié dédié.

### P6 — Rendre les deux doctrines de savoir SÉLECTIONNABLES (impact MOYEN · risque FAIBLE)
**Site** : `scps_ai.c:3302-3305`.
**Geste** : corriger l'échelle du terme savoir et ajouter un signal que les empires ont
réellement :
```
out[DOCT_TECHNOLOGIE]   = clamp(nlib*0.6, 0, 1.2)
                        + clamp((sv/ppp)*300.f, 0, 0.8)          /* ≈ 0,42 pour le leader */
                        + clamp(n_unlocked/74.f * 1.2f, 0, 1.2); /* ≈ 1,15 pour le leader */
out[DOCT_CONNAISSANCES] = clamp(met*3.f, 0, 1.2)
                        + clamp(ndig*0.4f, 0, 0.8)
                        + clamp(narch_profond*0.25f, 0, 1.0);    /* arch_depth ≥ PROF_PROFOND */
```
**Chiffre** : porte les deux scores dans la bande **0,5-2,4**, la même que Production /
Infrastructure / Vassaux ⇒ adoptions attendues **15-40 sur ~331** au lieu de 2.
**Risque** : FAIBLE (golden intact tant que `AI_DOCT=0` ; le module ne tire aucun xs32,
`scps_doctrines.c:4-6`). Vérifier que Production ne s'effondre pas en contrepartie.

### P7 — Assainir les clés fantômes (impact MOYEN · risque FAIBLE)
**Sites** : `scps_doctrines.c:67` et `:121` (`METAB_TIER12`, inexistante) · `:118`
(`FOG_SEA_HALO`, `#define` de `scps_api.c:656`) · `:120` (`AI_TECH_DIFFUSE_MAX`, site
`scps_ai.c:2438` sans `doctrine_key_mult`).
**Geste** : soit câbler (ajouter `METAB_TIER12` au registre comme diviseur commun de
`METAB_TIER1/2` au site `scps_ai.c:2345-2347`, et un `doctrine_key_mult` sur
`AI_TECH_DIFFUSE_MAX`), soit retirer la clé du catalogue. **Aujourd'hui, 4 idées sur 12 des
deux doctrines de savoir affichent une clé qui n'a pas de site.**
**Risque** : FAIBLE si on ne nettoie que le libellé ; MOYEN si on câble (sweep requis).

### P8 — Honnêteté de la télémétrie de l'arbre (impact MOYEN · risque NUL)
**Site** : `chronicle.c:2429-2444`.
**Geste** : exclure de la médiane `arbre %` les pays qui n'ont jamais RIEN payé
(`stats.techs == 0 && n_unlocked > 6` ⇒ héritier de succession, `scps_sim.c:1556`) et publier
deux lignes : `arbre RECHERCHÉ` et `arbre HÉRITÉ`.
**Chiffre** : la médiane 44 % actuelle mélange des copies (§3.6) et des fragments neufs.
**Risque** : NUL (affichage). **À faire AVANT le prochain sweep**, sinon P1-P4 se calibrent
sur du bruit.

### P9 — Enregistrer la cadence (impact FAIBLE · risque FAIBLE)
**Site** : `scps_ai.c:75`, `#define AI_RESEARCH_CADENCE 365`.
**Geste** : le porter au registre J (`TECH_CADENCE_DAYS`, défaut 365). La limite dure
« 1 nœud/an » est aujourd'hui hors de portée de tout calibrage, alors qu'elle mord sur les
tiers 1-2 du leader pendant ses 60 premières années.

---

## 5. Récapitulatif d'une ligne

| # | constat | chiffre | site |
|---|---|---|---|
| 1 | 3 nœuds inaccessibles à vie | 3/74, dont l'unique `triggers_crisis` | `scps_sim.c:1726` |
| 2 | le leader boucle l'arbre | 65 tech = 71/74 = 95 %, médiane sur 20 sims | `chronicle.c:2432` |
| 3 | production non plafonnée | ×3,03 (Σprod 1,65 + Σeff 0,38) | `econ_apply_country_tech` |
| 4 | accès tier-3 sans métaboliser | 1 groupe de pop suffit, coh ≥ 0,33 | `scps_ai.c:2233-2245`, `:2276` |
| 5 | remise de diffusion universelle | 70/74 nœuds soldés, −40 % max | `scps_ai.c:2434-2439` |
| 6 | héritage de l'arbre au cataclysme | 5 rég / 8k hab à 64 tech | `scps_sim.c:1556` |
| 7 | doctrine Technologie inatteignable | score ≈ 0,002 vs 2,4 | `scps_ai.c:3303-3304` |
| 8 | 4 idées de savoir sur 12 = clés sans site | `METAB_TIER12` inexistante | `scps_doctrines.c:67,118,120,121` |
| 9 | le revenu est le levier maître | `W=1` ⇒ 239 → 48 nœuds monde à l'an 120 | `scps_tune_list.h:315` |
| 10 | mais le prix mord aussi, et finement | prix ×1,43 ⇒ leader 49 → 31 tech (−37 %) | `scps_tune_list.h:265` |
| 11 | le nain ne rattrape jamais | ~270 ans pour UN tier-1 | `essai_s7_y200.log:716-718` |
| 12 | le thème FORGE n'est l'identité de personne | `0 Forge` dans 15 logs sur 20 | `chronicle.c:2434-2442` |
