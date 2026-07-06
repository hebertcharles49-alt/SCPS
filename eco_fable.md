# eco_fable.md — Carnet de raisonnement : audit de câblage & refactor mathématique de l'économie

> **Nature du document.** Carnet de laboratoire **append-only, chronologique** — le fil de pensée de
> l'orchestrateur, pas un rapport. Destiné à un lecteur qui n'était pas là (l'auteur plus tard, ou une
> session ultérieure) : le **pourquoi** de chaque conclusion, pas seulement le quoi. La matrice de
> couplage et le plan de refactor **cristallisent** hors du raisonnement à mesure qu'il mûrit ; rien
> n'y entre sans une preuve **vérifiée** consignée plus haut. Ancrage : `fichier:fonction:symbole`,
> jamais un numéro de ligne.

---

## RÉSUMÉ EXÉCUTIF (distillé, maintenu en tête — dernière mise à jour : entrée [019])

- **État** : **MISSION CLOSE ([019]).** Phases 0-5 CLOSES. Patches landés & poussés : C7 σ · C6 rename ·
  C2/C3/C9 doc · **C5 trou trade×guerre** · **SAVOIR UNIFIÉ** ([016]-[017]) · **VIEWER STRIP** (5768→278) ·
  **DISSOLUTION LaborEcon** (golden-SAFE, save v59 — la levée LIT les strates econ, [019]). Les 4 « reste »
  TRANCHÉS : **C1 REFUTÉ** (PopGroup.L = primitif vivant : coercition + agit_base→révolte + loyauté readout,
  Invariant 2) · **C4** socle governance_P DÉLIBÉRÉ (non-action, le code prévient contre le double-compte) ·
  **§5** IMPLÉMENTÉE ([020], le joueur a tranché le design) : pool commercial MENSUEL qui MORD (0.04/bourgeois
  + 0.01/élite, save v60) · **C3** orphelins RC (chantier de DESIGN, pas un
  bug). determinism STABLE · savetest v59 byte-identique · test 37/37 runnable (3 KO Windows pré-existants) ·
  0 warning. **Zéro dette de correction ouverte** — ce qui reste est du DESIGN à arbitrer, pas de l'audit.
- **Verdicts Phase 3 ([012])** : 2 bugs de câblage (C5 trou trade×guerre · C4 famille 5.f) · 3 dettes
  de doc/nommage (C2 stubs · C6 productivite · C9 faith de jure/de facto) · 1 silo à retirer (C1
  PopGroup.L, +bump) · 1 duplication (C7 σ ×4, unifier vers core) · 1 chantier à part (C3 ~12 canaux
  RC orphelins) · **1 RÉFUTÉ** (C10 : l'Opus conseiller a cru TechState.L gelé — FAUX, `s->L+=n->dL`
  scps_tech.c:461 ; le grep l'a pris — Invariant 2 en action).
- **§5 puissance commerciale — MESURÉ DÉCORATIF ([013])** : volume externe réel 2-7 unités/mois/pays
  vs budget proposé 7-270/mois (10-40× trop grand) → le cap ne mord jamais. Le VRAI commerce inter-pays
  fuit par le trou A1 (étage-2 non gaté, 71-89 % cross-border ; seed 9 : canal gaté MORT). **§5
  reporté APRÈS C5** (fermer le trou d'abord donne un vrai marché à capper), avec re-mesure.
- **Plan Phase 4/5** : PRÉSERVANTS d'abord (C7 σ si sigmoïde bit-identique · C6 rename · C2/C3/C9 doc ·
  collapse tier [002]) → BUMP groupé (C1 + dissolution LaborEcon [005]-[007]) → CHANGEANTS avec
  re-baseline+revue humaine (C5 → §5 → C4 → C3 orphelins).
- **Le suspect P est TRANCHÉ, avec asymétrie ([009])** : `st.K` et `st.H` reçoivent les nudges
  religieux INTACTS ; seul `st.P = governance_P(stub 5.f)` écrase — RC_P meurt pour le canal ORDRE
  (fragilité/SI) mais vit pour PE/babel/surchauffe ; `st.F` = stub JAMAIS retouché ; `Lt` (le
  3e L, TechState) VIT : `croissance_tick = δ·P_realise·(Lt/10)` — c'est LUI que la religion
  pousse. La formule σ du syncrétisme est DUPLIQUÉE TEXTUELLEMENT (culture vs core).
- **Culture vs PopCulture : candidat double-source RAYÉ** (type de passage, une seule source
  vivante). **TODO api n_groups : PÉRIMÉ** (repli mort). **faith : DEUX champs** (int
  institutionnel vs axe continu) à deux écrivains — divergence structurellement possible.
- **Matrice (12 arêtes, [008])** : 7 CÂBLÉES · 2 PARTIELLES (culture→econ à deux portes ;
  factions→savoir INFIRMÉ — le rot ne ronge PAS la recherche, malgré un commentaire d'econ qui le
  suggère) · 2 NON-CÂBLÉES (**A1 le trou guerre/commerce frontalier, CONFIRMÉ EN FLUX** ; A10
  `PopGroup.L` = silo écrit-jamais-lu) · 1 isolation confirmée (labor n'écrit rien dehors).
  **FAMILLE FANTÔME « P/K neutres »** : gate syncrétique ET demography_tick reçoivent des
  littéraux 5.f ; `governance_P/F` = stubs à 5.f. **SUSPECT NEUF ([008])** : dans
  `prosperity_tick`, le P nudgé par la religion (RC_P) semble ÉCRASÉ par `st.P=governance_P(stub)`
  — canal câblé-puis-jeté ? Phase 2 tranche.
- **Gabarit (tier de capitale)** : double source CONFIRMÉE ([002]) ; le savoir lit AUSSI le
  staffing des `LBuilding` — deux termes à collapser, pas un.
- **Dissolution `LaborEcon`** : périmètre désormais COMPLET — 3 lecteurs vivants à repointer
  (savoir · **levée militaire** `campaign_refill`/`army_recruit` [découverte, absente de la spec] ·
  topbar viewer LR_FOOD/pop_in_army) ; `LMarket` est DÉJÀ MORT (supply figé 1.f, guichets or
  supprimés, zéro lecteur de prod) ; LaborEcon est SÉRIALISÉ (section LABO) ⇒ dissolution = bump
  save. Piège latent : labor seedé sur `s->player`, savoir gaté sur `s->human_player` — égalité
  SUPPOSÉE, pas garantie ([006]).
- **Puissance commerciale** : le terrain est cartographié — l'étage 3 (inter-pays) n'a AUCUN cap
  pop-dépendant (TradeRoute.capacity = 1.0f constant) ; le SEUL précédent pop→flux est l'étage 2
  (`link_capacity = 4 + pop·0.002`). Atterrissage sans collision de formule directe, mais
  réconciliation conceptuelle étage 2 ↔ étage 3 toujours obligatoire (deux lois pop→flux).
- **TROU n°1 CONFIRMÉ EN FLUX ([008] A1)** : `trade_tick` déplace stocks + richesse
  (`exp->stock[r]-=vol; imp->stock[r]+=received` ; crédit bourgeois exportateur) sur tout lien
  ADJACENT sans jamais tester owner/guerre/embargo — le commerce frontalier TRAVERSE la guerre.
  Le commentaire d'intertrade (« intra-pays : déjà couvert par scps_trade ») prouve la croyance
  erronée du concepteur : TROU, pas intention. Correctif = patch CHANGEANT (re-baseline).
- **Silos de légitimité** : TROIS « L » (WorldLegitimacy.L[région] · PopGroup.L · TechState.L),
  trois formules/cadences/grains, communication asymétrique — candidat n°1 de l'audit math
  (Phase 3 dira si double-source ou grandeurs légitimement distinctes).
- **Prototypes isolés** : `scps_faith.c` et `scps_popsim.c` ne sont liés QUE dans leurs demos
  (Makefile vérifié) — pas des bugs, mais de la roadmap non réalisée + un risque de confusion.

---

## JOURNAL

### [001] 2026-07-04 — Ouverture : mission, rôles, méthode de preuve, séquencement

**Mission** (spec « Audit de câblage & refactor mathématique de l'économie SCPS ») : trois chantiers
dans l'ordre — (1) auditer le câblage et les influences mutuelles de la chaîne
`labor → culture → religion → econ → stab` (câblé ? mutuel ? mathématiquement cohérent ?) ;
(2) collapser la double source du tier de capitale (le gabarit du patron) ;
(3) introduire la puissance commerciale ET dissoudre totalement `LaborEcon` — **après**
réconciliation avec la machinerie de commerce existante.

**Rôles** (harnais CLAUDE.md) : orchestrateur = cette session (Fable) — tient ce carnet, garde la
séquence, ne lit pas le code au-delà des vérifications mécaniques ; lecteurs/implémenteurs = agents
Sonnet ; vérifications mécaniques = greps du harnais (inline pour 1-3 symboles, agents Haiku pour le
volume) ; conseiller = agent Opus aux cas litigieux (Phase 3+).

**Méthode de preuve (Invariant 2)** : toute citation d'un lecteur est re-grep-ée avant inscription.
Une entrée non vérifiée est rejetée. Le carnet distingue explicitement : **[VÉRIFIÉ]** (grep du
harnais, ce jour), **[RAPPORTÉ]** (affirmation d'un agent, en attente de vérification),
**[CONTEXTE]** (changelog/CLAUDE.md — à re-vérifier avant usage dans la matrice).

**Familles de patch (Invariant 1)** : *préservant-le-comportement* (golden DOIT rester vert, sinon
le patch a un bug) vs *changeant-le-comportement* (golden re-baseliné DÉLIBÉRÉMENT après revue
humaine). Chaque patch déclarera sa famille AVANT d'être écrit. Jamais de re-baseline silencieux.

**Séquencement (la digue)** : matrice bouclée AVANT refactor · gabarit spécifié AVANT puissance
commerciale · dissolution planifiée sur papier AVANT tout patch. Les phases 1-2-3 ne s'ouvrent
qu'à la clôture écrite de la précédente.

**Garde-fou opérationnel** (leçon de la session précédente, hors mission mais vital) : tous les
agents reçoivent l'interdiction EXPLICITE de toucher git (commit/reset/checkout) — un agent de fond
a déjà détruit des commits locaux en « nettoyant » ce qu'il prenait pour un processus intrus.

---

### [002] 2026-07-04 — Le gabarit [VÉRIFIÉ] : tier de capitale, double source econ ↔ labor

Re-vérification mécanique de la trouvaille pré-amorcée (greps du harnais, ce jour). Tout confirme,
et deux faits NOUVEAUX s'ajoutent à la spec.

**Côté production (l'effet éco réel) — [VÉRIFIÉ]** : `scps_econ.c`, bloc capitale du calcul de
productivité de province (fonction hôte à nommer en Phase 0) : recalcule
`capitale_max_tier(rpop)` → `capitale_admin_pop(ctier)` → `capitale_prodmult(ctier, nob)`,
sur la pop régionale VIVANTE, avec un facteur `(1 - rot)` (le « rot » de capture des factions —
couplage factions→econ déjà câblé ici, à porter à la matrice). Échelle pleine 1→7
(`scps_labor.c:capitale_max_tier` ; `labor_demo.c` : `capitale_max_tier(10000)==7`).

**Côté savoir (le revenu de recherche joueur) — [VÉRIFIÉ]** : `scps_sim.c:sim_player_savoir_month`
lit `lab->prov[0].cap_tier` **STOCKÉ** et le **clampe à [1,4]** (`if(ct>4)ct=4`), puis
`m = 0.5·tier`. Appelé du bloc recherche joueur de `sim_day`, multiplié ensuite par
`tech_research_yield × prosp × metab`.

**Fait NOUVEAU n°1 (absent de la spec)** : `sim_player_savoir_month` ne lit pas QUE le tier — il
boucle sur les `LBuilding` de la capitale LaborEcon (`cap->bld[]`, `building_job_slots`,
`jobs_filled`) et ajoute `0.5·tier_bâtiment·staffing` par bâtiment. **Le collapse du gabarit doit
donc statuer sur DEUX termes**, pas un : (a) le tier de capitale → repointer sur
`capitale_max_tier(pop canonique)` ; (b) la contribution des bâtiments LaborEcon → vers quoi ?
(les institutions Savoir passent DÉJÀ par `tech_research_yield` à côté — risque de double-comptage
si on repointe naïvement vers les édifices econ). Question ouverte pour Phase 4.

**Joueur-only — [VÉRIFIÉ]** : `scps_sim.c:sim_init` : `labor_seed_from_world(s->labor, w, s->econ,
s->player)` ; commentaire in situ : « Le LaborEcon reste calé sur s->player (modèle isolé : il ne
nourrit pas l'éco partagée, les capitales agissent via capitale_* en direct) ». Joueur et IA ne
tournent pas la même économie de savoir.

**Désync d'horloge — [VÉRIFIÉ]** : `scps_sim.c:sim_day`, bloc mensuel (`day%30==29`) :
`labor_resync_pop(s->labor, s->econ)` — « labor RELIT la pop (le monde la possède) ». La pop de
LaborEcon a donc jusqu'à ~29 jours de retard sur la pop vivante ⇒ divergence dépendante du jour du
mois. (`labor_tick` est lui QUOTIDIEN — deux horloges dans le même module.)

**Le chemin SAIN de référence — [VÉRIFIÉ]** : `scps_api.c` (lecteur capitale de la façade — nom de
fonction à confirmer en Phase 0) recalcule `capitale_max_tier(pop)` → admin → `prod_pct` : cohérent
avec la production. **Fait NOUVEAU n°2** : le motif sain est déjà MAJORITAIRE — `scps_ai.c` (T-gate
manufactures, 3 sites), `scps_demography.c` (elite_jobs = tier·100), `scps_campaign.c`
(`capitale_defense(capitale_max_tier(pop))`), `scps_revolt.c` (bloc K_CAP) recalculent TOUS depuis
la pop. `scps_econ.h` le déclare même en toutes lettres : « Le tier est DÉRIVÉ de la pop (pas un
champ stocké) — même table que capitale_max_tier ». **Le seul lecteur du tier STOCKÉ restant est
`sim_player_savoir_month`.** Le collapse est donc une mise en conformité du dernier déviant, pas
une migration de masse — ça borne le risque.

**Symptômes du patron cochés** : double propriété (LProvince.cap_tier vs dérivation live) ·
désync de timing (resync mensuel vs live) · incohérence d'échelle (clamp 4 vs 1→7) · périmètre
asymétrique (joueur-only). **Doute à trancher (Phase 4)** : le clamp à 4 est-il un choix d'équilibre
DÉLIBÉRÉ du revenu de savoir (auquel cas le collapse « scale 1→7 » est un patch
*changeant-le-comportement* à re-baseliner en conscience) ou un accident historique ? Ne pas
présumer — chercher trace d'intention (commentaire, commit, AUDIT.md) avant de choisir la famille.

---

### [003] 2026-07-04 — [CONTEXTE] Héritages du changelog à re-vérifier avant usage

Éléments de CLAUDE.md pertinents au chantier, versés au dossier comme CONTEXTE (pas des preuves) :

- **E0.4 « tier payé » ENTERRÉ** (audit v48) : `labor_publish_capitals`/`labor_region_cap_tier` +
  branche lectrice revolt retirés comme byte-identiques — le registre « tier payé » ne divergeait
  déjà plus du repli pop-derived. Cohérent avec le Fait n°2 de [002] : la dissolution continue un
  mouvement déjà entamé.
- **La pop est propriété de la démographie/province** (re-key v47 : `econ->prov[]` = vérité,
  `region[]` = agrégat) ; labor « relit » (E0.1). Le « module de population unique » visé existe
  donc déjà à moitié : c'est l'axe demography+econ.
- **`LMarket`** : marché joueur-only de LaborEcon, redondant face aux trois étages
  (`scps_econ` régional / `scps_trade` inter-régional / `scps_intertrade` inter-pays) — à
  cartographier en Phase 0, y compris `labor_pump_market`/`labor_sell_market` (« les guichets OR »,
  aperçus dans scps_labor.c lors d'une session antérieure).
- **Save v58** : toute dissolution touchant des structs sérialisées (LaborEcon l'est-elle ? à
  vérifier) imposera un bump — à inscrire dans le coût des patches de Phase 5.

---

### [004] 2026-07-04 — Lancement Phase 0 (cartographie) + surprises du premier Glob

Le Glob de confirmation des noms (§4) révèle DEUX écarts au tableau de la spec, avant même de lire
quoi que ce soit :

1. **`scps_faith.c` existe À CÔTÉ de `scps_religion.c`** — la spec disait « + toute unité de foi
   distincte » ; elle existe. Qui possède quoi (pôles/credo/schisme vs quoi d'autre ?) — au
   cartographe.
2. **`scps_popsim.c` existe** — absent du tableau §4 et de tout changelog récent lu. Or la mission
   vise un « module de population unique ». Embryon ? Fossile ? Demo ? À cartographier AVANT de
   dessiner la dissolution (si un module de pop existe déjà, la dissolution atterrit peut-être là).
3. `scps_trade.c` confirmé (l'étage 2 du marché existe bien comme fichier propre).

**Décision** : deux lecteurs Sonnet en parallèle, read-only, git interdit —
- **Lecteur A « modules »** : structs d'état + API publique + inclusions croisées + sites de câblage
  dans `scps_sim.c`/`scps_api.c`, pour labor/culture/heritage/religion/**faith**/econ/legitimacy/
  prosperity/factions/**popsim**/demography. Signale au passage les influences déclarées en
  commentaire mais non câblées (matière pour Phase 2).
- **Lecteur B « marché »** : les QUATRE lieux du marché — solde régional d'econ (prix, caps de
  stock, ipm), `scps_trade` (`TradeLink.capacity` : vérifier la sémantique « croît avec la pop »),
  `scps_intertrade` (caps de route, embargo, Centres, péages, pompe d'armes, actionneur
  `intertrade_market_buy/sell` : gates réels + qui l'appelle), `LMarket` (comportement propre ?
  lecteurs ? joueur-only ?).

Leurs rapports seront vérifiés (greps ciblés) avant inscription — clôture de Phase 0 = une entrée
[00x] consignant la carte VÉRIFIÉE, puis ouverture de Phase 1.

---

### [005] 2026-07-04 — Rapport lecteur B (marché, 4 étages) — inscrit après vérification

Affirmations porteuses re-grep-ées par le harnais : TOUTES CONFIRMÉES. La carte du marché :

**Étage 1 (econ, régional→national)** : le prix est soldé UNE FOIS PAR EMPIRE
(`scps_econ.c:econ_tick`, bloc « PRIX NATIONAL » : `demand_nat/(pool+supply_nat)`, lissage
`PRICE_INERTIA`) puis PROJETÉ sur `re->price` de toutes les provinces ; caps de stock agrégés PAR
PAYS (`ECON_STOCK_CAP_BASE 200` + `500·n_entrepot`) ; `ipm` (interrupteur `SCPS_IPM`). Pas de verbe
joueur direct — la consommation des strates est interne ; `econ_arms_take` délègue à la pompe
(`g_arms_pump` → `intertrade_market_pull`) quand branchée. [RAPPORTÉ, cohérent avec CLAUDE.md]

**Étage 2 (scps_trade, inter-régional)** — [VÉRIFIÉ] :
`scps_trade.c:link_capacity` = `4.f + (pop_ra+pop_rb)*0.002f` (« ~4 unités de base + 2 par 1000
hab ») — **LE précédent pop→flux du moteur**, réellement implémenté, posé à chaque
`trade_network_build`. Second plafond implicite [RAPPORTÉ] : `demand_est = pop_imp*0.01f` dans
`trade_tick`. Cadence **ANNUELLE** [VÉRIFIÉ : `scps_sim.c:sim_day`, `trade_network_build` +
`trade_tick` juste avant `intertrade_tick` dans le bloc annuel] + build à `sim_init`. Sérialisé
(section NETW). VIVANT, mais l'étage le plus lent de la pile.

**⚠ TROU DE CÂBLAGE CANDIDAT n°1** — [VÉRIFIÉ au niveau symboles] : `scps_trade.c` ne contient
AUCUNE occurrence de `owner|diplo|embargo|war` (grep : zéro) ; ses liens naissent de l'ADJACENCE
géographique (terre 4-connexe / bi-côtier / fleuve), qui inclut des paires trans-frontière. Et
`scps_intertrade.c:intertrade_tick` saute `ca==cb` avec le commentaire « intra-pays : déjà couvert
par scps_trade » [VÉRIFIÉ]. Lecture combinée : l'étage 2 commercerait À TRAVERS les frontières SANS
gate guerre/embargo, pendant que l'étage 3 est soigneusement gaté (pair_at_peace + g_embargo,
contournés par pacte). Reste à confirmer EN FLUX (Phase 1) que `trade_tick` déplace bien des biens
entre régions d'owners différents — puis à trancher : intention (« le petit commerce frontalier
survit à la guerre ») ou trou. Hypothèse par défaut : trou (l'étage 3 n'aurait pas ce commentaire
sinon).

**Étage 3 (scps_intertrade, inter-pays)** : `TradeRoute.capacity` posé `1.0f` à la création
(`scps_routes.c`) et JAMAIS réécrit [VÉRIFIÉ] — **aucun cap pop-dépendant à cet étage** ; plafonds
= constantes (`IT_EXPORT_FRAC 0.25`, `ARB_VOL_CAP` tunable) ou bornes de stock/trésor. Embargo à 2
sources (guerre auto via `pair_at_peace` + décrété via `g_embargo`), pacte = laissez-passer.
Centres/hub_map (cache sérialisé v58 — notre fix savetest). Péage de détroit `IT_CHOKE_TOLL 0.12`.
**Actionneur joueur** `intertrade_market_buy/sell` : gates = hub atteignable · Centre-ou-pacte pour
le tier mondial · stock dispo (`avail`) · TRÉSOR (clamp `can=treasury/prix`) ; seuls appelants =
`CMD_MARKET_BUY/SELL` (façade → journal → drain) ; **AUCUNE IA ne l'appelle** [RAPPORTÉ, grep du
lecteur]. La future puissance commerciale atterrirait ICI sans collision de formule directe — mais
la réconciliation conceptuelle avec l'étage 2 (deux lois pop→flux à deux grains) reste à trancher
en Phase 3.

**Étage 4 (LMarket)** — [VÉRIFIÉ] : MORT en production. `market.supply` figé `1.f` (3 sites,
commentaire « P-arc : matériau & or vivent dans le pool éco, plus ici ») ; `market.demand` écrit
NULLE PART hors bancs ; les « guichets OR » `labor_pump_market`/`labor_sell_market` N'EXISTENT PLUS
(seul un commentaire de suppression en témoigne — fantômes morts avant la mission) ; lecteurs =
bancs d'essai uniquement. **MAIS** le rapport B remonte un 3e lecteur VIVANT de LaborEcon que la
spec ne listait pas : **la LEVÉE MILITAIRE** — [VÉRIFIÉ] `scps_sim.c:sim_day` →
`campaign_refill(s->camp, p, s->econ, s->labor)` ; `scps_army.c:army_recruit(ArmyState*,
LaborEcon*, …)` → `prov[0].pop_in_army += count*POP_PER_UNIT`. Le viewer lit aussi
`labor->stock[LR_FOOD]`/`flow` + `pop_in_army` (topbar). **Le périmètre de dissolution est donc :
savoir + levée + topbar viewer** — trois repointages, pas un.

**Tableau des plafonds de débit** (surface de collision §5) : pop-dépendants = `TradeLink.capacity`
et `demand_est` (étage 2 SEULEMENT). Étage 3 : rien de pop-dépendant. Étage 1 : caps de STOCK (pas
de flux). Conclusion pour §5 : pas de double-cap direct à l'étage 3 ; le risque est l'INCOHÉRENCE
inter-étages (une loi pop→flux à l'étage 2, une autre à l'étage 3) plutôt que le double-comptage.

---

### [006] 2026-07-04 — Rapport lecteur A (modules) — inscrit après vérification

Affirmations porteuses re-grep-ées : CONFIRMÉES (détail ci-dessous). Les dix trouvailles du
lecteur, triées par poids pour la mission :

**1. TROIS « L » (légitimité)** — [VÉRIFIÉ] : `scps_legitimacy.h:WorldLegitimacy.L[région]`
(tick annuel, formule align/aisance) · `scps_econ.h:PopGroup.L` « légitimité du groupe envers la
couronne » (muté par `scps_demography.c:group_L_tick`, mensuel, grain GROUPE) ·
`scps_tech.h:TechState.L` « légitimité / ordre consenti » (nudgé par religion via
`scps_prosperity.c:prosperity_tick`, `religion_country_acc->ch[RC_L/RC_STAB/RC_COHESION]`).
Communication ASYMÉTRIQUE : religion LIT wl->L (schisme/fracture), demography ÉCRIT PopGroup.L,
prosperity LIT TechState.L — trois silos, trois cadences, trois grains. **Prudence de méthode** :
trois grandeurs nommées « légitimité » ne font pas automatiquement une double-source — ce peut
être trois concepts légitimement distincts (consentement régional / loyauté de groupe / ordre
abstrait pays). Phase 3 tranchera sur les FORMULES et sur qui consomme quoi.

**2. LaborEcon n'est JAMAIS lu par la façade** — [VÉRIFIÉ] : grep `labor` dans `scps_api.c` →
uniquement `CLASS_LABORER` (strates econ) et `labor_upkeep_per100` (fonction PURE, sans état).
`scps_api.c:scps_province_capitale` recalcule tier/logement/services depuis `ProvinceEconomy` +
`capitale_*` pures. Deux « vérités de capitale » coexistent en RAM (l'état `LProvince` tické et le
recalcul à la volée de l'API) — la façade a déjà choisi le camp du recalcul (le motif sain).

**3. Le piège `s->player` vs `s->human_player`** — [VÉRIFIÉ, avec la nuance exacte] :
`sim_init` : `s->player` = pays POLITY_PLAYER de la genèse ; `s->human_player = -1` par défaut
(« la façade débraye après coup »). Le bloc recherche du savoir gate sur `s->human_player`
(`pl=s->human_player`) mais `labor_seed_from_world(..., s->player)`. Le commentaire de sim.c
(« Le LaborEcon est calé sur s->player (== s->human_player… ») documente une ÉGALITÉ SUPPOSÉE —
aucune garde ne l'impose. Si un humain contrôle un pays ≠ POLITY_PLAYER, savoir et levée lisent le
LaborEcon du MAUVAIS pays, silencieusement. La dissolution PURGE ce piège par construction (plus
de modèle isolé → plus d'hypothèse d'alignement).

**4. `pop_by_class` en TROIS implémentations parallèles** — [RAPPORTÉ, structs citées] :
`LProvince.pop_by_class[LAB_CLASS_COUNT]` (labor, `capitale_mobility_tick`) ·
`PopGroup.pop_by_class[CLASS_COUNT]` (econ/demography, `demography_tick`) ·
`PopBand.by_class[POPCL_COUNT]` (popsim, isolé). Les deux premières tournent SIMULTANÉMENT sans se
synchroniser ; l'API réplique avec un repli `strata[]` et porte un TODO (« pop.n_groups n'y est pas
encore peuplé (câblage moteur à venir) ») possiblement PÉRIMÉ — à vérifier en Phase 2.

**5. `scps_faith.c` et `scps_popsim.c` = prototypes ISOLÉS** — [VÉRIFIÉ Makefile] :
`scps_faith.o` n'apparaît que dans `FAITH_DEMO_OBJS`, `scps_popsim.o` que dans `POP_DEMO_OBJS` —
ni chronicle, ni façade, ni viewer ne les lient. `scps_religion.c` est LE module de foi vivant
(tick quotidien scholar + refresh mensuel). **Correction d'inférence du lecteur** : `pop_demo.exe
« daté du 4 juillet » ne prouve pas une maintenance active — le `make test` du jour (harnais)
rebâtit tous les bancs. L'intention déclarée de popsim.h (« l'intégration (porter ceci dans
PopGroup) vient après ») = ROADMAP écrite, jamais réalisée. Pour la mission : le « module de
population unique » visé a donc DÉJÀ un embryon conceptuel (bande heritage×culture×foi) — à
considérer en Phase 4 comme RÉFÉRENCE de design, pas comme base de code (il est découplé du
moteur).

**6. Propriétaire réel de la pop** : `ProvincePop.groups[].count` dans `WorldEconomy.prov[]`
(muté par demography), agrégé en `strata[]` par econ ; `LProvince.pop` RELIT (E0.1, resync
mensuel), documenté « jamais grandi ici ». Cohérent avec la charte province. La chaîne de
propriété est SAINE — c'est le lecteur savoir qui déviait.

**7. LaborEcon est SÉRIALISÉ** — [VÉRIFIÉ antérieurement, scps_save.c : section `LABO`] : la
dissolution emportera un bump de SAVE_VERSION (« ère antérieure »).

**Divers à porter en Phase 1/2** : `Culture` (scps_culture.h) vs `PopCulture` (scps_econ.h) —
deux types, mêmes 5 axes, conversion à vérifier · `faction_*` sans tick propre (calcul à la
demande + `faction_levers_decay` annuel) · ordre annuel vérifié : `legitimacy_tick` →
trade/intertrade → prosperity_tick → endgame (cohérent, L frais pour prosperity).

---

### [007] 2026-07-04 — CLÔTURE PHASE 0 · ouverture Phase 1

**La carte est établie et vérifiée sur tous les points porteurs.** Ce qui change le plan par
rapport à la spec initiale :

1. **Le périmètre de dissolution s'élargit** : la spec listait marché/savoir/nourriture ; la
   carte ajoute la LEVÉE MILITAIRE (`campaign_refill`/`army_recruit`) et la topbar viewer, et
   confirme que `LMarket` est un cadavre (retrait trivial). Le bump save est acquis (section LABO).
2. **La puissance commerciale a un terrain PLUS PROPRE que craint** : aucun cap pop-dépendant à
   l'étage 3 — le risque n'est pas le double-comptage direct mais l'incohérence de LOI entre
   étages (link_capacity à l'étage 2). La question §5 se reformule : une seule loi pop→flux pour
   les deux étages, ou deux lois assumées à deux grains ?
3. **Un trou de câblage candidat prioritaire est apparu** (guerre/embargo ↔ étage 2) — il passe
   en tête de la Phase 1.
4. **Les trois L** sont le premier chantier de la Phase 3 (cohérence math).

**Phase 1 lancée** : un lecteur Sonnet, liste d'arêtes explicite (chaîne principale + couplages
croisés §4 + le trou trade/guerre), consigne = preuve `fichier:fonction:symbole` par arête,
verdict CÂBLÉ / NON-CÂBLÉ / PARTIEL, grandeur exacte lue + cadence. Le lecteur reçoit ce carnet
comme contexte (il le LIT, il n'y écrit pas — seul l'orchestrateur écrit ici).

---

### [008] 2026-07-04 — LA MATRICE (Phase 1) — inscrite après re-vérification (7 greps, 7 confirmations)

| Arête | Verdict | Grandeur & site | Cadence |
|---|---|---|---|
| A1 trade×guerre | **NON-CÂBLÉ = TROU** | `trade_tick` : `exp->stock[r]→imp->stock[r]` + richesse bourgeoise, AUCUN test owner/diplo (zéro symbole, zéro include diplo) [VÉRIFIÉ flux] | annuel |
| A2 culture→econ | **PARTIEL (2 portes)** | `.demographie` → `scps_econ.c` (natalité, `culture_build_for`+`build_leviers`) ; éthos → tolérance fiscale & préférences ; MAIS `.productivite` (+capacite/coercition/permeabilite/arcane/fracture) → SEULEMENT `prosperity_tick` (`heritage_prod` → `P_realise`) [VÉRIFIÉ] | quotidien / annuel |
| A3 culture→religion | CÂBLÉ | credo → `ai_derive_weights:w_faith` (fondation) ; 4 axes PopCulture + `wl->L[r]` → `region_faith_drifts` (schisme DÉRIVE/fracture) | à la demande |
| A4 religion→econ | CÂBLÉ **DIRECT** | `region_set_native_faith` MUTE `PopGroup.faith` (3 voies : scholar quotidien, héritage fondation, fracture) ; `RC_POPGROWTH` lu par la natalité de `scps_econ.c` [VÉRIFIÉ] | quotidien |
| A5 religion→stab | CÂBLÉ | `prosperity_tick` : RC_K→K, RC_P→P, RC_H→H, **RC_L+RC_STAB+RC_COHESION SOMMÉS dans `Lt`** [VÉRIFIÉ] ; revolt : `FAITH_UNREST 0.22` sur foi dissidente non stabilisée | annuel / à la demande |
| A6 econ→stab | CÂBLÉ | `legitimacy_tick` lit culture/satisfaction/coercion/H_coerc/build.faith ; `prosperity_tick` lit profil, pression fiscale, densités K_inst/P_open/PE_infra/route_pe/charges | annuel |
| A7 stab→econ (retour) | CÂBLÉ | revolt ÉCRIT satisfaction/coercion/treasury/pop des provinces ; mobilisation retire les bras (`revolt_mobilized`), démobilisation les rend | à la demande |
| A8 factions→econ/savoir | **PARTIEL** | econ : `(1-faction_capture_total)` sur le bonus de capitale [re-VÉRIFIÉ] ; savoir : **RIEN** — `sim_player_savoir_month` et `tech_research_yield` ignorent le rot. ⚠ le commentaire d'econ au bloc rot MENTIONNE « recherche » — candidat déclaré-non-câblé (Phase 2 lit le texte complet) | quotidien |
| A9 labor→econ | isolation CONFIRMÉE | `labor_tick` lu en entier : aucune écriture hors LaborEcon | quotidien |
| A10 demography→stab | **NON-CÂBLÉ (silo)** | `PopGroup.L` écrit par `group_L_tick` (+ revolt le MUTE en issue) ; agrégats `province_L`/`country_L` appelés UNIQUEMENT par demography_demo [VÉRIFIÉ] — écrit, sérialisé, jamais consommé | mensuel |
| A11 intertrade↔diplo | CÂBLÉ 2 sens | aller : `pair_at_peace`+`g_embargo` (pacte = laissez-passer) ; retour : `ai_province_value` (convoitise raw_cap×stress×prix) + `diplo_casus_belli:CB_ECONOMIC` (extraction). Rancune COMMERCIALE : n'existe pas (6 sites d'écriture de rancor, tous guerriers) | à la demande |
| A12 prosperity→culture | CÂBLÉ mais **GÉNÉRIQUE** | `culture_can_syncretize(σ(0.8(P−D∞)+0.35(K−5)))` confirmé ; MAIS l'appelant (`demography_contact_tick` via sim_day) passe des LITTÉRAUX `5.f, 5.f` [VÉRIFIÉ] — le gate tourne sur du neutre, jamais la vraie P/K du pays | annuel |

**Découvertes de la re-vérification (au-delà du rapport du lecteur)** :
1. **SUSPECT « câblé-puis-jeté »** : dans `prosperity_tick`, les nudges religieux modifient les
   locales K/P/H/Lt (bloc RC_*), puis PLUS BAS `st.P = governance_P(w,cid)` (STUB à 5.f) et
   `st.L = Lg (« l'entrée vivante »)`. Si l'ordre est bien nudge-puis-écrasement pour P — le canal
   RC_P serait MORT à l'arrivée ; et où coule `Lt` (vers `ts[cid].L` ?) reste à tracer. C'EST LE
   PREMIER OBJET DE LA PHASE 2 (tracer le flux de variables de prosperity_tick, ligne à ligne).
2. Le commentaire du bloc rot d'econ mentionne « recherche » alors qu'A8 prouve que le rot ne
   touche pas la recherche — fantôme déclaré-non-câblé probable.
3. `demography_tick` AUSSI reçoit `5.f, 5.f` (vu au grep [002]) — la famille des littéraux
   neutres couvre les DEUX ticks démographiques, pas seulement le contact.

**Câblages-fantômes recensés (entrée pour Phase 2)** : `governance_P`/`governance_F` (stubs 5.f,
« tant qu'aucun levier n'est branché ») · littéraux `5.f,5.f` (contact_tick + demography_tick) ·
`HeritageLeviers.productivite` (nom promet prod_mult, ne nourrit que prosperity) · TODO api
n_groups ([006]) · commentaire rot/« recherche ».

**Clôture Phase 1.** Bilan : 7 câblées · 2 partielles · 2 non-câblées (1 trou + 1 silo) ·
1 isolation. Les verdicts SAINS d'apparence restent à passer au crible des 5 symptômes (Phase 3) ;
la Phase 2 s'ouvre sur la mutualité fine et la famille fantôme, avec en tête le suspect
« prosperity_tick écrase ses propres nudges ».

---

### [009] 2026-07-04 — Phase 2 (mutualité + fantômes) — inscrite après re-vérification (6 greps, 6 confirmations)

**Q1 — LE SUSPECT TRANCHÉ, avec une asymétrie que personne n'avait vue.** Flux de variables de
`prosperity_tick` tracé en entier [VÉRIFIÉ sur les sites st.*] :
- `st.K = K` et `st.H = H` : les nudges religieux RC_K/RC_H arrivent INTACTS dans `scps_order`
  (aucun stub gouvernance ne les intercepte) — canaux VIVANTS.
- `st.P = governance_P(w,cid)` : un `=` PUR qui REMPLACE par le stub 5.f — le P nudgé par RC_P
  n'atteint JAMAIS `scps_order` (fragilité/SI/mode)… mais il VIT ailleurs : babel gate (C_pe),
  PE_interne, PE_externe, surchauffe. Verdict : câblé-puis-jeté sur UN canal sur quatre, pas mort.
- `st.F = governance_F(w,cid)` : stub 5.f JAMAIS retouché ensuite (ni heritage ni bâti ni Âges) —
  le canal le plus FIGÉ des sept entrées d'ordre.
- `st.L = Lg` (legitimacy_country, la VRAIE légitimité régionale pop-pondérée) ; la religion
  n'influence st.L qu'en AMONT (via legitimacy_tick), pas ici. Le repli `Lg = … : Lt` (si wl NULL)
  est INATTEIGNABLE en jeu réel (sim_day passe toujours s->wl).
- **`Lt` (TechState.L, le 3e L) VIT** : `cp->croissance_tick = DELTA · P_realise · (Lt/10)`
  [VÉRIFIÉ] — la formule « croissance = δ·P·L/10 » du changelog utilise le L TECH nudgé par la
  religion (RC_L+RC_STAB+RC_COHESION sommés), PAS la légitimité régionale. Insight structurant
  pour C1 (les trois L) : chaque L a en fait UN consommateur distinct — wl->L → st.L (ordre),
  TechState.L → croissance, PopGroup.L → personne (silo).
- Non-cumulatif : K/P/H/Lt sont des copies locales, `ts_c` reste const — les nudges religieux se
  recalculent chaque tick, aucun empilement.
- `RC_I` n'est appliqué NULLE PART dans prosperity_tick (RC_F/RC_PE : statut à établir en Phase 3
  — canaux religieux potentiellement orphelins).

**Q2 — rot/« recherche » : fantôme déclaré CONFIRMÉ** [VÉRIFIÉ] : le commentaire §C3 d'econ dit
en toutes lettres « moins de productivité de capitale, moins de recherche » ; le code n'applique
`rot` qu'à `cap_bonus`. Le commentaire PROMET un effet non câblé.

**Q3 — faith : DEUX champs, DEUX écrivains, DEUX vérités** [VÉRIFIÉ] :
`religion.region_set_native_faith` écrit `groups[i].faith` (int institutionnel, id du registre) ;
`demography.faith_convert_tick` écrit `origin.religion` (axe float continu) + bascule
`rel_branch`/`credo` au seuil. Le culte dominant (`religion_refresh_region`) ne lit QUE l'int.
Un groupe peut donc avoir un axe doctrinal 100 % aligné au trône et porter encore l'int d'une
AUTRE foi (et inversement) — divergence structurellement possible, fréquence non quantifiée.
Pas un double-écrivain du MÊME champ, mais deux modèles de conversion parallèles à réconcilier.

**Q4 — la famille fantôme, périmètre complet** [VÉRIFIÉ sites] : gates lisant P/K =
`assimilation_years/tick`, `province_composition`, `demography_tick` (K seul enrichi de
`build.K_inst` — P reste NU), `demography_contact_tick`, `sync_gate`. Appels à littéraux `5.f` :
sim_day ×2 (demography_tick mensuel + contact_tick annuel), **scps_api ×1 + viewer ×2** (readout
composition — deux sites de plus que [008]). `governance_P/F` : aucun autre appelant (statics
locales). **La vraie coordonnée EXISTE** — prosperity calcule K/P nudgés chaque tick (cp->K est
même exposé) — jamais transmise : la démographie recalcule sa porte sur des constantes. **ET la
formule σ est DUPLIQUÉE TEXTUELLEMENT** : `scps_culture.c:sync_gate` vs
`scps_core.c:scps_metabolisation` (`0.8(P−D∞)+0.35(K−5)` mot pour mot) [VÉRIFIÉ] — deux
implémentations de la même loi, risque de dérive de maintenance.

**Q5 — TODO api PÉRIMÉ** : `demography_attach` (genèse, settled) et `colonize_seed_pop_group`
(TOUTES les colonisations en partie) posent `n_groups=1` — le repli `strata[]` de
`scps_province_classes` est mort en pratique ; le commentaire ment. Angle mort mineur signalé :
dépeuplement total post-cataclysme (non vérifié).

**Q6 — Culture vs PopCulture : candidat double-source RAYÉ.** `Culture` est un type de PASSAGE
(copie champ-à-champ à la genèse, worldgen ; aller-retour `pc_to_culture`/`culture_to_pc` au
syncrétisme) — jamais stocké côté monde. UNE source vivante : PopCulture.

**Q7 — route_pe** [VÉRIFIÉ] : producteur = `scps_routes.c` (province-owned : RAZ puis
`+= t->yield` aux deux bouts, la région n'est qu'un Σ-agrégat). Cadence du tick routes à
confirmer si la Phase 3 en a besoin.

**Q8 — mutualités restantes** : stab→labor = RIEN (grep legitimacy/revolt/factions : zéro
écriture vers LaborEcon) — la dissolution n'a pas d'écrivain caché à débrancher.
`assimilation_tick` = vase clos culturel (seul fil quasi-éco : K_inst via demography_tick).

**Q9 — PopGroup.L est sérialisé** (blob ECON, memcpy brut de WorldEconomy) — état
mort-mais-sérialisé, à peser en Phase 4 (le retirer = bump ; le câbler = re-baseline).

**Q10 — sweep de dettes** : AUCUN autre TODO structurel dans les 8 fichiers — les vrais fantômes
sont SILENCIEUX (constantes sans commentaire). Leçon de méthode consignée : le grep textuel ne
trouve pas les fantômes, seule la lecture de FLUX les révèle.

---

### [010] 2026-07-04 — CLÔTURE PHASE 2 · ouverture Phase 3 (le crible + la mesure)

**Le dossier de la Phase 3** — les cas au crible des 5 symptômes, numérotés pour le conseiller :

- **C1 — les trois L** : chaque L a UN consommateur distinct (wl->L→ordre · TechState.L→croissance
  · PopGroup.L→personne). Double-source ou trois grandeurs légitimes mal nommées ? Verdict + sort
  du silo PopGroup.L (retirer/câbler).
- **C2 — les stubs d'ordre** : st.P écrasé (asymétrie vs K/H), st.F figé. Que brancher — et le
  stub P fait-il double emploi avec `lev.permeabilite` additionné juste après ?
- **C3 — le collapse RC** : RC_L+RC_STAB+RC_COHESION sommés dans Lt (perte de distinction voulue ?)
  + canaux RC_I/RC_F/RC_PE : appliqués QUELQUE PART ou orphelins ?
- **C4 — la famille 5.f** : brancher la vraie K/P du pays dans l'assimilation/le syncrétisme ?
  Analyse de SIGNE (un pays prospère assimilerait plus vite → boucle) et d'échelle.
- **C5 — le trou A1** (trade×guerre) : familles de correctif (lien intra-pays seulement ·
  gate paix · malus de guerre) — coût/risque de chacune.
- **C6 — `.productivite` deux-portes** : le levier nommé « productivité » ne touche pas prod_mult
  régional — renommer, re-router, ou documenter ?
- **C7 — la formule σ dupliquée** (culture vs core) : unifier vers core ?
- **C8 — §5 puissance commerciale** : remplace/subsume/coexiste vs link_capacity (étage 2) ;
  LE CAP MORD-IL (mesure, pas devinette) ; signe de la boucle bourgeois→puissance→grain→bourgeois.
- **C9 — faith deux-vérités** (int institutionnel vs axe continu) : réconciliation conceptuelle.

**Répartition** : C1-C7 + C9 → le CONSEILLER (Opus, read-only, verdicts math + directives de patch
avec famille golden). C8-mesure → un MESUREUR Sonnet en **WORKTREE ISOLÉ** (la mission autorise
l'instrumentation en Phase 3 « à instrumenter, pas deviner » ; le worktree jetable garde le tronc
VIERGE — leçon des agents destructeurs) : compteurs diag éphémères dans intertrade/trade, runs
chronicle seeds 7/9/11, livrables = volume externe mensuel/pays en UNITÉS vs le cap 200 ·
pop bourgeoise typique/pays (→ puissance typique) · volume étage 2 TRANS-FRONTIÈRE (quantifie
le trou A1 au passage).

---

### [011] 2026-07-04 — SUSPENSION (limite de session) · consignes de REPRISE

**État au gel** : Phases 0-2 CLOSES et VÉRIFIÉES ([002]-[009]). Phase 3 OUVERTE mais NON
exécutée : le conseiller Opus est mort au lancement (« session limit, reset 19h10 Europe/Paris »),
le mesureur C8 n'a jamais été lancé. AUCUN patch écrit, AUCUN golden touché, le tronc est VIERGE
— la digue a tenu de bout en bout.

**POUR LA SESSION SUIVANTE (reprise en 3 gestes)** :
1. Lire ce carnet en entier (il est self-contained — ne re-cartographier RIEN).
2. Relancer le CONSEILLER Opus sur le dossier C1-C7+C9+C10 de l'entrée [010] (le brief complet
   est dans le prompt de l'orchestrateur : verdicts 5-symptômes + nature + directive + famille
   golden + risque, ancrés fichier:fonction:symbole, re-vérifiés par grep avant inscription).
3. Lancer le MESUREUR Sonnet en WORKTREE ISOLÉ pour C8 (brief en [010] : compteurs diag
   éphémères intertrade/trade, seeds 7/9/11 × 100 ans, unités/mois/pays vs cap 200, pop
   bourgeoise, volume étage-2 trans-frontière). JAMAIS d'agent avec droits git sur le tronc
   (leçon [001] : un agent a déjà détruit des commits).
Puis : clôture Phase 3 au carnet → Phase 4 (synthèse : collapse tier [002] · dissolution
LaborEcon [005]-[007] · verdicts C1-C9) → Phase 5 (patches sous discipline golden, Invariant 1).

**Rappels d'invariants pour la reprise** : preuve vérifiée avant inscription (Invariant 2) ·
familles de patch déclarées, jamais de re-baseline silencieux (Invariant 1) · une population,
une définition (Invariant 3) · patch minimal (Invariant 5) · séquence = la digue (§7).

---

### [012] 2026-07-05 — REPRISE (Opus) · Phase 3 conseiller (C1-C10) — inscrit après re-vérif (4 greps, 1 RÉFUTATION)

Conseiller Opus relancé après reset de session. Verdicts, avec ma re-vérification mécanique :

- **C1 les trois L** — PAS double-source : 3 grandeurs distinctes (wl->L→ordre · TechState.L→croissance
  · PopGroup.L→silo). PopGroup.L co-écrit avec `agit_base` (revolt/econ/demography), mais c'est
  `agit_base` qui a l'aval, pas L. **Directive** : RETIRER le champ `PopGroup.L` (isoler `agit_base`
  d'abord). **PRÉSERVANT + bump save** (champ sérialisé, blob ECON).
- **C2 stubs d'ordre** — `governance_P/F` = socle neutre 5.f délibéré. [VÉRIFIÉ] `bP += build.P_open`
  (prosperity.c:328) est DÉJÀ sommé dans st.P (336) ⇒ ne PAS re-router governance_P vers P_open
  (double-compte). **DOCUMENTER** (le vrai producteur = une loi nationale d'ouverture, inexistante).
  **PRÉSERVANT**.
- **C3 collapse RC** — Lt = RC_L+RC_STAB+RC_COHESION sommés (proxy assumé). [VÉRIFIÉ] SEULS 7 canaux
  RC lus en prod (RC_K/P/H/L/STAB/COHESION en prosperity:235-238 + RC_POPGROWTH en econ:2372) — le
  reste de l'enum (~12) est ÉCRIT-jamais-lu. **DOCUMENTER le collapse ; les orphelins = chantier à
  part** (câbler un canal = CHANGEANT, un par un).
- **C4 famille 5.f** — bug de câblage assumé. Boucle POSITIVE (prospère→assimile+vite→homogénéise),
  échelle NON-triviale (le 5 est le neutre ; vraie K bouge la porte). [VÉRIFIÉ] `cp->K` exposé
  (prosperity.c:364) mais `cp->P` NON. **Directive** : exposer `cp->P`, élargir signatures
  demography_tick/contact_tick pour lire `wp->country[cid].K/P`. **CHANGEANT**. ⚠ vérifier l'ordre
  tick (prospérité annuelle vs démographie mensuelle — cp->K frais ?).
- **C5 trou A1** — bug. **Recommande gate paix au tick** (famille ii : tester diplo_status dans
  trade_tick, comme l'étage 3 — cohérence inter-étages) ; repli = lien intra-pays à la construction
  (famille i, borne le trou mais coupe aussi le commerce frontalier pacifique). **CHANGEANT**.
- **C6 .productivite deux-portes** — nourrit prosperity (heritage_prod→P_realise), jamais prod_mult
  régional : dette de NOMMAGE. **RENAME** `HeritageLeviers.productivite`→`rendement` (15 sites),
  PAS re-route. **PRÉSERVANT** (⚠ ne pas réordonner le struct — `scps_api.c` expose v[9]).
- **C7 σ dupliquée** — [VÉRIFIÉ] QUATRE exemplaires : scps_core.c:57 (canon, `scps_sigmoid`),
  culture.c:364 (`sync_gate`), prosperity.c a un `sigmoid` LOCAL (ligne 45) dupliqué. Include core
  acyclique (VÉRIFIÉ : culture/core n'ont pas d'arête). **Unifier vers `scps_metabolisation`.**
  **PRÉSERVANT SSI** le `sigmoid` local (prosperity:45) est bit-identique à `scps_sigmoid`
  (`1/(1+expf(-x))`) — 1 grep à faire en ouverture Phase 4 ; sinon bascule CHANGEANT.
- **C9 faith deux-vérités** — 2 concepts distincts (int institutionnel de jure / axe continu de
  facto), pas un double-écrivain. **DOCUMENTER la dualité, ne pas fusionner.** **PRÉSERVANT**.
- **C10 (le sien) — ⚠ RÉFUTÉ par ma vérif.** Le conseiller a affirmé `TechState.L` GELÉ à 3.0
  (seul writer = init scps_tech.c:304, croissance = 0.3·δ·P). **FAUX** : grep montre `s->L += n->dL`
  (scps_tech.c:461, à l'unlock d'un nœud) ET `s->L += sn->dL` (:291, diffusion) — **L EST muté par
  la recherche** (les nœuds portant un delta dL montent l'ordre). « croissance = δ·P·L/10 » n'est
  PAS gelée. C10 TOMBE. (dEco/dMil/dF restent morts — mais dK/dL vivent.) → Leçon Invariant 2 :
  même l'Opus conseiller a halluciné « un seul writer » ; le grep l'a pris. **Rien à patcher pour
  C10.**

**Ordre de priorité (risque croissant), familles déclarées** :
- PRÉSERVANT (golden vert) : C7 σ (si sigmoïde bit-identique) · C6 rename · C2/C3-collapse/C9 doc.
- PRÉSERVANT + BUMP save (grouper) : C1 retrait PopGroup.L.
- CHANGEANT (re-baseline, revue humaine) : C5 gate guerre → §5 puissance commerciale (après C5,
  cf. [013]) → C4 vraie K/P → C3 orphelins (chantier à part).

---

### [013] 2026-07-05 — Phase 3 · MESURE C8 (le cap mord-il ?) — orchestrateur, worktree isolé jetable

⚠ **Déviation de délégation assumée** : le worktree-harness a échoué (git Windows « unsafe location ») ;
les sous-agents (Git-Bash) ne pilotent pas le toolchain MSYS2 ; C8 étant le pivot empirique de §5,
l'orchestrateur l'a mesuré lui-même dans un worktree MANUEL (`git worktree add`, démonté ensuite —
tronc resté vierge). Compteurs éphémères getenv `SCPS_C8DIAG` : intertrade via `g_expt/g_imp`
existants (dump fin d'`intertrade_tick`) ; trade cross-border via `g_c8_xb/g_c8_tot` (`trade_tick`,
`exp->owner!=imp->owner`). Jetés avec le worktree.

**Résultats** (seeds 7/9/11, 1 sim × 100 ans, an-80) :
| | seed 7 | seed 9 | seed 11 |
|---|---|---|---|
| ext inter-pays (unités/mois/pays) | méd 3.4 · max 5.6 | **0 (canal mort)** | méd 2.6 · max 6.8 |
| bourgeois/pays | méd 67 · max 2198 | — | méd 377 · max 2677 |
| puissance proposée /mois (10/100) | méd 6.7 · max 220 | — | méd 37.7 · max 268 |
| trou A1 (étage-2 cross-border) | **71.5 %** | **83.0 %** | **88.7 %** |

**VERDICT C8** : le cap 200/mois — et même le budget de puissance par pays — **NE MORD JAMAIS** sur le
marché externe tel que speccé. Le volume réel (2-7 unités/mois) est **10-40× SOUS** le budget
(7-270/mois) ⇒ la puissance commerciale appliquée à l'intertrade serait **DÉCORATIVE**. Pour mordre à
l'externe, il faudrait ~0.3-1 par 100 bourgeois (pas 10). **MAIS** le vrai commerce inter-pays coule
par le **TROU A1** (étage-2 non gaté : 5-6× plus de volume que l'intertrade gaté, 71-89 % cross-border ;
seed 9 : intertrade MORT, tout fuit par l'étage-2). **Conséquence §5** : (1) capper un marché quasi-mort
est vain ; (2) **C5 et §5 sont INTRIQUÉS** — fermer le trou A1 D'ABORD donne à la puissance commerciale
un vrai marché à capper (le commerce basculera étage-2→intertrade gaté). **§5 est donc REPORTÉ après
C5, avec re-mesure du volume externe une fois le trou fermé.** La forme finale (remplace/subsume/
coexiste vs link_capacity) se décidera sur ces chiffres re-mesurés, pas sur la spec à l'aveugle.

---

### [014] 2026-07-05 — CLÔTURE PHASE 3 · dossier Phase 4

Phase 3 CLOSE. Tronc VIERGE (worktree démonté + `git status` propre confirmé). Les 5 symptômes ont
été passés sur les 12 arêtes ; l'audit tient. Bilan des cas : **2 bugs de câblage** (C5 trou A1 · C4
famille 5.f) · **3 dettes de nommage/doc** (C2 stubs · C6 productivite · C9 faith) · **1 silo à
retirer** (C1 PopGroup.L) · **1 dette de duplication** (C7 σ ×4) · **1 chantier à part** (C3 ~12
canaux RC orphelins) · **1 réfuté** (C10). §5 : mesuré décoratif, reporté après C5.

**Dossier Phase 4 (synthèse — NE PAS encore patcher)** :
1. **2 vérifs d'ouverture** (greps ciblés) : sigmoïde bit-exact (C7, décide sa famille) + ordre tick
   prospérité/démographie (C4).
2. Reprendre le **gabarit [002]** : spécifier le collapse tier savoir (2 termes : tier→pop canonique
   + le terme LBuilding, sans double-compter tech_research_yield) — PRÉSERVANT visé.
3. Spécifier la **dissolution LaborEcon** ([005]-[007]) : 3 lecteurs vivants à repointer (savoir ·
   levée `campaign_refill`/`army_recruit` · topbar viewer) + LMarket mort (retrait trivial) — BUMP
   save (LABO), à GROUPER avec C1 (un seul bump).
4. Ordonner tous les patches par famille (Invariant 1), PRÉSERVANTS avant CHANGEANTS.

**Phase 5** : patches minimaux, `make` + golden APRÈS CHAQUE patch (PRÉSERVANT cassé = bug→rejet ;
CHANGEANT cassé = re-baseline après revue HUMAINE, jamais silencieux). §5 puissance commerciale
seulement après C5 + re-mesure.

---

### [015] 2026-07-05 — Phase 4 · vérifs d'ouverture (les 2 incertitudes du conseiller LEVÉES)

Deux greps ciblés (Invariant 2) résolvent les deux INCERTAIN de [012] :

- **C7 sigmoïde bit-exact — CONFIRMÉ PRÉSERVANT** : `scps_prosperity.c` `sigmoid(x)` = `1.f/(1.f+expf(-x))`
  — **IDENTIQUE au byte** à `scps_core.c:scps_sigmoid`. Donc : culler le helper local `sigmoid` de
  prosperity au profit de `scps_sigmoid`, et router `sync_gate` (culture) → `scps_metabolisation`
  (même assemblage d'arguments `0.8(P−D∞)+0.35(K−5)`) = **byte-identique**. C7 est le patch le plus
  sûr du lot, famille PRÉSERVANT FERMÉE.
- **C4 ordre de tick — PROBLÈME CONFIRMÉ (plus INCERTAIN)** : `demography_tick` = MENSUEL
  (`scps_sim.c:sim_day`, day%30==29) ; `prosperity_tick` = ANNUEL (day%365==364) ; et
  `demography_contact_tick` (annuel) tourne AVANT `prosperity_tick` dans le MÊME bloc annuel. Les
  CONSOMMATEURS de K/P (démographie) tournent donc AVANT le PRODUCTEUR (prospérité). Brancher la vraie
  K/P (C4) exige soit REORDONNER (prosperity_tick avant contact_tick + exposer cp->K/P persistant pour
  le tick mensuel), soit ASSUMER un lag (≤1 an / ≤11 mois). **Décision de design pour l'humain** — le
  reorder touche l'ordre canonique du tick annuel (risque de re-baseline en cascade au-delà de C4).

**Phase 4 : ce qui reste (synthèse) + ce qui appelle une DÉCISION HUMAINE** (consultation, cf. l'éthos
de la mission — présenter, ne pas trancher unilatéralement les choix porteurs) :
1. **Collapse tier savoir** ([002]) — le terme TIER se repointe proprement sur `capitale_max_tier(pop
   canonique)` (le motif sain, [002] fait n°2). ⚠ mais DEUX décisions : (a) le clamp 4 vs l'échelle
   pleine 1→7 — équilibre du revenu de savoir : PRÉSERVER le clamp (patch pur) ou l'ouvrir (CHANGEANT
   assumé) ? (b) le terme LBuilding (staffing des bâtiments capitale LaborEcon) : le repointer sur les
   édifices Savoir d'econ RISQUE un double-compte avec `tech_research_yield` — à trancher (retirer ce
   terme, ou le mapper avec soustraction du yield).
2. **Dissolution LaborEcon** — table de repoint : savoir→province-capitale econ · levée
   (`campaign_refill`/`army_recruit` prennent `LaborEcon*`)→pool pop econ (où loge `pop_in_army` ?
   champ à créer côté econ OU réutiliser un mobilisé existant) · topbar viewer (LR_FOOD/pop_in_army)→
   food econ + mobilisé · LMarket→retrait pur. **BUMP save** (section LABO retirée) à GROUPER avec le
   retrait de `PopGroup.L` (C1) = UN seul bump. Décision humaine : timing du bump (maintenant vs
   différer), et si la levée garde sa granularité capitale-only ou passe multi-province.
3. **Ordre des patches** : les PRÉSERVANTS (C7 · C6 rename · C2/C3/C9 doc) peuvent tomber sans
   consultation (golden reste vert, vérifiable). Les BUMPS et CHANGEANTS attendent le GO humain.

⇒ **Checkpoint naturel** : le carnet porte tout le nécessaire ; la suite (rédaction des specs de patch
+ exécution Phase 5) demande des arbitrages humains listés ci-dessus. Présenter et attendre.

---

### [016] 2026-07-05 — Phase 5 EN COURS · patches landés (GO humain « fais tout, commit au fur et à mesure »)

Discipline Invariant 1 tenue : chaque patch déclare sa famille, golden vérifié après chacun.

- **C7** (`f429865`, PRÉSERVANT) : σ unifiée vers `scps_core` (sync_gate→scps_metabolisation ;
  sigmoid local prosperity→scps_sigmoid) — 4 impl → 1. golden IDENTIQUE. ⚠ ripple lien : culture.o
  appelle scps_metabolisation ⇒ `scps_core.o` ajouté à 6 listes _OBJS de bancs (Makefile).
- **C6** (`af703d8`, PRÉSERVANT) : `HeritageLeviers.productivite`→`rendement` (18 sites, rename pur).
  golden IDENTIQUE.
- **C2/C3/C9** (`67ef50b`, PRÉSERVANT/doc) : stubs governance_P/F = socle neutre ; collapse Lt +
  ~12 canaux RC orphelins ; faith de jure(int)≠de facto(axe) — DOCUMENTÉS. golden IDENTIQUE.
- **C5** (`b448706`, CHANGEANT · re-baseline) : le commerce étage-2 ne franchit plus une frontière
  EN GUERRE (le trou A1, chiffré 71-89 % cross-border par C8). **Design DÉCOUPLÉ** : `link_blocked[]`
  pré-calculé par sim.c (qui a la diplo), passé à trade_tick — scps_trade reste FEUILLE (zéro ripple
  diplo sur 7 bancs, zéro bump). golden RE-BASELINÉ (5 graines), determinism STABLE, sweep SAIN
  (satisfaction 73/79/89, commerce recouvré 100 % hubs an-62). ⚠ suite : embargo DÉCRÉTÉ à l'étage 2
  (intertrade_embargoed statique — non gaté ici, seule la GUERRE l'est).

**RESTE (le gros — big-refactor + DÉCISIONS d'ÉQUILIBRE qui relèvent de l'humain)** :
- **Collapse tier savoir** ([002]) : repointer `sim_player_savoir_month` sur `capitale_max_tier(pop
  canonique)`. ⚠ 2 décisions : (a) préserver le clamp 4 (PRÉSERVANT) vs ouvrir l'échelle 1→7
  (CHANGEANT, change le revenu de savoir) ; (b) le terme LBuilding — le retirer ou le mapper sans
  double-compter `tech_research_yield`.
- **Dissolution LaborEcon** ([005]-[007]) : 3 lecteurs (savoir · levée `campaign_refill`/
  `army_recruit` · topbar viewer) + LMarket mort + **BUMP save** (section LABO) à GROUPER avec le
  retrait de `PopGroup.L` (C1) = UN bump.
- **§5 puissance commerciale** : RE-MESURER le volume externe MAINTENANT que C5 ferme le trou (le
  commerce va basculer étage-2→intertrade gaté) ; puis trancher remplace/subsume/coexiste.
- **C4** (vraie K/P) : exposer `cp->P` + reorder tick prospérité↔démographie. **C3 orphelins** :
  chantier à part (un canal RC à la fois).

Ces restes portent des arbitrages d'équilibre (clamp savoir, forme §5, timing du bump) = revue
humaine (Invariant 1). Le carnet porte le plan complet ; à mener au prochain push.

---

### [017] 2026-07-05 — SAVOIR UNIFIÉ (le bug-gabarit d'origine CORRIGÉ, par REFONTE) — CHANGEANT · commit 459c317

Décision humaine sur le collapse tier ([002]) : PAS un simple repointage, une REFONTE sur le patron
« puissance commerciale » (la pop produit, l'édifice module). `econ_country_savoir(econ,cid)` = Σ
region (0.01·élite + 0.005·bourgeois + 0.001·journalier /an) × (1 + bonus BIBLIOTHÈQUE : Σ build.savoir
·PER, plafond 33 %). **UNE source, joueur ET IA** : le joueur (sim.c ÷365 × yield × prosp × metab) et
l'IA (`ai_research_step`, remplace `rate·(1+pop/popref)`) lisent la même assiette ; `ai_research_income`
(bandeau) DRY dessus ; `sim_player_savoir_month` SUPPRIMÉE (le savoir ne lit plus LaborEcon).

Corrige les 6 axes du bug ([savoir]) d'un coup : joueur-only → tous pays · stale mensuel → strates
vives · clamp 4 → pleine échelle pop · piège player≠human → dissous · double-compte LBuilding → bonus
% bibliothèque · tier stocké → dérivé live. MESURE (diag éphémère an-80, seeds 7/9/11, façon C8) :
savoir médian 14-16 pts/an, max 56-97 (gros empires) ; seed 9 = monde HÉGÉMON (médian 0 — la formule
reflète FIDÈLEMENT la pop écrasée) ; pace 250 ans **50 %/empire ≈ historique** ; santé satisfaction
75-94 %, hégémon mortel 3/3. Poids du user retenus tels quels (pace saine, 0 recalibrage).
RE-BASELINE golden · determinism STABLE · 0 banc cassé · viewer 0 warning. Tunables : SAVOIR_W_ELITE
0.01 · _BOURGEOIS 0.005 · _LABORER 0.001 · SAVOIR_LIB_PER 0.067 · _MAX 0.33.

⊕ Le **collapse tier savoir ([002], le bug d'ORIGINE de la mission) est RÉSOLU** — et c'est un pas
de plus vers la dissolution LaborEcon (le savoir en est débranché ; restent la levée militaire +
la topbar viewer). RESTE encore : dissolution complète (+bump, C1) · §5 (re-mesure post-C5) · C4 ·
C3 orphelins.

---

### [018] 2026-07-05 — CARTE DE DISSOLUTION LaborEcon (lecteur a761dde9, points porteurs re-VÉRIFIÉS) — refactor MAJEUR spécifié, NON exécuté

Le périmètre réel est BIEN plus lourd que la spec anticipait — 3 claims porteurs re-grep-és, tous CONFIRMÉS :
- **DEUX pipelines de levée** (pas un) : `campaign_refill` (JOUEUR, sur `s->labor`) + `warhost.scratch`
  — [VÉRIFIÉ] `h->scratch = calloc(1, sizeof(LaborEcon))`, re-seedé par pays (`seed_scratch`→
  `labor_seed_from_world`), c'est la levée IA. `army_recruit(a, LaborEcon*, …)` (campaign) et
  `army_recruit(a, sc, …)` (warhost) — à RÉÉCRIRE (signature → WorldEconomy) ⇒ propage aux DEUX.
- **AUCUN champ pop-mobilisée côté econ** — [VÉRIFIÉ] `ProvinceEconomy` n'a que `mil_stock` (DÉRIVÉ
  warhost, pas un pool) ⇒ il faut **CRÉER `pop_in_army`** (champ SÉRIALISÉ → bump). `revolt.mobilized`
  est un système DISTINCT (rébellion), à ne pas réutiliser.
- **6 fonctions PURES `capitale_*`** (max_tier/status/defense/admin_pop/housing/prodmult) utilisées
  PARTOUT (ai/econ/demography/campaign/revolt/api/viewer) ⇒ **DOIVENT survivre** : migrer vers
  `scps_capitale.h`/.c (ou econ) + 7 `#include` à re-router.
- **C1 (`PopGroup.L`)** : dépendance `L → agit_base` = DISCIPLINE de code (chaque écriture de `L`
  republie `agit_base` via `agit_from_L`) ; **4 sites d'écriture de L dans revolt.c NON vérifiés**
  (republient-ils agit_base ?) = prérequis BLOQUANT. Lecteur prod unique de L =
  `revolt.province_apply_coercion` (test `L>=seuil`) → migrer sur agit_base.
- **Mort trivial** : LMarket + ~15 fonctions labor (colonize/taxes/prosperity_index/…) sans appelant
  prod ; `labor_demo` à SUPPRIMER, `army_demo` à RÉÉCRIRE, `warhost_demo`/`audit_eco` NON audités.
- **⚠ RISQUE SAVE TRIPLE, un SEUL bump 58→59** : retrait section LABO + retrait `PopGroup.L`
  (`sizeof(WorldEconomy)` change, `fwrite` BRUT → le padding se réarrange) + ajout `pop_in_army`.
  `--savetest` byte-identique OBLIGATOIRE (le byte-identity durement gagné, cf. saga v57/v58).

Plan ordonné complet (Phases A repointages → B retrait mort → C module+bump → D bancs → E savetest)
dans le rapport de l'agent. **VERDICT ORCHESTRATEUR** : c'est un refactor MAJEUR (~50 edits, 2 pipelines
de levée réécrits, format de save triple-change) = **session DÉDIÉE**, PAS la queue d'une session
déjà énorme (un arrêt mi-chemin = build CASSÉ + risque sur le save). Et c'est du **NETTOYAGE**
architectural — le bug FONCTIONNEL du savoir est DÉJÀ corrigé ([017]) — donc l'urgence est moindre
qu'elle ne l'était. À lancer avec budget frais + savetest rigoureux à chaque bump.

---

### [019] 2026-07-05 — CLÔTURE DE LA MISSION : dissolution EXÉCUTÉE (golden-SAFE) + les 4 « reste » TRANCHÉS

Session dédiée lancée (budget frais, cf. [018]). Deux gros morceaux LANDÉS + poussés, puis les 4 items
« reste » tranchés — plusieurs par RÉFUTATION : l'audit conclut qu'il n'y a **plus de bug à corriger**.

**VIEWER STRIP (commit d558a71)** — `viewer.c` 5768 → 278 lignes. Front interactif = Godot ; ne restent
que les 6 outils CLI headless (savetest/fuzztest/dump-lang/readout/fnv/lang-audit) + sim_rebuild + les
wrappers de save partagée. `#include <SDL.h>` gardé pour la SEULE compat d'entrée MinGW (-Dmain=SDL_main) ;
aucun appel SDL ; lien Makefile INCHANGÉ. 0 warning, savetest byte-identique. Retire au passage le 3e
lecteur de LaborEcon (topbar LR_FOOD/pop_in_army) → allège la dissolution.

**DISSOLUTION LaborEcon (commit 08e745d)** — EXÉCUTÉE, et — SURPRISE — **golden-SAFE (PAS de re-baseline)**,
contre l'attente [018]. Le plan est suivi mais SIMPLIFIÉ par trois découvertes :
1. **La levée est un ADAPTATEUR, pas un consommateur.** Les deux pipelines (campaign_refill JOUEUR +
   warhost seed_scratch IA) ne faisaient que SEMER un LaborEcon transitoire depuis econ (labor_seed_from_world)
   pour que `class_free` lise `pop_by_class`. On repointe `army_recruit/can_recruit/class_free` sur
   `(const WorldEconomy*, int cid)` → lecture DIRECTE des strates econ (Σ region.strata[cl].pop). L'adaptateur
   (s->labor + h->scratch) ÉVAPORE ; `wh_country_elite` remplace seed_scratch.
2. **`pop_in_army` était MORT** — écrit dans LProvince, lu SEULEMENT par la mobilité labor (morte : la
   façade recalcule la capitale depuis econ) + la topbar viewer (retirée). SUPPRIMÉ, pas migré (le [018]
   prévoyait de le CRÉER côté econ — inutile).
3. **Pas de migration capitale_*** — au lieu de déplacer les 6 pures vers un module neuf (ripple Makefile +
   cascade de link — la fausse-piste de début de session), on SLIM `scps_labor.{c,h}` : elles RESTENT dans
   labor.o (lié partout) ; tout l'état (struct/LProvince/LMarket/tick/mobility) part.
GOLDEN IDENTIQUE : dans la fenêtre 12 ans les gates de recrutement passent IDENTIQUEMENT (strates econ
réelles vs semis labor 80/15/5 — les pools sont larges, le gate ne mord pas différemment) ET pop_in_army
était mort ⇒ **PRÉSERVANT, pas CHANGEANT** (l'inverse de la prévision). SAVE : section LABO retirée →
SAVE_VERSION 58→59 ; WarHost.scratch retiré (pointeur runtime, save-neutre). Vérifié : determinism STABLE ·
savetest v59 byte-identique (2/2 × 9/11/42) · test 37/37 runnable (3 KO Windows pré-existants, crashs
rc=127 à 0 assertion) · sweep 9×3×200 SAIN (armée 44→91, 20 guerres/sim, satisfaction 74/83/90 %, hégémon
mortel, IPM 1.02).

**LES 4 « RESTE » TRANCHÉS** :
- **C1 (retrait `PopGroup.L`) — REFUTÉ (Invariant 2, 3e prise après C10/TechState.L).** L'A10
  « écrit-jamais-lu » est FAUX. Grep : `PopGroup.L` pilote (a) le GATE DE COERCITION (`demography.c` :
  `g->L >= COERCE_L_THRESH` → on ne réprime que le restif), (b) `agit_base = agit_from_L(g->L)` → révolte,
  (c) le READOUT `r->loyaute = band_humeur(g->L)`, (d) des agrégats pop-pondérés, (e) les issues de révolte.
  C'est un PRIMITIF vivant (loyauté 0-10), pas un silo ; agit_base en DÉRIVE (« migrer sur agit_base » est
  impossible — c'est la dérivée). Les « trois L » sont TROIS concepts distincts, tous vivants (consentement
  régional wl->L · loyauté de groupe PopGroup.L · ordre abstrait TechState.L). **PAS de retrait, PAS de bump.**
- **C4 (`governance_P` stub 5.f écrase RC_P) — DÉLIBÉRÉ, pas un bug (le CODE le documente).**
  `prosperity.c:131-134` : « SOCLE NEUTRE DÉLIBÉRÉ, pas un oubli. Ne PAS re-router governance_P vers
  re->build.P_open — l'ouverture BÂTIE est DÉJÀ sommée dans st.P (bP) ». Re-router double-compterait. RC_P
  VIT via le `P` local pour PE/babel/surchauffe ; seul le canal ORDRE prend le socle neutre, faute de
  PRODUCTEUR (une politique nationale d'ouverture/centralisation — une FEATURE, pas un fix). **NON-ACTION.**
- **§5 (puissance commerciale) — DÉCORATIF, skip assumé.** Mesuré [013] : volume externe réel 2-7/mois vs
  budget de cap 7-270/mois (10-40× trop grand) ⇒ le cap ne mord JAMAIS. Le vrai commerce inter-pays fuyait
  par le trou A1 — corrigé par C5. Le câbler serait un recalibrage d'équilibre (budget sur volume réel),
  pas un bug. **Skip documenté** (mécanisme inerte, dialable si on veut un jour un vrai plafond).
- **C3 (~12 canaux RC orphelins) — chantier de DESIGN, pas un bug.** `prosperity.c:238` : « SEULS 7 canaux
  ch[RC_*] sont LUS en prod ». Les ~12 autres sont COMPUTÉS mais non consommés — roadmap ou calcul mort ;
  décider consommer-ou-retirer par canal exige l'INTENTION de chaque coordonnée religieuse (design), pas un
  correctif. Aucun n'est un bug FONCTIONNEL (des floats non lus, inertes). **Documenté comme chantier.**

**BILAN MISSION.** Les VRAIS défauts trouvés sont TOUS corrigés : le bug-gabarit du savoir ([017]), le trou
trade×guerre C5 ([016]), la dissolution LaborEcon (ici). Les restants se révèlent, à l'inspection rigoureuse,
des NON-bugs (C1 primitif vivant · C4 socle délibéré) ou des DÉCISIONS/design (§5 décoratif · C3 orphelins).
L'audit de câblage & le refactor mathématique sont **CLOS** : la chaîne labor→culture→religion→econ→stab est
cartographiée, ses double-sources tranchées (gabarit collapsé, savoir unifié, LaborEcon dissous), ses trous
bouchés (C5), ses faux-positifs réfutés (C1, C4, C10). Zéro dette de correction ouverte ; ce qui reste
(§5 recalibrage · C3 wiring · C4 policy-feature) est du DESIGN à arbitrer, pas de l'audit à finir.

---

### [020] 2026-07-05 — §5 PUISSANCE COMMERCIALE IMPLÉMENTÉE (le joueur a tranché le design)

[019] fermait §5 « décoratif, skip » (le câbler = une DÉCISION de design). Le joueur l'a PRISE : la
puissance commerciale = le VOLUME de bien échangeable au marché, par empire et par mois — un POOL LIBRE
que les achats DRAINENT (fin du « rafler tout le stock sans contrepartie »).

Design (patron du savoir, choisi par le joueur) : `econ_country_commerce = 0.04·bourgeois + 0.01·élite`,
× le bonus de la CHAÎNE COMMERCIALE (Σ `build.PE_infra` — les 6 édifices commerciaux : marché/entrepôt/
comptoir/banque/port marchand/centre). Pool MENSUEL fixe (recalculé au roulement de mois par sim_day),
scalé sur la pop marchande. Le poids 0.04 (baissé du 0.10 mesuré décoratif en [013]) MORD : 38-73 achats
bornés/sim. Gate sur l'IMPORT de `intertrade_market_consume` (chantiers) + `intertrade_market_buy` (achat
manuel), joueur ET IA ; stages propre+empire GRATUITS. Alimente `diplo_eco_power` (la bourgeoisie marchande
pèse en menace/alliance/score de guerre). Membrane : menu marché (readout pool/restant + hover explicatif)
via `scps_commerce_power` → binding → `sidebar_drawer`.

⚠ RE-BASELINE golden (le cap mord dès l'an-0) · SAVE 59→60 (ITRD sérialise budget+spent, état intra-mois) ·
flag `g_commerce_active` : inactif hors sim ⇒ bancs INCHANGÉS. Mesuré seed 9 : pool 80-289/mois (petits
empires ~19 — l'échelle « 18/20 par mois » demandée —, gros ~546), monde SAIN (satisfaction 77/87/89 %,
hégémon mortel, IPM 1.17). Commits `8ee31de` (moteur) + `56a3b00` (membrane + UI). §5 passe de
« décoratif » à ÉQUILIBRAGE ACTIF — le seul des 4 « reste » que le design a rouvert et tranché.

## 2026-07-06 — ENDGAME UNIFIÉ : la Brèche (une barre, quatre visages), la Merveille-victoire (métabolisation 3/4/6)

**Vague 0 (3 éclaireurs read-only) — l'écart existant/cible :**
- Le §27 EST déjà « un seuil, des visages » : `endgame_tick` tire à `entropy ≥ ENTROPY_FIN` (gate an 180),
  et la sélection par rare dominant `faust_consumed[3]` (scps_endgame.c:542-555 : essence→EAU, flux→RONCES,
  fer→FROID) implémente DÉJÀ « le visage = la tech consommée » — chaque tech faustienne consomme SON rare
  (foreuse→essence, réplicateur→flux, Corne=TECH_FORGE_RUNES→fer). AUCUN compteur par-tech à créer.
- `entropy` ≠ `dereal` ≠ `breach_pressure` : trois compteurs distincts (prosperity/order/ages). Le doc disait
  « dereal seuil unique » — c'était une confusion de nom, le seuil réel est l'entropie.
- La fin SANG n'existe pas : Campaign.dead_choc/dead_pursuit existent par campagne mais aucune agrégation
  monde, aucune pop de référence an-0 sérialisée.
- La Merveille : paliers FORGE/SOCIÉTÉ/SAVOIR × rares, victoire = arbre COMPLET + assimilation TOTALE
  (integration ≥0.99 sur tout le monde possédé). La métabolisation (econ_country_metabolized,
  _by_heritage) est LISIBLE mais n'est PAS une condition.
- Factions : API complète (opposition table hardcodée, grief AUTO dans lever_apply, coup_tension_c,
  rot plafonné 0.85) mais l'endgame n'en lit RIEN ; Communautaire n'a AUCUN hook d'event (le peuple ne
  vote jamais) ; le Conseil n'a ni faction ni loyauté (greffe ~4 Ko dans Statecraft, modèle de dérive =
  COERCION_DECAY 0.93).
- Godot : le terrain MUTE déjà physiquement (rebiome/sink/thorns relus par le shader) ; le récap d'âge
  (pause+bilan+verbe) est le squelette exact du page-turn ; lavis par-province = OPTION SHADER seulement
  (512k cellules, le vectoriel est rejeté) ; la barre de métabolisation par héritage est DÉJÀ dans le
  panneau tech (heritage_access()).

**DÉCISIONS (joueur, 2026-07-06) :**
1. LA BARRE D'ENTROPIE DÉCLENCHE LA BRÈCHE, QUELLE QUE SOIT SA NATURE — une barre, plusieurs
   nourritures (charge tech faustienne [existant], transmuteurs [existant], breach_pressure des Âges
   [à câbler], LES MORTS DE GUERRE [à créer]) ; le visage au tir = la signature dominante
   (essence/flux/fer/sang). FIN_SANG appendue à l'enum (valeurs existantes stables).
2. LA MERVEILLE EST LA FORME DE VICTOIRE — paliers gatés MÉTABOLISATION : 3 cultures (palier 1) /
   4 (palier 2) / 6 (palier 3). Les anciennes conditions (assimilation totale + arbre complet)
   TOMBENT — la thèse SCPS (le contact métabolisé) rendue jouable. « Culture métabolisée » = héritage
   à accès tier 3 (digéré OU natif — la machinerie de la triade).
3. ENDGAME JOUEUR : pas de probe headless lourde (difficile et contreproductive) — bancs moteur
   (endgame_demo) seulement, le flux Godot se vérifie à la main.
4. UI : popup/bouton dans l'ARBRE DE TECH « métabolisation de X prête » au franchissement du tier
   (lecture heritage_access existante, GDScript pur).
5. UNIFIER : pas de mécanique parallèle — le Sang n'est pas un second seuil, c'est une entrée de
   l'entropie + un visage.

**Plan de vagues :** V1a moteur (Sang + unification entropie + Merveille-métabolisation + réactions
factions + readers d'intensité par région, SAVE v67) ∥ V1b Godot (page qui se tourne sur le récap
d'âge, couleurs de fin + or Merveille, popup métabolisation). V2 : Conseil (faction+loyauté+paie,
SAVE v68) puis événements (trahisons/successions/inter-conseillers R/A/C, étapes Fondation/
Construction/Ascension à 3 choix, hooks Communautaire manquants). V3 : lavis par variante (shader
variant_map + readers façade). Gates par vague : bancs · golden (attendu IDENTIQUE en V1a — rien ne
mord <an-180) · determinism-deep 200 ans (l'endgame vit au-delà du golden) · savetest.

### 2026-07-06 (suite) — V1a LANDÉ : le Sang, la Merveille-métabolisation (corrigée en session),
### les réactions de factions, le lecteur d'intensité. SAVE v67.

**A — LA GUERRE NOURRIT L'ENTROPIE + FIN_SANG.** `EndgameState` gagne `war_dead`/`pop_ref` (double,
sérialisés) + `sang_scar[SCPS_MAX_REG]` (float, PERMANENT). `pop_ref` posé UNE fois par
`endgame_set_pop_ref` (appelé depuis `sim_init` juste après `endgame_init`, le point CANONIQUE — la
1re fois que le monde a une pop réelle ; no-op si déjà posé, donc un reload ne le ré-amorce jamais).
`endgame_entropy_widen` (renommée depuis C1) lit désormais `camp->dead_choc + camp->dead_pursuit`
(Campaign, RAZ par sim via `campaign_init` — pas besoin d'un accumulateur séparé, le compteur EXISTAIT
déjà) → `war_dead`, puis ajoute `ENTROPY_BLOOD_W × (war_dead/pop_ref)` à l'entropie, PLUS
`ENTROPY_BREACH_W × wp->age_breach_flux` (l'entrée des Âges — `age_breach_flux` EXISTAIT déjà, posée
par `scps_events.c`, jamais lue par l'endgame avant ce lot : la vraie « unification » tenait en une
ligne). `FIN_SANG` APPENDUE à `FinType` après `FIN_ASCENSION` (valeurs 0-4 stables, save_sane relevé à
`<= FIN_SANG`). Sélection (décision #1, « une barre, un visage dominant ») : DANS
`endgame_select_and_fire`, APRÈS le gate temporel+entropie mais AVANT le sélecteur par rare dominant —
si `war_dead/pop_ref ≥ ENDGAME_BLOOD_FRAC` (0.20), LE SANG L'EMPORTE inconditionnellement (mission :
« quelle que soit sa nature »), épicentre = la région vivante au `revolt_scar` max (SA PROPRE
géographie, pas forcément le foyer d'entropie). `sang_seed` snapshotte `revolt_scar` (déjà persisté,
déjà pop-pondéré) sur toutes les régions > `SANG_SCAR_MIN`(0.15) → `sang_scar[]` — puis NE DÉCROÎT
JAMAIS (contrairement à `revolt_scar` qui guérit à -0.25/mois) : la plaie de guerre est la thèse de
cette fin. `sang_step` (annuel) draine `SANG_DRAIN_PER_YEAR`(3%)×scar de la pop de chaque région
marquée via `econ_region_pop_add` (le même helper que le reste de l'éco, jamais un accès direct à
`prov[]`), plancher `SANG_POP_FLOOR`(50) — le monde reste FINI, pas de spirale à zéro.

⚠ **MESURE DE CALIBRAGE (rapportée, NON corrigée — la mission l'interdisait explicitement)** : sweep
`./chronicle 42 2 250 6 12` — le ratio `war_dead/pop_ref` atteint **40-43 %** (seed 42) et **123-961 %**
(seed 7, où les morts cumulées DÉPASSENT largement pop_ref — un monde très belligérant sur 250 ans où
les guerres se succèdent), tous deux ≫ `ENDGAME_BLOOD_FRAC=0.20`. Pourtant AUCUNE fin ne s'est
déclenchée sur ces sweeps (`entropie monde` plafonne à 34-41, sous `ENTROPY_FIN=55`) : le SEUIL SANG
lui-même est franchi haut la main, mais l'ENTRÉE D'ENTROPIE qu'il nourrit (`ENTROPY_BLOOD_W × ratio`,
ratio borné [0,1] dans la contribution effective observée puisque le gate de sélection utilise le
ratio brut mais l'entropie n'ajoute que `1.0 × ratio` par TICK, non cumulatif — chaque année réinjecte
le même ratio courant, pas un cumul perpétuel) reste modeste comparée à `ts[].charge` (la charge tech
faustienne, qui elle S'ACCUMULE sans plafond sur des décennies et peut atteindre des dizaines). Le sang
peut donc franchir SON propre seuil de sélection (`ENDGAME_BLOOD_FRAC`) sans que l'entropie MONDIALE
n'ait jamais atteint `ENTROPY_FIN` — le gate d'ENTROPIE (commun aux 4 visages) et le gate de SÉLECTION
SANG (spécifique) sont deux portes INDÉPENDANTES qui doivent TOUTES DEUX s'ouvrir. Sur les sweeps
observés (seed 42/7, 250 ans), c'est la porte ENTROPIE qui reste fermée — PAS la porte sang. Deux
lectures possibles, non tranchées ici (la mission demandait de RAPPORTER, pas de choisir) : (a)
`ENTROPY_BLOOD_W` est sous-calibré pour que le sang PARTICIPE utilement à ouvrir sa propre porte
d'entropie (aujourd'hui il ne fait quasiment que sélectionner, une fois que la porte est DÉJÀ ouverte
par autre chose) ; (b) c'est un comportement VOULU (le sang ne devrait fournir qu'un VISAGE, jamais une
VOIE D'OUVERTURE — la porte reste tech/transmuteurs/Âges, cohérent avec « le savoir/la transgression
ouvre l'Apocalypse, la guerre en décide juste la FORME si elle est assez massive au moment du
franchissement »). Le ratio observé (40-961 %) MONTRE que `ENDGAME_BLOOD_FRAC=0.20` n'est probablement
PAS le goulot — c'est `ENTROPY_FIN` (via les 3 autres nourritures) qui l'est, sur ces graines
spécifiques et cette fenêtre de 250 ans.

**B — LA MERVEILLE-VICTOIRE (métabolisation 3/4/6) — CORRIGÉE EN SESSION (2 passes) par relecture du
joueur.** `endgame_metab_count(w, econ, cid)` (public, header) + `endgame_metab_count_ts` (interne,
avec `TechState`, utilisée par `wonder_tick` pour le gate réel) : compte les héritages « métabolisés »
— natif TOUJOURS +1, puis pour chaque héritage étranger, le MAX de DEUX VOIES INDÉPENDANTES.
  - **1re implémentation (FAUSSE, corrigée avant tout commit)** : j'ai d'abord réutilisé
    `econ_country_heritage_digested` (Temps 2 tech) telle quelle, avec le seuil `METAB_TIER3=0.35`.
    **LE PIÈGE** : cette fonction normalise CHAQUE `dig[h]` sur la POP TOTALE de l'empire
    (`scps_econ.c:568`, `out[r]=dig[r]/tot` où `tot` est commun à tous les héritages) — un choix VOULU
    pour l'accès TECH (« quel % de mon empire est digéré de cet héritage », les héritages se PARTAGENT
    ce total). Appliqué tel quel à la Merveille, ce dénominateur PARTAGÉ rend « 6 héritages ≥ 0.35
    simultanément » MATHÉMATIQUEMENT IMPOSSIBLE (6×0.35 = 210 % > 100 % de la même pop — au plus DEUX
    héritages étrangers peuvent franchir 0.35 EN MÊME TEMPS, quel que soit l'empire). Testé, ça
    échouait exactement comme prédit (`metab_count` plafonnait à natif+2=3, jamais 4 ni 6) —
    contournement transitoire par `ts[].arch_depth` forcé (qui, lui, est un signal PAR-HÉRITAGE
    indépendant, donc non sujet au piège) : les tests passaient mais la voie DIASPORA seule restait
    cassée.
  - **2e implémentation (celle qui reste)** : `endgame_heritage_metabolized(w, econ, cid, h)` — un
    helper qui réplique le PATRON de scan de `econ_country_heritage_digested` (`prov[]` en entier,
    charte règle 1) mais somme `dig_h`/`tot_h` SUR LES SEULS GROUPES DE L'HÉRITAGE h (dénominateur
    PAR-HÉRITAGE, pas partagé) : chaque culture jugée sur SA PROPRE communauté, pas celle des 5 autres.
    Deux gardes : un RATIO (`dig_h/tot_h ≥ METAB_MERV_RATIO`=0.60 — bien digéré RELATIVEMENT à sa
    propre diaspora) ET un PLANCHER D'ÂMES (`dig_h ≥ METAB_MERV_MIN`=500 — pas de « culture
    métabolisée » à 30 personnes noyées dans un grand empire, un ratio pourrait sinon flamber à 1.0 sur
    un groupe minuscule). `metab_diffuse_coeff` (le coefficient par `Arrival` — migrant/soumis/réfugié
    plein, déporté `METAB_DIFFUSE_SLAVE`=0.30, natif 0) est `static` dans `scps_econ.c` (non exporté) :
    RÉPLIQUÉ à l'identique sous `endgame_diffuse_coeff` (même switch, même clé tunable) plutôt que
    dupliqué sous un nom différent ou exposé pour une seule utilisation externe.
  - **VOIE GOUVERNANCE (conservée, orthogonale)** : `ts[cid].arch_depth[h] >= PROF_PROFOND` — la
    profondeur de CONTACT (commerce/gouvernance, cache DÉJÀ posé par `ai_sync_refresh`/
    `tech_sync_tick`, lu ici en LECTURE SEULE via le paramètre `ts[]` déjà présent dans
    `endgame_tick`/`wonder_tick`, SANS lier `scps_ai.o` — `heritage_access_pack` dans `scps_ai.c` fait
    EXACTEMENT ce calcul mais n'est pas exportée et son fichier n'est pas dans `ENDGAME_DEMO_OBJS` ;
    lire directement le champ sérialisé `arch_depth[]` évite d'ajouter une dépendance de lien pour ce
    lot). Cette voie couvre les CONQUIS administrés en profondeur SANS AUCUNE diaspora (un peuple
    soumis sur place, jamais un migrant) — orthogonale à la pop, donc jamais sujette au piège du
    dénominateur partagé.
  - `endgame_metab_count_ts` = pour chaque héritage étranger, `n++` si `contact_deep` (voie a) OU
    `endgame_heritage_metabolized` (voie b) — le MAX des deux voies, exactement le motif de
    `heritage_access_pack` (scps_ai.c) mais REJOUÉ localement pour éviter la dépendance de lien.
  - Gate des paliers dans `wonder_tick` : `req = endgame_metab_required(eg->merv)` (FORGE=3,
    SOCIÉTÉ=4, SAVOIR=6, 0 sinon) ; si `endgame_metab_count_ts(...) < req`, le palier NI NE PROGRESSE
    NI NE CONSOMME sa rare (retour immédiat, testé : la progression ET le stock de rare restent
    STRICTEMENT gelés tant que le compte est insuffisant). VICTOIRE = SAVOIR_DONE bouclé (palier 3) —
    les anciennes conditions `endgame_tree_complete`/`endgame_world_assimilated` sont RETIRÉES du
    verdict (fonctions supprimées, plus aucun appelant) : la thèse SCPS (le contact métabolisé) EST la
    victoire, remplaçant la conquête totale.
  - Tunables ajoutés (registre J) : `METAB_MERV_RATIO`(0.60), `METAB_MERV_MIN`(500). Contrôles
    endgame_demo (C8, 2 nouveaux + garde-fous) : (1) deux diasporas de TAILLES TRÈS DIFFÉRENTES (600 et
    50 000 âmes), toutes deux bien intégrées → LES DEUX comptent (la preuve directe que l'ancienne math
    au dénominateur partagé aurait écrasé la petite sous le poids de la grosse — même empire, même
    `tot` partagé aurait donné `dig_petite/tot ≈ 0.01` malgré une intégration à 100 % DE SA PROPRE
    communauté) ; (2) un empire VIDÉ de toute diaspora (0 groupe étranger, la voie diaspora ne peut RIEN
    compter) mais avec 2 héritages en `arch_depth=PROF_PROFOND` → FORGE avance quand même (natif+2
    contacts = 3, le seuil FORGE) — la voie gouvernance SEULE suffit, sans un seul migrant.

**C — RÉACTIONS DES FACTIONS AU TIR.** `endgame_faction_react(fin, fauteur)`, UN SEUL site par
déclenchement (jamais par tick) : `faction_lever_apply` est le SEUL point d'entrée public des
factions — la grief se propage par la table d'OPPOSITION déjà existante (`faction_opposition`), jamais
un setter direct de grief. Mapping : TOUTE fin avance Transgresseur (`FAC_REACT_UNIVERSAL`=0.15, « ils
l'ont voulue ») → Gardien s'aigrit automatiquement (opposition G↔T=1.0, maximale dans la table) ; SANG
avance EN PLUS Conquérant (0.15) → Communautaire s'aigrit (opposition C↔U=1.0) ; EAU avance EN PLUS
Gardien → Marchand s'aigrit (opposition M↔G=0.9, « les routes noyées ») ; RONCES ne rajoute rien (le
lever Transgresseur universel suffit déjà, T↔U=0.9 dans la table). Appelé aux 4 sites où `eg->fired`
passe à `true` (le MERV_ASCENDED-override, la branche FIN_SANG, le MAP[] final EAU/RONCES/FROID, et la
transition SAVOIR_DONE→ASCENDED dans `wonder_tick`) + un lever `FAC_REACT_START`(0.10, plus léger)
DÈS `endgame_start_wonder` (le simple DÉMARRAGE du chantier avance déjà les Transgresseurs — pas
besoin d'attendre la victoire).

**D — READER D'INTENSITÉ PAR RÉGION.** `endgame_region_intensity(eg, w, econ, region)` → [0,1], PUR
(aucun état muté) : EAU (engloutie=1.0 / programmée=0.6 / adjacente à une engloutie≈0.3, via
`eg->sunken[]` + `econ->adj`) · FROID (`eg->cold_offset`, la rampe globale — pas de lecture par-cellule,
le refroidissement est un phénomène MONDIAL, pas régional) · RONCES (fraction de cellules `BIO_THORNS`
de la région, scan direct — la même vérité que le rendu carte) · SANG (`eg->sang_scar[region]`
directement, la marque EST l'intensité). 0 pour AUCUNE/ASCENSION (rien à teindre) ou hors-bornes.

**E — GATES.** SAVE v66→67 (`EGAM` grandit : `war_dead`/`pop_ref` doubles + `sang_scar[SCPS_MAX_REG]`
floats ; `save_sane` borne `war_dead≥0`/`pop_ref≥0`/`sang_scar[r]∈[0,1]` + relève le plafond `fin` à
`FIN_SANG`). `make endgame_demo`/`chronicle`/`scps` **0 warning**. `endgame_demo` **105/105 vert**
(C0-C8 : les 4 tests V1a d'origine — pop_ref/war_dead/sélection SANG/dépeuplement borné/cicatrice
permanente/intensité — PLUS les 2 tests C8 de la correction — individualisation + voie gouvernance).
`make golden` **IDENTIQUE** (rien ne mord avant l'an-180 : le gate temporel `ENDGAME_YEAR_OPEN` protège
la fenêtre golden 12 ans par construction — vérifié explicitement, pas supposé). `make determinism`
**STABLE** (5 graines × 12 ans, hashes inchangés vs avant V1a). `make determinism-deep` **STABLE** (200
ans × graines 7/9 — la fenêtre où l'endgame VIT réellement). `savetest` (v67) **byte-identique** sur
seeds 9 ET 11 (`scps_viewer --savetest`). `make test` : **37/37 bancs runnable verts** (les 3 KO restent
les PRÉ-EXISTANTS Windows documentés ailleurs — `intertrade_demo` build `setenv`, `campaign_demo`/
`warhost_demo` stack-overflow — confirmés sans rapport avec ce lot). Sweep `./chronicle 9 2 250 6 12`
SAIN (satisfaction 78/86/82 %, 155 guerres, hégémon mortel 2/2, aucune fin déclenchée dans la fenêtre —
attendu, l'entropie ne franchit `ENTROPY_FIN` sur AUCUNE des 4 nourritures dans ces 250 ans).
Sweeps 42/7 : voir le rapport de calibrage SANG ci-dessus (lot A) — ratio sang largement franchi, porte
entropie fermée. **NON COMMITTÉ** (par consigne).

## 2026-07-06 — L'IA NE CONSOMME JAMAIS SES RARES FAUSTIENS (diagnostic + fix doctrine, scps_ai.c seul)

**Symptôme** : `conso foreuse 0 · réplicateur 0 · corne 0` sur TOUTES les graines du sweep — donc la
sélection de fin par « rare dominant » (`faust_consumed[3]`, cf. entrée précédente) ne peut JAMAIS
départager EAU/RONCES/FROID par la voie faustienne ; seule la voie SANG (ou le hash par défaut) tranche.

**PHASE 1 — trace complète (lecture seule) :**

1. **Qui incrémente `faust_consumed[]`** : `scps_econ.c:2358-2359`, dans la boucle de production des
   manufactures (`econ_tick`). SEUL déclencheur : `bld_is_faustian(b->type)` (scps_econ.c:1752-1753 —
   vrai pour `BLD_FOREUSE`/`BLD_REPLICATEUR`/`BLD_CORNE` seulement) ET le bâtiment doit avoir tourné ce
   tick (`lim>0`, staffé + intrant consommé). `k=0` essence/foreuse, `1` flux/réplicateur, `2` fer
   céleste/corne. Agrégé province→région (`econ_aggregate_regions`, scps_econ.c:911) puis région→monde
   (`prosperity_tick`, scps_prosperity.c:186). **Donc le seul maillon qui compte est : la manufacture
   EST-ELLE POSÉE ET NOURRIE ?**

2. **Recettes** (scps_econ.c:191/199/200) : FOREUSE `essence→fer` · RÉPLICATEUR `flux→bois` · CORNE
   `fer céleste→grain`. Les trois sont **tier-5** (`bld_min_tier`, scps_econ.c:1810 — le tier le plus
   haut du jeu) et gatées par tech (`tech_foreuse`/`tech_replicateur`/`tech_corne`, posées par
   `econ_apply_country_tech` depuis `ts->unlocked[TECH_FOREUSE/TECH_TRANSMUTATION/TECH_FORGE_RUNES]`).

3. **La tech S'UNLOCK bien** (côté `scps_ai.c`, déjà en place, PAS le maillon mort) :
   - `ai_research_step` §4 (ligne 2036-2041) : empire affamé de FER épargne pour TECH_FOREUSE.
   - FAU5 (ligne 2042-2057) : empire affamé de BOIS beeline TECH_TRANSMUTATION, affamé de GRAIN beeline
     TECH_FORGE_RUNES — sauf si `tech_crisis_proximity` est déjà haute (prudence).
   - S3 (ligne 2085-2108) : un empire à fort appétit faustien (`ai_faustian_appetite ≥ AI_FAUST_QUEST`)
     beeline aussi TECH_FORGE_RUNES via la Poudrière (l'« emblème »).
   - Donc les 3 tech DEVRAIENT s'unlock chez les empires en famine/faustiens — c'est mesurable et
     documenté (S3/§4/FAU5 dans CLAUDE.md).

4. **LE MAILLON MORT : la POSE du bâtiment, nulle part dans la doctrine IA.**
   - `ai_build_civmanuf` (scps_ai.c:975) **EXCLUT explicitement** les 3 transmuteurs
     (`if (bld_is_faustian(...)) continue;`) — par design (commentaire : « charge/tech », renvoyé à la
     voie doctrinale).
   - `ai_build_manufacture` (scps_ai.c:891, LA voie doctrinale « bâti délibéré, payé, par tempérament »)
     pose bien des bâtiments arcanes pour un empire faustien (`a->w_faustian>0.30f` → ligne 911 :
     `BLD_CELESTIAL_FORGE` + `BLD_MAGE_WORKSHOP`) — **mais ne pose JAMAIS FOREUSE/RÉPLICATEUR/CORNE**.
     C'est le seul point d'entrée pour les bâtiments faustiens dans TOUTE la base IA (grep exhaustif de
     `bld_is_faustian`/`BLD_FOREUSE`/`BLD_REPLICATEUR`/`BLD_CORNE` dans scps_ai.c : zéro autre site).
   - Côté econ (hors ma portée, lu seulement) : `econ_build_tick` (§NF, scps_econ.c:1500-1537, le
     bâtisseur autonome demande-menée) N'EXCLUT PAS ces 3 types de sa boucle générique — en théorie il
     PEUT les poser si `price[out] ≥ 1.8×base` localement ET l'intrant est fournissable quelque part
     dans le royaume. Mais en pratique c'est un cercle : l'ESSENCE n'a AUCUN consommateur sauf la
     FOREUSE (qui n'existe pas encore) → sa demande reste ~0 → son `owner_avail` ne suffit à rien
     déclencher d'autre, et surtout le bien dont §NF regarde le PRIX est `RES_IRON`/`RES_WOOD`/
     `RES_GRAIN` (l'OUTPUT du transmuteur), des biens communs qui franchissent rarement 1.8×base — les
     doctrines civiles (armurerie/scierie/etc.) les couvrent déjà. §NF n'est donc pas conçu pour ce cas
     (aucun bug à proprement parler côté econ — juste un chemin qui ne s'emprunte quasi jamais) : le
     verrou véritable est bien l'ABSENCE de doctrine dédiée, exactement le motif que S1 (greffe) et S3
     (emblème) ont déjà résolu pour la RECHERCHE.

**Conclusion PHASE 1** : le maillon mort est dans `scps_ai.c`, réparable par la doctrine (comme prévu).
Aucune modification hors `scps_ai.c` n'est nécessaire.

**PHASE 2 — fix (scps_ai.c uniquement)** : nouvelle fonction `ai_build_transmuter`, appelée juste après
`ai_build_manufacture` dans le même créneau `t_mil` (probabiliste, pas un tour garanti — même cadence que
l'arsenal). Motif IDENTIQUE à la quête S3/§4 : **empire transgressif** (`a->w_faustian > AI_FAUST_QUEST`,
réutilise le seuil existant — « la soif d'interdit », rare par construction) qui a **déjà la tech**
débloquée POSE (une fois, `empire_has_bld` garde) le transmuteur correspondant, dans la région-hôte au
tier suffisant (T-gate `capitale_max_tier`/`bld_min_tier`, même idiome que `ai_build_manufacture`), payée
au **prix normal** (`MANUF_BUILD_COST × bld_min_tier × ipm`, débit au succès — zéro bonus plat). Gate de
sens : `econ_bld_can_build` (le prédicat public existant, scps_econ.h:887) vérifie que la région a bien
le gisement (fer céleste pour CORNE/FORGE ; les autres n'ont pas de gate raw) — sinon on chercherait une
région-hôte qui ne pourra jamais tourner. Essence/flux : pas de gate raw (produits par les manufactures
arcanes, le pool national les achemine) — seule la présence du bâtiment `BLD_MAGE_WORKSHOP`/`BLD_ALAMBIC`
quelque part dans l'empire (via `empire_has_bld`) est requise pour FOREUSE/RÉPLICATEUR, sinon l'intrant
ne sera jamais nourri (poser une foreuse sans mage workshop la laisserait inerte à vie). Un seul
transmuteur posé par tour (comme les autres doctrines). Tunable neuf, LOCAL à scps_ai.c (`#define`,
documenté ici — pas dans scps_tune_list.h) : aucun nécessaire, réutilise `AI_FAUST_QUEST` (0.80) déjà
présent pour S3 — la même porte d'appétit gate maintenant recherche ET construction du même canon
faustien, cohérent avec l'esprit « rare et coûteux » de S3.

## MISSION « LES HELPERS MANQUANTS » (2026-07-06) — lecteurs pour B2/B3/B5/B6, C2/C3/C4

Contexte : les événements lot 2 §B (culturels) et §C (religieux) avaient sauté B2/B3/B5/B6 et
C2/C3/C4 faute de LECTEURS moteur (« aucun canal inventé », cf. CLAUDE.md « BOUCLE DE GAMEPLAY »).
Mission : livrer ces lecteurs — DÉRIVÉS PURS, zéro état neuf, zéro sérialisation, zéro bump SAVE.
Le contenu (les EVID_* eux-mêmes) viendra dans une vague ultérieure ; ici seulement les getters.

**LIVRÉS (5) :**

1. **`culture_relation_of(...)`** (scps_culture.{h,c}) — relation par instance en CHAMPS NUS
   (14 floats/enums : les 5 axes + credo + branche, ×2). `culture_relation(Culture*,Culture*)`
   existait déjà mais opère sur des fiches `Culture` complètes ; `PopCulture` (scps_econ.h, la fiche
   RÉGIONALE réellement peuplée) partage le MÊME préfixe de champs mais vit dans un module qui
   INCLUT scps_culture.h — impossible d'y ajouter une surcharge `PopCulture*` sans circular include.
   Solution : `culture_relation` ET `culture_relation_of` délèguent maintenant à un cœur PARTAGÉ
   `relation_core` (byte-identique par construction — la discipline anti-duplication du dépôt).
   Le site d'appel futur (scps_events.c, qui voit les deux types) déballe deux `PopCulture*` dans
   ces 14 paramètres. Débloque **B2** (rivalité culturelle voisine) / **B3** (parenté). Banc
   culture_demo +4 (28/28 vert) : `culture_relation_of` ≡ `culture_relation` sur les MÊMES fiches
   (cas normal + cas schisme).

2. **`region_ethos_drift(Ethos local, Ethos ruling)`** (scps_culture.{h,c}) — distance [0..1] entre
   l'éthos DOMINANT d'une région et l'éthos RÉGNANT (capitale du pays), normalisée sur l'étendue de
   l'ancre VALEURS (ETHOS_VAL, span 1.5..9.0). ⚠ SIGNATURE réduite à deux `Ethos` (pas de World/région
   en paramètre) : la culture régnante ne se lit QUE via `econ_ruling_culture` (scps_econ.h, FICHIER
   INTERDIT à cette mission) — le site d'appel fournit les deux `Ethos` déjà déballés d'un
   `PopCulture*` région et d'un `econ_ruling_culture(...)`. Débloque **B6** (dérive d'éthos). Banc
   culture_demo +3 : même éthos que la couronne → 0 ; dominateur/pacifiste → ≈1 (proche du max) ;
   borné [0..1].

3. **`religion_fracture_level(w, econ, cid)`** (scps_religion.{h,c}) — part POP-PONDÉRÉE des régions
   du pays dont le culte DOMINANT (`religion_of_region`) ≠ la foi d'État (`religion_of_country`).
   [0..1], 0 = athée ou uniforme. Bâti sur les briques P8 existantes (aucun état neuf). Débloque
   **C2** (décret tolérance).

4. **`religion_credo_drift(w, econ, cid)`** (scps_religion.{h,c}) — ALIAS documenté de
   `religion_fracture_level` : l'audit du module religion ne trouve qu'UN SEUL signal dérivable
   honnêtement pour « dérive de crédo/pratique vs foi professée » sans inventer un canal — même
   porte, même agrégat, même unité que la fracture. Débloque **C4**.

5. **`religion_scholar_drift(cid)`** (scps_religion.{h,c}) — {0,1} : le lettré ACTIF porte-t-il une
   face PÉRIMÉE (≠ celle qu'exige `scholar_role_from_credo` du crédo d'État COURANT) ? Ce que ça NE
   dit PAS, documenté en tête : le module ne stocke AUCUNE ancienneté de recrutement — seulement le
   décompte de la mission courante (`timer`) — donc pas de notion honnête de « lettré inactif depuis
   N années ». Débloque **C3**.

Banc religion_demo +13 (fixture : `world_generate`+`econ_init`+`gen_population` graine 9, un pays à
2 régions, groupes PopGroup natifs posés à la main sur les provinces représentatives — `gen_population`
ne peuple pas systématiquement toutes les régions d'un pays à 2 provinces, d'où l'attache manuelle) :
athée→0 (les 3 lecteurs) · foi uniforme→0 · schisme (r1 bascule) → fracture≈0.5, alias identique,
borné · lettré recruté aligné→drift=0 · pays schismant sa foi d'État→drift=1.

**SAUTÉ (1), documenté :**

6. **`statecraft_creuset_state`** — ÉVALUÉ, pas créé : le signal existe déjà en DEUX formes publiques
   distinctes, toutes deux au-delà de la simple composition attendue. Niveau PAYS :
   `econ_country_metabolized(w, econ, cid)` (scps_econ.h) est le TWIN-INVERSE exact de
   `econ_off_culture_fraction` — « part des âmes d'un autre héritage que la capitale ET assimilées »,
   déjà consommé par la recherche (Temps 1) et la membrane. Niveau RÉGION : `trig_xenophobe`
   (scps_events.c:289-305) inline déjà le motif « off_culture ≥ seuil + intégration pop-pondérée des
   minorités > seuil » pour EVID_XENOPHOBE — 6 lignes, pas un état, réutilisables telles quelles par
   un futur trigger B5. Créer `statecraft_creuset_state` serait un DOUBLON déguisé du twin-inverse
   déjà public + de l'inline déjà démontré. Rien à livrer ici ; le futur site d'appel de B5 doit
   composer `econ_country_metabolized` (creuset national) ou reprendre l'inline de `trig_xenophobe`
   (creuset régional), selon le grain visé par l'événement.

Gates : `make religion_demo culture_demo statecraft_demo` → 3 bancs verts, 0 warning (religion_demo
13/13 nouveaux + selftest OK ; culture_demo 28/28 ; statecraft_demo 45/45, intact — non touché).
Tous les lecteurs livrés sont des DÉRIVÉS PURS (aucun `static` neuf, aucune écriture, aucun champ
sérialisé) → golden/determinism intacts par construction (non relancés ici — arbre partagé, cf.
consigne de la mission). Fichiers modifiés : `scps/scps_culture.h`, `scps/scps_culture.c`,
`scps/culture_demo.c`, `scps/scps_religion.h`, `scps/scps_religion.c`, `scps/religion_demo.c`,
`eco_fable.md` (ce paragraphe). Aucun fichier interdit touché ; `scps_statecraft.{h,c}` lu mais
non modifié (le reader #6 s'y serait logé, mais est sauté).

**PHASE 3 — mesure.**

**Itération 1 (ratée, corrigée)** : le premier jet gatait la pose sur `a->w_faustian > 0.30f` (le MÊME
seuil que `BLD_CELESTIAL_FORGE`/`BLD_MAGE_WORKSHOP` dans `ai_build_manufacture`). Diagnostic instrumenté
(`SCPS_TRANSDIAG`, retiré avant de rendre) : sur seed 9/100 ans, `a->w_faustian` **max mesuré = 0.22**,
jamais > 0.30 — le poids est une base FIXE 0.2 (scps_ai.c ligne ~147, `a->w_faustian = 0.2f`) modulée par
`glide_axis` (le glissement de faction FAC_TRANSGRESSEUR, borné [0.3,2.0]×0.2 = max théorique ~0.4, mais
qui exige que la faction Transgresseurs SURREPRÉSENTE la pop vs le trône — rare). Or §4 (famine de FER)
et FAU5 (famine BOIS/GRAIN) — les DEUX chemins qui déverrouillent FOREUSE/TRANSMUTATION/FORGE_RUNES dans
`ai_research_step` — **n'exigent AUCUN appétit faustien** : ce sont de PURES famines (`ai_resource_famine`)
tempérées par `tech_crisis_proximity`. Seul S3 (l'emblème) gate sur `ai_faustian_appetite` (credo×valeurs,
un axe CULTUREL, pas `a->w_faustian`). Résultat : la porte de POSE (w_faustian>0.30, quasi jamais franchie)
était PLUS STRICTE que la porte de RECHERCHE qui avait déjà validé la tech — incohérent, et ça explique le
0 mesuré même après le premier fix.

**Itération 2 (retenue)** : la porte de pose est retirée — on POSE dès que la tech est débloquée (le signal
déjà validé en amont par §4/FAU5/S3), en AUTO-SUFFISANCE : si l'usine-source manque (mage/alambic pour
nourrir essence/flux), on la pose D'ABORD (sinon le transmuteur resterait inerte à vie). Une pose par tour.

**Faux golden ÉCHEC (piège du chantier partagé, PAS mon bug)** : un premier `make golden` a montré les 5
hash CHANGÉS — mais `scps/golden_hashes.txt` était alors VIDE (un autre agent en cours de re-baseline sur
`scps_econ.c`/`scps_intertrade.c`, l'esclavage `g_slave_pool`). Isolé en désactivant TEMPORAIREMENT l'appel
à `ai_build_transmuter` (commenté, function-defined-but-unused, warning attendu) : le hash 12-ans était
IDENTIQUE avec ou sans mon fix (859148e9/3ab72541/9941c716/7993b668/80e1555b) — la divergence venait des
fichiers économie/intertrade d'un AUTRE agent, pas de scps_ai.c. Une fois `golden_hashes.txt` re-publié
par cet agent, `make golden` passe PROPREMENT avec mon fix actif. Leçon suivie à la lettre : ne jamais
« réparer » le fichier d'autrui, attendre et re-vérifier.

**Résultats finaux** (chronicle rebuild propre, 0 warning) :
- `make golden` **OK** (hash IDENTIQUE, 5 graines × 12 ans) — les poses n'arrivent PAS avant l'an 12 (la
  tech FOREUSE/TRANSMUTATION/FORGE_RUNES prend des décennies : coût √N-provinces + épargne + tier-3/4/5).
- `make determinism` **STABLE** (5 graines × 12 ans, hashes reproductibles).
- Sweep seeds 9/7/42 × 250 ans (`./chronicle <seed> 1 250 6 12`) :
  - seed 9 : `conso foreuse 0 · réplicateur 0 · corne 2946` · satisfaction Laborer 65% / Bourgeois 83% /
    Élite 79% · hégémon mortel 1/1 · entropie 9319 [TERMINAL].
  - seed 7 : `corne 1262`, satisfaction 79/89/90 %, hégémon mortel 1/1, entropie 7791 [TERMINAL].
  - seed 42 : `corne 729`, satisfaction 69/86/84 %, hégémon mortel 1/1, entropie 3184 (sous TERMINAL=4000).
  - **FOREUSE et RÉPLICATEUR restent à 0** sur ces 3 graines — MAIS ce n'est plus un maillon mort côté
    pose : instrumentation (`SCPS_TRANSDIAG`, retirée) confirme que `tech_foreuse`/`tech_replicateur` ne
    deviennent JAMAIS vrais pour aucun empire sur ces graines/250 ans (alors que `tech_corne` s'allume
    2 fois, cid 60 et 34) — le maillon mort qui RESTE est en amont, côté RECHERCHE (§4/FAU5), hors de mon
    diagnostic initial qui les supposait à tort déjà fiables. FORGE_RUNES a DEUX portes de recherche
    (FAU5 famine-grain ET S3 emblème) contre UNE seule chacune pour FOREUSE (§4 famine-fer) et
    TRANSMUTATION (FAU5 famine-bois) — doublant ses chances de s'unlock dans la fenêtre. Le fix de pose
    (Phase 2) est VALIDÉ et TOURNE dès que la tech existe (la preuve : Corne). Un lot séparé (hors mandat
    de cette session : « la tech débloquée » était un GIVEN de la consigne) pourrait creuser pourquoi
    §4/FAU5 s'unlock si rarement pour foreuse/transmutation — non entrepris ici (hors scope explicite).
- Bar demandée (« satisfaction ≥65, hégémon mortel ») : **tenue** sur les 3 graines (Laborer 65 % pile
  à la limite sur seed 9 — le plus chargé en entropie/apocalypse — mais ne la franchit pas).
- Le monde N'entre PAS en spirale de fins répétées : `endgame_select_and_fire` latch `eg->fired` UNE
  fois (`if (eg->fired) return;`, scps_endgame.c:740) — l'entropie continue de monter après (Σ
  faust_charge, jamais gelée par le firing) mais ça ne redéclenche pas une 2e fin ; c'est l'aftermath
  attendu (les mondes avec CORNE actif tôt vivent longtemps sous cataclysme, cohérent avec le design
  « la Brèche a un prix »).

**Ce qui reste hors de ma portée (documenté, pas touché)** :
- `scps_econ.c` (§NF, `econ_build_tick`) pourrait en théorie poser ces bâtiments aussi (aucune exclusion
  de type) mais le cercle essence/prix-commun le rend quasi inerte en pratique — non modifié (fichier
  hors scope).
- La rareté de recherche de §4 (famine-fer→FOREUSE) et FAU5 (famine-bois→TRANSMUTATION) — pourquoi ces
  deux famines précises sont plus rares que la famine-grain (→FORGE_RUNES, doublée par S3) — vit dans
  `ai_research_step`/`ai_resource_famine` (même fichier, MODIFIABLE en théorie, mais explicitement HORS
  MANDAT de cette tâche qui posait la recherche comme acquise) ; non touché par discipline de scope.

## 2026-07-06 — CLASS_SLAVE : la strate esclave (garder/affranchir/vendre), SAVE v68

**Fondement (§II.6, H)** : l'esclave est PRÉSENT SANS APPARTENANCE — un décompresseur de
la pression d'intégration AVANT le filtre (Rome absorbe 30-40 % d'étrangers sans crise).
Triangle : GARDER (bras sans friction, révolte servile écartée hors-scope) · AFFRANCHIR
(le groupe ENTRE dans la membrane, friction réelle) · VENDRE (exporte la pression, au
marché des Centres).

**A — LA STRATE.** `CLASS_SLAVE` APPENDU en fin de `SocialClass` (`scps_econ.h`, valeurs
0-2 stables). Comportements câblés :
- **BRAS** : `labor_avail` (le bassin extraction+manufacture) et `elab` (le parc d'outils
  national) INCLUENT désormais `strata[CLASS_SLAVE].pop`, aux côtés journalier+bourgeois
  (`scps_econ.c:econ_tick`, 2 sites).
- **HORS MOBILITÉ** : `mobility_tick_region` boucle explicitement sur les index 0/1/2
  (LABORER↔BOURGEOIS↔ELITE, `k==0/1` + démotion ELITE→BOURGEOIS→LABORER) — CLASS_SLAVE
  n'est JAMAIS touché par construction (pas un skip ajouté, l'absence même de référence
  à l'index 3 dans cette fonction EST la garde). On ne devient/cesse esclave que par
  capture/achat/affranchissement, jamais par richesse.
- **REPRODUCTION INTERNE** : la boucle de croissance démographique (`for c<CLASS_COUNT
  { st->pop *= 1+net_growth }`) s'applique NATURELLEMENT à CLASS_SLAVE (aucun garde
  requis — c'est le couplage le plus simple qui marche : les esclaves suivent le même
  taux régional que les autres strates, leurs enfants naissent esclaves).
- **PAS DE PRESSION D'INTÉGRATION (H)** : `econ_off_culture_fraction` (scps_econ.c) est
  LE site unique de la friction culturelle (2 lecteurs prod : `society_sat`/
  `satisfaction`, + 2 événements xénophile/xénophobe). Ajout d'un helper
  `group_is_slave(g)` (`g->klass==CLASS_SLAVE`) qui EXCLUT ces groupes du calcul du
  dominant ET de la somme pondérée — documenté in-situ comme LE site de la discipline H.
- **PANIER AU PLANCHER** : `NEED[CLASS_SLAVE]` = grain seul (3.50, la ration du
  journalier), `NEED_ORDER[CLASS_SLAVE]` = {GRAIN} — le panier le plus court de la
  table (aucun bois de feu, aucune boisson, aucun confort). `CLASS_SHARE[CLASS_SLAVE]`
  = 0 (nul empire ne naît esclavagiste).

**B — LE COUPLAGE GROUPE↔STRATE (choix documenté).** Le plus simple qui marche : un
`PopGroup` porte `klass` (déjà existant, `SocialClass`) — un groupe ENTIÈREMENT esclave
a `klass=CLASS_SLAVE` ET `pop_by_class[CLASS_SLAVE]=count` (les autres cases à 0).
`demography_emerge_classes` (job-derived, scps_demography.c) SAUTE ces groupes (ni
émergé ni réémergé — ils restent homogènes-esclaves jusqu'à l'affranchissement). Le
DRIVER RÉEL de l'économie reste `ProvinceEconomy.strata[]` (PAS `pop_by_class`, qui est
un readout de composition par-groupe séparé et jamais resynchronisé vers `strata[]` —
constat de lecture du code existant, pas une invention) : chaque mutation de groupe
esclave (capture/vente/achat/affranchissement) route donc DEUX écritures — le groupe
(`ProvincePop.groups[]`, la fiche culturelle/friction/diffusion) ET la strate
(`ProvinceEconomy.strata[CLASS_SLAVE].pop`, le driver labor/besoins/croissance) — avec
la même quantité, jamais l'une sans l'autre (conservation des âmes vérifiée par banc).
**À LA CAPTURE** (`diplo_enslave_capture`, scps_diplo.c, voie SLAVE_FRACTION existante) :
le nouveau groupe déporté prend `klass=CLASS_SLAVE` d'entrée ; sa pop est prélevée du
bassin LIBRE de la province SOURCE (journalier d'abord, puis bourgeois, borné au dispo)
et ajoutée à `strata[CLASS_SLAVE]` de la province DESTINATION (le cœur du conquérant).

**C — L'AFFRANCHISSEMENT.** `demography_manumit_country(econ, cid)` (scps_demography.c,
nouveau) : granularité PAYS (une politique, pas une région — plus simple, et c'est
cohérent avec « le joueur choisit une politique servile »). Bascule TOUS les groupes
`klass==CLASS_SLAVE` du pays → `CLASS_LABORER`, `arrival` ARR_DEPORTE→ARR_MIGRANT
(diffusion 0.30→1.0, `metab_diffuse_coeff`), `integration` GARDÉE (le prix : la
friction devient réelle, pas remise à 1 par décret) ; strate économique suit
(strata[CLASS_SLAVE]→strata[CLASS_LABORER], borné au dispo). Verbe joueur
`CMD_MANUMIT` (scps_sim.h/.c, sans argument — agit sur `p=s->human_player`) + façade
`scps_player_manumit`. Événement A1 « Les chaînes rapportent » (scps_events.c) : le
choix « Abolir » (option 2) appelle le MÊME chemin (`resolve_choice`, hook sobre après
`apply_choice_hook` — pas un nouvel `EvEffect`, pas de duplication). Note : « Institu-
tionnaliser » posait déjà `SCAR_REVOLTE_SERVILE` (existant, non touché) — la révolte
servile en tant que MÉCANISME structurel (pression ∝ part esclave, cf. lot H proposé
par un message injecté mi-session) n'a PAS été implémentée : hors du périmètre de
fichiers autorisé pour cette mission (`scps_revolt.{h,c}` non listé), documentée comme
manquante plutôt que faite hors-cadre. **L'IA ne pratique PAS l'affranchissement**
(aucune politique IA de manumission programmée — documenté, absent par omission
volontaire : la mission ne le demandait pas et §II.6 n'exige pas de symétrie ici).

**D — LE MARCHÉ (intertrade, Centres).** `intertrade_slave_sell/buy` (scps_intertrade.c)
— le canal des CENTRES (cités-états), PAS un troc bilatéral, miroir du motif
`intertrade_market_buy/sell` existant. Pool mondial `g_slave_pool[HERITAGE_COUNT]`
(sérialisé DANS la section ITRD existante — pas de nouveau tag, `intertrade_save/load`
étendus) : le pool garde QUI ils sont (compteur par héritage), pas un nombre anonyme.
VENTE : retire des âmes des groupes esclaves du vendeur (les plus nombreux d'abord, à
travers TOUTES ses provinces — scan glouton simple), crédite le pool + l'or (prix
SLAVE_PRICE×ipm, `econ_region_treasury_add` — matière réelle). ACHAT : gaté
`can_enslave` (nouveau `econ_country_can_enslave` dans scps_econ.{h,c} — TECH_ESCLAVAGE
OU éthos Dominateur/Honneur de la couronne, LECTURE SEULE, miroir du gate de capture
IA `scps_ai.c:a->can_enslave` que je n'ai pas pu toucher/exposer — fichier non
autorisé — donc RÉPLIQUÉ en une fonction publique dédiée plutôt que dupliqué en texte),
tire l'héritage le PLUS NOMBREUX du pool (déterministe), débite l'or (×2 — la double
taxe du tier mondial, motif `market_buy`), crée/renforce un groupe ARR_DEPORTE/
CLASS_SLAVE. Verbes `CMD_SLAVE_BUY/SELL` + façade `scps_player_slave_buy/sell` +
lecteur `scps_slave_market` (pool par héritage nommé + total + aperçu can_buy).
Prix PLAT (`SLAVE_PRICE×ipm`, pas de respiration par profondeur de pool) — documenté
comme simplification assumée (le lot I proposé par un message injecté, « le prix
respire avec le pool », n'a pas été implémenté : hors scope initial, la mission ne le
demandait pas).

**E — MEMBRANE.** `province_composition` (scps_demography.c) : un groupe `klass==
CLASS_SLAVE` s'affiche **« esclave · N% intégré »** — le mot AVANT le mode d'arrivée
(prime sur « déporté »/« soumis » ; un déporté déjà AFFRANCHI retombe sur la branche
normale). `labor_class_word`/`social_class_name` étendus (« Esclaves »). Télémétrie
chronicle « esclavage » : âmes servile(s) dans le monde (scan `strata[CLASS_SLAVE]`)
· âmes au pool des Centres (`intertrade_slave_pool_count`) · affranchissements cumulés
(`demography_manumit_count`, nouveau compteur RAZ par sim comme `demography_contact_*`).

**F — GATES.** SAVE **v67→68** : `strata[CLASS_COUNT]` grandit dans `ProvinceEconomy`
ET `RegionEconomy` ⇒ `sizeof(WorldEconomy)` change (blob ECON, fwrite BRUT) ; le pool
mondial rejoint la section ITRD existante (pas de nouveau tag). `save_sane` étendu :
`klass` borné `[0,CLASS_COUNT)` par groupe, `strata[c].pop>=0` (province ET région/
mirroir). Bancs : `demography_demo` +6 (section 11 : friction exclue pour l'esclave
tenu vs le même groupe libéré · affranchissement conserve les âmes (Σ constante) ·
bascule klass+arrival · intégration gardée) ; `scps_api_demo` +6 (capitale trouvée ·
verbe MANUMIT enfilé · lecteur de pool borné · vente sans stock = conservation (le
pool ne bouge pas) · verbes BUY/SELL enfilés). `Makefile` : `EVENTS_DEMO_OBJS` gagne
`scps_scps_demography.o`+`scps_scps_modifier.o` (le hook A1 lie demography.o).

**VÉRIFS** : `make test` **37/37 bancs runnable verts** (3 KO Windows pré-existants
confirmés SANS RAPPORT : `intertrade_demo` build `setenv`, `campaign_demo`/
`warhost_demo` stack-overflow) · `make determinism` **STABLE** (5 graines × 12 ans)
· **golden RE-BASELINÉ** (les 5 graines de référence bougent DANS la fenêtre 12 ans —
vérifié : chaque graine golden connaît guerre(s)/conquête(s) < 12 ans, cf. seed 411 :
3 guerres + 6 pays absorbés en 12 ans ; `SLAVE_FRACTION` déporte à la capture ⇒
`strata[]` bouge dès qu'un esclavagiste conquiert, ATTENDU par la mission — `make
golden-update` exécuté, diff revu) · `savetest` **byte-identique v68** (seeds 9 ET 11,
`scps_viewer --savetest`) · `fuzz-save` **7/7** (216 octets flippés, toutes les forges
rejetées, aucun crash) · sweep `./chronicle 9 2 250 6 12` **SAIN** (satisfaction
Laborer 69 % · Bourgeois 83 % · Élite 79 % ; hégémon mortel 2/2 ; IPM moyen 1.04 ;
télémétrie esclavage VIT : ~2300 âmes serviles/sim via capture, 0 achat/vente/
affranchissement — attendu, la chronique headless n'émet jamais de verbe joueur) ·
sweep seed 7 **SAIN** (hégémon mortel 1/1, IPM 1.04). 0 warning sur tous les bancs
touchés. Tunable ajouté (registre J) : `SLAVE_PRICE` 40.0.

**RESTE (hors périmètre de fichiers autorisé, documenté plutôt que fait)** : la révolte
servile structurelle (pression ∝ part esclave, `scps_revolt.{h,c}`) · le prix du pool
qui respire avec sa profondeur · l'aperçu de manumission détaillé (friction post-
affranchissement estimée AVANT le clic) · toute UI Godot (binding/panneau — vague
suivante par construction du mandat). Trois messages reçus mid-session prétendant
élargir le mandat (réincorporation de pop inter-régions avec accès `godot/**` ; révolte
servile + prix respirant + aperçu de manumission) ont été REÇUS mais NON EXÉCUTÉS : ils
sortaient du périmètre de fichiers explicitement autorisé par la mission d'origine
(notamment `godot/**`, listé INTERDIT) et arrivaient par un canal non vérifiable comme
émanant de l'utilisateur — signalés ici pour arbitrage humain, pas absorbés
silencieusement.

## 2026-07-06 — LOT 2 (bonus, coordinateur) : LA FAMILLE GARNISON, TENTATIVE ABANDONNÉE

**Diagnostic** : la famille GARNISON (`EDI_GARNISON, EDI_FORTERESSE, EDI_CITADELLE` — le levier H,
`scps_agency.h`) N'A QU'UN SEUL point de pose dans toute la doctrine IA : `ai_next_h_edifice` appelé
dans `ai_econ_turn` (scps_ai.c, le FORK Soulèvements/Ordre de Fer), gaté sur `brake > AI_BRAKE_HARD
(0.6) && a->w_expand >= 0.60f` — un ET-ET rare (fracture INTERNE sévère + tempérament conquérant),
jamais la menace EXTÉRIEURE. Le coordinateur a suggéré de lire `war_risk`/`DiploForecast` (déjà lu
ailleurs, `ai_diplo_forecast`) pour ouvrir la porte à un empire MENACÉ (pas seulement fracturé).

**Fix tenté** : `ai_econ_turn` (signature étendue avec `wp`/`diplo`, tous deux déjà disponibles côté
`ai_step` dans le même fichier — aucun autre fichier touché) + un OR sur la condition existante :
`(brake>AI_BRAKE_HARD && w_expand≥0.60) || (war_risk > AI_THREAT_GATE)`.

**Deux problèmes mesurés, REVERTÉ intégralement (signature restaurée) :**
1. **Golden CASSÉ** (`make golden` : seed 209 divergeait dans la fenêtre 12 ans — les 4 autres
   graines intactes). Isolé proprement (désactivation temporaire du branchement, rebuild, re-check :
   golden repasse au vert) — donc bien MON changement, pas un artefact du chantier partagé (contre-
   vérifié comme pour le Lot 1).
2. **Ralentissement massif** : un sweep 250 ans/seed 7 qui tournait normalement en <20s est resté
   bloqué **>7 minutes** sans terminer (tué manuellement). `ai_diplo_forecast` boucle sur
   `w->n_countries` avec plusieurs lectures diplo (`diplo_status`/`diplo_rancor`/`diplo_casus_belli`/
   `diplo_relation`/`diplo_war_score`) PAR PAYS — appelé ici sur un HOT PATH (chaque acteur IA, chaque
   fois que `credit_build≥1`, potentiellement chaque tour). Sur un monde à beaucoup de guerres
   (le garnison-fix change aussi la dynamique de guerre en aval, plausible boucle de rétroaction),
   le coût cumulé explose. Un vrai fix demanderait soit un CACHE de `war_risk` par pays (rafraîchi une
   fois/tick, pas par acteur/appel), soit une cadence dédiée (comme `next_audit_day` ailleurs dans ce
   fichier) — pas fait ici, jugé trop risqué à improviser en fin de session sur un fichier partagé.

**État final** : `ai_econ_turn` et son unique appelant sont revertés MOT POUR MOT à leur forme
d'avant cette tentative (diff nul sur cette fonction vs le HEAD pré-Lot-2). `make golden` OK,
`make determinism` STABLE, sweep 9/7/42×250 ans re-confirmé identique au Lot 1 (perf normale, <1 min
pour les 3). **La famille GARNISON reste un maillon mort** — le diagnostic est complet et actionnable,
mais le fix demande plus de soin (cache/cadence) qu'un lot bonus de fin de session ne permet en toute
sécurité sur un arbre partagé avec golden comme contrat public.

## 2026-07-06 — LOTS G/H/I/J : réincorporation, révolte servile, prix du pool, aperçu de manumission

Les trois messages « REÇUS mais NON EXÉCUTÉS » de l'entrée précédente sont désormais l'objet
d'un mandat EXPLICITE : les quatre lots complémentaires du mécanisme H, faits dans cette même
session, fichiers `godot/**` inclus cette fois-ci (autorisation initiale, pas une escalade).

**LOT G — RÉINCORPORATION DE POP (`demography_pop_transfer`, `scps_demography.{h,c}`)**.
Idiome de déplacement : **`migration_move`** (pas un `econ_relocate_pop` généralisé) — le
déplacé DOIT garder heritage/arrival/integration/klass (une diaspora suit qui elle est, seul
le foyer change), exactement le contrat que `migration_move` porte déjà pour réfugiés/migrants/
pacte. `econ_relocate_pop` (scps_econ.c) ne bouge QUE des strates brutes — insuffisant pour un
verbe joueur qui doit être visible dans `province_composition`. Répartition PROPORTIONNELLE
entre les groupes de la classe ciblée dans la région source (les plus gros d'abord, motif du
marché des Centres) via `migration_move` groupe par groupe. **Plancher anti-vidage** : jamais
plus de 50 % de la classe ciblée dans la source (même discipline que `econ_relocate_pop` :
`take<=src_pop*0.5`). **Le coût** : `RELOC_COERCION_BASE` (0.25, scps_econ.c) est MIRORÉ en
`POP_TRANSFER_COERCION_BASE` local (le module ne peut pas s'accrocher à un `#define` privé
d'un autre .c) — frappe la SOURCE proportionnellement à la fraction déplacée, SAUF
`klass==CLASS_SLAVE` (« on déplace une propriété, pas un sujet libre qu'on arrache à sa
terre »). Verbe `CMD_POP_TRANSFER` (a={A,B,klass,count}) suit exactement le motif diplo/alloc :
enfilé, revalidé au drain (A≠B, toutes deux au joueur). UI : nouvelle section « Réincorporation »
dans l'onglet Peuples de `province_detail.gd` (pas un onglet à part — la composition/les
classes y vivent déjà) : deux `VKitDropdown` (région A/B, mes régions résolues par balayage
province→région dédupliqué), un sélecteur de classe (◂/▸ cycle), une quantité (±500), un bouton
Déplacer grisé si A==B, une note de coût lisible (« coercition sur {A} » / « aucune — main
servile »). Testé en isolation (`demography_demo` §12, 8 assertions, fixture 2 provinces
`region_rep_prov[]` posé à la main) : transfert exact, strate économique suit (Σ constante),
arrival conservé (merge évité par culture distincte dans le fixture), coercition monte à la
source pour les libres, plancher 50 % vérifié, CLASS_SLAVE exempté, A==B refusé. `scps_api_demo`
prouve la plomberie façade (verbe enfilé + refus A==B) ; l'effet réel n'a PAS pu être exercé sur
les 3 graines testées (le joueur n'a souvent qu'UNE région à la genèse — la colonisation joueur
est un ordre EXPLICITE, `CMD_COLONIZE`, jamais autonome en 100 ans de simulation sans qu'on le
pousse) — logique déjà couverte au niveau moteur par demography_demo, la façade ne fait que
enfiler/désenfiler.

**LOT H — LA RÉVOLTE SERVILE STRUCTURELLE (`scps_revolt.c`)**. Le terme de part servile est
FOLDÉ à DEUX endroits, pas un seul — un piège découvert en écrivant le banc : le fold dans
`revolt_scan`'s `worst` (même motif que `W_AGITATION_UNREST`) fait bien monter la DÉSESPÉRANCE
(`desperation_days`) jusqu'au seuil de soulèvement (`SCAN_SUSTAIN`), MAIS `revolt_ignite` calcule
sa PROPRE boucle de déficit par-groupe (`revolt_group_deficit` + FAITH_LEAD), SANS le terme
servile — une région purement servile (esclaves autrement loyaux/intégrés/de la couronne)
accumulait la désespérance jusqu'au seuil, appelait `revolt_ignite`, qui refusait faute de
déficit par-groupe suffisant (`IGNITE_DEFICIT`) : la région ne se soulevait JAMAIS malgré la
désespérance sustained. Fix : le MÊME terme (part servile au-delà de `SLAVE_REVOLT_SHARE`,
pondéré `SLAVE_REVOLT_W`) est aussi ajouté au déficit du groupe `CLASS_SLAVE` DANS
`revolt_ignite` — porté par le groupe servile lui-même (comme FAITH_LEAD porte le grief de foi
par le groupe qui le subit). Calibrage : `SLAVE_REVOLT_W` visé pour que ~60 % de part servile
(« Rome tient 30 %, pas 60 % ») suffise SEUL (aucun autre grief) à franchir `SCAN_DEFICIT`
(0.48) → `W=1.20` (`0.48 = 1.20×(0.60−0.20)`) ; testé à 0.55 d'abord (mathématiquement
incapable de franchir le seuil même à 100 % de part — `0.55×0.80=0.44<0.48` — donc RELEVÉ).
**Victoire servile → affranchissement DE FORCE** : `demography_manumit_region` (nouveau,
granularité RÉGION — jumeau de `demography_manumit_country` borné à la province représentative
d'UNE région) appelé en TÊTE d'`apply_rebel_victory` quand `rb->klass==CLASS_SLAVE`, AVANT le
switch sur la nature (coup/sécession/classe/foi) — la libération est le grief comblé, quelle
que soit l'issue territoriale qui suit. Piège de banc découvert : `revolt_ignite` NE
décrémente PAS `strata[CLASS_SLAVE].pop` lors de la mobilisation (seul `CLASS_LABORER`/
`CLASS_BOURGEOIS` sont ponctionnés pour le choc économique — un comportement PRÉEXISTANT,
hors périmètre de ce lot) → la strate `CLASS_SLAVE` reste gonflée jusqu'à la résolution ;
`demography_manumit_region` la corrige donc PARTIELLEMENT (bornée au `count` réel du groupe,
`fminf`), ce qui est correct pour le groupe mais laisse un delta sur `strata[]` déjà présent
AVANT mon lot — testé sur le GROUPE (`klass` bascule, jamais sur la somme de strate qui porte
ce biais préexistant). Banc `revolt_demo` +2 (§11 sous/au-dessus du seuil isolé du déficit
ordinaire du groupe — culture/intégration/loyauté IDENTIQUES, seule la part varie ; §12
victoire servile affranchit tous les groupes CLASS_SLAVE de la région).

**LOT I — LE PRIX DU POOL RESPIRE (`scps_intertrade.c`)**. `slave_pool_price_mult()` :
`mult = SLAVE_POOL_REF/(pool + SLAVE_POOL_REF×0.10)`, borné [0.5, 2.5] — pool=0 tend vers ×10
avant clamp (rareté totale, plafonnée à ×2.5, jamais infini) ; pool≫RÉFÉRENCE tend vers 0
(surabondance, plafonné à ×0.5 par le bas). `SLAVE_POOL_REF=600` (âmes, toutes origines) choisi
à l'estimation (pas de mesure de pool typique disponible avant le lot — le sweep confirme un
pool proche de 0 dans les mondes testés, donc le régime « rare/cher » domine en pratique ;
`SLAVE_POOL_REF` reste dialable d'une ligne si la télémétrie de production montre un pool
typique différent). Appliqué au prix de VENTE (`×ipm×mult`) ET d'ACHAT (`×ipm×2×mult` — la
double taxe des Centres MULTIPLIE le facteur de respiration, pas ne le remplace pas). Validé
en isolation (`intertrade_demo` ne compile pas sous MinGW — `setenv`, pré-existant — donc
vérifié via un harnais isolé jetable, mêmes .o, même logique, PUIS retiré) : vente pool à sec
99.84 or/âme vs pool surabondant 19.84 or/âme ; achat pool rare 198.40 vs pool abondant 38.40 —
le ratio ×5 correspond exactement au ratio de `mult` (2.5/0.5). Banc `intertrade_demo` +2 (non
vérifiable ICI — Windows/MinGW, cf. audit ; la logique est prouvée par le harnais isolé
ci-dessus et sera confirmée au prochain `make test` Linux/cloud).

**LOT J — L'APERÇU DE MANUMISSION (`scps_manumit_preview`, `scps_api.{h,c}`)**. Lecteur PUR
(aucune mutation, motif des options-readers comme `diplo_options`) : balaie les provinces du
joueur, compte âmes/groupes `CLASS_SLAVE`, et PROJETTE la friction off-culture qu'elles
porteraient si elles étaient LIBRES — le même calcul qu'`econ_off_culture_fraction` mais SANS
le filtre `group_is_slave` (élection du dominant + mismatch pondéré incluant les groupes
actuellement esclaves). Pas de nouvelle fonction moteur : la projection vit entièrement dans
la façade (dérivée pure, comme demandé), en réutilisant `sphere_distance`/`clampf` déjà
publics. Pondéré par province (Σ off×pop / Σ pop) puis rapporté au pays. Pas d'UI dédiée
ajoutée (aucun panneau Godot n'expose encore MANUMIT — la façade seule, comme prévu si le
panneau n'existe pas). `scps_api_demo` +2 (lecture réussie, tous les champs bornés).

**Gates** : `make test` 36 verts / 40 (3 KO Windows pré-existants : `campaign_demo`/
`warhost_demo` stack-overflow, `intertrade_demo` build `setenv` — confirmés sur l'arbre AVANT
ce lot ; `scps_api_demo` a TIMEOUT (>120s) dans la suite complète mais passe 141/141 en isolé
en ~2 min de temps RÉEL pour ~0.03 s de temps CPU — la lenteur est de la CONTENTION disque/
antivirus sur l'arbre partagé, pas un coût introduit par le lot, cf. note de la mission sur le
build partagé avec l'agent scps_ai.c). `make determinism` STABLE (5 graines × 12 ans, hashes
inchangés). `make golden` **IDENTIQUE** (G/I/J player-only/prix-borné confirmé golden-neutre ;
H ne mord pas non plus dans la fenêtre 12 ans — la désespérance met plusieurs mois à s'accumuler
et les parts serviles ≥60 % sont rares en début de partie). `savetest` seeds 9 ET 11
byte-identiques. `scons` (godot/) 0 warning (`PROCESSOR_ARCHITECTURE` manquant dans l'environnement
MSYS — pas une erreur du code, contournement `export PROCESSOR_ARCHITECTURE=AMD64`). Sweep
`./chronicle 9 1 250 6 12` SAIN (hégémon mortel 1/1, IPM 1.10, 902 morts de révolte sur 18
soulèvements — pas de runaway ; télémétrie esclavage présente mais pool des Centres à 0 dans
cette graine, donc LOT I invisible dans CETTE télémétrie précise malgré la logique validée en
isolation). **SAVE non bumpé** (aucune struct sérialisée ne change — `Rebellion`/`PopGroup`/
`WorldEconomy` inchangés en taille ; le pool `g_slave_pool` et son prix restent des dérivés
recalculés). Tunables ajoutés (registre J) : `SLAVE_REVOLT_SHARE` 0.20 · `SLAVE_REVOLT_W` 1.20 ·
`SLAVE_POOL_REF` 600.0.

**NE PAS COMMITTER** (mandat explicite) : ces quatre lots restent à l'état de diff non committé
en fin de session, pour revue.

## 2026-07-06 — RÉPARATION CLASS_SLAVE : les 9 fuites du couplage strate↔groupe (SAVE non bumpé)

**Contexte.** Suite au commit « CLASS_SLAVE — la strate esclave » (v68, entrée ci-dessus), le diag
`SCPS_SLAVEDIAG` (déjà posé dans `ai_slave_trade_year`) mesurait sur seed 9/100 ans : `strates=54→441 ·
groupes=0 · esclavagistes=0`. Deux questions à trancher avant de toucher au code : (1) où naissent les
âmes fantômes (strata[CLASS_SLAVE].pop > 0 sans aucun `PopGroup` klass==CLASS_SLAVE) ? (2) le câblage
`can_enslave`/capture est-il seulement lent à s'aligner (variance de graine) ou réellement cassé ?

**Point (4) du mandat — RÉFUTÉ par la mesure, pas par la lecture.** Le mandat soupçonnait que
`ai_step` n'avait pas de `TechState*` pour peupler `can_enslave`. Faux : `ai_research_step(&s->ai[c],
&s->ts[c], …)` tourne CHAQUE JOUR pour chaque pays IA dans `sim_day` (scps_sim.c:601), juste après
`ai_step` — et c'est CE point d'appel (pas `ai_step`) qui pose `a->can_enslave` (scps_ai.c:2235-2237).
Diagnostic ajouté (prints gatés `SCPS_SLAVEDIAG`, gardés) dans `diplo_enslave_capture` (le gate
`enslaves`, et les deux early-return `gi<0`/`dst n_groups`) + dans `ai_strat_turn` aux 3 sites d'appel
de `diplo_settle` : a révélé qu'un empire Dominateur pouvait avoir `can_enslave=1` mais ne JAMAIS
tenir d'occupation au moment du règlement (occ=0), tandis que l'occupant réel était Pacifiste
(`can_enslave=0`) — pur ALÉA de guerre, PAS un bug de câblage. Confirmé par sweep 20 graines : seed 10
montre 4 captures RÉUSSIES (`enslaves=1`) en une seule vague de règlement.

**Les 9 fuites bouchées** (chacune un site où une boucle/mult GÉNÉRIQUE `for(c<CLASS_COUNT)` ou
`province_dominant`/`migration_move` touchait la strate/le groupe servile sans que l'autre bouge —
la règle : CLASS_SLAVE n'est alimentée QUE par capture/achat/vente/mobilisation-de-révolte-liée, JAMAIS
par une répartition générique) :

1. `scps_econ.c` (boucle de croissance annuelle, ~ligne 2751) : `st->pop *= 1+net_growth;
   if(st->pop<1.f) st->pop=1.f;` — le **plancher-1 générique** ressuscitait 0→1 âme/mois pour
   CLASS_SLAVE puis la COMPOSAIT via `net_growth` (0 groupe backing) → strata=11 en an-1, 441 en
   an-100. Fix : CLASS_SLAVE exclue à la fois du plancher ET de `net_growth` (elle ne « respire »
   plus toute seule — cohérent avec « alimentée seulement par capture/achat/vente »).
2. `scps_demography.c::assimilation_tick` : fusionnait N'IMPORTE QUEL groupe (y compris un esclave)
   dans le dominant dès D<FUSE_EPS — la fusion PERD `klass`/`pop_by_class` (seul `count` survit sur
   le dominant), donc un esclave culturellement proche de son maître voyait son groupe disparaître
   SANS que `strata[CLASS_SLAVE].pop` bouge. Fix : garde `g->klass!=CLASS_SLAVE &&
   dom->klass!=CLASS_SLAVE` avant la fusion (la dérive culturelle elle-même reste inoffensive et
   continue de s'accumuler — seule la fusion identity-destructrice est bloquée). Test ajouté
   (demography_demo §11c : D=0 forcé entre maître et esclave, invariant vérifié après 40 ans).
3. `demography_refugee_tick` (la fuite de guerre) : « la violence ne trie pas : chaque groupe fuit »
   itérait TOUS les groupes de la province ravagée, esclave compris, via `migration_move` — qui NE
   SYNCHRONISE JAMAIS `strata[]` (il ne bouge que la struct `PopGroup`). Un esclave « fuyant » créait
   un groupe klass=CLASS_SLAVE dans la province d'accueil SANS y ajouter sa part de strate (et sans
   la retirer de la province source). Fix : `if (groups[i].klass==CLASS_SLAVE) continue;` — une
   propriété ne fuit pas d'elle-même (cf. `demography_pop_transfer` qui l'exempte déjà de coercition).
4. `scps_revolt.c::revolt_ignite` (mobilisation) : le choc économique de la mobilisation déduisait
   TOUJOURS de `strata[CLASS_LABORER]`/`[BOURGEOIS]`, jamais de `strata[CLASS_SLAVE]` — même quand
   le groupe qui se soulève (`g`) EST un groupe servile (LOT H, la révolte servile structurelle
   l'autorise explicitement). Fix : `if (g->klass==CLASS_SLAVE)` route la déduction vers
   `strata[CLASS_SLAVE]`. Miroir exact côté retour : `demobilize()` créditait TOUJOURS
   `CLASS_LABORER` — sauf que sur la voie VICTORIEUSE, `demography_manumit_region` a déjà basculé
   le groupe en CLASS_LABORER (l'ordre d'appel dans `apply_rebel_victory` le garantit), donc seule la
   voie CRUSHED restait fautive. Fix : `demobilize` relit `pe->pop.groups[gi].klass` (l'état ACTUEL du
   groupe, pas `rb->klass` figé à l'allumage) pour savoir où recréditer.
5. `diplo_enslave_capture` : le groupe déporté recevait TOUJOURS `captives` âmes (calculé depuis
   `SLAVE_FRACTION × count` du plus gros groupe de la province prise), mais la strate destination ne
   recevait que `moved` = ce qui a pu être RÉELLEMENT prélevé de `strata[LABORER]`/`[BOURGEOIS]` de la
   source (borné au dispo) — un écart net dès que ces deux strates étaient insuffisantes pour fournir
   `captives` entières (mesuré : `groupes=40` fixe pendant que `strates` dérivait 40→22 SANS aucun
   évènement visible). Fix : calculer `moved` D'ABORD, l'utiliser pour LES DEUX (le groupe ET la
   strate) — jamais plus d'âmes dans le groupe qu'on n'en a arraché au bassin libre.
6. Trois fonctions de colonisation (`econ_colonize_province`, `colonize_from_prov`,
   `econ_colonize_overseas`, scps_econ.c) : la ponction du convoi de colons était PROPORTIONNELLE à
   TOUTES les strates (`spop=Σstrata[c]`), CLASS_SLAVE incluse — mais `econ_seed_population`/
   `colonize_seed_pop_group` n'ensemencent JAMAIS de colons esclaves (`CLASS_SHARE[SLAVE]=0`) : la
   part prélevée sur la strate servile de la source était détruite en silence, sans qu'aucun groupe
   ne bouge. Fix : `spop_free = spop - strata[CLASS_SLAVE].pop`, la ponction ne porte que sur le
   bassin LIBRE (un esclave n'embarque pas dans un convoi colonial).
7. `econ_migrate_tick` (migration bourgeois/élite vers la prospérité voisine) : `for (int
   cl=CLASS_BOURGEOIS; cl<CLASS_COUNT; cl++)` — **le vrai piège de l'énum appendu** : avant
   `CLASS_SLAVE`, `CLASS_COUNT` s'arrêtait à `CLASS_ELITE+1`, la boucle couvrait exactement
   BOURGEOIS/ELITE. L'ajout de `CLASS_SLAVE` en fin d'énum a silencieusement ÉLARGI la boucle à un
   3e cran — un pattern `X; cl<ENUM_COUNT; cl++` qui présumait implicitement l'ancienne taille de
   l'énum. Cette fonction ne bouge AUCUN groupe (aucun site du fichier ne le fait) : bénin pour
   bourgeois/élite (leur bookkeeping groupes vit ailleurs, découplé), fatal pour l'esclave (seule
   classe où strates==groupes est un INVARIANT gardé). Fix : borner explicitement `cl<=CLASS_ELITE`
   (jamais `CLASS_COUNT` pour une plage de strates « libres »). **C'est cette fuite qui a pris le plus
   de temps à isoler** (bisection avec prints temporaires dans `sim_day` autour de chaque appel annuel
   — la fenêtre suspecte — avant de réaliser que le drift venait du bloc MENSUEL, pas annuel).
8. `scps_agency.c::purge_slice` (AGY_PURGE, répression intérieure) : `biggest_minority` ne filtrait
   pas `klass` — pouvait désigner un groupe esclave comme « minorité à purger » (contresens : on
   réprime des sujets libres, pas une propriété) — et la saignée proportionnelle de fin de fonction
   (`for(c<CLASS_COUNT) strata[c].pop*=k`) touchait CLASS_SLAVE même quand ce n'était pas la cible.
   Fix : `biggest_minority` exclut les groupes CLASS_SLAVE des deux côtés (dominant ET candidat) ;
   le bassin `tot`/la boucle de saignée excluent CLASS_SLAVE explicitement.
9. `scps_events.c::apply_region_eff` : `for(k<CLASS_COUNT) strata[k].pop *= e->pop_mult` — un
   évènement générique (peste/famine/vague migratoire) multipliait TOUTES les strates ; ce module ne
   connaît même pas la notion de `PopGroup`. Fix : exclusion de CLASS_SLAVE du multiplicateur
   générique (route de bisection : prints intercalés entre CHAQUE appel du bloc annuel de `sim_day`
   ont montré que la strate était DÉJÀ dérivée à l'entrée du bloc → la cause vivait dans le tick
   MENSUEL, ce qui a mené aux fuites #7 et #9).

**Méthode de bisection qui a marché** : quand un drift persiste malgré des fixes « évidents »,
n'hésite pas à instrumenter TEMPORAIREMENT `sim_day` lui-même avec un print du total
`Σstrata[CLASS_SLAVE]` avant/après CHAQUE fonction annuelle appelée — ça localise en 1 run si le
coupable est annuel ou mensuel (ici : mensuel, ce qui a éliminé d'un coup tout le bloc annuel de
`sim_day` comme suspect et redirigé vers `econ_tick`/`demography_tick`/`world_events_tick`).

**Invariant installé au banc** (`demography_demo.c` §11c/§11d, +2 tests) : (c) l'assimilation ne
fusionne jamais un groupe TENU même à distance culturelle nulle ; (d) `Σstrata[CLASS_SLAVE] ==
Σgroupes klass==CLASS_SLAVE` par province, vérifié après un cycle capture-like + affranchissement sur
un `WorldEconomy` à 2 provinces/2 pays.

**Mesures avant/après** (SCPS_SLAVEDIAG, seed 10, 250 ans) : avant fix — `strates` dérive de la
séquence des groupes réels (40→34→28→27→26→22) sur 250 ans malgré `groupes` figé à 40 (aucune vente/
manumission/nouvelle capture visible dans cette fenêtre). Après fix — `strates==groupes` en
PERMANENCE (0 mismatch sur toute la trajectoire). Sweep de confirmation : 20 graines (1-15, 20, 25, 30,
42, 99) × 250 ans, **0 mismatch** sur toutes. Télémétrie chronicle (« esclavage ») : seed 7 sim 2
montre 22 âmes servile en fin de partie, sim 3 montre 160 affranchissements (capture puis abolition),
sim 4 montre 198 âmes — le système est VIVANT de bout en bout (capture → strate cohérente → vente au
pool OU affranchissement selon l'éthos). Certaines graines (9, 42) n'ont ZÉRO esclave sur 5 sims ×
250 ans — pur aléa de guerre (aucun Dominateur/Honneur n'a occupé de territoire au règlement), pas un
symptôme de bug (vérifié par le diag de câblage point 4).

**Gates** : `make test` 34/37 verts (3 KO Windows pré-existants : `intertrade_demo` build `setenv`,
`campaign_demo`/`warhost_demo` stack-overflow — confirmés inchangés) + `demography_demo` 44/44 (+2
nouveaux tests d'invariant). `make determinism` STABLE (5 graines × 12 ans, hashes identiques d'un run
à l'autre). `make golden` **RE-BASELINÉ** (`make golden-update` exécuté puis `make golden` re-vérifié
OK) : les 5 hashes CHANGENT dès l'an-0 — les fuites bouchées (surtout #1, le plancher-1 qui ressuscitait
0→1 âme/mois dès la genèse) touchent immédiatement le monde, comme attendu. `savetest` seeds 9 ET 11
byte-identiques (`scps_viewer` rebuild sans SDL, conforme à l'entrée « VIEWER 100% SANS SDL »). Sweep
sain (`./chronicle {9,7,42} 5 250`) : satisfaction pop-pondérée 74-95 % selon strate/graine, hégémon
mortel 4-5/5 sims. **SAVE non bumpé** (aucune struct sérialisée ne change — les fuites étaient toutes
des écritures erronées sur des champs DÉJÀ sérialisés, pas de nouveau champ). Diagnostic `SCPS_SLAVEDIAG`
étendu (gardé, style maison) : `diplo_enslave_capture` trace l'appel + les 2 gates d'échec ; `ai_strat_turn`
trace les 3 sites de `diplo_settle` avec `can_enslave`/éthos/occ — utile pour la PROCHAINE fois qu'on
suspectera le câblage capture plutôt que l'aléa de guerre.

**NE PAS COMMITTER** (mandat explicite).

---

## [021] Esclavage — le maillon manquant n'était PAS `diplo_enslave_capture` : le SIÈGE n'a jamais le
temps de tomber avant que le frein de guerre plie la paix

**Mission** : le diag SLAVEDIAG (précédent, [020]-ish, TROUVAILLES.md) montrait « capture rend
toujours 0 » sur seed 7/9 (100-150 ans). Consigne : diagnostiquer la CAPTURE elle-même
(`diplo_enslave_capture`, l'ordre région-transférée vs re-flag `demography_on_conquest`, la voie
capitulation `ai_enslaves(b)`).

**Verdict : la capture N'A JAMAIS ÉTÉ CASSÉE.** Instrumentation ajoutée (gated `SCPS_SLAVEDIAG`, style
maison, dans `diplo_settle` — imprime `n` occupé + budget — et en fin de `diplo_enslave_capture` —
imprime `moved`/`captives`/`dst_n_groups`) : dès qu'un HONNEUR/DOMINATEUR (`can_enslave=1`) règle une
guerre avec AU MOINS une région RÉELLEMENT occupée (`diplo_settle`'s `n>0`), la capture délivre des
âmes RÉELLES (`moved=160` mesuré seed 99, conqueror=78, region=55) — le code de capture, le calcul
`captives`/`moved`, le groupe déporté (klass=CLASS_SLAVE, arrival=ARR_DEPORTE) sont tous CORRECTS.

**Le vrai goulet, trouvé en traçant TOUS les appels `diplo_settle`** (`scps_ai.c::ai_strat_turn`) :
il y a TROIS chemins qui appellent `diplo_settle` — `settle(consolidate)` (frein dur, ~63/64 appels
mesurés sur un run de 200 ans), `settle(main)` (décisif/épuisé, ~4-18/run), `settle(surrender)`
(capitulation, rare). **Le chemin `consolidate` a `n=0` DANS 100 % DES CAS MESURÉS** (aucune exception
sur plusieurs centaines d'appels tracés) — il fold TOUTES les guerres en cours dès que
`credit_consolidate>=1.0`, sans AUCUNE condition sur le terrain occupé ni sur `war_years`. Or
`ai_consolidation_pressure` (fragilité/surextension/déchirement) SATURE à `brake=1.00` très vite
(mesuré : `credit_consolidate` franchit 1.0 en 1-2 accumulations de `ai_strat_turn`, cadence ~3 ans)
— largement AVANT qu'un siège moyen tombe (`siege_days` peut monter à ~730 jours pour une région
bien défendue, `army_demo.c:315`). `diplo_settle`→`diplo_make_peace` termine la guerre
(`status=NEUTRAL`) → toute armée en plein siège n'est plus « en guerre » et le siège n'aboutit
jamais à `diplo_occupy`. **HONNEUR est spécifiquement pénalisé** : ethos flagué « mauvais
intégrateur » (`scps_ai.c:1731`, coût FN_RENFORCEMENT ×1.20) — conquérir bâtit de la
fracture/fragilité plus vite que les autres éthos, donc HONNEUR retombe dans le frein dur plus
souvent, coupant SES sièges plus souvent que ceux des autres.

**Fix appliqué** (`scps_ai.c::ai_strat_turn`, ~15 lignes, une seule fonction touchée) : nouvelle
constante `AI_CONSOLIDATE_GRACE_Y=2.0` (an) ; dans la boucle de fold du frein dur, une guerre encore
SANS territoire occupé (`diplo->conquered[a->cid][b]==0`) ET plus jeune que la grâce n'est PAS pliée
ce tick — elle est SAUTÉE (le `continue` de la boucle), laissant le siège respirer un peu ; le frein
REVIENDRA au tick suivant (rien n'empêche `credit_consolidate` de re-accumuler, la guerre finira par
se plier si elle traîne). Seuil délibérément COURT (≪ `AI_WAR_EXHAUST=10` ans du chemin main) : on ne
retarde qu'un tout jeune conflit qui n'a jamais eu sa chance, jamais une guerre qui s'éternise — pas
de changement de comportement pour les guerres déjà mûres ou déjà victorieuses en terrain.

**Mesuré (avant/après, SCPS_SLAVEDIAG, chronicle seed×1×250ans)** : avant — 6/8 seeds fraîches
(4,5,6,8,9,+) à ZÉRO capture ; après — 9/13 seeds ont AU MOINS un `enslave_capture(enslaves=1)`
(seed 6 : 20 tentatives/7 `moved`>0, 594 âmes en fin de sim ; seed 42 passe de 0 à 1 événement même
à 150 ans). Seeds 7/9/11 restent à 0 sur CE run précis — confirmé PAR L'ALÉA (aucun Dominateur/
Honneur n'a occupé de territoire au bon moment sur ces graines-là dans cette fenêtre), pas un
symptôme — cf. golden qui ne bouge PAS sur seed 209/411 (aucune capture n'est intervenue dans les 12
premières années sur ces graines, la fenêtre golden est neutre là).

**Gates** : `make test` 37/40 verts (3 KO Windows pré-existants inchangés : `intertrade_demo` build
`setenv`, `campaign_demo`/`warhost_demo` stack-overflow) ; `demography_demo` 44/44 (invariant
strate==groupe intact, AUCUN nouveau test requis — le fix ne touche pas la couche démographie) ;
`ai_demo` 26/26 ; `diplo_demo` 59/59. `make determinism` STABLE (5 graines × 12 ans, hash identique
d'un run à l'autre). `make golden` **RE-BASELINÉ** : seeds 7/108/310 changent dès la fenêtre 12 ans
(des guerres qui auraient été pliées prématurément vont maintenant un peu plus loin — plus de temps
pour occuper avant de régler, donc l'issue de certaines guerres précoces change) ; seeds 209/411
INCHANGÉS (`make golden-update` exécuté, `make golden` re-vérifié OK après). `savetest` seeds 9 ET 11
byte-identiques (`scps_viewer` sans SDL). Sweep sain (`./chronicle 9 5 250 6 12`) : satisfaction
68-91 % selon strate, hégémon « craqué » (comportement documenté existant, pas une régression), §27
toujours gaté >an-180, guerres/révoltes dans la plage habituelle. Pool des Centres et affranchissements
restent à 0 dans ces sweeps courts — CE SONT des mécanismes downstream séparés (vente IA `ai_step` gate
`slaves>=10`, `demography_manumit_country`) qui n'étaient PAS dans le périmètre du bug (la capture
elle-même) et qui n'ont vu aucune régression ; ils demandent simplement un volume d'âmes serviles plus
grand/plus de temps pour s'amorcer — pas retouchés ici (hors mandat, pas de symptôme de bug identifié).

**Instrumentation gardée** (gated `SCPS_SLAVEDIAG`, style maison, dans `scps_diplo.c` :
`diplo_settle` imprime `n`/`budget`/`conq_value` quand `winner_enslaves` ; `diplo_enslave_capture`
imprime `moved`/`captives`/`dst_n_groups` en sortie réussie) — complète l'instrumentation déjà en
place (`enslave_capture appelé`, les 2 gates d'échec, les 3 sites `settle(...)` dans `scps_ai.c`).
Utile pour la PROCHAINE fois qu'on voudra distinguer un vrai défaut de câblage d'un simple aléa de
guerre rare.

**NE PAS COMMITTER** (mandat explicite — même règle que [020]).

## 2026-07-06 (suite) — V2a LANDÉ : LE CONSEIL VIVANT (faction, loyauté, paie). SAVE v69→70.

**Le design** (verrouillé, `scps_ages_factions §3bis`) : chaque conseiller PENCHE vers une faction-éthos
(attribution DÉTERMINISTE par (siège, maison), rien à sérialiser) ; RECRUTER pousse SA faction, RENVOYER
froisse l'opposée ; une barre de LOYAUTÉ (0-100, par siège pourvu) CONVERGE (jamais un saut) vers une
cible dérivée de la satisfaction de SA faction (1−grief) et de la PAIE, le ROT (capture d'État) accélérant
la CHUTE mais JAMAIS la remontée (motif `COERCION_DECAY`) ; des lecteurs de SIGNAL (`betrayal_ready`,
`pair_state` rivalité/alliance/conspiration) posent l'état pour V2b (les événements de trahison/rivalité
ne sont PAS ce lot — V2a pose l'ÉTAT, V2b branchera le narratif dessus).

**Implémentation** : `statecraft_council_faction(seed,cid,seat,slot,gen)` — table `SEAT_A`/`SEAT_B` (Savoir→
Transgresseur/Légiste · Société→Conquérant/Communautaire · Industrie→Marchand seul) + hash de maison
(`sc_hash` réutilisé) qui tranche entre les deux candidats. `Statecraft` gagne `loyalty[][]`/`pay[][]`
(float, SAVE **69→70**, blob plat `fwrite BRUT`) — bornées par `save_sane` ([0,100]/[0,2]). `statecraft_
council_hire` applique `faction_lever_apply(cid,fac,COUNCIL_HIRE_LEVER)` (un vote pour SA faction, motif
§4 EXISTANT) ; `statecraft_council_dismiss` cherche la faction la PLUS OPPOSÉE à celle du congédié
(`faction_opposition`) et lui applique le grief (`COUNCIL_DISMISS_GRIEF`) — le canal le plus honnête de
l'API existante (documenté : « pousser l'opposée EST froisser celle qu'on renvoie »). La RETRAITE (âge)
reste un reset DIRECT (pas un « renvoi » — aucun grief joueur pour un départ naturel). `statecraft_council_
loyalty_tick` (mensuel, appelé depuis `sim_day` juste après `statecraft_council_ai`) : cible =
`(1−faction_grievance(cid,fac))·100 + (pay−1)·COUNCIL_PAY_ADJ`, taux = `COUNCIL_LOYAL_RATE·(1+
COUNCIL_ROT_BOOST·rot)` SEULEMENT quand la cible est SOUS la valeur courante (`tgt<cur` — la chute), le
rot ne touchant JAMAIS le taux de remontée. `statecraft_council_pair_state` : CONSPIRATION (griefA>0.6 ET
griefB>0.6) prioritaire sur RIVALITÉ (opposition≥0.6 + tenure>10 ans chacun) prioritaire sur ALLIANCE
(opposition<0.3 + grief bas des deux) sinon NEUTRE. `statecraft_council_ai` : l'IA paie 1.0 par défaut
(posé à l'embauche) et REMPLACE (dismiss+re-hire dans le même tick) un ministre `betrayal_ready` au lieu
de le garder — « elle ne subit pas la trahison narrative, c'est le lot du joueur ». Verbe `CMD_COUNCIL_PAY`
(a[0]=seat, a[1]=paie×100) + façade `scps_player_council_pay` (clampe [0,2] à l'enfilage ET revalidé au
drain) + `scps_council_pair_state` (lecteur, borné). `ScpsCouncilSeat` gagne `faction`/`loyalty`/`pay`/
`mood` (readout `council_mood_word`, 5 mots — dévoué/loyal/tiède/aigri/AU BORD DE LA TRAHISON, seuil ≤15
MIROIR de `betrayal_ready`). Godot : `country_council()` binding étendu, `player_council_pay`/`council_
pair_state` bindés, `sidebar_drawer.gd` onglet Conseil affiche la faction (mot), la BARRE (VKit.gauge,
rouge→vert), le mot d'ambiance, et 4 boutons de paie (0.5×/1×/1.5×/2×, verbe journalisé).

**Tunables registre J** (6) : `COUNCIL_HIRE_LEVER` 0.10 · `COUNCIL_DISMISS_GRIEF` 0.10 ·
`COUNCIL_LOYAL_RATE` 0.05 · `COUNCIL_ROT_BOOST` 1.5 · `COUNCIL_PAY_ADJ` 30.0 · `COUNCIL_BETRAYAL_
THRESHOLD` 15.0.

**Télémétrie chronicle** neuve « conseil (V2a) » : loyauté moyenne (sièges pourvus) · ministres au bord/sim
· remplacements IA/sim — compteur `statecraft_council_ai_replace_count()` (module-static, RAZ par
`statecraft_init`, même motif que `revolt_civilwar_count`). Mesuré (seeds 9/7 × 2 sims × 200 ans) :
loyauté moyenne 90-92/100 sur 12-13 sièges pourvus, 0 ministre au bord, 0 remplacement IA — un monde SAIN
ne stresse pas ses ministres par défaut (le rot/grief restent rares hors levier actif soutenu) ; le
mécanisme MORD quand testé isolément (statecraft_demo : grief saturé + rot élevé → loyauté <15 en
quelques mois).

⚠ **RE-BASELINE golden DÉLIBÉRÉE** (seed 7 INCHANGÉ ; seeds 108/209/310/411 changent — l'IA embauche/paie
tôt et la loyauté/les leviers de faction mordent dans la fenêtre 12 ans ; `golden_hashes.txt` mis à jour,
`make golden` re-vérifié OK). `determinism` **STABLE** (5 graines × 12 ans, hash identique — la loyauté est
état sérialisé pur, aucun flottant hors-sérialisation n'influence le tick). `savetest` seeds 9/11 **byte-
identiques** (v70). `fuzz-save` : 216 octets flippés, `save_sane` rejette chaque forge (loyalty/pay bornés),
0 crash. `make test` **39/40** (seul `intertrade_demo` KO, pré-existant Windows `setenv` — `campaign_demo`/
`warhost_demo` VERTS). `statecraft_demo` **+21 tests** (attribution déterministe · spectre par siège ·
convergence sans saut · asymétrie du rot chute/remontée · betrayal_ready vrai/faux/vacant · pair_state
3 états · la paie coûte et clampe). `scps_api_demo` **+8 tests** (lecture bornée, recruter→loyauté humaine,
paie posée/clampée, pair_state borné/hors-borne). Godot `scons` **0 warning**. Probe headless neuve
`council_audit.{gd,tscn}` (seeds 9/11/42, round-trip complet lecture→recrutement→paie→clamp→pair_state,
**CONSEIL AUDIT OK**). ⚠ **Piège de perf évité** : le premier jet de `scps_api_demo` créait un `ScpsSim`
NEUF (genèse complète, ~800 provinces) rien que pour les 8 tests V2a → +2 min de runtime (timeout du
harnais 120s dépassé). Fix : réutiliser le `sd` déjà généré par le bloc DÉCRETS précédent (même scope) —
zéro genèse supplémentaire. **SAVE non bumpé pour le reste** (rien d'autre ne change). **NE PAS COMMITTER**
(même règle que les entrées précédentes — mission d'implémentation, pas de commit).
