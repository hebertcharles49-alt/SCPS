# SCPS — conventions du dépôt

## Principes de collaboration (non négociables)

- **Demander, ne pas supposer.** Si quelque chose n'est pas clair, demander AVANT
  d'écrire une seule ligne. Jamais de supposition silencieuse sur l'intention,
  l'architecture ou les exigences.
- **La solution la plus simple d'abord.** Toujours implémenter la chose la plus simple
  qui puisse marcher. Pas d'abstraction ni de flexibilité non explicitement demandée.
- **Ne pas toucher au code hors sujet.** Si un fichier ou une fonction n'est pas
  directement concerné par la tâche en cours, ne pas le modifier — même si on pense
  pouvoir l'améliorer.
- **Signaler l'incertitude explicitement.** En cas de doute sur une approche ou un
  détail technique, le dire avant de continuer. La confiance sans certitude fait plus
  de dégâts qu'un manque admis.
- **Ouvert aux meilleures idées.** Ne pas hésiter à proposer une meilleure façon de
  faire, ou une qui a un impact durable plutôt qu'un correctif tactique.

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
  `EMPIRE_CAP` 13000 / `CITY_CAP` 6500 (taille nourrie ; valeurs CODE, cf. scps_tune_list.h) · `HOUSE_MANUF` 100 · `SEED_POP`
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
  `make test` **40/40 vert** (les contrôles sensibles au monde — `ai_demo`, `social_demo` — étaient
  déjà robustes à l'échelle de pop) ; `make determinism` STABLE (save/reload byte-identique).
  **Dialable d'UNE ligne** ou `SCPS_TUNE=POP_R_BASE=…` vers /35 (≈64k) ou /30 (registre J :
  `POP_R_BASE` 0.01733). **SAVE non bumpé** (aucune struct sérialisée ne change).
- **MODIFICATEURS PROVINCIAUX (2026-06-17) — « Terre d'abondance » réveille les LOW SEEDS (un
  modificateur DIÉGÉTIQUE, pas un bonus plat)** : le /40 décolle les seeds RICHES mais laisse les
  mondes FRAGMENTÉS (seed 11/145) coincés au bassin bas — non par CAPACITÉ (seed 9 & 145 ont la
  MÊME cap_col ~60k, food_sat=1.0 partout) mais par ALLUMAGE : la révolte PRÉCOCE (an20-23) les
  assomme sous 50 % de remplissage pendant la fenêtre critique, et le plancher de croissance ne
  remonte pas la pente → ils IDLENT un siècle avant de s'allumer (seed 145 : 21k an1 → 24k an80 →
  46k an150). Le fix respecte la discipline (« l'effet passe par les ENTRÉES du moteur, jamais un
  bonus plat ») : un **MODIFICATEUR PROVINCIAL** dérivé de l'état réel, routé par l'entrée
  DÉMOGRAPHIE. **TERRE D'ABONDANCE** (`provmod_collect`, inline `scps_econ.h`) — une région
  SOUS-PEUPLÉE (fill < `PROVMOD_ABOND_REF`=0.45) + NOURRIE + en PAIX (cicatrice basse) gagne un
  surcroît de natalité ∝ (REF−fill)·food_sat·(1−scar), AJOUTÉ à `demo` (le levier d'espèce),
  échelle `PROVMOD_ABOND_K`=2.0. **Auto-ciblé** : une terre pleine (seed riche) reçoit 0 → seed 9
  ~inchangé (sa variation single-sim est du BRUIT — un boost ne peut pas BAISSER la pop, or
  REF=0.40 K=2.5 la baissait) ; les low seeds DÉCOLLENT (seed 11 +39 %, seed 145 +40 % à l'an120 ;
  l'idle d'un siècle RÉSORBÉ — seed 145 grimpe 21k→30k dès l'an40). Effet **DÉRIVÉ** (aucun champ
  stocké, recalculé moteur ET membrane) → **SAVE non bumpé**. La **cicatrice de révolte** (existante)
  est SURFACÉE dans le même slot (fléau, demo_bonus=0 — pas de double-compte). **Slot UI province
  « MODIFICATEURS »** (multiple) : `ProvinceReadout.mods[]` (membrane, mots + signe — faveur vert /
  fléau rouge), rendu par `viewer.c` entre Capitale et Bâtiments ; 7 `STR_*` (FR+EN). ⚠
  **RE-BASELINE** : les low seeds montent ; le **hash 12 ans re-baseline** (l'abondance mord dès
  l'an-0). `make test` 40/40 · `determinism` stable · `lang-check` 64/64. Éteignable :
  `SCPS_TUNE=PROVMOD_ABOND_K=0` (le modificateur reste AFFICHÉ). ⚠ **Mesure propre (5 sims
  appariés)** : seed 9 OFF 63.0k → ON 67.8k (**+7.6 %** — il était lui-même à ~½ rempli, il capte
  donc un peu d'abondance ; le single-sim « bruit » l'avait masqué) ; les low seeds montent ~5× plus.
  Tunables registre J : `PROVMOD_ABOND_REF` 0.45 · `PROVMOD_ABOND_K` 2.0.
- **MODIFICATEURS PROVINCIAUX lot 2 (2026-06-17) — ferveur fondatrice + reconstruction + limon
  (faveurs À ÉTAT)** : trois faveurs de plus sur le même slot « MODIFICATEURS » et le canal DÉMO.
  **FERVEUR FONDATRICE** (`re->ferveur`, semé à 1 par `colonize_from`, décru ~15 ans) : une colonie
  fraîchement fondée croît avec élan (incarne aussi « Nouvelles perspectives »). **RECONSTRUCTION**
  (`re->reconstruction`, amorcée à 1 par une cicatrice PROFONDE >0.5, LIBÉRÉE à mesure que la plaie
  se referme via `recon·(1−scar)`, décrue ~10 ans) : la renaissance d'après-révolte/sac — la reprise
  SUIT la paix, elle frappe pile le creux des low seeds. **LIMON FERTILE** (dérivé, `re->estuary`) :
  un delta nourrit une natalité dense. Tous routés par l'entrée DÉMO (`provmod_collect` inline),
  surfacés en faveurs, 6 `STR_*` (FR+EN). ⚠ **SAVE BUMPÉ 26→27** (RegionEconomy +ferveur
  +reconstruction = 2 floats À ÉTAT → `sizeof(WorldEconomy)` change ; <v27 refusé). ⚠ RE-BASELINE
  (les faveurs s'empilent ; hash 12 ans re-baseline). `make test` 40/40 · `determinism` STABLE.
  Tunables registre J : `PROVMOD_FERVEUR_K` 0.5 · `_DECAY` 0.067 · `PROVMOD_RECON_K` 0.6 · `_DECAY`
  0.10 · `PROVMOD_LIMON_K` 0.15. À VENIR (autres SOUS-SYSTÈMES) : boosts K admin (build/agency),
  bonus géo biome gibier/halieutique (worldgen), couplage satisfaction de la croissance.
- **ÉVÈNEMENT XÉNOPHILE (2026-06-17) — « Le creuset des peuples » (la diversité qui RÉUSSIT)** :
  un évènement POSITIF de plus, par le motif data-driven existant (`EVENTS[]`/`apply_effect`, effets =
  COORDONNÉES). `EVID_XENOPHILE` (EV_PROVINCE) : `trig_xenophile` exige plusieurs cultures qui
  CONVERGENT (`econ_off_culture_fraction` > 0.20) sous un ÉTHOS ACCUEILLANT (Bureaucrate « tient la
  diversité » · Mercantile « carrefours » · Pacifiste « ne fracture jamais » — les xénophobes
  Dominateur/Honneur en sont exclus) ET une province APAISÉE (agit < 30, satisfaction > 0.55 : la
  diversité s'intègre au lieu de fracturer). Effet : +légitimité +fertilité +trésor +influence +une
  ruée d'immigrants (pop_mult 1.02) — le creuset qui prospère. Tiré dans `world_events_tick` en
  court-circuit (`trigger && frand`) → **aucun tirage tant qu'aucun creuset n'a mûri** : le
  déterminisme 12 ans NE BOUGE PAS (la diversité met des décennies à monter). Textes LITTÉRAUX comme
  tout `scps_events.c` (le module n'est pas migré STR_* ; `events_text_clean` reste vert). **SAVE non
  bumpé** (EVENTS[] est une table statique ; rien de sérialisé n'indexe EVID_COUNT). `make test`
  40/40 · `determinism` STABLE.
- **MIROIR XÉNOPHOBE (2026-06-17) — « Le creuset DIGÉRÉ » (la cohésion par la MÉTABOLISATION)** :
  le pendant martial du creuset, mais l'éthos Dominateur/Honneur ne GARDE pas la diversité — il la
  DIGÈRE. `EVID_XENOPHOBE`/`trig_xenophobe` exige qu'il y ait EU de la diversité (`n_groups≥2`,
  `econ_off_culture_fraction>0.15`) ET qu'elle soit DIGÉRÉE — intégration pop-pondérée des minorités
  (`g->integration`) > 0.75, signal RÉGION-LOCAL (le cache effectif du groupe, pas la pile de dérive).
  Comme la métabolisation met des DÉCENNIES à monter, le tirage est TARDIF → le déterminisme 12 ans
  TIENT (vérifié STABLE). Effet : +légitimité +garnison −agitation +influence — la cohésion farouche
  de qui a tout fondu en un seul sang. Symétrie d'éthos (pas « diversité = bien/mal plat » : le
  creuset GARDÉ prospère pour l'accueillant, le creuset DIGÉRÉ pour le martial). **SAVE non bumpé**.
  `make test` 40/40 · `determinism` STABLE.
- **COUPLAGE SATISFACTION ↔ CROISSANCE (2026-06-17, ASYMÉTRIQUE)** : le bonus de fertilité gagne
  un terme satisfaction `POP_SAT_W·max(0, satisfaction−0.5)` — une province CONTENTE croît un peu
  plus vite, mais la satisfaction BASSE ne soustrait RIEN (plancher à 0). Le **double tranchant**
  identifié (coupler symétriquement creuserait le creux des low seeds en turbulence) est neutralisé :
  un peuple nourri mais grognon croît quand même, et la récompense PRIME la reprise une fois la
  province apaisée. Zéro risque baissier. Tunable `POP_SAT_W` 0.20 (registre J). ⚠ nudge seed 9 vers
  le haut (province contente) — c'est tout l'écosystème vitalité qui soulève un peu tout, les low
  seeds ~5× plus. `make test` 40/40 · `determinism` STABLE. **SAVE non bumpé**.
- **DONS GÉO PROVINCIAUX (2026-06-17) — gibier abondant + manne halieutique** : deux faveurs géo
  SÉLECTIVES sur le slot « MODIFICATEURS » et le canal DÉMO. Posées à `econ_init` (tirage
  DÉTERMINISTE par région) : **~1/3 des régions BOISÉES** (majorité de provinces FOREST/WOODS/JUNGLE)
  portent **GIBIER ABONDANT**, **~1/3 des CÔTIÈRES** une **MANNE HALIEUTIQUE** — la richesse vivrière
  du biome soutient une natalité un peu plus dense. Drapeaux `RegionEconomy.prov_geo` (uint8_t) → ⚠
  **SAVE BUMP 27→28** (`sizeof(WorldEconomy)` change). 4 `STR_*` (FR+EN), `PROVMOD_GIBIER_K`/
  `PROVMOD_HALIEU_K` 0.10. ⚠ ce sont des bonus de **PLAFOND** (texture : ils enrichissent TOUS les
  seeds, ne ferment PAS l'écart low/riche — cf. garde-fou) : K volontairement PETIT. `make test`
  40/40 · `determinism` STABLE.
- **BONNE ADMINISTRATION (2026-06-18) — « admin efficace » par le canal DÉMO** : dernière faveur du
  brainstorm. `PMOD_ADMIN` (dérivé, pas de champ) : des institutions bâties (`re->build.K_inst` > 1.5)
  tiennent l'ordre & les services → natalité un peu plus dense (`PROVMOD_ADMIN_K` 0.06, plafonné).
  C'est le pendant DÉMO du « boost K » (le levier K littéral — bâtir plus vite — serait une greffe
  AGENCY/build ; ici on route l'efficacité administrative par la croissance, cohérent avec le pipeline).
  ⚠ `provmod_collect` est en-tête `static inline` SANS `<math.h>` → clamps MANUELS obligatoires (pas
  de `fminf`/`clampf` : ils tirent une déclaration implicite qui CONFLITE avec `scps_world.c`).
  `make test` 40/40 · `determinism` STABLE · **0 warning** · **SAVE non bumpé**. ⊕ Le brainstorm
  diégétique est COMPLET (abondance · ferveur · reconstruction · limon · gibier · halieutique · admin
  + évents xénophile/xénophobe + couplage satisfaction) — 8 faveurs sur le slot « MODIFICATEURS ».
- **Anti-emballement dette (bug PRÉ-§27 corrigé)** : `credit_year_tick` plafonne taux & assiette
  d'intérêt au-delà de `CREDIT_RATIO_CAP·ligne` (sans ça : intérêt ∝ dette² → treasury → -1e31 →
  NaN vers l'an 105). Déterminisme 12 ans inchangé ; les longs runs restent finis.
- `make scps` : le visualiseur (SDL2) — **0 warning** (`-Wall -Wextra`), toujours.
- **MERGE (2026-06-18) — un seul tronc** : la lignée VITALITÉ (modificateurs diégétiques,
  endgame §27) et la lignée ASSETS/ARMÉE/WORLDGEN du collègue (matrice de contres,
  l'éthos compose l'armée, villes jamais sur l'eau, déboisement) FUSIONNENT. Le format de
  save COMBINE les deux jeux de sections → **`SAVE_VERSION` 25→29**, depuis porté à **31** (v30 ROSTER
  militaire 12→22 unités · v31 empreinte tunables, cf. audit) (l'entête `viewer.c`
  documente le combiné : EGAM endgame + assets/armée). `make test` vert, déterminisme STABLE.
- **STABILISATION (2026-06-18) — `make audit` au VERT + harnais durci** : suite à l'audit
  technique (point bloquant : `make audit` ROUGE 2/4, jamais capté). Les deux bornes
  rouges (POP, ACCESSION) étaient des artefacts du BANC `audit_eco`, **pas** des bugs
  moteur (le vrai jeu bootstrappe via `CS_TRADE_POOL`/marché de départ — l'extraction est
  demand-driven, le banc n'avait ni Centre ni source de pierre). Fixes de BANC seulement :
  hameau témoin = région du joueur la plus sous son eff_cap (au lieu d'une vierge de
  frontière gelée) ; capitale amorcée d'un socle bois/pierre/argile. **Harnais** :
  `audit_eco` + `lang_demo` rejoignent `run_tests.sh` (**35 → 37 bancs** — le rouge serait
  désormais capté), `timeout` par banc, split `make smoke` (rapide) / `make full-test`
  (bancs + déterminisme + ASan). **Aucune entrée moteur touchée** : re-baseline NULLE,
  aucun hash bougé, **SAVE non bumpé**. Détail en AUDIT.md §(a-bis).
- **FX ANIMÉS (2026-06-18) — houle · écume · armées · vortex (display-only)** : quatre
  planches à fond MAGENTA (`scps_fx_{sea,coast,army,vortex}.bmp`, suivies en repo comme les
  atlas), cadencées par **SDL_GetTicks** (horloge MUR, JAMAIS le moteur → déterminisme
  intact). `load_fx_bmp` (viewer) key le magenta SANS le despill des atlas de carte (la teinte
  FX passe verbatim — un vortex violet survit). **draw_sea_fx** : voile de houle tuilé sur la
  mer ouverte (`c->sea`), ligne d'atlas ∝ énergie du courant (`cur_vx/vy`), borné au viewport.
  **draw_coast_fx** : écume sur la TERRE côtière (`c->coast`, zoom franc). **draw_army_fx** : la
  force de campagne (`campaign_location/phase`) anime sa région (marche/assaut). **draw_vortex_fx** :
  la fin EAU §27 — maelström contrarotatif au `epicenter_reg` + tourbillons sur `sunken[]`. Absentes
  ⇒ NULL ⇒ no-op. **Membrane TENUE** (faits tangibles — où est l'eau, où marche l'armée — jamais
  un flottant §2.4). Vérif headless : **`make fx-proof`** composite les 4 planches sur un terrain
  `render_map` RÉEL (renderer logiciel) → `fx_proof.png` (aucun display requis). `make scps` **0
  warning**, **SAVE non bumpé** (display-only).
- **BASCULE GODOT (2026-06-18, spike) — le front passe sous Godot, le moteur reste C** : décision
  d'archi. Le cœur C99 déterministe NE BOUGE PAS ; seul le front-end migre (shaders/particules/
  tilemaps/UI). La membrane RENDAIT déjà ce déplacement naturel. **(1) Façade C `scps_api.{h,c}`**
  (dans `scps/`, additif) : la surface de binding STABLE — `scps_sim_new/generate/advance_days/free`,
  `scps_map_rgba` (render_map → octets RGBA), `scps_map_layer` (height/sea/biome/coast, pour shaders),
  nombres TANGIBLES (year/pop/gold/…), par-région (owner/pop/centroïde). Banc `scps_api_demo` (9/9,
  dans le harnais) prouve génération + rendu + avancement + **REPRODUCTIBILITÉ** (sim A == sim B au
  bit). Fidélité SPIKE : `advance_days` roule la COLONNE économique (boucle d'`audit_eco`) ; le tick
  PLEIN (fidèle au hash chronicle) viendra de l'extraction `chronicle::sim_day → scps_sim` **sans
  changer la surface de l'API**. **(2) `godot/`** (dossier à part, hors `make`) : binding GDExtension
  C++ (classe Godot **`ScpsWorld`** — nom distinct du type C `::ScpsSim`), `SConstruct` qui recompile
  les MÊMES `scps_*.c` + façade → `libscps.<plateforme>.so`, projet Godot 4 minimal + **`water.gdshader`**
  (la continuité eau↔asset EN SHADER). **Vérifié headless** : binding compile contre godot-cpp 4.3, `scons`
  LIE le `.so` (entrée `scps_library_init` exportée). **RÈGLE D'OR** : zéro logique sim côté GDScript —
  le déterminisme survit. `make test` inchangé (le moteur ne bouge pas), **SAVE non bumpé**.
- **GODOT PHASE 2 (2026-06-18) — LIRE LE MONDE : la membrane traverse le binding** : le clic ouvre
  des panneaux readout (ce qui rend SCPS JOUABLE, pas juste regardable). Trois ajouts à la façade
  `scps_api`, additifs et low-risk. **(1) PICKING** : `scps_province_at(x,y)` (cellule monde →
  province, l'entité de panneau) + `scps_province_region`. **(2) READOUTS TRAVERSANTS** :
  `scps_province_info`/`scps_country_info` remplissent des **POD de MOTS DÉJÀ RÉSOLUS** (terrain,
  humeur, éthos, bandes via `scps_readout.c`) **+ nombres TANGIBLES** (âmes, or, jauges 0-100) — la
  membrane elle-même, franchissant la frontière C↔C++ sans qu'un flottant moteur passe. La façade
  alloue+ticke `WorldProsperity`/`WorldLegitimacy`/`TechState` (l'ordre canonique L→P, lus en
  **CONST** sur econ/world → la colonne économique reste **byte-identique** ; pop inchangée). **(3)
  SÉLECTION SURLIGNÉE** : `scps_map_rgba` prend `selected_prov` (état d'AFFICHAGE, hors déterminisme).
  Côté Godot : `province_info`/`country_info` → `Dictionary`, clic gauche (slop anti-glissé) →
  `ProvincePanel`/`CountryPanel` bâtis en code (jauges rouge→vert), clic en mer referme. ⚠ Les zéros
  an-0 (or/prospérité/influence) sont l'AVEU HONNÊTE de la colonne éco (ni diplo ni commerce tické) ;
  ils se peuplent avec l'extraction `chronicle::sim_day` **sans changer la surface façade**. Vérifié
  **headless** (probe GDScript : picking province 18, readouts complets, borne hors-champ → invalide).
  `make test` **40/40**, `determinism` **STABLE** (hashes chronicle inchangés), `scps_api_demo` 9/9,
  GDExtension `scons` 0 warning. Le moteur ne bouge pas → **SAVE non bumpé**.
- **TICK PARTAGÉ (2026-06-18) — `chronicle::sim_day` → `scps_sim.{h,c}` : le monde Godot VIT
  PLEINEMENT** : la façade ne roulait que la COLONNE économique (d'où les zéros an-0 : ni guerre,
  ni diplo, ni prospérité réelle). On EXTRAIT le cœur de jeu de `chronicle.c` vers un module partagé
  `scps_sim` — `Sim` (l'état plein) + `sim_init` + `sim_day` (agency · IA · events · économie ·
  statecraft · démographie · navy · révolte · world_tick · légitimité · commerce · intertrade ·
  contact · prospérité · endgame · warhost · campagne · diplo · crédit · missions · factions) +
  `regions_of` + le PROFILER (PROF) + les compteurs d'occupation. **DÉPLACÉ VERBATIM** : chronicle
  inclut `scps_sim.h` et garde sa boucle/télémétrie ; **le hash de déterminisme est IDENTIQUE au
  byte près** (`make determinism` : 6ad331bf/5b4c5754/1a6611bd/390adfdf/4524729b, inchangés).
  `scps_api` embarque un `Sim` (helpers `sim_alloc`/`sim_free_members`), `scps_sim_generate`→
  `sim_init`, `scps_sim_advance_days`→ N×`sim_day` ; **la surface de la façade n'a PAS bougé** (tout
  le binding/panneaux Phase 2 tourne tel quel). Payoff (probe headless seed 9) : an-0 pop 43 k → an-80
  **111 k**, un pays à **or 141 k · prospérité 79/Aisance · savoir 29** (tout était à 0 sous la colonne
  éco). ⚠ **UN SEUL Sim actif par PROCESSUS** : intertrade/factions/arms_pump portent un état GLOBAL
  remis à plat par `sim_init` (la chronique enchaîne ses sims séquentiellement, la façade n'a qu'un
  monde). Le pays « joueur » est, pour l'instant, piloté par l'IA comme dans la chronique (le monde
  s'observe ; la main humaine viendra). `scps_api_demo` 9/9 (pop change — le monde vit — mais
  REPRODUCTIBLE A==B). Makefile : `scps_sim.o` dans `CHRONICLE_OBJS` ; SConstruct : `"sim"` dans
  ENGINE_MODULES. `make test` **40/40**, **0 warning**, **SAVE non bumpé** (rien de sérialisé ne change).
- **GODOT PHASE 3 (2026-06-18) — LES ACTEURS SUR LA CARTE** : le tick plein roule déjà guerres &
  campagnes ; on les EXPOSE et on les dessine. Façade (additive) : `scps_army_info` (loc · dest ·
  phase mot+brut · effectif · composition inf/arch/cav/mages, lu de `s->sim.camp`) + `scps_region_tier`
  (0-5 selon la pop, capitale ≥4 ; miroir du viewer). Binding → `Dictionary`. **`Overlay`** (Node2D
  enfant de MapView, espace MONDE → suit la caméra, display-only, redessine au tick) : **villes** par
  tier (disque teinté au pays, cœur clair) + **armées** (losange teinté au pays, posé au-dessus de la
  ville, halo de PHASE marche=blanc/siège=orange/bataille=rouge, ligne vers le but). Vérifié headless
  (xvfb : seed 9 an-110, 7 armées · 73 villes). Moteur inchangé (façade) → déterminisme & SAVE intacts.
- **GODOT PHASE 4 (2026-06-18) — L'ENDGAME §27 EN SCÈNE** : le moteur MUTE déjà le monde quand une fin
  éclôt (régions englouties → mer via `cataclysm_sink_region` ; biomes blanchis au froid via
  `world_rebiome_cell` ; ronces `BIO_THORNS` qui gagnent) → **`render_map` montre l'apocalypse PHYSIQUE
  sans shader**. On ajoute la LECTURE + la mise en scène. Façade : `scps_endgame_info` (entropie 0-100 +
  bande · augure · fin 0-4 · merveille · intensités froid/eau · épicentre, depuis `endgame_readout`) +
  `scps_region_sunken`. **`EndgameBanner`** (haut-centre) : la jauge d'entropie monte par bandes (Stable
  → Frémissante → Instable → Au bord) avec des **augures** qui escaladent ; au déclenchement, un
  **bandeau rouge** nomme la fin. **Épicentre** : anneaux pulsants (Overlay, `_process` horloge MUR — hors
  déterminisme — teintés par type de fin : EAU bleu · FROID glacé · RONCES vert). Vérifié headless (seed 9 :
  entropie 18→100 sur l'an 21-181, **fin RONCES an-181** épicentre rég. 30 ; bandeau + anneaux + terre
  assombrie capturés). Reste optionnel : particules GPU (neige/écume), shader de rift. `scps_api_demo` 9/9,
  moteur inchangé → **SAVE non bumpé**. ⚠ noms de fin = chrome GDScript (i18n Phase 5).
- **AUDIT STEAM — correctifs P0/P1/P2 (2026-06-25)** : durcissement « contrat public » avant que des
  saves touchent un joueur (toutes vérifs au vert : 40 bancs · déterminisme INCHANGÉ vs golden ·
  ASan/UBSan muet · viewer 0 warning). **P0-1** (le seul bug de CORRUPTION) : `save_sane` borne enfin
  `AgencyState.n`/`RevoltState.count`/`ArmyState.n_units` (+ régions d'ordre) ; `purge_slice` borne
  `reg` (la boucle PURGE l'appelait SANS la garde d'apply_action ⇒ écriture hors-bornes depuis une save
  forgée). **P0-2** : `game_load` rétablit `warhost_set_human` (sinon l'armée du joueur se re-mobilisait
  seule après un load). **P0-3** : empreinte des surcharges `SCPS_TUNE` dans le SaveHeader (`tune_ck`) →
  un reload sous d'autres règles AVERTIT ; **SAVE_VERSION 30→31**. **P0-4** : gate de NON-RÉGRESSION —
  `scps/golden_hashes.txt` committé + `make golden` (le `determinism` ne prouvait que la self-cohérence).
  **P1-1** : fuite du scratch warhost (~15 Ko/régénération) — `warhost_free` au teardown + avant ré-init,
  host calloc'd. **P1-2** : `clampf` ne propage plus le NaN (17 copies) ; accumulateur `faust_charge`
  borné (anti-dérive-inf cross-build). **P2-1** : `prosperity_demo` VÉRIFIE (17 assertions) au lieu de
  vert-sur-rc=0. Détail & vérif-de-l'audit (claims CONFIRMÉS/REFUTÉS, file:line) à part.
  **Bonus & suite** : `make fuzz-save` (`scps_viewer --fuzztest` headless — forge chaque compteur
  désérialisé hors-borne → save_sane REJETTE, le test qui aurait pris les 3 trous P0-1 d'un coup ; +
  fuzz d'octets → game_load ne plante jamais) ; `make determinism-deep` (200 ans × 2 graines, nightly —
  exerce l'endgame §27 / le clamp crédit / le cataclysme, hors du gate 12 ans) ; **P2-4** `culture_demo`
  rendu AUTO-VÉRIFIANT (23 assertions sur les invariants : corps/âme, mutation, 2 canaux de distance,
  continuum de syncrétisme), il était construit mais jamais lancé ; **P2-2** `navy_demo` NEUF (la flotte
  n'avait AUCUN test : rade `navy_best_coast`/`_port`, chantier `navy_order_build` — sans port rien, UN
  seul à la fois —, complétion au tick, emport 10 paquets, conversion marchand→pirate, invariants
  `save_sane` des coques/at_sea/build_hull — 20 contrôles). Les deux RECÂBLÉS au harnais (**40 bancs**).
- **REFONTE ÉCO — ressource PAR-OUVRIER + bouche annuelle + nourriture du spawn (2026-06-25)** : l'économie
  passe d'une extraction `raw_cap × √pop` à du LABOR-BOUND. (A0/A1) `out = ouvriers × EXTRACT_YIELD[r] ×
  geo_eff × effort(prix)` — table de rendement ANCRÉE (grain 800/poisson 400/bois 50/pierre·argile 25 par
  100 ouvriers/an), le ×2 bois/fer/or REPLIÉ dans le rendement ; les ouvriers se répartissent entre brutes
  ∝ geo_eff×prix (la part `EXTRACT_LABOR_SHARE` des journaliers, le reste staffe les manufactures). geo_eff =
  raw_cap/`EXTRACT_GEO_REF` (qualité de tuile, MULTIPLICATEUR — pas la base). (A2) la BOUCHE est ANNUELLE
  (1 food = ration-personne-an), grain+poisson INTERCHANGEABLES (food_sat les agrège), CALIBRÉE à la
  géographie (`FOOD_NEED`) car un bond ×8 brut ÉCRASE `needs_met` (le moteur de fertilité : la nourriture
  monopolise le budget des journaliers). (A3) recettes per-100-artisans — ratios INCHANGÉS, déjà compatibles.
  (A4) techs d'output = le `tech_prod` GLOBAL existant (+77 % à plein) booste la nouvelle extraction (pas de
  système neuf). (A5) commerce comble les déficits ; **nourriture du SPAWN** = la SEULE règle vivrière de
  worldgen : la capitale de chaque empire naît avec un socle de grain (`SPAWN_FOOD_RAW`), tout le reste est
  GÉOLOGIE. Calibré (seed 9) : pop/needs_met/remplissage = baseline (38.4k vs 38.8k). Tunables registre J :
  `EXTRACT_GEO_REF` 4.5 · `EXTRACT_LABOR_SHARE` 0.65 · `FOOD_NEED` 1.0 · `SPAWN_FOOD_RAW` 12. ⚠ RE-BASELINE
  (hash 12 ans re-baseliné — golden mis à jour). `make test` 40/40 · determinism STABLE · **SAVE non bumpé**.
- **PIPELINE IA ÉCO — la prévision (l'IA n'est plus aveugle de ses flux) (2026-06-25)** : un pipeline de
  DÉCISION lu des coordonnées (jamais une hiérarchie de criticité codée). (É1 forecast) `econ_country_forecast`
  — runway/shortfall_proj/déficit STRUCTUREL par flux + food_runway, dérivés de pop/raw_cap/demande/offre/
  stock/eff_cap/needs_met. L'offre SUIT la pop (plus de bras) jusqu'au POTENTIEL de la tuile → un flux en
  équilibre RESTE en équilibre (seuls les déficits STRUCTURELS arment). Câblé dans `AiView.fc` (non sérialisé).
  (É2 priorités) `top_flow = argmax(stress(runway court) × prix × deficit_vs_safe)` — la motivation ÉMERGE.
  (É3a colonisation needs-aware) score = expansion CAPACITÉ (base, préserve la pop saine) + STEER borné vers
  une tuile riche d'un flux à déficit URGENT ; anti-spirale : crise FOOD + aucune source au gate normal →
  colonie de SURVIE vers la meilleure tuile vivrière (brise le poule-œuf). (É3b) relocate au shortfall
  PROJETÉ + grenier stock-safe sur `food_alert`. Tunables : `AI_SAFETY_HORIZON` 12 · `AI_PROJ_HORIZON` 25 ·
  `AI_SAFE_STOCK_MONTHS` 6 · `COLONY_SURVIVE_SEED` 0.5 · `AI_COLONY_NEEDS_W` 1.5. `make test` 40/40 ·
  determinism STABLE · **SAVE non bumpé** (AiView est un cache de tick).
- **HAMEAUX LIBRES (POLITY_WILD) — les Peuples Libres (2026-06-25)** : un rôle neuf (1 slot porteur) occupe N
  hameaux ÉPARS près des jouables (BFS multi-source ≤ `WILD_SPAWN_HOPS`) — tue le « siècle d'inertie » (2
  objectifs voisins dès l'an 0). Chaque hameau : pop `WILD_POP`±`WILD_POP_VAR` (≈750±, ~1500/empire), cap
  `WILD_CAP`, réserve `WILD_HOARD`, raw food FORCÉE (`WILD_FOOD` — il se nourrit). Culture DISTINCTE du
  voisin (race + ÉTHOS forcés ≠ l'empire adjacent) et AUCUNE religion (credo PLURALISTE, branche ANIMISTE)
  → enclaves ÉTRANGÈRES. PASSIFS (ai_on=false : ne conquièrent/colonisent/recherchent JAMAIS). Ralliement
  CULTUREL (la règle neuve) : contact PACIFIQUE soutenu avec l'empire voisin → owner→empire après
  `WILD_DEFECT_YEARS` OU convergence de race ; la culture distincte devient minorité → assimilation/
  xénophile-xénophobe. L'absorption MILITAIRE passe par la conquête (WILD = cible valide). Tunables registre
  J (`WILD_PER_PLAYABLE` 2, 0=off · …). Télémétrie « hameaux libres ». seed 9 : 4 semés, 4 ralliés/100 ans.
  `make test` 40/40 · **SAVE non bumpé** (owner/strata déjà sérialisés).
- **PIPELINE DIPLO IA — valeur SUBJECTIVE de province (étages 1-2) (2026-06-25)** : l'IA raflait la province
  la plus RICHE (valeur OBJECTIVE), pas celle dont elle a BESOIN. (É1) `ai_province_value` (diplo.c) = prix
  objectif (`diplo_province_price`) + BESOIN (Σ raw_cap × stress(runway de cid) × prix) + stratégique —
  DÉRIVÉE, aucun état stocké, la valeur ÉMERGE (le grenier vaut cher pour l'AFFAMÉ, rien pour le REPU). (É2)
  CONQUÊTE : `ai_pick_rival += AI_COVET_W × best_coveted_value` (on vise qui TIENT ce qu'on convoite) ;
  BUTIN : `diplo_settle` trie l'occupé par valeur SUBJECTIVE décroissante (l'affamé exige le GRENIER, le
  budget OBJECTIF borne). VALEUR = cible, ÉTHOS = méthode (appétits non écrasés). Banc INVARIANT
  anti-modificateur (diplo_demo 51/51). Tunables `AI_COVET_W` 0.5 · `AI_COMPLEMENT_W` 1.0. ⚠ RE-BASELINE
  (ciblage). `make test` 40/40 · determinism STABLE · **SAVE non bumpé**.
- **PIPELINE DIPLO IA — la VASSALITÉ SUR LA DURÉE (étage 3) (2026-06-25)** : la suite logique — la VALEUR
  choisit la CIBLE, l'ÉTHOS décide la MÉTHODE (tenir-et-traire vs DIGÉRER). Tout vit dans `diplo_suzerainty_tick`
  (annuel) sur des vassaux EXISTANTS. **(intégration)** un vassal TENU à la paix se rapproche de son maître
  (`v_integration[v]` [0..1], ∝ proximité culturelle RÉELLE `suz_culture_prox` (D∞ des axes PopCulture) ×
  appréciation (1−grief)) — une vitesse 1/`AI_VASSAL_INTEGRATE_YEARS` (20). **(contribution typée)** passé
  `AI_VASSAL_CONTRIB_GATE` (0.65, bond MÛRI), le vassal VERSE selon sa FONCTION (`vassal_function` : agraire→
  vivres au pool P1 / martial→`mil_stock` / commerce→`treasury`) × appréciation, à hauteur `AI_VASSAL_CONTRIB_BASE`
  de son potentiel — le canal va à la CAPITALE du maître. **(annexion-PROCESSUS)** un maître ANNEXEUR (éthos
  Dominateur/Honneur) DIGÈRE un vassal INTÉGRÉ (`AI_ANNEX_MIN_INTEGRATION` 0.65) : durée ∝ prix `country_price`
  × (1 − `ANNEX_INTEGRATION_DISCOUNT`·intégration), PAYÉE `AI_ANNEX_GOLD_PER_PRICE` or/an (sans or → s'essouffle) ;
  à `v_annex[v]`≥1, transfert de TOUTES ses régions au maître (`re->owner`, idiome `settle_transfer`) + `polity_death`
  + **cicatrice DOUCE** `re->annex_scar = ANNEX_SOFT_SCAR·(1−intégration)` (la voie patiente = bien intégré ⇒
  peu de plaie). **(annex_scar)** plaie [0..1] qui FRAPPE la STABILITÉ (`econ_tick` : satisfaction −= annex_scar·
  `ANNEX_SAT_W`), PAS la croissance (≠ revolt_scar), décroît ~5 ans (`ANNEX_SCAR_DECAY`) — surfacée dans le slot
  MODIFICATEURS (`PMOD_ANNEX_SCAR`, fléau, demo_bonus=0 ; `STR_PMOD_ANNEX_*` FR+EN). **DÉTERMINISME 12 ans
  INTACT par CONSTRUCTION** : tous les seuils (0.65) sont INATTEIGNABLES dans la fenêtre golden (intégration max
  12/20 = 0.60 < 0.65) ⇒ aucun flux moteur ne bouge avant l'an-12 (`make golden` IDENTIQUE, vérifié). Le mécanisme
  VIT au-delà (seed 9 : 1 annexion/200 ans — rare, BORNÉ, complète la conquête violente). ⚠ **SAVE BUMP 31→32**
  (DiploState +`v_integration`/`v_annex`/`n_annex` ; RegionEconomy +`annex_scar` ; `save_sane` borne les trois
  ∈[0,1] + suzerain∈[-1,n)). `suzerainty_tick` prend désormais un `World*` NON-const (l'annexion MUTE le monde).
  Tunables registre J (10). Banc diplo_demo (61/61, +10 : surfaçage·intégration lente·digestion). Télémétrie
  chronicle « annexion(s) par digestion ». `make test` 40/40 · determinism STABLE · golden IDENTIQUE.
- **TÉLÉMÉTRIE chronicle (2026-06-26) — prévision (éco) + guerres motivées (diplo)** : deux lignes
  d'observabilité (« la télémétrie est la preuve d'équilibre »). `prévision` (étages 1-2 éco) : N/N pays
  en déficit vivrier STRUCTUREL (ne se nourrit pas à plein → import vital) vs sous tension de runway,
  dérivé de `econ_country_forecast`. `guerres motivées` : déclarations PAR casus belli (la part
  ÉCONOMIQUE — convoitise étage 2 — vs territoriale/subjugation), compteur `g_war_cb` STATIQUE dans
  scps_diplo.c (RAZ par sim, jamais sérialisé/lu par le moteur). Golden IDENTIQUE (télémétrie hors-moteur).
- **#26 — OPINION ±100 À MÉMOIRE + `ai_consider_offer` (2026-06-26)** : le champ opinion ±100
  (`statecraft_opinion`) EXISTAIT mais était DÉCORATIF (lu par aucune décision IA ; projection memoryless
  d'une cible-relation persistante ±35). Refonte en DEUX couches — **« les relations TENDENT VERS 0 :
  decay naturelle »** : (1) **MODIFICATEURS DE STATUT** temporaires, calculés chaque tick, qui
  DISPARAISSENT à la rupture (alliance +`OPINION_ALLY`, guerre −`OPINION_WAR`, vassalité +, pacte +,
  embargo −, rancune territoriale −`OPINION_RANCOR_W`·rancor = la rivalité) → à la rupture, l'opinion
  ne tend plus vers +50 mais vers **0** ; (2) **MÉMOIRE D'ACTES** durable (`Statecraft.opinion_mem[][]`,
  sérialisée) qui décroît vers 0 sur des années (`OPINION_MEM_DECAY`) — la **TRAHISON**
  (`statecraft_on_betrayal` → opinion_mem) SURVIT au statut. La STRUCTURE (kinship/complément) reste
  dans `diplo_relation` (le « avec qui ») ; l'opinion porte l'HISTOIRE (le « après ce qu'il m'a fait »).
  **`ai_consider_offer`** (scps_ai.c, PUBLIC) : `to` ÉVALUE une offre de `from` (alliance/paix/pacte),
  lue de l'opinion (`AI_OFFER_*_OPINION`) + relation + `diplo_war_score` → accepte/refuse. Les alliances
  IA deviennent **BILATÉRALES** (`ai_pick_ally` ne retient qu'un candidat qui CONSENT). `sc==NULL` ⇒ pas
  de porte d'opinion (décision relation-seule ; les bancs passent NULL → INCHANGÉS). `ai_step`/
  `ai_strat_turn`/`ai_pick_ally` prennent le `Statecraft` ; `AI_DEMO_OBJS` gagne `scps_statecraft.o`.
  ⚠ **SAVE BUMP 32→33** (opinion_mem). ⚠ **RE-BASELINE diplo** (golden mis à jour : 4/5 graines bougent —
  les alliances changent). Les alliances **RESPIRENT toujours** (seeds 7/9/11/42 : 1-2 actives/100 ans,
  pas de deadlock). Tunables registre J (11 : `OPINION_ALLY` 50 · `_WAR` 60 · `_VASSAL` 30 · `_PACT` 15 ·
  `_EMBARGO` 25 · `_RANCOR_W` 8 · `_MEM_DECAY` 0.0003 · `_MEM_BETRAYAL` 35 · `_MEM_CAP` 100 ·
  `AI_OFFER_ALLY_OPINION` 10 · `_PACT_OPINION` 0). Bancs : statecraft_demo +4 (modèle : guerre creuse →
  paix remonte vers 0 ; trahison durable → s'estompe), ai_demo +3 (offre : opinion gate accepte/refuse).
  `make test` 40/40 · determinism STABLE (v33 save/reload byte-identique) · golden re-baseliné · ASan muet.
- **§3 — VERBES DIPLO JOUEUR (2026-06-26, capstone #26)** : le joueur DIPLOMATE — le journal de
  commandes (`CMD_*`, FIFO 64, drainé au point fixe de `sim_day`) gagne 5 verbes : `CMD_DECLARE_WAR`
  (CB dérivé · trêve respectée), `CMD_MAKE_PEACE` (paix BLANCHE), `CMD_OFFER_ALLIANCE`, `CMD_OFFER_PACT`,
  `CMD_EMBARGO`. **Capstone de #26** : OFFER_ALLIANCE/PACT/PEACE passent par `ai_consider_offer` (le
  vis-à-vis ÉVALUE via l'opinion ±100) → le joueur PROPOSE, l'autre CONSENT (ou refuse) ; declare_war/
  embargo sont unilatéraux. Façade additive `scps_player_declare_war/_make_peace/_offer_alliance/
  _offer_pact/_embargo` (scps_api.{h,c}) — ENFILENT (différé, déterministe) ; le verdict tombe au drain,
  lu ensuite dans `country_relations`. Chaque verbe REVALIDÉ au drain (cible ∈ [0,n), ≠ soi, vivante,
  pas en trêve) — miroir save_sane, jamais d'index périmé déréférencé. `enum` borné par `CMD_COUNT`
  (sim_cmd_push gate). **La chronique n'enfile jamais** (cmd_n=0) ⇒ drain no-op ⇒ **golden IDENTIQUE**,
  SAVE non bumpé, déterminisme intact. Banc scps_api_demo +3 (aller-retour : déclarer→guerre au drain ;
  paix/embargo/alliance enfilés+drainés sans crash). `make test` 40/40 · ASan muet · 0 warning.
- **§3 — SURFACE DE VERBES COMPLÈTE + bande d'OPINION (2026-06-26)** : la plomberie additive est
  BOUCLÉE. **13 verbes de plus** au journal (même motif : `CMD_*` + case revalidé au drain + façade
  `scps_player_*`) — INTÉRIEUR : `repress`/`assimilate`(creuset)/`purge` (→ `agency_order_*`),
  `council_hire`/`_dismiss` ; COMMERCE : `route` (→ `routes_order`), `market_buy`/`_sell`
  (→ `intertrade_market_*`) ; GUERRE : `campaign` (→ `campaign_order`, force = l'ost mobilisé
  `host->army[p]`), `posture`, `refill`, `navy_build`, `disband`. Chaque verbe REVALIDÉ au drain
  (région ∈ [0,n) ET au joueur · seat/hull/good bornés · trêve respectée) — miroir save_sane, jamais
  d'index périmé. **Bande d'OPINION** : `ScpsRelation` gagne `opinion` (±100, ce que l'AUTRE pense de
  nous, `statecraft_opinion`) → la membrane porte enfin l'opinion #26 au panneau diplo. **Chronique
  n'enfile pas** ⇒ drain no-op ⇒ **golden IDENTIQUE, SAVE non bumpé, déterminisme intact**. Banc
  scps_api_demo (24/24 : déclarer→guerre · opinion bornée · 13 verbes enfilés+drainés sans crash).
  `make test` 40/40 · golden IDENTIQUE · ASan muet · 0 warning. La couverture de VERBES joueur de la
  roadmap §3 est COMPLÈTE (build/recruit/levy/research + diplo + intérieur + commerce + guerre).
- **§3 — READS d'OPTIONS (légalité des coups, pour griser les boutons) (2026-06-26)** : `scps_diplo_options`
  (struct par CIBLE : `can_declare_war`/`_make_peace`/`_offer_alliance`/`_offer_pact`/`_embargo`/
  `_lift_embargo` + APERÇUS `would_accept_*` = `ai_consider_offer`, l'opinion #26 PRÉVISUALISÉE — l'UI
  montre « il refusera » AVANT l'offre) ; `scps_build_legal(region,edifice)` (le roster `debloque` gate
  la TECH, ce prédicat gate la RÉGION+l'OR : `!edifice_build_blocked` × `credit_can_spend(agency_build_gold)`).
  Les rosters de build (`debloque`) et d'unités (`recrutable`) existaient déjà → la légalité par TECH
  est couverte ; ces deux readers complètent par CIBLE / RÉGION. PURS READS (aucune mutation moteur) ⇒
  golden trivialement IDENTIQUE. Banc scps_api_demo +5 (29/29 : cohérence guerre⇔paix · aperçus de
  consentement bornés · build legal {0,1}). `make test` 40/40 · ASan muet · 0 warning. Reste : l'UI
  Godot qui CONSOMME verbes+options (GDScript, hors moteur), et d'éventuels options-readers fins
  (recruit/campaign/market) sur le même motif si besoin.
- **GODOT PHASE 5 (2026-06-26) — L'UI DIPLOMATIE JOUEUR (la membrane §3 DEVIENT JOUABLE)** : le tiroir
  Diplomatie (onglet 6, `sidebar_drawer.gd`) passait read-only (nom + statut) ; il CONSOMME désormais
  la surface §3. **Binding** (additif, `scps_sim_node.{cpp,h}`) : `diplo_options(target)→Dictionary`
  (la légalité par cible, pour griser), 5 verbes `player_declare_war/make_peace/offer_alliance/
  offer_pact/embargo→bool` (enfilent le journal déterministe), `opinion` + `country` (l'index cible)
  ajoutés au Dictionary de `country_relations`. `ScpsRelation` gagne `country` (POD façade,
  **NON sérialisé** → aucun bump). **Panneau** : chaque pays affiche nom · statut · **BARRE D'OPINION
  ±100** (vert favorable / rouge hostile depuis le centre, la mémoire #26 de SES actes envers nous) ·
  une rangée de boutons (Guerre · Paix · Allier · Pacte · Embargo) **GRISÉS par `diplo_options`** —
  un geste permis mais dont l'offre serait REFUSÉE (`would_accept` faux) s'affiche en AMBRE (« il
  refusera » avant le clic). Le clic émet le verbe → flash « ordre émis » (≠ accepté : les offres
  passent par `ai_consider_offer` au drain). **RÈGLE D'OR tenue** : zéro logique sim en GDScript — le
  panneau LIT la façade et ENFILE des verbes ; le déterminisme survit. Probe headless **`diplo_audit.{gd,
  tscn}`** (pendant de `viewer_audit` pour les verbes) : seeds 9/11/42 — opinion bornée [-100,100], index
  cible valide+unique, options cohérentes (jamais guerre ET paix offrables ; en guerre ⇒ paix offrable),
  et le **ROUND-TRIP** (déclarer la guerre MUTE vraiment le monde via le journal), + INVARIANT 0 (le
  panneau compile). **Moteur INCHANGÉ** (façade + binding + GDScript seuls) ⇒ golden/déterminisme intacts,
  **SAVE non bumpé**. `scps_api_demo` 29/29, `make smoke` 8/8, GDExtension `scons` 0 warning. ⊕ La
  roadmap **§3 est COMPLÈTE de bout en bout** : verbes moteur → façade → binding → panneau jouable.

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
