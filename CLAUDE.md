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
- **RIVIÈRES — retrait du throttle « héritage de l'ancienne gen » (2026-06-26, DISPLAY-ONLY)** : la
  carte ne montrait que **1-4 fleuves** (graine 11 : UN SEUL) là où le moteur en trace ~10-18 distincts.
  Cause : le carve `iso_ground._build_river_field` héritait du postulat de l'ANCIENNE worldgen (« elle
  sème des fleuves PARTOUT → on ne grave QUE les majeurs ») via un **rayon de fusion ÉNORME**
  (`RIVER_MERGE` 64 cellules sur une carte **1024×512**) qui agglomérait des BASSINS distincts, + un
  **cap** `RIVER_MAX_SYS` 5. Le moteur trace un réseau dendritique (tributaires tracés jusqu'à la mer ⇒
  des brins braidés partagent le même tronc/embouchure) — la fusion ne doit DÉDUP que ces braids, pas
  écraser les vrais bassins. **Retiré** : `RIVER_MAX_SYS` (cap supprimé) ; `RIVER_MERGE` 64→**5** (dédup
  des braids SEULEMENT) ; `RIVER_FLOW_FLOOR` 0.16→**0.04** (plancher anti-trace-morte, plus le seuil
  « majeurs »). On garde la machinerie de qualité (pinceau ∝ débit, tronc mont→mer, affluents clipés à la
  confluence, `RIVER_MIN_PTS` anti-stub). Résultat (gate replay `river_diag`, fidèle aux mêmes gates) :
  fleuves gravés graine 9 **3→13** · 11 **1→7** · 7 **4→16** · 42 **4→17** ; tous rendus (`_brush_for`
  carve au cœur ≥ 0.72, bien au-dessus du seuil shader). **`viewer_audit` VERT** (graine 9 : aucun décor/
  bâti dans l'eau de rivière malgré le réseau plus dense — l'anti-bâti suit). **GDScript seul** (le carve
  est display-only) ⇒ moteur/déterminisme/golden INCHANGÉS, **SAVE non bumpé**.
- **RIVIÈRES — hiérarchie FORCÉE connectée + dé-aridification + méandre jitté (2026-06-26, RE-BASELINE)** :
  refonte du tracé pour une carte parchemin lisible. **Moteur** (`scps_world.c`, `trace_rivers`) :
  hiérarchie FORCÉE — **N=6 fleuves · 2N rivières · 4N affluents** — chaque bras SEEK son parent par
  champ de distance BFS ⇒ tout est CONNECTÉ (plus de bras morts). **`river_dist_to_sea`** : champ BFS
  distance-à-la-mer (depuis TOUTE l'eau) SANS minimum local sur la terre ⇒ un fleuve qui le descend
  ATTEINT TOUJOURS l'eau (corrige « le fleuve ne se jette dans aucune mer » : l'ancien `river_fleuve`
  se coinçait dans une cuvette endoréique). `river_seek` unifié = descente propre (plus basse distance,
  départage altitude). **Dé-aridification** (`gen_climate`) : corridor riparien ÉLARGI (rayon 4, débit
  décroissant, `+0.55·riv`) ⇒ verdit les vallées partout où l'eau coule. **Membrane** : le MÉANDRE et la
  LARGEUR sont posés au RENDU (`iso_ground.gd._meander`, display-only, ré-itérable sans re-baseline) :
  Chaikin arrondit l'escalier D8, puis sinusoïde JITTÉE (2 sinus incommensurables + bruit cohérent
  FastNoiseLite) d'amplitude ∝ **platitude × BIOME** (plaine ouverte serpente, forêt quasi droite, relief
  nul, marais fort) — hors-axe NSEO, zéro angle droit ; largeur qui croît vers l'aval (affluents
  collectés) ; disque doux sous-pixel = AA massif. Carte parchemin : **glyphes arbre/pic RETIRÉS**
  (`iso_antique.gdshader`), lavis de terrain seuls, rivières en vedette.
  ⚠ **BUG GENÈSE LATENT corrigé** (`scps_econ.c`) : une région PORTEUSE de la capitale d'un empire
  pouvait être déclarée « infranchissable » (≥35 % d'aire morte) — la capitale est choisie pour l'EAU →
  siège CÔTIER → la région agrège assez de provinces mortes (côte/glacier) pour franchir le seuil ⇒ un
  empire (voire le JOUEUR) naissait SANS région colonisée. La dé-aridification ne CRÉAIT pas le bug, elle
  déplaçait juste quel pays devient `ord[0]`=joueur, l'EXPOSANT. Fix : la région-siège est EXEMPTÉE du
  verdict de zone morte (`is_cap[]`, le siège tient sur sa province habitable). ⚠ **RE-BASELINE** (le
  monde change : qui est empire/colonisé bouge ; **golden re-baseliné**, déterminisme STABLE). 4 bancs
  sensibles au monde recalibrés (intention préservée) : `audit_eco` (témoin = hameau SINON capitale) ·
  `econ_arcane_demo` (essence au POOL NATIONAL, pas la part pop-share P1) · `ai_demo` (Bâtisseur
  métabolise via un 3e canal, les CONSOLIDATIONS) · `scps_api_demo` (cible diplo VALIDE — possède des
  régions, rôle indifférent — au lieu d'un index fixe). `make test` **40/40** · ASan/UBSan muet · **SAVE
  non bumpé** (rien de sérialisé ne change ; le monde se régénère depuis la graine).
- **CRÉATEUR DE CULTURE — le PONT (le joueur compose son empire, façon Stellaris) (2026-06-27)** : le
  MOTEUR du créateur existait (héritages ≠ traditions, composition 1 majeur/1 mineur/1 défaut,
  `culture_random_build` pour l'IA) ; il manquait la chaîne joueur → moteur. **(1) Override moteur**
  (`scps_species.{h,c}`) : `culture_player_compose/bind/clear` + `culture_build_for(cid)` — un état
  GLOBAL « joueur » INACTIF PAR DÉFAUT. Les **5 sites** qui lisaient les traditions d'un pays
  (`scps_diplo.c` ×2, `scps_econ.c`, `scps_prosperity.c`, `scps_world.c`) passent de
  `culture_random_build(cid)` à `culture_build_for(cid)` : l'empire du JOUEUR joue SA composition,
  l'IA garde son tirage. **(2) Héritage** câblé via `scps_sim.c` (`worldgen_seed_peoples(..,
  culture_player_heritage())` ≡ `HERITAGE_ADAPTATIF` quand inactif) → pilote les NOMS + la couleur du
  pays joueur. **(3) Éthos** : override des régions + groupes du joueur dans `worldgen_seed_peoples`
  AVANT la passe de noms → l'épithète du pays (« Horde »…« Havre ») et les factions le suivent. **(4)
  Façade** `scps_api.{h,c}` (membrane : des MOTS + des SIGNES, jamais un levier brut) :
  `scps_heritage_list`/`_ethos_list`/`_tradition_list`/`_culture_validate`/`_culture_preview`/
  `_culture_name`/`_set_player_culture`/`_clear_player_culture` ; le cid joueur est LIÉ dans
  `scps_sim_generate` avant `sim_init`. **(5) Binding** `scps_sim_node.{cpp,h}` (8 passe-plats +
  `_bind_methods`). **(6) Fenêtre Godot** `ui/culture_creator.gd` (câblée dans `main.gd`, touche **C**,
  monde EN PAUSE à l'ouverture) : déroulants héritage + éthos + 3 traditions (une par axe), aperçu LIVE
  (nom de culture + leviers), garde de validité, graine, « Commencer » → `set_player_culture` +
  régénération. **DÉTERMINISME INTACT par construction** (tout est gardé par `culture_player_active()`
  faux par défaut ; le seul geste inconditionnel passe la MÊME valeur `HERITAGE_ADAPTATIF`) : **`make
  golden` IDENTIQUE**, **`determinism` STABLE**, **SAVE non bumpé** (aucune struct sérialisée ne
  change ; l'override est un état runtime, recalculé). Vérif : `scps_api_demo` **41/41** (prouve
  l'override de bout en bout : `joueur=Ésotérique`, pays « Havre Lórond ») ; probe Godot headless
  **`culture_audit`** (binding LIVE via `libscps` : listes 6/6/36, validation, aperçu, round-trip
  « Havre Lórond ») → **CULTURE AUDIT OK**. ⚠ Sur **Windows/MinGW** 3 bancs échouent HORS-SUJET
  (pré-existants, confirmés sur `main` vierge) : `intertrade_demo` (build : `setenv` est POSIX),
  `campaign_demo`/`warhost_demo` (`0xC00000FD` STACK_OVERFLOW : pile Windows 1 Mo vs 8 Mo Linux) — le
  reste **37/37** vert (sur Linux/cloud : **40/40**).
- **MENU + NOUVELLE PARTIE + SAUVEGARDE (2026-06-27) — le shell de jeu (Godot)** : un vrai menu
  (Jouer · Charger · Options · Quitter) → écran Nouvelle partie → chargement → PAUSE an 0.
  **(1) Sliders monde** : la façade `scps_worldgen_set/clear` + `scps_worldparams_default` route les
  champs RÉELS de `WorldParams` (taille tiny→huge = nb empires/cités, âge, continents, terres,
  montagnes, érosion, température, humidité) vers `scps_sim_generate` (override clampé, défaut =
  worldparams_default) ; `WorldParams` mémorisé dans `ScpsSim` (pour la save). seed 9 : petit(2 emp)=98
  rég · grand(10)=441. **(2) Culture par EMPIRE (slots, façon Stellaris)** : le mono-joueur `g_player`
  devient un table `g_slot[CULTURE_SLOTS]` + map `g_cid_slot` ; slot 0 = joueur, 1..N = IA
  (POLITY_ANTAGONIST) liés à la genèse par ordinal (`culture_bind_cid`) ; `worldgen_seed_peoples`
  applique héritage+éthos par empire ; `culture_build_for(cid)` lit le slot. Façade
  `scps_set_empire_culture(slot,…)` + `scps_country_role`. seed 9 : joueur slot 0 = « Havre Lórond »,
  IA slot 1 (cid 9) = « Horde Grukgor ». Compat `culture_player_*` (= slot 0) conservée.
  **(3) Sauvegarde PARTAGÉE** : le format (en-tête versionné, sections taguées, ChaCha20, save_sane)
  est EXTRAIT de `viewer.c` vers **`scps_save.{h,c}`** (sur le `Sim` MOTEUR) — ⚠ le viewer garde SON
  `Sim` distinct (jamais migré vers scps_sim.h) donc garde SA copie du save ; `scps_save` sert la
  FAÇADE. Nouvelle section **CULT** (`culture_slots_save/load` : slots + map cid→slot) ⇒ un monde
  chargé garde les cultures composées (joueur ET IA) → **SAVE_VERSION 36** (côté façade). Façade
  `scps_sim_save/load` + `scps_save_slots` ; le binding Godot expose worldgen/culture-slot/save +
  `country_role`. **(4) UI Godot** (zéro logique sim) : `menu_root.gd` (shell + écran Charger :
  3 emplacements, Sauvegarder/Charger), `new_game_panel.gd` (sliders + liste d'empires composables
  via `culture_creator.gd` en mode SLOT + seed → Lancer → regenerate → pause), `main.gd` démarre sur
  le menu (Échap rouvre). **Déterminisme INTACT** (slots/override inactifs par défaut ; chronicle ne
  lie pas scps_save) : `make golden` **IDENTIQUE**, `determinism` **STABLE**, `scps_api_demo` **54/54**
  (dont save/load aller-retour), probes Godot **MENU AUDIT OK** + **CULTURE AUDIT OK**. ⚠ Le viewer
  SDL ne se bâtit pas sur ce poste (SDL2_ttf absent de MSYS2) — sans rapport, `viewer.c` est INCHANGÉ.
- **RELIGION (2026-06-27) — module composable (16 pôles · 3 crédos), façon Stellaris/Civ, P1→P8** :
  un système de foi qui EXISTE, est NOMMÉ, PERSISTE et AGIT, branché côté FAÇADE/Godot (le front
  jouable). Tout est GATÉ sur « le pays a-t-il une foi » → la chronique n'en fonde JAMAIS ⇒ `golden`
  IDENTIQUE & `determinism` STABLE à TOUTES les phases. **P1** `scps_religion.{h,c}` (PUR) : 8 axes /
  16 pôles appariés (deltas Civ-tier) + 3 crédos RÉUTILISÉS de `scps_culture.h`
  (PLURALISTE/EVANGELISTE/PURIFICATEUR ; `RELIG_CREDO[]` aligné) + primitives spawn/schism/apply/
  color + `religion_selftest` (banc `religion_demo`). **P2** i18n (relig_axis/pole_name + tips,
  scholar roles — littéraux façon credo_name). **P3** persistance : lien pays→religion en GLOBAL du
  module (PAS la struct pays partagée ⇒ viewer/déterminisme intacts), section RELG du save FAÇADE
  (`scps_save`, SAVE_VERSION 36→**39** au fil des phases). **P4** la foi MORD : cache d'accumulateur
  par pays + nudges NON DESTRUCTIFS gatés (prosperity K/P/H/L ; démo popgrowth) + éligibilité schisme
  (RUPTURE). **P8** religion par RÉGION (granularité moteur : adapté de « province ») : héritage +
  FRACTURE (régions distantes/peu légitimes basculent à l'enfant) + la D-interne religieuse abaisse la
  satisfaction des régions MINORITAIRES → alimente l'agitation/sécession EXISTANTE. **P6** le LETTRÉ
  (Missionnaire/Gourou/Moine, face=crédo) : tick quotidien (CONVERT répand · STABILIZE exempte du malus
  · RESIST bloque la bascule), sérialisé. **P7** l'IA : schisme RUPTURE (repick aléatoire valide,
  auto-résolvant) + emploi du lettré. **P5** UI GODOT (le prompt visait `viewer.c` SDL mais le front
  jouable est Godot ET le viewer ne LIE pas sur ce Windows — `-lGL`/`-ldl` Linux-only, pré-existant,
  PAS SDL2_ttf qui est en fait PRÉSENT) : façade listes/verbes + binding `scps_sim_node` + `religion_panel.gd`
  (touche R : fonder crédo+3 pôles axes distincts, schisme, readout) + probe `religion_audit` (**RELIGION
  AUDIT OK** via binding live). ⚠ RIPPLE : econ/prosperity/ai/sim appellent religion ⇒ 26 OBJS de bancs
  gagnent `scps_religion.o`. `make test` **38/38 runnable** (3 KO PRÉ-EXISTANTS Windows : intertrade
  `setenv`, campaign/warhost stack-overflow — prouvés sur main vierge ; 41/41 attendu Linux/cloud).
  SDL `viewer.c` religion UI = différé (front non jouable + non-buildable ici).
- **RELIGION — fondation par ÉDIFICE + plafond mondial + IA & headless (2026-06-27)** : le monde
  naît ATHÉE ; une foi n'apparaît qu'au **1er édifice religieux** (sanctuaire/temple/cathédrale/
  monastère, lu de `re->edi_built`). Côté JOUEUR (Godot) : `scps_religion_founding_ready` déclenche le
  **créateur de foi** (`main.gd` sur `Sim.ticked`, monde en pause, une fois). Côté **IA + sim headless** :
  `ai_strat_turn` FONDE une foi aléatoire au 1er édifice religieux **sous le PLAFOND mondial**
  `religion_cap = ⌈n_empires/3⌉` (4 empires ⇒ 2 ; `religion_root_count` = racines `parent==-1`) ; au-delà,
  l'empire **RALLIE** une foi existante (`religion_adopt_existing`) — les empires se PARTAGENT les
  religions. Le joueur idem (`scps_religion_found` crée sous le cap, rallie au-delà ; UI : bouton
  Fonder→« Rallier », `religion_can_found`). ⚠ **RE-BASELINE golden ASSUMÉE** : les religions ÉMERGENT
  désormais dans la chronique (1 graine sur 5 fonde une foi < 12 ans → son hash change ; les 4 autres
  INCHANGÉES) — `golden_hashes.txt` mis à jour. **`determinism` STABLE** (fondation IA déterministe :
  graine cid×jour). `make test` 38/38 runnable (les 3 KO restent les pré-existants Windows). Probe
  `religion_audit` : monde athée au départ, plafond=2, RALLIE au-delà.
- **RELIGION — ÉMERGENCE EN CHRONIQUE : zèle proactif + plafond TOTAL ancré + autel humble (2026-06-27)** :
  la foi N'ÉMERGEAIT PAS en sim headless (0.4 foi/sim — uniquement la sacralisation de CRISE, légitimité
  < 30 %). Trois gestes, mesurés sur un long balayage (`./chronicle 9 5 250 6 12`, EXIT 0, 59 s, monde
  SAIN — satisfaction 63/74/77, ~40 guerres/sim, inflation 1.24, hégémon mortel 5/5). **(1) ZÈLE
  PROACTIF** (`scps_ai.c`, bloc bâtiment civil) : un crédo prosélyte (évangéliste/purificateur ⇒
  `w_faith` ≥ `AI_FAITH_ZEAL` 0.5, registre J) qui n'a pas de foi bâtit son PREMIER sanctuaire DE
  LUI-MÊME → la fondation se déclenche (au lieu d'attendre une crise de L). On LIT `w_faith` (l'entrée
  moteur dérivée du crédo), jamais un bonus plat ; borné à UN chantier (athée + faith<1 + aucun chantier
  de foi en file — `agency_build` est ASYNCHRONE, anti-spam par scan de la file `AGY_BUILD`). **(2) AUTEL
  HUMBLE** : le Sanctuaire (`EDIFICES[]`) passe de {bois 35, argile 20} à **{bois 12}** — le monde NU
  (N1) manque d'argile (dispo ~3), ce qui ÉTRANGLAIT la fondation partout (gate matière `SCPS_GATEDIAG`) ;
  Temple/Cathédrale gardent la pierre (la progression). **(3) PLAFOND sur le TOTAL, ANCRÉ À LA GENÈSE** :
  l'ancien `religion_cap` bornait les RACINES → les schismes multipliaient les foi SANS LIMITE (44/sim au
  test). Neuf : `religion_can_create(N) = (g_religion_count < ⌈N/3⌉)` borne le TOTAL (racines + schismes),
  et N = empires de **GENÈSE** (`religion_set_empire_ref` posé par `sim_init`, `religion_empire_ref` lu par
  l'IA ET la façade) — « 6 empires ⇒ 2 religions » même quand les sécessions fragmentent le monde. Gate
  appliqué à la fondation ET au schisme (au plafond : la foi en exil PERSISTE, pas de foi de plus ; la
  façade grise le bouton schisme via `scps_religion_eligible`). ⚠ **RÉSULTAT** : 0.4 → **2.0 foi(s)/sim**
  (= ⌈6/3⌉), 0 schisme (cap plein), **6.4 pays fidèles/sim** (les empires PARTAGENT les 2 foi), 2.4 régions
  minoritaires, 8 guerres de religion/sim. **TÉLÉMÉTRIE chronicle** neuve « religion » (racines · schismes ·
  pays fidèles · régions minoritaires). ⚠ **RE-BASELINE** (3/5 graines fondent < 12 ans → `golden_hashes.txt`
  mis à jour) · **`determinism` STABLE** (zèle/fondation déterministes : w_faith + cid×jour) · **smoke 8/8**,
  bancs liés (ai/agency/api/religion/diplo/…) **tous verts** · **SAVE non bumpé** (l'état religion vit dans
  les globals du module + section façade RELG, déjà sérialisée). Dialable : `SCPS_TUNE=AI_FAITH_ZEAL=…`
  (0 = tout le monde fonde) · l'autel reste une ligne d'`EDIFICES[]`. Bug latent corrigé au passage :
  `sim_init` appelait pas `religion_reset()` → les foi FUYAIENT entre les sims d'une même chronique (multi-sim).
- **BRUT DE BÂTI — argile/pierre RENDUES À LA GÉOLOGIE + RAW-WORKS + CONFORT + garantie joueur (2026-06-27)** :
  la « carte nue » (N1) ÉCRASAIT argile & pierre (coupe `REGION_RAW_KEEP=2`) → AUCUNE source géologique → les
  chantiers (sanctuaire compris ⇒ religion) STAGNAIENT. Refonte de l'approvisionnement du brut de construction :
  **(1) GÉOLOGIE rendue** — `prot[RES_CLAY]=prot[RES_STONE]=true` dans la coupe : l'argile (terres d'eau :
  marais/tourbière/mangrove) et la pierre (relief : collines/hauts/montagnes/pic/volcan/height>0.55) SURVIVENT
  comme tout stratégique rare. Source BON MARCHÉ par EXTRACTION (part journalière 0.65) → ne CONCURRENCE pas la
  main-d'œuvre de manufacture (le vrai verrou : tout-manufacturé affamait vin/étoffe → bonheur en chute).
  **(2) GARANTIE JOUEUR** — `econ_guarantee_player_construction` (appelée par `sim_init` après capitales+adjacence) :
  si le biome n'a donné NI argile NI pierre dans le rayon 1-2 de la capitale, on en FORCE une tuile de chaque
  (`PLAYER_GUARANTEE_RAW` 4) — la construction jamais hors de portée. **(3) RAW-WORKS** (3 manufactures hors-sol,
  `in1=RES_NONE`, indépendantes de la tuile) : **four à brique→argile · carrière→pierre · scierie→bois**,
  rendement **100 ouvriers → 60/60/120 par mois** (qout 0.60/0.60/1.20), nées substantielles (`RAW_WORKS_LEVEL`
  10), bâties par le **FORECAST** (`econ_country_forecast` flagge un déficit STRUCTUREL du trio quand le BÂTISSABLE
  — stock+surplus, PAS la capacité géo — < `RAW_WORKS_NEED` 120 ; `ai_build_rawworks` dans le créneau manuf
  civile, ordre de bootstrap BOIS→argile→pierre, la scierie sans coût-bois). Le SUPPLÉMENT des régions
  pauvres + la chaîne confort ; coût normal (upkeep), AUCUNE logique magique (market_effort standard).
  **(4) CONFORT** : **poterie** (argile→poterie, confort journalier/bourgeois) + **atelier de sculpture**
  (pierre→statuaire, confort bourgeois/élite) — NORMAUX (`§NF` price-driven), CONSOMMENT argile/pierre (⇒ la
  demande qui ENTRETIENT les raw-works). Leur satisfaction est un **BONUS HORS-PANIER** (`COMFORT_JOY` 0.08/bien
  servi → bonheur AU-DESSUS du panier, AUCUNE pénalité si absent) + une **−15 % de besoin de LOGEMENT**
  (`COMFORT_HOUSE_RELIEF`, `econ_region_effcap`) quand servies. **(5) FORECAST resources** : le trio construction
  câblé dans `EconForecast` (signal stock+surplus, demande LATENTE). ⊕ Résultat (seed 9, 5×250) : satisfaction
  **66/74/78 (AU-DESSUS du socle 63/73/75 — bonheur UP)**, religion 2 racines + 3.6 schismes/sim, monde STABLE
  (revolt 20, sécession 16, inflation 1.23, hégémon mortel 5/5). ⚠ **SAVE BUMP 39→40** (RES_POTTERY/STATUE +
  3+2 BLD types ⇒ RegionEconomy grandit). ⚠ RE-BASELINE (golden mis à jour) · determinism STABLE · smoke 8/8 ·
  bancs liés tous verts. Tunables registre J : `RAW_WORKS_LEVEL`/`_WOOD`/`_NEED` · `COMFORT_JOY`/`_HOUSE_RELIEF`
  · `PLAYER_GUARANTEE_RAW`.
- **PLAFOND RELIGION refondu — RACINES ≤ ⌈N/3⌉ + ≤ 2 SCHISMES PAR RACINE (2026-06-27)** : l'ancien `religion_can_create`
  bornait le TOTAL (racines+schismes) à ⌈N/3⌉ → les schismes (P8) étaient ÉTOUFFÉS (0/sim). Neuf : `religion_can_found`
  (racines < ⌈N/3⌉) + `religion_can_schism(parent)` (descendants de la RACINE-ancêtre < `RELIG_SCHISM_MAX` 2, via
  `religion_root_of`). « 6 empires ⇒ 2 foi fondatrices, chacune jusqu'à 2 sectes » → la P8 REVIT bornée (2 racines +
  ~3.6 schismes/sim). Gates câblés IA (`scps_ai.c`) + façade (`scps_api.c` found/eligible/schism). **SAVE non bumpé**
  (état religion déjà sérialisé). RE-BASELINE incluse dans le golden ci-dessus.
- **PRÉVISION DIPLO — la menace ENTRANTE + le besoin d'allié (2026-06-27)** : l'IA voyait QUI la menace (threat) mais
  jamais « qui va m'attaquer, suis-je couvert ». `DiploForecast` (cache de tick, NON sérialisé, `scps_ai.c`) dérivé
  PUR des coordonnées (`diplo_relation`/`mil`/`eco_power`/`war_score`/`momentum`/`ambient_threat`) : `war_risk`
  (= pire menace entrante / (ma puissance + menace ambiante)), `alliance_need`, `war_outlook`. Deux conso GATÉES
  (deadband ⇒ fenêtre golden INTACTE) : (1) `ai_aggression` FREINE l'offensive quand `war_risk > AI_THREAT_GATE`
  (0.55) — anticiper la coalition au lieu d'ouvrir un 2e front ; (2) `ai_pick_ally` SURPONDÈRE l'allié qui COUVRE
  ma pire menace quand `alliance_need > 0.5`. ⊕ **`golden` IDENTIQUE** (les seuils ne sont pas atteints en 12 ans),
  determinism STABLE, guerres 40→39/sim (moins de surextension). Tunables : `AI_THREAT_GATE`/`_BRAKE` · `AI_WAR_LOSING`
  · `AI_ALLY_NEED_W`. **SAVE non bumpé**.
- **REFONTE ÉCO #2 — LEVIER `labor` (100 emplois → ~1000 hab) + BASSIN J+B + ALLOCATION joueur (2026-06-27)** :
  recalibrage demandé (« une manufacture sert 900-1200 hab ; 100 emplois = 100 emplois, qu'ils soient artisans ou
  bourgeois »). **Cause racine MESURÉE** (workflow + lecture source) : la manufacture produit **par-tick SANS ×dt**
  (`scps_econ.c` sortie `lim·qout`), l'extraction est annualisée (`×dt`) → un ouvrier de manufacture rendait **12×**
  un ouvrier d'extraction au même coefficient (la « redondance massive »). Le bon levier N'EST NI le `×dt` (casse les
  ratios + re-baseline militaire/arcane/endgame) NI baisser `qout` (casse les ratios intrant:sortie) mais **`labor`**
  (ouvriers/lot) : productivité = `12·qout/labor`, ratio = `q1:qout` (indépendant de labor) → on règle « combien de bras
  pour 1000 hab » SANS toucher ni ratios ni qout. Appliqué aux STAPLES de panier : **bière labor 0.8→27 · vin 0.9→38 ·
  tunique 0.8→28 · poterie 1.0→46** (`= 1200·qout/demande_1000`) ; les biens NICHE (papier/statue/sel/fourrure) restent
  efficaces (cibler 1000 hab y est absurde). **BASSIN J+B** : `labor_avail = JOURNALIERS + BOURGEOIS` (l'élite ne
  travaille pas) — le geste « 100 emplois = 100 emplois » ; corrige le double-compte rpop (`rp_`) + `elab` national.
  **bois 0.5→1.0** (le feu est un bien DIRECT, calé par le rendement). **FRUITS** (`RES_FRUIT`, brute un peu partout +
  forêt, protégée de la coupe) = repli du VIN (compense le sucre rare). ⊕ Mesuré seed 9/150 ans : **satisfaction
  75/75/81 (≥ baseline 66/74/78)**, boisson servie ~90 % — le levier rend les manufactures labor-intensives SANS casser
  le monde. `EXTRACT_LABOR_SHARE` testé 0.45 → **remis 0.65** (0.45 n'aide pas la boisson — limitée par la réserve
  vivrière locale — et baisse la satisfaction).
  **ALLOCATION DE MAIN-D'ŒUVRE (onglet province, item joueur)** : override par RÉGION du split AUTO. `RegionEconomy`
  gagne `alloc_on`/`alloc_raw[RES_PROD_FIRST]`/`alloc_bld[BLD_TYPE_COUNT]`/`bld_input[BLD_TYPE_COUNT]` (⚠ **SAVE BUMP
  41→42**). `alloc_on=0` (DÉFAUT) ⇒ AUTO inchangé (déterminisme préservé hors-override) ; `alloc_on=1` ⇒ le bassin est
  réparti par les POIDS (extraction + manufacture, un seul budget), poids 0 sur un bâtiment = FERMÉ, `bld_input` force
  le repli (alt1). FAÇADE : `scps_region_alloc` (reader : puits + poids + part suggérée en AUTO) + 4 verbes journalisés
  `scps_player_alloc_{raw,bld,input,auto}` (revalidés au drain : région à soi, bornes) + `scps_province_count`. Binding
  Godot (`region_alloc`→Dictionary + 4 verbes) + onglet **« Main-d'œuvre »** (`province_detail.gd` : barres %, [−]/[+],
  Fermer/Ouvrir, choix d'intrant, ↻ Auto — pousse l'allocation COMPLÈTE pour ne pas zéroter les autres puits ; lecture
  seule hors de ses régions). Probe headless `alloc_audit` (round-trip read→override→fermer→auto, seeds 11/42 OK).
  **IA = AUTO** : un override IA proportionnel a été codé puis **MESURÉ inférieur** (66-69 % vs 75 % auto — le glouton
  AUTO fait COULER les bras vers qui peut les employer ; un poids fixe en gâche) → **retiré** ; l'AUTO prix-driven EST
  l'allocation avisée de l'IA, l'override reste l'outil du JOUEUR (cf. garde-fou « keep only if ≥ auto »).
  ⚠ **RE-BASELINE golden** (le levier + le bassin J+B mordent dès l'an-0 ; golden mis à jour) · `determinism` **STABLE**
  (save/reload v42 byte-identique) · `make test` **38/40** (cap_demo RECALIBRÉ — fixture dotée large : le banc teste le
  CAP, pas la main-d'œuvre ; les 3 KO restants sont les pré-existants Windows : intertrade `setenv`, campaign/warhost
  stack) · `scps_api_demo` **91/91** (+7 alloc) · GDExtension `scons` **0 warning** · sweep 5×250 SAIN (24-45 pays, IPM
  1.24, §27 an-180, hégémon mortel 5/5). Tunables INCHANGÉS sauf les `labor` de recette (code) + `EXTRACT_LABOR_SHARE`
  0.65 (registre J). `resource_name`/color : +Fruits/Poterie/Statuaire.
- **FRUITS — forêt-nourriture + repli vin CHER (2026-06-28)** : `RES_FRUIT` (brute, agricole). Au départ « un peu partout »,
  RAMENÉ à **FORÊT/BOIS/JUNGLE SEULEMENT** (`base·0.65`, protégé de la coupe) pour ne PAS voler les bras d'extraction au
  grain. **NOURRITURE DE SUBSTITUTION** : `res_is_food(FRUIT)`=vrai + un FOOD-FILL après le panier (le fruit du pool
  comble le déficit vivrier RÉSIDUEL grain/poisson, 1:1, sans AJOUTER de besoin) → relève `food_sat` là où il pousse +
  crée la demande de fruit (fin du pile-up). `EXTRACT_YIELD[FRUIT]`=4.0 (« 100 emplois → 400 hab » à la tuile standard ;
  geo-modulé ⇒ ~230 forêt). Repli VIN à intrant ÉLEVÉ (winery `alt1_q` 4.0 : 4 fruits/vin vs 1.6 sucre → le fruit reste
  d'abord de la nourriture). Forecast vivrier inclut le fruit. ⚠ Coût mesuré : **−3 journalier** (sous PRIX NATIONAL ;
  c'était −10 sous prix régional — la distribution régionale AMPLIFIAIT le coût) pour 0 famine d'import.
- **MÉTAL SUPPRIMÉ — outils fer+bois DIRECT, coques au CUIVRE (2026-06-28)** : `RES_METAL` n'avait que DEUX consommateurs
  (taillanderie + coques navales). Le chaînon intermédiaire est RETIRÉ DÉFINITIVEMENT : `RES_METAL` et `BLD_FOUNDRY`
  sortent des enums ⇒ ⚠ **SAVE BUMP 42→43** (`RES_COUNT` change → tableaux `[RES_COUNT]` de `RegionEconomy` rétrécissent).
  **BLD_TOOLWORKS** : `1 fer + 1 bois → 3 OUTILS` (direct). **Coques** (`scps_navy.c`) : `HullCost.metal`→`copper`,
  les navires de guerre coûtent du **CUIVRE** (clous/doublage). Récompenses de mission métal→fer. Bancs recâblés
  (`econ_production_demo` réécrit chaîne directe, `navy_demo` cuivre, agency/ai/structural fer). `TOOLS_PER_LABORER`
  0.15→0.015→**0.05** (sous prix national, on remonte la cible pour un bonus prod RÉEL : outils ×2.4, stock ×2.4,
  bonus ~+4.4 %, plafonné par la rareté du fer). `resource_name`/color/strings `STR_RES_METAL` retirés.
- **PRIX NATIONAL — fin de l'artefact de distribution régionale (2026-06-28, le gros morceau)** : le STOCK était déjà
  national (pool P1) mais le PRIX était soldé PAR RÉGION sur sa pop-share du pool (`avail = S[r]*pshare + supply[r]`) —
  un ARTEFACT spatial : un bien produit dans 1 région flambait dans les autres (outils à ~7×base côté consommateur vs
  bas côté producteur ; boisson du journalier mal servie). Le workflow a identifié que le prix régional était le
  garde-fou anti-effondrement (mélanger demande régionale / stock national ferait ÷N → plancher 0.2 → effort 0.42 →
  prod −60 %). FIX : le prix est soldé **UNE FOIS par EMPIRE** sur `demand_nat[c]/(pool[c]+supply_nat[c])` (mêmes
  PALIERS ⇒ ratio invariant à l'échelle : ni artefact, ni effondrement), puis PROJETÉ sur `re->price` de toutes ses
  régions (matérialisation, comme `re->stock`). Accumulateurs `supply_nat`/`demand_nat` (statiques) sommés de l'offre/
  demande locales ; le solde par-région est GARDÉ pour `owner<0` (fixtures/hors-empire → prix local, bancs INCHANGÉS).
  `re->price` reste le store sérialisé ⇒ **AUCUN bump SAVE** (Option A) ; `re->stock` INTACT ⇒ les ~280 lecteurs externes
  cohérents. ⊕ Résultat : prix UNIFORME par empire (artefact disparu), journalier **63→69**, pop 189k (pas d'effondrement),
  prix outils moyen 63→~45 absolu. ⚠ **RE-BASELINE golden** (le prix mord dès l'an-0 sur les empires multi-régions ;
  `golden_hashes.txt` mis à jour) · `determinism` **STABLE** · bancs **37/37** runnable verts (3 KO pré-existants Windows) ·
  sweep 5×250 SAIN (satisfaction 66/76/83 an-250 post-Grand-Hiver, hégémon mortel 5/5, §27 an-180, IPM 1.22).
- **MÉTABOLISATION — Temps 1 : le creuset DIGÉRÉ accélère la recherche (2026-06-28)** : premier étage du
  mécanisme d'accès-tech par éthos/héritage. « Incorporer d'autres gens dans SA culture FONCTIONNE » au sens
  ACTIF — pas l'hétérogénéité de NAISSANCE (post-GR4 les régions d'un empire portent des héritages hash-assignés
  ≠ la capitale ; leurs natifs NE comptent PAS), mais les **NOUVEAUX VENUS** (DIASPORA : migrants + captifs de
  conquête, posés à `integration=0`) d'un AUTRE héritage, **à mesure qu'on les DIGÈRE**. Helper neuf
  `econ_country_metabolized(w,econ,cid)` (scps_econ.c) = TWIN-INVERSE de `econ_off_culture_fraction` : Σ
  (diaspora ∧ heritage≠natif) count·integration / Σ count ∈ [0..1], pondéré par les ÂMES (200 digérés ≠ 1000).
  La recherche en tire un boost — `income ×= 1 + W·métabolisé` — dans `ai_research_step` (IA) ET la voie joueur
  (`scps_sim.c`, no-op chronique : `human_player=-1`). Un captif fraîchement pris (integ 0) ne rapporte RIEN ;
  il rapporte en se métabolisant (= digérer). **Complément** de la diffusion par COMMERCE existante (S1/
  `tech_sync_tick` : le négoce ouvre les nœuds syncrétiques peu profonds jusqu'au seuil PROFOND) — la
  métabolisation est le canal ACTIF, le commerce le canal PASSIF. Membrane : `ScpsCountryInfo.metab_pct`
  (le +X% de recherche, pour le hover sous la barre de savoir). Télémétrie chronicle « métabolisation »
  (empires creuset · moyenne · max → +% au plus métabolisé). ⚠ **RE-BASELINE golden** (la migration est VIVE
  dès l'an-0 → le creuset digéré atteint ~25 % à l'an-12 → le boost mord ; `golden_hashes.txt` mis à jour) ·
  `determinism` **STABLE** (le boost est pur état sérialisé) · ai_demo 26/26 · scps_api_demo 91/91 · sweep 5×250
  SAIN (satisfaction ~70/78/84, hégémon mortel 5/5, **§27 toujours gaté an-180** — le boost NE tire PAS l'apocalypse,
  arbre 64-71 %/empire encore DIFFÉRENCIÉ ; le lever décolle : moyenne 4-8 %, **max +48 %** au plus métabolisé).
  Tunable registre J : `AI_METAB_RES_W` 1.0 (⇒ « métabolisation X% = +X% recherche », lisible) — dialable d'une
  ligne. **SAVE non bumpé** (rien de sérialisé ne change ; le signal se recalcule). À VENIR (Temps 2, re-baseline) :
  la BARRE D'ACCÈS continue (le seuil binaire `PROFOND` de `ai_heritage_access` → access_bar par tier) + la REMISE
  de prix (tech qu'un autre empire possède = moins chère) ; puis coût en N-provinces, reorg tiers 1-5, UI Medusa.
- **ÉTOFFE DE L'ARBRE — 12 branches culturelles d'héritage tier 1-2 (2026-06-28)** : prérequis de la barre de
  métabolisation. L'arbre n'avait que **7 signatures** (≈1/héritage, TOUTES tier-3) → « X% d'un éthos ⇒ tech R/S/T
  par tier » n'avait RIEN à graduer. On donne à chaque héritage une vraie montée **tier 1→2→3** : 2 nœuds
  PEU PROFONDS neufs (`native=héritage`, `faustian=false`, charge/flux faibles) menant vers sa signature tier-3.
  **Branches PARALLÈLES** (la signature garde son prérequis d'origine ; ce sont des rungs que la barre — Temps 2 —
  ouvrira par tier : peu de métabolisation → t1, plus → t2, beaucoup → la signature t3). Les 12 (conçus par
  workflow 6-designers + critique d'équilibre adversarial, deltas calés sur l'échelle des tier-1/2 existants) :
  **Ésotérique** Glyphes éthérés→Communion éthérée (Savoir·Renf) · **Métallurgiste** Alliages des profondeurs→
  Gravure runique (Forge·Armée) · **Mécaniste** Rouages de précision→Mécanisme d'horlogerie (Forge·Prod→Renf) ·
  **Adaptatif** Droit coutumier→Langue franque (Société·Renf) · **Agraire** Vergers étagés→Pâturages intégrés
  (Société·Prod) · **Clanique** Rites guerriers→Hordes conquérantes (Société·Armée). Enums APPENDUS (index stable ;
  `tech_heritage_affinity` inchangé — les signatures précèdent dans l'ordre) ; `NODES[]` les place par initialiseur
  désigné. ⚠ **SAVE BUMP 43→44** (+12 ⇒ `TECH_COUNT` grandit ⇒ `TechState.unlocked[TECH_COUNT]` change de taille).
  ⚠ **RE-BASELINE golden** (les natifs recherchent leurs nouveaux rungs — l'arbre vit ; golden mis à jour) ·
  `determinism` **STABLE** · `tech_demo` 22/22 · suite **38 runnable verts** (3 KO pré-existants Windows) · 0 warning ·
  sweep 5×250 SAIN (hégémon mortel 5/5, §27 gaté an-180, arbre 51-60 %/empire encore DIFFÉRENCIÉ, satisfaction
  ~67/75/82). À VENIR (Temps 2) : la barre de métabolisation gate ces rungs par tier + la remise de prix.
- **MÉTABOLISATION — Temps 2a : la BARRE D'ACCÈS GRADUÉE (2026-06-28)** : l'accès tech cesse d'être BINAIRE.
  Le masque `heritage_access` (un `unsigned`) encode désormais **2 bits/héritage = le TIER d'accès atteint (0..3)** ;
  `tech_can_research` exige `tech_heritage_access_tier(access, native) >= node.tier` (la signature tier-3 ne s'ouvre
  qu'à tier 3, les rungs ÉTOFFE tier-1/2 à leur tier). Le tier par héritage = **MAX de deux voies** : (1) la
  PROFONDEUR de contact (`ai_archetype_depth`, déjà graduée : commerce SURFACE→tier 1, frontière/foi MÉTIER→tier 2,
  gouvernance digérée PROFOND→tier 3) — « les techs s'échangent par le commerce JUSQU'À UN SEUIL » ; (2) la
  MÉTABOLISATION active (`econ_country_heritage_digested` : part d'âmes diaspora digérées de CET héritage ≥ T1/T2/T3
  ⇒ tier 1/2/3) — « incorporer ce peuple ouvre ses techs ». L'héritage NATIF = plein (tier 3). `tech_heritage_bit`
  garde sa sémantique d'OCTROI PLEIN (tier 3) pour les bancs/helpers. Câblé : `heritage_access_pack` (scps_ai.c, lu
  par `ai_heritage_access` ET la voie recherche) ; combos S3 (Forge runique) et orphelin de readout passent au tier.
  ⊕ Effet : le COMMERCE ouvre enfin les rungs peu profonds d'un voisin (Venise lit la Grèce sans la conquérir), la
  signature profonde restant réservée à la gouvernance/métabolisation — le canal ACTIF (Temps 1) et le canal PASSIF
  (commerce) convergent dans UNE barre. ⚠ **RE-BASELINE golden** (l'accès gradué mord dès l'an-0 : le commerce ouvre
  des rungs étrangers ; golden mis à jour) · `determinism` STABLE · tech_demo 22/22 · ai_demo 26/26 · scps_api_demo
  91/91 · readout_demo 27/27 (orphelin tier-aware) · sweep 5×250 SAIN (satisfaction 66/77/82, hégémon mortel 5/5,
  §27 gaté an-180, arbre 55 %/empire — l'accès gradué ENRICHIT sans inonder l'arbre ni déstabiliser). Tunables
  registre J : `METAB_TIER1` 0.10 · `_TIER2` 0.20 · `_TIER3` 0.35. **SAVE non bumpé** (l'accès se recalcule ;
  `arch_depth` déjà sérialisé, sens inchangé). À VENIR (Temps 2b) : la REMISE de prix (tech qu'un autre empire
  possède = moins chère) ; puis coût en N-provinces, reorg tiers 1-5, UI Medusa.
- **EFFETS TECH CONCRETS + MATRICE DE COMBOS tier-4 (2026-06-28)** : « que font les techs ? » — audit + réponse.
  ⚠ **Découverte** : `TechState.eco`, `.mil`, `.F` sont des champs **MORTS** (jamais lus) → `dEco`/`dMil`/`dF`
  ne font RIEN (ni mes nœuds, ni les techs existantes). Les leviers VIVANTS : `K`→prospérité · `L`→stabilité &
  croissance (`croissance = δ·P·L/10`) · `puissance`→prospérité+Brèche · `H`→coercition · `fracture`/`charge`/
  `flux`→Brèche · **`NODE_PROD_PCT`/`EFF_PCT`**→+production/+efficacité · **`army_doctrine`** (compte les techs
  FN_ARMÉE × tier : Forge→+dégâts, Société→+moral, Savoir→+magie) · chaîne Savoir·Prod→+recherche.
  **(1) Étoffe recâblée sur les leviers vivants** : les 12 rungs zéro-tent leurs champs morts ; les FN_ARMÉE
  (Alliages/Gravure→+dégâts, Rites/Hordes→+moral) passent par `army_doctrine` (PAS la prod — correction d'une
  mauvaise étiquette), les FN_PRODUCTION (Rouages/Vergers/Pâturages) par `NODE_PROD_PCT`, l'Horlogerie par
  `NODE_EFF_PCT`, les Savoir·Renf (Glyphes/Communion) par la prospérité. **Effet JOUEUR** affiché par tech
  (`TECH_UTILITY`, ex. « +dégâts d'armes », « +stabilité », « +recherche »). **(2) MATRICE DE COMBOS** — 15
  paires d'héritages (Forge runique existait ; +14 neuves), chacune un **nœud tier-4 COMBINATOIRE EXCLUSIF**
  recherchable seulement avec l'**accès PLEIN (tier 3) aux DEUX héritages** (natif OU métabolisé) — symétrie :
  métaboliser B ET C ouvre B×C même si on n'est ni l'un ni l'autre. Réutilise le motif `tech_combo_native`
  (native=A, combo renvoie B) + plafond de tier dans `tech_can_research` (`need = min(tier,3)` ⇒ tier-4 exige
  accès plein). Effets via leviers vivants (army_doctrine pour les combos militaires Poudre/Poliorcétique/Siège/
  Chamanisme/Foederati/Horde ; `NODE_PROD_PCT`/`EFF_PCT` pour Automates arcanes/Druide/Guildes/Charrues/Machines
  agricoles/Horlogerie marchande ; recherche pour Académie ; prospérité pour Grenier colonial). ⚠ **SAVE BUMP
  44→45** (+14 nœuds ⇒ `TECH_COUNT` grandit). ⚠ **RE-BASELINE golden**. `determinism` STABLE · suite **38 runnable
  verts** (3 KO pré-existants Windows) · 0 warning · sweep 5×250 SAIN (satisfaction ~65/75/80, hégémon mortel 5/5,
  §27 gaté an-180, **combos VIVANTS : 15-32 empires tiennent une fusion/sim**). Télémétrie chronicle « combos
  tier-4 ». Hook par-unité (vrai « +X% arquebusiers » ciblé) + apex triple (Arquebuse runique Méca×Métal×Éso)
  = différés (le `weapon_power` large suffit). À VENIR : coût en N-provinces, reorg tiers 1-5, UI Medusa.
- **COÛT DES TECHS EN √N-PROVINCES — wide récompensé (2026-06-28)** : le coût de recherche cesse d'être ∝ POP
  (size-neutral : ancien `popf = pop/POP_REF`, plancher 0.5) pour devenir **∝ √N** (N = provinces de l'empire).
  Le revenu de recherche monte DÉJÀ ∝ pop (∝ N) ; en scalant le coût ∝ **√N** (sous-linéaire), le coût MARGINAL
  d'une province reste INFÉRIEUR à son apport → **l'EXPANSION est récompensée** (rythme/empire ∝ N/√N = √N),
  mais sans snowball (le coût croît quand même). `tech_cost(id, n_provinces) = BASE_COST[tier]·COST_SCALE·
  max(FLOOR, K·√N)` (`#define` dans scps_tech.c, le module reste PUR — pas de tune_f) ; tunables `TECH_COST_N_K`
  **0.90** · `_EXP` 0.5 (=√N) · `_FLOOR` 0.5. **k=0.90 calé sur les GRANDS empires** (N~28 ⇒ coût ≈ l'ancien
  popf des gros — re-baseline BORNÉ là où vit l'essentiel de la pop) ; les petits paient ~1.8× plus (tall
  relativement freiné = la contrepartie du wide récompensé). Mesuré (NDIAG, gated `SCPS_NDIAG`) : N=1 ⇒ ~15 techs,
  N=28 ⇒ ~26 — **corrélation taille→tech POSITIVE mais sous-linéaire**. Tous les appelants passent désormais
  `w->country[cid].n_regions` (ai_pick_tech/ai_research_step, sim/viewer voie joueur, façade `scps_tech_*`,
  `tech_tree_readout`, bancs). ⚠ **RE-BASELINE golden** · `determinism` STABLE · 0 warning · tech_demo **23/23**
  (+1 : « ×N provinces ne ×N pas le coût ») · readout/ai/api verts · sweep 5×250 SAIN (satisfaction 69/79/85,
  hégémon mortel 5/5, §27 gaté an-180, IPM 1.23 ; **monde plus EXPANSIONNISTE** — guerres 33→45/sim, l'expansion
  payant, mais les soupapes tiennent). **SAVE non bumpé** (rien de sérialisé ne change). Dialable d'une ligne
  (`TECH_COST_N_K` ↑ pour freiner l'expansion, ↓ pour la pousser). À VENIR : reorg tiers 1-5, UI Medusa.
- **GODOT — UI MEDUSA : arbre tech + barre de métabolisation (2026-06-28)** : le graphe d'arbre Medusa EXISTAIT
  (`ui/tech_panel.gd` : Atomes radiaux angle=quartier/rayon=tier, couleur=état, clic→recherche, en-tête points/
  présage/crise) ; on le COMPLÈTE à la vision métabolisation. **Façade** (additive) : `ScpsTechInfo.metab_pct`
  (le « +X% recherche » du creuset, lu de `econ_country_metabolized`) + `scps_player_heritage_access` →
  `ScpsHeritageAccess[6]` (par héritage : tier d'accès 0-3 décodé du masque gradué Temps 2a + part diaspora
  digérée + natif). **Binding** : `tech_info` Dict +`metab_pct`, méthode `heritage_access()`→Array, buffer
  `tech_nodes` **64→96** (⚠ l'arbre fait désormais **71 nœuds** > 64 → sans le bump, 7 nœuds dont des combos
  TRONQUÉS). **Panneau** : bande de MÉTABOLISATION sous le graphe — « Métabolisation : +X% recherche » + les
  6 héritages en **barre de progression** (3 pips de tier remplis selon l'accès + % digéré + ★ natif) : la
  lecture directe de « digérer un peuple OUVRE ses signatures par tier ». ⚠ **Correctif rename** : le binding +
  `province_panel.gd` référençaient encore `.race`/`["race"]` (le rename race→héritage 92db155 n'avait pas
  touché le Godot, scons ne buildant pas d'office ici) → `heritage`/« Héritage ». ⚠ **Env** : la jonction
  `godot/godot-cpp` était un fichier-symlink cassé (28 o) → recréée vers `E:\JEUX\SCPS\godot\godot-cpp`.
  GDExtension `scons` **build OK** (`libscps...dll`) · `scps_api_demo` 91/91 · probe headless **`tech_audit`
  (TECH AUDIT OK** : 71 nœuds, métab +0..43 %, 1 natif tier-3, accès 0-3 bornés sur seeds 9/11/42). Moteur
  INCHANGÉ (façade+binding+GDScript) ⇒ golden/determinism/SAVE intacts. La roadmap tech est **JOUABLE de bout
  en bout** (moteur → façade → binding → panneau Medusa). Reste : reorg prix tiers 1-5 (cosmétique), remise de
  prix inter-empires, hook par-unité, apex triples.
- **MÉTABOLISATION — Temps 2b : la REMISE DE PRIX PAR DIFFUSION (le 3e effet, triade complète) (2026-06-28)** :
  « une tech déjà déverrouillée par un AUTRE empire coûte moins cher » — le savoir DIFFUSE, la (re)découverte
  d'un savoir RÉPANDU est plus facile (catch-up des retardataires). `g_tech_diff[TECH_COUNT]` (scps_ai.c) = nb
  d'empires VIVANTS qui possèdent chaque tech, recompté CHAQUE TICK par `tech_diffusion_refresh(w, ts, n)`
  (appelé de `sim_day` ET du viewer) — DÉTERMINISTE, non sérialisé (fonction PURE des TechState). `tech_diffusion_mult(id)`
  = `1 − AI_TECH_DIFFUSE_MAX·(possédants/vivants)` ∈ [0.6, 1] (registre J : `AI_TECH_DIFFUSE_MAX` **0.40** ⇒ tech
  que TOUS les autres ont = −40 %). Câblé via `ai_effective_cost` (= √N × biais éthos × remise) qui REMPLACE les
  6 sites de coût de l'IA (`ai_pick_tech` + les 5 épargnes de `ai_research_step`) ; la voie JOUEUR (sim.c + viewer)
  et le coût AFFICHÉ (façade `scps_tech_nodes`) appliquent la même remise (le prix montré = le prix payé). À g_tech_diff=0
  (bancs sans refresh) ⇒ mult=1 (aucune remise) ⇒ **bancs INCHANGÉS**. ⚠ **RE-BASELINE golden** (le savoir diffuse
  dès l'an-1). `determinism` STABLE · 0 warning · tech_demo 23/23 · ai_demo 26/26 · scps_api_demo 91/91 · sweep 5×250
  SAIN (satisfaction 70/77/83, hégémon mortel 5/5, §27 gaté an-180 par construction, **arbre 39→44 %/empire** — le
  catch-up VIT ; télémétrie « remise diffusion » : 40-43 tech(s) escomptée(s), max −40 %). **SAVE non bumpé** (g_tech_diff
  recalculé). ⊕ La **TRIADE de métabolisation est COMPLÈTE** : déverrouille (barre d'accès Temps 2a) · accélère
  (boost Temps 1) · escompte (cette remise). Dialable d'une ligne (`AI_TECH_DIFFUSE_MAX`). Reste (cosmétique/différé) :
  reorg prix tiers 1-5, hook par-unité, apex triples.
- **APEX TRIPLES tier-5 + HOOK PAR-UNITÉ (2026-06-28)** : le pinacle de l'arbre — la fusion de TROIS héritages.
  Le mécanisme combo passe à **N=3** : `tech_combo_native2` (3e héritage) ; `tech_can_research` exige l'accès
  PLEIN (tier 3) aux TROIS (natif OU métabolisé). 3 apex (tier-5, prereq=NONE, la porte = la triple-métabolisation
  + le coût tier-5) : **Arquebuse runique** (Méca×Métal×Éso, Forge·Armée) · **Concile des savants** (Éso×Adaptatif×Méca,
  Savoir·Prod → +recherche+efficacité) · **Légion universelle** (Adaptatif×Métal×Clanique, Société·Armée → +moral).
  **HOOK PAR-UNITÉ** (le vrai « +X% arquebusiers » ciblé que tu voulais) : `ArmyDoctrine` gagne `firearm_power` ;
  `unit_power` le multiplie sur l'ARQUEBUSIER (comme `arcane_power` sur le mage) ; `army_doctrine` le pose si
  l'Arquebuse runique est acquise (`APEX_FIREARM` 0.50 = +50 % dégâts arquebusier, en PLUS du weapon_power large).
  ⚠ **SAVE BUMP 45→46** (TechState +3 nœuds ⇒ TECH_COUNT ; ArmyDoctrine +firearm_power ⇒ ArmyState block sérialisé
  `sizeof` change). ⊕ **golden IDENTIQUE** (golden-safe : un apex exige l'accès plein à 3 peuples = jamais atteint
  en 12 ans ; le firearm ne mord que l'apex acquis) — PAS de re-baseline. `determinism` STABLE · 0 warning ·
  tech_demo 23/23 · army_demo 48/48 · ai_demo 26/26 · scps_api_demo 91/91 · sweep 5×250 SAIN (satisfaction
  70/77/83, hégémon mortel 5/5, IPM 1.22). `firearm_power` en `#define` local (scps_army.c, comme FORGE_STEP —
  pas de tune_f dans ce module). ⊕ **La roadmap tech/héritage est COMPLÈTE** (déverrouille/accélère/escompte +
  étoffe + combos paires + apex triples + coût √N + UI Medusa). ⊕ **Lisibilité par tier** : des ANNEAUX DE TIER
  (concentriques, display-only) ajoutés au panneau Medusa (`tech_panel.gd._draw_tier_rings`) — l'intent « arbre
  lisible par tier » de C, SANS toucher BASE_COST. Le reorg des PRIX (changer BASE_COST) reste volontairement
  non fait : il re-baselinerait l'équilibre pour un gain purement cosmétique (signalé, non un oubli).

- **MODTOOLS — palier 1 : surcharge des VALEURS ÉCO par fichier (2026-06-28)** : 1er étage d'un modtools
  durable (« éditer les prix/valeurs sans passer par une recompile »). Le motif `tune_f`/`scps_lang.txt`
  (défaut compilé + override OPT-IN, golden-safe sans override) étendu aux TABLES éco. `BASE_PRICE[]` et
  `EXTRACT_YIELD[]` passent **non-const** (valeurs compilées INCHANGÉES) ; `econ_moddata_dump(FILE*)` écrit un
  TSV **name-keyed** (robuste au réordonnancement d'enum, TAB-séparé car les noms ont des espaces) ;
  `econ_moddata_load(path)` applique les surcharges, appelé à `econ_init` **si l'env `SCPS_MODS` pointe un
  fichier** (sinon vanilla). `chronicle --dump-data` écrit le point de départ éditable. ⊕ **golden IDENTIQUE +
  determinism STABLE** (sans `SCPS_MODS` ⇒ valeurs compilées ⇒ rien ne bouge ; vérifié : golden re-confirmé
  identique APRÈS un run modé — le fichier ne fuit pas) ; **SAVE non bumpé** (les tables ne sont pas
  sérialisées, lues au tick). 0 warning. Round-trip prouvé (dump → édite Céréales prix→999 → `SCPS_MODS` →
  « [mods] 1 ressource surchargée »). Guide : **MODDING.md** (les 3 canaux : `SCPS_TUNE` ≈168 · `SCPS_MODS`
  éco · `scps_lang.txt` strings + assets Godot ; et comment AJOUTER du contenu = enum+table+1 recompile).
  À VENIR : étendre `SCPS_MODS` aux recettes/tech/unités · codegen de contenu (manifeste → enums/tables) ·
  panneau dev Godot (sliders live).
- **MODTOOLS — palier 2 : `SCPS_MODS` étendu aux recettes / tech / unités (2026-06-28)** : le canal fichier
  passe au format **TAG-keyed** unifié (une ligne = un objet, préfixée de sa table) couvrant TOUTES les valeurs
  en table. `RECIPE`/`BASE_COST`/`NODE_PROD_PCT`/`NODE_EFF_PCT`/`UNITS` passent **non-const** (valeurs compilées
  INCHANGÉES). Chaque module (econ/tech/army) a son `*_moddata_dump/load` (copie statique de `*_split`, fopen
  par module — décuplé, pas de nouveau module ni dépendance croisée) ; l'APP (chronicle/façade/viewer) charge
  les 3 si `SCPS_MODS` est défini (l'ancien hook `econ_init` retiré ⇒ pas de dépendance econ→tech/army, bancs
  intacts). Tags : `price` (base/yield) · `recipe` (labor/qout) · `basecost` (tier→coût) · `techbonus`
  (prod/eff) · `unit` (disc/moral/mvt/cmd). `chronicle --dump-data` écrit les 3 tables. ⊕ **golden IDENTIQUE +
  determinism STABLE** (sans `SCPS_MODS` ⇒ valeurs compilées ; vérifié + re-confirmé après un run modé) · **SAVE
  non bumpé** (tables non sérialisées) · 0 warning · bancs econ/tech/army/api verts · round-trip 3 tables prouvé
  (recipe/unit/basecost surchargés → 3 messages `[mods]`). MODDING.md mis à jour. À VENIR : codegen de contenu ·
  panneau dev Godot.
- **MODTOOLS — palier 4 : le PANNEAU DEV Godot (tunables EN DIRECT, F10) (2026-06-28)** : le 3e canal
  `SCPS_TUNE` (~168 scalaires du registre J) devient ÉDITABLE sans relancer. **Énumérateur** (`scps_tune.{h,c}`,
  additif) : `tune_count`/`tune_name_at`/`tune_value_at`/`tune_default_at`/`tune_overridden_at` parcourent
  `g_reg[]` (le registre X-macro existant) ; `tune_set` (déjà là) marque `overridden` et reconstruit la chaîne
  active. **Façade** (`scps_api.{h,c}`) : `ScpsTunable {nom,value,def_value,overridden}` + `scps_tune_count`/
  `scps_tune_at`/`scps_tune_set_val` (GLOBAL, pas par-sim — le registre est process-wide). **Binding** Godot
  (`scps_sim_node`) : `tunables()→Array[Dict]` + `tune_set(nom,value)`. **Panneau** `ui/devpanel.gd` (touche
  **F10**, câblé dans `main.gd`) : liste filtrable du registre, une LineEdit par tunable, Entrée applique la
  surcharge LIVE (l'effet apparaît là où le moteur relit `tune_f` au tick) ; `*` marque le surchargé. RÈGLE
  D'OR tenue (GUI → façade, zéro logique sim ; on ÉDITE des coefficients que le moteur LIT déjà). Probe headless
  **`devpanel_audit`** (168 tunables énumérés ; `tune_set` mute la valeur ET bascule `overridden`). ⊕ **golden
  IDENTIQUE** (chronicle n'appelle jamais `tune_set` ; tout est additif/pur) · GDExtension `scons` 0 warning ·
  **SAVE non bumpé** (le registre n'est pas sérialisé). Un monde modé en direct n'est plus rejouable vanilla
  (comme `SCPS_TUNE`). ⊕ Le modtools est COMPLET sur ses 3 canaux : valeurs (`SCPS_MODS` fichier + F10 live) ·
  chaînes (`scps_lang.txt`, F4) · contenu (`gen_content.py` + 1 recompile).

- **GODOT — CARTE PARCHEMIN UNIQUE (2026-06-28) : un seul rendu, zéro asset de carte** : le front
  passe à un rendu de carte UNIQUE — le shader cartographique `iso_antique.gdshader`, **100 %
  procédural** (lit les SEULES couches moteur `biome_map` + `river_map` + un bruit GÉNÉRÉ ; lavis
  sépia, côtes à l'encre, marais, rivières à la plume, relief en lavis, rose des vents, bords
  brûlés). **Retraits** : la vue GLOBE 3D (SubViewport/Camera3D/sphère), le splat iso 3D
  (`iso_blend.gdshader`), les falaises micro-mesh 3D (`cliff_3d.gd`), le backdrop eau
  (`water.gdshader`), la roche peinte (`cliff_rock.gdshader`). `map_view.gd` réécrit : ISO unique,
  zoom continu, `fit` cadre la carte entière (remplace l'overview globe). **Projection top-down
  LÉGÈREMENT INCLINÉE** (Y comprimé `TILT_Y`=0.80 via l'échelle du nœud IsoGround → sol & overlay
  restent alignés, pas un hack par-acteur). **Assets LARGUÉS** (le shader n'en a aucun besoin) :
  **359 fichiers** supprimés — `pack/{cities,centres,structures,clutter,dressing,campaign,bridges,
  foundations,rivers,iso_tiles}` ; **GARDÉS** : `pack/buildings` (icônes des boutons de
  construction), `pack/resources` (chips UI), `ui/{icons,chrome}` (boutons + barres de rendu) —
  conformément à « tout sauf les sprites de boutons et les barres ». Le bruit du shader devient
  **PROCÉDURAL** (`NoiseTexture2D` seamless, FastNoiseLite) → plus de `blend_noise.png`. **Overlay**
  réécrit : les acteurs sont en **ENCRE vectorielle** (zéro sprite carte) — villes = glyphes (cercle
  crème cerné d'encre, capitale étoilée), routes 3 passes, frontières (pays/régions), **noms
  d'empire** (taille écran constante), armées (losange + anneau de phase + ligne de marche),
  épicentre §27 (anneaux pulsants). **DISPLAY-ONLY** : moteur C / déterminisme / save **INTACTS**
  (aucun fichier C touché). Régressions mineures assumées : surbrillance de province sélectionnée
  sur le sol (à remettre en contour d'encre dans l'overlay) · rose des vents très légèrement ovale
  sous l'inclinaison. ⚠ Re-baseline NULLE (rien de sérialisé). **Code mort PURGÉ** : `iso_ground.gd`
  1074→294 lignes (atlas terrain/falaise/route, carve-brush, city-wear — tout le splat iso 3D) ;
  `overlay.gd` 2063→892 lignes (45 fonctions : builders de villes/structures/clutter/nature/ponts/
  tuiles-route + leur chaîne de teinte terrain + le chemin globe) ; ~1860 lignes display-only retirées.
  Bruit du parchemin rendu PROCÉDURAL (NoiseTexture2D). Vérifié au rendu (rivières/routes/villes/
  frontières/noms/armées intacts).
- **GODOT parchemin — FRONTIÈRES CALLIGRAPHIQUES + couleur par empire (2026-06-28)** : refonte des
  outlines pour la lecture parchemin. **Façade additive** `scps_border_segments_col(level)` (+ struct
  `ScpsSegC{…,owner}`) : le balayage bseg TAGGE chaque segment par l'owner (pays) → l'overlay groupe
  par entité. **Deux tiers** : **1px TOUTES les provinces** (niveaux 0+1, trame fine encre fanée, en
  **LOD** — fond en survol, se révèle au plan ; toutes restent tracées) · **3px les BLOCS d'empire**
  (niveau 2) **COULEUR PAR ENTITÉ** (`_empire_ink` = couleur pays foncée), en **TRAIT DE PINCEAU**
  (`_ink_brush` : pile de passes translucides du large plumé au cœur dense, TOUTES antialiasées →
  feutre le crénelage des arêtes de cellule = effet brosse/encre mouillée ; la trame fine reçoit un
  feutrage léger 2 passes). **Effet plume/calligraphie** : wobble déterministe ∝ position
  (`_jit`, même point monde → même offset → segments JOINTS, pas de trou) + antialiasing. **Noms
  d'empire SANS boîte** (fond transparent) — encre directe + halo papier doux (« écrit à la plume »).
  Binding `border_segments_col`→Dictionary {pts, owner}. **DISPLAY-ONLY** : `scps_api.c` n'est pas
  dans chronicle ⇒ **golden IDENTIQUE**, déterminisme/save intacts. GDExtension `scons` 0 warning.
  Tunable d'overlay : `BORDER_JIT` 0.4.
- **GODOT parchemin — ROUTES en POINTILLÉ sépia (2026-06-29)** : les routes quittent le tracé plein
  3-passes pour un **pointillé brossé** — `_dash_poly` découpe chaque polyligne en tirets (marcheur par
  MULTIPLE de période, `k` croît strictement → terminaison garantie ; remplace une 1re version `fmod`
  qui bouclait sur un piège de précision flottant), tous les tirets cumulés en UN batch, dessinés en
  **2 passes** en **sépia** (`ROAD_INK`). **ALLÉGÉ « carte au trésor » (2026-06-29)** : le gros web brun
  (cœur 2.2px + halo 4.2px qui comblait les trous) devient un **POINTILLÉ FIN de DIRECTION** — cœur
  **1.2px** + halo MINUSCULE (2.1px, ne comble plus les trous), tirets COURTS / gaps OUVERTS
  (`ROAD_DASH` 3.5 · `ROAD_GAP` 5.5), α 0.72 → fine trace en pointillé, plus de réseau lourd.
  Tunables d'overlay : `ROAD_INK` · `ROAD_DASH` · `ROAD_GAP`. (Le `_ink_brush` 5-passes est
  réservé aux blocs d'empire — sur le réseau routier dense entier il étranglait le rendu.) DISPLAY-ONLY,
  moteur intact.
- **GODOT parchemin — frontières affinées + noms en forme de pays (2026-06-29)** : 4 retouches lecture.
  (1) **CÔTES invisibles** : `scps_border_segments_col` (façade) n'émet plus les joints pays touchant la
  MER (le rivage du shader suffit) ; chaque segment porte désormais `owner` ET `other` (le voisin). (2)
  **HACHURES inter-empire** : un joint qui touche un AUTRE empire (`other`>=0) reçoit un tick
  perpendiculaire (hachure) en plus du trait — la marche (terre libre) reste un trait seul. (3) **NOMS
  qui SUIVENT LA FORME** : axe principal par **ACP** des centroïdes de région PROJETÉS (Chili vertical,
  Russie en travers) — orienté seulement si élongation > 1.8 ; ancre = barycentre (hors hubs routiers) ;
  ×1.35 pour la lisibilité ; toujours sans boîte. (4) **anti-blob** : le trait d'empire passe du pinceau
  5-passes (core 2.6/feather 7) à un **trait fin** (core 1.3 + halo réduit) ≈ moitié ; `BORDER_JIT`
  0.4→0.25. ⚠ `ScpsSegC` gagne `other` (façade/binding). DISPLAY-ONLY : `scps_api.c` hors chronicle ⇒
  **golden IDENTIQUE** ; déterminisme/save intacts. scons 0 warning. Tunable overlay : `HATCH_LEN` 1.1.
- **GODOT parchemin — passe de finition (2026-06-29)** : 5 retouches. (1) **Routes** : jitter de
  LONGUEUR par tiret (0.45..1.55×) + **noise directionnel** (offset perpendiculaire par tiret, `_h1`
  hash) → tirets « tracés à la main » (plus des points uniformes) ; encre **NOIRE** peu opaque (≠ la
  trame de provinces sépia) (`ROAD_INK` α 0.52, `ROAD_WOBBLE` 0.7). (2) **Frontières d'empire qui « se
  coupaient »** : les longs ticks de hachure noyaient le trait → **trait CONTINU** (1.6px) au-dessus
  d'un halo, hachures **COURTES** (`HATCH_LEN` 0.8) **espacées** (1 sur 2) et SUBORDONNÉES (α 0.7) ;
  `BORDER_JIT` 0.25→0.18. (3) **Shader rivière** : **adoucissant** (champ débit moyenné 5 taps) +
  **SEUIL** plus haut (smoothstep 0.14-0.40, sous le seuil rien ne s'imprime → plus de « bancs de sable »
  pâles) + **blend à la MER** (sur l'eau salée la rivière se fond, α 0.28) ; côte LISSÉE en continu
  (`ring_solid` proximité, plus de bandes discrètes). (4) **WILD** (`scps_world.c`) : NOM **TRIBAL
  ÉTHOS-DÉPENDANT** (« Barbares/Maraudeurs/Clan/Tribu/Marchands libres/Peuple libre XX ») + **COULEUR
  distincte** (recolorés depuis leur héritage WILD réel) — le slot réservé n'avait qu'un nom générique.
  ⚠ Nom/couleur = champs d'AFFICHAGE (pas dans le hash) ⇒ **golden IDENTIQUE** (vérifié). DISPLAY +
  worldgen-display-only ; déterminisme/save intacts. scons 0 warning.

- **GODOT parchemin — frontières en RUBAN façon Civ + cités-états or-argent (2026-06-29)** : refonte
  des outlines. (1) **Provinces** : trame fine NOIRE + fort feutrage (3 passes) + jitter accru
  (`FINE_JIT` 0.5) → l'escalier des arêtes se fond (plus de marches). (2) **Routes** : sépia CLAIR (≠
  provinces noires), tirets à longueur JITTÉE + wobble directionnel (`_h1`, `ROAD_WOBBLE`). (3) **Blocs
  (empires + cités-états)** : RUBAN dégradé INTÉRIEUR→EXTÉRIEUR — façade `scps_border_segments_col`
  gagne une **normale extérieure** (`ScpsSegC.nx,ny`) par segment ; l'overlay décale l'**inline** (ton
  CLAIR, côté intérieur, large) et l'**outline** (ton FONCÉ, côté extérieur, fin). Teinte EMPIRE par
  **ÉTHOS** sur l'axe ordre↔chaos (`scps_country_ethos` → 0..5 ; `ETHOS_HUE` : chaos chaud/rouge →
  ordre froid/bleu) ; **CITÉS-ÉTATS or↔argent** (et la façade GARDE leur côte — exception au « côtes
  invisibles » — pour que le ruban se voie). ⚠ `ScpsSegC` +nx,ny ; binding +`country_ethos` +"nrm".
  DISPLAY/façade hors chronicle ⇒ **golden IDENTIQUE** ; déterminisme/save intacts. scons 0 warning.

- **HAMEAUX LIBRES — UN SLOT PAR HAMEAU (entités politiques DISTINCTES) (2026-06-29)** : avant, TOUS
  les hameaux libres partageaient UN seul slot-pays WILD → en vassaliser/rallier un les entraînait
  tous. Désormais on réserve **un slot WILD par hameau** (`worldgen` : `need = WILD_PER_PLAYABLE ×
  nb_jouables` slots UNCLAIMED passés WILD) et `econ_init` rattache **un hameau par slot** (compteur
  `wnext` sur `wslots[]`) → chaque hameau est une ENTITÉ avec son **id + nom + couleur** propres ;
  les slots réservés non plantés (pas de terre viable) repassent UNCLAIMED dans `worldgen_seed_peoples`
  (w non-const). Le **nom tribal éthos-dépendant** (« Barbares/Maraudeurs/Clan/Tribu/Marchands libres/
  Peuple libre XX ») est posé par slot (plus de `break`). Vassalisation/ralliement/conquête désormais
  INDÉPENDANTS par hameau. ⚠ **RE-BASELINE golden** (les `region.owner` des hameaux passent d'un index
  unique à des index distincts) ; `determinism` STABLE ; sweep SAIN (seed 9 : 10 entités WILD distinctes
  à l'an-0, monde stable, hégémon mortel). `econ_init` reste `const World*` (le revert UNCLAIMED se fait
  côté worldgen). `scps_country_role`==4 = WILD.

- **GODOT parchemin — bandes de frontière BLENDÉES (culture×éthos) + capitale pourpre + noms uniques (2026-06-29)** :
  3 retouches. (1) **NOMS UNIQUES** : `place_make_name` gagne une terminaison (`NAME_END[8]`, 8×4×8 =
  256 variantes/héritage, ex-32) + une **passe de DÉDUP** dans `worldgen_seed_peoples` (re-tire le CORE
  d'un nom dupliqué jusqu'à unicité, copie bornée 0-warning) → 0 doublon (vérifié seeds 9/11/42, mondes
  HUGE couverts). (2) **RUBAN BLENDÉ** (`_draw_band`) : N=5 couches du ton EXTÉRIEUR au ton INTÉRIEUR,
  décalées le long de la normale (px écran ÷ zoom) et teintées par `lerp` → un VRAI dégradé, pas deux
  traits. **OUTLINE = CULTURE** (héritage, 6 familles `HERITAGE_HUE` + variation RGB par pays via `_h1`) ;
  **INLINE = ÉTHOS** (axe martial↔ordre `ETHOS_INLINE_HUE`, fluide) ; cités-états or↔argent. Façade
  `scps_country_heritage`. (3) **CAPITALE POURPRE** : un liseré (bande pourpre) autour de la
  province-capitale de chaque empire — façade `scps_country_capital_region` + `scps_region_border_segments`
  (contour d'une région + normale). Tout DISPLAY/façade (hors chronicle) + noms NON hashés ⇒ **golden
  IDENTIQUE** ; déterminisme/save intacts. scons 0 warning.

- **GODOT parchemin — palette de PIGMENTS limitée (anti-néon) + capitale en LISERÉ fin (2026-06-29)** :
  les bandes blendées tiraient leurs teintes de la roue HSV (`HERITAGE_HUE`/`ETHOS_INLINE_HUE`) → des
  bleus/magentas FLUO même désaturés, et le ruban (5 couches à α 0.95) s'empilait en stries SATURÉES =
  effet « cyberpunk ». Deux gestes (overlay.gd, GDScript SEUL). (1) **PALETTE CURÉE** : `HERITAGE_PIG`
  (outline, 6 ENCRES sombres terreuses choisies à la main — ardoise/rouille/sienne/olive/ocre/prune) +
  `ETHOS_PIG` (inline, 6 lavis CLAIRS de la même gamme — terre cuite/sable/ardoise pâle/céladon/ocre
  pâle/sauge), variation par pays sur la **VALEUR seule** (`_shade`, jamais la teinte → pas de dérive
  néon) ; cités-états or/argent FANÉS. (2) **RUBAN EN LAVIS** : `_draw_band` rampe désormais l'ALPHA d'un
  trait d'encre net (extérieur, α `BAND_A_OUT` 0.90) à un lavis ténu (intérieur, α `BAND_A_IN` 0.16) → le
  parchemin/terrain TRANSPARAÎT, la frontière se FOND dans le territoire au lieu d'une strie opaque. (3)
  **CAPITALE = LISERÉ FIN** : le contour de capitale passe d'une BANDE pleine (`_draw_band`) à un FILET
  pourpre sourd (`_draw_cap_lisere`, `CAP_INK` + halo doux, 1.1 px) posé juste à l'intérieur du contour —
  un accent discret, plus une bande qui « prend tout ». DISPLAY-ONLY (aucun fichier C) ⇒ **golden
  IDENTIQUE**, déterminisme/save intacts ; pas de rebuild DLL (façade inchangée).

- **GODOT parchemin — ÉPAISSEUR DE TRAIT ADAPTATIVE AU ZOOM (inspirée de CK3) (2026-06-29)** : les traits
  étaient tous `largeur_px / zoom` = px ÉCRAN constant (jamais soudés au terrain). On imite le RENDU de CK3
  (étude des shaders : `gfx/map/borders/settings.txt` seuils par tier · `camera.fxh` `GetZoomedInZoomedOutFactor`
  · `pdxverticalborder.shader` `clamp(1−f·2.5,0,1)`) : CK bake ses bordures en géométrie MONDE (elles
  GROSSISSENT à l'écran quand la caméra descend) puis les fond par un shader d'opacité SÉPARÉ. Un overlay 2D
  immédiat ne peut pas rebaker → helper **`_w(zoom, base, min, max)` = `clamp(base·zoom, min_px, max_px)/zoom`**
  (rend une largeur MONDE) : trois régimes C0-continus en px écran — **plancher** min_px (zoom OUT : le trait
  ne DISPARAÎT jamais au plan large) · **approche** = base (CONSTANTE en monde → SOUDÉE au terrain, s'épaissit
  à l'écran) · **plafond** max_px (zoom IN : une bordure n'AVALE jamais une province). L'OPACITÉ (fondu de la
  trame fine, gates routes/villes) reste la couche de visibilité INDÉPENDANTE (exactement CK). **HIÉRARCHIE par
  asymétrie de rails** : la bande d'EMPIRE respire fort (`ZW_EMPIRE_BASE` 0.85 · `_MIN` 2.0 · `_MAX` 3.4 →
  2.0px au plan / 3.4px au zoom profond, toujours dominante) tandis que la trame de PROVINCES reste un cheveu
  (cœur ≤1.3px, gouvernée par son fondu) → l'empire DOMINE, la province ÉMERGE (mirroir du « domain 0-20 vs
  province 0-10 » de CK). **8 sites** passent à `_w` (trame fine ×3, bande d'empire, routes ×2, anneau de phase
  d'armée, anneau d'épicentre §27) ; les 7 marqueurs (liseré de capitale, ligne/losange d'armée, étoile de
  capitale) restent en px écran (`min==max` ⇒ `_w` se réduit ALGÉBRIQUEMENT à `min/zoom`, byte-identique — on
  les laisse littéralement inchangés). Pièges ÉVITÉS : `_w` rend déjà une largeur MONDE (jamais `_w(...)/zoom`) ;
  les LONGUEURS/MOTIFS (`ROAD_DASH`/`ROAD_GAP`, rayon de pulse, demi-losange `s`, rayons de ville, offsets de
  normale) restent `/zoom`. Vérifié au rendu (seed 9, an 120) : bandes lisibles au fit (2px), épaissies à
  l'approche, gelées au zoom 12 sans dominer ; trame fine en cheveu. DISPLAY-ONLY (overlay.gd SEUL, aucun
  fichier C) ⇒ **golden IDENTIQUE**, déterminisme/save intacts, pas de rebuild DLL. Dialable d'une ligne
  (`ZW_EMPIRE_MAX` ↓ si lourd).

- **GODOT parchemin — FRONTIÈRES EN COURBES (escaliers → tracé lissé) (2026-06-29)** : la façade rend
  les arêtes en SEGMENTS UNITAIRES alignés sur la grille (escalier par CONSTRUCTION) — le SSAA/MSAA n'y
  change rien (c'est de la GÉOMÉTRIE, pas de l'aliasing ; vérifié : un ×4 surfacique donne un escalier
  proprement dessiné, pas une courbe). On lisse donc la GÉOMÉTRIE, display-only dans `overlay.gd`.
  **Pipeline** (3 étages, le 1er Chaikin seul ne faisait qu'« arrondir les marches ») : (1) **CHAÎNAGE**
  de la soupe de segments en polylignes ORDONNÉES (`_chain_segments`/`_chain_segments_n` : index de
  sommets entiers, adjacence, marche jusqu'aux jonctions degré≠2 ou bouclage ; pour le ruban, le côté
  INTÉRIEUR est déduit de la normale d'origine du 1er segment et tenu sur toute la chaîne). (2)
  **`_smooth_poly`** = ré-échantillonnage (`SMOOTH_RESAMPLE` 2.0 cellules — **casse la FRÉQUENCE de
  l'escalier** sans écraser la forme) → passe-bas **TAUBIN λ|μ** (`SMOOTH_TAUBIN` 6 itérations,
  `TAUBIN_LAMBDA` 0.5 / `TAUBIN_MU` −0.53 — aplatit les marches vers la diagonale moyenne, extrémités/
  jonctions FIXES, boucles cycliques) → **Chaikin** (`SMOOTH_CHAIKIN` 2 passes — arrondi final). ⚠ TAUBIN
  et non Laplacien PUR : le Laplacien RÉTRÉCIT les boucles vers leur centre (cumulatif) → les frontières
  dérivaient et BULGEAIENT par-dessus les VILLES (« placement avalé ») ; Taubin alterne un pas adoucissant
  (λ) et un pas regonflant (μ) → lisse SANS rétrécir, la frontière reste sur sa vraie ligne. (3) Pour le
  ruban, la normale intérieure est **recalculée perpendiculaire à la COURBE**
  locale (orientée par le côté de la chaîne). Appliqué à la trame fine (provinces+régions), aux bandes
  d'empire et au liseré de capitale. Le **jitter** « plume » (`_jit_a`/`_jit_poly`, `BORDER_JIT`/`FINE_JIT`)
  est RETIRÉ (il rajoutait du bruit ; la courbe EST le rendu voulu). **MSAA 2D** (`map_view.gd`) est GARDÉ
  derrière `RenderingServer.get_rendering_device() != null` : il n'existe PAS sous GL Compatibility (GLES3,
  warning) — il s'activera tout seul sous Forward+/Mobile ; sous GL Compat la netteté vient du lissage
  géométrique + de l'antialiasing par-trait (`draw_* antialiased`). Vérifié au rendu (seeds 9/11, zooms
  fit/mid/deep + gros plan VILLE) : courbes lisses, frontières FIDÈLES à leur ligne (villes bien dans
  leur cellule, plus avalées), pas de trou aux jonctions ni de province écrasée ; **0 warning**.
  DISPLAY-ONLY (aucun fichier C) ⇒ **golden IDENTIQUE**, déterminisme/save intacts, pas de rebuild DLL.
  Dialable : `SMOOTH_RESAMPLE`/`_TAUBIN`/`_CHAIKIN` ↑ = plus lisse (plus cher).

- **GODOT parchemin — DA « atlas » : frontières gravées (admin vs politique) + routes présentes (2026-06-29)** :
  retour DA — les grosses lignes noires « disaient grille de jeu avant carte ». Trois gestes (overlay.gd,
  display-only). (1) **PROVINCE = administrative, DISCRÈTE** : la trame fine passe du NOIR (3 passes
  feutrées) au **brun sombre gravé** `PROV_INK` #2a2419, **−30 % d'opacité** (LOD max 0.45→0.34 ⇒ cœur
  ≈ alpha 0.35) et **2 passes** seulement (halo doux + cœur ~1px). (2) **PAYS = politique, DOUBLE PASSE
  GRAVÉE** (remplace l'ancien ruban blendé 5-couches) : `_draw_band` trace sur la ligne lissée un **halo
  brun très sombre LARGE** (`POL_HALO` #17110b, α 0.45, `_w` 2.6→4.6px = le « creux » qui détache la
  frontière du terrain) puis un **pigment politique FIN** (`_entity_pigment` = encre d'HÉRITAGE +
  variation par pays sur la valeur, α 0.85, `_w` 1.4→2.4px). Sépare nettement admin (cheveu brun) du
  politique (trait coloré net). Cités-états = or fané. (3) **ROUTES plus PRÉSENTES** : `ROAD_INK` passe
  du sépia clair à une **encre BRUNE franche** (α 0.82) et les **ARTÈRES (niveau 0)** sont séparées des
  dessertes/mineures (`ROAD_INK_MINOR`, pâle & fin) — batches distincts, artères plus épaisses + léger
  halo « tracé à la plume ». Supprimés (orphelins du ruban blendé) : `N_BAND`/`BAND_*`/`LAYER_W_PX`/
  `ZW_EMPIRE_*`/`ETHOS_PIG`/`CS_SILVER`/`_border_pair`. Vérifié au rendu (seed 9, fit/mid/deep/ville) :
  hiérarchie admin<politique tenue, routes lisibles, parchemin respire ; **0 warning**. DISPLAY-ONLY ⇒
  **golden IDENTIQUE**, déterminisme/save intacts, pas de rebuild DLL. À VENIR (priorités 3-4 du brief) :
  EAU « statut carte » (shader : bleu vert-gris, liseré de lac, rivière ∝ débit, côte irrégulière) ·
  PORTS visibles (⚠ besoin d'un reader façade `scps_region_has_port` + rebuild) · capitales en sceau ·
  hachures de révolte (façade `agitation` déjà dispo) · lignes de commerce · sélection/survol lumineux.

- **WORLDGEN #1 — ÉROSION HYDRAULIQUE par gouttelettes (keystone, plan Gleba) (2026-06-30)** : 1er PR du
  plan d'amélioration worldgen. `step_hydraulic_erosion` (`scps_world.c`, ~110 lignes, d'après Sebastian
  Lague) : 200 000 gouttes seedées (xorshift `seed^0xE5051234`) descendent le gradient bilinéaire, ÉROSENT
  en montée de capacité (`cap = max(-dh,minSlope)·vel·eau·CAP`) et DÉPOSENT en remontée/saturation →
  réseau dendritique, vallées, plaines alluviales ÉMERGENTS sur le `height`. Insérée **après l'érosion
  thermique, AVANT `step_erosion`** (l'accumulation de flux D8 recalcule alors `cell.river` sur le relief
  CREUSÉ → les chenaux sont déjà là). Pinceau rayon 2 (étale l'érosion, pas de puits 1-cellule) ; **bornée
  par la pente locale `-dh`** → ne rabote PAS les crêtes (le risque « lisse tout » du plan, évité).
  DÉTERMINISTE : xorshift + ordre de gouttes fixe → `determinism` STABLE (bit-identique). Tunables en
  `#define` locaux (`HYD_*` : DROPLETS/LIFETIME/INERTIA/CAPACITY/DEPOSIT/ERODE/EVAP/GRAVITY/MIN_SLOPE/
  BRUSH_R). ⚠ **RE-BASELINE TOTAL** (le `height` change partout → mondes des graines de référence changés ;
  `golden_hashes.txt` re-baseliné). Bancs sensibles recalibrés (intention préservée) : `econ_arcane_demo`
  (armes enchantées mesurées au PAYS, pas la région-forge — la pop-share P1 a bougé) · `audit_eco`
  (plancher de matériaux à la capitale chaque mois : la borne ACCESSION teste la loi des prix, pas la
  disette — l'ext de matière monte avec la taille d'empire qui a changé). `make test` = **38 verts** (seuls
  les 3 KO Windows pré-existants restent : intertrade `setenv`, campaign/warhost stack-overflow) ;
  `determinism` STABLE ; vérif visuelle (mapshot + Godot nature, DLL rebâtie) : montagnes PRÉSERVÉES,
  rivières en vallées, monde sain. ⚠ ASan/UBSan NON vérifiés ici (libasan/libubsan absents du MinGW —
  limite d'env, comme les 3 KO ; le code est borné — chaque index gardé). **SAVE non bumpé** (la worldgen
  se régénère de la graine ; aucune struct sérialisée ne change).
- **WORLDGEN #2 & #4 — DÉJÀ EN PLACE (audit du plan Gleba, 2026-06-30)** : la revue du plan a trouvé
  que **DEUX des cinq étapes EXISTAIENT déjà** (l'auteur du plan, une session antérieure, ne le savait
  pas). **#2 plaques→orogenèse** : `plates_init` (germes continentaux/océaniques, dérive, rifts) +
  `step_architecture` font déjà l'orogenèse par CONVERGENCE (`conv=(1-dot)·0.5` ⇒ collision continentale
  → chaînes `bump=bs·conv·1.10`, subduction océanique → Andes 0.65, crêtes ALIGNÉES sur la frontière avec
  bruit hiérarchique R1/R2, volcans le long de la subduction, dureté différentielle) — plus riche que le
  croquis du plan. **#4 ombre pluviométrique** : `gen_climate` a déjà la marche de VENT directionnelle
  (`oro=humidity·rise·ORO_K` au versant au vent, `humidity-=p` ⇒ ombre sous le vent, ré-évaporation
  forestière) + ceinture subtropicale de Hadley + assèchement continental ; et **#1 a rendu le relief
  ANISOTROPE** ⇒ cette ombre existante AIGUISE désormais les déserts derrière les chaînes pour rien
  (l'effet diffus que le plan craignait disparaît). **Conclusion** : #2/#4 NON ré-implémentés (redondant +
  risqué) ; le travail neuf se concentre sur #5 (ci-dessous). #3 (priority-flood) et `trace_rivers`
  ÉMERGENT restent un COUPLE à fort risque (chaîne rivière→port→marine→estuaire→rendu + l'esthétique de
  lacs DISCRETS du parchemin que `fill_lakes` préserve volontairement) — à GREENLIGHTER à part, pas en
  aveugle.
- **WORLDGEN #5 — BIOMES PENTE/SOL (alluvial émergent, plan Gleba) (2026-06-30)** : dernier PR à faible
  risque du plan ; exploite le relief sculpté par #1. La boucle de pose des biomes (`scps_world.c`) gagne,
  APRÈS `assign_biome(h,m,t)`, un raffinement par PENTE locale (max |Δh| sur 8 voisins) et LIMON fluvial
  (`cell.river` = aire de drainage, déjà dampée par l'aridité). Deux règles, **uniquement sur le bas-pays**
  (sous la bande de collines `h<MOUNTAIN_H-0.05`) et **jamais sur un biome déjà humide** : (1) **ÉPAULE
  RAIDE** (`slope>0.030`, seuil ÉLEVÉ ⇒ rare) — un escarpement sous les sommets devient collines nues
  (HILLS/HIGHLANDS selon t) : le sol fin ne tient pas la pente ; (2) **PLANCHER ALLUVIAL** (`slope<0.006`
  ET `flux≥48`) — une vallée plate qui REÇOIT un débit monte d'un cran de verdure (désert/drylands→
  prairie/savane · steppe/savane/plaine→prairie · prairie→bois) : c'est la « dé-aridification » du plan,
  mais ÉMERGENTE (le fleuve dépose le limon — #1 — qui fertilise). `assign_biome` garde sa signature
  `(h,m,t)` ; slope/alluvial sont DÉRIVÉS dans la boucle (aucun champ stocké). DÉTERMINISTE (pure fonction
  du relief). ⚠ **RE-BASELINE** (les biomes changent dès l'an-0 → `golden_hashes.txt` re-baseliné). `make
  test` = **38 verts** (les 3 KO Windows pré-existants seuls ; **aucun banc cassé par #5** — ai/social/
  scps_api/endgame/econ tous verts) ; `determinism` STABLE ; sweep 5×250 SAIN (satisfaction 70/78/81 —
  légèrement RELEVÉE par les vallées plus fertiles —, 39 guerres/sim, hégémon mortel 5/5, IPM 1.23, §27
  gaté). **SAVE non bumpé** (aucune struct sérialisée ne change). Reste (à greenlighter) : #3 priority-flood
  + `trace_rivers` émergent — le couple hydrologique à fort risque (cf. entrée #2/#4).
- **WORLDGEN #3 + RIVIÈRES ÉMERGENTES (le couple hydrologique, plan Gleba) (2026-06-30)** : le gros
  morceau, conçu par panel (dossier + 4 lentilles + juge ; le min-risque a survécu, raffiné à la main).
  **(A) RIVIÈRES ÉMERGENTES** — `trace_rivers` ne FORCE plus la hiérarchie N/2N/4N (sources = maxima locaux,
  SEEK d'un champ BFS distance-mer, qsort flottant) ; il SUIT le réseau de drainage que `step_erosion` a
  déjà calculé sur le relief érodé #1 — `cell.flow_dir` (D8 aval) + `cell.river` (flux 0-255). Approche
  **bouche-amont** : EMBOUCHURES (cellule rivière qui se jette dans l'eau/endoréique) triées par flux
  DÉCROISSANT (tri par dénombrement 256 godets, départage par index → ordre total, **plus de clé
  flottante** : le risque qsort du dossier disparaît) ; pour chaque bassin, `trace_stem` remonte le MAIN
  STEM (contributeur amont de plus fort flux) → TRONC, et ses plus gros contributeurs (≥`RIVER_TRIB_T`)
  deviennent une file d'AFFLUENTS plafonnée `RIVER_MAX_TRIB`/bassin → **réseau dendritique RÉPARTI entre
  bassins** (le budget 64 ne s'épuise pas sur un seul). `flow_max` = flux de POINTE (continu, plus l'échelle
  figée 1.0/0.62/0.34). Les 5 helpers forcés (`river_src_cmp`/`river_dist_to`/`_to_sea`/`river_seek`/
  `RiverSrc`) SUPPRIMÉS. Le rendu Godot (parchemin) lit ces mêmes polylignes (`river_paths`→`_build_river_field`
  →shader) → le réseau émergent passe par le carve/méandre EXISTANT, 0 changement front. **(B) #3 LACS
  PRIORITY-FLOOD** (Barnes 2014) — `fill_lakes` remplacé : inonde TOUTE dépression fermée jusqu'au
  débordement depuis les exutoires (mer+bords), via un tas min à **clé entière** (`niveau<<32|index`, ordre
  total, **aucun flottant** → déterministe), puis étiquetage en composantes connexes + **PORTE** : un bassin
  ALIMENTÉ (flux≥`LAKE_INFLOW_MIN`) reçoit `cell.lake` (accès EAU jeu) ; s'il est aussi GRAND
  (≥`LAKE_VISIBLE_MIN`) il SURFACE en mer intérieure VISIBLE (`BIO_SHALLOW`+niveau aplani). Cuvette SANS
  apport = playa SÈCHE. Respecte les « lacs discrets » : quelques grandes mers intérieures (seed 9/7/11/42 :
  22/18/17/8 visibles), pas une nuée de mares. **(C) barrière de province** : `build_cross_cost` traite
  `cell.lake` comme infranchissable (comme la mer) → un grand lac BORNE les provinces, jamais colonisé.
  ⚠ **Décalage de bouche assumé** : `cell.river`/`flow_dir` ne sont PAS recalculés après le flood (la
  poignée de cellules de bassin visible montent à SEA_LEVEL+0.004 ; `trace_stem` casse sur `cell.lake` ⇒
  n'y entre jamais) — le ré-accumulation de flux (lac à exutoire hydro-correct) est le palier risqué SUIVANT,
  laissé hors scope (chaîne port/marine/estuaire). Tunables LOCAUX (`#define`, comme HYD_*) : `RIVER_FLUX_T`
  60 · `RIVER_TRIB_T` 110 · `RIVER_MAX_TRIB` 6 · `RIVER_MIN_PTS` 12 · `LAKE_DEPTH_MIN` 0.004 · `LAKE_VISIBLE_MIN`
  60 · `LAKE_INFLOW_MIN` 48. Télémétrie console (`[scps] lacs…`/`rivières…` : visibles/alimentés ·
  gravées/écartées). ⚠ **RE-BASELINE** (le monde change dès l'an-0 → `golden_hashes.txt` re-baseliné) ·
  `determinism` **STABLE** (tas & tri à clés ENTIÈRES, marche pure flow_dir, 0 rand). 1 banc recalibré
  (intention préservée) : `econ_production_demo` #4 (usure d'outils) — `region[2]` devenu NON-possédé par la
  re-baseline ⇒ hors pool ⇒ jamais d'usure ; rendu empire ISOLÉ mono-région (slot inutilisé) → usure NETTE.
  `make test` **38 verts** (3 KO Windows pré-existants seuls) · sweep 5×250 SAIN (satisfaction 61-78/79-84/
  85-91, hégémon mortel 5/5, IPM 1.22, **§27 gaté an-180**, 313 routes maritimes, exit 0) · 0 warning ·
  visuel mapshot (réseau dendritique troncs+affluents aux vallées, mers intérieures, embouchures aux côtes).
  **SAVE non bumpé** (River struct + champs Cell inchangés ; le monde se régénère de la graine). ⊕ Le plan
  Gleba est COMPLET : #1 érosion · #2/#4 pré-existants · #3 lacs · #5 biomes · rivières émergentes.
- **VIEWER DÉDOUBLONNÉ + AUDIT v48 + CARTE JOUABLE (2026-07-02, 4 commits)** : le but est d'intégrer
  viewer.c DANS Godot — en attendant il cesse de FORKER le moteur. **(a) Dedup** (−856 lignes) : le
  typedef `Sim`, le `sim_day` local (divergent : sans religion/hameaux/drain joueur/fix fuite warhost)
  et le bloc save v35 FOSSILE sont SUPPRIMÉS ; le viewer tient le MÊME `scps_sim` + `scps_save` que
  chronique/Godot (gates joueur miroir de la façade : `human_player` · `ai_on[joueur]=false` ·
  `warhost/econ_set_human`) ⇒ **slots de save INTERCHANGEABLES viewer ↔ Godot**. Il builde ICI :
  `make WIN=1 scps` (le flag manuel — WINLIBS/AUDIO_LIBS Windows). Harnais `--savetest`/`--fuzztest`
  = la vérif. **Bug moteur PRIS par le savetest (touchait Godot)** : `scps_load_game` écrasait
  `region_rep_prov` (état SÉRIALISÉ, figé genèse) d'un recalcul à l'état courant (« plus peuplée »
  bouge avec la pop) → sauve-recharge ≠ continuation ; fix `econ_rebuild_prov_adj` (ne rebâtit que le
  pointeur tas) + rep borné par `save_sane`. **(b) Audit** : ⚠ **SAVE BUMP 47→48** — section WILD
  (compteurs de ralliement des hameaux : un load en processus frais les remettait à 0 → ralliement
  retardé ≤8 ans) + `SaveMisc.player_age_engaged`. **CMD_AGE_ENGAGE** (§7) : l'IA s'engage auto au
  lever d'un âge, le joueur CHOISIT — verbe journalisé + façade `scps_age_state`/`scps_player_age_
  engage` + chip topbar AMBRE « Engager : <âge> » + probe `age_audit` (round-trip OK an 20). **E0.4
  ENTERRÉ** (`labor_publish_capitals`/`labor_region_cap_tier` + branche lectrice revolt) : depuis
  P-arc la capitale monte GRATUITEMENT au déblocage ⇒ le registre « tier payé » ne divergeait plus du
  repli pop-derived — byte-identique (golden INTACT). **(c) La carte Godot devient JOUABLE** (mesuré
  par captures avant/après fit·mid·deep) : **LAVIS POLITIQUE** (façade `scps_map_owner` → binding
  `political_image(pal)`, boucle 512k cellules en C++ — l'aquarelle de territoire sous l'encre, 0.36
  au fit → 0.15 au zoom : la carte DIT qui tient quoi) · **TRAME DISCIPLINÉE** (joints à deux rives
  vierges non tracés — `border_segments_col` 0/1 gagne `other` ; LOD 2.2+, plafond 0.24 : fin de la
  « boue craquelée » sur la terre sauvage) · **SÉLECTION dorée** (`scps_province_border_segments` →
  contour lissé or au grain de panneau — la régression parchemin résorbée) · **UNE famille de couleur
  par entité** (`_entity_hue` source unique → encre frontière sat 0.45/val 0.55 · lavis 0.60/0.82 ·
  jeton d'armée +0.22 · NOM teinté & agrandi 1.35→1.9 — jadis 3 roues indépendantes). golden IDENTIQUE
  partout · determinism STABLE · savetest A==B · fuzztest 7/7 · api_demo 90 · smoke 8/8 · 0 warning.
  Doublons d'affichage RESTANTS (assumés, deux fronts) : bseg viewer vs façade borders, teintes SDL vs
  pigments Godot — se résorbent quand Godot absorbe le reste du viewer.
- **BRASSAGE DÉMOGRAPHIQUE — la métabolisation par TOUTES les voies (2026-07-03, 4 commits, SAVE v50→51)** :
  la métabolisation (digérer une diaspora allophone ⇒ +recherche + accès tech) était **near-inerte**
  (~0.4 % au plus digéré) — le seul canal vivant était la sécession post-cataclysme. On distingue enfin,
  comme l'Histoire, la **DIASPORA** (venue libre) de la pop **acculturée/soumise/déportée**, chacune
  diffusant selon son RAPPORT AU POUVOIR. **(1/4) LE MODE D'ARRIVÉE** : `PopGroup.arrival` (natif/migrant/
  soumis/déporté ; ⚠ **SAVE BUMP 50→51**, blob WorldEconomy fwrite BRUT) pilote le coeff de diffusion du
  savoir — `ARR_NATIF` 0 (l'hétérogénéité de NAISSANCE post-GR4 ne compte pas : la dynastie Song avait des
  puits d'1 km sans diffusion), `ARR_MIGRANT`/`ARR_SOUMIS` plein (1.0), `ARR_DEPORTE` **faible** (0.30,
  `METAB_DIFFUSE_SLAVE`). `econ_country_metabolized`/`_heritage_digested` pondèrent par ÂMES × intégration ×
  coeff. La **CONQUÊTE** (`demography_on_conquest`) re-flagge les natifs allophones en `ARR_SOUMIS` (seed 42 :
  0.4 % → 5-19 %). **(2/4) LE PACTE MIGRATOIRE** (voie PACIFIQUE) : `DiploState.migration_pact` (réciproque,
  canal DISTINCT du pacte commercial) + `demography_migration_pact_tick` (échange passif annuel entre alliés
  non en guerre, flux ∝ attractivité relative — prospérité — de la destination, `migration_move(..,ARR_MIGRANT)`).
  L'IA le PROPOSE (`ai_strat_turn` 2c, aux alliés d'un autre héritage) ; `OFFER_MIGRATION` gate le consentement
  (opinion #26). **(3/4) L'ESCLAVAGE dé-restreint** : le gate `can_enslave` ne suit plus la SEULE tech
  (Économie servile, Clanique, quasi jamais) — un éthos CONQUÉRANT (Dominateur/Honneur) déporte par COUTUME
  (Rome, empires esclavagistes). `SLAVE_FRACTION` tunable calé **BAS** (0.25→0.08, « taux très faible » :
  janissaires/forge/créole, réel mais mineur) ; le double faible (volume × diffusion 0.30) rend l'apport
  marginal, présent sans dominer. **(4/4) LE VERBE JOUEUR + membrane** : `CMD_OFFER_MIGRATION` (moteur → façade
  `scps_player_offer_migration` + `diplo_options.{can,would_accept}_migration` → binding → bouton « Migration »
  gaté/ambré dans country_actions.gd) ; la membrane NOMME la voie — `province_composition` : « soumis · N %
  intégré » / « déporté · N % intégré » (le % SURFACE la métabolisation en cours, nombre tangible). ⚠
  **RE-BASELINE golden DÉLIBÉRÉE** (HASH 7/209/411 — mondes belliqueux asservissent < an-12 ; 108/310 INCHANGÉS ;
  le verbe joueur/readout sont golden-NEUTRES — drain no-op cmd_n=0, readout hors tick). Mesuré (6 graines × 3
  sims × 250 ans) : métabolisation **max 14.7-24.7 %** au plus digéré, 4-15 empires creuset/sim, brassage 0-18
  flux/sim (canal mineur, comme conçu), subjugation = casus belli prominent (la conquête digère) ; monde SAIN
  (satisfaction 67-91 %, hégémon mortel 2-3/3, §27 gaté an-180). `make test` **38/38 runnable** (3 KO Windows
  pré-existants), determinism STABLE (v51 byte-identique), GDExtension scons 0 warning. Tunables registre J :
  `METAB_DIFFUSE_SLAVE` 0.30 · `SLAVE_FRACTION` 0.08 · `MIG_PACT_FRAC` 0.006 · `MIG_PACT_MIN` 30 ·
  `AI_OFFER_MIG_OPINION` 40. Télémétrie chronicle « brassage » + « métabolisation ».
- **RÉFUGIÉS — la guerre fait FUIR, l'apaisement fait RESPIRER (2026-07-03, SAVE v51→52)** : statut
  de réfugié + le principe que TOUT déplacement RESPIRE (aucune migration définitive). `PopGroup`
  gagne `home_reg` (RÉGION d'origine ; ⚠ **SAVE BUMP 51→52**). **`demography_refugee_tick`** (annuel,
  moteur pur, déterministe) : **(FUITE)** une région RAVAGÉE (`revolt_scar > REFUGEE_FLEE_SCAR` : sac
  de guerre OU révolte) déverse `REFUGEE_FLEE_FRAC` de chaque groupe vers la voisine la MOINS ravagée
  (« si possible » : nulle part de sûr ⇒ piégé, reste) → diaspora `ARR_REFUGIE`, foyer inscrit.
  **(RESPIRATION)** foyer apaisé (`< REFUGEE_HOME_CALM`) + habitable ⇒ une part RENTRE ∝ `RETURN_PULL
  ·(1−intégration)` (le FIXÉ reste — Huguenot devenu prussien ; le rentré fond dans ses gens, redevient
  natif) ; réfugié intégré au-delà de `SETTLE_INTEG` se FIXE (`→ARR_MIGRANT`). **(NO DEFINITIVE)** le
  migrant économique a AUSSI un `home_reg` (retour ténu, `MIGRANT_RETURN_PULL`) ; un re-chassé garde son
  VRAI foyer. Diffusion : le réfugié apporte ses métiers (Huguenots : soie, horlogerie) ⇒ coeff PLEIN
  (1.0), la partialité de métabolisation vient du RETOUR + de l'intégration. **CALIBRAGE** (paired OFF/ON,
  5 graines × 3-4 sims × 200 ans) : `FLEE_FRAC` calé BAS **0.03** — au-delà de ~0.04 le flot de réfugiés
  RESTIFS bascule les hôtes dans le bassin BAS de satisfaction (monde bistable, cf. `POP_R_BASE`) ; à 0.03
  satisfaction 64-79 % (≈ baseline OFF 80), hégémon mortel 3/3, §27 gaté an-180, la pop RESPIRE (retours
  ≳ fuites), métab max 8-48 %. ⚠ **PIÈGE (init)** : `home_reg` est memset à **0** — région VALIDE, pas un
  sentinel ; un natif NE doit PAS être vu « ayant un foyer 0 » → `migration_move` teste le src RÉEL
  (déplacé = diaspora + arrivée migrant/réfugié), `home_reg=-1` posé explicitement à la genèse native /
  colon de conquête / esclave (tenu) / sécessionniste (souverain) ; `save_sane` borne ∈ [-1, n_regions).
  Membrane : « réfugié · N% intégré ». Télémétrie « réfugiés : N fuite(s) · M retour(s) ». ⚠ RE-BASELINE
  golden (HASH 7/108/310/411 ; 209 inchangé). `make test` 38/38 runnable · determinism **+ determinism-deep
  200 ans × 2 graines** STABLE (churn dynamique de groupes byte-identique au long horizon) · demography_demo
  +3. Tunables registre J (7 : `REFUGEE_FLEE_SCAR` 0.50 · `_FRAC` 0.03 · `_MIN` 30 · `_HOME_CALM` 0.25 ·
  `_RETURN_PULL` 0.12 · `MIGRANT_RETURN_PULL` 0.015 · `REFUGEE_SETTLE_INTEG` 0.90).
- **RÉVOLTE REDESIGN + FOI PAR GROUPE (2026-07-04) — 3 étages** : les triggers de révolte
  redeviennent empire/culture/faction/religion-wide (« pas région toutes les 5 min ») + la FOI
  descend au niveau du GROUPE (l'insight « simuler les pops individuellement »).
  **Phase 1 — le SPIRAL de révolte TUÉ** : (1) CD **empire-wide** (`g_revolt_grace`, 5 ans)
  généralise le patron coup-only à TOUTE révolte — une à la fois par empire (« on n'encaisse pas un
  printemps arabe par jour »). (2) SEUIL de pop (`REVOLT_MIN_POP` 3000) : un hameau n'insurge pas.
  (3) **Le vrai bug** : le cooldown surchargeait `desperation_days` (sentinel négatif effacé au
  moindre calme → boucle écrasement→rallumage → millions de morts vers l'an 105) — champ SÉPARÉ
  `revolt_cooldown[SCPS_MAX_REG]` (⚠ SAVE **v52→53**). (4) l'IA développe ses PÉRIPHÉRIES
  (`ai_neediest_civic_region`) : institutions à la région la plus démunie, pas qu'à la capitale.
  ⊕ seed 9 morts de révolte **2 084 183 → ~150/sim** (÷10 000+), satisfaction 80-83 %, hégémon
  mortel 5/5. **Phase 2 — la RELIGION, dimension de révolte** : `REBEL_HERESIE` (schisme de la foi
  d'État, MÊME racine) vs `REBEL_ZELOTE` (foi ÉTRANGÈRE) — calibrés par le MISMATCH culture↔foi
  (Rome catholique → Germanie protestante ; → Grèce orthodoxe). `RSE_DERIVE` implémenté (la Réforme :
  une marche culturellement distante schisme ; `religion_schism_eligible(w,econ,wl,cid)` + porte DRY
  `region_faith_drifts`, dosée `AI_DERIVE_ODDS`). Victoire → sécession coreligionnaire (diaspora sans
  terre = TOLÉRANCE ; natif étranger = Hollande) ; écrasement → Contre-Réforme (reconversion). Plafond
  schismes 2→5 (diversité riche). Compteurs `n_heresy`/`n_zelote` (dans v53). **FOI PAR GROUPE (le
  refactor)** : `PopGroup.faith` (id de religion) — la foi vit sur le GROUPE comme la culture/classe ;
  `religion_of_region` devient le culte DOMINANT (cache dérivé des groupes, `religion_refresh_all`
  post-démo). La migration la PORTE (struct copy `ng=*src` : un réfugié reste protestant en terre
  catholique). Fondation/mutations (`inherit`/`set_region`/`fracture`/missionnaire/Contre-Réforme)
  retargetées sur les GROUPES natifs (`region_set_native_faith` ; les diasporas GARDENT leur foi
  portée). 4 sites genèse `faith=-1` (piège memset 0 = religion 0) ; guard de borne-registre dans
  `group_carried_faith`. ⚠ SAVE **v53→54** (WorldEconomy blob grandit) ; `religion_demo` couplé econ
  (le cache dérivé). ⊕ La religion devient une dimension VIVE : le **ZÈLE** fire des réfugiés portant
  leur foi (seed 3 : 26 soulèvements/5 sims), filtré par l'INTÉGRATION (la minorité ÉTABLIE reste
  paisible — « les Juifs subissent les pogroms, ne se rebellent pas » ; seul le nouvel arrivant AIGRI
  se lève). L'**HÉRÉSIE** (même racine) reste RARE (minorités same-root peu nombreuses + sous le seuil
  de misère ; `AI_DERIVE_ODDS` ne bouge que le timing, pas le total) — fidèle à l'Histoire (la guerre
  de Trente Ans fut l'EXCEPTION). ⚠ **RE-BASELINE golden** (5/5 graines — la foi mord dès l'an-0) ·
  `determinism` STABLE (save/reload byte-identique v54) · bancs **37 runnable verts** (3 KO Windows
  pré-existants : intertrade `setenv`, campaign/warhost stack) · stabilité SAINE (seeds 9/3/11 :
  134-352 morts/sim, satisfaction 79-89 %, hégémon mortel 5/5). Tunables : `AI_DERIVE_ODDS` 8 ·
  `FAITH_LEAD` 0.20 · `FAITH_UNREST` 0.22. À VENIR (Phase 3) : rebelles = ARMÉES avec war-score
  (+ journal « Rebelles de X », soutien diplo aux rebelles, dé-doublonner les 2 systèmes de révolte).
- **PHASE 3a — LA RÉVOLTE EST UNE VRAIE GUERRE (rebelles = armée à score de guerre) (2026-07-04,
  save v54→55)** : la révolte cesse d'être un compare INSTANTANÉ garnison/rebelles (un « popup »). À
  l'allumage, le soulèvement FAIT NAÎTRE un **pays rebelle** (`spawn_rebel_polity` : slot
  POLITY_ANTAGONIST « Rebelles de X », NE TIENT AUCUNE RÉGION — l'enjeu reste à la couronne) + une
  **armée de campagne** (`deploy_rebel_army` : milice ∝ mobilisés, +cavalerie lourde si l'ÉLITE se
  lève = coup) qui **DÉCLARE la guerre** à la couronne et **ASSIÈGE** sa région (le siège pousse
  l'occupation/score et provoque la sortie défensive de la couronne → bataille). La résolution suit le
  **SCORE DE GUERRE** du système campagne/bataille EXISTANT. **VAINCU UNE SEULE FOIS** (règle joueur) :
  dès que l'armée rebelle est BRISÉE (`broken_days>0`, déroute) OU DÉTRUITE (`!active`/force vidée)
  APRÈS avoir combattu, OU que le score vire négatif, la révolte est **ÉCRASÉE** (pas de guerre
  d'attrition) ; une victoire DÉCISIVE (armée intacte + score nettement positif, `REBEL_WARSCORE_WIN`
  8) donne l'issue par nature (sécession/coup/concession/foi). Garde-fou `REBEL_WAR_MAX_DAYS` (5 ans)
  → fizzle=écrasée. `end_civil_war` solde : le rebelle vainqueur signe une paix blanche (devient État),
  le rebelle sans terre MEURT (`diplo_settle`→polity_death, slot libéré → pas de fuite de slots).
  **REPLI INSTANTANÉ** conservé quand `rebel_country<0` (slot épuisé) ou dp/camp NULL (bancs) — les
  issues (`apply_rebel_crush`/`_victory`) sont PARTAGÉES entre la guerre et le repli (DRY).
  `revolt_scan`/`revolt_tick` prennent `DiploState*` + `struct Campaign*` (threadés depuis sim_day) ;
  `revolt_demo` passe NULL (repli, fixtures inchangées). ⚠ SAVE **v54→55** (Rebellion +`rebel_country`/
  `war_days` ; le pays rebelle vit dans world.country[]/WRLD, l'armée dans Campaign/CAMP — tous
  sérialisés). ⊕ **golden IDENTIQUE** (la guerre civile n'éclôt qu'après les decide-days + le CD
  empire-wide, > 12 ans) · `determinism` STABLE (le pays rebelle transitoire survit save/reload
  byte-identique) · bancs verts · stabilité SAINE (satisfaction 79-89 %, hégémon mortel 4-5/5). ⚠
  Conséquence de BILAN : les révoltes deviennent MOINS nombreuses (une région en guerre civile ne
  rallume pas), plus LONGUES et plus SANGLANTES (les batailles) ; les **sécessions se RARÉFIENT** (la
  milice rebelle perd souvent contre l'armée de la couronne — fidèle à l'Histoire : la plupart des
  révoltes échouent). Dialable (force rebelle, seuils de score). À VENIR : « Rebelles de X » au journal
  (provlog) + télémétrie de guerre civile + dé-doublonner les 2 systèmes de révolte (statecraft/module).

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
