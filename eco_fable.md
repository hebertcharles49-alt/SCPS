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
  **§5** décoratif (cap ne mord jamais ; skip documenté) · **C3** orphelins RC (chantier de DESIGN, pas un
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
