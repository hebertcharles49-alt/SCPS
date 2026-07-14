# MONNAIE — M0 : L'AUDIT (création/destruction monétaire actuelle)

> Lecture seule. Aucune mutation de la sim. Contrat : `docs/MONNAIE_CONCEPT.md` §M0.
> Périmètre : toute écriture sur `strata[].wealth`, `ProvinceEconomy.treasury`
> (province — RÉEL) / `RegionEconomy.treasury` (région — VUE agrégée, jamais
> écrite directement hors fixtures), et l'or nation (`econ_country_gold` = Σ
> trésor des régions possédées). Sites classés en CRÉATION · DESTRUCTION ·
> TRANSFERT · DETTE · INITIALISATION. Les mutations de STOCK (matières) et de
> `mil_stock` (force d'armée) sont HORS PÉRIMÈTRE (ce ne sont pas des unités de
> monnaie) sauf quand elles éclairent un site monétaire ambigu (noté au cas par
> cas).

## Comment lire ce registre

Chaque ligne : `fichier:ligne · qui débite → qui crédite (ou personne) ·
ordre de grandeur · note de conversion M3`. L'ordre de grandeur est le
paramètre-source (constante ou tunable, valeur par défaut) — le CHIFFRAGE
RÉEL mesuré par sim (M(0), M(fin), dérive/an) est au §3, séparé, car il dépend
de la trajectoire (250 ans, seed, nombre d'empires).

---

## 1. CRÉATION (la monnaie apparaît sans qu'aucun compte ne soit débité)

### 1.1 — La valeur ajoutée de production (LE cœur — « la planche à billets »)
- **`scps/scps_econ.c:3082-3091`** · personne ne débite → `strata[LABORER/BOURGEOIS/ELITE].wealth` crédités.
  Chaque manufacture calcule `va = out×prix_sortie − intrants×prix_intrants` (les
  intrants sont pris au STOCK propre de la province, jamais payés en monnaie —
  l'extraction brute qui les a remplis est elle-même gratuite, §1.6) ; `va` est
  scindée 42 % salaires (`WAGE_SHARE`) / 20 % profit bourgeois / 38 % rente
  élite (`TAX_RATE`, ligne 663-664) et **créditée intégralement, chaque mois,
  à chaque province produisant**. Aucun débit correspondant nulle part.
  Ordre de grandeur : ∝ PIB mondial (le plus gros flux du jeu, largement > tous
  les autres réunis).
  **Conversion M3** : c'est LE site à convertir en ventes (Cœur A du plan) — le
  compte de marché provincial doit devenir le débiteur réel de cette VA (l'acheteur
  paie, pas une création ex nihilo).

### 1.2 — La colonisation crée de la richesse à chaque fondation
- **`scps/scps_econ.c:1032-1038`** (`econ_seed_population`) appelée à **chaque
  fondation de colonie**, pas seulement à la genèse : `scps/scps_econ.c:4017`
  (`econ_colony_day`, essaimage terrestre différé) et `scps/scps_econ.c:4099`
  (`colonize_from_prov`, essaimage immédiat/outre-mer). La strate colonisée
  reçoit `wealth = pop × (6 si élite, 2 si bourgeois, 0.5 sinon)` **sans aucun
  débit** : la province SOURCE ne perd que de la POPULATION (`src->strata[c].pop
  -= take`), jamais de richesse (`scps_econ.c:3987-3991`, `4086-4090`). Une
  colonie fondée à 250 pop crée donc jusqu'à ~500-1500 or instantanément (selon
  la répartition de classe), *en plus* du kit de départ (stock, hors périmètre
  monnaie). Sur 250 ans avec expansion active, c'est une CRÉATION répétée non
  négligeable — absente de la liste de sites « déjà repérés » du contrat.
  **Conversion M3** : soit accepter que la colonisation reste une injection
  fixe hors-frappe (documentée, plafonnée), soit la faire payer par la
  métropole (transfert au lieu de création).

### 1.3 — L'arbitrage des cités-états (le marché mondial invente sa marge)
- **`scps/scps_intertrade.c:1046-1066`** (bloc « M4 — l'arbitrage des
  cités-états », dans `intertrade_tick`). Un Centre importe `vol` d'un bien
  d'un Centre étranger moins cher : `econ_region_stock_add` déplace bien le
  stock (réel), MAIS **aucun trésor n'est débité pour l'achat**. Deux crédits
  apparaissent : la source reçoit `vol×prix_source×IT_MARGIN_TO_GOLD` (0.50,
  ligne 27) et le Centre importateur reçoit lui-même `profit =
  vol×(prix_local−prix_source)×ARB_CAPTURE` (0.35 par défaut). Les DEUX sont
  des crédits sans débit — la seule contrainte est un plafond de volume
  (`ARB_VOL_CAP=3`/bien/tick) et un spread minimal (`ARB_MIN_SPREAD=0.20`).
  Borné par tick mais répété sur tous les Centres × tous les biens × toute la
  partie : à vérifier au chiffrage si c'est un bruit ou un vrai contributeur.
  **Conversion M3** : soit un VRAI acheteur (le Centre importateur paie la
  source, empoche seulement sa marge de revente ultérieure — pas un double
  crédit immédiat), soit retirer le crédit `src_tr` (le stock qui bouge suffit
  à représenter le commerce, sans dupliquer en or).

### 1.4 — Le tribut « mûri » des vassaux en or (distinct du tribut de base)
- **`scps/scps_diplo.c:393-396`**, branche `VFN_MARTIAL`/`VFN_GOLD` (fonction
  `vassal_function`, ligne 227-238, purement LECTRICE du vassal). Une fois
  l'intégration du vassal ≥ `AI_VASSAL_CONTRIB_GATE` (0.65), sa « contribution
  typée » verse `base×gold` à la capitale du suzerain (`econ->prov[spid].treasury
  += ...`) **sans aucun débit sur le vassal** — contrairement au tribut de base
  (servage/protectorat, `scps_diplo.c:332-343`, lui bien conservé : débit
  vassal → crédit suzerain). La même fonction crée aussi du grain
  (`econ_region_stock_add`, branche `VFN_AGRAIRE`) et du `mil_stock` (branche
  `VFN_MARTIAL`) ex nihilo — hors périmètre monnaie mais même défaut structurel.
  **Conversion M3** : router sur `econ_region_treasury_add(vassal_region,
  -base*gold)` symétrique au crédit suzerain (comme le tribut de base juste
  au-dessus dans le même fichier — le motif existe déjà à 50 lignes de là).

### 1.5 — Récompenses de mission (le conseil)
- **`scps/scps_missions.c:162`** (`mission_grant`) : `econ->prov[crp].treasury
  += m->reward_gold * mult` (base 280-360 or selon la mission, ×multiplicateur
  de rang/efficacité du siège). Aucun coût d'entrée dans le module (les
  missions ne coûtent jamais rien à démarrer, `grep reward_gold` ne trouve pas
  de pendant `cost`). CRÉATION pure, plus modeste que 1.1-1.2 mais régulière
  (1/pays/plusieurs mois).
  **Conversion M3** : transformer en transfert narratif (le trésor RÉCUPÈRE un
  gain qu'un événement/mission découvre — p. ex. depuis une réserve/butin
  préexistant) ou assumer que c'est une source mineure et volontairement gardée
  hors-frappe (à trancher par le joueur).

### 1.6 — Extraction brute : la matière première est gratuite (note, pas un
  site monétaire en soi, mais LA RACINE du problème 1.1)
- L'extraction (`raw_cap`, la boucle avant `scps_econ.c:3000`) remplit `S[r]`
  (stock) sans jamais débiter ni créditer de monnaie — c'est cohérent (la terre
  n'est pas un vendeur), mais c'est ce qui rend 1.1 « gratuit à la source » :
  la VA n'a aucun coût de matière première réel, seulement un coût
  d'inventaire fictif. Note pour M3 : convertir 1.1 en ventes ne suffira pas
  seul si l'INTRANT reste gratuit — la chaîne complète (extraction → stock →
  manufacture → VA → vente) doit être valorisée de bout en bout ou le nu
  (marge) restera artificiellement gonflé.

### 1.7 — Événements/dilemmes à `d_treasury`/`d_treasury_mois` positif
- **`scps/scps_events.c:2104`** (`apply_region_eff`, `d_treasury` fixe) et
  **`scps/scps_events.c:2122-2126`** (`resolve_treasury_mois`, fraction du
  revenu mensuel). Table `EVENTS[]` : 77 sites au total portent `d_treasury`
  ou `d_treasury_mois` (mélange de signes, décisions joueur/IA — la « membrane
  de décision », dilemmes). Un seul flat positif recensé (`EVID_XENOPHILE`,
  +120) contre 5 flat négatifs (-10 à -50) ; côté `_mois`, les valeurs positives
  (0.15 à 2.0× le revenu mensuel) sont MOINS nombreuses que les négatives mais
  比 rien n'empêche un dilemme de créditer plus qu'il ne débite. Voir §2.6 pour
  le pendant négatif (même mécanisme, signe opposé).
  **Conversion M3** : les dilemmes à effet positif doivent puiser dans une
  réserve identifiable (butin déjà looté, caisse noire d'un conseiller) plutôt
  que créditer dans le vide — à trier un par un (hors scope M0, juste recensé).

---

## 2. DESTRUCTION (la monnaie disparaît sans que personne ne soit crédité)

### 2.1 — La consommation des ménages (LE trou noir principal)
- **`scps/scps_econ.c:3281` (`budget=re->strata[c].wealth`) puis les débits
  répétés `budget -= ...`** aux lignes **3300, 3316+3322, 3339+3345, 3364,
  3389** (confort poterie/statuaire, boisson bière/eau-de-vie, luxe de statut,
  achat générique, désir croisé d'éthos) et **3395** (`re->strata[c].wealth =
  fmaxf(0.f,budget)`, l'écriture retour). Chaque achat retire `S[r]` du STOCK
  de la province (le bien est bien consommé, physique) mais **le prix payé
  (`budget -= need*got*price`) ne va à AUCUN vendeur** — la province vend ses
  propres stocks à ses propres classes et empoche… rien. C'est très exactement
  « la consommation détruit la richesse sans créditer un vendeur » du contrat.
  Ordre de grandeur : le PLUS GROS puits du jeu (toute la demande de subsistance
  + confort + luxe de toute la population, chaque mois).
  **Conversion M3** : Cœur A du plan — un compte de marché provincial (ou une
  propriété par classe) doit encaisser cet argent en échange du stock vendu.

### 2.2 — Entretien de l'infrastructure bâtie
- **`scps/scps_econ.c:3148-3150`** (`paid_up`, `FX_UPKEEP`). Paie l'entretien
  de `K_inst+H_coerc×1.5+P_open+PE_infra+food_cap+faith+savoir` depuis le
  SURPLUS (au-dessus de `SINK_FLOOR=500`) ; borné, jamais de dette forcée.
  Personne n'est crédité (pas d'artisan, pas d'ouvrier du bâtiment).
  **Conversion M3** : transformer en salaire versé à une strate (les artisans
  d'entretien existent déjà conceptuellement dans le panier bourgeois/laboureur).

### 2.3 — Surcharge IPM + encadrement des manufactures
- **`scps/scps_econ.c:3159-3162`** (`FX_ENCADR`). Part IPM de l'entretien +
  `MANUF_UPKEEP_DAY×niveau_manufactures` — ponctionné uniquement au-dessus du
  seuil de hoarding (`COURT_FLOOR=4000`). Même trou : aucun crédit.

### 2.4 — Le faste de cour
- **`scps/scps_econ.c:3167-3169`** (`FX_COURT`) : `COURT_RATE=0.010`/mois×12
  du surplus au-dessus de `COURT_FLOOR=4000`. Frein au hoarding, explicitement
  documenté comme un SINK (« un trésor qui gonfle finance le prestige ») —
  mais le prestige ne va nulle part.

### 2.5 — L'administration
- **`scps/scps_econ.c:3174-3179`** (`FX_ADMIN`) : `ADMIN_BASE=0.4 ×
  n_régions^(1.3−1) × IPM`/an, prélevé au-dessus du seuil de hoarding.

### 2.6 — La redépense publique (le « trou de l'instrument »)
- **`scps/scps_econ.c:3191-3204`** (`FX_REDEP`). `depense = trésor ×
  STATE_SPEND_RATE(0.30) × dt`, bornée au surplus. **60 % (`PAYROLL_FRACTION`)
  revient aux strates au prorata de leur impôt versé** (TRANSFERT réel, voir
  §3) — mais **les 40 % restants** (« armée, travaux ») **ne créditent
  personne** : le commentaire du code l'assume explicitement (« il ne s'agit
  plus de hoarder », mais rien ne les reçoit). Ordre de grandeur : `0.4 ×
  0.30 × (trésor−500)` par tick, un des plus gros sinks après 2.1.
  **Conversion M3** : payer une armée/des travaux RÉELS (crédite `mil_stock`-
  adjacent ou une strate d'ouvriers de chantier) au lieu d'un pourcentage fixe.

### 2.7 — Curseurs joueur INVESTISSEMENT et ROUTES
- **`scps/scps_econ.c:3697-3701`** (`FX_INVEST`) et **`scps/scps_econ.c:3729-
  3733`** (`FX_ROADS`). `econ_region_treasury_add(e, r, -part)` finance le
  capital institutionnel K / la connectivité routière — des MULTIPLICATEURS
  abstraits, aucun vendeur. Défaut 0 (curseur non réglé) = coût nul,
  golden-neutre ; actif seulement si le joueur monte le curseur.

### 2.8 — Construction civile — manufactures (ASYMÉTRIE joueur/IA vs édifices)
- **Joueur** : `scps/scps_sim.c:621-626` (`CMD_BUILD_MANUF`) et
  `scps/scps_sim.c:637-641` (`CMD_MANUF_LEVEL`, monter un cran).
- **IA** : `scps/scps_ai.c:1052` (`ai_build_civmanuf`), `:1088`
  (`ai_pay_and_build`, transmuteurs), `:1201` (une variante voisine),
  `:1306` (`raw_boost`, palier d'extraction), `:1466` (fabrication d'armes,
  crédite bien `RES_ARMS_LIGHT` en stock mais le PAIEMENT `credit_spend`
  ne va à personne).
  Tous appellent `credit_spend(econ, w, cid, cost)` (`cost = MANUF_BUILD_COST
  (50) × tier × IPM`, ou variantes) **sans jamais router par
  `intertrade_market_consume`** — contrairement au chantier d'ÉDIFICE joueur
  (`scps_agency.c:372-394`, `agency_build_acct`) qui, LUI, paie intégralement
  les sources de matière (import) + la cité-état hôte (péage) — un flux
  parfaitement conservé (voir §4 TRANSFERT). **La construction de MANUFACTURE
  (civile, joueur ET IA) est donc un pur puits, alors que la construction
  d'ÉDIFICE (joueur, agency) est un TRANSFERT propre.** C'est une incohérence
  structurelle entre deux chemins de construction qui se ressemblent côté
  joueur — à unifier avant M3 (le chemin agency devrait servir de modèle).

### 2.9 — Salaires militaires, recrutement, marine, conseil, audits, décrets
Famille homogène : le trésor de la capitale paie un service (soldat, matelot,
conseiller, auditeur anti-corruption, décret actif) et **personne n'est
crédité** — ni le soldat (`strata[LABORER].wealth` n'est jamais touché ici),
ni un fournisseur.
- **`scps/scps_warhost.c:311-313`** (solde des régiments, `FX_SOLDE`) et
  **`scps/scps_warhost.c:392-397`** (prix au recrutement, `REGIMENT_PRICE=12`
  /régiment levé).
- **`scps/scps_navy.c:139-140`** (construction de coque), **`:168-169`**
  (conversion marchand↔pirate), **`:216-222`** (solde mensuelle de la flotte,
  `NAVY_UPKEEP_WAR=1.5`/`OTHER=0.8` par coque/an).
- **`scps/scps_statecraft.c:448-455`** (coût du Conseil, `FX_CONSEIL`).
- **`scps/scps_ai.c:2737-2747`** (audit anti-corruption, coût `50+8×corruption`
  ×IPM×[2 si faction au pouvoir]).
- **`scps/scps_decrees.c:182-193`** (`decree_spend_capital`, décisions
  ponctuelles) et **`scps/scps_decrees.c:194-207`**
  (`decree_afford_capital`, orientations mensuelles, tout-ou-rien).
- **`scps/scps_revolt.c:947-948`** (`CONCEDE_GOLD=150`, acheter la paix d'une
  révolte plutôt que la réprimer).
  **Conversion M3** : c'est la même famille conceptuelle que 2.1-2.6 — un
  service public qui devrait payer QUELQU'UN (le soldat vit de sa solde
  aujourd'hui inexistante monétairement — son entretien EN NATURE est ailleurs,
  `unit_roster.entretien_*`, mais l'OR de la solde, lui, part dans le vide).

### 2.10 — La fabrication d'un casus belli (corruption)
- **`scps/scps_diplo.c:655`** : `econ_region_treasury_add(econ, cr, -cost)`.
  Le commentaire du code lui-même le dit : « l'or SORT et disparaît
  (corruption) ». DESTRUCTION assumée et documentée — bon candidat pour rester
  un sink volontaire même après M3 (la corruption n'a pas vocation à enrichir
  qui que ce soit de traçable).

### 2.11 — Intérêt de dette sans créancier assigné (cas limite de §5 DETTE)
- **`scps/scps_credit.c:104`** débite TOUJOURS l'intérêt du débiteur, mais
  **`scps/scps_credit.c:106-108`** ne crédite un créancier QUE si
  `g_creditor[c]>=0` (un prêteur a déjà été assigné par `credit_spend`, ligne
  77-79, lequel exige `country_gold_prov(c)<0.0` ET `g_creditor[c]<0` — donc à
  la toute première année de dette, ou si `pick_lender` ne trouve aucun
  créancier solvable, l'intérêt de cette année-là est un pur sink). Voir §5.

### 2.12 — Le pillage/siège convertit du STOCK détruit en OR créé côté
  occupant (frontière CRÉATION/DESTRUCTION, classé ici car le bilan monétaire
  net du monde est positif)
- **`scps/scps_diplo.c:1338-1351`** (`diplo_pillage_value`) et
  **`scps/scps_diplo.c:1394-1407`** (`diplo_siege_loot`) : la part en LIQUIDE
  du butin est un vrai TRANSFERT (`pp->treasury -= gold` / `... += gold`,
  conservé, voir §4). Mais la part en STOCK (`taken = -econ_region_stock_add
  (...)`, valorisée `taken*price`) est **retirée du stock de la victime SANS
  jamais être livrée au stock de l'occupant** — seul le trésor de l'occupant
  est crédité (`econ->prov[dpid].treasury += loot`). Le bien looté est
  détruit (perdu pour tout le monde) tandis que sa VALEUR EN OR, elle,
  apparaît intacte chez l'occupant : net CRÉATION de monnaie adossée à une
  DESTRUCTION de matière — un vrai pillage devrait soit livrer le bien
  physique (transfert pur), soit ne pas le monétiser pour l'occupant (perte
  sèche pour la victime, rien pour personne).
  **Conversion M3** : livrer le stock pillé physiquement au vainqueur
  (`econ_region_stock_add(dst_region, g, taken)`) au lieu de le monétiser —
  cohérent avec le principe « on transporte, on ne dévalue pas ».

### 2.13 — Commerce intra-empire (`scps_trade.c`) : la marge de transport
  s'évapore (module ACTIF, distinct d'`scps_intertrade.c`)
- **`scps/scps_trade.c:240-259`** (`trade_tick`, appelé par
  `scps_sim.c:1204`, chaque tick, EN PLUS d'`intertrade_tick`). Le vendeur
  (bourgeois exportateurs) est crédité `revenue = vol × prix_importateur ×
  (1 − transport_cost)` (ligne 242-245), mais l'acheteur est débité
  `cost_imp = received × prix_importateur` où `received = vol×(1 −
  transport_cost×0.10)` (ligne 232-247) — **deux formules de perte
  différentes** (`transport_cost` complet côté prix vendeur, seulement 10 % de
  `transport_cost` côté volume acheteur). Puisque `(1−tc) < (1−0.1×tc)` pour
  tout `tc>0`, l'acheteur paie SYSTÉMATIQUEMENT plus que le vendeur ne touche
  — la différence `vol×prix×0.9×tc` **disparaît à chaque transaction inter-
  région intra-empire**, sans qu'aucun transporteur/route ne l'empoche (à la
  différence des routes d'`scps_intertrade.c`, où le péage va bien à un
  détenteur de détroit ou une cité-état hôte). Petit par transaction, mais
  systématique et actif chaque mois sur tout le réseau interne — à chiffrer.
  **Conversion M3** : soit aligner les deux formules (perte unique, cohérente),
  soit créditer la perte à un acteur (route/caravanier), au choix du design.

---

## 3. TRANSFERT (conservé — le débit d'un compte égale le crédit d'un autre)

Ces sites sont DÉJÀ sains ; listés pour mémoire (le contrat demande le
registre COMPLET, pas seulement les trous) — aucune action requise pour eux
en M3 au-delà de les laisser tels quels, sauf note contraire.

- **Impôt d'État** — `scps/scps_econ.c:3108-3111` : `strata[c].wealth -=
  collected` → `re->treasury += collected`. Conservé.
- **Redépense (part payroll)** — `scps/scps_econ.c:3201-3204` : 60 % de la
  redépense (§2.6) revient aux strates au prorata de leur impôt versé. Conservé
  (mais sourcé d'un trésor déjà alimenté par l'impôt — pas une création, un
  vrai recyclage).
- **Mobilité de classe** — `scps/scps_econ.c:2602-2607` (`mobility_move`,
  promotion/démotion laboureur↔bourgeois↔élite) : la richesse SUIT la
  population qui change de strate, dans la même province. Conservé.
- **Migration interne (bourgeois/élite)** — `scps/scps_econ.c:4326-4336`
  (`econ_migrate_tick`) : richesse transférée province source → province
  destination proportionnellement à la population qui migre. Conservé (note :
  `CLASS_SLAVE` explicitement exclu, §II.6 H — les esclaves ne migrent pas
  d'eux-mêmes, cohérent avec la doctrine).
- **Marché intertrade (achat/vente/consommation de chantier)** —
  `scps/scps_intertrade.c:481-664` (`intertrade_market_consume`,
  `intertrade_market_buy`, `intertrade_market_sell`) et le péage
  `scps/scps_agency.c:372-394` : chaîne ENTIÈREMENT conservée — le joueur paie
  `gold` (via `credit_spend`) qui se décompose exactement en (a) le nu des
  matières importées, versé aux sources étrangères par `intertrade_market_
  consume` (`scps_intertrade.c:501,507`), et (b) la marge de transport/double
  taxe, versée à la cité-état hôte du péage (`scps_agency.c:384`). Les
  matières puisées dans l'empire propre sont explicitement GRATUITES (aucun
  flux monétaire — juste une réquisition physique, hors périmètre).
- **Marché intertrade régional (routes empire↔empire)** —
  `scps/scps_intertrade.c:970-1026` : acheteur débité `total`, vendeur crédité
  `total` intégralement (répartis vendeur/péage détroit), conservé —
  contrairement à `scps_trade.c` (§2.13) qui, lui, fuit.
- **Marché des esclaves (Centres)** — `scps/scps_intertrade.c:711-746`
  (`intertrade_slave_sell`) et `:753-801` (`intertrade_slave_buy`) : vendeur
  crédité, acheteur débité, montants égaux (le pool d'héritage est fongible
  mais l'or est conservé). Sain.
- **Tribut de base (servage/protectorat)** — `scps/scps_diplo.c:332-343` :
  débit vassal → crédit suzerain, conservé (contraste avec §1.4, le tribut
  « mûri » qui LUI fuit).
- **Le don (achat de loyauté d'un vassal frondeur)** —
  `scps/scps_diplo.c:490-497` : suzerain débité, vassal crédité. Conservé.
- **Pillage/siège (part liquide), réparations de guerre, butin final** —
  `scps/scps_diplo.c:1338-1339` (trésor du pillage), `:1569-1589`
  (`diplo_reparations`), `:1596-1614` (`diplo_loot`), `:1135-1147`
  (`diplo_peace_take_gold`) : perdant débité, vainqueur crédité, montants
  égaux. Sain (voir §2.12 pour la nuance stock-vs-or du même mécanisme).
- **Butin de stock (pillage, règlement de paix)** —
  `scps/scps_diplo.c:1149-1164` (`diplo_peace_pillage_stock`) : le stock
  retiré de la victime est bien livré physiquement au vainqueur
  (`econ_region_stock_add(econ,dst,g,take)`, ligne 1160) — un pur transfert de
  MATIÈRE, aucun `treasury`/`wealth` en jeu, donc hors périmètre monnaie à
  proprement parler. Noté ici car son motif propre (livrer, pas monétiser)
  contraste avec §2.12 (`diplo_pillage_value`/`diplo_siege_loot`, qui EUX
  monétisent sans livrer) — le même fichier contient donc les deux
  philosophies, à 200 lignes d'écart.
- **Routes de détroit (péage du verrou)** — `scps/scps_intertrade.c:999-1011` :
  exportateur débité, tenant du détroit crédité. Conservé.

---

## 4. DETTE (catégorie à part — ni création ni transfert au sens strict)

### 4.1 — `credit_spend` : le principal n'est jamais avancé par un prêteur réel
- **`scps/scps_credit.c:67-80`**. `e->prov[pid].treasury -= cost` peut faire
  passer le trésor NET en négatif — c'est la « dette ». MAIS la dépense
  elle-même (`cost`, quel qu'il soit — un chantier, une solde, une manufacture)
  suit sa propre classification (TRANSFERT si `credit_spend` sert un site
  conservé comme `agency_build_acct`, DESTRUCTION si elle sert un site du §2).
  Le PASSAGE en négatif n'est adossé à AUCUN prêteur qui aurait réellement
  avancé les fonds — le déficit est de la monnaie dépensée qui n'a jamais
  existé. Ce n'est qu'APRÈS COUP (l'année suivante, `credit_year_tick`) qu'un
  créancier est assigné (`pick_lender`, le plus riche cité-état/mercantile/
  pacifiste solvable) et commence à toucher un intérêt bien réel sur cette
  dette fictive.
- **`scps/scps_credit.c:85-109`** (`credit_year_tick`) : l'intérêt annuel,
  LUI, est un vrai TRANSFERT quand un créancier est assigné (débiteur → 
  créancier, §3) — mais voir §2.11 pour le sink quand aucun créancier n'est
  encore assigné.
  **Conversion M3 (Cœur B du plan)** : un emprunt doit DÉPLACER des pièces
  réelles au moment de l'emprunt (le coffre du prêteur se vide QUAND le
  débiteur emprunte, pas un an plus tard sur les intérêts) ; une trésorerie
  négative ne doit plus exister comme « monnaie négative » — c'est exactement
  ce que documente déjà `docs/MONNAIE_CONCEPT.md` §M3 Cœur B.

---

## 5. INITIALISATION (mesure de M(0), pas une dérive)

### 5.1 — La dotation de genèse
- **`scps/scps_econ.c:1032-1038`** (`econ_seed_population`), appelée à la
  genèse pour : la capitale du joueur/antagoniste (`:1760`), chaque hameau
  POLITY_WILD planté (`:1869`). Formule : `wealth = pop × (6 élite / 2
  bourgeois / 0.5 laboureur+esclave)`. C'est M(0) — À MESURER par sim, pas à
  corriger.
- **Kit de départ (stock, hors périmètre monnaie)** — `scps/scps_econ.c:1811-
  1843` (bois/grain/argile/fer/pierre/outils/armes/bière) et pool cité-état
  `scps/scps_econ.c:1787-1802` (`CS_TRADE_POOL=1000`) : matière, pas monnaie —
  noté pour mémoire car il alimente indirectement la VA future (§1.1).
- **⚠ Rappel important** : `econ_seed_population` est LA MÊME fonction que
  §1.2 (colonisation en cours de partie) — seule la PREMIÈRE vague d'appels
  (à `econ_init`, avant le premier `econ_tick`) compte comme M(0) ; tout appel
  ultérieur (`econ_colony_day`, `colonize_from_prov`) est une CRÉATION (§1.2),
  pas une initialisation.

---

## 6. TOTAUX PAR CATÉGORIE (mesurés — `./chronicle <seed> 3 250 6 12`)

Protocole : `make chronicle` puis `./chronicle 9 3 250 6 12` (graine demandée)
+ `./chronicle 11 3 250 6 12` + `./chronicle 42 3 250 6 12` (3 graines × 3 sims
= 9 mondes, 6 empires/12 cités-états fixes, 250 ans, sans joueur). Lecture pure
(la télémétrie §7) ; golden/déterminisme vérifiés inchangés (§8) sur le même
binaire.

| graine | sim | M(0) | M(fin) | dérive/an | création mesurée (FX) | destruction mesurée (FX) |
|---|---|---:|---:|---:|---:|---:|
| 9  | 1 | 57 000 | 52 064 313 | +208 029 | 36 076 369 | 36 708 625 |
| 9  | 2 | 49 500 | 20 416 466 |  +81 468 | 17 170 844 | 17 576 653 |
| 9  | 3 | 57 000 | 52 963 905 | +211 628 | 31 777 270 | 31 956 907 |
| 11 | 1 | 57 000 | 72 962 808 | +291 623 | 48 037 025 | 48 008 431 |
| 11 | 2 | 57 000 | 52 869 984 | +211 252 | 38 484 696 | 38 807 345 |
| 11 | 3 | 57 000 | 36 653 407 | +146 386 | 27 386 845 | 27 927 614 |
| 42 | 1 | 52 000 | 42 772 854 | +170 883 | 38 281 503 | 38 346 577 |
| 42 | 2 | 57 000 | 46 747 848 | +186 763 | 33 514 087 | 34 588 687 |
| 42 | 3 | 57 000 | 53 597 928 | +214 164 | 41 194 782 | 41 841 086 |

**Lecture** : M(0) ~ 50-57 k or (la dotation de genèse, §5.1 — varie légèrement
avec le nombre d'empires/cités effectivement peuplés). M(fin), 250 ans plus
tard : **20 à 73 MILLIONS d'or** — un facteur ×360 à ×1280 sur la masse
monétaire initiale, dans les 9 mondes testés, SANS AUCUNE frappe (le concept
de M1+ n'existe pas encore). La dérive nette est TOUJOURS positive (+81 k à
+292 k or/an) : le monde IMPRIME nettement plus qu'il ne brûle.

Le recoupement FX_* (création/destruction « mesurées », qui ne couvre QUE les
sites déjà instrumentés — impôt, entretien, cour, admin, redépense, soldes,
marine, conseil, audits, chantiers, péages, intérêts, intrigues — PAS la VA de
production §1.1, PAS la consommation §2.1, PAS la colonisation §1.2) est
lui-même **quasi équilibré** (création ≈ destruction à 1-2 % près sur les 9
sims) — cela confirme que **la dérive nette du monde n'est PAS pilotée par
ces sinks/sources administratifs (déjà à peu près calés l'un sur l'autre),
mais par le DÉSÉQUILIBRE entre §1.1 (VA créée, non instrumentée) et §2.1
(consommation détruite, non instrumentée)** — exactement les deux sites que
le contrat désignait déjà comme prioritaires (« sans les puits, on
supprimerait les planches à billets en gardant les trous noirs »). La VA
créée (§1.1) croît avec la population/le PIB (composé sur 250 ans) plus vite
que la consommation ne peut la détruire (plafonnée par le panier de besoins
per capita) — d'où la dérive exponentielle observée.

Résumé qualitatif par catégorie :
- **CRÉATION** dominée de très loin par §1.1 (VA de production, non
  instrumentée FX — c'est elle qui explique l'essentiel des dizaines de
  millions d'écart) ; les autres sites (§1.2 colonisation, §1.3 arbitrage,
  §1.4 tribut mûri, §1.5 missions, §1.7 événements positifs) sont mineurs en
  comparaison mais non nuls et NON conservés.
- **DESTRUCTION** dominée par §2.1 (consommation, non instrumentée FX) et
  §2.6 (redépense publique, part non-payroll, elle EST instrumentée FX_REDEP)
  — les sinks militaires/administratifs (§2.2-2.5, 2.9) sont individuellement
  petits mais nombreux et permanents, et représentent l'essentiel du volume
  FX_* mesuré ci-dessus.
- Le monde n'est PAS à l'équilibre par construction : rien ne garantit
  Σcréation ≈ Σdestruction (ce sont des familles de formules indépendantes,
  jamais calées l'une sur l'autre). La dérive nette est un artefact de
  paramétrage, pas un signal économique voulu — raison d'être de M3.

---

## 7. TÉLÉMÉTRIE (chronicle, print-only)

`scps/chronicle.c` imprime désormais, une fois par simulation, une ligne :

```
masse monétaire : M(0)=<X> · M(fin)=<Y> · dérive +<Z>/an (création <A> ·
destruction <B> mesurées)
```

où `M(t) = Σ_provinces treasury + Σ_provinces Σ_classes wealth` (l'or national
`econ_country_gold` est déjà inclus car `econ_country_gold` = Σ trésor des
RÉGIONS possédées = même somme que Σ trésor des PROVINCES, agrégation
oblige — la ligne somme directement `prov[].treasury` pour rester au grain
province, doctrine oblige). `création`/`destruction` mesurées = Σ des deltas
positifs / négatifs de `econ_flux_get(FX_*)` cumulés sur toute la sim, pour
CHAQUE pays, sommés — une lecture partielle (elle ne couvre QUE les sites déjà
instrumentés en `econ_flux_add`, soit environ la moitié du registre ci-dessus
— §1.1, §1.2, §2.1 et §2.13 par exemple n'ont PAS de compteur `FX_*` dédié) :
elle sert de RECOUPEMENT grossier, pas de vérité — la vérité est `M(fin) −
M(0)`, mesurée en Σ RÉELLE, qui capture TOUS les sites sans exception (y
compris ceux non instrumentés en flux).

Aucun effet sur la sim (lecture pure de `prov[].treasury`/`.wealth`, imprimée
en fin de run) — golden inchangé, vérifié §8.
