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
- rallier le hameau WILD H (défection pacifique OU conquête — double voie).

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

Un pivot par branche, à mi-parcours (leçon 1.35/Anbennar) : p.ex. la branche du
Sol se scinde en **« L'Empire des marches »** (conquête, remises d'annexion,
gate Dominateur/Honneur) vs **« La Toile des serments »** (vassalités,
intégration, gate Bureaucrate/Pacifiste). Choix scellé par le joueur,
irréversible, l'autre voie s'éteint — rejouabilité entre graines ET entre
éthos.

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
| **Adopter une doctrine** | ~100 | ouvre la piste ; l'entretien mensuel en couronnes demeure (§4.2) |
| **Acheter une idée de doctrine** | 30-60 (croissant) | 6 par doctrine, en séquence — doctrine complète ≈ 300-400 |
| **Maintenir une synergie de paire** | **2/mois, puis 3·5·8… par synergie active supplémentaire** | le sink fibonaccien (§4.4) — suspendue si impayée ce mois |
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
- **6 slots maximum** sur toute la partie — on renonce à la moitié du
  catalogue. Un slot s'ouvre à l'avènement d'un âge où le joueur s'est
  **engagé** (motif `CMD_AGE_ENGAGE` + `year_eligible[]`, déjà persisté) :
  la doctrine est la récolte de l'âge vécu (8 âges possibles → 6 récoltes max).
- **Une paire opposée auto-exclusive** (motif décrets) : **Commerce ↔
  Mercantilisme** (libre-échange vs dirigisme). Gates d'éthos/héritage/
  religion (le créateur de culture décide de ce qui vous est pensable).
- **Abandon libre** (décision 2026-09-01) : on abandonne quand on veut, **sans
  remboursement et sans cicatrice**. Les idées achetées sont perdues, le slot
  se libère, les synergies qui reposaient dessus s'éteignent. Le frein naturel
  = tout re-payer si on revient.

### 4.2 Coût réel — l'influence achète, les couronnes entretiennent

- **Adoption** : **~100 d'influence** + un prérequis *réel* (famille
  d'édifices bâtie, palier tech, héritage métabolisé, N provinces d'un état
  donné). Ouvre la piste et son slot.
- **Les 6 idées** : achetées **en séquence**, ~30-60 d'influence chacune
  (croissant) — une doctrine complète ≈ 300-400, ~6-8 ans de génération.
  Chaque idée = un cran de mult nommé OU un morceau du verbe ; certaines
  portent en plus un prérequis d'usage (la 4e idée coloniale exige N colonies
  vivantes — la doctrine se prouve en jouant, leçon EU5).
- **Pas de bonus de complétion** : à 6/6, rien de spécial — la doctrine
  complète vaut par ses idées et rend ses paires éligibles aux synergies. Le
  verbe de la doctrine arrive donc comme une IDÉE du milieu de piste, pas en
  couronnement.
- **Entretien mensuel en couronnes** : le patron décret mot pour mot —
  `tax_year × RATE × IPM / 12`, contrat « non financé CE mois ⇒ mult = 1.0,
  sans effet CE mois ».
- **Coût d'opportunité** : slots limités + exclusivités + l'influence disputée
  avec la diplomatie et les synergies (§4.4) + l'assiette mensuelle partagée
  avec les décrets. Zéro création monétaire.

### 4.3 Catalogue (les 13 orientations dictées — chaque doctrine : leviers nommés + UN verbe)

| # | Doctrine | Gate | Leviers (mults nationaux sur le registre) | Le VERBE/la règle débloqué·e |
|---|---|---|---|---|
| 1 | **Offense** « Le Fer en avant » | Dominateur/Honneur + Caserne | entrée doctrine d'armée (dégâts/moral), `SIEGE_LOOT_FRAC`, coût de fabrication de CB réduit | posture « ost permanent » (renfort auto = déficit) |
| 2 | **Défense** « Le Bouclier des marches » | Garnison bâtie | `DEF_PER_H`, durées de siège subi, coût du bâti défensif | levée défensive instantanée (milice) quand on est envahi |
| 3 | **Commerce** « Les Routes franches » | Marché + tech Commerce | `TRADE_LEVY`, `COMMERCE_W_*`, portée du catchment | comptoir sur cité-état hôte (péage partagé) ; **exclut Mercantilisme** |
| 4 | **Mercantilisme** « L'Étape souveraine » | Entrepôt + tech Halles | `IMPORT_MARGIN_*`, `IMPORT_TOLL_FRAC`, bandes du stockeur, `BUILD_RESERVE_BULK` | embargo élargi + priorité du dispatch d'État ; **exclut Commerce** |
| 5 | **Peuple** « Le Creuset » | Adaptatif/Pacifiste | `ASSIM_*`, `POP_SAT_W`, accueil des réfugiés, pactes | pacte migratoire élargi (déporté → migrant) |
| 6 | **Colonisation** « L'Appel du large » | port bâti + tech Comptoirs | cadence coloniale, `COLONY_FOOD_GATE`, vitesse d'endurcissement climatique | **2e chantier colonial simultané** |
| 7 | **Diplomatie** « La Voix des cours » | Chancellerie | coûts d'influence réduits, `OPINION_*`, seuils d'acceptation d'offres | **second émissaire** (2 actions diplomatiques en vol) |
| 8 | **Vassaux** « La Toile des serments » | ≥ 1 vassal | `AI_VASSAL_CONTRIB_*`, vitesse d'intégration, annexion adoucie | imposer un contrat supérieur à la paix (servage→concordat…) |
| 9 | **Production** « L'Atelier du monde » | Fonderie + Outillage | `EXTRACT_*`, `RAW_BOOST_*`, recettes de manufactures | palier d'exploitation au-delà du plafond (`RAW_BOOST_MAX_TIER`+) |
| 10 | **Infrastructure** « La Pierre et l'eau » | Atelier de construction | durées/coûts d'édifices, `VETUSTE_RATE`, `RENOV_COST_FRAC`, `HOUSE_MANUF` | rénovation de masse (une file nationale de chantiers) |
| 11 | **Technologie** « Le Concile des lettrés » | Bibliothèque + Scriptorium | `SAVOIR_W_*`, `SAVOIR_LIB_*`, diffusion/catch-up | orienter la recherche (biais de quartier de l'arbre) |
| 12 | **Connaissances du monde** « Les Cartes et les langues » | Observatoire ou Amirauté | brouillard (rayon), vitesse de métabolisation, `SYNC_TRADE_*` | **expédition lointaine** (révèle une zone + ouvre un contact culturel) |
| 13 | **Faustien** « Le Pacte » | 1 tech ⚠ acquise | accès/coûts des nœuds faustiens, rendement des transmuteurs | l'échappatoire faustienne au choix (plus de refus IA) — la charge monte, **le prix est l'entropie** |

### 4.3bis Les COURANTS POLITIQUES (ajout joueur 2026-09-01) — un seul des quatre

Le motif EU4 des groupes « de gouvernement » (Aristocratic/Plutocratic/Divine),
assis sur nos strates réelles. **Quadruple auto-exclusif** : on n'épouse qu'un
courant. Sa propriété centrale : **il choisit l'ASSIETTE de l'influence** —
quelle classe porte ta voix — avec un taux propre (peu de nobles puissants, ou
beaucoup de petites voix) :

| # | Courant | Assiette d'influence | Leviers (mults nationaux) | Le VERBE |
|---|---|---|---|---|
| 14 | **Aristocratique** « Le Sang et la Terre » | élites ×0.0025 (la voix pleine) | contribution vassale, commandement (doctrine d'armée), loyauté du Conseil | **adoubement** : promouvoir des bourgeois en élites (transfert de strate contrôlé) |
| 15 | **Bourgeoise** « La Charte des villes » | bourgeois ×0.0006 | `CREDIT_LINE_BASE`/taux, `COMMERCE_W_BOURGEOIS`, `PROMOTE_BASKET_MULT` (accession facilitée) | **emprunt intérieur élargi** (la classe prête à l'État au-delà de la ligne) |
| 16 | **Populaire** « La Voix du grand nombre » | journaliers ×0.00012 | `POP_SAT_W`, `W_AGITATION_UNREST` (relief), concession moins chère | **levée en masse** (conscription au-delà du plafond, contre agitation) |
| 17 | **Divin** « Le Trône et l'Autel » | ∝ foi bâtie × ferveur (pas une classe : l'Église) | conversion, ferveur, entretien du bâti de foi, cap religion | **appel à la foi** (mobilise la ferveur : stabilité ou zélotes, au choix) |

Le courant occupe un slot de doctrine comme les autres (proposé — à
confirmer) ; gates d'éthos évidents (Dominateur→Aristocratique,
Mercantile→Bourgeoise, Pacifiste→Populaire, w_faith→Divin), mais non exclusifs
— on peut jouer contre son éthos, plus cher.

Tous les effets = `tune_f(...) × doctrine_mult(cid, ...)` aux sites de lecture
existants ; **aucune variable fantôme**. Le verbe = un `CMD_*` ou un gate
élargi, revalidé au drain. Compte final : **17 au catalogue** (13 orientations
+ 4 courants), deux exclusivités (Commerce↔Mercantilisme · un seul courant),
**6 slots**.

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

- **Mêmes règles** : l'IA génère l'influence (ses élites × son Conseil — à
  vérifier : si le Conseil est joueur-seul aujourd'hui, fallback déterministe
  `mult = f(éthos)`), paie l'adoption, paie l'entretien, subit les gates et
  l'exclusion Commerce↔Mercantilisme.
- **Choix par personnalité** : poids d'adoption dérivés de l'éthos/héritage
  (Dominateur → Offense/Vassaux, Mercantile → Mercantilisme/Commerce,
  Pacifiste → Peuple/Diplomatie, Ésotérique → Technologie/Faustien…), départagés
  par le besoin réel (motif demande-driven existant). Déterministe (xs32).
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
  existe. Entretien affiché **en couronnes/mois** (jamais l'annuel).
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
| **P3** | Doctrines moteur (`doctrine_mult` cloné de `decree_mult`, slots par âge, adoption en influence, entretien couronnes, maturation) **+ adoption IA par personnalité (§4.5)** + 4e sous-onglet Conseil. | mêmes gates + **re-baseline golden + sweep apparié 3×3 — le joueur lance** |
| **P4** | Desseins IA (biais `ai_province_value`/colonisation vers la cible du dessein courant) + calibrage d'ensemble. | re-baseline + sweep apparié — le joueur lance |

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
