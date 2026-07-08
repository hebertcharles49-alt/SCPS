# SCPS — Registre des leviers (modificateurs ajustables)

Tous les curseurs sur lesquels on a la main, dérivés de `scps/scps_tune_list.h` (la source de
vérité). Valeurs = défauts compilés au 2026-07-09 (SAVE v74).

## Comment ajuster

| Canal | Portée | Usage |
|---|---|---|
| **`SCPS_TUNE="NOM=VAL,…"`** (env) | les ~200 scalaires ci-dessous | `SCPS_TUNE=POP_R_BASE=0.023 ./chronicle 9 5 250`. Nom inconnu → refus (exit 2). |
| **Panneau F10** (Godot) | idem, en direct | Édite un tunable à chaud pendant la partie. |
| **`SCPS_MODS=fichier`** (env) | tables : prix, rendements, recettes, coûts tech, stats d'unités | `chronicle --dump-data` écrit le point de départ éditable. |
| **`scps_lang.txt`** | tous les textes face-joueur (`STR_*`) | `--dump-lang` puis F4 pour recharger. |

⚠ Une surcharge active rend la partie **non-rejouable en vanilla** (empreinte `tune_ck` dans la save,
avertit au reload). Sans surcharge = valeurs compilées = golden-safe.

---

## Économie — le robinet d'or (flux d'État)

| Tunable | Défaut | Effet |
|---|---|---|
| `ENTRETIEN_DIV` | 400 | diviseur de l'entretien d'État |
| `MANUF_UPKEEP_DAY` | 0.05 | entretien journalier d'une manufacture |
| `COURT_FLOOR` | 4000 | plancher du coût de cour |
| `COURT_RATE` | 0.010 | taux du coût de cour ∝ trésor |
| `ADMIN_BASE` / `ADMIN_EXP` | 0.4 / 1.3 | base & exposant du coût administratif (croît avec la taille) |
| `SINK_FLOOR` | 500 | plancher du puits monétaire |
| `TRADE_LEVY` | 0.10 | prélèvement de l'importateur sur une route (conservation : 0 faucet/0 sink) |
| `IMPORT_MARGIN_OWN` / `_THIRD` / `_NONE` | 1.3 / 1.8 / 2.0 | marge d'import selon le propriétaire du Centre (soi / tiers / personne) |
| `IMPORT_TOLL_FRAC` | 0.30 | péage versé à la cité-état hôte du Centre |
| `CREDIT_LINE_BASE` | 0.5 | taille de la ligne de crédit ∝ pop |
| `CREDIT_RATE_BASE` | 0.05 | taux d'intérêt de base (price le risque) |
| `CREDIT_RATIO_CAP` | 8.0 | anti-spirale : au-delà de ce ratio dette/ligne, l'intérêt plafonne (évite le NaN ~an 105) |

## Économie — extraction, nourriture, marché

| Tunable | Défaut | Effet |
|---|---|---|
| `EXTRACT_GEO_REF` | 4.5 | qualité de tuile donnant geo_eff = 1 |
| `EXTRACT_GEO_CAP` | 3.0 | plafond de qualité géo (multiplicateur d'extraction) |
| `EXTRACT_LABOR_SHARE` | 0.65 | **part des journaliers à l'extraction** (le levier du volume brut ; le reste staffe les manufactures) |
| `FOOD_NEED` | 1.0 | multiplicateur de la bouche vivrière (levier anti-famine ; 1.0 = table brute) |
| `SPAWN_FOOD_RAW` | 12 | socle de grain sur la capitale de chaque empire (seule règle vivrière de worldgen) |
| `REGION_RAW_KEEP` | 2 | nb de brutes gardées par région (sa vocation ; la traîne vient du commerce) |
| `MARKET_DIST_FALLOFF` | 0.12 | rendement dégressif du marché par saut de distance |
| `ARB_VOL_CAP` / `ARB_MIN_SPREAD` / `ARB_CAPTURE` | 3.0 / 0.20 / 0.35 | arbitrage borné des cités-états (volume, spread mini, part captée) |
| `SPEC_BUY_BAND` / `SPEC_SELL_BAND` / `SPEC_GOLD_FLOOR` | 0.80 / 1.25 / 350 | bandes du stockeur spéculatif IA (E3) |
| `CS_TRADE_POOL` | 1000 | réserve tradable de brutes déposée sur chaque cité-état (les empires nus importent) |

## Économie — confort & exploitation

| Tunable | Défaut | Effet |
|---|---|---|
| `COMFORT_JOY` | 0.08 | bonheur hors-panier par bien de confort servi (poterie/statuaire) |
| `COMFORT_HOUSE_RELIEF` | 0.15 | −15 % de besoin de logement quand le confort est servi |
| `RAW_BOOST_PER_TIER` / `_MAX_TIER` | 0.05 / 8 | +5 % d'extraction par palier d'exploitation bâti, plafonné 8 |
| `RAW_BOOST_COST` / `_PAYBACK` | 40 / 8 | coût d'or par palier · ROI visé (rembourse en ≤ 8 ans) |
| `RAW_WORKS_NEED` | 25 | seuil de déficit du trio de bâti qui arme un raw-works (four/carrière/scierie) |
| `PLAYER_GUARANTEE_RAW` | 4 | argile/pierre/fer/bois forcés près de la capitale joueur si le biome n'en donne pas |

## Manufactures, armes, construction

| Tunable | Défaut | Effet |
|---|---|---|
| `MANUF_BUILD_COST` | 50 | coût d'or de base (× tier × ipm) pour qu'une IA pose une manufacture |
| `MANUF_ARMS_MULT` | 10 | ×N au stock d'armes d'une manufacture d'armes (l'arsenal que la levée pompe) |
| `ARMS_PER_LABORER` | 0.05 | cible d'arsenal d'État ∝ bras (demande de marché → l'armurerie se bâtit) |
| `ARSENAL_DECAY` | 0.99 | rouille lente des armes (1 %/mois vs 15 % des périssables) |
| `BUILD_RESERVE_BULK` | 15 | fond de trio (bois/pierre/argile) qu'une région garde avant d'exporter |
| `HOUSE_MANUF` | 100 | logements bâtis par niveau de manufacture (le doublement Q6 : ½·cap → plein) |

## Population — capacité & genèse

| Tunable | Défaut | Effet |
|---|---|---|
| `EMPIRE_CAP` / `CITY_CAP` | 13000 / 6500 | taille pleine nourrie d'un empire / d'une cité-état |
| `EMPIRE_SEED` / `CITY_SEED` | 4000 / 2000 | pop an-0 semée par entité |

## Population — fertilité & vitalité

| Tunable | Défaut | Effet |
|---|---|---|
| **`POP_R_BASE`** | 0.01733 | **LE levier de vitalité** (ln2/40) : le monde est bistable — plus haut = plus plein & contesté, plus bas = « mou » figé au bassin ½·cap. Dialer vers /35 (0.0198) ou /30 (0.0231). |
| `POP_PROSP_MID` / `_SPAN` / `_W` | 0.2 / 1.8 / 0.15 | normalisation & poids de la prospérité (PIB/tête) dans le bonus de fertilité |
| `POP_NEEDS_W` | 0.85 | poids des besoins satisfaits dans la fertilité |
| `POP_SAT_W` | 0.20 | surcroît de croissance d'une province contente (asymétrique : la basse ne punit pas) |
| `NEEDS_MET_TAU` | 0.5 | seuil de couverture (got ≥ τ) comptant une catégorie comme satisfaite |
| `HAB_MALUS_K` | 0.20 | malus de terre rude sur prod ET croissance ((1−hab)·K ; exempte la région-siège) |

## Consommation individuelle par classe (le panier de besoins)

Table `NEED[CLASS][RES]` (`scps_econ.c`), **par 100 hab / tick mensuel** (×12/an). La nourriture
(grain/poisson, interchangeables via `food_sat`) est annualisée (A2) et modulée par le tunable
`FOOD_NEED`. Le reste (confort/statut) garde ses valeurs. ⚠ Table `const` compilée — le seul levier
runtime est `FOOD_NEED` (nourriture) ; le reste demande une recompilation (ou un futur canal `SCPS_MODS`).

| Bien | Journalier | Bourgeois | Élite | Esclave |
|---|---|---|---|---|
| **Grain** (nourriture) | 3.50 | 4.00 | 4.00 | 3.50 |
| **Poisson** (nourriture, interchangeable) | 1.00 | — | — | — |
| **Bois de feu** | 1.00 | — | — | — |
| **Eau-de-vie / bière** | 0.35 | 0.30 | 0.28 | — |
| **Tunique** (drap grossier) | 0.40 | — | — | — |
| **Drap** (cloth) | — | 0.34 | — | — |
| **Fourrure** | — | — | 0.12 | — |
| **Papier** | — | 0.25 | 0.12 | — |
| **Sel** | — | 0.20 | — | — |
| **Remède** (santé urbaine) | — | 0.15 | — | — |
| **Poterie** (confort) | 0.30 | 0.25 | — | — |
| **Statuaire** (ornement/statut) | — | 0.12 | 0.18 | — |
| **Orfèvrerie** (`PRECIOUS_WARE`, statut) | — | — | 0.13 | — |

**Ordre de déblocage** (`NEED_ORDER`) — le nombre de besoins *comptés dans la satisfaction* croît
avec le niveau de la capitale (∝ pop) : un bourg n'aspire qu'aux bases, une grande capitale à tout le
panier (le luxe se **mérite**). Le palier STATUT vient toujours en dernier.
- **Journalier** : grain → bière → poisson → bois de feu → tunique → poterie
- **Bourgeois** : grain → sel → drap → remède → bière → papier → poterie → statuaire
- **Élite** : grain → fourrure → papier → bière → orfèvrerie → statuaire
- **Esclave** : grain **seul** (le plancher vital, toujours débloqué, aucun confort — §II.6/H)

Notes : l'**outil** n'est PAS dans le panier (il ne touche que la productivité, jamais la
satisfaction). Le seuil d'accession journalier→bourgeois = `PROMOTE_BASKET_MULT` **1.4×** le panier.

## Modificateurs provinciaux (le slot « MODIFICATEURS », entrée DÉMO)

| Tunable | Défaut | Effet |
|---|---|---|
| `PROVMOD_ABOND_REF` / `_K` | 0.45 / 2.0 | **Terre d'abondance** : une région sous-peuplée + nourrie + en paix se repeuple vite (rebond des low seeds ; auto-ciblé) |
| `PROVMOD_FERVEUR_K` / `_DECAY` | 0.5 / 0.067 | **Ferveur fondatrice** : élan d'une colonie fraîche, décru ~15 ans |
| `PROVMOD_RECON_K` / `_DECAY` | 0.6 / 0.10 | **Reconstruction** : renaissance d'après-choc, libérée à mesure que la cicatrice se referme |
| `PROVMOD_LIMON_K` | 0.15 | **Limon fertile** : natalité dense d'un delta |
| `PROVMOD_GIBIER_K` / `PROVMOD_HALIEU_K` | 0.10 / 0.10 | **Gibier / manne halieutique** : dons géo (1/3 des bois / des côtes) |
| `PROVMOD_ADMIN_K` | 0.06 | **Bonne administration** : institutions bâties → natalité un peu plus dense |

## Brassage — migration, réfugiés, assimilation, esclavage

| Tunable | Défaut | Effet |
|---|---|---|
| `MIG_PACT_FRAC` | 0.006 | pacte commercial : part du dominant qui migre/an (calibrage d'origine, golden-safe) |
| `MIG_PACT_FRAC_ALLY` | 0.05 | pacte d'alliés : ×élevé (après l'an-12) |
| `MIG_PACT_FRAC_LATE` | 0.02 | taux de base APRÈS l'an-12 (volume : « pas 100 pélos qui déstabilisent ») |
| `MIG_PACT_MIN` | 30 | plancher anti-poussière d'un flux de pacte |
| `MIG_PACT_ALLY_GATE_DAYS` | 4380 | jour (12 ans) après lequel les taux élevés s'arment (golden) |
| `MIG_ATTRACT_INST_W` / `MIG_PULL_MAX` | 1.0 / 5.0 | attractivité = prospérité + bâti ; flux ×jusqu'à 5 selon le gradient |
| `REFUGEE_FLEE_SCAR` | 0.40 | cicatrice de sac/révolte au-delà de laquelle on fuit |
| `REFUGEE_FLEE_FRAC` | 0.12 | part d'un groupe qui fuit/an d'une région ravagée (l'exode historique) |
| `REFUGEE_FLEE_MIN` | 30 | plancher d'une fuite |
| `REFUGEE_HOME_CALM` | 0.25 | foyer sous ce seuil ⇒ retour possible |
| `REFUGEE_RETURN_PULL` | 0.12 | part du réfugié qui rentre/an |
| `MIGRANT_RETURN_PULL` | 0.015 | retour ténu du migrant économique (aucun déplacement définitif) |
| `REFUGEE_SETTLE_INTEG` | 0.90 | intégration au-delà de laquelle un réfugié se fixe |
| `ASSIM_K_INST_REF` / `_AMP` | 1.5 / 4.0 | les institutions de l'hôte accélèrent l'intégration (école/service = assimile vite) |
| `SLAVE_FRACTION` | 0.05 | part de pop déportée à chaque pillage d'un esclavagiste (tech OU éthos conquérant) |
| `SLAVE_PRICE` | 40 | prix de base d'une âme au marché des Centres |
| `SLAVE_POOL_REF` | 600 | profondeur de référence du pool (rare ⇒ ×2.5, surabondant ⇒ ×0.5) |
| `SLAVE_AI_KEEP_FRAC` / `_SELL_FRAC` | 0.02 / 0.25 | l'IA garde 2 % de pop servile, vend 25 % de l'excédent/an |
| `SLAVE_AI_BUY_FRAC` | 0.20 | un esclavagiste en pénurie de bras comble 20 %/an de son déficit au pool |
| `SLAVE_REVOLT_SHARE` / `_W` | 0.20 / 1.20 | au-delà de 20 % de part servile, la région pousse structurellement la révolte |

## Worldgen — échelle & spawn

| Tunable | Défaut | Effet |
|---|---|---|
| `WORLD_PROV_BASE` | 24 | base du nombre de territoires |
| `WORLD_PROV_PER_EMPIRE` | 120 | densité par empire (place : capitale + colonisation + tampon) |
| `WORLD_PROV_PER_CITY` | 5 | territoires par cité-état |
| `WORLD_EMP_COMFORT_LOW` / `_FULL` | 0.58 / 6.0 | compacité bas régime (à 2 empires, duel serré ; 6+ = vaste) |
| `WORLD_PROV_SAT_K` | 124416 | calage de saturation Poisson (germes/pas²) |
| `SPAWN_SAFE_HOPS` / `_MIN` | 6 / 5 | distance mini entre empires à la genèse (adaptatif : resserre pour tout caser) |

## IA — recherche & savoir

| Tunable | Défaut | Effet |
|---|---|---|
| `AI_SAVOIR_K` | 2.5 | échelle du savoir IA |
| **`AI_RESEARCH_INCOME_W`** | 4.5 | **multiplicateur global du revenu de recherche** (arbre méd 28 → 50 % ; découplage §27 sur les nœuds faustiens) |
| `SAVOIR_W_ELITE` / `_BOURGEOIS` / `_LABORER` | 0.01 / 0.005 / 0.001 | production de recherche par classe/an |
| `SAVOIR_LIB_PER` / `_MAX` | 0.067 / 0.33 | bonus % de la chaîne Bibliothèque (Σ savoir bâti, plafond +33 %) |
| `AI_SAVOIR_CATCHUP_FRAC` | 0.45 | un pays sous 45 % de la médiane bâtit sa Bibliothèque quel que soit son éthos |
| `AI_METAB_RES_W` | 1.0 | +X % recherche = X % d'âmes étrangères digérées (creuset) |
| `AI_TECH_DIFFUSE_MAX` | 0.40 | une tech possédée par tous les autres coûte −40 % (catch-up) |
| `METAB_TIER1` / `_TIER2` / `_TIER3` | 0.10 / 0.20 / 0.35 | part digérée d'un héritage débloquant ses signatures par tier |
| `METAB_DIFFUSE_SLAVE` | 0.30 | le déporté diffuse faible (savoir arraché) |
| `SYNC_TRADE_SEA_W` / `_LAND_W` | 2.0 / 1.0 | le commerce ouvre l'archétype (Venise ← Grèce), la mer pèse fort |
| `SYNC_TRADE_METIER` / `_PROFOND` / `_YIELDREF` | 1.0 / 2.0 / 5.0 | seuils de diffusion → accès recherche ; module par volume |
| `SYNC_FUSE_RATE` | 0.10 | vitesse de cristallisation culturelle par contact maritime soutenu |

## IA — commerce, colonisation, prévision

| Tunable | Défaut | Effet |
|---|---|---|
| `COMMERCE_W_BOURGEOIS` / `_ELITE` | 0.04 / 0.01 | volume échangeable produit par la pop marchande/mois |
| `COMMERCE_BLD_PER` / `_MAX` / `_ECO_W` | 0.10 / 0.50 / 0.05 | bonus de la chaîne commerciale ; poids dans la puissance éco diplo |
| `AI_SAFETY_HORIZON` | 12 | runway sous ce nb d'années = urgent (le stress monte) |
| `AI_PROJ_HORIZON` | 25 | fenêtre de projection du shortfall |
| `AI_SAFE_STOCK_MONTHS` | 6 | coussin de réserve visé (mois de conso) |
| `COLONY_SURVIVE_SEED` | 0.5 | fraction semée par une colonie de survie (anti-spirale poule-œuf) |
| `AI_COLONY_NEEDS_W` | 1.5 | poids du biais vers une tuile-déficit au-dessus de l'expansion de capacité |
| `AI_COLONY_TEMPO` | 3.0 | la cadence de colonisation suit la personnalité (Dominateur ~1 an, Pacifiste ~4) |

## IA — guerre & prévision diplo

| Tunable | Défaut | Effet |
|---|---|---|
| `AI_WAR_DECISIVE` / `_EXHAUST` | 50 / 10 | score décisif de paix / timeout de paix blanche |
| `AI_WAR_BASELINE` | 0.05 | socle d'appétit de guerre (les mondes consolidés voient quand même la guerre) |
| `AI_WAR_SATURATION` / `_CAP` | 0.20 / 3.0 | frein anti-spirale ∝ paires en guerre ; plafond |
| `AI_THREAT_GATE` / `_BRAKE` | 0.55 / 0.5 | war_risk au-delà du gate ⇒ on freine l'offensive |
| `AI_WAR_LOSING` | −25 | score de guerre en-dessous duquel on est « perdant » |
| `AI_ALLY_NEED_W` | 1.0 | surpondère l'allié qui couvre ma pire menace |
| `FAB_CB_COST_YEARS` | 2.0 | fabriquer un casus belli coûte 2 ans de revenu |
| `FAB_MATURE_DAYS` / `_VALID_DAYS` | 365 / 1825 | maturation (1 an) · fenêtre de validité (5 ans) du grief acheté |

## IA — diplomatie, opinion, vassalité

| Tunable | Défaut | Effet |
|---|---|---|
| `AI_COVET_W` | 0.5 | poids du besoin dans la valeur d'une province d'autrui (convoiter qui tient ce qui manque) |
| `AI_COMPLEMENT_W` | 1.0 | s'allier à qui me complète |
| `OPINION_ALLY` / `_WAR` / `_VASSAL` / `_PACT` / `_EMBARGO` | 50 / 60 / 30 / 15 / 25 | modificateurs de statut (disparaissent à la rupture → l'opinion tend vers 0) |
| `OPINION_RANCOR_W` | 8 | poids de la rancune territoriale (la rivalité) |
| `OPINION_MEM_DECAY` | 0.0003 | décrue/jour de la mémoire d'actes |
| `OPINION_MEM_BETRAYAL` / `_SECESSION` / `_CAP` | 35 / 45 / 100 | marques durables : trahison, sécession, plafond |
| `AI_OFFER_ALLY_OPINION` / `_PACT_OPINION` / `_MIG_OPINION` | 10 / 0 / 40 | seuils d'acceptation d'une offre entrante (le pacte migratoire exige plus de confiance) |
| `AI_VASSAL_INTEGRATE_YEARS` | 20 | temps de pleine intégration d'un vassal tenu à la paix |
| `AI_VASSAL_CONTRIB_GATE` / `_BASE` | 0.65 / 0.05 | seuil d'intégration → le vassal verse selon sa fonction (commerce/agraire/martial) |
| `AI_ANNEX_MIN_INTEGRATION` | 0.65 | intégration mini pour qu'un maître annexeur digère son vassal |
| `AI_ANNEX_YEARS_PER_PRICE` / `_GOLD_PER_PRICE` | 0.5 / 2.0 | durée & coût/an de l'annexion-processus |
| `ANNEX_INTEGRATION_DISCOUNT` | 0.6 | l'intégration raccourcit l'annexion |
| `ANNEX_SOFT_SCAR` / `_DECAY` / `ANNEX_SAT_W` | 0.4 / 0.20 / 0.5 | cicatrice douce d'annexion (frappe la stabilité, pas la croissance) |

## Guerre & bataille

| Tunable | Défaut | Effet |
|---|---|---|
| `BT_DMG_K` | 0.057 | dégâts de bataille |
| `CTR_BITE` / `CTR_PURSUIT` | 0.6 / 0.30 | mordant du contre composition-vs-composition + part de curée ∝ avantage |
| `BT_CHOC_MORTS` | 0.006 | morts au choc |
| `BT_RUPTURE` | 0.20 | seuil de rupture (déroute) |
| `CHOC_ROUNDS_BONUS` | 2.0 | bonus de rounds au choc |
| `CUREE_CAP` | 0.22 | plafond de la curée (poursuite) |
| `CAV_PURSUIT` / `CAV_CUREE_CAP` | 0.45 / 0.40 | la cavalerie fait la poursuite : part & plafond relevés |
| `REGIMENT_PAY` / `_PRICE` | 90 / 12 | solde & prix d'un régiment (armée à ~10-15 % des dépenses d'État) |
| `NAVY_UPKEEP_GOLD` | 90 | entretien naval |
| `DEF_PER_H` | 0.05 | chaque point de H_coerc (garnison bâtie) durcit la place assiégée |
| `SIEGE_LOOT_FRAC` | 0.25 | fraction de la production mensuelle détournée par une force en siège |
| `PILLAGE_INCOME_FRAC` | 0.20 | **pillage unifié** : un sac/raid/occupation prend 20 % du revenu annuel de la victime (transfert réel) |

## Rebelles & révolte

| Tunable | Défaut | Effet |
|---|---|---|
| `C3_K_HOLLOW` / `_L_HOLLOW` | 0.20 / 0.30 | creusement K/L d'une concession |
| `CONCEDE_GOLD` | 150 | coût d'une concession |
| `W_AGITATION_UNREST` | 0.20 | poids du signal d'agitation politique (légitimité) dans la révolte (dédup Option B) |
| `AI_REBEL_BACKING_OPINION` | 1.60 | seuil d'hostilité au-delà duquel un rival soutient les rebelles (2nd front) |
| `AI_REBEL_BACKING_ATWAR_W` / `_MAXWARS` | 0.35 / 1.0 | bonus « déjà belliqueux » ; le bailleur doit être en paix partout ailleurs |
| `AI_REBEL_MATERIEL_FRAC` | 0.20 | renfort de milice proportionnel à la force rebelle |
| `REBEL_VET_ADD` | 2.0 | noyau de vétérans ajouté (rend l'armée rebelle réelle → ~1 révolte sur 20 gagne) |

## Religion

| Tunable | Défaut | Effet |
|---|---|---|
| `AI_FAITH_ZEAL` | 0.5 | w_faith au-delà duquel un crédo prosélyte fonde sa foi de lui-même |
| `AI_DERIVE_ODDS` | 8.0 | 1 chance sur N/tour qu'une marche distante schisme (la Réforme mûrit sur des décennies) |

## Faustien & pente vers la Brèche

| Tunable | Défaut | Effet |
|---|---|---|
| `FAUST_SPAWN_CHARGE` | 0.15 | charge ajoutée par unité de sortie d'un transmuteur |
| `CHARGE_DECAY` | 0.04 | décrue passive de l'entropie régionale hors péché |
| `ENTROPY_TERMINAL` | 4000 | seuil d'entropie monde armant le signal terminal |
| `FAUST_BRECHE_CAUTION` | 0.55 | au-delà de cette proximité de Brèche, l'IA ne cède plus à l'échappatoire faustienne |

## Hameaux libres (POLITY_WILD)

| Tunable | Défaut | Effet |
|---|---|---|
| `WILD_PER_PLAYABLE` | 2 | hameaux libres par jouable (**0 = désactive**) |
| `WILD_POP` / `_VAR` / `_CAP` / `_FOOD` | 750 / 0 / 1600 / 8 | graine, jitter, plafond d'accueil, food forcée |
| `WILD_SPAWN_HOPS` | 3 | rayon BFS (restent près du spawn) |
| `WILD_CULTURE_DISTINCT` | 1 | culture distincte du voisin |
| `WILD_DEFECT_YEARS` | 8 | ans de contact pacifique avant ralliement culturel |
| `WILD_HOARD` / `WILD_REGIMENTS` | 60 / 2 | réserve de brutes · régiments défensifs |

## Conseil (les visages du pouvoir)

| Tunable | Défaut | Effet |
|---|---|---|
| `COUNCIL_HIRE_LEVER` / `_DISMISS_GRIEF` | 0.10 / 0.10 | recruter pousse sa faction / renvoyer froisse l'opposée |
| `COUNCIL_LOYAL_RATE` | 0.05 | vitesse de convergence de la loyauté/mois |
| `COUNCIL_ROT_BOOST` | 1.5 | le rot (capture d'État) accélère la chute de loyauté |
| `COUNCIL_PAY_ADJ` | 30 | effet de la paie sur la loyauté cible |
| `COUNCIL_BETRAYAL_THRESHOLD` | 15 | seuil « au bord de la trahison » |

## Endgame §27 — entropie, fins, exode

| Tunable | Défaut | Effet |
|---|---|---|
| `ENTROPY_FIN` | 55 | seuil terminal qui déclenche une fin |
| `ENDGAME_YEAR_OPEN` | 180 | **gate dur : aucune apocalypse avant cet an** (Merveille exemptée) |
| `ENTROPY_TECH_W` | 0.20 | poids de la charge de tech faustienne dans l'entropie (accumulateur monotone) |
| `ENTROPY_BREACH_W` | 0.3 | poids de la pression de l'Âge de la Brèche |
| `ENTROPY_BLOOD_W` | 8.0 | poids du ratio morts-de-guerre (un monde sanglant précipite sa fin) |
| `ENDGAME_BLOOD_FRAC` | 0.20 | au-delà de ce ratio, LE SANG l'emporte |
| `BLOOD_PLAYER_SHARE` | 0.25 | en partie joueur, SANG exige que sa part du sang atteigne ce seuil |
| `SANG_DRAIN_PER_YEAR` | 0.03 | fraction de pop drainée/an dans une région marquée SANG |
| `SANG_MEMORY_HL` | 40 | demi-vie de la mémoire des morts de guerre |
| `SINK_RIFTS_PER_YEAR` | 3 | régions englouties/an (fin EAU) |
| `COLD_RAMP_PER_YEAR` | 0.005 | décalage de température/an (fin FROID) |
| `THORN_CELLS_PER_YEAR` / `_RANDOM_FRAC` | 200 / 0.35 | cellules corrompues/an (fin RONCES) · part erratique |
| `EXODUS_INTENSITY_MIN` | 0.15 | la fin doit mordre avant qu'on fuie |
| `EXODUS_FRAC_PER_YEAR` | 0.10 | part de pop évacuée/an (EAU/FROID/RONCES) |
| `SANG_FLEE_FRAC` | 0.35 | part du drain SANG routée en fuite plutôt qu'au tombeau |
| `CALM_DISASTER_YEAR` / `_ENTFRAC` / `_MULT` | 200 / 0.5 / 2.5 | un monde calme (an > 200, entropie < ½ seuil) reçoit ×2.5 de catastrophes géo |

## Endgame §27 — fin RÉCHAUFFEMENT (repli) & Merveille

| Tunable | Défaut | Effet |
|---|---|---|
| `FUEL_FALLBACK_DELAY` | 60 | années après le gate 180 avant que le réchauffement de repli s'arme (laisse sortir les fins naturelles) |
| `FUEL_FALLBACK_MIN` | 4.0 | fuel_ratio mini — un monde calme ET sobre reste sans fin |
| `FUEL_COAL_W` | 3.0 | le charbon pèse ×3 le bois (industrie fossile) |
| `FUEL_MEMORY_HL` | 60 | demi-vie de la mémoire de combustible (le CO2 persiste) |
| `HEAT_RAMP_PER_YEAR` / `HEAT_DROUGHT` | 0.010 / 0.6 | rampe de température · sécheresse (sinon un monde tempéré se réchauffe en jungle habitable) |
| `SEA_RISE_CELLS_PER_YEAR` | 140 | montée des eaux passive (cellules côtières basses noyées/an) |
| `MERV_PHASE_DAYS` | 3650 | durée de chaque palier de la Merveille |
| `MERV_CHARGE_PER_TICK` | 0.5 | charge faustienne ajoutée par tick de chantier |
| `METAB_MERV_RATIO` / `_MIN` | 0.60 / 500 | part digérée de SA communauté par héritage pour la victoire Merveille |

## Tier de province (par POP, source unique)

| Tunable | Défaut | Effet |
|---|---|---|
| `TIER2_POP` … `TIER7_POP` | 2000 / 3000 / 4000 / 5000 / 8000 / 10000 | seuils de pop pour monter une province de tier (⚠ lus une fois, pas de F10 live) |

## Mise en scène — le directeur (drame émergent)

| Tunable | Défaut | Effet |
|---|---|---|
| `DIR_T_HOT` / `_COLD` | 0.50 / 0.32 | fenêtres de température du directeur |
| `AMPL_TRAUMA_CHARGE` / `_HALF` / `_MAX` / `_SCALE` | 180 / 900 / 2000 / 500 | intégrateur de traumatisme (monte aux chocs, redescend au calme) → amplitude dramatique |
| `AMPL_BUDGET_POP` / `_GOLD` / `_CAP` | 0.02 / 0.01 / 400 | budget de mise en scène ∝ pop·richesse·temps |
| `AMPL_OMEN_COST` / `_AMPL` | 60 / 0.35 | un présage coûte des points et ne sort qu'au-dessus d'une amplitude |

---

## Leviers NON-runtime (pas dans `SCPS_TUNE`)

- **Seuils des Âges** (`scps_events.c`, `#define`) : `X_NODES` 4 (Commerce) · `LUM_TOTAL_Y` 25 (Raison) ·
  `EMPIRES_INTEG` 10 (Empires) · `BREACH_CHARGE` 5 (Brèche) · seuils Lumières/Soulèvements/Ordre de Fer
  (savoir+connectivité, masse critique, fracture+déréalisation). Voir aussi les deltas d'effet d'âge
  (`AGE_DELTA_I/L/H/SOLV/MYTH`). Modifiables mais exigent une recompilation.
- **Tables `SCPS_MODS`** (fichier, `chronicle --dump-data`) : `BASE_PRICE[]` / `EXTRACT_YIELD[]` (prix &
  rendements par ressource), `RECIPE[]` (labor & sortie par manufacture, dont la foreuse), `BASE_COST[]`
  (coût de recherche par tier), `NODE_PROD_PCT` / `NODE_EFF_PCT` (bonus de tech), `UNITS[]` (discipline,
  moral, mouvement, commandement par unité).
- **Recettes d'édifices** (`EDIFICES[]`, `scps_agency.c`) : intrants/coûts de chaque bâtiment (arrondis
  aux multiples de 5).
