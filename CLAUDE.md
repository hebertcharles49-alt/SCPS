# SCPS — conventions du dépôt

## Build & vérification

- `make` → `core_demo` (35 contrôles auto-vérifiés, sortie ≠ 0 si échec).
- `make <x>_demo` : chaque module a son banc auto-vérifiant — **tout doit rester vert**. `ai_demo` : 23/23 depuis la graine par défaut 9 (L4-2026-06 ; le test « Bâtisseur +K » reste sensible au monde, documenté en AUDIT.md).
- `make chronicle && ./chronicle <graine> <sims> <années> [empires] [cités] [continents]` : le balayage headless — la télémétrie est la preuve d'équilibre.
- `make asan && ./chronicle_asan …` : ASan+UBSan doivent rester muets.
- **Re-baseline GR4 (2026-06)** : l'HÉRITAGE (ex-race) est DÉBRAYÉ de la géo — assigné
  par hash déterministe (graine × index pays), décorrélé du biome. Le biome décide
  toujours TERRAIN & RESSOURCES, plus QUEL héritage s'installe (« orques des forêts »
  possibles). Les noms SUIVENT l'héritage (country/region_make_name lisent culture.race).
  ⚠ Les mondes des graines de référence ONT CHANGÉ (qui porte quel héritage change ;
  chronique re-baselinée). GR1-GR3 (reskin race→héritage, religion→idéologie) sont PURS
  (hash chronique IDENTIQUE) ; SEUL GR4 bouge le monde.
- **Re-baseline L4 (2026-06)** : la genèse distribue désormais les empires entre
  continents (≥15 % habitable ⇒ ≥1 empire) — les mondes des graines de référence ONT
  CHANGÉ (qui est empire change). Mondes fendus (p.ex. graines 1/42) : H3 active la
  guerre trans-mer (2026-06) — calibrer la guerre sur une graine mono-continent (7/99).
- **Re-baseline Q6 (2026-06-14) — la capacité VIENT DU DÉVELOPPEMENT (48k→96k)** : monde
  âgé (`world_age=0.7` : la Pangée FEND, la mer s'éveille), genèse à **6 empires + 12
  cités-états**. An-0 = **48 000** hab, distribution **UNIFORME** (même pop par région,
  sous le plancher) ; à l'an-100 ça DIVERGE (le bâti). La pop ne croît plus vers un apex
  figé mais vers ce que le **BÂTI porte** : `eff_cap = ½·cap_pop` (la terre nue) **+
  LOGEMENTS bâtis** (manufactures UNIQUEMENT, `+HOUSE_MANUF`/niveau, plafond ½·cap_pop ≈
  25 ateliers·100) **+ grenier** (qui garde son rôle NOURRITURE). `cap_pop` = la taille
  PLEINE nourrie (le socle vivrier suit `cap_pop`). **Bâtir double la région ½→plein** ⇒
  le monde passe de 48k à ~96k au siècle par la seule CONSTRUCTION (la pop SUIT le bâti ;
  aucun taux de croissance touché ; la guerre redistribue). Tunables (registre J) :
  `EMPIRE_CAP` 10300 / `CITY_CAP` 5150 (taille nourrie) · `HOUSE_MANUF` 100 · `SEED_POP`
  48000. `cap_pop_sum` exact (zones mortes tranchées en Passe 1, RÉUTILISÉES en Passe 3).
  Le **readout** (viewer) reflète l'eff_cap moteur : les logements MONTENT quand on bâtit.
  ⚠ Pop & taille des pays ≫ qu'avant ; `ai_demo` a deux contrôles SENSIBLES AU MONDE
  (aggression/routes réalisées) rendus ROBUSTES. Diag : `SCPS_CAPDIAG` (`Σmanuf_lvl`).
  **#5 (étape 1) — les cités-états TIENNENT le marché mondial** : les Centres (hubs
  du commerce mondial) sont RARES, **N marchés = N(cités-états)/2**, plantés sur les
  meilleures cités-états (carrefour), espacés → **100 % du commerce mondial** passe
  par leurs Centres. `intertrade_seed_centres(w,e)`.
  **#5 (étape 2) — le PUMP À 2 ÉTAGES** (la chaîne joueur → cité-état → mondial) : chaque
  région se branche au marché de la cité-état (Centre) la **plus proche** (BFS multi-source
  sur l'adjacence, `hub_map_build`), à **rendement dégressif** (`MARKET_DIST_FALLOFF`/saut).
  Un achat de chantier est SOURCÉ pour de vrai (`intertrade_buy_cost`/`_market_consume`,
  lus par `agency_build_*`) : **stock PROPRE** de la région (×1, production sur place) →
  **marché LOCAL** = le stock du Centre le plus proche (×marge, péage à la cité-état hôte)
  → **marché MONDIAL** = le réseau des autres Centres (×marge×2, la **double taxe** quand
  le bien manque au local). Les achats **DÉPLÉTENT** ces stocks (pas dans le vide) ; la
  production y converge ⇒ elle a une destination. Chaque entité jouable garde SON stock
  par région (jamais drainé par autrui — seuls les Centres, le marché, sont partagés).
  Effet net POP-NEUTRE : le doublement tient (~100-102k year-100, seed 9) à `EMPIRE_CAP`
  **INCHANGÉ (13000)** — l'achat sur stock PROPRE reste ×1, ce qui compense le surcoût
  distance/double-taxe (le surcoût frappe l'import, pas la production locale). À VENIR
  (#5 suite) : la nourriture ∝ fertilité (régions stériles-riches ⇒ import).
- **V3 (2026-06-14) — LIBÉRER LE MARITIME RÉGIONAL (l'interaction est VIRTUELLE)** : le monde
  re-baseliné (Q6/L4 : Pangée fendue, empires écartés) avait mis les côtes étrangères les plus
  proches **hors** du vieux plafond de mer (72 j sur seed 7 > 60 j) ⇒ **0 route maritime**. La
  règle joueur : **deux ports + deux marchés + (pacte OU même empire) = commerce** ; la distance
  ne **FERME** plus la route — elle ne fait que **MODULER le rendement** (`routes_advance` :
  yield ∝ `1/(1+sea_days/40)`, loin = ténu, jamais nul ; hors-portée du calcul ⇒ distance
  VIRTUELLE = la borne). `routes_order` maritime ne garde donc que **DEUX PORTS** comme
  condition réelle. **`navy_best_coast`** ouvre une rade sur la meilleure côte (capitale côtière,
  sinon la côte la + peuplée) ⇒ un empire à **capitale enclavée** participe enfin à la mer ;
  l'IA navale (chronicle + viewer) bâtit le port là et trace ses routes depuis CE port. La
  **passerelle de mer** de `hub_map_build` est ROBUSTIFIÉE : toute côte déjà branchée à un marché
  par terre (Centre OU relais) fait porte — plus de dépendance à UN seul Centre côtier.
  ⚠ **RE-BASELINE** (le monde change : routes maritimes seed 7/11/19 = **6/18/20**, ex-0/3/0 ;
  commerce terrestre seed 7 **monte** 4493→4866). Sobriété : **3 routes/pays** ⇒ seed 7 (2 empires)
  plafonne à 6. SAVE **non bumpé**. Colonies outre-mer = f(géographie : côte vierge cross-continent)
  + paix (transports libres) — émergent, non forcé.
- **S1 (2026-06-14) — LE COMMERCE OUVRE L'ARCHÉTYPE (la cristallisation de Venise)** : deux
  ajouts à la diffusion syncrétique. (1) `ai_archetype_depth` gagne un **CHEMIN COMMERCIAL** :
  une route **OUVERTE** où l'empire est partie et dont l'AUTRE bout PORTE un archétype creuse la
  profondeur de contact — la **MER pèse fort** (`SYNC_TRADE_SEA_W` > `_LAND_W`), sommée sur les
  **ENTITÉS distinctes** (registre J : SEA_W 2 · LAND_W 1 · MÉTIER 1 · PROFOND 2 · YIELDREF 5).
  Ça ouvre la porte d'un archétype qu'on NE gouverne PAS (Venise ← Grèce, sans conquérir). (2) Le
  VRAI verrou n'était pas la porte (la gouvernance des héritages ÉPARS post-GR4 l'ouvre déjà
  largement : `accès[444440]`) mais la **RECHERCHE** : les signatures sont **tier-3 (~2300 pts)** et
  l'IA gloutonne ne paie JAMAIS le tier-3 (elle prend toujours le moins cher). La **GREFFE
  CULTURELLE** (`ai_research_step`) fait **ÉPARGNER** un empire INVESTISSEUR (mercantile/bâtisseur)
  pour la signature accessible la moins chère — **MÊME ressort que la foreuse** — bornée à ≤ 2
  greffes, **NON-faustienne** (le faustien = S3). ⚠ **RE-BASELINE syncrétique** : nœuds profonds
  seed 7/9/11/19 = **2/2/6/2** (ex-0) ; le « 0/6 archétypes » DÉCOLLE. Profondeur d'arbre (41 %),
  spécialisation, commerce **INCHANGÉS** (la greffe REMPLACE du tout-venant, elle n'ajoute pas de
  recherche). **SAVE non bumpé** (l'accès se recalcule). `ai_research_step`/`ai_race_access`/
  `ai_sync_refresh` prennent désormais le `RouteNetwork`.
- **S2 (2026-06-14) — LA CRISTALLISATION CULTURELLE SUIT LE CONTACT (réveil de `culture_syncretize`)** :
  le mutant hybride `culture_syncretize` (substrat A sous élite B) était **DORMANT** — seul le banc
  l'appelait ; la fusion VIVE (`assimilation_tick`) ne tire que vers la dominante LOCALE
  (intra-province ≈ « co-gouvernées »). `demography_contact_tick` (neuf, annuel) le RÉVEILLE : une
  région en CONTACT COMMERCIAL soutenu (route **OUVERTE**, à la **PAIX**) avec un AUTRE pays voit sa
  culture dominante dériver vers la sienne — **DURABLE** (pile de dérive, comme l'assimilation), la
  **MER** porte plus loin (×2) —, jugée par la MÊME porte métabolique **INCHANGÉE**
  (`culture_can_syncretize` σ(0.8(P−D∞)+0.35(K−5))). Au franchissement de `FUSE_EPS`, l'hybride
  **CRISTALLISE dans l'ORIGINE** (substrat durable) + la dérive culture remise à plat. Tunable
  `SYNC_FUSE_RATE` (registre J). ⚠ **RE-BASELINE** : cristallisations seed 7/9/11/19 = **5/1/0/4**
  (ex-0, dormant ; seed 11 = partenaires trop lointains en 100 ans) ; télémétrie neuve
  « cristallisation(s) culturelle(s) par contact ». Mondes **STABLES** (6 pays, 0 absorbés, commerce
  ~inchangé). **SAVE NON bumpé** (origine + pile de dérive déjà sérialisées). Les 2 bancs démographie
  gagnent `scps_diplo.o`/`scps_routes.o` au lien.
- **S3 (2026-06-14) — LE FREIN FAUSTIEN RÉCONCILIÉ → L'EMBLÈME S'ALLUME (forge runique × arcane)** :
  même S1 ouvrant les DEUX archétypes (NAIN+ELFE accessibles partout), l'emblème (`TECH_FORGE_RUNES`,
  faustien, **tier-3 derrière la Poudrière**) restait à **0** — non par le frein (déjà franchissable)
  mais par la **PROFONDEUR** (l'IA gloutonne n'atteint jamais le tier-3) ET l'absence d'empire assez
  transgressif (`w_faustian` plafonne ~0.18, appétit ~0.84). Deux gestes (`scps_ai.c`) : (1)
  `AI_TECH_FAUSTIAN` abaissé **2.5→1.2** (la rencontre appétit/frein demandée) ; (2) la **QUÊTE** de
  l'emblème — l'empire le plus faustien-enclin (`ai_faustian_appetite ≥ 0.80`, écarté des marchands
  par le filtre S1) **BEELINE** la chaîne (Poudrière → Forge à runes, `ai_step_toward`) en épargnant
  à chaque pas. On **NE touche PAS** la foreuse (son ressort propre). ⚠ **RE-BASELINE** : combo
  seed 7/9/11/19 = **3/1/2/1** (ex-0) ; la **charge → Brèche** la garde COÛTEUSE mais **BORNÉE**
  (mondes stables, 0 absorbé, âges inchangés). **SAVE non bumpé**.
- **P1 (2026-06-15) — LE STOCK NATIONAL (« toute ressource produite va dans le stock de SON
  empire »)** : fin de la fragmentation régionale qui bloquait les chaînes (l'atelier d'une province
  ne pouvait pas tirer la matière produite par une autre). `econ_tick` **AGRÈGE** les stocks régionaux
  en un **pool par pays** ; l'extraction y dépose, la **manufacture & la consommation Y PUISENT** (la
  matière de l'empire est fongible). La **MAIN-D'ŒUVRE reste LOCALE** (on ne staffe pas une fabrique
  avec les bras d'ailleurs : un atelier tourne = min(bras régionaux, intrants du pool)). Le **PRIX
  reste à l'échelle RÉGIONALE** — chaque province solde son marché sur **SA part du pool** (pop-share)
  → `market_effort` ne s'effondre pas sous un stock pooled « abondant » (sans ça : prix planché →
  effort 0.38× → prod amputée ~65 %). En **clôture**, le pool est **REDISTRIBUÉ aux régions au prorata
  de la pop** (Σ re->stock = pool, exact) → les **280 sites externes** (intertrade/Centres, viewer,
  butin de guerre, **save**) gardent une vue régionale cohérente **sans réécriture**. L'usure des
  OUTILS, le plafond E2 (Σ des caps) et la décrue ×0.85 sont **hoistés à UNE fois/empire** (un ×op
  par-région sur un pool partagé l'appliquerait N fois). La **réserve vivrière de brassage** suit le
  besoin de TOUT l'empire (le grenier est national). ⚠ **RE-BASELINE** (chronique : qui produit/
  consomme quoi change ; pop ~comparable, seed 9 year-100 ~36k). **SAVE non bumpé** (`re->stock`
  reste le store sérialisé, désormais une matérialisation du pool). Un empire **mono-région** ⇒ pool =
  la région, pshare = 1 : moteur **IDENTIQUE** (les bancs unitaires à 1 région/empire ne bougent pas ;
  `social_demo` marque ses 2 fixtures d'isolation `owner=-1` = stock propre). Le pool ne franchit pas
  la frontière (intra-empire seulement) — la chaîne à feu inter-empires restait morte : **RÉSOLU en P2**.
- **P2 (2026-06-15) — FINIR LA CHAÎNE À FEU (l'arquebusier paraît enfin)** : « si l'IA pose une
  poudrière, qu'elle FINISSE la chaîne — sinon ça n'a pas de sens ». La doctrine militaire
  (`ai_build_manufacture`) pose désormais la chaîne à feu **ENTIÈRE** (arquebuserie + **poudrière** +
  **charbonnière**, comme l'armurier des cités-états) au lieu d'une arquebuserie muette. Quatre gestes :
  (1) **completion** — bâtir l'arquebuserie déclenche poudrière (salpêtre+charbon→poudre) + charbonnière
  (bois→charbon) ; le **pool national (P1)** amène le salpêtre d'où qu'il tombe. (2) **gate de sens** —
  la chaîne ne se pose que si elle peut TOURNER : `TECH_POUDRIERE` découverte ET salpêtre dans l'empire
  (`empire_has_raw`) ET pas déjà posée (`empire_has_bld`, UNE fois) ; sinon **armurerie lourde** (fer,
  toujours dispo) — jamais de feu mort. (3) **ÉTHOS ORDRE** rejoint la doctrine martiale (l'État de
  **discipline** — le drill à poudre, le type le plus apte au feu) aux côtés de Dominateur/Honneur.
  (4) **T-gate par RÉGION-HÔTE** (et non plus la seule capitale) : avec le pool, une province-fer
  développée (tier ≥ 3) héberge la chaîne même sous une capitale modeste — la matière y afflue.
  ⚠ **RE-BASELINE** : feu **0 → 3.3/tick**, arsenal feu 0 → 90+, **arquebusier 0 → 3 unités**
  (seed 9, year-150) ; l'arc/le trait montent aussi (T-gate assoupli). Determinism 12-ans seed 7
  **inchangé** (la chaîne n'éclôt qu'après le tier-3, > 12 ans). **SAVE non bumpé**.
- **N1 (2026-06-16) — LA CARTE NUE (worldgen ne pose plus rien, SAUF les cités-états)** : le worldgen
  NE POSE plus socles de matière, manufactures ni niveaux pour les EMPIRES — la tuile produit sa
  **VOCATION** (`REGION_RAW_KEEP`=3 brutes les plus fortes + vivrier + stratégiques rares ; la longue
  traîne tombe), la carte naît **NUE** et l'IA/agency élèvent les manufactures DANS LE TEMPS
  (`econ_build_tick` §NF + chantiers payés). Trois compléments : **(1) À L'EXCEPTION DES CITÉS-ÉTATS** —
  elles tiennent le marché mondial (#5), donc EXEMPTÉES : elles gardent socles + voiles arcanes +
  manufactures au gisement + niveaux, comme avant. **(2) POOL TRADABLE CITÉ-ÉTAT** (`CS_TRADE_POOL`=1000
  bois/fer/argile/pierre sur la région-pivot de chaque cité-état) : le marché mondial le revend aux
  empires nés nus — la matière du bâti (trio bois/pierre/argile des chantiers) + le fer des outils ont
  une SOURCE, l'empire IMPORTE au lieu de stagner au plancher ½·cap_pop. **(3) MARCHÉ DE DÉPART** —
  chaque empire naît avec un `EDI_MARCHE` GRATUIT sur sa capitale (`agency_seed_capital_markets`, semé
  comme les Centres en chronicle/viewer ; un empire nu ne pourrait pas le payer). Le **gate** de pose
  autonome (§NF) reste les **RESSOURCES** : la manufacture ne s'implante que si le royaume sait la
  NOURRIR (intrant produit dans le pool OU importé en stock ≥ `NF_STOCK_MIN`) — PAS la tech (gate de
  5 manufactures avancées seulement : foreuse/alambic/réplicateur/corne/arquebus) ni une « volonté »
  d'IA (la voie §NF est price-driven ; l'IA-agency est la voie parallèle payée en or). Le charbon de la
  chaîne à feu/arcane reste au JOUEUR (la charbonnière, bois→charbon) — pas dans le pool. ⚠ **RE-BASELINE**
  (seed 9) : accession du 3e empire **an 66 → 43** (pool), vrac aval **0 → 1.6**, hub tenu **1/4 → 2/4**
  (marché de départ), commerce via cités-états **62 % → 55 %** (les empires négocient plus chez eux).
  Tunables registre J : `REGION_RAW_KEEP` 3 · `CS_TRADE_POOL` 1000. Bancs (carte nue ⇒ les fixtures qui
  dépendaient des bâtiments posés d'office sont recâblées) : `ai_demo` « Bâtisseur +K » ROBUSTIFIÉ
  (monde nu ⇒ digestion permanente, la voie K bascule en `builds_other` ; on garde « métabolise le plus »
  (K proactif OU édifices civils), l'appétit `w_build` reste STRICT) ; `social_demo` brasserie ISOLÉE
  (`owner=-1`, sinon diluée P1 sur les sœurs nues) ; `econ_arcane_demo` forge nourrie en CHARBON (in2).
  **SAVE non bumpé** (l'accès marché des empires — `hub_map` vers le Centre de la cité-état la plus proche,
  #5 — est INCHANGÉ ; `re->stock` reste le store sérialisé).
- **§27 CAPSTONE (2026-06-17) — L'ENDGAME : Entropie mondiale + 4 fins + Merveille** :
  l'étage final. L'**ENTROPIE MONDE** (`WorldProsperity.entropy`) incorpore désormais la
  **charge de TECH faustienne** (`endgame_tick` ajoute `ENTROPY_TECH_W·Σts[c].charge` APRÈS
  `prosperity_tick` — vraie entrée moteur, pas un bonus plat) en plus des transmuteurs : sans
  ça l'endgame ne se déclenchait JAMAIS (transmuteurs jamais posés en monde stable). Au seuil
  `ENTROPY_FIN`, `endgame_select_and_fire` latche UNE fin (compteur de rare dominant : essence→
  EAU · flux→RONCES · fer céleste→FROID ; sans transmuteur, signature déterministe du fauteur).
  **EAU** : carve radiale (rift, `sunken[]`), `world_recompute_adjacency`+`econ_build_adjacency`,
  refragmentation géo (`cataclysm_resplit_empire`). **FROID** : `cell.temperature` mutée par delta
  annuel → `world_rebiome_cell` → `econ_cold_refresh` (le grain re-dérivé de l'habitabilité gelée
  PLONGE sous le floor anti-famine → la pop décline). **RONCES** : `BIO_THORNS` + BFS-cellules
  erratique (`thorn_front[]`, rng dédié). **MERVEILLE** (`endgame_start_wonder`, JOUEUR seul) :
  3 paliers (FORGE/SOCIÉTÉ/SAVOIR ← fer céleste/flux/essence), charge-additive (ascension ET
  apocalypse sur la même course), victoire = 3 paliers + **assimilation EFFECTIVE de tout le
  monde** + arbre complet → l'empire disparaît (Dwemer, terre intacte). État sérialisé : section
  **EGAM** + **bump SAVE_VERSION 25→26**. Membrane : `BandEntropie`/`EndgameReadout` (enums
  miroirs, le seuil reste derrière la cloison). Tunables registre J : `ENTROPY_FIN` **55** ·
  `ENDGAME_YEAR_OPEN` **180** (gate dur : aucune apocalypse avant l'an 180 ; la victoire
  Merveille est **exemptée** — le joueur peut vaincre à tout moment) · `ENTROPY_TECH_W` 1.0 ·
  `SINK_RIFTS_PER_YEAR` 3 · `COLD_RAMP_PER_YEAR` 0.005 · `THORN_CELLS_PER_YEAR` 200 ·
  `THORN_RANDOM_FRAC` 0.35 · `MERV_PHASE_DAYS` 3650 · `MERV_CHARGE_PER_TICK` 0.5.
  **Calibration** (seed 9) : déclenchement EAU **an 195** (ex-184 ; gate 180 + FIN 55) ;
  an 100 = entropie 22.9 < 55 · an 180 = entropie 49.6 < 55 (**double cliquet** :
  gate temporelle ET seuil d'entropie tenus). Banc `endgame_demo` (contrôles C0-C6, 76/76).
  Télémétrie chronicle « §27 FIN ». **Déterminisme 12 ans INCHANGÉ**.
  ⚠ Re-baseline : les longs runs voient une fin à ~195 ans (ex-184).
  À VENIR : barre Entropie topbar + animations viewer (C7, dépend de SDL) ; pondération IA de
  l'entropie (C8, différée — non porteuse, l'endgame fonctionne sans).
- **VITALITÉ (2026-06-17) — LE MONDE VIT : `POP_R_BASE` ln2/100 → ln2/40 (sortie du bassin bas)** :
  le monde était « mou » — la pop des graines de référence se FIGEAIT autour de ½·cap_pop
  (≈34k seed 9) et AUCUN levier de bâti/colonisation ne la décollait (le LOGEMENT, §dev/a4d5c3c,
  a aidé +24 % mais sans sortir du bassin). Diagnostic (FILLDIAG `pop/EFF_CAP`, chronicle) : le
  frein n'était PAS la capacité (remplissage 42-46 %, aucune famine — food_sat ~0.97) mais la
  VITESSE de croissance. Le monde est **BISTABLE** : à `R_BASE`=ln2/100 la pop reste captive du
  bassin BAS et tout build/colonisation GLISSE dessus ; le seuil de bascule est entre /50 et /40.
  **`POP_R_BASE` passe à ln2/40 (0.01733)** — la pop sort du bassin bas (seed 9 : ≈60-70k sur
  5 sims, plage 44-97k ; doublement ~40 ans au plancher de besoins, ~20 ans au panier plein via
  le bonus). **Sortir du bassin a un PRIX** : le monde vif est plus CONTESTÉ (seed 9, /40 mesuré :
  satisfaction 76-80 %, ~12 guerres/sim, ~13 coups/sim, IPM 1.31, hégémon mortel 5/5) — c'est de la
  VIE (un monde à enjeux), pas du chaos ; **cette turbulence vaut AUSSI à /30** (≈80k), comparable —
  ce n'est PAS un défaut propre au taux haut. **/40 retenu pour le RÉALISME** (le critère demandé) :
  le doublement RÉALISÉ ≈20-40 ans (base 40 ans × bonus jusqu'à ×2) ENCADRE la cible ~30 ans, sans
  le biais trop-rapide du /30 ; monde **DIFFÉRENCIÉ** — riche = plein, pauvre = modeste (seed 11 reste
  partiel ~46 %) — **sans truquer**. C'est le **SEUL** levier touché (aucun build order forcé, aucune
  colonisation forcée, aucune inflation plate). ⚠ **EFFET AVAL §27** : le monde plus vif charge
  l'entropie plus vite → la fin éclôt PLUS TÔT (obs. RONCES an 127 / GRAND HIVER an 143, seed 9, vs
  ~184 documenté), toujours **post-100** (le cliquet tient). ⚠ **RE-BASELINE** : les mondes des graines
  de référence ONT CHANGÉ (pop ≈×1.8) ; le **hash 12 ans re-baseline** (le taux mord dès l'an-0,
  contrairement à l'endgame).
  `make test` **35/35 vert** (les contrôles sensibles au monde — `ai_demo`, `social_demo` — étaient
  déjà robustes à l'échelle de pop) ; `make determinism` STABLE (save/reload byte-identique).
  **Dialable d'UNE ligne** ou `SCPS_TUNE=POP_R_BASE=…` vers /35 (≈64k) ou /30 (registre J :
  `POP_R_BASE` 0.01733). **SAVE non bumpé** (aucune struct sérialisée ne change).
- **Anti-emballement dette (bug PRÉ-§27 corrigé)** : `credit_year_tick` plafonne taux & assiette
  d'intérêt au-delà de `CREDIT_RATIO_CAP·ligne` (sans ça : intérêt ∝ dette² → treasury → -1e31 →
  NaN vers l'an 105). Déterminisme 12 ans inchangé ; les longs runs restent finis.
- `make scps` : le visualiseur (SDL2) — **0 warning** (`-Wall -Wextra`), toujours.

## Disciplines non négociables

- **La membrane** : `viewer.c` n'inclut jamais `scps_core.h` et ne lit aucun flottant SCPS — des MOTS (readout) et des nombres tangibles seulement.
- **On lit des coordonnées, on n'assigne jamais de modificateur** : un effet passe par les entrées du moteur (K, P, H…), jamais par un bonus plat.

## Langue (brief table de chaînes)

- **Aucune chaîne littérale face-joueur hors des tables.** Tout panneau, journal, preview, bande, tooltip à venir naît directement en `STR_*` :
  - la liste maîtresse vit dans `scps/strings_ids.h` (X-macro, texte FR de référence inline) ;
  - l'anglais est la liste jumelle `scps/strings_en.h` (même ordre — la complétude est vérifiée à la **compilation**, retirer une ligne casse le build) ;
  - appels : `tr(STR_X)`, plages `tr_band(STR_X_0, idx, n)`, paramètres **positionnels** `tr_fmt(buf, n, STR_X, a0, a1)` — `{0}..{9}`, l'ordre des mots n'est pas universel ;
  - pluriels : deux clés (`STR_X_UN` / `STR_X_PLUSIEURS`) là où c'est nécessaire, et seulement là.
- **Clôture** : `chronicle.c`, `econ_scan.c`, `batch.c`, `dump.c`, tout `printf` de télémétrie/journal console et les commentaires restent en **français, définitivement** (l'outillage de l'ingénieur, pas le jeu).
- **Surcharge runtime éditable** (rupture ASSUMÉE de « zéro asset ») : `scps_lang.txt` à côté du binaire **remplace** n'importe quel `STR_*` par son ID. Les défauts compilés restent (le binaire tourne sans le fichier) ; c'est **display-only** (le moteur/déterminisme n'y touchent pas) et c'est le mécanisme de **traduction** (un fichier = une langue surchargée). `scps_viewer --dump-lang` écrit le fichier éditable complet ; **F4** le recharge à chaud. Tout texte face-joueur naît donc en `STR_*` ⇒ devient éditable sans recompiler.
- `make lang-check` : le **cliquet** — échoue si le nombre de littéraux face-joueur dépasse la base (`scps/lang_baseline.txt`). Le reflux est attendu à mesure que la migration avance (abaisser la base à chaque extraction).
- État de migration : lexique readout (bandes, labels, hovers) + shell (menu, pause, slots, tutoriel) **migrés** ; le reste de `viewer.c` (panneaux, chips, zone_add) part de la base 64 et descend.

## Sauvegarde

- Format versionné (`SAVE_VERSION`), sections taguées, ChaCha20 (clé = obfuscation assumée, pas un secret) + empreinte FNV du clair.
- Toute valeur désérialisée qui borne une boucle ou indexe un tableau **se revalide** au chargement (`save_sane`) — refus net.
- Changer la taille d'une struct sérialisée ⇒ bump `SAVE_VERSION` (« ère antérieure »).
