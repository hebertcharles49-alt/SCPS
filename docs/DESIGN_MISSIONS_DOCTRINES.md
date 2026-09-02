# DESIGN — Desseins (missions), Influence politique & Doctrines

Proposition 2026-09-01, à discuter avant tout code. Sources : wiki EU4 (missions,
claims, idea groups, réformes, religions, âges), Anbennar (arbres narratifs,
missions coloniales, machine à états aventurier→royaume), Stellaris (traditions,
ascension perks, civics, désignations), et la cartographie complète des crochets
moteur SCPS (agent 2026-09-01).

---

## 0. Les six leçons retenues du corpus

1. **Un verbe > un pourcentage.** Les spécialisations mémorables débloquent des
   actions ou changent des règles (Exploration EU4, Deus Vult, Catalytic
   Stellaris). La « soupe de modificateurs » est le repoussoir unanime.
2. **La bonne mission est directionnelle, pas transactionnelle.** Elle dit *où
   aller* et rend le chemin praticable (claim sur le jalon N+1) ; elle ne paie
   jamais le joueur en monnaie brute pour un état déjà atteint (« click for
   free stuff »).
3. **La grammaire EU4/Anbennar** : condition = prédicat d'état réel → récompense
   = revendication sur l'étape suivante + un coup de pouce **daté** (15-25 ans)
   à l'étape en cours. La cascade donne le tempo de toute la campagne.
4. **Nommer et raconter chaque récompense** (Anbennar : « The New Bloodwine »).
   Une carte qui garde la trace de son histoire (modificateurs provinciaux
   nommés) transforme la checklist en chronique.
5. **L'exclusivité fait le choix** (Stellaris : 7 arbres sur 15, coût
   superlinéaire). Adoption + finisher = deux hameçons. La spécialisation
   débloque la spécialisation (traditions → perks → ascension planétaire).
6. **Anti-railroading** : nos mondes sont procéduraux — c'est notre chance.
   Pas d'arbre écrit à la main par pays : des **gabarits** instanciés sur la
   géographie réelle de la graine. Chaque monde écrit un arbre différent.

---

## 1. L'existant (on étend, on ne crée pas)

- **`scps/scps_missions.{c,h}` existe déjà** : mission décennale du Conseil
  (`MIS_BUILD/CHAIN/TECH`), un siège responsable, récompense/échec → loyauté,
  section save `MISS`, reader `scps_mission_info` **déjà bindé Godot** (aucun
  .gd ne l'appelle). **Décision 2026-09-01 : la commission décennale DÉGAGE**
  (« engagements décennaux chiants ») — les Desseins la remplacent dans le
  même module. Ripple à traiter à la dépose : (a) **l'Âge des Héros** naît
  aujourd'hui d'une mission décennale réussie (`ages_hero_fire`,
  `sim.c:1487`) → ré-ancrer sur le scellage d'un échelon de Dessein avec
  Conseil de rang III ; (b) la boucle loyauté du Conseil
  (`COUNCIL_MISSION_*_LOYALTY`) → les Desseins la reprennent (le siège
  responsable du thème gagne/perd) ; (c) `missions_demo.c` réécrit ; (d) la
  dépose touche l'IA (loyauté/récompenses) ⇒ **re-baseline golden dès P1**.
- **`EventDef.trigger`** (scps_events.h) = le patron exact d'une condition de
  mission : prédicat `(ctx, sujet) → bool` sur l'état réel.
- **`EvEffect`** = la liste close des canaux d'effet (coordonnées, jamais un
  bonus fantôme) ; `d_treasury_mois` = le canal monétaire propre (fraction du
  revenu mensuel, déflatée IPM).
- **`decree_mult(cid, id, mult)`** = LE patron d'un effet national : le site de
  lecture moteur applique `tune_f("CLÉ", défaut) × mult(pays)`. Jamais
  `tune_set` (global, IA comprise).
- **`fab_region[a][b]` + `diplo_claim_region`** = la revendication *nommée*
  existe déjà ; une mission peut la poser sans payer les 2 ans de revenu de
  fabrication.
- **`AgesState.year_eligible[]` + `CMD_AGE_ENGAGE`** = le précédent exact d'un
  « slot qui s'ouvre à un âge et que le joueur choisit une fois ».
- **Annales/Fil** : `ANNAL_*` appendable, `FeedKind` extensible en 3 gestes,
  et le 12e genre de journal livré (`jrn_*`) attend justement un `kind`.

---

## 2. LES DESSEINS — l'arbre d'ambitions généré

### 2.1 Forme

À la genèse, chaque pays reçoit **3 branches** tirées de son contexte réel
(éthos, héritage, géographie, voisins, religion) — déterministe (graine + cid) :

- **La branche du Sol** (toujours présente) : consolidation → marches →
  hégémonie régionale. Territoriale, la colonne vertébrale.
- **Une branche d'ouverture** selon la géographie : **Mer & Comptoirs**
  (côtier/insulaire) ou **Routes & Caravanes** (continental).
- **Une branche d'esprit** selon la culture : **Foi** (w_faith haut), **Savoir**
  (Ésotérique/Mécaniste), **Creuset** (Adaptatif), **Horde** (Clanique
  dominateur)…

Chaque branche = **6-8 échelons** en cascade (l'échelon N accompli révèle et
arme le N+1), instanciés sur des objets *nommés* du monde : cette province-ci,
ce détroit-ci, ce hameau-ci, cette culture voisine-là. Les 3 branches avancent
**en parallèle** (EU4), sans échec ni date limite (le Dessein attend).
Complétion détectée à la clôture mensuelle ; le joueur **scelle** l'échelon
(un clic — c'est là qu'il lit ce qu'il gagne, et choisit quand il y a choix).

### 2.2 Grammaire d'un échelon

```
GABARIT  = { éligibilité (contexte genèse), cible (résolue sur le monde réel),
             trigger (prédicat état réel), récompense (canaux §2.4),
             textes STR_* (gabarits %s : noms réels injectés) }
```

Conditions type (tout est déjà lisible dans le moteur) :
- posséder la province P (`econ->prov[pid].owner`, jamais `region[].owner`) ;
- occuper la région R en guerre (`dp->occupier[]`) ;
- N colonies fondées dans la zone Z (latch `is_colonized` + owner) ;
- grenier ≥ X mois sur P (`econ_colony_food_ok`) ; catchment marché ≥ N ;
- héritage de C métabolisé (`econ_country_metabolized`) ; temple T2 bâti ;
- vassaliser V ; gagner la guerre contre le rival désigné (rancune max) ;
- rallier le hameau WILD H (conquête OU vassalité imposée à la paix —
  `WILD_DEFECT_YEARS=0` par décision joueur : un hameau ne se rallie ni ne se
  soumet jamais seul ; corrigé par l'annexe Desseins D2).

### 2.3 La branche coloniale (le modèle détaillé)

L'échelle géographique explicite, chaque barreau nommé (Portugal/Lorent) :

1. **« Les éclaireurs »** — révéler N provinces au-delà du brouillard →
   rayon de vision étendu (canal `ages_fog_radius_add`, joueur seul).
2. **« La première fondation »** — 1 colonie dans la zone frontière Z1 (choisie
   par BFS habitabilité depuis la capitale) → **Ferveur fondatrice renforcée**
   (mult national sur `PROVMOD_FERVEUR_K`, 15 ans) + désignation de Z2.
3. **« Le grenier de la mer »** — nourrir une colonie par le commerce (grenier
   ≥ `FOOD_STOCK_MONTHS`) → mult national `COLONY_FOOD_GATE` ×0.8 (la logistique
   apprise).
4. **« S'endurcir au climat »** — N habitants pleinement intégrés d'un climat
   étranger → **le bitmask `Country.climates` s'étend** : le désert/la jungle
   s'ouvrent. C'est notre « colonial range », et c'est déjà une entrée moteur.
5. **« La seconde vague »** — N colonies vivantes → cadence : mult national sur
   le cooldown de chantier (`cd_days` ×0.7).
6. **« Par-delà les mers »** — 1 colonie outre-mer → le surcoût ×2.0 devient
   ×1.5 (mult national sur le coût pop outre-mer).
7. **Parachèvement « Un monde nouveau »** — X % de la masse continentale visée
   → modificateur provincial permanent nommé sur la capitale coloniale
   (« Porte des Indes » : PE_infra bâtie) + Annale + épithète candidate.

### 2.4 Typologie des récompenses (et la règle d'or)

**JAMAIS d'or créé.** Aucune récompense ne crédite le trésor ex nihilo. La
monnaie n'apparaît que : (a) en **coût** ; (b) en **transfert réel** (tribut de
paix, prise — il sort de chez quelqu'un). Le reste :

| Canal | Exemple | Ancrage moteur |
|---|---|---|
| **Revendication nommée** | claim sur la marche suivante, sans coût de fabrication | `diplo_claim_region` / `fab_region[][]` |
| **Remise structurelle datée** | annexion −X %, cicatrice d'annexion adoucie, 20 ans | mult national sur `AI_ANNEX_*` / `ANNEX_SOFT_SCAR` |
| **Modificateur provincial nommé** | « Ferveur unificatrice », « Porte des Indes » | le slot MODIFICATEURS existant (motif ferveur/reconstruction) |
| **Coordonnée bâtie** | +K_inst/+PE_infra sur UNE province nommée | `ProvBuild` (le canal des édifices) |
| **Déverrouillage** | palier tech, climat, outre-mer, 2e chantier | `unlock_branch/tier`, `climates`, gates colonisation |
| **Cadence** | cooldown diplo/colonial réduit | mult national sur les `*_CD_DAYS` |
| **Personnage** | ministre nommé au Conseil (rang fixé) | conseil existant (motif Pizarro/Endral) |
| **Mémoire** | Annale, épithète, ligne du Fil | `ANNAL_MISSION`, `FeedKind`, `jrn_mission` |

Bonus plats **assumés comme gameplay** — mais ils passent tous par les leviers
du registre (docs/LEVIERS.md) via le patron `decree_mult`, portée pays, souvent
**datés** (15-25 ans, motif EU4 sain), et nommés face joueur.

### 2.5 Pivots exclusifs

Un pivot par branche, à mi-parcours (leçon 1.35/Anbennar) : p.ex. la branche
du Sol se scinde en **« L'Empire des marches »** (conquête, remises
d'annexion) vs **« La Couronne des serments »** (vassalités, intégration —
renommée : « La Toile des serments » est la doctrine Vassaux, annexe Desseins
D2/D4). Révision joueur 2026-09-01 : **prix de pivot UNIFORME (20
d'influence), aucune modulation d'éthos** — pas de gate. Choix scellé par le joueur,
irréversible, l'autre voie s'éteint — rejouabilité entre graines ET entre
éthos. Le contenu complet des 7 branches : docs/DESIGN_DESSEINS_ANNEXE.md.

### 2.6 Déterminisme, golden, save

- Génération à la genèse : `xs32(seed, cid)` — byte-identique au rejeu.
- **P1 joueur seul** : détection/scellage gatés `human_player >= 0` → golden
  intact **par construction** (motif décrets/pending).
- État nouveau (échelons, cibles résolues, latch scellés) dans `MissionsState`
  → `sizeof` change → **bump SAVE_VERSION** ; `save_sane` revalide cibles
  (pid/région bornés, gabarit connu).
- Cible invalidée (province engloutie, cédée à un allié) : re-résolution
  déterministe au tick, jamais un échec silencieux.

### 2.7 L'IA et les Desseins (P2, décision séparée)

La critique EU4 n° 1 : l'IA n'accomplit pas ses arbres. Notre IA est
demande-driven — le raccord propre n'est PAS un script mais **un biais
d'entrée** : la cible du dessein courant pèse dans `ai_province_value` et dans
le choix de zone de colonisation. Impact golden certain → vague à part,
re-baseline documenté, sweep apparié 3×3 (le joueur lance).

---

## 3. L'INFLUENCE POLITIQUE — la monnaie du jeu politique

Décision joueur 2026-09-01 : il FAUT une ressource politique dédiée. Mais pas
un mana abstrait tombé du ciel — une ressource **endogène**, dérivée des pops
simulées (la correction EU5 du point de monarque EU4).

### 3.1 Génération

```
influence/mois = 0.002 × nobles(pays) × mult_conseil
```

- **`nobles`** = l'effectif réel de la classe **Élite** (les strates existent
  par province ; somme nationale déjà agrégée pour la topbar). C'est
  l'assiette **par défaut (aristocratique)** — un courant politique adopté
  (§4.3bis) peut RE-SEOIR l'assiette sur une autre classe.
- **`mult_conseil`** = le niveau des conseillers : rang moyen des ministres en
  siège (I → ×1 … IV → ×4 ; siège vide compte 0 dans la moyenne). Le Conseil
  n'est plus seulement un décor de loyauté : il est le multiplicateur du jeu
  politique.
- Ordre de grandeur : ~1000 élites × 0.002 × rang II ≈ **4/mois**, ~50/an.
  Tunables registre J : `INFLUENCE_PER_NOBLE` (0.002), `INFLUENCE_CAP`.
- **Pas de plafond de stock pour l'instant** (décision 2026-09-01) — le
  tunable `INFLUENCE_CAP` existe (0 = sans plafond) si l'équilibrage le
  réclame un jour ; les sinks (synergies fibonacciennes) font le travail.

### 3.2 La boucle systémique (pourquoi c'est du SCPS pur)

Rien n'est gratuit en amont : les élites **coûtent** déjà — panier de luxe
(fourrure, orfèvrerie, statuaire), et la rivalité turchinienne
aspirants/positions (vague culture vivante) fait qu'une noblesse gonflée
déstabilise. Vouloir plus d'influence = nourrir plus de nobles = plus de
demande de luxe et plus de pression élitaire. La monnaie politique a un prix
économique et social réel, sans aucune création magique.

Écarté pour l'instant (décision 2026-09-01) : pas de modulation par la
satisfaction de la classe Élite. Le taux est plat : effectif × rang, point.

### 3.3 Ce que l'influence paie (les verbes du « jeu », pas la gestion)

| Dépense | Coût indicatif | Note |
|---|---|---|
| **Envoi de diplomate** (alliance, pacte, embargo, migration, paix offerte…) | 10-15 | **DÉCIDÉ : le coût REMPLACE le cooldown `diplo_ready_day`** — on enchaîne si on a économisé, on est muet à sec (plancher court anti-spam conservé) |
| **Fabriquer une revendication** | 20-30 | en sus du coût d'or existant (2 ans de revenu de la cible) — l'intrigue mobilise la cour |
| **Sceller un pivot exclusif de Dessein** | 15-25 | le choix d'orientation est un acte politique ; les échelons ordinaires restent gratuits |
| **Adopter une doctrine** | 50 + 25 × doctrines actives | coût fixe et scalable (révision 2026-09-01) |
| **Entretenir une doctrine** | **1/mois chacune** | l'entretien est EN INFLUENCE (pas en couronnes) — insolvable ce mois = les dernières adoptées se suspendent |
| **Acheter une idée de doctrine** | 30 + 3 × idées possédées (total) | façon unités Stellaris — plus t'en as, plus c'est cher ; 6 par doctrine, en séquence |
| **Maintenir une synergie de paire** | **1re GRATUITE, puis 2·3·5·8… par synergie active supplémentaire** | le sink fibonaccien (§4.4, révision 2026-09-02) — suspendue si impayée ce mois ; LE SEUL entretien du système (les doctrines coûtent flat) |
| **Décision ponctuelle de décret** (`DCR_DECISION`) | 10-20 | l'audit des offices mobilise l'appareil |
| **Soutenir une fronde chez autrui** (futur verbe) | 30+ | le trou identifié §2 diplo — l'influence est sa monnaie naturelle |

Ne coûtent JAMAIS d'influence : la gestion provinciale (allocation,
construction, colonisation, budget) — c'est de la gouvernance, pas du jeu
politique. La frontière est nette et lisible.

### 3.4 Technique

- Accumulateur inter-ticks par pays ⇒ **sérialisé** (jurisprudence
  EMOB/COLC/TXYR), revalidé `save_sane` (borné [0, cap]).
- P1 : génération et dépense **joueur seul** (`human_player`) → golden intact
  par construction. L'IA garde ses cadences propres (ses coûts sont déjà
  modelés par `AI_*`) ; symétrie IA éventuelle en P4 avec re-baseline.
- Membrane : un entier + « /mois » (jamais le calcul). Affichage : en-tête du
  Conseil + hover détaillé (nobles × taux × rang) ; les boutons qui en coûtent
  affichent le prix (motif checklist). Topbar : pas de 10e cellule pour
  l'instant — à réévaluer si la ressource devient centrale au quotidien.

---

## 4. LES DOCTRINES — la spécialisation profonde

### 4.1 Forme (décisions 2026-09-01)

- **Catalogue 17 doctrines** : 13 orientations (§4.3 — « guerre » est le
  chapeau d'Offense + Défense) + 4 courants politiques (§4.3bis). Chacune :
  **adoption → 6 IDÉES achetées en séquence**. **Compléter ne donne rien de
  spécial** (décision 2026-09-01) : la valeur, ce sont les 6 idées elles-mêmes
  — et l'éligibilité aux synergies de paires (§4.4).
- **6 slots, LIBRES d'entrée** (révision joueur 2026-09-02 — l'ouverture par
  âges engagés est SUPPRIMÉE) : le frein est purement économique — **la
  dépense est linéarisée sur la pop de nobles** : tous les coûts (adoption,
  idées, entretien) × `é = assiette de génération / INFLUENCE_BASE_REF (2.0)`,
  calculée SANS le multiplicateur du Conseil (anti-exploit), plancher 0.25.
  In fine le temps d'acquisition est constant quelle que soit la taille de
  l'empire — mais le joueur a l'impression de liberté.
- **Une paire opposée auto-exclusive** (motif décrets) : **Commerce ↔
  Mercantilisme** (libre-échange vs dirigisme). **Aucun autre gate** (révision
  joueur 2026-09-01) : ni éthos, ni héritage, ni religion — toutes les
  factions peuvent tout prendre.
- **Abandon libre** (décision 2026-09-01) : on abandonne quand on veut, **sans
  remboursement et sans cicatrice**. Les idées achetées sont perdues, le slot
  se libère, les synergies qui reposaient dessus s'éteignent. Le frein naturel
  = tout re-payer si on revient.

### 4.2 Coût — fixe et scalable, façon unités Stellaris (révision joueur 2026-09-01)

**Pas de gate, pas de prérequis : tu cliques, t'as le bonus** (le modèle EU4).
Ni prérequis d'adoption (édifice/tech), ni prérequis d'usage sur les idées,
ni gate d'éthos — supprimés partout. Le frein est le COÛT, qui monte avec ce
qu'on possède déjà :

- **Adopter une doctrine** : `DOCT_BASE (50) + DOCT_STEP (25) × doctrines
  actives` — 1re = 50, 6e = 175.
- **Acheter une idée** : `IDEA_BASE (30) + IDEA_STEP (3) × idées possédées,
  toutes doctrines confondues` — 1re = 30, 36e = 135. Les 6 idées restent
  achetées EN SÉQUENCE dans leur doctrine. Abandonner libère le compte.
- **Pas de bonus de complétion** (6/6 = rien) ; la paire complète ouvre le
  sous-menu de synergie (coût fibonaccien inchangé).
- **AUCUN ENTRETIEN de doctrine** (révision joueur 2026-09-02, remplace celle
  du 09-01) : les doctrines coûtent en FLAT — l'achat, rien d'autre. La
  mécanique de suspension mensuelle est SUPPRIMÉE (elle affamait l'IA : les
  8 400 pays-mois suspendus de la mesure P3-IA étaient un défaut de design,
  pas d'IA). **Seules les SYNERGIES paient un entretien**, fibonaccien —
  **et la PREMIÈRE synergie active est gratuite** : 0 · 2 · 3 · 5 · 8… /mois
  par rang d'activation. Les DÉCRETS gardent leur entretien en couronnes.
- **L'IA choisit PAR SCORE** sur ses propres modificateurs (côtier →
  Colonisation, beaucoup de vassaux → Vassaux…) — aucune restriction.

### 4.3 Catalogue — noms nus, zéro gate (révision joueur 2026-09-01)

**Les 13 orientations** : Offense · Défense · Commerce · Mercantilisme ·
Peuple · Colonisation · Diplomatie · Vassaux · Production · Infrastructure ·
Technologie · Connaissances du monde · Faustien. **Les 4 courants
politiques** (un seul à la fois — le courant re-siège l'ASSIETTE de
l'influence sur sa classe) : Aristocratie (élites ×0.0025) · Bourgeoisie
(bourgeois ×0.0006) · Populaire (journaliers ×0.00012) · Divin (foi bâtie ×
ferveur). Le courant occupe un slot comme les autres.

Plus aucun sous-titre d'apparat, plus aucun gate d'éthos ni prérequis : tout
le monde peut tout prendre, l'IA choisit par score. Chaque doctrine = 6 idées
(dont ≥ 1 verbe **V**), bonus affiché en UNE ligne lisible (« +30 % de portée
du marché »), effet moteur = `tune_f × doctrine_mult(cid)` au site de lecture
(aucune variable fantôme), verbes revalidés au drain. **Le catalogue complet
idée par idée : docs/DESIGN_DOCTRINES_ANNEXE.md.**

### 4.4 Les SYNERGIES de paires (décision joueur 2026-09-01)

Deux doctrines **complétées** (6/6) ouvrent un **sous-menu de paire** : le
bonus exclusif lié à cette combinaison y est **proposé à la dépense** — le
motif policies d'EU4, et le précédent maison des **combos de paires
d'héritages T4** de l'arbre tech (« accès aux DEUX → l'alliage »). Exemple
canon (le joueur) : Commerce + Aristocratique complétées ⇒ « **Maison de
commerce** » proposée.

- **Activable/désactivable**, entretenue en influence/mois, avec un **coût
  d'empilement fibonaccien** (décision 2026-09-01) : la 1re synergie active
  coûte **2/mois**, la 2e **3**, la 3e **5**, la 4e **8**… (chaque suivante =
  la somme des deux précédentes, « n + n−1 »). Le rang de coût suit l'ordre
  d'activation ; en désactiver une fait redescendre les suivantes d'un rang.
- À ~4/mois de génération de base, une synergie se tient tôt, deux se
  méritent, quatre exigent un vrai État (18/mois) — le sink superlinéaire qui
  empêche la thésaurisation ET l'empilement (l'anti-« modifier soup »
  structurel).
- Non financée CE mois ⇒ suspendue CE mois (le contrat décret, encore).
- Pas de plafond dur du nombre de synergies actives : la suite de coûts EST le
  plafond.
- Contenu : pas d'auteur pour C(17,2) paires — on n'écrit que les paires
  parlantes (~15-20 nommées : Commerce×Aristocratique « Maison de commerce »,
  Offense×Vassaux « Les Marches d'épée », Colonisation×Connaissances « Les
  Grandes Découvertes », Mercantilisme×Production « La Manufacture d'État »,
  Divin×Offense « La Guerre sainte », Populaire×Infrastructure « Les Grands
  Travaux »…) ; une paire sans synergie écrite n'affiche rien.

### 4.5 Couplage Desseins ↔ Doctrines

- Certains échelons **exigent** une doctrine (la branche coloniale profonde
  demande L'Appel du large) ; le parachèvement d'une branche **ouvre un slot**
  de doctrine en avance (la spécialisation débloque la spécialisation).
- Le pivot exclusif d'une branche peut être conditionné par la doctrine tenue —
  l'identité se répond d'un système à l'autre.

### 4.6 L'IA et les doctrines (décision 2026-09-01 : OUI, dès l'atterrissage)

L'« arbre de tech parallèle » est pour tout le monde — pas un jouet solo :

- **Mêmes règles** : l'IA génère l'influence (ses élites × son Conseil —
  vivier déjà multi-classes v100), paie l'adoption, paie l'entretien, subit
  les seules exclusivités (Commerce↔Mercantilisme, un courant).
- **Choix PAR SCORE** (révision joueur 2026-09-01) : l'IA note chaque
  doctrine sur SES PROPRES modificateurs — état réel, pas personnalité :
  côtier/colonies en cours → Colonisation, vassaux tenus → Vassaux, gros
  commerce → Commerce ou Mercantilisme, guerres fréquentes → Offense… —
  départagé par le besoin (motif demande-driven existant). Déterministe.
- **Garde-fou faustien** : l'IA n'adopte « Le Pacte » que sous le seuil
  `FAUST_BRECHE_CAUTION` déjà en place.
- Conséquence assumée : **impact golden dès la vague doctrines (P3)** —
  re-baseline documenté + sweep apparié 3×3 pour vérifier que les profils
  d'adoption diversifient les trajectoires sans casser l'équilibre.

---

## 5. Façade (charte UI, ≤ 3 clics, /mois, MOTS)

- **Desseins** : page **`empire_window`** (« Desseins » à côté
  d'Économie/Population/Diplomatie/Conseil) — le contexte est national. Les 3
  branches en colonnes, échelon courant en tête, détail en hover ; conditions
  affichées en **checklist `ScpsGateCond[]`** (contrat `ET(conds)==allowed`).
  Alerte du Fil au scellement possible (`jrn_mission`, le 12e genre livré).
- **Doctrines** : **4e sous-onglet du Conseil** (« Gouvernement · Politiques ·
  Factions · Doctrines ») — zéro composant neuf, le patron `_conseil_tab`
  existe. Entretien affiché **en influence/mois** (doctrines + synergies), à
  côté du revenu — le solde politique se lit d'un coup d'œil.
- **Influence politique** : en-tête du Conseil (stock + « /mois »), hover =
  nobles × taux × rang du Conseil en mots ; chaque bouton payant affiche son
  prix d'influence à côté du reste de sa checklist.
- Membrane : readers POD (`ScpsDessein`, extension `ScpsMission`), textes
  `STR_*` FR/EN, effets en mots+signes (motif `effets[]` des dilemmes).
- Assets (campagne Codex 3, plus tard) : médaillons de branche, encarts
  d'échelon (motif encarts tech 2:1), 10 icônes de doctrine, chrome de page.

---

## 6. Phasage & gates

| Phase | Contenu | Gates |
|---|---|---|
| **P1** | **Dépose de la commission décennale** (ré-ancrage Âge des Héros + loyauté Conseil sur les Desseins) + **Influence politique** (génération élites × Conseil, stock sérialisé, coûts sur les verbes diplo — le cooldown `diplo_ready_day` saute) + Desseins moteur : gabarits + génération + triggers + scellage + récompenses. Desseins/influence joueur seul. | bump SAVE_VERSION · `desseins_demo` banc neuf (remplace `missions_demo`) · full-test 40 · savetest (influence + échelons sérialisés) · determinism · **re-baseline golden documenté (la dépose touche l'IA)** · lang-check |
| **P2** | Façade : readers + page empire_window Desseins + influence au Conseil + checklist + Fil/Annales UI. | probes visuelles · lang-check |
| **P3** | Doctrines moteur (`doctrine_mult` cloné de `decree_mult`, slots par âge, adoption ET entretien en influence) **+ adoption IA par score (§4.6)** + 4e sous-onglet Conseil. | mêmes gates + **re-baseline golden + sweep apparié 3×3 — le joueur lance** |
| **P4** | Desseins IA (biais `ai_province_value`/colonisation vers la cible du dessein courant) + calibrage d'ensemble. | re-baseline + sweep apparié — le joueur lance |

**P3-IA — atterrissage (2026-09-02, `docs/BRIEF_P3_IA_CHRONICLE.md`)** :
l'adoption IA par score est LANDÉE (`ai_doctrines_year`, scps_ai.c), la
génération d'influence est passée à TOUS les pays vivants (seule la dépense
diplomatique reste gatée joueur), et le chronicle porte la télémétrie + les
corrélations-juges. **Re-baseline golden ACTÉ** (5 graines × 12 ans) —
| | 7 | 108 | 209 | 310 | 411 |
|---|---|---|---|---|---|
| **avant** | `9fa6ff52` | `f96da1f0` | `545c5872` | `6f979e33` | `fd265618` |
| **après** | `fa02fe96` | `f96da1f0` | `545c5872` | `8902b118` | `ff64ee5a` |

Trois graines sur cinq bougent (deux mondes n'ont aucun adoptant dans la
fenêtre de 12 ans) ; **`SCPS_TUNE=AI_DOCT=0` rend EXACTEMENT la colonne
« avant »** — la preuve que la dérive est ENTIÈREMENT l'adoption IA et rien
d'autre. Le **sweep apparié 3×3** (`tools/sweep_doct_ai.sh`) est écrit et prêt
— **le joueur le lance**.

Pièges déjà consignés à respecter : `region[].owner` dérivé (lire
`prov[pid].owner`) · enum appendus en fin + grep des boucles `*_COUNT` ·
`SCAR_NONE=0`-style pour tout nouvel enum à défaut implicite · Makefile ripple
(~33 sites si module neuf — préférer étendre `scps_missions.c`) · jamais
`tune_set` pour un effet national.

---

## 7. Décisions actées (2026-09-01) & questions restantes

**Acté par le joueur :**
- Noms conservés : Desseins · Influence politique · Doctrines.
- Influence : 0.002/noble × niveau des conseillers ; le coût **remplace** le
  cooldown diplo ; **pas** de modulation par satisfaction pour l'instant.
- Doctrines : **6 slots**, catalogue = 13 orientations (guerre = chapeau
  Offense+Défense) + **4 courants politiques** (Divin · Bourgeoise ·
  Aristocratique · Populaire, un seul des quatre, re-siègent l'assiette
  d'influence) = **17**.
- **Chaque doctrine porte 6 idées** achetées en séquence ; **complet = rien de
  spécial** ; **la paire complétée ouvre un sous-menu** où la synergie
  exclusive est proposée à la dépense — coût d'empilement fibonaccien
  2/3/5/8… par synergie active (§4.4).
- **Abandon libre** : pas de remboursement, pas de cicatrice ; le slot se
  libère.
- Les 6 slots se remplissent avec tout le catalogue, courants compris (lecture
  de « 6 slots à occuper, avec leur division »).
- **L'IA utilise l'arbre de doctrines** (P3, re-baseline assumé).
- **La commission décennale dégage** (les Desseins la remplacent).

**Acté 2026-09-01 (2e passe) :** pas de plafond d'influence pour l'instant ·
ré-ancrage de l'Âge des Héros sur les Desseins OK · vérif Conseil-IA en début
de P1 OK · **annexe de contenu lancée** (un agent par doctrine) →
docs/DESIGN_DOCTRINES_ANNEXE.md.
